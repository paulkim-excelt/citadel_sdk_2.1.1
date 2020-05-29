/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A k_thread_abort() routine
 *
 * The ARM Cortex-A architecture provides its own k_thread_abort() to deal
 * with different CPU modes (IRQ/FIQ/SYS/SVC/UND) when a thread aborts. When its
 * entry point returns or when it aborts itself, the CPU is in User mode and
 * must call z_swap() (which triggers a service call), but when in handler
 * mode, the context switch occurs at the end of the exception handler. All
 * exception handlers for Cortex-A make a tail call to SVC #0 to initiate a
 * context switch.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <ksched.h>
#include <kswap.h>
#include <wait_q.h>
#include <cortex_a/exc.h>

extern void z_thread_single_abort(struct k_thread *thread);

void z_impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;

	key = irq_lock();

	z_thread_single_abort(thread);
	z_thread_monitor_exit(thread);

	if (_current == thread) {
		if (z_GetCpuMode() == CPU_MODE_SYS) {
			/* Call Swap if in System mode */
			z_swap_irqlock(key);
			CODE_UNREACHABLE;
		} else {
			/* If we are not in system mode, then we must be in
			 * in an exception handler. And if we are in an
			 * exception, then exc_exit will always call
			 * svc #0 if a context switch is needed
			 */
		}
	}

	/* The abort handler might have altered the ready queue. */
	z_reschedule_irqlock(key);
}
