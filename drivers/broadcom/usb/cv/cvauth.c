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
 * cvauth.c:  Credential Vault auth handler
 */
/*
 * Revision History:
 *
 * 01/24/07 DPA Created.
*/
#include "cvmain.h"
#include "console.h"
#include "time.h"
#include "../dmu/crmu_apis.h"

extern const uint8_t ecc_default_prime[ECC256_KEY_LEN];
extern const uint8_t ecc_default_a[ECC256_KEY_LEN];
extern const uint8_t ecc_default_b[ECC256_KEY_LEN];
extern const uint8_t ecc_default_G[ECC256_POINT];
extern const uint8_t ecc_default_n[ECC256_KEY_LEN];
extern const uint8_t ecc_default_h[ECC256_KEY_LEN];
extern const uint8_t dsa_default_p[DSA1024_KEY_LEN];
extern const uint8_t dsa_default_q[DSA1024_PRIV_LEN];
extern const uint8_t dsa_default_g[DSA1024_KEY_LEN];

static const uint64_t DIGITS_POWER_FOR_HOTP[11] = {1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000,10000000000};

void
cvFindCountAuthType(uint32_t authListsLength, cv_auth_list *pAuthLists, cv_obj_value_auth_session *objValue)
{
	/* this routine checks to see if object auth lists contains count type auth entry.  if so, count is checked. */
	/* if count is 0,  auth corresponding to that auth list is deauthorized, if not, count is decremented */
	uint32_t i;
	cv_auth_list *authListPtr;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry;
	cv_auth_entry_count *count;

	authListPtr = pAuthLists;
	authListsEnd = (uint8_t *)authListPtr + authListsLength;
	do
	{
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			if  (authEntry->authType == CV_AUTH_COUNT)
			{
				count = (cv_auth_entry_count *)(authEntry + 1);
				if (count->count == 0)
					/* deauthorize authorizations associated with this auth list */
					objValue->statusLSH &= ~authListPtr->flags;
				else
					/* count hasn't elapsed yet, decrement it and return status indicating object must be updated */
					count->count--;
			}
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntry > authListsEnd)
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				break;
		}
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);
}

cv_status
cvAuthPassphrase(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, 
				 cv_bool determineAvail)
{
	/* this routine compares the passphrase in the input parameter list with that in the object */
	cv_status retVal = CV_SUCCESS;
	cv_auth_entry_passphrase *passphraseEntryParam, *passphraseEntryObj;
	cv_auth_entry_passphrase_handle *passphraseEntryObjHandle;
	cv_obj_hdr *objPtr;
	uint8_t *passphrasePtrObj;
	cv_obj_value_passphrase *passphraseValueObj; 
	cv_obj_properties objPropertiesNew;
	uint32_t objPassphraseLength;

	passphraseEntryParam = (cv_auth_entry_passphrase *)(authEntryParam + 1);
	passphraseEntryObj = (cv_auth_entry_passphrase *)(authEntryObj + 1);
	/* first check to see if object entry is passphrase or handle to object containing passphrase */
	if (passphraseEntryObj->passphraseLen == 0)
	{
		/* length of 0 indicates that this entry should contain object handle */
		memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
		passphraseEntryObjHandle = (cv_auth_entry_passphrase_handle *)passphraseEntryObj;
		objPropertiesNew.objHandle = passphraseEntryObjHandle->hPassphrase;
		if (determineAvail)
			return cvDetermineObjAvail(&objPropertiesNew);
		if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		passphraseValueObj = (cv_obj_value_passphrase *)cvFindObjValue(objPtr);
		passphrasePtrObj = passphraseValueObj->passphrase;
		objPassphraseLength = passphraseValueObj->passphraseLen;
	} else {
		if (determineAvail)
			/* just checking for availability, since no objects in auth, just return */
			return CV_SUCCESS;
		passphrasePtrObj = passphraseEntryObj->passphrase;
		objPassphraseLength = passphraseEntryObj->passphraseLen;
	}

	/* validate length */
	if (passphraseEntryParam->passphraseLen > authEntryParam->authLen)
		return CV_INVALID_AUTH_LIST;
	/* compare passphrases */
	if (passphraseEntryParam->passphraseLen != objPassphraseLength)
		retVal = CV_AUTH_FAIL;
	else if (memcmp(passphraseEntryParam->passphrase, passphrasePtrObj, passphraseEntryParam->passphraseLen))
		retVal = CV_AUTH_FAIL;

err_exit:
	return retVal;
}

cv_status
cvComputeParamHMAC(cv_input_parameters *inputParameters, uint8_t *hmacKey, uint32_t hmacLen, uint32_t sessionCount, uint8_t *hmac)
{
	/* this routine computes an HMAC of the input parameters and session count */
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE]; 
	uint32_t i;
	uint8_t *bufPtr = &buf[0], *bufStart;
	cv_status retVal = CV_SUCCESS;
	uint8_t intermedHash[SHA1_LEN];

	bufStart = bufPtr;
	for (i=0;i<inputParameters->numParams;i++)
	{
		memcpy(bufPtr, inputParameters->paramLenPtr[i].param, inputParameters->paramLenPtr[i].paramLen);
		bufPtr += inputParameters->paramLenPtr[i].paramLen;
	}
	memcpy(bufPtr, (uint8_t *)&sessionCount, sizeof(uint32_t ));
	bufPtr += sizeof(uint32_t);
	/* first do the hash of the parameters || session count, then do HMAC of hash */
	if ((retVal = cvAuth(bufStart, bufPtr - bufStart, NULL, intermedHash, hmacLen, NULL, 0, CV_SHA)) != CV_SUCCESS)
		goto err_exit;
	if ((retVal = cvAuth(intermedHash, SHA1_LEN, hmacKey, hmac, hmacLen, NULL, 0, CV_SHA)) != CV_SUCCESS)
		goto err_exit;

err_exit:
	return retVal;
}


cv_status
cvAuthPassphraseHMAC(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, 
					 cv_input_parameters *inputParameters, cv_bool determineAvail)
{
	/* this routine computes an HMAC of the input parameters along with the session handle and count */
	/* if determineAvail is TRUE, it just needs to chack to see if any objects associated with the */
	/* auth are available and set return status appropriately */
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_hash *hmac;
	uint32_t hmacLen;
	uint8_t hmacKey[SHA256_LEN], hmacSaved[SHA256_LEN], hmacComputed[SHA256_LEN];
	cv_auth_entry_passphrase_HMAC_obj *passphraseHMAC;
	uint8_t *tmp_hmac;

	/*HMAC from object auth list */
	passphraseHMAC = (cv_auth_entry_passphrase_HMAC_obj *)(authEntryObj + 1);
	if (determineAvail)
		/* just checking for availability, since no objects in auth, just return */
		return CV_SUCCESS;

	/* now get hmac from parameter list auth that will be used for comparison */
	hmac = (cv_obj_value_hash *)(authEntryParam + 1);
	tmp_hmac = (uint8_t*)hmac + offsetof(cv_obj_value_hash,hash);
	switch (hmac->hashType)
	{
	case CV_SHA1:
		hmacLen = SHA1_LEN;
		break;
	case CV_SHA256:
		hmacLen = SHA256_LEN;
		break;
	case 0xffff:
	default:
		/* if SHA384 implemented, size of hmac arrays must be increased */
		retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
		goto err_exit;
	}

	/* check to make sure SHA matches parameter list */
	if (hmac->hashType != passphraseHMAC->hash.hashType)
	{
		retVal = CV_INVALID_HMAC_TYPE;
		goto err_exit;
	}
	memcpy(hmacKey, passphraseHMAC->hash.hash, hmacLen);

	/* now do HMAC of parameters.  first save and zero HMAC location */
	memcpy(hmacSaved, tmp_hmac, hmacLen);
	memset(tmp_hmac,0,hmacLen);

	if ((retVal = cvComputeParamHMAC(inputParameters, hmacKey, hmacLen, (uint32_t)objProperties->session->secureSessionUsageCount, hmacComputed)) != CV_SUCCESS)
		goto err_exit;

	/* compare computed HMAC to one in parameter list */
	if (memcmp(hmacComputed, hmacSaved, hmacLen))
		retVal = CV_AUTH_FAIL;

err_exit:
	return retVal;
}

cv_status
cvAuthFingerprint(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, 
				  cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail)
{
	/* this routine uses all the template objects supplied and calls the FP support routine to determine if a match is made. */
	cv_status retVal = CV_SUCCESS;
	cv_auth_entry_fingerprint_obj *fpEntry;
	cv_fp_control fpControl;
	int32_t FARvalParam; 
	cv_handle cvHandle = (cv_handle)objProperties->session;
	cv_obj_handle pResultTemplateHandle; 
	uint32_t *paramPtr;
	cv_bool matchFound;
	uint32_t featureSetLength = 0;
	uint8_t *pFeatureSet = NULL;
	cv_obj_hdr *objPtr;
	cv_obj_properties objPropertiesNew;
	cv_obj_handle featureSetHandle;
	cv_obj_value_fingerprint *featureSetObj = NULL;
	uint32_t *pAuthEntryParamEnd;
	cv_nonce *captureID;
	cv_bool isIdentifyPurpose = TRUE;
	cv_auth_fp_with_session_id *pAuthFPWithSessionID = NULL;
	int i = 0;
	cv_bool didFindMatchingTemplateID = FALSE;
	unsigned int nSecondsPassed = 0;

	/* For now, template storage on devices which require prompting is not supported, do determineAvail always returns CV_SUCCESS */
	if (determineAvail)
		return CV_SUCCESS;

	pAuthEntryParamEnd = (uint32_t *)((uint8_t *)authEntryParam + authEntryParam->authLen + sizeof(cv_auth_hdr));
	/* first, determine where to get the FAR value */
	fpEntry = (cv_auth_entry_fingerprint_obj *)(authEntryObj + 1);
	paramPtr = (uint32_t *)(authEntryParam + 1);
	fpControl = *((cv_fp_control *)paramPtr);
	paramPtr++;
	/* validate fp control */
	if (fpControl & ~CV_FP_CONTROL_MASK)
		return CV_INVALID_AUTH_LIST;

	if (fpControl & CV_FINGER_WAS_ALREADY_PLACED_USE_SESSION_ID)
	{
		printf("cvAuthFingerprint matching based on session ID\n");
		
		pAuthFPWithSessionID = (cv_auth_fp_with_session_id *)paramPtr;
		paramPtr++;
		if (paramPtr > pAuthEntryParamEnd)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}

		if (CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess == 0)
		{
			printf("cvAuthFingerprint Timeout for object access is not set (did not call control unit API)\n");
			return CV_AUTH_FAIL;
		}

		if (CV_VOLATILE_DATA->wbfProtectData.didStartCRMUTimer == 0)
		{
			printf("cvAuthFingerprint CRMU Timer did not start (did not call control unit API).\n");
			return CV_AUTH_FAIL;
		}

		/* check that the following match -
		   session ID passed for verification - pAuthFPWithSessionID->sessionID
		   session ID in volatile memory - CV_VOLATILE_DATA->wbfProtectData.sessionID
		*/
		for (i = 0; i < WBF_CONTROL_UNIT_SESSION_ID_SIZE; ++i)
		{
			if (pAuthFPWithSessionID->sessionID[i] != CV_VOLATILE_DATA->wbfProtectData.sessionID[i])
			{
				printf ("cvAuthFingerprint Session IDs don't match\n");
				return CV_AUTH_FAIL;
			}
		}

		nSecondsPassed = cv_get_timer2_elapsed_time(CV_VOLATILE_DATA->wbfProtectData.CRMUElapsedTime);
		if (nSecondsPassed > CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess)
		{
			printf("cvAuthFingerprint Too much time passed (%d more than %d) since the call to the control unit api.\n", nSecondsPassed, CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess);
			return CV_AUTH_FAIL;
		}
		
		printf("template id from volatile memory: %d\n", CV_VOLATILE_DATA->wbfProtectData.template_id_of_identified_fingerprint);
		
		for (i=0; i < fpEntry->numTemplates; ++i)
		{
			
			printf("Template that this object is protected with: %d\n", fpEntry->templates[i]);
			if (fpEntry->templates[i] == CV_VOLATILE_DATA->wbfProtectData.template_id_of_identified_fingerprint)
			{
				didFindMatchingTemplateID = TRUE;
			}
		}
		
		if (!didFindMatchingTemplateID)
		{
			printf ("cvAuthFingerprint Didn't find a matching template ID\n");
			return CV_AUTH_FAIL;		
		}

		return CV_SUCCESS;
	}

	if (fpControl & CV_HINT_AVAILABLE)
		/* hint template not used for now, since all templates sent to host anyway */
		paramPtr++;
	if (paramPtr > pAuthEntryParamEnd)
	{
		retVal = CV_INVALID_AUTH_LIST;
		goto err_exit;
	}
	/* check to see if parameter value should be used */
	if (fpControl & CV_USE_FAR)
	{
		/* check to make sure CV admin allows FAR to be supplied as param */
		if (CV_PERSISTENT_DATA->cvPolicy & CV_DISALLOW_FAR_PARAMETER)
		{
			retVal = CV_FAR_PARAMETER_DISALLOWED;
			goto err_exit;
		}
		FARvalParam = *((int32_t *)paramPtr);
		paramPtr++;
		if (paramPtr > pAuthEntryParamEnd)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
	} else {
		/* no FAR param, must use configured value */
		FARvalParam = CV_PERSISTENT_DATA->FARval;
	}
	/* check to see if a feature set was included */
	if (fpControl & CV_USE_FEATURE_SET)
	{
		featureSetLength = *((uint32_t *)paramPtr);
		paramPtr++;
		pFeatureSet = (uint8_t *)paramPtr;
		paramPtr = (uint32_t *)((uint8_t *)paramPtr + featureSetLength);
		if (paramPtr > pAuthEntryParamEnd)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
	} else if (fpControl & CV_USE_FEATURE_SET_HANDLE) {
		featureSetHandle = *((cv_obj_handle *)paramPtr);
		paramPtr++;
		if (paramPtr > pAuthEntryParamEnd)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
		/* get feature set from object pointed to by handle */
		memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
		objPropertiesNew.objHandle = featureSetHandle;
		if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		featureSetObj = (cv_obj_value_fingerprint *)cvFindObjValue(objPtr);
		/* check to make sure this is a feature set */
		if (featureSetObj->type != getExpectedFingerprintFeatureSetType())
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		featureSetLength = featureSetObj->fpLen;
		pFeatureSet = &featureSetObj->fp[0];
	} else {
		/* no feature set provided.  checn to see if FP is present and fail if not */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT)) {
			retVal = CV_FP_NOT_PRESENT;
			goto err_exit;
		}
	}

	/* now, do verify */
	/* check to see if any persisted FP should be invalidated */
	if (fpControl & CV_INVALIDATE_CAPTURE)
		CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

	/* check to see if the persisted FP was captured by async FP capture.  if so, capture ID is required before */
	/* being used here.  ignore if auth contains feature set. */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_HAS_CAPTURE_ID) && !(fpControl & (CV_USE_FEATURE_SET | CV_USE_FEATURE_SET_HANDLE)))
	{
		/* check to see if capture ID included in auth list */
		if (!(fpControl & CV_CONTAINS_CAPTURE_ID))
		{
			/* it's ok to not have a capture ID if this is the result of an internal async FP capture */
			if (!(CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL))
			{
				retVal = CV_INVALID_CAPTURE_ID;
				goto err_exit;
			}
		} else {
			/* check capture ID */
			captureID = ((cv_nonce *)paramPtr);
			paramPtr += (sizeof(cv_nonce)/sizeof(uint32_t));
			if (memcmp(captureID,&CV_VOLATILE_DATA->captureID, sizeof(cv_nonce))) 
			{
				retVal = CV_INVALID_CAPTURE_ID;
				goto err_exit;
			}
		}
	}
	if (paramPtr != pAuthEntryParamEnd)
	{
		retVal = CV_INVALID_AUTH_LIST;
		goto err_exit;
	}


	/* here, cvFROnChipIdentify is used, because it's really just a special case of identify */
	if (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_PURPOSE_VERIFY)
		isIdentifyPurpose = FALSE;

	if ((retVal = cvFROnChipIdentify(cvHandle, FARvalParam, fpControl, isIdentifyPurpose,
				featureSetLength, pFeatureSet,
				fpEntry->numTemplates,
				(cv_obj_handle *)&fpEntry->templates[0],
				0, NULL,
				&pResultTemplateHandle, FALSE, &matchFound,
				callback, context)) != CV_SUCCESS)
	{
		goto err_exit;
	}

	if (!matchFound)
		retVal = CV_AUTH_FAIL;

