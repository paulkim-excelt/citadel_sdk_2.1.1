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

/* @file smau.c
 *
 * This file contains the smau driver API's for smau window configurations
 *
 *
 */

/*
 * TODO
 * 1. Check the magic numbers and replace them with #defines
 *
 *
 */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <zephyr/types.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <errno.h>
#include <toolchain.h>
#include <smau.h>
/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
#define SMAU_CACHE_LINES 128

#define SMAU_MMAP_CACHE		0x50000000
#define SMAU_MMAP_CAM		0x50020000
#define SMAU_MMAP_AES		0x50020200
#define SMAU_MMAP_AUTH		0x50020280
#define SMAU_MMAP_AES_PAR	0x50020300
#define SMAU_MMAP_AUTH_PAR	0x50020380
#define SMAU_MMAP_REGISTER	0x50020400
#define SMAU_MMAP_RESERVED	0x50020800
#define SMAU_MMAP_CACHE_PAR	0x50030000
#define SMAU_MMAP_CAM_PAR	0x50040000
#define SMAU_MMAP_RESERVED2	0x50040200

#define SMU_F_aes_key_length_MASK	0x00020000
#define SMU_F_cache_crypto_algo_MASK	0x00018000
#define SMU_F_page_replacement_MASK	0x00004000
#define SMU_F_smu_addr_chk_MASK		0x00002000
#define SMU_F_ctw_ro_chk_MASK		0x00001000
#define SMU_F_ctw_range_chk_MASK	0x00000800
#define SMU_F_auth_with_iv_MASK		0x00000400
#define SMU_F_auth_first_MASK		0x00000200
#define SMU_F_cache_ram_prot_MASK	0x00000100

/* Default SMAU window settings */
#define WIN_BASE_1	0x64000000
#define WIN_BASE_2	0x65000000
#define WIN_BASE_3	0x66000000
#define WIN_SIZE_DEF	0xFF000000
#define HMAC_BASE_1	0x64042000
#define HMAC_BASE_2	0x65044000
#define HMAC_BASE_3	0x66046000

#define TWOS_COMPLEMENT(A)	(~(A) + 1)
#define IS_POWER_OF_2(A)	(((A) & ((A) - 1)) == 0)

/*
 * The Access to flash through SMAU is through the SMAU cache.
 * And the SMAU apis disable and re-enable SMAU cache during their
 * operation, in order to program other SMAU regsters. But when
 * SMAU cache is enabled, there cannot be accesses to flash for a few cycles.
 * If the SMAU cache enabling was the last thing an API does, then the
 * instruction following it would be "bx lr", which could prefetch the
 * address in the link register. And the link register will have the caller's
 * address, which could potentially be located in the flash. This will
 * result in pre-fetch aborts, if sufficient wait instructions are not added
 * after the cache is enabled and the api returns.
 *
 * The number of cycles to wait before accessing flash needs to be provided
 * by the hardware team. As this is unavailable at this point, a simple delay
 * of 500 is used to implement this wait. This will be updated as per hardware
 * team's guidance once it is available.
 *
 */
#define SMAU_RETURN_WAIT() do {			\
		volatile int i;			\
		__asm__ volatile("dmb");	\
		__asm__ volatile("dsb");	\
		__asm__ volatile("isb");	\
		for (i = 0; i < 500; i++);	\
	} while (0)

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

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/
/**
 * The IV is a bit-permutated version of the 32-bit
 * Physical Memory Address (PMA), replicated four times
 * e.g.
 * IV[0], 4, 8 and 12 will have the bit map as follows
 * ----------------------------------------------
 * Physical Memory addr bits  : 28 20 12 4 30 22 14 6
 * corresponding IV[0] bits   : 07 06 05 4 03 02 01 0
 * ----------------------------------------------
 */
