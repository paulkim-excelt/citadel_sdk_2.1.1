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

/* @file spi_bcm58202.c
 *
 * SPI driver
 *
 * This driver provides apis to access the serial peripheral interface
 *
 */

#ifndef CONFIG_SPI_LEGACY_API

#include <board.h>
#include <broadcom/spi.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <pinmux.h>
#include <broadcom/gpio.h>
#include <genpll.h>

#ifdef CONFIG_SPI_DMA_MODE
#include <broadcom/dma.h>
#include <string.h>
#endif

#include "spi_bcm58202_regs.h"

/* SPI default speed - 1MHz */
#define DEFAULT_SPI_SPEED_HZ		1000000

/* SPI fifo size in bytes */
#define FIFO_SIZE	64
/* TNF interrupt is generated when number of entries in Tx fifo
 * is less than this number. Similarly RNE interrupt is generated
 * when number of entries in Rx fifo is equal or more than this number.
 */
#define FIFO_WATER_MARK 4

/* Timeout multiplier */
#ifdef CONFIG_SPI_DMA_MODE
#define TIMEOUT_MULT	100
#else
#define TIMEOUT_MULT	100000
#endif

/* Maximum DMA transfer size (12 bit) */
#define MAX_DMA_SIZE	(BIT(12) - 1)

/* Clock divisor limits */
#define MIN_SPI_SCR_VAL		0
#define MAX_SPI_SCR_VAL		255
#define MIN_CPSDVR_VAL		2
#define MAX_CPSDVR_VAL		254

#define INT_DIV_ROUND_UP(NUM, DEN)	(((NUM) + (DEN) - 1)/(DEN))

#define SPI_BUSY_WAIT_TIMEOUT		10000

#define SPI_CFG_CHANGED(CFG, DEV_CFG)			\
	((CFG->frequency != DEV_CFG->frequency) ||	\
	 (CFG->operation != DEV_CFG->operation) ||	\
	 (CFG->vendor != DEV_CFG->vendor) ||		\
	 (CFG->slave != DEV_CFG->slave))

#define MULTI_BYTE_SLAVE_ISR

/* SPI device configuration
 * This information is cached to avoid reprogramming the
 * SPI registers on each SPI operation
 * See 'struct spi_config' in spi.h for details
 * on the structure members
 *
 * The vendor specific data holds the pad control function mask
 * that has to be applied to the SPI pins.
 */
struct spi_dev_config {
	u32_t	frequency;
	u16_t	operation;
	u16_t	vendor;
	u16_t	slave;
};

/* Number of pins to be routed for SPI */
#define SPI_PIN_COUNT	3	/* Clk, MOSI, MISO
				 * Chip select is passed through spi_cs_control
				 */

#define SPI_PIN_CLOCK	0
#define SPI_PIN_MISO	1
#define SPI_PIN_MOSI	2

struct pinmux {
	u8_t pin;
	u8_t val;
};

struct spi_dev_data {
	/* Register base address */
	u32_t	base;
	/* IRQ number */
	u32_t	irq;
	/* pinmux settings to route CLK/MISO/MOSI/CS */
	struct pinmux spi_pins[SPI_PIN_COUNT];
	/* SPI Driver access lock */
	struct k_sem lock;
	/* SPI lock can be released only if the config param is the same
	 * as in the last SPI api call
	 */
	void *last_cfg;
	/* Slave mode callback function */
	spi_slave_cb cb;
	/* Slave mode callback function argument */
	void *cb_arg;
	/* SPI Tx Rx Done Signal, Initially available */
	struct k_sem txrx_done;
	/* The buffer pointers and buffer indices */
	struct spi_buf *tx;
	struct spi_buf *rx;
	u32_t tx_count;
	u32_t rx_count;
	u32_t tx_offset;
	u32_t rx_offset;
	/* Number of bytes left to Tx in this Transaction, where
	 * a transaction involves 0 or 1 tx & rx blocks (with 0
	 * or more bytes).
	 * These variables are initialised when both Tx & Rx block
	 * offsets are 0. Actual Tx happens only when max_tx_len
	 * is non-zero.
	 */
	u32_t max_tx_len;
	/* Clock Frequency  */
	u32_t freq;
	/* Timeout in us to wait for th entire rx/tx fifo to drain empty */
	u32_t fifo_empty_timeout;
	/* Timeout to wait for one data unit to be sent/received */
	u32_t tx_rx_timeout;
#ifdef CONFIG_SPI_DMA_MODE
	/* DMA device handle */
	struct device *dma_dev;
	/* Semaphore to indicate DMA complete */
	struct k_sem dma_complete;
	/* DMA peripheral IDs */
	u8_t tx_dma_id;
	u8_t rx_dma_id;
	/* DMA done callback */
	void (*dma_done_cb)(struct device *dev, u32_t ch, int error);
#endif
};

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
static int spi_config_sspcr0(struct device *dev)
{
	int ret;
	u32_t i, min_error;
	u32_t clk_div, scr, cpsdvr, base, spi_src_clk, cpsdvr_div_2, value;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);
	struct spi_dev_config *cfg
		 = (struct spi_dev_config *)(dev->config->config_info);

	u32_t stride = (SPI_WORD_SIZE_GET(cfg->operation) + 0x7) >> 3;

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
	clk_div = spi_src_clk / cfg->frequency;
	for (i = MIN_SPI_SCR_VAL; i <= MAX_SPI_SCR_VAL; i++) {
		int error;

		cpsdvr_div_2 = INT_DIV_ROUND_UP(clk_div, 2*(1+i));

		error = cfg->frequency - spi_src_clk/(cpsdvr_div_2*2*(1+i));

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
		SYS_LOG_ERR("%u : frequency too low\n", cfg->frequency);
		return -EINVAL;
	}

	/* Save actual clock frequency */
	dd->freq = spi_src_clk/(cpsdvr*(1+scr));

	/* Compute timeouts */
	dd->fifo_empty_timeout =
		((MHZ(1) * 8 * FIFO_SIZE) / dd->freq + 1) * TIMEOUT_MULT;
	dd->tx_rx_timeout =
		((MHZ(1) * 8 * stride) / dd->freq + 1) * TIMEOUT_MULT;

	/* Program the clock divisor */
	sys_write32(cpsdvr, base + SPI_SSPCPSR_BASE);

	/* Determine sspcr0 setting */
	value = 0;
	SET_FIELD(value, SPI_SSPCR0, SCR, scr);

	/* Set clock phase */
	if (cfg->operation & SPI_MODE_CPHA)
		SET_FIELD(value, SPI_SSPCR0, SPH, 0x1);

	/* Set clock polarity */
	if (cfg->operation & SPI_MODE_CPOL)
		SET_FIELD(value, SPI_SSPCR0, SPO, 0x1);

	/* Set the word size */
	SET_FIELD(value, SPI_SSPCR0, DSS,
		  SPI_WORD_SIZE_GET(cfg->operation) - 1);

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
static void spi_config_sspcr1(struct device *dev)
{
	u32_t value, base;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);
	struct spi_dev_config *cfg
		= (struct spi_dev_config *)(dev->config->config_info);

	base = dd->base;

	/* Program control register 1 */
	value = 0;
	if (cfg->operation & SPI_OP_MODE_SLAVE)
		SET_FIELD(value, SPI_SSPCR1, MS, 0x1);
	if (cfg->operation & SPI_MODE_LOOP)
		SET_FIELD(value, SPI_SSPCR1, LBM, 0x1);

	sys_write32(value, base + SPI_SSPCR1_BASE);

	/* SSE can be set only after setting MS bit */
	SET_FIELD(value, SPI_SSPCR1, SSE, 1);
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
static int spi_config_mfio_pad_control(struct device *dev)
{
	u8_t i;
	int ret;
	struct device *pinmux_dev;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);
	struct spi_dev_config *cfg
		= (struct spi_dev_config *)(dev->config->config_info);

	/* Configure pin muxing and directions */
	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	for (i = 0; i < SPI_PIN_COUNT; i++) {
		ret = pinmux_pin_set(pinmux_dev, dd->spi_pins[i].pin,
				     cfg->vendor | dd->spi_pins[i].val);
		if (ret) {
			SYS_LOG_ERR("Error setting MFIO pad control: %d -> %d",
				    dd->spi_pins[i].pin, dd->spi_pins[i].val);
			return ret;
		}
	}

	return 0;
}

/**
 * @brief       Drain SPI TX/RX Fifos
 * @details     This api waits for the TX fifo to be flushed and drains the RX
 *		fifo. This api should be called when there is a configuration
 *		change to the SPI controller, to ensure a clean state before
 *		starting the SPI transactions following the configuration change
 *
 * @param[in]   dd - Pointer to the SPI driver data for the driver instance
 *
 * @return      N/A
 */
void drain_fifos(struct spi_dev_data *dd)
{
	int i;

	/* Wait for any pending transfers to finish so the tx fifo is empty */
	for (i = 0; i < SPI_BUSY_WAIT_TIMEOUT; i++)
		if (TX_FIFO_EMPTY(dd))
			break;

	/* Drain the receive FIFO */
	while (RX_NOT_EMPTY(dd))
		sys_read32(dd->base + SPI_SSPDR_BASE);
}

