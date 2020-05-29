/*
 * Copyright (c) 2018 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <arch/cpu.h>
#include <cortex_a/barrier.h>
#include <cortex_a/cache.h>
#include <cortex_a/cp15.h>
#include "zephyr/types.h"
#include "pm_regs.h"
#include "M0_mproc_common.h"
#include <logging/sys_log.h>
#include "save_restore.h"

/* FIXMEs:
 * 1. Consider trimming the number of times isb, dsb & dmb are called.
 * It may not be necessary to call sync barriers so often.
 */

/***********************************************
 * Private Defines
 ***********************************************/
/* For now these mappings are just dummy 1 - to - 1 mappings
 * TODO:
 * When we have full fledged virtual memory, this should
 * be converted to a proper function.
 */
#define IO_VIRT_TO_PHYS(v) (v)
#define IO_PHYS_TO_VIRT(p) (p)

#define TIMER_MASK_IRQ  0x2

/* Is LPAE being used ? - Mask */
#define TTBCR_EAE (0x1 << 31)

#define PFR0_THUMB_EE_SUPPORT (1<<12)

#define JUMP_ADDRESS (CRMU_M0_SCRAM_START + 0x28)

/***********************************************
 * Private Data Structures to define CPU Context
 ***********************************************/
/* GIC 400 Context
 * Local to the CPU
 */
typedef struct {
	/* GIC context local to the CPU */
    u32_t gic_cpu_if_regs[32];
	/* GIC SGI/PPI context local to the CPU */
    u32_t gic_dist_if_pvt_regs[32];
} gic_cpu_ctx_t;

/* CP15 Fault Status Registers */
typedef struct {
    u32_t dfar;
    u32_t ifar;
    u32_t ifsr;
    u32_t dfsr;
    u32_t adfsr;
    u32_t aifsr;
} cp15_fault_regs_t;

typedef struct {
    u32_t cp15_misc_regs[2]; /* cp15 miscellaneous registers */
    u32_t cp15_ctrl_regs[20]; /* cp15 control registers */
    u32_t cp15_mmu_regs[16]; /* cp15 mmu registers */
    cp15_fault_regs_t cp15_fault_regs; /* cp15 fault status registers */
} cp15_ctx_t;

/* A7 Generic Timer - Source of Sys Tick */
typedef struct {
    u32_t cntfrq;    /* Counter Frequency Register */
    u64_t cntvoff;
    u32_t cnthctl;
    u32_t cntkctl;   /* Timer PL1 Control Register */
    u64_t cntp_cval; /* Physical Timer Count Compare Value Register */
    u32_t cntp_tval; /* PL1 Physical Timer Value Register */
    u32_t cntp_ctl;  /* PL1 Physical Timer Control Register */
    u64_t cntv_cval; /* Virtual Timer Count Compare Value Register */
    u32_t cntv_tval; /* Virtual Timer Value Register */
    u32_t cntv_ctl;  /* Virtual Timer Control Register */
    u64_t cnthp_cval;
    u32_t cnthp_tval;
    u32_t cnthp_ctl;
} gen_tmr_ctx_t;

typedef struct {
    u32_t gic_dist_if_regs[512]; /* GIC distributor context */
    u32_t generic_timer_regs[8]; /* Global timers */
} global_ctx_t;

typedef struct {
    u32_t banked_cpu_regs[32]; /* Banked cpu registers */
    cp15_ctx_t cp15_regs; /* cp15 context */
	/* Generic Timer Registers */
    gen_tmr_ctx_t timer_ctx;
	/* CPU Local GIC distributor and interface context */
    gic_cpu_ctx_t gic_cpu_ctx;
	/* Global Context (GIC Distributor & Timers) */
	global_ctx_t gbl_ctx;
	/* Placeholder for VFP context. */
	/* VFP registers are 8 byte registers */
	u32_t vfp_regs[2 + (sizeof(double)/sizeof(u32_t))*32];
} soc_ctx_t;


/* The Data Object holding the entire context of the A7 CPU core
 */
soc_ctx_t soc_ctx;

struct set_and_clear_regs {
    volatile u32_t set[32];
	volatile u32_t clear[32];
};

typedef struct {
    volatile u32_t control;             /* 0x000 */
    const u32_t controller_type;
    const u32_t implementer;
    const char padding1[116];
    volatile u32_t security[32];        /* 0x080 */
    struct set_and_clear_regs enable;   /* 0x100 */
    struct set_and_clear_regs pending;  /* 0x200 */
    struct set_and_clear_regs active;   /* 0x300 */
    volatile u32_t priority[256];       /* 0x400 */
    volatile u32_t target[256];         /* 0x800 */
    volatile u32_t configuration[64];   /* 0xC00 */
    const char padding3[512];           /* 0xD00 */
    volatile u32_t software_interrupt;  /* 0xF00 */
    volatile u32_t sgi_clr_pending[4];  /* 0xF10 */
    volatile u32_t sgi_set_pending[4];  /* 0xF20 */
    const char padding4[176];
    const u32_t peripheral_id[4];       /* 0xFE0 */
    const u32_t primecell_id[4];        /* 0xFF0 */
} intr_distributor_t;

typedef struct
{
    volatile u32_t control;                      /* 0x00 */
    volatile u32_t priority_mask;                /* 0x04 */
    volatile u32_t binary_point;                 /* 0x08 */
    volatile const u32_t interrupt_ack;          /* 0x0c */
    volatile u32_t end_of_interrupt;             /* 0x10 */
    volatile const u32_t running_priority;       /* 0x14 */
    volatile const u32_t highest_pending;        /* 0x18 */
    volatile u32_t aliased_binary_point;         /* 0x1c */
    volatile const u32_t aliased_interrupt_ack;  /* 0x20 */
    volatile u32_t alias_end_of_interrupt;       /* 0x24 */
    volatile const u32_t alias_highest_pending;  /* 0x28 */
} cpu_interface_t;


/*****************************************************
 * Private Helper Function
 *****************************************************/

/**
 * Description: This is a private version of the memcpy.
 * The reason it is kept here is to allow locating it in
 * a non-volatile region of memory where all save restore
 * functions would go. In DRIPS mode, the SRAM is ON and
 * retains content, but just in case we decide to have
 * ability to resume from Deep Sleep.
 *
 * This function requires stack to have been setup.
 * TODO: Check range overlap between (src, src+length)
 * & (dst, dst + length)
 *
 * Arg 1: r0: Destination start address (must be word aligned)
 * Arg 2: r1: Source start address (must be word aligned)
 * Arg 3: r2: Number of words to copy
 *
 * Return value is updated destination pointer (first unwritten word)
 */
static volatile u32_t *copy_words(volatile u32_t *dest,
		volatile u32_t *src, u32_t num_words)
{
	while (num_words--)
		*dest++ = *src++;

	return dest;
}

/**
 * Save CP15 Fault Status Registers
 */
