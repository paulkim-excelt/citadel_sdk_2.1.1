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
#include <board.h>
#include <device.h>
#include <test.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/des.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha1.h>
#include <mbedtls/rsa.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>

/*
 * @file test_mbedtls.c
 * @brief  mbedtls api test
 */

extern int mbedtls_platform_set_printf(void(*printf_func)(const char *, ...));
extern int mbedtls_platform_set_calloc_free(
					void * (*calloc_func)(size_t, size_t),
					void (*free_func)(void *));

int mbedtls_ecdh_self_test(int verbose)
{
	int ret = -1;
	mbedtls_ecdh_context ctx_cli, ctx_srv;

	mbedtls_ecdh_init(&ctx_cli);
	mbedtls_ecdh_init(&ctx_srv);
	mbedtls_mpi_init(&ctx_cli.Qp.X);
	mbedtls_mpi_init(&ctx_cli.Qp.Y);
	mbedtls_mpi_init(&ctx_srv.Qp.X);
	mbedtls_mpi_init(&ctx_srv.Qp.Y);
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&ctx_cli.Qp.X, 8));
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&ctx_cli.Qp.Y, 8));
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&ctx_srv.Qp.X, 8));
	MBEDTLS_MPI_CHK(mbedtls_mpi_grow(&ctx_srv.Qp.Y, 8));

	/* Client: inialize context and generate keypair */
	if (verbose != 0)
		TC_PRINT("  . Setting up client context...");

	ret = mbedtls_ecp_group_load(&ctx_cli.grp, MBEDTLS_ECP_DP_SECP256K1);
	if (ret) {
		if (verbose != 0)
			TC_PRINT(" fail mbedtls_ecp_group_load rv %d\n", ret);

		goto exit;
	}

	ret = mbedtls_ecdh_gen_public(&ctx_cli.grp, &ctx_cli.d, &ctx_cli.Q,
					NULL, NULL);
	if (ret) {
		if (verbose != 0)
			TC_PRINT(" fail mbedtls_ecdh_gen_public rv %d\n", ret);

		goto exit;
	}

	if (verbose != 0)
		TC_PRINT(" ok\n");

	/* Server: initialize context and generate keypair */
	if (verbose != 0)
		TC_PRINT("  . Setting up server context...");

	ret = mbedtls_ecp_group_load(&ctx_srv.grp, MBEDTLS_ECP_DP_SECP256K1);
	if (ret != 0) {
		if (verbose != 0)
			TC_PRINT(" fail mbedtls_ecp_group_load rv %d\n", ret);

		goto exit;
	}

	ret = mbedtls_ecdh_gen_public(&ctx_srv.grp, &ctx_srv.d, &ctx_srv.Q,
				      NULL, NULL);
	if (ret != 0) {
		if (verbose != 0)
			TC_PRINT(" fail mbedtls_ecdh_gen_public rv %d\n", ret);

		goto exit;
	}

	if (verbose != 0)
		TC_PRINT(" ok\n");

	/* Server: read peer's key and generate shared secret */
	if (verbose != 0)
		TC_PRINT(". Server reading client key and computing secret...");

	MBEDTLS_MPI_CHK(mbedtls_ecp_copy(&ctx_srv.Qp, &ctx_cli.Q));
	ret = mbedtls_ecdh_compute_shared(&ctx_srv.grp, &ctx_srv.z,
					&ctx_srv.Qp, &ctx_srv.d,
					NULL, NULL);
	if (ret != 0) {
		if (verbose != 0)
			TC_PRINT("fail ecdh_compute_shared rv %d\n", ret);
		goto exit;
	}

	if (verbose != 0)
		TC_PRINT(" ok\n");

	/* Client: read peer's key and generate shared secret */
	if (verbose != 0)
		TC_PRINT(". Client reading server key and computing secret..");
	MBEDTLS_MPI_CHK(mbedtls_ecp_copy(&ctx_cli.Qp, &ctx_srv.Q));
	ret = mbedtls_ecdh_compute_shared(&ctx_cli.grp, &ctx_cli.z,
					  &ctx_cli.Qp, &ctx_cli.d,
					  NULL, NULL);
	if (ret != 0) {
		if (verbose != 0)
			TC_PRINT("fail ecdh_compute_shared rv %d\n", ret);

		goto cleanup;
	}

	if (verbose != 0)
		TC_PRINT(" ok\n");

	/* Verification: are the computed secrets equal? */
	if (verbose != 0)
		TC_PRINT("  . Checking if both computed secrets are equal...");

	ret = mbedtls_mpi_cmp_mpi(&ctx_cli.z, &ctx_srv.z);
	if (ret != 0) {
		if (verbose != 0)
			TC_PRINT("fail ecdh_compute_shared rv %d\n", ret);

		goto cleanup;
	}

	if (verbose != 0)
		TC_PRINT(" ok\n");

