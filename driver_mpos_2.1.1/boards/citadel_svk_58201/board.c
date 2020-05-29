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
#include <board.h>
#include <init.h>
#include <arch.h>
#include <pinmux.h>
#include <broadcom/gpio.h>
#include <logging/sys_log.h>

/* FIXME: GPIO 100 configuring should be done by calling the GPIO
 * driver api. Once the GPIO driver is updated to handle pin 100,
 * this will be removed.
 *
 * NFC PULL UP GPIO on 58202 maps to GPIO 100 on 58201,
 * which is connected to the camera reset pin on 58201 SVK
 */
#define NFC_REG_PU_CTRL_ADDR    0x44098004

#define IOMUX_CTRL_SEL_MASK		0x7

#define IOMUX_CTRL0_SEL_PWM0		(0)
#define IOMUX_CTRL0_SEL_FSK		(2)
#define IOMUX_CTRL0_SEL_GPIO32		(3)

#define IOMUX_CTRL1_SEL_PWM1		(0)
#define IOMUX_CTRL1_SEL_GPIO33		(3)

#define IOMUX_CTRL2_SEL_QSPI_CLK	(0)
#define IOMUX_CTRL2_SEL_GPIO34		(3)

#define IOMUX_CTRL3_SEL_QSPI_CSN	(0)
#define IOMUX_CTRL3_SEL_GPIO35		(3)

#define IOMUX_CTRL4_SEL_QSPI_MOSI	(0)
#define IOMUX_CTRL4_SEL_QSPI_SIO0	(1)
#define IOMUX_CTRL4_SEL_GPIO36		(3)

#define IOMUX_CTRL5_SEL_QSPI_MISO	(0)
#define IOMUX_CTRL5_SEL_QSPI_SIO1	(1)
#define IOMUX_CTRL5_SEL_GPIO37		(3)

#define IOMUX_CTRL6_SEL_SPI1_SCLK	(0)
#define IOMUX_CTRL6_SEL_UART1_RX	(1)
#define IOMUX_CTRL6_SEL_GPIO38		(3)

#define IOMUX_CTRL7_SEL_SPI1_CS		(0)
#define IOMUX_CTRL7_SEL_UART1_TX	(1)
#define IOMUX_CTRL7_SEL_GPIO39		(3)

#define IOMUX_CTRL8_SEL_SPI1_MISO	(0)
#define IOMUX_CTRL8_SEL_UART1_CTS	(1)
#define IOMUX_CTRL8_SEL_QSPI_WPN	(2)
#define IOMUX_CTRL8_SEL_GPIO40		(3)

#define IOMUX_CTRL9_SEL_SPI1_MOSI	(0)
#define IOMUX_CTRL9_SEL_UART1_RTS	(1)
#define IOMUX_CTRL9_SEL_QSPI_HOLDN	(2)
#define IOMUX_CTRL9_SEL_GPIO41		(3)

#define IOMUX_CTRL10_SEL_SPI2_CLK	(0)
#define IOMUX_CTRL10_SEL_UART2_RX	(1)
#define IOMUX_CTRL10_SEL_GPIO42		(3)

#define IOMUX_CTRL11_SEL_SPI2_CS	(0)
#define IOMUX_CTRL11_SEL_UART2_TX	(1)
#define IOMUX_CTRL11_SEL_GPIO43		(3)

#define IOMUX_CTRL12_SEL_SPI2_MISO	(0)
#define IOMUX_CTRL12_SEL_UART2_CTS	(1)
#define IOMUX_CTRL12_SEL_FMIO0		(2)
#define IOMUX_CTRL12_SEL_GPIO44		(3)

#define IOMUX_CTRL13_SEL_SPI2_MOSI	(0)
#define IOMUX_CTRL13_SEL_UART2_RTS	(1)
#define IOMUX_CTRL13_SEL_FMIO1		(2)
#define IOMUX_CTRL13_SEL_GPIO45		(3)

#define IOMUX_CTRL14_SEL_SPI3_CLK	(0)
#define IOMUX_CTRL14_SEL_UART3_RX	(1)
#define IOMUX_CTRL14_SEL_GPIO46		(3)

#define IOMUX_CTRL15_SEL_SPI3_CS	(0)
#define IOMUX_CTRL15_SEL_UART3_TX	(1)
#define IOMUX_CTRL15_SEL_GPIO47		(3)

#define IOMUX_CTRL16_SEL_SPI3_MISO	(0)
#define IOMUX_CTRL16_SEL_UART3_CTS	(1)
#define IOMUX_CTRL16_SEL_UART2_DTR	(2)
#define IOMUX_CTRL16_SEL_GPIO48		(3)

