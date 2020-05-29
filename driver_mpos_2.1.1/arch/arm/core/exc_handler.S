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

#define _ASMLANGUAGE

#include <cortex_a/exc.h>

#define THUMB_STATE_MASK    (0x1 << 5)

.global __undef
.global __prefetch_abort
.global __data_abort
.global _Fault

.global __no_os_irq_handler

/**
 *
 * @brief Exception exit tail call
 *
 * This function is called at the end of the following exception handlers
 * to determine if a context switch needs to be made and trigger the svc
 * exception if the context switch is needed.
 *
 *  - Prefetch Abort
 *  - Data Abort
 *  - Undefined instruction
 *
 * At the entry of this routine, the stack is expected to have the following
 * register values. These values restored from the stack and the stack unwinded
 * before returning from the exception
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
 */
.section .text.exc_exit, "ax"; exc_exit :
    pop {lr}
    msr spsr, lr
    pop {lr}
    pop {r0-r3, r12}
    movs pc, lr

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
.section .text._fault_exit, "ax"; _fault_exit :
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
.section .text.__prefetch_abort, "ax"; __prefetch_abort :
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
.section .text.__data_abort, "ax"; __data_abort :
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
.section .text.__undef, "ax"; __undef :
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
.section .text.irq_exc, "ax"; __no_os_irq_handler :
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    sub lr, lr, #4  /* Preferred return address from IRQ interrupt is LR - 4 */
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

    /* Call the interrupt handler*/
    bl vApplicationFPUSafeIRQHandler

    pop {lr}
    msr spsr, lr
    pop {lr}
    pop {r0-r3, r12}
    movs pc, lr
