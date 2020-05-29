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

/* @file test_qspi_bcm58202.c
 *
 * Unit test for BCM58202 QSPI flash driver
 *
 * This file implements the unit tests for the bcm58202 qspi flash driver
 */

#include <test.h>
#include <errno.h>
#include <board.h>
#include <string.h>
#include <unicam.h>
#include <clock_a7.h>

#include <spi_legacy.h>
#include <broadcom/gpio.h>
#include <pinmux.h>
#include <broadcom/i2c.h>
#ifdef CONFIG_ZBAR
#include <zbar.h>
#endif

#include <lcd/lcd_ili9341.h>

#define SEGMENT_HEIGHT		40

#ifdef CONFIG_LCD_SPI_MODE_DC_GPIO
#define DATA_COMMAND_GPIO	CONFIG_LCD_SPI_MODE_DC_GPIO
#else
#define DATA_COMMAND_GPIO	2
#endif

/* GPIO to MFIO mapping */
#define GPIO_TO_MFIO(X)		(((X) >= 32) ? (X) - 32 : (X) + 37)
/* MFIO mode value to select GPIO */
#define MFIO_MODE_GPIO(X)	(((X) >= 32) ? 3 : 0)

/* Default to spi1 */
#ifndef CONFIG_LCD_SPI_DEV
#define CONFIG_LCD_SPI_DEV	"spi1"
#endif

#define	set_dc_high()	gpio_pin_write(gpio_dev, DATA_COMMAND_GPIO & 0x1F, 0x1)
#define	set_dc_low()	gpio_pin_write(gpio_dev, DATA_COMMAND_GPIO & 0x1F, 0x0)
#define start_write()	spi_slave_select(spi_dev, 0)
#define end_write()	spi_slave_select(spi_dev, 1)

#define SPI_WRITE16(s)			\
	do {				\
		lcd_spi_write((s) >> 8);\
		lcd_spi_write((u8_t)s);	\
	} while (0)

#define SPI_WRITE32(l)					\
	do {						\
		lcd_spi_write(((l) >> 24) & 0xFF);	\
		lcd_spi_write(((l) >> 16) & 0xFF);	\
		lcd_spi_write(((l) >> 8) & 0xFF);	\
		lcd_spi_write(l & 0xFF);		\
	} while (0)

#define ILI9341_TFTWIDTH   240   /* < ILI9341 max TFT width */
#define ILI9341_TFTHEIGHT  320   /* < ILI9341 max TFT height */

#define ILI9341_NOP        0x00  /* < No-op register */
#define ILI9341_SWRESET    0x01  /* < Software reset register */
#define ILI9341_RDDID      0x04  /* < Read display identification information */
#define ILI9341_RDDST      0x09  /* < Read Display Status */

#define ILI9341_SLPIN      0x10  /* < Enter Sleep Mode */
#define ILI9341_SLPOUT     0x11  /* < Sleep Out */
#define ILI9341_PTLON      0x12  /* < Partial Mode ON */
#define ILI9341_NORON      0x13  /* < Normal Display Mode ON */

#define ILI9341_RDMODE     0x0A  /* < Read Display Power Mode */
#define ILI9341_RDMADCTL   0x0B  /* < Read Display MADCTL */
#define ILI9341_RDPIXFMT   0x0C  /* < Read Display Pixel Format */
#define ILI9341_RDIMGFMT   0x0D  /* < Read Display Image Format */
#define ILI9341_RDSELFDIAG 0x0F  /* < Read Display Self-Diagnostic Result */

#define ILI9341_INVOFF     0x20  /* < Display Inversion OFF */
#define ILI9341_INVON      0x21  /* < Display Inversion ON */
#define ILI9341_GAMMASET   0x26  /* < Gamma Set */
#define ILI9341_DISPOFF    0x28  /* < Display OFF  */
#define ILI9341_DISPON     0x29  /* < Display ON */

#define ILI9341_CASET      0x2A  /* < Column Address Set  */
#define ILI9341_PASET      0x2B  /* < Page Address Set */
#define ILI9341_RAMWR      0x2C  /* < Memory Write */
#define ILI9341_RAMRD      0x2E  /* < Memory Read */