/**
 * @brief       Flush SPI TX Fifo
 * @details     This is a WAR to flush the TX fifo using
 *  	internal Loop Back Mode (by first putting the controller
 *  	in LBM and then initiating an Rx Transaction). This is
 *  	useful in error situations where the slave's tx fifo is
 *  	left in a non-empty state and there is no master clock
 *  	to drain it out. This is to address a hardware issue
 *  	where in disabling and re-enabling the SSE doesn't
 *  	completely empty the tx fifo and the very first data
 *  	element is found to be still present even after
 *  	re-initialization of the SPI device. This api should be
 *  	called when there is a configuration change to the SPI
 *  	controller, to ensure an empty tx fifo before starting
 *  	the SPI transactions following the configuration change.
 *  	Note: This function leaves the controller in disabled
 *  	state, so it should be called with the controller
 *  	disabled.
 *
 * @param[in]   dd - Pointer to the SPI driver data for the driver instance
 *
 * @return      N/A
 */
static void flush_txfifo_using_lbm(struct spi_config *config)
{
	u32_t value = 0;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	/* Reconfigure the SPI controller */
	spi_config_sspcr0(config->dev);
	/* WAR: Set LBM to flush out the Slave Tx FIFO */
	SET_FIELD(value, SPI_SSPCR1, LBM, 0x1);
	sys_write32(value, dd->base + SPI_SSPCR1_BASE);
	/* SSE can be set only after setting MS bit */
	SET_FIELD(value, SPI_SSPCR1, SSE, 1);
	sys_write32(value, dd->base + SPI_SSPCR1_BASE);
	drain_fifos(dd);

	/* Disable SSE before reconfiguring the SPI controller */
	sys_write32(sys_read32(dd->base + SPI_SSPCR1_BASE) &
			~BIT(SPI_SSPCR1__SSE),
			dd->base + SPI_SSPCR1_BASE);
}

static void spi_isr(int devnum);

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
	int ret = 0;
	u32_t base;
	static bool irqs_installed = false;
	struct spi_dev_data *dd = (struct spi_dev_data *)(dev->driver_data);

	base = dd->base;

	/* Disable SSE before reconfiguring the SPI controller */
	sys_write32(sys_read32(base + SPI_SSPCR1_BASE) & ~BIT(SPI_SSPCR1__SSE),
		    base + SPI_SSPCR1_BASE);

	/* Setup config reg 0 (clock rate/divisor, clock polarity/phase,
	 * data width
	 */
	ret = spi_config_sspcr0(dev);
	if (ret)
		return ret;

	/* Setup config reg 1 (master/slave mode, loop back mode */
	spi_config_sspcr1(dev);

	/* Setup pad control for SPI pins */
	ret = spi_config_mfio_pad_control(dev);
	if (ret)
		return ret;

	k_sem_init(&dd->lock, 1, 1);
	dd->last_cfg = NULL;
	dd->cb = NULL;
	dd->cb_arg = NULL;
	k_sem_init(&dd->txrx_done, 0, 1);
	dd->tx = NULL;
	dd->rx = NULL;
	dd->tx_count = 0;
	dd->rx_count = 0;
	dd->tx_offset = 0;
	dd->rx_offset = 0;

#ifdef CONFIG_SPI_DMA_MODE
	/* Initialize dma semaphore */
	k_sem_init(&dd->dma_complete, 0, 2);
	dd->dma_dev = device_get_binding(CONFIG_DMA_DEV_NAME);
	/* Enable TX and RX DMA */
	sys_write32(BIT(0) | BIT(1), dd->base + SPI_SSPDMACR_BASE);
#endif

	/* Install ISRs only once */
	if (irqs_installed == false) {
		IRQ_CONNECT(SPI_IRQ(SPI1_IRQ), 0, spi_isr, 1, 0);
		IRQ_CONNECT(SPI_IRQ(SPI2_IRQ), 0, spi_isr, 2, 0);
		IRQ_CONNECT(SPI_IRQ(SPI3_IRQ), 0, spi_isr, 3, 0);
		IRQ_CONNECT(SPI_IRQ(SPI4_IRQ), 0, spi_isr, 4, 0);
		IRQ_CONNECT(SPI_IRQ(SPI5_IRQ), 0, spi_isr, 5, 0);
		irqs_installed = true;
	}

	drain_fifos(dd);

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
 * @brief       Assert the slave SPI chip select signal
 * @details     This api asserts the SPI slave select to the specified level
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 *		level - 0 to assert low and 1 to assert high
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_assert_cs(struct spi_config *config, u8_t level)
{
	int ret;

	if (config->cs == NULL)
		return 0;

	/* Don't assert CS high if HOLD_ON_CS is set */
	if ((config->operation & SPI_HOLD_ON_CS) && level)
		return 0;

	/* Delay before asserting low */
	if ((!level) && config->cs->delay)
		k_busy_wait(config->cs->delay);

	ret =  gpio_pin_write(config->cs->gpio_dev,
			      (config->cs->gpio_pin & GPIO_PIN_COUNT_MASK),
		              level ? 1 : 0);

	/* Delay after asserting high */
	if (level && config->cs->delay)
		k_busy_wait(config->cs->delay);

	return ret;
}

#ifdef CONFIG_SPI_DMA_MODE
#define SPI_DMA_DIR_TX	0x1
#define SPI_DMA_DIR_RX	0x2

/**
 * @brief       Configure and start SPI Tx/Rx DMA
 * @details     This api configures the DMA engine to start a SPI Tx/Rx transfer
 *
 * @param[in]   dd - Pointer to the device private data structure
 * @param[in]   tx - Transmit buffer (Can be NULL, if rx != NULL)
 * @param[in]   rx - Receive buffer (Can be NULL, if tx != NULL)
 * @param[in]   stride - 1 : 8-bit transfer, 2: 16-bit transfer
 * @param[in]   dir - SPI_DMA_DIR_XX (Tx or Rx)
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int start_spi_dma(
	struct spi_dev_data *dd,
	const struct spi_buf *tx,
	const struct spi_buf *rx,
	u32_t stride)
{
	int ret = 0;
	bool tx_dma_needed;
	u32_t ch_tx, ch_rx, dummy, timeout;
	struct dma_config dma_cfg[2];
	struct dma_block_config dma_block_cfg[2];

	memset(dma_cfg, 0, sizeof(dma_cfg));
	memset(dma_block_cfg, 0, sizeof(dma_block_cfg));

	/* Tx DMA */
	dma_cfg[0].source_data_size = stride;
	dma_cfg[0].dest_data_size = stride;
	dma_cfg[0].source_burst_length = 1;
	dma_cfg[0].dest_burst_length = 1;
	dma_cfg[0].dma_callback = dd->dma_done_cb;
	if (k_is_in_isr()) { /* No callbacks in ISR mode */
		dma_cfg[0].complete_callback_en = 0;
		dma_cfg[0].error_callback_en = 0;
	} else {
		dma_cfg[0].complete_callback_en = 1;
		dma_cfg[0].error_callback_en = 1;
	}
	dma_cfg[0].block_count = 1;
	dma_cfg[0].head_block = &dma_block_cfg[0];
	dma_cfg[0].endian_mode = 0x00;

	dma_cfg[0].channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg[0].source_peripheral = 0x1F;
	dma_cfg[0].dest_peripheral = dd->tx_dma_id;
	if (tx) {
		dma_block_cfg[0].source_address = (u32_t)tx->buf;
		dma_block_cfg[0].source_addr_adj = ADDRESS_INCREMENT;
		dma_block_cfg[0].block_size = tx->len;
	} else {
		dma_block_cfg[0].source_address = (u32_t)&dummy;
		dma_block_cfg[0].source_addr_adj = ADDRESS_NO_CHANGE;
		dma_block_cfg[0].block_size = rx->len;
	}
	dma_block_cfg[0].dest_address = dd->base + SPI_SSPDR_BASE;
	dma_block_cfg[0].dest_addr_adj = ADDRESS_NO_CHANGE;

	/* Rx DMA */
	dma_cfg[1] = dma_cfg[0];
	dma_block_cfg[1] = dma_block_cfg[0];
	dma_cfg[1].head_block = &dma_block_cfg[1];

	dma_cfg[1].channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg[1].source_peripheral = dd->rx_dma_id;
	dma_cfg[1].dest_peripheral = 0x1F;
	dma_block_cfg[1].source_address = dd->base + SPI_SSPDR_BASE;
	dma_block_cfg[1].source_addr_adj = ADDRESS_NO_CHANGE;
	if (rx) {
		dma_block_cfg[1].dest_address = (u32_t)rx->buf;
		dma_block_cfg[1].dest_addr_adj = ADDRESS_INCREMENT;
		dma_block_cfg[1].block_size = rx->len;
	} else {
		dma_block_cfg[1].dest_address = (u32_t)&dummy;
		dma_block_cfg[1].dest_addr_adj = ADDRESS_NO_CHANGE;
		dma_block_cfg[1].block_size = tx->len;
	}

	if (0 != (ret = dma_request(dd->dma_dev, &ch_tx, NORMAL_CHANNEL))) {
		SYS_LOG_ERR("ERROR: No DMA channel found: %d", ret);
		return -EIO;
	}

	if (0 != (ret = dma_request(dd->dma_dev, &ch_rx, NORMAL_CHANNEL))) {
		SYS_LOG_ERR("ERROR: No DMA channel found: %d", ret);
		dma_release(dd->dma_dev, ch_tx);
		return -EIO;
	}

	if (0 != (ret = dma_config(dd->dma_dev, ch_tx, &dma_cfg[0]))) {
		SYS_LOG_ERR("ERROR configuring DMA: %d", ret);
		dma_release(dd->dma_dev, ch_tx);
		dma_release(dd->dma_dev, ch_rx);
		return -EIO;
	}

	if (0 != (ret = dma_config(dd->dma_dev, ch_rx, &dma_cfg[1]))) {
		SYS_LOG_ERR("ERROR configuring DMA: %d", ret);
		dma_release(dd->dma_dev, ch_tx);
		dma_release(dd->dma_dev, ch_rx);
		return -EIO;
	}

	/* Start the rx dma first from Memory to SPIDR */
	if (0 != (ret = dma_start(dd->dma_dev, ch_rx))) {
		SYS_LOG_ERR("Error starting DMA transfer: %d", ret);
		dma_release(dd->dma_dev, ch_tx);
		dma_release(dd->dma_dev, ch_rx);
		return -EIO;
	}

	/* In master mode, a Tx DMA is needed even for reads as the SPI protocol
	 * requries one data unit to be sent on on MOSI to receive a unit of da
	 * data on the MISO input
	 * This is not needed in slave mode sine the transfer is initiated by
	 * the master. Initiating a dummy Tx DMA for reads in slave mode can
	 * leave stale bytes in the TX fifo resulting incorrect response data
	 * to master commands.
	 * So the Tx DMA is initiated only if a tx buffer is provided or if the
	 * SPI controller is operating in the master mode.
	 */
	tx_dma_needed = tx || !GET_FIELD(sys_read32(dd->base + SPI_SSPCR1_BASE),
			       SPI_SSPCR1, MS);

	/* Start the dma, from Memory to SPI DR */
	if (tx_dma_needed) {
		if (0 != (ret = dma_start(dd->dma_dev, ch_tx))) {
			SYS_LOG_ERR("Error starting DMA transfer: %d", ret);
			dma_release(dd->dma_dev, ch_tx);
			dma_release(dd->dma_dev, ch_rx);
			return -EIO;
		}
	}

	/* Wait for both DMAs to complete */
	if (k_is_in_isr()) { /* Poll on dma status if called from ISR */
		timeout = dd->tx_rx_timeout * MAX_DMA_SIZE;
		do {
			u32_t rx_sts = 1, tx_sts = 1;

			k_busy_wait(1);

			/* If Tx DMA is not needed, set status to DMA complete*/
			if (tx_dma_needed)
				dma_status(dd->dma_dev, ch_tx, &tx_sts);
			else
				tx_sts = 0;

			dma_status(dd->dma_dev, ch_rx, &rx_sts);

			if (rx_sts || tx_sts)
				continue;
			else
				break;
		} while (--timeout);
		if (!timeout)
			ret = -ETIMEDOUT;
	} else {
		timeout = dd->tx_rx_timeout * MAX_DMA_SIZE;
		do {
			if (!k_sem_take(&dd->dma_complete, 1))
				break;
			timeout -= min(timeout, 1000); /* 1000 us in 1 ms */
		} while (timeout);
		if (!timeout)
			ret = -ETIMEDOUT;

		if (tx_dma_needed) {
			timeout = dd->tx_rx_timeout * MAX_DMA_SIZE;
			do {
				if (!k_sem_take(&dd->dma_complete, 1))
					break;
				timeout -= min(timeout, 1000);
			} while (timeout);
			if (!timeout)
				ret = -ETIMEDOUT;
		}
	}

	if (ret == -ETIMEDOUT) {
		dma_stop(dd->dma_dev, ch_rx);
		if (tx_dma_needed)
			dma_stop(dd->dma_dev, ch_tx);
	}

	dma_release(dd->dma_dev, ch_tx);
	dma_release(dd->dma_dev, ch_rx);

	if (!ret) {
		/* Wait for all the data in the fifo to be drained */
		if (tx) {
			timeout = dd->fifo_empty_timeout;
			while (TX_FIFO_EMPTY(dd) == 0) {
				if (--timeout == 0) {
					ret = -ETIMEDOUT;
					break;
				}
				k_busy_wait(1);
			}
		}

		/* Wait for last data unit to be sent out on the wire. This
		 * appears to be needed only when DMA engine is used for the
		 * transfer.
		 */
		k_busy_wait((MHZ(1) * stride * 8) / dd->freq + 1);
	} else {
		/* DMA timed out */
		/* Reset semaphore */
		while (k_sem_take(&dd->dma_complete, K_NO_WAIT) == 0)
			;
		k_sem_give(&dd->dma_complete);
		k_sem_give(&dd->dma_complete);
	}

	return ret;
}
#endif

