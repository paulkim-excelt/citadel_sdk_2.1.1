/*
 * Copyright (c) 2018 Broadcom Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board level pinmux initialization for 58201 SVK board
 *
 * This module provides routines to initialize board specific pinmux settings
 * for BCM58201 SVK board.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <arch/cpu.h>
#include <pinmux.h>
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

#define IOMUX_CTRL18_SEL_SPI4_CLK	(0)
#define IOMUX_CTRL18_SEL_PWM2		(1)
#define IOMUX_CTRL18_SEL_QSPI_WPN	(2)
#define IOMUX_CTRL18_SEL_GPIO50		(3)

#define IOMUX_CTRL19_SEL_SPI4_CS	(0)
#define IOMUX_CTRL19_SEL_PWM3		(1)
#define IOMUX_CTRL19_SEL_QSPI_HOLDN	(2)
#define IOMUX_CTRL19_SEL_GPIO51		(3)

#define IOMUX_CTRL71_SEL_QSPI_CS1	(1)

#define IOMUX_CTRL65_SEL_SPI4_CLK	(2)
#define IOMUX_CTRL66_SEL_GPIO_SS	(0)
#define IOMUX_CTRL67_SEL_SPI4_MISO	(2)
#define IOMUX_CTRL68_SEL_SPI4_MOSI	(2)

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
static int citadel_serp_cp_pinmux_init(struct device *arg)
{
	struct device *dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);

	ARG_UNUSED(arg);

#ifdef CONFIG_BRCM_DRIVER_UNICAM
	/* Set GPIO 100 -> CAM_RST to high */
	sys_write32(0x40, NFC_REG_PU_CTRL_ADDR);
#endif

#ifdef CONFIG_SECONDARY_FLASH
	iomux_enable(dev, 71, IOMUX_CTRL71_SEL_QSPI_CS1);
#endif

#if defined(CONFIG_QSPI_QUAD_IO_ENABLED) && defined(CONFIG_UART_NS16550_PORT_1)
#error "UART1 and QSPI signals are routed to the same I/O pins." \
	"They cannot be enabled simultaneously"
#endif

#ifdef CONFIG_QSPI_QUAD_IO_ENABLED
	iomux_enable(dev, 18, IOMUX_CTRL18_SEL_QSPI_WPN);
	iomux_enable(dev, 19, IOMUX_CTRL19_SEL_QSPI_HOLDN);
#endif

	return 0;
}

SYS_INIT(citadel_serp_cp_pinmux_init, PRE_KERNEL_1,
	 CONFIG_SOC_CONFIG_PRIORITY);
