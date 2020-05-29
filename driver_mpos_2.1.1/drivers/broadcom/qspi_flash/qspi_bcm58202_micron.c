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

/* @file qspi_bcm58202_micron.c
 *
 * QSPI flash driver for Micron N25Q256A device
 *
 * This driver implements the flash driver interface
 * defined by the common qspi driver module for Micron N25Q256
 * serial flash device
 *
 * Although the device supports Dual SPI (2-2-2) and Quad SPI protocols (4-4-4)
 * modes, this driver implements Quad and Dual lane accesses using the
 * extended SPI protocol (1-2-2 and 1-4-4), with Dual/Quad input/output mode.
 * In addition the dummy cycle count is always set to max of 10 cycles to allow
 * for operating at the highest allowed frequency in all modes.
 *
 * For writes (page program), it is not clear from the data sheet if 1-2-2 and
 * 1-4-4 modes are supported. Although the waveforms do suggest that "Extended
 * (Dual|Quad) input fast program" is the 1-x-x (x = 2 or 4) mode, the command
 * for these modes are provided explicitly. Also, the command provided quad mode
 * (0x12) is the same as the 4-byte page program command. Because of this
 * ambiguity, this driver only implements the 1-1-2 and 1-1-4 modes for write.
 */

#include <toolchain/gcc.h>
#include "qspi_bcm58202_interface.h"
#include "qspi_bcm58202.h"
#include <misc/util.h>

/* Basic Flash commands */
#define QSPI_MICRON_CMD_READ			0x03
#define QSPI_MICRON_CMD_WRITE			0x02
#define QSPI_MICRON_CMD_CHIP_ERASE		0xC7
#define QSPI_MICRON_CMD_WRITE_ENABLE		0x06
#define QSPI_MICRON_CMD_WRITE_DISABLE		0x04
#define QSPI_MICRON_CMD_READ_STATUS		0x05
#define QSPI_MICRON_CMD_WRITE_STATUS_CFG	0x01
#define QSPI_MICRON_CMD_READ_EXT_ADDR		0xC8
#define QSPI_MICRON_CMD_WRITE_EXT_ADDR		0xC5

/* Status bit positions */
#define QSPI_MICRON_CMD_WREN_BIT_POS		1
#define QSPI_MICRON_CMD_BUSY_BIT_POS		0

#define NVCR_RESET_HOLD_ENABLE_BIT_POS		4

/* Number of address bits in 4 byte addr mode */
#define QSPI_MICRON_CMD_NUM_ADDR_BITS		24

/* Additional flash commands */
#define QSPI_MICRON_CMD_FAST_READ		0x0B
#define QSPI_MICRON_CMD_PAGE_PROGRAM		0x02
#define QSPI_MICRON_CMD_1_1_2_READ		0x3B
#define QSPI_MICRON_CMD_1_2_2_READ		0xBB
#define QSPI_MICRON_CMD_1_1_4_READ		0x6B
#define QSPI_MICRON_CMD_1_4_4_READ		0xEB
#define QSPI_MICRON_CMD_1_1_2_WRITE		0xA2
#define QSPI_MICRON_CMD_1_1_4_WRITE		0x32
#define QSPI_MICRON_CMD_READ_ENH_VOL_CFG_REG	0x65
#define QSPI_MICRON_CMD_WRITE_ENH_VOL_CFG_REG	0x61

/* Erase commands */
#define QSPI_MICRON_CMD_ERASE_4K		0x20
#define QSPI_MICRON_CMD_ERASE_64K		0xD8

/* Dummy cycles */
#define QSPI_MICRON_FAST_READ_DUMMY		8
#define QSPI_MICRON_1_1_2_READ_DUMMY		8
#define QSPI_MICRON_1_2_2_READ_DUMMY		8
#define QSPI_MICRON_1_1_4_READ_DUMMY		8
#define QSPI_MICRON_1_4_4_READ_DUMMY		8

