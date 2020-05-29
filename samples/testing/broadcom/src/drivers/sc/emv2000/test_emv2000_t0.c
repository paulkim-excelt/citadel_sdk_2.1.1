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
 * @file test_emv2000_t0.c
 *
 * @brief  EMV2000 T0 test
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
#include "test_emv2000_t0.h"
#include <ztest.h>

s32_t cli_emv_t0_receive(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
			 u32_t ins_byte, u32_t expected_len,
			 bool data_expected, u32_t *received_len);

s32_t cli_emv_get_response(struct sc_cli_channel_handle *handle,
			   u32_t expected_length,
			   u8_t *emv_rx_buf, u32_t *response_length);

s32_t sc_cli_emv_t0_case_1or2_cmd(struct sc_cli_channel_handle *handle,
				  u8_t *tx_buf, u32_t length,
				  u8_t *response_buf, u32_t *response_len)
{
	s32_t rv = 0;
	u8_t rx_buf[SC_MAX_EMV_BUFFER_SIZE];
	u8_t ins_byte;
	u32_t expected_length;
	u32_t received_len;
	u32_t cnt;

	/* Reset response length before processing command */
	*response_len = 0;
	ins_byte = tx_buf[1];
	if (length < 5)
		tx_buf[4] = 0;

	memcpy(handle->tx_buf, tx_buf, 5);
	handle->tx_len = 5;
	rv = sc_cli_transmit(handle);
	if (rv) {
		TC_ERROR("Transmit failed");
		return rv;
	}

	/* No expected data, so params 3 and 4 are sent to zero */
	rv = cli_emv_t0_receive(handle, rx_buf, ins_byte, 0,
				SC_CLI_NO_DATA_EXPECTED, &received_len);
	if (rv)
		return rv;

	if (rx_buf[0] == 0x6C) {
		/*
		 * Case Two Command requesting a resending of the 5 byte
		 * header along with the 5th byte (P3) = rx_buf[1]
		 */
		tx_buf[4] = rx_buf[1];
		expected_length = (u32_t)rx_buf[1];
		memcpy(handle->tx_buf, tx_buf, 5);
		handle->tx_len = 5;
		rv = sc_cli_transmit(handle);
		if (rv) {
			TC_ERROR("Transmit failed");
			return rv;
		}

		/* No expected data, so params 3 and 4 are sent to zero */
		rv = cli_emv_t0_receive(handle,	rx_buf, ins_byte,
					expected_length, SC_CLI_DATA_EXPECTED,
					&received_len);
		if (rv)
			return rv;

		if (rx_buf[0] == 0x61) {
			/*
			 * At this step for Case 2, under normal processing,
			 * procedure bytes 61xx are expected from the terminal
			 * and GET RESPONSE command with length of rx_buf[1]
			 * should be issued by the terminal
			 */
			cli_emv_get_response(handle, (u32_t)(rx_buf[1]),
					     response_buf, response_len);
			*response_len = (u32_t)(response_buf[4] + 0x05);

			return 0;
		} else if (received_len == (expected_length + 2)) {
			/*
			 * expected_length+2 since two additional status bytes
			 * are sent to acknowledge command completion
			 */
			for (cnt = 0; cnt < expected_length; cnt++)
				response_buf[cnt] = rx_buf[cnt];
			*response_len = expected_length;
			return 0;
		}

		TC_ERROR("Invalid byte received from ICC");
		return -EINVAL;
	} else if (rx_buf[0] == 0x61) {
		/*
		 * At this step for Case 2, under normal processing,
		 * procedure bytes 61xx are expected from the terminal
		 * and GET RESPONSE command with length of rx_buf[1]
		 * should be issued by the terminal
		 */
		cli_emv_get_response(handle, (u32_t)(rx_buf[1]), response_buf,
								response_len);
			*response_len = (u32_t)(response_buf[4] + 0x05);

			return 0;
	} else if (((rx_buf[0] & 0xF0)  == 0x60) ||
		   ((rx_buf[0] & 0xF0)  == 0x90)) {
		/*
		 * If the ICC returns status bytes other than 6Cxx, then Case 1
		 * command has been detected, the terminal shall discontinue
		 * processing the command and return
		 */
		*response_len = 0;
		return 0;
	}

	return 0;
}

