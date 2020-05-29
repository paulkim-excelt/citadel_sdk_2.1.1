/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MMU initialization
 *
 * Memory management unit definitions for memory attributes, memory types,
 * access permissions etc ...
 */

#ifndef _ARM_CORTEXA_MMU__H_
#define _ARM_CORTEXA_MMU__H_

#ifdef __cplusplus
extern "C" {
#endif

/* Memory types */
#define MEM_TYPE_NORMAL			0x1
#define MEM_TYPE_DEVICE			0x2
#define MEM_TYPE_STRONGLY_ORDERED	0x3

/* Memory attributes
 * Note that not all attributes are checked for each of the memory types
 * - For strongly ordered memory type, the attributes are don't care
 * - For device memory type, only the shareable memory type is relevant
 */
#define MEM_ATTR_SHAREABLE			0x1
#define MEM_ATTR_NON_CACHEABLE			0x0
#define MEM_ATTR_CACHEABLE			0x2
#define MEM_ATTR_CACHEABLE_WRITE_THROUGH	0x0
#define MEM_ATTR_CACHEABLE_WRITE_BACK		0x4
#define MEM_ATTR_CACHEABLE_NO_WRITE_ALLOCATE	0x0
#define MEM_ATTR_CACHEABLE_WRITE_ALLOCATE	0x8

/* Uncached memory attribute is software defined
 * This attribute allows to create an uncached alias for a cached memory region
 * When this attribute is set, the MSB 8 bits of the attr member in
 * struct mmu_mem_region_info is used to determine which bit gets flipped for
 * uncached to cached mapping. For example, for the below entry
 *     addr = 0x24000000;
 *     size = 0x100000;
 *     attr = (27 << MEM_ATTR_UNCACHED_BIT_POS) | MEM_ATTR_UNCACHED_MAP;
 *
 * A page table entry will be created that maps VA = 0x24000000 | BIT(27)
 * = 0x2C000000 to PA 0x24000000, with the specified cacheable and shareable
 * attributes (non-cacheable and non-shareable in this case).
 *
 * Note that this is valid only for section (1 MB) and super section (16 MB)
 * entries. This attribute is ignored for page (large and small) entries
 */
#define MEM_ATTR_UNCACHED_MAP			0x100
#define MEM_ATTR_UNCACHED_BIT_POS		24

/* Higher nibble provides the outer cacheable policy params if
 * they are different from inner cacheable policy params
 */
#define MEM_ATTR_OUTER_CACHEABLE(C, WB, WA)	((WB | WA | C) << 4)

/* Memory access permissions */
#define MEM_ACCESS_NO_ACCESS	0x0
#define MEM_ACCESS_READWRITE	0x3
#define MEM_ACCESS_READONLY	0x7
#define MEM_ACCESS_NO_EXEC	0x8

/* Bit field positions and masks for section/super-section/small-page/large-page
 * entries in Short Descriptor Translation table format with TTBCR.N = 0
 */
#define ADDR_TO_L2_TABLE_INDEX_MASK	(0xFF000)
#define ADDR_TO_L2_TABLE_INDEX_SHIFT	(12)

#define PAGE_TABLE_BASE_ADDRESS_MASK	(0xFFFFFC00)
#define PAGE_TABLE_ENTRY_BITS		(BIT(0))

#define SMALL_PAGE_BASE_ADDRESS_MASK	(0xFFFFF000)
#define SMALL_PAGE_ENTRY_BITS		(BIT(1))
#define SMALL_PAGE_S_BIT_POS		(10)
#define SMALL_PAGE_TEX_BIT_POS		(6)
#define SMALL_PAGE_C_BIT_POS		(3)
#define SMALL_PAGE_B_BIT_POS		(2)
#define SMALL_PAGE_XN_BIT_POS		(0)
#define SMALL_PAGE_AP_BIT1_0_POS	(4)
#define SMALL_PAGE_AP_BIT2_POS		(9)

#define LARGE_PAGE_BASE_ADDRESS_MASK	(0xFFFF0000)
#define LARGE_PAGE_ENTRY_BITS		(BIT(0))
#define LARGE_PAGE_S_BIT_POS		(10)
#define LARGE_PAGE_TEX_BIT_POS		(12)
#define LARGE_PAGE_C_BIT_POS		(3)
#define LARGE_PAGE_B_BIT_POS		(2)
#define LARGE_PAGE_XN_BIT_POS		(15)
#define LARGE_PAGE_AP_BIT1_0_POS	(4)
#define LARGE_PAGE_AP_BIT2_POS		(9)

#define SECTION_BASE_ADDRESS_MASK	(0xFFF00000)
#define SECTION_ENTRY_BITS		(BIT(1))
#define SECTION_S_BIT_POS		(16)
#define SECTION_TEX_BIT_POS		(12)
#define SECTION_C_BIT_POS		(3)
#define SECTION_B_BIT_POS		(2)
#define SECTION_XN_BIT_POS		(4)
#define SECTION_AP_BIT1_0_POS		(10)
#define SECTION_AP_BIT2_POS		(15)

#define SUPER_SECTION_BASE_ADDRESS_MASK	(0xFF000000)
#define SUPER_SECTION_ENTRY_BITS	((BIT(18) | BIT(1)))
#define SUPER_SECTION_S_BIT_POS		(16)
#define SUPER_SECTION_TEX_BIT_POS	(12)
#define SUPER_SECTION_C_BIT_POS		(3)
#define SUPER_SECTION_B_BIT_POS		(2)
#define SUPER_SECTION_XN_BIT_POS	(4)
#define SUPER_SECTION_AP_BIT1_0_POS	(10)
#define SUPER_SECTION_AP_BIT2_POS	(15)

#define SMALL_PAGE_SIZE			(4*1024)
#define LARGE_PAGE_SIZE			(64*1024)
#define SECTION_SIZE			(1*1024*1024)
#define SUPER_SECTION_SIZE		(16*1024*1024)

/**
 * @brief Memory region information
 *
 * An array of this structure type is passed to the mmu_init() function
 * to create the translation tables. This structure specifies the a memory
 * region in the memory map, along with the memory type it should be configured
 * as and the access permission for the region.
 */
struct mmu_mem_region_info {
	/* Start address of the region, this should be aligned to the region
	 * size
	 */
	u32_t addr;

	/* Memory region size in bytes: Allowed sizes are 4K, 64K, 1M and 16M */
	u32_t size;

	/* Memory type */
	u8_t type;

	/* Memory attributes */
	u32_t attr;

	/* Memory access permissions */
	u8_t access;
};

/**
 * @brief MMU initialization function
 *
 * This routine takes the input memory region information passed and
 * initializes the first level translation table in the short descriptor
 * format (long descriptor format is not supported) to provide a flat memory
 * mapping from virtual address to physical address. All regions not specified
 * in the region info passed as configured with a fault entry.
 *
 * In addition, if the regions passed are smaller than the 'section' size
 * (1 MB), then it creates a second level translation table to configure the
 * memory region with the specified access permissions and attributes.
 *
 * Finally the MMU enable bit set in the SCTLR register if no errors are
 * encountered.
 *
 * @param mmu_mem_region_info is the pointer to the memory region info array
 *	  that specifies all the valid regions of memory and their access
 *	  permissions and memory attributes.
 * @param num_elems specifies the number of elements in the mmu_mem_region_info
 *	  array
 * @param l2_tt_mem is a pointer to the memory that can be used to create second
 *	  level translation tables if needed (size < 1M). This can be set to
 *	  NULL if all entries in the region info array have size >= 1M. Note
 *	  this pointer needs to be aligned to a 1KB boundary.
 * @param l2_tt_mem_size is the size of the memory pointed to by l2_tt_mem
 *
 * @return -EINVAL if there are alignment issues with the passed region info. Or
 *	    if there are memory region overlaps.
 *	   -ENOMEM if the passed l2_tt_mem_size is not sufficient to create
 *	    the 2nd level translation tables
 *	   0 on Success
 */
int arm_core_mmu_init(
		struct mmu_mem_region_info *rgn_info,
		u32_t num_elems,
		u8_t *l2_tt_mem,
		u32_t l2_tt_mem_size
	);

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXA_MMU__H_ */
