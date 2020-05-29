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

/* @file qspi_flash.h
 *
 * QSPI flash Driver api
 *
 * This driver provides apis to access the serial
 * flash device
 *
 */

#ifndef __QSPI_FLASH_H__
#define __QSPI_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/** All platform specific headers are included here */
#include <errno.h>
#include <stddef.h>
#include <device.h>

/* QSPI operation mode bit fields */
#define QSPI_CLOCK_POL(MODE)	((MODE) & 0x1)
#define QSPI_CLOCK_PHASE(MODE)	(((MODE) & 0x2) >> 1)
#define QSPI_DATA_LANE(MODE)	(((MODE) & 0xC) >> 2)

/* QSPI data lane mode */
#define QSPI_MODE_SINGLE	0x0	/**< Standard SPI operation */
#define QSPI_MODE_DUAL		0x1	/**< Dual SPI operation */
#define QSPI_MODE_QUAD		0x2	/**< Quad SPI operation */

/* Helper macros to set qspi op bit fields */
#define SET_QSPI_CLOCK_POL(OPMODE, VAL)		\
	do {					\
		(OPMODE) &= ~(0x1UL);		\
		(OPMODE) |= ((VAL) & 0x1);	\
	} while (0)

#define SET_QSPI_CLOCK_PHASE(OPMODE, VAL)	\
	do {					\
		(OPMODE) &= ~(0x2UL);		\
		(OPMODE) |= ((VAL) & 0x1) << 0x1;\
	} while (0)

#define SET_QSPI_DATA_LANE(OPMODE, VAL)		\
	do {					\
		(OPMODE) &= ~(0xCUL);		\
		(OPMODE) |= ((VAL) & 0x3) << 0x2;\
	} while (0)

/**
 * @brief   QSPI flash configuration
 * @details Configuration info for the serial flash device
 *
 * frequency - QSPI clock speed in Hz
 * op_mode   - QSPI Operation mode (bitmap)
 *              Clock Polarity [ 0 ]
 *                  1: indicates that the inactive state of SCK is 1
 *                  0: indicates that the inactive state of SCK is 0
 *              Clock Phase    [ 1 ]
 *                  1: indicates that data is transmitted on the leading
 *                     edge of SCK and captured on the falling edge of SCK.
 *                  0: indicates that data is captured on the leading
 *                     edge of SCK and transmitted on the falling edge of SCK
 *              Data lines     [ 3 : 2]
 *                  0:  Single lane
 *                  1:  Dual lane
 *                  2:  Quad lane
 *
 * slave_num -  Slave number for the flash device
 */
struct qspi_flash_config {
	u32_t    frequency;
	u8_t     op_mode;
	u8_t     slave_num;
};


/**
 * @brief   QSPI flash device information
 * @details Device info for the serial flash
 *
 * flash_size - Flash memory size in bytes
 * min_erase_sector_size   - Minimum sector size that can be erased
 *
 */
struct qspi_flash_info {
	char     *name;
	u32_t    flash_size;
	u32_t    min_erase_sector_size;
};

/**
 * @typedef qspi_flash_api_configure
 * @brief Callback API for configuring flash driver
 * See qspi_flash_configure() for argument descriptions
 */
typedef int (*qspi_flash_api_configure)(
				struct device *dev,
				struct qspi_flash_config *config);

/**
 * @typedef qspi_flash_api_get_device_info
 * @brief Callback API for retrieving flash device info
 * See qspi_flash_get_device_info() for argument descriptions
 */
typedef int (*qspi_flash_api_get_device_info)(
				struct device *dev,
				struct qspi_flash_info *info);

/**
 * @typedef qspi_flash_api_read
 * @brief Callback API for reading the flash
 * See qspi_flash_read() for argument descriptions
 */
typedef int (*qspi_flash_api_read)(
				struct device *dev,
				u32_t address,
				u32_t data_len,
				u8_t *data);

/**
 * @typedef qspi_flash_api_erase
 * @brief Callback API for erasing a flash memory block
 * See qspi_flash_erase() for argument descriptions
 */
typedef int (*qspi_flash_api_erase)(
				struct device *dev,
				u32_t address,
				u32_t len);

/**
 * @typedef qspi_flash_api_write
 * @brief Callback API for writing to flash device
 * See qspi_flash_write() for argument descriptions
 */
typedef int (*qspi_flash_api_write)(
				struct device *dev,
				u32_t address,
				u32_t data_len,
				u8_t *data);

/**
 * @typedef qspi_flash_api_chip_erase
 * @brief Callback API for erasing the entire flash memory
 * See qspi_flash_chip_erase() for argument descriptions
 */
typedef int (*qspi_flash_api_chip_erase)(struct device *dev);

/**
 * @brief QSPI flash driver API
 */
struct qspi_flash_driver_api {
	qspi_flash_api_configure configure;
	qspi_flash_api_get_device_info get_devinfo;
	qspi_flash_api_read read;
	qspi_flash_api_erase erase;
	qspi_flash_api_write write;
	qspi_flash_api_chip_erase chip_erase;
};

/**
 * @brief       Set flash device config
 * @details     This api sets the serial flash configuration for the
 *              specified flash device.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to flash device configuration
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int qspi_flash_configure(
			struct device *dev,
			struct qspi_flash_config *config)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->configure(dev, config);
}

/**
 * @brief       Get flash device Info
 * @details     This api gets the serial flash device info
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[out]  info - Pointer to flash device info to be populated
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int qspi_flash_get_device_info(
			struct device *dev,
			struct qspi_flash_info *info)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->get_devinfo(dev, info);
}

/**
 * @brief       Read the flash memory
 * @details     This api reads data from the flash device at specified offset
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - flash offset to read the data from
 * @param[in]   data_len - number of bytes to read
 * @param[out]  data - buffer to read the data to
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static inline int qspi_flash_read(
			struct device *dev,
			u32_t address,
			u32_t data_len,
			u8_t *data)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->read(dev, address, data_len, data);
}

/**
 * @brief       Erase serial flash
 * @details     This api erases the flash sectors specified by the address
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - Address of the sector that needs to be erased. Note
 *                        that address should be aligned to the sector boundary
 * @param[in]   len - Number of bytes to be erased. Note length needs to be a
 *                    multiple of the flash sector size
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static inline int qspi_flash_erase(
			struct device *dev,
			u32_t address,
			u32_t len)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->erase(dev, address, len);
}

/**
 * @brief       Write to the flash memory
 * @details     This api writes data to the flash device at the specified offset
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - flash offset to write the data to
 * @param[in]   data_len - number of bytes to write
 * @param[in]   data - Data to be written to flash
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static inline int qspi_flash_write(
			struct device *dev,
			u32_t address,
			u32_t data_len,
			u8_t *data)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->write(dev, address, data_len, data);
}

/**
 * @brief       Erase entire serial flash
 * @details     This api erases the complete serial flash
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static inline int qspi_flash_chip_erase(
			struct device *dev)
{
	struct qspi_flash_driver_api *api;
	api = (struct qspi_flash_driver_api *)dev->driver_api;
	return api->chip_erase(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* __QSPI_FLASH_H__ */
