/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/* @file sotp.c
 *
 * This file contains the secure OTP programming and reading API's
 *
 *
 */

/** @defgroup sotp
 * Connectivity SDK: SOTP read and write operation.
 *  @{
 */

/*
 *   TODO or FIXME
 *   1. All the magic numbers should be replaced with #defines
 *
 *
 **/
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <arch/cpu.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <logging/sys_log.h>
#include <sotp.h>
#include "sotp_crc_table.h"

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
#define OTP_MAX_VCO_ROW	15
#define OTP_MAX_VCO_START_BIT	20
#define OTP_MAX_VCO_WIDTH	2
#define OTP_BVC_ROW	15
#define OTP_BVC_BIT	17
#define CRMU_CHIP_OTPC_RST_CNTRL 0x3001c064

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/
/**
 * @brief       SOTP fuse read
 * @details     This function reads the fuse value
 *
 * @param[in]   addr - address of the SOTP row to read
 * @param[in]   fn -
 * @param[in]   regc_sel -
 * @param[in]   tm_r -
 * @param[in]   vsel_r -
 *
 * @return      returns the read value
 */
static u64_t sotp_fuse_read(u32_t addr, u32_t fn, u32_t regc_sel, u32_t tm_r,
							u32_t vsel_r);
/**
 * @brief       Calculate ECC
 * @details     This function calculates the ECC of the given data
 *
 * @param[in]   data - data over which ECC needs to be calculated
 *
 * @return      returns the ECC value
 */
static u32_t calc_ecc(u32_t data);

/**
 * @brief       Reflection
 * @details     This function Reflects the input data about it's centre bit
 *
 * @param[in]   data - data for the reflection needs to be calculated
 * @param[in]   nbits - Number of bits
 *
 * @return      returns the reflect value
 */
static u32_t reflect(u32_t data, u8_t nbits);

/**
 * @brief       Calculate CRC32
 * @details     This function calculates the crc 32 for the given array
 *
 * @param[in]   initval - Initialization value to the CRC32
 * @param[in]   charArr - Character array for which the CRC32 needs
 *							to be calculated
 * @param[in]   arraySize - Number of bytes
 *
 * @return      returns the crc32 value
 */
u32_t calc_crc32(u32_t initval, u8_t *charArr, u32_t arraySize);

/**
 * @brief       SOTP redundancy reduct
 * @details     This function reducts the redundant bits in the given row data
 *
 * @param[in]   sotp_row_data - SOTP row data
 *
 * @return      returns the row value
 */
static u32_t sotp_redundancy_reduct(u32_t sotp_row_data);

/**
 * @brief Count the number of one's
 * @details Count the number of one's
 *
 * @param[in] v value in which the number of one's to be counted
 *
 * @return the number of one's
 */
static u32_t bit_count(u64_t v);

/**
 * @brief Count the number of one's
 * @details Count the number of one's
 *
 * @param[in] v value in which the number of one's to be counted
 *
 * @return the number of one's
 */
static sotp_bcm58202_status_t sotp_read_key_helper(u8_t *key, u16_t *keySize,
					u32_t row_start, u32_t row_end);

/**
 * @brief Count the number of one's
 * @details Count the number of one's
 *
 * @param[in] v value in which the number of one's to be counted
 *
 * @return the number of one's
 */
static sotp_bcm58202_status_t sotp_set_key_helper(u8_t *key, u16_t keySize,
				u32_t row_start, u32_t row_end,
				u32_t wr_lock_flag, u64_t prg_lock_flag);
/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/
u64_t sotp_mem_read(u32_t addr, u32_t sotp_add_ecc)
{
	u64_t read_data   = 0;
	u64_t read_data1  = 0;
	u64_t read_data2  = 0;
#ifdef SOTP_DEBUG
	SYS_LOG_WRN("Enter sotp_mem_read\n");
#endif

	/* Check for FDONE status */
	while ((sys_read32(SOTP_REGS_OTP_STATUS_0) &
		   (1 << SOTP_REGS_OTP_STATUS_0__FDONE)) !=
		   (1 << SOTP_REGS_OTP_STATUS_0__FDONE)) {
		;
	}
#ifdef SOTP_DEBUG
	SYS_LOG_WRN("FDONE\n");
#endif
	 /* Enable OTB acces by CPU */
	sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);

	if (sotp_add_ecc == 1) {
		sys_clear_bit(SOTP_REGS_OTP_PROG_CONTROL,
			SOTP_REGS_OTP_PROG_CONTROL__OTP_DISABLE_ECC);
	}

	if (sotp_add_ecc == 0) {
		sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
			SOTP_REGS_OTP_PROG_CONTROL__OTP_DISABLE_ECC);
	}

	/* 10 bit row address */
	sys_write32(((addr & 0x3ff) << SOTP_REGS_OTP_ADDR__OTP_ROW_ADDR_R),
			 SOTP_REGS_OTP_ADDR);
	sys_write32((SOTP_READ << SOTP_REGS_OTP_CTRL_0__OTP_CMD_R),
			 SOTP_REGS_OTP_CTRL_0);

	/* Start bit to tell SOTP to send command to the OTP controller */
	sys_set_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("wait for CMD done\n");
#endif

	/* Wait for SOTP command done to be set */
	while ((sys_read32(SOTP_REGS_OTP_STATUS_1) &
		   (1<<SOTP_REGS_OTP_STATUS_1__CMD_DONE)) !=
		   (1<<SOTP_REGS_OTP_STATUS_1__CMD_DONE)) {
		;
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("CMD Done\n");
#endif

	/* Clr Start bit after command done */
	sys_clear_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

	/* ECC Det */
	if ((addr > 15) &&
		(sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x20000)) {
#ifdef SOTP_DEBUG
	SYS_LOG_WRN("SOTP ECC ERROR Detected ROW %d\n", addr);
#endif
		read_data = SOTP_ECC_ERR_DETECT;
	} else {
		read_data1 = (u64_t) sys_read32(SOTP_REGS_OTP_RDDATA_0);
		read_data1 = read_data1 & 0xFFFFFFFF;
		read_data2 = (u64_t) sys_read32(SOTP_REGS_OTP_RDDATA_1);
		read_data2 = (read_data2 & 0x1ff) << 32;
		read_data  = read_data2 | read_data1;
	}

	/* Command done is cleared */
	sys_set_bit(SOTP_REGS_OTP_STATUS_1, SOTP_REGS_OTP_STATUS_1__CMD_DONE);
	/* Disable OTP acces by CPU */
	sys_clear_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("read done\n");
#endif
	return read_data;
}

