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
 * @file sc_cit_atrpriv.c
 * @brief Smart card ATR related functions
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

#define SC_ATR_ETU_IMPLICIT_MASK		0x10

/**
 * @brief Read a byte from receive buffer
 *
 * @param handle Channel handle
 * @param data data buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_iso_atr_read_next_byte(struct ch_handle *handle, u8_t *data)
{
	s32_t rv = 0;
	u32_t rx_len = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (handle->rx_len == SC_MAX_ATR_SIZE)
		return -EINVAL;

	rv = cit_channel_receive_atr(handle, data, &rx_len, 1);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Read the ATR for ISO type and decode it
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_iso_atr_receive_decode(struct ch_handle *handle)
{
	struct sc_channel_param *n_ch_param = &handle->ch_neg_param;
	u8_t t0_byte, td1 = 0, td2 = 0, td3 = 0;
	u8_t len = 0;
	u8_t val, val1;
	u8_t buf[SC_MAX_RX_SIZE];
	u8_t historical_bytes = 0;
	bool tck_required = false;
	s32_t rv;

	SMART_CARD_FUNCTION_ENTER();
	/* assign negotiate settings to default */
	cit_get_default_channel_settings(0, &(handle->ch_neg_param));

	/* TS */
	len = 0;
	rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
	if (rv)
		goto exit;

	if ((buf[len] != SC_ATR_INVERSE_CONVENSION) &&
	    (buf[len] != SC_ATR_DIRECT_CONVENSION))
		goto error;

	/* T0 */
	len++;
	rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
	if (rv)
		goto exit;

	t0_byte = buf[len];
	historical_bytes = t0_byte & SC_ATR_LSN_MASK;
	len++;

	/* TA1 */
	if (t0_byte & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		n_ch_param->d_factor = val & SC_ATR_TA1_D_FACTOR;
		n_ch_param->f_factor = (val & SC_ATR_TA1_F_FACTOR) >>
						SC_ATR_TA1_F_FACTOR_SHIFT;

		if (val != SC_ATR_TA1_DEFAULT)
			handle->pps_needed = true;
		else
			handle->pps_needed = false;
	}
	/* TB1 */
	if (t0_byte & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		if (buf[len++])
			if (handle->reset_type == SC_COLD_RESET) {
				SYS_LOG_ERR("Non-Zero TB1 with cold reset");
				goto error;
			}
	}
	/* TC1 */
	if (t0_byte & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		if ((n_ch_param->protocol == SC_ASYNC_PROTOCOL_E0) &&
		    (val == UCHAR_MAX))
			n_ch_param->extra_guard_time.val = 0;
		else
			n_ch_param->extra_guard_time.val = val;

		n_ch_param->extra_guard_time.unit = SC_TIMER_UNIT_ETU;
	}
	/* TD1 */
	if (t0_byte & SC_ATR_TD_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		td1 = buf[len++];
		switch (td1 & SC_ATR_TD1_PROTOCOL_MASK) {
		case SC_ATR_PROTOCOL_T0:
			break;
		case SC_ATR_PROTOCOL_T1:
			n_ch_param->protocol = SC_ASYNC_PROTOCOL_E1;
			tck_required = true;
			break;
		default:
			SYS_LOG_ERR("Erroneous TD1");
			goto error;
		}
	}
	/* TA2 */
	if (td1 & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		if (val & SC_ATR_ETU_IMPLICIT_MASK) {
			SYS_LOG_ERR("Invalid TA2, this uses implicit values");
			goto error;
		} else {
			handle->pps_needed = false;
		}
	}
	/* TB2 */
	if (td1 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;
		len++;
		SYS_LOG_ERR("TB2 present - not required for Europay standard");
		goto error;
	}
	sc_f_d_adjust_without_update(n_ch_param, n_ch_param->f_factor,
				     n_ch_param->d_factor);
	n_ch_param->baudrate = handle->clock.clk_frequency /
				 n_ch_param->host_ctrl.etu_clkdiv/
				(n_ch_param->host_ctrl.prescale + 1)/
				n_ch_param->host_ctrl.bauddiv;
	/* TC2 */
	if (td1 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		if (val) {
			rv = cit_adjust_wwt(n_ch_param, val);
			if (rv)
				goto exit;
		} else {
			SYS_LOG_ERR("Invalid TC2");
			goto error;
		}
	}
	/* TD2 */
	if (td1 & SC_ATR_TD_PRESENT_MASK) {
		u8_t td1_lsn, td2_lsn;

		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		td2 = buf[len++];
		td1_lsn = td1 & SC_ATR_LSN_MASK;
		td2_lsn = td2 & SC_ATR_LSN_MASK;

		if ((td1_lsn == 0) && (td2_lsn != 0)) {
			/* If TD1_L and TD2_L are not same, TCK is needed */
			tck_required = true;
		} else if ((td1_lsn == SC_ATR_PROTOCOL_T1) && (td2_lsn == 0)) {
			/*
			 * If l.s. nibble of TD1 is '1', then l.s. nibble of
			 * TD2 must not be 0 according to standard
			 */
			SYS_LOG_ERR("Invalid TD2");
			goto error;
		}

		if ((td1_lsn != td2_lsn) && (td1_lsn <= SC_ATR_PROTOCOL_MAX) &&
			 (td1_lsn <= SC_ATR_PROTOCOL_MAX))
			handle->pps_needed = true;
	}
	/* TA3 */
	if (td2 & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		if ((val <= SC_ATR_LSN_MASK) || (val == UCHAR_MAX)) {
			SYS_LOG_ERR("Invalid ISO TA3");
			goto error;
		} else {
			n_ch_param->ifsc = val;
		}
	}
	/* TB3 */
	if (td2 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		val = buf[len++];
		val1 = (val & SC_ATR_TB3_BWI_MASK) >> SC_ATR_TB3_BWI_SHIFT;

		if (n_ch_param->protocol == SC_ASYNC_PROTOCOL_E1) {
			n_ch_param->cwi = (val & CIT_PROTO_CMD_CWI_MASK);
			cit_channel_set_bwt_integer(n_ch_param, val1);
		}
	}
	/* TC3 */
	if (td2 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		/* Terminal shall reject ATR with non-zero TC3 Byte */
		if (buf[len++]) {
			SYS_LOG_ERR("Invalid TC3");
			goto error;
		}
	}
	/* TD3 */
	if (td2 & SC_ATR_TD_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		td3 = buf[len++];
	}
	/* TA4 */
	if (td3 & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		len++;
	}
	/* TB4 */
	if (td3 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		len++;
	}
	/* TC4 */
	if (td3 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		len++;
	}

	if (historical_bytes) {
		u8_t *hist_buf = &buf[len];
		u8_t cnt;

		for (cnt = 0; cnt < historical_bytes; cnt++) {
			rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
			if (rv)
				goto exit;
			len++;
		}
		cit_channel_set_card_type_character(handle, hist_buf,
							 historical_bytes);
	}

	if (tck_required) {
		u8_t tck_calc = 0;
		u32_t cnt;

		rv = cit_iso_atr_read_next_byte(handle, &buf[len]);
		if (rv)
			goto exit;

		len++;

		/* From T0 to TCK.  Including historical bytes if they exist */
		for (cnt = 1; cnt < len; cnt++)
			tck_calc = tck_calc ^ buf[cnt];

		if (tck_calc) {
			SYS_LOG_ERR("TCK byte integrity fail\n");
			goto error;
		}
	}
	goto exit;
