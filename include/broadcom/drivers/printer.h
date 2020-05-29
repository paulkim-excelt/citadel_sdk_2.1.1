/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/* @file printer.h
 *
 * Printer Driver api
 *
 * This driver provides apis to access printer.
 */

#ifndef __PRINTER_H__
#define __PRINTER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*printer_api_power_control)(struct device *dev, bool enable);

typedef int (*printer_api_line_data_print)(struct device *dev,
					   struct device *spi_dev,
					   u8_t *data, u8_t length);

typedef int (*printer_api_stepper_motor_control)(struct device *dev,
						 u8_t phase);

typedef int (*printer_api_stepper_motor_run)(struct device *dev, u8_t steps);

typedef int (*printer_api_stepper_motor_disable)(struct device *dev);

struct printer_driver_api {
	printer_api_power_control power_control;
	printer_api_line_data_print line_data_print;
	printer_api_stepper_motor_control stepper_motor_control;
	printer_api_stepper_motor_run stepper_motor_run;
	printer_api_stepper_motor_disable stepper_motor_disable;
};

/**
 * @brief Power on/off the printer
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable true to on - false to off
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t printer_power_control(struct device *dev, bool enable)
{
	const struct printer_driver_api *api = dev->driver_api;

	return api->power_control(dev, enable);
}

/**
 * @brief Send dot line data to printer to print
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param spi_dev Pointer to the spi device structure
 * @param data Pointer to the dot line data
 * @param length length in bytes
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t printer_line_data_print(struct device *dev,
					   struct device *spi_dev,
					   u8_t *data, u8_t length)
{
	const struct printer_driver_api *api = dev->driver_api;

	return api->line_data_print(dev, spi_dev, data, length);
}

/**
 * @brief Set stepper motor phase
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param phase Phase to set
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t printer_stepper_motor_control(struct device *dev,
						  u8_t phase)
{
	const struct printer_driver_api *api = dev->driver_api;

	return api->stepper_motor_control(dev, phase);
}

/**
 * @brief Run the stepper motor for given steps
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param steps No of steps
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t printer_stepper_motor_run(struct device *dev, u8_t steps)
{
	const struct printer_driver_api *api = dev->driver_api;

	return api->stepper_motor_run(dev, steps);
}

/**
 * @brief Disable printer stepper motor
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t printer_stepper_motor_disable(struct device *dev)
{
	const struct printer_driver_api *api = dev->driver_api;

	return api->stepper_motor_disable(dev);
}
#endif
