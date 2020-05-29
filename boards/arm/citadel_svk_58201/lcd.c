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
	PARALLEL_MODE8080, /* 8-bit parallel mode (8080) */
};

#ifndef CONFIG_ALLOW_IOMUX_OVERRIDE
static bool pinmux_configured;
#endif

static int i2c_ioexpander_read(struct device *dev, u8_t reg_addr, u8_t *val,
			       u16_t addr)
{
	struct i2c_msg msgs[2];
	u8_t raddr = reg_addr;

	msgs[0].buf = &raddr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = val;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, addr);
}

static int i2c_ioexpander_write(struct device *dev, u8_t reg_addr, u8_t val,
				u16_t addr)
{
	struct i2c_msg msgs;
	u8_t buff[2];
	u8_t value = val;

	buff[0] = reg_addr;
	buff[1] = value;
	msgs.buf = buff;
	msgs.len = 2;
	msgs.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msgs, 1, addr);
}

static int ioexpander_set_lcd(struct device *dev, int cur_mode)
{
	u8_t val;

	/* Read port 0 */
	i2c_ioexpander_read(dev, IO_EXPANDER_REGA, &val, IO_EXPANDER_ADDR);
	/* Set IM1, IM2(bits 3,4). SMCC(bit 6)= 0 */
	if (cur_mode == SERIAL_SPI)
		val = (val & ~0x40) | 0x18;
	else /* IM1(bit 3), IM2(bit 4), GPIO_SMCC(bit 6) are 0 */
		val = 0x07;
	/* Write to port 0 */
	i2c_ioexpander_write(dev, 0x8 + IO_EXPANDER_REGA, val,
			     IO_EXPANDER_ADDR);
	/* Configure port 0 as output */
	i2c_ioexpander_write(dev, 0x18 + IO_EXPANDER_REGA, 0,
			     IO_EXPANDER_ADDR);
	/* Read back port 0 */
	i2c_ioexpander_read(dev, IO_EXPANDER_REGA, &val, IO_EXPANDER_ADDR);
	SYS_LOG_INF("IO_EXPANDER_REGA: 0x%x\n", val);

	/* Read port 1 */
	i2c_ioexpander_read(dev, IO_EXPANDER_REGB, &val, IO_EXPANDER_ADDR);
	if (cur_mode == SERIAL_SPI)
		val = (val & ~0x0C);
	else
		val = (val & ~0x55) | 0x55;
	i2c_ioexpander_write(dev, 0x8 + IO_EXPANDER_REGB, val,
			     IO_EXPANDER_ADDR);
	/* Configure port 1 as output */
	i2c_ioexpander_write(dev, 0x18 + IO_EXPANDER_REGB, 0,
			     IO_EXPANDER_ADDR);
	/* Read back port 1 */
	i2c_ioexpander_read(dev, IO_EXPANDER_REGB, &val, IO_EXPANDER_ADDR);
	SYS_LOG_INF("IO_EXPANDER_REGB: 0x%x\n", val);


	i2c_ioexpander_read(dev, IO_EXPANDER_REGC, &val, IO_EXPANDER_ADDR);
	if (cur_mode == PARALLEL_MODE8080) {
		val &= ~0x3;
		i2c_ioexpander_write(dev, 0x8 + IO_EXPANDER_REGC, val,
				     IO_EXPANDER_ADDR);
		i2c_ioexpander_write(dev, 0x18 + IO_EXPANDER_REGC, 0,
				     IO_EXPANDER_ADDR);
		i2c_ioexpander_read(dev, IO_EXPANDER_REGC, &val,
				    IO_EXPANDER_ADDR);
	}
	SYS_LOG_INF("IO_EXPANDER_REGC: 0x%x\n", val);

	i2c_ioexpander_read(dev, IO_EXPANDER_REGD, &val, IO_EXPANDER_ADDR);
	if (cur_mode == PARALLEL_MODE8080) {
		val &= ~0x3;
		i2c_ioexpander_write(dev, 0x8 + IO_EXPANDER_REGD, val,
				     IO_EXPANDER_ADDR);
		i2c_ioexpander_write(dev, 0x18 + IO_EXPANDER_REGD, 0,
				     IO_EXPANDER_ADDR);
		i2c_ioexpander_read(dev, IO_EXPANDER_REGD, &val,
				    IO_EXPANDER_ADDR);
	}
	SYS_LOG_INF("IO_EXPANDER_REGD: 0x%x\n", val);

	i2c_ioexpander_read(dev, IO_EXPANDER_REGE, &val, IO_EXPANDER_ADDR);
	i2c_ioexpander_write(dev, 0x8 + IO_EXPANDER_REGE, 0,
			     IO_EXPANDER_ADDR);
	i2c_ioexpander_write(dev, 0x18 + IO_EXPANDER_REGE, 0,
			     IO_EXPANDER_ADDR);
	i2c_ioexpander_read(dev, IO_EXPANDER_REGE, &val,
			    IO_EXPANDER_ADDR);
	i2c_ioexpander_read(dev, IO_EXPANDER_REGE, &val, IO_EXPANDER_ADDR);
	SYS_LOG_INF("IO_EXPANDER_REGE: 0x%x\n", val);

	/* Print for debug. IM0/IM4 should be 0 by default */
	i2c_ioexpander_read(dev, 0, &val, IO_EXPANDER_PCA9538_ADDR);
	SYS_LOG_INF("IO_EXPANDER_PCA9538: 0x%x\n", val);
	return 0;
}

