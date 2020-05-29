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
 * @file wdt_bcm5820x.c
 * @brief WDT driver implementation
 */
#include <arch/cpu.h>
#include <genpll.h>
#include <board.h>
#include <kernel.h>
#include <device.h>
#include <dmu.h>
#include <irq.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <wdt.h>

/* Timer register offset */
#define WATCHDOG_REG_LOAD	(0x0)
#define WATCHDOG_REG_VALUE	(0x4)
#define WATCHDOG_REG_CONTROL	(0x8)
#define WATCHDOG_REG_INTCLR	(0xC)
#define WATCHDOG_REG_RIS	(0x10)
#define WATCHDOG_REG_MIS	(0x14)
#define WATCHDOG_REG_LOCK	(0x0C00)
#define WDOGCONTROL__ITEN	0
#define WDOGCONTROL__RESEN	1

#define MAX_WDOG_LOAD_VAL	0xFFFFFFFF

struct wdt_config {
	u32_t base;
	int irq;
};

struct wdt_data {
	/* If this flag is enabled, the watchdog timer reset is enabled
	 * in the watchdog control register.
	 * Otherwise watchdog expiry would just result in interrupt, which
	 * would then reload decrementing counter. If the interrupt is not
	 * cleared before the counter becomes 0, the counter would just
	 * stop and reset would not be triggered.
	 */
	bool reset_en;
	wdt_isr wdt_handler;
};


static int wdt_set_reg(struct device *dev, u32_t reg, u32_t data)
{
	const struct wdt_config *cfg = dev->config->config_info;

	sys_write32(data, (cfg->base + reg));

	return 0;
}

static int wdt_get_reg(struct device *dev, u32_t reg, u32_t *data)
{
	const struct wdt_config *cfg = dev->config->config_info;

	*data = sys_read32(cfg->base + reg);

	return 0;
}

/*
 * Lock the Watchdog module register by writing into lock register
 */
static int wdt_lock(struct device *dev)
{
	u32_t val = 0;

	/* Any value should be fine */
	wdt_set_reg(dev, WATCHDOG_REG_LOCK, 0x101);
	wdt_get_reg(dev, WATCHDOG_REG_LOCK, &val);
	if (val != 1) {
		SYS_LOG_ERR("WATCHDOG_REG_LOCK 0x%x\n", val);
		return -1;
	}

	return 0;
}

/*
 * Write WDOGLOCK with value 0x1ACCE551 to lock register so that
 * watchdog module comes out of lock state
 */
static int wdt_unlock(struct device *dev)
{
	u32_t val = 0;

	wdt_set_reg(dev, WATCHDOG_REG_LOCK, 0x1ACCE551);
	wdt_get_reg(dev, WATCHDOG_REG_LOCK, &val);
	if (val != 0) {
		SYS_LOG_ERR("WATCHDOG_REG_LOCK 0x%x\n", val);
		return -1;
	}

	return 0;
}

/*
 * Clear the interrupt by writing into clear register
 */
static int wdt_clear_intr(struct device *dev)
{
	wdt_set_reg(dev, WATCHDOG_REG_INTCLR, 0x01);

	return 0;
}

/*
 * Enable the control register
 */
static int wdt_enable(struct device *dev)
{
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;
	u32_t val;

	if (data->reset_en) /* Enable interrupt and reset */
		wdt_set_reg(dev, WATCHDOG_REG_CONTROL,
			    ((0x1 << WDOGCONTROL__ITEN) |
			     (0x1 << WDOGCONTROL__RESEN)));
	else /* Only interrupt */
		wdt_set_reg(dev, WATCHDOG_REG_CONTROL,
			    (0x1 << WDOGCONTROL__ITEN));

	wdt_get_reg(dev, WATCHDOG_REG_CONTROL, &val);
	SYS_LOG_DBG("Watchdog control register: 0x%x\n", val);

	return 0;
}

/*
 * Disable the control register
 */
static int wdt_disable(struct device *dev)
{
	u32_t val;

	wdt_set_reg(dev, WATCHDOG_REG_CONTROL, 0);

	wdt_get_reg(dev, WATCHDOG_REG_VALUE, &val);
	SYS_LOG_DBG("Watchdog value register: 0x%x\n", val);

	return 0;
}

/* Watchdog API */

/*
 * unlock the wdog write, clear wdogintr status,
 * set wdt timeout value and enable the wdogintr
 * parameter "time_ms" is the interrupt interval in msec
 *
 */
