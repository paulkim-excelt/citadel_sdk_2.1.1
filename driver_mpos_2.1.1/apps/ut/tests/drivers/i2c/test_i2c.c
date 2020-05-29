/*
 * Copyright (c) 2018 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <broadcom/i2c.h>
#include <zephyr.h>
#include <test.h>
#include <board.h>

#define MAX_I2C_ADDR		0x80
#define I2C_EEPROM_ADDR		0x53
/* Don't use the full EEPROM size, as the tests on that will take too long */
#define EEPROM_SIZE_KB		1
#define EEPROM_PATTERN		0xb0

static union dev_config i2c_master_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

static int test_one_bus(struct device *dev)
{
	u8_t val;
	int i, rc;

	/* 1. Verify i2c_configure() */
	if (i2c_configure(dev, i2c_master_cfg.raw)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* Find any i2c devices present */
	for (i = 0; i < MAX_I2C_ADDR; i++) {
		rc = i2c_burst_read(dev, i, 0, &val, 1);
		if (!rc)
			TC_PRINT("I2C addr %x has device\n", i);
	}

	return TC_PASS;
}

#ifndef CONFIG_BOARD_SERP_CP
#define EEPROM_BUF_SIZE	128
static int test_i2c_eeprom(struct device *dev)
{
	int rc = 0, i;
	u8_t addr[2];
	u8_t buffer[EEPROM_BUF_SIZE + 2];

	/* Init data to write */
	buffer[0] = 0; buffer[1] = 0;
	for (i = 0; i < EEPROM_BUF_SIZE; i++)
		buffer[2 + i] = i;

	/* Write the data */
	rc = i2c_write(dev, buffer, sizeof(buffer), I2C_EEPROM_ADDR);
	if (rc) {
		TC_PRINT("I2C EEPROM write error %d\n", rc);
		return TC_FAIL;
	}

	/* We could poll the EEPROM for write complete or just add a delay */
	k_sleep(100);

	/* Reset buffer data */
	memset(buffer, 0, sizeof(buffer));

	/* Read back and verify */
	addr[0] = addr[1] = 0;
	i2c_write(dev, addr, 2, I2C_EEPROM_ADDR);
	i2c_read(dev, buffer, EEPROM_BUF_SIZE, I2C_EEPROM_ADDR);

	for (i = 0; i < EEPROM_BUF_SIZE; i++)
		zassert_true(buffer[i] == i, "Verify data failed");

	return TC_PASS;
}
#endif

static int test_bus_walking(void)
{
	struct device *dev;
	int rc;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	if (!dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	rc = test_one_bus(dev);
	if (rc == TC_FAIL)
		return rc;

#ifndef CONFIG_BOARD_SERP_CP
	/* EEPROM only present on I2C0 */
	rc = test_i2c_eeprom(dev);
	if (rc == TC_FAIL)
		return rc;
#endif

#ifdef BROKEN
	dev = device_get_binding(CONFIG_I2C1_NAME);
	if (!dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	rc = test_one_bus(dev);
	if (rc == TC_FAIL)
		return rc;

	dev = device_get_binding(CONFIG_I2C2_NAME);
	if (!dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	rc = test_one_bus(dev);
	if (rc == TC_FAIL)
		return rc;
#endif

	return TC_PASS;
}

static void test_i2c_bus_walking(void)
{
	zassert_true(test_bus_walking() == TC_PASS, NULL);
}

#if UNTESTED
static union dev_config i2c_slave_cfg = {
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 0,
		.speed = I2C_SPEED_STANDARD,
	},
};

static void i2c_slave_rd_handler(void *data)
{
	struct device *dev = data;
	struct i2c_msg msg;
	u8_t buffer[] = "deadbeeff00d";

	msg.buf = buffer;
	msg.len = sizeof(buffer);

	i2c_slave_read_handler(dev, &msg);

	/* No need to free msg here because it is on the stack and not malloced
	 */
}

#define MAX_WR_LEN	25

static void i2c_slave_wr_handler(void *data, struct i2c_msg *msg)
{
	u32_t i;

	ARG_UNUSED(data);

	TC_PRINT("Slave data received\n");
	for (i = 0; i < msg->len; i++)
		TC_PRINT("%x  ", msg->buf[i]);
	TC_PRINT("\n");

	k_free(msg->buf);
	k_free(msg);
}

struct i2c_handlers slave_handlers = {
	.rd_handler = i2c_slave_rd_handler,
	.wr_handler = i2c_slave_wr_handler,
};

static int i2c_slave_test(void)
{
	struct device *dev;
	int rc;

	dev = device_get_binding(CONFIG_I2C0_NAME);
	if (!dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	/* configure i2c device as slave and set speed */
	if (i2c_configure(dev, i2c_slave_cfg.raw)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* register handlers */
	i2c_register_handlers(dev, &slave_handlers, dev);

	/* set slave address */
	rc = i2c_slave_set_address(dev, 0, 0xf0);
	if (rc)
		return TC_FAIL;

	/* poll for events or something */
	do {
		i2c_slave_poll(dev);
	} while (1);

	return TC_PASS;
}

#else

static int i2c_slave_test(void)
{
	return TC_PASS;
}
#endif

static void test_i2c_slave(void)
{
	zassert_true(i2c_slave_test() == TC_PASS, NULL);
}

SHELL_TEST_DECLARE(test_i2c)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* FIXME - create a second series of tests which verify the EEPROM works
	 * at 400
	 */

	ztest_test_suite(i2c_test,
			 ztest_unit_test(test_i2c_slave),
			 ztest_unit_test(test_i2c_bus_walking));
	ztest_run_test_suite(i2c_test);

	return 0;
}
