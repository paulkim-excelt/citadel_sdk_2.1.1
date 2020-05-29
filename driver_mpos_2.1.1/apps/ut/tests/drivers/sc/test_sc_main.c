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
 * @file test_sc_main.c
 *
 * @brief  smart card cli main
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>
#include <sc/sc_datatypes.h>
#include "test_sc_infra.h"
#include <test.h>

s32_t sc_cli_open_channel_menu(struct sc_cli_handle *cli_handle,
			       struct  sc_cli_channel_handle *handle,
			       u8_t channel_no, enum sc_standard standard);

struct sc_cli_handle main_cli_handle;

s32_t sc_test_menu(struct sc_cli_handle *cli_handle, u8_t channel_no)
{
	struct sc_cli_channel_handle *handle = &cli_handle->handle[channel_no];
	s32_t command_id;
	s32_t rv = 0;

	handle->channel_no = channel_no;
	handle->dev = device_get_binding(CONFIG_SC_BCM58202_DEV_NAME);

	while (1) {
		TC_PRINT("\nSelect SmartCard Standard:\n");
		TC_PRINT("0) EXIT\n");
		TC_PRINT("%d) NDS\n", SC_STANDARD_NDS);
		TC_PRINT("%d) ISO\n", SC_STANDARD_ISO);
		TC_PRINT("%d) EMV1996\n", SC_STANDARD_EMV1996);
		TC_PRINT("%d) EMV2000\n", SC_STANDARD_EMV2000);
		TC_PRINT("%d) Irdeto, use external clock 24MHz\n",
							SC_STANDARD_IRDETO);
		TC_PRINT("%d) ARIB, , use external clock 24MHz\n",
							SC_STANDARD_ARIB);
		TC_PRINT("%d) MT\n", SC_STANDARD_MT);
		TC_PRINT("%d) Conax\n", SC_STANDARD_CONAX);
		TC_PRINT("%d) ES\n", SC_STANDARD_ES);
		TC_PRINT("10) SimpleT0Test using ACOS2 smartcard\n");
		TC_PRINT("11) APDU tests\n");
		TC_PRINT("12) ATR test\n");
		TC_PRINT("13) Test different Vcc level\n");
		TC_PRINT("14) Dell Java card test\n");
		TC_PRINT("15) Debit card test\n");

		command_id = prompt();
		if (command_id < 0)
			return -ENOTSUP;

		switch (command_id) {
		case 0:
			return 0; /* exit the test mode */
		case SC_STANDARD_NDS:
		case SC_STANDARD_ISO:
		case SC_STANDARD_EMV1996:
			TC_PRINT("Not Implemented\n");
			break;
		case SC_STANDARD_EMV2000:
			sc_cli_open_channel_menu(cli_handle, handle,
				channel_no, SC_STANDARD_EMV2000);
			sc_cli_emv_test_menu(handle);
			sc_cli_close_channel(handle);
			break;
		case SC_STANDARD_IRDETO:
		case SC_STANDARD_ARIB:
		case SC_STANDARD_MT:
		case SC_STANDARD_CONAX:
		case SC_STANDARD_ES:
			TC_PRINT("Not Implemented\n");
			break;

		/* Simple T0 Test */
		case 10:
			rv = sc_cli_open_channel_menu(cli_handle, handle,
					channel_no, SC_STANDARD_ISO);
			if (rv)
				return rv;

			rv = sc_cli_acos2_t0_test(handle);
			if (rv)
				TC_ERROR("T0 test FAIL\n");
			else
				TC_PRINT("T0 test PASS\n");

			sc_cli_close_channel(handle);
			break;

		/* ATR Test */
		case 12:
			rv = sc_cli_open_channel_menu(cli_handle, handle,
					channel_no, SC_STANDARD_ISO);
			if (rv)
				return rv;

			rv = sc_cli_atr_t0_test(handle);
			if (rv)
				TC_ERROR("ATR test FAIL\n");
			else
				TC_PRINT("ATR test PASS\n");
			sc_cli_close_channel(handle);
			break;

		/* Debit card test */
		case 15:
			rv = sc_cli_open_channel_menu(cli_handle, handle,
					channel_no, SC_STANDARD_ISO);
			if (rv)
				return rv;

			rv = sc_cli_atr_t0_test(handle);
			if (rv)
				TC_ERROR("ATR test FAIL\n");
			else
				TC_PRINT("ATR test PASS\n");

			rv = sc_cli_debit_card_test(handle);
			if (rv)
				TC_ERROR("Debit card test FAIL\n");
			else
				TC_PRINT("Debit card test PASS\n");
			sc_cli_close_channel(handle);
			break;

		default:
			TC_PRINT("\nInvalid Choice %d\n", command_id);
			break;
		}
	}

	return 0;
}


s32_t sc_cli_open_channel_menu(struct sc_cli_handle *cli_handle,
			       struct  sc_cli_channel_handle *handle,
			       u8_t channel_no, enum sc_standard standard)
{
	s32_t rv = 0;

	rv = sc_cli_get_channel_default_settings(cli_handle, handle);
	if (rv) {
		TC_ERROR("Failed to get channel default parameters");
		return -EINVAL;
	}

	handle->receive_decode_atr_func = NULL;

	switch (standard) {
	case SC_STANDARD_UNKNOWN:
		return 0;
	case SC_STANDARD_ISO:
		break;
	case SC_STANDARD_EMV2000:
		handle->ch_param.ctx_card_type.vcc_level = SC_VCC_LEVEL_5V;
		break;
	case SC_STANDARD_ARIB:
		break;
	default:
		TC_PRINT("\nInvalid Choice!, standard = %d", standard);
		break;
	}

	rv = sc_cli_open_channel(cli_handle, handle, channel_no,
				 &handle->ch_param);
	if (rv)
		TC_PRINT("Failed to open channel");
	else
		handle->is_init = true;

	return rv;
}

s32_t sc_cli_top_menu(void)
{
	sc_test_menu(&main_cli_handle, 0);

	return 0;
}

SHELL_TEST_DECLARE(test_sc)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	sc_cli_top_menu();

	return 0;
}
