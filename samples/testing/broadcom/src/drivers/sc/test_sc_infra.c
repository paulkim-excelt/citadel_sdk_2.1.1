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
 * @file test_sc_infra.c
 * @brief  Smart card test infrastructure
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>
#include <sc/sc_datatypes.h>
#include <sc/sc.h>
#include "test_sc_infra.h"
#include <ztest.h>
#include <test.h>
#include <stddef.h>

#define SC_TOTAL_T0_TEST_CMD		1

s32_t sc_cli_get_channel_default_settings(struct sc_cli_handle *cli_handle,
					  struct sc_cli_channel_handle *handle)
{
	ARG_UNUSED(cli_handle);
	return sc_channel_param_get(handle->dev, handle->channel_no,
				    DEFAULT_SETTINGS, &handle->ch_param);
}



s32_t sc_cli_open_channel(struct sc_cli_handle *cli_handle,
			  struct sc_cli_channel_handle *handle, u8_t channel_no,
			  struct sc_channel_param *ch_param)
{
	ARG_UNUSED(cli_handle);
	return sc_channel_open(handle->dev, channel_no, ch_param);
}

s32_t sc_cli_close_channel(struct sc_cli_channel_handle *handle)
{
	return sc_channel_close(handle->dev, handle->channel_no);
}

s32_t sc_cli_set_channel_settings(struct sc_cli_channel_handle *handle,
				  struct sc_channel_param *ch_param)
{
	s32_t rv = 0;

	rv = sc_channel_param_set(handle->dev, handle->channel_no,
				    ch_param);
	if (rv)
		return rv;

	rv = sc_channel_enable_interrupts(handle->dev,
			 handle->channel_no);
	return rv;
}


s32_t sc_cli_reset_ifd(struct sc_cli_channel_handle *handle,
		       enum sc_reset_type type)
{
	return sc_channel_interface_reset(handle->dev, handle->channel_no,
					  type);
}

s32_t sc_cli_activating(struct sc_cli_channel_handle *handle)
{
	return sc_channel_card_reset(handle->dev, handle->channel_no,
				     handle->ch_param.read_atr_on_reset);
}


s32_t sc_cli_atr_receive_decode(struct sc_cli_channel_handle *handle)
{
	return handle->receive_decode_atr_func((void *) handle);
}

s32_t sc_cli_post_atr_receive(struct sc_cli_channel_handle *handle)
{
	s32_t rv = 0;

	if (handle->ch_param.read_atr_on_reset == 0) {
		/* interpret and decode the ATR data and set channel settings */
		rv = sc_channel_param_set(handle->dev, handle->channel_no,
					    &handle->ch_param);
		if (rv)
			return rv;
		rv = sc_channel_enable_interrupts(handle->dev,
			 handle->channel_no);
	} else {
		/*
		 * SCD PI already interpreted and decoded the ATR data,
		 * and set channel settings
		 */
	}

	return rv;
}

s32_t sc_cli_send_cmd(struct sc_cli_channel_handle *handle, u8_t *tx_buf,
		      u32_t len)
{
	struct sc_transceive trx;

	trx.channel = 0;
	trx.tx = tx_buf;
	trx.tx_len = len;
	trx.rx = NULL;
	trx.rx_len = 0;
	trx.max_rx_len = 0;

	return sc_channel_transceive(handle->dev, SC_TPDU, &trx);
}

s32_t sc_cli_read_cmd(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
		      u32_t *len, u32_t max_len)
{
	struct sc_transceive trx;
	s32_t rv;

	trx.channel = handle->channel_no;
	trx.tx = NULL;
	trx.tx_len = 0;
	trx.rx = rx_buf;
	trx.rx_len = 0;
	trx.max_rx_len = max_len;

	rv = sc_channel_transceive(handle->dev, SC_TPDU, &trx);
	*len = trx.rx_len;
	return rv;
}

s32_t sc_cli_read_atr(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
		      u32_t *len, u32_t max_len)
{
	struct sc_transceive trx;
	s32_t rv;

	trx.channel = handle->channel_no;
	trx.tx = NULL;
	trx.tx_len = 0;
	trx.rx = rx_buf;
	trx.rx_len = 0;
	trx.max_rx_len = max_len;

	rv = sc_channel_read_atr(handle->dev, true, &trx);
	*len = trx.rx_len;
	return rv;
}

s32_t sc_cli_get_status(struct sc_cli_channel_handle *handle,
			struct sc_status *status)
{

	return sc_channel_status_get(handle->dev, handle->channel_no, status);
}

s32_t sc_cli_detect_card(struct sc_cli_channel_handle *handle,
			 enum sc_card_present status)
{
	return sc_channel_card_detect(handle->dev, handle->channel_no,
				      status, false);
}


