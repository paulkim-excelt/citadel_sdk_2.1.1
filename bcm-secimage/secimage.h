/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: secimage.h
 */

#ifndef _SECIMAGE_H_
#define _SECIMAGE_H_

#ifdef SUN_OS
#define BYTE_ORDER_BIG_ENDIAN
#else
#define BYTE_ORDER_LITTLE_ENDIAN
//#include <q_lip.h>
#include <stdint.h>
#endif
#include <sys/types.h>

#include "crypto.h"

#define TRUE	1
#define FALSE	0

/* Debug options */
#undef DEBUG_DATA
#undef DEBUG_DH
#undef DEBUG_DSA
#undef DEBUG_RSA
#undef DEBUG_SHA1
#undef DEBUG_HASH
#undef DEBUG_HMAC_SHA1
#define DEBUG_DLIES
#define DEBUG_IV
#define DEBUG_SIGNATURE
#define DEBUG_DSA_SIGNATURE

#define DSA_FLAG	1
#define RSA_FLAG	2
#define HMAC_FLAG	3
#define ECDSA_FLAG	4
#define PST_FLAG	5

/****************************************************
 **  Return Codes: Image Generation Failures
 ****************************************************/
#define STATUS_OK			 				0
#define SBERROR_DATA_READ					-1
#define SBERROR_DATA_FILE					-2
#define SBERROR_MALLOC						-3
#define SBERROR_NO_DSA_SIGNATURE			-4
#define SBERROR_SHUTDOWN_CRYPTO				-10
#define SBERROR_GENERATE_DH_KEY_PAIR		-11
#define SBERROR_GENERATE_DH_SHARED			-12
#define SBERROR_LOAD_DH_PARAMS				-13
#define SBERROR_LOAD_DSA_PARAMS				-14
#define SBERROR_LOAD_PEER_DH_PUBKEY			-15
#define SBERROR_SHARED_SECRET_FORMAT		-16
#define SBERROR_DATA_INPUT					-17
#define SBERROR_FAIL_SHA1_HMAC				-18
#define SBERROR_FAIL_DSA_SIGN				-19
#define SBERROR_MAX_DSA_SIGNATURE			-20
#define SBERROR_DSA_SIGNATURE_SIZE			-21
#define SBERROR_DSA_SIGN					-22
#define SBERROR_DSA_KEY_ERROR				-23
#define SBERROR_GET_PRIVKEY					-24
#define SBERROR_NO_RSA_SIGNATURE			-25
#define SBERROR_LOAD_RSA_PARAMS				-26
#define SBERROR_RSA_SIGNATURE_SIZE			-27
#define SBERROR_RSA_SIGN					-28
#define SBERROR_RSA_KEY_ERROR				-29
#define SBERROR_FAIL_RSA_SIGN				-30
#define SBERROR_MAX_RSA_SIGNATURE			-31
#define SBERROR_DER_SIGNATURE_SIZE			-32
#define SBERROR_FILENAME_LEN				-33
#define SBERROR_RSA_PKCS1_ENCODED_MSG_TOO_SHORT   -34
#define SBERROR_AES_INVALID_KEYSIZE			-35
#define SBERROR_INVALID_KEYSIZE				-36
#define SBERROR_INVALID_MONTCTXSIZE			-37
#define SBERROR_INVALID_IV					-38
#define SBERROR_EC_KEY_ERROR				-39
#define SBERROR_FAIL_ECDSA_SIGN				-40


/****************************************************
 **    ABI Return Codes:
 ****************************************************/
#define ABIERROR_BAD_MAGIC_NUMBER			-1
#define ABIERROR_NO_BOOT_IMAGE				-2
#define ABIERROR_BAD_BOOT_IMAGE_LENGTH		-3
#define ABIERROR_NO_PUBLIC_KEY				-6
#define ABIERROR_NO_CHAIN_TRUST				-7
#define ABIERROR_BAD_CHAIN_TRUST			-8
#define ABIERROR_NO_PUBLIC_SIGNATURE		-9
#define ABIERROR_NO_CUSTOMER_ID				-10
#define ABIERROR_IMAGE_HEADER				-13


