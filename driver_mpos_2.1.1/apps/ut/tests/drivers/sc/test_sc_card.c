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
 * @file test_sc_card.c
 * @brief Debit card related tests
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
#include <ztest.h>

#include "test_sc_infra.h"

#define APDU_MAGIC 0xF00D

u32_t app_card_debug_packet;
u32_t app_card_debug_flow;
/**
 * @brief   Information type enum
 * @details Application can read below information.
 */
enum apdu_info {
	INFO_LANGUAGE,
	INFO_CUSTOMER_NAME,
	INFO_CUSTOMER_ID,
	INFO_APP_TAG,
	INFO_APP_NAME,
	INFO_APP_ACCOUNT,
	INFO_TRACK_2,
	INFO_TRACK_1,
	INFO_BEGIN_DATE,
	INFO_END_DATE,
	INFO_TRANS_DATE,
	INFO_TRANS_TIME,
	INFO_TRANS_AUTH_AMOUNT,
	INFO_TRANS_OTHER_AMOUNT,
	INFO_TRANS_MERCHANT,
	INFO_TRANS_COUNTER,
	INFO_BALANCE,
};

/**
 * @brief   APDU context
 * @details Process card context.
 */
struct card_context {
	u32_t magic;
	struct  sc_cli_channel_handle *handle;
	u8_t format[64];
	u8_t format_length;
};

struct card_context ctx;

static s32_t apdu_debug_packet(char *name, u8_t *data, u32_t length)
{
	u32_t cnt;

	TC_PRINT("%s\n", name);
	TC_PRINT("----------------------------------------------\n");
	for (cnt = 0; cnt < length; cnt++) {
		TC_PRINT("0x%02x, ", data[cnt]);
		if ((cnt % 16) == 15)
			TC_PRINT("\n");
		else if (cnt == length - 1)
			TC_PRINT("\n");
	}
	TC_PRINT("----------------------------------------------\n");
	return 0;
}

static s32_t apdu_debug_char(char *name, u8_t *data, u32_t length)
{
	u32_t cnt;

	TC_PRINT("%s\n", name);
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	for (cnt = 0; cnt < length; cnt++)
		TC_PRINT("%c", data[cnt]);

	TC_PRINT("\n");
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	return 0;
}

static s32_t apdu_debug_hex(char *name, u8_t *data, u32_t length)
{
	u32_t cnt;

	TC_PRINT("%s\n", name);
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	for (cnt = 0; cnt < length; cnt++)
		TC_PRINT("%02x", data[cnt]);

	TC_PRINT("\n");
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	return 0;
}

static s32_t apdu_debug_money(char *name, u8_t *data, u32_t length)
{
	u32_t cnt;

	TC_PRINT("%s\n", name);
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	for (cnt = 0; cnt < length; cnt++) {
		if (cnt == (length - 1))
			TC_PRINT(".");

		TC_PRINT("%02x", data[cnt]);
	}
	TC_PRINT("\n");
	TC_PRINT("++++++++++++++++++++++++++++++++++++++++++++++\n");
	return 0;
}

s32_t apdu_init_ctx(struct card_context *ctx)
{
	s32_t rv = 0;

	memset(ctx, 0, sizeof(struct card_context));
	ctx->magic = APDU_MAGIC;
	return rv;
}

