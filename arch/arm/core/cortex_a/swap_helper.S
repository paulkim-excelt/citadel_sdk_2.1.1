/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-A
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-A CPUs.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <syscall.h>

_ASM_FILE_PROLOGUE

GTEXT(__swap)
GTEXT(__svc)
GDATA(_k_neg_eagain)

GDATA(_kernel)

/**
 *
 * @brief Service call handler
 *
 * svc #0 performs a context switch
 * svc #1 performs a irq offload if IRQ_OFFLOAD is enabled
 * svc #2 calls the fatal error handler and performs a context switch if the
 *        fatal error handler returns
 *
 * Context Switch:
 * svc #0 is called from 2 places
 *  - From __swap() call
 *  - As a tail call from interrupt/exception handlers (IRQ/FIQ/ABT/UNDEF)
 *    This is only done if the SPSR.mode == SYS in the intr/exc handler. In
 *    other words, the svc #0 is called at the end of a nested intr/exc call.
 *
 * To perform a context switch, the caller-saved and callee-saved registers need
 * to be saved to the thread arch object. Depending on CPU mode from which the
 * svc #0 call originated, the following registers to be saved will be retrieved
 * from different registers
 *    - Swap call
 *      PSR : SPSR_SVC
 *      PC  : LR_SVC + X (Where X = 2 if SPSR.T == 1, 4 otherwise)
 *    - Tail call from intr/exc handlers in mode <MODE>
 *      PSR : SPSR_<MODE>
 *      PC  : LR_<MODE>
 *
 * @return N/A
 */
#if defined(CONFIG_ARMV7_A)
SECTION_FUNC(TEXT, __svc)
    /* Save caller-saved registers and return address and PSR */
    push {r0-r3, r12}
    push {lr}       /* Save the return address */
    mrs r0, spsr
    push {r0}       /* Save SPSR */

    /* Determine svc instruction address
     *  PC = LR_SVC - 2 if SPSR.T == 1
     *  PC = LR_SVC - 4 if SPSR.T == 0
     */
    mov r1, lr      /* LR points to the instruction after the svc instruction */
    tst r0, #THUMB_STATE_MASK   /* Test SPSR.Thumb bit */
    ittee ne
    subne r1, #2    /* Assume thumb state - Get next instruction addr (LR+2) */
    ldrhne r2, [r1] /*                    - Get 2-byte instruction encoding */
    subeq r1, #4    /* If svc didn't exec in Thumb state - Update PC (LR+4) */
    ldreq r2, [r1]  /*                                   - Get 4-byte instr */

    /* Get SVC call number - LS Byte of the svc instruction executed
     *  0 - Context Switch
     *  1 - IRQ offload
     *  2 - Fatal Exception
     *  0xF0 - Disable IRQ
     *  0xF1 - Disable FIQ
     *  0xF2 - Disable FIQ and IRQ
     *  0xF3 - Enable IRQ
     *  0xF4 - Enable FIQ
     *  0xF5 - Enable FIQ and IRQ
     */
    ands r2, #0xff
    cmp r2, #0
    beq _svc_0
    cmp r2, #_SVC_CALL_IRQ_OFFLOAD
    beq _svc_irq_offload
    cmp r2, _SVC_CALL_RUNTIME_EXCEPT
    beq _svc_runtime_except
    cmp r2, _SVC_CALL_CPSID_I
    beq _svc_cpsid_i
    cmp r2, _SVC_CALL_CSPID_F
    beq _svc_cpsid_f
    cmp r2, _SVC_CALL_CPSID_IF
    beq _svc_cpsid_if
    cmp r2, _SVC_CALL_CPSIE_I
    beq _svc_cpsie_i
    cmp r2, _SVC_CALL_CPSIE_F
    beq _svc_cpsie_f
    cmp r2, _SVC_CALL_CPSIE_IF
    beq _svc_cpsie_if

    /* Fall through all other svc call numebrs: call  fatal error handler */
    /* SVC call 2 - k_oops/k_panic - Call z_NanoFatalErrorHandler */