err_exit:
	if (featureSetObj != NULL)
		/* bump handle object to LRU so same buffer gets reused.  Cache only has entries for 4 objects */
		cvUpdateObjCacheLRUFromHandle(objPropertiesNew.objHandle);
	return retVal;
}

cv_status
cvCalculateHOTPValue(uint8_t* randomKey, uint32_t randomKeyLength, uint8_t numberOfDigits, uint32_t counterValue, uint64_t* hotpValue)
{
	uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint8_t	lRandomKey[HOTP_KEY_MAX_LEN]={0};
	uint8_t hmacSha1[SHA1_LEN+4]={0};
	uint8_t counterText[8]={0};
	int32_t i=0;
	int32_t offset=0;
	int32_t binary=0;

	if (numberOfDigits > 10)
	{
		printf("Incorrect digits number %d", numberOfDigits);
		return CV_INVALID_KEY_PARAMETERS;
	}
	if (randomKeyLength > HOTP_KEY_MAX_LEN)
	{
		printf("Incorrect random key length %d %d", randomKeyLength, HOTP_KEY_MAX_LEN);
		return CV_INVALID_KEY_PARAMETERS;
	}
	memcpy(lRandomKey, randomKey, randomKeyLength);


	phx_crypto_init(cryptoHandle);

	printf( "CalculateHOTPValue: Counter value is: %d\n", counterValue);

	for (i=sizeof(counterText)-1;i>=0;--i)
	{
		counterText[i]=counterValue & 0xFF;
		counterValue >>=8;
	}

	if ((phx_get_hmac_sha1_hash(cryptoHandle, counterText, sizeof(counterText),
					lRandomKey, randomKeyLength, hmacSha1, TRUE)) != PHX_STATUS_OK)
	{
		printf("phx_get_hmac_sha1_hash error");
		return CV_CRYPTO_FAILURE;
	}

/*	printf( "CalculateHOTPValue: HMAC value is:");
	for (i=0;i<SHA1_LEN;++i)
	{
		printf(" %02x",hmacSha1[i]);
	}
	printf("\n");
*/

	offset = hmacSha1[SHA1_LEN-1]&0xf;
	binary = ((hmacSha1[offset] & 0x7f) << 24)
			 | ((hmacSha1[offset + 1] & 0xff) << 16)
			 | ((hmacSha1[offset + 2] & 0xff) << 8)
			 | (hmacSha1[offset + 3] & 0xff);

	*hotpValue = binary % DIGITS_POWER_FOR_HOTP[numberOfDigits];

//	printf("CalculateHOTPValue: Binary = %d  HOTP value = %llu\n", binary, *hotpValue);

	return CV_SUCCESS;
}

cv_status
cvAuthHOTP(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, 
				  cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail)
{
	cv_status retVal = CV_AUTH_FAIL;
	cv_auth_entry_hotp *hotpEntry = NULL;
	cv_auth_hotp_command *hotpCommand = NULL;
	cv_obj_hdr *objPtr = NULL;
	cv_obj_properties objPropertiesNew;
	cv_obj_handle hotpObject;
	cv_obj_value_hotp *hotpValueObj = NULL;
	uint32_t *pAuthEntryParamEnd = NULL;
	uint32_t *paramPtr = NULL;
	uint64_t calculatedHotpValue = 0;


	if (determineAvail)
		return CV_SUCCESS;

	printf("cvAuthHOTP\n");

	hotpEntry = (cv_auth_entry_hotp *)(authEntryObj + 1);

	paramPtr = (uint32_t *)(authEntryParam + 1);
	hotpCommand = (cv_auth_hotp_command *)paramPtr;
	paramPtr++;

	pAuthEntryParamEnd = (uint32_t *)((uint8_t *)authEntryParam + authEntryParam->authLen + sizeof(cv_auth_hdr));
	if (paramPtr > pAuthEntryParamEnd)
	{
		return CV_INVALID_AUTH_LIST;
	}

//	printf("hotpCommand->OTPValue: %llu\n", hotpCommand->OTPValue);

	// Get the HOTP object

	hotpObject = hotpEntry->hHOTPObj;
	memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
	objPropertiesNew.objHandle = hotpObject;
	if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
	{
		printf("cvGetObj error: %d", retVal);
		return CV_AUTH_FAIL;
	}
	hotpValueObj = (cv_obj_value_hotp *)cvFindObjValue(objPtr);
/*	printf("hotpValueObj->version: %d\n", hotpValueObj->version);
	printf("hotpValueObj->currentCounterValue: %d\n", hotpValueObj->currentCounterValue);
	printf("hotpValueObj->counterInterval: %d\n", hotpValueObj->counterInterval);
	printf("hotpValueObj->randomKeyLength: %d\n", hotpValueObj->randomKeyLength);
	printf("hotpValueObj->allowedMaxWindow: %d\n", hotpValueObj->allowedMaxWindow);
	printf("hotpValueObj->maxTries: %d\n", hotpValueObj->maxTries);
	printf("hotpValueObj->allowedLargeMaxWindow: %d\n", hotpValueObj->allowedLargeMaxWindow);
	printf("hotpValueObj->numberOfDigits: %d\n", hotpValueObj->numberOfDigits);
	printf("hotpValueObj->isBlocked: %d\n", hotpValueObj->isBlocked);
	printf("hotpValueObj->currentNumberOfTries: %d\n", hotpValueObj->currentNumberOfTries);
*/
	if (hotpValueObj->isBlocked)
	{
		printf("HOTP is blocked. Failing authentication.\n");
		return CV_AUTH_FAIL;
	}

	uint32_t counterValue = hotpValueObj->currentCounterValue;

	if (hotpValueObj->secondSubsequentOTPLocationInWindow)
	{
		// Handling of a special case - Trying to match a second subsequent OTP value inside the large window
		printf("Second OTP required now\n");

		counterValue = hotpValueObj->secondSubsequentOTPLocationInWindow;

		if ((retVal = cvCalculateHOTPValue(hotpValueObj->randomKey, hotpValueObj->randomKeyLength, hotpValueObj->numberOfDigits, counterValue, &calculatedHotpValue)) != CV_SUCCESS)
		{
			printf("cvCalculateHOTPValue failed with %d", retVal);
			hotpValueObj->secondSubsequentOTPLocationInWindow = 0;
			return CV_AUTH_FAIL;
		}
		if (hotpCommand->OTPValue == calculatedHotpValue)
		{
			printf("Correct second OTP value\n");
			counterValue += hotpValueObj->counterInterval;

			hotpValueObj->currentCounterValue = counterValue; // sync counter value
			hotpValueObj->currentNumberOfTries = 0;
			hotpValueObj->secondSubsequentOTPLocationInWindow = 0;
			return CV_SUCCESS;
		}

		hotpValueObj->currentNumberOfTries++;
		printf("Incorrect second OTP value. Increasing current number of tries to %d\n", hotpValueObj->currentNumberOfTries);

		// Check if should block
		if (hotpValueObj->currentNumberOfTries >= hotpValueObj->maxTries)
		{
			printf("Max tries %d has been reached. Blocking\n", hotpValueObj->maxTries);
			hotpValueObj->isBlocked = 1;
		}

		hotpValueObj->secondSubsequentOTPLocationInWindow = 0;
		return CV_AUTH_FAIL;
	}

	// Calculate HOTP values in a loop within allowedMaxWindow
	uint32_t delta  = 0;
	while (delta < hotpValueObj->allowedLargeMaxWindow)
	{
		if ((retVal = cvCalculateHOTPValue(hotpValueObj->randomKey, hotpValueObj->randomKeyLength, hotpValueObj->numberOfDigits, counterValue, &calculatedHotpValue)) != CV_SUCCESS)
		{
			printf("cvCalculateHOTPValue failed with %d", retVal);
			return CV_AUTH_FAIL;
		}

		++delta;
		counterValue += hotpValueObj->counterInterval;

		// Check if found correct OTP value
		if (hotpCommand->OTPValue == calculatedHotpValue)
		{
			if (delta < hotpValueObj->allowedMaxWindow)
			{
				printf("Correct OTP value\n");
				hotpValueObj->currentCounterValue = counterValue; // sync counter value
				hotpValueObj->currentNumberOfTries = 0;
				return CV_SUCCESS;
			}
			else
			{
				printf("Correct OTP value but within large window\n");
				hotpValueObj->secondSubsequentOTPLocationInWindow = counterValue;
				return CV_HOTP_SECOND_OTP_REQUIRED;
			}
		}
		printf("Incorrect OTP value\n");
	}

	hotpValueObj->currentNumberOfTries++;
	printf("Large max window reached. Increasing current number of tries to %d\n", hotpValueObj->currentNumberOfTries);

	// Check if should block
	if (hotpValueObj->currentNumberOfTries >= hotpValueObj->maxTries)
	{
		printf("Max tries %d has been reached. Blocking\n", hotpValueObj->maxTries);
		hotpValueObj->isBlocked = 1;
	}

	return CV_AUTH_FAIL;
}

