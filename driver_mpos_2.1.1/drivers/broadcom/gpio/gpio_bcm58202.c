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
 * @file gpio_bcm58202.c
 * @brief bcm58202 gpio driver implementation
 */

#include <board.h>
#include <device.h>
#include <errno.h>
#include <broadcom/gpio.h>
#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <limits.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <pm.h>
#include <sys_io.h>
#include <dmu.h>

#include "gpio_utils.h"

#define GPIO_DATA_IN			(0x00)
#define GPIO_DATA_OUT			(0x04)
#define GPIO_OUT_EN			(0x08)
#define GPIO_INT_TYPE			(0x0c)
#define GPIO_INT_DE			(0x10)
#define GPIO_INT_EDGE_TYPE		(0x14)
#define GPIO_INT_MSK			(0x18)
#define GPIO_INT_STAT			(0x1c)
#define GPIO_INT_MSTAT			(0x20)
#define GPIO_INT_CLR			(0x24)
#define GPIO_AUX_SEL			(0x28)
#define GPIO_SCR			(0x2c)
#define GPIO_INIT_VAL			(0x30)

#define GP_PAD_RES			(0x34)
#define GP_RES_EN			(0x38)

#define MFIO_HYS_EN			(1 << 10)
#define MFIO_DRIVE_SHIFT		(7)
#define MFIO_DRIVE_STRENGT_MASK		(0x07)
#define MFIO_DRIVE_STRENGT_2		(0x00)
#define MFIO_DRIVE_STRENGT_4		(0x04)
#define MFIO_DRIVE_STRENGT_6		(0x02)
#define MFIO_DRIVE_STRENGT_8		(0x06)
#define MFIO_DRIVE_STRENGT_10		(0x01)
#define MFIO_DRIVE_STRENGT_12		(0x05)
#define MFIO_DRIVE_STRENGT_14		(0x03)
#define MFIO_DRIVE_STRENGT_16		(0x07)
#define MFIO_PULL_DOWN			(1 << 6)
#define MFIO_PULL_UP			(1 << 5)
#define MFIO_SLEWR			(1 << 4)
#define MFIO_IND			(1 << 3)
#define MFIO_SELECT_MASK		(0x07)

#define AON_GPIO_CONTROL1		(0x00)
#define AON_GPIO_CONTROL2		(0x04)
#define AON_GPIO_CONTROL3		(0x08)
#define AON_GPIO_CONTROL4		(0x0c)
#define AON_GPIO_CONTROL5		(0x10)
#define AON_GPIO_CONTROL6		(0x14)

/* 2 ma */
#define AON_DEFAULT_DRIVE_STRENGTH	(0x00)
/* 8 ma */
#define AON_HIGH_DRIVE_STRENGTH		(0x03)

#define DRIVE_STRENGTH_BIT_2		(0x02)
#define DRIVE_STRENGTH_BIT_1		(0x01)
#define DRIVE_STRENGTH_BIT_0		(0x00)

#define EXCEPTION_GPIO			SPI_IRQ(GPIO_IRQ)
#define CONFIG_GPIO_IRQ_PRI		(0x00)
#define GPIO_BIT_SET			(0x01)

#define IOSYS_GPIO_COUNT		(5)

#define IOSYS_GPIO_IN_DATA_IN		BIT(5)
#define IOSYS_GPIO_IN_HYS_EN		BIT(4)
#define IOSYS_GPIO_IN_PULL_UP		BIT(3)
#define IOSYS_GPIO_IN_PULL_DOWN		BIT(2)
#define IOSYS_GPIO_IN_SLEWR		BIT(1)
#define IOSYS_GPIO_IN_IND		BIT(0)

#define IOSYS_GPIO_OUT_DATA_OUT		BIT(6)
#define IOSYS_GPIO_OUT_DRIVE0		BIT(5)
#define IOSYS_GPIO_OUT_DRIVE1		BIT(4)
#define IOSYS_GPIO_OUT_DRIVE2		BIT(3)
#define IOSYS_GPIO_OUT_PULL_DOWN	BIT(2)
#define IOSYS_GPIO_OUT_PULL_UP		BIT(1)
#define IOSYS_GPIO_OUT_DATA_OEB		BIT(0)

#define IOSYS_GPIO_DRIVE_SHIFT		(3)

#define MCU_AON_GPIO_INTERRUPT		BIT(2)