s32_t sc_cli_emv_t0_case_3or4_cmd(struct sc_cli_channel_handle *handle,
				  u8_t *tx_buf, u32_t length,
				  u8_t *response_buf, u32_t *response_len)

{
	u8_t rx_buf[SC_MAX_EMV_BUFFER_SIZE];
	u32_t byte_count = 5;
	u8_t ins_byte, ins_byte_complement;
	u32_t received_len;
	s32_t rv = 0;

	/* Reset response length before processing command */
	*response_len = 0;
	ins_byte = tx_buf[1];
	ins_byte_complement = ~ins_byte;
	memcpy(handle->tx_buf, tx_buf, 5);
	handle->tx_len = 5;
	rv = sc_cli_transmit(handle);
	if (rv) {
		TC_ERROR("sc_cli_emv_t0_case_3or4_cmd, Send fail%d\n", rv);
		return rv;
	}

	/* No expected data, so params 3 and 4 are sent to zero */
	rv = cli_emv_t0_receive(handle, rx_buf, ins_byte, 0,
				SC_CLI_NO_DATA_EXPECTED, &received_len);
	if (rv)
		return rv;

	while (byte_count < length) {
		if (rx_buf[0] == ins_byte) {
			/* Send remainder of data bytes	*/
			memcpy(handle->tx_buf, &tx_buf[byte_count],
						(length - byte_count));
			handle->tx_len = length - byte_count;
			rv = sc_cli_transmit(handle);
			if (rv)
				return rv;

			byte_count = length;
		} else if (rx_buf[0] == ins_byte_complement) {
			/* Send next byte only */
			memcpy(handle->tx_buf, &tx_buf[byte_count], 1);
			handle->tx_len = 1;
			rv = sc_cli_transmit(handle);
			if (rv)
				return rv;

			byte_count++;
			if (byte_count < length) {
				/*
				 * No expected data, params 3 and 4 are sent
				 * to zero
				 */
				rv = cli_emv_t0_receive(handle, rx_buf,
					 ins_byte, 0, SC_CLI_NO_DATA_EXPECTED,
					&received_len);
				if (rv)
					return rv;
			}
		} else if (((rx_buf[0] & 0xF0)  == 0x60) ||
			   ((rx_buf[0] & 0xF0)  == 0x90)) {
			*response_len = 0;
			/*
			 * If the ICC returns status bytes, the terminal shall
			 * discontinue processing the command and return
			 */
			return 0;
		}
	}

	/* No expected data, so params 3 and 4 are sent to zero */
	rv = cli_emv_t0_receive(handle, rx_buf, ins_byte, 0,
				SC_CLI_NO_DATA_EXPECTED, &received_len);
	if (rv)
		return rv;

	if ((rx_buf[0] == 0x90) && (rx_buf[1] == 0x00)) {
		/*
		 * After sending data, if terminal receives 9000 response
		 * from the ICC, then we now know that the case is Case 3,
		 * so return
		 */
		*response_len = 0;
		return 0;
	} else if (rx_buf[0] == 0x61) {
		/*
		 * At this step for Case 4, under normal processing,
		 * procedure bytes 61xx are expected from the terminal
		 * and GET RESPONSE command with length of rx_buf[1]
		 * should be issued by the terminal
		 */

		rv = cli_emv_get_response(handle, (u32_t)(rx_buf[1]),
					  response_buf, response_len);

		*response_len = (u32_t)(response_buf[4] + 0x05);
		if (rv)
			return rv;
		else
			return 0;
	}

	else if ((rx_buf[0] == 0x62) || (rx_buf[0] == 0x63) ||
		((rx_buf[0] > 0x90) && (rx_buf[0] <= 0x9F))) {

		/*
		 * At this step for Case 4, if status returned by ICC indicates
		 * a warning (62xx, or 63xx), or wich is application related
		 * (9xxx, but not 9000) the termnal should issue the GET
		 *  RESPONSE Command with the length of '0x00'
		 */
		/* EMV 2000 Test 1738 */
		if ((ins_byte == SC_CLI_EMV_APPLICATION_BLOCK) ||
		    (ins_byte == SC_CLI_EMV_APPLICATION_UNBLOCK) ||
		    (ins_byte == SC_CLI_EMV_CARD_BLOCK) ||
		    (ins_byte == SC_CLI_EMV_EXT_AUTHENTICATE) ||
		    (ins_byte == SC_CLI_EMV_PIN) ||
		    (ins_byte == SC_CLI_EMV_VERIFY)) {
			/* case 3 for test 1738 */
			*response_len = 0;
			return 0;
		}

		/* case 4 for test 1740 */
		rv = cli_emv_get_response(handle, 0x00,
			response_buf, response_len);
		*response_len = (u32_t)(response_buf[4] + 0x05);
		if (rv)
			return rv;
		else
			return 0;
	} else {
		/*
		 * If the ICC returns status bytes other than those
		 * specified above, the terminal shall discontinue processing
		 * the command and return
		 */
		*response_len = 0;
		return 0;
	}
	return 0;
}