cv_status
cvAuthSession(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_input_parameters *inputParameters, 
			  cv_obj_auth_flags *authFlags)
{
	/* this routine checks to see if the indicated auth session object has been authorized */
	cv_status retVal = CV_SUCCESS;
	cv_auth_data *authSessionObj;
	cv_auth_entry_auth_session_HMAC *authSessionParam;
	cv_obj_hdr *objPtr;
	cv_obj_value_auth_session *authSessionObjPtr;
	uint32_t index, dir0Index;
	cv_obj_value_hash *hmac;
	uint32_t hmacLen, hmacKeyLen;
	uint8_t hmacSaved[SHA256_LEN], hmacComputed[SHA256_LEN];
	cv_obj_value_hash *hmacKey;
	uint32_t *sessionCount;
	cv_obj_properties objProperties;
	uint8_t *tmp_hmac;

	authSessionParam = (cv_auth_entry_auth_session_HMAC *)(authEntryParam + 1);
	authSessionObj = (cv_auth_data *)(authEntryObj + 1);
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = authSessionParam->hAuthSessionObj;
	/* first get pointer to auth session object */
	if (!cvFindVolatileDirEntry(authSessionParam->hAuthSessionObj, &index))
	{
		/* none found, just fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[index].pObj;
	authSessionObjPtr = (cv_obj_value_auth_session *)cvFindObjValue(objPtr);
	/* check to see if auth session terminated */
	if (authSessionObjPtr->statusMSH & CV_AUTH_SESSION_TERMINATED)
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* now check to see if auth value in obj matches that in parameter list */
	if (memcmp(authSessionObj->authData, authSessionObjPtr->authData.authData, CV_NONCE_LEN))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* now check to see if this is authSessionHMAC.  if so, need to compute HMAC of parameters */
	/* using shared secret in auth session object */
	if (inputParameters != NULL)
	{
		hmac = &authSessionParam->hmac;
		tmp_hmac = (uint8_t*)hmac + offsetof(cv_obj_value_hash,hash);
		switch (hmac->hashType)
		{
		case CV_SHA1:
			hmacLen = SHA1_LEN;
			break;
		case CV_SHA256:
			hmacLen = SHA256_LEN;
			break;
		case 0xffff:
		default:
			/* if SHA384 implemented, size of hmac arrays must be increased */
			retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
			goto err_exit;
		}

		/* now do HMAC of parameters.  first save and zero HMAC location */
		memcpy(hmacSaved, tmp_hmac, hmacLen);
		memset(tmp_hmac,0,hmacLen);
		hmacKey = (cv_obj_value_hash *)(authSessionObjPtr + 1);
		switch (hmacKey->hashType)
		{
		case CV_SHA1:
			hmacKeyLen = SHA1_LEN;
			break;
		case CV_SHA256:
			hmacKeyLen = SHA256_LEN;
			break;
		case 0xffff:
		default:
			/* if SHA384 implemented, size of hmac arrays must be increased */
			retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
			goto err_exit;
		}
		if (hmacKeyLen != hmacLen)
		{
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		sessionCount = (uint32_t *)((uint8_t *)(hmacKey + 1) - sizeof(uint8_t) + hmacKeyLen);
		if ((retVal = cvComputeParamHMAC(inputParameters, hmacKey->hash, hmacLen, *sessionCount, hmacComputed)) != CV_SUCCESS)
			goto err_exit;

		/* compare computed HMAC to one in parameter list */
		if (memcmp(hmacComputed, hmacSaved, hmacLen))
		{
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		/* if got here, was successful.  update session object count */
		(*sessionCount)++;
	}
	/* indicate which operations have been authorized */
	*authFlags = authSessionObjPtr->statusLSH;
	/* now terminate session if single use and clear auth values if authorization required for each use */
	if (authSessionObjPtr->flags & CV_AUTH_SESSION_SINGLE_USE)
		authSessionObjPtr->statusMSH |= CV_AUTH_SESSION_TERMINATED;
	if (authSessionObjPtr->flags & CV_AUTH_SESSION_PER_USAGE_AUTH)
		authSessionObjPtr->statusLSH = 0;
	/* now search to see if auth session object contains count type auth.  if so, check to see if */
	/* zero.  if so, terminate auth session object.  if not, decrement count */
	memset(&objProperties, 0, sizeof(objProperties));
	cvFindObjPtrs(&objProperties, objPtr);
	cvFindCountAuthType(objProperties.authListsLength, objProperties.pAuthLists, authSessionObjPtr);
	/* check to see if session terminated or no authorizations left.  if so, can delete volatile object if  */
	/* there is a corresponding flash object because new volatile object will be created if authorize_session_object */
	/* called again */
	if ((authSessionObjPtr->statusMSH & CV_AUTH_SESSION_TERMINATED) || (authSessionObjPtr->statusLSH == 0))
	{
		/* terminated, check to see if there is a corresponding flash object */
		if (cvFindDir0Entry(authSessionParam->hAuthSessionObj, &dir0Index))
		{
			/* yes, remove volatile object */
			cv_free(objPtr, objPtr->objLen);
			CV_VOLATILE_OBJ_DIR->volObjs[index].hObj = 0;
		}
	}
err_exit:
	return retVal;
}

cv_status
cvAuthCount(cv_auth_hdr *authEntryObj)
{
	/* this routine tests whether the usage count associated with an object has expired */
	cv_auth_entry_count *count;

	count = (cv_auth_entry_count *)(authEntryObj + 1);
	if (count->count == 0)
		return CV_AUTH_FAIL;
	else
	{
		/* count hasn't elapsed yet, decrement it and return status indicating object must be updated */
		count->count--;
		return CV_OBJECT_UPDATE_REQUIRED;
	}
}

cv_status
cvAuthTime(cv_auth_hdr *authEntryObj)
{
	/* this routine tests whether the current time falls within the interval specified */
	cv_auth_entry_time *authTime;
	cv_time curTime;
	cv_status retVal = CV_SUCCESS;

	authTime = (cv_auth_entry_time *)(authEntryObj + 1);
	if ((retVal = get_time(&curTime)) != CV_SUCCESS)
		goto err_exit;
	if ((memcmp(&authTime->startTime, &curTime, sizeof(cv_time)) <= 0) 
		&& (memcmp(&curTime, &authTime->endTime, sizeof(cv_time)) <= 0))
		return TRUE;
	else
		return FALSE;

err_exit:
	return retVal;
}

cv_status
cvAuthHID(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine compares the contactless credential in the CV object with the one read in */
	/* from contactless card.  Note that the credential from the contactless card isn't a CV object and that */
	/* it should have been read in before this point */
	cv_auth_entry_contactless *HIDentryObj;
	cv_auth_entry_contactless_handle *HIDentryObjHandle;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_opaque *HIDCVPtr;
	cv_obj_hdr *objPtr;
	cv_obj_properties objPropertiesNew;
	uint16_t credLenObj;
	uint8_t *credPtrObj;

	HIDentryObj = (cv_auth_entry_contactless *)(authEntryObj + 1);
	/* first determine if credential or handle to credential stored in object */
	if (HIDentryObj->length == 0)
	{
		HIDentryObjHandle = (cv_auth_entry_contactless_handle *)(authEntryObj + 1);
		/* get credential from object pointed to by handle */
		memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
		objPropertiesNew.objHandle = HIDentryObjHandle->hHIDObj;
		if (determineAvail)
			return cvDetermineObjAvail(&objPropertiesNew);
		if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		HIDCVPtr = (cv_obj_value_opaque *)cvFindObjValue(objPtr);
		credLenObj = HIDCVPtr->valueLen;
		credPtrObj = (uint8_t *)&HIDCVPtr->value[0];
	} else {
		if (determineAvail)
			return CV_SUCCESS;
		/* get credential from object auth list */
		credLenObj = HIDentryObj->length;
		credPtrObj = (uint8_t *)&HIDentryObj->credential;
	}

	/* now compare credential from CV with one read in from HID card */
	if (CV_VOLATILE_DATA->HIDCredentialPtr != NULL)
	{
		/* first check to see if it's the right type of contactless smart card */
		if (CV_VOLATILE_DATA->credentialType != HIDentryObj->type)
		{
			retVal = CV_INVALID_CONTACTLESS_CARD_TYPE;
			CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
			goto err_exit;
		}
		/* now check credential */
		if (memcmp(credPtrObj, CV_VOLATILE_DATA->HIDCredentialPtr, credLenObj))
		{
			retVal = CV_AUTH_FAIL;
			CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
			goto err_exit;
		}
		/* invalidate HID credential pointer, since will need to get read in next time it's used */
		CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
	} else
		/* HID credential not resident.  fail, because should have been read in when determineAuthAvail called */
		retVal = CV_INVALID_AUTH_LIST;

err_exit:
	return retVal;
}

cv_status
cvAuthSmartCard(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine issues a challenge to a smart card and verifies the response */
	cv_auth_entry_smartcard *SCentryObj;
	cv_auth_entry_smartcard_handle *SCentryObjHandle;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_key *keyCVPtr;
	cv_obj_hdr *objPtr;
	cv_obj_properties objPropertiesNew;
	uint8_t challenge[CV_NONCE_LEN];
	uint8_t signature[RSA4096_KEY_LEN];  /* maximum size key supported */
	uint8_t keyLocal[RSA4096_KEY_LEN + sizeof(cv_obj_value_key) - 1];  /* maximum size key supported */
	uint32_t signatureLen;
	cvsc_status cvscStatus = CVSC_OK;
	cv_obj_type objType;
	cv_bool verify;
	cvsc_card_type cardType;
	uint32_t keyLenBytes;

	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT)) {
		retVal = CV_SC_NOT_PRESENT;
		goto err_exit;
	}
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
		retVal = CV_SC_NOT_ENABLED;
		goto err_exit;
	}
	/* test to see if smart card present */
	if (!scIsCardPresent())
		return CV_PROMPT_FOR_SMART_CARD;

	SCentryObj = (cv_auth_entry_smartcard *)(authEntryObj + 1);
	/* first determine if key or handle to key stored in object */
	if (SCentryObj->type == 0)
	{
		SCentryObjHandle = (cv_auth_entry_smartcard_handle *)(authEntryObj + 1);
		/* get key from object pointed to by handle */
		memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
		objPropertiesNew.objHandle = SCentryObjHandle->keyObj;
		if (determineAvail)
		{
			retVal = cvDetermineObjAvail(&objPropertiesNew);
			goto err_exit;
		}
		if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		keyCVPtr = (cv_obj_value_key *)cvFindObjValue(objPtr);
		objType = objPtr->objType;
	} else {
		/* get credential from object auth list.  copy key locally and convert to bigint */
		keyCVPtr = &SCentryObj->key;
		keyLenBytes = keyCVPtr->keyLen/8;
		if (SCentryObj->type == CV_RSA)
			objType = CV_TYPE_RSA_KEY;
		else /* SCentryObj->type == CV_ECC since this is validated when object created */
			objType = CV_TYPE_ECC_KEY;
		/* need to convert key to BIGINT format for SCAPI */
		cvCopyObjValue(objType, keyLenBytes + sizeof(cv_obj_value_key) - 1, keyLocal, keyCVPtr, FALSE, FALSE);
		keyCVPtr = (cv_obj_value_key *)&keyLocal[0];
	}

	/* make sure this is the right type of smart card */
	cvscStatus = scGetCardType(&cardType);
	if (cvscStatus == CVSC_OK) {
		if (cardType == CVSC_DELL_JAVA_CARD)
			return CV_INVALID_SMART_CARD_TYPE;
	} else
		return CV_SMART_CARD_FAILURE;

	/* now issue challenge to smart card and verify response */
	if ((retVal = cvRandom(CV_NONCE_LEN, (uint8_t *)&challenge[0])) != CV_SUCCESS)
		goto err_exit;
	signatureLen = RSA4096_KEY_LEN;
	cvscStatus = scChallenge(CV_NONCE_LEN, (uint8_t *)&challenge[0], &signatureLen, (uint8_t *)&signature[0], 
		objProperties->PINLen, objProperties->PIN);
	switch (cvscStatus)
	{
	case CVSC_OK:
		break;
	case CVSC_PIN_REQUIRED:
		retVal = CV_PROMPT_PIN;
		goto err_exit;
	default:
		retVal = CV_SMART_CARD_FAILURE;
		goto err_exit;
	}
	if ((retVal = cvVerify(objType, (uint8_t *)challenge[0], CV_NONCE_LEN, keyCVPtr, 
			  signature, signatureLen, &verify)) != CV_SUCCESS)
		  goto err_exit;

	if (!verify)
		retVal = CV_AUTH_FAIL;

err_exit:
	return retVal;
}

cv_status
cvAuthProprietarySC(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine reads the private key from a proprietary smart card and compares with value in object */
	cv_auth_entry_proprietary_sc *SCentryObj;
	cv_status retVal = CV_SUCCESS;
	cvsc_status cvscStatus = CVSC_OK;
	cvsc_card_type cardType;
	uint8_t msg[CV_MAX_PROPRIETARY_SC_KEYLEN + CV_MAX_USER_ID_LEN];
	uint32_t keyLen = CV_MAX_PROPRIETARY_SC_KEYLEN;
	uint32_t userIdLen = CV_MAX_USER_ID_LEN;
	uint8_t hash[SHA1_LEN];

	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT)) {
		retVal = CV_SC_NOT_PRESENT;
		goto err_exit;
	}
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
		retVal = CV_SC_NOT_ENABLED;
		goto err_exit;
	}

	/* test to see if smart card present */
	if (determineAvail)
	{
		if (!scIsCardPresent())
			return CV_PROMPT_FOR_SMART_CARD;
		else
			return CV_SUCCESS;
	}

	SCentryObj = (cv_auth_entry_proprietary_sc *)(authEntryObj + 1);

	/* make sure this is the right type of smart card */
	cvscStatus = scGetCardType(&cardType);
	if (cvscStatus == CVSC_OK) {
		if (cardType != CVSC_DELL_JAVA_CARD)
			return CV_INVALID_SMART_CARD_TYPE;
	} else
		return CV_SMART_CARD_FAILURE;
	/* now read key and public data from proprietary smart card */
	cvscStatus = scGetUserKey(&keyLen, (uint8_t *)&msg[0], objProperties->PINLen, objProperties->PIN);
	switch (cvscStatus)
	{
	case CVSC_OK:
		break;
	case CVSC_PIN_REQUIRED:
		retVal = CV_PROMPT_PIN;
		goto err_exit;
	default:
		retVal = CV_SMART_CARD_FAILURE;
		goto err_exit;
	}
	if ((cvscStatus = scGetUserID(&userIdLen, (uint8_t *)&msg[keyLen])) != CVSC_OK)
	{
		retVal = CV_SMART_CARD_FAILURE;
		goto err_exit;
	}
	/* now hash (priv key || public data) */
	if ((retVal = cvAuth(msg, keyLen + userIdLen , NULL, (uint8_t *)&hash[0], SHA1_LEN, NULL, 0, 0)) != CV_SUCCESS)
		goto err_exit;
	/* compare with value in object */
	if (memcmp((uint8_t *)&hash[0], SCentryObj->hash.hash, SHA1_LEN))
		retVal = CV_AUTH_FAIL;

err_exit:
	return retVal;
}

