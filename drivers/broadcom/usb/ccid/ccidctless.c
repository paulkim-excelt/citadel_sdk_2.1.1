/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/

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

extern void ushxHIDEvent(void);

#define CL_IN_TIMEOUT	(200)

#define CALLBACK 0

#define HIDDEBUG 0
#define DEBUG_OUTPUT_LEN 50

extern BERR_Code
BRFD_Channel_APDU_TransceiveBrcm(
	BRFD_ChannelHandle channelHandle,
	const BRFD_ChannelSettings *inp_channelDefSettings,
	unsigned char *inp_ucXmitData,
	unsigned int in_ulNumXmitBytes,
	unsigned char *outp_ucRcvData,
	unsigned int *outp_ulNumRcvBytes,
	unsigned int in_ulMaxReadBytes,
	unsigned int in_wLevelParameter,
	unsigned char *outp_bChainingParameter,
	void (*in_Callback)(BRFD_ChannelHandle channelHandle)
);


#define	_STATIC_	/* nothing */

_STATIC_ void HIDRF_handleErrorInCmd(HIDCCIDContext *hidCCIDContext, uint8_t errCode);
_STATIC_ void HIDRF_handleUnknownCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleUnknownSlot(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleTooLongCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleSetParametersCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleIccPowerOnCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleIccPowerOffCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleGetSlotStatusCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleT0APDUCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleEscapeCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleGetParameterCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleResetParametersCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleXfrBlockCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleAbortCmd(HIDCCIDContext *hidCCIDContext);
_STATIC_ void HIDRF_handleSetDataRateAndClockFreqCmd(HIDCCIDContext *hidCCIDContext);

_STATIC_ uint8_t HIDRF_errCodeMap(uint32_t SCIErrCode);
_STATIC_ void HIDRF_slotErrorStatus(HIDCCIDContext *, uint8_t, uint8_t, uint8_t);
_STATIC_ void HIDRF_parsePCtoRDRMessage(
		HIDCCIDContext *,
		uint32_t *,
		uint8_t *,
		uint8_t *);
_STATIC_ void HIDRF_setupRDRtoPCMessageHeader(
		HIDCCIDContext *,
		uint8_t,
		uint32_t,
		uint8_t,
		uint8_t,
		uint8_t,
		uint8_t);
_STATIC_ void HIDRF_sendCCIDDataEndpoint(HIDCCIDContext *, uint8_t, uint8_t *, uint16_t);
_STATIC_ uint8_t HIDRF_ICCStatus(HIDCCIDContext *hidCCIDContext, uint8_t bChange);
_STATIC_ uint8_t HIDRF_ICCClockStatus(HIDCCIDContext *, uint8_t);
_STATIC_ uint8_t HIDRF_copyATR(HIDCCIDContext *, uint8_t *);

void HIDRF_CCIDNotifyHardwareError(uint8_t, uint8_t, uint8_t);

_STATIC_ void HIDRF_callCmdHandler(HIDCCIDContext *);

/*
 * Reserve and release of the CL interface by CV
 */
int cl_interface_reserve(BSCD_ChannelHandle handle)
{
	return BRFD_Interface_Reserve(handle);
}

int cl_interface_release(BSCD_ChannelHandle handle)
{
	return BRFD_Interface_Release(handle);
}

/**
 * map internal error code to CCID error code
 */

_STATIC_ uint8_t HIDRF_errCodeMap(uint32_t SCIErrCode)
{
	switch (SCIErrCode) {
	case BRFD_STATUS_TIME_OUT:
		return HIDRF_PIN_TIMEOUT;
	case BRFD_STATUS_PARITY_EDC_ERR:
		return HIDRF_XFER_PARITY_ERROR;
	case BRFD_STATUS_NO_SC_RESPONSE:
		return HIDRF_ICC_MUTE;
	case BRFD_STATUS_FAILED:
		return HIDRF_HW_ERROR;
	}
	return 0;
}

/**
 * fill in error status into bulk in message
 */

_STATIC_ void HIDRF_slotErrorStatus(
		HIDCCIDContext *hidCCIDContext,
		uint8_t bErrCode,
		uint8_t bmICCStatus,
		uint8_t bmCmdStatus)
{
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_STATUS_OFFSET] = bmICCStatus
			| (bmCmdStatus<<6);
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_ERROR_OFFSET] = bErrCode;
}

/******* BulkOut Command Message *************/

/**
 * pre-parse command message to components
 */

_STATIC_ void HIDRF_parsePCtoRDRMessage(
		HIDCCIDContext *hidCCIDContext,
		uint32_t *dwLen,
		uint8_t *bSlot,
		uint8_t *bSeq)
{
	*dwLen = usb_to_cpu32up(hidCCIDContext->CCIDMsgBufferOut+HIDRF_EXTRA_BYTES_OFFSET);
	*bSlot = hidCCIDContext->CCIDMsgBufferOut[HIDRF_SLOT_OFFSET];
	*bSeq = hidCCIDContext->CCIDMsgBufferOut[HIDRF_SEQUENCE_OFFSET];
}

/********* BulkIn Response Message *************/

/**
 * construct bulk in response message header
 */

