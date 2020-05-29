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

/* @file test_sdio.c
 *
 * Unit test for SDIO driver
 *
 * This file implements the unit tests for the bcm5820x SDIO driver
 */

#include <test.h>
#include <errno.h>
#include <board.h>
#include <string.h>
#include <sdio.h>
#include <ctype.h>

#include <broadcom/gpio.h>
#include <broadcom/dma.h>
#include <pinmux.h>
#include <broadcom/i2c.h>
#include "sd_mem_card_test.h"

#ifdef CONFIG_FILE_SYSTEM
#include <fs.h>
#endif

#define IO_EXPANDER_ADDR	0x20
#define U100_EXPANDER_ADDR	0x70

#define IO_EXPANDER_REG		0x4
#define IO_EXPANDER_REGA	0x0
#define IO_EXPANDER_REGB	0x1
#define IO_EXPANDER_REGC	0x2
#define IO_EXPANDER_REGD	0x3
#define IO_EXPANDER_REGE	0x4

static union dev_config i2c_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

static int test_i2c_read_io_expander(struct device *dev,
		u8_t reg_addr, u8_t *val, u8_t i2_addr)
{
	struct i2c_msg msgs[2];
	u8_t raddr = reg_addr;

	msgs[0].buf = &raddr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = val;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, i2_addr);
}

static int test_i2c_write_io_expander(struct device *dev,
		u8_t reg_addr, u8_t val, u8_t i2_addr)
{
	struct i2c_msg msgs;
	u8_t buff[2];
	u8_t value = val;

	buff[0] = reg_addr;
	buff[1] = value;
	msgs.buf = buff;
	msgs.len = 2;
	msgs.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msgs, 1, i2_addr);
}

int configure_sdio_sel(void)
{
	int ret;
	u8_t val = 0;
	struct device *dev;

	/* Set IO expander straps first */
	/* SPI1_SMC_[1:0] = 0x2 and SPI2_SMC[1:0] = 0x2 */
	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");

	ret = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(ret == 0, "i2c_configure() returned error");

	/* Read port 1 */
	test_i2c_read_io_expander(dev, IO_EXPANDER_REGB, &val, IO_EXPANDER_ADDR);
	/* Write to port 1 */
	test_i2c_write_io_expander(dev, 0x8 + IO_EXPANDER_REGB,
				  (val & ~0x33) | 0x22, IO_EXPANDER_ADDR);
	/* Read port 1 direction reg */
	test_i2c_read_io_expander(dev, 0x18 + IO_EXPANDER_REGB, &val,
				 IO_EXPANDER_ADDR);
	/* Configure port 1 as output */
	test_i2c_write_io_expander(dev, 0x18 + IO_EXPANDER_REGB, val & ~0x33,
				  IO_EXPANDER_ADDR);

	/* Drive 0 on SDIO_CD_L by driving GPIO_102 low (NFC_HOST_WAKE_CTRL) */
	/* FIXME: Check if this is really needed
	 * #define NFC_HOST_WAKE_CTRL 0x4409800C
	 * sys_write32(0x0, NFC_HOST_WAKE_CTRL);
	 */

	/* Bit 5 of I2C IO expander PCS9538 should be driven high */
	test_i2c_read_io_expander(dev, 0x0, &val, U100_EXPANDER_ADDR);
	/* Ensure Bit 5 is set to drive SDIO_EN */
	test_i2c_write_io_expander(dev, 0x1, val | BIT(5), U100_EXPANDER_ADDR);
	/* Configure IO5 as output */
	test_i2c_read_io_expander(dev, 0x3, &val, U100_EXPANDER_ADDR);
	test_i2c_write_io_expander(dev, 0x3, val & ~BIT(5), U100_EXPANDER_ADDR);

	return 0;
}

#ifdef CONFIG_FILE_SYSTEM
static void lsdir(const char *path)
{
	int ret;
	FS_DIR_T dirp;
	struct fs_dirent entry;

	/* Verify fs_opendir() */
	ret = fs_opendir(&dirp, path);
	zassert_equal(ret, 0, "Error opening dir");

	TC_PRINT("=================\n");
	TC_PRINT("Listing dir %s:\n", path);
	TC_PRINT("=================\n");
	for (;;) {
		/* Verify fs_readdir() */
		ret = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (ret || entry.name[0] == 0)
			break;

		if (entry.type == FS_DIR_ENTRY_DIR) {
			TC_PRINT("[DIR ] %s\n", entry.name);
		} else {
			TC_PRINT("[FILE] %s (size = %u)\n",
			entry.name, entry.size);
		}
	}
	TC_PRINT("=================\n\n");

	/* Verify fs_closedir() */
	fs_closedir(&dirp);
}

