/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception handlers
 *
 * Exception handlers for aborts, interrupts and undefined exception
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <offsets_short.h>
#include <asm_inline.h>
#include <cortex_a/exc.h>

_ASM_FILE_PROLOGUE

#ifdef CONFIG_ARM_GIC_400
GTEXT(gic400_isr_wrapper)
#else
#error Unsupported interrupt controller
#endif

GTEXT(__undef)
GTEXT(__prefetch_abort)
GTEXT(__data_abort)
GTEXT(__irq_handler)
GTEXT(__fiq_handler)
GTEXT(z_check_stack_sentinel)
GTEXT(_Fault)

GDATA(_kernel)

/**
 *
 * @brief Exception exit tail call
 *
 * This function is called at the end of the following exception handlers
 * to determine if a context switch needs to be made and trigger the svc
 * exception if the context switch is needed.
 *
 *  - IRQ
 *  - FIQ
 *  - Prefetch Abort
 *  - Data Abort
 *  - Undefined instruction
 *
 * At the entry of this routine, the stack is expected to have the following
 * register values. These values restored from the stack and the stack unwinded
 * before returning from the exception or switching context with an svc call
 *    --------
 *    |  r12 |      <--- Top of Exception's stack
 *    --------
 *    |  r3  |
 *    --------
 *    |  r2  |
 *    --------
 *    |  r1  |
 *    --------
 *    |  r0  |
 *    --------
 *    |  lr  |      <--- Preferred return value after subtracting the offset
 *    --------
 *    | spsr |      <--- SP
 *    --------
 *
 * If premeption is enabled, then this routine performs the following checks
 * to determine if a context switch is needed.
 *
 * if (SPSR.M == SYS) {  -- Context switch only if we came from sys mode
 *      if (_current != ready_q_cache)
 *          _switch_context;
 *      } else {
 *          _no_context_switch;
 *      }
 * } else {
 *      _no_context_switch;
 * }
 */
