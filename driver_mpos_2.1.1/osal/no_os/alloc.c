/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A Simple heap implementation builds without an RTOS
 */

#include <stdbool.h>
#include <kernel.h>
#include <init.h>
#include <arch/cpu.h>
#include <misc/util.h>

extern u32_t __heap_start__;
extern u32_t __heap_end__;

#define MEM_BLOCK_POWER		(6)
#define MEM_BLOCK_SIZE		(0x1 << MEM_BLOCK_POWER)	/* 64 Bytes */
#define ALLOC_INFO_SIZE(HEAP)	((HEAP->num_blocks + 0x7) >> 3)
#define HEAP_START		(u8_t *)(&__heap_start__)

#define SET_ALLOC_INFO_BIT(INDEX)		\
	do {					\
		u8_t *addr;			\
		addr = &hi->alloc_info[(INDEX)/8];\
		*addr |= BIT((INDEX) % 8);	\
	} while (0)

#define RESET_ALLOC_INFO_BIT(INDEX)		\
	do {					\
		u8_t *addr;			\
		addr = &hi->alloc_info[(INDEX)/8];\
		*addr &= ~BIT((INDEX) % 8);	\
	} while (0)

#define GET_ALLOC_INFO_BIT(INDEX)		\
	((hi->alloc_info[(INDEX)/8] >> ((INDEX) % 8)) & 0x1)

struct heap_info {
	u8_t *start;		/* Start of heap */
	u32_t num_blocks;	/* Total no. of blocks available in the heap */
	u32_t free_blocks;	/* Number of block used */
	u8_t  *alloc_info;	/* Pointer to the allocation information
				 * The size of this array is num_block/8, with
				 * each bit in indicating whether the associate
				 * block is in use or free. The last block in an
				 * allocation is always left free to mark the
				 * end of the allocation. This way the size need
				 * not be stored for an allocation. This means
				 * a trade off with the memory utilization to
				 * achieve a simple heap implementation.
				 */
} heap_info;

static struct heap_info *hi = &heap_info;

static int heap_init(struct device *dev)
{
	u32_t size, i;

	ARG_UNUSED(dev);

	size = (u32_t)&__heap_end__ - (u32_t)&__heap_start__;

	/* Initialize heap info structure */
	/* Align start to MEM_BLOCK_SIZE */
	hi->start = (u8_t *)(((u32_t)(&__heap_start__) + MEM_BLOCK_SIZE - 1) &
		    ~(MEM_BLOCK_SIZE - 1));

	/* Reduce size by the alignment adjust */
	size -= (u32_t)(hi->start - (u8_t *)(&__heap_start__));

	/* Compute number of block
	 * One byte of alloc_info array is needed for 8 memory blocks.
	 */
	hi->num_blocks = (size / (1 + MEM_BLOCK_SIZE*8)) * 8;

	/* Alloc info array will start at the end of the last memory block */
	hi->alloc_info = hi->start + hi->num_blocks*MEM_BLOCK_SIZE;

	/* All blocks are free to start with */
	hi->free_blocks = hi->num_blocks;

	for (i = 0; i < ALLOC_INFO_SIZE(hi); i++) {
		hi->alloc_info[i] = 0x0;
	}

	return 0;
}

void *k_malloc(size_t size)
{
	bool skip_next;
	u32_t i, j;
	u32_t block_count, block_start;
	void *ptr = NULL;
	size_t num_blocks;

	num_blocks = (size + MEM_BLOCK_SIZE - 1) >> MEM_BLOCK_POWER;

	/* One extra block to mark the end of the allocation */
	num_blocks += 1;

	/* Check if we have enough free blocks first */
	if (num_blocks > hi->free_blocks)
		return NULL;

	block_count = 0;
	block_start = 0;
	skip_next = false;
	for (i = 0; i < ALLOC_INFO_SIZE(hi); i++) {
		for (j = 0; j < 8; j++) {
			/* skip_next flag is set if we are interating over an
			 * allocated block.
			 */
			if (skip_next) {
				/* Reset skip_next flag if we hit the
				 * terminating block for the current allocation
				 */
				if ((BIT(j) & hi->alloc_info[i]) == 0)
					skip_next = false;
				continue;
			}

			if (BIT(j) & hi->alloc_info[i]) {
				/* Block allocated - Skip the next block as
				 * it could be the terminating block for the
				 * current allocation
				 */
				skip_next = true;
				block_count = 0;
			} else {
				if (block_count == 0) {
					block_start = i*8 + j;
				}
				block_count++;
				if (block_count == num_blocks) {
					/* Found allocation */
					ptr = hi->start +
					      MEM_BLOCK_SIZE*block_start;
				}
			}
		}
		if (ptr)
			break;
	}

	/* Mark allocation info bits for allocated memory */
	if (ptr) {
		for (i = 0; i < num_blocks - 1; i++) {
			SET_ALLOC_INFO_BIT(block_start + i);
		}
		hi->free_blocks -= num_blocks;
	}

	return ptr;
}

void k_free(void *ptr)
{
	u32_t block_index;

	/* Invalid pointer (not alinged to MEM_BLOCK_SIZE) */
	if ((u32_t)ptr & (MEM_BLOCK_SIZE - 1))
		return;

	block_index = ((u8_t *)ptr - hi->start) / MEM_BLOCK_SIZE;

	/* The block index can at most point to block n-2,
	 * in which case (n-1) would be the terminating block.
	 * n here is the number of block in the heap.
	 */
	if (block_index >= (hi->num_blocks - 1))
		return;

	while (GET_ALLOC_INFO_BIT(block_index)) {
		RESET_ALLOC_INFO_BIT(block_index);
		hi->free_blocks++;
		block_index++;
	}

	/* For the terminating block */
	hi->free_blocks++;
}


SYS_INIT(heap_init, PRE_KERNEL_1, 0);
