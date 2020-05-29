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
 * @file sc_cit_priv.c
 * @brief Smart card private functions
 */

#include <arch/cpu.h>
#include <stdbool.h>
#include <string.h>
#include <board.h>
#include <device.h>
#include <sc/sc.h>
#include <sc/sc_datatypes.h>
#include <sc/sc_protocol.h>
#include <errno.h>
#include <init.h>
#include <limits.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <zephyr/types.h>

#include "sc_bcm58202.h"

#define SC_SYSTEM_EXEC_CYCLES		2000

/* Population count of 1's in a byte */
static const u8_t sc_popcount[] = {
	 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static const int sc_f_value[14] = {
	372, 372, 558, 744, 1116, 1488, 1860, -1,  -1, 512, 768,
	1024, 1536, 2048
};

/**
 * @brief Adjust work wait Time
 *
 * @param handle Channel handle
 * @param wwi Work wait time integer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_adjust_wwt(struct sc_channel_param *ch_param, u8_t wwi)
{
	u8_t baudrate_adjustor;

	SMART_CARD_FUNCTION_ENTER();
	if (ch_param->baudrate == 0)
		return -EINVAL;

	SYS_LOG_DBG("wwt in us = %d", 1000000/ch_param->baudrate);
	baudrate_adjustor = sc_d_value[ch_param->d_factor];

	ch_param->work_wait_time.val = SC_ISO_WORK_WAIT_TIME_DEFAULT_FACTOR *
					baudrate_adjustor * wwi;
	/* For EMV2000 */
	if (ch_param->standard == SC_STANDARD_EMV2000)
		ch_param->work_wait_time.val += baudrate_adjustor *
				SC_DEFAULT_EXTRA_WORK_WAITING_TIME_EMV2000 +
				SC_EMV2000_WORK_WAIT_TIME_DELTA;

	ch_param->work_wait_time.unit = SC_TIMER_UNIT_ETU;
	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Adjust parameters base on given Clock-rate conversion factor and
 *        Bit-rate adjustment factor
 *
 * @param handle Channel handle
 * @param f_val Clock-rate conversion factor
 * @param d_val Bit-rate adjustment factor
 *
 * @return 0 for success, error otherwise
 */
s32_t sc_f_d_adjust(struct ch_handle *handle, u8_t f_val, u8_t d_val)
{
	u8_t val;
	s32_t rv;

	SMART_CARD_FUNCTION_ENTER();
	val = cit_get_prescale(d_val, f_val);
	val = val * handle->ch_param.ext_clkdiv +
		(handle->ch_param.ext_clkdiv - 1);
	handle->ch_param.host_ctrl.prescale = val;
	handle->ch_param.host_ctrl.bauddiv = cit_get_bauddiv(d_val, f_val);
	handle->ch_param.host_ctrl.sc_clkdiv = cit_get_clkdiv(d_val, f_val);
	handle->ch_param.host_ctrl.etu_clkdiv =
					cit_get_etu_clkdiv(d_val, f_val);
	SYS_LOG_DBG("fd_adjust prescale %x bauddiv %x clkdiv %x etu_clkdiv %x",
			handle->ch_param.host_ctrl.prescale,
			handle->ch_param.host_ctrl.bauddiv,
			handle->ch_param.host_ctrl.sc_clkdiv,
			handle->ch_param.host_ctrl.etu_clkdiv);

	rv = cit_program_clocks(handle);

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Program clocks
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_program_clocks(struct ch_handle *handle)
{
	struct host_ctrl_settings *host_ctrl = &handle->ch_param.host_ctrl;
	u8_t sc_clkdiv_encode;
	u32_t base = handle->base;
	u32_t bauddiv_encode;
	u32_t val, cnt;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	/* program prescale register */
	sys_write32(host_ctrl->prescale, base + CIT_PRESCALE);
	SYS_LOG_DBG("prescale 0x%x", host_ctrl->prescale);

	for (cnt = 0; cnt < SC_MAX_BAUDDIV; cnt++) {
		if (host_ctrl->bauddiv == bauddiv_valid[cnt])
			break;
	}
	if ((cnt == SC_MAX_BAUDDIV) ||
			 (host_ctrl->sc_clkdiv >= CIT_CLKDIV_MAX)) {
		rv = -EINVAL;
		goto done;
	}

	bauddiv_encode = cnt;
	sc_clkdiv_encode = cit_clkdiv[host_ctrl->sc_clkdiv];

	/* program CLK_CMD_1 register */
	val = (host_ctrl->etu_clkdiv - 1)  << CIT_CLK_CMD_1_ETU_CLKDIV_SHIFT;

	if (bauddiv_encode & CIT_BAUDDIV_BIT_0)
		val |= BIT(CIT_CLK_CMD_1_BAUDDIV_0_BIT);

	val |= (sc_clkdiv_encode << CIT_CLK_CMD_1_SC_CLKDIV_SHIFT) &
					CIT_CLK_CMD_1_SC_CLKDIV_MASK;

	val |= BIT(CIT_CLK_CMD_1_CLK_EN_BIT);
	sys_write32(val, (handle->base + CIT_CLK_CMD_1));
	SYS_LOG_DBG("CIT_CLK_CMD_1 0x%x", val);

	/* program CLK_CMD_2 register */
	val = 0;
	if (sc_clkdiv_encode & CIT_SC_CLKDIV_BIT_3)
		val |= BIT(CIT_CLK_CMD_2_SC_CLKDIV_3);

	if (bauddiv_encode & CIT_BAUDDIV_BIT_2)
		val |= BIT(CIT_CLK_CMD_2_BAUDDIV_2_BIT);

	val |= CIT_DEFAULT_IF_CLKDIV << CIT_CLK_CMD_2_IF_CLKDIV_SHIFT;
	sys_write32(val, base + CIT_CLK_CMD_2);
	SYS_LOG_DBG("CIT_CLK_CMD_2 0x%x", val);

	if (bauddiv_encode & CIT_BAUDDIV_BIT_1)
		sys_set_bit((base + CIT_FLOW_CMD),
					CIT_FLOW_CMD_BAUDDIV_1_BIT);
	else
		sys_clear_bit((base + CIT_FLOW_CMD),
					CIT_FLOW_CMD_BAUDDIV_1_BIT);
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/* This does NOT modify registers */
/**
 * @brief Adjust parameters base on given Clock-rate conversion factor and
 *        Bit-rate adjustment factor without updating the cotroller registers
 *
 * @param ch_param Channel parameters struct
 * @param f_val Clock-rate conversion factor
 * @param d_val Bit-rate adjustment factor
 *
 * @return 0 for success, error otherwise
 */
s32_t sc_f_d_adjust_without_update(struct sc_channel_param *ch_param,
				   u8_t f_val, u8_t d_val)
{
	u32_t val;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if ((d_val >= SC_MIN_D_FACTOR) && (d_val <= SC_MAX_D_FACTOR) &&
	    (sc_d_value[d_val] != -1)) {
		ch_param->d_factor = d_val;
	} else {
		rv = -EINVAL;
		goto done;
	}

	if ((f_val >= SC_MIN_F_FACTOR) && (f_val <= SC_MAX_F_FACTOR) &&
	    (sc_f_value[f_val] != -1)) {
		ch_param->f_factor = f_val;
	} else {
		rv = -EINVAL;
		goto done;
	}

	ch_param->host_ctrl.etu_clkdiv = cit_get_etu_clkdiv(d_val, f_val);
	ch_param->host_ctrl.sc_clkdiv = cit_get_clkdiv(d_val, f_val);

	val = cit_get_prescale(d_val, f_val);
	val = val * ch_param->ext_clkdiv + (ch_param->ext_clkdiv - 1);
	ch_param->host_ctrl.prescale = val;
	ch_param->host_ctrl.bauddiv = cit_get_bauddiv(d_val, f_val);
	rv = cit_adjust_wwt(ch_param, SC_ISO_DEFAULT_WORK_WAIT_TIME_INTEGER);
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/* Default ISR Callback Functions */
void cit_ch_cardinsert_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.card_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_cardremove_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.card_wait = true;
	handle->wait_event.rcv_wait = true;
	handle->wait_event.tdone_wait = true;
	handle->wait_event.timer_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_rcv_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_atr_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.atr_start = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_wait_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	handle->wait_event.tdone_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_retry_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.tdone_wait = true;
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_timer_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.atr_start = true;
	handle->wait_event.rcv_wait = true;
	handle->wait_event.timer_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_rparity_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_tparity_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.tdone_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_cwt_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_bgt_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.tdone_wait = true;
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_rlen_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_rready_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_tdone_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.tdone_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_event1_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.rcv_wait = true;
	handle->wait_event.event1_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

void cit_ch_event2_cb(void *arg)
{
	struct ch_handle *handle = arg;

	SMART_CARD_FUNCTION_ENTER();
	handle->wait_event.event2_wait = true;
	handle->wait_event.rcv_wait = true;
	SMART_CARD_FUNCTION_EXIT();
}

/* Wait functions for different events */
s32_t cit_ch_wait_cardinsertion(struct ch_handle *handle)
{
	u32_t status1;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	status1 = handle->status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT);
	do {
		if (status1 != BIT(CIT_STATUS_1_CARD_PRES_BIT)) {
			rv = cit_wait_for_event(handle,
				&handle->wait_event.card_wait, K_FOREVER);
			if (rv)
				goto done;
		}
		status1 = handle->status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT);
	} while (status1 != BIT(CIT_STATUS_1_CARD_PRES_BIT));

	if (status1 == BIT(CIT_STATUS_1_CARD_PRES_BIT)) {
		handle->ch_status.card_present = true;
		handle->intr_status1 &= ~BIT(CIT_STATUS_1_CARD_PRES_BIT);
	}
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_ch_wait_cardremove(struct ch_handle *handle)
{
	u32_t status1;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	status1 = handle->status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT);
	cit_sc_exit_critical_section(handle);

	do {
		if (status1 == BIT(CIT_STATUS_1_CARD_PRES_BIT)) {
			rv = cit_wait_for_event(handle,
				&handle->wait_event.card_wait, K_FOREVER);
			if (rv)
				goto done;
		}

		cit_sc_enter_critical_section(handle);
		status1 = handle->status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT);
		cit_sc_exit_critical_section(handle);

	} while  (status1 == BIT(CIT_STATUS_1_CARD_PRES_BIT));

	if (status1 != BIT(CIT_STATUS_1_CARD_PRES_BIT)) {
		handle->ch_status.card_present = false;
		handle->intr_status1 &= ~BIT(CIT_STATUS_1_CARD_PRES_BIT);
	}
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_wait_timer_event(struct ch_handle *handle)
{
	u32_t intr_status1;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	intr_status1 = handle->intr_status1;
	cit_sc_exit_critical_section(handle);

	do {
		if ((handle->card_removed == true) &&
			(intr_status1 & BIT(CIT_INTR_STAT_1_PRES_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_PRES_BIT);
			handle->ch_status.status1 |= SC_RESET_CHANNEL_REQUIRED;
			handle->card_removed = false;
			cit_sc_exit_critical_section(handle);
			rv = -EINVAL;
			SYS_LOG_ERR("TimerEvent: card_removed error\n");
			goto done;
		} else if (!(intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT))) {
			rv = cit_wait_for_event(handle,
						&handle->wait_event.timer_wait,
						handle->ch_param.timeout.val);
			if (rv)
				goto done;
		}

		cit_sc_enter_critical_section(handle);
		intr_status1 = handle->intr_status1;
		cit_sc_exit_critical_section(handle);

	} while ((intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT)) !=
					BIT(CIT_INTR_STAT_1_TIMER_BIT));

	cit_sc_enter_critical_section(handle);
	handle->intr_status1 &=  ~BIT(CIT_INTR_STAT_1_TIMER_BIT);
	cit_sc_exit_critical_section(handle);
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_wait_atr_start(struct ch_handle *handle)
{
	s32_t rv = 0;
	u32_t intr_status1, intr_status2;
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	intr_status1 = handle->intr_status1;
	intr_status2 = handle->intr_status2 & BIT(CIT_INTR_EN_2_ATRS_BIT);
	cit_sc_exit_critical_section(handle);

	do {
		if ((intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT)) &&
							intr_status2) {
			break;
		} else if (intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT)) {
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_TIMER_BIT);
			rv = -ETIMEDOUT;
			goto done;

		} else if ((handle->card_removed == true) &&
			(intr_status1 & BIT(CIT_INTR_STAT_1_PRES_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= BIT(CIT_INTR_STAT_1_PRES_BIT);
			handle->ch_status.status1  |= SC_RESET_CHANNEL_REQUIRED;
			handle->card_removed = false;
			cit_sc_exit_critical_section(handle);
			rv = -EINVAL;
			goto done;
		} else if (intr_status2 != BIT(CIT_INTR_STAT_2_ATRS_BIT)) {
			val = handle->ch_param.timeout.val;
			rv = cit_wait_for_event(handle,
					&handle->wait_event.atr_start, val);
			if (rv) {
				handle->ch_status.status1 |= SC_RX_TIMEOUT;
				rv = -ETIMEDOUT;
				goto done;
			}
		}

		cit_sc_enter_critical_section(handle);
		intr_status1 = handle->intr_status1;
		intr_status2 = handle->intr_status2 &
					 BIT(CIT_INTR_EN_2_ATRS_BIT);
		cit_sc_exit_critical_section(handle);

	} while (intr_status2 != BIT(CIT_INTR_EN_2_ATRS_BIT));

	cit_sc_enter_critical_section(handle);
	handle->intr_status2  &= ~BIT(CIT_INTR_EN_2_ATRS_BIT);
	cit_sc_exit_critical_section(handle);
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_wait_tdone(struct ch_handle *handle)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t intr_status1, intr_status2;
	u32_t val = 0;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	intr_status1 = handle->intr_status1;
	intr_status2 = handle->intr_status2;
	cit_sc_exit_critical_section(handle);

	do {
		if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E1) &&
		    (intr_status1 & BIT(CIT_INTR_STAT_1_BGT_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_BGT_BIT);
			cit_sc_exit_critical_section(handle);
			rv = -EINVAL;
			goto done;
		} else if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E0) &&
			   (intr_status1 & BIT(CIT_INTR_STAT_1_RETRY_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_RETRY_BIT);
			cit_sc_exit_critical_section(handle);

			handle->ch_status.status1 |= SC_TX_PARITY;
			rv = -EINVAL;
			goto done;
		} else if (intr_status2 & BIT(CIT_INTR_STAT_2_WAIT_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_WAIT_BIT);
			cit_sc_exit_critical_section(handle);
			handle->ch_status.status1  |= SC_TX_TIMEOUT;
			rv = -EINVAL;
			goto done;
		} else if ((handle->card_removed == true) &&
			(intr_status1 & BIT(CIT_INTR_STAT_1_PRES_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_PRES_BIT);
			handle->ch_status.status1  |= SC_RESET_CHANNEL_REQUIRED;
			handle->card_removed = false;
			cit_sc_exit_critical_section(handle);
			rv = -EINVAL;
			goto done;
		} else if (!(intr_status1 & BIT(CIT_INTR_STAT_1_TDONE_BIT))) {
			val = handle->ch_param.timeout.val;
			rv = cit_wait_for_event(handle,
					&handle->wait_event.tdone_wait, val);
			if (rv) {
				cit_sc_enter_critical_section(handle);
				handle->ch_status.status1 |= SC_TX_TIMEOUT;
				cit_sc_exit_critical_section(handle);
				rv = -ETIMEDOUT;
				goto done;
			}
		}

		cit_sc_enter_critical_section(handle);
		intr_status1 = handle->intr_status1;
		intr_status2 = handle->intr_status2;
		cit_sc_exit_critical_section(handle);
	} while (((intr_status1 & BIT(CIT_INTR_STAT_1_TDONE_BIT)) !=
				BIT(CIT_INTR_STAT_1_TDONE_BIT)));

	cit_sc_enter_critical_section(handle);
	handle->intr_status1  &= ~BIT(CIT_INTR_STAT_1_TDONE_BIT);
	handle->status1 &= ~BIT(CIT_STATUS_1_TDONE_BIT);
	cit_sc_exit_critical_section(handle);

done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_wait_rcv(struct ch_handle *handle)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t intr_status1, intr_status2, status2;
	u32_t val;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	intr_status1 = handle->intr_status1;
	intr_status2 = handle->intr_status2;
	status2 = handle->status2;
	cit_sc_exit_critical_section(handle);

	do {
		if (intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->ch_status.status1  |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Timer intr error");
			rv = -ETIMEDOUT;
			goto done;
		} else if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E1) &&
			   (intr_status1 & BIT(CIT_INTR_STAT_1_BGT_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1  &= ~BIT(CIT_INTR_STAT_1_BGT_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("BGT intr error");
			rv = -EIO;
			goto done;
		} else if ((intr_status2 & BIT(CIT_INTR_STAT_2_EVENT2_BIT)))  {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &=
					 ~BIT(CIT_INTR_STAT_2_EVENT2_BIT);
			handle->ch_status.status2 |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Event2 intr error");
			rv = -EINVAL;
			goto done;
		} else if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E0) &&
			 (ch_param->standard != SC_STANDARD_IRDETO) &&
			 (intr_status1 & BIT(CIT_INTR_STAT_1_RETRY_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_RETRY_BIT);
			cit_sc_exit_critical_section(handle);
			handle->ch_status.status1 |= SC_RX_PARITY;
			SYS_LOG_DBG("Retry intr error");
			rv = -EBADMSG;
			goto done;
		} else if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E1) &&
			   (intr_status2 & BIT(CIT_INTR_STAT_2_RLEN_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_RLEN_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Rlen intr error");
			rv = -EFAULT;
			goto done;
		} else if (intr_status2 & BIT(CIT_INTR_STAT_2_WAIT_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_WAIT_BIT);
			handle->ch_status.status1 |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Wait intr error");
			rv = -ETIMEDOUT;
			goto done;
		} else if ((intr_status2 & BIT(CIT_INTR_STAT_2_CWT_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_CWT_BIT);
			handle->ch_status.status1  |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("CWT intr error");
			rv = -ETIMEDOUT;
			goto done;
		} else if (status2 & BIT(CIT_STATUS_2_ROVERFLOW_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->status2 &= ~BIT(CIT_STATUS_2_ROVERFLOW_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Overflow intr error");
			rv = -EFAULT;
			goto done;
		} else if ((handle->card_removed == true) &&
			   (intr_status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &=
					 ~BIT(CIT_STATUS_1_CARD_PRES_BIT);
			handle->ch_status.status1  |= SC_RESET_CHANNEL_REQUIRED;
			handle->card_removed = false;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Card remove error");
			rv = -EFAULT;
			goto done;
		} else if (!(intr_status2 & BIT(CIT_INTR_STAT_2_RCV_BIT))) {
			val = handle->ch_param.timeout.val;
			rv = cit_wait_for_event(handle,
					&handle->wait_event.rcv_wait, val);
			if (rv) {
				cit_sc_enter_critical_section(handle);
				handle->ch_status.status1 |= SC_RX_TIMEOUT;
				cit_sc_exit_critical_section(handle);
				rv = -ETIMEDOUT;
				goto done;
			}
		}

		cit_sc_enter_critical_section(handle);
		intr_status1 = handle->intr_status1;
		intr_status2 = handle->intr_status2;
		status2 = handle->status2;
		cit_sc_exit_critical_section(handle);

	} while ((intr_status2 & BIT(CIT_INTR_STAT_2_RCV_BIT)) !=
				 BIT(CIT_INTR_STAT_2_RCV_BIT));

	cit_sc_enter_critical_section(handle);
	handle->intr_status2  &= ~BIT(CIT_INTR_STAT_2_RCV_BIT);
	handle->status2 |= BIT(CIT_STATUS_2_REMPTY_BIT);
	cit_sc_exit_critical_section(handle);

done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_wait_rready(struct ch_handle *handle)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t intr_status1, intr_status2;
	u32_t status2;
	s32_t rv = 0;
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	intr_status1 = handle->intr_status1;
	intr_status2 = handle->intr_status2;
	status2 = handle->status2;
	cit_sc_exit_critical_section(handle);

	do {
		if ((intr_status1 & BIT(CIT_INTR_STAT_1_BGT_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_BGT_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("BGT intr error");
			rv = -EFAULT;
			goto done;
		}
#ifdef BSCD_EMV2000_CWT_PLUS_4
		else if ((intr_status2 & BIT(CIT_INTR_STAT_2_CWT_BIT)) &&
			 !(intr_status2 & BIT(CIT_INTR_STAT_2_RREADY_BIT)) &&
			 (ch_param->standard != SC_STANDARD_EMV2000)) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_CWT_BIT);
			handle->ch_status.status1  |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("CWT intr error");
			rv = -EFAULT;
			goto done;
		}
#elif defined(BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR)
		else if ((intr_status1 & BIT(CIT_INTR_STAT_1_EVENT1_BIT)) &&
			 !(intr_status2 & BIT(CIT_INTR_STAT_2_RREADY_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &=
					~BIT(CIT_INTR_STAT_1_EVENT1_BIT);
			handle->ch_status.status1 |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Event1 intr error");
			rv = -EFAULT;
			goto done;
		}
#else
		else if ((intr_status2 & BIT(CIT_INTR_STAT_2_CWT_BIT)) &&
			 !(intr_status2 & BIT(CIT_INTR_STAT_2_RREADY_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_CWT_BIT);
			handle->ch_status.status1 |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("CWT intr error");
			rv = -EFAULT;
			goto done;
		}
#endif
		else if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E1) &&
			 (intr_status2 & BIT(CIT_INTR_STAT_2_RLEN_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_RLEN_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Rlen intr error");
			rv = -EFAULT;
			goto done;
		} else if (intr_status2 & BIT(CIT_INTR_STAT_2_WAIT_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_WAIT_BIT);
			handle->ch_status.status1  |= SC_RX_TIMEOUT;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Wait intr error");
			rv = -ETIMEDOUT;
			goto done;
		} else if (status2 & BIT(CIT_STATUS_2_ROVERFLOW_BIT)) {
			cit_sc_enter_critical_section(handle);
			handle->status2 &= ~BIT(CIT_STATUS_2_ROVERFLOW_BIT);
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Roverflow intr error");
			rv = -EFAULT;
			goto done;
		} else if ((handle->card_removed == true) &&
			   (intr_status1 & BIT(CIT_INTR_STAT_1_PRES_BIT))) {
			cit_sc_enter_critical_section(handle);
			handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_PRES_BIT);
			handle->ch_status.status1  |= SC_RESET_CHANNEL_REQUIRED;
			handle->card_removed = false;
			cit_sc_exit_critical_section(handle);
			SYS_LOG_DBG("Card removed error");
			rv = -EFAULT;
			goto done;
		} else if (intr_status2 & BIT(CIT_INTR_STAT_2_RREADY_BIT)) {
			val = handle->ch_param.timeout.val;
			rv = cit_wait_for_event(handle,
				&handle->wait_event.rcv_wait, val);
			if (rv) {
				cit_sc_enter_critical_section(handle);
				handle->ch_status.status1 |= SC_RX_TIMEOUT;
				cit_sc_exit_critical_section(handle);
				SYS_LOG_DBG("Wait for timeout event error");
				rv = -ETIMEDOUT;
				goto done;
			}
		}

		cit_sc_enter_critical_section(handle);
		intr_status1 = handle->intr_status1;
		intr_status2 = handle->intr_status2;
		status2 = handle->status2;
		cit_sc_exit_critical_section(handle);

	} while (!(status2 & BIT(CIT_STATUS_2_RREADY_BIT)));

	cit_sc_enter_critical_section(handle);
	handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_RREADY_BIT);
	handle->status2 &= ~BIT(CIT_STATUS_2_RREADY_BIT);
	cit_sc_exit_critical_section(handle);
	SYS_LOG_DBG("rready intr received");
done:
#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
	/* Disable event1 */
	sys_clear_bit((handle->base + CIT_EVENT1_CMD_4),
						 CIT_EVENT1_CMD_4_EVENT_EN);
#endif

#ifdef BSCD_EMV2000_CWT_PLUS_4
	handle->receive = false;
#endif
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Activating channel
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_activating(struct ch_handle *handle)
{
	u32_t timer_cnt1, timer_cnt2;
	u32_t timer_cnt;
	u32_t prev_timer_cnt = 0;
  #if defined(CUST_X7_PLAT) && defined(SCI_DIAG_ALL)
	u32_t start_reset;
  #endif
	u32_t base = handle->base;
	s32_t rv = 0;
	u32_t cnt, val;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
								true, true};
	struct sc_time time = {SC_MAX_RESET_IN_CLK_CYCLES, SC_TIMER_UNIT_CLK};
	struct cit_timer wwt_timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE,
								true, true};
	struct sc_time wwt_time = {SC_MAX_ETU_PER_ATR_BYTE_EMV2000,
							SC_TIMER_UNIT_ETU};

	SMART_CARD_FUNCTION_ENTER();
	/*
	 * rst in low is too long; other code takes too long time or
	 * clock is not accurate?
	 */
	if (handle->ch_param.standard == SC_STANDARD_EMV2000)
		time.val = SC_MAX_RESET_IN_CLK_CYCLES - SC_SYSTEM_EXEC_CYCLES;

	/* On Dell X7 laptop, VCC is up late about 2ms for cold reset?*/
#ifdef CUST_X7_PLAT
	if (handle->reset_type == SC_COLD_RESET)
		time.val = SC_MAX_RESET_IN_CLK_CYCLES + 10500;
#endif

	/*
	 *  disable intr, if the card removal interrupt comes ISR
	 *  will set the vcc bit
	 */
	cit_sc_enter_critical_section(handle);

	/* Now check card presence */
	if (handle->ch_status.card_present == false) {
		cit_sc_exit_critical_section(handle);
		rv = -ECANCELED;
		goto done;
	}

	/*
	 * Set SC_VCC low = CMDVCC low which in turn starts the
	 * VCC signal rising in the Phillips chip
	 */
	val = sys_read32(base + CIT_IF_CMD_1);
	val &= ~BIT(CIT_IF_CMD_1_RST_BIT);

	/* Use Auto Deactivation instead of TDA8004 */
	val &= ~BIT(CIT_IF_CMD_1_VCC_BIT);
	if (handle->ch_param.auto_deactive_req == true)
		val |= BIT(CIT_IF_CMD_1_AUTO_VCC_BIT);

	sys_write32(val, (base + CIT_IF_CMD_1));
	cit_sc_exit_critical_section(handle);
	SYS_LOG_DBG("SC_RST low");
#if defined(CUST_X7_PLAT) && defined(SCI_DIAG_ALL)
	if (handle->reset_type == SC_COLD_RESET)
		start_reset = BSCD_GET_CPU_TICKS();
#endif
	/* wait for 42,000 clk cycles. */
	for (cnt = 0; cnt < handle->ch_param.ext_clkdiv; cnt++) {

		timer.interrupt_enable = true;
		timer.enable = true;

		rv = cit_config_timer(handle, &timer, &time);
		if (rv)
			goto done;

		rv = cit_wait_timer_event(handle);
		if (rv)
			goto done;

		/* Disable timer */
		timer.interrupt_enable = false;
		timer.enable = false;
		rv = cit_timer_isr_control(handle, &timer);
		if (rv)
			goto done;
	}
#if defined(CUST_X7_PLAT) && defined(SCI_DIAG_ALL)
	if (handle->reset_type == SC_COLD_RESET)
		cit_adjust_block_delay_ns(start_reset, 14150);
#endif
	/*
	 * Set this to 0 temporarily during ATR session.  For EMV,
	 * we will set it back in BSCD_Channel_P_EMVATRReceiveAndDecode.
	 * For the rest, the application should set it back
	 */
	sys_write32(0, (base + CIT_UART_CMD_2));

	/* Enable 2 interrupts with callback */
	if (cit_ch_interrupt_enable(handle, CIT_INTR_ATRS, cit_ch_atr_cb)) {
		rv =  -EINVAL;
		goto done;
	}
	if (cit_ch_interrupt_enable(handle, CIT_INTR_RCV, cit_ch_rcv_cb)) {
		rv =  -EINVAL;
		goto done;
	}
	/*
	 *  Enable WWT to ensure the max interval between 2 consecutive
	 *  ATR chars of 10080 ETU
	 */
	if (handle->ch_param.standard == SC_STANDARD_EMV2000)
		wwt_time.val = SC_MAX_ETU_PER_ATR_BYTE_EMV2000;
	else /* EMV 96 or the rest */
		wwt_time.val = SC_MAX_ETU_PER_ATR_BYTE;

	wwt_timer.timer_mode = CIT_WAIT_TIMER_WWT;
	rv = cit_config_timer(handle, &wwt_timer, &wwt_time);
	if (rv)
		goto done;

	/* Set to get ATR packet. */
	val = sys_read32(base + CIT_UART_CMD_1);
	val |= BIT(CIT_UART_CMD_1_GET_ATR_BIT) | BIT(CIT_UART_CMD_1_IO_EN_BIT);
	sys_write32(val, (base + CIT_UART_CMD_1));

	/* Set RST */
	/* Use Auto Deactivation instead of TDA8004 */
	val = sys_read32(base + CIT_IF_CMD_1);
	val |= BIT(CIT_IF_CMD_1_RST_BIT);
	if (handle->ch_param.auto_deactive_req == true)
		val |= BIT(CIT_IF_CMD_1_AUTO_RST_BIT);
	 sys_write32(val, (base + CIT_IF_CMD_1));

	/* wait for 40,000 clk cycles for EMV96 and 42000 for EMV2000 */
	for (cnt = 0; cnt < handle->ch_param.ext_clkdiv; cnt++)  {
		/* Set Timer */
		timer.interrupt_enable = true;
		timer.enable = true;
		timer.timer_type = CIT_GP_TIMER;
		timer.timer_mode = CIT_GP_TIMER_IMMEDIATE;
		if (handle->ch_param.standard == SC_STANDARD_EMV2000)
			time.val = SC_EMV2000_MAX_ATR_START_IN_CLK_CYCLES +
					 SC_ATR_START_BIT_DELAY_IN_CLK_CYCLES;
		else
			time.val = SC_MAX_ATR_START_IN_CLK_CYCLES +
					 SC_ATR_START_BIT_DELAY_IN_CLK_CYCLES;
		time.unit  = SC_TIMER_UNIT_CLK;
		SYS_LOG_DBG("Activating GP timer");
		rv = cit_config_timer(handle, &timer, &time);
		if (rv)
			goto done;

		rv = cit_wait_atr_start(handle);
		if (rv) {
			/* Disable timer */
			timer.interrupt_enable = false;
			timer.enable = false;
			cit_timer_isr_control(handle, &timer);

			if (rv == -ETIMEDOUT) {
				val = handle->ch_param.ext_clkdiv - 1;
				if (cnt == val) {
					/* if this is the last loop and we still
					 * timeout, major error Need to return
					 * deactivate for EMV2000
					 * test 1719 xy=30
					 */
					rv = -ECANCELED;
					goto done;
				} else {
					/* If this is not the last loop*/
					prev_timer_cnt +=
					      SC_MAX_ATR_START_IN_CLK_CYCLES;
					continue;
				}
			} else {
				/* If the error is not scTimeOut, major error */
				rv = -EFAULT;
				goto done;
			}
		}

		/* Disable timer */
		timer.interrupt_enable = false;
		timer.enable = false;
		rv = cit_timer_isr_control(handle, &timer);
		if (rv)
			goto done;

		/* Read timer counter, the ATR shall be received after
		 * 400 clock cycles
		 */
		timer_cnt2 = sys_read32(base + CIT_TIMER_CNT_2);
		timer_cnt1 = sys_read32(base + CIT_TIMER_CNT_1);
		timer_cnt = ((timer_cnt2 << 8) | timer_cnt1) + prev_timer_cnt;

		if ((timer_cnt < (SC_MIN_ATR_START_IN_CLK_CYCLES *
					 handle->ch_param.ext_clkdiv)) ||
		    (timer_cnt > time.val)) {
			/* Need to ret deactivate for EMV2000 test 1719 xy=30 */
			rv = -ECANCELED;
			goto done;
		}
#if defined(SCI_DIAG_ALL)
		/* below is general timer, not WWT timer. This does not conform
		 * to spec since there's no such timing requirement in spec
		 * This caused an issue with a Gemalto card where it takes
		 * around 890ms for one byte of ATR so we will time out
		 * here (JIRA-ESPSW 1950)
		 */
		/* Enable WWT to ensure all ATR data are received within time */
		if (handle->ch_param.standard == SC_STANDARD_EMV2000)
			time.val = SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES_EMV2000;
		else /* EMV 96 or the rest */
			time.val = SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES;
		time.unit = SC_TIMER_UNIT_ETU;
		timer.interrupt_enable = true;
		timer.enable = true;
		rv = cit_config_timer(handle, &timer, &time);
#else
		rv = 0;
#endif
		if (rv == 0) {
			handle->ch_status.card_activate = true;
			break;
		}
	}
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Read a single byte from controller buffer
 *
 * @param handle Channel handle
 * @param data Buffer to copy data
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_byte_read(struct ch_handle *handle, u8_t *data)
{
	u32_t status2;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	handle->status2 = status2 =  sys_read32(handle->base + CIT_STATUS_2);
	cit_sc_exit_critical_section(handle);

	if (!(status2 & BIT(CIT_STATUS_2_REMPTY_BIT))) {
		*data = (unsigned char)sys_read32(handle->base + CIT_RECEIVE);
		cit_sc_enter_critical_section(handle);
		handle->status2 = status2 =
				 sys_read32(handle->base + CIT_STATUS_2);
		cit_sc_exit_critical_section(handle);

		if ((status2 & BIT(CIT_STATUS_2_RPAR_ERR)) &&
		     (handle->ch_param.standard != SC_STANDARD_IRDETO)) {
			SYS_LOG_ERR("Receive a parity error byte\n");
			cit_sc_enter_critical_section(handle);
			handle->status1 |= SC_RX_PARITY;
			cit_sc_exit_critical_section(handle);
			rv = -EBADMSG;
		}
	} else {
		rv = -ENODATA;
	}

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Read data from controller buffer for T0
 *
 * @param handle Channel handle
 * @param rx Receive buffer
 * @param rx_len Receive read length
 * @param max_len Max length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_t0_read_data(struct ch_handle *handle, u8_t *rx,
			       u32_t *rx_len, u32_t rx_max_len)
{
	s32_t rv = 0;
	u32_t len = 0;
	u32_t status2;
#ifndef BSCD_DSS_ICAM
	struct cit_timer timer = {CIT_WAIT_TIMER, CIT_WAIT_TIMER_WWT,
						false, false};
	struct sc_time time = {SC_DEFAULT_WORK_WAITING_TIME, SC_TIMER_UNIT_ETU};
	u32_t enabled_here = 0;
#endif

	SMART_CARD_FUNCTION_ENTER();
	*rx_len = 0;
	cit_sc_enter_critical_section(handle);
	handle->status2 = status2 = sys_read32(handle->base + CIT_STATUS_2);
	cit_sc_exit_critical_section(handle);

	while (len < rx_max_len) {
#ifndef BSCD_DSS_ICAM
		/*
		 * This is a backup time out for non EMV standard. Just in case,
		 * we do not read all the byte in one shot but WWT was disable
		 * in BSCD_Channel_Receive
		 */
		if ((handle->ch_param.standard != SC_STANDARD_EMV1996) &&
		    (handle->ch_param.standard != SC_STANDARD_EMV2000)) {
			/* Cannot enable GT if WWT is disabled, since WWT can
			 * be disabled when reach here, but GT is too small
			 */
			if (!cit_timer_enable_status(handle, CIT_WAIT_TIMER)) {
				time.val = handle->ch_param.work_wait_time.val;
				rv = cit_config_timer(handle, &timer, &time);
				if (rv)
					goto done;
				enabled_here = 1;
			}
		}
#endif
		cit_sc_enter_critical_section(handle);
		status2 = handle->status2;
		cit_sc_exit_critical_section(handle);

		if (status2 & BIT(CIT_STATUS_2_REMPTY_BIT)) {
			rv = cit_wait_rcv(handle);
			if (rv) {
				SYS_LOG_DBG("Wait for receive rv %d", rv);
#ifndef BSCD_DSS_ICAM
				/* Disable timer */
				if ((handle->ch_param.standard !=
							SC_STANDARD_EMV1996) &&
				    (handle->ch_param.standard !=
							 SC_STANDARD_EMV2000)) {
					if (enabled_here) {
						timer.interrupt_enable = false;
						timer.enable = false;
						cit_timer_isr_control(handle,
								&timer);
					}
				}
#endif
				if ((rv == -EBADMSG) ||
				    (rv == -ETIMEDOUT))
					break;

				return -EINVAL;
			}
		}

#ifndef BSCD_DSS_ICAM
		/* Disable timer */
		if ((handle->ch_param.standard != SC_STANDARD_EMV1996) &&
			(handle->ch_param.standard != SC_STANDARD_EMV2000)) {
			if (enabled_here) {
				timer.interrupt_enable = false;
				timer.enable = false;
				rv = cit_timer_isr_control(handle, &timer);
				if (rv)
					goto done;
			}
		}
#endif
		while (len < rx_max_len) {
			rv = cit_channel_byte_read(handle, &rx[len]);
			if (rv == 0) {
				handle->ch_status.status1 &= ~SC_RX_PARITY;
				if ((rx[len] == 0x60) &&
				    (handle->ch_param.null_filter == true)) {
					SYS_LOG_DBG("Ignore 0x60");
					continue;
				} else {
					SYS_LOG_DBG("%2X ", rx[len]);
					len++;
				}
			} else if (rv == -EBADMSG) {
				SYS_LOG_ERR("parity or EDC error");
				continue;
			} else {
				break;
			}
		}
	}

#ifndef BSCD_DSS_ICAM
done:
#endif
	*rx_len = len;
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Read data from controller buffer for T1
 *
 * @param handle Channel handle
 * @param rx Receive buffer
 * @param rx_len Receive read length
 * @param max_len Max length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_t1_read_data(struct ch_handle *handle, u8_t *rx,
			       u32_t *rx_len, u32_t rx_max_len)
{
	u32_t base = handle->base;
	u32_t val, len1, len2;
	u32_t len = 0, cnt;
	struct cit_timer timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE,
							false, false};
	s32_t rv = 0;

	ARG_UNUSED(rx_max_len);
	SMART_CARD_FUNCTION_ENTER();
	*rx_len = 0;
	val =   sys_read32(base + CIT_PROTO_CMD);
	val |= BIT(CIT_PROTO_CMD_TBUF_RST_BIT);

	if ((handle->ch_param.standard == SC_STANDARD_ES) ||
	    (handle->ch_param.btpdu == true)) {
		/* This condition added for WHQL card 5 test */
		val &= ~BIT(CIT_PROTO_CMD_EDC_EN_BIT);
	} else {
		val |= BIT(CIT_PROTO_CMD_EDC_EN_BIT);
#if defined(SCI_DIAG_ALL)
		if (handle->ch_param.edc.crc)
			val |= BIT(CIT_PROTO_CMD_CRC_BIT);
		else
			val &= ~BIT(CIT_PROTO_CMD_CRC_BIT);
#endif
	}

	sys_write32(val, base + CIT_PROTO_CMD);
	rv = cit_wait_rready(handle);
	if (rv) {
		rv = -ENOMSG;
		goto done;
	}

	/* Disable block wait timer */
	timer.timer_type = CIT_WAIT_TIMER;
	timer.timer_mode = CIT_WAIT_TIMER_BWT;
	rv = cit_timer_isr_control(handle, &timer);
	if (rv)
		goto done;

	/* Disable cwt since we already receive all the bytes */
	sys_clear_bit((base + CIT_TIMER_CMD), CIT_TIMER_CMD_CWT_EN_BIT);
	/* Clear cwt_intr so that it won't show up next time */
	cit_sc_enter_critical_section(handle);
#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
	handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_EVENT1_BIT);
#else
	handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_CWT_BIT);
	sys_read32(base + CIT_INTR_STAT_2);
#endif
	cit_sc_exit_critical_section(handle);

	len1 = sys_read32(base + CIT_RLEN_1);
	len2 = sys_read32(base + CIT_RLEN_2);

	/* RLEN_9_BIT_MASK = 0x01ff */
	len = ((len2 << 8) | len1) & SC_RLEN_9_BIT_MASK;
	SYS_LOG_DBG("SmartCardBlockRead: len = %d\n", len);
	if (len) {
		for (cnt = 0; cnt < len; cnt++) {
			rx[cnt] = sys_read32(base + CIT_RECEIVE);
			val = sys_read32(base + CIT_STATUS_2);
			if (val & BIT(CIT_STATUS_2_RPAR_ERR)) {
				SYS_LOG_ERR("SC Block read: parity error\n");
				rv = -EBADMSG;
			} else if (val & BIT(CIT_STATUS_2_EDC_ERR_BIT)) {
				SYS_LOG_ERR("SC Block read: EDC error\n");
				rv = -EBADMSG;
			}

			SYS_LOG_DBG("%02x ", rx[cnt]);
		}
	}
done:
	if (rv)
		len = 0;

	*rx_len = len;
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}
/**
 * @brief Receive the data and decode ATR
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_receive_and_decode(struct ch_handle *handle)
{
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	SYS_LOG_DBG("Receive and decode");
	if (handle->ch_param.read_atr_on_reset) {
		SYS_LOG_DBG("standard %d", handle->ch_param.standard);
		switch (handle->ch_param.standard) {
		case SC_STANDARD_EMV1996:
		case SC_STANDARD_EMV2000:
			rv = cit_emv_atr_receive_decode(handle);
			break;

		case SC_STANDARD_ISO:
			rv = cit_iso_atr_receive_decode(handle);
			break;
		default:
			rv = -EFAULT;
			break;
		}
	}
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_channel_sync_read_data(struct ch_handle *handle, u8_t *rx,
				 u32_t *rx_len, u32_t rx_max_len)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(rx);
	ARG_UNUSED(rx_len);
	ARG_UNUSED(rx_max_len);
	return 0;
}

/**
 * @brief ISR handler for smart card
 *
 * @param handle Channel handle
 *
 * @return None
 */
void cit_channel_handler_isr(struct ch_handle *handle)
{
	u32_t status1, status2, proto_cmd;
	u32_t intr_en1, intr_en2, intr_status1, intr_status2;
	u32_t prev_status1;
	u32_t base = handle->base;
	s32_t rv = 0;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
								true, true};
	u32_t cnt;

#ifdef BSCD_EMV2000_CWT_PLUS_4
	struct cit_timer cwt_timer = {CIT_WAIT_TIMER, CIT_WAIT_TIMER_WWT,
							true, true};
	struct sc_time cwt_time = {16, SC_TIMER_UNIT_ETU};
#endif
	SMART_CARD_FUNCTION_ENTER();
	reinitialize_function_table();
	check_and_wake_system();
	if (handle->open == false)
		return;

	/* Read Smartcard Interrupt Status & Mask Register */
	proto_cmd = sys_read32(base + CIT_PROTO_CMD);
	status1 = sys_read32(base + CIT_STATUS_1);
	status2 = sys_read32(base + CIT_STATUS_2);
	intr_en1 = sys_read32(base + CIT_INTR_EN_1);
	intr_status1 = sys_read32(base + CIT_INTR_STAT_1);
	intr_en2 = sys_read32(base + CIT_INTR_EN_2);
	intr_status2 = sys_read32(base + CIT_INTR_STAT_2);


	/* Process interrupt */
	if ((intr_en1 & BIT(CIT_INTR_EN_1_PRES_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_PRES_BIT))) {
		/* Disable pres intr to debounce the card pres */
		rv = cit_ch_interrupt_disable(handle, CIT_INTR_PRES_INSERT);
		if (rv)
			goto done;

		/* Store status_1 to determine if hardware failure */
		prev_status1 = sys_read32(base + CIT_STATUS_1);

		/*
		 * In case this is an emergency deactivation, we have to set
		 * IF_CMD_1[VCC]=1 to detect card pres again.
		 */
		sys_set_bit((base + CIT_IF_CMD_1), CIT_IF_CMD_1_VCC_BIT);

		/*
		 * TDA8004 suggests we to wait until debounce stabilizes.
		 * NDS suggests to sleep for 10 milli seconds.  This may hold
		 * the system for 10ms but it is okay since the system should
		 * not continue without the card.
		 */
		/* All customers should use TDA8024 now */
		status1 = sys_read32(base + CIT_STATUS_1);

		/*
		 * According TDA 8004 Application note, this is how to determine
		 * card presence, card removal and hardware failure.
		 */
		if ((status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT)) &&
		    (!(prev_status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT)))) {
			handle->ch_status.card_present = true;
			handle->ch_status.status1 |= SC_HARDWARE_FAILURE |
						 SC_RESET_CHANNEL_REQUIRED;
		} else if ((status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT)) &&
			   (prev_status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT))) {
			handle->ch_status.card_present  = true;
			handle->ch_status.card_activate = false;
			handle->ch_status.pps_done = false;
		} else if (!(status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT)) &&
			 !(prev_status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT))) {
			/*
			 * Disable all interrupt but pres_intr to support
			 * auto-deactivation. Auto Deactvation will cause a
			 * parity_intr and retry_intr to loop forever
			 */
			sys_write32(0, (base + CIT_INTR_EN_1));
			sys_write32(0, (base + CIT_INTR_EN_2));

			/* 09/20/05,Allen.C, remember Card was removed */
			handle->card_removed = true;
			handle->ch_status.card_present = false;
			handle->ch_status.card_activate = false;
			handle->ch_status.pps_done = false;
			handle->ch_status.status1 |= SC_RESET_CHANNEL_REQUIRED;
		}

		handle->intr_status1 = intr_status1;
		handle->status1  = status1;

		if (handle->ch_status.card_present == true) {
#ifdef BSCD_EMV_TEST
			for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)  {
				if (handle->cb[CIT_INTR_PRES_INSERT][cnt])
					(*(handle->cb
					[CIT_INTR_PRES_INSERT][cnt]))(handle);
			}
