/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A Stack setup
 *
 *
 * Setup stacks for various CPU modes
 */

#include <cortex_a/stack.h>

/* _interrupt_stack is defined in kernel/init.c
 * This stack is used in IRQ mode and re-used as
 * initial system stack. The is safe since interrupts are not
 * enabled until after the kernel switches to the init thread.
 *
 * The other (mode specific) stacks that are specific to
 * ARM A7 are defined here.
 */

K_THREAD_STACK_DEFINE(_fiq_stack, CONFIG_ISR_STACK_SIZE);
K_THREAD_STACK_DEFINE(_svc_stack, CONFIG_SVC_STACK_SIZE);
K_THREAD_STACK_DEFINE(_abt_stack, CONFIG_ABT_STACK_SIZE);
K_THREAD_STACK_DEFINE(_und_stack, CONFIG_UND_STACK_SIZE);

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-A, the stack pointer is stored in the banked SP_XXX
 * register based on the CPU mode (cpsr.m) and switched to automatically
 * when taking an exception.
 *
 * @return N/A
 */
void z_InterruptStackSetup(void)
{
	u32_t sp;

	sp = (u32_t)(Z_THREAD_STACK_BUFFER(_interrupt_stack) +
		     CONFIG_ISR_STACK_SIZE);
	z_IrqStackSet(sp);

	sp = (u32_t)(Z_THREAD_STACK_BUFFER(_fiq_stack) + CONFIG_ISR_STACK_SIZE);
	z_FiqStackSet(sp);

	sp = (u32_t)(Z_THREAD_STACK_BUFFER(_svc_stack) + CONFIG_SVC_STACK_SIZE);
	z_SvcStackSet(sp);

	sp = (u32_t)(Z_THREAD_STACK_BUFFER(_abt_stack) + CONFIG_ABT_STACK_SIZE);
	z_AbtStackSet(sp);

	sp = (u32_t)(Z_THREAD_STACK_BUFFER(_und_stack) + CONFIG_ABT_STACK_SIZE);
	z_UndStackSet(sp);
}
