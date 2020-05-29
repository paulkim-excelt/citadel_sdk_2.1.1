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
 * @file sc_cli_states.c
 *
 * @brief  smart card cli states
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <uart.h>
#include <zephyr/types.h>
#include <sc/sc_datatypes.h>
#include <sc/sc.h>
#include "../test_sc_infra.h"
#include <ztest.h>
#include "test_emv2000_t0.h"
#include "test_emv2000_t1.h"

#define SC_CLI_EMV_AUTO_STX			0x02
#define SC_CLI_EMV_AUTO_ETX			0x03
#define SC_CLI_EMV_INVALID_LENGTH              0xffff

s32_t sc_cli_emv_set_channel_settings(struct sc_channel_param *ch_param);
s32_t sc_cli_emv_t0_test_routine(struct sc_cli_channel_handle *handle);
s32_t sc_cli_emv_t1_test_routine(struct sc_cli_channel_handle *handle);
extern u8_t cit_get_clkdiv(u8_t d_factor, u8_t f_factor);
extern u8_t cit_get_etu_clkdiv(u8_t d_factor, u8_t f_factor);
extern u8_t cit_get_prescale(u8_t d_factor, u8_t f_factor);
extern u8_t cit_get_bauddiv(u8_t d_factor, u8_t f_factor);
void sc_cli_emv_t1_create_first_t1_test_command(u8_t *tx_buf, u8_t *msg_length);
s32_t sc_cli_emv_t1_end_of_test(u8_t *rx_buf);

s32_t sc_cli_emv_transport_test(struct sc_cli_channel_handle  *handle)
{
	s32_t rv = 0;

	/* Reset the interface and check for card inserted. */
	rv = sc_channel_param_get(handle->dev, SC_DEFAULT_CHANNEL,
					 DEFAULT_SETTINGS, &handle->ch_param);
	if (rv)
		return -EINVAL;

	rv = sc_cli_emv_set_channel_settings(&handle->ch_param);
	if (rv)
		return -EINVAL;

	rv = sc_channel_param_set(handle->dev, SC_DEFAULT_CHANNEL,
						 &handle->ch_param);
	if (rv)
		return -EINVAL;

	sc_cli_reset(handle);
	sc_cli_detect_card(handle, SC_CARD_INSERTED);

	/* Get EMV ATR and validate it.*/
	rv = sc_cli_atr_receive(handle);

	/* Need this to update the application unCurrentIFSD */
	sc_channel_param_get(handle->dev, SC_DEFAULT_CHANNEL, CURRENT_SETTINGS,
				&handle->ch_param);

	if (!rv) {
		/* Only if ATR is successful */
		if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E0) {
			TC_DBG("This is Type 0 card");
			sc_cli_emv_t0_test_routine(handle);
		} else if (handle->ch_param.protocol == SC_ASYNC_PROTOCOL_E1) {
			TC_DBG("This is Type 1 card");
			sc_cli_emv_t1_test_routine(handle);
		}
	}

	sc_cli_deactivate(handle);
	return rv;
}

s32_t sc_cli_emv_automated_transport_test(struct sc_cli_channel_handle  *handle)
{
	struct device *dev;
	s32_t data = 0;
	s32_t rv = 0;

	TC_PRINT("Automated EMV2000 Transport Test, shared the same UART\n");
	k_sleep(500);
	dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
#ifdef CONFIG_UART_LINE_CTRL
	rv = uart_line_ctrl_set(dev, LINE_CTRL_BAUD_RATE, 19200);
	if (rv)
		return rv;
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
	k_sleep(500);

	while (1) {
		data = 0;
		TC_PRINT("\nWaiting for control byte from UART\n");
		while (data != SC_CLI_EMV_AUTO_STX) {
			if (data == SC_CLI_EMV_AUTO_ETX) {
				data = 0;
				TC_PRINT(".");
			}
			data = getch();
			if (data < 0)
				return -EINVAL;
		}
		TC_PRINT("got it\n");
		cit_sc_delay(9);
		TC_PRINT("Smartcard EMV Transport test...\n");

		sc_cli_emv_transport_test(handle);
	}

	return rv;
}

