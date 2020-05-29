/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/
/*
 * Include list
 */
#include "CVUserInterface.h"
#include "CVUIThreadHandlers.h"
#include "CVCmdStateList.h"
#include "CVPrompts.h"
#include "CVLogger.h"
#include "linuxifc.h"


/*
 * Variable declarations global to this file
 */
uint32_t				uCommandIndex = 0;		/* unique command index */
uint32_t				uPreviousPromptCode = PROMPT_NONE;	/* previous prompt code */
uint32_t				userPromptStatus = USER_PROMPT_NODISPLAY;	/* prompt status */
prompt_params			promptInfo;				/* prompt details */
cv_fp_callback_ctx		*callbackInfo = NULL;	/* callback information */	
int						dataInitialized = FALSE;


/*
 * Function:
 *      cvhTransmit
 * Purpose:
 *      Transmits  the  encapsulated  command to the USB device and  reads the 
 *		response from it. Synchronous,  asynchronous or  intermediate callback
 *      command processing is handled at CV User Mode Interface
 * Parameters:
 *		ppCommand		--> pointer to the encapslated command buffer
 *		pReturnStatus	<-- command execution status
 *		ppCommandResult	<-> pointer to the encapslated result buffer
 */
CVUSERLIBIFC_API void 
cvhTransmit(
			IN	cv_encap_command **ppCommand,
			OUT	cv_status *pReturnStatus,
			OUT	cv_encap_command **ppCommandResult
			)
{
	BEGIN_TRY
	{
		/* local variables declaration & init section */
		cv_encap_command	*pCommand = *ppCommand;
		cv_encap_command	**pCommandResult = ppCommandResult;
		thread_param		lParam;
		DWORD				bufferLen;

		if (!dataInitialized) {
			initializeData(&callbackInfo);	/* initialize global data */
			dataInitialized = TRUE;
		}

		(*pCommandResult)->returnStatus = CV_SUCCESS;
		*pReturnStatus = CV_SUCCESS;

		/* check for valid input parameters */
		if ((pCommand == NULL) /*|| (pCommandResult == NULL)*/)
		{
			*pReturnStatus = CV_UI_TRANSMIT_INVALID_INPUT;
			logError(CV_UI_TRANSMIT_INVALID_INPUT, ERROR_LOCATION);
			return;
		}

		if ( pCommand->commandID != CV_CMD_LOAD_SBI )
		{
			// check length if not SBI
			if(pCommand->transportLen > MAX_CV_COMMAND_LENGTH)
			{
				/* log the error, update command processing status & return */
				*pReturnStatus = CV_INVALID_COMMAND_LENGTH;
				logError(CV_INVALID_COMMAND_LENGTH, ERROR_LOCATION);
				return;
			}
		}

		/* resubmitted command can advance without taking the semaphore */
		if (isResubmitted(pCommand) == FALSE)
		{
			/* allow one command to process at a time - use a semphore object */
			/* for 5880, since it doesn't have async FP capture, if there's a FP capture active, cancel is */
			logMessage("cvhTransmit: pCommand->commandID 0x%x\n",pCommand->commandID);
			if (pCommand->commandID != CV_CMD_FINGERPRINT_CAPTURE && pCommand->commandID != CV_CMD_FINGERPRINT_CAPTURE_WBF)
			{
			}
			if (createSemaphore() == FALSE)
			{
				*pReturnStatus = CV_UI_FAILED_SEMAPHORE_CREATION; 
				logError(CV_UI_FAILED_SEMAPHORE_CREATION, ERROR_LOCATION);
				logMessage("cvhTransmit: failed to create semaphore\n");
				return;
			}

			/* new command processing started - remember the state */
			SetCmdState(pCommand->commandID, getCommandIndex(TRUE), CV_COMMAND_STARTED);

			/* append a unique command index */
			appendCommandIndex(&pCommand);
		}
		else
		{
			/* get the command status from state manager */
			uint32_t uCurrentState = CV_COMMAND_INVALID_STATE;
			uint32_t uCmdIndex = -1;

			retrieveCommandIndex(pCommand, &uCmdIndex);
			uCurrentState = GetCmdState(pCommand->commandID, uCmdIndex);

			/* check whether CV waits for resubmitted command */
			switch (uCurrentState)
			{
			case CV_COMMAND_WAITING:
				{

					/*Here the command is resubmitted with in the time, so process
					this command and Delete it's state node*/
					DeleteCmdState(pCommand->commandID, getCommandIndex(FALSE));
				}
				break;
			case CV_COMMAND_TIMEOUT:
			default:
				{
					/* return timeout error */
					*pReturnStatus = CV_COMMAND_TIMEOUT;
					(*pCommandResult)->returnStatus = CV_COMMAND_TIMEOUT;

					/*Here the command is timed out.
					So Delete it's state node and return the control*/
					DeleteCmdState(pCommand->commandID, getCommandIndex(FALSE));
				}
				releaseSemaphore();
				return;
			}
		}

		/* save the callback info locally */
		saveCallbackInfo(pCommand, &callbackInfo);

		// load receiver buffer length
		if (pCommand->commandID == CV_CMD_FINGERPRINT_CAPTURE)
			bufferLen = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommand->commandID == CV_CMD_RECEIVE_BLOCKDATA)
			bufferLen = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommand->commandID == CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT)
			bufferLen = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommand->commandID == CV_CMD_FINGERPRINT_CAPTURE_WBF)
			bufferLen = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommand->commandID == CV_CMD_FLASH_PROGRAMMING)
			 /* For flash programming CV command, response is not
			    expected. Expect response only for data part
			  */
			bufferLen = 0; 
		else
			bufferLen = MAX_ENCAP_RESULT_BUFFER;

		logMessage("cvhTransmit() cmd: 0x%x; bufferLen: 0x%x\n", pCommand->commandID, bufferLen);
		if (pCommand->commandID == CV_CMD_FLASH_PROGRAMMING) {
			uint32_t cv_length;
			uint8_t *data_ptr = (uint8_t *)pCommand;

			/* Trasmit the CV header and chip spec header */
			cv_length = sizeof(cv_encap_command) + sizeof(chip_spec_hdr_t) +
					sizeof(uint32_t);
			/* submit the command to CV USB device */
			if (processCommand(CV_SUBMIT_CMD, 
				pCommand, 512, 
				(LPVOID*)pCommandResult, bufferLen) == FALSE)
			{
				/* log the error, update command processing status & return */
				(*pCommandResult)->returnStatus = GetLastError();
				*pReturnStatus = (*pCommandResult)->returnStatus;

				SetCmdState(pCommand->commandID, getCommandIndex(FALSE), CV_COMMAND_COMPLETED);
				releaseSemaphore();
				return;
			}

			/* Transmit the data */
			bufferLen = MAX_ENCAP_RESULT_BUFFER;
			if (processCommand(CV_SUBMIT_CMD, 
				(data_ptr + cv_length), (pCommand->transportLen - cv_length), 
				(LPVOID*)pCommandResult, bufferLen) == FALSE)
			{
				/* log the error, update command processing status & return */
				(*pCommandResult)->returnStatus = GetLastError();
				*pReturnStatus = (*pCommandResult)->returnStatus;

				SetCmdState(pCommand->commandID, getCommandIndex(FALSE), CV_COMMAND_COMPLETED);
				releaseSemaphore();
				return;
			}
			else
			{
			*pReturnStatus = (*pCommandResult)->returnStatus;
			}

		} else {
			/* submit the command to CV USB device */
			if (processCommand(CV_SUBMIT_CMD, 
				pCommand, pCommand->transportLen, 
				(LPVOID*)pCommandResult, bufferLen) == FALSE)
			{
				/* log the error, update command processing status & return */
				(*pCommandResult)->returnStatus = GetLastError();
				*pReturnStatus = (*pCommandResult)->returnStatus;

				SetCmdState(pCommand->commandID, getCommandIndex(FALSE), CV_COMMAND_COMPLETED);
				releaseSemaphore();
				return;
			}
			else
			{
				*pReturnStatus = (*pCommandResult)->returnStatus;
			}
		}

		/* construct parameter for callback mechanism */
		lParam.callbackInfo = callbackInfo;
		lParam.cmd = pCommand;
		lParam.res = *pCommandResult;

		// check for host control and host storage control
		if ((*pCommandResult)->transportType != CV_TRANS_TYPE_ENCAPSULATED) {
			// return error for host control and storage control
			if ((*pCommandResult)->transportType == CV_TRANS_TYPE_HOST_STORAGE) {
				(*pCommandResult)->returnStatus = CV_HOST_STORE_REQUEST_FAILED;
				logMessage("cvhTransmit: rx host storage request - rejecting\n");
			}
			else if ((*pCommandResult)->transportType == CV_TRANS_TYPE_HOST_CONTROL) {
				(*pCommandResult)->returnStatus = CV_HOST_CONTROL_REQUEST_FAILED;
				logMessage("cvhTransmit: rx host control request - rejecting\n");
			}
		}

		/* handle the command result */
		if (*pReturnStatus == CV_SUCCESS_SUBMISSION)
		{
			/* create a thread for asynchronous processing */
			lParam.mode = ASYNCHRONOUS;
			logMessage("cvhTransmit: CV_SUCCESS_SUBMISSION\n");
			if (createThread(resultHandlerProc, (LPVOID)&lParam) == CV_FAILURE)
			{
				/* create thread failed. release the semaphore */
				*pReturnStatus = GetLastError();
				releaseSemaphore();
				logError(*pReturnStatus, ERROR_LOCATION);
				logMessage("cvhTransmit: failed to create Thread\n");
			}

			return;
		}
		else
		{
			logMessage("cvhTransmit: going to call resultHandlerProcx\n");
			/* use the callback procedure for synchronous processing */
			lParam.mode = SYNCHRONOUS;
			resultHandlerProc(&lParam);
			*pReturnStatus = lParam.res->returnStatus;
		}
	}
	END_TRY

	releaseSemaphore();
}


