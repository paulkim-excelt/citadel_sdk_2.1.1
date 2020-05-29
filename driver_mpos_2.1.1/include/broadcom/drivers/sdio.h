/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

#ifndef __SDIO_H__
#define __SDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/** All platform specific headers are included here */
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <device.h>

/**
 * @brief SD Mode Commands
 */
#define SD_CMD0_GO_IDLE_STATE			0		
#define SD_CMD2_ALL_SEND_CID			2
#define SD_CMD3_SEND_RELATIVE_ADDR		3
#define SD_CMD4_SET_DSR				4
#define SD_CMD5_IO_SEND_OP_COND			5
#define SD_CMD6_SWITCH_FUNC			6
#define SD_CMD7_SELECT_DESELECT_CARD		7
#define SD_CMD8_SEND_IF_COND			8
#define SD_CMD9_SEND_CSD			9
#define SD_CMD10_SEND_CID			10
#define SD_CMD11_VOLTAGE_SWITCH			11
#define SD_CMD12_STOP_TRANSMISSION		12
#define SD_CMD13_SEND_STATUS			13
#define SD_CMD15_GO_INACTIVE_STATE		15
#define SD_CMD16_SET_BLOCKLEN			16
#define SD_CMD17_READ_SINGLE_BLOCK		17
#define SD_CMD18_READ_MULTIPLE_BLOCK		18
#define SD_CMD19_SEND_TUNING_BLOCK		19
#define SD_CMD23_SET_BLOCK_COUNT		23
#define SD_CMD24_WRITE_SINGLE_BLOCK		24
#define SD_CMD25_WRITE_MULTIPLE_BLOCK		25
#define SD_CMD27_PROGRAM_CSD			27
#define SD_CMD28_SET_WRITE_PROT			28
#define SD_CMD29_CLR_WRITE_PROT			29
#define SD_CMD30_SEND_WRITE_PROT		30
#define SD_CMD32_ERASE_WR_BLK_START		32
#define SD_CMD33_ERASE_WR_BLK_END		33
#define SD_CMD38_ERASE				38
#define SD_CMD42_LOCK_UNLOCK			42
#define SD_CMD52_IO_RW_DIRECT			52
#define SD_CMD53_IO_RW_EXTENDED			53
#define SD_CMD55_APP_CMD			55
#define SD_CMD56_GEN_CMD			56

/**
 * @brief SD Mode Applciation Commands
 */
#define SD_ACMD6_SET_BUS_WIDTH			6
#define SD_ACMD13_SD_STATUS			13
#define SD_ACMD22_SEND_NUM_WR_BLOCKS		22
#define SD_ACMD23_SET_WR_BLK_ERASE_COUNT	23
#define SD_ACMD41_SD_APP_OP_COND		41
#define SD_ACMD42_SET_CLR_CARD_DETECT		42
#define SD_ACMD51_SEND_SCR			51

/**
 * @brief SD OCR (Operating conditions) register bit field masks
 */
#define SD_OCR_CARD_POWER_UP_STATUS		BIT(31)
/* Standard or high speed */
#define SD_OCR_CARD_CAPACITY_STATUS		BIT(30)
/* Voltage Profile */
#define SD_OCR_LOW_VOLTAGE			BIT(7)
#define SD_OCR_3_5_TO_3_6V			BIT(23)
#define SD_OCR_3_4_TO_3_5V			BIT(22)
#define SD_OCR_3_3_TO_3_4V			BIT(21)
#define SD_OCR_3_2_TO_3_3V			BIT(20)
#define SD_OCR_3_1_TO_3_2V			BIT(19)
#define SD_OCR_3_0_TO_3_1V			BIT(18)
#define SD_OCR_2_9_TO_3_0V			BIT(17)
#define SD_OCR_2_8_TO_2_7V			BIT(16)
#define SD_OCR_2_7_TO_2_8V			BIT(15)
#define SD_OCR_VOLTAGE_MASK			(BIT(23+1) - BIT(15))