/**
 * @brief       SPI transmit/receive data in master mode
 * @details     This api transfers and receives data on the SPI bus
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 * @param[in]   tx_bufs - Transmit buffer
 * @param[in]   tx_count - Number of elements to send on MOSI
 * @param[in]   rx_bufs - Receive buffer
 * @param[in]   rx_count - Number of elements to receive on MISO
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_master_transceive(struct spi_config *config,
					  const struct spi_buf *tx_bufs,
					  size_t tx_count,
					  struct spi_buf *rx_bufs,
					  size_t rx_count)
{
	int ret;
	u32_t stride, timeout;
	const struct spi_buf *tx, *rx;
#ifndef CONFIG_SPI_DMA_MODE
	u32_t data, tx_offset = 0, rx_offset = 0;
#endif
	struct spi_dev_config *dev_cfg =
		(struct spi_dev_config *)config->dev->config->config_info;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	if (k_sem_count_get(&dd->lock) == 0)
		return -EBUSY;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	/* Check params */
	if ((tx_bufs == NULL) && (rx_bufs == NULL))
		return -EINVAL;

	if ((tx_count == 0) && (rx_count == 0))
		return -EINVAL;

	if ((tx_bufs == NULL) && (tx_count != 0))
		return -EINVAL;

	if ((rx_bufs == NULL) && (rx_count != 0))
		return -EINVAL;

	if ((tx_bufs != NULL) && (tx_bufs->buf == NULL))
		return -EINVAL;

	if ((rx_bufs != NULL) && (rx_bufs->buf == NULL))
		return -EINVAL;

	tx = &tx_bufs[0];
	rx = &rx_bufs[0];

	if (config->operation & SPI_LOCK_ON) {
		k_sem_take(&dd->lock, K_FOREVER);
		dd->last_cfg = config;
	}

	/* Check if SPI is busy, wait for a timeout and return error
	 * if SPI bus remains busy
	 */
	if (spi_bus_wait_timeout(dd))
		return -EBUSY;

	/* Check if the config has changed from the last configured values */
	if (SPI_CFG_CHANGED(config, dev_cfg)) {
		u32_t addr, val;

		dev_cfg->frequency = config->frequency;
		dev_cfg->operation = config->operation;
		dev_cfg->vendor = config->vendor;
		dev_cfg->slave = config->slave;

		/* Disable SSE before reconfiguring the SPI controller */
		addr = dd->base + SPI_SSPCR1_BASE;
		val = sys_read32(addr);
		sys_write32(val & ~BIT(SPI_SSPCR1__SSE), addr);

		/* Invoke WAR to really flush slave Tx Fifo
		 * Not needed for Master
		 * flush_txfifo_using_lbm(config);
		 */

		/* Reconfigure the SPI controller & pad control for SPI pins */
		ret = spi_config_sspcr0(config->dev);
		if (ret)
			return ret;

		spi_config_sspcr1(config->dev);

		ret = spi_config_mfio_pad_control(config->dev);
		if (ret)
			return ret;

		drain_fifos(dd);
	}

	/* Assert SPI CS low if CS control is available */
	bcm58202_spi_assert_cs(config, 0);

#ifdef CONFIG_SPI_DMA_MODE
	ARG_UNUSED(timeout);
	do {
		/* Prepare dma and start it */
		ret = start_spi_dma(dd, tx, rx, stride);
		if (ret)
			goto error;

		if (tx_count) {
			tx_count--;
			/* Assert CS high and then low to start the
			 * next transaction
			 */
			if (tx_count) {
				tx++;
				bcm58202_spi_assert_cs(config, 1);
				bcm58202_spi_assert_cs(config, 0);
			} else {
				tx = NULL;
			}
		}
		if (rx_count) {
			rx_count--;
			/* Assert CS high and then low to start the
			 * next transaction
			 */
			if (rx_count) {
				rx++;
				bcm58202_spi_assert_cs(config, 1);
				bcm58202_spi_assert_cs(config, 0);
			} else {
				rx = NULL;
			}
		}
	} while (tx_count || rx_count);
#else
	/* Loop as long as there is something to send or receive */
	while (tx_count || rx_count) {
		/* If tx fifo has space and rx */
		data = 0x0;
		if (tx_count) {
			if (stride == 1)
				data = *((u8_t *)tx->buf + tx_offset);
			else
				data = *((u16_t *)tx->buf + tx_offset/2);

			tx_offset += stride;
			if (tx_offset >= tx->len) {
				tx++;
				tx_offset = 0;
				tx_count--;
				/* For SPI Write, wait until tx data
				 * has been sent out completely
				 */
				timeout = dd->fifo_empty_timeout;
				while (TX_FIFO_EMPTY(dd) == 0) {
					k_busy_wait(1);
					if (--timeout == 0) {
						ret = -ETIMEDOUT;
						goto error;
					}
				}
				/* Assert CS high and then low to start the
				 * next transaction
				 */
				if (tx_count) {
					bcm58202_spi_assert_cs(config, 1);
					bcm58202_spi_assert_cs(config, 0);
				}
			}
		}

		/* Wait till both tx and rx fifos has space for one
		 * entry
		 */
		timeout = dd->tx_rx_timeout;
		while (TX_NOT_FULL(dd) == 0) {
			k_busy_wait(1);
			if (--timeout == 0) {
				ret = -ETIMEDOUT;
				goto error;
			}
		}
		sys_write32(data, dd->base + SPI_SSPDR_BASE);

		/* Wait for Rx fifo to have atleast one data word before reading
		 */
		timeout = dd->tx_rx_timeout;
		while (RX_NOT_EMPTY(dd) == 0){
			k_busy_wait(1);
			if (--timeout == 0) {
				ret = -ETIMEDOUT;
				goto error;
			}
		}
		data = sys_read32(dd->base + SPI_SSPDR_BASE);
		if (rx_count) {
			if (stride == 1)
				*((u8_t *)rx->buf + rx_offset) = (u8_t)data;
			else
				*((u16_t *)rx->buf + rx_offset/2) = (u16_t)data;

			rx_offset += stride;
			if (rx_offset >= rx->len) {
				rx++;
				rx_offset = 0;
				rx_count--;
				/* Assert CS high and then low to start the
				 * next transaction
				 */
				if (rx_count) {
					bcm58202_spi_assert_cs(config, 1);
					bcm58202_spi_assert_cs(config, 0);
				}
			}
		}
	}
