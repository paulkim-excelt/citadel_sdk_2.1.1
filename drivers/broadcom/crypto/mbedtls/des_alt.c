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
 * @file des_alt.c
 *
 * @brief  mbedtls alternate des file.
 */

#include "mbedtls/des.h"

#if defined(MBEDTLS_DES_C)
#if defined(MBEDTLS_DES_ALT)

#include <logging/sys_log.h>
#include <string.h>
#include <stdbool.h>
#include <crypto/mbedtls/des_alt.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <pka/crypto_pka_util.h>
#include <string.h>
#include <stdbool.h>

static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = {0};

static void mbedtls_zeroize(void *v, size_t n)
{
	volatile unsigned char *p = (unsigned char *)v;

	while (n--)
		*p++ = 0;
}

void mbedtls_des_init(mbedtls_des_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_des_context));
}

void mbedtls_des_free(mbedtls_des_context *ctx)
{
	if (ctx == NULL)
		return;

	mbedtls_zeroize(ctx, sizeof(mbedtls_des_context));
}

void mbedtls_des3_init(mbedtls_des3_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_des3_context));
}

void mbedtls_des3_free(mbedtls_des3_context *ctx)
{
	if (ctx == NULL)
		return;

	mbedtls_zeroize(ctx, sizeof(mbedtls_des3_context));
}

static const unsigned char odd_parity_table[128] = {
	 1,  2,  4,  7,  8, 11, 13, 14,
	 16, 19, 21, 22, 25, 26, 28, 31,
	 32, 35, 37, 38, 41, 42, 44, 47,
	 49, 50, 52, 55, 56, 59, 61, 62,
	 64, 67, 69, 70, 73, 74, 76, 79,
	 81, 82, 84, 87, 88, 91, 93, 94,
	 97, 98, 100, 103, 104, 107, 109, 110,
	 112, 115, 117, 118, 121, 122, 124, 127,
	 128, 131, 133, 134, 137, 138, 140, 143,
	 145, 146, 148, 151, 152, 155, 157, 158,
	 161, 162, 164, 167, 168, 171, 173, 174,
	 176, 179, 181, 182, 185, 186, 188, 191,
	 193, 194, 196, 199, 200, 203, 205, 206,
	 208, 211, 213, 214, 217, 218, 220, 223,
	 224, 227, 229, 230, 233, 234, 236, 239,
	 241, 242, 244, 247, 248, 251, 253, 254
};

void mbedtls_des_key_set_parity(unsigned char key[MBEDTLS_DES_KEY_SIZE])
{
	int i;

	for (i = 0; i < MBEDTLS_DES_KEY_SIZE; i++)
		key[i] = odd_parity_table[key[i] / 2];
}

/*
 * Check the given key's parity, returns 1 on failure, 0 on SUCCESS
 */
int mbedtls_des_key_check_key_parity(
				const unsigned char key[MBEDTLS_DES_KEY_SIZE])
{
	int i;

	for (i = 0; i < MBEDTLS_DES_KEY_SIZE; i++)
		if (key[i] != odd_parity_table[key[i] / 2])
			return 1;

	return 0;
}

/*
 * Table of weak and semi-weak keys
 *
 * Source: http://en.wikipedia.org/wiki/Weak_key
 *
 * Weak:
 * Alternating ones + zeros (0x0101010101010101)
 * Alternating 'F' + 'E' (0xFEFEFEFEFEFEFEFE)
 * '0xE0E0E0E0F1F1F1F1'
 * '0x1F1F1F1F0E0E0E0E'
 *
 * Semi-weak:
 * 0x011F011F010E010E and 0x1F011F010E010E01
 * 0x01E001E001F101F1 and 0xE001E001F101F101
 * 0x01FE01FE01FE01FE and 0xFE01FE01FE01FE01
 * 0x1FE01FE00EF10EF1 and 0xE01FE01FF10EF10E
 * 0x1FFE1FFE0EFE0EFE and 0xFE1FFE1FFE0EFE0E
 * 0xE0FEE0FEF1FEF1FE and 0xFEE0FEE0FEF1FEF1
 *
 */

#define WEAK_KEY_COUNT 16

static const unsigned char weak_key_table
				[WEAK_KEY_COUNT][MBEDTLS_DES_KEY_SIZE] = {
	{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
	{0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE},
	{0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E},
	{0xE0, 0xE0, 0xE0, 0xE0, 0xF1, 0xF1, 0xF1, 0xF1},
	{0x01, 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E},
	{0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E, 0x01},
	{0x01, 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1},
	{0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1, 0x01},
	{0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE},
	{0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01},
	{0x1F, 0xE0, 0x1F, 0xE0, 0x0E, 0xF1, 0x0E, 0xF1},
	{0xE0, 0x1F, 0xE0, 0x1F, 0xF1, 0x0E, 0xF1, 0x0E},
	{0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E, 0xFE},
	{0xFE, 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E},
	{0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE},
	{0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1}
};

int mbedtls_des_key_check_weak(const unsigned char key[MBEDTLS_DES_KEY_SIZE])
{
	int i;

	for (i = 0; i < WEAK_KEY_COUNT; i++)
		if (memcmp(weak_key_table[i], key, MBEDTLS_DES_KEY_SIZE) == 0)
			return 1;

	return 0;
}

/*
 * DES key schedule (56-bit, encryption)
 */
int mbedtls_des_setkey_enc(mbedtls_des_context *ctx,
			   const unsigned char key[MBEDTLS_DES_KEY_SIZE])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE);
	ctx->enc = 1;

	return 0;
}