_STATIC_ void HIDRF_setupRDRtoPCMessageHeader(
		HIDCCIDContext *hidCCIDContext,
		uint8_t bMsgType,
		uint32_t dwLen,
		uint8_t bSlot,
		uint8_t bSeq,
		uint8_t bStatus,
		uint8_t bError)
{
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_COMMAND_OFFSET] = bMsgType;
	cpu_to_usb32up(hidCCIDContext->CCIDMsgBufferIn+HIDRF_EXTRA_BYTES_OFFSET, dwLen);
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_SLOT_OFFSET] = bSlot;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_SEQUENCE_OFFSET] = bSeq;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_STATUS_OFFSET] = bStatus;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_ERROR_OFFSET] = bError;
}

/**
 * send response message on bulk out or interrupt endpoints
 */
_STATIC_ void HIDRF_sendCCIDDataEndpoint(
		HIDCCIDContext *hidCCIDContext,
		uint8_t EpNum,
		uint8_t *pBuffer,
		uint16_t wlen)
{
	switch (EpNum) {
	case HIDRF_CCID_BULKIN:
		if (hidCCIDContext->bSlotChange == CARD_REMOVED) {
			HIDRF_slotErrorStatus(hidCCIDContext, 0xFE, 0x02, 0x01);
		}
#if HIDDEBUG
		{
		unsigned int i;
		get_ms_ticks(&i);
		printf("\nclRD2PC(%d, %d): ", i, cvGetDeltaTime(VOLATILE_MEM_PTR->ccidcl_msg_ts));
		for (i=0; i < (wlen > DEBUG_OUTPUT_LEN ? DEBUG_OUTPUT_LEN: wlen); i++)
			printf("%2x ", *(pBuffer + i));
				printf("\n");
		}
#endif
		if (hidCCIDContext->benumerated ==1 && hidCCIDContext->CCIDInterface.BulkInEp)
		{
			sendEndpointData(hidCCIDContext->CCIDInterface.BulkInEp, pBuffer, wlen);
			if (!usbTransferComplete(hidCCIDContext->CCIDInterface.BulkInEp, CL_IN_TIMEOUT)) {
				printf("cl: bulk-in failed %p\n", hidCCIDContext->CCIDInterface.BulkInEp);
			}
		}
		break;
	case HIDRF_CCID_INTERRUPTIN:
#if HIDDEBUG
		{
		unsigned int i;
		printf ("\nclRD2PC INTR: ");
		for (i=0; i < (wlen > DEBUG_OUTPUT_LEN ? DEBUG_OUTPUT_LEN: wlen); i++)
			printf("%2x ", *(pBuffer + i));
				printf("\n");
		}
#endif
		if (hidCCIDContext->benumerated ==1 && hidCCIDContext->CCIDInterface.IntrInEp)
		{
			sendEndpointData(hidCCIDContext->CCIDInterface.IntrInEp, pBuffer, wlen);
	 		usbTransferComplete(hidCCIDContext->CCIDInterface.IntrInEp, CL_IN_TIMEOUT);
		}
		break;
	}
}

/**
 * send WTX indication as of CCID 1.1 page 73 and 77 examples;
 * this is a callback which is always called in the context of
 * HIDRF_handleXfrBlockCmd call to BRFD_Channel_APDU_TransceiveNew
 */

void HIDRF_sendWTXIndication(BRFD_ChannelHandle channelHandle)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwMsgLen;

	/* get HOST CCID DATA HERE */
	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwMsgLen, &bSlot, &bSeq);
	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_DataBlock,
			0, bSlot, bSeq, 0x80, 10); /* use dummy multiplier of 10 */

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**

 * generate the correct card status indications
 * given present state and most recent command from host.
 *
 * The code must simulate conditions like 'card
 * present, inactive' and 'card clock stopped' in order
 * to give the class driver values it expects.
 */

_STATIC_ uint8_t HIDRF_ICCStatus(HIDCCIDContext *hidCCIDContext, uint8_t bChange)
{
	switch (bChange) {
	case HIDRF_PC2RDR_IccPowerOff:
		hidCCIDContext->bActiveStatus = ICC_INACTIVE;
		break;
	case HIDRF_PC2RDR_IccPowerOn:
		hidCCIDContext->bActiveStatus = ICC_ACTIVE;
		break;
	default:
		break;
	}
	return hidCCIDContext->bActiveStatus;
}

/**
 * generate the correct card status indications
 * given present state and most recent command from host.
 *
 * The code must simulate conditions like 'card
 * present, inactive' and 'card clock stopped' in order
 * to give the class driver values it expects.
 */

_STATIC_ uint8_t HIDRF_ICCClockStatus(HIDCCIDContext *hidCCIDContext, uint8_t bChange)
{
	switch (bChange) {
	case HIDRF_PC2RDR_IccPowerOff:
		hidCCIDContext->bClockStatus = HIDRF_ICC_CLOCK_STOPPED_L;
		break;
	case HIDRF_PC2RDR_IccPowerOn:
		hidCCIDContext->bClockStatus = HIDRF_ICC_CLOCK_RUNNING;
		break;
	default:
		break;
	}
	return hidCCIDContext->bClockStatus;
}

