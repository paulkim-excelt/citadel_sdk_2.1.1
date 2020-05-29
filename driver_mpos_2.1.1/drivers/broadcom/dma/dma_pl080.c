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

/*
 * @file dma_pl080.c
 * @brief pl080 dma driver implementation
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/dma.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <cortex_a/cache.h>

#ifdef CONFIG_MEM_LAYOUT_AVAILABLE
#include <arch/arm/soc/bcm58202/layout.h>
#endif

/* Controller configuration registers */
#define PL080_INT_STATUS			(0x00)
#define PL080_TC_STATUS				(0x04)
#define PL080_TC_CLEAR				(0x08)
#define PL080_ERR_STATUS			(0x0C)
#define PL080_ERR_CLEAR				(0x10)
#define PL080_RAW_TC_STATUS			(0x14)
#define PL080_RAW_ERR_STATUS			(0x18)
#define PL080_EN_CHAN				(0x1c)
#define PL080_SOFT_BREQ				(0x20)
#define PL080_SOFT_SREQ				(0x24)
#define PL080_SOFT_LBREQ			(0x28)
#define PL080_SOFT_LSREQ			(0x2C)

#define PL080_CONFIG				(0x30)
#define PL080_CONFIG_M2_BE			(1 << 2)
#define PL080_CONFIG_M1_BE			(1 << 1)
#define PL080_CONFIG_ENABLE			(1 << 0)

#define PL080_SYNC				(0x34)

/* Per channel configuration registers */
#define PL080_Cx_BASE(Cx)			((0x100 + (Cx * 0x20)))
#define PL080_Cx_SRC_ADDR(Cx)			((0x100 + (Cx * 0x20)))
#define PL080_Cx_DST_ADDR(Cx)			((0x104 + (Cx * 0x20)))
#define PL080_Cx_LLI(Cx)			((0x108 + (Cx * 0x20)))
#define PL080_Cx_CONTROL(Cx)			((0x10C + (Cx * 0x20)))
#define PL080_Cx_CONFIG(Cx)			((0x110 + (Cx * 0x20)))

#define PL080_CONTROL_TC_IRQ_EN			(1 << 31)
#define PL080_CONTROL_PROT_MASK			(0x7 << 28)
#define PL080_CONTROL_PROT_SHIFT		(28)
#define PL080_CONTROL_PROT_CACHE		(1 << 30)
#define PL080_CONTROL_PROT_BUFF			(1 << 29)
#define PL080_CONTROL_PROT_SYS			(1 << 28)
#define PL080_CONTROL_DST_INCR			(1 << 27)
#define PL080_CONTROL_SRC_INCR			(1 << 26)
#define PL080_CONTROL_DST_AHB2			(1 << 25)
#define PL080_CONTROL_SRC_AHB2			(1 << 24)
#define PL080_CONTROL_DWIDTH_MASK		(0x7 << 21)
#define PL080_CONTROL_DWIDTH_SHIFT		(21)
#define PL080_CONTROL_SWIDTH_MASK		(0x7 << 18)
#define PL080_CONTROL_SWIDTH_SHIFT		(18)
#define PL080_CONTROL_DB_SIZE_MASK		(0x7 << 15)
#define PL080_CONTROL_DB_SIZE_SHIFT		(15)
#define PL080_CONTROL_SB_SIZE_MASK		(0x7 << 12)
#define PL080_CONTROL_SB_SIZE_SHIFT		(12)
#define PL080_CONTROL_TRANSFER_SIZE_MASK	(0xfff << 0)
#define PL080_CONTROL_TRANSFER_SIZE_SHIFT	(0)

#define PL080_CONFIG_HALT			(1 << 18)
#define PL080_CONFIG_ACTIVE			(1 << 17)
#define PL080_CONFIG_LOCK			(1 << 16)
#define PL080_CONFIG_TC_IRQ_BIT			(15)
#define PL080_CONFIG_ERR_IRQ_BIT		(14)
#define PL080_CONFIG_FLOW_CONTROL_MASK		(0x7 << 11)
#define PL080_CONFIG_FLOW_CONTROL_SHIFT		(11)
#define PL080_CONFIG_DST_SEL_MASK		(0xf << 6)
#define PL080_CONFIG_DST_SEL_SHIFT		(6)
#define PL080_CONFIG_SRC_SEL_MASK		(0xf << 1)
#define PL080_CONFIG_SRC_SEL_SHIFT		(1)
#define PL080_CONFIG_ENABLE			(1 << 0)

