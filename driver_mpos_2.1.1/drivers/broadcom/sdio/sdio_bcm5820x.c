/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term “Broadcom”
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

/* @file sdio_bcm5820x.c
 *
 * @brief SDIO driver for BCM 5820X
 *
 * This driver provides apis, to configure secure digital interface for IO and
 * memory cards.
 *
 */

#include <board.h>
#include <sdio.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <genpll.h>
#include "sdio_bcm5820x_regs.h"

#ifdef CONFIG_SDIO_DMA_MODE
#include <cortex_a/cache.h>
#endif

/**
 * @brief SD driver private info
 *
 * @param bus_width SD controller configured bus width (1/4/8)
 * @param clock_speed SD bus configured clock speed
 * @param cd_cb Card detect callback function
 */
struct sd_priv_data {
	u8_t bus_width;
	u32_t clock_speed;
	sd_card_detect_cb cd_cb;
	struct k_fifo isr_fifo;
};

/**
 * @brief SD driver configuration
 *
 * @param base register base address
 * @param irq Interrupt number
 */
struct sd_config {
	u32_t base;
	u32_t irq;
};

static int bcm5820x_sd_init(struct device *dev);

/**
 * @brief Reset SDIO host controller
 */
static void sdio_hc_reset(struct sd_config *cfg)
{
	u32_t timeout;
	u32_t base = cfg->base;

	/* Software reset for all * 1 = reset * 0 = work */
	sys_write32(BIT(SDIO_CTRL1__RST), base + SDIO_CTRL1_ADDR);

	/* Wait max 100 ms */
	timeout = 100;

	/* hw clears the bit when it's done */
	do {
		if ((sys_read32(base + SDIO_CTRL1_ADDR) & BIT(SDIO_CTRL1__RST))
		    == 0)
			break;
		k_busy_wait(1000);
	} while (--timeout);

	if (timeout == 0)
		SYS_LOG_ERR("reset timed out");
	else
		SYS_LOG_DBG("Reset done");
}

/* ISR message FIFO entry
 * The message entry does not carry any information
 * The presence of a message indicates that an interrupt
 * occurred and the code waiting for the isr will wait on the
 * message fifo, so that CPU cycles are not spent polling the register
 * in the thread calling sd_send_command() api.
 */
struct isr_fifo_entry {
	void *link_in_fifo; /* Used by Zephyr kernel */
};

/* Fifo elements */
static struct isr_fifo_entry msg_entry;

/*
 * @brief Fifo add element
 * @details Called from SDIO ISR to add interrupt status register value to fifo
 */
static void add_msg_to_fifo(struct k_fifo *fifo)
{
	k_fifo_put(fifo, &msg_entry);
}

/**
 * @bried SDIO Interrupt handler
 */
static void sdio_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;

	u32_t msg;
	u32_t status = sys_read32(cfg->base + SDIO_INTR_ADDR);

	if (dd->cd_cb) {
		if (GET_FIELD(status, SDIO_INTR, CRDRMV))
			dd->cd_cb(false);
		if (GET_FIELD(status, SDIO_INTR, CRDINS))
			dd->cd_cb(true);
	}

	/* If bits other than the card ins/rem status are set, send a message */
	msg = status;
	msg &= ~BIT(SDIO_INTR__CRDINS);
	msg &= ~BIT(SDIO_INTR__CRDRMV);
	if (msg)
		add_msg_to_fifo(&dd->isr_fifo);

	/* Write the status bits back to clear all card ins/rem interrupts */
	sys_write32(status & (BIT(SDIO_INTR__CRDINS) | BIT(SDIO_INTR__CRDRMV)),
		    cfg->base + SDIO_INTR_ADDR);
}