static s32_t apdu_output(struct card_context *ctx, enum apdu_info type,
			 u8_t *info, u8_t info_length)
{
	s32_t rv = 0;
	s32_t i = 0;

	ARG_UNUSED(ctx);
	switch (type) {
	case INFO_LANGUAGE:
			apdu_debug_char("Language:", info, info_length);
			break;

	case INFO_CUSTOMER_NAME:
			apdu_debug_char("Customer Name:", info,
					info_length);
			break;

	case INFO_CUSTOMER_ID:
			apdu_debug_char("Customer ID:", info,
					info_length);
			break;

	case INFO_APP_TAG:
			apdu_debug_char("Application TAG:", info,
					info_length);
			break;

	case INFO_APP_NAME:
			apdu_debug_char("Application Name:", info,
					info_length);
			break;

	case INFO_APP_ACCOUNT:
			apdu_debug_hex("Application Account:", info,
				       info_length);
			break;

	case INFO_TRACK_2:
			apdu_debug_hex("Track 2:", info, info_length);
			break;

	case INFO_TRACK_1:
			apdu_debug_char("Track 1:", info, info_length);
			break;

	case INFO_BEGIN_DATE:
			apdu_debug_hex("Begin Date:", info,
				       info_length);
			break;

	case INFO_END_DATE:
			apdu_debug_hex("End Date:", info, info_length);
			break;

	case INFO_BALANCE:
			apdu_debug_money("Balance:", info, info_length);
			break;

	case INFO_TRANS_DATE:
			TC_PRINT("Transaction Date: ");
			for (i = 0; i < info_length; i++)
				TC_PRINT("%02X", info[i]);

			TC_PRINT("\n");
			break;

	case INFO_TRANS_TIME:
			TC_PRINT("Transaction Time: ");
			for (i = 0; i < info_length; i++)
				TC_PRINT("%02X", info[i]);

			TC_PRINT("\n");
			break;

	case INFO_TRANS_AUTH_AMOUNT:
			TC_PRINT("Transaction Auth Amount: ");
			for (i = 0; i < info_length; i++) {
				if (i == (info_length - 1))
					TC_PRINT(".");

				TC_PRINT("%02X", info[i]);
			}
			TC_PRINT("\n");
			break;

	case INFO_TRANS_OTHER_AMOUNT:
			TC_PRINT("Transaction Other Amount: ");
			for (i = 0; i < info_length; i++) {
				if (i == (info_length - 1))
					TC_PRINT(".");

				TC_PRINT("%02X", info[i]);
			}
			TC_PRINT("\n");
			break;

	case INFO_TRANS_MERCHANT:
			TC_PRINT("Transaction Merchant: ");
			for (i = 0; i < info_length; i++)
				TC_PRINT("%c", info[i]);

			TC_PRINT("\n");
			break;

	case INFO_TRANS_COUNTER:
			TC_PRINT("Transaction Counter: ");
			for (i = 0; i < info_length; i++)
				TC_PRINT("%02X", info[i]);

			TC_PRINT("\n");
			break;

	default:
			TC_PRINT("Does not support type %d\n", type);
			break;

	}

	return rv;
}

static s32_t apdu_get_response(struct card_context *ctx, u8_t size,
			       u8_t *response)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	struct sc_transceive trx;
	u8_t sw1, sw2;
	u8_t cmd[SC_TPDU_HEADER_WITH_P3_SIZE] = {0};
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x00;
	cmd[SC_APDU_OFFSET_INS] = 0xC0;
	cmd[SC_APDU_OFFSET_P1] = 0x00;
	cmd[SC_APDU_OFFSET_P2] = 0x00;
	cmd[SC_APDU_OFFSET_LE] = size;

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];
	if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n", sw1, sw2);
		TC_PRINT("This card does not support get response!!!\n");
		rv = -EINVAL;
	}

	return rv;
}
/* pdol Processing Options Data Objects List */
static s32_t apdu_decode_file(struct card_context *ctx, u8_t *data, u8_t length,
			      u8_t *pdol, u8_t *pdol_length)
{
	s32_t rv = 0;
	u32_t count = 0;
	u8_t frame_length = 0;

	while (count < length) {
		if (data[count] == 0x50) {
			/*Application tag */
			frame_length = data[count + 1] + 2;
			apdu_output(ctx, INFO_APP_TAG, &data[count + 2],
				    data[count + 1]);
		} else if (data[count] == 0x5f && data[count + 1] == 0x2d) {
			/* Language */
			frame_length = data[count + 2] + 3;
			apdu_output(ctx, INFO_LANGUAGE, &data[count + 3],
				    data[count + 2]);
		} else if (data[count] == 0x6f) {
			/* FCI template */
			frame_length = 2;
		} else if (data[count] == 0x84) {
			/* DF name */
			frame_length = data[count + 1] + 2;
		} else if (data[count] == 0x87) {
			/* Application priority */
			frame_length = data[count + 1] + 2;
		} else if (data[count] == 0x88) {
			/* Directory SFI */
			frame_length = data[count + 1] + 2;
		} else if (data[count] == 0x9f && data[count + 1] == 0x11) {
			/* Bank code table */
			frame_length = data[count + 2] + 3;
		} else if (data[count] == 0x9f && data[count + 1] == 0x12) {
			/* Application name */
			frame_length = data[count + 2] + 3;
			apdu_output(ctx, INFO_APP_NAME, &data[count + 3],
				    data[count + 2]);
		} else if (data[count] == 0x9f && data[count + 1] == 0x38) {
			/* PDOL */
			frame_length = data[count + 2] + 3;
			memcpy(pdol, &data[count + 3], data[count + 2]);
			*pdol_length = data[count + 2];
			if (app_card_debug_flow)
				apdu_debug_packet("PDOL:", pdol, *pdol_length);
		} else if (data[count] == 0xa5) {
			/* FCI unique template */
			frame_length = 2;
		} else if (data[count] == 0xbf && data[count + 1] == 0x0c) {
			/* bank private FCI */
			frame_length = data[count + 2] + 3;
		} else {
			TC_PRINT("Unknown tag 0x%02x:0x%02x\n", data[count],
				 data[count + 1]);
			return -1;
		}
		if (app_card_debug_flow) {
			TC_PRINT("tag 0x%02x:0x%02x at %d\n", data[count],
				 data[count + 1], count);
			TC_PRINT("Frame length %d\n", frame_length);
		}
		count += frame_length;
	}

	return rv;
}

