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
 * @file test_pm.c
 * @brief  Power Manager driver test
 *
 * @detail
 * Automatic Testing of the Power Manager:
 * ---------------------------------------
 * 1. Requires No User Interaction other than invoking the test.
 * 2. Code need to be built using PM_TEST=1 flag.
 * 3. The wakeup routine, i.e. exit routine to take system out of LP mode
 *    is triggered after a defined delay.
 *
 * Manual Testing of the Power Manager:
 * ------------------------------------
 * 1. Requires User Interaction other than invoking the test.
 * 2. User needs to change state of the switch SW11.1 on SVK board to
 *    trigger AON GPIO 0.
 * 3. Code need to be built without PM_TEST=1 flag.
 * 4. The wakeup routine, i.e. exit routine to take system out of LP mode
 *    is triggered after user toggles the state of the switch.
 *
 *
 * 1. Manual testing can be used to measures current & voltage
 *    after entering LP mode.
 *
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <bbl.h>
#include <pm.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

/********************************************
 *********** Automatic Tests ****************
 ********************************************
 *
 * Note: These tests require no User Intervetion.
 */
SHELL_TEST_DECLARE(test_pm_lprun)
{
	struct device *pm;

	pm = device_get_binding(CONFIG_PM_NAME);
	if (!pm) {
		TC_ERROR("Cannot get PM controller\n");
		return TC_FAIL;
	}

	if (argc > 1) {
		u32_t cpufreq = atoi(argv[1]);
		switch(cpufreq) {
			case 200:
				/**< run at 200MHz */
				pm_enter_target_state(pm, MPROC_PM_MODE__RUN_200);
				break;
			case 500:
				/**< run at 500MHz */
				pm_enter_target_state(pm, MPROC_PM_MODE__RUN_500);
				break;
			case 1000:
				/**< run at 1000MHz */
				pm_enter_target_state(pm, MPROC_PM_MODE__RUN_1000);
				break;
			default: break;
		}
	}

	TC_PRINT("CPU Frequency Test passed\n\n");
	return TC_PASS;
}

static void wakeup_from_sleep_isr(struct device *dev)
{
	ARG_UNUSED(dev);

	TC_PRINT("....Woke up from Simple WFI Sleep....\n");
}

static void wakeup_from_drips_isr(struct device *dev)
{
	ARG_UNUSED(dev);

	TC_PRINT("....Woke up from DRIPS Sleep....\n");
}

SHELL_TEST_DECLARE(test_pm_wfi)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *pm;
	u32_t cnt = 0;

	pm = device_get_binding(CONFIG_PM_NAME);
	if (!pm) {
		TC_ERROR("Cannot get PM controller\n");
		return TC_FAIL;
	}

	while(cnt < 10) {
		TC_PRINT("Count: %d\n", cnt);
		cnt++;
	}

	/* Configure to wakeup on Smart Card Insertion
	 * AON GPIO 3, Level Triggerred, Both Edges, Rising Edge / High Level
	 */
	pm_set_aongpio_wakeup_source(pm, 3, 0, 1, 0, 1);

	/* Configure the Wakeup ISR for AON GPIO 3 */
	pm_set_aon_gpio_irq_callback(pm, wakeup_from_sleep_isr, 3);

	/* Put MPROC (i.e. A7) to Sleep */
	pm_enter_target_state(pm, MPROC_PM_MODE__SLEEP);

	TC_PRINT("Woke Up from Simple WFI Sleep.\n");

	while(cnt < 20) {
		TC_PRINT("count: %d\n", cnt);
		cnt++;
	}

	TC_PRINT("Simple WFI Sleep Test passed\n\n");
	return TC_PASS;
}

/* Tamper test can be carried out for any type of tamper.
 * But with SVK, triggering Voltage/Temp/Frequency tampers
 * is rather difficult, so we resort to testing external
 * mesh tamper only.
 */
static void configure_ext_mesh_tamper(void)
{
	int ret, i;
	struct bbl_tamper_config cfg;
	struct bbl_ext_mesh_config emesh_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Disable callback first */
	ret = bbl_register_tamper_detect_cb(dev, NULL);
	zassert_equal(ret, 0, NULL);

	/* First run static test */
	cfg.tamper_type = BBL_TAMPER_DETECT_EXTERNAL_MESH;
	cfg.tamper_cfg = &emesh_cfg;
	emesh_cfg.dyn_mesh_enable = false;
	emesh_cfg.static_pin_mask = 0;
	/* Enable all digital input tamper sources.
	 *
	 * Notes on SERP Board:
	 * On SERP board, the first pair of n & p tampers are available to be
	 * configured in static mode. n[0] is connected to Vcc & p[0] is connected
	 * to Gnd. The second pair, i.e. n[1] & p[1] lines are configurable in dynamice mode.
	 * A jumper must be installed between pin 3 & 4 to complete the loop if dynamic
	 * tamper is to be enabled. To generate tamper event on P1/N1, remove the jumper
	 * on pins 3,4 for tamper event.
	 *
	 * All the remaining 3 pairs are by default looped on the board to operate
	 * dynamic mode. So if these are forced to be configured in static mode, this
	 * will instantly generate a tamper event.
	 *
	 */
#ifdef CONFIG_BOARD_SERP_CP
	ARG_UNUSED(i);
	emesh_cfg.static_pin_mask |= BBL_TAMPER_STATIC_INPUT_MASK_N(0) | BBL_TAMPER_STATIC_INPUT_MASK_P(0);
#else
	for (i = 0; i < BBL_TAMPER_P_0; i++) {
		emesh_cfg.static_pin_mask |= BBL_TAMPER_STATIC_INPUT_MASK_N(i) | BBL_TAMPER_STATIC_INPUT_MASK_P(i);
	}
#endif

	TC_PRINT("Set SW4.6 to off position\nHit any key ...\n");
	getch();

	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
	TC_PRINT("Now set switches SW3.[1..5] to on to trigger a tamper_n[] event\n"
		"And set switches SW4.[1..5] to on to trigger a tamper_p[] event\n"
		"Verify the correct tamper event is generated from the console log\n");
}

