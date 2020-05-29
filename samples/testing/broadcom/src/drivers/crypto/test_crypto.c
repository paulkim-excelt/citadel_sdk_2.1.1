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
#include <crypto/crypto_selftest.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <test.h>

static u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static fips_rng_context rngctx;


static void crypto_rng_selftest_s(void)
{
	BCM_SCAPI_STATUS ret;

	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_rng_selftest((crypto_lib_handle *)&cryptoHandle, &rngctx);
	zassert_equal(ret, 0, "crypto_rng_selftest failed");
}

static void crypto_selftest_sha1_s(void)
{
	BCM_SCAPI_STATUS ret;

	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha1((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha1 failed");
}

static void crypto_selftest_sha256_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha256((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_hmac_sha256_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_hmac_sha256((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_sha3_224_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha3_224((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_sha3_256_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha3_256((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_sha3_384_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha3_384((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_sha3_512_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_sha3_512((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_aes_cbc_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_aes_cbc((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_aes_ctr_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_aes_ctr((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_aes_128_ccm_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_aes_128_ccm((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_aes_cbc_sha256_hmac_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_aes_cbc_sha256_hmac((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_sha256 failed");
}

static void crypto_selftest_des_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_des((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_des failed");
}

static void crypto_selftest_3des_s(void)
{
	BCM_SCAPI_STATUS ret;
	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_selftest_3des((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto_selftest_3des failed");
}

void hex_dump_array(const u8_t *arr, size_t arr_len)
{
	u32_t cnt = 0;

	for (cnt = 0; cnt < arr_len; cnt++)
		TC_PRINT("%02x ", arr[cnt]);
}


int32_t brcm_gcm_self_test_01(crypto_lib_handle *cryptoHandle)
{
	u8_t key[16] = {
		0xAD, 0x7A, 0x2B, 0xD0, 0x3E, 0xAC, 0x83, 0x5A,
		0x6F, 0x62, 0x0F, 0xDC, 0xB5, 0x06, 0xB3, 0x45
	};
	size_t key_len = 128;
	u8_t iv[16] = {
		0x12, 0x15, 0x35, 0x24, 0xC0, 0x89, 0x5E, 0x81,
		0xB2, 0xC2, 0x84, 0x65
	};
	size_t iv_len = 12;
	u8_t plain_txt[] = {
		0x08, 0x00, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
		0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
		0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
		0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
		0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x00, 0x02
	};

	u8_t aad[] = {
		0xD6, 0x09, 0xB1, 0xF0, 0x56, 0x63, 0x7A, 0x0D,
		0x46, 0xDF, 0x99, 0x8D, 0x88, 0xE5, 0x2E, 0x00,
		0xB2, 0xC2, 0x84, 0x65, 0x12, 0x15, 0x35, 0x24,
		0xC0, 0x89, 0x5E, 0x81
	};
	u8_t cipher_txt[100] = {0};
	u8_t plain_decrypt[100] = {0};
	u8_t gcm_tag[16] = {0};

	TC_PRINT("\nStarting AES GCM test with 16 byte key\n");
	crypto_symmetric_aes_gcm_encrypt((crypto_lib_handle *)&cryptoHandle,
					key, key_len, iv, iv_len, plain_txt,
					sizeof(plain_txt), aad, sizeof(aad),
					cipher_txt, gcm_tag);

	crypto_symmetric_aes_gcm_decrypt((crypto_lib_handle *)&cryptoHandle,
					key, key_len, iv, iv_len, cipher_txt,
					sizeof(plain_txt), aad, sizeof(aad),
					gcm_tag, plain_decrypt);

	if (memcmp(plain_txt, plain_decrypt, sizeof(plain_txt)) != 0) {
		TC_PRINT("GCM self test failed\n");
		hex_dump_array(plain_txt, sizeof(plain_txt));
		return -1;
	}
	TC_PRINT("GCM self test successful\n");

	return 0;
}

int32_t brcm_gcm_self_test_02(crypto_lib_handle *cryptoHandle)
{
	u8_t key[32] = {
		0x4C, 0x97, 0x3D, 0xBC, 0x73, 0x64, 0x62, 0x16,
		0x74, 0xF8, 0xB5, 0xB8, 0x9E, 0x5C, 0x15, 0x51,
		0x1F, 0xCE, 0xD9, 0x21, 0x64, 0x90, 0xFB, 0x1C,
		0x1A, 0x2C, 0xAA, 0x0F, 0xFE, 0x04, 0x07, 0xE5
	 };
	size_t key_len = 256;
	u8_t iv[16] = {
		0x7A, 0xE8, 0xE2, 0xCA, 0x4E, 0xC5, 0x00, 0x01,
		0x2E, 0x58, 0x49, 0x5C
	};
	size_t iv_len = 12;
	u8_t plain_txt[] = {
		0x08, 0x00, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
		0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
		0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
		0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
		0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
		0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
		0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,
		0x45, 0x46, 0x47, 0x48, 0x49, 0x00, 0x08
	};

	u8_t aad[] = {
		0x68, 0xF2, 0xE7, 0x76, 0x96, 0xCE, 0x7A, 0xE8,
		0xE2, 0xCA, 0x4E, 0xC5, 0x88, 0xE5, 0x4D, 0x00,
		0x2E, 0x58, 0x49, 0x5C
	};
	u8_t cipher_txt[100] = {0};
	u8_t plain_decrypt[100] = {0};
	u8_t gcm_tag[16] = {0};

	TC_PRINT("\nStarting AES GCM test with 32 byte key\n");

	crypto_symmetric_aes_gcm_encrypt((crypto_lib_handle *)&cryptoHandle,
					key, key_len, iv, iv_len,
					plain_txt, sizeof(plain_txt),
					aad, sizeof(aad),
					cipher_txt, gcm_tag);

	crypto_symmetric_aes_gcm_decrypt((crypto_lib_handle *)&cryptoHandle,
					key, key_len, iv, iv_len,
					cipher_txt, sizeof(plain_txt),
					aad, sizeof(aad),
					gcm_tag, plain_decrypt);

	if (memcmp(plain_txt, plain_decrypt, sizeof(plain_txt)) != 0) {
		TC_PRINT("GCM self test failed\n");
		hex_dump_array(plain_txt, sizeof(plain_txt));
		return -1;
	}
	TC_PRINT("GCM self test successful\n");

	return 0;
}

static void crypto_selftest_aes_gcm_s(void)
{
	BCM_SCAPI_STATUS ret;

	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = brcm_gcm_self_test_01((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto selftest gcm 01 failed");
	ret = brcm_gcm_self_test_02((crypto_lib_handle *)&cryptoHandle);
	zassert_equal(ret, 0, "crypto selftest gcm 02 failed");
}

SHELL_TEST_DECLARE(pci_eval_test);
SHELL_TEST_DECLARE(test_crypto)
{
	/* Call PCI eval test if called with arguments */
	if (argc >= 2) {
		return SHELL_TEST_CALL(pci_eval_test, argc, argv);
	}

	ztest_test_suite(crypto_tests,  ztest_unit_test(crypto_rng_selftest_s),
				ztest_unit_test(crypto_selftest_sha1_s),
				ztest_unit_test(crypto_selftest_sha256_s),
				ztest_unit_test(crypto_selftest_hmac_sha256_s),
				ztest_unit_test(crypto_selftest_sha3_224_s),
				ztest_unit_test(crypto_selftest_sha3_256_s),
				ztest_unit_test(crypto_selftest_sha3_384_s),
				ztest_unit_test(crypto_selftest_sha3_512_s),
				ztest_unit_test(crypto_selftest_des_s),
				ztest_unit_test(crypto_selftest_3des_s),
				ztest_unit_test(crypto_selftest_aes_cbc_s),
				ztest_unit_test(crypto_selftest_aes_ctr_s),
				ztest_unit_test(crypto_selftest_aes_128_ccm_s),
				ztest_unit_test(crypto_selftest_aes_gcm_s),
				ztest_unit_test(crypto_selftest_aes_cbc_sha256_hmac_s));

	ztest_run_test_suite(crypto_tests);

	return 0;
}
