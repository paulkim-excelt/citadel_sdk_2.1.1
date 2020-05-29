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
/* Driver tests */

#ifndef __BRCM_TEST_H__
#define __BRCM_TEST_H__

#ifdef __ZEPHYR__
#include <version.h>
#else
#define KERNEL_VERSION_MAJOR	1
#define KERNEL_VERSION_MINOR	9
#endif

#include <misc/printk.h>
#include <misc/util.h>
#include <kernel.h>
#include <stddef.h>
#include <stdlib.h>
#include <ztest.h>
#include <zephyr/types.h>
#include <shell/shell.h>

#if (KERNEL_VERSION_MAJOR == 1) && (KERNEL_VERSION_MINOR == 9)
#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...)		\
	const struct shell_cmd name[] = {\
		__VA_ARGS__					\
	};

#define SHELL_CMD(CMD_NAME, SUB_CMD, HELP, FUNC)	\
	{STRINGIFY(CMD_NAME), FUNC, HELP}

#define SHELL_SUBCMD_SET_END	{NULL, NULL, NULL}

#define SHELL_REGISTER_TEST(A, B, C, D) SHELL_REGISTER(STRINGIFY(A), B)
#define TEST_CMDS		test_commands
#define SHELL_CMD_ENTRY 	struct shell_cmd
#define CMD_NAME(TEST)		TEST->cmd_name
#define CMD_HANDLER(TEST)	TEST->cb
#define SHELL_TEST_DECLARE(FUNC)	int FUNC(int argc, char *argv[])
#define SHELL_TEST_CALL(FUNC, ARGC, ARGV)	FUNC(ARGC, ARGV)
#define CALL_TEST_HANDLER(TEST, ARGC, ARGV)	CMD_HANDLER(TEST)(ARGC, ARGV)

#else /* Zephyr 1.14 and higher */

#define SHELL_REGISTER_TEST(A, B, C, D)	SHELL_CMD_REGISTER(A, &B, C, D)
#define TEST_CMDS		shell_test_commands
#define SHELL_CMD_ENTRY  	struct shell_static_entry
#define CMD_NAME(TEST)		TEST->syntax
#define CMD_HANDLER(TEST)	TEST->handler
#define SHELL_TEST_DECLARE(FUNC)	int FUNC(const struct shell *sh, size_t argc, char **argv)
#define SHELL_TEST_CALL(FUNC, ARGC, ARGV)	FUNC(sh, ARGC, ARGV)
#define CALL_TEST_HANDLER(TEST, ARGC, ARGV)	CMD_HANDLER(TEST)(NULL, ARGC, ARGV)
#endif

#include <uart.h>
#include <console/uart_console.h>
s32_t getch(void);

SHELL_TEST_DECLARE(test_adc);
SHELL_TEST_DECLARE(test_adc_msrr);
SHELL_TEST_DECLARE(test_bbl);
SHELL_TEST_DECLARE(test_clocks);
SHELL_TEST_DECLARE(test_cpu_speed);
SHELL_TEST_DECLARE(test_crypto);
SHELL_TEST_DECLARE(test_ft_msrr);
SHELL_TEST_DECLARE(test_pka);
SHELL_TEST_DECLARE(test_openssl_pka);
SHELL_TEST_DECLARE(test_dma);
SHELL_TEST_DECLARE(test_dmu);
SHELL_TEST_DECLARE(test_fp_spi);
SHELL_TEST_DECLARE(test_gpio);
SHELL_TEST_DECLARE(test_i2c);
SHELL_TEST_DECLARE(test_keypad);
SHELL_TEST_DECLARE(test_mbedtls);
SHELL_TEST_DECLARE(test_pwm);
SHELL_TEST_DECLARE(test_printer);
SHELL_TEST_DECLARE(test_qspi_flash_main);
SHELL_TEST_DECLARE(test_rtc);
SHELL_TEST_DECLARE(test_sc);
SHELL_TEST_DECLARE(test_sdio);
SHELL_TEST_DECLARE(test_smau);
SHELL_TEST_DECLARE(test_smc);
SHELL_TEST_DECLARE(test_sotp_main);
SHELL_TEST_DECLARE(test_spi);
SHELL_TEST_DECLARE(test_spi_ind);
SHELL_TEST_DECLARE(test_spi_legacy);
SHELL_TEST_DECLARE(test_spl);
SHELL_TEST_DECLARE(test_timer);
SHELL_TEST_DECLARE(test_pm_lprun);
SHELL_TEST_DECLARE(test_pm_wfi);
SHELL_TEST_DECLARE(test_pm_drips);
SHELL_TEST_DECLARE(test_pm_deepsleep);
SHELL_TEST_DECLARE(test_pm);
SHELL_TEST_DECLARE(test_unicam);
SHELL_TEST_DECLARE(test_usbd_cdc_acm);
SHELL_TEST_DECLARE(test_usbh);
SHELL_TEST_DECLARE(test_lcd);
SHELL_TEST_DECLARE(test_wdt);

/* Other tests */
SHELL_TEST_DECLARE(test_cpu);
SHELL_TEST_DECLARE(test_zbar);

#endif /* __BRCM_TEST_H__ */
