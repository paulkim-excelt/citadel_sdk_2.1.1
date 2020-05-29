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
 * @file sc_datatypes.h
 * @brief Smart card data types
 */

#ifndef SC_DATATYPES_H__
#define SC_DATATYPES_H__

#include <sc/sc_protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SC_NO_OF_CHANNELS					(2)
#define SC_ISO_WORK_WAIT_TIME_DEFAULT_FACTOR			(960)
#define SC_EMV2000_WORK_WAIT_TIME_DELTA				(1)
#define SC_ISO_DEFAULT_WORK_WAIT_TIME_INTEGER			(10)
#define SC_RLEN_9_BIT_MASK					(0x01ff)
/**
 * For EMV and ISO T=1, the minimum interval btw the leading edges of the start
 * bits of 2 consecutive characters sent in opposite directions shall be 22.
 */
#define SC_DEFAULT_BLOCK_GUARD_TIME				(22)

/**
 * For EMV T=0 only, the minimum interval btw the leading edges of the start
 * bits of 2 consecutive characters sent in opposite directions shall be 16.
 */
#define SC_MIN_DELAY_BEFORE_TZERO_SEND				(16)
#define SC_MAX_ETU_PER_ATR_BYTE					(9600)
#define SC_MAX_ETU_PER_ATR_BYTE_EMV2000				(10081)
#define SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES			(19200)
#define SC_MAX_EMV_ETU_FOR_ALL_ATR_BYTES_EMV2000		(20160)

/* Default EMV Information field size for T=1, used for IFSD defalut */
#define SC_DEFAULT_EMV_INFORMATION_FIELD_SIZE			(0x20)
#define SC_DEFAULT_IFSC						(0x20)
#define SC_MAX_RESET_IN_CLK_CYCLES				(42000)

/*  ATR timing */
#define SC_MAX_ATR_START_IN_CLK_CYCLES				(40000)
#define SC_EMV2000_MAX_ATR_START_IN_CLK_CYCLES			(42000)
#define SC_MIN_ATR_START_IN_CLK_CYCLES				(400)

/*
 * Delay between when ATR is sent from SmartCard
 * and when ATR_START bit get set. In micro secs
 */

#ifdef BESPVR
#define SC_ATR_START_BIT_DELAY_IN_CLK_CYCLES			(200)
#else
#define SC_ATR_START_BIT_DELAY_IN_CLK_CYCLES			(250)
#endif

#ifdef FPGA_CM3
#define SC_DELAY_MSEC_COUNTS					(525)
#else
#define SC_DELAY_MSEC_COUNTS					(230)
#endif

#define SC_DEFAULT_EXTERNAL_CLOCK_DIVISOR			1
#define SC_INTERNAL_CLOCK_FREQ_40MHz				40000000
#define SC_INTERNAL_CLOCK_FREQ_25MHz				25000000
#define SC_INTERNAL_CLOCK_FREQ			SC_INTERNAL_CLOCK_FREQ_40MHz

#ifndef CONFIG_SC_CH1_CLOCK_FREQ
#define CONFIG_SC_CH1_CLOCK_FREQ		SC_INTERNAL_CLOCK_FREQ
#endif

#ifndef CONFIG_SC_CH1_CLOCK_SOURCE
#define CONFIG_SC_CH1_CLOCK_SOURCE		SC_CLOCK_FREQ_SRC_INTERNAL
#endif

#define SC_DEFAULT_ETU_CLKDIV			(0x1)
#define SC_DEFAULT_SC_CLKDIV			(0xa)
#define SC_DEFAULT_IF_CLKDIV			(0x0)	/* 0 working;1doesn't */
#define SC_DEFAULT_PRESCALE			(0x77)
#define SC_MAX_PRESCALE				(0xFF)
#define SC_DEFAULT_BAUD_DIV			(31)

#define SC_DEFAULT_F_FACTOR			(1)
#define SC_MIN_F_FACTOR				(1)
#define SC_MAX_F_FACTOR				(13)

#define SC_DEFAULT_D_FACTOR			(1)
#define SC_MIN_D_FACTOR				(1)
#define SC_MAX_D_FACTOR				(9)

#define SC_DEFAULT_TX_PARITY_RETRIES		(5)
#define SC_MAX_TX_PARITY_RETRIES		(7)
#define SC_MAX_RX_PARITY_RETRIES		(7)
#define SC_DEFAULT_RX_PARITY_RETRIES		(5)

#define SC_EMV2000_DEFAULT_TX_PARITY_RETRIES	(4)
#define SC_EMV2000_DEFAULT_RX_PARITY_RETRIES	(4)

