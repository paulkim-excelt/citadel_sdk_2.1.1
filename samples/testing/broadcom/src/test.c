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

#include "test.h"

/* Zephyr v1.14 logging */
#include <logging/sys_log.h>
#ifdef LOG_MODULE_REGISTER
#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#endif

static SHELL_TEST_DECLARE(test_loop);
static SHELL_TEST_DECLARE(test_all);

static SHELL_TEST_DECLARE(test_not_available)
{
	ARG_UNUSED(argc);

	TC_ERROR("%s test not available\n", argv[0]);

	return -1;
}

#pragma weak test_adc = test_not_available
#pragma weak test_bbl = test_not_available
#pragma weak test_clocks = test_not_available
#pragma weak test_cpu_speed = test_not_available
#pragma weak test_crypto = test_not_available
#pragma weak test_pka = test_not_available
#pragma weak test_openssl_pka = test_not_available
#pragma weak test_dma = test_not_available
#pragma weak test_dmu = test_not_available
#pragma weak test_fp_spi = test_not_available
#pragma weak test_gpio = test_not_available
#pragma weak test_i2c = test_not_available
#pragma weak test_mbedtls = test_not_available
#pragma weak test_pwm = test_not_available
#pragma weak test_printer = test_not_available
#pragma weak test_qspi_flash_main = test_not_available
#pragma weak test_rtc = test_not_available
#pragma weak test_sdio = test_not_available
#pragma weak test_smau = test_not_available
#pragma weak test_smc = test_not_available
#pragma weak test_sotp_main = test_not_available
#pragma weak test_spi = test_not_available
#pragma weak test_spi_ind = test_not_available
#pragma weak test_spl = test_not_available
#pragma weak test_spi_legacy = test_not_available
#pragma weak test_timer = test_not_available
#pragma weak test_cpu = test_not_available
#pragma weak test_adc_msrr = test_not_available
#pragma weak test_ft_msrr = test_not_available
#pragma weak test_keypad = test_not_available
#pragma weak test_pm_lprun = test_not_available
#pragma weak test_pm_wfi = test_not_available
#pragma weak test_pm_drips = test_not_available
#pragma weak test_pm_deepsleep = test_not_available
#pragma weak test_sc = test_not_available
#pragma weak test_unicam = test_not_available
#pragma weak test_usbd_cdc_acm = test_not_available
#pragma weak test_usbh = test_not_available
#pragma weak test_zbar = test_not_available
#pragma weak test_lcd = test_not_available
#pragma weak test_wdt = test_not_available

