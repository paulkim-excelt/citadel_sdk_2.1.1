/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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
 * @file spru_bbl_access.c
 * @brief BBL register/memory access routines using SPRU interface
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <errno.h>
#include <kernel.h>
#include <misc/util.h>
#include <stdbool.h>
#include <dmu.h>
#include <logging/sys_log.h>
#include "spru_access.h"

/* SPRU registers */
#define SPRU_BBL_WDATA		(SPRU_BASE_ADDRESS + 0x0)
#define SPRU_BBL_CMD		(SPRU_BASE_ADDRESS + 0x4)
#define SPRU_BBL_STATUS		(SPRU_BASE_ADDRESS + 0x8)
#define SPRU_BBL_RDATA		(SPRU_BASE_ADDRESS + 0xC)

/* SPRU register bitfields */
#define CMD_IND_RD		12
#define CMD_IND_WR		11
#define CMD_IND_SOFT_RST_N	10

#define STATUS_ACC_DONE		0

/* CRMU Isolation cell bits for BBL */
#define CRMU_ISO_PDBBL_TAMPER	24
#define CRMU_ISO_PDBBL		16

/* SPRU source select status bits */
#define SPRU_SOURCE_SELECT	0

/* BBL access timeouts in us */
#define BBL_REG_READ_TIMEOUT	500
#define BBL_REG_WRITE_TIMEOUT	500
#define BBL_MEM_READ_TIMEOUT	1500
#define BBL_MEM_WRITE_TIMEOUT	1500

#define BBL_ACCESS_RETRIES	3

#define SPRU_RESET_TOGGLE_DELAY 100	/* 100 us */

/* RTC Second register */
#define BBL_RTC_SECOND		0x0C

#undef SYS_LOG_ERR
#define BBL_ISR_LOG(...)	\
	do {	\
		printk("[SPRU ERR] ");	\
		printk(__VA_ARGS__);	\
		printk("\n");		\
	} while (0)
#define SYS_LOG_ERR BBL_ISR_LOG

/* RTC time at system start */
static u32_t rtc_time_at_start = 0;

/* BBL access enabled flag. Set when bbl_access_enable() is called
 * and checked on all other api calls
 */
static bool access_enabled = false;

/**
 * @brief Poll SPRU status for access complete
 * @details Checks SPRU access complete status bit for the specified timeout
 *	    to determine if the access completed.
 *
 * @param delay_us Delay in micro seconds
 *
 * @return 0 on Success, -ETIMEDOUT if timeout expires
 */
static s32_t spru_poll_status(u32_t delay_us)
{
	/* Wait for timeout */
	do {
		if (sys_read32(SPRU_BBL_STATUS) & BIT(STATUS_ACC_DONE))
			return 0;

		k_busy_wait(1);
	} while (delay_us--);

	return -ETIMEDOUT;
}

/**
 * @brief Read BBL register/memory
 * @details This reads the BBL register/memory specified, by programming
 *	    the SPRU command register for a read.
 *
 * @param addr Address of BBL memory/register to read from
 * @param timeout Timeout in micro seconds to wait for read data to be available
 * @param data Pointer to return the read data
 *
 * return 0 on Success, -errno otherwise
 */
static int bbl_read(u32_t addr, u32_t *data, u32_t timeout)
{
	int ret;

	/* Write the command register to perform a read */
	sys_write32(BIT(CMD_IND_SOFT_RST_N) | BIT(CMD_IND_RD) | addr,
		    SPRU_BBL_CMD);

	/* Wait for the read data to be ready */
	ret = spru_poll_status(timeout);
	if (ret == 0)
		*data = sys_read32(SPRU_BBL_RDATA);

	return ret;
}

/**
 * @brief Write BBL register/memory
 * @details This writes to the BBL register/memory specified, by programming
 *	    the SPRU command register for a write.
 *
 * @param addr Address of BBL memory/register to write to
 * @param timeout Timeout in micro seconds to wait for write to complete
 * @param data Data to write to the specified address
 *
 * return 0 on Success, -ernno otherwise
 */
static int bbl_write(u32_t addr, u32_t data, u32_t timeout)
{
	/* Write the data register */
	sys_write32(data, SPRU_BBL_WDATA);

	/* Write the command register to perform a write */
	sys_write32(BIT(CMD_IND_SOFT_RST_N) | BIT(CMD_IND_WR) | addr,
		    SPRU_BBL_CMD);

	/* Wait for the read data to be ready */
	return spru_poll_status(timeout);
}

/**
 * @brief Reset SPRU Interface
 * @details Reset SPRU Interface by toggling the SPRU Soft reset bit
 *
 * @param delay_us Delay in toggling SPRU soft reset bit (in microseconds)
 *
 * @return none
 */
void spru_interface_reset(u32_t delay_us)
{
	/* Assert soft reset */
	sys_write32(0x0, SPRU_BBL_CMD);
	k_busy_wait(delay_us);

	/* Release reset */
	sys_write32(BIT(CMD_IND_SOFT_RST_N), SPRU_BBL_CMD);
	k_busy_wait(delay_us);
}

/**
 * @brief Write BBL Register
 * @details Write BBL Register using the spru interface
 *
 * @param addr register address
 * @param data Data to be written
 *
 * @return 0 in Success, -errno otherwise
 */
