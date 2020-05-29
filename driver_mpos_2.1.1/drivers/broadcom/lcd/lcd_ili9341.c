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

#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <broadcom/gpio.h>
#include <lcd_adafruit.h>
#include "lcd_ili9341.h"
#include <broadcom/i2c.h>
#include <spi_legacy.h>
#include <sys_clock.h>
#include <dmu.h>

/* @file lcd_ili9341.c
 *
 * Driver to interface with Adafruit ILI9341 LCD device
 * in serial SPI mode and parallel 8080 (SMC) mode.
 *
 */

/* Delay routine for LCD in nano seconds */
static void lcd_delay(u32_t nsec_to_wait)
{
	/* use 64-bit math to prevent overflow when multiplying */
	u32_t cycles_to_wait = (u32_t)(
		(u64_t)nsec_to_wait *
		(u64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/
		(u64_t)NSEC_PER_SEC
	);
	u32_t start_cycles = k_cycle_get_32();

	for (;;) {
		u32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

/* Drive the data/command pins as per the mode */
static void set_dc_high(struct device *dev)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

#ifdef CONFIG_LCD_SPI_MODE
	if (data->cur_mode == SERIAL_SPI)
		gpio_pin_write(data->gpio_dev_spi,
			DC_GPIO_SPI_MODE & 0x1f, 0x1);
#endif
	if (data->cur_mode == PARALLEL_MODE8080)
		gpio_pin_write(data->gpio_dev_8080,
			DC_GPIO_PARALLEL_MODE & 0x1f, 0x1);
}

static void set_dc_low(struct device *dev)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

#ifdef CONFIG_LCD_SPI_MODE
	if (data->cur_mode == SERIAL_SPI)
		gpio_pin_write(data->gpio_dev_spi,
			DC_GPIO_SPI_MODE & 0x1f, 0x0);
#endif
	if (data->cur_mode == PARALLEL_MODE8080)
		gpio_pin_write(data->gpio_dev_8080,
			DC_GPIO_PARALLEL_MODE & 0x1f, 0x0);
}

/* Wrappers for starting and ending write in SPI mode */
static void start_write(struct device *dev)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

	if (data->cur_mode == SERIAL_SPI)
		spi_slave_select(data->spi_dev, 0);
}

static void end_write(struct device *dev)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

	if (data->cur_mode == SERIAL_SPI)
		spi_slave_select(data->spi_dev, 1);
}

/* Write data to LCD */
static int write_data(struct device *dev, u8_t val)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;
	int ret = 0;
	u8_t buff, recv;

	buff = val;

	if (data->cur_mode == SERIAL_SPI) {
		ret = spi_transceive(data->spi_dev, &buff, 1, &recv, 1);
		if (ret)
			SYS_LOG_ERR("SPI write failed\n");
	} else if (data->cur_mode == PARALLEL_MODE8080) {
		*((volatile u8_t *)SMC_LCD_DATA_ADDR) = val;
		/* This delay ~250 nsec is observed to be needed
		 * while writing to SMC bus, otherwise back to back
		 * smc writes result in erratic display/failure.
		 * As per the LCD controller spec, write cycle (twc)
		 * is ~66 nsec.
		 */
		lcd_delay(LCD_SMC_DELAY);
	} else {
		SYS_LOG_ERR("LCD mode not configured\n");
		return -ENXIO;
	}

	return ret;
}

/* Write command to LCD */
static int write_command(struct device *dev, u8_t command)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;
	int ret = 0;

	set_dc_low(dev);

	if (data->cur_mode == SERIAL_SPI) {
		ret = write_data(dev, command);
	} else if (data->cur_mode == PARALLEL_MODE8080) {
		*((volatile u8_t *)SMC_LCD_CMD_ADDR) = command;
		/* This delay ~250 nsec is observed to be needed
		 * while writing to SMC bus, otherwise back to back
		 * smc writes result in erratic display/failure.
		 * As per the LCD controller spec, write cycle (twc)
		 * is ~66 nsec.
		 */
		lcd_delay(LCD_SMC_DELAY);
	} else {
		SYS_LOG_ERR("LCD mode not configured\n");
		return -ENXIO;
	}

	set_dc_high(dev);

	return ret;
}

/* Write data to LCD */
static int lcd_ili9341_write_data(struct device *dev, u8_t val)
{
	int ret;

	start_write(dev);
	ret = write_data(dev, val);
	end_write(dev);

	return ret;
}

