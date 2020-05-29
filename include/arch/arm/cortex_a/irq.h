/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public interrupt handling
 *
 * ARM-specific kernel interrupt handling interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_IRQ_H_

#include <irq.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_IntExit);
GTEXT(z_arch_irq_enable)
GTEXT(z_arch_irq_disable)
GTEXT(z_arch_irq_is_enabled)
#else
extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);
extern int z_arch_irq_is_enabled(unsigned int irq);

extern void _IntExit(void);

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/* internal routines documented in C file, needed by IRQ_CONNECT() macro */
extern void z_irq_priority_set(unsigned int irq, unsigned int prio,
			      u32_t flags);

/* Total Number of Private Signals */
#define NUM_GIC_PPIS  (16)
#define NUM_GIC_SGIS  (16)
#define NUM_GIC_PRIVATE_SIGNALS  (32)

/* Private Peripheral Interrupt number mapping */
#define PPI_IRQ(IRQ_NUM)	(IRQ_NUM)

/* Shared Peripheral Interrupt number mapping */
#define SPI_IRQ(IRQ_NUM)	((IRQ_NUM) + 32)

/* Arch specific IRQ flags */
#define	IRQ_TRIGGER_TYPE_LEVEL	0x1
#define	IRQ_GROUP_FIQ		0x2

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * Z_ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

/* No support for direct interrupts on Cotex-A
 * All interrupt handlers are called through the sw interrupt table
 */

/* Spurious interrupt handler. Throws an error if called */
extern void z_irq_spurious(void *unused);

#ifdef CONFIG_GEN_SW_ISR_TABLE
/* Architecture-specific common entry point for interrupts from the vector
 * table. Most likely implemented in assembly. Looks up the correct handler
 * and parameter from the _sw_isr_table and executes it.
 */
extern void _isr_wrapper(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_IRQ_H_ */
