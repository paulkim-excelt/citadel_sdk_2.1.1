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
#ifndef _HELPERFUNCTIONS_H_
#define _HELPERFUNCTIONS_H_ 1


/*
* Include file
*/
#include "cvapi.h"
#include "cvusrlib.h"
#include "linuxifc.h"

/* Global declarations for Remote Session*/
extern handle_t hRemoteBinding;

/*CVapi Helper Routines*/

/*
* Function:
*		InitParam_List
* Purpose:
*		This function initializes parameter type and length value to the list
* Parameters:
*		pType		: Type of Encapsulation
*		pLen		: Length of the Parameter
*		param       : Value of the parameter
*		param_list	: List into which the parameters are to be initialized
* Return Value:
*		cv_status
*/
cv_status
InitParam_List(
				IN	cv_param_type pType,
				IN	uint32_t pLen,
				IN	void *param,
				OUT	cv_param_list_entry **param_list
				);

/*
* Function:
*      Free
* Purpose:
*      To Free the allocated memory using malloc
* Parameters:
*		Params		: Variable of Memory allocated
*		nParamCount	: List of params
*/
void Free(IN	void **Params, IN	uint32_t nParamCount);

/*
* Function:
*      ProcessTransmitCmd
* Purpose:
*		Helper Function for appropriatly invoking the cvhTransmit() or cvhTransmitClient()
*
*
* Parameters:
*		stSession			: For identifying the Remote session and Non remote session
*		hBinding			: Handle for in case of Remote session
*		encapedCommand		: Encapsulated command for submission
*		cvStatus			: status of the submitted request
*		encapResult			: Encapsulated result from CV
*
* Returns:
*      cv_status
*/

cv_status
ProcessTransmitCmd(
			IN	cv_session *stSession,
			IN	cv_encap_command **encapedCommand,
			OUT	cv_status *cvStatus,
			OUT	cv_encap_command	**encapedResult
			);

/*
* Function:
*      is_valid_handle
* Purpose:
*      Function to check the handle
* Parameters:
*	cvhandle	:	Address of the cv_handle
*
* Returns:
*      BOOL
*/

BOOL is_valid_handle(IN	cv_handle cvhandle);

/*
* Function:
*      GetCVInternalHandle
* Purpose:
*      Function to get the PHX CV internal handle from the Session (used as cv handle for user modules)
* Parameters:
*		cvHandle	: Handle of the CV
*
* Returns:
*      cv_handle
*/
cv_handle
GetCVInternalHandle(IN cv_handle cvHandle);

/*
* Function:
*      cvhGetUniqueHandleNo
* Purpose:
*      This function will return the unique no by incrementing the previous count. This
*		no will be treated as cv handle to the calling application
* Parameters:
*
* Returns:
*      uint32_t
*/
uint32_t cvhGetUniqueHandleNo();


/*
* Function:
*   GetObjectLenAndData
* Purpose:
*   Function to get the data and the length of different structure
* Parameters:
*	objType			:	Type of the Object
*	objValue		:	Structure of the the Object
*
* Returns:
*	pLen			:	Length of the Object
*	pData			:	Data of the Object
*   cv_status		:	status
*/

cv_status GetObjectLenAndData(
					   IN		cv_obj_type objType,
					   IN		uint32_t objValueLength,
					   IN		void *objValue,
					   OUT		uint32_t *pLen,
					   OUT		void **pData
					   );

/*
* Function:
*      GetValidStatusLen
* Purpose:
*      Getting the valid length of the status type
*
* Parameters:
*		param		:	Type of the Parameter
*
* ReturnValue:
*		uint32_t	:   Length of the status type
*
*
*/
uint32_t GetValidStatusLen(int param) ;


/*
* Function:
*   ConstructBlob
* Purpose:
*   Function to prepare the blob from outbound param list
* Parameters:
*
*
* Returns:
*
*
*
*/
cv_status ConstructBlob(
				IN cv_param_list_entry **outBoundList,
				IN uint32_t numOutputParams,
				OUT cv_param_list_entry **ppstParamListBlob,
				OUT uint32_t *nBlobSize);

cv_status
GetCVSecureSessioncmd(IN cv_param_list_entry **outBoundList,
					  IN BOOL bOSVista,
					  OUT cv_session* stSession);

cv_status
GetPromptUserPinCmd(IN uint32_t uPINMsgCode,
					IN uint32_t auth_list_idx,
					IN cv_param_list_entry **outBoundList,
					IN uint32_t numOutputParams,
					OUT cv_param_list_entry **stParamListBlob,
					OUT uint32_t *pBlobsize);


cv_status
GetCommandIndex(IN	cv_encap_command *stEncapResult,
				OUT uint32_t *uCmdIndex);


/*Secure session and encrypt declaration*/

cv_status
get_SystemOS_ProcessorArchitecture(
					OUT	BOOL *bOSVista,
					OUT BOOL *b64Bit
					);

cv_status
GetCVPublicKeyCmd(
			IN	BOOL bSuiteB,
			IN	uint8_t *pbAntiReplayNonce,
			OUT	cv_encap_command **encapCmdGetPubKeyRequest
			);

cv_status
GetHostEncryptNonceCmd(
			IN	BOOL bSuiteB,
			IN	uint8_t *pbHostNonce,
			IN	uint8_t *pbECCHostPublicKey,
			OUT	cv_encap_command **encapCmdSendHostNonce
			);

/*Validation Declaration*/

cv_status
is_valid_command_id(int param);

cv_status
is_valid_obj_type(short param);

cv_status
is_valid_persistence(int param);

cv_status
is_valid_blob_type(int param);

cv_status
is_valid_enc_op(int param);

cv_status
is_valid_dec_op(int param);
cv_status
is_valid_hmac_op(int param);

cv_status
is_valid_hash_op(int param);

cv_status
is_valid_sign_op(int param);

cv_status
is_valid_hash_type(int param);

cv_status
is_valid_verify_op(int param);

cv_status
is_valid_bulk_mode(int param);

cv_status
is_valid_auth_algorithm(int param);

cv_status
is_valid_key_type(int param);

cv_status
is_valid_status_type(int param);

cv_status
is_valid_object_auth_flags(short param);

cv_status
is_valid_param_type(IN	unsigned int param);

/*
* Function:
*   getAntiHammeringProtectionDelay
* Purpose:
*   Get the Delay from Anti Hammering Protection Delay values from global vars
* Parameters:
*	DelayStatus	:	High or Medium
*
* Returns:
*	DWORD: Returns the Delay value based on the DelayStatus
*
*/

DWORD 
getAntiHammeringProtectionDelay(unsigned int DelayStatus);

/*
* Function:
*   setAntiHammerProtectionDelay
* Purpose:
*   Retrieve the Anti Hammering Protection Delay params from registry and Sets the global vars
* Parameters:
*	
*
* Returns:
*	boolean: true in case of Success / false in case of failure
*
*/
BOOL setAntiHammerProtectionDelay();

/*
* Function:
*		ChangeEndianess
* Purpose:
*		A utility function will change the Endianess of the Signature
*
* Parameters:
*
* ReturnValue:
*
*/
//void ChangeEndianess(unsigned char *pInputBuffer, unsigned int InputBufferLen, unsigned char *pOutputBuffer);

void ChangeEndianess(unsigned char *pInputBuffer, unsigned int InputBufferLen, 
					 DWORD BufferType, unsigned char *pOutputBuffer) ;

uint32_t cvGetDeltaTime(uint32_t startTime);
#endif   /* end _CVUSRLIBGLOBALS_H_ */