#else
#ifdef PHX_REQ_CT
			{
				unsigned int xTaskWoken = FALSE;

				xTaskWoken = queue_cmd(SCHED_CCID_CT_INS,
						QUEUE_FROM_ISR, xTaskWoken);
				SCHEDULE_FROM_ISR(xTaskWoken);
			}
#endif
#endif
		} else {
#ifdef BSCD_EMV_TEST
			for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)  {
				if (handle->cb[CIT_INTR_PRES_REMOVE][cnt])
					(*(handle->cb
					[CIT_INTR_PRES_REMOVE][cnt]))(handle);
			}
#else
#ifdef PHX_REQ_CT
			{
				unsigned int xTaskWoken = FALSE;

				xTaskWoken = queue_cmd(SCHED_CCID_CT_REM,
						QUEUE_FROM_ISR, xTaskWoken);
				SCHEDULE_FROM_ISR(xTaskWoken);
			}
#endif
#endif

		}

		/* re-enable pres intr */
		rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_INSERT,
							 cit_ch_cardinsert_cb);
		if (rv)
			goto done;
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_TPAR_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_TPAR_BIT))) {
		handle->ch_status.status1 |= SC_TX_PARITY;
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

#ifdef BSCD_EMV2000_FIME
		handle->parity_error_cnt++;
#endif
		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_TPARITY][cnt] != NULL)
				(*(handle->cb[CIT_INTR_TPARITY][cnt]))(handle);
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_TIMER_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_TIMER_BIT))) {
		timer.interrupt_enable = false;
		timer.enable = false;
		rv = cit_timer_isr_control(handle, &timer);
		if (rv)
			goto done;
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++) {
			if (handle->cb[CIT_INTR_TIMER][cnt] != NULL)
				(*(handle->cb[CIT_INTR_TIMER][cnt]))(handle);
		}
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_BGT_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_BGT_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_BGT][cnt] != NULL)
				(*(handle->cb[CIT_INTR_BGT][cnt]))(handle);
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_TDONE_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_TDONE_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_TDONE][cnt] != NULL)
				(*(handle->cb[CIT_INTR_TDONE][cnt]))(handle);
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_RETRY_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_RETRY_BIT))) {
		/*
		 * If parity tx or rx retrial failes, we should reset uart and
		 * NOT to continue tx any more data
		 */
		sys_set_bit((base + CIT_UART_CMD_1),
					CIT_UART_CMD_1_UART_RST_BIT);
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_RETRY][cnt] != NULL)
				(*(handle->cb[CIT_INTR_RETRY][cnt]))(handle);
	}

	if ((intr_en1 & BIT(CIT_INTR_EN_1_TEMPTY_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_TEMPTY_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_TEMPTY][cnt] != NULL)
				(*(handle->cb[CIT_INTR_TEMPTY][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_RPAR_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_RPAR_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;
		handle->ch_status.status1 |= SC_RX_PARITY;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_RPARITY][cnt] != NULL)
				(*(handle->cb[CIT_INTR_RPARITY][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_ATRS_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_ATRS_BIT))) {
		timer.interrupt_enable = false;
		timer.enable = false;
		rv = cit_timer_isr_control(handle, &timer);
		if (rv)
			goto done;

		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_ATRS][cnt] != NULL)
				(*(handle->cb[CIT_INTR_ATRS][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_CWT_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_CWT_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_CWT][cnt] != NULL)
				(*(handle->cb[CIT_INTR_CWT][cnt])) (handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_RLEN_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_RLEN_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_RLEN][cnt] != NULL)
				(*(handle->cb[CIT_INTR_RLEN][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_WAIT_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_WAIT_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_WAIT][cnt] != NULL)
				(*(handle->cb[CIT_INTR_WAIT][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_RCV_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_RCV_BIT))) {
/* Enable RCV_INTR only in T=1, EMV 2000 to resolve CWT+4 issue */
#ifdef BSCD_EMV2000_CWT_PLUS_4
		if ((handle->ch_param.standard == SC_STANDARD_EMV2000) &&
		    (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) &&
		    (handle->receive == true)) {

			/* Disable BWT timer */
			cwt_timer.interrupt_enable = false;
			cwt_timer.enable = false;
			rv = cit_timer_isr_control(handle, &cwt_timer);
			if (rv)
				goto done;
			/* Enable WWT in lieu of CWT */
			cwt_timer.interrupt_enable = true;
			cwt_timer.enable = true;
			if (handle->ch_param.cwi != 0)
				cwt_time.val = (2 << (handle->ch_param.cwi - 1))
				     + 15 + SC_CHARACTER_WAIT_TIME_GRACE_PERIOD;
			rv = cit_config_timer(handle, &cwt_timer, &cwt_time);

			BDBG_MSG(("RCV_INTR  cwt = %d\n", cwt_time.val));

			handle->status2 |= status2;
			intr_status2 &= ~BIT(CIT_INTR_STAT_2_RCV_BIT);
		}
#endif
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_RCV][cnt] != NULL)
				(*(handle->cb[CIT_INTR_RCV][cnt]))(handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_RREADY_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_RREADY_BIT))) {
#ifdef BSCD_EMV2000_CWT_PLUS_4
		if ((handle->ch_param.standard == SC_STANDARD_EMV2000) &&
		    (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1)) {
			/* Disable WWT timer, which is used as CWT + 4  */
			cwt_timer.interrupt_enable = false;
			cwt_timer.enable = false;
			rv = cit_timer_isr_control(handle, &cwt_timer);
			if (rv)
				goto done;

			BDBG_MSG(("RREADY_INTR  cwt disable\n"));
		}
#endif
		/*
		 *  fix for RReady hang, where it waits for Status2 but
		 *  status2 does not contain RcvRdy.
		 */
		if ((status2 & BIT(CIT_STATUS_2_RREADY_BIT)) == 0)
			status2 = sys_read32(base + CIT_STATUS_2);

		handle->status2 |= status2;
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_RREADY][cnt] != NULL)
				(*(handle->cb[CIT_INTR_RREADY][cnt]))(handle);
	}

	if ((proto_cmd & BIT(CIT_PROTO_CMD_EDC_EN_BIT)) &&
	    (status2 & BIT(CIT_STATUS_2_EDC_ERR_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_EDC][cnt] != NULL)
				(*(handle->cb[CIT_INTR_EDC][cnt])) (handle);

	}

