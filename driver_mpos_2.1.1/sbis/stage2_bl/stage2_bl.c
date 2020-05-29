/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

#include <platform.h>
#include <init.h>
#include <post_log.h>
#include <utils.h>
#include <sbi.h>
#include <dmu.h>
#include <string.h>
#include <reg_access.h>
#include <bbl.h>
#include <wdt.h>
#include <iproc_sotp.h>
#include <sotp.h>
#include <iproc_sw.h>
#include <pka/crypto_pka.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>

#include "stage2_bl.h"

#define SBI_VERSION_STRING "Stage2 BL"  " (" SBI_DATE " - " SBI_TIME ")"

static IPROC_STATUS iproc_aes_sha256(
			crypto_lib_handle *pHandle,
			uint8_t *pAESKey,
			uint32_t AESKeyLen,
			uint8_t *pAuthKey,
			SBI_HEADER *pSbiHeader,
			uint8_t *HashOutput,
			BCM_SCAPI_CIPHER_MODE cipher_mode,
			BCM_SCAPI_CIPHER_ORDER cipher_order);

static IPROC_STATUS InitializeSOTP(void);
static IPROC_STATUS InitializeDeviceCfgAndCustomerID(void);
static IPROC_STATUS iproc_read_to_scratch(
			uint32_t address, uint8_t *data, uint32_t data_len);

static IPROC_STATUS read_sbi_unau_header(uint32_t sbiAddress);
static IPROC_STATUS iproc_verify_sbi_authenticated_header(
			AUTHENTICATED_SBI_HEADER *sbiAuthHeader);

static IPROC_STATUS iproc_ecdsa_p256_verify(
			crypto_lib_handle *pHandle,
			uint8_t* pubKey,
			uint32_t hashLength,
			uint8_t* hash,
			uint8_t* Signature);

static IPROC_STATUS iproc_aes_ecb_decrypt(
			crypto_lib_handle *pHandle,
			uint8_t *pAESKey,
			uint32_t AESKeyLen,
			uint8_t *pMsg,
			uint32_t nMsgLen,
			uint8_t *outputCipherText);

static void *validate_aai(void);

#ifdef SBI_DEBUG
#define post_dbg	post_log
#else
#define post_dbg(A, ...)
#endif

#define err_handle(ERR)	do {		\
		post_log("Error: [%d] at %s:%u\n", ERR, __FUNCTION__, __LINE__);\
		post_log("Spinning!");	\
		while (1);		\
	} while (0)

struct _sotp
{
	struct {
		uint16_t    BRCMRevisionID;
		uint16_t    devSecurityConfig;
		uint16_t    ProductID;
		uint16_t    Reserved;
	} dev_cfg;
	uint32_t    SBLConfiguration;
	uint32_t    CustomerID;
	uint16_t    CustomerRevisionID;
	uint16_t    SBIRevisionID;
	uint8_t     kAES[OTP_SIZE_KAES];         /* 256 bits of AES key */
	uint8_t     kHMAC_SHA256[OTP_SIZE_KHMAC_SHA256]; /* 256 bits of HMAC-SHA256 key */
	uint8_t     DAUTH[OTP_SIZE_DAUTH];
} sotp;

struct s2bl_context {
	uint32_t	fOtpKAESValid;		/* If K-AES in S-OTP has valid value or not */
	uint32_t	fOtpKHMACSHA256Valid;	/* If K-HMACSHA256 in S-OTP has valid value or not */

	struct _sotp	sotp;
};

static struct s2bl_context s2bl = {0};

int sbi_main(void)
{
	void *pSbiEntry;

#ifdef SBI_DEBUG
	post_log("\n\n%s\n", SBI_VERSION_STRING);
	post_log("compiled by %s\n", LINUX_COMPILE_BY);
	post_log("compile host %s\n",LINUX_COMPILE_HOST);
	post_log("compiler %s\n",LINUX_COMPILER);
#endif

	s2bl.fOtpKAESValid = IPROC_FALSE;
	s2bl.fOtpKHMACSHA256Valid = IPROC_FALSE;

	pSbiEntry = validate_aai();
	post_log("Image address = %08x\n", pSbiEntry);

	/* Jump to SBI */
	((void (*)(void))pSbiEntry)();

	while(1);
	return 0;
}

void unauthent_sbi_header_print(UNAUTHENT_SBI_HEADER *data)
{
	post_dbg("== UNAUTHENT_SBI_HEADER(%08x) ==\n", data);
 	post_dbg("Tag         = %08x\n", data->Tag);
 	post_dbg("Length      = %08x\n", data->Length);
 	post_dbg("Reserved    = %08x\n", data->Reserved);
 	post_dbg("crc         = %08x\n\n", data->crc);    
}

void authenticated_sbi_header_print(AUTHENTICATED_SBI_HEADER *data)
{
	post_dbg( "===== UNAUTHENT_SBI_HEADER(%08x) =====\n", data );
	post_dbg( "SBIConfiguration        = %08x\n", data->SBIConfiguration        );
	post_dbg( "SBIRevisionID           = %08x\n", data->SBIRevisionID           );
	post_dbg( "CustomerID              = %08x\n", data->CustomerID              );
	post_dbg( "ProductID               = %08x\n", data->ProductID               );
	post_dbg( "CustomerRevisionID      = %08x\n", data->CustomerRevisionID      );
	post_dbg( "COTOffset               = %08x\n", data->COTOffset               );
	post_dbg( "nCOTEntries             = %08x\n", data->nCOTEntries             );
	post_dbg( "COTLength               = %08x\n", data->COTLength               );
	post_dbg( "BootLoaderCodeOffset    = %08x\n", data->BootLoaderCodeOffset    );
	post_dbg( "BootLoaderCodeLength    = %08x\n", data->BootLoaderCodeLength    );
	post_dbg( "AuthenticationOffset    = %08x\n", data->AuthenticationOffset    );
	post_dbg( "ImageVerificationConfig = %08x\n", data->ImageVerificationConfig );
	post_dbg( "AuthLength              = %08x\n", data->AuthLength              );
	post_dbg( "Reserved1               = %08x\n", data->Reserved1               );
	post_dbg( "SBIUsage                = %08x\n", data->SBIUsage                );
	post_dbg( "Reserved2               = %08x\n", data->Reserved2               );
	post_dbg( "MagicNumber             = %08x\n\n", data->MagicNumber           );
}

void printn(void *data, int len)
{
    uint8_t *p = (uint8_t *)data;
    int i;
    
    post_log("Addr = %08x, len = %d\n", data, len);
    post_log("==================================\n");
    for (i = 0; i < len; i++)
        post_log("%02x ", p[i]);

    post_log("\n\n");
}

