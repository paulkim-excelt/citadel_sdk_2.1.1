/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
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

/* @file crypto.h
 *
 * This file contains the common definitions across the symmetric and
 * asymmetric algorithms
 *
 *
 */

#ifndef  CRYPTO_H
#define  CRYPTO_H


#ifdef __cplusplus
extern "C" {
#endif

/* TODO
 * 1. Change the structure names and align with the coding guidelines
 * 2. _bcm_bulk_cipher_cmd,bcm_bulk_cipher_context_s and _fips_rng_context
 *    should go to symmetric header files
 * 3. DEFAULT_RAW_RNG_SIZE_WORD RNG define should go to symmetric header files
 * 4. smu_bulkcrypto_packet should go to crypto include
 *
 * */
/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/
#ifndef TRUE
#define TRUE        (1)
#endif

#ifndef FALSE
#define FALSE       (0)
#endif

#define CRYPTO_LIB_HANDLE_SIZE    (3*sizeof(crypto_lib_handle))

/**
 * The following configuration decides how many blocks of dma structure are
 * allocated statically. Depending on the packet counts this needs to be
 * increased
 */
#define CRYPTO_SMAU_DMA_PACKET_COUNT 	64

#define SPUM_MAX_PAYLOAD_SIZE  64 * 1023

/**
 * Definition
 */
#define CHECK_DEVICE_BUSY(nDevice) \
                { \
                        if (callback != NULL && pHandle -> busy == TRUE) \
                                return BCM_SCAPI_STATUS_DEVICE_BUSY; \
                }

/* sizes. all sizes (not specified ) are in bytes */
#define DEFAULT_RAW_RNG_SIZE_WORD    5
#define MIN_RAW_RNG_SIZE_WORD        5 /* 20 bytes, 160 bit */
#define MAX_RAW_RNG_SIZE_WORD        16/* 64 bytes, 512 bit */
#define RNG_FIPS_SEEDLEN                     55 /* seedlen = 440 bits */
#define RNG_FIPS_ENTROPY_SIZE                32  /* 256 bits of entroppy */

#define BITLEN2BYTELEN(len) (((len)+7) >> 3)
#define BYTELEN2BITLEN(len) (len << 3)
#define WORDLEN2BYTELEN(len) (len << 2)
#define BYTELEN2WORDLEN(len) (len >> 2)
#define SHA_INIT_STATE_NUM_WORDS 5

#define SHA1_HASH_SIZE               20
#define SHA1_BLOCK_SIZE              20 /* 160 bit block */
#define SHA256_BLOCK_SIZE            64 /* 512 bit block */
#define SHA256_HASH_SIZE             32
#define SHA256_KEY_SIZE              32

#define SHA3_224_HASH_SIZE           28
#define SHA3_256_HASH_SIZE           32
#define SHA3_384_HASH_SIZE           48
#define SHA3_512_HASH_SIZE           64

#define ECC_SIZE_P256				 32
/* for PKCS#1 EMSA_PKCS1_v1-5 encoding */
#define EMSA_PKCS1_V15_PS_LENGTH        8
#define EMSA_PKCS1_V15_PS_BYTE          0xff

#define SHA1_DER_PREAMBLE_LENGTH     15 /*refer to PKCS#1 v2.1 P38 section 9.2*/
#define SHA256_DER_PREAMBLE_LENGTH   19 /*refer to PKCS#1 v2.1 P38 section 9.2*/

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/

/**
 *
 */
typedef enum BCM_SCAPI_STATUS{

    BCM_SCAPI_STATUS_OK = 0,
    BCM_SCAPI_STATUS_OK_NEED_DMA = 1,
    BCM_SCAPI_STATUS_DEVICE_BUSY = 2,
    BCM_SCAPI_STATUS_CRYPTO_ERROR                 = 100,
    BCM_SCAPI_STATUS_PARAMETER_INVALID,
    BCM_SCAPI_STATUS_CRYPTO_NEED2CALLS,
    BCM_SCAPI_STATUS_CRYPTO_PLAINTEXT_TOOLONG,
    BCM_SCAPI_STATUS_CRYPTO_AUTH_FAILED,
    BCM_SCAPI_STATUS_CRYPTO_ENCR_UNSUPPORTED,
    BCM_SCAPI_STATUS_CRYPTO_AUTH_UNSUPPORTED,
    BCM_SCAPI_STATUS_CRYPTO_MATH_UNSUPPORTED,
    BCM_SCAPI_STATUS_CRYPTO_WRONG_STEP,
    BCM_SCAPI_STATUS_CRYPTO_KEY_SIZE_MISMATCH,
    BCM_SCAPI_STATUS_CRYPTO_AES_LEN_NOTALIGN,   /* 110 */
    BCM_SCAPI_STATUS_CRYPTO_AES_OFFSET_NOTALIGN,
    BCM_SCAPI_STATUS_CRYPTO_AUTH_OFFSET_NOTALIGN,
    BCM_SCAPI_STATUS_CRYPTO_DEVICE_NO_OPERATION,
    BCM_SCAPI_STATUS_CRYPTO_NOT_DONE,
    BCM_SCAPI_STATUS_CRYPTO_RNG_NO_ENOUGH_BITS,
	/* generate rng is same as what saved last time */
    BCM_SCAPI_STATUS_CRYPTO_RNG_RAW_COMPARE_FAIL,
	/* rng after hash is same as what saved last time */
    BCM_SCAPI_STATUS_CRYPTO_RNG_SHA_COMPARE_FAIL,
    BCM_SCAPI_STATUS_CRYPTO_TIMEOUT,
    BCM_SCAPI_STATUS_CRYPTO_AUTH_LENGTH_NOTALIGN,     /* for sha init/update */
    BCM_SCAPI_STATUS_CRYPTO_RN_PRIME,           /* 120 */
    BCM_SCAPI_STATUS_CRYPTO_RN_NOT_PRIME,
    BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW,
    BCM_SCAPI_STATUS_INPUTDATA_EXCEEDEDSIZE,
    BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY,
    BCM_SCAPI_STATUS_INVALID_AES_KEYSIZE,
	/* PKCS1 related error codes */
    BCM_SCAPI_STATUS_RSA_PKCS1_LENGTH_NOT_MATCH   = 150,
    BCM_SCAPI_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL,
    BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT,
    BCM_SCAPI_STATUS_RSA_INVALID_HASHALG,
    BCM_SCAPI_STATUS_ECDSA_VERIFY_FAIL,
    BCM_SCAPI_STATUS_ECDSA_SIGN_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_FAIL                = 200,
    BCM_SCAPI_STATUS_SELFTEST_3DES_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DES_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_AES_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_SHA1_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DH_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_ECDH_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_RSA_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DSA_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DSA_SIGN_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DSA_VERIFY_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_DSA_PAIRWISE_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_ECDSA_SIGN_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_ECDSA_VERIFY_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_ECDSA_PAIRWISE_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_ECDSA_PAIRWISE_INTRN_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_MATH_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_RNG_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_MODEXP_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_MODINV_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_SHA256_FAIL,
    BCM_SCAPI_STATUS_SELFTEST_AES_SHA256_FAIL,
    BCM_SCAPI_STATUS_INVALID_KEY

}BCM_SCAPI_STATUS;

/**
 *
 */
typedef enum BCM_HASH_ALG
{
    BCM_HASH_ALG_SHA1,
    BCM_HASH_ALG_SHA2_256,
    BCM_HASH_ALG_SHA2_384,
    BCM_HASH_ALG_SHA2_512,
    BCM_HASH_ALG_SHA3
} BCM_HASH_ALG;
typedef enum bcm_scapi_encr_algs {
    BCM_SCAPI_ENCR_ALG_NONE      =  0,
    BCM_SCAPI_ENCR_ALG_AES_128,
    BCM_SCAPI_ENCR_ALG_AES_192,
    BCM_SCAPI_ENCR_ALG_AES_256,
    BCM_SCAPI_ENCR_ALG_DES,
    BCM_SCAPI_ENCR_ALG_3DES,
    BCM_SCAPI_ENCR_ALG_RC4_INIT,
    BCM_SCAPI_ENCR_ALG_RC4_UPDT
} BCM_SCAPI_ENCR_ALG;

typedef enum bcm_scapi_encr_mode {
    BCM_SCAPI_ENCR_MODE_NONE     = 0,
    BCM_SCAPI_ENCR_MODE_CBC,
    BCM_SCAPI_ENCR_MODE_ECB ,
    BCM_SCAPI_ENCR_MODE_CTR,
    BCM_SCAPI_ENCR_MODE_CCM = 5,
    BCM_SCAPI_ENCR_MODE_CMAC,
    BCM_SCAPI_ENCR_MODE_OFB,
    BCM_SCAPI_ENCR_MODE_CFB,
    BCM_SCAPI_ENCR_MODE_GCM,
    BCM_SCAPI_ENCR_MODE_XTS

} BCM_SCAPI_ENCR_MODE;

typedef enum bcm_scapi_auth_algs {
      BCM_SCAPI_AUTH_ALG_NULL      =  0,
      BCM_SCAPI_AUTH_ALG_MD5 ,
      BCM_SCAPI_AUTH_ALG_SHA1,
      BCM_SCAPI_AUTH_ALG_SHA224,
      BCM_SCAPI_AUTH_ALG_SHA256,
      BCM_SCAPI_AUTH_ALG_AES,
	  BCM_SCAPI_AUTH_ALG_SHA3_224,
      BCM_SCAPI_AUTH_ALG_SHA3_256,
      BCM_SCAPI_AUTH_ALG_SHA3_384,
      BCM_SCAPI_AUTH_ALG_SHA3_512,
} BCM_SCAPI_AUTH_ALG;

typedef enum bcm_scapi_auth_modes {
      BCM_SCAPI_AUTH_MODE_HASH      =  0,
      BCM_SCAPI_AUTH_MODE_CTXT,
      BCM_SCAPI_AUTH_MODE_HMAC,
      BCM_SCAPI_AUTH_MODE_FHMAC,
      BCM_SCAPI_AUTH_MODE_CCM,
      BCM_SCAPI_AUTH_MODE_GCM,
      BCM_SCAPI_AUTH_MODE_XCBCMAC,
      BCM_SCAPI_AUTH_MODE_CMAC
} BCM_SCAPI_AUTH_MODE;

typedef enum bcm_scapi_auth_optype {
	BCM_SCAPI_AUTH_OPTYPE_FULL1 = 0,
	BCM_SCAPI_AUTH_OPTYPE_INIT = 1,
	BCM_SCAPI_AUTH_OPTYPE_UPDATE = 2,
	BCM_SCAPI_AUTH_OPTYPE_FINAL = 4,
	BCM_SCAPI_AUTH_OPTYPE_FULL = 8,
} BCM_SCAPI_AUTH_OPTYPE;

typedef enum bcm_scapi_cipher_modes {
    BCM_SCAPI_CIPHER_MODE_NULL   =  0,
    BCM_SCAPI_CIPHER_MODE_ENCRYPT,
    BCM_SCAPI_CIPHER_MODE_DECRYPT,
    BCM_SCAPI_CIPHER_MODE_AUTHONLY
} BCM_SCAPI_CIPHER_MODE;

typedef enum bcm_scapi_cipher_order {
    BCM_SCAPI_CIPHER_ORDER_NULL  =  0,
    BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT,
    BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH
} BCM_SCAPI_CIPHER_ORDER;

typedef enum bcm_scapi_cipher_optype{
    BCM_SCAPI_CIPHER_RC4_OPTYPE_INIT = 0,
    BCM_SCAPI_CIPHER_RC4_OPTYPE_UPDT,
    BCM_SCAPI_CIPHER_DES_OPTYPE_K56,
    BCM_SCAPI_CIPHER_3DES_OPTYPE_K168EDE,
    BCM_SCAPI_CIPHER_AES_OPTYPE_K128,
    BCM_SCAPI_CIPHER_AES_OPTYPE_K192,
    BCM_SCAPI_CIPHER_AES_OPTYPE_K256
}BCM_SCAPI_CIPHER_OPTYPE;

typedef enum bcm_scapi_hash_mode{
    BCM_SCAPI_HASH_HASH = 0,
    BCM_SCAPI_HASH_CTXT,
    BCM_SCAPI_HASH_HMAC,
    BCM_SCAPI_HASH_FHMAC,
    BCM_SCAPI_AES_HASH_XCBC_MAC,
    BCM_SCAPI_AES_HASH_CMAC,
    BCM_SCAPI_AES_HASH_CCM,
    BCM_SCAPI_AES_HASH_GCM
}BCM_SCAPI_HASH_MODE;

/**
 *
 */
typedef void scapi_callback (u32_t status, void *handle, void *arg);


struct msg_hdr0{
        u32_t crypto_algo_0        : 1,
         auth_enable          : 1,
         sctx_order           : 1,
         encrypt_decrypt      : 1,
         sctx_crypt_mode      : 4,
         sctx_aes_key_size    : 2,
         sha3En                : 1,
         sctx_sha             : 1,
         sctx_hash_mode       : 4,
         sctx_hash_type       : 4,
         check_key_length_sel : 1,
         length_override      : 1,
         send_hash_only       : 1,
         crypto_algo_1        : 1,
         auth_key_sz          : 8;
 };


struct msg_hdr1{
  u32_t pad_length ;
};

/*  auth and aes_offset programming should be relative to Data packet
... Actual crypto programming offset will be handled during packet generation */

struct msg_hdr2{
  u32_t aes_offset:16 ,
           auth_offset :16 ;
};
struct msg_hdr3{
  u32_t aes_length ;
};
struct msg_hdr4{
  u32_t auth_length ;
};

/**
 * Header 5 for the SHA3 and SM3 algorithms
 */
struct msg_hdr5{
/**
 *This bit along with ctrl[23] and ctrl[0] decides
 *whether SM4 encryption algorithm is used.
  {hdr5[0], ctrl[23], ctrl[0]} = 3:b100 - SM3 algorithm is used.
 *
 */
  u32_t  crypto_type: 1,

/**
  This bit along with ctrl[11] (msg_hdr0 : sctx_sha)
  decides the hash algorithm used.
  {hdr5[1], ctrl[11]} = 2:b10 - SHA3 algorithm used
  {hdr5[1], ctrl[11]} = 2:b11 - SM3 algorithm used
*/
  	  	 hash_type: 1,
/**
 *	00 - SHA3-224
	01 - SHA3-256
	10 - SHA3-384
	11 - SHA3-512
 *
 */
		 SHA3_algorithm : 2;
};

struct cryptoiv{

        // Crypto Keys
   	  u32_t DES_IV[2];
   	  u32_t AES_IV[4];
      u32_t iv_len_from_packet;
};

struct cryptokey{

        // Crypto Keys
   	  u32_t DES_key[2];
   	  u32_t _3DES_key[6];
   	  u32_t AES_128_key[4];
   	  u32_t AES_192_key[6];
   	  u32_t AES_256_key[8];

};

struct hashkey{
        // Hash Keys
   	  u32_t HMAC_key[16]; // HMAC key coulbe be max 64 bytes in a multiple of 4
      u32_t auth_key_val ;// this field should be set after the HMAC population
};

typedef void scapi_callback (u32_t status, void *handle, void *arg);

typedef struct _msg_hdr_top {
   struct msg_hdr0  *crypt_hdr0;
   struct msg_hdr1  *crypt_hdr1;
   struct msg_hdr2  *crypt_hdr2;
   struct msg_hdr3  *crypt_hdr3;
   struct msg_hdr4  *crypt_hdr4;
   struct msg_hdr5  *crypt_hdr5;
} msg_hdr_top;


typedef struct smu_bulkcrypto_packet_s {

    msg_hdr_top *msg_hdr;
    struct cryptokey *cryptokey;
    struct hashkey   *hashkey;
    struct cryptoiv  *cryptoiv;
    u32_t  header_length ;        // header length is in words
    u32_t  in_data_length ;      // Must for 128 bit aligned for AES and 64 bit aligned for 	DES
    u32_t  out_data_length ;	    // this will have both the crypto data+ IV + hash value
    u8_t * in_data ;
    u8_t * out_data ;
   /* callback functions for bulk crypto */
	scapi_callback *callback;
	u32_t    flags;
        u8_t SMAU_SRAM_HEADER_ADDR[256];
} smu_bulkcrypto_packet;


/**
 *
 */
typedef struct _bcm_bulk_cipher_cmd
{
        u8_t cipher_mode;     /* encrypt or decrypt */
         u8_t encr_alg ;    /* encryption algorithm */
         u8_t encr_mode ;
          u8_t auth_alg  ;    /* authentication algorithm */
          u8_t auth_mode ;
          u8_t auth_optype;
           u8_t cipher_order;     /* encrypt first or auth first */
} BCM_BULK_CIPHER_CMD;

/**
 *
 */
typedef struct bcm_bulk_cipher_context_s
{

        BCM_BULK_CIPHER_CMD *cmd;
        u32_t    flags;
        struct smu_bulkcrypto_packet_s cur_pkt;
        /* callback functions for bulk crypto */
        scapi_callback *callback;
	u32_t pad_length;
} bcm_bulk_cipher_context;

/* fips rng context */
typedef struct _fips_rng_context
{
    uint8_t __attribute__ ((aligned (4))) v[RNG_FIPS_SEEDLEN];
    uint8_t  __attribute__ ((aligned (4))) c[RNG_FIPS_SEEDLEN];

    uint8_t __attribute__ ((aligned (4))) saved_drbg[MAX_RAW_RNG_SIZE_WORD*4];
    uint8_t __attribute__ ((aligned (4))) saved_raw[MAX_RAW_RNG_SIZE_WORD*4];

    uint8_t  prediction_resist;
    uint32_t reseed_counter;
} fips_rng_context;

/**
 *
 */
typedef struct _crypto_lib_handle
{
    u32_t       cmd;
    u32_t       busy;
    u8_t        *result;
    u32_t       result_len;
    u32_t       *presult_len; /* tell the caller the result length */
    scapi_callback *callback;    /* callback functions for async calls */
    void           *arg;

    /* context pointers */
    /* These two context can exist at the same time,
     * so can not share one pointer */
    /* points to current active async bulk cipher context */
    bcm_bulk_cipher_context *bulkctx;
    fips_rng_context        *rngctx;  /* pionts to the fips rng context */
} crypto_lib_handle;



/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/

/** @} */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* CRYPTO_H */
