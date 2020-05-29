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
 * @file sc_bcm58202.h
 * @brief Register definitions of bcm58202 host controller
 */
#ifndef SC_BCM58202_H__
#define SC_BCM58202_H__

#define CIT_MAX_TX_SIZE				(264)
#define CIT_MAX_SUPPORTED_CHANNELS		(1)

#define CIT_UART_CMD_1				(0x00)
#define CIT_UART_CMD_1_INV_PAR_BIT		(7)
#define CIT_UART_CMD_1_GET_ATR_BIT		(6)
#define CIT_UART_CMD_1_IO_EN_BIT		(5)
#define CIT_UART_CMD_1_AUTO_RCV_BIT		(4)
#define CIT_UART_CMD_1_DIS_PAR_BIT		(3)
#define CIT_UART_CMD_1_TR_BIT			(2)
#define CIT_UART_CMD_1_XMIT_GO_BIT		(1)
#define CIT_UART_CMD_1_UART_RST_BIT		(0)

#define CIT_IF_CMD_1				(0x04)
#define CIT_IF_CMD_1_AUTO_CLK_BIT		(5)
#define CIT_IF_CMD_1_AUTO_IO_BIT		(6)
#define CIT_IF_CMD_1_AUTO_RST_BIT		(5)
#define CIT_IF_CMD_1_AUTO_VCC_BIT		(4)
#define CIT_IF_CMD_1_IO_BIT			(3)
#define CIT_IF_CMD_1_PRES_POL_BIT		(2)
#define CIT_IF_CMD_1_RST_BIT			(1)
#define CIT_IF_CMD_1_VCC_BIT			(0)

#define CIT_CLK_CMD_1				(0x08)
#define CIT_CLK_CMD_1_CLK_EN_BIT		(7)
#define CIT_CLK_CMD_1_SC_CLKDIV_MASK		(0x70)
#define CIT_CLK_CMD_1_SC_CLKDIV_SHIFT		(4)
#define CIT_CLK_CMD_1_ETU_CLKDIV_MASK		(0x0e)
#define CIT_CLK_CMD_1_ETU_CLKDIV_SHIFT		(1)
#define CIT_CLK_CMD_1_BAUDDIV_0_BIT		(0)

#define CIT_PROTO_CMD				(0x0c)
#define CIT_PROTO_CMD_CRC_BIT			(7)
#define CIT_PROTO_CMD_EDC_EN_BIT		(6)
#define CIT_PROTO_CMD_TBUF_RST_BIT		(5)
#define CIT_PROTO_CMD_RBUF_RST_BIT		(4)
#define CIT_PROTO_CMD_CWI_MASK			(0x0f)

#define CIT_PRESCALE				(0x10)
#define CIT_TGUARD				(0x14)
#define CIT_TRANSMIT				(0x18)
#define CIT_RECEIVE				(0x1c)
#define CIT_TLEN_2				(0x20)
#define CIT_TLEN_1				(0x24)

#define CIT_FLOW_CMD				(0x28)
#define CIT_FLOW_CMD_BAUDDIV_1_BIT		(7)
#define CIT_FLOW_CMD_FLOW_EN_BIT		(0)

#define CIT_RLEN_2				(0x2c)
#define CIT_RLEN_1				(0x30)
#define CIT_STATUS_1				(0x34)
#define CIT_STATUS_1_CARD_PRES_BIT		(6)
#define CIT_STATUS_1_TFLOW_BIT			(2)
#define CIT_STATUS_1_TEMPTY_BIT			(1)
#define CIT_STATUS_1_TDONE_BIT			(0)

#define CIT_STATUS_2				(0x38)
#define CIT_STATUS_2_RPAR_ERR			(7)
#define CIT_STATUS_2_ROVERFLOW_BIT		(3)
#define CIT_STATUS_2_EDC_ERR_BIT		(2)
#define CIT_STATUS_2_REMPTY_BIT			(1)
#define CIT_STATUS_2_RREADY_BIT			(0)

#define CIT_CLK_CMD_2				(0x3c)
#define CIT_CLK_CMD_2_BAUDDIV_2_BIT		(7)
#define CIT_CLK_CMD_2_IF_CLKDIV_SHIFT		(4)
#define CIT_CLK_CMD_2_SC_CLKDIV_3		(0)

