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
 * @file rtc_bcm5820x.c
 * @brief Citadel BBL RTC Driver
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <errno.h>
#include <kernel.h>
#include <misc/util.h>
#include <stdbool.h>
#include <broadcom/rtc.h>
#include <logging/sys_log.h>
#include <spru/spru_access.h>

/* RTC register addresses */
#define BBL_RTC_PER		0x00
#define BBL_RTC_MATCH		0x04
#define BBL_RTC_DIV		0x08
#define BBL_RTC_SECOND		0x0C

/* RTC interrupt registers */
#define BBL_INTERRUPT_EN	0x10
#define BBL_INTERRUPT_STAT	0x14
#define BBL_INTERRUPT_CLR	0x18

#define BBL_PERIODIC_INTR_BIT	0x0

/* RTC Stop/start register */
#define BBL_CONTROL		0x1C

/* RTC period values */
#define BBL_RTC_PERIOD_125ms	0x00000001
#define BBL_RTC_PERIOD_250ms	0x00000002
#define BBL_RTC_PERIOD_500ms	0x00000004
#define BBL_RTC_PERIOD_1s	0x00000008
#define BBL_RTC_PERIOD_2s	0x00000010
#define BBL_RTC_PERIOD_4s	0x00000020
#define BBL_RTC_PERIOD_8s	0x00000040
#define BBL_RTC_PERIOD_16s	0x00000080
#define BBL_RTC_PERIOD_32s	0x00000100
#define BBL_RTC_PERIOD_64s	0x00000200
#define BBL_RTC_PERIOD_128s	0x00000400
#define BBL_RTC_PERIOD_256s	0x00000800

/* Error check and return */
#define CHECK_AND_RETURN(RET)		\
	do {				\
		if (RET)		\
			return RET;	\
	} while (0)

/* Error check and log */
#define CHECK_AND_LOG_ERR(RET)		\
	do {				\
		if (RET) {		\
			SYS_LOG_ERR("%s returned : %d", __func__, RET); \
		}			\
	} while (0)

/* *RTL configuration structure */
struct bbl_rtc_dev_cfg {
	/* RTC start value */
	const char *init_val;
};

/* RTC device driver data */
struct bbl_rtc_dev_data {
	/*!< enable/disable alarm  */
	u8_t alarm_enable;
	/*!< initial configuration value for the 32bit RTC alarm value  */
	u32_t alarm_val;
	/*!< Pointer to function to call when alarm value
	 * matches current RTC value
	 */
	void (*cb_fn)(struct device *dev);
};

/* RTC periodic interrupt service routine (fires every one second) */
static void bbl_rtc_per_isr(void *arg)
{
	int ret;
	u32_t now;
	struct device *dev = (struct device *)arg;
	struct bbl_rtc_dev_data *dd =
		(struct bbl_rtc_dev_data *)dev->driver_data;

	/* Check if an alarm expired and all the callback function */
	if (dd->alarm_enable) {
		ret = bbl_read_reg(BBL_RTC_SECOND, &now);

		if (ret == 0) {
			if (dd->alarm_val <= now) {
				dd->cb_fn(dev);
				dd->alarm_enable = false;
			}
		}
	}

	/* Clear the interrupt */
	bbl_write_reg(BBL_INTERRUPT_CLR, BIT(BBL_PERIODIC_INTR_BIT));
}

/* Helper function to return time in seconds since Jan 1st 2018 from a string
 * with the format "Jun 1 2018 10:20:03"
 */
#define C2I(X)	(X == ' ' ? 0 : X - '0') /* Character to int : Space = 0 */

#define BUILD_YEAR(STR)	(((C2I(STR[7])*10 + C2I(STR[8]))*10 + \
			  C2I(STR[9]))*10 + C2I(STR[10]))