/*
 * IOSYS gpio pins are pre-configured to either input pin or output pin.
 * These pins can be configured for both 58201 and 58202 devices. In 58201
 * device, these pins are used for NFC control purpose.
 */
static bool iosys_gpio_input_pin[IOSYS_GPIO_COUNT] = {
	true, false, false, true, true};

static s32_t gpio_bcm58202_init(struct device *dev);

static s32_t gpio_aon_bcm58202_init(struct device *dev);

/**
 * @brief GPIO block configuration structure
 *
 * base is the base address for the gpio block
 * pin_ctrl_base is the base address of pin control
 * ngpio is the number of supported GPIOs
 */
struct gpio_bcm58202_cfg {
	u32_t base;
	u32_t pin_ctrl_base;
	u32_t ngpio;
};

/**
 * @brief GPIO private data structure
 *
 * cb is the sys_slist_t structure
 */
struct gpio_bcm58202_data {
	sys_slist_t cb;
};

static struct gpio_bcm58202_data gpio_data_0_31;
static struct gpio_bcm58202_data gpio_data_32_63;
static struct gpio_bcm58202_data gpio_data_64_68;
static struct gpio_bcm58202_data gpio_aon_data;
static struct gpio_bcm58202_data gpio_iosys_data;

/**
 * @brief Configure the pin physical parameters
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return None
 */
static void _gpio_pin_control(struct device *dev, u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->pin_ctrl_base;
	u32_t mfio_data;
	u32_t data = 0;

	/* set pull up/pull down */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP)
		data |= MFIO_PULL_UP;
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN)
		data |= MFIO_PULL_DOWN;

	/* set slew rate */
	if (flags & GPIO_SLEW_RATE_ENABLE)
		data |= MFIO_SLEWR;

	/* set hysteresis */
	if (flags & GPIO_HYSTERESIS_ENABLE)
		data |= MFIO_HYS_EN;

	/* set input disable */
	if (flags & GPIO_PIN_DISABLE)
		data |= MFIO_IND;

	/* Set the drive strength default - 2 ma, ALT - 8 ma */
	if ((flags & GPIO_DS_HIGH_MASK) == GPIO_DS_ALT_HIGH)
		data |= MFIO_DRIVE_STRENGT_8 << MFIO_DRIVE_SHIFT;
	else
		data |= MFIO_DRIVE_STRENGT_2 << MFIO_DRIVE_SHIFT;

	mfio_data = sys_read32(base + pin * 4);
	mfio_data &= MFIO_SELECT_MASK;
	mfio_data |= data;
	sys_write32(mfio_data, (base + pin * 4));
}

/**
 * @brief Get the pin physical parameters
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Pointer to flags
 *
 * @return None
 */
static void _gpio_pin_control_get(struct device *dev, u32_t pin, u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->pin_ctrl_base;
	u32_t data = 0;
	u32_t fl = 0;

	data = sys_read32(base + (pin * 4));
	if (data & MFIO_PULL_UP)
		fl |= GPIO_PUD_PULL_UP;

	if (data & MFIO_PULL_DOWN)
		fl |= GPIO_PUD_PULL_DOWN;

	if (data & MFIO_SLEWR)
		fl |= GPIO_SLEW_RATE_ENABLE;

	if (data & MFIO_HYS_EN)
		fl |= GPIO_HYSTERESIS_ENABLE;

	if (data & MFIO_IND)
		fl |= GPIO_PIN_DISABLE;

	if (((data >> MFIO_DRIVE_SHIFT) & MFIO_DRIVE_STRENGT_MASK) ==
						 MFIO_DRIVE_STRENGT_8)
		fl |= GPIO_DS_ALT_HIGH;
	else
		fl |= GPIO_DS_DFLT_HIGH;

	*flags |= fl;
}

/**
 * @brief Configure the pin physical parameters of AON device
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return None
 */