s32_t sc_cli_emv_set_channel_settings(struct sc_channel_param *ch_param)
{

	/*  Smart Card Standard */
	ch_param->standard = SC_STANDARD_EMV2000;
	TC_DBG("standard = %d\n", ch_param->standard);

	/* Asynchronous Protocol Types. */
	ch_param->protocol = SC_ASYNC_PROTOCOL_E0;
	TC_DBG("protocol = %d\n", ch_param->protocol);

	/* Set F, Clock Rate Conversion Factor */
	ch_param->f_factor = 1;
	TC_DBG("f_factor = %d\n", ch_param->f_factor);

	/* Set D,Baud Rate Adjustor */
	ch_param->d_factor = 1;
	TC_DBG("d_factor = %d\n", ch_param->d_factor);

	/* Set ETU Clock Divider */
	ch_param->host_ctrl.etu_clkdiv = cit_get_etu_clkdiv(1, 1);
	TC_DBG("etu_clkdiv = %d\n", ch_param->host_ctrl.etu_clkdiv);

	/* Set SC Clock Divider */
	ch_param->host_ctrl.sc_clkdiv = cit_get_clkdiv(1, 1);
	TC_DBG("sc_clkdiv = %d\n", ch_param->host_ctrl.sc_clkdiv);

	/* Set external Clock Divisor. For TDA only 1, 2,4,8 are valid value */
	ch_param->ext_clkdiv = 1;
	TC_DBG("External clock divisor = %d\n", ch_param->ext_clkdiv);

	/* Set Prescale */
	ch_param->host_ctrl.prescale = cit_get_prescale(1, 1);
	TC_DBG("Prescale = %d\n", ch_param->host_ctrl.prescale);

	/* Set baud divisor */
	ch_param->host_ctrl.bauddiv = cit_get_bauddiv(1, 1);
	TC_DBG("BaudDiv = %d\n", ch_param->host_ctrl.bauddiv);

	/* Set current IFSD */
	ch_param->ifsd = SC_MAX_TX_SIZE;
	TC_DBG("IFSD = %d\n", ch_param->ifsd);

	/* Set Number of transmit parity retries */
	ch_param->tx_retries = SC_EMV2000_DEFAULT_TX_PARITY_RETRIES;
	TC_DBG("Tx Retries = %d\n", ch_param->tx_retries);

	/* Set Number of receive parity retries */
	ch_param->rx_retries = SC_EMV2000_DEFAULT_RX_PARITY_RETRIES;
	TC_DBG("Rx Retries = %d\n", ch_param->rx_retries);

	/* Set work waiting time */
	ch_param->work_wait_time.val =  SC_DEFAULT_WORK_WAITING_TIME +
				 SC_DEFAULT_EXTRA_WORK_WAITING_TIME_EMV2000;
	ch_param->work_wait_time.unit = SC_TIMER_UNIT_ETU;
	TC_DBG("work Wait Time Value = %d\n", ch_param->work_wait_time.val);
	TC_DBG("work Wait Time Unit = %d\n", ch_param->work_wait_time.unit);

	/* Set block Wait time */
	ch_param->blk_wait_time.val =  SC_DEFAULT_BLOCK_WAITING_TIME +
				SC_DEFAULT_EXTRA_BLOCK_WAITING_TIME_EMV2000;
	ch_param->blk_wait_time.unit = SC_TIMER_UNIT_ETU;
	TC_DBG("block Wait Time Value = %d\n", ch_param->blk_wait_time.val);
	TC_DBG("block Wait Time Unit = %d\n", ch_param->blk_wait_time.unit);

	/* Set Extra Guard Time  */
	ch_param->extra_guard_time.val = 0;
	ch_param->extra_guard_time.unit = SC_TIMER_UNIT_ETU;
	TC_DBG("Extra Guard Time Value = %d\n",
					 ch_param->extra_guard_time.val);
	TC_DBG("Extra Guard Time unit = %d\n",
					 ch_param->extra_guard_time.unit);

	/* Set block Guard time */
	ch_param->blk_guard_time.val = SC_DEFAULT_BLOCK_GUARD_TIME;
	ch_param->blk_guard_time.unit = SC_TIMER_UNIT_ETU;
	TC_DBG("block Guard Time Value = %d\n", ch_param->blk_guard_time.val);
	TC_DBG("block Guard Time.unit = %d\n", ch_param->blk_guard_time.unit);

	/* Set character Wait time Integer */
	ch_param->cwi = SC_DEFAULT_CHARACTER_WAIT_TIME_INTEGER;
	TC_DBG("Character Wait Time Integer = %d\n", ch_param->cwi);

	/* Set EDC encoding */
	ch_param->edc.enable = true;
	ch_param->edc.crc = 0;
	TC_DBG("edc Enable = %d\n", ch_param->edc.enable);
	TC_DBG("edc crc = %d\n", ch_param->edc.crc);

	/* Set transaction time out */
	ch_param->timeout.val = SC_DEFAULT_TIME_OUT;
	ch_param->timeout.unit = SC_TIMER_UNIT_MSEC;
	TC_DBG("timeout value in milliseconds = %d\n", ch_param->timeout.val);
	TC_DBG("timeout unit = %d\n", ch_param->timeout.unit);

	/* auto deactivation sequence */
	ch_param->auto_deactive_req = false;
	TC_DBG("auto_deactive_req = %d\n", ch_param->auto_deactive_req);

	/* nullFilter */
	ch_param->null_filter = false;
	TC_DBG("Null Filter = %d\n", ch_param->null_filter);

	ch_param->read_atr_on_reset = true;
	TC_DBG("read_atr_on_reset = %d\n", ch_param->read_atr_on_reset);

	ch_param->blk_wait_time_ext.val =  0;
	ch_param->blk_wait_time_ext.unit = SC_TIMER_UNIT_ETU;
	TC_DBG("block Wait Time Ext Value in ETU = %d\n",
					ch_param->blk_wait_time_ext.val);
	TC_DBG("block Wait Time Ext unit = %d\n",
					ch_param->blk_wait_time_ext.unit);
	ch_param->ctx_card_type.vcc_level = SC_VCC_LEVEL_5V;

	return 0;
}

