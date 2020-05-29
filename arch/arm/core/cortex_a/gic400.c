/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file	gic400.h
 * @brief	ARM GIC 400 interrupt controller apis
 *
 * Interrupt management apis for configuring ARM Generic interrupt controller
 * (GIC) 400, which implements ARM GIC v2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(gic);

#include <arch/cpu.h>
#include <zephyr/types.h>
#include <toolchain.h>
#include <init.h>
#include <kernel.h>
#include <misc/util.h>
#include <cortex_a/gic400.h>
#include <asm_inline.h>
#include <soc.h>
#include <tracing.h>

#define CONFIG_NUM_GIC_INTERRUPTS	CONFIG_NUM_IRQS

/* GIC registers */
#define GICD_CTLR		(GIC_GICD_BASE_ADDR + 0x000)
#define GICD_TYPER		(GIC_GICD_BASE_ADDR + 0x004)
#define GICD_IIDR		(GIC_GICD_BASE_ADDR + 0x008)
#define GICD_IGROUPR(n)		(GIC_GICD_BASE_ADDR + 0x0080 + ((n) * 4))
#define GICD_ISENABLER(n)	(GIC_GICD_BASE_ADDR + 0x0100 + ((n) * 4))
#define GICD_ICENABLER(n)	(GIC_GICD_BASE_ADDR + 0x0180 + ((n) * 4))
#define GICD_ISPENDR(n)		(GIC_GICD_BASE_ADDR + 0x0200 + ((n) * 4))
#define GICD_ICPENDR(n)		(GIC_GICD_BASE_ADDR + 0x0280 + ((n) * 4))
#define GICD_ICACTIVER(n)	(GIC_GICD_BASE_ADDR + 0x0380 + ((n) * 4))
#define GICD_IPRIORITYR(n)	(GIC_GICD_BASE_ADDR + 0x0400 + ((n) * 4))
#define GICD_ITARGETSR(n)	(GIC_GICD_BASE_ADDR + 0x0800 + ((n) * 4))
#define GICD_ICFGR(n)		(GIC_GICD_BASE_ADDR + 0x0C00 + ((n) * 4))
#define GICD_SGIR(n)		(GIC_GICD_BASE_ADDR + 0x0F00 + ((n) * 4))

#define GICC_CTLR		(GIC_GICC_BASE_ADDR + 0x0000)
#define GICC_PMR		(GIC_GICC_BASE_ADDR + 0x0004)
#define GICC_BPR		(GIC_GICC_BASE_ADDR + 0x0008)
#define GICC_IAR		(GIC_GICC_BASE_ADDR + 0x000C)
#define GICC_EOIR		(GIC_GICC_BASE_ADDR + 0x0010)
#define GICC_RPR		(GIC_GICC_BASE_ADDR + 0x0014)
#define GICC_HPPIR		(GIC_GICC_BASE_ADDR + 0x0018)
#define GICC_ABPR		(GIC_GICC_BASE_ADDR + 0x001C)
#define GICC_IIDR		(GIC_GICC_BASE_ADDR + 0x00FC)

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_isr(void);
	extern void read_timer_end_of_isr(void);
#endif

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
void gic400_irq_enable(u32_t irq)
{
	u32_t addr, val, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return;

	bit_pos = BIT_FROM_IRQ(irq);

	/* Targeting to all CPU interface
	 * 8 bits per interrupt
	 */
	addr = GICD_ITARGETSR(irq / 4);
	val = sys_read32(addr);
	val |=  0xFFUL << (bit_pos % 4) * 8;
	sys_write32(val, addr);

	/* Set interrupt enable bit
	 * - Write 0 has no effect
	 */
	addr = GICD_ISENABLER(REG_FROM_IRQ(irq));
	sys_write32(BIT(bit_pos), addr);
}

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
void gic400_irq_disable(u32_t irq)
{
	u32_t addr, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return;

	bit_pos = BIT_FROM_IRQ(irq);

	addr = GICD_ICENABLER(REG_FROM_IRQ(irq));
	sys_write32(BIT(bit_pos), addr);
}

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int gic400_irq_is_enabled(u32_t irq)
{
	u32_t addr, val, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return false;

	bit_pos = BIT_FROM_IRQ(irq);

	addr = GICD_ISENABLER(REG_FROM_IRQ(irq));
	val = sys_read32(addr);

	return !!(val & BIT(bit_pos));
}

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
void gic400_set_group(u32_t irq, u32_t group)
{
	u32_t addr, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return;

	bit_pos = BIT_FROM_IRQ(irq);

	/* Set interrupt group to group 0 */
	addr = GICD_IGROUPR(REG_FROM_IRQ(irq));
	if (group == 0x0)
		sys_clear_bit(addr, bit_pos);	/* Group 0 for FIQ */
	else
		sys_set_bit(addr, bit_pos);	/* Group 1 for IRQ */
}

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
void gic400_set_trigger_type(u32_t irq, u32_t trigger)
{
	u32_t addr, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return;

	/* 2 bits per interrupt => 16 itnerrupt cfg bits per register
	 * MSBit for edge/level sensitive config (bit pos - [2n+1])
	 */
	bit_pos = ((irq % 16) * 2) + 1;

	/* Setup interrupt as edge/level triggered */
	addr = GICD_ICFGR(irq / 16);
	if (trigger == 0)
		sys_clear_bit(addr, bit_pos);
	else
		sys_set_bit(addr, bit_pos);
}

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
void gic400_set_priority(u32_t irq, u32_t priority)
{
	u32_t addr, val, bit_pos;

	if (irq >= CONFIG_NUM_GIC_INTERRUPTS)
		return;

	/* 8 bits per interrupt (4 interrupt priorities per reg) */
	bit_pos = (irq % 4) * 8;

	/* Clear any Pending and Active bits for the interrupt
	 * before setting the priority
	 * - Write 0 has no effect
	 */
	addr = GICD_ICPENDR(REG_FROM_IRQ(irq));
	sys_write32(BIT(bit_pos), addr);
	addr = GICD_ICACTIVER(REG_FROM_IRQ(irq));
	sys_write32(BIT(bit_pos), addr);

	/* Set priority for the interrupt */
	addr = GICD_IPRIORITYR(irq / 4);
	val = sys_read32(addr);		/* Read proprity reg */
	val &= ~(0xFF << bit_pos);	/* Clear existing priority */
	val |= priority << bit_pos;	/* Or in the priority to set */
	sys_write32(val, addr);		/* Program the priority register */
}