static void _gpio_aon_pin_control(struct device *dev, u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t drive_strength = AON_DEFAULT_DRIVE_STRENGTH;
	u32_t base = cfg->pin_ctrl_base;
	u32_t base1 = cfg->base;

	/* set pull up/pull down */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_NORMAL) {
		sys_clear_bit((base1 + GP_RES_EN), pin);
	} else {
		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP)
			sys_set_bit((base1 + GP_PAD_RES), pin);
		else
			sys_clear_bit((base1 + GP_PAD_RES), pin);
		sys_set_bit((base1 + GP_RES_EN), pin);
	}

	/* set slew rate */
	if (flags & GPIO_SLEW_RATE_ENABLE)
		sys_set_bit((base + AON_GPIO_CONTROL6), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL6), pin);

	/* set hysteresis */
	if (flags & GPIO_HYSTERESIS_ENABLE)
		sys_set_bit((base + AON_GPIO_CONTROL5), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL5), pin);

	/* set input disable */
	if (flags & GPIO_PIN_DISABLE)
		sys_set_bit((base + AON_GPIO_CONTROL4), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL4), pin);

	/* Set the drive strength default - 2 ma, ALT - 8 ma */
	if ((flags & GPIO_DS_HIGH_MASK) == GPIO_DS_ALT_HIGH)
		drive_strength = AON_HIGH_DRIVE_STRENGTH;

	if (drive_strength & BIT(DRIVE_STRENGTH_BIT_2))
		sys_set_bit((base + AON_GPIO_CONTROL1), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL1), pin);

	if (drive_strength & BIT(DRIVE_STRENGTH_BIT_1))
		sys_set_bit((base + AON_GPIO_CONTROL2), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL2), pin);

	if (drive_strength & BIT(DRIVE_STRENGTH_BIT_0))
		sys_set_bit((base + AON_GPIO_CONTROL3), pin);
	else
		sys_clear_bit((base + AON_GPIO_CONTROL3), pin);
}

/**
 * @brief Get the pin physical parameters of AON device
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Pointer to flags
 *
 * @return None
 */
static void _gpio_aon_pin_control_get(struct device *dev, u32_t pin,
				      u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t drive_strength = 0;
	u32_t base = cfg->pin_ctrl_base;
	u32_t base1 = cfg->base;
	u32_t fl = 0;

	/* get pull up/pull down */
	if (sys_test_bit((base1 + GP_RES_EN), pin)) {
		if (sys_test_bit((base1 + GP_PAD_RES), pin))
			fl |= GPIO_PUD_PULL_UP;
		else
			fl |= GPIO_PUD_PULL_DOWN;
	} else {
		fl |= GPIO_PUD_NORMAL;
	}

	/* get slew rate */
	if (sys_test_bit((base + AON_GPIO_CONTROL6), pin))
		fl |= GPIO_SLEW_RATE_ENABLE;

	/* get hysteresis */
	if (sys_test_bit((base + AON_GPIO_CONTROL5), pin))
		fl |= GPIO_HYSTERESIS_ENABLE;

	/* get input disable */
	if (sys_test_bit((base + AON_GPIO_CONTROL4), pin))
		fl |= GPIO_PIN_DISABLE;

	/* Get the drive strength default - 2 ma, ALT - 8 ma */
	if (sys_test_bit((base + AON_GPIO_CONTROL1), pin))
		drive_strength |= BIT(DRIVE_STRENGTH_BIT_2);

	if (sys_test_bit((base + AON_GPIO_CONTROL2), pin))
		drive_strength |= BIT(DRIVE_STRENGTH_BIT_1);

	if (sys_test_bit((base + AON_GPIO_CONTROL3), pin))
		drive_strength |= BIT(DRIVE_STRENGTH_BIT_0);

	if (drive_strength == AON_HIGH_DRIVE_STRENGTH)
		fl |= GPIO_DS_ALT_HIGH;
	else
		fl |= GPIO_DS_DFLT_HIGH;

	*flags |= fl;
}

/**
 * @brief Configure the pin parameters
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return None
 */
static void _gpio_pin_config(struct device *dev, u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	/* Set direction */
	if (flags & GPIO_DIR_OUT)
		sys_set_bit((base + GPIO_OUT_EN), pin);
	else
		sys_clear_bit((base + GPIO_OUT_EN), pin);

	/* Set interrupt type edge/level */
	if (flags & GPIO_INT_EDGE)
		sys_clear_bit((base + GPIO_INT_TYPE), pin);
	else
		sys_set_bit((base + GPIO_INT_TYPE), pin);

	/* Set interrupt edge type */
	if (flags & GPIO_INT_ACTIVE_HIGH)
		sys_set_bit((base + GPIO_INT_EDGE_TYPE), pin);
	else
		sys_clear_bit((base + GPIO_INT_EDGE_TYPE), pin);

	/* Set interrupt duel edge */
	if (flags & GPIO_INT_DOUBLE_EDGE)
		sys_set_bit((base + GPIO_INT_DE), pin);
	else
		sys_clear_bit((base + GPIO_INT_DE), pin);

	/* Set interrupt */
	if (flags & GPIO_INT)
		sys_set_bit((base + GPIO_INT_MSK), pin);
	else
		sys_clear_bit((base + GPIO_INT_MSK), pin);
}