u64_t sotp_mem_write(u32_t addr, u32_t sotp_add_ecc, u64_t wdata)
{
	u32_t  loop;
	u8_t  prog_array[4] = {0x0F, 0x04, 0x08, 0x0D};
	u64_t fuse0_v;
	u64_t fuse1_v;
	u64_t fuse0_n;
	u64_t fuse1_n;
	u64_t failed_bits = 0;
	u64_t bit;
	u64_t read_data;
	int i;
	u32_t value;

#ifdef SOTP_DEBUG
	u32_t status0, status1;
#endif
	value = sys_read32(SOTP_REGS_OTP_CLK_CTRL);

	/*
	 * Slow OTP clock to a "safer" range.
	 */
	if ((value&0x6) != 0x6) {
		/* Disable Clock divider */
		if ((value & 0x1) == 0x1)
			sys_write32((value & 0xFFFFFFFE),
				    SOTP_REGS_OTP_CLK_CTRL);


		value |= 0x6;
		sys_write32(value, SOTP_REGS_OTP_CLK_CTRL);
		/* Enable Clock divider */
		sys_write32(value|0x1, SOTP_REGS_OTP_CLK_CTRL);
	}

	/*
	 * Get inverse of existing value so that existing bits can be removed
	 * during write and verification check of value written.
	 */
	read_data = sotp_mem_read(addr, sotp_add_ecc);

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("TMPP: write row:%d Write:0x%08x-%08x Read:0x%08x-0x%08x\n",
	addr,
	(u32_t)((wdata >> 32) & 0xffffffff),
	(u32_t)(wdata & 0xffffffff),
	(u32_t)((read_data >> 32) & 0xffffffff),
	(u32_t)(read_data & 0xffffffff));
#endif

	if (read_data & SOTP_ECC_ERR_DETECT)
		return 0xFFFFFFFFFFFFFFFFLL;

	read_data = ~read_data;
	wdata &= read_data;

	if (wdata == 0LL)
		return 0LL;

	/* Check for FDONE status */
	while ((sys_read32(SOTP_REGS_OTP_STATUS_0) &
		   (0x1 << SOTP_REGS_OTP_STATUS_0__FDONE)) !=
		   (1 << SOTP_REGS_OTP_STATUS_0__FDONE)) {
		;
	}

	/* Enable OTP acces by CPU */
	sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);

	if (sotp_add_ecc == 0) {
		sys_clear_bit(SOTP_REGS_OTP_PROG_CONTROL,
			    SOTP_REGS_OTP_PROG_CONTROL__OTP_ECC_WREN);
	} else if (sotp_add_ecc == 1) {
		sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
			    SOTP_REGS_OTP_PROG_CONTROL__OTP_ECC_WREN);
	}

	sys_write32((sys_read32(SOTP_REGS_OTP_CTRL_0) & 0xffffffc1) |
			(SOTP_PROG_ENABLE << 1), SOTP_REGS_OTP_CTRL_0);

	/*
	 * In order to avoid unintentional writes programming of the OTP array,
	 * the OTP Controller must be put into programming mode before it will
	 * accept program commands. This is done by writing 0xF, 0x4, 0x8, 0xD
	 * with program commands prior to starting the actual
	 * programming sequence
	 */
	for (loop = 0; loop < 4; loop++) {
		sys_write32(prog_array[loop], SOTP_REGS_OTP_ADDR);

	    /* Start bit to tell SOTP to send command to the OTP controller */
		sys_set_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

		while ((sys_read32(SOTP_REGS_OTP_STATUS_1) &
				(1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) !=
				(1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) {
			;
		}

		sys_set_bit(SOTP_REGS_OTP_STATUS_1,
				SOTP_REGS_OTP_STATUS_1__CMD_DONE);
		sys_clear_bit(SOTP_REGS_OTP_CTRL_0,
			      SOTP_REGS_OTP_CTRL_0__START);
	}

	while ((sys_read32(SOTP_REGS_OTP_STATUS_0) &
		(1 << SOTP_REGS_OTP_STATUS_0__PROG_OK)) !=
		(1 << SOTP_REGS_OTP_STATUS_0__PROG_OK)) {
		;
	}

	/* Set  10 bit row address */
	sys_write32(((addr & 0x3ff) << 6), SOTP_REGS_OTP_ADDR);
	/* Set SOTP Row data */
	sys_write32((wdata & 0xffffffff), SOTP_REGS_OTP_WRDATA_0);
	 /* Set SOTP ECC and error bits */
	sys_write32(((wdata & 0x1ff00000000) >> 32), SOTP_REGS_OTP_WRDATA_1);
	sys_write32((sys_read32(SOTP_REGS_OTP_CTRL_0)
			& 0xffffffc1) |
			(SOTP_PROG_WORD << 1) |
			(1 << SOTP_REGS_OTP_CTRL_0__BURST_START_SEL) |
			(1 << SOTP_REGS_OTP_CTRL_0__WRP_CONTINUE_ON_FAIL),
			SOTP_REGS_OTP_CTRL_0);

	/* For Row0-7, we need to set access mode to 'manufacturing'
	 * For rest, access mode will be 'prog_en'
	 */
	if (addr < 8) {
		sys_write32((sys_read32(SOTP_REGS_OTP_CTRL_0) &
				0xff3fffff) | (0x1 << 22),
				SOTP_REGS_OTP_CTRL_0);
	} else {
		sys_write32((sys_read32(SOTP_REGS_OTP_CTRL_0) &
				0xff3fffff) | (0x2 << 22),
				SOTP_REGS_OTP_CTRL_0);
	}

	sys_set_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__OTP_PROG_EN);

	sys_set_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

	while ((sys_read32(SOTP_REGS_OTP_STATUS_1) &
		(1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) !=
		(1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) {
		;
	}

#ifdef SOTP_DEBUG
	/* AB501 simple error checking using SOTP_REGS_OTP_STATUS_0 and 1 */
	status0 = sys_read32(SOTP_REGS_OTP_STATUS_0);
	status1 = sys_read32(SOTP_REGS_OTP_STATUS_1);

	/* AB501 Print only if we see something different than expected */
	if ((status0&(~0x108f)) || (status1&(~0x3ff00002))) {
		SYS_LOG_WRN("TMPP:write:row=%d status0=0x%08x status1=0x%08x\n",
				addr,
				status0,
				status1);
		SYS_LOG_WRN("TMPP: write: row=%d Error bits:0x%08x-0x%08x\n",
				addr,
				sys_read32(SOTP_REGS_OTP_RDDATA_0),
				(0x1ff &
				sys_read32(SOTP_REGS_OTP_RDDATA_1)));
	}
#endif

	sys_set_bit(SOTP_REGS_OTP_STATUS_1, SOTP_REGS_OTP_STATUS_1__CMD_DONE);
	sys_clear_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);
	sys_clear_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__OTP_PROG_EN);
	sys_clear_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);

	/*
	 * Read Fuse0 Verify, Fuse1 Verify and Verify0. Return a bit mask of
	 * failed bits.
	 */
	fuse0_n = sotp_fuse_normal_read(addr, 0) & read_data;
	fuse1_n = sotp_fuse_normal_read(addr, 1) & read_data;
	fuse0_v = sotp_fuse_verify_read(addr, 0) & read_data;
	fuse1_v = sotp_fuse_verify_read(addr, 1) & read_data;

	/*
	 * If ECC is HW add ECC to wdata.
	 */
	if (sotp_add_ecc == 1)
		wdata |= ((u64_t)(calc_ecc((u32_t)wdata)) << 32);

	for (i = 0; i <= 39; i++) {
		bit = 1LL << i;

		if (((fuse0_v & bit) == (wdata & bit)) &&
		    ((fuse1_v & bit) == (wdata & bit)))
			continue; /* PASS */
		if (((fuse0_v & bit) == (wdata & bit)) &&
		    ((fuse1_n & bit) == (wdata & bit)))
			continue; /* PASS */
		if (((fuse0_n & bit) == (wdata & bit)) &&
		    ((fuse1_v & bit) == (wdata & bit)))
			continue; /* PASS */

		failed_bits |= bit;
	}