/* Write command to LCD */
static int lcd_ili9341_write_command(struct device *dev, u8_t command)
{
	int ret;

	start_write(dev);
	ret = write_command(dev, command);
	end_write(dev);

	return ret;
}

/* Read values from certain LCD registers */
static u8_t lcd_ili9341_read_data(struct device *dev, u8_t command)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;
	u8_t rx_val;
	int ret;

	start_write(dev);

	write_command(dev, 0xD9);
	write_data(dev, 0x10 + 0x0);
	write_command(dev, command);
	if (data->cur_mode == SERIAL_SPI) {
		ret = spi_transceive(data->spi_dev, NULL, 0, &rx_val, 1);
		if (ret)
			SYS_LOG_ERR("SPI read failed\n");
	} else if (data->cur_mode == PARALLEL_MODE8080) {
		rx_val = *((volatile u8_t *)SMC_LCD_DATA_ADDR);
	} else {
		SYS_LOG_ERR("LCD mode not configured\n");
		return -ENXIO;
	}

	end_write(dev);

	return rx_val;
}

/* Configure memory access control register from config */
static void lcd_set_orientation(struct device *dev, struct lcd_config *cfg)
{
	start_write(dev);
	write_command(dev, ILI9341_MADCTL);
	write_data(dev, (u8_t)cfg->orientation);
	end_write(dev);
}

/* Select Column and Row addresses to define an LCD window on which
 * memory writes will have effect.
 */
static void lcd_set_addr_window(struct device *dev, u32_t x, u32_t y,
				u32_t w, u32_t h)
{
	u32_t xa, ya;

	xa = ((u32_t)x << 16) | (x+w-1);
	ya = ((u32_t)y << 16) | (y+h-1);

	/* Column addr set */
	write_command(dev, ILI9341_CASET);
	write_data(dev, (char)(xa >> 24) & 0xFF);
	write_data(dev, (char)(xa >> 16) & 0xFF);
	write_data(dev, (char)(xa >> 8) & 0xFF);
	write_data(dev, (char)(xa) & 0xFF);

	/* Row addr set */
	write_command(dev, ILI9341_PASET);
	write_data(dev, (char)(ya >> 24) & 0xFF);
	write_data(dev, (char)(ya >> 16) & 0xFF);
	write_data(dev, (char)(ya >> 8) & 0xFF);
	write_data(dev, (char)(ya) & 0xFF);

	write_command(dev, ILI9341_RAMWR);
}

/*
 * Helper routine to draw rectangle.
 * (x, y) = (column, row) coordinates
 * w = width (can't be more than 320)
 * h = height (can't be more than240)
 * color = pixel value to be written
 */
static void lcd_ili9341_draw_rect(struct device *dev, s16_t x, s16_t y,
				  s16_t w, s16_t h, u16_t color)
{
	s16_t x2 = x + w - 1;
	s16_t y2 = y + h - 1;
	u32_t len, i;

	if ((x >= ILI9341_TFTHEIGHT) || (y >= ILI9341_TFTWIDTH))
		return;

	if ((x2 < 0) || (y2 < 0))
		return;

	/* Clip left/top */
	if (x < 0) {
		x = 0;
		w = x2 + 1;
	}

	if (y < 0) {
		y = 0;
		h = y2 + 1;
	}

	/* Clip right/bottom */
	if (x2 >= ILI9341_TFTHEIGHT)
		w = ILI9341_TFTHEIGHT - x;
	if (y2 >= ILI9341_TFTWIDTH)
		h = ILI9341_TFTWIDTH - y;

	len = (int32_t)w * h;
	lcd_set_addr_window(dev, x, y, w, h);

	for (i = 0; i < len; i++) {
		write_data(dev, (u8_t)(color >> 8));
		write_data(dev, (u8_t)color);
	}
}

static void lcd_ili9341_draw_vline(struct device *dev, s16_t x, s16_t y,
				   s16_t h, u16_t color)
{
	start_write(dev);
	lcd_ili9341_draw_rect(dev, x, y, 1, h, color);
	end_write(dev);
}

static void lcd_ili9341_draw_hline(struct device *dev, s16_t x, s16_t y,
				   s16_t w, u16_t color)
{
	start_write(dev);
	lcd_ili9341_draw_rect(dev, x, y, w, 1, color);
	end_write(dev);
}

