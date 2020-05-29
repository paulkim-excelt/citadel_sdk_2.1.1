/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public error handling
 *
 * ARM-specific kernel error handling interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_ERROR_H_

#include <arch/arm/cortex_a/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void z_NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);
extern void z_SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);
#endif

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_KERNEL_OOPS (4)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (5)	/* Kernel panic (fatal to system) */

/* IRQ/FIQ enable/disable service calls */
#define _SVC_CALL_CPSID_I		0xF0
#define _SVC_CALL_CSPID_F		0xF1
#define _SVC_CALL_CPSID_IF		0xF2
#define _SVC_CALL_CPSIE_I		0xF3
#define _SVC_CALL_CPSIE_F		0xF4
#define _SVC_CALL_CPSIE_IF		0xF5

#if defined(CONFIG_ARMV7_A)
/* On ARMv7A SVC and IRQ/FIQ are different exceptions, so svc #NUM instruction
 * to initiate a context switch will still trigger an SVC exception even if
 * interrupts (FIQ, IRQ) are disabled.
 */
#define Z_ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile ( \
		"mov r0, %[reason]\n\t" \
		"svc %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), [id] "i" (_SVC_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
	CODE_UNREACHABLE; \
} while (0)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_ERROR_H_ */