u8_t sc_cli_emv_determine_next_t0_cmd(u32_t msg_length, u8_t length_byte)
{
	if (msg_length == 0)
		return SC_CLI_EMV_COMMAND_TYPE_TEST_ROUTINE_CASE_4;

	else if ((msg_length < 5) || (length_byte == 0))
		return SC_CLI_EMV_COMMAND_TYPE_CASE_1OR2;

	else
		return SC_CLI_EMV_COMMAND_TYPE_CASE_3OR4;
}

s32_t sc_cli_emv_t0_test_routine(struct sc_cli_channel_handle *handle)
{
	u8_t auc_select_cmd[] = {
		0x00, SC_CLI_EMV2000_SELECT, 0x04, 0x00, 0x0E,
		0x31, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53,
		0x2E, 0x44, 0x44, 0x46, 0x30, 0x31};
	u8_t buf[SC_MAX_EMV_BUFFER_SIZE];
	u32_t msg_length;
	s32_t rv = 0;
	u8_t command;

	/* first command sent in T = 0 mode after ATR for Europay tests */
	command = SC_CLI_EMV_COMMAND_TYPE_TEST_ROUTINE_CASE_4;

	while (command != SC_CLI_EMV_COMMAND_TYPE_EOT_CMD) {
		/* Loop until end of test command has been received from ICC */
		switch (command) {
		case SC_CLI_EMV_COMMAND_TYPE_TEST_ROUTINE_CASE_4:
			/*
			 * All Europay T = 0 tests begin with the Select
			 * command that is input into the buffer below,
			 * and then a Case 4 command is sent.
			 */
			memcpy(buf, auc_select_cmd, sizeof(auc_select_cmd));
			msg_length = sizeof(auc_select_cmd);

			rv = sc_cli_emv_t0_case_3or4_cmd(handle, buf,
						msg_length, buf, &msg_length);
			if (rv) {
				TC_DBG("emv_t0_case_3or4_cmd fail\n");
				return rv;
			}
			break;
		case SC_CLI_EMV_COMMAND_TYPE_CASE_1OR2:
			rv = sc_cli_emv_t0_case_1or2_cmd(handle, buf,
						msg_length, buf, &msg_length);
			if (rv) {
				TC_ERROR("emv_t0_case_1or2_cmd fail\n");
				return rv;
			}
			break;
		case SC_CLI_EMV_COMMAND_TYPE_CASE_3OR4:
			rv = sc_cli_emv_t0_case_3or4_cmd(handle, buf,
						msg_length, buf, &msg_length);
			if (rv) {
				TC_DBG("emv_t0_case_3or4_cmd fail\n");
				return rv;
			}
			break;
		default:
			return -EINVAL;
		}

		if ((buf[0] == 0x11) && (buf[1] == 0x70) && (buf[2] == 0x33) &&
		    (buf[3] == 0x44) &&
		    ((buf[4] == 0x55) || (buf[4] == 0x00))) {
			TC_DBG("smartCardEMVT0_test_routine: success");
			return 0;
		}

		/*
		 *  For EMVeriPOS tester, (00, 70, 00, 00, 00)
		 *  to deactivate the ICC
		 */
		if ((buf[0] == 0x00) && (buf[1] == 0x70)) {
			TC_DBG("emv_t0_test_routine: End of Successful Test");
			return 0;
		}

		TC_DBG("emv_t0_test_routine: Len = %d, buf[4] = %d",
						 msg_length, buf[4]);
		/* Determine next command to be processed */
		command = sc_cli_emv_determine_next_t0_cmd(msg_length, buf[4]);
#ifdef BROADCOM_EMVCO_TESTER
		/*
		 * For patched loop-back application, will delay 150ms between
		 * each APDU exchange. It is a tool limitation for syncing the
		 * tester and tested board.
		 */
		cit_sc_delay(150);
#else
		/*
		 *  With New FIME EMVco Tester.
		 *  For patched loop-back application, will delay 200ms between
		 *  each APDU exchange. It is a tool limitation for syncing the
		 *  tester and tested board.
		 */
		cit_sc_delay(200);
#endif
	}

	return 0;
}