/****************************************************
 **    SBI Return Codes:
 ****************************************************/
#define SBIERROR_INITIAL_BOOT_OK			 0
#define SBIERROR_LOCAL_SECURE_OK			 1
#define SBIERROR_DLIES_SECURE_OK			 2
#define SBIERROR_DSA_SIGNED_OK				 3
#define SBIERROR_BAD_MAGIC_NUMBER			-1
#define SBIERROR_NO_BOOT_IMAGE				-2
#define SBIERROR_BAD_BOOT_IMAGE_LENGTH		-3
#define SBIERROR_NO_DLIES_ENCRPARAM			-4
#define SBIERROR_NO_DLIES_AUTHPARAM			-5
#define SBIERROR_NO_PUBLIC_KEY				-6
#define SBIERROR_NO_CHAIN_TRUST				-7
#define SBIERROR_BAD_CHAIN_TRUST			-8
#define SBIERROR_NO_PUBLIC_SIGNATURE		-9
#define SBIERROR_FAIL_SHA1_HMAC				-11
#define SBIERROR_FAIL_DSA_SIGN				-12
#define SBIERROR_IMAGE_HEADER				-13
#define SBIERROR_INIT_CRYPTO				-101


/****************************************************
 **        Fixed Paramaters Size and Limits
 ****************************************************/
#define SHA1_HASH_SIZE						20
#define SHA256_HASH_SIZE					32
#define PKCS1_PAD_SIZE						256
#define DSA_SIGNATURE_SIZE					(20+20)
#define MAX_DER_SIGNATURE_SIZE				256
#define SHA256_DER_PREAMBLE_LENGTH          (19)
#define SHA1_DER_PREAMBLE_LENGTH            (15)
#define EMSA_PKCS1_V15_PS_LENGTH            (8)
#define MAX_ENCR_PARAM_SIZE					256
#define MAX_AUTH_PARAM_SIZE					256
#define MAX_FILE_NAME						256
#define MAX_RELEASE_INFO_SIZE				256
#define MAX_DATA_LEN						2048
#define MAX_BOOTLOADER_SIZE					(1024 * 1024)
#define MAX_BOOTCODE_SIZE					16000
#define MAX_AUXDATA_SIZE					16000
#define MAX_SBI_SIZE						64000
#define SHARED_SECRET_SIZE					256
#define RSA_DEFAULT_KEY_BITS				2048
#define RSA_KEY_BYTES						(RSA_DEFAULT_KEY_BITS/8)
#define RSA_SIGNATURE_SIZE					RSA_KEY_BYTES

#define RSA_2048_KEY_BYTES		256
#define RSA_4096_KEY_BYTES		512
#define RSA_4096_MONT_CTX_BYTES 1576
#define EC_P256_KEY_BYTES		64
#define EC_P512_KEY_BYTES		128

typedef uint16_t IPROC_SBI_PUB_KEYTYPE;

typedef struct _key_rsa2048{
	uint8_t		Key[RSA_2048_KEY_BYTES];		/* 2048 RSA Key value */
} KEY_RSA2048;

//		/* Signature of all the above fields */


typedef struct _key_rsa4096{
	uint8_t		Key[RSA_4096_KEY_BYTES];			/* 4096 RSA Key value */
	uint8_t		MontCtx[RSA_4096_MONT_CTX_BYTES];
} KEY_RSA4096;


/****************************************************
 **        Function Prototypes
 ****************************************************/
