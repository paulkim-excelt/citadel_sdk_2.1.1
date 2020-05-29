/*
 * Copyright (c) 2019 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-A
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-A CPUs.
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>

#ifdef CONFIG_EXECUTION_BENCHMARKING
extern void read_timer_start_of_swap(void);
#endif
extern const int _k_neg_eagain;

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The __swap() routine is invoked by various kernel services to effect
 * a cooperative context context switch.  Prior to invoking __swap(), the caller
 * disables interrupts via irq_lock() and the return 'key' is passed as a
 * parameter to __swap().  The 'key' actually represents the CPSR.I/F/A/Mode
 * bits prior to disabling interrupts.
 *
 * __swap() itself does not do much.
 *
 * It simply stores the int lock key parameter into current->irq_lock_key,
 * and then triggers a service call exception (svc) which does the heavy lifting
 * of context switching.
 *
 * Since Cortex-A does not generate an interrupt stack of caller saved regs, as
 * is the case with Cortex-M, all the caller saved and callee saved registers
 * are saved to the thread arch object in the SVC handler.
 *
 * Given that __swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the thread of the
 * outgoing thread.
 *
 * @return may contain a return value setup by a call to
 * _set_thread_return_value()
 *
 */
int __swap(int key)
{
#ifdef CONFIG_EXECUTION_BENCHMARKING
	read_timer_start_of_swap();
#endif

	/* Save key */
	_current->arch.intlock_key = key;

	/* Set __swap()'s default return code to -EAGAIN. This eliminates the
	 * need for the timeout code to set it itself.
	 */
	_current->arch.swap_return_value = _k_neg_eagain;

	/* Trigger call to __svc */
	__asm__ volatile ("svc #0\n");

	irq_unlock(key);

	return _current->arch.swap_return_value;
}

