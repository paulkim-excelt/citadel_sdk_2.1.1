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

#ifndef __WDT_H__
#define __WDT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

typedef void (*wdt_isr)(void);

typedef int (*wdt_api_start)(struct device *dev, u32_t time_ms);
typedef int (*wdt_api_feed)(struct device *dev);
typedef int (*wdt_api_stop)(struct device *dev);
typedef int (*wdt_api_register_isr)(struct device *dev, wdt_isr handler);
typedef int (*wdt_api_unregister_isr)(struct device *dev);
typedef int (*wdt_api_reset_en)(struct device *dev, bool flag);

struct wdt_driver_api {
	wdt_api_start start;
	wdt_api_feed feed;
	wdt_api_stop stop;
	wdt_api_register_isr register_isr;
	wdt_api_unregister_isr unregister_isr;
	wdt_api_reset_en reset_en;
};

/**
 * @brief   Start watchdog.
 * @details Start watchdog.
 * @param dev: Pointer to device structure
 * @param time_ms: Watchdog timeout value.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_start(struct device *dev, u32_t time_ms)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->start(dev, time_ms);
}

/**
 * @brief   Feed watchdog.
 * @details Keep the watchdog counter alive by reloading WDOGLOAD register.
 * @param dev: Pointer to device structure
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_feed(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->feed(dev);
}

/**
 * @brief   Stop watchdog.
 * @details Stop watchdog.
 * @param dev: Pointer to device structure
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_stop(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->stop(dev);
}

/**
 * @brief   Register watchdog interrupt handler.
 * @details Register watchdog interrupt handler.
 * @param dev: Pointer to device structure
 * @param handler: Watchdog interrupt handler.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_register_isr(struct device *dev, wdt_isr handler)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->register_isr(dev, handler);
}

/**
 * @brief   Unregister watchdog interrupt handler.
 * @details Unregister watchdog interrupt handler.
 * @param dev: Pointer to device structure
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_unregister_isr(struct device *dev)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->unregister_isr(dev);
}

/**
 * @brief   Enable/disable watchdog reset.
 * @details Enable/disable watchdog reset.
 * @param dev: Pointer to device structure
 * @param flag: (true/false).
 *	If this flag is true, the watchdog timer reset is enabled
 *	in the watchdog control register.
 *	Otherwise watchdog expiry would just result in interrupt, which
 *	would then reload decrementing counter. If the interrupt is not
 *	cleared before the counter becomes 0, the counter would just
 *	stop and reset would not be triggered.
 * @retval  0:OK
 * @retval  -1:ERROR
 */
static inline int wdt_reset_en(struct device *dev, bool flag)
{
	const struct wdt_driver_api *api = dev->driver_api;

	return api->reset_en(dev, flag);
}
#endif
