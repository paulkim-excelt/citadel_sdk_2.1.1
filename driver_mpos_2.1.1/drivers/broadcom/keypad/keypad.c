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
 * @file keypad.c
 * @brief keypad driver
 */

#include <arch/cpu.h>
#include <board.h>
#include <broadcom/gpio.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <keypad.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <stdbool.h>
#include <string.h>
#include <sys_clock.h>
#include <zephyr/types.h>

#define KEYPAD_NO_OF_ROWS		(4)
#define KEYPAD_NO_OF_COLUMNS		(4)

/* FIXME: The GPIOs used for motor control and strobe pins are a function
 * of the board and Board specific settings should not be defined in the driver
 * sources. So we should either expose an api to set these pins or have Kconfig
 * flags defined so that they can be configured appropriately for different
 * boards without having to update the driver source code.
 */
#ifdef CONFIG_BOARD_CITADEL_SVK_58201
u8_t row_gpio_t[KEYPAD_NO_OF_ROWS] = {24, 25, 26, 27};
u8_t column_gpio_t[KEYPAD_NO_OF_COLUMNS] = {28, 29, 30, 31};
#elif defined CONFIG_BOARD_SERP_CP
u8_t row_gpio_t[KEYPAD_NO_OF_ROWS] = {44, 45, 60, 61};
u8_t column_gpio_t[KEYPAD_NO_OF_COLUMNS] = {7, 13, 14, 1};
#else
/* Add the appropriate GPIO pins used for row and columns here, to add driver
 * support
 */
#error Keypad driver not supported for board: CONFIG_BOARD
#endif
u8_t key_mapping_t[KEYPAD_NO_OF_ROWS][KEYPAD_NO_OF_COLUMNS] = {
	{'D', 'C', 'B', 'A'},
	{'#', '9', '6', '3'},
	{'0', '8', '5', '2'},
	{'*', '7', '4', '1'}
};

static struct keypad_data data;

#define KEYS_BUFFER_LENGTH		(8)
#define KEY_DEBOUNCE_WAIT_TIME		(100)

enum key_state {
	KEY_STATE_NONE,
	KEY_STATE_PRESS,
	KEY_STATE_RELEASE
};

struct keypad_data {
	u8_t key_mapping[KEYPAD_NO_OF_ROWS][KEYPAD_NO_OF_COLUMNS];
	u8_t column_gpio[KEYPAD_NO_OF_COLUMNS];
	u8_t row_gpio[KEYPAD_NO_OF_ROWS];
	u8_t keys_buf[KEYS_BUFFER_LENGTH];
	keypad_callback cb;
	struct gpio_callback gpio_cb[3];
	struct device *gpio_dev[3];
	u32_t key_change_time;
	enum key_state key_status;
	u8_t no_of_keys;
	u8_t columns;
	u8_t rows;
};


/* Internal gpio configure function that takes gpio number 0 - 68 as argument */
#define PINS_PER_GPIO_DEV	32
static int gpio_pin_configure_int(
	const struct keypad_data *priv, u32_t pin, int flags)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_configure(gpio_dev, pin % PINS_PER_GPIO_DEV, flags);
}

/* Internal gpio write function that takes gpio number 0 - 68 as argument */
static int gpio_pin_write_int(
	const struct keypad_data *priv, u32_t pin, u32_t value)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_write(gpio_dev, pin % PINS_PER_GPIO_DEV, value);
}

/* Internal gpio read function that takes gpio number 0 - 68 as argument */
static int gpio_pin_read_int(
	const struct keypad_data *priv, u32_t pin, u32_t *value)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_read(gpio_dev, pin % PINS_PER_GPIO_DEV, value);
}

/* Internal gpio enable callback function that takes gpio number 0 - 68 as
 * argument
 */
static int gpio_pin_enable_callback_int(
	const struct keypad_data *priv, u32_t pin)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_enable_callback(gpio_dev, pin % PINS_PER_GPIO_DEV);
}