#define CIT_UART_CMD_2				(0x40)
#define CIT_UART_CMD_2_CONVENTION_BIT		(6)
#define CIT_UART_CMD_2_RPAR_RETRY_MASK		(0x38)
#define CIT_UART_CMD_2_RPAR_RETRY_SHIFT		(0x3)
#define CIT_UART_CMD_2_TPAR_RETRY_MASK		(0x07)

#define CIT_BGT					(0x44)

#define CIT_TIMER_CMD				(0x48)
#define CIT_TIMER_CMD_TIMER_SRC			(7)
#define CIT_TIMER_CMD_TIMER_MODE		(6)
#define CIT_TIMER_CMD_TIMER_EN_BIT		(5)
#define CIT_TIMER_CMD_CWT_EN_BIT		(4)
#define CIT_TIMER_CMD_WAIT_MODE_BIT		(1)
#define CIT_TIMER_CMD_WAIT_EN_BIT		(0)

#define CIT_IF_CMD_2				(0x4c)
#define CIT_IF_CMD_2_DB_EN_BIT			(7)
#define CIT_IF_CMD_2_DB_MASK_BIT		(6)
#define CIT_IF_CMD_2_DB_WIDTH_MASK		0x1f

#define CIT_INTR_EN_1				(0x50)
#define CIT_INTR_EN_1_TPAR_BIT			(7)
#define CIT_INTR_EN_1_TIMER_BIT			(6)
#define CIT_INTR_EN_1_PRES_BIT			(5)
#define CIT_INTR_EN_1_BGT_BIT			(4)
#define CIT_INTR_EN_1_TDONE_BIT			(3)
#define CIT_INTR_EN_1_RETRY_BIT			(2)
#define CIT_INTR_EN_1_TEMPTY_BIT		(1)
#define CIT_INTR_EN_1_EVENT1_BIT		(0)

#define CIT_INTR_EN_2				(0x54)
#define CIT_INTR_EN_2_RPAR_BIT			(7)
#define CIT_INTR_EN_2_ATRS_BIT			(6)
#define CIT_INTR_EN_2_CWT_BIT			(5)
#define CIT_INTR_EN_2_RLEN_BIT			(4)
#define CIT_INTR_EN_2_WAIT_BIT			(3)
#define CIT_INTR_EN_2_EVENT2_BIT		(2)
#define CIT_INTR_EN_2_RCV_BIT			(1)
#define CIT_INTR_EN_2_RREADY_BIT		(0)

#define CIT_INTR_STAT_1				(0x58)
#define CIT_INTR_STAT_1_TPAR_BIT		(7)
#define CIT_INTR_STAT_1_TIMER_BIT		(6)
#define CIT_INTR_STAT_1_PRES_BIT		(5)
#define CIT_INTR_STAT_1_BGT_BIT			(4)
#define CIT_INTR_STAT_1_TDONE_BIT		(3)
#define CIT_INTR_STAT_1_RETRY_BIT		(2)
#define CIT_INTR_STAT_1_TEMPTY_BIT		(1)
#define CIT_INTR_STAT_1_EVENT1_BIT		(0)

#define CIT_INTR_STAT_2				(0x5c)
#define CIT_INTR_STAT_2_RPAR_BIT		(7)
#define CIT_INTR_STAT_2_ATRS_BIT		(6)
#define CIT_INTR_STAT_2_CWT_BIT			(5)
#define CIT_INTR_STAT_2_RLEN_BIT		(4)
#define CIT_INTR_STAT_2_WAIT_BIT		(3)
#define CIT_INTR_STAT_2_EVENT2_BIT		(2)
#define CIT_INTR_STAT_2_RCV_BIT			(1)
#define CIT_INTR_STAT_2_RREADY_BIT		(0)

#define CIT_TIMER_CMP_1				(0x60)
#define CIT_TIMER_CMP_2				(0x64)
#define CIT_TIMER_CNT_1				(0x68)
#define CIT_TIMER_CNT_2				(0x6c)
#define CIT_WAIT_1				(0x70)
#define CIT_WAIT_2				(0x74)
#define CIT_WAIT_3				(0x78)

#define CIT_IF_CMD_3				(0x7c)
#define CIT_IF_CMD_3_SYNC_TYPE2			(7)
#define CIT_IF_CMD_3_SYNC_TYPE1			(6)
#define CIT_IF_CMD_3_DEACT_BIT			(2)
#define CIT_IF_CMD_3_PRES_RST_EN_BIT		(1)
#define CIT_IF_CMD_3_VPP_BIT			(0)

