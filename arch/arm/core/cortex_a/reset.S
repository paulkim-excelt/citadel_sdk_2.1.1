/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <cortex_a/stack.h>

_ASM_FILE_PROLOGUE

GTEXT(__start)
GTEXT(__reset)
GTEXT(__undef)
GTEXT(__svc)
GTEXT(__prefetch_abort)
GTEXT(__data_abort)
GTEXT(__irq_handler)
GTEXT(__fiq_handler)

#ifdef CONFIG_BOOT_LOADER_IN_ARM_STATE
/* This prolog switches the CPU state from arm to thumb. If the boot loader
 * that is handing off control to the zephyr image is built to execute in ARM
 * state then this is required. The size of this prolog needs to be aligned
 * to 32 bytes, so that the __start symbol gets aligned to a 32 byte boundary.
 * This is needed because the Bit[4:0] of VBAR register (to which this __start
 * will be programmed) are reserved.
 *
 */
SECTION_SUBSEC_FUNC(TEXT,_startup,_arm_to_thumb_handoff)
    .word 0x00000000
    .word 0x00000000
    .word 0x00000000
    .word 0x00000000
    .word 0x00000000
    .word 0x00000000
    .word 0xe28f0001 /* add r0, pc, #1 */
    .word 0xe12fff10 /* bx r0 */
__start:
#else
/*
 * The entry point is located at the __reset symbol, which
 * is fetched by a XIP image playing the role of a bootloader, which jumps to
 * it, not through the reset vector mechanism. Such bootloaders might want to
 * search for a __start symbol instead, so create that alias here.
 */
SECTION_SUBSEC_FUNC(TEXT,_startup,__start)
#endif /* CONFIG_BOOT_LOADER_IN_ARM_STATE */
    ldr pc, reset_handler
    ldr pc, undef_instr_handler
    ldr pc, svc_handler
    ldr pc, prefetch_abort_handler
    ldr pc, data_abort_handler
    ldr pc, unused_handler
    ldr pc, irq_handler
    ldr pc, fiq_handler

    reset_handler:              .word __reset
    undef_instr_handler:        .word __undef
    svc_handler:            	.word __svc
    prefetch_abort_handler:	.word __prefetch_abort
    data_abort_handler:        	.word __data_abort
    unused_handler:            	.word __reset
    irq_handler:            	.word __irq_handler
    fiq_handler:            	.word __fiq_handler

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. Locking interrupts prevents anything
 * from interrupting the CPU.
 *
 * We initialize the stack pointer to the _main_stack after switching to SYS
 * mode
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 *
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset)

    /* lock interrupts: will get unlocked when switch to main task */
    cpsid ifa

#ifdef CONFIG_WDOG_INIT
    /* board-specific watchdog initialization is necessary */
    bl _WdogInit
#endif

    /* Hook for special handling when returning from low power mode */
    bl _handle_return_from_lp_mode

    /* Switch to System Stack Pointer */
    cps #CPU_MODE_SYS

    /* Set stack pointer to _main_stack
     * this will used by _PrepC to initialize
     * the stacks for all the CPU modes
     */
    ldr sp, =_main_stack
    add sp, sp, #CONFIG_MAIN_STACK_SIZE

    b _PrepC

.weak _handle_return_from_lp_mode

_handle_return_from_lp_mode:
    bx lr