/**
 * @brief Get the pin parameters
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return None
 */
static void _gpio_pin_config_get(struct device *dev, u32_t pin, u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;
	u32_t fl = 0;

	if (sys_test_bit((base + GPIO_OUT_EN), pin))
		fl |= GPIO_DIR_OUT;

	if (!(sys_test_bit((base + GPIO_INT_TYPE), pin)))
		fl |= GPIO_INT_EDGE;

	if (sys_test_bit((base + GPIO_INT_EDGE_TYPE), pin))
		fl |= GPIO_INT_ACTIVE_HIGH;

	if (sys_test_bit((base + GPIO_INT_DE), pin))
		fl |= GPIO_INT_DOUBLE_EDGE;

	if (sys_test_bit((base + GPIO_INT_MSK), pin))
		fl |= GPIO_INT;

	*flags |= fl;
}

/**
 * @brief Configure the pin parameters for gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_config(struct device *dev, s32_t access_op,
				  u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t gpio;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;

		_gpio_pin_config(dev, pin, flags);
		_gpio_pin_control(dev, pin, flags);
	} else {
		for (gpio = 0; gpio < cfg->ngpio; gpio++) {
			_gpio_pin_config(dev, gpio, flags);
			_gpio_pin_control(dev, gpio, flags);
		}
	}

	return 0;
}

/**
 * @brief Get configuration parameters for gpio
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Pointer to flags
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_config_get(struct device *dev, u32_t pin,
				      u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	if ((pin >= cfg->ngpio) || (flags == NULL))
		return -EINVAL;
	*flags = 0;
	_gpio_pin_config_get(dev, pin, flags);
	_gpio_pin_control_get(dev, pin, flags);

	return 0;
}

/**
 * @brief Configure the pin parameters for AON gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_aon_bcm58202_config(struct device *dev, s32_t access_op,
				      u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t gpio;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;

		_gpio_pin_config(dev, pin, flags);
		_gpio_aon_pin_control(dev, pin, flags);
	} else {
		for (gpio = 0; gpio < cfg->ngpio; gpio++) {
			_gpio_pin_config(dev, gpio, flags);
			_gpio_aon_pin_control(dev, gpio, flags);
		}
	}

	return 0;
}

/**
 * @brief Get configuration parameters for aon gpio
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Pointer to flags
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_aon_bcm58202_config_get(struct device *dev, u32_t pin,
					  u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	if ((pin >= cfg->ngpio) || (flags == NULL))
		return -EINVAL;
	*flags = 0;
	_gpio_pin_config_get(dev, pin, flags);
	_gpio_aon_pin_control_get(dev, pin, flags);

	return 0;
}

/**
 * @brief Write value to gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin number
 * @param value Value to write
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_write(struct device *dev, s32_t access_op,
				 u32_t pin, u32_t value)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;
		if (value & GPIO_BIT_SET)
			sys_set_bit((cfg->base + GPIO_DATA_OUT), pin);
		else
			sys_clear_bit((cfg->base + GPIO_DATA_OUT), pin);
	} else {
		sys_write32(value, (cfg->base + GPIO_DATA_OUT));
	}

	return 0;
}

/**
 * @brief Get the value of gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin number
 * @param value Pointer
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_read(struct device *dev, s32_t access_op,
				u32_t pin, u32_t *value)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;
	u32_t bit;

	if (value == NULL)
		return -EINVAL;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;
		bit = sys_test_bit((base + GPIO_DATA_IN), pin);
		if (bit)
			*value = 1;
		else
			*value = 0;
	} else {
		*value = sys_read32(base + GPIO_DATA_IN);
	}

	return 0;
}

/**
 * @brief Configure the pin parameters for iosys gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin GPIO number in the current device
 * @param flags Flags to configure
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_iosys_bcm58202_config(struct device *dev, s32_t access_op,
					u32_t pin, s32_t flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base, data;

	if ((access_op == GPIO_ACCESS_BY_PORT) || (pin >= cfg->ngpio))
		return -EINVAL;

	base = cfg->base + (pin * 4);
	data = sys_read32(base) & IOSYS_GPIO_OUT_DATA_OUT;

	if (iosys_gpio_input_pin[pin] == true) {
		if ((flags & GPIO_DIR_OUT) == GPIO_DIR_OUT)
			return -ENOTSUP;

		/* set pull up/pull down */
		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP)
			data |= IOSYS_GPIO_IN_PULL_UP;

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN)
			data |= IOSYS_GPIO_IN_PULL_DOWN;

		/* set slew rate */
		if (flags & GPIO_SLEW_RATE_ENABLE)
			data |= IOSYS_GPIO_IN_SLEWR;

		/* set hysteresis */
		if (flags & GPIO_HYSTERESIS_ENABLE)
			data |= IOSYS_GPIO_IN_HYS_EN;

		/* set input disable */
		if (flags & GPIO_PIN_DISABLE)
			data |= IOSYS_GPIO_IN_IND;
	} else {
		if (((flags & GPIO_DIR_OUT) != GPIO_DIR_OUT) ||
		    (flags & GPIO_PIN_DISABLE) ||
		    (flags & GPIO_HYSTERESIS_ENABLE) ||
		    (flags & GPIO_SLEW_RATE_ENABLE))
			return -ENOTSUP;

		/* Enable output */
		if (flags & GPIO_DIR_OUT)
			data |= IOSYS_GPIO_OUT_DATA_OEB;
		else
			data &= ~(IOSYS_GPIO_OUT_DATA_OEB);

		/* set pull up/pull down */
		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP)
			data |= IOSYS_GPIO_OUT_PULL_UP;

		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN)
			data |= IOSYS_GPIO_OUT_PULL_DOWN;

		/* Set the drive strength default - 2 ma, ALT - 8 ma */
		if ((flags & GPIO_DS_HIGH_MASK) == GPIO_DS_ALT_HIGH)
			data |= MFIO_DRIVE_STRENGT_8 << IOSYS_GPIO_DRIVE_SHIFT;
		else
			data |= MFIO_DRIVE_STRENGT_2 << IOSYS_GPIO_DRIVE_SHIFT;
	}

	sys_write32(data, base);
	return 0;
}

/**
 * @brief Get configuration parameters for iosys gpio
 *
 * @param dev Device struct
 * @param pin GPIO number in the current device
 * @param flags Pointer to flags
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_iosys_bcm58202_config_get(struct device *dev, u32_t pin,
					    u32_t *flags)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base, data;
	u32_t fl = 0;

	if ((pin >= cfg->ngpio) || (flags == NULL))
		return -EINVAL;

	base = cfg->base + (pin * 4);

	if (iosys_gpio_input_pin[pin] == true) {
		/* get pull up/pull down */
		if (sys_test_bit(base, IOSYS_GPIO_IN_PULL_UP))
			fl |= GPIO_PUD_PULL_UP;

		if (sys_test_bit(base, IOSYS_GPIO_IN_PULL_DOWN))
			fl |= GPIO_PUD_PULL_DOWN;

		/* get slew rate */
		if (sys_test_bit(base, IOSYS_GPIO_IN_SLEWR))
			fl |= GPIO_SLEW_RATE_ENABLE;

		/* get hysteresis */
		if (sys_test_bit(base, IOSYS_GPIO_IN_HYS_EN))
			fl |= GPIO_HYSTERESIS_ENABLE;

		/* get input disable */
		if (sys_test_bit(base, IOSYS_GPIO_IN_IND))
			fl |= GPIO_PIN_DISABLE;
	} else {
		/* get enable output */
		if (sys_test_bit(base, IOSYS_GPIO_OUT_DATA_OEB))
			fl |= GPIO_DIR_OUT;

		/* get pull up/pull down */
		if (sys_test_bit(base, IOSYS_GPIO_OUT_PULL_UP))
			fl |= GPIO_PUD_PULL_UP;

		if (sys_test_bit(base, IOSYS_GPIO_OUT_PULL_DOWN))
			fl |= GPIO_PUD_PULL_DOWN;

		/* Get the drive strength default - 2 ma, ALT - 8 ma */
		data = sys_read32(base);
		if (((data >> IOSYS_GPIO_DRIVE_SHIFT) &
			 MFIO_DRIVE_STRENGT_MASK) == MFIO_DRIVE_STRENGT_8)
			fl |= GPIO_DS_ALT_HIGH;
		else
			fl |= GPIO_DS_DFLT_HIGH;
	}

	*flags = fl;

	return 0;
}
/**
 * @brief Write value to iosys gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin number
 * @param value Value to write
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_iosys_bcm58202_write(struct device *dev, s32_t access_op,
				     u32_t pin, u32_t value)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	if ((iosys_gpio_input_pin[pin] == true) ||
	    (access_op == GPIO_ACCESS_BY_PORT) ||
	    (pin >= cfg->ngpio))
		return -EINVAL;

	if (value & GPIO_BIT_SET)
		sys_set_bit((cfg->base + (pin * 4)), IOSYS_GPIO_OUT_DATA_OUT);
	else
		sys_clear_bit((cfg->base + (pin * 4)), IOSYS_GPIO_OUT_DATA_OUT);

	return 0;
}

/**
 * @brief Get the value of gpio
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin number
 * @param value Pointer
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_iosys_bcm58202_read(struct device *dev, s32_t access_op,
				    u32_t pin, u32_t *value)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;
	u32_t bit;

	if ((iosys_gpio_input_pin[pin] == false) ||
	    (access_op == GPIO_ACCESS_BY_PORT) ||
	    (pin >= cfg->ngpio) || (value == NULL))
		return -EINVAL;

	bit = sys_test_bit((base + (pin * 4)), IOSYS_GPIO_IN_DATA_IN);
	if (bit)
		*value = 1;
	else
		*value = 0;

	return 0;
}

/**
 * @brief Manage call back
 *
 * @param dev Device struct
 * @param callback callback function
 * @param set Add or remove
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_manage_callback(struct device *dev,
					   struct gpio_callback *callback,
					   bool set)
{
	struct gpio_bcm58202_data *data = dev->driver_data;

	if (callback == NULL)
		return -EINVAL;

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

/**
 * @brief Call back enable
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin to enable
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_enable_callback(struct device *dev,
					   s32_t access_op, u32_t pin)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;
		sys_set_bit((base + GPIO_INT_MSK), pin);
	} else {
		for (pin = 0; pin < cfg->ngpio; pin++)
			sys_set_bit((base + GPIO_INT_MSK), pin);
	}

	return 0;
}

/**
 * @brief Call back disable
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin to disable
 *
 * @return 0 for success, error otherwise
 */