#define SC_DEFAULT_WORK_WAITING_TIME		(9600)
#define SC_DEFAULT_EXTRA_WORK_WAITING_TIME_EMV2000	(480)

/* Default block waiting time in ETUs = 11 + 960 etus, where bwi = 0 */
#define SC_DEFAULT_BLOCK_WAITING_TIME			(971)
#define SC_DEFAULT_EXTRA_BLOCK_WAITING_TIME_EMV2000	(960)
#define SC_DEFAULT_EXTRA_GUARD_TIME			(2)

/*
 * For EMV and ISO T=1, the minimum interval btw the leading edges
 * of the start bits of 2 consecutive characters sent in opposite
 * directions shall be 22 ETUs.
 */
#define SC_DEFAULT_BLOCK_GUARD_TIME			(22)
#define SC_MIN_BLOCK_GUARD_TIME				(12)
#define SC_MAX_BLOCK_GUARD_TIME				(31)

#define SC_DEFAULT_EDC_ENCODE_LRC			(0)

#define SC_DEFAULT_CHARACTER_WAIT_TIME_INTEGER		(0)
#define SC_CHARACTER_WAIT_TIME_GRACE_PERIOD		(1)
#define SC_MAX_CHARACTER_WAIT_TIME_INTEGER		(15)

#define SC_DEFAULT_TIME_OUT				(13000)
#define SC_RESET_BLOCK_WAITING_TIME			(0xffff)

/* T14 Irdeto clock Rate Conversion factor.  Spec says 625, card prefers 600 */
#define SC_T14_IRDETO_CONSTANT_CLOCK_RATE_CONV_FACTOR		(600)

/* Irdeto needs to wait for minimum of 1250 from last RX to the next TX  */
#define SC_T14_IRDETO_MIN_DELAY_RX2TX			(1250)

#define SC_MAX_CALLBACK_FUNC				(2)
#define SC_MAX_TX_SIZE					(264)
#define SC_MAX_RX_SIZE					(264)
#define SC_MAX_EMV_BUFFER_SIZE				(258)

#define SC_DEFAULT_DB_WIDTH				(0x07) /* ~ 10 ms */
#define SC_MAX_DB_WIDTH					(0x3F)

/**
 * Definition to be used in member ulStatus1 of struct SC_Status
 * Either one of the following defined values shall be set to ulStatus1.
 * Use OR if you have more than one.
 */
#define SC_RX_PARITY				(0x0800)
#define SC_TX_PARITY				(0x0400)
#define SC_RX_TIMEOUT				(0x0200)
#define SC_TX_TIMEOUT				(0x0100)
#define SC_EDC_ERROR				(0x0008)
#define SC_HARDWARE_FAILURE			(0x0004)
#define SC_RESET_CHANNEL_REQUIRED		(0x0002)

/* APDU Commands */
#define SC_APDU_PIN_VERIFY			(0x0001)
#define SC_APDU_GET_CHALLENGE			(0x0002)
#define SC_APDU_GET_RESPONSE			(0x0004)
#define SC_APDU_SELECT_FILE			(0x0008)
#define SC_APDU_READ_DATA			(0x0010)
#define SC_APDU_WRITE_DATA			(0x0020)
#define SC_APDU_DELETE_FILE			(0x0040)

/*
 * @brief Protocol types
 */
enum sc_protocol_type {
	SC_ASYNC_PROTOCOL_UNKNOWN,	/* Initial value */
	SC_ASYNC_PROTOCOL_E0,		/* Character-oriented asynchronous*/
	SC_ASYNC_PROTOCOL_E1,		/* Block-oriented asynchronous */
	SC_SYNC_PROTOCOL_E0,		/* Type 1 synchronous */
	SC_SYNC_PROTOCOL_E1,		/* Type 2 synchronous */
	SC_ASYNC_PROTOCOL_E14_IRDETO	/* Irdeto T=14 proprietary */
};

/*
 * @brief smart card type
 */
enum sc_card_type {
	SC_CARD_TYPE_UNKNOWN,	/* Initial value */
	SC_CARD_TYPE_ACOS,	/* ACOS smart card by Advanced Card Systems */
	SC_CARD_TYPE_JAVA,	/* Dell Java card */
	SC_CARD_TYPE_PKI,	/* PKI card */
	SC_CARD_TYPE_PIV,	/* PIV card */
	SC_CARD_TYPE_CAC	/* CAC card */
};

