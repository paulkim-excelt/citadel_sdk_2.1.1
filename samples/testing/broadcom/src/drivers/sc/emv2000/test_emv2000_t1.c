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
 * @file test_emv2000_t1.c
 *
 * @brief  EMV2000 T1 tests
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
#include <zephyr/types.h>
#include <sc/sc_datatypes.h>
#include <sc/sc.h>
#include "../test_sc_infra.h"
#include "test_emv2000_t1.h"
#include <ztest.h>

s32_t cli_emv_t1_pcb_s_block(struct sc_cli_channel_handle *handle,
			     u8_t block_type, u8_t inf_value)
{
	u32_t val;
	s32_t rv;

	handle->tx_buf[0] = 0x00;   /* NAD Byte always = 0 */
	handle->tx_buf[1] = block_type;

	if ((block_type == SC_S_BLK_WTX_REQ) ||
	    (block_type == SC_S_BLK_WTX_RESP) ||
	    (block_type == SC_S_BLK_IFS_REQ) ||
	    (block_type == SC_S_BLK_IFS_RESP)) {

		handle->tx_buf[2] = 0x01; /* Single byte length */
		handle->tx_buf[3] = inf_value;

		if (block_type ==  SC_S_BLK_IFS_RESP) {
			/* Specify new IFS */
			handle->ch_param.ifsd = inf_value;
		} else if (block_type == SC_S_BLK_WTX_RESP) {
			TC_DBG("About to set WTX timer...\n");
			val = inf_value *
					handle->ch_param.blk_wait_time.val;
			rv = sc_cli_set_block_wait_time_ext(handle, val);
			if (rv) {
				TC_DBG("Could not set WTX timer\n");
				return -EINVAL;
			}
			TC_DBG("WTX timer set extend BWT = %d\n", val);
		}

		handle->tx_len = 4;
		rv = sc_cli_transmit(handle);
	} else {
		/*
		 * INF is absent when S-block signals an error on VPP state
		 * or managing chain abortion or resynch
		 */
		handle->tx_buf[2] = 0x00;
		handle->tx_len = 3;
		rv = sc_cli_transmit(handle);
	}

	return rv;
}

s32_t cli_emv_t1_validate_r_blk_seqnum(struct sc_cli_channel_handle *handle,
				       u8_t pcb)
{
	if ((pcb == SC_R_BLK_SEQ_0_ERR_FREE) ||
	    (pcb == SC_R_BLK_SEQ_1_ERR_FREE)) {
		if (((pcb & 0x10) && !(handle->icc_seq_num)) ||
		    (!(pcb & 0x10) && (handle->icc_seq_num))) {
			/*
			 * R-block sequence number received does not match
			 * sequence number of the next I-block expected by
			 * the ICC
			 */
			TC_DBG("CLI: Incorrect R-Block Sequence Number");
			return -EINVAL;
		}
	} else {
		/*
		 * Sequence number should not equal that of expected next
		 * I-block from ICC
		 */
		if (((pcb & 0x10) && (handle->icc_seq_num)) ||
		    (!(pcb & 0x10) && !(handle->icc_seq_num))) {
			/*
			 * R-block sequence number received does not match
			 * sequence number of the next I-block expected by
			 * the ICC
			 */
			TC_DBG("CLI: Incorrect R-Block Sequence Number");
			return -EINVAL;
		}
	}

	return 0;
}


void cli_emv_t1_create_r_blk_error_free(struct sc_cli_channel_handle *handle,
					u8_t *next_blk_type_tx)
{
	if (handle->ifd_seq_num == SC_CLI_SEQUENCE_NUMBER_0)
		*next_blk_type_tx = SC_R_BLK_SEQ_0_ERR_FREE;
	else
		*next_blk_type_tx = SC_R_BLK_SEQ_1_ERR_FREE;
}

void cli_emv_t1_create_r_blk_edc_error(struct sc_cli_channel_handle *handle,
					u8_t *next_blk_type_tx)
{
	if (handle->ifd_seq_num == SC_CLI_SEQUENCE_NUMBER_0)
		*next_blk_type_tx = SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR;
	else
		*next_blk_type_tx = SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR;
}