#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
	if ((intr_en1 & BIT(CIT_INTR_EN_1_EVENT1_BIT)) &&
	    (intr_status1 & BIT(CIT_INTR_STAT_1_EVENT1_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_EVENT1][cnt] != NULL)
				(*(handle->cb[CIT_INTR_EVENT1][cnt])) (handle);
	}

	if ((intr_en2 & BIT(CIT_INTR_EN_2_EVENT2_BIT)) &&
	    (intr_status2 & BIT(CIT_INTR_STAT_2_EVENT2_BIT))) {
		handle->intr_status1 |= intr_status1;
		handle->intr_status2 |= intr_status2;

		for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)
			if (handle->cb[CIT_INTR_EVENT2][cnt] != NULL)
				(*(handle->cb[CIT_INTR_EVENT2][cnt])) (handle);
	}
#endif
done:
	if (rv)
		SYS_LOG_DBG("ISR rv %d", rv);
	SMART_CARD_FUNCTION_EXIT();
}

s32_t cit_channel_sync_transmit(struct ch_handle *handle, u8_t *tx,
				  u32_t tx_len)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(tx);
	ARG_UNUSED(tx_len);
	return 0;
}

/**
 * @brief Transmit data for T = 0 and T = 1
 *
 * @param handle Channel handle
 * @param tx Transmit buffer
 * @param tx_len Transmit length
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_t0t1_transmit(struct ch_handle *handle, u8_t *tx,
				u32_t tx_len)
{
	u32_t base = handle->base;
	u32_t val, cnt;
	s32_t rv = 0;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
						true, true};
	struct sc_time time = {SC_MIN_DELAY_BEFORE_TZERO_SEND,
						SC_TIMER_UNIT_ETU};

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	handle->intr_status1 &= BIT(CIT_INTR_STAT_1_PRES_BIT);
	handle->intr_status2 &= BIT(CIT_INTR_STAT_2_ATRS_BIT);
	cit_sc_exit_critical_section(handle);

	/* Reset the Transmit and Receive buffer */
	sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_TBUF_RST_BIT);
	sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_RBUF_RST_BIT);

	/*
	 * Enable cwt here for only T=1. We will disable cwt in
	 * SmartCardTOneReceive() after we receive RREADY_INTR
	 */
	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
		/* Clear the possible previous cwt_intr */
		handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_CWT_BIT);
		sys_read32(base + CIT_INTR_STAT_2);

		sys_set_bit((base + CIT_TIMER_CMD), CIT_TIMER_CMD_CWT_EN_BIT);

