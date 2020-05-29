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

/* @file spi_bcm58202_legacy.c
 *
 * SPI driver implementing the legacy SPI api
 *
 * This driver provides apis to access the serial peripheral interface as
 * defined in spi_legacy.h
 *
 */

#include <board.h>
#include <spi_legacy.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <pinmux.h>
#include <broadcom/gpio.h>
#include <genpll.h>

#include "spi_bcm58202_regs.h"

/* SPI default speed - 1MHz */
#define DEFAULT_SPI_SPEED_HZ		1000000

/* Clock divisor limits */
#define MIN_SPI_SCR_VAL		0
#define MAX_SPI_SCR_VAL		255
#define MIN_CPSDVR_VAL		2
#define MAX_CPSDVR_VAL		254

#define INT_DIV_ROUND_UP(NUM, DEN)	(((NUM) + (DEN) - 1)/(DEN))

#define SPI_BUSY_WAIT_TIMEOUT		1000

/* Number of pins to be routed for SPI */
#define SPI_PIN_COUNT	4	/* Clk, MOSI, MISO, C_Sel */

#define SPI_PIN_CLOCK	0
#define SPI_PIN_CSEL	1
#define SPI_PIN_MISO	2
#define SPI_PIN_MOSI	3

struct pinmux {
	u8_t pin;
	u8_t val;
};

struct spi_dev_data {
	/* Register base address */
	u32_t	base;
	/* pinmux settings to route CLK/MISO/MOSI/CS */
	struct pinmux spi_pins[SPI_PIN_COUNT];

	/* GPIO device handle */
	struct device *gpio_dev;
	/* Chip sel GPIO pin number */
	u8_t csel_gpio_pin;

	/* Transfer data size in bits */
	u8_t data_size;
};

#ifdef CONFIG_SPI_LEGACY_API
#if defined (CONFIG_SPI1_ENABLE) || \
	defined (CONFIG_SPI2_ENABLE) || \
	defined (CONFIG_SPI3_ENABLE) || \
	defined (CONFIG_SPI4_ENABLE) || \
	defined (CONFIG_SPI5_ENABLE)
/**
 * @brief       Configure SPI control register 0
 * @details     This function computes the clock rate divisor required to run
 *		the SPI clock at the configured frequency. It also configures
 *		the clock polarity, clock phase and data width for the SPI trfr
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on Success
 * @return      -errno on Failure
 */
static int spi_config_sspcr0(struct device *dev, struct spi_config *cfg)
{
	int ret;
	u32_t i, min_error;
	u32_t clk_div, scr, cpsdvr, base, spi_src_clk, cpsdvr_div_2, value;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);

	base = dd->base;

	ret = clk_get_wtus(&spi_src_clk);
	if (ret) {
		SYS_LOG_ERR("Error getting spi source clock rate : %d", ret);
		return ret;
	}

	/* Get the scr (Serial clock rate) from clock frequency
	 * Clock rate is computed as FSSPCLK/(CPSDVR x (1+SCR))
	 * Where FSSPCLK - Source clock
	 *	 CPSDVR  - Clock Divisor (2 - 254, even values only)
	 *	 SCR     - Reg field to program (2 - 254, even values only)
	 * The goal here is to arrive at the values of CPSDVR and SCR
	 * such that value = (CPSDVR)*(1+SCR). To minimize the error, we
	 * iterate through all valiues of SCR and find, the value of CPSDVR, for
	 * which the error is least.
	 */
	min_error = ~0;
	scr = ~0;
	clk_div = spi_src_clk / cfg->max_sys_freq;
	for (i = MIN_SPI_SCR_VAL; i <= MAX_SPI_SCR_VAL; i++) {
		int error;

		cpsdvr_div_2 = INT_DIV_ROUND_UP(clk_div, 2*(1+i));

		error = cfg->max_sys_freq - spi_src_clk/(cpsdvr_div_2*2*(1+i));

		if ((error > 0) &&
		    (cpsdvr_div_2 >= MIN_CPSDVR_VAL/2) &&
		    (cpsdvr_div_2 <= MAX_CPSDVR_VAL/2)) {
			if (min_error > (unsigned long)error) {
				scr = i;
				cpsdvr = cpsdvr_div_2*2;
				min_error = error;
			}
		}
	}

	/* Frequency requested is too low to be achieved by the
	 * clock pre scaler and divisor.
	 */
	if (scr == ~0UL) {
		SYS_LOG_ERR("%u : frequency too low\n", cfg->max_sys_freq);
		return -EINVAL;
	}

	/* Program the clock divisor */
	sys_write32(cpsdvr, base + SPI_SSPCPSR_BASE);

	/* Determine sspcr0 setting */
	value = 0;
	SET_FIELD(value, SPI_SSPCR0, SCR, scr);

	/* Set clock phase */
	if (cfg->config & SPI_MODE_CPHA)
		SET_FIELD(value, SPI_SSPCR0, SPH, 0x1);

	/* Set clock polarity */
	if (cfg->config & SPI_MODE_CPOL)
		SET_FIELD(value, SPI_SSPCR0, SPO, 0x1);

	/* Set the word size */
	SET_FIELD(value, SPI_SSPCR0, DSS,
		  SPI_WORD_SIZE_GET(cfg->config) - 1);

	/* Program controller register 0 */
	sys_write32(value, base + SPI_SSPCR0_BASE);

	return ret;
}

