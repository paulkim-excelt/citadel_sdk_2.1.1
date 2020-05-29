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
 * @file test_bbl.c
 * @brief Battery backed Logic driver test
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <bbl.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <broadcom/gpio.h>
#include <pinmux.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <test.h>

/* BBL read/write apis */
int bbl_write_reg(u32_t addr, u32_t data);
int bbl_read_reg(u32_t addr, u32_t *data);

/* Flag to indicate if a manual test has been requested
 * This flag is used to run manual tamper tests, which
 * involve changing switch positions on the SVK as part
 * of the test.
 */
static u8_t manual_test;

/* API param tests */
static void test_bbram_api_read(void)
{
	int ret;
	u32_t data[2];
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Incorrect offset */
	ret = bbl_bbram_read(dev, BBRAM_NUM_WORDS, data, 1);
	zassert_equal(ret, -EINVAL, NULL);

	/* Null buffer */
	ret = bbl_bbram_read(dev, 0, NULL, 10);
	zassert_equal(ret, -EINVAL, NULL);

	/* Out of bounds read test 1 */
	ret = bbl_bbram_read(dev, BBRAM_NUM_WORDS - 1, data, 2);
	zassert_equal(ret, -EINVAL, NULL);

	/* Out of bounds read test 2 */
	ret = bbl_bbram_read(dev, 0, data, BBRAM_NUM_WORDS + 1);
	zassert_equal(ret, -EINVAL, NULL);

	/* Last word read */
	ret = bbl_bbram_read(dev, BBRAM_NUM_WORDS - 1, data, 1);
	zassert_equal(ret, 0, NULL);
}

static void test_bbram_api_write(void)
{
	int ret;
	u32_t data[2] = {0};
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Incorrect offset */
	ret = bbl_bbram_write(dev, BBRAM_NUM_WORDS, data, 1);
	zassert_equal(ret, -EINVAL, NULL);

	/* Null buffer */
	ret = bbl_bbram_write(dev, 0, NULL, 10);
	zassert_equal(ret, -EINVAL, NULL);

	/* Out of bounds write test 1 */
	ret = bbl_bbram_write(dev, BBRAM_NUM_WORDS - 1, data, 2);
	zassert_equal(ret, -EINVAL, NULL);

	/* Out of bounds write test 2 */
	ret = bbl_bbram_write(dev, 0, data, BBRAM_NUM_WORDS + 1);
	zassert_equal(ret, -EINVAL, NULL);

	/* Last word read */
	ret = bbl_bbram_write(dev, BBRAM_NUM_WORDS - 1, data, 1);
	zassert_equal(ret, 0, NULL);
}