#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR

		/* Clear the possible previous event1 intr */
		handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_EVENT1_BIT);

		if (handle->ch_param.standard == SC_STANDARD_EMV2000) {
			sys_write32(5, base + CIT_EVENT1_CMP);
			/* start event src */
			sys_write32(CIT_CWT_INTR_EVENT_SRC,
						 base + CIT_EVENT1_CMD_3);
			/* increment event src */
			sys_write32(CIT_RX_ETU_TICK_EVENT_SRC,
						base + CIT_EVENT1_CMD_2);
			/* reset event src */
			sys_write32(CIT_RX_START_BIT_EVENT_SRC,
						base + CIT_EVENT1_CMD_1);

			val = BIT(CIT_EVENT1_CMD_4_EVENT_EN) |
			      BIT(CIT_EVENT1_CMD_4_INR_AFTER_COMPARE) |
			      BIT(CIT_EVENT1_CMD_4_RUN_AFTER_RESET);
			sys_write32(val, base + CIT_EVENT1_CMD_4);
		}
#endif
	}

	/*
	 * For EMV T=0 only, the minimum interval btw the leading
	 * edges of the start bits of 2 consecutive characters sent
	 * in opposite directions shall be 16.  For EMV and ISO T=1,
	 * the minimum interval btw the leading edges of the start bits of 2
	 * consecutive characters sent in opposite directions shall be 22.
	 */

	rv = cit_config_timer(handle, &timer, &time);
	if (rv)
		goto done;

	rv = cit_wait_timer_event(handle);
	if (rv)
		goto done;

	/* Disable timer */
	timer.interrupt_enable = false;
	timer.enable = false;
	rv = cit_timer_isr_control(handle, &timer);
	if (rv)
		goto done;
	/*
	 * For T=1, we have to check the Block wait time
	 * For T=0, we have to check the Work Wait Time.
	 * BSYT Issue: RC0 WWT timer could only check the interval
	 * btw the leading edge of 2 consecutive characters sent
	 * by the ICC.  We will use GP timer to check the interval
	 * btw the leading edge of characters in opposite directions
	 */
	timer.interrupt_enable = true;
	timer.enable = true;
	timer.timer_type = CIT_WAIT_TIMER;
	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) {
		/* Restore the original WWT */
		timer.timer_mode = CIT_WAIT_TIMER_WWT;
		time.val = handle->ch_param.work_wait_time.val;
	} else if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
		timer.timer_mode = CIT_WAIT_TIMER_BWT;
		time.val = handle->ch_param.work_wait_time.val;
		if (handle->ch_param.blk_wait_time_ext.val == 0)
			time.val = handle->ch_param.blk_wait_time.val;
		else
			time.val = handle->ch_param.blk_wait_time_ext.val;
	}
	rv = cit_config_timer(handle, &timer, &time);
	if (rv)
		goto done;

	/* Fill BCM FIFO with the request message. */
	for (cnt = 0; cnt < tx_len; cnt++) {
		sys_write32(tx[cnt], (base + CIT_TRANSMIT));
		SYS_LOG_DBG("Tx %x\n", tx[cnt]);
	}

	/* Enable EDC */
	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) {
		sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_RBUF_RST_BIT);
	} else if ((handle->ch_param.standard == SC_STANDARD_ES) ||
	    (handle->ch_param.btpdu == true)) {
		/* This condition added for WHQL card 5 test */
		/* application computes its own LRC or CRC */
		sys_clear_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_EDC_EN_BIT);
	} else {
		sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_EDC_EN_BIT);
