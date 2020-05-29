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

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <misc/util.h>

/* Stubs for OS services apis */

void k_sleep(s32_t duration)
{
	s32_t now, start;

	start = k_uptime_get_32();

	do {
		now = k_uptime_get_32();
	} while (duration > (now - start));
}

void k_busy_wait(u32_t usec_to_wait)
{
	u32_t now, start;

	start = _timer_cycle_get_32() /
		(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MHZ(1));

	do {
		now = _timer_cycle_get_32() /
			(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MHZ(1));
	} while (usec_to_wait >= (now - start));
}

u32_t k_uptime_get_32(void)
{
	return _timer_cycle_get_32() /
	  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

void k_fifo_init(struct k_fifo *fifo)
{
	ARG_UNUSED(fifo);
}

void k_fifo_put(struct k_fifo *fifo, void *data)
{
	ARG_UNUSED(fifo);
	ARG_UNUSED(data);
}

void *k_fifo_get(struct k_fifo *fifo, s32_t timeout)
{
	ARG_UNUSED(fifo);
	k_sleep(timeout);

	return NULL;
}

void k_sem_init(struct k_sem *sem, unsigned int initial_count,
			unsigned int limit)
{
	ARG_UNUSED(sem);
	ARG_UNUSED(initial_count);
	ARG_UNUSED(limit);
}

int k_sem_take(struct k_sem *sem, s32_t timeout)
{
	ARG_UNUSED(sem);
	ARG_UNUSED(timeout);

	return 0;
}

void k_sem_give(struct k_sem *sem)
{
	ARG_UNUSED(sem);
}

k_tid_t k_thread_create(struct k_thread *new_thread,
			       k_thread_stack_t stack,
			       size_t stack_size,
			       k_thread_entry_t entry,
			       void *p1, void *p2, void *p3,
			       int prio, u32_t options, s32_t delay)
{
	ARG_UNUSED(new_thread);
	ARG_UNUSED(stack);
	ARG_UNUSED(stack_size);
	ARG_UNUSED(entry);
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	ARG_UNUSED(prio);
	ARG_UNUSED(options);
	ARG_UNUSED(delay);

	return NULL;
}

k_tid_t k_current_get(void)
{
	return NULL;
}

void sys_tick_handler(u32_t ticks)
{
	ARG_UNUSED(ticks);
}

/* Spin if we get an interrupt for no_os env */
void default_exc_handler(void)
{
	printk("Received interrupt in no os build, spinning!\n");

	while(1);
}