/**
 * @brief       Configure SPI control register 1
 * @details     This function configures the the master/slave mode and loop back
 *		mode for the SPI controller
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      N/A
 */
static void spi_config_sspcr1(struct device *dev, struct spi_config *cfg)
{
	u32_t value, base;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);

	base = dd->base;

	/* Program control register 1 */
	value = 0;
	SET_FIELD(value, SPI_SSPCR1, SSE, 1);
	if (cfg->config & SPI_MODE_LOOP)
		SET_FIELD(value, SPI_SSPCR1, LBM, 0x1);

	sys_write32(value, base + SPI_SSPCR1_BASE);
}

/**
 * @brief       Configure SPI pad MFIO control
 * @details     This functions configures the MFIO mode to route the SPI pins to
 *		output and also configures the pad control settings (hysteresis,
 *		driver stength, slew rate, pull up/down etc ...)
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on Success
 * @return      -errno on Failure
 */
static int spi_config_mfio_pad_control(struct device *dev,
				       struct spi_config *cfg)
{
	u8_t i;
	int ret;
	u32_t val, func, pin;
	struct device *pinmux_dev;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);

	/* Setup cs gpio direction */
	ret = gpio_pin_configure(dd->gpio_dev,
				 (dd->csel_gpio_pin & GPIO_PIN_COUNT_MASK),
				 GPIO_DIR_OUT);

	/* Configure pin muxing */
	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	for (i = 0; i < SPI_PIN_COUNT; i++) {
		pin = dd->spi_pins[i].pin;
		pinmux_pin_get(pinmux_dev, pin, &val);

		func = (cfg->config >> VENDOR_INFO_BIT_OFFSET) |
			dd->spi_pins[i].val;
		if (val != func) {
			ret = pinmux_pin_set(pinmux_dev, pin, func);
			if (ret) {
				SYS_LOG_ERR("Error: pin_set failed : %d -> %d",
					    pin, dd->spi_pins[i].val);
				return ret;
			}
		}
	}

	if (ret)
		return ret;

	ret = gpio_pin_write(dd->gpio_dev,
			     (dd->csel_gpio_pin & GPIO_PIN_COUNT_MASK),
			     0x1);
	return ret;
}

/**
 * @brief       Initialize SPI controller
 * @details     This api initializes the SPI controller with configuration
 *		specified. It also configures the MFIO pad control for SPI pins.
 *		The rx fifo is drained as part of the controller initialization.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_init(struct device *dev)
{
	u32_t base;
	int ret = 0, i;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);
	struct spi_config *cfg =
		(struct spi_config *)(dev->config->config_info);

	base = dd->base;

	/* Populate device data structure for SPI driver (gpio_dev & data_size)
	 */

	/* Get the GPIO device handle for chip select gpio pin
	 * Mask lower 5 bits (32 pin per gpio group) to get pin group
	 */
	switch (dd->csel_gpio_pin & ~0x1F) {
	default:
	case 0x0:
		dd->gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
		break;
	case 0x20:
		dd->gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
		break;
	case 0x40:
		dd->gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
		break;
	}

	dd->data_size = SPI_WORD_SIZE_GET(cfg->config);

	/* Disable SSE before reconfiguring the SPI controller */
	sys_write32(sys_read32(base + SPI_SSPCR1_BASE) & ~BIT(SPI_SSPCR1__SSE),
		    base + SPI_SSPCR1_BASE);

	/* Setup config reg 0 (clock rate/divisor, clock polarity/phase,
	 * data width
	 */
	ret = spi_config_sspcr0(dev, cfg);
	if (ret)
		return ret;

	/* Setup config reg 1 (master/slave mode, loop back mode */
	spi_config_sspcr1(dev, cfg);

	/* Setup pad control for SPI pins */
	ret = spi_config_mfio_pad_control(dev, cfg);
	if (ret)
		return ret;

	/* Wait for any pending transfers to finish so the tx fifo is empty */
	for (i = 0; i < SPI_BUSY_WAIT_TIMEOUT; i++)
		if (TX_FIFO_EMPTY(dd))
			break;

	/* Drain the receive FIFO */
	while (RX_NOT_EMPTY(dd))
		sys_read32(base + SPI_SSPDR_BASE);

	return ret;
}