/**
 * @brief       Send SD command to device
 * @details     This api sends the SD command specified flash device and
 *		retrieves the response if any. If the command involves a data
 *		transfer, then the data is read from/written to the buffer
 *		provided
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cmd - Command information structure
 * @param[in]	data - Data buffer structure, optional depending on the command
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm5820x_sd_send_cmd(
			struct device *dev,
			struct sd_cmd *cmd,
			struct sd_data *data)
{
	u32_t i = 0;
	int flags = 0;
	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;
	u32_t timeout, regval, cmd_complete_retry = 100, resp_retry = 100;
	u32_t base = cfg->base;

	/* Wait max 10 ms */
	timeout = 10;

	/*
	 * PRNSTS
	 * CMDINHDAT[1] : Command Inhibit (DAT)
	 * CMDINHCMD[0] : Command Inhibit (CMD)
	 */
	/* Set command inhibit. */
	regval = BIT(SDIO_PSTATE__CMDINH);
	/* Set dat inhibit. */
	if ((data) || (cmd->resp_type & SD_RSP_BUSY))
		regval |= BIT(SDIO_PSTATE__DATINH);

	/*
	 * We shouldn't wait for data inihibit for stop commands, even
	 * though they might use busy signaling
	 */
	if (cmd->cmd_idx == SD_CMD12_STOP_TRANSMISSION)
		regval &= ~BIT(SDIO_PSTATE__DATINH);

	do {
		if ((sys_read32(base + SDIO_PSTATE_ADDR) & regval) == 0)
			break;
		k_sleep(1);
 	} while (--timeout);

	if (timeout == 0) {
		SYS_LOG_ERR("Timed out waiting for cmd/dat inhibit bits");
		return -ETIMEDOUT;
	}

	/* Set up block cnt, and block size.*/
	if (data) {
		sys_write32((u32_t)data->buf, base + SDIO_SYSADDR_ADDR);
		regval = 0;
		SET_FIELD(regval, SDIO_BLOCK, HSBS, 0);
		SET_FIELD(regval, SDIO_BLOCK, TBS, data->block_size);
		SET_FIELD(regval, SDIO_BLOCK, BCNT, data->num_blocks);
		sys_write32(regval, base + SDIO_BLOCK_ADDR);
	}

	sys_write32(cmd->cmd_arg, base + SDIO_ARG_ADDR);

	flags = 0;
	if (data) {
		/* Set block count enable */
		SET_FIELD(flags, SDIO_CMD, BCEN, 1);
		/* Multiple block select.*/
		if (data->num_blocks > 1)
			SET_FIELD(flags, SDIO_CMD, MSBS, 1);
		/* 1 = read, 0 = write. */
		if (data->flags & SD_DATA_READ)
			SET_FIELD(flags, SDIO_CMD, DTDS, 1);

#ifdef CONFIG_SDIO_DMA_MODE
		SET_FIELD(flags, SDIO_CMD, DMA, 1);
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Flush the src or dest buffer to ensure no writebacks
		 * are pending
		 */
		clean_dcache_by_addr((u32_t)data->buf,
			data->block_size * data->num_blocks);
#endif
#endif
	}

	if ((cmd->resp_type & SD_RSP_136) && (cmd->resp_type & SD_RSP_BUSY))
		return -EINVAL;

	/* Remove any stale messages in the fifo */
	k_fifo_get(&dd->isr_fifo, K_NO_WAIT);

	/*
	 * CMDREG
	 * CMDIDX[29:24]: Command index
	 * DPS[21]      : Data Present Select
	 * CCHK_EN[20]  : Command Index Check Enable
	 * CRC_EN[19]   : Command CRC Check Enable
	 * RTSEL[1:0]
	 *      00 = No Response
	 *      01 = Length 136
	 *      10 = Length 48
	 *      11 = Length 48 Check busy after response
	 */
	if ((cmd->resp_type & SD_RSP_PRESENT) == 0)
		SET_FIELD(flags, SDIO_CMD, RTSEL, 0);
	else if (cmd->resp_type & SD_RSP_136)
		SET_FIELD(flags, SDIO_CMD, RTSEL, 1);
	else if (cmd->resp_type & SD_RSP_BUSY)
		SET_FIELD(flags, SDIO_CMD, RTSEL, 3);
	else
		SET_FIELD(flags, SDIO_CMD, RTSEL, 2);

	/* Set CRC enable */
	if (cmd->resp_type & SD_RSP_CRC)
		SET_FIELD(flags, SDIO_CMD, CRC_EN, 1);

	/* Set command index check enable */
	if (cmd->resp_type & SD_RSP_CMDIDX)
		SET_FIELD(flags, SDIO_CMD, CCHK_EN, 1);

	/* Set data present select bit */
	if (data)
		SET_FIELD(flags, SDIO_CMD, DPS, 1);

	/* Set command index */
	SET_FIELD(flags, SDIO_CMD, CIDX, cmd->cmd_idx);
	sys_write32(flags, base + SDIO_CMD_ADDR);

	/* Wait for command complete */
	k_fifo_get(&dd->isr_fifo, cmd_complete_retry);
	regval = sys_read32(base + SDIO_INTR_ADDR);
	if (GET_FIELD(regval, SDIO_INTR, CMDDONE)) {
		if (!data)
			sys_write32(0x1, base + SDIO_INTR_ADDR);
	} else {
		SYS_LOG_ERR("Time out sending CMD%d: %x", cmd->cmd_idx, regval);
		/* Set CMDRST and DATARST bits.
		 * Problem :
		 * -------
		 *  When a command 8 is sent in case of MMC card, it will not
		 *  respond, and CMD INHIBIT bit of PRSTATUS register will be
		 *  set to 1, causing no more commands to be sent from host
		 *  controller. This causes things to stall.
		 *  Solution :
		 *  ---------
		 *  In order to avoid this situation, we clear the CMDRST and
		 *  DATARST bits in the case when card
		 *  doesn't respond back to a command sent by host controller.
		 */
		regval = sys_read32(base + SDIO_CTRL1_ADDR);
		SET_FIELD(regval, SDIO_CTRL1, CMDRST, 1);
		SET_FIELD(regval, SDIO_CTRL1, DATRST, 1);
		sys_write32(regval, base + SDIO_CTRL1_ADDR);

		/* Clear all interrupts */
		sys_write32(0xFFFFFFFF, base + SDIO_INTR_ADDR);

		return -ETIMEDOUT;
	}

	if (GET_FIELD(regval, SDIO_INTR, CTOERR)) {
		/* Timeout Error */
		SYS_LOG_ERR("error: CTOERROR //Command Time out error");
		return -ETIMEDOUT;
	} else if (GET_FIELD(regval, SDIO_INTR, ERRIRQ)) {
		/* Error Interrupt */
		if (GET_FIELD(regval, SDIO_INTR, DTOERR))
			SYS_LOG_ERR("error: DTOERROR //Data Time out error");

		if (GET_FIELD(regval, SDIO_INTR, DCRCERR))
			SYS_LOG_ERR("error: DCRCERR //Data CRC error");

		if (GET_FIELD(regval, SDIO_INTR, DEBERR))
			SYS_LOG_ERR("error: DEBERR //Data end bit error");

		SYS_LOG_ERR("error: %08x cmd %d\n", regval, cmd->cmd_idx);
		return -EIO;
	}

	if (cmd->resp_type & SD_RSP_PRESENT) {
		if (cmd->resp_type & SD_RSP_136) {
			u32_t resp[4];

			/* CRC is stripped so we need to do some shifting. */
			for (i = 0; i < 4; i++)
				resp[i] = sys_read32(
					    base + SDIO_RESP0_ADDR + i*4);

			cmd->response[0] = (resp[3] << 8) | (resp[2] >> 24);
			cmd->response[1] = (resp[2] << 8) | (resp[1] >> 24);
			cmd->response[2] = (resp[1] << 8) | (resp[0] >> 24);
			cmd->response[3] = (resp[0] << 8);

			for (i = 0; i < 4; i++)
				SYS_LOG_DBG("resp[%d]: %08x\n", i,
					    cmd->response[i]);

		} else if (cmd->resp_type & SD_RSP_BUSY) {
			for (i = 0; i < resp_retry; i++) {
				/* PRNTDATA[23:20] : DAT[3:0] Line Signal */
				regval = sys_read32(base + SDIO_PSTATE_ADDR);
				if (regval & BIT(SDIO_PSTATE__DLS3_0_R))
					break;
				k_sleep(1);
			}

			if (i == resp_retry) {
				SYS_LOG_WRN("card is still busy");
				return -ETIMEDOUT;
			}

			cmd->response[0] = sys_read32(base + SDIO_RESP0_ADDR);
			SYS_LOG_DBG("cmd->resp[0]: %08x\n", cmd->response[0]);
		} else {
			cmd->response[0] = sys_read32(base + SDIO_RESP0_ADDR);
			SYS_LOG_DBG("cmd->resp[0]: %08x\n", cmd->response[0]);
		}
	}

	/* Read PIO */
	if (data && (data->flags & SD_DATA_READ)) {
#ifndef CONFIG_SDIO_DMA_MODE
		u32_t len = data->block_size * data->num_blocks;
		u8_t *buf, byte_count;
#endif

		do {
			regval = sys_read32(base + SDIO_INTR_ADDR);
			if (GET_FIELD(regval, SDIO_INTR, CMDDONE)) {
#ifdef CONFIG_SDIO_DMA_MODE
				break;
#else
				/* Wait for read buffer to be ready if DMA
				 * disabled
				 */
				if (GET_FIELD(regval, SDIO_INTR, BRRDY))
					break;
#endif
			}

			if (GET_FIELD(regval, SDIO_INTR, ERRIRQ)) {
				/* Error Interrupt */
				sys_write32(regval, base + SDIO_INTR_ADDR);
				SYS_LOG_ERR("error during transfer: 0x%08x\n",
					    regval);
				return -EIO;
			}
		} while (1);

#ifndef CONFIG_SDIO_DMA_MODE
		/* Loop read */
		buf = (u8_t *)data->buf;
		byte_count = 0;
		while (len) {
			if (byte_count == 0)
				regval = sys_read32(base + SDIO_BUFDAT_ADDR);

			*buf = regval & 0xFF;

			buf++;
			byte_count++;
			byte_count %= 4;
			regval >>= 8;
			len--;
		}
#endif

		/* Wait for TXDONE */
		do {
			regval = sys_read32(base + SDIO_INTR_ADDR);

			if (GET_FIELD(regval, SDIO_INTR, TXDONE))
				break;

			if (GET_FIELD(regval, SDIO_INTR, ERRIRQ)) {
				/* Error Interrupt */
				sys_write32(regval, base + SDIO_INTR_ADDR);
				SYS_LOG_ERR("error during transfer: 0x%08x\n",
					    regval);
				return -EIO;
			}
		} while (1);

#ifdef CONFIG_SDIO_DMA_MODE
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Invalidate data written via DMA to SRAM */
		invalidate_dcache_by_addr((u32_t)data->buf,
				data->block_size * data->num_blocks);
#endif
#endif
	}
	/* Write PIO */
	else if (data && !(data->flags & SD_DATA_READ)) {
#ifndef CONFIG_SDIO_DMA_MODE
		u32_t len = data->block_size * data->num_blocks, byte_count;
		u8_t *buf;
#endif

		do {
			regval = sys_read32(base + SDIO_INTR_ADDR);

			if (GET_FIELD(regval, SDIO_INTR, CMDDONE)) {
#ifdef CONFIG_SDIO_DMA_MODE
				break;
#else
				/* Wait for write buffer to be ready if DMA
				 * disabled
				 */
				if (GET_FIELD(regval, SDIO_INTR, BWRDY))
					break;
#endif
			}

			if (GET_FIELD(regval, SDIO_INTR, ERRIRQ)) {
				/* Error Interrupt */
				sys_write32(regval, base + SDIO_INTR_ADDR);
				SYS_LOG_ERR("error during transfer: 0x%08x\n",
					    regval);
				return -EIO;
			}
		} while (1);

#ifndef CONFIG_SDIO_DMA_MODE
		buf = (u8_t *)data->buf;
		byte_count = 0;
		regval = 0;
		while (len) {
			regval >>= 8;
			regval |= (u32_t)*buf << 24;

			buf++;
			byte_count++;
			byte_count %= 4;
			len--;

			if (byte_count == 0) {
				sys_write32(regval, base + SDIO_BUFDAT_ADDR);
				regval = 0;
			}
		}
		/* last n bytes, where n = len % 4, n != 0 */
		if (byte_count)
			sys_write32(regval, base + SDIO_BUFDAT_ADDR);
#endif

		/* Wait for TX to complete */
		do {
			regval = sys_read32(base + SDIO_INTR_ADDR);

			if (GET_FIELD(regval, SDIO_INTR, TXDONE))
				break;

			if (GET_FIELD(regval, SDIO_INTR, ERRIRQ)) {
				/* Error Interrupt */
				sys_write32(regval, base + SDIO_INTR_ADDR);
				SYS_LOG_ERR("error during transfer: 0x%08x\n",
					    regval);
				return -EIO;
			}
		} while (1);
	}

	/* Clear all interrupts */
	sys_write32(0xFFFFFFFF, base + SDIO_INTR_ADDR);
	k_busy_wait(1000);

	return 0;
}