s32_t apdu_select_file_id(struct card_context *ctx, u8_t *dfi,
			  u8_t *pdol, u8_t *pdol_length)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	struct sc_transceive trx;
	u8_t sw1, sw2;
	u8_t cmd[SC_MAX_TX_SIZE] = {0};
	u8_t response[512] = { 0x00 };
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x00;
	cmd[SC_APDU_OFFSET_INS] = 0xa4;
	cmd[SC_APDU_OFFSET_P1] = 0x00;
	cmd[SC_APDU_OFFSET_P2] = 0x00;
	cmd[SC_APDU_OFFSET_LC] = 0x02;
	memcpy(&cmd[5], dfi, 0x02);

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE + 0x02;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	if (trx.rx_len < 2) {
		TC_PRINT("Invalid Rx length\n");
		return -1;
	}
	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];
	if (sw1 == 0x61) {
		if (app_card_debug_flow) {
			TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n",
								sw1, sw2);
			TC_PRINT(" needs to read further response...\n");
		}
		rv = apdu_get_response(ctx, sw2, response);
		if (rv) {
			TC_ERROR("apdu_get_response failed\n");
			return rv;
		}
	} else if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n", sw1, sw2);
		TC_PRINT("Card doesn't support select file id 0x%02x:0x%02x\n",
						dfi[0], dfi[1]);
		return -1;
	}
	rv = apdu_decode_file(ctx, response, sw2, pdol, pdol_length);
	if (rv)
		TC_ERROR("apdu_decode_file failed\n");

	return rv;
}

static s32_t apdu_select_file_name(struct card_context *ctx, u8_t *name,
				   u8_t length, u8_t *pdol, u8_t *pdol_length)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	struct sc_transceive trx;
	u8_t sw1, sw2;
	u8_t cmd[SC_MAX_TX_SIZE] = {0};
	u8_t response[512] = { 0x00 };
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x00;
	cmd[SC_APDU_OFFSET_INS] = 0xa4;
	cmd[SC_APDU_OFFSET_P1] = 0x04;
	cmd[SC_APDU_OFFSET_P2] = 0x00;
	cmd[SC_APDU_OFFSET_LC] = length;
	memcpy(&cmd[5], name, length);

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE + length;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	if (trx.rx_len < 2) {
		TC_PRINT("Invalid Rx length\n");
		return -1;
	}
	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];
	if (sw1 == 0x61) {
		if (app_card_debug_flow) {
			TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n",
								sw1, sw2);
			TC_PRINT(" needs to read further response...\n");
		}
		rv = apdu_get_response(ctx, sw2, response);
		if (rv) {
			TC_ERROR("apdu_get_response failed\n");
			return rv;
		}
	} else if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n",
								sw1, sw2);
		TC_PRINT("card does not support select file name\n");
		return -1;
	}
	rv = apdu_decode_file(ctx, response, sw2, pdol, pdol_length);
	if (rv)
		TC_ERROR("apdu_decode_file failed\n");

	return rv;
}