/*
 * Function		:	cvhUserPrompt
 * Purpose		:	Displays the user prompt for given prompt code. Handles the user
 *					cancellation
 * Parameters	:
 *					uPromptCode			--> prompt code
 *					uExtraDataLength	--> length of the extra data
 *					pExtraData			--> pointer to the extra data
 * Returns		:	cv_status			<-- status of the prompt
 */
CVUSERLIBIFC_API cv_status
cvhUserPrompt(
			  IN	cv_status uPromptCode,
			  IN	uint32_t uExtraDataLength,
			  IN	byte* pExtraData
			  )
{
	char *uStrResId_Msg;

	BEGIN_TRY
	{
		/*Previous and Present prompt is same*/
		if(uPrvPromptCode == uPromptCode ||
			uPromptCode == CV_PROMPT_FOR_RESAMPLE_SWIPE_TIMEOUT ||
			uPromptCode == CV_PROMPT_FOR_RESAMPLE_TOUCH_TIMEOUT)
		{
			/*Check whether user has canceled the same prompt last time*/
			printf("************************************************\n");
			printf("Prompt code (0x%x)\n", uPromptCode);
			printf("************************************************\n");
			logMessage("************************************************\n");
			logMessage("cvhUserPrompt: Prompt code (0x%x)\n", uPromptCode);
			logMessage("************************************************\n");
			if(bUserCanceledThePrompt)
			{
				/*Reset the Cancelation to FALSE*/
				bUserCanceledThePrompt = FALSE;

				/* Reset the uPrvPromptCode to SUCCESS */
				uPrvPromptCode = CV_SUCCESS;

				/*return the Cancellation indication to CV*/
				return CV_CANCELLED_BY_USER;
			}
			else
			{
				return CV_SUCCESS;
			}
		}
		/*Present prompt code is to remove any existing Prompt*/
		else if(uPromptCode == CV_REMOVE_PROMPT)
		{
			/*Reset the Cancelation to FALSE*/
			bUserCanceledThePrompt = FALSE;

			/*Store the Prompt Code*/
			uPrvPromptCode = uPromptCode;
			printf("************************************************\n");
			printf("Remove prompt\n");
			printf("************************************************\n");
			logMessage("************************************************\n");
			logMessage("cvhUserPrompt: Remove prompt\n");
			logMessage("************************************************\n");
		}
		/*Present prompt is to display the new prompt*/
		else
		{

			/*Reset the Cancelation to FALSE*/
			bUserCanceledThePrompt = FALSE;

			/*Store the Prompt Code*/
			uPrvPromptCode = uPromptCode;


			/*Set the Type of the Prompt*/
			switch (uPromptCode)
			{
				/* informative messages */
			case CV_PROMPT_FOR_SMART_CARD:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_SMART_CARD;
				}
				break;

			case CV_PROMPT_FOR_CONTACTLESS:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_CONTACTLESS;
				}
				break;

			case CV_PROMPT_FOR_FINGERPRINT_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FINGERPRINT_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_FINGERPRINT_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FINGERPRINT_TOUCH;
				}
				break;

			case CV_PROMPT_FOR_FIRST_FINGERPRINT_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FIRST_FINGERPRINT_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_SECOND_FINGERPRINT_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_SECOND_FINGERPRINT_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_THIRD_FINGERPRINT_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_THIRD_FINGERPRINT_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_FOURTH_FINGERPRINT_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FOURTH_FINGERPRINT_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_FIRST_FINGERPRINT_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FIRST_FINGERPRINT_TOUCH;
				}
				break;

			case CV_PROMPT_FOR_SECOND_FINGERPRINT_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_SECOND_FINGERPRINT_TOUCH;
				}
				break;

			case CV_PROMPT_FOR_THIRD_FINGERPRINT_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_THIRD_FINGERPRINT_TOUCH;
				}
				break;

			case CV_PROMPT_FOR_FOURTH_FINGERPRINT_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_FOURTH_FINGERPRINT_TOUCH;
				}
				break;

			case CV_PROMPT_FOR_RESAMPLE_SWIPE:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_RESAMPLE_SWIPE;
				}
				break;

			case CV_PROMPT_FOR_RESAMPLE_TOUCH:
				{
					uStrResId_Msg = IDS_CV_PROMPT_FOR_RESAMPLE_TOUCH;
				}
				break;

				/* error messages */
			default:
				{
					uStrResId_Msg = IDS_CV_ERROR_PROMPT;
				}
			}

			if(uPromptCode == CV_PROMPT_CONTACTLESS_DETECTED)
			{
				printf("************************************************\n");
				printf("%s\n", uStrResId_Msg);
				printf("************************************************\n");
				logMessage("************************************************\n");
				logMessage("cvhUserPrompt: %s\n", uStrResId_Msg);
				logMessage("************************************************\n");

				return CV_SUCCESS;
			}

			printf("************************************************\n");
			printf("%s\n", uStrResId_Msg);
			printf("************************************************\n");
			logMessage("************************************************\n");
			logMessage("cvhUserPrompt: %s\n", uStrResId_Msg);
			logMessage("************************************************\n");
		}
	}
	END_TRY

		return CV_SUCCESS;
}