s32_t cli_emv_t0_receive(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
			 u32_t ins_byte, u32_t expected_len,
			 bool data_expected, u32_t *received_len)
{
	s32_t rv = 0;
	u8_t data;
	bool procedure_byte_expected = false;
	bool ins_received = false;
	struct sc_status channel_status;
	u32_t i, k;
	u8_t rx_parity_cnt = 0;

	/* Set referenced param received_len = 0 */
	*received_len = 0;

	k = 0;
	i = 0;

	/*
	 * The following if clause handles T = 0 protocol
	 * (byte read) and includes the required checking of procedure bytes.
	 */
	handle->rx_max_len = 1;

	while (sc_cli_receive(handle) == 0)  {
		data = handle->rx_buf[0];

		/* Print out all received bytes */
		if ((k % 16) == 0) {
			TC_DBG("\n");
		}

		k++;
		TC_DBG("%02x ", data);

		/* BSYT Issue: Should we use the global variable here ? */
		rv = sc_cli_get_status(handle, &channel_status);
		if (rv)
			return rv;
		/*
		 * For T=0, if a parity error is detected, we will ignore the
		 * byte in receive buffer and continue reading the next byte.
		 * After repeating 3 more times, we will receive SC_RETRY_INTR
		 * interrupt and initiate deactivation sequence
		 */
		if ((channel_status.status1 & SC_RX_PARITY) == SC_RX_PARITY) {
			if (rx_parity_cnt++ >
				handle->ch_param.rx_retries) {
				TC_DBG("SC_RETRY_INTR\n");
				return -EINVAL;
			}
			TC_DBG("SC_RPAR_ERR\n");
			/* For 1714 x=1 and 1714 x=2 on LYNX SCI should already
			 * retry reading data from ICC. If still failed three
			 * more times, SCI should get SC_RETRY_INTR failure.
			 * Here SC_RPAR_ERR is just a warning messages.
			 *  The received data is valid.
			 */
		}
		/*
		 * If the value 0x60 is received, and we don't expect data from
		 * the ICC, then it is a request for additional	work waiting
		 * time, so simply wait for the next character from the ICC
		 */
		if (((i == 0) && (data == 0x60)) && (!ins_received)) {
			TC_DBG("emv T0 receive: Wait more");
			continue;
		}

		if (i == 0) {
			/* Procedure byte equal to INS byte */
			if ((data == ins_byte) && (data_expected)) {
				TC_DBG("[%s:%d]", __func__, __LINE__);
				ins_received = true;
				continue;
			}

			/* Procedure byte equal to complement of INS byte */
			if ((data == (u8_t)~ins_byte) && (data_expected)) {
				TC_DBG("[%s:%d]", __func__, __LINE__);
				procedure_byte_expected = true;
				continue;
			}

			if ((data == ins_byte) && (!data_expected)) {
				TC_DBG("[%s:%d]", __func__, __LINE__);
				/* Store byte in receive buffer */
				rx_buf[i] = data;
				*received_len = i + 1;
				goto read_complete;
			}

			if ((data == (u8_t)~ins_byte) && (!data_expected)) {
				TC_DBG("[%s:%d]", __func__, __LINE__);
				/* Store byte in receive buffer */
				rx_buf[i] = data;
				*received_len = i+1;
				goto read_complete;
			}
		}

		/*
		 * Continue reading if we keeping receiving either
		 * INS or complement of INS
		 */
		if (procedure_byte_expected == true) {
			if (data == ins_byte) {
				procedure_byte_expected = false;
				continue;
			}

			else if (data == (u8_t)~ins_byte)	{
				continue;
			}
		}

		/* Store byte in receive buffer */
		rx_buf[i] = data;

		if (i > 0) {
			if ((rx_buf[i - 1] == 0x90) && (rx_buf[i] == 0)) {
				/* Complete command has received from ICC */
				*received_len = i + 1;
				goto read_complete;
			}

			/* GET RESPONSE has been requested, so return */
			if ((rx_buf[0] == 0x61) ||
			    ((rx_buf[i - 1] == 0x61) &&
			     (i == expected_len))) {
				*received_len = i + 1;
				goto read_complete;
			}

			/* TTL should immediately resend previous command header
			 * to ICC using length of rx_buf[1]
			 */
			if ((rx_buf[0] == 0x6C) ||
			    ((rx_buf[i - 1] == 0x6C) &&
			     (i == expected_len))) {
				*received_len = i + 1;
				goto read_complete;
			}

			/* Some type of status bytes have been received, so
			 * return and analyse
			 */
			if ((rx_buf[0] > 0x60) && (rx_buf[0] <= 0x6F) &&
						  (!data_expected)) {

				*received_len = i + 1;
				goto read_complete;
			}

			if ((rx_buf[0] > 0x90) && (rx_buf[0] <= 0x9F) &&
						  (!data_expected)) {

				*received_len = i + 1;
				goto read_complete;
			}
		}

		if ((!data_expected) &&
		    ((rx_buf[0] != ins_byte) &&
		     (rx_buf[0] != (u8_t)~ins_byte) &&
		     ((rx_buf[0] & 0xF0) != 0x60) &&
		     ((rx_buf[0] & 0xF0) != 0x90))) {
			/*
			 * Invalid status byte received from the ICC
			 * Test 1736: If this is an invalid status byte, we need
			 * to read in the 2nd byte too
			 */
			test_sc_delay(5);
			return -EINVAL;
		}

		i++;

		if (i == (expected_len + 2)) {

			/* Entire data expected plus status bytes have
			 * been received, so now return
			 */
			*received_len = i;
			goto read_complete;
		}
	}

	*received_len = i;

	/*
	 * If Smart Card Byte Read returns failed, break from while
	 * loop and return fail
	 */
	return -EINVAL;
read_complete:
	/* Disable the WWT, which we enable in SmartCardSend */
	/* Need this for MetroWerks */
	rv = sc_cli_disable_work_wait_timer(handle);
	return rv;
}

