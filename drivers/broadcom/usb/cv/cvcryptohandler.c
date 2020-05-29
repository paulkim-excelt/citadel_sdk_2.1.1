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
 * cvcryptohandler.c:  CVAPI Crypto Handler
 */
/*
 * Revision History:
 *
 * 03/09/07 DPA Created.
*/
#include "cvmain.h"
#include "console.h"
extern const uint8_t ecc_default_prime[ECC256_KEY_LEN];
extern const uint8_t ecc_default_a[ECC256_KEY_LEN];
extern const uint8_t ecc_default_b[ECC256_KEY_LEN];
extern const uint8_t ecc_default_G[ECC256_POINT];
extern const uint8_t ecc_default_n[ECC256_KEY_LEN];
extern const uint8_t ecc_default_h[ECC256_KEY_LEN];
extern const uint8_t dsa_default_p[DSA1024_KEY_LEN];
extern const uint8_t dsa_default_q[DSA1024_PRIV_LEN];
extern const uint8_t dsa_default_g[DSA1024_KEY_LEN];

#define SHA256_DER_PREAMBLE_LENGTH 19
#pragma push
#pragma arm section rodata="__section_cv_crypto_rodata"
static const uint8_t sha256DerPkcs1Array[SHA256_DER_PREAMBLE_LENGTH] =
		{0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
static const uint8_t sha1DerPkcs1Array[SHA1_DER_PREAMBLE_LENGTH] =
		{0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, \
         0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14};
#pragma pop

PHX_STATUS
cvPkcs1V15Encode(crypto_lib_handle *pHandle,
				uint32_t mLen,  uint8_t *M, uint32_t emLen, uint8_t *em)
{
	/* this routine is the same as emsa_pkcs1_v15_encode, but doesn't do hash of message (already hashed) */
	/* expects 'M' to be 16-byte or 32-byte hash */
	PHX_STATUS status = PHX_STATUS_OK;
	uint8_t outputSha1[SHA256_LEN];
	uint32_t i;
	uint32_t psLen;                    /* pad string length (PS) */
	uint16_t tLen;
	uint8_t *bp = em;  /* Setup pointer buffers */

	/* 1. Generate Hash */
	/* M in little endian, so we swap */
	/* set tLen based on hash used */
	if (mLen == SHA256_LEN)
		tLen = SHA256_DER_PREAMBLE_LENGTH + SHA256_LEN;
	else
		tLen = SHA1_DER_PREAMBLE_LENGTH + SHA1_LEN;
	memcpy(outputSha1, M, mLen);

	/* 2. Generate T: Encode algorithm ID etc. into DER --- refer to step 5 */

	/* 3. length check */
	if (emLen < tLen + 11)
		return PHX_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* 4. Generate PS, which is a number of 0xFF --- refer to step 5 */

	/* 5. Generate em = 0x00 || 0x01 || PS || 0x00 || T */
	*bp = 0x00; bp++;
	*bp = 0x01; bp++;

	for (i=0, psLen=0; i<((int)emLen - (int)tLen - 3); i++)
	{
		*bp = EMSA_PKCS1_V15_PS_BYTE;
		bp++;
		psLen++;
	}
    if (psLen < EMSA_PKCS1_V15_PS_LENGTH)
		return PHX_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

    /* Post the last flag uint8_t */
    *bp = 0x00; bp++;

    /* Post T(h) data into em(T) buffer */
	if (mLen == SHA256_LEN)
	{
		memcpy(bp, sha256DerPkcs1Array, SHA256_DER_PREAMBLE_LENGTH);
		bp += SHA256_DER_PREAMBLE_LENGTH;
	} else {
		memcpy(bp, sha1DerPkcs1Array, SHA1_DER_PREAMBLE_LENGTH);
		bp += SHA1_DER_PREAMBLE_LENGTH;
	}


    memcpy(bp, outputSha1, mLen);

	return status;
}

cv_status
cv_get_random (cv_handle cvHandle, uint32_t randLen, byte *pRandom, cv_callback *callback, cv_callback_ctx context)
{
	/* This routine uses SCAPI routine PHX_RNG_FIPS_GENERATE to generate a random number.  Since only a fixed number */
	/* of bytes is produced for each call, it is called multiple times depending on the size of the random number requested. */
	cv_status retVal = CV_SUCCESS;
	
	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	if (randLen > CV_MAX_RANDOM_NUMBER_LEN)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	retVal = cvRandom(randLen, pRandom);

err_exit:
	return retVal;

}

cv_status
cvGenRSAKey(cv_obj_properties *objProperties, cv_obj_value_key *key, uint32_t keyLength)
{
	/* this routine generates an RSA key */
	cv_status retVal = CV_SUCCESS;
	uint32_t *e;
	uint32_t keyLenBytes;
	uint32_t mLen, qLen = 0, pLen = 0, pinvLen, edqLen, edpLen;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint32_t m_numBits, d_numBits;
	uint8_t d[RSA2048_KEY_LEN];

	/* validate the key size */
	if (!(keyLength == 256 || keyLength == 512 || keyLength == 768 || keyLength == 1024 || keyLength == 2048))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	key->keyLen = keyLength;
	keyLenBytes = keyLength/8;
	/* now generate key */
	/* store default public key */
	e = (uint32_t *)&key->key[keyLenBytes];
	*e = RSA_PUBKEY_CONSTANT;
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);
	m_numBits = 32;
	if ((phx_rsa_key_generate(cryptoHandle, keyLength, 
		&m_numBits, (uint8_t *)e,
		&pLen, &key->key[keyLenBytes + 4] /* p */,
		&qLen, &key->key[keyLenBytes + 4 + keyLenBytes/2] /* q */,
		&mLen, &key->key[0] /* m */,
		&d_numBits, d,
		&edpLen, &key->key[keyLenBytes + 4 + 3*(keyLenBytes/2)] /* dp */,
		&edqLen, &key->key[keyLenBytes + 4 + 4*(keyLenBytes/2)] /* dq */,
		&pinvLen, &key->key[keyLenBytes + 4 + 2*(keyLenBytes/2)] /* pinv */)) != PHX_STATUS_OK)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	objProperties->objValueLength = 2 /* key size */+ keyLenBytes + 4 /* e */ + 5*(keyLenBytes/2);

err_exit:
	return retVal;
}

cv_status
cvGenDHGen(cv_obj_properties *objProperties, cv_obj_value_key *key, uint32_t keyLength,
		   uint32_t extraParamsLength, void *pExtraParams)
{
	/* this routine creates a Diffie-Helman key */
	cv_status retVal = CV_SUCCESS;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_obj_value_key *keyParams;
	uint32_t keyLenBytes, keyLenBytesOut;

	/* validate the key size */
	if (!(keyLength == 1024 || keyLength == 2048))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	key->keyLen = keyLength;
	/* extra parameters are cv_dh_generate_keygen_params (no actual structure since variable length) */
	/* validate extra parameters length */
	keyLenBytes = keyLength/8;
	if (extraParamsLength != (sizeof(uint16_t) + 2*keyLenBytes))
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	keyParams = (cv_obj_value_key *)pExtraParams;
	/* validate private key length (not greater than public key and also byte aligned) */
	if (keyParams->keyLen != keyLength || (keyParams->keyLen & 0x7)) 
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	/* compute private key */
	if ((retVal = cvRandom(keyParams->keyLen/8, &key->key[3*keyLenBytes])) != CV_SUCCESS)
		goto err_exit;
	/* copy generator and modulus into key.  do appropriate swap to put into BIGINT format */
	cvCopySwap((uint32_t *)&keyParams->key[0], (uint32_t *)&key->key[keyLenBytes], keyLenBytes);
	cvCopySwap((uint32_t *)&keyParams->key[keyLenBytes], (uint32_t *)&key->key[2*keyLenBytes], keyLenBytes);
	/* now compute D-H key */
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if ((phx_diffie_hellman_generate (
						cryptoHandle,
						&key->key[3*keyLenBytes],    keyParams->keyLen,
						1,
						&key->key[0],				 &keyLenBytesOut,
						&key->key[keyLenBytes],      keyLenBytes*8,
						&key->key[2*keyLenBytes],    keyLenBytes*8,
						NULL, NULL)) != PHX_STATUS_OK)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	objProperties->objValueLength = 2 /* key size */ + 4*keyLenBytes;

err_exit:
	return retVal;
}

#pragma push
#pragma arm section rodata="__section_const_dh_ss"
#pragma arm section code="__section_code_dh_ss"
/* DH domain parameters */
const uint8_t dh_default_g[] = {DH_DEFAULT_DOMAIN_PARAMS_G};
const uint8_t dh_default_m[] = {DH_DEFAULT_DOMAIN_PARAMS_M};