static void test_bbram_api_config(void)
{
	int ret;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Null param */
	ret = bbl_bbram_configure(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

static void test_bbl_api_get_tamper_pin_state(void)
{
	int ret;
	u8_t state;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Null param */
	ret = bbl_get_tamper_pin_state(dev, 0, NULL);
	zassert_equal(ret, -EINVAL, NULL);

	/* Invalid pin */
	ret = bbl_get_tamper_pin_state(dev, BBL_TAMPER_P_4 + 1, &state);
	zassert_equal(ret, -EINVAL, NULL);
}

static void test_bbl_api_set_lfsr_params(void)
{
	int ret;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Null param */
	ret = bbl_set_lfsr_params(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

static void test_bbl_api_td_configure(void)
{
	int ret;
	struct bbl_tamper_config cfg;
	struct bbl_tmon_tamper_config tmon_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Null configuration param */
	ret = bbl_tamper_detect_configure(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);

	/* Invalid tamper type */
	cfg.tamper_type = BBL_TAMPER_DETECT_EXTERNAL_MESH + 1;
	cfg.tamper_cfg = &tmon_cfg;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, -EINVAL, NULL);
}

/* Functional tests */
static void test_bbram_config(void)
{
	int ret, i;
	struct bbl_bbram_config cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* bit flip enable = 0 */
	cfg.bit_flip_enable = false;
	ret = bbl_bbram_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* bit flip enable = 1 */
	cfg.bit_flip_enable = true;

	/* Set all possible values of Bit flip rate */
	for (i = BBL_BBRAM_BFR_9_HOURS; i <= BBL_BBRAM_BFR_72_HOURS; i++) {
		ret = bbl_bbram_configure(dev, &cfg);
		zassert_equal(ret, 0, NULL);
	}

	/* Reset BFR */
	cfg.bit_flip_enable = false;
	ret = bbl_bbram_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

static void test_bbram_read(void)
{
	int ret, i;
	u32_t data[4];
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	ret = bbl_bbram_erase(dev);
	zassert_equal(ret, 0, NULL);

	/* Read one word at a time */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data[0] = 0xFFFFFFFF;
		ret = bbl_bbram_read(dev, i, data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data[0], 0, NULL);
	}

	/* Read 4 words at a time */
	for (i = 0; i < BBRAM_NUM_WORDS; i += 4) {
		memset(data, 0xFF, sizeof(data));
		ret = bbl_bbram_read(dev, i, data, 4);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data[0], 0, NULL);
		zassert_equal(data[1], 0, NULL);
		zassert_equal(data[2], 0, NULL);
		zassert_equal(data[3], 0, NULL);
	}
}

/* pattern 0 = 0x03020100
 * pattern 1 = 0x07060504
 * ...
 */
#define OFFSET_PATTERN(I) (u32_t)(4*i + ((4*i + 1) << 8) + \
				  ((4*i + 2) << 16) + ((4*i + 3) << 24))

static void test_bbram_write(void)
{
	int ret, i;
	u32_t data;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Enable write access */
	ret = bbl_bbram_set_access(dev, true);
	zassert_equal(ret, 0, NULL);

	/* Write 0xA5 pattern and verify */
	data = 0xA5A5A5A5;
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		ret = bbl_bbram_write(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
	}
	/* Verify 0xA5 */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, 0xA5A5A5A5, NULL);
	}

	/* Write 0x5A pattern and verify */
	data = 0x5A5A5A5A;
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		ret = bbl_bbram_write(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
	}
	/* Verify 0x5A */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, 0x5A5A5A5A, NULL);
	}

	/* Write offset values and verify */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = OFFSET_PATTERN(i);
		ret = bbl_bbram_write(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
	}
	/* Verify */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, OFFSET_PATTERN(i), NULL);
	}
}

static void test_bbram_clear(void)
{
	int ret, i;
	u32_t data;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	ret = bbl_bbram_erase(dev);
	zassert_equal(ret, 0, NULL);

	/* Verify all of the secure memory has been cleared */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0xFFFFFFFF;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, 0, NULL);
	}

}

static void test_bbram_set_access(void)
{
	int ret, i;
	u32_t data;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Enable write access */
	ret = bbl_bbram_set_access(dev, true);
	zassert_equal(ret, 0, NULL);

	/* Write the offset pattern mem[i] = i */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = OFFSET_PATTERN(i);
		ret = bbl_bbram_write(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
	}

	/* Verify data is written */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, OFFSET_PATTERN(i), NULL);
	}

	/* Disable write access */
	ret = bbl_bbram_set_access(dev, false);
	zassert_equal(ret, 0, NULL);

	/* Write the reverse order offset pattern mem[39 - i] = i */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = OFFSET_PATTERN(BBRAM_NUM_WORDS - 1 - i);
		ret = bbl_bbram_write(dev, i, &data, 1);
		zassert_equal(ret, -EPERM, NULL);
	}

	/* Verify data is not written and previous data is still present */
	for (i = 0; i < BBRAM_NUM_WORDS; i++) {
		data = 0;
		ret = bbl_bbram_read(dev, i, &data, 1);
		zassert_equal(ret, 0, NULL);
		zassert_equal(data, OFFSET_PATTERN(i), NULL);
	}

	/* Restore write access */
	ret = bbl_bbram_set_access(dev, true);
	zassert_equal(ret, 0, NULL);
}

static void test_bbl_get_tamper_pin_state(void)
{
	int ret, i;
	u8_t state;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	for (i = 0; i <= BBL_TAMPER_P_4; i++) {
		ret = bbl_get_tamper_pin_state(dev, i, &state);
		/* Check return value is no error */
		zassert_equal(ret, 0, NULL);
		/* Check returned state is either 0 or 1 */
		zassert_equal(state >> 1, 0, NULL);
		TC_PRINT("Tamper pin %c%d state: %d\n",
			 i < BBL_TAMPER_P_0 ? 'N' : 'P', i, state);
	}
}