static void lcd_ili9341_display_pixel(struct device *dev, s16_t x, s16_t y,
				      u16_t color)
{
	start_write(dev);
	lcd_set_addr_window(dev, x, y, 1, 1);
	write_data(dev, (u8_t)(color >> 8));
	write_data(dev, (u8_t)color);
	end_write(dev);
}

static void lcd_ili9341_draw_bitmap_RGB(struct device *dev, s16_t x, s16_t y,
					u16_t *pcolors, s16_t w, s16_t h)
{
	s16_t x2, y2; /* Lower-right coord */
	s16_t bx1 = 0, by1 = 0; /* Clipped top-left within bitmap */
	s16_t saveW = w;	/* Save original bitmap width value */

	x2 = x + w - 1;
	y2 = y + h - 1;
	if ((x >= ILI9341_TFTHEIGHT) ||   /* Off-edge right */
	    (y >= ILI9341_TFTWIDTH) ||  /* top */
	    (x2 < 0) ||	/* left */
	    (y2 < 0))	/* bottom */
		return;

	if (x < 0) { /* Clip left */
		w  +=  x;
		bx1 = -x;
		x   =  0;
	}

	if (y < 0) { /* Clip top */
		h  +=  y;
		by1 = -y;
		y   =  0;
	}

	if (x2 >= ILI9341_TFTHEIGHT)
		w = ILI9341_TFTHEIGHT - x; /* Clip right */

	if (y2 >= ILI9341_TFTWIDTH)
		h = ILI9341_TFTWIDTH - y; /* Clip bottom */

	/* Offset bitmap ptr to clipped top-left */
	pcolors += by1 * saveW + bx1;

	start_write(dev);

	lcd_set_addr_window(dev, x, y, w, h); /* Clipped area */
	while (h--) { /* For each (clipped) scanline... */
		/* Push one (clipped) row */
		for (int i = 0; i < (w * 2); i += 2) {
			write_data(dev, ((u8_t *)(pcolors))[i+1]);
			write_data(dev, ((u8_t *)(pcolors))[i]);
		}
		pcolors += saveW; /* Incr ptr by one full (unclipped) line */
	}
	end_write(dev);
}

static void lcd_ili9341_display_clear(struct device *dev)
{
	start_write(dev);

	lcd_set_addr_window(dev, 0, 0, ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH);
	for (int i = 0; i < (ILI9341_TFTHEIGHT * ILI9341_TFTWIDTH); i++) {
		write_data(dev, 0);
		write_data(dev, 0);
	}

	end_write(dev);
}

/*
 * Initialize ILI9341 TFT LCD hardware with default values.
 * These are the init values provided in reference code by
 * vendor (Adafruit).
 */