cv_status
cvAuthChallenge(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine issues a challenge to a smart card and verifies the response */
	cv_auth_entry_challenge *challengeParam;
	cv_auth_entry_challenge_obj *challengeObj;
	cv_auth_entry_challenge_handle_obj *challengeObjHandle;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_key *keyCVPtr;
	cv_obj_hdr *objPtr;
	cv_obj_properties objPropertiesNew;
	cv_obj_type objType;
	cv_bool verify;

	challengeParam = (cv_auth_entry_challenge *)(authEntryParam + 1);
	challengeObj = (cv_auth_entry_challenge_obj *)(authEntryObj + 1);
	/* first determine if key or handle to key stored in object */
	if (challengeObj->type == 0)
	{
		challengeObjHandle = (cv_auth_entry_challenge_handle_obj *)(authEntryObj + 1);
		/* get key from object pointed to by handle */
		memcpy(&objPropertiesNew, objProperties, sizeof(cv_obj_properties));
		objPropertiesNew.objHandle = challengeObjHandle->keyObj;
		if (determineAvail)
		{
			retVal = cvDetermineObjAvail(&objPropertiesNew);
			goto err_exit;
		}
		if ((retVal = cvGetObj(&objPropertiesNew, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		keyCVPtr = (cv_obj_value_key *)cvFindObjValue(objPtr);
		objType = objPtr->objType;
	} else {
		/* get key from object auth list */
		keyCVPtr = (cv_obj_value_key *)&challengeObj;
		if (challengeObj->type == CV_RSA)
			objType = CV_TYPE_RSA_KEY;
		else /* SCentryObj->type == CV_ECC since this is validated when object created */
			objType = CV_TYPE_ECC_KEY;
	}

	/* now verify challenge */
	if ((retVal = cvVerify(objType, (uint8_t *)&challengeParam->nonce.nonce[0], CV_NONCE_LEN, keyCVPtr, 
			  (uint8_t *)&challengeParam->signature[0], keyCVPtr->keyLen/8, &verify)) != CV_SUCCESS)
		  goto err_exit;

	if (!verify)
		retVal = CV_AUTH_FAIL;

err_exit:
	return retVal;
}

cv_status
cvAuthSuper(cv_auth_hdr *authEntryParam)
{
	/* this routine determines if a response to a previously issued challenge is successful */
	cv_status retVal = CV_SUCCESS;
	uint32_t responseLength = authEntryParam->authLen;
	uint8_t *response = (uint8_t *)(authEntryParam + 1);

	/* make sure SUPER has been registered */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
		return CV_SUPER_NOT_REGISTERED;

	/* validate parameters */
	if (responseLength != AES_128_BLOCK_LEN)
		return CV_INVALID_INPUT_PARAMETER;

	if ((retVal = cvResponseSuper(response)) != CV_SUCCESS)
		retVal = CV_AUTH_FAIL;

	return retVal;
}

#ifdef ST_AUTH
cv_status stValidateResponse(uint32_t *table_rows, cv_st_challenge *cha, cv_auth_entry_st_response *resp)
{
    cv_st_hash_buff hb;
	uint8_t new_digest[SHA1_LEN];
	cv_status retVal = CV_SUCCESS;

    // concatenate crc and entropy.
    hb.crc32 = table_rows[cha->rowNumber - 1];
    memcpy(hb.nonce, cha->nonce, CV_NONCE_LEN);
 
    // generate digest based on concatenated crc32 and entropy
	if ((retVal = cvAuth((uint8_t *)&hb, sizeof(cv_st_hash_buff), NULL, (uint8_t *)&new_digest[0], SHA1_LEN, NULL, 0, 0)) != CV_SUCCESS)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}

    // compare both digests
    if( 0 == memcmp( new_digest, resp->respHash, SHA1_LEN))
    {
        return CV_SUCCESS;
    }
    return CV_AUTH_FAIL;
err_exit:
	return retVal;
}

cv_status
cvAuthSTChallenge(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj)
{
	cv_status retVal = CV_SUCCESS;
	cv_auth_entry_st_response *responseParam;
	cv_auth_entry_st_response_obj *responseObj;

	responseParam = (cv_auth_entry_st_response *)(authEntryParam + 1);
	responseObj = (cv_auth_entry_st_response_obj *)(authEntryObj + 1);
	retVal = stValidateResponse((uint32_t *)responseObj->crc32, &CV_VOLATILE_DATA->stChallenge, 
		(cv_auth_entry_st_response *)responseParam->respHash);

	return retVal;
}
#endif

cv_bool
isValidDate(uint32_t month, uint32_t day, uint32_t year)
{
	/* check to see if this is a valid date */
	if (month < 1 || month > 12)
		return FALSE;
	switch (month)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if (day < 1 || day > 31)
			return FALSE;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if (day < 1 || day > 30)
			return FALSE;
		break;
	case 2:
		if ((!(year%4) && (year%100)) || !(year%400))
		{
			if (day < 1 || day > 29)
				return FALSE;
		} else {
			if (day < 1 || day > 28)
				return FALSE;
		}
	}
	return TRUE;
}

cv_status
cvAuthCACPIV(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine does the authorization required for CAC and PIV cards */
	cv_auth_entry_cac_piv *cacPiv;
	cv_auth_entry_cac_piv_obj *cacPivObj;
	cv_status retVal = CV_SUCCESS;
	cvsc_status cvscStatus;
	cvsc_card_type cardType;
	uint8_t response[RSA2048_KEY_LEN + 2];		/* assume max key len is RSA 2048 (plus 2 for SC response bytes) */
	uint32_t responseLen = sizeof(response);
	uint8_t rand[SHA256_LEN];
	uint8_t challenge[RSA2048_KEY_LEN];		/* assume max key len is RSA 2048 */
	uint32_t challengeLen;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint32_t keyLenBytes;
	cv_bool verify;
	cv_obj_type objType;
	/* keyBuf is sized for ECC-256 which is > RSA2048 */
	uint8_t keyBuf[sizeof(cv_obj_value_key) - 1 + 9*ECC256_KEY_LEN] = {0x00, 0x00, ECC_DEFAULT_DOMAIN_PARAMS_PRIME,  \
		ECC_DEFAULT_DOMAIN_PARAMS_A,ECC_DEFAULT_DOMAIN_PARAMS_B,ECC_DEFAULT_DOMAIN_PARAMS_n,             \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  \
		ECC_DEFAULT_DOMAIN_PARAMS_G};
	cv_obj_value_key *key = (cv_obj_value_key *)&keyBuf[0];

	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT)) {
		retVal = CV_SC_NOT_PRESENT;
		goto err_exit;
	}
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
		retVal = CV_SC_NOT_ENABLED;
		goto err_exit;
	}

	/* test to see if smart card present */
	if (determineAvail)
	{
		if (!scIsCardPresent())
			return CV_PROMPT_FOR_SMART_CARD;
		else
			return CV_SUCCESS;
	}

	/* make sure PIN is available, else prompt */
	if (objProperties->PINLen == 0)
	{
		retVal = CV_PROMPT_PIN;
		goto err_exit;
	}

	cacPiv = (cv_auth_entry_cac_piv *)(authEntryParam + 1);
	cacPivObj = (cv_auth_entry_cac_piv_obj *)(authEntryObj + 1);

	/* make sure this is the right type of smart card */
	cvscStatus = scGetCardType(&cardType);
	if (cvscStatus == CVSC_OK) {
		if (cardType != CVSC_CAC_CARD && cardType != CVSC_PIV_CARD)
			return CV_INVALID_SMART_CARD_TYPE;
	} else
		return CV_SMART_CARD_FAILURE;

	/* compute random challenge and encode (only if RSA) */
	if ((retVal = cvRandom(sizeof(rand), &rand[0])) != CV_SUCCESS)
		goto err_exit;
	if (cacPivObj->cardSignAlg == CV_SIGN_ALG_RSA_1024 || cacPivObj->cardSignAlg == CV_SIGN_ALG_RSA_2048)
	{
		/* PKCS1 v1.5 encode */
		/* init SCAPI crypto lib */
		memset(&keyBuf[0],0,sizeof(keyBuf));
		phx_crypto_init(cryptoHandle);
		keyLenBytes = (cacPivObj->cardSignAlg == CV_SIGN_ALG_RSA_1024) ? RSA2048_KEY_LEN/2 : RSA2048_KEY_LEN;
		if ((cvPkcs1V15Encode(cryptoHandle, SHA256_LEN, rand, keyLenBytes, (uint8_t *)&challenge[0]) != PHX_STATUS_OK))
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		challengeLen = keyLenBytes;
		objType = CV_TYPE_RSA_KEY;
		cvCopySwap((uint32_t *)&cacPivObj->pubkey[0], (uint32_t *)&key->key[0], keyLenBytes);
		cvCopySwap((uint32_t *)&cacPivObj->pubkeyExp, (uint32_t *)&key->key[keyLenBytes], sizeof(uint32_t));
	} else {
		/* ECDSA */
		memcpy(&challenge[0], &rand[0], sizeof(rand));
		keyLenBytes = ECC256_KEY_LEN;
		challengeLen = sizeof(rand);
		objType = CV_TYPE_ECC_KEY;
		cvCopySwap((uint32_t *)&cacPivObj->pubkey[0],(uint32_t *)&key->key[7*keyLenBytes], keyLenBytes);
		cvCopySwap((uint32_t *)&cacPivObj->pubkey[ECC256_KEY_LEN],(uint32_t *)&key->key[8*keyLenBytes], keyLenBytes);
	}
	key->keyLen = keyLenBytes*8;

	/* now issue challenge to smart card */
	if (cardType == CVSC_CAC_CARD)
		cvscStatus = scCACChallenge(cacPivObj->cardSignAlg, challengeLen, &challenge[0], 
			&responseLen, &response[0], objProperties->PINLen, &objProperties->PIN[0]);
	else
		cvscStatus = scPIVChallenge(cacPivObj->cardSignAlg, FALSE, challengeLen, &challenge[0], 
			&responseLen, &response[0], objProperties->PINLen, &objProperties->PIN[0]);

	if (cvscStatus != PHX_STATUS_OK) {
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* now verify signature using pubkey in object auth */
	if ((retVal = cvVerify(objType, &rand[0], sizeof(rand), key, &response[0], responseLen, &verify)) != CV_SUCCESS)
		goto err_exit;
	if (!verify)
	{
		/* verification failed.  if this is PIV, it may have failed because enrollment used GSC-IS certificate. */
		/* try again with key corresponding to GSC-IS */
		if (cardType == CVSC_PIV_CARD)
		{
			if ((scPIVChallenge(cacPivObj->cardSignAlg, TRUE, challengeLen, &challenge[0], 
				&responseLen, &response[0], objProperties->PINLen, &objProperties->PIN[0])) == PHX_STATUS_OK)
				if ((cvVerify(objType, &rand[0], sizeof(rand), key, &response[0], responseLen, &verify)) == CV_SUCCESS)
					if (verify)
						goto compare_date;
		}
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* now compare expiration date */
compare_date:
	if (!isValidDate(cacPiv->currentDate.month, cacPiv->currentDate.day, cacPiv->currentDate.year))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
    if (cacPiv->currentDate.year > cacPivObj->expirationDate.year)
		retVal = CV_AUTH_FAIL;
	else if (cacPiv->currentDate.year == cacPivObj->expirationDate.year)
	{
		if (cacPiv->currentDate.month > cacPivObj->expirationDate.month)
			retVal = CV_AUTH_FAIL;
		else if (cacPiv->currentDate.month == cacPivObj->expirationDate.month)
			if (cacPiv->currentDate.day > cacPivObj->expirationDate.day)
				retVal = CV_AUTH_FAIL;
	}

err_exit:
	return retVal;
}

cv_status
cvAuthPIVCtls(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail)
{
	/* this routine compares the contactless credential in the CV object with the one read in */
	/* from contactless card.  Note that the credential from the contactless card isn't a CV object and that */
	/* it should have been read in before this point */
	cv_auth_entry_piv_ctls_obj *PIVCtlsObj;
	cv_auth_entry_cac_piv *PIVCtls;
	cv_status retVal = CV_SUCCESS;

	PIVCtls = (cv_auth_entry_cac_piv *)(authEntryParam + 1);
	PIVCtlsObj = (cv_auth_entry_piv_ctls_obj *)(authEntryObj + 1);
	if (determineAvail)
		return CV_SUCCESS;

	/* now compare credential from CV with one read in from PIV card */
	if (CV_VOLATILE_DATA->HIDCredentialPtr != NULL)
	{
		/* first check to see if it's the right type of contactless smart card */
		if (CV_VOLATILE_DATA->credentialType != CV_CONTACTLESS_TYPE_PIV)
		{
			retVal = CV_INVALID_CONTACTLESS_CARD_TYPE;
			CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
			goto err_exit;
		}
		/* now check credential */
		if (memcmp(&PIVCtlsObj->hCHUID[0], CV_VOLATILE_DATA->HIDCredentialPtr, sizeof(PIVCtlsObj->hCHUID)))
		{
			retVal = CV_AUTH_FAIL;
			CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
			goto err_exit;
		}
		/* invalidate HID credential pointer, since will need to get read in next time it's used */
		CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
	} else
		/* HID credential not resident.  fail, because should have been read in when determineAuthAvail called */
		retVal = CV_INVALID_AUTH_LIST;

	/* now compare expiration date */
	if (!isValidDate(PIVCtls->currentDate.month, PIVCtls->currentDate.day, PIVCtls->currentDate.year))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
    if (PIVCtls->currentDate.year > PIVCtlsObj->expirationDate.year)
		retVal = CV_AUTH_FAIL;
	else if (PIVCtls->currentDate.year == PIVCtlsObj->expirationDate.year)
	{
		if (PIVCtls->currentDate.month > PIVCtlsObj->expirationDate.month)
			retVal = CV_AUTH_FAIL;
		else if (PIVCtls->currentDate.month == PIVCtlsObj->expirationDate.month)
			if (PIVCtls->currentDate.day > PIVCtlsObj->expirationDate.day)
				retVal = CV_AUTH_FAIL;
	}

err_exit:
	return retVal;
}

cv_status
cvPromptForSmartCardOrFingerprint(cv_callback *callback, cv_callback_ctx context)
{
	/* this routine invokes the callback to cause a user prompt to be displayed */
	/* indicating a smart card insertion or fingerprint swipe is required.  a loop */
	/* is set up to check for 1 of 3 things to occur: SC insertion, FP swipe or user cancellation */
	uint32_t startTime, lastCallback;
	cv_status retVal = CV_SUCCESS;
	uint32_t localPrompt = CV_PROMPT_FOR_SMART_CARD;
	cv_bool promptDisplayed = FALSE;
	cv_bool scIsPresent = FALSE, fpIsPresent = TRUE;

	/* check to make sure no async FP capture is active */
	if (!isFPImageBufAvail())
		return CV_FP_DEVICE_BUSY;

	/* change prompt based on devices present */
	if (CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT)
	{
		fpIsPresent = TRUE;
		if (CV_PERSISTENT_DATA->persistentFlags & (CV_HAS_FP_AT_SWIPE | CV_HAS_FP_UPEK_SWIPE))
			localPrompt = CV_PROMPT_FOR_FINGERPRINT_SWIPE;
		else
			localPrompt = CV_PROMPT_FOR_FINGERPRINT_TOUCH;
	} else if (CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT) {
		scIsPresent = TRUE;
		localPrompt = CV_PROMPT_FOR_SMART_CARD;
	}
	/* check to make sure SC not present first */
	if (scIsPresent && scIsCardPresent())
		/* yes, fail, because user must physically insert card */
		return CV_AUTH_FAIL;

	/* set up loop to send prompt and check for insertion/swipe */
	get_ms_ticks(&startTime);
	lastCallback = startTime;
	/* set up bulk out.  this is because the cvActualCallback routine expects this to be set the first time */
	/* it's called for prompting (based on FP capture yield processing) */
	usbCvBulkOut(CV_SECURE_IO_COMMAND_BUF);

	if (fpIsPresent)
	{
		fpNextSensorDoBaseline();
	}

	do
	{
		/* now call callback, if time to */
		if(cvGetDeltaTime(lastCallback) >= USER_PROMPT_CALLBACK_INTERVAL)
		{
			if ((retVal = (*callback)(localPrompt, 0, NULL, context)) == CV_CANCELLED_BY_USER)
				break;
			promptDisplayed = TRUE;
			get_ms_ticks(&lastCallback);
		}
		if (scIsPresent && scIsCardPresent())
		{
			retVal = CV_SUCCESS;
			/* now send callback to remove prompt */
			if (promptDisplayed)
				retVal = (*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
			/* now delay briefly to allow card to be completely inserted */
			get_ms_ticks(&lastCallback);
			do
			{
				if(cvGetDeltaTime(lastCallback) >= SMART_CARD_INSERTION_DELAY)
					break;
			} while (TRUE);
			break;
		}
		if (fpIsPresent)
		{
        	if ((fpGrab((uint32_t *)NULL, (uint32_t *)NULL, PBA_FP_DETECT_TIMEOUT, 
        		 	NULL, NULL, FP_POLLING_PERIOD, FP_FINGER_DETECT)) == FP_OK) {
									retVal = CV_SUCCESS;
				/* now send callback to remove prompt */
				if (promptDisplayed)
					retVal = (*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
				break;
			}
		}

		if(cvGetDeltaTime(startTime) >= USER_PROMPT_TIMEOUT)
		{
			/* now send callback to remove prompt */
			if (promptDisplayed)
				(void)(*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
			retVal = CV_CALLBACK_TIMEOUT;
			goto err_exit;
		}
		yield();
	} while (TRUE);

err_exit:
	return retVal;
}

cv_bool
cvInDALockout(void)
{
	/* this routine checks to see if a dictionary attack lockout is in progress or not */

	/* check to see if lockout active */
	if (CV_VOLATILE_DATA->CVInternalState & CV_IN_DA_LOCKOUT)
	{
		/* yes, check to see if lockout has expired */
		if (cvGetDeltaTime(CV_VOLATILE_DATA->DALockoutStart) > 
			((CV_PERSISTENT_DATA->DAInitialLockoutPeriod)*(1<<(CV_VOLATILE_DATA->DALockoutCount - 1))))
		{
			/* yes, lockout has expired, clear condition */
			CV_VOLATILE_DATA->CVInternalState &= ~CV_IN_DA_LOCKOUT;
			CV_VOLATILE_DATA->DAFailedAuthCount = 0;
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

void
cvChkDA(cv_status status)
{
	/* this routine checks to see if lockout should be initiated, based on auth failures */
	if (status == CV_AUTH_FAIL)
	{
		/* this is an auth failure, see if failure count exceeded */
		if (++CV_VOLATILE_DATA->DAFailedAuthCount >= CV_PERSISTENT_DATA->DAAuthFailureThreshold)
		{
			/* yes, initiate lockout */
			CV_VOLATILE_DATA->CVInternalState |= CV_IN_DA_LOCKOUT;
			CV_VOLATILE_DATA->DALockoutCount++;
			get_ms_ticks(&CV_VOLATILE_DATA->DALockoutStart);
		}
	}
}

cv_bool
cvFindObjAuthList(cv_auth_list *pList, cv_auth_list *oList, uint32_t objAuthListsLen, cv_auth_list **oListFound)
{
	/* now need to find the auth list in the object that corresponds to the one from the parameter list */
	cv_auth_list *oListEnd;
	uint32_t authID;
	cv_bool found = FALSE;
	cv_auth_hdr *authEntryObj;
	uint32_t i;

	oListEnd = (cv_auth_list *)((uint8_t *)oList + objAuthListsLen);
	authID = pList->listID;
	do
	{
		if (authID == oList->listID)
		{
			found = TRUE;
			break;
		}
		/* step through entries of auth list */
		authEntryObj = (cv_auth_hdr *)(oList + 1);
		for (i=0;i<oList->numEntries;i++)
			authEntryObj = (cv_auth_hdr *)((uint8_t *)authEntryObj + authEntryObj->authLen + sizeof(cv_auth_hdr));
		oList = (cv_auth_list *)authEntryObj;
	} while (oList < oListEnd);
	if (found)
	{
		*oListFound = oList;
		return TRUE;
	} else
		return FALSE;
}

cv_auth_entry_PIN *cvFindPINAuthEntry(cv_auth_list *pList, cv_obj_properties *objProperties)
{
	/* this routine finds the PIN auth list entry (if any) in the parameter list */
	uint32_t i;
	cv_auth_hdr *authEntryParam;
	cv_auth_entry_PIN *PIN;

	authEntryParam = (cv_auth_hdr *)(pList + 1);
	for (i=0;i<pList->numEntries;i++)
	{
		if (authEntryParam->authType == CV_AUTH_PIN)
		{
			/* PIN found */
			/* reset any pending state waiting for PIN */
			PIN = (cv_auth_entry_PIN *)(authEntryParam + 1);
			if (PIN->PINLen == 0)
			{
				objProperties->PIN = NULL;
				objProperties->PINLen = 0;
				return NULL;
			} else {
				CV_VOLATILE_DATA->CVInternalState &= ~CV_WAITING_FOR_PIN_ENTRY;
				objProperties->PIN = PIN->PIN;
				objProperties->PINLen = PIN->PINLen;
			}
			return PIN;
		}
		authEntryParam = (cv_auth_hdr *)((uint8_t *)authEntryParam + authEntryParam->authLen +  + sizeof(cv_auth_hdr));
	}
	return NULL;
}

cv_bool
cvIsPrompt(cv_status status)
{
	/* this routine determines if the indicated status is prompt related (else error code) */
	if (status == CV_PROMPT_FOR_SMART_CARD || status == CV_PROMPT_FOR_CONTACTLESS || status == CV_PROMPT_PIN
		|| status == CV_PROMPT_PIN_AND_SMART_CARD || status == CV_PROMPT_PIN_AND_CONTACTLESS)
		return TRUE;
	else
		return FALSE;
}

cv_status
cvDoPartialAuthListsChange(
	/* in */ uint32_t changeAuthListsLen,
	/* in */ cv_auth_list *changeAuthLists,    /* note: this points past the initial empty auth list with CV_CHANGE_AUTH_PARTIAL set in it */
	/* in */ uint32_t currentAuthListsLen,
	/* in */ cv_auth_list *currentAuthLists,
	/* out */ uint32_t *newAuthListsLen,
	/* out */ cv_auth_list *newAuthLists)
{
	/* this routine uses an input change auth lists to modify an existing auth lists and produce a new auth lists.  */
	/* The calling routine will need to have a buffer large enough to hold the maximum object size for the new auth lists */
	uint32_t i;
	cv_auth_list *authListPtr, *authListPtrDeletions, *authListPtrChangeAdds, *newAuthListsPtr = newAuthLists;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry;
	uint32_t numDeletions = 0;
	cv_status retVal = CV_SUCCESS;
	uint32_t deletionsLen = 0, changeAddsLen = 0, authListLen = 0;
	cv_auth_list *foundAuthListPtr;

	/* Determine if any deletions.  Deletions are empty auth lists with CV_CHANGE_AUTH_PARTIAL flag set in it.  */
	/* All deletions must preceed any changes/additions.  Changes and additions won’t have the CV_CHANGE_AUTH_PARTIAL flag set. */
	/* Set pointers to start of deletions and changes/additions in change auth lists. */
	authListPtr = changeAuthLists;
	authListsEnd = (uint8_t *)authListPtr + changeAuthListsLen;
	authListPtrDeletions = authListPtr;
	do
	{
		/* check to see if this is a deletion */
		if (authListPtr->flags & CV_CHANGE_AUTH_PARTIAL)
		{
			/* check to make sure this auth list exists */
			if (!cvFindObjAuthList(authListPtr, currentAuthLists, currentAuthListsLen, &foundAuthListPtr))
			{
				/* invalid deletion  */
				retVal = CV_INVALID_AUTH_LIST;
				goto err_exit;
			}
			numDeletions++;
		} else 
			/* not a deletion, just exit */
			break;
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntry > authListsEnd)
			{
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				retVal = CV_INVALID_AUTH_LIST;
				goto err_exit;
			}
		}
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);
	/* set up pointer to start of change/add list */
	authListPtrChangeAdds = authListPtr;
	/* compute lengths of both lists */
	if (numDeletions)
		deletionsLen = (uint8_t *)authListPtr - (uint8_t *)changeAuthLists;
	changeAddsLen = changeAuthListsLen - deletionsLen;

	/* process deletions and changes: */
	/* Parse current auth list and for each entry, search deletions and changes lists to see if found. */
	authListPtr = currentAuthLists;
	authListsEnd = (uint8_t *)authListPtr + currentAuthListsLen;
	do
	{
		/* check to see if this auth list s/b deleted */
		if (!deletionsLen || !cvFindObjAuthList(authListPtr, authListPtrDeletions, deletionsLen, &foundAuthListPtr))
		{
			/* not deletion, check to see if changed */
			if (changeAddsLen && cvFindObjAuthList(authListPtr, authListPtrChangeAdds, changeAddsLen, &foundAuthListPtr))
			{
				/* this auth list has changed, copy new one */
				cvCopyAuthList(newAuthListsPtr, foundAuthListPtr, &authListLen);
				newAuthListsPtr = (cv_auth_list *)((uint8_t *)newAuthListsPtr + authListLen);
			} else {
				/* auth list hasn't changed, just copy existing one */
				cvCopyAuthList(newAuthListsPtr, authListPtr, &authListLen);
				newAuthListsPtr = (cv_auth_list *)((uint8_t *)newAuthListsPtr + authListLen);
			}
		}

		/* skip to next auth list */
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntry > authListsEnd)
			{
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				retVal = CV_INVALID_AUTH_LIST;
				goto err_exit;
			}
		}
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);

	/* process additions: */
	/* Parse change auth lists and for each entry, search current auth list to see if found */
	/* if not found in current list, copy to new auth lists buffer */
	authListPtr = authListPtrChangeAdds;
	authListsEnd = (uint8_t *)authListPtr + changeAddsLen;
	if (changeAddsLen)
	{
		do
		{
			/* check to see if this auth list is in current lists */
			if (!cvFindObjAuthList(authListPtr, currentAuthLists, currentAuthListsLen, &foundAuthListPtr))
			{
				/* not in current, this is an add so copy to new buffer */
				cvCopyAuthList(newAuthListsPtr, authListPtr, &authListLen);
				newAuthListsPtr = (cv_auth_list *)((uint8_t *)newAuthListsPtr + authListLen);
			}

			/* skip to next auth list */
			authEntry = (cv_auth_hdr *)(authListPtr + 1);
			for (i=0;i<authListPtr->numEntries;i++)
			{
				authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
				if ((uint8_t *)authEntry > authListsEnd)
				{
					/* shouldn't get here.  something must be wrong with auth list, just bail  */
					retVal = CV_INVALID_AUTH_LIST;
					goto err_exit;
				}
			}
			authListPtr = (cv_auth_list *)authEntry;
		} while ((uint8_t *)authListPtr < authListsEnd);
	}

err_exit:
	*newAuthListsLen = (uint8_t *)newAuthListsPtr - (uint8_t *)newAuthLists;
	return retVal;
}

cv_status
cvDoAuth(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties, 
		 uint32_t paramAuthListLength, cv_auth_list *paramAuthList,
		 cv_obj_auth_flags *authFlags, cv_input_parameters *inputParameters,
		 cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail,
		 uint32_t *statusCount, cv_status *statusList)
{
	/* this routine does one of 2 things, based on whether determineAvail is set or not.  if not set, it */
	/* determines if the authorization provided satisfies the authorization required by the object */
	/* if so, CV_SUCCESS is returned and authFlags indicates what action is authorized.  if determineAvail is */
	/* TRUE, this routine just checks to see if any prompts will be required for subsequent auth processing */
	/* and outputs statusList */
	cv_auth_list *oList, *oListEnd;
	cv_status retVal = CV_SUCCESS, retValObjSave = CV_SUCCESS;
	cv_obj_auth_flags retAuth, modifiedRetAuth;
	uint32_t i;
	cv_auth_hdr *authEntryObj,  *authEntryParam;
	uint32_t retStatusCount = 0;
	uint32_t objAuthListsLen = objProperties->authListsLength;
	cv_auth_list *objAuthLists = objProperties->pAuthLists;
	cv_bool hasCountAuth = FALSE;
	uint32_t totLen = 0, remLength = 0, additionalAuths = 0;

	/* first check to see if no auth list provided.  this is only ok if object also has no auth */
	if (!paramAuthListLength)
	{
		/* first check to see if CV admin auth required.  if so, fail here, because no auth list provided */
		if (cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH || cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH_ALLOW_PP_AUTH)
		{
			retVal = CV_ADMIN_AUTH_REQUIRED;
			goto err_exit;
		}
		if (determineAvail)
			*statusCount = 0;
		if (!objAuthListsLen)
			return CV_SUCCESS;
		else
			return CV_AUTH_FAIL;
	}

	/* check to see if physical presence auth is being asserted */
	if (paramAuthList->flags & CV_PHYSICAL_PRESENCE_AUTH)
	{
		*statusCount = 0;
		/* only allow physical presence auth as a substitute for CV admin auth */
		if (cvAdminAuthPermission != CV_REQUIRE_ADMIN_AUTH_ALLOW_PP_AUTH)
		{
			retVal = CV_PHYSICAL_PRESENCE_AUTH_NOT_ALLOWED;
			goto err_exit;
		}
		if (determineAvail)
			return CV_SUCCESS;
		/* now wait for FP swipe or card to be inserted */
		if ((retVal = cvPromptForSmartCardOrFingerprint(callback, context)) != CV_SUCCESS)
			goto err_exit;
		/* physical presence succeeded.  allow all authorizations */
		*authFlags = CV_OBJ_AUTH_READ_PUB | CV_OBJ_AUTH_READ_PRIV | CV_OBJ_AUTH_WRITE_PUB
			| CV_OBJ_AUTH_WRITE_PRIV | CV_OBJ_AUTH_DELETE | CV_OBJ_AUTH_ACCESS | CV_OBJ_AUTH_CHANGE_AUTH;
		/* check to see if clear auth requested */
		if (paramAuthList->flags & CV_CLEAR_ADMIN_AUTH)
			*authFlags |= CV_CLEAR_ADMIN_AUTH; 
		return CV_SUCCESS;
	}

	/* now check to see if CV admin auth is being provided */
	if (paramAuthList->flags & CV_ADMINISTRATOR_AUTH)
	{
		if (cvAdminAuthPermission == CV_DENY_ADMIN_AUTH)
		{
			retVal = CV_ADMIN_AUTH_NOT_ALLOWED;
			goto err_exit;
		}
		/* make sure CV admin auth exists */
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_CV_ADMIN))
		{
			retVal = CV_ADMIN_AUTH_REQUIRED;
			goto err_exit;
		}
		/* set up pointer to CV admin auth */
		authEntryParam = (cv_auth_hdr *)(paramAuthList + 1);
		/* check to see if CV_SUPER_AUTH is being used to change CV admin auth.  if so, use param auth list instead of */
		/* CV admin auth list */
		if (cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH_ALLOW_SUPER_AUTH && authEntryParam->authType == CV_AUTH_SUPER)
		{
			objAuthLists = paramAuthList;
			objAuthListsLen = paramAuthListLength;
		} else {
			objAuthLists = &CV_PERSISTENT_DATA->CVAdminAuthList;
			objAuthListsLen = CV_PERSISTENT_DATA->CVAdminAuthListLen;
		}
		/* set up clear admin auth, if requested */
		if (paramAuthList->flags & CV_CLEAR_ADMIN_AUTH)
			additionalAuths = CV_CLEAR_ADMIN_AUTH; 
		additionalAuths |= CV_ADMINISTRATOR_AUTH;
	} else {
		/* CV admin auth not provided.  see if it should have been */
		if (cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH || cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH_ALLOW_PP_AUTH
			|| cvAdminAuthPermission == CV_REQUIRE_ADMIN_AUTH_ALLOW_SUPER_AUTH)
		{
			retVal = CV_ADMIN_AUTH_REQUIRED;
			goto err_exit;
		}
	}
	/* check to see if there is no object auth list.  if so, authorization is automatically granted */
	if (objAuthListsLen == 0)
	{
		/* set auth flags to allow all operations */
		*authFlags = CV_OBJ_AUTH_READ_PUB | CV_OBJ_AUTH_READ_PRIV | CV_OBJ_AUTH_WRITE_PUB
			| CV_OBJ_AUTH_WRITE_PRIV | CV_OBJ_AUTH_DELETE | CV_OBJ_AUTH_ACCESS | CV_OBJ_AUTH_CHANGE_AUTH;
		if (determineAvail)
			*statusCount = 0;
		return CV_SUCCESS;
	}

	/* find auth list in object */
	if (!cvFindObjAuthList(paramAuthList, objAuthLists, objAuthListsLen, &oList))
	{
		retVal = CV_CORRESPONDING_AUTH_NOT_FOUND_IN_OBJECT;
		goto err_exit;
	}
	oListEnd = (cv_auth_list *)((uint8_t *)objAuthLists + objAuthListsLen);

	/* check to see if auth list selected has 0 auth entries. if so, automatically grant auth */
	/* indicated by flags entry */
	if (oList->numEntries == 0)
	{
		*authFlags = oList->flags;
		if (determineAvail)
			*statusCount = 0;
		return CV_SUCCESS;
	}

	/* now parse auth list and call routines to do specific authorization test */
	authEntryObj = (cv_auth_hdr *)(oList + 1);
	authEntryParam = (cv_auth_hdr *)(paramAuthList + 1);
	/* only allow authorization provided by object and requested by parameter list */
	retAuth = oList->flags & paramAuthList->flags;
	for (i=0;i<oList->numEntries;i++)
	{
		retVal = CV_SUCCESS;
		switch (authEntryObj->authType)
		{
		case CV_AUTH_PASSPHRASE:
			/* do check to see if in dictionary attack mitigation */
			if (cvInDALockout())
				retVal = CV_IN_LOCKOUT;
			else {
				retVal = cvAuthPassphrase(authEntryParam, authEntryObj, objProperties, determineAvail);
				/* check to see if an auth failure occurred and if so, check to see if dictionary attack */
				/* mitigation required */
				cvChkDA(retVal);
			}
			break;
		case CV_AUTH_PASSPHRASE_HMAC:
			/* do check to see if in dictionary attack mitigation */
			if (cvInDALockout())
				retVal = CV_IN_LOCKOUT;
			else {
				retVal = cvAuthPassphraseHMAC(authEntryParam, authEntryObj, objProperties, inputParameters, determineAvail); 
				/* check to see if an auth failure occurred and if so, check to see if dictionary attack */
				/* mitigation required */
				cvChkDA(retVal);
			}
			break;
		case CV_AUTH_FINGERPRINT:
			retVal = cvAuthFingerprint(authEntryParam, authEntryObj, objProperties, callback, context, determineAvail); 
			break;
		case CV_AUTH_HOTP:
			retVal = cvAuthHOTP(authEntryParam, authEntryObj, objProperties, callback, context, determineAvail); 
			break;
		case CV_AUTH_SESSION:
			if (!determineAvail)
			{
				retVal = cvAuthSession(authEntryParam, authEntryObj, NULL, &modifiedRetAuth); 
				/* only allow authorization that's provided by auth session */
				retAuth &= modifiedRetAuth;
			}
			break;
		case CV_AUTH_SESSION_HMAC:
			if (!determineAvail)
			{
				retVal = cvAuthSession(authEntryParam, authEntryObj, inputParameters, &modifiedRetAuth); 
				/* only allow authorization that's provided by auth session */
				retAuth &= modifiedRetAuth;
			}
			break;
		case CV_AUTH_COUNT:
			if (!determineAvail)
			{
				retVal = cvAuthCount(authEntryObj); 
				/* save the status, in case it indicates that object save required */
				retValObjSave = retVal;
				if (retVal == CV_OBJECT_UPDATE_REQUIRED)
					hasCountAuth = TRUE;
			}
			break;
		case CV_AUTH_TIME:
			if (!determineAvail)
				retVal = cvAuthTime(authEntryObj); 
			break;
		case CV_AUTH_CONTACTLESS:
			retVal = cvAuthHID(authEntryObj, objProperties, determineAvail); 
			/* if determining availability, cvAuthHID just determines availability of HID credential */
			/* in CV.  also add prompt to cause HID card to be inserted to get HID credential for */
			/* comparison with CV value */
			if (determineAvail)
			{
				/* check to make sure radio present */
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_RF_PRESENT)) {
					retVal = CV_RADIO_NOT_PRESENT;
					break;
				}
				/* check to make sure radio enabled */
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_RF_ENABLED)) {
					retVal = CV_RADIO_NOT_ENABLED;
					break;
				}
				/* check to see if credential already cached by cv_identify_user */
				if (CV_VOLATILE_DATA->HIDCredentialPtr == NULL)
				{
					/* no, need to prompt for contactless card */
					if (retStatusCount < *statusCount)
						statusList[retStatusCount++] = CV_READ_HID_CREDENTIAL;
				}
			}
			break;
		case CV_AUTH_SMARTCARD:
			retVal = cvAuthSmartCard(authEntryObj, objProperties, determineAvail); 
			break;
		case CV_AUTH_PROPRIETARY_SC:
			retVal = cvAuthProprietarySC(authEntryObj, objProperties, determineAvail); 
			break;
		case CV_AUTH_CHALLENGE:
			retVal = cvAuthChallenge(authEntryParam, authEntryObj, objProperties, determineAvail); 
			break;
		case CV_AUTH_SUPER:
			if (!determineAvail)
				retVal = cvAuthSuper(authEntryParam); 
			break;
		case CV_AUTH_CAC_PIV:
			retVal = cvAuthCACPIV(authEntryParam, authEntryObj, objProperties, determineAvail); 
			break;
		case CV_AUTH_PIV_CTLS:
			retVal = cvAuthPIVCtls(authEntryParam, authEntryObj, objProperties, determineAvail); 
			/* if determining availability, cvAuthPIVCtls just returns TRUE.  also add prompt to cause */
			/* PIV card to be inserted to get PIV CHUID for comparison with CV value */
			if (determineAvail)
			{
				/* check to make sure radio present */
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_RF_PRESENT)) {
					retVal = CV_RADIO_NOT_PRESENT;
					break;
				}
				/* check to make sure radio enabled */
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_RF_ENABLED)) {
					retVal = CV_RADIO_NOT_ENABLED;
					break;
				}
				/* check to see if credential already cached by cv_identify_user */
				if (CV_VOLATILE_DATA->HIDCredentialPtr == NULL)
				{
					/* no, need to prompt for contactless card */
					if (retStatusCount < *statusCount)
						statusList[retStatusCount++] = CV_READ_HID_CREDENTIAL;
				}
			}
			break;
