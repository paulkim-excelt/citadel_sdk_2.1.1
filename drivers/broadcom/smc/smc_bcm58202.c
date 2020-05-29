/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/*
 * @file smc_bcm58202.c
 * @brief bcm58202 smc driver implementation
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#ifdef CONFIG_MEM_LAYOUT_AVAILABLE
#include <arch/arm/soc/bcm58202/layout.h>
#endif
#include <smc.h>
#include <stdbool.h>
#include <sys_clock.h>
#include <zephyr/types.h>

#define SMC_MEMC_STATUS				(0x00)
#define SMC_MEMIF_CFG				(0x04)
#define SMC_MEM_CFG_SET				(0x08)
#define SMC_MEM_CFG_CLR				(0x0c)

#define SMC_DIRECT_CMD				(0x10)
#define SMC_DIRECT_CMD_CHIP_NMBR_MASK		(0x03)
#define SMC_DIRECT_CMD_CHIP_NMBR_SHIFT		(23)
#define SMC_DIRECT_CMD_CMD_TYPE_MASK		(0x03)
#define SMC_DIRECT_CMD_CMD_TYPE_SHIFT		(21)
#define SMC_DIRECT_CMD_SET_CRE_BIT		(20)
#define SMC_DIRECT_CMD_ADDR_MASK		(0xfffff)

#define SMC_SET_CYCLES				(0x14)
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
#define SMC_SET_CYCLES_T0_SHIFT			(0)

#define SMC_SET_OPMODE				(0x18)
#define SMC_SET_OPMODE_SET_BURST_ALIGN_MASK	(0x18)

#define SMC_TR_TURN_AROUND_TIME			(3)
#define SMC_PC_PAGE_CYCLE_TIME			(2)
#define SMC_WP_WE_ASSERTION_DELAY		(2)
#define SMC_CEOE_OE_ASSERTION_DELAY		(2)
#define SMC_WC_WRITE_CYCLE_TIME			(0x0a)
#define SMC_RC_READ_CYCLE_TIME			(0x0a)

#define CDRU_SMC_ADDRESS_MATCH_DEFAULT		(0x6261FFFF)
#define REG_FN_MOD_AHB				(0x55042028)

struct smc_bcm58202_config {
	u32_t base;
};

/**
 * @brief Write SMC
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to write
 * @param data - Data to write
 *
 * @return None
 */
static void smc_bcm58202_write(struct device *dev, u32_t reg, u32_t data)
{
	const struct smc_bcm58202_config *cfg = dev->config->config_info;

	sys_write32(data, (cfg->base + reg));
}

/**
 * @brief Read SMC
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to read
 *
 * @return Data from the register
 *
 */
static u32_t smc_bcm58202_read(struct device *dev, u32_t reg)
{
	const struct smc_bcm58202_config *cfg = dev->config->config_info;

	SYS_LOG_DBG("smc base %x reg %x", cfg->base, reg);
	return sys_read32(cfg->base + reg);
}

/**
 * @brief Initialization function of smc
 *
 * @param dev Device struct
 *
 * @return 0 if successful, failed otherwise.
 */
s32_t smc_bcm58202_init(struct device *dev)
{
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	struct device *cdru = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	u32_t value;

	/*
	 * SMC non-cached address writes resulting into data corruption
	 * of adjacent bytes Fix from Design team: register write to split inc
	 * burst into single transaction reg32_write(0x55042028,0x3.
	 * NIC_400 Document
	 * fn_mod_ahb Register
	 * This register is valid for AHB interfaces only. You can configure the
	 * register bits as follows:
	 * 0 rd_incr_override.
	 * 1 wr_incr_override.
	 * 2 lock_override.
	 *
	 * rd_incr_override = 1
	 * wr_incr_override = 1
	 * lock_override = 0
	 */
	value = 3;
	sys_write32(value, REG_FN_MOD_AHB);

	/* Reset SMC controller */
	dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_SMC, DMU_DEVICE_RESET);
	k_busy_wait(500);

	value = CDRU_SMC_ADDRESS_MATCH_DEFAULT;
	dmu_write(cdru, CIT_CDRU_SMC_ADDRESS_MATCH_MASK, value);
	value = SMC_TR_TURN_AROUND_TIME << SMC_SET_CYCLES_T5_SHIFT |
		SMC_PC_PAGE_CYCLE_TIME << SMC_SET_CYCLES_T4_SHIFT |
		SMC_WP_WE_ASSERTION_DELAY << SMC_SET_CYCLES_T3_SHIFT |
		SMC_CEOE_OE_ASSERTION_DELAY << SMC_SET_CYCLES_T2_SHIFT |
		SMC_WC_WRITE_CYCLE_TIME << SMC_SET_CYCLES_T1_SHIFT |
		SMC_RC_READ_CYCLE_TIME << SMC_SET_CYCLES_T0_SHIFT;
	smc_bcm58202_write(dev, SMC_SET_CYCLES, value);

	value = 0;
	smc_bcm58202_write(dev, SMC_SET_OPMODE, value);

	value = 2 << SMC_DIRECT_CMD_CMD_TYPE_SHIFT;
	smc_bcm58202_write(dev, SMC_DIRECT_CMD, value);

	value = 0;
	smc_bcm58202_write(dev, SMC_SET_OPMODE, value);

	value = 2 << SMC_DIRECT_CMD_CMD_TYPE_SHIFT |
		1 << SMC_DIRECT_CMD_CHIP_NMBR_SHIFT;
	smc_bcm58202_write(dev, SMC_DIRECT_CMD, value);

	value = 0;
	smc_bcm58202_write(dev, SMC_SET_OPMODE, value);

	return 0;
}

static const struct smc_driver_api smc_funcs = {
	.write = smc_bcm58202_write,
	.read = smc_bcm58202_read,
};

static struct smc_bcm58202_config smc_bcm58202_cfg = {
	.base = SMC_BASE_ADDR,
};

DEVICE_AND_API_INIT(smc, CONFIG_SMC_DEV_NAME, smc_bcm58202_init,
		    NULL, &smc_bcm58202_cfg, POST_KERNEL,
		    CONFIG_SMC_DRIVER_INIT_PRIORITY, &smc_funcs);