error:
	rv = -EINVAL;
exit:
	memcpy(&handle->auc_rx_buf[0], buf, len);
	handle->rx_len = len;
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_channel_atr_byte_read(struct ch_handle *handle, u8_t *data,
				u32_t byte_time, u32_t max_time,
				u32_t *current_time)
{
	u32_t timer_cnt_val1, timer_cnt_val2, timer_cnt_val;
	u32_t base = handle->base;
	u32_t status;
	s32_t rv = 0;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
								false, false};
	struct cit_timer wwt_timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE,
								false, false};

	ARG_UNUSED(byte_time);
	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);
	status = handle->status2 = sys_read32(base + CIT_STATUS_2);
	cit_sc_exit_critical_section(handle);

	if (status & BIT(CIT_STATUS_2_REMPTY_BIT)) {
		/* Do not have any byte in SC_RECEIVE */
		rv = cit_wait_rcv(handle);
		if (rv) {
			/*
			 * disable the timer, always return the  previous error
			 * Disable timer, which was enable upon receiving
			 * atr_intr
			 */
			cit_timer_isr_control(handle, &timer);
			/* disable WWT.  This was enabled in activating time */
			wwt_timer.timer_mode = CIT_WAIT_TIMER_WWT;
			cit_timer_isr_control(handle, &wwt_timer);
			/* Read timer count and accumulate it to current_time */
			timer_cnt_val2 = sys_read32(base + CIT_TIMER_CNT_2);
			timer_cnt_val1 = sys_read32(base + CIT_TIMER_CNT_1);
			timer_cnt_val = (timer_cnt_val2 << 8) | timer_cnt_val1;
			*current_time += timer_cnt_val;

			if (*current_time > max_time)
				return -EINVAL;
		}
	} else {
		cit_sc_enter_critical_section(handle);
		handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_RCV_BIT);
		cit_sc_exit_critical_section(handle);
	}

	*data = sys_read32(base + CIT_RECEIVE);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t cit_emv_validate_tb3(struct ch_handle *handle, u8_t tb3);

