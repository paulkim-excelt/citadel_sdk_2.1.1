/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MMU initialization
 *
 * MMU initialization routines and helper functions
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <cortex_a/cp15.h>
#include <cortex_a/mmu.h>
#include <misc/util.h>
#include <errno.h>

/* Number of entries in first level translation table
 * - Here we use only TTBR0 which means it has one entry per 1 MB for a 4 GB map
 */
#define MMU_L1_TT_NUM_ENTRIES	(4096)

/* Number of entries in second level translation table
 * - one entry per 4KB page in 1MB map
 */
#define MMU_L2_TT_NUM_ENTRIES	(256)

/* Alignment requirement for first level translation table */
#define MMU_L1_TT_ALIGN		(16384)

/* MMU first level translation table */
static u32_t __attribute__((aligned(MMU_L1_TT_ALIGN)))
	mmu_l1_tt[MMU_L1_TT_NUM_ENTRIES];

#define MMU_TT_FAULT_ENTRY	(0x0)

/* Memory region sizes */
#define SIZE_4K		(4*1024)
#define SIZE_64K	(64*1024)
#define SIZE_1M		(1*1024*1024)
#define SIZE_16M	(16*1024*1024)

#define SIZE_1K		(1*1024)

/**
 * @brief get tex, c, b, bits : From Table B3-10 of ARMv7-A Arch Ref manual
 */
static void get_tex_c_b_bits(u8_t attr, u8_t type, u8_t *tex, u8_t *c, u8_t *b)
{
	u8_t o_attr;

	/* Memory type - Strongly ordered */
	if (type == MEM_TYPE_STRONGLY_ORDERED) {
		*tex = 0, *c = 0, *b = 0;
		return;
	}

	/* Memory type - Device */
	if (type == MEM_TYPE_DEVICE) {
		if (attr & MEM_ATTR_SHAREABLE)
			*tex = 0, *c = 0, *b = 1;
		else
			*tex = 2, *c = 0, *b = 0;
		return;
	}

	/* Memory type - Normal */
	if (attr & 0xF0) {
		/* Different cacheable attributes for inner and outer */
		if (attr & MEM_ATTR_CACHEABLE)
			if (attr & MEM_ATTR_CACHEABLE_WRITE_ALLOCATE)
				*tex = 5;
			else
				if (attr & MEM_ATTR_CACHEABLE_WRITE_BACK)
					*tex = 7;
				else
					*tex = 6;
		else
			*tex = 4;
		o_attr = attr >> 4;
		if (o_attr & MEM_ATTR_CACHEABLE)
			if (o_attr & MEM_ATTR_CACHEABLE_WRITE_ALLOCATE)
				*c = 0, *b = 1;
			else
				if (o_attr & MEM_ATTR_CACHEABLE_WRITE_BACK)
					*c = 1, *b = 1;
				else
					*c = 1, *b = 0;
		else
			*c = 0, *b = 0;
	} else {
		if (attr & MEM_ATTR_CACHEABLE) {
			if (attr & MEM_ATTR_CACHEABLE_WRITE_ALLOCATE) {
				*tex = 1, *c = 1, *b = 1;
			} else {
				*tex = 0;
				*c = 1;
				*b = (attr & MEM_ATTR_CACHEABLE_WRITE_BACK) ?
				    0x1 : 0x0;
			}
		} else {
			*tex = 1, *c = 0, *b = 0;
		}
	}
}

