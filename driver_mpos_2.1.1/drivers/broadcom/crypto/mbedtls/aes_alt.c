/******************************************************************************
 *  Copyright (C) 2019 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/*
 * @file aes_alt.c
 *
 * @brief  mbedtls alternate aes file.
 */

#include <logging/sys_log.h>
#include <mbedtls/aes.h>
#include <crypto/mbedtls/aes_alt.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <pka/crypto_pka_util.h>
#include <string.h>
#include <stdbool.h>

#ifdef MBEDTLS_AES_ALT
static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = { 0 };

static void mbedtls_zeroize(void *v, size_t n)
{
	volatile unsigned char *p = (unsigned char *)v;

	while (n--)
		*p++ = 0;
}

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
	if (ctx == NULL)
		return;

	mbedtls_zeroize(ctx, sizeof(mbedtls_aes_context));
}

/*
 * AES key schedule (encryption)
 */
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
			   unsigned int keybits)
{
	unsigned int i;
	unsigned char *key_ptr;

	switch (keybits) {
	case 128:
		ctx->keySize = 16;
		break;
	case 192:
		ctx->keySize = 24;
		break;
	case 256:
		ctx->keySize = 32;
		break;
	default:
		return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
	}

	key_ptr = (unsigned char *)ctx->keys;

	for (i = 0; i < ctx->keySize; i++)
		key_ptr[i] = key[i];

	return 0;
}

/*
 * AES key schedule (decryption)
 */
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
			   unsigned int keybits)
{
	mbedtls_aes_setkey_enc(ctx, key, keybits);
	return 0;
}

/*
 * AES-ECB block encryption/decryption
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *aes_ctx,
			  int mode,
			  const unsigned char input[16],
			  unsigned char output[16])
{
	s32_t rv = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	crypto_init((crypto_lib_handle *) &handle);

	if (mode == MBEDTLS_AES_ENCRYPT)
		cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_ENCRYPT;
	else
		cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT;

	switch (aes_ctx->keySize) {
	case AES_128_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128;
		break;
	case AES_192_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_192;
		break;
	case AES_256_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256;
		break;
	default:
		return -1;
		/* invalid key length */
		break;
	}

	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rv = crypto_smau_cipher_init((crypto_lib_handle *) handle, &cmd,
				     (u8_t *) &aes_ctx->keys[0], NULL, 0,
				     &ctx);
	if (rv != BCM_SCAPI_STATUS_OK) {
		SYS_LOG_DBG("cipher init failed %d\n", rv);
		return -1;
	}

	rv = crypto_smau_cipher_start((crypto_lib_handle *) handle,
				      (u8_t *) &input[0], 16, NULL, 16, 0, 0,
				      0, (u8_t *) &output[0], &ctx);
	if (rv != BCM_SCAPI_STATUS_OK) {
		SYS_LOG_DBG("cipher start failed %d\n", rv);
		return -1;
	}

	return 0;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * AES-CBC buffer encryption/decryption
 */
int mbedtls_aes_crypt_cbc(mbedtls_aes_context *aes_ctx,
			  int mode,
			  size_t len,
			  unsigned char iv[16],
			  const unsigned char *input, unsigned char *output)
{
	BCM_SCAPI_STATUS status;
	BCM_SCAPI_ENCR_ALG encr_alg;

	crypto_init((crypto_lib_handle *) &handle);
	switch (aes_ctx->keySize) {
	case AES_128_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_128;
		break;
	case AES_192_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_192;
		break;
	case AES_256_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_256;
		break;
	default:
		return BCM_SCAPI_STATUS_CRYPTO_ENCR_UNSUPPORTED;
	}

	if (mode == MBEDTLS_AES_ENCRYPT)
		/* test encrypt */
		status = crypto_symmetric_fips_aes((crypto_lib_handle *) handle,
						BCM_SCAPI_ENCR_MODE_CBC,
						encr_alg,
						BCM_SCAPI_CIPHER_MODE_ENCRYPT,
						(u8_t *) &aes_ctx->keys[0],
						iv, (u8_t *)input,
						len, len, 0, (u8_t *)output);
	else
		status = crypto_symmetric_fips_aes((crypto_lib_handle *) handle,
						BCM_SCAPI_ENCR_MODE_CBC,
						encr_alg,
						BCM_SCAPI_CIPHER_MODE_DECRYPT,
						(u8_t *) &aes_ctx->keys[0],
						iv, (u8_t *)input,
						len, len, 0, (u8_t *)output);

	if (status != BCM_SCAPI_STATUS_OK)
		return -1;

	return 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
int mbedtls_aes_crypt_cfb128(mbedtls_aes_context *ctx,
			     int mode,
			     size_t length,
			     size_t *iv_off,
			     unsigned char iv[16],
			     const unsigned char *input, unsigned char *output)
{
	return 0;
}

/*
 * AES-CFB8 buffer encryption/decryption
 */
int mbedtls_aes_crypt_cfb8(mbedtls_aes_context *ctx,
			   int mode,
			   size_t length,
			   unsigned char iv[16],
			   const unsigned char *input, unsigned char *output)
{
	return 0;
}
#endif /*MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 * AES-CTR buffer encryption/decryption
 */
int mbedtls_aes_crypt_ctr(mbedtls_aes_context *aes_ctx,
			  size_t length,
			  size_t *nc_off,
			  unsigned char nonce_counter[16],
			  unsigned char stream_block[16],
			  const unsigned char *input, unsigned char *output)
{
	BCM_SCAPI_STATUS status;
	BCM_SCAPI_ENCR_ALG encr_alg;
	u8_t mode = MBEDTLS_AES_DECRYPT;

	crypto_init((crypto_lib_handle *) &handle);
	switch (aes_ctx->keySize) {
	case AES_128_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_128;
		break;
	case AES_192_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_192;
		break;
	case AES_256_KEY_LEN_BYTES:
		encr_alg = BCM_SCAPI_ENCR_ALG_AES_256;
		break;
	default:
		return BCM_SCAPI_STATUS_CRYPTO_ENCR_UNSUPPORTED;
	}

	if (mode == MBEDTLS_AES_ENCRYPT)
		/* test encrypt */
		status = crypto_symmetric_fips_aes((crypto_lib_handle *) handle,
						BCM_SCAPI_ENCR_MODE_CTR,
						encr_alg,
						BCM_SCAPI_CIPHER_MODE_ENCRYPT,
						(u8_t *) &aes_ctx->keys[0],
						nonce_counter,
						(u8_t *)input, length, length,
						0, (u8_t *)output);
	else
		status = crypto_symmetric_fips_aes((crypto_lib_handle *) handle,
						BCM_SCAPI_ENCR_MODE_CTR,
						encr_alg,
						BCM_SCAPI_CIPHER_MODE_DECRYPT,
						(u8_t *) &aes_ctx->keys[0],
						nonce_counter,
						(u8_t *)input, length, length,
						0, (u8_t *)output);
	if (status != BCM_SCAPI_STATUS_OK)
		return -1;

	return 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CTR */
#endif