static void write_file(char *file, const char *contents)
{
	int ret;
	FS_FILE_T fp;

	/* Remove existing file */
	fs_unlink(file);

	ret = fs_open(&fp, file);
	zassert_equal(ret, 0, "failed to open file for writing");

	ret = fs_write(&fp, contents, strlen(contents));
	zassert_equal(ret, strlen(contents), "failed to write to file");

	TC_PRINT("Wrote %s file to SD card\n", file);
	TC_PRINT("Check contents by connecting it to an SD card reader\n");

	fs_close(&fp);
}
#endif /* CONFIG_FILE_SYSTEM */

/* This test detects an SD memory card and if found, initializes it with a
 * FAT file system and adds a file hello.txt to the root with contents
 * "Hello World".
 */
void sdio_init_fs_test(void)
{
#ifdef CONFIG_FILE_SYSTEM
	const char *contents =  "This file is written to the SD card "
				"by running the sdio test!\n"
				"Date: " __DATE__ " Time: " __TIME__ "\n";

	/* If file system support is available, get the file system contents */
	lsdir(ROOT_PATH);

	/* Write citadel test log file */
	write_file(ROOT_PATH "citadel.log", contents);

	return;
#else
	int ret, i, j;
	u8_t *buf;

	buf = cache_line_aligned_alloc(512);
	zassert_not_null(buf, NULL);

	ret = sd_mem_card_test_init();
	zassert_equal(ret, 0, NULL);

	ret = sd_mem_card_read_blocks(0, 1, buf);
	zassert_equal(ret, 1, NULL);
	for (i = 0; i < 512; i+=16) {
		for (j = 0; j < 16; j++)
			TC_PRINT("%02x ", buf[i + j]);
		for (j = 0; j < 16; j++)
			TC_PRINT("%c", isprint(buf[i + j]) ? buf[i + j] : ' ');
		TC_PRINT("\n");
	}

	cache_line_aligned_free(buf);
#endif
}

#ifdef CONFIG_FILE_SYSTEM
#include <crypto/crypto_selftest.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_rng.h>
#include <clock_a7.h>

static u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static fips_rng_context rngctx;

static void crypto_rng_selftest_s(void)
{
	BCM_SCAPI_STATUS ret;

	crypto_init((crypto_lib_handle *)&cryptoHandle);

	ret = crypto_rng_selftest((crypto_lib_handle *)&cryptoHandle, &rngctx);
	zassert_equal(ret, 0, "crypto_rng_selftest failed");
}

/* Use a 512 byte buffer to match the SD card block length (optimal access) */
static u8_t data_buf[512];
int dump_rand_data_to_file(const char *file, size_t size)
{
	FS_FILE_T fp;
	int ret, count = 0, i;

	/* Set CPU frequency to max */
	clk_a7_set(MHZ(1000));

	/* Remove existing file */
	TC_PRINT("Note: Existing file will be removed and overwritten\n");
	fs_unlink(file);

	ret = fs_open(&fp, file);
	if (ret) {
		TC_ERROR("Failed to open file %s for writing: %d\n", file, ret);
		TC_ERROR("Check the file name is in 8.3 format!\n");
		return ret;
	}

	crypto_rng_selftest_s();

	while (count < size) {
		for (i = 0; i < sizeof(data_buf); i += 64) {
			/* Generate 64 bytes of random data at a time */
			ret = crypto_rng_raw_generate(
					(crypto_lib_handle *)&cryptoHandle,
					&data_buf[i], 16,	/* In words */
					NULL, NULL);
			if (ret) {
				TC_ERROR("crypto_rng_raw_generate() error: %d\n"
					 , ret);
				TC_ERROR("Only %d bytes were written\n", count);
				fs_close(&fp);
				return ret;
			}
		}

		ret = fs_write(&fp, data_buf, sizeof(data_buf));
		if (ret < sizeof(data_buf)) {
			TC_ERROR("File write failed: %d\n", ret);
			TC_ERROR("Only %d bytes were written\n", count);
			fs_close(&fp);
			return ret;
		}
		count += sizeof(data_buf);
		if ((count & 0xFFFF) == 0)
			TC_PRINT(".");
	}

	TC_PRINT("Wrote %s file to SD card\n", file);
	fs_close(&fp);

	return 0;
}
#endif

SHELL_TEST_DECLARE(test_sdio)
{
	/* Set board level muxing for SDIO */
	configure_sdio_sel();

#ifdef CONFIG_FILE_SYSTEM
	/*
	 * Run the below command to write a binary file ('rand.bin') with
	 * random numbers worth 10000 bytes to SD card
	 * test sdio_driver gen_rand rand.bin 10000
	 */
	if (argc == 4) {
		if (strcmp("gen_rand", argv[1]) == 0) {
			return dump_rand_data_to_file(
					argv[2], strtoul(argv[3], NULL, 10));
		}
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
#endif

	ztest_test_suite(sd_driver_tests,
			 ztest_unit_test(sdio_init_fs_test));

	ztest_run_test_suite(sd_driver_tests);

	return 0;
}
