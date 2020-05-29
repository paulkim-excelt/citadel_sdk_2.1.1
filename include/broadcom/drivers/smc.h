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

/* @file smc.h
 *
 * @brief Public APIs for the smc driver.
 */

#ifndef _SMC_H_
#define _SMC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* memc_status */
#define SMC_RAW_INT_STATUS1_BIT			(6)
#define SMC_RAW_INT_STATUS0_BIT			(5)
#define SMC_INT_STATUS1_BIT			(4)
#define SMC_INT_STATUS0_BIT			(3)
#define SMC_INT_EN1_BIT				(2)
#define SMC_INT_EN0_BIT				(1)
#define SMC_STATE_BIT				(0)

/* memif_cfg */
#define SMC_EXCLUSIVE_MONITORS_MASK		(0x03)
#define SMC_EXCLUSIVE_MONITORS_SHIFT		(16)
#define SMC_REMAP1_BIT				(14)
#define SMC_MEMORY1_BYTES_MASK			(0x03)
#define SMC_MEMORY1_BYTES_SHIFT			(12)
#define SMC_MEMORY1_CHIPS_MASK			(0x03)
#define SMC_MEMORY1_CHIPS_SHIFT			(10)
#define SMC_MEMORY1_TYPE_MASK			(0x03)
#define SMC_MEMORY1_TYPE_SHIFT			(8)
#define SMC_REMAP0_BIT				(14)
#define SMC_MEMORY0_BYTES_MASK			(0x03)
#define SMC_MEMORY0_BYTES_SHIFT			(4)
#define SMC_MEMORY0_CHIPS_MASK			(0x03)
#define SMC_MEMORY0_CHIPS_SHIFT			(2)
#define SMC_MEMORY0_TYPE_MASK			(0x03)
#define SMC_MEMORY0_TYPE_SHIFT			(0)

/* mem_cfg_set */
#define SMC_LOW_POWER_REQ_BIT			(2)
#define SMC_INT_ENABLE1_BIT			(1)
#define SMC_INT_ENABLE0_BIT			(0)

/* mem_cfg_clr */
#define SMC_INT_CLEAR1_BIT			(4)
#define SMC_INT_CLEAR0_BIT			(3)
#define SMC_LOW_POWER_EXIT_BIT			(2)
#define SMC_INT_DISABLE1_BIT			(1)
#define SMC_INT_DISABLE0_BIT			(0)

/* direct_cmd */
#define SMC_CHIP_NUMBER_MASK			(0x03)
#define SMC_CHIP_NUMBER_SHIFT			(23)
#define SMC_CMD_TYPE_MASK			(0x03)
#define SMC_CMD_TYPE_SHIFT			(21)
#define SMC_SET_CRE_BIT				(20)
#define SMC_ADDR_19_TO_0_MASK			(0xfffff)

/* set_cycles */
#define SMC_SET_CYCLES_T6_MASK			(0x0f)
#define SMC_SET_CYCLES_T6_SHIFT			(20)
#define SMC_SET_CYCLES_T5_MASK			(0x07)
#define SMC_SET_CYCLES_T5_SHIFT			(17)
#define SMC_SET_CYCLES_T4_MASK			(0x07)
#define SMC_SET_CYCLES_T4_SHIFT			(14)
#define SMC_SET_CYCLES_T3_MASK			(0x07)
#define SMC_SET_CYCLES_T3_SHIFT			(11)
#define SMC_SET_CYCLES_T2_MASK			(0x07)
#define SMC_SET_CYCLES_T2_SHIFT			(8)
#define SMC_SET_CYCLES_T1_MASK			(0x0f)
#define SMC_SET_CYCLES_T1_SHIFT			(4)
#define SMC_SET_CYCLES_T0_MASK			(0x0f)

