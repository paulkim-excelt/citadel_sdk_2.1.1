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

/*
 * @file test_spl.c
 * @brief SPL test functions
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <broadcom/gpio.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <kernel.h>
#include <spl.h>

#define MAX_FREQ		0xFFFF
#define SPL_MAGIC		0xABCD
#define SPL_DELAY_MAX		0xffff

s32_t spl_test;

void test_handler(void)
{
	spl_test = SPL_MAGIC;
	TC_PRINT("SPL ISR");
}

static void spl_delay(uint32_t cnt)
{
	volatile u32_t counter;

	for (counter = 0; counter < cnt;)
		counter++;
}

static s32_t spl_test_freq_monitor(u8_t monitor_id)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	u32_t min = 0, max = MAX_FREQ, test = 0, count = 0;
	u8_t high_freq = 0, low_freq = 0, reset = 0;
	struct frequency_monitor freq_mon;
	s32_t rv = 0;

	do {
		count++;
		if (count > 64) {
			TC_PRINT("Monitor %d: iterate time out!\n", monitor_id);
			break;
		}
		test = (min + max) / 2;
		rv = spl_clear_event_log(spl);
		if (rv) {
			TC_PRINT("Clear log failed");
			return TC_FAIL;
		}

		freq_mon.low_en = 0;
		freq_mon.high_en = 0;
		freq_mon.monitor_point = 0x400;
		freq_mon.low_threshold = 0;
		freq_mon.high_threshold = 0;
		rv = spl_set_freq_monitor(spl, monitor_id, &freq_mon);
		if (rv) {
			TC_PRINT("Enable frequency monitor failed");
			return TC_FAIL;
		}

		freq_mon.low_en = 1;
		freq_mon.high_en = 1;
		freq_mon.monitor_point = 0x400;
		freq_mon.low_threshold = test;
		freq_mon.high_threshold = test + 1;
		rv = spl_set_freq_monitor(spl, monitor_id, &freq_mon);
		if (rv) {
			TC_PRINT("Enable frequency monitor failed");
			return TC_FAIL;
		}

		spl_delay(SPL_DELAY_MAX);

		rv = spl_read_event_log(spl, &high_freq, &low_freq, &reset);
		if (rv) {
			TC_PRINT("Read log failed");
			return TC_FAIL;
		}

		if (high_freq & (0x1 << monitor_id)) {
			TC_PRINT("Monitor %d: frequency is greater than 0x%x\n",
					 monitor_id, test + 1);
			min = test + 1;
		} else if (low_freq & (0x1 << monitor_id)) {
			TC_PRINT("Monitor %d: frequency is less than 0x%x\n",
						monitor_id, test);
			max = test;
		} else {
			TC_PRINT("Monitor %d: frequency is 0x%x\n",
							monitor_id, test);
			break;
		}
	} while (min < max);

	return TC_PASS;
}

static u32_t spl_test_freq(void)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	struct frequency_monitor mon;
	s32_t rv = 0;
	u8_t id;

	for (id = 0; id < 3; id++) {
		rv = spl_get_freq_monitor(spl, id, &mon);
		if (rv)
			TC_PRINT("Read frequency monitor failed\n");

		TC_PRINT("Monitor %d: low_en %d, high_en %d", id, mon.low_en,
								mon.high_en);
		TC_PRINT("low_thresh 0x%x, high_thresh 0x%x\n",
					mon.low_threshold, mon.high_threshold);

		mon.low_en = 0;
		mon.high_en = 0;
		mon.monitor_point = 0x400;
		mon.low_threshold = 0;
		mon.high_threshold = 0;
		rv = spl_set_freq_monitor(spl, id, &mon);
		if (rv)
			TC_PRINT("Disable frequency monitor failed\n");
	}

	return TC_PASS;
}

static s32_t spl_test_reset_monitor_01(void)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	struct reset_monitor mon;
	s32_t rv = 0;
	u8_t id;

	for (id = 0; id < 4; id++) {
		mon.reset_en = 0;
		mon.reset_count_threshold = 1;
		mon.window_count_threshold = 1;
		rv = spl_set_reset_monitor(spl, id, &mon);
		if (rv) {
			TC_PRINT("Enable reset monitor failed\n");
			return rv;
		}

		mon.reset_en = 1;
		mon.reset_count_threshold = 1;
		mon.window_count_threshold = 1;
		rv = spl_set_reset_monitor(spl, id, &mon);
		if (rv) {
			TC_PRINT("Enable reset monitor failed\n");
			return rv;
		}
	}

	return TC_PASS;
}

static s32_t spl_test_reset_monitor_02(void)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	struct reset_monitor mon;
	s32_t rv = 0;
	u8_t id;

	for (id = 0; id < 4; id++) {
		rv = spl_get_reset_monitor(spl, id, &mon);
		if (rv) {
			TC_PRINT("Read reset monitor failed\n");
			return TC_FAIL;
		}
		TC_PRINT("Monitor %d: reset %d, window %d\n", id,
					 mon.reset_count, mon.window_count);
	}

	return TC_PASS;
}

static int32_t spl_test_watchdog_monitor(void)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	s32_t rv;
	u32_t loop = 50000, i = 0, value = 0;

	rv = spl_register_isr(spl, SPL_WDOG_INTERRUPT, test_handler);
	if (rv) {
		TC_PRINT("Register handler failed\n");
		return TC_FAIL;
	}

	spl_test = 0;
	rv = spl_set_watchdog_val(spl, loop);
	if (rv) {
		TC_PRINT("Set watchdog failed\n");
		return TC_FAIL;
	}

	while (i < loop) {
		if (spl_test == SPL_MAGIC) {
			break;
		}

		TC_PRINT("+");
		i++;
	}
	TC_PRINT("\n");

	rv = spl_get_watchdog_val(spl, &value);
	if (rv) {
		TC_PRINT("Get watchdog failed\n");
		return TC_FAIL;
	}

	if (value != 0) {
		TC_PRINT("Watchdog wrong result %d\n", value);
		return TC_FAIL;
	}

	rv = spl_unregister_isr(spl, SPL_WDOG_INTERRUPT);
	if (rv) {
		TC_PRINT("Unregister handler failed\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static s32_t spl_test_log(void)
{
	struct device *spl = device_get_binding(CONFIG_SPL_DEV_NAME);
	u8_t high_freq = 0, low_freq = 0, reset = 0;
	s32_t rv;

	rv = spl_read_event_log(spl, &high_freq, &low_freq, &reset);
	if (rv) {
		TC_PRINT("Read log failed\n");
		return TC_FAIL;
	}
	TC_PRINT("high_freq = %d, low_freq = %d, reset = %d\n", high_freq,
						low_freq, reset);

	rv = spl_clear_event_log(spl);
	if (rv) {
		TC_PRINT("Clear log failed\n");
		return TC_FAIL;
	}

	return TC_PASS;
}




static void spl_tests(void)
{
	zassert_true(spl_test_log() == TC_PASS, NULL);
	zassert_true(spl_test_freq_monitor(0) == TC_PASS, NULL);
	zassert_true(spl_test_freq_monitor(1) == TC_PASS, NULL);
	zassert_true(spl_test_freq_monitor(2) == TC_PASS, NULL);
	zassert_true(spl_test_freq() == TC_PASS, NULL);

	zassert_true(spl_test_reset_monitor_01() == TC_PASS, NULL);
	zassert_true(spl_test_reset_monitor_02() == TC_PASS, NULL);

	zassert_true(spl_test_watchdog_monitor() == TC_PASS, NULL);
}

s32_t test_spl(s32_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(spl_driver_tests, ztest_unit_test(spl_tests));
	ztest_run_test_suite(spl_driver_tests);

	return 0;
}
