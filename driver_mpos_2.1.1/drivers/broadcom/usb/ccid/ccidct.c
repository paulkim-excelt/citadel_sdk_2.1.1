/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/


#include "console.h"
#include "bstd.h"
#include "bkni.h"
#include "bint.h"
#include "berr_ids.h"
#include "bscd.h"
#include "bscd_cli_states.h"
#include "bscd_cli_infra.h"
#include "bscd_cli_util.h"
#include "bscd_datatypes.h"
#include "bscd_priv.h"
#include "ccidclass.h"
#include "usbdevice.h"
#include "ccidmemblock.h"
#include "volatile_mem.h"
#include "HID/byteorder.h"
#include "cvinternal.h"

#include "sched_cmd.h"
#include "phx_upgrade.h"
#include "board.h"

#define CCIDInterface		(pccid_mem_block->CCIDInterface)
#define CCIDNotifyBufferIn	(pccid_mem_block->CCIDNotifyBufferIn)
#define CCIDMsgBufferOut	(pccid_mem_block->CCIDMsgBufferOut)
#define CCIDMsgBufferIn		(pccid_mem_block->CCIDMsgBufferIn)
#define CCIDCmdMsg		(pccid_mem_block->CCIDCmdMsg)
#define wRcvLength		(pccid_mem_block->wRcvLength)
#define gSmartCard		(pccid_mem_block->gSmartCard)
#define bActiveStatus		(pccid_mem_block->bActiveStatus)
#define bClockStatus		(pccid_mem_block->bClockStatus)
#define bSlotChange		(pccid_mem_block->bSlotChange)
#define bRcvStatus		(pccid_mem_block->bRcvStatus)
#define bCCIDMode		(pccid_mem_block -> bCCIDMode)
#define ccid_atr        (pccid_mem_block->ccid_atr)

#define CT_TX_WAIT	(200)

// CCID spec 4.2.2.2:
#define ICC_CLOCK_RUNNING 0
#define ICC_CLOCK_STOPPED_L 1
#define ICC_CLOCK_STOPPED_H 2
#define ICC_CLOCK_STOPPED_UNKNOWN 3


#define SLOT0_PRESENT      1  // current status, card present
#define SLOT0_NOTPRESENT   0  // current status, card not present
#define SLOT0_CHANGED      2  // slot card status changed



#define CCID_INTERRUPTIN   6
#define CCID_BULKIN        2
#define CCID_BULKOUT       2


#define CCID_DEBUG 0
#define DEBUG_OUTPUT_LEN 50


/* Parameter updating debug support */
#if 1
#define UPDATE_SETTING(field, value)	do { \
						if (value != field) { \
							printf("ccidct change " #field " from %d to %d\n", field, value); \
							field = value; \
						} \
					} while (0)
#else
#define UPDATE_SETTING(field, value)
#endif

#define TA		0x1
#define TB		0x2
#define TC		0x4
#define TD		0x8
#define TD_COUNT	16		/* Maximum of 16 TD bytes*/

#define CLASSA	0x1			/* 5V */
#define CLASSB	0x2			/* 3V */
#define CLASSC	0x4			/* 1.8V */

/* Some function Prototypes */
void CardInsertNotify(BSCD_ChannelHandle  in_channelHandle, void *inp_data); 
void CardRemoveNotify(BSCD_ChannelHandle  in_channelHandle, void *inp_data);
uint8_t handleSCPowerOff(void);
void ccidGeneratePPS( uint8_t *pps, uint32_t *dwLen );
/*******BulkOut Command Message *************/

void ParsePCtoRDRMessage(uint32_t *dwLen, uint8_t *bSlot, uint8_t *bSeq)
{
	uint8_t *pBuf = CCIDMsgBufferOut + EXTRA_BYTES_OFFSET;
	*dwLen = usb_to_cpu32up(pBuf);
	pBuf = CCIDMsgBufferOut + SLOT_OFFSET;
	*bSlot = *pBuf;
	pBuf = CCIDMsgBufferOut + SEQUENCE_OFFSET;
	*bSeq = *pBuf;
}

/********* BulkIn Response Message *************/

void RDRtoPCMessageHeader(uint8_t bMsgType, uint32_t dwLen, uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError)
{
	uint8_t *pBuf = CCIDMsgBufferIn + COMMAND_OFFSET;
	*pBuf = bMsgType;
	pBuf = CCIDMsgBufferIn + EXTRA_BYTES_OFFSET;
	*pBuf++ = (uint8_t) dwLen;
	*pBuf++ = (uint8_t)(dwLen >>8);
	*pBuf++= (uint8_t)(dwLen>>16);
	*pBuf = (uint8_t) (dwLen >>24);
	pBuf = CCIDMsgBufferIn + SLOT_OFFSET;
	*pBuf = bSlot;
	pBuf = CCIDMsgBufferIn + SEQUENCE_OFFSET;
	*pBuf = bSeq;
	pBuf = CCIDMsgBufferIn + STATUS_OFFSET;
	*pBuf = bStatus;
	pBuf = CCIDMsgBufferIn + CCID_ERROR_OFFSET;
	*pBuf = bError;
}

void CCIDsendDataEndpoint(uint8_t EpNum, uint8_t *pBuffer, uint16_t wlen)
{
#if CCID_DEBUG
	uint32_t i;
	get_ms_ticks(&i);
   	printf("\nRD2PC(%d, %d): ", i, cvGetDeltaTime(VOLATILE_MEM_PTR->ccidct_msg_ts));
    for (i=0; i < (wlen > DEBUG_OUTPUT_LEN ? DEBUG_OUTPUT_LEN: wlen); i++)
      	printf("%2x ", *(pBuffer + i));
			printf("\n");
#endif

	if (EpNum == CCID_BULKIN) {
		/* allow previous transfer (if any) to complete */
		if (CCIDInterface.BulkInEp)
		{
			usbTransferComplete(CCIDInterface.BulkInEp, CT_TX_WAIT);
			sendEndpointData(CCIDInterface.BulkInEp, pBuffer, wlen);
			/* allow this transfer to complete */
			usbTransferComplete(CCIDInterface.BulkInEp, CT_TX_WAIT);
		}
	}
	if (EpNum == CCID_INTERRUPTIN) {
#if CCID_DEBUG
	        printf ("^\n");
#endif
		if (CCIDInterface.IntrInEp)
		{
			/* allow previous transfer (if any) to complete */
			/* Don't wait for the card present data to be picked by the host.
			 The host might not have a ccid driver(eg. DOS) or ccid driver might not be ready.
			 The host would pick it when it boots up. -- leave it as is*/
			/* removing the timeout ...DOS does not like unwanted waits */
			//usbTransferComplete(CCIDInterface.IntrInEp, CT_TX_WAIT);
			sendEndpointData(CCIDInterface.IntrInEp, pBuffer, wlen);
			/* allow this transfer to complete */
			//usbTransferComplete(CCIDInterface.IntrInEp, CT_TX_WAIT);
		}
	}
}


