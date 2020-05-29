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

/* @file crypto_pka.h
 *
 * pka algorithms implementation
 * This file contains the pka algorithms API's declaration.
 *
 */

#ifndef  CRYPTOPKA_H
#define  CRYPTOPKA_H


#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include <crypto/crypto.h>

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/
/**
 * Definition
 */
#define CRYPTO_INDEX_PKEMATH 1

#define RSA_MIN_PUBLIC_KEY_BITS (256)
#define RSA_MAX_PUBLIC_KEY_BITS (4096)
#define RSA_MAX_RNG_BITS        (2048)
#define RSA_MAX_MATH_BITS       (4096)

#define RSA_MAX_PUB_EXP_BITS    (17)
#define RSA_MAX_PUB_EXP_BYTES   ((MAX_PUB_EXP_BITS + BITS_PER_BYTE - 1)\
								/ BITS_PER_BYTE)
#define RSA_DEFAULT_PUB_EXP     (0x10001)
#define RSA_DEFAULT_PUB_EXP_WORD    ((sizeof( u32_t ) -\
									RSA_MAX_PUB_EXP_BYTES) * BITS_PER_BYTE)
/*  The above constant is useful for initializing a word with the exponent when
you only wish to use the first (i.e., upper) RSA_MAX_PUB_EXP_BYTES bytes of the
word. */
#define RSA_NONCE_SIZE              (20)
#define RSA_NUMBER_OF_PRIMES        (2)
#define RSA_RN_INCREMENT            (1000)
#define RSA_N_GEN_ATTEMPTS          (100)
#define RSA_NUM_WITNESS_TESTS       (3)
#define RSA_MAX_MOD_BYTES           (512)
#define RSA_MAX_MODULUS_SIZE        (RSA_MAX_PUBLIC_KEY_BITS)
#define RSA_MIN_PUBLIC_KEY_BYTES    (ROUNDUP_TO_32_BIT(RSA_MIN_PUBLIC_KEY_BITS)\
									/8)
#define RSA_MAX_PUBLIC_KEY_BYTES    (ROUNDUP_TO_32_BIT(RSA_MAX_PUBLIC_KEY_BITS)\
									/8)
#define RSA_MAX_RNG_BYTES           (ROUNDUP_TO_32_BIT(RSA_MAX_RNG_BITS)/8)
#define RSA_MAX_MATH_BYTES          (ROUNDUP_TO_32_BIT(RSA_MAX_MATH_BITS)/8)
#define RSA_MAX_MATH_WORDS          (RSA_MAX_MATH_BYTES/4)
#define RSA_SIZE_SIGNATURE_DEFAULT  RSA_SIZE_MODULUS_DEFAULT

#define DH_SIZE_MODULUS_DEFAULT     (256) /* default DH modulus size */
#define ECC_SIZE_P256                32

/**
 * DSA related definitions
 */
#define DSA_SIZE_SIGNATURE           40
#define DSA_SIZE_V                   20
#define DSA_SIZE_P_DEFAULT           128 /* 1024 bit.p:512 to 1024 bit prime */
#define DSA_SIZE_PUB_DEFAULT         128
#define DSA_SIZE_G                   128
#define DSA_SIZE_Q                   20
#define DSA_SIZE_X                   20
#define DSA_SIZE_RANDOM              20
#define DSA_SIZE_HASH                20
#define DSA_SIZE_R                   20
#define DSA_SIZE_S                   20

/**
 * For 2048 bit DSA signature
 */
#define DSA_SIZE_SIGNATURE_2048           64
#define DSA_SIZE_V_2048                   32
#define DSA_SIZE_P_DEFAULT_2048           256 /* 2048 bit. p: 2048 bit prime */
#define DSA_SIZE_PUB_DEFAULT_2048         256
#define DSA_SIZE_G_2048                   256
#define DSA_SIZE_Q_2048                   32
#define DSA_SIZE_X_2048                   32
#define DSA_SIZE_RANDOM_2048              32
#define DSA_SIZE_HASH_2048                32
#define DSA_SIZE_R_2048                   32
#define DSA_SIZE_S_2048                   32

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/
/**
 * Enumeration for RSA modes
 *
 */