static int populate_page_table(struct mmu_mem_region_info *p, u8_t *mem)
{
	u32_t *entries = (u32_t *)mem;
	u32_t entry = 0, index, i;
	u8_t tex, c, b, ap;

	/* Bits 19:12 provide the index to the page table entry */
	index = (p->addr & ADDR_TO_L2_TABLE_INDEX_MASK)
		 >> ADDR_TO_L2_TABLE_INDEX_SHIFT;

	/* Handle small page */
	if (p->size == SIZE_4K) {
		/* Set address bits [31:12] */
		entry |= p->addr & SMALL_PAGE_BASE_ADDRESS_MASK;

		/* Set small page entry bit */
		entry |= SMALL_PAGE_ENTRY_BITS;

		/* Set shareable bit */
		entry |= (p->attr & MEM_ATTR_SHAREABLE) << SMALL_PAGE_S_BIT_POS;

		/* Get and set tex, c, b bits */
		get_tex_c_b_bits(p->attr, p->type, &tex, &c, &b);

		entry |=  tex << SMALL_PAGE_TEX_BIT_POS;
		entry |=  c << SMALL_PAGE_C_BIT_POS;
		entry |=  b << SMALL_PAGE_B_BIT_POS;

		/* Set xn bit */
		if (p->access & MEM_ACCESS_NO_EXEC)
			entry |= BIT(SMALL_PAGE_XN_BIT_POS);

		/* Set access permission bits AP[2:0] */
		ap = p->access;
		/* Set AP Bits[1:0] */
		entry |= (ap & 0x3) << SMALL_PAGE_AP_BIT1_0_POS;
		/* Set AP Bit[2] */
		entry |= ((ap >> 2) & 0x1) << SMALL_PAGE_AP_BIT2_POS;

		if (entries[index] != MMU_TT_FAULT_ENTRY)
			return -EINVAL;
		entries[index] = entry;
	} else { /* p->size = SIZE_4K */
		/* Set address bits [31:16] */
		entry |= p->addr & LARGE_PAGE_BASE_ADDRESS_MASK;

		/* Set large page entry bit */
		entry |= LARGE_PAGE_ENTRY_BITS;

		/* Set shareable bit */
		entry |= (p->attr & MEM_ATTR_SHAREABLE) << LARGE_PAGE_S_BIT_POS;

		/* Get and set tex, c, b bits */
		get_tex_c_b_bits(p->attr, p->type, &tex, &c, &b);

		entry |=  tex << LARGE_PAGE_TEX_BIT_POS;
		entry |=  c << LARGE_PAGE_C_BIT_POS;
		entry |=  b << LARGE_PAGE_B_BIT_POS;

		/* Set xn bit */
		if (p->access & MEM_ACCESS_NO_EXEC)
			entry |= BIT(LARGE_PAGE_XN_BIT_POS);

		/* Set access permission bits AP[2:0] */
		ap = p->access;
		/* Set AP Bits[1:0] */
		entry |= (ap & 0x3) << LARGE_PAGE_AP_BIT1_0_POS;
		/* Set AP Bit[2] */
		entry |= ((ap >> 2) & 0x1) << LARGE_PAGE_AP_BIT2_POS;

		/* Large page entries are required to be repeated
		 * 16 times
		 */
		for (i = index; i < (index + 16); i++) {
			if (entries[i] != MMU_TT_FAULT_ENTRY)
				return -EINVAL;
			entries[i] = entry;
		}
	}

	return 0;
}

/*
 * @brief Generate the translation table entry for a section
 */
static u32_t get_section_entry(struct mmu_mem_region_info *p)
{
	u32_t entry = 0;
	u8_t tex, c, b, ap;

	/* Set address bits [31:20] */
	entry |= p->addr & SECTION_BASE_ADDRESS_MASK;

	/* Map uncached => cached by clearing the uncached mapping bit */
	if (p->attr & MEM_ATTR_UNCACHED_MAP)
		entry &= ~BIT(p->attr >> MEM_ATTR_UNCACHED_BIT_POS);

	/* Set section entry bit */
	entry |= SECTION_ENTRY_BITS;

	/* Set shareable bit */
	entry |= (p->attr & MEM_ATTR_SHAREABLE) << SECTION_S_BIT_POS;

	/* Get and set tex, c, b bits */
	get_tex_c_b_bits(p->attr, p->type, &tex, &c, &b);

	entry |=  tex << SECTION_TEX_BIT_POS;
	entry |=  c << SECTION_C_BIT_POS;
	entry |=  b << SECTION_B_BIT_POS;

	/* Set xn bit */
	if (p->access & MEM_ACCESS_NO_EXEC)
		entry |= BIT(SECTION_XN_BIT_POS);

	/* Set access permission bits AP[2:0] */
	ap = p->access;
	/* Set AP Bits[1:0] */
	entry |= (ap & 0x3) << SECTION_AP_BIT1_0_POS;
	/* Set AP Bit[2] */
	entry |= ((ap >> 2) & 0x1) << SECTION_AP_BIT2_POS;

	return entry;
}

/*
 * @brief Generate the translation table entry for a super section
 */
