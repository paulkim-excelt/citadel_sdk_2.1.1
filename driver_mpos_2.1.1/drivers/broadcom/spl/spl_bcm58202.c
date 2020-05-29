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
 * @file spl_bcm58202.c
 * @brief Secure protection logic driver implementation
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <spl.h>
#include <zephyr/types.h>
#include <cortex_a/cache.h>

#define SPL_WDOG			(0x00)
#define SPL_CALIB_0			(0x04)
#define SPL_CALIB_1			(0x08)
#define SPL_CALIB_2			(0x0c)

#define SPL_FREQ_MON_EN(ID)		(0x10 + (ID * 0x10))
#define SPL_FREQ_MON_HIGH_FREQ_EN_BIT	(16)
#define SPL_FREQ_MON_LOW_FREQ_EN_BIT	(0)
#define SPL_MONITOR_POINT(ID)		(0x14 + (ID * 0x10))
#define SPL_LOW_THRESH(ID)		(0x18 + (ID * 0x10))
#define SPL_HIGH_THRESH(ID)		(0x1c + (ID * 0x10))

#define SPL_RST_MON_EN(ID)		(0x40 + (ID * 0x14))
#define SPL_RST_MON_EN_BIT		(0)
#define SPL_RST_CNT_THRESH(ID)		(0x44 + (ID * 0x14))
#define SPL_WIN_CNT_THRESH(ID)		(0x48 + (ID * 0x14))
#define SPL_RST_CNT_VAL(ID)		(0x4c + (ID * 0x14))
#define SPL_WIN_CNT_VAL(ID)		(0x50 + (ID * 0x14))

#define OSC_EN				(0x90)
#define OSC_EN_BIT			(0)

#define SPL_MAX_CALIB0			(0x3fffffff)
#define SPL_MAX_CALIB1			(0x3fffffff)
#define SPL_MAX_CALIB2			(0x7fff)

#define SPL_MAX_FREQ_MON		(0x3)
#define SPL_MAX_MONITOR_POINT		(0xffff)
#define SPL_MAX_LOW_THRESH		(0xffff)
#define SPL_MAX_HIGH_THRESH		(0xffff)

#define SPL_MAX_RESET_MON		(0x4)
#define SPL_MAX_RESET_COUNT		(0xff)
#define SPL_MAX_RESET_THRESH		(0xff)

#define CRMU_SPL_EVENT_LOG_CLEAR_BIT		(31)
#define CRMU_SPL_HI_FREQ_EVENT_LOG_OFFSET	(16)
#define CRMU_SPL_HI_FREQ_EVENT_LOG_MASK		(0x07)
#define CRMU_SPL_LO_FREQ_EVENT_LOG_OFFSET	(16)
#define CRMU_SPL_LO_FREQ_EVENT_LOG_MASK		(0x07)
#define CRMU_SPL_RESET_EVENT_LOG_OFFSET		(16)
#define CRMU_SPL_RESET_EVENT_LOG_MASK		(0x0f)

struct spl_config {
	u32_t base;
};

struct spl_data {
	spl_isr wdog_cb;
	spl_isr freq_cb;
	spl_isr pvt_cb;
	spl_isr reset_cb;
};

static struct spl_data spl_bcm58202_data;

static s32_t spl_bcm58202_init(struct device *dev);

/**
 * @brief Set SPL watchdog timer value
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param value Watchdog timer value
 *
 * @retval 0
 */
static s32_t spl_bcm58202_set_watchdog_val(struct device *dev, u32_t value)
{
	const struct spl_config *cfg = dev->config->config_info;

	sys_write32(value, (cfg->base + SPL_WDOG));

	return 0;
}

/**
 * @brief   Get SPL watchdog timer value
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param value Watchdog timer value ptr
 *
 * @retval 0
 */
