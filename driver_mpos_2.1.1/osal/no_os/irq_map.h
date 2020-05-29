/*
 * Copyright (c) 2018, Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file irq_map.h
 *
 * Maps the exception handlers for the OS
 *
 */

#define os_svc_handler default_exc_handler
#define os_fiq_handler default_exc_handler
#define os_sys_tick_handler sys_tick_handler

#ifdef CONFIG_ENABLE_IRQ_FOR_NO_OS
	#define os_irq_handler __no_os_irq_handler
#else
	#define os_irq_handler default_exc_handler
#endif /* CONFIG_ENABLE_IRQ_FOR_NO_OS */
