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
 * @file sc_cit.c
 * @brief Smart card driver implementation for bcm58202
 */

#include <arch/cpu.h>
#include <board.h>
#include <broadcom/clock_control.h>
#include <device.h>
#include <dmu.h>
#include <genpll.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>
#include <sc/sc.h>
#include <sc/sc_datatypes.h>
#include "sc_bcm58202.h"

#define EXCEPTION_SMART_CARD				(32 + 41)
#define EXCEPTION_SMART_CARD_PRIORITY			(0)
/*
 * @brief BCM58202 config structure
 *
 * @param base - base address for the channel
 */
struct cit_sc_config {
	u32_t base[CIT_MAX_SUPPORTED_CHANNELS];
};

/*
 * @brief BCM58202 private structure
 *
 * @param ch_handle - handle to channel data
 */
struct cit_sc_data {
	struct ch_handle handle[CIT_MAX_SUPPORTED_CHANNELS];
};

static s32_t cit_sc_channel_open(struct device *dev, u32_t channel,
				 struct sc_channel_param *ch_param)
{
	const struct cit_sc_config *cfg = dev->config->config_info;
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle;

	handle = &priv->handle[channel];
	if (handle->open == true)
		return -EALREADY;

	memset(handle, 0, sizeof(struct ch_handle));
	handle->base = cfg->base[channel];

	/* Enable SCI core IRQ on the chip  */
	sys_write32(1, (handle->base + CIT_SCIRQ0_SCIRQEN));

	return cit_channel_open(handle, ch_param);
}

static s32_t cit_sc_channel_close(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_close(handle);
}

static s32_t cit_sc_channel_deactivate(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_deactivate(handle);
}

static s32_t cit_sc_channel_param_set(struct device *dev, u32_t channel,
				      struct sc_channel_param *ch_param)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_set_parameters(handle, ch_param);
}

static s32_t cit_sc_channel_param_get(struct device *dev, u32_t channel,
				      enum param_type type,
				      struct sc_channel_param *ch_param)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];
	s32_t rv = 0;

	switch (type) {
	case CURRENT_SETTINGS:
		rv = cit_channel_get_parameters(handle, ch_param);
		break;

	case ATR_NEG_SETTINGS:
		rv = cit_channel_get_nego_parameters(handle, ch_param);
		break;

	case DEFAULT_SETTINGS:
		rv = cit_get_default_channel_settings(channel, ch_param);
		break;
	default:
		rv = -EPERM;
	}

	return rv;
}

static s32_t cit_sc_channel_set_vcc_level(struct device *dev,  u32_t channel,
					  enum sc_vcc_level level)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_set_vcclevel(handle, level);
}

static s32_t cit_sc_channel_status_get(struct device *dev,  u32_t channel,
				       struct sc_status *status)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_get_status(handle, status);
}

static s32_t cit_sc_channel_interface_reset(struct device *dev, u32_t channel,
					    enum sc_reset_type type)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_reset_ifd(handle, type);
}

static s32_t cit_sc_channel_card_reset(struct device *dev, u32_t channel,
				       u8_t decode_atr)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_card_reset(handle, decode_atr);
}

static s32_t cit_sc_channel_perform_pps(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_pps(handle);
}

static s32_t cit_sc_card_power_up(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_card_power_up(handle);
}

static s32_t cit_sc_card_power_down(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_card_power_down(handle);
}

static s32_t cit_sc_channel_transceive(struct device *dev, enum pdu_type pdu,
				       struct sc_transceive *trx)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[trx->channel];
	s32_t rv;

	if (trx->rx == NULL) {
		/* Only Tx */
		rv = cit_ch_transmit(handle, trx->tx, trx->tx_len);
	} else if (trx->tx == NULL) {
		/* Only Rx */
		rv = cit_ch_receive(handle, trx->rx, &trx->rx_len,
				    trx->max_rx_len);
	} else {
		/* Tx and Rx */
		if (pdu == SC_TPDU)
			rv = cit_tpdu_transceive(handle, trx);
		else if (pdu == SC_APDU)
			rv = cit_apdu_transceive(handle, trx);
		else
			rv = -ENOTSUP;
	}

	return rv;
}

static s32_t cit_sc_channel_card_detect(struct device *dev, u32_t channel,
					enum sc_card_present status, bool block)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_detect_card(handle, status, block);
}

static s32_t cit_sc_channel_card_detect_cb_set(struct device *dev,
					       u32_t channel,
					       enum sc_card_present status,
					       sc_api_card_detect_cb cb)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_detect_cb_set(handle, status, cb);
}

static s32_t cit_sc_channel_set_card_type(struct device *dev, u32_t channel,
					  u32_t card_type)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_set_card_type(handle, card_type);
}

static s32_t cit_sc_channel_apdu_set_objects(struct device *dev, u32_t channel,
					     s8_t *obj_id, u32_t obj_id_len)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_apdu_set_objects(handle, obj_id, obj_id_len);
}

static s32_t cit_sc_channel_apdu_get_objects(struct device *dev, u32_t channel,
					     s8_t *obj_id, u32_t obj_id_len)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_apdu_get_objects(handle, obj_id, obj_id_len);
}

static s32_t cit_sc_channel_apdu_commands(struct device *dev, u32_t channel,
					  u32_t apducmd, u8_t *buf, u32_t len)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_apdu_commands(handle, apducmd, buf, len);
}