#define BUILD_DAY(STR)	((C2I(STR[4])*10 + C2I(STR[5])) - 1)
#define BUILD_MONTH(STR) \
	((STR[0] == 'J' && STR[1] == 'a' && STR[2] == 'n') ? 0 : \
	(STR[0] == 'F') ? 1 : \
	(STR[0] == 'M' && STR[1] == 'a' && STR[2] == 'r') ? 2 : \
	(STR[0] == 'A' && STR[1] == 'p') ? 3 : \
	(STR[0] == 'M' && STR[1] == 'a' && STR[2] == 'y') ? 4 : \
	(STR[0] == 'J' && STR[1] == 'u' && STR[2] == 'n') ? 5 : \
	(STR[0] == 'J' && STR[1] == 'u' && STR[2] == 'l') ? 6 : \
	(STR[0] == 'A' && STR[1] == 'u') ? 7 : \
	(STR[0] == 'S') ? 8 : \
	(STR[0] == 'O') ? 9 : \
	(STR[0] == 'N') ? 10 : 11)

#define BUILD_HOUR(STR)	(C2I(STR[12])*10 + C2I(STR[13]))
#define BUILD_MIN(STR)	(C2I(STR[15])*10 + C2I(STR[16]))
#define BUILD_SEC(STR)	(C2I(STR[18])*10 + C2I(STR[19]))

#define SECONDS_SINCE_EPOCH(STR)	\
	(((BUILD_YEAR(STR) - 1970)*365 + BUILD_MONTH(STR)*30 +	\
		BUILD_DAY(STR))*24*60*60 +			\
	 ((BUILD_HOUR(STR)*60 + BUILD_MIN(STR))*60 + BUILD_SEC(STR)))

static u32_t get_time_from_str(const char *str)
{
	return SECONDS_SINCE_EPOCH(str);
}

static void bbl_rtc_enable_per_intr(void);

/* RTC initialization routine */
static int bbl_rtc_init(struct device *dev)
{
	int ret;
	u32_t build_time, now;
	const struct bbl_rtc_dev_cfg *cfg = dev->config->config_info;

	/* Enable BBL access first */
	ret = bbl_access_enable();
	CHECK_AND_RETURN(ret);

	/* Set RTC period to 1 second */
	ret = bbl_write_reg(BBL_RTC_PER, BBL_RTC_PERIOD_1s);
	CHECK_AND_RETURN(ret);

	/* Enable RTC interrupt */
	bbl_rtc_enable_per_intr();

	/* Check if the build time is greater that the current time
	 * and update the clock
	 */
	build_time = get_time_from_str(cfg->init_val);
	ret = bbl_read_reg(BBL_RTC_SECOND, &now);
	CHECK_AND_RETURN(ret);

	if (build_time > now) {
		SYS_LOG_DBG("Progamming build time to rtc: %u\n", build_time);
		/* Stop the clock */
		ret = bbl_write_reg(BBL_CONTROL, 0x1);
		CHECK_AND_RETURN(ret);
		/* Program RTC */
		ret = bbl_write_reg(BBL_RTC_SECOND, build_time);
		CHECK_AND_RETURN(ret);
		/* Start the clock */
		ret = bbl_write_reg(BBL_CONTROL, 0x0);
		CHECK_AND_RETURN(ret);
	}

	return 0;
}

/**
 * @brief Function to start RTC
 *
 * This function starts the RTC clock. The clock time will resume from what it
 * was configured with bbl_rtc_set_config() or from the when it was stopped
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return none
 */
static void bbl_rtc_enable(struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);
	ret = bbl_write_reg(BBL_CONTROL, 0x0);
	CHECK_AND_LOG_ERR(ret);
}

/**
 * @brief Function to stop RTC
 *
 * This function starts the RTC clock. Any pending alarms will not trigger
 * until the RTC is started again.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return none
 */
static void bbl_rtc_disable(struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);
	ret = bbl_write_reg(BBL_CONTROL, 0x1);
	CHECK_AND_LOG_ERR(ret);
}

/**
 * @brief Function to Set RTC configuration
 *
 * This function configures the RTC clock as per the specified configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the RTC configuration structure.
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_rtc_set_config(struct device *dev, struct rtc_config *config)
{
	int ret;
	struct bbl_rtc_dev_data *dd = dev->driver_data;

	if (config == NULL)
		return -EINVAL;

	/* Set init val */
	/* Stop the clock */
	ret = bbl_write_reg(BBL_CONTROL, 0x1);
	CHECK_AND_RETURN(ret);
	/* Program RTC */
	ret = bbl_write_reg(BBL_RTC_SECOND, config->init_val);
	CHECK_AND_RETURN(ret);
	/* Start the clock */
	ret = bbl_write_reg(BBL_CONTROL, 0x0);
	CHECK_AND_RETURN(ret);

	/* Install alarm callback */
	if (config->alarm_enable) {
		dd->alarm_enable = true;
		dd->alarm_val = config->alarm_val;
		dd->cb_fn = config->cb_fn;
	} else {
		dd->alarm_enable = false;
		dd->cb_fn = NULL;
	}

	return 0;
}