int Usage(void);
int InitCrypto(void);
int ShutdownCrypto(void);
void DataDump(char * msg, void *buf, int len);
void ShortDump(char * msg, void *buf, int len);
void swap_word(void *s);
int DataWrite(char *filename, char *buf, int length);
int DataRead(char *filename, uint8_t *buf, int *length);
int DataAppend(char *out, char *in);
int Shared2Bignum(uint8_t *buf, int len);
int ReadHexFile (char *fname, uint8_t *buf, int maxlen);
int ReadBinaryFile (char *fname, uint8_t *buf, int maxlen);
int Sha1Hash (uint8_t *data, uint32_t len, uint8_t *digest);
int AppendDSASignature(uint8_t *data, uint32_t length, char *filename);
int	VerifyDSASignature(uint8_t *data, uint32_t len, uint8_t *sig, char *key);
void rsaSignaturePrework(uint8_t *data, char *filename, int verbose);
int AppendRSASignature(uint8_t *data, uint32_t length, char *filename, uint32_t DAuth, int verbose);
int	VerifyRSASignature(uint8_t *data, uint32_t len, uint8_t *sig, char *key, int verbose);
int AppendHMACSignature(uint8_t *data, uint32_t length, char *filename, char *bblkeyfile, int verbose, int destOffset);
int OutputHashFile(uint8_t *data, uint32_t length, char *filename, int flag, int verbose);
int	TestDsaSign(char *keyfile, char *sigfile, char *datafile);
int	DLIESKeyGenerate(uint8_t *pb, uint8_t *ekey, uint16_t ekeylen,
			uint8_t *akey, uint8_t *pubKey, uint16_t pblen, uint16_t akeylen,
			uint8_t *iv, uint16_t ivlen, char *filename);
int WriteDefaultDHParams(uint8_t *data, uint16_t *length);
int WriteDSAPubkey(char *filename, uint8_t *dest, uint16_t *len, int privflag);
int WriteRSAPubkey(char *filename, uint8_t *dest, uint16_t *len, int privflag, int verbose);
int GetNewDHPublic(uint8_t *data, uint16_t *length);
int ReadDSASigFile(char *filename, uint8_t *sig);
int ReadRSASigFile(char *filename, uint8_t *sig, uint32_t *length);
int WriteDSASigFile(char *filename);
int CheckDHParam(char *filename);
int VerifyShaHmac(void);
int WriteDauth(char *filename, int verbose);
char * ecdsSignaturePrework(uint8_t *data, char *filename, int verbose);

/****************************************************
 **        ABI Functions Prototypes
 ****************************************************/
int AuthenticatedBootImageHandler(int ac, char **av);
int CreateAuthenticatedBootImage(int ac, char **av);
int VerifyABI(int ac, char **av);
int ABIUsage(void);

/****************************************************
 **        SBI Functions Prototypes
 ****************************************************/
int SecureBootImageHandler(int ac, char **av);
int CreateSecureBootImage(int ac, char **av);
int DLIESCrypto(uint8_t *sbi, uint16_t len, uint8_t *ekey, uint8_t *akey, uint8_t *iv);
int SBILocalCrypto(uint32_t enc, uint8_t *data, uint32_t dataLen, uint8_t *ekey, uint32_t AESKeySize, uint8_t *iv, int verbose);
int VerifySBI(char *filename);
int SBIUsage(void);

uint32_t calc_crc32( uint32_t initval, uint8_t *charArr, uint32_t arraySize);
/****************************************************
 **        Chain of Trust Functions Prototypes
 ****************************************************/
int ChainTrustHandler(int ac, char **av);
int CreateChainTrust(int ac, char **av);
int VerifyChainTrust(int ac, char **av);
int ChainTrustUsage(void);

/*** New definitions added ***/
int GenDAuthHash(int ac, char **av);
int DauthUsage(void);
int AddMontCont( uint32_t *h, RSA *RsaKey, int Dauth );
int DauthGenHandler(int ac, char **av);

int YieldHandler(void);

#define OS_YIELD_FUNCTION YieldHandler
#define QLIP_HEAP_SIZE 4096 /* Heapsize is in Words */
#define QLIP_CONTEXT &qlip_ctx

#define LOCAL_QLIP_CONTEXT_DEFINITION \
q_lip_ctx_t qlip_ctx; \
uint32_t qlip_heap[QLIP_HEAP_SIZE]; \
q_ctx_init(&qlip_ctx,&qlip_heap[0],QLIP_HEAP_SIZE,OS_YIELD_FUNCTION);


#endif /* _SECIMAGE_H_ */
