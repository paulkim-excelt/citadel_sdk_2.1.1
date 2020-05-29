/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC configuration macros for the BCM5820X
 *
 */

#ifndef _BCM5820X_SOC_H_
#define _BCM5820X_SOC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Bit 31 is used as the uncached address bit
 * This is by software convention. For 5820X, there is an
 * internal SRAM starting at 0x24000000 (1 MB) and
 *
 * With bit 31 set these SRAM addresses maps to the
 * corresponding uncached memory regions. So the uncached SRAM regions are
 * Internal Uncached SRAM: 0xA4000000 - 0xA40FFFFF
 *
 * These uncached memory maps are available only when MMU is enabled.
 */
#define UNCACHED_ADDR_BIT	31

/* Helper macros to convert between cached and uncached addresses */
#define UNCACHED_ADDR_MASK	BIT(UNCACHED_ADDR_BIT)
#define UNCACHED_ADDR(ADDR)	\
	((((u32_t)ADDR >= CONFIG_SRAM_BASE_ADDRESS) && \
	  ((u32_t)ADDR < (CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE*1024))) \
	  ? (void *)((u32_t)ADDR | UNCACHED_ADDR_MASK) : (void *)ADDR)
#define CACHED_ADDR(ADDR)	\
	((((u32_t)ADDR >= (CONFIG_SRAM_BASE_ADDRESS | UNCACHED_ADDR_MASK)) && \
	  (u32_t)(ADDR < ((CONFIG_SRAM_BASE_ADDRESS | UNCACHED_ADDR_MASK) \
		   + CONFIG_SRAM_SIZE*1024))) \
	  ? (void *)((u32_t)ADDR & ~UNCACHED_ADDR_MASK) : (void *)ADDR)

#ifdef __cplusplus
}
#endif

#endif /* _BCM5820X_SOC_H_ */
