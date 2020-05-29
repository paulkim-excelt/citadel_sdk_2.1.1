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
#include <errno.h>
#include <cortex_a/cache.h>
#include <board.h>
#include <spi_legacy.h>

#ifdef CONFIG_SPI_LEGACY_API

#ifdef CONFIG_SOC_HAS_NFC_MODULE
#include <socregs.h>
#include <gpio.h>

static void nfc_reg_ctrl_pu(int val)
{
	u32_t read_val;

	if (val) {
		/* Power ON */
		read_val = sys_read32(NFC_REG_PU_CTRL);
		read_val = read_val | 0x40;
		sys_write32(read_val, NFC_REG_PU_CTRL);
	} else {
		read_val = sys_read32(NFC_REG_PU_CTRL);
		read_val = read_val & (~0x40);
		sys_write32(read_val, NFC_REG_PU_CTRL);
	}

}

static u8_t nfc_spi_int_state(void)
{
	return (sys_read32(NFC_SPI_INT_CTRL) >> NFC_SPI_INT_CTRL__DATA_IN)
		& 0x1;
}

/* FIXME: This test code has been borrowed from Dell CS code base and does not
 * conform to the required coding style guidelines.
 *
 * This test will also be enabled only for SoCs which have the NFC modules on
 * the SoC. This test is expected to fail on SoCs that do not have the SoC
 * modules
 */
void test_spi_legacyfunc_nfc_read_id(void)
{
	int i;
	int timeout = 0;
	u8_t buf[255];
	u32_t read_val;
	/* RESET Notification data */
	u8_t rest_notf_data[5] = {0x60, 0x00, 0x02, 0x00, 0x01};
	u8_t *ptr = buf;
	u8_t in_byte, out_byte;
	struct spi_config cfg;
	u32_t vendor_spec;
	struct device *dev;
	u16_t len = sizeof(buf);

	dev = device_get_binding(CONFIG_SPI5_DEV_NAME);
	for (i = 0; i < len; i++)
		buf[i] = 0;

	cfg.max_sys_freq = 1172000;
	vendor_spec = BIT(10)		| /* Hysteresis enable */
		      GPIO_PUD_PULL_UP	| /* Pull up enable */
		      BIT(4)		| /* Slew rate enable */
		      (0x7 << 7);	  /* Drive strength 16 mA (0x7) */
	cfg.config = SPI_TRANSFER_MSB | SPI_WORD(8)
		     | (vendor_spec << 16);
	spi_configure(dev, &cfg);

	/* make Reg PU OEB to 0 */
	read_val = sys_read32(NFC_REG_PU_CTRL);
	read_val = read_val & (~0x01);
	sys_write32(read_val, NFC_REG_PU_CTRL);

	/* Make SPI_INT PAd as input pin */
	read_val = sys_read32(NFC_SPI_INT_CTRL);
	read_val = read_val & (~0x1);
	/* Enable Hysteresis and Slewrate */
	read_val = read_val | 0x12;
	sys_write32(read_val, NFC_SPI_INT_CTRL);
	/* NFC pins enable drive strength and Hysteresis */
	read_val = sys_read32(NFC_REG_PU_CTRL);
	read_val = read_val | 0x38;
	sys_write32(read_val, NFC_REG_PU_CTRL);

	read_val = sys_read32(NFC_NFC_WAKE_CTRL);
	read_val = read_val | 0x38;
	sys_write32(read_val, NFC_NFC_WAKE_CTRL);

	read_val = sys_read32(NFC_HOST_WAKE_CTRL);
	read_val = read_val | 0x12;
	sys_write32(read_val, NFC_HOST_WAKE_CTRL);

	read_val = sys_read32(NFC_CLK_REQ_CTRL);
	read_val = read_val | 0x12;
	sys_write32(read_val, NFC_CLK_REQ_CTRL);

	/* The following code is removed currently as 20797A1 Eval board is
	 * powered externally. The following code needs to be enabled on 58103
	 * part
	 */

	/* NFC power down */
	nfc_reg_ctrl_pu(0);
	k_busy_wait(100000);
	/* NFC power Up */
	nfc_reg_ctrl_pu(1);

	/* Reading RESET_NTF */
	k_busy_wait(50000);

	while ((timeout++ < 100000) && (nfc_spi_int_state() != 0))
		;
	zassert_true(timeout < 100000, "*** FAIL_Initial_SPI_INIT_LOW ***\n");

	spi_slave_select(dev, 0);

	out_byte = 0x2;
	spi_write(dev, &out_byte, 1);
	out_byte = 0x0;
	spi_write(dev, &out_byte, 1);
	spi_write(dev, &out_byte, 1);

	in_byte = 0;
	while (1) {
		spi_transceive(dev, &out_byte, 1, &in_byte, 1);
		if (in_byte)
			break;
	}

	len = in_byte;

	for (i = 0; i < len; i++)
		spi_transceive(dev, &out_byte, 1, &ptr[i], 1);

	spi_slave_select(dev, 1);

	TC_PRINT("Reading RESET_NTF Data len = 0x%02x\n", in_byte);
	for (i = 0; i < len; i++) {
		zassert_equal(rest_notf_data[i], buf[i], "Data Mismatch");
		TC_PRINT("0x%02x\n", buf[i]);
	}

}
#endif /* CONFIG_SOC_HAS_NFC_MODULE */

