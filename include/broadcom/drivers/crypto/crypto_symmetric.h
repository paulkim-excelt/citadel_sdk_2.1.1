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

/* @file crypto_symmetric.h
 *
 *
 * This file contains the
 *
 */

#ifndef  CRYPTO_SYMMETRIC_H
#define  CRYPTO_SYMMETRIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <string.h>
#include <misc/printk.h>
#include <zephyr/types.h>
#include <crypto/crypto.h>
/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/
#define AES_128_KEY_LEN_BYTES		16
#define AES_192_KEY_LEN_BYTES		24
#define AES_256_KEY_LEN_BYTES		32
/* IV size is the same as the block size of AES = 128 bits */
#define AES_IV_SIZE			16
/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/
/**
 * This API perform the HMAC SHA1 or SHA1 operation
 * (depends on if the keys are passed in), outputs the hash
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pMsg : Pointer to the message to be hashed
 * @param[in] nMsgLen : Length of the message
 * @param[in] pKey : Pointer to the key if HMAC has to be calculated
 * @param[in] nKeyLen : Length of the key
 * @param[out] output : Output pointer for the hash data
 * @param[in] bSwap : NOT USED
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha1(crypto_lib_handle *pHandle,
										u8_t *pMsg, u32_t nMsgLen,
										u8_t *pKey, u32_t nKeyLen,
										u8_t *output, u32_t bSwap);

/**
 * @brief Perform the HMAC SHA (depends on if the keys are passed in) or
 *        SHA init operation with hash output. This function is required in
 *        case of generating hash using parts of the complete data.
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_init(crypto_lib_handle *handle,
						BCM_SCAPI_AUTH_ALG alg,
						u8_t *msg, u32_t msg_len,
						u8_t *key, u32_t key_len,
						u8_t *output);

/**
 * @brief Perform the HMAC SHA (depends on if the keys are passed in)or
 *        SHA update operation with hash output
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param pre_sha previous sha
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_update(crypto_lib_handle *handle,
						  BCM_SCAPI_AUTH_ALG alg,
						  u8_t *msg, u32_t msg_len,
						  u8_t *key, u32_t key_len,
						  u8_t *pre_sha,
						  u8_t *output);

/**
 * @brief Perform the HMAC SHA1 (depends on if the keys are passed in) or
 *        SHA1 finish operation with hash output
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param pre_sha previous sha
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_final(crypto_lib_handle *handle,
						 BCM_SCAPI_AUTH_ALG alg,
						 u8_t *msg, u32_t msg_len,
						 u32_t total_len,
						 u8_t *key, u32_t key_len,
						 u8_t *pre_sha,
						 u8_t *output);

/**
 * This API perform the HMAC SHA256 or SHA256 operation
 * (depends on if the keys are passed in), outputs the hash
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pMsg : Pointer to the message to be hashed
 * @param[in] nMsgLen : Length of the message
 * @param[in] pKey : Pointer to the key if HMAC has to be calculated
 * @param[in] nKeyLen : Length of the key
 * @param[out] output : Output pointer for the hash data
 * @param[in] bSwap : NOT USED
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha256(crypto_lib_handle *pHandle,
												u8_t *pMsg, u32_t nMsgLen,
												u8_t *pKey, u32_t nKeyLen,
												u8_t *output, u32_t bSwap);

/**
 * This API perform the HMAC SHA256 or SHA256 operation
 * (depends on if the keys are passed in), outputs the hash
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pMsg : Pointer to the message to be hashed
 * @param[in] nMsgLen : Length of the message
 * @param[in] eAlg : SHA3 bit legths to be used #BCM_SCAPI_AUTH_ALG
 * @param[out] output : Output pointer for the hash data
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_sha3(crypto_lib_handle *pHandle,
										u8_t *pMsg, u32_t nMsgLen,
										u8_t *output, BCM_SCAPI_AUTH_ALG eAlg);

/**
 * This API Perform a generic AES operation
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] encr_mode : encryption mode (CBC, ECB, CTR)
 * @param[in] encr_alg : ecryption alg (AES 128, 192, 256)
 * @param[in] cipher_mode : encrypt or decrypt
 * @param[in] pAESKey : buffer containing AES key
 * @param[in] pAESIV : buffer containing AES IV
 * @param[in] pMsg : buffer containing AAD (padded) + Msg (padded)
 * @param[in] nMsgLen : length of Msg
 * @param[in] crypto_len : length of Msg (unpadded)
 * @param[in] crypto_offset : length of AAD (unpadded, without length field)
 * @param[out] output : output buffer
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_fips_aes(crypto_lib_handle *pHandle,
			BCM_SCAPI_ENCR_MODE encr_mode,
			BCM_SCAPI_ENCR_ALG encr_alg,
			BCM_SCAPI_CIPHER_MODE cipher_mode,
			u8_t *pAESKey, u8_t *pAESIV,
			u8_t *pMsg, u32_t nMsgLen,
			u32_t crypto_len, u32_t crypto_offset,
			u8_t *output);

/**
 * This API Perform the AES CCM operation
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] encr_mode : encryption mode (CBC, ECB, CTR)
 * @param[in] encr_alg : ecryption alg (AES 128, 192, 256)
 * @param[in] cipher_mode : encrypt or decrypt
 * @param[in] pAESKey : buffer containing AES key
 * @param[in] pAESIV : buffer containing AES IV
 * @param[in] pMsg : buffer containing AAD (padded) + Msg (padded)
 * @param[in] nMsgLen : length of Msg
 * @param[in] crypto_len : length of Msg (unpadded)
 * @param[in] crypto_offset : length of AAD (unpadded, without length field)
 * @param[out] output : output buffer
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_ccm(crypto_lib_handle *pHandle,
				BCM_SCAPI_ENCR_ALG encr_alg,
				BCM_SCAPI_CIPHER_MODE cipher_mode,
				u8_t *pAESKey, u8_t *pAESIV,
				u8_t *pMac, u32_t nMacLen,
				u8_t *pMsg, u32_t nMsgLen,
				u32_t crypto_len, u32_t crypto_offset,
				u8_t *output);

/**
 * This API Perform the AES GCM encryption operation
 *
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] key : buffer containing AES key
 * @param[in] key_len : length of the AES key
 * @param[in] iv : buffer containing AES IV
 * @param[in] iv_len : length of the IV
 * @param[in] plain : plain text buffer pointer
 * @param[in] plain_len : plain text length
 * @param[in] aad : Additional authenticated data
 * @param[in] aad_len : aad length
 * @param[out] crypt : cipher pointer
 * @param[out] tag : tag pointer
 *
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_gcm_encrypt(crypto_lib_handle *pHandle,
						  const u8_t *key,
						  size_t key_len,
						  const u8_t *iv, size_t iv_len,
						  const u8_t *plain,
						  size_t plain_len,
						  const u8_t *aad,
						  size_t aad_len,
						  u8_t *crypt, u8_t *tag);
/**
 * This API Perform the AES GCM decryption operation
 *
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] key : buffer containing AES key
 * @param[in] key_len : length of the AES key
 * @param[in] iv : buffer containing AES IV
 * @param[in] iv_len : length of the IV
 * @param[in] crypt : cipher text buffer pointer
 * @param[in] crypt_len : cipher text length
 * @param[in] aad : Additional authenticated data
 * @param[in] aad_len : aad length
 * @param[] tag : tag pointer
 * @param[out] plain : plain text buffer pointer
 *
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_gcm_decrypt(crypto_lib_handle *pHandle,
						  const u8_t *key,
						  size_t key_len,
						  const u8_t *iv, size_t iv_len,
						  const u8_t *crypt,
						  size_t crypt_len,
						  const u8_t *aad,
						  size_t aad_len,
						  const u8_t *tag, u8_t *plain);

/*
 * Name    : crypto_symmetric_des_encrypt
 * Purpose : Perform a generic DES operation
 *
 * @param[in] handle Crypto context handle
 * @param[in] encr_alg Ecryption alg (des or 3des)
 * @param[in] key Buffer containing key
 * @param[in] key_len Key length
 * @param[in] plain Plain text buffer pointer
 * @param[in] plain_len Plain text length
 * @param[in] iv Buffer containing IV
 * @param[in] iv_len IV length
 * @param[out] Output The calculated des enc
 *
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller
 *
 */