/**
 * @brief Read next byte in ATR from receive buffer
 *
 * @param handle Channel handle
 * @param data Data buffer
 * @param time Total ATR byte time in ETU
 * @param p_err True if parity error detected
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_emv_atr_rd_next_byte(struct ch_handle *handle, u8_t *data,
			       u32_t *time, bool *p_err)
{
	u32_t val;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (handle->rx_len == SC_MAX_ATR_SIZE) {
		rv = -EINVAL;
		goto done;
	}

	/*EMV2000*/
	if (handle->ch_param.standard == SC_STANDARD_EMV2000)
		rv = cit_channel_atr_byte_read(handle, data,
				SC_MAX_ETU_PER_ATR_BYTE_EMV2000,
				SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES_EMV2000,
				time);
	else /* EMV 96 */
		rv = cit_channel_atr_byte_read(handle, data,
			SC_MAX_ETU_PER_ATR_BYTE,
			SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES,
			time);
	if (rv) {
		rv = -ECANCELED;
		goto done;
	}

	val = sys_read32(handle->base + CIT_STATUS_2);
	/*
	 * If a parity error is detected, then continue to read in entire
	 * ATR, but immediately after doing so, return FAILED to deactivate as
	 * per EMV spec
	 */
	if (val & BIT(CIT_STATUS_2_RPAR_ERR)) {

		SYS_LOG_DBG("Parity Error found in ATR byte");
		/*
		 * Clear this retry interrupt so that we could continue reading.
		 * Test 1726 x=2 need this
		 */
		cit_sc_enter_critical_section(handle);
		handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_RETRY_BIT);
		cit_sc_exit_critical_section(handle);
		*p_err = true;
	}

