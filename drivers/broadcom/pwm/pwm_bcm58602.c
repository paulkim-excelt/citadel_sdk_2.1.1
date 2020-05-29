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
 * @file pwm_bcm58602.c
 * @brief bcm58602 pwm driver implementation
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/dma.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <limits.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <sys_clock.h>
#include <zephyr/types.h>
#include <broadcom/pwm.h>
#include <stdbool.h>

#define PWM_CONTROL_OFFSET			(0x00000000)
#define PWM_CONTROL_SMOOTH_SHIFT(chan)		(24 + (chan))
#define PWM_CONTROL_TYPE_SHIFT(chan)		(16 + (chan))
#define PWM_CONTROL_POLARITY_SHIFT(chan)	(8 + (chan))
#define PWM_CONTROL_TRIGGER_SHIFT(chan)		(chan)

#define PRESCALE_OFFSET				(0x00000004)
#define PRESCALE_SHIFT(chan)			((chan) << 2)
#define PRESCALE_MASK(chan)			(0x7 << PRESCALE_SHIFT(chan))
#define PRESCALE_MIN				(0x00000000)
#define PRESCALE_MAX				(0x00000007)

#define PERIOD_COUNT_OFFSET(chan)		(0x00000008 + ((chan) << 3))
#define PERIOD_COUNT_MIN			(0x00000002)
#define PERIOD_COUNT_MAX			(0x00ffffff)

#define DUTY_CYCLE_HIGH_OFFSET(chan)		(0x0000000c + ((chan) << 3))
#define DUTY_CYCLE_HIGH_MIN			(0x00000000)
#define DUTY_CYCLE_HIGH_MAX			(0x00ffffff)

#define PWM_SHUTDOWN				(0x000000c0)

#define ONE_PULSE_DIV_CONTROL(chan)		(0x00000100 + ((chan) << 5))
#define ONE_PULSE_DIV_CONTROL_MASK		(0x3ff)
#define ONE_PULSE_DIV_CONTROL_MAX		(512)

#define ONE_PULSE_COUNTER_CONTROL(chan)		(0x00000104 + ((chan) << 5))
#define ONE_PULSE_COUNTER_CONTROL_MASK		(0xffff)

#define ONE_PULSE_PULSE_TIME_CONTROL(chan)	(0x00000108 + ((chan) << 5))
#define ONE_PULSE_PULSE_TIME_CONTROL_MASK	(0xffff)
#define ONE_PULSE_SAFETY_CONTROL(chan)		(0x0000010c + ((chan) << 5))

#define ONE_PULSE_STATUS(chan)			(0x00000110 + ((chan) << 5))
#define ONE_PULSE_STATUS_END_PULSE		(0)

#define ONE_PULSE_CTRL0_CONTROL(chan)		(0x00000114 + ((chan) << 5))
#define ONE_PULSE_CTRL0_CONTROL_FORCE_END	(3)
#define ONE_PULSE_CTRL0_CONTROL_SOP		(2)

#define ONE_PULSE_CTRL1_CONTROL(chan)		(0x00000118 + ((chan) << 5))
#define ONE_PULSE_CTRL1_CONTROL_POLARITY	(1)
#define ONE_PULSE_CTRL1_CONTROL_ENABLE		(0)

#define PRESCALE_SHIFT_IN_CYCLES		(24)
#define ONE_PULSE_PULSE_DIV_SHIFT		(16)
#define ONE_PULSE_DIV_BITS			(10)

#define PWM_CLK_DIV_HIGH_SHIFT			(16)
#define CLK_DIV_ENABLE_BIT			(31)
#define PWM_CLK_DIV_MASK			(0x3ff)

#define PWM_NUM_PORTS				(4)

struct pwm_bcm58202_config {
	u32_t base;
	u32_t num_ports;
};

/**
 * Set the period and the pulse of PWM.
 *
 * @param dev Device struct
 * @param pwm PWM pin to set
 * @param period_cycles Period in clock cycles of the pwm.
 * @param pulse_cycles PWM width in clock cycles
 * @param polarity Polarity of the output 1 - Active high
 *
 * @return 0 for success, error otherwise
 */
