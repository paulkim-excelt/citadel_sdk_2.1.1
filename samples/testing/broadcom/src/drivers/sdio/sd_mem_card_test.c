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

/* @file sd_mem_card_test.c
 *
 * SD memory card test module
 *
 * This file implements a basic version of the Multi-media card driver that is
 * meant to only demonstrate the usage of SDIO driver api.
 */


#include <ztest.h>
#include <errno.h>
#include <board.h>
#include <string.h>
#include <sdio.h>
#include <stdbool.h>
#include <misc/util.h>
#include "sd_mem_card_test.h"

/* Recommended check pattern for CMD8 */
#define CHECK_PATTERN	0xAA

/* Since the citadel SDIO host controller does not use DMA to read/write data
 * there appears to be a 512 byte limit on the max data that can be read or
 * written with a rea/write block command
 */
#define HOST_CONTROLLER_MAX_BLOCK_LEN 512

/* SD card information */
static u32_t csd[4], cid[4], card_addr, rd_blk_len, wr_blk_len, ocr, max_blks;
static u64_t tran_speed, capacity;
static bool data_4bit = false, hs_supported = false, hc_supported = false;

static int reset_card(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	k_busy_wait(1000);
	cmd.cmd_idx = SD_CMD0_GO_IDLE_STATE;
	cmd.cmd_arg = 0;
	cmd.resp_type = SD_RSP_NONE;

	ret = sd_send_command(dev, &cmd, NULL);

	if (ret)
		return ret;

	return 0;
}

static int send_if_cond(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	/* Send SEND IF COND command */
	cmd.cmd_idx = SD_CMD8_SEND_IF_COND;

	/* Set VHS - 0x1 (2.7 to 3.6 V) */
	cmd.cmd_arg = (0x1 << 8) | CHECK_PATTERN;
	cmd.resp_type = SD_RSP_R7;

	ret = sd_send_command(dev, &cmd, NULL);

	if (ret)
		return ret;

	if ((cmd.response[0] & 0xff) != CHECK_PATTERN) {
		TC_ERROR("Failed to verify check pattern in CMD8 response\n");
		return -EIO;
	} else {
		TC_PRINT("SD version: 2.0\n");
	}

	return 0;
}

static int send_op_cond(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;
	int timeout = 1000;

	/* Get OCR till card power up status bit is set */
	do {
		cmd.cmd_idx = SD_CMD55_APP_CMD;
		cmd.resp_type = SD_RSP_R1;
		cmd.cmd_arg = 0; /* RCA = 0x0 */

		ret = sd_send_command(dev, &cmd, NULL);

		if (ret) {
			TC_ERROR("Error sending APP command: %d\n", ret);
			return ret;
		}

		cmd.cmd_idx = SD_ACMD41_SD_APP_OP_COND;
		cmd.resp_type = SD_RSP_R3;

		/* All supported voltages */
		cmd.cmd_arg = SD_OCR_3_5_TO_3_6V |
			      SD_OCR_3_4_TO_3_5V |
			      SD_OCR_3_3_TO_3_4V |
			      SD_OCR_3_2_TO_3_3V |
			      SD_OCR_3_1_TO_3_2V |
			      SD_OCR_3_0_TO_3_1V |
			      SD_OCR_2_9_TO_3_0V |
			      SD_OCR_2_8_TO_2_7V |
			      SD_OCR_2_7_TO_2_8V;

		/* Host supports high cacacity SD cards */
		cmd.cmd_arg |= SD_OCR_CARD_CAPACITY_STATUS;

		ret = sd_send_command(dev, &cmd, NULL);

		if (ret) {
			TC_ERROR("Error sending send_op_cond() cmd: %d\n", ret);
			return ret;
		}

		/* Card has powered up */
		if (cmd.response[0] & SD_OCR_CARD_POWER_UP_STATUS)
			break;
		k_sleep(1);
	} while (--timeout);

	if (timeout == 0) {
		TC_ERROR("Timedout waiting for OCR busy bit to clear\n");
		return -ETIMEDOUT;
	} else {
		TC_PRINT("SD Card powered up!\n");
	}

	ocr = cmd.response[0];
	if (ocr & SD_OCR_CARD_CAPACITY_STATUS) {
		hc_supported = true;
		TC_PRINT("High capacity SD card (HC) detected\n");
	} else {
		hc_supported = false;
		TC_PRINT("Standard capacity SD card detected\n");
	}

	return 0;
}

