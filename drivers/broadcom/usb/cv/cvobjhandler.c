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
 * cvobjhandler.c:  CVAPI Object Management Handler
 */
/*
 * Revision History:
 *
 * 02/13/07 DPA Created.
*/
#include "cvmain.h"
#include "console.h"
#include "ushx_api.h"
#include "fp_spi_enum.h"

cv_status
cvValidateAttributes(uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes)
{
	/* this routine validates attributes */
	cv_obj_attributes *pAttrib;
	uint8_t *pAttribsEnd;
	cv_status retVal = CV_SUCCESS;
	cv_bool foundFlags = FALSE;
	cv_attrib_flags flags;

	/* now parse attribs and check each entry */
	pAttrib = pObjAttributes;
	pAttribsEnd = (uint8_t *)pObjAttributes + objAttributesLength;
	do
	{
		switch (pAttrib->attribType)
		{
		case CV_ATTRIB_TYPE_FLAGS:
			foundFlags = TRUE;
			if (pAttrib->attribLen > sizeof(uint32_t))
			{
				retVal = CV_OBJECT_ATTRIBUTES_INVALID;
				goto err_exit;
			}
			/* now check for any unsupported attributes */
			flags = *((cv_attrib_flags *)(pAttrib + 1));
			if (flags & (CV_ATTRIB_SC_STORAGE | CV_ATTRIB_CONTACTLESS_STORAGE))
			{
				retVal = CV_OBJECT_ATTRIBUTES_INVALID;
				goto err_exit;
			}
			/* disable host storage objects */
			if ((flags & CV_ATTRIB_PERSISTENT) && !(flags & CV_ATTRIB_NVRAM_STORAGE))
			{
				retVal = CV_OBJECT_ATTRIBUTES_INVALID;
				goto err_exit;
			}
			break;
		case CV_ATTRIB_TYPE_LABEL:
			if (pAttrib->attribLen > MAX_ATTRIB_TYPE_LABEL_LENGTH)
			{
				retVal = CV_OBJECT_ATTRIBUTES_INVALID;
				goto err_exit;
			}
			break;
		default:
			retVal = CV_OBJECT_ATTRIBUTES_INVALID;
			goto err_exit;
		}
		pAttrib = (cv_obj_attributes *)((uint8_t *)pAttrib + pAttrib->attribLen + sizeof(cv_obj_attributes));
		if ((uint8_t *)pAttrib > pAttribsEnd)
		{
			/* shouldn't get here.  something must be wrong with attribs, just bail  */
			retVal = CV_OBJECT_ATTRIBUTES_INVALID;
			goto err_exit;
		}
	} while ((uint8_t *)pAttrib < pAttribsEnd);
	/* check to make sure flags attributes found, since required */
	if (!foundFlags)
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;

err_exit:
	return retVal;
}

