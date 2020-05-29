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

/* @file qspi_bcm58202_interface.h
 *
 * QSPI flash Driver interface header
 *
 * This header defines the interface that each flash
 * device specific driver needs to implement. The common
 * module in the flash driver uses this interface to
 * read/write/erase the flash.
 *
 */

#ifndef __QSPI_BCM58202_INTERFACE_H__
#define __QSPI_BCM58202_INTERFACE_H__

#include <qspi_flash.h>
#include <stdbool.h>

/* Forward declaration */
struct qspi_driver_data;

/** Flash devices supported */
typedef enum qspi_serial_flash_type {
	FLASH_SPANSION = 0,
	FLASH_MACRONIX,
	FLASH_MICRON,
	FLASH_GIGA_DEVICE,
	FLASH_XMC,
	FLASH_WINBOND,
	FLASH_MAXIMUM
} qspi_serial_flash_type_e;

/** Flash commands and status bit info
 *  specific to the flash device */
typedef struct qspi_flash_command_info {
	u8_t  read;		/**!< Read command (regular, not fast read) */
	u8_t  write;		/**!< Write command */
	u8_t  chip_erase;	/**!< Chip erase command */
	u8_t  write_enable;	/**!< Write enable command */
	u8_t  write_disable;	/**!< Write enable command */
	u8_t  read_status;	/**!< Read status command */
	u8_t  wren_bit_pos;	/**!< Bit position of write enable bit in Status
				  */
	u8_t  busy_bit_pos;	/**!< Bit position of busy (WIP) bit in Status
				  */
	bool  page_program;	/**!< Page Program command is available */
	u8_t  num_addr_bits;	/**!< The no. of address bits to send in 4 byte
				  *   mode. This is either 32 or 24. Some flash
				  *   devices require a 3 byte address on the
				  *   wire (SI) even for accessing addresses >
				  *   16M byte. In such flash devices, the 4th
				  *   byte comes from an internal register
				  *   inside the flash. The device specific
				  *   driver module for such flash devices
				  *   will set num_addr_bits to 24. All other
				  *   drivers should set this to 32.
				  */
} qspi_flash_command_info_s;

/** Flash erase command information */
typedef struct qspi_erase_command_info {
	u32_t    erase_size;	/**!< Erase size in bytes */
	u8_t     erase_command;	/**!< Erase command byte */
} qspi_erase_command_info_s;

/**
 * @brief       Typedef for Initialization function
 * @details     Performs flash device specific initialization and configures
 *              MSPI/BSPI/WSPI with flash specific programming info
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   config - Flash device configuration
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_initialize)(
		struct qspi_driver_data *dd,
		struct qspi_flash_config *config);

/**
 * @brief       Typedef for Get size function
 * @details     Determines the flash device size in bytes
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[out]  size - Flash size in bytes
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_get_size)(struct qspi_driver_data *dd, u32_t *size);

/**
 * @brief       Typedef for Getting flash specific SPI commands
 * @details     Returns the flash device specific command bytes
 *
 * @param[out]  commands - Flash command information
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_get_command_info)(qspi_flash_command_info_s *commands);

/**
 * @brief       Typedef for Getting flash specific erase commands
 * @details     Returns the flash device specific erase command bytes and their
 *              sizes in the increasing order of the sizes
 *
 * @param[out]  erase_commands - List of erase command  bytes with their size
 * @param[out]  count - Number of erase commands
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_get_erase_commands)(
		qspi_erase_command_info_s **erase_cmds,
		u8_t *count);
/**
 * @brief       Typedef for function to enable 4 byte addressing on flash device
 * @details     This functions configures the QSPI controller to enable 4 byte
 *              addressing
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   mode - QSPI mode single/dual/quad
 * @param[in]   address - Address to be accessed
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_enable_4byte_addressing)(
		struct qspi_driver_data *dd,
		u8_t mode,
		u32_t address);

/**
 * @brief       Typedef for function to disable 4 byte addressing on flash
 * @details     This functions configures the QSPI controller to resume 3 byte
 *              address mode
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
typedef int (*qspi_flash_disable_4byte_addressing)(struct qspi_driver_data *dd,
						   u8_t mode);

/**
 * @brief   QSPI flash driver interface
 * @details Interface to the flash device specific driver module
 */
typedef struct qspi_flash_device_interface {
	/**!< Initializes the QSPI hardware */
	qspi_flash_initialize			init;
	/**!< Returns the size of the flash device in bytes */
	qspi_flash_get_size			get_size;
	/**!< Returns the commands and status bit masks for the flash device */
	qspi_flash_get_command_info		get_command_info;
	/**!< Returns the erase commands supported by flash */
	qspi_flash_get_erase_commands		get_erase_commands;
	/**!< Enables 4 byte addressing on the flash device */
	qspi_flash_enable_4byte_addressing	enable_4byte_addr;
	/**!< Restores 4 byte addressing on the flash device  */
	qspi_flash_disable_4byte_addressing	disable_4byte_addr;
} qspi_flash_device_interface_s;

/** Flash device get interface function */
typedef int (*qspi_flash_get_interface)(
		qspi_flash_device_interface_s **interface);

/** Flash device specific get interface functions for supported flashes */
int qspi_flash_spansion_get_interface(
		qspi_flash_device_interface_s **interface);
int qspi_flash_macronix_get_interface(
		qspi_flash_device_interface_s **interface);
int qspi_flash_micron_get_interface(
		qspi_flash_device_interface_s **interface);
int qspi_flash_giga_device_get_interface(
		qspi_flash_device_interface_s **interface);
int qspi_flash_xmc_get_interface(
		qspi_flash_device_interface_s **interface);
int qspi_flash_winbond_get_interface(
		qspi_flash_device_interface_s **interface);

#endif /* __QSPI_BCM58202_INTERFACE_H__ */
