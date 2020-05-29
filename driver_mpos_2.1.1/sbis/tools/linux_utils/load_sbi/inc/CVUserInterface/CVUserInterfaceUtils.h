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
#ifndef _CVUSERINTERFACEUTILS_H_
#define _CVUSERINTERFACEUTILS_H_ 1

#include "linuxifc.h"
/*
 * Function:
 *      initializeData
 * Purpose:
 *      Initializes the global variables
 * Parameters:
 *		callbackInfo <-> callback info
 */
void initializeData(cv_fp_callback_ctx **callbackInfo);
/*
 * Function:
 *      uninitializeData
 * Purpose:
 *      Un-initializes the global variables
 * Parameters:
 *		callbackInfo <-> callback info
 */
void uninitializeData(cv_fp_callback_ctx **callbackInfo);
/*
 * Function:
 *      isResubmitted
 * Purpose:
 *      Checks whether the command is resubmitted or not 
 * Parameters:
 *      ptr_cv_encap_command	--> encapsulated command 
 * Returns:
 *      BOOL					<-- TRUE if resubmitted
 *									FALSE otherwise
 */
BOOL isResubmitted(cv_encap_command *ptr_cv_encap_command);
/*
 * Function:
 *      isPINRequired
 * Purpose:
 *      Checks whether status requires PIN? 
 * Parameters:
 *      status	--> status
 * Returns:
 *      BOOL	<-- TRUE if requires PIN
 *					FALSE otherwise
 */
BOOL isPINRequired(cv_status status);
/*
 * Function:
 *      isCallbackPresent
 * Purpose:
 *      Checks whether the command callback is present or not 
 * Parameters:
 *      pCommand	--> encapsulated command 
 * Returns:
 *      BOOL		<-- TRUE if callback present
 *					    FALSE otherwise
 */
BOOL isCallbackPresent(cv_encap_command *pCommand);
/*
 * Function:
 *      processCommand
 * Purpose:
 *      Gets the device handle 
 * Parameters:
 *		dwIOCtrlCode	 --> device open handle
 *		lpInBuffer		 --> input buffer
 *		dwInBufferSize	 --> input buffer length
 *		lpOutBuffer		 --> output buffer
 *		dwOutBufferSize	 --> output buffer length
 *		lpOverlapped	 --> flag indicates overlapped command processing
 * Returns:
 *      BOOL	<-- TURE on success, FALSE otherwise
 */
BOOL processCommand(DWORD dwIOCtrlCode, 
			   LPVOID lpInBuffer, DWORD dwInBufferSize, 
			   LPVOID *lpOutBuffer, DWORD dwOutBufferSize);

/*
 * Function:
 *      saveCallbackInfo
 * Purpose:
 *      Returns the callback information from command buffer
 * Parameters:
 *		ptr_encap_cmd		--> encapsulated command
 *								SYNCHRONOUS/ASYNCHRONOUS
 *		cv_fp_callback_ctx  <-> callback information
 */
void saveCallbackInfo(cv_encap_command *ptr_encap_cmd, 
			   cv_fp_callback_ctx **callbackInfo);
/*
 * Function:
 *      retrieveCommandIndex
 * Purpose:
 *      Returns the command index stored in the command
 * Parameters:
 *		pCommand			--> encapsulated command
 *		uCmdIndex			<-> command index
 */
void retrieveCommandIndex(cv_encap_command *pCommand,
			   uint32_t *uCmdIndex);
/*
 * Function:
 *      appendCallback
 * Purpose:
 *      Appends the callback context at the end of command
 * Parameters:
 *		ptr_encap_res		<-> encapsulated result
 *		context				-->	callback context
 */
void appendCallback(cv_encap_command **ptr_encap_res, 
			   cv_callback_ctx context);
/*
 * Function:
 *      appendCommandIndex
 * Purpose:
 *      Appends the unique command index on encapsulated command
 * Parameters:
 *		ppCommand		<-> encapsulated command
 */
void appendCommandIndex(cv_encap_command **ppCommand);
/*
 * Function:
 *      cv_callback_status
 * Purpose:
 *      Used to invoke CV USB driver to send callback status
 * Parameters:
 *		ppCommandResult		<-> encapsulated result
 *		lpOverlapped		--> pointer to the overlapped object
 * Returns:
 *      cv_status			<-- status of the processed command
 */
cv_status cv_callback_status(cv_encap_command **ppCommandResult, 
				   LPOVERLAPPED lpOverlapped);
/*
 * Function:
 *      cv_abort
 * Purpose:
 *      Aborts the currently executing command
 * Parameters:
 *		ptr_encap_result	<-> encapsulated result
 * Returns:
 *      cv_status			<-- status of the processed command
 */
cv_status cv_abort(cv_encap_command **ptr_encap_result);
/*
 * Function:
 *      releaseSemaphore
 * Purpose:
 *      Release the semaphore occupied by previous command
 * Returns:
 *      BOOL	<-- TRUE on success FALSE otherwise
 */
BOOL releaseSemaphore(void);
/*
 * Function:
 *      createSemaphore
 * Purpose:
 *      Creates the semaphore to allow single command to be processed ar a time
 * Returns:
 *      BOOL	<-- TRUE on success FALSE otherwise
 */
BOOL createSemaphore(void);
/*
 * Function:
 *      getCommandIndex
 * Purpose:
 *      get the command index
 * Parameter:
 *		bNewIndex - Create new index
 * Returns:
 *		DWORD - unique ID for a command
 */
uint32_t getCommandIndex(BOOL bNewIndex);

BOOL closeSemaphore(void);

/*
 * Function:
 *      createThread
 * Purpose:
 *      get the command index
 * Parameter:
 *		lpThreadProc - thread procedure
 *      lParam - thread parameter
 * Returns:
 *		HANDLE - thread handle
 */
int createThread(void* lpThreadProc, void* lParam);
/*
 * Function:
 *      displayMsgDlg
 * Purpose:
 *      Display the prompt dialog box
 * Parameters:
 *		promptInfo	--> prompt message information
 * Returns:
 *      cv_status	<-- messagebox display status
 */
cv_status displayMsgDlg(prompt_params *promptInfo);

#endif												/* end of _CVUSERINTERFACEUTILS_H_ */
