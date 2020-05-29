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

#ifndef  CRYPTO_SMAU_H
#define  CRYPTO_SMAU_H


#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <stdlib.h>
#include <zephyr/types.h>
#include "crypto.h"

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/

#define reg32setbit(addr , num)    (reg32_write(addr, reg32_read(addr) |\
									(1<< num)))
#define reg32clrbit(addr , num)    (reg32_write(addr, reg32_read(addr) & \
									(0xffffffff ^ (1<< num))))
#ifndef reg32_read
#define reg32_read(addr)   *((volatile u32_t *)(addr))
#define reg32_write(addr,data)   *((volatile u32_t *)(addr)) = (u32_t)(data)
#define reg8_write(addr,data)    *((volatile u8_t *)(addr)) = (u8_t)(data)
#endif

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/

typedef enum  {ZERO=0, AES=1, DES=2, _3DES=3} sctx_crypt_algo_e;
typedef enum  {AUTH_DISABLE=0, AUTH_ENABLE=1} sctx_crypt_auth_en;
typedef enum  {ENCR_AUTH=0, AUTH_ENCR=1} sctx_order_e;
typedef enum  {DECRYPT=0, ENCRYPT=1} sctx_encrypt_decrypt;
typedef enum  {CBC=0, CTR=1, ECB=2, CCM=5, CMAC=6 , RSVD=8} sctx_crypt_mode_e;
typedef enum  {AES_128=0, AES_192=1, AES_256=2 } sctx_crypt_aes_key_size;
typedef enum  {SHA1=0, SHA2=1 } sctx_crypt_sha;
typedef enum  {SHA_DIS=0,HCTX=1, HASH=2,HMAC=4,FMAC=8} sctx_hash_mode_e;
typedef enum  {INIT=1, UPDT=2, FIN=4,CMPLTE=8} sctx_hash_type_e;
typedef enum  {HMAC_KEY_SZ_20bytes=0, HMAC_KEY_SZ_FRM_AUTH_KEY_LEN=1} sctx_auth_key_len_sel;

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/
/**
 * This API initializes the crypto. This API has to be called before any
 * operations of PKA, symmetric crypto.
 * @param[out] pHandle : Crypto handle
 * @return None
 * @retval
 */
void crypto_init(crypto_lib_handle *pHandle);

/**
 * This API deinitalizes the symmetric crypto.
 * @param[in] pHandle : Crypto handle
 * @param[in] ctx : Crypto bulk cipher context
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_smau_cipher_deinit (
                    crypto_lib_handle *pHandle,
                    bcm_bulk_cipher_context *ctx);

/**
 * This API resets the dma and cache blocks of the smau
 */
void crypto_smau_reset_blocks(void);

/**
 * This API initializes the symmetric cipher engine (Symmetric algos+Hash/HMAC)
 * @param[in] pHandle : Crypto handle
 * @param[in] cmd : Crypto commands #BCM_BULK_CIPHER_CMD
 * @param[in] encr_key : Pointer to the encryption key
 * @param[in] auth_key : Pointer to the authentication key
 * @param[in] auth_key_len : Authentication key length
 * @param[in] ctx : Crypto bulk cipher context
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_smau_cipher_init (
                    crypto_lib_handle *pHandle,
                    BCM_BULK_CIPHER_CMD *cmd,
                    u8_t *encr_key,
                    u8_t *auth_key,
                    u32_t auth_key_len,
                    bcm_bulk_cipher_context *ctx);

/**
 * This API starts the cipher operation (Symmetric algos+Hash/HMAC)
 * @param[in] pHandle : Crypto handle
 * @param[in] data_in : Pointer to the input data
 * @param[in] data_len : Length of the total data message
 * @param[in] iv : Pointer to the initialization vector
 * @param[in] crypto_len : Length of the crypto
 * @param[in] crypto_offset : Crypto offset from the message
 * @param[in] auth_len : Length of the message to be authenticated
 * @param[in] auth_offset : Authentication offset from the message
 * @param[out] output : Pointer to the output
 * @param[in] ctx : Crypto bulk cipher context
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_smau_cipher_start( crypto_lib_handle *pHandle,
										u8_t *data_in,
										u32_t data_len,
										u8_t *iv,
										u32_t crypto_len,
										u16_t crypto_offset,
										u32_t auth_len,
										u16_t auth_offset,
										u8_t *output,
										bcm_bulk_cipher_context *ctx);
/** @} */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* CRYPTO_SMAU_H */
