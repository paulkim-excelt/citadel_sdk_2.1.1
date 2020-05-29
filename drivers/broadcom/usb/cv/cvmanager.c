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
#include "cvmain.h"
#include "usbbootinterface.h"
#include "bootrom.h"
#include "nvm_cpuapi.h"
#include "console.h"
#include "sbi_ush_mbox.h"
#include "qspi_flash.h"
#include "qspi_flash_misc.h"
#include "fp_spi_enum.h"
#include "drv_nextfp_spi.h"
#include "nci.h"
#include "sotp_apis.h"
#include "cvinterface.h"

#ifdef USH_BOOTROM /*AAI */
#include <toolchain/gcc.h>
#endif

#include "stub.h"


#ifndef USH_BOOTROM  /* SBI */
/* un-used funcs */
#define set_smem_openwin_secure(arg)
#define set_smem_openwin_open(arg)
#define tpmCvQueueCmdToHost(cmd) {}
extern unsigned int flash_device;

// slow
//#define ush_write_flash(flashAddr, len, buf) phx_spi_flash_write_verify(flashAddr, buf, len, flash_device)
// fast
#define ush_write_flash(flashAddr, len, buf) phx_qspi_flash_write_extrachecks(flashAddr, buf, len, flash_device)
#endif /* !USH_BOOTROM */

#include "tcpa_scl.h"
#include "tcpa_ush_remap.h"
#if defined(HID_CNSL_REDIRECT) || defined(PHX_REQ_HDOTP_DIAGS)
#include "cvdiag.h"
#endif


#ifdef USH_BOOTROM  /*AAI */
#include "sched_cmd.h"
#include "pwr.h"
#include "isr.h"
#pragma push
#pragma arm section rodata="____section_const_ss"
#pragma arm section code="__section_code_ss"
/* ECC domain parameters */
const uint8_t ecc_default_prime[] = {ECC_DEFAULT_DOMAIN_PARAMS_PRIME};
const uint8_t ecc_default_a[] = {ECC_DEFAULT_DOMAIN_PARAMS_A};
const uint8_t ecc_default_b[] = {ECC_DEFAULT_DOMAIN_PARAMS_B};
const uint8_t ecc_default_G[] = {ECC_DEFAULT_DOMAIN_PARAMS_G};
const uint8_t ecc_default_n[] = {ECC_DEFAULT_DOMAIN_PARAMS_n};
const uint8_t ecc_default_h[] = {ECC_DEFAULT_DOMAIN_PARAMS_h};
/* DSA domain parameters */
const uint8_t dsa_default_p[] = {DSA_DEFAULT_DOMAIN_PARAMS_P};
const uint8_t dsa_default_q[] = {DSA_DEFAULT_DOMAIN_PARAMS_Q};
const uint8_t dsa_default_g[] = {DSA_DEFAULT_DOMAIN_PARAMS_G};
/* signature private keys for secure session testing */
const uint8_t ecdsa_priv_test[] = {ECDSA_PRIV_TEST_KEY};
const uint8_t dsa_priv_test[] = {DSA_PRIV_TEST_KEY};

unsigned int
checkIfAntiHammeringTriggered(unsigned int timer);
void clearAntiHammeringCounters();
cv_status
cvSetupSecureSession(cv_encap_command *cmd)
{
	cv_session session, *tempSession;
	cv_status retVal = CV_SUCCESS;
	cv_secure_session_get_pubkey_out *getPubkeyOut;
	cv_secure_session_get_pubkey_in *getPubkeyIn;
	cv_secure_session_get_pubkey_in_suite_b *getPubkeyInSuiteB;
	cv_secure_session_host_nonce *hostNonce;
	cv_secure_session_host_nonce_suite_b *hostNonceSuiteB;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint8_t	hashBuf[RSA2048_KEY_LEN + sizeof(cv_nonce)]; /* this should also be large enough for ECC shared secret and 2 nonces */
	uint8_t	hash[SHA256_LEN];
	uint8_t *KdiECCPriv, *KdiDSAPriv;
	cv_nonce savedNonce;
	uint32_t sigLen, decNonceLen;
	uint32_t paramLen;
	uint32_t e_numBits, d_numBits;
	uint8_t d[RSA2048_KEY_LEN];
	fips_rng_context  rngctx;
	uint32_t hmacLen;
	uint8_t k[ECC256_KEY_LEN];

	/* check to see which command this is */
	phx_crypto_init(cryptoHandle);
	switch (cmd->commandID)
	{
	case CV_SECURE_SESSION_GET_PUBKEY:
		/* init session template */
		memset(&session, 0, sizeof(session));

		/* check to see if suite b crypto must be used */
		getPubkeyOut = (cv_secure_session_get_pubkey_out *)&cmd->parameterBlob;
		if (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS)
		{
			/* CV configured to use Suite B, check request from host */
			if (!getPubkeyOut->useSuiteB)
			{
				retVal = CV_SECURE_SESSION_SUITE_B_REQUIRED;
				goto err_exit;
			}
			session.flags |= CV_USE_SUITE_B;
		}
		if (getPubkeyOut->useSuiteB)
			session.flags |= CV_USE_SUITE_B;
		/* save nonce */
		memcpy(savedNonce.nonce, getPubkeyOut->nonce, CV_NONCE_LEN);
		if (session.flags & CV_USE_SUITE_B)
		{
			/* check to see if key exists already */
			if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_SECURE_SESS_ECC_KEY))
			{
				/* no, need to create one */
				if (phx_ecp_diffie_hellman_generate(cryptoHandle, PRIME_FIELD, (uint8_t *)ecc_default_prime, ECC256_KEY_LEN_BITS,
					(uint8_t *)ecc_default_a, (uint8_t *)ecc_default_b, (uint8_t *)ecc_default_n,
					(uint8_t *)ecc_default_G, (uint8_t *)ecc_default_G + ECC256_KEY_LEN,
					CV_PERSISTENT_DATA->eccSecureSessionSetupKey.x, 0,
					CV_PERSISTENT_DATA->eccSecureSessionSetupKey.Q, CV_PERSISTENT_DATA->eccSecureSessionSetupKey.Q + ECC256_KEY_LEN,
					NULL, NULL) != PHX_STATUS_OK)
				{
					retVal = CV_SECURE_SESSION_KEY_GENERATION_FAIL;
					goto err_exit;
				}
				/* now set flag and write persistent data to flash */
				CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_SECURE_SESS_ECC_KEY;
				if ((retVal = cvWritePersistentData(TRUE)) != CV_SUCCESS)
					goto err_exit;
			}
			/* set up values to go into command parameter blob */
			getPubkeyInSuiteB = (cv_secure_session_get_pubkey_in_suite_b *)&cmd->parameterBlob;
			getPubkeyInSuiteB->useSuiteB = TRUE;
			/* copy key to output buffer */
			/* need to byteswap since CNG expects big endian */
			cvCopySwap((uint32_t *)&CV_PERSISTENT_DATA->eccSecureSessionSetupKey.Q[0], (uint32_t *)&getPubkeyInSuiteB->pubkey[0], ECC256_KEY_LEN);
			cvCopySwap((uint32_t *)&CV_PERSISTENT_DATA->eccSecureSessionSetupKey.Q[ECC256_KEY_LEN], (uint32_t *)&getPubkeyInSuiteB->pubkey[ECC256_KEY_LEN], ECC256_KEY_LEN);

			/* now generate signature of pubkey || anti-replay nonce, using device signing key */
			/* determine whether should use Kdi-ECC in 6T or Kdix in persistent data */
			if (CV_PERSISTENT_DATA->persistentFlags & CV_USE_KDIX_ECC)
				/* use persistent data value */
				KdiECCPriv = CV_PERSISTENT_DATA->secureSessionSigningKey.eccKey.x;
			else {
				/* if customer ID 0, use test values, otherwise, use NVM values */
				if(ush_get_custid()) {
					/* use NVM value */
					KdiECCPriv = NV_6TOTP_DATA->KecDSA;
				} else {
					/* use test value */
					KdiECCPriv = (uint8_t *)&ecdsa_priv_test[0];
				}
			}
			memcpy(hashBuf, (uint8_t *)&getPubkeyInSuiteB->pubkey[0], ECC256_POINT);
			if ((retVal = cvAuth(hashBuf, ECC256_POINT, NULL, hash, SHA256_LEN, savedNonce.nonce, sizeof(cv_nonce), 0)) != CV_SUCCESS)
				goto err_exit;
			/* convert the hash to BIGINT */
			cvInplaceSwap((uint32_t *)hash, SHA256_LEN/4);
			if ((phx_ecp_ecdsa_sign(cryptoHandle, hash, PRIME_FIELD, (uint8_t *)ecc_default_prime,
				ECC256_KEY_LEN_BITS, (uint8_t *)ecc_default_a, (uint8_t *)ecc_default_b, (uint8_t *)ecc_default_n,
				(uint8_t *)ecc_default_G[0],(uint8_t *)ecc_default_G[ECC256_KEY_LEN],
				(uint8_t *)k, 0, KdiECCPriv, getPubkeyInSuiteB->sig, getPubkeyInSuiteB->sig + ECC256_SIZE_R,
				NULL, NULL)) != PHX_STATUS_OK)
			{
				retVal = CV_SECURE_SESSION_KEY_SIGNATURE_FAIL;
				goto err_exit;
			}
			/* convert the output from BIGINT */
			cvInplaceSwap((uint32_t *)getPubkeyInSuiteB->sig, ECC256_SIG_LEN/2);
			cvInplaceSwap((uint32_t *)getPubkeyInSuiteB->sig[ECC256_SIG_LEN/2], ECC256_SIG_LEN/2);
			/* create client nonce (temporarily store in session HMAC key) */
			if ((retVal = cvRandom(CV_NONCE_LEN, (uint8_t *)&session.secureSessionHMACKey)) != CV_SUCCESS)
				goto err_exit;
			memcpy(getPubkeyInSuiteB->nonce, (uint8_t *)&session.secureSessionHMACKey, CV_NONCE_LEN);
			paramLen = sizeof(cv_secure_session_get_pubkey_in_suite_b);
		} else {
			/* not using Suite B */
			/* check to see if key exists already.  check both flag and if non-zero, in case flag erroneously got */
			/* set when key didn't get created */
			if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_SECURE_SESS_RSA_KEY)
				|| cvIsZero((uint8_t *)&CV_PERSISTENT_DATA->rsaSecureSessionSetupKey, sizeof(cv_rsa2048_key)))
			{
				uint32_t mLen, qLen = 0, pLen = 0, pinvLen, edqLen, edpLen;

				/* no, need to create one */
				/* first check to see if one is available from the TPM key cache */
				if ((retVal = tpmGetKey((uint8_t *)&CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.p,(uint8_t *)&CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.q)) == CV_SUCCESS)
				{
					/* key in key cache, use p and q values to generate others */
					pLen = RSA2048_KEY_LEN_BITS/2;
					qLen = RSA2048_KEY_LEN_BITS/2;
				}

				/* now generate key */
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.e = RSA_PUBKEY_CONSTANT;
				e_numBits = 32;
				if ((phx_rsa_key_generate(cryptoHandle, RSA2048_KEY_LEN_BITS,
					&e_numBits, (uint8_t *)&CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.e,
					&pLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.p,
					&qLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.q,
					&mLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.m,
					&d_numBits, d,
					&edpLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.dp,
					&edqLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.dq,
					&pinvLen, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.pinv)) != PHX_STATUS_OK)
				{
					retVal = CV_SECURE_SESSION_KEY_GENERATION_FAIL;
					CV_PERSISTENT_DATA->persistentFlags &= ~CV_HAS_SECURE_SESS_RSA_KEY;
					goto err_exit;
				}
				/* now set flag and write persistent data to flash */
				CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_SECURE_SESS_RSA_KEY;
				if ((retVal = cvWritePersistentData(TRUE)) != CV_SUCCESS)
					goto err_exit;
			}
			/* set up values to go into command parameter blob */
			getPubkeyIn = (cv_secure_session_get_pubkey_in *)&cmd->parameterBlob;
			getPubkeyIn->useSuiteB = FALSE;
			/* copy key to output buffer */
			memcpy(getPubkeyIn->pubkey, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.m, RSA2048_KEY_LEN);
//			cvInplaceSwap((uint32_t *)getPubkeyIn->pubkey, RSA2048_KEY_LEN/4);
			/* byteswap the buffer */
//			for (i=0;i<RSA2048_KEY_LEN;i+=4)
//				cvByteSwap(&(getPubkeyIn->pubkey[i]));
			/* now generate signature of pubkey || anti-replay nonce, using device signing key */
			/* determine whether should use Kdi-ECC in 6T or Kdix in persistent data */
			if (CV_PERSISTENT_DATA->persistentFlags & CV_USE_KDIX_DSA)
				/* use persistent data value */
				KdiDSAPriv = CV_PERSISTENT_DATA->secureSessionSigningKey.dsaKey.x;
			else {
				/* if customer ID 0, use test values, otherwise, use NVM values */
				if(ush_get_custid()) {
					/* use NVM value */
					KdiDSAPriv = NV_6TOTP_DATA->Kdi_priv;
				} else {
					/* use test value */
					KdiDSAPriv = (uint8_t *)&dsa_priv_test[0];
				}
			}
			memcpy(hashBuf, CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.m, RSA2048_KEY_LEN);
			if ((retVal = cvAuth(hashBuf, RSA2048_KEY_LEN, NULL, hash, SHA1_LEN, savedNonce.nonce, sizeof(cv_nonce), 0)) != CV_SUCCESS)
				goto err_exit;
			/* convert the hash to BIGINT */
			cvInplaceSwap((uint32_t *)hash, SHA1_LEN/4);
			if ((phx_rng_fips_init(cryptoHandle, &rngctx, TRUE)) != PHX_STATUS_OK) /* rng selftest included */
			{
				retVal = CV_RNG_FAILURE;
				goto err_exit;
			}
#if 0
			if ((phx_dsa_sign(cryptoHandle, hash, NULL,
				(uint8_t *)dsa_default_p, DSA1024_KEY_LEN_BITS, (uint8_t *)dsa_default_q, (uint8_t *)dsa_default_g,
				KdiDSAPriv,
				getPubkeyIn->sig, &sigLen, NULL, NULL)) != PHX_STATUS_OK)
			{
				retVal = CV_SECURE_SESSION_KEY_SIGNATURE_FAIL;
				goto err_exit;
			}