s32_t sc_cli_deactivate(struct sc_cli_channel_handle *handle)
{
	return sc_channel_deactivate(handle->dev, handle->channel_no);
}

s32_t sc_cli_disable_work_wait_timer(struct sc_cli_channel_handle *handle)
{
	struct sc_time time;

	time.val = 0;
	time.unit = SC_TIMER_UNIT_ETU;
	return sc_channel_time_set(handle->dev, &handle->ch_param,
					WORK_WAIT_TIME, &time);
}

s32_t sc_cli_reset_block_wait_timer(struct sc_cli_channel_handle *handle)
{
	struct sc_time time;

	time.val = SC_RESET_BLOCK_WAITING_TIME;
	time.unit = SC_TIMER_UNIT_ETU;
	return sc_channel_time_set(handle->dev, &handle->ch_param,
					 BLK_WAIT_TIME, &time);
}

s32_t sc_cli_set_block_wait_time_ext(struct sc_cli_channel_handle *handle,
				     u32_t bwt_ext_etu)
{
	struct sc_time time;

	time.val = bwt_ext_etu;
	time.unit = SC_TIMER_UNIT_ETU;
	return sc_channel_time_set(handle->dev, &handle->ch_param,
					 BLK_WAIT_TIME_EXT, &time);
}

s32_t sc_cli_get_channel_parameters(struct sc_cli_channel_handle *handle,
					struct sc_channel_param *ch_param)
{
	return sc_channel_param_get(handle->dev, handle->channel_no,
					CURRENT_SETTINGS, ch_param);
}

s32_t  sc_cli_reset(struct sc_cli_channel_handle *handle)
{
	s32_t rv = 0;

	handle->state = sc_cli_state_initialized;
	rv = sc_cli_transit_state(handle, sc_cli_event_cold_reset);

	return rv;
}

s32_t  sc_cli_detect(struct sc_cli_channel_handle *handle, u8_t card_present)
{

	s32_t rv = 0;

	rv = sc_channel_status_get(handle->dev, handle->channel_no,
				   &handle->ch_status);
	if (rv)
		return rv;

	if (card_present == SC_CARD_INSERTED) {
		if (handle->ch_status.card_present == true) {
			TC_DBG("SmartCard present\n");
			return 0;
		}

		TC_DBG("SmartCard not present\n");
		rv = sc_channel_card_detect(handle->dev, handle->channel_no,
						SC_CARD_INSERTED, false);
	} else if (card_present == SC_CARD_REMOVED) {
		if (handle->ch_status.card_present == false) {
			TC_DBG("SmartCard Removed\n");
			return 0;
		}
		TC_DBG("SmartCard Present\n");
		TC_DBG("Please remove the SmartCard\n");
		rv = sc_channel_card_detect(handle->dev, handle->channel_no,
						SC_CARD_REMOVED, false);
	}

	return rv;
}

s32_t  sc_cli_atr_receive(struct sc_cli_channel_handle  *handle)
{
	return sc_cli_transit_state(handle, sc_cli_event_activate);
}


s32_t sc_cli_transmit(struct sc_cli_channel_handle  *handle)
{
	return sc_cli_transit_state(handle, sc_cli_event_tx_data);
}


s32_t sc_cli_receive(struct sc_cli_channel_handle  *handle)
{

	return sc_cli_transit_state(handle, sc_cli_event_rx_data);
}

void sc_cli_hex_dump(char *title, u8_t *buf, u32_t len)
{
	u32_t cnt;

	ARG_UNUSED(title);
	TC_DBG("\n%s (%d bytes):", title, len);

	for (cnt = 0; cnt < len; cnt++) {
		if (!(cnt % 20))
			TC_PRINT("\n");

		TC_PRINT("%02X  ", *(buf + cnt));
	}
	TC_PRINT("\n");
}

s32_t prompt(void)
{
	char choice[10];
	u8_t input_char;
	u32_t command;
	u32_t cnt = 0;

	input_char = 0;
	TC_PRINT("\nChoice: ");

#ifdef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
	while (1) {
		input_char = getch();
		choice[cnt] = input_char;
		cnt++;
		TC_PRINT("%c", input_char);
		if ((input_char == '\r') || (cnt >= 2))
			break;
	}
#else
	ARG_UNUSED(input_char);
	TC_PRINT("'getch' is not supported\n");
	return -ENOTSUP;
#endif

	choice[cnt] = '\0';

	if ((choice[0] == 'y')  ||  (choice[0] == 'Y')  ||
	    (choice[0] == 'n')  ||  (choice[0] == 'N'))
		return choice[0];

	command = atoi(choice);

	return command;
}

