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

/* @file lcd_adafruit.h
 *
 * LCD Driver api header file.
 *
 * This driver provides apis to configure and display data on LCD.
 *
 */

#ifndef __LCD_ADAFRUIT_H__
#define __LCD_ADAFRUIT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** All platform specific headers are included here */
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <device.h>

/* Configurable LCD modes*/
enum lcd_mode {
	SERIAL_SPI, /* serial mode (SPI) */
	PARALLEL_MODE8080, /* 8-bit parallel mode (8080) */
	UNCONFIGURED,
};

enum lcd_orientation {
	LANDSCAPE,
	PORTRAIT,
};

struct lcd_config {
	/* Mode - SPI (serial) or Parallel 8-bit (smc) */
	u8_t mode;
	/* Orientation - Landscape/portrait */
	u8_t orientation;
};

typedef int (*lcd_api_configure)(struct device *dev,
				 struct lcd_config *config);
typedef u8_t (*lcd_api_read_data)(struct device *dev, u8_t cmd);
typedef int (*lcd_api_write_command)(struct device *dev, u8_t cmd);
typedef int (*lcd_api_write_data)(struct device *dev, u8_t val);
typedef void (*lcd_api_display_pixel)(struct device *dev, s16_t x, s16_t y,
				     u16_t val);
typedef void (*lcd_api_draw_vline)(struct device *dev, s16_t x, s16_t y,
				  s16_t h, u16_t val);
typedef void(*lcd_api_draw_hline)(struct device *dev, s16_t x, s16_t y,
				  s16_t w, u16_t val);
typedef void (*lcd_api_display_clear)(struct device *dev);
typedef void (*lcd_api_draw_bitmap)(struct device *dev, s16_t x, s16_t y,
				    u16_t *pcolors, s16_t w, s16_t h);

struct lcd_driver_api {
	lcd_api_configure configure;
	lcd_api_read_data read_data;
	lcd_api_write_command write_cmd;
	lcd_api_write_data write_data;
	lcd_api_draw_vline draw_vline;
	lcd_api_draw_hline draw_hline;
	lcd_api_display_pixel display_pixel;
	lcd_api_display_clear display_clear;
	lcd_api_draw_bitmap draw_bitmap;
};

/**
 * @brief Configure LCD mode and orientation parameters
 *
 * @details This API provides user options to configures lcd.
 *	    Configurable parameters are:
 *		mode - SPI(Serial) or SMC (Parallel)
 *		Orientation - landscape or Portrait
 *
 * @param Pointer to device structure
 * @param Pointer to lcd_config structure
 *
 * @return 0:OK; ERROR otherwise
 */
static inline int lcd_configure(struct device *dev,
				struct lcd_config *config)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->configure(dev, config);
}

/**
 * @brief Read values using supported LCD read commands
 *
 * @details This API provides user options to read
 *	    values by issuing LCD read commands as specified
 *	    in the data sheet.
 *
 * @param device structure
 * @param read command value
 *
 * @return u8_t value as requested by cmd; ERROR otherwise
 */
static inline u8_t lcd_read_data(struct device *dev, u8_t cmd)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->read_data(dev, cmd);
}

/**
 * @brief Write command to LCD device
 *
 * @details This API writes command to the LCD device.
 *	    There are commands as specified in lcd_ili9341.h
 *	    (also in sec 8.1 in)adafruit specification doc
 *	    "ILI9341.pdf") that can be used to control some LCD
 *	    settings like display on/off, invertion on/off,
 *	    idle mode on/off an so on. This API can be used for
 *	    such cases.
 *	    Some commands (e.g., MADCTL) can be used to program
 *	    values by doing a subsequent data write to modify
 *	    the current value.
 *
 * @param Pointer to device structure
 * @param command value
 *
 * @return 0:OK; ERROR otherwise
 */
static inline int lcd_write_command(struct device *dev, u8_t cmd)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->write_cmd(dev, cmd);
}

/**
 * @brief API to do bus write to LCD
 *
 * @details  This API can be used along with lcd_write_command api
 *	     to program values as described in ref manual.
 *
 * @param Pointer to device structure
 * @param data to be written
 *
 * @return 0:OK; ERROR otherwise
 */
static inline int lcd_write_data(struct device *dev, u8_t val)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->write_data(dev, val);
}

/* @brief API to draw vertical line on LCD
 *
 * @details Draws a vertical line on the LCD screen
 *
 * @param dev - Pointer to device structure
 *	  x - column coordinate for start of line
 *	  y - row coordinate for start of line
 *	  h - length of the line (should not be greater than
 *	      max height (240)
 *	 val - pixel value to be written (16-bit RGB565 format)
 *
 * @return nothing
 */
static inline void lcd_draw_vline(struct device *dev, s16_t x, s16_t y,
				 s16_t h, u16_t val)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->draw_vline(dev, x, y, h, val);
}

/* @brief API to draw horizontal line on LCD
 *
 * @details Draws a horizontal line on the LCD screen
 *
 * @param dev - Pointer to device structure
 *	  x - column coordinate for start of line
 *	  y - row coordinate for start of line
 *	  w - length of the line (should not be greater than
 *	      max width (320)
 *	 val - pixel value to be written (16-bit RGB565 format)
 *
 * @return nothing
 */
static inline void lcd_draw_hline(struct device *dev, s16_t x, s16_t y,
				 s16_t w, u16_t val)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->draw_hline(dev, x, y, w, val);
}

/* @brief API to display pixel on LCD
 *
 * @details Draws a pixel on the LCD screen
 *
 * @param dev - Pointer to device structure
 *	  x - column coordinate of the pixel
 *	  y - row coordinate of the pixel
 *	 val - pixel value to be written (16-bit RGB565 format)
 *
 * @return nothing
 */
static inline void lcd_display_pixel(struct device *dev, s16_t x, s16_t y,
				    u16_t val)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->display_pixel(dev, x, y, val);
}

/* @brief API to clear screen
 *
 * @details API to clear entire 240x320 area of the LCD display
 *
 * @param dev - Pointer to device structure
 *
 * @return nothing
 */
static inline void lcd_display_clear(struct device *dev)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->display_clear(dev);
}

/* @brief API to draw bitmap
 *
 * @details Draws bitmap on the LCD screen
 *
 * @param dev - Pointer to device structure
 *	  x - column coordinate for start of bitmap
 *	  y - row coordinate for start of bitmap
 *	  pcolors - pointer to RAW bitmap data (565RGB format)
 *	  h - bitmap height (should not be greater than
 *	      max height (240)
 *	  w - bitmap width (should not be greater than
 *	      max width (320)
 *
 * @return nothing
 */
static inline void lcd_draw_bitmap(struct device *dev, s16_t x, s16_t y,
				   u16_t *pcolors, s16_t w, s16_t h)
{
	struct lcd_driver_api *api;

	api = (struct lcd_driver_api *)dev->driver_api;
	return api->draw_bitmap(dev, x, y, pcolors, w, h);
}
#ifdef __cplusplus
}
#endif

#endif /* __LCD_ADAFRUIT_H__ */
