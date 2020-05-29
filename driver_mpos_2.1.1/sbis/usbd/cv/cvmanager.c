/*
 * $Copyright Broadcom Corporation$
 *
 */
/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/
/*
 * Broadcom Corporation Credential Vault
 */
/*
 * cvmanager.c:  Credential Vault manager
 */
/*
 * Revision History:
 *
 * 01/05/07 DPA Created.
*/
#include <string.h>
#include "cvmain.h"
#include "usbbootinterface.h"
#include "usbd_if.h"

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

#define epInterfaceSendBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceSendBuffer())
#define epInterfaceBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceBuffer())

extern cv_status usbCvCommandComplete(uint8_t *pBuffer);
extern bool usbCvSend(cv_usb_interrupt *interrupt, uint8_t *pBuffer, uint32_t len, int ticks);


cv_status
cvDecapsulateCmd(cv_encap_command *cmd, uint32_t **inputParams, uint32_t *numInputParams)
{
	uint32_t *endPtr;
	cv_param_list_entry *paramEntryPtr;
	cv_status retVal = CV_SUCCESS;
	uint32_t cmdSize = cmd->transportLen;
	uint8_t *nextParamStart;

	/* validate parameter blob length */
	if (cmd->parameterBlobLen > (cmd->transportLen - sizeof(cv_encap_command) + sizeof(uint32_t)))
	{
		retVal = CV_PARAM_BLOB_INVALID_LENGTH;
		goto err_exit;
	}

	*numInputParams = 0;

	/* now decapsulate parameters.  parse parameter blob and save pointers */
	paramEntryPtr = (cv_param_list_entry *)&cmd->parameterBlob;
	endPtr = (uint32_t *)paramEntryPtr + (ALIGN_DWORD(cmd->parameterBlobLen)/sizeof(uint32_t));
	do
	{
		if (*numInputParams >= MAX_INPUT_PARAMS)
		{
			retVal = CV_PARAM_BLOB_INVALID;
			goto err_exit;
		}

		switch (paramEntryPtr->paramType)
		{
		case CV_ENCAP_STRUC:
			/* parameter pointer should point to parameter and not length */
			inputParams[*numInputParams] = &paramEntryPtr->param;
			break;

		case CV_ENCAP_LENVAL_STRUC:
			/* parameter pointer should point to length */
			inputParams[*numInputParams] = &paramEntryPtr->paramLen;
			break;

		case CV_ENCAP_LENVAL_PAIR:
			/* first parameter points to length and second points to parameter value */
			inputParams[*numInputParams] = &paramEntryPtr->paramLen;
			(*numInputParams)++;
			if (*numInputParams >= MAX_INPUT_PARAMS)
			{
				retVal = CV_PARAM_BLOB_INVALID;
				goto err_exit;
			}
			inputParams[*numInputParams] = &paramEntryPtr->param;
			break;

		case CV_ENCAP_INOUT_LENVAL_PAIR:
			/* only length provided for inbound */
			inputParams[*numInputParams] = &paramEntryPtr->paramLen;
			/* now need to shift buffer down to make room for output.  first make sure length provided */
			/* doesn't cause command to exceed max size */
			cmdSize += paramEntryPtr->paramLen;
			if (cmdSize > CV_IO_COMMAND_BUF_SIZE)
			{
				/* the exception to this is cv_fingerprint_capture, where captureOnly is TRUE */
				/* this is because an FP image must be returned and it could be up to 90K, so */
				/* special handling is required.  Just allocate 0 bytes here for now */
				if (cmd->commandID == CV_CMD_FINGERPRINT_CAPTURE && ((cv_bool)*inputParams[1]) == TRUE)
					cmdSize -= paramEntryPtr->paramLen;
				else {
					retVal = CV_PARAM_BLOB_INVALID_LENGTH;
					goto err_exit;
				}
			}
			nextParamStart = (uint8_t *)paramEntryPtr + 2*sizeof(uint32_t);
			memmove((void *)(nextParamStart + ALIGN_DWORD(paramEntryPtr->paramLen)), ( void *)nextParamStart, (uint8_t *)endPtr - nextParamStart);
			endPtr = (uint32_t *)((uint8_t *)endPtr + ALIGN_DWORD(paramEntryPtr->paramLen));
			break;

		case 0xffff:
		default:
			retVal = CV_PARAM_BLOB_INVALID;
			goto err_exit;
		}

		/* get next parameter */
		(*numInputParams)++;
		paramEntryPtr = (cv_param_list_entry *)((uint8_t *)(paramEntryPtr + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(paramEntryPtr->paramLen));
	} while ((uint32_t *)paramEntryPtr < endPtr);
	{
		uint32_t i = 0;
		for(i=0; i< *numInputParams; i++) {
			DEBUG_LOG("*inputParams[%d] = 0x%x\n",i,*inputParams[i]);
		}
	}
	return CV_SUCCESS;
err_exit:
	return retVal;
}

cv_status
cvEncapsulateCmd(cv_encap_command *cmd, uint32_t numOutputParams, uint32_t **outputParams, uint32_t *outputLengths,
				 uint32_t *outputEncapTypes, cv_status retVal)
{
	uint32_t i;
	cv_param_list_entry *paramEntryPtr, *paramEntryPtrPrev;
	uint8_t *paramBlob, *paramBlobPtr;
	uint32_t paramLen, paramLenPrev;

	/* at this point, output parameters are stored in the buffer based on the length provided in the input */
	/* parameter.  since the actual output may be shorter, the parameter list is compressed here */
	/* step through the output params compress, if necessary */
	paramBlob = paramBlobPtr = (uint8_t *)&cmd->parameterBlob;
	paramEntryPtrPrev = NULL;
	paramLenPrev = 0;
	for (i=0;i<numOutputParams;i++)
	{
		/* get pointer to this parameter list entry in output param block (backup from parameter pointer) */
		paramEntryPtr = (cv_param_list_entry *)((uint8_t *)outputParams[i] - sizeof(cv_param_list_entry) + sizeof(uint32_t));
		paramEntryPtr->paramType = outputEncapTypes[i];
		paramEntryPtr->paramLen = outputLengths[i];
		paramLen = sizeof(cv_param_list_entry) - sizeof(uint32_t)+ ALIGN_DWORD(paramEntryPtr->paramLen);
		/* handle case where the return code CV_INVALID_OUTPUT_PARAMETER_LENGTH indicates that one or more of the parameters */
		/* had insufficient length provided as input for value generated by CV.  in this case, expected length is returned, */
		/* but there is no value field */
		if (retVal == CV_INVALID_OUTPUT_PARAMETER_LENGTH && outputEncapTypes[i] == CV_ENCAP_INOUT_LENVAL_PAIR)
		{
			paramLen = sizeof(cv_param_list_entry) - sizeof(uint32_t);
		}
		/* zero the portion between end of this list and start of next, if not aligned */
		memset((void *)((uint8_t *)&paramEntryPtr->param + paramEntryPtr->paramLen), 0, ALIGN_DWORD(paramEntryPtr->paramLen - paramEntryPtr->paramLen));

		/* check to see this parameter list needs to be moved to be adjacent to previous one */
		if (paramLenPrev)
			if (((uint8_t *)paramEntryPtrPrev + paramLenPrev) < (uint8_t *)paramEntryPtr)
			{
				memmove((void *) ((uint8_t *)paramEntryPtrPrev + paramLenPrev), ( void *)paramEntryPtr, paramLen);
				paramEntryPtr = (cv_param_list_entry *)((uint8_t *)paramEntryPtrPrev + paramLenPrev);
			}

		/* update pointers */
		paramEntryPtrPrev = paramEntryPtr;
		paramLenPrev = paramLen;
		paramBlobPtr += paramLen;
	}
	cmd->parameterBlobLen = paramBlobPtr - paramBlob;
	/* fill in other command fields */
	cmd->flags.returnType = CV_RET_TYPE_RESULT;
	cmd->returnStatus = retVal;
	cmd->transportLen = sizeof(cv_encap_command) - sizeof(uint32_t) + cmd->parameterBlobLen;

	return CV_SUCCESS;
}



void
cvManager(uint32_t epbuflen)
{
	cv_status retValCmd = CV_SUCCESS;
	uint32_t *inputParams[MAX_INPUT_PARAMS];
	uint32_t *outputParams[MAX_OUTPUT_PARAMS] = {0};
	uint32_t outputLengths[MAX_OUTPUT_PARAMS] = {0};
	uint32_t outputEncapTypes[MAX_OUTPUT_PARAMS] = {0};
	uint32_t	numInputParams;
	cv_param_list_entry *outputParamEntry;
	uint32_t numOutputParams = 0;
	uint8_t encapsulateResult = TRUE;
	//cv_internal_callback_ctx ctx;
	cv_usb_interrupt *usbInt;
	cv_encap_command *cmd;
	cv_command_id commandID;
	//cv_bool captureOnlySpecialCase = FALSE;
	//uint8_t *captureOnlyCmdPtr = NULL;
	cv_encap_flags sav_encap_flags;
	cv_version sav_version;
	cv_transport_type sav_transportType;
	//RPB_STRUCT* rpbptr;



	/* first, copy from iobuf to command buf */
	//cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	cmd = (cv_encap_command *)epInterfaceBuffer;

	/* Save encap command values for later use, since buffer is reused during host storage communication */
	sav_encap_flags = cmd->flags;
	sav_version = cmd->version;
	sav_transportType = cmd->transportType;
	commandID = cmd->commandID;

	/* next, decapsulate command */
	if ((retValCmd = cvDecapsulateCmd(cmd, &inputParams[0], &numInputParams)) != CV_SUCCESS) {
		goto cmd_err;
	}



	/* command buffer used for output as well.  API handlers must ensure that input parameters not overwritten */
	outputParamEntry = (cv_param_list_entry *)&cmd->parameterBlob;
	/* set up pointer to command in context */
	//ctx.cmd = cmd;
	switch (cmd->commandID)
	{
		case CV_CMD_LOAD_SBI:
			if (inputParams[0] == 0)
			{
				/* abort and send error */
				numOutputParams = 0;
				goto cmd_err;
			}

            usb_device_ctx->usb_sbi_ptr = (uint8_t *)inputParams[1];
			usb_device_ctx->cur_read_offset = (uint32_t)inputParams[1];
			usb_device_ctx->total_rem_len = epbuflen - ((uint32_t)inputParams[1] - (uint32_t)epInterfaceBuffer);
			usb_device_ctx->sbi_read_offset = 0;
			break;

		case CV_CMD_GET_PRB:
			//rpbptr = (RPB_STRUCT*)IPROC_BTROM_GET_RPB();
			outputParams[0] = &outputParamEntry->param;
			outputEncapTypes[0] = CV_ENCAP_LENVAL_STRUC;
			numOutputParams = 1;
			outputParams[0] = &outputParamEntry->param;
			outputLengths[0]= MAX_OUTPUT_PARAM_SIZE;
/*			sprintf((char*)outputParams[0],
					"%08x%08x%08x%04x%04x%08x%04x%04x"
					"%08x%08x%08x%08x%08x%08x%08x%08x",
					rpbptr->magic,
					rpbptr->length,
					rpbptr->sbiAddress,
					rpbptr->devSecurityConfig,
					rpbptr->SBIRevisionID,
					rpbptr->CustomerID,
					rpbptr->ProductID,
					rpbptr->CustomerRevisionID,
					rpbptr->ErrorStatus,
					rpbptr->SBLState,
					rpbptr->errCnt,
					rpbptr->failedAuthAttempts,
					rpbptr->failedRecoveryAttempts,
					rpbptr->ExceptionState,
					rpbptr->TOCArrayPtr,
					rpbptr->TOCEntryInfo); */
			outputLengths[0] = strlen((const char*)outputParams[0]);
			outputLengths[0] = ALIGN_DWORD(outputLengths[0]);
			break;

		case 0xffff:
		default:
			/* Invalid command */
			retValCmd = CV_INVALID_COMMAND;
			numOutputParams = 0;
			break;
	}

cmd_err:
	/* Restore encap command values not set by encapsulate */
	cmd->flags = sav_encap_flags;
	cmd->version = sav_version;
	cmd->transportType = sav_transportType;

	/* now, encapsulate result */
	cmd->commandID = commandID;
	if (encapsulateResult)
		cvEncapsulateCmd(cmd, numOutputParams, &outputParams[0], &outputLengths[0],
			&outputEncapTypes[0], retValCmd);

	memcpy((void*)epInterfaceSendBuffer, (void*)cmd, cmd->transportLen);
	/* now point to io buf */
	/* now copy to io buf and send result */
	cmd = (cv_encap_command *)epInterfaceSendBuffer;
	//cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	/* set up interrupt after command */
	usbInt = (cv_usb_interrupt *)((uint8_t *)cmd + cmd->transportLen);
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	usbInt->resultLen = cmd->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	//memset(CV_SECURE_IO_COMMAND_BUF + cmd->transportLen + sizeof(cv_usb_interrupt), 0, CV_IO_COMMAND_BUF_SIZE - (cmd->transportLen + sizeof(cv_usb_interrupt)));
	memset((void *)(epInterfaceSendBuffer + cmd->transportLen + sizeof(cv_usb_interrupt)), 0, 256 - (cmd->transportLen + sizeof(cv_usb_interrupt)));
}

void cvCmdReply()
{
	cv_encap_command *cmd;
	cv_usb_interrupt *usbInt;
	char intInBuf [8]= 	{0x0,0x0,0x0,0x0,0x2c,0x0,0x0,0x0};
	char bulkInBuf [44]= {0x01,0x00 ,0x00 ,0x00  ,0x2c ,0x00 ,0x00 ,0x00,
					0x22 ,0x00 ,0x40 ,0x81  ,0x01 ,0x00 ,0x00 ,0x00,
					0x00 ,0x00 ,0x00 ,0x00  ,0x00 ,0x00 ,0x00 ,0x00,
					0x00 ,0x00 ,0x00 ,0x00  ,0x00 ,0x00 ,0x00 ,0x00,
					0x00 ,0x00 ,0x00 ,0x00  ,0x00 ,0x00 ,0x00 ,0x00,
					0x00 ,0x00 ,0x00 ,0x00};
	uint32_t i=0;
	DEBUG_LOG("[%s():%d],begin\n",__func__,__LINE__);
	
	memset(epInterfaceSendBuffer, 0, IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size);
	for(i=0;i<sizeof(bulkInBuf);i++){
		epInterfaceSendBuffer[i]=bulkInBuf [i];
	}
	for(i=0;i<sizeof(intInBuf);i++){
		epInterfaceSendBuffer[i+44]=intInBuf [i];
	}
	
	{
		for(i=0;i<52;i++){
			DEBUG_LOG("0x%x ",((char *)epInterfaceSendBuffer)[i]);
		}
		DEBUG_LOG("\n");
	}

	cmd = (cv_encap_command *)epInterfaceSendBuffer;
	usbInt = (cv_usb_interrupt *)((uint8_t *)cmd + cmd->transportLen);
	if (cmd->flags.USBTransport)
	{
		/* send IntrIn and BulkIn, then wait for completion */
		//usbCvSend(usbInt, (uint8_t *)cmd, cmd->transportLen, 5 * HOST_CONTROL_REQUEST_TIMEOUT);
		usbCvCommandComplete((uint8_t *)cmd);
	}
}





