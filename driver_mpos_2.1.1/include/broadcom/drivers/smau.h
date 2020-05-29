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

/* @file smau.h
 *
 *
 * This file contains
 *
 */

/**
 * TODO
 * 1.
 */
#ifndef __SMAU_H__
#define __SMAU_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <misc/__assert.h>

/* SMAU register offsets */
#define SMU_CFG_0			(SMAU_BASE_ADDR + 0x00)
#define SMU_CFG_1			(SMAU_BASE_ADDR + 0x04)
#define SMU_CONTROL			(SMAU_BASE_ADDR + 0x08)
#define SMU_STATUS			(SMAU_BASE_ADDR + 0x0c)
#define SMU_INT_STATUS			(SMAU_BASE_ADDR + 0x10)
#define SMU_INT_MASK			(SMAU_BASE_ADDR + 0x14)
#define SMU_INT_CLR			(SMAU_BASE_ADDR + 0x18)
#define SMU_LSFR_SEED			(SMAU_BASE_ADDR + 0x1c)
#define SMU_DMA_CONTROL			(SMAU_BASE_ADDR + 0x20)
#define SMU_DMA_IN			(SMAU_BASE_ADDR + 0x24)
#define SMU_DMA_OUT			(SMAU_BASE_ADDR + 0x28)
#define SMU_PAGE_LOCK_0			(SMAU_BASE_ADDR + 0x2c)
#define SMU_PAGE_LOCK_1			(SMAU_BASE_ADDR + 0x30)
#define SMU_PAGE_LOCK_2			(SMAU_BASE_ADDR + 0x34)
#define SMU_PAGE_LOCK_3			(SMAU_BASE_ADDR + 0x38)
#define SMU_HMAC_BASE_0			(SMAU_BASE_ADDR + 0x3c)
#define SMU_HMAC_BASE_1			(SMAU_BASE_ADDR + 0x40)
#define SMU_HMAC_BASE_2			(SMAU_BASE_ADDR + 0x44)
#define SMU_HMAC_BASE_3			(SMAU_BASE_ADDR + 0x48)
#define SMU_WIN_BASE_0			(SMAU_BASE_ADDR + 0x4c)
#define SMU_WIN_BASE_1			(SMAU_BASE_ADDR + 0x50)
#define SMU_WIN_BASE_2			(SMAU_BASE_ADDR + 0x54)
#define SMU_WIN_BASE_3			(SMAU_BASE_ADDR + 0x58)
#define SMU_WIN_SIZE_0			(SMAU_BASE_ADDR + 0x5c)
#define SMU_WIN_SIZE_1			(SMAU_BASE_ADDR + 0x60)
#define SMU_WIN_SIZE_2			(SMAU_BASE_ADDR + 0x64)
#define SMU_WIN_SIZE_3			(SMAU_BASE_ADDR + 0x68)
#define SMU_STAT_DR_TOT			(SMAU_BASE_ADDR + 0xac)
#define SMU_STAT_DW_TOT			(SMAU_BASE_ADDR + 0xb0)
#define SMU_STAT_DR_MISS		(SMAU_BASE_ADDR + 0xb4)
#define SMU_STAT_DW_MISS		(SMAU_BASE_ADDR + 0xb8)
#define SMU_SPI_CFG			(SMAU_BASE_ADDR + 0xbc)
#define SMU_SPI_CS_CNT			(SMAU_BASE_ADDR + 0xc0)
#define SMU_CAM_BIST_CONTROL		(SMAU_BASE_ADDR + 0xd0)
#define SMU_CAM_BIST_DBG_CONTROL	(SMAU_BASE_ADDR + 0xd4)
#define SMU_CAM_BIST_DBG_SEL		(SMAU_BASE_ADDR + 0xd8)
#define SMU_CAM_BIST_DBG_DATA		(SMAU_BASE_ADDR + 0xdc)
#define SMU_CAM_BIST_STATUS		(SMAU_BASE_ADDR + 0xe0)
#define SMU_CAM_TM			(SMAU_BASE_ADDR + 0xe4)
#define SMU_CFG_1__DMA_SOFT_RST		2
#define SMU_CFG_1__CACHE_SOFT_RST	1
#define SMU_CFG_1__CACHE_ENABLE		0

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/
/**
 *
 */
#define SMU_CFG0_AES_KEY_LEN_128	0x00020000

/**
 * Total Number of SMAU Windows
 */
#define SMU_CFG_NUM_WINDOWS		4

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/
/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

typedef int (*smau_api_pma_iv_fp)(struct device *dev, u32_t addr, u8_t *iv);

typedef int (*smau_cache_fp)(struct device *dev);