enum sc_clock_freq_src {
	SC_CLOCK_FREQ_SRC_UNKNOWN,
	SC_CLOCK_FREQ_SRC_INTERNAL,
	SC_CLOCK_FREQ_SRC_EXTERNAL
};

/*
 * @brief smart card standard
 */
enum sc_standard {
	SC_STANDARD_UNKNOWN = 0,
	SC_STANDARD_NDS,
	SC_STANDARD_ISO,
	SC_STANDARD_EMV1996,
	SC_STANDARD_EMV2000,
	SC_STANDARD_IRDETO,
	SC_STANDARD_ARIB,
	SC_STANDARD_MT,
	SC_STANDARD_CONAX,
	SC_STANDARD_ES
};

/**
 * @brief Reset type
 */
enum sc_reset_type {
	SC_COLD_RESET,
	SC_WARM_RESET

};

/**
 * @brief Power type
 */
enum sc_power_icc {
	SC_ICC_POWER_UP = 0,
	SC_ICC_POWER_DOWN
};

/**
 * @brief Voltage level
 */
enum sc_vcc_level {
	SC_VCC_LEVEL_5V,
	SC_VCC_LEVEL_3V,
	SC_VCC_LEVEL_1_8V
};

/*
 * @brief Action for sc_channel_reset_card function.
 */
enum sc_reset_card_action {
	/* Only Reset the card */
	SC_RESET_CARD_NO_ACTION = 0,
	/* Decode ATR */
	SC_RESET_CARD_ATR_DECODE
};

/*
 * @brief Unit of timer value
 */
enum sc_time_unit {
	SC_TIMER_UNIT_ETU,	/* in Elementary Time Units */
	SC_TIMER_UNIT_CLK,	/* in raw clock cycles*/
	SC_TIMER_UNIT_MSEC	/* in milli seconds */
};

/*
 * @brief card staus
 */
enum sc_card_present {
	SC_CARD_REMOVED = 0,
	SC_CARD_INSERTED
};

/**
 * @brief clock
 *
 * @param external True if external clock
 * @param clk_frequency clock
 */
struct icc_clock {
	u8_t external;
	u32_t clk_frequency;
};

/**
 * @brief time value
 *
 * @param val timer value
 * @param unit ETU or milli sec or clock
 */
struct sc_time {
	u32_t val;
	u8_t unit;
};

/**
 * @brief EDC setting for T=1 protocol only
 *
 * @param enable Enable
 * @param crc True if using CRC or it will be LRC
 */
struct edc_setting {
	u8_t enable;
	u8_t crc;
};

/**
 * @brief debounce info
 *
 * @param sc_mask True if mask mode else debounce mode
 * @param enable Enable
 * @param db_Width Db width [5:0]
 */
struct sc_pres_dbinfo {
	u8_t sc_mask;
	u8_t enable;
	u8_t db_width;
};

/* Card characterics */
/* JAVA card specific */
struct sc_java_ctx {
	u16_t private_data_size;
	u16_t public_data_size;
};

struct sc_acos_user_file {
	u8_t  record_len;
	u8_t  record_no;
	u8_t  read_attr;
	u8_t  write_attr;
	u16_t file_id;
};

/* ACOS card specific */
enum phx_sc_card {
	SC_ACOS2,
	SC_ACOS3,
	SC_NON_ACOS
};

struct sc_acos_ctx {
	enum phx_sc_card card;
	u32_t card_stage;
	u8_t reg_option;
	u8_t reg_security;
	u8_t no_of_files;
	u8_t personalized;   /* if the card has personalization done or not */
	u8_t serial_num_string[8];    /* card serial number */
	u32_t record_max_len;         /* user file max record length */
	u32_t record_max_num;         /* user file max record number */
	u8_t  code_no;                /* Code reference */
	struct sc_acos_user_file user_file;    /* User data file */
};

#define SC_MAX_OBJ_ID_STRING  20
#define SC_MAX_OBJ_ID_SIZE  5
struct sc_card_object_id {
	u8_t  id_num;
	u8_t  id_string[SC_MAX_OBJ_ID_STRING];
};

struct sc_card_type_ctx {
	enum sc_card_type card_type;   /* Type of card */
	enum sc_vcc_level vcc_level;   /* Vcc voltage level */
	u32_t inited;
	union {
		struct sc_java_ctx java_ctx;
		struct sc_acos_ctx acos_ctx;
	} card_char;
	u8_t current_id;
	struct sc_card_object_id obj_id[SC_MAX_OBJ_ID_SIZE];
	u8_t hist_bytes[SC_MAX_ATR_HISTORICAL_SIZE];
};