typedef enum bcm_scapi_math_modes {
    BCM_SCAPI_MATH_MODADD =  0,
    BCM_SCAPI_MATH_MODSUB,
    BCM_SCAPI_MATH_MODMUL,
    BCM_SCAPI_MATH_MODEXP,
    BCM_SCAPI_MATH_MODREM,
    BCM_SCAPI_MATH_MODINV,
    BCM_SCAPI_MATH_MUL,
} BCM_SCAPI_MATH_MODES;

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/
/**
 * This API Perform a modular math operation on parameter A and parameter B.
 * For ModExp, paramB is the exponent
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] cmd : Command for the math operation, #BCM_SCAPI_MATH_MODES
 * @param[in] modN : Pointer to the Modulus
 * @param[in] modN_bitlen : Modulus bit length
 * @param[in] paramA : First parameter for the math operation
 * @param[in] paramA_bitlen : Parameter A bit length
 * @param[in] paramB : Second parameter for the math operation
 * @param[in] paramB_bitlen : Parameter B bit length
 * @param[out] result : Pointer to the result of the math operation
 * @param[out] result_bytelen : Pointer to the resultant byte length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_math_accelerate (
                    crypto_lib_handle *pHandle,u32_t cmd,
                    u8_t *modN,   u32_t  modN_bitlen,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u8_t *paramB, u32_t  paramB_bitlen,
                    u8_t *result, u32_t  *result_bytelen,
                    scapi_callback callback, void *arg
                    );

/**
 * This API Perform a perform a Montgomery modular math operation.
 * For ModExp, paramB is the exponent
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] cmd : Command for the math operation, #BCM_SCAPI_MATH_MODES
 * @param[in] modN : Pointer to the Modulus
 * @param[in] modN_bitlen : Modulus bit length
 * @param[in] paramA : First parameter for the math operation
 * @param[in] paramA_bitlen : Parameter A bit length
 * @param[in] paramB : Second parameter for the math operation
 * @param[in] paramB_bitlen : Parameter B bit length
 * @param[out] result : Pointer to the result of the math operation
 * @param[out] result_bytelen : Pointer to the resultant byte length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_mont_math_accelerate (
                    crypto_lib_handle *pHandle,u32_t cmd,
                    u8_t *modN,   u32_t  modN_bitlen,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u8_t *paramB, u32_t  paramB_bitlen,
                    u8_t *result, u32_t  *result_bytelen,
                    scapi_callback callback, void *arg
                    );

/**
 * This API perform an RSA mod exponentiation
 * y: = x**e mod m
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] x : The plain text message
 * @param[in] x_bitlen : Plain text message bit length
 * @param[in] m : Pointer to the Modulus
 * @param[in] m_bitlen : Modulus bit length
 * @param[in] e : Pointer to the Exponent
 * @param[in] e_bitlen : Exponent bit length
 * @param[out] y : Pointer to the result of the exponentiation
 * @param[out] y_bytelen : Pointer to the resultant byte length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_rsa_mod_exp (
                     crypto_lib_handle *pHandle,
                     u8_t *x, u32_t x_bitlen,
                     u8_t *m, u32_t m_bitlen,
                     u8_t *e, u32_t e_bitlen,
                     u8_t *y, u32_t *y_bytelen,
                     scapi_callback callback, void *arg
                     );

/**
 * This API perform an RSA Chinese Remainder Theorem operation
 *
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] x : The encrypted message
 * @param[in] x_bitlen : encrypted message bit length
 * @param[in] edq : dq key in the CRT keys
 * @param[in] q : Prime number q
 * @param[in] q_bitlen : q bit length
 * @param[in] edp : dp key in the CRT keys
 * @param[in] p : Prime number p
 * @param[in] p_bitlen : p bit length
 * @param[in] qinv : Inverse of q
 * @param[out] y : Pointer to the result of the exponentiation
 * @param[out] y_bytelen : Pointer to the resultant byte length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_rsa_mod_exp_crt (
                    crypto_lib_handle *pHandle,
                    u8_t *x,    u32_t x_bitlen,
                    u8_t *edq,
                    u8_t *q,    u32_t q_bitlen,
                    u8_t *edp,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *qinv,
                    u8_t *y,    u32_t *y_bytelen,
                    scapi_callback callback, void *arg
                    );

/**
 * This API generate RSA public and private keys, PKCS#1 v2.1
 *          If any *p_bits, *q_bits == NULL for In/Outs then
 *          generated output has been requested. If these are
 *          not NULL then present values of p or q are used
 *          upon function entry. The user is responsible for
 *          establishing valid primes.
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] modulus_bits : Requested modulus size
 * @param[in] e_bits : Length pointer to public key exponent (e_bits)
 * @param[in] e : Byte pointer (e), manual input
 * @param[in] p_bits : Length pointer to private key prime (p_bits)
 * @param[in] p : Byte pointer (p), manual input or generated output
 * @param[in] q_bits : Length pointer to private key prime (q_bit)
 * @param[in] q : Byte pointer (q), manual input or generated output
 * @param[out] n_bits : Length pointer to public key modulus (n_bits)
 * @param[out] n : Byte pointer modulus (n), generated output
 * @param[out] d_bits : Length pointer to private key exponent (d_bits)
 * @param[out] d : Byte pointer private exponent(d), generated output
 * @param[out] dp_bits :Length pointer to private prime coefficient (dp_bits)
 * @param[out] dp : Byte pointer (dp), generated output
 * @param[out] dq_bits : Length pointer to private prime coefficient (dq_bits)
 * @param[out] dq : Byte pointer (dq), generated output
 * @param[out] qinv_bits : Length pointer to private prime inverse (qinv_bits)
 * @param[out] qinv : Byte pointer prime inverse (qinv), generated output
 * @remark : All input and output parameters in BIGNUM format
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_rsa_key_generate (
		crypto_lib_handle *pHandle,
		u32_t    modulus_bits,            /* input, requested modulus size */
		u32_t *  e_bits,    u8_t *  e,    /* input */
		u32_t *  p_bits,    u8_t *  p,    /* manual input or generated output */
		u32_t *  q_bits,    u8_t *  q,    /* manual input or generated output */
		u32_t *  n_bits,    u8_t *  n,    /* generated output */
		u32_t *  d_bits,    u8_t *  d,    /* generated output */
		u32_t *  dp_bits,   u8_t *  dp,   /* generated output */
		u32_t *  dq_bits,   u8_t *  dq,   /* generated output */
		u32_t *  qinv_bits, u8_t *  qinv  /* generated output */
		);