#endif

			/* convert the output from BIGINT */
			cvInplaceSwap((uint32_t *)&getPubkeyIn->sig[0], CV_NONCE_LEN/sizeof(uint32_t));
			cvInplaceSwap((uint32_t *)&getPubkeyIn->sig[CV_NONCE_LEN], CV_NONCE_LEN/sizeof(uint32_t));
			/* create client nonce (temporarily store in session HMAC key) */
			if ((retVal = cvRandom(CV_NONCE_LEN, (uint8_t *)&session.secureSessionHMACKey)) != CV_SUCCESS)
				goto err_exit;
			memcpy(getPubkeyIn->nonce, (uint8_t *)&session.secureSessionHMACKey, CV_NONCE_LEN);
			paramLen = sizeof(cv_secure_session_get_pubkey_in);
		}
		if ((tempSession = cv_malloc(sizeof(cv_session))) == NULL)
		{
			retVal = CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
			goto err_exit;
		}
		/* now save session using temporary pointer from volatile data */
		memcpy(tempSession, &session, sizeof(cv_session));
		memcpy((uint8_t *)&tempSession->magicNumber,"SeSs", sizeof(tempSession->magicNumber));
		CV_VOLATILE_DATA->secureSession = tempSession;
		break;

	case CV_SECURE_SESSION_HOST_NONCE:
		/* now need to create encryption key */
		tempSession = CV_VOLATILE_DATA->secureSession;
		if (tempSession->flags & CV_USE_SUITE_B)
		{
			/* check to make sure correct parameters sent */
			if (cmd->parameterBlobLen != sizeof(cv_secure_session_host_nonce_suite_b))
			{
				retVal = CV_SECURE_SESSION_BAD_PARAMETERS;
				goto err_exit;
			}
			/* now, create shared secret */
			hostNonceSuiteB = (cv_secure_session_host_nonce_suite_b *)&cmd->parameterBlob;
			/* convert the pubkey to BIGINT */
			cvInplaceSwap((uint32_t *)&hostNonceSuiteB->pubkey[0], ECC256_KEY_LEN/sizeof(uint32_t));
			cvInplaceSwap((uint32_t *)&hostNonceSuiteB->pubkey[ECC256_KEY_LEN], ECC256_KEY_LEN/sizeof(uint32_t));
			if ((phx_ecp_diffie_hellman_shared_secret(cryptoHandle, PRIME_FIELD, (uint8_t *)ecc_default_prime,
				ECC256_KEY_LEN_BITS, (uint8_t *)ecc_default_a, (uint8_t *)ecc_default_b, (uint8_t *)ecc_default_n,
				&CV_PERSISTENT_DATA->eccSecureSessionSetupKey.x[0], &hostNonceSuiteB->pubkey[0],
				 &hostNonceSuiteB->pubkey[ECC256_KEY_LEN], &hashBuf[SHA1_LEN], &hashBuf[SHA1_LEN + ECC256_KEY_LEN],
				NULL, NULL)) != PHX_STATUS_OK)
			{
				retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
				goto err_exit;
			}
			/* convert the x component of output from BIGINT and do SHA1 (to match what CNG does in NCryptDeriveKey) */
			cvInplaceSwap((uint32_t *)&hashBuf[SHA1_LEN], ECC256_KEY_LEN/sizeof(uint32_t));
			if ((retVal = cvAuth(&hashBuf[SHA1_LEN], ECC256_KEY_LEN, NULL, hashBuf, SHA1_LEN,
				NULL, 0, 0)) != CV_SUCCESS)
			{
				retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
				goto err_exit;
			}
			/* copy CV and host nonces*/
			memcpy(hashBuf + SHA1_LEN, tempSession->secureSessionHMACKey.SHA1, CV_NONCE_LEN);
			memcpy(hashBuf + SHA1_LEN + CV_NONCE_LEN, hostNonceSuiteB->nonce, CV_NONCE_LEN);
			if ((retVal = cvAuth(hashBuf, SHA1_LEN + 2*CV_NONCE_LEN, NULL, hash, SHA1_LEN,
				NULL, 0, 0)) != CV_SUCCESS)
			{
				retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
				goto err_exit;
			}
		} else {
			/* check to make sure correct parameters sent */
			if (cmd->parameterBlobLen != sizeof(cv_secure_session_host_nonce))
			{
				retVal = CV_SECURE_SESSION_BAD_PARAMETERS;
				goto err_exit;
			}
			/* now, create shared secret */
			hostNonce = (cv_secure_session_host_nonce *)&cmd->parameterBlob;
			/* convert the encrypted host nonce to BIGINT */
//			cvInplaceSwap(&hostNonce->encNonce, RSA2048_KEY_LEN/4);
//			cvInplaceSwap((uint32_t *)hostNonce, RSA2048_KEY_LEN/4);
//			for (i=0;i<RSA2048_KEY_LEN;i+=4)
//				cvByteSwap(&(hostNonce->encNonce[i]));
			if ((phx_rsa_mod_exp_crt(cryptoHandle, hostNonce->encNonce, sizeof(hostNonce->encNonce)*8,
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.dq,
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.q, sizeof(CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.q)*8,
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.dp,
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.p, sizeof(CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.p)*8,
				CV_PERSISTENT_DATA->rsaSecureSessionSetupKey.pinv,
				d, &decNonceLen, NULL, NULL)) != PHX_STATUS_OK)
			{
				retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
				goto err_exit;
			}
			cvInplaceSwap((uint32_t *)d, CV_NONCE_LEN/4);
			memcpy(hashBuf, tempSession->secureSessionHMACKey.SHA1, CV_NONCE_LEN);
			memcpy(hashBuf + CV_NONCE_LEN, d, CV_NONCE_LEN);

			if ((retVal = cvAuth(hashBuf, 2*CV_NONCE_LEN , NULL, hash, SHA1_LEN,
				NULL, 0, 0)) != CV_SUCCESS)
			{
				retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
				goto err_exit;
			}
		}
		memcpy(tempSession->secureSessionKey, hash, AES_128_BLOCK_LEN);
		/* compute HMAC key */
		memcpy(hashBuf, "CV secure session blob", 23);
		if (tempSession->flags & CV_USE_SUITE_B)
			hmacLen = SHA256_LEN;
		else
			hmacLen = SHA1_LEN;
		if ((cvAuth(hashBuf, 23, NULL, tempSession->secureSessionHMACKey.SHA256, hmacLen, NULL, 0, 0)) != CV_SUCCESS)
		{
			retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
			goto err_exit;
		}
		paramLen = 0;
		break;
	case 0xffff:
	default:
		retVal = CV_SECURE_SESSION_SHARED_SECRET_FAIL;
		goto err_exit;
	}
	/* now set up rest of fields in command */
	cmd->parameterBlobLen = paramLen;
	cmd->returnStatus = retVal;
	cmd->transportLen = sizeof(cv_encap_command) - sizeof(uint32_t) + cmd->parameterBlobLen;
	return CV_SUCCESS;
err_exit:
	cmd->parameterBlobLen = 0;
	cmd->returnStatus = retVal;
	cmd->transportLen = sizeof(cv_encap_command) - sizeof(uint32_t) + cmd->parameterBlobLen;
	return retVal;
}
#pragma pop
#endif /* USH_BOOTROM */

#ifdef USH_BOOTROM /* AAI */
cv_status
cvDecryptCmd(cv_encap_command *cmd)
{
	uint32_t authLen, cryptLen, hmacLen;
	cv_session *session;
	uint8_t *pHMAC;
	cv_status retVal = CV_SUCCESS;
	cv_param_list_entry *paramListEntry;

	/* determine which hash to use */
	/* first check to see if session available (not if this is cv_open command) */
	if (cmd->commandID == CV_CMD_OPEN)
	{
		session = (cv_session *)cmd->hSession;
		if (session == 0 || cvValidateSessionHandle((cv_handle)session) != CV_SUCCESS)
			return CV_INVALID_HANDLE;
	} else {
		session = (cv_session *)cmd->hSession;
		if (session == 0)
			return retVal;
		else if ((retVal = cvValidateSessionHandle((cv_handle)session)) != CV_SUCCESS)
		{
			/* this session doesn't exist.  check to see if the command is cv_close.  if so, */
			/* just return status to let this command succeed, because cv_close would have */
			/* just deleted it anyway */
			if (cmd->commandID == CV_CMD_CLOSE)
			{
				/* since can't decrypt command, just fill in session handle and let cv_close */
				/* get executed normally */
				paramListEntry = (cv_param_list_entry *)&cmd->parameterBlob;
				paramListEntry->paramType = CV_ENCAP_STRUC;
				paramListEntry->paramLen = sizeof(cv_handle);
				paramListEntry->param = cmd->hSession;
				return CV_SUCCESS;
			} else
				return retVal;
		}
	}
	if (session->flags & CV_USE_SUITE_B)
		hmacLen = SHA256_LEN;
	else
		hmacLen = SHA1_LEN;

	cryptLen = cmd->transportLen - sizeof(cv_encap_command) + sizeof(uint32_t);
	authLen = cmd->transportLen - cryptLen + cmd->parameterBlobLen;
	pHMAC = (uint8_t *)cmd + authLen;
	retVal = cvDecryptBlob((uint8_t *)cmd, authLen, (uint8_t *)&cmd->parameterBlob, cryptLen,
		NULL, 0,
		cmd->IV, session->secureSessionKey, NULL, NULL, (uint8_t *)&session->secureSessionHMACKey,
		pHMAC, hmacLen, CV_CBC,
		(uint8_t *)&session->secureSessionUsageCount, sizeof(session->secureSessionUsageCount));

#if 0
	/* temp skip HMAC check */
	if (retVal == CV_PARAM_BLOB_AUTH_FAIL)
		retVal = CV_SUCCESS;
#endif

	return retVal;
}

cv_status
cvEncryptCmd(cv_encap_command *cmd)
{
	uint32_t authLen, cryptLen, hmacLen, padLen;
	cv_session *session;
	uint8_t *pHMAC;
	cv_status retVal = CV_SUCCESS;
	uint8_t padByte;

	/* now check to see which HMAC to use */
	session = (cv_session *)cmd->hSession;
	if (session->flags & CV_USE_SUITE_B)
		hmacLen = SHA256_LEN;
	else
		hmacLen = SHA1_LEN;
	cryptLen = cmd->parameterBlobLen + hmacLen;
	/* see if padding required */
	padLen = AES_BLOCK_LENGTH - (cryptLen  % AES_BLOCK_LENGTH);
	padByte = (padLen == AES_BLOCK_LENGTH) ? AES_BLOCK_LENGTH : padLen;
	memset((uint8_t *)&cmd->parameterBlob + cryptLen, padByte, padLen);
	cryptLen += padLen;
	/* compute auth length and recompute entire command length */
	authLen = cmd->transportLen;
	cmd->transportLen += hmacLen + padLen;
	pHMAC = (uint8_t *)cmd + authLen;
	if ((retVal = cvEncryptBlob((uint8_t *)cmd, authLen, (uint8_t *)&cmd->parameterBlob, cryptLen,
		NULL, 0,
		cmd->IV, session->secureSessionKey, NULL, NULL, (uint8_t *)&session->secureSessionHMACKey, pHMAC, hmacLen, CV_CBC,
		(uint8_t *)&session->secureSessionUsageCount, sizeof(session->secureSessionUsageCount))) != CV_SUCCESS)
			goto err_exit;


	/* bump the secure session usage count, unless this command will be resubmitted */
	if (cmd->returnStatus != CV_ANTIHAMMERING_PROTECTION_DELAY_MED && cmd->returnStatus != CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH)
		session->secureSessionUsageCount++;

err_exit:
	return retVal;
}
#endif /*USH_BOOTROM */

cv_status
cvDecapsulateCmd(cv_encap_command *cmd, uint32_t **inputParams, uint32_t *numInputParams)
{
	uint32_t *endPtr;
	cv_param_list_entry *paramEntryPtr;
	cv_status retVal = CV_SUCCESS;
	uint32_t cmdSize = cmd->transportLen;
	uint8_t *nextParamStart;

#ifdef USH_BOOTROM /* AAI */
	/* first check version field of command */
	if ( !((cmd->version.versionMajor == CV_MAJOR_VERSION) &&
	 	(cmd->version.versionMinor == CV_MINOR_VERSION))  &&
		!((cmd->version.versionMajor == CV_MAJOR_VERSION_1) &&
	 	(cmd->version.versionMinor == CV_MINOR_VERSION_0))  ) {

		/* Reassign the version so that the return parameter version is valid */
		cvGetVersion(&CV_VOLATILE_DATA->curCmdVersion);

		retVal = CV_INVALID_VERSION;
		goto err_exit;
	}
#endif /* USH_BOOTROM */

	/* validate parameter blob length */
	if (cmd->parameterBlobLen > (cmd->transportLen - sizeof(cv_encap_command) + sizeof(uint32_t)))
	{
		retVal = CV_PARAM_BLOB_INVALID_LENGTH;
		goto err_exit;
	}

	*numInputParams = 0;

#ifdef USH_BOOTROM /* AAI */
	/* if this is a secure session setup, don't need to decapsulate */
	if (cmd->commandID == CV_SECURE_SESSION_GET_PUBKEY || cmd->commandID == CV_SECURE_SESSION_HOST_NONCE)
	{
		/* first check to make sure session is valid in the case of CV_SECURE_SESSION_HOST_NONCE */
		if (cmd->commandID == CV_SECURE_SESSION_HOST_NONCE && cvValidateSessionHandle(cmd->hSession) != CV_SUCCESS)
		{
			retVal = CV_INVALID_HANDLE;
			goto err_exit;
		}
		goto normal_exit;
	}
	/* check to see if this command is encrypted */
	if (cmd->flags.secureSession)
	{
		if ((retVal = cvDecryptCmd(cmd)) != CV_SUCCESS)
			goto err_exit;
	}
#endif

	/* now decapsulate parameters.  parse parameter blob and save pointers */
	paramEntryPtr = (cv_param_list_entry *)&cmd->parameterBlob;
	endPtr = (uint32_t *)paramEntryPtr + (ALIGN_DWORD(cmd->parameterBlobLen)/sizeof(uint32_t));

	/* skip param validation if blob len == 0 */
	if (cmd->parameterBlobLen == 0 )  {
		goto  normal_exit;
	}

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
			/* check to see if NULL pointer (ie, parameter length is zero) */
			if (paramEntryPtr->paramLen)
				inputParams[*numInputParams] = &paramEntryPtr->param;
			else
				inputParams[*numInputParams] = NULL;
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
				if (((cmd->commandID == CV_CMD_FINGERPRINT_CAPTURE || (cmd->commandID == CV_CMD_FINGERPRINT_CAPTURE_WBF)) && ((cv_bool)*inputParams[1]) == TRUE) 
					|| (cmd->commandID == CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT && ((cv_bool)*inputParams[1]) == TRUE)
					|| cmd->commandID == CV_CMD_RECEIVE_BLOCKDATA)
					cmdSize -= paramEntryPtr->paramLen;
				else {
					retVal = CV_PARAM_BLOB_INVALID_LENGTH;
					goto err_exit;
				}
			}
			nextParamStart = (uint8_t *)paramEntryPtr + 2*sizeof(uint32_t);
			memmove(nextParamStart + ALIGN_DWORD(paramEntryPtr->paramLen), nextParamStart, (uint8_t *)endPtr - nextParamStart);
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
normal_exit:
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

