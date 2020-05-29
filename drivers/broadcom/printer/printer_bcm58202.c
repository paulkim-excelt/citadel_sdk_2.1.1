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
 * @file printer_bcm58202.c
 * @brief bcm58202 Thermal printer driver implementation
 */

#include <arch/cpu.h>
#include <board.h>
#include <broadcom/gpio.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <printer.h>
#include <spi_legacy.h>
#include <zephyr/types.h>

/* FIXME: The GPIOs used for motor control and strobe pins are a function
 * of the board and Board specific settings should not be defined in the driver
 * sources. So we should either expose an api to set these pins or have Kconfig
 * flags defined so that they can be configured appropriately for different
 * boards without having to update the driver source code.
 */
#ifdef CONFIG_BOARD_CITADEL_SVK_58201
#define GPIO_0_STROBE_PIN1	(0)
#define GPIO_0_STROBE_PIN2	(1)
#define GPIO_0_STROBE_PIN3	(2)
#define GPIO_0_STROBE_PIN4	(3)

#define GPIO_0_ST_MOTOR_PIN1	(4)
#define GPIO_0_ST_MOTOR_PIN2	(5)
#define GPIO_0_ST_MOTOR_PIN3	(6)
#define GPIO_0_ST_MOTOR_PIN4	(7)

#define GPIO_1_LATCH_PIN	(47)
#elif defined CONFIG_BOARD_SERP_CP
#define GPIO_0_STROBE_PIN1	(32)
#define GPIO_0_STROBE_PIN2	(33)
#define GPIO_0_STROBE_PIN3	(5)
#define GPIO_0_STROBE_PIN4	(12)

#define GPIO_0_ST_MOTOR_PIN1	(3)
#define GPIO_0_ST_MOTOR_PIN2	(4)
#define GPIO_0_ST_MOTOR_PIN3	(11)
#define GPIO_0_ST_MOTOR_PIN4	(2)

#define GPIO_1_LATCH_PIN	(25)
#else
/* Add the appropriate GPIO pins used for strobe/moto coltrol/latch here,
 * to add driver support
 */
#error Printer driver not supported for board: CONFIG_BOARD
#endif

struct printer_data {
	struct device *gpio_dev[3];
	u8_t stm_phase;
};

/* Internal gpio configure function that takes gpio number 0 - 68 as argument */
#define PINS_PER_GPIO_DEV	32
static int gpio_pin_configure_int(
	const struct printer_data *priv, u32_t pin, int flags)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_configure(gpio_dev, pin % PINS_PER_GPIO_DEV, flags);
}

/* Internal gpio write function that takes gpio number 0 - 68 as argument */
static int gpio_pin_write_int(
	const struct printer_data *priv, u32_t pin, u32_t value)
{
	struct device *gpio_dev = priv->gpio_dev[pin / PINS_PER_GPIO_DEV];

	return gpio_pin_write(gpio_dev, pin % PINS_PER_GPIO_DEV, value);
}

/**
 * @brief Disable printer stepper motor
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 for success, error otherwise
 */
static s32_t bcm5820x_stepper_motor_disable(struct device *dev)
{
	const struct printer_data *priv = dev->driver_data;

	gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 0);
	gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 0);
	gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 0);
	gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 0);

	return 0;
}

static s32_t bcm5820x_strobe_assert(struct device *dev)
{
	struct printer_data *priv = dev->driver_data;

	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN1, 0);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN2, 0);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN3, 0);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN4, 0);
	k_busy_wait(10);

	return 0;
}

static s32_t bcm5820x_strobe_deassert(struct device *dev)
{
	struct printer_data *priv = dev->driver_data;

	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN1, 1);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN2, 1);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN3, 1);
	gpio_pin_write_int(priv, GPIO_0_STROBE_PIN4, 1);
	k_busy_wait(10);

	return 0;
}

static s32_t bcm5820x_latch_assert(struct device *dev)
{
	struct printer_data *priv = dev->driver_data;

	gpio_pin_write_int(priv, GPIO_1_LATCH_PIN, 0);
	k_busy_wait(10);

	return 0;
}

static s32_t bcm5820x_latch_deassert(struct device *dev)
{
	struct printer_data *priv = dev->driver_data;

	gpio_pin_write_int(priv, GPIO_1_LATCH_PIN, 1);
	k_busy_wait(10);

	return 0;
}