#define IOMUX_CTRL17_SEL_SPI3_MOSI	(0)
#define IOMUX_CTRL17_SEL_UART3_RTS	(1)
#define IOMUX_CTRL17_SEL_UART2_DSR	(2)
#define IOMUX_CTRL17_SEL_GPIO49		(3)

#define IOMUX_CTRL18_SEL_SPI4_CLK	(0)
#define IOMUX_CTRL18_SEL_PWM2		(1)
#define IOMUX_CTRL18_SEL_QSPI_WPN	(2)
#define IOMUX_CTRL18_SEL_GPIO50		(3)

#define IOMUX_CTRL19_SEL_SPI4_CS	(0)
#define IOMUX_CTRL19_SEL_PWM3		(1)
#define IOMUX_CTRL19_SEL_QSPI_HOLDN	(2)
#define IOMUX_CTRL19_SEL_GPIO51		(3)

#define IOMUX_CTRL20_SEL_SPI4_MISO	(0)
#define IOMUX_CTRL20_SEL_PWM0		(1)
#define IOMUX_CTRL20_SEL_FMIO2		(2)
#define IOMUX_CTRL20_SEL_GPIO52		(3)

#define IOMUX_CTRL21_SEL_SPI4_MOSI	(0)
#define IOMUX_CTRL21_SEL_PWM1		(1)
#define IOMUX_CTRL21_SEL_FSK_OUT	(2)
#define IOMUX_CTRL21_SEL_GPIO53		(3)

#define IOMUX_CTRL22_SEL_SPI5_CLK	(0)
#define IOMUX_CTRL22_SEL_FMIO0		(1)
#define IOMUX_CTRL22_SEL_GPIO54		(3)

#define IOMUX_CTRL23_SEL_SPI5_CS	(0)
#define IOMUX_CTRL23_SEL_FMIO1		(1)
#define IOMUX_CTRL23_SEL_GPIO55		(3)

#define IOMUX_CTRL24_SEL_SPI5_MISO	(0)
#define IOMUX_CTRL24_SEL_FMIO2		(1)
#define IOMUX_CTRL24_SEL_GPIO56		(3)

#define IOMUX_CTRL25_SEL_SPI5_MOSI	(0)
#define IOMUX_CTRL25_SEL_GPIO57		(3)

#define IOMUX_CTRL26_SEL_UART0_RX	(0)
#define IOMUX_CTRL26_SEL_GPIO58		(3)

#define IOMUX_CTRL27_SEL_UART0_TX	(0)
#define IOMUX_CTRL27_SEL_GPIO59		(3)

#define IOMUX_CTRL28_SEL_UART0_CTS	(0)
#define IOMUX_CTRL28_SEL_UART2_DCD	(2)
#define IOMUX_CTRL28_SEL_GPIO60		(3)

#define IOMUX_CTRL29_SEL_UART0_RTS	(0)
#define IOMUX_CTRL29_SEL_UART2_RI	(2)
#define IOMUX_CTRL29_SEL_GPIO61		(3)

#define IOMUX_CTRL30_SEL_SC_CLK		(0)
#define IOMUX_CTRL30_SEL_GPIO62		(3)

#define IOMUX_CTRL31_SEL_SC_CMDVCC_L	(0)
#define IOMUX_CTRL31_SEL_GPIO63		(3)

#define IOMUX_CTRL32_SEL_SC_DETECT	(0)
#define IOMUX_CTRL32_SEL_GPIO64		(3)

#define IOMUX_CTRL33_SEL_SC_FCB		(0)
#define IOMUX_CTRL33_SEL_GPIO65		(3)

#define IOMUX_CTRL34_SEL_SC_IO		(0)
#define IOMUX_CTRL34_SEL_GPIO66		(3)

#define IOMUX_CTRL35_SEL_SC_VPP		(0)
#define IOMUX_CTRL35_SEL_GPIO67		(3)

#define IOMUX_CTRL36_SEL_SC_RST_L	(0)
#define IOMUX_CTRL36_SEL_GPIO68		(3)

#define IOMUX_CTRL37_SEL_GPIO0		(0)
#define IOMUX_CTRL37_SEL_SRAM_CE0_L	(1)

#define IOMUX_CTRL38_SEL_GPIO1		(0)
#define IOMUX_CTRL38_SEL_SRAM_CE1_L	(1)