#if defined(SCI_DIAG_ALL)
		if (handle->ch_param.edc.crc == true)
			sys_set_bit((base + CIT_PROTO_CMD),
						CIT_PROTO_CMD_CRC_BIT);
		else
			sys_clear_bit((base + CIT_PROTO_CMD),
						CIT_PROTO_CMD_CRC_BIT);
#else
		sys_clear_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_CRC_BIT);
#endif
	}

	/* Set flow cmd */
	if ((handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) &&
			(handle->ch_param.standard == SC_STANDARD_NDS))
		sys_set_bit((base + CIT_FLOW_CMD), CIT_FLOW_CMD_FLOW_EN_BIT);
	else
		/* No flow control for T=1 protocol or T=14. */
		sys_clear_bit((base + CIT_FLOW_CMD), CIT_FLOW_CMD_FLOW_EN_BIT);

	/* Ready to transmit */
	val = sys_read32(base + CIT_UART_CMD_1);
	/* Always set auto receive */
	val = BIT(CIT_UART_CMD_1_TR_BIT) | BIT(CIT_UART_CMD_1_XMIT_GO_BIT) |
	      BIT(CIT_UART_CMD_1_IO_EN_BIT) | BIT(CIT_UART_CMD_1_AUTO_RCV_BIT);
	sys_write32(val, (base + CIT_UART_CMD_1));
