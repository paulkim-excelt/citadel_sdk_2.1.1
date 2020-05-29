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
 * @file gcm_alt.c
 *
 * @brief  mbedtls alternate gcm file.
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
 #include <zephyr/types.h>
#if defined(MBEDTLS_GCM_C)
#include <mbedtls/gcm.h>
#include <mbedtls/aes.h>
#include <crypto/mbedtls/aes_alt.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>


#include <string.h>

#if defined(MBEDTLS_AESNI_C)
#include "mbedtls/aesni.h"
#endif

static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = { 0 };
/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
	volatile u8_t *p = v; while (n--) *p++ = 0;
}

/*
 * Initialize a context
 */
void mbedtls_gcm_init(mbedtls_gcm_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_gcm_context));
}

s32_t mbedtls_gcm_setkey(mbedtls_gcm_context *ctx, mbedtls_cipher_id_t cipher,
		       const u8_t *key, u32_t keybits)
{
	unsigned int i;
	u8_t *key_ptr;

	switch (keybits) {
	case 128:
		ctx->key_size = 16;
		break;
	case 192:
		ctx->key_size = 24;
		break;
	case 256:
		ctx->key_size = 32;
		break;
	default:
		return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
	}

	key_ptr = (unsigned char *)ctx->keys;

	for (i = 0; i < ctx->key_size; i++)
		key_ptr[i] = key[i];

	return 0;
}

s32_t mbedtls_gcm_starts(mbedtls_gcm_context *ctx, int mode,
			const u8_t *iv, size_t iv_len,
			const u8_t *add, size_t add_len)
{
	return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

s32_t mbedtls_gcm_update(mbedtls_gcm_context *ctx, size_t length,
			const u8_t *input,
			u8_t *output)
{
	return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

s32_t mbedtls_gcm_finish(mbedtls_gcm_context *ctx, u8_t *tag, size_t tag_len)
{
	return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

s32_t mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
			      s32_t mode, size_t length, const u8_t *iv,
			      size_t iv_len, const u8_t *add,
			      size_t add_len, const u8_t *input,
			      u8_t *output, size_t tag_len,
			      u8_t *tag)
{
	BCM_SCAPI_STATUS rv;

	crypto_init((crypto_lib_handle *) &handle);

	if (mode == MBEDTLS_GCM_ENCRYPT)
		rv = crypto_symmetric_aes_gcm_encrypt(
				(crypto_lib_handle *)handle,
				ctx->keys,
				(ctx->key_size * 8), iv,
					iv_len, input, length, add, add_len,
					output, tag);
	else if (mode == MBEDTLS_GCM_DECRYPT)
		rv = crypto_symmetric_aes_gcm_decrypt(
				(crypto_lib_handle *)handle,
				(u8_t *) &ctx->keys[0],
				(ctx->key_size * 8), iv,
					iv_len, input, length, add, add_len,
					 tag, output);
	else
		return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;

	return rv;
}

s32_t mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *ctx,
			     size_t length, const u8_t *iv,
			     size_t iv_len, const u8_t *add,
			     size_t add_len, const u8_t *tag,
			     size_t tag_len, const u8_t *input,
			     u8_t *output)
{
	BCM_SCAPI_STATUS rv;

	crypto_init((crypto_lib_handle *) &handle);
	rv = crypto_symmetric_aes_gcm_decrypt((crypto_lib_handle *)handle,
			(u8_t *) &ctx->keys[0], ctx->key_size, iv,
					iv_len, input, length, add, add_len,
					tag, output);

	return rv;
}

void mbedtls_gcm_free(mbedtls_gcm_context *ctx)
{
	mbedtls_zeroize(ctx, sizeof(mbedtls_gcm_context));
}

#endif /* MBEDTLS_GCM_C */
