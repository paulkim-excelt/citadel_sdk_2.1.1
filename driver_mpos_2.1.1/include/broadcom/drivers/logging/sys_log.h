/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is included to avoid changing the logging code
 * in broadcom drivers and unit test code, which were developed
 * for Zephyr version 1.9.2 using the SYS_LOG_XXX macros. To allow
 * the same code to be built without changes in Zephyr v1.14, which
 * uses the LOG_XXX macros, the required macro redirections are
 * defined here. Note that for v1.9 builds, this file should not
 * be included and the sys_log.h under include/logging should be
 * included.
 */

#ifndef _SYS_LOG_H_
#define  _SYS_LOG_H_

#ifdef __ZEPHYR__
#include <version.h>
#else
#define KERNEL_VERSION_MAJOR	1
#define KERNEL_VERSION_MINOR	9
#endif

#if (KERNEL_VERSION_MAJOR == 1) && (KERNEL_VERSION_MINOR == 9)
#error "This file should not be included in zephyr v1.9 builds"
#endif

#if (KERNEL_VERSION_MAJOR == 1) && (KERNEL_VERSION_MINOR >= 14)
#include <logging/log.h>

#define LOG_MODULE_NAME general
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#ifndef CONFIG_LOG_DEFAULT_LEVEL
#define CONFIG_LOG_DEFAULT_LEVEL 2
#endif

#define SYS_LOG_INF		LOG_INF
#define SYS_LOG_DBG		LOG_DBG
#define SYS_LOG_WRN		LOG_WRN
#define SYS_LOG_ERR		LOG_ERR
#endif


#endif /* _SYS_LOG_H_ */