/**
 * @brief Function to Set RTC alarm
 *
 * This function sets the RTC alarm. When the alarm expires, the callback
 * function configured with set_config is called.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param alarm_val Alarm value in seconds. The alarm expires when the RTC
 *		    clock value matches the alarm. If the alarm value is
 *		    smaller than current clock value, then the callback is
 *		    immediately called.
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_rtc_set_alarm(struct device *dev, const u32_t alarm_val)
{
	int ret;
	u32_t now;
	struct bbl_rtc_dev_data *dd = dev->driver_data;

	ret = bbl_read_reg(BBL_RTC_SECOND, &now);
	CHECK_AND_RETURN(ret);

	if (dd->cb_fn) {
		if (now >= alarm_val) {
			/* Alarm already expired, call callback immediately */
			dd->cb_fn(dev);
			dd->alarm_enable = false;
		} else {
			/* Install alarm callback */
			dd->alarm_enable = true;
			dd->alarm_val = alarm_val;
		}
	} else {
		SYS_LOG_WRN("Set alarm called, but no callback function set!");
	}

	return 0;
}

/**
 * @brief Function to return the RTC value
 *
 * This function sets the current RTC value in seconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return RTC timer value
 */
static u32_t bbl_rtc_read(struct device *dev)
{
	int ret;
	u32_t now;

	ARG_UNUSED(dev);

	ret = bbl_read_reg(BBL_RTC_SECOND, &now);
	if (ret) {
		SYS_LOG_WRN("Error reading RTC!");
		return 0;
	}

	return now;
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 1 if the rtc interrupt is pending.
 * @retval 0 if no rtc interrupt is pending.
 */
static u32_t bbl_rtc_get_pending_int(struct device *dev)
{
	ARG_UNUSED(dev);

	/* This function is not supported with the BBL RTC
	 * as wakeup from low power state is managed by the
	 * PM driver. So this function always returns a 0
	 * to indicate not rtc interrupt is pending
	 */
	return 0;
}

static struct bbl_rtc_dev_cfg bbl_rtc_cfg = {
	.init_val = __DATE__ " " __TIME__
};

static struct bbl_rtc_dev_data bbl_rtc_data = {
	.alarm_enable = false,
	.alarm_val = 0,
	.cb_fn = NULL
};

static const struct rtc_driver_api bbl_rtc_api = {
	.enable = bbl_rtc_enable,
	.disable = bbl_rtc_disable,
	.read = bbl_rtc_read,
	.set_config = bbl_rtc_set_config,
	.set_alarm = bbl_rtc_set_alarm,
	.get_pending_int = bbl_rtc_get_pending_int
};

DEVICE_AND_API_INIT(rtc, CONFIG_RTC_DEV_NAME,
		    bbl_rtc_init, &bbl_rtc_data,
		    &bbl_rtc_cfg, PRE_KERNEL_1,
		    CONFIG_RTC_INIT_PRIORITY, &bbl_rtc_api);

static void bbl_rtc_enable_per_intr(void)
{
	u32_t val;

	/* Clear any pending interupts */
	bbl_write_reg(BBL_INTERRUPT_CLR, BIT(BBL_PERIODIC_INTR_BIT));

	/* Set the interrupt enable bit */
	bbl_read_reg(BBL_INTERRUPT_EN, &val);
	val |= BIT(BBL_PERIODIC_INTR_BIT);
	bbl_write_reg(BBL_INTERRUPT_EN, val);

	IRQ_CONNECT(SPI_IRQ(RTC_PERIODIC_INTERRUPT), 0,
		bbl_rtc_per_isr, DEVICE_GET(rtc), 0);
	irq_enable(SPI_IRQ(RTC_PERIODIC_INTERRUPT));
}
