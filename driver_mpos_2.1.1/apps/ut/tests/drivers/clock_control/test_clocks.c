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
#include <board.h>
#include <device.h>
#include <test.h>
#include <broadcom/clock_control.h>

static void genpll_test(int chan)
{
	struct device *dev;
	u32_t i, rate;
	int rc;

	TC_PRINT("Querying channel %d\n", chan);

	dev = device_get_binding(CLOCK_CONTROL_NAME);
	zassert_not_equal(dev, NULL, "Error querying clock rate");

	TC_PRINT("Getting rate for channel %d\n", chan);
	rc = clock_control_get_rate(dev, (void *)chan, &rate);
	if (!rc)
		TC_PRINT("\tChannel %d has a rate of %d\n", chan, rate);
	zassert_equal(rc, 0, "Error querying clock rate");

	/* Messing with the speed of chan 2 or 3 will cause the UART to stop
	 * working
	 */
	if (chan == 2 || chan == 3) {
		TC_PRINT("Skipping channel %d tests to prevent UART issues\n",
			 chan);
		return;
	}

	/* Just try some random speeds and verify they are accepted */
	TC_PRINT("Setting speeds for channel %d\n", chan);
	for (i = 10; i <= 1000; i += 10) {
		rc = clock_control_set_rate(dev, (void *)chan, MHZ(i));
		zassert_equal(rc, 0, "Error setting clock rate");
	}

	rc = clock_control_set_rate(dev, (void *)chan, rate);
	zassert_equal(rc, 0, "Error resetting clock rate");
}

static void a7_test(void)
{
	int rc;
	struct device *dev;
	u32_t i, rate, before, after, key;

	TC_PRINT("Querying for CPU\n");

	dev = device_get_binding(A7_CLOCK_CONTROL_NAME);
	zassert_not_equal(dev, NULL, "Error querying clock rate");

	TC_PRINT("Getting rate for CPU\n");
	rc = clock_control_get_rate(dev, NULL, &rate);
	if (!rc)
		TC_PRINT("\tCPU has a rate of %d\n", rate);
	zassert_equal(rc, 0, "Error querying clock rate");

	TC_PRINT("Setting speeds for CPU\n");
	/* Attempt to set the CPU speed for every possible value */

	/* For CPU speeds between 1 and 10 MHz, the system is always in
	 * IRQ mode owing to the slow CPU speed and high tick interrupt rate
	 * (1 ms). To avoid this lock-up the CPU speeds between 1 and 10 MHz
	 * will be tested with interrupts locked.
	 */
	key = irq_lock();
	for (i = 1; i < 10; i++) {
		rc = clock_control_set_rate(dev, NULL, MHZ(i));
		zassert_equal(rc, 0, "Error setting clock rate");
	}
	irq_unlock(key);

	/* Attempt to set the CPU speed for every possible value */
	for (i = 10; i <= 1000; i++) {
		rc = clock_control_set_rate(dev, NULL, MHZ(i));
		zassert_equal(rc, 0, "Error setting clock rate");
	}

	/* Set speed back to original value */
	rc = clock_control_set_rate(dev, NULL, rate);
	zassert_equal(rc, 0, "Error resetting clock rate");

	/* Now see how many operations we can do in a second */
	/* get the time */
	before = _timer_cycle_get_32();

	/* Half the iterations because we're doing the count and branch */
	for (i = 0; i < 1000 / 2; i++)
		__asm__ ("nop");

	/* get the time again */
	after = _timer_cycle_get_32();

	/* determine the difference and extrapolate to per sec */
	TC_PRINT("CPU performed %d inst in %d jiffies at %d MHz, %d kIPS\n",
		 1000, after - before, rate,
		 CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / (after - before));
}

static void query_clocks(void)
{
	int i;

	/* Test every GENPLL channel */
	for (i = 1; i < 7; i++)
		genpll_test(i);

	a7_test();
}

SHELL_TEST_DECLARE(test_clocks)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(clock_tests, ztest_unit_test(query_clocks));

	ztest_run_test_suite(clock_tests);

	return 0;
}

#define REF_CLK_FREQ_MHZ 26
#define NUM_ITERATIONS 2000000
static inline void print_line(void)
{
	TC_PRINT("-----------------------------------------------");
	TC_PRINT("-----------------------------------------------");
	TC_PRINT("-----------------------------\n");
}

