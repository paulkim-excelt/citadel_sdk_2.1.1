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
 * @file bbl_bcm5820x.c
 * @brief Citadel BBL Driver
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <errno.h>
#include <kernel.h>
#include <misc/util.h>
#include <stdbool.h>
#include <bbl.h>
#include <logging/sys_log.h>
#include <spru/spru_access.h>

#include "bbl_regs.h"

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

/* Voltage monitor power up timeout in milli seconds */
#define VMON_STATE_TIMEOUT 1000

/* FMON trim calibration timeout in milli seconds */
#define FMON_TRIM_TIMEOUT 10000

#define FMON_STATE_MASK			0x3

#define FMON_STATE_FREQ_HIGH		0
#define FMON_STATE_FREQ_IN_RANGE	1
#define FMON_STATE_NO_STATE		2
#define FMON_STATE_FREQ_LOW		3

/* Dynamic mesh filter thresholds */
#define IMESH_FILTER_THRESHOLD		0x1
#define EMESH_FILTER_THRESHOLD		0x1
#define EMESH_OPEN_FILTER_THRESHOLD	0x1

/* Temperature monitor samples are collected every 4 seconds */
#define BBL_TMON_TEMP_SAMPLE_PERIOD_MS	4000

/* BBRAM Offset */
#define BBRAM_START			0x00000200

/* Calibrate frequency monitor as part of bbl init */
#define CALIBRATE_FMON_TRIM

/* Seconds elapsed since Jan 1st 1970 (epoch) */
#define SECONDS_UNTIL_JAN_1_2018	(((2018-1970)*365 + 12)*24*60*60)

/* BBL device driver data */
struct bbl_dev_data {
	/* Callback function to be called on a tamper event */
	bbl_tamper_detect_cb cb_fn;
};

static void bbl_enable_tamper_intr(void);

/**
 * @brief Default tamper event logger
 */
#define BBL_ISR_LOG(...)	\
	do {	\
		printk("[BBL ISR] ");	\
		printk(__VA_ARGS__);	\
		printk("\n");		\
	} while (0)
static void log_tamper_event(u32_t tamper_events, u32_t time_stamp)
{
	if (tamper_events) {
		BBL_ISR_LOG("Tamper detected at %d seconds !!!", time_stamp);
		BBL_ISR_LOG("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		if (tamper_events & BBL_TAMPER_EVENT_N_0)
			BBL_ISR_LOG("Input tamper n[0] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_N_1)
			BBL_ISR_LOG("Input tamper n[1] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_N_2)
			BBL_ISR_LOG("Input tamper n[2] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_N_3)
			BBL_ISR_LOG("Input tamper n[3] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_N_4)
			BBL_ISR_LOG("Input tamper n[4] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_P_0)
			BBL_ISR_LOG("Input tamper p[0] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_P_1)
			BBL_ISR_LOG("Input tamper p[1] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_P_2)
			BBL_ISR_LOG("Input tamper p[2] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_P_3)
			BBL_ISR_LOG("Input tamper p[3] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_P_4)
			BBL_ISR_LOG("Input tamper p[4] Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_TMON_LOW)
			BBL_ISR_LOG("Temperature below limit Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_TMON_HIGH)
			BBL_ISR_LOG("Temperature too high Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_TMON_ROC)
			BBL_ISR_LOG("Temperature changed too fast Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_VMON_LVD)
			BBL_ISR_LOG("Voltage below threshold Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_VMON_HVD)
			BBL_ISR_LOG("Voltage above threshold Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_FMON_LOW)
			BBL_ISR_LOG("Frequency low Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_FMON_HIGH)
			BBL_ISR_LOG("Frequency high Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_IMESH_FAULT)
			BBL_ISR_LOG("Internal mesh fault Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_EMESH_FAULT)
			BBL_ISR_LOG("External mesh fault Detected!");
		if (tamper_events & BBL_TAMPER_EVENT_EMESH_OPEN)
			BBL_ISR_LOG("External mesh open Detected!");
	}
}

/**
 * @brief ISR routine for DMA
 *
 * @parm arg Void pointer
 *
 * @return void
 */
static void bbl_tamper_isr(void *arg)
{
	int i;
	u32_t time_stamp, tamper_events, src_stat, src_stat1, val, val1, src_en;
	struct device *dev = (struct device *)arg;
	struct bbl_dev_data *dd = (struct bbl_dev_data *)dev->driver_data;

	/* Generate the tamper event mask */
	bbl_read_reg(BBL_TAMPER_SRC_STAT, &src_stat);
	bbl_read_reg(BBL_TAMPER_SRC_STAT_1, &src_stat1);

	/* Report only those events that are enabled */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &src_en);
	val = src_stat & src_en;
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &src_en);
	val1 = src_stat1 & src_en;

	tamper_events = 0;
	/* Digital tamper input events */
	for (i = 0; i < BBL_TAMPER_P_0; i++) {
		if (val & BIT(i))
			tamper_events |= BBL_TAMPER_EVENT_N_0 << i;
		/* [17:9] = Tamper_P[8:0] */
		if (val & BIT(9 + i))
			tamper_events |= BBL_TAMPER_EVENT_P_0 << i;
	}

	/* External mesh faults */
	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Egrid_sts_R))	/* Fault */
		tamper_events |= BBL_TAMPER_EVENT_EMESH_FAULT;

	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Egrid_sts_R + 1))/* Open */
		tamper_events |= BBL_TAMPER_EVENT_EMESH_OPEN;

	/* Internal mesh fault */
	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_IGrid_sts))
		tamper_events |= BBL_TAMPER_EVENT_IMESH_FAULT;

	/* Tmon tamper events */
	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Tmon_sts_R))	/* High delta */
		tamper_events |= BBL_TAMPER_EVENT_TMON_ROC;

	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Tmon_sts_R + 1))	/* Temp low */
		tamper_events |= BBL_TAMPER_EVENT_TMON_LOW;

	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Tmon_sts_R + 2))	/* Temp high */
		tamper_events |= BBL_TAMPER_EVENT_TMON_HIGH;

	/* Vmon tamper events */
	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Vmon_sts))	/* Low voltage*/
		tamper_events |= BBL_TAMPER_EVENT_VMON_LVD;

	if (val1 & BIT(BBL_TAMPER_SRC_STAT_1__hvd_n_tamper_sts))/* Hi voltage*/
		tamper_events |= BBL_TAMPER_EVENT_VMON_HVD;

	/* Fmon tamper events */
	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Fmon_sts_R))	/* Freq low */
		tamper_events |= BBL_TAMPER_EVENT_FMON_LOW;

	if (val & BIT(BBL_TAMPER_SRC_STAT__BBL_Fmon_sts_R + 1))	/* Freq high */
		tamper_events |= BBL_TAMPER_EVENT_FMON_HIGH;

	/* Get the tamper timestamp */
	bbl_read_reg(BBL_TAMPER_TIMESTAMP, &time_stamp);

	/* Clear the sources to prevent a flurry of tamper events
	 * The user callback called will take the necessary action to
	 * re-enable the tamepr source if needed.
	 */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &src_en);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, src_en & ~(val));
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &src_en);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE_1, src_en & ~(val1));

	if (tamper_events) {
		if (dd->cb_fn)
			dd->cb_fn(tamper_events, time_stamp);
		else
			log_tamper_event(tamper_events, time_stamp);
	}

	/* Clear tamper event status */
	bbl_read_reg(BBL_TAMPER_SRC_STAT, &val);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, src_stat);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0);

	bbl_read_reg(BBL_TAMPER_SRC_STAT_1, &val1);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, src_stat1);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0);

	/* Clear the tamper interrupt status bits (all excluding RTC events) */
	bbl_write_reg(BBL_INTERRUPT_clr,
			~(BIT(BBL_INTERRUPT_EN__bbl_match_intr_en) |
			  BIT(BBL_INTERRUPT_EN__bbl_period_intr_en)));
}