/**
 * Copy the ATR to the pointer dest
 * @return the length of the ATR
 */

_STATIC_ uint8_t HIDRF_copyATR(HIDCCIDContext *hidCCIDContext, uint8_t *dest)
{
	BRFD_ChannelHandle in_channelHandle =
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle;
	size_t len = in_channelHandle->ulRxLen;

	memcpy(dest, in_channelHandle->aucRxBuf, len);
	return len;
}

/*****************Interrupt In Endpoint ****************/

/**
 * report any change on slot status using interrupt in endpoint
 */

void HIDRF_CCIDNotifySlotChange(uint8_t bSlotNumber, uint8_t status)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;

	hidCCIDContext->CCIDNotifyBufferIn[0] = HIDRF_RDR2PC_NotifySlotChange;
	hidCCIDContext->CCIDNotifyBufferIn[1] = status;
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_INTERRUPTIN,
			hidCCIDContext->CCIDNotifyBufferIn, 2);
}

/**
 * notify host of HW problems such as overcurrent using interrupt in endpoint
 */

void HIDRF_CCIDNotifyHardwareError(
		uint8_t bSlotNumber,
		uint8_t bSeq,
		uint8_t bHwErrCode)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;

	hidCCIDContext->CCIDNotifyBufferIn[0] = HIDRF_RDR2PC_HardwareError;
	hidCCIDContext->CCIDNotifyBufferIn[1] = bSlotNumber;
	hidCCIDContext->CCIDNotifyBufferIn[2] = bSeq;
	hidCCIDContext->CCIDNotifyBufferIn[3] = bHwErrCode;
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_INTERRUPTIN,
			hidCCIDContext->CCIDNotifyBufferIn, 4);
}

/**
 * generic command error handler
 */

_STATIC_ void HIDRF_handleErrorInCmd(HIDCCIDContext *hidCCIDContext, uint8_t errCode)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);
	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_DataBlock, 0,
			bSlot, bSeq, hidCCIDContext->bActiveStatus, errCode);
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * unknown command handler
 */

_STATIC_ void HIDRF_handleUnknownCmd(HIDCCIDContext *hidCCIDContext)
{
	HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_CMD_NOT_SUPPORTED);
}

/**
 * unknown or busy slot handler
 */

_STATIC_ void HIDRF_handleUnknownSlot(HIDCCIDContext *hidCCIDContext)
{
	HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_SLOT_DOES_NOT_EXIST);
}

/**
 * incoming command is too long
 */

_STATIC_ void HIDRF_handleTooLongCmd(HIDCCIDContext *hidCCIDContext)
{
	HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_BAD_DWLENGTH_PARAMETER);
}

/**
 * power on command handler
 */

_STATIC_ void HIDRF_handleIccPowerOnCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t errCode = 0;
	uint8_t bCount = 0;


	BRFD_ChannelHandle in_channelHandle = hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);

	if (hidCCIDContext->bActiveStatus == ICC_ABSENT) {
		errCode = SLOTERR_ICC_MUTE;
		printf("CL: Got PowerOn while no card.\n");
	}
	else {
		errCode = BRFD_Channel_ResetCard( in_channelHandle,
										  hidCCIDContext->gRFIDSmartCard.cliChannel.channelSettings.resetCardAction);

		if (errCode != BERR_SUCCESS) {
			errCode = HIDRF_ICC_MUTE; /* timeout */
			HIDRF_slotErrorStatus(hidCCIDContext, errCode, 0, 1);
		} else {
			HIDRF_slotErrorStatus(hidCCIDContext, 0, 0, 0);
			/* copy the ATR into the buffer */
			bCount = HIDRF_copyATR(hidCCIDContext, hidCCIDContext->CCIDMsgBufferIn
									+ HIDRF_MESSAGE_HEADER);
			hidCCIDContext->bActiveStatus = ICC_ACTIVE;
		}
	}

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_DataBlock,
                        bCount, bSlot, bSeq, hidCCIDContext->bActiveStatus, errCode);

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
                        hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER + bCount);

}

/**
 * block transfer command handler
 */

_STATIC_ void HIDRF_handleXfrBlockCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t  bSeq;
	uint8_t  bSlot;
	uint32_t dwMsgLen;
	uint16_t wLevelParameter;
	uint8_t  bChainingParameter = 0;

	uint8_t *pBuf;
	uint32_t dwRetLen = 0;
	uint8_t *pBufIn= hidCCIDContext->CCIDMsgBufferIn + HIDRF_MESSAGE_HEADER;
	uint8_t  errCode = 0;

	BRFD_ChannelHandle in_channelHandle =
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle;

	/* get HOST CCID DATA HERE */
	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwMsgLen, &bSlot, &bSeq);

	pBuf = hidCCIDContext->CCIDMsgBufferOut + HIDRF_MESSAGE_HEADER;

	wLevelParameter = usb_to_cpu16up(hidCCIDContext->CCIDMsgBufferOut + HIDRF_LEVEL_OFFSET);

	switch (wLevelParameter) {
	case 0:
		/* The command APDU begins and ends with this command */
		if (hidCCIDContext->bXfrStatus != HIDRF_XFR_NONE) {
			hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
			HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_WLEVEL_INVALID);
			return;
		}
		break;
	case 1:
		/* The command APDU begins with this command, and continues in the next XfrBlock */
		if (hidCCIDContext->bXfrStatus != HIDRF_XFR_NONE) {
			hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
			HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_WLEVEL_INVALID);
			return;
		}
		break;
	case 2:
		/* This is not the first block and the APDU ends here */
		if (hidCCIDContext->bXfrStatus == HIDRF_XFR_NONE) {
			HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_WLEVEL_INVALID);
			return;
		}
		hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
		break;
	case 3:
		/* this is not the first block and the APDU does not end here */
		if (hidCCIDContext->bXfrStatus == HIDRF_XFR_NONE) {
			HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_WLEVEL_INVALID);
			return;
		}
		hidCCIDContext->bXfrStatus = HIDRF_XFR_PENDING;
		break;
	case 0x10:
		/* empty field, the Xfr continues in the next block */
		break;
	default:
		HIDRF_handleErrorInCmd(hidCCIDContext, HIDRF_WLEVEL_INVALID);
		hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
		return;
	}