static int pwm_bcm58202_pin_set(struct device *dev, u32_t pwm,
				u32_t period_cycles, u32_t pulse_cycles,
				u32_t polarity)
{
	const struct pwm_bcm58202_config *cfg = dev->config->config_info;
	u32_t period_cnt, duty_cnt;
	u32_t prescale;
	u32_t val;

	/* make sure the PWM port exists */
	if (pwm >= cfg->num_ports)
		return -EIO;

	if ((pulse_cycles > period_cycles) || (polarity > 1))
		return -EINVAL;

	if ((pulse_cycles == 0) || (period_cycles == 0)) {
		/* Disable PWM */
		sys_clear_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_SMOOTH_SHIFT(pwm));
		sys_clear_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_TRIGGER_SHIFT(pwm));
		val = sys_read32(cfg->base + PRESCALE_OFFSET);
		val &= ~PRESCALE_MASK(pwm);
		sys_write32(val, cfg->base + PRESCALE_OFFSET);
		sys_write32(0, cfg->base + PERIOD_COUNT_OFFSET(pwm));
		sys_write32(0, cfg->base + DUTY_CYCLE_HIGH_OFFSET(pwm));
		sys_set_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_TRIGGER_SHIFT(pwm));

		/*  Minimum delay is 400ns */
		k_busy_wait(1);
		return 0;
	}

	prescale = period_cycles >> PRESCALE_SHIFT_IN_CYCLES;
	if (prescale > PRESCALE_MAX)
		return -EINVAL;

	if (prescale) {
		duty_cnt = pulse_cycles / (prescale + 1);
		if ((duty_cnt < PERIOD_COUNT_MIN) ||
		    (duty_cnt > DUTY_CYCLE_HIGH_MAX))
			return -EINVAL;
	} else {
		duty_cnt = pulse_cycles;
	}

	/* Clear previous one pulse settings if any*/
	sys_set_bit((cfg->base + ONE_PULSE_CTRL0_CONTROL(pwm)),
					ONE_PULSE_CTRL0_CONTROL_FORCE_END);

	if (sys_test_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
					ONE_PULSE_CTRL1_CONTROL_ENABLE))
		sys_clear_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
						ONE_PULSE_CTRL1_CONTROL_ENABLE);

	if (sys_test_bit((cfg->base + PWM_SHUTDOWN), pwm))
		sys_clear_bit((cfg->base + PWM_SHUTDOWN), pwm);

	period_cnt = period_cycles & PERIOD_COUNT_MAX;
	duty_cnt = duty_cnt & PERIOD_COUNT_MAX;

	SYS_LOG_DBG("pwm-%d prescale %d period %d duty %d", pwm, prescale,
							period_cnt, duty_cnt);
	sys_set_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_SMOOTH_SHIFT(pwm));
	sys_clear_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_TRIGGER_SHIFT(pwm));
	/* Configure PWM channel registers*/
	val = sys_read32(cfg->base + PRESCALE_OFFSET);
	val &= ~PRESCALE_MASK(pwm);
	val |= prescale << PRESCALE_SHIFT_IN_CYCLES;
	sys_write32(val, cfg->base + PRESCALE_OFFSET);

	sys_write32(period_cnt, cfg->base + PERIOD_COUNT_OFFSET(pwm));
	sys_write32(duty_cnt, cfg->base + DUTY_CYCLE_HIGH_OFFSET(pwm));
	if (polarity)
		sys_set_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_POLARITY_SHIFT(pwm));
	else
		sys_clear_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_POLARITY_SHIFT(pwm));
	sys_set_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_TRIGGER_SHIFT(pwm));

	return 0;
}

/**
 * Get the cycles per second.
 *
 * @param dev Device struct
 * @param pwm PWM pin
 * @param cycles cycles per second.
 *
 * @return 0 for success, error otherwise
 */
static int pwm_bcm58202_get_cycles_per_sec(struct device *dev,
					   u32_t pwm, u64_t *cycles)
{
	struct device *cdru = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	u32_t pwm_clk_div;
	u32_t crmu_pwm_div;

	ARG_UNUSED(pwm);
	ARG_UNUSED(dev);

	if (cycles == NULL)
		return -EINVAL;

	/* get the pwm div and divide system clock with it */
	crmu_pwm_div = dmu_read(cdru, CIT_CRMU_PWM_CLK_DIV);
	pwm_clk_div = ((crmu_pwm_div >> PWM_CLK_DIV_HIGH_SHIFT) &
						PWM_CLK_DIV_MASK) + 1;
	pwm_clk_div += (crmu_pwm_div & PWM_CLK_DIV_MASK) + 1;

	*cycles = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / pwm_clk_div;

	return 0;
}
/**
 * Set the period and the pulse of one pulse PWM.
 *
 * @param dev Device struct
 * @param pwm Which PWM pin to set
 * @param period_cycles This field is unused in this function
 * @param pulse_cycles PWM width in clock cycles
 * @param polarity Polarity of the output 1 - Active high
 *
 * @return 0 for success, error otherwise
 */