/**
 * @brief BBL Power up band gap
 * @details BBL POwer up band gap
 *
 * @param none
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_bg_powerup(void)
{
	u32_t data;

	/* Power up BG */
	bbl_read_reg(BBL_MACRO_CFG, &data);

	if (data & BIT(BBL_MACRO_CFG__bg_pd)) {
		data &= ~BIT(BBL_MACRO_CFG__bg_pd);
		bbl_write_reg(BBL_MACRO_CFG, data);
		k_busy_wait(1000);
	}

	return 0;
}

#ifdef CALIBRATE_FMON_TRIM
/**
 * @brief Calibrate FMON trim value
 * @details This function manually calibrates trim value, and user should apply
 *	    this calibration on each device and program the suggested trim value
 *	    into BBL_MACRO_CFG[fmon_trim] register.
 *
 * 1.  The BBL is programmed only once and stay powered till the device is
 * dead.
 * 2.  During the device init, one would do the manual calibration at the
 * secure facility.
 * 3.  Once the calibration is done, the value is programmed in the reg,
 * and stays.
 *
 * @param trim_value
 *
 * @return status, trim value
 */
static s32_t bbl_calibrate_fmon(u32_t *trim_value)
{
	u32_t start;
	u32_t trim = 0x0;
	u32_t data, macro;
	u32_t curr_fmon_stat = 0x0;
	u32_t last_fmon_stat = 0x0;
	u32_t trim_low_boundary = 0x0;
	u32_t trim_high_boundary = 0x0;

	for (trim = 0; trim <= 0xF; trim++) {
		/* Power up frequency monitor,
		 * Active low reset frequency monitor
		 */
		bbl_read_reg(BBL_MACRO_CFG, &macro);
		macro &= ~(BIT(BBL_MACRO_CFG__fmon_pd) |
			   BIT(BBL_MACRO_CFG__fmon_rstb));
		bbl_write_reg(BBL_MACRO_CFG, macro);
		bbl_read_reg(BBL_MACRO_CFG, &macro);
		macro |= BIT(BBL_MACRO_CFG__fmon_rstb);
		bbl_write_reg(BBL_MACRO_CFG, macro);

		/* set trim value */
		macro &= ~(0xF << BBL_MACRO_CFG__fmon_trim_R);
		macro |= trim << BBL_MACRO_CFG__fmon_trim_R;
		bbl_write_reg(BBL_MACRO_CFG, macro);

		start = k_uptime_get_32();
		do {
			bbl_read_reg(BBL_MACRO_STAT, &data);

			if ((k_uptime_get_32() - start) >= FMON_TRIM_TIMEOUT) {
				*trim_value = 0;
				SYS_LOG_ERR("Fmon calibration timed out!");
				return -ETIMEDOUT;
			}
		} while ((data & FMON_STATE_MASK) == FMON_STATE_NO_STATE);

		curr_fmon_stat = (data & FMON_STATE_MASK);
		if ((curr_fmon_stat == FMON_STATE_FREQ_IN_RANGE) &&
		    (last_fmon_stat == FMON_STATE_FREQ_HIGH))
			/* found low_trim_bounday */
			trim_low_boundary = trim;

		if ((curr_fmon_stat == FMON_STATE_FREQ_LOW) &&
		    (last_fmon_stat == FMON_STATE_FREQ_IN_RANGE))
			/* found high_trim_bounday */
			trim_high_boundary = trim - 1;

		last_fmon_stat = curr_fmon_stat;
	}

	SYS_LOG_DBG("BBL TRIM windows [0x%x:0x%x], suggested trim value:0x%x\n",
		  trim_low_boundary, trim_high_boundary,
		  ((trim_low_boundary + trim_high_boundary) >> 1));

	*trim_value = ((trim_low_boundary + trim_high_boundary) >> 1);

	return 0;
}
#endif

