/*
 * $Copyright Broadcom Corporation$
 *
 */
/*******************************************************************
 *
 *  Copyright 2013
 *  Broadcom Corporation
 *  5300 California Avenue
 *  Irvine, CA 92617
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************
 *
 *  Broadcom iProc SBL Project
 *  File: sbi.h
 *  Description: SBI header structure declarations
 *
 *  $Copyright (C) 2014 Broadcom Corporation$
 *
 *  Version: $Id: $
 *
 *****************************************************************************/
#ifndef _SBI_H_
#define _SBI_H_

#include <zephyr/types.h>

/*****************************************************
 *****       Defintion of SBI Header            *****
 *****************************************************/
typedef struct UnAuthenticatedHeader_t {
	uint32_t Tag;				            /* SBI Tag used to locate SBI in memory */
	uint32_t Length;			            /* Length of the SBI */
	uint32_t Reserved;						/* This field contains the SBI address for the non authenticated boot case. The SBI address is alligned to 16 byte boundary. The lower 4 bits are used for ClkConfig. */
	/* Any other fields should be placed here */

	/* CRC and field should be the last field, CRC should be computed on all
	   other fields in this structure excluding crc field */
	uint32_t crc;
} UNAUTHENT_SBI_HEADER;

typedef uint16_t IPROC_SBI_CONFIG;

#define IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE			0x0001		/* Not Supported for Cygnus */
#define IPROC_SBI_CONFIG_RSA					0x1 << 4	/* 0x0010 */
#define IPROC_SBI_CONFIG_ECDSA					0x1 << 5	/* 0x0020 */
#define IPROC_SBI_CONFIG_SYMMETRIC				0x1 << 6	/* 0x0040 */
#define IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS	0x1 << 8	/* 0x0100 */
#define IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS	0x2 << 8	/* 0x0200 */
#define IPROC_SBI_CONFIG_AES_256_ENCRYPTION			0x3 << 8	/* 0x0300 */
#define IPROC_SBI_CONFIG_AES_ENCRYPTION_MASK			0x3 << 8	/* 0x0300 */
#define IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH			0x1 << 10	/* 0x0400 */

typedef struct AuthenticatedHeader_t {
	IPROC_SBI_CONFIG SBIConfiguration;		/* Indicates SBI configuration */
	uint16_t SBIRevisionID;					/* Indicates SBI Revision ID */
	uint32_t CustomerID;					/* Customer ID */
	uint16_t ProductID;						/* Product ID */
	uint16_t CustomerRevisionID;			/* Customer Revision ID */
	uint32_t COTOffset;						/* Chain of Trust offset (or offset to DAUTH if DAUTH is present) */
	uint16_t nCOTEntries;					/* 0 means no chain of trust */
	uint16_t COTLength;						/* Length of COTEntries */
	uint32_t BootLoaderCodeOffset;			/* Offset to bootloader payload */
	uint32_t BootLoaderCodeLength;			/* Length of bootloader payload */
	uint32_t AuthenticationOffset;			/* End of image, start of Auth data */
	uint16_t ImageVerificationConfig;
	uint16_t AuthLength;
	uint16_t Reserved1;
	uint16_t SBIUsage;
	uint32_t Reserved2;
	uint32_t MagicNumber;          /* Sentinel for header */
} AUTHENTICATED_SBI_HEADER;


typedef struct AuthenticatedBootImageHeader_t {
	UNAUTHENT_SBI_HEADER		unh;  /* Not covered by DSA signature */
	AUTHENTICATED_SBI_HEADER	sbi;  /* Start of signed data */
} SBI_HEADER;


#define SBITotalLength		AuthenticationOffset

#define SBI_MAGIC_NUMBER				0x45F2D99A
#define SBI_BBL_STATUS_DONT_CARE     (0) /* Run SBI regardless of BBL state */
#define SBI_BBL_STATUS_KEY_VALID	 (1) /* Run SBI if BBL KEK is valid */
#define SBI_BBL_STATUS_KEY_CLEARED   (2) /* Run SBI if BBL KEK is cleared */
#define SBI_BBL_STATUS_UNLOCKED      (3) /* Run SBI if BBL lock bits are not set */
#define SBI_BBL_STATUS_KEY_INVALID   (4) /* Run SBI if BBL KEK is invalid */
#define SBI_BBL_STATUS_BATTERY_OFF   (5) /* Secure Bootloader internal use only, 
                                            Should NOT be used by SBI */
#define SBI_BBL_STATUS_ACC_TIMEOUT   (6) /* Timeout on BBL indirect access */ 

#define IPROC_SBI_KEY_ALG_RSA					0x0001
#define IPROC_SBI_KEY_ALG_EC					0x0002