static s32_t spl_bcm58202_get_watchdog_val(struct device *dev, u32_t *value)
{
	const struct spl_config *cfg = dev->config->config_info;

	if (value == NULL)
		return -EINVAL;

	*value = sys_read32(cfg->base + SPL_WDOG);

	return 0;
}

/**
 * @brief Set SPL calibration
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param calib0  Calibration 0, range 0...0x3fffffff
 * @param calib1  Calibration 1, range 0...0x3fffffff
 * @param calib2  Calibration 2, range 0...0x7fff
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_set_calib_val(struct device *dev, u32_t calib0,
					u32_t calib1, u32_t calib2)
{
	const struct spl_config *cfg = dev->config->config_info;

	if ((calib0 > SPL_MAX_CALIB0) || (calib1 > SPL_MAX_CALIB1) ||
	    (calib2 > SPL_MAX_CALIB2))
		return -EINVAL;

	sys_write32(calib0, (cfg->base + SPL_CALIB_0));
	sys_write32(calib1, (cfg->base + SPL_CALIB_1));
	sys_write32(calib2, (cfg->base + SPL_CALIB_2));

	return 0;
}

/**
 * @brief Get SPL calibration.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param calib0  Calibration 0 ptr
 * @param calib1  Calibration 1 ptr
 * @param calib2  Calibration 2 ptr
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_get_calib_val(struct device *dev, u32_t *calib0,
					u32_t *calib1, u32_t *calib2)
{
	const struct spl_config *cfg = dev->config->config_info;

	if ((calib0 == NULL) || (calib1 == NULL) || (calib2 == NULL))
		return -EINVAL;

	*calib0 = sys_read32(cfg->base + SPL_CALIB_0) & SPL_MAX_CALIB0;
	*calib1 = sys_read32(cfg->base + SPL_CALIB_1) & SPL_MAX_CALIB1;
	*calib2 = sys_read32(cfg->base + SPL_CALIB_2) & SPL_MAX_CALIB2;

	return 0;
}

/**
 * @brief   Set SPL frequency monitor.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Frequency monitor id
 * @param mon Frequency monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_set_freq_monitor(struct device *dev, u8_t id,
					   struct frequency_monitor *mon)
{
	const struct spl_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if ((mon == NULL) || (id >= SPL_MAX_FREQ_MON) ||
	    (mon->monitor_point > SPL_MAX_MONITOR_POINT) ||
	    (mon->low_threshold > SPL_MAX_LOW_THRESH) ||
	    (mon->high_threshold > SPL_MAX_HIGH_THRESH))
		return -EINVAL;

	if (mon->low_en)
		sys_set_bit((base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_LOW_FREQ_EN_BIT);
	else
		sys_clear_bit((base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_LOW_FREQ_EN_BIT);
	if (mon->high_en)
		sys_set_bit((base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_HIGH_FREQ_EN_BIT);
	else
		sys_clear_bit((base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_HIGH_FREQ_EN_BIT);

	sys_write32(mon->monitor_point, (cfg->base + SPL_MONITOR_POINT(id)));
	sys_write32(mon->low_threshold, (cfg->base + SPL_LOW_THRESH(id)));
	sys_write32(mon->high_threshold, (cfg->base + SPL_HIGH_THRESH(id)));

	return 0;
}

/**
 * @brief   Get SPL frequency monitor.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Frequency monitor id
 * @param mon Frequency monitor structure ptr
 *
 * @retval  0 for success, error otherwise
 */
static s32_t spl_bcm58202_get_freq_monitor(struct device *dev, u8_t id,
					   struct frequency_monitor *mon)
{
	const struct spl_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if ((mon == NULL) || (id >= SPL_MAX_FREQ_MON))
		return -EINVAL;

	if (sys_test_bit((base + SPL_FREQ_MON_EN(id)),
				 SPL_FREQ_MON_LOW_FREQ_EN_BIT))
		mon->low_en = true;
	else
		mon->low_en = false;