#endif /* CONFIG_SPI_DMA_MODE */

error:
	/* Assert SPI CS high if cs_hold is not set */
	bcm58202_spi_assert_cs(config, 1);

	if (ret) {
		/* Recover from error */
		/* Disable SSE before reconfiguring the SPI controller */
		sys_write32(sys_read32(dd->base + SPI_SSPCR1_BASE) &
			    ~BIT(SPI_SSPCR1__SSE),
			    dd->base + SPI_SSPCR1_BASE);

		/* Reconfigure the SPI controller */
		spi_config_sspcr0(config->dev);
		spi_config_sspcr1(config->dev);

		/* Drain the fifos */
		drain_fifos(dd);
	}

	return ret;
}

/**
 * @brief       SPI transmit/receive data in slave mode
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
static int bcm58202_spi_slave_transceive(struct spi_config *config,
					 const struct spi_buf *tx_bufs,
					 size_t tx_count,
					 struct spi_buf *rx_bufs,
					 size_t rx_count)
{
	int ret;
	const struct spi_buf *tx, *rx;
	u32_t stride, timeout;
#ifndef CONFIG_SPI_DMA_MODE
	u32_t data, tx_offset = 0, rx_offset = 0;
#endif
	struct spi_dev_config *dev_cfg =
		(struct spi_dev_config *)config->dev->config->config_info;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	/* Check params */
	if ((tx_bufs == NULL) && (rx_bufs == NULL))
		return -EINVAL;

	if ((tx_count == 0) && (rx_count == 0))
		return -EINVAL;

	if ((tx_bufs == NULL) && (tx_count != 0))
		return -EINVAL;

	if ((rx_bufs == NULL) && (rx_count != 0))
		return -EINVAL;

	if ((tx_bufs != NULL) && (tx_bufs->buf == NULL))
		return -EINVAL;

	if ((rx_bufs != NULL) && (rx_bufs->buf == NULL))
		return -EINVAL;

	tx = &tx_bufs[0];
	rx = &rx_bufs[0];

	/* Check if the config has changed from the last configured values */
	if (dev_cfg->operation != config->operation) {
		u32_t addr, val;

		/* Default slave mode freq */
		dev_cfg->frequency = MHZ(10);
		dev_cfg->operation = config->operation;

		/* Disable SSE before reconfiguring the SPI controller */
		addr = dd->base + SPI_SSPCR1_BASE;
		val = sys_read32(addr);
		sys_write32(val & ~BIT(SPI_SSPCR1__SSE), addr);

		/* Invoke WAR to really flush slave Tx Fifo */
		flush_txfifo_using_lbm(config);

		/* Reconfigure the SPI controller & pad control for SPI pins */
		ret = spi_config_sspcr0(config->dev);
		if (ret)
			return ret;

		spi_config_sspcr1(config->dev);

		drain_fifos(dd);
	}

	/* Loop as long as there is something to send or receive */
#ifdef CONFIG_SPI_DMA_MODE
	do {
		/* Prepare dma and start it */
		ret = start_spi_dma(dd, tx, rx, stride);
		if (ret)
			goto error;

		if (tx_count) {
			tx_count--;
			/* Assert CS high and then low to start the
			 * next transaction
			 */
			tx = tx_count ? (tx + 1) : NULL;
		}
		if (rx_count) {
			rx_count--;
			/* Assert CS high and then low to start the
			 * next transaction
			 */
			rx = rx_count ? (rx + 1) : NULL;
		}
	} while (tx_count || rx_count);
#else
	while (tx_count || rx_count) {
		/* If tx fifo has space and rx */
		data = 0x0;
		if (tx_count) {
			if (stride == 1)
				data = *((u8_t *)tx->buf + tx_offset);
			else
				data = *((u16_t *)tx->buf + tx_offset/2);

			/* Wait till both tx and rx fifos has space for one
			 * entry
			 */
			timeout = dd->tx_rx_timeout;
			while (TX_NOT_FULL(dd) == 0) {
				k_busy_wait(1);
				if (--timeout == 0) {
					ret = -ETIMEDOUT;
					goto error;
				}
			}
			sys_write32(data, dd->base + SPI_SSPDR_BASE);

			tx_offset += stride;
			if (tx_offset >= tx->len) {
				tx++;
				tx_offset = 0;
				tx_count--;
			}
		}

		/* Wait for Rx fifo to have atleast one data word before reading
		 */
		timeout = dd->tx_rx_timeout;
		while (RX_NOT_EMPTY(dd) == 0) {
			if (!k_is_in_isr()) {
				timeout -= min(1000, timeout);
				k_sleep(1);
			} else {
				timeout--;
				k_busy_wait(1);
			}

			if (timeout == 0) {
				ret = -ETIMEDOUT;
				goto error;
			}
		}
		data = sys_read32(dd->base + SPI_SSPDR_BASE);
		if (rx_count) {
			if (stride == 1)
				*((u8_t *)rx->buf + rx_offset) = (u8_t)data;
			else
				*((u16_t *)rx->buf + rx_offset/2) = (u16_t)data;

			rx_offset += stride;
			if (rx_offset >= rx->len) {
				rx_count--;
				rx_offset = 0;
				if (rx_count)
					rx++;
			}
		}
	}
#endif /* CONFIG_SPI_DMA_MODE */

	/* For SPI Write, wait until tx data has been sent out completely */
	if (tx_count) {
		timeout = dd->fifo_empty_timeout;
		while (TX_FIFO_EMPTY(dd) == 0) {
			k_busy_wait(1);
			if (--timeout == 0) {
				ret = -ETIMEDOUT;
				goto error;
			}
		}
	}

error:
	if (ret) {
		/* Recover from error */
		/* Disable SSE before reconfiguring the SPI controller */
		sys_write32(sys_read32(dd->base + SPI_SSPCR1_BASE) &
			    ~BIT(SPI_SSPCR1__SSE),
			    dd->base + SPI_SSPCR1_BASE);

		/* Invoke WAR to really flush slave Tx Fifo */
		flush_txfifo_using_lbm(config);

		/* Reconfigure the SPI controller */
		spi_config_sspcr0(config->dev);
		spi_config_sspcr1(config->dev);

		/* Drain the fifos */
		drain_fifos(dd);
	}

	return ret;
}

/**
 * @brief       Release SPI device lock
 * @details     This api should be called to release the SPI driver if it was
 *		locked by setting cs_hold in a previous SPI api call
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_release(struct spi_config *config)
{
	int ret = 0;
	struct spi_cs_control *cs = config->cs;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	/* Release in not a valid api in Slave mode */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE)
		return -EINVAL;

	if (k_sem_count_get(&dd->lock) == 0) {
		if (config && (dd->last_cfg == config)) {
			/* Assert SPI CS high if cs_hold is not set */
			if (config->cs &&
			    (!(config->operation & SPI_HOLD_ON_CS))) {
				ret = gpio_pin_write(cs->gpio_dev,
				       (cs->gpio_pin & GPIO_PIN_COUNT_MASK), 1);
				if (ret)
					return ret;
				if (config->cs->delay)
					k_busy_wait(config->cs->delay);
			}

			k_sem_give(&dd->lock);
			return ret;
		}
	}

	return -ENOLCK;
}

/**
 * @brief Register slave callback
 *
 * Note: This function is used to register a callback for slave mode. The
 *	 callback function will be called when the slave SPI interface receives
 *	 data on it MOSI pin.
 *
 * @param dev Pointer to a SPI device structure.
 * @param cb Callback function
 * @param arg Argument to callback function
 *
 * @return 0 on Success
 * @return -errno on Failure
 */
int bcm58202_spi_slave_cb_register(
	struct spi_config *config, spi_slave_cb cb, void *arg)
{
	int ret;
	u32_t value;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;
	struct spi_dev_config *dev_cfg =
		(struct spi_dev_config *)config->dev->config->config_info;

	/* Check if the config has changed from the last configured values */
	if (dev_cfg->operation != config->operation) {
		u32_t addr, val;

		/* Default slave mode freq */
		dev_cfg->frequency = MHZ(10);
		dev_cfg->operation = config->operation;

		/* Disable SSE before reconfiguring the SPI controller */
		addr = dd->base + SPI_SSPCR1_BASE;
		val = sys_read32(addr);
		sys_write32(val & ~BIT(SPI_SSPCR1__SSE), addr);

		/* Invoke WAR to really flush slave Tx Fifo */
		flush_txfifo_using_lbm(config);

		/* Reconfigure the SPI controller & pad control for SPI pins */
		ret = spi_config_sspcr0(config->dev);
		if (ret)
			return ret;

		spi_config_sspcr1(config->dev);

		drain_fifos(dd);
	}