#ifdef SOTP_DEBUG
	if (failed_bits != 0L) {
		SYS_LOG_WRN(
		"TMPP:write:row=%d Failed bits:0x%08x-0x%08x 0v:0x%08x-0x%08x ",
		addr,
		(u32_t)((failed_bits >> 32) & 0xffffffff),
		(u32_t)(failed_bits & 0xffffffff),
		(u32_t)((fuse0_v >> 32) & 0xffffffff),
		(u32_t)(fuse0_v & 0xffffffff));

		SYS_LOG_WRN("1v:0x%08x-0x%08x0n:0x%08x-0x%08x1n:0x%08x-0x%08x ",
				(u32_t)((fuse1_v >> 32) & 0xffffffff),
				(u32_t)(fuse1_v & 0xffffffff),
				(u32_t)((fuse0_n >> 32) & 0xffffffff),
				(u32_t)(fuse0_n & 0xffffffff),
				(u32_t)((fuse1_n >> 32) & 0xffffffff),
				(u32_t)(fuse1_n & 0xffffffff));

		SYS_LOG_WRN("wdata:0x%08x-0x%08x\n",
				(u32_t)((wdata >> 32) & 0xffffffff),
				(u32_t)(wdata & 0xffffffff));
	}

	if (failed_bits != 0L)
		SYS_LOG_WRN("SOTP: write: row=%d Failed bits:0x%08x-0x%08x\n",
				addr,
				(u32_t)((failed_bits >> 32) & 0xffffffff),
				(u32_t)(failed_bits & 0xffffffff));
#endif /* SOTP_DEBUG */

	return failed_bits;
}

u64_t sotp_fuse_normal_read(u32_t addr, u32_t fn)
{
	return sotp_fuse_read(addr, fn, 0x4, 0x4, 0x5);
}

u64_t sotp_fuse_verify_read(u32_t addr, u32_t fn)
{
	return sotp_fuse_read(addr, fn, 0x4, 0x84, 0xA);
}

sotp_bcm58202_status_t sotp_is_fastauth_enabled(void)
{
	u64_t section3_row12_data;
	u64_t section3_row13_data;
	u32_t chip_device_type;      /* 5 bits */
	u32_t chip_state_reg;
	u32_t encoded_chip_device_type;      /* 9 bits */
	u32_t manu_debug_strap;

	 /*No ecc for section 3*/
	section3_row12_data = sotp_mem_read(12, 0);
	section3_row13_data = sotp_mem_read(13, 0);

	chip_device_type = sotp_redundancy_reduct(
				(u32_t)(section3_row12_data & 0x3ff));

	/*EDTYPE_UNASSIGNED_0 starts from 64th location */
	/* 9*2 bits input to function*/
	encoded_chip_device_type = sotp_redundancy_reduct(
				(u32_t)((section3_row13_data >> 23) & 0x3ffff));

	chip_state_reg = sys_read32(SOTP_REGS_SOTP_CHIP_STATES) & 0x3f;

	manu_debug_strap = ((sys_read32(SOTP_REGS_SOTP_CHIP_STATES) >>
				SOTP_REGS_SOTP_CHIP_STATES__MANU_DEBUG) & 0x1);

	/* Actual development test starts here */
	if ((chip_state_reg == CHIP_STATE_AB_DEV) &&
		(chip_device_type == DEVICE_TYPE_AB_DEV) &&
		(encoded_chip_device_type == EDTYPE_ABdev) &&
		(manu_debug_strap == 1)) {

		return IPROC_OTP_FASTAUTH;
	}
	return IPROC_OTP_NOFASTAUTH;
}

sotp_bcm58202_status_t sotp_is_sotp_erased(void)
{
	u64_t row_data;
	sotp_bcm58202_status_t status = IPROC_OTP_VALID;

	row_data = sotp_mem_read(12, 0);

	if (row_data & 0x30000)
		status = IPROC_OTP_ERASED;
	else if (sys_read32(SOTP_REGS_SOTP_CHIP_STATES) & 0x10000)
		status = IPROC_OTP_ERASED;

	return status;
}

sotp_bcm58202_status_t sotp_read_fips_mode(u32_t *FIPSMode)
{
	u64_t row_data;
	sotp_bcm58202_status_t status = IPROC_OTP_VALID;

	row_data = sotp_mem_read(14, 0);
	*FIPSMode = (sotp_redundancy_reduct((u32_t)(row_data & 0xc0)))>>3;

	return status;
}

sotp_bcm58202_status_t sotp_read_dev_config(
				struct sotp_bcm58202_dev_config *devConfig)
{
	/* Section 4: Region 4: Rows 16 to 19 */
	u64_t row_data;
	sotp_bcm58202_status_t status = IPROC_OTP_VALID;

	/*Check for CRC Error Section 4 */
	if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x100000) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("OTP Command not set\n");
