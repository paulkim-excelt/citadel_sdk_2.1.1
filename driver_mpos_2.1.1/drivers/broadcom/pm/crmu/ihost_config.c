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
#include "ihost_config.h"

/* TODO: Post functionality verification cleaup:
 * 1. Use #define for magic constants
 *
 * !!!Warning!!!
 *
 * This code uses legacy macros from DV software
 * which concatenates strings resulting int constants defined
 * in socregs.h. So removing socregs.h will break compilation
 * of this code or may result in unpredictable behavior.
 * Therefore, before removing socregs.h from the code base
 * the constants referenced by this code must be extracted
 * and provided.
 */

/**
 * @brief ihost_config
 *
 * Power On PD_CPU (Note that it is different from PD_SYS)
 *
 * @param None
 *
 * @return N/A
 */
void ihost_config(void)
{
	register u32_t spare3, spare4;
	register u32_t data1, data2;
	register u32_t lock, lock_status;

	/* 2] PLL Power up
	 * b] Turn on PLL LDO/PWR
	 */
	WrReg(A7_CRM_PLL_PWR_ON,0xB);

	/* Wait for 50us before removing PLL ISO */
	timer_wait(IHOST_PLL_PWRON_WAIT);

	/* 3] TOP(switched) Power up
	 * c] Turn on switched top supply (VDD_SW_TOP) through
	 * distributed switches:
	 * Turn on switches bit by bit 
	 */
	/* Weak switch programming */
	spare3 = 0x0;
	for (spare4 = 0x0; spare4 <= 0x7; spare4++) {
		spare3 |= (1 << spare4);
		WrRegField(IHOST0_CONFIG_ROOT, A7_CRM_PWRON_IN_DOMAIN_4,
			PWRON_IN_DOMAIN_4_VECTOR, spare3);
	}

	/* Strong switch programming */
	WrRegField(IHOST0_CONFIG_ROOT, A7_CRM_PWROK_IN_DOMAIN_4,
		PWROK_IN_DOMAIN_4_VECTOR, 0xFF);

	timer_wait(IHOST_SWITCH_PROG_TIMEOUT); /* 10us */

	data1 = RdRegField(IHOST0_CONFIG_ROOT,
		A7_CRM_PWRON_OUT_DOMAIN_4, PWRON_OUT_DOMAIN_4_VECTOR);
	data2 = RdRegField(IHOST0_CONFIG_ROOT,
		A7_CRM_PWROK_OUT_DOMAIN_4, PWROK_OUT_DOMAIN_4_VECTOR);

	if ((data1 & data2) == 0xff) {
		/* GPIO[3] bit set to '1' to Display switch programming Pass */
		WrReg(GP_DATA_OUT, RdReg(GP_DATA_OUT) | IHOST_SWITCH_STATUS);
	}

	/* [e] Remove PLL/TOP(memory and logic) isolations */
	WrReg(A7_CRM_PLL_PWR_ON, 0x3); /* Remove PLL output ISO */
 
	/* Remove Memory isolation */
	WrReg(A7_CRM_DOMAIN_4_CONTROL, 0x9);
	/* Remove logic isolation */
	WrReg(A7_CRM_DOMAIN_4_CONTROL, 0x8);

	/* 4] PLL Programming
	 * [a] Set up Interrupt
	 * clear off any interrupt and get ready
	 * to monitor lock interrupt status
	 */
	WrReg(A7_CRM_PLL_INTERRUPT_STATUS,0x300);
	WrReg(A7_CRM_PLL_INTERRUPT_ENABLE,0x300);

	/* [b] PLL programming for desired ARM clock frequency
     * PDIV              : 'd1
     * NDIV_INT          : 'd38
     * NDIV_FRAC         : 'd0
     * MDIV              : 'd1
     * KP                : 'd10
     * KI                : 'd3
     * KA                : 'd3
	 */

	/* NDIV_FRAC LSB */
	WrReg(A7_CRM_PLL0_CTRL1, 0x0);
	/* NDIV_FRAC MSB */
	WrReg(A7_CRM_PLL0_CTRL2, 0x0);
	/*  PDIV + NDIV_INT */
#ifdef CITADEL_DV_BOOT
	WrReg(A7_CRM_PLL0_CTRL3, ((0x1 << A7_CRM_PLL0_CTRL3__PLL0_PDIV_R ) | 0x26));
#else
	WrReg(A7_CRM_PLL0_CTRL3, ((0x1 << A7_CRM_PLL0_CTRL3__PLL0_PDIV_R) | 0x18));
#endif
	/* KA/KI/KP */
	WrReg(A7_CRM_PLL0_CTRL4, ((0x1a << A7_CRM_PLL0_CTRL4__PLL0_FREFEFF_INFO_R)
		| (0x3 << A7_CRM_PLL0_CTRL4__PLL0_KA_R)
		| (0x3 << A7_CRM_PLL0_CTRL4__PLL0_KI_R)
		| (0xa << A7_CRM_PLL0_CTRL4__PLL0_KP_R)));

	/* these 4 registers drive the i_pll_ctrl[63:0] input
	 * of the pll (16b per register). the values are derived
	 * from the spec (sections 8 and 10) PLL VCO range selection
	 */
	WrReg(A7_CRM_PLL0_CFG0_CTRL, 0x00000000);
	WrReg(A7_CRM_PLL0_CFG1_CTRL, 0x00000000); 
	WrReg(A7_CRM_PLL0_CFG2_CTRL, 0x00000000);
	WrReg(A7_CRM_PLL0_CFG3_CTRL, 0x00000000);

	/* [c] CRM PLL control update
     * PLL channel control - set to always on
	 */
	WrReg(A7_CRM_PLL0_CHANNEL_CONTROL, 0x3);

	/* PLL channel0 control (for post divider) */
#ifdef CITADEL_DV_BOOT
	WrReg(A7_CRM_PLL0_CHANNEL0_DESCRIPTION, 0x1);
#else
	WrReg(A7_CRM_PLL0_CHANNEL0_DESCRIPTION, 0x3);
#endif

	/* update the PLL post dividers and vco */
	WrReg(A7_CRM_PLL_COMMAND, 0x3);

	/* [d] PLL reset
	 * de-asert PLL reset
	 */
	WrReg(A7_CRM_PLL_PWR_ON, 0x13);

	/* 5] Clock Programming
	 * [a] Release clock source reset
	 */
	WrReg(A7_CRM_SOFTRESETN_0, 0x7); 
	/* wait ~500ns (500/38.46 = 13) */
	timer_wait(10);

	/* [b] Setup clock description
	 * clock dividers
	 */
	WrReg(A7_CRM_AXI_CLK_DESC, 0x3); /* div by 4 */
	WrReg(A7_CRM_ACP_CLK_DESC, 0x3); /* div by 4 */
	WrReg(A7_CRM_ATB_CLK_DESC, 0x3); /* div by 4 */
	WrReg(A7_CRM_PCLKDBG_DESC, 0x7); /* div by 8 */

	/* clock change trigger - must set to take
	 * effect after clock source change
	 */
	WrReg(A7_CRM_CLOCK_MODE_CONTROL, 0x1); 

	/* [c] Enable clocks - turn on functional clocks */
	/* SRAM clock enable */
	WrReg(A7_CRM_SUBSYSTEM_CONFIG_2, 0x1);

	/* turn on function clocks */
	/* { PDBG, ATB, ACP, AXI, ARM } */
	WrReg(A7_CRM_CLOCK_CONTROL_0, 0x3FF);
	/* { APB, TMON } */
	WrReg(A7_CRM_CLOCK_CONTROL_1, 0x3C0);

	/* 6] PLL Lock and post -divider channel reset removal */
	/* [a] Poll PLL lock status until locked */
	WrReg(CRMU_TIM_TIMER1Control, 0x02);
	WrReg(CRMU_TIM_TIMER1IntClr, 0x1);
	if (!isFunctionalTestMode()) {
		WrReg(CRMU_TIM_TIMER1Load, ARM_PLL_TIMEOUT_2);
	}
	else {
		WrReg(CRMU_TIM_TIMER1Load, ARM_PLL_TIMEOUT);
	}
	WrReg(CRMU_TIM_TIMER1Control, 0x82);

	while (!RdReg(CRMU_TIM_TIMER1RIS)) {
		if (!isFunctionalTestMode()) {
			if(RdReg(IHOST0_CONFIG_ROOT + A7_CRM_PLL_STATUS)
			 & (0x1 << A7_CRM_PLL_STATUS__PLL0_LOCK_R))
				break;
		}
	}

	if (RdReg(IHOST0_CONFIG_ROOT + A7_CRM_PLL_STATUS)
		& (0x1 << A7_CRM_PLL_STATUS__PLL0_LOCK_R)) {
		/* [b] PLL post-divider channel reset */
		WrReg(A7_CRM_PLL_PWR_ON, 0x33);
		/* [c] Switch to PLL output */
		/* PLL channel control */
		WrReg(A7_CRM_PLL_CHANNEL_BYPASS_ENABLE, 0x0);
		/* GPIO[4] bit set to '1' to Display PLL pass */
		WrReg(GP_DATA_OUT, RdReg(GP_DATA_OUT) | IHOST_PLL_LOCK_STATUS);
	}

	/* 8] Reset Programming */
	/* [a] Release soft resets */
	WrReg(A7_CRM_SUBSYSTEM_CONFIG_0, 0x00F0); /* Indicate DBG powered up */

	/* Bus clocks --> PERIPH --> L2 --> CPU */
	/* Bus { PCLKDBG, ATB, ACP, AXI } */
	WrReg(A7_CRM_SOFTRESETN_0, 0x1E07);
	/* Bus { APB } */
	WrReg(A7_CRM_SOFTRESETN_1, 0x0100);
	/* Periph components */
	WrReg(A7_CRM_SOFTRESETN_0, 0xFE07);
	/* L2 */
	WrReg(A7_CRM_SOFTRESETN_0, 0xFF0F);
	/* CPUPOR */
	WrReg(A7_CRM_SOFTRESETN_0, 0xFFFF);
	/* CPU/DBG */
	WrReg(A7_CRM_SOFTRESETN_1, 0xFFFF);
}
