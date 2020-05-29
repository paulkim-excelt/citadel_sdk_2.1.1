/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-A CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_EXC__H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_EXC__H_

#include <arch/cpu.h>
#include <asm_inline.h>
#include <cortex_a/cp15.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Cortex-A exceptions */
#define RESET_EXCEPTION			0
#define UNDEF_INSTRUCTION_EXCEPTION	1
#define SUPERVISOR_CALL_EXCEPTION	2
#define PREFETCH_ABORT_EXCEPTION	3
#define DATA_ABORT_EXCEPTION		4
#define IRQ_EXCEPTION			5
#define FIQ_EXCEPTION			6

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <irq_offload.h>
#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/**
 *
 * @brief Find out if running in an ISR context
 *
 * If the ARM CPU mode is one of the following then, we consider to be in an
 * interrupt context
 *
 * - CPU_MODE_FIQ
 * - CPU_MODE_IRQ
 * - CPU_MODE_ABT
 * - CPU_MODE_UND
 * - CPU_MODE_SVC
 *
 * @return true if in ISR, false if not.
 */
static ALWAYS_INLINE bool z_IsInIsr(void)
{
	u8_t mode;

	mode = z_GetCpuMode();

	switch (mode) {
	case CPU_MODE_FIQ:
	case CPU_MODE_IRQ:
	case CPU_MODE_ABT:
	case CPU_MODE_UND:
	case CPU_MODE_SVC:
		return true;
	default:
		return false;
	}
}

/**
 * @brief Setup system exceptions
 *
 * Configure secure configuration register to
 *  - Make Abort disable bit writate in non-secure state
 *  - Make FIQ disable bit writate in non-secure state
 *  - Disable External abort routing to monitor mode
 *  - Make FIQ exceptions to be routed to FIQ mode
 *  - Make IRQ exceptions to be routed to IRQ mode
 *
 * Configure system control register
 *  - Set Thumb Exception enable bit
 *  - Enable unaligned accesses
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_ExcSetup(void)
{
	u32_t scr;

	/* Read in the Secure configuration register */
	scr = read_scr();

	/* SCR.AW = 1 */
	scr |= (0x1UL << 5);
	/* SCR.FW = 1 */
	scr |= (0x1UL << 4);
	/* SCR.EA = 0 */
	scr &= ~(0x1UL << 3);
	/* SCR.FIQ = 0 */
	scr &= ~(0x1UL << 2);
	/* SCR.IRQ = 0 */
	scr &= ~(0x1UL << 1);

	/* Write back the Secure configuration register */
	write_scr(scr);

	/* Read in the system control register */
	scr = read_sctlr();

	/* SCTLR.TE = 1 */
	scr |= (0x1UL << 30);

	/* SCTLR.A = 0 */
	scr &= ~(0x1UL << 1);

	/* Write back the system control register */
	write_sctlr(scr);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_EXC__H_ */
