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
 * @file test_smc.c
 * @brief smc test
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
#include <broadcom/i2c.h>
#include <pinmux.h>

#define PINMUX_SMC_SELECT		PINMUX_FUNC_B

#define IO_EXPANDER_ADDR		(0x20)

#define PCA9505_OUTPUT_BASE		(0x08)
#define PCA9505_CONFIG_BASE		(0x18)

#define SMC_MFIO_START			(37)
#define SMC_PIN_COUNT			(32)

static union dev_config i2c_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

static s32_t i2c_read_io_expander(struct device *dev, u8_t reg_addr, u8_t *val)
{
	struct i2c_msg msgs[2];
	u8_t raddr = reg_addr;

	msgs[0].buf = &raddr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = val;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, IO_EXPANDER_ADDR);
}

static s32_t i2c_write_io_expander(struct device *dev, u8_t reg_addr, u8_t val)
{
	struct i2c_msg msgs;
	u8_t buff[2];
	u8_t value = val;

	buff[0] = reg_addr;
	buff[1] = value;
	msgs.buf = buff;
	msgs.len = 2;
	msgs.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msgs, 1, IO_EXPANDER_ADDR);
}

/* Set ioport port, bank(0~4), port(0~7), value(0~1) */
void ioport_set_port(u8_t bank, u8_t port, u8_t value)
{
	u8_t regval = 0;
	s32_t rv = 0;
	struct device *dev;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found");

	rv = i2c_read_io_expander(dev, (PCA9505_OUTPUT_BASE + bank), &regval);
	zassert_true(rv == 0, "i2c read returned error");
	regval &= ~(0x1 << port);
	regval |= (value & 0x1) << port;
	rv = i2c_write_io_expander(dev, (PCA9505_OUTPUT_BASE + bank), regval);
	zassert_true(rv == 0, "i2c write returned error");

	rv = i2c_read_io_expander(dev, (PCA9505_CONFIG_BASE + bank), &regval);
	zassert_true(rv == 0, "i2c read returned error");
	regval &= ~(0x1 << port);
	rv = i2c_write_io_expander(dev, (PCA9505_CONFIG_BASE + bank), regval);
	zassert_true(rv == 0, "i2c write returned error");
}

s32_t test_pattern(u32_t addr, u32_t length, u32_t pattern)
{
	u32_t *smc_addr;
	u32_t len;
	u32_t cnt;
	u32_t val;

	len = length / 4;
	smc_addr = (u32_t *) addr;
	TC_PRINT("\nTest address: 0x%x pattern 0x%x\n", addr, pattern);
	TC_PRINT("Writing data\n");
	for (cnt = 0; cnt < len; cnt++)
		*(smc_addr + cnt) = pattern;

	TC_PRINT("Done\n");

	TC_PRINT("Reading data and matching with the pattern\n");
	for (cnt = 0; cnt < len; cnt++) {
		val = *(smc_addr + cnt);

		if (val != pattern) {
			TC_ERROR("Error: addr 0x%x pattern 0x%x data 0x%x\n",
					(addr + cnt), pattern, val);
			return TC_FAIL;
		}
	}
	TC_PRINT("Done\n");

	return TC_PASS;
}

s32_t test_incremental(u32_t addr, u32_t length)
{
	u32_t *smc_addr;
	u32_t len;
	u32_t cnt;

	len = length / 4;
	smc_addr = (u32_t *) addr;
	TC_PRINT("\nTest address: 0x%x Incremental data\n", addr);
	TC_PRINT("Writing data\n");
	/* Write incremental data */
	for (cnt = 0; cnt < len; cnt++)
		*(smc_addr + cnt) = cnt;

	TC_PRINT("Done\n");

	TC_PRINT("Reading data and verifying with expected data\n");
	for (cnt = 0; cnt < len; cnt++)
		if (*(smc_addr + cnt) != cnt) {
			TC_ERROR("Error: addr 0x%x expected 0x%x data 0x%x\n",
					(addr + cnt), cnt, *(smc_addr + cnt));
			return TC_FAIL;
		}

	TC_PRINT("Done\n");

	return TC_PASS;
}

static void smc_test(void)
{
	zassert_true(test_pattern(0x61000000, 0x100000, 0xaaaaaaaa) == TC_PASS,
								 NULL);
	zassert_true(test_pattern(0x61000000, 0x100000, 0x55555555) == TC_PASS,
								 NULL);
	zassert_true(test_pattern(0x61000000, 0x100000, 0x0) == TC_PASS,
								 NULL);
	zassert_true(test_incremental(0x61000000, 0x100000) == TC_PASS,
								 NULL);
}

s32_t test_smc(s32_t argc, char *argv[])
{
	struct device *dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	struct device *gpio = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
	u32_t gpio_cfg[SMC_PIN_COUNT];
	u32_t flags;
	s32_t rv = 0;
	u32_t pin;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	/* Select MFIO to SMC */
	TC_PRINT("\nThe settings of MFIO pins related to SMC are altered\n\n");
	for (pin = SMC_MFIO_START; pin < (SMC_MFIO_START + SMC_PIN_COUNT);
							 pin++) {
		rv = pinmux_pin_set(dev, pin, PINMUX_SMC_SELECT);
		zassert_true(rv == 0, "pinmux_pin_set() failed");
	}

	for (pin = 0; pin < SMC_PIN_COUNT; pin++) {
		rv = gpio_pin_configure_get(gpio, pin, &flags);
		zassert_true(rv == 0, "gpio config get returned error");
		gpio_cfg[pin] = flags;
		flags &= ~GPIO_DS_HIGH_MASK;
		flags |= GPIO_DS_ALT_HIGH;
		rv = gpio_pin_configure(gpio, pin, flags);
		zassert_true(rv == 0, "gpio config set returned error");
	}

	/* Configure I2C expander o/p's to select SMC */
	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");
	rv = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(rv == 0, "I2C configure returned error");
	/* Set I/O - Bank, Port, Value */
	ioport_set_port(0, 2, 0);
	ioport_set_port(0, 5, 0);
	ioport_set_port(0, 6, 0);
	ioport_set_port(0, 7, 0);
	ioport_set_port(1, 0, 0);
	ioport_set_port(1, 1, 0);
	ioport_set_port(1, 4, 0);
	ioport_set_port(1, 5, 0);
	ioport_set_port(2, 0, 0);
	ioport_set_port(2, 1, 0);
	ioport_set_port(3, 0, 0);
	ioport_set_port(3, 1, 0);

	ztest_test_suite(smc_driver_tests, ztest_unit_test(smc_test));
	ztest_run_test_suite(smc_driver_tests);

	/* Restore GPIO config */
	for (pin = 0; pin < SMC_PIN_COUNT; pin++) {
		flags = gpio_cfg[pin];
		rv = gpio_pin_configure(gpio, pin, flags);
		zassert_true(rv == 0, "gpio config set returned error");
	}

	return 0;
}