s32_t cli_emv_get_response(struct sc_cli_channel_handle *handle,
			   u32_t expected_length,
			   u8_t *emv_rx_buf, u32_t *response_length)
{
	s32_t rv = 0;
	u8_t tx_buf[5], ins_byte;
	u32_t bytes_received;
	u8_t rx_buf[SC_MAX_EMV_BUFFER_SIZE];
	u32_t cnt;

	tx_buf[0] = 0x00;
	tx_buf[1] = 0xC0;
	tx_buf[2] = 0x00;
	tx_buf[3] = 0x00;
	tx_buf[4] = expected_length;

	/* Send GET RESPONSE Command To ICC */
	memcpy(handle->tx_buf, tx_buf, 5);
	handle->tx_len = 5;
	rv = sc_cli_transmit(handle);
	ins_byte = 0xC0;

	rv = cli_emv_t0_receive(handle, rx_buf, ins_byte,
				expected_length, true,
				&bytes_received);

	/* No expected data, so params 3 and 4 are sent to zero */
	TC_DBG("[%s:%d]", __func__, __LINE__);
	if ((rv) && (bytes_received != 2))
		return rv;

	if (bytes_received == 2) {
		if (((expected_length == 0) && (rx_buf[0] == 0x6C)) ||
		    (rx_buf[0] == 0x61)) {
			rv = cli_emv_get_response(handle, (u32_t)(rx_buf[1]),
			emv_rx_buf, response_length);
			if (rv)
				return rv;

			return 0;
		} else if (((rx_buf[0] == 0x62) && (rx_buf[1] == 0x81)) ||
			   ((rx_buf[0] == 0x67) && (rx_buf[1] == 0x00)) ||
			   ((rx_buf[0] == 0x6F) && (rx_buf[1] == 0x00)) ||
			   ((rx_buf[0] == 0x6A) && (rx_buf[1] == 0x86))) {
			/* Warning byte received, simply return success */
			TC_DBG("Warning bytes received in Get Response\n");
			return 0;
		}
		TC_DBG("Failing in Get Response prior to point A");
		return -EINVAL;
	}

	if (bytes_received == (expected_length + 2)) {
		if ((rx_buf[expected_length] == 0x90) &&
		    (rx_buf[expected_length + 1] == 0x00)) {
			/*
			 * expected_length+2 since two additional status bytes
			 * are sent to acknowledge command completion
			 */
			for (cnt = 0; cnt < expected_length; cnt++)
				emv_rx_buf[*response_length + cnt] =
								 rx_buf[cnt];

			*response_length += expected_length;

			if (*response_length < 5)
				emv_rx_buf[4] = 0x00;

			return 0;
		} else if ((rx_buf[expected_length] == 0x61) &&
			   (rx_buf[expected_length + 1] != 0x00)) {
			/*
			 * If another SmartCardEMVget_response is required,
			 * then recursively call it, incrementing the buffer
			 * position so that only data is stored in the buffer
			 */

			for (cnt = 0; cnt < expected_length; cnt++)
				emv_rx_buf[*response_length + cnt] =
								 rx_buf[cnt];

			*response_length += expected_length;

			rv = cli_emv_get_response(handle,
					(u32_t)(rx_buf[expected_length + 1]),
					emv_rx_buf, response_length);
			if (rv)
				return rv;

			return 0;
		} else if (((rx_buf[expected_length] == 0x62) &&
			    (rx_buf[expected_length + 1] == 0x81)) ||
			   ((rx_buf[expected_length] == 0x67) &&
			    (rx_buf[expected_length + 1] == 0x00)) ||
			   ((rx_buf[expected_length] == 0x6F) &&
			    (rx_buf[expected_length + 1] == 0x00)) ||
			   ((rx_buf[expected_length] == 0x6A) &&
			    (rx_buf[expected_length + 1] == 0x86))) {
			/* Warning byte received, simply return success */
			TC_DBG("Warning bytes received in Get Response\n");
			return 0;
		}
		for (cnt = 0; cnt < expected_length; cnt++)
			emv_rx_buf[*response_length + cnt] = rx_buf[cnt];

		*response_length += expected_length;
		return 0;
	} else {
		return -EINVAL;
	}
	return 0;
}