static void *validate_aai(void)
{
	uint8_t			cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
	struct _sotp		*pSOTP = NULL;
	uint8_t			outputHmacSha256[SHA256_HASH_SIZE] __attribute__((__aligned__(4)));
	uint8_t			sha256hash_DAUTH[SHA256_HASH_SIZE] __attribute__((__aligned__(4)));
	uint8_t			*sbiAuthPtr = NULL;   
	/* auth info (dsa/rsa signature or sha256 hash), located in the SBI image */
	uint32_t		sbiAddress = 0;    /* SBI Address */
	uint32_t		authType = IPROC_AUTH_TYPE_RSA;     
 	/* which type of auth will be used */
	uint32_t		authLen;
	/* 256 bits of AES key */
	uint8_t			Kaes[OTP_SIZE_KAES] __attribute__((__aligned__(4)));
	/* 256 bits of HMAC-SHA256 key */
	uint8_t			Khmac[OTP_SIZE_KHMAC_SHA256] __attribute__((__aligned__(4)));
	uint8_t			*pkAES = NULL;
	uint8_t 		*pkHMAC = NULL;
	uint32_t		AESKeySize = 0;
	/* chain of trust pub key, signature */
	uint8_t			*pChainKey = NULL, *pChainNextKey, *pChainSig;
	uint32_t		nChainDepth;
	uint32_t		status;
	SBI_HEADER		*pSbiHeader;
	/* the authenticated part of the SBI header */
	AUTHENTICATED_SBI_HEADER  *sbiAuthHeader; 
	UNAUTHENT_SBI_HEADER      *sbiUnauHeader;
	void *pSbiEntry = NULL;
	volatile uint32_t          i;
	uint32_t	keySize = RSA_2048_KEY_BYTES;
	uint32_t	maxAuthAttempts = 0, maxRecoveryAttempts = 0;
	uint32_t	NextKeySize = RSA_2048_KEY_BYTES;
	uint8_t		*COTPointer;
	uint32_t	CustomerID;		/* Customer ID */
	uint16_t	ProductID;		/* Product ID */
	uint16_t	CustomerRevisionID;	/* Customer Revision ID */
	uint16_t	BroadcomRevisionID;
	uint16_t	COTLength, SignatureVerificationConfig, SigSize;
	IPROC_PUB_KEYTYPE KeyType;
	uint32_t	ptrIndex;
	uint32_t crc;
	uint32_t bUseDBA = IPROC_FALSE, bUseAES = IPROC_FALSE, bUseAESsecond128bits = IPROC_FALSE, bAES_CBC_SelfTestDone = IPROC_FALSE;
	uint32_t bootSrc, bootAddr;
	uint32_t chainDepthCounter = 0;
	uint8_t* pDAUTH = NULL;

	uint32_t bPOR = IPROC_FALSE, bPINReset = IPROC_FALSE, bSleep = IPROC_FALSE, bDeepSleep = IPROC_FALSE;
	uint32_t bNonAB_Boot = IPROC_FALSE, bAB_Prod = IPROC_FALSE, bAB_FastAuth = IPROC_FALSE;
	uint32_t recoveryWDTimer = 0, authWDTimer = 0;
	uint32_t nCurrentFlashBlock = 0;
	
	uint32_t sbiAddress_nonAB = 0;
	uint16_t keyType_DAUTH = 0;
	uint16_t length_DAUTH = 0;

	uint32_t jumpAddr = 0;
	uint32_t expectedDAUTHlen = 0;

	post_dbg("In validate_aai()\n");

	bAB_Prod = IPROC_TRUE;

	/*----- Step 4: SOTP read out -----*/
	pSOTP = &s2bl.sotp;
	post_dbg("Before InitializeDeviceCfgAndCustomerID()\n");

	/* Read Section 4 of SOTP */
	status = InitializeDeviceCfgAndCustomerID();
	post_dbg("InitializeDeviceCfgAndCustomerID: status = %d\n", status);

	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);
	sbiAddress = IPROC_SBI_FLASH_ADDRESS ;

	post_dbg("SBI Address is %08x\n", sbiAddress);

	/* read in unauthenticated SBI header - CRC check is done later */
	status = read_sbi_unau_header(sbiAddress);
	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	pSbiHeader = (SBI_HEADER *)IPROC_SBI_LOAD_ADDRESS;
	sbiUnauHeader = &(pSbiHeader->unh);
	sbiAuthHeader = &(pSbiHeader->sbi);

	unauthent_sbi_header_print(sbiUnauHeader);

	/* Checks for the unauthenticated header */
	if(sbiUnauHeader->Tag != SBI_TAG)
		err_handle(IPROC_STATUS_SBI_INVALID_SBI_TAG);

	/* Check CRC on SBI header */
	crc = nvm_crc32(0xFFFFFFFF,(uint8_t*)sbiUnauHeader,
			sizeof(UNAUTHENT_SBI_HEADER) - sizeof(uint32_t));
	if(crc != sbiUnauHeader->crc)
		err_handle(IPROC_STATUS_SBI_UNAUTH_HDR_CRC_FAIL);

	crypto_init((crypto_lib_handle *)&cryptoHandle);

	post_dbg("Before InitializeSOTP()\n");

	// Complete SOTP Initialization
	status = InitializeSOTP();
	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);
	post_dbg("After InitializeSOTP()\n");

	// Check whether the CustomerID in SOTP is valid
	if((pSOTP->CustomerID & CID_TYPE_MASK) != CID_TYPE_PROD)
		err_handle(IPROC_STATUS_SOTP_CUST_ID_INVALID);

	/*----- Load SBI -----*/
	/* load in authenticated SBI header */
	status = iproc_read_to_scratch(sbiAddress + sizeof(UNAUTHENT_SBI_HEADER),
				       (uint8_t *)sbiAuthHeader,
				       sizeof(AUTHENTICATED_SBI_HEADER));
	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	authenticated_sbi_header_print(sbiAuthHeader);

	/* Verify authenticated header */
	status = iproc_verify_sbi_authenticated_header(sbiAuthHeader);
	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	post_dbg("iproc_verify_sbi_authenticated_header returns: %08x\n",status);
	post_dbg("sbiAuthHeader->SBIConfiguration is %08x\n", sbiAuthHeader->SBIConfiguration);

	/* Check if the SBI configuration is supported by the device. If unsupported configuration, error out */
	if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_SYMMETRIC) == IPROC_SBI_CONFIG_SYMMETRIC) /* symmetric authentication */
	{
   		if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_AUTHTYPE_SYMMETRIC) !=  OTP_DCFG_AUTHTYPE_SYMMETRIC)
			err_handle(IPROC_STATUS_BTROM_SYM_AUTH_DISABLED);

		if (s2bl.fOtpKHMACSHA256Valid!= IPROC_TRUE) /*K-HMAC is not valid*/
	      		err_handle(IPROC_STATUS_BTROM_SYM_VERIFY_NOTALLOW);

		authType = IPROC_AUTH_TYPE_HMAC_SHA256;
	}
	else if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_RSA) == IPROC_SBI_CONFIG_RSA) /*RSA authentication*/
	{
		if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_AUTHTYPE_RSA) !=  OTP_DCFG_AUTHTYPE_RSA)
			err_handle(IPROC_STATUS_BTROM_RSA_AUTH_DISABLED);

   		authType = IPROC_AUTH_TYPE_RSA;
	}
	else if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_ECDSA) == IPROC_SBI_CONFIG_ECDSA) /*ECDSA authentication*/
	{
		if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_AUTHTYPE_ECDSA) !=  OTP_DCFG_AUTHTYPE_ECDSA)
			err_handle(IPROC_STATUS_BTROM_ECDSA_AUTH_DISABLED);

   		authType = IPROC_AUTH_TYPE_ECDSA;
	} else {
		err_handle(IPROC_STATUS_SBI_INVALID_VERIFICATION_CONFIG_SIGNATURE_SCHEME);
	}

	/* Check if the bootimage is encrypted and if the device supports encryption */
	if ( ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS) ||
		 ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS) )
	{
		if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_ENCR_AES128) !=  OTP_DCFG_ENCR_AES128)
			err_handle(IPROC_STATUS_BTROM_AES128_ENCR_DISABLED);
	}

	if (((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_256_ENCRYPTION))
	{
		if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_ENCR_AES256) !=  OTP_DCFG_ENCR_AES256)
			err_handle(IPROC_STATUS_BTROM_AES256_ENCR_DISABLED);
	}

	if ( ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS) ||
		 ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS) ||
		 ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_256_ENCRYPTION) )
	{
	    if (s2bl.fOtpKAESValid != IPROC_TRUE)
		     err_handle(IPROC_STATUS_SOTP_KAES_INVALID);

		bUseAES = IPROC_TRUE;

		if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_256_ENCRYPTION)
		{
			post_dbg("IPROC_SBI_CONFIG_AES_256_ENCRYPTION\n");
			AESKeySize = IPROC_AES_256_KEY_BYTES;
		}
		else if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS)
		{
			post_dbg("IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS\n");
			AESKeySize = IPROC_AES_128_KEY_BYTES;
		}
		else if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS)
		{
			post_dbg("IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS\n");
			bUseAESsecond128bits = IPROC_TRUE;
			AESKeySize = IPROC_AES_128_KEY_BYTES;
		}

		if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH) == IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH)
		{
			if ((pSOTP->dev_cfg.devSecurityConfig & OTP_DCFG_DECRYPT_BEFORE_AUTH) !=  OTP_DCFG_DECRYPT_BEFORE_AUTH)
			{
				err_handle(IPROC_STATUS_BTROM_DBA_DISABLED);
			}
			bUseDBA = IPROC_TRUE;
		}
	}

	/* load the SBI image following the header */