#define ILI9341_PTLAR      0x30  /* < Partial Area */
#define ILI9341_MADCTL     0x36  /* < Memory Access Control */
#define ILI9341_VSCRSADD   0x37  /* < Vertical Scrolling Start Address */
#define ILI9341_PIXFMT     0x3A  /* < COLMOD: Pixel Format Set */

#define ILI9341_FRMCTR1    0xB1  /* < Frame Rate (Normal Mode/Full Colors) */
#define ILI9341_FRMCTR2    0xB2  /* < Frame Rate (Idle Mode/8 colors) */
#define ILI9341_FRMCTR3    0xB3  /* < Frame Rate (Partial Mode/Full Colors) */
#define ILI9341_INVCTR     0xB4  /* < Display Inversion Control */
#define ILI9341_DFUNCTR    0xB6  /* < Display Function Control */

#define ILI9341_PWCTR1     0xC0  /* < Power Control 1 */
#define ILI9341_PWCTR2     0xC1  /* < Power Control 2 */
#define ILI9341_PWCTR3     0xC2  /* < Power Control 3 */
#define ILI9341_PWCTR4     0xC3  /* < Power Control 4 */
#define ILI9341_PWCTR5     0xC4  /* < Power Control 5 */
#define ILI9341_VMCTR1     0xC5  /* < VCOM Control 1 */
#define ILI9341_VMCTR2     0xC7  /* < VCOM Control 2 */

#define ILI9341_RDID1      0xDA  /* < Read ID 1 */
#define ILI9341_RDID2      0xDB  /* < Read ID 2 */
#define ILI9341_RDID3      0xDC  /* < Read ID 3 */
#define ILI9341_RDID4      0xDD  /* < Read ID 4 */

#define ILI9341_GMCTRP1    0xE0  /* < Positive Gamma Correction */
#define ILI9341_GMCTRN1    0xE1  /* < Negative Gamma Correction */

/* Color definitions */
#define ILI9341_BLACK       0x0000  /* <   0,   0,   0 */
#define ILI9341_NAVY        0x000F  /* <   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0  /* <   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF  /* <   0, 128, 128 */
#define ILI9341_MAROON      0x7800  /* < 128,   0,   0 */
#define ILI9341_PURPLE      0x780F  /* < 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0  /* < 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618  /* < 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF  /* < 128, 128, 128 */
#define ILI9341_BLUE        0x001F  /* <   0,   0, 255 */
#define ILI9341_GREEN       0x07E0  /* <   0, 255,   0 */
#define ILI9341_CYAN        0x07FF  /* <   0, 255, 255 */
#define ILI9341_RED         0xF800  /* < 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F  /* < 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0  /* < 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF  /* < 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20  /* < 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5  /* < 173, 255,  47 */

#define MADCTL_MY  0x80 /* < Bottom to top */
#define MADCTL_MX  0x40 /* < Right to left */
#define MADCTL_MV  0x20 /* < Reverse Mode */
#define MADCTL_ML  0x10 /* < LCD refresh Bottom to top */
#define MADCTL_RGB 0x00 /* < Red-Green-Blue pixel order */
#define MADCTL_BGR 0x08 /* < Blue-Green-Red pixel order */
#define MADCTL_MH  0x04 /* < LCD refresh right to left */

static struct device *spi_dev, *gpio_dev;

/* IO expander on SVK board needs to be configured
 * to route the SPI1 signals out of the J37 header.
 */
#define IO_EXPANDER_ADDR	0x20
#define IO_EXPANDER_REG		0x4
#define IO_EXPANDER_REGA	0x0
#define IO_EXPANDER_REGB	0x1
#define IO_EXPANDER_REGC	0x2
#define IO_EXPANDER_REGD	0x3
#define IO_EXPANDER_REGE	0x4

#ifdef CONFIG_BOARD_CITADEL_SVK_58201
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


