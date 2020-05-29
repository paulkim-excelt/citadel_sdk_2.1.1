/*
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 */

#include <kernel.h>
#include <device.h>
#include <errno.h>
#include <board.h>
#include <arch/cpu.h>
#include <pinmux.h>
#include <broadcom/i2c.h>
#include <broadcom/gpio.h>
#include <logging/sys_log.h>

#define IO_EXPANDER_ADDR	0x20
#define IO_EXPANDER_REG		0x4
#define IO_EXPANDER_REGA	0x0
#define IO_EXPANDER_REGB	0x1
#define IO_EXPANDER_REGC	0x2
#define IO_EXPANDER_REGD	0x3
#define IO_EXPANDER_REGE	0x4

#define IO_EXPANDER_PCA9538_ADDR 0x70

/* Configurable LCD modes*/
enum lcd_mode {
	SERIAL_SPI, /* serial mode (SPI) */
	PARALLEL_MODE8080, /* 8-bit parallel mode (8080) SERP_CP: unsupported */
};

static bool pinmux_configured;

/* For SPI mode, mux should be configured to select SPI1.
 * For Parallel mode, mux should be configured to select SMC (SRAM0)
 */
#define D_C_GPIO_SPI_LCD_MODE (53-32)
#define D_C_GPIO_PARALLEL_MODE 12
#define IOMUX_CTRL0_SEL_SRAM 1
#define IOMUX_CTRL0_SEL_GPIO53 3

static int iomux_set_lcd(struct device *pinmux_dev, int cur_mode)
{
	struct device *gpio_dev;
	int ret, i;
	u32_t val;
	s32_t pin;

	if (pinmux_configured)
		return 0;

	gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	if (gpio_dev == NULL) {
		SYS_LOG_ERR("Error getting gpio device");
		return -ENXIO;
	}

	if (cur_mode == SERIAL_SPI)
		pin = (s32_t) (D_C_GPIO_SPI_LCD_MODE);
	else
		pin = (s32_t) (D_C_GPIO_PARALLEL_MODE);

	ret = gpio_pin_write(gpio_dev, pin, 0);
	if (ret) {
		SYS_LOG_ERR("Failed to write to D/C GPIO\n");
		return ret;
	}

	ret = gpio_pin_configure(gpio_dev, pin, GPIO_DIR_OUT);
	if (ret) {
		SYS_LOG_ERR("Failed to write to D/C GPIO\n");
		return ret;
	}

	if (cur_mode == PARALLEL_MODE8080) {
		for (i = 0; i < 31; i++) {
			if (i == 1)
				continue;
			ret = pinmux_pin_set(pinmux_dev, (i + 38),
					     IOMUX_CTRL0_SEL_SRAM);
			if (ret) {
				SYS_LOG_ERR("Failed to set pinmux %d\n",
					    (i + 38));
				return ret;
			}
		}
	}

	/* Enable GPIO function for GPIO53 (MFIO 21) */
	pinmux_pin_get(pinmux_dev, 21, &val);

	ret = pinmux_pin_set(pinmux_dev, 21, IOMUX_CTRL0_SEL_GPIO53);
	if (ret) {
		SYS_LOG_ERR("Failed to set pinmux to enable GPIO53: %d\n",
			    ret);
		return ret;
	}

	pinmux_configured = true;

	return 0;
}

int lcd_board_configure(int mode)
{
	struct device *pinmux_dev;
	int ret;

	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	if (pinmux_dev == NULL) {
		SYS_LOG_ERR("Error getting pinmux device\n");
		return -ENXIO;
	}

	/* IOMUX settings */
	ret = iomux_set_lcd(pinmux_dev, mode);
	if (ret) {
		SYS_LOG_ERR("Error configuring iomux\n");
		return -ENXIO;
	}

	return 0;
}