/**
 * @brief       Set SD bus width
 * @details     This api configures the SDIO host controller according to the
 *		bus width specified
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   bus_width - Bus width to be used (Valid values: 1/4/8)
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm5820x_sd_set_width(struct device *dev, u8_t bus_width)
{
	u32_t regval;
	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;

	regval = sys_read32(cfg->base + SDIO_CTRL_ADDR);
	/*  1 = 4-bit mode , 0 = 1-bit mode */
	switch (bus_width) {
		default:
			return -EINVAL;
		case 1:
			SET_FIELD(regval, SDIO_CTRL, DXTW, 0);
			break;
		case 4:
			SET_FIELD(regval, SDIO_CTRL, DXTW, 1);
			break;
		case 8:
			SET_FIELD(regval, SDIO_CTRL, SDB, 1);
			break;
	}

	/* Set SD voltage select and SD power bit */
	SET_FIELD(regval, SDIO_CTRL, SDVSEL, 0x7); /* SD voltage - 3.3 V */
	SET_FIELD(regval, SDIO_CTRL, SDPWR, 1);
	SET_FIELD(regval, SDIO_CTRL, HSEN, 1);

	sys_write32(regval, cfg->base + SDIO_CTRL_ADDR);

	dd->bus_width = bus_width;
	return 0;
}

