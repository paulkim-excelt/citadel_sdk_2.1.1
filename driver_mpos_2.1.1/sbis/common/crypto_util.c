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

#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto_util.h>

int32_t crypto_block_encrypt_auth(
        uint8_t *enc_key, uint32_t enc_key_length,
        uint8_t *auth_key, uint32_t auth_key_length,
        uint8_t *iv_plain, uint32_t iv_length, uint32_t plain_length,
        uint8_t *cipher)
{
    int32_t rc = 0;
    uint8_t *ptr = iv_plain + iv_length + plain_length + iv_length + iv_length;
    uint8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
    crypto_lib_handle *pHandle = (crypto_lib_handle *)cryptoHandle;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cmd;

    crypto_assert(enc_key != NULL);
    crypto_assert(auth_key != NULL);
    crypto_assert(iv_plain != NULL);
    crypto_assert(cipher != NULL);
    crypto_assert(enc_key_length == AES128_KEY_SIZE);
    crypto_assert(iv_length == 16);
    crypto_assert(plain_length > 0);
    crypto_assert(plain_length <= CRYPTO_DATA_SIZE);

    cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_ENCRYPT;    /* encrypt or decrypt */
    cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_AES_128;   /* encryption algorithm */
    cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CBC;
    cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_SHA256;
    cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
    cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;
    cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH;

    rc = crypto_smau_cipher_init(pHandle, &cmd, enc_key, auth_key, auth_key_length, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_init failed");

    rc = crypto_smau_cipher_start(
                pHandle, iv_plain, iv_length+plain_length, iv_plain, plain_length, iv_length, iv_length+plain_length, 0, cipher, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_start failed");

    return rc;
}

int32_t crypto_aes_128_cbc_encrypt(
        uint8_t *key, uint32_t key_length,
        uint8_t *plain, uint32_t plain_length,
        uint8_t *iv, uint32_t iv_length,
        uint8_t *cipher)
{
    int32_t rc = 0;
    uint8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
    crypto_lib_handle *pHandle = (crypto_lib_handle *)cryptoHandle;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cmd;

    crypto_assert(key != NULL);
    crypto_assert(plain != NULL);
    crypto_assert(iv != NULL);
    crypto_assert(cipher != NULL);
    crypto_assert(key_length == AES128_KEY_SIZE);
    crypto_assert(iv_length == 16);
    crypto_assert(plain_length > 0);
    crypto_assert(plain_length <= CRYPTO_DATA_SIZE);

    cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_ENCRYPT;    /* encrypt or decrypt */
    cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_AES_128;   /* encryption algorithm */
    cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CBC;
    cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;    /* authentication algorithm */
    cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
    cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_INIT;
    cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;    /* encrypt first or auth first */

    rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_init failed");

    rc = crypto_smau_cipher_start(
                pHandle, plain, plain_length, iv, plain_length, 0, 0, 0, cipher, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_start failed");

    return rc;
}

int32_t crypto_aes_128_cbc_decrypt(
        uint8_t *key, uint32_t key_length,
        uint8_t *cipher, uint32_t cipher_length,
        uint8_t *iv, uint32_t iv_length,
        uint8_t *plain)
{
    int32_t rc = 0;
    uint8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
    crypto_lib_handle *pHandle = (crypto_lib_handle *)cryptoHandle;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cmd;

    crypto_assert(key != NULL);
    crypto_assert(cipher != NULL);
    crypto_assert(iv != NULL);
    crypto_assert(plain != NULL);
    crypto_assert(key_length == AES128_KEY_SIZE);
    crypto_assert(iv_length == 16);
    crypto_assert(cipher_length > 0);
    crypto_assert(cipher_length <= CRYPTO_DATA_SIZE);

    cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_DECRYPT;    /* encrypt or decrypt */
    cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_AES_128;   /* encryption algorithm */
    cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CBC;
    cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;    /* authentication algorithm */
    cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
    cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_INIT;
    cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;    /* encrypt first or auth first */

    rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_init failed");
    rc = crypto_smau_cipher_start(
                pHandle, cipher, cipher_length, iv, cipher_length, 0, 0, 0, plain, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_start failed");

    return rc;
}

int32_t crypto_hmac_sha256_digest(uint8_t *key, uint32_t key_length, uint8_t *data, uint32_t data_length, uint8_t *digest)
{
    int32_t rc = 0;
    uint8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
    crypto_lib_handle *pHandle = (crypto_lib_handle *)cryptoHandle;
    bcm_bulk_cipher_context ctx;
    BCM_BULK_CIPHER_CMD     cmd;

    crypto_assert(key != NULL);
    crypto_assert(data != NULL);
    crypto_assert(digest != NULL);
    crypto_assert(data_length > 0);
    crypto_assert(data_length <= CRYPTO_DATA_SIZE);

    cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
    cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_NONE;
    cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_NONE;
    cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_SHA256;
    cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HMAC;
    cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;
    cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT;

    rc = crypto_smau_cipher_init(pHandle, &cmd, NULL, key, key_length, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_init failed");
    rc = crypto_smau_cipher_start(
                pHandle, data, data_length, NULL, 0, 0, data_length, 0, digest, &ctx);
    CRYPTO_RESULT(rc, "bcm_bulk_cipher_start failed");

    return rc;
}


