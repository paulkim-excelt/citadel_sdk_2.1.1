/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARM Cortex-A
 *
 * Common fault handler for ARM Cortex-A processors.
 */

#include <toolchain.h>

#include <kernel.h>
#include <cortex_a/exc.h>
#include <cortex_a/cp15.h>
#include <misc/util.h>

NANO_ESF _default_esf;

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

#if (CONFIG_FAULT_DUMP == 2)
/* IFSR status bits */
#define IFSR_EXT_ABORT_MASK			(0x1UL << 12)
#define IFSR_LPAE_BIT_MASK			(0x1UL << 9)
/* Short translation table format */
#define IFSR_FAULT_STATUS_BITS_SHORT_TT(IFSR)	(((IFSR >> 6) & 0x10) | \
						 (IFSR & 0xF))
/* Long translation table format */
#define IFSR_FAULT_STATUS_BITS_LONG_TT(IFSR)	(IFSR & 0x3F)

/* DFSR status bits */
#define DFSR_CACHE_MAINTENANCE_FAULT_MASK	(0x1UL << 13)
#define DFSR_EXTERNAL_ABORT_MASK		(0x1UL << 12)
#define DFSR_READ_FAULT_MASK			(0x1UL << 11)
#define DFSR_LPAE_BIT_MASK			(0x1UL << 9)
/* Short translation table format */
#define DFSR_FAULT_STATUS_BITS_SHORT_TT(DFSR)	(((DFSR >> 6) & 0x10) | \
						 (DFSR & 0xF))
/* Long translation table format */
#define DFSR_FAULT_STATUS_BITS_LONG_TT(DFSR)	(DFSR & 0x3F)

struct fault_status {
	u8_t fs_val;
	const char *reason;
};

/* Fault status values and their encodings */
static const struct fault_status instr_fs_short_desc[] = {
	{0x2, "Debug event"},
	{0x3, "Access flag fault, section."},
	{0x5, "Translation fault, section."},
	{0x6, "Access flag fault, page."},
	{0x7, "Translation fault, page."},
	{0x8, "Synchronous external abort, non-translation"},
	{0x9, "Domain fault, section."},
	{0xB, "Domain fault, page."},
	{0xC, "Synchronous external abort on translation table walk: Level 1"},
	{0xD, "Permission Fault, Section."},
	{0xC, "Synchronous external abort on translation table walk: Level 2"},
	{0xF, "Permission fault, page."}
};

static const struct fault_status instr_fs_long_desc[] = {
	{0x4, "Translation fault : Reserved"},
	{0x5, "Translation fault : Level 1"},
	{0x6, "Translation fault : Level 2"},
	{0x7, "Translation fault : Level 3"},
	{0x8, "Access fault : Level 0"},
	{0x9, "Access fault : Level 1"},
	{0xA, "Access fault : Level 2"},
	{0xB, "Access fault : Level 3"},
	{0xC, "Permission fault : Reserved"},
	{0xD, "Permission fault : Level 1"},
	{0xE, "Permission fault : Level 2"},
	{0xF, "Permission fault : Level 3"},
	{0x10, "Synchronous external abort"},
	{0x22, "Debug Event"}
};

static const struct fault_status data_fs_short_desc[] = {
	{0x1, "Alignment fault"},
	{0x2, "Debug event"},
	{0x3, "Access flag fault, section"},
	{0x4, "Instruction cache maintenance fault"},
	{0x5, "Translation fault, section"},
	{0x6, "Access flag fault, page"},
	{0x7, "Translation fault, page"},
	{0x8, "Synchronous external abort, non-translation"},
	{0x9, "Domain fault, section"},
	{0xB, "Domain fault, page"},
	{0xC, "Synchronous external abort on translation table walk: Level 1"},
	{0xD, "Permission fault, section"},
	{0xE, "Synchronous external abort on translation table walk: Level 2"},
	{0xF, "Permission fault, page"},
	{0x16, "Asynchronous external abort"}

};

static const struct fault_status data_fs_long_desc[] = {
	{0x4, "Translation fault : Reserved"},
	{0x5, "Translation fault : Level 1"},
	{0x6, "Translation fault : Level 2"},
	{0x7, "Translation fault : Level 3"},
	{0x8, "Access fault : Level 0"},
	{0x9, "Access fault : Level 1"},
	{0xA, "Access fault : Level 2"},
	{0xB, "Access fault : Level 3"},
	{0xC, "Permission fault : Reserved"},
	{0xD, "Permission fault : Level 1"},
	{0xE, "Permission fault : Level 2"},
	{0xF, "Permission fault : Level 3"},
	{0x10, "Synchronous External Abort"},
	{0x11, "Asynchronous External Abort"},
	{0x14, "Synchronous external abort on translation table walk: Reserve"},
	{0x15, "Synchronous external abort on translation table walk: Level 1"},
	{0x16, "Synchronous external abort on translation table walk: Level 2"},
	{0x17, "Synchronous external abort on translation table walk: Level 3"},
	{0x21, "Alignment Fault"},
	{0x22, "Debug Event"}
};

