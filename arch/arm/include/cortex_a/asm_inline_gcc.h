/* ARM Cortex-A GCC specific inline assembler functions and macros */

/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ASM_INLINE_GCC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include asm_inline.h instead
 */


#define CPU_MODE_MASK	0x1F

#define CPU_MODE_USR	0x10
#define CPU_MODE_FIQ	0x11
#define CPU_MODE_IRQ	0x12
#define CPU_MODE_SVC	0x13
#define CPU_MODE_MON	0x16
#define CPU_MODE_ABT	0x17
#define CPU_MODE_HYP	0x1A
#define CPU_MODE_UND	0x1B
#define CPU_MODE_SYS	0x1F

#ifndef _ASMLANGUAGE
/**
 *
 * @brief Set stack pointer for IRQ mode
 *
 * Store the value of <sp> in SP_IRQ register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_IrqStackSet(u32_t sp) /* value to store in SP_IRQ */
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t"	/* Read cpsr */
			 "cps %[irq_mode]\n\t"		/* Switch to IRQ mode */
			 "mov sp, %[sp]\n\t"		/* Set sp (SP_IRQ) */
			 "msr cpsr, %[cpsr]\n\t"	/* Restore cpsr */
			 : [cpsr] "=&r" (cpsr)
			 : [sp] "r" (sp), [irq_mode] "I" (CPU_MODE_IRQ)
			);
}

/**
 *
 * @brief Set stack pointer for FIQ mode
 *
 * Store the value of <sp> in SP_FIQ register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_FiqStackSet(u32_t sp) /* value to store in SP_FIQ */
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t"	/* Read cpsr */
			 "cps %[fiq_mode]\n\t"		/* Switch to FIQ mode */
			 "mov sp, %[sp]\n\t"		/* Set sp (SP_FIQ) */
			 "msr cpsr, %[cpsr]\n\t"	/* Restore cpsr */
			 : [cpsr] "=&r" (cpsr)
			 : [sp] "r" (sp), [fiq_mode] "I" (CPU_MODE_FIQ)
			);
}

/**
 *
 * @brief Set stack pointer for SVC mode
 *
 * Store the value of <sp> in SP_SVC register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_SvcStackSet(u32_t sp) /* value to store in SP_SVC */
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t"	/* Read cpsr */
			 "cps %[svc_mode]\n\t"		/* Switch to SVC mode */
			 "mov sp, %[sp]\n\t"		/* Set sp (SP_SVC) */
			 "msr cpsr, %[cpsr]\n\t"	/* Restore cpsr */
			 : [cpsr] "=&r" (cpsr)
			 : [sp] "r" (sp), [svc_mode] "I" (CPU_MODE_SVC)
			);
}

/**
 *
 * @brief Set stack pointer for Abort mode
 *
 * Store the value of <sp> in SP_ABT register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_AbtStackSet(u32_t sp) /* value to store in SP_ABT */
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t"	/* Read cpsr */
			 "cps %[abt_mode]\n\t"		/* Switch to ABT mode */
			 "mov sp, %[sp]\n\t"		/* Set sp (SP_ABT) */
			 "msr cpsr, %[cpsr]\n\t"	/* Restore cpsr */
			 : [cpsr] "=&r" (cpsr)
			 : [sp] "r" (sp), [abt_mode] "I" (CPU_MODE_ABT)
			);
}

/**
 *
 * @brief Set stack pointer for Undefined mode
 *
 * Store the value of <sp> in SP_UND register.
 *
 * @return N/A
 */
static ALWAYS_INLINE void z_UndStackSet(u32_t sp) /* value to store in SP_UND */
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t"	/* Read cpsr */
			 "cps %[und_mode]\n\t"		/* Switch to UND mode */
			 "mov sp, %[sp]\n\t"		/* Set sp (SP_UND) */
			 "msr cpsr, %[cpsr]\n\t"	/* Restore cpsr */
			 : [cpsr] "=&r" (cpsr)
			 : [sp] "r" (sp), [und_mode] "I" (CPU_MODE_UND)
			);
}

/**
 *
 * @brief Get the current CPU mode
 *
 * Extract the CPU mode from the CPSR and return it
 *
 * @return current processor mode.
 *
 */
static ALWAYS_INLINE u8_t z_GetCpuMode(void)
{
	u32_t cpsr;

	__asm__ volatile("mrs %[cpsr], cpsr\n\t" : [cpsr] "=r" (cpsr));

	return (u8_t)(cpsr & CPU_MODE_MASK);
}

/**
 *
 * @brief Get the Saved CPU mode
 *
 * Extract the CPU mode from the SPSR and return it
 * If we are in system mode then just return current CPU mode
 *
 * @return saved processor mode.
 *
 */
static ALWAYS_INLINE u8_t z_GetSavedCpuMode(void)
{
	u32_t spsr;

	if (z_GetCpuMode() == CPU_MODE_SYS)
		return CPU_MODE_SYS;

	__asm__ volatile("mrs %[spsr], spsr\n\t" : [spsr] "=r" (spsr));

	return (u8_t)(spsr & CPU_MODE_MASK);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ASM_INLINE_GCC_H_ */