static int get_cid(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	cmd.cmd_idx = SD_CMD2_ALL_SEND_CID;
	cmd.resp_type = SD_RSP_R2;
	cmd.cmd_arg = 0;

	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Error sending CMD2: %d\n", ret);
		return ret;
	}

	TC_PRINT("MFG ID: 0x%02x\n", cmd.response[0] >> 24);
	TC_PRINT("OEM ID: 0x%02x\n", (cmd.response[0] >> 8) & 0xFFFF);
	TC_PRINT("NAME  : %c%c%c%c%c\n", cmd.response[0] & 0xFF,
					 (cmd.response[1] >> 24) & 0xFF,
					 (cmd.response[1] >> 16) & 0xFF,
					 (cmd.response[1] >> 8) & 0xFF,
					 cmd.response[1] & 0xFF);
	TC_PRINT("REV NO: 0x%02x\n", (cmd.response[2] >> 24) & 0xFF);
	TC_PRINT("SER NO: 0x%08x\n", (cmd.response[2] << 8) |
				     (cmd.response[3] >> 24));

	memcpy(cid, cmd.response, sizeof(cid));
	return 0;
}

static int get_card_addr(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	cmd.cmd_idx = SD_CMD3_SEND_RELATIVE_ADDR;
	cmd.cmd_arg = 0;
	cmd.resp_type = SD_RSP_R6;

	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Failed to send CMD3: %d\n", ret);
		return ret;
	}

	card_addr = (cmd.response[0] >> 16) & 0xFFFF;

	return 0;
}

static int get_csd(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	cmd.cmd_idx = SD_CMD9_SEND_CSD;
	cmd.resp_type = SD_RSP_R2;
	cmd.cmd_arg = card_addr << 16;

	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Failed to send CMD9: %d\n", ret);
		return ret;
	}

	memcpy(csd, cmd.response, sizeof(csd));
	return 0;
}

static int select_card(struct device *dev)
{
	int ret;
	struct sd_cmd cmd;

	cmd.cmd_idx = SD_CMD7_SELECT_DESELECT_CARD;
	cmd.resp_type = SD_RSP_R1b;
	cmd.cmd_arg = card_addr << 16;

	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Error sending CMD7: %d\n", ret);
		return ret;
	}

	return 0;
}

static int get_scr(struct device *dev)
{
	int ret, timeout = 3;
	struct sd_cmd cmd;
	struct sd_data data;
	u8_t scr[64] __attribute__((__aligned__(64)));

	cmd.cmd_idx = SD_CMD55_APP_CMD;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = card_addr << 16;

	/* Send CMD55 to tell teh card the next command is an app spec command*/
	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Error sending APP CMD: %d\n", ret);
		return ret;
	}

	cmd.cmd_idx = SD_ACMD51_SEND_SCR;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = 0;

	data.buf = scr;
	data.num_blocks = 1;
	data.block_size = 8;
	data.flags = SD_DATA_READ;
	do {
		ret = sd_send_command(dev, &cmd, &data);
		if (ret == 0) {
			if (scr[1] & BIT(2))
				data_4bit = true;
			return 0;
		}
	} while (--timeout);

	return -ETIMEDOUT;
}

static int check_n_switch_hs(struct device *dev)
{
	int ret, timeout = 4;
	u8_t buf[64] __attribute__((__aligned__(64)));
	struct sd_cmd cmd;
	struct sd_data data;

	/* Send CMD6 Check mode */
	cmd.cmd_idx = SD_CMD6_SWITCH_FUNC;
	cmd.resp_type = SD_RSP_R1;
	/* Check HS with BIT(1) of Access mode group */
	cmd.cmd_arg = SD_CMD6_MODE_CHECK | 0xfffff0 | BIT(1);

	data.buf = buf;
	data.block_size = 64;
	data.num_blocks = 1;
	data.flags = SD_DATA_READ;

	do {
		ret = sd_send_command(dev, &cmd, &data);
		if (ret) {
			TC_ERROR("Timedout checking HS with CMD6: %d\n", ret);
			return ret;
		}
		/* Check busy status for group 1 BIT[287:272], function 1 (HS)*/
		if ((buf[29] & BIT(1)) == 0)
			break;
	} while (--timeout);

	if (timeout == 0) {
		TC_ERROR("Timed out checking HS with CMD6: %d\n", ret);
		return -ETIMEDOUT;
	}

	/* Check if HS function is supported, group 1 BIT[415:400] */
	if (buf[13] & BIT(1))
		hs_supported = true;
	else
		return 0;

	cmd.cmd_idx = SD_CMD6_SWITCH_FUNC;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = SD_CMD6_MODE_SWITCH | 0xfffff0 | BIT(1);

	data.buf = buf;
	data.block_size = 64;
	data.num_blocks = 1;
	data.flags = SD_DATA_READ;

	ret = sd_send_command(dev, &cmd, &data);
	if (ret) {
		TC_ERROR("Timed out switching to HS with CMD6: %d\n", ret);
		return ret;
	}

	return 0;
}