/* Disable whatever was enabled for test purpose. */
static void disable_ext_mesh_tamper(void)
{
	int ret;
	struct bbl_tamper_config cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Disable external mesh */
	cfg.tamper_type = BBL_TAMPER_DETECT_EXTERNAL_MESH;
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

SHELL_TEST_DECLARE(test_pm_drips)
{
	struct device *pm;
	u32_t cnt = 20;

	pm = device_get_binding(CONFIG_PM_NAME);
	if (!pm) {
		TC_ERROR("Cannot get PM controller\n");
		return TC_FAIL;
	}

	while(cnt > 10) {
		TC_PRINT("Count: %d\n", cnt);
		cnt--;
	}

	if (argc > 1) {
		if(strcmp(argv[1], "tamper") == 0) {
			/* Configure External Mesh tamper as wake-up source */
			configure_ext_mesh_tamper();

			/* Enable AON GPIOs */
#ifdef CONFIG_BOARD_SERP_CP
			pm_set_aongpio_wakeup_source(pm, 2, 0, 0, 0, 1);
			pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 2);
#else
			pm_set_aongpio_wakeup_source(pm, 0, 0, 1, 0, 1);
			pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 0);
			pm_set_aongpio_wakeup_source(pm, 3, 0, 1, 0, 1);
			pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 3);
#endif
			/* Disable RTC Wakeup - in case it doesn't wake via tamper
			 * it can be woken up by AON GPIO. This also clears Pending
			 * Tamper Events.
			 */
			pm_set_rtc_wakeup_source(pm, 0);

			TC_PRINT("Entering DRIPS Sleep\n");

			/* Put MPROC (i.e. A7) to DRIPS */
			pm_enter_target_state(pm, MPROC_PM_MODE__DRIPS);

			TC_PRINT("Woke Up from DRIPS Sleep\n");
			k_sleep(500);

			/* Disable External Mesh tamper */
			disable_ext_mesh_tamper();
		}
		else {
			u32_t i, num_iter, delay = 0;
			num_iter = atoi(argv[1]);
			if(num_iter == 0)
				num_iter = 1;
			if(argc > 2)
				delay = atoi(argv[2]);
			for (i = 0; i < num_iter; i++) {
				/* Enable AON GPIOs */
#ifdef CONFIG_BOARD_SERP_CP
				pm_set_aongpio_wakeup_source(pm, 2, 0, 0, 0, 1);
				pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 2);
#else
				pm_set_aongpio_wakeup_source(pm, 0, 0, 1, 0, 1);
				pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 0);
				pm_set_aongpio_wakeup_source(pm, 3, 0, 1, 0, 1);
				pm_set_aon_gpio_irq_callback(pm, wakeup_from_drips_isr, 3);
#endif
				/* Enable RTC Wakeup after delay second, when delay is '0'
				 * RTC is disabled. This also clears Pending Tamper Events.
				 */
				pm_set_rtc_wakeup_source(pm, delay);

				/* Put MPROC (i.e. A7) to DRIPS */
				pm_enter_target_state(pm, MPROC_PM_MODE__DRIPS);

				TC_PRINT("Woke Up from DRIPS Sleep: %d\n", i);
				k_sleep(500);
			}
		}
	}

	while(cnt > 0) {
		TC_PRINT("count: %d\n", cnt);
		cnt--;
	}

	TC_PRINT("DRIPS Test passed\n\n");
	return TC_PASS;
}

SHELL_TEST_DECLARE(test_pm_deepsleep)
{
	struct device *pm;
	u32_t cnt = 50;

	pm = device_get_binding(CONFIG_PM_NAME);
	if (!pm) {
		TC_ERROR("Cannot get PM controller\n");
		return TC_FAIL;
	}

	while(cnt > 30) {
		TC_PRINT("Count: %d\n", cnt);
		cnt = cnt - 5;
	}

	/* Enable AON GPIOs */
#ifdef CONFIG_BOARD_SERP_CP
	pm_set_aongpio_wakeup_source(pm, 2, 0, 0, 0, 1);
#else
	pm_set_aongpio_wakeup_source(pm, 0, 0, 1, 0, 1);
	pm_set_aongpio_wakeup_source(pm, 3, 0, 1, 0, 1);
#endif

	if (argc > 1) {
		if(strcmp(argv[1], "tamper") == 0) {
			/* Configure External Mesh tamper as wake-up source */
			configure_ext_mesh_tamper();

			/* Disable RTC Wakeup
			 * This also clears Pending Tamper Events.
			 */
			pm_set_rtc_wakeup_source(pm, 0);
		}
		else {
			u32_t delay = 0;
			delay = atoi(argv[1]);

			/* Enable RTC Wakeup after delay second, when delay is '0'
			 * RTC is disabled. This also clears Pending Tamper Events.
			 */
			pm_set_rtc_wakeup_source(pm, delay);
		}
	}

	/* Put MPROC (i.e. A7) to DEEP SLEEP */
	pm_enter_target_state(pm, MPROC_PM_MODE__DEEPSLEEP);

	/* Control Will never reach here */
	return TC_PASS;
}
