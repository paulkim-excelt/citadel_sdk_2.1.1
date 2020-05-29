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

/* @file test_fp_spi.c
 *
 * Unit test to enumerate Finger print sensor (crossmatch) on SPI2 interface
 *
 * This file implements test that enumerates a crossmatch finger print sensor
 */

#include <test.h>
#include <errno.h>
#include <cortex_a/cache.h>
#include <board.h>
#include <spi_legacy.h>

#ifdef CONFIG_FP_SENSOR_ENUMERATE
#include <socregs.h>
#include <gpio.h>
#include <pinmux.h>
#include <i2c.h>

#define FP_SPI_DEV_NAME		CONFIG_SPI2_DEV_NAME
#define SPI_PACKET_SIZE		9
#define FP_SPI_NRESET		32
#define FP_SPI_NAWAKE		5
#define SSPCR1_MS		0x0000	/* Master mode */
#define FP_SPI_NRESET_DRV_STRENGTH	0x4
#define IOMUX_CTRL0_SEL_GPIO32		3

/* IO expander on SVK board needs to be configured
 * to route the SPI2 signals out of the J37 header.
 */
#define IO_EXPANDER_ADDR	0x20
#define IO_EXPANDER_REG		0x4
#define IO_EXPANDER_REGB	0x1
#define IO_EXPANDER_REGC	0x2

static union dev_config i2c_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

static int test_i2c_read_io_exander(struct device *dev,
		u8_t reg_addr, u8_t *val)
{
	struct i2c_msg msgs[2];
	u8_t raddr = reg_addr;

	msgs[0].buf = &raddr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = val;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, IO_EXPANDER_ADDR);
}

static int test_i2c_write_io_exander(struct device *dev,
		u8_t reg_addr, u8_t val)
{
	struct i2c_msg msgs;
	u8_t buff[2];
	u8_t value = val;

	buff[0] = reg_addr;
	buff[1] = value;
	msgs.buf = buff;
	msgs.len = 2;
	msgs.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msgs, 1, IO_EXPANDER_ADDR);
}

static int configure_spi2_sel(void)
{
	int ret;
	u8_t val = 0;
	struct device *dev;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");

	ret = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(ret == 0, "i2c_configure() returned error");

	/* Read port 1 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGB, &val);
	/* Write to port 1 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGB, val & ~0x80);
	/* Configure port 1 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGB, val & ~0x80);
	/* Read back port 1 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGB, &val);
	TC_PRINT("Reg B : %x\n", val);

	/* Read port 2 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGC, &val);
	/* Write to port 2 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGC, val & ~0x80);
	/* Configure port 2 as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGC, val & ~0x80);
	/* Read back port 2 */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGC, &val);
	TC_PRINT("Reg C : %x\n", val);

	return 0;
}

static u32_t flags;

void fp_spi_set_volatile_mem_flag(u32_t flag1)
{
	flags = flag1;
}

u32_t fp_spi_get_volatile_mem_flag(void)
{
	return flags;
}

enum {
	FP_SPI_SENSOR_FOUND	= 0x00000001,
	FP_SPI_INVERT_SS	= 0x00000002,
	FP_SPI_DP_TOUCH		= 0x00000004,
	FP_SPI_NEXT_TOUCH	= 0x00000008,
	FP_SPI_SYNAPTICS_TOUCH	= 0x00000010,
	FP_SPI_WAKEUP_BYPASS	= 0x00000020
};

void fp_spi_ss(u32_t value)
{
	if (flags & FP_SPI_INVERT_SS)
		value = !value;

	spi_slave_select(device_get_binding(FP_SPI_DEV_NAME), value);
}

void fp_spi_nreset(u32_t value)
{
	struct device *gpio_dev;

	switch (FP_SPI_NRESET & ~0x1F) {
	default:
	case 0x0:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
		break;
	case 0x20:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
		break;
	case 0x40:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
		break;
	}

	gpio_pin_write(gpio_dev, FP_SPI_NRESET & 0x1F, value ? 0x1 : 0x0);
}

static void compute_crc(u8_t *command)
{
	u8_t idx;
	u8_t crc = 0;

	/* CRC is computed without a command header */
	u8_t *it = (u8_t *)command + 1;

	for (idx = 0; idx < SPI_PACKET_SIZE - 1; idx++, it++) {
		crc = (u8_t)((u8_t)(crc >> 4) | (crc << 4));
		crc ^= *it;
		crc ^= (u8_t)(crc & 0xf) >> 2;
		crc ^= (crc << 4) << 2;
		crc ^= ((crc & 0xf) << 2) << 1;
	}

	command[2] = crc;
}

