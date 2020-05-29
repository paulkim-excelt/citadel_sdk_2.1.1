/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CP15 register read/write
 *
 * Macros to read ARM Cortex-A cp15 registers
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_CP15__H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_CP15__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#define READ_CP15(CRn, OP1, CRm, OP2)	({		\
	u32_t regval;					\
	__asm__ volatile ("mrc 15, " #OP1 ", %0, " #CRn ", " #CRm ", " #OP2 \
			  : "=r" (regval) : : "memory");\
	regval;						\
})

#define WRITE_CP15(REGVAL, CRn, OP1, CRm, OP2)	({	\
	__asm__ volatile ("mcr 15, " #OP1 ", %0, " #CRn ", " #CRm ", " #OP2 \
			  : : "r" (REGVAL) : "memory");	\
})

#define READ_CP15_64(REG1, REG2, OP1, CRm)	({		\
	__asm__ volatile ("mrrc 15, " #OP1 ", %0, %1, " #CRm \
			  : : "r" (REG1), "r" (REG2) : "memory");\
})

#define WRITE_CP15_64(REG1, REG2, OP1, CRm)	({	\
	__asm__ volatile ("mcrr 15, " #OP1 ", %0, %1, " #CRm \
			  : : "r" (REG1), "r" (REG2) : "memory");	\
})

#define READ_CP14(CRn, OP1, CRm, OP2)	({		\
	u32_t regval;					\
	__asm__ volatile ("mrc 14, " #OP1 ", %0, " #CRn ", " #CRm ", " #OP2 \
			  : "=r" (regval) : : "memory");\
	regval;						\
})

#define WRITE_CP14(REGVAL, CRn, OP1, CRm, OP2)	({	\
	__asm__ volatile ("mcr 14, " #OP1 ", %0, " #CRn ", " #CRm ", " #OP2 \
			  : : "r" (REGVAL) : "memory");	\
})

#ifdef CONFIG_ARMV7_A
/* Fault status registers */
#define read_dfsr()	READ_CP15(C5, 0, C0, 0)
#define write_dfsr(x) WRITE_CP15(x, C5, 0, C0, 0)

#define read_ifsr()	READ_CP15(C5, 0, C0, 1)
#define write_ifsr(x) WRITE_CP15(x, C5, 0, C0, 1)

#define read_dfar()	READ_CP15(C6, 0, C0, 0)
#define write_dfar(x) WRITE_CP15(x, C6, 0, C0, 0)

#define read_ifar()	READ_CP15(C6, 0, C0, 2)
#define write_ifar(x) WRITE_CP15(x, C6, 0, C0, 2)

#define read_adfsr() READ_CP15(C5, 0, C1, 0)
#define write_adfsr(x) WRITE_CP15(x, C5, 0, C1, 0)

#define read_aifsr() READ_CP15(C5, 0, C1, 1)
#define write_aifsr(x) WRITE_CP15(x, C5, 0, C1, 1)

/******************************
 **** MMU Status Registers ****
 ******************************/
/* VBAR */
#define read_vbar()	READ_CP15(C12, 0, C0, 0)
#define write_vbar(x) WRITE_CP15(x, C12, 0, C0, 0)

/* Read TTBCR */
#define read_ttbcr()	READ_CP15(C2, 0, C0, 2)
/* Write x to TTBCR */
#define write_ttbcr(x) WRITE_CP15(x, C2, 0, C0, 2)

/* Read & write 32-bit TTBR0 & TTBR1 */
#define read_ttbr0_32() READ_CP15(C2, 0, C0, 0)
#define read_ttbr1_32() READ_CP15(C2, 0, C0, 1)
#define write_ttbr0_32(x) WRITE_CP15(x, C2, 0, C0, 0)
#define write_ttbr1_32(x) WRITE_CP15(x, C2, 0, C0, 1)

/* Read & write 64-bit TTBR0 & TTBR1
 * The parameters x1 & x2 are pointers to u32_t
 */
#define read_ttbr0_64(x1, x2) READ_CP15_64(x1, x2, 0, C2)
#define read_ttbr1_64(x1, x2) READ_CP15_64(x1, x2, 1, C2)
#define write_ttbr0_64(x1, x2) WRITE_CP15_64(x1, x2, 0, C2)
#define write_ttbr1_64(x1, x2) WRITE_CP15_64(x1, x2, 1, C2)