	/* Clear existing interrupts */
	sys_write32(0xFF, dd->base + SPI_SSPI1CR_BASE);

	value = sys_read32(dd->base + SPI_SSPI1MSC_BASE);
	if (cb) {
		/* Clear RX Fifo not empty interrupt mask */
		SET_FIELD(value, SPI_SSPI1MSC, RXIM, 1);
#ifndef CONFIG_SPI_DMA_MODE
		SET_FIELD(value, SPI_SSPI1MSC, TXIM, 1);
#endif
		dd->cb_arg = arg;
		dd->cb = cb;
		irq_enable(dd->irq);
	} else {
		/* Set RX Fifo not empty interrupt mask */
		SET_FIELD(value, SPI_SSPI1MSC, RXIM, 0);
#ifndef CONFIG_SPI_DMA_MODE
		SET_FIELD(value, SPI_SSPI1MSC, TXIM, 0);
#endif
		irq_disable(dd->irq);
		dd->cb = NULL;
	}
	sys_write32(value, dd->base + SPI_SSPI1MSC_BASE);

	return 0;
}

#ifndef CONFIG_SPI_DMA_MODE
/**
 * @brief       SPI transmit/receive data in master mode
 * @details     This api transfers and receives data on the SPI bus
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_master_transceive_nondma(struct spi_config *config)
{
	int ret;
	u32_t stride;
	u32_t data;

	struct spi_dev_config *dev_cfg =
		(struct spi_dev_config *)config->dev->config->config_info;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	if (k_sem_count_get(&dd->lock) == 0)
		return -EBUSY;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	/* TBD: Can't understand why not doing the following leads to
	 * Continuuous IRQs. Need to unistall the callback
	 * as soon as both counts are o.
	 */
	/* Check params */
	if ((dd->tx == NULL) && (dd->rx == NULL))
		return -EINVAL;

	if ((dd->tx_count == 0) && (dd->rx_count == 0))
		return -EINVAL;

	if ((dd->tx == NULL) && (dd->tx_count != 0))
		return -EINVAL;

	if ((dd->rx == NULL) && (dd->rx_count != 0))
		return -EINVAL;

	if ((dd->tx != NULL) && (dd->tx->buf == NULL))
		return -EINVAL;

	if ((dd->rx != NULL) && (dd->rx->buf == NULL))
		return -EINVAL;

	if (config->operation & SPI_LOCK_ON) {
		k_sem_take(&dd->lock, K_FOREVER);
		dd->last_cfg = config;
	}

	/* Check if SPI is busy, wait for a timeout and return error
	 * if SPI bus remains busy
	 */
	if (spi_bus_wait_timeout(dd))
		return -EBUSY;

	/* Check if the config has changed from the last configured values */
	if (SPI_CFG_CHANGED(config, dev_cfg)) {
		u32_t addr, val;

		dev_cfg->frequency = config->frequency;
		dev_cfg->operation = config->operation;
		dev_cfg->vendor = config->vendor;
		dev_cfg->slave = config->slave;

		/* Disable SSE before reconfiguring the SPI controller */
		addr = dd->base + SPI_SSPCR1_BASE;
		val = sys_read32(addr);
		sys_write32(val & ~BIT(SPI_SSPCR1__SSE), addr);

		/* Invoke WAR to really flush slave Tx Fifo
		 * Not needed for Master
		 * flush_txfifo_using_lbm(config);
		 */

		/* Reconfigure the SPI controller & pad control for SPI pins */
		ret = spi_config_sspcr0(config->dev);
		if (ret)
			return ret;

		spi_config_sspcr1(config->dev);

		ret = spi_config_mfio_pad_control(config->dev);
		if (ret)
			return ret;

		drain_fifos(dd);
	}

	/* Assert SPI CS low if CS control is available */
	bcm58202_spi_assert_cs(config, 0);

	/* while tx fifo has space */
	while (TX_NOT_FULL(dd)) {
		data = 0x0;
		if (dd->tx_count) {
			if (stride == 1)
				data = *((u8_t *)dd->tx->buf + dd->tx_offset);
			else
				data = *((u16_t *)dd->tx->buf + dd->tx_offset/2);
		}
		sys_write32(data, dd->base + SPI_SSPDR_BASE);
		if (dd->tx_count) {
			dd->tx_offset += stride;
			if (dd->tx_offset >= dd->tx->len) {
				dd->tx_offset = 0;
				dd->tx_count--;
				/* Assert CS high and then low to start the
				 * next transaction
				 */
				if (dd->tx_count) {
					dd->tx++;
					/* Fixme: Is this really required? We should be
					 * able to send a continuous stream of data
					 * regardless of where they are coming from.
					 */
					bcm58202_spi_assert_cs(config, 1);
					bcm58202_spi_assert_cs(config, 0);
				}
			}
		}
	}

	/* While Rx fifo is not empty */
	while (RX_NOT_EMPTY(dd)) {
		data = sys_read32(dd->base + SPI_SSPDR_BASE);
		if (dd->rx_count) {
			if (stride == 1)
				*((u8_t *)dd->rx->buf + dd->rx_offset) = (u8_t)data;
			else
				*((u16_t *)dd->rx->buf + dd->rx_offset/2) = (u16_t)data;

			dd->rx_offset += stride;
			if (dd->rx_offset >= dd->rx->len) {
				dd->rx_offset = 0;
				dd->rx_count--;
				/* Assert CS high and then low to start the
				 * next transaction
				 */
				if (dd->rx_count) {
					dd->rx++;
					/* Fixme: Is this really required? We should be
					 * able to send a continuous stream of data
					 * regardless of where they are coming from.
					 */
					bcm58202_spi_assert_cs(config, 1);
					bcm58202_spi_assert_cs(config, 0);
				}
			}
		}
	}

	if (dd->tx_count == 0 && dd->rx_count == 0)
		k_sem_give(&dd->txrx_done);

	return ret;
}

/* Default callback for spi master in non-DMA mode
 * used with interrupts.
 */
static void spi_master_nondma_cb(void *arg)
{
	int ret;
	struct spi_config *cfg = (struct spi_config *)arg;

	ret = bcm58202_spi_master_transceive_nondma(cfg);
}

static int bcm58202_spi_master_txrx(struct spi_config *config,
				   const struct spi_buf *tx_bufs,
				   size_t tx_count,
				   struct spi_buf *rx_bufs,
				   size_t rx_count)
{
	int ret;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	dd->rx = &rx_bufs[0];
	dd->tx = &tx_bufs[0];
	dd->tx_count = tx_count;
	dd->rx_count = rx_count;
	dd->tx_offset = 0;
	dd->rx_offset = 0;

	/* Install SPI master callback */
	ret = bcm58202_spi_slave_cb_register(config, spi_master_nondma_cb, config);

	/* Wait on the txrx_done signal */
	k_sem_take(&dd->txrx_done, K_FOREVER);

	if (dd->tx_count == 0 && dd->rx_count == 0) {
		/* Allow the pending Tx if any to go through */
		while (TX_FIFO_EMPTY(dd) == 0)
			k_sleep(1);
	}

	/* Assert SPI CS high if cs_hold is not set */
	bcm58202_spi_assert_cs(config, 1);

	if (ret) {
		/* Recover from error */
		/* Disable SSE before reconfiguring the SPI controller */
		sys_write32(sys_read32(dd->base + SPI_SSPCR1_BASE) &
			    ~BIT(SPI_SSPCR1__SSE),
			    dd->base + SPI_SSPCR1_BASE);
		/* Invoke WAR to really flush slave Tx Fifo
		 * Not needed for Master
		 * flush_txfifo_using_lbm(config);
		 */
		/* Reconfigure the SPI controller */
		spi_config_sspcr0(config->dev);
		spi_config_sspcr1(config->dev);

		/* Drain the fifos */
		drain_fifos(dd);
	}

	/* Uninstall the callback & disable Interrupts */
	ret = bcm58202_spi_slave_cb_register(config, NULL, NULL);

	return ret;
}

/**
 * @brief       SPI transmit/receive data in slave non-DMA
 *  			interrupt mode
 * @details     This api transfers and receives data on the SPI
 *  			bus for slave operation in non-DMA mode using
 *  			interrupts.
 *
 * @param[in]   config - Pointer to the device structure for the driver instance
 *  			SPI device data such as tx & rx fifo is expected
 *  			to be initialized when this function is invoked.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_slave_transceive_nondma(struct spi_config *config)
{
	int ret = 0;
	u32_t stride;
	u32_t data;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	/* TBD: Can't understand why not doing the following leads to
	 * Continuuous IRQs. Need to unistall the callback
	 * as soon as both counts are o.
	 */
	/* Check params */
	if ((dd->tx == NULL) && (dd->rx == NULL))
		ret = -EINVAL;
	if ((dd->tx_count == 0) && (dd->rx_count == 0))
		ret = -EINVAL;
	if ((dd->tx == NULL) && (dd->tx_count != 0))
		ret = -EINVAL;
	if ((dd->rx == NULL) && (dd->rx_count != 0))
		ret = -EINVAL;
	if ((dd->tx != NULL) && (dd->tx->buf == NULL))
		ret = -EINVAL;
	if ((dd->rx != NULL) && (dd->rx->buf == NULL))
		ret = -EINVAL;
	if (ret) {
		k_sem_give(&dd->txrx_done);
		return ret;
	}

