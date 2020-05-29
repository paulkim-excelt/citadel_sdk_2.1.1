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
 * @file rsa_alt.c
 * @brief rsa alternate driver implementation
 */

#include <logging/sys_log.h>
#include <mbedtls/aes.h>
#include <crypto/mbedtls/aes_alt.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <string.h>
#include <stdbool.h>
#include <misc/printk.h>
#include "mbedtls/bignum.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/rsa.h"
#include "mbedtls/oid.h"

#include <string.h>
#if defined(MBEDTLS_PKCS1_V15) && !defined(__OpenBSD__)
#include <stdlib.h>
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#endif

#if defined(MBEDTLS_RSA_ALT)
static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = { 0 };

int rand(void)
{
	return 1;
}

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
	volatile unsigned char *p = (unsigned char *)v;

	while (n--)
		*p++ = 0;
}

/*
 * Initialize an RSA context
 */
void mbedtls_rsa_init(mbedtls_rsa_context *ctx, int padding, int hash_id)
{
	memset(ctx, 0, sizeof(mbedtls_rsa_context));

	mbedtls_rsa_set_padding(ctx, padding, hash_id);

#if defined(MBEDTLS_THREADING_C)
	mbedtls_mutex_init(&ctx->mutex);
#endif
}

/*
 * Set padding for an existing RSA context
 */
void mbedtls_rsa_set_padding(mbedtls_rsa_context *ctx, int padding,
			     int hash_id)
{
	ctx->padding = padding;
	ctx->hash_id = hash_id;
}

/*
 * Generate an RSA keypair
 */
int mbedtls_rsa_gen_key(mbedtls_rsa_context *ctx,
			int (*f_rng)(void *, unsigned char *, size_t),
			void *p_rng, unsigned int nbits, int exponent)
{
	BCM_SCAPI_STATUS rv;
	mbedtls_mpi test;
	unsigned int e_bits, p_bits, q_bits, n_bits, d_bits, dp_bits, dq_bits;

	crypto_init((crypto_lib_handle *) &handle);
	test.n = 1;
	test.p = &exponent;
	e_bits = mbedtls_mpi_bitlen(&test);
	p_bits = mbedtls_mpi_bitlen(&ctx->P);
	q_bits = mbedtls_mpi_bitlen(&ctx->Q);
	n_bits = mbedtls_mpi_bitlen(&ctx->N);
	d_bits = mbedtls_mpi_bitlen(&ctx->D);
	dp_bits = mbedtls_mpi_bitlen(&ctx->DP);
	dq_bits = mbedtls_mpi_bitlen(&ctx->DQ);

	rv = crypto_pka_rsa_key_generate((crypto_lib_handle *) handle, nbits,
					&e_bits, (u8_t *) &exponent,
					&p_bits, (u8_t *) ctx->P.p,
					&q_bits, (u8_t *) ctx->Q.p,
					&n_bits, (u8_t *) ctx->N.p,
					&d_bits, (u8_t *) ctx->D.p,
					&dp_bits, (u8_t *) ctx->DP.p,
					&dq_bits, (u8_t *) ctx->DQ.p,
					&dq_bits, (u8_t *) ctx->DQ.p);
					/* Check the last param */
	return 0;
}

/*
 * Check a public RSA key
 */
int mbedtls_rsa_check_pubkey(const mbedtls_rsa_context *ctx)
{
	if (!ctx->N.p || !ctx->E.p)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;

	if ((ctx->N.p[0] & 1) == 0 || (ctx->E.p[0] & 1) == 0)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;

	if (mbedtls_mpi_bitlen(&ctx->N) < 128 ||
	    mbedtls_mpi_bitlen(&ctx->N) > MBEDTLS_MPI_MAX_BITS)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;

	if (mbedtls_mpi_bitlen(&ctx->E) < 2 ||
	    mbedtls_mpi_cmp_mpi(&ctx->E, &ctx->N) >= 0)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;

	return 0;
}

/*
 * Check a private RSA key
 */