s32_t sc_cli_emv_t1_test_routine(struct sc_cli_channel_handle *handle)
{
	u8_t chain_buf[4 * SC_MAX_EMV_BUFFER_SIZE];
	u8_t buf[SC_MAX_EMV_BUFFER_SIZE];
	u8_t resend_i_buf[SC_MAX_EMV_BUFFER_SIZE];
	u8_t check_buf[SC_MAX_EMV_BUFFER_SIZE];
	u32_t chain_len = 0, chain_position = 0;
	u32_t msg_length = 0, no_of_resends = 0;
	u32_t repeat_parity_or_edc = 0;
	/*
	 * Make sure the prev length as invalid length. If it is 0 and the next
	 * packet length is 0 due to errors, it will be treated as same packet
	 * which results 1785 00, 01, 02 fail.
	 */
	u32_t prev_rcvd_msg_length = SC_CLI_EMV_INVALID_LENGTH;
	u32_t repeated_icc_block = 0;
	u8_t previous_blk_sent;
	u8_t next_blk_type_tx, nxt_expect_block_type_to_rec;
	u8_t data_len;
	u32_t cnt;
	s32_t rv = 0;

	/* Reset sequence numbers to initial value of zero */
	handle->icc_seq_num = SC_CLI_SEQUENCE_NUMBER_0;
	handle->ifd_seq_num = SC_CLI_SEQUENCE_NUMBER_0;

	while (1) {
		/* Begin with an IFS request to ICC to set IFSD to 254 bytes */
		rv = cli_emv_t1_pcb_s_block(handle, SC_S_BLK_IFS_REQ, 0xFE);
		if (rv == -EINVAL)
			return rv;

		rv = sc_cli_emv_t1_receive(handle, buf, &msg_length);
		if ((rv == -EINVAL) || (rv == -ENOMSG))
			return rv;

		previous_blk_sent = SC_S_BLK_IFS_REQ;

		rv = sc_cli_emv_t1_get_next_cmd(handle,	previous_blk_sent,
			&next_blk_type_tx, &nxt_expect_block_type_to_rec,
			buf, buf, &data_len, rv);

		if ((next_blk_type_tx == SC_S_BLK_IFS_RESP) &&
					(msg_length == 5)) {
			if ((buf[0] == 0x00) &&	(buf[1] == 0xE1) &&
			    (buf[2] == 0x01) &&	(buf[3] == 0xFE) &&
						(buf[4] == 0x1E))
				/* Successful response returned from ICC */
				break;
		}

		no_of_resends++;
		if (no_of_resends == 3)
			return -EINVAL;
	}

	/*
	 * MUST MAKE THIS A LOOP UNTIL SUCCESSFUL IFS RESPONSE
	 * IS RECEIVED FROM ICC
	 */
	sc_cli_emv_t1_create_first_t1_test_command(buf, &data_len);
	next_blk_type_tx = SC_I_BLK_SEQ_0_NO_CHAINING;
	 /* Should receive any I-Block */
	nxt_expect_block_type_to_rec = SC_I_BLK_SEQ_0_NO_CHAINING;
	goto send_cmd;

	while (1) {
		rv = sc_cli_emv_t1_receive(handle, buf, &msg_length);
		if (rv == -EINVAL || rv == -ENOMSG)
			return rv;

		/* Copy contents into check buffer */
		if (prev_rcvd_msg_length == msg_length) {
			for (cnt = 0; cnt < msg_length; cnt++) {
				if (check_buf[cnt] != buf[cnt])
					break;
			}

			if (cnt == msg_length)
				repeated_icc_block++;
			else
				repeated_icc_block = 0;

			if (repeated_icc_block == 2) {
				TC_DBG("Three repeated blocks received,");
				TC_DBG("no valid response from terminal-");
				TC_DBG("deactivating\n");
			}
		} else {
			repeated_icc_block = 0;
		}

		if (rv == -EBADMSG)
			repeat_parity_or_edc++;
		else
			repeat_parity_or_edc = 0;

		if (repeat_parity_or_edc == 3) {
			TC_DBG("3 parity/EDC errors , deactivating\n");
			return -EINVAL;
		}

		/* Copy contents into check buffer */
		for (cnt = 0; cnt < msg_length; cnt++)
			check_buf[cnt] = buf[cnt];

		prev_rcvd_msg_length = msg_length;

		if (rv != -EBADMSG) {
			rv = sc_cli_reset_block_wait_timer(handle);
			if (rv)
				return rv;
		}

		TC_DBG("Error determining next terminal command 0x%x\n", rv);

		rv = sc_cli_emv_t1_get_next_cmd(handle, previous_blk_sent,
			&next_blk_type_tx, &nxt_expect_block_type_to_rec,
			buf, buf, &data_len, rv);

		if (rv == -EINVAL)
			return rv;
#ifdef BROADCOM_EMVCO_TESTER
		/*
		 * For patched loop-back application, will delay 150ms between
		 * each APDU exchange. It is a tool limitation for syncing the
		 * tester and tested board.
		 */
		cit_sc_delay(150);
#else
		/*
		 *  With New FIME EMVco Tester.
		 *  For patched loop-back application, will delay 200ms
		 *  between each APDU exchange. It is a tool limitation for
		 *  syncing the tester and tested board.
		 */
		cit_sc_delay(200);
#endif

send_cmd:
		if ((next_blk_type_tx == SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR) ||
		    (next_blk_type_tx == SC_R_BLK_SEQ_0_OTHER_ERR) ||
		    (next_blk_type_tx == SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR) ||
		    (next_blk_type_tx == SC_R_BLK_SEQ_1_OTHER_ERR)) {
			if (repeated_icc_block == 2)
				return -EINVAL;
		} else {
			repeated_icc_block = 0;
		}

		switch (next_blk_type_tx) {
		case SC_I_BLK_SEQ_0_NO_CHAINING:
			if (sc_cli_emv_t1_end_of_test(buf))
				return 0;

			if ((chain_len == 0) &&
			    (data_len > handle->ch_param.ifsd)) {
				chain_len = data_len;
				for (cnt = 0; cnt < chain_len; cnt++)
					chain_buf[cnt] = buf[cnt];
			}

			if (chain_len != 0) {
				if (((previous_blk_sent == 0x80) ||
				     (previous_blk_sent == 0x90)) &&
				    (data_len != 0)) {
					for (cnt = 0; cnt < data_len; cnt++)
						chain_buf[cnt + chain_len] =
								buf[cnt];
					chain_len += data_len;
				}

				if (chain_len > handle->ch_param.ifsd) {
					data_len = handle->ch_param.ifsd;
					for (cnt = 0; cnt < data_len; cnt++)
						buf[cnt] = chain_buf[cnt +
							       chain_position];

					chain_len -= data_len;
					chain_position += data_len;

					next_blk_type_tx =
						 SC_I_BLK_SEQ_0_CHAINING;
					nxt_expect_block_type_to_rec =
							SC_R_BLK_SEQ_0_ERR_FREE;
				} else {
					data_len = chain_len;
					for (cnt = 0; cnt < data_len; cnt++)
						buf[cnt] = chain_buf[cnt +
								chain_position];

					chain_position = 0;
					chain_len = 0;
				}
			}
			rv = sc_cli_emv_t1_i_block(handle, next_blk_type_tx,
						   buf, data_len, resend_i_buf);

			if (rv == -EINVAL)
				return rv;
			break;

		case SC_I_BLK_SEQ_1_NO_CHAINING:
			if (sc_cli_emv_t1_end_of_test(buf))
				return 0;
			if ((chain_len == 0) &&
			    (data_len > handle->ch_param.ifsd)) {
				chain_len = data_len;
				for (cnt = 0; cnt < chain_len; cnt++)
					chain_buf[cnt] = buf[cnt];
			}

			if (chain_len != 0) {
				if (((previous_blk_sent == 0x80) ||
				     (previous_blk_sent == 0x90)) &&
				    (data_len != 0)) {
					for (cnt = 0; cnt < data_len; cnt++)
						chain_buf[cnt + chain_len] =
								buf[cnt];
					chain_len += data_len;
				}

				if (chain_len > handle->ch_param.ifsd) {
					data_len = handle->ch_param.ifsd;
					for (cnt = 0; cnt < data_len; cnt++)
						buf[cnt] = chain_buf[cnt +
							       chain_position];

					chain_len -= data_len;
					chain_position += data_len;

					next_blk_type_tx =
						 SC_I_BLK_SEQ_1_CHAINING;
					nxt_expect_block_type_to_rec =
							SC_R_BLK_SEQ_0_ERR_FREE;
				} else {
					data_len = chain_len;
					for (cnt = 0; cnt < data_len; cnt++)
						buf[cnt] = chain_buf[cnt +
							  chain_position];

					chain_position = 0;
					chain_len = 0;
				}
			}
			rv = sc_cli_emv_t1_i_block(handle, next_blk_type_tx,
						   buf, data_len, resend_i_buf);

			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_0_ERR_FREE:
			if (previous_blk_sent != next_blk_type_tx) {
				/* See 1776 */
				for (cnt = 0; cnt < data_len; cnt++)
				chain_buf[cnt + chain_len] = buf[cnt];
				chain_len += data_len;
			}

			rv = cli_emv_t1_pcb_r_block(handle,
						SC_R_BLK_SEQ_0_ERR_FREE);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_1_ERR_FREE:
			if (previous_blk_sent != next_blk_type_tx) {
			/* See 1776 */
				for (cnt = 0; cnt < data_len; cnt++)
					chain_buf[cnt + chain_len] = buf[cnt];
				chain_len += data_len;
			}

			rv = cli_emv_t1_pcb_r_block(handle,
						SC_R_BLK_SEQ_1_ERR_FREE);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR:
			rv = cli_emv_t1_pcb_r_block(handle,
					SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR:
			rv = cli_emv_t1_pcb_r_block(handle,
					SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_0_OTHER_ERR:
			rv = cli_emv_t1_pcb_r_block(handle,
						SC_R_BLK_SEQ_0_OTHER_ERR);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_R_BLK_SEQ_1_OTHER_ERR:
			rv = cli_emv_t1_pcb_r_block(handle,
						SC_R_BLK_SEQ_1_OTHER_ERR);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_S_BLK_WTX_RESP:
			rv = cli_emv_t1_pcb_s_block(handle,
						SC_S_BLK_WTX_RESP, buf[0]);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_S_BLK_IFS_RESP:
			rv = cli_emv_t1_pcb_s_block(handle,
						SC_S_BLK_IFS_RESP, buf[0]);
			if (rv == -EINVAL)
				return rv;
			break;

		case SC_S_BLK_ABORT_RESP:
			rv = cli_emv_t1_pcb_s_block(handle,
					SC_S_BLK_ABORT_RESP, buf[0]);
			if (rv == -EINVAL)
				return rv;

			TC_DBG("S BLK ABORT RESPONSE, need to be delayed\n");
			test_sc_delay(1);

			return 0;

		case SC_RESEND_PREVIOUS_BLK:
			if (no_of_resends == 2)
				return -EINVAL;

			TC_DBG("Re-sending Previous I Block");

			memcpy(handle->tx_buf, resend_i_buf,
				resend_i_buf[2] + SC_EMV_PROLOGUE_FIELD_LENGTH);
			handle->tx_len = resend_i_buf[2] +
						 SC_EMV_PROLOGUE_FIELD_LENGTH;
			rv = sc_cli_transmit(handle);
			if (rv == -EINVAL)
				return rv;
			break;

		default:
			break;
		}

		if (next_blk_type_tx == SC_RESEND_PREVIOUS_BLK) {
			no_of_resends++;
			previous_blk_sent = resend_i_buf[1];
		} else {
			no_of_resends = 0;
			previous_blk_sent = next_blk_type_tx;
		}
	}

	return 0;
}

s32_t sc_cli_emv_t1_end_of_test(u8_t *rx_buf)
{
	/* EMV 2000 test 1767 xy=20 end with 1170334401 */
	if ((rx_buf[0] == 0x11) && (rx_buf[1] == 0x70) && (rx_buf[2] == 0x33) &&
	    (rx_buf[3] == 0x44) &&
	    ((rx_buf[4] == 0x55) || (rx_buf[4] == 0x01))) {
		return 1;
	};

	/* EOT of EMVco test, R_APDU = 0070800000 or 0070000000 for EMVeriPOS */
	if ((rx_buf[0] == 0x00) &&
	    (rx_buf[1] == 0x70) &&
	    (rx_buf[2] == 0x80 || rx_buf[2] == 0x00) &&
	    (rx_buf[3] == 0x00) &&
	    (rx_buf[4] == 0x00)) {
		return 1;
	};

	return 0;
}

s32_t sc_cli_emv_test_menu(struct sc_cli_channel_handle *handle)
{
	u32_t command_id;
	s32_t rv = 0;

	while (1) {
		TC_PRINT("\nSelect EMV2000 SmartCard Standard:\n");
		TC_PRINT("1) EMV 2000 Test\n");
		TC_PRINT("4) Deactivation sequence\n");
		TC_PRINT("5) Automated EMV2000 Transport Test, shared UART\n");
		TC_PRINT("0) Exit\n");

		command_id = prompt();

		switch (command_id) {
		case 0:
			return 0;
		case 1:
			rv = sc_cli_emv_transport_test(handle);
			break;

		case 4:
			rv = sc_cli_deactivate(handle);
			break;

		case 5:
			rv = sc_cli_emv_automated_transport_test(handle);
			break;

		default:
			TC_PRINT("\nInvalid Choice!");
			rv = -ENOTSUP;
			break;
		}
	}
	return rv;
}

void sc_cli_emv_t1_create_first_t1_test_command(u8_t *tx_buf, u8_t *msg_length)
{
	/* Select payment system environment PSE */
	tx_buf[0] = 0x00;
	tx_buf[1] = 0xA4;  /* Select command */
	tx_buf[2] = 0x04;  /* Select directory by name */
	tx_buf[3] = 0x00;
	tx_buf[4] = 0x0E;  /* data length of "1PAY.SYS.DDF01" */
	tx_buf[5] = 0x31;  /* '1' */
	tx_buf[6] = 0x50;  /* 'P' */
	tx_buf[7] = 0x41;  /* 'A' */
	tx_buf[8] = 0x59;  /* 'Y' */
	tx_buf[9] = 0x2E;  /* '.' */
	tx_buf[10] = 0x53; /* 'S' */
	tx_buf[11] = 0x59; /* 'Y' */
	tx_buf[12] = 0x53; /* 'S' */
	tx_buf[13] = 0x2E; /* '.' */
	tx_buf[14] = 0x44; /* 'D' */
	tx_buf[15] = 0x44; /* 'D' */
	tx_buf[16] = 0x46; /* 'F' */
	tx_buf[17] = 0x30; /* '0' */
	tx_buf[18] = 0x31; /* '1' */
	tx_buf[19] = 0x00;

	*msg_length = 0x14;
}