/* Flash access commands */
static qspi_flash_command_info_s commands = {
	.read          = QSPI_MICRON_CMD_READ,
	.write         = QSPI_MICRON_CMD_WRITE,
	.chip_erase    = QSPI_MICRON_CMD_CHIP_ERASE,
	.write_enable  = QSPI_MICRON_CMD_WRITE_ENABLE,
	.write_disable = QSPI_MICRON_CMD_WRITE_DISABLE,
	.read_status   = QSPI_MICRON_CMD_READ_STATUS,
	.wren_bit_pos  = QSPI_MICRON_CMD_WREN_BIT_POS,
	.busy_bit_pos  = QSPI_MICRON_CMD_BUSY_BIT_POS,
	.page_program  = QSPI_MICRON_CMD_PAGE_PROGRAM,
	.num_addr_bits = QSPI_MICRON_CMD_NUM_ADDR_BITS
};

/* Erase commands info in 3 byte addressing mode */
static qspi_erase_command_info_s erase_commands[] = {
	{KB(4),  QSPI_MICRON_CMD_ERASE_4K},
	{KB(64), QSPI_MICRON_CMD_ERASE_64K},
};

/** Interface functions - See qspi_bcm58202_interface.h for api info */
static int micron_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config);

static int micron_flash_get_size(struct qspi_driver_data *dd, u32_t *size);

static int micron_flash_get_command_info(qspi_flash_command_info_s *commands);

static int micron_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count);

static int micron_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode,
		u32_t address);

static int micron_disable_4byte_addressing(struct qspi_driver_data *dd,
					     u8_t mode);

static qspi_flash_device_interface_s micron_interface = {
	.init               = micron_flash_init,
	.get_size           = micron_flash_get_size,
	.get_command_info   = micron_flash_get_command_info,
	.get_erase_commands = micron_flash_get_erase_commands,
	.enable_4byte_addr  = micron_enable_4byte_addressing,
	.disable_4byte_addr = micron_disable_4byte_addressing,
};

int qspi_flash_micron_get_interface(qspi_flash_device_interface_s **interface)
{
	if (interface != NULL) {
		*interface = &micron_interface;
		return 0;
	} else {
		return -EINVAL;
	}
}

static int micron_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config)
{
	/* Disable the RESET/HOLD signalling on the flash to allow
	 * DQ2/DQ3 signalling on these lines for Quad mode
	 */
	if (QSPI_DATA_LANE(config->op_mode) == QSPI_MODE_QUAD) {
		int ret;
		u8_t cmd_resp[2];

		cmd_resp[0] = QSPI_MICRON_CMD_READ_ENH_VOL_CFG_REG;
		ret = mspi_transfer(dd, &cmd_resp[0], 1, &cmd_resp[1], 1);
		if (ret)
			return ret;

		/* Disable only if it is enabled */
		if (cmd_resp[1] & BIT(NVCR_RESET_HOLD_ENABLE_BIT_POS)) {
			/* Write enable needed before NVCR can be written to */
			cmd_resp[0] = QSPI_MICRON_CMD_WRITE_ENABLE;
			ret = mspi_transfer(dd, &cmd_resp[0], 1, NULL, 0);
			if (ret)
				return ret;

			/* Write NVCR */
			cmd_resp[0] = QSPI_MICRON_CMD_WRITE_ENH_VOL_CFG_REG;
			cmd_resp[1] &= ~BIT(NVCR_RESET_HOLD_ENABLE_BIT_POS);
			ret = mspi_transfer(dd, &cmd_resp[0], 2, NULL, 0);
			if (ret)
				return ret;

			cmd_resp[0] = QSPI_MICRON_CMD_WRITE_DISABLE;
			mspi_transfer(dd, &cmd_resp[0], 1, NULL, 0);
		}
	}

	/* Boot SPI initialization - Initialize to 3 byte fast read */
	bspi_set_4_byte_addr_mode(false, 0);

	switch (QSPI_DATA_LANE(config->op_mode)) {
	default:
	case QSPI_MODE_SINGLE:
		bspi_set_command_byte(QSPI_MICRON_CMD_FAST_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_1_1);
		bspi_set_dummy_cycles(QSPI_MICRON_FAST_READ_DUMMY);
		bspi_set_mode_byte(NULL, 0);
		break;
	case QSPI_MODE_DUAL:
		bspi_set_command_byte(QSPI_MICRON_CMD_1_2_2_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_2_2);
		bspi_set_dummy_cycles(QSPI_MICRON_1_2_2_READ_DUMMY);
		bspi_set_mode_byte(NULL, 0);
		break;
	case QSPI_MODE_QUAD:
		bspi_set_command_byte(QSPI_MICRON_CMD_1_1_4_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_1_4);
		bspi_set_dummy_cycles(QSPI_MICRON_1_1_4_READ_DUMMY);
		bspi_set_mode_byte(NULL, 0);
		break;
	}

	/* WSPI specific initialization */
	wspi_set_wren_bit_info(QSPI_MICRON_CMD_WREN_BIT_POS, false);
	wspi_set_wip_bit_info(QSPI_MICRON_CMD_BUSY_BIT_POS, false);

	/* Commands */
	wspi_set_command(WSPI_COMMAND_WREN, QSPI_MICRON_CMD_WRITE_ENABLE);
	wspi_set_command(WSPI_COMMAND_RDSR, QSPI_MICRON_CMD_READ_STATUS);
	wspi_set_command(WSPI_COMMAND_PP, QSPI_MICRON_CMD_PAGE_PROGRAM);
	wspi_set_command(WSPI_COMMAND_2PP, QSPI_MICRON_CMD_1_1_2_WRITE);
	wspi_set_command(WSPI_COMMAND_4PP, QSPI_MICRON_CMD_1_1_4_WRITE);

	switch (QSPI_DATA_LANE(config->op_mode)) {
	default:
	case QSPI_MODE_SINGLE:
		wspi_configure_lane_mode(QSPI_LANE_MODE_1_1_1);
		break;
	case QSPI_MODE_DUAL:
		wspi_configure_lane_mode(QSPI_LANE_MODE_1_1_2);
		break;
	case QSPI_MODE_QUAD:
		wspi_configure_lane_mode(QSPI_LANE_MODE_1_1_4);
		break;
	}

	/* 3 byte address mode */
	wspi_set_4_byte_addr_mode(false);
	wspi_set_use_4th_byte_from_register(NULL);

	return 0;
}