uint8_t ct_GetICCStatus(void)
{
    return bActiveStatus;
}


uint8_t ICCClockStatus(uint8_t bChange)
{
	switch(bChange) {
	case PC_to_RDR_IccPowerOff:
		bClockStatus = ICC_CLOCK_STOPPED_L;
		break;
	case PC_to_RDR_IccPowerOn:
		bClockStatus = ICC_CLOCK_RUNNING;
		break;
	default:
		printf("ct: unsupported ICCClockStatus\n");
		break;
   }
   return bClockStatus;
}

/*
   Copy the ATR to the pointer dest, return
   the length of the ATR.
 */
uint8_t CopyATR(uint8_t *dest)
{
	uint8_t bindex;
	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;

	for (bindex = 0; bindex < in_channelHandle->ulRxLen; bindex++)
		*dest++ = in_channelHandle->aucRxBuf[bindex];

#if CCID_DEBUG
	printf("ct: ATR = ");
	for (bindex = 0; bindex < in_channelHandle->ulRxLen; bindex++)
		printf("%02x ", in_channelHandle->aucRxBuf[bindex]);
	printf("\n");
#endif	

	return (in_channelHandle->ulRxLen);
}

/*****************Interrupt In Endpoint ****************/

void NotifySlotChange(uint8_t bSlotNumber, uint8_t status)
{
	uint8_t *pBuf = CCIDNotifyBufferIn;
	*pBuf++ = RDR_to_PC_NotifySlotChange;
	*pBuf = status;
	CCIDsendDataEndpoint(CCID_INTERRUPTIN, CCIDNotifyBufferIn, 2);
}

void NotifyHardwareError(uint8_t bSlotNumber, uint8_t bSeq, uint8_t bHwErrCode)
{
	uint8_t *pBuf = CCIDNotifyBufferIn;

	printf("ct: NotifyHardwareError\n");
	*pBuf++= RDR_to_PC_HardwareError;
	*pBuf++ = bSlotNumber;
	*pBuf++ = bSeq;
	*pBuf = bHwErrCode;
	CCIDsendDataEndpoint(CCID_INTERRUPTIN, CCIDNotifyBufferIn, 4);
}

void handleUnknownCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLength = 0;

	printf("ct: handleUnknownCmd\n");

	ParsePCtoRDRMessage(&dwLength, &bSlot, &bSeq);

	bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
	bErr     = SLOTERR_CMD_NOT_SUPPORTED;

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, 0, bSlot, bSeq, bStatus, bErr);

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void ccidctSendWTXIndication(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLength = 0;

#if CCID_DEBUG
	printf("ct: ccidctSendWTXIndication\n");
#endif

	ParsePCtoRDRMessage(&dwLength, &bSlot, &bSeq);

	bStatus  = (CMDSTATUS_TIMEEXT | bActiveStatus);
	bErr     = 1; /* in WTX case, error is used to indicate how many WWT the card needs */

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, 0, bSlot, bSeq, bStatus, bErr);

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

uint8_t handleSCPowerOn(void) {
	uint8_t errCode = 0;

	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;

	if (bActiveStatus != ICCSTATUS_ABSENT)
	{
		errCode = BSCD_Channel_ResetIFD(in_channelHandle, BSCD_ResetType_eCold);
		if (errCode == BERR_SUCCESS)
			errCode = BSCD_Channel_SetParameters(in_channelHandle, &gSmartCard.cliChannel.channelSettings);
		if (errCode == BERR_SUCCESS)
			errCode = BSCD_Channel_ResetCard(in_channelHandle, gSmartCard.cliChannel.channelSettings.resetCardAction);
	}

	return errCode;

}

void handleIccPowerOnCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint32_t dwLen = 0;
	uint8_t errCode = 0;
	uint8_t bStatus, bErr;
	uint8_t bCount = 0;
	uint8_t *pBuf = CCIDMsgBufferIn + MESSAGE_HEADER;
	int voltage;
	uint8_t cardClass;

	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;

	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		/* Loop through the sc voltages */
		for (voltage = 1; voltage >= 0; voltage--)		/* Cycle through voltages (Only trying 3V & 5V for now). 2 = 1.8V, 1 = 3V, 0 = 5V */
		{
			gSmartCard.cliChannel.channelSettings.ctxCardType.eVccLevel = (BSCD_VccLevel)voltage;
			gSmartCard.cliChannel.channelHandle->currentChannelSettings.ctxCardType.eVccLevel = (BSCD_VccLevel)voltage;
			
			BSCD_Channel_SetVccLevel( in_channelHandle, (BSCD_VccLevel)voltage, true);
		
			errCode = BSCD_Channel_ResetIFD(in_channelHandle, BSCD_ResetType_eCold);
			if (errCode == BERR_SUCCESS)
				errCode = BSCD_Channel_SetParameters(in_channelHandle, &gSmartCard.cliChannel.channelSettings);
			if (errCode == BERR_SUCCESS)
				errCode = BSCD_Channel_ResetCard(in_channelHandle, gSmartCard.cliChannel.channelSettings.resetCardAction);

			if (errCode == BERR_SUCCESS) {	/* Successfully initialized card at the given voltage */
				bCount = CopyATR(pBuf);
				/* Parse ATR */
				if (parseCtATR(pBuf, bCount, &cardClass) == 0) 	/* Class information is present */
				{
					/* Check to see if correct voltage */
					if ((voltage == 0) && (cardClass & CLASSA)) {
						printf("ct: Selected CLASSA\n");
						break;			/* ClassA supported, all done */
					}
					else if ((voltage == 1) && (cardClass & CLASSB)) {
						printf("ct: Selected CLASSB\n");
						break;			/* ClassB supported, all done */
					}
					else if ((voltage == 0) && (cardClass & CLASSC)) {
						printf("ct: Selected CLASSC\n");
						break;			/* ClassC supported, all done */
					}
				}
				else /* Class information is not present */
				{
					printf("ct: No class information on smartcard\n");
					break;
				}
			}
			else {/* Power Off */
				handleSCPowerOff();
				BKNI_Delay(1000);
			}	
		}
		
		if (voltage >= 0)
	    {	printf("ct: Voltage set to %d\n", voltage);    }
		else
		{	printf("ct: Card could not be initialized\n"); }

		// copy the ATR into the buffer
		/* Only copy when there's no error. Actually this is too strict since CCID allows to return ATR parsing errors.
		   But we can not distinguish if it's card activation error (no ATR received) or ATR parsing error.
		*/
		if (errCode == BERR_SUCCESS)
			bCount = CopyATR(pBuf);

		bActiveStatus = ICCSTATUS_ACTIVE;

		switch (errCode) {
		case 0:
			bErr     = SLOTERR_NONE;
			bStatus  = CMDSTATUS_OK | ICCSTATUS_ACTIVE;
			break;

		case 0x0d: // timeout
			printf("ct: IccPowerOnCmd timeout\n");
			bErr     = SLOTERR_ICC_MUTE;
			bStatus  = CMDSTATUS_OK | ICCSTATUS_INACTIVE;
			break;

		case 0x07: // card backwards?
			bErr     = SLOTERR_ICC_MUTE;
			bStatus  = CMDSTATUS_FAIL | ICCSTATUS_INACTIVE;
			break;

		default:
			printf("ct: card reset error %d\n", errCode);
			bErr     = SLOTERR_ICC_MUTE;
			bStatus  = CMDSTATUS_FAIL | ICCSTATUS_INACTIVE;
			break;
		}
	}

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, bCount, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER+bCount);
}