void save_fault_status(u32_t *ptr)
{
	instr_sync_barrier();
	data_sync_barrier();

	*ptr++ = read_dfar(); /* read DFAR */
	*ptr++ = read_ifar(); /* read IFAR */
	*ptr++ = read_dfsr(); /* read DFSR */
	*ptr++ = read_ifsr(); /* read IFSR */
	*ptr++ = read_adfsr(); /* read ADFSR */
	*ptr = read_aifsr(); /* read AIFSR */

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore CP15 Fault Status Registers
 */
void restore_fault_status(u32_t *ptr)
{
	instr_sync_barrier();
	data_sync_barrier();

	write_dfar(*ptr++); /* write DFAR */
	write_ifar(*ptr++); /* write IFAR */
	write_dfsr(*ptr++); /* write DFSR */
	write_ifsr(*ptr++); /* write IFSR */
	write_adfsr(*ptr++); /* write ADFSR */
	write_aifsr(*ptr); /* write AIFSR */

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Stop & Disable the Generic Timer
 */
void stop_generic_timer(u32_t val)
{
	instr_sync_barrier();
	data_sync_barrier();

	/* Disable the Generic timer and mask the irq
	 * CNTP_CTL.ISTATUS = 0, CNTP_CTL.IMASK = 1, CNTP_CTL.ENABLE = 0
	 */
	write_cntp_ctl(val);
	instr_sync_barrier();
}

#ifdef SAVE_RESTORE_GENERIC_TIMER
/**
 * Save State of Generic Timer
 *
 * If r1 is 0, we assume that the OS is not using
 * the Virtualization extensions, and that the warm
 * boot code will set up CNTHCTL correctly.
 * If r1 is non-zero then CNTHCTL is saved and restored.
 *
 * TODO: Check if CNTP_CVAL is in the always-on domain
 * and will it be preserved as it is?
 */
void save_generic_timer(u32_t *ptr, int is_hyp)
{
	instr_sync_barrier();
	data_sync_barrier();

	/* read CNTP_CTL */
	*ptr++ = read_cntp_ctl();
	/* read CNTP_TVAL */
	*ptr++ = read_cntp_tval();
	/* read CNTKCTL */
	*ptr++ = read_cntkctl();

	if(is_hyp != 0) {
		*ptr = read_cnthctl();
	}

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore State of Generic Timer
 *
 * If r1 is 0, we assume that the OS is not using
 * the Virtualization extensions, and that the warm
 * boot code will set up CNTHCTL correctly.
 * If r1 is non-zero then CNTHCTL is saved and restored.
 *
 * TODO: Check if CNTP_CVAL is in the always-on domain
 * and will it be preserved as it is?
 */
void restore_generic_timer(u32_t *ptr, int is_hyp)
{
	instr_sync_barrier();
	data_sync_barrier();

	/* read CNTP_CTL */
	write_cntp_ctl(*ptr++);
	/* read CNTP_TVAL */
	write_cntp_tval(*ptr++);
	/* read CNTKCTL */
	write_cntkctl(*ptr++);

	if(is_hyp != 0) {
		write_cnthctl(*ptr);
	}

	instr_sync_barrier();
	data_sync_barrier();
}
#endif

/**
 * Save VFP State (Floating Point Unit Registers)
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
void save_vfp(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	instr_sync_barrier();
	data_sync_barrier();

	/* FPU state save/restore.
	 * FPSID,MVFR0 and MVFR1 don't get
	 * serialized/saved (Read Only).
	 */
	/* CPACR allows CP10 and CP11 access */
	__asm volatile("mrc p15, 0, r3, c1, c0, 2\n");
	__asm volatile("orr r2, r3, #0xF00000\n");
	__asm volatile("mcr p15, 0, r2, c1, c0, 2\n");

	instr_sync_barrier();

	__asm volatile("mrc p15, 0, r2, c1, c0, 2\n");
	__asm volatile("and r2, r2, #0xF00000\n");
	__asm volatile("cmp r2, #0xF00000\n");
	__asm volatile("beq 0f\n");
	__asm volatile("movs r2, #0\n");
	__asm volatile("b 2f\n");

	/* Save configuration registers and enable. */
	__asm volatile("0:\n");
	/* vmrs   r12, FPEXC
	 * Read Floating Point Exception Register into r12
	 */
	__asm volatile("FMRX r12, FPEXC\n");
	/* Save the FPEXC */
	__asm volatile("str r12, [r0], #4\n");

	/* Enable FPU access to save/restore the other registers. */
	__asm volatile("ldr r2,=0x40000000\n");
	/* vmsr FPEXC, r2
	 * Write the pattern into Floating Point Exception Register
	 */
	__asm volatile("FMXR FPEXC, r2\n");
	/* vmrs r2, FPSCR
	 * Read Contents of Floating Point Status & Control Register to r2
	 */
	__asm volatile("FMRX r2, FPSCR\n");
	/* Save the FPSCR */
	__asm volatile("str r2, [r0], #4\n");
	/* Store the VFP-D16 registers. */
	__asm volatile("vstm r0!, {D0-D15}\n");

	/* Check for Advanced SIMD/VFP-D32 support */
	/* vmrs   r2,MVFR0
	 * Read Contents of Media & Floating Point Feature Register to r2
	 */
	__asm volatile("FMRX r2, MVFR0\n");
	/* extract the A_SIMD bitfield */
	__asm volatile("and r2, r2, #0xF\n");
	__asm volatile("cmp r2, #0x2\n");
	__asm volatile("blt 1f\n");

	/* Store the Advanced SIMD/VFP-D32 additional registers.
	 * Extension Register Store Multiple
	 */
	__asm volatile("vstm   r0!, {D16-D31}\n");

	/* IMPLEMENTATION DEFINED:
	 * save any subarchitecture defined state
     * NOTE: Don't change the order of the FPEXC
	 * and CPACR restores
     *
     * Restore the original En bit of FPU.
	 */
	__asm volatile("1:\n");
	/* vmsr   FPEXC,r12
	 * Restore Floating Point Exception Register from r12
	 */
	__asm volatile("FMXR FPEXC, r12\n");

	/* Restore the original CPACR value. */
	__asm volatile("2: \n");
	__asm volatile("mcr p15, 0, r3, c1, c0, 2\n");

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore VFP State (Registers)
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
void restore_vfp(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	instr_sync_barrier();
	data_sync_barrier();

	/* FPU state save/restore. Obviously FPSID, MVFR0
	 * and MVFR1 don't get serialized (RO).
	 */
	/* Modify CPACR to allow CP10 and CP11 access */
	__asm volatile("mrc p15, 0, r1, c1, c0, 2\n");
	__asm volatile("orr r2, r1, #0x00F00000\n");
	__asm volatile("mcr p15, 0, r2, c1, c0, 2 \n");

	/* Enable FPU access to save/restore the rest of registers. */
	__asm volatile("ldr r2,=0x40000000\n");
	/* vmsr FPEXC, r2 */
	__asm volatile("FMXR FPEXC, r2\n");
	/* Recover FPEXC and FPSCR. These will be restored later. */
	__asm volatile("ldm r0!, {r3, r12}\n");
	/* Restore the VFP-D16 registers. */
	__asm volatile("vldm r0!, {D0-D15}\n");
	/* Check for Advanced SIMD/VFP-D32 support */
	/* vmrs r2, MVFR0 */
	__asm volatile("FMRX r2, MVFR0\n");
	/* extract the A_SIMD bitfield */
	__asm volatile("and r2,r2,#0xF\n");
	__asm volatile("cmp r2, #0x2\n");
	__asm volatile("blt 0f\n");

	/* Store the Advanced SIMD/VFP-D32 additional registers. */
	__asm volatile("vldm    r0!, {D16-D31}\n");

	/* IMPLEMENTATION DEFINED: restore any subarchitecture defined state */
	__asm volatile("0:\n");

	/* Restore configuration registers and enable.
	 * Restore FPSCR _before_ FPEXC since FPEXC
	 * could disable FPU and make setting FPSCR
	 * unpredictable.
	 */
	/* vmsr FPSCR, r12 */
	__asm volatile("FMXR FPSCR, r12\n");
	/* Restore FPEXC after FPSCR */
	/* vmsr FPEXC, r3 */
	__asm volatile("FMXR FPEXC, r3\n");
	/* Restore CPACR
	 * TODO: Is this the right place to restore CPACR?
	 * May be this should be done later?
	 *
	 * __asm volatile("mcr p15, 0, r1, c1, c0, 2\n");
	 */

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Save MMU Registers
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
void save_mmu(u32_t *ptr)
{
	u32_t ttbcr;

	instr_sync_barrier();
	data_sync_barrier();

	/* ASSUMPTION: no useful fault address / fault status information */
	/* VBAR */
	*ptr++ = read_vbar();
	/* TTBCR */
	ttbcr = read_ttbcr();
	*ptr++ = ttbcr;
	/* Are we using LPAE? Is the 40-bit Translation system
	 * enabled?
	 */
	if(ttbcr & TTBCR_EAE) {
		/* save 64 bit TTBRs */
		/* 64 bit TTBR0 & TTBR1 */
		read_ttbr0_64(ptr, ptr+1);
		ptr = ptr + 2;
		read_ttbr1_64(ptr, ptr+1);
		ptr = ptr + 2;
	}
	else {
		/* 32 bit TTBR0 */
		*ptr++ = read_ttbr0();
		/* 32 bit TTBR1 */
		*ptr++ = read_ttbr1();
	}
	/* Read DACR */
	*ptr++ = read_dacr();
	/* Read PAR */
	*ptr++ = read_par();
	/* Read PRRR */
	*ptr++ = read_prrr();
	/* Read NMRR */
	*ptr = read_nmrr();

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore MMU Registers
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
void restore_mmu(u32_t *ptr)
{
	u32_t ttbcr;

	instr_sync_barrier();
	data_sync_barrier();

	/* VBAR */
	write_vbar(*ptr++);
	/* TTBCR */
	ttbcr = *ptr;
	write_ttbcr(*ptr++);

	/* Are we using LPAE? Is the 40-bit Translation system
	 * enabled?
	 */
	if(ttbcr & TTBCR_EAE) {
		/* Restore 64 bit TTBRs */
		/* 64 bit TTBR0 & TTBR1 */
		write_ttbr0_64(ptr, ptr+1);
		ptr = ptr + 2;
		write_ttbr1_64(ptr, ptr+1);
		ptr = ptr + 2;
	}
	else {
		/* 32 bit TTBR0 */
		write_ttbr0(*ptr++);
		/* 32 bit TTBR1 */
		write_ttbr1(*ptr++);
	}

	/* Write DACR */
	write_dacr(*ptr++);
	/* Write PAR */
	write_par(*ptr++);
	/* Write PRRR */
	write_prrr(*ptr++);
	/* Write NMRR */
	write_nmrr(*ptr);

	instr_sync_barrier();
	data_sync_barrier();
}

#ifdef SAVE_RESTORE_MPU_REGISTERS
/**
 * Save MPU Registers
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
static void save_mpu(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	 /* Read MPUIR */
	__asm volatile("mrc p15, 0, r1, c0, c0, 4\n");
	__asm volatile("and r1, r1, #0xff00\n");
	/* Extract number of MPU regions */
	__asm volatile("lsr r1, r1, #8\n");

	/* Loop over the number of regions */
	__asm volatile("10:\n");
	__asm volatile("cmp r1, #0\n");
	__asm volatile("beq 20f\n");

	__asm volatile("sub r1, r1, #1\n");
	/* Write RGNR */
	__asm volatile("mcr p15, 0, r1, c6, c2, 0\n");
	/* Read MPU Region Base Address Register */
	__asm volatile("mrc p15, 0, r2, c6, c1, 0\n");
	/* Read Data MPU Region Size and Enable Register */
	__asm volatile("mrc p15, 0, r3, c6, c1, 2\n");
	/* Read Region access control Register */
	__asm volatile("mrc p15, 0, r12, c6, c1, 4\n");
	__asm volatile("stm r0!, {r2, r3, r12}\n");
	__asm volatile("b 10b\n");

	__asm volatile("20:\n");
}

/**
 * Restore MPU Registers
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have any save and restore order restriction.
 *
 */
static void restore_mpu(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	/* Read MPUIR */
	__asm volatile("mrc p15, 0, r1, c0, c0, 4\n");
	__asm volatile("and r1, r1, #0xff00\n");
	/* Extract number of MPU regions */
	__asm volatile("lsr r1, r1, #8\n");

	/* Loop over the number of regions */
	__asm volatile("10:\n");
	__asm volatile("cmp r1, #0\n");
	__asm volatile("beq 20f\n");

	__asm volatile("ldm r0!, {r2, r3, r12}\n");
	__asm volatile("sub r1, r1, #1\n");
	/* Write RGNR */
	__asm volatile("mcr p15, 0, r1, c6, c2, 0 \n");
	/* Write MPU Region Base Address Register */
	__asm volatile("mcr p15, 0, r2, c6, c1, 0 \n");
	/* Write Data MPU Region Size and Enable Register */
	__asm volatile("mcr p15, 0, r3, c6, c1, 2 \n");
	/* Write Region access control Register */
	__asm volatile("mcr p15, 0, r12, c6, c1, 4\n");
	__asm volatile("b 10b\n");

	__asm volatile("20:\n");
}
#endif

/**
 * Save CP15 Register Values
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have save and restore order that relate
 * to other CP15 registers or logical grouping requirements.
 *
 * Proprietary features may include TCM support, lockdown support etc.
 * TODO: Check the ROM lockdown feature and re-confirm implications
 * therefore occur at any point in this sequence.
 */
void save_cp15(u32_t *ptr)
{
	instr_sync_barrier();
	data_sync_barrier();

	/* CSSELR Cache Size Selection Register */
	*ptr = read_csselr();

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore CP15 Register Values
 *
 * NOTE:
 * TODO: Check if there are any Implementation Defined registers.
 * and if these registers have save and restore order that relate
 * to other CP15 registers or logical grouping requirements.
 *
 * Proprietary features may include TCM support, lockdown support etc.
 * TODO: Check the ROM lockdown feature and re-confirm implications
 * therefore occur at any point in this sequence.
 */
void restore_cp15(u32_t *ptr)
{
	instr_sync_barrier();
	data_sync_barrier();

	/* CSSELR Cache Size Selection Register */
	write_csselr(*ptr);

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Save CP15 Control Registers such as SCTLR, CPACR etc.
 * ptr: r0 contains address to store control registers
 * is_secure: r1 is non-zero if we are Secure
 */
void save_control_registers(u32_t *ptr, s32_t is_secure)
{
	u32_t id_pfr0;

	instr_sync_barrier();
	data_sync_barrier();

	/* 1. ACTLR - Auxiliary Control Register */
	*ptr++ = read_actlr();
	/* 2. SCTLR - System Control Register */
	*ptr++ = read_sctlr();
	/* 3. CPACR - Coprocessor Access Control Register */
	*ptr++ = read_cpacr();

	/* Are we Secure? TODO: Confirm if anything special
	 * need to be done if non-secure
	 */
	if(is_secure != 0) {
		/* 4. MVBAR - Monitor Vector Base Address Register */
		*ptr++ = read_mvbar();
		/* 5. Secure Configuration Register */
		*ptr++ = read_scr();
		/* 6. Secure Debug Enable Register */
		*ptr++ = read_sder();
		/* 7. Non-Secure Access Control Register */
		*ptr++ = read_nsacr();
	}

	/* 8. CONTEXTIDR */
	*ptr++ = read_contextidr();
	 /* 9. TPIDRURW */
	*ptr++ = read_tpidrurw();
	 /* 10. TPIDRURO */
	*ptr++ = read_tpidruro();
	/* 11. TPIDRPRW */
	*ptr++ = read_tpidrprw();

	/* The below two registers are only present
	 * if ThumbEE is implemented. TODO: Confirm
	 * if necessary.
	 * --- FIXME --- is the ID_PFR0 saved correctly??
	 */
	/* Read ID_PFR0 */
	id_pfr0 = read_id_pfr0();
	if(id_pfr0 & PFR0_THUMB_EE_SUPPORT) {
		/* 12. TEECR */
		*ptr++ = read_teecr();
		/* 13. TEEHBR */
		*ptr++ = read_teehbr();
	}

	/* 14. JOSCR */
	*ptr++ = read_joscr();
	/* 15. JMCR */
	*ptr = read_jmcr();

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Restore CP15 Control Registers such as SCTLR, CPACR etc.
 * ptr: r0 contains address to store control registers
 * is_secure: r1 is non-zero if we are Secure
 */
void restore_control_registers(u32_t *ptr, s32_t is_secure)
{
	u32_t id_pfr0;

	instr_sync_barrier();
	data_sync_barrier();

	/* 1. ACTLR - Auxiliary Control Register */
	write_actlr(*ptr++);
	/* 2. SCTLR - System Control Register */
	write_sctlr(*ptr++);
	/* 3. CPACR - Coprocessor Access Control Register */
	write_cpacr(*ptr++);

	/* Are we Secure? TODO: Confirm if anything special
	 * need to be done if non-secure
	 */
	if(is_secure != 0) {
		/* 4. MVBAR - Monitor Vector Base Address Register */
		write_mvbar(*ptr++);
		/* 5. Secure Configuration Register */
		write_scr(*ptr++);
		/* 6. Secure Debug Enable Register */
		write_sder(*ptr++);
		/* 7. Non-Secure Access Control Register */
		write_nsacr(*ptr++);
	}

	/* 8. CONTEXTIDR */
	write_contextidr(*ptr++);
	/* 9. TPIDRURW */
	write_tpidrurw(*ptr++);
	/* 10. TPIDRURO */
	write_tpidruro(*ptr++);
	/* 11. TPIDRPRW */
	write_tpidrprw(*ptr++);

	/* The below two registers are only present
	 * if ThumbEE is implemented. TODO: Confirm
	 * if necessary.
	 *
	 * --- FIXME --- is the ID_PFR0 saved correctly??
	 */
	/* Read ID_PFR0 */
	id_pfr0 = read_id_pfr0();
	if(id_pfr0 & PFR0_THUMB_EE_SUPPORT) {
		/* 12. TEECR */
		write_teecr(*ptr++);
		/* 13. TEEHBR */
		write_teehbr(*ptr++);
	}

	/* 14. JOSCR */
	write_joscr(*ptr++);
	/* 15. JMCR */
	write_jmcr(*ptr++);

	instr_sync_barrier();
	data_sync_barrier();
}


/**
 * Description: Saves all the general purpose registers
 * and the CPSR.
 *
 * This function requires stack to have been setup.
 * While going to sleep obviously we have the stack, but
 * consider the case of coming out of sleep.
 *
 * Arg 1: r0: Context Store address
 *
 * Return value: None
 */
void save_banked_registers(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	/* Aliases for mode encodings - do not change */
	__asm volatile(".equ MODE_USR, 0x10\n");
	__asm volatile(".equ MODE_FIQ, 0x11\n");
	__asm volatile(".equ MODE_IRQ, 0x12\n");
	__asm volatile(".equ MODE_SVC, 0x13\n");
	/* A-profile (Security Extensions) only */
	__asm volatile(".equ MODE_MON, 0x16\n");
	__asm volatile(".equ MODE_ABT, 0x17\n");
	__asm volatile(".equ MODE_UND, 0x1B\n");
	__asm volatile(".equ MODE_SYS, 0x1F\n");
	__asm volatile(".equ MODE_HYP, 0x1A\n");

	instr_sync_barrier();
	data_sync_barrier();

	/* 1. Save scratch reg & lr
	 * Yes, the function preamble would have pushed
	 * the lr into stack already, but we need it here
	 * so that we can explicitly access it to save it
	 * away into the context.
	 */
	__asm volatile("push   {r3, lr}\n");

	/* 2. save current mode */
	__asm volatile("mrs    r2, CPSR\n");

	/* 3. switch to System mode */
	__asm volatile("cps    #MODE_SYS\n");
	/* save the User SP */
	__asm volatile("str    sp, [r0], #4\n");
	/* save the User LR */
	__asm volatile("str    lr, [r0], #4\n");

	/* 4. switch to Abort mode */
	__asm volatile("cps    #MODE_ABT\n");
	/* save the current SP */
	__asm volatile("str    sp, [r0], #4\n");
	/* save the current SPSR, LR */
	__asm volatile("mrs    r3, SPSR\n");
	__asm volatile("stm    r0!, {r3, lr}\n");

	/* 5 switch to Undefined mode */
	__asm volatile("cps    #MODE_UND\n");
	/* save the current SP */
	__asm volatile("str    sp, [r0], #4\n");
	/* save the current SPSR, LR */
	__asm volatile("mrs    r3, SPSR    \n");
	__asm volatile("stm    r0!, {r3, lr}\n");

	/* 6. switch to IRQ mode */
	__asm volatile("cps    #MODE_IRQ\n");
	/* save the current SP */
	__asm volatile("str    sp, [r0], #4\n");
	/* save the current SPSR, LR */
	__asm volatile("mrs    r3, SPSR\n");
	__asm volatile("stm    r0!, {r3, lr}\n");

	/* 7. switch to FIQ mode */
	__asm volatile("cps    #MODE_FIQ\n");
	/* save the current SP */
	__asm volatile("str    SP, [r0], #4\n");
	/* save the current SPSR, r8-r12, LR */
	__asm volatile("mrs    r3, SPSR\n");
	__asm volatile("stm    r0!, {r3, r8-r12, lr}\n");

	/* 8. switch back to original mode */
	__asm volatile("msr    CPSR_cxsf, r2\n");
	__asm volatile("pop    {r3, lr}\n");

	/* save the current SP */
	__asm volatile("str    SP, [r0], #4\n");
	/* save the current CPSR, SPSR, r4-r12, LR */
	__asm volatile("mrs    r3, SPSR\n");
	__asm volatile("stm    r0!, {r2, r3, r4-r12, lr}\n");

	instr_sync_barrier();
	data_sync_barrier();
}

/**
 * Description: Restores all the general purpose registers
 * and the CPSR.
 *
 * This function requires stack to have been setup.
 * While going to sleep obviously we have the stack, but
 * consider the case of coming out of sleep.
 *
 * TODO: Verify the order & the offsets in the context data.
 *
 * Arg 1: r0: Context Store address
 *
 * Return value: None
 */
void restore_banked_registers(u32_t *ptr)
{
	/* Passed in register r0 */
	ARG_UNUSED(ptr);

	instr_sync_barrier();
	data_sync_barrier();

	/* Aliases for mode encodings - do not change */
	__asm volatile(".equ MODE_USR, 0x10\n");
	__asm volatile(".equ MODE_FIQ, 0x11\n");
	__asm volatile(".equ MODE_IRQ, 0x12\n");
	__asm volatile(".equ MODE_SVC, 0x13\n");
	/* A-profile (Security Extensions) only */
	__asm volatile(".equ MODE_MON, 0x16\n");
	__asm volatile(".equ MODE_ABT, 0x17\n");
	__asm volatile(".equ MODE_UND, 0x1B\n");
	__asm volatile(".equ MODE_SYS, 0x1F\n");
	__asm volatile(".equ MODE_HYP, 0x1A\n");

	/* 1. Save scratch reg & lr
	 * Yes, the function preamble would have pushed
	 * the lr into stack already, but we need it here
	 * so that we can explicitly access it to save it
	 * away into the context.
	 */
	/* save current mode */
	__asm volatile("mrs r2, CPSR\n");

	/* 1. switch to System mode */
	__asm volatile("cps #MODE_SYS\n");
	/* restore the User SP
	 * TODO: Confirm the saved one was actually
	 * system SP & LR at this offset
	 */
	__asm volatile("ldr sp, [r0], #4\n");
	/* restore the User LR */
	__asm volatile("ldr lr, [r0], #4\n");

	/* 2. switch to Abort mode */
	__asm volatile("cps #MODE_ABT\n");
	/* restore the current SP */
	__asm volatile("ldr sp, [r0], #4\n");
	/* restore the current LR */
	__asm volatile("ldm r0!, {r3, lr}\n");
	/* restore the current SPSR */
	__asm volatile("msr SPSR_fsxc, r3\n");

	/* 3. switch to Undefined mode */
	__asm volatile("cps #MODE_UND\n");
	/* restore the current SP */
	__asm volatile("ldr sp, [r0], #4\n");
	/* restore the current LR */
	__asm volatile("ldm r0!, {r3, lr}\n");
	/* restore the current SPSR */
	__asm volatile("msr SPSR_fsxc, r3\n");

	/* 4. switch to IRQ mode */
	__asm volatile("cps #MODE_IRQ\n");
	/* restore the current SP */
	__asm volatile("ldr sp, [r0], #4\n");
	/* restore the current LR */
	__asm volatile("ldm r0!, {r3, lr}\n");
	/* restore the current SPSR */
	__asm volatile("msr SPSR_fsxc, r3\n");

	/* 5. switch to FIQ mode */
	__asm volatile("cps #MODE_FIQ\n");
	/* restore the current SP */
	__asm volatile("ldr sp, [r0], #4\n");
	/* restore the current r8-r12,LR */
	__asm volatile("ldm r0!, {r3, r8-r12, lr}\n");
	/* restore the current SPSR */
	__asm volatile("msr SPSR_fsxc, r3\n");

	/* switch back to original mode */
	__asm volatile("msr CPSR_cxsf, r2\n");
	/* restore the current SP */
	__asm volatile("ldr SP, [r0], #4\n");
	/* restore the current r4-r12, LR */
	__asm volatile("ldm r0!, {r2, r3, r4-r12, LR} \n");
	/* restore the current SPSR */
	__asm volatile("msr SPSR_fsxc, r3\n");
	/* restore the current CPSR */
	__asm volatile("msr CPSR_fsxc, r2\n");

	instr_sync_barrier();
	data_sync_barrier();
}

/* Saves the GIC CPU interface context
 * Requires 4 words of memory
 */
void save_gic_interface(u32_t *ptr, u32_t gic_if_addr)
{
    cpu_interface_t *ci = (cpu_interface_t *)gic_if_addr;

	instr_sync_barrier();
	data_sync_barrier();

    ptr[0] = ci->control;
    ptr[1] = ci->priority_mask;
    ptr[2] = ci->binary_point;
    ptr[3] = ci->aliased_binary_point;

    /* TODO: add nonsecure stuff */

	instr_sync_barrier();
	data_sync_barrier();
}

/* Saves this CPU's banked parts of the distributor
 * Returns non-zero if an SGI/PPI interrupt is pending
 * (after saving all required context)
 * Requires 19 words of memory
 */
void save_gic_distributor_private(volatile u32_t*ptr,
		u32_t gic_distributor_addr)
{
    intr_distributor_t *id = (intr_distributor_t *)gic_distributor_addr;
    volatile u32_t *temp_ptr = 0x0;

	instr_sync_barrier();
	data_sync_barrier();

    /*  Save SGI,PPI enable status*/
    *ptr = id->enable.set[0];
    ++ptr;
    /*  Save SGI,PPI priority status*/
    ptr = copy_words(ptr, id->priority, 8);
    /*  Save SGI,PPI target status*/
    ptr = copy_words(ptr, id->target, 8);
    /*  Save just the PPI configurations (SGIs are not configurable) */
    *ptr = id->configuration[1];
    ++ptr;
    /*  Save SGI,PPI security status*/
    *ptr = id->security[0];
    ++ptr;

    /*  Save SGI,PPI pending status*/
    *ptr = id->pending.set[0];
    ++ptr;

    /* IPIs are different and can be replayed just by saving
     * and restoring the set/clear pending registers
     */
    temp_ptr = ptr;
    copy_words(ptr, id->sgi_set_pending, 4);
    ptr += 8; 

    /* Clear the pending SGIs on this cpuif so that they don't
     * interfere with the wfi later on.
     */
    copy_words(id->sgi_clr_pending, temp_ptr, 4);

	instr_sync_barrier();
	data_sync_barrier();
}

/* Saves the shared parts of the distributor
 * Requires 1 word of memory, plus 20 words for each block
 * of 32 SPIs (max 641 words)
 * Returns non-zero if an SPI interrupt is pending (after
 * saving all required context)
 */
void save_gic_distributor_shared(volatile u32_t *ptr,
		u32_t gic_distributor_addr)
{
	intr_distributor_t *id = (intr_distributor_t *)gic_distributor_addr;
	u32_t num_spis; /* *saved_pending; */

	/* Calculate how many SPIs the GIC supports */
	num_spis = 32 * (id->controller_type & 0x1f);

	instr_sync_barrier();
	data_sync_barrier();

	/* TODO: add nonsecure stuff */

	/* Save rest of GIC configuration */
	if (num_spis) {
		ptr = copy_words(ptr, id->enable.set + 1, num_spis / 32);
		ptr = copy_words(ptr, id->priority + 8, num_spis / 4);
		ptr = copy_words(ptr, id->target + 8, num_spis / 4);
		ptr = copy_words(ptr, id->configuration + 2, num_spis / 16);
		ptr = copy_words(ptr, id->security + 1, num_spis / 32);
		/*saved_pending = ptr; */
		ptr = copy_words(ptr, id->pending.set + 1, num_spis / 32);
	}

	/* Save control register */
	*ptr = id->control;

	instr_sync_barrier();
	data_sync_barrier();
}

/* Restores the GIC CPU interface context
 * Requires 4 words of memory
 */
void restore_gic_interface(u32_t *ptr, u32_t gic_interface_addr)
{
	cpu_interface_t *ci = (cpu_interface_t *)gic_interface_addr;

	instr_sync_barrier();
	data_sync_barrier();

	/* TODO: add nonsecure stuff */

	ci->priority_mask = ptr[1];
	ci->binary_point = ptr[2];
	ci->aliased_binary_point = ptr[3];

	/* Restore control register last */
	ci->control = ptr[0];

	instr_sync_barrier();
	data_sync_barrier();
}

/* Restores this CPU's banked parts of the distributor
 * (after restoring all required context)
 * Requires 19 words of memory
 */
void restore_gic_distributor_private(volatile u32_t *ptr,
		u32_t gic_distributor_address)
{
	intr_distributor_t *id = (intr_distributor_t *)gic_distributor_address;
	u32_t tmp;

	instr_sync_barrier();
	data_sync_barrier();

	/* First disable the distributor so we can write
	 * to its config registers
	 */
	tmp = id->control;
	id->control = 0;
	/* Restore SGI,PPI enable status*/
	id->enable.set[0] = *ptr;
	++ptr;
	/* Restore SGI,PPI priority  status*/
	copy_words(id->priority, ptr, 8);
	ptr += 8;
	/* Restore SGI,PPI target status*/
	copy_words(id->target, ptr, 8);
	ptr += 8;
	/* Restore just the PPI configurations (SGIs are not configurable) */
	id->configuration[1] = *ptr;
	++ptr;
	/* Restore SGI,PPI security status*/
	id->security[0] = *ptr;
	++ptr;

	/*  Restore SGI,PPI pending status*/
	id->pending.set[0] = *ptr;
	++ptr;

	/* Restore pending SGIs */
	copy_words(id->sgi_set_pending, ptr, 4);
	ptr += 4;

	id->control = tmp;

	instr_sync_barrier();
	data_sync_barrier();
}

/* Restore the shared parts of the distributor
 * Requires 1 word of memory, plus 20 words for each block
 * of 32 SPIs (max 641 words)
 * Returns non-zero if an SPI interrupt is pending (after
 * saving all required context)
 */
void restore_gic_distributor_shared(volatile u32_t *ptr, u32_t gic_distributor_addr)
{
	intr_distributor_t *id = (intr_distributor_t *)gic_distributor_addr;
	u32_t num_spis;
	s32_t i, j;
	u32_t crmu_iproc_intr_mask;
	u32_t crmu_iproc_intr_stat;

	instr_sync_barrier();
	data_sync_barrier();

	/* First disable the distributor so we can write to its config registers */
	id->control = 0;

	/* Calculate how many SPIs the GIC supports */
	num_spis = 32 * ((id->controller_type) & 0x1f);

	/* TODO: add nonsecure stuff */

	/* Restore rest of GIC configuration */
	if (num_spis) {
		copy_words(id->enable.set + 1, ptr, num_spis / 32);
		ptr += num_spis / 32;
		copy_words(id->priority + 8, ptr, num_spis / 4);
		ptr += num_spis / 4;
		copy_words(id->target + 8, ptr, num_spis / 4);
		ptr += num_spis / 4;
		copy_words(id->configuration + 2, ptr, num_spis / 16);
		ptr += num_spis / 16;
		copy_words(id->security + 1, ptr, num_spis / 32);
		ptr += num_spis / 32;
		copy_words(id->pending.set + 1, ptr, num_spis / 32);

		/* TODO (Important):
		 * Though we have restored the Pending Set to what
		 * it was before we went to sleep, while we were
		 * asleep, some wake-up event happened, and since the
		 * GIC was powered OFF, the wake-up interrupt / event
		 * would not have pended in GIC.
		 *
		 * So go ahead and read the CRMU registers that indicate
		 * that they have send interrupts and make them pend in GIC.
		 *
		 * One other possible approach is to have SBL do this job.
		 */
		crmu_iproc_intr_mask = sys_read32(IO_VIRT_TO_PHYS(CRMU_IPROC_INTR_MASK));
		crmu_iproc_intr_stat = sys_read32(IO_VIRT_TO_PHYS(CRMU_IPROC_INTR_STATUS));
		/* FIXME: Don't invoke printk here, it causes aborts on Zephyr
		 * though it works on OpenRTOS. May be it is owing to the difference
		 * in schedulers. Better to implement the raw writes into UART ports
		 * as was done in early versions of the Power Manager and keep it OS
		 * agnostic.
		 */
		SYS_LOG_DBG("CRMU_MPROC_INTR_MASK:%x", crmu_iproc_intr_mask);
		SYS_LOG_DBG("CRMU_MPROC_INTR_STAT:%x", crmu_iproc_intr_stat);

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_USBPHY1_WAKE_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_USBPHY1_WAKE_INTR))) {
		/* TODO: No Corresponding IRQ to Pend */
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_USBPHY0_WAKE_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_USBPHY0_WAKE_INTR))) {
		/* TODO: No Corresponding IRQ to Pend */
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_VOLTAGE_GLITCH_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_VOLTAGE_GLITCH_TAMPER_INTR))) {
			/* Need to Pend the Voltage Glitch Tamper Interrupt IRQ17 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_VOLTAGE_GLITCH_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_VOLTAGE_GLITCH_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_CRMU_ECC_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_CRMU_ECC_TAMPER_INTR))) {
			/* Need to Pend the ECC Tamper Interrupt IRQ18 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_ECC_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_ECC_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_RTIC_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_RTIC_TAMPER_INTR))) {
			/* Need to Pend the RTIC Tamper Interrupt IRQ19 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_RTIC_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_RTIC_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_CHIP_FID_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_CHIP_FID_TAMPER_INTR))) {
			/* Need to Pend the Chip FID Tamper Interrupt IRQ20 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CHIP_FID_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CHIP_FID_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_CRMU_FID_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_CRMU_FID_TAMPER_INTR))) {
			/* Need to Pend the CRMU FID Tamper Interrupt IRQ21 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_FID_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_FID_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_CRMU_CLK_GLITCH_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_CRMU_CLK_GLITCH_TAMPER_INTR))) {
			/* Need to Pend the CRMU Clock Glitch Tamper Interrupt IRQ22 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_CLK_GLITCH_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_CRMU_CLK_GLITCH_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_BBL_CLK_GLITCH_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_BBL_CLK_GLITCH_TAMPER_INTR))) {
			/* Need to Pend the CRMU MPROC BBL CLK Glitch Tamper Interrupt IRQ23 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_BBL_CLK_GLITCH_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_BBL_CLK_GLITCH_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_IHOST_CLK_GLITCH_TAMPER_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_IHOST_CLK_GLITCH_TAMPER_INTR))) {
			/* Need to Pend the CRMUp FID Tamper Interrupt IRQ24 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_IHOST_CLK_GLITCH_TAMPER_INTR) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_IHOST_CLK_GLITCH_TAMPER_INTR) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPRU_RTC_INTR))) {
			/* Need to Pend the CRMUp FID Tamper Interrupt IRQ25 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_RTC_EVENT_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_RTC_EVENT_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPRU_ALARM_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPRU_ALARM_INTR))) {
			/* Need to Pend the IPROC SPRU Alarm Interrupt IRQ26 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_ALARM_EVENT_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_ALARM_EVENT_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPL_FREQ_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPL_FREQ_INTR))) {
			/* Need to Pend the IPROC Special Frequency Interrupt IRQ27 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_FREQ_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_FREQ_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPL_PVT_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPL_PVT_INTR))) {
			/* Need to Pend the IPROC Special Private Interrupt IRQ28 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_PVT_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_PVT_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPL_RST_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPL_RST_INTR))) {
			/* Need to Pend the IPROC Special Reset Interrupt IRQ29 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_RST_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_RST_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPL_WDOG_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPL_WDOG_INTR))) {
			/* Need to Pend the IPROC Special WatchDog Interrupt IRQ30 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_WDOG_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPL_WDOG_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_MAILBOX_INTR))) {
			/* Need to Pend the Mail Box Interrupt IRQ31 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_AXI_DEC_ERR_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_AXI_DEC_ERR_INTR))) {
			/* Need to Pend the AXI Decoding Error Interrupt IRQ32 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_AXI_DEC_ERR_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_AXI_DEC_ERR_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if((crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_PERIODIC_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_SPRU_RTC_PERIODIC_INTR))) {
			/* Need to Pend the SPRU RTC Periodic Interrupt IRQ33 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		if(!(crmu_iproc_intr_mask & (0x1 << CRMU_IPROC_INTR_MASK__IPROC_M0_LOCKUP_INTR_MASK))
		&& (crmu_iproc_intr_stat & (0x1 << CRMU_IPROC_INTR_STATUS__IPROC_M0_LOCKUP_INTR))) {
			/* Need to Pend the IPROC Lock-Up Interrupt IRQ34 */
			i = SPI_IRQ(CHIP_INTR__CRMU_MPROC_M0_LOCKUP_INTERRUPT) / NUM_GIC_PRIVATE_SIGNALS;
			j = SPI_IRQ(CHIP_INTR__CRMU_MPROC_M0_LOCKUP_INTERRUPT) % NUM_GIC_PRIVATE_SIGNALS;
			id->pending.set[i] |= (1 << j);
		}

		ptr += num_spis / 32;
	}

	/* We assume the I and F bits are set in the CPSR so that
	 * we will not respond to interrupts!
	 */
	/* Restore control register */
	id->control = *ptr;

	instr_sync_barrier();
	data_sync_barrier();
}