#ifdef PHX_FELICA_TEST

	errCode = BRFD_Channel_APDU_TransceiveBrcm(in_channelHandle,
			&in_channelHandle->currentChannelSettings, pBuf, dwMsgLen, pBufIn,
			&dwRetLen, BRFD_MAX_CCID_RX_SIZE,
			wLevelParameter,
			&bChainingParameter,
			HIDRF_sendWTXIndication
	);

#else

	errCode = BRFD_Channel_APDU_TransceiveNew(in_channelHandle,
			&in_channelHandle->currentChannelSettings, pBuf, dwMsgLen, pBufIn,
			&dwRetLen, BRFD_MAX_CCID_RX_SIZE,
			wLevelParameter,
			&bChainingParameter,
			HIDRF_sendWTXIndication
	);
#endif

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_DataBlock,
			dwRetLen, bSlot, bSeq, 0, 0);

	hidCCIDContext->CCIDMsgBufferIn[9] = bChainingParameter;

	if (errCode) {
		errCode = HIDRF_ICC_MUTE; /* timeout */
		HIDRF_slotErrorStatus(hidCCIDContext, errCode, 0, 1);
	}

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER + dwRetLen);
}

/**
 * power off command handler
 */

_STATIC_ void HIDRF_handleIccPowerOffCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t errCode;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);

	if (hidCCIDContext->bActiveStatus == ICC_ABSENT) {
		errCode = SLOTERR_ICC_MUTE;
		printf("CL: Got PowerOff while no card.\n");
	}
	else {
		errCode = BRFD_Channel_PowerICC(
				hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
				BRFD_PowerICC_ePowerDown);

		hidCCIDContext->CCIDMsgBufferIn[HIDRF_ERROR_OFFSET] = errCode;
		hidCCIDContext->CCIDMsgBufferIn[HIDRF_CLOCK_STATUS_OFFSET]
				= HIDRF_ICCClockStatus(hidCCIDContext, HIDRF_PC2RDR_IccPowerOff);

		if (errCode == 0) {
			hidCCIDContext->bActiveStatus = ICC_INACTIVE;
		}
	}

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_SlotStatus, 0,
			bSlot, bSeq, hidCCIDContext->bActiveStatus, errCode);

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * set parameters command handler
 */

_STATIC_ void HIDRF_handleSetParametersCmd(HIDCCIDContext *hidCCIDContext)
{
	/* host wants to set a bunch of parameters about which we don't care.
	 * host wants us to echo the parameters back.  do so here.
	 */
	uint32_t dwLen;
	uint8_t bSlot;
	uint8_t bSeq;

	uint8_t bProtocolNum;

	BRFD_ChannelHandle in_channelHandle =
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);
	bProtocolNum = hidCCIDContext->CCIDMsgBufferOut[HIDRF_PROTOCOL_OFFSET];
	if (bProtocolNum == PROTOCOL_T0) {
		/* T=0 per CCID spec. 6.1.7 */
		in_channelHandle->currentChannelSettings.eProtocolType
				= BRFD_AsyncProtocolType_e0;
	}

	if (bProtocolNum == PROTOCOL_T1) {
		/* T=1 per CCID spec. 6.1.7 */
		in_channelHandle->currentChannelSettings.eProtocolType
				= BRFD_AsyncProtocolType_e1;
		in_channelHandle->currentChannelSettings.bTPDU = 1;
	}

	memcpy(hidCCIDContext->CCIDMsgBufferIn,	hidCCIDContext->CCIDMsgBufferOut,
			hidCCIDContext->wRcvLength);
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_COMMAND_OFFSET] = HIDRF_RDR2PC_Parameters;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_ERROR_OFFSET] = 0;

	hidCCIDContext->CCIDMsgBufferIn[HIDRF_CLOCK_STATUS_OFFSET] = in_channelHandle->currentChannelSettings.eProtocolType == BRFD_AsyncProtocolType_e1 ? 1 : 0;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_STATUS_OFFSET] = 0;


	/* HIDRF_MESSAGE_HEADER */
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
				hidCCIDContext->CCIDMsgBufferIn,
				hidCCIDContext->wRcvLength); /* same as incoming message */
}