/* Read & write DACR */
#define read_dacr() READ_CP15(C3, 0, C0, 0)
#define write_dacr(x) WRITE_CP15(x, C3, 0, C0, 0)
/* Read & write PAR */
#define read_par() READ_CP15(C7, 0, C4, 0)
#define write_par(x) WRITE_CP15(x, C7, 0, C4, 0)
/* Read & write  PRRR */
#define read_prrr() READ_CP15(C10, 0, C2, 0)
#define write_prrr(x) WRITE_CP15(x, C10, 0, C2, 0)
/* Read & write NMRR */
#define read_nmrr() READ_CP15(C10, 0, C2, 1)
#define write_nmrr(x) WRITE_CP15(x, C10, 0, C2, 1)

/* Read & write CONTEXTIDR */
#define read_contextidr() READ_CP15(C13, 0, C0, 1)
#define write_contextidr(x) WRITE_CP15(x, C13, 0, C0, 1)

/* Read & write TPIDRURW */
#define read_tpidrurw() READ_CP15(C13, 0, C0, 2)
#define write_tpidrurw(x) WRITE_CP15(x, C13, 0, C0, 2)

/* Read & write TPIDRURO */
#define read_tpidruro() READ_CP15(C13, 0, C0, 3)
#define write_tpidruro(x) WRITE_CP15(x, C13, 0, C0, 3)

/* Read & write TPIDRPRW */
#define read_tpidrprw() READ_CP15(C13, 0, C0, 4)
#define write_tpidrprw(x) WRITE_CP15(x, C13, 0, C0, 4)

/* Read & write ID_PFR0 */
#define read_id_pfr0() READ_CP15(C0, 0, C1, 0)
#define write_id_pfr0(x) WRITE_CP15(x, C0, 0, C1, 0)

/* Read & write TEECR */
#define read_teecr() READ_CP14(C0, 6, C0, 0)
#define write_teecr(x) WRITE_CP14(x, C0, 6, C0, 0)

/* Read & write TEEHBR */
#define read_teehbr() READ_CP14(C1, 6, C0, 0)
#define write_teehbr(x) WRITE_CP14(x, C1, 6, C0, 0)

/* Read & write JOSCR */
#define read_joscr() READ_CP14(C1, 7, C0, 0)
#define write_joscr(x) WRITE_CP14(x, C1, 7, C0, 0)

/* Read & write JMCR */
#define read_jmcr() READ_CP14(C2, 7, C0, 0)
#define write_jmcr(x) WRITE_CP14(x, C2, 7, C0, 0)

/********************************************
 **** Generic Timer Extensions Registers ****
 ********************************************/
#define read_cntfrq()	READ_CP15(C14, 0, C0, 0)
#define write_cntfrq(x) WRITE_CP15(x, C14, 0, C0, 0)

#define read_cntkctl()	READ_CP15(C14, 0, C1, 0)
#define write_cntkctl(x) WRITE_CP15(x, C14, 0, C1, 0)

#define read_cnthctl()	READ_CP15(C14, 4, C1, 0)
#define write_cnthctl(x) WRITE_CP15(x, C14, 4, C1, 0)

#define read_cntp_tval()	READ_CP15(C14, 0, C2, 0)
#define write_cntp_tval(x) WRITE_CP15(x, C14, 0, C2, 0)

#define read_cntp_ctl()	READ_CP15(C14, 0, C2, 1)
#define write_cntp_ctl(x) WRITE_CP15(x, C14, 0, C2, 1)

#define read_cntv_tval()	READ_CP15(C14, 0, C3, 0)
#define write_cntv_tval(x) WRITE_CP15(x, C14, 0, C3, 0)

#define read_cntv_ctl()	READ_CP15(C14, 0, C3, 1)
#define write_cntv_ctl(x) WRITE_CP15(x, C14, 0, C3, 1)

/* System control register */
#define read_sctlr()	READ_CP15(C1, 0, C0, 0)
#define write_sctlr(x)	WRITE_CP15(x, C1, 0, C0, 0)

/* Auxiliary control register */
#define read_actlr()	READ_CP15(C1, 0, C0, 1)
#define write_actlr(x)	WRITE_CP15(x, C1, 0, C0, 1)