/** This function is the main function that saves
 * context of the mproc (i.e. A7) subsystem of the SOC.
 *
 * Note: It is expected to be executed from SVC mode.
 */
void platform_save_context(void)
{
	u32_t lpstate;
	soc_ctx_t *soc_ctx_ptr = &soc_ctx;

	u32_t *gp_ctx_ptr = soc_ctx_ptr->banked_cpu_regs;
	cp15_ctx_t *cp15_ctx_ptr = &soc_ctx_ptr->cp15_regs;
	gic_cpu_ctx_t *gic_cpu_ctx_ptr = &soc_ctx_ptr->gic_cpu_ctx;
	global_ctx_t *gbl_ctx_ptr = &soc_ctx_ptr->gbl_ctx;
	cp15_fault_regs_t *fault_ctx = &cp15_ctx_ptr->cp15_fault_regs;
	u32_t *vfp_ctx = soc_ctx_ptr->vfp_regs;
#ifdef SAVE_RESTORE_GENERIC_TIMER
	gen_tmr_ctx_t *gen_tmr_ctx_ptr = &soc_ctx_ptr->timer_ctx;
#endif


	/* Indicate that We are Entering LP Mode.
	 * When we complete saving the context and this function returns,
	 * we need to send message to CRMU M0 to initiate the powering
	 * sequence of various Power Domains and Clock Domains.
	 *
	 * The problem is when restore context is executed, the control
	 * comes to the exact same point, as if it was return from call
	 * to save context function. In this case, where we are exiting
	 * from a LP mode, we need not follow the same sequence as we did
	 * while entering, i.e. sending request to CRMU.
	 *
	 * To differentiate between these two situations, we use bits 8-15
	 * of DRIPS_OR_DEEPSLEEP_REG, which is a persistent register.
	 */
	lpstate = sys_read32(DRIPS_OR_DEEPSLEEP_REG);
	lpstate = (lpstate & ENTRY_EXIT_MASK) | ENTERING_LP;
	sys_write32(lpstate, DRIPS_OR_DEEPSLEEP_REG);

	SYS_LOG_DBG("Saving Context...%x\n", lpstate);

	instr_sync_barrier();
	data_sync_barrier();

	/* 1. Save the 32-bit Generic timer context & stop them */
#ifdef SAVE_RESTORE_GENERIC_TIMER
	save_generic_timer((u32_t *)gen_tmr_ctx_ptr, 0x0);
#endif
	stop_generic_timer(TIMER_MASK_IRQ);

	/* 2. Save cp15 context */
	/* 2.1. Save cp15 Context Misc Registers */
	save_cp15(cp15_ctx_ptr->cp15_misc_regs);
	/* 2.2. Save cp15 Context Control Registers */
	save_control_registers(cp15_ctx_ptr->cp15_ctrl_regs, 0x0);
	/* 2.3. Save cp15 Context MMU Registers */
	save_mmu(cp15_ctx_ptr->cp15_mmu_regs);
	/* 2.4. Save cp15 Context Fault Registers */
	save_fault_status((u32_t *)fault_ctx);
	/* Save VFP Context */
	save_vfp(vfp_ctx);
	/* 3. Save GIC cpu interface (cpu view) context */
	save_gic_interface(gic_cpu_ctx_ptr->gic_cpu_if_regs,
		GIC_GICC_BASE_ADDR);
	/* TODO: Still have to consider cases e.g
	 * SGIs / Localtimers becoming pending.
	 */
	/* 3.2 Save distributoer interface private context */
	save_gic_distributor_private(gic_cpu_ctx_ptr->gic_dist_if_pvt_regs,
		GIC_GICD_BASE_ADDR);
	/* 3.3 Save distributoer interface global context */
	save_gic_distributor_shared(gbl_ctx_ptr->gic_dist_if_regs,
		GIC_GICD_BASE_ADDR);

	/* TODO: Is there any Bus Interface Unit Register to be Saved ? */

	/* TODO: Do I need to disable L2 invalidate when reset */

	/* 4. Save cpu general purpose banked registers */
	save_banked_registers(gp_ctx_ptr);

	instr_sync_barrier();
	data_sync_barrier();
}