int mbedtls_rsa_check_privkey(const mbedtls_rsa_context *ctx)
{
	int ret;
	mbedtls_mpi PQ, DE, P1, Q1, H, I, G, G2, L1, L2, DP, DQ, QP;

	ret = mbedtls_rsa_check_pubkey(ctx);
	if (ret != 0)
		return ret;

	if (!ctx->P.p || !ctx->Q.p || !ctx->D.p)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;

	mbedtls_mpi_init(&PQ);
	mbedtls_mpi_init(&DE);
	mbedtls_mpi_init(&P1);
	mbedtls_mpi_init(&Q1);
	mbedtls_mpi_init(&H);
	mbedtls_mpi_init(&I);
	mbedtls_mpi_init(&G);
	mbedtls_mpi_init(&G2);
	mbedtls_mpi_init(&L1);
	mbedtls_mpi_init(&L2);
	mbedtls_mpi_init(&DP);
	mbedtls_mpi_init(&DQ);
	mbedtls_mpi_init(&QP);

	MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&PQ, &ctx->P, &ctx->Q));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&DE, &ctx->D, &ctx->E));
	MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&P1, &ctx->P, 1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&Q1, &ctx->Q, 1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&H, &P1, &Q1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_gcd(&G, &ctx->E, &H));

	MBEDTLS_MPI_CHK(mbedtls_mpi_gcd(&G2, &P1, &Q1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_div_mpi(&L1, &L2, &H, &G2));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&I, &DE, &L1));

	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&DP, &ctx->D, &P1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&DQ, &ctx->D, &Q1));
	MBEDTLS_MPI_CHK(mbedtls_mpi_inv_mod(&QP, &ctx->Q, &ctx->P));
	/*
	 * Check for a valid PKCS1v2 private key
	 */
	if (mbedtls_mpi_cmp_mpi(&PQ, &ctx->N) != 0 ||
	    mbedtls_mpi_cmp_mpi(&DP, &ctx->DP) != 0 ||
	    mbedtls_mpi_cmp_mpi(&DQ, &ctx->DQ) != 0 ||
	    mbedtls_mpi_cmp_mpi(&QP, &ctx->QP) != 0 ||
	    mbedtls_mpi_cmp_int(&L2, 0) != 0 ||
	    mbedtls_mpi_cmp_int(&I, 1) != 0 ||
	    mbedtls_mpi_cmp_int(&G, 1) != 0) {
		ret = MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;
	}

cleanup:
	mbedtls_mpi_free(&PQ);
	mbedtls_mpi_free(&DE);
	mbedtls_mpi_free(&P1);
	mbedtls_mpi_free(&Q1);
	mbedtls_mpi_free(&H);
	mbedtls_mpi_free(&I);
	mbedtls_mpi_free(&G);
	mbedtls_mpi_free(&G2);
	mbedtls_mpi_free(&L1);
	mbedtls_mpi_free(&L2);
	mbedtls_mpi_free(&DP);
	mbedtls_mpi_free(&DQ);
	mbedtls_mpi_free(&QP);

	if (ret == MBEDTLS_ERR_RSA_KEY_CHECK_FAILED)
		return ret;

	if (ret != 0)
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED + ret;

	return 0;
}

/*
 * Check if contexts holding a public and private key match
 */
int mbedtls_rsa_check_pub_priv(const mbedtls_rsa_context *pub,
			       const mbedtls_rsa_context *prv)
{
	if (mbedtls_rsa_check_pubkey(pub) != 0 ||
	    mbedtls_rsa_check_privkey(prv) != 0) {
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;
	}

	if (mbedtls_mpi_cmp_mpi(&pub->N, &prv->N) != 0 ||
	    mbedtls_mpi_cmp_mpi(&pub->E, &prv->E) != 0) {
		return MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;
	}

	return 0;
}

/*
 * Do an RSA public key operation
 */
int mbedtls_rsa_public(mbedtls_rsa_context *ctx, const unsigned char *input,
		       unsigned char *output)
{
	BCM_SCAPI_STATUS rv;
	unsigned int e_bits, n_bits, m_bits, out_bits;

	crypto_init((crypto_lib_handle *) &handle);
	n_bits = mbedtls_mpi_bitlen(&ctx->N);
	e_bits = mbedtls_mpi_bitlen(&ctx->E);
	int ret;
	size_t olen;
	mbedtls_mpi T;

	mbedtls_mpi_init(&T);

	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&T, input, ctx->len));
	m_bits = mbedtls_mpi_bitlen(&T);

	if (mbedtls_mpi_cmp_mpi(&T, &ctx->N) >= 0) {
		ret = MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
		goto cleanup;
	}

	olen = ctx->len;
	rv = crypto_pka_rsa_mod_exp((crypto_lib_handle *) handle,
				    (u8_t *) T.p, m_bits,
				    (u8_t *) ctx->N.p, n_bits,
				    (u8_t *) ctx->E.p, e_bits,
				    (u8_t *) T.p, &out_bits, NULL, 0);

	MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&T, output, olen));