static int configure_spi1_sel(void)
{
	int ret;
	u8_t val = 0;
	struct device *dev;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");

	ret = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(ret == 0, "i2c_configure() returned error");

	/* Read port 0 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGA, &val);
	/* Write to port 0 - Bits 3,4 = 1 and Bit 6 = 0 */
	test_i2c_write_io_exander(
		dev, 0x8 + IO_EXPANDER_REGA, (val & ~0x40) | 0x18);
	/* Configure port 0 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGA, val & ~0x58);
	/* Read back port 0 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGA, &val);

	/* Read port 1 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGB, &val);
	/* Write to port 1 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGB, val & ~0x0C);
	/* Configure port 1 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGB, val & ~0x0C);
	/* Read back port 1 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGB, &val);

	/* Read port 5 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGE, &val);
	/* Write to port 5 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGE, val & ~0x01);
	/* Configure port 5 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGE, val & ~0x01);
	/* Read back port 5 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGE, &val);

	return 0;
}
#endif /* CONFIG_BOARD_CITADEL_SVK_58201 */

static int lcd_spi_config(u32_t frequency)
{
	int ret;
	struct spi_config cfg;
	struct device *pinmux_dev;

#ifdef CONFIG_BOARD_CITADEL_SVK_58201
	configure_spi1_sel();
#endif

	TC_PRINT("lcd spi speed: %d.%d Mhz\n",
		 frequency/1000000, (frequency/1000) % 1000);

	/* Prepare the config structure
	 * Currently the SPI driver only supports master mode
	 */
	cfg.max_sys_freq = frequency;
	cfg.config = SPI_WORD(8);

	ret = spi_configure(spi_dev, &cfg);
	if (ret) {
		TC_ERROR("spi_configure() failed with %d\n", ret);
		return ret;
	}

	/* Slave select Initialize to 1 (unselect slave) */
	spi_slave_select(spi_dev, 1);

	switch (DATA_COMMAND_GPIO & ~0x1F) {
	default:
	case 0x0:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
		break;
	case 0x20:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
		break;
	case 0x40:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
		break;
	}

	zassert_not_null(gpio_dev, "Failed to get gpio device binding");

	ret = gpio_pin_write(gpio_dev, DATA_COMMAND_GPIO & 0x1F, 0);
	zassert_true(ret == 0, "Failed to write to D/C GPIO");
	ret = gpio_pin_configure(
			gpio_dev, DATA_COMMAND_GPIO & 0x1F, GPIO_DIR_OUT);
	zassert_true(ret == 0, "Failed to configure D/C GPIO");

	/* Enable GPIO function for */
	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	zassert_not_null(pinmux_dev, "Failed to get pinmux binding");
	pinmux_pin_set(pinmux_dev, GPIO_TO_MFIO(DATA_COMMAND_GPIO),
				   MFIO_MODE_GPIO(DATA_COMMAND_GPIO));

	return 0;
}

static int lcd_spi_write(u8_t val)
{
	int ret;
	u8_t buff, recv;

	buff = val;
	ret = spi_transceive(spi_dev, &buff, 1, &recv, 1);
	zassert_true(ret == 0, "SPI write failed\n");
	return ret;
}

static int write_command(u8_t command)
{
	int ret;

	set_dc_low();
	ret = lcd_spi_write(command);
	set_dc_high();
	return ret;
}

u8_t lcd_spi_read(u8_t command)
{
	u8_t r;
	int ret;

	start_write();
	write_command(0xD9);  /* woo sekret command? */
	lcd_spi_write(0x10 + 0x0);
	write_command(command);
	ret = spi_transceive(spi_dev, NULL, 0, &r, 1);
	zassert_true(ret == 0, "SPI read failed\n");
	end_write();

	return r;
}

static int init_ili9341(struct device *spi_dev)
{
	start_write();

	write_command(0xEF);
	lcd_spi_write(0x03);
	lcd_spi_write(0x80);
	lcd_spi_write(0x02);

	write_command(0xCF);
	lcd_spi_write(0x00);
	lcd_spi_write(0XC1);
	lcd_spi_write(0X30);

	write_command(0xED);
	lcd_spi_write(0x64);
	lcd_spi_write(0x03);
	lcd_spi_write(0X12);
	lcd_spi_write(0X81);

	write_command(0xE8);
	lcd_spi_write(0x85);
	lcd_spi_write(0x00);
	lcd_spi_write(0x78);

	write_command(0xCB);
	lcd_spi_write(0x39);
	lcd_spi_write(0x2C);
	lcd_spi_write(0x00);
	lcd_spi_write(0x34);
	lcd_spi_write(0x02);

	write_command(0xF7);
	lcd_spi_write(0x20);

	write_command(0xEA);
	lcd_spi_write(0x00);
	lcd_spi_write(0x00);

	write_command(ILI9341_PWCTR1);    /* Power control */
	lcd_spi_write(0x23);   /* VRH[5:0] */

	write_command(ILI9341_PWCTR2);    /* Power control */
	lcd_spi_write(0x10);   /* SAP[2:0];BT[3:0] */

	write_command(ILI9341_VMCTR1);    /* VCM control */
	lcd_spi_write(0x3e);
	lcd_spi_write(0x28);

	write_command(ILI9341_VMCTR2);    /* VCM control2 */
	lcd_spi_write(0x86);  /* -- */

	write_command(ILI9341_MADCTL);    /* Memory Access Control */
	lcd_spi_write(0x48);

	write_command(ILI9341_VSCRSADD);  /* Vertical scroll */
	lcd_spi_write(0);                 /* Zero */

	write_command(ILI9341_PIXFMT);
	lcd_spi_write(0x55);

	write_command(ILI9341_FRMCTR1);
	lcd_spi_write(0x00);
	lcd_spi_write(0x18);

	write_command(ILI9341_DFUNCTR);    /* Display Function Control */
	lcd_spi_write(0x08);
	lcd_spi_write(0x82);
	lcd_spi_write(0x27);

	write_command(0xF2);    /* 3Gamma Function Disable */
	lcd_spi_write(0x00);

	write_command(ILI9341_GAMMASET);    /* Gamma curve selected */
	lcd_spi_write(0x01);

	write_command(ILI9341_GMCTRP1);    /* Set Gamma */
	lcd_spi_write(0x0F);
	lcd_spi_write(0x31);
	lcd_spi_write(0x2B);
	lcd_spi_write(0x0C);
	lcd_spi_write(0x0E);
	lcd_spi_write(0x08);
	lcd_spi_write(0x4E);
	lcd_spi_write(0xF1);
	lcd_spi_write(0x37);
	lcd_spi_write(0x07);
	lcd_spi_write(0x10);
	lcd_spi_write(0x03);
	lcd_spi_write(0x0E);
	lcd_spi_write(0x09);
	lcd_spi_write(0x00);

	write_command(ILI9341_GMCTRN1);    /* Set Gamma */
	lcd_spi_write(0x00);
	lcd_spi_write(0x0E);
	lcd_spi_write(0x14);
	lcd_spi_write(0x03);
	lcd_spi_write(0x11);
	lcd_spi_write(0x07);
	lcd_spi_write(0x31);
	lcd_spi_write(0xC1);
	lcd_spi_write(0x48);
	lcd_spi_write(0x08);
	lcd_spi_write(0x0F);
	lcd_spi_write(0x0C);
	lcd_spi_write(0x31);
	lcd_spi_write(0x36);
	lcd_spi_write(0x0F);

	write_command(ILI9341_SLPOUT);    /* Exit Sleep */
	k_sleep(120);
	write_command(ILI9341_DISPON);    /* Display on */
	k_sleep(120);

	end_write();
	return 0;
}

static void lcd_init(void)
{
	int ret;
	u32_t x = 0;
	u32_t y = 0;
	u32_t w = ILI9341_TFTHEIGHT, h = ILI9341_TFTWIDTH, i, j;
	u32_t xa, ya;

	xa = ((u32_t)x << 16) | (x+w-1);
	ya = ((u32_t)y << 16) | (y+h-1);

	spi_dev = device_get_binding(CONFIG_LCD_SPI_DEV);
	zassert_not_null(spi_dev, "SPI device binding not found");

	/* Configure SPI */
	ret = lcd_spi_config(MHZ(15));
	zassert_true(ret == 0, "lcd_spi_config() failed");

	set_dc_low();

	/* Initialize ILI9341 */
	ret = init_ili9341(spi_dev);

	/*
	 * read diagnostics (optional but can help debug problems)
	 * TC_PRINT("Display Power Mode: 0x%x\n", lcd_spi_read(ILI9341_RDMODE));
	 * TC_PRINT("MADCTL Mode: 0x%x\n", lcd_spi_read(ILI9341_RDMADCTL));
	 * TC_PRINT("Pixel Format: 0x%x\n", lcd_spi_read(ILI9341_RDPIXFMT));
	 * TC_PRINT("Image Format: 0x%x\n", lcd_spi_read(ILI9341_RDIMGFMT));
	 * TC_PRINT("Self Diag: 0x%x\n", lcd_spi_read(ILI9341_RDSELFDIAG));
	 */

	/* Set orientation */
	start_write();
	write_command(ILI9341_MADCTL);
	lcd_spi_write(MADCTL_MV | MADCTL_RGB);
	end_write();

	/* Set address window */
	start_write();
	write_command(ILI9341_CASET); /* Column addr set */
	SPI_WRITE32(xa);
	write_command(ILI9341_PASET); /* Row addr set */
	SPI_WRITE32(ya);
	write_command(ILI9341_RAMWR); /* write to RAM */

	/* Fill color RGB 565 */
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			SPI_WRITE16(i*0x843);
		}
	}
	end_write();
}