/**
 * @brief       SPI busy wait
 * @details     This api blocks for a timeout period or till SPI bus is idle
 *		whichever comes early and returns the SPI bus status
 *
 * @param[in]   dev - Pointer to the device driver data for the driver instance
 *
 * @return      0 if SPI bus is idle
 * @return      1 if SPI bus is busy
 */
static int spi_bus_wait_timeout(struct spi_dev_data *dd)
{
	u32_t i;

	for (i = 0; i < SPI_BUSY_WAIT_TIMEOUT; i++)
		if (!(SPI_BUS_BUSY(dd)))
			return 0;

	return 1;
}

/**
 * @brief       Configure SPI controller
 * @details     This api configures the SPI controller with the settings
 *		in spi_config structure
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *		cfg - Pointer to SPI configuration structure
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_configure(struct device *dev,
				  struct spi_config *cfg)
{
	int ret, i;
	u32_t base;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);

	base = dd->base;

	if (cfg == NULL)
		return -EINVAL;

	dd->data_size = SPI_WORD_SIZE_GET(cfg->config);

	/* Check if SPI is busy, wait for a timeout and return error
	 * if SPI bus remains busy
	 */
	if (spi_bus_wait_timeout(dd))
		return -EBUSY;

	/* Disable SSE before reconfiguring the SPI controller */
	sys_write32(sys_read32(base + SPI_SSPCR1_BASE) & ~BIT(SPI_SSPCR1__SSE),
		    base + SPI_SSPCR1_BASE);

	/* Reconfigure the SPI controller & pad control for SPI pins */
	ret = spi_config_sspcr0(dev, cfg);
	if (ret)
		return ret;
	spi_config_sspcr1(dev, cfg);

	ret = spi_config_mfio_pad_control(dev, cfg);
	if (ret)
		return ret;

	/* Wait for any pending transfers to finish so the tx fifo is empty */
	for (i = 0; i < SPI_BUSY_WAIT_TIMEOUT; i++)
		if (TX_FIFO_EMPTY(dd))
			break;

	/* Drain the receive FIFO */
	while (RX_NOT_EMPTY(dd))
		sys_read32(dd->base + SPI_SSPDR_BASE);

	return 0;
}