/**
 * @brief SD CSR (Card status) register bit field masks
 */
#define SD_CSR_OUT_OF_RANGE			BIT(31)
#define SD_CSR_ADDRESS_ERROR			BIT(30)
#define SD_CSR_BLOCK_LEN_ERROR			BIT(29)
#define SD_CSR_ERASE_SEQ_ERROR			BIT(28)
#define SD_CSR_ERASE_PARAM			BIT(27)
#define SD_CSR_WP_VIOLATION			BIT(26)
#define SD_CSR_CARD_IS_LOCKED			BIT(25)
#define SD_CSR_LOCK_UNLOCK_FAIL			BIT(24)
#define SD_CSR_COM_CRC_ERROR			BIT(23)
#define SD_CSR_ILLEGAL_CMD			BIT(22)
#define SD_CSR_CARD_ECC_FAILED			BIT(21)
#define SD_CSR_CC_ERROR				BIT(20)
#define SD_CSR_ERROR				BIT(19)
#define SD_CSR_UNDERRUN				BIT(18)
#define SD_CSR_OVERRUN				BIT(17)
#define SD_CSR_CID_CSD_OVERRIDE			BIT(16)
#define SD_CSR_WP_ERASE_SKIP			BIT(15)
#define SD_CSR_CARD_ECC_DISABLED		BIT(14)
#define SD_CSR_ERASE_RESET			BIT(13)
#define SD_CSR_CURRENT_STATE			(0xF << 9)
#define SD_CSR_READY_FOR_DATA			BIT(8)
#define SD_CSR_APP_CMD				BIT(5)
#define SD_CSR_AKE_SEQ_ERROR			BIT(3)

/**
 * @brief SD Switch function (CMD6) mode bit masks
 */
#define SD_CMD6_MODE_CHECK			0
#define SD_CMD6_MODE_SWITCH			BIT(31)

/**
 * @brief SD response types and their attributes
 */
#define SD_RSP_PRESENT	BIT(0)
#define SD_RSP_136	BIT(1)		/* respons length - 136 bits */
#define SD_RSP_CRC	BIT(2)		/* CRC valid */
#define SD_RSP_BUSY	BIT(3)		/* Card may become busy */
#define SD_RSP_CMDIDX	BIT(4)		/* Contains command index */

#define SD_RSP_NONE	(0)
#define SD_RSP_R1	(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_CMDIDX)
#define SD_RSP_R1b	(SD_RSP_R1 | SD_RSP_BUSY)
#define SD_RSP_R2	(SD_RSP_PRESENT | SD_RSP_136 | SD_RSP_CRC)
#define SD_RSP_R3	(SD_RSP_PRESENT)
#define SD_RSP_R6	(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_CMDIDX)
#define SD_RSP_R7	(SD_RSP_PRESENT | SD_RSP_CRC | SD_RSP_CMDIDX)

/**
 * @brief Flags for SD commands that include a data transfer
 */
#define SD_DATA_READ	BIT(0)
#define SD_DATA_WRITE	BIT(1)

/**
 * @brief SD command/reponse structure
 * @details This structure is used to send the command the SD device
 *	    and get the response back, if the command expects one
 *
 * @param cmd_idx Command index of the command to send
 * @param resp_type Expected response type attributes
 * @param cmd_arg Optional command argument depending on the command
 * @param response Response words, excludes the 8 LSB bits (7-bit crc & end bit)
 */
struct sd_cmd {
	u8_t cmd_idx;
	u8_t resp_type;
	u32_t cmd_arg;
	u32_t response[4];
};

/**
 * @brief SD data transfer information structure
 * @details This structure provides the information required for commands
 *	    that invovle a data transfer
 *
 * @param buf Pointer to the buffer to hold the data for/from the data transfer
 * @param flags Flags to indicate whether the data is from host (Write) or to
 *		host (Read)
 * @param num_blocks Number of blocks of data to transfer
 * @param block_size Block size in bytes
 */
