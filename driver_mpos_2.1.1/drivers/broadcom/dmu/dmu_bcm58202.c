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

/*
 * @file dmu_bcm58202.c
 * @brief APIs for DMU
 */
#include <board.h>
#include <arch/cpu.h>
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <dmu.h>
#include <logging/sys_log.h>
#include <sys_io.h>

#define POLL_CNTR		5000

/**
 * @brief dmu block configuration structure
 *
 * @param base Base address for the dmu block
 */
struct dmu_bcm58202_cfg {
	u32_t base;
};

/**
 * @brief Write data to register in corresponding dmu block
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register to write
 * @param data Data to write
 *
 * @return none
 *
 * @note For registers, use macros provided in dmu.h
 */
static void dmu_bcm58202_write(struct device *dev, u32_t reg, u32_t data)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_write32(data, (dmu->base + reg));
}

/**
 * @brief Read data from register in corresponding dmu block
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register to read
 *
 * @return Data from the register
 *
 * @note For registers, use macros provided in dmu.h
 */
static u32_t dmu_bcm58202_read(struct device *dev, u32_t reg)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	SYS_LOG_DBG("dmu base %x reg %x", dmu->base, reg);
	return sys_read32(dmu->base + reg);
}

/**
 * @brief Control the DMU device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param blk - DMU block
 * @param device - Device to control
 * @param action - Action for the device: reset, shut down or up
 *
 * @return 0 for success, error otherwise
 *
 * @note Use binding with CONFIG_DMU_DEV_NAME device
 */
static s32_t dmu_bcm58202_device_ctrl(struct device *dev, enum dmu_block blk,
				      enum dmu_device device,
				      enum dmu_action action)
{
	struct device *cdru_dev = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	const struct dmu_bcm58202_cfg *cdru = cdru_dev->config->config_info;
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;
	u32_t base;

	switch (blk) {
	case DMU_SYSTEM:
		base = dmu->base + CIT_CRMU_SOFT_RESET_CTRL;
		break;
	case DMU_DR_UNIT:
		base = cdru->base + CIT_CDRU_SW_RESET_CTRL;
		break;
	case DMU_SP_LOGIC:
		base = dmu->base + CIT_CRMU_SPL_RESET_CONTROL;
		break;
	default:
		return -EINVAL;
	};