/**
 * @brief Configure frequency monitor
 * @details Configure frequency monitor settings and enable it as a tamper
 *	    source
 *
 * @param tamper_cfg Tamper configuration for frequency monitor
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_configure_fmon(void *tamper_cfg)
{
	u32_t val, start;
	struct bbl_fmon_tamper_config *cfg =
		(struct bbl_fmon_tamper_config *)tamper_cfg;

	if (cfg == NULL) {
		/* Disable fmon tamper sources */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R);
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R + 1);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		return 0;
	}

	/* Power up frequency monitor */
	bbl_read_reg(BBL_MACRO_CFG, &val);
	val &= ~BIT(BBL_MACRO_CFG__fmon_pd);
	bbl_write_reg(BBL_MACRO_CFG, val);
	/* Wait for 1 ms */
	k_busy_wait(1000);

	/* Deassert Active low reset frequency monitor */
	val |= BIT(BBL_MACRO_CFG__fmon_rstb);
	bbl_write_reg(BBL_MACRO_CFG, val);

	/* Wait for fmon status to transition to NO_STATE */
	start = k_uptime_get_32();
	do {
		bbl_read_reg(BBL_MACRO_STAT, &val);

		if ((k_uptime_get_32() - start) >= FMON_TRIM_TIMEOUT)
			return -ETIMEDOUT;
	} while ((val & FMON_STATE_MASK) == FMON_STATE_NO_STATE);

	/* Clear fmon tamper status bits */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_Fmon_Clr_R));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_Fmon_Clr_R + 1));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0x0);

	/* Set the tamper sources */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	if (cfg->low_freq_detect)
		val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R);
	else
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R);

	if (cfg->high_freq_detect)
		val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R + 1);
	else
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Fmon_En_R + 1);

	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	return 0;
}

/**
 * @brief Configure voltage monitor
 * @details Configure voltage monitor settings and enable it as a tamper
 *	    source
 *
 * @param tamper_cfg Tamper configuration for voltage monitor
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_configure_vmon(void *tamper_cfg)
{
	u32_t val, val1, start;
	struct bbl_vmon_tamper_config *cfg =
		(struct bbl_vmon_tamper_config *)tamper_cfg;

	if (cfg == NULL) {
		/* Power down frequency monitor */
		bbl_read_reg(BBL_MACRO_CFG, &val);
		val |= BIT(BBL_MACRO_CFG__vmon_pd);
		bbl_write_reg(BBL_MACRO_CFG, val);

		/* Disable low voltage detect tamper */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Vmon_En);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		/* Disable high voltage detect tamper */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &val1);
		val1 &= ~BIT(BBL_TAMPER_SRC_ENABLE_1__hvd_n_tamper_en);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE_1, val1);
		return 0;
	}

	/* Disable voltage detect tamper before altering the thresholds */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Vmon_En);
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &val1);
	val1 &= ~BIT(BBL_TAMPER_SRC_ENABLE_1__hvd_n_tamper_en);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE_1, val1);

	/* Set voltage thresholds */
	bbl_read_reg(BBL_MACRO_CFG, &val);
	val &= ~(0x3 << BBL_MACRO_CFG__vmon_cmp_t_sel_R);
	val |= (cfg->high_voltage_thresh & 0x3) <<
		BBL_MACRO_CFG__vmon_cmp_t_sel_R;
	val &= ~(0x3 << BBL_MACRO_CFG__vmon_cmp_b_sel_R);
	val |= (cfg->low_voltage_thresh & 0x3) <<
		BBL_MACRO_CFG__vmon_cmp_b_sel_R;
	bbl_write_reg(BBL_MACRO_CFG, val);

	/* Power up voltage monitor */
	bbl_read_reg(BBL_MACRO_CFG, &val);
	val &= ~BIT(BBL_MACRO_CFG__vmon_pd);
	bbl_write_reg(BBL_MACRO_CFG, val);

	/* Code adapted from Lynx driver: There seems to be a wait
	 * for 1 second or vmon_status bits being set (which ever comes
	 * first), before programming the voltage thresholds/source
	 * enable and after powering up the voltage monitor.
	 */
	start = k_uptime_get_32();
	do {
		bbl_read_reg(BBL_TAMPER_SRC_STAT, &val);
		bbl_read_reg(BBL_TAMPER_SRC_STAT_1, &val1);
		if ((val & BIT(BBL_TAMPER_SRC_STAT__BBL_Vmon_sts)) &&
		    (val1 &
		     (BIT(BBL_TAMPER_SRC_STAT_1__vmon_cmp_data_tamper_sts) |
		      BIT(BBL_TAMPER_SRC_STAT_1__hvd_n_tamper_sts)))) {
			break;
		}
	} while ((k_uptime_get_32() - start) < VMON_STATE_TIMEOUT);

	/* Clear Vmon tamper status bits */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_Vmon_Clr));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1,
		BIT(BBL_TAMPER_SRC_CLEAR_1__vmon_cmp_data_tamper_clr));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1,
		BIT(BBL_TAMPER_SRC_CLEAR_1__hvd_n_tamper_clr));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0x0);

	/* Set low voltage detect tamper */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	if (cfg->low_voltage_detect)
		val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_Vmon_En);
	else
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Vmon_En);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	/* Set high voltage detect tamper */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &val1);
	if (cfg->high_voltage_detect)
		val1 |= BIT(BBL_TAMPER_SRC_ENABLE_1__hvd_n_tamper_en);
	else
		val1 &= ~BIT(BBL_TAMPER_SRC_ENABLE_1__hvd_n_tamper_en);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE_1, val1);

	return 0;
}