/*
 * Function:
 *      cvhPromptUserPIN
 * Purpose:
 *      Displays the PIN dialog for entering the PIN or cancellation of the prompt.
 * Parameters:
 *		uRetID		<-- User Input Control Code
 *		pPINData	<-- PIN data
  * Returns:
 *      cv_status	<-- Return status of the prompt
 */
CVUSERLIBIFC_API cv_status
cvhPromptUserPIN(
				 IN	uint32_t uPINMsgCode,
				 OUT	char *pPINData
				 )
{
	cv_status cvRetStatus = CV_SUCCESS;
		char PINMsgText[PIN_MSG_LENGTH];
		char *uStrResId;

		/*Get string resource id for the PIN Prompt code*/
			switch (uPINMsgCode)
			{
			case CV_PROMPT_PIN:
				{
					uStrResId = IDS_CV_PROMPT_PIN;
				}
				break;
			case CV_PROMPT_PIN_AND_SMART_CARD:
				{
					uStrResId = IDS_CV_PROMPT_PIN_AND_SMART_CARD;
				}
				break;
			case CV_PROMPT_PIN_AND_CONTACTLESS:
				{
					uStrResId = IDS_CV_PROMPT_PIN_AND_CONTACTLESS;
				}
			default:
				{
					uStrResId = "UNKNOWN prompt.";
				}
			}

		/*Load the PIN msg text from string resource*/
		printf("************************************************\n");
		printf("%s :", uStrResId);
		printf("************************************************\n");
		logMessage("************************************************\n");
		logMessage("cvhPromptUserPIN: %s\n", uStrResId);
		logMessage("************************************************\n");

		/*Set the Window's Content*/
		scanf("%s", PINMsgText);
		if (strlen(PINMsgText) < MIN_PIN_LENGTH) {
			cvRetStatus = CV_CANCELLED_BY_USER;
		}
		else
		{
			memcpy(pPINData, pPinValue, MAX_PIN_LENGTH);
		}

	return cvRetStatus;
}