cv_status
cvValidateObjectValue(cv_obj_type objType, uint32_t objValueLength, void *pObjValue)
{
	/* this routine validates object value */
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_certificate *certValue;
	cv_obj_value_aes_key *aesKey;
	cv_obj_value_opaque *opaqueValue;
	cv_obj_value_passphrase *passphraseValue;
	cv_obj_value_hash *hashValue;
	cv_obj_value_rsa_token *rsaTokenValue;
	cv_obj_value_oath_token *oathTokenValue;
	cv_obj_value_key *key;
	cv_obj_value_auth_session *authSession;
	cv_obj_value_fingerprint *fingerprintValue;
	cv_obj_value_hotp *hotpValue;
	uint32_t keyLenBytes;

	switch (objType)
	{
	case CV_TYPE_CERTIFICATE:
		certValue = (cv_obj_value_certificate *)pObjValue;
		if ((certValue->certLen + sizeof(cv_obj_value_certificate) - 1) != objValueLength)
			retVal = CV_OBJECT_NOT_VALID;
		break;
	case CV_TYPE_AES_KEY:
		aesKey = (cv_obj_value_aes_key *)pObjValue;
		switch (aesKey->keyLen)
		{
		case 128:
		case 192:
		case 256:
			keyLenBytes = aesKey->keyLen/8;
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		if ((keyLenBytes + sizeof(cv_obj_value_aes_key) - 1) != objValueLength)
			retVal = CV_OBJECT_NOT_VALID;
		break;
	case CV_TYPE_OPAQUE:
		opaqueValue = (cv_obj_value_opaque *)pObjValue;
		if ((opaqueValue->valueLen + sizeof(cv_obj_value_opaque) - 1) != objValueLength)
			retVal = CV_OBJECT_NOT_VALID;
		break;
	case CV_TYPE_PASSPHRASE:
		passphraseValue = (cv_obj_value_passphrase *)pObjValue;
		if ((passphraseValue->passphraseLen + sizeof(cv_obj_value_passphrase) - 1) != objValueLength)
			retVal = CV_OBJECT_NOT_VALID;
		break;
	case CV_TYPE_HASH:
		hashValue = (cv_obj_value_hash *)pObjValue;
		switch (hashValue->hashType)
		{
		case CV_SHA1:
			if ((SHA1_LEN + sizeof(cv_obj_value_hash) - 1) != objValueLength)
				retVal = CV_OBJECT_NOT_VALID;
			break;
		case CV_SHA256:
			if ((SHA256_LEN + sizeof(cv_obj_value_hash) - 1) != objValueLength)
				retVal = CV_OBJECT_NOT_VALID;
			break;
		default:
				retVal = CV_OBJECT_NOT_VALID;
		}
		break;
	case CV_TYPE_OTP_TOKEN:
		rsaTokenValue = (cv_obj_value_rsa_token *)pObjValue;
		switch (rsaTokenValue->type)
		{
		case CV_OTP_TYPE_RSA:
			if (sizeof(cv_obj_value_rsa_token) != objValueLength || rsaTokenValue->sharedSecretLen != AES_128_BLOCK_LEN)
				retVal = CV_OBJECT_NOT_VALID;
			break;
		case CV_OTP_TYPE_OATH:
			oathTokenValue = (cv_obj_value_oath_token *)pObjValue;
			if (sizeof(cv_obj_value_oath_token) != objValueLength || oathTokenValue->sharedSecretLen != CV_NONCE_LEN)
				retVal = CV_OBJECT_NOT_VALID;
			break;
		default:
				retVal = CV_OBJECT_NOT_VALID;
		}
		break;
	case CV_TYPE_RSA_KEY:
		key = (cv_obj_value_key *)pObjValue;
		switch (key->keyLen)
		{
		case 256:
		case 512:
		case 768:
		case 1024:
		case 2048:
			keyLenBytes = key->keyLen/8;
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		if ((keyLenBytes + sizeof(uint32_t) + 5*keyLenBytes/2 + sizeof(cv_obj_value_key) - 1) != objValueLength)
		{
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		break;
	case CV_TYPE_DH_KEY:
		key = (cv_obj_value_key *)pObjValue;
		switch (key->keyLen)
		{
		case 1024:
		case 2048:
			keyLenBytes = key->keyLen/8;
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		if ((4*keyLenBytes + sizeof(cv_obj_value_key) - 1) != objValueLength)
		{
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		break;
	case CV_TYPE_DSA_KEY:
		key = (cv_obj_value_key *)pObjValue;
		switch (key->keyLen)
		{
		case 1024:
		case 2048:
			keyLenBytes = key->keyLen/8;
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		if ((3*keyLenBytes + 2*((key->keyLen == 1024) ? DSA1024_PRIV_LEN : DSA2048_PRIV_LEN) + sizeof(cv_obj_value_key) - 1) != objValueLength)
		{
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		break;
	case CV_TYPE_ECC_KEY:
		key = (cv_obj_value_key *)pObjValue;
		switch (key->keyLen)
		{
		case 192:
		case 224:
		case 256:
		case 384:
			keyLenBytes = key->keyLen/8;
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		if ((10*keyLenBytes + sizeof(cv_obj_value_key) - 1) != objValueLength)
		{
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		break;
	case CV_TYPE_HOTP:
		hotpValue = (cv_obj_value_hotp *)pObjValue;
		switch (hotpValue->numberOfDigits)
		{
		case 6:
		case 8:
		case 10:
			break;
		default:
			retVal = CV_OBJECT_NOT_VALID;
		}
		if (sizeof(cv_obj_value_hotp) != objValueLength)
		{
			retVal = CV_OBJECT_NOT_VALID;
		}
		if (hotpValue->randomKeyLength == 0 || hotpValue->counterInterval == 0 || hotpValue->randomKeyLength > HOTP_KEY_MAX_LEN)
		{
			retVal = CV_OBJECT_NOT_VALID;
		}
		break;
	case CV_TYPE_FINGERPRINT:
		fingerprintValue = (cv_obj_value_fingerprint *)pObjValue;
		if (fingerprintValue->type > CV_FINGERPRINT_TEMPLATE_SYNAPTICS)
			retVal = CV_OBJECT_NOT_VALID;
		if ((fingerprintValue->fpLen + sizeof(cv_fingerprint_type) + sizeof(uint16_t)) != objValueLength)
			retVal = CV_OBJECT_NOT_VALID;
		break;
	case CV_TYPE_AUTH_SESSION:
		authSession = (cv_obj_value_auth_session *)pObjValue;
		/* check to see if auth session object has shared secret */
		if (authSession->flags & CV_AUTH_SESSION_CONTAINS_SHARED_SECRET)
		{
			hashValue = (cv_obj_value_hash *)(authSession + 1);
			if ((sizeof(cv_obj_value_auth_session) + ((hashValue->hashType == CV_SHA1) ? SHA1_LEN : SHA256_LEN) 
				+ sizeof(cv_obj_value_hash) - 1 + sizeof(uint32_t)) != objValueLength)
			{
				retVal = CV_OBJECT_NOT_VALID;
				goto err_exit;
			}
		} else {
			/* no shared secret */
			if (sizeof(cv_obj_value_auth_session) != objValueLength)
			{
				retVal = CV_OBJECT_NOT_VALID;
				goto err_exit;
			}
		}
		/* now check flag settings */
		if ((authSession->statusLSH & ~CV_OBJ_AUTH_FLAGS_MASK) || authSession->statusMSH || (authSession->flags & ~CV_AUTH_SESSION_FLAGS_MASK))
		{
			retVal = CV_OBJECT_NOT_VALID;
			goto err_exit;
		}
		break;
	case 0xffff:
		break;
	default:
		retVal = CV_INVALID_OBJECT_TYPE;
	}
err_exit:
	return retVal;
}

cv_status
cv_create_object (cv_handle cvHandle, cv_obj_type objType,           
				  uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
				  uint32_t authListsLength, cv_auth_list *pAuthLists,           
				  uint32_t objValueLength, void *pObjValue,              
				  cv_obj_handle *pObjHandle,
				  cv_callback *callback, cv_callback_ctx context)
{
	/* This routine takes the associated parameters and formats the indicated type of object.  */
	/* Then, cvPutObj is called to store the object and return the new handle. */
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_auth_entry_PIN *PIN;
	cv_auth_hdr *PINhdr;
	uint32_t pinLen, remLen;
	uint8_t obj[MAX_CV_OBJ_SIZE];
	uint8_t *pObjValLocal = &obj[0];
	cv_attrib_flags *flags;
	cv_obj_storage_attributes attributes;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;
	/* validate object contents lists provided */
	if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
		goto err_exit;
	if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
		goto err_exit;
	if ((retVal = cvValidateObjectValue(objType, objValueLength, pObjValue)) != CV_SUCCESS)
		goto err_exit;

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	objProperties.objAttributesLength = objAttributesLength;
	objProperties.pObjAttributes = pObjAttributes;
	objProperties.authListsLength = authListsLength;
	objProperties.pAuthLists = pAuthLists;
	objProperties.objValueLength = objValueLength;
	objProperties.pObjValue = pObjValue;
	/* check to make sure object not too large */
	if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_OBJECT_SIZE;
		goto err_exit;
	}
	objProperties.objHandle = 0;
	objPtr.objType = objType;
	/* copy object value locally, and do any format coversion required by keys */
	cvCopyObjValue(objType, objValueLength, pObjValLocal, pObjValue, TRUE, FALSE);
	objProperties.pObjValue = pObjValLocal;
	/* search to see if PIN supplied in auth list.  it must be in the first auth */
	/* list, if present.  this PIN is used to access the destination device for storing the object */
	/* and is removed from the auth list which is stored with the object */
	if (authListsLength && (PIN = cvFindPINAuthEntry(pAuthLists, &objProperties)) != NULL)
	{
		/* PIN found.  now remove from auth list */
		PINhdr = (cv_auth_hdr *)((uint8_t *)PIN - sizeof(cv_auth_hdr));
		pinLen = PIN->PINLen + sizeof(cv_auth_entry_PIN) - sizeof(uint8_t);
		remLen = authListsLength - (uint32_t)((uint8_t *)PINhdr - (uint8_t *)pAuthLists);
		memmove(PINhdr, (uint8_t *)PIN + pinLen, remLen);
		objProperties.authListsLength -= (pinLen + sizeof(cv_auth_hdr));
		/* now decrement auth list entry count */
		pAuthLists->numEntries--;
	}
	objPtr.objLen = cvComputeObjLen(&objProperties);
	/* check to see if this is an auth session object.  if so, don't allow host hard drive storage */
	if (objType == CV_TYPE_AUTH_SESSION)
	{
		if ((retVal = cvFindObjAtribFlags(&objProperties, &objPtr, &flags, &attributes)) != CV_SUCCESS)
			goto err_exit;
		if (attributes.storageType == CV_STORAGE_TYPE_HARD_DRIVE)
		{
			retVal = CV_OBJECT_ATTRIBUTES_INVALID;
			goto err_exit;
		}
	}

	/* now save object */
	retVal = cvHandlerPostObjSave(&objProperties, &objPtr, callback, context);
	*pObjHandle = objProperties.objHandle;

err_exit:
	return retVal;

}

cv_status
cvCopyObjValue(cv_obj_type objType, uint32_t lenObjValue, void *pObjValueOut, void *pObjValue, cv_bool allowPriv, cv_bool omitPriv)
{
	/* this routine copies an object value to destination.  if private fields not allowed, those are zeroed. */
	/* also, appropriate format conversion between BIGINT and big endian byte stream done (since this is a */
	/* copy from CV internal to external, or vice versa */
	uint32_t keyLenBytes, privKeyLenBytes;
	uint8_t *pIn = (uint8_t *)pObjValue, *pOut = (uint8_t *)pObjValueOut;
	cv_obj_value_key *keyIn = (cv_obj_value_key *)pIn, *keyOut = (cv_obj_value_key *)pOut;
	uint32_t i;
	cv_obj_value_auth_session *authSessionIn, *authSessionOut;
	uint32_t privLen;
	cv_status retVal = CV_SUCCESS;

	switch (objType)
	{
	case CV_TYPE_CERTIFICATE:
	case CV_TYPE_AES_KEY:
	case CV_TYPE_OPAQUE:
	case CV_TYPE_PASSPHRASE:
	case CV_TYPE_HASH:
	case CV_TYPE_OTP_TOKEN:
	case CV_TYPE_HOTP:
	case CV_TYPE_FINGERPRINT:
		/* no pub/priv area.  consider all private */
		if (allowPriv)
			memcpy(pObjValueOut, pObjValue, lenObjValue); 
		else if (omitPriv)
			/* can't copy any of object value, this is a failure case */
			retVal = CV_AUTH_FAIL;
		else
			memset(pObjValueOut,0,lenObjValue);
		break;
	case CV_TYPE_RSA_KEY:
		keyOut->keyLen = keyIn->keyLen;
		pIn += sizeof(keyIn->keyLen);
		pOut += sizeof(keyIn->keyLen);
		keyLenBytes = keyIn->keyLen / 8;
		/* copy out the public fields */
		/* public modulus */
		cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
		pIn += keyLenBytes;
		pOut += keyLenBytes;
		/* public exponent */
		*((uint32_t *)pOut) = *((uint32_t *)pIn);
		cvByteSwap(pOut);
		pIn += sizeof(uint32_t);
		pOut += sizeof(uint32_t);
		/* now check to see if private portion authorized */
		if (allowPriv)
		{
			/* yes, copy out private portion too p, q, pinv, dp, dq */
			for (i=0;i<5;i++)
			{
				cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes/2);
				pIn += keyLenBytes/2;
				pOut += keyLenBytes/2;
			}
		} else if (!omitPriv)
			/* zero private portion of output */
			memset(pOut,0,5*(keyLenBytes/2));
		break;
	case CV_TYPE_DH_KEY:
		keyOut->keyLen = keyIn->keyLen;
		pIn += sizeof(keyIn->keyLen);
		pOut += sizeof(keyIn->keyLen);
		keyLenBytes = keyIn->keyLen / 8;
		/* copy out the public fields: public key, public generator, public modulus */
		for (i=0;i<3;i++)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		}
		/* now check to see if private portion authorized */
		if (allowPriv)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		} else if (!omitPriv)
			/* zero private portion of output */
			memset(pOut,0,keyLenBytes);
		break;
	case CV_TYPE_DSA_KEY:
		keyOut->keyLen = keyIn->keyLen;
		pIn += sizeof(keyIn->keyLen);
		pOut += sizeof(keyIn->keyLen);
		keyLenBytes = keyIn->keyLen / 8;
		if (keyIn->keyLen == DSA1024_KEY_LEN_BITS)
			privKeyLenBytes = DSA1024_PRIV_LEN;
		else
			/* only 1024 and 2048 bit keys allow; validation already done */
			privKeyLenBytes = DSA2048_PRIV_LEN;
		/* copy out the public modulus */
		cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
		pIn += keyLenBytes;
		pOut += keyLenBytes;
		/* copy public q field */
		cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, privKeyLenBytes);
		pIn += privKeyLenBytes;
		pOut += privKeyLenBytes;
		/* now copy public key and generator */
		for (i=0;i<2;i++)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		}
		/* now check to see if private portion authorized */
		if (allowPriv)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, privKeyLenBytes);
			pIn += privKeyLenBytes;
			pOut += privKeyLenBytes;
		} else if (!omitPriv)
			/* zero private portion of output */
			memset(pOut,0,privKeyLenBytes);
		break;
	case CV_TYPE_ECC_KEY:
		keyOut->keyLen = keyIn->keyLen;
		pIn += sizeof(keyIn->keyLen);
		pOut += sizeof(keyIn->keyLen);
		keyLenBytes = keyIn->keyLen / 8;
		/* copy out the public fields: p, a, b, n, h */
		for (i=0;i<5;i++)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		}
		/* copy out G and Q (separately swap x and y */
		for (i=0;i<4;i++)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		}
		/* now check to see if private portion authorized */
		if (allowPriv)
		{
			cvCopySwap((uint32_t *)pIn, (uint32_t *)pOut, keyLenBytes);
			pIn += keyLenBytes;
			pOut += keyLenBytes;
		} else if (!omitPriv)
			/* zero private portion of output */
			memset(pOut,0,keyLenBytes);
		break;
	case CV_TYPE_AUTH_SESSION:
		authSessionIn = (cv_obj_value_auth_session *)pIn;
		authSessionOut = (cv_obj_value_auth_session *)pOut;
		/* copy out public part */
		authSessionOut->statusLSH = authSessionIn->statusLSH;
		authSessionOut->statusMSH = authSessionIn->statusMSH;
		authSessionOut->flags = authSessionIn->flags;
		if (allowPriv)
		{
			authSessionOut->authData = authSessionIn->authData;
			pIn += sizeof(cv_obj_value_auth_session);
			pOut += sizeof(cv_obj_value_auth_session);
			memcpy(pOut, pIn, lenObjValue - sizeof(cv_obj_value_auth_session));
		} else if (!omitPriv) {
			privLen = sizeof(cv_obj_value_auth_session) - sizeof(cv_auth_data);
			memset(pOut + lenObjValue - privLen, 0, privLen);
		}
		break;
	case 0xffff:
		break;
	}
	return retVal;
}

