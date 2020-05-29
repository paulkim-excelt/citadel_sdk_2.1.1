/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cache maintenance
 *
 * Cache maintenance operation routines to enable/disable and clean/invalidate
 * data and instruction caches
 */

#include <zephyr.h>
#include <cortex_a/cp15.h>
#include <cortex_a/cache.h>
#include <misc/util.h>
#include <cortex_a/barrier.h>

#ifdef CONFIG_CPU_CORTEX_A7

#define DCACHE_CLEAN_ONLY		0
#define DCACHE_INVALIDATE_ONLY		1
#define DCACHE_CLEAN_N_INVALIDATE	2

/**
 * @brief Invalidate instruction cache
 *
 * This routine invalidates instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void invalidate_icache(void)
{
	/* Invalidate all instruction caches to PoU. */
	write_iciallu();
	/* Invalidate entire branch predictor array */
	write_bpiall();

	/* Full system DSB - make sure that the invalidation is complete */
	data_sync_barrier();
	/* Full system ISB - make sure the instruction stream sees it */
	instr_sync_barrier();
}

#ifdef CONFIG_DATA_CACHE_SUPPORT
/**
 * @brief Clean and/or invalidate data cache
 *
 * This routine clean and invalidates data cache by iterating over all the
 * set indices for the cache
 *
 * @param N/A
 *
 * @return N/A
 */
static void clean_or_invalidate_dcache(u8_t op)
{
	u32_t reg, max_set, max_way, way_pos;
	u32_t i, j, ccsidr, linelen, l, cache_num = 0;

	/* Ensure a sequence point here, i.e. all memory accesses
	 * prior to this must be complete.
	 */
	data_mem_barrier();
	data_sync_barrier();
	instr_sync_barrier();

	/* Clean & Invalidate Data cache level 0, i.e. L1 Data cache.
	 * Read in clidr register - to know if a data cache is implemented.
	 * Extract the Cache Type Field from clidr and check if there is a
	 * Data cache.
	 * Mask of the bits for current cache only, i.e. Bits 0-2.
	 */
	reg = read_clidr();
	reg &= 0x7;

	/* Return if no cache(0), or only i-cache(1) present */
	if (reg < 0x2)
		goto done;

	/* Data Cache is Present. Select current cache level in
	 * Cache Size Selection Register (CSSELR).
	 * 0 for Level 1 Data Cache.
	 */
	write_csselr(cache_num);

	/* isb to synch the new CSSELR & CCSIDR */
	instr_sync_barrier();

	/* Read the new CCSIDR as pointed by CSSELR */
	ccsidr = read_ccsidr();
	/* extract the length of the cache lines (this is in words)
	 * since this is encoded as Log(line length - 2)
	 */
	linelen = ccsidr & 0x7;
	linelen += 2;
	linelen = 0x1 << linelen;
	linelen <<= 2; /* Words to bytes */

	/* L: log2(line length in bytes)
	 * 1. If L <= 4 then there is no SBZ field between the set and
	 * level fields in the DCCISW register.
	 * 2. So, If the above value L is less than 4 then make it 4.
	 */
	l = 32 - __builtin_clz(linelen - 1); /* log2(linelen) */
	if (l < 4)
		l = 4;

	/* Extract the Associativity, i.e. Number of ways per Set
	 * = CCISDR[12:3] (10 bits)
	 */
	max_way = (ccsidr >> 3) & 0x3ff;

	/* In DCCISW, the bit position for the Associativity Field.
	 * way_pos = 32 - A, where A = Log2(ASSOCIATIVITY), rounded up to the
	 * next integer if necessary
	 */
	way_pos = __builtin_clz(max_way);

	/* Extract Max Value of the Set Index, i.e. (No. of Sets - 1)
	 * = CCSIDR[27:13] (15 bits)
	 */
	max_set = (ccsidr >> 13) & 0x7fff;

	for (i = 0; i <= max_way; i++) {
		for (j = 0; j <= max_set; j++) {
			/* factor cache number, way and set index */
			reg = cache_num | (i << way_pos) | (j << l);
			/* clean and/or invalidate by set/way
			 * To do only Clean:			write_dccsw()
			 * To do only Invalidate:		write_dcisw()
			 * To do only Clean & Invalidate:	write_dccisw()
			 */
			switch (op) {
			case DCACHE_CLEAN_ONLY:
				write_dccsw(reg);
				break;
			case DCACHE_INVALIDATE_ONLY:
				write_dcisw(reg);
				break;
			case DCACHE_CLEAN_N_INVALIDATE:
			default:
				write_dccisw(reg);
				break;
			}
		}
	}

done:
	/* switch back to cache level 0.
	 * select current cache level in CSSELR
	 */
	write_csselr(0);

	data_mem_barrier();
	data_sync_barrier();
	instr_sync_barrier();
}