/**
 * @brief GIC controller initialization.
 *
 * Initialization of gic interrupt controller. GIC distributor and
 * GIC controller are enabled for group 0 and group 1 interrupts and
 * priority mask register is configured to forward interrupts of all
 * priority levels.
 *
 */
static int _gic400_init(struct device *dev)
{
	u32_t max_irqs;

	ARG_UNUSED(dev);

	/* Enable both Group 0 and Group 1 interrupts (BIT[1:0]) */
	sys_write32(0x3, GICD_CTLR);

	/* Enable both Group 0 and Group 1 interrupts (BIT[1:0])
	 * Enable FIQ (BIT[3]) - This will forward group 0 interrupts as FIQ
	 */
	sys_write32(0xB, GICC_CTLR);

	/* Set priority levels to max to allow all interrupts to
	 * be forwarded to the CPU interface
	 */
	sys_write32(0x000000FF, GICC_PMR);

	max_irqs = NUM_IRQS_PER_REG * ((sys_read32(GICD_TYPER) & 0x1F) + 1);
	if (max_irqs != CONFIG_NUM_GIC_INTERRUPTS)
		LOG_WRN("max irqs %d, actual irqs %d\n",
			    CONFIG_NUM_GIC_INTERRUPTS, max_irqs);

	return 0;
}

/**
 *
 * @brief ISR wrapper for IRQ and FIQ interrupts
 *
 * This routine is a wrapper for all IRQ and FIQ interrupt. It checks for the
 * enable/pending interrupt state in GIC registers and calls the corresponding
 * ISR connected to the irq (via IRQ_CONNECT). It uses the auto generated
 * _sw_isr_table array to retrieve the ISR and argument values for the irq
 *
 * The CPSR.M bits are used to determine if FIQ interrupts are being serviced
 * or IRQ interrupts.
 *
 */
void gic400_isr_wrapper(void)
{
	u8_t i, j, num_int_sets, mode;
	u32_t addr, status, irq;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	read_timer_start_of_isr();
#endif

#ifdef CONFIG_TRACING
	z_sys_trace_isr_enter();
#endif

	mode = z_GetCpuMode();

	num_int_sets = (sys_read32(GICD_TYPER) & 0x1F) + 1;
	for (i = 0; i < num_int_sets; i++) {
		addr = GICD_ISENABLER(i);
		status = sys_read32(addr);	/* Enabled interrupts */
		addr = GICD_IGROUPR(i);
		if (mode == CPU_MODE_IRQ)
			status &= sys_read32(addr);	/* Group 1 - IRQs */
		else
			status &= ~sys_read32(addr);	/* Group 0 - FIQs */
		addr = GICD_ICPENDR(i);
		status &= sys_read32(addr);

		if (status == 0x0)
			continue;

		for (j = 0; j < NUM_IRQS_PER_REG; j++) {
			if (status & BIT(j)) {
				/* Clear interrupt */
				sys_write32(BIT(j), addr);
				irq = (i * NUM_IRQS_PER_REG) + j;
				_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
			}
		}
	}

#ifdef CONFIG_EXECUTION_BENCHMARKING
	read_timer_end_of_isr();
#endif
}

SYS_INIT(_gic400_init, PRE_KERNEL_1, CONFIG_GIC_INIT_PRIORITY);
