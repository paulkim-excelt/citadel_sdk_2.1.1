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
 * @file ecdh_alt.c
 *
 * @brief  mbedtls alternate ecdh file.
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECDH_C)

#include "mbedtls/ecdh.h"

#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_rng.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <string.h>
#include <limits.h>

#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT) || \
	defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
static u8_t handle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static fips_rng_context rngctx;
#endif

#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
/* Generate public key: simple wrapper around mbedtls_ecp_gen_keypair */
int mbedtls_ecdh_gen_public(mbedtls_ecp_group *grp, mbedtls_mpi *d,
			    mbedtls_ecp_point *Q,
			    int (*f_rng)(void *, unsigned char *, size_t),
			    void *p_rng)
{
	BCM_SCAPI_STATUS ret;
	u32_t p_bitlen;
	u32_t d_valid;

	crypto_init((crypto_lib_handle *)&handle);
	crypto_rng_init((crypto_lib_handle *)&handle, &rngctx, 1,
			 NULL, 0, NULL, 0, NULL, 0);

	/* If the user doesn't provide the 'd', generate it */
	if (d->p == NULL) {
		mbedtls_mpi_init(d);
		MBEDTLS_MPI_CHK(mbedtls_mpi_grow(d, grp->P.n));
		d_valid = 0;
	} else {
		d_valid = 1;
	}

	mbedtls_mpi_init(&Q->X);
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&Q->X, grp->P.n));
	mbedtls_mpi_init(&Q->Y);
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&Q->Y, grp->P.n));

	p_bitlen = grp->P.n * sizeof(mbedtls_mpi_uint) * CHAR_BIT;
	ret = crypto_pka_ecp_ecdsa_key_generate((crypto_lib_handle *)handle,
				0, (u8_t *)grp->P.p, p_bitlen,
				(u8_t *)grp->A.p, (u8_t *)grp->B.p,
				(u8_t *)grp->N.p, (u8_t *)grp->G.X.p,
				(u8_t *)grp->G.Y.p,
				(u8_t *)d->p, d_valid, (u8_t *)Q->X.p,
				(u8_t *)Q->Y.p, NULL, NULL);
cleanup:
	if (ret)
		return -1;

	return 0;
}
#endif /* MBEDTLS_ECDH_GEN_PUBLIC_ALT */

#if defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
/*
 * Compute shared secret
 */
int mbedtls_ecdh_compute_shared(mbedtls_ecp_group *grp, mbedtls_mpi *z,
				const mbedtls_ecp_point *Q,
				const mbedtls_mpi *d,
				int (*f_rng)(void *, unsigned char *, size_t),
				void *p_rng)
{
	BCM_SCAPI_STATUS rv;
	u32_t p_bitlen;
	s32_t ret = 0;
	u32_t ky[8] = {0};

	crypto_init((crypto_lib_handle *)&handle);
	mbedtls_mpi_init(z);
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(z, grp->P.n));
	p_bitlen = grp->P.n * sizeof(mbedtls_mpi_uint) * CHAR_BIT;
	rv = crypto_pka_ecp_diffie_hellman_shared_secret(
			(crypto_lib_handle *)handle, 1, (u8_t *)grp->P.p,
			p_bitlen, (u8_t *)grp->A.p, (u8_t *)grp->B.p,
			(u8_t *)grp->N.p, (u8_t *) d->p,
			(u8_t *)Q->X.p, (u8_t *)Q->Y.p,
			(u8_t *)z->p, (u8_t *)ky, NULL, NULL);
cleanup:
	if (ret)
		return -1;

	return 0;
}
#endif /* MBEDTLS_ECDH_COMPUTE_SHARED_ALT */

#endif /* MBEDTLS_ECDH_C */