static s32_t apdu_get_format(struct card_context *ctx, u8_t length)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	u8_t response[512] = { 0x00 };
	struct sc_transceive trx;
	u8_t sw1, sw2;
	u8_t cmd[SC_TPDU_HEADER_WITH_P3_SIZE] = {0};
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x80;
	cmd[SC_APDU_OFFSET_INS] = 0xca;
	cmd[SC_APDU_OFFSET_P1] = 0x9f;
	cmd[SC_APDU_OFFSET_P2] = 0x4f;
	cmd[SC_APDU_OFFSET_LE] = length;

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	if (trx.rx_len < 2) {
		TC_PRINT("Invalid Rx length\n");
		return -1;
	}
	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];
	if (sw1 == 0x6c) {
		if (app_card_debug_flow) {
			TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n",
								sw1, sw2);
			TC_PRINT("Needs to read further response...\n");
		}
		rv = apdu_get_format(ctx, sw2);
	} else if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n", sw1, sw2);
		TC_PRINT("Get format failed!!!\n");
		rv = -EINVAL;
	} else {
		if (response[0] == 0x9f && response[1] == 0x4f) {
			memcpy(ctx->format, &response[3], response[2]);
			ctx->format_length = response[2];
		}
	}

	return rv;
}

static s32_t apdu_decode_log(struct card_context *ctx, u8_t *log,
			     u8_t log_length)
{
	s32_t rv = 0;
	u32_t log_count = 0;
	u8_t *format_pt = ctx->format;
	u8_t format_count = 0;

	if (ctx->format_length == 0) {
		rv = apdu_get_format(ctx, 0x1c);
		if (rv) {
			TC_ERROR("Get format failed\n");
			return rv;
		}
	}

	while (log_count < log_length && format_count < ctx->format_length) {
		if (format_pt[format_count] == 0x5f
		    && format_pt[format_count + 1] == 0x2a) {
			/* Currency code */
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9a) {
			/* Log date */
			apdu_output(ctx, INFO_TRANS_DATE, &log[log_count],
				    format_pt[format_count + 1]);
			log_count += format_pt[format_count + 1];
			format_count += 2;
		} else if (format_pt[format_count] == 0x9c) {
			/* Transaction type */
			log_count += format_pt[format_count + 1];
			format_count += 2;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x02) {
			/* Authorised amount */
			apdu_output(ctx, INFO_TRANS_AUTH_AMOUNT,
				    &log[log_count],
				    format_pt[format_count + 2]);
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x03) {
			/* Other Amount */
			apdu_output(ctx, INFO_TRANS_OTHER_AMOUNT,
				    &log[log_count],
				    format_pt[format_count + 2]);
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x1a) {
			/* Terminal country code */
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x21) {
			/* Log time */
			apdu_output(ctx, INFO_TRANS_TIME, &log[log_count],
				    format_pt[format_count + 2]);
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x36) {
			/* Transaction counter */
			apdu_output(ctx, INFO_TRANS_COUNTER, &log[log_count],
				    format_pt[format_count + 2]);
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else if (format_pt[format_count] == 0x9f
			   && format_pt[format_count + 1] == 0x4e) {
			/* Merchant name */
			apdu_output(ctx, INFO_TRANS_MERCHANT, &log[log_count],
				    format_pt[format_count + 2]);
			log_count += format_pt[format_count + 2];
			format_count += 3;
		} else {
			TC_PRINT("Unknown format 0x%02x:0x%02x\n",
				 format_pt[format_count],
				 format_pt[format_count + 1]);
			return -1;
		}
	}

	return rv;
}

static s32_t apdu_decode_record(struct card_context *ctx, u8_t *record,
				u8_t record_length)
{
	s32_t rv = 0;
	u32_t count = 0;
	u8_t frame_length = 0;

	while (count < record_length) {
		if (record[count] == 0x57) {
			/* Track 2 data */
			frame_length = record[count + 1] + 2;
			apdu_output(ctx, INFO_TRACK_2, &record[count + 2],
				    record[count + 1]);
		} else if (record[count] == 0x5a) {
			/* Application account */
			frame_length = record[count + 1] + 2;
			apdu_output(ctx, INFO_APP_ACCOUNT, &record[count + 2],
				    record[count + 1]);
		} else if (record[count] == 0x5f && record[count + 1] == 0x20) {
			/* Customer name */
			frame_length = record[count + 2] + 3;
			apdu_output(ctx, INFO_CUSTOMER_NAME, &record[count + 3],
				    record[count + 2]);
		} else if (record[count] == 0x5f && record[count + 1] == 0x24) {
			/* End date */
			frame_length = record[count + 2] + 3;
			apdu_output(ctx, INFO_END_DATE, &record[count + 3],
				    record[count + 2]);
		} else if (record[count] == 0x5f && record[count + 1] == 0x25) {
			/* Begin date */
			frame_length = record[count + 2] + 3;
			apdu_output(ctx, INFO_BEGIN_DATE, &record[count + 3],
				    record[count + 2]);
		} else if (record[count] == 0x5f && record[count + 1] == 0x28) {
			/* Country code */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x5f && record[count + 1] == 0x30) {
			/* Service code */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x5f && record[count + 1] == 0x34) {
			/* Application serial */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x8c) {
			/* CDOL1 */
			frame_length = record[count + 1] + 2;
		} else if (record[count] == 0x8d) {
			/* CDOL2 */
			frame_length = record[count + 1] + 2;
		} else if (record[count] == 0x8e) {
			/* CVM */
			frame_length = record[count + 1] + 2;
		} else if (record[count] == 0x90) {
			/* IPK cert */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x92) {
			/* IPK rem */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x93) {
			/* SAD */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x07) {
			/* Application control */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x08) {
			/* Application version */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x0d) {
			/* IAC */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x0e) {
			/* IAC deny */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x0f) {
			/* IAC connect */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x14) {
			/* Low limit */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x1f) {
			/* Track 1 data */
			frame_length = record[count + 2] + 3;
			apdu_output(ctx, INFO_TRACK_1, &record[count + 3],
				    record[count + 2]);
		} else if (record[count] == 0x9f && record[count + 1] == 0x23) {
			/* High limit */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x32) {
			/* IPK exp */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x46) {
			/* ICC cert */
			frame_length = record[count + 3] + 4;
		} else if (record[count] == 0x9f && record[count + 1] == 0x47) {
			/* ICC exp */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x48) {
			/* ICC rem */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x49) {
			/* DDOL */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x4a) {
			/* SDA list */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x61) {
			/* Customer ID number */
			frame_length = record[count + 2] + 3;
			apdu_output(ctx, INFO_CUSTOMER_ID, &record[count + 3],
				    record[count + 2]);
		} else if (record[count] == 0x9f && record[count + 1] == 0x62) {
			/* Customer ID type */
			frame_length = record[count + 2] + 3;
		} else if (record[count] == 0x9f && record[count + 1] == 0x63) {
			/* File ID */
			frame_length = record[count + 2] + 3;
		} else {
			TC_PRINT("Unknown tag 0x%02x:0x%02x\n", record[count],
				 record[count + 1]);
			return -1;
		}
		count += frame_length;
	}

	return rv;
}

static s32_t apdu_read_record(struct card_context *ctx, u8_t record_num,
			      u8_t control, u8_t length)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	struct sc_transceive trx;
	u8_t response[512] = { 0x00 };
	u8_t sw1, sw2;
	u8_t cmd[SC_TPDU_HEADER_WITH_P3_SIZE] = {0};
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x00;
	cmd[SC_APDU_OFFSET_INS] = 0xb2;
	cmd[SC_APDU_OFFSET_P1] = record_num;
	cmd[SC_APDU_OFFSET_P2] = control;
	cmd[SC_APDU_OFFSET_LE] = length;

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	if (trx.rx_len < 2) {
		TC_PRINT("Invalid Rx length\n");
		return -1;
	}
	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];
	if (sw1 == 0x6c) {
		if (app_card_debug_flow) {
			TC_PRINT("Response status SW1 0x%02X SW2 0x%02X,\n",
								sw1, sw2);
			TC_PRINT("needs to read further response...\n");
		}
		rv = apdu_read_record(ctx, record_num, control, sw2);
	} else if ((sw1 == 0x6a) && (sw2 == 0x83)) {
		TC_PRINT("There's no record %d control 0x%x!!!\n", record_num,
			 control);
		rv = -EINVAL;
	} else if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT("Response status SW1 0x%02X SW2 0x%02X\n", sw1, sw2);
		TC_PRINT("Read record %d control 0x%x failed!!!\n",
						record_num, control);
		rv = -EINVAL;
	} else {
		if (response[0] == 0x70 && response[1] == 0x81) {
			rv = apdu_decode_record(ctx, &response[3], response[2]);
		if (rv) {
			TC_ERROR("apdu_decode_record failed\n");
			return rv;
		}
		} else if (response[0] == 0x70) {
			rv = apdu_decode_record(ctx, &response[2], response[1]);
			if (rv) {
				TC_ERROR("apdu_decode_record failed\n");
				return rv;
			}
		} else {
			rv = apdu_decode_log(ctx, &response[0], length);
			if (rv) {
				TC_ERROR("apdu_decode_log failed\n");
				return rv;
			}
		}
	}

	return rv;
}

