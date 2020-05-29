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
#include <device.h>
#include <irq.h>
#include <drivers/system_timer.h>
#include <cortex_a/barrier.h>
#include <cortex_a/cp15.h>
#include <misc/util.h>
#include <sys_clock.h>

#define CNTP_CTL_STATUS				(1 << 2)
#define CNTP_CTL_MASK				(1 << 1)
#define CNTP_CTL_ENABLE				(1 << 0)

#define ARCH_TIMER_PHYS_ACCESS			0
#define ARCH_TIMER_VIRT_ACCESS			1
#define ARCH_TIMER_MEM_PHYS_ACCESS		2
#define ARCH_TIMER_MEM_VIRT_ACCESS		3

#define RATE				(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / \
					 CONFIG_SYS_CLOCK_TICKS_PER_SEC)

enum arch_timer_reg {
	ARCH_TIMER_REG_CTRL,
	ARCH_TIMER_REG_TVAL,
};

/*
 * These register accessors are marked inline so the compiler can
 * nicely work out which register we want, and chuck away the rest of
 * the code. At least it does so with a recent GCC (4.6.3).
 */
static inline
void arch_timer_reg_write_cp15(int access, enum arch_timer_reg reg, u32_t val)
{
	if (access == ARCH_TIMER_PHYS_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			WRITE_CP15(val, c14, 0, c2, 1);
			break;
		case ARCH_TIMER_REG_TVAL:
			WRITE_CP15(val, c14, 0, c2, 0);
			break;
		}
	} else if (access == ARCH_TIMER_VIRT_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			WRITE_CP15(val, c14, 0, c3, 1);
			break;
		case ARCH_TIMER_REG_TVAL:
			WRITE_CP15(val, c14, 0, c3, 0);
			break;
		}
	}

	instr_sync_barrier();
}

static inline
u32_t arch_timer_reg_read_cp15(int access, enum arch_timer_reg reg)
{
	u32_t val = 0;

	if (access == ARCH_TIMER_PHYS_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			READ_CP15(c14, 0, c2, 1);
			break;
		case ARCH_TIMER_REG_TVAL:
			READ_CP15(c14, 0, c2, 0);
			break;
		}
	} else if (access == ARCH_TIMER_VIRT_ACCESS) {
		switch (reg) {
		case ARCH_TIMER_REG_CTRL:
			READ_CP15(c14, 0, c3, 1);
			break;
		case ARCH_TIMER_REG_TVAL:
			READ_CP15(c14, 0, c3, 0);
			break;
		}
	}

	return val;
}

static inline void arch_timer_set_cntfrq(u32_t rate)
{
	WRITE_CP15(rate, c14, 0, c0, 0);
}

static inline u64_t arch_counter_get_cntpct(void)
{
	u64_t val;

	instr_sync_barrier();
	__asm__ volatile("mrrc p15, 0, %Q0, %R0, c14" : "=r" (val));
	return val;
}

static inline u64_t arch_counter_get_cntp_cval(void)
{
	u64_t val;

	instr_sync_barrier();
	__asm__ volatile("mrrc p15, 2, %Q0, %R0, c14" : "=r" (val));
	return val;
}

static inline void arch_counter_set_cntp_cval(u64_t val)
{
	__asm__ volatile("mcrr p15, 2, %Q0, %R0, c14" : : "r" (val));
	instr_sync_barrier();
}

u32_t _timer_cycle_get_32(void)
{
	return arch_counter_get_cntpct();
}

/*
 * Brief: Update arm generic timer's event trigger
 *
 * Description: This function updates the arm generic timer's compare value
 *		register by an amount equivalent of one tick. So that the
 *		timer event is generated when the physical timer value equals
 *		the compare value.
 *
 * Params: None
 *
 * Returns: 0 if the compare value was updated to a value in the future
 *	    (compare value > physical count value)
 *	    1 if the compare value is in the past. Meaning the physical count
 *	    value has already passed the compare value programmed.
 *
 * This function is called from tick(). Here is an example of how this will
 * help recover lost ticks across critical sections that lock interrupts.
 * Say interrupts were locked for 100 ticks during flash erase. Then, as soon
 * as the interrupts are unlocked, this function will be called 100 times (until
 * compare value > physucal count value) by tick(), before tick() returns and
 * tick() will call _sys_clock_tick_announce() as many times.
 */
static int bcm58202_arm_timer(u32_t tick_duration)
{
	u64_t val;

	val = arch_counter_get_cntp_cval();
	val += tick_duration;
	arch_counter_set_cntp_cval(val);

	if (val < arch_counter_get_cntpct())
		return 1;
	else
		return 0;
}

static void tick(void *val)
{
	u32_t tick_duration = (u32_t) val;

	do {
		_sys_clock_tick_announce();
	} while (bcm58202_arm_timer(tick_duration));
}

int _sys_clock_driver_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* CNTFRQ is undefined on power-on.  Set it to a known value */
	arch_timer_set_cntfrq(RATE);

	/* Enable timer interrupt */
	IRQ_CONNECT(PPI_IRQ(ARM_TIMER_PPI_IRQ), 0, tick, (void *)RATE, 0);

	arch_counter_set_cntp_cval(RATE);

	/* Enable timer */
	arch_timer_reg_write_cp15(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_CTRL,
				  CNTP_CTL_ENABLE);

	irq_enable(PPI_IRQ(ARM_TIMER_PPI_IRQ));

	return 0;
}