/* ATR for PC/SC Compliance Test Cards
 * 1) Athena T0 Test Card - 17
 *    3B D6 18 00 80 B1 80 6D 1F 03 80 51 00 61 10 30 9E
 * 2) Athena T0 Inverse Convention Card - 12
 *    3F 96 18 80 01 80 51 00 61 10 30 9F
 * 3) Axalto 32K eGate Test Card Number 3 - 10
 *    3B 95 18 40 FF 62 01 02 01 04
 * 4) Infineon SICRYPT Test Card (D4) - 24
 *    3B DF 18 00 81 31 FE 67 00 5C 49 43 4D D4 91 47 D2 76 00 00 38 33 00 58
 * 5) Infineon Version 1.0 - 24
 *    3B EF 00 00 81 31 20 49 00 5C 50 43 54 10 27 F8 D2 76 00 00 38 33 00 4D
 */

s32_t sc_cli_atr_t0_test(struct  sc_cli_channel_handle *handle)
{
	u8_t auc_rx_buf[SC_MAX_RX_SIZE];
	struct sc_transceive trx;
	s32_t rv = 0;

	rv = sc_cli_reset_ifd(handle, SC_COLD_RESET);
	if (rv)
		return rv;

	rv = sc_channel_card_detect(handle->dev, handle->channel_no,
					SC_CARD_INSERTED, false);
	if (rv)
		return rv;

	rv = sc_channel_card_reset(handle->dev, handle->channel_no,
				handle->ch_param.read_atr_on_reset);
	if (rv)
		return rv;

	trx.channel = handle->channel_no;
	trx.tx = NULL;
	trx.tx_len = 0;
	trx.rx = auc_rx_buf;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;
	rv = sc_channel_read_atr(handle->dev, false, &trx);

	sc_cli_hex_dump("ATR Data", auc_rx_buf, trx.rx_len);

	return rv;
}

s32_t sc_cli_acos2_t0_test(struct  sc_cli_channel_handle *handle)
{
	struct sc_transceive trx;
	u8_t auc_rx_buf[SC_MAX_RX_SIZE];
	s32_t test_count = 0;
	s32_t rv = 0;
	u32_t cnt;
	struct sc_cli_tx_rx_cmd t0_test[SC_TOTAL_T0_TEST_CMD] = {
		{
			7,
			{0x00, 0xa4, 0x00, 0x00, 0x02, 0x01, 0x02},
			1,
			{0xa4}
		},
	};

	rv = sc_cli_reset_ifd(handle, SC_COLD_RESET);
	if (rv)
		return rv;

	rv = sc_channel_card_detect(handle->dev, handle->channel_no,
					SC_CARD_INSERTED, false);
	if (rv)
		return rv;

	rv = sc_channel_card_reset(handle->dev, handle->channel_no,
					 handle->ch_param.read_atr_on_reset);
	if (rv)
		return rv;

	trx.channel = handle->channel_no;
	trx.tx = NULL;
	trx.tx_len = 0;
	trx.rx = auc_rx_buf;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;
	rv = sc_channel_read_atr(handle->dev, false, &trx);
	if (rv)
		return rv;
	sc_cli_hex_dump("ATR Data", auc_rx_buf, trx.rx_len);

	for (test_count = 0; test_count < SC_TOTAL_T0_TEST_CMD; test_count++) {
		/* Transmit some data to smart card */
		trx.tx = t0_test[test_count].tx_data;
		trx.tx_len = t0_test[test_count].tx_len;
		trx.rx = 0;
		trx.rx_len = 0;
		trx.max_rx_len = 0;
		rv = sc_channel_transceive(handle->dev, SC_TPDU, &trx);
		if (rv) {
			TC_DBG("Transmit data fail %d\n", rv);
			return rv;
		}
		sc_cli_hex_dump("TX:", trx.tx, trx.tx_len);

		/* Receive data from smart card */
		trx.tx = 0;
		trx.tx_len = 0;
		trx.rx = auc_rx_buf;
		trx.rx_len = 0;
		trx.max_rx_len = t0_test[test_count].rx_len;
		rv = sc_channel_transceive(handle->dev, SC_TPDU, &trx);
		if (rv) {
			TC_DBG("Receive data fail %d\n", rv);
			return rv;
		}
		sc_cli_hex_dump("RX:", auc_rx_buf, trx.rx_len);

		for (cnt = 0; cnt < t0_test[test_count].rx_len; cnt++) {
			if (auc_rx_buf[cnt] !=
					 t0_test[test_count].rx_data[cnt]) {
				TC_DBG("Receive data match fail %d\n", rv);
				return -EINVAL;
			}
		}

		TC_PRINT("\n\nSimple T0 passes\n\n");
	}
	return rv;
}