#define CIT_EVENT1_CNT				(0x80)
#define CIT_EVENT1_CMP				(0x88)
#define CIT_EVENT1_CMD_1			(0x90)
#define CIT_EVENT1_CMD_2			(0x94)
#define CIT_EVENT1_CMD_3			(0x98)

#define CIT_EVENT1_CMD_4			(0x9c)
#define CIT_EVENT1_CMD_4_EVENT_EN		(7)
#define CIT_EVENT1_CMD_4_INR_AFTER_RESET	(3)
#define CIT_EVENT1_CMD_4_INR_AFTER_COMPARE	(2)
#define CIT_EVENT1_CMD_4_RUN_AFTER_RESET	(1)
#define CIT_EVENT1_CMD_4_RUN_AFTER_COMPARE	(0)

#define CIT_EVENT2_CMP				(0xa0)
#define CIT_EVENT2_CNT				(0xa8)
#define CIT_EVENT2_CMD_1			(0xb0)
#define CIT_EVENT2_CMD_2			(0xb4)
#define CIT_EVENT2_CMD_3			(0xb8)
#define CIT_EVENT2_CMD_4			(0xbc)
#define CIT_EVENT2_CMD_4_EVENT_EN		(7)
#define CIT_EVENT2_CMD_4_INR_AFTER_RESET	(3)
#define CIT_EVENT2_CMD_4_INR_AFTER_COMPARE	(2)
#define CIT_EVENT2_CMD_4_RUN_AFTER_RESET	(1)
#define CIT_EVENT2_CMD_4_RUN_AFTER_COMPARE	(0)

#define CIT_SCIRQ1_SCIRQEN			(0x200)
#define CIT_SCIRQ1_SCIRQSTAT			(0x204)
#define CIT_SCIRQ0_SCIRQEN			(0x210)
#define CIT_SCIRQ0_SCIRQSTAT			(0x214)
#define CIT_SCIRQ_SCPU_SCIRQEN			(0x220)
#define CIT_SCIRQ_SCPU_SCIRQSTAT		(0x224)

#define CIT_BAUDDIV_BIT_0			(0x01)
#define CIT_BAUDDIV_BIT_1			(0x02)
#define CIT_BAUDDIV_BIT_2			(0x04)

#define CIT_SC_CLKDIV_BIT_3			(0x08)

#define CIT_DEFAULT_IF_CLKDIV			(0x0)

/* Smart Card Module Event source for Event interrupt */
#define CIT_EVENT1_INTR_EVENT_SRC		(0x00)
#define CIT_TEMPTY_INTR_EVENT_SRC		(0x01)
#define CIT_RETRY_INTR_EVENT_SRC		(0x02)
#define CIT_TDONE_INTR_EVENT_SRC		(0x03)
#define CIT_BGT_INTR_EVENT_SRC			(0x04)
#define CIT_PRES_INTR_EVENT_SRC			(0x05)
#define CIT_TIMER_INTR_EVENT_SRC		(0x06)
#define CIT_TPAR_INTR_EVENT_SRC			(0x07)
#define CIT_RREADY_INTR_EVENT_SRC		(0x08)
#define CIT_RCV_INTR_EVENT_SRC			(0x09)
#define CIT_EVENT2_INTR_EVENT_SRC		(0x0a)
#define CIT_WAIT_INTR_EVENT_SRC			(0x0b)
#define CIT_RLEN_INTR_EVENT_SRC			(0x0c)
#define CIT_CWT_INTR_EVENT_SRC			(0x0d)
#define CIT_ATRS_INTR_EVENT_SRC			(0x0e)
#define CIT_RPAR_INTR_EVENT_SRC			(0x0f)
#define CIT_RX_ETU_TICK_EVENT_SRC		(0x10)
#define CIT_TX_ETU_TICK_EVENT_SRC		(0x11)
#define CIT_HALF_TX_ETU_TICK_EVENT_SRC		(0x12)
#define CIT_RX_START_BIT_EVENT_SRC		(0x13)
#define CIT_TX_START_BIT_EVENT_SRC		(0x14)
#define CIT_LAST_TX_START_BIT_BLOCK_EVENT_SRC	(0x15)
#define CIT_ICC_FLOW_CTRL_ASSERTED_EVENT_SRC	(0x16)
#define CIT_ICC_FLOW_CTRL_DONE_EVENT_SRC	(0x17)
#define CIT_START_IMMEDIATE_EVENT_SRC		(0x1f)
#define CIT_DISABLE_COUNTING_EVENT_SRC		(0x1f)
#define CIT_NO_EVENT_EVENT_SRC			(0x1f)