void handleXfrBlockCmd(void)
{
	uint8_t bSeq = *(CCIDMsgBufferOut + SEQUENCE_OFFSET);
	uint8_t bSlot = *(CCIDMsgBufferOut + SLOT_OFFSET);
	uint8_t bStatus, bErr;
	uint8_t bBWI = 0; /* the value contained in the CCID msg */
	uint32_t dwBWT;
	uint8_t *pBuf;
	uint32_t dwMsgActLength = 0;
	unsigned long dwRetLen = 0;
	uint8_t *pBufIn = CCIDMsgBufferIn + MESSAGE_HEADER;
	uint8_t errCode = 0;

	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;

	// get HOST CCID DATA HERE
	pBuf = CCIDMsgBufferOut + EXTRA_BYTES_OFFSET;
	dwMsgActLength = (*pBuf) | (*(pBuf+1)) << 8 | (*(pBuf+2)) << 16 | (*(pBuf+3)) << 24;
	pBuf = CCIDMsgBufferOut + MESSAGE_HEADER;


	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		dwRetLen = 0;
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		// set BWI for T=1, since we are TPDU based (CCID $6.1.4)
		if (in_channelHandle->currentChannelSettings.eProtocolType == BSCD_AsyncProtocolType_e1)
		{
			/* We have BWT and BWT extension two fields in the channel setting, we only change the extension field here */
			bBWI = *(CCIDMsgBufferOut + BWI_OFFSET);
			BSCD_Channel_GetBlockWaitTime   (in_channelHandle, &dwBWT);
			BSCD_Channel_SetBlockWaitTimeExt(in_channelHandle, bBWI * dwBWT); /* If bBWI is 0 this will set BWT Extension to 0 */
		}

		errCode = BSCD_Channel_TPDU_Transceive(in_channelHandle, &in_channelHandle->currentChannelSettings,
							pBuf, dwMsgActLength, pBufIn, &dwRetLen, BSCD_MAX_RX_SIZE);

		switch (errCode) {
		case BERR_SUCCESS:
		case BSCD_STATUS_ATR_SUCCESS:
			bStatus  = (CMDSTATUS_OK | ICCSTATUS_ACTIVE);
			bErr     = SLOTERR_NONE;
			break;

		case BSCD_STATUS_NO_SC_RESPONSE:
		case BERR_STATUS_FAILED:
		default:
			bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_INACTIVE);
			bErr     = SLOTERR_ICC_MUTE;
			printf("ct: handleXfrBlockCmd error=%08x seq=%d\n", errCode, bSeq);
			break;
		}
	}

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, dwRetLen, bSlot, bSeq, bStatus, bErr);

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER+dwRetLen);

}

uint8_t handleSCPowerOff(void) {
	uint8_t errCode = 0;

	if (bActiveStatus != ICCSTATUS_ABSENT)
	{
		errCode = (uint8_t)BSCD_Channel_PowerICC(gSmartCard.cliChannel.channelHandle, BSCD_PowerICC_ePowerDown);

	}

	return errCode;
}

void handleIccPowerOffCmd(void)
{
	uint8_t bSeq = *(CCIDMsgBufferOut + SEQUENCE_OFFSET);
	uint8_t bSlot = *(CCIDMsgBufferOut + SLOT_OFFSET);
	uint8_t bStatus, bErr;
	uint8_t *pBuf;
	uint8_t errCode = 0;

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		errCode = BSCD_Channel_PowerICC(gSmartCard.cliChannel.channelHandle, BSCD_PowerICC_ePowerDown);

		if (errCode == BERR_SUCCESS)
		{
			bActiveStatus = ICCSTATUS_INACTIVE;

			bStatus  = (CMDSTATUS_OK | ICCSTATUS_INACTIVE);
			bErr     = SLOTERR_NONE;
		}
		else
		{
			bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_INACTIVE);
			bErr     = SLOTERR_ICC_MUTE;
		}
	}


	RDRtoPCMessageHeader(RDR_to_PC_SlotStatus, 0, bSlot, bSeq, bStatus, bErr);

	pBuf = CCIDMsgBufferIn + CLOCK_STATUS_OFFSET;
	*pBuf = ICC_CLOCK_STOPPED_L;

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

#define PROTOCOL_T0 0
#define PROTOCOL_T1 1

