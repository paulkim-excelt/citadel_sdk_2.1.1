/* ARM Cortex-A cache maintenance */

/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARM_CORTEXA_CACHE__H
#define _ARM_CORTEXA_CACHE__H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Invalidate instruction cache
 *
 * This routine invalidates instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void invalidate_icache(void);

#ifdef CONFIG_DATA_CACHE_SUPPORT
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
void invalidate_dcache(void);

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
void clean_dcache(void);

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
void clean_n_invalidate_dcache(void);
#endif /* CONFIG_DATA_CACHE_SUPPORT */

/**
 * @brief Enable instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void enable_icache(void);

#ifdef CONFIG_DATA_CACHE_SUPPORT
/**
 * @brief Enable data cache
 *
 * @param N/A
 *
 * @return N/A
 */
void enable_dcache(void);
#endif /* CONFIG_DATA_CACHE_SUPPORT */

/**
 * @brief Disable instruction cache
 *
 * @param N/A
 *
 * @return N/A
 */
void disable_icache(void);

#ifdef CONFIG_DATA_CACHE_SUPPORT
/**
 * @brief Disable data cache
 *
 * @param N/A
 *
 * @return N/A
 */
void disable_dcache(void);

/**
 * @brief Invalidate data cache by address range
 *
 * @param addr - Address for which the cache lines should be invalidated
 * @param len - Size of memory for which the cache lines should be invalidated
 *
 * @return N/A
 */
void invalidate_dcache_by_addr(unsigned long addr, unsigned long len);

/**
 * @brief Clean data cache by address range
 *
 * @param addr - Address for which the cache lines should be flushed
 * @param len - Size of memory for which the cache lines should be flushed
 *
 * @return N/A
 */
void clean_dcache_by_addr(unsigned long addr, unsigned long len);

/**
 * @brief Clean and Invalidate data cache by address range
 *
 * @param addr - Address for which the cache lines should be flushed/invalidated
 * @param len - Size of memory for which the cache lines should be flushed and
 *		invalidated
 *
 * @return N/A
 */
void clean_n_invalidate_dcache_by_addr(unsigned long addr, unsigned long len);

/**
 * @brief Get L1 data cache line size
 *
 * @param N/A
 *
 * @return L1 data cache line size
 */
u32_t get_l1_dcache_line_size(void);

#define DCACHE_LINE_SIZE	get_l1_dcache_line_size()

#endif /* CONFIG_DATA_CACHE_SUPPORT */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXA_CACHE__H */