#endif
		return IPROC_OTP_INVALID;
	}

	/* Row 16 : If programmed and ECC Error not detected,
	 * this row contains BRCM ID and Security Config
	 */
	row_data = sotp_mem_read(16, 1);

	if ((row_data == 0x0LL) || (row_data == SOTP_ECC_ERR_DETECT)) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("ECC Error Detected Row 16\n");
#endif
		status = IPROC_OTP_INVALID;
	} else {
		devConfig->BRCMRevisionID = row_data & 0xFFFF;
		devConfig->devSecurityConfig = (row_data >> 16) & 0xFFFF;
	}

	/* Row 17 : If programmed and ECC Error not detected,
	 * this row contains  ProductID(COTBindingID)
	 */
	row_data = sotp_mem_read(17, 1);
	if ((row_data == 0x0LL) || (row_data == SOTP_ECC_ERR_DETECT)) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("ECC Error Detected Row 17\n");
#endif
		status = IPROC_OTP_INVALID;
	} else {
		devConfig->ProductID = row_data & 0xFFFF;
	}
	return status;
}

sotp_bcm58202_status_t sotp_read_sbl_config(u32_t *SBLConfig)
{
	u32_t CustomerID;
	u64_t row_data;
	sotp_bcm58202_status_t status;

	status = sotp_read_customer_id(&CustomerID);

	if (status == IPROC_OTP_INVALID) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("SOTP Customer ID Invalid\n");
#endif
		return IPROC_OTP_INVALID;
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("Customer ID Read = 0x%x\n", CustomerID);
#endif
	if ((sotp_is_ABProd() || sotp_is_ABDev()) && CustomerID != 0) {
		/* read from Section 5 and fill the SBL Config */
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("AB Production\n");
#endif

		 /* Check for CRC Error Section 5 */
		if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x200000) {
#ifdef SOTP_DEBUG
			SYS_LOG_WRN("CRC Error Section 5\n");
#endif
			return IPROC_OTP_INVALID;
		}

		/* Row 21 : If programmed and ECC Error not detected,
		 * this row contains Customer SBLConfiguration
		 */
		row_data = sotp_mem_read(21, 1);

		if ((row_data == 0x0LL) || (row_data == SOTP_ECC_ERR_DETECT) ||
			(row_data & 0x18000000000)) {
#ifdef SOTP_DEBUG
			SYS_LOG_WRN("ECC Error Detected Row 21\n");
#endif
			status = IPROC_OTP_INVALID;
		} else {
#ifdef SOTP_DEBUG
			SYS_LOG_WRN("SBL Config read\n");
#endif
			*SBLConfig = row_data & 0xFFFFFFFF;
		}
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("return IPROC_OTP_VALID\n");
#endif

	return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_read_customer_id(u32_t *customerID)
{
	/* Section 5: Region 5: Rows 20
	 * Fail bits, ECC bits, Row 23: CRC
	 */
	u64_t row_data;

	row_data = sotp_mem_read(20, 1);

	/* Return customerID  if a valid ROW is found;
	 * else return IPROC_OTP_INVALID
	 */
	if (!((row_data & SOTP_ECC_ERR_DETECT) ||
		  (row_data & 0x18000000000))) {

		/* returning the entire 32 bits;
		 * If only 24 bits of valid CID is returned,
		 * how to identify whether dev or production code.
		 */

		*customerID = (row_data & 0xFFFFFFFF);

#ifdef SOTP_DEBUG
		SYS_LOG_WRN("SOTP Customer ID = 0x%x\n", *customerID);
#endif
		return IPROC_OTP_VALID;
	}
#ifdef SOTP_DEBUG
	SYS_LOG_WRN("SOTP Customer ID Invalid\n");
#endif
	return IPROC_OTP_INVALID;
}

sotp_bcm58202_status_t sotp_read_customer_config(u16_t *CustomerRevisionID,
						 u16_t *SBIRevisionID)
{
	/* Section 6: Region 6: Rows 24 to 27
	 * ECC cannot be trsuted as ECC is not updated when the rows are updated
	 * No CRC, Only Redundancy
	 * All valid bits rows take part in a voting scheme.
	 * Voting is on a bit basis.
	 */

	u64_t row_data[4], rowdata;
	int i = 0, j = 0, k = 0;
	u32_t row = 24;

	/* Read and store all valid rows in an array */
	do {
		rowdata = sotp_mem_read(row, 0);
		if (!(rowdata & 0x18000000000)) {
			row_data[i] = rowdata;
			i++;
		}
		row++;
	} while (row < 27);

	/* If no valid row, return IPROC_OTP_INVALID */
	if (i == 0) {
		return IPROC_OTP_INVALID;
	} else if (i == 1) {
		/* If only 1 valid row, return the value from that row. */
		*CustomerRevisionID = (row_data[i-1]>>16) & 0xFFFF;
		*SBIRevisionID = row_data[i-1] & 0xFFFF;
	} else if (i == 2) {
		/* If two valid rows, any bit set in either
		 * row is a valid bit
		 */
		rowdata = row_data[0] | row_data[1];
		*CustomerRevisionID = (rowdata>>16) & 0xFFFF;
		*SBIRevisionID = rowdata & 0xFFFF;
	} else {
		/* If more that two valid rows, set all bits set in
		 * 50% of valid rows.
		 */
		rowdata = 0x0;
		for (j = 0; j <= i-2; j++) {
			for (k = j+1; k <= i-1; k++)
				rowdata = rowdata | (row_data[j]&row_data[k]);
		}
		*CustomerRevisionID = (rowdata>>16) & 0xFFFF;
		*SBIRevisionID = rowdata & 0xFFFF;
	}
	return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_read_key(u8_t *key, u16_t *keySize,
				     sotp_bcm58202_key_type_t type)
{
	if (type == OTPKey_DAUTH) {
		/* DAUTH: Section 7, Region 7,8,9 : Rows 28-39
		 * Row36 - CRC, Rows 37,38,39 - Redundant rows
		 */
		/* Wait for SOTP command done to be set */
		if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x800000)
			return IPROC_OTP_INVALID;

		return sotp_read_key_helper(key, keySize, 28, 39);

	} else if (type == OTPKey_HMAC_SHA256) {
		/* KHMAC : Section 8, Region 10,11,12 : Rows 40-51
		 * Row48 - CRC, Rows 49,50,51 - Redundant rows
		 */

		/* Wait for SOTP command done to be set */

		if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x1000000)
			return IPROC_OTP_INVALID;

		return sotp_read_key_helper(key, keySize, 40, 51);

	} else if (type == OTPKey_AES) {

		/* KAES : Section 9, Region 13,14,15 : Rows 52-63
		 * Row60 - CRC, Rows 61,62,63- Redundant rows
		 */
		/* Wait for SOTP command done to be set */
		if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x2000000)
			return IPROC_OTP_INVALID;

		return sotp_read_key_helper(key, keySize, 52, 63);

	} else if (type == OTPKey_ecDSA) {
		/* DAUTH: Section 10, Region 16,17,18 : Rows 64-75 :
		 * Row72 - CRC, Rows 73,74,75 - Redundant rows
		 */
		/* Wait for SOTP command done to be set */
		if (sys_read32(SOTP_REGS_OTP_STATUS_1) & 0x4000000)
			return IPROC_OTP_INVALID;

		return sotp_read_key_helper(key, keySize, 64, 75);
	}

	return IPROC_OTP_INVALID;
}