#ifdef MULTI_BYTE_SLAVE_ISR
	while ((TX_NOT_FULL(dd) && dd->max_tx_len) || RX_NOT_EMPTY(dd)) {
#endif
	/* Interleaving Rx & Tx: 1 byte Rx is immediately followed by
	 * 1 byte Tx to fill the tx fifo as soon as possible, rather
	 * than late, since it is important to keep tx fifo filled
	 * any time master is expected to clk in data, otherwise it
	 * may loose clk from master.
	 */
	if (RX_NOT_EMPTY(dd)) {
		data = sys_read32(dd->base + SPI_SSPDR_BASE);
		if (dd->rx_count) {
			if (stride == 1)
				*((u8_t *)dd->rx->buf + dd->rx_offset) = (u8_t)data;
			else
				*((u16_t *)dd->rx->buf + dd->rx_offset/2) = (u16_t)data;
			dd->rx_offset += stride;
			if (dd->rx_offset >= dd->rx->len) {
				dd->rx_count--;
				dd->rx_offset = 0;
				if (dd->rx_count)
					dd->rx++;
			}
		}
	}
	/* Slave transmits zeros when it doesn't have anything
	 * to transmit as long as there is something to Rx or Tx.
	 */
	if (TX_NOT_FULL(dd)) {
		data = 0;
		if (dd->tx_count) {
			if (stride == 1)
				data = *((u8_t *)dd->tx->buf + dd->tx_offset);
			else
				data = *((u16_t *)dd->tx->buf + dd->tx_offset/2);
		}
		if (dd->max_tx_len) {
			sys_write32(data, dd->base + SPI_SSPDR_BASE);
			dd->max_tx_len--;
		}
		if (dd->tx_count) {
			dd->tx_offset += stride;
			if (dd->tx_offset >= dd->tx->len) {
				dd->tx_count--;
				dd->tx_offset = 0;
				if (dd->tx_count)
					dd->tx++;
			}
		}
	}
#ifdef MULTI_BYTE_SLAVE_ISR
	}
#endif

	if ((dd->tx_count == 0 && dd->rx_count == 0) ||
	   (dd->rx_count && ((dd->rx->len - dd->rx_offset) <= 2 * FIFO_WATER_MARK)) ||
	   (dd->tx_count && ((dd->tx->len - dd->tx_offset) <= FIFO_WATER_MARK)))
		k_sem_give(&dd->txrx_done);

	return ret;
}

/* Default callback for spi slave in non-DMA mode
 * used with interrupts.
 */
static void spi_slave_nondma_cb(void *arg)
{
	int ret;
	struct spi_config *cfg = (struct spi_config *)arg;

	ret = bcm58202_spi_slave_transceive_nondma(cfg);
}

static int bcm58202_spi_slave_txrx(struct spi_config *config,
				   const struct spi_buf *tx_bufs,
				   size_t tx_count,
				   struct spi_buf *rx_bufs,
				   size_t rx_count)
{
	int ret;
	u32_t timeout, stride, data = 0;
	u32_t datacount = FIFO_WATER_MARK;
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;
	struct spi_dev_config *dev_cfg =
		(struct spi_dev_config *)config->dev->config->config_info;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	/* Check if the config has changed from the last configured values */
	if (dev_cfg->operation != config->operation) {
		u32_t addr, val;

		/* Default slave mode freq */
		dev_cfg->frequency = MHZ(10);
		dev_cfg->operation = config->operation;

		/* Disable SSE before reconfiguring the SPI controller */
		addr = dd->base + SPI_SSPCR1_BASE;
		val = sys_read32(addr);
		sys_write32(val & ~BIT(SPI_SSPCR1__SSE), addr);

		/* Invoke WAR to really flush slave Tx Fifo */
		flush_txfifo_using_lbm(config);

		/* Reconfigure the SPI controller & pad control for SPI pins */
		ret = spi_config_sspcr0(config->dev);
		if (ret)
			return ret;

		spi_config_sspcr1(config->dev);

		drain_fifos(dd);
	}

	dd->rx = &rx_bufs[0];
	dd->tx = &tx_bufs[0];
	dd->tx_count = tx_count;
	dd->rx_count = rx_count;
	dd->tx_offset = 0;
	dd->rx_offset = 0;
	dd->max_tx_len = 0;

	/* Word size rounded up to bytes */
	stride = (SPI_WORD_SIZE_GET(config->operation) + 0x7) >> 3;

	while (dd->tx_count || dd->rx_count) {
		/* Step 1: Initialization & Prefill
		 * Done only at the begining of a block transaction.
		 * (i) Calculate/Initialize the max_tx_len, which is
		 * the total number of bytes slave will be sending and
		 * receiving. This is simply the max of tx length &
		 * rx length.
		 *
		 * (ii) Pre-fill the Tx Buffer with some bytes. Normally
		 * slave need to pre-fill WATER_MARK number of bytes in its
		 * tx fifo because when we get the first "rx fifo half full
		 * or more" interrupt, that is number of bytes slave should
		 * have received in rx fifo. So when master is clocking in
		 * those bytes, slave should have its tx fifo ready so that
		 * master also clocks those out of the tx fifo. Not doing
		 * this means slave's tx will lag behind master's tx and
		 * therefore will fail to complete.
		 *
		 * Situation 1 (both rx and tx lengths less than 4):
		 * ---------------------------------------------------
		 * In this situation slave needs to prefill only what is required,
		 * i.e. max(tx length, rx length). Doing more will leave un-txed
		 * bytes in tx fifo. No further processing (such as installing
		 * ISR callback, waiting on semaphore) is required.
		 *
		 * Situation 2 (max(rxlen, txlen) > 4 but <= 8):
		 * ---------------------------------------------
		 * In such a situation, slave prefers to do the whole tx and rx
		 * in one shot, i.e. when master clocks. Slave stays ready with
		 * all the bytes it has to tx. Master clocks in all its bytes and
		 * simultaneously clocks out all of slave's bytes. Again, no further
		 * processing (such as installing ISR callback, waiting on semaphore)
		 * is required.
		 *
		 * Situation 3 (max(rxlen, txlen) > 8):
		 * -------------------------------------
		 * In this case, slave needs to just prefill WATER_MARK number
		 * of bytes, so that when these bytes are clocked out, tx fifo
		 * interrupt will happen and it process more bytes from inside
		 * the ISR. The expected HW behavior is that as soon as master
		 * clocks in its first byte, and therefore clock out slave's
		 * first byte, the tx fifo will fall below the watermark and the
		 * interrupt will trigger, thus allowing the slave to fill in the
		 * tx fifo as soon as possible. It is expected that this method of
		 * relying on the tx fifo half empty interrupt should provide
		 * latency advantage of 3 bytes tx time, around 3 us while running
		 * with 1MHz SPI clock. In contrast relying only on rx fifo half
		 * full interrupt, will delay the ISR by this amount (i.e. 3us)
		 * and slave's tx may end up missing clocking from master.
		 *
		 * Fixme:
		 * Accordingly, it is observed that whenver tx fifo interrupt is
		 * unmasked, rx fio interrupts is found missing, possibly because
		 * the condition for rx fifo interrupt becomes both true and false
		 * while slave is still inside the ISR. Confirming this requires
		 * profiling the ISR. This is more profound in case of multi byte
		 * ISRs rather than single byte ISRs.
		 *
		 * (iii) Optionally (Fixme) flush the Rx FIFO, because sometimes
		 * stray bytes are found to be left in it despite reset.
		 *
		 */
		if (!dd->tx_offset && !dd->rx_offset) {
			/* Initialize tx_remaining to max of tx length & rx length
			 * and rx_remaining to rx length.
			 */
			if (dd->rx)
				dd->max_tx_len = dd->rx->len;
			if (dd->tx)
				if(dd->max_tx_len < dd->tx->len)
					dd->max_tx_len = dd->tx->len;
			if (datacount > dd->max_tx_len ||
				dd->max_tx_len <= 2 * FIFO_WATER_MARK)
				datacount = dd->max_tx_len;
			while (TX_NOT_FULL(dd) && datacount) {
				data = 0;
				/* grab actual data if buffer is available. */
				if (dd->tx_count) {
					if (stride == 1)
						data = *((u8_t *)dd->tx->buf + dd->tx_offset);
					else
						data = *((u16_t *)dd->tx->buf + dd->tx_offset/2);
					dd->tx_offset += stride;
					if (dd->tx_offset >= dd->tx->len) {
						dd->tx_count--;
						dd->tx_offset = 0;
						if (dd->tx_count)
							dd->tx++;
					}
				}
				/* Fill in Tx fifo only if max_tx_len is non-zero. */
				if (dd->max_tx_len) {
					sys_write32(data, dd->base + SPI_SSPDR_BASE);
					dd->max_tx_len--;
				}
				datacount--;
			}
			/* Ensure the rx fifo is clean when we are starting
			 * a new transfer. This should be benign considering the
			 * fact that application will ensure the slave is ready
			 * to transceive with master.
			 * Fixme: Need to understand why this happens even when
			 * the data from master has been read, since slave always
			 * reads. Even in case of error handling, the rx fifo is
			 * flushed.
			 */
			while (RX_NOT_EMPTY(dd)) {
				data = sys_read32(dd->base + SPI_SSPDR_BASE);
#ifdef SPI_DEBUG
				printk("stray data flushed: %x\n", data);
#endif
			}
		}
		/* Install callback and wait on semaphore:
		 * (i) If there is anything to tx and the number of bytes
		 * left to tx is more than FIFO_WATER_MARK.
		 * or
		 * (ii) If there is anything to rx and the number of bytes
		 * left to rx is more than 2 * FIFO_WATER_MARK.
		 *
		 * The difference arises from the fact that slave has prefilled
		 * the tx fifo with at least FIFO_WATER_MARK, in situations
		 * where the tx length is greater than FIFO_WATER_MARK.
		 *
		 */
		if ((dd->rx_count && (dd->rx->len - dd->rx_offset) > 2 * FIFO_WATER_MARK) ||
			(dd->tx_count && (dd->tx->len - dd->tx_offset) > FIFO_WATER_MARK)) {
			/* Install SPI slave callback if not done already */
			if (dd->cb == NULL)
				ret = bcm58202_spi_slave_cb_register(config, spi_slave_nondma_cb, config);

			/* Wait on the txrx_done signal */
			k_sem_take(&dd->txrx_done, K_FOREVER);
		}

		/* Left Over Data Processing:
		 * --------------------------
		 * Note that this part will be used only in cases where
		 * amount of either tx or rx exceeds fifo water mark.
		 *
		 * If amount of data left to rx (after whatever could be
		 * collected inside the ISR), is less than fifo size then
		 * slave needs to poll on the Rx buffer, because the RNE
		 * interrupt will never happen.
		 *
		 * Note: When the ISR exited with the semaphore released
		 * it must have done so at a point where the tx fifo was
		 * full, because the multi-byte ISR does so, even after
		 * the rx fifo has become empty. This is similar to the
		 * pre-filling of the tx fifo at the very beginning of the
		 * transaction.
		 */
		if ((dd->tx_count && ((dd->tx->len - dd->tx_offset) <= FIFO_WATER_MARK)) ||
			(dd->rx_count && ((dd->rx->len - dd->rx_offset) <= 2 * FIFO_WATER_MARK))) {
			/* Can do Tx only when Rx is available. So wait on Rx from Master. */
			while(!(RX_NOT_EMPTY(dd)) && (dd->tx_count || dd->rx_count))
				k_sleep(1);
			while ((TX_NOT_FULL(dd) && dd->max_tx_len) || RX_NOT_EMPTY(dd)) {
				if (RX_NOT_EMPTY(dd)) {
					data = sys_read32(dd->base + SPI_SSPDR_BASE);
					if (dd->rx_count) {
						if (stride == 1)
							*((u8_t *)dd->rx->buf + dd->rx_offset) = (u8_t)data;
						else
							*((u16_t *)dd->rx->buf + dd->rx_offset/2) = (u16_t)data;
						dd->rx_offset += stride;
						if (dd->rx_offset >= dd->rx->len) {
							dd->rx_count--;
							dd->rx_offset = 0;
							if (dd->rx_count)
								dd->rx++;
						}
					}
				}
				/* Slave transmits zeros when it doesn't have anything
				 * to transmit as long as there is something to Rx or Tx.
				 */
				if (TX_NOT_FULL(dd)) {
					data = 0;
					if (dd->tx_count) {
						if (stride == 1)
							data = *((u8_t *)dd->tx->buf + dd->tx_offset);
						else
							data = *((u16_t *)dd->tx->buf + dd->tx_offset/2);
					}
					if (dd->max_tx_len) {
						sys_write32(data, dd->base + SPI_SSPDR_BASE);
						dd->max_tx_len--;
					}
					if (dd->tx_count) {
						dd->tx_offset += stride;
						if (dd->tx_offset >= dd->tx->len) {
							dd->tx_count--;
							dd->tx_offset = 0;
							if (dd->tx_count)
								dd->tx++;
						}
					}
				}
				/* Note on a remote possibility:
				 * Can it happen that rx fifo is found non-empty
				 * (possibly with just 1 byte) at the very first entry
				 * in to this loop? It is not expected to be the case.
				 * Note that at the entry of this loop, the tx fifo was
				 * expected to be full or at least non-empty. When the "rx
				 * fifo non-empty" condition became true from false, the
				 * "tx fifo non-full" condition also must have become true
				 * (either may have remained true or become true from false)
				 * simultaneously. So even if the rx fifo becomes non-empty
				 * momentarily and then empty again, the tx fifo is expected
				 * to be non-full during the second iteration of this loop.
				 */
			}
		}
	}

	/* After the entire transaction is complete wait
	 * until data has actually gone out.
	 */
	if (dd->tx_count == 0 && dd->rx_count == 0) {
		/* Allow the pending Tx if any to go through */
		timeout = 6;
		while (TX_FIFO_EMPTY(dd) == 0) {
			if (--timeout == 0) {
				ret = -ETIMEDOUT;
				goto error;
			}
			k_sleep(1);
		}
	}

error:
	if (ret) {
		/* Recover from error */
		/* Disable SSE before reconfiguring the SPI controller */
		sys_write32(sys_read32(dd->base + SPI_SSPCR1_BASE) &
			    ~BIT(SPI_SSPCR1__SSE),
			    dd->base + SPI_SSPCR1_BASE);

		/* Invoke WAR to really flush slave Tx Fifo */
		flush_txfifo_using_lbm(config);

		/* Reconfigure the SPI controller */
		spi_config_sspcr0(config->dev);
		spi_config_sspcr1(config->dev);

		/* Drain the fifos */
		drain_fifos(dd);
	}

	/* Uninstall the callback & disable Interrupts */
	bcm58202_spi_slave_cb_register(config, NULL, NULL);

	return ret;
}
#endif