int bbl_write_reg(u32_t addr, u32_t data)
{
	u32_t key;
	int ret, i;

	if (access_enabled == false)
		return -EPERM;

	key = irq_lock();
	for (i = 0; i < BBL_ACCESS_RETRIES; i++) {
		ret = bbl_write(addr, data, BBL_REG_WRITE_TIMEOUT);
		if (ret == 0)
			break;
		spru_interface_reset(SPRU_RESET_TOGGLE_DELAY);
	}
	irq_unlock(key);

	if (ret) {
		SYS_LOG_ERR("Error writing BBL Reg: 0x%x\n", addr);
	}

	return ret;
}

/**
 * @brief Read BBL Register
 * @details Read BBL Register using the spru interface
 *
 * @param addr register address
 * @param data Data to be Read
 *
 * @return 0 in Success, -errno otherwise
 */
int bbl_read_reg(u32_t addr, u32_t *data)
{
	u32_t key;
	int ret, i;

	if (access_enabled == false)
		return -EPERM;

	key = irq_lock();
	for (i = 0; i < BBL_ACCESS_RETRIES; i++) {
		ret = bbl_read(addr, data, BBL_REG_READ_TIMEOUT);
		if (ret == 0)
			break;
		spru_interface_reset(SPRU_RESET_TOGGLE_DELAY);
	}
	irq_unlock(key);

	if (ret) {
		SYS_LOG_ERR("Error reading BBL Reg: 0x%x\n", addr);
	}

	return ret;
}

/**
 * @brief Write BBL Memory
 * @details Write BBL Memory using the SPRU interface
 *
 * @param addr Memory address
 * @param data Data to be written
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_write_mem(u32_t addr, u32_t data)
{
	u32_t key;
	int ret, i;

	if (access_enabled == false)
		return -EPERM;

	key = irq_lock();
	for (i = 0; i < BBL_ACCESS_RETRIES; i++) {
		ret = bbl_write(addr, data, BBL_MEM_WRITE_TIMEOUT);
		if (ret == 0)
			break;
		spru_interface_reset(SPRU_RESET_TOGGLE_DELAY);
	}
	irq_unlock(key);

	if (ret) {
		SYS_LOG_ERR("Error Writing BBL Memory: 0x%x\n", addr);
	}

	return ret;
}

/**
 * @brief Read BBL Memory
 * @details Read BBL Memory using SPRU interface
 *
 * @param addr Memory address
 * @param data Location to write the read data to
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_read_mem(u32_t addr, u32_t *data)
{
	u32_t key;
	int ret, i;

	if (access_enabled == false)
		return -EPERM;

	key = irq_lock();
	for (i = 0; i < BBL_ACCESS_RETRIES; i++) {
		ret = bbl_read(addr, data, BBL_MEM_READ_TIMEOUT);
		if (ret == 0)
			break;
		spru_interface_reset(SPRU_RESET_TOGGLE_DELAY);
	}
	irq_unlock(key);

	if (ret) {
		SYS_LOG_ERR("Error Reading BBL Memory: 0x%x\n", addr);
	}

	return ret;
}

/**
 * @brief Enable access to BBL registers/memory
 * @details Set BBL authentication code/check registers to enable BBL access.
 *	    This api must be called before calling any other BBL red/writes apis
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_access_enable(void)
{
	int ret;
	struct device *dmu_dev;
	u32_t val, retries = 500;

	dmu_dev = device_get_binding(CONFIG_DMU_DEV_NAME);
	if (dmu_dev == NULL)
		return -EIO;

	/* Wait till SPRU is powered by AON */
	do {
		val = dmu_read(dmu_dev, CIT_CRMU_SPRU_SOURCE_SEL_STAT);
		val &= BIT(SPRU_SOURCE_SELECT);
	} while ((val != 0x0) && retries--);

	if (retries == 0) {
		SYS_LOG_ERR("SPRU powered by AON not set!");
		return -EIO;
	}

	/* Reset SPRU interface */
	spru_interface_reset(SPRU_RESET_TOGGLE_DELAY);

	/* Disable Isolation cell for BBL */
	val = dmu_read(dmu_dev, CIT_CRMU_ISO_CELL_CONTROL);
	val &= ~BIT(CRMU_ISO_PDBBL_TAMPER);
	val &= ~BIT(CRMU_ISO_PDBBL);
	dmu_write(dmu_dev, CIT_CRMU_ISO_CELL_CONTROL, val);

	/* Set auth code/check to same value */
	val = dmu_read(dmu_dev, CIT_CRMU_BBL_AUTH_CODE);

	val++;
	dmu_write(dmu_dev, CIT_CRMU_BBL_AUTH_CODE, val);
	dmu_write(dmu_dev, CIT_CRMU_BBL_AUTH_CHECK, val);

	access_enabled = true;

	/* Save the RTC time at system startup */
	ret = bbl_read_reg(BBL_RTC_SECOND, &rtc_time_at_start);
	if (ret)
		rtc_time_at_start = 0;

	return 0;
}

/**
 * @brief Get RTC time at system bootup
 * @details Retrieves the RTC time at startup.
 *
 * @return RTC time before SPRU interface is enabled
 */
u32_t spru_get_rtc_time_at_startup(void)
{
	if (access_enabled == false)
		bbl_access_enable();

	return rtc_time_at_start;
}
