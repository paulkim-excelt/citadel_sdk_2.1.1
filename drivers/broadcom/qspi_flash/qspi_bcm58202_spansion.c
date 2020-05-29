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

/* @file qspi_bcm58202_spansion.c
 *
 * QSPI flash driver for Spansion S25FL129P device
 *
 * This driver implements the flash driver interface
 * defined by the common qspi driver module for Spansion S25FL128S/S25FL256S
 * serial flash device
 *
 * This device supports
 * FAST READ
 * DUAL OUTPUT READ	: 1-1-2 read - 8 dummy
 * DUAL IO READ		: 1-2-2 read - Mode byte (0xA0)
 * QUAD OUTPUT READ	: 1-1-4 read - 8 dummy
 * QUAD IO READ		: 1-4-4 read - Mode byte (0xA0) + 4 dummy cycles
 * QPP (Quad page prog)	: 1-1-4 write
 *
 */

#include <toolchain/gcc.h>
#include "qspi_bcm58202_interface.h"
#include "qspi_bcm58202.h"
#include <misc/util.h>

#define CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS

/* Basic Flash commands */
#define QSPI_SPANSION_CMD_READ			0x03
#define QSPI_SPANSION_CMD_WRITE			0x02
#define QSPI_SPANSION_CMD_CHIP_ERASE		0x60
#define QSPI_SPANSION_CMD_WRITE_ENABLE		0x06
#define QSPI_SPANSION_CMD_WRITE_DISABLE		0x04
#define QSPI_SPANSION_CMD_READ_STATUS		0x05
#define QSPI_SPANSION_CMD_WRITE_STATUS_CFG	0x01
#define QSPI_SPANSION_CMD_READ_CFG		0x35
#define QSPI_SPANSION_CMD_BANK_REG_READ		0x16
#define QSPI_SPANSION_CMD_BANK_REG_WRITE	0x17

/* Status bit positions */
#define QSPI_SPANSION_CMD_WREN_BIT_POS		1
#define QSPI_SPANSION_CMD_BUSY_BIT_POS		0

/* Configuration register bit positions */
#define QSPI_SPANSION_CMD_QUAD_BIT_POS		1

/* Number of address bits in 4 byte addr mode */
#define QSPI_SPANSION_CMD_NUM_ADDR_BITS		24

/* Additional flash commands */
#define QSPI_SPANSION_CMD_FAST_READ		0x0B
#define QSPI_SPANSION_CMD_PAGE_PROGRAM		0x02
#define QSPI_SPANSION_CMD_1_1_2_READ		0x3B
#define QSPI_SPANSION_CMD_1_2_2_READ		0xBB
#define QSPI_SPANSION_CMD_1_1_4_READ		0x6B
#define QSPI_SPANSION_CMD_1_4_4_READ		0xEB
#define QSPI_SPANSION_CMD_1_1_4_PAGE_PROG	0x32

/* Erase commands */
#define QSPI_SPANSION_CMD_ERASE_4K		0x20
#define QSPI_SPANSION_CMD_ERASE_8K		0x40
#define QSPI_SPANSION_CMD_ERASE_64K		0xD8

/* Dummy cycles */
#define QSPI_SPANSION_FAST_READ_DUMMY		8
#define QSPI_SPANSION_1_1_2_READ_DUMMY		8
#define QSPI_SPANSION_1_2_2_READ_DUMMY		0
#define QSPI_SPANSION_1_1_4_READ_DUMMY		8
#define QSPI_SPANSION_1_4_4_READ_DUMMY		4

#ifdef CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS
/* Basic Flash commands */
#define QSPI_SPANSION_4B_CMD_READ		0x13
#define QSPI_SPANSION_4B_CMD_WRITE		0x12

/* Number of address bits in 4 byte addr mode */
#define QSPI_SPANSION_4B_CMD_NUM_ADDR_BITS	32

/* Additional flash commands */
#define QSPI_SPANSION_4B_CMD_FAST_READ		0x0C
#define QSPI_SPANSION_4B_CMD_PAGE_PROGRAM	0x12
#define QSPI_SPANSION_4B_CMD_1_1_2_READ		0x3C
#define QSPI_SPANSION_4B_CMD_1_2_2_READ		0xBC
#define QSPI_SPANSION_4B_CMD_1_1_4_READ		0x6C
#define QSPI_SPANSION_4B_CMD_1_4_4_READ		0xEC
#define QSPI_SPANSION_4B_CMD_1_1_4_PAGE_PROG	0x34

/* Erase commands */
#define QSPI_SPANSION_4B_CMD_ERASE_4K		0x21
#define QSPI_SPANSION_4B_CMD_ERASE_64K		0xDC

