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
 * @file sc_acos.c
 * @brief Commands specific to ACOS2 and ACOS3 SmartCard
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

#include "sc_acos2.h"

/*
 * the Submit Code command
 * code points to a buffer of 8 bytes
 * fixme: do we want to return the remaining try time?
 */
s32_t cit_acos2_submit_code(struct ch_handle *handle, u8_t code_ref, u8_t *code)
{
	u8_t rx_buf[SC_MAX_RX_SIZE];
	u8_t cmd[ACOS_SUBMIT_CODE_CMDLEN] = {
		ACOS_CLA, ACOS_INS_SUBMIT_CODE, 0x0, 0x00,
		ACOS_SUBMIT_CODE_CODELEN, /* rest is code */
	};
	struct sc_transceive tx_rx;
	s32_t rv;

	cmd[ACOS_SUBMIT_CODE_IDX_CODE_REF] = code_ref;
	memcpy(&cmd[ACOS_SUBMIT_CODE_IDX_CODE], code,
		ACOS_SUBMIT_CODE_CODELEN);

	/* Submit code command */
	tx_rx.channel = handle->ch_number;
	tx_rx.tx = cmd;
	tx_rx.tx_len = sizeof(cmd);
	tx_rx.rx = rx_buf;
	tx_rx.rx_len = 0;
	tx_rx.max_rx_len = SC_MAX_RX_SIZE;
	rv = cit_tpdu_transceive(handle, &tx_rx);
	if (rv)
		return rv;

	/* now check the return value */
	if (SC_RESP_STATUS_OK(rx_buf))
		return 0;
	else if (SC_RESP_VERIFY_FAIL(rx_buf))
		return ACOS_SUBMIT_CODE_RET_WRONG_CODE;
	else if (SC_RESP_AUTH_METHOD_BLOCKED(rx_buf))
		return ACOS_SUBMIT_CODE_RET_CODE_LOCKED;
	else if (SC_RESP_COND_NOT_SATISFIED(rx_buf))
		return ACOS_SUBMIT_CODE_RET_AUTH_NEEDED;

	return ACOS_CMD_RET_UNKNOWN;
}

/* the Select File command file_id is the 2 bytes file ID */
static s32_t cit_acos_select_file(struct ch_handle *handle, u16_t file_id)
{
	struct sc_transceive tx_rx;
	u8_t rx_buf[SC_MAX_RX_SIZE];
	u8_t cmd[ACOS_SELECT_FILE_CMDLEN] = {
		ACOS_CLA, ACOS_INS_SELECT_FILE, 0x00, 0x00, 0x02
	};
	s32_t rv;

	cmd[ACOS_SELECT_FILE_IDX_FILE_ID] = (file_id >> 8);
	cmd[ACOS_SELECT_FILE_IDX_FILE_ID + 1] = (file_id & 0xFF);

	/* Select file command */
	tx_rx.channel = handle->ch_number;
	tx_rx.tx = cmd;
	tx_rx.tx_len = sizeof(cmd);
	tx_rx.rx = rx_buf;
	tx_rx.rx_len = 0;
	tx_rx.max_rx_len = SC_MAX_RX_SIZE;
	rv = cit_tpdu_transceive(handle, &tx_rx);
	if (rv)
		return rv;

	/* now check the return value */
	if (SC_RESP_STATUS_OK(rx_buf))
		return 0;
	else if (SC_RESP_FILE_NOT_FOUND(rx_buf))
		return -ENOENT;
	else if (rx_buf[0] == 0x91)
		return ACOS_SELECT_FILE_RET_FILE_SELECTED;

	return ACOS_CMD_RET_UNKNOWN;
}

/*
 * the Read Record command
 * it will read len bytes to p_buf, from the record number of record_no
 */