int fp_spi_transfer(u8_t *output_buffer, u8_t *input_buffer, u16_t length)
{
	int ret = 0;

	/* basic error checking only */
	if ((input_buffer == (u8_t *)0) || (output_buffer == (u8_t *)0))
		return -EINVAL;

	/* assert slave select */
	fp_spi_ss(0);

	/* worst case sensor vendors cs to clk timing = 31.5 ns */
	k_busy_wait(1); /* Min delay resolution available - 1 us */

	ret = spi_transceive(device_get_binding(FP_SPI_DEV_NAME),
			     output_buffer, length, input_buffer, length);
	if (ret)
		TC_ERROR("Failed to transfer SPI data : (%d)\n", ret);

	/* release slave select */
	fp_spi_ss(1);

	return ret;
}

u8_t  fp_spi_send_status_cmd(u8_t *empty_frame, u8_t *status_rsp)
{
	int i;
	u8_t in_buf[9];
	int try_count = 30;

	while (try_count > 0) {
		k_sleep(1);
		fp_spi_transfer(empty_frame, in_buf, 9);
		if (memcmp(in_buf, status_rsp, 9)) {
			try_count--;
		} else {
			TC_PRINT("STATUS cmd. Success: %d\n", try_count);
			return 1;
		}
	}

	TC_ERROR("Failed cmd. Received:\n");
	for (i = 0; i < 9; i++)
		TC_PRINT("0x%02x ", in_buf[i]);
	TC_PRINT("\n");
	return 0;
}

u32_t fp_spi_nawake(void)
{
	u32_t value;
	struct device *gpio_dev;

	gpio_dev = device_get_binding(CONFIG_GPIO_AON_DEV_NAME);
	gpio_pin_read(gpio_dev, FP_SPI_NAWAKE, &value);

	return value;
}

int fp_spi_set_regs(u16_t data_size, u32_t frequency,
		    u8_t clk_phase, u8_t clk_polarity, u16_t master_slave)
{
	int ret;
	struct spi_config cfg;
	struct device *spi_dev;
	u32_t vendor_spec;

	ARG_UNUSED(master_slave);

	spi_dev = device_get_binding(FP_SPI_DEV_NAME);
	if (spi_dev == NULL) {
		TC_ERROR("Failed to get SPI device binding\n");
		return -ENODEV;
	}

	TC_PRINT("fp-spi speed: %d.%d Mhz\n",
		 frequency/1000000, (frequency/1000) % 1000);

	/* Prepare the config structure
	 * Currently the SPI driver only supports master mode
	 */
	cfg.max_sys_freq = frequency;
	vendor_spec = 0x4 << 7; /* 10 mA drive strength */
	cfg.config = SPI_WORD(data_size) |
		 (clk_polarity ? SPI_MODE_CPOL : 0) |
		 (clk_phase ? SPI_MODE_CPHA : 0) |
		 (vendor_spec << 16);

	ret = spi_configure(spi_dev, &cfg);
	if (ret) {
		/* Currently we don't have a mapping from errno error codes to
		 * PHX_STATUS error codes, so we will just return device_failed
		 * and log the error with a print
		 */
		TC_ERROR("spi_configure() failed with %d\n", ret);
		return ret;
	}

	/* Clear existing IRQ's */
	sys_write32(BIT(SPI2_SSPI2CR__RTIC) | BIT(SPI2_SSPI2CR__RORIC),
		    SPI2_SSPI2CR);

	return 0;
}