/* set_opmode */
#define SMC_BURST_ALIGN_MASK			(0x03)
#define SMC_BURST_ALIGN_SHIFT			(13)
#define SMC_SET_BLS_TIME_BIT			(12)
#define SMC_ADV_BIT				(11)
#define SMC_BAA_BIT				(10)
#define SMC_SET_WR_BL_MASK			(0x03)
#define SMC_SET_WR_BL_SHIFT			(7)
#define SMC_WR_SYNC_BIT				(6)
#define SMC_SET_RD_BL_MASK			(0x03)
#define SMC_SET_RD_BL_SHIFT			(3)
#define SMC_RD_SYNC_BIT				(2)
#define SMC_SET_MW_MASK				(0x03)

/* refresh_0 */
#define SMC_REF_PERIOD_MASK			(0x0f)

/* sram_cycles0_0 */
#define SMC_WE_TIME0_BIT			(20)
#define SMC_T_TR0_MASK				(0x07)
#define SMC_T_TR0_SHIFT				(17)
#define SMC_T_PC0_MASK				(0x07)
#define SMC_T_PC0_SHIFT				(14)
#define SMC_T_WP0_MASK				(0x07)
#define SMC_T_WP0_SHIFT				(11)
#define SMC_T_CEOE0_MASK			(0x07)
#define SMC_T_CEOE0_SHIFT			(8)
#define SMC_T_WC0_MASK				(0x0f)
#define SMC_T_WC0_SHIFT				(4)
#define SMC_T_RC0_MASK				(0x0f)
#define SMC_T_RC0_SHIFT				(0)

/* opmode0_0 */
#define SMC_ADDR_MATCH0_MASK			(0xff)
#define SMC_ADDR_MATCH0_SHIFT			(24)
#define SMC_ADDR_MASK0_MASK			(0xff)
#define SMC_ADDR_MASK0_SHIFT			(16)
#define SMC_BURST_ALIGN0_MASK			(0x07)
#define SMC_BURST_ALIGN0_SHIFT			(13)
#define SMC_BLS_TIME0_BIT			(12)
#define SMC_ADV0_BIT				(11)
#define SMC_BAA0_BIT				(10)
#define SMC_WR_BL0_MASK				(0x07)
#define SMC_WR_BL0_SHIFT			(7)
#define SMC_WR_SYNC0_BIT			(6)
#define SMC_RD_BL0_MASK				(0x07)
#define SMC_RD_BL0_SHIFT			(3)
#define SMC_RD_SYNC0_BIT			(2)
#define SMC_MW0_MASK				(0x03)

/* sram_cycles0_1 */
#define SMC_WE_TIME1_BIT			(20)
#define SMC_T_TR1_MASK				(0x07)
#define SMC_T_TR1_SHIFT				(17)
#define SMC_T_PC1_MASK				(0x07)
#define SMC_T_PC1_SHIFT				(14)
#define SMC_T_WP1_MASK				(0x07)
#define SMC_T_WP1_SHIFT				(11)
#define SMC_T_CEOE1_MASK			(0x07)
#define SMC_T_CEOE1_SHIFT			(8)
#define SMC_T_WC1_MASK				(0x0f)
#define SMC_T_WC1_SHIFT				(4)
#define SMC_T_RC1_MASK				(0x0f)
#define SMC_T_RC1_SHIFT				(0)

/* opmode0_1 */
#define SMC_ADDR_MATCH1_MASK			(0xff)
#define SMC_ADDR_MATCH1_SHIFT			(24)
#define SMC_ADDR_MASK1_MASK			(0xff)
#define SMC_ADDR_MASK1_SHIFT			(16)
#define SMC_BURST_ALIGN1_MASK			(0x07)
#define SMC_BURST_ALIGN1_SHIFT			(13)
#define SMC_BLS_TIME1_BIT			(12)
#define SMC_ADV1_BIT				(11)
#define SMC_BAA1_BIT				(10)
#define SMC_WR_BL1_MASK				(0x07)
#define SMC_WR_BL1_SHIFT			(7)
#define SMC_WR_SYNC1_BIT			(6)
#define SMC_RD_BL1_MASK				(0x07)
#define SMC_RD_BL1_SHIFT			(3)
#define SMC_RD_SYNC1_BIT			(2)
#define SMC_MW1_MASK				(0x03)