#ifndef V_QFLASH // Verify AAI in Flash
	status = iproc_read_to_scratch(sbiAddress + sizeof(SBI_HEADER),
		(uint8_t *)((uint32_t)pSbiHeader + sizeof(SBI_HEADER)),
		sbiAuthHeader->AuthenticationOffset - sizeof(SBI_HEADER));
#else
	status = iproc_read_to_scratch(sbiAddress + sizeof(SBI_HEADER),
		(uint8_t *)((uint32_t)pSbiHeader + sizeof(SBI_HEADER)),
		sbiAuthHeader->BootLoaderCodeOffset - sizeof(SBI_HEADER));
#endif	

	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	authLen = sbiAuthHeader->AuthLength;

	/* read in SBI Auth */
#ifndef V_QFLASH // Verify AAI in Flash
	sbiAuthPtr = (uint8_t *)pSbiHeader + sbiAuthHeader->AuthenticationOffset;
	if (!((sbiAuthPtr >= (uint8_t *)IPROC_SBI_LOAD_ADDRESS) ))
	{
		err_handle( IPROC_STATUS_BTROM_BTSRC_BOUNDARY_CHECK_FAIL);
	}
#else
	sbiAuthPtr = (uint8_t *)pSbiHeader + sbiAuthHeader->BootLoaderCodeOffset;
#endif

	post_dbg("authLen = %08x, sbiAuthPtr = %08x\n", authLen, sbiAuthPtr);

	status = iproc_read_to_scratch(sbiAddress + sbiAuthHeader->AuthenticationOffset,
	 			       sbiAuthPtr, authLen);
	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	if(bUseAES == IPROC_TRUE)
	{
		if(bUseAESsecond128bits == IPROC_TRUE)
		{
			post_dbg("IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS\n");
			pkAES = &pSOTP->kAES[IPROC_AES_128_KEY_BYTES];	// Use the second 128 bits of kAES as the AES 128 bit key
		}
		else
		{
			pkAES = &pSOTP->kAES[0];
		}

	}

	if(authType == IPROC_AUTH_TYPE_HMAC_SHA256)
	{
		pkHMAC = &pSOTP->kHMAC_SHA256[0];
	}
	else
	{
		// use HMAC-fixed
		pkHMAC = (uint8_t*)&device_keys.Khmac_fixed[0];
	}

	memset(outputHmacSha256, 0, sizeof(outputHmacSha256));
#ifndef V_QFLASH // Verify AAI in Flash	
	status = iproc_aes_sha256((crypto_lib_handle *)&cryptoHandle,
				  pkAES,
				  AESKeySize,
				  pkHMAC,
				  pSbiHeader,
				  outputHmacSha256,
				  BCM_SCAPI_CIPHER_MODE_DECRYPT,
				  (bUseDBA == IPROC_TRUE)?
					BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH:
					BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT);
#else
	status = iproc_aes_sha256((crypto_lib_handle *)&cryptoHandle,
				  pkAES, 
				  AESKeySize, 
				  pkHMAC, 
				  (SBI_HEADER *)sbiAddress, 
				  outputHmacSha256, 
				  BCM_SCAPI_CIPHER_MODE_DECRYPT,
				  (bUseDBA == IPROC_TRUE)?
					BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH:
					BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT);
