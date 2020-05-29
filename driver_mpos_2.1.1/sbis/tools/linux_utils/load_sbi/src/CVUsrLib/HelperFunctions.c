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


#define _CRT_RAND_S /*For Random No Generation*/

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "CVUsrLibGlobals.h"
#include "CVList.h"
#include "SetupSecureSession.h"
#include "CVLogger.h"

extern cv_version		version;
/* shared memory variable pointers */


static DWORD	ulAntiHammeringDelayMed = ANTI_HAMMERING_DELAY_MED_DEFAULT, 
				ulAntiHammeringDelayHigh = ANTI_HAMMERING_DELAY_HIGH_DEFAULT ;

/*
* Function:
*      InitParam_List
* Purpose:
*      This function initializes parameter type and length value to the list
* Parameters:
*		pType		: Type of Encapsulation
*		pLen		: Length of the Parameter
*		param       : Value of the parameter
*		param_list	: List into which the parameters are to be initialized
*
* ReturnValue:
*      cv_status
*/
cv_status
InitParam_List(
			   IN		cv_param_type pType,
			   IN		uint32_t pLen,
			   IN		void *param,
			   OUT		cv_param_list_entry **param_list
			   )
{
	cv_status status = CV_SUCCESS;

	/*For all the Param types the following code is common.*/
	BEGIN_TRY
	{
		if(NULL == ((*param_list) = (cv_param_list_entry *) malloc(sizeof(cv_param_list_entry)+ pLen)))
		{
			logErrorMessage("Memory Allocation Failed", ERROR_LOCATION);
			return  CV_MEMORY_ALLOCATION_FAIL;
		}

		memset((*param_list), 0, sizeof(cv_param_list_entry) + pLen);
		(*param_list)->paramType = pType;
		(*param_list)->paramLen = pLen;
		if(pType != CV_ENCAP_INOUT_LENVAL_PAIR) // This type will not have data.
			memcpy((((byte*)(*param_list))+ sizeof(cv_param_list_entry)),param, pLen);
	}
	END_TRY

	return status;
}


/*
* Function:
*		Free
* Purpose:
*		To Free the allocated memory using malloc
* Parameters:
*		Params		: Variable of Memory allocated
*		nParamCount	: List of params
*
*/
void
Free(
	 IN	void **Params,
	 IN	uint32_t nParamCount
	 )
{
	uint32_t nParams = 0;

	BEGIN_TRY
	{
		if(Params == NULL)
			return;

		/*Freeing the memory pointing by individual element's*/
		for(nParams = 0; nParams < nParamCount; nParams++)
		{
			if(Params[nParams] != NULL)
			{
				free(Params[nParams]);
				Params[nParams] = NULL;
			}
		}
	}
	END_TRY
}


cv_status
InitLibrary(void)
{
	cv_status status = CV_SUCCESS;
	char* error=NULL;
	//logMessage("start cvhInitLibrary\n");
	//Load the Interface Library
	dlerror();
	hUserIfc = dlopen(UILibrary, RTLD_NOW);
	//logMessage("dlopen() hUserIfc: %p\n", hUserIfc);
	if(hUserIfc != NULL)
	{
		if(fncvhTransmit == NULL /*|| fncvhFPEventRegistration == NULL*/)
		{
			fncvhTransmit = (LPFNTRANSMIT)dlsym(hUserIfc, UITransmit);
			//fncvhFPEventRegistration = (LPFNFPEVENTREGISTRATION)dlsym(hUserIfc, UIFPEventRegistration);
			if( fncvhTransmit == NULL /*|| fncvhFPEventRegistration == NULL*/)
			{
				perror("cvhInitLibrary dlsym");
				return CV_ERROR_LOADING_INTERFACE_LIBRARY;
			}
		}
		if(fncvhUserPrompt == NULL)
		{
			fncvhUserPrompt = (LPFNUSERPROMPT)dlsym(hUserIfc, UIUserPrompt);
			if( fncvhUserPrompt == NULL )
			{
				perror("cvhInitLibrary dlsym");
				return CV_ERROR_LOADING_INTERFACE_LIBRARY;
			}
		}
		if(fncvhUserPromptBIP == NULL)
		{
			fncvhUserPromptBIP = (LPFNPROMPTUSERPIN)dlsym(hUserIfc, UIPromptUserPINBIP);
			if( fncvhUserPromptBIP == NULL )
			{
				perror("cvhInitLibrary dlsym");
				return CV_ERROR_LOADING_INTERFACE_LIBRARY;
			}
		}
	}
	else
	{
		if((error = dlerror()) != NULL)
			logMessage("%s\n",error);
		//cvhErrorExit((LPTSTR)cvhInitLibrary);
		return  CV_ERROR_LOADING_INTERFACE_LIBRARY;
	}
	//logErrorMessage("CV User Interface Dll Loaded Successfully", ERROR_LOCATION);
	//logMessage("end cvhInitLibrary\n");
	return status;
}


/*
* Function:
*      cvhCloseLibrary
* Purpose:
*      Close the User Interface Library (DLL)
*
* Parameters:
*		None
*
* ReturnValue:
*		cv_status
*/
void CloseLibrary(void)
{
	//logMessage("start cvhCloseLibrary; hUserIfc: %p\n", hUserIfc);
	if(hUserIfc != NULL)
	{
		fncvhUserPrompt = NULL;
		fncvhTransmit = NULL;
		dlclose(hUserIfc);
		hUserIfc = NULL;
	}
	//logMessage("end cvhCloseLibrary\n");
	return;
}


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
				   )
{
	cv_status status = CV_SUCCESS;

	//Calling InitLibrary to intialize the dll
	if(hUserIfc == NULL) {
		logErrorMessage("calling InitLibrary()....", ERROR_LOCATION);
		status = InitLibrary();
	}

	if (status == CV_SUCCESS) {
		logErrorMessage("calling cvhTransmit....", ERROR_LOCATION);
		//Calling fncvhTransmit command to send Encapbuffer to device
		fncvhTransmit(encapedCommand, cvStatus, encapedResult);
	}
	else {
		logErrorMessage("InitLibrary() failed", ERROR_LOCATION);
		CloseLibrary();
	}

	return  status;
}


/*
The following function routines are used for validating CV parameters and types
*/

