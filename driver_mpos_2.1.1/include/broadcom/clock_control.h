/* clock_control.h - public clock controller driver API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CLOCK_CONTROL_H__
#define __CLOCK_CONTROL_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <errno.h>
#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Clock control API */

/* Used to select all subsystem of a clock controller */
#define CLOCK_CONTROL_SUBSYS_ALL	NULL

/* Index 0: New Method, 1: Old Method */
struct clock_params {
	u32_t mdiv[2];
	u32_t pdiv[2];
	u32_t ndivint[2];
	u32_t ndivfrac[2];
	u64_t fvco[2];
	u32_t resrate[2];
	u32_t error[2];
	/* the following status are filled in after applying the
	 * desired parameters.
	 */
	u32_t effmdiv; /* Effective mdiv that HW finally applied */
	u32_t effpdiv; /* Effective pdiv that HW finally applied */
	u32_t effndivint; /* Effective ndivint that HW finally applied */
	u64_t efffvco; /* Effective Resultant fvco that is expected */
	u32_t effresrate; /* Effective Resultant CPU clock that is expected */
};

/**
 * clock_control_subsys_t is a type to identify a clock controller sub-system.
 * Such data pointed is opaque and relevant only to the clock controller
 * driver instance being used.
 */
typedef void *clock_control_subsys_t;

typedef int (*clock_control)(struct device *dev, clock_control_subsys_t sys);

typedef int (*clock_control_get)(struct device *dev,
				 clock_control_subsys_t sys,
				 u32_t *rate);

typedef int (*clock_control_set)(struct device *dev,
				 clock_control_subsys_t sys,
				 u32_t rate);

typedef int (*clock_control_compare)(struct device *dev, const u32_t rate_mhz,
				struct clock_params *p);

typedef int (*clock_control_get_effective_params)(struct device *dev,
				struct clock_params *p, u32_t method);

typedef int (*clock_control_apply)(struct device *dev,
			  u32_t mdiv, u32_t pdiv, u32_t ndiv_int, u32_t ndiv_frac);

struct clock_control_driver_api {
	clock_control				on;
	clock_control				off;
	clock_control_get			get_rate;
	clock_control_set			set_rate;
	clock_control_compare			compare;
	clock_control_get_effective_params 	get_effective_params;
	clock_control_apply			apply;
};

/**
 * @brief Enable the clock of a sub-system controlled by the device
 * @param dev Pointer to the device structure for the clock controller driver
 * 	instance
 * @param sys A pointer to an opaque data representing the sub-system
 */
static inline int clock_control_on(struct device *dev,
				   clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	return api->on(dev, sys);
}

/**
 * @brief Disable the clock of a sub-system controlled by the device
 * @param dev Pointer to the device structure for the clock controller driver
 * 	instance
 * @param sys A pointer to an opaque data representing the sub-system
 */
static inline int clock_control_off(struct device *dev,
				    clock_control_subsys_t sys)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	return api->off(dev, sys);
}

/**
 * @brief Obtain the clock rate of given sub-system
 * @param dev Pointer to the device structure for the clock controller driver
 *        instance
 * @param sys A pointer to an opaque data representing the sub-system
 * @param[out] rate Subsystem clock rate
 */
static inline int clock_control_get_rate(struct device *dev,
					 clock_control_subsys_t sys,
					 u32_t *rate)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	__ASSERT(api->get_rate, "%s not implemented for device %s",
		__func__, dev->config->name);

	return api->get_rate(dev, sys, rate);
}

/**
 * @brief Set the clock rate of given sub-system
 * @param dev Pointer to the device structure for the clock controller driver
 *        instance
 * @param sys A pointer to an opaque data representing the sub-system
 * @param rate Subsystem clock rate
 */
static inline int clock_control_set_rate(struct device *dev,
					 clock_control_subsys_t sys,
					 u32_t rate)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	if (!api->set_rate)
		return -EACCES;

	return api->set_rate(dev, sys, rate);
}

static inline int compare_clk_params(struct device *dev,
			const u32_t rate_mhz, struct clock_params *p)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	if (!api->compare)
		return -EACCES;

	return api->compare(dev, rate_mhz, p);
}

static inline int get_effective_clk_params(struct device *dev,
			struct clock_params *p, u32_t method)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	if (!api->get_effective_params)
		return -EACCES;

	return api->get_effective_params(dev, p, method);
}


static inline int apply_clk_params(struct device *dev,
			 u32_t mdiv, u32_t pdiv, u32_t ndiv_int, u32_t ndiv_frac)
{
	const struct clock_control_driver_api *api = dev->driver_api;

	if (!api->apply)
		return -EACCES;

	return api->apply(dev, mdiv, pdiv, ndiv_int, ndiv_frac);
}

#ifdef __cplusplus
}
#endif

#endif /* __CLOCK_CONTROL_H__ */