/* Internal gpio disable callback function that takes gpio number 0 - 68 as
 * argument
 */
static int gpio_pin_disable_callback_int(
	const struct keypad_data *priv, u32_t pin)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_disable_callback(gpio_dev, pin % PINS_PER_GPIO_DEV);
}

/**
 * @brief Key interrupt
 *
 * @param "struct device *dev" Device struct for the GPIO device.
 * @param "struct gpio_callback *cb" Original struct gpio_callback
 *        owning this handler
 * @param "u32_t pins" Mask of pins that triggers the callback handler
 */
static void key_int(struct device *dev, struct gpio_callback *cb, u32_t pins)
{
	struct keypad_data *priv = &data;

	u32_t cnt;
	u32_t row, column;
	u32_t val;
	u32_t key;

	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/*
	 * Avoid multiple interrupts in key debounce time.
	 * We may get multiple pulses detected at initial contact/release
	 * with single key press/release.
	 */
	if ((k_uptime_get_32() - priv->key_change_time) <
					KEY_DEBOUNCE_WAIT_TIME)
		return;

	/* Disable callback and scan the columns with making single row high */
	for (column = 0; column < priv->columns; column++)
		gpio_pin_disable_callback_int(priv, priv->column_gpio[column]);

	for (row = 0; row < priv->rows; row++)
		gpio_pin_write_int(priv, priv->row_gpio[row], 0);

	/* Scan columns for key detection */
	key = 0;
	for (row = 0; row < priv->rows; row++) {
		gpio_pin_write_int(priv, priv->row_gpio[row], 1);
		for (column = 0; column < priv->columns; column++) {
			gpio_pin_read_int(priv, priv->column_gpio[column],
					 &val);
			if (val) {
				key = priv->key_mapping[row][column];
				priv->key_change_time = k_uptime_get_32();
				break;
			}
		}
		gpio_pin_write_int(priv, priv->row_gpio[row], 0);
	}

	if (key == 0) {
		if (priv->key_status == KEY_STATE_PRESS) {
			/* Key released */
			priv->key_status = KEY_STATE_RELEASE;
			priv->key_change_time = k_uptime_get_32();
		}
	} else {
		/* Key activated */
		priv->key_status = KEY_STATE_PRESS;
		if (priv->no_of_keys < KEYS_BUFFER_LENGTH) {
			priv->keys_buf[priv->no_of_keys] = key;
			priv->no_of_keys++;
		} else {
			for (cnt = 0; cnt < (KEYS_BUFFER_LENGTH - 1); cnt++)
				priv->keys_buf[cnt] = priv->keys_buf[cnt + 1];

			priv->keys_buf[priv->no_of_keys - 1] = key;
		}

		if (priv->cb)
			priv->cb();
	}

	/* Make rows high as default */
	for (row = 0; row < priv->rows; row++)
		gpio_pin_write_int(priv, priv->row_gpio[row], 1);

	for (column = 0; column < priv->columns; column++)
		gpio_pin_enable_callback_int(priv, priv->column_gpio[column]);
}

/**
 * @brief Keypad init
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 for success, error otherwise
 */