#define CIT_MAX_NUM_CALLBACK_FUNC		(2)
#define CIT_MAX_WAIT_TIME_COUNT			(0xffffff)

#define SC_MAX_BAUDDIV				(5)  /*just support 5 for SCI */
#define CIT_CLKDIV_MAX				(33)

#ifdef SC_DEBUG
#define SMART_CARD_FUNCTION_ENTER()	printk("BSCD: %s() ENTER\n", __func__)
#define SMART_CARD_FUNCTION_EXIT()	printk("BSCD: %s() LEAVE\n", __func__)
#else
#define SMART_CARD_FUNCTION_ENTER()
#define SMART_CARD_FUNCTION_EXIT()
#endif

#ifdef CONFIG_EMVCO_TEST
#define BSCD_EMV2000_CWT_PLUS_4_EVENT_INTR
#define BSCD_EMV_TEST
#endif

static const s8_t sc_d_value[10] = {-1, 1, 2, 4, 8, 16, 32, -1, 12, 20};

/* The clkdiv encode is the array element of the division.ex: clkdiv[16] -> 7 */
static const u8_t cit_clkdiv[CIT_CLKDIV_MAX] = {
	0, 0, 1, 2, 3, 4, 0, 0, 5, 0, 6, 0, 0, 0, 0, 0,
	7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8
};

static const u8_t bauddiv_valid[SC_MAX_BAUDDIV] = {31, 32, 25, 20, 10};

/*
 * @brief Interrupt Types
 */
enum cit_intr_type {
	CIT_INTR_EVENT1,	/* Event1 Interrupt */
	CIT_INTR_TEMPTY,	/* Transmit Empty Interrupt */
	CIT_INTR_RETRY,		/* Transmit or Receive Retry Overflow Intr */
	CIT_INTR_TDONE,		/* Transmit Done Interrupt */
	CIT_INTR_BGT,		/* Block Guard Time Interrupt */
	CIT_INTR_PRES_INSERT,	/* Card Insertion Interrupt */
	CIT_INTR_PRES_REMOVE,	/* Card Removal Interrupt */
	CIT_INTR_TIMER,		/* General-purpose Timer Interrupt */
	CIT_INTR_TPARITY,	/* Transmit Parity Interrupt. Only for T=0 */
	CIT_INTR_RREADY,	/* Receive Ready Interrupt. Only for T=1 */
	CIT_INTR_RCV,		/* Receive Character Interrupt */
	CIT_INTR_EVENT2,	/* Event2 Interrupt. */
	CIT_INTR_WAIT,		/* Block or Work Waiting Time Interrupt */
	CIT_INTR_RLEN,		/* Receive Length Error Interrupt. For T=1 */
	CIT_INTR_CWT,		/* CWT Interrupt */
	CIT_INTR_ATRS,		/* ATR Start Interrupt */
	CIT_INTR_RPARITY,	/* Receive Parity Interrupt. Only for T=0 */
	CIT_INTR_EDC,		/* EDC Error. Yet to be implemented. */
	CIT_INTR_MAX,
};

struct wait_events {
	u8_t card_wait;   /* card detection */
	u8_t tdone_wait;  /* transmit done */
	u8_t rcv_wait;    /*receive at least one byte */
	u8_t atr_start;   /* receive start bit of the ATR */
	u8_t timer_wait;  /* timer expired */
	u8_t event1_wait; /* timer expired */
	u8_t event2_wait; /* timer expired */
};

/*
 * @brief Timer type
 */
enum cit_timer_type {
	CIT_GP_TIMER = 0,
	CIT_WAIT_TIMER
};

/*
 * @brief Timer mode
 */
enum cit_timer_mode {
	CIT_GP_TIMER_IMMEDIATE = 0,
	CIT_GP_TIMER_NEXT_START_BIT,
	CIT_WAIT_TIMER_WWT,
	CIT_WAIT_TIMER_BWT
};

