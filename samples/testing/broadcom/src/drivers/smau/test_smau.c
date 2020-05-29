/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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
#include <board.h>
#include <device.h>
#include <smau.h>
#include <qspi_flash.h>
#include <test.h>

u32_t keyAesCBC[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	};
u32_t keyAuthCBC[] = {
		0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28
	};
u32_t keyAesCBC1[] = {
		0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
	};
u32_t keyAuthCBC1[] = {
		0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30
	};
u32_t keyAesCBC2[] = {
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
	};
u32_t keyAuthCBC2[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38
	};
u32_t keyAesCBC3[] = {
		0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
	};
u32_t keyAuthCBC3[] = {
		0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
	};

u32_t enc_image[] = {
	0x1b508234, 0xa6572f77, 0x74106b85, 0xb2ce6727,
	0x481ac02e, 0x7d419774, 0xd6e983e5, 0x1dc75005,
	0x18279ffd, 0x4c30aedd, 0x59705f63, 0x5c0a1b29,
	0x3298ed44, 0xe8bd3732, 0x8717462b, 0x921585f4,
	0x9b0ee344, 0x4a721c08, 0x09d37183, 0x52187d06,
	0x2fcbc06d, 0x2f38349e, 0x4530a715, 0xc9c69dbf,
	0x8543e20a, 0xc5a6e589, 0xae5e665f, 0x1c0e0d12,
	0x0fde6be8, 0x9204cd5e, 0x354ba9a3, 0xc304e825,
	0x5e0ce042, 0xc72a5f88, 0xc04f7a42, 0xfe622e9f,
	0x4ab92a76, 0x8e4f86bd, 0xdc4e549a, 0x166c6759,
	0x71fff191, 0xcf9e5659, 0x9efd01b5, 0xfc39db2a,
	0x1a683faf, 0x50b18881, 0x289271d7, 0x72f67d81,
	0x484958e8, 0x84792f27, 0xd0532ef3, 0x030fa8b4,
	0xcad6b030, 0xb2154de1, 0x50b7a7bd, 0xada18998,
	0xfd06235d, 0xfdd57db2, 0x8f1ac354, 0xb82c4b16,
	0xa9a3907f, 0x3d79941d, 0x263f9a76, 0xe6487c67,
};

u32_t plain_image[] = {
	0xea000006, 0xea00002f, 0xea000035, 0xea000040,
	0xea000051, 0xe320f000, 0xea000056, 0xea000059,
	0xe3a01000, 0xe3a02000, 0xe3a03000, 0xe3a04000,
	0xe3a05000, 0xe3a06000, 0xe3a07000, 0xe3a08000,
	0xe3a09000, 0xe3a0a000, 0xe3a0b000, 0xe3a0c000,
	0xe3a0e000, 0xe59f013c, 0xe321f0d1, 0xe1a0d000,
	0xe3a0e000, 0xe2400a01, 0xe321f0d2, 0xe1a0d000,
	0xe3a0e000, 0xe2400a01, 0xe321f0df, 0xe1a0d000,
	0xe3a0e000, 0xe2400a01, 0xe321f0d7, 0xe1a0d000,
	0xe3a0e000, 0xe2400a01, 0xe321f0db, 0xe1a0d000,
	0xe3a0e000, 0xe2400a01, 0xe321f0d3, 0xe1a0d000,
	0xe3a0e000, 0xe10f0000, 0xe3c000c0, 0xe121f000,
	0xeb00007c, 0xeb0002d3, 0xe92d5fff, 0xe59fe0c8,
	0xe59e0000, 0xe1a0e00f, 0xe12fff10, 0xe8bd5fff,
	0xe1b0f00e, 0xe92d5fff, 0xe14f0000, 0xe3100020,
	0x115e00b2, 0x13c00cff, 0x051e0004, 0x03c004ff,
};