#define PL080_BSIZE_1				(0x0)
#define PL080_BSIZE_4				(0x1)
#define PL080_BSIZE_8				(0x2)
#define PL080_BSIZE_16				(0x3)
#define PL080_BSIZE_32				(0x4)
#define PL080_BSIZE_64				(0x5)
#define PL080_BSIZE_128				(0x6)
#define PL080_BSIZE_256				(0x7)

#define PL080_AHB_M1_BIG_ENDIAN			(0x02)
#define PL080_AHB_M2_BIG_ENDIAN			(0x04)

#define PL080_CHANNEL_UNUSED			(0x00)
#define PL080_CHANNEL_BUSY			(0x01)
#define PL080_CHANNEL_ALLOCATED			(0x02)

#define PL080_TRANSFER_WIDTH			(4)

#define ADDRESS_IN_CACHED_MEMORY(A)	\
	((A >= CONFIG_SRAM_BASE_ADDRESS) &&	\
	 (A < (CONFIG_SRAM_BASE_ADDRESS + KB(CONFIG_SRAM_SIZE))))

#define DMA_CHANNEL_SEMA_WAIT_TIME		(1000)

static s32_t dma_pl080_init(struct device *dev);

/*
 * @brief pl080 private structure
 *
 * @param dma_user_cb - Dma user call back after completion
 * @param lli_ptr - Linked list pointers
 * @param ch_status - Dma channel request status
 * @param pl080_access - Semaphore
 * @param ch_access channel access semaphore
 */
struct dma_pl080_data {
	void (*dma_user_cb[DMA_MAX_NUM_CHAN])(struct device *dev,
					     u32_t channel_id,
					     s32_t error_code);
	struct pl080_lli *lli_ptr[DMA_MAX_NUM_CHAN];
	u8_t ch_status[DMA_MAX_NUM_CHAN];
	struct k_sem pl080_access;
	struct k_sem ch_access[DMA_MAX_NUM_CHAN];
};

struct dma_pl080_config {
	u32_t base;
};

/* DMA linked list chain structure */
struct pl080_lli {
	u32_t src_addr;
	u32_t dst_addr;
	u32_t next_lli;
	u32_t control0;
};

/**
 * @brief Copy memory using DMA without OS calls
 *
 * @param dev Device struct
 * @param ch Channel number
 * @param cfg Dma config struct
 *
 * @return 0 for success, error otherwise
 * @note This function is relocated to RAM using "text.fastcode" attribute.
 */
