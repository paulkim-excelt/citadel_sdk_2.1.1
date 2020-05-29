/******************************************************************************
 *	Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *	and/or its subsidiaries.
 *
 *	This program is the proprietary software of Broadcom and/or its licensors,
 *	and may only be used, duplicated, modified or distributed pursuant to the
 *	terms and conditions of a separate, written license agreement executed
 *	between you and Broadcom (an "Authorized License").  Except as set forth in
 *	an Authorized License, Broadcom grants no license (express or implied),
 *	right to use, or waiver of any kind with respect to the Software, and
 *	Broadcom expressly reserves all rights in and to the Software and all
 *	intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *	THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *	IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *	Except as expressly set forth in the Authorized License,
 *
 *	1.	   This program, including its structure, sequence and organization,
 *	constitutes the valuable trade secrets of Broadcom, and you shall use all
 *	reasonable efforts to protect the confidentiality thereof, and to use this
 *	information only in connection with your use of Broadcom integrated circuit
 *	products.
 *
 *	2.	   TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *	"AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *	OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *	RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *	IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *	PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *	ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *	ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *	3.	   TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *	ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *	INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *	RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *	HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *	EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *	WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *	FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/* @file stub.c
 *
 * This file contains stubs for compiling CS code base driver in Mpos code stream.
 * FIXME: Remove this later.
 *
 */


#include "stub.h"


tUSB_MEM_BLOCK usb_mem_block;
#include <kernel.h>

#ifdef USH_BOOTROM /* SBI */
#include <sys_clock.h>
#endif

#ifndef USH_BOOTROM /* SBI */

/* Most of the timer functionality required for basic sbi */
#define CRMU_REF_CLK 26

void phx_delay_us(int us)
{
#if 0
	int n;
	n = us * CRMU_REF_CLK;
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Control, 0xA2, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2IntClr, 0x1, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Load, n, AHB_BIT32);
#endif
	k_busy_wait(us);
}

PHX_STATUS phx_set_timer_milli_seconds(uint8_t timer, uint32_t time_ms, uint8_t mode, bool oneshot)
{
	ARG_UNUSED(timer);
	ARG_UNUSED(mode);
	ARG_UNUSED(oneshot);
#if 0
	time_ms *= 1000; /*just set time timer and exit no blocking */
	phx_delay_us(time_ms);
#endif
	volatile unsigned int i;

	for (i = 0; i < time_ms; i++) {
		k_busy_wait(1000);
	}
	return 0;
}

uint32_t phx_timer_get_current_value(uint8_t timer)
{
	uint32_t reg_data;

	ARG_UNUSED(timer);

	/* Read tick value */
	reg_data = AHB_READ_SINGLE(CRMU_TIM_TIMER2Value, AHB_BIT32);

	return reg_data;
}

void phx_blocked_delay_us(unsigned int us)
{
#if 0
	unsigned int reg_data;
	uint32_t n;

	n = us * CRMU_REF_CLK;
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Control, 0xA2, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2IntClr, 0x1, AHB_BIT32);	/* Interrupt clear */
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Load, n, AHB_BIT32);

	do {
		/* Raw interrupt status will be 1 on expiry */
		reg_data = AHB_READ_SINGLE(CRMU_TIM_TIMER2RIS, AHB_BIT32);
	} while (reg_data != 1);
#endif
	k_busy_wait(us);

}

void phx_blocked_delay_ms(unsigned int ms)
{
	unsigned int i;
	for (i = 0; i < ms; i++)
#if 0
		phx_blocked_delay_us(1000);
#endif
		k_busy_wait(1000);
}


/* DUMMY FUNCTION FOR TESTING CV COMMAND HANDLING*/
uint32_t ush_bootrom_versions(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer)
{
	ARG_UNUSED(numInputParams);
	ARG_UNUSED(inputparam);
	/* ARG_UNUSED(outLength); */
	ARG_UNUSED(outBuffer);
	unsigned int i;

	char *curptr = (char *) outBuffer;

	char arr[256] = {'1', '1', '1', '1', '1', '1', '1', '1',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', '\n',

					 '2', '2', '2', '2', '2', '2', '2', '2',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', '\n',

					 '3', '3', '3', '3', '3', '3', '3', '3',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', '\n',

					 '4', '4', '4', '4', '4', '4', '4', '4',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
					 'B', 'R', 'O', 'A', 'D', 'C', 'O', '\O'};

	for (i = 0; i < sizeof(arr); i++) {
		*curptr = arr[i];
		curptr++;
	}

	*outLength = sizeof(arr);

	return 0;
}
#else



#endif

