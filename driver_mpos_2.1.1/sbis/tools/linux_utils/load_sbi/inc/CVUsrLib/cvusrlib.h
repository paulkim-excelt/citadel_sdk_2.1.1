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
#ifndef _CVUSRLIB_H_
#define _CVUSRLIB_H_ 1

#include <stdint.h>
#include <sys/types.h>
#include "cvapi.h"
#include "cvcommon.h"
#include "linuxifc.h"

#define RSA2048_KEY_LEN 256
#define ECC256_KEY_LEN 32
#define ECC256_SIG_LEN 2*ECC256_KEY_LEN
#define ECC256_POINT (2*ECC256_KEY_LEN)

#define SIGNATURE_DATA	1
#define KEYBLOB_DATA	2

/* Session definition */
typedef struct td_host_callback {
				cv_callback			*callback;	/*callback routine passed in with API */
				cv_callback_ctx		context;	/* context passed in with API*/
				uint32_t			numParams;	/* number of input parameters */
} PACKED cvh_host_callback;


typedef unsigned char BYTE;
typedef BYTE *PBYTE;

/*CVapi Helper Routines*/

/*
* Function:
*      cvhcvHandleRequired
* Purpose:
*		Some commands to not require CV Handle to communicate with USH.
*       This api will return 0 for those commands that do not require handles
*
* Parameters:
*		cmd_id				: Command ID of the operation
*
* ReturnValue:
*      1 - require cvhandle
*      0 - Don't require cvhandle
*/
BOOL
cvhCvHandleRequired (	IN	cv_command_id cmd_id
					);

/* 
* Function:
*      cvhManageCVAPICall
* Purpose:
*		The manager rotuine which is called by all cv_xxx functions. which internally creates
*		New Session handle, calls Encapsulate command, DLL(Transmit), Decapsulate command and 
*		Save the return values.
*
* Parameters:
*		numOutputParams		: Number of parameters to be sent to CV
*		outBoundList		: List of Outbound parameters 
*		numInputParams		: Number of result parameters expected from CV
*		inBoundList			: List of inbound parameters
*		ptr_callback_ctx	: Pointer to application callback and context
*		cvHandle			: Address of the cv_handle
*		auth_list_idx		: Index number of the authorization list
*		cmd_id				: Command ID of the operation
*
* ReturnValue:
*      cv_status
*/
cv_status 
cvhManageCVAPICall (
				IN	uint32_t numOutputParams, 
				IN	cv_param_list_entry **outBoundList, 
				IN	uint32_t numInputParams,
				OUT	cv_param_list_entry **inBoundList, 
				IN	cv_fp_callback_ctx *ptr_callback_ctx,
				IN	cv_handle cvHandle,
				IN	uint32_t auth_list_idx, 
				IN	cv_command_id cmd_id
				);

/* 
* Function:
*      cvhManageCallback
* Purpose:
*		If the return type of flags in Encapsulated result is either Intermediate callback or
*		Completion Callback then this routine is internally called by User Mode Interface.
* Parameters:
*		status			:	Return value of the Encapsulated Result
*		extraDataLength	:
*		extraData		:
*		ptr_encap_result:	Pointer to Encapusulated result
* ReturnValue:
*      cv_status
*/
cv_status 
cvhManageCallback(
				IN	uint32_t status,
				IN	uint32_t extraDataLength,
				IN	void *extraData,
				IN	void  *ptr_encap_result
				);

/* 
* Function:
*      cvhEncapsulateCmd
* Purpose:
*     Encapsulates the out bound parameters into Encapsulation structure
*     Inbound list is encapsulated and stored for Host callback structure
* Parameters:
*		stBlob				:	Outbound Parameters
*		nBlobLen			:   Length of the Outbound Parameters
*		InBoundlist			:	Inbound Parameters
*		nInbound			:	number of Inbound parameters
*		ptr_callback_ctx	:	Pointer to Callback and context structure
*		stSession			:	Pointer to Session structure
*		cmd_id				:	Command ID for CV operation
*		stInEncapCmd		:	Allocated Encapsulated Structure
*
* ReturnValue:
*		stInEncapCmd		:	filled Encapsulated structure based on securesession / callback & context
*		cv_status			
*/
cv_status 
cvhEncapsulateCmd (
				IN	cv_param_list_entry *stBlob,
				IN	uint32_t nBlobLen,
				IN	cv_param_list_entry **InBoundlist,
				IN	uint32_t nInbound,  
				IN	cv_fp_callback_ctx *ptr_callback_ctx,
				IN	cv_session_details *stSessionDetails,
				IN	cv_command_id cmd_id,
				IN	BOOL b64Bit,
				IN	uint32_t stInEncapCmdSize,
				OUT	cv_encap_command *stInEncapCmd
				);