static int bcm58202_spi_transceive(struct spi_config *config,
				   const struct spi_buf *tx_bufs,
				   size_t tx_count,
				   struct spi_buf *rx_bufs,
				   size_t rx_count)
{
#ifdef CONFIG_SPI_DMA_MODE
	struct spi_dev_data *dd =
		(struct spi_dev_data *)config->dev->driver_data;
	if (dd->dma_dev == NULL)
		return -EIO;
#endif

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE)
#ifdef CONFIG_SPI_DMA_MODE
		return bcm58202_spi_slave_transceive(config, tx_bufs, tx_count,
						     rx_bufs, rx_count);
#else
		return bcm58202_spi_slave_txrx(config, tx_bufs, tx_count,
						 rx_bufs, rx_count);
#endif
	else
		return bcm58202_spi_master_transceive(config, tx_bufs, tx_count,
						      rx_bufs, rx_count);
}

#ifdef CONFIG_POLL
/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param tx_count Number of element in the tx_bufs array.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param rx_count Number of element in the rx_bufs array.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm58202_spi_transceive_async(struct spi_config *config,
					 const struct spi_buf *tx_bufs,
					 size_t tx_count,
					 struct spi_buf *rx_bufs,
					 size_t rx_count,
					 struct k_poll_signal *async)
{
#warning "Asynchronous transceive not supported"
	return -EOPNOTSUPP;
}
#endif /* CONFIG_POLL */

/* DMA done callback apis */
#ifdef CONFIG_SPI_DMA_MODE
static void dma_done(
		struct spi_dev_data *dd,
		struct device *dev,
		u32_t ch,
		int error)
{
	ARG_UNUSED(dev);

	if (error)
		SYS_LOG_ERR("DMA[%d] Error: %d", ch, error);
	k_sem_give(&dd->dma_complete);
}

#ifdef CONFIG_SPI1_ENABLE
static void dma_done_cb_sp1(struct device *dev, u32_t ch, int error)
{
	dma_done(device_get_binding(CONFIG_SPI1_DEV_NAME)->driver_data,
		 dev, ch, error);
}
#else
#define dma_done_cb_sp1	NULL
#endif

#ifdef CONFIG_SPI2_ENABLE
static void dma_done_cb_sp2(struct device *dev, u32_t ch, int error)
{
	dma_done(device_get_binding(CONFIG_SPI2_DEV_NAME)->driver_data,
		 dev, ch, error);
}
#else
#define dma_done_cb_sp2	NULL
#endif

#ifdef CONFIG_SPI3_ENABLE
static void dma_done_cb_sp3(struct device *dev, u32_t ch, int error)
{
	dma_done(device_get_binding(CONFIG_SPI3_DEV_NAME)->driver_data,
		 dev, ch, error);
}
#else
#define dma_done_cb_sp3	NULL
#endif

#ifdef CONFIG_SPI4_ENABLE
static void dma_done_cb_sp4(struct device *dev, u32_t ch, int error)
{
	dma_done(device_get_binding(CONFIG_SPI4_DEV_NAME)->driver_data,
		 dev, ch, error);
}
#else
#define dma_done_cb_sp4	NULL
#endif

#ifdef CONFIG_SPI5_ENABLE
static void dma_done_cb_sp5(struct device *dev, u32_t ch, int error)
{
	dma_done(device_get_binding(CONFIG_SPI5_DEV_NAME)->driver_data,
		 dev, ch, error);
}
#else
#define dma_done_cb_sp5	NULL
#endif
#endif /* CONFIG_SPI_DMA_ENABLE */

/* SPI api */
static const struct spi_driver_api bcm58202_spi_api = {
	.transceive = bcm58202_spi_transceive,
#ifdef CONFIG_POLL
	.transceive_async = bcm58202_spi_transceive_async,
#endif
	.release = bcm58202_spi_release,
	.register_slave_cb = bcm58202_spi_slave_cb_register
};