cleanup:

	mbedtls_mpi_free(&T);

	if (ret != 0)
		return MBEDTLS_ERR_RSA_PUBLIC_FAILED + ret;

	return 0;
}

/*
 * Exponent blinding supposed to prevent side-channel attacks using multiple
 * traces of measurements to recover the RSA key. The more collisions are there,
 * the more bits of the key can be recovered. See [3].
 *
 * Collecting n collisions with m bit long blinding value requires 2^(m-m/n)
 * observations on avarage.
 *
 * For example with 28 byte blinding to achieve 2 collisions the adversary has
 * to make 2^112 observations on avarage.
 *
 * (With the currently (as of 2017 April) known best algorithms breaking 2048
 * bit RSA requires approximately as much time as trying out 2^112 random keys.
 * Thus in this sense with 28 byte blinding the security is not reduced by
 * side-channel attacks like the one in [3])
 *
 * This countermeasure does not help if the key recovery is possible with a
 * single trace.
 */
#define RSA_EXPONENT_BLINDING 28

/*
 * Do an RSA private key operation
 */
int mbedtls_rsa_private(mbedtls_rsa_context *ctx,
			int (*f_rng)(void *, unsigned char *, size_t),
			void *p_rng, const unsigned char *input,
			unsigned char *output)
{
	BCM_SCAPI_STATUS rv;
	unsigned int d_bits, n_bits, m_bits, out_bits;

	crypto_init((crypto_lib_handle *) &handle);
	n_bits = mbedtls_mpi_bitlen(&ctx->N);
	d_bits = mbedtls_mpi_bitlen(&ctx->D);
	int ret;
	size_t olen;
	mbedtls_mpi T;

	/* Make sure we have private key info, prevent possible misuse */
	if (ctx->P.p == NULL || ctx->Q.p == NULL || ctx->D.p == NULL)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	mbedtls_mpi_init(&T);

	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&T, input, ctx->len));
	m_bits = mbedtls_mpi_bitlen(&T);

	if (mbedtls_mpi_cmp_mpi(&T, &ctx->N) >= 0) {
		ret = MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
		goto cleanup;
	}

	olen = ctx->len;
	rv = crypto_pka_rsa_mod_exp((crypto_lib_handle *) handle,
				    (u8_t *) T.p, m_bits,
				    (u8_t *) ctx->N.p, n_bits,
				    (u8_t *) ctx->D.p, d_bits,
				    (u8_t *) T.p, &out_bits, NULL, 0);

	MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&T, output, olen));

cleanup:

	mbedtls_mpi_free(&T);

	if (ret != 0)
		return MBEDTLS_ERR_RSA_PRIVATE_FAILED + ret;

	return 0;
}

#if defined(MBEDTLS_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-ENCRYPT function
 */
int mbedtls_rsa_rsaes_pkcs1_v15_encrypt(mbedtls_rsa_context *ctx,
					int (*f_rng)(void *, unsigned char *,
						      size_t), void *p_rng,
					int mode, size_t ilen,
					const unsigned char *input,
					unsigned char *output)
{
	size_t nb_pad, olen;
	int ret;
	unsigned char *p = output;

	if (mode == MBEDTLS_RSA_PRIVATE && ctx->padding != MBEDTLS_RSA_PKCS_V15)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	/* We don't check p_rng because it won't be dereferenced here */
	if (f_rng == NULL || input == NULL || output == NULL)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	olen = ctx->len;

	/* first comparison checks for overflow */
	if (ilen + 11 < ilen || olen < ilen + 11)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	nb_pad = olen - 3 - ilen;

	*p++ = 0;
	if (mode == MBEDTLS_RSA_PUBLIC) {
		*p++ = MBEDTLS_RSA_CRYPT;

		while (nb_pad-- > 0) {
			int rng_dl = 100;

			do {
				ret = f_rng(p_rng, p, 1);
			} while (*p == 0 && --rng_dl && ret == 0);

			/* Check if RNG failed to generate data */
			if (rng_dl == 0 || ret != 0)
				return (MBEDTLS_ERR_RSA_RNG_FAILED + ret);

			p++;
		}
	} else {
		*p++ = MBEDTLS_RSA_SIGN;

		while (nb_pad-- > 0)
			*p++ = 0xFF;
	}

	*p++ = 0;
	memcpy(p, input, ilen);

	return ((mode == MBEDTLS_RSA_PUBLIC)
		? mbedtls_rsa_public(ctx, output, output)
		: mbedtls_rsa_private(ctx, f_rng, p_rng, output, output));
}
#endif /* MBEDTLS_PKCS1_V15 */