BCM_SCAPI_STATUS crypto_symmetric_des_encrypt(crypto_lib_handle *pHandle,
					      BCM_SCAPI_ENCR_ALG encr_alg,
					      u8_t *key, u8_t key_len,
					      u8_t *plain, u8_t plain_length,
					      u8_t *iv, u8_t iv_length,
					      u8_t *output);

/*
 * Name    : crypto_symmetric_des_decrypt
 * Purpose : Perform a generic DES operation
 * Input   : [in]  pHandle : crypto context handle
 *           [in]  encr_alg : ecryption alg (AES 128, 192, 256)
 *           [in]  key : buffer containing key
 *           [in]  key_len : key length
 *           [in]  cipher : buffer containing cipher
 *           [in]  cipher_length : Cipher length
 *           [in]  iv : buffer containing IV
 *           [in]  iv_len : IV length
 *           [out] output : output buffer
 * Output  : the calculated des dec
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller
 *
 */
BCM_SCAPI_STATUS crypto_symmetric_des_decrypt(crypto_lib_handle *pHandle,
					      BCM_SCAPI_ENCR_ALG encr_alg,
					      u8_t *key, u8_t key_len,
					      u8_t *cipher, u8_t cipher_length,
					      u8_t *iv, u8_t iv_length,
					      u8_t *output);