/**
 * get parameters command handler
 */

_STATIC_ void HIDRF_handleGetParameterCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t bStatus = 0;
	uint8_t bErrCode = 0;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_Parameters, 0,
			bSlot, bSeq, bStatus, bErrCode);
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * reset parameters command handler
 */

_STATIC_ void HIDRF_handleResetParametersCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t bStatus = 0;
	uint8_t bErrCode = 0;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);
	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_Parameters, 0,
			bSlot, bSeq, bStatus, bErrCode);
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * get slot status command handler
 */

_STATIC_ void HIDRF_handleGetSlotStatusCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t bErrCode = 0;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_SlotStatus, 0,
			bSlot, bSeq, hidCCIDContext->bActiveStatus, bErrCode);

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * escape command handler
 */

_STATIC_ void HIDRF_handleEscapeCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLength;

	uint8_t bStatus = 0;
	uint8_t bErrCode = 0;

	uint32_t dwRetLen = 0;


	BRFD_ChannelHandle in_channelHandle =
                        hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle;

	uint8_t *pBuf = hidCCIDContext->CCIDMsgBufferOut + HIDRF_MESSAGE_HEADER;
	uint8_t *pBufIn= hidCCIDContext->CCIDMsgBufferIn + HIDRF_MESSAGE_HEADER;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLength, &bSlot, &bSeq);

	bErrCode = BRFD_Channel_ScardControl(in_channelHandle,
                        &in_channelHandle->currentChannelSettings, pBuf, dwLength, pBufIn,
                        &dwRetLen, BRFD_MAX_ESCAPE_SIZE);

	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_Escape, dwRetLen,
                        bSlot, bSeq, bStatus, bErrCode);
	if (bErrCode) {
		bErrCode = HIDRF_ICC_MUTE; /* timeout */
		HIDRF_slotErrorStatus(hidCCIDContext, bErrCode, 0, 1);
	}

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER + dwRetLen);
}

/**
 * abort command handler
 */

_STATIC_ void HIDRF_handleAbortCmd(HIDCCIDContext *hidCCIDContext)
{
	uint8_t bSeq;
	uint8_t bSlot;
	uint32_t dwLen;

	uint8_t bStatus = 0;
	uint8_t bErrCode = 0;

	HIDRF_parsePCtoRDRMessage(hidCCIDContext, &dwLen, &bSlot, &bSeq);
	HIDRF_setupRDRtoPCMessageHeader(hidCCIDContext, HIDRF_RDR2PC_SlotStatus, 0,
			bSlot, bSeq, bStatus, bErrCode);
	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, HIDRF_MESSAGE_HEADER);
}

/**
 * set data rate and clock frequency command handler
 */

_STATIC_ void HIDRF_handleSetDataRateAndClockFreqCmd(HIDCCIDContext *hidCCIDContext)
{
	memcpy(hidCCIDContext->CCIDMsgBufferIn, hidCCIDContext->CCIDMsgBufferOut,
			hidCCIDContext->wRcvLength);
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_COMMAND_OFFSET]
			= HIDRF_RDR2PC_DataRateAndClockFrequency;
	hidCCIDContext->CCIDMsgBufferIn[HIDRF_ERROR_OFFSET] = 0;

	HIDRF_sendCCIDDataEndpoint(hidCCIDContext, HIDRF_CCID_BULKIN,
			hidCCIDContext->CCIDMsgBufferIn, hidCCIDContext->wRcvLength);
}



/**
 * callback to handle card insertion event
 */

void HIDRF_CardInsertNotify(BRFD_ChannelHandle in_channelHandle, void *inp_data)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	uint8_t bStatus = 0x03;

	printf("ccidcl: card insert called\n");

	/* check if radio is disabled first */
	if (isRadioEnabled(FALSE) == FALSE)
	{
		printf("card insert msg discarded because radio is not enabled for ccid\n");
		return;
	}

	if (hidCCIDContext->bActiveStatus == ICC_INACTIVE || hidCCIDContext->bActiveStatus == ICC_ACTIVE) {
//		printf("card insert msg discarded because of current slot status %d\n", hidCCIDContext->bActiveStatus);
		return;
	}

        /* Notify card insertion to CV callbacks */
        cvClCardDetectNofity(TRUE);

	usbdResume();				/* Resume the usb device port if selectively suspended */

	hidCCIDContext->bActiveStatus = ICC_INACTIVE;

	HIDRF_CCIDNotifySlotChange(0, bStatus);
}

/**
 * callback to handle card removal event
 */

void HIDRF_CardRemoveNotify(BRFD_ChannelHandle in_channelHandle, void *inp_data)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	uint8_t bStatus = 0x02;

	printf("ccidcl: card remove called\n");

#ifdef PHX_FELICA_TEST
	return; // don't report card change to host
#endif

	if (hidCCIDContext->bActiveStatus == ICC_ABSENT) {
//		printf("card remove msg discarded because of current slot status %d\n", hidCCIDContext->bActiveStatus);
		return;
	}

        /* Notify card removal  to CV callbacks */
        cvClCardDetectNofity(FALSE);

	usbdResume();				/* Resume the usb device port if selectively suspended */
	
	hidCCIDContext->bActiveStatus = ICC_ABSENT;

	HIDRF_CCIDNotifySlotChange(0, bStatus);
}