#endif

	if (status != IPROC_STATUS_OK)
		err_handle((IPROC_STATUS)status);

	post_dbg("*** outputHmacSha256 ***\n");
	for(i=0; i< SHA256_HASH_SIZE; i+=4)
		post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(outputHmacSha256 + i), *(outputHmacSha256 + i + 1), *(outputHmacSha256 + i + 2), *(outputHmacSha256 + i + 3));


	post_dbg("*** sbiAuthPtr ***\n");
	for(i=0; i< SHA256_HASH_SIZE; i+=4)
		post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(sbiAuthPtr + i), *(sbiAuthPtr + i + 1), *(sbiAuthPtr + i + 2), *(sbiAuthPtr + i + 3));

	if(authType == IPROC_AUTH_TYPE_HMAC_SHA256)
	{
		post_dbg("Symmetric verification\n");

		if ((sbiAuthHeader->ImageVerificationConfig & IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256) != IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256)
			err_handle(IPROC_STATUS_SBI_INVALID_VERIFICATION_CONFIG_HMAC_HASHALG);

		if(authLen != SHA256_HASH_SIZE)
			err_handle(IPROC_STATUS_SBI_INVALID_SIZE_HMAC_HASHALG);

		if(memcmp(outputHmacSha256, sbiAuthPtr, authLen))
		{
			post_dbg("Symmetric verification failed\n");
			err_handle(IPROC_STATUS_SBI_HMAC_SHA256_NOTMATCH);
		}
		else
		{
			post_dbg("Symmetric verification successful\n");
		}
	}
	else if ( (authType == IPROC_AUTH_TYPE_RSA) || (authType == IPROC_AUTH_TYPE_ECDSA))
	{
		post_dbg("sbiAuthHeader->ImageVerificationConfig = %08x\n", sbiAuthHeader->ImageVerificationConfig);
		post_dbg("IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256 = %08x\n", IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256);

		// Asymmetric authentication
		if( (sbiAuthHeader->ImageVerificationConfig & IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256) != IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256)
			err_handle(IPROC_STATUS_SBI_INVALID_VERIFICATION_CONFIG_SIGNATURE_HASHALG);

		nChainDepth = sbiAuthHeader->nCOTEntries;
		if (nChainDepth == 0)
		{
			/* chain depth = 0: no chain, this is not allowed */
			err_handle(IPROC_STATUS_SBI_NO_CHAIN);
		}

		COTPointer = (uint8_t *)((uint32_t)pSbiHeader + sbiAuthHeader->COTOffset);

		post_dbg("COTPointer = %08x\n", COTPointer);
		post_dbg("sbiAuthHeader->ImageVerificationConfig = %08x\n", sbiAuthHeader->ImageVerificationConfig);

		if (authType == IPROC_AUTH_TYPE_RSA)
		{
     		if ((sbiAuthHeader->ImageVerificationConfig & IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5) != IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5)
				err_handle(IPROC_STATUS_SBI_INVALID_VERIFICATION_CONFIG_SIGNATURE_SCHEME);
		}
		else if (authType == IPROC_AUTH_TYPE_ECDSA)
		{
     		if ((sbiAuthHeader->ImageVerificationConfig & IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256) != IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256)
				err_handle(IPROC_STATUS_SBI_INVALID_VERIFICATION_CONFIG_SIGNATURE_SCHEME);
		}

		post_dbg("Using DAUTH\n");
		// This check is done again to prevent against fault attacks
		if(bAB_Prod == IPROC_TRUE) {
			// Check that the customerID is of type CID_TYPE_PROD
			if ((pSOTP->CustomerID & CID_TYPE_MASK) != CID_TYPE_PROD)
				err_handle(IPROC_STATUS_SOTP_CUST_ID_INVALID);
		} else
			err_handle(IPROC_STATUS_SBL_INVALID_ENC_DEVTYPE);

		pDAUTH = (uint8_t *)((uint8_t*)sbiAuthHeader + sizeof(AUTHENTICATED_SBI_HEADER));
		keyType_DAUTH = (*(uint16_t*)pDAUTH);

		if (authType == IPROC_AUTH_TYPE_RSA)
		{
			// Check that the DAUTH key type is RSA
			if((keyType_DAUTH & IPROC_SBI_KEY_ALG_RSA) != IPROC_SBI_KEY_ALG_RSA)
				err_handle(IPROC_STATUS_SBI_INVALID_DAUTH_KEYALGORITHM);

			if((keyType_DAUTH & IPROC_SBI_KEY_INFO_RSA2048) == IPROC_SBI_KEY_INFO_RSA2048)
			{
				keySize = IPROC_RSA_2048_KEY_BYTES;
				expectedDAUTHlen = IPROC_RSA_2048_KEY_BYTES + 3*sizeof(uint32_t);
			}
			else if((keyType_DAUTH & IPROC_SBI_KEY_INFO_RSA4096) == IPROC_SBI_KEY_INFO_RSA4096)
			{
				keySize = IPROC_RSA_4096_KEY_BYTES;
				expectedDAUTHlen = IPROC_RSA_4096_KEY_BYTES + IPROC_RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t);
			}
			else
				err_handle(IPROC_STATUS_SBI_INVALID_DAUTH_KEYINFO);
		}
		else if (authType == IPROC_AUTH_TYPE_ECDSA)
		{
			// Check that the DAUTH key type is EC
			if((keyType_DAUTH & IPROC_SBI_KEY_ALG_EC) != IPROC_SBI_KEY_ALG_EC)
				err_handle(IPROC_STATUS_SBI_INVALID_DAUTH_KEYALGORITHM);

			if((keyType_DAUTH & IPROC_SBI_KEY_INFO_CURVE_P256) == IPROC_SBI_KEY_INFO_CURVE_P256)
			{
				keySize = IPROC_EC_P256_KEY_BYTES;
				expectedDAUTHlen = IPROC_EC_P256_KEY_BYTES + 3*sizeof(uint32_t);
			}
			else
				err_handle(IPROC_STATUS_SBI_INVALID_DAUTH_KEYINFO);
		}

		length_DAUTH =  *(uint16_t*)((uint8_t*)pDAUTH + sizeof(uint16_t));

		post_dbg("expectedDAUTHlen:0x%08x length_DAUTH:0x%08x\n", expectedDAUTHlen, length_DAUTH);

		if (length_DAUTH != expectedDAUTHlen)
			err_handle(IPROC_STATUS_SBL_DAUTH_LENGTH_DOES_NOT_MATCH_EXPECTED_LENGTH); 

		// calculate the hash of the DAUTH key in SBI image and compare it with the value in SOTP
		// if the values match, set DAUTH as ROT;
		// else call error handling IPROC_STATUS_SBI_DAUTH_HASH_NOMATCH
		post_dbg("Calling crypto_symmetric_hmac_sha256\n");

		status = crypto_symmetric_hmac_sha256(
				(crypto_lib_handle *)&cryptoHandle,
				pDAUTH,
				length_DAUTH,
				NULL,
				0,
				sha256hash_DAUTH,
				TRUE);
		if (status != IPROC_STATUS_OK)
			err_handle((IPROC_STATUS)status);

		post_dbg("*** sha256hash_DAUTH ***\n");
		for(i=0; i< SHA256_HASH_SIZE; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(sha256hash_DAUTH + i), *(sha256hash_DAUTH + i + 1), *(sha256hash_DAUTH + i + 2), *(sha256hash_DAUTH + i + 3));

		if(memcmp(pSOTP->DAUTH, sha256hash_DAUTH, SHA256_HASH_SIZE))
			err_handle(IPROC_STATUS_SBI_DAUTH_HASH_NOTMATCH);

		post_dbg("Valid DAUTH found\n");

		CustomerID = (*(uint32_t *)((uint8_t*)pDAUTH + length_DAUTH - (2*sizeof(uint32_t))));

		post_dbg("DAUTH CustomerID = %08x\n", CustomerID);

		// Check that the Customer ID in the DAUTH is equal to pSOTP->CustomerID
		if(CustomerID != pSOTP->CustomerID)
			err_handle(IPROC_STATUS_SBI_CUST_ID_NOTMATCH);

		ProductID = (*(uint16_t *)((uint8_t*)pDAUTH + length_DAUTH - sizeof(uint32_t)));

		post_dbg("DAUTH ProductID = %08x\n", ProductID);

		// Check that the COT binding ID in the DAUTH is equal to pSOTP->dev_cfg.ProductID
		if(ProductID != pSOTP->dev_cfg.ProductID)
			err_handle(IPROC_STATUS_SBI_PRODUCT_ID_NOTMATCH);

		BroadcomRevisionID = (*(uint16_t *)((uint8_t*)pDAUTH + length_DAUTH - sizeof(uint16_t)));

		post_dbg("DAUTH BroadcomRevisionID = %08x\n", BroadcomRevisionID);

		// Check that the COT binding ID in the DAUTH is equal to pSOTP->dev_cfg.ProductID
		if(BroadcomRevisionID != pSOTP->dev_cfg.BRCMRevisionID)
			err_handle(IPROC_STATUS_SBI_BRCM_REV_ID_NOTMATCH);

		if (authType == IPROC_AUTH_TYPE_RSA)
		{
			pChainKey = (uint8_t *)(pDAUTH + sizeof(uint32_t));
			bcm_int2bignum((uint32_t*)pChainKey, keySize);
		}
		else if (authType == IPROC_AUTH_TYPE_ECDSA)
		{
			pChainKey = (uint8_t *)(pDAUTH + sizeof(uint32_t));
			bcm_int2bignum((uint32_t*)pChainKey, ECC_SIZE_P256);
			bcm_int2bignum((uint32_t*)((uint8_t*)pChainKey+ECC_SIZE_P256), ECC_SIZE_P256);
		}


		post_dbg("COTPointer:0x%08x pDauth:0x%08x length_DAUTH:0x%08x\n", COTPointer, pDAUTH, length_DAUTH);

		if (COTPointer != pDAUTH + length_DAUTH)
			err_handle(IPROC_STATUS_SBL_INVALID_COT_OFFSET); 

		for (chainDepthCounter = 0; chainDepthCounter < nChainDepth; chainDepthCounter++)
		{
			KeyType = (*(uint16_t *)COTPointer);

			post_dbg("KeyType = %08x\n", KeyType);

			COTLength = (*(uint16_t *)(COTPointer + sizeof(uint16_t)));

			post_dbg("COTLength = %08x\n", COTLength);

			ptrIndex = sizeof(uint32_t);
			pChainNextKey = COTPointer + ptrIndex;

			post_dbg("pChainNextKey = %08x\n", pChainNextKey);

			if(KeyType == IPROC_SBI_RSA2048)
			{
				NextKeySize = IPROC_RSA_2048_KEY_BYTES;
			}
			else if(KeyType == IPROC_SBI_RSA4096)
			{
				NextKeySize = IPROC_RSA_4096_KEY_BYTES;
			}
			else if(KeyType == IPROC_SBI_EC_P256)
			{
				NextKeySize = IPROC_EC_P256_KEY_BYTES;
			}
			else
			{
				err_handle(IPROC_INVALID_KEY_SIZE);
			}
			ptrIndex += NextKeySize;

			if(KeyType == IPROC_SBI_RSA4096)
				ptrIndex += IPROC_RSA_4096_MONT_CTX_BYTES;

			CustomerID = (*(uint32_t *)(COTPointer + ptrIndex));

			post_dbg("CustomerID = %08x\n", CustomerID);

			// Check that the Customer ID in the COTEntry is equal to pSOTP->CustomerID
			if(CustomerID != pSOTP->CustomerID)
				err_handle(IPROC_STATUS_SBI_CUST_ID_NOTMATCH);

			ptrIndex += sizeof(uint32_t);
			ProductID = (*(uint16_t *)(COTPointer + ptrIndex));

			post_dbg("ProductID = %08x\n", ProductID);

			// Check that the Product ID in the COTEntry is equal to pSOTP->dev_cfg.ProductID
			if(ProductID != pSOTP->dev_cfg.ProductID)
				err_handle(IPROC_STATUS_SBI_PRODUCT_ID_NOTMATCH);

			ptrIndex += sizeof(uint16_t);
			CustomerRevisionID = (*(uint16_t *)(COTPointer + ptrIndex));

			post_dbg("CustomerRevisionID = %08x\n", CustomerRevisionID);

			if(CustomerRevisionID != pSOTP->CustomerRevisionID)
				err_handle(IPROC_STATUS_SBI_CUST_REV_NOTMATCH);

			ptrIndex += sizeof(uint16_t);
			SignatureVerificationConfig = (*(uint16_t *)(COTPointer + ptrIndex));

			post_dbg("SignatureVerificationConfig = %08x\n", SignatureVerificationConfig);

			// Note: SignatureVerificationConfig will be used later to call the appropriate verification function for RSA when PSS mode is implemented
			ptrIndex += sizeof(uint16_t);
			SigSize = (*(uint16_t *)(COTPointer + ptrIndex));

			post_dbg("SigSize = %08x\n", SigSize);

			ptrIndex += sizeof(uint16_t);
			pChainSig = COTPointer + ptrIndex;

			post_dbg("pChainSig = %08x\n", pChainSig);

			if( (SigSize != IPROC_RSA_2048_KEY_BYTES) && (SigSize != IPROC_RSA_4096_KEY_BYTES) && (SigSize != IPROC_EC_P256_KEY_BYTES) )
			{
				err_handle( IPROC_INVALID_SIGNATURE_SIZE);
			}

			if (authType == IPROC_AUTH_TYPE_RSA)
			{
				post_dbg("Calling bcm_int2bignum\n" );
				bcm_int2bignum((uint32_t*)pChainSig, SigSize);
			}
			else if (authType == IPROC_AUTH_TYPE_ECDSA)
			{
				post_dbg("Calling bcm_int2bignum (ECDSA)\n" );
				bcm_int2bignum((uint32_t*)pChainSig, ECC_SIZE_P256);
				bcm_int2bignum((uint32_t*)((uint8_t*)pChainSig+ECC_SIZE_P256), ECC_SIZE_P256);
			}

			post_dbg("***Signature - chainDepth: %d***\n", chainDepthCounter);
			//for(i=0; i< SigSize; i+=4)
			//	post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pChainSig + i), *(pChainSig + i + 1), *(pChainSig + i + 2), *(pChainSig + i + 3));
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pChainSig), *(pChainSig + 1), *(pChainSig + 2), *(pChainSig + 3));

			post_dbg("***Message - chainDepth: %d***\n", chainDepthCounter);
			//for(i=0; i< ptrIndex; i+=4)
			//	post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(COTPointer + i), *(COTPointer + i + 1), *(COTPointer + i + 2), *(COTPointer + i + 3));
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(COTPointer), *(COTPointer + 1), *(COTPointer + 2), *(COTPointer + 3));

			if(chainDepthCounter > 0)
			{
				if (authType == IPROC_AUTH_TYPE_RSA)
				{
					bcm_int2bignum((uint32_t*)pChainKey, keySize);
				}
				else if (authType == IPROC_AUTH_TYPE_ECDSA)
				{
					bcm_int2bignum((uint32_t*)pChainKey, ECC_SIZE_P256);
					bcm_int2bignum((uint32_t*)((uint8_t*)pChainKey+ECC_SIZE_P256), ECC_SIZE_P256);
				}
			}

			post_dbg("***Key - chainDepth: %d ***\n", chainDepthCounter);
			//for(i=0; i< keySize; i+=4)
			//	post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pChainKey + i), *(pChainKey + i + 1), *(pChainKey + i + 2), *(pChainKey + i + 3));
			if (pChainKey)
				post_dbg("0x%x 0x%x 0x%x 0x%x \n",
					 *(pChainKey), *(pChainKey + 1),
					 *(pChainKey + 2), *(pChainKey + 3));

			if (authType == IPROC_AUTH_TYPE_RSA)
			{
				post_dbg("Calling crypto_pka_rsassa_pkcs1_v15_verify: SigSize = %08x, pChainSig = %08x, ptrIndex = %08x, COTPointer = %08x\n",  SigSize, pChainSig, ptrIndex, COTPointer);
				post_dbg("keySize = %08x, pChainKey = %08x\n", keySize, pChainKey);

				if(keySize == IPROC_RSA_4096_KEY_BYTES)
				{
					pChainKey += IPROC_RSA_4096_KEY_BYTES; // skip the key bytes and start from the montgomery context
					keySize = IPROC_RSA_4096_MONT_CTX_BYTES;
					post_dbg("RSA 4096 pChainKey = %08x keySize = %d\n", pChainKey, keySize);
				}

				status = crypto_pka_rsassa_pkcs1_v15_verify((crypto_lib_handle *)&cryptoHandle,
						keySize, (uint8_t *)pChainKey,/* modulus */
						4, (uint8_t *)&(device_keys.Kbase_Rsa_pubE),/* exponent */
						ptrIndex, COTPointer, /* message */
						SigSize, pChainSig,/* signature */
						BCM_HASH_ALG_SHA2_256);
			}
			else if (authType == IPROC_AUTH_TYPE_ECDSA)
			{
				post_dbg("Calling iproc_ecdsa_p256_verify: SigSize = %08x, pChainSig = %08x, ptrIndex = %08x, COTPointer = %08x\n",  SigSize, pChainSig, ptrIndex, COTPointer);
				post_dbg("keySize = %08x, pChainKey = %08x\n", keySize, pChainKey);

				status = iproc_ecdsa_p256_verify((crypto_lib_handle *)&cryptoHandle,
						 (uint8_t *)pChainKey,
						 ptrIndex, COTPointer,	/* message */
						 pChainSig);			/* signature */

				if(status == IPROC_STATUS_OK)
					post_dbg("iproc_ecdsa_p256_verify SUCCESSFUL\n");
			}
			if (status != IPROC_STATUS_OK)
				err_handle((IPROC_STATUS)status);


			COTPointer += ptrIndex + SigSize;
			ptrIndex = 0;
			pChainKey = pChainNextKey;
			keySize = NextKeySize;
		}

		if (authType == IPROC_AUTH_TYPE_RSA)
		{
			bcm_int2bignum((uint32_t*)pChainKey, keySize);
			bcm_int2bignum((uint32_t*)sbiAuthPtr, authLen);
		}
		else if (authType == IPROC_AUTH_TYPE_ECDSA)
		{
			bcm_int2bignum((uint32_t*)pChainKey, ECC_SIZE_P256);
			bcm_int2bignum((uint32_t*)((uint8_t*)pChainKey+ECC_SIZE_P256), ECC_SIZE_P256);

			bcm_int2bignum((uint32_t*)sbiAuthPtr, ECC_SIZE_P256);
			bcm_int2bignum((uint32_t*)((uint8_t*)sbiAuthPtr+ECC_SIZE_P256), ECC_SIZE_P256);
		}

		post_dbg("***Key - image***\n");
		post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pChainKey), *(pChainKey + 1), *(pChainKey + 2), *(pChainKey + 3));

		post_dbg("Calling crypto_pka_rsassa_pkcs1_v15_verify for image: authLen(SigSize) = %08x, sbiAuthPtr(Sig) = %08x, msgSize = %08x, msg (outputHmacSha256) = %08x\n", keySize, sbiAuthPtr, SHA256_HASH_SIZE, outputHmacSha256);
		post_dbg("keySize = %08x, pChainKey = %08x\n", keySize, pChainKey);

		if (authType == IPROC_AUTH_TYPE_RSA)
		{
			if(keySize == IPROC_RSA_4096_KEY_BYTES)
			{
				pChainKey += IPROC_RSA_4096_KEY_BYTES; // skip the key bytes and start from the montgomery context
				keySize = IPROC_RSA_4096_MONT_CTX_BYTES;
				post_dbg("RSA 4096 pChainKey = %08x keySize = %d\n", pChainKey, keySize);
			}

			status = crypto_pka_rsassa_pkcs1_v15_verify((crypto_lib_handle *)&cryptoHandle,
					 keySize, (uint8_t *)pChainKey,/* modulus */
					 4, (uint8_t *)&(device_keys.Kbase_Rsa_pubE),/* exponent */
					 SHA256_HASH_SIZE, outputHmacSha256,/* message */
					 authLen, sbiAuthPtr,
					 BCM_HASH_ALG_SHA2_256);

			post_dbg("crypto_pka_rsassa_pkcs1_v15_verify for image returns %d\n", status );
		}
		else if (authType == IPROC_AUTH_TYPE_ECDSA)
		{
			status = iproc_ecdsa_p256_verify((crypto_lib_handle *)&cryptoHandle,
					 (uint8_t *)pChainKey,
					 SHA256_HASH_SIZE, outputHmacSha256,	/* message */
					 sbiAuthPtr);			/* signature */

			post_dbg("iproc_ecdsa_p256_verify for image returns %d\n", status );
		}
		if (status != IPROC_STATUS_OK)
			err_handle((IPROC_STATUS)status);
	}

	post_dbg("Decrementing the fail count and stopping the WD Timer\n");