/**
 * This API perform an RSA mod exponentiation
 * y: = x**e mod m
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] nLen : Modulus length
 * @param[in] n : Pointer to the Modulus
 * @param[in] eLen : Public exponent length
 * @param[in] e : Pointer to the Public exponent
 * @param[in] MLen : Message length
 * @param[in] M : Pointer to the Message
 * @param[in] SLen : Signature length
 * @param[in] S : Pointer to the signature
 * @param[in] hashAlg : Hash algorithm used for signature creation #BCM_HASH_ALG
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_verify (
                    crypto_lib_handle *pHandle,
                    u32_t nLen, u8_t *n, /* modulus */
                    u32_t eLen, u8_t *e, /* exponent */
                    u32_t MLen, u8_t *M, /* message */
                    u32_t SLen, u8_t *S,  /* signature */
                    BCM_HASH_ALG hashAlg
                    );

/**
 * This API perform Encoding Method for Signatures with Appendix.
 *      ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.pdf
 *      http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
 *      or see RFC#3447
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] mLen : Length of message to encode (m) in bytes
 * @param[in] M : Byte pointer to message (n)
 * @param[out] emLen : Length of encoded message (em) in bytes
 * @param[out] em : Byte pointer to encoded message (em)
 * @param[in] hashAlg : Hash algorithm used for signature creation #BCM_HASH_ALG
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_emsa_pkcs1_v15_encode (
                    crypto_lib_handle *pHandle,
                    u32_t mLen, u8_t *M, u32_t emLen,
                    u8_t *em, BCM_HASH_ALG hashAlg
                    );

/**
 * @brief   Generate DSA public and private keys.
 * @details Need random input.
 * @param[in]   l           The bit lengths of p
 * @param[in]   n           The bit lengths of g
 * @param[in]   p           The prime p.
 * @param[in]   p           The prime q.
 * @param[in]   g           g = h^((p-1)/q) mod p.
 * @param[inout]   privkey  The random number, length is the same as q.
 * @param[out]  pubkey      The public key.
 * @param[in]   bPrivValid  The private key or random number is valid or not
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_dsa_key_generate(crypto_lib_handle *pHandle,
				u32_t l, u32_t n,
				u8_t *p, u8_t *q, u8_t *g,
				u8_t *privkey, u8_t *pubkey, u32_t bPrivValid);

/**
 * This API perform a DSA signature
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash : SHA1 hash of the message
 * @param[in] random : random number, length is 20 bytes,
                       generated internally if specified NULL
 * @param[in] p : 512 bit to 1024 bit prime, length must be exact in 64 bit
 *           	increments
 * @param[in] p_bitlen : Prime bit length
 * @param[in] q : 160 bit prime
 * @param[in] g : h**(p-1)/q mod p, where h is a generator. g length is same
 *           	as p length
 * @param[in] x : private key, length is 20 bytes
 * @param[out] rs : The signature, first part is r, followed by s,
 * 				    each is q length
 * @param[out] rs_bytelen : The signature length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_dsa_sign (
                  crypto_lib_handle *pHandle,
                  u8_t *hash,
                  u8_t *random,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *q,
                  u8_t *g,
                  u8_t *x,
                  u8_t *rs,     u32_t *rs_bytelen,
                  scapi_callback   callback, void *arg
                  );

/**
 * This API Verify a DSA signature (private key: x, public key: p, q, g, y.)
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash : SHA1 hash of the message
 * @param[in] random : random number, length is 20 bytes,
                       generated internally if specified NULL
 * @param[in] p : 512 bit to 1024 bit prime, length must be exact in 64 bit
 *           	increments
 * @param[in] p_bitlen : Prime bit length
 * @param[in] q : 160 bit prime
 * @param[in] g : h**(p-1)/q mod p, where h is a generator. g length is same
 *           	as p length
 * @param[in] y : g**x mod p, length is same as p length
 * @param[in] r : signature
 * @param[in] s : signature For parameters q, r, s, they don't have a
 *                length field, since it's fixed 20 bytes long.
 * @param[in] v : If v == r then verify success
 * @param[out] v_bytelen : The signature length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_dsa_verify (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *q,
                    u8_t *g,
                    u8_t *y,
                    u8_t *r,    u8_t  *s,
                    u8_t *v,    u32_t *v_bytelen,
                    scapi_callback callback, void *arg
                    );

/**
 * This API perform a DSA signature for 2048 bit prime
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash : SHA1 hash of the message
 * @param[in] random : random number, length is 20 bytes,
                       generated internally if specified NULL
 * @param[in] p : 2048 bit prime
 * @param[in] p_bitlen : Prime bit length
 * @param[in] q :  160-256 bit prime
 * @param[in] g : h**(p-1)/q mod p, where h is a generator. g length is same
 *           	as p length
 * @param[in] x : private key, length is 20 bytes
 * @param[out] rs : The signature, first part is r, followed by s,
 * 				    each is q length
 * @param[out] rs_bytelen : The signature length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_dsa_sign_2048 (
                  crypto_lib_handle *pHandle,
                  u8_t *hash,
                  u8_t *random,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *q,
                  u8_t *g,
                  u8_t *x,
                  u8_t *rs,     u32_t *rs_bytelen,
                  scapi_callback   callback, void *arg
                  );

/**
 * This API Verify a DSA signature for 2048 bit
 * (private key: x, public key: p, q, g, y.)
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash : SHA1 hash of the message
 * @param[in] random : random number, length is 20 bytes,
                       generated internally if specified NULL
 * @param[in] p : 2048 bit prime
 * @param[in] p_bitlen : Prime bit length
 * @param[in] q : 160 bit prime
 * @param[in] g : h**(p-1)/q mod p, where h is a generator. g length is same
 *           	as p length
 * @param[in] y : g**x mod p, length is same as p length
 * @param[in] r : signature
 * @param[in] s : signature For parameters q, r, s, they don't have a
 *                length field, since it's fixed 20 bytes long.
 * @param[in] v : If v == r then verify success
 * @param[out] v_bytelen : The signature length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_dsa_verify_2048 (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *q,
                    u8_t *g,
                    u8_t *y,
                    u8_t *r,    u8_t  *s,
                    u8_t *v,    u32_t *v_bytelen,
                    scapi_callback callback, void *arg);

/**
 * This API generates a Diffie-Hellman key pair
 * @param[in] pHandle : Pointer to the crypto context
 * @param[inout] x : The secret value, has actual bit length (IN/OUT)
 * @param[in] x_bitlen : The secret value bit length
 * @param[in] xvalid : 1 - manual input of private key pointed by x,
 *                     0 - generate private key internally.
 * @param[out] y : The public value. y = g**x mod m,
 *                y length is find_mod_size(m_bitlen)
 * @param[out] y_bytelen : y bit length
 * @param[in] g : generator, has actual bit length
 * @param[in] g_bitlen : generator bit length
 * @param[in] m : modulus, has actual bit length
 * @param[in] m_bitlen : modulus bit length
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_diffie_hellman_generate (
                    crypto_lib_handle *pHandle,
                    u8_t *x,      u32_t x_bitlen,
                    u32_t xvalid,
                    u8_t *y,      u32_t *y_bytelen,
                    u8_t *g,      u32_t g_bitlen,
                    u8_t *m,      u32_t m_bitlen,
                    scapi_callback callback, void *arg
                    );

/**
 * This API generates a Diffie-Hellman shared secret
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] x : The secret value, has actual bit length
 * @param[in] x_bitlen : x bit length
 * @param[in] y : The other end public value, has actual bit length
 * @param[in] y_bitlen : y bit length
 * @param[in] m : Modulus, has actual bit length
 * @param[in] m_bitlen : modulus bit length
 * @param[out] k : The shared secret. k = y**x mod m,
 * 				   k length is find_mod_size(m_bitlen)
 * @param[out] k_bytelen : Shared secret length in bytes
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_diffie_hellman_shared_secret (
                     crypto_lib_handle *pHandle,
                     u8_t *x, u32_t x_bitlen,
                     u8_t *y, u32_t y_bitlen,
                     u8_t *m, u32_t m_bitlen,
                     u8_t *k, u32_t *k_bytelen,
                     scapi_callback callback, void *arg);

/**
 * This API generates an Elliptic Curve Diffie-Hellman key pair
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] type : Curve type, 0 - Prime field, 1 - binary field
 * @param[in] p : Prime p
 * @param[in] p_bitlen : size of prime p in bits
 * @param[in] a : Parameter a
 * @param[in] b : Parameter b
 * @param[in] n : The order of the Base point G (represented by "r"
 *                in sample curves defined in FIPS 186-2 document)
 * @param[in] Gx : x coordinate of Base point G
 * @param[in] Gy : y coordinate of Base point G
 * @param[in] d : private key, random integer from [1, n-1]
 * @param[in] dvalid : 1 - manual input of private key pointed by d,
 *                     0 - generate private key internally.
 * @param[out] Qx : x coordinate of public key Q
 * @param[out] Qy : y coordinate of public key Q
 *            Qx and Qy, x and y coordinates of point Q, public key (Q = dG)
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_ecp_diffie_hellman_generate (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *d,
                  u32_t dvalid,
                  u8_t *Qx,
                  u8_t *Qy,
                  scapi_callback callback, void *arg
                  );

/**
 * This API generates an Elliptic Curve Diffie-Hellman shared secret
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] type : Curve type, 0 - Prime field, 1 - binary field
 * @param[in] p : Prime p
 * @param[in] p_bitlen : size of prime p in bits
 * @param[in] a : Parameter a
 * @param[in] b : Parameter b
 * @param[in] n : The order of the Base point G (represented by "r"
 *                in sample curves defined in FIPS 186-2 document)

 * @param[in] d : private key, random integer from [1, n-1]
 * @param[in] dvalid : 1 - manual input of private key pointed by d,
 *                     0 - generate private key internally.
 * @param[in] Qx : x coordinate of other end public key Q
 * @param[in] Qy : y coordinate of other end public key Q
 *            Qx and Qy, x and y coordinates of point Q, public key (Q = dG)
 * @param[out] Kx : The shared secret. (Kx, Ky, Kz) = dQ
 * @param[out] Ky : The shared secret. (Kx, Ky, Kz) = dQ
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_ecp_diffie_hellman_shared_secret (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *d,
                  u8_t *Qx,
                  u8_t *Qy,
                  u8_t *Kx,
                  u8_t *Ky,
                  scapi_callback callback, void *arg
                  );

/**
 * This API generates Elliptic Curve DSA key pair
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] type : Curve type, 0 - Prime field, 1 - binary field
 * @param[in] p : Prime p
 * @param[in] p_bitlen : size of prime p in bits
 * @param[in] a : Parameter a
 * @param[in] b : Parameter b
 * @param[in] n : The order of the Base point G (represented by "r"
 *                in sample curves defined in FIPS 186-2 document)
 * @param[in] Gx : x coordinate of Base point G
 * @param[in] Gy : y coordinate of Base point G
 * @param[in] d : private key, random integer from [1, n-1]
 * @param[in] dvalid : 1 - manual input of private key pointed by d,
 *                     0 - generate private key internally.
 * @param[out] Qx : x coordinate of public key Q
 * @param[out] Qy : y coordinate of public key Q
 *            Qx and Qy, x and y coordinates of point Q, public key (Q = dG)
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_key_generate (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *d,
                  u32_t dvalid,
                  u8_t *Qx,
                  u8_t *Qy,
                  scapi_callback callback, void *arg
                  );

/**
 * This API perform an elliptic curve DSA signature
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash :
 * @param[in] hashLength :
 * @param[in] type : Curve type, 0 - Prime field, 1 - binary field
 * @param[in] p : Prime p
 * @param[in] p_bitlen : size of prime p in bits
 * @param[in] a : Parameter a
 * @param[in] b : Parameter b
 * @param[in] n : The order of the Base point G (represented by "r"
 *                in sample curves defined in FIPS 186-2 document)
 * @param[in] Gx : x coordinate of Base point G
 * @param[in] Gy : y coordinate of Base point G
 * @param[in] k : random integer from [1, n-1]
 * @param[in] kvalid : 1 - manual input of private key pointed by k,
 *                     0 - generate private key internally.
 * @param[in] d : private key
 * @param[out] r : The signature, first part is r (q length)
 * @param[out] s : The signature, second part s (q length)
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_sign (
                  crypto_lib_handle *pHandle,
                  u8_t *hash,
                  u32_t hashLength,
                  u8_t type,
                  u8_t *p,
                  u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *k,
                  u32_t kvalid,
                  u8_t *d,
                  u8_t *r,
                  u8_t *s,
                  scapi_callback   callback, void *arg
                  );

/**
 * This API verify an Elliptic curve DSA signature
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] hash :
 * @param[in] hashLength :
 * @param[in] type : Curve type, 0 - Prime field, 1 - binary field
 * @param[in] p : Prime p
 * @param[in] p_bitlen : size of prime p in bits
 * @param[in] a : Parameter a
 * @param[in] b : Parameter b
 * @param[in] n : The order of the Base point G (represented by "r"
 *                in sample curves defined in FIPS 186-2 document)
 * @param[in] Gx : x coordinate of Base point G
 * @param[in] Gy : y coordinate of Base point G
 * @param[in] Qx : x coordinate of Base point Q
 * @param[in] Qy : y coordinate of Base point Q
 * @param[in] Qz : y coordinate of Base point Q
 * @param[in] r : First part of signature
 * @param[in] s : Second part signature
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @remark : If the callback parameter is NULL, the operation is synchronous
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_verify (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u32_t hashLength,
                    u8_t type,
                    u8_t *p,
                    u32_t p_bitlen,
                    u8_t *a,
                    u8_t *b,
                    u8_t *n,
                    u8_t *Gx,
                    u8_t *Gy,
                    u8_t *Qx,
                    u8_t *Qy,
                    u8_t *Qz,
                    u8_t *r,
                    u8_t  *s,
                    /*u8_t *v,*/
                    scapi_callback callback, void *arg
                    );