/**
 * @brief       SPI slave selection
 * @details     This api selects the slave for the SPI interface
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   slave - Slave to be selected
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_slave_select(struct device *dev, u32_t slave)
{
	struct spi_dev_data *dd = (struct spi_dev_data *)dev->driver_data;

	return gpio_pin_write(dd->gpio_dev,
			      (dd->csel_gpio_pin & GPIO_PIN_COUNT_MASK),
			      slave ? 1 : 0);
}

/**
 * @brief       SPI transmit/receive data
 * @details     This api transfers and receives data on the SPI bus
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 *		tx_bufs - Transmit buffer
 *		tx_count - Number of elements to send on MOSI
 *		rx_bufs - Receive buffer
 *		rx_count - Number of elements to receive on MISO
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_transceive(struct device *dev,
				   const void *tx_buf,
				   u32_t tx_buf_len,
				   void *rx_buf,
				   u32_t rx_buf_len)
{
	void *tx, *rx;
	u32_t stride, data, key;
	u32_t tx_count, rx_count;
	struct spi_dev_data *dd = (struct spi_dev_data *)dev->driver_data;

	/* Word size in bits rounded up to bytes */
	stride = (dd->data_size + 0x7) >> 3;

	/* Check params */
	if ((tx_buf == NULL) && (rx_buf == NULL))
		return -EINVAL;

	if ((tx_buf == NULL) && (tx_buf_len != 0))
		return -EINVAL;

	if ((rx_buf == NULL) && (rx_buf_len != 0))
		return -EINVAL;

	if ((tx_buf_len == 0) && (rx_buf_len == 0))
		return -EINVAL;

	tx = (void *)tx_buf;
	rx = rx_buf;

	tx_count = tx_buf_len / stride;
	rx_count = rx_buf_len / stride;

	key = irq_lock();
	/* Loop as long as there is something to send or receive */
	while (tx_count || rx_count) {
		/* If tx fifo has space and rx */
		data = 0x0;
		if (tx_count) {
			data = (stride == 1) ? *(u8_t *)tx : *(u16_t *)tx;
			tx = (void *)((u32_t)tx + stride);
			tx_count--;
		}

		/* Wait till both tx and rx fifos has space for one entry
		 * before writing
		 */
		while ((TX_NOT_FULL(dd) == 0) || RX_FIFO_FULL(dd))
			;
		sys_write32(data, dd->base + SPI_SSPDR_BASE);

		/* Wait for Rx fifo to have atleast one data word before reading
		 */
		while (RX_NOT_EMPTY(dd) == 0)
			;
		data = sys_read32(dd->base + SPI_SSPDR_BASE);

		if (rx_count) {
			if (stride == 1)
				*(u8_t *)rx = (u8_t)(data & 0xFF);
			else
				*(u16_t *)rx = (u16_t)(data & 0xFFFF);

			rx = (void *)((u32_t)rx + stride);
			rx_count--;
		}
	}

	/* For SPI Write, wait until tx data has been sent out completely */
	if (tx_buf_len)
		while (TX_FIFO_EMPTY(dd) == 0)
			;
	irq_unlock(key);

	return 0;
}

/* SPI api */
static const struct spi_driver_api bcm58202_spi_api = {
	.configure = bcm58202_spi_configure,
	.slave_select = bcm58202_spi_slave_select,
	.transceive = bcm58202_spi_transceive,
};
#endif /* CONFIG_SPIx_ENABLE (x=1,2,3,4,5) */

#ifdef CONFIG_SPI1_ENABLE
/* SPI 1 device configuration and initialization */
static struct spi_config spi1_dev_cfg = {
	.max_sys_freq = DEFAULT_SPI_SPEED_HZ,
	.config = SPI_TRANSFER_MSB	|	/* MSB first */
		  SPI_WORD(8)			/* Word Size - 8 */
};

/* SPI 1 pin routing will conflict with QSPI quad mode pins
 * or SDIO pins. Currently, SPI1 is configured to use SDIO pins (53, 55, 56)
 * If we need SDIO pins then SPI1 driver will have to be disabled.
 */
static struct spi_dev_data spi1_dev_data = {
	.base = SPI1_BASE,
#ifdef CONFIG_SPI1_USES_MFIO_6_TO_9
	.spi_pins = {
			{6, 0}, /* Clock */
			{7, 3}, /* CSel */
			{8, 0}, /* MISO */
			{9, 0}  /* MOSI */
		},
	.csel_gpio_pin = 39
#else
	.spi_pins = {
			{53, 2}, /* Clock */
			{54, 0}, /* CSel */
			{55, 2}, /* MISO */
			{56, 2}  /* MOSI */
		},
	.csel_gpio_pin = 17
#endif
};

