/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/

#ifdef PHX_FELICA_TEST /* this whole file is only used for felica testing */

#include <string.h>
#include "phx_glbs.h"
#include "phx.h"
#include "usbdesc.h"
#include "usbdevice.h"
#include "ushx_api.h"
#include "rfid/rfid_hw_api.h"

#include "HID/GeneralDefs.h"

/* system context */
#include "HID/CardContext.h"
#include "HID/applHidContext.h"
#include "HID/brfd_api.h"
#include "HID/ChannelContext.h"

#include "sched_cmd.h"
#include "HID/byteorder.h"
#include "HID/ccidctless.h"
#include "HID/debug.h"
#include "ccidmemblock.h"
#include "volatile_mem.h"
#include "phx_upgrade.h"
#include "cvmain.h"
#include "console.h"
#include "cvapi.h"
#include "cvdiag.h"

extern RFID_STATUS	rfid_rd_transceive_frame(rfid_system *pSys, uint32_t nInFlag, uint32_t nWaitMS, uint32_t nInLen, uint8_t *pInData,
									uint32_t *pOutFlag, uint32_t *pOutLen, uint8_t **pOutData);


#define BYTE2WORD(p, w) {   w = (*p | (*(p+1) << 8) | (*(p+2) << 16) | (*(p+3) << 24)); }
#define WORD2BYTE(w, p) {   *(p+0) = (uint8_t)(w>>0);   \
							*(p+1) = (uint8_t)(w>>8);   \
							*(p+2) = (uint8_t)(w>>16);  \
							*(p+3) = (uint8_t)(w>>24);  \
						}



#define PREFIX_APDU_SIZE   5  /* CLA, INS, P0, P1, Lc */
#define PREFIX_FELICA      "FELICA "
#define PREFIX_FELICA_SIZE (sizeof(PREFIX_FELICA) - 1)

#define FELICA_POLL        "P"
#define FELICA_POLL_SIZE   (sizeof(FELICA_POLL) - 1)

#define FELICA_READWRITE        "R"
#define FELICA_READWRITE_SIZE   (sizeof(FELICA_READWRITE) - 1)

#define FELICA_INIT        "I"
#define FELICA_INIT_SIZE   (sizeof(FELICA_INIT) - 1)

#define FELICA_FRAME        "F"
#define FELICA_FRAME_SIZE   (sizeof(FELICA_FRAME) - 1)


#define RETCODE_SUCCESS     0x9000
#define RETCODE_FAIL        0x8000
#define RETCODE_UNKNOWN     0x6000

#define SET_RETURN_CODE(pLen, pData, retCode)		*pLen += 2;       \
													pData[0] = (retCode >> 8); \
													pData[1] = (retCode & 0xFF);