/* SPI 1 device configuration and initialization */
#ifdef CONFIG_SPI1_ENABLE
static struct spi_dev_config spi1_dev_cfg = {
	.frequency = DEFAULT_SPI_SPEED_HZ,
	.operation = SPI_OP_MODE_MASTER	|	/* Master mode */
		     SPI_TRANSFER_MSB	|	/* MSB first */
		     SPI_WORD_SET(8)	|	/* Word Size - 8 */
		     SPI_LINES_SINGLE,		/* Single data line */
	.vendor = 0,
	.slave = 0
};

/* SPI 1 pin routing will conflict with QSPI quad mode pins
 * or SDIO pins. Currently, SPI1 is configured to use SDIO pins (53, 55, 56)
 * If we need SDIO pins then SPI1 driver will have to be disabled.
 */
static struct spi_dev_data spi1_dev_data = {
	.base = SPI1_BASE,
	.irq = SPI_IRQ(SPI1_IRQ),
#ifdef CONFIG_SPI_DMA_MODE
	.tx_dma_id = DMAC_PERIPHERAL_SPI1_TX,
	.rx_dma_id = DMAC_PERIPHERAL_SPI1_RX,
	.dma_done_cb = dma_done_cb_sp1,
#endif
#ifdef CONFIG_SPI1_USES_MFIO_6_TO_9
	.spi_pins = {
			{6, 0}, /* Clock */
			{8, 0}, /* MISO */
			{9, 0}  /* MOSI */
		}
#else
	.spi_pins = {
			{53, 2}, /* Clock */
			{55, 2}, /* MISO */
			{56, 2}  /* MOSI */
		}
#endif
};

DEVICE_AND_API_INIT(spi1, CONFIG_SPI1_DEV_NAME,
		    bcm58202_spi_init, &spi1_dev_data,
		    &spi1_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI1_ENABLE */

#ifdef CONFIG_SPI2_ENABLE
/* SPI 2 device configuration and initialization */
static struct spi_dev_config spi2_dev_cfg = {
	.frequency = DEFAULT_SPI_SPEED_HZ,
	.operation = SPI_OP_MODE_MASTER	|	/* Master mode */
		     SPI_TRANSFER_MSB	|	/* MSB first */
		     SPI_WORD_SET(8)	|	/* Word Size - 8 */
		     SPI_LINES_SINGLE,		/* Single data line */
	.vendor = PAD_DRIVE_STRENGTH(0x4),	/* Drive strength - 10 mA */
	.slave = 0
};

static struct spi_dev_data spi2_dev_data = {
	.base = SPI2_BASE,
	.irq = SPI_IRQ(SPI2_IRQ),
#ifdef CONFIG_SPI_DMA_MODE
	.tx_dma_id = DMAC_PERIPHERAL_SPI2_TX,
	.rx_dma_id = DMAC_PERIPHERAL_SPI2_RX,
	.dma_done_cb = dma_done_cb_sp2,
#endif
#ifdef CONFIG_SPI2_USES_MFIO_10_TO_13
	.spi_pins = {
			{10, 0}, /* Clock */
			{12, 0}, /* MISO */
			{13, 0}  /* MOSI */
		}
#else
	.spi_pins = {
			{57, 2}, /* Clock */
			{59, 2}, /* MISO */
			{60, 2}  /* MOSI */
		}
#endif
};

DEVICE_AND_API_INIT(spi2, CONFIG_SPI2_DEV_NAME,
		    bcm58202_spi_init, &spi2_dev_data,
		    &spi2_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI2_ENABLE */

#ifdef CONFIG_SPI3_ENABLE
/* SPI 3 device configuration and initialization */
static struct spi_dev_config spi3_dev_cfg = {
	.frequency = DEFAULT_SPI_SPEED_HZ,
	.operation = SPI_OP_MODE_SLAVE	|	/* Slave mode */
		     SPI_TRANSFER_MSB	|	/* MSB first */
		     SPI_WORD_SET(8)	|	/* Word Size - 8 */
		     SPI_LINES_SINGLE,		/* Single data line */
	.vendor = 0,
	.slave = 0
};

static struct spi_dev_data spi3_dev_data = {
	.base = SPI3_BASE,
	.irq = SPI_IRQ(SPI3_IRQ),
#ifdef CONFIG_SPI_DMA_MODE
	.tx_dma_id = DMAC_PERIPHERAL_SPI3_TX,
	.rx_dma_id = DMAC_PERIPHERAL_SPI3_RX,
	.dma_done_cb = dma_done_cb_sp3,
#endif
#ifdef CONFIG_SPI3_USES_MFIO_14_TO_17
	.spi_pins = {
			{14, 0}, /* Clock */
			{16, 0}, /* MISO */
			{17, 0}  /* MOSI */
		}
#else
	.spi_pins = {
			{61, 2}, /* Clock */
			{63, 2}, /* MISO */
			{64, 2}  /* MOSI */
		}
#endif
};

DEVICE_AND_API_INIT(spi3, CONFIG_SPI3_DEV_NAME,
		    bcm58202_spi_init, &spi3_dev_data,
		    &spi3_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI3_ENABLE */

#ifdef CONFIG_SPI4_ENABLE
/* SPI 4 device configuration and initialization */
static struct spi_dev_config spi4_dev_cfg = {
	.frequency = DEFAULT_SPI_SPEED_HZ,
	.operation = SPI_OP_MODE_MASTER	|	/* Master mode */
		     SPI_TRANSFER_MSB	|	/* MSB first */
		     SPI_WORD_SET(8)	|	/* Word Size - 8 */
		     SPI_LINES_SINGLE,		/* Single data line */
	.vendor = 0,
	.slave = 0
};

static struct spi_dev_data spi4_dev_data = {
	.base = SPI4_BASE,
	.irq = SPI_IRQ(SPI4_IRQ),
#ifdef CONFIG_SPI_DMA_MODE
	.tx_dma_id = DMAC_PERIPHERAL_SPI4_TX,
	.rx_dma_id = DMAC_PERIPHERAL_SPI4_RX,
	.dma_done_cb = dma_done_cb_sp4,
#endif
#ifdef CONFIG_SPI4_USES_MFIO_18_TO_21
	.spi_pins = {
			{18, 0}, /* Clock */
			{20, 0}, /* MISO */
			{21, 0}  /* MOSI */
		}
#else
	.spi_pins = {
			{65, 2}, /* Clock */
			{67, 2}, /* MISO */
			{68, 2}  /* MOSI */
		}
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
static struct spi_dev_config spi5_dev_cfg = {
	.frequency = DEFAULT_SPI_SPEED_HZ,
	.operation = SPI_OP_MODE_MASTER	|	/* Master mode */
		     SPI_TRANSFER_MSB	|	/* MSB first */
		     SPI_WORD_SET(8)	|	/* Word Size - 8 */
		     SPI_LINES_SINGLE,		/* Single data line */
	.vendor = PAD_HYS_EN(1) |		/* Hysteresis enable */
		  PAD_DRIVE_STRENGTH(0x6),	/* Drive strength - 14 mA */
	.slave = 0
};

static struct spi_dev_data spi5_dev_data = {
	.base = SPI5_BASE,
	.irq = SPI_IRQ(SPI5_IRQ),
#ifdef CONFIG_SPI_DMA_MODE
	.tx_dma_id = DMAC_PERIPHERAL_SPI5_TX,
	.rx_dma_id = DMAC_PERIPHERAL_SPI5_RX,
	.dma_done_cb = dma_done_cb_sp5,
#endif
	.spi_pins = {
			{22, 0}, /* Clock */
			{24, 0}, /* MISO */
			{25, 0}  /* MOSI */
		}
};

DEVICE_AND_API_INIT(spi5, CONFIG_SPI5_DEV_NAME,
		    bcm58202_spi_init, &spi5_dev_data,
		    &spi5_dev_cfg, POST_KERNEL,
		    CONFIG_SPI_DRIVER_INIT_PRIORITY, &bcm58202_spi_api);
#endif /* CONFIG_SPI5_ENABLE */

/**
 * @brief SPI Interrupt service routine
 */
static void spi_isr(int devnum)
{
	struct device *dev = NULL;
	struct spi_dev_data *dd;
	u32_t intstat;

	switch (devnum) {
	case 1:
#ifdef CONFIG_SPI1_ENABLE
		dev = DEVICE_GET(spi1);
#endif
		break;
	case 2:
#ifdef CONFIG_SPI2_ENABLE
		dev = DEVICE_GET(spi2);
#endif
		break;
	case 3:
#ifdef CONFIG_SPI3_ENABLE
		dev = DEVICE_GET(spi3);
#endif
		break;
	case 4:
#ifdef CONFIG_SPI4_ENABLE
		dev = DEVICE_GET(spi4);
#endif
		break;
	case 5:
#ifdef CONFIG_SPI5_ENABLE
		dev = DEVICE_GET(spi5);
#endif
		break;
	}

	if (dev == NULL)
		return;

	dd = (struct spi_dev_data *)(dev->driver_data);

	intstat = sys_read32(dd->base + SPI_SSPRIS_BASE);
	/* Clear timeout/overrun interrupt status
	 * Note: The RAW interrupt status register (RIS)
	 * needs to be read here. Not reading the register
	 * results in subsequent DMA read hangs
	 */
	sys_write32(intstat, dd->base + SPI_SSPI1CR_BASE);

	if (dd->cb)
		dd->cb(dd->cb_arg);
}
#endif /* CONFIG_SPIx_ENABLE (x=1,2,3,4,5) */

#endif /* CONFIG_SPI_LEGACY_API */