/* Display image segment on LCD */
void display_image_segment(
	u8_t *buffer,
	u32_t width,
	u32_t height,
	u8_t seg,
	u32_t line_stride)
{
	u32_t x, y, xa, ya, i, j;
	u32_t disp_width, disp_height;

	/* 640x480 => 320x240 conversion */
	disp_width = width >> 1;
	disp_height = height >> 1;

	/* LCD write should not go beyond the end of the display */
	if (disp_height*(seg + 1) > ILI9341_TFTWIDTH)
		return;

	/* Set address window */
	x = 0;
	y = seg*disp_height;
	xa = (x << 16) | (x + disp_width - 1);
	ya = (y << 16) | (y + disp_height - 1);
	start_write();
	write_command(ILI9341_CASET); /* Column addr set */
	SPI_WRITE32(xa);
	write_command(ILI9341_PASET); /* Row addr set */
	SPI_WRITE32(ya);
	write_command(ILI9341_RAMWR); /* write to RAM */

	/* Write averaged pixel values */
	for (i = 0; i < disp_height; i++) {
		for (j = 0; j < disp_width; j++) {
			u16_t pixel;

			pixel = buffer[2*i*line_stride + 2*j];
			pixel += buffer[2*i*line_stride + 2*j + 1];
			pixel += buffer[(2*i+1)*line_stride + 2*j];
			pixel += buffer[(2*i+1)*line_stride + 2*j + 1];
			pixel /= 4;
			pixel = (pixel >> 3) |
				((pixel >> 2) << 5) |
				((pixel >> 3) << 11);
			SPI_WRITE16(pixel);
		}
	}
	end_write();
}