/* Erase commands info in 4 byte addressing mode */
static qspi_erase_command_info_s erase_commands_4b[] = {
	{KB(4),  QSPI_SPANSION_4B_CMD_ERASE_4K},
	{KB(64), QSPI_SPANSION_4B_CMD_ERASE_64K},
};

/* Flash access commands in 4 byte addressing mode */
static qspi_flash_command_info_s commands_4b = {
	.read          = QSPI_SPANSION_4B_CMD_READ,
	.write         = QSPI_SPANSION_4B_CMD_WRITE,
	.chip_erase    = QSPI_SPANSION_CMD_CHIP_ERASE,
	.write_enable  = QSPI_SPANSION_CMD_WRITE_ENABLE,
	.write_disable = QSPI_SPANSION_CMD_WRITE_DISABLE,
	.read_status   = QSPI_SPANSION_CMD_READ_STATUS,
	.wren_bit_pos  = QSPI_SPANSION_CMD_WREN_BIT_POS,
	.busy_bit_pos  = QSPI_SPANSION_CMD_BUSY_BIT_POS,
	.page_program  = QSPI_SPANSION_4B_CMD_PAGE_PROGRAM,
	.num_addr_bits = QSPI_SPANSION_4B_CMD_NUM_ADDR_BITS
};

#endif /* CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS */

/* Flash access commands in 3 byte addressing mode */
static qspi_flash_command_info_s commands_3b = {
	.read          = QSPI_SPANSION_CMD_READ,
	.write         = QSPI_SPANSION_CMD_WRITE,
	.chip_erase    = QSPI_SPANSION_CMD_CHIP_ERASE,
	.write_enable  = QSPI_SPANSION_CMD_WRITE_ENABLE,
	.write_disable = QSPI_SPANSION_CMD_WRITE_DISABLE,
	.read_status   = QSPI_SPANSION_CMD_READ_STATUS,
	.wren_bit_pos  = QSPI_SPANSION_CMD_WREN_BIT_POS,
	.busy_bit_pos  = QSPI_SPANSION_CMD_BUSY_BIT_POS,
	.page_program  = QSPI_SPANSION_CMD_PAGE_PROGRAM,
	.num_addr_bits = QSPI_SPANSION_CMD_NUM_ADDR_BITS
};

/* Erase commands info in 3 byte addressing mode */
static qspi_erase_command_info_s erase_commands_3b[] = {
	{KB(4),  QSPI_SPANSION_CMD_ERASE_4K},
	{KB(64), QSPI_SPANSION_CMD_ERASE_64K},
};

static qspi_erase_command_info_s *erase_cmds_in_use = &erase_commands_3b[0];
static qspi_flash_command_info_s *cmds_in_use = &commands_3b;

/* We use 0x0 an not 0xA0, because the qspi controller
 * does not support continuous reads
 */
#define QSPI_SPANSION_DUAL_QUAD_READ_MODE_BYTE	0x00

/** Interface functions - See qspi_bcm58202_interface.h for api info */
static int spansion_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config);

static int spansion_flash_get_size(struct qspi_driver_data *dd, u32_t *size);

static int spansion_flash_get_command_info(qspi_flash_command_info_s *commands);

static int spansion_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count);

static int spansion_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode,
		u32_t address);

static int spansion_disable_4byte_addressing(struct qspi_driver_data *dd,
					     u8_t mode);

static qspi_flash_device_interface_s spansion_interface = {
	.init               = spansion_flash_init,
	.get_size           = spansion_flash_get_size,
	.get_command_info   = spansion_flash_get_command_info,
	.get_erase_commands = spansion_flash_get_erase_commands,
	.enable_4byte_addr  = spansion_enable_4byte_addressing,
	.disable_4byte_addr = spansion_disable_4byte_addressing,
};

int qspi_flash_spansion_get_interface(qspi_flash_device_interface_s **interface)
{
	if (interface != NULL) {
		*interface = &spansion_interface;
		return 0;
	} else {
		return -EINVAL;
	}
}