typedef int (*smau_config_windows_fp)(struct device *dev,
		s32_t win, u32_t base, u32_t size,
		u32_t hmac_base, s32_t spi_mapped, s32_t auth_bypass,
		s32_t enc_bypass, s32_t common_params, s32_t swap);

typedef int  (*smau_config_keys_fp)(struct device *dev,
		s32_t win,  u32_t key_size, u32_t *key);

typedef int (*smau_get_hmac_base_fp)(struct device *dev,
		u32_t size, u32_t win_mask);

typedef int (*smau_mem_init_extra_fp)(struct device *dev);

struct smau_driver_api {
	smau_api_pma_iv_fp get_pma_iv;
	smau_cache_fp disable_cache;
	smau_cache_fp enable_cache;
	smau_config_windows_fp config_windows;
	smau_config_keys_fp config_aeskeys;
	smau_config_keys_fp config_authkeys;
	smau_get_hmac_base_fp get_hmac_base;
	smau_mem_init_extra_fp mem_init_extra;
};

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/

/**
 * @brief Get smau IV
 * @details	Get smau IV according to AES block physical address.
 *		The IV is a bit-permutated version of the 32-bit
 *		Physical Memory Address (PMA), replicated four times
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in]   addr	PMA : physical memory address
 * @param[out]	iv	pointer to initialization vector result
 * @retval 0 if successful.
 */
static inline int smau_get_pma_iv(struct device *dev,
					u32_t addr, u8_t *iv)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->get_pma_iv(dev, addr, iv);
}

/**
 * @brief smau disable cache
 * @details smau disable secure cache
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @retval 0 if successful.
 */
static inline int smau_disable_cache(struct device *dev)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->disable_cache(dev);
}

/**
 * @brief smau enable cache
 * @details smau enable secure cache
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @retval 0 if successful.
 */
static inline int smau_enable_cache(struct device *dev)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->enable_cache(dev);
}

/**
 * @brief smau parameter configure
 * @details This Apis is used to configure
 *		the memory addresses that serve as sources
 *		for the cleartext windows,
 *		the size of each cleartext window,
 *		and memory pointers to the pair of keys (AES/HMAC) associated
 *		with each window
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] win	cleartext window number
 * @param[in] base	physical memory to map to cleartext
 * @param[in] size	size of the physical memory to map
 * @param[in] hmac_base	pointer to HMAC
 * @param[in] spi_mapped mapping SPI memory (readonly)
 * @param[in] auth_bypass bypass authentication
 * @param[in] enc_bypass bypass encryption/decryption
 * @param[in] common_params fields to set within cfg0 register
 * @param[in] swap	perform endianness swapping (cfg1)
 * @retval 0 if successful.
 */
static inline int smau_config_windows(struct device *dev,
		s32_t win, u32_t base, u32_t size,
		u32_t hmac_base, s32_t spi_mapped, s32_t auth_bypass,
		s32_t enc_bypass, s32_t common_params, s32_t swap)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->config_windows(dev, win, base, size,
			hmac_base, spi_mapped, auth_bypass,
			enc_bypass, common_params, swap);
}

/**
 * @brief Configure the aes keys
 * @details This Api configures the aes keys for encryption/decryption for
 *		the required window.
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] win	cleartext window number
 * @param[in] key_size	AES key size
 * @param[in] key	address of key in physical memory
 * @retval 0 if successful.
 */
static inline int smau_config_aeskeys(struct device *dev,
		s32_t win,  u32_t key_size, u32_t *key)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->config_aeskeys(dev, win, key_size, key);
}

/**
 * @brief Configure the auth keys
 * @details This Api configures the authentication keys for the required window
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] win	cleartext window number
 * @param[in] key_size	AES key size
 * @param[in] key	address of key in physical memory
 * @retval 0 if successful.
 */
static inline int smau_config_authkeys(struct device *dev,
		s32_t win,  u32_t key_size, u32_t *key)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->config_authkeys(dev, win, key_size, key);
}

/**
 * @brief Get the hmac base
 * @details This Api is used to get the base address to calculate the hmac over
 *		a perticular window. FIXME details of the algorithms to be
 *		documented
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] size size of the window
 * @param[in] win_mask window mask
 * @retval 0 if successful.
 */
static inline int smau_get_hmacbase(struct device *dev,
		u32_t size, u32_t win_mask)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->get_hmac_base(dev, size, win_mask);
}

/**
 * @brief Memory map initialization
 * @details This api initializes the different memory map used for the smau
 *		parameters like aes keys, hmac keys.
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @retval 0 if successful.
 */
static inline int smau_mem_init_extra(struct device *dev)
{
	const struct smau_driver_api *api = dev->driver_api;

	return api->mem_init_extra(dev);
}

#endif /* __SMAU_H__ */
