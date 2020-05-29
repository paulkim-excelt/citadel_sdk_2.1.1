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

/* @file test_qspi_bcm58202.c
 *
 * Unit test for BCM58202 QSPI flash driver
 *
 * This file implements the unit tests for the bcm58202 qspi flash driver
 */

#include <test.h>
#include <stdlib.h>
#include <qspi_flash.h>
#include <cortex_a/cache.h>
#include <board.h>
#include <broadcom/dma.h>
#include <smau.h>

#define QSPI_DRIVER_TEST_ADDRESS	get_test_addr()
#define QSPI_DRIVER_TEST_SECTOR_SIZE	0x10000		/* 64 K sector size */

#ifdef CONFIG_CITADEL_EMULATION
#define STRESS_TEST_ITERATIONS	2
#else
#define STRESS_TEST_ITERATIONS	20
#endif

#define TEST_PATTERN	0x36
#define MAX_READ_FREQ	(50000000)
#define MAX_WITE_FREQ	(25000000)

#define CHAR_ARRY_TO_U32(A) ((A[3] << 24) | (A[2] << 16) | (A[1] << 8) | A[0])

#define BUFF_SIZE	1024UL
static u8_t buff[BUFF_SIZE];

/* Destination buffer should be aligned to cache line size
 * And its size must be a multiple of cache line size
 */
static u8_t __attribute__((__aligned__(64)))read_buff[BUFF_SIZE];

/* Configurable test address*/
static u32_t qspi_test_addr = MB(4);	/* 4 MB offset */

static void set_test_addr(u32_t addr)
{
	qspi_test_addr = addr;
}

static u32_t get_test_addr(void)
{
	return qspi_test_addr;
}

/* slave select for the flash being tested */
static u8_t test_flash_id;

static struct device *dev;

static u32_t get_sys_time(void)
{
	/* FIXME: This function should return the time elapsed,
	 * even if interrupts are locked, else erase times cannot
	 * be measured.
	 */
	return k_uptime_get_32();
}

