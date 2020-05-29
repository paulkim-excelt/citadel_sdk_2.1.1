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
 * @file test_keypad.c
 * @brief Keypad test
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <broadcom/gpio.h>
#include <init.h>
#include <keypad.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <kernel.h>

/* IO expander on SVK board needs to be configured
 * to route the SPI2 signals out of the J37 header.
 */
#include <broadcom/i2c.h>
#include <pinmux.h>

/*
 * Make sure the SPI is disabled to test KEYPAD. Both share the MFIO's
 * 61, 62, 63, 64
 */

#define IO_EXPANDER_ADDR	0x20
#define IO_EXPANDER_REG		0x4
#define IO_EXPANDER_REGC	0x2
#define IO_EXPANDER_REGD	0x3

#define NUM_ROWS		4
#define NUM_COLS		4

#ifdef CONFIG_BOARD_CITADEL_SVK_58201
const int mfios[NUM_ROWS + NUM_COLS] = {61, 62, 63, 64, 65, 66, 67, 68};
#elif defined CONFIG_BOARD_SERP_CP
const int mfios[NUM_ROWS + NUM_COLS] = {12, 13, 28, 29, 44, 50, 51, 38};
#else
/* Add the appropriate MFIO pins used for row and columns here, to add driver
 * support
 */
#error Keypad driver not supported for board: CONFIG_BOARD
#endif

#define KEYPAD_READ_TEST_KEYS	16

static union dev_config i2c_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

static int test_i2c_read_io_exander(struct device *dev,
		u8_t reg_addr, u8_t *val)
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

static int test_i2c_write_io_exander(struct device *dev,
		u8_t reg_addr, u8_t val)
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

int configure_keypad_sel(void)
{
	int ret;
	u8_t val = 0;
	struct device *dev;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");

	ret = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(ret == 0, "i2c_configure() returned error");

	/* Read port 2 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGC, &val);
	val |= 0x2;
	val &= ~0x1;
	/* Write to port 2 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGC, val);
	/* Configure port 2 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGC, val);
	/* Read back port 2 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGC, &val);

	/* Read port 3 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGD, &val);
	val |= 0x2;
	val &= ~0x1;
	/* Write to port 3 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGD, val);
	/* Configure port 3 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGD, val);
	/* Read back port 3 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGD, &val);

	return 0;
}

/**
 * @brief Keypad nonblock read test
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return None
 */
static void test_keypad_nonblock_read(struct device *keypad)
{
	u8_t buf[15];
	u8_t cnt = 0;
	u8_t len = 10;
	s32_t rv;

	rv = keypad_read_nonblock(keypad, buf, len);
	if (rv == 0) {
		TC_ERROR("No keys detected\n");
	} else if (rv > 0) {
		TC_PRINT("Keys detected:\n");
		for (cnt = 0; cnt < rv; cnt++)
			TC_PRINT("%d)  %c\n", cnt, buf[cnt]);
	} else {
		TC_ERROR("Function fail %d\n", rv);
	}
}

/**
 * @brief Keypad read test
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return None
 */
void test_keypad_read(struct device *keypad)
{
	u8_t val;
	u8_t cnt = 0;
	s32_t rv;

	for (cnt = 0; cnt < KEYPAD_READ_TEST_KEYS; cnt++) {
		rv = keypad_read(keypad, &val);
		if (rv) {
			TC_ERROR("Function fail %d\n", rv);
			break;
		}

		TC_PRINT("Key: %c 0x%x\n", val, val);
	}
}

static s32_t set_keypad_pinmux(void)
{
	int ret;
	u8_t cnt;
	struct device *dev;

	dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	zassert_not_null(dev, "PIMUX Driver binding not found!");

	for (cnt = 0; cnt < (NUM_ROWS + NUM_COLS); cnt++) {
		/* GPIO function mode is
		 * 3 for mfio 0 - 36 and
		 * 0 for mfio 37 - 68
		 * And enable pull ups (0x20) for all rows and
		 * pull downs (0x40) for all columns
		 */
		ret = pinmux_pin_set(dev, mfios[cnt],
			(mfios[cnt] >= 37 ? PINMUX_FUNC_A : PINMUX_FUNC_D) +
			(cnt >= NUM_ROWS ? 0x40 : 0x20));
		zassert_true(ret == 0, "pinmux_pin_set() failed");
	}

	return ret;
}

s32_t test_keypad(s32_t argc, char *argv[])
{
	struct device *keypad;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	keypad = device_get_binding(CONFIG_KEYPAD_DEV_NAME);
	if (!keypad) {
		TC_ERROR("Cannot get keypad device\n");
		return TC_FAIL;
	}

#ifdef CONFIG_BOARD_CITADEL_SVK_58201
	configure_keypad_sel();
#endif
	set_keypad_pinmux();

	TC_PRINT("\nNote: Make sure the SPI is disabled to test KEYPAD\n");
	TC_PRINT("Both share the MFIO's\n");

	TC_PRINT("\nKeypad nonblock read test\n");
	TC_PRINT("\nActivate few keys in 5 seconds\n");
	k_sleep(5000);
	test_keypad_nonblock_read(keypad);

	TC_PRINT("\nKeypad read test\n");
	TC_PRINT("Try pressing some keys now!\n");
	TC_PRINT("Maximun %d keys\n", KEYPAD_READ_TEST_KEYS);
	test_keypad_read(keypad);

	return 0;
}
