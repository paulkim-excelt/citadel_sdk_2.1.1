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

/* @file qspi_bcm58202.h
 *
 * QSPI driver header for bcm58202
 *
 * This header declares the qspi common driver functions for bcm58202
 * that can be used by the flash device specific driver modules
 *
 */

#ifndef __QSPI_BCM58202_H__
#define __QSPI_BCM58202_H__

#include <qspi_flash.h>

struct qspi_driver_data;

#define QSPI_FLASH_JEDEC_READ_MFG_ID_COMMAND        0x9F

typedef enum qspi_flash_lane_mode {
	QSPI_LANE_MODE_1_1_1 = 0,   /**!< Cmd: 1, Addr: 1, Data: 1 */
	QSPI_LANE_MODE_1_1_2,       /**!< Cmd: 1, Addr: 1, Data: 2 */
	QSPI_LANE_MODE_1_2_2,       /**!< Cmd: 1, Addr: 2, Data: 2 */
	QSPI_LANE_MODE_2_2_2,       /**!< Cmd: 2, Addr: 2, Data: 2 */
	QSPI_LANE_MODE_1_1_4,       /**!< Cmd: 1, Addr: 1, Data: 4 */
	QSPI_LANE_MODE_1_4_4,       /**!< Cmd: 1, Addr: 4, Data: 4 */
	QSPI_LANE_MODE_4_4_4,       /**!< Cmd: 4, Addr: 4, Data: 4 */
} qspi_flash_lane_mode_e;

typedef enum wspi_command {
	WSPI_COMMAND_WREN = 0,      /**!< Write enable */
	WSPI_COMMAND_RDSR,          /**!< Read status register */
	WSPI_COMMAND_PP,            /**!< Page Program */
	WSPI_COMMAND_2PP,           /**!< Dual lane Page Program */
	WSPI_COMMAND_4PP,           /**!< Quad lane Page Program */
	WSPI_COMMAND_ENTER_4B,      /**!< Enter 4 byte mode */
	WSPI_COMMAND_EXIT_4B        /**!< Exit 4 byte mode */
} wspi_command_e;

/** BSPI functions */
/**
 * @brief       Configure BSPI data transfer mode
 * @details     This function configures BSPI for the specified lane mode
 *
 * @param[in]   lane_mode - Lane mode to configure to
 */
void bspi_configure_lane_mode(qspi_flash_lane_mode_e lane_mode);

/**
 * @brief       Configure BSPI dummy cycle count
 * @details     This function configures BSPI dummy cycle count for reads
 *
 * @param[in]   dummy_cycles - no. of dummy cycles to be clocked after address
 */
void bspi_set_dummy_cycles(u8_t dummy_cycles);

/**
 * @brief       Set BSPI mode byte
 * @details     This function configures BSPI for transmitting the
 *              optional mode byte in the SPI read transaction
 *
 * @param[in]   mode - pointer to mode byte, null to disable mode byte
 * @param[in]   qspi_mode - QSPI mode
 */
void bspi_set_mode_byte(u8_t *mode, u8_t qspi_mode);

/**
 * @brief       Set BSPI Command byte
 * @details     This function configures the command to be used
 *              for BSPI read
 *
 * @param[in]   mode - pointer to mode byte, null to disable mode byte
 */
void bspi_set_command_byte(u8_t command);

/**
 * @brief       Set BSPI 4 byte address mode
 * @details     This function configures BSPI to address the flash in
 *              4 byte address mode
 *
 * @param[in]   enable - true to enable, false to disable
 * @param[in]   byte_4 - Byte 4 of the address, ignored if enable == false
 */
void bspi_set_4_byte_addr_mode(bool enable, u8_t byte_4);

