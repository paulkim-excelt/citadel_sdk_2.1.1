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
 * @file test_emv2000_t0.h
 *
 * @brief  EMV2000 T0 header file
 */

#ifndef TEST_EMV2000_T0_H__
#define TEST_EMV2000_T0_H__

/* EMV Smart Card message types.
 * For EMV2000 test 1738 and 1740, we need this to tell if the command
 * is 3 or 4
 */
#define SC_CLI_EMV_APPLICATION_BLOCK			(0x1E)	/* case 3 */
#define SC_CLI_EMV_APPLICATION_UNBLOCK			(0x18)	/* case 3 */
#define SC_CLI_EMV_CARD_BLOCK				(0x16)	/* case 3 */
#define SC_CLI_EMV_EXT_AUTHENTICATE			(0x82)	/* case 3 */
#define SC_CLI_EMV_GEN_APPL_CRYPTO			(0xAE)	/* case 4 */
#define SC_CLI_EMV_GET_CHALLENGE			(0x84)	/* case 4 */
#define SC_CLI_EMV_GET_DATA				(0xCA)	/* case 4 */
#define SC_CLI_EMV_GET_PROC_OPTIONS			(0xA8)	/* case 4 */
#define SC_CLI_EMV_INT_AUTHENTICATE			(0x88)	/* case 4 */
#define SC_CLI_EMV_PIN					(0x24)	/* case 3 */
#define SC_CLI_EMV_READ_RECORD				(0xB2)	/* case 4 */
#define SC_CLI_EMV_VERIFY				(0x20)	/* case 3 */
#define SC_CLI_EMV2000_SELECT				(0xA4)	/* case 4 */

#define SC_CLI_NO_DATA_EXPECTED				(0)
#define SC_CLI_DATA_EXPECTED				(1)

enum SC_CLI_EMV_COMMAND_TYPE {
	SC_CLI_EMV_COMMAND_TYPE_CASE_1OR2,
	SC_CLI_EMV_COMMAND_TYPE_CASE_3OR4,
	SC_CLI_EMV_COMMAND_TYPE_TEST_ROUTINE_CASE_4,
	SC_CLI_EMV_COMMAND_TYPE_EOT_CMD
};

s32_t sc_cli_emv_get_response(struct sc_cli_channel_handle *handle,
			      u32_t unexpected_len, u8_t *rx_buf,
			      u32_t response_len);

s32_t sc_cli_emv_t0_receive(struct sc_cli_channel_handle *handle, u8_t *rx_buf,
			      u8_t instruction, u32_t expected_len,
			      u32_t rx_len);

s32_t sc_cli_emv_t0_case_3or4_cmd(struct sc_cli_channel_handle *handle,
				  u8_t *tx_buf, u32_t length,
				  u8_t *response_buf, u32_t *response_len);

s32_t sc_cli_emv_t0_case_1or2_cmd(struct sc_cli_channel_handle *handle,
				  u8_t *tx_buf, u32_t length,
				  u8_t *response_buf, u32_t *response_len);
#endif /* TEST_EMV2000_T0_H__ */
