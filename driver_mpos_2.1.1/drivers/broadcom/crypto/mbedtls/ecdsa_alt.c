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
 * @file ecdsa_alt.c
 *
 * @brief  mbedtls alternate ecdsa file.
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECDSA_C)

#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/asn1write.h"

#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_rng.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <string.h>

#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
#include "mbedtls/hmac_drbg.h"
#endif

#if defined(MBEDTLS_ECDSA_SIGN_ALT) || defined(MBEDTLS_ECDSA_VERIFY_ALT)
static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static fips_rng_context rngctx;
#endif

#if defined(MBEDTLS_ECDSA_SIGN_ALT)
/*
 * Compute ECDSA signature of a hashed message
 */
int mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
		       const mbedtls_mpi *d, const unsigned char *buf,
		       size_t blen,
		       int (*f_rng)(void *, unsigned char *, size_t),
		       void *p_rng)
{
	BCM_SCAPI_STATUS rv;
	u32_t p_bitlen;
	u8_t k[32];
	s32_t ret;

	crypto_init((crypto_lib_handle *)&handle);
	crypto_rng_init((crypto_lib_handle *)&handle, &rngctx, 1,
			 NULL, 0, NULL, 0, NULL, 0);
	ret = mbedtls_mpi_grow(r, 8);
	if (ret)
		return ret;
	ret = mbedtls_mpi_grow(s, 8);
	if (ret)
		return ret;
	/* Fail on curves such as Curve25519 that can't be used for ECDSA */
	if (grp->N.p == NULL)
		return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

	p_bitlen = mbedtls_mpi_bitlen(&grp->P);
	rv = crypto_pka_ecp_ecdsa_sign((crypto_lib_handle *)handle, (u8_t *)buf,
			blen, 0, (u8_t *)grp->P.p, p_bitlen, (u8_t *)grp->A.p,
			(u8_t *)grp->B.p, (u8_t *)grp->N.p, (u8_t *)grp->G.X.p,
			(u8_t *)grp->G.Y.p, k, 0, (u8_t *)d->p,
			(u8_t *)r->p, (u8_t *)s->p,
			NULL, NULL);
	if (rv)
		return -1;

	/* Fail on curves such as Curve25519 that can't be used for ECDSA */
	if (grp->N.p == NULL)
		return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

	return 0;
}
#endif /* MBEDTLS_ECDSA_SIGN_ALT */

#if defined(MBEDTLS_ECDSA_VERIFY_ALT)
/*
 * Verify ECDSA signature of hashed message
 */
int mbedtls_ecdsa_verify(mbedtls_ecp_group *grp,
			 const unsigned char *buf, size_t blen,
			 const mbedtls_ecp_point *Q, const mbedtls_mpi *r,
			 const mbedtls_mpi *s)
{
	BCM_SCAPI_STATUS rv;
	u32_t p_bitlen;

	/* Fail on curves such as Curve25519 that can't be used for ECDSA */
	if (grp->N.p == NULL)
		return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;

	crypto_init((crypto_lib_handle *)&handle);
	/* Step 1: make sure r and s are in range 1..n-1 */
	if (mbedtls_mpi_cmp_int(r, 1) < 0 ||
	    mbedtls_mpi_cmp_mpi(r, &grp->N) >= 0 ||
	    mbedtls_mpi_cmp_int(s, 1) < 0 ||
	    mbedtls_mpi_cmp_mpi(s, &grp->N) >= 0)
		return MBEDTLS_ERR_ECP_VERIFY_FAILED;

	p_bitlen = mbedtls_mpi_bitlen(&grp->P);
	rv = crypto_pka_ecp_ecdsa_verify((crypto_lib_handle *)handle,
			(u8_t *)buf, blen, 0, (u8_t *)grp->P.p, p_bitlen,
			(u8_t *)grp->A.p, (u8_t *)grp->B.p,
			(u8_t *)grp->N.p, (u8_t *)grp->G.X.p,
			(u8_t *)grp->G.Y.p, (u8_t *)Q->X.p, (u8_t *)Q->Y.p,
			(u8_t *)Q->Z.p, (u8_t *)r->p, (u8_t *)s->p, NULL, NULL);

	if (rv)
		return -1;

	return 0;
}
#endif /* MBEDTLS_ECDSA_VERIFY_ALT */

#if defined(MBEDTLS_ECDSA_GENKEY_ALT)
/*
 * Generate key pair
 */
int mbedtls_ecdsa_genkey(mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
			 int (*f_rng)(void *, unsigned char *, size_t),
			 void *p_rng)
{
	BCM_SCAPI_STATUS rv;

	rv = mbedtls_ecp_group_load(&ctx->grp, gid);
	if (rv)
		return -1;

	rv = mbedtls_ecdh_gen_public(&ctx->grp, &ctx->d, &ctx->Q, NULL, NULL);
	if (rv)
		return -1;

	return 0;
}
#endif /* MBEDTLS_ECDSA_GENKEY_ALT */

#endif /* MBEDTLS_ECDSA_C */
