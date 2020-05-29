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
 * cvmanager.c:  Credential Vault Utilities
 */
/*
 * Revision History:
 *
 * 01/07/07 DPA Created.
*/
#include "cvmain.h"
#include <errno.h>
#include <stdlib.h>
#include "console.h"

#include "stub.h"

#ifdef USH_BOOTROM /*AAI */
#include <toolchain/gcc.h>
#endif


#undef CV_DBG

#define DEBUG_WO_HD_MIRROR

void cvGetVersion(cv_version *version)
{
	/* this routine gets the version of the CV firmware and is updated with a firmware upgrade */
	version->versionMajor = CV_MAJOR_VERSION;
	version->versionMinor = CV_MINOR_VERSION;
}
void cvGetCurCmdVersion(cv_version *version)
{
	/* this routine gets the version of the Cur Cmd version */
	*version = CV_VOLATILE_DATA->curCmdVersion;
}

void cvByteSwap(uint8_t *word)
{
	/* do byte swap from BE to LE */
	uint8_t temp;

	temp = word[0];
	word[0] = word[3];
	word[3] = temp;
	temp = word[1];
	word[1] = word[2];
	word[2] = temp;
}


cv_status cvRandom(uint32_t randLen, uint8_t *randOut)
{
	/* this routine computes the requested number of bytes of random data */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	fips_rng_context  rngctx;
	cv_status retVal = CV_SUCCESS;
	uint32_t blockSize, remBytes = randLen, bytesComputed, actualBytes;
	uint32_t block[MAX_RAW_RNG_SIZE_WORD];

	//FIXME
	ARG_UNUSED(bytesComputed);
	ARG_UNUSED(rngctx);
	ARG_UNUSED(cryptoHandle);

	if (randLen > (MAX_RAW_RNG_SIZE_WORD*sizeof(uint32_t)))
		blockSize = MAX_RAW_RNG_SIZE_WORD*sizeof(uint32_t);
	else if (randLen < (MIN_RAW_RNG_SIZE_WORD*sizeof(uint32_t)))
		blockSize = MIN_RAW_RNG_SIZE_WORD*sizeof(uint32_t);
	else
		blockSize = randLen;
	phx_crypto_init(cryptoHandle);

#ifndef CV_DBG
	if ((phx_rng_fips_init(cryptoHandle, &rngctx, TRUE)) != PHX_STATUS_OK) /* rng selftest included */
	{
		retVal = CV_RNG_FAILURE;
		goto err_exit;
	}
	do
	{
		actualBytes = (remBytes > blockSize) ? blockSize : remBytes;
		bytesComputed = (actualBytes < MIN_RAW_RNG_SIZE_WORD*sizeof(uint32_t)) ? MIN_RAW_RNG_SIZE_WORD*sizeof(uint32_t) : actualBytes;
		if ((phx_rng_raw_generate(cryptoHandle, (uint8_t *)block,
								   bytesComputed/4, 0, 0)) != PHX_STATUS_OK)
		{
			retVal = CV_RNG_FAILURE;
			goto err_exit;
		}

		memcpy(randOut, block, actualBytes);
		remBytes -= actualBytes;
		randOut += actualBytes;
	} while(remBytes);
#else
	{
		uint32_t i;
		int32_t randNum;
		uint32_t byteIndex;

		for (i=0;i<randLen;i++)
		{
			byteIndex = i%(sizeof(int));
			if (!byteIndex)
				randNum = rand();
			randOut[i] = ((uint8_t *)&randNum)[byteIndex];
		}
	}
#endif
err_exit:
	return retVal;
}

cv_status cvAuth(uint8_t *authStart, uint32_t authLen, uint8_t *HMACKey, uint8_t *hashOrHMAC, uint32_t hashOrHMACLen,
		   uint8_t *otherHashData, uint32_t otherHashDataLen, cv_hash_op_internal hashOp)
{
	/* this routine does a hash or HMAC of the input data */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cipher_cmd;
	cv_status retVal = CV_SUCCESS;
	uint32_t keyLen = (HMACKey == NULL) ? 0 : hashOrHMACLen;

	//FIXME
	ARG_UNUSED(hashOrHMAC);
	ARG_UNUSED(cryptoHandle);
	ARG_UNUSED(keyLen);
	ARG_UNUSED(ctx);

	/* now do auth */
	/* if other hash data is provided, copy it in */
	if (otherHashData)
	{
		memcpy(authStart + authLen, otherHashData, otherHashDataLen);
		authLen += otherHashDataLen;
	}

	/* there are a couple of reasons for the 2 implementations below:  */
	/*      1. because the routines phx_get_hmac_sha1/256_hash() don't handle multiple stage sha */
	/*		2. it was discovered that endianness wasn't being handled correctly for unaligned input, but some hmacs were */
	/*		   used for persistent data, so fixing them would cause existing data blob to not authenticate, so CV_SHA_LEGACY */
	/*		   was created for these */ 

	phx_crypto_init(cryptoHandle);
	if (hashOp != CV_SHA)
	{
		/* check to see if HMAC or hash */
		memset(&cipher_cmd,0,sizeof(cipher_cmd));
		if (HMACKey == NULL)
		{
			if (hashOrHMACLen == SHA256_LEN)
				cipher_cmd.auth_alg     = PHX_AUTH_ALG_SHA256;
			else
    			cipher_cmd.auth_alg     = PHX_AUTH_ALG_SHA1;
			hashOrHMACLen = 0;
		} else {
			if (hashOrHMACLen == SHA256_LEN)
				cipher_cmd.auth_alg     = PHX_AUTH_ALG_SHA256;
			else
    			cipher_cmd.auth_alg     = PHX_AUTH_ALG_SHA1;
		}
    		cipher_cmd.auth_mode     = BCM_SCAPI_AUTH_MODE_HMAC;
		cipher_cmd.cipher_mode  = PHX_CIPHER_MODE_DECRYPT;
		cipher_cmd.encr_alg     = PHX_ENCR_ALG_NONE;
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_NONE;
		cipher_cmd.cipher_order = PHX_CIPHER_ORDER_NULL;
		/* set up stage */
		switch (hashOp)
		{
		case CV_SHA_START:
			cipher_cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
            phx_bulk_cipher_swap_endianness(cryptoHandle, &ctx, TRUE, TRUE, TRUE);
			break;
		case CV_SHA_UPDATE:
			cipher_cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_UPDATE;
            phx_bulk_cipher_swap_endianness(cryptoHandle, &ctx, TRUE, TRUE, TRUE);
			break;
		case CV_SHA_FINAL:
			cipher_cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_FINAL;
            phx_bulk_cipher_swap_endianness(cryptoHandle, &ctx, TRUE, TRUE, TRUE);
			break;
		case CV_SHA_LEGACY:
		default:
			cipher_cmd.auth_mode = PHX_AUTH_MODE_ALL;
			break;
		}
#ifndef CV_5890_DBG
		if ((phx_bulk_cipher_init(cryptoHandle, &cipher_cmd, NULL, HMACKey, hashOrHMACLen, &ctx)) != PHX_STATUS_OK)
		{
			retVal = CV_CRYPTO_FAILURE;
			goto err_exit;
		}
		if ((phx_bulk_cipher_start(cryptoHandle, authStart, NULL, 0, 0, authLen, 0, hashOrHMAC, &ctx)) != PHX_STATUS_OK)
		{
			retVal = CV_CRYPTO_FAILURE;
			goto err_exit;
		}
		if ((phx_bulk_cipher_dma_start (cryptoHandle, &ctx, 0, NULL, 0)) != PHX_STATUS_OK)
		{
			retVal = CV_CRYPTO_FAILURE;
			goto err_exit;
		}
#endif
	} else {
		/* here if CV_SHA */
		/* first check to see if SHA1 or SHA256 */
		if (hashOrHMACLen == SHA1_LEN)
		{
			/* SHA1 */
			if ((phx_get_hmac_sha1_hash(cryptoHandle, authStart, authLen,
					HMACKey, keyLen, hashOrHMAC, TRUE)) != PHX_STATUS_OK)
			{
				retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
				goto err_exit;
			}
		}
		else if (hashOrHMACLen == SHA256_LEN)
		{
			/* SHA256 */
			if ((phx_get_hmac_sha256_hash(cryptoHandle, authStart, authLen,
					HMACKey, keyLen, hashOrHMAC, TRUE)) != PHX_STATUS_OK)
			{
				retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
				goto err_exit;
			}
		} else {
			retVal = CV_UNSUPPORTED_CRYPT_FUNCTION;
			goto err_exit;
		}
	}

err_exit:
	return retVal;
}