static int bcm58202_smau_get_pma_iv(struct device *dev,
				u32_t addr,
				u8_t *iv)
{
	 ARG_UNUSED(dev);
	iv[0] = iv[4] = iv[8] = iv[12] =
	(((addr >> 28) & 0x00000001) << 7) |
	(((addr >> 20) & 0x00000001) << 6) |
	(((addr >> 12) & 0x00000001) << 5) |
	(((addr >> 4) & 0x00000001) << 4) |
	(((addr >> 30) & 0x00000001) << 3) |
	(((addr >> 22) & 0x00000001) << 2) |
	(((addr >> 14) & 0x00000001) << 1) |
	 ((addr >> 6) & 0x00000001);

	iv[1] = iv[5] = iv[9] = iv[13] =
	(((addr >> 24) & 0x00000001) << 7) |
	(((addr >> 16) & 0x00000001) << 6) |
	(((addr >> 8) & 0x00000001) << 5) |
	(((addr >> 0) & 0x00000001) << 4) |
	(((addr >> 26) & 0x00000001) << 3) |
	(((addr >> 18) & 0x00000001) << 2) |
	(((addr >> 10) & 0x00000001) << 1) |
	 ((addr >> 2) & 0x00000001);

	iv[2] = iv[6] = iv[10] = iv[14] =
	(((addr >> 29) & 0x00000001) << 7) |
	(((addr >> 21) & 0x00000001) << 6) |
	(((addr >> 13) & 0x00000001) << 5) |
	(((addr >> 5) & 0x00000001) << 4) |
	(((addr >> 31) & 0x00000001) << 3) |
	(((addr >> 23) & 0x00000001) << 2) |
	(((addr >> 15) & 0x00000001) << 1) |
	 ((addr >> 7) & 0x00000001);

	iv[3] = iv[7] = iv[11] = iv[15] =
	(((addr >> 25) & 0x00000001) << 7) |
	(((addr >> 17) & 0x00000001) << 6) |
	(((addr >> 9) & 0x00000001) << 5) |
	(((addr >> 1) & 0x00000001) << 4) |
	(((addr >> 27) & 0x00000001) << 3) |
	(((addr >> 19) & 0x00000001) << 2) |
	(((addr >> 11) & 0x00000001) << 1) |
	 ((addr >> 3) & 0x00000001);

	return 0;
}

static int bcm58202_smau_disable_cache(struct device *dev)
{
	ARG_UNUSED(dev);
	sys_clear_bit(SMU_CFG_1, SMU_CFG_1__CACHE_ENABLE);
	return 0;
}

static int bcm58202_smau_enable_cache(struct device *dev)
{
	ARG_UNUSED(dev);
	sys_set_bit(SMU_CFG_1, SMU_CFG_1__CACHE_ENABLE);
	SMAU_RETURN_WAIT();
	return 0;
}
#include <errno.h>
#define TWOS_COMPLEMENT(A)	(~(A) + 1)
#define IS_POWER_OF_2(A)	(((A) & ((A) - 1)) == 0)

/**
 * FIXME, there are currently no status registers for the failure cases
 * like auth fail. Check this with HW people
 */
static int bcm58202_smau_config_windows(struct device *dev,
		s32_t win, u32_t base, u32_t size,
		u32_t hmac_base, s32_t spi_mapped, s32_t auth_bypass,
		s32_t enc_bypass, s32_t common_params, s32_t swap)
{
	ARG_UNUSED(dev);
	u32_t mask, win_size_bytes;

	/* size of zero */
	if (size == 0xffffffff)
		return -EINVAL;

	win_size_bytes = TWOS_COMPLEMENT(size);
	if (!IS_POWER_OF_2(win_size_bytes))
		return -EINVAL;

	/* Check the windows size is bigger than the minimum allowed size */
	if (win_size_bytes < CONFIG_MIN_SMAU_WINDOW_SIZE)
		return -EINVAL;

	/* Disable cache first  */
	sys_write32(0, SMU_CFG_1);

	sys_write32(base, (SMU_WIN_BASE_0 + (win * 4)));
	sys_write32(size, (SMU_WIN_SIZE_0 + (win * 4)));
	sys_write32(hmac_base, (SMU_HMAC_BASE_0 + (win * 4)));

	mask = 0x3 << (24 + (win * 2));
	mask |= 0x3 << (win * 2);

	/* clear all the common paramareters */
	mask |= (SMU_F_aes_key_length_MASK |
			SMU_F_cache_crypto_algo_MASK |
			SMU_F_page_replacement_MASK |
			SMU_F_smu_addr_chk_MASK |
			SMU_F_ctw_ro_chk_MASK |
			SMU_F_ctw_range_chk_MASK |
			SMU_F_auth_with_iv_MASK |
			SMU_F_auth_first_MASK |
			SMU_F_cache_ram_prot_MASK);

	sys_write32((sys_read32(SMU_CFG_0) & ~mask), SMU_CFG_0);

	mask = enc_bypass << (24 + (win * 2));
	mask |= auth_bypass << (25 + (win * 2));
	mask |= spi_mapped << (1 + (win * 2));

	/* what ever was the last configuration sticks */
	mask |= common_params;

	sys_write32((sys_read32(SMU_CFG_0) | mask), SMU_CFG_0);

	if (swap) {
		/* SWAP */
		sys_write32((sys_read32(SMU_CFG_1) | 0x701), SMU_CFG_1);
	} else {
		/* NO SWAP */
		sys_write32((sys_read32(SMU_CFG_1) | 0x1), SMU_CFG_1);
	}

	SMAU_RETURN_WAIT();

	return 0;
}

