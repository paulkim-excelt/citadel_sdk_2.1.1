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

/* @file test_zbar.c
 *
 * Unit test for zbar application
 *
 * This file implements the unit tests for the bcm58202 zbar application
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <test.h>
#include <zbar.h>

#ifdef CONFIG_ZBAR_STATIC_TEST_Y800
#include "QR_Image_Y800_VGA.c"
static unsigned int demo_img_y8_width = 640;
static unsigned int demo_img_y8_height = 480;
static const char *demo_img_y8_data_ret = "QR-Code:This is Citadel testing image";
#endif

#ifdef CONFIG_ZBAR_STATIC_TEST_YUY2
#include "QR_Image_YUY2_QQVGA.c"
static unsigned int demo_img_yuy2_width = 160;
static unsigned int demo_img_yuy2_height = 120;
static const char *demo_img_yuy2_data_ret = "QR-Code:http://weixin.qq.com/r/kmdnf8LETJ8zrYYc9zKA";
#endif

static int zbar_test_usage(void)
{
	TC_PRINT("Usage:\n");
	TC_PRINT("\tzbar <image_type>\n");
	TC_PRINT("\tY800: QRcode decode from Y800 image\n");
	TC_PRINT("\tYUY2: QRcode decode from YUV422 image\n");
	return 0;
}

#ifdef CONFIG_ZBAR_STATIC_TEST_Y800
void test_zbar_y800(void)
{
	int rc;
	char output[64];
	unsigned int len = 0;

	memset(output, 0, sizeof(output));

	TC_PRINT("Decoding Y800 image in flash\n");

	rc = zbarimg_scan_one_y8_image(
			(unsigned char *)demo_img_y8_data,
			demo_img_y8_width,
			demo_img_y8_height,
			output,
			&len);
	zassert_true(rc >= 1, "Failed to decode QR code");

	TC_PRINT("Decoded: '%s', len=%d\n", output, len);
	TC_PRINT("Expected: '%s'\n", demo_img_y8_data_ret);

	zassert_false(strncmp(output, demo_img_y8_data_ret,
		      strlen(demo_img_y8_data_ret)), "Test Fail!");
}
#else
void test_zbar_y800(void)
{
	TC_PRINT("Test not enabled!\n");
}
#endif

#ifdef CONFIG_ZBAR_STATIC_TEST_YUY2
void test_zbar_yuy2(void)
{
	int rc;
	char output[64];
	unsigned int len = 0;

	memset(output, 0, sizeof(output));

	TC_PRINT("Decoding YUY2 image in flash\n");

	rc = zbarimg_scan_one_yuv422_image(
			(unsigned char *)demo_img_yuy2_data,
			demo_img_yuy2_width,
			demo_img_yuy2_height,
			output,
			&len);
	zassert_true(rc >= 1, "Failed to decode QR code");

	TC_PRINT("Decoded: '%s', len=%d\n", output, len);
	TC_PRINT("Expected: '%s'\n", demo_img_yuy2_data_ret);

	zassert_false(strncmp(output, demo_img_yuy2_data_ret,
		      strlen(demo_img_yuy2_data_ret)), "Test Fail!");
}
#else
void test_zbar_yuy2(void)
{
	TC_PRINT("Test not enabled!\n");
}
#endif

SHELL_TEST_DECLARE(test_zbar)
{
	if (argc == 1) {
		zbar_test_usage();
		return TC_FAIL;
	}

	TC_PRINT("Decoding %s image in flash\n", argv[1]);

	if (!strcmp(argv[1], "Y800")) {
		ztest_test_suite(zbar_tests, ztest_unit_test(test_zbar_y800));
		ztest_run_test_suite(zbar_tests);
	} else if (!strcmp(argv[1], "YUY2")) {
		ztest_test_suite(zbar_tests, ztest_unit_test(test_zbar_yuy2));
		ztest_run_test_suite(zbar_tests);
	} else {
		ztest_test_suite(zbar_tests,
				 ztest_unit_test(test_zbar_y800),
				 ztest_unit_test(test_zbar_yuy2));
		ztest_run_test_suite(zbar_tests);
	}

	return 0;
}