#ifdef USH_BOOTROM /* AAI */
	cv_session *session;
#endif

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
		memset((uint8_t *)&paramEntryPtr->param + paramEntryPtr->paramLen, 0, ALIGN_DWORD(paramEntryPtr->paramLen - paramEntryPtr->paramLen));

		/* check to see this parameter list needs to be moved to be adjacent to previous one */
		if (paramLenPrev)
			if (((uint8_t *)paramEntryPtrPrev + paramLenPrev) < (uint8_t *)paramEntryPtr)
			{
				memmove((uint8_t *)paramEntryPtrPrev + paramLenPrev, paramEntryPtr, paramLen);
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

	//Should this piece of code be here?? - Ani
	/* bump the secure session usage count.  this is regardless whether it is a secure session or not */
	/* because this count is also possibly used for HMAC-type auth computations by app */
	// Only if we are not closing session
	if (cmd->commandID != CV_CMD_CLOSE)
	{
		if (cmd->hSession)
		{
#ifdef USH_BOOTROM /* AAI */
			session = (cv_session *)cmd->hSession;

			/* now encrypt command, if secure session. */
			/* if invalid handle detected during decapsulation, skip encryption */
			if (cmd->flags.secureSession && cmd->returnStatus != CV_INVALID_HANDLE)
			{
				if ((retVal = cvEncryptCmd(cmd)) != CV_SUCCESS)
				{
					cmd->parameterBlobLen = 0;
					cmd->returnStatus = retVal;
					cmd->transportLen = sizeof(cv_encap_command) - sizeof(uint32_t) + cmd->parameterBlobLen;
					goto err_exit;
				}
			} else {
				/* don't bump count if this command will be resubmitted */
				if (cmd->returnStatus != CV_ANTIHAMMERING_PROTECTION_DELAY_MED && cmd->returnStatus != CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH)
					session->secureSessionUsageCount++;
			}
#endif
		}
	}
	return CV_SUCCESS;

#ifdef USH_BOOTROM /* AAI */
err_exit:
	return retVal;
#endif

}

#ifdef USH_BOOTROM  /*AAI */

cv_status
cvActualCallback (uint32_t status, uint32_t extraDataLength,
							void * extraData, void  *arg)
{
	cv_encap_command *cmd;
	cv_internal_callback_ctx *ctx;
	uint32_t startTime;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;

	cv_status retHost = CV_SUCCESS;



	/* set up encapsulated status */
	ctx = (cv_internal_callback_ctx *)arg;
	cmd = ctx->cmd;

	/* copy command to io buf */
	memcpy(CV_SECURE_IO_COMMAND_BUF, cmd, cmd->transportLen - cmd->parameterBlobLen);
	cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;

	/* if extra data provided, copy it where parameter blob goes.  NOTE: commands must have sufficient space */
	if (extraDataLength)
	{
		memcpy((uint8_t *)&cmd->parameterBlob, (uint8_t *)extraData, extraDataLength);
		cmd->parameterBlobLen = extraDataLength;
	} else
		cmd->parameterBlobLen = 0;
	cmd->returnStatus = status;
	cmd->flags.returnType = CV_RET_TYPE_INTERMED_STATUS;
	cmd->transportLen = sizeof(cv_encap_command) - sizeof(uint32_t) + cmd->parameterBlobLen;

	/* first post buffer to host to receive callback status.  it's ok to use the same buffer that is posted */
	/* as output to host, because no data from host will be received until callback has been processed */
	/* set up interrupt after command */
	usbInt = (cv_usb_interrupt *)((uint8_t *)cmd + cmd->transportLen);
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	usbInt->resultLen = cmd->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	memset(CV_SECURE_IO_COMMAND_BUF + cmd->transportLen + sizeof(cv_usb_interrupt), 0, CV_IO_COMMAND_BUF_SIZE - (cmd->transportLen + sizeof(cv_usb_interrupt)));
	set_smem_openwin_open(CV_IO_OPEN_WINDOW);
	/* now wait for USB transfer complete */
	if (!usbCvSend(usbInt, (uint8_t *)cmd, cmd->transportLen, HOST_RESPONSE_TIMEOUT)) {
		retVal = CV_CALLBACK_TIMEOUT;
		goto err_exit;
	}

	/* Wait for status command from Host */
	/* Don't overwrite if we already received the response ...since we are almost always on for response */
	ENTER_CRITICAL();
	if (!usbCvPollCmdRcvd()) {
		usbCvBulkOut(cmd);
	}
	EXIT_CRITICAL();


	/* now wait for result from host */
	get_ms_ticks(&startTime);
	do
	{
		int cmds_done;

		if(cvGetDeltaTime(startTime) >= HOST_RESPONSE_TIMEOUT)
		{
			retVal = CV_CALLBACK_TIMEOUT;
			goto err_exit;
		}
		cmds_done = phx_delay_sched_task_with_yield_with_wakereg(100, usbCvPollCmdPtr(), 1 );

		if (cmds_done > 0) {
			CV_VOLATILE_DATA->fp_actual_polling_period = FP_POLLING_PERIOD_FAST;	/* Poll faster to get more commands */
			/*printf("CV: Polling Faster\n");*/
		}
		else
			CV_VOLATILE_DATA->fp_actual_polling_period = CV_VOLATILE_DATA->fp_polling_period;

		if (IS_SYSTEM_RESTARTING()) {
			printf("CV: System restarting... skipped fp detect loop\n");
			retVal = CV_CALLBACK_TIMEOUT;
			goto err_exit;
		}

		//yield_and_delay(100);
		if (usbCvPollCmdRcvd())
			break;
	} while (TRUE);

	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);

	retHost = cmd->returnStatus;

	ENTER_CRITICAL();
	if (usbCvPollCmdRcvd()) {
		usbCvBulkOut(cmd);
	}
	EXIT_CRITICAL();

	return retHost;

err_exit:
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	return retVal;
}

cv_status
cvCallback (uint32_t status, uint32_t extraDataLength,
							void * extraData, void  *arg)
{
	/* this routine invokes cvActualCallback in order to allow the callback rtn to be firmware */
	/* upgradable.  */
	cv_status retVal;

	retVal = cvActualCallback (status, extraDataLength, extraData, arg);
	return retVal;
}

#endif /* USH_BOOTROM */

uint8_t *
cvWord2HalfWord(uint32_t *word)
{
	/* this routine handles the case where the length portion of a length/value pair is 16-bits, instead */
	/* of 32-bits by shifting it down */
	*word = *word<<16;
	return ((uint8_t *)word + 2);
}

uint8_t *
cvWord2Byte(uint32_t *word)
{
	/* this routine handles the case where the length portion of a length/value pair is 8-bits, instead */
	/* of 32-bits by shifting it down */
	*word = *word<<24;
	return ((uint8_t *)word + 3);
}

/*
 * This is used to validate if a command looks to be correct or not.
 * buf points to the beginning of the command
 * len is the length of data received so far.
 * complete is true if we think the whole command has been received
 *
 * Note that this function will be called with partial commands, unless complete is
 * true, this is not an error.
 */
int
cvIsCmdValid(void *buf, uint32_t len, uint32_t complete)
{
	uint32_t *ptr = (uint32_t *) buf;
	cv_encap_command *cmd = (cv_encap_command *)buf;

	/* Check for bogus 12-byte packet from old versions of the host services */
	if (len == 12) {
		if ((ptr[0] == CV_TRANS_TYPE_HOST_STORAGE)
		    && (ptr[2] == CV_HOST_STORAGE_GET_REQUEST)) {
		    	printf("CV: Bad host-storage packet detected\n");
		    	return 0;
		}
		if ((ptr[0] == CV_TRANS_TYPE_HOST_CONTROL)
		    && (ptr[2] == CV_HOST_CONTROL_GET_REQUEST)) {
		    	printf("CV: Bad host-control packet detected\n");
			return 0;
		}
	}

	/* Check for various bogus commands */
	if (cmd->transportLen > CV_IO_COMMAND_BUF_SIZE) {
		printf("CV: command too long (%d)\n", cmd->transportLen);
		return 0;
	}
	/* If we received more data than the command is long, error */
	if (cmd->transportLen < len) {
		printf("CV: wrong header length (%d < %d)\n", cmd->transportLen, len);
		return 0;
	}
	/* Tests that only apply to complete commands */
	if (complete) {
		if (cmd->transportLen != len) {
			printf("CV: wrong length received (%d != %d)\n", cmd->transportLen, len);
			return 0;
		}
	}
	return 1;
}

#ifdef USH_BOOTROM /*AAI */
void
cvManager(void)
{
	cv_status retValCmd = CV_SUCCESS;
	uint32_t *inputParams[MAX_INPUT_PARAMS];
	uint32_t *outputParams[MAX_OUTPUT_PARAMS];
	uint32_t outputLengths[MAX_OUTPUT_PARAMS];
	uint32_t outputEncapTypes[MAX_OUTPUT_PARAMS];
	uint32_t	numInputParams;
	cv_param_list_entry *outputParamEntry;
	uint32_t numOutputParams = 0;
	uint8_t encapsulateResult = TRUE;

	cv_usb_interrupt *usbInt;
	cv_encap_command *cmd;
	cv_command_id commandID;
	cv_bool captureOnlySpecialCase = FALSE;
	cv_encap_flags sav_encap_flags;
	cv_transport_type sav_transportType;
	memset(outputParams, 0, MAX_OUTPUT_PARAMS);
	memset(outputLengths, 0, MAX_OUTPUT_PARAMS);
	memset(outputEncapTypes,0, MAX_OUTPUT_PARAMS);
// #define DEBUG_CVMANAGER 1


#ifdef USH_BOOTROM  /*AAI */
	cv_internal_callback_ctx ctx;
	uint32_t reboot_cmd=0, do_menu_cmds=0, do_pwr_cmds=0;
	uint32_t inputParams_0 =0;
	uint8_t *captureOnlyCmdPtr = NULL;
	cv_volatile *volatileData = CV_VOLATILE_DATA;
	cv_bulk_params *pBulkParameters;
#endif

#ifdef DEBUG_CVMANAGER
    printf("%s enter\n",__func__);
#endif
	/* first, copy from iobuf to command buf */
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	memcpy(CV_COMMAND_BUF, cmd, cmd->transportLen);
	cmd = (cv_encap_command *)CV_COMMAND_BUF;

#ifdef USH_BOOTROM  /*AAI */
	printf("%s: cid %x \n",__func__, (int)cmd->commandID);
#endif

	/* Save encap command values for later use, since buffer is reused during host storage communication */
	sav_encap_flags = cmd->flags;
	CV_VOLATILE_DATA->curCmdVersion = cmd->version;
	sav_transportType = cmd->transportType;
	commandID = cmd->commandID;

	/* reenable antihammering tests, if disabled during last command */
	CV_VOLATILE_DATA->CVInternalState &= ~CV_ANTIHAMMERING_SUSPENDED;
	/* clear CV-only radio mode override, in case previous command was interrupted before could be cleared */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_OVERRIDE;
#ifdef USH_BOOTROM  /*AAI */
	/* invalidate HDOTP skip table, in case read in by diags */
	cvInvalidateHDOTPSkipTable();
#endif

	/* next, decapsulate command */
	if ((retValCmd = cvDecapsulateCmd(cmd, &inputParams[0], &numInputParams)) != CV_SUCCESS) {
#ifdef DEBUG_CVMANAGER
    printf("%s: cvdecapulate fail ret %d\n",__func__,retValCmd);
#endif
		goto cmd_err;
    }

	/* command buffer used for output as well.  API handlers must ensure that input parameters not overwritten */
	outputParamEntry = (cv_param_list_entry *)&cmd->parameterBlob;
#ifdef USH_BOOTROM  /*AAI */
	/* set up pointer to command in context */
	ctx.cmd = cmd;
#endif

	/* indicate that CV command is active */
	CV_VOLATILE_DATA->CVDeviceState |= CV_COMMAND_ACTIVE;

	/* set USH Initialized status */
	// CV_VOLATILE_DATA->CVDeviceState |= CV_USH_INITIALIZED;

#ifdef USH_BOOTROM  /*AAI */
	/* check to see if async FP task needs to be cancelled (deferred from reinit) */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_REINIT_CANCELLATION)
	{
		printf("cvManager: calling cvFpCancel\n");
		cvFpCancel(FALSE);
	}
	/* check to see if an async FP capture was initiated by cv_identify_user.  if so, cancel if any other command is sent */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS) && (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL)
		&& cmd->commandID != CV_CMD_IDENTIFY_USER)
	{
		printf("cvUSHfpCapture: calling cvFpCancel for cv_identify_user\n");
		cvFpCancel(FALSE);
		/* set first pass flag for PBA, so that FP capture status will get cleared before new capture is initiated */
		CV_VOLATILE_DATA->CVDeviceState |= CV_FIRST_PASS_PBA;
	}
	/* if CV cmd cancellation was sent while previous CV cmd was active, clear it here */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CANCEL_PENDING_CMD;