#ifndef V_QFLASH // Verify AAI in Flash
	pSbiEntry = (void *)((uint32_t)pSbiHeader + (uint32_t)sbiAuthHeader->BootLoaderCodeOffset);
#else
	pSbiEntry = (void *)((uint32_t)sbiAddress + (uint32_t)sbiAuthHeader->BootLoaderCodeOffset);
#endif

	post_dbg("pSbiEntry is %08x\n", pSbiEntry);

	memset((void*)pSOTP, 0, sizeof(*pSOTP));

	post_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	post_log("!!!!!!!!!!! Authentication Success !!!!!!!!!!!!\n");
	post_log("!!! If Secure Boot works then you should !!!!!!!\n");
	post_log("!!! See the \"Citadel Test AAI\" boot up !!!!!!!!\n");
	post_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	k_sleep(1000);post_log(".");
	k_sleep(1000);post_log(".");
	k_sleep(1000);post_log(".\n\n");

	return pSbiEntry;
}

/* Do hmac-sha256 auth. Also possibly do AES128 CBC decryption at the same time if it's encrypted
   For decryption, it will be done in-place, e.g. overwrite the encrypted payload.

   Since our hash output always follows crypto output, in case of in-place decryption, auth output
   will overwrite the 20 bytes after the encrypted payload. So need to store it somewhere first, then copy back.
*/
static IPROC_STATUS iproc_aes_sha256(crypto_lib_handle *pHandle, uint8_t *pAESKey, uint32_t AESKeyLen, uint8_t *pAuthKey,
				  SBI_HEADER * pSbiHeader, uint8_t *HashOutput,
				  BCM_SCAPI_CIPHER_MODE cipher_mode, BCM_SCAPI_CIPHER_ORDER cipher_order)
{
	IPROC_STATUS status = IPROC_STATUS_FAIL;
	BCM_SCAPI_STATUS scapi_status;
	uint8_t 	*pAESIV;
	uint8_t   	*pOutput;
	uint8_t    	tmpStore[SHA256_HASH_SIZE]; /* store the 32 bytes after the encrypted payload here */
	uint32_t   	nCryptoLen, nCryptoOffset;
	uint32_t 	authLen;
	AUTHENTICATED_SBI_HEADER  *sbiAuthHeader = &(pSbiHeader->sbi); /* the authenticated part of the SBI header */
	uint32_t i;

	authLen = sbiAuthHeader->AuthenticationOffset - sizeof(UNAUTHENT_SBI_HEADER);
	
	if (pAESKey)
	{
		post_dbg("pAESKey is present\n");
		// Obtain IV at sbiAuthHeader->BootLoaderCodeOffset - AES_IV_SIZE
		pAESIV = (uint8_t *)((uint32_t)pSbiHeader + sbiAuthHeader->BootLoaderCodeOffset - AES_IV_SIZE);

		post_dbg("*** pAESIV ***\n");
		for(i=0; i< AES_IV_SIZE; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pAESIV + i), *(pAESIV + i + 1), *(pAESIV + i + 2), *(pAESIV + i + 3));

		post_dbg("*** pAESKey ***\n");
		for(i=0; i< AESKeyLen; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pAESKey + i), *(pAESKey + i + 1), *(pAESKey + i + 2), *(pAESKey + i + 3));

		post_dbg("*** pAuthKey ***\n");
		for(i=0; i< SHA256_HASH_SIZE; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pAuthKey + i), *(pAuthKey + i + 1), *(pAuthKey + i + 2), *(pAuthKey + i + 3));

		nCryptoLen	= sbiAuthHeader->BootLoaderCodeLength;
		nCryptoOffset   = sbiAuthHeader->BootLoaderCodeOffset - sizeof(UNAUTHENT_SBI_HEADER);
		pOutput         = (uint8_t *)((uint32_t)pSbiHeader + sbiAuthHeader->BootLoaderCodeOffset); /* output will be in place decryption, following IV */

		post_dbg("*** nCryptoLen = %d ***\n", nCryptoLen);
		post_dbg("*** nCryptoOffset = %d ***\n", nCryptoOffset);
		post_dbg("*** pOutput before function call: %08x ***\n", pOutput);
		for(i=0; i< SHA256_HASH_SIZE; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pOutput + i), *(pOutput + i + 1), *(pOutput + i + 2), *(pOutput + i + 3));

		/* save the 32 bytes after the decryption output */
		if (cipher_mode == BCM_SCAPI_CIPHER_MODE_DECRYPT)
		{
			post_dbg("cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT\n");
			memcpy(tmpStore, pOutput + nCryptoLen, SHA256_HASH_SIZE);
			memset((void*)(pOutput + nCryptoLen), 0, SHA256_HASH_SIZE);
		}
	}
	else
	{
		post_dbg("pAESKey is NOT present\n");
		pAESIV          = 	NULL;
		nCryptoLen      = 	0;
		nCryptoOffset   = 	0;
		pOutput         = 	HashOutput; /* output will be only auth output */
	}

	post_dbg("Calling crypto_symmetric_aes_hmacsha256\n");
	scapi_status = crypto_symmetric_aes_hmacsha256(pHandle,(uint8_t *)sbiAuthHeader, authLen, pAESKey, AESKeyLen, pAuthKey, SHA256_KEY_SIZE, pAESIV,
					nCryptoLen, nCryptoOffset, authLen, /* auth offset */ 0,
					pOutput, cipher_mode, cipher_order);
	if (scapi_status == BCM_SCAPI_STATUS_OK)
		status = IPROC_STATUS_OK;

	if(nCryptoLen > 0)
	{
		post_dbg("*** pOutput after function call: %08x ***\n", pOutput);
		for(i=0; i< SHA256_HASH_SIZE; i+=4)
			post_dbg("0x%x 0x%x 0x%x 0x%x \n", *(pOutput + i), *(pOutput + i + 1), *(pOutput + i + 2), *(pOutput + i + 3));
	}

	if (pAESKey)
	{
		/* get the auth output */
		memcpy(HashOutput, (uint8_t*)(pOutput + nCryptoLen), SHA256_HASH_SIZE);

		/* restore the saved 20 bytes */
		if (cipher_mode == BCM_SCAPI_CIPHER_MODE_DECRYPT)
		{
			memcpy(pOutput + nCryptoLen, tmpStore, SHA256_HASH_SIZE);
		}
	}
	return status;

}