static s32_t keypad_init(struct device *dev)
{
	struct keypad_data *priv = dev->driver_data;
	u32_t pin_mask[3];
	u32_t cnt, i;
	s32_t rv;

	priv->rows = KEYPAD_NO_OF_ROWS;
	priv->columns = KEYPAD_NO_OF_COLUMNS;

	memcpy(priv->row_gpio, row_gpio_t, priv->rows);
	memcpy(priv->column_gpio, column_gpio_t, priv->columns);
	memcpy(priv->key_mapping, key_mapping_t, priv->rows * priv->columns);

	priv->cb = NULL;
	priv->no_of_keys = 0;
	priv->key_change_time = 0;
	priv->key_status = KEY_STATE_NONE;

	priv->gpio_dev[0] = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
	if (!priv->gpio_dev[0])
		return -ENODEV;

	priv->gpio_dev[1] = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	if (!priv->gpio_dev[1])
		return -ENODEV;

	priv->gpio_dev[2] = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
	if (!priv->gpio_dev[2])
		return -ENODEV;

	/* Configure rows */
	for (cnt = 0; cnt < priv->rows; cnt++) {
		rv = gpio_pin_configure_int(priv, priv->row_gpio[cnt],
					GPIO_DIR_OUT);
		if (rv)
			return rv;
		rv = gpio_pin_write_int(priv, priv->row_gpio[cnt], 1);
		if (rv)
			return rv;
	}

	/* Configure columns */
	memset(pin_mask, 0, sizeof(pin_mask));
	for (cnt = 0; cnt < priv->columns; cnt++) {
		rv = gpio_pin_configure_int(priv, priv->column_gpio[cnt],
				GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				 GPIO_INT_DOUBLE_EDGE);
		if (rv)
			return rv;

		/* Set the pinmask appropriately for each gpio device */
		pin_mask[priv->column_gpio[cnt] / PINS_PER_GPIO_DEV] |=
			BIT(priv->column_gpio[cnt] % PINS_PER_GPIO_DEV);
	}

	/* Enable interrupts on columns */
	for (i = 0; i < 3; i++) {
		if (pin_mask[i] == 0)
			continue;
		gpio_init_callback(&priv->gpio_cb[i], key_int, pin_mask[i]);
		rv = gpio_add_callback(priv->gpio_dev[i], &priv->gpio_cb[i]);
		if (rv)
			return rv;
	}

	return 0;
}

/**
 * @brief Keypad key read
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param key - pointer to key
 *
 * @return 0
 */
static s32_t key_driver_read(struct device *dev, u8_t *key)
{
	struct keypad_data *priv = dev->driver_data;
	u32_t cnt;

	while (priv->no_of_keys == 0) {
		k_yield();
	}

	*key = priv->keys_buf[0];
	priv->no_of_keys--;
	for (cnt = 0; cnt < priv->no_of_keys; cnt++)
		priv->keys_buf[cnt] = priv->keys_buf[cnt + 1];

	return 0;
}

/**
 * @brief Keypad key read nonblock
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param key - pointer to key
 *
 * @return Key count from buffer
 */
static s32_t key_driver_read_nonblock(struct device *dev, u8_t *buf, u8_t len)
{
	struct keypad_data *priv = dev->driver_data;
	u32_t ret_len;
	u32_t cnt;

	if (priv->no_of_keys == 0)
		return 0;

	if (priv->no_of_keys >= len)
		ret_len = len;
	else
		ret_len = priv->no_of_keys;

	for (cnt = 0; cnt < ret_len; cnt++)
		key_driver_read(dev, &buf[cnt]);

	return ret_len;
}

/**
 * @brief Keypad callback register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb - callback function
 *
 * @return 0
 */
static s32_t key_driver_callback_register(struct device *dev,
					    keypad_callback cb)
{
	struct keypad_data *priv = dev->driver_data;

	priv->cb = cb;

	return 0;
}

/**
 * @brief Keypad callback unregister
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0
 */
static s32_t key_driver_callback_unregister(struct device *dev)
{
	struct keypad_data *priv = dev->driver_data;

	priv->cb = NULL;

	return 0;
}

static const struct keypad_driver_api keypad_api = {
	.read = key_driver_read,
	.read_nonblock = key_driver_read_nonblock,
	.callback_register = key_driver_callback_register,
	.callback_unregister = key_driver_callback_unregister,
};

DEVICE_AND_API_INIT(keypad, CONFIG_KEYPAD_DEV_NAME, keypad_init,
		    &data, NULL, POST_KERNEL,
		    CONFIG_KEYPAD_DRIVER_INIT_PRIORITY, &keypad_api);