#endif
	switch (cmd->commandID)
	{
#ifdef USH_BOOTROM  /*AAI */
	case CV_CMD_OPEN:
	case CV_CMD_OPEN_REMOTE:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_status);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		{

		}
		/* if secure session, temp session was created and handle is in command header */
		if (cmd->flags.secureSession)
			CV_VOLATILE_DATA->secureSession = (cv_session *)cmd->hSession;
		/* now call function */
		if ((retValCmd = cv_open((cv_options )*inputParams[0], (cv_app_id *)cvWord2Byte(inputParams[1]),
			(cv_user_id *)cvWord2Byte(inputParams[2]), (cv_obj_value_passphrase *)cvWord2HalfWord(inputParams[3]),
			(cv_handle *)outputParams[0])) != CV_SUCCESS)
		{
#ifdef DEBUG_CVMANAGER
            printf("%s: cv_open fail ret %d\n",__func__,retValCmd);
#endif
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		/* save the session handle in the command */
		cmd->hSession = *(outputParams[0]);

		break;
	case CV_CMD_CLOSE:
		/* now call function */
		if ((retValCmd = cv_close((cv_handle)*inputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_INIT:
		/* now call function */
		if ((retValCmd = cv_init((cv_handle)*inputParams[0], (cv_bool)*inputParams[1],
			(cv_bool)*inputParams[2], *inputParams[3], (cv_auth_list *)inputParams[4],
			(cv_obj_value_aes_key *)inputParams[5], *inputParams[6],
			(cv_auth_list *)inputParams[7], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_GET_STATUS:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		switch (*inputParams[1])
		{
		case CV_STATUS_VERSION:
			outputLengths[0] = sizeof(cv_version);
			break;
		case CV_STATUS_OP:
			outputLengths[0] = sizeof(cv_operating_status);
			break;
		case CV_STATUS_SPACE:
			outputLengths[0] = sizeof(cv_space);
			break;
		case CV_STATUS_TIME:
			outputLengths[0] = sizeof(cv_time);
			break;
		case CV_STATUS_FP_POLLING_INTERVAL:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_FP_PERSISTANCE:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_FP_HMAC_TYPE:
			outputLengths[0] = sizeof(cv_hash_type);
			break;
		case CV_STATUS_FP_FAR:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_DA_PARAMS:
			outputLengths[0] = sizeof(cv_da_params);
			break;
		case CV_STATUS_POLICY:
			outputLengths[0] = sizeof(cv_policy);
			break;
		case CV_STATUS_HAS_CV_ADMIN:
			outputLengths[0] = sizeof(cv_bool);
			break;
		case CV_STATUS_IDENTIFY_USER_TIMEOUT:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_ERROR_CODE_MORE_INFO:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_SUPER_CHALLENGE:
			outputLengths[0] = sizeof(cv_super_challenge);
			break;
		case CV_STATUS_IN_DA_LOCKOUT:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_DEVICES_ENABLED:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_ANTI_REPLAY_OVERRIDE_OCCURRED:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_RFID_PARAMS_ID:
			outputLengths[0] = sizeof(uint32_t);
			break;
		case CV_STATUS_ST_CHALLENGE:
			outputLengths[0] = sizeof(cv_st_challenge);
            break;
		case CV_STATUS_IS_BOUND:
			outputLengths[0] = sizeof(cv_bool);
            break;
		case CV_STATUS_HAS_SUPER_REGISTRATION:
			outputLengths[0] = sizeof(cv_bool);
            break;
		case CV_STATUS_POA_TEST_RESULT:
			outputLengths[0] = sizeof(cv_bool);
            break;
		case CV_STATUS_IS_POA_ENABLED:
			outputLengths[0] = sizeof(cv_bool);
            break;
		case CV_STATUS_CAPABILITIES:
			outputLengths[0] = sizeof(uint32_t);
            break;
		case 0xffff:
			break;
		}
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_get_status((cv_handle)*inputParams[0], (cv_status_type)*inputParams[1],
			outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_CONFIG:
		/* now call function */
		if ((retValCmd = cv_set_config((cv_handle)*inputParams[0], *inputParams[1],
			(cv_auth_list *)inputParams[2],
			(cv_config_type)*inputParams[3], inputParams[4],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SEND_BLOCKDATA:
		/* now call function */
		if ((retValCmd = cv_send_blockdata(*inputParams[0], (uint8_t *)inputParams[1],
			*inputParams[2])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_RECEIVE_BLOCKDATA:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[2];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* just save output checksum after output length, since it will be later copied into actual (FP) buffer */
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(uint32_t);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_receive_blockdata((cv_bool)*inputParams[0], *inputParams[1], &outputLengths[0], 
			(uint8_t *)outputParams[0],	(uint32_t *)outputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		/* handle as special case, just like FP capture, to accomodate for buffers up to 90K */
		captureOnlySpecialCase = TRUE;
		captureOnlyCmdPtr = FP_CAPTURE_LARGE_HEAP_PTR - sizeof(cv_encap_command) + sizeof(uint32_t) - sizeof(cv_param_list_entry) + sizeof(uint32_t);
		memcpy(captureOnlyCmdPtr, cmd, sizeof(cv_encap_command));
		outputParams[0] = (uint32_t *)FP_CAPTURE_LARGE_HEAP_PTR;
		/* need to put checksum after image */
		/* NOTE: FP capture image buffer must be sized large enough to have this encapsulation added to image */
		/* save checksum to add after buf */
		outputParams[2] = outputParams[1];
		outputParams[1] = (uint32_t *)(FP_CAPTURE_LARGE_HEAP_PTR + ALIGN_DWORD(outputLengths[0]) + sizeof(cv_param_list_entry) - sizeof(uint32_t));
		*outputParams[1] = *outputParams[2];
		cmd = (cv_encap_command *)captureOnlyCmdPtr;
		break;
	case CV_CMD_CREATE_OBJECT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_handle);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;

		/* now call function */
		if ((retValCmd = cv_create_object((cv_handle)*inputParams[0], (cv_obj_type)*inputParams[1],
			*inputParams[2], (cv_obj_attributes *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			*inputParams[6], inputParams[7], (cv_obj_handle *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_DELETE_OBJECT:
		/* now call function */
		if ((retValCmd = cv_delete_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_OBJECT:
		/* now call function */
		if ((retValCmd = cv_set_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			*inputParams[4], (cv_obj_attributes *)inputParams[5],
			*inputParams[6], inputParams[7],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_GET_OBJECT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_hdr);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[4];
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[1]));
		outputEncapTypes[2] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[2] = *inputParams[5];
		outputParams[2] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[2]));
		outputEncapTypes[3] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[3] = *inputParams[6];
		outputParams[3] = &outputParamEntry->param;
		numOutputParams++;

		/* Parameters pointers are initially set to requested lengths, pass pointers to the parameters */
		/* so they can be updated based on actual computed lengths */
		/* now call function */
		if ((retValCmd = cv_get_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_obj_hdr *)outputParams[0],
			&outputLengths[1], (cv_auth_list *)outputParams[1],
			&outputLengths[2], (cv_obj_attributes *)outputParams[2],
			&outputLengths[3], outputParams[3],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_ENUMERATE_OBJECTS:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[2];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_enumerate_objects((cv_handle)*inputParams[0], (cv_obj_type)*inputParams[1],
			&outputLengths[0], (cv_obj_handle *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_ENUMERATION_BUFFER_FULL, send return parameters, because */
			/* need to return the number of handles that fit in the buffer provided */
			if (retValCmd != CV_ENUMERATION_BUFFER_FULL)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_ENUMERATE_OBJECTS_DIRECT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[3];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_enumerate_objects_direct(*inputParams[0], (uint8_t *)inputParams[1], 
			(cv_obj_type)*inputParams[2], &outputLengths[0], (cv_obj_handle *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_ENUMERATION_BUFFER_FULL, send return parameters, because */
			/* need to return the number of handles that fit in the buffer provided */
			if (retValCmd != CV_ENUMERATION_BUFFER_FULL)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_DUPLICATE_OBJECT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_handle);
		//outputLengths[0] = *inputParams[4];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_duplicate_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_obj_handle *) outputParams[0], (cv_persistence)*inputParams[4],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_IMPORT_OBJECT:
		/* set up output params */
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_handle);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* check to see if bulk parameters pointer is null (check to see if whole structure is zero) */
		if (cvIsZero((uint8_t *)inputParams[9], sizeof(cv_bulk_params)))
			pBulkParameters = NULL;
		else
			pBulkParameters = (cv_bulk_params *)inputParams[9];
		/* now call function */
		if ((retValCmd = cv_import_object((cv_handle)*inputParams[0], (cv_blob_type)*inputParams[1],
			*inputParams[2], (cv_obj_attributes *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			*inputParams[6], inputParams[7],
			(cv_dec_op)*inputParams[8], pBulkParameters,
			*inputParams[10], inputParams[11],
			*inputParams[12], (cv_auth_list *)inputParams[13],
			(cv_obj_handle)*inputParams[14],
			*inputParams[15], (cv_obj_handle *)inputParams[16],
			(cv_obj_handle *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_EXPORT_OBJECT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[9];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* check to see if bulk parameters pointer is null (check to see if whole structure is zero) */
		if (cvIsZero((uint8_t *)inputParams[7], sizeof(cv_bulk_params)))
			pBulkParameters = NULL;
		else
			pBulkParameters = (cv_bulk_params *)inputParams[7];
		/* now call function */
		if ((retValCmd = cv_export_object((cv_handle)*inputParams[0], (cv_blob_type)*inputParams[1],
			(uint8_t *)inputParams[2], (cv_obj_handle)*inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			(cv_enc_op)*inputParams[6], pBulkParameters,
			(cv_obj_handle)*inputParams[8],
			&outputLengths[0], outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_CHANGE_AUTH_OBJECT:
		/* now call function */
		if ((retValCmd = cv_change_auth_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_AUTHORIZE_SESSION_OBJECT:
		if ((retValCmd = cv_deauthorize_session_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			CV_OBJ_AUTH_FLAGS_MASK)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		/* now call function */
		if ((retValCmd = cv_authorize_session_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_DEAUTHORIZE_SESSION_OBJECT:
		/* now call function */
		if ((retValCmd = cv_deauthorize_session_object((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			(cv_obj_auth_flags)*inputParams[2])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_GET_RANDOM:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[1];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_get_random((cv_handle)*inputParams[0], outputLengths[0],
			(byte *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_ENCRYPT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[8];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* check to see if bulk parameters pointer is null (check to see if whole structure is zero) */
		if (cvIsZero((uint8_t *)inputParams[7], sizeof(cv_bulk_params)))
			pBulkParameters = NULL;
		else
			pBulkParameters = (cv_bulk_params *)inputParams[7];
		/* now call function */
		if ((retValCmd = cv_encrypt((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_enc_op)*inputParams[4],
			*inputParams[5], (byte *)inputParams[6],
			pBulkParameters,
			&outputLengths[0], (byte *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_DECRYPT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[8];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* check to see if bulk parameters pointer is null (check to see if whole structure is zero) */
		if (cvIsZero((uint8_t *)inputParams[7], sizeof(cv_bulk_params)))
			pBulkParameters = NULL;
		else
			pBulkParameters = (cv_bulk_params *)inputParams[7];
		/* now call function */
		if ((retValCmd = cv_decrypt((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_dec_op)*inputParams[4],
			*inputParams[5], (byte *)inputParams[6],
			pBulkParameters,
			&outputLengths[0], (byte *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_HMAC:
		/* now call function */
		if ((retValCmd = cv_hmac((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_hmac_op)*inputParams[4],
			*inputParams[5], (byte *)inputParams[6],
			(cv_obj_handle)*inputParams[7], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_HASH:
		/* now call function */
		if ((retValCmd = cv_hash((cv_handle)*inputParams[0],
			(cv_hash_op)*inputParams[1],
			*inputParams[2], (byte *)inputParams[3],
			(cv_obj_handle)*inputParams[4])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_HASH_OBJECT:
		/* now call function */
		if ((retValCmd = cv_hash_object((cv_handle)*inputParams[0],
			(cv_hash_op)*inputParams[1],
			(cv_obj_handle)*inputParams[2], (cv_obj_handle)*inputParams[3])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_GENKEY:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_handle);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_genkey((cv_handle)*inputParams[0], (cv_obj_type)*inputParams[1],
			(cv_key_type)*inputParams[2], *inputParams[3],
			*inputParams[4], (cv_obj_attributes *)inputParams[5],
			*inputParams[6], (cv_auth_list *)inputParams[7],
			*inputParams[8], inputParams[9], (cv_obj_handle *)outputParams[0],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SIGN:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[6];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_sign((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_sign_op)*inputParams[4], (cv_obj_handle)*inputParams[5],
			&outputLengths[0], (byte *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_VERIFY:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_obj_handle);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_verify((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_verify_op)*inputParams[4], (cv_obj_handle)*inputParams[5],
			*inputParams[6], (byte *)inputParams[7], (cv_bool *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_HOTP_UNBLOCK:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(byte);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_hotp_unblock((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			(cv_hotp_unblock_auth *)inputParams[2],
			(byte *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_OTP_GET_VALUE:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[8];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_otp_get_value((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_time *)inputParams[4], *inputParams[5], (byte *)inputParams[6],
			(cv_obj_handle)*inputParams[7],
			&outputLengths[0], (byte *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_OTP_GET_MAC:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_otp_nonce);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_otp_get_mac((cv_handle)*inputParams[0], (cv_obj_handle)*inputParams[1],
			*inputParams[2], (cv_auth_list *)inputParams[3],
			(cv_otp_mac_type)*inputParams[4], (cv_otp_nonce *)inputParams[5],
			(cv_otp_nonce *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_WBF_GET_SESSION_ID:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_wbf_getsessionid);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_wbf_get_session_id((uint32_t)*inputParams[0],
			(cv_wbf_getsessionid *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_ENROLL:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[5];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_enroll((cv_handle)*inputParams[0],
			*inputParams[1], (cv_obj_attributes *)inputParams[2],
			*inputParams[3], (cv_auth_list *)inputParams[4],
			&outputLengths[0], (uint8_t *)outputParams[0],
			(cv_obj_handle *)outputParams[1],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_ENROLL_DUP_CHK:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[7];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_enroll_dup_check((cv_handle)*inputParams[0],
			*inputParams[1], (cv_obj_attributes *)inputParams[2],
			*inputParams[3], (cv_auth_list *)inputParams[4],
			*inputParams[5], (uint8_t *)inputParams[6],
			&outputLengths[0], (uint8_t *)outputParams[0],
			(cv_obj_handle *)outputParams[1],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd == CV_FP_DUPLICATE)
			{
				// if duplicate return duplicate template handle.
				printf("CV_CMD_FINGERPRINT_ENROLL_DUP_CHK duplicate fingerprint found 0x%x\n", *outputParams[1]);
			}
			else if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				printf("CV_CMD_FINGERPRINT_ENROLL_DUP_CHK error 0x%x\n", retValCmd);
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_VERIFY:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_fp_result);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_verify((cv_handle)*inputParams[0], (cv_fp_control)*inputParams[1],
			(int32_t)*inputParams[2], (uint32_t)*inputParams[3], (byte *)inputParams[4], (cv_obj_handle)*inputParams[5],
			(uint32_t)*inputParams[6], (byte *)inputParams[7], (cv_obj_handle)*inputParams[8],
			(cv_fp_result *)outputParams[0],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_VERIFY_W_HASH:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_fp_result);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[9];
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_verify_w_hash((cv_handle)*inputParams[0], (cv_fp_control)*inputParams[1],
			(int32_t)*inputParams[2], (uint32_t)*inputParams[3], (byte *)inputParams[4], (cv_obj_handle)*inputParams[5],
			(uint32_t)*inputParams[6], (byte *)inputParams[7], (cv_obj_handle)*inputParams[8],
			(cv_fp_result *)outputParams[0], outputLengths[1], (uint8_t *)outputParams[1],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_CAPTURE_WBF:
		/* Set polling period for wbf */
		CV_VOLATILE_DATA->fp_polling_period = FP_POLLING_PERIOD_WBF;
		/*Intentionally missing Break. So ignore Coverity error*/
	case CV_CMD_FINGERPRINT_CAPTURE:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[6];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_capture((cv_handle)*inputParams[0], (cv_bool)*inputParams[1],
			*inputParams[2], (cv_obj_attributes *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			&outputLengths[0], (uint8_t *)outputParams[0],
			(cv_obj_handle *)outputParams[1],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* Set fp polling period back to default */
			CV_VOLATILE_DATA->fp_polling_period = FP_POLLING_PERIOD;

			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			} else
				/* don't need to do captureOnly processing below, because length error occurred */
				break;
		}

		/* Set fp polling period back to default */
		CV_VOLATILE_DATA->fp_polling_period = FP_POLLING_PERIOD;

		/* need special handling of captureOnly case, since image could be up to 90K.  the image is */
		/* in the buffer pointed to by FP_CAPTURE_LARGE_HEAP_PTR.  move the cmd to the FP I/O command */
		/* buf immediately before the image and let encapsulate operate on that buffer */
		if ((cv_bool)*inputParams[1] == TRUE)
		{
			captureOnlySpecialCase = TRUE;
			captureOnlyCmdPtr = FP_CAPTURE_LARGE_HEAP_PTR - sizeof(cv_encap_command) + sizeof(uint32_t) - sizeof(cv_param_list_entry) + sizeof(uint32_t);
			memcpy(captureOnlyCmdPtr, cmd, sizeof(cv_encap_command));
			outputParams[0] = (uint32_t *)FP_CAPTURE_LARGE_HEAP_PTR;
			/* since this is capture only, handle will be null.  put encap handle after image */
			/* NOTE: FP capture image buffer must be sized large enough to have this encapsulation added to image */
			outputParams[1] = (uint32_t *)(FP_CAPTURE_LARGE_HEAP_PTR + ALIGN_DWORD(outputLengths[0]) + sizeof(cv_param_list_entry) - sizeof(uint32_t));
			*outputParams[1] = 0;
			cmd = (cv_encap_command *)captureOnlyCmdPtr;
		}
		break;
	case CV_CMD_FINGERPRINT_IDENTIFY:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_fp_result);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_identify((cv_handle)*inputParams[0], (cv_fp_control)*inputParams[1],
			(int32_t)*inputParams[2], (cv_obj_handle)*inputParams[3], (uint32_t)*inputParams[4], (cv_obj_handle *)inputParams[5],
			(cv_fp_result *)outputParams[0], (cv_obj_handle *)outputParams[1],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_IDENTIFY_W_HASH:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_fp_result);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[1]));
		outputEncapTypes[2] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[2] = *inputParams[6];
		outputParams[2] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_identify_w_hash((cv_handle)*inputParams[0], (cv_fp_control)*inputParams[1],
			(int32_t)*inputParams[2], (cv_obj_handle)*inputParams[3], (uint32_t)*inputParams[4], (cv_obj_handle *)inputParams[5],
			(cv_fp_result *)outputParams[0], (cv_obj_handle *)outputParams[1], outputLengths[2], (uint8_t *)outputParams[2],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_CONFIGURE:
		/* set up output params if this is a read */
		if (numInputParams == 7)  /* if read, pData parameter not provided on input */
		{
			outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
			outputParams[0] = &outputParamEntry->param;
			numOutputParams++;
		} else
			/* this is a write, put pointer to data in outputParams, so same call can be used below for input or output */
			outputParams[0] = inputParams[6];

		/* length needed whether input or output */
		outputLengths[0] = *inputParams[5];
		/* now call function */
		if ((retValCmd = cv_fingerprint_configure((cv_handle)*inputParams[0],
			*inputParams[1], (cv_auth_list *)inputParams[2],
			(cv_fp_config_data_type)*inputParams[3], *inputParams[4],
			&outputLengths[0], (uint8_t *)outputParams[0],
			*inputParams[numInputParams - 1],  /* this is the last parameter, but index varies based on whether input or output */
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_TEST:
#if 0
    // USH_QSPI_RAMFUNC_AREA 0x24019200 3*1024
    printf("STOP THE ICE!!!\n");
    long long l = 0x1000000000;
    while (l--) ;
    qspi_init();

    printf("Performing Bills one-off flash xip test flash_dev=%d\n",VOLATILE_MEM_PTR->flash_device);
    PHX_STATUS (*init_func)(void *) = 0x24019209; 
    PHX_STATUS (*read_func)(uint32_t address,uint8_t *data, uint32_t data_len, uint32_t flash_device) = 0x24019205; 
    PHX_STATUS (*write_func)(uint32_t address,uint8_t *data, uint32_t data_len, uint32_t flash_device) = 0x24019201; 

    uint8_t nuke[512];
    uint8_t duke[512];
    int i;
    printf("Init\n");
    init_func(VOLATILE_MEM_PTR);

    for (i=0;i<100;i++)
    {
        memset(nuke,0xa5+i,sizeof(nuke));
        printf("W");
        write_func(0x03fffc20, nuke, sizeof(nuke), VOLATILE_MEM_PTR->flash_device);
        printf("R");
        read_func(0x03fffc20, duke, sizeof(nuke), VOLATILE_MEM_PTR->flash_device);
        if (memcmp(nuke,duke,sizeof(nuke))) {
            printf("memcmp failed on iteration %d nuke %02x duke %02x",i,nuke[0],duke[0]);
        } else {
            printf(".");
        }
    }
#else
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_test((cv_handle)*inputParams[0], (uint32_t)*inputParams[1],
			(uint32_t *)outputParams[0],
			(uint32_t)*inputParams[2], (byte *)inputParams[3])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
#endif
		break;
	case CV_CMD_FINGERPRINT_TEST_OUT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));

		/* Fingerprint test API modified to support Validity need for ouptputting data if test fails,
		   extraDataLength and extraData are now outputs from CV 
		 */
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[2];
		outputParams[1] = &outputParamEntry->param;
				numOutputParams++;

		/* now call function */
		retValCmd = cv_fingerprint_test((cv_handle)*inputParams[0], (uint32_t)*inputParams[1],
			(uint32_t *)outputParams[0], 
			outputLengths[1], (byte *)outputParams[1]);

		retValCmd = CV_SUCCESS;			/* Force SUCCESS code to return extraInfo Data */
		break;
	case CV_CMD_FINGERPRINT_CALIBRATE:
		/* now call function */
		if ((retValCmd = cv_fingerprint_calibrate((cv_handle)*inputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_CAPTURE_START:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_nonce);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;

		/* first check to see if internal flag set in cvFpCapControl.  if so, fail command */
		if (*inputParams[2] & CV_CAP_CONTROL_INTERNAL)
		{
			numOutputParams = 0;
			retValCmd = CV_INVALID_INPUT_PARAMETER;
			goto cmd_err;
		}
		/* now call function */
		if ((retValCmd = cv_fingerprint_capture_start((cv_handle)*inputParams[0], *inputParams[1],
			(cv_fp_cap_control)*inputParams[2],
			(cv_nonce *)outputParams[0],
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
        /* Block the sleep mode, fp sensor is polling the fp touch event  */
        if( !fpSensorSupportInterrupt() ) lynx_config_sleep_mode(0);
		break;
    case CV_CMD_HOST_ENTER_CS_STATE:
        /* set up output params */
        outputEncapTypes[0] = CV_ENCAP_STRUC;
        outputLengths[0] = 4;
        outputParams[0] = &outputParamEntry->param;
        numOutputParams++;
        *outputParams[0] = fpSensorSupportInterrupt();

        /* Allow the sleep mode and check the capture operation */
        if( !fpSensorSupportInterrupt() ) {
            cv_fingerprint_capture_cancel();
            lynx_config_sleep_mode(1);
        }
        break;
    case CV_CMD_HOST_EXIT_CS_STATE:
        /* Block the sleep mode, and restart the capture */
        if( !fpSensorSupportInterrupt() ) {
            cv_fingerprint_capture_start( NULL, 0x0, 
                            CV_CAP_CONTROL_DO_FE | CV_CAP_CONTROL_OVERRIDE_PERSISTENCE | CV_CAP_CONTROL_TERMINATE | CV_CAP_CONTROL_NOSESSION_VALIDATION | CV_CAP_CONTROL_RESTART_CAPTURE, 
                            NULL, NULL, 0);
            lynx_config_sleep_mode(0);
        }
        break;
	case CV_CMD_FINGERPRINT_CAPTURE_CANCEL:
        /* Allow the sleep mode and cancel the capture operation */
        if( !fpSensorSupportInterrupt() ) lynx_config_sleep_mode(1);
		/* now call function */
		if ((retValCmd = cv_fingerprint_capture_cancel()) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[3];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_capture_get_result((cv_handle)*inputParams[0], (cv_bool)*inputParams[1],
			(cv_nonce*)inputParams[2], 
			&outputLengths[0], (uint8_t *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
					
				numOutputParams = 0;
				goto cmd_err;
			} else
				/* don't need to do captureOnly processing below, because length error occurred */
				break;
		}
	
		/* need special handling of captureOnly case, since image could be up to 90K.  the image is */
		/* in the buffer pointed to by FP_CAPTURE_LARGE_HEAP_PTR.  move the cmd to the FP I/O command */
		/* buf immediately before the image and let encapsulate operate on that buffer */
		if ((cv_bool)*inputParams[1] == TRUE)
		{
			captureOnlySpecialCase = TRUE;
			captureOnlyCmdPtr = FP_CAPTURE_LARGE_HEAP_PTR - sizeof(cv_encap_command) + sizeof(uint32_t) - sizeof(cv_param_list_entry) + sizeof(uint32_t);
			memcpy(captureOnlyCmdPtr, cmd, sizeof(cv_encap_command));
			outputParams[0] = (uint32_t *)FP_CAPTURE_LARGE_HEAP_PTR;
			cmd = (cv_encap_command *)captureOnlyCmdPtr;
		}
		break;
	case CV_CMD_FINGERPRINT_CREATE_FEATURE_SET:
		/* now call function */
		if ((retValCmd = cv_fingerprint_create_feature_set((cv_handle)*inputParams[0], (cv_fp_fe_control)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_COMMIT_FEATURE_SET:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_commit_feature_set((cv_handle)*inputParams[0], (cv_nonce *)inputParams[1],
			*inputParams[2], (cv_obj_attributes *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			(cv_obj_handle *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
					
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_UPDATE_ENROLLMENT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_nonce);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[1]));
		// pDupTemplateHandle
		outputEncapTypes[2] = CV_ENCAP_STRUC;
		outputLengths[2] = sizeof(cv_obj_handle);
		outputParams[2] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_update_enrollment((cv_handle)*inputParams[0], (cv_nonce *)inputParams[1],
			*inputParams[2], (uint8_t*)inputParams[3],
			(cv_bool *)outputParams[0], (cv_nonce *)outputParams[1], (cv_obj_handle *)outputParams[2] )) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd == CV_FP_DUPLICATE)
			{
				// if duplicate return duplicate template handle.
				printf("CV_CMD_FINGERPRINT_UPDATE_ENROLLMENT duplicate fingerprint found 0x%x\n", *outputParams[2]);
			}
			else if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
					
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_DISCARD_ENROLLMENT:
		/* now call function */
		if ((retValCmd = cv_fingerprint_discard_enrollment()) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_FINGERPRINT_COMMIT_ENROLLMENT:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[6];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_STRUC;
		outputLengths[1] = sizeof(cv_obj_handle);
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_commit_enrollment((cv_handle)*inputParams[0], (cv_nonce *)inputParams[1],
			*inputParams[2], (cv_obj_attributes *)inputParams[3],
			*inputParams[4], (cv_auth_list *)inputParams[5],
			&outputLengths[0], (uint8_t *)outputParams[0], (cv_obj_handle *)outputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
					
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_FINGERPRINT_CREATE_TEMPLATE:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[9];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_fingerprint_create_template((cv_handle)*inputParams[0], 
			*inputParams[1], (uint8_t *)inputParams[2],
			*inputParams[3], (uint8_t *)inputParams[4],
			*inputParams[5], (uint8_t *)inputParams[6],
			*inputParams[7], (uint8_t *)inputParams[8],
			&outputLengths[0], (uint8_t *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
					
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_BIND:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		//outputLengths[0] = sizeof(cv_obj_value_hash);
		outputLengths[0] = SHA1_LEN + sizeof(cv_obj_value_hash) - 1;	/* SHA1 type hash computation used for bind */
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_bind((cv_handle)*inputParams[0], (cv_obj_value_hash *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_BIND_CHALLENGE:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		//outputLengths[0] = sizeof(cv_obj_value_hash);
		outputLengths[0] = SHA1_LEN + sizeof(cv_obj_value_hash) - 1;	/* SHA1 type hash computation used for bind */
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_bind_challenge((cv_handle)*inputParams[0], (cv_obj_value_hash *)inputParams[1],
			(cv_obj_value_hash *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_IDENTIFY_USER:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_identify_type);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[5];
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_identify_user((cv_handle)*inputParams[0], (uint32_t)*inputParams[1], (cv_auth_list *)inputParams[2],
			(uint32_t)*inputParams[3], (cv_obj_handle *)inputParams[4], (cv_identify_type *)outputParams[0],
			&outputLengths[1], (uint8_t *)outputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_ACCESS_PBA_CODE_FLASH:
		/* set up output params if this is a read */
		if (numInputParams == 6)  /* if read, pData parameter not provided on input */
		{
			outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
			outputParams[0] = &outputParamEntry->param;
			numOutputParams++;
		} else
			/* this is a write, put pointer to data in outputParams, so same call can be used below for input or output */
			outputParams[0] = inputParams[5];



		/* length needed whether input or output */
		outputLengths[0] = *inputParams[4];
		/* now call function */
		if ((retValCmd = cv_access_PBA_code_flash((cv_handle)*inputParams[0],
			*inputParams[1], (cv_auth_list *)inputParams[2],
			*inputParams[3],
			&outputLengths[0], (uint8_t *)outputParams[0],
			*inputParams[numInputParams - 1],  /* this is the last parameter, but index varies based on whether input or output */
			cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_ENABLE_RADIO:
		/* now call function */
		if ((retValCmd = cv_enable_radio((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_ENABLE_AND_LOCK_RADIO:
		/* now call function */
		if ((retValCmd = cv_enable_and_lock_radio((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_RADIO_PRESENT:
		/* now call function */
		if ((retValCmd = cv_set_radio_present((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_SMARTCARD_PRESENT:
		/* now call function */
		if ((retValCmd = cv_set_smartcard_present((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_MANAGE_POA:
		/* now call function */
		if ((retValCmd = cv_manage_poa((cv_handle)*inputParams[0], (cv_bool)*inputParams[1], (uint32_t)*inputParams[2], (uint32_t)*inputParams[3], TRUE)) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_NFC_PRESENT:
		/* now call function */
		if ((retValCmd = cv_set_nfc_present((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_CV_ONLY_RADIO_MODE_VOL:
		/* for 5880 treat volatile like persistent, because 5880 loses power during S3 */
		goto cmd_err;

	case CV_CMD_SET_CV_ONLY_RADIO_MODE:
		/* now call function */
		if ((retValCmd = cv_set_cv_only_radio_mode((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_RFID_PARAMS:
		/* now call function */
		if ((retValCmd = cv_set_rfid_params((cv_handle)*inputParams[0], (uint32_t)*inputParams[1],
			*inputParams[2], (byte *)inputParams[3], (uint32_t)*inputParams[4])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_ENABLE_SMARTCARD:
		/* now call function */
		if ((retValCmd = cv_enable_smartcard((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_ENABLE_NFC:
		/* now call function */
		if ((retValCmd = cv_enable_nfc((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_SET_T4_CARDS_ROUTING:
		/* now call function */
		if ((retValCmd = cv_set_t4_cards_routing((cv_handle)*inputParams[0], (uint32_t)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;		
	case CV_CMD_ENABLE_FINGERPRINT:
		/* now call function */
		if ((retValCmd = cv_enable_fingerprint((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_READ_CONTACTLESS_CARD_ID:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_contactless_type);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[3];
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_read_contactless_card_id((cv_handle)*inputParams[0], (uint32_t)*inputParams[1], (cv_auth_list *)inputParams[2],
			(cv_identify_type *)outputParams[0], &outputLengths[1], (uint8_t *)outputParams[1], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_READ_NFC_CARD_UUID:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		outputParamEntry = (cv_param_list_entry *)((uint8_t *)(outputParamEntry + 1) - sizeof(uint32_t)
			+ ALIGN_DWORD(outputLengths[0]));
		outputEncapTypes[1] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[1] = *inputParams[3];
		outputParams[1] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_read_nfc_card_uuid((cv_handle)*inputParams[0], (uint32_t)*inputParams[1], (cv_auth_list *)inputParams[2],
			(cv_identify_type *)outputParams[0], &outputLengths[1], (uint8_t *)outputParams[1], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_REGISTER_SUPER:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_super_registration_params);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		/* now call function */
		if ((retValCmd = cv_register_super((cv_handle)*inputParams[0], (uint32_t)*inputParams[1], (cv_auth_list *)inputParams[2],
			(uint32_t)*inputParams[3], (uint8_t *)inputParams[4], (cv_super_registration_params *)outputParams[0], cvCallback, &ctx)) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_CHALLENGE_SUPER:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_STRUC;
		outputLengths[0] = sizeof(cv_super_challenge);
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		inputParams_0 = *inputParams[0];
		/* now call function */
		if ((retValCmd = cv_challenge_super((cv_handle)inputParams_0, (cv_super_challenge *)outputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_RESPONSE_SUPER:
		numOutputParams++;
		inputParams_0 = *inputParams[0];
		/* now call function */
		if ((retValCmd = cv_response_super((cv_handle)inputParams_0, (uint8_t *)inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error.  if CV_INVALID_OUTPUT_PARAMETER_LENGTH, send return parameters, because */
			/* need to return expected length */
			if (retValCmd != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
		}
		break;
	case CV_CMD_NOTIFY_SYSTEM_POWER_STATE:
		/* now call function */
		if ((retValCmd = cv_notify_system_power_state((cv_system_power_state)*inputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_SECURE_SESSION_GET_PUBKEY:
	case CV_SECURE_SESSION_HOST_NONCE:
		/* set up temporary secure session pointer from command header (for CV_SECURE_SESSION_HOST_NONCE) */
		CV_VOLATILE_DATA->secureSession = (cv_session *)cmd->hSession;
		cvSetupSecureSession(cmd);
		/* put temporary secure session pointer in command header result.  cv user lib will return it in next command */
		cmd->hSession = (cv_handle)CV_VOLATILE_DATA->secureSession;
		/* routine has already encapsulated result */
		encapsulateResult = FALSE;
		/* update saved flags to indicate result type */
		sav_encap_flags.returnType = CV_RET_TYPE_RESULT;
		break;


	case CV_CMD_DIAG_TEST:
		outputParams[0] = &outputParamEntry->param;
		outputEncapTypes[0] = CV_ENCAP_LENVAL_STRUC;
		numOutputParams = 1;
#ifdef HID_CNSL_REDIRECT
		if(*(inputParams[0]) == CV_DIAG_CMD_GET_CONSOLE) {
			outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		}
#endif
#ifdef PHX_REQ_HDOTP_DIAGS
		if((*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_SKIP_LIST) ||
		   (*(inputParams[0]) == CV_DIAG_CMD_HDOTP_SET_SKIP_CNTR)) {
			outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		}
		else if((*(inputParams[0]) == CV_DIAG_CMD_HDOTP_SET_SKIP_LIST) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_SET_STATUS) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_POKE)) {
			numOutputParams = 0;
		}
		else if((*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_STATUS) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_PEEK) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_CV_CNTR) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_AH_CRDT_LVL) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_ELPSD_TIME) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_SKIP_MONO_CNTR) ||
				(*(inputParams[0]) == CV_DIAG_CMD_HDOTP_GET_SPEC_SKIP_CNTR)) {
			outputEncapTypes[0] = CV_ENCAP_STRUC;
		}
#endif
#ifdef PHX_REQ_SCI
		if(*(inputParams[0]) == CV_DIAG_CMD_SC_DEACTIVATE) {
			numOutputParams = 0;
		}
#endif

		retValCmd = ush_diag(*(inputParams[0]), *(inputParams[1]), numInputParams - 2, &(inputParams[2]), outputParams[0], &outputLengths[0]);
		if (retValCmd != CV_SUCCESS)
		{
			numOutputParams = 0;
			goto cmd_err;
		}


		break;
	case CV_CMD_UPGRADE_OBJECTS:
		retValCmd = cv_upgrade_objects((cv_handle)*inputParams[0], (uint32_t)*inputParams[1], (cv_obj_handle *) inputParams[2]);
		numOutputParams = 0;
		break;
#if 0
	case CV_CMD_PARTNER_DISABLE_PROG:
		{
		uint32_t otp_bits;
		uint32_t flash_bits;
		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}
		otp_bits = (*(inputParams[0]+1) & CUSTID_PARTNER_DISABLE_BIT_MASK); // -- should we mask off the partner bits
		flash_bits = (*(inputParams[0]+2) & CUSTID_PARTNER_DISABLE_BIT_MASK); // -- should we mask off the partner bits
		if (flash_bits) {
			extern PHX_STATUS read_enable_partner_flags(unsigned int *flags);
			extern PHX_STATUS save_enable_partner_flags(unsigned int flags);
			unsigned int cur_partner_flags ;

			/* Call the appropriate vendor */


			/* save to flash */
			read_enable_partner_flags(&cur_partner_flags);
			cur_partner_flags &= ~flash_bits;
			save_enable_partner_flags(cur_partner_flags);
		}

		if (otp_bits) {
			uint32_t custID; uint8_t custRev;
			retValCmd=nvm_OEMFields(&custID, &custRev);
			if (retValCmd != PHX_STATUS_OK) {
				break;
			}
        		custID |= (*(inputParams[0]+1) & CUSTID_PARTNER_DISABLE_BIT_MASK); // -- should we mask off the partner bits
			retValCmd=nvm_progOEMFields(custID, custRev);
			if (retValCmd != PHX_STATUS_OK) {
				break;
			}
			VOLATILE_MEM_PTR->nCustomerID= custID;
			VOLATILE_MEM_PTR->nCustomerRev= (uint32_t )custRev;
			VOLATILE_MEM_PTR->enable_partner_flags &= ~(VOLATILE_MEM_PTR->nCustomerID & CUSTID_PARTNER_DISABLE_BIT_MASK);
		}
		/* required reboot after this */
		break;
		}
	case CV_CMD_PARTNER_ENABLE_PROG:
		{
		uint32_t custID; uint8_t custRev;
		uint32_t enable_bits;
		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}
		enable_bits = (*(inputParams[0]+1) & CUSTID_PARTNER_DISABLE_BIT_MASK); // -- should we mask off the partner bits
		retValCmd=nvm_OEMFields(&custID, &custRev);
		if (retValCmd != PHX_STATUS_OK) {
			break;
		}
		/* cannot enable an otp disabled feature */
		if (enable_bits & custID) {
			retValCmd = CV_INVALID_INPUT_PARAMETER;
			break;
		}
		if (enable_bits) {
			extern PHX_STATUS read_enable_partner_flags(unsigned int *flags);
			extern PHX_STATUS save_enable_partner_flags(unsigned int flags);

			unsigned int cur_partner_flags ;

			/* Call the appropriate vendor */


			/* save to flash */
			read_enable_partner_flags(&cur_partner_flags);
			cur_partner_flags |= enable_bits;
			save_enable_partner_flags(cur_partner_flags);
		}
		/* required reboot after this */
		break;
		}
	case CV_CMD_OTP_PROG:
		{
		uint16_t BrcmRev; uint32_t Dcfg; uint8_t Scfg;
		extern PHX_STATUS otp_program_MfgKeys(uint16_t BrcmRev, uint32_t Dcfg, uint8_t  Scfg) ;
		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

        	BrcmRev = *((uint16_t *)(inputParams[0]+1));
        	Scfg = *((uint8_t*)(inputParams[0]+1)+2);
        	Dcfg = *(inputParams[0]+2);

		retValCmd = otp_program_MfgKeys(BrcmRev, Dcfg, Scfg);

		break;
		}
	case CV_CMD_CUSTID_PROG:
		{
		uint32_t custID; uint8_t custRev;
		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}
        	custID = *(inputParams[0]+1) /*& CUSTID_MASK */; // -- should we mask off the partner bits
        	custRev = *((uint8_t *)(inputParams[0]+2));
		retValCmd=nvm_progOEMFields(custID, custRev);
		break;
		}
#endif /* if 0 */
	case CV_CMD_START_CONSOLE_LOG:
		/* not allowed if FP image buf not available.  s/b ok, because just used for diags */
		if (!isFPImageBufAvail())
			retValCmd = CV_FP_DEVICE_BUSY;
		else
			start_console_log();
		break;
	case CV_CMD_STOP_CONSOLE_LOG:
		stop_console_log();
		break;
	case CV_CMD_GET_CONSOLE_LOG:
		outputParams[0] = &outputParamEntry->param;
		outputEncapTypes[0] = CV_ENCAP_LENVAL_STRUC;
		numOutputParams = 1;
		outputParams[0] = &outputParamEntry->param;
		outputLengths[0]= MAX_CV_COMMAND_LENGTH;

		outputLengths[0]=copy_console_log(outputParams[0], outputLengths[0]);

		outputLengths[0] = ALIGN_DWORD(outputLengths[0]);
		/* Removed the below dead code to fix coverity issue */
		/*
		if (retValCmd != CV_SUCCESS)
		{
			numOutputParams = 0;
			goto cmd_err;
		}
		*/
		break;
	case CV_CMD_PUTCHARS:
		copy_inp_to_console_inbuf((inputParams[0]+1),*(inputParams[0]));
		break;
	case CV_CMD_CONSOLE_MENU:
		do_menu_cmds=*inputParams[0];	/* could be implemented as subcommand */
		printf("CV: do_menu_cmds %d\n", do_menu_cmds);
		break;
	case CV_CMD_PWR:
		if ((*inputParams[0] < PHX_PWR_USB_LPC_WAKEUP ) || (*inputParams[0] > PHX_PWR_MOD_MAX)) {
			retValCmd = CV_INVALID_INPUT_PARAMETER;
			printf("CV: pwr wakeup modes not supported. %u < (input) %u > %u \n",  PHX_PWR_USB_LPC_WAKEUP , *inputParams[0], PHX_PWR_MOD_MAX );
			break;
		}
		if ((*inputParams[0] >= PHX_PWR_USB_LPC_WAKEUP)) {
			if (isNotCvOnlyRadioMode()) {
				retValCmd = CV_FEATURE_NOT_SUPPORTED;
				printf("CV: usb pwr wakeup modes not supported if CCID-CL is enabled.\n");
				break;
			}
		}

		do_pwr_cmds=*inputParams[0];	
		VOLATILE_MEM_PTR->pwr_mode=do_pwr_cmds;	
		printf("CV: do_pwr_cmds %d\n", do_pwr_cmds);
		break;
	case CV_CMD_PWR_START_IDLE_TIME:
		if (*inputParams[0] != PWR_SKIP_AUTO_PWRSAVE_MS) {
			if (*inputParams[0] < PWR_MIN_ENTER_TIME_MS) {
				retValCmd = CV_INVALID_INPUT_PARAMETER;
				printf("CV: usb pwr start idle %u < %u.\n", *inputParams[0], PWR_MIN_ENTER_TIME_MS);
				break;
			}
		}
		retValCmd = cv_pwrSaveStart_conf(*inputParams[0]);
		break;
	case CV_CMD_FINGERPRINT_RESET:
		{
		/* check to make sure FP device is enabled */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
			retValCmd = CV_FP_NOT_ENABLED;
			goto cmd_err;
		}
	
		/* check to make sure FP device present */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
			retValCmd = CV_FP_NOT_PRESENT;
			goto cmd_err;
		}
		
		/* Call fingerprint sensor reset function */
		retValCmd = fpSensorReset();

		/* set flag so that first async FP capture will check to see if calibration necessary */
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CHECK_CALIBRATION;
		}
		break;
	case CV_CMD_SET_NFC_POWER_TO_MAX:
		printf("CV_CMD_SET_NFC_POWER_TO_MAX\n");
#if 0		
        if (nci_spi_int_state() == 0) {
            uint8_t rsp[255];
            uint16_t rlen = sizeof(rsp);
	        printf("%s: UNEXPECTED NOTIFICATION!\n",__func__);
            nci_notification(rsp, &rlen);
        }
        nci_reset(0);
        nci_init(0);
        if (nci_set_maxpowerlevel(1)) {
		    retValCmd = CV_FAILURE;
        } else {
		    retValCmd = CV_SUCCESS;
        }
#endif
		retValCmd = cv_set_nfc_max_power();
		break;
	case CV_CMD_BIOMETRIC_UNIT_CREATED:
		printf("WBF BU created\n");
		if (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_NEXT_TOUCH)
		{
            fpNextEventHandler(eNE_BiometricUnitCreated);
		}
		retValCmd = CV_SUCCESS;
		break;
	case CV_CMD_ENROLLMENT_STARTED:
		printf("Enrollment started\n");
		CV_VOLATILE_DATA->doingFingerprintEnrollmentNow = 1;
		retValCmd = CV_SUCCESS;
		break;
	case CV_CMD_FUNCTION_ENABLE:
		/* now call function */
		if ((retValCmd = cv_enable_function((cv_function_enables)*inputParams[0])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
		
	case CV_CMD_SET_DEBUG_LOGGING:
		if ((retValCmd = cv_logging_enable_set((cv_handle)*inputParams[0], (cv_bool)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		retValCmd = CV_SUCCESS;
		break;
		
	case CV_CMD_GET_DEBUG_LOGGING:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = sizeof(uint32_t);
		outputParams[0] = &outputParamEntry->param;
		memcpy(outputParams[0],&(CV_PERSISTENT_DATA->loggingEnables),sizeof(uint32_t)); 
		numOutputParams++;
		retValCmd = CV_SUCCESS;
		break;

	case CV_CMD_SET_EC_PRESENCE:
		/* now call function */
		if ((retValCmd = cv_set_EC_presence((cv_handle)*inputParams[0], (uint32_t)*inputParams[1])) != CV_SUCCESS)
		{
			/* abort and send error */
			numOutputParams = 0;
			goto cmd_err;
		}
		break;
	case CV_CMD_ERASE_FLASH_CHIP:
		{
			PHX_STATUS ROMFUNC_phx_qspi_chip_erase(int flash_device);
			printf("Will Erase the whole flash, don't power off the system!!!\nAfter the erase completion, Lynx will reset and enter the SBL mode.\n");
			ROMFUNC_phx_qspi_chip_erase( CUR_USH_FLASH_DEVICE );
			retValCmd = CV_SUCCESS;
			break;
		}
	case CV_CMD_CANCEL_POA_SSO:
		{
			if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
			{
				CV_VOLATILE_DATA->poaAuthSuccessFlag = CV_POA_AUTH_NOT_DONE;
				CV_VOLATILE_DATA->poaAuthSuccessTemplate = 0;
				printf("POA SSO: Canceling \n");
			}
			retValCmd = CV_SUCCESS;
		}
		break;
	/******************************************************/
	/* Add new cv cmds for non-sbi images above this line */
	/******************************************************/
#endif /* USH_BOOTROM */

	case CV_CMD_GENERATE_ECDSA_KEY:
			{
				uint32_t	KecDSA_priv[OTP_SIZE_KECDSA_PRIV/sizeof(uint32_t)];
				uint16_t    key_size = OTP_SIZE_KECDSA_PRIV;
				if( readKeys(KecDSA_priv,&key_size,OTPKey_ecDSA)!=IPROC_OTP_VALID)
				{
					uint32_t    outputRng[SHA1_HASH_SIZE*2];
					OTP_STATUS  otp_status;

					printf("ECDSA Keys are not populated \n");
					memset(KecDSA_priv,0,sizeof(KecDSA_priv));
					memset(outputRng,0,sizeof(outputRng));
					if(bcm_generate_ecdsa_priv_key(outputRng) == BCM_SCAPI_STATUS_OK)
					{
					    memcpy(KecDSA_priv, outputRng, OTP_SIZE_KECDSA_PRIV);
						otp_status = setKeys((uint8_t*)KecDSA_priv, (uint16_t)sizeof(KecDSA_priv), OTPKey_ecDSA);
						if(otp_status != IPROC_OTP_VALID) 
							goto cmd_err;
						else
						{
						   printf("ECDSA Keys write success \n");
						}
					}
					else
					{
						printf("ECDSA Keys failure to gen priv key \n");
						goto cmd_err;
					}
				}
				else
				{
					printf("ECDSA Keys are already populated \n");
				}
		    }
			retValCmd = CV_SUCCESS;
			break;

	case CV_CMD_GET_MACHINE_ID:
		{
			uint8_t challenge_buff[SHA256_HASH_SIZE+1];
			// printf("Input blob - %d \n",*inputParams[0]);
			// dumpHex((uint8_t *)inputParams[1], *inputParams[0]);
			memset(challenge_buff,0,sizeof(challenge_buff));
			if(*inputParams[0] <= sizeof(challenge_buff))
				memcpy(challenge_buff,(uint8_t*)inputParams[1],*inputParams[0]);
			else
				memcpy(challenge_buff,(uint8_t*)inputParams[1],sizeof(challenge_buff));
			if ((retValCmd = ush_getMachineID(challenge_buff, MEM_OTP, &outputParamEntry->param)) != CV_SUCCESS) {
				goto cmd_err;
			}
			/* set up output params */
			outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
			outputLengths[0] = SHA256_HASH_SIZE;
			outputParams[0] = &outputParamEntry->param;
			numOutputParams++;
		}
		break;
	case CV_CMD_REBOOT_TO_SBI:
		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			printf("CV: TPM upgrade activated %d\n", isTPMUpgradeActivated());
			goto cmd_err;
		}
		reboot_cmd = SBI_CMD_REBOOT_USH_TO_SBI;
		break;
	case CV_CMD_REBOOT_USH:
		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			printf("CV: TPM upgrade activated %d\n", isTPMUpgradeActivated());
			goto cmd_err;
		}
		reboot_cmd = SBI_CMD_REBOOT_USH_TO_USH | SBI_CMD_REBOOT_USH_SKIP_USB;
		break;
	case CV_CMD_USH_VERSIONS:
		outputParams[0] = &outputParamEntry->param;
		outputEncapTypes[0] = CV_ENCAP_LENVAL_STRUC;
		numOutputParams = 1;
		outputParams[0] = &outputParamEntry->param;
		outputLengths[0]= MAX_CV_COMMAND_LENGTH;
		retValCmd = ush_bootrom_versions(numInputParams - 2, (inputParams[2]), &outputLengths[0], outputParams[0]);
		outputLengths[0] = ALIGN_DWORD(outputLengths[0]);
		if (retValCmd != CV_SUCCESS)
		{
			numOutputParams = 0;
			goto cmd_err;
		}
#ifdef USH_BOOTROM /*AAI */
        /* should restart fp capture after s3 resume */
        if( lynx_get_fp_sensor_state() )
        {
            lynx_save_fp_sensor_state();  // clear the sensor state in M0_SRAM
            queue_cmd(SCHED_FP_START, QUEUE_FROM_TASK, NULL);
        }
#endif
		break;
	case CV_CMD_FLASH_PROGRAMMING:
		{
		uint32_t initialOffset;
		uint32_t totalBytes;
		uint32_t type, size;

		if (checkIfAntiHammeringTriggered(ANTIHAM_FLSHPRG)) {
			retValCmd = CV_ANTIHAMMERING_PROTECTION_FLSHPRG;
			goto cmd_err;
		}

		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			goto cmd_err;
		}

		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

        	totalBytes = *(inputParams[0]+1);
        	initialOffset = *(inputParams[0]+2);
#ifdef PHX_NEXT_OTP
        printf("CV: FlashCMD totalBytes %d initialOffset %x next_otp_valid %d\n", totalBytes, initialOffset, CV_PERSISTENT_DATA->next_otp_valid);
        if( initialOffset == 0x1234 )
        {
            printf("CV: Received NEXT OTP data\n");
            CV_PERSISTENT_DATA->next_otp_valid = 1;
            memcpy(CV_PERSISTENT_DATA->next_otp, (uint8_t *)(inputParams[0]+2), (totalBytes>596)? 596 : totalBytes);
        } else if ( initialOffset == 0x4321 )
        {
            printf("CV: Invalidate NEXT OTP data\n");
            CV_PERSISTENT_DATA->next_otp_valid = 0;
        }
        if( initialOffset == 0x1234 || initialOffset == 0x4321 )
        {
            /* update OTP */
            cvWritePersistentData(TRUE);
        }
        break;
#endif
#ifndef USH_BOOTROM /* SBI */
		phx_qspi_flash_enumerate((uint8_t *)&type, (uint8_t *)&size);

		size = (size*1024*1024)/8;
#if 0
		if (initialOffset < USH_SBI_SIZE) {
			printf("WARNING: sbi might be getting corrupted. Offset %08x\n", initialOffset);
		}
#endif
		if (initialOffset > size) {
			printf("WARNING: Offset %08x greater than size of flash %08x\n", initialOffset, size);
		}
#endif

		/* check if the memory is in range */
		if ((initialOffset > USH_FLASH_SIZE) ||
			((initialOffset > USH_SBI_SIZE) && (initialOffset < (USH_FLASH_SIZE - USH_FLASH_SCD_PERSISTANT_SIZE))) ||
			(((initialOffset+totalBytes) > USH_SBI_SIZE) && ((initialOffset+totalBytes) < (USH_FLASH_SIZE - USH_FLASH_SCD_PERSISTANT_SIZE))) ) {
			printf("FLASH_PRG Error: %08x %d\n", initialOffset, totalBytes);
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

		/* check for sbi upgrade and set a flag*/
		if (initialOffset < USH_SBI_SIZE) {
			SET_SBI_UPGRADE_FLAG();
		}

		printf("FLASH_PRG: %08x %d\n", initialOffset, totalBytes);

		if (ush_write_flash(initialOffset, totalBytes, (uint8_t *)(inputParams[0]+3)) != PHX_STATUS_OK) {
			retValCmd = CV_FLASH_ACCESS_FAILURE;
		}

		/* check for sbi upgrade and unset the flag*/
		if (initialOffset < USH_SBI_SIZE) {
			UNSET_SBI_UPGRADE_FLAG();
		}

		break;
		}
	case CV_CMD_FW_UPGRADE_START:
		{
		uint32_t initialOffset;
		uint32_t totalBytes;
#ifdef USH_BOOTROM
		retValCmd = CV_FEATURE_NOT_AVAILABLE;
		goto cmd_err;

#else
		/* No output parameters, status indicated by returnStatus */
		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			goto cmd_err;
		}

		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

        	totalBytes = *(inputParams[0]+1);
        	initialOffset = *(inputParams[0]+2);

			*(inputParams[0]+2) = sizeof(tpm_secure_code_desc);

        if(ushFieldUpgradeStart(MEM_BASE_XMEM,
			                    (field_upgrade_start_blob *)(inputParams[0]+2),
		                        0, 0 ) != PHX_STATUS_OK)
			retValCmd = CV_FW_UPGRADE_START_FAILED;
		break;
#endif
		}
	case CV_CMD_FW_UPGRADE_UPDATE:
		{
		uint32_t initialOffset;
		uint32_t totalBytes;
#ifdef USH_BOOTROM  /*AAI */
		retValCmd = CV_FEATURE_NOT_AVAILABLE;
		goto cmd_err;

#else
		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			goto cmd_err;
		}

		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

        	totalBytes = *(inputParams[0]+1);
        	initialOffset = *(inputParams[0]+2);

			*(inputParams[0]+2) = totalBytes;

        if (ushFieldUpgradeUpdate(MEM_BASE_XMEM, (field_upgrade_update_blob *)(inputParams[0]+2), 0)
		                        != PHX_STATUS_OK) {
            printf("UPGRADE UPDATE FAILED !!!\n");
			retValCmd = CV_FW_UPGRADE_UPDATE_FAILED;
		    goto cmd_err;
        }
		break;
#endif
		}
	case CV_CMD_FW_UPGRADE_COMPLETE:
		{
		uint32_t initialOffset;
		uint32_t totalBytes;
#ifdef USH_BOOTROM  /*AAI */
		retValCmd = CV_FEATURE_NOT_AVAILABLE;
		goto cmd_err;

#else
		if (isTPMUpgradeActivated()) {
			retValCmd = CV_FEATURE_NOT_AVAILABLE;
			goto cmd_err;
		}

		/* No output parameters, status indicated by returnStatus */
		if (inputParams[0] == 0) {
			/* abort and send error */
			numOutputParams = 0;
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}

        	totalBytes = *(inputParams[0]+1);
        	initialOffset = *(inputParams[0]+2);

			*(inputParams[0]+2) = 0;

        if(ushFieldUpgradeComplete(MEM_BASE_XMEM,
			                    (field_upgrade_complete_blob *)(inputParams[0]+2),
						0)
		                        != PHX_STATUS_OK)
			retValCmd = CV_FW_UPGRADE_COMPLETE_FAILED;
		break;
#endif
		}

	case CV_CMD_IO_DIAG:
		{
		unsigned char tmp[512];

		if (*inputParams[2] > sizeof(tmp) ) {
			numOutputParams = 0;
			printf("LOOPBACK_DIAG: invalid output lenght %d\n", *inputParams[2]);
            		retValCmd = CV_INVALID_INPUT_PARAMETER;
            		goto cmd_err;
		}
		printf("LOOPBACK_DIAG: length %d\n", *inputParams[0]);
		//dumpArray("Input blob\n", (uint8_t *)inputParams[1], *inputParams[0]);
		memcpy(&tmp[0], (uint8_t *)inputParams[1], sizeof(tmp));

		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[2];
		outputParams[0] = &outputParamEntry->param;
		memcpy((uint8_t *)outputParams[0], (uint8_t *)&tmp[0], outputLengths[0]);
		//dumpArray("Ouput blob\n", (uint8_t *)outputParams[0], outputLengths[0]);
		numOutputParams++;
		}
		break;

#ifdef CV_TEST_TPM				
	case CV_CMD_TEST_TPM:
		/* set up output params */
		outputEncapTypes[0] = CV_ENCAP_INOUT_LENVAL_PAIR;
		outputLengths[0] = *inputParams[2];
		outputParams[0] = &outputParamEntry->param;
		numOutputParams++;
		if ((retValCmd = cv_test_tpm(*inputParams[0], (uint8_t *)inputParams[1], &outputLengths[0],
			(uint8_t *)outputParams[0])) != CV_SUCCESS) {
			goto cmd_err;
		}
		break;
#endif

	case 0xffff:
	default:
		/* Invalid command */
		retValCmd = CV_INVALID_COMMAND;
		numOutputParams = 0;
		break;
	}

cmd_err:
#if 0
	/* if the result is an error, dump the command */
	if (retValCmd != CV_SUCCESS) {
		cvclass_dump_cmd(cmd, cmd->transportLen);
	}
#endif

	/* Restore encap command values not set by encapsulate */
	cmd->flags = sav_encap_flags;
	cmd->version = CV_VOLATILE_DATA->curCmdVersion;
	cmd->transportType = sav_transportType;

	/* now, encapsulate result */
	cmd->commandID = commandID;
	if (encapsulateResult)
		cvEncapsulateCmd(cmd, numOutputParams, &outputParams[0], &outputLengths[0],
			&outputEncapTypes[0], retValCmd);

	/* need special handling for captureOnly case.  encapsulated command is built around image buffer, which */
	/* will be output directly */
	if (captureOnlySpecialCase)
	{
		/* set up interrupt before command */
		usbInt = (cv_usb_interrupt *)((uint8_t *)cmd - sizeof(cv_usb_interrupt));
		usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
		usbInt->resultLen = cmd->transportLen;

		/* before switching to open mode, move window pointers to correspond to encap command */
		set_smem_openwin_base_addr(CV_IO_OPEN_WINDOW, (uint8_t *)usbInt);
		set_smem_openwin_end_addr(CV_IO_OPEN_WINDOW, (uint8_t *)cmd + cmd->transportLen);
		set_smem_openwin_open(CV_IO_OPEN_WINDOW);
		if (cmd->flags.USBTransport)
		{
			usbCvSend(usbInt, (uint8_t *)cmd, cmd->transportLen, HOST_CONTROL_REQUEST_TIMEOUT);
			/* now wait for command to be sent, in order to move window pointers back */
			/* note that this doesn't have a timeout, because it must complete before window */
			/* pointers can be moved */

			set_smem_openwin_base_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF);
			set_smem_openwin_end_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF + CV_IO_COMMAND_BUF_SIZE);
			usbCvCommandComplete((uint8_t *)CV_SECURE_IO_COMMAND_BUF);
		} else
			tpmCvQueueCmdToHost(cmd);
	}
	else
	{
		memcpy(CV_SECURE_IO_COMMAND_BUF, cmd, cmd->transportLen);
		/* now point to io buf */
		/* now copy to io buf and send result */
		cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
		/* set up interrupt after command */
		usbInt = (cv_usb_interrupt *)((uint8_t *)cmd + cmd->transportLen);
		usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
		usbInt->resultLen = cmd->transportLen;

		/* before switching to open mode, zero all memory in window after command */
		memset(CV_SECURE_IO_COMMAND_BUF + cmd->transportLen + sizeof(cv_usb_interrupt), 0, CV_IO_COMMAND_BUF_SIZE - (cmd->transportLen + sizeof(cv_usb_interrupt)));
		set_smem_openwin_open(CV_IO_OPEN_WINDOW);

		/* send IntrIn and BulkIn, then wait for completion */
		if (!usbCvSend(usbInt, (uint8_t *)cmd, cmd->transportLen, HOST_CONTROL_REQUEST_TIMEOUT))
		{
			/* check to see if any of the commands associated with setting up a session timed out.  if so, need */
			/* to delete the temporary session */
			printf("cvManager: command cancelled %x\n",cmd->commandID);
#ifdef USH_BOOTROM  /*AAI */
			if ((cmd->commandID == CV_CMD_OPEN) || (cmd->commandID == CV_SECURE_SESSION_GET_PUBKEY)
				|| (cmd->commandID == CV_SECURE_SESSION_HOST_NONCE))
			{
				/* close session */
				printf("cvManager: close session %x\n",cmd->hSession);
				cv_close(cmd->hSession);
			}
#endif
		}
#ifdef USH_BOOTROM  /*AAI */
		printf("%s: Completed commandid=%x\n",__func__, (int)commandID);
#endif
		usbCvCommandComplete((uint8_t *)cmd);
	}

	/* Reinitialize Cmd version */
	cvGetVersion(&CV_VOLATILE_DATA->curCmdVersion);

	if (reboot_cmd) {
		reboot_ush(reboot_cmd);
	}
	if (do_menu_cmds) {
		set_yield_all_queues();
		start_console_log();
		switch (do_menu_cmds) {
			case 1:
				//BSCD_CLI_main();
			break;
		}
		stop_console_log();
		unset_yield_all_queues();
	}

	if (do_pwr_cmds) {
		phx_ush_enter_pwr_mode(do_pwr_cmds);
	}

#if 0
	printf("CV: cmd done %x\n", retValCmd);
#endif
	/* indicate that CV command is not active */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_COMMAND_ACTIVE;

}

#else /* SBI */
void cvManager(void)
{
	cv_status retValCmd = CV_SUCCESS;
	uint32_t *inputParams[MAX_INPUT_PARAMS];
	uint32_t *outputParams[MAX_OUTPUT_PARAMS];
	uint32_t outputLengths[MAX_OUTPUT_PARAMS];
	uint32_t outputEncapTypes[MAX_OUTPUT_PARAMS];
	uint32_t	numInputParams;
	cv_param_list_entry *outputParamEntry;
	uint32_t numOutputParams = 0;
	uint8_t encapsulateResult = TRUE;
	cv_version curCmdVersion;

	cv_usb_interrupt *usbInt;
	cv_encap_command *cmd;
	cv_command_id commandID;
	cv_encap_flags sav_encap_flags;
	cv_transport_type sav_transportType;

	/* clear all the local buffers */
	memset(outputParams, 0, MAX_OUTPUT_PARAMS);
	memset(outputLengths, 0, MAX_OUTPUT_PARAMS);
	memset(outputEncapTypes,0, MAX_OUTPUT_PARAMS);

	post_log("\n%s",__func__);

	/* first, copy from iobuf to command buf */
	cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	memcpy(CV_COMMAND_BUF, cmd, cmd->transportLen);
	cmd = (cv_encap_command *)CV_COMMAND_BUF;

	/* Save encap command values for later use, since buffer is reused during host storage communication */
	sav_encap_flags = cmd->flags;

	/* FIXME */
	//CV_VOLATILE_DATA->curCmdVersion = cmd->version;

	curCmdVersion = cmd->version;
	sav_transportType = cmd->transportType;
	commandID = cmd->commandID;

	/* reenable antihammering tests, if disabled during last command */
	//CV_VOLATILE_DATA->CVInternalState &= ~CV_ANTIHAMMERING_SUSPENDED;
	/* clear CV-only radio mode override, in case previous command was interrupted before could be cleared */
	//CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_OVERRIDE;

	/* next, decapsulate command */
	retValCmd = cvDecapsulateCmd(cmd, &inputParams[0], &numInputParams);
	if (retValCmd != CV_SUCCESS)
	{
		/* Failed to decapsulate command */
		goto cmd_err;
	}

	/* command buffer used for output as well.	API handlers must ensure that input parameters not overwritten */
	outputParamEntry = (cv_param_list_entry *)&cmd->parameterBlob;
	post_log("\ncommandID:0x%x",cmd->commandID);

	/*Only following subset supported in SBI */
	switch (cmd->commandID)
	{
		case CV_CMD_USH_VERSIONS:
			outputParams[0] = &outputParamEntry->param;
			outputEncapTypes[0] = CV_ENCAP_LENVAL_STRUC;
			numOutputParams = 1;
			outputParams[0] = &outputParamEntry->param;
			outputLengths[0]= MAX_CV_COMMAND_LENGTH;
			retValCmd = ush_bootrom_versions(numInputParams - 2, (inputParams[2]), &outputLengths[0], outputParams[0]);
			outputLengths[0] = ALIGN_DWORD(outputLengths[0]);
			if (retValCmd != CV_SUCCESS)
			{
				numOutputParams = 0;
				goto cmd_err;
			}
			break;
		case CV_CMD_LOAD_SBI:
		case CV_CMD_FLASH_PROGRAMMING:
		case CV_CMD_FW_UPGRADE_START:
		case CV_CMD_FW_UPGRADE_UPDATE:
		case CV_CMD_FW_UPGRADE_COMPLETE:
		case CV_CMD_REBOOT_TO_SBI:
		case CV_CMD_REBOOT_USH:
		case CV_CMD_GET_MACHINE_ID:
		case CV_CMD_IO_DIAG:
		case CV_CMD_GENERATE_ECDSA_KEY:
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
	/* FIXME */
	//cmd->version = CV_VOLATILE_DATA->curCmdVersion;
	
	cmd->version = curCmdVersion;
	cmd->transportType = sav_transportType;

	/* now, encapsulate result */
	cmd->commandID = commandID;
	if (encapsulateResult)
		cvEncapsulateCmd(cmd, numOutputParams, &outputParams[0], &outputLengths[0],
			&outputEncapTypes[0], retValCmd);

	memcpy(CV_SECURE_IO_COMMAND_BUF, cmd, cmd->transportLen);
	/* now point to io buf */
	/* now copy to io buf and send result */
	cmd = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	/* set up interrupt after command */
	usbInt = (cv_usb_interrupt *)((uint8_t *)cmd + cmd->transportLen);
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	usbInt->resultLen = cmd->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	memset(CV_SECURE_IO_COMMAND_BUF + cmd->transportLen + sizeof(cv_usb_interrupt), 0, CV_IO_COMMAND_BUF_SIZE - (cmd->transportLen + sizeof(cv_usb_interrupt)));

	/* send IntrIn and BulkIn, then wait for completion */
	if (!usbCvSend(usbInt, (uint8_t *)cmd, cmd->transportLen, HOST_CONTROL_REQUEST_TIMEOUT))
	{
		/* check to see if any of the commands associated with setting up a session timed out.	if so, need */
		/* to delete the temporary session */
		post_log("\n%s: command cancelled %x",__func__,cmd->commandID);
	}

	usbCvCommandComplete((uint8_t *)cmd);
	post_log("\n%s: Completed command id=%x",__func__, (int)commandID);

}
#endif /*SBI */

//#pragma push
//#pragma arm section rodata="__section_antihamm_rodata" code="antihammering_code"

unsigned int
checkIfAntiHammeringTriggered(unsigned int timer)
{
	ARG_UNUSED(timer);
	return 0;   // antihammering is not implemented yet and we have seen this causing an issue during an upgrade

#if 0  /*anti hammering not implemented TBD */
#ifdef USH_BOOTROM  /*AAI */
#define ANTIHAM_FLSHPRG_TIMEOUT	30*1000
#define ANTIHAM_FWUP_TIMEOUT	60*1000

#define ANTIHAM_TIMEOUT_TIMEOUT	360*1000	/* 6 minutes lockout for flash writes */

	const unsigned int ah_timeout[] = {
				ANTIHAM_FLSHPRG_TIMEOUT,
				ANTIHAM_FWUP_TIMEOUT
				};

	const unsigned int ah_timeout_timeout[] = {
				ANTIHAM_TIMEOUT_TIMEOUT,
				ANTIHAM_TIMEOUT_TIMEOUT
				};

	unsigned int cur_tick = GET_CUR_TICK_IN_MS();
	unsigned int last_tick = VOLATILE_MEM_PTR->last_antiham_start_time[timer];
	unsigned int delta_time = cvGetDeltaTime(VOLATILE_MEM_PTR->last_antiham_start_time[timer]);
	unsigned int ah_timeout_locked = ah_timeout_timeout[timer] ;

	if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL) {
		ah_timeout_locked *= 2; /* twice .... due to  C0 flash write limitations */
	}


	if ((last_tick == 0x0) ||
		(delta_time > ah_timeout_locked)) {
		VOLATILE_MEM_PTR->last_antiham_start_time[timer] = cur_tick;
		set_reboot_flshwrites_count(0); /* reset the flshwrite count */
	} else if ( delta_time > ah_timeout[timer]) {
		printf("FLSH: ANTIHAM: timer %d delta %u timeout %u\n", timer, delta_time, ah_timeout[timer]);
		return 1;

	}
#endif /* USH_BOOTROM */
#define ANTIHAM_FLSHWRITES_MAX	175 * 3 /* ie  2 fw upgrades + sbi */
	/* sbi */
	inc_reboot_flshwrites_count(); /* increment the flash write count */
	if (get_reboot_flshwrites_count() > ANTIHAM_FLSHWRITES_MAX) {
#ifndef USH_BOOTROM /* SBI */
		printf("FLSH: ANTIHAM: timer %d writes %u Max %u\n", timer, get_reboot_flshwrites_count(), ANTIHAM_FLSHWRITES_MAX);
		return 1;
#endif
	}
	return 0;
#endif
}

#ifdef USH_BOOTROM  /*AAI */
void clearAntiHammeringCounters()
{
#if 0 /*anti hammering not implemented TBD */
	OS_MEMSET(&VOLATILE_MEM_PTR->last_antiham_start_time[0], 0x0, sizeof(VOLATILE_MEM_PTR->last_antiham_start_time));
#endif
}

#endif

//#pragma pop