u32_t hmac_image[] = {
	0x94a55990, 0xeef35c15, 0x038dfcf4, 0x39e34e3e,
	0x85c3d320, 0x5bc162ed, 0xa3cb5c55, 0xe2613277,
};

static struct device *flash_dev;
#define BUFF_SIZE	128UL
u8_t buff[BUFF_SIZE*2];
u8_t read_buff[BUFF_SIZE*2];

#define SMAU_FLASH_ACCESS_ADDR_WINDOW0 0x63000000
#define SMAU_FLASH_ACCESS_ADDR_WINDOW1 0x64000000
#define QSPI_DRIVER_TEST_ADDRESS	0x200000	/* 8 Meg offset */
#define QSPI_DRIVER_HMAC_TEST_ADDRESS	0x242000	/* hmac offset */
#define QSPI_DRIVER_TEST_SECTOR_SIZE	0x10000		/* 64 K sector size */

/* FIXME ideally the following things should come from smau.h,
 * they are not available yet
 */
#define SMU_F_aes_key_length_MASK	0x00020000
/* AES-CBC with HMAC_SHA256 */
#define SMU_F_cache_crypto_algo_MASK	0x00008000
#define SMU_F_page_replacement_MASK	0x00004000
#define SMU_F_smu_addr_chk_MASK		0x00002000
#define SMU_F_ctw_ro_chk_MASK		0x00001000
#define SMU_F_ctw_range_chk_MASK	0x00000800
#define SMU_F_auth_with_iv_MASK		0x00000400
#define SMU_F_auth_first_MASK		0x00000200
#define SMU_F_cache_ram_prot_MASK	0x00000100

#define SMAU_WINDOW_BASE_ADDR		0x63200000
#define SMAU_WINDOW_SIZE_1MB		0xFFF00000
#define SMAU_WINDOW_HMAC_BASE_ADDR	0x63242000

#define SMU_CFG_0_INIT_VAL	0xFF0006AA
#define SMAU_RETURN_WAIT() do {			\
		volatile int i;			\
		for (i = 0; i < 100; i++);	\
	} while (0)

typedef enum select_algo {
	PLAIN,
	ENCRYPTION,
	HMAC
} select_algo_t;

static void dumpHex(u8_t *ptr, u32_t size)
{
	u32_t i;

	for (i = 0; i < size; i++, ptr++) {
		TC_PRINT("0x%02x ", *ptr);
		if ((i & 0x0F) == 0x0F)
			TC_PRINT("\n");
	}
	if ((i & 0x0F) != 0x00)
		TC_PRINT("\n");
}