static void test_bbl_set_lfsr_params(void)
{
	int ret;
	struct bbl_lfsr_params cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	cfg.lfsr_update = true;
	cfg.lfsr_seed = 0x12345678;
	cfg.lfsr_period = BBL_LFSR_PERIOD_8_MS;
	cfg.mfm_enable = true;
	ret = bbl_set_lfsr_params(dev, &cfg);
	/* Check return val for successful lfsr param update
	 * There is no way to verify the functionality without probing the
	 * external mesh output and capturing the waveform.
	 */
	zassert_equal(ret, 0, NULL);
}

/* Tamper detected callback */
static u32_t tamper_event;
static u32_t tamper_timestamp;
static void reset_tamper_event(void)
{
	tamper_event = 0;
}
static void tamper_detected(u32_t event, u32_t timestamp)
{
	tamper_event = event;
	tamper_timestamp = timestamp;

	TC_PRINT("Tamper Event Detected (0x%08x at %d)\n", event, timestamp);
}

static void test_bbl_fmon_tamper_test(void)
{
	int ret;
	struct bbl_tamper_config cfg;
	struct bbl_fmon_tamper_config fmon_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Disable callback first */
	ret = bbl_register_tamper_detect_cb(dev, NULL);
	zassert_equal(ret, 0, NULL);

	/* There is no easy way to test the fmon tamper violation, so this
	 * test will call the fmon td config api with all possible params
	 * and check for api return value.
	 */
	cfg.tamper_type = BBL_TAMPER_DETECT_FREQUENCY_MONITOR;
	cfg.tamper_cfg = &fmon_cfg;
	fmon_cfg.low_freq_detect = true;
	fmon_cfg.high_freq_detect = true;

	/* Enable both limit violations */
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Only high freq tamper detect */
	fmon_cfg.low_freq_detect = false, fmon_cfg.high_freq_detect = true;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Only low freq tamper detect */
	fmon_cfg.low_freq_detect = true, fmon_cfg.high_freq_detect = false;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Disable freq limit tamper */
	fmon_cfg.low_freq_detect = false, fmon_cfg.high_freq_detect = false;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* NULL fmon tamper cfg to disable fmon */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

static void test_bbl_vmon_tamper_test(void)
{
	int ret, i, j;
	struct bbl_tamper_config cfg;
	struct bbl_vmon_tamper_config vmon_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Disable callback first */
	ret = bbl_register_tamper_detect_cb(dev, NULL);
	zassert_equal(ret, 0, NULL);

	/* There is no easy way to test the vmon tamper violation, so this
	 * test will call the vmon td config api with all possible params
	 * and check for api return value.
	 */
	cfg.tamper_type = BBL_TAMPER_DETECT_VOLTAGE_MONITOR;
	cfg.tamper_cfg = &vmon_cfg;

	for (i = 0; i <= BBL_TMON_LVD_THRESH_1_9_V + 1; i++) {
		if (i <= BBL_TMON_LVD_THRESH_1_9_V) {
			vmon_cfg.low_voltage_detect = true;
			vmon_cfg.low_voltage_thresh = i;
		} else {
			vmon_cfg.low_voltage_detect = false;
		}
		for (j = 0; j < BBL_TMON_HVD_THRESH_3_4_V + 1; j++) {
			if (i <= BBL_TMON_HVD_THRESH_3_4_V) {
				vmon_cfg.high_voltage_detect = true;
				vmon_cfg.high_voltage_thresh = i;
			} else {
				vmon_cfg.high_voltage_detect = false;
			}

			ret = bbl_tamper_detect_configure(dev, &cfg);
			zassert_equal(ret, 0, NULL);
			TC_PRINT(".");
		}
		TC_PRINT("\n");
	}

	/* NULL vmon tamper cfg to disable vmon */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

static void test_bbl_tmon_tamper_test(void)
{
	int ret;
	u32_t timeout;
	struct bbl_tamper_config cfg;
	struct bbl_tmon_tamper_config tmon_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Register a callback first */
	ret = bbl_register_tamper_detect_cb(dev, tamper_detected);
	zassert_equal(ret, 0, NULL);

	/* Set upper limit to a very low value (0 degrees) and verify
	 * tamper event is detected
	 */
	cfg.tamper_type = BBL_TAMPER_DETECT_TEMP_MONITOR;
	cfg.tamper_cfg = &tmon_cfg;

	reset_tamper_event();
	tmon_cfg.upper_limit = 0;
	tmon_cfg.lower_limit = -55;
	tmon_cfg.roc = 10;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	TC_PRINT("Expecting tmon high tamper event\n");
	timeout = 10000;
	do {
		if (tamper_event & BBL_TAMPER_EVENT_TMON_HIGH) {
			TC_PRINT("Tamper event detected : %x\n", tamper_event);
			reset_tamper_event();
			cfg.tamper_cfg = NULL;
			bbl_tamper_detect_configure(dev, &cfg);
			break;
		}
		k_sleep(1);
	} while (--timeout);
	zassert_true(timeout != 0, "Expected Temp too high tamper event\n"
			      "But event not detected!");

	/* Repeat test with lower limit set to a very high value (200 degrees)*/
	cfg.tamper_cfg = &tmon_cfg;
	tmon_cfg.upper_limit = 300;
	tmon_cfg.lower_limit = 200;
	tmon_cfg.roc = 10;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	TC_PRINT("Expecting tmon low tamper event\n");
	timeout = 10000;
	do {
		if (tamper_event & BBL_TAMPER_EVENT_TMON_LOW) {
			TC_PRINT("Tamper event detected : %x\n", tamper_event);
			reset_tamper_event();
			cfg.tamper_cfg = NULL;
			bbl_tamper_detect_configure(dev, &cfg);
			break;
		}
		k_sleep(1);
	} while (--timeout);
	zassert_true(timeout, "Expected Temp too low tamper event\n"
			      "But event not detected!");

	/* NULL tmon tamper cfg to disable tmon */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

#define DBG_CONFIG 0x0000007c
int bbl_write_reg(u32_t addr, u32_t data);

static void test_bbl_int_mesh_tamper_test(void)
{
	int ret;
	u32_t timeout;
	struct bbl_lfsr_params lfsr;
	struct bbl_tamper_config cfg;
	struct bbl_int_mesh_config imesh_cfg;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* The BBL hardware provides a imesh tamper simulate register
	 * to simulate the internal mesh tamper. This test utilizes it
	 * to test the imesh tamper config api.
	 */
	/* Register a callback first */
	ret = bbl_register_tamper_detect_cb(dev, tamper_detected);
	zassert_equal(ret, 0, NULL);

	/* Set LFSR params */
	lfsr.lfsr_update = true;
	lfsr.lfsr_seed = 0x12345678;
	lfsr.lfsr_period = BBL_LFSR_PERIOD_2_MS;
	lfsr.mfm_enable = true;
	ret = bbl_set_lfsr_params(dev, &lfsr);
	zassert_equal(ret, 0, NULL);

	/* Configure imesh tamper detection */
	cfg.tamper_type = BBL_TAMPER_DETECT_INTERNAL_MESH;
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Simulate tamper and verify callback is not called */
	tamper_event = 0;
	bbl_write_reg(DBG_CONFIG, 0x1);
	k_sleep(1000);
	bbl_write_reg(DBG_CONFIG, 0);
	zassert_equal(tamper_event, 0,
		      "Internal mesh tamper detected with tamper disabled!!!");

	/* Repeat the test with imesh tamper enabled */
	cfg.tamper_cfg = &imesh_cfg;
	imesh_cfg.fault_check_enable = true;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Simulate tamper and verify callback is called */
	reset_tamper_event();
	bbl_write_reg(DBG_CONFIG, 0x1);
	timeout = 10000;
	do {
		if (tamper_event & BBL_TAMPER_EVENT_IMESH_FAULT) {
			TC_PRINT("Tamper event detected : %x\n", tamper_event);
			reset_tamper_event();
			break;
		}
		k_sleep(1);
	} while (--timeout);

	/* Remove tamper simulate */
	bbl_write_reg(DBG_CONFIG, 0);

	/* Disable imesh source */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);

	/* Unregister callback */
	bbl_register_tamper_detect_cb(dev, NULL);

	reset_tamper_event();
	zassert_true(timeout, "Failed to detect internal mesh fault!!");
}

void bbl_dump_regs(char *str)
{
	int i;

	TC_PRINT("========\n");
	TC_PRINT("%s\n", str);
	TC_PRINT("========\n");
	for (i = 0; i < 0x9C; i+=4) {
		u32_t val;
		bbl_read_reg(i, &val);
		TC_PRINT("BBL REG[%x] = 0x%08x\n", i, val);
	}
	TC_PRINT("========\n");
}

static void test_bbl_ext_mesh_tamper_static_test(struct device *dev)
{
	int ret, i;
	struct bbl_tamper_config cfg;
	struct bbl_ext_mesh_config emesh_cfg;

	/* Setup static tamper */
	cfg.tamper_type = BBL_TAMPER_DETECT_EXTERNAL_MESH;
	cfg.tamper_cfg = &emesh_cfg;

	emesh_cfg.dyn_mesh_enable = false;
	emesh_cfg.static_pin_mask = 0;
	/* Enable all digital input tamper sources */
	for (i = 0; i < BBL_TAMPER_P_0; i++) {
		emesh_cfg.static_pin_mask |=
			BBL_TAMPER_STATIC_INPUT_MASK_N(i) |
			BBL_TAMPER_STATIC_INPUT_MASK_P(i);
	}

	TC_PRINT("Set SW4.6 off and SW3.6 off\n");
	TC_PRINT("Hit any key ...\n");
	getch();

	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
	TC_PRINT("Now set switches SW3.[1..5] to on to trigger "
		 "a tamper_n[] event\n"
		 "And set switches SW4.[1..5] to on to trigger "
		 "a tamper_p[] event\n"
		 "Verify the correct tamper event is generated from "
		 "the console log\nHit any key to continue ...\n");
	getch();
	TC_PRINT("Set switches SW3/SW4[1..5] back to off position\n");
	TC_PRINT("Hit any key to continue ...\n");
	getch();

	/* Disable external mesh */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

static void test_bbl_ext_mesh_tamper_dynamic_test(struct device *dev)
{
	int ret, i;
	struct bbl_tamper_config cfg;
	struct bbl_ext_mesh_config emesh_cfg;

	struct device *gpio = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	struct device *pinmux =
		device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);

	cfg.tamper_type = BBL_TAMPER_DETECT_EXTERNAL_MESH;
	cfg.tamper_cfg = &emesh_cfg;

	/* Run Emesh tamper test in dynamic mode */
	TC_PRINT("Set SW4.6 on and SW3.6 off\n");
	TC_PRINT("Hit any key to continue ...\n");
	getch();

	emesh_cfg.dyn_mesh_enable = true;
	emesh_cfg.static_pin_mask = 0;
	emesh_cfg.dyn_mesh_mask = 0;
	/* Enable all dynamic mesh tamper sources */
	for (i = 1; i < BBL_TAMPER_P_0; i++)
		emesh_cfg.dyn_mesh_mask |= BBL_TAMPER_DYN_MESH_MASK(i);

	tamper_event = 0;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	TC_PRINT("Verify no external tamper events are logged\n");
	for (i = 0; i < 10; i++) {
		k_sleep(1000);
		TC_PRINT(".");
	}
	TC_PRINT("\nHit any key to continue ...\n");
	getch();

	/* Break tamper_loop and verify external mesh faults are
	 * detected
	 */
	/* Configure MFIO 0 (GPIO 32: PWM0_MFIO0) as output and drive it
	 * to 1
	 */
	ret = gpio_pin_write(gpio, 0, 1); /* GPIO 32 is pin 0 group 1 */
	zassert_true(ret == 0, "Failed to write to GPIO 32");
	ret = gpio_pin_configure(gpio, 0, GPIO_DIR_OUT);
	zassert_true(ret == 0, "Failed to configure GPIO 32");
	ret = pinmux_pin_set(pinmux, 0, 3); /* Set MFIO0 to mode 3 - GPIO */
	zassert_true(ret == 0, "Failed to set GPIO mode for MFIO0");

	TC_PRINT("Set SW3.6 to on position\n");
	TC_PRINT("Verify external tamper event was logged\n");
	TC_PRINT("Hit any key to continue ...\n");
	getch();

	/* Set PWM_MFIO0 to 0 to restore tamper loop connection */
	gpio_pin_write(gpio, 0, 0); /* GPIO 32 is pin 0 group 1 */

	/* Disable external mesh */
	cfg.tamper_cfg = NULL;
	ret = bbl_tamper_detect_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

static void test_bbl_ext_mesh_tamper_test(void)
{
	int ret;
	struct device *dev = device_get_binding(CONFIG_BBL_DEV_NAME);

	zassert_not_null(dev, "Failed to get BBL device binding!");

	/* Disable callback first */
	ret = bbl_register_tamper_detect_cb(dev, NULL);
	zassert_equal(ret, 0, NULL);

	if (manual_test) {
		if (manual_test & 0x1)
			test_bbl_ext_mesh_tamper_dynamic_test(dev);
		if (manual_test & 0x2)
			test_bbl_ext_mesh_tamper_static_test(dev);
	} else {
		TC_ERROR("External mesh tests can only be run manually\n");
		TC_ERROR("Please re-run tests as 'test bbl manual'\n");
	}
}

SHELL_TEST_DECLARE(test_bbl)
{
	u32_t addr, val;

	if (argc > 2) {
		if (strcmp(argv[1], "bw") == 0) {
			addr = strtoul(&argv[2][2], NULL, 16);
			if (argc > 3)
				val = strtoul(&argv[3][2], NULL, 16);
			else
				val = 0;

			TC_PRINT("Writing 0x%x to register 0x%x\n", val, addr);
			bbl_write_reg(addr, val);
			return 0;
		} else if (strcmp(argv[1], "br") == 0) {
			addr = strtoul(&argv[2][2], NULL, 16);

			bbl_read_reg(addr, &val);
			TC_PRINT("Read 0x%x to register 0x%x\n", val, addr);
			return 0;
		}
	}

	if ((argc >= 2) && (strncmp(argv[1], "manual", 6) == 0)) {
		if (argc > 2) {
			manual_test = strtoul(argv[2], NULL, 10);
		} else {
			manual_test = 3;
		}
	} else {
		manual_test = 0;
	}

	if (manual_test) {
#ifndef CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS
		manual_test = 0;
		TC_PRINT("Skipping manual tests\n");
		TC_PRINT("Please enable CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS\n");
#endif
	}

	ztest_test_suite(bbl_bbram_api_tests,
			 ztest_unit_test(test_bbram_api_read),
			 ztest_unit_test(test_bbram_api_write),
			 ztest_unit_test(test_bbram_api_config));

	ztest_test_suite(bbl_api_tests,
			 ztest_unit_test(test_bbl_api_get_tamper_pin_state),
			 ztest_unit_test(test_bbl_api_set_lfsr_params),
			 ztest_unit_test(test_bbl_api_td_configure));

	ztest_test_suite(bbl_bbram_functional_tests,
			 ztest_unit_test(test_bbram_config),
			 ztest_unit_test(test_bbram_read),
			 ztest_unit_test(test_bbram_write),
			 ztest_unit_test(test_bbram_clear),
			 ztest_unit_test(test_bbram_set_access));

	ztest_test_suite(bbl_functional_tests,
			 ztest_unit_test(test_bbl_get_tamper_pin_state),
			 ztest_unit_test(test_bbl_set_lfsr_params),
			 ztest_unit_test(test_bbl_fmon_tamper_test),
			 ztest_unit_test(test_bbl_tmon_tamper_test),
			 ztest_unit_test(test_bbl_vmon_tamper_test),
			 ztest_unit_test(test_bbl_int_mesh_tamper_test),
			 ztest_unit_test(test_bbl_ext_mesh_tamper_test));

	ztest_run_test_suite(bbl_bbram_api_tests);
	ztest_run_test_suite(bbl_bbram_functional_tests);

	ztest_run_test_suite(bbl_api_tests);
	ztest_run_test_suite(bbl_functional_tests);

	return 0;
}