static bool double_buff_en;

#ifdef CONFIG_ZBAR
/* Max capture buffer size for VGA  (640x480) image (line size aligned to
 * 256 bytes). Extra memory for aligning the width to 256 bytes is only needed
 * for one segment.
 */
u8_t capture_buffer[640*480 + (768-640)*SEGMENT_HEIGHT];
#endif

void test_unicam_driver(void)
{
	int ret;
#ifdef CONFIG_ZBAR
	u32_t len;
	char output[64];
#endif
	u8_t *buffer;
	u8_t num_segs;
	u32_t time, count, stride;

	struct unicam_config cfg;
	struct image_buffer img_buff;
	struct device *dev = device_get_binding(CONFIG_UNICAM_DEV_NAME);

	zassert_not_null(dev, "Failed to get unicam driver device handle!\n");

	/* Set CPU frequency to 1 GHz */
	clk_a7_set(MHZ(1000));

	/* Initialize LCD */
	lcd_init();

	/* Init configuration structure */
	memset(&cfg, 0, sizeof(cfg));

	/* Set bus type and serial mode */
#ifdef CONFIG_UNICAM_PHYS_INTERFACE_CSI
	cfg.bus_type = UNICAM_BUS_TYPE_SERIAL;
	cfg.serial_mode = UNICAM_MODE_CSI2;
#elif defined CONFIG_UNICAM_PHYS_INTERFACE_CCP
	cfg.bus_type = UNICAM_BUS_TYPE_SERIAL;
	cfg.serial_mode = UNICAM_MODE_CCP2;
#elif defined CONFIG_UNICAM_PHYS_INTERFACE_CPI
	cfg.bus_type = UNICAM_BUS_TYPE_PARALLEL;
#else
#warning "No unicam physical interfaces defined"
#endif

	cfg.capture_width = 640;
	cfg.capture_height = 480;
	cfg.capture_rate = 30;
	cfg.pixel_format = UNICAM_PIXEL_FORMAT_RAW8;
	cfg.frame_format = UNICAM_FT_PROGRESSIVE;
	cfg.capture_mode = UNICAM_CAPTURE_MODE_STREAMING;
	cfg.double_buff_en = double_buff_en;
	cfg.segment_height = double_buff_en ?
			     SEGMENT_HEIGHT : cfg.capture_height;

	num_segs = cfg.capture_height / cfg.segment_height;
	stride = (cfg.capture_width + 0xFF) & ~0xFF;

	ret = unicam_configure(dev, &cfg);
	zassert_true(ret == 0, "Unicam configure failed\n");

	ret = unicam_start_stream(dev);
	zassert_true(ret == 0, "Unicam start_stream failed\n");

	/* Get one segment at a time */
	time = k_uptime_get_32();
	time += 600000; /* 10 minutes */

	/* Allocate segment buffer - width needs to be aligned to 256 bytes */
#ifdef CONFIG_ZBAR
	/* Extra memory for aligning the width to 256 bytes is only needed for
	 * one segment.
	 */
	img_buff.length = cfg.capture_width * cfg.capture_height +
			  (stride - cfg.capture_width) * cfg.segment_height;
	buffer = capture_buffer;
	zassert_true(sizeof(capture_buffer) >= img_buff.length,
		     "Capture buffer size too small");
#else
	img_buff.length = stride * cfg.segment_height;
	buffer = k_malloc(img_buff.length);
	zassert_not_null(buffer, "Buffer allocation failed");
#endif
	memset(buffer, 0, img_buff.length);

	/* Get image segments and display them */
	count = 0;
	TC_PRINT("Image capture started ... Check LCD for live stream\n");
	do {
		u32_t i, j, index = cfg.capture_width;

		k_sleep(1);

#ifdef CONFIG_ZBAR
		img_buff.buffer = buffer +
			(count * cfg.segment_height * cfg.capture_width);
#else
		img_buff.buffer = buffer;
#endif
		ret = unicam_get_frame(dev, &img_buff);
		if (ret) {
			if (ret == -EAGAIN)
				continue;
			else
				break;
		}

		/* Remove stride alignment padding */
		for (i = 1; i < cfg.segment_height; i++)
			for (j = 0; j < cfg.capture_width; j++)
				*((u8_t *)(img_buff.buffer) + index++) =
				    *((u8_t *)(img_buff.buffer) + i*stride + j);

		/* Got an image segment, display it */
		if (img_buff.segment_num == count) {
			display_image_segment(img_buff.buffer,
					      img_buff.width,
					      img_buff.height,
					      img_buff.segment_num,
					      cfg.capture_width);

			if (count == 0)
				TC_PRINT(".");
			count++;
#ifdef CONFIG_ZBAR
			/* Got all segments try and decode it */
			if (count == num_segs) {
				/* Try decoding the QR scan */
				ret = zbarimg_scan_one_y8_image(
					buffer, cfg.capture_width,
					cfg.capture_height, output, &len);

				if (ret >= 1) {
					TC_PRINT("Decoded: '%s', len=%d\n",
						 output, len);
					break;
				}

				TC_PRINT("Failed to decode, ret: %d\n", ret);
			}
#endif
			count %= num_segs;
		}
	} while (time > k_uptime_get_32());

#ifndef CONFIG_ZBAR
	k_free(buffer);
#endif
	ret = unicam_stop_stream(dev);
	zassert_true(ret == 0, "Unicam stop_stream failed\n");
}

SHELL_TEST_DECLARE(test_unicam)
{
	/* Enable double buffering by default */
	double_buff_en = true;

	if ((argc == 2) && (strcmp(argv[1], "-single_buff") == 0))
		double_buff_en = false;

	TC_PRINT("Running unicam test in %s buffer mode\n",
		 double_buff_en ? "Double" : "Single");

	ztest_test_suite(unicam_driver_test,
			 ztest_unit_test(test_unicam_driver));

	ztest_run_test_suite(unicam_driver_test);

	return 0;
}
