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
#include <kernel.h>
#include <sys_clock.h>
#include <test.h>
#include <broadcom/pwm.h>

#define PWM_ONE_PULSE_WIDTH		(50000)

static void test_one_pulse_pwm(void)
{
	struct device *dev;
	u32_t polarity = 0;
	u32_t cnt;
	s32_t rv;

	dev = device_get_binding(CONFIG_PWM_ONE_PULSE_BCM58202_DEV_NAME);
	zassert_not_equal(dev, NULL, "Error getting pwm dev");

	/* Generate 50 msec pulse with 1 sec delay for 15 times */
	for (cnt = 0; cnt < 15; cnt++) {
		TC_PRINT("Check PWM0/LED(D9) for pulse of %d\n",
							 PWM_ONE_PULSE_WIDTH);
		rv = pwm_pin_set_usec(dev, 0, 0, PWM_ONE_PULSE_WIDTH, polarity);
		zassert_equal(rv, 0, "Single pulse PWM configure failed");
		k_sleep(1000);
	}
}

static void test_pwm_train(void)
{
	struct device *dev;
	u32_t period, duty_high;
	u32_t polarity = 0;
	u32_t pwm;
	s32_t rv;

	dev = device_get_binding(CONFIG_PWM_BCM58202_DEV_NAME);
	zassert_not_equal(dev, NULL, "Error getting pwm dev");

	/* Activate pwm1 to observe buzzer sound */
	pwm = 1;
	period = 250; /* micro seconds */
	duty_high = 125; /* micro seconds */
	TC_PRINT("\nCheck for Buzzer sound.\n");
	TC_PRINT("PWM-%d Period %d usec, Duty high %d usec\n", pwm,
							period,	duty_high);
	rv = pwm_pin_set_usec(dev, pwm, period, duty_high, polarity);
	zassert_equal(rv, 0, "PWM Configure failed");

	/* Activate pwm0 to observe LED blinking */
	pwm = 0;
	period = 50000;
	duty_high = 20000;
	TC_PRINT("\nCheck for PWM LED(D9) blinking.\n");
	TC_PRINT("PWM-%d Period %d usec, Duty high %d usec\n", pwm,
							period,	duty_high);
	rv = pwm_pin_set_usec(dev, pwm, period, duty_high, polarity);
	zassert_equal(rv, 0, "PWM Configure failed");
	/* Wait for 5 seconds to observe LED and buzzer */
	k_sleep(5000);

	/* Disable both pwm0 and pwm1) */
	pwm = 1;
	period = 0;
	duty_high = 0;
	TC_PRINT("\nDisabling PWM-%d\n", pwm);
	rv = pwm_pin_set_usec(dev, pwm, period, duty_high, polarity);
	zassert_equal(rv, 0, "PWM Configure failed");
	pwm = 0;
	TC_PRINT("Disabling PWM-%d\n", pwm);
	rv = pwm_pin_set_usec(dev, pwm, period, duty_high, polarity);
	zassert_equal(rv, 0, "PWM Configure failed");
}

SHELL_TEST_DECLARE(test_pwm)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(pwm_driver_api_tests,
			 ztest_unit_test(test_one_pulse_pwm),
			 ztest_unit_test(test_pwm_train)
			);

	ztest_run_test_suite(pwm_driver_api_tests);

	return 0;
}