_svc_runtime_except:
    /* Populate default ESF and call z_NanoFatalErrorHandler */
    mov r0, #_NANO_ERR_KERNEL_OOPS
    ldr r1, =_default_esf

    ldr r2, [sp, #8]   /* r0 */
    str r2, [r1]
    mov r0, r2         /* r0 has the reason for z_NanoFatalErrorHandler() */

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

    blx z_NanoFatalErrorHandler

    /* Perform context switch if fatal error handler returns */
    b _check_mode_and_switch_context

    /* SVC call to disable IRQ - SPSR is at the top of the stack */
_svc_cpsid_i:
    pop {r3}
    orr r3, r3, #IRQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call to disable FIQ - SPSR is at the top of the stack */
_svc_cpsid_f:
    pop {r3}
    orr r3, r3, #FIQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call to disable IRQ and FIQ - SPSR is at the top of the stack */
_svc_cpsid_if:
    pop {r3}
    orr r3, r3, #IRQ_DISABLE_MASK
    orr r3, r3, #FIQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call to enable IRQ - SPSR is at the top of the stack */
_svc_cpsie_i:
    pop {r3}
    bic r3, #IRQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call to enable FIQ - SPSR is at the top of the stack */
_svc_cpsie_f:
    pop {r3}
    bic r3, #FIQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call to enable IRQ and FIQ - SPSR is at the top of the stack */
_svc_cpsie_if:
    pop {r3}
    bic r3, #IRQ_DISABLE_MASK
    bic r3, #FIQ_DISABLE_MASK
    push {r3}
    b _no_context_switch

    /* SVC call 1 - IRQ Offload */
_svc_irq_offload:
    /* Call _irq_do_offload() */
#if CONFIG_IRQ_OFFLOAD
    blx _irq_do_offload  /* call C routine which executes the offload */
    b _check_mode_and_switch_context
#endif

_check_mode_and_switch_context:
    ldr r3, [sp]       /* Get SPSR */
    and r3, #CPU_MODE_MASK
    cmp r3, #CPU_MODE_SYS /* Check if we came from system mode */
    bne _no_context_switch

    ldr r1, =_kernel
    ldr r2, [r1, _kernel_offset_to_ready_q_cache]
    ldr r1, [r1, #_kernel_offset_to_current]
    cmp r1, r2
    beq _no_context_switch
    b _switch_context

_no_context_switch:
    /* Restore caller-saved regs */
    pop {r3}
    msr spsr, r3
    pop {lr}
    pop {r0 - r3, r12}
    movs pc, lr

_switch_context:
    /* Call check_stack sentinel */
#ifdef CONFIG_STACK_SENTINEL
    bl z_check_stack_sentinel
#endif
    b _svc_0

    /* SVC call 0 - Perform a context switch */
_svc_0:
    /* Perform context switch here */

    /* Lock all interrupts */
    mrs r3, cpsr    /* Save current psr */
    cpsid ifa

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    stmia r0!, {r4 - r11}   /* Save r4 - r11 */
    cps #CPU_MODE_SYS
    str sp, [r0]            /* Save r13 (sp) */
    cps #CPU_MODE_SVC

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_caller_saved
    add r0, r2

    /* Restore PC and PSR from appropriate mode */
    pop {r2}    /* CPSR to r2 */
    pop {r1}    /* PC to r1 */

    /* Save r0 - r3, r12 */
    pop {r4 - r7, r8}       /* r0 - r3, r12 -> r4 - r7, r8 */
    stmia r0!, {r4 - r7, r8}
    cps #CPU_MODE_SYS
    stm r0!, {r14}          /* Save r14 (lr) */
    cps #CPU_MODE_SVC

    /* To save the PC and the PSR of the current thread, the system mode
     * version of these registers are required. If the svc #0 was invoked
     * through __swap() call, then the LR_SVC and SPSR_SVC will hold these
     * values. Otherwise if svc #0 is triggered in a exception exit (_exc_exit),
     * which would occur only if the SPSR.M == SYS, then PC and PSR will be
     * available in the LR_XXX and SPSR_XXX registers, where XXX indicates the
     * mode from the svc #0 was triggered.
     */
    mov r12, r2
    and r12, #CPU_MODE_MASK
    cmp r12, #CPU_MODE_SYS
    beq _save_pc_spr_from_sys_mode

    cmp r12, #CPU_MODE_FIQ
    beq _save_pc_spr_from_fiq_mode

    cmp r12, #CPU_MODE_IRQ
    beq _save_pc_spr_from_irq_mode

    cmp r12, #CPU_MODE_ABT
    beq _save_pc_spr_from_abt_mode

    cmp r12, #CPU_MODE_UND
    beq _save_pc_spr_from_und_mode

_save_pc_spr_from_abt_mode:
    cps #CPU_MODE_ABT
    str lr, [r0]
    mrs r2, spsr
    str r2, [r0, #4]
    cps #CPU_MODE_SVC
    b _done_saving_pc_spr

_save_pc_spr_from_und_mode:
    cps #CPU_MODE_UND
    str lr, [r0]
    mrs r2, spsr
    str r2, [r0, #4]
    cps #CPU_MODE_SVC
    b _done_saving_pc_spr

_save_pc_spr_from_fiq_mode:
    cps #CPU_MODE_FIQ
    str lr, [r0]
    mrs r2, spsr
    str r2, [r0, #4]
    cps #CPU_MODE_SVC
    b _done_saving_pc_spr

_save_pc_spr_from_irq_mode:
    cps #CPU_MODE_IRQ
    str lr, [r0]
    mrs r2, spsr
    str r2, [r0, #4]
    cps #CPU_MODE_SVC
    b _done_saving_pc_spr

_save_pc_spr_from_sys_mode:
    str r1, [r0]
    str r2, [r0, #4]

_done_saving_pc_spr:

    /* reload _kernel to r1 */
    ldr r1, =_kernel

#ifdef CONFIG_FP_SHARING
    ldr r2, [r1, _kernel_offset_to_current]
    add r0, r2, #_thread_offset_to_preempt_float
    vmrs r2, fpscr
    stm r0!, {r2}
    vstmia r0!, {d0-d15}
    vstmia r0, {d16-d31}
#endif /* CONFIG_FP_SHARING */

    /* Restore CPSR */
    msr cpsr, r3

    /* fetch the thread to run from the ready queue cache */
    ldr r2, [r1, _kernel_offset_to_ready_q_cache]
    str r2, [r1, #_kernel_offset_to_current]

#ifdef CONFIG_EXECUTION_BENCHMARKING
    blx read_timer_end_of_swap
#endif /* CONFIG_EXECUTION_BENCHMARKING */

#ifdef CONFIG_TRACING
    blx z_sys_trace_thread_switched_in
#endif /* CONFIG_TRACING  */

#ifdef CONFIG_FP_SHARING
    /* r2 is already holding _kernel->current */
    add r0, r2, #_thread_offset_to_preempt_float
    ldm r0!, {r3}
    vmsr fpscr, r3
    vldmia r0!, {d0-d15}
    vldmia r0, {d16-d31}
#endif /* CONFIG_FP_SHARING */

    /* Restore callee-saved regs
     * r2 still has _kernel_offset_to_current
     * addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    ldmia r0!, {r4 - r11}
    cps #CPU_MODE_SYS
    ldr sp, [r0]
    cps #CPU_MODE_SVC

    /* Restore caller-saved regs */
    ldr r0, =_thread_offset_to_caller_saved
    add r0, r2

    ldr lr, [r0, #24]   /* Thread PC -> LR */

    ldr r12, [r0, #28]  /* PSR  -> SPSR */
    msr spsr, r12

    add r3, r0, #20     /* Address to LR_SYS */
    cps #CPU_MODE_SYS
    ldr r14, [r3]       /* Thread LR -> LR_SYS */
    cps #CPU_MODE_SVC

    ldr r12, [r0, #16]  /* Restore r12 */

    ldm r0, {r0 - r3}   /* Restore r0 - r3 */

    subs pc, lr, #0     /* PC <- LR & CPSR <- SPSR */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV7_A */