static int spansion_cfg_quad_bit(struct qspi_driver_data *dd, u8_t mode)
{
	int ret = 0;

	u8_t cfg;
	u8_t status;

	u8_t command[3], response;

	/* Read status register */
	command[0] = QSPI_SPANSION_CMD_READ_STATUS;
	ret = mspi_transfer(dd, &command[0], 1, &status, 1);
	if (ret)
		return ret;

	/* Read configuration register */
	command[0] = QSPI_SPANSION_CMD_READ_CFG;
	ret = mspi_transfer(dd, &command[0], 1, &cfg, 1);
	if (ret)
		return ret;

	/* Check if quad bit needs to be programmed */
	if (mode == QSPI_MODE_QUAD) {
		if ((cfg & (0x1 << QSPI_SPANSION_CMD_QUAD_BIT_POS)) != 0x0)
			return 0;
	} else {
		if ((cfg & (0x1 << QSPI_SPANSION_CMD_QUAD_BIT_POS)) == 0x0)
			return 0;
	}

	/* Flip the quad bit and prgram it */
	cfg ^= (0x1 << QSPI_SPANSION_CMD_QUAD_BIT_POS);

	/* Wait for WIP to be 0 */
	command[0] = QSPI_SPANSION_CMD_READ_STATUS;
	do {
		ret = mspi_transfer(dd, &command[0], 1, &response, 1);
		if (ret)
			return ret;
	} while (response & (0x1 << QSPI_SPANSION_CMD_BUSY_BIT_POS));

	/* Set Write enable latch:210
	 */
	command[0] = QSPI_SPANSION_CMD_WRITE_ENABLE;
	ret = mspi_transfer(dd, &command[0], 1, NULL, 0);
	if (ret)
		return ret;

	/* Write the configuration register */
	command[0] = QSPI_SPANSION_CMD_WRITE_STATUS_CFG;
	command[1] = status;
	command[2] = cfg;
	ret = mspi_transfer(dd, &command[0], 3, NULL, 0);
	if (ret)
		return ret;

	/* WEL is disabled after the write to config register
	 * So no need to send write disable here
	 */

	return ret;
}

static int spansion_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config)
{
	int ret;
	u8_t mode;

	/* Boot SPI initialization - Initialize to 3 byte fast read */
	bspi_set_4_byte_addr_mode(false, 0);

	switch (QSPI_DATA_LANE(config->op_mode)) {
	default:
	case QSPI_MODE_SINGLE:
		bspi_set_command_byte(QSPI_SPANSION_CMD_FAST_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_1_1);
		bspi_set_dummy_cycles(QSPI_SPANSION_FAST_READ_DUMMY);
		bspi_set_mode_byte(NULL, 0);
		break;
	case QSPI_MODE_DUAL:
		bspi_set_command_byte(QSPI_SPANSION_CMD_1_2_2_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_2_2);
		bspi_set_dummy_cycles(QSPI_SPANSION_1_2_2_READ_DUMMY);
		mode = QSPI_SPANSION_DUAL_QUAD_READ_MODE_BYTE;
		bspi_set_mode_byte(&mode, QSPI_MODE_DUAL);
		break;
	case QSPI_MODE_QUAD:
		bspi_set_command_byte(QSPI_SPANSION_CMD_1_4_4_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_4_4);
		bspi_set_dummy_cycles(QSPI_SPANSION_1_4_4_READ_DUMMY);
		mode = QSPI_SPANSION_DUAL_QUAD_READ_MODE_BYTE;
		bspi_set_mode_byte(&mode, QSPI_MODE_QUAD);
		break;
	}

	/* WSPI specific initialization */
	wspi_set_wren_bit_info(QSPI_SPANSION_CMD_WREN_BIT_POS, false);
	wspi_set_wip_bit_info(QSPI_SPANSION_CMD_BUSY_BIT_POS, false);

	/* Commands */
	wspi_set_command(WSPI_COMMAND_WREN, QSPI_SPANSION_CMD_WRITE_ENABLE);
	wspi_set_command(WSPI_COMMAND_RDSR, QSPI_SPANSION_CMD_READ_STATUS);
	wspi_set_command(WSPI_COMMAND_PP, QSPI_SPANSION_CMD_PAGE_PROGRAM);
	wspi_set_command(WSPI_COMMAND_4PP, QSPI_SPANSION_CMD_1_1_4_PAGE_PROG);

	switch (QSPI_DATA_LANE(config->op_mode)) {
	default:
	case QSPI_MODE_SINGLE:
	case QSPI_MODE_DUAL: /* No support for 2 PP in Macronix */
		wspi_configure_lane_mode(QSPI_LANE_MODE_1_1_1);
		break;
	case QSPI_MODE_QUAD:
		wspi_configure_lane_mode(QSPI_LANE_MODE_1_1_4);
		break;
	}

	/* 3 byte address mode */
	wspi_set_4_byte_addr_mode(false);
	wspi_set_use_4th_byte_from_register(NULL);

	/* Set/Reset the Quad bit in the flash configuration register */
	ret = spansion_cfg_quad_bit(dd, QSPI_DATA_LANE(config->op_mode));

	return ret;
}

static int spansion_flash_get_size(struct qspi_driver_data *dd, u32_t *size)
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

static int spansion_flash_get_command_info(qspi_flash_command_info_s *commands)
{
	if (commands == NULL)
		return -EINVAL;

	*commands = *cmds_in_use;

	return 0;
}

