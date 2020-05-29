/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  thread.c
 * @brief New thread creation for ARM Cortex-A
 *
 * Core thread related primitives for the ARM Cortex-A processor architecture.
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <wait_q.h>
#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

/**
 *
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (thread) is put at the lower address of the stack.
 *
 * <options> is currently unused.
 *
 * @param stack the aligned stack memory
 * @param stackSize stack size in bytes
 * @param pEntry the entry point
 * @param parameter1 entry point to the first param
 * @param parameter2 entry point to the second param
 * @param parameter3 entry point to the third param
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS
 *
 * @return N/A
 */

void z_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		 size_t stackSize, k_thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	struct __esf *pInitCtx;
	char *pStackMem = Z_THREAD_STACK_BUFFER(stack);

	Z_ASSERT_VALID_PRIO(priority, pEntry);

	char *stackEnd = pStackMem + stackSize;

	z_new_thread_init(thread, pStackMem, stackSize, priority, options);

	/* carve the thread entry struct from the "base" of the stack */
	pInitCtx = (struct __esf *)(STACK_ROUND_DOWN(stackEnd -
						     sizeof(struct __esf)));

	pInitCtx->pc = ((u32_t)z_thread_entry) & 0xfffffffe;
	pInitCtx->a1 = (u32_t)pEntry;
	pInitCtx->a2 = (u32_t)parameter1;
	pInitCtx->a3 = (u32_t)parameter2;
	pInitCtx->a4 = (u32_t)parameter3;
	/* Set state = thumb and mode = system */
	pInitCtx->xpsr = (u32_t)THUMB_STATE_MASK | CPU_MODE_SYS;

	/* Initialize thread's caller saved structures */
	thread->caller_saved.a1 = (u32_t)pEntry;
	thread->caller_saved.a2 = (u32_t)parameter1;
	thread->caller_saved.a3 = (u32_t)parameter2;
	thread->caller_saved.a4 = (u32_t)parameter3;
	/* _thread_entry never returns, so LR need not be initialized */
	thread->caller_saved.pc = ((u32_t)z_thread_entry) & 0xfffffffe;
	thread->caller_saved.psr = THUMB_STATE_MASK | CPU_MODE_SYS;

	/* Initialize thread's caller and callee saved structures */
	thread->callee_saved.psp = (u32_t)pInitCtx;

	/* swap_return_value can contain garbage */
	thread->arch.intlock_key = 0;

	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
}