static s32_t gpio_bcm58202_disable_callback(struct device *dev,
					    s32_t access_op, u32_t pin)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= cfg->ngpio)
			return -EINVAL;
		sys_clear_bit((base + GPIO_INT_MSK), pin);
	} else {
		for (pin = 0; pin < cfg->ngpio; pin++)
			sys_clear_bit((base + GPIO_INT_MSK), pin);
	}

	return 0;
}

/**
 * @brief Manage call back
 *
 * @param dev Device struct
 * @param callback callback function
 * @param set Add or remove
 *
 * @return error not supported
 */
static s32_t manage_callback_not_supported(struct device *dev,
					   struct gpio_callback *callback,
					   bool set)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(set);

	return -ENOTSUP;
}

/**
 * @brief Get pending interrupt
 *
 * @param dev Device struct
 *
 * @return error not supported
 */
static u32_t get_pending_int_not_supported(struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

/**
 * @brief Call back not suppotred
 *
 * @param dev Device struct
 * @param access_op Port or Pin
 * @param pin Pin to enable
 *
 * @return error not supported
 */
static s32_t callback_not_supported(struct device *dev,
				    s32_t access_op, u32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return -ENOTSUP;
}

/**
 * @brief Initialization function of iosys gpio
 *
 * @param dev Device struct
 * @return 0
 */
static s32_t gpio_iosys_bcm58202_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief Interrupt service routine for device
 *
 * @param device Device structure
 *
 * @return None
 */
static void _gpio_bcm58202_isr(struct device *gpio)
{
	const struct gpio_bcm58202_cfg *cfg = gpio->config->config_info;
	struct gpio_bcm58202_data *data = gpio->driver_data;
	u32_t int_status;

	/* Get the interrupts and clear it */
	int_status = sys_read32(cfg->base + GPIO_INT_MSTAT);
	sys_write32(int_status, (cfg->base + GPIO_INT_CLR));
	_gpio_fire_callbacks(&data->cb, gpio, int_status);
}

/**
 * @brief Interrupt service routine
 *
 * @param arg arg
 *
 * @return None
 */
static void gpio_bcm58202_isr(void *arg)
{
	struct device *gpio;

	ARG_UNUSED(arg);
	gpio = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
	_gpio_bcm58202_isr(gpio);
	gpio = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	_gpio_bcm58202_isr(gpio);
	gpio = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
	_gpio_bcm58202_isr(gpio);
}

/**
 * @brief Get pending interrupts
 *
 * @param dev Device struct
 *
 * @return Pending interrupt status
 */
static u32_t gpio_bcm58202_get_pending_int(struct device *dev)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	return sys_read32(cfg->base + GPIO_INT_MSTAT);
}