SHELL_STATIC_SUBCMD_SET_CREATE(test_commands,
	SHELL_CMD(AUTOMATED_TESTS, NULL, " ", NULL),
	SHELL_CMD(adc, NULL, "Run ADC driver tests", test_adc),
	SHELL_CMD(clock_driver, NULL, "Run Clock driver tests", test_clocks),
	SHELL_CMD(crypto_driver, NULL, "Run crypto driver tests", test_crypto),
	SHELL_CMD(pka_driver, NULL, "Run pka driver tests", test_pka),
	SHELL_CMD(dma_driver, NULL, "Run DMA driver tests", test_dma),
	SHELL_CMD(dmu_driver, NULL, "Run DMU tests", test_dmu),
	SHELL_CMD(fp_spi, NULL, "Finger print sensor enumeration test", test_fp_spi),
	SHELL_CMD(gpio_driver, NULL, "Run GPIO driver tests", test_gpio),
	SHELL_CMD(i2c_driver, NULL, "Run I2C driver tests", test_i2c),
	SHELL_CMD(mbedtls, NULL, "Run mbedtls tests", test_mbedtls),
	SHELL_CMD(pwm_driver, NULL, "Run pwm driver tests", test_pwm),
	SHELL_CMD(qspi_driver, NULL, "Run QSPI driver tests", test_qspi_flash_main),
	SHELL_CMD(rtc_driver, NULL, "Run RTC driver tests", test_rtc),
	SHELL_CMD(smau_driver, NULL, "Run smau tests", test_smau),
	SHELL_CMD(sdio_driver, NULL, "Run SDIO tests", test_sdio),
	SHELL_CMD(smc_driver, NULL, "Run smc tests", test_smc),
	SHELL_CMD(spl_driver, NULL, "Run spl tests", test_spl),
	SHELL_CMD(sotp_driver, NULL, "Run SOTP driver tests: "
			"If you want to set the keys then enter command : "
			"sotp_driver set_key", test_sotp_main),
	SHELL_CMD(spi_driver, NULL, "Run SPI Legacy driver tests", test_spi),
	SHELL_CMD(spi_legacy_driver, NULL, "Run SPI Legacy driver tests", test_spi_legacy),
	SHELL_CMD(timer_driver, NULL, "Run Timer driver tests", test_timer),
	SHELL_CMD(cpu, NULL, "Run CPU tests", test_cpu),
	SHELL_CMD(lprun, NULL, "Run PM test to Switch CPU to Low Power Run mode", test_pm_lprun),
	/* Anything after the "all" command will not be run by the all test
	 * command
	 */
	SHELL_CMD(all, NULL, "Run all of the tests above", test_all),
	SHELL_CMD(loop, NULL, "Run tests in a loop", test_loop),
	SHELL_CMD(MANUAL_TESTS, NULL, " ", NULL),
	SHELL_CMD(bbl, NULL, "Run BBL test", test_bbl),
	SHELL_CMD(adc_msrr, NULL, "Run ADC MSRR driver tests", test_adc_msrr),
	SHELL_CMD(ft_msrr, NULL, "Run FT MSRR driver tests", test_ft_msrr),
	SHELL_CMD(keypad, NULL, "Run keypad test", test_keypad),
	SHELL_CMD(wfi, NULL, "Run PM Simple WFI Sleep tests", test_pm_wfi),
	SHELL_CMD(drips, NULL, "Run PM DRIPS Sleep tests", test_pm_drips),
	SHELL_CMD(deepsleep, NULL, "Run PM Deep Sleep tests", test_pm_deepsleep),
	SHELL_CMD(printer_driver, NULL, "Run printer driver tests", test_printer),
	SHELL_CMD(sc_driver, NULL, "Run Smart card tests", test_sc),
	SHELL_CMD(unicam_driver, NULL, "Run Unicam driver test", test_unicam),
	SHELL_CMD(usbd, NULL, "Run USB device CDC-ACM test", test_usbd_cdc_acm),
	SHELL_CMD(usbh_driver, NULL, "Run usbhost tests", test_usbh),
	SHELL_CMD(zbar, NULL, "Run zbar tests", test_zbar),
	SHELL_CMD(lcd, NULL, "Run lcd tests", test_lcd),
	SHELL_CMD(wdt, NULL, "Run wdt tests", test_wdt),
	SHELL_CMD(openssl_pka, NULL, "Run OpenSSL PKA tests", test_openssl_pka),
	SHELL_CMD(spi, NULL, "Run independent SPI master & slave tests", test_spi_ind),
	SHELL_CMD(cpu_speed, NULL, "Run CPU Speed tests", test_cpu_speed),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

void test_main(void)
{
	ARG_UNUSED(test_commands);
}

static SHELL_TEST_DECLARE(test_loop)
{
	const SHELL_CMD_ENTRY *test;
	unsigned int loop_count, i;
	char **test_argv;
	int test_argc;
	char *cmd;

	if (argc >= 2)
		cmd = argv[1];
	else
		cmd = "all";

	/* Find the matching command in the list */
	for (test = TEST_CMDS;
	     test != TEST_CMDS + (ARRAY_SIZE(TEST_CMDS) - 1);
	     test++) {
		if (!strcmp(CMD_NAME(test), cmd))
			break;
	}

	/* Not found in list */
	if (!CMD_HANDLER(test))
		return -1;

	if (argc >= 3) {
		/* Determine number of iterations */
		loop_count = atoi(argv[2]);

		/* Determine argc/argv to pass to the client */
		test_argc = argc - 3;
		test_argv = argv + argc;
	} else {
		loop_count = 0;

		test_argc = 0;
		test_argv = NULL;
	}

	/* Run it */
	for (i = 0; (loop_count == 0) || (i < loop_count); i++)
		CALL_TEST_HANDLER(test, test_argc, test_argv);

	return 0;
}

static SHELL_TEST_DECLARE(test_all)
{
	const SHELL_CMD_ENTRY *test;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Run though the list of commands, skipping over all and loop to
	 * prevent rerunning the commands multiple times
	 */
	for (test = TEST_CMDS;
	     test != TEST_CMDS + ARRAY_SIZE(TEST_CMDS);
	     test++) {
		if (!strcmp(CMD_NAME(test), "all"))
			break;

		if (CMD_HANDLER(test)) {
			char *args[1] = {(char *)CMD_NAME(test)};

			CALL_TEST_HANDLER(test, 1, args);
		}
	}

	return 0;
}

/* getch implementation */
#ifdef CONFIG_CONSOLE_GETCHAR
/* For Zephyr v1.14 */
#define MAX_RX_CHARS_PER_LINE	16
#define INC(A)	A = (A + 1) & (MAX_RX_CHARS_PER_LINE - 1)
static u8_t rx_chars[MAX_RX_CHARS_PER_LINE] = {0xFF};
static u8_t rd_idx, wr_idx;
s32_t getch(void)
{
	u8_t c;
	struct device *dev = device_get_binding(CONFIG_UART_SHELL_ON_DEV_NAME);

	if (rd_idx != wr_idx) {
		goto done;
	}

	z_impl_uart_irq_rx_disable(dev);
	while (uart_poll_in(dev, &c));
	do {
		bool can_write;

		can_write = rd_idx > wr_idx ? (rd_idx - wr_idx) > 1
				: MAX_RX_CHARS_PER_LINE - (wr_idx - rd_idx) > 1;
		if (can_write && c) {
			rx_chars[wr_idx] = c;
			INC(wr_idx);
		}
		k_busy_wait(500);
	} while (uart_poll_in(dev, &c) == 0);
	z_impl_uart_irq_rx_enable(dev);

done:
	c = rx_chars[rd_idx];
	INC(rd_idx);
	return c;
}
#elif defined CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
/* For Zephyr v1.9 */
u8_t rx_char;
static volatile bool received;

s32_t uart_in_hook(u8_t c)
{
	rx_char = c;
	received = true;

	return c;
}

s32_t getch(void)
{
	received = false;
	uart_console_in_debug_hook_install(uart_in_hook);
	while (!(received)) {
		k_sleep(1);
	}
	uart_console_in_debug_hook_install(NULL);

	return rx_char;
}
#else
s32_t getch(void)
{
	return -ENOTSUP;
}
#endif

SHELL_REGISTER_TEST(test, test_commands, "Brcm driver test commands", NULL);

