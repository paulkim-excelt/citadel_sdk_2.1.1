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
#include <board.h>
#include <device.h>
#include <pka/crypto_pka_selftest.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_rng.h>
#include <test.h>

static u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static fips_rng_context rngctx;

static void crypto_pka_selftest_modexp_s(void)
{
	BCM_SCAPI_STATUS ret;

	crypto_init((crypto_lib_handle *)&cryptoHandle);
	ret = crypto_rng_init((crypto_lib_handle *)&cryptoHandle, &rngctx, 1,
		    NULL, 0, NULL, 0, NULL, 0);
	zassert_equal(ret, 0, "crypto rng initialization failed");

	ret = crypto_pka_selftest_modexp((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_modinv_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_modinv((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_rsa_keygen_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_rsa_keygen((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_rsa_enc_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_rsa_enc((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

/*
static void crypto_pka_selftest_rsa_dec_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_rsa_dec((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}
*/

static void crypto_pka_selftest_pkcs1_rsassa_v15_sig_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_pkcs1_rsassa_v15_sig((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_pkcs1_rsassa_v15_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_pkcs1_rsassa_v15_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_dsa_sign_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_dsa_sign((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_dsa_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_dsa_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_dsa2048_sign_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_dsa2048_sign_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_dh_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_dh((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_ecdh_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_ecdh((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_fips_selftest_ecdh_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_fips_selftest_ecdh((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_selftest_ecdsa_sign_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_selftest_ecdsa_sign_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_cust_selftest_ecdsa_p256_sign_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_cust_selftest_ecdsa_p256_sign_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_cust_selftest_ecp_diffie_hellman_shared_secret_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_cust_selftest_ecp_diffie_hellman_shared_secret((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_cust_selftest_ecdsa_p256_key_generate_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_cust_selftest_ecdsa_p256_key_generate((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

static void crypto_pka_cust_selftest_ec_point_verify_s(void)
{
	BCM_SCAPI_STATUS ret;

	ret = crypto_pka_cust_selftest_ec_point_verify((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_pka_selftest_modexp failed");
}

SHELL_TEST_DECLARE(test_pka)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(pka_tests,  ztest_unit_test(crypto_pka_selftest_modexp_s),
		ztest_unit_test(crypto_pka_selftest_modinv_s),
		ztest_unit_test(crypto_pka_selftest_rsa_keygen_s),
		ztest_unit_test(crypto_pka_selftest_rsa_enc_s),
		/*  FIXME the RSA decryption test has to be executed once the
		 * test vectors are available for CRT mode */
		/* ztest_unit_test(crypto_pka_selftest_rsa_dec_s), */
		ztest_unit_test(crypto_pka_selftest_pkcs1_rsassa_v15_sig_s),
		ztest_unit_test(crypto_pka_selftest_pkcs1_rsassa_v15_verify_s),
		ztest_unit_test(crypto_pka_selftest_dsa_sign_s),
		ztest_unit_test(crypto_pka_selftest_dsa_verify_s),
		ztest_unit_test(crypto_pka_selftest_dsa2048_sign_verify_s),
		ztest_unit_test(crypto_pka_selftest_dh_s),
		ztest_unit_test(crypto_pka_selftest_ecdh_s),
		ztest_unit_test(crypto_pka_fips_selftest_ecdh_s),
		ztest_unit_test(crypto_pka_selftest_ecdsa_sign_verify_s),
		ztest_unit_test(crypto_pka_cust_selftest_ecdsa_p256_sign_verify_s),
		ztest_unit_test(crypto_pka_cust_selftest_ecp_diffie_hellman_shared_secret_s),
		ztest_unit_test(crypto_pka_cust_selftest_ecdsa_p256_key_generate_s),
		ztest_unit_test(crypto_pka_cust_selftest_ec_point_verify_s)
		);

	ztest_run_test_suite(pka_tests);

	return 0;
}