int fp_spi_config(u16_t data_size, u32_t frequency,
		  u8_t clk_phase, u8_t clk_polarity, u16_t master_slave)
{
	int ret;
	struct device *gpio_dev;
	struct device *pinmux_dev;

	if (master_slave != SSPCR1_MS) {
		/* Only master mode is currently supported */
		return -EINVAL;
	}

	/* Set SPI interface */
	fp_spi_set_regs(data_size, frequency, clk_phase,
			clk_polarity, master_slave);

	/* At this point GPIO 43 - FP_SPI_SS is already configured as a GPIO
	 * and set to output and initialized to 1. Also, the other SPI signals
	 * (CLK, MOSI, MISO) have been selected on MFIO 42, 44, 45
	 */
	/* Call fp_spi_ss, in case invert_ss is set for the SPI slave */
	fp_spi_ss(1);

	/* take control and set FP_SPI_NAWAKE as input */
	gpio_pin_configure(device_get_binding(CONFIG_GPIO_AON_DEV_NAME),
			   FP_SPI_NAWAKE, GPIO_DIR_IN | GPIO_PUD_PULL_DOWN);

	/* CRMU_IOMUX_CONTROL__CRMU_OVERRIDE_UART_RX bit has to be 1,
	 * then AON_GPIO_5 will be "wired" to FP_NAWAKE
	 */
	sys_write32(BIT(CRMU_IOMUX_CONTROL__CRMU_OVERRIDE_UART_RX),
		    CRMU_IOMUX_CONTROL);

	/* manual control FP_SPI_NRESET, leave in reset */
	switch (FP_SPI_NRESET & ~0x1F) {
	default:
	case 0x0:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_0);
		break;
	case 0x20:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
		break;
	case 0x40:
		gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_2);
		break;
	}

	gpio_pin_write(gpio_dev, FP_SPI_NRESET & 0x1F, 0);
	gpio_pin_configure(gpio_dev, FP_SPI_NRESET & 0x1F, GPIO_DIR_OUT);

	/* increase the 10ma drive strength - GPIO 32 (MFIO 0) */
	pinmux_dev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	ret = pinmux_pin_set(pinmux_dev, 0, (FP_SPI_NRESET_DRV_STRENGTH << 7)
					    | IOMUX_CTRL0_SEL_GPIO32);

	return ret;
}

void test_spi_func_fp_sensor_enumerate(void)
{
	int ret;

	u8_t in_buf[9];
	u32_t i, timeout;
	u8_t wake_cmd[] = {
		0xAF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	u8_t wake_rsp[] = {
		0xAF, 0x42, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	u8_t status_cmd[] = {
		0xAF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	u8_t status_rsp[] = {
		0xAF, 0x44, 0x0A, 0x25, 0xA4, 0x00, 0x00, 0x00, 0x00
	};

	u8_t empty_frame[] = {
		0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	compute_crc(wake_cmd);
	compute_crc(status_cmd);

	/* Enabled SPI2 selection on SVK board */
	configure_spi2_sel();

	/* The first fpOpen call could ignore SPI interface reset.
	 * SS is inverted.
	 */
	fp_spi_set_volatile_mem_flag(FP_SPI_WAKEUP_BYPASS);

	/* 6.25 MHz SPI MODE 0 Master */
	fp_spi_config(8, 6250000, 0, 0, SSPCR1_MS);

	/* Reset */
	fp_spi_ss(1);
	fp_spi_nreset(0);
	k_sleep(6);
	fp_spi_nreset(1);
	k_sleep(200);

	/* Pulse for Wake */
	fp_spi_ss(0);
	k_sleep(6);
	fp_spi_ss(1);

	/* Send Wake */
	TC_PRINT("Sending the WAKE Cmd\n");

	fp_spi_transfer(wake_cmd, in_buf, 9);
	ret = fp_spi_send_status_cmd(empty_frame, wake_rsp);
	zassert_true(ret == 1, "Send status failed\n");

	/* wait for FP_NAWAKE to assert, timeout after 300 ms (max wake time) */
	timeout = 300;
	for (i = 0; i < timeout; i++) {
		if (!fp_spi_nawake())
			break;
		k_sleep(1);
	}
	zassert_true(i < timeout, "Never got AWAKE signal\n");

	/* Get Status */
	TC_PRINT("Sending the Status Cmd\n");

	fp_spi_transfer(status_cmd, in_buf, 9);
	ret = fp_spi_send_status_cmd(empty_frame, status_rsp);
	zassert_true(ret == 1, "Send status failed\n");
}
#else
void test_spi_func_fp_sensor_enumerate(void)
{
	TC_PRINT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
	TC_PRINT("Enable CONFIG_FP_SENSOR_ENUMERATE=1 to run this test\n\n");
	TC_PRINT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}
#endif /* CONFIG_FP_SENSOR_ENUMERATE */

SHELL_TEST_DECLARE(test_fp_spi)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(fp_spi_driver_enum_test,
			 ztest_unit_test(test_spi_func_fp_sensor_enumerate));

	ztest_run_test_suite(fp_spi_driver_enum_test);

	return 0;
}
