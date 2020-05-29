/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A interrupt management
 *
 *
 * Interrupt management: enabling/disabling Interrupts.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <misc/printk.h>
#include <kernel_structs.h>
#include <arch/arm/cortex_a/error.h>

#ifdef CONFIG_ARM_GIC_400
#include <cortex_a/gic400.h>
#endif


/**
 *
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 * @internal
 *
 * On Cortex-A7, this function sets the IRQ disable and FIQ disable bits
 * and returns these bit values prior to setting them.
 *
 */

unsigned int z_arch_irq_lock(void)
{
	unsigned int key;

#if defined(CONFIG_ARMV7_A)
	if (z_GetCpuMode() == CPU_MODE_USR) {
		__asm__ volatile("mrs %0, cpsr;"
			"svc %[id]"
			: "=r" (key)
			: [id] "i" (_SVC_CALL_CPSID_IF)
			: "memory", "cc");
	} else {
		__asm__ volatile("mrs %0, cpsr;"	/* Read CPSR to key */
			"cpsid if"			/* Disable IRQ & FIQ */
			: "=r" (key)
			:
			: "memory", "cc");
	}
	/* Return the IRQ/FIQ disable status bits */
	key &= (IRQ_DISABLE_MASK | FIQ_DISABLE_MASK);
#else
#warning Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */

	return key;
}


/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter is an
 * architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @param key architecture-dependent lock-out key
 *
 * @return N/A
 *
 * On Cortex A7, this function restores the IRQ and FIQ disable bits to the
 * values before the corresponding irq_lock() call.
 *
 */

void z_arch_irq_unlock(unsigned int key)
{
#if defined(CONFIG_ARMV7_A)
	if (z_GetCpuMode() == CPU_MODE_USR) {
		if ((key & IRQ_DISABLE_MASK) == 0x0)
			__asm__ volatile("svc %[id]"
				: : [id] "i" (_SVC_CALL_CPSIE_I)
				: "memory", "cc");
		if ((key & FIQ_DISABLE_MASK) == 0x0)
			__asm__ volatile("svc %[id]"
				: : [id] "i" (_SVC_CALL_CPSIE_F)
				: "memory", "cc");
	} else {
		/* (IRQ was enabled (IRQ disable bit = 0) at irq_lock() */
		if ((key & IRQ_DISABLE_MASK) == 0x0)
			__asm__ volatile("cpsie i");	/* Enable IRQ back */

		/* FIQ was enabled (FIQ disable bit = 0) at irq_lock() */
		if ((key & FIQ_DISABLE_MASK) == 0x0)
			__asm__ volatile("cpsie f");	/* Enable FIQ back */
	}
#else
#warning Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */
}

/**
 *
 * @brief Enable an interrupt line
 *
 * Enable the interrupt. After this call, the CPU will receive interrupts for
 * the specified <irq>.
 *
 * @return N/A
 */
void z_arch_irq_enable(unsigned int irq)
{
#ifdef CONFIG_ARM_GIC_400
	gic400_irq_enable(irq);
#else
	#error "Unspported interrupt controller"
#endif
}

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified <irq>.
 *
 * @return N/A
 */
void z_arch_irq_disable(unsigned int irq)
{
#ifdef CONFIG_ARM_GIC_400
	gic400_irq_disable(irq);
#else
	#error "Unspported interrupt controller"
#endif
}

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int z_arch_irq_is_enabled(unsigned int irq)
{
#ifdef CONFIG_ARM_GIC_400
	return gic400_irq_is_enabled(irq);
#else
	#error "Unspported interrupt controller"
#endif
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * @return N/A
 */
void z_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
#ifdef CONFIG_ARM_GIC_400
	if (flags & IRQ_TRIGGER_TYPE_LEVEL)
		gic400_set_trigger_type(irq, GIC_INT_TRIGGER_TYPE_LEVEL);
	else
		gic400_set_trigger_type(irq, GIC_INT_TRIGGER_TYPE_EDGE);

	if (flags & IRQ_GROUP_FIQ)
		gic400_set_group(irq, GIC_INT_GROUP_FIQ);
	else
		gic400_set_group(irq, GIC_INT_GROUP_IRQ);

	return gic400_set_priority(irq, prio);
#else
	#error "Unspported interrupt controller"
#endif
}

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * @return N/A
 */
void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("Spurious interrupt detected. Spinning...\n");
	while (1)
		;
}

/**
 * brief Interrupt Initializations
 *
 * Nothing to be done for Cortex-A. This api is called in prepare_multithreading
 * in kernel/init.c
 */
void _IntLibInit(void)
{
     /* nothing needed, here because the kernel requires it */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