/**
 * @brief SMC registers.
 *
 * Please refer the data sheet for detailed register description
 */
struct smc_regs {
	u32_t memc_status;
	u32_t memif_cfg;
	u32_t mem_cfg_set;
	u32_t mem_cfg_clr;
	u32_t direct_cmd;
	u32_t set_cycles;
	u32_t set_opmode;
	u32_t refresh_0;
	u32_t sram_cycles0_0;
	u32_t opmode0_0;
	u32_t sram_cycles0_1;
	u32_t opmode0_1;
	u32_t user_status;
	u32_t user_config;
	u32_t periph_id_0;
	u32_t periph_id_1;
	u32_t periph_id_2;
	u32_t periph_id_3;
	u32_t pcell_id_0;
	u32_t pcell_id_1;
	u32_t pcell_id_2;
	u32_t pcell_id_3;
};

/* smc registers enum*/
enum smc_regs_enum {
	SMC_MEMC_STATUS = offsetof(struct smc_regs, memc_status),
	SMC_MEMIF_CFG = offsetof(struct smc_regs, memif_cfg),
	SMC_MEM_CFG_SET = offsetof(struct smc_regs, mem_cfg_set),
	SMC_MEM_CFG_CLR = offsetof(struct smc_regs, mem_cfg_clr),
	SMC_DIRECT_CMD = offsetof(struct smc_regs, direct_cmd),
	SMC_SET_CYCLES = offsetof(struct smc_regs, set_cycles),
	SMC_SET_OPMODE = offsetof(struct smc_regs, set_opmode),
	SMC_REFRESH_0 = offsetof(struct smc_regs, refresh_0),
	SMC_SRAM_CYCLES0_0 = offsetof(struct smc_regs, sram_cycles0_0),
	SMC_OPMODE0_0 = offsetof(struct smc_regs, opmode0_0),
	SMC_USER_STATUS = offsetof(struct smc_regs, user_status),
	SMC_USER_CONFIG = offsetof(struct smc_regs, user_config),
	SMC_PERIPH_ID_0 = offsetof(struct smc_regs, periph_id_0),
	SMC_PERIPH_ID_1 = offsetof(struct smc_regs, periph_id_1),
	SMC_PERIPH_ID_2 = offsetof(struct smc_regs, periph_id_2),
	SMC_PERIPH_ID_3 = offsetof(struct smc_regs, periph_id_3),
	SMC_PCELL_ID_0 = offsetof(struct smc_regs, pcell_id_0),
	SMC_PCELL_ID_1 = offsetof(struct smc_regs, pcell_id_1),
	SMC_PCELL_ID_2 = offsetof(struct smc_regs, pcell_id_2),
	SMC_PCELL_ID_3 = offsetof(struct smc_regs, pcell_id_3),
};

typedef void (*smc_api_write)(struct device *dev, u32_t reg, u32_t data);

typedef u32_t (*smc_api_read)(struct device *dev, u32_t reg);

struct smc_driver_api {
	smc_api_write write;
	smc_api_read read;
};

/**
 * @brief Write SMC
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to write
 * @param data - Data to write
 *
 * @return None
 *
 * @note For register field, use macros provided in smc_regs_enum
 */
static inline void smc_write(struct device *dev, u32_t reg, u32_t data)
{
	const struct smc_driver_api *api = dev->driver_api;

	api->write(dev, reg, data);
}

/**
 * @brief Read SMC
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to read
 *
 * @return Data from the register
 *
 * @note For register field, use macros provided in smc_regs_enum
 */
static inline s32_t smc_read(struct device *dev, u32_t reg)
{
	const struct smc_driver_api *api = dev->driver_api;

	return api->read(dev, reg);
}

#ifdef __cplusplus
}
#endif

#endif /* _SMC_H_ */