/*
* Function:
*      is_valid_command_id
* Purpose:
*      Validating the Command Id
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_command_id(
					IN	int param
					)
{
	switch(param)
	{
	case CV_ABORT :
	case CV_CALLBACK_STATUS :
	case CV_CMD_OPEN :
	case CV_CMD_OPEN_REMOTE :
	case CV_CMD_CLOSE :
	case CV_CMD_GET_SESSION_COUNT :
	case CV_CMD_INIT :
	case CV_CMD_GET_STATUS :
	case CV_CMD_SET_CONFIG :
	case CV_CMD_CREATE_OBJECT :
	case CV_CMD_DELETE_OBJECT :
	case CV_CMD_SET_OBJECT :
	case CV_CMD_GET_OBJECT :
	case CV_CMD_ENUMERATE_OBJECTS :
	case CV_CMD_DUPLICATE_OBJECT :
	case CV_CMD_IMPORT_OBJECT :
	case CV_CMD_EXPORT_OBJECT :
	case CV_CMD_CHANGE_AUTH_OBJECT :
	case CV_CMD_AUTHORIZE_SESSION_OBJECT :
	case CV_CMD_DEAUTHORIZE_SESSION_OBJECT :
	case CV_CMD_GET_RANDOM :
	case CV_CMD_ENCRYPT :
	case CV_CMD_DECRYPT :
	case CV_CMD_HMAC :
	case CV_CMD_HASH :
	case CV_CMD_HASH_OBJECT :
	case CV_CMD_GENKEY :
	case CV_CMD_SIGN :
	case CV_CMD_VERIFY :
	case CV_CMD_OTP_GET_VALUE :
	case CV_CMD_OTP_GET_MAC :
	case CV_CMD_UNWRAP_DLIES_KEY :
	case CV_CMD_FINGERPRINT_ENROLL :
	case CV_CMD_FINGERPRINT_VERIFY :
	case CV_SECURE_SESSION_GET_PUBKEY :
	case CV_SECURE_SESSION_HOST_NONCE :
	case CV_HOST_STORAGE_GET_REQUEST :
	case CV_HOST_STORAGE_STORE_OBJECT :
	case CV_HOST_STORAGE_DELETE_OBJECT :
	case CV_HOST_STORAGE_RETRIEVE_OBJECT :
	case CV_HOST_STORAGE_OPERATION_STATUS :
	case CV_HOST_CONTROL_GET_REQUEST :
	case CV_HOST_CONTROL_FEATURE_EXTRACTION :
	case CV_HOST_CONTROL_CREATE_TEMPLATE :
	case CV_HOST_CONTROL_IDENTIFY_AND_HINT_CREATION :
	case CV_CMD_FINGERPRINT_CAPTURE :
	case CV_CMD_FINGERPRINT_IDENTIFY :
	case CV_CMD_FINGERPRINT_CONFIGURE :
	case CV_CMD_FINGERPRINT_TEST :
	case CV_CMD_FINGERPRINT_CALIBRATE :
	case CV_CMD_BIND :
	case CV_CMD_BIND_CHALLENGE :
	case CV_CMD_IDENTIFY_USER :
	case CV_CMD_FLASH_PROGRAMMING :
	case CV_CMD_HOTP_UNBLOCK :

		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid CommandId", ERROR_LOCATION);
			return CV_INVALID_COMMAND;

		}
	}
}


/*
* Function:
*      is_valid_obj_type
* Purpose:
*      Validating the type of Object
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_obj_type(
				  IN	short param
				  )
{
	cv_status status = CV_SUCCESS;
	if (param !=0)
	{
		switch(param)
		{
		case CV_TYPE_CERTIFICATE :
		case CV_TYPE_RSA_KEY :
		case CV_TYPE_DH_KEY :
		case CV_TYPE_DSA_KEY :
		case CV_TYPE_ECC_KEY :
		case CV_TYPE_AES_KEY :
		case CV_TYPE_FINGERPRINT :
		case CV_TYPE_OPAQUE :
		case CV_TYPE_OTP_TOKEN :
		case CV_TYPE_PASSPHRASE :
		case CV_TYPE_HASH :
		case CV_TYPE_AUTH_SESSION :
		case CV_TYPE_HOTP :	
			return status;
		default :
			{
				logErrorMessage("Invalid Object Type", ERROR_LOCATION);
				return  CV_INVALID_OBJECT_TYPE;
			}

		}
	}
	return CV_INVALID_OBJECT_TYPE;
}


/*
* Function:
*      is_valid_persistence
* Purpose:
*      Validating the type of persistence
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_persistence(
					 IN	int param
					 )
{
	switch(param)
	{
	case CV_KEEP_PERSISTENCE :
	case CV_MAKE_PERSISTENT :
	case CV_MAKE_VOLATILE : return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid PERSISTENT Type", ERROR_LOCATION);
			return CV_INVALID_PERSISTENT_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_blob_type
* Purpose:
*      Validating the type of blob
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_blob_type(
				   IN	int param
				   )
{
	switch(param)
	{
	case CV_NATIVE_BLOB :
	case CV_MSCAPI_BLOB :
	case CV_CNG_BLOB :
	case CV_PKCS_BLOB :
	case CV_NATIVE_BLOB_RETAIN_HANDLE:
		return CV_SUCCESS;
	default :
		{
			logErrorMessage("Invalid BLOB Type", ERROR_LOCATION);
			return CV_INVALID_BLOB_TYPE;
		}
	}
}


/*
* Function:
*      is_valid_enc_op
* Purpose:
*      Validating the type of encrypt
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_enc_op(
				IN	int param
				)
{
	switch(param)
	{
	case CV_BULK_ENCRYPT :
	case CV_BULK_ENCRYPT_THEN_AUTH :
	case CV_BULK_AUTH_THEN_ENCRYPT :
	case CV_ASYM_ENCRYPT :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid ENC_OP Type", ERROR_LOCATION);
			return CV_INVALID_ENC_OP_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_dec_op
* Purpose:
*      Validating the type of Decrypt
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_dec_op(
				IN	int param
				)
{
	switch(param)
	{
	case CV_BULK_DECRYPT :
	case CV_BULK_DECRYPT_THEN_AUTH :
	case CV_BULK_AUTH_THEN_DECRYPT :
	case CV_ASYM_DECRYPT :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid DEC_OP Type", ERROR_LOCATION);
			return CV_INVALID_DEC_OP_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_hmac_op
* Purpose:
*      Validating the type of HMAC
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_hmac_op(
				 IN	int param
				 )
{
	switch(param)
	{
	case CV_HMAC_SHA :
	case CV_HMAC_SHA_START :
	case CV_HMAC_SHA_UPDATE :
	case CV_HMAC_SHA_FINAL :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid HMAC Type", ERROR_LOCATION);
			return CV_INVALID_HMAC_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_hash_op
* Purpose:
*      Validating the Hash OP
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_hash_op(
				 IN	int param
				 )
{
	switch(param)
	{
	case CV_SHA :
	case CV_SHA_START :
	case CV_SHA_UPDATE :
	case CV_SHA_FINAL :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid HASH_OP Type", ERROR_LOCATION);
			return CV_INVALID_HASH_OP;
		}

	}
}


/*
* Function:
*      is_valid_sign_op
* Purpose:
*      Validating the Sign OP
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_sign_op(
				 IN	int param
				 )
{
	switch(param)
	{
	case CV_SIGN_NORMAL :
	case CV_SIGN_USE_KDI :
	case CV_SIGN_USE_KDIECC:
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid Sign Type", ERROR_LOCATION);
			return CV_INVALID_SIGN_TYPE;
		}
	}

}


/*
* Function:
*      is_valid_verify_op
* Purpose:
*      Validating the type of Verify
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_verify_op(
				   IN	int param
				   )
{

	if(param == CV_VERIFY_NORMAL)
	{
		return CV_SUCCESS;
	}
	else
	{
		logErrorMessage("Invalid Verify Type", ERROR_LOCATION);
		return CV_INVALID_VERIFY;
	}
}


/*
* Function:
*      is_valid_bulk_mode
* Purpose:
*      Validating the type of Bulk
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_bulk_mode(
				   IN	int param
				   )
{
	switch(param)
	{
	case CV_CBC :
	case CV_ECB :
	case CV_CTR :
	case CV_CMAC :
	case CV_CCMP :
	case CV_CBC_NOPAD:
	case CV_ECB_NOPAD:
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid Bulk Mode Type", ERROR_LOCATION);
			return CV_INVALID_BULK_MODE;
		}
	}
}


/*
* Function:
*      is_valid_auth_algorithm
* Purpose:
*      Validating the Auth Algorithm
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_auth_algorithm(
						IN	int param
						)
{
	switch(param)
	{
	case CV_AUTH_ALG_NONE :
	case CV_AUTH_ALG_HMAC_SHA1 :
	case CV_AUTH_ALG_SHA1 :
	case CV_AUTH_ALG_HMAC_SHA2_256 :
	case CV_AUTH_ALG_SHA2_256 :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid Auth Alg Type", ERROR_LOCATION);
			return CV_INVALID_AUTH_ALG;
		}

	}
}


/*
* Function:
*      is_valid_hash_type
* Purpose:
*      Validating the Hash type
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_hash_type(
				   IN	int param
				   )
{
	switch(param)
	{
	case CV_SHA1 :
	case CV_SHA256 :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid Hash Type", ERROR_LOCATION);
			return CV_INVALID_HASH_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_key_type
* Purpose:
*      Validating the Key Type
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_key_type(
				  IN	int param
				  )
{
	switch(param)
	{
	case CV_AES :
	case CV_DH_GENERATE :
	case CV_DH_SHARED_SECRET :
	case CV_RSA :
	case CV_DSA :
	case CV_ECC :
	case CV_ECDH_SHARED_SECRET :
	case CV_OTP_RSA :
	case CV_OTP_OATH :
		return CV_SUCCESS;

	default :
		{
			logErrorMessage("Invalid Key Type", ERROR_LOCATION);
			return CV_INVALID_KEY_TYPE;
		}

	}
}


/*
* Function:
*      is_valid_status_type
* Purpose:
*      Validating the Status Type
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_status_type(
					 IN	int param
					 )
{
	switch(param)
	{
	case CV_STATUS_VERSION :
		break;
	case CV_STATUS_OP :
		break;
	case CV_STATUS_SPACE :
		break;
	case CV_STATUS_TIME :
		break;
	case CV_STATUS_FP_POLLING_INTERVAL:
		break;
	case CV_STATUS_FP_PERSISTANCE:
		break;
	case CV_STATUS_FP_HMAC_TYPE:
		break;
	case CV_STATUS_FP_FAR:
		break;
	case CV_STATUS_DA_PARAMS:
		break;
	case CV_STATUS_POLICY:
		break;
	case CV_STATUS_HAS_CV_ADMIN:
		break;
	case CV_STATUS_IDENTIFY_USER_TIMEOUT:
		break;
	case CV_STATUS_ERROR_CODE_MORE_INFO:
		break;
	case CV_STATUS_SUPER_CHALLENGE:
		break;
	case CV_STATUS_IN_DA_LOCKOUT:
		break;
	case CV_STATUS_DEVICES_ENABLED:
		break;
	case CV_STATUS_ANTI_REPLAY_OVERRIDE_OCCURRED:
		break;
	case CV_STATUS_RFID_PARAMS_ID:
		break;
	case CV_STATUS_ST_CHALLENGE:
		break;
	case CV_STATUS_IS_BOUND:
		break;
	case CV_STATUS_HAS_SUPER_REGISTRATION:
		break;
	default :
		{
			logErrorMessage("Invalid Status Type", ERROR_LOCATION);
			return CV_INVALID_STATUS_TYPE;
		}
	}
	return CV_SUCCESS;
}


/*
* Function:
*      is_valid_param_type
* Purpose:
*      Validating the Param Type
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
*		cv_status
*
*/
cv_status
is_valid_param_type(
					IN	unsigned int param
					)
{
	switch(param)
	{
	case CV_ENCAP_STRUC:
		break;
	case CV_ENCAP_LENVAL_STRUC :
		break;
	case CV_ENCAP_LENVAL_PAIR :
		break;
	case CV_ENCAP_INOUT_LENVAL_PAIR :
		break;
	default :
		{
			logErrorMessage("Invalid Status Type", ERROR_LOCATION);
			return CV_INVALID_INBOUND_PARAM_TYPE;
		}
	}
	return CV_SUCCESS;
}


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
uint32_t
GetValidStatusLen(
				  IN int param
				  )
{
	uint32_t pLen = 0;

	switch(param)
	{
	case CV_STATUS_VERSION :
		pLen = sizeof(cv_version);
		break;
	case CV_STATUS_OP :
		pLen = sizeof(cv_operating_status);
		break;
	case CV_STATUS_SPACE :
		pLen = sizeof(cv_space);
		break;
	case CV_STATUS_TIME :
		pLen = sizeof(cv_time);
		break;
	case CV_STATUS_FP_POLLING_INTERVAL:
		pLen = sizeof(cv_config_type);
		break;
	case CV_STATUS_FP_PERSISTANCE:
		pLen = sizeof(cv_config_type);
		break;
	case CV_STATUS_FP_HMAC_TYPE:
		pLen = sizeof(cv_config_type);
		break;
	case CV_STATUS_FP_FAR: //need to clarify
		pLen = sizeof(cv_config_type);
		break;
	case CV_STATUS_DA_PARAMS:
		pLen = sizeof(cv_da_params);
		break;
	case CV_STATUS_POLICY:
		pLen = sizeof(cv_policy);
		break;
	case CV_STATUS_HAS_CV_ADMIN:
		pLen = sizeof(cv_bool);
		break;
	case CV_STATUS_IDENTIFY_USER_TIMEOUT:
		pLen = sizeof(cv_config_type);
		break;
	case CV_STATUS_ERROR_CODE_MORE_INFO:
		pLen = sizeof(uint32_t);
		break;
	case CV_STATUS_DEVICES_ENABLED:
		pLen = sizeof(uint32_t);
		break;
	case CV_STATUS_SUPER_CHALLENGE:
		pLen = sizeof(cv_super_challenge);
		break;
	case CV_STATUS_IN_DA_LOCKOUT:
		pLen = sizeof(uint32_t);
		break;
	case CV_STATUS_ANTI_REPLAY_OVERRIDE_OCCURRED:
		pLen = sizeof(uint32_t);
		break;
	case CV_STATUS_RFID_PARAMS_ID:
		pLen = sizeof(uint32_t);
		break;
	case CV_STATUS_ST_CHALLENGE:
		pLen = sizeof(cv_st_challenge);
		break;
	case CV_STATUS_IS_BOUND:
		pLen = sizeof(cv_bool);
		break;
	case CV_STATUS_HAS_SUPER_REGISTRATION:
		pLen = sizeof(cv_bool);
		break;
	default :
		{
			logErrorMessage("Invalid Status Type", ERROR_LOCATION);
			return CV_INVALID_STATUS_TYPE;
		}
	}
	return pLen;
}

