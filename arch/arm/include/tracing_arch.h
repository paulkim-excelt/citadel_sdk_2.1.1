/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for ARM
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_TRACING_ARCH_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_TRACING_ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CPU_CORTEX_M
#include "arch/arm/cortex_m/cmsis.h"
#endif

/**
 * @brief Get the identification of the current interrupt/mode.
 *
 * On Cortex-M, this routine obtain the key of the interrupt that is currently
 * processed if it is called from a ISR context.
 *
 * Cortex-A does not implement a vectored interrupt controller, so this routine
 * just returns a key that indicates if an FIQ or and IRQ exception is being
 * processed. The key returned is 1 for IRQ and 2 for FIQ and 0 otherwise.
 *
 * @return The key of the interrupt that is currently being processed.
 */
int z_sys_current_irq_key_get(void)
{
#ifdef CONFIG_CPU_CORTEX_M
	return __get_IPSR();
#elif defined CONFIG_CPU_CORTEX_A
	switch (z_GetCpuMode()) {
	case CPU_MODE_IRQ:
		return 1;
	case CPU_MODE_FIQ:
		return 2;
	default:
		return 0;
	}
#else
	return 0;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_TRACING_ARCH_H_ */