cv_status
cv_get_object (cv_handle cvHandle, cv_obj_handle objHandle,           
				uint32_t authListLength, cv_auth_list *pAuthList, 
				cv_obj_hdr *objHeader,                                    
				uint32_t *pAuthListsLength, cv_auth_list *paramAuthLists,             
				uint32_t *pObjAttributesLength, cv_obj_attributes *paramObjAttributes, 
				uint32_t *pObjValueLength, void *paramObjValue,
				cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the object and performs the authorization.  If auth is successful */
	/* the corresponding portions of the indicated object are returned in the parameters provided */
	cv_auth_list *pAuthLists;
	cv_obj_attributes *pObjAttributes;
	uint8_t *pObjValue;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_bool doUnauthorizedGet = FALSE;

	pAuthLists = paramAuthLists;
	pObjAttributes = paramObjAttributes;
	pObjValue = (uint8_t *)paramObjValue;
	
	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	printf("cv_get_object: handle 0x%x\n",objHandle);
	cvSetupInputParameterPtrs(&inputParameters, 6, sizeof(objHandle), &objHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(uint32_t), pAuthListsLength,
		sizeof(uint32_t), pObjAttributesLength, sizeof(uint32_t), pObjValueLength,
		0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* check to see if this is the unauthorized read case */
	if (!authListLength)
		doUnauthorizedGet = TRUE;
	retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context);
	/* if auth failed, check to see if this is the case of unauthorized read, where obj header, attributes and auth lists */
	/* (with secret portions zeroed) if allow */
	if (doUnauthorizedGet)
	{
		/* if any other return value, just fail */
		if (!(retVal == CV_SUCCESS || retVal == CV_AUTH_FAIL))
			goto err_exit;
		retVal = CV_SUCCESS;
	} else {
		/* this is not the case of unauthorized read, need to check returned auth results */
		if (retVal != CV_SUCCESS)
			goto err_exit;

		/* auth succeeded, but need to make sure read auth granted */
		if (!((authFlags & CV_OBJ_AUTH_READ_PUB) || (authFlags & CV_OBJ_AUTH_READ_PRIV)))
		{
			/* read auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}
	/* now copy out fields, if requested and authorized */
	*objHeader = *objPtr;
	/* auth lists field */
	if (*pAuthListsLength)
	{
		/* check to see if adequate length provided */
		if (*pAuthListsLength < objProperties.authListsLength)
			/* field isn't long enough, just save required length */
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		else
		{
			/* check to see if unauthorized read.  if so, zero private fields on output */
			if (doUnauthorizedGet || !(authFlags & CV_OBJ_AUTH_READ_PRIV))
				cvCopyAuthListsUnauthorized(pAuthLists, objProperties.pAuthLists, objProperties.authListsLength);
			else {
				memcpy(pAuthLists, objProperties.pAuthLists, *pAuthListsLength);
#ifdef ST_AUTH
				/* make sure ST table is zeroed, because it is never allowed to be read */
				cvZeroSTTable(pAuthLists, objProperties.authListsLength);
#endif
			}
		}
		*pAuthListsLength = objProperties.authListsLength;
	}
	/* attributes field */
	if (*pObjAttributesLength)
	{
		if (*pObjAttributesLength < objProperties.objAttributesLength)
			/* field isn't long enough, just save required length */
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		else
			/* Update lengths and parameter pointers */
			memcpy(pObjAttributes, objProperties.pObjAttributes, *pObjAttributesLength);
		*pObjAttributesLength = objProperties.objAttributesLength;
	}
	/* object value field */
	/* check to see if requested and authorized to read private part of object */
	if (*pObjValueLength && !doUnauthorizedGet)
	{
		/* yes, check to see if adequate length provided */
		if (*pObjValueLength < objProperties.objValueLength)
			/* field isn't long enough, just save required length */
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		else
			/* copy public portion (and private portion if authorized) */
			cvCopyObjValue(objHeader->objType, *pObjValueLength, (void *)pObjValue, 
				objProperties.pObjValue, (authFlags & CV_OBJ_AUTH_READ_PRIV), FALSE);
		*pObjValueLength = objProperties.objValueLength;
	} else
		*pObjValueLength = 0;

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

void
cvDeleteVolatileAuthSessionObj(cv_obj_handle objHandle)
{
	/* this routine checks to see if there is a volatile object mirror of a flash auth session object, and deletes it if so */
	uint32_t index, volDirIndex;
	cv_obj_hdr *objPtr;
	cv_obj_storage_attributes objAttribs;

	/* check to see if there is a volatile object, if not, just exit */
	if (!cvFindVolatileDirEntry(objHandle, &volDirIndex))
		return;

	/* check to see if flash object exists */
	if (cvFindDir0Entry(objHandle, &index))
	{
		/* object found, determine where object resident. */
		objAttribs = CV_DIR_PAGE_0->dir0Objs[index].attributes;
		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH)
		{
			/* object persists in flash, delete volatile object */
			objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj;
			cv_free(CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj, objPtr->objLen);
			CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hObj = 0;
			/* make sure not still in cache also */
			cvFlushCacheEntry(objHandle);
		}
	}
}

cv_status
cv_set_object (cv_handle cvHandle, cv_obj_handle objHandle,           
				uint32_t authListLength, cv_auth_list *pAuthList, 
				uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
				uint32_t objValueLength, void *pObjValue,
				cv_callback *callback, cv_callback_ctx context)
{
	/* This routine sets the corresponding portions of the object (attributes or value), but not auth */
	cv_obj_properties objProperties, objPropertiesNew;
	cv_obj_hdr *objPtr, objHdr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	uint8_t obj[MAX_CV_OBJ_SIZE];
	cv_bool allowPriv = FALSE;
	cv_attrib_flags *pFlags_cur, *pFlags_new;
	cv_obj_storage_attributes objAttribs_cur, objAttribs_new;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(objHandle), &objHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(uint32_t), &objAttributesLength,
		objAttributesLength, pObjAttributes, sizeof(uint32_t), &objValueLength, objValueLength, pObjValue,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure write auth granted */
	if (!(authFlags & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)))
	{
		/* write auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* save obj attribs */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &pFlags_cur, &objAttribs_cur)) != CV_SUCCESS)
		goto err_exit;

	/* now compose object locally, using fields from existing object, where not provided in input param */
	memcpy(&objPropertiesNew, &objProperties, sizeof(objProperties));
	if (objAttributesLength != 0)
	{
		if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
			goto err_exit;
		objPropertiesNew.objAttributesLength = objAttributesLength;
		objPropertiesNew.pObjAttributes = pObjAttributes;
	}
	if (objValueLength != 0)
	{
		if ((retVal = cvValidateObjectValue(objPtr->objType, objValueLength, pObjValue)) != CV_SUCCESS)
			goto err_exit;
		objPropertiesNew.objValueLength = objValueLength;
		objPropertiesNew.pObjValue = pObjValue;
		/* determine if object private data can be written */
		if (authFlags & CV_OBJ_AUTH_WRITE_PRIV)
			allowPriv = TRUE;
	}
	/* check to make sure object not too large */
	objHdr = *objPtr;
	objPtr = (cv_obj_hdr *)&obj[0];
	if ((objHdr.objLen = cvComputeObjLen(&objPropertiesNew)) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_OBJECT_SIZE;
		goto err_exit;
	}
	cvComposeObj(&objPropertiesNew, &objHdr, (uint8_t *)objPtr);
	cvFindObjPtrs(&objProperties, objPtr);

	/* get new obj attribs */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &pFlags_new, &objAttribs_new)) != CV_SUCCESS)
		goto err_exit;

	/* check to make sure storage type is same */
	if (objAttribs_new.storageType != objAttribs_cur.storageType)
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* now do any necessary conversions of key formats */
	if (objValueLength != 0)
	{
		if ((retVal = cvCopyObjValue(objPtr->objType, objValueLength, objProperties.pObjValue, pObjValue, allowPriv, TRUE)) != CV_SUCCESS)
			goto err_exit;
	}

	/* flush object from cache, because the cache has the old version */
	cvFlushCacheEntry(objHandle);

	/* if this is an auth session object which has a copy mirrored as a volatile object, delete it so changew are made to flash copy */
	if (objPtr->objType == CV_TYPE_AUTH_SESSION)
		cvDeleteVolatileAuthSessionObj(objHandle);


	/* now save object */
	retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cv_enumerate_objects (cv_handle cvHandle, cv_obj_type objType,                                
					  uint32_t *pObjHandleListLength, cv_obj_handle *pObjHandleList)
{
	/* This function returns a list of handles to all of the objects visible to the calling application.   */
	/* If the length of the object list isn’t sufficient, an error will be returned and the necessary length */
	/* will be provided.  If the app/user identifier fields provided when this CV session was created are all */
	/* zeros, handles of all objects in the CV will be returned. */
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	
	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	return cvEnumObj(&objProperties, objType, pObjHandleListLength, pObjHandleList, TRUE);
}

cv_status
cv_enumerate_objects_direct (uint32_t userAppHashListLength, uint8_t *pUserAppHashListBuf, cv_obj_type objType,                                
					  uint32_t *pObjHandleListLength, cv_obj_handle *pObjHandleList)
{
	/* This function returns a list of handles to all of the objects which match the userID/appID hashes included.   */
	/* If the length of the object list isn’t sufficient, an error will be returned and the necessary length */
	/* will be provided.  */
	cv_status retVal = CV_SUCCESS;
	uint32_t			numHashs = userAppHashListLength/SHA1_LEN;
	cv_session			tempSession;
	uint32_t			remBytes = *pObjHandleListLength;
	uint32_t			availBytes, totBytes = 0;
	uint8_t				*bufPtr = (uint8_t *)pObjHandleList;
	uint32_t			idx;
	cv_obj_properties objProperties;

	if ( numHashs == 0 ) {
		// if no hashes then no objects return ok.
		*pObjHandleListLength = 0;
		return CV_SUCCESS;
	}

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)&tempSession;
	// init cv_session
	memset((void*)&tempSession, 0, sizeof(cv_session));
	for ( idx=0; idx<numHashs; idx++ ) {

		// load appUserID hash
		memcpy((void*)tempSession.appUserID, (void*)&pUserAppHashListBuf[idx*SHA1_LEN], SHA1_LEN);
		availBytes = remBytes;
		retVal = cvEnumObj(&objProperties, objType, &availBytes, (cv_obj_handle *)bufPtr, FALSE);
		if ( retVal != CV_SUCCESS ) {
			printf("cv_enumerate_objects_direct() cvEnumObj returned 0x%x\n", retVal);
			return retVal;
		}
		remBytes -= availBytes;
		totBytes += availBytes;
		bufPtr += availBytes;
		if (remBytes == 0)
			break;
	}
	*pObjHandleListLength = totBytes;

	return retVal;
}