/**
 * This API TODO
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_emsa_pkcs1_v15_encode_hash(
                    const u8_t *preamble, unsigned preambleLen,
                    const u8_t *hash, unsigned hashLen,
                    u32_t emLen, u8_t *em
                    );

/**
 * This API regenerate RSA private key exponent and public modulus from p and q
 * @param[in] modulus_bits : input, requested modulus size
 * @param[in] p : Byte pointer (p), manual input
 * @param[in] q : Byte pointer (q), manual input
 * @param[out] n : Byte pointer (n, public modulus), generated output
 * @param[out] d : Byte pointer (d, private exponent), generated output
 * @remark : All input and output parameters in BIGNUM format
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_fips_rsa_key_regenerate (
				      crypto_lib_handle *pHandle,
				      u32_t    modulus_bits,
				      u8_t *  p,
				      u8_t *  q,
				      u8_t *  n,
				      u8_t *  d
					);

/**
 * This API TODO
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_sign_hash(
                    crypto_lib_handle *pHandle, u32_t pqLen,
                    u8_t *pq,
                    u32_t hashLen, u8_t * hash,
                    u32_t *sigLen, u8_t *sig
                    );

/**
 * This API TODO
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_verify_hash(
                    crypto_lib_handle *pHandle,
                    u32_t nLen, u8_t *n,
                    u32_t eLen, u8_t *e,
                    u32_t HLen, u8_t *H,
                    u32_t SLen, u8_t *S,
                    BCM_HASH_ALG hashAlg
                    );

/**
 * This API Generate an Elliptic curve DSA Key Pair
 * @param[in] pHandle : Pointer to the crypto context
 * @param[out] privateKey : Pointer to the privateKey.
 * 					It must be 32 Bytes in size
 * @param[out] publicKey : Pointer to the publicKey.
 * 					It must be 64 Bytes in size.
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_key_generate(
		crypto_lib_handle *pHandle,
		u8_t* privateKey,
		u8_t* publicKey);

/**
 * This API Generate an Elliptic curve DSA Key Pair
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] privateKey : Pointer to the privateKey.It must be 32 Bytes in size
 * @param[out] publicKey : Pointer to the publicKey. It must be 64 Bytes in size.
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ecp_ecdsa_key_generate_publickeyonly(
		crypto_lib_handle *pHandle,
		u8_t* privateKey,
		u8_t* publicKey);

/**
 * This API Perform an Elliptic curve DSA signature with
 * SHA-256 on NIST P-256 curve Signature
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] privKey : Pointer to the Private Key
 * @param[in] messageLength : Length of the message to be signed
 * @param[in] message : Pointer to the message
 * @param[out] signature : Pointer to the signature for the message.
 * 						   It must be 64 Bytes in size.
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_sign(
		crypto_lib_handle *pHandle,
		u8_t* privKey,
        u32_t messageLength,
		u8_t* message,
		u8_t* signature);

/**
 * This API Perform an Elliptic curve DSA signature verification with
 * SHA-256 on NIST P-256 curve Signature
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pubKey : Pointer to the public Key
 * @param[in] messageLength : Length of the message signed
 * @param[in] message : Pointer to the message
 * @param[in] signature : Pointer to the signature for the message.
 * @return If Message Hash == r then verify success( = 0)
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_verify(
		crypto_lib_handle *pHandle,
		u8_t* pubKey,
        u32_t messageLength,
		u8_t* message,
		u8_t* Signature);

/**
 * This API Generate an Elliptic Curve Diffie-Hellman shared secret
 * with P-256 curve
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] privKey : Pointer to the Private Key
 * @param[in] pubKey : Pointer to the Public Key
 * @param[out] sharedSecret : Pointer to the shared Secret.
 * 				This must be 96 bytes in length.
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ecp_diffie_hellman_shared_secret(
		crypto_lib_handle *pHandle,
        u8_t* privKey,
		u8_t* pubKey,
		u8_t* sharedSecret);

/**
 * This API Verify an Elliptic curve public key (an EC point on P-256)
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pubKey : Pointer to the Public Key
 * @return If public key is valid, then success( = 0)
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_pka_cust_ec_point_verify(
		crypto_lib_handle *pHandle,
		u8_t* pubKey);

/**
 * @brief   Test input prime.
 * @details Test if input data is prime, Rabin-Miller primality test,
 * 			there's still possibility that a composite number could pass test.
 * @param[in]     prime        The input data pointer. The buffer length should
 * 					 be a multiple of 32, i.e. 32, 64, 96...
 * @param[in]     prime_bitlen The input data length in bits, need to be byte
 * 					aligned, must not be greater than 4096.
 * @param[in]     t            The Rabin-Miller test loop value
 * @param[in]     rand         The big random data for Rabin-Miller test, the
 * 					buffer length should be (prime buffer length) * t.
 * @param[out]    test         The test result, 0-Not a prime, 1-Possible to
 * 				be prime, possibility is 1-4^(-t).
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_prime_test(
                    u8_t *prime, u32_t prime_bitlen,
                    u32_t t, u8_t *rand,
                    u8_t *test
                    );

/**
 * @brief   Generate prime.
 * @details Generate prime from input random number, with Rabin-Miller
 * 			primality test.
 * @param[in]     seed         The random seed to generate random
 * @param[in]     t            The Rabin-Miller test loop value
 * @param[in]     rand         The big random data for Rabin-Miller test, the
 * 					buffer length should be (prime buffer length) * t.
 * @param[in]     prime_bitlen The output prime length in bits, need to be
 * 					byte aligned, must not be greater than 4096.
 * @param[out]    prime        The output prime pointer, the buffer length
 * 					should be 32, 64...32*n
 * @param[out]    check        The test result, 0-Fail to generate a prime,
 * 					need to start from new random; 1-Generate a new number,
 * 					possible to be prime, possibility is 1-4^(-t).
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_prime_generate(
                    u8_t *seed, u32_t t,
                    u8_t *rand, u32_t prime_bitlen,
                    u8_t *prime, u8_t *check
                    );

/**
 * @brief   Perform an China crypto SM2 signature.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 			published by China Crypto. Bureau
 * @param[in]   hash        Hash of the message.
 * @param[in]   hash_bitlen Hash length in bits,160 for SHA1 and 256 for SHA256.
 * @param[in]   random      Random integer.
 * @param[in]   p           The prime p.
 * @param[in]   p_bitlen    size of prime p in bits.
 * @param[in]   a           The elliptic curve parameter a.
 * @param[in]   b           The elliptic curve parameter b.
 * @param[in]   n           The order of the Base point G.
 * @param[in]   d           The private key, random integer from [1, n-1].
 * @param[in]   Gx          x coordinate of Base point G.
 * @param[in]   Gy          y coordinate of Base point G.
 * @param[out]  r           The first part of signature.
 * @param[out]  s           The second part of signature.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypt_pka_ecp_sm2_sign(
                    u8_t *hash, u32_t hash_bitlen,
                    u8_t *random,
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n, u8_t *d,
                    u8_t *Gx, u8_t *Gy,
                    u8_t *r, u8_t *s
                    );

/**
 * @brief   Verify an China crypto SM2 signature.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 				published by China Crypto. Bureau
 * @param[in]   hash        Hash of the message.
 * @param[in]   hash_bitlen Hash length in bits,160 for SHA1 and 256 for SHA256.
 * @param[in]   p           The prime p.
 * @param[in]   p_bitlen    size of prime p in bits.
 * @param[in]   a           The elliptic curve parameter a.
 * @param[in]   b           The elliptic curve parameter b.
 * @param[in]   n           The order of the Base point G.
 * @param[in]   Gx          x coordinate of Base point G.
 * @param[in]   Gy          y coordinate of Base point G.
 * @param[in]   Qx          x coordinate of Base point Q.
 * @param[in]   Qy          y coordinate of Base point Q.
 * @param[in]   r           The first part of signature.
 * @param[in]   s           The second part of signature.
 * @param[out]  v           If v == r then verify success.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_ecp_sm2_verify(
                    u8_t *hash, u32_t hash_bitlen,
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n,
                    u8_t *Gx, u8_t *Gy,
                    u8_t *Qx, u8_t *Qy,
                    u8_t *r, u8_t *s,
                    u8_t *v
                    );

/**
 * @brief   Generate an SM2 key pair.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 			published by China Crypto. Bureau.
 * @param[in]   p        The prime p.
 * @param[in]   p_bitlen size of prime p in bits.
 * @param[in]   a        The elliptic curve parameter a.
 * @param[in]   b        The elliptic curve parameter b.
 * @param[in]   n        The order of the Base point G.
 * @param[in]   d        The private key, random integer from [1, n-1].
 * @param[in]   Gx       x coordinate of Base point G.
 * @param[in]   Gy       y coordinate of Base point G.
 * @param[out]  Qx       x coordinate of Base point Q.
 * @param[out]  Qy       y coordinate of Base point Q.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_ecp_sm2_key_generate(
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n, u8_t *d,
                    u8_t *Gx, u8_t *Gy,
                    u8_t *Qx, u8_t *Qy
                    );

/**
 * @brief   Generate an SM2 exchange key.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 				published by China Crypto. Bureau.
 * @param[in]   p        The prime p.
 * @param[in]   p_bitlen size of prime p in bits.
 * @param[in]   a        The elliptic curve parameter a.
 * @param[in]   b        The elliptic curve parameter b.
 * @param[in]   n        The order of the Base point G.
 * @param[in]   r        The random number from [1, n-1].
 * @param[in]   Gx       x coordinate of Base point G.
 * @param[in]   Gy       y coordinate of Base point G.
 * @param[out]  Rx       x coordinate of Base point R.
 * @param[out]  Ry       y coordinate of Base point R.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_ecp_sm2_generate_exchange(
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n, u8_t *r,
                    u8_t *Gx, u8_t *Gy,
                    u8_t *Rx, u8_t *Ry
                    );

/**
 * @brief   Generate an SM2 shared secret.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 			published by China Crypto. Bureau.
 * @param[in]   p        The prime p.
 * @param[in]   p_bitlen size of prime p in bits.
 * @param[in]   a        The elliptic curve parameter a.
 * @param[in]   b        The elliptic curve parameter b.
 * @param[in]   n        The order of the Base point G.
 * @param[in]   d        The local private key from [1, n-1].
 * @param[in]   h        The div factor, h = E(Fp)/n.
 * @param[in]   r        The random number from [1, n-1].
 * @param[in]   Lx       x coordinate of local exchange key.
 * @param[in]   Ly       y coordinate of local exchange key.
 * @param[in]   Rx       x coordinate of remote exchange key.
 * @param[in]   Ry       y coordinate of remote exchange key.
 * @param[in]   Px       x coordinate of remote public key.
 * @param[in]   Py       y coordinate of remote public key.
 * @param[out]  Sx       x coordinate of shared secret.
 * @param[out]  Sy       y coordinate of shared secret.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_ecp_sm2_shared_secret(
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n,
                    u8_t *d, u8_t *h, u8_t *r,
                    u8_t *Lx, u8_t *Ly,
                    u8_t *Rx, u8_t *Ry,
                    u8_t *Px, u8_t *Py,
                    u8_t *Sx, u8_t *Sy
                    );

/**
 * @brief   Test if a point is on an SM2 curve.
 * @details The SM2 is public key algorithms based on ECC, and it is
 * 			published by China Crypto. Bureau.
 * @param[in]   p        The prime p.
 * @param[in]   p_bitlen size of prime p in bits.
 * @param[in]   a        The elliptic curve parameter a.
 * @param[in]   b        The elliptic curve parameter b.
 * @param[in]   n        The order of the Base point G.
 * @param[in]   Px       x coordinate of test point.
 * @param[in]   Py       y coordinate of test point.
 * @param[out]  test     test result, 0-not on curve, 1-is on curve.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_ecp_sm2_test_point(
                    u8_t *p, u32_t p_bitlen,
                    u8_t *a, u8_t *b, u8_t *n,
                    u8_t *Px, u8_t *Py,
                    u32_t *test
                    );

/**
 * @brief   SM3 hash digest.
 * @details The SM3 is Cryptographic Hash Algorithm, and it is
 * 			published by China Crypto. Bureau.
 * @param[in]   data        Input data for digest.
 * @param[in]   data_length Input data length in bytes.
 * @param[out]  hash        hash result, should use 32 bytes buffer.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_sm3_digest(
                    u8_t *data, u32_t data_length,
                    u8_t *hash
                    );

/**
 * @brief   Generate key from SM3 hash result.
 * @details The SM3 is Cryptographic Hash Algorithm, and it is
 * 			published by China Crypto. Bureau.
 * @param[in]   data_length Input data length in bytes, range 1...256.
 * @param[in]   data        Input data for digest.
 * @param[in]   key_length  Key length in bytes.
 * @param[out]  key         Key result.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
s32_t crypto_pka_sm3_kdf(
                    u32_t data_length, u8_t *data,
                    u32_t key_length, u8_t *key
                    );


/**
 * TODO dirty implementation to be removed
 */
BCM_SCAPI_STATUS YieldHandler(void);

/** @} */


#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* CRYPTOPKA_H */
