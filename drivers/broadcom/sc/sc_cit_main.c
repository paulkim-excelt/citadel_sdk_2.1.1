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
 * @file sc_cit_main.c
 * @brief smart card driver implementation
 */

#include <arch/cpu.h>
#include <stdbool.h>
#include <string.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
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
#include "sc_acos2.h"

static struct sc_channel_param default_channel_settings = {
	SC_STANDARD_ISO,
	SC_ASYNC_PROTOCOL_E0,
	{0x00},
	SC_INTERNAL_CLOCK_FREQ / SC_DEFAULT_ETU_CLKDIV / SC_DEFAULT_SC_CLKDIV,
	SC_INTERNAL_CLOCK_FREQ/SC_DEFAULT_ETU_CLKDIV /
			 (SC_DEFAULT_PRESCALE + 1)/SC_DEFAULT_BAUD_DIV,
	SC_MAX_TX_SIZE,
	SC_DEFAULT_EMV_INFORMATION_FIELD_SIZE,
	SC_DEFAULT_F_FACTOR,
	SC_DEFAULT_D_FACTOR,
	SC_DEFAULT_EXTERNAL_CLOCK_DIVISOR,
	SC_DEFAULT_TX_PARITY_RETRIES,
	SC_DEFAULT_RX_PARITY_RETRIES,
	{SC_DEFAULT_WORK_WAITING_TIME, SC_TIMER_UNIT_ETU},
	{SC_DEFAULT_BLOCK_WAITING_TIME, SC_TIMER_UNIT_ETU},
	{SC_DEFAULT_EXTRA_GUARD_TIME, SC_TIMER_UNIT_ETU},
	{SC_DEFAULT_BLOCK_GUARD_TIME, SC_TIMER_UNIT_ETU},
	SC_DEFAULT_CHARACTER_WAIT_TIME_INTEGER,
	{SC_DEFAULT_EDC_ENCODE_LRC, false},
	{SC_DEFAULT_TIME_OUT, SC_TIMER_UNIT_MSEC},
	false,
	false,
	/* Debounce info for IF_CMD_2 */
	{true,  true, SC_DEFAULT_DB_WIDTH},
	true,
	{0, SC_TIMER_UNIT_ETU},

	false, /* Is packet in TPDU? */
	{SC_DEFAULT_ETU_CLKDIV, SC_DEFAULT_SC_CLKDIV,
	 SC_DEFAULT_PRESCALE, SC_DEFAULT_BAUD_DIV},
	SC_DEFAULT_IFSC
};

/* Default historical bytes for various card types */
#define COUNTOF(ary)   ((int)(sizeof(ary) / sizeof((ary)[0])))

static const struct sc_card_type_settings sc_def_card_type_settings[] = {
	{SC_CARD_TYPE_JAVA, 5,
		{0xae, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	},

	{SC_CARD_TYPE_JAVA, 13,
		{0x80, 0x31, 0x80, 0x65, 0xb0, 0x83, 0x02,
		 0x04, 0x7e, 0x83, 0x00, 0x90, 0x00, 0x00, 0x00}
	},

	{SC_CARD_TYPE_PKI, 10,
		{0x4a, 0x43, 0x4f, 0x50, 0x34, 0x31, 0x56,
		 0x32, 0x32, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00}
	},

	{SC_CARD_TYPE_ACOS, 3,
	/*
	 *  T1    T2    T3    T4    T5    T6    T7    T8    T9   T10   T11
	 * ACOS |Ver. |Rev. |Option Registers|Personalization File bytes |
	 *
	 *  T12   T13   T14   T15
	 * Stage | --  | --  | N/A
	 */
		{0x41, 0x01, 0x38, 0x01, 0x00, 0x03, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x02, 0x90, 0x00, 0x00}
	}
};

/**
 * @brief Set Block Wait Time Extension
 *
 * @param handle Channel handle
 * @param bwt_ext ext value
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_bwt_ext_set(struct ch_handle *handle, u32_t bwt_ext)
{
	handle->ch_param.blk_wait_time_ext.val = bwt_ext;

	return 0;
}

/**
 * @brief Enable interrupts
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_enable_interrupts(struct ch_handle *handle)
{
	s32_t rv = 0;

	cit_sc_enter_critical_section(handle);
	rv = cit_channel_enable_intrs(handle);
	cit_sc_exit_critical_section(handle);

	return rv;
}

/**
 * @brief Transceive the tpdu
 *
 * @param handle Channel handle
 * @param tx_rx sc_transceive structure pointer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_tpdu_transceive(struct ch_handle *handle, struct sc_transceive *tx_rx)
{
	struct sc_apdu apdu;
	u8_t *data = tx_rx->tx;
	u32_t tx_len = tx_rx->tx_len;
	u8_t ack, val;
	u8_t pcb, resync = 0, wtx = 0;
	u8_t buf[SC_T1_MAX_BUF_SIZE];
	u32_t rx_len;
	u32_t len = 0;
	u32_t zero_256 = 0; /* flag to indicate if this is Le=0 => 256 case */
	struct sc_time time = {SC_DEFAULT_BLOCK_WAITING_TIME,
							 SC_TIMER_UNIT_ETU};
	u16_t cse;
	s32_t rv = 0;

#ifdef SC_DEBUG
	u32_t count;

	printk("BSCD %s Tx %d bytes: ", __func__, tx_rx->tx_len);
	for (count = 0; count < tx_rx->tx_len; count++)
		printk("%2x ", tx_rx->tx[count]);
	printk("\n");
#endif
	SMART_CARD_FUNCTION_ENTER();
	tx_rx->rx_len = 0;
	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) {
		/* Parse APDU command buffer */
		memset(&apdu, 0, sizeof(struct sc_apdu));
		apdu.data = tx_rx->rx;
		apdu.cla = *data++;
		apdu.ins = *data++;
		apdu.p1 = *data++;
		apdu.p2 = *data++;

		/* Check the APDU command cases */
		if (tx_len < 5) { /* case 1: Header only */
			cse = 1;
		} else if (tx_len == 5) { /* case 2: Header + Le */
			cse = 2;
			apdu.le = tx_rx->tx[4];
			if (apdu.le == 0)
				/* jTOP PIV T=0 card sends "0 c0 0 0 0", we will
				 * fail this frame
				 */
				zero_256 = 1;
		} else {
			apdu.lc = tx_rx->tx[4];
			if (tx_len == (u32_t)(apdu.lc + 5)) {
				 /* case 3: Header + Lc + data */
				cse = 3;
			} else if (tx_len == (u32_t)(apdu.lc + 6)) {
				 /* case 4: Header + Lc + data + Le */
				cse = 4;
				apdu.le = tx_rx->tx[tx_len - 1];
				if (apdu.le == 0)
					zero_256 = 1;
			} else {
				SYS_LOG_ERR("INVALID tx length %d\n", tx_len);
				rv = -EINVAL; /* invalid APDU command */
				goto done;
			}
		}

		/* Check if receive buffer is big enough */
		if (tx_rx->max_rx_len < (u32_t)(apdu.le + 2)) {
			SYS_LOG_ERR("Insufficient Rx buffer");
			rv = -EINVAL;
			goto done;
		}

		/* clear return data (le bytes) */
		memset(apdu.data, 0, apdu.le);

		/* Send 5-byte command header */
		if (cse == 1)
			memset(buf, 0, 5);

		if (tx_rx->tx_len == SC_TPDU_MIN_HEADER_SIZE)
			memcpy(buf, tx_rx->tx, SC_TPDU_MIN_HEADER_SIZE);
		else
			memcpy(buf, tx_rx->tx, SC_TPDU_HEADER_WITH_P3_SIZE);

		rv = cit_ch_transmit(handle, buf, SC_TPDU_HEADER_WITH_P3_SIZE);
		if (rv)
			goto done;

		while (handle->ch_status.card_present == true) {
			rv = cit_ch_receive(handle, &ack, &rx_len, 1);
			if (rv)
				goto done;

			/* If receive null value '60' - wait a little */
			if (ack == 0x60) {
				SYS_LOG_DBG("Received 0x60");
#if DELL
				ccidctSendWTXIndication();
#endif
				continue;
			}

			/* Check if it's SW1 || SW2 */
			if ((ack & 0xf0) == 0x60 || (ack & 0xf0) == 0x90) {
				apdu.sw[0] = ack;
				rv = cit_ch_receive(handle, &ack, &rx_len, 1);
				if (rv)
					goto done;
				apdu.sw[1] = ack;
				memcpy(apdu.data + len, apdu.sw, 2);
				len += 2;
				break;
			}

			/* Receive ACK (PB) - go ahead and send data */
			if ((ack == apdu.ins) && (apdu.lc)) {
				SYS_LOG_DBG("Received ack");
				rv = cit_ch_transmit(handle, ++data, apdu.lc);
				if (rv)
					goto done;

				continue;
			}

			/* Receive ~ACK, send single byte only */
			val = ~apdu.ins;
			if ((ack == val) && (apdu.lc)) {
				SYS_LOG_DBG("Received ~ack");
				rv = cit_ch_transmit(handle, ++data, 1);
				if (rv)
					goto done;
				apdu.lc--;
				continue;
			}

			/* 0 means 256 for this case */
			if (zero_256 && (apdu.le == 0))
				apdu.le = 256;

			/* get ACK and no data body to receive */
			if ((ack == apdu.ins) && (apdu.le == 0))
				continue;

			SYS_LOG_DBG("Ready to receive %d bytes", apdu.le);
			rv = cit_ch_receive(handle, (apdu.data + len), &rx_len,
								 apdu.le);
			len += rx_len;
		}
		tx_rx->rx_len = len;
	} else if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
		if (handle->ch_param.btpdu == true) {
			/* fixme: not really in APDU format */
			pcb = tx_rx->tx[SC_T1_OFFSET_PCB];
			len = tx_rx->tx[SC_T1_OFFSET_LEN];
			memcpy(buf, tx_rx->tx, len + SC_TPDU_MIN_HEADER_SIZE);

			if ((pcb & SC_T1_BLK_MASK) == SC_T1_S_BLK) {
				switch (pcb) {
				case SC_T1_S_WTX_REQ:
				case SC_T1_S_WTX_REP:
					wtx = tx_rx->tx[SC_T1_OFFSET_INF];
					time.val = wtx *
					 handle->ch_param.blk_wait_time.val;
					cit_channel_bwt_ext_set(handle,
								 time.val);
				break;

				default:
					break;
				}
			}

			rv = cit_ch_transmit(handle, buf,
					 (len + SC_TPDU_MIN_HEADER_SIZE));
			if (rv)
				goto error;
			cit_ch_receive(handle, buf, &rx_len,
							 SC_T1_MAX_BUF_SIZE);
			if (rx_len == 0) {
				cit_sc_delay(1);
				rv = cit_ch_receive(handle,  buf, &rx_len,
							SC_T1_MAX_BUF_SIZE);
				if ((rv == -ENOMSG) && (rx_len == 0))
					goto error;
			}
			tx_rx->rx_len = rx_len;
			memcpy(tx_rx->rx, buf, rx_len);
			return rv;
		}

		/* Start with an I-bolck */
		pcb = SC_T1_I_BLK;

		/* Check if more I blocks needed */
		len = tx_rx->tx_len;
		if (len > handle->ch_param.ifsd) {
			pcb |= SC_T1_I_BLK_MORE;
			len = handle->ch_param.ifsd;
		}

		if ((pcb & SC_T1_BLK_MASK) == SC_T1_I_BLK)
			pcb |= handle->ns << SC_T1_I_BLK_N_SHIFT;

		/* Send the first I-block */
		buf[SC_T1_OFFSET_NAD] = 0; /* NAD byte always = 0 */
		buf[SC_T1_OFFSET_PCB] = pcb;
		buf[SC_T1_OFFSET_LEN] = tx_rx->tx_len;
		memcpy((buf + SC_T1_OFFSET_INF), tx_rx->tx, len);

		rv = cit_ch_transmit(handle, buf, (len + SC_T1_OFFSET_INF));
		if (rv)
			goto error;

		handle->state = SC_T1_TX;
		while (1) {
			/* Get response from ICC */
			rv = cit_ch_receive(handle, buf, &len,
							 SC_T1_MAX_BUF_SIZE);
			if (rv)
				goto error;

			/* Process returned block from ICC */
			pcb = buf[SC_T1_OFFSET_PCB];
			switch (pcb & SC_T1_BLK_MASK) {
			case SC_T1_R_BLK:
				if (SC_T1_R_IS_ERROR(pcb))
					goto resync_lable;

				if (handle->state == SC_T1_RX) {
					buf[SC_T1_OFFSET_NAD] = 0;
					buf[SC_T1_OFFSET_PCB] = SC_T1_R_BLK;
					buf[SC_T1_OFFSET_LEN] = 0;
					rv = cit_ch_transmit(handle, buf,
							 SC_T1_OFFSET_INF);
					if (rv)
						goto error;
					break;
				}

				if (SC_T1_R_BLK_N(pcb) != handle->ns)
					handle->ns ^= 1;
				break;

			case SC_T1_S_BLK:
				if ((pcb & SC_T1_S_RESYNC_REP) &&
				    (handle->state == SC_T1_RESYNC)) {
					handle->state = SC_T1_TX;
					resync = 0;

					/* Resend the I-block */
					buf[0] = 0;
					buf[1] = pcb;
					buf[2] = tx_rx->tx_len;
					memcpy(buf + SC_T1_OFFSET_INF,
								tx_rx->tx, len);
					rv = cit_ch_transmit(handle, buf,
						(len + SC_T1_OFFSET_INF));
					if (rv)
						goto error;
					continue;
				}

				/* Check S-block message type */
				switch (pcb) {
				case SC_T1_S_IFS_REQ:
				case SC_T1_S_IFS_REP:
				case SC_T1_S_RESYNC_REQ:
				case SC_T1_S_RESYNC_REP:
				case SC_T1_S_ABORT_REQ:
				case SC_T1_S_ABORT_REP:
				case SC_T1_S_WTX_REQ:
				case SC_T1_S_WTX_REP:
					goto resync_lable;
				default:
					break;
				}

				buf[0] = 0;
				buf[1] = SC_T1_S_BLK | SC_T1_S_RESPONSE |
							((pcb) & 0x0F);
				buf[2] = 0;
				rv = cit_ch_transmit(handle, buf,
							 SC_T1_OFFSET_INF);
				if (rv)
					goto error;
				break;

			case SC_T1_I_BLK:
			default:
				if (handle->state == SC_T1_TX)
					handle->ns ^= 1; /* update sequence */

				handle->state = SC_T1_RX;

				/* Check if receive sequence number matched */
				if (SC_T1_I_BLK_N(pcb) != handle->nr) {
					/* Send R-block to signal error */
					buf[0] = 0;
					buf[1] = SC_T1_R_BLK |
							 SC_T1_R_BLK_ERR_OTHER;
					buf[2] = 0;
					rv = cit_ch_transmit(handle, buf,
							 SC_T1_OFFSET_INF);
					if (rv)
						goto error;
					continue;
				}
				handle->nr ^= 1;

				if ((pcb & SC_T1_I_BLK_MORE) == 0) {
					/* only one I-block */
					memcpy(tx_rx->rx, buf + 3, buf[2]);
					tx_rx->rx_len = buf[2];
					goto done;
				}

				/* Send R-block */
				buf[0] = 0; /* NAD byte always = 0 */
				buf[1] = SC_T1_R_BLK;
				buf[2] = 0;
				rv = cit_ch_transmit(handle, buf,
							 SC_T1_OFFSET_INF);
				if (rv)
					goto error;
				break;
			}
resync_lable:
			if (resync) {
				rv = -ETIMEDOUT;
				goto error;
			}

			handle->ns = 0;
			handle->nr = 0;
			resync = 1;
			handle->state = SC_T1_RESYNC;
			buf[0] = 0; /* NAD byte always = 0 */
			buf[1] = SC_T1_S_RESYNC_REQ;
			buf[2] = 0;
			rv = cit_ch_transmit(handle, buf, SC_T1_OFFSET_INF);
				if (rv)
					goto error;
			continue;
		} /* while */
	} else {
		rv = -ENOTSUP;
	}