/**
 * @brief Convert temperature in celcius to tmon adc val.
 * @details Helper function to convert temperature from celcius to adc value
 *	    governed by the equation T = 413.185549 - 0.48334732 x ADC_VAL[9:0]
 *
 * @param temp Temperature in celcius
 *
 * @return 10 bit adc value
 */
static u32_t bbl_temp_to_adc(int temp)
{
	u32_t adc_val;

	if (temp < -81) {
		adc_val = 0x3FF;
	} else if (temp > 413) {
		adc_val = 0x0;
	} else {
		/* Approximation is fine since we are specifying temperature
		 * as an integer value
		 */
		adc_val = (413 - temp)*1000/483;
	}

	return adc_val;
}

/**
 * @brief Convert tmon adc value to temperature in celcius.
 * @details Helper function to convert tmon adc value to temperature from degree
 *	    celcius governed by T = 413.185549 - 0.48334732 x ADC_VAL[9:0]
 *
 * @param adc_val 10 bit ADC Value
 *
 * @return temperature in degree celcius
 */
int bbl_adc_to_temp(u32_t adc_val)
{
	u32_t temp;

	if (adc_val > 1023) {
		temp = -81;
	} else {
		/* Approximation is fine since we are returning temperature
		 * as an integer value
		 */
		temp = 413 - (adc_val*483/1000);
	}

	return temp;
}

/**
 * @brief Configure Temperature monitor
 * @details Configure termperature monitor settings and enable it as a tamper
 *	    source
 *
 * @param tamper_cfg Tamper configuration for temperature monitor
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_configure_tmon(void *tamper_cfg)
{
	u32_t val;
	struct bbl_tmon_tamper_config *cfg =
		(struct bbl_tmon_tamper_config *)tamper_cfg;

	/* Power down temp monitor */
	bbl_read_reg(BBL_TMON_CONFIG, &val);
	val |= BIT(BBL_TMON_CONFIG__temp_pwrdn);
	bbl_write_reg(BBL_TMON_CONFIG, val);

	bbl_read_reg(BBL_MACRO_CFG, &val);
	val &= ~BIT(BBL_MACRO_CFG__tmon_rstb);
	bbl_write_reg(BBL_MACRO_CFG, val);
	k_busy_wait(100);

	/* Power down ADC */
	bbl_read_reg(BBL_MACRO_CFG, &val);
	val |= BIT(BBL_MACRO_CFG__tmon_adc_pd);
	bbl_write_reg(BBL_MACRO_CFG, val);

	if (cfg == NULL) {
		/* Disable Tmon tamper source */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		/* Disable all 3 sources (high, low, rate of change) */
		val &= ~(0x7 << BBL_TAMPER_SRC_ENABLE__BBL_Tmon_En_R);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		return 0;
	}

	/* Disable Tmon tamper sources before changing the limits */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	/* Disable all 3 sources (high, low, rate of change) */
	val &= ~(0x7 << BBL_TAMPER_SRC_ENABLE__BBL_Tmon_En_R);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	/* Set tmon limits */
	val = bbl_temp_to_adc(cfg->lower_limit) << BBL_TMON_CONFIG__min_lim_R |
	      bbl_temp_to_adc(cfg->upper_limit) << BBL_TMON_CONFIG__max_lim_R |
	      bbl_temp_to_adc(cfg->roc) << BBL_TMON_CONFIG__delta_lim_R;
	bbl_write_reg(BBL_TMON_CONFIG, val);

	/* Power up temp monitor */
	bbl_read_reg(BBL_TMON_CONFIG, &val);
	val &= ~BIT(BBL_TMON_CONFIG__temp_pwrdn);
	bbl_write_reg(BBL_TMON_CONFIG, val);

	/* Power up ADC and toggle tmon reset */
	bbl_read_reg(BBL_MACRO_CFG, &val);
	val |= BIT(BBL_MACRO_CFG__tmon_rstb);
	bbl_write_reg(BBL_MACRO_CFG, val);

	val &= ~BIT(BBL_MACRO_CFG__tmon_adc_pd);
	bbl_write_reg(BBL_MACRO_CFG, val);

	/* Clear all pending tmon tamper events (high, low, roc) */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		0x7 << BBL_TAMPER_SRC_CLEAR__BBL_Tmon_Clr_R);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);

	/* Need to wait for one temperature sample collection period
	 * after re-configuring the limits */
	k_sleep(BBL_TMON_TEMP_SAMPLE_PERIOD_MS);

	/* Set Tmon tamper source */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	/* Enable all 3 sources (high, low, rate of change) */
	val |= (0x7 << BBL_TAMPER_SRC_ENABLE__BBL_Tmon_En_R);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	return 0;
}