/*
* Function:
*      is_valid_object_auth_flags
* Purpose:
*      Validating the Object Authorizations
*
* Parameters:
*		param	:	Type of the Parameter
*
* ReturnValue:
cv_status
*
*/
cv_status
is_valid_object_auth_flags(
						   IN	short param
						   )
{
	if (param & ~CV_OBJ_AUTH_FLAGS_MASK)
	{
		logErrorMessage("Invalid Authorization Type", ERROR_LOCATION);
		return CV_INVALID_OBJ_AUTH_FLAG;
	}
	return CV_SUCCESS;
}


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
BOOL
is_valid_handle(
				IN	cv_handle cvhandle
				)
{
	BOOL bValidSession = FALSE;

	BEGIN_TRY
	{
		if(cvhandle == 0)
			bValidSession = FALSE;
		else if(GetSessionOfTheHandle(cvhandle) != NULL)
			bValidSession = TRUE;
	}
	END_TRY

	return bValidSession;
}


/*
* Function:
*      cvhGetCVInternalHandle
* Purpose:
*      Function to get the PHX CV internal handle from the Session (used as cv handle for user modules)
* Parameters:
*		cvHandle	: Handle of the CV
*
* Returns:
*      cv_handle
*/
cv_handle GetCVInternalHandle(IN cv_handle cvHandle)
{
	cv_handle CVInternalHandle = 0;
	void *pCVSession = NULL;

	pCVSession = GetSessionOfTheHandle(cvHandle);
	if(pCVSession != NULL)
	{
		memcpy(&CVInternalHandle, &((cv_session_details*)pCVSession)->cvInternalHandle, sizeof(uint32_t));
	}

	return CVInternalHandle;
}