	if (sys_test_bit((base + SPL_FREQ_MON_EN(id)),
				 SPL_FREQ_MON_HIGH_FREQ_EN_BIT))
		mon->high_en = true;
	else
		mon->high_en = false;

	mon->monitor_point = sys_read32(base + SPL_MONITOR_POINT(id)) &
						SPL_MAX_MONITOR_POINT;
	mon->low_threshold = sys_read32(base + SPL_LOW_THRESH(id)) &
						SPL_MAX_LOW_THRESH;
	mon->high_threshold = sys_read32(base + SPL_HIGH_THRESH(id)) &
						SPL_MAX_HIGH_THRESH;

	return 0;
}

/**
 * @brief Set Reset monitor
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Reset monitor id
 * @param mon Reset monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_set_reset_monitor(struct device *dev, u8_t id,
					    struct reset_monitor *mon)
{
	const struct spl_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if ((mon == NULL) || (id >= SPL_MAX_RESET_MON))
		return -EINVAL;

	if (mon->reset_en)
		sys_set_bit((base + SPL_RST_MON_EN(id)),
						 SPL_RST_MON_EN_BIT);
	else
		sys_clear_bit((base + SPL_RST_MON_EN(id)),
						SPL_RST_MON_EN_BIT);

	sys_write32(mon->reset_count_threshold,
					 (cfg->base + SPL_RST_CNT_THRESH(id)));
	sys_write32(mon->window_count_threshold,
					 (cfg->base + SPL_WIN_CNT_THRESH(id)));

	return 0;
}

/**
 * @brief Get Reset monitor
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id Reset monitor id
 * @param mon Reset monitor structure ptr
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_get_reset_monitor(struct device *dev, u8_t id,
					    struct reset_monitor *mon)
{
	const struct spl_config *cfg = dev->config->config_info;
	u32_t base = cfg->base;

	if ((mon == NULL) || (id >= SPL_MAX_RESET_MON))
		return -EINVAL;

	if (sys_test_bit((base + SPL_RST_MON_EN(id)), SPL_RST_MON_EN_BIT))
		mon->reset_en = true;
	else
		mon->reset_en = false;

	mon->reset_count = sys_read32(cfg->base + SPL_RST_CNT_VAL(id)) &
						SPL_MAX_RESET_COUNT;
	mon->reset_count_threshold =
			 sys_read32(cfg->base + SPL_RST_CNT_THRESH(id)) &
						SPL_MAX_RESET_COUNT;
	mon->window_count = sys_read32(cfg->base + SPL_WIN_CNT_VAL(id));
	mon->window_count_threshold =
			 sys_read32(cfg->base + SPL_WIN_CNT_THRESH(id));

	return 0;
}

/**
 * @brief Set oscillator enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable Enable SPL oscillator, range 0.1
 *
 * @retval 0
 */
static s32_t spl_bcm58202_set_oscillator_enable(struct device *dev, bool enable)
{
	const struct spl_config *cfg = dev->config->config_info;

	if (enable)
		sys_set_bit((cfg->base + OSC_EN), OSC_EN_BIT);
	else
		sys_clear_bit((cfg->base + OSC_EN), OSC_EN_BIT);

	return 0;
}

