/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Exception stack frame dump routines
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <inttypes.h>

#include <misc/printk.h>


#if CONFIG_STACK_DUMP_SIZE_ON_FAULT
/*
 */
static void dump_sys_stack(const NANO_ESF *esf)
{
	u32_t key, sp_mode, i, *sp;
	u8_t cpu_mode, fault_cpu_mode;

	/* Get stack pointer for sys mode */
	cpu_mode = _GetCpuMode();
	fault_cpu_mode = esf->xpsr & CPU_MODE_MASK;

	/* Dump stack only if fault occurred from sys/fiq/irq mode
	 * and current cpu mode is und/abt/svc
	 */
	switch (fault_cpu_mode) {
	case CPU_MODE_SYS:
	case CPU_MODE_FIQ:
	case CPU_MODE_IRQ:
		break;
	default:
		return;
	}

	switch (cpu_mode) {
	case CPU_MODE_UND:
	case CPU_MODE_ABT:
	case CPU_MODE_SVC:
		break;
	default:
		return;
	}

	key = irq_lock();

	/* Switch to the mode from which the fault occurred */
	switch (fault_cpu_mode) {
	case CPU_MODE_FIQ:
	/* Switch to fiq mode an retrieve SP_FIQ */
	__asm__ volatile ("cps %[mode]\n"
			  "mov %[sp_mode], sp\n"
			  : [sp_mode] "=r" (sp_mode)
			  : [mode] "I" (CPU_MODE_FIQ)
			 );
		break;
	case CPU_MODE_IRQ:
	/* Switch to irq mode an retrieve SP_IRQ */
	__asm__ volatile ("cps %[mode]\n"
			  "mov %[sp_mode], sp\n"
			  : [sp_mode] "=r" (sp_mode)
			  : [mode] "I" (CPU_MODE_IRQ)
			 );
		break;
	case CPU_MODE_USR:
	case CPU_MODE_SYS:
		/* Switch to sys mode an retrieve SP_USR */
	__asm__ volatile ("cps %[mode]\n"
			  "mov %[sp_mode], sp\n"
			  : [sp_mode] "=r" (sp_mode)
			  : [mode] "I" (CPU_MODE_SYS)
			 );
		break;
	default:
		irq_unlock(key);
		return;
	}

	/* Restore CPU mode */
	switch (cpu_mode) {
	case CPU_MODE_UND:
		__asm__ volatile("cps %[mode]" :: [mode] "I" (CPU_MODE_UND));
		break;
	case CPU_MODE_ABT:
		__asm__ volatile("cps %[mode]" :: [mode] "I" (CPU_MODE_ABT));
		break;
	case CPU_MODE_SVC:
		__asm__ volatile("cps %[mode]" :: [mode] "I" (CPU_MODE_SVC));
		break;
	}

	printk("---------------\n");
	printk("SP = 0x%08x\n", sp_mode);
	printk("---------------\n");
	sp = (u32_t *)sp_mode;
	for (i = 0; i < CONFIG_STACK_DUMP_SIZE_ON_FAULT / 4; i += 4) {
		printk("[0x%08x]: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			(u32_t)(sp + i), sp[i], sp[i+1], sp[i+2], sp[i+3]);
	}

	irq_unlock(key);
}
#endif

void sys_exc_esf_dump(const NANO_ESF *esf)
{
	printk("Exception Stack Frame\n");
	printk("---------------------\n");
	printk("r0:  0x%08x  ", esf->r0);
	printk("r1:  0x%08x  ", esf->r1);
	printk("r2:  0x%08x\n", esf->r2);
	printk("r3:  0x%08x  ", esf->r3);
	printk("pc:  0x%08x  ", esf->pc);
	printk("psr: 0x%08x\n", esf->xpsr);

#if CONFIG_STACK_DUMP_SIZE_ON_FAULT
	dump_sys_stack(esf);
#endif
}