/*
* Function:
*      cvhGetUniqueHandleNo
* Purpose:
*      This function will return a unique random no. This no will be treated as
*	   cv handle to the calling application
* Parameters:
*
* Returns:
*      uint32_t
*/
uint32_t cvhGetUniqueHandleNo()
{
	uint32_t	uUniqueHandleNo		= 0;
	uint32_t	uNoOfAttempts		= 50;

	/*Generate Random No*/
	while(uNoOfAttempts-- != 0)
	{
		srand(time(NULL));
		uUniqueHandleNo = (uint32_t)rand();
		/*Chech this Random No is already in the previous list*/
		if(uUniqueHandleNo == 0)  /*Checking for 0 is very much IMPORTANT*/
		{
			return 0;
		}
		else
		{
			/* == NULL means this uUniqueHandleNo is not present in the list*/
			if(GetSessionOfTheHandle(uUniqueHandleNo) == NULL)
				break;
		}
	}

	/*Return the Unique Random No*/
	return uUniqueHandleNo;
}


/*
* Function:
*      get_SystemOS_ProcessorArchitecture
* Purpose:
*      Function to check OS type & Processor architecture
* Parameters:
*
* Returns:
*      cv_status
*/
cv_status
get_SystemOS_ProcessorArchitecture(
								   OUT	BOOL *bOSVista,
								   OUT BOOL *b64Bit
								   )
{
	*bOSVista = FALSE; //enable the Vista OS
	*b64Bit = FALSE; //enable the 64 bit flag

	return CV_SUCCESS;
}


/*
* Function:
*      GetCVPublicKeyCmd
* Purpose:
*      Function to create the encap command for submitting the request to get the CV Public Key Request
* Parameters:
*		bSuiteB				- Whether SuiteB flag is Set
*		pbAntiReplayNonce	- Anti-Replay nonce
*		cv_encap_command	- Encapsulated command for Submission
*
* Returns:
*      cv_status
*/
cv_status
GetCVPublicKeyCmd(
				  IN	BOOL bSuiteB,
				  IN	uint8_t *pbAntiReplayNonce,
				  OUT	cv_encap_command **encapCmdGetPubKeyRequest
				  )
{
	char				*pParamBlob = NULL;
	uint32_t			blobLen = 0;
	unsigned int		encapCmdSize = 0;
	cv_encap_command	*encapCmd = 0;

	pParamBlob = (char*)malloc(ANTI_REPLAY_NONCE_SIZE+ sizeof(bSuiteB));
	if (pParamBlob == NULL)
	{
		logErrorMessage("Memory Allocation is fail ", ERROR_LOCATION);
		return CV_MEMORY_ALLOCATION_FAIL;
	}
	memset(pParamBlob, 0x00, ANTI_REPLAY_NONCE_SIZE+ sizeof(bSuiteB));
	memcpy(pParamBlob, pbAntiReplayNonce, ANTI_REPLAY_NONCE_SIZE);
	blobLen += ANTI_REPLAY_NONCE_SIZE;
	memcpy(pParamBlob+blobLen, &bSuiteB, sizeof(bSuiteB));
	blobLen += sizeof(bSuiteB);

	encapCmdSize = sizeof(cv_encap_command) + blobLen - sizeof(encapCmd->parameterBlob);

	/* This extra 4 bytes required by User interface to store the unique command index*/
	encapCmdSize += sizeof(uint32_t);

	encapCmd = (cv_encap_command*)malloc(encapCmdSize);
	if (encapCmd == NULL)
	{
		free(pParamBlob);
		pParamBlob = NULL;
		return CV_MEMORY_ALLOCATION_FAIL;
	}

	memset(encapCmd, 0x00, encapCmdSize);

	encapCmd->transportType = CV_TRANS_TYPE_ENCAPSULATED;
	encapCmd->commandID = CV_SECURE_SESSION_GET_PUBKEY;
	encapCmd->flags.USBTransport = TRUE;
	encapCmd->flags.secureSession = TRUE;
	encapCmd->version.versionMajor = version.versionMajor;
	encapCmd->version.versionMinor = version.versionMinor;
	encapCmd->parameterBlobLen = blobLen;
	memcpy((void*)&(encapCmd->parameterBlob), pParamBlob, blobLen);
	encapCmd->transportLen = sizeof(cv_encap_command) + encapCmd->parameterBlobLen - sizeof(encapCmd->parameterBlob);

	*encapCmdGetPubKeyRequest = encapCmd;

	//Free the pParamBlob
	if(pParamBlob != NULL)
	{
		free(pParamBlob);
		pParamBlob = NULL;
	}

	return CV_SUCCESS ;
}