/* For SPI mode, mux should be configured to select SPI1.
 * For Parallel mode, mux should be configured to select SMC (SRAM0)
 */
#define D_C_GPIO_SPI_LCD_MODE 2
#define D_C_GPIO_PARALLEL_MODE 12
#define IOMUX_CTRL0_SEL_SRAM 1
#define IOMUX_CTRL0_SEL_GPIO2 0

static int iomux_set_lcd(struct device *pinmux_dev, int cur_mode)
{
	struct device *gpio_dev;
	int ret, i;
	s32_t pin;

#ifndef CONFIG_ALLOW_IOMUX_OVERRIDE
	if (pinmux_configured) {
		printk("Pinmux already configured for lcd %s mode\n",
			(cur_mode ? "SMC" : "SPI"));
		return 0;
	}
#endif

	gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
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
			ret = pinmux_pin_set(pinmux_dev, (i + 38),
					     IOMUX_CTRL0_SEL_SRAM);
			if (ret) {
				SYS_LOG_ERR("Failed to set pinmux %d\n",
					    (i + 38));
				return ret;
			}
		}
	}

	/* Enable GPIO function for GPIO2 (MFIO 39) for SPI mode */
	if (cur_mode == SERIAL_SPI) {
		ret = pinmux_pin_set(pinmux_dev, 39, IOMUX_CTRL0_SEL_GPIO2);
		if (ret) {
			SYS_LOG_ERR("Failed to set pinmux to enable GPIO2: %d",
				    ret);
			return ret;
		}
	}
#ifndef CONFIG_ALLOW_IOMUX_OVERRIDE
	pinmux_configured = true;
#endif

	return 0;
}

int lcd_board_configure(int mode)
{
	struct device *pinmux_dev, *i2c_dev;
	int ret;

	if (mode != SERIAL_SPI &&
	    mode != PARALLEL_MODE8080) {
		SYS_LOG_ERR("Incorrect lcd mode: %d\n", mode);
		return -EINVAL;
	}

	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	if (pinmux_dev == NULL) {
		SYS_LOG_ERR("Error getting pinmux device\n");
		return -ENXIO;
	}

	i2c_dev = device_get_binding(CONFIG_I2C0_NAME);
	if (i2c_dev == NULL) {
		SYS_LOG_ERR("Error getting i2c device\n");
		return -ENXIO;
	}

	/* IO expander settings */
	ret = ioexpander_set_lcd(i2c_dev, mode);
	if (ret) {
		SYS_LOG_ERR("Error configuring ioexpander\n");
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