	switch (action) {
	case DMU_DEVICE_RESET:
		sys_clear_bit(base, device);
		sys_set_bit(base, device);
		break;
	case DMU_DEVICE_SHUTDOWN:
		sys_clear_bit(base, device);
		break;
	case DMU_DEVICE_BRINGUP:
		sys_set_bit(base, device);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Mask the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static void crmu_bcm58202_iproc_intr_mask_set(struct device *dev,
					      enum iproc_intr id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_set_bit((dmu->base + CIT_CRMU_IPROC_INTR_MASK), id);
}

/**
 * @brief Mask the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static void crmu_bcm58202_iproc_intr_unmask(struct device *dev,
					    enum iproc_intr id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_clear_bit((dmu->base + CIT_CRMU_IPROC_INTR_MASK), id);
}

/**
 * @brief Get the iproc interrupt status
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return status
 */
static s32_t crmu_bcm58202_iproc_intr_status_get(struct device *dev,
					  enum iproc_intr id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;
	u32_t status = 0;
	u32_t cnt;

	for (cnt = 0; cnt < POLL_CNTR; cnt++) {
		status = sys_test_bit((dmu->base + CIT_CRMU_IPROC_INTR_CLEAR),
				      id);
		if (status)
			break;
	}

	return status;
}

/**
 * @brief clear the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static void crmu_bcm58202_iproc_intr_clr(struct device *dev, enum iproc_intr id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_clear_bit((dmu->base + CIT_CRMU_IPROC_INTR_CLEAR), id);
}

/**
 * @brief Mask the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static void crmu_bcm58202_event_mask(struct device *dev, enum crmu_event id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_set_bit((dmu->base + CIT_CRMU_MCU_EVENT_MASK), id);
}

/**
 * @brief Unmask the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static void crmu_bcm58202_event_unmask(struct device *dev, enum crmu_event id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_clear_bit((dmu->base + CIT_CRMU_MCU_EVENT_MASK), id);
}

/**
 * @brief Get the crmu event status
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return status
 */
static s32_t crmu_bcm58202_event_status_get(struct device *dev,
					    enum crmu_event id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;
	u32_t status = 0;
	u32_t cnt;

	for (cnt = 0; cnt < POLL_CNTR; cnt++) {
		status = sys_test_bit((dmu->base + CIT_CRMU_MCU_EVENT_STATUS),
				      id);
		if (status)
			break;
	}

	return status;
}

/**
 * @brief clear the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static void crmu_bcm58202_event_clear(struct device *dev, enum crmu_event id)
{
	const struct dmu_bcm58202_cfg *dmu = dev->config->config_info;

	sys_clear_bit((dmu->base + CIT_CRMU_MCU_EVENT_CLEAR), id);
}

static s32_t dmu_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static s32_t cdru_init(struct device *dev)
{
	const struct dmu_bcm58202_cfg *cdru = dev->config->config_info;

	/* Set IO_PAD_CONTROL.FORCE_PAD_IN = 0 */
	sys_write32(0, cdru->base + CIT_CRMU_CHIP_IO_PAD_CONTROL);

	return 0;
}

static const struct dmu_driver_api dmu_bcm58202_api = {
	.write = dmu_bcm58202_write,
	.read = dmu_bcm58202_read,
	.device_ctrl = dmu_bcm58202_device_ctrl,
	.mcu_event_mask = crmu_bcm58202_event_mask,
	.mcu_event_unmask = crmu_bcm58202_event_unmask,
	.mcu_event_clear = crmu_bcm58202_event_clear,
	.mcu_event_status_get = crmu_bcm58202_event_status_get,
	.iproc_intr_mask_set = crmu_bcm58202_iproc_intr_mask_set,
	.iproc_intr_unmask = crmu_bcm58202_iproc_intr_unmask,
	.iproc_intr_clear = crmu_bcm58202_iproc_intr_clr,
	.iproc_intr_status_get = crmu_bcm58202_iproc_intr_status_get,
};

static const struct dmu_driver_api cdru_bcm58202_api = {
	.write = dmu_bcm58202_write,
	.read = dmu_bcm58202_read,
};

static const struct dmu_driver_api dru_bcm58202_api = {
	.write = dmu_bcm58202_write,
	.read = dmu_bcm58202_read,
};

static const struct dmu_bcm58202_cfg bcm58202_cfg_dmu = {
	.base = DMU_COMMON_SPACE_BASE_ADDR,
};

static const struct dmu_bcm58202_cfg bcm58202_cfg_cdru = {
	.base = CDRU_BASE_ADDR,
};

static const struct dmu_bcm58202_cfg bcm58202_cfg_dru = {
	.base = CRMU_DRU_BASE_ADDR,
};

DEVICE_AND_API_INIT(dmu_0, CONFIG_DMU_DEV_NAME, dmu_init, NULL,
		    &bcm58202_cfg_dmu, PRE_KERNEL_2,
		    CONFIG_DMU_INIT_PRIORITY, &dmu_bcm58202_api);

DEVICE_AND_API_INIT(dmu_1, CONFIG_DMU_CDRU_DEV_NAME, cdru_init, NULL,
		    &bcm58202_cfg_cdru, PRE_KERNEL_2,
		    CONFIG_DMU_INIT_PRIORITY, &cdru_bcm58202_api);

DEVICE_AND_API_INIT(dmu_2, CONFIG_DMU_DRU_DEV_NAME, dmu_init, NULL,
		    &bcm58202_cfg_dru, PRE_KERNEL_2,
		    CONFIG_DMU_INIT_PRIORITY, &dru_bcm58202_api);
