/**
 * @file
 * @brief ARM Cortex-A7 GCC specific floating point register macros
 */

/*
 * Copyright (c) 2017, Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_ARM_GCC_A7_H
#define _FLOAT_REGS_ARM_GCC_A7_H

#if !defined(__GNUC__) || !defined(CONFIG_CPU_CORTEX_A7)
#error __FILE__ goes only with Cortex-A7 GCC
#endif

#include <toolchain.h>
#include "float_context.h"

/**
 *
 * @brief Load all floating point registers
 *
 * This function loads ALL floating point registers pointed to by @a regs.
 * It is expected that a subsequent call to _store_all_float_registers()
 * will be issued to dump the floating point registers to memory.
 *
 * The format/organization of 'struct fp_register_set'; the generic C test
 * code (main.c) merely treat the register set as an array of bytes.
 *
 * The only requirement is that the arch specific implementations of
 * _load_all_float_registers() and _store_all_float_registers() agree
 * on the format.
 *
 * @return N/A
 */

static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"vldmia %[addr1], {d0-d15};\n\t"
		"vldmia %[addr2], {d16-d31};\n\t"
		: : [addr1] "r" (&regs->fp_non_volatile.d[0]),
		    [addr2] "r" (&regs->fp_non_volatile.d[16])
		);
}

/**
 *
 * @brief Dump all floating point registers to memory
 *
 * This function stores ALL floating point registers to the memory buffer
 * specified by @a regs. It is expected that a previous invocation of
 * _load_all_float_registers() occurred to load all the floating point
 * registers from a memory buffer.
 *
 * @return N/A
 */

static inline void _store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"vstmia %[addr1], {d0-d15};\n\t"
		"vstmia %[addr2], {d16-d31};\n\t"
		: : [addr1] "r" (&regs->fp_non_volatile.d[0]),
		    [addr2] "r" (&regs->fp_non_volatile.d[16])
		: "memory"
		);
}

/**
 *
 * @brief Load then dump all float registers to memory
 *
 * This function loads ALL floating point registers from the memory buffer
 * specified by @a regs, and then stores them back to that buffer.
 *
 * This routine is called by a high priority thread prior to calling a primitive
 * that pends and triggers a co-operative context switch to a low priority
 * thread.
 *
 * @return N/A
 */

static inline void _load_then_store_all_float_registers(struct fp_register_set
							*regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}
#endif /* _FLOAT_REGS_ARM_GCC_A7_H */