// host wants to set a bunch of parameters about which we don't care.
// host wants us to echo the parameters back.  do so here.
void handleSetParametersCmd(void)
{
	uint8_t *pBuf = CCIDMsgBufferOut + MESSAGE_HEADER;
	uint32_t errCode = 0;
	uint32_t dwLength = 0;
	uint8_t bSlot, bSeq, bProtocolNum;
	uint8_t bStatus, bErr = SLOTERR_NONE;
	BSCD_ChannelSettings *settings;
	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;
	bool    bNeedReset = false;
	bool    bFDupdated = false;

	ParsePCtoRDRMessage(&dwLength, &bSlot, &bSeq);
	
	bProtocolNum = *(CCIDMsgBufferOut + PROTOCOL_OFFSET);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
		goto reply;
	}

	errCode = BSCD_Channel_GetNegotiateParametersPointer(in_channelHandle, &settings);
	if (errCode != BERR_SUCCESS) {
		printf("ct: cannot get parameters %d\n", errCode);
		bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
		bErr     = SLOTERR_HW_ERROR;
		goto reply;
	}

	// check if the protocol needs to be changed, if so we need to reset the card
	if ((bProtocolNum == PROTOCOL_T0) && (settings->eProtocolType != BSCD_AsyncProtocolType_e0))
	{
		bNeedReset = true;
	}
	if ((bProtocolNum == PROTOCOL_T1) && (settings->eProtocolType != BSCD_AsyncProtocolType_e1))
	{
		bNeedReset = true;
	}

	// check if the F/D need to be changed, if so we need to reset the card
	if ( (settings->ucFFactor != CCID_GET_F(pBuf[0])) || (settings->ucDFactor != CCID_GET_D(pBuf[0])))
	{
		bNeedReset = true;
		bFDupdated = true;
	}

	/* Reactivate card if there's need. This will re-assign NegotiatedParameters */
	if (bNeedReset == true)
	{
		printf("ccidct: reset card since parameter changed\n");

		errCode = BSCD_Channel_ResetIFD(in_channelHandle, BSCD_ResetType_eCold);
		if (errCode == BERR_SUCCESS)
			errCode = BSCD_Channel_SetParameters(in_channelHandle, &gSmartCard.cliChannel.channelSettings);
		if (errCode == BERR_SUCCESS)
			errCode = BSCD_Channel_ResetCard(in_channelHandle, gSmartCard.cliChannel.channelSettings.resetCardAction);
	}

	/* Now we can update the values contained in the NegotiatedParameters */
	/* Update F/D */
	if ((bFDupdated == true) && (errCode == BERR_SUCCESS))
		errCode = BSCD_P_FDAdjust_WithoutRegisterUpdate(settings, CCID_GET_F(pBuf[0]), CCID_GET_D(pBuf[0]));


	if (errCode != BERR_SUCCESS)
	{
		printf("ccidct: SetParameter failed\n");
		bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
		bErr     = SLOTERR_HW_ERROR;
		goto reply;
	}

	// Protocol specific settings
	switch (bProtocolNum) {
	case PROTOCOL_T0:
		printf("ct: handleSetParametersCmd T0 %02x %02x %02x %02x %02x\n",
			pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4]);

		// per CCID spec. 6.1.7
		// protocol number
		UPDATE_SETTING(settings->eProtocolType, BSCD_AsyncProtocolType_e0);

		// bmGuardTimeT0 (FF is same as 0), FFh is the same as 00h.
		UPDATE_SETTING(settings->extraGuardTime.ulValue, (pBuf[2] == 0xff) ? 0 : pBuf[2]);

		break;

	case PROTOCOL_T1:
		printf("ct: handleSetParametersCmd T1 %02x %02x %02x %02x %02x %02x %02x\n",
			pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4], pBuf[5], pBuf[6]);

		// per CCID spec. 6.1.7
		// protocol number
		UPDATE_SETTING(settings->eProtocolType, BSCD_AsyncProtocolType_e1);

		// bmGuardTimeT1 (offset 12), If value is FFh, then guardtime is reduced by 1 etu.
		if (pBuf[2] == 0xff)
		{
			if (settings->extraGuardTime.ulValue > 0)
				UPDATE_SETTING(settings->extraGuardTime.ulValue, settings->extraGuardTime.ulValue-1);
			else
				UPDATE_SETTING(settings->extraGuardTime.ulValue, 0);
		}
		else
			UPDATE_SETTING(settings->extraGuardTime.ulValue, pBuf[2]);

		// BWI (offset 13, bit [7-4])
		BSCD_Channel_SetBlockWaitTimeInteger(settings, (pBuf[3]>>4));

		// CWI (offset 13, bit [3-0])
		UPDATE_SETTING(settings->ulCharacterWaitTimeInteger, pBuf[3]&0x0F);

		// bIFSC (offset 15)
		UPDATE_SETTING(settings->unCurrentIFSC, pBuf[5]);

		// bNadValue (not supported)

		break;

	default:
		printf("ct: unknown protocol %d\n", bProtocolNum);
		bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
		bErr     = SLOTERR_HW_ERROR;
		goto reply;
	}

	/* do PPS if it's not done, this can happen when:
	   either there's no cv sc access after PowerOn and before SetP,
	   or there's such cv sc access but the protocol changed (like we set to t=0 but ccid want to use t=1
	*/
	if (BSCD_Channel_IsPPSDone(in_channelHandle) == false)
	{
		errCode = BSCD_Channel_PPS(in_channelHandle);
		if (errCode == BERR_STATUS_FAKE_FAILED)
		{
			printf("ccidct: PPS fake failed\n");
			bStatus  = (CMDSTATUS_OK | bActiveStatus);
			goto reply;
		}
		else if (errCode != BERR_SUCCESS)
		{
			printf("ccidct: PPS failed\n");
			bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
			bErr     = SLOTERR_HW_ERROR;
			goto reply;
		}
	}
	else
		printf("ccidct: PPS skipped since it's done\n");

	/* Host can send SetP to change IFSC, after the data layer communication has started */
	/* Also the card can only offer T=1 with Fd Dd, so PPS can be skipped, however we still need to SetP */
	errCode = BSCD_Channel_SetParameters(in_channelHandle, settings);
	if (errCode == BERR_SUCCESS)
    {	printf("ccidct: PPS ok, SetP ok\n");   }
	else
	{	printf("ccidct: PPS ok, SetP fail\n"); }

	bStatus  = (CMDSTATUS_OK | bActiveStatus);


reply:
	memcpy(CCIDMsgBufferIn, CCIDMsgBufferOut, wRcvLength);

	RDRtoPCMessageHeader(RDR_to_PC_Parameters, dwLength, bSlot, bSeq, bStatus, bErr);

    CCIDMsgBufferIn[RES_PROTOCOL_OFFSET] = bProtocolNum; 

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, wRcvLength);
}

void handleGetSlotStatusCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLen = 0;

	printf("ct: NYI handleGetSlotStatusCmd\n");
	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

	/* Return CMDSTATUS_OK as well if there's no card */
	bStatus  = (CMDSTATUS_OK | bActiveStatus);
	bErr     = SLOTERR_NONE;

	RDRtoPCMessageHeader(RDR_to_PC_SlotStatus, 0, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void handleEscapeCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLength = 0;

	printf("ct: NYI handleEscapeCmd\n");
	ParsePCtoRDRMessage(&dwLength, &bSlot, &bSeq);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	RDRtoPCMessageHeader(RDR_to_PC_Escape, 0, bSlot, bSeq, bStatus, bErr);

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void stopClock(void)
{
	printf("ct: NYI stopClock\n");
}

void restartClock (void)
{
	printf("ct: NYI restartClock\n");
}

void handleIccClockCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLen = 0;
	uint8_t bClockCommand = *(CCIDMsgBufferOut + PROTOCOL_OFFSET);

	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);
	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;

		if (bClockCommand == 1)
			stopClock();
		if (bClockCommand == 0)
			restartClock();
	}

	RDRtoPCMessageHeader(RDR_to_PC_SlotStatus, 0, bSlot, bSeq, bStatus, bErr);

	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);

}

