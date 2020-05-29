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
 * @file test_dma.c
 * @brief gpio driver test
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <broadcom/gpio.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

static void gpio_handler(struct device *port, struct gpio_callback *cb,
			 u32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);

	TC_PRINT("GPIO_test_handler: bit mask %x\n", pins);
}

static int test_task(const char *dev, bool interrupt_support, u32_t no_of_pins)
{
	struct device *gpio = device_get_binding(dev);
	struct gpio_callback cb_str;
	u32_t pin, flags;
	u32_t val, read_val;

	TC_PRINT("\nStarting test on dev %s\n", dev);
	val = 0;
	for (pin = 0; pin < no_of_pins; pin++) {
		/* Save current pin configuration */
		gpio_pin_configure_get(gpio, pin, &flags);

		gpio_pin_read(gpio, pin, &read_val);
		if (!read_val) {
			val = 0x01;
			gpio_pin_write(gpio, pin, val);
			gpio_pin_read(gpio, pin, &read_val);
		}
		gpio_pin_configure(gpio, pin, GPIO_DIR_OUT);
		gpio_pin_read(gpio, pin, &read_val);
		TC_PRINT("\nPin %d Read val %d\n", pin, read_val);

		if (interrupt_support == true) {
			gpio_init_callback(&cb_str, gpio_handler, (1 << pin));
			if (gpio_add_callback(gpio, &cb_str) < 0) {
				TC_PRINT("Callback fail\n");
				return TC_FAIL;
			}
		}

		gpio_pin_configure(gpio, pin,
				   GPIO_DIR_OUT | GPIO_INT_EDGE | GPIO_INT);

		if (interrupt_support == true) {
			gpio_pin_enable_callback(gpio, pin);
		}

		val = 0;
		TC_PRINT("Setting pin %d val %d\n", pin, val);
		gpio_pin_write(gpio, pin, val);
		gpio_pin_read(gpio, pin, &read_val);
		TC_PRINT("Read val %d\n", read_val);

		if (interrupt_support == true) {
			gpio_pin_disable_callback(gpio, pin);
			gpio_remove_callback(gpio, &cb_str);
		}

		/* Restore initial configuration after test */
		gpio_pin_configure(gpio, pin, flags);
	}

	TC_PRINT("Test finished\n");
	return TC_PASS;
}

static void test_all_gpio(void)
{
	int rc;

	TC_PRINT("\nGPIO test\n");

	rc = test_task(CONFIG_GPIO_DEV_NAME_0, true, 32);
	zassert_true((rc == TC_PASS), NULL);

	rc = test_task(CONFIG_GPIO_DEV_NAME_1, true, 32);
	zassert_true((rc == TC_PASS), NULL);

	rc = test_task(CONFIG_GPIO_DEV_NAME_2, true, 5);
	zassert_true((rc == TC_PASS), NULL);

#ifdef CONFIG_AON_GPIO_INTERRUPT_SUPPORT
	rc = test_task(CONFIG_GPIO_AON_DEV_NAME, true, 11);
#else
	rc = test_task(CONFIG_GPIO_AON_DEV_NAME, false, 11);
#endif
	zassert_true((rc == TC_PASS), NULL);
}

SHELL_TEST_DECLARE(test_gpio)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_BOARD_SERP_CP
	/* Running GPIO tests (which toggles all gpios by configuring them as
	 * output causes board to power down. So disabling these tests for serp_cp
	 */
	TC_PRINT("GPIO Tests disabled for serp_cp board!\n");
	return 0;
#endif

	ztest_test_suite(gpio_tests, ztest_unit_test(test_all_gpio));

	ztest_run_test_suite(gpio_tests);

	return 0;
}