static u32_t get_super_section_entry(struct mmu_mem_region_info *p)
{
	u32_t entry = 0;
	u8_t tex, c, b, ap;

	/* Set address bits [31:24] */
	entry |= p->addr & SUPER_SECTION_BASE_ADDRESS_MASK;

	/* Map uncached => cached by clearing the uncached mapping bit */
	if (p->attr & MEM_ATTR_UNCACHED_MAP)
		entry &= ~BIT(p->attr >> MEM_ATTR_UNCACHED_BIT_POS);

	/* Set super section entry bits */
	entry |= SUPER_SECTION_ENTRY_BITS;

	/* Set shareable bit */
	entry |= (p->attr & MEM_ATTR_SHAREABLE) << SUPER_SECTION_S_BIT_POS;

	/* Get and set tex, c, b bits */
	get_tex_c_b_bits(p->attr, p->type, &tex, &c, &b);

	entry |=  tex << SUPER_SECTION_TEX_BIT_POS;
	entry |=  c << SUPER_SECTION_C_BIT_POS;
	entry |=  b << SUPER_SECTION_B_BIT_POS;

	/* Set xn bit */
	if (p->access & MEM_ACCESS_NO_EXEC)
		entry |= BIT(SUPER_SECTION_XN_BIT_POS);

	/* Set access permission bits AP[2:0] */
	ap = p->access & 0x7;
	/* Set AP Bits[1:0] */
	entry |= (ap & 0x3) << SUPER_SECTION_AP_BIT1_0_POS;
	/* Set AP Bit[2] */
	entry |= ((ap >> 2) & 0x1) << SUPER_SECTION_AP_BIT2_POS;

	return entry;
}

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
 * Note that the domain is set to 0 for all section and page entries. This is
 * mapped to the 'Clients' domain in DACR.
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
	)
{
	int ret;
	u32_t i, j, regval;
	u32_t entry, index;
	struct mmu_mem_region_info *p;
	u8_t *next_avail = l2_tt_mem, *page_table;

	/* Atleast one valid memory region is required */
	if ((rgn_info == NULL) || (num_elems == 0))
		return -EINVAL;

	/* Populate first level TT with fault entries */
	for (i = 0; i < MMU_L1_TT_NUM_ENTRIES; i++)
		mmu_l1_tt[i] = MMU_TT_FAULT_ENTRY;

	/* Loop through each region info entry */
	for (i = 0; i < num_elems; i++) {
		p = &rgn_info[i];

		/* Check alignment */
		if (p->addr & (p->size - 1))
			return -EINVAL;

		/* Bits[31:20] give the index into the TT */
		index = p->addr >> 20;

		switch (p->size) {
		case SIZE_4K:
		case SIZE_64K:
			if (mmu_l1_tt[index] != MMU_TT_FAULT_ENTRY) {
				/* Carve out a new page table */
				if ((next_avail + SIZE_1K) >
					(l2_tt_mem + l2_tt_mem_size)) {
					return -ENOMEM;
				}

				/* Check alignemnt */
				if ((u32_t)next_avail & (SIZE_1K - 1))
					return -EINVAL;
				/* Init to fault entries */
				for (j = 0; j < SIZE_1K; j++)
					next_avail[j] = 0;

				page_table = next_avail;
				/* Update available memory for 2nd level TT */
				next_avail += SIZE_1K;
			} else {
				/* Use existing page table */
				page_table =
				    (u8_t *)(mmu_l1_tt[index] & ~(SIZE_1K-1));
			}

			/* Populate page table */
			ret = populate_page_table(p, page_table);
			if (ret)
				return ret;

			/* Set address bits [31:10] and page entry bit */
			mmu_l1_tt[index] =
			    ((u32_t)page_table & PAGE_TABLE_BASE_ADDRESS_MASK)
				| PAGE_TABLE_ENTRY_BITS;
			break;
		case SIZE_1M:
			entry = get_section_entry(p);
			if (mmu_l1_tt[index] != MMU_TT_FAULT_ENTRY)
				return -EINVAL;
			mmu_l1_tt[index] = entry;
			break;
		case SIZE_16M:
			entry = get_super_section_entry(p);
			/* Super section entries are required to be repeated
			 * 16 times
			 */
			for (j = index; j < (index + 16); j++) {
				if (mmu_l1_tt[j] != MMU_TT_FAULT_ENTRY)
					return -EINVAL;
				mmu_l1_tt[j] = entry;
			}
			break;
		default:
			return -EINVAL;
		}
		/* Check alignment */
	}

	/* Configure TTBCR and TTBR0 */
	regval = (u32_t)mmu_l1_tt & ~(MMU_L1_TT_ALIGN-1); /* Bits [31:14] */
	write_ttbr0(regval);
	/* TTBCR.N = 0, TTBCR.EAE = 0, TTBCR.PD0 = 0 */
	write_ttbcr(0x0);

	/* Write DACR - Set domain 0 to client */
	write_dacr(0x1);

	/* Invalidate TLB before enabling MMU */
	write_itlbiall(0x0);
	write_dtlbiall(0x0);
	write_tlbiall(0x0);

	regval = read_sctlr();
	/* Clear SCTLR.AFE (Access flag enable) */
	regval &= ~SCTLR_ACCESS_FLAG_EN_BIT;
	/* Clear SCTLR.TRE (TEX remap enable) */
	regval &= ~SCTLR_TEX_REMAP_BIT;
	/* Set SCTLR.M (MMU enable bit) */
	regval |= SCTLR_MMU_ENABLE_BIT;
	write_sctlr(regval);

	return 0;
}