static void test_qspi_write(struct qspi_flash_config *cfg)
{
	int ret;
	u32_t test_addr, i;

	zassert_not_null(flash_dev, "Driver binding not found!");

	for (i = 0; i < BUFF_SIZE; i++)
		buff[i] = 0x80 + i + QSPI_DATA_LANE(cfg->op_mode);

	/* Erase 2 sectors */
	ret = qspi_flash_erase(flash_dev, QSPI_DRIVER_TEST_ADDRESS,
			       2*QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	/* Read and verify 128 bytes across a sector boundary
	 * and ensure the contents are 0xFF
	 */
	test_addr = QSPI_DRIVER_TEST_ADDRESS + QSPI_DRIVER_TEST_SECTOR_SIZE
		    - (BUFF_SIZE / 2);
	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_true(read_buff[i] == 0xFF, NULL);

	/* Apply configuration only before the write, which is being tested */
	ret = qspi_flash_configure(flash_dev, cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Write the pattern and read it back */
	ret = qspi_flash_write(flash_dev, test_addr, BUFF_SIZE, &buff[0]);
	zassert_equal(ret, 0, "Flash write returned error");

	/* Restore to single lane mode */
	if (QSPI_DATA_LANE(cfg->op_mode) != QSPI_MODE_SINGLE) {
		SET_QSPI_DATA_LANE(cfg->op_mode, QSPI_MODE_SINGLE);
		ret = qspi_flash_configure(flash_dev, cfg);
		zassert_equal(ret, 0, "Flash configure returned error");
	}

	/* Read and verify */
	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_equal(buff[i], read_buff[i], NULL);

	TC_PRINT("After writing to the flash area, 0x%x\n", test_addr);
	dumpHex((u8_t *)read_buff, BUFF_SIZE);
}

static void test_qspi_write_encrypted(struct qspi_flash_config *cfg)
{
	int ret;
	u32_t test_addr, i;

	zassert_not_null(flash_dev, "Driver binding not found!");

	memcpy(buff, enc_image, BUFF_SIZE*2);

	/* Erase 2 sectors */
	ret = qspi_flash_erase(flash_dev, QSPI_DRIVER_TEST_ADDRESS,
			       2*QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	/* Read and verify 128 bytes across a sector boundary
	 * and ensure the contents are 0xFF
	 */
	test_addr = QSPI_DRIVER_TEST_ADDRESS;

	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE*2, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE; i++)
		zassert_true(read_buff[i] == 0xFF, NULL);

	/* Apply configuration only before the write, which is being tested */
	ret = qspi_flash_configure(flash_dev, cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Write the pattern and read it back */
	ret = qspi_flash_write(flash_dev, test_addr, BUFF_SIZE*2, &buff[0]);
	zassert_equal(ret, 0, "Flash write returned error");

	/* Restore to single lane mode */
	if (QSPI_DATA_LANE(cfg->op_mode) != QSPI_MODE_SINGLE) {
		SET_QSPI_DATA_LANE(cfg->op_mode, QSPI_MODE_SINGLE);
		ret = qspi_flash_configure(flash_dev, cfg);
		zassert_equal(ret, 0, "Flash configure returned error");
	}

	/* Read and verify */
	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE*2, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE*2; i++)
		zassert_equal(buff[i], read_buff[i], NULL);
	TC_PRINT("After writing to the flash area, 0x%x\n", test_addr);
	dumpHex((u8_t *)read_buff, BUFF_SIZE*2);
}

static void test_qspi_write_hmac(struct qspi_flash_config *cfg)
{
	int ret;
	u32_t test_addr, i;

	zassert_not_null(flash_dev, "Driver binding not found!");

	memcpy(buff, hmac_image, BUFF_SIZE/4);

	/* Erase 2 sectors */
	ret = qspi_flash_erase(flash_dev, QSPI_DRIVER_HMAC_TEST_ADDRESS,
			       QSPI_DRIVER_TEST_SECTOR_SIZE);
	zassert_equal(ret, 0, "Flash erase returned error");

	/* Read and verify 128 bytes across a sector boundary
	 * and ensure the contents are 0xFF
	 */
	test_addr = QSPI_DRIVER_HMAC_TEST_ADDRESS;

	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE/4, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE/4; i++)
		zassert_true(read_buff[i] == 0xFF, NULL);

	/* Apply configuration only before the write, which is being tested */
	ret = qspi_flash_configure(flash_dev, cfg);
	zassert_equal(ret, 0, "Flash configure returned error");

	/* Write the pattern and read it back */
	ret = qspi_flash_write(flash_dev, test_addr, BUFF_SIZE/4, &buff[0]);
	zassert_equal(ret, 0, "Flash write returned error");

	/* Restore to single lane mode */
	if (QSPI_DATA_LANE(cfg->op_mode) != QSPI_MODE_SINGLE) {
		SET_QSPI_DATA_LANE(cfg->op_mode, QSPI_MODE_SINGLE);
		ret = qspi_flash_configure(flash_dev, cfg);
		zassert_equal(ret, 0, "Flash configure returned error");
	}

	/* Read and verify */
	ret = qspi_flash_read(flash_dev, test_addr, BUFF_SIZE/4, &read_buff[0]);
	zassert_equal(ret, 0, "Flash read returned error");

	for (i = 0; i < BUFF_SIZE/4; i++)
		zassert_equal(buff[i], read_buff[i], NULL);
	TC_PRINT("After writing hmac to the flash area, 0x%x\n", test_addr);
	dumpHex((u8_t *)read_buff, BUFF_SIZE/4);
}

static void test_qspi_single_lane_write(select_algo_t algo)
{
	struct qspi_flash_config cfg;

	/* Configure single lane mode */
	cfg.op_mode = 0;
	cfg.frequency = 12500000;
	cfg.slave_num = 0;
	SET_QSPI_CLOCK_POL(cfg.op_mode, 0x1);
	SET_QSPI_CLOCK_PHASE(cfg.op_mode, 0x1);
	SET_QSPI_DATA_LANE(cfg.op_mode, QSPI_MODE_SINGLE);

	switch (algo) {

	case PLAIN:
		test_qspi_write(&cfg);
		break;
	case ENCRYPTION:
		test_qspi_write_encrypted(&cfg);
		break;
	case HMAC:
		test_qspi_write_hmac(&cfg);
		break;
	}
}

static void smau_test_window_plain(void)
{

	struct device *smau_dev;
	u32_t smu_cfg0_common_params = 0x00000000;
	u32_t win1_base = 0;
	u32_t win1_size_mask;
	u32_t win1_hmac = 0;
	u32_t smu_swap = 0;
	u32_t key;
	int ret = -1;
	u32_t testaddr = (u32_t)(SMAU_FLASH_ACCESS_ADDR_WINDOW1
			+ QSPI_DRIVER_TEST_SECTOR_SIZE
			- (BUFF_SIZE / 2));

	smau_dev = device_get_binding(CONFIG_SMAU_NAME);
	zassert_not_null(smau_dev, "Critical error: Smau is not initialized!");
	flash_dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);

	/* Write some patterns for the flash */
	test_qspi_single_lane_write(PLAIN);

	/* Disable the IRQ's. since we are executing the smau in ram, and if
	 * interrupt arrives, the handler will be in flash and flash accesses
	 * are not allowed during the window configuration
	 * Configure the smau windows
	 */
	key = irq_lock();

	/* Disable the cache */
	smau_disable_cache(smau_dev);

	/* Is this initialization required ? FIXME */
	smau_mem_init_extra(smau_dev);

	smu_cfg0_common_params = 0;

	/* Configure the AES and HMAC keys for window 1 */
	smau_config_aeskeys(smau_dev, 1, 32, keyAesCBC1);
	smau_config_authkeys(smau_dev, 1, 32, keyAuthCBC1);

	win1_base = SMAU_WINDOW_BASE_ADDR;
	win1_size_mask = SMAU_WINDOW_SIZE_1MB;
	win1_hmac = SMAU_WINDOW_HMAC_BASE_ADDR;

	smau_config_windows(smau_dev, 1, win1_base, win1_size_mask, win1_hmac,
	1 /* spi_mapped */, 1 /* auth_bypass */, 1 /* No enc_bypass */,
	smu_cfg0_common_params, smu_swap);

	/* enable cache */
	smau_enable_cache(smau_dev);

	irq_unlock(key);

	/* Verify the read */
	ret = memcmp((u8_t *)testaddr, read_buff, BUFF_SIZE);
	zassert_equal(ret, 0, "Flash read via smau returned error");

	TC_PRINT("After configuring the smau read the flash area, "
			"window1 0x%x\n", testaddr);
	dumpHex((u8_t *)testaddr, BUFF_SIZE);

	/* Put back the window configuration for normal operation */
	sys_write32(0x0, SMU_CFG_1);
	sys_write32(SMU_CFG_0_INIT_VAL, SMU_CFG_0);
	sys_write32(1 << SMU_CFG_1__CACHE_ENABLE, SMU_CFG_1);

	SMAU_RETURN_WAIT();
}