static s32_t cit_acos_read_record(struct ch_handle *handle, u8_t record_no,
				  u8_t len, u8_t *p_buf)
{
	struct sc_transceive tx_rx;
	u8_t rx_buf[SC_MAX_RX_SIZE];
	u8_t tmp_val[2];
	u8_t cmd[ACOS_READ_RECORD_CMDLEN] = {
		ACOS_CLA, ACOS_INS_READ_RECORD, 0x00 /*record no. */, 0x00,
	};
	s32_t rv;

	cmd[ACOS_READ_RECORD_IDX_RECORD_NO] = record_no;
	cmd[ACOS_READ_RECORD_IDX_LEN] = len;

	/* Read Record command */
	tx_rx.channel = handle->ch_number;
	tx_rx.tx = cmd;
	tx_rx.tx_len = sizeof(cmd);
	tx_rx.rx = rx_buf;
	tx_rx.rx_len = 0;
	tx_rx.max_rx_len = SC_MAX_RX_SIZE;
	rv = cit_tpdu_transceive(handle, &tx_rx);
	if (rv)
		return rv;

	tmp_val[1] = rx_buf[tx_rx.rx_len - 1];
	tmp_val[0] = rx_buf[tx_rx.rx_len - 2];

	/* now check the return value */
	if (SC_RESP_STATUS_OK(tmp_val)) {
		memcpy(p_buf, rx_buf, len);
		return 0;
	} else if (SC_RESP_NO_SECURITY_CODE(rx_buf)) {
		return ACOS_READ_RECORD_RET_NO_SECURITY_CODE;
	} else if (SC_RESP_RECORD_NOT_FOUND(rx_buf)) {
		return ACOS_READ_RECORD_RET_RECORD_NOT_FOUND;
	} else if (SC_RESP_WRONG_LENGTH(rx_buf)) {
		return ACOS_READ_RECORD_RET_LEN_TOO_LARGE;
	} else if (SC_RESP_NO_FILE_SELECTED(rx_buf)) {
		return -ACOS_READ_RECORD_RET_NO_FILE_SELECTED;
	} else if (SC_RESP_COMMAND_ABORTED(rx_buf)) {
		return ACOS_READ_RECORD_RET_IO_ERROR;
	}

	return ACOS_CMD_RET_UNKNOWN;
}

/* the Write Record command
 * it will write len bytes from p_buf to the card,
 * to the record number of record_no
 */
static s32_t cit_acos_write_record(struct ch_handle *handle, u8_t record_no,
				   u8_t len, u8_t *p_buf)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	struct sc_transceive tx_rx;
	s32_t rv;
	u8_t rx_buf[SC_MAX_RX_SIZE];
	u8_t cmd[ACOS_WRITE_RECORD_CMDLEN] = {
		ACOS_CLA, ACOS_INS_WRITE_RECORD, 0x00 /*record no. */, 0x00,
	};
	u32_t record_max_len = ctx_card->card_char.acos_ctx.record_max_len;

	if (len > record_max_len)
		return ACOS_WRITE_RECORD_RET_BUF_TOO_BIG;

	cmd[ACOS_READ_RECORD_IDX_RECORD_NO] = record_no;
	cmd[ACOS_READ_RECORD_IDX_LEN] = len;
	memcpy(&cmd[ACOS_WRITE_RECORD_IDX_DATA], p_buf, len);

	/* Write Record command */
	tx_rx.channel = handle->ch_number;
	tx_rx.tx = cmd;
	tx_rx.tx_len = len + SC_T0_CMDHDR_LEN;
	tx_rx.rx = rx_buf;
	tx_rx.rx_len = 0;
	tx_rx.max_rx_len = SC_MAX_RX_SIZE;
	rv = cit_tpdu_transceive(handle, &tx_rx);
	if (rv)
		return rv;

	/* now check the return value */
	if (SC_RESP_STATUS_OK(rx_buf))
		return 0;
	else if (SC_RESP_NO_SECURITY_CODE(rx_buf))
		return ACOS_READ_RECORD_RET_NO_SECURITY_CODE;
	else if (SC_RESP_RECORD_NOT_FOUND(rx_buf))
		return ACOS_READ_RECORD_RET_RECORD_NOT_FOUND;
	else if (SC_RESP_WRONG_LENGTH(rx_buf))
		return ACOS_READ_RECORD_RET_LEN_TOO_LARGE;
	else if (SC_RESP_NO_FILE_SELECTED(rx_buf))
		return ACOS_READ_RECORD_RET_NO_FILE_SELECTED;
	else if (SC_RESP_COMMAND_ABORTED(rx_buf))
		return ACOS_READ_RECORD_RET_IO_ERROR;

	return ACOS_CMD_RET_UNKNOWN;
}

/*
 * get ACOS card personalization info
 * (e.g. value returns from personalization file)
 */