__attribute__((__section__("text.fastcode")))
s32_t dma_memcpy(struct device *dev, u32_t ch, struct dma_config *cfg)
{
	struct dma_block_config *blk;
	/* dma_memcpy() should not access any flash memory and the config
	 * structure happens to be located in flash as it is a read only
	 * structure. Hence this api uses the base address
	 * directly without accessing the config structure.
	 */
	u32_t base = DMA_BASE_ADDR;
	u32_t val, trfr_width, trfr_align, trfr_size;

	ARG_UNUSED(dev);

	if (!(cfg) || !(cfg->head_block) || (ch >= DMA_MAX_NUM_CHAN))
		return -EINVAL;

	val = sys_read32(base + PL080_EN_CHAN);
	if (val & (1 << ch))
		return -EBUSY;

	blk = cfg->head_block;
	sys_write32(blk->source_address, (base + PL080_Cx_SRC_ADDR(ch)));
	sys_write32(blk->dest_address, (base + PL080_Cx_DST_ADDR(ch)));
	sys_write32(0, (base + PL080_Cx_LLI(ch)));

	val = PL080_CONTROL_SRC_INCR | PL080_CONTROL_DST_INCR;

	/* Get the alignment for the transfer from src, dst and size */
	trfr_align = blk->block_size | blk->source_address | blk->dest_address;
	trfr_align &= 0x3;

	switch (trfr_align) {
	case 0x0: /* 4 byte aligned */
		trfr_width = 0x2;
		trfr_size = blk->block_size >> 2;
		break;
	case 0x2: /* 2 byte aligned */
		trfr_width = 0x1;
		trfr_size = blk->block_size >> 1;
		break;
	case 0x1: /* 1 byte aligned */
	case 0x3:
	default:
		trfr_width = 0x0;
		trfr_size = blk->block_size;
		break;
	}

	/* Src and Dst Width */
	val |= (trfr_width << PL080_CONTROL_SWIDTH_SHIFT) |
			(trfr_width << PL080_CONTROL_DWIDTH_SHIFT);

	/* Size */
	val |= trfr_size;

	/* Set burst size to 256 */
	val |= PL080_BSIZE_256 << PL080_CONTROL_DB_SIZE_SHIFT;
	val |= PL080_BSIZE_256 << PL080_CONTROL_SB_SIZE_SHIFT;

	sys_write32(val, base + PL080_Cx_CONTROL(ch));

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* If Source buffer is in cached memory clean it and
	 * If Destination buffer is in cached memory invalidate it
	 */
	if (ADDRESS_IN_CACHED_MEMORY(blk->source_address))
		clean_dcache_by_addr((u32_t)blk->source_address,
				     blk->block_size);

	if (ADDRESS_IN_CACHED_MEMORY(blk->dest_address)) {
		/* Check buffer alignment for destination address
		 * See CS-4092 for details
		 */
		if ((u32_t)blk->dest_address & (DCACHE_LINE_SIZE - 1))
			return -EINVAL;

		invalidate_dcache_by_addr((u32_t)blk->dest_address,
					  blk->block_size);
	}
#endif

	sys_write32(PL080_CONFIG_ENABLE, base + PL080_CONFIG);
	val = sys_read32(base + PL080_Cx_CONFIG(ch));
	val |= PL080_CONFIG_ENABLE;
	sys_write32(val, (base + PL080_Cx_CONFIG(ch)));

	/* Wait for DMA to finish */
	do {
		val = sys_read32(base + PL080_EN_CHAN);
	} while (val & (1 << ch));

	return 0;
}

/**
 * @brief Get the channel request status
 *
 * @param dev Device struct
 * @param channel Channel number
 *
 * @return 0 for Idle, status otherwise
 */
static s32_t dma_channel_request_status(struct device *dev, u32_t channel)
{
	struct dma_pl080_data *priv = dev->driver_data;

	return priv->ch_status[channel];
}

/**
 * @brief Get a free channel
 *
 * @param dev Device struct
 * @param channel Pointer to channel number
 * @param type Normal or allocated channel
 *
 * @return 0 for success, error otherwise
 */
static s32_t dma_pl080_channel_request(struct device *dev, u32_t *channel,
				       u8_t type)
{
	struct dma_pl080_data *priv = dev->driver_data;
	s32_t ret = 0;
	u8_t ch;

	if (channel == NULL)
		return -EINVAL;

	if (!k_is_in_isr()) {
		ret = k_sem_take(&priv->pl080_access, DMA_CHANNEL_SEMA_WAIT_TIME);
		if (ret)
			return ret;
	}

	for (ch = 0; ch < DMA_MAX_NUM_CHAN; ch++) {
		if (priv->ch_status[ch] == PL080_CHANNEL_UNUSED) {
			if (type == NORMAL_CHANNEL) {
				priv->ch_status[ch] = PL080_CHANNEL_BUSY;
				*channel = ch;
			} else if (type == ALLOCATED_CHANNEL) {
				priv->ch_status[ch] = PL080_CHANNEL_ALLOCATED;
				*channel = ch;
			} else {
				ret = -EINVAL;
			}
			break;
		}
	}

	if (ch == DMA_MAX_NUM_CHAN)
		ret = -EAGAIN;

	if (!k_is_in_isr())
		k_sem_give(&priv->pl080_access);
	return ret;
}

/**
 * @brief Release the channel
 *
 * @param dev Device struct
 * @param channel Channel number
 *
 * @return 0 for success, error otherwise
 */