static int lcd_hw_init(struct device *dev)
{
	start_write(dev);

	write_command(dev, 0xEF);
	write_data(dev, 0x03);
	write_data(dev, 0x80);
	write_data(dev, 0x02);

	write_command(dev, 0xCF);
	write_data(dev, 0x00);
	write_data(dev, 0XC1);
	write_data(dev, 0X30);

	write_command(dev, 0xED);
	write_data(dev, 0x64);
	write_data(dev, 0x03);
	write_data(dev, 0X12);
	write_data(dev, 0X81);

	write_command(dev, 0xE8);
	write_data(dev, 0x85);
	write_data(dev, 0x00);
	write_data(dev, 0x78);

	write_command(dev, 0xCB);
	write_data(dev, 0x39);
	write_data(dev, 0x2C);
	write_data(dev, 0x00);
	write_data(dev, 0x34);
	write_data(dev, 0x02);

	write_command(dev, 0xF7);
	write_data(dev, 0x20);

	write_command(dev, 0xEA);
	write_data(dev, 0x00);
	write_data(dev, 0x00);

	/* Power control1 */
	write_command(dev, ILI9341_PWCTR1);
	write_data(dev, 0x23);

	/* Power control2 */
	write_command(dev, ILI9341_PWCTR2);
	write_data(dev, 0x10);

	/* VCM control */
	write_command(dev, ILI9341_VMCTR1);
	write_data(dev, 0x3e);
	write_data(dev, 0x28);

	/* VCM control2 */
	write_command(dev, ILI9341_VMCTR2);
	write_data(dev, 0x86);

	/* Memory Access Control */
	write_command(dev, ILI9341_MADCTL);
	write_data(dev, 0x48);

	/* Vertical scroll */
	write_command(dev, ILI9341_VSCRSADD);
	write_data(dev, 0);

	write_command(dev, ILI9341_PIXFMT);
	write_data(dev, 0x55);

	write_command(dev, ILI9341_FRMCTR1);
	write_data(dev, 0x00);
	write_data(dev, 0x18);

	/* Display Function Control */
	write_command(dev, ILI9341_DFUNCTR);
	write_data(dev, 0x08);
	write_data(dev, 0x82);
	write_data(dev, 0x27);

	/* 3Gamma Function Disable */
	write_command(dev, 0xF2);
	write_data(dev, 0x00);

	/* Gamma curve selected */
	write_command(dev, ILI9341_GAMMASET);
	write_data(dev, 0x01);

	/* Set Gamma */
	write_command(dev, ILI9341_GMCTRP1);
	write_data(dev, 0x0F);
	write_data(dev, 0x31);
	write_data(dev, 0x2B);
	write_data(dev, 0x0C);
	write_data(dev, 0x0E);
	write_data(dev, 0x08);
	write_data(dev, 0x4E);
	write_data(dev, 0xF1);
	write_data(dev, 0x37);
	write_data(dev, 0x07);
	write_data(dev, 0x10);
	write_data(dev, 0x03);
	write_data(dev, 0x0E);
	write_data(dev, 0x09);
	write_data(dev, 0x00);

	/* Set Gamma */
	write_command(dev, ILI9341_GMCTRN1);
	write_data(dev, 0x00);
	write_data(dev, 0x0E);
	write_data(dev, 0x14);
	write_data(dev, 0x03);
	write_data(dev, 0x11);
	write_data(dev, 0x07);
	write_data(dev, 0x31);
	write_data(dev, 0xC1);
	write_data(dev, 0x48);
	write_data(dev, 0x08);
	write_data(dev, 0x0F);
	write_data(dev, 0x0C);
	write_data(dev, 0x31);
	write_data(dev, 0x36);
	write_data(dev, 0x0F);

	/* Exit Sleep */
	write_command(dev, ILI9341_SLPOUT);
	k_sleep(120);

	/* Display on */
	write_command(dev, ILI9341_DISPON);
	k_sleep(120);

	end_write(dev);
	return 0;

}

static int lcd_spi_mode_configure(struct lcd_data *data)
{
	int ret;
	struct spi_config cfg;

	cfg.max_sys_freq = LCD_SPI_FREQ;
	cfg.config = SPI_WORD(8);

	ret = spi_configure(data->spi_dev, &cfg);
	if (ret) {
		SYS_LOG_ERR("Error configuring SPI interface\n");
		return ret;
	}

	spi_slave_select(data->spi_dev, 1);

	return 0;
}

/*TODO: Use SMC driver APIs when available */
static int lcd_parallel_mode_configure(struct lcd_data *data)
{
	ARG_UNUSED(data);
	u32_t val = 0;

	sys_write32(0x3, REG_FN_MOD_AHB);

	sys_clear_bit(CDRU_SW_RESET_CTRL, SMC_SW_RST);
	k_busy_wait(1000);
	sys_set_bit(CDRU_SW_RESET_CTRL, SMC_SW_RST);
	k_busy_wait(1000);

	sys_write32(SMC_MATCH_MASK_VAL, CDRU_SMC_ADDRESS_MATCH_MASK);
	k_busy_wait(1000);

	sys_write32(SRAM_CYCLES_VAL, set_cycles);
	k_busy_wait(1000);

	sys_write32(0x0, set_opmode);
	k_busy_wait(1000);

	SYS_LOG_INF("opmode0_0: 0x%x\n", sys_read32(opmode0_0));
	SYS_LOG_INF("opmode0_1: 0x%x\n", sys_read32(opmode0_1));

	val = CMD_TYPE << CMD_TYPE_SHIFT;
	sys_write32(val, direct_cmd);
	k_busy_wait(1000);

	sys_write32(0x0, set_opmode);
	k_busy_wait(1000);

	val = CMD_TYPE << CMD_TYPE_SHIFT | CHIP_NUM << CHIP_NUM_SHIFT;
	sys_write32(val, direct_cmd);
	k_busy_wait(1000);

	sys_write32(0x0, set_opmode);
	k_busy_wait(1000);

	return 0;
}