/**
 * @brief Citadel timer
 *
 * @param timer_type General purpose or waiting
 * @param timer_mode Timer mode
 * @param enable Enable
 * @param interrupt_enable interrupt enable
 */
struct cit_timer {
	u8_t timer_type;
	u8_t timer_mode;
	u8_t enable;
	u8_t interrupt_enable;
};

enum sc_state {
	CIT_STATE_DATASEND,
	CIT_STATE_DATARCV,
};

enum sc_t1_action {
	CIT_ACTION_TX_I,
	CIT_ACTION_TX_R,
	CIT_ACTION_TX_RESYNC,
	CIT_ACTION_TX_WTX,
	CIT_ACTION_TX_IFS,
	CIT_ACTION_TX_ABT
};

/**
 * @param state;
 * @param action;
 * @param device Device side send sequence number, for I block, 0 or 1
 * @param card Device side expected receive N, for R block 0 or 1,
 *             should match with Rxed N in Iblk
 * @param err Number of errors
 */
struct sc_tpdu_t1 {
	enum sc_state  state;
	enum sc_t1_action action;
	u8_t n_device;
	u8_t n_card;
	u32_t err;
};

typedef void (*isr_callback_func)(void *arg);
/**
 * @brief channel data
 *
 * @param ch_number Channel number
 * @param open True if the channel is opened
 * @param ch_status Channel status
 * @param base Register address base
 * @param clock Clock data
 * @param ch_param Channel parameters
 * @param ch_atr_param Channel parameters with negotiation
 * @param pps_needed Is PPS needed
 * @param reset_type Warm reset or cold reset
 * @param cb Array of callback functions
 * @param auc_tx_buf Transmission bytes
 * @param tx_len Number of transmission bytes
 * @param auc_rx_buf Receiving bytes
 * @param rx_len Number receiving bytes
 * @param tpdu_t1 Tpdu structure
 * @param receive
 * @param status1 Value of SC_STATUS_1
 * @param status2 Value of SC_STATUS_2
 * @param intr_status1 Value of SC_INTR_STATUS_1
 * @param intr_status2 Value of SC_INTR_STATUS_2
 * @param card_removed Is the Card removed
 * @param ns send sequence number, for T=1
 * @param nr Receive sequence number, for T=1
 * @param state T1 state
 * @param parity_error_cnt FIME EMV2000
 * @param key Irq lock key
 */
struct ch_handle {
	u8_t ch_number;
	u8_t open;
	struct sc_status ch_status;
	u32_t base;
	struct wait_events wait_event;
	struct icc_clock clock;
	struct sc_channel_param ch_param;
	struct sc_channel_param ch_neg_param;
	u8_t pps_needed;
	enum sc_reset_type reset_type;
	isr_callback_func cb[CIT_INTR_MAX][CIT_MAX_NUM_CALLBACK_FUNC];
	u8_t auc_tx_buf[SC_MAX_RX_SIZE];
	u32_t tx_len;
	u8_t auc_rx_buf[SC_MAX_RX_SIZE];
	u32_t rx_len;
	struct sc_tpdu_t1 tpdu_t1;
#ifdef BSCD_EMV2000_CWT_PLUS_4
	u8_t receive;
#endif
	u32_t status1;
	u32_t status2;
	u32_t intr_status1;
	u32_t intr_status2;
	u8_t card_removed;
	u8_t ns;
	u8_t nr;
	u8_t state;
#ifdef BSCD_EMV2000_FIME
	u8_t parity_error_cnt;
#endif
	u32_t key;
};

/**
 * @brief Clock parameter structure

 * @param sc_clkdiv Clock divisor
 * @param prescale Prescale
 * @param bauddiv Baud divisor
 * @param etu_clkdiv Etu clock divisor
 */
struct cit_df_clocks {
	u8_t sc_clkdiv;
	u8_t prescale;
	u8_t bauddiv;
	u8_t etu_clkdiv;
};

u8_t cit_get_clkdiv(u8_t d_factor, u8_t f_factor);

u8_t cit_get_etu_clkdiv(u8_t d_factor, u8_t f_factor);

u8_t cit_get_prescale(u8_t d_factor, u8_t f_factor);

u8_t cit_get_bauddiv(u8_t d_factor, u8_t f_factor);

static inline void cit_sc_enter_critical_section(struct ch_handle *handle)
{
	handle->key = irq_lock();
}