#define SPI_TEST_FREQ 1000000

static char *spi_device_name;

void test_spi_legacyapi_configure(void)
{
	int ret;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_configure(dev, NULL);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_legacyapi_slave_select(void)
{
	int ret;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_slave_select(dev, 0);
	zassert_equal(ret, 0, NULL);
}

void test_spi_legacyapi_read(void)
{
	int ret;
	char rx;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_read(dev, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_read(dev, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_read(dev, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_legacyapi_write(void)
{
	int ret;
	char tx;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_write(dev, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_write(dev, &tx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_write(dev, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_legacyapi_transceive(void)
{
	int ret;
	char tx, rx;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_transceive(dev, NULL, 0, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, &tx, 0, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, NULL, 0, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, NULL, 1, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, NULL, 0, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, &tx, 1, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(dev, &tx, 0, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_legacyfunc_configure(void)
{
	int ret;
	struct device *dev;
	struct spi_config cfg;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	cfg.max_sys_freq = SPI_TEST_FREQ;
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(4);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);
}

void test_spi_legacyfunc_slave_select(void)
{
	int ret;
	struct device *dev;

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	ret = spi_slave_select(dev, 0);
	zassert_equal(ret, 0, NULL);

	ret = spi_slave_select(dev, 1);
	zassert_equal(ret, 0, NULL);

	ret = spi_slave_select(dev, 0);
	zassert_equal(ret, 0, NULL);
}

void test_spi_legacyfunc_read(void)
{
	int ret, i, count;
	struct device *dev;
	struct spi_config cfg;
	char rx[16];
	char pattern[16];

	for (i = 0; i < 16; i++)
		pattern[i] = (u8_t)(3*i*i + 7*i + 2);

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	/* Configure in 4 bit data size mode */
	cfg.max_sys_freq = SPI_TEST_FREQ;
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(8);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	memcpy(rx, pattern, 16);
	/* Read 16 bytes */
	ret = spi_read(dev, &rx[0], 16);
	zassert_equal(ret, 0, NULL);
	count = 0;
	for (i = 0; i < 16; i++)
		if (pattern[i] == rx[i])
			count++;
	zassert_true(count < 4, "Atleast 4 bytes read from the SPI "
		"device are matching the random pattern.\nSo either this "
		"is a rare co-incidence or the read is not working");

	/* Read 4 half words and verify the rx buffer is not written beyond
	 * 8 bytes
	 */
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(16);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	memcpy(rx, pattern, 16);
	ret = spi_read(dev, &rx[0], 4);
	zassert_equal(ret, 0, NULL);
	for (i = 8; i < 16; i++)
		zassert_equal(rx[i], pattern[i], NULL);
}

void test_spi_legacyfunc_write(void)
{
	int ret, i;
	struct device *dev;
	struct spi_config cfg;
	char pattern[16];

	for (i = 0; i < 16; i++)
		pattern[i] = (u8_t)(3*i*i + 7*i + 2);

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	/* Configure in 6 bit data size mode */
	cfg.max_sys_freq = SPI_TEST_FREQ;
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(6);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	/* Write 16 bytes */
	ret = spi_write(dev, &pattern[0], 16);
	zassert_equal(ret, 0, NULL);

	/* Read 8 half words */
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(16);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	ret = spi_write(dev, &pattern[0], 8);
	zassert_equal(ret, 0, NULL);
}

void test_spi_legacyfunc_transceive(void)
{
	int ret, i;
	struct device *dev;
	struct spi_config cfg;
	char rx[16];
	char pattern[16];

	for (i = 0; i < 16; i++)
		pattern[i] = (u8_t)(3*i*i + 7*i + 2);

	dev = device_get_binding(spi_device_name);
	zassert_not_null(dev, "SPI Driver binding not found!");

	/* Configure in 4 bit data size mode and loop back mode
	 * and ensure data written is read back
	 */
	cfg.max_sys_freq = SPI_TEST_FREQ;
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA |
		     SPI_MODE_LOOP | SPI_WORD(4);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	memset(rx, 0, 16);
	/* Write and Read 16 bytes */
	ret = spi_transceive(dev, &pattern[0], 16, &rx[0], 16);
	zassert_equal(ret, 0, NULL);

	/* We expect to receive only the 4 LSB bits back (data size - 4) */
	for (i = 0; i < 16; i++)
		zassert_equal(pattern[i] & 0xF, rx[i], NULL);

	/* Write and Read 8 half words and verify the written data is read back
	 */
	cfg.config = SPI_MODE_CPOL | SPI_MODE_CPHA |
		     SPI_MODE_LOOP | SPI_WORD(16);
	ret = spi_configure(dev, &cfg);
	zassert_equal(ret, 0, NULL);

	memset(rx, 0, 16);
	ret = spi_transceive(dev, &pattern[0], 16, &rx[0], 16);
	zassert_equal(ret, 0, NULL);

	/* We expect to receive back with same data that was written*/
	for (i = 0; i < 16; i++)
		zassert_equal(pattern[i], rx[i], NULL);
}

SHELL_TEST_DECLARE(test_spi_legacy)
{
	u8_t spi_dev_idx = 1;

	if (argc == 2) {
		spi_dev_idx = argv[1][0] - 0x30;
		if ((spi_dev_idx < 1) || (spi_dev_idx > 5)) {
			TC_ERROR("SPI Device index : %d not supported\n",
				 spi_dev_idx);
			return -1;
		}
	}

	switch (spi_dev_idx) {
	default:
	case 1:
		spi_device_name = CONFIG_SPI1_DEV_NAME;
		break;
	case 2:
		spi_device_name = CONFIG_SPI2_DEV_NAME;
		break;
	case 3:
		spi_device_name = CONFIG_SPI3_DEV_NAME;
		break;
	case 4:
		spi_device_name = CONFIG_SPI4_DEV_NAME;
		break;
	case 5:
		spi_device_name = CONFIG_SPI5_DEV_NAME;
		break;
	}

	TC_PRINT("SPI driver (legacy) tests will be run for SPI%d\n",
		  spi_dev_idx);

	ztest_test_suite(spi_driver_api_tests,
			 ztest_unit_test(test_spi_legacyapi_configure),
			 ztest_unit_test(test_spi_legacyapi_slave_select),
			 ztest_unit_test(test_spi_legacyapi_read),
			 ztest_unit_test(test_spi_legacyapi_write),
			 ztest_unit_test(test_spi_legacyapi_transceive));

	ztest_test_suite(spi_driver_functional_tests,
			 ztest_unit_test(test_spi_legacyfunc_configure),
			 ztest_unit_test(test_spi_legacyfunc_slave_select),
			 ztest_unit_test(test_spi_legacyfunc_read),
			 ztest_unit_test(test_spi_legacyfunc_write),
#ifdef CONFIG_SOC_HAS_NFC_MODULE
			 ztest_unit_test(test_spi_legacyfunc_nfc_read_id),
#endif
			 ztest_unit_test(test_spi_legacyfunc_transceive));

	ztest_run_test_suite(spi_driver_api_tests);
	ztest_run_test_suite(spi_driver_functional_tests);

	return 0;
}

#endif /* CONFIG_SPI_LEGACY_API */