/**
 * @brief Function to clr an single pending GPIO interrupt.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the pending interrupt is cleared.
 * @return 0 if successful, negative errno code on failure
 */
static u32_t gpio_bcm58202_clr_pending_int(struct device *dev, u32_t pin)
{
    const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

    if (pin >= cfg->ngpio)
		return 1;

	/* clear the pending interrupt */
	sys_write32((1<<pin), (cfg->base + GPIO_INT_CLR));

	return 0;
}

#ifdef CONFIG_AON_GPIO_INTERRUPT_SUPPORT
static void aon_gpio_run_irq(struct device *dev, u32_t int_status)
{
	struct device *gpio = device_get_binding(CONFIG_GPIO_AON_DEV_NAME);
	struct gpio_bcm58202_data *data = gpio->driver_data;

	ARG_UNUSED(dev);

	_gpio_fire_callbacks(&data->cb, gpio, int_status);
}
#endif

static const struct gpio_driver_api gpio_bcm58202_api = {
	.config = gpio_bcm58202_config,
	.config_get = gpio_bcm58202_config_get,
	.write = gpio_bcm58202_write,
	.read = gpio_bcm58202_read,
	.manage_callback = gpio_bcm58202_manage_callback,
	.enable_callback = gpio_bcm58202_enable_callback,
	.disable_callback = gpio_bcm58202_disable_callback,
	.get_pending_int = gpio_bcm58202_get_pending_int,
	.clr_pending_int = gpio_bcm58202_clr_pending_int,
};

static const struct gpio_driver_api gpio_aon_bcm58202_api = {
	.config = gpio_aon_bcm58202_config,
	.config_get = gpio_aon_bcm58202_config_get,
	.write = gpio_bcm58202_write,
	.read = gpio_bcm58202_read,
#ifdef CONFIG_AON_GPIO_INTERRUPT_SUPPORT
	.manage_callback = gpio_bcm58202_manage_callback,
	.enable_callback = gpio_bcm58202_enable_callback,
	.disable_callback = gpio_bcm58202_disable_callback,
	.get_pending_int = gpio_bcm58202_get_pending_int,
	.clr_pending_int = gpio_bcm58202_clr_pending_int,
#else
	.manage_callback = manage_callback_not_supported,
	.enable_callback = callback_not_supported,
	.disable_callback = callback_not_supported,
	.get_pending_int = get_pending_int_not_supported,
#endif
};

static const struct gpio_driver_api gpio_iosys_bcm58202_api = {
	.config = gpio_iosys_bcm58202_config,
	.config_get = gpio_iosys_bcm58202_config_get,
	.write = gpio_iosys_bcm58202_write,
	.read = gpio_iosys_bcm58202_read,
	.manage_callback = manage_callback_not_supported,
	.enable_callback = callback_not_supported,
	.disable_callback = callback_not_supported,
	.get_pending_int = get_pending_int_not_supported,
};

static const struct gpio_bcm58202_cfg gpio_bcm58202_cfg_0 = {
	.base = GPIO_0_31_BASE_ADDR,
	.pin_ctrl_base = MFIO_PIN_CTRL_BASE_ADDR_37,
	.ngpio = 32
};

static const struct gpio_bcm58202_cfg gpio_bcm58202_cfg_1 = {
	.base = GPIO_32_63_BASE_ADDR,
	.pin_ctrl_base = MFIO_PIN_CTRL_BASE_ADDR_0,
	.ngpio = 32
};