/**
 * @brief smart card type
 *
 * @param card_type card type
 * @param card_id_length Length used to identify card
 * @param hist_bytes Historical bytes
 */
struct sc_card_type_settings {
	enum sc_card_type card_type;
	u32_t  card_id_length;
	u8_t hist_bytes[SC_MAX_ATR_HISTORICAL_SIZE];
};

/**
 * @brief Transceive information
 *
 * @param channel Channel for transceive
 * @param tx Transmit buffer
 * @param tx_len Transmit length
 * @param rx Receive buffer
 * @param rx_len receive length
 * @param max_rx_len Max length of receive buffer
 *
 */

struct sc_transceive {
	u32_t channel;
	u8_t *tx;
	u32_t tx_len;
	u8_t *rx;
	u32_t rx_len;
	u32_t max_rx_len;
};

struct sc_apdu {
	u8_t cla;
	u8_t ins;
	u8_t p1;
	u8_t p2;
	u16_t lc;
	u16_t le;
	u8_t *data;
	u8_t sw[2];
};

/**
 * @brief Card status
 *
 * @param card_present
 * @param card_activate
 * @param pps_done
 */
struct sc_status {
	u8_t card_present;
	u8_t card_activate;
	u8_t pps_done;
	u32_t status1;
	u32_t status2;
};

enum param_type {
	CURRENT_SETTINGS,
	ATR_NEG_SETTINGS,
	DEFAULT_SETTINGS
};

enum pdu_type {
	SC_APDU,
	SC_TPDU
};

enum time_type {
	WORK_WAIT_TIME,
	BLK_WAIT_TIME,
	EXTRA_GUARD_TIME,
	BLK_GUARD_TIME,
	BLK_WAIT_TIME_EXT
};

enum t1_states {
	SC_T1_TX,
	SC_T1_RX,
	SC_T1_RESYNC
};

/**
 * @brief host controller clock settings
 *
 * @param etu_clkdiv ETU clock divider
 * @param sc_clkdiv Sc clock devider
 * @param prescale Prescale value
 * @param bauddiv Baud divisor
 */
struct host_ctrl_settings {
	u8_t etu_clkdiv;
	u8_t sc_clkdiv;
	u32_t prescale;
	u8_t bauddiv;
};

/**
 *
 * @brief Settings structure for smart card channel.
 *
 * @param standard Smart Card Standard
 * @param protocol Asynchronous Protocol Types
 * @param ctx_card_type Card characteristics
 * @param src_clk_freq
 * @param icc_clk_freq Current ICC CLK frequency
 * @param baudrate Current baudrat
 * @param max_ifsd Specifies the maximum IFSD. Should be 264.
 * @param ifsd Current information field size for the interface device
 * @param f_factor Clock rate conversion factor F
 * @param d_factor Baudrate adjustment factor D
 * @param ext_clk_div External clock divisor for TDA chips
 * @param tx_retries Number of transmit parity retries
 * @param rx_retries Number of receive parity retries
 * @param work_wait_time Work waiting time
 * @param block_wait_time Block Wait time
 * @param extra_guard_time Extra Guard Time
 * @param block_guard_time block Guard time
 * @param cwi Character wait time integer
 * @param edc EDC encoding
 * @param timeout Time Out value in milli seconds for any sync transaction
 * @param auto_deactive_req Need auto deactivation sequence
 * @param null_filter True if we receive 0x60 in T=0, we will ignore it.
 *
 * @param ifsc Current information field size for the card
 *
 */
struct sc_channel_param {
	enum sc_standard standard;
	enum sc_protocol_type protocol;
	struct sc_card_type_ctx ctx_card_type;
	u32_t icc_clk_freq;
	u32_t baudrate;
	u32_t max_ifsd;
	u32_t ifsd;
	u8_t f_factor;
	u8_t d_factor;
	u8_t ext_clkdiv;
	u8_t tx_retries;
	u8_t rx_retries;
	struct sc_time work_wait_time;
	struct sc_time blk_wait_time;
	struct sc_time extra_guard_time;
	struct sc_time blk_guard_time;
	u32_t cwi;
	struct edc_setting edc;
	struct sc_time timeout;
	u8_t auto_deactive_req;
	u8_t null_filter;
	struct sc_pres_dbinfo debounce;
	u8_t read_atr_on_reset;
	struct sc_time blk_wait_time_ext;
	u8_t btpdu; /* True if packet is in TPDU rather than APDU */
	struct host_ctrl_settings host_ctrl;
	u8_t ifsc;
};

#ifdef __cplusplus
}
#endif

#endif /* SC_DATATYPES_H__ */