/*
* Function:
*      GetHostEncryptNonceCmd
* Purpose:
*      Function to create the encap command for submitting the request to send the Encrypted Host Nonce
* Parameters:
*		bSuiteB				- Whether SuiteB flag is Set
*		pbHostNonce			- Host nonce
*		pbECCHostPublicKey	- Host Public Key (will be NULL in case of Non-SuiteB)
*		cv_encap_command	- Encapsulated command for Submission
*
* Returns:
*      cv_status
*/
cv_status
GetHostEncryptNonceCmd(
					   IN	BOOL bSuiteB,
					   IN	uint8_t *pbHostNonce,
					   IN	uint8_t *pbECCHostPublicKey,
					   OUT	cv_encap_command	**encapCmdSendHostNonce
					   )
{
	char				*pParamBlob = NULL;
	uint32_t			blobLen = 0;
	unsigned int		encapCmdSize = 0;
	cv_encap_command	*encapCmd = NULL;

	if(!bSuiteB)
	{
		// Send the Encrypted Host Nonce to CV
		pParamBlob = (char*)malloc(RSA_PUBLIC_KEY_SIZE);
		if ( pParamBlob == NULL)
			return CV_MEMORY_ALLOCATION_FAIL;

		memset(pParamBlob, 0x00, RSA_PUBLIC_KEY_SIZE);
		memcpy(pParamBlob, pbHostNonce, RSA_PUBLIC_KEY_SIZE);
		blobLen += RSA_PUBLIC_KEY_SIZE;

		encapCmdSize = sizeof(cv_encap_command) + blobLen - sizeof(encapCmd->parameterBlob);

		/* This extra 4 bytes required by User interface to store the unique command index*/
		encapCmdSize += sizeof(uint32_t);

		encapCmd = (cv_encap_command*)malloc(encapCmdSize);
		if ( encapCmd == NULL)
		{
			if ( pParamBlob )
				free(pParamBlob);

			logErrorMessage("Memory Allocation is fail", ERROR_LOCATION);
			return CV_MEMORY_ALLOCATION_FAIL;
		}
		memset(encapCmd, 0x00, encapCmdSize);

		encapCmd->transportType = CV_TRANS_TYPE_ENCAPSULATED;
		encapCmd->commandID = CV_SECURE_SESSION_HOST_NONCE;
		encapCmd->flags.USBTransport = TRUE;
		encapCmd->flags.secureSession = TRUE;
		encapCmd->version.versionMajor = version.versionMajor;
		encapCmd->version.versionMinor = version.versionMinor;
		encapCmd->parameterBlobLen = blobLen;
		memcpy((void*)&(encapCmd->parameterBlob), pParamBlob, blobLen);
		encapCmd->transportLen = sizeof(cv_encap_command) + encapCmd->parameterBlobLen - sizeof(encapCmd->parameterBlob);
	}
	else
	{
		pParamBlob = (char*)malloc(ECC_PUBLIC_KEY_SIZE + HOST_NONCE_SIZE);
		if ( pParamBlob == NULL)
			return CV_MEMORY_ALLOCATION_FAIL;

		memset(pParamBlob, 0x00, ECC_PUBLIC_KEY_SIZE + HOST_NONCE_SIZE);

		memcpy(pParamBlob, pbHostNonce, HOST_NONCE_SIZE);
		blobLen += HOST_NONCE_SIZE;
		memcpy(pParamBlob+blobLen, pbECCHostPublicKey, ECC_PUBLIC_KEY_SIZE);
		blobLen += ECC_PUBLIC_KEY_SIZE;

		encapCmdSize = sizeof(cv_encap_command) + blobLen - sizeof(encapCmd->parameterBlob);

		/* This extra 4 bytes required by User interface to store the unique command index*/
		encapCmdSize += sizeof(uint32_t);

		encapCmd = (cv_encap_command*)malloc(encapCmdSize);
		if ( encapCmd == NULL)
		{
			if (pParamBlob)
				free(pParamBlob);
			logErrorMessage("Memory Allocation is fail ", ERROR_LOCATION);
			return CV_MEMORY_ALLOCATION_FAIL;
		}

		memset(encapCmd, 0x00, encapCmdSize);

		encapCmd->transportType = CV_TRANS_TYPE_ENCAPSULATED;
		encapCmd->commandID = CV_SECURE_SESSION_HOST_NONCE;
		encapCmd->flags.secureSession = TRUE;
		encapCmd->version.versionMajor = version.versionMajor;
		encapCmd->version.versionMinor = version.versionMinor;
		encapCmd->parameterBlobLen = blobLen;
		memcpy((void*)&(encapCmd->parameterBlob), pParamBlob, blobLen);
		encapCmd->transportLen = sizeof(cv_encap_command) + encapCmd->parameterBlobLen - sizeof(encapCmd->parameterBlob);
	}

	*encapCmdSendHostNonce = encapCmd;

	if ( pParamBlob )
		free(pParamBlob);

	//if (encapCmd)
	//free(encapCmd);

	return CV_SUCCESS;
}


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
cv_status
GetObjectLenAndData(
					IN		cv_obj_type objType,
					IN		uint32_t objValueLength,
					IN		void *objValue,
					OUT		uint32_t *pLen,
					OUT		void **pData
					)
{
	cv_status status = CV_SUCCESS;

	logErrorMessage("Inside cvhInitParam_List", ERROR_LOCATION);

	BEGIN_TRY
	{
		switch(objType)
		{
		case CV_TYPE_CERTIFICATE:
			{
				*pLen = ((cv_obj_value_certificate *)objValue)->certLen;
				*pData = &((cv_obj_value_certificate *)objValue)->cert;
			}
			break;
		case CV_TYPE_OPAQUE:
			{
				*pLen = ((cv_obj_value_opaque *)objValue)->valueLen;
				*pData = ((cv_obj_value_opaque *)objValue)->value;
			}
			break;
		case CV_TYPE_AES_KEY:
			{
				void *tmpobjValue = NULL;
				tmpobjValue = (void*) malloc(objValueLength);
				if(tmpobjValue == NULL)
				{
					return CV_MEMORY_ALLOCATION_FAIL;
				}
				memset(tmpobjValue, 0, objValueLength);
				//copy the size of structure
				memcpy(pLen,&objValueLength,sizeof(uint32_t));
				//copy both length and data
				memcpy(tmpobjValue, objValue, objValueLength);
				*pData = (cv_obj_value_aes_key *)tmpobjValue;

				//*pLen = ((cv_obj_value_aes_key *)objValue)->keyLen;
				//*pData = ((cv_obj_value_aes_key *)objValue)->key;

			}
			break;
		case CV_TYPE_PASSPHRASE:
			{
				*pLen = ((cv_obj_value_passphrase *)objValue)->passphraseLen;
				*pData = ((cv_obj_value_passphrase *)objValue)->passphrase;
			}
			break;
		case CV_TYPE_HASH:
			{
				//Calculate the length of Hash - based on the hashtype either SHA1_LEN / SHA256_LEN
				if(((cv_obj_value_hash *)objValue)->hashType == CV_SHA1)
				{
					//the sizeof hashtype and the constant value of SHA1_LEN (XP)
					*pLen = (SHA1_LEN + sizeof(cv_hash_type));
				}
				if(((cv_obj_value_hash *)objValue)->hashType == CV_SHA256)
				{
					//the sizeof hashtype and the constant value of SHA256_LEN (VISTA)
					*pLen = (SHA256_LEN + sizeof(cv_hash_type));
				}

				*pData = ((cv_obj_value_hash *)objValue);
			}
			break;

		case CV_TYPE_RSA_KEY:
		case CV_TYPE_DH_KEY:
		case CV_TYPE_DSA_KEY:
		case CV_TYPE_ECC_KEY:
			{
				*pLen = ((cv_obj_value_key *)objValue)->keyLen;
				*pData = ((cv_obj_value_key *)objValue)->key;
			}
			break;
		case CV_TYPE_OTP_TOKEN:
			{
				//
			}
			break;
		case CV_TYPE_AUTH_SESSION:
			{
				void *tmpobjValue = NULL;
				tmpobjValue = (void*) malloc(objValueLength);
				if(tmpobjValue == NULL)
				{
					return CV_MEMORY_ALLOCATION_FAIL;
				}
				memset(tmpobjValue, 0x00, objValueLength);
				//copy the size of structure
				memcpy(pLen,&objValueLength,sizeof(uint32_t));
				//copy both length and data
				memcpy(tmpobjValue, objValue, objValueLength);
				*pData = (cv_obj_value_auth_session*)tmpobjValue;
			}
			break;
		default:
			{
				//
			}
			break;
		}
	}
	END_TRY

		logErrorMessage("Exiting cvhInitParam_List", ERROR_LOCATION);

	return status;
}