BERR_Code
BRFD_Channel_APDU_TransceiveBrcm(
	BRFD_ChannelHandle in_channelHandle,
	const BRFD_ChannelSettings *inp_channelDefSettings,
	unsigned char *inp_ucXmitData,  /* input */
	unsigned int in_ulNumXmitBytes,
	unsigned char *outp_ucRcvData,  /* output */
	unsigned int *outp_ulNumRcvBytes,
	unsigned int in_ulMaxReadBytes,
	unsigned int in_wLevelParameter,
	unsigned char *outp_bChainingParameter,
	void (*in_Callback)(BRFD_ChannelHandle channelHandle)
)
{
	uint8_t  errCode = 0;
	uint8_t *pTestDataStart;
	uint32_t nOutLen, nTmp, nTmpOut;
	cv_status status;

	if (in_ulNumXmitBytes >= (PREFIX_APDU_SIZE + PREFIX_FELICA_SIZE))
	{
		pTestDataStart = inp_ucXmitData + PREFIX_APDU_SIZE;
		if (!memcmp(pTestDataStart, PREFIX_FELICA, PREFIX_FELICA_SIZE))
		{
			/* This is a felica test packet */
			pTestDataStart += PREFIX_FELICA_SIZE;

			*outp_ulNumRcvBytes = 0;

			if (!memcmp(pTestDataStart, FELICA_POLL, FELICA_POLL_SIZE)) /* ask fw to do the polling test */
			{
				nTmp = CV_DIAG_RFID_FELICA;
				status = rfid_complex_diag(1, &nTmp, &nOutLen, &nTmpOut, 0);

				if (status == CV_SUCCESS)
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_SUCCESS)
				}
				else
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_FAIL)
				}

			}
			else if (!memcmp(pTestDataStart, FELICA_READWRITE, FELICA_READWRITE_SIZE)) /* ask fw to do the read/write test */
			{
				nTmp = CV_DIAG_RFID_FELICA_RW;
				status = rfid_complex_diag(1, &nTmp, &nOutLen, &nTmpOut, 0);

				if (status == CV_SUCCESS)
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_SUCCESS)
				}
				else
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_FAIL)
				}

			}


			else if (!memcmp(pTestDataStart, FELICA_INIT, FELICA_INIT_SIZE)) /* init only, host will send command later */
			{
				nTmp = CV_DIAG_RFID_FELICA_RW;
				status = rfid_complex_diag(1, &nTmp, &nOutLen, &nTmpOut, 1);

				if (status == CV_SUCCESS)
				{
					/* nTmpOut has the pointer in stack to the rfid context */
					/* we disable HID polling so it will not interfere with our test, also we can re-use hid memory for our context */
					VOLATILE_MEM_PTR->rfid_hid_poll_enable = 0;
//					memcpy(VOLATILE_MEM_PTR->rfid_system_ptr, (uint32_t *)nTmpOut, sizeof(rfid_system));

					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_SUCCESS)
				}
				else
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_FAIL)
				}

			}
			else if (!memcmp(pTestDataStart, FELICA_FRAME, FELICA_FRAME_SIZE))
			{
				uint32_t nInFlag, nInWait, nInLen;
				uint32_t nOutFlag;
				uint8_t *pOutData;

				pTestDataStart += FELICA_FRAME_SIZE;

				BYTE2WORD(pTestDataStart, nInFlag);
				pTestDataStart += 4;

				BYTE2WORD(pTestDataStart, nInWait);
				pTestDataStart += 4;

				BYTE2WORD(pTestDataStart, nInLen);
				pTestDataStart += 4;

				status = rfid_rd_transceive_frame((rfid_system *)VOLATILE_MEM_PTR->rfid_system_ptr,
										nInFlag, nInWait, nInLen, pTestDataStart,
										&nOutFlag, &nOutLen, &pOutData);

				if (status == CV_SUCCESS)
				{
					/* check for flag, if overrun need to reduce the length, otherwise too big for TPDU */
					if (nOutFlag & RFID_FRAME_RXFLAG_ERR_OVERRUN)
						nOutLen -= 20; /* nOutLen would be 256 */

					WORD2BYTE(nOutFlag, outp_ucRcvData)
					outp_ucRcvData += 4;

					WORD2BYTE(nOutLen,  outp_ucRcvData)
					outp_ucRcvData += 4;

					if (pOutData)
					{
						memcpy(outp_ucRcvData, pOutData, nOutLen);
						outp_ucRcvData += nOutLen;
					}

					*outp_ulNumRcvBytes += (4 + 4 + nOutLen);

					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_SUCCESS) /* last 2 bytes are the APDU return code */
				}
				else
				{
					SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_FAIL)
				}

			}
			else
			{
				SET_RETURN_CODE(outp_ulNumRcvBytes, outp_ucRcvData, RETCODE_UNKNOWN)
			}

			return errCode;

		}
	}

	/* no, this belongs to normal handling */
	errCode = BRFD_Channel_APDU_TransceiveNew(in_channelHandle,
			inp_channelDefSettings, inp_ucXmitData, in_ulNumXmitBytes, outp_ucRcvData,
			outp_ulNumRcvBytes, in_ulMaxReadBytes,
			in_wLevelParameter,
			outp_bChainingParameter,
			in_Callback
			);

	return errCode;
}



#endif /* PHX_FELICA_TEST */