#ifdef ST_AUTH
		case CV_AUTH_ST_CHALLENGE:
			if (!determineAvail)
				retVal = cvAuthSTChallenge(authEntryParam, authEntryObj); 
			break;
#endif
		case 0xffff:
			break;
		}
		if (retVal != CV_SUCCESS && retVal != CV_OBJECT_UPDATE_REQUIRED)
		{
			if (determineAvail)
			{
				/* add this status to output list (if room) and continue */
				if (cvIsPrompt(retVal) && retStatusCount < *statusCount)
				{
					statusList[retStatusCount++] = retVal;
					retVal = CV_SUCCESS;
				}
				else
				{
					/* exceeded number of statuses expected or got error status */
					if (cvIsPrompt(retVal))
					{
						/* exceeded statuses */
						retVal = CV_INVALID_AUTH_LIST;
						*statusCount = 0;
					}
					goto err_exit;
				}
			} else
				goto err_exit;
		}
		authEntryObj = (cv_auth_hdr *)((uint8_t *)authEntryObj + authEntryObj->authLen + sizeof(cv_auth_hdr));
		authEntryParam = (cv_auth_hdr *)((uint8_t *)authEntryParam + authEntryParam->authLen + sizeof(cv_auth_hdr));
		/* bump key handle object to MRU so doesn't get flushed from cache.  Cache only has entries for 4 objects */
		/* and auth could potentially use 2 for each auth */
		cvUpdateObjCacheMRUFromHandle(objProperties->objHandle);
		if ((cv_auth_list *)authEntryObj > oListEnd)
		{
			/* shouldn't get here.  something must be wrong with auth list in object, just bail  */
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
	}
	/* now do check of length of auth list provided in parameter list.  if not same as length provided, fail */
	totLen = (uint8_t *)authEntryParam - (uint8_t *)paramAuthList;
	if (totLen != paramAuthListLength)
	{
		/* check to see if the remaining parameter is the PIN auth entry.  all other differences in size are failures */
		remLength = paramAuthListLength - totLen;
		if (!(authEntryParam->authType == CV_AUTH_PIN && remLength == (authEntryParam->authLen + sizeof(cv_auth_hdr))))
		{
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}
	/* if got here, must have passed all authorization checks.  if just checking for object availability, */
	/* update count */
	if (determineAvail)
		*statusCount = retStatusCount;
	else
		*authFlags = retAuth | additionalAuths;

err_exit:
	/* if status is success, return the value saved by CV_AUTH_COUNT, in case it indicates object save reqd */
	if (retVal == CV_SUCCESS || retVal == CV_OBJECT_UPDATE_REQUIRED)
		return retValObjSave;
	else
	{
		/* auth failed.  if count auth found, need to invalidate object so that updated count won't be used again */
		if (hasCountAuth)
			cvFlushCacheEntry(objProperties->objHandle);
		return retVal;
	}
}