static inline void print_header(void)
{
	print_line();
	TC_PRINT("rate(mhz) - mdiv1 pdiv1 ndivint1 ndivfrac1   vco1 resrate1 error1 - ");
	TC_PRINT("mdiv2 pdiv2 ndivint2 ndivfrac2   vco2  resrate2 error2\n");
	print_line();
}

static void test_cpu_clock(u32_t minfreq, u32_t maxfreq, bool apply, bool verify, u32_t method)
{
	int rc;
	struct device *dev;
	u32_t i, rate, before, after, key;
	struct clock_params p;
	u32_t test_freq, actual_speed;

	/* method != 0 => Use old method */
	if ((verify || apply) && method)
		method = 1;
	else
		method = 0;
	TC_PRINT("\n--- Verifying CPU Clock Speeds from %dMHz to %dMHz  ---\n", minfreq, maxfreq);
	TC_PRINT("Querying for CPU\n");
	dev = device_get_binding(A7_CLOCK_CONTROL_NAME);
	zassert_not_equal(dev, NULL, "Error querying clock rate");
	TC_PRINT("Getting rate for CPU\n");
	rc = clock_control_get_rate(dev, NULL, &rate);
	if (!rc)
		TC_PRINT("\tCurrent CPU clock is %dHz\n", rate);
	zassert_equal(rc, 0, "Error querying clock rate");

	/* Testing Calculations and errors in output for different
	 * frequencies.
	 * Calculate different parameters for different speeds
	 * using old and new methods & and compare errors.
	 */
	TC_PRINT("Parameter Calculations (All Frequencies & Errors in MHz):\n");
	TC_PRINT("rate(mhz) - mdiv1 pdiv1 ndivint1 ndivfrac1   vco1 resrate1 error1 - ");
	TC_PRINT("mdiv2 pdiv2 ndivint2 ndivfrac2   vco2  resrate2 error2\n");
	for (test_freq = minfreq; test_freq <= maxfreq; test_freq++) {
		if (!(test_freq % 30) || verify)
			print_header();
		compare_clk_params(dev, test_freq, &p);
		TC_PRINT("%9d - %5d %5d %8d %9d %6lld %8d %6d - ",
				 test_freq, p.mdiv[0], p.pdiv[0], p.ndivint[0],
				 p.ndivfrac[0], p.fvco[0], p.resrate[0], p.error[0]);
		TC_PRINT("%5d %5d %8d %9d %6lld %9d %6d\n",
				 p.mdiv[1], p.pdiv[1], p.ndivint[1], p.ndivfrac[1],
				 p.fvco[1], p.resrate[1], p.error[1]);

		if (verify || apply) {
			print_line();
			TC_PRINT("Method:%d for Frequency: %dMHz\n", method, test_freq);
			TC_PRINT("Applied Parameters:   ");
			TC_PRINT("mdiv:%d, pdiv:%d, ndivint:%d, ndivfrac:%d, ",
					 p.mdiv[method], p.pdiv[method],
					 p.ndivint[method], p.ndivfrac[method]);
			TC_PRINT("fvco:%lld, resrate:%d, error:%d\n",
					 p.fvco[method], p.resrate[method], p.error[method]);
			/* For CPU speeds between 1 and 10 MHz, the system is always in
			 * IRQ mode owing to the slow CPU speed and high tick interrupt rate
			 * (1 ms). To avoid this lock-up the CPU speeds between 1 and 10 MHz
			 * will be tested with interrupts locked.
			 */
			if (test_freq < 10)
				key = irq_lock();
			apply_clk_params(dev, p.mdiv[method], p.pdiv[method],
					p.ndivint[method], p.ndivfrac[method]);
			if (test_freq < 10)
				irq_unlock(key);
			get_effective_clk_params(dev, &p, method);
			TC_PRINT("Effective Parameters: ");
			TC_PRINT("mdiv:%d, pdiv:%d, ndivint:%d, ndivfrac:%d, ",
					 p.effmdiv, p.effpdiv, p.effndivint, p.ndivfrac[method]);
			TC_PRINT("fvco:%lld, resrate:%d\n", p.efffvco, p.effresrate);
			if (verify) {
				TC_PRINT("Verifying accuracy of cpu clock for %dMHz\n", test_freq);
				/* Now see how many operations we can do in a second */
				/* get the time */
				before = _timer_cycle_get_32();
				/* Half the iterations because we're doing the count and branch */
				for (i = 0; i < (NUM_ITERATIONS / 2); i++)
					__asm__ ("nop");
				/* get the time again */
				after = _timer_cycle_get_32();
				actual_speed = (NUM_ITERATIONS * REF_CLK_FREQ_MHZ) / (after - before);
				/* determine the difference and extrapolate to per sec */
				TC_PRINT("CPU performed %d inst in %d %dMHz Clock Cycles, ",
					 NUM_ITERATIONS, after - before, REF_CLK_FREQ_MHZ);
				TC_PRINT("Observed Speed:%dMHz\n", actual_speed);
			}
			print_line();
		}
	}

	/* Set speed back to original value */
	test_freq = rate / MHZ(1);
	compare_clk_params(dev, test_freq, &p);
	TC_PRINT("Method:%d for Frequency: %dMHz\n", method, test_freq);
	TC_PRINT("Applied Parameters:   ");
	TC_PRINT("mdiv:%d, pdiv:%d, ndivint:%d, ndivfrac:%d, ",
			 p.mdiv[method], p.pdiv[method], p.ndivint[method], p.ndivfrac[method]);
	TC_PRINT("fvco:%lld, resrate:%d, error:%d\n",
			 p.fvco[method], p.resrate[method], p.error[method]);
	/* For CPU speeds between 1 and 10 MHz, the system is always in
	 * IRQ mode owing to the slow CPU speed and high tick interrupt rate
	 * (1 ms). To avoid this lock-up the CPU speeds between 1 and 10 MHz
	 * will be tested with interrupts locked.
	 */
	if (test_freq < 10)
		key = irq_lock();
	apply_clk_params(dev, p.mdiv[method], p.pdiv[method],
					 p.ndivint[method], p.ndivfrac[method]);
	if (test_freq < 10)
		irq_unlock(key);
	get_effective_clk_params(dev, &p, method);
	TC_PRINT("Effective Parameters: ");
	TC_PRINT("mdiv:%d, pdiv:%d, ndivint:%d, ndivfrac:%d, ",
			 p.effmdiv, p.effpdiv, p.effndivint, p.ndivfrac[method]);
	TC_PRINT("fvco:%lld, resrate:%d\n", p.efffvco, p.effresrate);
	if (verify) {
		TC_PRINT("Verifying accuracy of cpu clock for %dMHz\n", test_freq);
		/* Now see how many operations we can do in a second */
		/* get the time */
		before = _timer_cycle_get_32();
		/* Half the iterations because we're doing the count and branch */
		for (i = 0; i < (NUM_ITERATIONS / 2); i++)
			__asm__ ("nop");
		/* get the time again */
		after = _timer_cycle_get_32();
		actual_speed = (NUM_ITERATIONS * REF_CLK_FREQ_MHZ) / (after - before);
		/* determine the difference and extrapolate to per sec */
		TC_PRINT("CPU performed %d inst in %d %dMHz Clock Cycles, ",
			 NUM_ITERATIONS, after - before, REF_CLK_FREQ_MHZ);
		TC_PRINT("Observed Speed:%dMHz\n", actual_speed);
	}
	print_line();
	TC_PRINT("---Test Complete---\n");
	print_line();
}

SHELL_TEST_DECLARE(test_cpu_speed)
{
	/* Default Range frequency in MHz. */
	u32_t minfreq = 100, maxfreq = 200;
	bool verify = 0, apply = 0;
	/* 0: New more accurate method,
	 * 1: Old simpler but less accurate method
	 */
	u32_t method = 0;
	if (argc > 2) {
		minfreq = atoi(argv[1]);
		maxfreq = atoi(argv[2]);
		if (argc > 3) {
			if(strcmp(argv[3], "apply") == 0)
				apply = 1;
			if(strcmp(argv[3], "verify") == 0)
				verify = 1;
			if (argc > 4)
				method = atoi(argv[4]);
		}
	}
	test_cpu_clock(minfreq, maxfreq, apply, verify, method);

	return 0;
}
