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
 * @file test_rtc.c
 * @brief RTC driver tests
 *
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/rtc.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

static int rtc_alarm_count = 0;

static void rtc_callback(struct device *dev)
{
	ARG_UNUSED(dev);
	rtc_alarm_count++;
}

/* API param tests */
static void test_rtc_api_set_config(void)
{
	int ret;
	struct device *dev;

	dev = device_get_binding(CONFIG_RTC_DEV_NAME);
	zassert_not_null(dev, "RTC Driver binding not found!");

	ret = rtc_set_config(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

/* Functional tests */
static void test_rtc_read(void)
{
	u32_t rtc;
	struct device *dev;

	dev = device_get_binding(CONFIG_RTC_DEV_NAME);
	zassert_not_null(dev, "RTC Driver binding not found!");

	/* Read the RTC and print out the time */
	rtc = rtc_read(dev);

	TC_PRINT("Date and Time per RTC\n");
	TC_PRINT("%02d/%02d/%04d %02d:%02d:%02d\n",
			(((rtc/(24*60*60)) % 365) % 30) + 1,	/* Day  */
			(((rtc/(24*60*60)) % 365) / 30) + 1,	/* Month */
			rtc/(365*24*60*60) + 1970,	/* Year (add epoch) */
			(rtc/(60*60)) % 24,		/* Hour */
			(rtc/60) % 60,			/* Min */
			rtc % 60);			/* Sec */
}

static void test_rtc_enable_disable(void)
{
	u32_t rtc, i;
	struct device *dev;

	dev = device_get_binding(CONFIG_RTC_DEV_NAME);
	zassert_not_null(dev, "RTC Driver binding not found!");

	/* Disable RTC */
	TC_PRINT("Testing rtc_disable() ");
	rtc_disable(dev);

	/* Check clock is not ticking */
	rtc = rtc_read(dev);
	for (i = 0; i < 5; i++) {
		TC_PRINT(".");
		k_sleep(1000);
	}
	TC_PRINT("\n");
	zassert_equal(rtc, rtc_read(dev), "RTC ticking even when disabled!");

	/* Enable first */
	rtc_enable(dev);

	/* Check clock has ticked ahead appropriately */
	rtc = rtc_read(dev);
	TC_PRINT("Testing rtc_enable() ");
	for (i = 0; i < 5; i++) {
		TC_PRINT(".");
		k_sleep(1000);
	}
	TC_PRINT("\n");
	zassert_true(rtc + 5 <= rtc_read(dev), "RTC didn't tick as expected!");
}

static void test_rtc_set_config(void)
{
	int ret;
	u32_t rtc, time_ms, timeout;
	struct device *dev;
	struct rtc_config cfg;

	dev = device_get_binding(CONFIG_RTC_DEV_NAME);
	zassert_not_null(dev, "RTC Driver binding not found!");

	/* Add 5 seconds to make up for the enable/disable test stopping
	 * clock for 5 seconds
	 */
	rtc = rtc_read(dev);
	cfg.init_val = rtc + 5;
	cfg.alarm_enable = false;

	ret = rtc_set_config(dev, &cfg);
	zassert_equal(ret, 0, "RTC Set config returned error!");

	/* Check rtc is set to 5 seconds ahead */
	zassert_true(rtc + 5 <= rtc_read(dev), "RTC set init val failed");

	/* Check alarm works as expected */
	rtc = rtc_read(dev);
	cfg.init_val = rtc;
	cfg.alarm_enable = true;
	cfg.alarm_val = rtc + 5;
	cfg.cb_fn = rtc_callback;
	rtc_alarm_count = 0; /* Reset alarm count */
	ret = rtc_set_config(dev, &cfg);
	zassert_equal(ret, 0, "RTC Set config returned error!");

	/* Time the alarm expiry */
	time_ms = k_uptime_get_32();
	timeout = 10000; /* 10 seconds */
	while ((rtc_alarm_count == 0) && (--timeout))
		k_sleep(1);
	zassert_true(timeout, "Timed out waiting for rtc callback!");
	time_ms = k_uptime_get_32() - time_ms;

	/* Since the RTC clock res is 1 second, the alarm expiry time can be
	 * anywhere between 4.5 seconds and 5.5 seconds
	 */
	TC_PRINT("Measured time = %d ms\n", time_ms);
	zassert_true(time_ms > 4500, "RTC alarm expired too soon");
	zassert_true(time_ms < 5500, "RTC alarm expired too late");
}

static void test_rtc_set_alarm(void)
{
	int ret;
	u32_t rtc, time_ms, timeout;
	struct device *dev;

	dev = device_get_binding(CONFIG_RTC_DEV_NAME);
	zassert_not_null(dev, "RTC Driver binding not found!");

	rtc = rtc_read(dev);
	rtc_alarm_count = 0;
	ret = rtc_set_alarm(dev, rtc - 1);
	zassert_equal(ret, 0, "RTC Set alarm returned error!");
	/* With alarm < rtc, the alarm should expire immediately */
	zassert_equal(rtc_alarm_count, 1, "RTC alarm did not expire");

	/* Set alarm for 3 seconds into future */
	rtc = rtc_read(dev);
	rtc_alarm_count = 0;
	ret = rtc_set_alarm(dev, rtc + 3);
	zassert_equal(ret, 0, "RTC Set alarm returned error!");

	/* Time the alarm expiry */
	time_ms = k_uptime_get_32();
	timeout = 10000; /* 10 seconds */
	while ((rtc_alarm_count == 0) && (--timeout))
		k_sleep(1);
	zassert_true(timeout, "Timed out waiting for rtc alarm!");
	time_ms = k_uptime_get_32() - time_ms;

	/* The alarm expiry time can be anywhere between 2.5 and 3.5 seconds */
	TC_PRINT("Measured time = %d ms\n", time_ms);
	zassert_true(time_ms > 2500, "RTC alarm expired too soon");
	zassert_true(time_ms < 3500, "RTC alarm expired too late");
}

SHELL_TEST_DECLARE(test_rtc)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(rtc_driver_api_tests,
			 ztest_unit_test(test_rtc_api_set_config));

	ztest_test_suite(rtc_driver_functional_tests,
			 ztest_unit_test(test_rtc_read),
			 ztest_unit_test(test_rtc_enable_disable),
			 ztest_unit_test(test_rtc_set_config),
			 ztest_unit_test(test_rtc_set_alarm));

	ztest_run_test_suite(rtc_driver_api_tests);
	ztest_run_test_suite(rtc_driver_functional_tests);

	return 0;
}