/**
 * @brief Get oscillator enable
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable Ptr to get mode
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_get_oscillator_enable(struct device *dev,
						bool *enable)
{
	const struct spl_config *cfg = dev->config->config_info;

	if (enable == NULL)
		return -EINVAL;

	if (sys_test_bit((cfg->base + OSC_EN), OSC_EN_BIT))
		*enable = true;
	else
		*enable = false;

	return 0;
}

/**
 * @brief Clear event log
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_clear_event_log(struct device *dev)
{
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);

	ARG_UNUSED(dev);
	dmu_write(dmu, CIT_CRMU_SPL_EVENT_LOG, 1);

	return 0;
}

/**
 * @brief Read event log
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param high_freq High frequency event log ptr
 * @param low_freq Low frequency event log ptr
 * @param reset Reset event log ptr
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_read_event_log(struct device *dev, u8_t *high_freq,
					 u8_t *low_freq, u8_t *reset)
{
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	u32_t value;

	ARG_UNUSED(dev);
	if ((dmu == NULL) || (high_freq == NULL) || (low_freq == NULL) ||
	    (reset == NULL))
		return -EINVAL;

	value = dmu_read(dmu, CIT_CRMU_SPL_EVENT_LOG);
	*high_freq = (value >> CRMU_SPL_HI_FREQ_EVENT_LOG_OFFSET) &
					 CRMU_SPL_HI_FREQ_EVENT_LOG_MASK;
	*low_freq = (value >> CRMU_SPL_LO_FREQ_EVENT_LOG_OFFSET) &
					 CRMU_SPL_LO_FREQ_EVENT_LOG_MASK;
	*reset = (value >> CRMU_SPL_RESET_EVENT_LOG_OFFSET) &
					CRMU_SPL_RESET_EVENT_LOG_MASK;

	return 0;
}

static void spl_bcm58202_wdog_isr(void *arg)
{
	struct device *dev = arg;
	struct spl_data *priv = dev->driver_data;

	spl_bcm58202_set_watchdog_val(dev, 0);
	if (priv->wdog_cb)
		priv->wdog_cb();
}

static void spl_bcm58202_freq_isr(void *arg)
{
	struct device *dev = arg;
	struct spl_data *priv = dev->driver_data;
	const struct spl_config *cfg = dev->config->config_info;
	u32_t id;

	for (id = 0; id < SPL_MAX_FREQ_MON; id++)
		sys_clear_bit((cfg->base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_LOW_FREQ_EN_BIT);

	if (priv->freq_cb)
		priv->freq_cb();
}

static void spl_bcm58202_pvt_isr(void *arg)
{
	struct device *dev = arg;
	struct spl_data *priv = dev->driver_data;
	const struct spl_config *cfg = dev->config->config_info;
	u32_t id;

	for (id = 0; id < SPL_MAX_FREQ_MON; id++)
		sys_clear_bit((cfg->base + SPL_FREQ_MON_EN(id)),
					 SPL_FREQ_MON_HIGH_FREQ_EN_BIT);

	if (priv->pvt_cb)
		priv->pvt_cb();
}

static void spl_bcm58202_rst_isr(void *arg)
{
	struct device *dev = arg;
	struct spl_data *priv = dev->driver_data;
	const struct spl_config *cfg = dev->config->config_info;
	u32_t id;

	for (id = 0; id < SPL_MAX_RESET_MON; id++)
		sys_clear_bit((cfg->base + SPL_RST_MON_EN(id)),
						SPL_RST_MON_EN_BIT);

	if (priv->reset_cb)
		priv->reset_cb();
}

/**
 * @brief Register SPL interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param intr Interrupt to register
 * @param handler Interrupt handler
 *
 * @retval 0 for success, error otherwise
 */