static s32_t apdu_get_balance(struct card_context *ctx, u8_t length)
{
	struct sc_cli_channel_handle *handle = ctx->handle;
	struct sc_transceive trx;
	u8_t response[512] = { 0x00 };
	u8_t sw1, sw2;
	u8_t cmd[SC_TPDU_HEADER_WITH_P3_SIZE] = {0};
	s32_t rv = 0;

	cmd[SC_APDU_OFFSET_CLA] = 0x80;
	cmd[SC_APDU_OFFSET_INS] = 0xca;
	cmd[SC_APDU_OFFSET_P1] = 0x9f;
	cmd[SC_APDU_OFFSET_P2] = 0x79;
	cmd[SC_APDU_OFFSET_LE] = length;

	trx.channel = handle->channel_no;
	trx.tx = cmd;
	trx.tx_len = SC_TPDU_HEADER_WITH_P3_SIZE;
	trx.rx = response;
	trx.rx_len = 0;
	trx.max_rx_len = SC_MAX_RX_SIZE;

	rv = sc_channel_transceive(handle->dev, SC_APDU, &trx);
	if (rv) {
		TC_PRINT("Couldn't transceive the message\n");
		return rv;
	}

	if (trx.rx_len < 2) {
		TC_PRINT("Invalid Rx length\n");
		return -1;
	}
	sw1 = response[trx.rx_len - 2];
	sw2 = response[trx.rx_len - 1];

	if (sw1 == 0x6c) {
		if (app_card_debug_flow) {
			TC_PRINT("Response status SW1 0x%02X SW2 0x%02X,\n",
								sw1, sw2);
			TC_PRINT("needs to read further response...\n");
		}
		rv = apdu_get_balance(ctx, sw2);
	} else if ((sw1 != 0x90) || (sw2 != 0x00)) {
		TC_PRINT
		    ("Response status SW1 0x%2X SW2 0x%02X get format failed\n",
		     sw1, sw2);
		rv = -1;
	} else {
		if (response[0] == 0x9f && response[1] == 0x79) {
			apdu_output(ctx, INFO_BALANCE, &response[3],
				    response[2]);
		}
	}

	return rv;
}

s32_t sc_cli_debit_card_test(struct  sc_cli_channel_handle *handle)
{
	struct card_context ctx;
	s32_t rv = 0;
	u8_t pboc_name[16] = {
		0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53,
		0x2e, 0x44, 0x44, 0x46, 0x30, 0x31
	};
	u8_t pboc_length = 0x0e;
	u8_t pdol[64] = { 0 };
	u8_t pdol_length = 0;
	u8_t i;

	ctx.format_length = 0;
	ctx.handle = handle;
	rv = apdu_select_file_name(&ctx, pboc_name, pboc_length, pdol,
				  &pdol_length);
	if (rv) {
		TC_ERROR("apdu_select_file_name failed\n");
		return rv;
	}
	for (i = 1; i < 3; i++) {
		rv = apdu_read_record(&ctx, i, 0x0c, 0x00);
		if (rv != 0)
			break;
	}
	apdu_read_record(&ctx, 1, 0x14, 0x00);
	for (i = 1; i < 4; i++) {
		rv = apdu_read_record(&ctx, i, 0x5c, 0x2d);
		if (rv != 0)
			break;
	}
	rv = apdu_get_balance(&ctx, 0x00);
	if (rv)
		TC_ERROR("apdu_get_balance failed\n");

	return rv;
}