/**
 * fake notification to host the present of ICC card
 */

void ClInitialCCIDDevice(void)
{
	HIDRF_CCIDInitialize();
#if 0
	int channelHandle;
	BERR_Code  result;

        /* Initialise the heap */
        ushxInitHeap(24000, HID_LARGE_HEAP_PTR);

        result = BRFD_Open( &channelHandle, HID_LARGE_HEAP_PTR );

    if ( BERR_SUCCESS == result ) {
        result = BRFD_Channel_Open( channelHandle, HID_LARGE_HEAP_PTR );
        }

        return result;
#endif
}

void HidPollEvent(void)
{
	unsigned int xTaskWoken =_FALSE;

	/* Does not make sense to put it in a queue and take it again so lets execut it from here */
	if (VOLATILE_MEM_PTR->rfid_hid_poll_enable)
		xTaskWoken = queue_nfc_cmd( USHX_HID_EVENT_CMD, xTaskWoken);
}

/*
 * This function is called when USB Enumeration is done.
 * We need to send out card status ccid message (The earlier send was lost due to usb reset)
 */
void
cl_ccidUsbEnumDone(void)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	tCCIDINTERFACE *pInterface = &(hidCCIDContext->CCIDInterface);
	uint8_t errCode = 0;
	extern void *hid_glb_ptr;
	extern void *hid_glb_diag_ptr;

	printf("cl_ccidUsbEnumDone: Reset the BRFD library \n");

	hid_glb_ptr = 0;
	hid_glb_diag_ptr = FP_CAPTURE_LARGE_HEAP_PTR;

	ushxInitHeap(HID_LARGE_HEAP_SIZE, HID_LARGE_HEAP_PTR);
	ushxInitHeap(HID_SMALL_HEAP_SIZE, HID_SMALL_HEAP_PTR);

	BRFD_Init(&hidCCIDContext->gRFIDSmartCard, BRFD_VccLevel_e5V);
	hidCCIDContext->benumerated = 0;
	if (!isRadioPresent())
		return;

	BRFD_Channel_SetDetectCardCB(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eInserted, HIDRF_CardInsertNotify);

	BRFD_Channel_SetDetectCardCB(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eRemoved, HIDRF_CardRemoveNotify);

	errCode = BRFD_Channel_DetectCardNonBlock(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eInserted);

	if (errCode == 0) {
		/* card present */
		hidCCIDContext->bActiveStatus = ICC_INACTIVE;
		HIDRF_CCIDNotifySlotChange(0, 3);
	} else {
		hidCCIDContext->bActiveStatus = ICC_ABSENT;
		HIDRF_CCIDNotifySlotChange(0, 2);
	}
	hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
	hidCCIDContext->benumerated = 1;
}
/*
 * Called from the main task thread by USBD after a reset has been detected
 * and the HW & endpoint data structures have been reinitialized.
 * Reset any machnine state necessary, and initialize the endpoints as needed.
 */
void cl_usbd_reset(void)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	tCCIDINTERFACE *pInterface = &(hidCCIDContext->CCIDInterface);
	uint8_t errCode = 0;

	printf("cl_usbd_reset: Reset the CL interface callbacks \n");

	hidCCIDContext->bActiveStatus = ICC_ABSENT;
	hidCCIDContext->bClockStatus = HIDRF_ICC_CLOCK_STOPPED_L;
	hidCCIDContext->bThisSlot = 0;


#if 0	// moved to cl_ccidUsbEnumDone
	BRFD_Init(&hidCCIDContext->gRFIDSmartCard, BRFD_VccLevel_e5V);
	if (!isRadioPresent())
		return;
#endif

	/* acquire endpoint handles */
#ifdef USBD_CL_INTERFACE
	pInterface->BulkInEp  = usbd_find_endpoint(USBD_CL_INTERFACE, BULK_TYPE | IN_DIR);
	pInterface->BulkOutEp = usbd_find_endpoint(USBD_CL_INTERFACE, BULK_TYPE | OUT_DIR);
	pInterface->IntrInEp  = usbd_find_endpoint(USBD_CL_INTERFACE, INTR_TYPE | IN_DIR);
	if (pInterface->BulkOutEp)
		pInterface->BulkOutEp->appCallback = HIDRF_CCIDCallBackHandler;
#endif

#if 0	// moved to cl_ccidUsbEnumDone

	BRFD_Channel_SetDetectCardCB(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eInserted, HIDRF_CardInsertNotify);

	BRFD_Channel_SetDetectCardCB(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eRemoved, HIDRF_CardRemoveNotify);

	errCode = BRFD_Channel_DetectCardNonBlock(
			hidCCIDContext->gRFIDSmartCard.cliChannel.channelHandle,
			BRFD_CardPresent_eInserted);

	if (errCode == 0) {
		/* card present */
		hidCCIDContext->bActiveStatus = ICC_INACTIVE;
		HIDRF_CCIDNotifySlotChange(0, 3);
	} else {
		hidCCIDContext->bActiveStatus = ICC_ABSENT;
	}
	hidCCIDContext->bXfrStatus = HIDRF_XFR_NONE;