#ifdef BSCD_EMV2000_CWT_PLUS_4
	handle->receive = true;
#endif
	/* Wait until the BCM sent all the data. */
	rv = cit_wait_tdone(handle);
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Transmit data for T = 14
 *
 * @param handle Channel handle
 * @param tx Transmit buffer
 * @param tx_len Transmit length
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_t14_irdeto_transmit(struct ch_handle *handle, u8_t *tx,
				      u32_t tx_len)
{
	u32_t base = handle->base;
	s32_t rv = 0;
	u32_t cnt, val;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
							true, true};
	struct sc_time time = {SC_T14_IRDETO_MIN_DELAY_RX2TX,
							 SC_TIMER_UNIT_CLK};

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	handle->intr_status1 &=
		~BIT(CIT_INTR_EN_1_TPAR_BIT) &
		~BIT(CIT_INTR_EN_1_TIMER_BIT) &
		~BIT(CIT_INTR_EN_1_BGT_BIT) &
		~BIT(CIT_INTR_EN_1_TDONE_BIT) &
		~BIT(CIT_INTR_EN_1_RETRY_BIT) &
		~BIT(CIT_INTR_EN_1_TEMPTY_BIT);

	handle->intr_status2 &=
		~BIT(CIT_INTR_EN_2_RPAR_BIT) &
		~BIT(CIT_INTR_EN_2_CWT_BIT) &
		~BIT(CIT_INTR_EN_2_RLEN_BIT) &
		~BIT(CIT_INTR_EN_2_WAIT_BIT) &
		~BIT(CIT_INTR_EN_2_RCV_BIT) &
		~BIT(CIT_INTR_EN_2_RREADY_BIT);
	cit_sc_exit_critical_section(handle);

	/* Reset the Transmit and Receive buffer */
	sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_TBUF_RST_BIT);
	sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_RBUF_RST_BIT);

	/* Need to wait for minimum of 1250 from last RX to this TX */
	rv = cit_config_timer(handle, &timer, &time);
	if (rv)
		goto done;

	rv = cit_wait_timer_event(handle);
	if (rv)
		goto done;

	/* Disable timer */
	timer.interrupt_enable = false;
	timer.enable = false;
	rv = cit_timer_isr_control(handle, &timer);
	if (rv)
		goto done;

	/* Enable EDC */
	sys_set_bit((base + CIT_PROTO_CMD), CIT_PROTO_CMD_RBUF_RST_BIT);

	/* No flow control for T=1 protocol or T=14. */
	sys_clear_bit((base + CIT_FLOW_CMD), CIT_FLOW_CMD_FLOW_EN_BIT);

	/* Fill BCM FIFO with the request message. */
	for (cnt = 0; cnt < tx_len; cnt++) {
		sys_write32(tx[cnt], (base + CIT_TRANSMIT));
		SYS_LOG_DBG("%02x", tx[cnt]);
		/* Ready to transmit */
		val = sys_read32(base + CIT_UART_CMD_1);
		val = BIT(CIT_UART_CMD_1_TR_BIT) |
			 BIT(CIT_UART_CMD_1_XMIT_GO_BIT) |
			 BIT(CIT_UART_CMD_1_IO_EN_BIT);

		if (sc_popcount[tx[cnt]] % 2 == 0) {
			SYS_LOG_DBG("Even number => Parity = 0\n");
			val |= BIT(CIT_UART_CMD_1_INV_PAR_BIT);
		} else {
			SYS_LOG_DBG("Odd number => Parity = 1\n");
			val &= ~BIT(CIT_UART_CMD_1_INV_PAR_BIT);
		}

		if (cnt == (tx_len - 1))
			  /* Last TX byte, ready to receive */
			val |= BIT(CIT_UART_CMD_1_AUTO_RCV_BIT);

		sys_write32(val, (base + CIT_UART_CMD_1));
		/* Wait until the BCM sent all the data. */
		rv = cit_wait_tdone(handle);
	}
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Enable interrupts
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_enable_intrs(struct ch_handle *handle)
{
	struct sc_channel_param *channel;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	channel = &handle->ch_param;
	if ((channel->protocol == SC_ASYNC_PROTOCOL_E0) &&
		(channel->standard != SC_STANDARD_IRDETO)) {
		if (cit_ch_interrupt_enable(handle, CIT_INTR_RETRY,
							 cit_ch_retry_cb))
			goto error;

		if (cit_ch_interrupt_enable(handle, CIT_INTR_RCV,
							 cit_ch_rcv_cb))
			goto error;

		if (cit_ch_interrupt_enable(handle, CIT_INTR_RPARITY,
							cit_ch_rparity_cb))
			goto error;

		if (cit_ch_interrupt_enable(handle, CIT_INTR_TPARITY,
							cit_ch_tparity_cb))
			goto error;
	} else if (channel->protocol == SC_ASYNC_PROTOCOL_E1) {
#ifdef BSCD_EMV2000_CWT_PLUS_4
		if ((channel->standard != SC_STANDARD_EMV2000) ||
			(channel->protocol != SC_ASYNC_PROTOCOL_E1)) {
#endif
			if (cit_ch_interrupt_enable(handle, CIT_INTR_CWT,
								cit_ch_cwt_cb))
				goto error;
#ifdef BSCD_EMV2000_CWT_PLUS_4
		}
#endif
#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
		if (cit_ch_interrupt_enable(handle, CIT_INTR_EVENT1,
							cit_ch_event1_cb))
			goto error;
#endif
		if (cit_ch_interrupt_enable(handle, CIT_INTR_BGT,
								 cit_ch_bgt_cb))
			goto error;

		if (cit_ch_interrupt_enable(handle, CIT_INTR_RLEN,
							 cit_ch_rlen_cb))
			goto error;

		if (cit_ch_interrupt_enable(handle, CIT_INTR_RREADY,
							cit_ch_rready_cb))
			goto error;

		sys_write32(handle->ch_param.blk_guard_time.val,
						(handle->base + CIT_BGT));
#ifdef BSCD_EMV2000_CWT_PLUS_4
		if (channel->standard == SC_STANDARD_EMV2000)
			if (cit_ch_interrupt_enable(handle, CIT_INTR_RCV,
								cit_ch_rcv_cb))
				goto error;
#endif
	} else if (channel->standard == SC_STANDARD_IRDETO) {
		if (cit_ch_interrupt_enable(handle, CIT_INTR_RCV,
							cit_ch_rcv_cb))
			goto error;
	}

	if (cit_ch_interrupt_enable(handle, CIT_INTR_PRES_INSERT,
							 cit_ch_cardinsert_cb))
		goto error;

	if (cit_ch_interrupt_enable(handle, CIT_INTR_PRES_REMOVE,
							 cit_ch_cardremove_cb))
		goto error;

	if (cit_ch_interrupt_enable(handle, CIT_INTR_TDONE, cit_ch_tdone_cb))
		goto error;

	goto done;
error:
	rv = -EINVAL;
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Set the smart card standard
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_standard(struct ch_handle *handle,
			       struct sc_channel_param *new)
{
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if ((new->protocol <= SC_ASYNC_PROTOCOL_UNKNOWN) ||
		(new->protocol > SC_ASYNC_PROTOCOL_E14_IRDETO)) {
		rv = -EINVAL;
		goto done;
	}

	/* Asynchronous Protocol Types. */
	switch (new->standard) {
	case SC_STANDARD_NDS:
		/* NDS. T=0 with flow control. */
		if (new->protocol != SC_ASYNC_PROTOCOL_E0)
			rv = -ENOTSUP;
		break;

	case SC_STANDARD_ISO:
	case SC_STANDARD_EMV1996:
	case SC_STANDARD_EMV2000:
	case SC_STANDARD_ARIB:
		/* T=0 or T=1 */
		if ((new->protocol != SC_ASYNC_PROTOCOL_E0) &&
			(new->protocol != SC_ASYNC_PROTOCOL_E1))
			rv = -ENOTSUP;
		break;

	case SC_STANDARD_IRDETO:
		/* Irdeto. T=14.  Need Major software workarouond to support */
		if ((new->protocol != SC_ASYNC_PROTOCOL_E0) &&
			(new->protocol != SC_ASYNC_PROTOCOL_E14_IRDETO))
			rv = -ENOTSUP;
		break;

	case SC_STANDARD_ES:
		/* ES, T=1.  Obsolete. Use ISO */
		if ((new->protocol != SC_ASYNC_PROTOCOL_E0) &&
			(new->protocol != SC_ASYNC_PROTOCOL_E1))
			rv = -ENOTSUP;
		break;

	case SC_STANDARD_MT:
	case SC_STANDARD_CONAX:
		/* MT, CONAX T=0.  Obsolete. Use ISO */
		if (new->protocol != SC_ASYNC_PROTOCOL_E0)
			rv = -ENOTSUP;
		break;

	default:
		rv = -EINVAL;
		break;
	}
	if (rv == 0)
		handle->ch_param.protocol = new->protocol;
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Set the frequency
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_frequency(struct ch_handle *handle,
				struct sc_channel_param *new)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u8_t new_val;
	u32_t new_pre;
	u32_t val, cnt;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	/* Set F, clock Rate Conversion Factor */
	if (new->f_factor == 0)
		ch_param->f_factor = SC_DEFAULT_F_FACTOR;
	else if ((new->f_factor >= SC_MIN_F_FACTOR) &&
			 (new->f_factor <= SC_MAX_F_FACTOR))
		ch_param->f_factor = new->f_factor;
	else
		goto error;
	SYS_LOG_DBG("f_factor %d", ch_param->f_factor);

	/* Set D, Baud Rate Adjustor */
	if (new->d_factor == 0)
		ch_param->d_factor = SC_DEFAULT_D_FACTOR;
	else if (((new->d_factor >= SC_MIN_D_FACTOR) &&
			(new->d_factor <= SC_MAX_D_FACTOR)) &&
			(sc_d_value[new->d_factor] != -1))
		ch_param->d_factor = new->d_factor;
	else
		goto error;
	SYS_LOG_DBG("d_factor %d", ch_param->f_factor);

	/* Set ETU Clock Divider */
	new_val = new->host_ctrl.etu_clkdiv;
	if (new_val == 0)
		ch_param->host_ctrl.etu_clkdiv =
			 cit_get_etu_clkdiv(new->d_factor, new->f_factor);
	else if ((new_val >= 1) && (new_val <= 8))
		ch_param->host_ctrl.etu_clkdiv = new_val;
	else
		goto error;
	SYS_LOG_DBG("etu_clkdiv %d", ch_param->host_ctrl.etu_clkdiv);

	/* Set SC Clock Divider */
	new_val = new->host_ctrl.sc_clkdiv;
	if (new_val == 0)
		ch_param->host_ctrl.sc_clkdiv =
			 cit_get_clkdiv(new->d_factor, new->f_factor);
	else if ((new_val == 1) || (new_val == 2) || (new_val == 3) ||
			(new_val == 4) || (new_val == 5) || (new_val == 8) ||
			(new_val == 10) || (new_val == 16) || (new_val == 32))
		ch_param->host_ctrl.sc_clkdiv = new_val;
	else
		goto error;
	SYS_LOG_DBG("sc_clkdiv %d", ch_param->host_ctrl.sc_clkdiv);

	/* Set external Clock Divisor.  For TDA only 1, 2,4,8 are valid. */
	if (new->ext_clkdiv == 0)
		ch_param->ext_clkdiv = SC_DEFAULT_EXTERNAL_CLOCK_DIVISOR;
	else
		ch_param->ext_clkdiv = new->ext_clkdiv;
	SYS_LOG_DBG("ext_clkdiv %d", ch_param->ext_clkdiv);

	/* Set Prescale */
	new_pre = new->host_ctrl.prescale;
	if (new_pre == 0) {
		val = cit_get_prescale(new->d_factor, new->f_factor);
		val = (val * ch_param->ext_clkdiv) +
			(ch_param->ext_clkdiv - 1);
		ch_param->host_ctrl.prescale = val;
	} else if (new_pre <= SC_MAX_PRESCALE) {
		val = (new_pre * ch_param->ext_clkdiv) +
			(ch_param->ext_clkdiv - 1);
		ch_param->host_ctrl.prescale = val;
	} else {
		goto error;
	}
	SYS_LOG_DBG("prescale %d", ch_param->host_ctrl.prescale);

	/* Set baud divisor */
	new_val = new->host_ctrl.bauddiv;
	if (new_val == 0) {
		ch_param->host_ctrl.bauddiv = SC_DEFAULT_BAUD_DIV;
	} else {
		for (cnt = 0; cnt < SC_MAX_BAUDDIV; cnt++) {
			if (new_val == bauddiv_valid[cnt])
				break;
		}
		if (cnt == SC_MAX_BAUDDIV)
			goto error;

		ch_param->host_ctrl.bauddiv = new_val;
	}
	SYS_LOG_DBG("bauddiv %d", ch_param->host_ctrl.bauddiv);

	/* Set ICC CLK Freq */
	ch_param->icc_clk_freq = handle->clock.clk_frequency /
				ch_param->host_ctrl.sc_clkdiv /
				ch_param->host_ctrl.etu_clkdiv /
				ch_param->ext_clkdiv;
	SYS_LOG_DBG("icc_clk_freq %d", ch_param->icc_clk_freq);

	ch_param->baudrate = handle->clock.clk_frequency /
				ch_param->host_ctrl.etu_clkdiv /
				(ch_param->host_ctrl.prescale + 1) /
				ch_param->host_ctrl.bauddiv;
	SYS_LOG_DBG("current baudrate %d", ch_param->baudrate);

	/* Verify for correct baudrate */
	if (ch_param->standard == SC_STANDARD_IRDETO) {
		if (ch_param->baudrate != (ch_param->icc_clk_freq /
			 SC_T14_IRDETO_CONSTANT_CLOCK_RATE_CONV_FACTOR))
			goto error;
	} else {
		if (ch_param->baudrate != (ch_param->icc_clk_freq *
			 sc_d_value[ch_param->d_factor]) /
			 sc_f_value[ch_param->f_factor])
			goto error;
	}

	goto done;
error:
	rv = -EINVAL;
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Set the work wait time
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_wait_time(struct ch_handle *handle,
			   struct sc_channel_param *new)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t val;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	/* Set work waiting time */
	if (new->work_wait_time.val == 0) {
		ch_param->work_wait_time.val = SC_DEFAULT_WORK_WAITING_TIME;
		ch_param->work_wait_time.unit = SC_TIMER_UNIT_ETU;
	} else {
		switch (new->work_wait_time.unit) {
		case SC_TIMER_UNIT_ETU:
			val = new->work_wait_time.val;
			break;

		case SC_TIMER_UNIT_CLK:
			val = (new->work_wait_time.val * ch_param->baudrate) /
						ch_param->icc_clk_freq;
			break;

		case SC_TIMER_UNIT_MSEC:
			val = (new->work_wait_time.val * ch_param->baudrate) /
							1000;
			break;

		default:
			goto error;
		}
		ch_param->work_wait_time.unit = SC_TIMER_UNIT_ETU;
		ch_param->work_wait_time.val = val;
	}
	SYS_LOG_DBG("work_wait_time = (%d %d)\n",
		ch_param->work_wait_time.val, ch_param->work_wait_time.unit);

	/* Set block Wait time */
	if (new->blk_wait_time.val == 0) {
		ch_param->blk_wait_time.val = SC_DEFAULT_BLOCK_WAITING_TIME;
		ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;
	} else {
		switch (new->blk_wait_time.unit) {
		case SC_TIMER_UNIT_ETU:
			val = new->blk_wait_time.val;
			break;

		case SC_TIMER_UNIT_CLK:
			val = (new->blk_wait_time.val * ch_param->baudrate) /
						ch_param->icc_clk_freq;
			break;
		case SC_TIMER_UNIT_MSEC:
			val = (new->blk_wait_time.val * ch_param->baudrate) /
							1000;
			break;

		default:
			goto error;
		}
		ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;
		ch_param->blk_wait_time.val = val;
	}

	ch_param->blk_wait_time_ext.val = new->blk_wait_time_ext.val;
	ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;
	SYS_LOG_DBG("blk_wait_time = (%d %d)\n",
		ch_param->blk_wait_time.val, ch_param->blk_wait_time.unit);

	if (new->cwi > SC_MAX_CHARACTER_WAIT_TIME_INTEGER)
		goto error;

	ch_param->cwi = new->cwi;
	goto done;
error:
	rv = -EINVAL;
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Set the guard time
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_guard_time(struct ch_handle *handle,
				 struct sc_channel_param *new)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	/* Set Extra Guard Time  */
	switch (new->extra_guard_time.unit) {
	case SC_TIMER_UNIT_ETU:
		val = new->extra_guard_time.val;
		break;
	case SC_TIMER_UNIT_CLK:
		val = (new->extra_guard_time.val * ch_param->baudrate) /
					ch_param->icc_clk_freq;
		break;
	case SC_TIMER_UNIT_MSEC:
		val = (new->extra_guard_time.val * ch_param->baudrate) /
					1000;
		break;
	default:
		return -EINVAL;
	}
	ch_param->extra_guard_time.val = val;
	ch_param->extra_guard_time.unit = SC_TIMER_UNIT_ETU;
	SYS_LOG_DBG("extra_guard_time = (%d %d)\n",
		ch_param->extra_guard_time.val,
		ch_param->extra_guard_time.unit);

	/* Set block guard time */
	if (new->blk_guard_time.val == 0) {
		ch_param->blk_guard_time.val = SC_DEFAULT_BLOCK_GUARD_TIME;
		ch_param->blk_guard_time.unit = SC_TIMER_UNIT_ETU;
	} else {
		switch (new->blk_guard_time.unit) {
		case SC_TIMER_UNIT_ETU:
			val = new->blk_guard_time.val;
			break;
		case SC_TIMER_UNIT_CLK:
			val = (new->blk_guard_time.val * ch_param->baudrate) /
						ch_param->icc_clk_freq;
			break;
		case SC_TIMER_UNIT_MSEC:
			val = (new->blk_guard_time.val * ch_param->baudrate) /
						 1000;
			break;
		default:
			return -EINVAL;
		}
		if ((val < SC_MIN_BLOCK_GUARD_TIME) ||
		    (val > SC_MAX_BLOCK_GUARD_TIME))
			return -EINVAL;
		ch_param->blk_guard_time.val = val;
		ch_param->blk_guard_time.unit = SC_TIMER_UNIT_ETU;
	}
	SYS_LOG_DBG("blk_guard_time= (%d %d)\n",
		ch_param->blk_guard_time.val, ch_param->blk_guard_time.unit);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Set the Transaction time
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_transaction_time(struct ch_handle *handle,
				       struct sc_channel_param *new)
{
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	/* Set block guard time */
	if (new->timeout.val == 0) {
		handle->ch_param.timeout.val = SC_DEFAULT_TIME_OUT;
		handle->ch_param.timeout.unit = SC_TIMER_UNIT_MSEC;
	} else {
		switch (new->timeout.unit) {
		case SC_TIMER_UNIT_ETU:
			val = (new->timeout.val * 1000000) /
				handle->ch_param.host_ctrl.bauddiv;
			break;
		case SC_TIMER_UNIT_CLK:
			val = (new->timeout.val * 1000000) /
				handle->ch_param.icc_clk_freq;
			break;
		case SC_TIMER_UNIT_MSEC:
			val = new->timeout.val;
			break;
		default:
			return -EINVAL;
		}
		handle->ch_param.timeout.val = val;
		handle->ch_param.timeout.unit = SC_TIMER_UNIT_MSEC;
	}
	SYS_LOG_DBG("timeout = (%d %d)\n",
		handle->ch_param.timeout.val, handle->ch_param.timeout.unit);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Set the edc parity settings
 *
 * @param handle Channel handle
 * @param new Smart card parameters structure
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_edc_parity(struct ch_handle *handle,
				 struct sc_channel_param *new)
{

	SMART_CARD_FUNCTION_ENTER();
	/* Set Number of transmit parity retries */
	if (new->tx_retries > SC_MAX_TX_PARITY_RETRIES)
		return -EINVAL;
	handle->ch_param.tx_retries = new->tx_retries;

	if (new->rx_retries > SC_MAX_RX_PARITY_RETRIES)
		return -EINVAL;

	handle->ch_param.rx_retries = new->rx_retries;
	handle->ch_param.edc = new->edc;
	SYS_LOG_DBG("edc = (%d %d)\n", handle->ch_param.edc.crc,
					handle->ch_param.edc.enable);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

s32_t cit_wait_for_event(struct ch_handle *handle, u8_t *event, s32_t time_out)
{
	s32_t rv = 0;
	s32_t cnt;

#ifdef BSCD_EMV2000_FIME
	if (handle->parity_error_cnt == 0xff) {
		handle->parity_error_cnt = 0;
		return -ETIMEDOUT;
	}
#endif
	cnt = 0;
	while ((*event == false) && (handle->ch_status.card_present == true)) {
		if (time_out != K_FOREVER) {
			if (time_out > cnt) {
				cnt++;
				cit_sc_delay(1);
			} else {
				rv = -ETIMEDOUT;
				break;
			}
		}
	}

	if (handle->ch_status.card_present == false)
		rv = -EFAULT;

	if (rv == 0)
		*event = false;

	return rv;
}

void channel_dump_param(const struct sc_channel_param  *ch_param)
{
	if (!ch_param)
		return;
	SYS_LOG_DBG("\n\n================================================");
	SYS_LOG_DBG("standard = %02x", ch_param->standard);
	SYS_LOG_DBG("protocol = %02x", ch_param->protocol);
	SYS_LOG_DBG("icc_clk_freq = %d", ch_param->icc_clk_freq);
	SYS_LOG_DBG("baudrate = %d", ch_param->baudrate);
	SYS_LOG_DBG("max_ifsd = %d", ch_param->max_ifsd);
	SYS_LOG_DBG("ifsc = %d", ch_param->ifsc);
	SYS_LOG_DBG("f_factor = %d", ch_param->f_factor);
	SYS_LOG_DBG("d_factor = %d", ch_param->d_factor);
	SYS_LOG_DBG("ext_clkdiv  = %d", ch_param->ext_clkdiv);
	SYS_LOG_DBG("baudrate = %d", ch_param->baudrate);
	SYS_LOG_DBG("tx_retries = %d", ch_param->tx_retries);
	SYS_LOG_DBG("rx_retries = %d", ch_param->rx_retries);
	SYS_LOG_DBG("work_wait_time = (%d %d)",
		ch_param->work_wait_time.val, ch_param->work_wait_time.unit);
	SYS_LOG_DBG("blk_wait_time = (%d %d)",
		ch_param->blk_wait_time.val, ch_param->blk_wait_time.unit);
	SYS_LOG_DBG("extra_guard_time = (%d %d)",
		ch_param->extra_guard_time.val,
		ch_param->extra_guard_time.unit);
	SYS_LOG_DBG("blk_guard_time= (%d %d)",
		ch_param->blk_guard_time.val, ch_param->blk_guard_time.unit);
	SYS_LOG_DBG("cwi = %d", ch_param->cwi);
	SYS_LOG_DBG("edc = (%d %d)", ch_param->edc.crc, ch_param->edc.enable);
	SYS_LOG_DBG("timeout = (%d %d)",
		ch_param->timeout.val, ch_param->timeout.unit);
	SYS_LOG_DBG("auto_deactive_req = %d",  ch_param->auto_deactive_req);
	SYS_LOG_DBG("null_filter = %d",  ch_param->null_filter);
	SYS_LOG_DBG("read_atr_on_reset = %d",  ch_param->read_atr_on_reset);
	SYS_LOG_DBG("blk_wait_time_ext = (%d %d)",
				ch_param->blk_wait_time_ext.val,
				ch_param->blk_wait_time_ext.unit);
	SYS_LOG_DBG("btpdu = %d", ch_param->btpdu);
	SYS_LOG_DBG("etu_clkdiv = %d", ch_param->host_ctrl.etu_clkdiv);
	SYS_LOG_DBG("sc_clkdiv = %d", ch_param->host_ctrl.sc_clkdiv);
	SYS_LOG_DBG("prescale = %d", ch_param->host_ctrl.prescale);
	SYS_LOG_DBG("bauddiv = %d", ch_param->host_ctrl.bauddiv);

	SYS_LOG_DBG("****END****\n");
}

void cit_channel_dump_registers(struct ch_handle *handle)
{
	u32_t b = handle->base;

	if (!b)
		return;
	SYS_LOG_DBG("UART_CMD_1 = %02x", sys_read32(b + CIT_UART_CMD_1));
	SYS_LOG_DBG("UART_CMD_2 = %02x", sys_read32(b + CIT_UART_CMD_2));
	SYS_LOG_DBG("CLK_CMD_1 = %02x", sys_read32(b + CIT_CLK_CMD_1));
	SYS_LOG_DBG("PROTO_CMD = %02x", sys_read32(b + CIT_PROTO_CMD));
	SYS_LOG_DBG("PRESCALE = %02x", sys_read32(b + CIT_PRESCALE));
	SYS_LOG_DBG("RLEN_2 = %02x", sys_read32(b + CIT_RLEN_2));
	SYS_LOG_DBG("BGT = %02x", sys_read32(b + CIT_BGT));
	SYS_LOG_DBG("IF_CMD_2 = %02x", sys_read32(b + CIT_IF_CMD_2));
	SYS_LOG_DBG("INTR_EN_1 = %02x", sys_read32(b + CIT_INTR_EN_1));
	SYS_LOG_DBG("INTR_EN_2 = %02x", sys_read32(b + CIT_INTR_EN_2));
	SYS_LOG_DBG("INTR_STAT_1 = %02x", sys_read32(b + CIT_INTR_STAT_1));
	SYS_LOG_DBG("INTR_STAT_2 = %02x", sys_read32(b + CIT_INTR_STAT_2));
	SYS_LOG_DBG("TIMER_CMP_1 = %02x", sys_read32(b + CIT_TIMER_CMP_1));
	SYS_LOG_DBG("TIMER_CMP_2 = %02x", sys_read32(b + CIT_TIMER_CMP_2));
	SYS_LOG_DBG("TIMER_CNT_1 = %02x", sys_read32(b + CIT_TIMER_CNT_1));
	SYS_LOG_DBG("TIMER_CNT_2 = %02x", sys_read32(b + CIT_TIMER_CNT_2));
	SYS_LOG_DBG("WAIT_1 = %02x", sys_read32(b + CIT_WAIT_1));
	SYS_LOG_DBG("WAIT_2 = %02x", sys_read32(b + CIT_WAIT_2));
	SYS_LOG_DBG("WAIT_3 = %02x", sys_read32(b + CIT_WAIT_3));
	SYS_LOG_DBG("IF_CMD_3 = %02x", sys_read32(b + CIT_IF_CMD_3));
}