/**
 * This API perform the AES/HMAC/HASH combination case. (AES+HASH)
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pMsg :Pointer to the Message to be encrypted/decrypted and/ hashed
 * @param[in] nMsgLen : Length of the message.
 * @param[in] pAESKey : Pointer to the AES key.
 * @param[in] AESKeyLen : Lenght of the AES key.
 * @param[in] pAuthKey : Pointer to the HMAC key.
 * @param[in] AuthKeylen : Length of the HMAC key.
 * @param[in] pCipherIV : Pointer to the initialization vector.
 * @param[in] crypto_len : Length of the crypto operations to be carried out
 * @param[in] crypto_offset : Offset of the crypto start in the message
 * @param[in] auth_len : Authentication length
 * @param[in] auth_offset : Authentication offset (Start of the Auth Message)
 * @param[out] HashOutput : Output pointer for the hash data and cipher data
 * @param[in] cipher_mode : Cipher mode #BCM_SCAPI_CIPHER_MODE
 * @param[in] cipher_order : Order of operation, #BCM_SCAPI_CIPHER_ORDER
 * 							 Enc/Auth or Auth/Enc
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_hmacsha256(crypto_lib_handle *pHandle,
									u8_t *pMsg, u32_t nMsgLen,
									u8_t *pAESKey, u32_t AESKeyLen,
									u8_t *pAuthKey, u32_t AuthKeylen,
									u8_t *pCipherIV,
									u32_t crypto_len,u32_t crypto_offset,
									u32_t auth_len,u32_t auth_offset,
			     		    		u8_t *HashOutput,
									BCM_SCAPI_CIPHER_MODE cipher_mode,
									BCM_SCAPI_CIPHER_ORDER cipher_order);
/**
 * This API AES crypto for the encryption or decryption
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] pMsg :Pointer to the Message to be encrypted/decrypted and/ hashed
 * @param[in] nMsgLen : Length of the message.
 * @param[in] pAESKey : Pointer to the AES key.
 * @param[in] AESKeyLen : Lenght of the AES key.
 * @param[in] pCipherIV : Pointer to the initialization vector.
 * @param[in] crypto_len : Length of the crypto operations to be carried out
 * @param[out] outputCipherText : Output pointer for the cipher data
 * @param[in] cipher_mode : Cipher mode #BCM_SCAPI_CIPHER_MODE
 * @param[in] cipher_order : Order of operation, #BCM_SCAPI_CIPHER_ORDER
 * 							 Enc/Auth or Auth/Enc
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_cust_aes_crypto(
									 crypto_lib_handle *pHandle,u8_t *pMsg,
									 u32_t nMsgLen, u8_t *pAESKey,
                                     u32_t AESKeyLen,
                                     u8_t *pCipherIV, u32_t crypto_len,
                                     u8_t *outputCipherText,
                                     BCM_SCAPI_CIPHER_MODE cipher_mode,
                                     BCM_SCAPI_ENCR_MODE   encr_mode);
#endif /* CRYPTO_SYMMETRIC_H */