#define IOMUX_CTRL39_SEL_GPIO2		(0)
#define IOMUX_CTRL39_SEL_SRAM_WE_L	(1)

#define IOMUX_CTRL40_SEL_GPIO3		(0)
#define IOMUX_CTRL40_SEL_SRAM_RE_L	(1)

#define IOMUX_CTRL41_SEL_GPIO4		(0)
#define IOMUX_CTRL41_SEL_SRAM_D0	(1)

#define IOMUX_CTRL42_SEL_GPIO5		(0)
#define IOMUX_CTRL42_SEL_SRAM_D1	(1)

#define IOMUX_CTRL43_SEL_GPIO6		(0)
#define IOMUX_CTRL43_SEL_SRAM_D2	(1)

#define IOMUX_CTRL44_SEL_GPIO7		(0)
#define IOMUX_CTRL44_SEL_SRAM_D3	(1)

#define IOMUX_CTRL45_SEL_GPIO8		(0)
#define IOMUX_CTRL45_SEL_SRAM_D4	(1)

#define IOMUX_CTRL46_SEL_GPIO9		(0)
#define IOMUX_CTRL46_SEL_SRAM_D5	(1)

#define IOMUX_CTRL47_SEL_GPIO10		(0)
#define IOMUX_CTRL47_SEL_SRAM_D6	(1)

#define IOMUX_CTRL48_SEL_GPIO11		(0)
#define IOMUX_CTRL48_SEL_SRAM_D7	(1)

#define IOMUX_CTRL49_SEL_GPIO12		(0)
#define IOMUX_CTRL49_SEL_SRAM_A0	(1)

#define IOMUX_CTRL50_SEL_GPIO13		(0)
#define IOMUX_CTRL50_SEL_SRAM_A1	(1)

#define IOMUX_CTRL51_SEL_GPIO14		(0)
#define IOMUX_CTRL51_SEL_SRAM_A2	(1)

#define IOMUX_CTRL52_SEL_GPIO15		(0)
#define IOMUX_CTRL52_SEL_SRAM_A3	(1)

#define IOMUX_CTRL53_SEL_GPIO16		(0)
#define IOMUX_CTRL53_SEL_SRAM_A4	(1)
#define IOMUX_CTRL53_SEL_SPI1_CLK	(2)
#define IOMUX_CTRL53_SEL_SDIO_EMMC_RST	(3)

#define IOMUX_CTRL54_SEL_GPIO17		(0)
#define IOMUX_CTRL54_SEL_SRAM_A5	(1)
#define IOMUX_CTRL54_SEL_SPI1_CS	(2)
#define IOMUX_CTRL54_SEL_SDIO_LED	(3)

#define IOMUX_CTRL55_SEL_GPIO18		(0)
#define IOMUX_CTRL55_SEL_SRAM_A6	(1)
#define IOMUX_CTRL55_SEL_SPI1_MISO	(2)
#define IOMUX_CTRL55_SEL_SDIO_CLK	(3)

#define IOMUX_CTRL56_SEL_GPIO19		(0)
#define IOMUX_CTRL56_SEL_SRAM_A7	(1)
#define IOMUX_CTRL56_SEL_SPI1_MOSI	(2)
#define IOMUX_CTRL56_SEL_SDIO_CMD	(3)

#define IOMUX_CTRL57_SEL_GPIO20		(0)
#define IOMUX_CTRL57_SEL_SRAM_A8	(1)
#define IOMUX_CTRL57_SEL_SPI2_CLK	(2)
#define IOMUX_CTRL57_SEL_SDIO_DAT0	(3)

#define IOMUX_CTRL58_SEL_GPIO21		(0)
#define IOMUX_CTRL58_SEL_SRAM_A9	(1)
#define IOMUX_CTRL58_SEL_SPI2_CS	(2)
#define IOMUX_CTRL58_SEL_SDIO_DAT1	(3)

#define IOMUX_CTRL59_SEL_GPIO22		(0)
#define IOMUX_CTRL59_SEL_SRAM_A10	(1)
#define IOMUX_CTRL59_SEL_SPI2_MISO	(2)
#define IOMUX_CTRL59_SEL_SDIO_DAT2	(3)

#define IOMUX_CTRL60_SEL_GPIO23		(0)
#define IOMUX_CTRL60_SEL_SRAM_A11	(1)
#define IOMUX_CTRL60_SEL_SPI2_MOSI	(2)
#define IOMUX_CTRL60_SEL_SDIO_DAT3	(3)