/**
 * @brief Configure internal mesh
 * @details Configure internal mesh enable it as a tamper source
 *
 * @param tamper_cfg Tamper configuration for internal mesh
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_configure_imesh(void *tamper_cfg)
{
	u32_t val;
	struct bbl_int_mesh_config *cfg =
		(struct bbl_int_mesh_config *)tamper_cfg;

	if ((cfg == NULL) || (cfg->fault_check_enable == false)) {
		/* Disable Imesh fault */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_IGrid_En);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		/* Disable internal mesh fault check */
		bbl_read_reg(BBL_MESH_CONFIG, &val);
		val &= ~BIT(BBL_MESH_CONFIG__en_intmesh_fc);
		bbl_write_reg(BBL_MESH_CONFIG, val);

		return 0;
	}

	/* Set imesh filter threshold. */
	bbl_read_reg(FILTER_THREHOLD_CONFIG1, &val);
	val &= ~0xFF;
	val |= IMESH_FILTER_THRESHOLD;
	bbl_write_reg(FILTER_THREHOLD_CONFIG1, val);

	/* Enable internal mesh fault check and LFSR update */
	bbl_read_reg(BBL_MESH_CONFIG, &val);
	val |= BIT(BBL_MESH_CONFIG__en_intmesh_fc);
	val |= BIT(BBL_MESH_CONFIG__en_mesh_dyn);
	bbl_write_reg(BBL_MESH_CONFIG, val);

	/* Clear any pending imesh tamper events */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_IGrid_Clr));
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);

	/* Enable Imesh tamper source */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_IGrid_En);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	return 0;
}

/**
 * @brief Configure external mesh
 * @details Configure external mesh and/or digital tamper inputs and enable them
 *	    as a tamper source
 *
 * @param tamper_cfg Tamper configuration for digital tamper inputs and external
 *		     mesh
 *
 * @return 0 on Success, -errno otherwise
 */
static int bbl_configure_emesh(void *tamper_cfg)
{
	u32_t val, pin_mask;
	struct bbl_ext_mesh_config *cfg =
		(struct bbl_ext_mesh_config *)tamper_cfg;

	if (cfg == NULL) {
		/* Disable digital tamper input sources */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		/* Clear the 18 bit Tamperpn_En field */
		val &= ~0x3FFFF;
		/* Disable Emesh open fault */
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Egrid_En_R);
		/* Disable Emesh fault */
		val &= ~BIT(BBL_TAMPER_SRC_ENABLE__BBL_Egrid_En_R + 1);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		/* Disable all digital input tampers except P0/N0 pair
		 * Disabling P0/N0 makes the BBRAM inaccessible.
		 */
		bbl_write_reg(BBL_EN_TAMPERIN, 0x1);

		/* Disable all dynamic mesh tamper outputs */
		bbl_read_reg(BBL_EMESH_CONFIG, &val);
		val &= ~0xFF;
		bbl_write_reg(BBL_EMESH_CONFIG, val);

		/* Clear emesh states */
		bbl_read_reg(BBL_EMESH_CONFIG, &val);
		bbl_write_reg(BBL_EMESH_CONFIG,
			val | BIT(BBL_EMESH_CONFIG__bbl_mesh_clr));
		bbl_write_reg(BBL_EMESH_CONFIG,
			val & ~BIT(BBL_EMESH_CONFIG__bbl_mesh_clr));

		return 0;
	}

	/* Clear emesh states */
	bbl_read_reg(BBL_EMESH_CONFIG, &val);
	bbl_write_reg(BBL_EMESH_CONFIG,
		val | BIT(BBL_EMESH_CONFIG__bbl_mesh_clr));
	bbl_write_reg(BBL_EMESH_CONFIG,
		val & ~BIT(BBL_EMESH_CONFIG__bbl_mesh_clr));

	/* Only need to configure tamper inputs in static mode */
	if (cfg->dyn_mesh_enable == false) {
		/* Since tamper inputs can be enabled only in pairs, a pair
		 * needs to be enabled even if one of the inputs needs to be
		 * set as a tamper source. The individual tamper pin control is
		 * set through the BBL_TAMPER_SRC_ENABLE register.
		 */
		pin_mask = cfg->static_pin_mask |
			   (cfg->static_pin_mask >> BBL_TAMPER_P_0);
		/* Enable tamper inputs as per the mask */
		bbl_write_reg(BBL_EN_TAMPERIN, pin_mask & 0x1F);

		/* Configure external mesh as static */
		bbl_read_reg(BBL_EMESH_CONFIG, &val);
		val &= ~BIT(BBL_EMESH_CONFIG__en_extmesh_dyn);
		/* Clear all en_extmesh for all input pairs */
		val &= ~0xFF;
		bbl_write_reg(BBL_EMESH_CONFIG, val);

		/* Clear any pending tamper sources */
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x3FFFF); /* P/N[0:4]*/
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);

		/* Enable digital tamper input source */
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
		/* Clear the 18 bit Tamperpn_En field */
		val &= ~0x3FFFF;
		val |= (cfg->static_pin_mask & 0x1F) | /* Tamper N[0:4] */
		       /* Tamper P[0:4] */
		       ((cfg->static_pin_mask >> BBL_TAMPER_P_0) << 9);
		bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

		return 0;
	}

	/* Dynamic mesh enabled case */
	/* Configure P[0] and N[0] if enabled */
	pin_mask = cfg->static_pin_mask |
		   (cfg->static_pin_mask >> BBL_TAMPER_P_0);
	/* Enable tamper inputs as per the mask - Only P[0]/N[0] can be
	 * configured as static tamper inputs, as others are being used with
	 * dynamic mesh mode
	 */
	bbl_write_reg(BBL_EN_TAMPERIN, pin_mask & 0x1);

	/* Configure external mesh as dynamic */
	bbl_read_reg(BBL_EMESH_CONFIG, &val);
	val |= BIT(BBL_EMESH_CONFIG__en_extmesh_dyn); /* Enable dynamic mode */
	/* Configure en_extmesh for the dynamic mesh */
	val &= ~0xFF;
	val |= cfg->dyn_mesh_mask;
	bbl_write_reg(BBL_EMESH_CONFIG, val);

	/* Enable LFSR update */
	bbl_read_reg(BBL_MESH_CONFIG, &val);
	bbl_write_reg(BBL_MESH_CONFIG, val | BIT(BBL_MESH_CONFIG__en_mesh_dyn));

	/* Enable noise filter and set its threshold.
	 * (Disable_emesh_filter=0, emesh_fil_sel=0x1f)
	 */
	bbl_write_reg(BBL_EMESH_CONFIG_1,
		0x1F << BBL_EMESH_CONFIG_1__emesh_fil_sel_R);

	/* Set emesh threshold. */
	bbl_read_reg(FILTER_THREHOLD_CONFIG1, &val);
	val &= ~0x00FFFF00;
	val |= EMESH_FILTER_THRESHOLD << 8;
	val |= EMESH_OPEN_FILTER_THRESHOLD << 16;
	bbl_write_reg(FILTER_THREHOLD_CONFIG1, val);

	/* Enable fault check */
	bbl_read_reg(BBL_EMESH_CONFIG, &val);
	val |= BIT(BBL_EMESH_CONFIG__en_extmesh_fc);
	bbl_write_reg(BBL_EMESH_CONFIG, val);

	/* Clear any pending emesh tamper events */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_Egrid_Clr_R)); /* Fault */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR,
		BIT(BBL_TAMPER_SRC_CLEAR__BBL_Egrid_Clr_R + 1)); /* Open */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0x0);

	/* Enable tamper sources: p[0]/n[0] (static) and ext mesh */
	bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &val);
	val &= ~0x3FFFF;
	val |= (cfg->static_pin_mask & 0x1) | /* Tamper N[0] */
	       /* Tamper P[0] */
	       ((cfg->static_pin_mask & BIT(BBL_TAMPER_P_0)) << 9);
	val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_Egrid_En_R);
	val |= BIT(BBL_TAMPER_SRC_ENABLE__BBL_Egrid_En_R + 1);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, val);

	return 0;
}