/* 
* Function:
*      cvhDecapsulateCmd
* Purpose:
*      Decapsulate the resulted information from CV
* Parameters:
*		stEncapResult	:	Structure of the Encapsulated command
*		NoOfParms		:	Number of params
*		retDecapParams	:	Allocated encapsulate structure
* ReturnValue:
*		retDecapParams	:	List of inbound params	
*		cv_status
*/
cv_status 
cvhDecapsulateCmd(
			IN	cv_encap_command *stEncapResult,
			IN	cv_session *stSession,
			IN	uint32_t *NoOfParms, 
			OUT	cv_param_list_entry **retDecapParams
			);

/*
* Function:
*      cvhEncryptCmd
* Purpose:
*      Function to Encrypt the  Outbound params
* Parameters:
*		stInEncapCmd			- Encapsulated Command for Encryption
*		stSession				- Session structure
*		stParamListBlob			- ParamBlob to be Encrypted
*		nBlobLen				- ParamBlob length
*		pbData					- Secure Session Key Encrypting Command
*		pbSecureSessionHMAC		- Session HMAC of the complete Encap command
*		pbEncryptedParamBlob	- Encrypted Blob
* Returns:
*      cv_status
*/
cv_status 
cvhEncryptCmd(
			IN	cv_encap_command *stInEncapCmd, 
			IN	cv_session *stSession,
			IN	cv_param_list_entry *stParamListBlob, 
			IN	DWORD nBlobLen,
			IN	uint8_t *pbData, 
			//IN	PBYTE *pbSecureSessionHMAC, 
			OUT	uint8_t **pbEncryptedParamBlob,
			OUT	DWORD *pbdwBlobLen
			);

/* 
* Function:
*      cvhDecryptCmd
* Purpose:
*      Decrypt the resulted information from CV
* Parameters:
*		stInEncapCmd	:	Structure of the Encapsulated command
*		
* ReturnValue:
*		pbDecryptedParamBlob	:	Decrypted param blob structure	
*		pbdwBlobLen				:	Length of the inbound list
*		cv_status				:	return status
*/
cv_status cvhDecryptCmd(
					IN 	cv_session *stSession,
					IN cv_encap_command *stEncapResult, 
					OUT	uint8_t **pbDecryptedParamBlob, 
					OUT DWORD *pbdwBlobLen);

/* 
* Function:
*      cvhSaveReturnValues
* Purpose:
*       Saves the return values received from the CV 
* Parameters:
*		Param			:	Pointer to param list to Save the return values
*		NoOfParams		:	Number of Params
*		inBoundList		:	List of Inbound received from CV
*		nRetStatus		:   Return status of the CV to validate CV_INVALID_OUTPUT_PARAMETER_LENGTH
* ReturnValue:
*		Param			:	List of save return values
*		cv_status
*/
cv_status 
cvhSaveReturnValues(
			OUT	void **Param,
			IN	uint32_t NoOfParams,
			IN  unsigned int nRetStatus,
			IN	cv_param_list_entry **inBoundList
			);

cv_status 
cvhSetUpSecureSession(
					  IN	BOOL bWinVista, 
					  IN	uint8_t **pbSecureSessionKey, 
					  IN    cv_session *stSession,
					  OUT	uint8_t **pbSecureSessionHMACKey
					  );


unsigned int 
MSCAPI_GenerateRandom(
			OUT	uint8_t *pbData,
			IN	DWORD dwLength
			);