static void smau_test_window_encryption(void)
{
	struct device *smau_dev;
	u32_t smu_cfg0_common_params = 0x00000000;
	u32_t win1_base = 0;
	u32_t win1_size_mask;
	u32_t win1_hmac = 0;
	u32_t smu_swap = 0;
	u32_t key;
	u32_t testaddr = (u32_t)(SMAU_FLASH_ACCESS_ADDR_WINDOW1);
	int ret = -1;

	smau_dev = device_get_binding(CONFIG_SMAU_NAME);
	zassert_not_null(smau_dev, "Critical error: Smau is not initialized!");
	flash_dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);

	/* Write some patterns for the flash */
	test_qspi_single_lane_write(ENCRYPTION);

	/* Disable the IRQ's. since we are executing the smau in ram, and if
	 * interrupt arrives, the handler will be in flash and flash accesses
	 * are not allowed during the window configuration
	 * Configure the smau windows
	 */
	key = irq_lock();

	/* Disable the cache */
	smau_disable_cache(smau_dev);

	/* Is this initialization required ? FIXME */
	smau_mem_init_extra(smau_dev);

	/* Configure the AES and HMAC keys for window 1 */
	smau_config_aeskeys(smau_dev, 1, 32, keyAesCBC1);
	smau_config_authkeys(smau_dev, 1, 32, keyAuthCBC1);

	/* Enable the 32 bit key encryption,
	 * Enable with IV mask,
	 * Enable with auth first then encrypt mask
	 * Crypto mask AES-CBC with HMAC_SHA256
	 */
	smu_cfg0_common_params |= (SMU_F_aes_key_length_MASK |
			SMU_F_auth_with_iv_MASK |
			SMU_F_auth_first_MASK |
			SMU_F_cache_crypto_algo_MASK);

	win1_base = SMAU_WINDOW_BASE_ADDR;
	win1_size_mask = SMAU_WINDOW_SIZE_1MB;
	win1_hmac = SMAU_WINDOW_HMAC_BASE_ADDR;

	smau_config_windows(smau_dev, 1, win1_base, win1_size_mask, win1_hmac,
	1 /* spi_mapped */, 1 /* auth_bypass */, 0 /* enc_bypass */,
	smu_cfg0_common_params, smu_swap);

	/* enable cache */
	smau_enable_cache(smau_dev);

	irq_unlock(key);

	/* Verify the read */
	ret = memcmp((u8_t *)(testaddr), plain_image, BUFF_SIZE*2);
	zassert_equal(ret, 0, "decrypted Flash read via smau returned error");

	TC_PRINT("After configuring the smau read the flash area, "
			"window1 0x%x\n", testaddr);
	dumpHex((u8_t *)testaddr, BUFF_SIZE*2);

	/* Put back the window configuration for normal operation */
	sys_write32(0x0, SMU_CFG_1);
	sys_write32(SMU_CFG_0_INIT_VAL, SMU_CFG_0);
	sys_write32(1 << SMU_CFG_1__CACHE_ENABLE, SMU_CFG_1);

	SMAU_RETURN_WAIT();
}