/* BBL initialization routine */
static int bbl_init(struct device *dev)
{
	int ret = 0;
	u32_t data;

	ARG_UNUSED(dev);

	/* Check if this is a BBL POR by reading the rtc time at
	 * system start. If the start time is < Jan 1 2018 (Time
	 * before this was developed), then it is a BBL POR, since
	 * the RTC driver initializes the build time into the
	 * RTC_SECOND register if the RTC time is less than the
	 * build time.
	 */
	data = spru_get_rtc_time_at_startup();
	if (data > SECONDS_UNTIL_JAN_1_2018) {
		/* Skip BBL init as BBL has already been initialized
		 * in a previous boot up.
		 */
		SYS_LOG_DBG("Skipping BBL initialization!");

		/* Install tamper interrupt */
		bbl_enable_tamper_intr();
		return 0;
	}

	SYS_LOG_DBG("Initializing Battery Backed Logic Driver");

	/* Enable BBL access first */
	ret = bbl_access_enable();
	CHECK_AND_RETURN(ret);

	/* Pull down for all the tamper Lines */
	bbl_read_reg(PAD_PULL_DN_CFG, &data);
	/* 10 tamper lines Bits[9:0] */
	data &= ~(0x3FF);
	/* Pull down setting is inverted for P0 and N0 */
	data |= BIT(BBL_TAMPER_N_0) | BIT(BBL_TAMPER_P_0);
	bbl_write_reg(PAD_PULL_DN_CFG, data);

	/* Reset SPRU Interface */
	spru_interface_reset(0);

	/* init alarm polarity and CMOS output */
	bbl_read_reg(BBL_CONFIG, &data);
	data |= BIT(BBL_CONFIG__bbl_alarm_cmos)|BIT(BBL_CONFIG__bbl_alarm_pol);
	bbl_write_reg(BBL_CONFIG, data);

	/* Power up band gap */
	bbl_bg_powerup();

#ifdef CALIBRATE_FMON_TRIM
	{
		u32_t trim;

		/* Calibrate frequency monitor trim */
		ret = bbl_calibrate_fmon(&trim);
		if (ret) {
			SYS_LOG_ERR("FMON trim calibration failed!");
			return ret;
		}

		trim &= 0xF;
		bbl_read_reg(BBL_MACRO_CFG, &data);
		data &= ~(0xF << BBL_MACRO_CFG__fmon_trim_R);
		data |= (trim << BBL_MACRO_CFG__fmon_trim_R);
		bbl_write_reg(BBL_MACRO_CFG, data);
	}
#endif /* CALIBRATE_FMON_TRIM */

	/* Set Deglitch filter threshold for tamper inputs */
	bbl_write_reg(FILTER_THREHOLD_CONFIG2, 0x7);
	bbl_write_reg(TAMPER_INP_TIMEBASE, 0xff);

	/* Clear imesh tamper simulation */
	bbl_write_reg(DBG_CONFIG, 0x0);

	/* Clear all tamper source status and interrupts */
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0xFFFFFFFF);
	bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0xFFFFFFFF);
	bbl_write_reg(BBL_INTERRUPT_clr, 0xFFFFFFFF);

	/* Clear all sources */
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE, 0x0);
	bbl_write_reg(BBL_TAMPER_SRC_ENABLE_1, 0x0);

	/* Configure tamper out signal on the pad (default is BBL clock) */
	bbl_read_reg(PAD_SRC_CFG, &data);
	data &= ~BIT(PAD_SRC_CFG__bbl_clk_on_alarm_out);
	data &= ~BIT(PAD_SRC_CFG__sticky_emesh_tamper);
	bbl_write_reg(PAD_SRC_CFG, data);

	/* Clear any pending interupts */
	bbl_write_reg(BBL_INTERRUPT_clr,
		      BIT(BBL_INTERRUPT_EN__bbl_tampern_intr_en));

	/* Set the interrupt enable bit */
	bbl_read_reg(BBL_INTERRUPT_EN, &data);
	data |= BIT(BBL_INTERRUPT_EN__bbl_tampern_intr_en);
	bbl_write_reg(BBL_INTERRUPT_EN, data);

	/* Install tamper interrupt */
	bbl_enable_tamper_intr();

	return ret;
}