/**
 * @brief Invalidate data cache
 *
 * This routine invalidates data cache by iterating over all the
 * set indices for the cache
 *
 * @param N/A
 *
 * @return N/A
 */
void invalidate_dcache(void)
{
	clean_or_invalidate_dcache(DCACHE_INVALIDATE_ONLY);
}

/**
 * @brief Clean data cache
 *
 * This routine cleans data cache by iterating over all the
 * set indices for the cache
 *
 * @param N/A
 *
 * @return N/A
 */
void clean_dcache(void)
{
	clean_or_invalidate_dcache(DCACHE_CLEAN_ONLY);
}

/**
 * @brief Clean and invalidate data cache
 *
 * This routine clean and invalidates data cache by iterating over all the
 * set indices for the cache
 *
 * @param N/A
 *
 * @return N/A
 */
void clean_n_invalidate_dcache(void)
{
	clean_or_invalidate_dcache(DCACHE_CLEAN_N_INVALIDATE);
}
#endif /* CONFIG_DATA_CACHE_SUPPORT */

/**
 * @brief Enable instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void enable_icache(void)
{
	u32_t reg;

	/* Return if cache is already enabled */
	if (read_sctlr() & INSTR_CACHE_ENABLE_BIT)
		return;

	/* Invalidate cache before enabling it */
	invalidate_icache();

	/* Ensure a sequence point here, i.e. all memory accesses
	 * prior to this must be complete.
	 */
	data_mem_barrier();

	/* Read in the System Control Register */
	reg = read_sctlr();
	/* SCTRL.I = 1 */
	reg |= INSTR_CACHE_ENABLE_BIT;
	write_sctlr(reg);

	/* Another Sequence Point before starting/resuming cached-execution */
	data_sync_barrier();
	instr_sync_barrier();
}

#ifdef CONFIG_DATA_CACHE_SUPPORT
/**
 * @brief Enable data cache
 *
 * @param N/A
 *
 * @return N/A
 */
void enable_dcache(void)
{
	u32_t reg, key;

	/* Return if cache is already enabled */
	if ((read_sctlr() & DATA_CACHE_ENABLE_BIT)
	    && (read_actlr() & AUX_CTRL_SMP_ENABLE_BIT))
		return;

	key = irq_lock();

	/* Invalidate cache before enabling it */
	invalidate_dcache();

	/* Ensure a sequence point here, i.e. all memory accesses
	 * prior to this must be complete.
	 */
	data_mem_barrier();

	reg = read_actlr();
	/* Set ACTLR.SMP bit to enable data cache */
	reg |= AUX_CTRL_SMP_ENABLE_BIT;
	write_actlr(reg);

	/* Read in the System Control Register */
	reg = read_sctlr();
	/* SCTRL.C = 1 */
	reg |= DATA_CACHE_ENABLE_BIT;
	write_sctlr(reg);

	/* Another Sequence Point before starting/resuming cached-execution */
	data_sync_barrier();
	instr_sync_barrier();

	irq_unlock(key);
}
#endif /* CONFIG_DATA_CACHE_SUPPORT */

/**
 * @brief Disable instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void disable_icache(void)
{
	u32_t reg;

	/* Ensure a sequence point here, i.e. all memory accesses
	 * prior to this must be complete.
	 */
	data_mem_barrier();

	/* Read in the System Control Register */
	reg = read_sctlr();
	/* SCTRL.I = 0 */
	reg &= ~INSTR_CACHE_ENABLE_BIT;
	write_sctlr(reg);

	/* Another Sequence Point before starting/resuming cached-execution */
	data_sync_barrier();
	instr_sync_barrier();
}

#ifdef CONFIG_DATA_CACHE_SUPPORT
/**
 * @brief Disable data cache
 *
 *  Disable Data Cache (After Cleaning & Invalidating all entries).
 *  Cleaning of Data Cache is essential for a write-back cache before
 *  the disable operation. This is because the caller's call stack that
 *  was cache-resident while d-cache was enabled, must be written back
 *  to memory for the return address and other register values to be
 *  restored correctly.
 *
 * @param N/A
 *
 * @return N/A
 */
void disable_dcache(void)
{
	u32_t reg, key;

	key = irq_lock();

	/* Ensure a sequence point here, i.e. all memory accesses
	 * prior to this must be complete.
	 */
	data_mem_barrier();

	reg = read_actlr();
	/* Set ACTLR.SMP bit to enable data cache */
	reg &= ~AUX_CTRL_SMP_ENABLE_BIT;
	write_actlr(reg);

	/* Read in the System Control Register */
	reg = read_sctlr();
	/* SCTRL.C = 0 */
	reg &= ~DATA_CACHE_ENABLE_BIT;
	write_sctlr(reg);

	/* Another Sequence Point before starting/resuming cached-execution */
	data_sync_barrier();
	instr_sync_barrier();

	/* CPU will start fetching Data(including those on stack) from memory
	 * right after Cache is disabled. So it is imperative that the
	 * Data/Unified cache is flushed/cleaned so that memory is synced.
	 */
	clean_n_invalidate_dcache();

	irq_unlock(key);
}