/** MSPI functions */
/**
 * @brief       MSPI transfer
 * @details     This function carries out a SPI transaction over MSPI
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   tx - Transmit buffer
 * @param[in]   tx_len - Data size to send
 * @param[in]   rx - Receive buffer
 * @param[in]   rx_len - Number of bytes to read after sending tx bytes
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
int mspi_transfer(
	struct qspi_driver_data *dd,
	u8_t *tx,
	u32_t tx_len,
	u8_t *rx,
	u32_t rx_len);

/** WSPI functions */
/**
 * @brief       Set Write enable latch bit info
 * @details     This function programs WSPI with bit location and polarity
 *              of the WREN status bit as specified
 *
 * @param[in]   bit_pos - Bit position in status register
 * @param[in]   invert_polatity - true if bit needs to be flipped
 */
void wspi_set_wren_bit_info(u8_t bit_pos, bool invert_polarity);

/**
 * @brief       Set Write in progress bit info
 * @details     This function programs WSPI with bit location and polarity
 *              of the WIP(BUSY) status bit as specified
 *
 * @param[in]   bit_pos - Bit position in status register
 * @param[in]   invert_polatity - true if bit needs to be flipped
 */
void wspi_set_wip_bit_info(u8_t bit_pos, bool invert_polarity);

/**
 * @brief       Configure WSPI data transfer mode
 * @details     This function configures WSPI for the specified lane mode
 *
 * @param[in]   lane_mode - Lane mode to configure to
 */
void wspi_configure_lane_mode(qspi_flash_lane_mode_e lane_mode);

/**
 * @brief       Set WSPI Command byte
 * @details     This function configures the WSPI command register
 *
 * @param[in]   type - command type
 * @param[in]   command - command byte
 */
void wspi_set_command(wspi_command_e type, u8_t command);

/**
 * @brief       Set WSPI 4 byte address mode
 * @details     This function configures WSPI to address the flash in
 *              4 byte address mode
 *
 * @param[in]   enable - true to enable
 */
void wspi_set_4_byte_addr_mode(bool enable);

/**
 * @brief       Configures 4th byte of address for WSPI
 * @details     This function configures WSPI to derive the 4th address byte
 *              either from the register or from the address
 *
 * @param[in]   byte_4 - Byte 4 of the address to be used from register,
 *                       Setting it to NULL will cause WSPI to use 4th byte from
 *                       from the address
 */
void wspi_set_use_4th_byte_from_register(u8_t *byte_4);

/**
 * @brief       memcpy with maximized AXI burst size for unaligned src addresses
 * @details     This function performs a memory to memory copy, while maximizing
 *              AXI burst size, by using ldmia/stmia instructions. This function
 *              works on the assumption that the destination address and the
 *              size is 4-byte aligned. There is no resitrion on the alignement
 *              of the source address.
 *
 * @param[in]   dst_addr - destination address to copy data to (word aligned)
 * @param[in]   src_addr - source address to copy the data from
 * @param[in]   size - Size of data transfer (word aligned)
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static inline void qspi_memcpy_src_unaligned(u8_t *dst_addr,
				      u8_t *src_addr,
				      u32_t size)
{
	__asm__ volatile ("push {r0-r3, lr}\t\n"
			  "push {%[size]}\t\n"
			  "push {%[src]}\t\n"
			  "push {%[dst]}\t\n"
			  "pop {r0 - r2} \t\n"
			  "blx qspi_memcpy_src_unaligned_asm\t\n"
			  "pop {r0-r3, lr}\t\n"
			  : : [dst] "r" (dst_addr),
			      [src] "r" (src_addr),
			      [size] "r" (size)
			  : "memory");
}


/**
 * @brief       Updated QSPI command information
 * @details     This function updates the commands needed to transact with the
 *		qspi flash device. This api is called by the flash specific
 *		driver to update the command bytes depending on the mode in
 *		which flash is being configured. For example, some flashes
 *		(Spansion 256 Mbit) provide a different set of commands
 *		to access the flash, when they are in 4 byte mode. So the flash
 *		specific api that enables/disables the 4 byte addressing mode
 *		would use this command to update the spi device access commands
 *              appropriately.
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   commands - Pointer to qspi commands structure
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
int qspi_update_command_info(struct qspi_driver_data *dd,
			     qspi_flash_command_info_s *commands);

#endif /* __QSPI_BCM58202_H__ */
