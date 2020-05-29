/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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
 * @file sha1_alt.c
 * @brief sha1 alternate driver implementation
 */

#include "mbedtls/sha1.h"
#include "mbedtls/cipher.h"

#include <logging/sys_log.h>
#include <crypto/mbedtls/sha1_alt.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <pka/crypto_pka_util.h>
#include <string.h>
#include <stdbool.h>

#if defined(MBEDTLS_SHA1_ALT)
#include "mbedtls/platform.h"

#undef mbedtls_printf
#define mbedtls_printf printk

static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = {0};

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
	volatile u8_t *p = v;

	while (n--) {
		*p++ = 0;
	}
}

void mbedtls_sha1_init(mbedtls_sha1_context *ctx)
{
	if (ctx == NULL)
		return;

	memset(ctx, 0, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_free(mbedtls_sha1_context *ctx)
{
	if (ctx == NULL)
		return;

	mbedtls_zeroize(ctx, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
			  const mbedtls_sha1_context *src)
{
	*dst = *src;
}

#ifdef MBEDTLS_VERSION_2_16
void mbedtls_sha1_starts(mbedtls_sha1_context *ctx)
{
	mbedtls_sha1_starts_ret(ctx);
}
s32_t mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx)
#else
s32_t mbedtls_sha1_starts(mbedtls_sha1_context *ctx)
#endif
{
	ctx->init = 0;
	ctx->end = 0;
	ctx->last_block_present = 0;
	crypto_init((crypto_lib_handle *)&handle);

	return 0;
}

#ifdef MBEDTLS_VERSION_2_16
s32_t mbedtls_internal_sha1_process(mbedtls_sha1_context *ctx,
			   const u8_t data[BLOCK_SIZE])
#else
s32_t mbedtls_sha1_process(mbedtls_sha1_context *ctx,
			   const u8_t data[BLOCK_SIZE])
#endif
{
	return 0;
}

#ifdef MBEDTLS_VERSION_2_16
void mbedtls_sha1_update(mbedtls_sha1_context *ctx, const u8_t *input,
				size_t ilen)
{
	mbedtls_sha1_update_ret(ctx, input, ilen);
}
s32_t mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx, const u8_t *input,
				size_t ilen)
#else
s32_t mbedtls_sha1_update(mbedtls_sha1_context *ctx, const u8_t *input,
				size_t ilen)
#endif
{
	BCM_SCAPI_STATUS rv;

	if (ctx->end) {
		printk("Session finished\n");
		return -1;
	}

	if (ilen == 0)
		return -1;

	if (ctx->last_block_present) {
		if (!ctx->init) {
			rv = crypto_symmetric_hmac_sha_init(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, ctx->last_block,
				BLOCK_SIZE, NULL, 0, ctx->digest);
			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

			ctx->init = TRUE;
			ctx->last_block_present = 0;
		} else {
			rv = crypto_symmetric_hmac_sha_update(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, ctx->last_block,
				BLOCK_SIZE, NULL, 0,
				ctx->digest, ctx->digest);
			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

			ctx->last_block_present = 0;
		}
		ctx->total_len = ctx->total_len + BLOCK_SIZE;
	}

	/* SHA finished start again */
	if ((ilen % SHA256_BLOCK_SIZE) != 0) {
		if (!ctx->init) {
			/* Call complete sha generation function */
			rv = crypto_symmetric_hmac_sha1(
				(crypto_lib_handle *)handle, (u8_t *)input,
				ilen, NULL, 0, ctx->digest, FALSE);
			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

			ctx->init = TRUE;

		} else {
			ctx->total_len = ctx->total_len + ilen;
			/* Call finish function */
			rv = crypto_symmetric_hmac_sha_final(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, (u8_t *)input,
				ilen, ctx->total_len, NULL, 0, ctx->digest,
				ctx->digest);
			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
		}
		ctx->end = TRUE;
	} else {
		secure_memcpy(ctx->last_block,
				&input[ilen - BLOCK_SIZE],
				BLOCK_SIZE);
		ilen = ilen - BLOCK_SIZE;
		ctx->last_block_present = 1;

		if (ilen == 0)
			return 0;


		if (!ctx->init) {
			rv = crypto_symmetric_hmac_sha_init(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, (u8_t *)input,
				ilen, NULL, 0, ctx->digest);
			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

			ctx->init = TRUE;
			ctx->total_len = ctx->total_len + ilen;
		} else {
			rv = crypto_symmetric_hmac_sha_update(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, (u8_t *)input,
				ilen, NULL, 0, ctx->digest, ctx->digest);

			if (rv)
				return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

			ctx->total_len = ctx->total_len + ilen;
		}
	}

	return 0;
}

#ifdef MBEDTLS_VERSION_2_16
void mbedtls_sha1_finish(mbedtls_sha1_context *ctx, u8_t output[32])
{
	mbedtls_sha1_finish_ret(ctx, output);
}
s32_t mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx, u8_t output[32])
#else
s32_t mbedtls_sha1_finish(mbedtls_sha1_context *ctx, u8_t output[32])
#endif
{
	BCM_SCAPI_STATUS rv;

	if (ctx->last_block_present) {
		ctx->total_len = ctx->total_len + BLOCK_SIZE;
		rv = crypto_symmetric_hmac_sha_final(
				(crypto_lib_handle *)handle,
				BCM_SCAPI_AUTH_ALG_SHA1, ctx->last_block,
				BLOCK_SIZE, ctx->total_len, NULL, 0,
				ctx->digest, ctx->digest);
		if (rv)
			return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

		ctx->last_block_present = 0;
	}

	secure_memcpy(output, ctx->digest, SHA256_HASH_SIZE);

	return 0;
}

#endif /*MBEDTLS_SHA1_ALT*/
