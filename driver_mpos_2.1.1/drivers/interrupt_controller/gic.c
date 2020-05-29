/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/*
 * file: gic.c
 *
 * description: This file defines the apis required to configure the Generic
 *		Interrupt Controller and install/uninstall ISRs
 */

#include <stddef.h>
#include <arch/cpu.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <board.h>
#include <toolchain.h>
#include <init.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>

/* GICD_TYPER.ITLinesNumber = 4 ==> num interrupts = 32(N+1) = 32*(4+1) = 160 */
#define MAX_INTERRUPTS_SUPPORTED	160

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

struct irq_handler {
	void *param;
	void (*isr)(void *);
};

static struct irq_handler isrs[MAX_INTERRUPTS_SUPPORTED] = {NULL};

void gic400_irq_enable(u32_t irq)
{
	u32_t addr, val, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	/* Clear interrupts - Write 0 has no effect */
	addr = GICD_ICENABLER(irq / 32);
	sys_write32(BIT(bit_pos), addr);

	/* Clear Pending and Active bits for the interrupt
	 * - Write 0 has no effect
	 */
	addr = GICD_ICPENDR(irq / 32);
	sys_write32(BIT(bit_pos), addr);
	addr = GICD_ICACTIVER(irq / 32);
	sys_write32(BIT(bit_pos), addr);

	/* Set interrupt group to group 0 */
	addr = GICD_IGROUPR(irq / 32);
	/* Group 1 for IRQ */
	sys_set_bit(addr, bit_pos);

	/* Setup interrupt as level triggered
	 * 2 bits per interrupts, MSB for edge/level sensitive
	 * bit pos - [2n+1]
	 */
	addr = GICD_ICFGR(irq / 16);
	sys_set_bit(addr, (bit_pos % 16) * 2 + 1);

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
	addr = GICD_ISENABLER(irq / 32);
	sys_write32(BIT(bit_pos), addr);
}

void gic400_irq_disable(u32_t irq)
{
	u32_t addr, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	addr = GICD_ICENABLER(irq / 32);
	sys_write32(BIT(bit_pos), addr);
}

void gic400_irq_set_pend(u32_t irq)
{
	u32_t addr, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	/* Set Pending bits for the interrupt
	 * - Write 0 has no effect
	 */
	addr = GICD_ISPENDR(irq / 32);
	sys_write32(BIT(bit_pos), addr);
}

void gic400_irq_clear_pend(u32_t irq)
{
	u32_t addr, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	/* Set Pending bits for the interrupt
	 * - Write 0 has no effect
	 */
	addr = GICD_ICPENDR(irq / 32);
	sys_write32(BIT(bit_pos), addr);
}

int gic400_irq_is_enabled(u32_t irq)
{
	u32_t addr, val, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return false;

	addr = GICD_ISENABLER(irq / 32);
	val = sys_read32(addr);

	return !!(val & BIT(bit_pos));
}

void gic400_set_priority(u32_t irq, u32_t priority)
{
	u32_t addr, val, bit_pos;

	bit_pos = irq % 32;

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	/* Set priority for the interrupt
	 * 8 bits per interrupt
	 */
	addr = GICD_IPRIORITYR(irq / 4);
	val = sys_read32(addr);
	val &= ~(0xFF << ((bit_pos % 4) * 8));
	val |= priority << ((bit_pos % 4) * 8);
	sys_write32(val, addr);
}

int gic400_install_isr(u32_t irq, u32_t flags, void (*func)(void *),
		       void *param)
{
	ARG_UNUSED(flags);

	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return -EINVAL;

	isrs[irq].isr = func;
	isrs[irq].param = param;
	return 0;
}

static int gic400_init(struct device *dev)
{
	u32_t max_irqs;

	ARG_UNUSED(dev);

	/* Enable both Group 0 and Group 1 interrupts */
	sys_write32(0x3, GICD_CTLR);

	/* Enable both Group 0 and Group 1 interrupts [1:0]
	 * Enable FIQ [2] - This will forward group 0 interrupts as FIQ
	 */
	sys_write32(0xB, GICC_CTLR);

	/* Set priority levels to max to allow all interrupts to
	 * be forwarded to the CPU interface
	 */
	sys_write32(0x000000F8, GICC_PMR);

	max_irqs = 32 * ((sys_read32(GICD_TYPER) & 0x1f) + 1);
	if (max_irqs != MAX_INTERRUPTS_SUPPORTED)
		SYS_LOG_WRN("max irqs %d, actual irqs %d\n",
			    MAX_INTERRUPTS_SUPPORTED, max_irqs);

	return 0;
}

SYS_INIT(gic400_init, PRE_KERNEL_1, CONFIG_INTERRUPT_INIT_PRIORITY);

/* FIXME - Pulling this into here since it has to do with interrupts, but it
 * needs to be revisited (and the core interrupt handling too)
 */

void gic400_call_isr(u32_t irq)
{
	if (irq >= MAX_INTERRUPTS_SUPPORTED)
		return;

	if (isrs[irq].isr != NULL)
		isrs[irq].isr(isrs[irq].param);
}

void vApplicationFPUSafeIRQHandler(void)
{
	u8_t i, j, num_int_sets;
	u32_t addr, status;

	num_int_sets = 4 + 1; /* GICD_TYPER.ITLinesNumber + 1 */
	for (i = 0; i < num_int_sets; i++) {
		addr = GICD_ISENABLER(i);
		status = sys_read32(addr);

		/* Filter out only IRQ interrupts */
		addr = GICD_IGROUPR(i);
		status &= sys_read32(addr);	/* Group 1 - IRQs */

		addr = GICD_ICPENDR(i);
		status &= sys_read32(addr);

		if (status == 0x0)
			continue;

		for (j = 0; j < 32; j++) {
			if (status & BIT(j)) {
				/* Clear interrupt */
				sys_write32(BIT(j), addr);
				gic400_call_isr(i * 32 + j);
			}
		}
	}
}