unsigned int 
MSCAPI_VerifyDSASignature(
			IN	byte* cvPublicKey,
			IN	DWORD cvPublicKeyLen, 
			IN	byte* pbAntiReplayNonce,
			IN	DWORD AntiReplayNonceLen,
			IN	byte* cvPublicKeySig, 
			IN	DWORD cvPublicKeySigLen
			);

unsigned int 
MSCAPI_VerifyRSASignature(void);

unsigned int 
MSCAPI_GenerateSessionEncKey(
			IN	uint8_t *pbCVNonce, 
			IN	uint8_t *pbHostNonce, 
			OUT	uint8_t **pbSessionKey
			);

unsigned int
MSCAPI_GenerateSessionHMACKey(
			OUT	uint8_t **pbSessionHMACKey
			);

unsigned int 
MSCAPI_EncryptHostNonce(
			IN	uint8_t *pbCVPublicKey, 
			IN	DWORD cvPublicKeyLen, 
			IN	uint8_t *pbHostNonce, 
			OUT	uint8_t **pbEncHostNonce,
			OUT	DWORD *pdwEncHostNonceLength
			);

unsigned int 
MSCAPI_EncryptParamBlob(
			IN	uint8_t *pbParamBlob, 
			IN	DWORD dwParamBlobLen, 
			IN	uint8_t *pbIVForEnc,
			IN	uint8_t *pbSecureSessionKey, 
			OUT	uint8_t **pbEncryptedParamBlob,
			OUT	DWORD	*pdwEncParamBlobLen
			);

unsigned int 
MSCAPI_DecryptParamBlob(
				IN	uint8_t *pbEncParamBlob, 
				IN	DWORD dwEncParamBlobLen, 
				IN	uint8_t *pbIVForEnc, 
				IN	uint8_t *pbSecureSessionKey, 
				OUT	uint8_t **pbDecryptedParamBlob, 
				OUT	DWORD *pbdwBlobLen
				);

unsigned int
MSCAPI_GenerateSecureSessionHMAC(
					IN	uint8_t *pbBuffer,
					IN	uint32_t nBufferSize,
					IN	uint8_t *pbKeyToHMAC,
					IN  DWORD dwKeyToHMACLen,
					OUT	uint8_t **pbSecureSessionHMAC
					);

unsigned int 
MSCAPI_GeneratePrivacyKey(
				IN	uint8_t *pbBuffer,
				IN	DWORD dwBufLength, 
				OUT	uint8_t **pbPrivacyKey
				);

unsigned int 
MSCAPI_GenerateAppUserIDHash(
				IN	uint8_t *pbBuffer, 
				IN	DWORD dwBufLength, 
				OUT	uint8_t **pbAppUserIDHash
				);


BOOL 
MSCNG_GenerateRandom(
				OUT	uint8_t *pbBuffer, 
				IN	DWORD dwBufLength
				);

BOOL 
MSCNG_GenerateHostECCKey(
				IN	uint8_t	**pbECCHostPublicKey
				);

BOOL 
MSCNG_VerifyECDSAPubKeySignature(
				IN	uint8_t *pbCVECCPublicKey, 
				IN	DWORD dwCVECCPubKeyLen, 
				IN	uint8_t *pbECDSASignature,
				IN	DWORD dwSignatureLen
				);

BOOL 
MSCNG_VerifyECDSAKdiSignature(void);

BOOL 
MSCNG_GenerateSessionHMACKey(
				OUT	uint8_t **pbSessionHMACKey
				);

BOOL 
MSCNG_GenerateSessionEncryptionKey(
				IN	uint8_t *pbCVECCPublicKey, 
				IN	DWORD dwCVECCPubKeyLen,
				IN	uint8_t *pbCVNonce, 
				IN	DWORD dwCVNonceLength,
				IN	uint8_t *pbHostNonce, 
				IN	DWORD dwHostNonceLength,
				OUT	uint8_t **pbSessionEncKey
				);

unsigned int
MSCNG_GenerateSecureSessionHMAC(
					IN	uint8_t *pbBuffer,
					IN	uint32_t nBufferSize,
					IN	PBYTE pbSecureSessionHMACKey,
					IN	DWORD dwSecureSessionHMACKeyLen,
					OUT	uint8_t **pbSecureSessionHMAC
					);


#endif				       /* end _CVUSRLIB_H_ */