/**
 * @brief Get L1 data cache line size
 *
 * @param N/A
 *
 * @return L1 data cache line size
 */
u32_t get_l1_dcache_line_size(void)
{
	u32_t ccsidr;
	static u32_t cache_line_size;

	/* Compute it only the first time and save it */
	if (cache_line_size == 0) {
		write_csselr(0);

		/* Read the CCSIDR */
		ccsidr = read_ccsidr();

		/* extract the length of the cache lines
		 * ccsid[2:0] + 2 gives L, where L = log (cache line size) words
		 */
		cache_line_size =  (0x1 << ((ccsidr & 0x7) + 2)) << 2;
	}

	return cache_line_size;
}

/**
 * @brief Invalidate data cache by address range
 *
 * @param addr - Address for which the cache lines should be invalidated
 * @param len - Size of memory for which the cache lines should be invalidated
 *
 * @return N/A
 */
void invalidate_dcache_by_addr(unsigned long addr, unsigned long len)
{
	unsigned long mva, cache_line_size;

	if (len == 0)
		return;

	cache_line_size = get_l1_dcache_line_size();

	mva = addr & ~(cache_line_size - 1);
	while (mva < (addr + len)) {
		write_dcimvac(mva);
		mva += cache_line_size;
	}
}

/**
 * @brief Clean data cache by address range
 *
 * @param addr - Address for which the cache lines should be flushed
 * @param len - Size of memory for which the cache lines should be flushed
 *
 * @return N/A
 */
void clean_dcache_by_addr(unsigned long addr, unsigned long len)
{
	unsigned long mva, cache_line_size;

	if (len == 0)
		return;

	cache_line_size = get_l1_dcache_line_size();

	mva = addr & ~(cache_line_size - 1);
	while (mva < (addr + len)) {
		write_dccmvac(mva);
		mva += cache_line_size;
	}
}

/**
 * @brief Clean and Invalidate data cache by address range
 *
 * @param addr - Address for which the cache lines should be flushed/invalidated
 * @param len - Size of memory for which the cache lines should be flushed and
 *		invalidated
 *
 * @return N/A
 */
void clean_n_invalidate_dcache_by_addr(unsigned long addr, unsigned long len)
{
	unsigned long mva, cache_line_size;

	if (len == 0)
		return;

	cache_line_size = get_l1_dcache_line_size();

	mva = addr & ~(cache_line_size - 1);
	while (mva < (addr + len)) {
		write_dccimvac(mva);
		mva += cache_line_size;
	}
}
#endif /* CONFIG_DATA_CACHE_SUPPORT */

#ifdef CONFIG_CACHE_DEBUG_SUPPORT
#include <misc/printk.h>
#define NUM_SETS	0x80
#define NUM_WAYS	4
#define SET(A)	((A >> 6) & (NUM_SETS-1))
/* @brief   Dump cache line data
 * @details For a given address, this function, dumps the
 *          cache line data and attributes if the tag matches
 *
 * @param addr - Address for which the cache lines need to be logged
 *
 * @return N/A
 */
void print_cache_line_info(u32_t addr)
{
	u32_t d0, d1, i, j, address;

	printk("Cache line info for addr: %x\n", addr);
	printk("=================================\n");
	for (i = 0; i < NUM_WAYS; i++) {
		WRITE_CP15((i << 30) | (SET(addr) << 6), c15, 3, c2, 0);

		d0 = READ_CP15(c15, 3, c0, 0);
		d1 = READ_CP15(c15, 3, c0, 1);

		/* Check for a tag match */
		if (((d1 << 11) ^ addr) < BIT(13)) {
			printk("[%d] tag = %x : mosi = %x\n", i, d1 << 11,
					((d0 << 2) | (d1 >> 30)) & 0xF);
			address = addr & ~(get_l1_dcache_line_size() - 1);
			for (j = 0; j < 8; j++) {
				WRITE_CP15(
					(i << 30) | (SET(addr) << 6) | (j << 3),
					c15, 3, c4, 0);

				d0 = READ_CP15(c15, 3, c0, 0);
				d1 = READ_CP15(c15, 3, c0, 1);
				printk("[0x%08x] : 0x%08X 0x%08X\n",
					address, d0, d1);
				address += 8;
			}
		}
	}
	printk("=================================\n");
}
#endif /* CONFIG_CACHE_DEBUG_SUPPORT */
#else
#error Unsupported ARM architecture
#endif /* CONGIG_CPU_CORTEX_A7*/