static IPROC_STATUS InitializeDeviceCfgAndCustomerID(void)
{
	struct sotp_bcm58202_dev_config devConfig = {0};
	struct _sotp *pSOTP = NULL;
	u32_t CustomerID = 0;
	u32_t SBLConfig = 0;
	sotp_bcm58202_status_t otpStatus = IPROC_OTP_INVALID;
	
	pSOTP = &s2bl.sotp;
	memset((void*)pSOTP, 0, sizeof(*pSOTP));

	// Reading SOTP Section 4
	otpStatus = sotp_read_dev_config(&devConfig);
	post_dbg("sotp_read_dev_config: otpStatus is %d\n", otpStatus);

	if(otpStatus == IPROC_OTP_VALID)
	{
		memcpy(&pSOTP->dev_cfg, &devConfig, sizeof(devConfig));
		post_dbg("pSOTP->dev_cfg.devSecurityConfig is %08x\n", pSOTP->dev_cfg.devSecurityConfig);
		post_dbg("pSOTP->dev_cfg.BRCMRevisionID is %08x\n", pSOTP->dev_cfg.BRCMRevisionID);
		post_dbg("pSOTP->dev_cfg.ProductID is %08x\n", pSOTP->dev_cfg.ProductID);
	}
	else
	{
		// Clear all the SOTP values
		memset((void*)pSOTP, 0, sizeof(*pSOTP));
		return IPROC_STATUS_SOTP_INVALID;
	}

	// Reading SBL Configuration - This is read from Section 4 for AB_PENDING and from Section 5 after customization
	otpStatus = sotp_read_sbl_config(&SBLConfig);
	post_dbg("sotp_read_dev_config: otpStatus is %d\n", otpStatus);
	post_dbg("SBLConfig is %08x\n", SBLConfig);

	if(otpStatus == IPROC_OTP_VALID)
	{
		memcpy(&pSOTP->SBLConfiguration, &SBLConfig, sizeof(uint32_t));
		post_dbg("pSOTP->SBLConfiguration is %08x\n", pSOTP->SBLConfiguration);
	}
	else
	{
		// Clear all the SOTP values
		memset((void*)pSOTP, 0, sizeof(*pSOTP));
		return IPROC_STATUS_SOTP_INVALID;
	}

	otpStatus = sotp_read_customer_id (&CustomerID);
	post_dbg("sotp_read_customer_id: otpStatus is %d\n", otpStatus);

	if(otpStatus == IPROC_OTP_VALID)
	{
		memcpy(&pSOTP->CustomerID, &CustomerID, sizeof(uint32_t));
		post_dbg("pSOTP->CustomerID is %08x\n", pSOTP->CustomerID);
	}
	else
	{
		// Clear all the SOTP values
		memset((void*)pSOTP, 0, sizeof(*pSOTP));
		return IPROC_STATUS_SOTP_INVALID;
	}

	return IPROC_STATUS_OK;
}