SECTION_SUBSEC_FUNC(TEXT,_exc_exit,exc_exit)
#ifdef CONFIG_PREEMPT_ENABLED
    /* Check and switch context if needed and allowed */
    ldr r0, [sp]

    /* Check we came from sys mode */
    and r0, #CPU_MODE_MASK
    cmp r0, #CPU_MODE_SYS
    bne _no_context_switch

    /* Check a context switch is needed */
    ldr r0, =_kernel
    ldr r1, [r0, #_kernel_offset_to_current]

    /* Current thread and thread to switch are the same */
    ldr r2, [r0, #_kernel_offset_to_ready_q_cache]
    cmp r1, r2
    beq _no_context_switch

_switch_context:
    /* Call check_stack sentinel */
    pop {lr}
    msr spsr, lr
    pop {lr}
    pop {r0-r3, r12}
    svc #0

_no_context_switch:
#endif  /* CONFIG_PREEMPT_ENABLED */

#ifdef CONFIG_STACK_SENTINEL
    bl z_check_stack_sentinel
#endif
    pop {lr}
    msr spsr, lr
    pop {lr}
    pop {r0-r3, r12}
    movs pc, lr

/**
 *
 * @brief IRQ handler
 *
 * IRQ interrupt handler
 *  - Update LR to preferred exception return address
 *  - save the context as expected by exc_exit
 *  - Call the IRQ wrapper
 *  - Exception exit
 */
SECTION_SUBSEC_FUNC(TEXT,irq_exc,__irq_handler)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    sub lr, lr, #4  /* Preferred return address from IRQ interrupt is LR - 4 */
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
    /*
     * All interrupts are disabled when handling idle wakeup.  For tickless
     * idle, this ensures that the calculation and programming of the device
     * for the next timer deadline is not interrupted.  For non-tickless
     * idle, this ensures that the clearing of the kernel idle state is not
     * interrupted.  In each case, z_sys_power_save_idle_exit is called with
     * interrupts disabled.
     */
    mrs r3, cpsr
    cpsid if  /* Disable FIQ/IRQ */

    /* is this a wakeup from idle ? */
    ldr r2, =_kernel
    /* requested idle duration, in ticks */
    ldr r0, [r2, #_kernel_offset_to_idle]
    cmp r0, #0

#if defined(CONFIG_ARMV7_A)
    ittt ne
    movne   r1, #0
    /* clear kernel idle state */
    strne   r1, [r2, #_kernel_offset_to_idle]
    blxne   z_sys_power_save_idle_exit
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */

    /* restore interrupts */
    msr cpsr, r3
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

    /* Call the interrupt handler*/
#ifdef CONFIG_ARM_GIC_400
    bl gic400_isr_wrapper
#else
    #error "Unspported interrupt controller"
#endif

    bl exc_exit

/**
 *
 * @brief FIQ handler
 *
 * FIQ interrupt handler
 *  - Update LR to preferred exception return address
 *  - save the context as expected by exc_exit
 *  - Call the FIQ wrapper
 *  - Exception exit
 */
SECTION_SUBSEC_FUNC(TEXT,fiq_exc,__fiq_handler)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    sub lr, lr, #4  /* Preferred return address from FIQ interrupt is LR - 4 */
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

#ifdef CONFIG_SYS_POWER_MANAGEMENT
    /*
     * All interrupts are disabled when handling idle wakeup.  For tickless
     * idle, this ensures that the calculation and programming of the device
     * for the next timer deadline is not interrupted.  For non-tickless
     * idle, this ensures that the clearing of the kernel idle state is not
     * interrupted.  In each case, z_sys_power_save_idle_exit is called with
     * interrupts disabled.
     */
    mrs r3, cpsr
    cpsid if  /* Disable FIQ/IRQ */

    /* is this a wakeup from idle ? */
    ldr r2, =_kernel
    /* requested idle duration, in ticks */
    ldr r0, [r2, #_kernel_offset_to_idle]
    cmp r0, #0

#if defined(CONFIG_ARMV7_A)
    ittt ne
    movne   r1, #0
    /* clear kernel idle state */
    strne   r1, [r2, #_kernel_offset_to_idle]
    blxne   z_sys_power_save_idle_exit
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */

    /* restore interrupts */
    msr cpsr, r3
#endif /* CONFIG_SYS_POWER_MANAGEMENT */

    /* Call the interrupt handler*/
#ifdef CONFIG_ARM_GIC_400
    bl gic400_isr_wrapper
#else
    #error "Unspported interrupt controller"
#endif

    bl exc_exit

/**
 *
 * @brief Fault exit
 *
 * This routine generates the fault stack in _default_esf using
 * stack frame as defined by exc_exit and calls the C _Fault function, which
 * in turn dumps fault related information and calls the fatal error handler
 *
 * C function prototype:
 * NORETURN _fault_exit(unsigned int fault_reason)
 */
SECTION_SUBSEC_FUNC(TEXT,_fault_exit,_fault_exit)
    /* Prepare ESF and call Fatal error handler */
    ldr r1, =_default_esf

    ldr r2, [sp, #8]   /* r0 */
    str r2, [r1]

    ldr r2, [sp, #12]   /* r1 */
    str r2, [r1, #4]

    ldr r2, [sp, #16]   /* r2 */
    str r2, [r1, #8]

    ldr r2, [sp, #20]   /* r3 */
    str r2, [r1, #12]

    ldr r2, [sp, #4]    /* PC */
    str r2, [r1, #16]

    ldr r2, [sp]        /* PSR */
    str r2, [r1, #20]

    bl _Fault
    bl exc_exit

/**
 *
 * @brief Prefetch abort handler
 *
 * Prefect abort exception handling
 *  - Update LR to preferred exception return address
 *  - save the context as expected by exc_exit
 *  - Branch the _fault_exit
 */
SECTION_SUBSEC_FUNC(TEXT,prefetch_abt_exc,__prefetch_abort)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    sub lr, lr, #4  /* Preferred return address from Prefetch abort is LR - 4 */
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

    mov r0, #PREFETCH_ABORT_EXCEPTION
    bl _fault_exit

/**
 *
 * @brief Data abort handler
 *
 * Data abort exception handling
 *  - Update LR to preferred exception return address
 *  - save the context as expected by exc_exit
 *  - Branch the _fault_exit
 */
SECTION_SUBSEC_FUNC(TEXT,data_abt_exc,__data_abort)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    sub lr, lr, #8  /* Preferred return address from Data abort is LR - 8 */
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

    mov r0, #DATA_ABORT_EXCEPTION
    bl _fault_exit

/**
 *
 * @brief Undefined exception handler
 *
 * Undefined instruction exception handling
 *  - Update LR to preferred exception return address
 *  - save the context as expected by exc_exit
 *  - Branch the _fault_exit
 */
SECTION_SUBSEC_FUNC(TEXT,undef_exc,__undef)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    mrs r0, spsr
    tst r0, #THUMB_STATE_MASK
    ite ne
    subne lr, lr, #2    /* Preferred return address: LR - 2 if SPSR.Thumb = 1 */
    subeq lr, lr, #4    /* Preferred return address: LR - 4 if SPSR.Thumb = 0 */
    push {lr}           /* Save the return address */
    push {r0}           /* Save SPSR */

    mov r0, #UNDEF_INSTRUCTION_EXCEPTION
    bl _fault_exit
