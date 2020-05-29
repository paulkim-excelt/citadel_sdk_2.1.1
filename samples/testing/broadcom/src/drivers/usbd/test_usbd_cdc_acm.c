/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for CDC ACM class driver
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <stdio.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include <stdio.h>
#include <test.h>
#include <clock_a7.h>

static const char *banner1 = "USB Serial Device (Citadel SVK)\r\n";
static const char *banner2 = "===============================\r\n";

static volatile bool data_transmitted;
static volatile bool data_arrived;
static	char data_buf[64];

static void interrupt_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (uart_irq_rx_ready(dev)) {
		data_arrived = true;
	}
}

static void write_data(struct device *dev, const char *buf, int len)
{
	int sent;

	uart_irq_tx_enable(dev);

	while (len) {
		data_transmitted = false;
		sent = uart_fifo_fill(dev, (const u8_t *)buf, len);

		if (!sent) {
			printf("Unable to send Data !\n");
			break;
		}

		/* Wait until Tx interrupr is generated*/
		while (data_transmitted == false)
			;
		/* Update remainging Data length to transfer*/
		len -= sent;
		buf += sent;
	}

	uart_irq_tx_disable(dev);
}

static void read_and_echo_data(struct device *dev, int *bytes_read)
{
	u32_t timeout = 10; /* 10 milli seconds */

	do {
		if (data_arrived)
			break;
		k_sleep(1);
	} while (--timeout);

	data_arrived = false;

	/* Read all data and echo it to console */
	while ((*bytes_read = uart_fifo_read(dev,
	    (u8_t *)data_buf, sizeof(data_buf) - 1))) {
		/* Write back to USB console for data echo */
		write_data(dev, data_buf, *bytes_read);

		/* Log the same to serial console */
		data_buf[*bytes_read] = 0x0;
		printk("%s", data_buf);
	}
}

SHELL_TEST_DECLARE(test_usbd_cdc_acm)
{
	struct device *dev;
	u32_t baudrate, bytes_read, time;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Set CPU frequency to 1 GHz */
	clk_a7_set(MHZ(1000));

	dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
	zassert_not_null(dev, "Device binding not found for CDC ACM port");

	TC_PRINT("=========================================================\n"
		 "Connect the USB device port on SVK to a PC\n"
		 "Open a hyper terminal app (terterm)\n"
		 "Choose the CDC-ACM com port (USB Serial Device)\n"
		 "Set baud rate to 115200 and turn local echo off\n"
		 "=========================================================\n");

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, LINE_CTRL_DCD, 1);
	if (ret)
		printf("Failed to set DCD, ret code %d\n", ret);

	ret = uart_line_ctrl_set(dev, LINE_CTRL_DSR, 1);
	if (ret)
		printf("Failed to set DSR, ret code %d\n", ret);

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	ret = uart_line_ctrl_get(dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret)
		printf("Failed to get baudrate, ret code %d\n", ret);
	else
		printf("Baudrate detected: %d\n", baudrate);

	uart_irq_callback_set(dev, interrupt_handler);
	write_data(dev, banner1, strlen(banner1));
	write_data(dev, banner2, strlen(banner2));

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	TC_PRINT("Type something on the CDC-ACM console and see it\n"
		 "echo here and echo back on the CDC-ACM console\n"
		 "=========================================================\n");

	/* Echo the received data for 10 seconds */
	time = k_uptime_get_32();
	do {
		read_and_echo_data(dev, (int *) &bytes_read);
	} while ((k_uptime_get_32() - time) < 10000);

	TC_PRINT("USBD test complete\n");
	return 0;
}