static int read_ssr(struct device *dev)
{
	int ret;
	u8_t ssr[64] __attribute__((__aligned__(64)));
	struct sd_cmd cmd;
	struct sd_data data;
	int timeout = 3;

	cmd.cmd_idx = SD_CMD55_APP_CMD;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = card_addr << 16;

	ret = sd_send_command(dev, &cmd, NULL);
	if (ret) {
		TC_ERROR("Error sending APP CMD: %d\n", ret);
		return ret;
	}

	cmd.cmd_idx = SD_ACMD13_SD_STATUS;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = 0;

	data.buf= ssr;
	data.block_size = 64;
	data.num_blocks = 1;
	data.flags = SD_DATA_READ;

	do {
		ret = sd_send_command(dev, &cmd, &data);
		if (ret == 0)
			break;
	} while (--timeout);

	if (timeout == 0)
		return -ETIMEDOUT;

	if ((ssr[0] >> 6) == 2)
		TC_PRINT("4-Bit mode configured\n");
	else if ((ssr[0] >> 6) == 0)
		TC_PRINT("1-Bit mode configured\n");
	else {
		TC_PRINT("ssr[0] = %x\n", ssr[0]);
		TC_ERROR("SD Card status indicates bus width not configured\n");
		return -EIO;
	}

	if (ssr[9])
		TC_PRINT("Performance Speed: %d MB/s\n", ssr[9]);

	return 0;
}

static int set_block_length(struct device *dev, u32_t blk_len)
{
	struct sd_cmd cmd;

	cmd.cmd_idx = SD_CMD16_SET_BLOCKLEN;
	cmd.resp_type = SD_RSP_R1;
	cmd.cmd_arg = blk_len;

	return sd_send_command(dev, &cmd, NULL);
}

static void card_detect_cb(bool inserted)
{
	TC_PRINT("SD Card %s\n", inserted ? "Insterted" : "Removed");
}

