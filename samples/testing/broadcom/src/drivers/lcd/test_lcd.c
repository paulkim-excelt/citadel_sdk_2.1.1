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

/* @file test_lcd.c
 *
 * Unit test file for BCM958201 LCD driver.
 *
 */

#include <test.h>
#include <errno.h>
#include <board.h>
#include <string.h>
#include <lcd_adafruit.h>
#include "dragon.h"

#define LCD_HEIGHT_MAX	320
#define LCD_WIDTH_MAX	240

/* Color definitions */
#define ILI9341_DARKGREEN	0x03E0      /*   0, 128,   0 */
#define ILI9341_YELLOW		0xFFE0      /* 255, 255,   0 */
#define ILI9341_ORANGE		0xFD20      /* 255, 165,   0 */

/* Commands */
#define ILI9341_DISPOFF	0x28	/* Turn Off Display */
#define ILI9341_DISPON	0x29	/* Turn On Display */
#define ILI9341_RDMODE	0x0A	/* Read Display Power Mode */

extern int lcd_board_configure(int mode);

static int glcd_mode;

void test_lcd_driver(void)
{
	int ret;
	u32_t w = LCD_WIDTH_MAX, h = LCD_HEIGHT_MAX, i, j;
	struct lcd_config cfg;
	struct device *dev = device_get_binding(CONFIG_LCD_DEV_NAME);

	zassert_not_null(dev, "Failed to get lcd driver device handle!\n");

	/* Init configuration structure */
	memset(&cfg, 0, sizeof(cfg));
	cfg.mode = glcd_mode;
	cfg.orientation = LANDSCAPE;

	/* Do board related settings */
	ret = lcd_board_configure(cfg.mode);
	zassert_true(ret == 0, "LCD board configure failed\n");
	TC_PRINT("LCD board settings.. Done\n");

	/* Initialize LCD */
	ret = lcd_configure(dev, &cfg);

	zassert_true(ret == 0, "LCD configure failed\n");
	TC_PRINT("LCD mode configured\n");

	/* Test display pixels (data write) by filling colors */
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			lcd_display_pixel(dev, i, j, ILI9341_DARKGREEN);
		}
	}
	k_busy_wait(2000000); /* 2 sec display time */

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			lcd_display_pixel(dev, i, j, ILI9341_YELLOW);
		}
	}
	k_busy_wait(2000000); /* 2 sec display time */

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			lcd_display_pixel(dev, i, j, ILI9341_ORANGE);
		}
	}
	k_busy_wait(2000000); /* 2 sec display time */

	lcd_display_clear(dev);
	k_busy_wait(100000); /* Wait a second */

	/* Display vertical line */
	lcd_draw_vline(dev, 50, 0, LCD_WIDTH_MAX, ILI9341_DARKGREEN);
	k_busy_wait(2000000); /* 2 sec display time */

	lcd_display_clear(dev);
	k_busy_wait(100000); /* Wait a second */

	/* Display horizontal line */
	lcd_draw_hline(dev, 0, 50, LCD_HEIGHT_MAX, ILI9341_DARKGREEN);
	k_busy_wait(2000000); /* 2 sec display time */

	lcd_display_clear(dev);
	k_busy_wait(100000); /* Wait a second */

	/* Display a bitmap
	 * Coordinates (105, 85) are arrived at considering this particular
	 * bitmap to fit in the center of the screen (for testing).
	 */
	lcd_draw_bitmap(dev, 105, 85, (u16_t *)dragonBitmap, DRAGON_WIDTH,
			DRAGON_HEIGHT);
	k_busy_wait(10000000); /* 10 sec display time */

	/* Test command write. Turn Display Off */
	ret = lcd_write_command(dev, ILI9341_DISPOFF);
	zassert_true(ret == 0, "lcd display clear failed\n");
}

SHELL_TEST_DECLARE(test_lcd)
{
	if ((argc == 2) && (strcmp(argv[1], "8080") == 0)) {
		glcd_mode = PARALLEL_MODE8080;
	} else if ((argc == 2) && (strcmp(argv[1], "spi") == 0)) {
		glcd_mode = SERIAL_SPI;
	} else if ((argc == 1) || (argc == 2)) {
		TC_PRINT("Pass argument 'spi' or '8080' to begin test\n");
		return 0;
	}

	TC_PRINT("Running lcd test in %s mode\n",
	 (glcd_mode == SERIAL_SPI) ? "serial (SPI)" : "parallel (8080)");

	ztest_test_suite(lcd_driver_test,
			 ztest_unit_test(test_lcd_driver));

	ztest_run_test_suite(lcd_driver_test);

	return 0;
}