#endif


    	/* prepare to receive the first CCID command */
   	hidCCIDContext->wRcvLength = 0;
	hidCCIDContext->benumerated = 0;


	if (hidCCIDContext->CCIDInterface.BulkOutEp)
	   	recvEndpointData(hidCCIDContext->CCIDInterface.BulkOutEp,
			hidCCIDContext->CCIDMsgBufferOut,
			hidCCIDContext->CCIDInterface.BulkOutEp->maxPktSize);
}

void HIDRF_CCIDInitialize(void)
{

	/* setup USB endpoints and prepare to receive first command */
	cl_usbd_reset();

	cl_ccidUsbEnumDone();

	enable_low_res_timer_int(300, HidPollEvent); // changed it from 100 to 300 
}

/**
 * bulk out command dispatcher (once whole command read into memory)
 */

void ClccidCmdCall(void)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
#if HIDDEBUG
	uint32_t i;
	get_ms_ticks(&i);
	VOLATILE_MEM_PTR->ccidcl_msg_ts = i;
   	printf("\nclPC2RD(%d): ", i);
    for (i=0; i < (hidCCIDContext->wRcvLength > DEBUG_OUTPUT_LEN ? DEBUG_OUTPUT_LEN : hidCCIDContext->wRcvLength); i++)
      	printf("%2x ", *(hidCCIDContext->CCIDMsgBufferOut + i));
   	printf("\n");
#endif
	HIDRF_callCmdHandler(hidCCIDContext);
}



_STATIC_ void HIDRF_callCmdHandler(HIDCCIDContext *hidCCIDContext)
{
	memset( &(hidCCIDContext->CCIDMsgBufferIn[0]), 0, 10);
	if (hidCCIDContext->wRcvLength > sizeof(hidCCIDContext->CCIDMsgBufferOut)) {
		/* we have a buffer overflow */
		HIDRF_handleTooLongCmd(hidCCIDContext);
		/* do NOT return here, we need the re-init code below */
	} else if (hidCCIDContext->CCIDMsgBufferOut[HIDRF_SLOT_OFFSET]
				!= hidCCIDContext->bThisSlot) {
		/* we have wrong slot */
		HIDRF_handleUnknownSlot(hidCCIDContext);
		/* do NOT return here, we need the re-init code below */
	} else switch (hidCCIDContext->CCIDMsgBufferOut[HIDRF_COMMAND_OFFSET]) {
	case HIDRF_PC2RDR_SetParameters:
		HIDRF_handleSetParametersCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_IccPowerOn:
		HIDRF_handleIccPowerOnCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_IccPowerOff:
		HIDRF_handleIccPowerOffCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_GetSlotStatus:
		HIDRF_handleGetSlotStatusCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_Escape:
		HIDRF_handleEscapeCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_GetParameters:
		HIDRF_handleGetParameterCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_ResetParameters:
		HIDRF_handleResetParametersCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_XfrBlock:
		HIDRF_handleXfrBlockCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_Abort:
		HIDRF_handleAbortCmd(hidCCIDContext);
		break;
	case HIDRF_PC2RDR_SetDataRateAndClockFrequency:
		HIDRF_handleSetDataRateAndClockFreqCmd(hidCCIDContext);
		break;
	default:
		HIDRF_handleUnknownCmd(hidCCIDContext);
		break;
	}

	hidCCIDContext->wRcvLength = 0;
	if (hidCCIDContext->CCIDInterface.BulkOutEp)
		recvEndpointData(hidCCIDContext->CCIDInterface.BulkOutEp,
			hidCCIDContext->CCIDMsgBufferOut,
			hidCCIDContext->CCIDInterface.BulkOutEp->maxPktSize); /* for the next transfer */
}


/**
 * handler of class control get clock frequency message
 *
 * Note: obsoleted, see the handling in ccidcl_class_request()
 */

void HIDRF_ClassGetClockfrequency(void)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	hidCCIDContext->ClockFrequency[0]= 0x80;
	hidCCIDContext->ClockFrequency[1]= 0x35;
	hidCCIDContext->ClockFrequency[2]= 0x00;
	hidCCIDContext->ClockFrequency[3]= 0x00;
	sendCtrlEndpointData( hidCCIDContext->ClockFrequency, 4);

}

/**
 * bulk out packet handler, assembles command packet
 */