/**
 * @brief       Set SD bus clock speed
 * @details     This api configures the SDIO clock to the specified clock speed
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]	clock_speed - Clock speed in Hz
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm5820x_sd_set_speed(struct device *dev, u32_t clock_speed)
{
	int ret;
	u32_t regval, timeout, bclk, div = 0;
	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;

	regval = sys_read32(cfg->base + SDIO_CTRL1_ADDR);
	regval &= 0xFFFF0000;	/* Clean up all bits related to clock.*/
	sys_write32(regval, cfg->base + SDIO_CTRL1_ADDR);
	regval = 0;

	if (clock_speed == 0)
		return -EINVAL;

	/* Clock speed SDCLK = BCLK/2*N, where BCLK is the base clock
	 * in SDIO_CAPABILITIES1 register and N is the divisor
	 */
	bclk = GET_FIELD(sys_read32(cfg->base + SDIO_CAPABILITIES1_ADDR),
			 SDIO_CAPABILITIES1, BCLK);
	bclk *= MHZ(1);
	if (bclk == 0) {
		ret = clk_get_ahb(&bclk);
		if (ret)
			return ret;
	}

	if (clock_speed > bclk)
		return -EINVAL;

	div = bclk / (2*clock_speed);

	/* If the divisor turns out to be fractional, ensure that the clock
	 * speed configured is less than the requested speed
	 */
	if ((bclk / (2*div)) > clock_speed)
		div++;

	/* Divisor is a 10 bit field, which limits the minimum clk frequency */
	if (div >= BIT(10))
		return -ENOTSUP;

	/* Write divider value, and enable internal clock. */
	regval = sys_read32(cfg->base + SDIO_CTRL1_ADDR);
	SET_FIELD(regval, SDIO_CTRL1, SDCLKSEL, div & 0xFF);
	if (div > 0xFF)
		SET_FIELD(regval, SDIO_CTRL1, SDCLKSEL_UP, div >> 8);
	SET_FIELD(regval, SDIO_CTRL1, ICLKEN, 1);
	sys_write32(regval, cfg->base + SDIO_CTRL1_ADDR);

	/* Wait for clock to stabilize */
	/* Wait max 20 ms */
	timeout = 20;
	do {
		if (GET_FIELD(sys_read32(cfg->base + SDIO_CTRL1_ADDR),
			      SDIO_CTRL1, ICLKSTB))
			break;
		k_busy_wait(1000);
	} while (--timeout);

	if (timeout == 0)
		return -ETIMEDOUT;

	/* Enable sdio clock now.*/
	regval = sys_read32(cfg->base + SDIO_CTRL1_ADDR);
	SET_FIELD(regval, SDIO_CTRL1, SDCLKEN, 1);
	sys_write32(regval, cfg->base + SDIO_CTRL1_ADDR);

	dd->clock_speed = clock_speed;

	return 0;
}