/*
 * DES key schedule (56-bit, decryption)
 */
int mbedtls_des_setkey_dec(mbedtls_des_context *ctx,
			   const unsigned char key[MBEDTLS_DES_KEY_SIZE])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE);
	ctx->enc = 0;

	return 0;
}

/*
 * Triple-DES key schedule (112-bit, encryption)
 */
int mbedtls_des3_set2key_enc(mbedtls_des3_context *ctx,
			     const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE * 2);
	memcpy(&ctx->sk[MBEDTLS_DES_KEY_SIZE * 2], key, MBEDTLS_DES_KEY_SIZE);
	ctx->enc = 1;

	return 0;
}

/*
 * Triple-DES key schedule (112-bit, decryption)
 */
int mbedtls_des3_set2key_dec(mbedtls_des3_context *ctx,
			     const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE * 2);
	memcpy(&ctx->sk[MBEDTLS_DES_KEY_SIZE * 2], key, MBEDTLS_DES_KEY_SIZE);
	ctx->enc = 0;

	return 0;
}

/*
 * Triple-DES key schedule (168-bit, encryption)
 */
int mbedtls_des3_set3key_enc(mbedtls_des3_context *ctx,
			     const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE * 3);
	ctx->enc = 1;

	return 0;
}

/*
 * Triple-DES key schedule (168-bit, decryption)
 */
int mbedtls_des3_set3key_dec(mbedtls_des3_context *ctx,
			     const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3])
{
	memcpy(ctx->sk, key, MBEDTLS_DES_KEY_SIZE * 3);
	ctx->enc = 0;

	return 0;
}

/*
 * DES-ECB block encryption/decryption
 */
int mbedtls_des_crypt_ecb(mbedtls_des_context *ctx,
			  const unsigned char input[8],
			  unsigned char output[8])
{
	u8_t cipher[8]__aligned(64) = {0};
	int rv;

	crypto_init((crypto_lib_handle *)&handle);

	if (ctx->enc)
		rv = crypto_symmetric_des_encrypt((crypto_lib_handle *)handle,
						  BCM_SCAPI_ENCR_ALG_DES,
						  ctx->sk, 8,
						  (unsigned char *)input, 8,
						  NULL, 0, cipher);
	else
		rv = crypto_symmetric_des_decrypt((crypto_lib_handle *)handle,
						  BCM_SCAPI_ENCR_ALG_DES,
						  ctx->sk, 8,
						  (unsigned char *)input, 8,
						  NULL, 0, cipher);
	memcpy(output, cipher, 8);

	return rv;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * DES-CBC buffer encryption/decryption
 */
int mbedtls_des_crypt_cbc(mbedtls_des_context *ctx,
			  int mode,
			  size_t length,
			  unsigned char iv[8],
			  const unsigned char *input,
			  unsigned char *output)
{
	u8_t buf[8]__aligned(64) = {0};
	int rv;

	crypto_init((crypto_lib_handle *)&handle);

	if (mode == MBEDTLS_DES_ENCRYPT)
		rv = crypto_symmetric_des_encrypt((crypto_lib_handle *)handle,
				 BCM_SCAPI_ENCR_ALG_DES, ctx->sk, 8,
				 (unsigned char *)input, length,
				 iv, 8, buf);
	else
		rv = crypto_symmetric_des_decrypt((crypto_lib_handle *)handle,
				 BCM_SCAPI_ENCR_ALG_DES, ctx->sk, 8,
				 (unsigned char *)input, length,
				 iv, 8, buf);
	memcpy(output, buf, 8);

	return rv;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/*
 * 3DES-ECB block encryption/decryption
 */
int mbedtls_des3_crypt_ecb(mbedtls_des3_context *ctx,
			   const unsigned char input[8],
			   unsigned char output[8])
{
	u8_t buf[8]__aligned(64) = {0};
	int rv;

	crypto_init((crypto_lib_handle *)&handle);

	if (ctx->enc)
		rv = crypto_symmetric_des_encrypt((crypto_lib_handle *)handle,
						  BCM_SCAPI_ENCR_ALG_3DES,
						  ctx->sk, 24,
						  (unsigned char *)input, 8,
						  NULL, 0, buf);
	else
		rv = crypto_symmetric_des_decrypt((crypto_lib_handle *)handle,
						  BCM_SCAPI_ENCR_ALG_3DES,
						  ctx->sk, 24,
						  (unsigned char *)input, 8,
						  NULL, 0, buf);

	memcpy(output, buf, 8);

	return rv;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * 3DES-CBC buffer encryption/decryption
 */
int mbedtls_des3_crypt_cbc(mbedtls_des3_context *ctx,
			   int mode,
			   size_t length,
			   unsigned char iv[8],
			   const unsigned char *input,
			   unsigned char *output)
{
	u8_t buf[8]__aligned(64) = {0};
	int rv;

	crypto_init((crypto_lib_handle *)&handle);

	if (mode == MBEDTLS_DES_ENCRYPT)
		rv = crypto_symmetric_des_encrypt((crypto_lib_handle *)handle,
				 BCM_SCAPI_ENCR_ALG_3DES, ctx->sk, 24,
				 (unsigned char *)input, length, iv, 8, buf);
	else
		rv = crypto_symmetric_des_decrypt((crypto_lib_handle *)handle,
				 BCM_SCAPI_ENCR_ALG_3DES, ctx->sk, 24,
				 (unsigned char *)input, length, iv, 8, buf);

	memcpy(output, buf, 8);

	return rv;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#endif /* MBEDTLS_DES_ALT */
#endif /* MBEDTLS_DES_C */