error:
done:
#ifdef SC_DEBUG
	printk("BSCD Rx %d bytes: ", tx_rx->rx_len);
	for (count = 0; count < tx_rx->tx_len; count++)
		printk("%2x ", tx_rx->rx[count]);
	printk("\n");
#endif
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Get the default channel parameters
 *
 * @param channel Channel number
 * @param ch_param Channel parameters struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_get_default_channel_settings(u32_t channel,
				       struct sc_channel_param *ch_param)
{
	ARG_UNUSED(channel);
	SMART_CARD_FUNCTION_ENTER();
	*ch_param = default_channel_settings;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

s32_t cit_channel_open(struct ch_handle *handle,
		       struct sc_channel_param *ch_param)
{
	u8_t vcc_level;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (handle->open == true)
		return -EALREADY;

	handle->clock.external = CONFIG_SC_CH1_CLOCK_SOURCE;
	handle->clock.clk_frequency = CONFIG_SC_CH1_CLOCK_FREQ;
#ifdef BSCD_EMV2000_CWT_PLUS_4
	handle->receive = false;
#endif
	cit_sc_enter_critical_section(handle);
	sys_write32(0, (handle->base + CIT_INTR_EN_1));
	sys_write32(0, (handle->base + CIT_INTR_EN_2));

	handle->status1 = 0x00;
	handle->status2 = 0x00;
	handle->intr_status1 = 0x00;
	handle->intr_status2 = 0x00;
	cit_sc_exit_critical_section(handle);

	if (ch_param != NULL)
		rv = cit_channel_set_parameters(handle, ch_param);
	else
		rv = cit_channel_set_parameters(handle,
					 &default_channel_settings);
	if (rv)
		SYS_LOG_INF("Smart card param set failed %d\n", rv);
	/* Todo, Only enable intr for ATR */
	cit_channel_enable_interrupts(handle);

	/*08/23/06, Set default VCC level is 5V*/
	if (ch_param != NULL)
		vcc_level = ch_param->ctx_card_type.vcc_level;
	else
		vcc_level = default_channel_settings.ctx_card_type.vcc_level;

	cit_channel_set_vcclevel(handle, vcc_level);

