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
 * @file dma_test.c
 * @brief  pl080 dma driver test
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/dma.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <cortex_a/cache.h>
#include <zephyr/types.h>
#include <test.h>

#define RX_BUFF_SIZE (64)
#define DMA_MAX_RETRY   1000
static const char tx_data[6][60] __aligned(4) = {
	"It is harder to be kind than to be wise",
	"Testing LLI Mode ptr 1 Testing LLI Mode ptr 1 *",
	"Testing LLI Mode ptr 2 Testing LLI Mode ptr 2 **",
	"Testing LLI Mode ptr 3 Testing LLI Mode ptr 3 ***",
	"Testing LLI Mode ptr 4 Testing LLI Mode ptr 4 ****",
	"Testing LLI Mode ptr 5 Testing LLI Mode ptr 5 *****",
};

static char rx_data0[RX_BUFF_SIZE] __aligned(64) = { 0 };
static char rx_data1[RX_BUFF_SIZE] __aligned(64) = { 0 };
static char rx_data2[RX_BUFF_SIZE] __aligned(64) = { 0 };
static char rx_data3[RX_BUFF_SIZE] __aligned(64) = { 0 };
static char rx_data4[RX_BUFF_SIZE] __aligned(64) = { 0 };
static char rx_data5[RX_BUFF_SIZE] __aligned(64) = { 0 };

static void test_done(struct device *dev, u32_t id, s32_t error_code)
{
	ARG_UNUSED(dev);

	TC_PRINT(" DMA status - channel: %d err: %d\n", id, error_code);
	if (error_code == 0) {
		TC_PRINT("DMA transfer done\n");
	} else {
		TC_ERROR("DMA transfer met an error\n");
	}
}