void cli_emv_t1_create_r_blk_error_other(struct sc_cli_channel_handle *handle,
					 u8_t *next_blk_type_tx)
{
	if (handle->ifd_seq_num == SC_CLI_SEQUENCE_NUMBER_0)
		*next_blk_type_tx = SC_R_BLK_SEQ_0_OTHER_ERR;
	else
		*next_blk_type_tx = SC_R_BLK_SEQ_1_OTHER_ERR;
}

s32_t sc_cli_emv_t1_receive(struct sc_cli_channel_handle *handle, u8_t *rx,
			    u32_t *len)
{
	s32_t rv = 0;
	u32_t cnt = 0;

	handle->rx_max_len = SC_MAX_EMV_BUFFER_SIZE;
	rv = sc_cli_receive(handle);

	for (cnt = 0; cnt < handle->rx_len; cnt++)
		rx[cnt] = handle->rx_buf[cnt];

	*len = handle->rx_len;

	return rv;
}

s32_t sc_cli_emv_t1_get_next_cmd(struct sc_cli_channel_handle *handle,
				 u8_t pre_pcb, u8_t *next_blk_to_send,
				 u8_t *next_blk_to_rcv, u8_t *rx_buf,
				 u8_t *tx_inf_buf, u8_t *cmd_len,
				 s32_t incoming_status)
{
	s32_t rv = 0;
	u32_t cnt = 0;

	*cmd_len = 0;

	/* check for parity error */
	if (incoming_status == -EBADMSG) {
		/* Parity or EDC error, response is R-block, parity/edc error */
		if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
		    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
			/* Have a look at 1772 */
			*next_blk_to_send = pre_pcb;
			return 0;
		}

		cli_emv_t1_create_r_blk_edc_error(handle, next_blk_to_send);
		return 0;
	}

	/* check for NAD byte */
	if (rx_buf[0]) {
		/* NAD Byte != 0, so response is R-block, other error */
		TC_DBG("%s NAD byte not equal to zero\n", __func__);

		if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
		    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
			/*
			 * look at 1772 - See invlaid block after first
			 * rbllock sent in emv book
			 */
			*next_blk_to_send = pre_pcb;
			return 0;
		}

		cli_emv_t1_create_r_blk_error_other(handle, next_blk_to_send);
		return 0;
	}

	/* check for valid pcb */
	while (cnt < SC_MAX_VALID_PCBS) {
		if (rx_buf[1] == sc_emv_valid_pcbs[cnt])
			break;
		cnt++;
	}

	if (cnt == SC_MAX_VALID_PCBS) {
		TC_DBG("%s PCB Byte is invalid", __func__);
		/* PCB Byte is not valid, so response is R-block, other error */
		if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
		    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
			/* Have a look at 1772 */
			*next_blk_to_send = pre_pcb;
			return 0;
		}
		cli_emv_t1_create_r_blk_error_other(handle, next_blk_to_send);
		return 0;
	}

	/* Validating S-block received */
	if ((rx_buf[1] & 0xC0) == 0xC0) {
		/* S-block has been received from ICC */
		if (((rx_buf[1] == SC_S_BLK_ABORT_RESP) &&
		     (pre_pcb != SC_S_BLK_ABORT_REQ)) ||
		    ((rx_buf[1] == SC_S_BLK_RESYNCH_RESP) &&
		     (pre_pcb != SC_S_BLK_RESYNCH_REQ)) ||
		    ((rx_buf[1] == SC_S_BLK_IFS_RESP) &&
		     (pre_pcb != SC_S_BLK_IFS_REQ)) ||
		    ((rx_buf[1] == SC_S_BLK_WTX_RESP) &&
		     (pre_pcb != SC_S_BLK_WTX_REQ))) {

			TC_DBG("S-block response S-block request made\n");
			if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
			    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
		}

		if (rx_buf[1] == SC_S_BLK_ABORT_REQ) {
			/* Abort request, so response is Abort response */
			if (rx_buf[2] != 0x00) {
			/*
			 * If LEN byte != 0 for abort request, then
			 * response is R-block, other error
			 */
				TC_DBG("Abort request with Len != 0");
				if (((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_0_ERR_FREE) ||
				    ((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
			}

			*next_blk_to_send = SC_S_BLK_ABORT_RESP;
			return 0;
		} else if (rx_buf[1] == SC_S_BLK_RESYNCH_REQ) {
			if (rx_buf[2] != 0x00) {
			/*
			 * If LEN byte != 0 for resynch request, then
			 * response is R-block, other error
			 */
				TC_DBG("Resynch request with Len != 0");

				if (((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_0_ERR_FREE) ||
				    ((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
			}

			*next_blk_to_send = SC_S_BLK_RESYNCH_RESP;
			return 0;
		} else if (rx_buf[1] == SC_S_BLK_WTX_REQ) {
			if (rx_buf[2] != 0x01) {
			/*
			 * If LEN byte != 1 for WTX request, then
			 * response is R-block, other error
			 */
				TC_DBG("WTX request with Len != 0");

				if (((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_0_ERR_FREE) ||
				    ((pre_pcb & 0xF0) ==
						 SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
			}

			/* Specify INF Value for WTX Response */
			tx_inf_buf[0] = rx_buf[3];
			*next_blk_to_send = SC_S_BLK_WTX_RESP;
			return 0;
		} else if (rx_buf[1] == SC_S_BLK_IFS_REQ) {
			if ((rx_buf[2] != 0x01) || (rx_buf[3] < 0x10) ||
						   (rx_buf[3] > 0xFE)) {
				/*
				 * If LEN byte != 1 for IFS request, then
				 * response is R-block, other error
				 */
				/* and IFSI may only be between 10 and FE */

				TC_DBG("IFS request with Len != 1\n");
				if (((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_0_ERR_FREE) ||
				((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_1_ERR_FREE)) {
					/* Have a look at 1772 */
					*next_blk_to_send = pre_pcb;
					return 0;
				}

				cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
				return 0;
			}

			/* Specify INF Value for IFS Response */
			tx_inf_buf[0] = rx_buf[3];
			*next_blk_to_send = SC_S_BLK_IFS_RESP;
			return 0;
		} else if (rx_buf[1] == SC_S_BLK_VPP_ERR) {
			if (rx_buf[2] != 0x00) {

				/*
				 * If LEN byte != 0 for resynch request, then
				 * response is R-block, other error
				 */
				TC_DBG(" VPP Error with Len != 0\n");
				if (((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_0_ERR_FREE) ||
					((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_1_ERR_FREE)) {
					/* Have a look at 1772 */
					*next_blk_to_send = pre_pcb;
					return 0;
				}
				cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
				return 0;
			}

			TC_DBG("VPP Error, deactivating\n");
			return -EINVAL;
		} else if (rx_buf[1] == SC_S_BLK_IFS_RESP) {
			/*
			 * Only used for initial IFSD after ATR from
			 * terminal to ICC
			 */
			if (rx_buf[2] != 0x01) {
				/*
				 * If LEN byte != 1 for IFS response, then
				 * response is R-block, other error
				 */
				TC_DBG("IFS response with Len != 1\n");
				if (((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_0_ERR_FREE) ||
				    ((pre_pcb & 0xF0) ==
						SC_R_BLK_SEQ_1_ERR_FREE)) {
					/* Have a look at 1772 */
					*next_blk_to_send = pre_pcb;
					return 0;
				}
				cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
				return 0;
			}
			*next_blk_to_send = SC_S_BLK_IFS_RESP;
			return 0;
		}
	}

	/* Validating I-block received */
	if ((rx_buf[1] & 0xF0) < 0x80) {
		/* I-block has been received from ICC */
		/*
		 * Test 1804, xy=09.  ICC responses with I-Block after IFD sends
		 * IFS request. IFD normally only cares about SC_S_BLK_IFS_REQ.
		 * We could comment out the rest of S Block request.
		 */
		if ((pre_pcb == SC_S_BLK_RESYNCH_REQ) ||
		    (pre_pcb == SC_S_BLK_IFS_REQ) ||
		    (pre_pcb == SC_S_BLK_WTX_REQ) ||
		    (pre_pcb == SC_S_BLK_ABORT_REQ)) {
			*next_blk_to_send = pre_pcb;
			return 0;
		}

		if ((pre_pcb == SC_I_BLK_SEQ_1_CHAINING) ||
		    (pre_pcb == SC_I_BLK_SEQ_0_CHAINING) ||
		    (*next_blk_to_rcv == SC_R_BLK_SEQ_0_ERR_FREE)) {
			/*
			 * Not allowed to receive another I-Block while still
			 * transmitting a series of chained I-blocks, indicated
			 * by bit 6 being set high
			 **/
			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
		}

		/* validating i-block seq num */
		if (((rx_buf[1] & 0x40) && !(handle->ifd_seq_num)) ||
		    (!(rx_buf[1] & 0x40) && (handle->ifd_seq_num))) {
			/*
			 * If sequence number received does not match sequence
			 * number expected by the terminal (IFD)
			 */
			TC_DBG("Incorrect I-Block Sequence Number\n");

			/*
			 * Test 1778, subtest 71.  Re-transmit R-Block if the
			 * response to R-Block is incorrect
			 */
			if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
			    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}
			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
				return 0;
		}

		/* Update the sequence numbers */
		cli_emv_t1_update_seq_num(handle, SC_EMV_IFD_ID);

		if ((rx_buf[1] == SC_I_BLK_SEQ_0_NO_CHAINING) ||
		    (rx_buf[1] == SC_I_BLK_SEQ_1_NO_CHAINING)) {
			/* Either an I-block has been received with no chaining,
			 * or the last I-block of a chained set has been
			 * received from the ICC
			 */
			/*
			 * Accounting for status bytes at the end of incoming
			 * command from ICC
			 */
			*cmd_len = rx_buf[2] - 2;

			/* Store INF data bytes */
			for (cnt = 0; cnt < *cmd_len; cnt++)
				tx_inf_buf[cnt] = rx_buf[cnt +
						SC_EMV_PROLOGUE_FIELD_LENGTH];

			if (handle->icc_seq_num == SC_CLI_SEQUENCE_NUMBER_0) {
				*next_blk_to_send = SC_I_BLK_SEQ_0_NO_CHAINING;
				/* Should receive any I-Block */
				*next_blk_to_rcv = SC_I_BLK_SEQ_0_NO_CHAINING;
			} else {
				*next_blk_to_send = SC_I_BLK_SEQ_1_NO_CHAINING;
				/* Should receive any I-Block */
				*next_blk_to_rcv = SC_I_BLK_SEQ_1_NO_CHAINING;
			}

			return 0;
		} else if ((rx_buf[1] == SC_I_BLK_SEQ_0_CHAINING) ||
			 (rx_buf[1] == SC_I_BLK_SEQ_1_CHAINING)) {
			/* I-block with chaining received */
			*cmd_len = rx_buf[2];

			/* Store INF data bytes */
			for (cnt = 0; cnt < *cmd_len; cnt++)
				tx_inf_buf[cnt] = rx_buf[cnt +
						SC_EMV_PROLOGUE_FIELD_LENGTH];

			/* Send R-blk resp. continue receiving chained blks */
			cli_emv_t1_create_r_blk_error_free(handle,
							 next_blk_to_send);
			/* Test 1782.  Need to reset this */
			*next_blk_to_rcv = SC_NULL_BLK;
			return 0;
		}
	}

	/* validating r-block received */
	if (((rx_buf[1] & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
	    ((rx_buf[1] & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
		/* R-block has been received from ICC */
		if (rx_buf[2] != 0x00) {
			/* Length of R-block must have a zero value */
			if (((pre_pcb & 0xF0) == SC_R_BLK_SEQ_0_ERR_FREE) ||
			    ((pre_pcb & 0xF0) == SC_R_BLK_SEQ_1_ERR_FREE)) {
				/* Have a look at 1772 */
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			cli_emv_t1_create_r_blk_error_other(handle,
							 next_blk_to_send);
			return 0;
		}

		if (rx_buf[1] == SC_R_BLK_SEQ_0_ERR_FREE) {
			rv = cli_emv_t1_validate_r_blk_seqnum(handle,
								rx_buf[1]);

			if (rv) {
				/* Check with Integri About this test 1769 */
				if ((pre_pcb & 0xF0) < 0x80)
					*next_blk_to_send =
							SC_RESEND_PREVIOUS_BLK;
				return 0;
			}

			if (/* Send non-chain I(*,0), should receive I-Block */
			    (pre_pcb == SC_I_BLK_SEQ_0_NO_CHAINING) ||
			    (pre_pcb == SC_I_BLK_SEQ_1_NO_CHAINING) ||
				/*
				 * Test 1794, y=2 subtest 76.  Send non-chian
				 * I-block, should receive I block
				 */
			     (*next_blk_to_rcv == SC_I_BLK_SEQ_0_NO_CHAINING) ||
				/*
				 * Test 1794, y=2 add checking for sequence
				 * number 1 as well
				 */
			     (*next_blk_to_rcv == SC_I_BLK_SEQ_1_NO_CHAINING) ||
			     (pre_pcb == SC_S_BLK_IFS_RESP)) {
				cli_emv_t1_create_r_blk_error_other(handle,
							next_blk_to_send);
				return 0;
			}

			/*
			 * Indicates to continue sending next I-block,
			 * with no sequence number set
			 */
			*next_blk_to_send = SC_I_BLK_SEQ_0_NO_CHAINING;
			*next_blk_to_rcv = SC_I_BLK_SEQ_0_NO_CHAINING;
			return 0;
		} else if (rx_buf[1] == SC_R_BLK_SEQ_1_ERR_FREE) {
			rv = cli_emv_t1_validate_r_blk_seqnum(handle,
								rx_buf[1]);

			if (rv) {
				/* Check with Integri About this test 1769 */
				if ((pre_pcb & 0xF0) < 0x80)
					*next_blk_to_send =
							 SC_RESEND_PREVIOUS_BLK;
				return 0;
			}

			if (/* Send non-chain I(*,0), should receive I-Block */
			    (pre_pcb == SC_I_BLK_SEQ_0_NO_CHAINING) ||
			    (pre_pcb == SC_I_BLK_SEQ_1_NO_CHAINING) ||
				/* Test 1794, y=2 subtest 76.  Send non-chian
				 * I-block, should receive I block
				 */
			     (*next_blk_to_rcv == SC_I_BLK_SEQ_0_NO_CHAINING) ||
				/*
				 * Test 1794, y=2 add checking for sequence
				 * number 1 as well
				 */
			     (*next_blk_to_rcv == SC_I_BLK_SEQ_1_NO_CHAINING) ||
			     (pre_pcb == SC_S_BLK_IFS_RESP)) {
				cli_emv_t1_create_r_blk_error_other(handle,
							next_blk_to_send);
				return 0;
			}

			/*
			 * Indicates to continue sending next I-block,
			 * with sequence number set
			 */
			*next_blk_to_send = SC_I_BLK_SEQ_1_NO_CHAINING;
			*next_blk_to_rcv = SC_I_BLK_SEQ_0_NO_CHAINING;
			return 0;
		} else if ((rx_buf[1] == SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR) ||
			   (rx_buf[1] == SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR)) {

			TC_DBG("R-block, EDC/Parity error from ICC");
			if ((((pre_pcb & 0xF0) == 0x80) &&
			      (rx_buf[1] ==
					 SC_R_BLK_SEQ_1_EDC_OR_PARITY_ERR)) ||
			     (((pre_pcb & 0xF0) == 0x90) &&
			      (rx_buf[1] ==
					 SC_R_BLK_SEQ_0_EDC_OR_PARITY_ERR)) ||
			     ((pre_pcb & 0xF3) == 0x80) ||
			     ((pre_pcb & 0xF3) == 0x90)) {
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			rv = cli_emv_t1_validate_r_blk_seqnum(handle,
								rx_buf[1]);
			if (rv) {
				cli_emv_t1_create_r_blk_error_other(handle,
							next_blk_to_send);
				return 0;
			}

			*next_blk_to_send = SC_RESEND_PREVIOUS_BLK;
			return 0;
		} else if ((rx_buf[1] == SC_R_BLK_SEQ_0_OTHER_ERR) ||
			   (rx_buf[1] == SC_R_BLK_SEQ_1_OTHER_ERR)) {

			TC_DBG("R-block, other error from ICC\n");

			if ((((pre_pcb & 0xF0) == 0x80) &&
			      (rx_buf[1] == SC_R_BLK_SEQ_1_OTHER_ERR)) ||
			     (((pre_pcb & 0xF0) == 0x90) &&
			      (rx_buf[1] == SC_R_BLK_SEQ_0_OTHER_ERR)) ||
			     ((pre_pcb & 0xF3) == 0x80) ||
			     ((pre_pcb & 0xF3) == 0x90)) {
				*next_blk_to_send = pre_pcb;
				return 0;
			}

			rv = cli_emv_t1_validate_r_blk_seqnum(handle,
								rx_buf[1]);

			if (rv) {
				cli_emv_t1_create_r_blk_error_other(handle,
							next_blk_to_send);
				return 0;
			}

			*next_blk_to_send = SC_RESEND_PREVIOUS_BLK;
			return 0;
		}
	}

	return 0;
}

s32_t sc_cli_emv_t1_i_block(struct sc_cli_channel_handle *handle, u8_t blk_type,
			    u8_t *in_tx_buf, u8_t i_len, u8_t *backup_buf)
{
	u8_t tx_buf[SC_MAX_EMV_BUFFER_SIZE];
	u32_t length;
	u32_t cnt;

	cli_emv_t1_update_seq_num(handle, SC_EMV_ICC_ID);
	tx_buf[0] = 0x00;  /* NAD Byte always = 0 */
	tx_buf[1] = blk_type;
	/* For I-blocks, INF field always > 0 and < 0xFF */
	if ((i_len == 0x00) || (i_len == 0xFF) ||
	    (i_len > handle->ch_param.ifsd))
		return -EINVAL;

	tx_buf[2] = i_len;

	length = i_len + SC_EMV_PROLOGUE_FIELD_LENGTH;
	/* BSYT Issue: use memcpy here??  */
	for (cnt = SC_EMV_PROLOGUE_FIELD_LENGTH; cnt < length; cnt++)
		tx_buf[cnt] = in_tx_buf[cnt - SC_EMV_PROLOGUE_FIELD_LENGTH];

	/* Creating backup for possible re-sending*/
	for (cnt = 0; cnt < length; cnt++)
		backup_buf[cnt] = tx_buf[cnt];

	handle->tx_len = SC_EMV_PROLOGUE_FIELD_LENGTH + i_len;

	for (cnt = 0; cnt < handle->tx_len; cnt++)
		handle->tx_buf[cnt] = tx_buf[cnt];

	return sc_cli_transmit(handle);
}

void cli_emv_t1_update_seq_num(struct sc_cli_channel_handle *handle, u32_t id)
{
	if (id == SC_EMV_ICC_ID) {
		if (handle->icc_seq_num == SC_CLI_SEQUENCE_NUMBER_0)
			handle->icc_seq_num = SC_CLI_SEQUENCE_NUMBER_1;
		else
			handle->icc_seq_num = SC_CLI_SEQUENCE_NUMBER_0;
	} else if (id == SC_EMV_IFD_ID) {
		if (handle->ifd_seq_num == SC_CLI_SEQUENCE_NUMBER_0)
			handle->ifd_seq_num = SC_CLI_SEQUENCE_NUMBER_1;
		else
			handle->ifd_seq_num = SC_CLI_SEQUENCE_NUMBER_0;
	}

	TC_DBG("icc_seq_num = %d, ifd_seq_num = %d", handle->icc_seq_num,
							handle->ifd_seq_num);
}

s32_t cli_emv_t1_pcb_r_block(struct sc_cli_channel_handle *handle,
			     u8_t block_type)
{
	handle->tx_buf[0] = 0x00;   /* NAD Byte always = 0 */
	handle->tx_buf[1] = block_type;
	handle->tx_buf[2] = 0x00;  /* For R-blocks, INF field always = 0 */

	/* R blocks have a message length of 3, not including the EDC byte(s) */
	handle->tx_len = 3;

	return sc_cli_transmit(handle);
}
