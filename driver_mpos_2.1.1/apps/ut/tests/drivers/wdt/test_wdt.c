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

/* @file test_wdt.c
 *
 * Unit test file for BCM958201 WDT driver.
 *
 */

#include <test.h>
#include <errno.h>
#include <board.h>
#include <string.h>
#include <device.h>
#include <wdt.h>

#define WDT_MAGIC 0x8866
static int wdt_test;

void test_isr(void)
{
	wdt_test = WDT_MAGIC;
	TC_PRINT("\nWDT test ISR invoked!\n");
}

static void wdt_test_no_timeout(struct device *dev)
{
	int rc = 0, i = 0, loop = 1000;

	wdt_test = 0;

	rc = wdt_register_isr(dev, test_isr);
	zassert_equal(rc, 0, "Can't register isr\n");

	rc = wdt_reset_en(dev, false);
	zassert_equal(rc, 0, "Can't disable reset");

	rc = wdt_start(dev, loop);
	zassert_equal(rc, 0, "Can't start watchdog");

	for (i = 0; i < 10; i++) {
		rc = wdt_feed(dev);
		zassert_equal(rc, 0, "Can't feed watchdog");
		k_busy_wait(10 * 1000);
	}

	rc = wdt_stop(dev);
	zassert_equal(rc, 0, "Can't stop watch dog");

	zassert_false((wdt_test == WDT_MAGIC), "WDT ISR called!\n");

	rc = wdt_unregister_isr(dev);
	zassert_equal(rc, 0, "Can't unregister isr");

	TC_PRINT("PASSED\n\n");
}

/* Will trigger interrupt after counter decrements to 0 */
static void wdt_test_timeout(struct device *dev)
{
	int rc = 0, i = 0, loop = 1000;

	wdt_test = 0;

	rc = wdt_register_isr(dev, test_isr);
	zassert_equal(rc, 0, "Can't register isr");

	rc = wdt_reset_en(dev, false);
	zassert_equal(rc, 0, "Can't disable reset");

	rc = wdt_start(dev, loop);
	zassert_equal(rc, 0, "Can't start watch dog");

	while (wdt_test != WDT_MAGIC) {
		TC_PRINT("+");
		i++;
		zassert_false((i > loop), "WDT did not timeout\n");
		k_busy_wait(10 * 1000);
	}

	TC_PRINT("WDT elapsed time before timeout: %d sec\n", (i*10)/loop);

	rc = wdt_stop(dev);
	zassert_equal(rc, 0, "Can't stop watch dog");

	rc = wdt_unregister_isr(dev);
	zassert_equal(rc, 0, "Can't unregister isr");

	TC_PRINT("PASSED\n\n");
}

/* Will trigger reset (if watchdog reset is connected to system reset)
 * after counter decrements to 0 if the interrupt is not handled.
 */
static void wdt_test_reset(struct device *dev)
{
	int rc = 0, i = 0, loop = 1000;

	wdt_test = 0;

	rc = wdt_reset_en(dev, true);
	zassert_equal(rc, 0, "Can't enable reset");

	rc = wdt_start(dev, loop);
	zassert_equal(rc, 0, "Can't start watch dog");

	while (i < loop) {
		TC_PRINT("+");
		i++;
		k_busy_wait(10 * 1000);
	}

	TC_PRINT("\nWDT elapsed time: %d sec\n", (i*10)/loop);
	TC_PRINT("WDT reset not connected to system reset!!!\n");

	rc = wdt_stop(dev);
	zassert_equal(rc, 0, "Can't stop watch dog");

	TC_PRINT("PASSED\n\n");
}

static void test_wdt_main(void)
{
	struct device *dev;

	dev = device_get_binding(CONFIG_WDT_DEV_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	TC_PRINT("Watchdog timer test with no timeout..\n");
	TC_PRINT("=====================================\n");
	wdt_test_no_timeout(dev);

	TC_PRINT("Watchdog timer test with timeout..\n");
	TC_PRINT("==================================\n");
	wdt_test_timeout(dev);

	TC_PRINT("Watchdog timer reset test..\n");
	TC_PRINT("===========================\n");
	wdt_test_reset(dev);
}

SHELL_TEST_DECLARE(test_wdt)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(wdt_driver_tests,
			 ztest_unit_test(test_wdt_main));

	ztest_run_test_suite(wdt_driver_tests);

	return 0;
}