static int test_task(u32_t chan_id, u32_t blen, bool cb)
{
	struct dma_block_config dma_block_cfg0 = {0};
	struct dma_block_config dma_block_cfg1 = {0};
	struct dma_block_config dma_block_cfg2 = {0};
	struct dma_block_config dma_block_cfg3 = {0};
	struct dma_block_config dma_block_cfg4 = {0};
	struct dma_block_config dma_block_cfg5 = {0};
	struct dma_config dma_cfg = {0};
	struct device *dma;
	u32_t status, cnt;

	dma = device_get_binding(CONFIG_DMA_DEV_NAME);
	if (!dma) {
		TC_ERROR("Cannot get dma controller\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_peripheral = 0;
	dma_cfg.dest_peripheral = 0;
	dma_cfg.source_data_size = 4;
	dma_cfg.dest_data_size = 4;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	if (cb) {
		dma_cfg.complete_callback_en = 1;
		dma_cfg.error_callback_en = 1;
	} else {
		dma_cfg.complete_callback_en = 0;
		dma_cfg.error_callback_en = 0;
	}
	dma_cfg.block_count = 6;
	dma_cfg.head_block = &dma_block_cfg0;
	dma_cfg.endian_mode = 0x00;
	TC_PRINT("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
		 chan_id, blen >> 3);

	TC_PRINT("Starting the transfer\n");
	memset(rx_data0, 0, sizeof(rx_data0));
	dma_block_cfg0.block_size = strlen(&tx_data[0][0]);
	dma_block_cfg0.source_address = (u32_t)&tx_data[0][0];
	dma_block_cfg0.dest_address = (u32_t)rx_data0;
	dma_block_cfg0.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg0.dest_addr_adj = ADDRESS_INCREMENT;

	dma_block_cfg0.next_block = &dma_block_cfg1;
	memset(rx_data1, 0, sizeof(rx_data1));
	dma_block_cfg1.block_size = strlen(&tx_data[1][0]);
	dma_block_cfg1.source_address = (u32_t)&tx_data[1][0];
	dma_block_cfg1.dest_address = (u32_t)rx_data1;
	dma_block_cfg1.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg1.dest_addr_adj = ADDRESS_INCREMENT;

	dma_block_cfg1.next_block = &dma_block_cfg2;
	memset(rx_data2, 0, sizeof(rx_data2));
	dma_block_cfg2.block_size = strlen(&tx_data[2][0]);
	dma_block_cfg2.source_address = (u32_t)&tx_data[2][0];
	dma_block_cfg2.dest_address = (u32_t)rx_data2;
	dma_block_cfg2.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg2.dest_addr_adj = ADDRESS_INCREMENT;

	dma_block_cfg2.next_block = &dma_block_cfg3;
	memset(rx_data3, 0, sizeof(rx_data3));
	dma_block_cfg3.block_size = strlen(&tx_data[3][0]);
	dma_block_cfg3.source_address = (u32_t)&tx_data[3][0];
	dma_block_cfg3.dest_address = (u32_t)rx_data3;
	dma_block_cfg3.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg3.dest_addr_adj = ADDRESS_INCREMENT;

	dma_block_cfg3.next_block = &dma_block_cfg4;
	memset(rx_data4, 0, sizeof(rx_data4));
	dma_block_cfg4.block_size = strlen(&tx_data[4][0]);
	dma_block_cfg4.source_address = (u32_t)&tx_data[4][0];
	dma_block_cfg4.dest_address = (u32_t)rx_data4;
	dma_block_cfg4.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg4.dest_addr_adj = ADDRESS_INCREMENT;

	dma_block_cfg4.next_block = &dma_block_cfg5;
	memset(rx_data5, 0, sizeof(rx_data5));
	dma_block_cfg5.block_size = strlen(&tx_data[5][0]);
	dma_block_cfg5.source_address = (u32_t)&tx_data[5][0];
	dma_block_cfg5.dest_address = (u32_t)rx_data5;
	dma_block_cfg5.source_addr_adj = ADDRESS_INCREMENT;
	dma_block_cfg5.dest_addr_adj = ADDRESS_INCREMENT;

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_ERROR("Config\n");
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_ERROR("Transfer\n");
		return TC_FAIL;
	}

	TC_PRINT("DMA transfer started\n");
	for (cnt = 0; cnt < DMA_MAX_RETRY; cnt++) {
		if (dma_status(dma, chan_id, &status)) {
			TC_ERROR("Get channel status\n");
			return TC_FAIL;
		}
		if (status == 0)
			break;
	}
	if (cnt == DMA_MAX_RETRY) {
		TC_ERROR("Transfer long time\n");
		return TC_FAIL;
	}

	TC_PRINT("DATA0: %s\n", rx_data0);
	TC_PRINT("DATA1: %s\n", rx_data1);
	TC_PRINT("DATA2: %s\n", rx_data2);
	TC_PRINT("DATA3: %s\n", rx_data3);
	TC_PRINT("DATA4: %s\n", rx_data4);
	TC_PRINT("DATA5: %s\n", rx_data5);
	TC_PRINT("DMA transfer done\n");

	if (strcmp(&tx_data[0][0], rx_data0) != 0)
		return TC_FAIL;
	if (strcmp(&tx_data[1][0], rx_data1) != 0)
		return TC_FAIL;
	if (strcmp(&tx_data[2][0], rx_data2) != 0)
		return TC_FAIL;
	if (strcmp(&tx_data[3][0], rx_data3) != 0)
		return TC_FAIL;
	if (strcmp(&tx_data[4][0], rx_data4) != 0)
		return TC_FAIL;
	if (strcmp(&tx_data[5][0], rx_data5) != 0)
		return TC_FAIL;

	TC_PRINT("Test passed\n\n");
	return TC_PASS;
}

static void dma_test_battery(void)
{
	unsigned int ch, chan, blen;
	struct device *dma;
	u32_t start_ch;

	dma = device_get_binding(CONFIG_DMA_DEV_NAME);
	zassert_not_null(dma, "Cannot get dma controller\n");

	/* Dynamic Channel test */
	zassert_false(dma_request(dma, &chan, NORMAL_CHANNEL),
		      "Cannot get Channel\n");
	zassert_true((test_task(chan, 8, 1) == TC_PASS), NULL);
	/* Try again, we should get the same channel or there is a problem */
	zassert_false(dma_request(dma, &ch, NORMAL_CHANNEL),
		      "Cannot get Channel\n");
	zassert_true(ch == chan, "Did not get the channel expected\n");
	/* Use it to free it up again */
	zassert_true((test_task(ch, 8, 1) == TC_PASS), NULL);

	/* Static channel test */
	TC_PRINT("Static channel test\n");
	/* Get the first free channel after allocated channels */
	zassert_false(dma_request(dma, &start_ch, ALLOCATED_CHANNEL),
			      "Cannot get Channel\n");
	zassert_false(dma_release(dma, start_ch), "Cannot release Channel\n");

	for (ch = start_ch; ch <= 7; ch++) {
		zassert_false(dma_request(dma, &chan, ALLOCATED_CHANNEL),
			      "Cannot get Channel\n");

		zassert_false(ch != chan, "Did not get the channel expected\n");

		for (blen = 8; blen <= 256; blen *= 2)
			zassert_true((test_task(ch, blen, 0) == TC_PASS), NULL);

		/* Normally, we would release the DMA channel.  However, we are
		 * NOT doing to do this to guarantee we get the next sequential
		 * channel.
		 */
	}

	/* The testing is over, free them now so that they can be used again */
	for (ch = start_ch; ch <= 7; ch++)
		zassert_false(dma_release(dma, ch), "Cannot release Channel\n");
}

SHELL_TEST_DECLARE(test_dma)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(dma_driver_tests,
			 ztest_unit_test(dma_test_battery));

	ztest_run_test_suite(dma_driver_tests);

	return 0;
}
