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

/* @file qspi_bcm58202_xmc.c
 *
 * QSPI flash driver for XMC parts
 *
 * This driver implements the flash driver interface
 * defined by the common qspi driver module for XMC
 * XM25QH64A/XM25QH128A serial flash device. The driver currently
 * only supports devices <= 16 MB (So 4 byte addressing in not supported)
 *
 * This device supports
 * FAST READ
 * DUAL OUTPUT READ : 1-1-2 read - 8 dummy
 * DUAL I/O READ    : 1-2-2 read - 4 dummy
 * QUAD OUTPUT READ : 1-1-4 read - 8 dummy
 * QUAD I/O READ    : 1-4-4 read - 6 dummy
 *
 */

#include <toolchain/gcc.h>
#include "qspi_bcm58202_interface.h"
#include "qspi_bcm58202.h"
#include <misc/util.h>

/* Basic Flash commands */
#define QSPI_XMC_CMD_READ		0x03
#define QSPI_XMC_CMD_WRITE		0x02
#define QSPI_XMC_CMD_CHIP_ERASE		0xC7
#define QSPI_XMC_CMD_WRITE_ENABLE	0x06
#define QSPI_XMC_CMD_WRITE_DISABLE	0x04
#define QSPI_XMC_CMD_READ_STATUS	0x05
#define QSPI_XMC_CMD_WRITE_STATUS	0x01

/* Status bit positions */
#define QSPI_XMC_CMD_WREN_BIT_POS	1
#define QSPI_XMC_CMD_BUSY_BIT_POS	0

/* Number of address bits in 4 byte addr mode */
#define QSPI_XMC_CMD_NUM_ADDR_BITS	24

/* Additional flash commands */
#define QSPI_XMC_CMD_FAST_READ		0x0B
#define QSPI_XMC_CMD_PAGE_PROGRAM	0x02
#define QSPI_XMC_CMD_DREAD		0x3B
#define QSPI_XMC_CMD_2READ		0xBB
#define QSPI_XMC_CMD_QREAD		0x6B
#define QSPI_XMC_CMD_4READ		0xEB
#define QSPI_XMC_CMD_4PP		0x32

/* Erase commands */
#define QSPI_XMC_CMD_ERASE_4K		0x20
#define QSPI_XMC_CMD_ERASE_32K		0x52
#define QSPI_XMC_CMD_ERASE_64K		0xD8

/* Dummy cycles */
#define QSPI_XMC_FAST_READ_DUMMY	8
#define QSPI_XMC_DREAD_DUMMY		8
#define QSPI_XMC_2READ_DUMMY		4
#define QSPI_XMC_QREAD_DUMMY		8
#define QSPI_XMC_4READ_DUMMY		6

/** Interface functions - See qspi_interface.h for api info */
static int xmc_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config);

static int xmc_flash_get_size(struct qspi_driver_data *dd, u32_t *size);

static int xmc_flash_get_command_info(qspi_flash_command_info_s *commands);

static int xmc_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count);

static int xmc_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode,
		u32_t address);

static int xmc_disable_4byte_addressing(struct qspi_driver_data *dd, u8_t mode);

static qspi_flash_device_interface_s xmc_interface = {
	.init               = xmc_flash_init,
	.get_size           = xmc_flash_get_size,
	.get_command_info   = xmc_flash_get_command_info,
	.get_erase_commands = xmc_flash_get_erase_commands,
	.enable_4byte_addr  = xmc_enable_4byte_addressing,
	.disable_4byte_addr = xmc_disable_4byte_addressing,
};

static qspi_erase_command_info_s erase_commands[] = {
	{KB(4),  QSPI_XMC_CMD_ERASE_4K},
	{KB(32), QSPI_XMC_CMD_ERASE_32K},
	{KB(64), QSPI_XMC_CMD_ERASE_64K},
};

int qspi_flash_xmc_get_interface(
	qspi_flash_device_interface_s **interface)
{
	if (interface != NULL) {
		*interface = &xmc_interface;
		return 0;
	} else {
		return -EINVAL;
	}
}