cv_status
cvCrypt(uint8_t *cryptStart, uint32_t cryptLen, uint8_t *AESKey, uint8_t *IV, cv_bulk_mode mode, cv_bool encrypt)
{
	/* this routine does in-place encrypt/decrypt */
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cipher_cmd;
	cv_status retVal = CV_SUCCESS;

	//FIXME
	ARG_UNUSED(cryptoHandle);
	ARG_UNUSED(ctx);
	ARG_UNUSED(cryptStart);
	ARG_UNUSED(cryptLen);
	ARG_UNUSED(AESKey);
	ARG_UNUSED(IV);


	phx_crypto_init(cryptoHandle);
	memset(&cipher_cmd,0,sizeof(cipher_cmd));
	if (encrypt)
		cipher_cmd.cipher_mode  = PHX_CIPHER_MODE_ENCRYPT;
	else
 		cipher_cmd.cipher_mode  = PHX_CIPHER_MODE_DECRYPT;
	cipher_cmd.encr_alg     = PHX_ENCR_ALG_AES_128;
	if (mode == CV_CBC)
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CBC;
	else if (mode == CV_CTR)
		cipher_cmd.encr_mode    = PHX_ENCR_MODE_CTR;
	else return CV_CRYPTO_FAILURE;
    cipher_cmd.auth_alg     = PHX_AUTH_ALG_NONE;
	cipher_cmd.cipher_order = PHX_CIPHER_ORDER_NULL;
#ifndef CV_5890_DBG
    if ((phx_bulk_cipher_init(cryptoHandle, &cipher_cmd, AESKey, NULL, 0, &ctx)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}
    if ((phx_bulk_cipher_start(cryptoHandle, cryptStart,IV, cryptLen, 0, 0, 0, cryptStart, &ctx)) != PHX_STATUS_OK)
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
#endif
err_exit:
	return retVal;
}

cv_status
cvDecryptBlob(uint8_t *authStart, uint32_t authLen, uint8_t *cryptStart, uint32_t cryptLen, 
			  uint8_t *cryptStart2, uint32_t cryptLen2,
			  uint8_t *IV, uint8_t *AESKey, uint8_t *IV2, uint8_t *AESKey2, uint8_t *HMACKey, 
			  uint8_t *HMAC, uint32_t HMACLen, cv_bulk_mode mode,
			  uint8_t *otherHashData, uint32_t otherHashDataLen)
{
	/* this routine assumes that the HMAC is in the encrypted area immediately following the rest of the */
	/* encrypted data.  also, otherHashDataLen must be <= size of the HMAC */
	cv_status retVal = CV_SUCCESS;
	cv_hmac_val hmacOut;
	cv_hmac_val hmacIn;

	/* first decrypt parameters */
	if ((retVal = cvCrypt(cryptStart, cryptLen, AESKey, IV, mode, FALSE)) != CV_SUCCESS)
		goto err_exit;

	/* now do auth */
	/* first copy out the decrypted HMAC and insert additional hash data (if any) */
	memcpy((uint8_t *)&hmacIn, HMAC, HMACLen);
	memset(HMAC,0,HMACLen);
	/* compute auth */
	if ((retVal = cvAuth(authStart, authLen, HMACKey, (uint8_t *)&hmacOut, HMACLen, otherHashData, otherHashDataLen,  CV_SHA)) != CV_SUCCESS)
		goto err_exit;
	/* compare HMAC */
	if (memcmp((uint8_t *)&hmacIn, (uint8_t *)&hmacOut, HMACLen))
		retVal = CV_PARAM_BLOB_AUTH_FAIL;

	/* check to see if 2nd decryption key supplied */
	if (AESKey2 != NULL)
		if ((retVal = cvCrypt(cryptStart2, cryptLen2, AESKey2, IV2, mode, FALSE)) != CV_SUCCESS)
			goto err_exit;

err_exit:
	return retVal;
}

cv_status
cvEncryptBlob(uint8_t *authStart, uint32_t authLen, uint8_t *cryptStart, uint32_t cryptLen,
			  uint8_t *cryptStart2, uint32_t cryptLen2,
			  uint8_t *IV, uint8_t *AESKey, uint8_t *IV2, uint8_t *AESKey2, 
			  uint8_t *HMACKey, uint8_t *HMAC, uint32_t HMACLen, cv_bulk_mode mode, 
			  uint8_t *otherHashData, uint32_t otherHashDataLen)
{
	/* this routine assumes that the HMAC is in the encrypted area immediately following the rest of the */
	/* encrypted data.  also, otherHashDataLen must be <= size of the HMAC */
	cv_status retVal = CV_SUCCESS;
	cv_hmac_val hmacOut;
	uint8_t otherDataTemp[SHA256_LEN];

	/* first generate IV */
	if ((retVal = cvRandom(AES_128_IV_LEN, IV)) != CV_SUCCESS)
		goto err_exit;

	/* check to see if 2nd IV required for 2nd encryption */
	if (AESKey2 != NULL)
		if ((retVal = cvRandom(AES_128_IV_LEN, IV2)) != CV_SUCCESS)
			goto err_exit;

	/* check to see if 2nd encryption key supplied */
	if (AESKey2 != NULL)
		cvCrypt(cryptStart2, cryptLen2, AESKey2, IV2, mode, TRUE);

	/* first set area to put HMAC to zero */
	memset(HMAC,0,HMACLen);

	/* need to save data where otherHashData will be stored (may be padding) */
	if (otherHashDataLen)
		memcpy(otherDataTemp, authStart+authLen, otherHashDataLen);

	/* now do auth.  do this after inner encryption, because may need to verify auth without decrypting inner encryption */
	if ((retVal = cvAuth(authStart, authLen, HMACKey, (uint8_t *)&hmacOut, HMACLen, otherHashData, otherHashDataLen, CV_SHA)) != CV_SUCCESS)
		goto err_exit;

	/* need to restore otherHashData */
	if (otherHashDataLen)
		memcpy(authStart+authLen, otherDataTemp , otherHashDataLen);

	/* now copy HMAC to area for encryption */
	memcpy(HMAC, (uint8_t *)&hmacOut, HMACLen);

	/* now encrypt parameters */
	cvCrypt(cryptStart, cryptLen, AESKey, IV, mode, TRUE);

err_exit:
	return retVal;
}

#ifdef USH_BOOTROM  /*AAI */
uint32_t
cvGetDeltaTime(uint32_t startTime)
{
	uint32_t endTime;

	/* this routine computes the delta time and takes into account possible rollover */
	get_ms_ticks(&endTime);
	if (endTime >= startTime)
		return (endTime - startTime);
	else {
		/* ignore any deltas of <= 100 ms, because could be a clock glitch */
		if ((startTime - endTime) <= 100)
			return 0;
		/* here if it's the rollover case */
		return (0xffffffff - startTime + endTime);
	}
}

uint32_t
cvGetPrevTime(uint32_t deltaTime)
{
	uint32_t curTime;

	/* this routine computes the previous time, given a delta and takes into account possible rollover */
	get_ms_ticks(&curTime);
	if (curTime >= deltaTime)
		return (curTime - deltaTime);
	else 
		return (0xffffffff - deltaTime + curTime);
}

cv_status
cvWriteTopLevelDir(cv_bool mirror)
{
	/* this routine writes the top level to flash and mirrors it to the host hard drive. */
	/* note that the monotonic counter must be updated before this routine is called */
	uint32_t count;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	uint8_t *outBuf = HDOTP_AND_TLD_IO_BUF;
/*Removed below code as it is not used when DEBUG_WO_HD_MIRROR */
/*
#ifdef DEBUG_WO_HD_MIRROR
	mirror = FALSE;
#endif	
*/	
	memset(&objAttribs,0,sizeof(cv_obj_storage_attributes));
	memset(&objProperties,0,sizeof(cv_obj_properties));
	if ((retVal = get_predicted_monotonic_counter(&count)) != CV_SUCCESS)
		/* skip clearing of buffer, since it hasn't been used */
		goto err_exit1;

	/* now wrap and write top level dir to flash */
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA; /* this type is because uses same wrapping key */
	objDirPtrs[2] = (uint8_t *)CV_TOP_LEVEL_DIR;
	objLen = sizeof(cv_top_level_dir);
	objDirHandles[2] = CV_TOP_LEVEL_DIR_HANDLE;
	/* use alternate buffer, because CV_SECURE_IO_COMMAND_BUF may be in use when this function is run */
	objProperties.altBuf = HDOTP_AND_TLD_IO_BUF;
	/* if using async FP capture and FP image buffer isn't available, use alternate, base on whether */
	/* this function is called from CV or TPM */
	if (!isFPImageBufAvail())
	{
		if (CV_VOLATILE_DATA->CVDeviceState & CV_COMMAND_ACTIVE)
		{
			/* CV command is active, can use IO buf */
			objProperties.altBuf = CV_SECURE_IO_COMMAND_BUF;
			outBuf = CV_SECURE_IO_COMMAND_BUF;
		} else {
			/* CV not active, must be from TPM, use CV command buf instead */
			objProperties.altBuf = CV_COMMAND_BUF;
			outBuf = CV_COMMAND_BUF;
		}
	}
	CV_TOP_LEVEL_DIR->header.thisDirPage.counter = count;
	if ((retVal = cvWrapObj(&objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			goto err_exit;
	/* compute flash address based on multibuffering.  don't update if this is a retry */
	if (!(CV_VOLATILE_DATA->retryWrites & CV_RETRY_TLD_WRITE))
	{
		CV_VOLATILE_DATA->tldMultibufferIndex = (CV_VOLATILE_DATA->tldMultibufferIndex + 1) & CV_FLASH_MULTI_BUF_MASK;
		/* if 2T disabled, need to write index to flash */
		if (!(CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE))
			ush_write_flash((uint32_t)CV_TLD_MULTIBUF_INDX_FLASH_ADDR, sizeof(uint32_t), (uint8_t *)&CV_VOLATILE_DATA->tldMultibufferIndex);
	}
	CV_VOLATILE_DATA->retryWrites |= CV_RETRY_TLD_WRITE;


//	printf("write TLD credits %d count %d index %d\n",CV_TOP_LEVEL_DIR->header.antiHammeringCredits,count,CV_VOLATILE_DATA->tldMultibufferIndex);
	flashEntry.pObj = (uint8_t *)CV_TOP_LEVEL_DIR_FLASH_ADDR + CV_TOP_LEVEL_DIR_FLASH_OFFSET*(CV_VOLATILE_DATA->tldMultibufferIndex);
	flashEntry.objLen = objLen;
	if ((retVal = cvPutFlashObj(&flashEntry, outBuf)) != CV_SUCCESS)
			goto err_exit;
	/* Removed the below dead code to fix coverity issue */
	
	#ifndef DEBUG_WO_HD_MIRROR
	if (mirror)
	{
		/* now mirror top level dir to host HD as well */
		objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ;
		objDirPtrs[2] = (uint8_t *)CV_TOP_LEVEL_DIR;
		objLen = sizeof(cv_top_level_dir);
		objDirHandles[2] = CV_MIRRORED_TOP_LEVEL_DIR;
		if ((retVal = cvWrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
				goto err_exit;
		/* (ignore return status in case host hard drive not available */
		cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF);
	}
	#endif
err_exit:
	/* clear buffer used for output */
	memset(outBuf,0x55,sizeof(cv_top_level_dir));
err_exit1:
	return retVal;
}

cv_status
cvWriteDirectoryPage0(cv_bool mirror)
{
	/* this routine writes the directory page 0 to flash and mirrors it to the host hard drive. */
	/* note that the monotonic counter must be updated before this routine is called */
	uint32_t count;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_status retVal = CV_SUCCESS;
/*Removed below code as it is not used when DEBUG_WO_HD_MIRROR */
/*
#ifdef DEBUG_WO_HD_MIRROR
	mirror = FALSE;
#endif
*/	
	memset(&objAttribs,0,sizeof(cv_obj_storage_attributes));
	if ((retVal = get_predicted_monotonic_counter(&count)) != CV_SUCCESS)
		goto err_exit;

	/* now wrap and write directory page 0 to flash */
	CV_TOP_LEVEL_DIR->dirEntries[0].counter = count;
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_NON_OBJ;
	objDirPtrs[2] = (uint8_t *)CV_DIR_PAGE_0;
	objLen = sizeof(cv_dir_page_0);
	objDirHandles[2] = CV_DIR_0_HANDLE;
	if ((retVal = cvWrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
		goto err_exit;
	/* compute flash address based on multibuffering.  update buffer index in top level dir (if this isn't a retry) */
	if (!(CV_VOLATILE_DATA->retryWrites & CV_RETRY_DIR0_WRITE))
		CV_TOP_LEVEL_DIR->dir0multiBufferIndex = (CV_TOP_LEVEL_DIR->dir0multiBufferIndex + 1) & CV_FLASH_MULTI_BUF_MASK;
	CV_VOLATILE_DATA->retryWrites |= CV_RETRY_DIR0_WRITE;
	flashEntry.pObj = (uint8_t *)CV_DIRECTORY_PAGE_0_FLASH_ADDR + CV_DIR_PAGE_0_FLASH_OFFSET*CV_TOP_LEVEL_DIR->dir0multiBufferIndex;
	flashEntry.objLen = objLen;
	if ((retVal = cvPutFlashObj(&flashEntry, CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
		goto err_exit;
	/*Removed the below dead code to fix coverity issue */
	#ifndef DEBUG_WO_HD_MIRROR
	if (mirror)
	{
		/* now mirror directory page 0 to host HD as well */
		objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ;
		objDirPtrs[2] = (uint8_t *)CV_DIR_PAGE_0;
		objLen = sizeof(cv_dir_page_0);
		objDirHandles[2] = CV_MIRRORED_DIR_0;
		if ((retVal = cvWrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			goto err_exit;
		/* (ignore return status in case host hard drive not available */
		cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF);
	}
	#endif
err_exit:
	return retVal;
}

cv_status
cvWritePersistentData(cv_bool mirror)
{
	/* this routine writes persistent data to flash.  persistent data is multi-buffered */
	/* top level dir must also be written (so monotonic counter must be bumped) */
	uint32_t count, predictedCount;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_status retVal = CV_SUCCESS;
	uint32_t i;
	cv_hdotp_skip_table *pHdotpSkipTable = (cv_hdotp_skip_table *)((CHIP_IS_5882) ? CV_SKIP_TABLE_BUFFER : FP_CAPTURE_LARGE_HEAP_PTR);

#ifdef DEBUG_WO_HD_MIRROR
	mirror = FALSE;
#endif
#ifndef DISABLE_PERSISTENT_STORAGE
	memset(&objAttribs,0,sizeof(cv_obj_storage_attributes));
	/* now check to see if anti-hammering will allow 2T usage */
	if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
		goto err_exit;

	/* read skip tables */
	cvReadHDOTPSkipTable(pHdotpSkipTable);
	/* loop with 2T bump last, so that failure during process will cause retry of entire process */
	for (i=0;i<CV_MAX_2T_BUMP_FAIL;i++)
	{
		/* first do predictive bump to get next 2T value */
		if (get_predicted_monotonic_counter(&predictedCount) != CV_SUCCESS)
			goto err_exit;

		CV_TOP_LEVEL_DIR->persistentData.entry.counter = predictedCount;
		objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
		objDirPtrs[2] = (uint8_t *)CV_PERSISTENT_DATA;
		objLen = sizeof(cv_persistent);
		objDirHandles[2] = CV_PERSISTENT_DATA_HANDLE;
		if ((retVal = cvWrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			continue;
		/* compute flash address based on multibuffering.  update buffer index in top level dir (if 1st time through loop) */
		if (i == 0)
			CV_TOP_LEVEL_DIR->persistentData.multiBufferIndex = (CV_TOP_LEVEL_DIR->persistentData.multiBufferIndex + 1) & CV_FLASH_MULTI_BUF_MASK;
		CV_TOP_LEVEL_DIR->persistentData.len = objLen;
		flashEntry.pObj = (uint8_t *)CV_PERSISTENT_DATA_FLASH_ADDR + CV_PERSISTENT_DATA_FLASH_OFFSET*CV_TOP_LEVEL_DIR->persistentData.multiBufferIndex;
		flashEntry.objLen = objLen;
		if ((retVal = cvPutFlashObj(&flashEntry, CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
			continue;
		/* now wrap and write top level dir to flash */
		if ((retVal = cvWriteTopLevelDir(mirror)) != CV_SUCCESS)
			continue;
		/* write out HDOTP skip table */
		if ((retVal = cvWriteHDOTPSkipTable()) != CV_SUCCESS)
			continue;
		/* now try to bump 2T */
		if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
			continue;
		/* if 2T succeeded, make sure matches predicted count */
		if (count == predictedCount)
		{
			/* successful, zero retry flags */
			CV_VOLATILE_DATA->retryWrites = 0;
			break;
		}
	}
	/* check to see if loop terminated successfully */
	if (retVal == CV_SUCCESS && (i == CV_MAX_2T_BUMP_FAIL))
		/* count never matched predicted value */
		retVal = CV_MONOTONIC_COUNTER_FAIL;
	if (retVal != CV_SUCCESS)
		goto err_exit;
	
err_exit:
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
#endif
	return retVal;
}

cv_status
cvWriteAntiHammeringCreditData(void)
{
#ifndef CV_ANTIHAMMERING
	return CV_SUCCESS;
#else
	/* this routine writes the anti hammering credit data */
	/* note that this blob is not anti-replay protected */
	cv_status retVal = CV_SUCCESS;
	uint32_t count;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_antihammering_credit creditData;
	uint8_t buf[sizeof(cv_antihammering_credit) + SHA256_LEN + AES_128_BLOCK_LEN + sizeof(cv_obj_dir_page_blob)];
	cv_obj_properties objProperties;

	memset(&objAttribs,0,sizeof(cv_obj_storage_attributes));
	memset(&objProperties,0,sizeof(cv_obj_properties));

	/* don't need to read in skip tables here, because get_monotonie_counter will use local count */
	if ((retVal = get_monotonic_counter(&count)) != CV_SUCCESS)
		goto err_exit;

	/* now wrap and write antireplay credit data to flash */
//	printf("write antihammering data credits %d count %d\n",CV_TOP_LEVEL_DIR->header.antiHammeringCredits,count);
	creditData.counter = count;
	creditData.credits = CV_TOP_LEVEL_DIR->header.antiHammeringCredits;
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA; /* this type is because uses same wrapping key */
	objDirPtrs[2] = (uint8_t *)&creditData;
	objLen = sizeof(cv_antihammering_credit);
	objDirHandles[2] = CV_ANTIHAMMERING_CREDIT_DATA_HANDLE;
	/* use buffer on stack, because CV_SECURE_IO_COMMAND_BUF may be in use when this function is run */
	objProperties.altBuf = buf;
	if ((retVal = cvWrapObj(&objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			goto err_exit;
	flashEntry.pObj = (uint8_t *)CV_AH_CREDIT_FLASH_ADDR;
	flashEntry.objLen = objLen;
	if ((retVal = cvPutFlashObj(&flashEntry, buf)) != CV_SUCCESS)
		goto err_exit;
	
err_exit:
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
	return retVal;
#endif
}

cv_status
cvWriteHDOTPSkipTable(void)
{
#ifndef HDOTP_SKIP_TABLES
	return CV_SUCCESS;
#else
	/* this routine writes the HDOTP skip table */
	/* note that this blob is not anti-replay protected */
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen, offset;
	cv_hdotp_skip_table *hdotpSkipTable = CV_VOLATILE_DATA->hdotpSkipTablePtr;
	cv_obj_properties objProperties;
	uint8_t *outBuf = HDOTP_AND_TLD_IO_BUF;

	/* table should be resident, but if not, exit with success here */
	if (CV_VOLATILE_DATA->hdotpSkipTablePtr ==0)
		return CV_SUCCESS;
	memset(&objAttribs,0,sizeof(cv_obj_storage_attributes));
	memset(&objProperties,0,sizeof(cv_obj_properties));
	/* now wrap and write HDOTP skip table to flash */
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA; /* this type is because uses same wrapping key */
	objDirPtrs[2] = (uint8_t *)hdotpSkipTable;
	objLen = sizeof(uint32_t) + hdotpSkipTable->tableEntries * sizeof(uint32_t);
	objDirHandles[2] = CV_HDOTP_TABLE_HANDLE;
	/* use alternate buffer, because CV_SECURE_IO_COMMAND_BUF may be in use when this function is run */
	objProperties.altBuf = HDOTP_AND_TLD_IO_BUF;
	/* if using async FP capture and FP image buffer isn't available, use alternate, base on whether */
	/* this function is called from CV or TPM */
	if (!isFPImageBufAvail())
	{
		if (CV_VOLATILE_DATA->CVDeviceState & CV_COMMAND_ACTIVE)
		{
			/* CV command is active, can use IO buf */
			objProperties.altBuf = CV_SECURE_IO_COMMAND_BUF;
			outBuf = CV_SECURE_IO_COMMAND_BUF;
		} else {
			/* CV not active, must be from TPM, use CV command buf instead */
			objProperties.altBuf = CV_COMMAND_BUF;
			outBuf = CV_COMMAND_BUF;
		}
	}
	/* set offset based on multibuffer index for TLD */
	offset = (CV_VOLATILE_DATA->tldMultibufferIndex == 0) ? 0 : CV_HDOTP_SKIP_TABLE_FLASH_OFFSET;
	if ((retVal = cvWrapObj(&objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			goto err_exit;
	if ((ushx_flash_read_write(CV_BRCM_HDOTP_CUST_ID, offset, objLen, outBuf, 1)) != PHX_SUCCESS)
	{
		/* read/write failure, translate to CVAPI return code */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}
	
err_exit:
	/* clear buffer used for output */
	memset(outBuf,0x56,objLen);
	return retVal;
#endif
}

cv_status
cvReadHDOTPSkipTable(cv_hdotp_skip_table *hdotpSkipTable)
{
#ifndef HDOTP_SKIP_TABLES
	return CV_SUCCESS;
#else
	/* this routine reads the HDOTP skip table into the buffer supplied */
	/* note that this blob is not anti-replay protected */
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL}, *objDirPtrsOut[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t hdotpTableLen, padLen, hdotpTableOffset;
	uint8_t *objStoragePtr = HDOTP_AND_TLD_IO_BUF;

	/* if using async FP capture and FP image buffer isn't available, use alternate, based on whether */
	/* this function is called from CV or TPM */
	if (!isFPImageBufAvail())
	{
		if (CV_VOLATILE_DATA->CVDeviceState & CV_COMMAND_ACTIVE)
			/* CV command is active, can use IO buf */
			objStoragePtr = CV_SECURE_IO_COMMAND_BUF;
		else
			/* CV not active, must be from TPM, use CV command buf instead */
			objStoragePtr = CV_COMMAND_BUF;
	}
	objDirPtrs[2] = objStoragePtr;
	objDirPtrsOut[2] = (uint8_t *)hdotpSkipTable;
	objDirHandles[2] = CV_HDOTP_TABLE_HANDLE;
	/* assume max size table here, because don't know size till it's unwrapped */
	hdotpTableLen = sizeof(cv_hdotp_skip_table) + sizeof(cv_obj_dir_page_blob) + SHA256_LEN;
	padLen = GET_AES_128_PAD_LEN(hdotpTableLen);
	hdotpTableLen += padLen;
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
	/* set offset based on multibuffer index for TLD */
	hdotpTableOffset = (CV_VOLATILE_DATA->tldMultibufferIndex == 0) ? 0 : CV_HDOTP_SKIP_TABLE_FLASH_OFFSET;
	if ((ushx_flash_read_write(CV_BRCM_HDOTP_CUST_ID, hdotpTableOffset, hdotpTableLen, objStoragePtr, 0)) != PHX_SUCCESS)
		/* read/write failure, silently re-init table */
		hdotpSkipTable->tableEntries = 0;
	if ((cvUnwrapObj(NULL, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
		/* unwrap failure, silently re-init table */
		hdotpSkipTable->tableEntries = 0;

	/* save pointer to HDOTP skip table */
	CV_VOLATILE_DATA->hdotpSkipTablePtr = hdotpSkipTable;

	/* also initialize retry writes flags.  this will allow multibuffer index for flash writes to be bumped the first time */
	/* through loop which updates flash and monotonic counter */
	CV_VOLATILE_DATA->retryWrites = 0;

	/* clear buffer used for input */
	memset(objStoragePtr,0x57,hdotpTableLen);
	return CV_SUCCESS;
#endif
}

void 
cvInvalidateHDOTPSkipTable(void)
{
#ifdef HDOTP_SKIP_TABLES
	/* this routine invalidates the HDOTP skip table in RAM so will need to be read in again next time */ 
	CV_VOLATILE_DATA->hdotpSkipTablePtr = 0;
#endif
}

cv_bool 
cvIsValidHDOTPSkipTable(void)
{
	/* this routine returns TRUE if the HDOTP skip table is in RAM */ 
	return ((CV_VOLATILE_DATA->hdotpSkipTablePtr != 0) ? TRUE : FALSE);
}

void
cvSet2TandAHEnables(cv_enables_2TandAH enables)
{
	cv_enables_2TandAH value = 0, value_cur = 0;
	uint16_t cookie;

	/* this routine sets 2T and AH enables for engineering mode */
	if (!(enables & CV_ENABLES_USE_DEFAULTS))
	{
		memcpy(&cookie, "2T", 2);
		value = cookie << 16;
		if (enables & CV_ENABLES_2T)
			value |= CV_ENABLES_2T;
		if (enables & CV_ENABLES_AH)
		{
			value |= CV_ENABLES_AH;
			/* also reset credits to default (engr mode only) */
 			if (!(VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) || (VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) == 0xff)
			{
				CV_TOP_LEVEL_DIR->header.antiHammeringCredits = 200/*CV_AH_INITIAL_CREDITS*/;
				cvWriteAntiHammeringCreditData();
			}
		}
		if (enables & CV_ENABLES_DA)
			value |= CV_ENABLES_DA;
	}
	cvGet2TandAHEnables(&value_cur);
	/* only write to flash if changes */
	if ((value & ~CV_ENABLES_2TANDAH_COOKIE_MASK) != value_cur)
		ush_write_flash((uint32_t)CV_ENABLES_2TANDAH_FLASH_ADDR, sizeof(cv_enables_2TandAH), (uint8_t *)&value);
}

void
cvGet2TandAHEnables(cv_enables_2TandAH *enables)
{
	cv_enables_2TandAH value = 0;
	uint16_t cookie;

	/* this routine gets 2T and AH enables for engineering mode */
	ush_read_flash((uint32_t)CV_ENABLES_2TANDAH_FLASH_ADDR, sizeof(cv_enables_2TandAH), (uint8_t *)&value);
	cookie = (value & CV_ENABLES_2TANDAH_COOKIE_MASK) >> 16;
	if (!memcmp("2T", &cookie, 2))
		*enables = value & ~CV_ENABLES_2TANDAH_COOKIE_MASK;
	else
		*enables = CV_ENABLES_USE_DEFAULTS;
}

void
cvGetAHCredits(uint32_t *credits)
{
	/* only get credits if in engineering mode */
 	if (!(VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) || (VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) == 0xff)
		*credits = CV_TOP_LEVEL_DIR->header.antiHammeringCredits;
	else
		*credits = 0;
}

void
cvGetElapsedTime(uint32_t *time)
{
	/* only get elapsed time if in engineering mode */
 	if (!(VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) || (VOLATILE_MEM_PTR->nCustomerID & CUSTID_MASK) == 0xff)
		get_ms_ticks(time);
	else
		*time = 0;
}

/*************************************************************************
 *
 *  Function Name:  doLshft1
 *
 *  Inputs:     src - src bytes
 *              len - number of bytes
 *
 *  Outputs:    src - src << 1
 *
 *  Returns:    
 *
 *  Description:  
 *
 *  This function does a left shift by 1
 *
 *************************************************************************/
void doLshft1(uint8_t *src, uint32_t len)
{
	uint32_t i;

	for (i=0;i<(len-1);i++)
		src[i] = (src[i]<<1) | (src[i+1]>>7);
	src[len-1] = src[len-1]<<1;
}

/*************************************************************************
 *
 *  Function Name:  doXor
 *
 *  Inputs:     src - src bytes
 *              dst - dest byte
 *              len - number of bytes
 *
 *  Outputs:    dst - src ^ dst
 *
 *  Returns:    
 *
 *  Description:  
 *
 *  This function does an xor of 2 byte strings
 *
 *************************************************************************/
void doXor(uint8_t *src, uint8_t *dst, uint32_t len)
{
	uint32_t i;

	for (i=0;i<len;i++)
		dst[i] ^= src[i];
}
/*************************************************************************
 *
 *  Function Name:  omac1Aes
 *
 *  Inputs:     xmem - heap pointer
 *              key - input key
 *              message - random bytes to be used to generate key
 *              messageLen - length of message
 *
 *  Outputs:    res - result block generated
 *
 *  Returns:    tpm_result standard error code
 *
 *  Description:  
 *
 *  This function performs OMAC1 as specified in reference 6 of CT-KIP Protocol v1.0
 *
 *************************************************************************/
int omac1Aes(uint8_t *key, uint8_t *message, uint32_t messageLen, uint8_t *res)
{
	uint32_t	retVal = CV_SUCCESS;
	uint8_t zero[AES_128_BLOCK_LEN];
	uint8_t L[AES_128_BLOCK_LEN];
	uint8_t LdotU[AES_128_BLOCK_LEN];
	uint8_t LdotUSq[AES_128_BLOCK_LEN];
	uint32_t blocks = messageLen/AES_128_BLOCK_LEN, i, remainderBytes;
	uint8_t buf[2*AES_128_BLOCK_LEN]; 
	uint8_t *Y_prev = &buf[0];
	uint8_t *Y = &buf[AES_128_BLOCK_LEN];
	uint8_t X[AES_128_BLOCK_LEN];

	/* compute L */
	memset(zero,0,AES_128_BLOCK_LEN);
	memcpy(L, zero, AES_128_BLOCK_LEN);
	if ((retVal = cvCrypt(L, AES_128_BLOCK_LEN, key, NULL, CV_CTR, TRUE)) != CV_SUCCESS)
		goto err_exit;
	/* compute L.u */
	memcpy(LdotU, L, AES_128_BLOCK_LEN);
	doLshft1(LdotU, AES_128_BLOCK_LEN);
	/* check to see if most significant bit is zero */
	if (L[0] & 0x80)
		/* 'xor' in Constant (0x87) */
		LdotU[AES_128_BLOCK_LEN - 1] ^= 0x87;
	/* compute L.u**2 */
	memcpy(LdotUSq, LdotU, AES_128_BLOCK_LEN);
	doLshft1(LdotUSq, AES_128_BLOCK_LEN);
	/* check to see if most significant bit is zero */
	if (LdotU[0] & 0x80)
		/* 'xor' in Constant (0x87) */
		LdotUSq[AES_128_BLOCK_LEN - 1] ^= 0x87;

	/* now compute blocks for tag generation */
	remainderBytes = messageLen - blocks*AES_128_BLOCK_LEN;
	if (remainderBytes)
		blocks++;
	memset(Y,0,AES_128_BLOCK_LEN);
	memset(Y_prev,0,AES_128_BLOCK_LEN);
	for (i=1;i<blocks;i++)
	{
		doXor(message, Y_prev, AES_128_BLOCK_LEN);
		memcpy(Y, Y_prev, AES_128_BLOCK_LEN);
		if ((retVal = cvCrypt(Y, AES_128_BLOCK_LEN, key, NULL, CV_CTR, TRUE)) != CV_SUCCESS)
			goto err_exit;
		message+=AES_128_BLOCK_LEN;
		memcpy(Y_prev, Y, AES_128_BLOCK_LEN);
	}
	/* now check to see if the last block has AES_128_BLOCK_LEN bytes */
	if (!remainderBytes)
	{
		memcpy(X, message, AES_128_BLOCK_LEN);
		doXor(Y, X, AES_128_BLOCK_LEN);
		doXor(LdotU, X, AES_128_BLOCK_LEN);
	} else {
		memcpy(X, message, remainderBytes);
		X[remainderBytes] = 0x80;
		memset(&X[remainderBytes+1],0,AES_128_BLOCK_LEN - (remainderBytes + 1));
		doXor(Y, X, AES_128_BLOCK_LEN);
		doXor(LdotUSq, X, AES_128_BLOCK_LEN);
	}
	memcpy(res, X, AES_128_BLOCK_LEN);
	if ((retVal = cvCrypt(X, AES_128_BLOCK_LEN, key, NULL, CV_CTR, TRUE)) != CV_SUCCESS)
		goto err_exit;

err_exit:
	return retVal;
}

/*************************************************************************
 *
 *  Function Name:  ctkipPrfAes
 *
 *  Inputs:     key - input key
 *              message - random bytes to be used to generate key
 *              messageLen - length of message
 *              secretLen - desired length of key to be generated
 *
 *  Outputs:    secret - output key generated
 *
 *  Returns:    standard CV error code
 *
 *  Description:  secretLen must be <= 2*AES_BLOCK_SIZE
 *
 *  This function performs CT-KIP-PRF-AES specified in CT-KIP Protocol v1.0
 *
 *************************************************************************/
cv_status
ctkipPrfAes(uint8_t *key, uint8_t *message, uint32_t messageLen, uint32_t secretLen, uint8_t *secret)
{
	cv_status	retVal = CV_SUCCESS;
	uint32_t	numOutputBlocks;
	uint8_t res[2*AES_128_BLOCK_LEN];

	numOutputBlocks = secretLen/AES_128_BLOCK_LEN;
	if (numOutputBlocks*AES_128_BLOCK_LEN < secretLen)
		numOutputBlocks++;
	/* exit if output secret greater than 2 blocks */
	if (numOutputBlocks > 2)
		return CV_CRYPTO_FAILURE;

	/* generate first output block */
	if (omac1Aes(key, message, messageLen, res) != CV_SUCCESS)
		return CV_CRYPTO_FAILURE;

	/* see if need to generate 2nd block */
	if (numOutputBlocks == 2)
	{
		*((uint32_t *)message) = 2;
		if (omac1Aes(key, message, messageLen, &res[AES_128_BLOCK_LEN]) != CV_SUCCESS)
			return CV_CRYPTO_FAILURE;
	}

	/* copy bytes to output */
	memcpy(secret, res, secretLen);

	return retVal;
}


void
cvCopySwap(uint32_t *in, uint32_t *out, uint32_t numBytes)
{
	/* this routine does a conversion between a big-endian byte stream and BIGINT used by SCAPI crypto API, */
	/* which is little endian */
	/* input must be word aligned */
	uint32_t i;
	uint32_t *src = (uint32_t *)((uint8_t *)in + numBytes - 4);
	uint32_t *dst = (uint32_t *)out;
	uint32_t sizeWords = numBytes/sizeof(uint32_t);

	for (i = 0; i < sizeWords; i++)
	{
		*dst = *src;
		cvByteSwap((uint8_t *)dst);
		dst++;
		src--;
	}
}

void
cvInplaceSwap(uint32_t *in, uint32_t numWords)
{
	/* this routine does a conversion between a big-endian byte stream and BIGINT used by SCAPI crypto API, */
	/* which is little endian */
	/* input must be word aligned */
	uint32_t i;
	uint32_t temp;

	for (i=0;i<numWords/2;i++)
	{
		temp = in[i];
		in[i] = in[numWords - i - 1];
		in[numWords - i - 1] = temp;
		cvByteSwap((uint8_t *)(&in[i]));
		cvByteSwap((uint8_t *)(&in[numWords - i - 1]));
	}
	/* if there's an odd number of words, byte swap the 'middle' one here */
	if (numWords & 0x1)
		cvByteSwap((uint8_t *)(&in[i]));
}

uint32_t 
cvGetNormalizedLength(uint8_t *in, uint32_t totalLength)
{
	/* this routine returns the number of bytes of a BIGINT, ignoring leading zeros */
	int32_t i, lenOut = 0;

	for (i=(totalLength - 1);i>=0;i--)
	{
		if (in[i] !=0)
		{
			lenOut = i+1;
			break;
		}
	}
	return lenOut;
}


cv_bool 
cvIsZero(uint8_t *in, uint32_t totalLength)
{
	/* this routine returns TRUE if the array pointed to is all zeroes */
	uint32_t i;

	for (i=0;i<totalLength;i++)
		if (in[i] != 0)
			return FALSE;
	return TRUE;
}


void
cvSetupInputParameterPtrsLong(cv_input_parameters *inputParameters, uint32_t count, 
						  uint32_t paramLen1, void *paramPtr1, uint32_t paramLen2, void *paramPtr2,
						  uint32_t paramLen3, void *paramPtr3, uint32_t paramLen4, void *paramPtr4,
						  uint32_t paramLen5, void *paramPtr5, uint32_t paramLen6, void *paramPtr6,
						  uint32_t paramLen7, void *paramPtr7, uint32_t paramLen8, void *paramPtr8,
						  uint32_t paramLen9, void *paramPtr9, uint32_t paramLen10, void *paramPtr10,
						  uint32_t paramLen11, void *paramPtr11, uint32_t paramLen12, void *paramPtr12,
						  uint32_t paramLen13, void *paramPtr13, uint32_t paramLen14, void *paramPtr14,
						  uint32_t paramLen15, void *paramPtr15, uint32_t paramLen16, void *paramPtr16,
						  uint32_t paramLen17, void *paramPtr17, uint32_t paramLen18, void *paramPtr18,
						  uint32_t paramLen19, void *paramPtr19, uint32_t paramLen20, void *paramPtr20)
{
	/* this routine handles parameter lists longer than 10 parameters */

	cvSetupInputParameterPtrs(inputParameters, count, 
						  paramLen1, paramPtr1, paramLen2, paramPtr2,
						  paramLen3, paramPtr3, paramLen4, paramPtr4,
						  paramLen5, paramPtr5, paramLen6, paramPtr6,
						  paramLen7, paramPtr7, paramLen8, paramPtr8,
						  paramLen9, paramPtr9, paramLen10, paramPtr10);

	inputParameters->paramLenPtr[10].paramLen = paramLen11;
	inputParameters->paramLenPtr[10].param = paramPtr11;
	inputParameters->paramLenPtr[11].paramLen = paramLen12;
	inputParameters->paramLenPtr[11].param = paramPtr12;
	inputParameters->paramLenPtr[12].paramLen = paramLen13;
	inputParameters->paramLenPtr[12].param = paramPtr13;
	inputParameters->paramLenPtr[13].paramLen = paramLen14;
	inputParameters->paramLenPtr[13].param = paramPtr14;
	inputParameters->paramLenPtr[14].paramLen = paramLen15;
	inputParameters->paramLenPtr[14].param = paramPtr15;
	inputParameters->paramLenPtr[15].paramLen = paramLen16;
	inputParameters->paramLenPtr[15].param = paramPtr16;
	inputParameters->paramLenPtr[16].paramLen = paramLen17;
	inputParameters->paramLenPtr[16].param = paramPtr17;
	inputParameters->paramLenPtr[17].paramLen = paramLen18;
	inputParameters->paramLenPtr[17].param = paramPtr18;
	inputParameters->paramLenPtr[18].paramLen = paramLen19;
	inputParameters->paramLenPtr[18].param = paramPtr19;
	inputParameters->paramLenPtr[19].paramLen = paramLen20;
	inputParameters->paramLenPtr[19].param = paramPtr20;
}

	void
cvSetupInputParameterPtrs(cv_input_parameters *inputParameters, uint32_t count, 
						  uint32_t paramLen1, void *paramPtr1, uint32_t paramLen2, void *paramPtr2,
						  uint32_t paramLen3, void *paramPtr3, uint32_t paramLen4, void *paramPtr4,
						  uint32_t paramLen5, void *paramPtr5, uint32_t paramLen6, void *paramPtr6,
						  uint32_t paramLen7, void *paramPtr7, uint32_t paramLen8, void *paramPtr8,
						  uint32_t paramLen9, void *paramPtr9, uint32_t paramLen10, void *paramPtr10)
{
	/* this routine sets up the input parameter pointers */
	inputParameters->numParams = count;
	inputParameters->paramLenPtr[0].paramLen = paramLen1;
	inputParameters->paramLenPtr[0].param = paramPtr1;
	inputParameters->paramLenPtr[1].paramLen = paramLen2;
	inputParameters->paramLenPtr[1].param = paramPtr2;
	inputParameters->paramLenPtr[2].paramLen = paramLen3;
	inputParameters->paramLenPtr[2].param = paramPtr3;
	inputParameters->paramLenPtr[3].paramLen = paramLen4;
	inputParameters->paramLenPtr[3].param = paramPtr4;
	inputParameters->paramLenPtr[4].paramLen = paramLen5;
	inputParameters->paramLenPtr[4].param = paramPtr5;
	inputParameters->paramLenPtr[5].paramLen = paramLen6;
	inputParameters->paramLenPtr[5].param = paramPtr6;
	inputParameters->paramLenPtr[6].paramLen = paramLen7;
	inputParameters->paramLenPtr[6].param = paramPtr7;
	inputParameters->paramLenPtr[7].paramLen = paramLen8;
	inputParameters->paramLenPtr[7].param = paramPtr8;
	inputParameters->paramLenPtr[8].paramLen = paramLen9;
	inputParameters->paramLenPtr[8].param = paramPtr9;
	inputParameters->paramLenPtr[9].paramLen = paramLen10;
	inputParameters->paramLenPtr[9].param = paramPtr10;
}

cv_status
cvValidateSessionHandle(cv_handle cvHandle)
{
	cv_session *session = (cv_session *)cvHandle;

	/* this routine validates the session handle parameter */
	if (session >= (cv_session *)&CV_HEAP[0] && session < (cv_session *)&CV_HEAP[CV_VOLATILE_OBJECT_HEAP_LENGTH])
	{
		if (memcmp((uint8_t *)&session->magicNumber,"SeSs", sizeof(session->magicNumber)))
			/* not session */
			return CV_INVALID_HANDLE;
		/* this is a valid session.  check to see if this session has UI prompts suppressed.  if so, set */
		/* volatile internal state to allow PIN prompts to be returned to app for command resubmission */
		if (session->flags & CV_SUPPRESS_UI_PROMPTS)
			CV_VOLATILE_DATA->CVInternalState |= CV_SESSION_UI_PROMPTS_SUPPRESSED;
		else
			CV_VOLATILE_DATA->CVInternalState &= ~CV_SESSION_UI_PROMPTS_SUPPRESSED;
		return CV_SUCCESS;
	} else
		return CV_INVALID_HANDLE;
}

void
cvCloseOpenSessions(cv_handle cvHandle)
{
	/* this function closes all open sessions except the input parameter */
	cv_session *session;
	uint32_t i;

	/* go through all volatile memory and look for open sessions */
	for (i=0;i<CV_VOLATILE_OBJECT_HEAP_LENGTH;i+=4)
	{
		session = (cv_session *)&CV_HEAP[i];
		/* skip if this is the session to keep open */
		if ((uint8_t *)session == (uint8_t *)cvHandle)
			continue;
		/* if this is an open session, close it */
		if (!memcmp((uint8_t *)&session->magicNumber,"SeSs", sizeof(session->magicNumber)))
		{
			/* this is a session, free session memory */
			/* first clear magic number */
			memset((uint8_t *)&session->magicNumber,0,sizeof(session->magicNumber));
			cv_free(session, sizeof(cv_session));
		}
	}
}

void
cvPrintOpenSessions(void)
{
	/* this function prints all open sessions */
	cv_session *session;
	uint32_t i;

	/* go through all volatile memory and look for open sessions */
	printf("cvPrintOpenSessions:\n");
	for (i=0;i<CV_VOLATILE_OBJECT_HEAP_LENGTH;i+=4)
	{
		session = (cv_session *)&CV_HEAP[i];
		/* if this is an open session, print it */
		if (!memcmp((uint8_t *)&session->magicNumber,"SeSs", sizeof(session->magicNumber)))
			printf("0x%x\n",session);
	}
}

uint8
cvUpdateAntiHammeringCredits(void)
{
	/* this routine determines the available credits, based on elapsed time */
#ifndef CV_ANTIHAMMERING
	return USH_RET_TIMER_INT_OK;
#else
	uint32_t deltaTimeMs;
	uint32_t credits;
	uint32_t remTimeMs;

	/* get time since last update */
	deltaTimeMs = cvGetDeltaTime(CV_VOLATILE_DATA->timeLastAHCreditsUpdate);
	/* workaround to account for Atmel flash requiring 2 writes for each page write */
	if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
		credits = deltaTimeMs/(2*CV_AH_MS_PER_CREDIT);
	else
		credits = deltaTimeMs/CV_AH_MS_PER_CREDIT;
	if (credits)
	{
		/* at least one credit if not maxed out */
		CV_TOP_LEVEL_DIR->header.antiHammeringCredits += credits;
		/* limit max credits that can accumulate */
		if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits > CV_AH_MAX_CREDITS)
			CV_TOP_LEVEL_DIR->header.antiHammeringCredits = CV_AH_MAX_CREDITS;
		/* workaround to account for Atmel flash requiring 2 writes for each page write */
		if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
			remTimeMs = deltaTimeMs % (2*CV_AH_MS_PER_CREDIT);
		else
			remTimeMs = deltaTimeMs % CV_AH_MS_PER_CREDIT;
		/* now save the update time, subtracting off any remaining delta */
		CV_VOLATILE_DATA->timeLastAHCreditsUpdate = cvGetPrevTime(remTimeMs);
	}
	//printf("update credits: new %d total %d\n",credits, CV_TOP_LEVEL_DIR->header.antiHammeringCredits);
	/* now check to see if AH credits need to be written to flash.  these are written as part of top level dir */
	/* but may also be written here, if TLD hasn't been written for the defined period of */
	/* time.  this is because if time elapses without using a 2T bit, the TLD isn't written, but credits */
	/* may accumulate and need to be persisted */
	if (cvGetDeltaTime(CV_VOLATILE_DATA->timeLast2Tbump) > CV_UPDATE_AH_CREDITS_INTERVAL)
	{
		cvWriteAntiHammeringCreditData();
		get_ms_ticks(&CV_VOLATILE_DATA->timeLast2Tbump);
	}
	return USH_RET_TIMER_INT_OK;
#endif
}

cv_status
cvCheckAntiHammeringStatus(void)
{
	/* this routine determines if anti-hammering is active and returns the appropriate status */
#ifndef CV_ANTIHAMMERING
	return CV_SUCCESS;
#else
	uint32_t interval;

	/* check to see if anti-hammering has been suspended to allow a 'must-succeed' operation */
	/* to occur */
	if (CV_VOLATILE_DATA->CVInternalState & CV_ANTIHAMMERING_SUSPENDED)
		return CV_SUCCESS;

	/* also see if disabled */
	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_ANTIHAMMERING_ACTIVE))
		return CV_SUCCESS;

	/* first determine the threshold by number of credits available */
	interval = cvGetDeltaTime(CV_VOLATILE_DATA->timeLast2Tbump);
//	printf("bump 2T interval: %d\n",interval);
	if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits == 0)
		return CV_ANTIHAMMERING_PROTECTION;
	if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits <= CV_AH_CREDIT_THRESHOLD_LOW)
	{
		if (interval < CV_AH_BUMP_INTERVAL_LOW)
			return CV_ANTIHAMMERING_PROTECTION;
	} else if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits <= CV_AH_CREDIT_THRESHOLD_MED) {
		if (interval < CV_AH_BUMP_INTERVAL_MED)
			return CV_ANTIHAMMERING_PROTECTION_DELAY_MED;
	} else if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits <= CV_AH_CREDIT_THRESHOLD_HIGH) {
		if (interval < CV_AH_BUMP_INTERVAL_HIGH)
			return CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH;
	}
	/* subtract one credit here */
	CV_TOP_LEVEL_DIR->header.antiHammeringCredits--;
	get_ms_ticks(&CV_VOLATILE_DATA->timeLast2Tbump);
//	printf("bump 2T: %d\n",CV_TOP_LEVEL_DIR->header.antiHammeringCredits);
	return CV_SUCCESS;
#endif
}

cv_status
cvCheckAntiHammeringStatusPreCheck(void)
{
	/* this routine calls cvCheckAntiHammeringStatus to get the status, but restores the state afterwards. */
	/* the purpose is to allow for commands to exit sooner than the operation that will decrement the AH credits */
	/* in case the command is resubmitted */
	uint32_t savedCredits, savedInterval;
	cv_status retVal;

	savedCredits = CV_TOP_LEVEL_DIR->header.antiHammeringCredits;
	savedInterval = CV_VOLATILE_DATA->timeLast2Tbump;
	if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
		goto err_exit;
	CV_TOP_LEVEL_DIR->header.antiHammeringCredits = savedCredits;
	CV_VOLATILE_DATA->timeLast2Tbump = savedInterval;

err_exit:
	return retVal;
}

cv_bool
updateHDOTPReadLimitCount(void)
{
	/* this routine increments the number of reads of HDOTP in order to determine if limit has been reached */
	/* if limit is reached, HDOTP must be bumped, so persistent data is written */
	cv_hdotp_read_limit readLimitFlash1, readLimitFlash2, readLimitFlash;
	uint32_t cookie1, cookie2, cookie, readLimit1, readLimit2, readLimit = 0;
	cv_bool limitExceeded = FALSE;
	uint32_t writeIndex = 0;
	uint32_t addrBuf[2] = {(uint32_t)CV_HDOTP_READ_LIMIT_FLASH_ADDR1, (uint32_t)CV_HDOTP_READ_LIMIT_FLASH_ADDR2};

	/* first read both buffers and determine which to update */
	ush_read_flash(addrBuf[0], sizeof(cv_hdotp_read_limit), (uint8_t *)&readLimitFlash1);
	ush_read_flash(addrBuf[1], sizeof(cv_hdotp_read_limit), (uint8_t *)&readLimitFlash2);
	cookie1 = (readLimitFlash1 & CV_HDOTP_READ_LIMIT_MASK) >> 16;
	cookie2 = (readLimitFlash2 & CV_HDOTP_READ_LIMIT_MASK) >> 16;
	memcpy(&cookie, "HD", 2);
	readLimit1 = readLimitFlash1 & ~CV_HDOTP_READ_LIMIT_MASK;
	readLimit2 = readLimitFlash2 & ~CV_HDOTP_READ_LIMIT_MASK;
	if (memcmp("HD", &cookie1, 2))
		/* buffer 1 uninitialized, initialize and use it */
		writeIndex = 0;
	else if (memcmp("HD", &cookie2, 2))
		/* buffer 2 uninitialized, initialize and use it */
		writeIndex = 1;
	else {
		/* both have been initialized, determine the larger count and use it, but write new value to other buf */
		if (readLimit1 > readLimit2)
		{
			readLimit = readLimit1;
			writeIndex = 1;
		} else {
			readLimit = readLimit2;
			writeIndex = 0;
		}
	}
	readLimit += (CV_VOLATILE_DATA->extraCount) ? 2 : 1;
	if (readLimit > CV_HDOTP_READ_LIMIT)
		/* if limit exceeded, persistent data will be written and read limit bufs will be reset */
		limitExceeded = TRUE;
	else {
		/* limit not exceeded, write out new value */
		readLimitFlash = (cookie << 16) | readLimit;
		ush_write_flash(addrBuf[writeIndex], sizeof(cv_hdotp_read_limit), (uint8_t *)&readLimitFlash);
	}
	return limitExceeded;
}

void
resetHDOTPReadLimit(void)
{
	/* this routine resets the HDOTP read limit counter */
	cv_hdotp_read_limit readLimitFlash;
	uint32_t cookie, readLimit = 0;

	memcpy(&cookie, "HD", 2);
	readLimitFlash = (cookie << 16) | readLimit;
	ush_write_flash((uint32_t)CV_HDOTP_READ_LIMIT_FLASH_ADDR1, sizeof(cv_hdotp_read_limit), (uint8_t *)&readLimitFlash);
	ush_write_flash((uint32_t)CV_HDOTP_READ_LIMIT_FLASH_ADDR2, sizeof(cv_hdotp_read_limit), (uint8_t *)&readLimitFlash);
}

cv_status 
cv_pkcs1_v15_encrypt_encode(uint32_t mLen,  uint8_t *M, uint32_t emLen, uint8_t *em)
{
	/* this routine does a PKCS1 v1.5 encoding for RSA encryption */
	uint32_t i;
	uint32_t psLen;                    /* pad string length (PS) */
	uint8_t *bp = em;  /* Setup pointer buffers */
	cv_status retVal = CV_SUCCESS;

	/* 1. length check */
	if (emLen < mLen + 11)
		return CV_ENCRYPTION_FAILURE;

	/* 4. Generate PS, which is a number of 0xFF --- refer to step 5 */

	/* 5. Generate em = 0x00 || 0x02 || PS || 0x00 || T */
	psLen = emLen - mLen - 3;
	*bp = 0x00; bp++;
	*bp = 0x02; bp++;

    for(i=0; i<psLen; i++) {
        do {
			if ((retVal = cvRandom(1,bp)) != CV_SUCCESS)
				goto err_exit;
        } while(*bp == 0x00);
        bp++;
    }

	/* Post the last flag uint8_t */
    *bp = 0x00; bp++;

    memcpy(bp, M, mLen);

err_exit:
	return retVal;
}

cv_status 
cv_pkcs1_v15_decrypt_decode(uint32_t emLen,  uint8_t *em, uint32_t *mLen, uint8_t *M)
{
	/* this routine does a PKCS1 v1.5 decoding for RSA decryption */
	uint32_t i;
	uint32_t psLen;                    /* pad string length (PS) */
	uint8_t *bp = em;  /* Setup pointer buffers */
	uint32   cleartextLen = *mLen;

   /* Error check preamble in em buffer */
   if (*bp != 0x00)
	   return CV_ENCRYPTION_FAILURE;
   bp++;

   /* Skip preamble flag byte and error check the flag */
   if (*bp != 0x02)
	   return CV_ENCRYPTION_FAILURE;
   bp++;

   /* Skip PS data in em buffer */
   for (i=0, psLen=0; i<((int)emLen-3); i++) {
     if ( *bp == 0x00 ) {
       bp++;
       break;
     }
     bp++;
     psLen++;
   }
   *mLen = emLen - psLen - 3;
    /* make sure output buffer large enough */
	if (*mLen > cleartextLen)
		return CV_INVALID_OUTPUT_PARAMETER_LENGTH;
    memcpy(M,bp,*mLen);

   /* Error check number of PS data bytes */
   if (psLen < EMSA_PKCS1_V15_PS_LENGTH)
	   return CV_ENCRYPTION_FAILURE;

   /* And the special case where message length is zero, check last flag == 0x00 */ 
   if (*mLen == 0 && *bp != 0) 
	   return CV_ENCRYPTION_FAILURE;

	return CV_SUCCESS;
}

#pragma push
#pragma arm section rodata="__section_cv_pke_rodata"
static const uint8_t sha1DerPkcs1Array[SHA1_DER_PREAMBLE_LENGTH] =
		{0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, \
         0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14};
#pragma pop

cv_status 
cv_pkcs1_v15_sign_encode(uint32_t mLen,  uint8_t *M, uint32_t emLen, uint8_t *em)
{
	/* this routine does a PKCS1 v1.5 encoding for RSA signature */
	cv_status retVal= CV_SUCCESS;
	uint32_t i;
	uint32_t psLen;                    /* pad string length (PS) */
	uint16_t tLen  = SHA1_DER_PREAMBLE_LENGTH + SHA1_HASH_SIZE;
	uint8_t *bp = em;  /* Setup pointer buffers */

	/* 3. length check */
	if (emLen < tLen + 11)
		return CV_ENCRYPTION_FAILURE;

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
		return CV_ENCRYPTION_FAILURE;

    /* Post the last flag uint8_t */
    *bp = 0x00; bp++;

    /* Post T(h) data into em(T) buffer */
    memcpy(bp, sha1DerPkcs1Array, SHA1_DER_PREAMBLE_LENGTH);
    bp += SHA1_DER_PREAMBLE_LENGTH;

    memcpy(bp, M, SHA1_HASH_SIZE);

	return retVal;
}

void
DumpBuffer(uint32_t len, void *buf)
{
	uint32_t i = 0;
	unsigned char *bufLocal = (unsigned char *)buf;

	for (i=0;i<len;i++)
	{
		printf(" %02x", bufLocal[i]);
		if (!((i+1)%16))
		{
			printf("\n");
		}
	}
	printf("\n");
}

#endif /* USH_BOOTROM */

#ifdef CV_5890_DBG
/**********************************************************************************
   Method: readByte
   Description:
	This function reads a byte from an input file.  The file is expected to
	consist of whitespace, hex numbers, and comments.  Comments begin with
	"//" and extend to the end of the line.  Hex numbers must contain two
	hex digits without intervening whitespace or comments.  Multiple pairs
	of hex digits (i.e., bytes) may be concatenated (without intervening
	whitespace).

	Input files must be correct or this function aborts.  It returns false
	either when we have reached the end of a command or the end of a file.
	The caller must determine the difference.

   Assumptions: None
**********************************************************************************/
bool
cvReadByte( FILE * fp, byte * output )
{
	static  int line = 1;
	static int			ch;
	int			comment_idx = 0;
	bool		inComment = FALSE;
#define COMMENT_BUF_LEN 25
	char 		comment_buf[ COMMENT_BUF_LEN ] = { '\0' };

	while ((ch = fgetc( fp )) != EOF)
	{
		if (ch == '\n')
			line++;

		if (inComment == TRUE)
		{
			/*
			 *	When we have a comment, we need to look for special key words.
			 */
			if (ch == '\n')
			{
				/* A comment ends when we reach the end of a line. */
				inComment = FALSE;
				comment_buf[ 0 ] = '\0';
				comment_idx = 0;
			}
			else if (isspace( ch ))
			{
				/* We ignore spaces, but it restarts our search for special key words. */
				comment_buf[ 0 ] = '\0';
				comment_idx = 0;
			}
			else
			{
				/* We have to see if we have found a key word. */
				comment_buf[ comment_idx++ ] = ch;
				if (comment_idx >= COMMENT_BUF_LEN-1)
				{
					comment_idx = 0;
					/*
					 *	Note: if the comment is list long, it cannot match
					 *	any of our key words.
					 */
				}
				comment_buf[ comment_idx ] = '\0';

				/* Look for known keywords.  We currently only use one. */
				/* Note: Since the '#' has been introduced, this code is
				   inefficient.  But, speed is not an issue when simulating. */
				if (!strncmp( comment_buf, "#CMD_END", COMMENT_BUF_LEN ))
				{
					return FALSE; /* We have reached the end of the command. */
				}
			}
			/* Else, nothing.  We just ignore the input. */
		}
		else if (isxdigit( ch ))
		{
		        char buf[3] = { 0, 0, 0 };
			char ch2 = fgetc( fp );
			unsigned hexValue;

			buf[0] = ch;

			if (!isxdigit( ch2 ))
			{
				printf("FATAL: Invalid input: solitary hex character '%c' on line %d.\n", ch, line);
				exit( 1 );
				/* Theoretically should increment line count, but why bother? */
			}

			/* Convert the two digit hex number to binary. */
			buf[1] = ch2;
			sscanf(buf, "%x", &hexValue);
			*output = hexValue;

			return TRUE;
		}
		else if (!isspace( ch ))
		{
			/*
			 *	The only valid non-whitespace, non-hex-digit character
			 *	is a comment delimiter.
			 */
			if (ch == '/')
			{
				ch = fgetc( fp );
				if (ch != '/')
				{
					printf("FATAL: Invalid input: solitary '/' on line %d.\n", line);
					exit( 1 );
					/* Theoretically should increment line count, but why bother? */
				}
				inComment = TRUE;
			}
			else
			{
				printf("FATAL: Invalid input: char '%c' on line %d.\n", ch, line);
				exit( 1 );
			}
		}
		/* else it is white space, which we ignore */
	}

	return FALSE; /* We have reached the end of file. */
}
cv_status
cvReadCmd(FILE *input_file)
{
	cv_status retVal = CV_CANCELLED_BY_USER;
	uint32 cmdCount = 0;
	uint32 cCount = 8;
//	uint8_t buf[2048];
	uint8_t *iomem = CV_SECURE_IO_COMMAND_BUF;
	cv_encap_command *cmd =(cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;

	while ((cmdCount < cCount) && (cvReadByte(input_file, iomem)))
	{
	    cmdCount++;
		iomem++;
		if (!(cmdCount%4))
			cvByteSwap(iomem - 4);
		if (cmdCount == 8)
		   cCount = cmd->transportLen;
		retVal = CV_SUCCESS; // Have to read at least one byte to return true.
	}
	return retVal;
}
cv_status
cvWriteResult(FILE *output_file)
{
	cv_status retVal = CV_CANCELLED_BY_USER;
	uint32 cmdCount = 0;
//	uint8_t buf[2048];
	uint8_t *iomem = CV_SECURE_IO_COMMAND_BUF;
	cv_encap_command *cmd =(cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	uint32 cCount = cmd->transportLen;
	uint32_t i;

	/* byteswap the buffer */
	for (i=0;i<cCount;i+=4)
		cvByteSwap(&iomem[i]);
	while ((cmdCount < cCount) && (EOF != fprintf(output_file, "%02x",*iomem)))
	{
	    cmdCount++;
		iomem++;
		if (!(cmdCount%4))
			fprintf(output_file,"\n");
	}
	if (cmdCount == cCount)
	{
		retVal = CV_SUCCESS;
		fprintf(output_file,"*** END OF COMMAND ***\n");
	}
	return retVal;
}
cv_status
cvReadCommandsFromFile(void)
{
#define CV_MAX_TEST_SESSIONS 2
#define CV_MAX_TEST_OBJECTS 4
#define CV_MAX_FILENAME_LEN 100
	/* this is a test routine that reads a CV encapsulated command from a file */
	FILE *input_testname_file = NULL;
	FILE *input_file = NULL;
	FILE *output_file = NULL;
	cv_status retVal;
	uint8_t filename[CV_MAX_FILENAME_LEN];
	uint8_t filename_testname[] = "tests/cv_testname.inp";
	uint8_t filename_out[] = "cv_test.out";
	cv_handle sessions[CV_MAX_TEST_SESSIONS];
	cv_encap_command *cmd =(cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	uint32_t i, maxSession = 0, maxObj = 0;
	cv_param_list_entry *param = (cv_param_list_entry *)&cmd->parameterBlob;
	int ioretcode;
	cv_obj_handle objHandles[CV_MAX_TEST_OBJECTS];

	for (i=0;i<CV_MAX_TEST_SESSIONS;i++)
		sessions[i] = 0;
	if ((input_testname_file = fopen( filename_testname, "r" )) == NULL)
	{
		retVal = errno;
		return retVal;
	}
	ioretcode = fscanf(input_testname_file, "%s", filename);
	if (ioretcode == EOF || ioretcode ==0)
	{
		retVal = errno;
		return retVal;
	}
	if ((input_file = fopen( filename, "r" )) == NULL)
	{
		retVal = errno;
		return retVal;
	}
	if ((output_file = fopen( filename_out, "w+" )) == NULL)
	{
		retVal = errno;
		return retVal;
	}
	while (cvReadCmd( input_file ) == CV_SUCCESS)
	{
		/* replace session number in file with saved value */
		if (cmd->hSession != 0 && cmd->hSession <= CV_MAX_TEST_SESSIONS)
		{
			cmd->hSession = sessions[cmd->hSession - 1];
			/* also replace session handle in param 1, if not cv_open */
			if (cmd->commandID != CV_CMD_OPEN)
				param->param = cmd->hSession;
		}
		/* if cv_get_object, replace handle */
		if (cmd->commandID == CV_CMD_GET_OBJECT)
			param[1].param = objHandles[param[1].param - 1];
		/* if cv_set_object, replace handle */
		if (cmd->commandID == CV_CMD_SET_OBJECT)
			param[1].param = objHandles[param[1].param - 1];
		/* if cv_delete_object, replace handle */
		if (cmd->commandID == CV_CMD_DELETE_OBJECT)
			param[1].param = objHandles[param[1].param - 1];
		cvManager();
		/* if this command was a cv_open, save returned session value */
		if (maxSession < CV_MAX_TEST_SESSIONS)
		{
			if (cmd->commandID == CV_CMD_OPEN)
				sessions[maxSession++] = param->param;
		}
		/* if cv_create_object, save handle */
		if (cmd->commandID == CV_CMD_CREATE_OBJECT)
			objHandles[maxObj++] = param->param;
		cvWriteResult(output_file);
	}
	fflush(output_file);
	fclose( input_file );
	fclose( output_file );
	return CV_SUCCESS;
}

#endif



