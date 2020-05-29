/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A power management
 *
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#ifdef CONFIG_TICKLESS_IDLE
#include <kernel_structs.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(k_cpu_idle)
GTEXT(k_cpu_atomic_idle)

#ifdef CONFIG_SYS_POWER_MANAGEMENT

GTEXT(_NanoIdleValGet)
GTEXT(_NanoIdleValClear)

/**
 *
 * @brief Get the kernel idle setting
 *
 * Returns the kernel idle setting, in ticks.
 *
 * @return the requested number of ticks for the kernel to be idle
 *
 * C function prototype:
 *
 * s32_t _NanoIdleValGet (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValGet)
	ldr r0, =_kernel
	ldr r0, [r0, #_kernel_offset_to_idle]
	bx lr

/**
 *
 * @brief Clear the kernel idle setting
 *
 * Sets the kernel idle setting to 0.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _NanoIdleValClear (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValClear)
	ldr r0, =_kernel
	mov r1, #0
	str r1, [r0, #_kernel_offset_to_idle]
	bx lr

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

/**
 *
 * @brief Power save idle routine for ARM Cortex-A
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.  The ARM 'wfi' instruction
 * will be issued, causing a low-power consumption sleep mode.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void k_cpu_idle (void);
 */

SECTION_FUNC(TEXT, k_cpu_idle)
#ifdef CONFIG_TRACING
	push {lr}
	bl    z_sys_trace_idle
	pop {lr}
#endif

#if defined(CONFIG_ARMV7_A)
	mov r0, #0
	push {lr}
	bl z_arch_irq_unlock
	pop {lr}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */

	wfi
	bx lr

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that occurs
 *    if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void k_cpu_atomic_idle (unsigned int imask);
 */

SECTION_FUNC(TEXT, k_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	push {lr}
	bl    z_sys_trace_idle
	pop {lr}
#endif

#if defined(CONFIG_ARMV7_A)
	/*
	 * Mask IRQ/FIQ: wfe will still get interrupted by incoming
	 * interrupts but the CPU will not service them right away.
	 */
	push {lr}
	bl z_arch_irq_lock

	/* r0: irq key (as obtained from irq_lock()) from caller */

	wfe

	bl _arch_irq_unlock
	pop {lr}
	bx lr

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */
