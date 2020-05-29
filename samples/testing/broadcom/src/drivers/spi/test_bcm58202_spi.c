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

/* @file test_spi_bcm58202.c
 *
 * Unit test for BCM5820X SPI driver
 *
 * This file implements the unit tests for the bcm5820x spi driver
 */

#ifndef CONFIG_SPI_LEGACY_API

#include <test.h>
#include <errno.h>
#include <cortex_a/cache.h>
#include <board.h>
#include <broadcom/spi.h>
#include <broadcom/gpio.h>
#include <broadcom/dma.h>
#include <pinmux.h>

#ifdef CONFIG_SPI_DMA_MODE
#define SPI_DEF_FREQ	KHZ(100)
#else
#define SPI_DEF_FREQ	MHZ(1)
#endif

/*
 * The SPI device used for api test should be different from the
 * ones (SPI2/3) used for the master-slave functional tests, as the reads/writes
 * from the api tests could affect the master-slave tests by leaving stale
 * data in the rx/tx fifos
 */
#define TEST_SPI_DEV CONFIG_SPI5_DEV_NAME

static struct spi_cs_control def_csel;

/* IO expander on SVK board needs to be configured
 * to route the SPI1 signals out of the J37 header.
 */
#include <broadcom/i2c.h>

#define IO_EXPANDER_ADDR	0x20
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

static void setup_spi2_spi3_sel(void)
{
	int ret;
	u8_t val = 0;
	struct device *dev;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	zassert_not_null(dev, "I2C Driver binding not found!");

	ret = i2c_configure(dev, i2c_cfg.raw);
	zassert_true(ret == 0, "i2c_configure() returned error");

	/* Read port 1 value */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGB, &val);
	/* Write to port 1 value [7:6] = 0 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGB, val & ~0xC0);
	/* Read port 1 direction reg */
	test_i2c_read_io_exander(dev, 0x18 + IO_EXPANDER_REGB, &val);
	/* Configure port 1 [7:6] as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGB, val & ~0xC0);

	/* Read port 2 value */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGC, &val);
	/* Write to port 2 value [7:4] = 0 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGC, val & ~0xFC);
	/* Read port 2 direction reg */
	test_i2c_read_io_exander(dev, 0x18 + IO_EXPANDER_REGC, &val);
	/* Configure port 2 [7:4] as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGC, val & ~0xFC);

	/* Read port 5 value */
	test_i2c_read_io_exander(dev, IO_EXPANDER_REGE, &val);
	/* Write to port 5 value [2:1] = 0 */
	test_i2c_write_io_exander(dev, 0x8 + IO_EXPANDER_REGE, val & ~0x06);
	/* Read port 5 direction reg */
	test_i2c_read_io_exander(dev, 0x18 + IO_EXPANDER_REGE, &val);
	/* Configure port 5 [2:1] as output */
	test_i2c_write_io_exander(dev, 0x18 + IO_EXPANDER_REGE, val & ~0x06);
}

void get_default_spi_config(struct spi_config *cfg)
{
	cfg->dev = device_get_binding(TEST_SPI_DEV);
	cfg->frequency = SPI_DEF_FREQ;
	cfg->operation =  SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |
			  SPI_TRANSFER_MSB | SPI_WORD_SET(8);
	cfg->vendor = 0;
	cfg->slave = 0;
	cfg->cs = &def_csel;

	/* Use MFIO 11 (SPI2_SS) as slave select GPIO (GPIO 43 -> group 1) */
	def_csel.gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	def_csel.gpio_pin = 43;
	def_csel.delay = 100;
}

void test_spi_api_read(void)
{
	int ret;
	char byte;
	struct spi_buf rx;
	struct spi_config cfg;

	rx.buf = &byte;
	rx.len = 1;

	get_default_spi_config(&cfg);

	ret = spi_read(&cfg, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_read(&cfg, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_read(&cfg, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_api_write(void)
{
	int ret;
	char byte;
	struct spi_buf tx;
	struct spi_config cfg;

	tx.buf = &byte;
	tx.len = 1;

	get_default_spi_config(&cfg);

	ret = spi_write(&cfg, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_write(&cfg, &tx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_write(&cfg, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);
}

void test_spi_api_transceive(void)
{
	int ret;
	char tx_byte;
	struct spi_buf tx, rx;
	struct spi_config cfg;

	rx.buf = cache_line_aligned_alloc(1);
	rx.len = 1;
	tx.buf = &tx_byte;
	tx.len = 1;

	get_default_spi_config(&cfg);

	ret = spi_transceive(&cfg, NULL, 0, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, &tx, 0, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, NULL, 0, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, NULL, 1, NULL, 0);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, NULL, 0, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, &tx, 1, NULL, 1);
	zassert_equal(ret, -EINVAL, NULL);

	ret = spi_transceive(&cfg, &tx, 0, &rx, 0);
	zassert_equal(ret, -EINVAL, NULL);

	cache_line_aligned_free(rx.buf);
}

void test_spi_api_release(void)
{
	int ret;
	struct spi_buf rx;
	struct spi_config cfg;

	rx.buf = cache_line_aligned_alloc(1);
	rx.len = 1;

	get_default_spi_config(&cfg);

	/* Release without locking */
	ret = spi_release(&cfg);
	zassert_equal(ret, -ENOLCK, NULL);

	/* Release operation not valid for slave mode */
	cfg.operation |= SPI_OP_MODE_SLAVE;
	ret = spi_release(&cfg);
	zassert_equal(ret, -EINVAL, NULL);

	cfg.operation &= ~SPI_OP_MODE_SLAVE;
	cfg.operation |= SPI_LOCK_ON;

	/* Valid lock and release */
	ret = spi_read(&cfg, &rx, 1);
	zassert_equal(ret, 0, NULL);

	ret = spi_release(&cfg);
	zassert_equal(ret, 0, NULL);

	cache_line_aligned_free(rx.buf);
}

/* SPI slave thread */
#define STACK_SIZE	2048
static K_THREAD_STACK_DEFINE(spi_slave_task_stack, STACK_SIZE);
static struct k_thread spi_slave_task;
#define TEST_NUM_BYTES	100

/* Calcualtor Operation codes */
#define OP_CODE_DONE	0x0
#define OP_CODE_ADD	0x1
#define OP_CODE_SUB	0x2
#define OP_CODE_MUL	0x3
#define OP_CODE_DIV	0x4
#define OP_CODE_MAX	OP_CODE_DIV

/* @brief Slave task echo back
 * @details In this task, the slave SPI reads from the master
 *	    and echoes back the value read in the previous read
 *	    after modifing it with the equation: TEST_NUM_BYTES - rx_byte.
 *	    The test verifies each byte that is echoed.
 *	    In addition the slave task returns the sum total of all bytes
 *	    received which is again verified by the master to qualify the
 *	    test as PASS.
 */
static void spi_slave_task_echo_back(void *p1, void *p2, void *p3)
{
	int ret;
	u8_t tx_byte = TEST_NUM_BYTES;
	u32_t *done = (u32_t *)p2;
	u32_t sum = 0, received;
	struct spi_buf rx_buf, tx_buf;
	struct spi_config *cfg = (struct spi_config *)p1;

	/* Here done is used to return the sum or -1 on failure */
	*done = 0;

	/* Read data from the slave (SPI 3) */
	rx_buf.buf = cache_line_aligned_alloc(1);
	received = 0;
	tx_buf.buf = &tx_byte;
	tx_buf.len = 1;
	while (received < TEST_NUM_BYTES) {
		u8_t byte;
		rx_buf.len = 1;

		ret = spi_transceive(cfg, &tx_buf, 1, &rx_buf, 1);
		byte = *(u8_t *)rx_buf.buf;
		sum += byte;
		TC_PRINT("[SLAVE] : Sent 0x%x, Received 0x%x\n", *((u8_t *)tx_buf.buf), byte);
		if (ret)
			TC_ERROR("Slave SPI read returned error: %d\n", ret);
		/* Send back the (TEST_NUM_BYTES - rx_byte) */
		tx_byte = TEST_NUM_BYTES - byte;
		received++;
	}

	cache_line_aligned_free(rx_buf.buf);
	TC_PRINT("Sum of bytes received = %d\n", sum);
	*done = sum ? sum : -1UL;
}

/*
 * @brief Get SPI master configuration
 */
static void get_spi_master_cfg(struct spi_config *cfg)
{
	int ret;
	static struct spi_cs_control cs;
	struct device *spi_m = device_get_binding(CONFIG_SPI2_DEV_NAME);

	zassert_not_null(spi_m, "Could not get dev binding for SPI master (2)");

	/* Set SPI master config */
	cfg->dev = spi_m;
	cfg->frequency = SPI_DEF_FREQ;
	cfg->operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |
			 SPI_TRANSFER_MSB | SPI_WORD_SET(8);
	cfg->vendor = 0;
	cfg->slave = 0;
	cfg->cs = &cs;

	/* Use MFIO 11 (SPI2_SS) as slave select GPIO (GPIO 43 -> group 1) */
	cs.gpio_dev = device_get_binding(CONFIG_GPIO_DEV_NAME_1);
	cs.gpio_pin = 43; /* GPIO pin goes from 0 - 31 */
	cs.delay = 0; /* No delay after transaction completes */

	/* Configure MFIO 11 as gpio for slave select (GPIO 43) */
	ret = pinmux_pin_set(device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME),
			     cs.gpio_pin - 32, 3);
	zassert_true(ret == 0, "Error setting SS pad control for SPI2");
	ret = gpio_pin_configure(cs.gpio_dev, cs.gpio_pin % 32, GPIO_DIR_OUT);
	zassert_true(ret == 0, "Failed to configure chip select pin as output");
	ret = gpio_pin_write(cs.gpio_dev, cs.gpio_pin % 32, 1);
	zassert_true(ret == 0, "Failed to write to chip select pin");
}

/*
 * @brief Get SPI slave configuration
 */
static void get_spi_slave_cfg(struct spi_config *cfg)
{
	struct device *spi_s = device_get_binding(CONFIG_SPI3_DEV_NAME);
	zassert_not_null(spi_s, "Could not get dev binding for SPI slave (3)");

	/* Set SPI slave config */
	cfg->dev = spi_s;
	cfg->frequency = SPI_DEF_FREQ;
	cfg->operation = SPI_OP_MODE_SLAVE | SPI_MODE_CPOL | SPI_MODE_CPHA |
			 SPI_TRANSFER_MSB | SPI_WORD_SET(8);
	cfg->cs = NULL;
}

/* SPI master and slave configurations */
static struct spi_config spi_cfg_m, spi_cfg_s;

/*
 * @brief SPI Master function for the echo back test
 */
void test_spi_master_slave_echo(void)
{
	int ret, i;
	u8_t byte;
	k_tid_t id;
	volatile u32_t read_done;
	struct spi_buf tx_buf, rx_buf;

	/* Get SPI master and slave configurations */
	get_spi_master_cfg(&spi_cfg_m);
	get_spi_slave_cfg(&spi_cfg_s);

	read_done = 0;
	/* Create SPI slave thread to read sent data */
	id = k_thread_create(&spi_slave_task, spi_slave_task_stack,
			     STACK_SIZE, spi_slave_task_echo_back, &spi_cfg_s,
			     (void *)&read_done, NULL,
			     K_PRIO_COOP(7), 0, K_NO_WAIT);
	zassert_not_null(id, "Failed to create SPI master thread");

	/* Send data over SPI 2 using a thread */
	/* Allow some time for the slave read to start */
	k_sleep(100);

	/* Send 100 bytes */
	tx_buf.buf = &byte;
	tx_buf.len = 1;
	rx_buf.buf = cache_line_aligned_alloc(1);
	rx_buf.len = 1;
	for (i = 1; i <= TEST_NUM_BYTES; i++) {
		u8_t rx_byte;

		/* Send byte */
		byte = i;
		ret = spi_transceive(&spi_cfg_m, &tx_buf, 1, &rx_buf, 1);
		if (ret)
			TC_ERROR("Error Writing SPI byte : %d\n", ret);
		rx_byte = *(u8_t *)rx_buf.buf;
		TC_PRINT("[MASTER]: Sent 0x%x, Received 0x%x\n", byte, rx_byte);
		/* +1 because the rx_byte is is generated by slave using the
		 * sent byte from previous iteration
		 */
		zassert_true((byte + rx_byte) == (TEST_NUM_BYTES + 1),
			     "Data not looped back from slave");
		/* Yield to allow Slave thread to execute */
		k_sleep(10);
	}

	while (read_done == 0)
		k_sleep(1);

	cache_line_aligned_free(rx_buf.buf);

	/* Abort the SPI master thread */
	k_thread_abort(id);

	zassert_true(read_done == TEST_NUM_BYTES*(TEST_NUM_BYTES+1)/2,
		     "Did not receive all the bytes sent!");
}

/* @brief Slave task (calculator)
 * @details In this task, the slave task waits for a command from the master
 *	    followed by 2 byte operands. It then computes the result based
 *	    on the command of operation and the operands and returns a 16 bit
 *	    result LSB first. The test verifies the result returned
 */
static void spi_slave_task_calculator(void *p1, void *p2, void *p3)
{
	int ret;
	u16_t res;
	u8_t ops[2], cmd;
	char op[] = {'X', '+', '-', '*', '/'};
	u32_t *done = (u32_t *)p2;
	struct spi_buf rx_buf, tx_buf;
	struct spi_config *cfg = (struct spi_config *)p1;

	u8_t *rx_byte = cache_line_aligned_alloc(2);

	*done = 0;
	do {
		rx_buf.buf = rx_byte;
		rx_buf.len = 1;
		ret = spi_read(cfg, &rx_buf, 1);
		if (ret)
			TC_ERROR("[Slave] : Error reading: %d\n", ret);
		cmd = *rx_byte;

		TC_PRINT("[Slave] : Command = cmd: %c\n", op[cmd]);
		if ((cmd > OP_CODE_MAX) || (cmd == OP_CODE_DONE)) {
			TC_PRINT("Exiting slave task\n");
			break;
		}

		/* Valid command received: Read operands */
		rx_buf.buf = rx_byte;
		rx_buf.len = 2;
		ret = spi_read(cfg, &rx_buf, 1);
		if (ret)
			TC_ERROR("[Slave] : Error reading operands: %d\n", ret);
		ops[0] = *rx_byte;
		ops[1] = *(rx_byte+1);
		TC_PRINT("[Slave] : OP: [%d %c %d]\n", ops[0], op[cmd], ops[1]);

		switch (cmd) {
		case OP_CODE_ADD: res = ops[0] + ops[1]; break;
		case OP_CODE_SUB: res = ops[0] - ops[1]; break;
		case OP_CODE_MUL: res = ops[0] * ops[1]; break;
		case OP_CODE_DIV: res = ops[0] / ops[1]; break;
		}
		TC_PRINT("[Slave] : Result = %d\n", res);

		/* Return result */
		tx_buf.buf = &res;
		tx_buf.len = 2;
		ret = spi_write(cfg, &tx_buf, 1);
		if (ret)
			TC_ERROR("[Slave] : Error sending result: %d\n", ret);
	} while (1);

	cache_line_aligned_free(rx_byte);

	*done = 1;
}

/*
 * @breif Send a command and get result from SPI slave
 */
u16_t get_op_result(struct spi_config *cfg_m, u8_t cmd, u8_t op_a, u8_t op_b)
{
	int ret;
	u8_t *res;
	u16_t result;
	struct spi_buf tx_buf, rx_buf;

	tx_buf.len = 1;
	rx_buf.len = 1;

	res = cache_line_aligned_alloc(2);

	/* Send command */
	tx_buf.buf = &cmd;
	ret = spi_write(cfg_m, &tx_buf, 1);
	k_sleep(5); /* Needed we are running master & slave on the same board */
	if (ret)
		TC_PRINT("[Master]: Error sending command %d\n", ret);

	/* No operands for done command */
	if (cmd == OP_CODE_DONE)
		return 0;

	/* Send operand a */
	tx_buf.buf = &op_a;
	ret = spi_write(cfg_m, &tx_buf, 1);
	k_sleep(5); /* Needed we are running master & slave on the same board */
	if (ret)
		TC_PRINT("[Master]: Error sending Operand 'a' %d\n", ret);

	/* Send operand b */
	tx_buf.buf = &op_b;
	ret = spi_write(cfg_m, &tx_buf, 1);
	k_sleep(5); /* Needed we are running master & slave on the same board */
	if (ret)
		TC_PRINT("[Master]: Error sending Operand 'b' %d\n", ret);

	/* Read result */
	rx_buf.buf = &res[0];
	ret = spi_read(cfg_m, &rx_buf, 1);
	k_sleep(5); /* Needed we are running master & slave on the same board */
	if (ret)
		TC_PRINT("[Master]: Error reading result [LSB] %d\n", ret);

	rx_buf.buf = &res[1];
	ret = spi_read(cfg_m, &rx_buf, 1);
	if (ret)
		TC_PRINT("[Master]: Error reading result [MSB] %d\n", ret);
	k_sleep(5); /* Needed we are running master & slave on the same board */

	result = *res | ((u16_t)(*(res + 1)) << 8);
	cache_line_aligned_free(res);

	return result;
}

/*
 * @brief SPI Master function for the echo back test
 */
void test_spi_slave_calculator(void)
{
	u16_t res;
	k_tid_t id;
	volatile u32_t slave_exited;

	/* Get SPI master and slave configurations */
	get_spi_master_cfg(&spi_cfg_m);
	get_spi_slave_cfg(&spi_cfg_s);

	slave_exited = 0;
	/* Create SPI slave thread to read sent data */
	id = k_thread_create(&spi_slave_task, spi_slave_task_stack,
			     STACK_SIZE, spi_slave_task_calculator, &spi_cfg_s,
			     (void *)&slave_exited, NULL,
			     K_PRIO_COOP(7), 0, K_NO_WAIT);
	zassert_not_null(id, "Failed to create SPI master thread");

	/* Send data over SPI 2 usng a thread */
	/* Allow some time for the slave read to start */
	k_sleep(100);

	/* Verify add */
	res = get_op_result(&spi_cfg_m, OP_CODE_ADD, 0x24, 0x3F);
	zassert_equal(res, 0x24 + 0x3F, NULL);

	/* Verify sub */
	res = get_op_result(&spi_cfg_m, OP_CODE_SUB, 0xF3, 0x28);
	zassert_equal(res, 0xF3 - 0x28, NULL);

	/* Verify mul */
	res = get_op_result(&spi_cfg_m, OP_CODE_MUL, 0x32, 0x7D);
	zassert_equal(res, 0x32 * 0x7D, NULL);

	/* Verify div */
	res = get_op_result(&spi_cfg_m, OP_CODE_DIV, 0x92, 0x11);
	zassert_equal(res, 0x92 / 0x11, NULL);

	/* Get slave task to exit */
	get_op_result(&spi_cfg_m, OP_CODE_DONE, 0, 0);

	/* Allow time for slave thread to exit */
	while (!slave_exited)
		k_sleep(100);

	/* Abort the SPI master thread */
	k_thread_abort(id);
}

/* This test verifies the interrupt mode of the SPI slave functionality
 * The test configures SPI3 as slave and SPI2 as master. The test then
 * sends a sequence of bytes from the master port and checks it against
 * the bytes received from the slave. The test passes when all the bytes
 * sent are received from the slave port.
 *
 * spi_write() in slave mode cannot be tested in interrupt mode, since
 * the call to spi_write() will block in interrupt mode and the spi_read()
 * from master which initiates the transaction cannot execute, resulting
 * in a deadlock(). Note that this limitation only applies because the SPI
 * slave and master are running on the same board, which is only a test
 * scenario. For a real application, which would have the master on an
 * external device, the spi_write() call will block till the master initiates
 * a read and it would then return.
 */
#define INT_TEST_TFR_SIZE	256
#define INT_DUPLEX_TFR_SIZE 70

static K_THREAD_STACK_DEFINE(spi_slave_task_stack, STACK_SIZE);
static struct k_thread spi_slave_task;
/* SPI master thread */
static K_THREAD_STACK_DEFINE(spi_master_task_stack, STACK_SIZE);
static struct k_thread spi_master_task;
struct k_sem slave_ready;
struct k_sem slave_done;
struct k_sem master_done;
/* Slave will tx these 3 buffers */
struct spi_buf s_tx_buf;
struct spi_buf s_rx_buf;
/* Master will tx these 3 buffers */
struct spi_buf m_tx_buf;
struct spi_buf m_rx_buf;
u32_t scratch_data;

#ifdef CONFIG_SPI_DMA_MODE
static void spi_slave_test_dma_cb(void *arg)
{
	int ret;
	struct spi_config *cfg = (struct spi_config *)arg;

	if (s_rx_buf.buf == NULL)
		s_rx_buf.buf = cache_line_aligned_alloc(INT_TEST_TFR_SIZE);

	memset(s_rx_buf.buf, 0, INT_TEST_TFR_SIZE);
	s_rx_buf.len = INT_TEST_TFR_SIZE;

	ret = spi_read(cfg, &s_rx_buf, 1);
	if (ret)
		TC_ERROR("[Slave] : Error reading: %d\n", ret);
}
#else
/* @brief Slave task (txrx)
 * @details In this task, the slave task waits for a command from the master
 *	    followed by 2 byte operands. It then computes the result based
 *	    on the command of operation and the operands and returns a 16 bit
 *	    result LSB first. The test verifies the result returned
 */
static void spi_slave_task_txrx(void *p1, void *p2, void *p3)
{
	int ret, i;
	struct spi_config *cfg = (struct spi_config *)p1;
	u32_t *sdone = (u32_t *)p2;

	/* Hardware Bug WAR: Blocking Slave Rx.
	 * The issue is that Master fails to receive the first 4 bytes
	 * sent by slave presumably because the slave fifo gets filled
	 * only upon receiving the interrupt on "Rx Fifo not empty"
	 * condition. However, we don't know what makes masters "Rx
	 * fifo not empty" condition true especially when the slave
	 * has not send any data. But we have no way of knowing how a
	 * different SPI vendor's master behaves. So we assume that
	 * slave's fifo is simply not ready with the data when master
	 * clocks the data out. To address this presumed issue, we
	 * pre-fill the Slave Tx Fifo with 4 or buffer length bytes
	 * of data (whichever is lower), so that when master starts
	 * clocking this data can be clocked out of slave's tx fifo.
	 * However, the hardware seems to be capable of doing this
	 * only when this pre-filling (which preceds any master slave
	 * communication with slave in interrupt mode) itself is preceded
	 * by some transmit (at least one byte) from master.
	 *
	 * The following lines are to do a receive of one byte from the
	 * master before actually preparing a full Transcieve with master
	 * with a pre-filled Tx fifo.
	 *
	 * The transceive below uses busy wait, so is not expected to
	 * yield to the main master thread. However, it seems the scehduler
	 * is able to bring back the main master thread since it had yielded
	 * to this slave thread by sleeping for 10ms.
	 */
	s_rx_buf.buf = &scratch_data;
	s_rx_buf.len = 1;
	ret = spi_transceive(cfg, NULL, 0, &s_rx_buf, 1);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);

	*sdone = 0;
	s_tx_buf.buf = cache_line_aligned_alloc(INT_DUPLEX_TFR_SIZE);
	memset(s_tx_buf.buf, 0, INT_DUPLEX_TFR_SIZE);
	s_tx_buf.len = INT_DUPLEX_TFR_SIZE;
	s_rx_buf.buf = cache_line_aligned_alloc(INT_DUPLEX_TFR_SIZE);
	memset(s_rx_buf.buf, 0, INT_DUPLEX_TFR_SIZE);
	s_rx_buf.len = INT_DUPLEX_TFR_SIZE;
	/* Prepare Slave Tx data */
	for (i = 0; i < INT_DUPLEX_TFR_SIZE; i++)
		((u8_t *)s_tx_buf.buf)[i] = (i ^ 0x5A);

	ret = spi_transceive(cfg, &s_tx_buf, 1, &s_rx_buf, 1);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	*sdone = 1;
}
#endif

static void test_spi_slave_interrupt_mode(void)
{
	int ret, i;
#ifndef CONFIG_SPI_DMA_MODE
	u32_t data = 0x55;
	k_tid_t sid;
	volatile u32_t slave_exited;
#endif

	/* Get SPI master and slave configurations */
	get_spi_master_cfg(&spi_cfg_m);
	get_spi_slave_cfg(&spi_cfg_s);

#ifdef CONFIG_SPI_DMA_MODE
	/* Install SPI slave callback */
	ret = spi_slave_cb_register(
			&spi_cfg_s, spi_slave_test_dma_cb, &spi_cfg_s);
	zassert_equal(ret, 0, NULL);
#else
	/* Create SPI slave thread */
	sid = k_thread_create(&spi_slave_task, spi_slave_task_stack,
			     STACK_SIZE, spi_slave_task_txrx, &spi_cfg_s,
			     (void *)&slave_exited, NULL,
			     K_PRIO_COOP(7), 0, K_NO_WAIT);
	zassert_not_null(sid, "Failed to create SPI slave thread");

	/* Wait until after the slave callback is initialised
	 * before starting the tx from master.
	 */
	k_sleep(10);
#endif

#ifndef CONFIG_SPI_DMA_MODE
	/* Hardware Bug WAR: Blocking Master Tx - See comment above
	 * This is required only for the first test using interrupt
	 * mode.
	 */
	m_tx_buf.buf = &data;
	m_tx_buf.len = 1;
	ret = spi_transceive(&spi_cfg_m, &m_tx_buf, 1, NULL, 0);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	/* Wait for a second for Slave to read the sent dat */
	k_sleep(500);
	TC_PRINT("Slave Rxed: %x\n", scratch_data);
#endif

	/* Prepare master Tx & Rx buffers and Tx data */
#ifdef CONFIG_SPI_DMA_MODE
	m_tx_buf.len = INT_TEST_TFR_SIZE;
#else
	m_tx_buf.len = INT_DUPLEX_TFR_SIZE;
#endif
	m_tx_buf.buf = cache_line_aligned_alloc(m_tx_buf.len);
	memset(m_tx_buf.buf, 0, m_tx_buf.len);
	for (i = 0; i < m_tx_buf.len; i++)
		((u8_t *)m_tx_buf.buf)[i] = (i ^ 0xA5);

#ifndef CONFIG_SPI_DMA_MODE
	m_rx_buf.buf = cache_line_aligned_alloc(INT_DUPLEX_TFR_SIZE);
	memset(m_rx_buf.buf, 0, INT_DUPLEX_TFR_SIZE);
	m_rx_buf.len = INT_DUPLEX_TFR_SIZE;
	ret = spi_transceive(&spi_cfg_m, &m_tx_buf, 1, &m_rx_buf, 1);
#else
	ret = spi_write(&spi_cfg_m, &m_tx_buf, 1);
#endif
	zassert_equal(ret, 0, NULL);

	/* Wait for a second for Slave to read the sent dat */
	k_sleep(1000);

#ifdef CONFIG_SPI_DMA_MODE
	zassert_not_null(s_rx_buf.buf, "rx is NULL, callback never triggered?");
#else
	zassert_not_null(s_rx_buf.buf, "rx is NULL, Slave Transceive task never spawned?");
#endif

#ifndef CONFIG_SPI_DMA_MODE
	TC_PRINT("\nMaster Expected: ");
	for (i = 0; i < m_rx_buf.len; i++)
		TC_PRINT("%02x,", i ^ 0x5A);
	TC_PRINT("\nMaster Received: ");
	for (i = 0; i < m_rx_buf.len; i++)
		TC_PRINT("%02x,", ((u8_t *)m_rx_buf.buf)[i]);
	TC_PRINT("\n-----------------------------------");
#endif
	TC_PRINT("\nSlave Expected: ");
	for (i = 0; i < s_rx_buf.len; i++)
		TC_PRINT("%02x,", i ^ 0xA5);
	TC_PRINT("\nSlave Received: ");
	for (i = 0; i < s_rx_buf.len; i++) {
		TC_PRINT("%02x,", ((u8_t *)s_rx_buf.buf)[i]);
		if (((u8_t *)s_rx_buf.buf)[i] != (i ^ 0xA5))
			TC_ERROR("rx[%d] = %02x\n", i, ((u8_t *)s_rx_buf.buf)[i]);
		zassert_equal(i ^ 0xA5, ((u8_t *)s_rx_buf.buf)[i], NULL);
	}
	TC_PRINT("\n");

#ifdef CONFIG_SPI_DMA_MODE
	/* Uninstall SPI slave callback */
	ret = spi_slave_cb_register(&spi_cfg_s, NULL, NULL);
	zassert_equal(ret, 0, NULL);
#endif

	/* Free memory allocated in callback */
	if (s_rx_buf.buf) {
		cache_line_aligned_free(s_rx_buf.buf);
		s_rx_buf.buf = NULL;
	}
	if (s_tx_buf.buf) {
		cache_line_aligned_free(s_tx_buf.buf);
		s_tx_buf.buf = NULL;
	}
	if (m_rx_buf.buf) {
		cache_line_aligned_free(m_rx_buf.buf);
		m_rx_buf.buf = NULL;
	}
	if (m_tx_buf.buf) {
		cache_line_aligned_free(m_tx_buf.buf);
		m_tx_buf.buf = NULL;
	}

#ifndef CONFIG_SPI_DMA_MODE
	/* Abort the SPI slave thread */
	k_thread_abort(sid);
#endif
}

SHELL_TEST_DECLARE(test_spi)
{
	/* Setup IO expander straps for SPI2, SPI3 selection */
	setup_spi2_spi3_sel();

	ztest_test_suite(spi_driver_api_tests,
			 ztest_unit_test(test_spi_api_release),
			 ztest_unit_test(test_spi_api_read),
			 ztest_unit_test(test_spi_api_write),
			 ztest_unit_test(test_spi_api_transceive));

	ztest_test_suite(spi_driver_functional_tests,
			 ztest_unit_test(test_spi_slave_interrupt_mode),
			 ztest_unit_test(test_spi_master_slave_echo),
			 ztest_unit_test(test_spi_slave_calculator));

	ztest_run_test_suite(spi_driver_functional_tests);
	ztest_run_test_suite(spi_driver_api_tests);

	return 0;
}

/******************************************************************************
 ********************  Single Board & Double Boards Tests *********************
 *****************************************************************************/
/* @brief Slave Transceive task Non-blocking
 * @details In this task, the slave transceives some data
 * with master. This task should spawn before master task
 * and call it transceive function which should block.
 */
static void spi_slave_task_1board(void *p1, void *p2, void *p3)
{
	int i, ret;
	struct spi_config *cfg = (struct spi_config *)p1;
	u32_t *sdone = (u32_t *)p2;
	u32_t txdata[5] = {0x77, 0x66, 0x55, 0x44, 0x33};
	u32_t rxdata[5] = {0x77, 0x66, 0x55, 0x44, 0x33};
	u32_t txlen = (*((u32_t *)p3)) & 0xffff;
	u32_t rxlen = *((u32_t *)p3) >> 16;

	/* Hardware WAR */
	s_rx_buf.buf = &rxdata[0];
	s_rx_buf.len = 1;
	s_tx_buf.buf = &txdata[0];
	s_tx_buf.len = 1;
	ret = spi_transceive(cfg, &s_tx_buf, 1, &s_rx_buf, 1);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	for (i = 0; i < 1; i++)
		TC_PRINT("Slave Txed %x - Rxed %x\n", ((u8_t *)s_tx_buf.buf)[i], ((u8_t *)s_rx_buf.buf)[i]);

	*sdone = 0;
	s_tx_buf.buf = cache_line_aligned_alloc(txlen);
	memset(s_tx_buf.buf, 0, txlen);
	s_tx_buf.len = txlen;
	s_rx_buf.buf = cache_line_aligned_alloc(rxlen);
	memset(s_rx_buf.buf, 0, rxlen);
	s_rx_buf.len = rxlen;
	for (i = 0; i < txlen; i++)
		((u8_t *)s_tx_buf.buf)[i] = (i ^ 0xA5);

	/* Start Tx & Rx */
	if (txlen && rxlen)
		ret = spi_transceive(cfg, &s_tx_buf, 1, &s_rx_buf, 1);
	else if (txlen)
		ret = spi_transceive(cfg, &s_tx_buf, 1, NULL, 0);
	else if (rxlen)
		ret = spi_transceive(cfg, NULL, 0, &s_rx_buf, 1);
	/* Needed since running master & slave on the same board */
	k_sleep(10);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	*sdone = 1;
}

/* @brief Master Transceive task Non-blocking
 * @details In this task, the Master transceives some data
 * with slave.
 */
static void spi_master_task_1board(void *p1, void *p2, void *p3)
{
	int i, ret;
	struct spi_config *cfg = (struct spi_config *)p1;
	u32_t *mdone = (u32_t *)p2;
	u32_t txlen = (*((u32_t *)p3)) & 0xffff;
	u32_t rxlen = *((u32_t *)p3) >> 16;

#ifndef CONFIG_SPI_DMA_MODE
	/* Hardware Bug WAR: Blocking Master Tx - See comment above
	 * This is required only for the first test using interrupt
	 * mode.
	 */
	u32_t txdata[5] = {0x12, 0x34, 0x56, 0x78, 0x9a};
	u32_t rxdata[5] = {0x12, 0x34, 0x56, 0x78, 0x9a};
	m_tx_buf.buf = &txdata[0];
	m_tx_buf.len = 1;
	m_rx_buf.buf = &rxdata[0];
	m_rx_buf.len = 1;
	ret = spi_transceive(cfg, &m_tx_buf, 1, &m_rx_buf, 1);
	if (ret)
		TC_PRINT("Master TxRx Error %d\n", ret);

	/* Wait for a second for Slave to read the sent dat */
	k_sleep(1);
	for (i = 0; i < 1; i++)
		TC_PRINT("Master Txed %x - Rxed: %x\n", ((u8_t *)m_tx_buf.buf)[i], ((u8_t *)m_rx_buf.buf)[i]);
#endif

	*mdone = 0;
	m_tx_buf.buf = cache_line_aligned_alloc(txlen);
	memset(m_tx_buf.buf, 0, txlen);
	m_tx_buf.len = txlen;
	m_rx_buf.buf = cache_line_aligned_alloc(rxlen);
	memset(m_rx_buf.buf, 0, rxlen);
	m_rx_buf.len = rxlen;
	for (i = 0; i < txlen; i++)
		((u8_t *)m_tx_buf.buf)[i] = (i ^ 0x5A);

	/* Start Tx & Rx */
	if (txlen && rxlen)
		ret = spi_transceive(cfg, &m_tx_buf, 1, &m_rx_buf, 1);
	else if (txlen)
		ret = spi_transceive(cfg, &m_tx_buf, 1, NULL, 0);
	else if (rxlen)
		ret = spi_transceive(cfg, NULL, 0, &m_rx_buf, 1);
	/* Needed since running master & slave on the same board */
	k_sleep(10);
	if (ret)
		TC_PRINT("Master TxRx Error %d\n", ret);
	*mdone = 1;
}

static int test_spi_interrupt_mode_1board(u16_t stxlen, u16_t srxlen)
{
	int i;
	k_tid_t mid, sid;
	volatile u32_t slave_exited;
	volatile u32_t master_exited;
	int slave_res = 0, master_res = 0;
	/* master's rx is slave's tx and vice versa
	 * the parameters to this function are from slave's PoV.
	 */
	u32_t srxtxlen = ((srxlen << 16) | stxlen);
	u32_t mrxtxlen = ((stxlen << 16) | srxlen);

	/* Get SPI master and slave configurations */
	get_spi_master_cfg(&spi_cfg_m);
	get_spi_slave_cfg(&spi_cfg_s);

	k_sem_init(&slave_ready, 0, 1);

	slave_exited = 0;
	/* Create SPI slave thread */
	sid = k_thread_create(&spi_slave_task, spi_slave_task_stack,
			     STACK_SIZE, spi_slave_task_1board, &spi_cfg_s,
			     (void *)&slave_exited, &srxtxlen,
			     K_PRIO_COOP(7), 0, K_NO_WAIT);
	zassert_not_null(sid, "Failed to create SPI slave thread");

	/* Wait until after the slave callback is initialised
	 * before starting the tx from master. Slave thread
	 * will start and block on txrx_complete semaphore.
	 * k_sem_take(&slave_ready, K_FOREVER);
	 *
	 * This sleep will allow the slave thread to run.
	 */
	k_sleep(100);

	master_exited = 0;
	/* Create SPI master thread */
	mid = k_thread_create(&spi_master_task, spi_master_task_stack,
			     STACK_SIZE, spi_master_task_1board, &spi_cfg_m,
			     (void *)&master_exited, &mrxtxlen,
			     K_PRIO_COOP(7), 0, K_NO_WAIT);
	zassert_not_null(mid, "Failed to create SPI master thread");

	/* Allow time for slave & master threads to exit */
	while (!slave_exited || !master_exited)
		k_sleep(100);

	zassert_not_null(m_rx_buf.buf, "m_rx is NULL..!");
	zassert_not_null(s_rx_buf.buf, "s_rx is NULL..!");
	/* Compare results */
	for (i = 0; i < s_rx_buf.len; i++) {
		if (((u8_t *)s_rx_buf.buf)[i] != (i ^ 0x5A)) {
			TC_ERROR("Slave Failed at rx[%d] = %02x\n", i, ((u8_t *)s_rx_buf.buf)[i]);
			slave_res = -1;
			break;
		}
	}
	if (slave_res) {
		TC_PRINT("Slave Expected: ");
		for (i = 0; i < s_rx_buf.len; i++)
			TC_PRINT("%02x,", i ^ 0x5A);
		TC_PRINT("\nSlave Received: ");
		for (i = 0; i < s_rx_buf.len; i++)
			TC_PRINT("%02x,", ((u8_t *)s_rx_buf.buf)[i]);
	}
	TC_PRINT("\n----------------------------------------------\n");
	for (i = 0; i < m_rx_buf.len; i++) {
		if (((u8_t *)m_rx_buf.buf)[i] != (i ^ 0xA5)) {
			TC_ERROR("Master Failed at rx[%d] = %02x\n", i, ((u8_t *)m_rx_buf.buf)[i]);
			master_res = -1;
			break;
		}
	}
	if (master_res) {
		TC_PRINT("\nMaster Expected: ");
		for (i = 0; i < m_rx_buf.len; i++)
			TC_PRINT("%02x,", i ^ 0xA5);
		TC_PRINT("\nMaster Received: ");
		for (i = 0; i < m_rx_buf.len; i++)
			TC_PRINT("%02x,", ((u8_t *)m_rx_buf.buf)[i]);
		TC_PRINT("\n");
	}

	/* Free memory allocated in callback */
	if (s_rx_buf.buf) {
		cache_line_aligned_free(s_rx_buf.buf);
		s_rx_buf.buf = NULL;
	}
	if (s_tx_buf.buf) {
		cache_line_aligned_free(s_tx_buf.buf);
		s_tx_buf.buf = NULL;
	}
	if (m_rx_buf.buf) {
		cache_line_aligned_free(m_rx_buf.buf);
		m_rx_buf.buf = NULL;
	}
	if (m_tx_buf.buf) {
		cache_line_aligned_free(m_tx_buf.buf);
		m_tx_buf.buf = NULL;
	}
	/* Abort the SPI slave & master threads */
	k_thread_abort(mid);
	k_thread_abort(sid);

	return (slave_res | master_res);
}

/* @brief Slave Transceive task Non-blocking
 * @details In this task, the slave transceives some data
 * with master. This task should spawn before master task
 * and call it transceive function which should block.
 */
static void spi_slave_task_2boards(void *p1, void *p2, void *p3)
{
	int i, ret;
	struct spi_config *cfg = (struct spi_config *)p1;
	u32_t txdata[5] = {0x77, 0x66, 0x55, 0x44, 0x33};
	u32_t rxdata[5] = {0x77, 0x66, 0x55, 0x44, 0x33};
	u32_t txlen = (*((u32_t *)p3)) & 0xffff;
	u32_t rxlen = *((u32_t *)p3) >> 16;

	/* Hardware WAR */
	s_rx_buf.buf = &rxdata[0];
	s_rx_buf.len = 1;
	s_tx_buf.buf = &txdata[0];
	s_tx_buf.len = 1;
	ret = spi_transceive(cfg, &s_tx_buf, 1, &s_rx_buf, 1);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	for (i = 0; i < 1; i++)
		TC_PRINT("Slave Txed %x - Rxed %x\n", ((u8_t *)s_tx_buf.buf)[i], ((u8_t *)s_rx_buf.buf)[i]);

	s_tx_buf.buf = cache_line_aligned_alloc(txlen);
	memset(s_tx_buf.buf, 0, txlen);
	s_tx_buf.len = txlen;
	s_rx_buf.buf = cache_line_aligned_alloc(rxlen);
	memset(s_rx_buf.buf, 0, rxlen);
	s_rx_buf.len = rxlen;
	for (i = 0; i < txlen; i++)
		((u8_t *)s_tx_buf.buf)[i] = (i ^ 0xA5);

	/* Start Tx & Rx */
	if (txlen && rxlen)
		ret = spi_transceive(cfg, &s_tx_buf, 1, &s_rx_buf, 1);
	else if (txlen)
		ret = spi_transceive(cfg, &s_tx_buf, 1, NULL, 0);
	else if (rxlen)
		ret = spi_transceive(cfg, NULL, 0, &s_rx_buf, 1);
	if (ret)
		TC_PRINT("Slave TxRx Error %d\n", ret);
	k_sem_give(&slave_done);
}

/* @brief Master Transceive task Non-blocking
 * @details In this task, the Master transceives some data
 * with slave.
 */
static void spi_master_task_2boards(void *p1, void *p2, void *p3)
{
	int i, ret;
	struct spi_config *cfg = (struct spi_config *)p1;
	u32_t txlen = (*((u32_t *)p3)) & 0xffff;
	u32_t rxlen = *((u32_t *)p3) >> 16;

	/* Hardware Bug WAR: Blocking Master Tx - See comment above
	 * This is required only for the first test using interrupt
	 * mode.
	 */
	u32_t txdata[5] = {0x12, 0x34, 0x56, 0x78, 0x9a};
	u32_t rxdata[5] = {0x12, 0x34, 0x56, 0x78, 0x9a};
	m_tx_buf.buf = &txdata[0];
	m_tx_buf.len = 1;
	m_rx_buf.buf = &rxdata[0];
	m_rx_buf.len = 1;
	ret = spi_transceive(cfg, &m_tx_buf, 1, &m_rx_buf, 1);
	if (ret)
		TC_PRINT("Master TxRx Error %d\n", ret);
	/* Wait for a second for Slave to read the sent dat */
	k_sleep(3);
	for (i = 0; i < 1; i++)
		TC_PRINT("Master Txed %x - Rxed: %x\n", ((u8_t *)m_tx_buf.buf)[i], ((u8_t *)m_rx_buf.buf)[i]);

	m_tx_buf.buf = cache_line_aligned_alloc(txlen);
	memset(m_tx_buf.buf, 0, txlen);
	m_tx_buf.len = txlen;
	m_rx_buf.buf = cache_line_aligned_alloc(rxlen);
	memset(m_rx_buf.buf, 0, rxlen);
	m_rx_buf.len = rxlen;
	for (i = 0; i < txlen; i++)
		((u8_t *)m_tx_buf.buf)[i] = (i ^ 0x5A);

	/* Start Tx & Rx */
	if (txlen && rxlen)
		ret = spi_transceive(cfg, &m_tx_buf, 1, &m_rx_buf, 1);
	else if (txlen)
		ret = spi_transceive(cfg, &m_tx_buf, 1, NULL, 0);
	else if (rxlen)
		ret = spi_transceive(cfg, NULL, 0, &m_rx_buf, 1);
	if (ret)
		TC_PRINT("Master TxRx Error %d\n", ret);
	k_sem_give(&master_done);
}

/* This test need to be invoked separately on two different boards.
 * one as master and another as slave.
 */
static int test_spi_interrupt_mode_2boards(u32_t role, u16_t txlen, u16_t rxlen)
{
	/* role = 1 : Master */
	if (role) {
		int i, master_res = 0;
		k_tid_t mid;
		u32_t mtxrxlen = (rxlen << 16) | txlen;

		/* Get SPI master and slave configurations */
		get_spi_master_cfg(&spi_cfg_m);

		k_sem_init(&master_done, 0, 1);
		/* Create SPI master thread */
		mid = k_thread_create(&spi_master_task, spi_master_task_stack,
					 STACK_SIZE, spi_master_task_2boards, &spi_cfg_m,
					 NULL, &mtxrxlen,
					 K_PRIO_COOP(7), 0, K_NO_WAIT);
		zassert_not_null(mid, "Failed to create SPI master thread");
		/* Allow time for slave & master threads to exit */
		k_sleep(100);
		k_sem_take(&master_done, K_FOREVER);

		zassert_not_null(m_rx_buf.buf, "m_rx is NULL..!");
		/* Compare results */
		for (i = 0; i < m_rx_buf.len; i++) {
			if (((u8_t *)m_rx_buf.buf)[i] != (i ^ 0xA5)) {
				TC_ERROR("Master Failed at rx[%d] = %02x\n", i, ((u8_t *)m_rx_buf.buf)[i]);
				master_res = -1;
				break;
			}
		}
		if (master_res) {
			TC_PRINT("\nMaster Expected: ");
			for (i = 0; i < m_rx_buf.len; i++)
				TC_PRINT("%02x,", i ^ 0xA5);
			TC_PRINT("\nMaster Received: ");
			for (i = 0; i < m_rx_buf.len; i++)
				TC_PRINT("%02x,", ((u8_t *)m_rx_buf.buf)[i]);
		}
		TC_PRINT("\n----------------------------------------------\n");
		/* Free memory allocated in callback */
		if (m_rx_buf.buf) {
			cache_line_aligned_free(m_rx_buf.buf);
			m_rx_buf.buf = NULL;
		}
		if (m_tx_buf.buf) {
			cache_line_aligned_free(m_tx_buf.buf);
			m_tx_buf.buf = NULL;
		}
		/* Abort the SPI master thread */
		k_thread_abort(mid);

		return master_res;
	} else {
		int i, slave_res = 0;
		k_tid_t sid;
		u32_t stxrxlen = (rxlen << 16) | txlen;

		/* Get SPI master and slave configurations */
		get_spi_slave_cfg(&spi_cfg_s);

		k_sem_init(&slave_done, 0, 1);
		/* Create SPI slave thread */
		sid = k_thread_create(&spi_slave_task, spi_slave_task_stack,
					 STACK_SIZE, spi_slave_task_2boards, &spi_cfg_s,
					 NULL, &stxrxlen,
					 K_PRIO_COOP(7), 0, K_NO_WAIT);
		zassert_not_null(sid, "Failed to create SPI slave thread");
		/* Wait until after the slave callback is initialised
		 * before starting the tx from master. Slave thread
		 * will start and block on txrx_complete semaphore.
		 * k_sem_take(&slave_ready, K_FOREVER);
		 *
		 * This sleep will allow the slave thread to run.
		 *
		 * Allow time for slave & master threads to exit
		 */
		k_sleep(100);
		k_sem_take(&slave_done, K_FOREVER);

		zassert_not_null(s_rx_buf.buf, "s_rx is NULL..!");
		/* Compare results */
		for (i = 0; i < s_rx_buf.len; i++) {
			if (((u8_t *)s_rx_buf.buf)[i] != (i ^ 0x5A)) {
				TC_ERROR("Slave Failed at rx[%d] = %02x\n", i, ((u8_t *)s_rx_buf.buf)[i]);
				slave_res = -1;
				break;
			}
		}
		if (slave_res) {
			TC_PRINT("Slave Expected: ");
			for (i = 0; i < s_rx_buf.len; i++)
				TC_PRINT("%02x,", i ^ 0x5A);
			TC_PRINT("\nSlave Received: ");
			for (i = 0; i < s_rx_buf.len; i++)
				TC_PRINT("%02x,", ((u8_t *)s_rx_buf.buf)[i]);
		}
		TC_PRINT("\n----------------------------------------------\n");

		/* Free memory allocated in callback */
		if (s_rx_buf.buf) {
			cache_line_aligned_free(s_rx_buf.buf);
			s_rx_buf.buf = NULL;
		}
		if (s_tx_buf.buf) {
			cache_line_aligned_free(s_tx_buf.buf);
			s_tx_buf.buf = NULL;
		}
		/* Abort the SPI slave thread */
		k_thread_abort(sid);

		return slave_res;
	}
}

void print_top_banner(u32_t setup, u32_t role)
{
	TC_PRINT("===============================================================\n");
	if (role) {
		TC_PRINT("Running SPI test as master.\n");
		TC_PRINT("Single Test Command: test spi 1 <tx length> <rx length> master\n");
		TC_PRINT("All Tests Command: test spi 1 all master\n");
		TC_PRINT("Ensure slave rx length is same as master tx length & vice versa!!\n");
		TC_PRINT("Ensure slave is ready and waiting to be clocked!!\n");
	} else {
		TC_PRINT("Running SPI test as slave. Will wait for master to select & clock.\n");
		TC_PRINT("Single Test Command: test spi 1 <tx length> <rx length> slave\n");
		TC_PRINT("All Tests Command: test spi 1 all slave\n");
		TC_PRINT("Ensure slave rx length is same as master tx length & vice versa!!\n");
		TC_PRINT("Ensure invocation of this test precedes invocation of master test!\n");
	}
	TC_PRINT("===============================================================\n");
}

void print_tc_header(u32_t setup, u32_t role, u16_t txlen, u16_t rxlen)
{
	TC_PRINT("\n");
	if (role) {
		if (setup)
			TC_PRINT("2 Boards - Txing %d and Rxing %d as Master.\n", txlen, rxlen);
		else
			TC_PRINT("1 Board - Txing %d and Rxing %d as Master.\n", txlen, rxlen);
	} else {
		if (setup)
			TC_PRINT("2 Boards - Txing %d and Rxing %d as Slave.\n", txlen, rxlen);
		else
			TC_PRINT("1 Board - Txing %d and Rxing %d as Slave.\n", txlen, rxlen);
	}
	TC_PRINT("-------------------------------------------------------\n");
}

SHELL_TEST_DECLARE(test_spi_ind)
{
	u32_t role;
	/* 0: run tests on one board, 1: run tests on two boards */
	u32_t setup = 0;
	/* Slave's tx & rx length */
	u32_t i, j;
	u16_t txrxlens[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 14, 25, 37, 43, 50, 64, 73, 80, 100, 200, 256};
	u32_t numcases = 20;
	u32_t stxlen = INT_DUPLEX_TFR_SIZE, srxlen = INT_DUPLEX_TFR_SIZE;
	u32_t res = 0, errorcount = 0;
	u16_t err_srxlen, err_stxlen;
	/* Setup IO expander straps for SPI2, SPI3 selection */
	setup_spi2_spi3_sel();

	if (argc > 1) {
		setup = atoi(argv[1]);
		/* Two board Set-up */
		if (setup) {
			if(argc > 2) {
				if(strcmp(argv[2], "all") == 0) {
					if (argc > 3) {
						if(strcmp(argv[3], "master") == 0) {
							role = 1;
							print_top_banner(setup, role);
							for (i = 0; i < numcases; i++) {
								for (j = 0; j < numcases; j++) {
									print_tc_header(setup, role, txrxlens[j], txrxlens[i]);
									TC_PRINT("\nIf Slave is ready Hit any key to continue ...\n");
									getch();
									/* Swap txlen and rxlen for master (wrt slave's) */
									res = test_spi_interrupt_mode_2boards(1, txrxlens[j], txrxlens[i]);
									if (res) {
										errorcount++;
										err_stxlen = txrxlens[j];
										err_srxlen = txrxlens[i];
									}
								}
							}
						} else if (strcmp(argv[3], "slave") == 0) {
							role = 0;
							print_top_banner(setup, role);
							for (i = 0; i < numcases; i++) {
								for (j = 0; j < numcases; j++) {
									print_tc_header(setup, role, txrxlens[i], txrxlens[j]);
									res = test_spi_interrupt_mode_2boards(0, txrxlens[i], txrxlens[j]);
									if (res) {
										errorcount++;
										err_stxlen = txrxlens[i];
										err_srxlen = txrxlens[j];
									}
								}
							}
						}
					}
				} else {
					/* 2 Boards Command Example Variant 1:
					 * Param: 0 1 2     3     4
					 * ------------------------------
					 * test spi 1 txlen rxlen master/slave
					 *
					 * Variant 2 (same tx & rx length):
					 * Param: 0 1 2       3
					 * ------------------------------
					 * test spi 1 txrxlen master/slave
					 */
					stxlen = atoi(argv[2]);
					srxlen = atoi(argv[2]);
					if(argc > 4) {
						srxlen = atoi(argv[3]);
						if(strcmp(argv[4], "master") == 0)
							role = 1;
						else if (strcmp(argv[4], "slave") == 0)
							role = 0;
					} else if(argc > 3) {
						if(strcmp(argv[3], "master") == 0)
							role = 1;
						else if (strcmp(argv[3], "slave") == 0)
							role = 0;
					}
					print_top_banner(setup, role);
					if(role) {
						print_tc_header(setup, role, stxlen, srxlen);
						res = test_spi_interrupt_mode_2boards(1, stxlen, srxlen);
					} else {
						print_tc_header(setup, role, stxlen, srxlen);
						res = test_spi_interrupt_mode_2boards(0, stxlen, srxlen);
					}
					if (res) {
						errorcount++;
						err_stxlen = stxlen;
						err_srxlen = srxlen;
					}
				}
			}
		} else { /* one board Set-up */
			if(argc > 2) {
				/* Command Example Variant 1 (run all tests):
				 * Param: 0 1 2
				 * ---------------
				 * test spi 0 all
				 *
				 * Variant 2 (run individual tests):
				 * when parameter 3 not provided tx & rx
				 * lengths are same.
				 * Param: 0 1 2     <3>
				 * ------------------------
				 * test spi 0 txlen <rxlen>
				 */
				if(strcmp(argv[2], "all") == 0) {
					for (i = 0; i < numcases; i++) {
						for (j = 0; j < numcases; j++) {
							print_tc_header(setup, 0, txrxlens[i], txrxlens[j]);
							print_tc_header(setup, 1, txrxlens[j], txrxlens[i]);
							res = test_spi_interrupt_mode_1board(txrxlens[i], txrxlens[j]);
							if (res) {
								errorcount++;
								err_stxlen = txrxlens[i];
								err_srxlen = txrxlens[j];
							}
						}
					}
				} else {
					stxlen = atoi(argv[2]);
					srxlen = atoi(argv[2]);
					if(argc > 3)
						srxlen = atoi(argv[3]);
					print_tc_header(setup, 0, stxlen, srxlen);
					print_tc_header(setup, 1, srxlen, stxlen);
					res = test_spi_interrupt_mode_1board(stxlen, srxlen);
					if (res) {
						errorcount++;
						err_stxlen = stxlen;
						err_srxlen = srxlen;
					}
				}
			}
		}
	}
	if (errorcount) {
		TC_PRINT("SPI Test Failed %d times!!\n", errorcount);
		TC_PRINT("Last failure happened with stxlen: %d srxlen: %d!!\n", err_stxlen, err_srxlen);
	} else
		TC_PRINT("SPI Test passed\n\n");
	return TC_PASS;
}

#endif /* CONFIG_SPI_LEGACY_API */