struct sd_data {
	u8_t *buf;
	u8_t flags;
	u32_t num_blocks;
	u32_t block_size;
};

/**
 * @typedef SD card inserted/removed callback
 * @brief Callback for SD card insertion/removal notification
 * @param inserted true if card insterted, false if card removed
 */
typedef void (*sd_card_detect_cb)(bool inserted);

/**
 * @typedef sd_api_send_command
 * @brief API for sending an SD command to the device
 * See sd_send_command() for argument descriptions
 */
typedef int (*sd_api_send_command)(
		struct device *dev,
		struct sd_cmd *cmd,
		struct sd_data *data);

/**
 * @typedef sd_api_set_bus_width
 * @brief API for setting the SD bus width
 * See sd_set_bus_width() for argument descriptions
 */
typedef int (*sd_api_set_bus_width)(struct device *dev, u8_t bus_width);

/**
 * @typedef sd_api_set_clock_speed
 * @brief API for setting the SD bus clock speed
 * See sd_set_clock_speed() for argument descriptions
 */
typedef int (*sd_api_set_clock_speed)(struct device *dev, u32_t clock_speed);

/**
 * @typedef sd_api_register_card_detect_cb
 * @brief API for registering card insertion/removal detect callback
 * See sd_set_bus_width() for argument descriptions
 */
typedef int (*sd_api_register_card_detect_cb)(
		struct device *dev,
		sd_card_detect_cb cb);

/**
 * @brief SD driver API
 *
 * @param send_cmd API to send a command to device
 * @param set_width API to set SD bus width
 * @param set_speed API to set clock speed
 * @param register_cd_cb API to register card detect callback
 */
struct sd_driver_api {
	sd_api_send_command send_cmd;
	sd_api_set_bus_width set_width;
	sd_api_set_clock_speed set_speed;
	sd_api_register_card_detect_cb register_cd_cb;
};

/**
 * @brief       Send SD command to device
 * @details     This api sends the SD command specified flash device and
 *		retrieves the response if any. If the command involves a data
 *		transfer, then the data is read from/written to the buffer
 *		provided
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cmd - Command information structure
 * @param[in]	data - Data buffer structure, optional depending on the command
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int sd_send_command(
			struct device *dev,
			struct sd_cmd *cmd,
			struct sd_data *data)
{
	struct sd_driver_api *api;
	api = (struct sd_driver_api *)dev->driver_api;
	return api->send_cmd(dev, cmd, data);
}

/**
 * @brief       Set SD bus width
 * @details     This api configures the SDIO host controller according to the
 *		bus width specified
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   bus_width - Bus width to be used (Valid values: 1/4/8)
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int sd_set_bus_width(struct device *dev, u8_t bus_width)
{
	struct sd_driver_api *api;
	api = (struct sd_driver_api *)dev->driver_api;
	return api->set_width(dev, bus_width);
}

/**
 * @brief       Set SD bus clock speed
 * @details     This api configures the SDIO clock to the specified clock speed
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]	clock_speed - Clock speed in Hz
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int sd_set_clock_speed(struct device *dev, u32_t clock_speed)
{
	struct sd_driver_api *api;
	api = (struct sd_driver_api *)dev->driver_api;
	return api->set_speed(dev, clock_speed);
}

/**
 * @brief       Register card detect callback
 * @details     This api registers a callback that will be called upon detecting
 *		an SD card insertion or removal
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]	cb - Pointer to card detect callback
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int sd_register_card_detect_cb(
			struct device *dev,
			sd_card_detect_cb cb)
{
	struct sd_driver_api *api;
	api = (struct sd_driver_api *)dev->driver_api;
	return api->register_cd_cb(dev, cb);
}

#ifdef __cplusplus
}
#endif

#endif /* __SDIO_H__ */