/*
 * Add the message padding, then do an RSA operation
 */
int mbedtls_rsa_pkcs1_encrypt(mbedtls_rsa_context *ctx,
			      int (*f_rng)(void *, unsigned char *, size_t),
			      void *p_rng,
			      int mode, size_t ilen,
			      const unsigned char *input, unsigned char *output)
{
	switch (ctx->padding) {
#if defined(MBEDTLS_PKCS1_V15)
	case MBEDTLS_RSA_PKCS_V15:
		return mbedtls_rsa_rsaes_pkcs1_v15_encrypt(ctx, f_rng, p_rng,
							   mode, ilen, input,
							   output);
#endif

	default:
		break;
	}

	return MBEDTLS_ERR_RSA_INVALID_PADDING;
}

#if defined(MBEDTLS_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-DECRYPT function
 */
int mbedtls_rsa_rsaes_pkcs1_v15_decrypt(mbedtls_rsa_context *ctx,
					int (*f_rng)(void *, unsigned char *,
						      size_t), void *p_rng,
					int mode, size_t *olen,
					const unsigned char *input,
					unsigned char *output,
					size_t output_max_len)
{
	int ret;
	size_t ilen, pad_count = 0, i;
	unsigned char *p, bad, pad_done = 0;
	unsigned char buf[MBEDTLS_MPI_MAX_SIZE];

	if (mode == MBEDTLS_RSA_PRIVATE && ctx->padding != MBEDTLS_RSA_PKCS_V15)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	ilen = ctx->len;

	if (ilen < 16 || ilen > sizeof(buf))
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	ret = (mode == MBEDTLS_RSA_PUBLIC)
	    ? mbedtls_rsa_public(ctx, input, buf)
	    : mbedtls_rsa_private(ctx, f_rng, p_rng, input, buf);

	if (ret != 0)
		goto cleanup;

	p = buf;
	bad = 0;

	/*
	 * Check and get padding len in "constant-time"
	 */
	bad |= *p++;		/* First byte must be 0 */

	/* This test does not depend on secret data */
	if (mode == MBEDTLS_RSA_PRIVATE) {
		bad |= *p++ ^ MBEDTLS_RSA_CRYPT;

		/*
		 * Get padding len, but always read till end of buffer
		 * (minus one, for the 00 byte)
		 */
		for (i = 0; i < ilen - 3; i++) {
			pad_done |= ((p[i] | (unsigned char)-p[i]) >> 7) ^ 1;
			pad_count +=
			    ((pad_done | (unsigned char)-pad_done) >> 7) ^ 1;
		}

		p += pad_count;
		bad |= *p++;	/* Must be zero */
	} else {
		bad |= *p++ ^ MBEDTLS_RSA_SIGN;

		/* Get padding len, but always read till end of buffer
		 * (minus one, for the 00 byte)
		 */
		for (i = 0; i < ilen - 3; i++) {
			pad_done |= (p[i] != 0xFF);
			pad_count += (pad_done == 0);
		}

		p += pad_count;
		bad |= *p++;	/* Must be zero */
	}

	bad |= (pad_count < 8);

	if (bad) {
		ret = MBEDTLS_ERR_RSA_INVALID_PADDING;
		goto cleanup;
	}

	if (ilen - (p - buf) > output_max_len) {
		ret = MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE;
		goto cleanup;
	}

	*olen = ilen - (p - buf);
	memcpy(output, p, *olen);
	ret = 0;

cleanup:
	mbedtls_zeroize(buf, sizeof(buf));

	return ret;
}
#endif /* MBEDTLS_PKCS1_V15 */

