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

#ifndef __CRYPTO_UTIL_H__
#define __CRYPTO_UTIL_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <misc/printk.h>

#define crypto_debug printk

#define CRYPTO_DEBUG(str)                                                      \
do {                                                                           \
    crypto_debug("%s(%d): %s\n", __FUNCTION__, __LINE__, (str));              \
}while(0)
#define CRYPTO_RESULT(result, str)                                             \
do {                                                                           \
    if ((result) != 0) {                                                       \
        CRYPTO_DEBUG(str);                                                     \
        return (result);                                                       \
    }                                                                          \
}while(0)

#define CRYPTO_DATA_SIZE     (128*1024)
#define AES128_KEY_SIZE      16
#define AES256_KEY_SIZE      32
#define CRYPTO_DES_KEY_SIZE  8
#define CRYPTO_3DES_KEY_SIZE 24
#define SHA1_HASH_SIZE       20
#define SHA256_HASH_SIZE     32
#define DPA_KEY_SIZE         6
#define CCM_FLAG_A  0
#define CCM_FLAG_M  16
#define CCM_FLAG_L  4

#define crypto_assert(value)                                                \
do {                                                                        \
    if (0 == (value)) {                                                     \
        crypto_debug("%s(%d): assert failed!\n", __FUNCTION__, __LINE__);   \
        return -1;                                                          \
    }                                                                       \
}while(0)

int32_t crypto_block_encrypt_auth(
        uint8_t *enc_key, uint32_t enc_key_length,
        uint8_t *auth_key, uint32_t auth_key_length,
        uint8_t *iv_plain, uint32_t iv_length, uint32_t plain_length,
        uint8_t *cipher);

int32_t crypto_aes_128_cbc_encrypt(
        uint8_t *key, uint32_t key_length,
        uint8_t *plain, uint32_t plain_length,
        uint8_t *iv, uint32_t iv_length,
        uint8_t *cipher);

int32_t crypto_aes_128_cbc_decrypt(
        uint8_t *key, uint32_t key_length,
        uint8_t *cipher, uint32_t cipher_length,
        uint8_t *iv, uint32_t iv_length,
        uint8_t *plain);

int32_t crypto_hmac_sha256_digest(
        uint8_t *key,
        uint32_t key_length,
        uint8_t *data,
        uint32_t data_length,
        uint8_t *digest);

#endif /* __CRYPTO_UTIL_H__ */