void HIDRF_CCIDCallBackHandler(EPSTATUS *ep)
{
	HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;
	uint32_t totalCommandSize = HIDRF_MESSAGE_HEADER + usb_to_cpu32up(hidCCIDContext->CCIDMsgBufferOut+HIDRF_EXTRA_BYTES_OFFSET);
	uint32_t nBytes = ep->dma.transferred;

	hidCCIDContext->wRcvLength += nBytes;

	if (hidCCIDContext->wRcvLength < totalCommandSize) {
		/*
		 * The command does not fit into a single packet:
		 * if the supposed command size is larger than the buffer size,
		 * we must skip all the data and reject it
		 */
		uint32_t moreToRead = totalCommandSize - hidCCIDContext->wRcvLength;
		if (totalCommandSize > sizeof(hidCCIDContext->CCIDMsgBufferOut)) {
			recvEndpointData(
					ep,
					hidCCIDContext->CCIDMsgBufferOut+HIDRF_MESSAGE_HEADER, /* into the variable part of the buffer */
					moreToRead > sizeof(hidCCIDContext->CCIDMsgBufferOut) - HIDRF_MESSAGE_HEADER ?
							sizeof(hidCCIDContext->CCIDMsgBufferOut) - HIDRF_MESSAGE_HEADER : /* whatever will fit into the buffer */
							moreToRead
			);
		} else {
			recvEndpointData(
					ep,
					hidCCIDContext->CCIDMsgBufferOut+hidCCIDContext->wRcvLength, /* after the current data */
					totalCommandSize - hidCCIDContext->wRcvLength /* remaining balance of data */
			);
		}
		return;
	}

#if CALLBACK
	HIDRF_callCmdHandler(hidCCIDContext);
#else
	unsigned int xTaskWoken =_FALSE;
	xTaskWoken = queue_cmd( SCHED_CCID_CL_CMD, QUEUE_FROM_ISR, xTaskWoken);
	SCHEDULE_FROM_ISR(xTaskWoken);
#endif
}


void ClCCIDCallBackHandler( EPSTATUS *ep)
{
	HIDRF_CCIDCallBackHandler(ep);
}

/**
 * handler of class control abort message
 *
 * This Control pipe request works with the Bulk-OUT PC_to_RDR_Abort command to tell
 * the CCID to stop any current transfer at the specified slot and return to a state
 * where the slot is ready to accept a new command pipe Bulk-OUT message.
 * The wValue field contains the slot number (bSlot) in the low byte and the
 * sequence number (bSeq) in the high byte. The bSlot value tells the CCID which
 * slot should be aborted.
 * The bSeq number is not related to the slot number or the Bulk-OUT message
 * being aborted, but it does match bSeq in the PC_to_RDR_Abort command.
 * When the Host wants to abort a Bulk-OUT command message, it will send a pair
 * of abort messages to the CCID.
 * Both will have the same bSlot and bSeq values.
 * The Host will first send this request to the CCID over the Control pipe.
 * The Host will next send the PC_to_RDR_Abort command message over the Bulk-OUT pipe.
 * Both are necessary due to the asynchronous nature of control pipes and Bulk-Out
 * pipes relative to each other.
 * Upon receiving the Control pipe ABORT request the CCID should check the state
 * of the requested slot.
 * If the last Bulk-OUT message received by the CCID was a PC_to_RDR_Abort
 * command with the same bSlot and bSeq as the ABORT request, then the CCID will
 * respond to the Bulk-OUT message with the RDR_to_PC_SlotStatus response.
 * If the previous Bulk-OUT message received by the CCID was not a PC_to_RDR_Abort
 * command with the same bSlot and bSeq as the ABORT request, then the CCID
 * will fail all Bulk-Out commands to that slot until the PC_to_RDR_Abort
 * command with the same bSlot and bSeq is received. Bulk-OUT commands will
 * be failed by sending a response with bmCommandStatus=Failed
 * and bError=CMD_ABORTED.
 */

void HIDRF_ClassAbort(void)
{
}



/**
 * handler of class control get data rate message
 */

void HIDRF_ClassGetDataRate(void)
{
}

/**
 * dummy function for CCID diagnostic command
 */

uint8_t HIDRF_CCIDClassDignostic(
		uint32_t dwParam,
		uint32_t *pdwLen,
		uint32_t *pOutBuffer)
{
	return CCID_DIGN_SUCCESS;
}



/*
 * Handle USB class requests for the CCID device
 */
void ccidcl_class_request(SetupPacket_t *req, EPSTATUS *ctrl_in, EPSTATUS *ctrl_out)
{
#ifndef PHX_REQ_MCU_USB

        HIDCCIDContext *hidCCIDContext = (HIDCCIDContext *)pccid_mem_block_ctless;

        /* CCID class requests, goes to the control endp */
        switch (req->bRequestType & SETUPKT_DIRECTION) {

        case SETUPKT_HOST_TO_DEVICE:
                break;

        case SETUPKT_DEVICE_TO_HOST:
                switch(req->bRequest) {
                case HIDRF_CCID_CLOCK_FREQUENCIES:
                        cpu_to_usb32up(hidCCIDContext->ClockFrequency, 3580);
                        sendCtrlEpData(ctrl_in, hidCCIDContext->ClockFrequency, 4);
                        return;
                case HIDRF_CCID_DATA_RATES:
                        cpu_to_usb32up(hidCCIDContext->ClockFrequency, 9600);
                        sendCtrlEpData(ctrl_in, hidCCIDContext->ClockFrequency, 4);
                        return;
                default:
                        printf("cl: class request(UNKNOWN 0x%02x H->D)\n", req->bRequest);
                        break;
                }
                break;
        }
        /* Only safe to call *if* sendCtrlEpData() not called above */
        sendCtrlEpData(ctrl_in, (void *)hidCCIDContext->bActiveStatus/*anything here*/, 0);
#endif
}