static const char *get_fault_reason(u8_t fs_val,
				    const struct fault_status *fs,
				    u32_t table_size)
{
	u32_t i;

	for (i = 0; i < table_size; i++)
		if (fs[i].fs_val == fs_val)
			return fs[i].reason;

	return "Unknown fault";
}
#endif /* CONFIG_FAULT_DUMP == 2 */

/**
 *
 * @brief Dump thread information
 *
 * @return N/A
 */
static void _FaultThreadShow(void)
{
	PR_EXC("Executing thread ID (thread): %p\n", k_current_get());
}

#if CONFIG_FAULT_DUMP
/**
 *
 * @brief Dump instruction abort information
 *
 * In the short format this function dumps the IFSR and IFAR registers. In the
 * long form, it dumps descriptive information about all the IFSR status bits
 * depending on the translation table format (long-desc / short-desc)
 *
 * @return N/A
 */
static void dump_instruction_abort_info(void)
{
	u32_t ifsr, ifar;

	/* Read IFSR and IFAR registers and dump them */
	ifsr = read_ifsr();
	ifar = read_ifar();
	PR_EXC("Inst. Fault generated by %s\n", (ifsr & IFSR_EXT_ABORT_MASK) ?
		"Slave" : "AXI Decode");
	PR_EXC("IFSR: 0x%08x\tIFAR: 0x%08x\n", ifsr, ifar);

#if (CONFIG_FAULT_DUMP == 2)
	/* Dump additional fault status info */
	if ((ifsr & IFSR_LPAE_BIT_MASK) == 0) {
		/* Short Description translation table format */
		const char *reason = get_fault_reason(
					IFSR_FAULT_STATUS_BITS_SHORT_TT(ifsr),
					&instr_fs_short_desc[0],
					ARRAY_SIZE(instr_fs_short_desc));
		PR_EXC("%s\n", reason);
	} else {
		/* Long Description translation table format */
		const char *reason = get_fault_reason(
					IFSR_FAULT_STATUS_BITS_LONG_TT(ifsr),
					&instr_fs_long_desc[0],
					ARRAY_SIZE(instr_fs_long_desc));
		PR_EXC("%s\n", reason);
	}
#endif
}

/**
 *
 * @brief Dump data abort information
 *
 * In the short format this function dumps the DFSR and DFAR registers. In the
 * long form, it dumps descriptive information about all the DFSR status bits
 * depending on the translation table format (long-desc / short-desc)
 *
 * @return N/A
 */
static void dump_data_abort_info(void)
{
	u32_t dfsr, dfar;

	/* Read DFSR and DFAR registers and dump them */
	dfsr = read_dfsr();
	dfar = read_dfar();
	PR_EXC("Data Fault generated by %s\n", (dfsr & DFSR_READ_FAULT_MASK) ?
		"Write" : "Read");
	PR_EXC("DFSR: 0x%08x\tDFAR: 0x%08x\n", dfsr, dfar);

#if (CONFIG_FAULT_DUMP == 2)
	/* Dump additional fault status info */
	if (dfsr & DFSR_CACHE_MAINTENANCE_FAULT_MASK)
		PR_EXC("Cache maintenace fault\n");
	if (dfsr & DFSR_EXTERNAL_ABORT_MASK)
		PR_EXC("External abort\n");
	if ((dfsr & DFSR_LPAE_BIT_MASK) == 0) {
		/* Short Description translation table format */
		const char *reason = get_fault_reason(
					DFSR_FAULT_STATUS_BITS_SHORT_TT(dfsr),
					&data_fs_short_desc[0],
					ARRAY_SIZE(data_fs_short_desc));
		PR_EXC("%s\n", reason);
	} else {
		/* Long Description translation table format */
		const char *reason = get_fault_reason(
					DFSR_FAULT_STATUS_BITS_LONG_TT(dfsr),
					&data_fs_long_desc[0],
					ARRAY_SIZE(data_fs_long_desc));
		PR_EXC("%s\n", reason);
	}
#endif
}

#endif /* CONFIG_FAULT_DUMP */

/**
 *
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy. Below are the
 * exceptions that will invoke this routine.
 *
 * - Undefined instruction Exception
 * - Prefetch Abort Exception
 * - Data Abort Exception
 *
 * @param esf Pointer to exception stack frame
 *
 */
void _Fault(u32_t exception, const NANO_ESF *esf)
{
	_FaultThreadShow();

	switch (exception) {
	case UNDEF_INSTRUCTION_EXCEPTION:
		PR_EXC("Undefined instruction!\n");
		break;
	case PREFETCH_ABORT_EXCEPTION:
		PR_EXC("Prefetch Abort!\n");
#if CONFIG_FAULT_DUMP
		dump_instruction_abort_info();
#endif
		break;
	case DATA_ABORT_EXCEPTION:
		PR_EXC("Data Abort!\n");
#if CONFIG_FAULT_DUMP
		dump_data_abort_info();
#endif
		break;
	default:
		break;
	}

	sys_exc_esf_dump(esf);
	while (1)
		;
}
