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

/* @file spl.h
 *
 * Secure Protection Logic Driver api
 *
 * This driver provides apis to configure the detection of possible security
 * attacks on system clock and reset inputs. Like Low frequency detect,
 * high frequency detect and no of resets issued in rapid succession.
 */

#ifndef __SPL_H__
#define __SPL_H__

#ifdef __cplusplus
extern "C" {
#endif

enum spl_int {
	SPL_WDOG_INTERRUPT,
	SPL_RST_INTERRUPT,
	SPL_PVT_INTERRUPT,
	SPL_FREQ_INTERRUPT
};

/**
 * @brief Frequency monitor structure
 *
 * low_en Enable low frequency monitor, 0/1
 * high_en Enable high frequency monitor, range 0/1
 * monitor_point Frequency monitor point, range 0...0xffff
 * low_threshold Low frequency monitor threshold, range 0...0xffff
 * high_threshold High frequency monitor threshold, range 0...0xffff
 */
struct frequency_monitor {
	bool low_en;
	bool high_en;
	u32_t monitor_point;
	u32_t low_threshold;
	u32_t high_threshold;
};

/**
 * @brief Reset monitor structure
 *
 * reset_en Enable reset monitor, 0/1
 * reset_count Reset count
 * reset_threshold Reset threshold, range 0...0xff
 * window_count Window count
 * window_threshold Window threshold, range 0...0xffffffff
 */
struct reset_monitor {
	u8_t reset_en;
	u8_t reset_count;
	u8_t reset_count_threshold;
	u32_t window_count;
	u32_t window_count_threshold;
};

typedef void (*spl_isr)(void);
typedef int (*spl_api_set_watchdog_val)(struct device *dev, u32_t val);
typedef int (*spl_api_get_watchdog_val)(struct device *dev, u32_t *val);
typedef int (*spl_api_set_calib_val) (struct device *dev, u32_t calib0,
				      u32_t calib1, u32_t calib2);
typedef int (*spl_api_get_calib_val) (struct device *dev, u32_t *calib0,
				      u32_t *calib1, u32_t *calib2);
typedef int (*spl_api_set_freq_monitor) (struct device *dev, u8_t id,
					 struct frequency_monitor *freq_mon);
typedef int (*spl_api_get_freq_monitor) (struct device *dev, u8_t id,
					 struct frequency_monitor *freq_mon);
typedef int (*spl_api_set_reset_monitor) (struct device *dev, u8_t id,
					 struct reset_monitor *reset_mon);
typedef int (*spl_api_get_reset_monitor) (struct device *dev, u8_t id,
					 struct reset_monitor *reset_mon);
typedef int (*spl_api_set_oscillator)(struct device *dev, bool val);
typedef int (*spl_api_get_oscillator)(struct device *dev, bool *val);
typedef int (*spl_api_register_isr)(struct device *dev, enum spl_int interrupt,
				    spl_isr isr);
typedef int (*spl_api_unregister_isr)(struct device *dev,
				      enum spl_int interrupt);
typedef int (*spl_api_clear_event_log)(struct device *dev);
typedef int (*spl_api_read_event_log)(struct device *dev, u8_t *high_freq,
				      u8_t *low_freq, u8_t *reset);

struct spl_driver_api {
	spl_api_set_watchdog_val set_watchdog_val;
	spl_api_get_watchdog_val get_watchdog_val;
	spl_api_set_calib_val set_calib_val;
	spl_api_get_calib_val get_calib_val;
	spl_api_set_freq_monitor set_freq_monitor;
	spl_api_get_freq_monitor get_freq_monitor;
	spl_api_set_reset_monitor set_reset_monitor;
	spl_api_get_reset_monitor get_reset_monitor;
	spl_api_set_oscillator set_oscillator_enable;
	spl_api_get_oscillator get_oscillator_enable;
	spl_api_register_isr register_isr;
	spl_api_unregister_isr unregister_isr;
	spl_api_clear_event_log clear_event_log;
	spl_api_read_event_log read_event_log;
};

/**
 * @brief Set SPL watchdog timer value
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param value Watchdog timer value
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_set_watchdog_val(struct device *dev, u32_t value)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->set_watchdog_val(dev, value);
}

/**
 * @brief   Get SPL watchdog timer value
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param value Watchdog timer value ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_get_watchdog_val(struct device *dev, u32_t *value)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->get_watchdog_val(dev, value);
}

/**
 * @brief Set SPL calibration
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param calib0 Calibration 0, range 0...0x3fffffff
 * @param calib1 Calibration 1, range 0...0x3fffffff
 * @param calib2 Calibration 2, range 0...0x7fff
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_set_calibration(struct device *dev, u32_t calib0,
					u32_t calib1, u32_t calib2)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->set_calib_val(dev, calib0, calib1, calib2);
}

/**
 * @brief Get SPL calibration.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param calib0 Calibration 0 ptr
 * @param calib1 Calibration 1 ptr
 * @param calib2 Calibration 2 ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_get_calibration(struct device *dev, u32_t *calib0,
					u32_t *calib1, u32_t *calib2)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->get_calib_val(dev, calib0, calib1, calib2);
}

/**
 * @brief Set SPL frequency monitor.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Frequency monitor id
 * @param freq_mon Frequency monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_set_freq_monitor(struct device *dev, u8_t id,
					 struct frequency_monitor *freq_mon)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->set_freq_monitor(dev, id, freq_mon);
}

/**
 * @brief Get SPL frequency monitor.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Frequency monitor id
 * @param freq_mon Frequency monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_get_freq_monitor(struct device *dev, u8_t id,
					 struct frequency_monitor *freq_mon)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->get_freq_monitor(dev, id, freq_mon);
}

/**
 * @brief Set Reset monitor
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Reset monitor id
 * @param reset_mon Reset monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_set_reset_monitor(struct device *dev, u8_t id,
					  struct reset_monitor *reset_mon)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->set_reset_monitor(dev, id, reset_mon);
}

/**
 * @brief Get Reset monitor
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Reset monitor id
 * @param reset_mon Reset monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_get_reset_monitor(struct device *dev, u8_t id,
					  struct reset_monitor *reset_mon)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->get_reset_monitor(dev, id, reset_mon);
}

/**
 * @brief Set oscillator enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable Enable SPL oscillator, range 0.1
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_set_oscillator_enable(struct device *dev, bool enable)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->set_oscillator_enable(dev, enable);
}

/**
 * @brief Get oscillator enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable Ptr to get
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_get_oscillator_enable(struct device *dev, bool *enable)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->get_oscillator_enable(dev, enable);
}

/**
 * @brief Register SPL interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param spl_int Interrupt to register
 * @param handler Interrupt handler
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_register_isr(struct device *dev, enum spl_int intr,
				     spl_isr handler)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->register_isr(dev, intr, handler);
}

/**
 * @brief Unregister SPL interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param spl_int Interrupt to unregister
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_unregister_isr(struct device *dev, enum spl_int intr)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->unregister_isr(dev, intr);
}

/**
 * @brief Clear event log
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_clear_event_log(struct device *dev)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->clear_event_log(dev);
}

/**
 * @brief Register SPL interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param high_freq High frequency event log
 * @param low_freq Low frequency event log
 * @param reset Reset event log
 *
 * @retval 0 for success, error otherwise
 */
static inline s32_t spl_read_event_log(struct device *dev, u8_t *high_freq,
			 u8_t *low_freq, u8_t *reset)
{
	const struct spl_driver_api *api = dev->driver_api;

	return api->read_event_log(dev, high_freq, low_freq, reset);
}

#endif