/*
* Function:
*   ConstructBlob
* Purpose:
*   Function to prepare the blob from outbound param list
* Parameters:
*	outBoundList	:	parm list entry of outbound list
*	numOutputParams	:	number of outbound params
*
* Returns:
*	ppstParamListBlob:	filled the outbound paramlist blob
*	nBlobSize		:	size of the outbound paramlist blob
*	cv_status		:	return status
*
*/
cv_status ConstructBlob(IN cv_param_list_entry **outBoundList,
						IN uint32_t numOutputParams,
						OUT cv_param_list_entry **ppstParamListBlob,
						OUT uint32_t *nBlobSize)
{
	cv_status			status = CV_SUCCESS;
	uint32_t			nParams = 0;
	uint32_t			nMaxParams = 0;
	byte				*ptTmpBlob = NULL;
	byte				*tmpParamData = NULL;
	cv_param_list_entry *stParamListBlob = *ppstParamListBlob;

	BEGIN_TRY
	{
		//Calculate the size of all the outBoundList params
		for(nParams=0; nParams < numOutputParams; nParams++)
		{
			uint32_t allocLen = 0;

			allocLen = (outBoundList[nParams]->paramLen%PACKING_SIZE)? (PACKING_SIZE + (outBoundList[nParams]->paramLen/PACKING_SIZE)*PACKING_SIZE):(outBoundList[nParams]->paramLen);

			if(outBoundList[nParams]->paramType == CV_ENCAP_INOUT_LENVAL_PAIR)
				*nBlobSize += sizeof(cv_param_list_entry);
			else
				*nBlobSize += sizeof(cv_param_list_entry) + allocLen;

			// Find out the Heighest paramLen in the Blob
			if(nMaxParams < outBoundList[nParams]->paramLen)
				nMaxParams = outBoundList[nParams]->paramLen;
		}

		//check the allocation of paramlist blob for the purpose of userprompt pin
		if(stParamListBlob != NULL)
			Free((void *)&stParamListBlob,1);

		//Allocate the total size of outBoundList
		stParamListBlob = (cv_param_list_entry *)malloc(*nBlobSize);
		if(NULL == stParamListBlob)
		{
			logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
			return  CV_MEMORY_ALLOCATION_FAIL;
		}
		memset(stParamListBlob, 0x00, *nBlobSize);

		// Store the new memory address into ppstParamListBlob[0]
		*ppstParamListBlob = stParamListBlob;

		// Store the starting memory address
		ptTmpBlob = (byte *)stParamListBlob;

		// Memory for storing the data portion alone for simplification
		tmpParamData = (byte *)malloc(nMaxParams);
		if(NULL == tmpParamData)
		{
			logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
			return  CV_MEMORY_ALLOCATION_FAIL;
		}
		memset(tmpParamData, 0, nMaxParams);

		// Consolidateing all the outBoundList as Blob
		for(nParams=0; nParams < numOutputParams; nParams++)
		{
			uint32_t		nDataSize = 0;
			uint32_t		nBytesLeft = 0;

			// copy type and lentgh elements
			((cv_param_list_entry *)ptTmpBlob)->paramType = outBoundList[nParams]->paramType;
			((cv_param_list_entry *)ptTmpBlob)->paramLen = outBoundList[nParams]->paramLen;

			// Increment the ptTmpBlob by the size of cv_param_list_entry as these many bytes are already copied
			ptTmpBlob += sizeof(cv_param_list_entry);

			// There will not be any data for this type
			if(outBoundList[nParams]->paramType == CV_ENCAP_INOUT_LENVAL_PAIR)
			{
				continue;
			}

			// Copy the Data into tmpParamData
			memcpy(tmpParamData, (byte *)outBoundList[nParams] + sizeof(cv_param_list_entry), outBoundList[nParams]->paramLen);

			// Calculate the datasize of max possible multiple of 4 bytes and bytes left
			nDataSize = (outBoundList[nParams]->paramLen%PACKING_SIZE)? (outBoundList[nParams]->paramLen/PACKING_SIZE)*PACKING_SIZE: outBoundList[nParams]->paramLen;
			nBytesLeft = (outBoundList[nParams]->paramLen%PACKING_SIZE);

			// copy the datasize of max possible multiple of 4 bytes
			if(nDataSize >= PACKING_SIZE)
			{
				memcpy(ptTmpBlob, tmpParamData, nDataSize);
				ptTmpBlob += nDataSize;
			}

			// copy the bytes left if any
			if(nBytesLeft > 0)
			{
				// Padding method, eg: 12c40000
				memcpy(ptTmpBlob, tmpParamData + nDataSize, nBytesLeft);
				ptTmpBlob += sizeof(uint32_t);
			}

			// Reset the tmpParamDataAddrs memory and restore the org address of tmpParamData
			memset(tmpParamData, 0, nMaxParams);
		}

		free(tmpParamData);
	}
	END_TRY

	return status;
}


/*
* Function:
*   GetCVSecureSessioncmd
* Purpose:
*   Function to prepare the cv_session structure for secure session
* Parameters:
*	outBoundList	:	Param list entry of outboundlist to recquire info
*	bOSVista		:	OS Type either XP/Vista
*
* Returns:
*	stSession		:	structure of cv_session to fill the secure session elements
*	cv_status		:	return status
*
*/
cv_status GetCVSecureSessioncmd(IN cv_param_list_entry **outBoundList,
								IN BOOL bOSVista,
								OUT cv_session* stSession)
{
	uint8_t		*pbSecureSessionKey = {NULL};
	uint8_t		*pbSecureSessionHMACKey = {NULL};
	//BYTE		*pbAPPUsrBuffer = NULL;
	//BYTE		*pbObjPassPharseBuffer = NULL;
	cv_status	status = CV_SUCCESS;

	BEGIN_TRY
	{
		//calling cvhSetupSecureSession
		status = cvhSetUpSecureSession(bOSVista, &pbSecureSessionKey, stSession, &pbSecureSessionHMACKey);
		if(status != CV_SUCCESS)
		{
			goto clean_up;
		}

		//copy the value to cv_session secure session key
		memcpy(&stSession->secureSessionKey,pbSecureSessionKey,AES_128_BLOCK_LEN);

		//Check the OS(VISTA/XP) and copy the value to cv_session secure session HMAC key
		if((stSession->flags & CV_USE_SUITE_B) != CV_USE_SUITE_B)
		{
			memcpy(&stSession->secureSessionHMACKey.SHA1,pbSecureSessionHMACKey,SHA1_LEN); //xp
		}
		else
		{
			memcpy(&stSession->secureSessionHMACKey.SHA256,pbSecureSessionHMACKey,SHA256_LEN); //vista
		}

clean_up:
		Free((void *)&pbSecureSessionKey, 1);
		Free((void *)&pbSecureSessionHMACKey, 1);

	}
	END_TRY

	return status;
}