/*
 * Do an RSA operation, then remove the message padding
 */
int mbedtls_rsa_pkcs1_decrypt(mbedtls_rsa_context *ctx,
			      int (*f_rng)(void *, unsigned char *, size_t),
			      void *p_rng,
			      int mode, size_t *olen,
			      const unsigned char *input,
			      unsigned char *output, size_t output_max_len)
{
	switch (ctx->padding) {
#if defined(MBEDTLS_PKCS1_V15)
	case MBEDTLS_RSA_PKCS_V15:
		return mbedtls_rsa_rsaes_pkcs1_v15_decrypt(ctx, f_rng, p_rng,
							   mode, olen, input,
							   output,
							   output_max_len);
#endif
	default:
		break;
	}

	return MBEDTLS_ERR_RSA_INVALID_PADDING;
}

#if defined(MBEDTLS_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-V1_5-SIGN function
 */
/*
 * Do an RSA operation to sign the message digest
 */
int mbedtls_rsa_rsassa_pkcs1_v15_sign(mbedtls_rsa_context *ctx,
				      int (*f_rng)(void *, unsigned char *,
						    size_t), void *p_rng,
				      int mode, mbedtls_md_type_t md_alg,
				      unsigned int hashlen,
				      const unsigned char *hash,
				      unsigned char *sig)
{
	size_t nb_pad, olen, oid_size = 0;
	unsigned char *p = sig;
	const char *oid = NULL;
	unsigned char *sig_try = NULL, *verif = NULL;
	size_t i;
	unsigned char diff;
	volatile unsigned char diff_no_optimize;
	int ret;

	if (mode == MBEDTLS_RSA_PRIVATE && ctx->padding != MBEDTLS_RSA_PKCS_V15)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	olen = ctx->len;
	nb_pad = olen - 3;

	if (md_alg != MBEDTLS_MD_NONE) {
		const mbedtls_md_info_t *md_info =
		    mbedtls_md_info_from_type(md_alg);
		if (md_info == NULL)
			return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

		if (mbedtls_oid_get_oid_by_md(md_alg, &oid, &oid_size) != 0)
			return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

		nb_pad -= 10 + oid_size;

		hashlen = mbedtls_md_get_size(md_info);
	}

	nb_pad -= hashlen;

	if ((nb_pad < 8) || (nb_pad > olen))
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	*p++ = 0;
	*p++ = MBEDTLS_RSA_SIGN;
	memset(p, 0xFF, nb_pad);
	p += nb_pad;
	*p++ = 0;

	if (md_alg == MBEDTLS_MD_NONE) {
		memcpy(p, hash, hashlen);
	} else {
		/*
		 * DigestInfo ::= SEQUENCE {
		 *   digestAlgorithm DigestAlgorithmIdentifier,
		 *   digest Digest }
		 *
		 * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
		 *
		 * Digest ::= OCTET STRING
		 */
		*p++ = MBEDTLS_ASN1_SEQUENCE | MBEDTLS_ASN1_CONSTRUCTED;
		*p++ = (unsigned char)(0x08 + oid_size + hashlen);
		*p++ = MBEDTLS_ASN1_SEQUENCE | MBEDTLS_ASN1_CONSTRUCTED;
		*p++ = (unsigned char)(0x04 + oid_size);
		*p++ = MBEDTLS_ASN1_OID;
		*p++ = oid_size & 0xFF;
		memcpy(p, oid, oid_size);
		p += oid_size;
		*p++ = MBEDTLS_ASN1_NULL;
		*p++ = 0x00;
		*p++ = MBEDTLS_ASN1_OCTET_STRING;
		*p++ = hashlen;
		memcpy(p, hash, hashlen);
	}

	if (mode == MBEDTLS_RSA_PUBLIC)
		return mbedtls_rsa_public(ctx, sig, sig);

	/*
	 * In order to prevent Lenstra's attack, make the signature in a
	 * temporary buffer and check it before returning it.
	 */
	sig_try = mbedtls_calloc(1, ctx->len);
	if (sig_try == NULL)
		return MBEDTLS_ERR_MPI_ALLOC_FAILED;

	verif = mbedtls_calloc(1, ctx->len);
	if (verif == NULL) {
		mbedtls_free(sig_try);
		return MBEDTLS_ERR_MPI_ALLOC_FAILED;
	}

	MBEDTLS_MPI_CHK(mbedtls_rsa_private(ctx, f_rng, p_rng, sig, sig_try));
	MBEDTLS_MPI_CHK(mbedtls_rsa_public(ctx, sig_try, verif));

	/* Compare in constant time just in case */
	for (diff = 0, i = 0; i < ctx->len; i++)
		diff |= verif[i] ^ sig[i];
	diff_no_optimize = diff;

	if (diff_no_optimize != 0) {
		ret = MBEDTLS_ERR_RSA_PRIVATE_FAILED;
		goto cleanup;
	}

	memcpy(sig, sig_try, ctx->len);

cleanup:
	mbedtls_free(sig_try);
	mbedtls_free(verif);

	return ret;
}
#endif /* MBEDTLS_PKCS1_V15 */

