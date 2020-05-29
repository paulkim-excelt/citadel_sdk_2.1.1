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
#include <board.h>
#include <genpll.h>
#include <device.h>
#include <init.h>
#include <irq.h>
#include <sys_clock.h>
#include <timer.h>
#include <logging/sys_log.h>
#include <toolchain/gcc.h>

#define TIMER_LOAD_ADDR			(0x00UL)
#define TIMER_VALUE_ADDR		(0x04UL)
#define TIMER_CONTROL_ADDR		(0x08UL)
#define TIMER_CONTROL_ONESHOT		(1 << 0)
#define TIMER_CONTROL_32BIT_COUNTER	(1 << 1)
#define TIMER_CONTROL_PRESCALE_0	(0 << 2)
#define TIMER_CONTROL_PRESCALE_4	(1 << 2)
#define TIMER_CONTROL_PRESCALE_8	(2 << 2)
#define TIMER_CONTROL_RESERVED		(0 << 4)
#define TIMER_CONTROL_IRQ_ENABLE	(1 << 5)
#define TIMER_CONTROL_MODE_PERIODIC	(1 << 6)
#define TIMER_CONTROL_ENABLE		(1 << 7)
#define TIMER_IRQ_CLEAR_ADDR		(0x0CUL)
#define TIMER_RIS_ADDR			(0x10UL)
#define TIMER_MIS_ADDR			(0x14UL)
#define TIMER_BGLOAD_ADDR		(0x18UL)

#define TIMER_INTERRUPT_PRIORITY_LEVEL	0
#define TIMER_MAX_TIME			((1ULL << 32) - 1)

struct timer_cfg {
	mem_addr_t base;
	int irq;
	void (*irq_connect)(void);
};

struct timer_cb {
	void (*cb)(void *data);
	void *cb_data;
	u32_t flags;
};

static u32_t bcm58202_query_timer(struct device *dev)
{
	const struct timer_cfg *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;

	return sys_read32(base + TIMER_VALUE_ADDR);
}

static void bcm58202_resume_timer(struct device *dev)
{
	const struct timer_cfg *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;
	u32_t val;

	val = sys_read32(base + TIMER_CONTROL_ADDR);
	val |= TIMER_CONTROL_ENABLE;
	sys_write32(val, base + TIMER_CONTROL_ADDR);
}

static void bcm58202_suspend_timer(struct device *dev)
{
	const struct timer_cfg *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;
	u32_t val;

	val = sys_read32(base + TIMER_CONTROL_ADDR);
	val &= ~TIMER_CONTROL_ENABLE;
	sys_write32(val, base + TIMER_CONTROL_ADDR);
}

static void bcm58202_stop_timer(struct device *dev)
{
	const struct timer_cfg *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;
	u32_t flags = ((struct timer_cb *)dev->driver_data)->flags;
	int irq = cfg->irq;

	if (!(flags & TIMER_NO_IRQ)) {
		irq_disable(irq);

		/* Write a 0 to clear the interrupt bit, just incase one popped
		 * while calling this
		 */
		sys_write32(0, base + TIMER_IRQ_CLEAR_ADDR);
	}

	/* Disable the timer */
	sys_write32(0, base + TIMER_CONTROL_ADDR);
}

static int bcm58202_start_timer(struct device *dev, u64_t time)
{
	const struct timer_cfg *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;
	u32_t val, rate, flags = ((struct timer_cb *)dev->driver_data)->flags;
	int rc, irq = cfg->irq;

	rc = clk_get_wtus(&rate);
	if (rc)
		return rc;

	/* We are assuming that the time being passed in is in NS */
	time = (time * rate) / NSEC_PER_SEC;
	if (!time) {
		SYS_LOG_ERR("Attempting to set timer below min of %d ns",
			    NSEC_PER_SEC / rate);
		return -EINVAL;
	}

	/* A design decision is being made here for timers >32bit.  We are
	 * assuming that it is acceptable to lose the bottom 4-8bits of
	 * accruacy for very large times.  This is done based on the assumption
	 * that the amount of time it would take to get the interrupt, service
	 * interrupt, reference the software for time to reprogram the timer for
	 * the lower 4-8bits, reprogram the timer, and have it pop again is
	 * greater than the bottom 4-8 bits.  Also, the overhead of the SW to
	 * add for this option is not trivial.
	 */
	if (time <= TIMER_MAX_TIME) {
		val = TIMER_CONTROL_PRESCALE_0;
	} else if (time >> 4 <= TIMER_MAX_TIME) {
		time >>= 4;
		val = TIMER_CONTROL_PRESCALE_4;
	} else if (time >> 8 <= TIMER_MAX_TIME) {
		time >>= 8;
		val = TIMER_CONTROL_PRESCALE_8;
	} else {
		SYS_LOG_ERR("Attempting to set timer above max of %lld ns",
			    TIMER_MAX_TIME << 8);
		return -EINVAL;
	}
	SYS_LOG_DBG("rate %d, time %lld, val %x", rate, time, val);

	if (!(flags & TIMER_NO_IRQ)) {
		irq_disable(irq);

		/* Write a 0 to clear the interrupt bit */
		sys_write32(0, base + TIMER_IRQ_CLEAR_ADDR);

		/* We must enable the interrupt prior to flipping the enable bit
		 * in the timer or we risk missing the interrupt for small
		 * timeout values
		 */
		irq_enable(irq);
	}

	sys_write32((u32_t) time, base + TIMER_LOAD_ADDR);

	/* Setup the Timer */
	val |= TIMER_CONTROL_32BIT_COUNTER;
	val |= TIMER_CONTROL_ENABLE;
	if (flags & TIMER_PERIODIC)
		val |= TIMER_CONTROL_MODE_PERIODIC;
	if (!(flags & TIMER_NO_IRQ))
		val |= TIMER_CONTROL_IRQ_ENABLE;
	if (flags & TIMER_ONESHOT)
		val |= TIMER_CONTROL_ONESHOT;
	sys_write32(val, base + TIMER_CONTROL_ADDR);

	return 0;
}