done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Check for additional bytes in ATR for EMV type it
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_emv_check_additional_atr_bytes(struct ch_handle *handle)
{
#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
	u32_t base = handle->base;
#endif
	u32_t total_atr_etu = 0;
	u32_t byte_count = 0;
	u8_t val;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	while (rv == 0) {
		/*
		 * Since GP is used for total ATR time and WWT is used for WWT,
		 * we use event2_intr for this 200 ETU checking
		 */
#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
		/* 200 ETU */
		sys_write32(200, (base + CIT_EVENT2_CMP));

		/* start event src */
		sys_write32(CIT_RX_ETU_TICK_EVENT_SRC, base + CIT_EVENT2_CMD_3);

		/* increment event src */
		sys_write32(CIT_RX_ETU_TICK_EVENT_SRC, base + CIT_EVENT2_CMD_2);

		/* reset event src */
		sys_write32(CIT_NO_EVENT_EVENT_SRC, base + CIT_EVENT2_CMD_1);

		/* event_en, intr_mode, run_after_reset and run_after_compare*/
		val = BIT(CIT_EVENT2_CMD_4_EVENT_EN) |
			BIT(CIT_EVENT2_CMD_4_INR_AFTER_COMPARE) |
			BIT(CIT_EVENT2_CMD_4_RUN_AFTER_RESET) |
			BIT(CIT_EVENT2_CMD_4_RUN_AFTER_COMPARE);
		val &= ~(BIT(CIT_EVENT2_CMD_4_INR_AFTER_RESET));

		sys_write32(val, base + CIT_EVENT2_CMD_4);

		rv = cit_ch_interrupt_enable(handle, CIT_INTR_EVENT2,
							cit_ch_event2_cb);
		if (rv)
			return -ECANCELED;
#endif
		rv = cit_channel_atr_byte_read(handle, &val, 200,
					SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES,
					&total_atr_etu);

#ifdef BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
		/* Disable event2 */
		sys_clear_bit((base + CIT_EVENT2_CMD_4),
					CIT_EVENT2_CMD_4_EVENT_EN);
#endif
		if (rv == 0) {
			SYS_LOG_DBG("SC: EMV ATR Bytes: Extra Byte Detected\n");
			byte_count++;
		}
	}

	if (byte_count)
		rv = -EINVAL;
	else
		rv = 0;

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Read the ATR for EMV type and decode it
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_emv_atr_receive_decode(struct ch_handle *handle)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u8_t f_val, d_val;
	u8_t historical_bytes = 0;
	u8_t t0_byte, td1 = 0, td2 = 0, td3 = 0, tc1 = 0;
	u8_t buf[SC_MAX_RX_SIZE];
	u32_t time = 0;
	bool p_err = false;
	bool failed_atr = false;
	bool tck_required = false;
	u32_t val;
	u8_t len = 0;
	s32_t rv = 0;
	struct cit_timer timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE,
							false, true};
	struct cit_timer wwt_timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE,
							false, false};

	SMART_CARD_FUNCTION_ENTER();
	handle->rx_len = 0;
	rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
	if (rv == -ECANCELED)
		goto done;

	if ((buf[len] != SC_ATR_INVERSE_CONVENSION) &&
	    (buf[len] != SC_ATR_DIRECT_CONVENSION)) {
		SYS_LOG_ERR("TS = %02x, Unknown convention\n", buf[len]);
		rv = -ECANCELED;
		goto done;
	}

	/* T0 */
	len++;
	rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
	if (rv == -ECANCELED)
		goto done;

	t0_byte = buf[len++];
	historical_bytes = t0_byte & SC_ATR_LSN_MASK;

	/* TA1 */
	if (t0_byte & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
		goto done;

		val = buf[len++];
		if (!((val == SC_EMV_ATR_TA1_VALID1) ||
		      (val == SC_EMV_ATR_TA1_VALID2) ||
		      (val == SC_EMV_ATR_TA1_VALID3))) {
			SYS_LOG_ERR("Invalid adjustment factor");
			failed_atr = true;
		}
	} else {
		val = SC_ATR_TA1_DEFAULT;
	}

	if (failed_atr == false) {
		ch_param->d_factor = val & SC_ATR_TA1_D_FACTOR;
		ch_param->f_factor = (val & SC_ATR_TA1_F_FACTOR) >>
					SC_ATR_TA1_F_FACTOR_SHIFT;
	}
	/* TB1 */
	if (t0_byte & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		if (buf[len++])
			if (handle->reset_type == SC_COLD_RESET) {
				SYS_LOG_ERR("Non-Zero TB1 with cold reset");
				failed_atr = true;
			}
	} else if (handle->reset_type == SC_COLD_RESET) {
		SYS_LOG_ERR("Missing TB1 with cold reset");
		failed_atr = true;
	}
	/* TC1 */
	if (t0_byte & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		tc1 = buf[len++];
	} else {
		ch_param->extra_guard_time.val = 0;
		ch_param->extra_guard_time.unit = SC_TIMER_UNIT_ETU;
		sys_write32(0, handle->base + CIT_TGUARD);

	}
	/* TD1 */
	if (t0_byte & SC_ATR_TD_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		td1 = buf[len++];
		switch (td1 & SC_ATR_TD1_PROTOCOL_MASK) {
		case SC_ATR_PROTOCOL_T0:
			break;
		case SC_ATR_PROTOCOL_T1:
			ch_param->protocol = SC_ASYNC_PROTOCOL_E1;
			tck_required = true;
			break;
		default:
			SYS_LOG_ERR("Erroneous TD1");
			tck_required = true;
			failed_atr = true;
		}
	}
	/* check for TC1 here */
	if (tc1 != 0) {
		if ((ch_param->protocol == SC_ASYNC_PROTOCOL_E0) &&
		    (tc1 == UCHAR_MAX))
			ch_param->extra_guard_time.val = 0;
		else
			ch_param->extra_guard_time.val = tc1;

		ch_param->extra_guard_time.unit = SC_TIMER_UNIT_ETU;
		sys_write32(ch_param->extra_guard_time.val,
					 handle->base + CIT_TGUARD);
	}
	/* TA2 */
	if (td1 & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		val = buf[len++];
		if (val & SC_ATR_ETU_IMPLICIT_MASK) {
			SYS_LOG_ERR("Invalid TA2");
			failed_atr = true;
		}
	} else {
		ch_param->d_factor = SC_ATR_DEFAULT_D_FACTOR;
		ch_param->f_factor = SC_ATR_DEFAULT_F_FACTOR;
	}
	/* EMV2000 */
	d_val = ch_param->d_factor;
	f_val = ch_param->f_factor;
	ch_param->host_ctrl.etu_clkdiv = cit_get_etu_clkdiv(d_val, f_val);
	ch_param->host_ctrl.sc_clkdiv = cit_get_clkdiv(d_val, f_val);
	val = cit_get_prescale(d_val, f_val);
	val = (val * handle->ch_param.ext_clkdiv) +
				(handle->ch_param.ext_clkdiv - 1);
	ch_param->host_ctrl.prescale = val;
	ch_param->host_ctrl.bauddiv = cit_get_bauddiv(d_val, f_val);
	ch_param->baudrate = handle->clock.clk_frequency /
				ch_param->host_ctrl.etu_clkdiv /
				(ch_param->host_ctrl.prescale + 1) /
				ch_param->host_ctrl.bauddiv;
	rv = cit_adjust_wwt(&handle->ch_param,
			 SC_ISO_DEFAULT_WORK_WAIT_TIME_INTEGER);
	/* TB2 */
	if (td1 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;
		len++;
		SYS_LOG_ERR("TB2 present - not required for Europay standard");
		failed_atr = true;
	}
	/* TC2 */
	if (td1 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		val = buf[len++];
		if ((val == 0) || (val > 0x0a)) {
			SYS_LOG_ERR("Invalid TC2");
			failed_atr = true;
		} else {
			rv = cit_adjust_wwt(ch_param, val);
			if (rv)
				failed_atr = true;
		}
	} else {
		rv = cit_adjust_wwt(ch_param,
					 SC_ISO_DEFAULT_WORK_WAIT_TIME_INTEGER);
		if (rv)
			goto done;
	}
	/* TD2 */
	if (td1 & SC_ATR_TD_PRESENT_MASK) {
		u8_t td1_lsn, td2_lsn;

		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		td2 = buf[len++];
		td1_lsn = td1 & SC_ATR_LSN_MASK;
		td2_lsn = td2 & SC_ATR_LSN_MASK;

		if (td1_lsn == SC_ATR_PROTOCOL_T0) {
			if ((td2_lsn != SC_ATR_PROTOCOL_MAX) &&
					(td2_lsn != SC_ATR_PROTOCOL_T1)) {

				/* If TD1_L is 0, TD2_L must be 0x0e or 0x01 */
				SYS_LOG_ERR("Invalid TD2");
				failed_atr = true;
			} else {
				tck_required = true;
			}
		} else if (td1_lsn == SC_ATR_PROTOCOL_T1) {
			/*
			 * If l.s. nibble of TD1 is '1', then l.s. nibble of
			 * TD2 must be 0x01
			 */
			if (td2_lsn != SC_ATR_PROTOCOL_T1) {
				SYS_LOG_ERR("Invalid TD2");
				failed_atr = true;
			}
		}

	}
	/* TA3 */
	if (td2 & SC_ATR_TA_PRESENT_MASK) {

		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		val = buf[len++];
		if ((val <= SC_ATR_LSN_MASK) || (val == UCHAR_MAX)) {
			SYS_LOG_ERR("Invalid ISO TA3");
			failed_atr = true;
		} else {
			ch_param->ifsd = val;
		}
	} else {
		ch_param->ifsd = SC_DEFAULT_EMV_INFORMATION_FIELD_SIZE;
	}
	/* TB3 */
	if (td2 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		val = buf[len++];
		rv = cit_emv_validate_tb3(handle, (u8_t)val);
		if (rv)
			failed_atr = true;
	} else {
		/*
		 * if TB3 is absent, then we must be in T = 0 mode,
		 * otherwise reject ATR
		 */
		if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
			SYS_LOG_DBG("Invalid: TB3 absent in T = 1");
			/*
			 * Cannot just return BSCD_STATUS_FAILED.
			 * Test 1800 needs to read the TCK
			 */
			failed_atr = true;
		}
	}
	/* TC3 */
	if (td2 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		/* Terminal shall reject ATR with non-zero TC3 Byte */
		if (buf[len++]) {
			SYS_LOG_ERR("Invalid TC3");
			failed_atr = true;
		}
	}
	/* TD3 */
	if (td2 & SC_ATR_TD_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		td3 = buf[len++];
	}
	/* TA4 */
	if (td3 & SC_ATR_TA_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		len++;
	}
	/* TB4 */
	if (td3 & SC_ATR_TB_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		len++;
	}
	/* TC4 */
	if (td3 & SC_ATR_TC_PRESENT_MASK) {
		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		len++;
	}

	/* EMV 2000 */
	if (historical_bytes) {
		u8_t *hist_buf = &buf[len];
		u8_t cnt;

		for (cnt = 0; cnt < historical_bytes; cnt++) {
			rv = cit_emv_atr_rd_next_byte(handle, &buf[len],
						      &time, &p_err);
			if (rv == -ECANCELED)
				goto done;
			len++;
		}
		cit_channel_set_card_type_character(handle, hist_buf,
							 historical_bytes);
	}

	if (tck_required) {
		u8_t tck_calc = 0;
		u32_t cnt;

		rv = cit_emv_atr_rd_next_byte(handle, &buf[len], &time, &p_err);
		if (rv == -ECANCELED)
			goto done;

		len++;
		/* From T0 to TCK.  Including historical bytes if they exist */
		for (cnt = 1; cnt < len; cnt++)
			tck_calc = tck_calc ^ buf[cnt];

		if (tck_calc) {
			SYS_LOG_ERR("TCK byte integrity fail\n");
			rv = -ECANCELED;
			goto done;
		}
	}

	/*
	 * Only after reading entire ATR, now if a parity error has been
	 * detected, return BSCD_STATUS_FAILED
	 */
	if (p_err)
		return -ECANCELED;

	rv = cit_channel_emv_check_additional_atr_bytes(handle);
	if (rv)
		goto done;

	/* Disable timer, which was enable upon receiving atr_intr */
	cit_timer_isr_control(handle, &timer);

	/* disable WWT.  This was enabled in activating time */
	wwt_timer.timer_mode = CIT_WAIT_TIMER_WWT;
	cit_timer_isr_control(handle, &wwt_timer);

	if (failed_atr) {
		rv = -EINVAL;
		goto done;
	}

	rv = sc_f_d_adjust(handle, f_val, d_val);
	if (rv)
		goto done;

	rv = cit_channel_enable_interrupts(handle);
	if (rv)
		goto done;

	val = sys_read32(handle->base + CIT_UART_CMD_2);
	val &= BIT(CIT_UART_CMD_2_CONVENTION_BIT);
	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) {
		val |= handle->ch_param.tx_retries;
		val |= handle->ch_param.rx_retries <<
				 CIT_UART_CMD_2_RPAR_RETRY_SHIFT;
	}
	sys_write32(val, (handle->base + CIT_UART_CMD_2));
