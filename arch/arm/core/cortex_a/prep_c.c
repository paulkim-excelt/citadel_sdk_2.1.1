/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel.h>
#include <zephyr/types.h>
#include <toolchain.h>
#include <linker/linker-defs.h>
#include <kernel_internal.h>
#include <string.h>
#include <cortex_a/cp15.h>
#include <cortex_a/stack.h>

#if defined(CONFIG_ARMV7_A)

extern void __start(void);

#ifdef CONFIG_INIT_STACKS
void init_stacks(void)
{
	/* Initialize interrupt stacks. */
	memset(&_interrupt_stack, 0xAA, CONFIG_ISR_STACK_SIZE);
	memset(&_fiq_stack, 0xAA, CONFIG_ISR_STACK_SIZE);
	memset(&_svc_stack, 0xAA, CONFIG_SVC_STACK_SIZE);
	memset(&_abt_stack, 0xAA, CONFIG_ABT_STACK_SIZE);
	memset(&_und_stack, 0xAA, CONFIG_UND_STACK_SIZE);
}
#endif

static void switch_to_int_stack(void)
{
	/* The re-use of interrupt stack as system stack at init, is
	 * fine, because interrupts are enabled only after we switch
	 * to the init thread, at which point the system stack will
	 * point to the stack of the current thread
	 */
	__asm__ volatile ("mov sp, %[stack_ptr]\n\t"
			  :: [stack_ptr] "r" ((u32_t)&_interrupt_stack +
			  CONFIG_ISR_STACK_SIZE));
}

static inline void configure_vector_table(void)
{
	u32_t sctlr;

	/* VBAR Setup */

	/* Read in the System Control Register */
	sctlr = read_sctlr();
	/* Clear the V bit enabling Relocation of VBAR */
	sctlr &= ~(0x1UL << 13);
	/* Write Back the System Control Register */
	write_sctlr(sctlr);

	/* Write the low vecs addr to VBAR
	 * Bits[4:0] are Reserved, UNK/SBZP
	 */
	write_vbar((u32_t)__start & ~(0x1F));
}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMv7_A */

#ifdef CONFIG_FLOAT
static inline void enable_floating_point(void)
{
	u32_t reg, fp_exc;

	/* Enable full Access to cp10 and cp11 registers */
	reg = read_cpacr();
	reg |= 0x3 << 20; /* Cp10 full access */
	reg |= 0x3 << 22; /* Cp11 full access */
	write_cpacr(reg);

	/* Enable non-secure access to cp10 and cp11 registers */
	reg = read_nsacr();
	reg |= 0x1 << 10; /* Cp10 non-secure access */
	reg |= 0x1 << 11; /* Cp11 non-secure access */
	write_nsacr(reg);

	/* Set FPSCR.EN to enable VFP */
	__asm__ volatile ("vmrs %[fp_exc], fpexc\t\n"
			  "orr %[fp_exc], #0x40000000\t\n"
			  "vmsr fpexc, %[fp_exc]"
			  : [fp_exc] "=r" (fp_exc));
}
#else
static inline void enable_floating_point(void)
{
}
#endif

extern FUNC_NORETURN void z_cstart(void);
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	extern u64_t __start_time_stamp;
#endif
void _PrepC(void)
{
#ifdef CONFIG_INIT_STACKS
	init_stacks();
#endif

	/* Use interrupt stack until we switch to main thread. We don't expect
	 * any interrupts until then, so this re-use is valid.
	 */
	switch_to_int_stack();

	configure_vector_table();
	enable_floating_point();
	z_bss_zero();
	z_data_copy();
#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	__start_time_stamp = 0U;
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