/*
* Function:
*   GetPromptUserPinCmd
* Purpose:
*   Function to get the pin value and construct the Prompt User Pin Authlist structure
* Parameters:
*	auth_list_idx	:	Index of the authlist of cv_xxx api
*	outBoundList	:	List of Outbound parameters
*	numOutputParams	:	Number of parameters to be sent to CV
*
* Returns:
*	stParamListBlob	:	structure of outbound parameters
*	pBlobsize		:	Length of the Outbound Parameters
*
*/
cv_status GetPromptUserPinCmd(IN uint32_t uPINMsgCode,
							  IN uint32_t auth_list_idx,
							  IN cv_param_list_entry **outBoundList,
							  IN uint32_t numOutputParams,
							  OUT cv_param_list_entry **stParamListBlob,
							  OUT uint32_t *pBlobsize)
{
	cv_status				status			    = CV_SUCCESS;
	byte					*pAuthList			= NULL;
	uint32_t				uAuthListLen		= 0;
	cv_auth_hdr				stNew_Auth_Hdr		= {0};
	cv_auth_entry_PIN		*pAuthEntryPin		= {NULL};
	uint32_t				nBlobSize			= 0;
	unsigned int			TmpParamtype		= 0;
	char					strPinValue[PINSIZE]	= {0};
	cv_bool					bAuthPINExists = FALSE ;

	BEGIN_TRY
	{
//==== Op 1
		//Get the user input (either PIN value or Cancellation status)
		status = fncvhUserPromptBIP(uPINMsgCode, strPinValue);

		//If status is prompt pin failure return status.
		if(status == CV_PROMPT_PIN_FAILURE)
		{
			logErrorMessage("cvhPromptUserPin is Fail ", ERROR_LOCATION);
			goto clean_up;
		}

		//if status is CV_SUCCESS or CV_CANCELLED_BY_USER,
		//then insert authtype as cv_auth_pin and insert the PIN value
		//PIN value could be "" (only in case of CV_CANCELLED_BY_USER) or some value
		if((status == CV_SUCCESS) || (status == CV_CANCELLED_BY_USER))
		{
			//calculate the size of paramblob size with updated param size
			uint16_t nAuthEntryLen = 0;
			uint32_t nTmpOffset = 0;
			uint8_t nCurrent_numEntries = 0;
			uint32_t nPINAuthOffset = 0;
			uint32_t nNextAuthOffset = 0;

			nBlobSize = 0;

//==== Op 2
			//Construct the cv_auth_entry_PIN
			if(strlen(strPinValue) > 0)
			{
				nAuthEntryLen = sizeof(pAuthEntryPin->PINLen) + (uint8_t)strlen(strPinValue);
			}
			else
			{
				nAuthEntryLen = sizeof(cv_auth_entry_PIN);
			}

			pAuthEntryPin = (cv_auth_entry_PIN*)malloc(nAuthEntryLen);
			if(pAuthEntryPin == NULL)
			{
				logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
				status = CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}
			memset(pAuthEntryPin, 0x00, nAuthEntryLen);

			//copy the length of pinvalue and value
			pAuthEntryPin->PINLen = (uint8_t)strlen(strPinValue);
			if(strlen(strPinValue) > 0)
				memcpy(pAuthEntryPin->PIN, strPinValue, strlen(strPinValue));

			//Constructing the cv_auth_hdr with respect to the cv_auth_entry_PIN
			stNew_Auth_Hdr.authType = CV_AUTH_PIN;
			stNew_Auth_Hdr.authLen = nAuthEntryLen;

//==== Op 3
			//Read the count of Auth_list Entries
			nCurrent_numEntries = ((cv_auth_list*)(((byte*)(outBoundList[auth_list_idx])) + sizeof(cv_param_list_entry)))->numEntries;

			//Calculate the total space for this parameter
			if ( nCurrent_numEntries > 0 )
			{
				uint32_t AuthOffset = 0;
				cv_auth_hdr	stCurrent_Auth_Hdr	= {0};

				/*Check whether CV_AUTH_PIN entry is already present*/
				AuthOffset  = sizeof(cv_auth_list);
				while(AuthOffset < outBoundList[auth_list_idx]->paramLen)
				{
					memcpy(&stCurrent_Auth_Hdr, ((byte*)(outBoundList[auth_list_idx]) + sizeof(cv_param_list_entry) + AuthOffset), sizeof(cv_auth_hdr));
					if(stCurrent_Auth_Hdr.authType == CV_AUTH_PIN )
					{
						nPINAuthOffset = AuthOffset;
						nNextAuthOffset = AuthOffset + sizeof(cv_auth_hdr) + stCurrent_Auth_Hdr.authLen;
						bAuthPINExists = TRUE;
						break; //No more search required :)
					}

					AuthOffset += sizeof(cv_auth_hdr) + stCurrent_Auth_Hdr.authLen;
				}

				if(bAuthPINExists) //So adjust to the new size for insertion
				{
					uAuthListLen = (outBoundList[auth_list_idx]->paramLen
						- stCurrent_Auth_Hdr.authLen) + stNew_Auth_Hdr.authLen; // delta space
				}
				else //Space for the CV_AUTH_PIN
				{
					uAuthListLen = outBoundList[auth_list_idx]->paramLen
						+ sizeof(cv_auth_hdr) + stNew_Auth_Hdr.authLen;
				}
			}
			else
			{
				uAuthListLen = sizeof(cv_auth_list) + sizeof(cv_auth_hdr) + stNew_Auth_Hdr.authLen;
			}

			//prepare the authlist
			pAuthList = (byte*)malloc(uAuthListLen);
			if(NULL == pAuthList)
			{
				logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
				status = CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}
			memset(pAuthList, 0, uAuthListLen);

//==== Op 4
			//Some valid Auth List is present
			if(nCurrent_numEntries > 0)
			{
				if(bAuthPINExists)
				{
					uint32_t nBytesCopyed = 0;
					uint32_t nBytesToBeCopyed = 0;

					//Copy the initial data
					nBytesToBeCopyed = nPINAuthOffset;
					memcpy(pAuthList, ((byte*)(outBoundList[auth_list_idx])) + sizeof(cv_param_list_entry), nBytesToBeCopyed);
					nBytesCopyed += nPINAuthOffset;

					//Replace the previous PIN auth list with the new one
					//copy the cv_auth_hdr with respect to cv_auth_entry_PIN
					nBytesToBeCopyed  = sizeof(cv_auth_hdr);
					memcpy(pAuthList + nBytesCopyed, &stNew_Auth_Hdr, nBytesToBeCopyed);
					nBytesCopyed += sizeof(cv_auth_hdr);

					//Copy the auth entry pin
					nBytesToBeCopyed = stNew_Auth_Hdr.authLen;
					memcpy(pAuthList + nBytesCopyed, pAuthEntryPin, nBytesToBeCopyed);
					nBytesCopyed += stNew_Auth_Hdr.authLen;

					//Copy the remaining data if present
					nBytesToBeCopyed = outBoundList[auth_list_idx]->paramLen - nNextAuthOffset;
					if(nBytesToBeCopyed > 0)
						memcpy(pAuthList + nBytesCopyed,
						((byte*)(outBoundList[auth_list_idx]))
						+ sizeof(cv_param_list_entry) +
						nNextAuthOffset, nBytesToBeCopyed);
				}
				else
				{
					//copy the existing cv_auth_list, cv_auth_hdr(s) & auth_entry(s)
					memcpy(pAuthList, ((byte*)(outBoundList[auth_list_idx])) + sizeof(cv_param_list_entry),
						outBoundList[auth_list_idx]->paramLen);

					nTmpOffset += outBoundList[auth_list_idx]->paramLen;

					// Since cv_auth_entry_PIN will be appended, increment the numEntries
					((cv_auth_list*)pAuthList)->numEntries++;
				}
			}
//==== Op 5
			//Incomplete Auth List is present, so initilize this list.
			else
			{
				((cv_auth_list*)pAuthList)->numEntries = 1;
				((cv_auth_list*)pAuthList)->listID = 1;
				((cv_auth_list*)pAuthList)->flags = CV_OBJ_AUTH_FLAGS_MASK; //Which flag should be set?

				nTmpOffset += sizeof(cv_auth_list);
			}

			if(!bAuthPINExists)
			{
				//copy the cv_auth_hdr with respect to cv_auth_entry_PIN
				memcpy(pAuthList + nTmpOffset, &stNew_Auth_Hdr, sizeof(cv_auth_hdr));
				nTmpOffset += sizeof(cv_auth_hdr);

				//copy the auth entry pin
				memcpy(pAuthList + nTmpOffset, pAuthEntryPin, stNew_Auth_Hdr.authLen);
				nTmpOffset += stNew_Auth_Hdr.authLen;
			}

//==== Op 6
			//copying the paramtype of outboundlist[auth_list_idx]
			TmpParamtype = outBoundList[auth_list_idx]->paramType;

			//reallocating the memory for outboundlist[auth_list_idx]
			outBoundList[auth_list_idx] = realloc(outBoundList[auth_list_idx],(sizeof(cv_param_list_entry)+ uAuthListLen));
			memset(outBoundList[auth_list_idx], 0, (sizeof(cv_param_list_entry)+ uAuthListLen));

			//updating the outboundlist
			outBoundList[auth_list_idx]->paramType = TmpParamtype;
			outBoundList[auth_list_idx]->paramLen = uAuthListLen;
			memcpy((((byte*)(outBoundList[auth_list_idx]))+ sizeof(cv_param_list_entry)),pAuthList, uAuthListLen);

//==== Op 7
			//call ConstructBlob to Prepare the Blob from outBoundList info
			status = ConstructBlob(outBoundList, numOutputParams, stParamListBlob, &nBlobSize);
			if(status != CV_SUCCESS)
				goto clean_up;

			//return value blobsize with new size
			*pBlobsize = nBlobSize;
		}
	}
	END_TRY

clean_up:
	//free the allocated memory
	Free((void *)&pAuthEntryPin,1);
	Free((void *)&pAuthList,1);

	return status;
}