/* Cache enable/disable bits in system control register */
#define DATA_CACHE_ENABLE_BIT	BIT(2)
#define AUX_CTRL_SMP_ENABLE_BIT	BIT(6)
#define INSTR_CACHE_ENABLE_BIT	BIT(12)

/* Secure configuration register */
#define read_scr()	READ_CP15(C1, 0, C1, 0)
#define write_scr(x)	WRITE_CP15(x, C1, 0, C1, 0)

/* Secure Debug Enable register */
#define read_sder()	READ_CP15(C1, 0, C1, 1)
#define write_sder(x)	WRITE_CP15(x, C1, 0, C1, 1)

/* Non-Secure Access Control register */
#define read_nsacr()	READ_CP15(C1, 0, C1, 2)
#define write_nsacr(x)	WRITE_CP15(x, C1, 0, C1, 2)

/* Monitor Vector base address register */
#define read_mvbar()	READ_CP15(C12, 0, C0, 1)
#define write_mvbar(x)	WRITE_CP15(x, C12, 0, C0, 1)

/* Vector base address register */
#define read_vbar()	READ_CP15(C12, 0, C0, 0)
#define write_vbar(x)	WRITE_CP15(x, C12, 0, C0, 0)

/* Translation table registers */
#define read_ttbr0()	READ_CP15(C2, 0, C0, 0)
#define write_ttbr0(x)	WRITE_CP15(x, C2, 0, C0, 0)

#define read_ttbr1()	READ_CP15(C2, 0, C0, 1)
#define write_ttbr1(x)	WRITE_CP15(x, C2, 0, C0, 1)

#define read_ttbcr()	READ_CP15(C2, 0, C0, 2)
#define write_ttbcr(x)	WRITE_CP15(x, C2, 0, C0, 2)

#define read_dacr()	READ_CP15(C3, 0, C0, 0)
#define write_dacr(x)	WRITE_CP15(x, C3, 0, C0, 0)

/* TLB invalidation registers */
#define write_itlbiall(x)	WRITE_CP15(x, c8, 0, c5, 0)
#define write_dtlbiall(x)	WRITE_CP15(x, c8, 0, c6, 0)
#define write_tlbiall(x)	WRITE_CP15(x, c8, 0, c7, 0)

/* Cache maintenance registers */
#define read_ccsidr()	READ_CP15(c0, 1, c0, 0)
#define read_clidr()	READ_CP15(c0, 1, c0, 1)

#define write_iciallu()	WRITE_CP15(0, c7, 0, c5, 0)
#define write_bpiall()	WRITE_CP15(0, c7, 0, c5, 6)
#define write_dcisw(x)	WRITE_CP15(x, c7, 0, c6, 2)
#define write_dccsw(x)	WRITE_CP15(x, c7, 0, c10, 2)
#define write_dccisw(x)	WRITE_CP15(x, c7, 0, c14, 2)
#define write_dcimvac(x)	WRITE_CP15(x, c7, 0, c6, 1)
#define write_dccmvac(x)	WRITE_CP15(x, c7, 0, c10, 1)
#define write_dccimvac(x)	WRITE_CP15(x, c7, 0, c14, 1)

#define read_csselr()	READ_CP15(c0, 2, c0, 0)
#define write_csselr(x)	WRITE_CP15(x, c0, 2, c0, 0)

/* Co-processor access control registers */
#define read_cpacr()	READ_CP15(c1, 0, c0, 2)
#define write_cpacr(x)	WRITE_CP15(x, c1, 0, c0, 2)

#define ASEDIS_DISABLE_BIT	BIT(31)
#define D32DIS_DISABLE_BIT	BIT(30)
/* NOTE: when setting this value, USR requires PRIV to be set */
#define CP11_PRIV_ACCESS	BIT(23)
#define CP11_USR_ACCESS		BIT(22)
/* NOTE: when setting this value, USR requires PRIV to be set */
#define CP10_PRIV_ACCESS	BIT(21)
#define CP10_USR_ACCESS		BIT(20)

/* System control register bits */
#define SCTLR_MMU_ENABLE_BIT		BIT(0)
#define SCTLR_TEX_REMAP_BIT		BIT(28)
#define SCTLR_ACCESS_FLAG_EN_BIT	BIT(29)

#else
#error Unsupported ARM architecture
#endif /* CONFIG_ARMV7_A */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_ARM_CORTEXA_CP15__H_ */
