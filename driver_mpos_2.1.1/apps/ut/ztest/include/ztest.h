/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing suite
 */

#ifndef __ZTEST_H__
#define __ZTEST_H__

/**
 * @defgroup ztest Zephyr testing suite
 */

#if !defined(CONFIG_ZTEST) && !defined(ZTEST_UNITTEST)
#error "You need to add CONFIG_ZTEST to your config file."
#endif

#define CONFIG_ZTEST_ASSERT_VERBOSE 1

#include <misc/printk.h>
#define PRINT printk

#include <zephyr/types.h>
#include <ztest_assert.h>
#include <ztest_test.h>
#include <tc_util.h>
#include <misc/util.h>
#include <stdbool.h>

#endif /* __ZTEST_H__ */