static int pwm_one_pulse_bcm58202_pin_set(struct device *dev, u32_t pwm,
					  u32_t period_cycles,
					  u32_t pulse_cycles, u32_t polarity)
{
	const struct pwm_bcm58202_config *cfg = dev->config->config_info;
	u32_t div = 1;
	u32_t cycles = 0;
	u32_t data;

	ARG_UNUSED(period_cycles);
	/* make sure the PWM port exists */
	if (pwm >= cfg->num_ports)
		return -EIO;

	if (pulse_cycles == 0) {
		/* Force end one pulse PWM */
		sys_set_bit((cfg->base + ONE_PULSE_CTRL0_CONTROL(pwm)),
					ONE_PULSE_CTRL0_CONTROL_FORCE_END);
		sys_clear_bit((cfg->base + PWM_SHUTDOWN), pwm);
		sys_clear_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_POLARITY_SHIFT(pwm));
		sys_clear_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
					ONE_PULSE_CTRL1_CONTROL_ENABLE);
		return 0;
	}

	if (polarity > 1)
		return -EINVAL;

	/* Calculate divisor and count */
	if (pulse_cycles > USHRT_MAX) {
		u32_t msb = 0;

		msb = pulse_cycles >> (CHAR_BIT * 2);
		for (div = 1; div <= ONE_PULSE_DIV_CONTROL_MAX; div *= 2) {
			if (div > msb) {
				cycles = pulse_cycles / div;
				break;
			}
		}

		if (div > ONE_PULSE_DIV_CONTROL_MAX) {
			SYS_LOG_ERR("Invalid pulse width");
			return -EINVAL;
		}

	} else {
		cycles = pulse_cycles;
	}

	SYS_LOG_DBG("One pulse pwm-%d div %d cycles %d pol %d", pwm, div,
							cycles, polarity);
	/* Configure one pulse PWM */
	sys_set_bit((cfg->base + PWM_SHUTDOWN), pwm);
	data = sys_read32(cfg->base + ONE_PULSE_CTRL0_CONTROL(pwm));
	data = data | 0x03;
	sys_write32(data, (cfg->base + ONE_PULSE_CTRL0_CONTROL(pwm)));

	sys_write32(div, (cfg->base + ONE_PULSE_DIV_CONTROL(pwm)));
	sys_write32(cycles, (cfg->base + ONE_PULSE_PULSE_TIME_CONTROL(pwm)));
	if (polarity)
		sys_set_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
					ONE_PULSE_CTRL1_CONTROL_POLARITY);
	else
		sys_clear_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
					ONE_PULSE_CTRL1_CONTROL_POLARITY);
	sys_set_bit((cfg->base + ONE_PULSE_CTRL1_CONTROL(pwm)),
					ONE_PULSE_CTRL1_CONTROL_ENABLE);
	sys_set_bit((cfg->base + PWM_CONTROL_OFFSET),
					PWM_CONTROL_POLARITY_SHIFT(pwm));
	/*  Minimum delay is 400ns */
	k_busy_wait(1);
	/* Trigger pwm */
	sys_set_bit((cfg->base + ONE_PULSE_CTRL0_CONTROL(pwm)),
					ONE_PULSE_CTRL0_CONTROL_SOP);
	return 0;
}

static struct pwm_driver_api pwm_bcm58202_api = {
	.pin_set = pwm_bcm58202_pin_set,
	.get_cycles_per_sec = pwm_bcm58202_get_cycles_per_sec
};

static struct pwm_driver_api pwm_one_pulse_bcm58202_api = {
	.pin_set = pwm_one_pulse_bcm58202_pin_set,
	.get_cycles_per_sec = pwm_bcm58202_get_cycles_per_sec
};

/**
 * @brief Initialization function of pwm
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
int pwm_bcm58202_init(struct device *dev)
{
	struct device *cdru = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	struct device *dmu = device_get_binding(CONFIG_DMU_DEV_NAME);
	u32_t ref_clk_mhz = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MHZ(1);
	u32_t clk_div_high, clk_div_low, pwm_clk_div;

	ARG_UNUSED(dev);

	dmu_device_ctrl(dmu, DMU_DR_UNIT, DMU_PWM, DMU_DEVICE_RESET);
	/*
	 * The default divider values are one clock less. Calculate div high and
	 * div low to get 1 MHz.
	 */
	clk_div_low = ref_clk_mhz / 2;
	clk_div_high = ref_clk_mhz - clk_div_low;
	clk_div_low = clk_div_low - 1;
	clk_div_high = clk_div_high - 1;
	pwm_clk_div = (clk_div_high << PWM_CLK_DIV_HIGH_SHIFT) | clk_div_low;
	pwm_clk_div |= BIT(CLK_DIV_ENABLE_BIT);
	SYS_LOG_DBG("PWM_CLK_DIV 0x%x\n", pwm_clk_div);
	dmu_write(cdru, CIT_CRMU_PWM_CLK_DIV, pwm_clk_div);

	return 0;
}

static struct pwm_bcm58202_config pwm_bcm58202_cfg = {
	.base = PWM_BASE_ADDR,
	.num_ports = PWM_NUM_PORTS,
};

DEVICE_AND_API_INIT(pwm, CONFIG_PWM_BCM58202_DEV_NAME, pwm_bcm58202_init,
		    NULL, &pwm_bcm58202_cfg, POST_KERNEL,
		    CONFIG_PWM_DRIVER_INIT_PRIORITY,
		    &pwm_bcm58202_api);

DEVICE_AND_API_INIT(pwm_1, CONFIG_PWM_ONE_PULSE_BCM58202_DEV_NAME,
		    pwm_bcm58202_init, NULL, &pwm_bcm58202_cfg,
		    POST_KERNEL, CONFIG_PWM_DRIVER_INIT_PRIORITY,
		    &pwm_one_pulse_bcm58202_api);
