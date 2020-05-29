/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file	gic.h
 * @brief	ARM GIC v2.0 interrupt controller
 *
 * Interrupt management apis for ARM Generic interrupt controller (GIC) v2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_GIC__H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_GIC__H_

/* GIC interrupt trigger types */
#define GIC_INT_TRIGGER_TYPE_LEVEL	(0)
#define GIC_INT_TRIGGER_TYPE_EDGE	(1)

/* GIC interrupt groups */
#define GIC_INT_GROUP_FIQ		(0)
#define GIC_INT_GROUP_IRQ		(1)

/**
 *
 * @brief Enable an interrupt line
 *
 * Enable the interrupt. After this call, the CPU will receive interrupts for
 * the specified <irq>. The pending and active status bits are cleared
 * for the interrupt before the interrupt is enabled. The interrupt is
 * configured as edge-triggered interrupt and is configured to be forwarded
 * to all CPU interaces.
 *
 * @param irq IRQ line
 * @return N/A
 */
void gic400_irq_enable(u32_t irq);

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified <irq>.
 *
 * @param irq IRQ line
 * @return N/A
 */
void gic400_irq_disable(u32_t irq);

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int gic400_irq_is_enabled(u32_t irq);

/**
 * @brief Set IRQ Group
 *
 * Configures the Group number for irq line
 *
 * @param irq IRQ line
 * @param group Group number : [0 - FIQ, 1 - IRQ]
 *
 * @return interrupt enable state, true or false
 */
void gic400_set_group(u32_t irq, u32_t group);

/**
 * @brief Set IRQ Trigger type
 *
 * Configures the Group number for irq line
 *
 * @param irq IRQ line
 * @param trigger Trigger type : [0 - level, 1 - edge]
 *
 * @return interrupt enable state, true or false
 */
void gic400_set_trigger_type(u32_t irq, u32_t trigger);

/**
 * @brief Set an interrupt's priority
 *
 * The GIC priority levels are used for masking interrupts with priority
 * below a threshold by programming the priority mask register (GICC_PMR).
 * In this sense the interrupt priority is not the same as a vectored interrupt
 * priority where the higher priority interrupt will be serviced before a
 * lower priority interrupt. If this kind of prioritization is required, then
 * it has to be implemented in the software isr handler.
 *
 * @param irq IRQ line
 * @param priority Priority to mask the interrupt
 *
 * @return N/A
 */
void gic400_set_priority(u32_t irq, u32_t priority);

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_GIC__H_ */