cv_obj_handle
cvGetNewHandle(cv_obj_handle handle, uint32_t count, cv_obj_handle *currentHandles, cv_obj_handle *newHandles)
{
	/* this routine finds a handle in currentHandles list and returns the new handle (or same handle, if not found in currentHandles list */
	uint32_t i;

	for (i=0; i<count; i++)
	{
		if (handle == currentHandles[i])
			return newHandles[i];
	}
	return handle;
}

void
cvRemapHandles(uint32_t authListsLength, cv_auth_list *pAuthLists, uint32_t count, cv_obj_handle *currentHandles, cv_obj_handle *newHandles)
{
	/* this routine parses the auth lists of an object and remaps object handles */
	uint32_t i, j;
	cv_auth_list *authListPtr;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry;
	cv_auth_entry_passphrase_handle *passphraseHandleEntry;
	cv_auth_entry_fingerprint_obj *fingerprintEntry;
	cv_auth_entry_hotp *hotpEntry;
	cv_auth_entry_contactless_handle *HIDEntry;
	cv_auth_entry_smartcard_handle *SCEntry;
	cv_auth_entry_challenge_handle_obj *challengeEntry;

	/* now parse auth lists and check each entry */
	authListPtr = pAuthLists;
	authListsEnd = (uint8_t *)authListPtr + authListsLength;
	do
	{
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			switch (authEntry->authType)
			{
			case CV_AUTH_PASSPHRASE_HMAC:
			case CV_AUTH_PASSPHRASE:
				passphraseHandleEntry = (cv_auth_entry_passphrase_handle *)(authEntry + 1);
				if (passphraseHandleEntry->constZero == 0)
					/* get new handle  */
					passphraseHandleEntry->hPassphrase = cvGetNewHandle(passphraseHandleEntry->hPassphrase, count, currentHandles, newHandles);
				break;
			case CV_AUTH_FINGERPRINT:
				fingerprintEntry = (cv_auth_entry_fingerprint_obj *)(authEntry + 1);
				/* convert templates */
				for (j=0;j<fingerprintEntry->numTemplates;j++)
					fingerprintEntry->templates[j] = cvGetNewHandle(fingerprintEntry->templates[j], count, currentHandles, newHandles);
				break;
			case CV_AUTH_HOTP:
				hotpEntry = (cv_auth_entry_hotp *)(authEntry + 1);
				/* get new handle  */
				hotpEntry->hHOTPObj = cvGetNewHandle(hotpEntry->hHOTPObj, count, currentHandles, newHandles);
				break;
			case CV_AUTH_CONTACTLESS:
				HIDEntry = (cv_auth_entry_contactless_handle *)(authEntry + 1);
				/* get new handle  */
				if (HIDEntry->constZero == 0)
					HIDEntry->hHIDObj = cvGetNewHandle(HIDEntry->hHIDObj, count, currentHandles, newHandles);
				break;
			case CV_AUTH_SMARTCARD:
				SCEntry = (cv_auth_entry_smartcard_handle *)(authEntry + 1);
				/* get new handle  */
				if (SCEntry->constZero == 0)
					SCEntry->keyObj = cvGetNewHandle(SCEntry->keyObj, count, currentHandles, newHandles);
				break;
			case CV_AUTH_CHALLENGE:
				challengeEntry = (cv_auth_entry_challenge_handle_obj *)(authEntry + 1);
				/* get new handle  */
				if (challengeEntry->constZero == 0)
					challengeEntry->keyObj = cvGetNewHandle(challengeEntry->keyObj, count, currentHandles, newHandles);
				break;
			case 0xffff:
				break;
			}
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
		}
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);
}