static inline void cit_sc_exit_critical_section(struct ch_handle *handle)
{
	irq_unlock(handle->key);
}

static inline void cit_sc_delay(u32_t msec)
{
	k_busy_wait(msec * 1000);
}

s32_t cit_channel_open(struct ch_handle *handle,
		       struct sc_channel_param *ch_param);

s32_t cit_channel_close(struct ch_handle *handle);

void cit_channel_handler_isr(struct ch_handle *handle);

/**
 * @brief Modify the current smart card channel settings.
 *
 * @param handle Channel handle
 * @param new Channel parameters struct
 *
 *@return 0 for success, error otherwise
 */
s32_t cit_channel_set_parameters(struct ch_handle *handle,
				 struct sc_channel_param *new);

/**
 * @brief Get the current smart card channel settings.
 *
 * @param handle Channel handle
 * @param new Channel parameters struct
 *
 *@return 0 for success, error otherwise
 */
s32_t cit_channel_get_parameters(struct ch_handle *handle,
				 struct sc_channel_param *ch_param);

/**
 * @brief Get the atr negitiated smart card channel setting.
 *
 * @param handle Channel handle
 * @param ch_neg_param Channel parameters struct
 *
 *@return 0 for success, error otherwise
 */
s32_t cit_channel_get_nego_parameters(struct ch_handle *handle,
				      struct sc_channel_param *ch_neg_param);

/**
 * @brief Get the default smart card channel settings.
 *
 * @param handle Channel handle
 * @param ch_param Channel parameters struct
 *
 *@return 0 for success, error otherwise
 */
s32_t cit_get_default_channel_settings(u32_t channel,
				       struct sc_channel_param *ch_param);

/**
 * @brief Set the vcc level.
 *
 * @param handle Channel handle
 * @param vcc_level Vcc level
 *
 *@return 0 for success, error otherwise
 */
s32_t cit_channel_set_vcclevel(struct ch_handle *handle, u8_t vcc_level);

s32_t cit_channel_card_power_up(struct ch_handle *handle);

s32_t cit_channel_card_power_down(struct ch_handle *handle);

s32_t cit_tpdu_transceive(struct ch_handle *handle,
			  struct sc_transceive *tx_rx);

s32_t cit_apdu_transceive(struct ch_handle *handle,
			  struct sc_transceive *tx_rx);

s32_t cit_channel_detect_card(struct ch_handle *handle,
			      enum sc_card_present status, bool block);

s32_t cit_channel_detect_cb_set(struct ch_handle *handle,
				enum sc_card_present status,
				sc_api_card_detect_cb cb);

s32_t cit_channel_get_status(struct ch_handle *handle,
			     struct sc_status *status);

s32_t cit_channel_reset_ifd(struct ch_handle *handle,
			    enum sc_reset_type reset_type);

s32_t cit_channel_card_reset(struct ch_handle *handle, u8_t decode_atr);

s32_t cit_ch_transmit(struct ch_handle *handle, u8_t *tx, u32_t len);

s32_t cit_ch_receive(struct ch_handle *handle, u8_t *rx, u32_t *rx_len,
		     u32_t max_len);

s32_t cit_channel_enable_intrs(struct ch_handle *handle);

s32_t cit_channel_set_standard(struct ch_handle *handle,
			       struct sc_channel_param *new);

s32_t cit_channel_set_frequency(struct ch_handle *handle,
				struct sc_channel_param *new);

s32_t cit_ch_wait_cardinsertion(struct ch_handle *handle);

s32_t cit_ch_wait_cardremove(struct ch_handle *handle);

s32_t cit_channel_set_edc_parity(struct ch_handle *handle,
				 struct sc_channel_param *new);

s32_t cit_channel_set_transaction_time(struct ch_handle *handle,
				       struct sc_channel_param *new);

s32_t cit_channel_set_guard_time(struct ch_handle *handle,
				 struct sc_channel_param *new);

s32_t cit_channel_set_wait_time(struct ch_handle *handle,
				struct sc_channel_param *new);

s32_t cit_channel_bwt_ext_set(struct ch_handle *handle, u32_t bwt_ext);

s32_t cit_ch_interrupt_disable(struct ch_handle *handle, u8_t int_type);

s32_t cit_program_clocks(struct ch_handle *handle);

s32_t cit_channel_activating(struct ch_handle *handle);