static void test_qspi_api_configure(void)
{
	int ret;

	/* Invalid config pointer */
	ret = qspi_flash_configure(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

static void test_qspi_api_getinfo(void)
{
	int ret;

	/* Invalid info pointer */
	ret = qspi_flash_get_device_info(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

static void test_qspi_api_read(void)
{
	int ret;
	struct qspi_flash_info info;

	/* Invalid address - 2 Gig offset */
	ret = qspi_flash_read(dev, 0x80000000, 1, read_buff);
	zassert_equal(ret, -EINVAL, NULL);

	/* Zero size */
	ret = qspi_flash_read(dev, 0x1000, 0, read_buff);
	zassert_equal(ret, -EINVAL, NULL);

	/* Invalid data pointer */
	ret = qspi_flash_read(dev, 0x1000, 5, NULL);
	zassert_equal(ret, -EINVAL, NULL);

	/* Too large size */
	ret = qspi_flash_read(dev, 0x1000, 0x2000000, read_buff);
	zassert_equal(ret, -EINVAL, NULL);

	/* Read from last location in flash */
	ret = qspi_flash_get_device_info(dev, &info);
	zassert_equal(ret, 0, NULL);

	ret = qspi_flash_read(dev, info.flash_size - 1, 0x1, read_buff);
	zassert_equal(ret, 0, NULL);
}

static void test_qspi_api_write(void)
{
	int ret;
	u8_t data;
	struct qspi_flash_info info;

	/* Invalid address - 2 Gig offset */
	ret = qspi_flash_write(dev, 0x80000000, 1, &data);
	zassert_equal(ret, -EINVAL, NULL);

	/* Zero size */
	ret = qspi_flash_write(dev, 0x1000, 0, &data);
	zassert_equal(ret, -EINVAL, NULL);

	/* Invalid data pointer */
	ret = qspi_flash_write(dev, 0x1000, 5, NULL);
	zassert_equal(ret, -EINVAL, NULL);

	/* Too large size */
	ret = qspi_flash_write(dev, 0x1000, 0x2000000, &data);
	zassert_equal(ret, -EINVAL, NULL);

	/* Write to last location in flash */
	ret = qspi_flash_get_device_info(dev, &info);
	zassert_equal(ret, 0, NULL);

	ret = qspi_flash_write(dev, info.flash_size - 1, 0x1, &data);
	zassert_equal(ret, 0, NULL);
}

static void test_qspi_api_erase(void)
{
	int ret;
	struct qspi_flash_info info;

	/* Invalid address - 2 Gig offset */
	ret = qspi_flash_erase(dev, 0x80000000, 1);
	zassert_equal(ret, -EINVAL, NULL);

	/* Zero size */
	ret = qspi_flash_erase(dev, 0x1000, 0x0);
	zassert_equal(ret, -EINVAL, NULL);

	/* Too large size */
	ret = qspi_flash_erase(dev, 0x1000, 0x2000000);
	zassert_equal(ret, -EINVAL, NULL);

	/* Unaligned size */
	TC_PRINT("This test generates an error message from QSPI driver about\n"
		 "the erase size. This is expected for the test to pass\n");
	ret = qspi_flash_erase(dev, 0x1000, 0x400);
	zassert_equal(ret, -EINVAL, NULL);

	/* Erase last sector in flash */
	ret = qspi_flash_get_device_info(dev, &info);
	zassert_equal(ret, 0, NULL);

	ret = qspi_flash_erase(dev,
			info.flash_size - QSPI_DRIVER_TEST_SECTOR_SIZE,
			QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, NULL);
}

static void test_qspi_read(struct qspi_flash_config *cfg)
{
	int ret;
	u32_t test_addr, i;

	zassert_not_null(dev, "Driver binding not found!");

	for (i = 0; i < BUFF_SIZE; i++)
		buff[i] = i + QSPI_DATA_LANE(cfg->op_mode);

	/* Erase 2 sectors */
	ret = qspi_flash_erase(dev, QSPI_DRIVER_TEST_ADDRESS,
			       2*QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	/* Read and verify 128 bytes across a sector boundary
	 * and ensure the contents are 0xFF
	 */
	test_addr = QSPI_DRIVER_TEST_ADDRESS + QSPI_DRIVER_TEST_SECTOR_SIZE
		    - (BUFF_SIZE / 2);
	ret = qspi_flash_read(dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_true(read_buff[i] == 0xFF, NULL);

	/* Write the pattern and read it back */
	ret = qspi_flash_write(dev, test_addr, BUFF_SIZE, &buff[0]);
	zassert_equal(ret, 0, "Flash write returned error");

	/* Apply configuration only before the read, which is being tested */
	ret = qspi_flash_configure(dev, cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	ret = qspi_flash_read(dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_equal(buff[i], read_buff[i], NULL);

	/* Restore to single lane mode */
	if (QSPI_DATA_LANE(cfg->op_mode) != QSPI_MODE_SINGLE) {
		SET_QSPI_DATA_LANE(cfg->op_mode, QSPI_MODE_SINGLE);
		ret = qspi_flash_configure(dev, cfg);
		zassert_equal(ret, 0, "Flash configure returned error");
	}
}

static void test_qspi_config(void)
{
	int ret;
	struct qspi_flash_config cfg;
	u32_t read_times[3], time, i;

	zassert_not_null(dev, "Driver binding not found!");

	/* Try setting different frequencies measure read times */
	cfg.op_mode = 0;
	cfg.frequency = 12500000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);

	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	cfg.frequency = 2000000;
	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Log time taken */
	time = get_sys_time();
	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		ret = qspi_flash_write(dev, QSPI_DRIVER_TEST_ADDRESS,
				       BUFF_SIZE, &buff[0]);
		zassert_equal(ret, 0, "Flash configure returned error");
	}
	read_times[0] = get_sys_time() - time;

	cfg.frequency = 5000000;
	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Log time taken */
	time = get_sys_time();
	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		ret = qspi_flash_write(dev, QSPI_DRIVER_TEST_ADDRESS,
				       BUFF_SIZE, &buff[0]);
		zassert_equal(ret, 0, "Flash configure returned error");
	}
	read_times[1] = get_sys_time() - time;

	cfg.frequency = 10000000;
	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Log time taken */
	time = get_sys_time();
	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		ret = qspi_flash_write(dev, QSPI_DRIVER_TEST_ADDRESS,
				       BUFF_SIZE, &buff[0]);
		zassert_equal(ret, 0, "Flash configure returned error");
	}
	read_times[2] = get_sys_time() - time;

	/* Check that read_times[0] >= read_times[1] >= read_times[2] */
	zassert_true(read_times[0] >= read_times[1],
			"Error: Read at 2 MHz is faster than read at 5 MHz!");
	zassert_true(read_times[1] >= read_times[2],
			"Error: Read at 5 MHz is faster than read at 10 MHz!");
}


static void test_qspi_getinfo(void)
{
	int ret;
	struct qspi_flash_info info;

	zassert_not_null(dev, "Driver binding not found!");

	/* Get info and check erase size and flash size are non-zero */
	ret = qspi_flash_get_device_info(dev, &info);
	zassert_equal(ret, 0, "Flash configure returned error");

	zassert_not_equal(info.flash_size, 0, "Flash size zero!!!");
	zassert_not_equal(info.min_erase_sector_size, 0, "Erase size zero!!!");
}

static void test_qspi_single_lane_read(void)
{
	struct qspi_flash_config cfg;

	/* Configure single lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 12500000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);

	test_qspi_read(&cfg);
}

static void test_qspi_dual_lane_read(void)
{
	struct qspi_flash_config cfg;

	/* Configure dual lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 25000000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_DUAL);

	test_qspi_read(&cfg);
}

static void test_qspi_quad_lane_read(void)
{
	struct qspi_flash_config cfg;

	/* Configure quad lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 25000000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_QUAD);

	test_qspi_read(&cfg);
}

static void test_qspi_sector_erase(void)
{
	int ret;
	u32_t test_addr, i, j, erase_size;

	struct qspi_flash_info info;

	zassert_not_null(dev, "Driver binding not found!");

	ret = qspi_flash_get_device_info(dev, &info);
	zassert_equal(ret, 0, "Flash get device info returned error");

	/* Align erase address to sector size */
	test_addr = QSPI_DRIVER_TEST_ADDRESS
		    & ~(info.min_erase_sector_size - 1);

	erase_size = 2*info.min_erase_sector_size;

	/* Write 0s to the sectors first*/
	for (i = 0; i < BUFF_SIZE; i++)
		buff[i] = 0;
	for (i = 0; i < (erase_size / BUFF_SIZE); i++) {
		ret = qspi_flash_write(dev, test_addr + i*BUFF_SIZE,
				       BUFF_SIZE, &buff[0]);
		zassert_equal(ret, 0, "Flash write returned error");

		/* Read and verify 0s */
		ret = qspi_flash_read(dev, test_addr + i*BUFF_SIZE,
				      BUFF_SIZE, &read_buff[0]);
		zassert_equal(ret, 0, "Flash read returned error");

		for (j = 0; j < BUFF_SIZE; j++)
			zassert_true(read_buff[j] == 0, NULL);
	}

	/* Erase 2 sectors and verify */
	ret = qspi_flash_erase(dev, test_addr, erase_size);
	zassert_equal(ret, 0, "Flash erase returned error");

	for (i = 0; i < (erase_size / BUFF_SIZE); i += 8) {
		/* Read and verify 0xFFs */
		ret = qspi_flash_read(dev, test_addr + i*BUFF_SIZE,
				      BUFF_SIZE, &read_buff[0]);
		zassert_equal(ret, 0, "Flash read returned error");

		for (j = 0; j < BUFF_SIZE; j++)
			zassert_true(read_buff[j] == 0xFF, NULL);
	}
}

static void test_qspi_write(struct qspi_flash_config *cfg)
{
	int ret;
	u32_t test_addr, i;

	zassert_not_null(dev, "Driver binding not found!");

	for (i = 0; i < BUFF_SIZE; i++)
		buff[i] = 0x80 + i + QSPI_DATA_LANE(cfg->op_mode);

	/* Erase 2 sectors */
	ret = qspi_flash_erase(dev, QSPI_DRIVER_TEST_ADDRESS,
			       2*QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	/* Read and verify 128 bytes across a sector boundary
	 * and ensure the contents are 0xFF
	 */
	test_addr = QSPI_DRIVER_TEST_ADDRESS + QSPI_DRIVER_TEST_SECTOR_SIZE
		    - (BUFF_SIZE / 2);
	ret = qspi_flash_read(dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_true(read_buff[i] == 0xFF, NULL);

	/* Apply configuration only before the write, which is being tested */
	ret = qspi_flash_configure(dev, cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Write the pattern and read it back */
	ret = qspi_flash_write(dev, test_addr, BUFF_SIZE, &buff[0]);
	zassert_equal(ret, 0, "Flash write returned error");

	/* Restore to single lane mode */
	if (QSPI_DATA_LANE(cfg->op_mode) != QSPI_MODE_SINGLE) {
		SET_QSPI_DATA_LANE(cfg->op_mode, QSPI_MODE_SINGLE);
		ret = qspi_flash_configure(dev, cfg);
		zassert_equal(ret, 0, "Flash configure returned error");
	}

	/* Read and verify */
	ret = qspi_flash_read(dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_equal(buff[i], read_buff[i], NULL);
}

static void test_qspi_single_lane_write(void)
{
	struct qspi_flash_config cfg;

	/* Configure single lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 12500000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);

	test_qspi_write(&cfg);
}

static void test_qspi_dual_lane_write(void)
{
	struct qspi_flash_config cfg;

	/* Configure dual lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 20000000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_DUAL);

	test_qspi_write(&cfg);
}

static void test_qspi_quad_lane_write(void)
{
	struct qspi_flash_config cfg;

	/* Configure quad lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 15000000;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_QUAD);

	test_qspi_write(&cfg);
}

static void test_qspi_chip_erase(void)
{
	int ret;
	u32_t i, j;
	struct qspi_flash_info info;

	zassert_not_null(dev, "Driver binding not found!");

	if (test_flash_id == 0) {
		TC_PRINT("Skipping chip erase test for Flash : 0\n");
	} else {
		ret = qspi_flash_get_device_info(dev, &info);
		zassert_equal(ret, 0, "Flash configure returned error");

		ret = qspi_flash_chip_erase(dev);
		zassert_equal(ret, 0, "Flash chip erase returned error");
		TC_PRINT("Chip erase complete ... Verification in progress\n");

		for (i = 0; i < (info.flash_size / BUFF_SIZE); i += 8192) {
			/* Read and verify 0xFFs */
			ret = qspi_flash_read(dev, i*BUFF_SIZE,
					      BUFF_SIZE, &read_buff[0]);
			zassert_equal(ret, 0, "Flash read returned error");

			for (j = 0; j < BUFF_SIZE; j++)
				zassert_true(read_buff[j] == 0xFF, NULL);
		}
	}
}

static void test_stress_qspi_read(void)
{
	int ret;
	u32_t i, data[2], temp[2];

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Read 2 locations 1000 times with cache disabled */
	disable_dcache();
#endif
	zassert_not_null(dev, "Driver binding not found!");

	ret = qspi_flash_read(dev, 0x1000, 4, read_buff);
	zassert_equal(ret, 0, "Flash read returned error");
	data[0] = CHAR_ARRY_TO_U32(read_buff);

	ret = qspi_flash_read(dev, 0x2000, 4, read_buff);
	zassert_equal(ret, 0, "Flash read returned error");
	data[1] = CHAR_ARRY_TO_U32(read_buff);

	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		/* Set temp to not(data) so that the read can be verified */
		temp[0] = ~data[0];
		temp[1] = ~data[1];

		ret = qspi_flash_read(dev, 0x1000, 4, read_buff);
		zassert_equal(ret, 0, "Flash read returned error");
		temp[0] = CHAR_ARRY_TO_U32(read_buff);
		zassert_equal(temp[0], data[0], "Data read is inconsistent");

		ret = qspi_flash_read(dev, 0x2000, 4, read_buff);
		zassert_equal(ret, 0, "Flash read returned error");
		temp[1] = CHAR_ARRY_TO_U32(read_buff);
		zassert_equal(temp[1], data[1], "Data read is inconsistent");
	}

#ifdef CONFIG_DATA_CACHE_SUPPORT
	enable_dcache();
#endif
}

static void test_stress_qspi_write(void)
{
	int ret;
	u8_t byte;
	u32_t i, data, temp, test_addr;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Write repeatedly to a location 1000 times with cache disabled */
	disable_dcache();
#endif

	zassert_not_null(dev, "Driver binding not found!");

	test_addr = QSPI_DRIVER_TEST_ADDRESS;
	ret = qspi_flash_erase(dev, test_addr, QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	data = TEST_PATTERN;
	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		ret = qspi_flash_write(dev, test_addr, 4, (u8_t *)&data);
		zassert_equal(ret, 0, "Flash write returned error");
	}

	/* Read back and check */
	ret = qspi_flash_read(dev, test_addr, 4, read_buff);
	zassert_equal(ret, 0, "Flash read returned error");
	temp = CHAR_ARRY_TO_U32(read_buff);
	zassert_equal(temp, data, "Data read is inconsistent");

	/* Write patterns with only 1 -> 0 transistions and verify written value
	 */
	byte = 0xFF;
	test_addr = QSPI_DRIVER_TEST_ADDRESS + 0x100;
	while (byte) {
		ret = qspi_flash_write(dev, test_addr, 1, &byte);
		zassert_equal(ret, 0, "Flash write returned error");

		ret = qspi_flash_read(dev, test_addr, 1, read_buff);
		zassert_equal(ret, 0, "Flash read returned error");
		zassert_equal(byte, read_buff[0], "Data write is inconsistent");

		byte >>= 1;
	}

	byte = 0xFF;
	test_addr = QSPI_DRIVER_TEST_ADDRESS + 0x200;
	while (byte) {
		ret = qspi_flash_write(dev, test_addr, 1, &byte);
		zassert_equal(ret, 0, "Flash write returned error");

		ret = qspi_flash_read(dev, test_addr, 1, read_buff);
		zassert_equal(ret, 0, "Flash read returned error");
		zassert_equal(byte, read_buff[0], "Data write is inconsistent");

		byte <<= 1;
	}

#ifdef CONFIG_DATA_CACHE_SUPPORT
	enable_dcache();
#endif
}

static void test_stress_qspi_erase(void)
{
	int ret;
	u32_t i, test_addr, data;

	zassert_not_null(dev, "Driver binding not found!");

	test_addr = QSPI_DRIVER_TEST_ADDRESS + QSPI_DRIVER_TEST_SECTOR_SIZE;

	/* Erase a sector 1000 times and then write a 0 and erase and
	 * confirm erase still works
	 */
	for (i = 0; i < STRESS_TEST_ITERATIONS; i++) {
		ret = qspi_flash_erase(dev, test_addr,
					QSPI_DRIVER_TEST_SECTOR_SIZE);
		zassert_equal(ret, 0, "Flash erase returned error");
	}

	data = 0;
	ret = qspi_flash_write(dev, test_addr, 4, (u8_t *)&data);
	zassert_equal(ret, 0, "Flash write returned error");

	ret = qspi_flash_erase(dev, test_addr, QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	ret = qspi_flash_read(dev, test_addr, 4, read_buff);
	zassert_equal(ret, 0, "Flash read returned error");
	data = CHAR_ARRY_TO_U32(read_buff);
	zassert_equal(data, -1UL, "Flash read returned error");
}

static u32_t measure_qspi_read_time(u32_t test_addr, u32_t size)
{
	int ret;
	u8_t *ptr;
	u32_t time;

	ptr = cache_line_aligned_alloc(size);

	zassert_not_null(ptr, "Malloc failed");
	time = get_sys_time();
	ret = qspi_flash_read(dev, test_addr, size, ptr);
	time = get_sys_time() - time;
	zassert_equal(ret, 0, "Flash read returned error");
	cache_line_aligned_free(ptr);

	return time;
}

static u32_t measure_qspi_write_time(u32_t test_addr, u32_t size)
{
	int ret;
	u8_t *ptr;
	u32_t time, i;

	ptr = (u8_t *)k_malloc(size);
	zassert_not_null(ptr, "Malloc failed");

	/* Initialize write data to 0 */
	for (i = 0; i < size; i++)
		ptr[i] = 0;

	/* Erase sector to cause all 1 -> 0 transitions on write */
	ret = qspi_flash_erase(dev, test_addr, QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	time = get_sys_time();
	ret = qspi_flash_write(dev, test_addr, size, ptr);
	time = get_sys_time() - time;
	zassert_equal(ret, 0, "Flash read returned error");
	k_free(ptr);

	return time;
}

static void test_profile_qspi_read_lane(u8_t mode)
{
	int ret;
	struct qspi_flash_config cfg;
	u32_t time, test_addr;

	zassert_not_null(dev, "Driver binding not found!");

	/* Read 1K, 10K and 50K in all modes */
	cfg.op_mode = 0;
	cfg.frequency = MAX_READ_FREQ;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, mode);

	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	test_addr = QSPI_DRIVER_TEST_ADDRESS;

	time = measure_qspi_read_time(test_addr, KB(1));
	TC_PRINT("Read time for 1 KB   : %d (ms)\n", time);

	time = measure_qspi_read_time(test_addr, KB(10));
	TC_PRINT("Read time for 10 KB  : %d (ms)\n", time);

	time = measure_qspi_read_time(test_addr, KB(50));
	TC_PRINT("Read time for 50 KB  : %d (ms)\n", time);

	/* Restore single lane mode */
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);
	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");
}

static void test_profile_qspi_read(void)
{
	TC_PRINT("Single lane read times\n----------------------\n");
	test_profile_qspi_read_lane(QSPI_MODE_SINGLE);
	TC_PRINT("Dual lane read times\n----------------------\n");
	test_profile_qspi_read_lane(QSPI_MODE_DUAL);
	TC_PRINT("Quad lane read times\n----------------------\n");
	test_profile_qspi_read_lane(QSPI_MODE_QUAD);
}

static void test_profile_qspi_write_lane(u8_t mode)
{
	int ret;
	struct qspi_flash_config cfg;
	u32_t time, test_addr;

	zassert_not_null(dev, "Driver binding not found!");

	/* Write 1K, 10K and 50K in all modes */
	cfg.op_mode = 0;
	cfg.frequency = MAX_WITE_FREQ;
	cfg.slave_num = test_flash_id;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, mode);

	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	test_addr = QSPI_DRIVER_TEST_ADDRESS;

	time = measure_qspi_write_time(test_addr, KB(1));
	TC_PRINT("Write time for 1 KB   : %d (ms)\n", time);

	time = measure_qspi_write_time(test_addr, KB(10));
	TC_PRINT("Write time for 10 KB  : %d (ms)\n", time);

	time = measure_qspi_write_time(test_addr, KB(50));
	TC_PRINT("Write time for 50 KB  : %d (ms)\n", time);

	/* Restore single lane mode */
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);
	ret = qspi_flash_configure(dev, &cfg);
	zassert_equal(ret, 0, "Flash configure returned error");
}

static void test_profile_qspi_write(void)
{
	TC_PRINT("Single lane write times\n-----------------------\n");
	test_profile_qspi_write_lane(QSPI_MODE_SINGLE);
	TC_PRINT("Dual lane write times\n-----------------------\n");
	test_profile_qspi_write_lane(QSPI_MODE_DUAL);
	TC_PRINT("Quad lane write times\n-----------------------\n");
	test_profile_qspi_write_lane(QSPI_MODE_QUAD);
}

static void test_profile_qspi_sector_erase(void)
{
	int ret;
	u32_t time, i;

	/* Measure erase time for 64K, 32K, 16K, 8K and 4K sizes */
	u32_t sizes[] = {KB(64), KB(32), KB(16), KB(8), KB(4)};

	TC_PRINT("Sector erase times\n------------------\n");
	for (i = 0; i < (sizeof(sizes)/sizeof(u32_t)); i++) {
		time = get_sys_time();
		ret = qspi_flash_erase(dev, QSPI_DRIVER_TEST_ADDRESS, sizes[i]);
		time = get_sys_time() - time;
		zassert_equal(ret, 0, "Flash erase returned error");
		TC_PRINT("Sector size (%d KB) : %d (ms)\n",
			  sizes[i] / 1024, time);
	}
}

static void test_profile_qspi_chip_erase(void)
{
	int ret;
	u32_t time;

	zassert_not_null(dev, "Driver binding not found!");

	if (test_flash_id == 0) {
		TC_PRINT("Skipping chip erase test for Flash : 0\n");
	} else {
		time = get_sys_time();
		ret = qspi_flash_chip_erase(dev);
		time = get_sys_time() - time;
		zassert_equal(ret, 0, "Flash chip erase returned error");
		TC_PRINT("Chip Erase time      : %d (ms)\n", time);
	}
}

/* Default SMAU window settings */
#define WIN_BASE_1	0x64000000
#define WIN_BASE_2	0x65000000
#define WIN_BASE_3	0x66000000
#define WIN_SIZE_DEF	0xFF000000
#define HMAC_BASE_1	0x64042000
#define HMAC_BASE_2	0x65044000
#define HMAC_BASE_3	0x66046000

static void init_smau_windows(void)
{
	static bool init_done = false;

	if (init_done == false) {
		u32_t key = irq_lock();
		struct device *dev = device_get_binding(CONFIG_SMAU_NAME);

		smau_config_windows(dev, 1, WIN_BASE_1, WIN_SIZE_DEF,
				    HMAC_BASE_1, 1, 1, 1, 0, 0);
		smau_config_windows(dev, 2, WIN_BASE_2, WIN_SIZE_DEF,
				    HMAC_BASE_2, 1, 1, 1, 0, 0);
		smau_config_windows(dev, 3, WIN_BASE_3, WIN_SIZE_DEF,
				    HMAC_BASE_3, 1, 1, 1, 0, 0);
		init_done = true;
		irq_unlock(key);
	}
}

SHELL_TEST_DECLARE(test_qspi_flash_main)
{
	/* Default to 2 MB offset for testing */
	set_test_addr(MB(2));

	test_flash_id = 0;

	/* Usage:
	 *	test qspi_driver @800000	# Test flash 0 at offset 8 MB
	 *	test qspi_driver flash1@1300000	# Test flash 1 at offset 19 MB
	 */
	if (argc == 2) {
		if (strncmp(argv[1], "flash1", 6) == 0) {
#ifdef CONFIG_SECONDARY_FLASH
			struct qspi_flash_info info;

			/* Check if secondary flash exists and run the test
			 * on it
			 */
			dev =
			device_get_binding(CONFIG_SECONDARY_FLASH_DEV_NAME);
			if ((qspi_flash_get_device_info(dev, &info) == 0)
				&& (info.flash_size != 0)) {
				TC_PRINT("Detected secondary flash\n");
				TC_PRINT("Tests will run on the flash @CS1\n");
				test_flash_id = 1;
			} else {
				TC_PRINT("Failed to detect secondary flash\n");
			}
#else
			TC_PRINT("Set CONFIG_SECONDARY_FLASH=1 "
				 "to run tests on CS1\n");
#endif
			if (argv[1][6] == '@')
				set_test_addr(strtoul(&argv[1][7], NULL, 16));
		} else if (argv[1][0] == '@') {
			set_test_addr(strtoul(&argv[1][1], NULL, 16));
		}
	}

	if (test_flash_id == 0) {
		TC_PRINT("Tests will be run on the flash @CS0\n");
		dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);
	}

	/* All 4 SMAU windows need to be initialized for accessing flash @CS1
	 * and offset > 16M for flash @CS0. So this initialization will be done
	 * as part of unit test. In an actual application, the app will take
	 * care of initializing the windows as per the app's requirement
	 */
	init_smau_windows();

	ztest_test_suite(qspi_driver_api_tests,
			 ztest_unit_test(test_qspi_api_configure),
			 ztest_unit_test(test_qspi_api_getinfo),
			 ztest_unit_test(test_qspi_api_read),
			 ztest_unit_test(test_qspi_api_write),
			 ztest_unit_test(test_qspi_api_erase));

	ztest_test_suite(qspi_driver_functional_tests,
			 ztest_unit_test(test_qspi_config),
			 ztest_unit_test(test_qspi_getinfo),
			 ztest_unit_test(test_qspi_single_lane_read),
			 ztest_unit_test(test_qspi_dual_lane_read),
			 ztest_unit_test(test_qspi_quad_lane_read),
			 ztest_unit_test(test_qspi_sector_erase),
			 ztest_unit_test(test_qspi_single_lane_write),
			 ztest_unit_test(test_qspi_dual_lane_write),
			 ztest_unit_test(test_qspi_quad_lane_write),
			 ztest_unit_test(test_qspi_chip_erase));

	ztest_test_suite(qspi_driver_stress_tests,
			 ztest_unit_test(test_stress_qspi_read),
			 ztest_unit_test(test_stress_qspi_write),
			 ztest_unit_test(test_stress_qspi_erase));

	ztest_test_suite(qspi_driver_performance_tests,
			 ztest_unit_test(test_profile_qspi_read),
			 ztest_unit_test(test_profile_qspi_write),
			 ztest_unit_test(test_profile_qspi_sector_erase),
			 ztest_unit_test(test_profile_qspi_chip_erase));

	ztest_run_test_suite(qspi_driver_api_tests);
	ztest_run_test_suite(qspi_driver_functional_tests);
	ztest_run_test_suite(qspi_driver_stress_tests);
	ztest_run_test_suite(qspi_driver_performance_tests);

	return 0;
}