/**
 * @brief Set stepper motor phase
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param phase Phase to set
 *
 * @retval 0 for success, error otherwise
 */
s32_t bcm5820x_stepper_motor_control(struct device *dev, uint8_t phase)
{
	struct printer_data *priv = dev->driver_data;

       /* Driven as per the timing diagram in SS205-V4-LV technical manual */
	switch (phase) {
	case 0:
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 1);
		break;

	case 1:
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 1);
		break;

	case 2:
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 0);
		break;

	case 3:
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 0);
		break;

	default:
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN1, 1);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN2, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN3, 0);
		gpio_pin_write_int(priv, GPIO_0_ST_MOTOR_PIN4, 1);
		break;
	}
	priv->stm_phase = phase;

	/* Need to wait some time otherwise it will print failed. */
	k_busy_wait(2000);

	return 0;
}

/**
 * @brief Run the stepper motor for given steps
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param steps No of steps
 *
 * @retval 0 for success, error otherwise
 */
static s32_t bcm5820x_stepper_motor_run(struct device *dev, u8_t steps)
{
	struct printer_data *priv = dev->driver_data;
	u8_t step;

	for (step = 0; step < steps; step++) {
		if (priv->stm_phase >= 3)
			priv->stm_phase = 0;
		else
			priv->stm_phase++;

		bcm5820x_stepper_motor_control(dev, priv->stm_phase);
	}

	return 0;
}

/**
 * @brief Send dot line data to printer to print
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param spi Pointer to the spi device structure
 * @param data Pointer to the dot line data
 * @param length length in bytes
 *
 * @retval 0 for success, error otherwise
 */
static s32_t bcm5820x_line_data_print(struct device *dev, struct device *spi,
				      u8_t *data, u8_t length)
{
	s32_t rv;

	bcm5820x_latch_assert(dev);
	bcm5820x_strobe_deassert(dev);

	rv = spi_transceive(spi, data, length, NULL, 0);
	if (rv)
		return -EINVAL;

	bcm5820x_strobe_deassert(dev);
	k_busy_wait(1);
	bcm5820x_latch_assert(dev);
	k_busy_wait(1);
	bcm5820x_latch_deassert(dev);
	k_busy_wait(1);
	bcm5820x_strobe_assert(dev);
	k_busy_wait(10);

	return 0;
}

static const struct printer_driver_api printer_funcs = {
	.line_data_print = bcm5820x_line_data_print,
	.stepper_motor_control = bcm5820x_stepper_motor_control,
	.stepper_motor_run = bcm5820x_stepper_motor_run,
	.stepper_motor_disable = bcm5820x_stepper_motor_disable,
};

static struct printer_data printer_priv;

/**
 * @brief Initialyze printer pins
 *
 * @param dev Device struct
 *
 * @return 0 for success, error otherwise
 */
static s32_t printer_bcm58202_init(struct device *dev)
{
	s32_t rv;
	struct printer_data *priv = dev->driver_data;

	priv->stm_phase = 0;
	priv->gpio_dev[0] = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
	if (!priv->gpio_dev[0])
		return -EINVAL;

	priv->gpio_dev[1] = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	if (!priv->gpio_dev[1])
		return -EINVAL;

	priv->gpio_dev[2] = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
	if (!priv->gpio_dev[2])
		return -EINVAL;

	rv = gpio_pin_configure_int(priv, GPIO_0_ST_MOTOR_PIN1,
							GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_ST_MOTOR_PIN2,
							GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_ST_MOTOR_PIN3,
							GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_ST_MOTOR_PIN4,
							GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;

	/* Strobe signals */
	rv = gpio_pin_configure_int(priv, GPIO_0_STROBE_PIN1, GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_STROBE_PIN2, GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_STROBE_PIN3, GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;
	rv = gpio_pin_configure_int(priv, GPIO_0_STROBE_PIN4, GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;

	/* Latch signal */
	rv = gpio_pin_configure_int(priv, GPIO_1_LATCH_PIN, GPIO_DIR_OUT);
	if (rv)
		return -EINVAL;

	return 0;
}

DEVICE_AND_API_INIT(printer, CONFIG_PRINTER_DEV_NAME, printer_bcm58202_init,
		    &printer_priv, NULL, POST_KERNEL,
		    CONFIG_PRINTER_DRIVER_INIT_PRIORITY, &printer_funcs);