static const struct gpio_bcm58202_cfg gpio_bcm58202_cfg_2 = {
	.base = GPIO_64_68_BASE_ADDR,
	.pin_ctrl_base = MFIO_PIN_CTRL_BASE_ADDR_32,
	.ngpio = 5
};

static const struct gpio_bcm58202_cfg gpio_aon_bcm58202_cfg = {
	.base = GPIO_AON_BASE_ADDR,
	.pin_ctrl_base = GPIO_AON_PIN_CTRL_BASE_ADDR,
	.ngpio = 11
};

static const struct gpio_bcm58202_cfg gpio_iosys_bcm58202_cfg = {
	.base = GPIO_IOSYS_BASE_ADDR,
	.ngpio = IOSYS_GPIO_COUNT
};

DEVICE_AND_API_INIT(gpio_0, CONFIG_GPIO_DEV_NAME_0, gpio_bcm58202_init,
		    &gpio_data_0_31, &gpio_bcm58202_cfg_0, PRE_KERNEL_2,
		    CONFIG_GPIO_DRIVER_INIT_PRIORITY, &gpio_bcm58202_api);

DEVICE_AND_API_INIT(gpio_1, CONFIG_GPIO_DEV_NAME_1, gpio_bcm58202_init,
		    &gpio_data_32_63, &gpio_bcm58202_cfg_1, PRE_KERNEL_2,
		    CONFIG_GPIO_DRIVER_INIT_PRIORITY, &gpio_bcm58202_api);

DEVICE_AND_API_INIT(gpio_2, CONFIG_GPIO_DEV_NAME_2, gpio_bcm58202_init,
		    &gpio_data_64_68, &gpio_bcm58202_cfg_2, PRE_KERNEL_2,
		    CONFIG_GPIO_DRIVER_INIT_PRIORITY, &gpio_bcm58202_api);

DEVICE_AND_API_INIT(gpio_aon, CONFIG_GPIO_AON_DEV_NAME, gpio_aon_bcm58202_init,
		    &gpio_aon_data, &gpio_aon_bcm58202_cfg, PRE_KERNEL_2,
		    CONFIG_GPIO_DRIVER_INIT_PRIORITY, &gpio_aon_bcm58202_api);

DEVICE_AND_API_INIT(gpio_iosys, CONFIG_GPIO_IOSYS_DEV_NAME,
		    gpio_iosys_bcm58202_init, &gpio_iosys_data,
		    &gpio_iosys_bcm58202_cfg, PRE_KERNEL_2,
		    CONFIG_GPIO_DRIVER_INIT_PRIORITY, &gpio_iosys_bcm58202_api);

/**
 * @brief Initialze AON GPIO
 *
 * @param dev Device struct
 *
 * @return 0
 */
static s32_t gpio_aon_bcm58202_init(struct device *dev)
{
#ifdef CONFIG_AON_GPIO_INTERRUPT_SUPPORT
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	struct device *pm;
	u32_t data;

	pm = device_get_binding(CONFIG_PM_NAME);
	if (!pm)
		return -ENODEV;

	pm_set_aon_gpio_run_irq_callback(pm, aon_gpio_run_irq);
	/* clear the interrupts */
	sys_write32(UINT_MAX, (cfg->base + GPIO_INT_CLR));
	data = dmu_read(dmu, CIT_CRMU_MCU_INTR_MASK);
	data &= ~(MCU_AON_GPIO_INTERRUPT);
	dmu_write(dmu, CIT_CRMU_MCU_INTR_MASK, data);
#else
	ARG_UNUSED(dev);
#endif
	return 0;
}

/**
 * @brief Initialze GPIO
 *
 * @param dev Device struct
 *
 * @return 0
 */
static s32_t gpio_bcm58202_init(struct device *dev)
{
	const struct gpio_bcm58202_cfg *cfg = dev->config->config_info;

	/* clear the interrupts */
	sys_write32(UINT_MAX, (cfg->base + GPIO_INT_CLR));

	/* enable the interrupt */
	if (!irq_is_enabled(EXCEPTION_GPIO)) {
		IRQ_CONNECT(EXCEPTION_GPIO, CONFIG_GPIO_IRQ_PRI,
			    gpio_bcm58202_isr, NULL, 0);
		irq_enable(EXCEPTION_GPIO);
	}

	return 0;
}