s32_t cit_channel_deactivate(struct ch_handle *handle);

s32_t cit_ch_interrupt_enable(struct ch_handle *handle, u8_t int_type,
			      isr_callback_func callback);

s32_t cit_channel_enable_interrupts(struct ch_handle *handle);

s32_t cit_config_timer(struct ch_handle *handle, struct cit_timer *timer,
		       struct sc_time *time);

s32_t cit_timer_isr_control(struct ch_handle *handle, struct cit_timer *timer);

bool cit_timer_enable_status(struct ch_handle *handle, u8_t timer_type);

s32_t cit_channel_receive_and_decode(struct ch_handle *handle);

s32_t cit_wait_for_event(struct ch_handle *handle, u8_t *event, s32_t time_out);

s32_t cit_channel_sync_read_data(struct ch_handle *handle, u8_t *rx,
				 u32_t *rx_len, u32_t rx_max_len);

s32_t cit_channel_t0_read_data(struct ch_handle *handle, u8_t *rx,
			       u32_t *rx_len, u32_t rx_max_len);

s32_t cit_channel_t1_read_data(struct ch_handle *handle, u8_t *rx,
			       u32_t *rx_len, u32_t rx_max_len);

s32_t cit_channel_sync_read_data(struct ch_handle *handle, u8_t *rx,
				 u32_t *rx_len, u32_t rx_max_len);

s32_t cit_channel_t0t1_transmit(struct ch_handle *handle, u8_t *tx,
				u32_t tx_len);

s32_t cit_channel_sync_transmit(struct ch_handle *handle, u8_t *tx,
				u32_t tx_len);

s32_t cit_channel_t14_irdeto_transmit(struct ch_handle *handle, u8_t *tx,
				      u32_t tx_len);

void cit_ch_cardinsert_cb(void *arg);

void cit_ch_cardremove_cb(void *arg);

void cit_ch_rcv_cb(void *arg);

void cit_ch_atr_cb(void *arg);

void cit_ch_wait_cb(void *arg);

void cit_ch_retry_cb(void *arg);

void cit_ch_timer_cb(void *arg);

void cit_ch_rparity_cb(void *arg);

s32_t cit_wait_timer_event(struct ch_handle *handle);

s32_t cit_wait_rcv(struct ch_handle *handle);

s32_t cit_channel_atr_byte_read(struct ch_handle *handle, u8_t *data,
				u32_t byte_time, u32_t max_time,
				u32_t *current_time);

s32_t cit_emv_atr_receive_decode(struct ch_handle *handle);

s32_t cit_iso_atr_receive_decode(struct ch_handle *handle);

s32_t cit_adjust_wwt(struct sc_channel_param *ch_param, u8_t wwi);

s32_t cit_channel_set_bwt_integer(struct sc_channel_param *ch_param, u32_t bwt);

s32_t cit_channel_reset_bwt(struct ch_handle *handle);

s32_t cit_channel_receive_atr(struct ch_handle *handle, u8_t *rx, u32_t *rx_len,
			      u32_t max_len);

s32_t sc_f_d_adjust_without_update(struct sc_channel_param *ch_param,
				   u8_t f_val, u8_t d_val);

s32_t cit_channel_set_card_type_character(struct ch_handle *handle,
					  u8_t *historical_bytes, u32_t len);

s32_t cit_set_card_type(struct ch_handle *handle, u32_t card_type);

s32_t cit_channel_apdu_set_objects(struct ch_handle *handle, s8_t *obj_id,
				   u32_t obj_id_len);

s32_t cit_channel_apdu_get_objects(struct ch_handle *handle, s8_t *obj_id,
				   u32_t obj_id_len);

s32_t cit_channel_apdu_commands(struct ch_handle *handle, u32_t apducmd,
				u8_t *buf, u32_t len);

s32_t sc_f_d_adjust(struct ch_handle *handle, u8_t f_val, u8_t d_val);

void channel_dump_param(const struct sc_channel_param  *ch_param);

void cit_ch_event2_cb(void *arg);

s32_t cit_channel_pps(struct ch_handle *handle);
/* STUBS */

#define reinitialize_function_table()
#define push_glb_regs()
#define pop_glb_regs()
#define check_and_wake_system()

#define CV_SUCCESS		0
#define CV_DIAG_FAIL		0x67

#endif /* SC_BCM58202_H__ */
