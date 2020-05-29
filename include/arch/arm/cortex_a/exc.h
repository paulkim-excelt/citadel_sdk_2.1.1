/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public exception handling
 *
 * ARM-specific kernel exception handling interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_EXC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_EXC_H_

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
#else
#include <zephyr/types.h>

/* Arm Cortex-A does not generate an exception stack frame on exception entry
 *
 * - Upon an exception the PC is stored into the LR
 *   for the mode to which the exception was taken (e.g. LR_IRQ
 *   for IRQ exception) or ELR_hyp. In addition the CPSR is stored
 *   into the SPSR register for the mode to which the exception was
 *   taken.
 *
 * - Since the hardware does not generate an exception stack frame, the
 *   exception handlers will be generating the exception stack frame with the
 *   following registers instead and passing it to the fatal error handlers
 *   and fault handler. SP and LR are banked registers and will be preserved.
 *   - r0 - r3
 *   - pc
 *   - psr
 */
struct __esf {
	sys_define_gpr_with_alias(a1, r0);
	sys_define_gpr_with_alias(a2, r1);
	sys_define_gpr_with_alias(a3, r2);
	sys_define_gpr_with_alias(a4, r3);
	sys_define_gpr_with_alias(pc, r15);
	u32_t xpsr;
};

typedef struct __esf NANO_ESF;

extern void _ExcExit(void);

/**
 * @brief display the contents of a exception stack frame
 *
 * @return N/A
 */

extern void sys_exc_esf_dump(const NANO_ESF *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_EXC_H_ */