static s32_t spl_bcm58202_register_isr(struct device *dev, enum spl_int intr,
				       spl_isr handler)
{
	struct spl_data *priv = dev->driver_data;

	switch (intr) {
	case SPL_WDOG_INTERRUPT:
		priv->wdog_cb = handler;
		irq_enable(SPI_IRQ(CHIP_INTR_SPL_WDOG_INTERRUPT));
		break;

	case SPL_RST_INTERRUPT:
		priv->reset_cb = handler;
		irq_enable(SPI_IRQ(CHIP_INTR_SPL_RST_INTERRUPT));
		break;

	case SPL_PVT_INTERRUPT:
		priv->pvt_cb = handler;
		irq_enable(SPI_IRQ(CHIP_INTR_SPL_PVT_INTERRUPT));
		break;

	case SPL_FREQ_INTERRUPT:
		priv->freq_cb = handler;
		irq_enable(SPI_IRQ(CHIP_INTR_SPL_FREQ_INTERRUPT));
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Unregister SPL interrupt handler
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param intr Interrupt to unregister
 *
 * @retval 0
 */
static s32_t spl_bcm58202_unregister_isr(struct device *dev, enum spl_int intr)
{
	struct spl_data *priv = dev->driver_data;

	switch (intr) {
	case SPL_WDOG_INTERRUPT:
		irq_disable(SPI_IRQ(CHIP_INTR_SPL_WDOG_INTERRUPT));
		priv->wdog_cb = NULL;
		break;

	case SPL_RST_INTERRUPT:
		irq_disable(SPI_IRQ(CHIP_INTR_SPL_RST_INTERRUPT));
		priv->reset_cb = NULL;
		break;

	case SPL_PVT_INTERRUPT:
		irq_disable(SPI_IRQ(CHIP_INTR_SPL_PVT_INTERRUPT));
		priv->pvt_cb = NULL;
		break;

	case SPL_FREQ_INTERRUPT:
		irq_disable(SPI_IRQ(CHIP_INTR_SPL_FREQ_INTERRUPT));
		priv->freq_cb = NULL;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct spl_driver_api spl_funcs = {
	.set_watchdog_val = spl_bcm58202_set_watchdog_val,
	.get_watchdog_val = spl_bcm58202_get_watchdog_val,
	.set_calib_val = spl_bcm58202_set_calib_val,
	.get_calib_val = spl_bcm58202_get_calib_val,
	.set_freq_monitor = spl_bcm58202_set_freq_monitor,
	.get_freq_monitor = spl_bcm58202_get_freq_monitor,
	.set_reset_monitor = spl_bcm58202_set_reset_monitor,
	.get_reset_monitor = spl_bcm58202_get_reset_monitor,
	.set_oscillator_enable = spl_bcm58202_set_oscillator_enable,
	.get_oscillator_enable = spl_bcm58202_get_oscillator_enable,
	.register_isr = spl_bcm58202_register_isr,
	.unregister_isr = spl_bcm58202_unregister_isr,
	.clear_event_log = spl_bcm58202_clear_event_log,
	.read_event_log = spl_bcm58202_read_event_log,
};

static const struct spl_config spl_bcm58202_config = {
	.base = SPL_BASE_ADDR,
};

DEVICE_AND_API_INIT(spl, CONFIG_SPL_DEV_NAME, spl_bcm58202_init,
		    &spl_bcm58202_data, &spl_bcm58202_config, POST_KERNEL,
		    CONFIG_SPL_INIT_PRIORITY, &spl_funcs);

/**
 * @brief SPL init
 *
 * @param dev Device struct
 *
 * @return 0 for success
 */
static s32_t spl_bcm58202_init(struct device *dev)
{
	struct spl_data *priv = dev->driver_data;
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);

	dmu_write(dmu, CIT_CRMU_SPL_RESET_CONTROL, 0);
	dmu_write(dmu, CIT_CRMU_SPL_RESET_CONTROL, 1);

	priv->freq_cb = NULL;
	priv->pvt_cb = NULL;
	priv->reset_cb = NULL;

	IRQ_CONNECT(SPI_IRQ(CHIP_INTR_SPL_WDOG_INTERRUPT), 0,
		spl_bcm58202_wdog_isr, DEVICE_GET(spl), 0);

	IRQ_CONNECT(SPI_IRQ(CHIP_INTR_SPL_RST_INTERRUPT), 0,
		spl_bcm58202_rst_isr, DEVICE_GET(spl), 0);

	IRQ_CONNECT(SPI_IRQ(CHIP_INTR_SPL_PVT_INTERRUPT), 0,
		spl_bcm58202_pvt_isr, DEVICE_GET(spl), 0);

	IRQ_CONNECT(SPI_IRQ(CHIP_INTR_SPL_FREQ_INTERRUPT), 0,
		spl_bcm58202_freq_isr, DEVICE_GET(spl), 0);

	return 0;
}