DEVICE_AND_API_INIT(spi1, CONFIG_SPI1_DEV_NAME,
		    bcm58202_spi_init, &spi1_dev_data,
		    &spi1_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI1_ENABLE */

#ifdef CONFIG_SPI2_ENABLE
/* SPI 2 device configuration and initialization */
static struct spi_config spi2_dev_cfg = {
	.max_sys_freq = DEFAULT_SPI_SPEED_HZ,
	.config = SPI_TRANSFER_MSB	|	/* MSB first */
		  SPI_WORD(8)		|	/* Word Size - 8 */
		  PAD_DRIVE_STRENGTH(0x4)	/* Drive Strength - 10 mA */
};

static struct spi_dev_data spi2_dev_data = {
	.base = SPI2_BASE,
#ifdef CONFIG_SPI2_USES_MFIO_10_TO_13
	.spi_pins = {
			{10, 0}, /* Clock */
			{11, 3}, /* CSel */
			{12, 0}, /* MISO */
			{13, 0}  /* MOSI */
		},
	.csel_gpio_pin = 43
#else
	.spi_pins = {
			{57, 2}, /* Clock */
			{58, 0}, /* CSel */
			{59, 2}, /* MISO */
			{60, 2}  /* MOSI */
		},
	.csel_gpio_pin = 21
#endif
};

DEVICE_AND_API_INIT(spi2, CONFIG_SPI2_DEV_NAME,
		    bcm58202_spi_init, &spi2_dev_data,
		    &spi2_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI2_ENABLE */

#ifdef CONFIG_SPI3_ENABLE
/* SPI 3 device configuration and initialization */
static struct spi_config spi3_dev_cfg = {
	.max_sys_freq = DEFAULT_SPI_SPEED_HZ,
	.config = SPI_TRANSFER_MSB	|	/* MSB first */
		  SPI_WORD(8)			/* Word Size - 8 */
};

static struct spi_dev_data spi3_dev_data = {
	.base = SPI3_BASE,
#ifdef CONFIG_SPI3_USES_MFIO_14_TO_17
	.spi_pins = {
			{14, 0}, /* Clock */
			{15, 3}, /* CSel */
			{16, 0}, /* MISO */
			{17, 0}  /* MOSI */
		},
	.csel_gpio_pin = 47
#else
	.spi_pins = {
			{61, 2}, /* Clock */
			{62, 0}, /* CSel */
			{63, 2}, /* MISO */
			{64, 2}  /* MOSI */
		},
	.csel_gpio_pin = 25
#endif
};

DEVICE_AND_API_INIT(spi3, CONFIG_SPI3_DEV_NAME,
		    bcm58202_spi_init, &spi3_dev_data,
		    &spi3_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI3_ENABLE */

#ifdef CONFIG_SPI4_ENABLE
/* SPI 4 device configuration and initialization */
static struct spi_config spi4_dev_cfg = {
	.max_sys_freq = DEFAULT_SPI_SPEED_HZ,
	.config = SPI_TRANSFER_MSB	|	/* MSB first */
		  SPI_WORD(8)			/* Word Size - 8 */
};

static struct spi_dev_data spi4_dev_data = {
	.base = SPI4_BASE,
#ifdef CONFIG_SPI4_USES_MFIO_18_TO_21
	.spi_pins = {
			{18, 0}, /* Clock */
			{19, 3}, /* CSel */
			{20, 0}, /* MISO */
			{21, 0}  /* MOSI */
		},
	.csel_gpio_pin = 51
#else
	.spi_pins = {
			{65, 2}, /* Clock */
			{66, 0}, /* CSel */
			{67, 2}, /* MISO */
			{68, 2}  /* MOSI */
		},
	.csel_gpio_pin = 29
#endif
};

DEVICE_AND_API_INIT(spi4, CONFIG_SPI4_DEV_NAME,
		    bcm58202_spi_init, &spi4_dev_data,
		    &spi4_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI4_ENABLE */

#ifdef CONFIG_SPI5_ENABLE
/* SPI 5 device configuration and initialization
 * SPI5 is used for NFC and requires the hysteresis and drive strength to be set
 * by default as below.
 */
static struct spi_config spi5_dev_cfg = {
	.max_sys_freq = DEFAULT_SPI_SPEED_HZ,
	.config = SPI_TRANSFER_MSB	|	/* MSB first */
		  SPI_WORD(8)		|	/* Word Size - 8 */
		  PAD_HYS_EN(1)		|	/* Hysteresis enable */
		  PAD_DRIVE_STRENGTH(0x6)	/* Drive strength - 14 mA */
};

static struct spi_dev_data spi5_dev_data = {
	.base = SPI5_BASE,
	.spi_pins = {
			{22, 0}, /* Clock */
			{23, 3}, /* Csel */
			{24, 0}, /* MISO */
			{25, 0}  /* MOSI */
		},
	.csel_gpio_pin = 55
};

DEVICE_AND_API_INIT(spi5, CONFIG_SPI5_DEV_NAME,
		    bcm58202_spi_init, &spi5_dev_data,
		    &spi5_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI5_ENABLE */

#endif /* CONFIG_SPI_LEGACY_API */