/* IRQ_CONNECT needs "all of its arguments must be computable by the compiler at
 * build time".  So, use this function as an intermediary.
 */
static void bcm58202_irq_cb(void *data)
{
	struct device *dev = data;
	struct timer_cb *tcb = dev->driver_data;

	if (tcb->cb)
		tcb->cb(tcb->cb_data);
}

static int bcm58202_register_timer(struct device *dev, void (*cb)(void *data),
				   void *cb_data, u32_t flags)
{
	struct timer_cb *tcb = dev->driver_data;

	tcb->flags = flags;
	tcb->cb = cb;
	tcb->cb_data = cb_data;

	if (!(flags & TIMER_NO_IRQ)) {
		const struct timer_cfg *cfg = dev->config->config_info;

		/* Enable timer interrupt */
		cfg->irq_connect();
	}

	return 0;
}

static int bcm58202_timer_init(struct device *dev)
{
	dev->driver_data = k_malloc(sizeof(struct timer_cb));
	if (!dev->driver_data)
		return -ENOMEM;

	return  0;
}

static struct timer_driver_api bcm58202_timer_api = {
	.init = bcm58202_register_timer,
	.start = bcm58202_start_timer,
	.stop = bcm58202_stop_timer,
	.suspend = bcm58202_suspend_timer,
	.resume = bcm58202_resume_timer,
	.query = bcm58202_query_timer,
};

DEVICE_DECLARE(bcm58202_timer0);

static void irq_connect_bcm58202_timer0(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45040000_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer0), 0);
}

static struct timer_cfg timer0_cfg_data = {
	.base = ARM_SP804_TIMER_45040000_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45040000_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer0,
};

DEVICE_AND_API_INIT(bcm58202_timer0, TIMER0_NAME, bcm58202_timer_init,
		    NULL, &timer0_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer1);

static void irq_connect_bcm58202_timer1(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45040020_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer1), 0);
}

static struct timer_cfg timer1_cfg_data = {
	.base = ARM_SP804_TIMER_45040020_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45040020_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer1,
};

DEVICE_AND_API_INIT(bcm58202_timer1, TIMER1_NAME, bcm58202_timer_init,
		    NULL, &timer1_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer2);

static void irq_connect_bcm58202_timer2(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45050000_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer2), 0);
}

static struct timer_cfg timer2_cfg_data = {
	.base = ARM_SP804_TIMER_45050000_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45050000_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer2,
};

DEVICE_AND_API_INIT(bcm58202_timer2, TIMER2_NAME, bcm58202_timer_init,
		    NULL, &timer2_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer3);

static void irq_connect_bcm58202_timer3(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45050020_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer3), 0);
}

static struct timer_cfg timer3_cfg_data = {
	.base = ARM_SP804_TIMER_45050020_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45050020_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer3,
};

DEVICE_AND_API_INIT(bcm58202_timer3, TIMER3_NAME, bcm58202_timer_init,
		    NULL, &timer3_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer4);

static void irq_connect_bcm58202_timer4(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45140000_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer4), 0);
}

static struct timer_cfg timer4_cfg_data = {
	.base = ARM_SP804_TIMER_45140000_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45140000_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer4,
};

DEVICE_AND_API_INIT(bcm58202_timer4, TIMER4_NAME, bcm58202_timer_init,
		    NULL, &timer4_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer5);

static void irq_connect_bcm58202_timer5(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45140020_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer5), 0);
}

static struct timer_cfg timer5_cfg_data = {
	.base = ARM_SP804_TIMER_45140020_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45140020_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer5,
};

DEVICE_AND_API_INIT(bcm58202_timer5, TIMER5_NAME, bcm58202_timer_init,
		    NULL, &timer5_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer6);

static void irq_connect_bcm58202_timer6(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45150000_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer6), 0);
}

static struct timer_cfg timer6_cfg_data = {
	.base = ARM_SP804_TIMER_45150000_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45150000_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer6,
};

DEVICE_AND_API_INIT(bcm58202_timer6, TIMER6_NAME, bcm58202_timer_init,
		    NULL, &timer6_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);

DEVICE_DECLARE(bcm58202_timer7);

static void irq_connect_bcm58202_timer7(void)
{
	IRQ_CONNECT(SPI_IRQ(ARM_SP804_TIMER_45150020_IRQ_0),
		    TIMER_INTERRUPT_PRIORITY_LEVEL, bcm58202_irq_cb,
		    DEVICE_GET(bcm58202_timer7), 0);
}

static struct timer_cfg timer7_cfg_data = {
	.base = ARM_SP804_TIMER_45150020_BASE_ADDRESS_0,
	.irq = SPI_IRQ(ARM_SP804_TIMER_45150020_IRQ_0),
	.irq_connect = irq_connect_bcm58202_timer7,
};

DEVICE_AND_API_INIT(bcm58202_timer7, TIMER7_NAME, bcm58202_timer_init,
		    NULL, &timer7_cfg_data, PRE_KERNEL_2,
		    CONFIG_TIMER_INIT_PRIORITY, &bcm58202_timer_api);
