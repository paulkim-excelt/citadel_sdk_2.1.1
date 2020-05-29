/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-A CPUs
 *
 * Stack helper functions.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_STACK__H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_STACK__H_

#ifndef _ASMLANGUAGE
/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-A, the stack pointer is stored in the banked SP_XXX
 * register based on the CPU mode (cpsr.m) and switched to automatically
 * when taking an exception.
 *
 * This function is declared before the #includes, because kernel_arch_func.h
 * uses it.
 *
 * @return N/A
 */
extern void z_InterruptStackSetup(void);
#endif

#include <kernel_structs.h>
#include <asm_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Unless otherwise defined svc stack size is same as isr stack size */
#ifndef CONFIG_SVC_STACK_SIZE
#define CONFIG_SVC_STACK_SIZE	CONFIG_ISR_STACK_SIZE
#endif

/* Unless otherwise defined abt stack size is same as isr stack size */
#ifndef CONFIG_ABT_STACK_SIZE
#define CONFIG_ABT_STACK_SIZE	CONFIG_ISR_STACK_SIZE
#endif

/* Unless otherwise defined undef mode stack size is same as isr stack size */
#ifndef CONFIG_UND_STACK_SIZE
#define CONFIG_UND_STACK_SIZE	CONFIG_ISR_STACK_SIZE
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_fiq_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_svc_stack, CONFIG_SVC_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_abt_stack, CONFIG_ABT_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_und_stack, CONFIG_UND_STACK_SIZE);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_STACK__H_ */