#define IOMUX_CTRL61_SEL_GPIO24		(0)
#define IOMUX_CTRL61_SEL_SRAM_A12	(1)
#define IOMUX_CTRL61_SEL_SPI3_CLK	(2)
#define IOMUX_CTRL61_SEL_SDIO_DAT4	(3)

#define IOMUX_CTRL62_SEL_GPIO25		(0)
#define IOMUX_CTRL62_SEL_SRAM_A13	(1)
#define IOMUX_CTRL62_SEL_SPI3_CS	(2)
#define IOMUX_CTRL62_SEL_SDIO_DAT5	(3)

#define IOMUX_CTRL63_SEL_GPIO26		(0)
#define IOMUX_CTRL63_SEL_SRAM_A14	(1)
#define IOMUX_CTRL63_SEL_SPI3_MISO	(2)
#define IOMUX_CTRL63_SEL_SDIO_DAT6	(3)

#define IOMUX_CTRL64_SEL_GPIO27		(0)
#define IOMUX_CTRL64_SEL_SRAM_A15	(1)
#define IOMUX_CTRL64_SEL_SPI3_MOSI	(2)
#define IOMUX_CTRL64_SEL_SDIO_DAT7	(3)

#define IOMUX_CTRL65_SEL_GPIO28		(0)
#define IOMUX_CTRL65_SEL_SRAM_A16	(1)
#define IOMUX_CTRL65_SEL_SPI4_CLK	(2)

#define IOMUX_CTRL66_SEL_GPIO29		(0)
#define IOMUX_CTRL66_SEL_SRAM_A17	(1)
#define IOMUX_CTRL66_SEL_SPI4_CS	(2)

#define IOMUX_CTRL67_SEL_GPIO30		(0)
#define IOMUX_CTRL67_SEL_SRAM_A18	(1)
#define IOMUX_CTRL67_SEL_SPI4_MISO	(2)

#define IOMUX_CTRL68_SEL_GPIO31		(0)
#define IOMUX_CTRL68_SEL_SRAM_A19	(1)
#define IOMUX_CTRL68_SEL_SPI4_MOSI	(2)

#define IOMUX_CTRL71_SEL_QSPI_CS1	(1)

/*
 * @brief Function to configure IOMUX setting for a given GPIO pin
 */
static void iomux_enable(struct device *dev, u32_t pin, u32_t val)
{
	u32_t func;
	int rc;

	rc = pinmux_pin_get(dev, pin, &func);
	if (rc) {
		SYS_LOG_ERR("Error reading pin %d", pin);
		return;
	}

	func &= ~IOMUX_CTRL_SEL_MASK;
	func |= val;
	rc = pinmux_pin_set(dev, pin, func);
	if (rc) {
		SYS_LOG_ERR("Error writing pin %d", pin);
		return;
	}
	SYS_LOG_DBG("Wrote %x to pin %d", func, pin);
}

/*
 * @brief Pinmux initialization for SVK 58201
 */