/**
 * @brief       Register card detect callback
 * @details     This api registers a callback that will be called upon detecting
 *		an SD card insertion or removal
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]	cb - Pointer to card detect callback
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bcm5820x_sd_register_cd_cb(
			struct device *dev,
			sd_card_detect_cb cb)
{
	/* On BCM5820x SoCs, the SD card detection logic is a convoluted
	 * affair. The SDIO controller does not have a dedicated input pin
	 * for card detect. So to support card detection, the SDIO controller
	 * provides a bit in the register CDRU_SDIO_FUNC to be set by software
	 * upon card insertion and removal. The expectation is that a GPIO
	 * input pin is dedicated for card detection, with that GPIO's interrupt
	 * handler being responsible for setting CDRU_SDIO_FUNC.card_detect_n
	 * bit based on the state of the input level at the GPIO pin. This makes
	 * the SDIO controller's card detect logic redundant since the GPIO
	 * handler callback can in itself be used as a card detect callback.
	 */
#ifdef SDIO_CD_INT_SUPPORT
	u32_t intmask;

	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;

	/* Set callback before enabling interrupt */
	if (cb)
		dd->cd_cb = cb;

	/* Set card insert/remove interrupt bits */
	intmask = sys_read32(cfg->base + SDIO_INTREN1_ADDR);
	SET_FIELD(intmask, SDIO_INTREN1, CRDRMVEN, cb ? 1 : 0);
	SET_FIELD(intmask, SDIO_INTREN1, CRDINSEN, cb ? 1 : 0);
	sys_write32(intmask, cfg->base + SDIO_INTREN1_ADDR);

	/* Set card insert/remove interrupt signal enable bits */
	intmask = sys_read32(cfg->base + SDIO_INTREN2_ADDR);
	SET_FIELD(intmask, SDIO_INTREN2, CRDRVMEN, cb ? 1 : 0);
	SET_FIELD(intmask, SDIO_INTREN2, CRDINSEN, cb ? 1 : 0);
	sys_write32(intmask, cfg->base + SDIO_INTREN2_ADDR);

	/* Reset callback after disabling interrupt */
	if (cb == NULL)
		dd->cd_cb = NULL;
	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