int sd_mem_card_test_init(void)
{
	int ret;
	struct device *dev = device_get_binding(CONFIG_SDIO_DEV_NAME);
	u8_t tran_speed_vals_10x[] =
		{0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
	u32_t tran_speed_units[] = {KHZ(100), MHZ(1), MHZ(10), MHZ(100)};
	static bool initialized = false;

	if (initialized)
		return 0;

	if (dev == NULL) {
		TC_ERROR("Failed to get device binding\n");
		return -EINVAL;
	}

	/* First reset card */
	ret = reset_card(dev);
	if (ret) {
		TC_ERROR("reset_card() failed: %d\n", ret);
		return ret;
	}

	/* Send CMD8 to verify the card insterted is a memory card */
	ret = send_if_cond(dev);
	if (ret) {
		TC_ERROR("send_if_cond() failed: %d\n", ret);
		TC_ERROR("NO SD memory card found\n");
		return ret;
	}

	/* Get OCR from the card */
	ret = send_op_cond(dev);
	if (ret) {
		TC_ERROR("send_op_cond() failed: %d\n", ret);
		return ret;
	}

	/* Send Card identify command */
	ret = get_cid(dev);
	if (ret) {
		TC_ERROR("send_cid() failed: %d\n", ret);
		return ret;
	}

	/* Get Relative card address */
	ret = get_card_addr(dev);
	if (ret) {
		TC_ERROR("Error getting relative card address: %d\n", ret);
		return ret;
	}

	/* Get Card Specific Data */
	ret = get_csd(dev);
	if (ret) {
		TC_ERROR("Error getting card specific data: %d\n", ret);
		return ret;
	}

	/* Get Transfer Speed */
	tran_speed = (tran_speed_vals_10x[((csd[0] >> 3) & 0xf)] *
		     tran_speed_units[csd[0] & 0x7]) / 10;
	TC_PRINT("Transfer Speed = %d Kbps\n", (u32_t)(tran_speed/1000));

	wr_blk_len = rd_blk_len = BIT((csd[1] >> 16) & 0xF);

	if ((csd[0] >> 30) == 0) {
		/* CSD Version 1.0 */
		capacity = ((((csd[1] & 0x3FF) << 2) | (csd[2] >> 30)) + 1) *
			   BIT(2 + ((csd[2] >> 15) & 0x7)); /* C_SIZE_MULT */
		capacity *= rd_blk_len;
	} else {
		/* CSD Version 2.0 */
		capacity = ((csd[2] >> 16) + 1) */* CSIZE: ignore 6 bits (HC) */
			   KB(512); /* C_SIZE_MULT */
	}

	if (rd_blk_len > HOST_CONTROLLER_MAX_BLOCK_LEN)
		rd_blk_len = HOST_CONTROLLER_MAX_BLOCK_LEN;
	if (wr_blk_len > HOST_CONTROLLER_MAX_BLOCK_LEN)
		wr_blk_len = HOST_CONTROLLER_MAX_BLOCK_LEN;

	/* Compute max blocks */
	max_blks = capacity / rd_blk_len;
	TC_PRINT("Memory capacity = %d MB\n", (u32_t)(capacity >> 20));
	TC_PRINT("Maximum Blocks  = %d\n", max_blks);

	/* Select the card, and put it into Transfer Mode */
	ret = select_card(dev);
	if (ret) {
		TC_ERROR("Error Selecting card for transfer: %d\n", ret);
		return ret;
	}

	/* Get card data bus widths supported */
	ret = get_scr(dev);
	if (ret) {
		TC_ERROR("Error getting card's SCR: %d\n", ret);
		return ret;
	}
	TC_PRINT("Bus width: %d-bit\n", data_4bit ? 4 : 1);

	/* Check and switch to high speed mode (if supported) */
	ret = check_n_switch_hs(dev);
	if (ret) {
		TC_ERROR("Failed to check/switch to HS mode: %d\n", ret);
		return ret;
	}
	TC_PRINT("HS Support: %s\n", hs_supported ? "yes" : "no");

	/* Switch to 4-bit mode if supported */
	if (data_4bit) {
		struct sd_cmd cmd;

		cmd.cmd_idx = SD_CMD55_APP_CMD;
		cmd.resp_type = SD_RSP_R1;
		cmd.cmd_arg = card_addr << 16;

		ret = sd_send_command(dev, &cmd, NULL);
		if (ret) {
			TC_ERROR("Error sending APP CMD: %d\n", ret);
			return ret;
		}

		cmd.cmd_idx = SD_ACMD6_SET_BUS_WIDTH;
		cmd.resp_type = SD_RSP_R1;
		cmd.cmd_arg = BIT(1); /* 4-bit */
		ret = sd_send_command(dev, &cmd, NULL);
		if (ret) {
			TC_ERROR("Error sending SET_BUS_WIDTH: %d\n", ret);
			return ret;
		}

		ret = sd_set_bus_width(dev, 4);
		if (ret) {
			TC_ERROR("Error configuring host controller bus width"
				 ": %d\n", ret);
		}
	}

	/* Set the clock to transfer speed reported by the card */
	sd_set_clock_speed(dev, tran_speed);

	/* Read the SSR to verify the card is responsive after the
	 * updated bus width and clock speed
	 */
	ret = read_ssr(dev);
	if (ret) {
		TC_ERROR("Error reading SSR: %d\n", ret);
		return ret;
	}

	/* Set block length */
	ret = set_block_length(dev, rd_blk_len);
	if (ret) {
		TC_ERROR("Error setting blk len to %d: %d\n", rd_blk_len, ret);
		return ret;
	}

	sd_register_card_detect_cb(dev, card_detect_cb);

	initialized = true;
	return 0;
}

/* Function to read SD memory card blocks
 * Returns number of blocks read
 */
int sd_mem_card_read_blocks(u32_t start, u32_t num_blocks, u8_t *buf)
{
	u32_t i;
	int ret = 0;
	struct sd_cmd cmd;
	struct sd_data data;
	struct device *dev;

	dev = device_get_binding(CONFIG_SDIO_DEV_NAME);
	if (dev == NULL)
		return -EPERM;

	if ((start + num_blocks) > max_blks) {
		TC_ERROR("Max Blocks = %d : Read request [%d -> %d]\n",
			 max_blks, start, start + num_blocks);
		return -EINVAL;
	}

	for (i = 0; i < num_blocks; i++) {
		cmd.cmd_idx = SD_CMD17_READ_SINGLE_BLOCK;

		if (hc_supported)
			cmd.cmd_arg = start + i;
		else
			cmd.cmd_arg = (start + i) * rd_blk_len;

		cmd.resp_type = (SD_RSP_PRESENT | SD_RSP_CMDIDX);

		data.buf = buf + i*rd_blk_len;
		data.num_blocks = 1;
		data.block_size = rd_blk_len;
		data.flags = SD_DATA_READ;

		if (sd_send_command(dev, &cmd, &data)) {
			TC_ERROR("Error reading block %d\n", start + i);
			return ret;
		}

		ret++;
	}

	return ret;
}

/* Function to erase SD memory card blocks
 * Returns number of blocks erased
 */
int sd_mem_card_erase_blocks(u32_t start, u32_t num_blocks)
{
	u32_t i;
	int ret = 0, timeout;
	struct sd_cmd cmd;
	struct device *dev;

	dev = device_get_binding(CONFIG_SDIO_DEV_NAME);
	if (dev == NULL)
		return -EPERM;

	if ((start + num_blocks) > max_blks) {
		TC_ERROR("Max Blocks = %d : Erase request [%d -> %d]\n",
			 max_blks, start, start + num_blocks);
		return -EINVAL;
	}

	for (i = 0; i < num_blocks; i++) {
		/* Set start erase block */
		cmd.cmd_idx = SD_CMD32_ERASE_WR_BLK_START;
		cmd.cmd_arg = hc_supported ? (start + i) :
					     (start + i)*wr_blk_len;
		cmd.resp_type = SD_RSP_R1;

		if (sd_send_command(dev, &cmd, NULL)) {
			TC_ERROR("Error setting erase block start: %d\n", ret);
			return ret;
		}

		/* Set end erase block (same as start for 1 block erase) */
		cmd.cmd_idx = SD_CMD33_ERASE_WR_BLK_END;

		if (sd_send_command(dev, &cmd, NULL)) {
			TC_ERROR("Error setting erase block end: %d\n", ret);
			return ret;
		}

		/* Erase selected block */
		cmd.cmd_idx = SD_CMD38_ERASE;
		cmd.cmd_arg = 0;
		cmd.resp_type = SD_RSP_R1b;

		if ((ret = sd_send_command(dev, &cmd, NULL)) != 0) {
			TC_ERROR("Error erasing block-%d: %d\n", i, ret);
			return ret;
		}

		/* Wait for erase to complete */
		cmd.cmd_idx = SD_CMD13_SEND_STATUS;
		cmd.cmd_arg = card_addr << 16;
		cmd.resp_type = SD_RSP_R1;

		timeout = 1000;
		do {
			if (sd_send_command(dev, &cmd, NULL) == 0) {
				/* READY_FOR_DATA */
				if ((cmd.response[0] & BIT(8)) &&
				    /* Current state != PRG */
				    (((cmd.response[0] >> 9) & 0xF) != 0x7))
					break;
			}
			k_sleep(1);
		} while (--timeout);

		if (timeout == 0)
			return ret;

		ret++;
	}

	return ret;
}

/* Function to write SD memory card blocks
 * Returns number of blocks written
 */
int sd_mem_card_write_blocks(u32_t start, u32_t num_blocks, u8_t *buf)
{
	u32_t i;
	int ret = 0;
	struct sd_cmd cmd;
	struct sd_data data;
	struct device *dev;

	dev = device_get_binding(CONFIG_SDIO_DEV_NAME);
	if (dev == NULL)
		return -EPERM;

	if ((start + num_blocks) > max_blks) {
		TC_ERROR("Max Blocks = %d : Write request [%d -> %d]\n",
			 max_blks, start, start + num_blocks);
		return -EINVAL;
	}

	for (i = 0; i < num_blocks; i++) {
		cmd.cmd_idx = SD_CMD24_WRITE_SINGLE_BLOCK;

		if (hc_supported)
			cmd.cmd_arg = start + i;
		else
			cmd.cmd_arg = (start + i) * wr_blk_len;

		cmd.resp_type = SD_RSP_R1;

		data.buf = buf + i*wr_blk_len;
		data.num_blocks = 1;
		data.block_size = wr_blk_len;
		data.flags = SD_DATA_WRITE;

		if (sd_send_command(dev, &cmd, &data)) {
			TC_ERROR("Error writing block %d\n", start + i);
			return ret;
		}

		ret++;
	}

	return ret;
}

u32_t sd_mem_card_get_sector_size(void)
{
	return rd_blk_len;
}

u32_t sd_mem_card_get_num_blocks(void)
{
	return max_blks;
}