cv_status
cvBuildEmbeddedHandlesList(cv_status status, uint8_t * maxPtr, uint32_t authListsLength, cv_auth_list *pAuthLists, uint32_t *length, cv_obj_handle *handles)
{
	/* this routine parses the auth lists of an object and builds an embedded handles list */
	/* if buffer length exceeded, don't store handle, but continue to determine required length */
	uint32_t i, j;
	cv_auth_list *authListPtr;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry;
	cv_auth_entry_passphrase_handle *passphraseHandleEntry;
	cv_auth_entry_fingerprint_obj *fingerprintEntry;
	cv_auth_entry_hotp *hotpEntry;
	cv_auth_entry_contactless_handle *HIDEntry;
	cv_auth_entry_smartcard_handle *SCEntry;
	cv_auth_entry_challenge_handle_obj *challengeEntry;
	cv_status retVal = status;
	uint8_t *listStart = (uint8_t *)handles;


	/* now parse auth lists and check each entry */
	authListPtr = pAuthLists;
	authListsEnd = (uint8_t *)authListPtr + authListsLength;
	do
	{
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			if (((uint8_t *)handles + sizeof(cv_obj_handle)) > maxPtr)
				retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			switch (authEntry->authType)
			{
			case CV_AUTH_PASSPHRASE_HMAC:
			case CV_AUTH_PASSPHRASE:
				passphraseHandleEntry = (cv_auth_entry_passphrase_handle *)(authEntry + 1);
				if (passphraseHandleEntry->constZero == 0)
				{
					/* save handle  */
					if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
						*handles = passphraseHandleEntry->hPassphrase;
					handles++;
				}
				break;
			case CV_AUTH_FINGERPRINT:
				fingerprintEntry = (cv_auth_entry_fingerprint_obj *)(authEntry + 1);
				/* convert templates */
				for (j=0;j<fingerprintEntry->numTemplates;j++)
				{
					/* save handle  */
					if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
						*handles = fingerprintEntry->templates[j];
					handles++;
				}
				break;
			case CV_AUTH_HOTP:
				hotpEntry = (cv_auth_entry_hotp *)(authEntry + 1);
				/* save handle  */
				if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
					*handles = hotpEntry->hHOTPObj;
				handles++;
				break;
			case CV_AUTH_CONTACTLESS:
				HIDEntry = (cv_auth_entry_contactless_handle *)(authEntry + 1);
				/* save handle  */
				if (HIDEntry->constZero == 0)
				{
					if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
						*handles = HIDEntry->hHIDObj;
					handles++;
				}
				break;
			case CV_AUTH_SMARTCARD:
				SCEntry = (cv_auth_entry_smartcard_handle *)(authEntry + 1);
				/* save handle  */
				if (SCEntry->constZero == 0)
				{
					if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
						*handles = SCEntry->keyObj;
					handles++;
				}
				break;
			case CV_AUTH_CHALLENGE:
				challengeEntry = (cv_auth_entry_challenge_handle_obj *)(authEntry + 1);
				/* save handle  */
				if (challengeEntry->constZero == 0)
				{
					if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
						*handles = challengeEntry->keyObj;
					handles++;
				}
				break;
			case 0xffff:
				break;
			}
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
		}
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);
	*length = (uint8_t *)handles - listStart;
	return retVal;
}

cv_status
cvValidateAuthLists(cv_admin_auth_permission cvAdminAuthPermission, uint32_t authListsLength, cv_auth_list *pAuthLists)
{
	/* this routine determines whether the auth lists provided have a valid format */
	uint32_t i,j;
	cv_auth_list *authListPtr;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry;
	cv_auth_entry_passphrase *passphraseEntry;
	cv_auth_entry_passphrase_handle *passphraseHandleEntry;
	cv_obj_properties objProperties;
	cv_auth_entry_fingerprint_obj *fingerprintEntry;
	cv_auth_entry_hotp *hotpEntry;
	cv_auth_entry_contactless_handle *HIDEntryHandle;
	cv_auth_entry_contactless *HIDEntry;
	cv_auth_entry_smartcard_handle *SCEntryHandle;
	cv_auth_entry_smartcard *SCEntry;
	cv_obj_value_key *key;
	uint32_t keyLenBytes;
	cv_auth_entry_proprietary_sc *propSCEntry;
	cv_auth_entry_challenge_obj *challengeEntry;
	cv_auth_entry_challenge_handle_obj *challengeEntryHandle;
	cv_auth_entry_cac_piv_obj *CACPIVEntry;
	cv_auth_entry_piv_ctls_obj *PIVCtlsEntry;
	cv_auth_entry_PIN *PINEntry;
	cv_auth_entry_passphrase_HMAC_obj *passphraseHMAC;
	uint32_t authListCount = 1;
	uint8_t authListIDs[256];

	/* if length is 0, allow */
	if (!authListsLength)
		return CV_SUCCESS;

	/* now parse auth lists and check each entry */
	authListPtr = pAuthLists;
	authListsEnd = (uint8_t *)authListPtr + authListsLength;
	do
	{
		/* check to make sure auth list ID is unique */
		for (j=0;j<(authListCount-1);j++)
			if (authListPtr->listID == authListIDs[j])
				goto err_exit;
		authListIDs[authListCount-1] = authListPtr->listID;

		/* if CV admin auth, check to see if permissions set correctly */
#if 0
/* determine what this is supposed to check */
		if (!((cvAdminAuthPermission & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)) 
			|| (cvAdminAuthPermission & CV_OBJ_AUTH_CHANGE_AUTH)))
			goto err_exit;
#endif
		if ((cvAdminAuthPermission & CV_REQUIRE_ADMIN_AUTH) && !(authListPtr->flags & CV_ADMINISTRATOR_AUTH))
			/* must be CV admin auth, but isn't */
			goto err_exit;
		if ((cvAdminAuthPermission & CV_DENY_ADMIN_AUTH) && (authListPtr->flags & CV_ADMINISTRATOR_AUTH))
			/* must not be CV admin auth, but is */
			goto err_exit;
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		for (i=0;i<authListPtr->numEntries;i++)
		{
			switch (authEntry->authType)
			{
			case CV_AUTH_PASSPHRASE_HMAC:
				passphraseHMAC = (cv_auth_entry_passphrase_HMAC_obj *)(authEntry + 1);
				if (passphraseHMAC->hash.hashType == CV_SHA1)
				{
					if (authEntry->authLen != sizeof(cv_auth_entry_passphrase_HMAC_obj) - 1 + SHA1_LEN)
						goto err_exit;
				} else if (passphraseHMAC->hash.hashType == CV_SHA256) {
					if (authEntry->authLen != sizeof(cv_auth_entry_passphrase_HMAC_obj) - 1 + SHA256_LEN)
						goto err_exit;
				} else
					goto err_exit;
				break;
				
			case CV_AUTH_PASSPHRASE:
				/* check size of entry */
				passphraseHandleEntry = (cv_auth_entry_passphrase_handle *)(authEntry + 1);
				/* first check to see if entry contains a handle to object containing passphrase */
				if (passphraseHandleEntry->constZero == 0)
				{
					if (authEntry->authLen != sizeof(cv_auth_entry_passphrase_handle))
						goto err_exit;
					/* check handle.  note that this doesn't check for existence of host hard drive objects  */
					objProperties.objHandle = passphraseHandleEntry->hPassphrase;
					if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
						goto err_exit;
				} else {
					passphraseEntry = (cv_auth_entry_passphrase *)(authEntry + 1);
					if (authEntry->authLen != (passphraseEntry->passphraseLen + sizeof(passphraseEntry->passphraseLen)))
						goto err_exit;
				}
				break;
			case CV_AUTH_FINGERPRINT:
				fingerprintEntry = (cv_auth_entry_fingerprint_obj *)(authEntry + 1);
				/* check length */
				if (authEntry->authLen != (fingerprintEntry->numTemplates * sizeof(cv_obj_handle) 
					+ sizeof(fingerprintEntry->numTemplates)))
					goto err_exit;
				/* check templates */
				for (j=0;j<fingerprintEntry->numTemplates;j++)
				{
					objProperties.objHandle = fingerprintEntry->templates[j];
					if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
						goto err_exit;
				}
				break;
			case CV_AUTH_HOTP:
				hotpEntry = (cv_auth_entry_hotp *)(authEntry + 1);
				/* check length */
				if (authEntry->authLen != sizeof(cv_obj_handle))
				{
					goto err_exit;
				}
				/* check templates */
				objProperties.objHandle = hotpEntry->hHOTPObj;
				if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
				{
					goto err_exit;
				}
				break;
			case CV_AUTH_SESSION_HMAC:
			case CV_AUTH_SESSION:
				/* check length */
				if (authEntry->authLen != sizeof(cv_auth_data))
					goto err_exit;
				break;
			case CV_AUTH_COUNT:
				/* check length */
				if (authEntry->authLen != sizeof(cv_auth_entry_count))
					goto err_exit;
				break;
			case CV_AUTH_TIME:
				/* check length */
				if (authEntry->authLen != sizeof(cv_auth_entry_time))
					goto err_exit;
				break;
			case CV_AUTH_CONTACTLESS:
				/* check type  */
				HIDEntryHandle = (cv_auth_entry_contactless_handle *)(authEntry + 1);
				if (!(HIDEntryHandle->type == CV_CONTACTLESS_TYPE_HID || HIDEntryHandle->type == CV_CONTACTLESS_TYPE_PIV
					|| HIDEntryHandle->type == CV_CONTACTLESS_TYPE_MIFARE_CLASSIC || HIDEntryHandle->type == CV_CONTACTLESS_TYPE_MIFARE_ULTRALIGHT
					|| HIDEntryHandle->type == CV_CONTACTLESS_TYPE_MIFARE_DESFIRE || HIDEntryHandle->type == CV_CONTACTLESS_TYPE_FELICA 
					|| HIDEntryHandle->type == CV_CONTACTLESS_TYPE_ISO15693))
					goto err_exit;
				if (HIDEntryHandle->constZero == 0)
				{
					/* entry contains handle */
					if (authEntry->authLen != sizeof(cv_auth_entry_contactless_handle))
						goto err_exit;
					objProperties.objHandle = HIDEntryHandle->hHIDObj;
					if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
						goto err_exit;
				} else {
					/* credential directly contained */
					HIDEntry = (cv_auth_entry_contactless *)(authEntry + 1);
					if (authEntry->authLen != (sizeof(cv_auth_entry_contactless) - 1 + HIDEntry->length))
						goto err_exit;
				}
				break;
			case CV_AUTH_SMARTCARD:
				SCEntryHandle = (cv_auth_entry_smartcard_handle *)(authEntry + 1);
				if (SCEntryHandle->constZero == 0)
				{
					/* entry contains handle */
					if (authEntry->authLen != sizeof(cv_auth_entry_smartcard_handle))
						goto err_exit;
					objProperties.objHandle = SCEntryHandle->keyObj;
					if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
						goto err_exit;
				} else {
					/* key directly contained */
					SCEntry = (cv_auth_entry_smartcard *)(authEntry + 1);
					key = &SCEntry->key;
					switch(SCEntry->type)
					{
					case CV_TYPE_RSA_KEY:
						if (!(key->keyLen == 256 || key->keyLen == 512 || key->keyLen == 768. || key->keyLen == 1024 ||
							key->keyLen == 2048))
							goto err_exit;
						keyLenBytes = key->keyLen/8;   /* pubKey length */
						if (authEntry->authLen != (sizeof(cv_auth_entry_smartcard) - 1 + keyLenBytes))
							goto err_exit;
						break;
					case CV_TYPE_ECC_KEY:
						if (!(key->keyLen == 192 || key->keyLen == 224 || key->keyLen == 256 || key->keyLen == 384))
							goto err_exit;
						keyLenBytes = key->keyLen/8;   /* pubKey length */
						if (authEntry->authLen != (sizeof(cv_auth_entry_smartcard) - 1 + 9*keyLenBytes))
							goto err_exit;
						break;
					default:
						goto err_exit;
					}
				}
				break;
			case CV_AUTH_PROPRIETARY_SC:
				/* check size of entry */
				propSCEntry = (cv_auth_entry_proprietary_sc *)(authEntry + 1);
				if (authEntry->authLen != sizeof(cv_auth_entry_proprietary_sc) - 1 + SHA1_LEN)
					goto err_exit;
				if (propSCEntry->hash.hashType != CV_SHA1)
					goto err_exit;
				break;
			case CV_AUTH_CHALLENGE:
				challengeEntryHandle = (cv_auth_entry_challenge_handle_obj *)(authEntry + 1);
				if (challengeEntryHandle->constZero == 0)
				{
					/* entry contains handle */
					if (authEntry->authLen != sizeof(cv_auth_entry_challenge_handle_obj))
						goto err_exit;
					objProperties.objHandle = challengeEntryHandle->keyObj;
					if (cvDetermineObjAvail(&objProperties) == CV_INVALID_OBJECT_HANDLE)
						goto err_exit;
				} else {
					/* key directly contained */
					challengeEntry = (cv_auth_entry_challenge_obj *)(authEntry + 1);
					key = &challengeEntry->key;
					switch(challengeEntry->type)
					{
					case CV_TYPE_RSA_KEY:
						if (!(key->keyLen == 256 || key->keyLen == 512 || key->keyLen == 768. || key->keyLen == 1024 ||
							key->keyLen == 2048))
							goto err_exit;
						keyLenBytes = key->keyLen/8;   /* pubKey length */
						if (authEntry->authLen != (sizeof(cv_auth_entry_challenge_obj) - 1 + keyLenBytes))
							goto err_exit;
						break;
					case CV_TYPE_ECC_KEY:
						if (!(key->keyLen == 192 || key->keyLen == 224 || key->keyLen == 256 || key->keyLen == 384))
							goto err_exit;
						keyLenBytes = key->keyLen/8;   /* pubKey length */
						if (authEntry->authLen != (sizeof(cv_auth_entry_challenge_obj) - 1 + 9*keyLenBytes))
							goto err_exit;
						break;
					default:
						goto err_exit;
					}
				}
				break;
#ifdef ST_AUTH
			case CV_AUTH_ST_CHALLENGE:
				/* check size of entry */
				if (authEntry->authLen != sizeof(cv_auth_entry_st_response_obj))
					goto err_exit;
				break;
#endif
			case CV_AUTH_SUPER:
				/* check size of entry */
				if (authEntry->authLen != 0)
					goto err_exit;
				break;
			case CV_AUTH_CAC_PIV:
				/* check size of entry and contents */
				CACPIVEntry = (cv_auth_entry_cac_piv_obj *)(authEntry + 1);
				if (CACPIVEntry->cardType != CV_TYPE_CAC && CACPIVEntry->cardType != CV_TYPE_PIV)
					goto err_exit;
				if (!isValidDate(CACPIVEntry->expirationDate.month, CACPIVEntry->expirationDate.day,
						CACPIVEntry->expirationDate.year))
					goto err_exit;
				if (CACPIVEntry->cardSignAlg == CVSC_RSA_1024)
					keyLenBytes = RSA2048_KEY_LEN/2;
				else if (CACPIVEntry->cardSignAlg == CVSC_RSA_2048)
					keyLenBytes = RSA2048_KEY_LEN;
				else if (CACPIVEntry->cardSignAlg == CVSC_ECC_256)
					keyLenBytes = 2*ECC256_KEY_LEN;
				else
					goto err_exit;
				if (authEntry->authLen != (sizeof(cv_auth_entry_cac_piv_obj) - 1 + keyLenBytes))
					goto err_exit;
				break;
			case CV_AUTH_PIV_CTLS:
				/* check size of entry and contents */
				PIVCtlsEntry = (cv_auth_entry_piv_ctls_obj *)(authEntry + 1);
				if (authEntry->authLen != sizeof(cv_auth_entry_piv_ctls_obj))
					goto err_exit;
				if (!isValidDate(PIVCtlsEntry->expirationDate.month, PIVCtlsEntry->expirationDate.day,
						PIVCtlsEntry->expirationDate.year))
					goto err_exit;
				break;
			case CV_AUTH_PIN:
				/* check size of entry.  also, if multiple auth lists, can only appear in 1st one */
				if (authListCount != 1)
					goto err_exit;
				PINEntry = (cv_auth_entry_PIN *)(authEntry + 1);
				if (authEntry->authLen != (sizeof(cv_auth_entry_PIN) - 1 + PINEntry->PINLen))
					goto err_exit;
				break;
			default:
				goto err_exit;
			case 0xffff:
				break;
			}
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntry > authListsEnd)
			{
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				goto err_exit;
			}
		}
		authListCount++;
		authListPtr = (cv_auth_list *)authEntry;
	} while ((uint8_t *)authListPtr < authListsEnd);

	return CV_SUCCESS;

