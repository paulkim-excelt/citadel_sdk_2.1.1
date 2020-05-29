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

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

typedef int (*register_timer)(struct device *dev, void (*cb)(void *data),
			      void *cb_data, u32_t flags);
typedef int (*start_timer)(struct device *dev, u64_t time);
typedef void (*stop_timer)(struct device *dev);
typedef void (*suspend_timer)(struct device *dev);
typedef void (*resume_timer)(struct device *dev);
typedef u32_t (*query_timer)(struct device *dev);

struct timer_driver_api {
	register_timer init;
	start_timer start;
	stop_timer stop;
	suspend_timer suspend;
	resume_timer resume;
	query_timer query;
};

/* Flags */
#define	TIMER_NO_IRQ	(1 << 0)
#define	TIMER_ONESHOT	(1 << 1)
#define	TIMER_PERIODIC	(1 << 2)

/**
 * @brief Register/Initialize a given timer
 * @param dev Pointer to the device structure for the timer driver instance
 * @param cb A pointer the function which will be invoked when the timer pops
 * @param cb_data A pointer to any data that should be passed into the above
 *		  function
 * @param flags A bitwise collection of the above flags to be enabled in the
 *		timer
 */
static inline int timer_register(struct device *dev, void (*cb)(void *data),
				 void *cb_data, u32_t flags)
{
	const struct timer_driver_api *api = dev->driver_api;

	if (!api)
		return -EINVAL;

	return api->init(dev, cb, cb_data, flags);
}

/**
 * @brief Start a given timer
 * @param dev Pointer to the device structure for the timer driver instance
 * @param time The amount of time (in NS) for the timer to run
 */
static inline int timer_start(struct device *dev, u64_t time)
{
	const struct timer_driver_api *api = dev->driver_api;

	return api->start(dev, time);
}

/**
 * @brief Stop a given timer
 * @param dev Pointer to the device structure for the timer driver instance
 */
static inline void timer_stop(struct device *dev)
{
	const struct timer_driver_api *api = dev->driver_api;

	api->stop(dev);
}

/**
 * @brief Pause a given timer
 * @param dev Pointer to the device structure for the timer driver instance
 */
static inline void timer_suspend(struct device *dev)
{
	const struct timer_driver_api *api = dev->driver_api;

	api->suspend(dev);
}

/**
 * @brief Resume a previously suspended timer
 * @param dev Pointer to the device structure for the timer driver instance
 */
static inline void timer_resume(struct device *dev)
{
	const struct timer_driver_api *api = dev->driver_api;

	api->resume(dev);
}

/**
 * @brief Query the amount of time passed on a given timer
 * @param dev Pointer to the device structure for the timer driver instance
 */
static inline u32_t timer_query(struct device *dev)
{
	const struct timer_driver_api *api = dev->driver_api;

	return api->query(dev);
}

#endif