static void iomux_setup(void)
{
	struct device *dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
#ifdef CONFIG_BRCM_DRIVER_SDIO
	int i;
	struct device *gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
#endif

#ifdef CONFIG_BRCM_DRIVER_UNICAM
	/* Set GPIO 100 -> CAM_RST to high */
	sys_write32(0x40, NFC_REG_PU_CTRL_ADDR);
#endif

#ifdef CONFIG_SECONDARY_FLASH
	iomux_enable(dev, 71, IOMUX_CTRL71_SEL_QSPI_CS1);
#endif

#ifdef CONFIG_QSPI_QUAD_IO_ENABLED
	iomux_enable(dev, 18, IOMUX_CTRL18_SEL_QSPI_WPN);
	iomux_enable(dev, 19, IOMUX_CTRL19_SEL_QSPI_HOLDN);
#endif

#ifdef CONFIG_UART_NS16550
#ifdef CONFIG_UART_NS16550_PORT_1
	/* UART 1 */
	iomux_enable(dev, 6, IOMUX_CTRL6_SEL_UART1_RX);
	iomux_enable(dev, 7, IOMUX_CTRL7_SEL_UART1_TX);
	iomux_enable(dev, 8, IOMUX_CTRL8_SEL_UART1_CTS);
	iomux_enable(dev, 9, IOMUX_CTRL9_SEL_UART1_RTS);
#endif

#ifdef CONFIG_UART_NS16550_PORT_2
	/* UART 4 */
	iomux_enable(dev, 10, IOMUX_CTRL10_SEL_UART2_RX);
	iomux_enable(dev, 11, IOMUX_CTRL11_SEL_UART2_TX);
	iomux_enable(dev, 12, IOMUX_CTRL12_SEL_UART2_CTS);
	iomux_enable(dev, 13, IOMUX_CTRL13_SEL_UART2_RTS);
	iomux_enable(dev, 16, IOMUX_CTRL16_SEL_UART2_DTR);
	iomux_enable(dev, 17, IOMUX_CTRL17_SEL_UART2_DSR);
	iomux_enable(dev, 28, IOMUX_CTRL28_SEL_UART2_DCD);
	iomux_enable(dev, 29, IOMUX_CTRL29_SEL_UART2_RI);
#endif

#ifdef CONFIG_UART_NS16550_PORT_3
	/* UART 3 */
	iomux_enable(dev, 14, IOMUX_CTRL14_SEL_UART3_RX);
	iomux_enable(dev, 15, IOMUX_CTRL15_SEL_UART3_TX);
	iomux_enable(dev, 16, IOMUX_CTRL16_SEL_UART3_CTS);
	iomux_enable(dev, 17, IOMUX_CTRL17_SEL_UART3_RTS);
#endif

#ifdef CONFIG_UART_NS16550_PORT_0
	/* UART 0 */
	iomux_enable(dev, 26, IOMUX_CTRL26_SEL_UART0_RX);
	iomux_enable(dev, 27, IOMUX_CTRL27_SEL_UART0_TX);
	iomux_enable(dev, 28, IOMUX_CTRL28_SEL_UART0_CTS);
	iomux_enable(dev, 29, IOMUX_CTRL29_SEL_UART0_RTS);
#endif
#endif

#ifdef CONFIG_BRCM_DRIVER_SDIO
	/* SDIO */
	iomux_enable(dev, 53, IOMUX_CTRL53_SEL_SDIO_EMMC_RST);
	iomux_enable(dev, 54, IOMUX_CTRL54_SEL_SDIO_LED);
	iomux_enable(dev, 55, IOMUX_CTRL55_SEL_SDIO_CLK);
	iomux_enable(dev, 56, IOMUX_CTRL56_SEL_SDIO_CMD);
	iomux_enable(dev, 57, IOMUX_CTRL57_SEL_SDIO_DAT0);
	iomux_enable(dev, 58, IOMUX_CTRL58_SEL_SDIO_DAT1);
	iomux_enable(dev, 59, IOMUX_CTRL59_SEL_SDIO_DAT2);
	iomux_enable(dev, 60, IOMUX_CTRL60_SEL_SDIO_DAT3);
	iomux_enable(dev, 61, IOMUX_CTRL61_SEL_SDIO_DAT4);
	iomux_enable(dev, 62, IOMUX_CTRL62_SEL_SDIO_DAT5);
	iomux_enable(dev, 63, IOMUX_CTRL63_SEL_SDIO_DAT6);
	iomux_enable(dev, 64, IOMUX_CTRL64_SEL_SDIO_DAT7);

	/* Setup Drive strength for LED, CLK, and RST */
	gpio_pin_configure(gpio_dev, 16 & 0x1F, GPIO_DS_ALT_HIGH);
	gpio_pin_configure(gpio_dev, 17 & 0x1F, GPIO_DS_ALT_HIGH);
	gpio_pin_configure(gpio_dev, 18 & 0x1F, GPIO_DS_ALT_HIGH);

	/* GPIO 19 = CMD, GPIO[20:27] = DAT[0:7]
	 * Set drive strength and pull up for command
	 */
	for (i = 19; i <= 27; i++)
		gpio_pin_configure(gpio_dev, i, GPIO_DS_ALT_HIGH |
						GPIO_PUD_PULL_UP);
#endif
}

static int board_setup(struct device *device)
{
	ARG_UNUSED(device);

	iomux_setup();

	/* FIXME: Please use apropriate GPIO API
	 * as and when it is available.
	 *
	 * Note1: On BCM58202 the register NFC_REG_PU_CTRL_ADDR
	 * provides some power control for NFC die present on
	 * citadel chip, however on 58201 in the absence of NFC die,
	 * these bits are used as additional GPIOs.
	 *
	 * Note2: Currently GPIO 100 is used as default Camera
	 * reset control on BCM58201.
	 */
	/* Set GPIO 100 -> CAM_RST to high */
	sys_write32(0x40, NFC_REG_PU_CTRL_ADDR);

	return 0;
}

SYS_INIT(board_setup, PRE_KERNEL_1, CONFIG_SOC_CONFIG_PRIORITY);