err_exit:
	return CV_INVALID_AUTH_LIST;
}

#ifdef ST_AUTH
void
cvZeroSTTable( cv_auth_list *toAuthLists, uint32_t authListsLength)
{
	/* this routine determines of auth lists contains an ST table and if so, zeros it because */
	/* ST tables are never allowed to be read out */
	uint32_t i;
	cv_auth_list *authListPtrOut;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntryOut;

	/* now parse auth lists and check each entry */
	authListPtrOut = toAuthLists;
	authListsEnd = (uint8_t *)authListPtrOut + authListsLength;
	do
	{
		authEntryOut = (cv_auth_hdr *)(authListPtrOut + 1);
		for (i=0;i<authListPtrOut->numEntries;i++)
		{
			switch (authEntryOut->authType)
			{
			case CV_AUTH_ST_CHALLENGE:
				/* zero ST table */
				memset(authEntryOut + 1,0,authEntryOut->authLen);
				break;
			case 0xffff:
				break;
			default:
				break;
			}
			authEntryOut = (cv_auth_hdr *)((uint8_t *)authEntryOut + authEntryOut->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntryOut > authListsEnd)
			{
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				goto err_exit;
			}
		}
		authListPtrOut = (cv_auth_list *)authEntryOut;
	} while ((uint8_t *)authListPtrOut < authListsEnd);

err_exit:
	return;
}
#endif

cv_bool
cvHasIndicatedAuth( cv_auth_list *toAuthLists, uint32_t authListsLength, uint32_t authFlags)
{
	/* this routine checks the auth lists to determine if there is write authorization */
	uint32_t i;
	cv_auth_list *authListPtrOut;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntryOut;
	uint32_t authFlagsNotFound = authFlags;
	cv_bool hasIndicatedAuth = FALSE;

	/* now parse auth lists and check each entry */
	authListPtrOut = toAuthLists;
	authListsEnd = (uint8_t *)authListPtrOut + authListsLength;
	do
	{
		/* check to see if indicated flag is set and there's at least one auth entry in the list */
		if ((authListPtrOut->flags & authFlags) && authListPtrOut->numEntries)
		{
			/* clear this flag from flags not found and see if any still remain to be found */
			authFlagsNotFound &= ~authListPtrOut->flags;
			if (!authFlagsNotFound)
			{
				hasIndicatedAuth = TRUE;
				break;
			}
		}
		authEntryOut = (cv_auth_hdr *)(authListPtrOut + 1);
		for (i=0;i<authListPtrOut->numEntries;i++)
		{
			authEntryOut = (cv_auth_hdr *)((uint8_t *)authEntryOut + authEntryOut->authLen + sizeof(cv_auth_hdr));
		}
		authListPtrOut = (cv_auth_list *)authEntryOut;
	} while ((uint8_t *)authListPtrOut < authListsEnd);

	return hasIndicatedAuth;
}
void
cvCopyAuthList(cv_auth_list *toAuthList, cv_auth_list *fromAuthList, uint32_t *len)
{
	/* this routine copies an auth list */
	uint32_t i;
	cv_auth_list *authListPtr, *authListPtrOut;
	cv_auth_hdr *authEntry, *authEntryOut;

	/* now parse auth lists and check each entry */
	authListPtr = fromAuthList;
	authListPtrOut = toAuthList;
	authEntry = (cv_auth_hdr *)(authListPtr + 1);
	authEntryOut = (cv_auth_hdr *)(authListPtrOut + 1);
	/* copy auth list header */
	*authListPtrOut = *authListPtr;
	for (i=0;i<authListPtr->numEntries;i++)
	{
		/* copy auth entry */
		*authEntryOut = *authEntry;
		memcpy(authEntryOut + 1, authEntry + 1, authEntry->authLen);
		authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
		authEntryOut = (cv_auth_hdr *)((uint8_t *)authEntryOut + authEntryOut->authLen + sizeof(cv_auth_hdr));
	}
	*len = (uint8_t *)authEntry - (uint8_t *)authListPtr;
}

void
cvCopyAuthListsUnauthorized( cv_auth_list *toAuthLists, cv_auth_list *fromAuthLists, uint32_t authListsLength)
{
	/* this routine copies an auth list, but zeroes secret fields */
	uint32_t i;
	cv_auth_list *authListPtr, *authListPtrOut;
	uint8_t *authListsEnd;
	cv_auth_hdr *authEntry, *authEntryOut;
	cv_auth_entry_passphrase *passphraseEntry, *passphraseEntryOut;
	cv_auth_entry_passphrase_HMAC_obj *passphraseEntryHMAC, *passphraseEntryHMACOut;
	cv_auth_entry_passphrase_handle *passphraseHandleEntry;
	cv_auth_entry_proprietary_sc *propSCEntry, *propSCEntryOut;
	cv_auth_entry_cac_piv_obj *CACPIVEntry, *CACPIVEntryOut;
	cv_auth_entry_piv_ctls_obj *PIVCtlsEntry, *PIVCtlsEntryOut;
	uint32_t keyLenBytes;

	/* now parse auth lists and check each entry */
	authListPtr = fromAuthLists;
	authListPtrOut = toAuthLists;
	authListsEnd = (uint8_t *)authListPtr + authListsLength;
	do
	{
		authEntry = (cv_auth_hdr *)(authListPtr + 1);
		authEntryOut = (cv_auth_hdr *)(authListPtrOut + 1);
		/* copy auth list header */
		*authListPtrOut = *authListPtr;
		for (i=0;i<authListPtr->numEntries;i++)
		{
			/* copy auth entry header */
			*authEntryOut = *authEntry;
			switch (authEntry->authType)
			{
			case CV_AUTH_PASSPHRASE_HMAC:
				passphraseEntryHMAC = (cv_auth_entry_passphrase_HMAC_obj *)(authEntry + 1);
				passphraseEntryHMACOut = (cv_auth_entry_passphrase_HMAC_obj *)(authEntryOut + 1);
				/* zero HMAC key */
				passphraseEntryHMACOut->hash.hashType = passphraseEntryHMAC->hash.hashType;
				if (passphraseEntryHMACOut->hash.hashType == CV_SHA1)
					memset(&passphraseEntryHMACOut->hash.hash[0],0,SHA1_LEN);
				else
					memset(&passphraseEntryHMACOut->hash.hash[0],0,SHA256_LEN);
				break;
			case CV_AUTH_PASSPHRASE:
				passphraseHandleEntry = (cv_auth_entry_passphrase_handle *)(authEntry + 1);
				passphraseEntry = (cv_auth_entry_passphrase *)(authEntry + 1);
				passphraseEntryOut = (cv_auth_entry_passphrase *)(authEntryOut + 1);
				/* first check to see if entry contains a handle to object containing passphrase */
				if (passphraseHandleEntry->constZero == 0)
					/* handle, just copy */
					memcpy(authEntryOut + 1, authEntry + 1, authEntry->authLen);
				else {
					/* passphrase, zero */
					passphraseEntryOut->passphraseLen = passphraseEntry->passphraseLen;
					memset(&passphraseEntryOut->passphrase,0,passphraseEntry->passphraseLen);
				}
				break;
			case CV_AUTH_SESSION_HMAC:
			case CV_AUTH_SESSION:
				/* zero output */
				memset(authEntryOut + 1,0,authEntry->authLen);
				break;
			case CV_AUTH_PROPRIETARY_SC:
				/* zero output */
				propSCEntry = (cv_auth_entry_proprietary_sc *)(authEntry + 1);
				propSCEntryOut = (cv_auth_entry_proprietary_sc *)(authEntryOut + 1);
				propSCEntryOut->hash.hashType = propSCEntry->hash.hashType;
				memset(&propSCEntryOut->hash.hash,0,SHA1_LEN);
				break;
			case CV_AUTH_CAC_PIV:
				/* zero all output except type and signature alg */
				CACPIVEntry = (cv_auth_entry_cac_piv_obj *)(authEntry + 1);
				CACPIVEntryOut = (cv_auth_entry_cac_piv_obj *)(authEntryOut + 1);
				CACPIVEntryOut->cardType = CACPIVEntry->cardType;
				CACPIVEntryOut->cardSignAlg = CACPIVEntry->cardSignAlg;
				memset(&CACPIVEntryOut->expirationDate,0,sizeof(cv_date));
				if (CACPIVEntry->cardSignAlg == CVSC_RSA_1024)
					keyLenBytes = RSA2048_KEY_LEN/2;
				else if (CACPIVEntry->cardSignAlg == CVSC_RSA_2048)
					keyLenBytes = RSA2048_KEY_LEN;
				else
					keyLenBytes = 2*ECC256_KEY_LEN;
				memset(&CACPIVEntryOut->pubkey,0,keyLenBytes);
				break;
			case CV_AUTH_PIV_CTLS:
				/* zero all output except salt */
				PIVCtlsEntry = (cv_auth_entry_piv_ctls_obj *)(authEntry + 1);
				PIVCtlsEntryOut = (cv_auth_entry_piv_ctls_obj *)(authEntryOut + 1);
				memcpy(&PIVCtlsEntryOut->salt, &PIVCtlsEntry->salt, sizeof(PIVCtlsEntryOut->salt));
				memset(&PIVCtlsEntryOut->hCHUID,0,sizeof(PIVCtlsEntryOut->hCHUID));
				memset(&PIVCtlsEntryOut->expirationDate,0,sizeof(cv_date));
				break;
#ifdef ST_AUTH
			case CV_AUTH_ST_CHALLENGE:
				/* zero ST table */
				memset(authEntryOut + 1,0,authEntry->authLen);
				break;
#endif
			case 0xffff:
				break;
			default:
				/* just copy */
				memcpy(authEntryOut + 1, authEntry + 1, authEntry->authLen);
			}
			authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
			authEntryOut = (cv_auth_hdr *)((uint8_t *)authEntryOut + authEntryOut->authLen + sizeof(cv_auth_hdr));
			if ((uint8_t *)authEntry > authListsEnd)
			{
				/* shouldn't get here.  something must be wrong with auth list, just bail  */
				goto err_exit;
			}
		}
		authListPtr = (cv_auth_list *)authEntry;
		authListPtrOut = (cv_auth_list *)authEntryOut;
	} while ((uint8_t *)authListPtr < authListsEnd);

err_exit:
	return;
}

cv_status
cvValidateObjWithExistingAuth(uint32_t authListLength, cv_auth_list *pAuthList, 
							  uint32_t authListsLengthRef, cv_auth_list *pAuthListsRef,
							  uint32_t authListsLength, cv_auth_list *pAuthLists,
							  uint32_t altFlags, cv_bool *needObjWrite)
{
	/* this routine uses an input parameter auth list which has already validated against one object and checks to find the */
	/* corresponding auth list in another object.  If found, success is returned, otherwise, auth failure is returned */
	cv_status retVal = CV_SUCCESS;
	uint32_t i;
	cv_auth_list *authListRef, *authList;
	cv_auth_hdr *authEntry, *authEntryRef;

	/* first check to see if auth is needed (object may not require auth */
	if (needObjWrite != NULL)
		*needObjWrite = FALSE;
	if (!authListsLength)
		return CV_SUCCESS;

	/* check to see if no auth provided, but needed */
	if (!authListLength)
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* get both auth lists, then compare */
	if (!cvFindObjAuthList(pAuthList, pAuthListsRef, authListsLengthRef, &authListRef))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	if (!cvFindObjAuthList(pAuthList, pAuthLists, authListsLength, &authList))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* now compare lists */
	/* compare header up to flags (flags checked below) */
	if (memcmp(authListRef, authList, sizeof(cv_auth_list_hdr) - sizeof(cv_obj_auth_flags)))
	{
		/* list header doesn't match, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* now check flags.  if altFlags non-zero, then that flag should be compared, otherwise reference flags should be compared. */
	if (altFlags)
	{
		if (!(authList->flags & altFlags))
		{
			/* auth flags don't match, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	} else {
		if (authListRef->flags != authList->flags)
		{
			/* auth flags don't match, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	authEntryRef = (cv_auth_hdr *)(authListRef + 1);
	authEntry = (cv_auth_hdr *)(authList + 1);
	for (i=0;i<authListRef->numEntries;i++)
	{
		/* compare auth entry header */
		if (memcmp(authEntryRef, authEntry, sizeof(cv_auth_hdr)))
		{
			/* entry header doesn't match, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		switch (authEntry->authType)
		{
		case CV_AUTH_COUNT:
			/* count is a special case, because count values may not match */
			retVal = cvAuthCount(authEntry);
			if (retVal == CV_OBJECT_UPDATE_REQUIRED)
			{
				if (needObjWrite != NULL)
					*needObjWrite = TRUE;
				retVal = CV_SUCCESS;
			}
			break;
		case 0xffff:
			break;
		default:
			/* just compare */
			if (memcmp(authEntryRef + 1, authEntry + 1, authEntryRef->authLen))
			{
				retVal = CV_AUTH_FAIL;
				goto err_exit;
			}
		}
		authEntryRef = (cv_auth_hdr *)((uint8_t *)authEntryRef + authEntryRef->authLen + sizeof(cv_auth_hdr));
		authEntry = (cv_auth_hdr *)((uint8_t *)authEntry + authEntry->authLen + sizeof(cv_auth_hdr));
	}

err_exit:
	return retVal;
}
