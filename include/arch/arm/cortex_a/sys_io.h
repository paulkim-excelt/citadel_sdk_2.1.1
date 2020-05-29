/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ARM CORTEX-A Series memory mapped register I/O operations
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_SYS_IO_H_

#if !defined(_ASMLANGUAGE)

#include <sys_io.h>

/* Memory mapped registers I/O functions */

static inline u8_t sys_read8(mem_addr_t addr)
{
	return *(volatile u8_t *)addr;
}

static inline void sys_write8(u8_t data, mem_addr_t addr)
{
	*(volatile u8_t *)addr = data;
}

static inline u16_t sys_read16(mem_addr_t addr)
{
	return *(volatile u16_t *)addr;
}

static inline void sys_write16(u16_t data, mem_addr_t addr)
{
	*(volatile u16_t *)addr = data;
}

static inline u32_t sys_read32(mem_addr_t addr)
{
	return *(volatile u32_t *)addr;
}


static inline void sys_write32(u32_t data, mem_addr_t addr)
{
	*(volatile u32_t *)addr = data;
}


/* Memory bit manipulation functions */

static inline void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp & ~(1 << bit);
}

static inline int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	return temp & (1 << bit);
}

static inline void sys_set_bitmask(mem_addr_t addr, unsigned int mask,
				  unsigned int shift, unsigned int data)
{
	u32_t temp = *(volatile u32_t *)addr;

	temp &= ~(mask << shift);
	temp |=  (mask & data) << shift;
	*(volatile u32_t *)addr = temp;
}

static inline int sys_get_bitmask(mem_addr_t addr, unsigned int mask,
				  unsigned int shift)
{
	u32_t temp = *(volatile u32_t *)addr;

	temp &= mask;
	return (temp >> shift);
}
#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_SYS_IO_H_ */
