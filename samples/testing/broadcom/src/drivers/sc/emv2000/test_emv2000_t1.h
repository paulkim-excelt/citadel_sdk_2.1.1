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
 * @file test_emv2000_t1.h
 *
 * @brief  smart card cli emv2000 t1 header file
 */

#ifndef TEST_EMV2000_T1_H__
#define TEST_EMV2000_T1_H__

/* Required for I Blocks */
#define SC_I_BLK_SEQ_1_CHAINING			(0x60)
#define SC_I_BLK_SEQ_1_NO_CHAINING		(0x40)
#define SC_I_BLK_SEQ_0_CHAINING			(0x20)
#define SC_I_BLK_SEQ_0_NO_CHAINING		(0x00)

/* Required for R Blocks */
#define SC_R_BLK_SEQ_0_ERR_FREE			(0x80)
#define SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR	(0x81)
#define SC_R_BLK_SEQ_0_OTHER_ERR		(0x82)
#define SC_R_BLK_SEQ_1_ERR_FREE			(0x90)
#define SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR	(0x91)
#define SC_R_BLK_SEQ_1_OTHER_ERR		(0x92)

/* Required for S Blocks, request and responses */
#define SC_S_BLK_RESYNCH_REQ			(0xC0)
#define SC_S_BLK_RESYNCH_RESP			(0xE0)
#define SC_S_BLK_IFS_REQ			(0xC1)
#define SC_S_BLK_IFS_RESP			(0xE1)
#define SC_S_BLK_ABORT_REQ			(0xC2)
#define SC_S_BLK_ABORT_RESP			(0xE2)
#define SC_S_BLK_WTX_REQ			(0xC3)
#define SC_S_BLK_WTX_RESP			(0xE3)
#define SC_S_BLK_VPP_ERR			(0xE4)

#define SC_RESEND_PREVIOUS_BLK			(0xFF)
#define SC_NULL_BLK				(0xFE)

#define SC_EMV_ICC_ID				(0)
#define SC_EMV_IFD_ID				(1)

#define SC_EMV_PROLOGUE_FIELD_LENGTH		(3)

#define SC_MAX_VALID_PCBS			(19)

static const u8_t sc_emv_valid_pcbs[SC_MAX_VALID_PCBS] = {
	SC_I_BLK_SEQ_1_CHAINING,
	SC_I_BLK_SEQ_1_NO_CHAINING,
	SC_I_BLK_SEQ_0_CHAINING,
	SC_I_BLK_SEQ_0_NO_CHAINING,
	SC_R_BLK_SEQ_0_ERR_FREE,
	SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR,
	SC_R_BLK_SEQ_0_OTHER_ERR,
	SC_R_BLK_SEQ_1_ERR_FREE,
	SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR,
	SC_R_BLK_SEQ_1_OTHER_ERR,
	SC_S_BLK_RESYNCH_REQ,
	SC_S_BLK_RESYNCH_RESP,
	SC_S_BLK_IFS_REQ,
	SC_S_BLK_IFS_RESP,
	SC_S_BLK_ABORT_REQ,
	SC_S_BLK_ABORT_RESP,
	SC_S_BLK_WTX_REQ,
	SC_S_BLK_WTX_RESP,
	SC_S_BLK_VPP_ERR
};

void cli_emv_t1_create_r_blk_error_other(struct sc_cli_channel_handle *handle,
					 u8_t *next_blk_type_tx);

s32_t cli_emv_t1_pcb_s_block(struct sc_cli_channel_handle *handle,
			     u8_t block_type, u8_t inf_value);

s32_t sc_cli_emv_t1_receive(struct sc_cli_channel_handle *handle, u8_t *rx,
			    u32_t *len);

s32_t sc_cli_emv_t1_get_next_cmd(struct sc_cli_channel_handle *handle,
				 u8_t pre_pcb, u8_t *next_blk_to_send,
				 u8_t *next_blk_to_rcv, u8_t *rx_buf,
				 u8_t *tx_inf_buf, u8_t *cmd_len,
				 s32_t incoming_status);

s32_t sc_cli_emv_t1_i_block(struct sc_cli_channel_handle *handle, u8_t blk_type,
			    u8_t *in_tx_buf, u8_t i_len, u8_t *backup_buf);

s32_t cli_emv_t1_pcb_r_block(struct sc_cli_channel_handle *handle,
			     u8_t block_type);

void cli_emv_t1_update_seq_num(struct sc_cli_channel_handle *handle, u32_t id);
#endif /* TEST_EMV2000_T1_H__ */