static IPROC_STATUS InitializeSOTP(void)
{
	uint16_t keySize = 0;
	struct _sotp *pSOTP = NULL;
	sotp_bcm58202_status_t otpStatus = IPROC_OTP_INVALID;
	uint16_t CustomerRevisionID = 0;
	uint16_t SBIRevisionID = 0;
	
	pSOTP = &s2bl.sotp;

	// For AB pending state and MFG_DEBUG mode, this function call is expected to return IPROC_OTP_VALID with value 0 for the RevisionIDs
	otpStatus = sotp_read_customer_config(&CustomerRevisionID, &SBIRevisionID);
	post_dbg("sotp_read_customer_config: otpStatus is %d\n", otpStatus);

	if(otpStatus == IPROC_OTP_VALID)
	{
		memcpy(&pSOTP->CustomerRevisionID, &CustomerRevisionID, sizeof(uint16_t));
		post_dbg("pSOTP->CustomerRevisionID is %08x\n", pSOTP->CustomerRevisionID);

		memcpy(&pSOTP->SBIRevisionID, &SBIRevisionID, sizeof(uint16_t));
		post_dbg("pSOTP->SBIRevisionID is %08x\n", pSOTP->SBIRevisionID);
	}
	else
	{
		goto error_handler;
	}

	// The reading of the keys below are optional, hence the function doesnt return on failure
	otpStatus = sotp_read_key(&pSOTP->kHMAC_SHA256[0], &keySize, OTPKey_HMAC_SHA256);
	post_dbg("sotp_read_key (HMAC_SHA256): otpStatus is %d\n", otpStatus);

	if((otpStatus == IPROC_OTP_VALID) && (keySize == OTP_SIZE_KHMAC_SHA256))
		s2bl.fOtpKHMACSHA256Valid = IPROC_TRUE;
	else
		s2bl.fOtpKHMACSHA256Valid = IPROC_FALSE;

	otpStatus = sotp_read_key(&pSOTP->kAES[0], &keySize, OTPKey_AES);
	post_dbg("sotp_read_key (AES): otpStatus is %d\n", otpStatus);

	if((otpStatus == IPROC_OTP_VALID) && (keySize == OTP_SIZE_KAES))
		s2bl.fOtpKAESValid = IPROC_TRUE;
	else
		s2bl.fOtpKAESValid = IPROC_FALSE;

	otpStatus = sotp_read_key(&pSOTP->DAUTH[0], &keySize, OTPKey_DAUTH);
	post_dbg("sotp_read_key (DAUTH): otpStatus is %d\n", otpStatus);

	return IPROC_STATUS_OK;

error_handler:
	// Clear all the SOTP values
	memset((void*)pSOTP, 0, sizeof(*pSOTP));
	return IPROC_STATUS_SOTP_INVALID;
}


static IPROC_STATUS iproc_read_to_scratch(uint32_t address, uint8_t *data, uint32_t dataLen)
{
	/* Boundary check, iProc has 256K scratch */
	post_dbg("address to read from: %08x, address to write: %08x, no. of bytes copied: %d\n", address, data, dataLen);

	/* Make sure start address falls within SRAM Landing Zone */
	if ( data < (uint8_t *)IPROC_SBI_LOAD_ADDRESS ) {
		/* Start address falls outside of SRAM landing zone */
		return IPROC_STATUS_BTROM_BTSRC_BOUNDARY_CHECK_FAIL;
	}

	if ((uint8_t *)address != data)
	{
		memcpy(data, (uint32_t *)address, dataLen);
	}

	return IPROC_STATUS_OK;
}

static IPROC_STATUS read_sbi_unau_header(uint32_t sbiAddress)
{
	IPROC_STATUS status = IPROC_STATUS_FAIL;

	/* see if the vector is word aligned */
	if (sbiAddress & 0x3)
	{
		return (IPROC_STATUS_SBI_STARTVEC_NOTALIGN);
	}

	status = iproc_read_to_scratch(sbiAddress,	
			(uint8_t *)IPROC_SBI_LOAD_ADDRESS,
			sizeof(UNAUTHENT_SBI_HEADER));

	return status;
}

