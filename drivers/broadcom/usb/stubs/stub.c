/*******************************************************************
 *
 *  Copyright 2013
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  Irvine CA 92619-7013
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************
 *
 *
 *******************************************************************/

#include "stub.h"

#ifndef USH_BOOTROM /* SBI */

tUSB_MEM_BLOCK usb_mem_block;

#ifdef USBD_CV_INTERFACE
#include "cvmemblock.h"
tCVCLASS_MEM_BLOCK cvclass_mem_block;
#endif
#endif


#ifndef USH_BOOTROM /* SBI */
/* Most of the timer functionality required for basic sbi */
#define CRMU_REF_CLK 26

void phx_delay_us(int us)
{
	int n;
	n = us * CRMU_REF_CLK;
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Control, 0xA2, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2IntClr, 0x1, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Load, n, AHB_BIT32);
}

PHX_STATUS phx_set_timer_milli_seconds(uint8_t timer, uint32_t time_ms, uint8_t mode, bool oneshot)
{
	ARG_UNUSED(timer);
	ARG_UNUSED(mode);
	ARG_UNUSED(oneshot);

	time_ms*=1000; //just set time timer and exit no blocking
	phx_delay_us(time_ms);

	return 0;
}

uint32_t phx_timer_get_current_value(uint8_t timer)
{
	uint32_t reg_data;

	ARG_UNUSED(timer);

	/* Read tick value */
	reg_data= AHB_READ_SINGLE(CRMU_TIM_TIMER2Value,AHB_BIT32);

	return reg_data;
}

void phx_blocked_delay_us(unsigned int us)
{
	unsigned int reg_data;
	uint32_t n;

	n = us * CRMU_REF_CLK;
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Control, 0xA2, AHB_BIT32);
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2IntClr, 0x1, AHB_BIT32);		// Interrupt clear
	AHB_WRITE_SINGLE(CRMU_TIM_TIMER2Load, n, AHB_BIT32);

	do {
		/* Raw interrupt status will be 1 on expiry */
		reg_data= AHB_READ_SINGLE(CRMU_TIM_TIMER2RIS,AHB_BIT32);
	}while(reg_data!= 1);

}

void phx_blocked_delay_ms(unsigned int ms)
{
	unsigned int i;
	for(i = 0; i < ms; i++)
		phx_blocked_delay_us(1000);
}


/* DUMMY FUNCTION FOR TESTING CV COMMAND HANDLING*/
uint32_t ush_bootrom_versions(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer)
{
	ARG_UNUSED(numInputParams);
	ARG_UNUSED(inputparam);
	ARG_UNUSED(outLength);
	ARG_UNUSED(outBuffer);
	unsigned int i;

	char *curptr = (char *) outBuffer;

	char arr[256] = {'1','1','1','1','1','1','1','1',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','\n',

					 '2','2','2','2','2','2','2','2',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','\n',

					 '3','3','3','3','3','3','3','3',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','\n',

					 '4','4','4','4','4','4','4','4',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','M',
					 'B','R','O','A','D','C','O','\0'};

	for(i=0;i<sizeof(arr);i++)
	{
		*curptr = arr[i];
		curptr++;
	}
	
	*outLength = sizeof(arr);
	
	return 0;
}

#endif