cv_status
cvGenDHSharedSecret(cv_obj_properties *objProperties, cv_obj_type objType, cv_obj_value_key *key, uint32_t keyLength,
		   uint32_t extraParamsLength, void *pExtraParams, cv_input_parameters *inputParameters,
		   cv_callback *callback, cv_callback_ctx context)
{
	/* this routine creates a Diffie-Helman shared secret */
	cv_status retVal = CV_SUCCESS;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_obj_value_key *dhKey;
	uint32_t keyLenBytes, keyLenBytesOut, dhPrivKeyLenBits, dhKeyLenBytes, dhKeyLenBits;
	uint32_t *useKdc, *pubKeyLen, *authListLen, pubKeyLenBytes;
	uint8_t *pubKey;
	cv_obj_handle *pKeyObjHandle;
	cv_auth_list *authList;
	uint8_t *x, *y, *m;
	cv_obj_hdr *objPtr;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_attrib_flags *flags;
	cv_obj_properties dhObjProperties;
	uint32_t keyTemp[DH2048_KEY_LEN];
	uint8_t hashTemp[SHA256_LEN];
	uint32_t hashLen;
#define FIXED_SIZE_PARAM_ELEMENTS (4 + 4 + 4 + 4)

	/* validate the key size based on type of symmetric key being created */
	if ((objType == CV_TYPE_AES_KEY && !(keyLength == 128 || keyLength == 192 || keyLength == 256)))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	keyLenBytes = keyLength/8;
	key->keyLen = keyLength;
	/* extra parameters are cv_dh_shared_secret_keygen_params (no actual structure since variable length) */
	/* validate and set up pointers to parameters */
	useKdc = pExtraParams;
	pubKeyLen = useKdc + 1;
	pubKeyLenBytes = *pubKeyLen;
	pubKey = (uint8_t *)(pubKeyLen + 1);
	if ((pubKeyLenBytes + FIXED_SIZE_PARAM_ELEMENTS) > extraParamsLength)
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	pKeyObjHandle = (cv_obj_handle *)(pubKey + pubKeyLenBytes);
	authListLen = (uint32_t *)(pKeyObjHandle + 1);
	authList = (cv_auth_list *)(authListLen + 1);
	if ((pubKeyLenBytes + FIXED_SIZE_PARAM_ELEMENTS + *authListLen) != extraParamsLength)
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	y = pubKey;
	/* need to put pubkey into BIGINT format */
	cvInplaceSwap((uint32_t *)y, pubKeyLenBytes/4);
	/* determine if use key from handle provided or kdc (in 6T) */
	if (*useKdc)
	{
		/* check to make sure the public key provided is a 2048-bit key */
		if (*pubKeyLen != KDC_PUBKEY_LEN)
		{
			/* invalid structure */
			retVal = CV_INVALID_KEY_PARAMETERS;
			goto err_exit;
		}
		/* use private key from 6T and domain parameters in ROM */
		dhPrivKeyLenBits = OTP_SIZE_KDC_PRIV*8;
		dhKeyLenBits = KDC_PUBKEY_LEN; 
		x = NV_6TOTP_DATA->Kdc_priv;
		m = (uint8_t *)&dh_default_m[0];
	} else {
		/* get key from handle provided and use that */
		memset(&dhObjProperties,0,sizeof(dhObjProperties));
		dhObjProperties.objHandle = *pKeyObjHandle;
		cvFindPINAuthEntry(authList, &dhObjProperties);
		if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &dhObjProperties, *authListLen, authList, inputParameters, &objPtr, 
			&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
			goto err_exit;

		/* auth succeeded, but need to make sure access auth granted */
		if (!(authFlags & CV_OBJ_AUTH_ACCESS))
		{
			/* access auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		/* make sure key can be used for key derivation */
		if ((retVal = cvFindObjAtribFlags(&dhObjProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
			goto err_exit;
		if (!(*flags & CV_ATTRIB_USE_FOR_KEY_DERIVATION))
		{
			retVal = CV_OBJECT_ATTRIBUTES_INVALID;
			goto err_exit;
		}
		/* validate this is a D-H key */
		if (objPtr->objType != CV_TYPE_DH_KEY)
		{
			retVal = CV_INVALID_KEY_PARAMETERS;
			goto err_exit;
		}

		/* set up pointers to key elements */
		dhKey = (cv_obj_value_key *)dhObjProperties.pObjValue;
		dhKeyLenBits = dhKey->keyLen;
		dhKeyLenBytes = dhKeyLenBits/8;
		m = &dhKey->key[2*dhKeyLenBytes];
		x = &dhKey->key[3*dhKeyLenBytes];
		dhPrivKeyLenBits = 8*cvGetNormalizedLength(x, dhKeyLenBytes);
	}
	/* now compute shared secret */
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if ((phx_diffie_hellman_shared_secret (
						cryptoHandle,
						x, dhPrivKeyLenBits,
						y, dhKeyLenBits,
						m, dhKeyLenBits,
						(uint8_t *)&keyTemp[0], &keyLenBytesOut,
						NULL, NULL)) != PHX_STATUS_OK)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	/* convert output from BIGINT format and do hash of result.  If key to be */
	/* generated is larger than 16 bytes (AES-128), SHA256 is used instead of SHA1 */
	hashLen = (keyLenBytes > SHA1_LEN) ? SHA256_LEN : SHA1_LEN;
	cvInplaceSwap((uint32_t *)&keyTemp[0], keyLenBytesOut/4);
	if ((retVal = cvAuth((uint8_t *)&keyTemp[0], keyLenBytesOut, NULL, hashTemp, hashLen,
		NULL, 0, 0)) != CV_SUCCESS)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	memcpy(key->key, hashTemp, keyLenBytes);

	objProperties->objValueLength = 2 /* key size */ + keyLenBytes;
	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&dhObjProperties, objPtr, callback, context);

err_exit:
	return retVal;
}
#pragma pop

cv_status
cvGenDSAKey(cv_obj_properties *objProperties, cv_obj_value_key *key, uint32_t keyLength,
		   uint32_t extraParamsLength, void *pExtraParams)
{
	/* this routine creates a DSA key */
	cv_status retVal = CV_SUCCESS;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint8_t *keyParams;
	uint32_t keyLenBytes, privKeyLen;
	union 
	{
		uint32_t privKeyWords[DSA2048_PRIV_LEN/sizeof(uint32_t)];
		uint8_t  privKeyBytes[DSA2048_PRIV_LEN];
	} privKeyLocal;

	/* validate the key size */
	if (!(keyLength == 1024 || keyLength == 2048))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	key->keyLen = keyLength;
	/* extra parameters are cv_dsa_keygen_params (no actual structure since variable length) */
	/* validate extra parameters length */
	keyLenBytes = keyLength/8;
	privKeyLen = (keyLength == DSA1024_KEY_LEN_BITS) ? DSA1024_PRIV_LEN : DSA2048_PRIV_LEN;
	if (extraParamsLength != (privKeyLen + 2*keyLenBytes))
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	keyParams = pExtraParams;
	/* copy p, q and g into key.  do appropriate swap to put into BIGINT format */
	cvCopySwap((uint32_t *)&keyParams[0], (uint32_t *)&key->key[0], keyLenBytes);
	cvCopySwap((uint32_t *)&keyParams[keyLenBytes], (uint32_t *)&key->key[keyLenBytes], privKeyLen);
	cvCopySwap((uint32_t *)&keyParams[keyLenBytes + privKeyLen], 
		(uint32_t *)&key->key[keyLenBytes + privKeyLen], keyLenBytes);
	/* now compute DSA key */
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if ((phx_dsa_key_generate (
						cryptoHandle,
						&key->key[0], keyLength,						// p 
						&key->key[keyLenBytes], privKeyLen*8,			// q 
						&key->key[keyLenBytes + privKeyLen],			// g 
						&privKeyLocal.privKeyBytes[0],
						&key->key[2*keyLenBytes + privKeyLen])) != PHX_STATUS_OK)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	/* parameter uses local value, because phx_rng_fips_generate() expects 32-bit aligned value */
	memcpy(&key->key[3*keyLenBytes + privKeyLen], privKeyLocal.privKeyBytes, privKeyLen);
	objProperties->objValueLength = 2 /* key size */ + 3*keyLenBytes + 2*privKeyLen;

err_exit:
	return retVal;
}

cv_status
cvGenECCKey(cv_obj_properties *objProperties, cv_obj_value_key *key, uint32_t keyLength,
		   uint32_t extraParamsLength, void *pExtraParams)
{
	/* this routine creates an ECC key */
	cv_status retVal = CV_SUCCESS;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint8_t *keyParams;
	uint32_t keyLenBytes, keyLenWords;
	uint8_t *p, *a, *b, *n, *h, *Gx, *Gy, *Q, *x;

	/* validate the key size */
	if (!(keyLength == 192 || keyLength == 224 || keyLength == 256 || keyLength == 384))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	key->keyLen = keyLength;
	/* extra parameters are cv_ec_generate_keygen_params (no actual structure since variable length) */
	/* validate extra parameters length */
	keyLenBytes = keyLength/8;
	keyLenWords = keyLenBytes/4;
	if (extraParamsLength != (7*keyLenBytes))
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	keyParams = pExtraParams;
	/* copy p, a, b, n, h and G into key.  do appropriate swap to put into BIGINT format */
	p = &key->key[0];
	a = &key->key[keyLenBytes];
	b = &key->key[2*keyLenBytes];
	n = &key->key[3*keyLenBytes];
	h = &key->key[4*keyLenBytes];
	Gx = &key->key[5*keyLenBytes];
	Gy = &key->key[6*keyLenBytes];
	Q = &key->key[7*keyLenBytes];
	x = &key->key[9*keyLenBytes];
	cvCopySwap((uint32_t *)&keyParams[0], (uint32_t *)p, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[keyLenBytes], (uint32_t *)a, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[2*keyLenBytes], (uint32_t *)b, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[3*keyLenBytes], (uint32_t *)h, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[4*keyLenBytes], (uint32_t *)n, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[5*keyLenBytes], (uint32_t *)Gx, keyLenWords*4);
	cvCopySwap((uint32_t *)&keyParams[6*keyLenBytes], (uint32_t *)Gy, keyLenWords*4);
	/* now compute public portion of EC key */
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if (phx_ecp_diffie_hellman_generate(cryptoHandle, PRIME_FIELD, p, keyLength, a,
		b, n, Gx, Gy, x, FALSE, Q,  Q + keyLenBytes,
		NULL, NULL) != PHX_STATUS_OK)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	objProperties->objValueLength = 2 /* key size */ + 10*keyLenBytes;

err_exit:
	return retVal;
}

cv_status
cvGenECCSharedSecret(cv_obj_properties *objProperties, cv_obj_type objType, cv_obj_value_key *key, uint32_t keyLength,
		   uint32_t extraParamsLength, void *pExtraParams, cv_input_parameters *inputParameters,
		   cv_callback *callback, cv_callback_ctx context)
{
	/* this routine creates an ECC shared secret */
	cv_status retVal = CV_SUCCESS;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_obj_value_key *eccKey;
	uint32_t keyLenBytes, eccKeyLenBytes;
	uint32_t *pubKeyLen, *authListLen, pubKeyLenBytes;
	uint8_t *pubKey, *y;
	cv_obj_handle *pKeyObjHandle;
	cv_auth_list *authList;
	uint8_t *p, *a, *b, *n, *x;
	cv_obj_hdr *objPtr;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_attrib_flags *flags;
	uint32_t keyTemp[ECC256_POINT/4];
	uint8_t hashTemp[SHA256_LEN];
	uint32_t hashLen;
	cv_obj_properties ecdhObjProperties;

#define FIXED_SIZE_PARAM_ELEMENTS_ECC (4 + 4 + 4)

	/* validate the key size based on type of symmetric key being created */
	if ((objType == CV_TYPE_AES_KEY && !(keyLength == 128 || keyLength == 192 || keyLength == 256)))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	keyLenBytes = keyLength/8;
	key->keyLen = keyLength;
	/* extra parameters are cv_ec_shared_secret_keygen_params (no actual structure since variable length) */
	/* validate and set up pointers to parameters */
	pubKeyLen = (uint32_t *)pExtraParams;
	pubKeyLenBytes = *pubKeyLen;
	pubKey = (uint8_t *)(pubKeyLen + 1);
	if ((pubKeyLenBytes + FIXED_SIZE_PARAM_ELEMENTS_ECC) > extraParamsLength)
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	pKeyObjHandle = (cv_obj_handle *)(pubKey + pubKeyLenBytes);
	authListLen = (uint32_t *)(pKeyObjHandle + 1);
	authList = (cv_auth_list *)(authListLen + 1);
	if ((pubKeyLenBytes + FIXED_SIZE_PARAM_ELEMENTS_ECC + *authListLen) != extraParamsLength)
	{
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	y = pubKey;
	/* need to put pubkey into BIGINT format */
	cvInplaceSwap((uint32_t *)y, pubKeyLenBytes/8);
	cvInplaceSwap((uint32_t *)&y[pubKeyLenBytes/2], pubKeyLenBytes/8);

	/* get key from handle provided and use that */
	memset(&ecdhObjProperties,0,sizeof(ecdhObjProperties));
	ecdhObjProperties.objHandle = *pKeyObjHandle;
	cvFindPINAuthEntry(authList, &ecdhObjProperties);
	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &ecdhObjProperties, *authListLen, authList, inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* make sure key can be used for key derivation */
	if ((retVal = cvFindObjAtribFlags(&ecdhObjProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (!(*flags & CV_ATTRIB_USE_FOR_KEY_DERIVATION))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}
	/* validate this is an ECC key */
	if (objPtr->objType != CV_TYPE_ECC_KEY)
	{
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}

	/* set up pointers to key elements */
	eccKey = (cv_obj_value_key *)ecdhObjProperties.pObjValue;
	eccKeyLenBytes = eccKey->keyLen/8;
	p = &eccKey->key[0];
	a = &eccKey->key[eccKeyLenBytes];
	b = &eccKey->key[2*eccKeyLenBytes];
	n = &eccKey->key[3*eccKeyLenBytes];
	x = &eccKey->key[9*eccKeyLenBytes];

	/* now compute shared secret */
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	

	if ((phx_ecp_diffie_hellman_shared_secret(cryptoHandle, PRIME_FIELD, (uint8_t *)p,
		eccKey->keyLen, (uint8_t *)a, (uint8_t *)b, (uint8_t *)n,
		x, y, y + eccKeyLenBytes,
		(uint8_t *)&keyTemp[0], (uint8_t *)&keyTemp[eccKeyLenBytes/4],
		NULL, NULL)) != PHX_STATUS_OK)			
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	/* convert output from BIGINT format and do hash of result.  If key to be */
	/* generated is larger than 16 bytes (AES-128), SHA256 is used instead of SHA1 */
	hashLen = (keyLenBytes > SHA1_LEN) ? SHA256_LEN : SHA1_LEN;
	cvInplaceSwap(&keyTemp[0], eccKeyLenBytes/4);
	if ((retVal = cvAuth((uint8_t *)&keyTemp[0], eccKeyLenBytes, NULL, hashTemp, hashLen,
		NULL, 0, 0)) != CV_SUCCESS)
	{
		retVal = CV_KEY_GENERATION_FAILURE;
		goto err_exit;
	}
	memcpy(key->key, hashTemp, keyLenBytes);

	objProperties->objValueLength = 2 /* key size */ + keyLenBytes;
	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&ecdhObjProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cvGenAESKey(cv_obj_properties *objProperties, cv_obj_value_key *key, uint32_t keyLength)
{
	/* this routine creates an AES key */
	cv_status retVal = CV_SUCCESS;
	uint32_t kenLenBytes;

	/* validate the key size */
	if (!(keyLength == 128 || keyLength == 192 || keyLength == 256))
	{
		/* invalid size */
		retVal = CV_INVALID_KEY_LENGTH;
		goto err_exit;
	}
	key->keyLen = keyLength;
	kenLenBytes = keyLength/8;
	/* now compute AES key */
	if ((retVal = cvRandom(kenLenBytes, key->key)) != CV_SUCCESS)
		goto err_exit;

	objProperties->objValueLength = 2 /* key size */ + kenLenBytes;

err_exit:
	return retVal;
}

cv_status
cvGenRSAOTPToken(cv_obj_properties *objProperties, uint8_t *token, uint32_t extraParamsLength, void *pExtraParams)
{
	/* this routine creates an RSA OTP token */
	cv_status retVal = CV_SUCCESS;
	cv_otp_rsa_keygen_params_direct *tokenParamsDirect;
	cv_otp_rsa_keygen_params_ctkip *tokenParamsCtkip;
	cv_obj_value_rsa_token *rsaToken = (cv_obj_value_rsa_token *)token;
	uint8_t message[4 + 14 + CV_MAX_CTKIP_PUBKEY_LEN + AES_128_BLOCK_LEN];  /* message for CTKIP shared secret computation */
	uint8_t *messageNext = &message[0];
	uint32_t messageLen;

	/* extra parameters are cv_otp_rsa_keygen_params */
	/* check to see provisioning type, to determine parameter format */
	tokenParamsDirect = (cv_otp_rsa_keygen_params_direct *)pExtraParams;
	rsaToken->type = CV_OTP_TYPE_RSA;
	rsaToken->rsaParams = tokenParamsDirect->rsaParams;
	rsaToken->sharedSecretLen = AES_128_BLOCK_LEN;
	if (tokenParamsDirect->otpProvType == CV_OTP_PROV_DIRECT)
	{
		/* validate parameters */
		if (extraParamsLength != sizeof(cv_otp_rsa_keygen_params_direct))
		{
			retVal = CV_INVALID_KEY_PARAMETERS;
			goto err_exit;
		}
		/* this is the direct provisioning method, just copy shared secret into token */
		memcpy(rsaToken->sharedSecret, tokenParamsDirect->masterSeed, AES_128_BLOCK_LEN);
		objProperties->objValueLength = sizeof(cv_obj_value_rsa_token);
	} else if (tokenParamsDirect->otpProvType == CV_OTP_PROV_CTKIP) {
		/* validate parameters */
		tokenParamsCtkip = (cv_otp_rsa_keygen_params_ctkip *)pExtraParams;
		if ((tokenParamsCtkip->serverPubkeyLen > CV_MAX_CTKIP_PUBKEY_LEN) 
			|| extraParamsLength != (sizeof(cv_otp_rsa_keygen_params_ctkip) - 1 + tokenParamsCtkip->serverPubkeyLen))
		{
			retVal = CV_INVALID_KEY_PARAMETERS;
			goto err_exit;
		}
		memcpy(rsaToken->clientNonce, tokenParamsCtkip->clientNonce, AES_128_BLOCK_LEN);
		/* this is the ctkip provisioning method, must compute shared secret */
		*((uint32_t *)message) = 1;
		messageNext += sizeof(uint32_t);
		messageNext = memcpy(messageNext, "Key generation", 14);
		messageNext = memcpy(messageNext, tokenParamsCtkip->serverPubkey, tokenParamsCtkip->serverPubkeyLen);
		messageNext = memcpy(messageNext, tokenParamsCtkip->serverNonce, AES_128_BLOCK_LEN);
		messageLen = messageNext - message;
		if ((retVal = ctkipPrfAes(tokenParamsCtkip->clientNonce, message, messageLen, AES_128_BLOCK_LEN, rsaToken->sharedSecret)) != CV_SUCCESS)
			goto err_exit;
	} else {
		/* invalid structure */
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	objProperties->objValueLength = sizeof(cv_obj_value_rsa_token);

err_exit:
	return retVal;
}

cv_status
cvGenOATHOTPToken(cv_obj_properties *objProperties, uint8_t *token, uint32_t extraParamsLength, void *pExtraParams)
{
	/* this routine creates an OATH OTP token */
	cv_status retVal = CV_SUCCESS;
	cv_otp_oath_keygen_params *tokenParamsCtkip;
	cv_obj_value_oath_token *oathToken = (cv_obj_value_oath_token *)token;
	uint8_t message[4 + 14 + CV_MAX_CTKIP_PUBKEY_LEN + AES_128_BLOCK_LEN];  /* message for CTKIP shared secret computation */
	uint8_t *messageNext = &message[0];
	uint32_t messageLen;

	/* extra parameters are cv_otp_oath_keygen_params */
	oathToken->type = CV_OTP_TYPE_OATH;
	oathToken->sharedSecretLen = CV_NONCE_LEN;
	oathToken->countValue = 0;
	/* validate parameters */
	tokenParamsCtkip = (cv_otp_oath_keygen_params *)pExtraParams;
	if ((tokenParamsCtkip->serverPubkeyLen > CV_MAX_CTKIP_PUBKEY_LEN) 
		|| extraParamsLength != (sizeof(cv_otp_oath_keygen_params) - 1 + tokenParamsCtkip->serverPubkeyLen))
	{
		retVal = CV_INVALID_KEY_PARAMETERS;
		goto err_exit;
	}
	memcpy(oathToken->clientNonce, tokenParamsCtkip->clientNonce, AES_128_BLOCK_LEN);
	/* this is the ctkip provisioning method, must compute shared secret */
	*((uint32_t *)message) = 1;
	messageNext += sizeof(uint32_t);
	messageNext = memcpy(messageNext, "Key generation", 14);
	messageNext = memcpy(messageNext, tokenParamsCtkip->serverPubkey, tokenParamsCtkip->serverPubkeyLen);
	messageNext = memcpy(messageNext, tokenParamsCtkip->serverNonce, AES_128_BLOCK_LEN);
	messageLen = messageNext - message;
	if ((retVal = ctkipPrfAes(tokenParamsCtkip->clientNonce, message, messageLen, CV_NONCE_LEN, oathToken->sharedSecret)) != CV_SUCCESS)
		goto err_exit;

	objProperties->objValueLength = sizeof(cv_obj_value_oath_token);

err_exit:
	return retVal;
}

cv_status
cv_genkey (cv_handle cvHandle, cv_obj_type objType, cv_key_type keyType, uint32_t keyLength,                                            
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t extraParamsLength,    void *pExtraParams,     
			cv_obj_handle *pKeyObjHandle,  
			cv_callback *callback, cv_callback_ctx context)
{
	/* This routine computes a key of the indicated object and key type, creates an object for this key */
	/* and returns the corresponding handle. */
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_auth_entry_PIN *PIN;
	cv_auth_hdr *PINhdr;
	uint32_t pinLen, remLen;
	uint8_t obj[MAX_CV_OBJ_SIZE];
	uint8_t *pObjValLocal = &obj[0];
	cv_obj_value_key *key = (cv_obj_value_key *)pObjValLocal;
	cv_input_parameters inputParameters;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 9, sizeof(cv_obj_type), &objType, 
		sizeof(cv_key_type), &keyType, sizeof(uint32_t), &keyLength, sizeof(uint32_t), &objAttributesLength,
		objAttributesLength, pObjAttributes, sizeof(uint32_t), &authListsLength,
		authListsLength, pAuthLists, sizeof(uint32_t), &extraParamsLength, extraParamsLength, pExtraParams, 0, NULL);

	/* validate new auth lists provided */
	if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
		goto err_exit;
	if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
		goto err_exit;
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	objProperties.objAttributesLength = objAttributesLength;
	objProperties.pObjAttributes = pObjAttributes;
	objProperties.authListsLength = authListsLength;
	objProperties.pAuthLists = pAuthLists;
	objProperties.pObjValue = pObjValLocal;

	/* determine what type of key to be created */
	switch (keyType)
	{
	case CV_RSA:
		if (objType != CV_TYPE_RSA_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenRSAKey(&objProperties, key, keyLength)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_DH_GENERATE:
		if (objType != CV_TYPE_DH_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenDHGen(&objProperties, key, keyLength, extraParamsLength, pExtraParams)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_DH_SHARED_SECRET:
		if (objType != CV_TYPE_AES_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenDHSharedSecret(&objProperties, objType, key, keyLength, extraParamsLength, pExtraParams,
			&inputParameters, callback, context)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_DSA:
		if (objType != CV_TYPE_DSA_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenDSAKey(&objProperties, key, keyLength, extraParamsLength, pExtraParams)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_ECC:
		if (objType != CV_TYPE_ECC_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenECCKey(&objProperties, key, keyLength, extraParamsLength, pExtraParams)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_ECDH_SHARED_SECRET:
		if (objType != CV_TYPE_AES_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenECCSharedSecret(&objProperties, objType, key, keyLength, extraParamsLength, pExtraParams,
			&inputParameters, callback, context)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_AES:
		if (objType != CV_TYPE_AES_KEY)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenAESKey(&objProperties, key, keyLength)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_OTP_RSA:
		if (objType != CV_TYPE_OTP_TOKEN)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenRSAOTPToken(&objProperties, (uint8_t *)key, extraParamsLength, pExtraParams)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_OTP_OATH:
		if (objType != CV_TYPE_OTP_TOKEN)
		{
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
		if ((retVal = cvGenOATHOTPToken(&objProperties, (uint8_t *)key, extraParamsLength, pExtraParams)) != CV_SUCCESS)
			goto err_exit;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_KEY_TYPE;
		goto err_exit;
	};
	/* check to make sure object not too large */
	if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_OBJECT_SIZE;
		goto err_exit;
	}
	objProperties.objHandle = 0;
	objPtr.objType = objType;
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
	/* now save object */
	retVal = cvHandlerPostObjSave(&objProperties, &objPtr, callback, context);
	*pKeyObjHandle = objProperties.objHandle;

err_exit:
	return retVal;
}

#define XOR32(a,b,x)  (*(uint32_t*)(x)=*(uint32_t*)(a)^*(uint32_t*)(b)) 

/*----------------------------------------------------------------------
 * Name    : XOR128
 * Purpose : XOR two 128 bit blocks
 * Input   : block a, block b 
 * Output  : block out
 *---------------------------------------------------------------------*/
void XOR128(uint8_t *a, uint8_t *b, uint8_t *out)
{
	XOR32(a   ,b   ,out);
	XOR32(a+4 ,b+4 ,out+4);
	XOR32(a+8 ,b+8 ,out+8);
	XOR32(a+12,b+12,out+12);
}

/*----------------------------------------------------------------------
 * Name    : shiftLeft
 * Purpose : shiftLeft a 128 bit block
 * Input   : block in
 * Output  : block out
 *---------------------------------------------------------------------*/
void shiftLeft(uint8_t *in, uint8_t *out)
{
    int n;
    uint8_t overflow = 0;

	// Start from the end and keep track of the overflow
    for (n = 15; n >= 0; n--) {
        out[n] = in[n] << 1;
        out[n] |= overflow;
        overflow = (in[n] & 0x80)? 1: 0;
    }
    return;
}

/*----------------------------------------------------------------------
 * Name    : makeK1K2
 * Purpose : Create the two AES CMAC keys K1, K2 from the root key
 * Input   : key, keySize 
 * Output  : K1, K2
 *---------------------------------------------------------------------*/
void makeK1K2(uint8_t *key, uint32_t keySize, uint8_t *K1, uint8_t *K2)
{
    uint8_t L[AES_BLOCK_LENGTH];
    uint8_t Z[AES_BLOCK_LENGTH];
    uint8_t tmp[AES_BLOCK_LENGTH];
	uint8_t R[AES_BLOCK_LENGTH] = {
    	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
	};
	cv_bulk_params localBulkParams;
	uint8_t keyLocal[sizeof(cv_obj_value_key) - 1 + AES_128_BLOCK_LEN];
	cv_obj_value_key *pKey = (cv_obj_value_key *)&keyLocal[0];
	uint32_t encryptedLength = AES_128_BLOCK_LEN;

	memset (Z, 0, AES_BLOCK_LENGTH);
	// Encrypt a null block and save it as L
//    AES_oneblock(key, keySize, Z, L);
	memset(&localBulkParams,0,sizeof(localBulkParams));
	localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
	localBulkParams.authOffset = 0;
	localBulkParams.bulkMode = CV_ECB_NOPAD;
	localBulkParams.encOffset = 0;
	pKey->keyLen = 128;
	memcpy(&pKey->key, key, AES_BLOCK_LENGTH);
	if ((cvBulkEncrypt(Z, AES_BLOCK_LENGTH, L, &encryptedLength, NULL, NULL, 
		CV_BULK_ENCRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
		goto err_exit;

	// Check for the high order bit of L
	// This calculates the value of the first key K1
    if ((L[0] & 0x80)) { 
        shiftLeft (L, tmp);
        XOR128 (tmp, R, K1);
	}
    else 
        shiftLeft (L, K1);

	// Check for the high order bit of K1
	// This calculates the value of the second key K2
    if ((K1[0] & 0x80)) {
        shiftLeft (K1, tmp);
        XOR128 (tmp, R, K2);
    }
    else 
        shiftLeft (K1, K2);

err_exit:
    return;
}

/*----------------------------------------------------------------------
 * Name    : cvAESCMAC
 * Purpose : Perform AES CMAC on the input data put the result in the digest
 * Input   : data, dataLen, key, keySize
 * Output  : digest
 *---------------------------------------------------------------------*/
cv_status
cvAESCMAC(uint8_t *data, uint32_t dataLen, uint8_t *key, uint32_t keySize, uint8_t *digest)
{
	cv_status retVal = CV_SUCCESS; 
	uint8_t X[AES_BLOCK_LENGTH];
    uint8_t Y[AES_BLOCK_LENGTH];
    uint8_t lastBlock[AES_BLOCK_LENGTH];
    uint8_t pad[AES_BLOCK_LENGTH];
    uint8_t K1[AES_BLOCK_LENGTH]; 
	uint8_t K2[AES_BLOCK_LENGTH];
    int blockAligned = FALSE; 
	int padLen = 0;
	int i, blocks,j;
	cv_bulk_params localBulkParams;
	uint8_t keyLocal[sizeof(cv_obj_value_key) - 1 + AES_128_BLOCK_LEN];
	cv_obj_value_key *pKey = (cv_obj_value_key *)&keyLocal[0];
	uint32_t encryptedLength = AES_128_BLOCK_LEN;

	// Create the two keys for the AES CMAC operation
    makeK1K2 (key, keySize, K1,K2);
	// Calculate the number of blocks
    blocks = (dataLen+(AES_BLOCK_LENGTH-1)) / AES_BLOCK_LENGTH;
    if (!blocks) 
        blocks = 1;
    else if (!(dataLen%AES_BLOCK_LENGTH)) 
            blockAligned = TRUE;

    if (blockAligned) 
		// If aligned use the K1 key to initialize the last block
        XOR128(&data[AES_BLOCK_LENGTH*(blocks-1)], K1, lastBlock);
    else {
		// If not aligned then pad it then use the K2 key 
        padLen = AES_BLOCK_LENGTH - (dataLen%AES_BLOCK_LENGTH);
        memcpy (pad, &data[AES_BLOCK_LENGTH*(blocks-1)], dataLen%AES_BLOCK_LENGTH);
		pad [dataLen%AES_BLOCK_LENGTH] = 0x80;
		j=(dataLen%AES_BLOCK_LENGTH);
		for (i = 1,j+=i; i < padLen && j < AES_BLOCK_LENGTH; i++,j++)
		{
			pad[j] = 0;
		}
        XOR128 (pad, K2, lastBlock);
    }

	memset (X, 0, AES_BLOCK_LENGTH);
    for (i = 0; i < blocks; i++) {
		// Perform the block calculation then encrypt it
		if (i < blocks - 1)
        	XOR128 (X, &data[AES_BLOCK_LENGTH*i], Y);
		else
    		XOR128(X, lastBlock, Y);
 //       AES_oneblock (key, keySize, Y, X); 
		memset(&localBulkParams,0,sizeof(localBulkParams));
		localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
		localBulkParams.authOffset = 0;
		localBulkParams.bulkMode = CV_ECB_NOPAD;
		localBulkParams.encOffset = 0;
		pKey->keyLen = 128;
		memcpy(&pKey->key, key, AES_BLOCK_LENGTH);
		if ((retVal = cvBulkEncrypt(Y, AES_BLOCK_LENGTH, X, &encryptedLength, NULL, NULL, 
			CV_BULK_ENCRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
			goto err_exit;
    }

	memcpy (digest, X, AES_BLOCK_LENGTH);

err_exit:
	return retVal;;
}

cv_status
cvBulkEncrypt(byte *pCleartext, uint32_t cleartextLength, byte *pEncData, uint32_t  *pEncLength,
			  uint8_t *hashOut, uint32_t *hashOutLen, cv_enc_op encryptionOperation, cv_bulk_params *pBulkParameters, 
			  cv_obj_type objType, cv_obj_value_key *key, cv_obj_value_hash *HMACkey)
{
	/* this routine does a bulk encryption */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cipher_cmd;
	cv_status retVal = CV_SUCCESS;
	uint8_t bufIn[CV_STACK_BUFFER_SIZE]; /* size buffer for max size object + HMAC + pad */
	uint8_t bufOut[CV_STACK_BUFFER_SIZE] = {0};
	uint32_t computedEncLen, padLen = 0, cleartextAuthLength = cleartextLength - pBulkParameters->authOffset;
	uint32_t authLen = 0, authKeyLen = 0;
	uint8_t *localHMACKeyPtr = NULL;
	uint32_t IV[AES_BLOCK_LENGTH/sizeof(uint32_t)];
	uint8_t *pIV = (uint8_t *)&IV[0];
//	uint32_t unalignedLength = cleartextLength - pBulkParameters->encOffset;

	/* copy cleartext data locally, in case needs to be padded */
	if (cleartextLength > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_INPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	if (HMACkey != NULL)
		localHMACKeyPtr = &HMACkey->hash[0];
	memcpy(bufIn, pCleartext, cleartextLength);
	computedEncLen = cleartextLength - pBulkParameters->encOffset;
	memset(&cipher_cmd,0,sizeof(cipher_cmd));
	cipher_cmd.cipher_mode  = PHX_CIPHER_MODE_ENCRYPT;
	if (objType == CV_TYPE_AES_KEY)
	{
		/* key should have been validated when created, so should have valid key length */
		if (key->keyLen == 128)
		{
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_128;
		} else if (key->keyLen == 192) {
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_192;
		} else if (key->keyLen == 256) {
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_256;
		}
		padLen = GET_AES_128_PAD_LEN(computedEncLen);  /* pad is same for all key lengths */

		/* make sure output length is sufficient */
		computedEncLen += padLen;
		if (computedEncLen > *pEncLength)
		{
			*pEncLength = computedEncLen;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		/* zero pad area */
		memset(bufIn + cleartextLength, 0, padLen); 
		cleartextLength = computedEncLen;
	} else {
		/* invalid symmetric key type */
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	switch (pBulkParameters->bulkMode)
	{
	case CV_CBC:
	case CV_CBC_NOPAD:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CBC;
		break;
	case CV_ECB:
	case CV_ECB_NOPAD:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_ECB;
		break;
	case CV_CTR:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CTR;
		break;
	/* TBD: these aren't yet defined in SCAPI */
	case CV_CMAC:
	case CV_CCMP:
	case 0xffff:
	default:
		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	switch (pBulkParameters->authAlgorithm)
	{
	case CV_AUTH_ALG_NONE:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_NONE;
		cleartextAuthLength = 0;
		break;
	case CV_AUTH_ALG_HMAC_SHA1:
		cipher_cmd.auth_alg    = BCM_SCAPI_AUTH_ALG_SHA1;
		cipher_cmd.auth_optype    = BCM_SCAPI_AUTH_OPTYPE_FULL;
		cipher_cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
		if (HMACkey == NULL || HMACkey->hashType != CV_SHA1)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		authLen = SHA1_LEN;
		authKeyLen = SHA1_LEN;
		break;
	case CV_AUTH_ALG_SHA1:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_SHA1;
		authLen = SHA1_LEN;
		break;
	case CV_AUTH_ALG_HMAC_SHA2_256:
		cipher_cmd.auth_alg    = BCM_SCAPI_AUTH_ALG_SHA256;
		cipher_cmd.auth_optype    = BCM_SCAPI_AUTH_OPTYPE_FULL;
		cipher_cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
		if (HMACkey == NULL || HMACkey->hashType != CV_SHA256)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		authLen = SHA256_LEN;
		authKeyLen = SHA256_LEN;
		break;
	case CV_AUTH_ALG_SHA2_256:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_SHA256;
		authLen = SHA256_LEN;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	/* now recheck size of output buffer, including auth, which SCAPI appends to end of encrypted data */
	if ((computedEncLen + authLen) > CV_STACK_BUFFER_SIZE)
	{
		retVal = CV_INVALID_INPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	switch (encryptionOperation)
	{
	case CV_BULK_ENCRYPT: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_NULL;
		break;
	case CV_BULK_ENCRYPT_THEN_AUTH: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_CRYPT_AUTH;
		break;
	case CV_BULK_AUTH_THEN_ENCRYPT: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_AUTH_CRYPT;
		break;
	case 0xffff:
	default:
 		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	/* also validate bulk parameters offsets */
	if (pBulkParameters->encOffset > cleartextLength || pBulkParameters->authOffset > cleartextLength)
	{
 		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);
	/* do encryption op */
    if ((phx_bulk_cipher_init(cryptoHandle, &cipher_cmd, key->key, localHMACKeyPtr, authKeyLen, &ctx)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}

	/* copy IV locally to ensure 4-byte alignment */
	memcpy(&IV[0],pBulkParameters->IV,sizeof(IV));
	/* check to see if NULL IV */
	if (pBulkParameters->IVLen == 0)
		pIV = NULL;
	else if (pBulkParameters->IVLen != AES_BLOCK_LENGTH)
	{
		 retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	if (((phx_bulk_cipher_start(cryptoHandle, pCleartext, pIV, computedEncLen, 
		pBulkParameters->encOffset, cleartextAuthLength, 
		pBulkParameters->authOffset, bufOut, &ctx))) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}
 	/* swaps endianness, both input and output */
    phx_bulk_cipher_swap_endianness(cryptoHandle, &ctx, TRUE, TRUE, TRUE);
    if ((phx_bulk_cipher_dma_start (cryptoHandle, &ctx, 0, NULL, 0)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}
	/* now copy to output */
	/* if there is auth data, it's after the encrypted data */
	if (authLen)
	{
		*hashOutLen = authLen;
		memcpy(hashOut, bufOut + computedEncLen, authLen);
	}
	*pEncLength = computedEncLen;
	memcpy(pEncData, bufOut, computedEncLen);

err_exit:
	return retVal;
}

cv_status
cvAsymEncrypt(byte *pCleartext, uint32_t cleartextLength, byte *pEncData, uint32_t  *pEncLength,
			  cv_obj_type objType, cv_obj_value_key *key)
{
	/* this routine does an asymmetric encryption */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_status retVal = CV_SUCCESS;
	uint32_t computedEncLen;
	uint32_t bufIn[MAX_CV_OBJ_SIZE/sizeof(uint32_t)];
	uint8_t *m, *e;
	uint32_t keyLenBytes;

	computedEncLen = key->keyLen/8;
	/* make sure can support cleartext encryption length */
	if (cleartextLength > computedEncLen)
	{
		retVal = CV_INVALID_INPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	/* make sure output buffer large enough */
	if (computedEncLen > *pEncLength)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	if (objType != CV_TYPE_RSA_KEY)
	{
		/* invalid key type */
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	/* do pkcs1 v1.5 encode */
	if ((retVal = cv_pkcs1_v15_encrypt_encode(cleartextLength,  pCleartext, computedEncLen, (uint8_t *)&bufIn[0])) != CV_SUCCESS)
		goto err_exit;
	/* get pointers to key params */
	keyLenBytes = key->keyLen/8;
	e = &key->key[keyLenBytes];
	m = &key->key[0];
	/* convert from network byte order (big endian) to little endian */
	cvInplaceSwap(&bufIn[0], computedEncLen/sizeof(uint32_t));
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if ((phx_rsa_mod_exp(cryptoHandle, (uint8_t *)&bufIn[0], computedEncLen*8, 
		m, keyLenBytes*8,
		e, sizeof(uint32_t)*8,
		pEncData, pEncLength,
		NULL, NULL)) != PHX_STATUS_OK)
	{
		retVal = CV_ENCRYPTION_FAILURE;
		goto err_exit;
	}
	/* convert from little endian to network byte order (big endian) */
	cvInplaceSwap((uint32_t *)pEncData, *pEncLength/sizeof(uint32_t));

err_exit:
	return retVal;
}

cv_status
cv_encrypt (cv_handle cvHandle, cv_obj_handle encKeyHandle,           
			uint32_t authListLength, cv_auth_list *pAuthList,                                 
			cv_enc_op encryptionOperation,                              
			uint32_t cleartextLength, byte *pCleartext,            
			cv_bulk_params *pBulkParameters,                            
			uint32_t *pEncLength, byte *pEncData,                     
			cv_callback *callback, cv_callback_ctx context) 
{
	/* This routine gets the key object and performs authorization then does encryption of the data provided, */
	/* based on the key type and other parameters provided. */
	cv_obj_properties objProperties, HMACobjProperties;
	cv_obj_hdr *objPtr, *HMACobjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE, needObjWriteHMACKey = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key;
	cv_obj_value_hash *HMACkey, *hashObj;
	cv_obj_type objType;
	uint8_t hashOut[SHA256_LEN];
	uint32_t hashOutLen = 0;
	cv_bulk_params savedBulkParams;
	cv_attrib_flags *flags;
	cv_bool isBulkOperation = FALSE;
	uint32_t padLen = 0;
	uint8_t padByte;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* make sure bulk parameters provided, if needed */
	if (encryptionOperation == CV_BULK_ENCRYPT || encryptionOperation == CV_BULK_ENCRYPT_THEN_AUTH 
			||  encryptionOperation == CV_BULK_AUTH_THEN_ENCRYPT)
		isBulkOperation = TRUE;
	if (isBulkOperation && pBulkParameters == NULL)
		return CV_INVALID_INPUT_PARAMETER;
	if (cleartextLength == 0)
		return CV_INVALID_INPUT_PARAMETER;

	memset(&savedBulkParams,0,sizeof(cv_bulk_params));

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(encKeyHandle), &encKeyHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_enc_op), &encryptionOperation,
		sizeof(uint32_t), &cleartextLength, cleartextLength, pCleartext, sizeof(cv_bulk_params), pBulkParameters,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = encKeyHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* make sure encryption key can be used for this purpose */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (!(*flags & CV_ATTRIB_USE_FOR_ENCRYPTION))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* check to see if need to get HMAC key object */
	if (isBulkOperation && pBulkParameters->hAuthKey != 0)
	{
		/* this assumes that HMAC key object is available, otherwise will fail */
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = pBulkParameters->hAuthKey;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		if (HMACobjPtr->objType != CV_TYPE_HASH)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		/* now check that auth already done for encryption key authorizes HMAC key */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
						objProperties.authListsLength, objProperties.pAuthLists,
						HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
						0, &needObjWriteHMACKey)) != CV_SUCCESS)
			goto err_exit;

		HMACkey = (cv_obj_value_hash *)HMACobjProperties.pObjValue;
	} else
		HMACkey = NULL;

	/* now do authorization for output hash, if any */
	if (isBulkOperation && pBulkParameters->hHashObj != 0)
	{
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = pBulkParameters->hHashObj;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		/* now check that auth already done for encryption key authorizes HMAC key */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
						objProperties.authListsLength, objProperties.pAuthLists,
						HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
						0, NULL)) != CV_SUCCESS)
			goto err_exit;
		/* need to make sure auth granted allows write as well */
		if (!(authFlags & CV_OBJ_AUTH_WRITE_PUB))
		{
			/* write auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}
	/* check auth for output hash/HMAC if there is one.  this is done before encryption because auth list in */
	/* parameter list will get overwritten by encryption */
	if (isBulkOperation)
		memcpy(&savedBulkParams, pBulkParameters, sizeof(cv_bulk_params));
	if (isBulkOperation && pBulkParameters->hHashObj != 0)
	{
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = pBulkParameters->hHashObj;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		/* now check that auth already done for decryption key authorizes hash object write */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
						objProperties.authListsLength, objProperties.pAuthLists,
						HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
						0, NULL)) != CV_SUCCESS)
			goto err_exit;
		/* need to make sure auth granted allows write as well */
		if (!(authFlags & CV_OBJ_AUTH_WRITE_PUB))
		{
			/* write auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	/* now do requested encyption */
	key = (cv_obj_value_key *)objProperties.pObjValue;
	objType = objPtr->objType;
	switch (encryptionOperation)
	{
	case CV_BULK_ENCRYPT:
	case CV_BULK_ENCRYPT_THEN_AUTH:
	case CV_BULK_AUTH_THEN_ENCRYPT:
		if (pBulkParameters == NULL)
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		/* do PKCS5 padding.  note that for CTR mode (w/o auth), this is only done to workaround h/w issue and is removed afterwards */
		if (pBulkParameters->bulkMode != CV_CBC_NOPAD && pBulkParameters->bulkMode != CV_ECB_NOPAD)
		{
			padLen = AES_BLOCK_LENGTH - (cleartextLength  % AES_BLOCK_LENGTH);
			padByte = (padLen == AES_BLOCK_LENGTH) ? AES_BLOCK_LENGTH : padLen;
			memset(pCleartext + cleartextLength, padByte, padLen);
		}
		cleartextLength += padLen;
        if ((retVal = cvBulkEncrypt(pCleartext, cleartextLength, pEncData, pEncLength, &hashOut[0], &hashOutLen, 
			encryptionOperation, &savedBulkParams, objType, key, HMACkey)) != CV_SUCCESS)
			goto err_exit;
		break;
	case CV_ASYM_ENCRYPT:
		if ((retVal = cvAsymEncrypt(pCleartext, cleartextLength, pEncData, pEncLength, objType, key)) != CV_SUCCESS)
			goto err_exit;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_ENCRYPTION_PARAMETER;
		goto err_exit;
	}

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	if (needObjWriteHMACKey)
		retVal = cvHandlerPostObjSave(&HMACobjProperties, HMACobjPtr, callback, context);

	/* if hash or HMAC needs to be saved, get and save object here. object must be available or this will fail */
	if (hashOutLen)
	{
		if (savedBulkParams.hHashObj == 0)
		{
			/* no handle available, fail */
			retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		/* already checked auth above, just get object */
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = savedBulkParams.hHashObj;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		hashObj = (cv_obj_value_hash *)HMACobjProperties.pObjValue;
		if (HMACobjPtr->objType != CV_TYPE_HASH || (hashOutLen == SHA1_LEN && hashObj->hashType != CV_SHA1)
			 || (hashOutLen == SHA256_LEN && hashObj->hashType != CV_SHA256))
		{
			retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		memcpy(hashObj->hash, hashOut, hashOutLen);
		if ((retVal = cvPutObj(&HMACobjProperties, HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
	}

err_exit:
	return retVal;
}


cv_status
cvBulkDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength, 
			  uint8_t *hashOut, uint32_t *hashOutLen, cv_enc_op decryptionOperation, cv_bulk_params *pBulkParameters, 
			  cv_obj_type objType, cv_obj_value_key *key, cv_obj_value_hash *HMACkey)
{
	/* this routine does a bulk decryption */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cipher_cmd;
	cv_status retVal = CV_SUCCESS;
	uint32_t authLen = 0, authKeyLen = 0, encAuthLength = encLength - pBulkParameters->authOffset;
	uint8_t bufOut[MAX_CV_OBJ_SIZE] ={0};
	uint8_t *hash;
	uint32_t modval = 0;
	uint32_t IV[AES_BLOCK_LENGTH/sizeof(uint32_t)];
	uint8_t *pIV = (uint8_t *)&IV[0];
//	uint32_t unalignedLength = encLength - pBulkParameters->encOffset;

	if (HMACkey == NULL)
		hash = NULL;
	else
		hash = HMACkey->hash;
	memset(&cipher_cmd,0,sizeof(cipher_cmd));
	cipher_cmd.cipher_mode  = PHX_CIPHER_MODE_DECRYPT;
	/* decrypted length is total length minus offset */
	encLength -= pBulkParameters->encOffset;
	/* make sure output length is sufficient */
	if (encLength > *pCleartextLength)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	if (objType == CV_TYPE_AES_KEY)
	{
		/* key should have been validated when created, so should have valid key length */
		if (key->keyLen == 128)
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_128;
		else if (key->keyLen == 192)
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_192;
		else if (key->keyLen == 256)
			cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_256;
	} else {
		/* invalid symmetric key type */
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	/* check alignment */
	modval = encLength % AES_BLOCK_LENGTH;
	if (modval)
	{
		retVal = CV_UNALIGNED_ENCRYPTION_DATA;
		goto err_exit;
	}
	switch (pBulkParameters->bulkMode)
	{
	case CV_CBC:
	case CV_CBC_NOPAD:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CBC;
		break;
	case CV_ECB:
	case CV_ECB_NOPAD:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_ECB;
		break;
	case CV_CTR:
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CTR;
		break;
	/* TBD: these aren't yet defined in SCAPI */
	case CV_CMAC:
	case CV_CCMP:
	case 0xffff:
	default:
		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	switch (pBulkParameters->authAlgorithm)
	{
	case CV_AUTH_ALG_NONE:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_NONE;
		encAuthLength = 0;
		break;
	case CV_AUTH_ALG_HMAC_SHA1:
		cipher_cmd.auth_alg    = BCM_SCAPI_AUTH_ALG_SHA1;
		cipher_cmd.auth_optype    = BCM_SCAPI_AUTH_OPTYPE_FULL;
		cipher_cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
		if (HMACkey == NULL || HMACkey->hashType != CV_SHA1)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		authLen = SHA1_LEN;
		authKeyLen = SHA1_LEN;
		break;
	case CV_AUTH_ALG_SHA1:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_SHA1;
		authLen = SHA1_LEN;
		break;
	case CV_AUTH_ALG_HMAC_SHA2_256:
		cipher_cmd.auth_alg    = BCM_SCAPI_AUTH_ALG_SHA256;
		cipher_cmd.auth_optype    = BCM_SCAPI_AUTH_OPTYPE_FULL;
		cipher_cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
		if (HMACkey == NULL || HMACkey->hashType != CV_SHA256)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		authLen = SHA256_LEN;
		authKeyLen = SHA256_LEN;
		break;
	case CV_AUTH_ALG_SHA2_256:
		cipher_cmd.auth_alg    = PHX_AUTH_ALG_SHA256;
		authLen = SHA256_LEN;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	/* now recheck size of output buffer, including auth, which SCAPI appends to end of encrypted data */
	if ((encLength + authLen) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_INPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	switch (decryptionOperation)
	{
	case CV_BULK_DECRYPT: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_NULL;
		break;
	case CV_BULK_DECRYPT_THEN_AUTH: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_CRYPT_AUTH;
		break;
	case CV_BULK_AUTH_THEN_DECRYPT: 
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_AUTH_CRYPT;
		break;
	case 0xffff:
	default:
 		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	/* also validate bulk parameters offsets */
	if (pBulkParameters->encOffset > encLength || pBulkParameters->authOffset > encLength)
	{
 		retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}

	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);
	/* do decryption op */
    if ((phx_bulk_cipher_init(cryptoHandle, &cipher_cmd, key->key, hash, authKeyLen, &ctx)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}

	/* copy IV locally to ensure 4-byte alignment */
	memcpy(&IV[0],pBulkParameters->IV,sizeof(IV));
	/* check to see if NULL IV */
	if (pBulkParameters->IVLen == 0)
		pIV = NULL;
	else if (pBulkParameters->IVLen != AES_BLOCK_LENGTH)
	{
		 retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
		goto err_exit;
	}
	if ((phx_bulk_cipher_start(cryptoHandle, pEncData, pIV, encLength, 
		pBulkParameters->encOffset, encAuthLength, 
		pBulkParameters->authOffset, bufOut, &ctx)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}
 	/* swaps endianness, both input and output */
    phx_bulk_cipher_swap_endianness(cryptoHandle, &ctx, TRUE, TRUE, TRUE);
    if ((phx_bulk_cipher_dma_start (cryptoHandle, &ctx, 0, NULL, 0)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}
	/* now copy to output */
	/* if there is auth data, it's after the cleartext data */
	if (authLen)
	{
		*hashOutLen = authLen;
		memcpy(hashOut, bufOut + encLength, authLen);
	}
	*pCleartextLength = encLength;
	memcpy(pCleartext, bufOut, encLength);

err_exit:
	return retVal;
}

cv_status
cvAsymDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength, 
			  cv_obj_type objType, cv_obj_value_key *key)
{
	/* this routine does an asymmetric decryption */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_status retVal = CV_SUCCESS;
	uint32_t bufOut[MAX_CV_OBJ_SIZE/sizeof(uint32_t)] ={0};
	uint8_t *p, *q, *edq, *edp, *pinv;
	uint32_t keyLenBytes, decryptLen = *pCleartextLength;

	/* check for correct object type */
	if (objType != CV_TYPE_RSA_KEY)
	{
		/* invalid key type */
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	/* get pointers to key params */
	keyLenBytes = key->keyLen/8;
	p = &key->key[keyLenBytes + sizeof(uint32_t)];
	q = &key->key[keyLenBytes + sizeof(uint32_t) + keyLenBytes/2];
	pinv = &key->key[2*keyLenBytes + sizeof(uint32_t)];
	edp = &key->key[2*keyLenBytes + sizeof(uint32_t) + keyLenBytes/2];
	edq = &key->key[3*keyLenBytes + sizeof(uint32_t)];
	/* convert from network byte order (big endian) to little endian */
	cvInplaceSwap((uint32_t *)pEncData, encLength/sizeof(uint32_t));
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);	
	if ((phx_rsa_mod_exp_crt(cryptoHandle, pEncData, encLength*8, 
		edq, 
		q, (keyLenBytes/2)*8,
		edp, 
		p, (keyLenBytes/2)*8,
		pinv,
		(uint8_t *)&bufOut[0], &decryptLen,
		NULL, NULL)) != PHX_STATUS_OK)
	{
		retVal = CV_ENCRYPTION_FAILURE;
		goto err_exit;
	}
	/* convert from little endian to network byte order (big endian) */
	cvInplaceSwap(bufOut, decryptLen/sizeof(uint32_t));
	/* now do PKCS 1 v1.5 decode */
	if ((retVal = cv_pkcs1_v15_decrypt_decode(decryptLen,  (uint8_t *)&bufOut[0], pCleartextLength, pCleartext)) != CV_SUCCESS)
		return retVal;

err_exit:
	return retVal;
}

cv_status
cv_decrypt (cv_handle cvHandle, cv_obj_handle decKeyHandle,           
			uint32_t authListLength, cv_auth_list *pAuthList,                                 
			cv_enc_op decryptionOperation,                              
			uint32_t encLength, byte *pEncData,                     
			cv_bulk_params *pBulkParameters,                            
			uint32_t *pCleartextLength, byte *pCleartext,            
			cv_callback *callback, cv_callback_ctx context) 
{
	/* This routine gets the key object and performs authorization then does decryption of the data provided, */
	/* based on the key type and other parameters provided. */
	cv_obj_properties objProperties, HMACobjProperties;
	cv_obj_hdr *objPtr, *HMACobjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE, needObjWriteHMACKey = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key;
	cv_obj_value_hash *HMACkey, *hashObj;
	cv_obj_type objType;
	uint8_t hashOut[SHA256_LEN];
	uint32_t hashOutLen = 0;
	cv_bulk_params savedBulkParams;
	cv_attrib_flags *flags;
	cv_bool isBulkOperation = FALSE;
	uint32_t padByte, i;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* make sure bulk parameters provided, if needed */
	if (decryptionOperation == CV_BULK_DECRYPT || decryptionOperation == CV_BULK_DECRYPT_THEN_AUTH 
			||  decryptionOperation == CV_BULK_AUTH_THEN_DECRYPT)
		isBulkOperation = TRUE;
	if (isBulkOperation && pBulkParameters == NULL)
		return CV_INVALID_INPUT_PARAMETER;
	if (encLength == 0)
		return CV_INVALID_INPUT_PARAMETER;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(decKeyHandle), &decKeyHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_enc_op), &decryptionOperation,
		sizeof(uint32_t), &encLength, encLength, pEncData, sizeof(cv_bulk_params), pBulkParameters,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = decKeyHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* make sure decryption key can be used for this purpose */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (!(*flags & CV_ATTRIB_USE_FOR_DECRYPTION))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* check to see if need to get HMAC key object */
	if (isBulkOperation && pBulkParameters->hAuthKey != 0)
	{
		/* this assumes that HMAC key object is available, otherwise will fail */
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = pBulkParameters->hAuthKey;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		if (HMACobjPtr->objType != CV_TYPE_HASH)
		{
			retVal = CV_INVALID_HMAC_KEY;
			goto err_exit;
		}
		/* now check that auth already done for decryption key authorizes HMAC key */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
						objProperties.authListsLength, objProperties.pAuthLists,
						HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
						0, &needObjWriteHMACKey)) != CV_SUCCESS)
			goto err_exit;
		HMACkey = (cv_obj_value_hash *)HMACobjProperties.pObjValue;
	} else
		HMACkey = NULL;

	/* check auth for output hash/HMAC if there is one.  this is done before decryption because auth list in */
	/* parameter list will get overwritten by decryption */
	if (isBulkOperation)
		memcpy(&savedBulkParams, pBulkParameters, sizeof(cv_bulk_params));
	if (isBulkOperation && pBulkParameters->hHashObj != 0)
	{
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = pBulkParameters->hHashObj;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		/* now check that auth already done for decryption key authorizes hash object write */
		if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
						objProperties.authListsLength, objProperties.pAuthLists,
						HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
						0, NULL)) != CV_SUCCESS)
			goto err_exit;
		/* need to make sure auth granted allows write as well */
		if (!(authFlags & CV_OBJ_AUTH_WRITE_PUB))
		{
			/* write auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	/* now do requested decyption */
	key = (cv_obj_value_key *)objProperties.pObjValue;
	objType = objPtr->objType;
	switch (decryptionOperation)
	{
	case CV_BULK_DECRYPT:
	case CV_BULK_DECRYPT_THEN_AUTH:
	case CV_BULK_AUTH_THEN_DECRYPT:
		if ((retVal = cvBulkDecrypt(pEncData, encLength, pCleartext, pCleartextLength, &hashOut[0], &hashOutLen, 
			decryptionOperation, pBulkParameters, objType, key, HMACkey)) != CV_SUCCESS)
			goto err_exit;
		/* see if need to check and remove padding */
		if (savedBulkParams.bulkMode != CV_CTR)
		{
			/* not counter mode, check PKCS5 padding */
			if (savedBulkParams.bulkMode != CV_CBC_NOPAD && savedBulkParams.bulkMode != CV_ECB_NOPAD)
			{
				padByte = pCleartext[*pCleartextLength - 1];
				if (padByte == 0 || padByte > AES_BLOCK_LENGTH)
				{
					retVal = CV_ENCRYPTION_FAILURE;
					goto err_exit;
				}
				for (i=1;i<=padByte;i++)
				{
					if (pCleartext[*pCleartextLength - i] != padByte)
					{
						retVal = CV_ENCRYPTION_FAILURE;
						goto err_exit;
					}
				}
				*pCleartextLength -= padByte;
			}
		}
		break;
	case CV_ASYM_DECRYPT:
		if ((retVal = cvAsymDecrypt(pEncData, encLength, pCleartext, pCleartextLength, objType, key)) != CV_SUCCESS)
			goto err_exit;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_ENCRYPTION_PARAMETER;
		goto err_exit;
	}

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	if (needObjWriteHMACKey)
		retVal = cvHandlerPostObjSave(&HMACobjProperties, HMACobjPtr, callback, context);

	/* if hash or HMAC needs to be saved, get and save object here. object must be available or this will fail */
	if (hashOutLen)
	{
		if (savedBulkParams.hHashObj == 0)
		{
			/* no handle available, fail */
			retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		/* already checked auth above, just get object */
		memcpy(&HMACobjProperties, &objProperties, sizeof(cv_obj_properties));
		HMACobjProperties.objHandle = savedBulkParams.hHashObj;
		if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
		hashObj = (cv_obj_value_hash *)HMACobjProperties.pObjValue;
		if (HMACobjPtr->objType != CV_TYPE_HASH || (hashOutLen == SHA1_LEN && hashObj->hashType != CV_SHA1)
			 || (hashOutLen == SHA256_LEN && hashObj->hashType != CV_SHA256))
		{
			retVal = CV_INVALID_BULK_ENCRYPTION_PARAMETER;
			goto err_exit;
		}
		memcpy(hashObj->hash, hashOut, hashOutLen);
		if ((retVal = cvPutObj(&HMACobjProperties, HMACobjPtr)) != CV_SUCCESS)
			goto err_exit;
	}

err_exit:
	return retVal;
}

cv_status
cv_hash (cv_handle cvHandle, cv_hash_op hashOperation,                                   
		 uint32_t dataLength, byte *pData, cv_obj_handle hashHandle)
{
	/* this routine computes a hash of the data provided and stores the result in the object provided */
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_value_hash *hashObj;
	uint32_t hashLen = 0;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* don't allow zero length hashes */
	if (dataLength == 0)
		return CV_INVALID_INPUT_PARAMETER;

	/* staged hashes not implemented for now */
	if (hashOperation != CV_SHA)
		return CV_FEATURE_NOT_AVAILABLE;

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = hashHandle;
	objProperties.session = (cv_session *)cvHandle;
	/* first get hash object and determine hash type */
	if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
		goto err_exit;
	/* validate object type */
	if (objPtr->objType != CV_TYPE_HASH)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}
	/* make sure can do unauthorized write to hash object */
	if (cvHasIndicatedAuth(objProperties.pAuthLists, objProperties.authListsLength, CV_OBJ_AUTH_WRITE_PUB)
		|| cvHasIndicatedAuth(objProperties.pAuthLists, objProperties.authListsLength, CV_OBJ_AUTH_WRITE_PRIV))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	hashObj = (cv_obj_value_hash *)objProperties.pObjValue;
	if (hashObj->hashType == CV_SHA1)
		hashLen = SHA1_LEN;
	else if (hashObj->hashType == CV_SHA256)
		hashLen = SHA256_LEN;
	else {
		retVal = CV_INVALID_HASH_TYPE;
		goto err_exit;
	}

	if ((retVal = cvAuth(pData, dataLength, NULL, hashObj->hash, hashLen, NULL, 0, hashOperation/*TBD*/)) != CV_SUCCESS)
			goto err_exit;

	/* now save hash object */
	retVal = cvPutObj(&objProperties, objPtr);

err_exit:
	return retVal;
}

cv_status
cv_hmac (cv_handle cvHandle, cv_obj_handle hmacKeyHandle,            
			uint32_t authListLength, cv_auth_list *pAuthList,                                
			cv_hmac_op hmacOperation,                                 
			uint32_t dataLength, byte *pData, cv_obj_handle hmacHandle,
			cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the HMAC key object and performs HMAC then saves result in object handle provided, */
	/* based on the key type and other parameters provided. */
	cv_obj_properties objProperties, HMACobjProperties;
	cv_obj_hdr *objPtr, *HMACobjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_hash *HMACkey, *hashObj;
	uint32_t hashLen = 0;
	uint32_t hmacKey[SHA256_LEN/sizeof(uint32_t)];

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* don't allow zero length hashes */
	if (dataLength == 0)
		return CV_INVALID_INPUT_PARAMETER;

	/* staged hashes not implemented for now */
	if (hmacOperation != CV_HMAC_SHA)
		return CV_FEATURE_NOT_AVAILABLE;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(hmacKeyHandle), &hmacKeyHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_hmac_op), &hmacOperation,
		sizeof(uint32_t), &dataLength, dataLength, pData, sizeof(hmacHandle), &hmacHandle,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = hmacKeyHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	HMACkey = (cv_obj_value_hash *)objProperties.pObjValue;

	/* now get object to save HMAC in */
	memset(&HMACobjProperties,0,sizeof(cv_obj_properties));
	HMACobjProperties.objHandle = hmacHandle;
	HMACobjProperties.session = (cv_session *)cvHandle;
	/* first get hash object and determine hash type */
	if ((retVal = cvGetObj(&HMACobjProperties, &HMACobjPtr)) != CV_SUCCESS)
		goto err_exit;
	/* now check that auth already done for decryption key authorizes hash object write */
	if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
					objProperties.authListsLength, objProperties.pAuthLists,
					HMACobjProperties.authListsLength, HMACobjProperties.pAuthLists,
					0, NULL)) != CV_SUCCESS)
		goto err_exit;
	/* need to make sure auth granted allows write as well */
	if (!(authFlags & CV_OBJ_AUTH_WRITE_PUB))
	{
		/* write auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* validate object type and that HMAC key matches HMAC */
	hashObj = (cv_obj_value_hash *)HMACobjProperties.pObjValue;
	if (HMACobjPtr->objType != CV_TYPE_HASH || (HMACkey->hashType != hashObj->hashType))
	{
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	if (hashObj->hashType == CV_SHA1)
		hashLen = SHA1_LEN;
	else if (hashObj->hashType == CV_SHA256)
		hashLen = SHA256_LEN;
	else {
		retVal = CV_INVALID_HASH_TYPE;
		goto err_exit;
	}

	/* now create HMAC */
	/* store hmac key locally to ensure alignment */
	memcpy(&hmacKey[0],HMACkey->hash,hashLen);
	if ((retVal = cvAuth(pData, dataLength, (uint8_t *)&hmacKey[0], hashObj->hash, hashLen, NULL, 0, hmacOperation/*TBD*/)) != CV_SUCCESS)
			goto err_exit;

	/* now save HMAC object */
	if ((retVal = cvPutObj(&HMACobjProperties, HMACobjPtr)) != CV_SUCCESS)
		goto err_exit;

	/* now check to see if auth requires HMAC key object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cvSign(cv_obj_type objType, uint8_t *hash, uint32_t hashLen, cv_obj_value_key *key, 
			  cv_sign_op signOperation, uint8_t *signature)
{
	/* this routine does a signature operation */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_status retVal = CV_SUCCESS;
	uint32_t bufIn[MAX_CV_OBJ_SIZE/sizeof(uint32_t)];
	uint8_t *p, *q, *edq, *edp, *pinv, *g, *priv;
	uint8_t *a, *b, *G, *n, *h;
	uint32_t keyLenBytes, signatureLength, privKeyLenBytes, keyLenBits;
	fips_rng_context  rngctx;

	/* copy hash into local buffer */
	memcpy(&bufIn[0], hash, hashLen);
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);

	/* now determine what type of signature to do */
	switch (objType)
	{
	case CV_TYPE_RSA_KEY:
		/* get pointers to key params */
		keyLenBytes = key->keyLen/8;
		/* encode hash for encryption */
		if ((cvPkcs1V15Encode(cryptoHandle, hashLen, hash, keyLenBytes, (uint8_t *)&bufIn[0]) != PHX_STATUS_OK))
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		p = &key->key[keyLenBytes + sizeof(uint32_t)];
		q = &key->key[keyLenBytes + sizeof(uint32_t) + keyLenBytes/2];
		pinv = &key->key[2*keyLenBytes + sizeof(uint32_t)];
		edp = &key->key[2*keyLenBytes + sizeof(uint32_t) + keyLenBytes/2];
		edq = &key->key[3*keyLenBytes + sizeof(uint32_t)];
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap(&bufIn[0], keyLenBytes/sizeof(uint32_t));
		if ((phx_rsa_mod_exp_crt(cryptoHandle, (uint8_t *)&bufIn[0], key->keyLen, 
			edq, 
			q, (keyLenBytes/2)*8,
			edp, 
			p, (keyLenBytes/2)*8,
			pinv,
			signature, &signatureLength,
			NULL, NULL)) != PHX_STATUS_OK)
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		/* convert from little endian to network byte order (big endian) */
		cvInplaceSwap((uint32_t *)&signature[0], signatureLength/sizeof(uint32_t));
		break;
	case CV_TYPE_DSA_KEY:
		/* determine whether signature operation indicates use Kdi in 6T or in key provided */
		if (signOperation == CV_SIGN_USE_KDI)
		{
			/* use Kdi and default domain params */
			p = (uint8_t *)dsa_default_p;
			q = (uint8_t *)dsa_default_q;
			g = (uint8_t *)dsa_default_g;
			priv = NV_6TOTP_DATA->Kdi_priv;
			keyLenBits = DSA1024_KEY_LEN_BITS;
		} else {
			/* use values from key */
			keyLenBits = key->keyLen;
			keyLenBytes = keyLenBits/8;
			privKeyLenBytes = (key->keyLen == DSA1024_KEY_LEN_BITS) ? DSA1024_PRIV_LEN : DSA2048_PRIV_LEN;
			p = &key->key[0];
			q = &key->key[keyLenBytes];
			g = &key->key[keyLenBytes + privKeyLenBytes];
			priv = &key->key[3*keyLenBytes + privKeyLenBytes];;
		}
		if ((phx_rng_fips_init(cryptoHandle, &rngctx, TRUE)) != PHX_STATUS_OK) /* rng selftest included */
		{
			retVal = CV_RNG_FAILURE;
			goto err_exit;
		}
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap((uint32_t *)&bufIn[0], hashLen/sizeof(uint32_t));
		if ((phx_dsa_sign(cryptoHandle, (uint8_t *)&bufIn[0], NULL,
			p, keyLenBits, q, g, priv,
			signature, &signatureLength, NULL, NULL)) != PHX_STATUS_OK)
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		/* convert from little endian to network byte order (big endian) */
		cvInplaceSwap((uint32_t *)&signature[0], CV_NONCE_LEN/sizeof(uint32_t));
		cvInplaceSwap((uint32_t *)(&signature[0] + CV_NONCE_LEN), CV_NONCE_LEN/sizeof(uint32_t));
		break;
	case CV_TYPE_ECC_KEY:
		/* determine whether signature operation indicates use Kdi in 6T or in key provided */
		if (signOperation == CV_SIGN_USE_KDIECC)
		{
			/* use Kdi and default domain params */
			p = (uint8_t *)ecc_default_prime;
			a = (uint8_t *)ecc_default_a;
			b = (uint8_t *)ecc_default_b;
			G = (uint8_t *)ecc_default_G;
			n = (uint8_t *)ecc_default_n;
			h = (uint8_t *)ecc_default_h;
			priv = NV_6TOTP_DATA->KecDSA;
			keyLenBits = ECC256_KEY_LEN;
			keyLenBytes = keyLenBits/8;
		} else {
			/* use values from key */
			keyLenBits = key->keyLen;
			keyLenBytes = keyLenBits/8;
			p = &key->key[0];
			a = &key->key[keyLenBytes];
			b = &key->key[2*keyLenBytes];
			G = &key->key[5*keyLenBytes];
			n = &key->key[3*keyLenBytes];
			h = &key->key[4*keyLenBytes];
			priv = &key->key[9*keyLenBytes];
		}
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap((uint32_t *)&bufIn[0], hashLen/sizeof(uint32_t));
		if ((phx_ecp_ecdsa_sign(cryptoHandle, (uint8_t *)&bufIn[0], PRIME_FIELD, p,  
			keyLenBits, a, b, n, 
			(uint8_t *)G,(uint8_t *)G + keyLenBytes,
			h, 0, priv, signature, signature + keyLenBytes,
			NULL, NULL)) != PHX_STATUS_OK)
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}

		/* convert from little endian to network byte order (big endian) */
		cvInplaceSwap((uint32_t *)&signature[0], keyLenBytes/sizeof(uint32_t));
		cvInplaceSwap((uint32_t *)(&signature[0] + keyLenBytes), keyLenBytes/sizeof(uint32_t));
		break;
	case 0xffff:
		break;
	}

err_exit:
	return retVal;
}

cv_status
cv_sign (cv_handle cvHandle, cv_obj_handle signKeyHandle,             
			uint32_t authListLength, cv_auth_list *pAuthList,                                
			cv_sign_op signOperation,                                   
			cv_obj_handle hashHandle, 
			uint32_t *pSignatureLength, byte *pSignature,                     
			cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the key object and performs the authorization. It then does a digital */
	/* signature of the data provided, based on the key type and other parameters provided. */
	cv_obj_properties objProperties, hashObjProperties;
	cv_obj_hdr *objPtr, *hashObjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE, needObjWriteHash = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key = NULL;
	cv_obj_value_hash *hashObj;
	cv_obj_type objType;
	uint32_t hashLen, expectedSigLen;
	cv_attrib_flags *flags;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 5, sizeof(signKeyHandle), &signKeyHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(signOperation), &signOperation,
		sizeof(hashHandle), &hashHandle, 0, NULL, 0, NULL,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = signKeyHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* check to see if request is to use Kdi (PHX2 signing key in 6T).  if so, need admin auth */
	if (signOperation == CV_SIGN_NORMAL) {
		/* normal signing operation, get key object and authorize */
		if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
			&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
			goto err_exit;
		key = (cv_obj_value_key *)objProperties.pObjValue;
		objType = objPtr->objType;
		switch (objType)
		{
		case CV_TYPE_ECC_KEY:
			expectedSigLen = 2*(key->keyLen/8);
			break;
		case CV_TYPE_DSA_KEY:
			expectedSigLen = DSA_SIG_LEN;
			break;
		case CV_TYPE_RSA_KEY:
			expectedSigLen = key->keyLen/8;
			/* Removed the below dead code as per Coverity */
			
			/* check for selection of Kdi.  not valid for RSA type key */
			/*
			if (signOperation == CV_SIGN_USE_KDI)
			{
				retVal = CV_INVALID_OBJECT_TYPE;
				goto err_exit;
			}
			*/
			break;
		case 0xffff:
		default:
			retVal = CV_INVALID_OBJECT_TYPE;
			goto err_exit;
		}
	} else if (FALSE/*signOperation == CV_SIGN_USE_KDI || signOperation == CV_SIGN_USE_KDIECC*/) {
		/* signature using Kdi, just verify CV admin auth (no key object required) */
		if ((retVal = cvDoAuth(CV_REQUIRE_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &authFlags, &inputParameters, 
			callback, context, FALSE,
			NULL, NULL)) != CV_SUCCESS)
			goto err_exit;
		if (signOperation == CV_SIGN_USE_KDI) 
		{
			objType = CV_TYPE_DSA_KEY;
			expectedSigLen = DSA_SIG_LEN;
		} else {
			objType = CV_TYPE_ECC_KEY;
			expectedSigLen = ECC256_SIG_LEN;
		}
	} else {
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* make sure signature key can be used for this purpose */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (!(*flags & CV_ATTRIB_USE_FOR_SIGNING))
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	}

	/* now get hash object */
	memcpy(&hashObjProperties, &objProperties, sizeof(cv_obj_properties));
	hashObjProperties.objHandle = hashHandle;
	if ((retVal = cvGetObj(&hashObjProperties, &hashObjPtr)) != CV_SUCCESS)
		goto err_exit;
	/* now check that auth already done for decryption key authorizes hash object write */
	if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
					objProperties.authListsLength, objProperties.pAuthLists,
					hashObjProperties.authListsLength, hashObjProperties.pAuthLists,
					0, &needObjWriteHash)) != CV_SUCCESS)
		goto err_exit;
	if (hashObjPtr->objType != CV_TYPE_HASH)
	{
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	hashObj = (cv_obj_value_hash *)hashObjProperties.pObjValue;
	hashLen = (hashObj->hashType == CV_SHA1) ? SHA1_LEN : SHA256_LEN;

	/* validate length of signature output */
	if (expectedSigLen > *pSignatureLength)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	*pSignatureLength = expectedSigLen;
	/* now do requested signature */
	if ((retVal = cvSign(objType, hashObj->hash, hashLen, key, signOperation, pSignature)) != CV_SUCCESS)
		goto err_exit;

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	if (needObjWriteHash)
		retVal = cvHandlerPostObjSave(&hashObjProperties, hashObjPtr, callback, context);
	
	return retVal;
err_exit:
	cvFlushCacheEntry(signKeyHandle);
	return retVal;
}

cv_status
cvVerify(cv_obj_type objType, uint8_t *hash, uint32_t hashLen, cv_obj_value_key *key, 
			  uint8_t *signature, uint32_t signatureLen, cv_bool *verify)
{
	/* this routine does a signature verification operation */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	cv_status retVal = CV_SUCCESS;
	uint32_t bufIn[MAX_CV_OBJ_SIZE/sizeof(uint32_t)], bufOut[MAX_CV_OBJ_SIZE/sizeof(uint32_t)] ={0};
	uint8_t *m, *e, *g, *pubKey, *p, *q, *y, *r, *s;
	uint8_t *a, *b, *G, *n;
	uint32_t keyLenBytes, keyLenBits, privKeyLenBytes;
	uint32_t cleartextLen;
	uint8_t *compareValue = hash;
	uint32_t rbuf[ECC512_SIZE_R/sizeof(uint32_t)];    /* this is sized for ecdsa 512-bit key which is 64 bytes */
	uint8_t hashSwapped[SHA256_LEN];
	uint32_t compareLen = hashLen;

	/* copy signature into local buffer */
	memcpy(&bufIn[0], signature, signatureLen);
	/* init SCAPI crypto lib */
	phx_crypto_init(cryptoHandle);

	/* now determine what type of signature verification to do */
	switch (objType)
	{
	case CV_TYPE_RSA_KEY:
		/* get pointers to key params */
		keyLenBytes = key->keyLen/8;
		/* encode hash for verify */
		if ((cvPkcs1V15Encode(cryptoHandle, hashLen, hash, keyLenBytes, (uint8_t *)&bufOut[0]) != PHX_STATUS_OK))
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		m = &key->key[0];
		e = &key->key[keyLenBytes];
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap(&bufIn[0], signatureLen/sizeof(uint32_t));
		if ((phx_rsa_mod_exp(cryptoHandle, (uint8_t *)&bufIn[0], signatureLen*8, 
			m, keyLenBytes*8,
			e, sizeof(uint32_t)*8,
			(uint8_t *)&bufIn[0], &cleartextLen,
			NULL, NULL)) != PHX_STATUS_OK)
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		/* convert from little endian to network byte order (big endian) */
		cvInplaceSwap(bufIn, cleartextLen/sizeof(uint32_t));
		compareValue = (uint8_t *)&bufOut[0];
		compareLen = keyLenBytes;
		break;
	case CV_TYPE_DSA_KEY:
		/* use values from key */
		keyLenBits = key->keyLen;
		keyLenBytes = keyLenBits/8;
		privKeyLenBytes = (key->keyLen == DSA1024_KEY_LEN_BITS) ? DSA1024_PRIV_LEN : DSA2048_PRIV_LEN;
		p = &key->key[0];
		q = &key->key[keyLenBytes];
		g = &key->key[keyLenBytes + privKeyLenBytes];
		y = &key->key[2*keyLenBytes + privKeyLenBytes];
		r = (uint8_t *)&bufIn[0];
		s = (uint8_t *)&bufIn[0] + CV_NONCE_LEN;
		/* save r for comparison */
		memcpy(rbuf, r, CV_NONCE_LEN);
		compareValue = (uint8_t *)&rbuf[0];
		compareLen = CV_NONCE_LEN;
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap((uint32_t *)r, CV_NONCE_LEN/sizeof(uint32_t));
		cvInplaceSwap((uint32_t *)s, CV_NONCE_LEN/sizeof(uint32_t));
		cvCopySwap((uint32_t *)hash,(uint32_t *)hashSwapped,hashLen);
		if (phx_dsa_verify (
                    cryptoHandle,
                    hashSwapped,
                    p,    keyLenBits,
                    q,
                    g,
                    y,
                    r,    s,
                    (uint8_t *)&bufIn[0],    &cleartextLen,
                    NULL, NULL))
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		/* convert from little endian to network byte order (big endian) */
		cvInplaceSwap(bufIn, cleartextLen/sizeof(uint32_t));
		break;
	case CV_TYPE_ECC_KEY:
		/* use values from key */
		keyLenBits = key->keyLen;
		keyLenBytes = keyLenBits/8;
		p = &key->key[0];
		a = &key->key[keyLenBytes];
		b = &key->key[2*keyLenBytes];
		G = &key->key[5*keyLenBytes];
		n = &key->key[3*keyLenBytes];
		pubKey = &key->key[7*keyLenBytes];
		r = (uint8_t *)&bufIn[0];
		s = (uint8_t *)&bufIn[0] + keyLenBytes;
		compareValue = (uint8_t *)&bufOut[0];
		compareLen = keyLenBytes;
		/* convert from network byte order (big endian) to little endian */
		cvInplaceSwap((uint32_t *)r, keyLenBytes/sizeof(uint32_t));
		cvInplaceSwap((uint32_t *)s, keyLenBytes/sizeof(uint32_t));
		cvCopySwap((uint32_t *)hash,(uint32_t *)hashSwapped,hashLen);
		if ((phx_ecp_ecdsa_verify(cryptoHandle, hashSwapped, PRIME_FIELD, p,  
			keyLenBits, a, b, n, 
			(uint8_t *)G,(uint8_t *)G + keyLenBytes,
			pubKey, pubKey + keyLenBytes, NULL, 
			r, s, (uint8_t *)&bufOut[0],
			NULL, NULL)) != PHX_STATUS_OK)			
		{
			retVal = CV_SIGNATURE_FAILURE;
			goto err_exit;
		}
		break;
	case 0xffff:
		break;
	}

	/* now compare and determine verify status */
	if (memcmp(bufIn, compareValue, compareLen))
		*verify = FALSE;
	else
		*verify = TRUE;

err_exit:
	return retVal;
}

cv_status
cv_verify (cv_handle cvHandle, cv_obj_handle verifyKeyHandle,             
			uint32_t authListLength, cv_auth_list *pAuthList,                                
			cv_verify_op verifyOperation,                                   
			cv_obj_handle hashHandle, 
			uint32_t signatureLength, byte *pSignature, cv_bool *pVerify,                     
			cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the key object and performs the authorization. It then does a digital */
	/* signature of the data provided, based on the key type and other parameters provided. */
	cv_obj_properties objProperties, hashObjProperties;
	cv_obj_hdr *objPtr, *hashObjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE, needObjWriteHash = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_key *key = NULL;
	cv_obj_value_hash *hashObj;
	cv_obj_type objType;
	uint32_t hashLen, expectedSigLen;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(verifyKeyHandle), &verifyKeyHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(verifyOperation), &verifyOperation,
		sizeof(hashHandle), &hashHandle, sizeof(signatureLength), &signatureLength, signatureLength, pSignature,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = verifyKeyHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* get key object and authorize */
	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;
	key = (cv_obj_value_key *)objProperties.pObjValue;
	objType = objPtr->objType;
	switch (objType)
	{
	case CV_TYPE_ECC_KEY:
		expectedSigLen = 2*(key->keyLen/8);
		break;
	case CV_TYPE_DSA_KEY:
		expectedSigLen = DSA_SIG_LEN;
		break;
	case CV_TYPE_RSA_KEY:
		expectedSigLen = key->keyLen/8;
		break;
	case 0xffff:
	default:
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}

	/* auth succeeded, but need to make sure access auth granted */
	if (!(authFlags & CV_OBJ_AUTH_ACCESS))
	{
		/* access auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	/* now get hash object */
	memcpy(&hashObjProperties, &objProperties, sizeof(cv_obj_properties));
	hashObjProperties.objHandle = hashHandle;
	if ((retVal = cvGetObj(&hashObjProperties, &hashObjPtr)) != CV_SUCCESS)
		goto err_exit;
	/* now check that auth already done for decryption key authorizes hash object write */
	if ((retVal = cvValidateObjWithExistingAuth(authListLength, pAuthList, 
					objProperties.authListsLength, objProperties.pAuthLists,
					hashObjProperties.authListsLength, hashObjProperties.pAuthLists,
					0, &needObjWriteHash)) != CV_SUCCESS)
		goto err_exit;
	if (hashObjPtr->objType != CV_TYPE_HASH)
	{
		retVal = CV_INVALID_OBJECT_TYPE;
		goto err_exit;
	}
	hashObj = (cv_obj_value_hash *)hashObjProperties.pObjValue;
	hashLen = (hashObj->hashType == CV_SHA1) ? SHA1_LEN : SHA256_LEN;

	/* validate length of signature output */
	if (expectedSigLen != signatureLength)
	{
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}
	/* now do requested signature verification */
	if ((retVal = cvVerify(objType, hashObj->hash, hashLen, key, pSignature, signatureLength, pVerify)) != CV_SUCCESS)
		goto err_exit;

	/* now check to see if auth requires object to be saved */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);
	if (needObjWriteHash)
		retVal = cvHandlerPostObjSave(&hashObjProperties, hashObjPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cv_hash_object (cv_handle cvHandle, cv_hash_op hashOperation,                                   
				cv_obj_handle objectHandle, cv_obj_handle hashHandle)
{
	/* This routine is similar to cv_hash, but the data to be hashed in the value field of the associated */
	/* object.   If the attributes of this object indicate that it is non-hashable, an error is returned */
	cv_obj_properties objProperties, hashObjProperties;
	cv_obj_hdr *objPtr, *hashObjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_hash *hashObj;
	uint32_t hashLen = 0;
	cv_attrib_flags *flags;
	uint8_t obj[MAX_CV_OBJ_SIZE];

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* staged hashes not implemented for now */
	if (hashOperation != CV_SHA)
		return CV_FEATURE_NOT_AVAILABLE;

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = objectHandle;
	objProperties.session = (cv_session *)cvHandle;

	/* just get object w/o auth (this won't work for objects in removable devices) */
	if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
		goto err_exit;

	/* check to make sure object is hashable.  find attributes flags */
	if ((retVal = cvFindObjAtribFlags(&objProperties, objPtr, &flags, NULL)) != CV_SUCCESS)
		goto err_exit;
	if (*flags & CV_ATTRIB_NON_HASHABLE)
	{
		retVal = CV_OBJECT_NONHASHABLE;
		goto err_exit;
	}

	/* now get object to save hash in */
	memset(&hashObjProperties,0,sizeof(cv_obj_properties));
	hashObjProperties.objHandle = hashHandle;
	hashObjProperties.session = (cv_session *)cvHandle;
	/* first get hash object and determine hash type */
	if ((retVal = cvGetObj(&hashObjProperties, &hashObjPtr)) != CV_SUCCESS)
		goto err_exit;
	/* validate object type */
	hashObj = (cv_obj_value_hash *)hashObjProperties.pObjValue;
	if (hashObjPtr->objType != CV_TYPE_HASH)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}
	/* make sure can do unauthorized write to hash object */
	if (cvHasIndicatedAuth(hashObjProperties.pAuthLists, hashObjProperties.authListsLength, CV_OBJ_AUTH_WRITE_PUB)
		|| cvHasIndicatedAuth(hashObjProperties.pAuthLists, hashObjProperties.authListsLength, CV_OBJ_AUTH_WRITE_PRIV))
	{
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}
	if (hashObj->hashType == CV_SHA1)
		hashLen = SHA1_LEN;
	else if (hashObj->hashType == CV_SHA256)
		hashLen = SHA256_LEN;
	else {
		retVal = CV_INVALID_HASH_TYPE;
		goto err_exit;
	}

	/* copy value locally in order to change from internal to external format */
	cvCopyObjValue(objPtr->objType, objProperties.objValueLength, (void *)obj, objProperties.pObjValue, TRUE, FALSE);

	/* now create hash */
	if ((retVal = cvAuth(obj, objProperties.objValueLength, NULL, hashObj->hash, hashLen, NULL, 0, CV_SHA/*TBD*/)) != CV_SUCCESS)
			goto err_exit;

	/* now save hash object */
	if ((retVal = cvPutObj(&hashObjProperties, hashObjPtr)) != CV_SUCCESS)
		goto err_exit;

err_exit:
	return retVal;
}

extern cv_status cvCalculateHOTPValue(uint8_t* randomKey, uint32_t randomKeyLength, uint8_t numberOfDigits, uint32_t counterValue, uint64_t* hotpValue);

cv_bool validateHOTPAuthForUnblock(cv_obj_value_hotp *hotpValueObj, cv_hotp_unblock_auth* unblockAuth, cv_bool* needObjWrite)
{
	cv_status retVal = CV_AUTH_FAIL;
	uint32_t counterValue = hotpValueObj->currentCounterValue;
	cv_bool bFirstOTPValueIsCorrect=0;
	uint64_t calculatedHotpValue = 0;

	uint32_t delta  = 0;
	while (delta < hotpValueObj->allowedLargeMaxWindow)
	{
		if ((retVal = cvCalculateHOTPValue(hotpValueObj->randomKey, hotpValueObj->randomKeyLength, hotpValueObj->numberOfDigits, counterValue, &calculatedHotpValue)) != CV_SUCCESS)
		{
			printf("cvCalculateHOTPValue failed with %d", retVal);
			return FALSE;
		}

		++delta;
		counterValue += hotpValueObj->counterInterval;

		if (bFirstOTPValueIsCorrect)
		{
			if (unblockAuth->OTPValue2 == calculatedHotpValue)
			{
				printf("validateHOTPAuthForUnblock Two correct OTP values\n");
				hotpValueObj->currentCounterValue = counterValue; // sync counter value
				*needObjWrite = TRUE;
				return TRUE;
			}
			else
			{
				printf("validateHOTPAuthForUnblock Second OTP value is incorrect\n");
				bFirstOTPValueIsCorrect = FALSE;
			}
		}

		if (unblockAuth->OTPValue1 == calculatedHotpValue)
		{
			printf("validateHOTPAuthForUnblock One correct OTP value\n");
			bFirstOTPValueIsCorrect = TRUE;
		}
	}

	printf("validateHOTPAuthForUnblock Incorrect two OTP values. counterValue:%d", counterValue);
	return FALSE;
}


cv_status
cv_hotp_unblock (cv_handle cvHandle, cv_obj_handle hotpObjectHandle,
				 cv_hotp_unblock_auth* unblockAuth,
				 byte* bIsBlockedNow)
{

	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_hotp *hotpValueObj = NULL;
	cv_bool needObjWrite = FALSE;

	*bIsBlockedNow = 0; // First set output param to 0. Below it can be changed to 1 only if needed.

	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
	{
		printf("cv_hotp_unblock Not a valid session handle\n");
		goto err_exit;
	}

	if (unblockAuth == NULL)
	{
		printf("cv_hotp_unblock unblockAuth is NULL\n");
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}
	
	// Get the object

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = hotpObjectHandle;
	objProperties.session = (cv_session *)cvHandle;

	if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
	{
		printf("cv_hotp_unblock cvGetObj error: %d", retVal);
		goto err_exit;
	}

	if (objPtr == NULL)
	{
		printf("cv_hotp_unblock objPtr error\n");
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	if (objPtr->objType != CV_TYPE_HOTP)
	{
		printf("cv_hotp_unblock Incorrect object type (not HOTP): %d\n", objPtr->objType);
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	hotpValueObj = (cv_obj_value_hotp *)cvFindObjValue(objPtr);
	if (hotpValueObj == NULL)
	{
		printf("cv_hotp_unblock cvFindObjValue error\n");
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}
	
	printf("hotpValueObj->isBlocked: %d\n", hotpValueObj->isBlocked);

	// Try to unblock only if blocked and got valid OTP values
	if (hotpValueObj->isBlocked && !(unblockAuth->OTPValue1 == 0 && unblockAuth->OTPValue2 == 0) )
	{
		if (!validateHOTPAuthForUnblock(hotpValueObj, unblockAuth, &needObjWrite))
		{
			printf("cv_hotp_unblock auth failure\n");
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}

		printf("cv_hotp_unblock unblocking\n");
		hotpValueObj->isBlocked = 0;
		hotpValueObj->currentNumberOfTries = 0;
		needObjWrite = TRUE;
	}

	// Now check if the object is still blocked and set output param accordingly
	if (hotpValueObj->isBlocked)
	{
		*bIsBlockedNow = 1;
	}
	printf("cv_hotp_unblock bIsBlockedNow:%d\n", *bIsBlockedNow);

	/* save original object (if necessary) */
	if (needObjWrite)
	{
		if ((retVal = cvPutObj(&objProperties, objPtr)) != CV_SUCCESS)
		{
			printf("cvPutObj error: %d", retVal);
			goto err_exit;
		}
	}
	
err_exit:
	return retVal;
}

cv_status
cv_otp_get_value (cv_handle cvHandle, cv_obj_handle otpTokenHandle,             
					uint32_t authListLength, cv_auth_list *pAuthList,                                   
					cv_time *pTime, uint32_t signatureLength, byte *pSignature,
					cv_obj_handle verifyKeyHandle,
					uint32_t *pOtpValueLength, byte *pOtpValue,
					cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the OTP object and performs the authorization.  It then computes a one-time-password value, */
	/* based on the parameters in the OTP object and the input parameters. */
	cv_obj_properties objProperties, keyObjProperties;
	cv_obj_hdr *objPtr, *keyObjPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_rsa_token *rsaToken;
	cv_obj_value_oath_token *oathToken;
	cv_time time;
	cv_obj_value_key *key = NULL;
	uint32_t expectedSigLen;
	uint8_t hash[SHA256_LEN];
	uint32_t hashLen;
	cv_bool verify;
	uint32_t otpVal;
	uint8_t otpValOath[SHA1_LEN];

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(otpTokenHandle), &otpTokenHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_time), pTime,
		sizeof(uint32_t), &signatureLength, signatureLength, pSignature, sizeof(verifyKeyHandle), &verifyKeyHandle,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = otpTokenHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* get token object */
	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* verify object type */
	if (objPtr->objType != CV_TYPE_OTP_TOKEN)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	/* determine type of token */
	rsaToken = (cv_obj_value_rsa_token *)objProperties.pObjValue;
	oathToken = (cv_obj_value_oath_token *)objProperties.pObjValue;
	if (rsaToken->type == CV_OTP_TYPE_RSA)
	{
		/* RSA token */
		/* validate output token length */
		if (*pOtpValueLength < sizeof(uint32_t ))
		{
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pOtpValueLength = sizeof(uint32_t);
		/* determine how to get time. Note that the time can be provided a number of ways.  If the RTC has been */
		/* configured with the time, that time is used (GMT time must be used, so both GMT and local time are */
		/* provided when RTC is configured).  If the RTC hasn’t been configured, the time provided as a parameter */
		/* is used.  If the CV policy set in the persistent data indicates signed time must be used, the signature */
		/* parameter must be used to verify the time using the public key pointed to by the key handle provided. */
		if (CV_PERSISTENT_DATA->persistentFlags & CV_RTC_TIME_CONFIGURED)
		{
			/* use RTC */
			if ((retVal = get_time(&time)) != CV_SUCCESS)
				goto err_exit;
		} else {
			/* determine if need to check signature of time */
			if (CV_PERSISTENT_DATA->cvPolicy & CV_REQUIRE_OTP_SIGNED_TIME)
			{
				/* verify signature with verify key object */
				if (signatureLength == 0 || verifyKeyHandle == 0)
				{
					retVal = CV_SIGNED_TIME_REQUIRED;
					goto err_exit;
				}
				/* get verify key object */
				memset(&keyObjProperties,0,sizeof(cv_obj_properties));
				keyObjProperties.objHandle = verifyKeyHandle;
				keyObjProperties.session = (cv_session *)cvHandle;
				/* first get hash object and determine hash type */
				if ((retVal = cvGetObj(&keyObjProperties, &keyObjPtr)) != CV_SUCCESS)
					goto err_exit;
				/* validate object type */
				key = (cv_obj_value_key *)keyObjProperties.pObjValue;
				switch (keyObjPtr->objType)
				{
				case CV_TYPE_ECC_KEY:
					expectedSigLen = 2*(key->keyLen/8);
					/* assume sha256 for ecc, sha1 for others */
					hashLen = SHA256_LEN;
					break;
				case CV_TYPE_DSA_KEY:
					expectedSigLen = DSA_SIG_LEN;
					hashLen = SHA1_LEN;
					break;
				case CV_TYPE_RSA_KEY:
					expectedSigLen = key->keyLen/8;
					hashLen = SHA1_LEN;
					break;
				case 0xffff:
				default:
					retVal = CV_INVALID_OBJECT_TYPE;
					goto err_exit;
				}
				if (expectedSigLen != signatureLength)
				{
					retVal = CV_INVALID_INPUT_PARAMETER;
					goto err_exit;
				}
				/* now do requested signature verification */
				/* first do hash of time.  assume use SHA1 unless ecc */
				if ((retVal = cvAuth((uint8_t *)pTime, sizeof(cv_time), NULL, hash, hashLen, NULL, 0, 0)) != CV_SUCCESS)
					goto err_exit;
				if ((retVal = cvVerify(keyObjPtr->objType, hash, hashLen, key, pSignature, signatureLength, &verify)) != CV_SUCCESS)
					goto err_exit;
				if (verify == FALSE)
				{
					retVal = CV_INVALID_SIGNATURE;
					goto err_exit;
				}
			}
			/* time ok to use */
			memcpy(&time, pTime, sizeof(cv_time));
		}
		/* now compute otp value */
		genTokencode(0, (uint8_t *)&time, rsaToken->sharedSecret,	rsaToken->rsaParams.serialNumber,
				rsaToken->rsaParams.displayDigits, rsaToken->rsaParams.displayIntervalSeconds, &otpVal);
		memcpy(pOtpValue, &otpVal, sizeof(otpVal));
	} else {
		/* OATH token */
		/* validate output token length */
		if (*pOtpValueLength < sizeof(oathToken->sharedSecretLen ))
		{
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pOtpValueLength = sizeof(oathToken->sharedSecretLen);
		if ((retVal = cvAuth((uint8_t *)&oathToken->countValue, sizeof(oathToken->countValue), oathToken->sharedSecret, (uint8_t *)&otpValOath, 
			oathToken->sharedSecretLen, NULL, 0, 0)) != CV_SUCCESS)
			goto err_exit;
		oathToken->countValue++;
		needObjWrite = TRUE;
		memcpy(pOtpValue, &otpValOath, sizeof(otpValOath));
	}

	/* now save original object (if necessary) */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

cv_status
cv_otp_get_mac (cv_handle cvHandle, cv_obj_handle otpTokenHandle,             
				uint32_t authListLength, cv_auth_list *pAuthList, 
				cv_otp_mac_type macType,                                   
				cv_otp_nonce *serverRandom, cv_otp_nonce *pMac,
				cv_callback *callback, cv_callback_ctx context)
{
	/* This routine gets the OTP object and performs the authorization then computes a MAC value, based */
	/* on the parameters in the OTP object and the input parameters.  The purpose of this routine is to support the CT-KIP protocol.  */
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool needObjWrite = FALSE;
	cv_input_parameters inputParameters;
	cv_obj_value_rsa_token *rsaToken;
	cv_obj_value_oath_token *oathToken;
	uint8_t *sharedSecret;
	uint8_t *clientRandom;
	uint8_t message[17 + sizeof(cv_otp_nonce)];  /* message for MAC computation */
	uint8_t *messageNext = &message[0];
	uint32_t messageLen;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 5, sizeof(otpTokenHandle), &otpTokenHandle, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList, sizeof(cv_otp_mac_type), &macType,
		sizeof(cv_nonce), serverRandom,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = otpTokenHandle;
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* get token object */
	if ((retVal = cvHandlerObjAuth(CV_DENY_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &inputParameters, &objPtr, 
		&authFlags, &needObjWrite, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* verify object type */
	if (objPtr->objType != CV_TYPE_OTP_TOKEN)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	/* determine type of token */
	rsaToken = (cv_obj_value_rsa_token *)objProperties.pObjValue;
	oathToken = (cv_obj_value_oath_token *)objProperties.pObjValue;
	if (rsaToken->type == CV_OTP_TYPE_RSA)
	{
		/* RSA token */
		sharedSecret = (uint8_t *)&rsaToken->sharedSecret;
		clientRandom = (uint8_t *)&rsaToken->clientNonce;
	} else {
		/* OATH token */
		sharedSecret = (uint8_t *)&oathToken->sharedSecretLen;
		clientRandom = (uint8_t *)&oathToken->clientNonce;
	}

	/* now determine MAC type */
	if (macType == CV_MAC1)
	{
		messageNext = memcpy(messageNext, "MAC 1 computation", 17);
		messageNext = memcpy(messageNext, serverRandom, AES_128_BLOCK_LEN);
	} else if (macType == CV_MAC2)
	{
		messageNext = memcpy(messageNext, "MAC 2 computation", 17);
		messageNext = memcpy(messageNext, clientRandom, AES_128_BLOCK_LEN);
	} else {
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}
	messageLen = messageNext - message;
	if ((retVal = ctkipPrfAes(sharedSecret, message, messageLen, AES_128_BLOCK_LEN, (uint8_t *)pMac)) != CV_SUCCESS)
		goto err_exit;

	/* now save original object (if necessary) */
	if (needObjWrite)
		retVal = cvHandlerPostObjSave(&objProperties, objPtr, callback, context);

err_exit:
	return retVal;
}

#if 0 // Code to test the bulk encrypt 
typedef PACKED struct td_cv_obj_value_aes_key1 {
	uint16_t	keyLen;	/* key size in bits */
	uint8_t	key[32];		/* key */
} cv_obj_value_aes_key1;

BCM_SCAPI_STATUS bcm_cust_selftest_bulk_crypto(void)
{
	BCM_SCAPI_STATUS status=BCM_SCAPI_STATUS_OK; 
	uint8_t         cryptoHandle[BCM_SCAPI_CRYPTO_LIB_HANDLE_SIZE]={0};
	uint8_t output[256] __attribute__((aligned(4))) ;

	const uint8_t NIST_CBCAES_CipherIV[] __attribute__((aligned(4)))= {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	const uint8_t NIST_CBCAES_Plaintext[] __attribute__((aligned(4))) = {
		0x6B,0xC1,0xBE,0xE2,0x2E,0x40,0x9F,0x96,0xE9,0x3D,0x7E,0x11,0x73,0x93,0x17,0x2A,
		0xAE,0x2D,0x8A,0x57,0x1E,0x03,0xAC,0x9C,0x9E,0xB7,0x6F,0xAC,0x45,0xAF,0x8E,0x51,
		0x30,0xC8,0x1C,0x46,0xA3,0x5C,0xE4,0x11,0xE5,0xFB,0xC1,0x19,0x1A,0x0A,0x52,0xEF,
		0xF6,0x9F,0x24,0x45,0xDF,0x4F,0x9B,0x17,0xAD,0x2B,0x41,0x7B,0xE6,0x6C,0x37,0x10};

	const uint8_t NIST_CBCAES256_Key[] __attribute__((aligned(4))) =     {
		0x60,0x3D,0xEB,0x10,0x15,0xCA,0x71,0xBE,0x2B,0x73,0xAE,0xF0,0x85,0x7D,0x77,0x81,
		0x1F,0x35,0x2C,0x07,0x3B,0x61,0x08,0xD7,0x2D,0x98,0x10,0xA3,0x09,0x14,0xDF,0xF4};

	const uint8_t NIST_CBCAES256_Ciphertext[] __attribute__((aligned(4))) = {
		0xF5,0x8C,0x4C,0x04,0xD6,0xE5,0xF1,0xBA,0x77,0x9E,0xAB,0xFB,0x5F,0x7B,0xFB,0xD6,
		0x9C,0xFC,0x4E,0x96,0x7E,0xDB,0x80,0x8D,0x67,0x9F,0x77,0x7B,0xC6,0x70,0x2C,0x7D,
		0x39,0xF2,0x33,0x69,0xA9,0xD9,0xBA,0xCF,0xA5,0x30,0xE2,0x63,0x04,0x23,0x14,0x61,
		0xB2,0xEB,0x05,0xE2,0xC3,0x9B,0xE9,0xFC,0xDA,0x6C,0x19,0x07,0x8C,0x6A,0x9D,0x1B};

		cv_bulk_params pBulkParameters;
		cv_obj_value_aes_key1 key1;

//		uint8_t NIST_ECBAES_Plaintext[] __attribute__((aligned(4))) = {0x01,0x47,0x30,0xf8,0x0a,0xc6,0x25,0xfe,0x84,0xf0,0x26,0xc6,0x0b,0xfd,0x54,0x7d};
/*		014730f8 0ac625fe 84f026c6 0bfd547d*/
		/* {0x0b,0xfd,0x54,0x7d,   0x84,0xf0,0x26,0xc6, 0x84,0xf0,0x26,0xc6,  0x01,0x47,0x30,0xf8}; */
			/* {0x7d,0x54,0xfd,0x0b,0xc6,0x26,0xf0,0x84,0xc6,0x26,0xf0,0x84,0xf8,0x30,0x47,0x01 };  */
//		uint8_t NIST_ECBAES256_Key[32] __attribute__((aligned(4))) = {0};
//		uint8_t NIST_ECBAES_Ciphertext[] __attribute__((aligned(4))) ={0x5c,0x9d,0x84,0x4e,0xd4,0x6f,0x98,0x85,0x08,0x5e,0x5d,0x6a,0x4f,0x94,0xc7,0xd7};

	bcm_crypto_init((crypto_lib_handle *)cryptoHandle);


	memset(&pBulkParameters,0,sizeof(cv_bulk_params));
	memset(&key1,0,sizeof(cv_obj_value_aes_key1));

	

	pBulkParameters.bulkMode = CV_CBC;
	memset(pBulkParameters.IV,NIST_CBCAES_CipherIV,sizeof(NIST_CBCAES_CipherIV));
	pBulkParameters.IVLen = 0; //sizeof(NIST_CBCAES_CipherIV);
	key1.keyLen = sizeof(NIST_CBCAES256_Key)*8;
	memset(key1.key,NIST_CBCAES256_Key,sizeof(NIST_CBCAES256_Key));

	cvBulkEncrypt(NIST_CBCAES_Plaintext, sizeof(NIST_CBCAES_Plaintext), output, sizeof(output), 0, 0, 
				CV_BULK_ENCRYPT, &pBulkParameters, CV_TYPE_AES_KEY,(cv_obj_value_aes_key*)&key1, NULL);
				

	if (memcmp(NIST_CBCAES256_Ciphertext, output, sizeof(NIST_CBCAES256_Ciphertext)))
	{
		printf("AES 256 CBC encryption failed \n");
		return status;
	}
	else
	{
		printf("AES 256 CBC encryption Passed \n");
	}

	return status;
}
#endif