/**
 * @brief       Configure tamper detection
 * @details     This api configures the BBL tamper detection hardware based
 *		on the configruation parameters rovided
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to BBL tamper configiuration structure.
 *			 Setting config->tamper_cfg to NULL, causes the
 *			 tamper source indicated by config->tamper_type to be
 *			 disabled.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_tamper_detect_configure(
	struct device *dev,
	struct bbl_tamper_config *config)
{
	int ret;

	ARG_UNUSED(dev);

	if (config == NULL)
		return -EINVAL;

	switch (config->tamper_type) {
	case BBL_TAMPER_DETECT_FREQUENCY_MONITOR:
		ret = bbl_configure_fmon(config->tamper_cfg);
		break;
	case BBL_TAMPER_DETECT_VOLTAGE_MONITOR:
		ret = bbl_configure_vmon(config->tamper_cfg);
		break;
	case BBL_TAMPER_DETECT_TEMP_MONITOR:
		ret = bbl_configure_tmon(config->tamper_cfg);
		break;
	case BBL_TAMPER_DETECT_INTERNAL_MESH:
		ret = bbl_configure_imesh(config->tamper_cfg);
		break;
	case BBL_TAMPER_DETECT_EXTERNAL_MESH:
		ret = bbl_configure_emesh(config->tamper_cfg);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * @brief       Sets the LFSR parameters
 * @details     Sets the LFSR parameters used by internal and external meshes
 *		in dynamic mode
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cfg - Pointer to LFSR params structure
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_set_lfsr_params(
		struct device *dev,
		struct bbl_lfsr_params *cfg)
{
	int ret;
	u32_t data;

	ARG_UNUSED(dev);

	if (cfg == NULL)
		return -EINVAL;

	/* Set LFSR seed */
	ret = bbl_write_reg(BBL_LFSR_SEED, cfg->lfsr_seed);
	CHECK_AND_RETURN(ret);

	/* Set LFSR period and dynamic mesh setting */
	ret = bbl_read_reg(BBL_MESH_CONFIG, &data);
	CHECK_AND_RETURN(ret);

	data &= ~BBL_LFSR_PERIOD_MASK;
	data |= (cfg->lfsr_period & BBL_LFSR_PERIOD_MASK);

	if (cfg->mfm_enable)
		data |= BIT(BBL_MESH_CONFIG__mesh_mfm_en);
	else
		data &= ~BIT(BBL_MESH_CONFIG__mesh_mfm_en);

	if (cfg->lfsr_update)
		data |= BIT(BBL_MESH_CONFIG__en_mesh_dyn);
	else
		data &= ~BIT(BBL_MESH_CONFIG__en_mesh_dyn);

	ret = bbl_write_reg(BBL_MESH_CONFIG, data);
	CHECK_AND_RETURN(ret);

	return 0;
}

/**
 * @brief       Get tamper pin state
 * @details     Get the state of the digital tamper input pin
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   pin - Tamper input pin for which the state is required. Use
 *		      BBL_TAMPER_[P|N]_X macros to specify the pin.
 * @param[out]  state - 0 if the pin is pulled low, 1 if the pin is pulled high
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_get_tamper_pin_state(
		struct device *dev,
		u8_t pin,
		u8_t *state)
{
	int ret;
	u32_t data, bit_pos;

	ARG_UNUSED(dev);

	/* Param check */
	if ((state == NULL) || (pin > BBL_TAMPER_P_4))
		return -EINVAL;

	/* Get input pin state */
	ret = bbl_read_reg(BBL_INPUT_STATUS, &data);
	CHECK_AND_RETURN(ret);

	/* The bit positions are
	 * [17:14] Unused
	 * [13: 9] Tamper P[4:0]
	 * [ 8: 5] Unused
	 * [ 4: 0] Tamper N[4:0]
	 */
	bit_pos = pin < BBL_TAMPER_P_0 ? pin : pin + 4;

	*state = (data >> bit_pos) & 0x1;

	return 0;
}

/**
 * @brief       Configure BBRAM
 * @details     Set the BBRAM bit flip settings
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to the BBRAM configuration structure
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_bbram_configure(
		struct device *dev,
		struct bbl_bbram_config *config)
{
	int ret;
	u32_t val;

	ARG_UNUSED(dev);

	if (config == NULL)
		return -EINVAL;

	ret = bbl_read_reg(BBL_CONFIG, &val);
	CHECK_AND_RETURN(ret);

	if (config->bit_flip_enable) {
		/* Set bit flip enable bit */
		val |= BIT(BBL_CONFIG__bitflip_en);

		/* Set bit flip period */
		val &= ~(BIT(12) | BIT(11));
		config->bit_flip_rate &= BBL_BBRAM_BFR_MASK;
		val |= config->bit_flip_rate << BBL_CONFIG__bitflip_period_R;
	} else {
		val &= ~BIT(BBL_CONFIG__bitflip_en);
	}
	ret = bbl_write_reg(BBL_CONFIG, val);
	CHECK_AND_RETURN(ret);

	return 0;
}

