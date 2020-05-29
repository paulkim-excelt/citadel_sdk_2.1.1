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
 * @file sc_acos.h
 * @brief ACOS smart card header file
 */

#ifndef __SC_ACOS_H
#define __SC_ACOS_H

/* FILE IDs */
/* These are internal data files. User file ID does not start with FF */
#define ACOS_FID_MCU_ID			0xFF00
#define ACOS_FID_MANUFACTURER		0xFF01
#define ACOS_FID_PERSONALIZATION	0xFF02
#define ACOS_FID_SECURITY		0xFF03
#define ACOS_FID_USERFILE_MNG		0xFF04 /* user file management file */
#define ACOS_FID_ACCOUNT		0xFF05
#define ACOS_FID_ACCOUNT_SECURITY	0xFF06
#define ACOS_FID_ATR			0xFF07

/* FILE record length and number of records: fixed numbers only */
#define ACOS_RECLEN_MCU_ID		8
#define ACOS_NUMREC_MCU_ID		2
#define ACOS_RECLEN_MANUFACTURER	8
#define ACOS_NUMREC_MANUFACTURER	2
#define ACOS_RECLEN_PERSONALIZATION	4
#define ACOS_NUMREC_PERSONALIZATION	3
#define ACOS_RECLEN_USERFILE_MNG	6

/* number of records is determined by how many files there (max is 255) */
#define ACOS_NUMREC_USERFILE_MNG		255
/* max number of bytes one record can have */
#define ACOS2_USERFILE_MAX_RECORD_LENGTH	32
/* max number of records a user file can have */
#define ACOS2_USERFILE_MAX_RECORD_NUMBER	255
#define ACOS3_USERFILE_MAX_RECORD_LENGTH	255
#define ACOS3_USERFILE_MAX_RECORD_NUMBER	255
/* return value, don't be same as status_ok */
#define ACOS_RET_BASE            (1)
#define ACOS_CMD_RET_UNKNOWN     (ACOS_RET_BASE + 0) /* same to all commands */
#define ACOS_CMD_RET_FILESIZE    (ACOS_RET_BASE + 1) /* file size too big */
#define ACOS_CMD_RET_BASE        ACOS_CMD_RET_FILESIZE

/* CLA AND INSs */
#define ACOS_CLA				(0x80)
#define ACOS_INS_SUBMIT_CODE			(0x20)
#define ACOS_INS_SELECT_FILE			(0xA4)
#define ACOS_INS_READ_RECORD			(0xB2)
#define ACOS_INS_WRITE_RECORD			(0xD2)

/* submit code command */
#define ACOS_CODE_REF_AC1			(1)
#define ACOS_CODE_REF_AC2			(2)
#define ACOS_CODE_REF_AC3			(3)
#define ACOS_CODE_REF_AC4			(4)
#define ACOS_CODE_REF_AC5			(5)
#define ACOS_CODE_REF_PIN			(6)
#define ACOS_CODE_REF_IC			(7)

#define ACOS_SUBMIT_CODE_CMDLEN			(13)
#define ACOS_SUBMIT_CODE_CODELEN		(8)
#define ACOS_SUBMIT_CODE_IDX_CODE_REF		(2) /* Index to the code ref */
#define ACOS_SUBMIT_CODE_IDX_CODE		(5)

#define ACOS_SUBMIT_CODE_RET_WRONG_CODE		(ACOS_CMD_RET_BASE + 1)
#define ACOS_SUBMIT_CODE_RET_CODE_LOCKED	(ACOS_CMD_RET_BASE + 2)
#define ACOS_SUBMIT_CODE_RET_AUTH_NEEDED	(ACOS_CMD_RET_BASE + 3)

/* select file command */
#define ACOS_SELECT_FILE_CMDLEN			(7)
#define ACOS_SELECT_FILE_IDX_FILE_ID		(5) /* Index to the code ref */

#define ACOS_SELECT_FILE_RET_FILE_NOTEXIST	(ACOS_CMD_RET_BASE + 1)
#define ACOS_SELECT_FILE_RET_FILE_SELECTED	(ACOS_CMD_RET_BASE + 2)

/* read record command */
#define ACOS_READ_RECORD_CMDLEN			(5)
#define ACOS_READ_RECORD_IDX_RECORD_NO		(2)
#define ACOS_READ_RECORD_IDX_LEN		(4)

#define ACOS_READ_RECORD_RET_NO_SECURITY_CODE	(ACOS_CMD_RET_BASE + 1)
#define ACOS_READ_RECORD_RET_RECORD_NOT_FOUND	(ACOS_CMD_RET_BASE + 2)
#define ACOS_READ_RECORD_RET_LEN_TOO_LARGE	(ACOS_CMD_RET_BASE + 3)
#define ACOS_READ_RECORD_RET_NO_FILE_SELECTED	(ACOS_CMD_RET_BASE + 4)
#define ACOS_READ_RECORD_RET_IO_ERROR		(ACOS_CMD_RET_BASE + 5)

/* write record command */
#define ACOS_WRITE_RECORD_CMDLEN	(5 + ACOS3_USERFILE_MAX_RECORD_LENGTH)
#define ACOS_WRITE_RECORD_IDX_RECORD_NO		(2)
#define ACOS_WRITE_RECORD_IDX_LEN		(4)
#define ACOS_WRITE_RECORD_IDX_DATA		(5)

#define ACOS_WRITE_RECORD_RET_NO_SECURITY_CODE     (ACOS_CMD_RET_BASE + 1)
#define ACOS_WRITE_RECORD_RET_RECORD_NOT_FOUND     (ACOS_CMD_RET_BASE + 2)
#define ACOS_WRITE_RECORD_RET_LEN_TOO_LARGE        (ACOS_CMD_RET_BASE + 3)
#define ACOS_WRITE_RECORD_RET_NO_FILE_SELECTED     (ACOS_CMD_RET_BASE + 4)
#define ACOS_WRITE_RECORD_RET_IO_ERROR             (ACOS_CMD_RET_BASE + 5)
#define ACOS_WRITE_RECORD_RET_BUF_TOO_BIG          (ACOS_CMD_RET_BASE + 6)
/* ACOS3 speed as saved to ATR file */
#define ACOS3_SPEED_115200			(0x95)
#define ACOS3_SPEED_57600			(0x94)
#define ACOS3_SPEED_28800			(0x93)
#define ACOS3_SPEED_14400			(0x92)
#define ACOS3_SPEED_9600			(0x11)

#define SC_T0_CMDHDR_LEN       5

#define WAIT()  /* phx_blocked_delay_ms(1); */
s32_t cit_acos2_submit_code(struct ch_handle *handle, u8_t code_ref,
			    u8_t *code);
s32_t cit_acos_get_user_file_info(struct ch_handle *handle, u8_t file_number,
				  u8_t *record_len, u8_t *record_no,
				  u8_t *read_attr, u8_t *write_attr,
				  u16_t *file_id);
s32_t cit_acos_create_user_file(struct ch_handle *handle, u8_t file_number,
				u32_t file_len, u8_t read_attr,
				u8_t write_attr, u16_t file_id);
s32_t cit_acos2_read_file(struct ch_handle *handle, u16_t file_id,
			  u32_t read_len, u8_t *p_buf);
s32_t cit_acos2_write_file(struct ch_handle *handle, u16_t file_id,
			   u32_t write_len, u8_t *p_buf);
s32_t cit_acos_get_card_info(struct ch_handle *handle);

#endif
