/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
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
 * @file sotp_test.c
 * @brief  sotp tests are performed
 */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <zephyr/types.h>
#include <test.h>
#include <sotp.h>
/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/

/* ***************************************************************************
 * Private Functions
 * ****************************************************************************/
static void test_sotp_read(void)
{
	s32_t i = 0;
	u64_t data = 0;

	for (i = 0; i < 112; i++) {
		TC_PRINT("Read row %d:\n", i);
		data = sotp_mem_read(i, 1);
		TC_PRINT("SOTP row LOW %d = 0x%08x\n", i, (u32_t)(data));
		TC_PRINT("SOTP row HIGH %d = 0x%08x\n", i, (u32_t)(data >> 32));
	}
}

static void test_sotp_write(void)
{
	u64_t data = 0;
	s32_t row = 89;

	data = sotp_mem_read(row, 1);
	TC_PRINT("Before : Row LOW %d: data %x\n", row, (u32_t)(data));
	TC_PRINT("Before : Row HIGH %d: data %x\n", row, (u32_t)(data >> 32));

	data = 0x44332211;
	sotp_mem_write(row, 1, data);

	data = sotp_mem_read(row, 1);
	TC_PRINT("After : Row LOW %d: data %x\n", row, (u32_t)(data));
	TC_PRINT("After : Row HIGH %d: data %x\n", row, (u32_t)(data >> 32));
	zassert_equal((u32_t)(data & 0xffffffff), 0x44332211,
			"After the write data did not match");
}

static void test_sotp_set_key(void)
{
	u8_t dAuthKey[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			};

	sotp_bcm58202_status_t otpStatus = IPROC_OTP_INVALID;

	otpStatus = sotp_set_key(dAuthKey, 32, OTPKey_DAUTH);
	zassert_equal(otpStatus, IPROC_OTP_VALID, "Set key returned error");

	otpStatus = sotp_set_key(dAuthKey, 32, OTPKey_AES);
	zassert_equal(otpStatus, IPROC_OTP_VALID, "Set key returned error");

	otpStatus = sotp_set_key(dAuthKey, 32, OTPKey_HMAC_SHA256);
	zassert_equal(otpStatus, IPROC_OTP_VALID, "Set key returned error");
}

static void test_sotp_read_config(void)
{
	struct sotp_bcm58202_dev_config config;
	sotp_bcm58202_status_t status;
	u32_t customer_id = 0;
	u16_t customer_revision_id = 0, sbi_revision_id = 0;

	status = sotp_read_dev_config(&config);
	zassert_equal(status, IPROC_OTP_VALID, "Reading devcfg failed");
	TC_PRINT("BRCM Revision ID = 0x%x\n", config.BRCMRevisionID);
	TC_PRINT("dev Security Config = 0x%x\n", config.devSecurityConfig);
	TC_PRINT("COTBindingID = 0x%x\n", config.ProductID);

	status = sotp_read_customer_id(&customer_id);
	zassert_equal(status, IPROC_OTP_VALID, "Reading customer ID failed");
	TC_PRINT("customerID = 0x%x\n", customer_id);

	status = sotp_read_customer_config(&customer_revision_id,
					&sbi_revision_id);
	zassert_equal(status, IPROC_OTP_VALID,
			"Reading customer config failed");
	TC_PRINT("Customer revision ID = 0x%x\n", customer_revision_id);
	TC_PRINT("SBI revision ID = 0x%x\n", sbi_revision_id);
}

#define BUF_SIZE 100
static void test_sotp_read_keys(void)
{
	sotp_bcm58202_status_t result = 0;
	s32_t i;
	u8_t buffer_AES[BUF_SIZE] = {0};
	u8_t buffer_AUTH[BUF_SIZE] = {0};
	u8_t buffer_HMAC[BUF_SIZE] = {0};
	s32_t *buffer = NULL;
	u16_t size = 0;

	result = sotp_read_key(buffer_AUTH, &size, OTPKey_DAUTH);
	zassert_equal(result, IPROC_OTP_VALID, "Reading DAUTH failed");
	TC_PRINT("OTP_KEY_DAUTH:\n");

	buffer = (s32_t *)buffer_AUTH;
	size = size / 4;
	for (i = 0; i < size; i++)
		TC_PRINT("0x%08X | ", buffer[i]);

	result = sotp_read_key(buffer_HMAC, &size, OTPKey_HMAC_SHA256);
	zassert_equal(result, IPROC_OTP_VALID, "Read buffer_HMAC failed");
	TC_PRINT("OTP_KEY_HMAC_SHA256:\n");

	buffer = (s32_t *)buffer_HMAC;
	size = size / 4;
	for (i = 0; i < size; i++)
		TC_PRINT("0x%08X | ", buffer[i]);

	result = sotp_read_key(buffer_AES, &size, OTPKey_AES);
	zassert_equal(result, IPROC_OTP_VALID, "Read buffer_AES failed");
	TC_PRINT("OTP_KEY_AES:\n");

	buffer = (s32_t *)buffer_AES;
	size = size / 4;
	for (i = 0; i < size; i++)
		TC_PRINT("0x%08X | ", buffer[i]);
}

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/

SHELL_TEST_DECLARE(test_sotp_main)
{
	if (argc == 2) {
		if (strcmp(argv[1], "set_key") == 0) {

			ztest_test_suite(sotp_driver_api_setkey_tests,
					 ztest_unit_test(test_sotp_set_key)
					);

			ztest_run_test_suite(sotp_driver_api_setkey_tests);
			return 0;
		}
	}

	ztest_test_suite(sotp_driver_api_tests,
			 ztest_unit_test(test_sotp_read),
			 ztest_unit_test(test_sotp_write),
			 ztest_unit_test(test_sotp_read_config),
			 ztest_unit_test(test_sotp_read_keys)
			);

	ztest_run_test_suite(sotp_driver_api_tests);

	return 0;
}