static s32_t dma_pl080_channel_release(struct device *dev, u32_t channel)
{
	struct dma_pl080_data *priv = dev->driver_data;
	s32_t ret = 0;

	if (channel < DMA_MAX_NUM_CHAN) {
		if (!k_is_in_isr()) {
			ret = k_sem_take(&priv->pl080_access,
						DMA_CHANNEL_SEMA_WAIT_TIME);
			if (ret)
				return ret;
			priv->ch_status[channel] = PL080_CHANNEL_UNUSED;
			k_sem_give(&priv->pl080_access);
		} else {
			priv->ch_status[channel] = PL080_CHANNEL_UNUSED;
		}
		return 0;
	}

	return -EINVAL;
}

/**
 * @brief Get burst val corresponding to given burst size
 *
 * @param size Size to be converted
 * @param val Value to be programmed
 *
 * @return 0 for success, error otherwise
 */
static s32_t dma_pl080_burst_val(u32_t size, u32_t *val)
{
	s32_t ret = 0;

	switch (size) {
	case 1:
		*val = PL080_BSIZE_1;
		break;
	case 4:
		*val = PL080_BSIZE_4;
		break;
	case 8:
		*val = PL080_BSIZE_8;
		break;
	case 16:
		*val = PL080_BSIZE_16;
		break;
	case 32:
		*val = PL080_BSIZE_32;
		break;
	case 64:
		*val = PL080_BSIZE_64;
		break;
	case 128:
		*val = PL080_BSIZE_128;
		break;
	case 256:
		*val = PL080_BSIZE_256;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_DATA_CACHE_SUPPORT
/*
 * @brief Invalidate all destination buffers in the DMA chain
 */
void invalidate_dma_dest_buffers(struct device *dev, u8_t ch)
{
	struct dma_pl080_data *priv = dev->driver_data;
	struct pl080_lli *lli = priv->lli_ptr[ch];

	while (lli) {
		u32_t size;

		size = lli->control0 & PL080_CONTROL_TRANSFER_SIZE_MASK;
		size <<= (lli->control0 & PL080_CONTROL_DWIDTH_MASK) >>
			 PL080_CONTROL_DWIDTH_SHIFT;
		invalidate_dcache_by_addr(lli->dst_addr, size);
		lli = (struct pl080_lli *)lli->next_lli;
	}
}
#endif

/**
 * @brief Configure the given DMA channel
 *
 * @param dev Device struct
 * @param ch Channel to be configured
 * @param cfg Dma config struct
 *
 * @return 0 for success, error otherwise
 */
static s32_t dma_pl080_chan_config(struct device *dev, u32_t ch,
				   struct dma_config *cfg)
{
	const struct dma_pl080_config *config = dev->config->config_info;
	struct dma_pl080_data *priv = dev->driver_data;
	u32_t block_count;
	struct dma_block_config *blk;
	struct pl080_lli *lli = NULL;
	u32_t base = config->base;
	u32_t cnt, val;
	u32_t ctrl;
	s32_t ret = 0;

	if (!(cfg) || !(cfg->head_block) || (ch >= DMA_MAX_NUM_CHAN))
		return -EINVAL;

	if (!k_is_in_isr()) {
		ret = k_sem_take(&priv->ch_access[ch], DMA_CHANNEL_SEMA_WAIT_TIME);
		if (ret)
			return ret;
	}

	if (dma_channel_request_status(dev, ch) == PL080_CHANNEL_UNUSED) {
		ret = -EPERM;
		goto out;
	}

	block_count = cfg->block_count;

	val = sys_read32(base + PL080_EN_CHAN);
	if (val & (1 << ch)) {
		ret = -EBUSY;
		goto out;
	}

	/* Free up any previous memory allocations and get new allocation */
	if (priv->lli_ptr[ch])
		cache_line_aligned_free(priv->lli_ptr[ch]);
	priv->lli_ptr[ch] = 0;
	/* Allocate cache line size aligned address as this region
	 * is read by the DMA controller.
	 */
	lli = cache_line_aligned_alloc(
			block_count * sizeof(struct pl080_lli));
	if (!lli) {
		ret = -ENOMEM;
		goto out;
	}

	blk = cfg->head_block;
	sys_write32(blk->source_address, (base + PL080_Cx_SRC_ADDR(ch)));
	sys_write32(blk->dest_address, (base + PL080_Cx_DST_ADDR(ch)));
	sys_write32(0, (base + PL080_Cx_LLI(ch)));

	val = 0;
	val |= cfg->channel_direction << PL080_CONFIG_FLOW_CONTROL_SHIFT;
	val |= cfg->source_peripheral << PL080_CONFIG_SRC_SEL_SHIFT;
	val |= cfg->dest_peripheral << PL080_CONFIG_DST_SEL_SHIFT;
	sys_write32(val, base + PL080_Cx_CONFIG(ch));

	for (cnt = 0; cnt < block_count; cnt++) {
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* If Source buffer is in cached memory clean it and
		 * If Destination buffer is in cached memory clean it before
		 * the DMA and invalidate it after the DMA completes
		 */
		if (ADDRESS_IN_CACHED_MEMORY(blk->source_address))
			clean_dcache_by_addr((u32_t)blk->source_address,
					     blk->block_size);

		if (ADDRESS_IN_CACHED_MEMORY(blk->dest_address))
			clean_dcache_by_addr((u32_t)blk->dest_address,
					     blk->block_size);
#endif

		ctrl = 0;
		/* Align the block size to data size. Get the transfer size */
		val = blk->block_size;
		if (val % cfg->source_data_size)
			val = val + (cfg->source_data_size - 1);
		val = val / cfg->source_data_size;
		ctrl |= val & PL080_CONTROL_TRANSFER_SIZE_MASK;

		/* 1 byte - 0x00, 2 byte - 0x01, 4 byte - 0x10 */
		val = cfg->source_data_size >> 1;
		ctrl |= val << PL080_CONTROL_SWIDTH_SHIFT;
		val = cfg->dest_data_size >> 1;
		ctrl |= val << PL080_CONTROL_DWIDTH_SHIFT;

		/* burst size set */
		ret = dma_pl080_burst_val(cfg->source_burst_length, &val);
		if (ret)
			goto out;
		ctrl |= val << PL080_CONTROL_SB_SIZE_SHIFT;

		ret = dma_pl080_burst_val(cfg->dest_burst_length, &val);
		if (ret)
			goto out;
		ctrl |= val << PL080_CONTROL_DB_SIZE_SHIFT;

		ctrl |= PL080_CONTROL_SRC_AHB2;
		if (blk->source_addr_adj == ADDRESS_INCREMENT)
			ctrl |= PL080_CONTROL_SRC_INCR;
		if (blk->dest_addr_adj == ADDRESS_INCREMENT)
			ctrl |= PL080_CONTROL_DST_INCR;

		if (cnt == 0) {
			/* Enable interrupt if no linked list */
			if ((cfg->complete_callback_en) && (block_count == 1))
				ctrl |= PL080_CONTROL_TC_IRQ_EN;
			sys_write32(ctrl, base + PL080_Cx_CONTROL(ch));

			if (block_count > 1) {
				val = (u32_t)&lli[cnt + 1].src_addr;
				sys_write32(val, (base + PL080_Cx_LLI(ch)));
			}
		}

		if (cfg->endian_mode & PL080_AHB_M1_BIG_ENDIAN) {
			if (cnt)
				sys_put_be32((u32_t)&lli[cnt].src_addr,
					     (u8_t *)&lli[cnt - 1].next_lli);
			sys_put_be32(blk->source_address,
				(u8_t *) &lli[cnt].src_addr);
			sys_put_be32(blk->dest_address,
				(u8_t *) &lli[cnt].dst_addr);
			sys_put_be32(ctrl, (u8_t *)&lli[cnt].control0);
		} else {
			if (cnt)
				lli[cnt-1].next_lli = (u32_t)&lli[cnt].src_addr;
			lli[cnt].src_addr = blk->source_address;
			lli[cnt].dst_addr = blk->dest_address;
			lli[cnt].control0 = ctrl;
		}
		lli[cnt].next_lli = 0;

		if (blk->next_block)
			blk = blk->next_block;
		else
			break;
	}
	val = cfg->endian_mode | PL080_CONFIG_ENABLE;
	sys_write32(val, base + PL080_CONFIG);

	/* Enable interrupts */
	if (cfg->complete_callback_en || cfg->error_callback_en)
		priv->dma_user_cb[ch] = cfg->dma_callback;

	if (cfg->complete_callback_en) {
		sys_set_bit((base + PL080_Cx_CONFIG(ch)),
			    PL080_CONFIG_TC_IRQ_BIT);
		if (lli)
			lli[block_count - 1].control0 |=
				 PL080_CONTROL_TC_IRQ_EN;
	}

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Flush the linked list buffer to make sure the DMA controller
	 * sees the data written to it.
	 */
	if (lli) {
		clean_dcache_by_addr((u32_t)lli,
			block_count * sizeof(struct pl080_lli));
	}
#endif

	if (cfg->error_callback_en)
		sys_set_bit((base + PL080_Cx_CONFIG(ch)),
			    PL080_CONFIG_ERR_IRQ_BIT);

	/* Save pointer to release after dma finished */
	priv->lli_ptr[ch] = lli;
	lli = NULL;
out:
	if (lli)
		cache_line_aligned_free(lli);

	if (!k_is_in_isr())
		k_sem_give(&priv->ch_access[ch]);

	return ret;
}

/**
 * @brief Start the given DMA channel
 *
 * @param dev Device struct
 * @param channel Channel to be started
 *
 * @return 0 for success
 */
static s32_t dma_pl080_start(struct device *dev, u32_t channel)
{
	struct dma_pl080_data *priv = dev->driver_data;
	const struct dma_pl080_config *config = dev->config->config_info;
	u32_t val;
	s32_t ret = 0;

	if (channel >= DMA_MAX_NUM_CHAN)
		return -EINVAL;

	if (!k_is_in_isr()) {
		ret = k_sem_take(&priv->ch_access[channel], DMA_CHANNEL_SEMA_WAIT_TIME);
		if (ret)
			return ret;
	}

	if (dma_channel_request_status(dev, channel) == PL080_CHANNEL_UNUSED) {
		ret = -EPERM;
		goto out;
	}

	val = sys_read32(config->base + PL080_Cx_CONFIG(channel));
	val |= PL080_CONFIG_ENABLE;
	sys_write32(val, (config->base + PL080_Cx_CONFIG(channel)));
out:
	if (!k_is_in_isr())
		k_sem_give(&priv->ch_access[channel]);

	return ret;
}

/**
 * @brief Stop the given DMA channel
 *
 * @param dev Device struct
 * @param ch Channel to be stopped
 *
 * @return 0 for success
 */
static s32_t dma_pl080_stop(struct device *dev, u32_t channel)
{
	struct dma_pl080_data *priv = dev->driver_data;
	const struct dma_pl080_config *cfg = dev->config->config_info;
	u32_t val;
	s32_t ret = 0;

	if (channel >= DMA_MAX_NUM_CHAN)
		return -EINVAL;

	if (!k_is_in_isr()) {
		ret = k_sem_take(&priv->ch_access[channel], DMA_CHANNEL_SEMA_WAIT_TIME);
		if (ret)
			return ret;
	}

	if (dma_channel_request_status(dev, channel) == PL080_CHANNEL_UNUSED) {
		ret = -EPERM;
		goto out;
	}

	/* Disable interrupts */
	sys_clear_bit((cfg->base + PL080_Cx_CONFIG(channel)),
			    PL080_CONFIG_TC_IRQ_BIT);

	sys_clear_bit((cfg->base + PL080_Cx_CONFIG(channel)),
			    PL080_CONFIG_ERR_IRQ_BIT);

	val = sys_read32(cfg->base + PL080_Cx_CONFIG(channel));
	val &= ~PL080_CONFIG_ENABLE;
	sys_write32(val, (cfg->base + PL080_Cx_CONFIG(channel)));
out:
	if (!k_is_in_isr())
		k_sem_give(&priv->ch_access[channel]);

	return ret;
}

/**
 * @brief Get the status of given DMA channel
 *
 * @param dev Device struct
 * @param channel Channel number
 * @status Channel status 0 for idle
 *
 * @return 0 for success
 */
static s32_t dma_pl080_status(struct device *dev, u32_t channel, u32_t *status)
{
	const struct dma_pl080_config *config = dev->config->config_info;
	u32_t val;

	if (status == NULL)
		return -EINVAL;

	val = sys_read32(config->base + PL080_EN_CHAN);
	if (val & (1 << channel)) {
		*status = 1;
	} else {
		*status = 0;
#ifdef CONFIG_DATA_CACHE_SUPPORT
		invalidate_dma_dest_buffers(dev, channel);
#endif
	}

	return 0;
}

/**
 * @brief ISR routine for DMA
 *
 * @param arg Void pointer
 *
 * @return void
 */
static void dma_pl080_isr(void *arg)
{
	const struct dma_pl080_config *cfg;
	struct dma_pl080_data *priv;
	struct device *dev = arg;
	u32_t base, ch;
	bool free_ch;

	priv = dev->driver_data;
	cfg = dev->config->config_info;
	base = cfg->base;
	for (ch = 0; ch < DMA_MAX_NUM_CHAN; ch++) {
		free_ch = false;

		if (sys_test_bit((base + PL080_TC_STATUS), ch)) {
			sys_write32((1 << ch), (base + PL080_TC_CLEAR));

			if (priv->dma_user_cb[ch]) {
#ifdef CONFIG_DATA_CACHE_SUPPORT
				invalidate_dma_dest_buffers(dev, ch);
#endif
				priv->dma_user_cb[ch](dev, ch, 0);
			}

			free_ch = true;
		}

		if (sys_test_bit((base + PL080_ERR_STATUS), ch)) {
			sys_write32((1 << ch), (base + PL080_ERR_CLEAR));

			if (priv->dma_user_cb[ch])
				priv->dma_user_cb[ch](dev, ch, -1);

			free_ch = true;
		}

		if (free_ch == true)
			if (priv->ch_status[ch] != PL080_CHANNEL_ALLOCATED)
				priv->ch_status[ch] = PL080_CHANNEL_UNUSED;
	}
}

static const struct dma_driver_api dma_funcs = {
	.config = dma_pl080_chan_config,
	.start = dma_pl080_start,
	.stop = dma_pl080_stop,
	.status = dma_pl080_status,
	.request = dma_pl080_channel_request,
	.release = dma_pl080_channel_release
};

static struct dma_pl080_data pl080_dev_data;

static const struct dma_pl080_config pl080_dev_config = {
	.base = DMA_BASE_ADDR,
};

DEVICE_AND_API_INIT(dma_pl080, CONFIG_DMA_PL080_NAME, dma_pl080_init,
		    &pl080_dev_data, &pl080_dev_config, PRE_KERNEL_2,
		    CONFIG_DMA_INIT_PRIORITY, &dma_funcs);

/**
 * @brief Dma init
 *
 * @param dev Device struct
 *
 * @return 0 for success
 */
static s32_t dma_pl080_init(struct device *dev)
{
	struct dma_pl080_data *priv = dev->driver_data;
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	u8_t ch;

	dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_DMA, DMU_DEVICE_RESET);
	k_sem_init(&priv->pl080_access, 1, 1);

	for (ch = 0; ch < DMA_MAX_NUM_CHAN; ch++) {
		priv->ch_status[ch] = PL080_CHANNEL_UNUSED;
		priv->lli_ptr[ch] = 0;
		k_sem_init(&priv->ch_access[ch], 1, 1);
	}

	IRQ_CONNECT(SPI_IRQ(DMA_TC0_IRQ), CONFIG_DMA_IRQ_PRI,
		dma_pl080_isr, DEVICE_GET(dma_pl080), 0);
	irq_enable(SPI_IRQ(DMA_TC0_IRQ));

	IRQ_CONNECT(SPI_IRQ(DMA_ERR0_IRQ), CONFIG_DMA_IRQ_PRI,
		dma_pl080_isr, DEVICE_GET(dma_pl080), 0);
	irq_enable(SPI_IRQ(DMA_ERR0_IRQ));

	return 0;
}
