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
 * @file test_sc_infra.h
 *
 * @brief smart card driver cli test header file
 */

#ifndef TEST_SC_INFRA_H__
#define TEST_SC_INFRA_H__

#include <test.h>

#define SC_CLI_MAX_CHANNELS		(1)
#define SC_DEFAULT_CHANNEL		(0)

#ifdef DEBUG_SC
#define TC_DBG(fmt, ...)		TC_PRINT(fmt, ##__VA_ARGS__)
#else
#define TC_DBG(fmt, ...)
#endif

s32_t prompt(void);

/* This enum is to identify modulo-2 Sequence Number in T=1 protocol */
enum sc_cli_sequence {
	SC_CLI_SEQUENCE_NUMBER_0 = 0,
	SC_CLI_SEQUENCE_NUMBER_1
};

/**
 * @brief This enum represents all the Smart Card State Transition Events.
 */
enum sc_cli_event {
	sc_cli_event_cold_reset,
	sc_cli_event_cold_reset_failed,
	sc_cli_event_cold_reset_passed,
	sc_cli_event_warm_reset,
	sc_cli_event_warm_reset_failed,
	sc_cli_event_warm_reset_passed,
	sc_cli_event_activate,
	sc_cli_event_activate_failed,
	sc_cli_event_activate_passed,
	sc_cli_event_rx_decode_failed,
	sc_cli_event_rx_decode_passed,
	sc_cli_event_tx_data,
	sc_cli_event_tx_data_failed,
	sc_cli_event_tx_data_passed,
	sc_cli_event_rx_data,
	sc_cli_event_rx_data_done,
	sc_cli_event_deactivate,
	sc_cli_event_initialize,
	sc_cli_event_release,
	sc_cli_event_max_event
};

/**
 * @brief This enum represents all the Smart Card cli states.
 */
enum sc_cli_state {
	sc_cli_state_unknown = 0,
	sc_cli_state_cold_resetting,
	sc_cli_state_warm_resetting,
	sc_cli_state_reset_done,
	sc_cli_state_activating,
	sc_cli_state_receive_decode_atr,
	sc_cli_state_ready,
	sc_cli_state_transmitting,
	sc_cli_state_transmitted,
	sc_cli_state_receiving,
	sc_cli_state_ignore,
	sc_cli_state_initialized,
	sc_cli_state_max_state
};

/* Smart Card Client Callback function to optimize the codes */
typedef s32_t (*sc_cli_rx_decode_ATR)(void *handle);

struct sc_cli_channel_handle {
	u8_t channel_no;
	struct sc_channel_param ch_param;
	u8_t state;
	u8_t event;
	u8_t reset_state;
	struct sc_status ch_status;
	u8_t tx_buf[SC_MAX_TX_SIZE];
	u32_t tx_len;
	u8_t rx_buf[SC_MAX_TX_SIZE];
	u32_t rx_len;
	u32_t rx_max_len;
	u8_t is_init;
	sc_cli_rx_decode_ATR receive_decode_atr_func;
	u8_t icc_seq_num;
	u8_t ifd_seq_num;
	struct device *dev;
};

struct sc_cli_tx_rx_cmd {
	u32_t tx_len;
	u8_t tx_data[SC_MAX_TX_SIZE];
	u32_t rx_len;
	u8_t rx_data[SC_MAX_RX_SIZE];
};

struct sc_cli_handle {
	struct sc_cli_channel_handle handle[SC_CLI_MAX_CHANNELS];
};

static inline void test_sc_delay(u32_t msec)
{
	k_busy_wait(msec * 1000);
}

s32_t sc_cli_open_channel(struct sc_cli_handle *cli_handle,
			  struct sc_cli_channel_handle *handle, u8_t channel_no,
			  struct sc_channel_param *ch_param);

s32_t sc_cli_get_channel_default_settings(struct sc_cli_handle *cli_handle,
					  struct sc_cli_channel_handle *handle);

s32_t sc_cli_close_channel(struct sc_cli_channel_handle *handle);

s32_t sc_cli_reset_ifd(struct sc_cli_channel_handle *handle,
		       enum sc_reset_type type);

s32_t sc_cli_activating(struct sc_cli_channel_handle *handle);

s32_t sc_cli_atr_receive_decode(struct sc_cli_channel_handle *handle);

s32_t sc_cli_post_atr_receive(struct sc_cli_channel_handle *handle);

s32_t sc_cli_PostATRReceive(struct sc_cli_channel_handle *handle);

s32_t sc_cli_send_cmd(struct sc_cli_channel_handle *handle, u8_t *tx,
		      u32_t len);

s32_t sc_cli_read_cmd(struct sc_cli_channel_handle *handle, u8_t *rx,
		      u32_t *len, u32_t max_len);

s32_t sc_cli_read_atr(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
		      u32_t *len, u32_t max_rx_len);

s32_t sc_cli_get_status(struct sc_cli_channel_handle *handle,
			struct sc_status *ch_status);

s32_t sc_cli_detect_card(struct sc_cli_channel_handle *handle,
			 enum sc_card_present card_present);

s32_t sc_cli_Deactivate(struct sc_cli_channel_handle *handle);

s32_t sc_cli_set_channel_settings(struct sc_cli_channel_handle *handle,
				  struct sc_channel_param *ch_param);

s32_t sc_cli_get_channel_parameters(struct sc_cli_channel_handle *handle,
				    struct sc_channel_param *ch_param);

s32_t sc_cli_reset_block_wait_timer(struct sc_cli_channel_handle *handle);

s32_t sc_cli_disable_work_wait_timer(struct sc_cli_channel_handle *handle);

s32_t sc_cli_set_block_wait_time_ext(struct sc_cli_channel_handle *handle,
				     u32_t bwt_ext);

void sc_cli_card_insert_cb_isr(struct sc_cli_channel_handle *handle,
			       void *inp_data);

void  sc_cli_card_remove_cb_isr(struct sc_cli_channel_handle *handle,
				void *inp_data);

s32_t sc_cli_transit_state(struct sc_cli_channel_handle *handle,
			   enum sc_cli_event event);

s32_t sc_cli_emv_test_menu(struct sc_cli_channel_handle *handle);

s32_t  sc_cli_atr_receive(struct sc_cli_channel_handle *handle);

s32_t sc_cli_transmit(struct sc_cli_channel_handle *handle);

s32_t sc_cli_receive(struct sc_cli_channel_handle *handle);

s32_t  sc_cli_reset(struct sc_cli_channel_handle *handle);

s32_t sc_cli_detect_card(struct sc_cli_channel_handle *handle,
			 enum sc_card_present status);

s32_t sc_cli_deactivate(struct sc_cli_channel_handle *handle);

s32_t sc_cli_acos2_t0_test(struct  sc_cli_channel_handle *handle);

s32_t sc_cli_apdu_test(struct sc_cli_channel_handle  *handle);

s32_t sc_cli_atr_t0_test(struct sc_cli_channel_handle  *handle);

s32_t sc_cli_dell_java_card_test(struct  sc_cli_channel_handle *handle);

s32_t sc_cli_debit_card_test(struct  sc_cli_channel_handle *handle);

void sc_cli_hex_dump(char *title, u8_t *buf, u32_t len);

static inline void cit_sc_delay(u32_t msec)
{
	k_busy_wait(msec * 1000);
}

#endif /* SC_CLI_INFRA_H__ */