static int bcm58202_smau_config_aeskeys(struct device *dev,
		s32_t win, u32_t key_size, u32_t *key)
{
	ARG_UNUSED(dev);
	u32_t j = 0;
	u32_t addr = ((u32_t) SMAU_MMAP_AES + (win * 32));

	/* Set aes key data */
	for (j = 0; j < (key_size/4); j++) {
		sys_write32(*key, addr);
		addr += 4;
		key ++;
	}
	return 0;
}

static int bcm58202_smau_config_authkeys(struct device *dev,
		s32_t win, u32_t key_size, u32_t *key)
{
	ARG_UNUSED(dev);
	u32_t j = 0;
	u32_t addr = ((u32_t) SMAU_MMAP_AUTH + (win * 32));

	/* Set auth key data */
	for (j = 0; j < (key_size/4); j++) {
		sys_write32(*key, addr);
		addr += 4;
		key ++;
	}
	return 0;
}

static int bcm58202_smau_get_hmacbase(struct device *dev,
		u32_t size, u32_t win_mask)
{
	ARG_UNUSED(dev);
	u32_t base = 0;
	u32_t hmac_mask;
	u32_t min_hmac_base;

	hmac_mask = (win_mask >> 3) | 0xe0000000;
	min_hmac_base = ~hmac_mask + 1;

	if (min_hmac_base > size) {
		base = min_hmac_base;
	} else {
		base = size + (min_hmac_base - (size & ~hmac_mask));
		while (base < size)
			base += min_hmac_base;
	}

	return base;
}

static int bcm58202_smau_mem_init_extra(struct device *dev)
{
	ARG_UNUSED(dev);
	int i;

	/* Init Cam */
	for (i = 0; i < SMAU_CACHE_LINES; i++)
		sys_write32(0x00000000, (SMAU_MMAP_CAM + (i << 2)));

	/* Init AUTH mem */
	for (i = 0; i < 32; i++)
		sys_write32(0x00000000, (SMAU_MMAP_AUTH + (i << 2)));

	/* Init AES mem */
	for (i = 0; i < 32; i++)
		sys_write32(0x00000000, (SMAU_MMAP_AES + (i << 2)));

	return 0;
}

static struct smau_driver_api bcm58202_smau_control_api = {
	.get_pma_iv		= bcm58202_smau_get_pma_iv,
	.disable_cache		= bcm58202_smau_disable_cache,
	.enable_cache		= bcm58202_smau_enable_cache,
	.config_windows		= bcm58202_smau_config_windows,
	.config_aeskeys		= bcm58202_smau_config_aeskeys,
	.config_authkeys	= bcm58202_smau_config_authkeys,
	.get_hmac_base		= bcm58202_smau_get_hmacbase,
	.mem_init_extra		= bcm58202_smau_mem_init_extra,
};

static int bcm58202_smau_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_AND_API_INIT(bcm58202_smau, CONFIG_SMAU_NAME,
			bcm58202_smau_init, NULL, NULL, PRE_KERNEL_2,
			CONFIG_SMAU_DRIVER_INIT_PRIORITY,
			&bcm58202_smau_control_api);
