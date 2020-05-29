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
 * !!! DO NOT ADD ANY OTHER FUNCTIONS IN THIS FILE !!!
 */
#include "M0.h"

/**
 * @brief Mailbox Interrupt Handler
 *
 * TODO: Verify this code.
 * Description: This is the user Mailbox Interrupt Handler for CRMU.
 *
 * TODO: Is this the common handler to be invoked (by CRMU M0) for:
 * (Why two different terminology for event & IRQ?)
 * 1. SMBUS Interrupt
 * 2. Mailbox Event / Interrupt
 * 3. Soft Reset Event (Special Event generated internally by CRMU that
 *    triggers a soft reset by writing Mailbox message? This will result
 *    in this ISR being invoked twice.)
 *
 * @param None
 *
 * @return TODO (Ask CRMU ROM devs): What does return value of 1 mean?
 */
s32_t user_mbox_handler(void)
{
	/* CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS(); */
	u32_t code0       = sys_read32(IPROC_CRMU_MAIL_BOX0);
	u32_t code1       = sys_read32(IPROC_CRMU_MAIL_BOX1);
	u32_t irqstatus   = sys_read32(CRMU_MCU_INTR_STATUS);
	u32_t eventstatus = sys_read32(CRMU_MCU_EVENT_STATUS);
	u32_t need_wakeup = 0;

	MCU_SET_STATE_IN_MBOX_ENTRY_C();

#ifdef PM_UNIT_TEST
	/* Test Code: Just trigger a AONGPIO interrupt back to A7 upon
	 * receiving a MailBox Msg. Give some delay before sending out
	 * the mailbox interrupt to allow the A7 enter wfi.
	 * -- For testing M0 ISRs on emulator --
	 */
	volatile u32_t delay = 5000;
	volatile u32_t cnt1, cnt2;
	cnt1 = sys_read32(CRMU_SPARE_REG_4);
	sys_write32(cnt1 + 1, CRMU_SPARE_REG_4);

	while(delay--)
		;

	/* Clear the MCU MailBox Event.
	 * If this is NOT cleared M0 won't be able to receive subsequent
	 * mailbox interrupts from A7.
	 */
	MCU_clear_event(MCU_MAILBOX_EVENT);

	if (code0 == MAILBOX_CODE0__SLEEP && code1 == MAILBOX_CODE1__PM) {
		delay = 50000;
		while(delay--);
		/* Send a simple Mailbox Msg to Interrupt the A7*/
		MCU_send_msg_to_mproc(MAILBOX_CODE0__NOTHING, MAILBOX_CODE1__NOTHING);
	}
	else if (code0 == MAILBOX_CODE0__DRIPS && code1 == MAILBOX_CODE1__PM) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();

		/* Suppress Compiler Optimization */
		MCU_SYNC();

		/* Enter DRIPS Mode */
		MCU_SoC_DRIPS_Handler();

		delay = 5000;
		while(delay--);

		/* Exit DRIPS Mode */
		MCU_SoC_Wakeup_Handler(MPROC_PM_MODE__DRIPS, MPROC_PM_MODE__DRIPS,
			MAILBOX_CODE0__WAKEUP, MAILBOX_CODE1__WAKEUP_TEST);
	}
	else if (code0 == MAILBOX_CODE0__DEEPSLEEP && code1 == MAILBOX_CODE1__PM) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();

		/* Suppress Compiler Optimization */
		MCU_SYNC();

		/* Enter Deep Sleep Mode */
		MCU_SoC_DeepSleep_Handler();

		delay = 5000;
		while(delay--);

		/* Exit Deep Sleep Mode */
		MCU_SoC_Wakeup_Handler(MPROC_PM_MODE__DEEPSLEEP, MPROC_PM_MODE__DEEPSLEEP,
			MAILBOX_CODE0__WAKEUP, MAILBOX_CODE1__WAKEUP_TEST);
	}

	MCU_SET_STATE_IN_MBOX_ENTRY_C();

	cnt2 = sys_read32(CRMU_SPARE_REG_5);
	sys_write32(cnt2 + 1, CRMU_SPARE_REG_5);
	return 1;
#endif