/* TODO: If the MMU was active when we went to sleep / DRIPS
 * and all the addresses were virtual addresses, then while
 * waking up, we need to use Physical addresses, in which case
 * the saved virtual addresses would have to converted to physical
 * addresses. Or while saving itself we should have saved the
 * physical addresses.
 *
 * However, I guess we are going to be using physical addresses
 * only for now.
 */
void platform_restore_context(void)
{
	/* What LP State we are waking from */
	u32_t lpstate;
	soc_ctx_t *soc_ctx_ptr = &soc_ctx;

	u32_t *gp_ctx_ptr = soc_ctx_ptr->banked_cpu_regs;
	cp15_ctx_t *cp15_ctx_ptr = &soc_ctx_ptr->cp15_regs;
	gic_cpu_ctx_t *gic_cpu_ctx_ptr = &soc_ctx_ptr->gic_cpu_ctx;
	global_ctx_t *gbl_ctx_ptr = &soc_ctx_ptr->gbl_ctx;
	cp15_fault_regs_t *fault_ctx = &cp15_ctx_ptr->cp15_fault_regs;
	u32_t *vfp_ctx = soc_ctx_ptr->vfp_regs;
#ifdef SAVE_RESTORE_GENERIC_TIMER
	gen_tmr_ctx_t *gen_tmr_ctx_ptr = &soc_ctx_ptr->timer_ctx;
#endif

	instr_sync_barrier();
	data_sync_barrier();

	/* First Read the Persistent Register that indicates whether
	 * this wake-up is from "Deep Sleep" or from "DRIPS"
	 */
	lpstate = sys_read32(DRIPS_OR_DEEPSLEEP_REG);
	if((lpstate & PM_STATE_MASK_SET) == CPU_PW_STAT_DRIPS) {
		/* Indicate that We are Exiting LP Mode. */
		lpstate = (lpstate & ENTRY_EXIT_MASK) | EXITING_LP;
		sys_write32(lpstate, DRIPS_OR_DEEPSLEEP_REG);

		/* 3.3. Restore GIC distributor's Global context */
		restore_gic_distributor_shared(gbl_ctx_ptr->gic_dist_if_regs,
			GIC_GICD_BASE_ADDR);

		/* 3.2. Restore GIC Distributor private context  */
		restore_gic_distributor_private(gic_cpu_ctx_ptr->gic_dist_if_pvt_regs,
			GIC_GICD_BASE_ADDR);

		/* 3.1 Restore GIC CPU Interface context */
		restore_gic_interface(gic_cpu_ctx_ptr->gic_cpu_if_regs,
			GIC_GICC_BASE_ADDR);

		instr_sync_barrier();
		data_sync_barrier();

		/* Restore VFP Context */
		restore_vfp(vfp_ctx);

		/* 2. Restore cp15 context */
		/* 2.4 Restore CP15 Fault Status Registers */
		restore_fault_status((u32_t *)fault_ctx);
		/* 2.3 Restore CP15 MMU Status Registers */
		restore_mmu(cp15_ctx_ptr->cp15_mmu_regs);
		/* If MMU was used & restored, Now on we have virtual addresses */
		/* 2.2 Restore CP15 Misc Registers */
		restore_cp15(cp15_ctx_ptr->cp15_misc_regs);
		/* invalidate TLB if applicable */
		restore_control_registers(cp15_ctx_ptr->cp15_ctrl_regs, 0x0);

		instr_sync_barrier();
		data_sync_barrier();

#ifdef SAVE_RESTORE_GENERIC_TIMER
		/* 1. Restore the 32-bit Generic timer context */
		restore_generic_timer((u32_t *)gen_tmr_ctx_ptr, 0x0);
#endif

		/* TODO: Restore any Bus Interface Unit Register (if Saved) */

		/* TODO: Do I need to enable L2 invalidate when reset
		 * (if disabled in save context)
		 */
		/* 4. Restore cpu general purpose banked registers
		 * Last Because this restores the PC.
		 */
		restore_banked_registers(gp_ctx_ptr);
	}
	else if((lpstate & PM_STATE_MASK_SET) == CPU_PW_STAT_DEEPSLEEP) {
		/* Woke Up from Deep Sleep:
		 *
		 * In this case no Power was retained. SRAM Must be empty.
		 * Need to transfer control to the Reset Initialization Code.
		 */
	}

	instr_sync_barrier();
	data_sync_barrier();
}