static s32_t cit_acos_get_personalization_info(struct ch_handle *handle)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	s32_t rv;
	u8_t tmpstring[ACOS_RECLEN_PERSONALIZATION];

	rv = cit_acos_select_file(handle, ACOS_FID_PERSONALIZATION);
	if (rv)
		return rv;

	WAIT();
	rv = cit_acos_read_record(handle, 0, ACOS_RECLEN_PERSONALIZATION,
				 tmpstring);
	if (rv)
		return rv;

	ctx_card->card_char.acos_ctx.reg_option = tmpstring[0];
	ctx_card->card_char.acos_ctx.reg_security = tmpstring[1];
	ctx_card->card_char.acos_ctx.no_of_files = tmpstring[2];
	ctx_card->card_char.acos_ctx.personalized =
	    (tmpstring[3] & 0x80) ? true : false;

	return 0;
}

/*
 * to get a user file info (number of records, record length, read/write
 * security attribute, file ID)
 * input is the file number (e.g. 0, 1, 2, ... till number of files-1 )
 */
s32_t cit_acos_get_user_file_info(struct ch_handle *handle, u8_t file_number,
				  u8_t *record_len, u8_t *record_no,
				  u8_t *read_attr, u8_t *write_attr,
				  u16_t *file_id)
{
	u8_t tmpstring[ACOS_RECLEN_USERFILE_MNG];
	s32_t rv;

	/*
	 * select user management file, to get each user file record
	 * length and number of records
	 */
	rv = cit_acos_select_file(handle, ACOS_FID_USERFILE_MNG);
	if (rv)
		return rv;

	WAIT();
	rv = cit_acos_read_record(handle, file_number,
				 ACOS_RECLEN_USERFILE_MNG, tmpstring);
	if (rv)
		return rv;

	*record_len = tmpstring[0];
	*record_no = tmpstring[1];
	*read_attr = tmpstring[2];
	*write_attr = tmpstring[3];
	*file_id = ((u16_t) tmpstring[4] << 8) | tmpstring[5];

	return 0;
}

/*
 * to create a user file (number of records, record length, read/write security
 * attribute, file ID) input is the file number (e.g. 0, 1, 2, ... till number
 * of files-1 ) and all the related file attributes
 * We always use max record length.
 * NOTE:
 * 1. must submit code before this operation
 * 2. will check the personalization file, set the number of file to 1 if it's
 *    0, otherwise don't change it.
 */
s32_t cit_acos_create_user_file(struct ch_handle *handle, u8_t file_number,
				u32_t file_len,	u8_t read_attr, u8_t write_attr,
				 u16_t file_id)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	s32_t rv;
	u8_t tmpstring[ACOS_RECLEN_USERFILE_MNG];

	/* check the size */
	if (file_len > (ctx_card->card_char.acos_ctx.record_max_len
			* ctx_card->card_char.acos_ctx.record_max_num))
		return -EPERM;

	rv = cit_acos_get_personalization_info(handle);
	if (rv)
		return rv;

	if (ctx_card->card_char.acos_ctx.no_of_files == 0) {
		/*fixme: error code here, since cannot write to it anymore*/
		if (ctx_card->card_char.acos_ctx.personalized)
			return 1;

		tmpstring[0] = ctx_card->card_char.acos_ctx.reg_option;
		tmpstring[1] = ctx_card->card_char.acos_ctx.reg_security;
		tmpstring[2] = 1;	/* number of files */
		tmpstring[3] = 0;

		WAIT();
		rv = cit_acos_write_record(handle, 0,
				 ACOS_RECLEN_PERSONALIZATION, tmpstring);
		if (rv)
			return rv;

		ctx_card->card_char.acos_ctx.no_of_files = 1;
	}

	WAIT();
	rv = cit_acos_select_file(handle, ACOS_FID_USERFILE_MNG);
	if (rv)
		return rv;

	WAIT();
	tmpstring[0] = ctx_card->card_char.acos_ctx.record_max_len;
	tmpstring[1] =
	    (file_len + ctx_card->card_char.acos_ctx.record_max_len - 1)
	    / ctx_card->card_char.acos_ctx.record_max_len;
	tmpstring[2] = read_attr;
	tmpstring[3] = write_attr;
	tmpstring[4] = file_id >> 8;
	tmpstring[5] = file_id & 0x00FF;

	rv = cit_acos_write_record(handle, file_number,
				 ACOS_RECLEN_USERFILE_MNG, tmpstring);
	if (rv)
		return rv;

	return 0;
}