static int micron_flash_get_size(struct qspi_driver_data *dd, u32_t *size)
{
	int ret;
	u8_t command;
	u8_t response[3];

	if (size == NULL)
		return -EINVAL;

	command = QSPI_FLASH_JEDEC_READ_MFG_ID_COMMAND;

	ret = mspi_transfer(dd, &command, 1, response, 3);
	if (ret)
		return ret;

	/* In bytes */
	*size = 0x1UL << response[2];

	return 0;
}

static int micron_flash_get_command_info(qspi_flash_command_info_s *cmds)
{
	if (cmds == NULL)
		return -EINVAL;

	*cmds = commands;

	return 0;
}

static int micron_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count)
{
	if ((erase_cmds == NULL) || (count == NULL))
		return -EINVAL;

	*count = sizeof(erase_commands) / sizeof(qspi_erase_command_info_s);
	*erase_cmds = &erase_commands[0];

	return 0;
}

static int micron_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode, u32_t address)
{
	int ret;
	u8_t command[2];

	ARG_UNUSED(mode);
	ARG_UNUSED(address);

	/* First Enable WEL */
	command[0] = QSPI_MICRON_CMD_WRITE_ENABLE;
	ret = mspi_transfer(dd, &command[0], 1, NULL, 0);
	if (ret)
		return ret;

	/* Then send program it - of Ext addr register to 1. */
	command[0] = QSPI_MICRON_CMD_WRITE_EXT_ADDR;
	command[1] = 0x1;
	ret = mspi_transfer(dd, &command[0], 2, NULL, 0);

	return ret;
}

static int micron_disable_4byte_addressing(struct qspi_driver_data *dd,
					     u8_t mode)
{
	int ret;

	u8_t command[2];

	ARG_UNUSED(mode);

	/* First Enable WEL */
	command[0] = QSPI_MICRON_CMD_WRITE_ENABLE;
	ret = mspi_transfer(dd, &command[0], 1, NULL, 0);
	if (ret)
		return ret;

	/* Then send program it - of Ext addr register to 0. */
	command[0] = QSPI_MICRON_CMD_WRITE_EXT_ADDR;
	command[1] = 0x0;
	ret = mspi_transfer(dd, &command[0], 2, NULL, 0);

	return ret;
}