/*
* Function:
*   GetCommandIndex
* Purpose:
*   Get the unique command Index
* Parameters:
*	stEncapResult	:	Result of Encapsulted strucuture
*
* Returns:
*	uCmdIndex		:	Unique command id
*
*/
cv_status
GetCommandIndex(IN cv_encap_command *stEncapResult,
				OUT uint32_t *uCmdIndex)
{
	uint32_t		nSize	= 0;
	cv_status		status	= CV_SUCCESS;

	BEGIN_TRY
	{
		if(stEncapResult->flags.completionCallback == TRUE)
		{
			//Identify the callback & context buffer size
			nSize = sizeof(cv_fp_callback_ctx); //need to check 32/64 bit
		}
		//copy the cmd index from stencapresult
		memcpy(uCmdIndex, (((byte*)stEncapResult) + stEncapResult->transportLen + nSize),
			sizeof(uint32_t));
	}
	END_TRY

	return status;
}


BOOL setAntiHammerProtectionDelay()
{
	 BOOL		bRetValue			= TRUE;
	return bRetValue;
}


DWORD getAntiHammeringProtectionDelay(unsigned int DelayStatus)
{
	switch (DelayStatus)
	{
		case CV_ANTIHAMMERING_PROTECTION_DELAY_MED:
			return ulAntiHammeringDelayMed ;

		case CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH:
			return ulAntiHammeringDelayHigh ;

		default :
		return 0;
	}
}


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
//void ChangeEndianess(unsigned char *pInputBuffer, unsigned int InputBufferLen, unsigned char *pOutputBuffer)
//{
//	int nBytePos = 0;
//	int nElement = 0;
//	int nElementLen = 0;
//	int nRelativePos = 0;
//	unsigned int pElementDetails[20] = {0};
//
//
//	pElementDetails[0] = InputBufferLen / 2;
//	pElementDetails[1] = InputBufferLen / 2;
//
//
//	for(; pElementDetails[nElement] != 0; nElement++)
//	{
//		nElementLen += pElementDetails[nElement];
//
//		for(; nBytePos < nElementLen; nBytePos++)
//		{
//			pOutputBuffer[nBytePos] = pInputBuffer[nElementLen - nRelativePos - 1];
//			nRelativePos++;
//		}
//		nRelativePos = 0;
//	}
//
//}

//BufferType - Signature data or Key Blob data

void ChangeEndianess(unsigned char *pInputBuffer, unsigned int InputBufferLen, 
					 DWORD BufferType, unsigned char *pOutputBuffer)
{
	int nBytePos = 0;
	int nElement = 0;
	int nElementLen = 0;
	int nRelativePos = 0;
	unsigned int pElementDetails[20] = {0};
	unsigned int keyLen = 1024 ;

	switch(BufferType)
	{
	case SIGNATURE_DATA:
		{
			pElementDetails[0] = InputBufferLen / 2;
			pElementDetails[1] = InputBufferLen / 2;
		}
		break;
	case KEYBLOB_DATA:
		{
			pElementDetails[0] = keyLen /8;
			pElementDetails[1] = 20;
			pElementDetails[2] = keyLen /8;
			pElementDetails[3] = keyLen /8;
			pElementDetails[4] = 4;
			pElementDetails[5] = 20;

		}
		break;
	default:
		return;
	}

	for(; pElementDetails[nElement] != 0; nElement++)
	{
		nElementLen += pElementDetails[nElement];

		for(; nBytePos < nElementLen; nBytePos++)
		{
			pOutputBuffer[nBytePos] = pInputBuffer[nElementLen - nRelativePos - 1];
			nRelativePos++;
		}
		nRelativePos = 0;
	}
}

uint32_t
cvGetDeltaTime(uint32_t startTime)
{
	return 0;
}