cleanup:
exit:
	mbedtls_ecdh_free(&ctx_srv);
	mbedtls_ecdh_free(&ctx_cli);

	return ret;
}

int mbedtls_ecdsa_self_test(int verbose)
{
	int ret = 0;
	mbedtls_ecdsa_context ctx_sign, ctx_verify;
	unsigned char message[64];
	unsigned char hash[32];
	unsigned char sig[MBEDTLS_ECDSA_MAX_LEN];
	size_t sig_len;

	mbedtls_ecdsa_init(&ctx_sign);
	mbedtls_ecdsa_init(&ctx_verify);

	memset(sig, 0, sizeof(sig));
	memset(message, 0x25, sizeof(message));

	/* Generate a key pair for signing */

	TC_PRINT("Generating key pair...");


	ret = mbedtls_ecdsa_genkey(&ctx_sign, MBEDTLS_ECP_DP_SECP256K1,
					NULL, NULL);
	if (ret != 0) {
		TC_PRINT("failed mbedtls_ecdsa_genkey returned %d\n", ret);
		goto exit;
	}

	TC_PRINT(" ok (key size: %d bits)\n", (int) ctx_sign.grp.pbits);

	/* Compute message hash  */
	TC_PRINT("  . Computing message hash...");

#ifdef MBEDTLS_VERSION_2_16
	mbedtls_sha256_ret(message, sizeof(message), hash, 0);
#else
	mbedtls_sha256(message, sizeof(message), hash, 0);
#endif
	TC_PRINT(" ok\n");

	/* Sign message hash */
	TC_PRINT("  . Signing message hash...");

	ret = mbedtls_ecdsa_write_signature(&ctx_sign, MBEDTLS_MD_SHA256,
					    hash, sizeof(hash), sig, &sig_len,
					    NULL, NULL);
	if (ret != 0) {
		TC_PRINT("failed mbedtls_ecdsa_genkey returned %d\n", ret);
		goto exit;
	}
	TC_PRINT("ok (signature length = %u)\n", (unsigned int) sig_len);

	/*
	 * Transfer public information to verifying context
	 *
	 * We could use the same context for verification and signatures, but we
	 * chose to use a new one in order to make it clear that the verifying
	 * context only needs the public key (Q), and not the private key (d).
	 */
	TC_PRINT("  . Preparing verification context...");

	ret = mbedtls_ecp_group_copy(&ctx_verify.grp, &ctx_sign.grp);
	if (ret) {
		TC_PRINT("failed mbedtls_ecp_group_copy returned %d\n", ret);
		goto exit;
	}

	ret = mbedtls_ecp_copy(&ctx_verify.Q, &ctx_sign.Q);
	if (ret) {
		TC_PRINT(" failed\n  ! mbedtls_ecp_copy returned %d\n", ret);
		goto exit;
	}

	/* Verify signature */
	TC_PRINT(" ok\n  . Verifying signature...");

	ret = mbedtls_ecdsa_read_signature(&ctx_verify,
						hash, sizeof(hash),
						sig, sig_len);
	if (ret) {
		TC_PRINT("fail mbedtls_ecdsa_read_signature rv %d\n", ret);
		goto exit;
	}

	TC_PRINT(" ok\n");

exit:
	mbedtls_ecdsa_free(&ctx_verify);
	mbedtls_ecdsa_free(&ctx_sign);

	return ret;
}

static void mbedtls_selftest_s(void)
{
	s32_t verbose;
	s32_t ret;

	verbose = 1;

	mbedtls_platform_set_printf(printk);
	mbedtls_platform_set_calloc_free(k_calloc, k_free);

	ret = mbedtls_aes_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_aes_self_test failed");

	ret = mbedtls_gcm_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_gcm_self_test failed");

	ret = mbedtls_des_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_des_self_test failed");

	ret = mbedtls_sha256_block_aligned_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_sha256_self_test failed");

	ret = mbedtls_sha1_block_aligned_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_sha1_self_test failed");

	ret = mbedtls_rsa_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_rsa_self_test failed");

	ret = mbedtls_ecdh_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_ecdh_self_test failed");

	ret = mbedtls_ecdsa_self_test(verbose);
	zassert_equal(ret, 0, "mbedtls_ecdsa_self_test failed");

}

SHELL_TEST_DECLARE(test_mbedtls)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(mbedtls_tests, ztest_unit_test(mbedtls_selftest_s));
	ztest_run_test_suite(mbedtls_tests);

	return 0;
}