#ifdef MPROC_PM__A7_WDOG_RESET
	/* Must check MCU MPROC CRMU event status
	 * prior to decode mbox interrupt
	 * TODO: Why Event3? Is Event3 for Soft
	 * Reset? Is this still valid?
	 */
	if (eventstatus & BIT(MCU_MPROC_CRMU_EVENT3)) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();

		MCU_clear_event(MCU_MPROC_CRMU_EVENT3);

		MCU_send_msg_to_mcu(MAILBOX_CODE0_SysSoftReset,
		MAILBOX_CODE1_SysSoftReset);

		return 1;
	}
#endif

#ifdef MPROC_PM__A7_SMBUS_ISR
	/* Must check MCU MPROC CRMU event
	 * status prior to decode mbox interrupt
	 */
	if (irqstatus & BIT(MCU_SMBUS_INTR)) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();
		/* Clear the SMBUS IRQ for M0 */
		MCU_clear_irq(MCU_SMBUS_INTR);
		/* Forward the SMBUS Interrupt to A7 using MailBox */
		MCU_send_msg_to_mproc(MAILBOX_CODE0__SMBUS_ISR,
		MAILBOX_CODE1__SMBUS_ISR);

		return 1;
	}
#endif

	/* Clear irq / event */
	if (eventstatus & BIT(MCU_MAILBOX_EVENT)) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();

		MCU_clear_event(MCU_MAILBOX_EVENT);

		/* Decode MBOX messages */
		if (code1 == MAILBOX_CODE1__PM) {
			if (code0 == MAILBOX_CODE0__RUN_1000) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_Run_Handler(MPROC_PM_MODE__RUN_1000);
			}
			else if (code0 == MAILBOX_CODE0__RUN_500) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_Run_Handler(MPROC_PM_MODE__RUN_500);
			}
#ifdef MPROC_PM__DPA_CLOCK
			else if (code0 == MAILBOX_CODE0__RUN_187) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_Run_Handler(MPROC_PM_MODE__RUN_187);
			}
#endif
			else if (code0 == MAILBOX_CODE0__RUN_200) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_Run_Handler(MPROC_PM_MODE__RUN_200);
			} else if (code0 == MAILBOX_CODE0__SLEEP) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_Sleep_Handler();
				need_wakeup = 1;
			} else if (code0 == MAILBOX_CODE0__DRIPS) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_DRIPS_Handler();
				need_wakeup = 1;
			} else if (code0 == MAILBOX_CODE0__DEEPSLEEP) {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
				MCU_SoC_do_policy();
				MCU_SoC_DeepSleep_Handler();
				need_wakeup = 1;
			} else {
				MCU_SET_STATE_IN_MBOX_ENTRY_C();
			}
		} else if (code0 == MAILBOX_CODE0__NOTHING
				   && code1 == MAILBOX_CODE1__NOTHING) {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
		}
		/* system restart, default Mbox ISR does not check code0 */
		else if (code0 == MAILBOX_CODE0_SysSoftReset
				 && code1 == MAILBOX_CODE1_SysSoftReset) {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
			sys_write32(0x1, CRMU_SOFT_RESET_CTRL);
		}
		else if (code0 == MAILBOX_CODE0_CrmuWdtReset_L1
				 && code1 == MAILBOX_CODE1_CrmuWdtReset_L1) {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
			sys_write32(0x1ACCE551, CRMU_WDT_WDOGLOCK);
			sys_write32(0x3, CRMU_WDT_WDOGCONTROL);
			sys_write32(0x1, CRMU_WDT_WDOGLOAD);
			sys_write32(0x0, CRMU_WDT_WDOGLOCK);
		}
		else if (code0 == MAILBOX_CODE0_SWWarmReboot_L0
				 && code1 == MAILBOX_CODE1_SWWarmReboot_L0) {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
			sys_write32(0x1, CRMU_SOFT_RESET_CTRL);
		}
		else if (code0 == MAILBOX_CODE0_SWColdReboot_L0
				 && code1 == MAILBOX_CODE0_SWColdReboot_L0) {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
			sys_write32(0x2, CRMU_SOFT_RESET_CTRL);
		}
		/* other mbox handlers can be added here */
		else {
			MCU_SET_STATE_IN_MBOX_ENTRY_C();
		}
	}

	if (need_wakeup) {
		MCU_SET_STATE_IN_MBOX_ENTRY_C();
		/*Setup wakeup sources independently*/
		MCU_setup_wakeup_src(MPROC_ENTER_LP);
	}

	/* Don't change any wakeup related configurations from now,
	 * i.e. led debug !!!!!
	 */
	MCU_SET_STATE_IN_MBOX_ENTRY_C();

	return 1;
}