/**
 * @brief       Read the BBRAM contents
 * @details     Read the 1280 bit secure BBRAM memory contents
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   offset - Word offset into the BBRAM memory (0 - 39)
 * @param[out]  buffer - Buffer pointer to which the BBRAM contents should be
 *			 written.
 * @param[in]   length - Length of the data to be read in words. length + offset
 *		should be less than 40.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_bbram_read(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length)
{
	int ret;
	u32_t i;

	ARG_UNUSED(dev);

	/* Check params */
	if ((offset + length) > BBRAM_NUM_WORDS)
		return -EINVAL;

	if ((buffer == NULL) || (length == 0))
		return -EINVAL;

	/* Read BBRAM contents */
	for (i = 0; i < length; i++) {
		ret = bbl_read_mem(BBRAM_START + (offset + i)*4, &buffer[i]);
		CHECK_AND_RETURN(ret);
	}

	return 0;
}

/**
 * @brief       Write to BBRAM
 * @details     Write to the 1280 bit secure BBRAM memory
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   offset - Word offset into the BBRAM memory (0 - 39)
 * @param[out]  buffer - Pointer to buffer with the data to be written.
 * @param[in]   length - Length of the data to be read in words. length + offset
 *		should be less than 40.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_bbram_write(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length)
{
	int ret;
	u32_t val, i;

	ARG_UNUSED(dev);

	/* Params */
	if ((offset + length) > BBRAM_NUM_WORDS)
		return -EINVAL;

	if ((buffer == NULL) || (length == 0))
		return -EINVAL;

	/* Check access */
	ret = bbl_read_reg(BBL_WR_BLOCK, &val);
	CHECK_AND_RETURN(ret);

	if (val & 0x1) {
		/* Write blocked */
		return -EPERM;
	}

	/* Write to BBRAM */
	for (i = 0; i < length; i++) {
		ret = bbl_write_mem(BBRAM_START + (offset + i)*4, buffer[i]);
		CHECK_AND_RETURN(ret);
	}

	return 0;
}

/**
 * @brief       Clear the BBRAM contents
 * @details     Erase the 1280 bit secure BBRAM memory contents
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_bbram_erase(struct device *dev)
{
	int ret;
	u32_t val;

	ARG_UNUSED(dev);

	/* Set sec1280_clr bit in BBL_CONFIG1 */
	ret = bbl_read_reg(BBL_CONFIG_1, &val);
	CHECK_AND_RETURN(ret);

	val |= BIT(BBL_CONFIG_1__sec1280_clr);
	ret = bbl_write_reg(BBL_CONFIG_1, val);
	CHECK_AND_RETURN(ret);

	return 0;
}

/**
 * @brief	Set BBRAM write access
 * @details     Configure BBRAM as read-only or writable
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   enable_write - Set to true for read-write access and false for
 *		read-only access
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_bbram_set_access(struct device *dev, bool enable_write)
{
	int ret;
	u32_t val;

	ARG_UNUSED(dev);

	/* Get access */
	ret = bbl_read_reg(BBL_WR_BLOCK, &val);
	CHECK_AND_RETURN(ret);

	if (enable_write)
		val &= ~0x1;
	else
		val |= 0x1;

	/* Set access */
	ret = bbl_write_reg(BBL_WR_BLOCK, val);
	CHECK_AND_RETURN(ret);

	return 0;
}

/**
 * @brief       Register tamper event callback
 * @details     API to register for a tamper event callback. The tamper event
 *		mask is passed to the callback to indicate which tamper sources
 *		triggered the tamper event
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cb - Callback function to be called on a tamper event
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static int bbl_bcm5820x_register_tamper_detect_cb(
		struct device *dev,
		bbl_tamper_detect_cb cb)
{
	struct bbl_dev_data *dd = (struct bbl_dev_data *)dev->driver_data;

	dd->cb_fn = cb;
	return 0;
}

static struct bbl_dev_data bbl_data = {
	.cb_fn = NULL
};

static const struct bbl_driver_api bbl_api = {
	.td_config = bbl_bcm5820x_tamper_detect_configure,
	.set_lfsr_params = bbl_bcm5820x_set_lfsr_params,
	.get_tamper_pin_state = bbl_bcm5820x_get_tamper_pin_state,
	.bbram_config = bbl_bcm5820x_bbram_configure,
	.bbram_read = bbl_bcm5820x_bbram_read,
	.bbram_write = bbl_bcm5820x_bbram_write,
	.bbram_clear = bbl_bcm5820x_bbram_erase,
	.bbram_set_access = bbl_bcm5820x_bbram_set_access,
	.register_td_cb = bbl_bcm5820x_register_tamper_detect_cb
};

DEVICE_AND_API_INIT(bbl, CONFIG_BBL_DEV_NAME,
		    bbl_init, &bbl_data,
		    NULL, PRE_KERNEL_1,
		    CONFIG_BBL_INIT_PRIORITY, &bbl_api);

static void bbl_enable_tamper_intr(void)
{
	IRQ_CONNECT(SPI_IRQ(BBL_TAMPER_EVENT_INTR), 0,
		bbl_tamper_isr, DEVICE_GET(bbl), 0);
	irq_enable(SPI_IRQ(BBL_TAMPER_EVENT_INTR));
}
