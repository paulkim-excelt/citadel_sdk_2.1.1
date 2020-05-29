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
#include <sys_clock.h>
#include <timer.h>
#include <test.h>

#define MS_TO_NS(val)		(val * NSEC_PER_USEC * USEC_PER_MSEC)

volatile bool test;

static void timer_cb(void *unused)
{
	ARG_UNUSED(unused);

	test = true;
}

static void timer_irq_test(struct device *dev)
{
	int rc, time;

	TC_PRINT("Timer IRQ test:\t\t\t");

	rc = timer_register(dev, timer_cb, NULL, TIMER_PERIODIC);
	zassert_equal(rc, 0, "Error registering timer");

	/* Test for 1, 10, 100, 1000ms */
	for (time = 1; time <= 1000; time *= 10) {
		test = false;
		rc = timer_start(dev, MS_TO_NS(time));
		zassert_equal(rc, 0, "Error starting timer");

		/* It is possible for it to pop the next tick, depending on when
		 * this is executed.  Add one more tick for this reason.
		 */
		k_sleep(time + (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC));

		zassert_equal(test, true, "Timer should have popped");
	}
	TC_PRINT("PASSED\n");
}

static void timer_poll_test(struct device *dev)
{
	int rc, time, i, ticks;

	TC_PRINT("Timer polling test:\t\t");

	rc = timer_register(dev, NULL, NULL,
			    TIMER_NO_IRQ | TIMER_ONESHOT | TIMER_PERIODIC);
	zassert_equal(rc, 0, "Error registering timer");

	/* Test for 1, 10, 100, 1000ms */
	for (time = 1; time <= 1000; time *= 10) {
		rc = timer_start(dev, MS_TO_NS(time));
		zassert_equal(rc, 0, "Error starting timer");

		/* It is possible for it to pop the next tick, depending on when
		 * this is executed.  Add one more tick for this reason.
		 */
		ticks = MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
		for (i = 0; timer_query(dev) && i <= time + ticks; i++)
			k_sleep(1);

		zassert(i <= time+1, "Timer should have popped",
			"Timer should have popped");
	}
	TC_PRINT("PASSED\n");
}

static void timer_suspend_resume_test(struct device *dev)
{
	int rc, time;

	TC_PRINT("Timer suspend/resume test:\t");

	rc = timer_register(dev, timer_cb, NULL, TIMER_PERIODIC);
	zassert_equal(rc, 0, "Error registering timer");

	/* Test for 1, 10, 100, 1000ms */
	for (time = 1; time <= 1000; time *= 10) {
		u32_t before;

		test = false;
		rc = timer_start(dev, MS_TO_NS(time));
		zassert_equal(rc, 0, "Error starting timer");

		timer_suspend(dev);
		before = timer_query(dev);
		k_sleep(1);
		zassert_equal(before, timer_query(dev),
			      "Time should be the same while suspended");
		zassert_equal(test, false, "Timer should not have popped");
		timer_resume(dev);

		/* It is possible for it to pop the next tick, depending on when
		 * this is executed.  Add one more tick for this reason.
		 */
		k_sleep(time + (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC));
		zassert_equal(test, true, "Timer should have popped");
	}
	TC_PRINT("PASSED\n");
}

static void timer_test_battery(void)
{
	struct device *dev;

	/* Get the timer to run the tests on */
	TC_PRINT("Timer0\n");
	dev = device_get_binding(TIMER0_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer1\n");
	dev = device_get_binding(TIMER1_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer2\n");
	dev = device_get_binding(TIMER2_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer3\n");
	dev = device_get_binding(TIMER3_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer4\n");
	dev = device_get_binding(TIMER4_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer5\n");
	dev = device_get_binding(TIMER5_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer6\n");
	dev = device_get_binding(TIMER6_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);

	TC_PRINT("Timer7\n");
	dev = device_get_binding(TIMER7_NAME);
	zassert_not_equal(dev, NULL, "Error getting Timer device");

	timer_irq_test(dev);
	timer_poll_test(dev);
	timer_suspend_resume_test(dev);
}

SHELL_TEST_DECLARE(test_timer)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(timer_tests, ztest_unit_test(timer_test_battery));

	ztest_run_test_suite(timer_tests);

	return 0;
}