void handleT0APDUCmd(void)
{
	uint8_t bSeq = *(CCIDMsgBufferOut + SEQUENCE_OFFSET);
	uint8_t bSlot = *(CCIDMsgBufferOut + SLOT_OFFSET);
	uint8_t *pBuf;
	uint32_t dwMsgActLength;
	unsigned long dwRetLen = 0;
	uint8_t *pBufIn = CCIDMsgBufferIn + MESSAGE_HEADER;
	uint8_t bStatus, bErr;
	uint8_t errCode = 0;

	printf("ct: NYI handleT0APDUCmd\n");
	BSCD_ChannelHandle in_channelHandle = gSmartCard.cliChannel.channelHandle;

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{

		// get HOST CCID DATA HERE
		pBuf = CCIDMsgBufferOut + EXTRA_BYTES_OFFSET;
		dwMsgActLength = (uint32_t) *pBuf;
		pBuf = CCIDMsgBufferOut + MESSAGE_HEADER;
		errCode = BSCD_Channel_TPDU_Transceive(in_channelHandle,&gSmartCard.cliChannel.channelSettings,
								   pBuf, dwMsgActLength, pBufIn, &dwRetLen, BSCD_MAX_RX_SIZE);

		if (errCode != BERR_SUCCESS)
		{
			bStatus  = (CMDSTATUS_FAIL | bActiveStatus);
			bErr     = SLOTERR_HW_ERROR;
		}
		else
		{
			bStatus  = (CMDSTATUS_OK | bActiveStatus);
			bErr     = SLOTERR_NONE;
		}
	}

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, dwRetLen, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER+dwRetLen);
}

void handleGetParameterCmd (void)
{
    uint8_t bSeq = 0;
    uint8_t bSlot = 0;
    uint8_t bStatus, bErr;
    uint32_t dwLen = 0;

    printf("ct: NYI handleGetParameterCmd\n");
    ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

    if (bActiveStatus == ICCSTATUS_ABSENT)
    {
        bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
        bErr     = SLOTERR_ICC_MUTE;
    }
    else
    {
        bStatus  = (CMDSTATUS_OK | bActiveStatus);
        bErr     = SLOTERR_NONE;
    }

   /* generate pps based on ATR data */
    ccidGeneratePPS (CCIDMsgBufferIn, &dwLen);

    RDRtoPCMessageHeader(RDR_to_PC_Parameters, dwLen, bSlot, bSeq, bStatus, bErr);
    CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER + dwLen);
}

void handleResetParametersCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLen = 0;

	printf("ct: NYI handleResetParametersCmd\n");
	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	RDRtoPCMessageHeader(RDR_to_PC_Parameters, 0, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void handleSecureCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint32_t dwLen = 0;
//	uint16_t wLevelParameter = (uint16_t) (*(CCIDMsgBufferOut + 8));
	uint8_t bStatus, bErr;

	printf("ct: NYI handleSecureCmd\n");
	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	RDRtoPCMessageHeader(RDR_to_PC_DataBlock, 0, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void handleMechanicalCmd(void)
{
	uint8_t bSeq = *(CCIDMsgBufferOut + SEQUENCE_OFFSET);
	uint8_t bSlot = *(CCIDMsgBufferOut + SLOT_OFFSET);
	uint8_t bStatus, bErr;

	printf("ct: NYI handleMechanicalCmd\n");
#if 0
	switch (bFunc)
	{
	case 0x01:
		AcceptCard();
		break;
	case 0x02:
		EjectCard();
		break;
	case 0x03:
		CaputureCard();
		break;
	case 0x04:
		LockCard();
		break;
	case 0x05:
		UnlockCard();
		break;
	default:
		;
	}
#endif

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	RDRtoPCMessageHeader(RDR_to_PC_SlotStatus, 0, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void handleAbortCmd(void)
{
	uint8_t bSeq = *(CCIDMsgBufferOut + SEQUENCE_OFFSET);
	uint8_t bSlot = *(CCIDMsgBufferOut + SLOT_OFFSET);
	uint8_t bStatus, bErr;

	printf("ct: NYI handleAbortCmd\n");

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	RDRtoPCMessageHeader(RDR_to_PC_SlotStatus, 0, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, MESSAGE_HEADER);
}

void handleSetDataRateAndClockFreqCmd(void)
{
	uint8_t bSeq = 0;
	uint8_t bSlot = 0;
	uint8_t bStatus, bErr;
	uint32_t dwLen = 0;

	printf("ct: NYI handleSetDataRateAndClockFreqCmd\n");

	ParsePCtoRDRMessage(&dwLen, &bSlot, &bSeq);

	if (bActiveStatus == ICCSTATUS_ABSENT)
	{
		bStatus  = (CMDSTATUS_FAIL | ICCSTATUS_ABSENT);
		bErr     = SLOTERR_ICC_MUTE;
	}
	else
	{
		bStatus  = (CMDSTATUS_OK | bActiveStatus);
		bErr     = SLOTERR_NONE;
	}

	memcpy(CCIDMsgBufferIn, CCIDMsgBufferOut, wRcvLength);

	RDRtoPCMessageHeader(RDR_to_PC_DataRateAndClockFrequency, dwLen, bSlot, bSeq, bStatus, bErr);
	CCIDsendDataEndpoint(CCID_BULKIN, CCIDMsgBufferIn, wRcvLength);
}

void (*const CmdAction[])(void) =
{
	handleUnknownCmd,	 //                                             0x60   :N/A
	handleSetParametersCmd,  //PC_to_RDR_SetParameters  	        	0x61   :V
	handleIccPowerOnCmd ,	 //PC_to_RDR_IccPowerOn                   	0x62   :V
	handleIccPowerOffCmd,	 //PC_to_RDR_IccPowerOff                   	0x63   :V
	handleUnknownCmd,	 //                                             0x64   :N/A
	handleGetSlotStatusCmd,	 //PC_to_RDR_GetSlotStatus                	0x65   :X
	handleUnknownCmd,	 //                                             0x66   :N/A
	handleUnknownCmd,	 //                                             0x67   :N/A
	handleUnknownCmd,	 //                                             0x68   :N/A
	handleSecureCmd,	 //PC_to_RDR_Secure	   			0x69   :X
	handleT0APDUCmd,	 //PC_to_RDR_T0APDU 	               		0x6A   :X
	handleEscapeCmd,	 //PC_to_RDR_Escape		       		0x6B   :X
	handleGetParameterCmd ,	 //PC_to_RDR_GetParameters	       		0x6C   :X
	handleResetParametersCmd,//PC_to_RDR_ResetParameters          	        0x6D   :X
	handleIccClockCmd,       //PC_to_RDR_IccClock		       		0x6E   :X
	handleXfrBlockCmd,	 //PC_to_RDR_XfrBlock		       		0x6F   :V
 	handleUnknownCmd,	 //					        0x70   :N/A
	handleMechanicalCmd,	 //PC_to_RDR_Mechanical	        		0x71   :X
	handleAbortCmd,		 //PC_to_RDR_Abort		               	0x72   :X
	handleSetDataRateAndClockFreqCmd //PC_to_RDR_SetDataRateAndClockFrequency     0x73    :VX
};

#define CCID_CMD  0x00
#define CCID_DAT  0x02
#define CCID_EXC  0x03

void CardInsertNotify(BSCD_ChannelHandle  in_channelHandle, void *inp_data)
{
	uint8_t bStatus = (SLOT0_CHANGED | SLOT0_PRESENT);

    printf("CardInsertNotified\n");

#ifdef PHX_DEEPSLEEP_SIMULATION
// Use contacted smartcard insertion to put LYNX into deepsleep mode
    {
        void lynx_put_lowpower_mode(uint32_t);
        #define CRMU_IHOST_POWER_CONFIG_DEEPSLEEP 0x3
        lynx_put_lowpower_mode( CRMU_IHOST_POWER_CONFIG_DEEPSLEEP );
        return;
    }
#endif
	if ((bActiveStatus == ICCSTATUS_INACTIVE) || (bActiveStatus == ICCSTATUS_ACTIVE))
		return;

	usbdResume();				/* Resume the usb device port if selectively suspended */

	bActiveStatus = ICCSTATUS_INACTIVE;
	NotifySlotChange(0, bStatus);

#if CCID_DEBUG
	printf("CardInsertNotified\n");
#endif

}

void CardRemoveNotify(BSCD_ChannelHandle  in_channelHandle, void *inp_data)
{
	uint8_t bStatus = (SLOT0_CHANGED | SLOT0_NOTPRESENT);

    printf("CardRemoveNotified\n");

#ifdef PHX_DEEPSLEEP_SIMULATION
// Use contacted smartcard removal to wake LYNX from deepsleep mode.
// Disable the normal SC functionality
    {
        return;
    }    
#endif

	if (bActiveStatus == ICCSTATUS_ABSENT)
		return;

	usbdResume();				/* Resume the usb device port if selectively suspended */

	bActiveStatus = ICCSTATUS_ABSENT;
	NotifySlotChange(0, bStatus);

#if CCID_DEBUG
	printf("CardRemoveNotified\n");
#endif

}

/*
 * This function is called when USB Enumeration is done.
 * We need to send out card status ccid message (The earlier send was lost due to usb reset)
 */
void
ccidUsbEnumDone(void)
{
	uint8_t errCode;

	if(!isSCPresent())
	{
		printf("ccidct: skip enum done \n");	
		return ;
	}
	
	errCode = BSCD_Channel_DetectCardNonBlock(gSmartCard.cliChannel.channelHandle, BSCD_CardPresent_eInserted);

	if (errCode == 0) {
		bActiveStatus = ICCSTATUS_INACTIVE;
	}
	else {
		bActiveStatus = ICCSTATUS_ABSENT;
	}

	switch (bActiveStatus)
	{
		case ICCSTATUS_ABSENT:
			printf("ccidct: enum done, send card not present\n");
			NotifySlotChange(0, (SLOT0_CHANGED | SLOT0_NOTPRESENT));
			break;

		case ICCSTATUS_ACTIVE:
		case ICCSTATUS_INACTIVE:
			printf("ccidct: enum done, send card present\n");
			NotifySlotChange(0, (SLOT0_CHANGED | SLOT0_PRESENT));
			break;

		default: /* error */
			printf("ccidct: enum done, err status\n");
			break;
	}
}

/*
 * Called from the main task thread by USBD after a reset has been detected
 * and the HW & endpoint data structures have been reinitialized.
 * Reset any machnine state necessary, and initialize the endpoints as needed.
 */
void
ct_usbd_reset(void)
{
//	uint8_t errCode;


	/* acquire endpoint handles */
	CCIDInterface.BulkInEp = usbd_find_endpoint(USBD_CT_INTERFACE, BULK_TYPE | IN_DIR);
	CCIDInterface.BulkOutEp = usbd_find_endpoint(USBD_CT_INTERFACE, BULK_TYPE | OUT_DIR);
	CCIDInterface.IntrInEp = usbd_find_endpoint(USBD_CT_INTERFACE, INTR_TYPE | IN_DIR);
	if (CCIDInterface.BulkOutEp)
		CCIDInterface.BulkOutEp->appCallback = CCIDCallBackHandler;

#if 0 //ccidUsbEnumDone
	/* ??? should we reset the card here? */

	errCode = BSCD_Channel_DetectCardNonBlock(gSmartCard.cliChannel.channelHandle, BSCD_CardPresent_eInserted);

	if (errCode == 0) {
		bActiveStatus = ICCSTATUS_INACTIVE;
		NotifySlotChange(0, 3); // done after usb enum is complete -- not sure why this has to be done again ???
	}
	else {
		bActiveStatus = ICCSTATUS_ABSENT;
	}
#endif

	/* prepare to receive the first CCID command */
	wRcvLength = 0;

	if (CCIDInterface.BulkOutEp)
		recvEndpointData((CCIDInterface.BulkOutEp), CCIDMsgBufferOut, CCIDInterface.BulkOutEp->maxPktSize);
}

// this function is a fake notification to host the present of ICC card
void InitialCCIDDevice(void)
{
	bActiveStatus = ICCSTATUS_ABSENT;
	bClockStatus = ICC_CLOCK_STOPPED_L;

#if CCID_DEBUG
	printf("ccidct:InitialCCIDDevice\n");
#endif


	BSCD_Init(&gSmartCard, BSCD_VccLevel_e3V);
	BSCD_Channel_SetDetectCardCB(gSmartCard.cliChannel.channelHandle,
				BSCD_CardPresent_eInserted, CardInsertNotify);
	BSCD_Channel_SetDetectCardCB(gSmartCard.cliChannel.channelHandle,
				BSCD_CardPresent_eRemoved, CardRemoveNotify);

}


void ccidCmdCall(void)
{
	uint8_t cmdIndex;

#if CCID_DEBUG
	uint32_t i;
	get_ms_ticks(&i);
	VOLATILE_MEM_PTR->ccidct_msg_ts = i;
   	printf("\nPC2RD(%d): ", i);
    for (i=0; i < (wRcvLength > DEBUG_OUTPUT_LEN ? DEBUG_OUTPUT_LEN : wRcvLength); i++)
      	printf("%2x ", *(CCIDMsgBufferOut + i));
   	printf("\n");
#endif

	memset(&(CCIDMsgBufferIn[0]), 0, 10);

	cmdIndex = (CCIDMsgBufferOut[COMMAND_OFFSET]&0x7F)-0x60; /* All CCID OUT message CMD starts with 0x60 */

	if (cmdIndex < (sizeof(CmdAction)/sizeof(CmdAction[0])))
	{
		(*CmdAction[cmdIndex])();
	}
	else
		handleUnknownCmd();

	wRcvLength = 0;
	if (CCIDInterface.BulkOutEp)
		recvEndpointData((CCIDInterface.BulkOutEp), CCIDMsgBufferOut, CCIDInterface.BulkOutEp->maxPktSize);
}

void ccidCmdRemCall(void)
{
	BSCD_IntrType event = BSCD_IntType_eCardRemoveInt;
	int i;

	for (i = 0; i < BSCD_MAX_NUM_CALLBACK_FUNC; i++) {
		if (gSmartCard.cliChannel.channelHandle->callBack.cardRemoveIsrCBFunc[i] != NULL) {
			gSmartCard.cliChannel.channelHandle->callBack.cardRemoveIsrCBFunc[i](gSmartCard.cliChannel.channelHandle, &event);
		}
	}
}

void ccidCmdInsCall(void)
{
	BSCD_IntrType event = BSCD_IntType_eCardInsertInt;
	int i;

	for (i = 0; i < BSCD_MAX_NUM_CALLBACK_FUNC; i++) {
		if (gSmartCard.cliChannel.channelHandle->callBack.cardInsertIsrCBFunc[i] != NULL) {
			gSmartCard.cliChannel.channelHandle->callBack.cardInsertIsrCBFunc[i](gSmartCard.cliChannel.channelHandle, &event);
		}
	}
}

void CCIDCallBackHandler(EPSTATUS *ep)
{
	uint32_t totalCommandSize = MESSAGE_HEADER + usb_to_cpu32up(CCIDMsgBufferOut+EXTRA_BYTES_OFFSET);
	uint32_t nBytes = ep->dma.transferred;

#if CCID_DEBUG
	printf("Receive totalCommandSize: %d, this time %d bytes\n",  totalCommandSize, nBytes);
#endif

	wRcvLength += nBytes;

	if (wRcvLength < totalCommandSize) {
		/*
		 * The command does not fit into a single packet:
		 * if the supposed command size is larger than the buffer size,
		 * we must skip all the data and reject it
		 */
		uint32_t moreToRead = totalCommandSize - wRcvLength;
		if (totalCommandSize > sizeof(CCIDMsgBufferOut)) {
			printf("ccidct err: get more than bufsize\n");
			recvEndpointData(
					ep,
					CCIDMsgBufferOut+MESSAGE_HEADER, /* into the variable part of the buffer */
					moreToRead > sizeof(CCIDMsgBufferOut) - MESSAGE_HEADER ?
							sizeof(CCIDMsgBufferOut) - MESSAGE_HEADER : /* whatever will fit into the buffer */
							moreToRead
			);
		} else {
			recvEndpointData(
					ep,
					CCIDMsgBufferOut+wRcvLength, /* after the current data */
					totalCommandSize - wRcvLength /* remaining balance of data */
			);
		}
		return;
	}

#if CALLBACK
	ccidCmdCall();
#else
	unsigned int xTaskWoken =_FALSE;
	xTaskWoken = queue_cmd( SCHED_CCID_CMD, QUEUE_FROM_ISR, xTaskWoken);
	SCHEDULE_FROM_ISR(xTaskWoken);
#endif
}

void ClassAbort(void)
{
}

void ClassGetClockfrequency(void)
{
}

void ClassGetDataRate(void)
{
}

#define CCID_DIGN_SUCCESS   0
#define CCID_DIGN_FAILURE   0xFF

// This is a dummy function for CCID diagnostic command,

uint8_t CCIDClassDignostic(uint32_t dwParam, uint32_t *pdwLen, uint32_t *pOutBuffer)
{
	return CCID_DIGN_SUCCESS;
}

/*
	Routine to parse ATR to determine Class information from the card.
	Class information may or may not be present.
	Returns: 0 If class information is present. Appropriate bits of cardClass are set.
			 1 If class information is not present in ATR.
			 2 If invalid ATR start code
*/

uint8_t parseCtATR(uint8_t *atrBuf, uint8_t atrLen, uint8_t *cardClass)
{
	uint8_t errCode = 1;
	uint8_t td[TD_COUNT + 4];
	uint8_t y[TD_COUNT + 4];
#if CCID_DEBUG			
	int k;
#endif
	int count, td_count;
	uint8_t classInformationPresent;
	
	memset(td,0x00,TD_COUNT+4);
	count = 0;
	td_count = 0;
	classInformationPresent = 0;

    memset(&ccid_atr, 0x00, sizeof(ATR_t));

	while (count < atrLen)
	{
		/* Get TS */
		if ((atrBuf[count] != 0x3B) && (atrBuf[count] != 0x3F))		/* TS byte should indicate Direct or Inverse Convention */
		{
			printf("Invalid ATR\n");
			errCode = 2; 			/* Invalid ATR */
			return errCode;
		}
        ccid_atr.TS = atrBuf[count];
		count++;

		/* Get T0*/
		y[td_count] = (atrBuf[count] >> 4) & 0xF;
#if CCID_DEBUG		
		printf("y[%d]: %02X\n", (td_count + 1), y[td_count]);
#endif		
		
#if CCID_DEBUG		
		k = atrBuf[count] & 0xF;
		printf("Historical bytes: %02X\n", k);
#endif		
        ccid_atr.T0 = atrBuf[count];
		count++;

		/* Get TA-TD 1*/
		if (y[td_count] & TA) {
#if CCID_DEBUG		
			printf("TA(%d) present\n", td_count + 1);
#endif			
            ATR_SET_INTERFACE_BYTE(td_count, ATR_INTERFACE_BYTE_TA, atrBuf[count]);
			count++;
		}
		if (y[td_count] & TB) {
            ATR_SET_INTERFACE_BYTE(td_count, ATR_INTERFACE_BYTE_TB, atrBuf[count]);
			count++;
		}
		if (y[td_count] & TC) {
            ATR_SET_INTERFACE_BYTE(td_count, ATR_INTERFACE_BYTE_TC, atrBuf[count]);
			count++;
		}
		if (y[td_count] & TD) {
            ATR_SET_INTERFACE_BYTE( td_count, ATR_INTERFACE_BYTE_TD, atrBuf[count]);
			td[td_count++] = 1;
			count++;
		}

		while ((td[td_count - 1] == 1) && (count < atrLen)) 
		{
			y[td_count] = (atrBuf[count - 1] >> 4) & 0xF;
#if CCID_DEBUG			
			printf("y[%d]: %02X\n", (td_count + 1), y[td_count]);
#endif			
			/* Get TA-TD n*/
			if (y[td_count] & TA) {
#if CCID_DEBUG			
				printf("TA(%d) present\n", td_count + 1);
#endif				
				if (classInformationPresent) {
					*cardClass = atrBuf[count];
					errCode = 0;
#if CCID_DEBUG					
					printf("TA(%d): %02X\n", td_count + 1, atrBuf[count]);
					if (atrBuf[count] & CLASSA)
						printf("Card supports ClassA (5V)\n");
					if (atrBuf[count] & CLASSB)
						printf("Card supports ClassB (3V)\n");
					if (atrBuf[count] & CLASSC)
						printf("Card supports ClassC (1.8V)\n");
#endif						
				}
                ATR_SET_INTERFACE_BYTE( td_count, ATR_INTERFACE_BYTE_TA, atrBuf[count]);
				count++;
			}
			if (y[td_count] & TB) {
                ATR_SET_INTERFACE_BYTE( td_count, ATR_INTERFACE_BYTE_TB, atrBuf[count]);
				count++;
			}
			if (y[td_count] & TC) {
                ATR_SET_INTERFACE_BYTE( td_count, ATR_INTERFACE_BYTE_TC, atrBuf[count]);
				count++;
			}
			if (y[td_count] & TD) {
                ATR_SET_INTERFACE_BYTE( td_count, ATR_INTERFACE_BYTE_TD, atrBuf[count]);
				if (td_count >= 1) {
#if CCID_DEBUG				
					printf("TD(%d): %02X\n", td_count + 1, atrBuf[count]);
#endif					
					if ((atrBuf[count] & 0xF) == 0xF) {		/* If TD(i) protocol is 15, next byte has class information */
						classInformationPresent = 1;
					}
				}
				td[td_count++] = 1;
				count++;
			}
			else {
				td[td_count++] = 0;
			}
		}
		break;
	}
	return errCode;
}

/*
 * Configurate the transmission protocol offered in ATR data
 */

uint8_t ccid_get_pps_protocol(void)
{
    uint8_t protocolNum = PROTOCOL_T0;

    // Check TD1 TD1
    if( ccid_atr.ib[0][ATR_INTERFACE_BYTE_TD].present ) { 
        // Check  TD2
        if( ccid_atr.ib[1][ATR_INTERFACE_BYTE_TD].present == 0 ) {
            // NO TD2, then protocol define by the first bit of TD1 
            if( (ccid_atr.ib[0][ATR_INTERFACE_BYTE_TD].value & 0x1 )==1 ) protocolNum = PROTOCOL_T1;
        } else {
            // Has TD2, then it is T1
            protocolNum = PROTOCOL_T1;
        }
    }

    return protocolNum;
}

/*
 * Configurate the bmFindexDindex field of PPS based on ATR data
 */

uint8_t ccid_get_pps_fd(void)
{
    // TA1 is FD
    if( ccid_atr.ib[0][ATR_INTERFACE_BYTE_TA].present )
        return ccid_atr.ib[0][ATR_INTERFACE_BYTE_TA].value;
    else 
        return CCID_ATR_DEFAULT_TA1;
}  
 
/*
 * Configurate the bGuardTimeT field of PPS based on ATR data
 */

uint8_t ccid_get_pps_guardTime(void)
{
    // TC1
    if (ccid_atr.ib[0][ATR_INTERFACE_BYTE_TC].present)
        return ccid_atr.ib[0][ATR_INTERFACE_BYTE_TC].value;
    else
        return CCID_ATR_DEFAULT_TC1;
}

/* 
 * Configurate WaitingIntegerT0 field of PPS based on ATR data
 */
uint8_t ccid_get_pps_WI_T0(void)
{
    // TC2
    if (ccid_atr.ib[1][ATR_INTERFACE_BYTE_TC].present)
        return ccid_atr.ib[1][ATR_INTERFACE_BYTE_TC].value;
    else
        return CCID_ATR_DEFAULT_TC2;
}

/* 
 * Configurate WaitingIntegerT1 field of PPS based on ATR data
 */
uint8_t ccid_get_pps_WI_T1(void)
{
    // TB3
    if (ccid_atr.ib[2][ATR_INTERFACE_BYTE_TB].present)
        return ccid_atr.ib[2][ATR_INTERFACE_BYTE_TB].value;
    else
        return CCID_ATR_DEFAULT_TB3;
}

/* 
 * Configurate IFSC field of PPS based on ATR data
 */
uint8_t ccid_get_pps_IFSC(void)
{
    uint8_t i, ifsc = CCID_ATR_DEFAULT_TA3;

   /* only T=1 needs to call it */

    for( i = 2; i < ATR_MAX_PROTOCOLS; i++)
    {
        // TA3
        if (ccid_atr.ib[2][ATR_INTERFACE_BYTE_TA].present)
        {
          /* only the first TAi (i>2) must be used */
            ifsc = ccid_atr.ib[2][ATR_INTERFACE_BYTE_TA].value;
            break;
        }
    }

    if (ifsc == 0xff)
    {
        /* 0xFF is not a valid value for IFSC */
        ifsc = 0xfe;
    }
    return ifsc;  
}

/*
 *  Generate PPS based on ATR data
 */
void ccidGeneratePPS( uint8_t *pps, uint32_t *dwLen )
{
    printf("ct: ccidGeneratePPS\n");

    if( ccid_get_pps_protocol() == PROTOCOL_T0 )
    {
        /* T0: Protocol Data Structure */
        pps [RES_PROTOCOL_OFFSET] = PROTOCOL_T0;
        pps [SETP_FD_OFFSET] = ccid_get_pps_fd(); 
        pps [RES_TCCKST_OFFSET] = (ccid_atr.TS==0x3f) ? 0x2 : 0x00;
        pps [SETP_GUARD_OFFSET] = ccid_get_pps_guardTime();
        pps [SETP_T1WAIT_OFFSET] = ccid_get_pps_WI_T0();
        pps [RES_CLKSTOP_OFFSET] = 0x00;
       *dwLen = 5;
    }
    else  // For Protocol_T1
    {
        /* T1: Protocol Data Structure */
        pps [RES_PROTOCOL_OFFSET] = PROTOCOL_T1;
        pps [SETP_FD_OFFSET] = ccid_get_pps_fd(); 
        pps [RES_TCCKST_OFFSET] = 0x10 | ((ccid_atr.TS==0x3f) ? 0x2 : 0x00);
        pps [SETP_GUARD_OFFSET] = ccid_get_pps_guardTime();
        pps [SETP_T1WAIT_OFFSET] = ccid_get_pps_WI_T1();
        pps [RES_CLKSTOP_OFFSET] = 0x00;
        pps [RES_BIFSC_OFFSET] = ccid_get_pps_IFSC();
        pps [RES_NAD_OFFSET] = 0x00;
       *dwLen = 7;
    }
}
 