#define IPROC_SBI_KEY_INFO_RSA2048				0x0010
#define IPROC_SBI_KEY_INFO_RSA4096				0x0020

#define IPROC_SBI_KEY_INFO_CURVE_P256				0x0010
#define IPROC_SBI_KEY_INFO_CURVE_P384				0x0020
#define IPROC_SBI_KEY_INFO_CURVE_P521				0x0030

#define IPROC_SBI_KEY_USAGE_IMAGE_AUTHENTICATE_ONLY		0x1000
#define IPROC_SBI_KEY_USAGE_CHAIN_AUTHENTICATE_ONLY		0x2000
#define IPROC_SBI_KEY_USAGE_CHAIN_AND_IMAGE_AUTHENTICATE	0x3000
#define IPROC_SBI_KEY_USAGE_DEBUG				0x5000

#define IPROC_SBI_RSA2048		(IPROC_SBI_KEY_ALG_RSA | IPROC_SBI_KEY_INFO_RSA2048)
#define IPROC_SBI_RSA4096		(IPROC_SBI_KEY_ALG_RSA | IPROC_SBI_KEY_INFO_RSA4096)
#define IPROC_SBI_EC_P256		(IPROC_SBI_KEY_ALG_EC  | IPROC_SBI_KEY_INFO_CURVE_P256)

#define IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5											0x0001
#define IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PSS												0x0002
#define IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256							0x0010
#define IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA384							0x0020
#define IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA512							0x0030
#define IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA3							0x0040


#define TOC_TAG		0x5A5A5A5A
#define DELIMITER_TAG	0x5555AAAA
#define SBI_TAG		0xA5A5A5A5

#define NBLOCKS			4
#define SEARCH_SIZE		64
#define BLOCK_SIZE		(128*1024)
#define BLOCK_SIZE_WORDS	(128*256)

typedef struct ToC_Entry
{
	uint16_t	Reserved;		/* Reserved */
	uint16_t	Usage;			/* Usage */
	uint32_t	BootImageLocation;	/* Boot image location */
} TOC_ENTRY;

typedef enum SBI_USAGE{
	SBI_USAGE_NONE			= 0,
	SBI_USAGE_SLEEP			= 1,
 	SBI_USAGE_DEEP_SLEEP		= 2,
	SBI_USAGE_EXCEPTION		= 4
} SBI_USAGE;

typedef enum EXCEPTION_STATE{
	EXCEPTION_STATE_NONE			= 0,
	EXCEPTION_STATE_NORMAL			= 1,
	EXCEPTION_STATE_EXCEPTION_TOC		= 2,
	EXCEPTION_STATE_TAMPER			= 4
} EXCEPTION_STATE;

typedef struct ToC_Header
{
	uint32_t	Tag;		/* Tag */
	uint16_t	nEntries;	/* Number of entries */
	uint16_t	Length;		/* Length of ToC */
} TOC_HEADER;

typedef TOC_ENTRY (*PointerToToCEntryArray)[];

#define CID_TYPE_PROD		(uint32_t)(0x6 << 28) 	// (0110) Indicates that the device is a production SBI
#define CID_TYPE_DEV		(uint32_t)(0x9 << 28)	// (1001) Indicates that the device is a development SBI
#define CID_TYPE_MASK		(uint32_t)(0xF << 28)	// Mask for checking CID Type

#define	KEYSIZE_AES256	(256/8)
#define KEYSIZE_AES128	(128/8)



/*****************************************************
 *****        SBI Functions Prototypes           *****
 *****************************************************/
int InitializeSBI(SBI_HEADER *header);
int PrintSBI(SBI_HEADER *header);
int VerifySBI(char *filename);
int SBIAddReleasePayload(SBI_HEADER *header, char *filename);
int SBIAddBootLoaderPayload(SBI_HEADER *header, char *filename, char *ivfilename, uint8_t big_endian);
int SBIAddSBIcodePayload(SBI_HEADER *header, char *filename);
int CreateLocalSecureBootImage(SBI_HEADER *h, char *aesfile, char *bblkeyfile, uint32_t AESKeySize);
int SBIAddPubkeyChain(SBI_HEADER *h, char *filename);
int WriteSBI(SBI_HEADER *header, char *filename);
int  SBIAddDauthKey( SBI_HEADER *h, char *DauthKey, int dauth_pubin, uint32_t BroadcomRevisionID);
void FillUnauthFromConfigFile(SBI_HEADER *sbi, char* ConfigFileName);
int SBIAddRecoveryImage(SBI_HEADER *h, char *filename, int recovery_offset);
int  SBIAddECDauthKey(SBI_HEADER *h, char *DauthKey, int dauth_pubin, uint32_t BroadcomRevisionID);

#endif /* _SBI_H_ */
