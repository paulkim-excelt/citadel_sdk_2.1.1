/* Inline assembler kernel functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_ASM_INLINE_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_ASM_INLINE_H_

#if defined(CONFIG_CPU_CORTEX_A)
#if defined(__GNUC__)
#include <cortex_a/asm_inline_gcc.h>
#else
#include <cortex_a/asm_inline_other.h>
#endif /* __GNUC__ */
#endif /* CONFIG_CPU_CORTEX_A */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_ASM_INLINE_H_ */