static int wdt_bcm5820x_start(struct device *dev, u32_t time_ms)
{
	int result = 0;
	u64_t wdogload;
	u32_t actual_val;
	u32_t reg_val = 0;
	u32_t rate;
	u32_t max_timeout;

	result = clk_get_wtus(&rate);
	if (result)
		return result;

	wdogload = (u64_t) ((u64_t)time_ms * (rate/MSEC_PER_SEC) - 1);
	actual_val = (u32_t) wdogload;
	max_timeout = ((MAX_WDOG_LOAD_VAL/rate) * MSEC_PER_SEC);

	if (time_ms > max_timeout) {
		SYS_LOG_ERR("Exceed max watchdog timeout %us\n",
			    max_timeout);
		return -1;
	}

	if (wdogload > MAX_WDOG_LOAD_VAL)
		actual_val = MAX_WDOG_LOAD_VAL;

	SYS_LOG_DBG("clock: %dHz -- wdogload 0x%x, actual_val 0x%x\n", rate,
			(u32_t)wdogload, actual_val);

	result = wdt_unlock(dev);
	if (result) {
		SYS_LOG_ERR("unlock failed");
		return result;
	}

	result = wdt_disable(dev);
	if (result) {
		SYS_LOG_ERR("disable failed");
		return result;
	}

	wdt_set_reg(dev, WATCHDOG_REG_LOAD, actual_val);
	result = wdt_clear_intr(dev);
	if (result) {
		SYS_LOG_ERR("clear intr failed");
		return result;
	}

	wdt_get_reg(dev, WATCHDOG_REG_LOAD, &reg_val);
	if (reg_val != actual_val) {
		SYS_LOG_ERR("Set LOAD failed");
		return -1;
	}

	result = wdt_enable(dev);
	if (result) {
		SYS_LOG_ERR("enable failed");
		return result;
	}

	result = wdt_lock(dev);
	if (result) {
		SYS_LOG_ERR("lock failed");
		return result;
	}

	return 0;
}

static int wdt_bcm5820x_feed(struct device *dev)
{
	int result = 0;

	result = wdt_unlock(dev);
	if (result) {
		SYS_LOG_ERR("unlock failed");
		return result;
	}

	result = wdt_disable(dev);
	if (result) {
		SYS_LOG_ERR("disable failed");
		return result;
	}

	result = wdt_clear_intr(dev);
	if (result) {
		SYS_LOG_ERR("clear intr failed");
		return result;
	}

	result = wdt_enable(dev);
	if (result) {
		SYS_LOG_ERR("enable failed");
		return result;
	}

	result = wdt_lock(dev);
	if (result) {
		SYS_LOG_ERR("lock failed");
		return result;
	}

	return 0;
}

static int wdt_bcm5820x_stop(struct device *dev)
{
	int result = 0;

	result = wdt_unlock(dev);
	if (result) {
		SYS_LOG_ERR("unlock failed");
		return result;
	}

	result = wdt_disable(dev);
	if (result) {
		SYS_LOG_ERR("disable failed");
		return result;
	}

	result = wdt_clear_intr(dev);
	if (result) {
		SYS_LOG_ERR("clear intr failed");
		return result;
	}

	result = wdt_lock(dev);
	if (result) {
		SYS_LOG_ERR("lock failed");
		return result;
	}

	return 0;
}

static void wdt_isr_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;

	wdt_unlock(dev);
	wdt_clear_intr(dev);
	wdt_lock(dev);

	if (data->wdt_handler) {
		data->wdt_handler();
	}
}

DEVICE_DECLARE(wdt);
static int wdt_bcm5820x_register_isr(struct device *dev,
				     wdt_isr handler)
{
	const struct wdt_config *cfg = dev->config->config_info;
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;

	data->wdt_handler = handler;
	IRQ_CONNECT(SPI_IRQ(WDT_IRQ), 0, wdt_isr_handler, DEVICE_GET(wdt), 0);
	irq_enable(cfg->irq);

	return 0;
}

static int wdt_bcm5820x_unregister_isr(struct device *dev)
{
	const struct wdt_config *cfg = dev->config->config_info;
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;

	irq_disable(cfg->irq);
	data->wdt_handler = NULL;

	return 0;
}

static int wdt_bcm5820x_reset_en(struct device *dev, bool flag)
{
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;

	data->reset_en = flag;
	wdt_enable(dev);

	return 0;
}

static int wdt_bcm5820x_init(struct device *dev)
{
	struct wdt_data *data = (struct wdt_data *)dev->driver_data;

	data->wdt_handler = NULL;

	data->reset_en = false;

	return 0;
}

static const struct wdt_driver_api wdt_bcm5820x_api = {
	.start = wdt_bcm5820x_start,
	.feed = wdt_bcm5820x_feed,
	.stop = wdt_bcm5820x_stop,
	.register_isr = wdt_bcm5820x_register_isr,
	.unregister_isr = wdt_bcm5820x_unregister_isr,
	.reset_en = wdt_bcm5820x_reset_en,
};

static const struct wdt_config wdt_bcm5820x_cfg = {
	.base = WDT_BASE_ADDR,
	.irq = SPI_IRQ(WDT_IRQ),
};

static struct wdt_data wdt_bcm5820x_data;

DEVICE_AND_API_INIT(wdt, CONFIG_WDT_DEV_NAME, wdt_bcm5820x_init,
		    &wdt_bcm5820x_data, &wdt_bcm5820x_cfg, POST_KERNEL,
		    CONFIG_WDT_DRIVER_INIT_PRIORITY, &wdt_bcm5820x_api);
