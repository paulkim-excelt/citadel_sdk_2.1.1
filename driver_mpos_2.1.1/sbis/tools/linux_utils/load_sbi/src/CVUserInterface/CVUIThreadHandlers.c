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
#include "CVUIThreadHandlers.h"
#include "CVUserInterfaceUtils.h"
#include "CVCmdStateList.h"
#include "CVLogger.h"
#include "linuxifc.h"

/*
 * Local macro definitions 
 */
#ifdef _MANAGED
#pragma managed(push, off)
#endif
void __attribute__((constructor)) Statelistobj_init(void)
{
	logMessage("StateListObj init done \n");
    /* create new state list object */

}

void __attribute__((destructor)) Statelistobj_fini(void)
{
	logMessage("StateListObj uninit done\n");
	/* clear and free state list object */

}

#ifdef _MANAGED
#pragma managed(pop)
#endif
/*
 * Function:
 *      resultHandlerProc
 * Purpose:
 *      Handles intermediate callback and completion callback
 * Parameters:
 *		lParam		--> thread param
 * Returns:
 *      DWORD		<-- status of the intermediate command processing
 */
unsigned long 
resultHandlerProc(
				IN	LPVOID lParam
				)
{
	cv_status			status = CV_SUCCESS;
	thread_param		*threadParam =  (thread_param*)lParam;
	
	logMessage("resultHandlerProc: enter\n");
	/* check for valid input parameters */
	if (threadParam == NULL)
	{
		status = CV_UI_FAILED_ASYNC_THREAD_CREATION;	/* null command */
		logError(CV_UI_FAILED_ASYNC_THREAD_CREATION, ERROR_LOCATION);
		logMessage("ERROR: threadParam == NULL\n");
		return status;
	}

	/* check the callback status and wait for result */
	do
	{
		logMessage("flags.returnType: 0x%x\n", threadParam->res->flags.returnType);
		switch (threadParam->res->flags.returnType)
		{
			case CV_RET_TYPE_SUBMISSION_STATUS:		/* successful submission status */
			{
				logMessage("flags.returnType: CV_RET_TYPE_SUBMISSION_STATUS\n");
				// Included the below condition from BCM as part of Merging - 5th Dec 2007
				if ( threadParam->cmd->commandID == CV_CMD_LOAD_SBI || threadParam->cmd->commandID == CV_CMD_FLASH_PROGRAMMING ||
					 threadParam->cmd->commandID == CV_CMD_FW_UPGRADE_START || threadParam->cmd->commandID == CV_CMD_FW_UPGRADE_UPDATE || threadParam->cmd->commandID == CV_CMD_FW_UPGRADE_COMPLETE )
				{
					threadParam->res->flags.returnType = CV_RET_TYPE_RESULT;
					status = threadParam->res->returnStatus;
					break;
				}
				else if ( threadParam->res->returnStatus == CV_USH_BOOT_FAILURE )
				{
					threadParam->res->flags.returnType = CV_RET_TYPE_RESULT;
					status = threadParam->res->returnStatus;
					break;
				}
				/* get the command result */
				status = cv_callback_status(&threadParam->res, NULL);

			}
			break;
			case CV_RET_TYPE_INTERMED_STATUS:		/* intermediate status */
			{
				logMessage("flags.returnType: CV_RET_TYPE_INTERMED_STATUS\n");
				if (threadParam->callbackInfo->callback != NULL)
				{
					logMessage("callback != NULL\n");
					/* append the callback context with result */
					appendCallback(&threadParam->res, 
							threadParam->callbackInfo->context);

					logMessage("calling callback\n");
					/* call cvhManageCallback */
					status = threadParam->callbackInfo->callback(
						threadParam->res->returnStatus, 
						0, NULL, 
						threadParam->res);

					logMessage("callback status: 0x%x\n", status);
					/* get the status from CV */
					threadParam->res->returnStatus = status;
					//warning: implicit declaration of function `cv_callback_status'-- commented
					status = cv_callback_status(&threadParam->res, NULL);
				}
			}
			break;
			case CV_RET_TYPE_RESULT:				/* completion status */
			{
				logMessage("flags.returnType: CV_RET_TYPE_RESULT\n");
				/* update the return status */
				status = threadParam->res->returnStatus;
			}
			break;

			default:
				logMessage("flags.returnType: default\n");
				break;
			}
	}
	while ((status != CV_CANCELLED_BY_USER) &&
		(threadParam->res->flags.returnType != CV_RET_TYPE_RESULT) &&
		(status != CV_UI_FAILED_COMMAND_PROCESS));

	/* you got a completion callback with encapsulated result */
	if (isPINRequired(threadParam->res->returnStatus) == FALSE)
	{
		logMessage("resultHandlerProc() isPINRequired == false\n");
		/* command completed, remember the state and release semaphore */
		SetCmdState(threadParam->cmd->commandID, getCommandIndex(FALSE), CV_COMMAND_COMPLETED);
		if (threadParam->mode!=SYNCHRONOUS)
			releaseSemaphore();
	}
	else
	{
		if (threadParam->res->flags.suppressUIPrompts)
		{
			logMessage("resultHandlerProc() suppressUIPrompts\n");
			/*Command Completed, Release the Semaphore*/
			SetCmdState(threadParam->cmd->commandID, getCommandIndex(FALSE), CV_COMMAND_COMPLETED);
			if (threadParam->mode!=SYNCHRONOUS)
				releaseSemaphore();
		}
		//otherwise, wait for the command re-submission
		else
		{
			/* special case - don't release the semaphore and expect command resubmission */
			SetCmdState(threadParam->res->commandID, getCommandIndex(FALSE), CV_COMMAND_WAITING);
		}
	}

	/* for asynchronous mode invoke the callback */
	if ((threadParam->mode == ASYNCHRONOUS) && 
		(threadParam->callbackInfo->callback != NULL))
	{
		appendCallback(&threadParam->res, callbackInfo->context);
		status = threadParam->callbackInfo->callback(
			threadParam->res->returnStatus, 0, NULL, 
			threadParam->res);
	}
	
	logMessage("resultHandlerProc: exit\n");
	/* return the status */
	return status;
}
