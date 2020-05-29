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
 * @file test_dmu.c
 * @brief dmu driver test
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

#define CIT_SMBUS_ALERT_PROGRAM_DEFAULT		(0x400000)
#define CIT_CDRU_ARM_M3_CTRL_DEFAULT		(0x3d090)
#define CIT_CRMU_CLOCK_SWITCH_STATUS_DEFAULT	(0x01)

static s32_t dmu_access_test(void)
{
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	struct device *cdru = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	struct device *dru = device_get_binding(CONFIG_DMU_DRU_DEV_NAME);
	u32_t val;
	s32_t rv;

	/* Read the registers and check for default value */
	TC_PRINT("Read 'SMBUS_ALERT_PROGRAM' register and check for default\n");
	zassert_not_null(dmu, "Cannot get dmu device\n");
	val = dmu_read(dmu, CIT_SMBUS_ALERT_PROGRAM);
	if (val != CIT_SMBUS_ALERT_PROGRAM_DEFAULT) {
		TC_ERROR("DMU read doesn't match default value 0x%x 0x%x\n",
			CIT_SMBUS_ALERT_PROGRAM_DEFAULT, val);
		return TC_FAIL;
	}

	TC_PRINT("Read 'CDRU_ARM_M3_CTRL' register and check for default\n");
	zassert_not_null(cdru, "Cannot get cdru device\n");
	val = dmu_read(cdru, CIT_CDRU_ARM_M3_CTRL);
	if (val != CIT_CDRU_ARM_M3_CTRL_DEFAULT) {
		TC_ERROR("CDRU read doesn't match default value 0x%x 0x%x\n",
			CIT_CDRU_ARM_M3_CTRL_DEFAULT, val);
		return TC_FAIL;
	}

	TC_PRINT("Read 'CRMU_CLOCK_SWITCH_STATUS' reg and check for default\n");
	zassert_not_null(dru, "Cannot get dru device\n");
	val = dmu_read(dru, CIT_CRMU_CLOCK_SWITCH_STATUS);
	if (val != CIT_CRMU_CLOCK_SWITCH_STATUS_DEFAULT) {
		TC_ERROR("DRU read doesn't match default value 0x%x 0x%x\n",
			CIT_CRMU_CLOCK_SWITCH_STATUS_DEFAULT, val);
		return TC_FAIL;
	}

	/*
	 * Verify device shutdown and bringup.
	 * Use DMA controller for this test. The test will shutdown and
	 * bringup the DMA.
	 */
	TC_PRINT("\nTest DMA shutdown and bringup using 'dmu_device_ctrl'\n");
	zassert_not_null(dmu, "Cannot get dmu device\n");
	rv = dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_DMA, DMU_DEVICE_SHUTDOWN);
	if (rv) {
		TC_ERROR("Couldn't contol the device %d\n", rv);
		return TC_FAIL;
	}

	/* Check for shutdown */
	val = dmu_read(cdru, CIT_CDRU_SW_RESET_CTRL);
	if (val & BIT(DMU_DMA)) {
		TC_ERROR("Couldn't shutdown the DMA device");
		return TC_FAIL;
	}

	rv = dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_DMA, DMU_DEVICE_BRINGUP);
	if (rv) {
		TC_ERROR("Couldn't contol the device %d\n", rv);
		return TC_FAIL;
	}
	/* Check for bringup */
	val = dmu_read(cdru, CIT_CDRU_SW_RESET_CTRL);
	if (!(val & BIT(DMU_DMA))) {
		TC_ERROR("Couldn't bringup the DMA device\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

void dmu_test(void)
{
	zassert_true(dmu_access_test() == TC_PASS, NULL);
}

SHELL_TEST_DECLARE(test_dmu)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(dmu_driver_tests,
			 ztest_unit_test(dmu_test));

	ztest_run_test_suite(dmu_driver_tests);

	return 0;
}