cv_status
cv_delete_object (cv_handle cvHandle, cv_obj_handle objHandle,           
					uint32_t authListLength, cv_auth_list *pAuthList,
					cv_callback *callback, cv_callback_ctx context)
{

	/* This function deletes an object.  The authorization provided in the auth list parameter will be */
	/* checked before allowing the deletion.  The authorization flags should indicate delete authorization */
	/* requested. */
	/* This routine sets the corresponding portions of the object (attributes or value), but not auth */
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_type objType;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 3, sizeof(objHandle), &objHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_ALLOW_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure delete auth granted */
	if (!(authFlags & CV_OBJ_AUTH_DELETE))
	{
		/* delete auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	objType = objPtr->objType;

	retVal = cvDelObj(&objProperties, FALSE);

    if ( objType == CV_TYPE_FINGERPRINT && retVal == CV_SUCCESS)
    {
        /* No FP enrolled, disable POA in EC */
        if( CV_PERSISTENT_DATA->poa_enable && ( ush_get_fp_template_count() == 0 ))
        {
                cv_manage_poa(NULL, FALSE, 0, 0, FALSE);
           
        }
    }

err_exit:
	return retVal;
}

cv_status
cv_change_auth_object (cv_handle cvHandle, cv_obj_handle objHandle,   
						uint32_t curAuthListLength, cv_auth_list *pCurAuthList,                                  
						uint32_t newAuthListsLength, cv_auth_list *pNewAuthLists,
						cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the object and performs the authorization.  The object is modified to */
	/* include the new auth lists provided and the object is saved again */
	cv_obj_properties objProperties, objPropertiesNew;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	uint8_t obj[MAX_CV_OBJ_SIZE];
	cv_auth_list *mergedAuthLists;
	uint32_t mergedAuthListsLen = 0;
	uint32_t LRUobj;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 5, sizeof(objHandle), &objHandle, 
		sizeof(uint32_t), &curAuthListLength, curAuthListLength, pCurAuthList,
		sizeof(uint32_t), &newAuthListsLength, newAuthListsLength, pNewAuthLists,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pCurAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, curAuthListLength, pCurAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure change auth granted */
	if (!(authFlags & CV_OBJ_AUTH_CHANGE_AUTH))
	{
		/* change auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* check to see if this is a partial auth lists update */
	if (pNewAuthLists->flags & CV_CHANGE_AUTH_PARTIAL)
	{
		/* yes, validate auth lists */
		if (pNewAuthLists->numEntries != 0)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
		/* bump pointer past initial empty auth list */
		pNewAuthLists++;
		newAuthListsLength -= sizeof(cv_auth_list);
		/* use object cache entry to store merged auth lists */ 
		cvObjCacheGetLRU(&LRUobj, (uint8_t **)&mergedAuthLists);

		if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, newAuthListsLength, pNewAuthLists)) != CV_SUCCESS)
			goto err_exit;
		if ((retVal = cvDoPartialAuthListsChange(newAuthListsLength, pNewAuthLists, objProperties.authListsLength,
				objProperties.pAuthLists, &mergedAuthListsLen, mergedAuthLists)) != CV_SUCCESS)
			goto err_exit;
		newAuthListsLength = mergedAuthListsLen;
		pNewAuthLists = mergedAuthLists;
	}

	/* validate new auth lists provided */
	if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, newAuthListsLength, pNewAuthLists)) != CV_SUCCESS)
		goto err_exit;

	/* now compose object locally, using fields from existing object, where not provided in input param */
	memcpy(&objPropertiesNew, &objProperties, sizeof(objProperties));
	objPropertiesNew.authListsLength = newAuthListsLength;
	objPropertiesNew.pAuthLists = pNewAuthLists;
	/* check to make sure object not too large */
	*((cv_obj_hdr *)&obj[0]) = *objPtr;
	objPtr = (cv_obj_hdr *)&obj[0];
	if ((objPtr->objLen = cvComputeObjLen(&objPropertiesNew)) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_OBJECT_SIZE;
		goto err_exit;
	}
	cvComposeObj(&objPropertiesNew, objPtr, &obj[0]);
	cvFindObjPtrs(&objProperties, objPtr);

	/* flush object from cache, because the cache has the old version */
	cvFlushCacheEntry(objHandle);

	/* if this is an auth session object which has a copy mirrored as a volatile object, delete it so changew are made to flash copy */
	if (objPtr->objType == CV_TYPE_AUTH_SESSION)
		cvDeleteVolatileAuthSessionObj(objHandle);

	/* now save object */
	retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cv_duplicate_object (cv_handle cvHandle, cv_obj_handle objHandle,                               
						uint32_t authListLength, cv_auth_list *pAuthList,                              
						cv_obj_handle *pNewObjHandle, cv_persistence persistenceAction,
						cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the object and performs the authorization.  A new object is created */
	/* and cvPutObj is called to store the new object and return the new handle. */
	/* This routine sets the corresponding portions of the object (attributes or value), but not auth */
	cv_obj_properties objProperties, objPropertiesNew;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_attrib_flags *flags;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 4, sizeof(objHandle), &objHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_persistence), &persistenceAction,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* now create new object */
	memcpy(&objPropertiesNew, &objProperties, sizeof(objProperties));
	objPropertiesNew.objHandle = 0;

	/* find attributes flags and check to see if need to change based on persistence */
	if ((retVal = cvFindObjAtribFlags(&objPropertiesNew, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (persistenceAction == CV_MAKE_PERSISTENT)
	{
		/* was volatile, make persistent (must be host hard drive type) */
		if (*flags & CV_ATTRIB_PERSISTENT)
		{
			/* already persistent, return error */
			retVal = CV_INVALID_OBJECT_PERSISTENCE;
			goto err_exit;
		}
		*flags |= CV_ATTRIB_PERSISTENT;
	} else if (persistenceAction == CV_MAKE_VOLATILE) 
	{
		/* was persistent, make volatile */
		if (!(*flags & CV_ATTRIB_PERSISTENT))
		{
			/* already volatile, return error */
			retVal = CV_INVALID_OBJECT_PERSISTENCE;
			goto err_exit;
		}
		*flags &= ~(CV_ATTRIB_PERSISTENT | CV_ATTRIB_NVRAM_STORAGE | CV_ATTRIB_SC_STORAGE | CV_ATTRIB_CONTACTLESS_STORAGE);
	} else if (persistenceAction != CV_KEEP_PERSISTENCE){
		/* invalid action */
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}
	/* flush original object from cache, because modified attributes to save as new object */
	cvFlushCacheEntry(objHandle);

	/* now save new object and get handle */
	if ((retVal = cvPutObj(&objPropertiesNew, objPtr)) != CV_SUCCESS)
		goto err_exit;
	*pNewObjHandle = objPropertiesNew.objHandle;

	/* now save original object (if auth modified it) */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cv_authorize_session_object (cv_handle cvHandle,              
								cv_obj_handle authSessObjHandle,  
								uint32_t authListLength, cv_auth_list *pAuthList,
								cv_callback *callback, cv_callback_ctx context)
{
	/* This routine performs the authorization of the session object */
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_input_parameters inputParameters;
	cv_bool needObjWrite = FALSE;
	uint32_t index;
	cv_obj_value_auth_session *objValue;
	cv_obj_storage_attributes objAttribs;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 4, sizeof(authSessObjHandle), 
		&authSessObjHandle,	 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	/* first make sure auth session object is in volatile memory */
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = authSessObjHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);
	if (!cvFindVolatileDirEntry(authSessObjHandle, &index))
	{
		/* if not in volatile memory, may be stored in flash.  check to see and if so, create volatile memory object */
		/* and copy from flash */
		if (cvFindDir0Entry(authSessObjHandle, &index))
		{
			/* object found, determine where object resident.  only allow case where object in flash */
			objAttribs = CV_DIR_PAGE_0->dir0Objs[index].attributes;
			if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH)
			{
				/* now get object */
				if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
					goto err_exit;

				/* store as volatile object */
				objProperties.attribs.reqHandle = TRUE;
				objProperties.attribs.storageType = CV_STORAGE_TYPE_VOLATILE;
				/* note: object doesn't really have privacy key, but this will allow attribs in objProperties to be used */
				/* instead of in the object itself.  doesn't matter since this is volatile obj */
				objProperties.attribs.hasPrivacyKey = 1;
				/* set authorized flags to false */
                objValue = (cv_obj_value_auth_session *)objProperties.pObjValue;
				objValue->statusLSH = 0;
				if ((retVal = cvPutObj(&objProperties, objPtr)) != CV_SUCCESS)
					goto err_exit;
				/* now get index to volatile obj */
				(void)cvFindVolatileDirEntry(authSessObjHandle, &index);
				/* flush object from cache, because want to use volatile object below */
				cvFlushCacheEntry(authSessObjHandle);
			} else {
				/* fail if not flash object */
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
		} else {
			/* fail if not dir 0 entry */
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
	}
	/* volatile object exists (shouldn't fail here), set up pointers */
	if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
		goto err_exit;

	/* now do auth */
	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, set flags to indicate which auth granted */
	objValue = (cv_obj_value_auth_session *)objProperties.pObjValue;
	objValue->statusLSH |= authFlags;

	/* no need to save object, since it's a volatile object */

err_exit:
	return retVal;
}

cv_status
cv_deauthorize_session_object (cv_handle cvHandle, cv_obj_handle authSessObjHandle, cv_obj_auth_flags flags)
{
	/* This routine resets the flags in the auth session object corresponding to the flags parameter, which has */
	/* the effect of deauthorizing the corresponding actions associated with this session object. */
	/* This routine performs the authorization of the session object */
	cv_status retVal = CV_SUCCESS;
	uint32_t index, volDirIndex;
	cv_obj_value_auth_session *objValue;
	cv_obj_hdr *objPtr;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* validate flags parameter */
	if (flags & ~CV_OBJ_AUTH_FLAGS_MASK)
	{
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}

	/* get pointer to auth session object in volatile memory */
	if (!cvFindVolatileDirEntry(authSessObjHandle, &volDirIndex))
	{
		/* check to see if flash object exists (if so, and no volatile object exists, by definition it's deauthorized) */
		if (!cvFindDir0Entry(authSessObjHandle, &index))
			retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}
	/* now reset flags indicated */
	objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj;
	objValue = (cv_obj_value_auth_session *)cvFindObjValue(objPtr);
	objValue->statusLSH &= ~(flags);
	/* if all authorizations deauthorized, check to see if there's a flash object.  if so, can delete */
	/* volatile object */
	if (!objValue->statusLSH)
		cvDeleteVolatileAuthSessionObj(authSessObjHandle);

err_exit:
	return retVal;
}

cv_status
cvComputeBlobHMAC(uint8_t *blobHdr, uint32_t blobHdrLen, uint8_t *obj, uint32_t objLen, uint8_t *hmacKey, uint32_t hmacLen, uint8_t *hmac)
{
	/* this routine computes an HMAC of the import blob */
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE]; 
	uint8_t *bufPtr = &buf[0];
	cv_status retVal = CV_SUCCESS;

	memcpy(bufPtr, blobHdr, blobHdrLen);
	memcpy(bufPtr + blobHdrLen, obj, objLen);
	/* first do the hash of the parameters || session count, then do HMAC of hash */
	if ((retVal = cvAuth(bufPtr, blobHdrLen + objLen, hmacKey, hmac, hmacLen, NULL, 0, CV_SHA)) != CV_SUCCESS)
		goto err_exit;

err_exit:
	return retVal;
}

cv_bool canCrossmatchFingerprintTemplateBeRestored()
{
	if ((VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH ) ||
	    (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_NEXT_TOUCH) )
	{
		return TRUE;
	}

	return FALSE;
}

cv_bool canSynapticsFingerprintTemplateBeRestored()
{
	if ((VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_SYNAPTICS_TOUCH) )
	{
		return TRUE;
	}

	return FALSE;
}


cv_status
cv_import_object (cv_handle cvHandle, cv_blob_type blobType,                               
					uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
					uint32_t authListsLength, cv_auth_list *pAuthLists,           
					uint32_t objValueLength, void *pObjValue,
					cv_dec_op decryptionOperation,                              
					cv_bulk_params *pBulkParameters,                            
					uint32_t importBlobLength, void *pImportBlob,                 
					uint32_t authListLength, cv_auth_list *pAuthList,                             
					cv_obj_handle unwrappingObjectHandle, 
					uint32_t embeddedHandlesLength,
					cv_obj_handle *pEmbeddedHandlesMap, 
					cv_obj_handle *pNewObjHandle,
					cv_callback *callback, cv_callback_ctx context)
{
	/* This function imports a wrapped object and returns the handle to the new object.  */
	/* The attribute, auth list  and object value parameters must be supplied if the blob type */
	/* indicates that they aren’t supplied in the blob */
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key;
	cv_obj_type objType;
	cv_export_blob *blob = (cv_export_blob *)pImportBlob;
	cv_bulk_params bulkParams, innerBulkParams;
	uint8_t *pEncData, *pIV, *pIVPriv = NULL, *blobPtr;
	uint32_t keyLen = 0;
	uint32_t embeddedHandleLengthBlob, embeddedHandlesCount = embeddedHandlesLength/sizeof(cv_obj_handle), embeddedHandlesCountBlob;
	cv_obj_handle *embeddedHandlesBlobPtr;
	uint32_t cleartextLength, decryptedLength;
	uint8_t HMACblob[SHA256_LEN], HMACcomputed[SHA256_LEN];
	uint8_t HMACKey[SHA256_LEN];
	uint8_t symKey[sizeof(cv_obj_value_key) - 1 + AES_256_BLOCK_LEN];
	cv_obj_value_key *pSymKey = (cv_obj_value_key *)&symKey[0];
	uint8_t obj[CV_STACK_BUFFER_SIZE];
	cv_attrib_flags *flags;
	cv_admin_auth_permission permission = CV_DENY_ADMIN_AUTH;
	uint8_t hmacKeyValue[14];
	cv_obj_value_fingerprint *pTemplate;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrsLong(&inputParameters, 16, sizeof(cv_blob_type), &blobType,
		sizeof(uint32_t), &objAttributesLength, objAttributesLength, pObjAttributes,
		sizeof(uint32_t), &authListsLength, authListsLength, pAuthLists,
		sizeof(uint32_t), &objValueLength, objValueLength, pObjValue, sizeof(cv_dec_op), &decryptionOperation,
		sizeof(cv_bulk_params), pBulkParameters,
		sizeof(uint32_t), &importBlobLength, importBlobLength, pImportBlob,
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_obj_handle), &unwrappingObjectHandle,
		sizeof(uint32_t), &embeddedHandlesLength, embeddedHandlesLength, pEmbeddedHandlesMap,
		0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = unwrappingObjectHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* if want to import native CV object and request the same handle, must have CV admin auth */
	if (blobType == CV_NATIVE_BLOB_RETAIN_HANDLE)
		/* don't allow this function */
		return CV_FEATURE_NOT_AVAILABLE;
	/* only allow CV admin auth in managed mode */
	if (CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE)
		permission = CV_ALLOW_ADMIN_AUTH;
	if ((retVal = cvHandlerObjAuth(permission, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure proper auth granted. if CV admin auth used, must be export auth, but if */
	/* normal auth used, must be access auth, since this is for wrapping key object */
	if (authFlags & CV_ADMINISTRATOR_AUTH) 
	{
		/* CV admin auth */
		if (!(authFlags & CV_OBJ_AUTH_EXPORT))
		{
			/* export auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	} else {
		/* normal auth */
		if (!(authFlags & CV_OBJ_AUTH_ACCESS))
		{
			/* export auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	/* make sure unwrapping key can be used for this purpose */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (!(*flags & CV_ATTRIB_USE_FOR_UNWRAPPING))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* now check to see what type of import blob */
	/* validate blob.  disallow privacy key blobs for now */
	if (blob->storageAttribs.hasPrivacyKey || blob->encAreaLen > MAX_CV_OBJ_SIZE || blob->objLen > blob->encAreaLen)
	{
		retVal = CV_INVALID_IMPORT_BLOB;
		goto err_exit;
	}
	switch (blobType)
	{
	case CV_NATIVE_BLOB:
	case CV_NATIVE_BLOB_RETAIN_HANDLE:
		/* fail if obj params sent, because not used for native blobs */
		if (objAttributesLength !=0 || authListsLength != 0 || objValueLength != 0)
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		/* now determine unwrapping key type and unwrap blob */
		key = (cv_obj_value_key *)objProperties.pObjValue;
		objType = objPtr->objType;
		switch (decryptionOperation)
		{
		case CV_BULK_DECRYPT:
		case CV_BULK_DECRYPT_THEN_AUTH:
		case CV_BULK_AUTH_THEN_DECRYPT:
			if (pBulkParameters == NULL)
			{
				retVal = CV_INVALID_INPUT_PARAMETER;
				goto err_exit;
			}
			if (blob->symKeyEncAreaLen != 0)
			{
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
			}
			/* determine key length */
			switch (key->keyLen)
			{
			case 128:
				keyLen = AES_128_BLOCK_LEN;
				break;
			case 192:
				keyLen = AES_192_BLOCK_LEN;
				break;
			case 256:
				keyLen = AES_256_BLOCK_LEN;
				break;
			default:
				/* key length wil be one of the above, because validated when object is created */
				break;
			}
			/* get pointers to fields in blob */
			blobPtr = (uint8_t *)blob + sizeof(cv_export_blob);
			/* determine if privacy key IV */
			if (blob->storageAttribs.hasPrivacyKey)
			{
				pIVPriv = blobPtr;
				blobPtr += AES_128_BLOCK_LEN;
			}
			embeddedHandleLengthBlob = *((uint32_t *)blobPtr);
			/* check validity of length */
			if (embeddedHandleLengthBlob > (CV_MAX_EXPORT_BLOB_EMBEDDED_HANDLES * sizeof(cv_obj_handle)))
			{
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
			}
			embeddedHandlesBlobPtr = (cv_obj_handle *)(blobPtr + sizeof(uint32_t));
			blobPtr = (uint8_t *)embeddedHandlesBlobPtr + embeddedHandleLengthBlob;
			pIV = blobPtr;
			blobPtr += keyLen;
			pEncData = blobPtr;
			/* use bulk Params from input, except use IV from blob */
			memcpy(&bulkParams, pBulkParameters, sizeof(bulkParams));
			memcpy(bulkParams.IV, pIV, keyLen);
			/* ignore hmac if indicated in input params, because hmac computed separately */
			if (bulkParams.authAlgorithm != CV_AUTH_ALG_NONE)
				bulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
			/* zero offsets, if any */
			bulkParams.authOffset = 0;
			bulkParams.encOffset = 0;
			/* set up max length of decrypted obj */
			cleartextLength = CV_STACK_BUFFER_SIZE;
			if ((retVal = cvBulkDecrypt(pEncData, blob->encAreaLen, obj, &cleartextLength, NULL, NULL, 
				CV_BULK_DECRYPT, &bulkParams, objType, key, NULL)) != CV_SUCCESS)
				goto err_exit;
			break;
		case CV_ASYM_DECRYPT:
			/* use asymmetric unwrapping key to decrypt AES-128 symmetric key */
			keyLen = AES_128_BLOCK_LEN;
			/* get pointers to fields in blob */
			blobPtr = (uint8_t *)blob + sizeof(cv_export_blob);
			/* determine if privacy key IV */
			if (blob->storageAttribs.hasPrivacyKey)
			{
				pIVPriv = blobPtr;
				blobPtr += AES_128_BLOCK_LEN;
			}
			embeddedHandleLengthBlob = *((uint32_t *)blobPtr);
			/* check validity of length */
			if (embeddedHandleLengthBlob > (CV_MAX_EXPORT_BLOB_EMBEDDED_HANDLES * sizeof(cv_obj_handle)))
			{
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
			}
			embeddedHandlesBlobPtr = (cv_obj_handle *)(blobPtr + sizeof(uint32_t));
			blobPtr = (uint8_t *)embeddedHandlesBlobPtr + embeddedHandleLengthBlob;
			pIV = blobPtr;
			blobPtr += keyLen;
			pEncData = blobPtr;
			/* validate length field before using */
			if (blob->symKeyEncAreaLen > (key->keyLen*8))
			{
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
			}
			/* now decrypt symmetric encryption key */
			cleartextLength = CV_STACK_BUFFER_SIZE;
			if ((retVal = cvAsymDecrypt(pEncData, blob->symKeyEncAreaLen, pEncData, &cleartextLength, objType, key)) != CV_SUCCESS)
				goto err_exit;
			/* now decrypt object using decrypted symmetric key */
			key = (cv_obj_value_key *)&symKey[0];
			key->keyLen = AES_128_BLOCK_LEN*8;
			memcpy(key->key, pEncData, AES_128_BLOCK_LEN); 
			memset(pEncData,0,blob->symKeyEncAreaLen);
			/* use default bulk Params, except use IV from blob */
			memcpy(&bulkParams, pBulkParameters, sizeof(bulkParams));
			memcpy(bulkParams.IV, pIV, keyLen);
			bulkParams.IVLen = keyLen;
			bulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
			bulkParams.authOffset = 0;
			bulkParams.encOffset = 0;
			memcpy(bulkParams.IV, pIV, keyLen);
			pEncData += blob->symKeyEncAreaLen;
			objType = CV_TYPE_AES_KEY;
			/* set up max length of decrypted obj */
			cleartextLength = CV_STACK_BUFFER_SIZE;
			if ((retVal = cvBulkDecrypt(pEncData, blob->encAreaLen, obj, &cleartextLength, NULL, NULL, 
				CV_BULK_DECRYPT, &bulkParams, CV_TYPE_AES_KEY, key, NULL)) != CV_SUCCESS)
				goto err_exit;
			break;
		case 0xffff:
		default:
			retVal = CV_INVALID_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_MSCAPI_BLOB:
	case CV_CNG_BLOB:
	case CV_PKCS_BLOB:
	default:
		/* not supported yet */
		return CV_FEATURE_NOT_AVAILABLE;
	}
	/* now do HMAC and compare with value in blob */
	/* copy HMAC from blob locally */
	/* handle cv v1.0 blobs differently */
	if (blob->version.versionMajor == CV_MAJOR_VERSION_1 && blob->version.versionMinor == CV_MINOR_VERSION_0)
		memcpy(HMACblob, obj + blob->objLen, SHA256_LEN);
	else
		memcpy(HMACblob, obj + blob->encAreaLen - SHA256_LEN, SHA256_LEN);
	/* compute HMAC key */
	memcpy(hmacKeyValue, "CV import blob", sizeof(hmacKeyValue));
	if ((retVal = cvAuth(hmacKeyValue, sizeof(hmacKeyValue), NULL, (uint8_t *)&HMACKey[0], SHA256_LEN, NULL, 0, 0)) != CV_SUCCESS)
		goto err_exit;
	/* handle cv v1.0 blobs differently */
	if (blob->version.versionMajor > CV_MAJOR_VERSION_1)
	{
		/* now compute HMAC.   */
		if ((retVal = cvComputeBlobHMAC((uint8_t *)blob, pEncData - (uint8_t *)blob, obj, blob->encAreaLen - SHA256_LEN, (uint8_t *)&HMACKey[0], SHA256_LEN, (uint8_t *)&HMACcomputed[0])) != CV_SUCCESS)
			goto err_exit;
		/* compare with HMAC from blob */
		if (memcmp(HMACblob, HMACcomputed, SHA256_LEN))
		{
			retVal = CV_INVALID_IMPORT_BLOB;
			goto err_exit;
		}
		/* now remove inner 2 encryptions: USH unique and GCK, if any */
		memset(&innerBulkParams,0,sizeof(cv_bulk_params));
		innerBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
		innerBulkParams.authOffset = 0;
		innerBulkParams.encOffset = 0;
		innerBulkParams.bulkMode = CV_CBC;
		innerBulkParams.IVLen = AES_128_BLOCK_LEN;
		pSymKey->keyLen = AES_128_BLOCK_LEN*8;
		/* check to see if GCK exists */
		if (CV_PERSISTENT_DATA->persistentFlags & CV_GCK_EXISTS)
		{
			decryptedLength = cleartextLength;
			memcpy(&pSymKey->key[0], &CV_PERSISTENT_DATA->GCKKey[0], AES_128_BLOCK_LEN);
			if ((retVal = cvBulkDecrypt(obj, cleartextLength - SHA256_LEN, obj, &decryptedLength, NULL, NULL, 
				CV_BULK_DECRYPT, &innerBulkParams, CV_TYPE_AES_KEY, pSymKey, NULL)) != CV_SUCCESS)
				goto err_exit;
		}
		if ((ushx_rom_read(CV_HID2_FLASH_BLOCK_CUST_ID, CV_EXPORT_ENC_INNER_KEY_ADDR, AES_128_BLOCK_LEN, &pSymKey->key[0])) != PHX_STATUS_OK)
		{
			/* should only fail for non-C0 platforms, just use zeroes */
			memset(&pSymKey->key[0],0,AES_128_BLOCK_LEN);
		}
		decryptedLength = cleartextLength;
		if ((retVal = cvBulkDecrypt(obj, cleartextLength - SHA256_LEN, obj, &decryptedLength, NULL, NULL, 
			CV_BULK_DECRYPT, &innerBulkParams, CV_TYPE_AES_KEY, pSymKey, NULL)) != CV_SUCCESS)
			goto err_exit;
		/* copy HMAC from blob locally */
		memcpy(HMACblob, obj + blob->objLen, SHA256_LEN);
	}
	/* now compute HMAC.   */
	if ((retVal = cvComputeBlobHMAC((uint8_t *)blob, pEncData - (uint8_t *)blob, obj, blob->objLen, (uint8_t *)&HMACKey[0], SHA256_LEN, (uint8_t *)&HMACcomputed[0])) != CV_SUCCESS)
		goto err_exit;
	/* compare with HMAC from blob */
	if (memcmp(HMACblob, HMACcomputed, SHA256_LEN))
	{
		retVal = CV_INVALID_IMPORT_BLOB;
		goto err_exit;
	}

	/* now see if should remap handles in object */
	objPtr = (cv_obj_hdr *)&obj[0];
	objProperties.objHandle = NULL;
	if (blob->storageAttribs.hasPrivacyKey)
		/* save IV for storage of object */
		/* can't get object pointers if encrypted with privacy key */
		objProperties.pIVPriv = pIVPriv;
	else
		cvFindObjPtrs(&objProperties, objPtr);
	if (embeddedHandlesLength != 0)
	{
		/* can't remap handles if encrypted with privacy key */
		if (blob->storageAttribs.hasPrivacyKey)
		{
			retVal = CV_CANT_REMAP_HANDLES;
			goto err_exit;
		}
		embeddedHandlesCountBlob = embeddedHandleLengthBlob/sizeof(cv_obj_handle);
		/* first make sure there's a new handle for each handle in blob */
		if (embeddedHandlesCount != embeddedHandlesCountBlob)
		{
			retVal = CV_CANT_REMAP_HANDLES;
			goto err_exit;
		}
		cvRemapHandles(objProperties.authListsLength, objProperties.pAuthLists, embeddedHandlesCount, embeddedHandlesBlobPtr, pEmbeddedHandlesMap);
	}

	/* now save object */
	objProperties.attribs = blob->storageAttribs;
	/* indicate if want to keep handle */
#ifdef blobtype_CV_NATIVE_BLOB_RETAIN_HANDLE	
	if (blobType == CV_NATIVE_BLOB_RETAIN_HANDLE)
	{
		objProperties.attribs.reqHandle = 1;
		objProperties.objHandle = blob->handle;
		/* also must check to see if object exists.  if so, fail, because can't import over existing object (must delete first) */
		/* if on host hard drive, must read in to determine existance */
		if (objProperties.attribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE)
		{
			if ((retVal = cvGetObj(&objProperties, &objPtr)) == CV_SUCCESS)
			{
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
			objPtr = (cv_obj_hdr *)&obj[0];
		} else if ((retVal = cvDetermineObjAvail(&objProperties)) != CV_INVALID_OBJECT_HANDLE) {
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
	}
	}
#endif

	if (objPtr->objType == CV_TYPE_FINGERPRINT)
	{
		pTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;

		if ( ( (pTemplate->type == CV_FINGERPRINT_TEMPLATE) || (pTemplate->type == CV_FINGERPRINT_FEATURE_SET) ) &&
			 !canCrossmatchFingerprintTemplateBeRestored() )
		{
				printf("Crossmatch matcher fingerprint object will not be restored because a compatible sensor is not connected\n");
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
		}

		if ( ( (pTemplate->type == CV_FINGERPRINT_TEMPLATE_SYNAPTICS) || (pTemplate->type == CV_FINGERPRINT_FEATURE_SET_SYNAPTICS) ) &&
			 !canSynapticsFingerprintTemplateBeRestored() )
		{
				printf("Synaptics fingerprint object will not be restored because a Synaptics sensor is not connected\n");
				retVal = CV_INVALID_IMPORT_BLOB;
				goto err_exit;
		}
	}

	retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	*pNewObjHandle = objProperties.objHandle;

err_exit:
	return retVal;
}

cv_status
cv_export_object (cv_handle cvHandle, cv_blob_type blobType,                               
					byte blobLabel[16], cv_obj_handle objectHandle,                                 
					uint32_t authListLength, cv_auth_list *pAuthList,                             
					cv_enc_op encryptionOperation,                              
					cv_bulk_params *pBulkParameters,                            
					cv_obj_handle wrappingObjectHandle,                                 
					uint32_t *pExportBlobLength, void *pExportBlob,
					cv_callback *callback, cv_callback_ctx context)
{
	/* This routine is analogous to cv_import object, but it is necessary to get and authorize both the wrapping object */
	/* and the object to be exported.  Otherwise, the blob type and parameters are used to build and encrypt the blob for export. */
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties, objPropertiesExpObj;
	cv_obj_hdr *objPtr, *objPtrExpObj;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE, needObjWriteExpObj = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key;
	cv_obj_type objType;
	cv_export_blob *blob = (cv_export_blob *)pExportBlob;
	uint8_t *pEncData, *pIV, *blobPtr, *pIVPriv;
	uint32_t keyLen = 0;
	uint32_t *embeddedHandlesLengthBlob;
	uint32_t cleartextLength, encryptedLength;
	uint8_t *pHMAC;
	uint8_t HMACKey[SHA256_LEN];
	cv_attrib_flags *flags, *flagsExp;
	cv_admin_auth_permission permission = CV_DENY_ADMIN_AUTH;
	uint32_t computedBlobLen, hashLen;
	uint8_t *maxPtr;
	uint32_t retLength = 0, padLen, asymKeyLen;
	uint8_t AESKey[AES_128_BLOCK_LEN + sizeof(cv_obj_value_key) - 1];
	cv_bulk_params localBulkParams, innerBulkParams;
	uint32_t encryptedAsymKeyLen;
	uint8_t hmacKeyValue[14];
	cv_obj_value_key *symKey = (cv_obj_value_key *)&AESKey[0];
	uint8_t *pSymKey;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 8, sizeof(cv_blob_type), &blobType,
		16, &blobLabel[0], sizeof(cv_obj_handle), &objectHandle,
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		sizeof(cv_enc_op), &encryptionOperation,
		sizeof(cv_bulk_params), pBulkParameters,
		sizeof(cv_obj_handle), &wrappingObjectHandle, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = wrappingObjectHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* only allow CV admin auth in managed mode */
	if (CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE)
		permission = CV_ALLOW_ADMIN_AUTH;
	if ((retVal = cvHandlerObjAuth(permission, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;


	/* auth succeeded, but need to make sure proper auth granted. if CV admin auth used, must be export auth, but if */
	/* normal auth used, must be access auth, since this is for wrapping key object */
	if (authFlags & CV_ADMINISTRATOR_AUTH) 
	{
		/* CV admin auth */
		if (!(authFlags & CV_OBJ_AUTH_EXPORT))
		{
			/* export auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	} else {
		/* normal auth */
		if (!(authFlags & CV_OBJ_AUTH_ACCESS))
		{
			/* export auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	/* make sure wrapping key can be used for this purpose */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;

	if (!(*flags & CV_ATTRIB_USE_FOR_WRAPPING))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* now get object to be exported.  don't do auth again, but check to make sure the same auth would be */
	/* successful for exported object too (ie, same auth used for both).  skip this step if CV admin auth */
	/* used (which is backup case) */
	memset(&objPropertiesExpObj,0,sizeof(cv_obj_properties));
	objPropertiesExpObj.objHandle = objectHandle;
	objPropertiesExpObj.session = (cv_session *)cvHandle;

#if 0 	/* According to Doug, comment it out */
	/* if this is the CV admin, indicate that it's ok to leave the privacy encryption layer on */ 
	if (pAuthList->flags & CV_ADMINISTRATOR_AUTH)
		objPropertiesExpObj.attribs.hasPrivacyKey = 1;

#endif
	if ((retVal = cvGetObj(&objPropertiesExpObj, &objPtrExpObj)) != CV_SUCCESS)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	/* check to see if this is an auth session object.  if so, check to see if volatile copy exists  */
	/* if so, delete and get NVRAM copy */
	if (objPtrExpObj->objType == CV_TYPE_AUTH_SESSION)
	{
		uint32_t volDirIndex;

		if (cvFindVolatileDirEntry(objectHandle, &volDirIndex))
		{
			cvDeleteVolatileAuthSessionObj(objectHandle);
			memset(&objPropertiesExpObj,0,sizeof(cv_obj_properties));
			objPropertiesExpObj.objHandle = objectHandle;
			objPropertiesExpObj.session = (cv_session *)cvHandle;
			if ((retVal = cvGetObj(&objPropertiesExpObj, &objPtrExpObj)) != CV_SUCCESS)
			{
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
		}
	}


	/* Find the attribs of the exported obj */
	if ((retVal = cvFindObjAtribFlags(&objPropertiesExpObj, objPtrExpObj, &flagsExp, NULL)) != CV_SUCCESS)
		goto err_exit;

	/* Verify Export object restrictions of the object */
	if ((authFlags & CV_ADMINISTRATOR_AUTH) &&  (*flagsExp & CV_ATTRIB_NOT_IN_GLB_BKUP)) {
		retVal = CV_OBJECT_ATTRIBUTES_NOT_IN_GLB_BKUP;
		goto err_exit;
	}

	if (!(pAuthList->flags & CV_ADMINISTRATOR_AUTH))
	{
		/* get both auth lists, then compare */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
							  objProperties.authListsLength, objProperties.pAuthLists,
							  objPropertiesExpObj.authListsLength, objPropertiesExpObj.pAuthLists,
							  CV_OBJ_AUTH_EXPORT, &needObjWriteExpObj)) != CV_SUCCESS)
			goto err_exit;

	}

	/* determine fixed portion of blob length */
	computedBlobLen = sizeof(cv_export_blob) + objPtrExpObj->objLen + sizeof(uint32_t) /* embedded handles list length */
		+ SHA256_LEN /* HMAC length */;
	if (objPropertiesExpObj.attribs.hasPrivacyKey)
		computedBlobLen += AES_128_BLOCK_LEN /* privacy key IV */;
	/* if blob length less than length parameter, set up blob */
	maxPtr = (uint8_t *)pExportBlob + *pExportBlobLength; 
	if (computedBlobLen <= *pExportBlobLength)
	{
		blob->handle = objectHandle;
		memcpy(blob->label, blobLabel, 16);
		blob->objLen = objPtrExpObj->objLen;
		blob->storageAttribs = objPropertiesExpObj.attribs;
		cvGetCurCmdVersion(&blob->version);
	} else
		/* if output buffer not large enough, don't store data in it, but need to continue in order to compute required length */
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
	blobPtr = (uint8_t *)(blob + 1);
	/* now search for embedded handles and add to list */
	if (objPropertiesExpObj.attribs.hasPrivacyKey)
	{
		/* object is encrypted, can't include embedded handles */
		pIVPriv = blobPtr;
		blobPtr += AES_128_BLOCK_LEN;
		embeddedHandlesLengthBlob = (uint32_t *)blobPtr;
		blobPtr += sizeof(uint32_t);
		if (retVal != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
		{
			*embeddedHandlesLengthBlob = 0;
			memcpy(pIVPriv, objPropertiesExpObj.pIVPriv, AES_128_BLOCK_LEN);
		}
	} else {
		/* no privacy key, can build embedded handles list */
		embeddedHandlesLengthBlob = (uint32_t *)blobPtr;
		blobPtr += sizeof(uint32_t);
		retVal = cvBuildEmbeddedHandlesList(retVal, maxPtr, objPropertiesExpObj.authListsLength, objPropertiesExpObj.pAuthLists, 
			embeddedHandlesLengthBlob, (cv_obj_handle *)blobPtr);
	}
	blobPtr += *embeddedHandlesLengthBlob;

	/* now check to see what type of export blob */
	/* compute length of encrypted object + HMAC */
	hashLen = blobPtr - (uint8_t *)blob + objPtrExpObj->objLen;
	cleartextLength = objPtrExpObj->objLen + SHA256_LEN;
	/* compute HMAC key */
	memcpy(hmacKeyValue,"CV import blob",sizeof(hmacKeyValue)); 
	if ((retVal = cvAuth(hmacKeyValue, sizeof(hmacKeyValue), NULL, (uint8_t *)&HMACKey[0], SHA256_LEN, NULL, 0, 0)) != CV_SUCCESS)
		goto err_exit;
	switch (blobType)
	{
	case CV_NATIVE_BLOB:
	case CV_NATIVE_BLOB_RETAIN_HANDLE:
		/* now determine wrapping key type and wrap blob */
		key = (cv_obj_value_key *)objProperties.pObjValue;
		objType = objPtr->objType;
		if (pBulkParameters == NULL)
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		/* save bulk parameters locally, because will reuse input buffer for output */
		memcpy(&localBulkParams, pBulkParameters, sizeof(localBulkParams));
		/* override auth algorithm in bulk params, because auth done separately from encryption */
		/* also override offsets */
		localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
		localBulkParams.authOffset = 0;
		localBulkParams.encOffset = 0;
		switch (encryptionOperation)
		{
		case CV_BULK_ENCRYPT:
		case CV_BULK_ENCRYPT_THEN_AUTH:
		case CV_BULK_AUTH_THEN_ENCRYPT:
			/* determine key length */
			padLen = GET_AES_128_PAD_LEN(cleartextLength);   /* pad length is same for all key lengths */
			switch (key->keyLen)
			{
			case 128:
				keyLen = AES_128_BLOCK_LEN;
				break;
			case 192:
				keyLen = AES_192_BLOCK_LEN;
				break;
			case 256:
				keyLen = AES_256_BLOCK_LEN;
				break;
			default:
				/* key length wil be one of the above, because validated when object is created */
				retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
				goto err_exit;
			}
			/* get pointers to fields in blob */
			cleartextLength += padLen;
			pIV = blobPtr;
			blobPtr += keyLen;
			pEncData = blobPtr;
			pHMAC = pEncData + objPtrExpObj->objLen;
			/* now know length of output.  check to see if length of buffer provided is sufficient */
			encryptedLength = cleartextLength +  SHA256_LEN /* for 2nd (inner) HMAC */;
			blob->encAreaLen = encryptedLength;
			blob->symKeyEncAreaLen = 0;
			retLength = pEncData - (uint8_t *)blob + encryptedLength;
			if (retLength >  *pExportBlobLength)
			{
				retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
				goto err_exit;
			}
			/* use bulk Params from input */
			memcpy(pIV, localBulkParams.IV, keyLen);
			memcpy(pEncData, objPtrExpObj, objPtrExpObj->objLen);
			/* zero pad, if any */
			memset(pHMAC + SHA256_LEN, 0, padLen);
			/* now compute HMAC.   */
			if ((retVal = cvAuth((uint8_t *)blob, pHMAC - (uint8_t *)blob, (uint8_t *)&HMACKey[0], pHMAC, SHA256_LEN, NULL, 0, CV_HMAC_SHA)) != CV_SUCCESS)
				goto err_exit;
			/* now add inner 2 encryptions: USH unique and GCK, if any */
			memset(&innerBulkParams,0,sizeof(cv_bulk_params));
			innerBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
			innerBulkParams.authOffset = 0;
			innerBulkParams.encOffset = 0;
			innerBulkParams.bulkMode = CV_CBC;
			innerBulkParams.IVLen = AES_128_BLOCK_LEN;
			symKey->keyLen = AES_128_BLOCK_LEN*8;
			if ((ushx_rom_read(CV_HID2_FLASH_BLOCK_CUST_ID, CV_EXPORT_ENC_INNER_KEY_ADDR, AES_128_BLOCK_LEN, &symKey->key[0])) != PHX_STATUS_OK)
			{
				/* should only fail for non-C0 platforms, just use zeroes */
				memset(&symKey->key[0],0,AES_128_BLOCK_LEN);
			}
			if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
				CV_BULK_ENCRYPT, &innerBulkParams, CV_TYPE_AES_KEY, symKey, NULL)) != CV_SUCCESS)
				goto err_exit;
			/* now check to see if GCK exists */
			if (CV_PERSISTENT_DATA->persistentFlags & CV_GCK_EXISTS)
			{
				memcpy(&symKey->key[0], &CV_PERSISTENT_DATA->GCKKey[0], AES_128_BLOCK_LEN);
				if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
					CV_BULK_ENCRYPT, &innerBulkParams, CV_TYPE_AES_KEY, symKey, NULL)) != CV_SUCCESS)
					goto err_exit;
			}
			/* now compute outer HMAC.   */
			pHMAC = pEncData + cleartextLength;
			if ((retVal = cvAuth((uint8_t *)blob, pHMAC - (uint8_t *)blob, (uint8_t *)&HMACKey[0], pHMAC, SHA256_LEN, NULL, 0, CV_HMAC_SHA)) != CV_SUCCESS)
				goto err_exit;
			/* encrypt object and outer HMAC */
			cleartextLength += SHA256_LEN;
			encryptedLength = cleartextLength;
			if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
				CV_BULK_ENCRYPT, &localBulkParams, objType, key, NULL)) != CV_SUCCESS)
				goto err_exit;

			break;
		case CV_ASYM_ENCRYPT:
			/* use asymmetric unwrapping key to encrypt AES-128 symmetric key */
			keyLen = AES_128_BLOCK_LEN;
				padLen = GET_AES_128_PAD_LEN(cleartextLength);
			cleartextLength += padLen;
			asymKeyLen = key->keyLen/8;
			blob->symKeyEncAreaLen = asymKeyLen;
			/* get pointers to fields in blob */
			pIV = blobPtr;
			blobPtr += keyLen;
			pEncData = blobPtr;
			/* now know length of output.  check to see if length of buffer provided is sufficient */
			encryptedLength = cleartextLength +  SHA256_LEN /* for 2nd (inner) HMAC */;
			blob->encAreaLen = encryptedLength;
			blob->symKeyEncAreaLen = asymKeyLen;
			encryptedAsymKeyLen = asymKeyLen;
			retLength = pEncData - (uint8_t *)blob + encryptedLength + asymKeyLen;
			hashLen += asymKeyLen;
			if (retLength >  *pExportBlobLength)
			{
				retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
				goto err_exit;
			}
			symKey->keyLen = AES_128_BLOCK_LEN*8;
			memset(pEncData,0,asymKeyLen);
			pSymKey = pEncData;
			pEncData += asymKeyLen;
			pHMAC = pEncData + objPtrExpObj->objLen;
			/* use bulk Params from input */
			memcpy(pIV, localBulkParams.IV, keyLen);
			memcpy(pEncData, objPtrExpObj, objPtrExpObj->objLen);
			/* zero pad, if any */
			memset(pHMAC + SHA256_LEN, 0, padLen);
			/* now compute HMAC.   */
			if ((retVal = cvAuth((uint8_t *)blob, pHMAC - (uint8_t *)blob, (uint8_t *)&HMACKey[0], pHMAC, SHA256_LEN, NULL, 0, CV_HMAC_SHA)) != CV_SUCCESS)
				goto err_exit;
			/* now add inner 2 encryptions: USH unique and GCK, if any.  do this before asym encryption added to allow external rewrapping */
			memset(&innerBulkParams,0,sizeof(cv_bulk_params));
			innerBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
			innerBulkParams.authOffset = 0;
			innerBulkParams.encOffset = 0;
			innerBulkParams.bulkMode = CV_CBC;
			innerBulkParams.IVLen = AES_128_BLOCK_LEN;
			symKey->keyLen = AES_128_BLOCK_LEN*8;
			if ((ushx_rom_read(CV_HID2_FLASH_BLOCK_CUST_ID, CV_EXPORT_ENC_INNER_KEY_ADDR, AES_128_BLOCK_LEN, &symKey->key[0])) != PHX_STATUS_OK)
				/* should only fail for non-C0 platforms, just use zeroes */
				memset(&symKey->key[0],0,AES_128_BLOCK_LEN);
			if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
				CV_BULK_ENCRYPT, &innerBulkParams, CV_TYPE_AES_KEY, symKey, NULL)) != CV_SUCCESS)
				goto err_exit;
			/* now check to see if GCK exists */
			if (CV_PERSISTENT_DATA->persistentFlags & CV_GCK_EXISTS)
			{
				memcpy(&symKey->key[0], &CV_PERSISTENT_DATA->GCKKey[0], AES_128_BLOCK_LEN);
				if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
					CV_BULK_ENCRYPT, &innerBulkParams, CV_TYPE_AES_KEY, symKey, NULL)) != CV_SUCCESS)
					goto err_exit;
			}
			/* now compute outer HMAC.   */
			pHMAC = pEncData + cleartextLength;
			if ((retVal = cvAuth((uint8_t *)blob, pHMAC - (uint8_t *)blob, (uint8_t *)&HMACKey[0], pHMAC, SHA256_LEN, NULL, 0, CV_HMAC_SHA)) != CV_SUCCESS)
				goto err_exit;
			/* now create symmetric encryption key and encrypt */
			if ((retVal = cvRandom(AES_128_BLOCK_LEN, (uint8_t *)&symKey->key)) != CV_SUCCESS)
				goto err_exit;
			memcpy(pSymKey, (uint8_t *)&symKey->key, AES_128_BLOCK_LEN); 
			if ((retVal = cvAsymEncrypt(pSymKey, keyLen, pSymKey, &encryptedAsymKeyLen, objType, key)) != CV_SUCCESS)
				goto err_exit;
			/* use defaults for some bulk params */
			localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
			localBulkParams.authOffset = 0;
			localBulkParams.encOffset = 0;
			/* encrypt object and outer HMAC */
			cleartextLength += SHA256_LEN;
			encryptedLength = cleartextLength;
			if ((retVal = cvBulkEncrypt(pEncData, cleartextLength, pEncData, &encryptedLength, NULL, NULL, 
				CV_BULK_ENCRYPT, &localBulkParams, CV_TYPE_AES_KEY, symKey, NULL)) != CV_SUCCESS)
				goto err_exit;
			break;
		case 0xffff:
		default:
			retVal = CV_INVALID_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_MSCAPI_BLOB:
	case CV_CNG_BLOB:
	case CV_PKCS_BLOB:
	default:
		/* not supported yet */
		return CV_FEATURE_NOT_AVAILABLE;
	}

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	if (needObjWriteExpObj)
		retVal = cvHandlerPostObjSave(&objPropertiesExpObj, objPtrExpObj, callback, context);
err_exit:
	*pExportBlobLength = retLength;
	return retVal;
}



cv_status
cv_upgrade_objects (cv_handle cvHandle, uint32_t numObjHandles,                               
					cv_obj_handle *cvObjectHandles)
{
	cv_status retVal = CV_SUCCESS;
	uint32_t i=0;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_handle *cvObjectHandles_sav;
	cv_version version;
	uint32_t dir0Index;


	cvObjectHandles_sav = cvObjectHandles;

	/* Verify if all the objects are valid */
	for (i=0; i < numObjHandles; i++) {
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.objHandle = *cvObjectHandles;
		objProperties.session = (cv_session *)cvHandle;
		if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS) {
			retVal = CV_INVALID_OBJECT_ID;
			goto err_exit;
		}
		cvObjectHandles++;
	}

	/* upgrade the objects */
	cvGetCurCmdVersion(&version);
	cvObjectHandles = cvObjectHandles_sav; // restore the ptr
	for (i=0; i < numObjHandles; i++) {
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.objHandle = *cvObjectHandles;
		objProperties.session = (cv_session *)cvHandle;

		/* flush cache so that we can read obj with version */
		cvFlushCacheEntry(objProperties.objHandle);
		if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS) {
			goto err_exit;
		}
		if (!((objProperties.version.versionMajor == version.versionMajor) &&
			(objProperties.version.versionMinor == version.versionMinor) ) ) {
			printf("CV: upgrade: obj %d:%d  cmd: %d:%d [change]\n",
				objProperties.version.versionMajor, objProperties.version.versionMinor,
				version.versionMajor, version.versionMinor);
			/* update appID/userID in dir 0, if it exists */
			if (cvFindDir0Entry(objProperties.objHandle, &dir0Index))
				memcpy(&CV_DIR_PAGE_0->dir0Objs[dir0Index].appUserID, &objProperties.session->appUserID, 
					sizeof(objProperties.session->appUserID));
			if ((retVal = cvPutObj(&objProperties, objPtr)) != CV_SUCCESS) {
				goto err_exit;
			}
		} else {
			printf("CV: upgrade: %d:%d [no change]\n", objProperties.version.versionMajor, 
				objProperties.version.versionMinor);
		}
		cvObjectHandles++;
	}


err_exit:	
	return retVal;

}                                 