/** This function is the entry function to which
 * Secure Boot Loader (or any other entity) will
 * transfer control upon waking from DRIPS mode.
 *
 * This prepares a temporary 1K stack for running
 * the restore routines.
 *
 * Note that UART is NOT powered OFF since the UART
 * is powered by PD_SYS and in DRIPS we only turn
 * PD_CPU OFF. So, it assumes that UART retains all
 * the programming that was done when the system entered
 * DRIPS mode and attempts to print: "RESET\n" on to
 * the UART0 Port 0 Console.
 */
void cpu_wakeup(void)
{
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");
	__asm volatile("nop\n");

	/* Prepare a temporary Stack
	 * FIXME: Check if the address loaded is
	 * word aligned. if not make it.
	 *
	 * TODO (Re-confirm):
	 * Adjust the Stack Base (Stack Pointer)
	 * by subtracting Page Offset (possibly for Page alignment)
	 * sub r0, r0, #PAGE_OFFSET -OR- do:
	 * ldr r1,=PAGE_OFFSET
	 * sub r0, r0, r1
	 *
	 * Also adjust by adding PHYS_OFFSET (if applicable)
	 * add r0, r0, #PHYS_OFFSET -OR- do:
	 * ldr r1,=PHYS_OFFSET
	 * add r0, r0, r1
	 *
	 * Initialize the SP with the Restore Stack
	 * Currently this is done at POR code, but
	 * this is temporary WAR, since ideally SBL
	 * is supposed to transfer control directly
	 * to this function.
	 *
	 * Note: This function's preamble pushes into
	 * stack, so watch - out.
	 */
	/* Interrupts are expected to be disabled at this point.
	 * When PoR path is taken this indeed is the case, but not
	 * so in fast xip work-around path.
	 */
	extern u8_t __RSTR_STACK_START__;
	__asm volatile("cpsid ifa\n");
	__asm volatile("cps #31\n");
	__asm volatile("mov sp, %[stack]\n" :: [stack] "r" (&__RSTR_STACK_START__));
	/* FIXME: Don't invoke printk here, it causes aborts on Zephyr
	 * though it works on OpenRTOS. May be it is owing to the difference
	 * in schedulers. Better to implement the raw writes into UART ports
	 * as was done in early versions of the Power Manager and keep it OS
	 * agnostic.
	 */
	SYS_LOG_DBG("___W_O_K_E___U_P___F_R_O_M___D_R_I_P_S___");

	platform_restore_context();

}

/** This function is to be invoked whenver we enter
 * a Low Power State that involves powering down the CPU
 * with an intention of resuming execution from where it
 * left off. e.g. DRIPS mode.
 */
s32_t cpu_power_down(void)
{
#ifdef CONFIG_SOC_UNPROGRAMMED
	/* In case we are trying DRIPS with an Unprogrammed image
	 * the address of the function cpu_wakeup need to be written
	 * to the jump address of SBL's image TOC header.
	 *
	 * In our case, the SBL's "Jump Address" has to be loaded with
	 * the cpu_wakeup routine.
	 */
	*((u32_t *)JUMP_ADDRESS) = (u32_t)cpu_wakeup;
#endif

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Clean and invalidate all data from the L1+L2 data cache */
	disable_dcache();
#endif

	/* Execute a CLREX instruction */
	__asm volatile("clrex");

	platform_save_context();

	/* Will never reach here */
	return 1;
}