static s32_t cit_sc_channel_time_set(struct device *dev,
				     struct sc_channel_param *ch_param,
				     enum time_type type,
				     struct sc_time *time)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[0];
	s32_t rv = -ENOTSUP;

	if (type == BLK_WAIT_TIME) {
		if (time->val == SC_RESET_BLOCK_WAITING_TIME)
			rv = cit_channel_reset_bwt(handle);
		else
			rv = cit_channel_set_bwt_integer(ch_param, time->val);
	}

	if ((type == BLK_WAIT_TIME_EXT) &&
			 (time->unit == SC_TIMER_UNIT_ETU))
		rv = cit_channel_bwt_ext_set(handle, time->val);

	if ((type == WORK_WAIT_TIME) &&
	    (time->val == 0)) {
		/* Disable the work wait timer */
		struct cit_timer timer = {CIT_WAIT_TIMER, CIT_WAIT_TIMER_WWT,
								false, false};

		rv = cit_timer_isr_control(handle, &timer);
	}

	return rv;
}

static s32_t cit_sc_channel_read_atr(struct device *dev, bool from_hw,
				     struct sc_transceive *trx)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[trx->channel];
	s32_t rv = 0;
	u32_t len = 0;

	if (from_hw) {
		/* Read from receive buf of controller */
		rv = cit_channel_receive_atr(handle, trx->rx, &trx->rx_len,
						trx->max_rx_len);
	} else {
		len = handle->rx_len;
		if (handle->rx_len > trx->max_rx_len)
			len = trx->max_rx_len;

		memcpy(trx->rx, handle->auc_rx_buf, len);
		trx->rx_len = len;
	}
	return rv;
}

static s32_t cit_sc_adjust_param_with_f_d(struct device *dev,
					  struct sc_channel_param *ch_param,
					  u8_t f_val, u8_t d_val)
{
	ARG_UNUSED(dev);

	return sc_f_d_adjust_without_update(ch_param, f_val, d_val);
}

static s32_t cit_sc_channel_enable_interrupts(struct device *dev, u32_t channel)
{
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle = &priv->handle[channel];

	return cit_channel_enable_interrupts(handle);
}

/**
 * @brief ISR routine for Smart card
 *
 * @param arg Void pointer
 *
 * @return void
 */
static void cit_sc_handler_isr(void *arg)
{
	struct device *dev = arg;
	struct cit_sc_data *priv = dev->driver_data;
	struct ch_handle *handle;

	handle = &priv->handle[0];
	cit_channel_handler_isr(handle);
}

static const struct sc_driver_api sc_funcs = {
	.channel_open = cit_sc_channel_open,
	.channel_close = cit_sc_channel_close,
	.channel_deactivate = cit_sc_channel_deactivate,
	.channel_param_set = cit_sc_channel_param_set,
	.channel_param_get = cit_sc_channel_param_get,
	.channel_enable_interrupts = cit_sc_channel_enable_interrupts,
	.channel_set_vcc_level = cit_sc_channel_set_vcc_level,
	.channel_status_get = cit_sc_channel_status_get,
	.channel_time_set = cit_sc_channel_time_set,
	.channel_read_atr = cit_sc_channel_read_atr,
	.channel_interface_reset = cit_sc_channel_interface_reset,
	.channel_card_reset = cit_sc_channel_card_reset,
	.channel_perform_pps = cit_sc_channel_perform_pps,
	.channel_card_power_up = cit_sc_card_power_up,
	.channel_card_power_down = cit_sc_card_power_down,
	.channel_transceive = cit_sc_channel_transceive,
	.channel_card_detect_cb_set = cit_sc_channel_card_detect_cb_set,
	.channel_card_detect = cit_sc_channel_card_detect,
	.channel_set_card_type = cit_sc_channel_set_card_type,
	.channel_apdu_commands = cit_sc_channel_apdu_commands,
	.channel_apdu_set_objects = cit_sc_channel_apdu_set_objects,
	.channel_apdu_get_objects = cit_sc_channel_apdu_get_objects,
	.adjust_param_with_f_d = cit_sc_adjust_param_with_f_d
};

static const struct cit_sc_config sc_dev_config = {
	.base = {SCA_SC_BASE_ADDR},
};
static struct cit_sc_data sc_dev_data;
static s32_t sc_cit_init(struct device *dev);

DEVICE_AND_API_INIT(smart_card, CONFIG_SC_BCM58202_DEV_NAME, sc_cit_init,
		    &sc_dev_data, &sc_dev_config, POST_KERNEL,
		     CONFIG_SC_INIT_PRIORITY, &sc_funcs);

/**
 * @brief Dma init
 *
 * @param dev Device struct
 *
 * @return 0 for success
 */
static s32_t sc_cit_init(struct device *dev)
{
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	u32_t clk_rate = SC_INTERNAL_CLOCK_FREQ;
	s32_t rv = 0;

	ARG_UNUSED(dev);
	dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_SCI, DMU_DEVICE_BRINGUP);

	/* Set clock rate */
	rv = clk_set_sc(clk_rate);
	if (rv)
		SYS_LOG_ERR("Smart card host clock set error %d", rv);

	rv = clk_get_sc(&clk_rate);
	if (rv)
		SYS_LOG_ERR("Smart card host clock get error %d", rv);

	SYS_LOG_DBG("Smart card host clock %d", clk_rate);

	IRQ_CONNECT(EXCEPTION_SMART_CARD, EXCEPTION_SMART_CARD_PRIORITY,
		cit_sc_handler_isr, DEVICE_GET(smart_card), 0);

	irq_enable(EXCEPTION_SMART_CARD);

	return 0;
}
