/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Reset handler:
 *	- Initializes the stack for various CPU modes
 *	- Initializes data section
 *	- Enables I$ and configures VBAR with excpetion vector table address
 *	- Checks if the system is rcoming out of power saving mode and takes
 *	  appropriate action
 */

.section ".text.startup"

/* Some drivers refer to code entry point as __start */
.global __start
.global _start

	/* Aliases for mode encodings - do not change */
	.equ MODE_USR, 0x10
	.equ MODE_FIQ, 0x11
	.equ MODE_IRQ, 0x12
	.equ MODE_SVC, 0x13
	.equ MODE_MON, 0x16
	.equ MODE_ABT, 0x17
	.equ MODE_UND, 0x1B
	.equ MODE_SYS, 0x1F
	.equ MODE_HYP, 0x1A

__start: /* Some drivers refer to code entry point as __start */
_start:
	ldr pc, _reset_handler_addr
	ldr pc, _undef_instr_addr
	ldr pc, _svc_handler_addr
	ldr pc, _prefetch_abort_addr
	ldr pc, _data_abort_addr
	ldr pc, _unused_handler_addr
	ldr pc, _irq_handler_addr
	ldr pc, _fiq_handler_addr

	_reset_handler_addr:		.word _reset
	_undef_instr_addr:		.word __undef
	_svc_handler_addr:		.word os_svc_handler
	_prefetch_abort_addr:		.word __prefetch_abort
	_data_abort_addr:		.word __data_abort
	_unused_handler_addr:		.word _reset
	_irq_handler_addr:		.word os_irq_handler
	_fiq_handler_addr:		.word os_fiq_handler

 jumpaddr:        .word 0x30018028
 drips_deepsleep: .word 0x30024c80

_reset:
	/* Check & jump - For debugging DRIPS wakeup */
	ldr r0, drips_deepsleep
	ldr r0, [r0, #0]
	ldr r1, jumpaddr
	ldr r1, [r1, #0]
	uxtb	r2, r0
	cmp	r2, #2
	bne _coldboot
	/* Switch to User / System Mode - The original mode during save */
	cps #31
	ldr sp, =__RSTR_STACK_START__
	mov pc, r1

_coldboot:
	/* I+BTB cache invalidate */
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	/* Read in the System Control Register */
	mrc p15, 0, r0, c1, c0, 0
	/* Set i and d-cache bits */
	movw r1, #0x1000
	orr r0, r0, r1

	/* VBAR Setup */
	/* Clear the V bit enabling Relocation of VBAR */
	movw r1, #8192
	mvn r1, r1
	and r0, r0, r1

	/* Write Back the System Control Register */
	mcr p15, 0, r0, c1, c0, 0
	/* i and d-cache sync barriers now that it's enabled */
	dsb
	isb

	/* Write the reset handler addr to VBAR */
	ldr r0, =_start
	mcr p15, 0, r0, c12, c0, 0

	/* Set the stacks starting at __STACKS_START__ and decending */

	/* Switch to SVC mode & setup Supervisor Stack Pointer */
	cps #MODE_SVC
	ldr sp, =__SVC_STACK_START__

	/* Switch to FIQ mode and setup FIQ Stack Pointer */
	cps #MODE_FIQ
	ldr sp, =__FIQ_STACK_START__

	/* Switch to IRQ mode and setup IRQ Stack Pointer */
	cps #MODE_IRQ
	ldr sp, =__IRQ_STACK_START__

	/* Switch to Abort Stack Pointer */
	cps #MODE_ABT
	ldr sp, =__ABT_STACK_START__

	/* Switch to Undefined Stack Pointer */
	cps #MODE_UND
	ldr sp, =__UND_STACK_START__

	/* Switch to System Stack Pointer */
	cps #MODE_SYS
	ldr sp, =__USR_STACK_START__

	/* Copy the data segment initializers from flash to SRAM */
	movs r1, #0
	b LoopCopyDataInit

CopyDataInit:
	ldr r3, =_sidata
	ldr r3, [r3, r1]
	str r3, [r0, r1]
	adds r1, r1, #4

LoopCopyDataInit:
	ldr r0, =_sdata
	ldr r3, =_edata
	adds r2, r0, r1
	cmp r2, r3
	bcc CopyDataInit

	/* Call start_scheduler() - should not return and will call main */
	b start_scheduler

_inf_loop:
	b _inf_loop
