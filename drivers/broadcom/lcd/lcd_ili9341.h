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
#ifndef _LCD_ILI9341_H_
#define _LCD_ILI9341_H_

#ifdef CONFIG_LCD_SPI_MODE

/* Default spi device to spi1 */
#ifndef CONFIG_LCD_SPI_DEV
#define CONFIG_LCD_SPI_DEV CONFIG_SPI1_DEV_NAME
#endif

/* Define SPI mode DC GPIO */
#ifdef CONFIG_LCD_SPI_MODE_DC_GPIO
#define DC_GPIO_SPI_MODE	CONFIG_LCD_SPI_MODE_DC_GPIO
#else
/* Default to SVK pin */
#define DC_GPIO_SPI_MODE	2
#endif /* CONFIG_LCD_SPI_MODE_DC_GPIO */

#endif /* CONFIG_LCD_SPI_MODE */

/* Define 8080 mode DC GPIO */
#ifdef CONFIG_LCD_8080_MODE_DC_GPIO
#define DC_GPIO_PARALLEL_MODE	CONFIG_LCD_8080_MODE_DC_GPIO
#else
/* Default to SVK pin */
#define DC_GPIO_PARALLEL_MODE	12
#endif /* CONFIG_LCD_8080_MODE_DC_GPIO */

#define LCD_SPI_FREQ		MHZ(15)
/* FIXME: Get this delay based on SMC clock?
 * This delay is based on SMC clock running at 100MHz.
 */
#define LCD_SMC_DELAY		250

struct lcd_data {
	struct device *spi_dev;
#ifdef CONFIG_LCD_SPI_MODE
	struct device *gpio_dev_spi;
#endif
	struct device *gpio_dev_8080;
	struct device *smc_dev;
	u8_t cur_mode;
	bool spi_mode_en;
};

/* Parallel 8080 mode defines */
#define SMC_LCD_CMD_ADDR		0x61000000
#define SMC_LCD_DATA_ADDR		0x61000001

#define CDRU_SW_RESET_CTRL		0x3001d090
#define SMC_SW_RST			22
#define CDRU_SMC_ADDRESS_MATCH_MASK	0x3001d1e8
#define SMC_MATCH_MASK_VAL		0x61E8FFFF
#define set_cycles			0x53000014
#define SRAM_CYCLES_VAL			0x692AA
#define set_opmode			0x53000018
#define direct_cmd			0x53000010
#define CMD_TYPE_SHIFT			21
#define CMD_TYPE			2
#define CHIP_NUM_SHIFT			23
#define CHIP_NUM			1
#define REG_FN_MOD_AHB			0x55042028
#define opmode0_0			0x53000104
#define opmode0_1			0x53000124

/* ILI9431 LCD defines */
#define ILI9341_TFTWIDTH   240
#define ILI9341_TFTHEIGHT  320

#define ILI9341_NOP        0x00
#define ILI9341_SWRESET    0x01
#define ILI9341_RDDID      0x04
#define ILI9341_RDDST      0x09

#define ILI9341_SLPIN      0x10
#define ILI9341_SLPOUT     0x11
#define ILI9341_PTLON      0x12
#define ILI9341_NORON      0x13

#define ILI9341_RDMODE     0x0A
#define ILI9341_RDMADCTL   0x0B
#define ILI9341_RDPIXFMT   0x0C
#define ILI9341_RDIMGFMT   0x0D
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF     0x20
#define ILI9341_INVON      0x21
#define ILI9341_GAMMASET   0x26
#define ILI9341_DISPOFF    0x28
#define ILI9341_DISPON     0x29

#define ILI9341_CASET      0x2A
#define ILI9341_PASET      0x2B
#define ILI9341_RAMWR      0x2C
#define ILI9341_RAMRD      0x2E

#define ILI9341_PTLAR      0x30
#define ILI9341_MADCTL     0x36
#define ILI9341_VSCRSADD   0x37
#define ILI9341_PIXFMT     0x3A

#define ILI9341_FRMCTR1    0xB1
#define ILI9341_FRMCTR2    0xB2
#define ILI9341_FRMCTR3    0xB3
#define ILI9341_INVCTR     0xB4
#define ILI9341_DFUNCTR    0xB6

#define ILI9341_PWCTR1     0xC0
#define ILI9341_PWCTR2     0xC1
#define ILI9341_PWCTR3     0xC2
#define ILI9341_PWCTR4     0xC3
#define ILI9341_PWCTR5     0xC4
#define ILI9341_VMCTR1     0xC5
#define ILI9341_VMCTR2     0xC7

#define ILI9341_RDID1      0xDA
#define ILI9341_RDID2      0xDB
#define ILI9341_RDID3      0xDC
#define ILI9341_RDID4      0xDD

#define ILI9341_GMCTRP1    0xE0
#define ILI9341_GMCTRN1    0xE1
#define ILI9341_PWCTR6     0xFC

/* Color definitions */
#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F

/*
 * Configurable orientation parameters to be programmed in
 * Memory Access Control (MADCTL) register.
 */
#define MADCTL_MY	0x80 /* Bottom to top */
#define MADCTL_MX	0x40 /* Right to left */
#define MADCTL_MV	0x20 /* Reverse Mode */
#define MADCTL_ML	0x10 /* LCD refresh Bottom to top */
#define MADCTL_RGB	0x00 /* Red-Green-Blue pixel order */
#define MADCTL_BGR	0x08 /* Blue-Green-Red pixel order */
#define MADCTL_MH	0x04 /* LCD refresh right to left */

#endif /* _LCD_ILI9341_H_ */