static IPROC_STATUS iproc_verify_sbi_authenticated_header (AUTHENTICATED_SBI_HEADER *sbiAuthHeader)
{
	struct _sotp *pSOTP = NULL;

	pSOTP = &s2bl.sotp;

	if(sbiAuthHeader->SBIRevisionID < pSOTP->SBIRevisionID)
	{
		post_dbg("sbiAuthHeader->SBIRevisionID: %08x doesn't match pSOTP->SBIRevisionID: %08x\n", sbiAuthHeader->SBIRevisionID, pSOTP->SBIRevisionID);
		return IPROC_STATUS_SBI_SBIREVID_NOTMATCH;
	}

	if(sbiAuthHeader->CustomerID != pSOTP->CustomerID)
	{
		post_dbg("sbiAuthHeader->CustomerID: %08x doesn't match pSOTP->CustomerID: %08x\n", sbiAuthHeader->CustomerID, pSOTP->CustomerID);
		return IPROC_STATUS_SBI_CUST_ID_NOTMATCH;
	}

	if(sbiAuthHeader->CustomerRevisionID != pSOTP->CustomerRevisionID)
	{
		post_dbg("sbiAuthHeader->CustomerRevisionID: %08x doesn't match pSOTP->CustomerRevisionID: %08x\n", sbiAuthHeader->CustomerRevisionID, pSOTP->CustomerRevisionID);
		return IPROC_STATUS_SBI_CUST_REV_NOTMATCH;
	}

	if(sbiAuthHeader->ProductID != pSOTP->dev_cfg.ProductID)
	{
		post_dbg("sbiAuthHeader->ProductID: %08x doesn't match pSOTP->dev_cfg.ProductID: %08x\n", sbiAuthHeader->ProductID, pSOTP->dev_cfg.ProductID);
		return IPROC_STATUS_SBI_PRODUCT_ID_NOTMATCH;
	}

	if (! ( (sbiAuthHeader->AuthLength == IPROC_RSA_4096_KEY_BYTES) ||		/*RSA 4096*/
		    (sbiAuthHeader->AuthLength == IPROC_RSA_2048_KEY_BYTES) ||		/*RSA 2048*/
			(sbiAuthHeader->AuthLength == IPROC_EC_P256_KEY_BYTES)  ||		/*ECDSA P-256*/
			(sbiAuthHeader->AuthLength == IPROC_HMAC_SHA256_KEY_BYTES )))	/*HMAC SHA-256*/
	{
		post_dbg("sbiAuthHeader->AuthLength: %08x\n", sbiAuthHeader->AuthLength);
		return IPROC_STATUS_AUTH_HEADER_VERIFY_FAILED_AUTHLENGTH;
	}

	/* Number of COT Entries has to be either 0 or at most 3 */
	if (sbiAuthHeader->nCOTEntries > 0x3)
	{
		return IPROC_STATUS_AUTH_HEADER_VERIFY_FAILED_NCOTENTRIES;
	}

	if( ( (sbiAuthHeader->BootLoaderCodeOffset < (sizeof(UNAUTHENT_SBI_HEADER ) + sizeof(AUTHENTICATED_SBI_HEADER ) ) ) ) )
	{
		post_dbg("sbiAuthHeader->BootLoaderCodeOffset: %08x\n", sbiAuthHeader->BootLoaderCodeOffset);
		return IPROC_STATUS_AUTH_HEADER_VERIFY_FAILED_BOOTLOADEROFFSET;
	}

	if(sbiAuthHeader->COTOffset > 0 ) {
		if( ( ( (sbiAuthHeader->COTOffset + sbiAuthHeader->COTLength) >  sbiAuthHeader->BootLoaderCodeOffset) ) ) {
			post_dbg("sbiAuthHeader->COTOffset: %08x\n", sbiAuthHeader->COTOffset);
			return IPROC_STATUS_AUTH_HEADER_VERIFY_FAILED_COTOFFSET;
        	}
	}


	if( sbiAuthHeader->AuthenticationOffset < sbiAuthHeader->COTOffset )
	{
		post_dbg("sbiAuthHeader->AuthenticationOffset: %08x\n", sbiAuthHeader->AuthenticationOffset);
		return IPROC_STATUS_AUTH_HEADER_VERIFY_FAILED_AUTHENTICATIONOFFSET;
	}

	if(sbiAuthHeader->SBIUsage != SBI_USAGE_NONE)
	{
		post_dbg("sbiAuthHeader->SBIUsage: %08x doesn't match SBI_USAGE_NONE (%08x)\n", sbiAuthHeader->SBIUsage, SBI_USAGE_NONE);
		return IPROC_STATUS_SBI_CUST_REV_NOTMATCH;
	}

	/* Check magic number */
	if (sbiAuthHeader->MagicNumber != SBI_MAGIC_NUMBER)
	{
		return IPROC_STATUS_SBI_MAGIC_NOTMATCH_AUTH;
	}

	/* Check for valid SBI configuration and error out if invalid configuration */
	if ((sbiAuthHeader->SBIConfiguration & IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE) == IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE) // Authenticate in place
	{
		return IPROC_STATUS_SBICONFIG_INVALID_AUTHENTICATE_INPLACE;
	}

	return IPROC_STATUS_OK;
}

static IPROC_STATUS iproc_ecdsa_p256_verify(crypto_lib_handle *pHandle, uint8_t* pubKey, uint32_t messageLength, uint8_t* message, uint8_t* Signature)
{
        BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	uint8_t outputSha[SHA256_HASH_SIZE];
	uint32_t QZ[ECC_SIZE_P256>>2] = 
	{
		0x00000001, 0x00000000, 0x00000000, 0x00000000, 
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	/* The prime modulus */
	uint32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] = 
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 
		0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
	};
	/* Co-efficient defining the elliptic curve; In FIPS 186-3, the selection a = p-3 for the coefficient of x was made for reasons of efficiency */
	uint32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
	{
		0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
		0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
	};
	/* Co-efficient defining the elliptic curve */
	uint32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
	{
		0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
		0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
	};
	/* Order n */
	uint32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
	{
		0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
		0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
	};
	/* The base point x coordinate Gx */
	uint32_t ECC_P256DomainParams_Gx [ECC_SIZE_P256>>2] =
	{
		0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
		0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2
	};
	/* The base point y coordinate Gy */
	uint32_t ECC_P256DomainParams_Gy [ECC_SIZE_P256>>2] =
	{
		0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
		0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2
	};

   	status = crypto_symmetric_hmac_sha256(pHandle, message, messageLength, NULL, 0, outputSha, TRUE);
	if(status != BCM_SCAPI_STATUS_OK)
		return (IPROC_STATUS)status;

	bcm_int2bignum((uint32_t*)outputSha, SHA256_HASH_SIZE);

	return (IPROC_STATUS)crypto_pka_ecp_ecdsa_verify (pHandle, outputSha, SHA256_HASH_SIZE, 0 /* Curve Type = 0 (Prime field) */,
								(uint8_t *)ECC_P256DomainParams_Prime,
								(ECC_SIZE_P256)*8,
								(uint8_t *)ECC_P256DomainParams_a,
								(uint8_t *)ECC_P256DomainParams_b,
								(uint8_t *)ECC_P256DomainParams_n,
								(uint8_t *)ECC_P256DomainParams_Gx,
								(uint8_t *)ECC_P256DomainParams_Gy,
								pubKey, // pubKey_x
								(uint8_t *)(pubKey + ECC_SIZE_P256), // pubKey_y
								(uint8_t *)QZ,
								Signature,
								(uint8_t *)(Signature + ECC_SIZE_P256),
                  				NULL, NULL);
}

static IPROC_STATUS iproc_aes_ecb_decrypt(crypto_lib_handle *pHandle,
			uint8_t *pAESKey,
			uint32_t AESKeyLen,
			uint8_t *pMsg,
			uint32_t nMsgLen,
			uint8_t *outputCipherText)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_CRYPTO_ERROR;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;

	if(pAESKey == NULL)
		return IPROC_INVALID_KEY;

	cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_DECRYPT;
	if(AESKeyLen == AES_128_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128; 		/* encryption algorithm */
	else if (AESKeyLen == AES_256_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256; 		/* encryption algorithm */
	else
		return IPROC_INVALID_KEY_SIZE;

	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	// The following initializations are not relevant since auth key is NULL
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;

	status = crypto_smau_cipher_init(pHandle,
			                 &cmd,
					 pAESKey,
					 NULL,
					 0,
					 &ctx);

	if(status != BCM_SCAPI_STATUS_OK)
		return (IPROC_STATUS)status;

	status = crypto_smau_cipher_start(pHandle, pMsg, nMsgLen,
					  NULL, nMsgLen, 0, 0, 0,
					  outputCipherText, &ctx);
	if(status != BCM_SCAPI_STATUS_OK) {
		crypto_smau_cipher_deinit(pHandle, &ctx);
		return (IPROC_STATUS)status;
	}

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return IPROC_STATUS_OK;
}