static int lcd_ili9341_configure(struct device *dev,
				   struct lcd_config *config)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

	if (config->mode == SERIAL_SPI) {
		if (!data->spi_mode_en) {
			SYS_LOG_ERR("LCD SPI mode not enabled\n");
			return -EINVAL;
		}
	} else {
		if (data->spi_mode_en) {
			SYS_LOG_ERR("LCD SPI mode is enabled\n");
			return -EINVAL;
		}
	}

	data->cur_mode = config->mode;

	/* SPI/SMC interface config */
	if (config->mode == SERIAL_SPI) {
		lcd_spi_mode_configure(data);
	} else if (config->mode == PARALLEL_MODE8080) {
		lcd_parallel_mode_configure(data);
	} else {
		SYS_LOG_ERR("Configure failed: LCD Mode invalid\n");
		return -EINVAL;
	}
	SYS_LOG_INF("Mode Configure success\n");

	/* LCD hardware init */
	lcd_hw_init(dev);

	/* LCD set orientation */
	if (config->orientation == LANDSCAPE) {
		config->orientation = (u8_t)(MADCTL_MV | MADCTL_RGB);
	} else if (config->orientation == PORTRAIT) { /* PORTRAIT */
		config->orientation = (u8_t)MADCTL_RGB;
	} else {
		SYS_LOG_ERR("Configure failed: LCD orientation invalid\n");
		return -EINVAL;
	}

	lcd_set_orientation(dev, config);

	return 0;
}

static struct lcd_driver_api lcd_dev_api = {
	.configure	= lcd_ili9341_configure,
	.read_data	= lcd_ili9341_read_data,
	.write_cmd	= lcd_ili9341_write_command,
	.write_data	= lcd_ili9341_write_data,
	.display_pixel	= lcd_ili9341_display_pixel,
	.draw_vline	= lcd_ili9341_draw_vline,
	.draw_hline	= lcd_ili9341_draw_hline,
	.display_clear	= lcd_ili9341_display_clear,
	.draw_bitmap	= lcd_ili9341_draw_bitmap_RGB,
};

static struct lcd_config lcd_dev_config;

static struct lcd_data lcd_dev_data = {
	.spi_mode_en = false,
};

static int lcd_ili9341_init(struct device *dev)
{
	struct lcd_data *data = (struct lcd_data *)dev->driver_data;

#ifdef CONFIG_LCD_SPI_MODE
	data->spi_dev = device_get_binding(CONFIG_LCD_SPI_DEV);
	if (data->spi_dev == NULL) {
		SYS_LOG_ERR("Error getting spi device");
		return -ENXIO;
	}
	data->spi_mode_en = true;

	switch (DC_GPIO_SPI_MODE / 32) {
		default:
		case 0:
			data->gpio_dev_spi =
				device_get_binding(CONFIG_GPIO_DEV_NAME_0);
			break;
		case 1:
			data->gpio_dev_spi =
				device_get_binding(CONFIG_GPIO_DEV_NAME_1);
			break;
		case 2:
			data->gpio_dev_spi =
				device_get_binding(CONFIG_GPIO_DEV_NAME_2);
			break;
	}

	if (data->gpio_dev_spi == NULL) {
		SYS_LOG_ERR("Error getting gpio device for spi mode");
		return -ENXIO;
	}
#endif /* CONFIG_LCD_SPI_MODE */

	switch (DC_GPIO_PARALLEL_MODE / 32) {
		default:
		case 0:
			data->gpio_dev_8080 =
				device_get_binding(CONFIG_GPIO_DEV_NAME_0);
			break;
		case 1:
			data->gpio_dev_8080 =
				device_get_binding(CONFIG_GPIO_DEV_NAME_1);
			break;
		case 2:
			data->gpio_dev_8080 =
				device_get_binding(CONFIG_GPIO_DEV_NAME_2);
			break;
	}

	if (data->gpio_dev_8080 == NULL) {
		SYS_LOG_ERR("Error getting gpio device for 8080 mode");
		return -ENXIO;
	}

	data->cur_mode = UNCONFIGURED;
	SYS_LOG_INF("lcd_ili9341 driver initialized\n");

	return 0;
}

DEVICE_AND_API_INIT(lcd, CONFIG_LCD_DEV_NAME,
		    lcd_ili9341_init, &lcd_dev_data,
		    &lcd_dev_config, POST_KERNEL,
		    CONFIG_LCD_DRIVER_INIT_PRIORITY, &lcd_dev_api);