/* Will read multiple records of file */
s32_t cit_acos2_read_file(struct ch_handle *handle, u16_t file_id,
			  u32_t read_len, u8_t *p_buf)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	u8_t record_read_len, record_no;
	s32_t rv;

	/* Select file command */
	rv = cit_acos_select_file(handle, file_id);
	if (rv != ACOS_SELECT_FILE_RET_FILE_SELECTED)
		return rv;

	/* read multiple records */
	for (record_no = 0;; record_no++) {
		WAIT();

		record_read_len =
		(read_len >
		     ctx_card->card_char.acos_ctx.record_max_len)
		    ? ctx_card->card_char.acos_ctx.
		    record_max_len : (u8_t) read_len;

		rv = cit_acos_read_record(handle, record_no, record_read_len,
							 p_buf);
		if (rv)
			return rv;

		if (read_len <= record_read_len)
			break;

		read_len -= record_read_len;
		p_buf += record_read_len;
	}

	return 0;
}

/*
 * The Write File command
 * it will write to multiple records
 */
s32_t cit_acos2_write_file(struct ch_handle *handle, u16_t file_id,
			   u32_t write_len, u8_t *p_buf)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	s32_t rv;
	u8_t record_write_len, record_no;

	rv = cit_acos_select_file(handle, file_id);
	if (rv != ACOS_SELECT_FILE_RET_FILE_SELECTED)
		return rv;

	/* write multiple records */
	for (record_no = 0;; record_no++) {
		WAIT();
		record_write_len =
		    (write_len >
		     ctx_card->card_char.acos_ctx.record_max_len)
		    ? ctx_card->card_char.acos_ctx.
		    record_max_len : (u8_t) write_len;

		rv = cit_acos_write_record(handle, record_no, record_write_len,
						 p_buf);
		if (rv)
			return rv;

		if (write_len <= record_write_len)
			break;

		write_len -= record_write_len;
		p_buf += record_write_len;
	}

	return 0;
}

/*
 * get ACOS card information
 * called after ATR is done
 */
s32_t cit_acos_get_card_info(struct ch_handle *handle)
{
	struct sc_card_type_ctx *ctx_card = &handle->ch_param.ctx_card_type;
	s32_t rv;
	u8_t tmpstring[ACOS_RECLEN_MCU_ID];

	/*
	 * check and assign ACOS context values
	 * the historical bytes have meaning, refer to P36 of ACOS2 manual
	 */
	if (ctx_card->hist_bytes[0] != 0x41)
		return -ENOTSUP; /*NOT ACOS card */

	/* 0: user stage, 1: manufacturing, 2: personalization */
	ctx_card->card_char.acos_ctx.card_stage
	    = ctx_card->hist_bytes[11];
	ctx_card->card_char.acos_ctx.no_of_files
	    = ctx_card->hist_bytes[5];
	ctx_card->card_char.acos_ctx.personalized
	    = (ctx_card->hist_bytes[9] & 0x80) ? true : false;

	rv = cit_acos_get_personalization_info(handle);
	if (rv)
		return rv;

	WAIT();
	rv = cit_acos_select_file(handle, ACOS_FID_MCU_ID);
	if (rv)
		return rv;

	WAIT();
	/* read card serial number, this is the first record of MCU file */
	rv = cit_acos_read_record(handle, 0, ACOS_RECLEN_MCU_ID,
				  ctx_card->card_char.acos_ctx.
				  serial_num_string);
	if (rv)
		return rv;

	/* read card name, this is the second record of MCU file */
	rv = cit_acos_read_record(handle, 1, ACOS_RECLEN_MCU_ID, tmpstring);
	if (rv)
		return rv;

	if ((!strncmp((char *)tmpstring, "ACOS", 4)) && (tmpstring[4] == 3)) {
		ctx_card->card_char.acos_ctx.card = SC_ACOS3;
		ctx_card->card_char.acos_ctx.record_max_len =
		    ACOS3_USERFILE_MAX_RECORD_LENGTH;
		ctx_card->card_char.acos_ctx.record_max_num =
		    ACOS3_USERFILE_MAX_RECORD_NUMBER;
	} else {		/* ACOS2 does not have (tmpstring[4] == 2)!!! */

		ctx_card->card_char.acos_ctx.card = SC_ACOS2;
		ctx_card->card_char.acos_ctx.record_max_len =
		    ACOS2_USERFILE_MAX_RECORD_LENGTH;
		ctx_card->card_char.acos_ctx.record_max_num =
		    ACOS2_USERFILE_MAX_RECORD_NUMBER;
	}

	return 0;
}