	cit_sc_enter_critical_section(handle);
	handle->open = true;
	cit_sc_exit_critical_section(handle);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

s32_t cit_channel_close(struct ch_handle *handle)
{
	SMART_CARD_FUNCTION_ENTER();
	if (handle->open == false)
		return -EALREADY;

	handle->open = false;

	cit_channel_deactivate(handle);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Detect the card based on given status
 *
 * @param handle Channel handle
 * @param status Inserted or removed
 * @param block True if need to block for a given status
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_detect_card(struct ch_handle *handle,
			      enum sc_card_present status, bool block)
{
	u32_t base = handle->base;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (handle->open == false)
		return -EINVAL;

	cit_sc_enter_critical_section(handle);
	handle->status1 = sys_read32(base + CIT_STATUS_1);
	cit_sc_exit_critical_section(handle);

	switch (status) {
	case SC_CARD_INSERTED:
		cit_sc_enter_critical_section(handle);
		if (handle->status1 & BIT(CIT_STATUS_1_CARD_PRES_BIT)) {
			handle->ch_status.card_present = true;
			cit_sc_exit_critical_section(handle);
			goto done;
		} else {
			rv = -EINVAL;
			SYS_LOG_DBG("SmartCard Not Present");
		}
		cit_sc_exit_critical_section(handle);

		if (block)
			rv = cit_ch_wait_cardinsertion(handle);
		break;

	case SC_CARD_REMOVED:
		{
			cit_sc_enter_critical_section(handle);
			if (!(handle->status1 &
					 BIT(CIT_STATUS_1_CARD_PRES_BIT))) {
				handle->ch_status.card_present = false;
				cit_sc_exit_critical_section(handle);
				goto done;
			} else {
				rv = -EINVAL;
				SYS_LOG_DBG("SmartCard Present");
			}
			cit_sc_exit_critical_section(handle);

			if (block)
				rv = cit_ch_wait_cardremove(handle);
		}
		break;
	default:
		rv = -EINVAL;
		break;
	}
done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;

}

/**
 * @brief Card detect callback set
 *
 * @param handle Channel handle
 * @param status Inserted or removed
 * @param cb callback function
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_detect_cb_set(struct ch_handle *handle,
				enum sc_card_present status,
				sc_api_card_detect_cb cb)
{
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	switch (status) {
	case SC_CARD_INSERTED:
		rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_INSERT, cb);
		break;
	case SC_CARD_REMOVED:
		rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_REMOVE, cb);
		break;
	default:
		rv = -EINVAL;
		break;
	}

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Modify the current smart card channel settings.
 *
 * @param handle Channel handle
 * @param new Channel parameters struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_parameters(struct ch_handle *handle,
				 struct sc_channel_param *new)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	struct host_ctrl_settings *host_ctrl = &handle->ch_param.host_ctrl;
	u32_t bauddiv_encode;
	u32_t sc_clkdiv_encode;
	u32_t base = handle->base;
	u32_t val, cnt;
	s32_t rv;

	SMART_CARD_FUNCTION_ENTER();
	channel_dump_param(new);
	/*  Smart Card Standard */
	if ((new->standard <= SC_STANDARD_UNKNOWN) ||
		(new->standard > SC_STANDARD_ES))
		return -ENOTSUP;

	ch_param->standard = new->standard;

	rv = cit_channel_set_standard(handle, new);
	if (rv)
		return rv;

	rv = cit_channel_set_frequency(handle, new);
	if (rv)
		return rv;

	/* Set Vcc level used */
	ch_param->ctx_card_type.vcc_level = new->ctx_card_type.vcc_level;
	ch_param->ctx_card_type.inited = new->ctx_card_type.inited;

	/* Set maximum IFSD */
	ch_param->max_ifsd = SC_MAX_TX_SIZE;

	/* Set current IFSD */
	if (new->ifsd > SC_MAX_TX_SIZE)
		return -ENOTSUP;

	if (new->max_ifsd == 0)
		ch_param->ifsd = SC_MAX_TX_SIZE;
	else
		ch_param->ifsd = new->ifsd;

	/* Set current IFSC */
	ch_param->ifsc =  new->ifsc;

	rv = cit_channel_set_edc_parity(handle, new);
	if (rv)
		return rv;

	rv = cit_channel_set_wait_time(handle, new);
	if (rv)
		return rv;

	rv = cit_channel_set_guard_time(handle, new);
	if (rv)
		return rv;

	rv = cit_channel_set_transaction_time(handle, new);
	if (rv)
		return rv;

	/* auto deactivation sequence */
	ch_param->auto_deactive_req = new->auto_deactive_req;

	/* nullFilter */
	ch_param->null_filter =  new->null_filter;

	/* debounce info */
	if (new->debounce.db_width > SC_MAX_DB_WIDTH)
		return -ENOTSUP;

	ch_param->debounce = new->debounce;

	/* Specify if the driver to read, decode and program registers */
	ch_param->read_atr_on_reset = new->read_atr_on_reset;

	/* Update the PRESCALE */
	sys_write32(host_ctrl->prescale, (base + CIT_PRESCALE));

	/* Get the clock encode vals */
	for (cnt = 0; cnt < SC_MAX_BAUDDIV; cnt++) {
		if (host_ctrl->bauddiv == bauddiv_valid[cnt])
			break;
	}
	if (cnt == SC_MAX_BAUDDIV)
		return -EINVAL;

	bauddiv_encode = cnt;
	sc_clkdiv_encode = cit_clkdiv[host_ctrl->sc_clkdiv];
	/*
	 * Don't enable clock here since auto_clk need to be set first in
	 * ResetIFD before clock enabling for auto_deactivation
	 */
	val = sys_read32(base + CIT_CLK_CMD_1) & BIT(CIT_CLK_CMD_1_CLK_EN_BIT);

	/* If enabled before, change the the clock values */
	if (val) {
		/* program CIT_CLK_CMD_1 */
		val |= (host_ctrl->etu_clkdiv - 1) <<
					CIT_CLK_CMD_1_ETU_CLKDIV_SHIFT;
		val |= (sc_clkdiv_encode << CIT_CLK_CMD_1_SC_CLKDIV_SHIFT) &
					CIT_CLK_CMD_1_SC_CLKDIV_MASK;
		if (bauddiv_encode & CIT_BAUDDIV_BIT_0)
			val |= BIT(CIT_CLK_CMD_1_BAUDDIV_0_BIT);

		sys_write32(val, base + CIT_CLK_CMD_1);

		/* program CIT_CLK_CMD_2 */
		val = 0;
		if (sc_clkdiv_encode & CIT_SC_CLKDIV_BIT_3)
			val |= BIT(CIT_CLK_CMD_2_SC_CLKDIV_3);

		if (bauddiv_encode & CIT_BAUDDIV_BIT_2)
			val |= BIT(CIT_CLK_CMD_2_BAUDDIV_2_BIT);

		val |= CIT_DEFAULT_IF_CLKDIV << CIT_CLK_CMD_2_IF_CLKDIV_SHIFT;
		sys_write32(val, base + CIT_CLK_CMD_2);

		if (bauddiv_encode & CIT_BAUDDIV_BIT_1)
			sys_set_bit((base + CIT_FLOW_CMD),
						 CIT_FLOW_CMD_BAUDDIV_1_BIT);
		else
			sys_clear_bit((base + CIT_FLOW_CMD),
						 CIT_FLOW_CMD_BAUDDIV_1_BIT);
	}

	/* Update the SC_P_UART_CMD_2 */
	val = sys_read32(base + CIT_UART_CMD_2);
	val &=  BIT(CIT_UART_CMD_2_CONVENTION_BIT);

	if (new->protocol == SC_ASYNC_PROTOCOL_E0)
		val |= (ch_param->rx_retries <<
				 CIT_UART_CMD_2_RPAR_RETRY_SHIFT) |
			handle->ch_param.tx_retries;

	sys_write32(val, base + CIT_UART_CMD_2);

	/* Update the BSCD_P_PROTO_CMD */
	val = sys_read32(base + CIT_PROTO_CMD);
	if ((new->protocol == SC_ASYNC_PROTOCOL_E1) &&
	    (handle->ch_param.edc.enable)) {
		val |= BIT(CIT_PROTO_CMD_EDC_EN_BIT);
		if (handle->ch_param.edc.crc)
			val |= BIT(CIT_PROTO_CMD_CRC_BIT);
		else
			val &= ~BIT(CIT_PROTO_CMD_CRC_BIT);
	} else {
		val &= ~BIT(CIT_PROTO_CMD_EDC_EN_BIT);
	}

	val |= ch_param->cwi & CIT_PROTO_CMD_CWI_MASK;
	sys_write32(val, base + CIT_PROTO_CMD);

	/* Update the BSCD_P_FLOW_CMD */
	val = 0;
	if (handle->ch_param.standard == SC_STANDARD_NDS)
		sys_set_bit(base + CIT_FLOW_CMD, CIT_FLOW_CMD_FLOW_EN_BIT);

	/* Update the BSCD_P_IF_CMD_2 */
	val = 0;

	if (handle->ch_param.debounce.enable == true)
		val |= BIT(CIT_IF_CMD_2_DB_EN_BIT);

	if (handle->ch_param.debounce.sc_mask == true)
		val |= BIT(CIT_IF_CMD_2_DB_MASK_BIT);

	val |= handle->ch_param.debounce.db_width;
	sys_write32(val, base + CIT_IF_CMD_2);
	sys_write32(ch_param->extra_guard_time.val, base + CIT_TGUARD);

#if defined(SCI_DIAG_ALL)
	/* for T=1 we always use TPDU, e.g. we do not build T=1 APDU */
	handle->ch_param.btpdu = false;
#else
	/* for T=1 we always use TPDU, e.g. we do not build T=1 APDU */
	handle->ch_param.btpdu = true;
#endif
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Get the current smart card channel settings.
 *
 * @param handle Channel handle
 * @param ch_param Channel parameters struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_get_parameters(struct ch_handle *handle,
				 struct sc_channel_param *ch_param)
{
	SMART_CARD_FUNCTION_ENTER();
	*ch_param = handle->ch_param;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Get the negotiated smart card channel settings.
 *
 * @param handle Channel handle
 * @param ch_neg_param Channel parameters struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_get_nego_parameters(struct ch_handle *handle,
				      struct sc_channel_param *ch_neg_param)
{
	SMART_CARD_FUNCTION_ENTER();
	*ch_neg_param = handle->ch_neg_param;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Deativate the channel
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_deactivate(struct ch_handle *handle)
{
	u32_t base = handle->base;

	SMART_CARD_FUNCTION_ENTER();
	/* Disable all interrupts */
	sys_write32(0, (base + CIT_INTR_EN_1));
	sys_write32(0, (base + CIT_INTR_EN_2));

	/* Turn off VCC */
	sys_set_bit((base + CIT_IF_CMD_1), CIT_IF_CMD_1_VCC_BIT);

#ifndef CUST_X7_PLAT
	/*
	 * Dell X7 use 8034, will do auto-deactivation; don't need to pull down
	 * rst i/o and clk signal
	 */

	/* Set RST = 0.     */
	sys_clear_bit((base + CIT_IF_CMD_1), CIT_IF_CMD_1_RST_BIT);

	/* Set CLK = 0.      */
	sys_write32(0, (base + CIT_CLK_CMD_1));

	/* Set IO = 0.      */
	sys_set_bit((base + CIT_IF_CMD_1), CIT_IF_CMD_1_IO_BIT);
#endif

	/* Reset Tx & Rx buffers.   */
	sys_write32(~BIT(CIT_UART_CMD_1_IO_EN_BIT), (base + CIT_UART_CMD_1));
	sys_write32(BIT(CIT_PROTO_CMD_TBUF_RST_BIT) |
		    BIT(CIT_PROTO_CMD_RBUF_RST_BIT),
						(base + CIT_UART_CMD_1));

	if ((handle->ch_param.protocol == SC_SYNC_PROTOCOL_E0) ||
	    (handle->ch_param.protocol == SC_SYNC_PROTOCOL_E1))
		sys_set_bit((handle->base + CIT_IF_CMD_3),
						 CIT_IF_CMD_3_DEACT_BIT);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Reset a specific smart card interface. It shall not
 *        reset the smart card (ICC)
 *
 * @param handle Channel handle
 * @param reset_type Cold reset or warm reset
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_reset_ifd(struct ch_handle *handle,
			    enum sc_reset_type reset_type)
{
	u32_t base = handle->base;
	u32_t val;
	s32_t rv;
	struct cit_timer timer = {CIT_GP_TIMER, CIT_GP_TIMER_IMMEDIATE,
								true, true};
	struct sc_time time = {2, SC_TIMER_UNIT_ETU};

	SMART_CARD_FUNCTION_ENTER();
	/*
	 * cold reset will generete remnoval/insertion interrupt unexpectly,
	 * disable the removel intterrupt as a SW workaround.
	 */
	if (reset_type == SC_COLD_RESET) {
		SYS_LOG_DBG("Cold reset");
		rv = cit_ch_interrupt_disable(handle, CIT_INTR_PRES_REMOVE);
		if (rv)
			goto done;
	}
	/* Reset all status */
	handle->status1 = 0;
	handle->status2 = 0;
	handle->intr_status1 = 0;
	handle->intr_status2 = 0;

	handle->ch_status.status1 = 0;

	if (reset_type == SC_COLD_RESET) {
		handle->ch_status.card_present = false;
		/*
		 * 09/20/05,Allen.C, reset bIsCardRemoved after card
		 * removed and reinitialize
		 */
		handle->card_removed = false;
	}
	/* Reset some critical registers */
	sys_write32(0, (base + CIT_TIMER_CMD));
	sys_write32(0, (base + CIT_INTR_EN_1));
	sys_write32(0, (base + CIT_INTR_EN_2));
	sys_write32(0, (base + CIT_UART_CMD_1));
	sys_write32(0, (base + CIT_UART_CMD_2));

	/* Set up debounce filter */
	val = 0;
	if (handle->ch_param.debounce.enable == true) {
		val |= BIT(CIT_IF_CMD_2_DB_EN_BIT);
		if (handle->ch_param.debounce.sc_mask)
			val |= BIT(CIT_IF_CMD_2_DB_MASK_BIT);
		val |= handle->ch_param.debounce.db_width;
	}
	sys_write32(val, (base + CIT_IF_CMD_2));

	/* Cold Reset or Warm Reset */
	if (reset_type == SC_COLD_RESET) {
		SYS_LOG_DBG("Cold Reset");
		handle->reset_type = SC_COLD_RESET;
		val = BIT(CIT_IF_CMD_1_VCC_BIT);
		sys_write32(val, (base + CIT_IF_CMD_1));

		val |= BIT(CIT_IF_CMD_1_PRES_POL_BIT);
		sys_write32(val, (base + CIT_IF_CMD_1));
	} else {
		SYS_LOG_DBG("Warm Reset");
		handle->reset_type = SC_WARM_RESET;
		val = sys_read32(base + CIT_IF_CMD_1);
	}

	/* Use Auto Deactivation instead of TDA8004 */
	if (handle->ch_param.auto_deactive_req == true) {
		val |= BIT(CIT_IF_CMD_1_AUTO_CLK_BIT);
		sys_write32(val, (base + CIT_IF_CMD_1));
	}

	/* Set Clk cmd */
	cit_program_clocks(handle);

	/* Use Auto Deactivation instead of TDA8004 */
	if (handle->ch_param.auto_deactive_req == true) {
		val |= BIT(CIT_IF_CMD_1_AUTO_IO_BIT);
		sys_write32(val, (base + CIT_IF_CMD_1));
	}

	val |= BIT(CIT_IF_CMD_1_RST_BIT);
	sys_write32(val, (base + CIT_IF_CMD_1));

	sys_write32(0, (base + CIT_UART_CMD_1));

	/* Enable 2 interrupts with callback */
	SYS_LOG_DBG("Enable Insert/Remove interrupts");
	rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_INSERT,
						cit_ch_cardinsert_cb);
	if (rv)
		goto done;

	rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_REMOVE,
						cit_ch_cardremove_cb);
	if (rv)
		goto done;

	sys_write32(BIT(CIT_UART_CMD_1_UART_RST_BIT),
				 (handle->base + CIT_UART_CMD_1));

	/* UART Reset should be set within 1 ETU (however, we will give 2 etus*/
	rv = cit_config_timer(handle, &timer, &time);
	if (rv)
		goto done;

	rv = cit_wait_timer_event(handle);
	if (rv)
		goto done;

	/* Disable timer */
	timer.enable = false;
	timer.interrupt_enable = false;
	rv = cit_timer_isr_control(handle, &timer);
	if (rv)
		goto done;
	val = sys_read32(base + CIT_UART_CMD_1);

	/* If equal to zero, then UART reset has gone low, so return success */
	if ((val & BIT(CIT_UART_CMD_1_UART_RST_BIT)) == 0) {
		SYS_LOG_DBG("Reset Success\n");

		/*
		 *   INITIAL_CWI_SC_PROTO_CMD = 0x0f is required so that
		 *   CWI does not remain equal to zero, which causes an
		 *   erroneous timeout, the CWI is set correctly in the
		 *   SmartCardEMVATRDecode procedure
		 */
		sys_write32(BIT(CIT_PROTO_CMD_RBUF_RST_BIT) |
			    BIT(CIT_PROTO_CMD_TBUF_RST_BIT),
					(base + CIT_PROTO_CMD));
	}

	if ((handle->ch_param.protocol == SC_SYNC_PROTOCOL_E0) ||
	    (handle->ch_param.protocol == SC_SYNC_PROTOCOL_E1)) {
		if (handle->ch_param.protocol == SC_SYNC_PROTOCOL_E1) {
			sys_clear_bit((base + CIT_IF_CMD_3),
					BIT(CIT_IF_CMD_3_SYNC_TYPE1));
			sys_set_bit((base + CIT_IF_CMD_3),
					BIT(CIT_IF_CMD_3_SYNC_TYPE2));
		} else {
			sys_clear_bit((base + CIT_IF_CMD_3),
					BIT(CIT_IF_CMD_3_SYNC_TYPE2));
			sys_set_bit((base + CIT_IF_CMD_3),
					BIT(CIT_IF_CMD_3_SYNC_TYPE1));
		}
	}

done:
	if (reset_type == SC_COLD_RESET)
		rv = cit_ch_interrupt_enable(handle, CIT_INTR_PRES_REMOVE,
						cit_ch_cardremove_cb);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Power up the card
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_card_power_up(struct ch_handle *handle)
{
	SMART_CARD_FUNCTION_ENTER();
	SYS_LOG_DBG("Card power up");
	cit_channel_set_vcclevel(handle,
				 handle->ch_param.ctx_card_type.vcc_level);
	sys_clear_bit((handle->base + CIT_IF_CMD_1), CIT_IF_CMD_1_VCC_BIT);

	handle->ch_status.card_activate = false;
	handle->ch_status.pps_done = false;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Power down the card
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_card_power_down(struct ch_handle *handle)
{
	SMART_CARD_FUNCTION_ENTER();
	SYS_LOG_DBG("Card power down");
	sys_set_bit((handle->base + CIT_IF_CMD_1), CIT_IF_CMD_1_VCC_BIT);

	handle->ch_status.card_activate = false;
	handle->ch_status.pps_done = false;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Set Vcc level
 *
 * @param handle Channel handle
 * @param vcc_level vcc level
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_vcclevel(struct ch_handle *handle, u8_t vcc_level)
{
	u32_t base = handle->base;
	s32_t ret = 0;

	SMART_CARD_FUNCTION_ENTER();
	cit_sc_enter_critical_section(handle);

	switch (vcc_level) {
	case SC_VCC_LEVEL_3V:
		sys_clear_bit((base + CIT_IF_CMD_3), CIT_IF_CMD_3_VPP_BIT);
		break;

	case SC_VCC_LEVEL_5V:
		sys_set_bit((base + CIT_IF_CMD_3), CIT_IF_CMD_3_VPP_BIT);
		break;

	case SC_VCC_LEVEL_1_8V:
	default:
		ret = -ENOTSUP;
		break;
	}

	cit_sc_exit_critical_section(handle);
	SYS_LOG_DBG("Set Vcc level: %d rv:%d", vcc_level, ret);
	SMART_CARD_FUNCTION_EXIT();
	return ret;

}

/**
 * @brief Reset the card
 *
 * @param handle Channel handle
 * @param decode_atr Read and interpret the ATR data and program the
 *                   registers accordingly
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_card_reset(struct ch_handle *handle, u8_t decode_atr)
{
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (!decode_atr) {
		cit_channel_activating(handle);
	} else {
		rv = cit_channel_activating(handle);
		if (rv) {
			SYS_LOG_ERR("Activating fail %d", rv);
			return rv;
		}

		rv = cit_channel_receive_and_decode(handle);
		if (rv) {
			SYS_LOG_ERR("Receive and decode fail %d", rv);
			return rv;
		}
	}

	/* Clear internal states */
	memset(&(handle->tpdu_t1), 0, sizeof(struct sc_tpdu_t1));

	SYS_LOG_DBG("Reset card rv:%d", rv);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Perform PPS transaction
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_pps(struct ch_handle *handle)
{
	s32_t rv = 0;
	u8_t tx_buf[4]; /* size of minimum PPS packet */
	u8_t rx_buf[4] = {0}; /* size of minimum PPS packet */
	u32_t ret_len = 0;

	SMART_CARD_FUNCTION_ENTER();
	if (handle->pps_needed == false) {
		SYS_LOG_DBG("PPS not needed");
		handle->ch_status.pps_done = true;
		return rv;
	}

	{ /* ACOS5 32G card workaroud: skip PPS */
		s8_t st_acos5_32g_atr[] = {
			0x3B, 0xBE, 0x18, 0x0, 0x0, 0x41, 0x5, 0x10, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x90, 0x0
		};

		if (handle->rx_len == sizeof(st_acos5_32g_atr))	{
			if (memcmp(handle->auc_rx_buf, st_acos5_32g_atr,
					 sizeof(st_acos5_32g_atr)) == 0) {
				SYS_LOG_INF("This is a ACOS 32G card.\n");
				rv = -ECONNREFUSED;
				goto done;
			}
		}
	}

	/* assign PPS */
	tx_buf[0] = 0xFF; /* PPSS */
	/* PPS0: PPS1 exist, plus T */
	tx_buf[1] = 0x10 | (handle->ch_neg_param.protocol ==
						SC_ASYNC_PROTOCOL_E1 ? 1 : 0);
	tx_buf[2] = (handle->ch_neg_param.f_factor << 4) |
		    (handle->ch_neg_param.d_factor & 0x0f);
	tx_buf[3] = (tx_buf[0] ^ tx_buf[1] ^ tx_buf[2]); /* PCK */

	/* send and receive PPS */
	rv = cit_ch_transmit(handle, tx_buf, sizeof(tx_buf));
	if (rv == 0)
		rv = cit_ch_receive(handle, rx_buf, &ret_len, sizeof(rx_buf));

	handle->ch_status.pps_done = true;

	if (rv)	{
		s8_t st_set_cos43atr[]  = {
			0x3B, 0x9F, 0x94, 0x40, 0x1E, 0x00, 0x67, 0x11, 0x43,
			0x46, 0x49, 0x53, 0x45, 0x10, 0x52, 0x66, 0xFF, 0x81,
			0x90, 0x00
		};
		s8_t st_tw_citizen_atr[] = {
			0x3B, 0xB8, 0x13, 0x00, 0x81, 0x31, 0xFA, 0x52, 0x43,
			0x48, 0x54, 0x4D, 0x4F, 0x49, 0x43, 0x41, 0xA5
		};

		SYS_LOG_INF("ct: PPS tx/rx error");

		if (handle->rx_len == sizeof(st_set_cos43atr)) {
			if (memcmp(handle->auc_rx_buf, st_set_cos43atr,
					 sizeof(st_set_cos43atr)) == 0)	{
				SYS_LOG_INF("This is a SetCos4.3 card.");
				rv = 0;
				goto done;
			}
			if (memcmp(handle->auc_rx_buf, st_tw_citizen_atr,
					sizeof(st_tw_citizen_atr)) == 0) {
				SYS_LOG_INF("This is a taiwan citizen card.");
				rv = 0;
				goto done;
			}
		}
		goto done;
	} else {
		/* check if response is same as request */
		if (memcmp(tx_buf, rx_buf, sizeof(rx_buf))) {
			/*
			 * We treat all non-matching cases as failure, although
			 * the spec says we need to look byte by byte.
			 * From spec:
			 * PPSS should be echoed
			 * PPS0 b1-b4 should be echoed
			 * PPS0 b5 should be either echoed or not
			 * When echoed, PPS1 must echoed;
			 * When not echoed, PPS1(e.g. F/D) should not be used,
			 * we should regard it as PPS fail so our parameter
			 * is not changed
			 * So in all cases, the response must exactly same as
			 * request.
			 */

			SYS_LOG_INF("ct: PPS response error\n");
			rv = -EINVAL;
		}
	}

done:
	SYS_LOG_DBG("PPS rv:%d", rv);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Get the channel status
 *
 * @param handle Channel handle
 * @param status status struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_get_status(struct ch_handle *handle, struct sc_status *status)
{
	*status = handle->ch_status;
	return 0;
}

/**
 * @brief Transmit the data
 *
 * @param handle Channel handle
 * @param tx Transmit buffer
 * @param len Transmit length
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_ch_transmit(struct ch_handle *handle, u8_t *tx, u32_t len)
{
	s32_t rv = 0;

	SYS_LOG_DBG("Trasmit data length: %d", len);
	SMART_CARD_FUNCTION_ENTER();
	rv = cit_channel_enable_interrupts(handle);

	if (handle->ch_param.standard == SC_STANDARD_IRDETO)
		rv = cit_channel_t14_irdeto_transmit(handle, tx, len);
	else if ((handle->ch_param.protocol == SC_SYNC_PROTOCOL_E0) ||
		 (handle->ch_param.protocol == SC_SYNC_PROTOCOL_E1))
		rv = cit_channel_sync_transmit(handle, tx, len);
	else
		rv = cit_channel_t0t1_transmit(handle, tx, len);

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Receive the data
 *
 * @param handle Channel handle
 * @param rx Receive buffer
 * @param rx_len Receive read length
 * @param max_len Max length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_ch_receive(struct ch_handle *handle, u8_t *rx, u32_t *rx_len,
		     u32_t max_len)
{
	s32_t rv = 0;
#ifndef BSCD_DSS_ICAM
	struct cit_timer timer = {CIT_WAIT_TIMER, CIT_WAIT_TIMER_WWT,
				 false, false};
#endif

	SMART_CARD_FUNCTION_ENTER();
	rv = cit_channel_enable_interrupts(handle);
	if (rv)
		return rv;

	*rx_len = 0;

	if ((handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) ||
		(handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E14_IRDETO)) {
		rv = cit_channel_t0_read_data(handle, rx, rx_len, max_len);
		if (rv)
			goto done;

		/*
		 * The Work Wait Timer is enabled in channel_t0t1_transmit.
		 * We cannot disable it in channel_t0_read_data since
		 * channel_t0_read_data is also used by reading ATR,
		 * which is one byte at a time.
		 */

#ifndef BSCD_DSS_ICAM
		/* BSYT leave this WWT enabled. We only disable WWT in tx */
		/*
		 * I assume all standards, other than EMV, will read all the
		 * bytes in BSCD_Channel_P_T0ReadData, therefore we couold
		 * safely disable the WWT here.  EMV only read 1 bytes at a
		 * time, therefore we have to disable WWT in the application
		 */
		/*
		 * fye: this channel receive can be called multiple times
		 * without tx in between, in the case of rx 0x60 only.
		 * this will result the WWT being disabled here for the 2nd
		 * rx and so on
		 */
		if ((handle->ch_param.standard != SC_STANDARD_EMV1996) &&
			(handle->ch_param.standard != SC_STANDARD_EMV2000)) {
			rv = cit_timer_isr_control(handle, &timer);
			if (rv)
				return rv;
		}
#endif
	} else if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
		rv = cit_channel_t1_read_data(handle, rx, rx_len, max_len);
		if (rv)
			goto done;
	} else {
		rv = cit_channel_sync_read_data(handle, rx, rx_len, max_len);
		if (rv)
			goto done;
	}

	if (*rx_len == 0) {
		SYS_LOG_DBG("No Response detected");
		return -EINVAL;
	}
done:
	SYS_LOG_DBG("Receive data length: %d rv: %d", *rx_len, rv);
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Receive the ATR data
 *
 * @param handle Channel handle
 * @param rx Receive buffer
 * @param rx_len Receive read length
 * @param max_len Max length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_receive_atr(struct ch_handle *handle,	u8_t *rx, u32_t *rx_len,
			      u32_t max_len)
{
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	*rx_len = 0;

	if ((handle->ch_param.protocol == SC_SYNC_PROTOCOL_E0) ||
		(handle->ch_param.protocol == SC_SYNC_PROTOCOL_E1)) {
		rv = cit_channel_sync_read_data(handle, rx, rx_len, max_len);
		if (rv)
			goto done;
	} else {
		rv = cit_channel_t0_read_data(handle, rx, rx_len, max_len);
		if (rv)
			goto done;
	}

	if (*rx_len > 0) {

		/*
		 * For T=0, we depend on timeout to
		 * identify that there is no more byte to be received
		 */
		/* Ignore the ReadTimeOut error returned by SmartCardByteRead */
	}

	else {
		SYS_LOG_DBG("No Response detected...deactivating");
		rv = -ENODATA;
	}

done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Configure the timer
 *
 * @param handle Channel handle
 * @param timer Timer type and mode in cit_timer struct
 * @param sc_time Time and unit
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_config_timer(struct ch_handle *handle, struct cit_timer *timer,
		       struct sc_time *time)
{
	u32_t base = handle->base;
	s32_t rv = 0;
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	if (timer->timer_type == CIT_GP_TIMER) {
		if (time->val > USHRT_MAX) {
			SYS_LOG_WRN("Set GP timer more than 16 bits");
			time->val = USHRT_MAX;
		}

		/* disbale timer */
		sys_clear_bit((base + CIT_TIMER_CMD),
						 CIT_TIMER_CMD_TIMER_EN_BIT);

		cit_sc_enter_critical_section(handle);
		handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_TIMER_BIT);
		cit_sc_exit_critical_section(handle);

		/* Set timer_cmp registers */
		val = (time->val >> CHAR_BIT) & UCHAR_MAX;
		sys_write32(val, (base + CIT_TIMER_CMP_2));
		val = time->val & UCHAR_MAX;
		sys_write32(val, (base + CIT_TIMER_CMP_1));

		/* Set the timer unit and mode */
		val = sys_read32(base + CIT_TIMER_CMD);

		if (time->unit == SC_TIMER_UNIT_CLK)
			val |= BIT(CIT_TIMER_CMD_TIMER_SRC);
		else if (time->unit  == SC_TIMER_UNIT_ETU)
			val &= ~BIT(CIT_TIMER_CMD_TIMER_SRC);
		else
			return -EINVAL;

		if (timer->timer_mode == CIT_GP_TIMER_NEXT_START_BIT)
			val |= BIT(CIT_TIMER_CMD_TIMER_MODE);
		else if (timer->timer_mode == CIT_GP_TIMER_IMMEDIATE)
			val &= ~BIT(CIT_TIMER_CMD_TIMER_MODE);
		else
			return -EINVAL;

		/* Enable interrupt */
		if (timer->interrupt_enable) {
			rv = cit_ch_interrupt_enable(handle, CIT_INTR_TIMER,
							cit_ch_timer_cb);
			if (rv)
				return -EINVAL;
		} else {
			rv = cit_ch_interrupt_disable(handle, CIT_INTR_TIMER);
			if (rv)
				return -EINVAL;
		}

		if (timer->enable)
			val |= BIT(CIT_TIMER_CMD_TIMER_EN_BIT);
		else
			val &= ~BIT(CIT_TIMER_CMD_TIMER_EN_BIT);
	} else {

		if (time->val > CIT_MAX_WAIT_TIME_COUNT) {
			SYS_LOG_WRN("Set wait timer more than 24 bits");
			time->val = CIT_MAX_WAIT_TIME_COUNT;
		}

		/* Disable timer */
		sys_clear_bit((base + CIT_TIMER_CMD),
						 CIT_TIMER_CMD_WAIT_EN_BIT);

		cit_sc_enter_critical_section(handle);
		handle->intr_status2 &= ~BIT(CIT_INTR_STAT_2_WAIT_BIT);
		cit_sc_exit_critical_section(handle);

		/* Set wait registers */
		val = (time->val >> (CHAR_BIT + CHAR_BIT)) & UCHAR_MAX;
		sys_write32(val, (base + CIT_WAIT_3));
		val = (time->val >> CHAR_BIT) & UCHAR_MAX;
		sys_write32(val, (base + CIT_WAIT_2));
		val = time->val & UCHAR_MAX;
		sys_write32(val, (base + CIT_WAIT_1));

		/* Enable interrupt */
		if (timer->interrupt_enable) {
			rv = cit_ch_interrupt_enable(handle, CIT_INTR_WAIT,
							cit_ch_wait_cb);
			if (rv)
				return -EINVAL;
		} else {
			rv = cit_ch_interrupt_disable(handle, CIT_INTR_WAIT);
			if (rv)
				return -EINVAL;
		}

		val = sys_read32(base + CIT_TIMER_CMD);
		if (timer->enable) {
			if (timer->timer_mode == CIT_WAIT_TIMER_BWT)
				val |= BIT(CIT_TIMER_CMD_WAIT_MODE_BIT);
			else if (timer->timer_mode == CIT_WAIT_TIMER_WWT)
				val &= ~BIT(CIT_TIMER_CMD_WAIT_MODE_BIT);
			else
				return -EINVAL;

			val |= BIT(CIT_TIMER_CMD_WAIT_EN_BIT);
		} else {
			val &= ~BIT(CIT_TIMER_CMD_WAIT_EN_BIT);
		}
	}
	sys_write32(val, (base + CIT_TIMER_CMD));

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Get the timer status
 *
 * @param handle Channel handle
 * @param timer_type Timer type
 *
 * @return 0 for success, error otherwise
 */
bool cit_timer_enable_status(struct ch_handle *handle, u8_t timer_type)
{
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	val = sys_read32(handle->base + CIT_TIMER_CMD);

	if (timer_type == CIT_GP_TIMER) {
		if (val & BIT(CIT_TIMER_CMD_TIMER_EN_BIT))
			return true;
	} else if (timer_type == CIT_WAIT_TIMER) {
		if (val & BIT(CIT_TIMER_CMD_WAIT_EN_BIT))
			return true;
	}

	SMART_CARD_FUNCTION_EXIT();
	return false;
}

/**
 * @brief Control the timer
 *
 * @param handle Channel handle
 * @param timer Timer type and mode in cit_timer struct
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_timer_isr_control(struct ch_handle *handle, struct cit_timer *timer)
{
	u32_t base = handle->base;
	u32_t val;
	s32_t rv;

	SMART_CARD_FUNCTION_ENTER();
	val = sys_read32(base + CIT_TIMER_CMD);

	if (timer->timer_type == CIT_GP_TIMER) {
		handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_TIMER_BIT);

		if (timer->interrupt_enable) {
			rv = cit_ch_interrupt_enable(handle, CIT_INTR_TIMER,
							cit_ch_timer_cb);
			if (rv)
				return -EINVAL;
		} else {
			rv = cit_ch_interrupt_disable(handle, CIT_INTR_TIMER);
			if (rv)
				return -EINVAL;
		}

		if (timer->enable)
			val |= BIT(CIT_TIMER_CMD_TIMER_EN_BIT);
		else
			val &= ~BIT(CIT_TIMER_CMD_TIMER_EN_BIT);

	} else {
		handle->intr_status1 &= ~BIT(CIT_INTR_STAT_1_TIMER_BIT);

		/* Enable interrupt */
		if (timer->interrupt_enable) {
			rv = cit_ch_interrupt_enable(handle, CIT_INTR_WAIT,
							cit_ch_wait_cb);
			if (rv)
				return -EINVAL;
		} else {
			rv = cit_ch_interrupt_disable(handle, CIT_INTR_WAIT);
			if (rv)
				return -EINVAL;
		}

		if (timer->enable) {
			if (timer->timer_mode == CIT_WAIT_TIMER_BWT)
				val |= BIT(CIT_TIMER_CMD_WAIT_MODE_BIT);
			else if (timer->timer_mode == CIT_WAIT_TIMER_WWT)
				val &= ~BIT(CIT_TIMER_CMD_WAIT_MODE_BIT);
			else
				return -EINVAL;

			val |= BIT(CIT_TIMER_CMD_WAIT_EN_BIT);
		} else {
			val &= ~BIT(CIT_TIMER_CMD_WAIT_EN_BIT);
		}
	}
	sys_write32(val, (base + CIT_TIMER_CMD));

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Enable the interrupt
 *
 * @param handle Channel handle
 * @param int_type Interrupt type
 * @param callback callback function
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_ch_interrupt_enable(struct ch_handle *handle, u8_t int_type,
			      isr_callback_func callback)
{
	u32_t reg, cnt;
	u32_t bit;

	SMART_CARD_FUNCTION_ENTER();
	SYS_LOG_DBG("Interrupt enable %d\n", int_type);
	if ((int_type >= CIT_INTR_MAX) || !(callback))
		return -EINVAL;

	if (int_type == CIT_INTR_EDC) {
		sys_set_bit((handle->base + CIT_PROTO_CMD),
					CIT_PROTO_CMD_EDC_EN_BIT);
		return 0;
	}

	for (cnt = 0; cnt < CIT_MAX_NUM_CALLBACK_FUNC; cnt++)  {
		if (handle->cb[int_type][cnt] == NULL) {
			handle->cb[int_type][cnt] = callback;
			break;
		} else if ((handle->cb[int_type][cnt] != NULL) &&
			(handle->cb[int_type][cnt] == callback)) {
			break;
		}
	}

	bit = int_type;
	/* One interrupt for card insert and remove */
	if (int_type >= CIT_INTR_PRES_REMOVE)
		bit--;

	if (bit < CHAR_BIT) {
		reg = handle->base + CIT_INTR_EN_1;
	} else {
		reg = handle->base + CIT_INTR_EN_2;
		bit = bit - CHAR_BIT;
	}

	if (!(sys_test_bit(reg, bit)))
		sys_set_bit(reg, bit);

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Disable the interrupt
 *
 * @param handle Channel handle
 * @param int_type Interrupt type
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_ch_interrupt_disable(struct ch_handle *handle, u8_t int_type)
{
	u32_t reg, bit;

	SMART_CARD_FUNCTION_ENTER();
	SYS_LOG_DBG("Interrupt disable %d\n", int_type);
	if (int_type >= CIT_INTR_MAX)
		return -EINVAL;

	if (int_type == CIT_INTR_EDC) {
		sys_clear_bit((handle->base + CIT_PROTO_CMD),
					CIT_PROTO_CMD_EDC_EN_BIT);
		return 0;
	}

	bit = int_type;
	/* One interrupt for both card insert and remove */
	if (int_type >= CIT_INTR_PRES_REMOVE)
		bit--;

	if (bit < CHAR_BIT) {
		reg = handle->base + CIT_INTR_EN_1;
	} else {
		reg = handle->base + CIT_INTR_EN_2;
		bit = bit - CHAR_BIT;
	}

	sys_clear_bit(reg, bit);
	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Reset block wait time
 *
 * @param handle Channel handle
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_reset_bwt(struct ch_handle *handle)
{
	s32_t rv = 0;
	struct cit_timer timer = {CIT_WAIT_TIMER, CIT_GP_TIMER_IMMEDIATE, false,
								 true};
	struct sc_time time = {SC_DEFAULT_BLOCK_WAITING_TIME,
							 SC_TIMER_UNIT_ETU};

	SMART_CARD_FUNCTION_ENTER();
	/* Need this for MetroWerks */
	timer.timer_type = CIT_WAIT_TIMER;
	timer.timer_mode = CIT_WAIT_TIMER_BWT;
	time.val = handle->ch_param.blk_wait_time.val;

	rv = cit_config_timer(handle, &timer, &time);
	handle->ch_param.blk_wait_time_ext.val = 0;

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Reset block wait time
 *
 * @param handle Channel handle
 * @param bwt block wait time value
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_bwt_integer(struct sc_channel_param *ch_param, u32_t bwt)
{
	u8_t baud_div, clk_div;
	u32_t val;

	SMART_CARD_FUNCTION_ENTER();
	/*
	 * The block waiting time is encoded as described in ISO 7816-3,
	 * repeated here in the following equation:
	 *	BWT = [11 + 2 bwi x 960 x 372 x D/F] etu
	 *
	 *	e.g If bwi = 4 and F/D = 372 then BWT = 15,371 etu.
	 *	The minimum and maximum BWT are ~186 and 15,728,651 etu.
	 */
	ch_param->host_ctrl.prescale = cit_get_prescale(ch_param->d_factor,
				 ch_param->f_factor) * ch_param->ext_clkdiv +
				(ch_param->ext_clkdiv - 1);

	baud_div = cit_get_bauddiv(ch_param->d_factor, ch_param->f_factor);
	clk_div = cit_get_clkdiv(ch_param->d_factor, ch_param->f_factor);

	if (bwt == 0x00)
		val = 960 * 372 * clk_div / (ch_param->host_ctrl.prescale + 1) /
							 baud_div + 11;
	else
		val = (2 << (bwt - 1)) * 960 *  372 * clk_div /
			(ch_param->host_ctrl.prescale + 1) / baud_div + 11;


	/* Change timer to equal calculated BWT */
	ch_param->blk_wait_time.val = val;
	ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;

	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Set card type
 *
 * @param handle Channel handle
 * @param card_type card type
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_set_card_type(struct ch_handle *handle, u32_t card_type)
{
	SMART_CARD_FUNCTION_ENTER();
	handle->ch_param.ctx_card_type.card_type = card_type;
	SMART_CARD_FUNCTION_EXIT();
	return 0;
}

/**
 * @brief Set card type characters
 *
 * @param handle Channel handle
 * @param historical_bytes Buffer pointer
 * @param len Length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_set_card_type_character(struct ch_handle *handle,
					  u8_t *historical_bytes, u32_t len)
{
	struct sc_card_type_ctx *ctx_card_type;
	s32_t rv = 0;
	u32_t cnt;

	SMART_CARD_FUNCTION_ENTER();
	if (len > SC_MAX_ATR_HISTORICAL_SIZE)
		return -EINVAL;

	ctx_card_type = &handle->ch_param.ctx_card_type;
	for (cnt = 0; cnt < COUNTOF(sc_def_card_type_settings); cnt++) {
		if (!memcmp(sc_def_card_type_settings[cnt].hist_bytes,
			historical_bytes,
			sc_def_card_type_settings[cnt].card_id_length)) {
			ctx_card_type->card_type
				= sc_def_card_type_settings[cnt].card_type;
			rv = 0;
			break;
		}
	}

	if (rv == 0) {
		switch (ctx_card_type->card_type) {
		case SC_CARD_TYPE_JAVA:
			break;
		case SC_CARD_TYPE_ACOS:
			ctx_card_type->card_char.acos_ctx.card_stage
							= historical_bytes[11];
			ctx_card_type->card_char.acos_ctx.no_of_files
							= historical_bytes[5];
			ctx_card_type->card_char.acos_ctx.personalized
					= (historical_bytes[9] & 0x80) ? 1 : 0;
			ctx_card_type->card_char.acos_ctx.code_no = 7;
			break;
		default:
			rv = -ENOTSUP;
			break;
		}
		memcpy(ctx_card_type->hist_bytes, historical_bytes, len);
	}

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Send apdu commands
 *
 * @param handle Channel handle
 * @param apducmd Command code
 * @param buf Command data
 * @param len Length of the buffer
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_apdu_commands(struct ch_handle *handle, u32_t apducmd,
				u8_t *buf, u32_t len)
{
	s32_t rv = 0;
	struct sc_card_type_ctx *ctx = &handle->ch_param.ctx_card_type;
	u32_t cnt;

	SMART_CARD_FUNCTION_ENTER();
	if (ctx->card_type == SC_CARD_TYPE_UNKNOWN)
		return -EINVAL;

	switch (apducmd) {
	case SC_APDU_PIN_VERIFY:
		if (ctx->card_type == SC_CARD_TYPE_ACOS) {
			if (len == 0) {
				u8_t default_pin[8] = {
					0x41, 0x43, 0x4f, 0x53, 0x54, 0x45,
					0x53, 0x54
				};

				rv = cit_acos2_submit_code(handle,
						ACOS_CODE_REF_IC, default_pin);
				if (rv)
					goto done;
				rv = cit_acos_get_card_info(handle);
				if (rv)
					goto done;
			} else {
				if (len != ACOS_SUBMIT_CODE_CODELEN) {
					rv = -EINVAL;
					goto done;
				}

				rv = cit_acos2_submit_code(handle,
						ACOS_CODE_REF_IC, buf);
				if (rv)
					goto done;
			}
		}
		break;

	case SC_APDU_READ_DATA:
		if (ctx->card_type == SC_CARD_TYPE_ACOS) {
			u8_t record_len, num_of_record, r_attr, w_attr;
			u16_t fid;

			rv = cit_acos_get_user_file_info(handle, 0, &record_len,
					 &num_of_record, &r_attr, &w_attr,
					 &fid);
			if (rv)
				goto done;
			rv = cit_acos2_read_file(handle, fid, len, buf);
			if (rv)
				goto done;
		}
		break;


	case SC_APDU_WRITE_DATA:
		if (ctx->card_type == SC_CARD_TYPE_ACOS) {
			u8_t record_len, num_of_record, r_attr, w_attr;
			u16_t fid;

			rv = cit_acos_get_user_file_info(handle, 0, &record_len,
					 &num_of_record, &r_attr, &w_attr,
					 &fid);
			if (rv)
				goto done;
			rv = cit_acos_create_user_file(handle, 0, len, r_attr,
							 w_attr, fid);
			if (rv)
				goto done;
			rv = cit_acos2_write_file(handle, fid, len, buf);
			if (rv)
				goto done;
		}
		break;

	case SC_APDU_DELETE_FILE:
		if (ctx->card_type == SC_CARD_TYPE_ACOS) {
			for (cnt = 0; cnt < SC_MAX_OBJ_ID_SIZE; cnt++) {
				if (ctx->current_id ==
						 ctx->obj_id[cnt].id_num) {
					memset(ctx->obj_id[cnt].id_string, 0,
					       SC_MAX_OBJ_ID_STRING);
					rv = 0;
					break;
				}
			}

		}
		break;

	default:
		rv = -ENOTSUP;
		break;
	}

done:
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Apdu set objects
 *
 * @param handle Channel handle
 * @param obj_id Object id
 * @param obj_id_len Object id length
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_apdu_set_objects(struct ch_handle *handle, s8_t *obj_id,
				   u32_t obj_id_len)
{
	struct sc_card_type_ctx *ctx = &handle->ch_param.ctx_card_type;
	u32_t cnt, len;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	len = obj_id_len;
	if (len > SC_MAX_OBJ_ID_STRING)
		len = SC_MAX_OBJ_ID_STRING;

	if (ctx->card_type == SC_CARD_TYPE_UNKNOWN)
		return -EINVAL;

	switch (ctx->card_type) {
	case SC_CARD_TYPE_JAVA:
	case SC_CARD_TYPE_ACOS:
		for (cnt = 0; cnt < SC_MAX_OBJ_ID_SIZE; cnt++) {
			if (!memcmp(ctx->obj_id[cnt].id_string, obj_id, len)) {
				ctx->current_id = ctx->obj_id[cnt].id_num;
				return 0;
			}
		}
		for (cnt = 0; cnt < SC_MAX_OBJ_ID_SIZE; cnt++) {
			if (ctx->obj_id[cnt].id_string[0] == 0) {
				memcpy(ctx->obj_id[cnt].id_string, obj_id, len);
				ctx->obj_id[cnt].id_num =
				  ctx->obj_id[(cnt == 0) ? 0 : (cnt - 1)].id_num
							+ (cnt == 0) ? 0 : 1;
				return 0;
			}
		}
		rv = -ENOMEM;
		break;
	default:
		rv = -ENOTSUP;
		break;
	}

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

/**
 * @brief Apdu get objects
 *
 * @param handle Channel handle
 * @param obj_id Object id
 * @param obj_id_len Object id length
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_channel_apdu_get_objects(struct ch_handle *handle, s8_t *obj_id,
				   u32_t obj_id_len)
{
	struct sc_card_type_ctx *ctx = &handle->ch_param.ctx_card_type;
	u32_t cnt, len;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	len = obj_id_len;
	if (len > SC_MAX_OBJ_ID_STRING)
		len = SC_MAX_OBJ_ID_STRING;

	if (ctx->card_type == SC_CARD_TYPE_UNKNOWN)
		return -EINVAL;

	switch (ctx->card_type) {
	case SC_CARD_TYPE_JAVA:
	case SC_CARD_TYPE_ACOS:
		for (cnt = 0; cnt < SC_MAX_OBJ_ID_SIZE; cnt++) {
			if (!memcmp(ctx->obj_id[cnt].id_string, obj_id, len)) {
				ctx->current_id = ctx->obj_id[cnt].id_num;
				return 0;
			}
		}
		rv = -ENXIO;
		break;

	default:
		rv = -ENOTSUP;
		break;
	}

	SMART_CARD_FUNCTION_EXIT();
	return rv;
}

s32_t sc_ush_self_test(struct ch_handle *handle)
{
	s32_t rv = 0;
	struct sc_channel_param ch_param;

	rv = cit_channel_detect_card(handle, SC_CARD_INSERTED, false);
	if (rv == 0) {
		if (handle->ch_status.card_activate == false) {
			SYS_LOG_DBG("USH_SelfTest: Activate card\n");
			cit_get_default_channel_settings(0, &ch_param);
			rv = cit_channel_reset_ifd(handle, SC_COLD_RESET);
			if (rv)
				goto done;
			rv = cit_channel_card_reset(handle, true);
			if (rv)
				goto done;
		}
	} else {
		SYS_LOG_INF("\n no card");
	}

	if (handle->rx_len == 0)
		rv = -EINVAL;
done:
	if (rv == 0)
		return CV_SUCCESS;
	else
		return CV_DIAG_FAIL;
}

/**
 * @brief Get card type
 *
 * @param handle Channel handle
 * @param card_type card type
 *
 * @return 0 for success, error otherwise
 */
s32_t cit_get_card_type(struct ch_handle *handle, u32_t *card_type)
{
	*card_type = handle->ch_param.ctx_card_type.card_type;
	return 0;
}

s32_t cit_apdu_transceive_t1(struct ch_handle *handle,
			     struct sc_transceive *tx_rx)
{
	struct sc_channel_param *ch_param = &handle->ch_param;
	struct sc_tpdu_t1 *tpdu_t1 = &handle->tpdu_t1;
	struct sc_transceive trx;
	u8_t buf[SC_T1_MAX_BUF_SIZE];
	u8_t tx_pcb = 0, rx_pcb, r_err = 0;
	u32_t tx_len = 0, tpdu_len, rx_len;
	u32_t card_rx_len, cnt;
	u8_t wtx = 0, ifs = 0, bwt_ext;
	u32_t err = 0, abort = 0;
	s32_t rv = 0;
	u8_t lrc;

	SMART_CARD_FUNCTION_ENTER();
	tpdu_t1->state = CIT_STATE_DATASEND;
	tpdu_t1->action = CIT_ACTION_TX_I;
	trx = *tx_rx;
	tpdu_t1->err = 0;

	while (1) {
		buf[SC_T1_OFFSET_NAD] = 0;
		switch (tpdu_t1->action) {
		case CIT_ACTION_TX_I:
			/* send out the application data */
			tx_pcb = SC_T1_I_BLK |
				 (tpdu_t1->n_device << SC_T1_I_BLK_N_SHIFT);
			tx_len = trx.tx_len;
			if (tx_len > ch_param->ifsd) {
				tx_len = ch_param->ifsd;
				tx_pcb |= SC_T1_I_BLK_MORE;
			}

			buf[SC_T1_OFFSET_PCB] = tx_pcb;
			buf[SC_T1_OFFSET_LEN] = tx_len;
			memcpy(&buf[SC_T1_OFFSET_INF], tx_rx->tx, tx_len);
			tpdu_len = tx_len + 3;
			break;

		case CIT_ACTION_TX_R:
			/* Send Ack to the Rx I block, or in send I Err case */
			SYS_LOG_DBG("Tx R blk");
			buf[SC_T1_OFFSET_PCB] = SC_T1_R_BLK | r_err |
				 (tpdu_t1->n_card << SC_T1_R_BLK_N_SHIFT);
			buf[SC_T1_OFFSET_LEN] = 0;
			tpdu_len = 3;
			break;

		case CIT_ACTION_TX_RESYNC:
			buf[SC_T1_OFFSET_PCB] = SC_T1_S_RESYNC_REQ;
			buf[SC_T1_OFFSET_LEN] = 0;
			tpdu_len = 3;
			break;

		case CIT_ACTION_TX_WTX:
			/* WTX response */
			SYS_LOG_DBG("Tx WTX");
			buf[SC_T1_OFFSET_PCB] = SC_T1_S_WTX_REP;
			buf[SC_T1_OFFSET_LEN] = 1;
			buf[SC_T1_OFFSET_INF] = wtx;
			tpdu_len = 4;
			break;

		case CIT_ACTION_TX_IFS:
			buf[SC_T1_OFFSET_PCB] = SC_T1_S_IFS_REP;
			buf[SC_T1_OFFSET_LEN] = 1;
			buf[SC_T1_OFFSET_INF] = ifs;
			tpdu_len = 4;
			break;

		case CIT_ACTION_TX_ABT:
			buf[SC_T1_OFFSET_PCB] = SC_T1_S_ABORT_REP;
			buf[SC_T1_OFFSET_LEN] = 0;
			tpdu_len = 3;
			break;

		default:
			return -EINVAL;
		}

		if (!ch_param->edc.enable) {
			lrc = 0;
			for (cnt = 0; cnt < tpdu_len; cnt++)
				lrc ^= buf[cnt];

			buf[tpdu_len++] = lrc;
		}

		rv = cit_ch_transmit(handle, buf, tpdu_len);
		if (rv) {
			SYS_LOG_ERR("Transmit fail %d", rv);
			err = 1;
		}

		rv = cit_ch_receive(handle, buf, &card_rx_len,
						 SC_T1_MAX_BUF_SIZE);
		if (rv) {
			SYS_LOG_ERR("Receive fail %d", rv);
			err = 1;
		}

		if (buf[SC_T1_OFFSET_NAD]) {
			SYS_LOG_ERR("Receive NAD not 0");
			err = 1;
		}

		if (!ch_param->edc.enable) {
			lrc = 0;
			for (cnt = 0; cnt < card_rx_len; cnt++)
				lrc ^= buf[cnt];

			if (lrc) {
				SYS_LOG_ERR("Receive LRC not correct");
				err = 1;
			}
		}
		card_rx_len--;

		if (err)
			goto error;

		rx_pcb = buf[SC_T1_OFFSET_PCB];
		rx_len = buf[SC_T1_OFFSET_LEN];

		switch (SC_T1_GET_BLK_TYPE(rx_pcb)) {
		case SC_T1_S_WTX_REQ:
			if (rx_len != 1) {
				err = 1;
				tpdu_t1->action = CIT_ACTION_TX_R;
			} else {
				wtx = buf[SC_T1_OFFSET_INF];
				bwt_ext = wtx * ch_param->blk_wait_time.val;
				cit_channel_bwt_ext_set(handle, bwt_ext);
				tpdu_t1->action = CIT_ACTION_TX_WTX;
			}
			break;

		case SC_T1_S_ABORT_REQ:
			if (rx_len != 0) {
				err = 1;
				tpdu_t1->action = CIT_ACTION_TX_R;
			} else {
				/*
				 * For Abort, we continue our normal handling,
				 * but return fail when all are done
				 */
				abort = 1;
				tpdu_t1->action = CIT_ACTION_TX_ABT;
			}
			break;

		case SC_T1_S_IFS_REQ:
			if (rx_len != 1) {
				err = 1;
				tpdu_t1->action = CIT_ACTION_TX_R;
			} else {
				ifs = buf[SC_T1_OFFSET_INF];
				tpdu_t1->action = CIT_ACTION_TX_IFS;
			}
			break;

		case SC_T1_I_BLK:
			/*
			 * Check LEN field here, INF field length should equal
			 * to LEN field
			 */
			if ((card_rx_len < SC_T1_OFFSET_INF) ||
				 ((card_rx_len - 3) != buf[SC_T1_OFFSET_LEN]))
				err = 1;

			/*
			 * This can only happen when we have sent out all of our
			 * data, e.g. tx_pcb does not have M bit set
			 */
			if (tpdu_t1->state == CIT_STATE_DATASEND)
				err = (tx_pcb & SC_T1_I_BLK_MORE) ? 1 : err;

			/*
			 * Our I blk can be acked by: receive N(S) in I
			 * different than previous received N(S)
			 * But the first N(S) from card is 0, and we initialize
			 * n_card to 0, so we check for equal here instead of
			 * difference
			 */
			err = (tpdu_t1->n_card !=
					 SC_T1_I_BLK_N(rx_pcb)) ? 1 : err;

			if (err == 0) {
				/* Get the data from received I block */
				memcpy(trx.rx, &buf[SC_T1_OFFSET_INF], rx_len);
				trx.rx += rx_len;
				tx_rx->rx_len += rx_len;
				/* assign card sequence number */
				tpdu_t1->n_card = SC_T1_I_BLK_N(rx_pcb);
				/* toggle since Ncard is next expected number */
				tpdu_t1->n_card =
						(tpdu_t1->n_card == 0) ? 1 : 0;
				SYS_LOG_DBG("n_card value changed to %d ",
							 tpdu_t1->n_card);
				/* Check if there are more I blocks pending */
				if (rx_pcb & SC_T1_I_BLK_MORE) {
					tpdu_t1->state  = CIT_STATE_DATARCV;
					tpdu_t1->action = CIT_ACTION_TX_R;
					r_err = 0;
					SYS_LOG_DBG("continue to rx I blk\n");
				} else {
					/* finished I block receive */
					SYS_LOG_DBG("all done\n");
					return (abort == 0) ? 0 : -EINVAL;
				}
			} else {
				SYS_LOG_DBG("Either N does not match or\n");
				SYS_LOG_DBG("we should not rx I blk now\n");
			}
			break;

		case SC_T1_R_BLK:
			if (SC_T1_R_BLK_OK(rx_pcb) && (rx_len == 0)) {
				/*
				 * We got ACK by R block
				 * Our I blk can also be acked by: receive N(S)
				 * in R different than previous transmitted N(S)
				 * e.g. same as toggled N(S)
				 */
				if (tpdu_t1->n_device ==
						SC_T1_R_BLK_N(rx_pcb)) {
					if (trx.tx_len >= tx_len) {
						/* for the next I block */
						SYS_LOG_DBG("Send next I\n");
						trx.tx += tx_len;
						trx.tx_len -= tx_len;
					} else {
						SYS_LOG_DBG("should get I blk");
						err = 1;
					}
				} else {
					/*
					 * Retransmit I blk, not regarded as
					 * error (Senario 10), toggle back
					 */
					SYS_LOG_DBG("N doesn't match, tx I");
					tpdu_t1->n_device =
					      (tpdu_t1->n_device == 0) ? 1 : 0;
				}
				tpdu_t1->action = CIT_ACTION_TX_I;
			} else {
				err = 1;
				SYS_LOG_DBG("Err. pcb=%d, rxLen=%d\n",
							 rx_pcb, rx_len);
			}
			break;

		default:
			err = 1;
		}

error:
		if (err) {
			r_err = SC_T1_R_BLK_ERR_OTHER;
			tpdu_t1->err++;
			SYS_LOG_DBG("Err counter %d", tpdu_t1->err);
			if (tpdu_t1->action == CIT_ACTION_TX_I) {
				SYS_LOG_DBG("Transmit I");
				tpdu_t1->action = CIT_ACTION_TX_R;
			}

			if (tpdu_t1->err <= SC_T1_ERR_COUNT_RETRANSMIT) {
				SYS_LOG_DBG("Retransmit Previous one");
				continue;
			}

			if (tpdu_t1->err <= SC_T1_ERR_COUNT_RESYNCH) {
				SYS_LOG_DBG("Transmit resync");
				tpdu_t1->action = CIT_ACTION_TX_RESYNC;
				continue;

			}
			SYS_LOG_DBG("Retrasmit limit exceeded");
			return -EINVAL;
		}
	}
	SMART_CARD_FUNCTION_EXIT();
}

s32_t cit_apdu_transceive_t0(struct ch_handle *handle,
			     struct sc_transceive *tx_rx)
{
	u8_t buf[SC_TPDU_HEADER_WITH_P3_SIZE];
	struct sc_transceive trx;
	u32_t tx_len;
	s32_t rv = 0;

	SMART_CARD_FUNCTION_ENTER();
	trx = *tx_rx;
	tx_len = trx.tx_len;
	if (tx_len < SC_TPDU_MIN_HEADER_SIZE)
		return -EINVAL;

	if (tx_len == SC_TPDU_MIN_HEADER_SIZE) {
		/* header only */
		memcpy(buf, trx.tx, SC_TPDU_MIN_HEADER_SIZE);
		buf[SC_TPDU_MIN_HEADER_SIZE] = 0;
		trx.tx = buf;
		trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE;
		rv = cit_tpdu_transceive(handle, &trx);
	} else if ((tx_len == SC_TPDU_HEADER_WITH_P3_SIZE) ||
		   (tx_len == (u32_t)(trx.tx[SC_APDU_OFFSET_LC] +
				SC_TPDU_HEADER_WITH_P3_SIZE))) {
		/* (header + Le) or (header + Lc + data) */
		rv = cit_tpdu_transceive(handle, &trx);
	} else if (tx_len == (u32_t)(trx.tx[SC_APDU_OFFSET_LC] +
				SC_TPDU_HEADER_WITH_P3_SIZE + 1)) {
		/* header + Lc + data + Le */
		trx.tx_len = tx_len - 1;
		rv = cit_tpdu_transceive(handle, &trx);
		if (!rv) {
			u16_t sw1_sw2;

			sw1_sw2 = trx.rx[trx.rx_len - 2] << 8 |
				  trx.rx[trx.rx_len - 1];
			/* use temp tpdu to send get response */
			buf[SC_APDU_OFFSET_CLA] = 0x00;
			buf[SC_APDU_OFFSET_INS] = 0xC0;
			buf[SC_APDU_OFFSET_P1] = 0x00;
			buf[SC_APDU_OFFSET_P2] = 0x00;

			if (sw1_sw2 == SC_RESPONSE_SUCCESS)  {
				/* success, Le will be used */
				buf[SC_APDU_OFFSET_LE] = tx_rx->tx[tx_len - 1];
			} else if ((sw1_sw2 & 0xff00) ==
					 SC_RESP_CMD_SUCCESS_DATA_AVAILABLE) {
				buf[SC_APDU_OFFSET_LE] = sw1_sw2 & 0Xff;
			} else {
				SYS_LOG_ERR("no Get Response APDU sent");
				rv = -EINVAL;
				goto done;
			}

			trx.tx = buf;
			rv = cit_tpdu_transceive(handle, &trx);
		}
	} else {
		rv = -ENOTSUP;
	}
done:
	tx_rx->rx_len = trx.rx_len;
	SMART_CARD_FUNCTION_EXIT();
	return rv;
}



s32_t cit_apdu_transceive(struct ch_handle *handle, struct sc_transceive *tx_rx)
{
	SMART_CARD_FUNCTION_ENTER();
	if (tx_rx->tx_len < SC_TPDU_MIN_HEADER_SIZE)
		return -EINVAL;

	if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0)
		return cit_apdu_transceive_t0(handle, tx_rx);
	else if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1)
		return cit_apdu_transceive_t1(handle, tx_rx);

	SMART_CARD_FUNCTION_EXIT();
	return -ENOTSUP;
}