static int spansion_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count)
{
	if ((erase_cmds == NULL) || (count == NULL))
		return -EINVAL;

#ifdef CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS
	if (erase_cmds_in_use == &erase_commands_3b[0])
		*count = sizeof(erase_commands_3b)
			 / sizeof(qspi_erase_command_info_s);
	else
		*count = sizeof(erase_commands_4b)
			 / sizeof(qspi_erase_command_info_s);
#else
	*count = sizeof(erase_commands_3b) / sizeof(qspi_erase_command_info_s);
#endif

	*erase_cmds = erase_cmds_in_use;

	return 0;
}

#ifdef CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS
static int spansion_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode, u32_t address)
{
	int ret;

	ARG_UNUSED(mode);

	/* Configure 4 byte addressing mode commands */
	cmds_in_use = &commands_4b;
	erase_cmds_in_use = &erase_commands_4b[0];
	ret = qspi_update_command_info(dd, cmds_in_use);
	if (ret)
		return ret;

	/* Configure BSPI to enable 4 byte addressing */
	bspi_set_4_byte_addr_mode(true, (u8_t)(address >> 24));

	/* Mode byte and dummy cycles don't change, just the command byte */
	switch (mode) {
	default:
	case QSPI_MODE_SINGLE:
		bspi_set_command_byte(QSPI_SPANSION_4B_CMD_FAST_READ);
		break;
	case QSPI_MODE_DUAL:
		bspi_set_command_byte(QSPI_SPANSION_4B_CMD_1_2_2_READ);
		break;
	case QSPI_MODE_QUAD:
		bspi_set_command_byte(QSPI_SPANSION_4B_CMD_1_4_4_READ);
		break;
	}

	/* Configure WSPI to enable 4 byte addressing */
	wspi_set_4_byte_addr_mode(true);

	wspi_set_command(WSPI_COMMAND_PP, QSPI_SPANSION_4B_CMD_PAGE_PROGRAM);
	wspi_set_command(WSPI_COMMAND_4PP,
			 QSPI_SPANSION_4B_CMD_1_1_4_PAGE_PROG);

	return ret;
}

static int spansion_disable_4byte_addressing(struct qspi_driver_data *dd,
					     u8_t mode)
{
	int ret;

	/* Restore 3 byte addressing mode commands */
	cmds_in_use = &commands_3b;
	erase_cmds_in_use = &erase_commands_3b[0];
	ret = qspi_update_command_info(dd, cmds_in_use);
	if (ret)
		return ret;

	/* Configure BSPI to enable 3 byte addressing */
	bspi_set_4_byte_addr_mode(false, 0);
	/* Mode byte and dummy cycles don't change, just the command byte */
	switch (mode) {
	default:
	case QSPI_MODE_SINGLE:
		bspi_set_command_byte(QSPI_SPANSION_CMD_FAST_READ);
		break;
	case QSPI_MODE_DUAL:
		bspi_set_command_byte(QSPI_SPANSION_CMD_1_2_2_READ);
		break;
	case QSPI_MODE_QUAD:
		bspi_set_command_byte(QSPI_SPANSION_CMD_1_4_4_READ);
		break;
	}

	/* Configure WSPI to enable 3 byte addressing */
	wspi_set_4_byte_addr_mode(false);
	wspi_set_command(WSPI_COMMAND_PP, QSPI_SPANSION_CMD_PAGE_PROGRAM);
	wspi_set_command(WSPI_COMMAND_4PP, QSPI_SPANSION_CMD_1_1_4_PAGE_PROG);

	return ret;
}

#else /* !CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS */

static int spansion_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode, u32_t address)
{
	int ret;

	ARG_UNUSED(mode);

	u8_t command[2];

	ARG_UNUSED(address);

	/* Set (Bank address register) BAR[0] = 0x1 to enable
	 * access to higher 16 MB memory array. Note that with
	 * this config only 24 address bits are needed with
	 * same command bytes
	 */
	command[0] = QSPI_SPANSION_CMD_BANK_REG_WRITE;
	command[1] = 0x1;
	ret = mspi_transfer(dd, &command[0], 2, NULL, 0);

	return ret;
}

static int spansion_disable_4byte_addressing(struct qspi_driver_data *dd,
					     u8_t mode)
{
	int ret;

	u8_t command[2];

	ARG_UNUSED(mode);

	/* Set (Bank address register) BAR[0] = 0x0 to enable
	 * access to lower 16 MB memory array with 3 byte addressing
	 */
	command[0] = QSPI_SPANSION_CMD_BANK_REG_WRITE;
	command[1] = 0x0;
	ret = mspi_transfer(dd, &command[0], 2, NULL, 0);

	return ret;
}
#endif /* CONFIG_USE_4_BYTE_ADDRESSING_COMMANDS */