done:
	memcpy(&handle->auc_rx_buf[0], buf, len);
	handle->rx_len = len;
	/* Disable timer, which was enable upon receiving atr_intr */
	cit_timer_isr_control(handle, &timer);

	/* disable WWT.  This was enabled in activating time */
	wwt_timer.timer_mode = CIT_WAIT_TIMER_WWT;
	cit_timer_isr_control(handle, &wwt_timer);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Validate the TB3 byte in ATR
 *
 * @param handle Channel handle
 * @param tb3 TB3 byte
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_emv_validate_tb3(struct ch_handle *handle, u8_t tb3)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	u32_t guard_time, guard_time_plus_one, cwt_val, bwt;
	u32_t base = handle->base;
	u8_t cwi_val, bwi_val;
	u8_t baud_div, clk_div;
	u32_t cwi_check;

	SMART_CARD_FUNCTION_ENTER();
	if (ch_param->protocol == SC_ASYNC_PROTOCOL_E1) {
		/* Decode TB3. */
		cwi_val = tb3 & SC_ATR_LSN_MASK;
		bwi_val = (tb3 & SC_ATR_MSN_MASK) >> 4;

		/*
		 * Obtain the power(2,CWI) factor from the value of
		 * CWI - see TB3 in EMV'96 spec for more
		 */
		if (cwi_val)
			cwi_check = 2 << (cwi_val - 1);
		else
			cwi_check = 1;
		/* Obtain guard time for comparison check below */
		guard_time = ch_param->extra_guard_time.val;
		/*
		 * Note: if ulGuardTime = 0xFF, then unGuardTimePlusOne = 0,
		 * according to EMV ATR rules P. I-28
		 */
		if (guard_time == 0xff)
			guard_time_plus_one = 0;
		else
			guard_time_plus_one = guard_time + 1;
		/*
		 * Terminal shall reject ATR not containing TB3, or containing
		 * TB3 indicating BWI greater than 4 and/or CWI greater than 5,
		 * or power(2,CWI) < (N+1), where N is value of TC1
		 */
		if ((cwi_val > 5) || (bwi_val > 4) ||
				(cwi_check <= guard_time_plus_one)) {
			SYS_LOG_ERR("Invalid TB3 = %02x", tb3);
			return -EINVAL;
		}
		/* Set CWT */
		cwt_val = sys_read32(base + CIT_PROTO_CMD);

		/*
		 *  And with 0xf0 to remove the original 0x0f inserted
		 *  into this register
		 */
		cwt_val &= 0xf0;
		cwt_val |= cwi_val;
		handle->ch_param.cwi = cwi_val;
#ifdef BSCD_EMV2000_CWT_PLUS_4
		if ((ch_param->standard != SC_STANDARD_EMV2000) ||
		    (ch_param->protocol != SC_ASYNC_PROTOCOL_E1))
			sys_write32(cwt_val, base + CIT_PROTO_CMD);
#else
		sys_write32(cwt_val, base + CIT_PROTO_CMD);
#endif
		/* set BWT */
		/*
		 * The block waiting time is encoded as described in
		 * ISO 7816-3, repeated here in the following equation:
		 * BWT = [11 + 2 bwi x 960 x 372 x D/F] etu
		 *
		 * e.g If bwi = 4 and F/D = 372 then BWT = 15,371 etu.
		 * The minimum and maximum BWT are ~186 and 15,728,651
		 * etu.
		 */
		ch_param->host_ctrl.prescale =
				 cit_get_prescale(ch_param->d_factor,
				 ch_param->f_factor) * ch_param->ext_clkdiv +
					(ch_param->ext_clkdiv - 1);
		baud_div = cit_get_bauddiv(ch_param->d_factor,
							 ch_param->f_factor);
		clk_div = cit_get_clkdiv(ch_param->d_factor,
							 ch_param->f_factor);

		if (bwi_val == 0x00)
			bwt = 960 * 372 * clk_div /
				(ch_param->host_ctrl.prescale + 1) /
						baud_div + 11;
		else
			bwt = (2 << (bwi_val - 1)) * 960 *  372 * clk_div /
					(ch_param->host_ctrl.prescale + 1) /
						baud_div + 11;

		/* Change timer to equal calculated BWT */
		if (ch_param->standard == SC_STANDARD_EMV2000)
			ch_param->blk_wait_time.val = bwt +
				sc_d_value[handle->ch_param.d_factor] *
				SC_DEFAULT_EXTRA_BLOCK_WAITING_TIME_EMV2000;
		else
			ch_param->blk_wait_time.val = bwt;

		ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;
		SYS_LOG_DBG("TB3, blockWaitTime = %d\n",
					ch_param->blk_wait_time.val);
	}

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}