static void smau_test_window_hmac(void)
{
	struct device *smau_dev;
	u32_t smu_cfg0_common_params = 0x00000000;
	u32_t win1_base = 0;
	u32_t win1_size_mask;
	u32_t win1_hmac = 0;
	u32_t smu_swap = 0;
	u32_t key;
	u32_t testaddr = (u32_t)(SMAU_FLASH_ACCESS_ADDR_WINDOW1);
	int ret = -1;

	smau_dev = device_get_binding(CONFIG_SMAU_NAME);
	zassert_not_null(smau_dev, "Critical error: Smau is not initialized!");
	flash_dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);

	/* Write some patterns for the flash */
	test_qspi_single_lane_write(ENCRYPTION);

	/* Write some patterns for the flash */
	test_qspi_single_lane_write(HMAC);

	/* Disable the IRQ's. since we are executing the smau in ram, and if
	 * interrupt arrives, the handler will be in flash and flash accesses
	 * are not allowed during the window configuration
	 * Configure the smau windows
	 */
	key = irq_lock();

	/* Disable the cache */
	smau_disable_cache(smau_dev);

	/* Is this initialization required ? FIXME */
	smau_mem_init_extra(smau_dev);

	/* Configure the AES and HMAC keys for window 1 */
	smau_config_aeskeys(smau_dev, 1, 32, keyAesCBC1);
	smau_config_authkeys(smau_dev, 1, 32, keyAuthCBC1);

	/* Enable the 32 bit key encryption,
	 * Enable with IV mask,
	 * Enable with auth first then encrypt mask
	 * Crypto mask AES-CBC with HMAC_SHA256
	 */
	smu_cfg0_common_params |= (SMU_F_aes_key_length_MASK |
			SMU_F_auth_with_iv_MASK |
			SMU_F_cache_crypto_algo_MASK);

	win1_base = SMAU_WINDOW_BASE_ADDR;
	win1_size_mask = SMAU_WINDOW_SIZE_1MB;
	win1_hmac = SMAU_WINDOW_HMAC_BASE_ADDR;

	smau_config_windows(smau_dev, 1, win1_base, win1_size_mask, win1_hmac,
	1 /* spi_mapped */, 0 /* auth_bypass */, 0 /* enc_bypass */,
	smu_cfg0_common_params, smu_swap);

	/* enable cache */
	smau_enable_cache(smau_dev);

	irq_unlock(key);

	TC_PRINT("After configuring the smau read the flash area,"
			"window1 0x%x\n", testaddr);
	dumpHex((u8_t *)testaddr, BUFF_SIZE*2);

	/* Verify the read */
	ret = memcmp((u8_t *)(testaddr), plain_image, BUFF_SIZE*2);
	zassert_equal(ret, 0, "decrypted Flash with hmac "
			"verified read returned error");

	/* Put back the window configuration for normal operation */
	sys_write32(0x0, SMU_CFG_1);
	sys_write32(SMU_CFG_0_INIT_VAL, SMU_CFG_0);
	sys_write32(1 << SMU_CFG_1__CACHE_ENABLE, SMU_CFG_1);

	SMAU_RETURN_WAIT();
}