sotp_bcm58202_status_t sotp_is_ABProd(void)
{
	u64_t secConfig, encodeType;
	u64_t section3_row12_data, section3_row13_data;

	 /* No ecc for section 3 */
	section3_row12_data = sotp_mem_read(12, 0);
	section3_row13_data = sotp_mem_read(13, 0);
	secConfig = sotp_redundancy_reduct(
			(u32_t)(section3_row12_data & 0x3ff));

	 /*EDTYPE_UNASSIGNED_0 starts from 64th location */
	/* 9*2 bits input to function */
	encodeType = sotp_redundancy_reduct((u32_t)((section3_row13_data >> 23)
					& 0x3ffff));

	if (secConfig != DEVICE_TYPE_AB_PROD) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("Device type not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}

	if (encodeType != EDTYPE_ABprod) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("encodeType not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}
#ifdef SOTP_DEBUG
	SYS_LOG_WRN("AB Production\n");
#endif
	return  IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_is_ABDev(void)
{
	u64_t secConfig, encodeType;
	u64_t section3_row12_data, section3_row13_data;

	 /*No ecc for section 3*/
	section3_row12_data = sotp_mem_read(12, 0);
	section3_row13_data = sotp_mem_read(13, 0);
	secConfig = sotp_redundancy_reduct(
			(u32_t)(section3_row12_data & 0x3ff));

	/*EDTYPE_UNASSIGNED_0 starts from 64th location */
	/* 9*2 bits input to function*/
	encodeType = sotp_redundancy_reduct((u32_t)((section3_row13_data >> 23)
					& 0x3ffff));

	if (secConfig != DEVICE_TYPE_AB_DEV) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("Device type not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}

	if (encodeType != EDTYPE_ABdev) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("encodeType not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("AB Development\n");
#endif
	return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_is_unassigned(void)
{
	u64_t secConfig, encodeType;
	u64_t section3_row12_data, section3_row13_data;

	 /* No ecc for section 3 */
	section3_row12_data = sotp_mem_read(12, 0);
	section3_row13_data = sotp_mem_read(13, 0);
	secConfig  = sotp_redundancy_reduct(
			(u32_t)(section3_row12_data & 0x3ff));
	 /*EDTYPE_UNASSIGNED_0 starts from 64th location */
	/* 9*2 bits input to function*/
	encodeType = sotp_redundancy_reduct((u32_t)((section3_row13_data >> 23)
					& 0x3ffff));

	if (secConfig != DEVICE_TYPE_UNASSIGNED) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("Device type not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}

	if (encodeType != EDTYPE_unassigned) {
#ifdef SOTP_DEBUG
		SYS_LOG_WRN("encodeType not valid\n");
#endif
		return IPROC_OTP_INVALID;
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("Unassigned\n");
#endif
	return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_is_nonAB(void)
{
	u64_t secConfig, encodeType;
	/* Security Configuration  - nonAB */
	u64_t row_data = 0x0000000000000030;

	secConfig = sotp_mem_read(12, 0);
	if (row_data != secConfig)
		return IPROC_OTP_INVALID;

	/* Encoding type */
	row_data = 0x18067800000;
	encodeType = sotp_mem_read(13, 0);

	if (row_data != encodeType)
		return IPROC_OTP_INVALID;

	return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_set_key(u8_t *key, u16_t keySize,
				sotp_bcm58202_key_type_t type)
{
	u32_t wr_lock_flag = 0;
	u64_t prg_lock_flag = 0;

	if (type == OTPKey_DAUTH) {
		/* DAUTH: Section 7, Region 7,8,9 : Rows 28-39 */
		wr_lock_flag = ((1<<7) | (1<<8) | (1<<9));
		/* Lock the programmed row in all following boot cycle */
		/* Bits to lock the
		 * Region 7 - > Bits 15:14
		 * Region 8 - > Bits 17:16
		 * Region 9 - > Bits 19:18
		 * These bits fall in the ROW8
		 * Value = 0xFC000
		 */
		prg_lock_flag = 0xFC000;

		return sotp_set_key_helper(key, keySize, 28, 39,
					wr_lock_flag, prg_lock_flag);

	} else if (type == OTPKey_HMAC_SHA256) {
		/* KHMAC : Section 8, Region 10,11,12 : Rows 40-51 */
		wr_lock_flag = ((1<<10) | (1<<11) | (1<<12));
		/* Lock the programmed row in all following boot cycle */
		/* Bits to lock the
		 * Region 10 - > Bits 21:20
		 * Region 11 - > Bits 23:22
		 * Region 12 - > Bits 25:24
		 * These bits fall in the ROW8
		 * Value = 0x3F00000
		 */
		prg_lock_flag = 0x3F00000;

		return sotp_set_key_helper(key, keySize, 40, 51,
					wr_lock_flag, prg_lock_flag);

	} else if (type == OTPKey_AES) {
		/* KAES : Section 9, Region 13,14,15 : Rows 52-63 */
		wr_lock_flag = ((1<<7) | (1<<8) | (1<<9));
		/* Lock the programmed row in all following boot cycle */
		/* Bits to lock the
		 * Region 13 - > Bits 27:26
		 * Region 14 - > Bits 29:28
		 * Region 15 - > Bits 31:30
		 * These bits fall in the ROW8
		 * Value = 0xFC000000
		 */
		prg_lock_flag = 0xFC000000;

		return sotp_set_key_helper(key, keySize, 52, 63,
					wr_lock_flag, prg_lock_flag);

	} else if (type == OTPKey_ecDSA) {
		/* ECDSA : Section 10, Region 16,17,18 : Rows 64-75 */
		wr_lock_flag = ((1<<18) | (1<<17) | (1<<16));
		/* Lock the programmed row in all following boot cycle */
		/* Bits to lock the
		 * Region 16 - > Bits 33:32
		 * Region 17- > Bits 35:34
		 * Region 18 - > Bits 37:36
		 * These bits fall in the ROW8
		 * Value = 0x3F00000000
		 */
		prg_lock_flag = 0x3F00000000;

		return sotp_set_key_helper(key, keySize, 64, 75,
					wr_lock_flag, prg_lock_flag);
	}
	return IPROC_OTP_INVALID;
}

/** This function reads Bits 20 & 21 of Row 15 of Chip OTP.
 * which are bits 244 & 245 of chip OTP. 244 & 245 are bit
 * offsets starting from the beginning of the chip config
 * section of the chip OTP, which itself is at the offset 256 of
 * OTP. So, Row Number = (244 + 256) / 32 = 15, and bit offset =
 * (244 + 256) % 32 = 20. Note that Chip OTP is different from
 * SOTP.
 */
u32_t read_crmu_otp_max_vco(void)
{
	u32_t max_vco;
	u32_t data = 0;

	sys_write32(0x1, CRMU_CHIP_OTPC_RST_CNTRL);

	sys_write32((sys_read32(OTPC_MODE_REG) | 0x1), OTPC_MODE_REG);

	/* Enable OTPC Programming */
	data = sys_read32(OTPC_CNTRL_0);
	data |= (1 << OTPC_CNTRL_0__OTP_PROG_EN);
	sys_write32(data, OTPC_CNTRL_0);

	/* COMMAND - 0x0 */
	data &= ~(0x1F << OTPC_CNTRL_0__COMMAND_R);
	sys_write32(data, OTPC_CNTRL_0);

	/*post_log("After update OTPC_CNTRL_0: 0x%04X\n",data); */
	sys_write32(OTP_MAX_VCO_ROW << 5, OTPC_CPUADDR_REG);

	/* Set the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data |= (1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	while ((sys_read32(OTPC_CPU_STATUS) & 0x1) != 0x1) {
		;
	}
	data = sys_read32(OTPC_CPU_DATA);
	max_vco = (data & (0x3 << OTP_MAX_VCO_START_BIT)) >> OTP_MAX_VCO_START_BIT;

	/* Reset the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data &= ~(1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	return max_vco;
}

/* This function reads Bit 17 o Row 15 of Chip OTP.
 * Note that Chip OTP is different from SOTP.
 */
u32_t read_bvc_from_chip_otp(void)
{
	u32_t otp_bvc;
	u32_t data = 0;

	sys_write32(0x1, CRMU_CHIP_OTPC_RST_CNTRL);

	sys_write32((sys_read32(OTPC_MODE_REG) | 0x1), OTPC_MODE_REG);

	/* Enable OTPC Programming */
	data = sys_read32(OTPC_CNTRL_0);
	data |= (1 << OTPC_CNTRL_0__OTP_PROG_EN);
	sys_write32(data, OTPC_CNTRL_0);

	/* COMMAND - 0x0 */
	data &= ~(0x1F << OTPC_CNTRL_0__COMMAND_R);
	sys_write32(data, OTPC_CNTRL_0);

	/*post_log("After update OTPC_CNTRL_0: 0x%04X\n",data); */
	sys_write32(OTP_BVC_ROW << 5, OTPC_CPUADDR_REG);

	/* Set the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data |= (1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	while ((sys_read32(OTPC_CPU_STATUS) & 0x1) != 0x1) {
		;
	}
	data = sys_read32(OTPC_CPU_DATA);
	otp_bvc = (data & (0x1 << OTP_BVC_BIT)) >> OTP_BVC_BIT;

	/* Reset the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data &= ~(1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	return otp_bvc;
}

u32_t sotp_read_chip_id(void)
{
	s32_t chip_rev = 0;
	/* 8th Dword has CRMU_OTP_DEVICE_ID and CRMU_OTP_REVISION _ID */
	s32_t l = 8;
	u32_t data = 0;
	u32_t brcm_chip_id = 0;

	sys_write32((sys_read32(OTPC_MODE_REG) | 0x1), OTPC_MODE_REG);
	data = sys_read32(OTPC_CNTRL_0);

	/* post_log("Before update OTPC_CNTRL_0: 0x%04X\n",data); */
	/* Enable OTPC Programming */
	data |= (1 << OTPC_CNTRL_0__OTP_PROG_EN);
	sys_write32(data, OTPC_CNTRL_0);

	/* COMMAND - 0x0 */
	data &= ~(0x1F << OTPC_CNTRL_0__COMMAND_R);
	sys_write32(data, OTPC_CNTRL_0);

	/*post_log("After update OTPC_CNTRL_0: 0x%04X\n",data); */
	sys_write32(l << 5, OTPC_CPUADDR_REG);

	/* Set the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data |= (1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	while ((sys_read32(OTPC_CPU_STATUS) & 0x1) != 0x1) {
		;
	}
	data = sys_read32(OTPC_CPU_DATA);
	chip_rev = (data & 0xFF);

	/* Bits 8 - 23 is the Chip Id.
	 * Chip Id for Lynx is 0x5810x which is 20 bits
	 * 58 is  represented as 5=8 = 13, 0xD. So OTPed Chip_Id is only
	 * 16 bits
	 */
	brcm_chip_id = ((data >> 8) & 0xFFFF);
	brcm_chip_id = ((((brcm_chip_id >> 12) & 0xF) == 0xD) ?
			((brcm_chip_id & 0xFFF) | 0x58000):0x0);

	/* Reset the start bit */
	data = sys_read32(OTPC_CNTRL_0);
	data &= ~(1 << OTPC_CNTRL_0__START);
	sys_write32(data, OTPC_CNTRL_0);

	return (brcm_chip_id << 8) | chip_rev;
}

u32_t calc_crc32(u32_t initval, u8_t *charArr, u32_t arraySize)
{
	u32_t cval = initval;
	u8_t data, charVal;
	int ijk, j;

	for (ijk = 0; ijk < arraySize/4; ijk++) {
		for (j = 3; j >= 0; j--) {
			charVal = charArr[4*ijk+j];
			charVal = reflect(charVal, 8);
			data = charVal ^ (cval >> 24);
			cval = ctable[data] ^ (cval << 8);
		}
	}
	cval = reflect(cval, 32);
	cval = cval ^ 0xFFFFFFFF;
	return cval;
}

/* ***************************************************************************
 * Private Functions
 * ****************************************************************************/
/**
 * FIXME this function logic to be documented
 */
static u64_t sotp_fuse_read(u32_t addr, u32_t fn, u32_t regc_sel,
				u32_t tm_r, u32_t vsel_r)
{
	u64_t read_data   = 0;
	u64_t read_data1  = 0;
	u64_t read_data2  = 0;
#ifdef SOTP_DEBUG
	u32_t status0, status1;
#endif

	/* Check for FDONE status */
	while ((sys_read32(SOTP_REGS_OTP_STATUS_0) &
	       (1 << SOTP_REGS_OTP_STATUS_0__FDONE)) !=
	       (1 << SOTP_REGS_OTP_STATUS_0__FDONE)) {
		;
	}
	/*  Enable OTP acces by CPU */
	sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);
	sys_set_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_DISABLE_ECC);

	/* 10 bit row address */
	sys_write32(((addr & 0x3ff) <<
			SOTP_REGS_OTP_ADDR__OTP_ROW_ADDR_R),
			SOTP_REGS_OTP_ADDR);
	sys_write32(((SOTP_READ << SOTP_REGS_OTP_CTRL_0__OTP_CMD_R) |
			(1 << SOTP_REGS_OTP_CTRL_0__OTP_DEBUG_MODE) |
			(regc_sel << SOTP_REGS_OTP_CTRL_0__WRP_REGC_SEL_R)),
			SOTP_REGS_OTP_CTRL_0);
	sys_write32(((fn << SOTP_REGS_OTP_CTRL_1__WRP_FUSELSEL0) |
			(tm_r << SOTP_REGS_OTP_CTRL_1__WRP_TM_R) |
			(vsel_r << SOTP_REGS_OTP_CTRL_1__WRP_VSEL_R)),
			SOTP_REGS_OTP_CTRL_1);

	/* Start bit to tell SOTP to send command to the OTP controller */
	sys_set_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

	/* Wait for SOTP command done to be set */
	while ((sys_read32(SOTP_REGS_OTP_STATUS_1) &
	       (1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) !=
	       (1 << SOTP_REGS_OTP_STATUS_1__CMD_DONE)) {
		;
	}

#ifdef SOTP_DEBUG
	SYS_LOG_WRN("CMD Done\n");
#endif
	/* Clr Start bit after command done */
	sys_clear_bit(SOTP_REGS_OTP_CTRL_0, SOTP_REGS_OTP_CTRL_0__START);

#ifdef SOTP_DEBUG
	/* AB501 simple error checking using SOTP_REGS_OTP_STATUS_0 and 1 */
	status0 = sys_read32(SOTP_REGS_OTP_STATUS_0);
	status1 = sys_read32(SOTP_REGS_OTP_STATUS_1);
#endif

	read_data1 =  (u64_t) sys_read32(SOTP_REGS_OTP_RDDATA_0);
	read_data1 = read_data1 & 0xFFFFFFFF;
	read_data2 =  (u64_t) sys_read32(SOTP_REGS_OTP_RDDATA_1);
	read_data2 = (read_data2&0x1ff)<<32;
	read_data  =  read_data2 | read_data1;

	/* Command done is cleared */
	sys_set_bit(SOTP_REGS_OTP_STATUS_1,
				SOTP_REGS_OTP_STATUS_1__CMD_DONE);
	sys_clear_bit(SOTP_REGS_OTP_CTRL_0,
				SOTP_REGS_OTP_CTRL_0__OTP_DEBUG_MODE);
	/* disable OTP acces by CPU */
	sys_clear_bit(SOTP_REGS_OTP_PROG_CONTROL,
				SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN);

#ifdef SOTP_DEBUG
	/* AB501 print only if we see something different than expected*/
	if ((status0&(~0x108f)) || (status1&(~0x3ff00002))) {
		SYS_LOG_WRN("SOTP: verify read: row=%d ", addr);
		SYS_LOG_WRN("status0=0x%08x status1=0x%08x\n",
			    status0, status1);
	}
#endif /* SOTP_DEBUG */

	return read_data;
}
/**
 *  Calculate Error Correcting Code
 */
static u32_t calc_ecc(u32_t data)
{
	u32_t ecc = 0;

	ecc |=   (data & 0x1) ^
			((data >> 1) & 0x1) ^
			((data >> 2) & 0x1) ^
			((data >> 3) & 0x1) ^
			((data >> 4) & 0x1) ^
			((data >> 5) & 0x1) ^
			((data >> 6) & 0x1) ^
			((data >> 7) & 0x1) ^
			((data >> 14) & 0x1) ^
			((data >> 19) & 0x1) ^
			((data >> 22) & 0x1) ^
			((data >> 24) & 0x1) ^
			((data >> 30) & 0x1) ^
			((data >> 31) & 0x1);

	ecc |= (((data >> 4) & 0x1) ^
			((data >> 7) & 0x1) ^
			((data >> 8) & 0x1) ^
			((data >> 9) & 0x1) ^
			((data >> 10) & 0x1) ^
			((data >> 11) & 0x1) ^
			((data >> 12) & 0x1) ^
			((data >> 13) & 0x1) ^
			((data >> 14) & 0x1) ^
			((data >> 15) & 0x1) ^
			((data >> 18) & 0x1) ^
			((data >> 21) & 0x1) ^
			((data >> 24) & 0x1) ^
			((data >> 29) & 0x1)) << 1;

	ecc |= (((data >> 3) & 0x1) ^
			((data >> 11) & 0x1) ^
			((data >> 16) & 0x1) ^
			((data >> 17) & 0x1) ^
			((data >> 18) & 0x1) ^
			((data >> 19) & 0x1) ^
			((data >> 20) & 0x1) ^
			((data >> 21) & 0x1) ^
			((data >> 22) & 0x1) ^
			((data >> 23) & 0x1) ^
			((data >> 26) & 0x1) ^
			((data >> 27) & 0x1) ^
			((data >> 29) & 0x1) ^
			((data >> 30) & 0x1)) << 2;

	ecc |= (((data >> 2) & 0x1) ^
			((data >> 6) & 0x1) ^
			((data >> 10) & 0x1) ^
			((data >> 13) & 0x1) ^
			((data >> 15) & 0x1) ^
			((data >> 16) & 0x1) ^
			((data >> 24) & 0x1) ^
			((data >> 25) & 0x1) ^
			((data >> 26) & 0x1) ^
			((data >> 27) & 0x1) ^
			((data >> 28) & 0x1) ^
			((data >> 29) & 0x1) ^
			((data >> 30) & 0x1) ^
			((data >> 31) & 0x1)) << 3;

	ecc |= (((data >> 1) & 0x1) ^
			((data >> 2) & 0x1) ^
			((data >> 5) & 0x1) ^
			((data >> 7) & 0x1) ^
			((data >> 9) & 0x1) ^
			((data >> 12) & 0x1) ^
			((data >> 15) & 0x1) ^
			((data >> 20) & 0x1) ^
			((data >> 21) & 0x1) ^
			((data >> 22) & 0x1) ^
			((data >> 23) & 0x1) ^
			((data >> 25) & 0x1) ^
			((data >> 26) & 0x1) ^
			((data >>  28) & 0x1)) << 4;

	ecc |= ((data & 0x1) ^
			((data >> 5) & 0x1) ^
			((data >> 6) & 0x1) ^
			((data >> 8) & 0x1) ^
			((data >> 12) & 0x1) ^
			((data >> 13) & 0x1) ^
			((data >> 14) & 0x1) ^
			((data >> 16) & 0x1) ^
			((data >> 17) & 0x1) ^
			((data >> 18) & 0x1) ^
			((data >> 19) & 0x1) ^
			((data >> 20) & 0x1) ^
			((data >> 28) & 0x1)) << 5;

	ecc |=  ((data & 0x1) ^
			((data >> 1) & 0x1) ^
			((data >> 3) & 0x1) ^
			((data >> 4) & 0x1) ^
			((data >> 8) & 0x1) ^
			((data >> 9) & 0x1) ^
			((data >> 10) & 0x1) ^
			((data >> 11) & 0x1) ^
			((data >> 17) & 0x1) ^
			((data >> 23) & 0x1) ^
			((data >> 25) & 0x1) ^
			((data >> 27) & 0x1) ^
			((data >> 31) & 0x1)) << 6;

	return ecc;
}
/**
 *
 Function Description:
 Reflects the input data about it's centre bit
 *********************************************
 *
 */
static u32_t reflect(u32_t data, u8_t nbits)
{
	u32_t reflection;
	int i;

	reflection = 0;
	for (i = 0; i < nbits; i++) {
		/* If the LSB bit is set, set the reflection of it. */
		if (data & 0x01)
			reflection |= (1 << ((nbits - 1) - i));
		data = (data >> 1);
	}
	return reflection;
}

static u32_t sotp_redundancy_reduct(u32_t sotp_row_data)
{
	u32_t opt_data;
	u32_t opt_loop;
	u32_t temp_data;

	opt_data = 0;
	for (opt_loop = 0; opt_loop < 16; opt_loop = opt_loop + 1) {
		temp_data =  ((sotp_row_data  >> (opt_loop*2)) & 0x3);
		if (temp_data !=  0x0)
			opt_data = (opt_data | (1 << opt_loop));
	}
	return opt_data;
}

static u32_t bit_count(u64_t v)
{
	u32_t i;
	u32_t count = 0;

	for (i = 0; i < 64; i++) {
		if (v & (1LL << i))
			count++;
	}

	return count;
}

sotp_bcm58202_status_t sotp_read_key_helper(u8_t *key, u16_t *keySize,
					u32_t row_start, u32_t row_end)
{
	u32_t i, status = 0;
	u32_t status2 = 0xFFFFFFFF;
	u64_t row_data;
	u32_t row;
	u32_t *temp_key = (u32_t *)key;

	*keySize = 32;
	row = row_start;
	i = 0;
	while ((i < *keySize/4) && (row <= row_end)) {
		row_data = sotp_mem_read(row, 1);

		if (!(row_data & SOTP_ECC_ERR_DETECT) &&
			!(row_data & 0x18000000000)) {
			*temp_key = row_data & 0xFFFFFFFF;
			status |= *temp_key;
			status2 &= *temp_key++;
			i++;
		}
		row++;
	}
	if (status2 == 0xFFFFFFFF)
		return IPROC_OTP_ALLONE;
	else if ((status != 0x0) && (row <= row_end))
		return IPROC_OTP_VALID;
	return IPROC_OTP_INVALID;
}

static sotp_bcm58202_status_t sotp_set_key_helper(u8_t *key, u16_t keySize,
					u32_t row_start, u32_t row_end,
					u32_t wr_lock_flag, u64_t prg_lock_flag)
{
	u32_t i = 0;
	u64_t row_data;
	u32_t row, wr_lock;
	u32_t *temp_key = (u32_t *) key;
	u32_t crc_array[8];
	u32_t *data, crc32, row_data_32;
	u64_t write_result, crc_write_result;
	u32_t fail_count = 0;

	row = row_start;
	i = 0;
	data = (u32_t *) crc_array;
	while ((i < keySize / 4) && (row < row_end)) {
		write_result = sotp_mem_write(row, 1, (uint64_t)(*temp_key));

		/* If there are any failed bits, including ECC then fail
		 * the row,
		 * so mark the row as failed and try and use the next row.
		 */
		if (write_result != 0) {
			write_result = sotp_mem_write(row, 0, 0x18000000000LL);
			if (bit_count(write_result & 0x18000000000LL) > 1)
				return IPROC_OTP_INVALID;

			if (++fail_count > 3)
				return IPROC_OTP_INVALID;
		} else {
			i++;
			row_data_32 = *temp_key & 0xFFFFFFFF;
			*(u32_t *)data++ = row_data_32;
			temp_key++;
		}
		row++;
	}

	if (i == keySize / 4) {
		crc32 = calc_crc32(0xFFFFFFFF, (u8_t *)crc_array, keySize);

		write_result = 0L;
		crc_write_result = 0L;

		do {
			crc_write_result = sotp_mem_write(row, 1,
					(uint64_t)crc32);

			if (crc_write_result != 0) {
				write_result = sotp_mem_write(row, 0,
						0x18000000000LL);
				if (bit_count(write_result &
						0x18000000000LL) > 1)
					return IPROC_OTP_INVALID;

				if (++fail_count > 3)
					return IPROC_OTP_INVALID;
				row++;
			}
		} while (crc_write_result != 0L && row <= row_end);

	}

	wr_lock = sys_read32(SOTP_REGS_OTP_WR_LOCK);
	wr_lock = wr_lock | wr_lock_flag;
	sys_write32(wr_lock, SOTP_REGS_OTP_WR_LOCK);

	/* Lock the programmed row in all following boot cycle */
	row_data = sotp_mem_read(8, 0);
	sotp_mem_write(8, 0, row_data | prg_lock_flag);

	if (i < keySize / 4)
		return IPROC_OTP_INVALID;
	else
		return IPROC_OTP_VALID;

}