static int xmc_flash_init(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config)
{
	int ret;
	u32_t size;

	/* Get the size to determine what flash commands
	 * are supported by the device
	 */
	ret = xmc_flash_get_size(dd, &size);
	if (ret)
		return ret;

	/* Only 128 MBit flash and lower is supported for now */
	if (size > MB(16)) {
		return -EOPNOTSUPP;
	}

	/* Boot SPI initialization - Initialize to 3 byte fast read */
	bspi_set_4_byte_addr_mode(false, 0);
	bspi_set_mode_byte(NULL, 0); /* Macronix does not use a mode byte */

	switch (QSPI_DATA_LANE(config->op_mode)) {
	default:
	case QSPI_MODE_SINGLE:
		bspi_set_command_byte(QSPI_XMC_CMD_FAST_READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_1_1);
		bspi_set_dummy_cycles(QSPI_XMC_FAST_READ_DUMMY);
		break;
	case QSPI_MODE_DUAL:
		bspi_set_command_byte(QSPI_XMC_CMD_2READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_2_2);
		bspi_set_dummy_cycles(QSPI_XMC_2READ_DUMMY);
		break;
	case QSPI_MODE_QUAD:
		bspi_set_command_byte(QSPI_XMC_CMD_4READ);
		bspi_configure_lane_mode(QSPI_LANE_MODE_1_4_4);
		bspi_set_dummy_cycles(QSPI_XMC_4READ_DUMMY);
		break;
	}

	/* WSPI specific initialization */
	wspi_set_wren_bit_info(QSPI_XMC_CMD_WREN_BIT_POS, false);
	wspi_set_wip_bit_info(QSPI_XMC_CMD_BUSY_BIT_POS, false);

	/* Commands */
	wspi_set_command(WSPI_COMMAND_WREN, QSPI_XMC_CMD_WRITE_ENABLE);
	wspi_set_command(WSPI_COMMAND_RDSR, QSPI_XMC_CMD_READ_STATUS);
	wspi_set_command(WSPI_COMMAND_PP, QSPI_XMC_CMD_PAGE_PROGRAM);
	wspi_set_command(WSPI_COMMAND_4PP, QSPI_XMC_CMD_4PP);

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

	/* 3 byte address mode by default */
	wspi_set_4_byte_addr_mode(false);
	/* Use 4th byte from address */
	wspi_set_use_4th_byte_from_register(NULL);

	return 0;
}

static int xmc_flash_get_size(struct qspi_driver_data *dd, u32_t *size)
{
	int ret;
	u8_t command;
	u8_t response[3];
	u32_t capacity;

	if (size == NULL)
		return -EINVAL;

	command = QSPI_FLASH_JEDEC_READ_MFG_ID_COMMAND;

	ret = mspi_transfer(dd, &command, 1, response, 3);
	if (ret)
		return ret;

	/* In Mbits: 0x18 - 128 MBits, 0x17 - 64 MBits */
	capacity = (0x1 << ((unsigned int)response[2] - 0x11));

	/* In bytes */
	*size = MB(capacity >> 3);

	return 0;
}

static int xmc_flash_get_command_info(
		qspi_flash_command_info_s *commands)
{
	if (commands == NULL)
		return -EINVAL;

	commands->read          = QSPI_XMC_CMD_READ;
	commands->write         = QSPI_XMC_CMD_WRITE;
	commands->chip_erase    = QSPI_XMC_CMD_CHIP_ERASE;
	commands->write_enable  = QSPI_XMC_CMD_WRITE_ENABLE;
	commands->write_disable = QSPI_XMC_CMD_WRITE_DISABLE;
	commands->read_status   = QSPI_XMC_CMD_READ_STATUS;
	commands->wren_bit_pos  = QSPI_XMC_CMD_WREN_BIT_POS;
	commands->busy_bit_pos  = QSPI_XMC_CMD_BUSY_BIT_POS;
	commands->page_program  = QSPI_XMC_CMD_PAGE_PROGRAM;
	commands->num_addr_bits = QSPI_XMC_CMD_NUM_ADDR_BITS;

	return 0;
}

static int xmc_flash_get_erase_commands(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count)
{
	if ((erase_cmds == NULL) || (count == NULL))
		return -EINVAL;

	*count = sizeof(erase_commands)/sizeof(qspi_erase_command_info_s);
	*erase_cmds = &erase_commands[0];

	return 0;
}

static int xmc_enable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode,
		u32_t address)
{
	/* 4 byte addressing is currently not supported */
	ARG_UNUSED(dd);
	ARG_UNUSED(mode);
	ARG_UNUSED(address);

	return 0;
}

static int xmc_disable_4byte_addressing(
		struct qspi_driver_data *dd,
		u8_t mode)
{
	/* 4 byte addressing is currently not supported */
	ARG_UNUSED(dd);
	ARG_UNUSED(mode);

	return 0;
}