/* Default SMAU window settings */
#define WIN_BASE_1	0x64000000
#define WIN_BASE_2	0x65000000
#define WIN_BASE_3	0x66000000
#define WIN_SIZE_DEF	0xFF000000
#define HMAC_BASE_1	0x64042000
#define HMAC_BASE_2	0x65044000
#define HMAC_BASE_3	0x66046000

SHELL_TEST_DECLARE(test_smau)
{
	static bool init_done = false;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_BOARD_SERP_CP
	TC_PRINT("Skipping SMAU tests for SERP board\n");
	TC_PRINT("All the test data is encrypted for flash offset 8 MB\n");
	TC_PRINT("And serp board has only a 4 MB flash. See CS-4112\n");
	return 0;
#endif

	if (init_done == false) {
		u32_t key = irq_lock();
		struct device *dev = device_get_binding(CONFIG_SMAU_NAME);

		smau_config_windows(dev, 1, WIN_BASE_1, WIN_SIZE_DEF,
				    HMAC_BASE_1, 1, 0, 0, 0, 0);
		smau_config_windows(dev, 2, WIN_BASE_2, WIN_SIZE_DEF,
				    HMAC_BASE_2, 1, 0, 0, 0, 0);
		smau_config_windows(dev, 3, WIN_BASE_3, WIN_SIZE_DEF,
				    HMAC_BASE_3, 1, 0, 0, 0, 0);
		init_done = true;
		irq_unlock(key);
	}

	ztest_test_suite(smau_tests, ztest_unit_test(smau_test_window_plain),
				ztest_unit_test(smau_test_window_encryption),
				ztest_unit_test(smau_test_window_hmac));

	ztest_run_test_suite(smau_tests);

	return 0;
}