#endif
}

static struct sd_driver_api bcm5820x_sd_api = {
	.send_cmd = bcm5820x_sd_send_cmd,
	.set_width = bcm5820x_sd_set_width,
	.set_speed = bcm5820x_sd_set_speed,
	.register_cd_cb = bcm5820x_sd_register_cd_cb,
};

static struct sd_priv_data sd_dev_data = {
	.cd_cb = NULL,
	.bus_width = 1,
	.clock_speed = KHZ(400),
};

static struct sd_config sd_dev_config = {
	.base = SDIO_BASE_ADDR,
	.irq = SDIO_IRQ,
};

DEVICE_AND_API_INIT(sdio, CONFIG_SDIO_DEV_NAME,
		    bcm5820x_sd_init, &sd_dev_data,
		    &sd_dev_config, POST_KERNEL,
		    CONFIG_SDIO_DRIVER_INIT_PRIORITY, &bcm5820x_sd_api);

/**
 * @brief SDIO host controller initialization
 */
static int bcm5820x_sd_init(struct device *dev)
{
	int ret;
	u32_t regval;
	struct sd_config *cfg = (struct sd_config *)dev->config->config_info;
	struct sd_priv_data *dd = (struct sd_priv_data *)dev->driver_data;
	u32_t base = cfg->base;

	/* First, release SDIO IDM reset */
	sys_write32(0x1, SDIO_IDM_RESET_CONTROL);
	k_busy_wait(10000);
	sys_write32(0x0, SDIO_IDM_RESET_CONTROL);

	sdio_hc_reset(cfg);

	/* Wait for 10 ms */
	k_busy_wait(10000);

	/* Set power now.*/
	regval = sys_read32(base + SDIO_CTRL_ADDR);
	SET_FIELD(regval, SDIO_CTRL, SDVSEL, 5);
	SET_FIELD(regval, SDIO_CTRL, SDPWR, 1);
	sys_write32(regval, base + SDIO_CTRL_ADDR);

	/* Wait for 10 ms */
	k_busy_wait(10000);

	/* Enable all error interrupts */
	sys_write32(0xFFFF0000, base + SDIO_INTREN1_ADDR);
	sys_write32(0xFFFF0000, base + SDIO_INTREN2_ADDR);

	/* DATA line timeout: TMCLK - 2^26 */
	regval = sys_read32(base + SDIO_CTRL1_ADDR);
	SET_FIELD(regval, SDIO_CTRL1, DTCNT, 0xD);
	sys_write32(regval, base + SDIO_CTRL1_ADDR);

	/*
	 * Interrupt Status Enable Register init
	 * bit 5 : Buffer Read Ready Status Enable
	 * bit 4 : Buffer write Ready Status Enable
	 * bit 1 : Transfer Complete Status Enable
	 * bit 0 : Command Complete Status Enable
	 */
	regval = sys_read32(base + SDIO_INTREN1_ADDR);
	regval &= ~(0xFFFF);
	SET_FIELD(regval, SDIO_INTREN1, BUFRREN, 1);
	SET_FIELD(regval, SDIO_INTREN1, BUFWREN, 1);
	SET_FIELD(regval, SDIO_INTREN1, TXDONEEN, 1);
	SET_FIELD(regval, SDIO_INTREN1, CMDDONEEN, 1);
	SET_FIELD(regval, SDIO_INTREN1, DMAIRQEN, 1);
	SET_FIELD(regval, SDIO_INTREN1, FIXZ, 1);
	sys_write32(regval, base + SDIO_INTREN1_ADDR);

	/*
	 * Interrupt Signal Enable Register init
	 * bit 1 : Transfer Complete Signal Enable
	 */
	regval = sys_read32(base + SDIO_INTREN2_ADDR);
	regval &= ~(0xFFFF);
	SET_FIELD(regval, SDIO_INTREN2, TXDONE, 1);
	SET_FIELD(regval, SDIO_INTREN2, BUFRRDYEN, 1);
	SET_FIELD(regval, SDIO_INTREN2, BUFWRDYEN, 1);
	SET_FIELD(regval, SDIO_INTREN2, FIXZERO, 1);
	SET_FIELD(regval, SDIO_INTREN2, CMDDONE, 1);
	sys_write32(regval, base + SDIO_INTREN2_ADDR);

	/* Set default clock speed and bus width */
	ret = bcm5820x_sd_set_width(dev, dd->bus_width);
	if (ret) {
		SYS_LOG_ERR("Set width(%d) failed: %d", dd->bus_width, ret);
		return ret;
	}
	ret = bcm5820x_sd_set_speed(dev, dd->clock_speed);
	if (ret) {
		SYS_LOG_ERR("Set speed(%d) failed: %d", dd->clock_speed, ret);
		return ret;
	}

	/* Create the ISR fifo so avoid polling for command/data-transfer
	 * completion status
	 */
	k_fifo_init(&dd->isr_fifo);

	/* Install ISR */
	IRQ_CONNECT(SDIO_IRQ, 0, sdio_isr, DEVICE_GET(sdio), 0);
	irq_enable(SDIO_IRQ);

	return 0;
}