/*
 * Do an RSA operation to sign the message digest
 */
int mbedtls_rsa_pkcs1_sign(mbedtls_rsa_context *ctx,
			   int (*f_rng)(void *, unsigned char *, size_t),
			   void *p_rng,
			   int mode,
			   mbedtls_md_type_t md_alg,
			   unsigned int hashlen,
			   const unsigned char *hash, unsigned char *sig)
{
	switch (ctx->padding) {
#if defined(MBEDTLS_PKCS1_V15)
	case MBEDTLS_RSA_PKCS_V15:
		return mbedtls_rsa_rsassa_pkcs1_v15_sign(ctx, f_rng, p_rng,
							 mode, md_alg, hashlen,
							 hash, sig);
#endif
	default:
		break;
	}

	return MBEDTLS_ERR_RSA_INVALID_PADDING;
}

#if defined(MBEDTLS_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-v1_5-VERIFY function
 */
int mbedtls_rsa_rsassa_pkcs1_v15_verify(mbedtls_rsa_context *ctx,
					int (*f_rng)(void *, unsigned char *,
						      size_t), void *p_rng,
					int mode, mbedtls_md_type_t md_alg,
					unsigned int hashlen,
					const unsigned char *hash,
					const unsigned char *sig)
{
	int ret;
	size_t len, siglen, asn1_len;
	unsigned char *p, *p0, *end;
	mbedtls_md_type_t msg_md_alg;
	const mbedtls_md_info_t *md_info;
	mbedtls_asn1_buf oid;
	unsigned char buf[MBEDTLS_MPI_MAX_SIZE];

	if (mode == MBEDTLS_RSA_PRIVATE && ctx->padding != MBEDTLS_RSA_PKCS_V15)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	siglen = ctx->len;

	if (siglen < 16 || siglen > sizeof(buf))
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;

	ret = (mode == MBEDTLS_RSA_PUBLIC)
	    ? mbedtls_rsa_public(ctx, sig, buf)
	    : mbedtls_rsa_private(ctx, f_rng, p_rng, sig, buf);

	if (ret != 0)
		return ret;

	p = buf;

	if (*p++ != 0 || *p++ != MBEDTLS_RSA_SIGN)
		return MBEDTLS_ERR_RSA_INVALID_PADDING;

	while (*p != 0) {
		if (p >= buf + siglen - 1 || *p != 0xFF)
			return MBEDTLS_ERR_RSA_INVALID_PADDING;
		p++;
	}
	p++;			/* skip 00 byte */

	/* We've read: 00 01 PS 00 where PS must be at least 8 bytes */
	if (p - buf < 11)
		return MBEDTLS_ERR_RSA_INVALID_PADDING;

	len = siglen - (p - buf);

	if (len == hashlen && md_alg == MBEDTLS_MD_NONE) {
		if (memcmp(p, hash, hashlen) == 0)
			return 0;
		else
			return MBEDTLS_ERR_RSA_VERIFY_FAILED;
	}

	md_info = mbedtls_md_info_from_type(md_alg);
	if (md_info == NULL)
		return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
	hashlen = mbedtls_md_get_size(md_info);

	end = p + len;

	/*
	 * Parse the ASN.1 structure inside the PKCS#1 v1.5 structure.
	 * Insist on 2-byte length tags, to protect against variants of
	 * Bleichenbacher's forgery attack against lax PKCS#1v1.5 verification.
	 */
	p0 = p;
	ret = mbedtls_asn1_get_tag(&p, end, &asn1_len,
					MBEDTLS_ASN1_CONSTRUCTED |
					MBEDTLS_ASN1_SEQUENCE);
	if (ret != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	if (p != p0 + 2 || asn1_len + 2 != len)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	p0 = p;
	ret = mbedtls_asn1_get_tag(&p, end, &asn1_len,
					MBEDTLS_ASN1_CONSTRUCTED |
					MBEDTLS_ASN1_SEQUENCE);

	if (ret != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	if (p != p0 + 2 || asn1_len + 6 + hashlen != len)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	p0 = p;
	mbedtls_asn1_get_tag(&p, end, &oid.len, MBEDTLS_ASN1_OID);

	if (ret != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;
	if (p != p0 + 2)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	oid.p = p;
	p += oid.len;

	if (mbedtls_oid_get_md_alg(&oid, &msg_md_alg) != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	if (md_alg != msg_md_alg)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	/*
	 * assume the algorithm parameters must be NULL
	 */
	p0 = p;
	ret = mbedtls_asn1_get_tag(&p, end, &asn1_len, MBEDTLS_ASN1_NULL);
	if (ret != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;
	if (p != p0 + 2)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	p0 = p;
	ret = mbedtls_asn1_get_tag(&p, end, &asn1_len,
				   MBEDTLS_ASN1_OCTET_STRING);
	if (ret != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;
	if (p != p0 + 2 || asn1_len != hashlen)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	if (memcmp(p, hash, hashlen) != 0)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	p += hashlen;

	if (p != end)
		return MBEDTLS_ERR_RSA_VERIFY_FAILED;

	return 0;

}
#endif /* MBEDTLS_PKCS1_V15 */

/*
 * Do an RSA operation and check the message digest
 */
int mbedtls_rsa_pkcs1_verify(mbedtls_rsa_context *ctx,
			     int (*f_rng)(void *, unsigned char *, size_t),
			     void *p_rng,
			     int mode,
			     mbedtls_md_type_t md_alg,
			     unsigned int hashlen,
			     const unsigned char *hash,
			     const unsigned char *sig)
{
	switch (ctx->padding) {
#if defined(MBEDTLS_PKCS1_V15)
	case MBEDTLS_RSA_PKCS_V15:
		return mbedtls_rsa_rsassa_pkcs1_v15_verify(ctx, f_rng, p_rng,
							   mode, md_alg,
							   hashlen, hash, sig);
#endif

	default:
		break;
	}

	return MBEDTLS_ERR_RSA_INVALID_PADDING;
}

/*
 * Copy the components of an RSA key
 */
int mbedtls_rsa_copy(mbedtls_rsa_context *dst, const mbedtls_rsa_context *src)
{
	int ret;

	dst->ver = src->ver;
	dst->len = src->len;

	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->N, &src->N));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->E, &src->E));

	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->D, &src->D));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->P, &src->P));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Q, &src->Q));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->DP, &src->DP));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->DQ, &src->DQ));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->QP, &src->QP));

	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RN, &src->RN));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RP, &src->RP));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->RQ, &src->RQ));

	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Vi, &src->Vi));
	MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&dst->Vf, &src->Vf));

	dst->padding = src->padding;
	dst->hash_id = src->hash_id;

cleanup:
	if (ret != 0)
		mbedtls_rsa_free(dst);

	return ret;
}

/*
 *Free the components of an RSA key
 */
void mbedtls_rsa_free(mbedtls_rsa_context *ctx)
{
	mbedtls_mpi_free(&ctx->Vi);
	mbedtls_mpi_free(&ctx->Vf);
	mbedtls_mpi_free(&ctx->RQ);
	mbedtls_mpi_free(&ctx->RP);
	mbedtls_mpi_free(&ctx->RN);
	mbedtls_mpi_free(&ctx->QP);
	mbedtls_mpi_free(&ctx->DQ);
	mbedtls_mpi_free(&ctx->DP);
	mbedtls_mpi_free(&ctx->Q);
	mbedtls_mpi_free(&ctx->P);
	mbedtls_mpi_free(&ctx->D);
	mbedtls_mpi_free(&ctx->E);
	mbedtls_mpi_free(&ctx->N);

#if defined(MBEDTLS_THREADING_C)
	mbedtls_mutex_free(&ctx->mutex);
#endif
}
#endif
