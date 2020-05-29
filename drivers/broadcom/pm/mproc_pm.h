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
#ifndef _MPROC_PM_H
#define _MPROC_PM_H

#include <string.h>
#include <drivers/system_timer.h>
#include <board.h>
#include <sw_isr_table.h>
#include "M0_mproc_common.h"
#include "bbl.h"
#include "logging/sys_log.h"

/* FIXME: Once this driver is migrated to Zephyr, socregs.h will not be
 * available and the following define can be removed and sotp.h can be included
 * Including sotp.h now, results in redefinition errors.
 */
u32_t sotp_read_chip_id(void);

#define MPROC_PM_DEBUG

/*A7 IRQ number*/
#define MPROC_INTR__MAILBOX_FROM_MCU                170
#define MPROC_INTR__MAILBOX_FROM_MCU_PRIORITY_LEVEL 0

#define CHIP_STATE_UNPROGRAMMED           0x1
#define CHIP_STATE_UNASSIGNED             0x2
#define CHIP_STATE_NONAB                  0x4
#define CHIP_STATE_AB_PROD                0x10
#define CHIP_STATE_AB_DEV                 0x20

/**-------------------------------------------------------------------------*
 *  DMU data structures & functions brought over from Lynx - Begin.
 *-------------------------------------------------------------------------*/
/**
 * @brief    DMU indirect ISR install info
 * @details The install info of one indirect ISR
 */
typedef struct dmu_mcu_isr_bin_info {
	/**< user ISR number */
	u32_t irqno;
	/**< user ISR install addr, 0 means no new ISR installed for this IRQ */
	u32_t isr_entry;
	/**< real ISR size, 0 means no new ISR installed for this IRQ */
	u32_t isr_len;
} DMU_MCU_ISR_INSTALL_INFO_s;

/**
 * @brief   Citadel chip states
 * @details Citadel chip states
 * from SBL Code:
 * #define CHIP_STATE_UNPROGRAMMED           0x1
 * #define CHIP_STATE_UNASSIGNED             0x2
 * #define CHIP_STATE_NONAB                  0x4
 * #define CHIP_STATE_AB_PROD                0x10
 * #define CHIP_STATE_AB_DEV                 0x20
 */
typedef enum citadel_soc_states {
	CITADEL_STATE__unknown = 0,
	CITADEL_STATE__unprogrammed,
	CITADEL_STATE__unassigned,
	CITADEL_STATE__noAB,
	CITADEL_STATE__AB_PROD,
	CITADEL_STATE__AB_DEV,
} CITADEL_SOC_STATES_e;

/**
 * @brief Read SOC Revision
 * @details TODO
 * -- TODO -- *****-
 * 1. Verify if this is algorithm is correct.
 *
 * @param SOC Id
 *
 * @return soc revision
 */
CITADEL_REVISION_e dmu_get_soc_revision(u32_t *soc_id)
{
	static CITADEL_REVISION_e rev = CITADEL_REV__UNKNOWN;
	static s32_t init;
	static u32_t socid;
	u32_t chip_rev;
	char bcm_soc_rev[3];

	*soc_id = socid;

	if (init)
		return rev;

	init = 1;

	socid = sotp_read_chip_id();
	chip_rev = socid & 0xFF;
	*soc_id = socid;
	bcm_soc_rev[0] = (((chip_rev == 0x1) ? 'A' : (chip_rev == 0x11) ? 'B' : '0'));
	bcm_soc_rev[1] = '0';
	bcm_soc_rev[2] = '\0';

	SYS_LOG_DBG("SOC Revision: %x!", socid);

	switch (bcm_soc_rev[0]) {
	case 'A':
		rev = CITADEL_REV__AX;      break;
	case 'B':
		rev = CITADEL_REV__BX;      break;
	default:
		SYS_LOG_DBG("Invalid soc rev: %c!", bcm_soc_rev[0]);
		rev = CITADEL_REV__UNKNOWN;
	}

	return rev;
}

/**
 * @brief Get SOC Revision Decsription
 * @details Get SOC Revision Decsription
 *
 * @param SOC Id
 *
 * @return soc revision description string
 */
char *dmu_get_soc_revision_desc(CITADEL_REVISION_e soc_rev)
{
	switch (soc_rev) {
	case CITADEL_REV__AX:
		return "AX";
	case CITADEL_REV__BX:
		return "BX";
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief Read SOC State
 * @details Assigned the enumerated soc states based on what bit is set
 *          in the SOTP register
 *
 * @param none
 *
 * @return soc state
 */
CITADEL_SOC_STATES_e dmu_get_soc_state(void)
{
	static u32_t init;
	static CITADEL_SOC_STATES_e soc_state = CITADEL_STATE__unknown;
	u32_t value = 0;

	if (init)
		return soc_state;

	init = 1;

	/* method 1: */
	value = sys_read32(SOTP_REGS_SOTP_CHIP_STATES);
	switch (value) {
	case CHIP_STATE_UNPROGRAMMED:
		return CITADEL_STATE__unprogrammed;
	case CHIP_STATE_UNASSIGNED:
		return CITADEL_STATE__unassigned;
	case CHIP_STATE_NONAB:
		return CITADEL_STATE__noAB;
	case CHIP_STATE_AB_PROD:
		return CITADEL_STATE__AB_PROD;
	case CHIP_STATE_AB_DEV:
		return CITADEL_STATE__AB_DEV;
	default:
		SYS_LOG_ERR("Invalid soc state: %d", value);
		return CITADEL_STATE__unknown;
	}
}

/**
 * @brief Read SOC State Description
 * @details Return a string containing SOC state description
 *
 * @param SOC state
 *
 * @return soc state string
 */
char *dmu_get_soc_state_desc(CITADEL_SOC_STATES_e state)
{
	switch (state) {
	case CITADEL_STATE__unprogrammed:
		return "unprogrammed";
	case CITADEL_STATE__unassigned:
		return "unassinged";
	case CITADEL_STATE__noAB:
		return "noAB";
	case CITADEL_STATE__AB_PROD:
		return "AB_PROD";
	case CITADEL_STATE__AB_DEV:
		return "AB_DEV";
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief Clear MCU (M0) Interrupt
 * @details Clear MCU (M0) Interrupt
 *
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 * Is this to clear pending Interrupts?
 *
 * @param IRQ Id
 *
 * @return None
 */
void dmu_clear_mcu_interrupt(DMU_MCU_IRQ_ID_e id)
{
	if (id == MCU_DEC_ERR_INTR
		|| (id >= MCU_ERROR_LOG_INTR && id <= MCU_SECURITY_INTR))
		sys_set_bit(CRMU_MCU_INTR_CLEAR, id);
}

/**
 * @brief Set MCU (M0) Interrupt
 * @details Set MCU (M0) Interrupt
 *
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 *
 * @param IRQ Id
 * @param Enable / Disable
 *
 * @return None
 */
void dmu_set_mcu_interrupt(DMU_MCU_IRQ_ID_e id, int en)
{
	if (en)
		sys_clear_bit(CRMU_MCU_INTR_MASK, id);
	else
		sys_set_bit(CRMU_MCU_INTR_MASK, id);
}

/**
 * @brief Clear MCU (M0) Event
 * @details Clear MCU (M0) Event
 *
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 *
 * @param Event Id
 *
 * @return None
 */
void dmu_clear_mcu_event(DMU_MCU_EVENT_ID_e id)
{
	sys_set_bit(CRMU_MCU_EVENT_CLEAR, id);
}

/**
 * @brief Clear MCU (M0) Event
 * @details Clear MCU (M0) Event
 *
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 *
 * @param Event Id
 * @param Enable / Disable
 *
 * @return None
 */
void dmu_set_mcu_event(DMU_MCU_EVENT_ID_e id, int en)
{
	if (en)
		sys_clear_bit(CRMU_MCU_EVENT_MASK, id);
	else
		sys_set_bit(CRMU_MCU_EVENT_MASK, id);
}

/**
 * @brief Send MBOX Message to CRMU / MCU (M0)
 * @details Send MBOX Message to CRMU / MCU (M0)
 * code0=0, code1=0xffffffff: reset whole chip
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 *
 * @param code 0
 * @param code 1
 *
 * @return None
 */
void dmu_send_mbox_msg_to_mcu(u32_t code0, u32_t code1)
{
	sys_write32(code0, IPROC_CRMU_MAIL_BOX0);
	sys_write32(code1, IPROC_CRMU_MAIL_BOX1);
}

/**
 * @brief Set CRMU / MCU (M0) Access Permission
 * @details Set CRMU / MCU (M0) Access Permission
 * @type:
 * 00, 11 = non secure
 * 01     = use address window
 * 10     = force secure mode.
 *
 * -- TODO -- ***** - Verify if this is the right CRMU register?
 * Need to understand the what are the access types and what do they mean.
 *
 * @param type
 *
 * @return None
 */
void dmu_set_mcu_access_permission(u32_t type)
{
	sys_write32(type, CRMU_MCU_ACCESS_CONTROL);
}

/**
 * For Quick Reference
 *
 * CRMU to IPROC (A7) Interrupts that may be triggered while A7
 * is sleeping (Powered Off), which also means that it won't pend
 * on GIC.
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_STATUS 0x30024068
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_STATUS__IPROC_USBPHY1_WAKE_INTR 19
 * #define CRMU_IPROC_INTR_STATUS__IPROC_USBPHY0_WAKE_INTR 18
 * #define CRMU_IPROC_INTR_STATUS__IPROC_VOLTAGE_GLITCH_TAMPER_INTR 17
 * #define CRMU_IPROC_INTR_STATUS__IPROC_CRMU_ECC_TAMPER_INTR 16
 * #define CRMU_IPROC_INTR_STATUS__IPROC_RTIC_TAMPER_INTR 15
 * #define CRMU_IPROC_INTR_STATUS__IPROC_CHIP_FID_TAMPER_INTR 14
 * #define CRMU_IPROC_INTR_STATUS__IPROC_CRMU_FID_TAMPER_INTR 13
 * #define CRMU_IPROC_INTR_STATUS__IPROC_CRMU_CLK_GLITCH_TAMPER_INTR 12
 * #define CRMU_IPROC_INTR_STATUS__IPROC_BBL_CLK_GLITCH_TAMPER_INTR 11
 * #define CRMU_IPROC_INTR_STATUS__IPROC_IHOST_CLK_GLITCH_TAMPER_INTR 10
 * #define CRMU_IPROC_INTR_STATUS__IPROC_M0_LOCKUP_INTR 9
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPRU_RTC_PERIODIC_INTR 8
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPRU_RTC_INTR 7
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPRU_ALARM_INTR 6
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPL_FREQ_INTR 5
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPL_PVT_INTR 4
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPL_RST_INTR 3
 * #define CRMU_IPROC_INTR_STATUS__IPROC_SPL_WDOG_INTR 2
 * #define CRMU_IPROC_INTR_STATUS__IPROC_AXI_DEC_ERR_INTR 1
 * #define CRMU_IPROC_INTR_STATUS__IPROC_MAILBOX_INTR 0
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_MASK 0x3002406c
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_MASK__IPROC_USBPHY1_WAKE_INTR_MASK 19
 * #define CRMU_IPROC_INTR_MASK__IPROC_USBPHY0_WAKE_INTR_MASK 18
 * #define CRMU_IPROC_INTR_MASK__IPROC_VOLTAGE_GLITCH_TAMPER_INTR_MASK 17
 * #define CRMU_IPROC_INTR_MASK__IPROC_CRMU_ECC_TAMPER_INTR_MASK 16
 * #define CRMU_IPROC_INTR_MASK__IPROC_RTIC_TAMPER_INTR_MASK 15
 * #define CRMU_IPROC_INTR_MASK__IPROC_CHIP_FID_TAMPER_INTR_MASK 14
 * #define CRMU_IPROC_INTR_MASK__IPROC_CRMU_FID_TAMPER_INTR_MASK 13
 * #define CRMU_IPROC_INTR_MASK__IPROC_CRMU_CLK_GLITCH_TAMPER_INTR_MASK 12
 * #define CRMU_IPROC_INTR_MASK__IPROC_BBL_CLK_GLITCH_TAMPER_INTR_MASK 11
 * #define CRMU_IPROC_INTR_MASK__IPROC_IHOST_CLK_GLITCH_TAMPER_INTR_MASK 10
 * #define CRMU_IPROC_INTR_MASK__IPROC_M0_LOCKUP_INTR_MASK 9
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_PERIODIC_INTR_MASK 8
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_INTR_MASK 7
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPRU_ALARM_INTR_MASK 6
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPL_FREQ_INTR_MASK 5
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPL_PVT_INTR_MASK 4
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPL_RST_INTR_MASK 3
 * #define CRMU_IPROC_INTR_MASK__IPROC_SPL_WDOG_INTR_MASK 2
 * #define CRMU_IPROC_INTR_MASK__IPROC_AXI_DEC_ERR_INTR_MASK 1
 * #define CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK 0
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_CLEAR 0x30024070
 *----------------------------------------------------------
 * #define CRMU_IPROC_INTR_CLEAR__IPROC_USBPHY1_WAKE_INTR_CLR 19
 * #define CRMU_IPROC_INTR_CLEAR__IPROC_USBPHY0_WAKE_INTR_CLR 18
 * #define CRMU_IPROC_INTR_CLEAR__IPROC_CHIP_FID_TAMPER_INTR_CLR 14
 * #define CRMU_IPROC_INTR_CLEAR__IPROC_CRMU_FID_TAMPER_INTR_CLR 13
 * #define CRMU_IPROC_INTR_CLEAR__IPROC_AXI_DEC_ERR_INTR_CLR 1
 *----------------------------------------------------------
 *
 * #define  IRQ34 CHIP_INTR__CRMU_MPROC_M0_LOCKUP_INTERRUPT
 * #define  IRQ33 CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT
 * #define  IRQ32 CHIP_INTR__CRMU_MPROC_AXI_DEC_ERR_INTERRUPT
 * #define  IRQ31 CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT
 * #define  IRQ30 CHIP_INTR__CRMU_MPROC_SPL_WDOG_INTERRUPT
 * #define  IRQ29 CHIP_INTR__CRMU_MPROC_SPL_RST_INTERRUPT
 * #define  IRQ28 CHIP_INTR__CRMU_MPROC_SPL_PVT_INTERRUPT
 * #define  IRQ27 CHIP_INTR__CRMU_MPROC_SPL_FREQ_INTERRUPT
 * #define  IRQ26 CHIP_INTR__CRMU_MPROC_SPRU_ALARM_EVENT_INTERRUPT
 * #define  IRQ25 CHIP_INTR__CRMU_MPROC_SPRU_RTC_EVENT_INTERRUPT
 * #define  IRQ24 CHIP_INTR__CRMU_MPROC_IHOST_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ23 CHIP_INTR__CRMU_MPROC_BBL_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ22 CHIP_INTR__CRMU_MPROC_CRMU_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ21 CHIP_INTR__CRMU_MPROC_CRMU_FID_TAMPER_INTR
 * #define  IRQ20 CHIP_INTR__CRMU_MPROC_CHIP_FID_TAMPER_INTR
 * #define  IRQ19 CHIP_INTR__CRMU_MPROC_RTIC_TAMPER_INTR
 * #define  IRQ18 CHIP_INTR__CRMU_MPROC_CRMU_ECC_TAMPER_INTR
 * #define  IRQ17 CHIP_INTR__CRMU_MPROC_VOLTAGE_GLITCH_TAMPER_INTR
 * #define  IRQ16 CHIP_INTR__CRMU_MPROC_INTERRUPT
 */
/**
 * @brief Disable all ISRs
 * @details Disable all ISRs
 *
 * -- TODO -- ***** - Verify if this is right?
 *
 * Is it OK to do a "cpsied if"? May be not, because this will allow the
 * interrupts that occur during the sleep, to stay in pending state. So
 * when we re-enable the interrupts while waking up, those pending interrupts
 * will be taken, which may not be what was desired in the first place.
 *
 * Total possible IRqs is 160
 *
 * The following is the list of IRQs for reference:
 * #define  IRQ86 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT5
 * #define  IRQ85 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT4
 * #define  IRQ84 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT3
 * #define  IRQ83 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT2
 * #define  IRQ82 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT1
 * #define  IRQ81 CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT0
 * #define  IRQ80 CHIP_INTR__IHOST_COMMTX_INTERRUPT_BIT1
 * #define  IRQ79 CHIP_INTR__IHOST_COMMTX_INTERRUPT_BIT0
 * #define  IRQ78 CHIP_INTR__IHOST_COMMRX_INTERRUPT_BIT1
 * #define  IRQ77 CHIP_INTR__IHOST_COMMRX_INTERRUPT_BIT0
 * #define  IRQ76 CHIP_INTR__IOSYS_SDIO_INTERRUPT
 * #define  IRQ75 CHIP_INTR__IOSYS_UNICAM_INTERRUPT
 * #define  IRQ74 CHIP_INTR__IOSYS_TIM7_INTERRUPT
 * #define  IRQ73 CHIP_INTR__IOSYS_TIM6_INTERRUPT
 * #define  IRQ72 CHIP_INTR__IOSYS_TIM5_INTERRUPT
 * #define  IRQ71 CHIP_INTR__IOSYS_TIM4_INTERRUPT
 * #define  IRQ70 CHIP_INTR__IOSYS_USB2D_INTERRUPT
 * #define  IRQ69 CHIP_INTR__IOSYS_FSK_INTERRUPT
 * #define  IRQ68 CHIP_INTR__IOSYS_FLEX_TIMER_INTERRUPT
 * #define  IRQ67 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT5
 * #define  IRQ66 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT4
 * #define  IRQ65 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT3
 * #define  IRQ64 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT2
 * #define  IRQ63 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT1
 * #define  IRQ62 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT0
 * #define  IRQ61 CHIP_INTR__IOSYS_KEYPAD_INTERRUPT
 * #define  IRQ60 CHIP_INTR__IOSYS_RNG_INTERRUPT
 * #define  IRQ59 CHIP_INTR__IOSYS_TIM3_INTERRUPT
 * #define  IRQ58 CHIP_INTR__IOSYS_TIM2_INTERRUPT
 * #define  IRQ57 CHIP_INTR__IOSYS_TIM1_INTERRUPT
 * #define  IRQ56 CHIP_INTR__IOSYS_TIM0_INTERRUPT
 * #define  IRQ55 CHIP_INTR__MPROC_CRMU_FID_INTERRUPT
 * #define  IRQ54 CHIP_INTR__IOSYS_GPIO_INTERRUPT
 * #define  IRQ53 CHIP_INTR__IOSYS_SPI5_INTERRUPT
 * #define  IRQ52 CHIP_INTR__IOSYS_SPI4_INTERRUPT
 * #define  IRQ51 CHIP_INTR__IOSYS_SPI3_INTERRUPT
 * #define  IRQ50 CHIP_INTR__IOSYS_SPI2_INTERRUPT
 * #define  IRQ49 CHIP_INTR__IOSYS_SPI1_INTERRUPT
 * #define  IRQ48 CHIP_INTR__IOSYS_UART3_INTERRUPT
 * #define  IRQ47 CHIP_INTR__IOSYS_UART2_INTERRUPT
 * #define  IRQ46 CHIP_INTR__IOSYS_UART1_INTERRUPT
 * #define  IRQ45 CHIP_INTR__IOSYS_UART0_INTERRUPT
 * #define  IRQ44 CHIP_INTR__IOSYS_USB2H_INTERRUPT
 * #define  IRQ43 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT2
 * #define  IRQ42 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT1
 * #define  IRQ41 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT0
 * #define  IRQ40 CHIP_INTR__IOSYS_QSPI_INTERRUPT
 * #define  IRQ39 CHIP_INTR__IOSYS_SMC_INTERRUPT
 * #define  IRQ38 CHIP_INTR__IOSYS_WDOG_INTERRUPT
 * #define  IRQ37 CHIP_INTR__NFC_HOST_WAKE
 * #define  IRQ36 CHIP_INTR__NFC_SPI_INT
 * #define  IRQ35 CHIP_INTR__IDM_ERROR_INTERRUPT
 * #define  IRQ34 CHIP_INTR__CRMU_MPROC_M0_LOCKUP_INTERRUPT
 * #define  IRQ33 CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT
 * #define  IRQ32 CHIP_INTR__CRMU_MPROC_AXI_DEC_ERR_INTERRUPT
 * #define  IRQ31 CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT
 * #define  IRQ30 CHIP_INTR__CRMU_MPROC_SPL_WDOG_INTERRUPT
 * #define  IRQ29 CHIP_INTR__CRMU_MPROC_SPL_RST_INTERRUPT
 * #define  IRQ28 CHIP_INTR__CRMU_MPROC_SPL_PVT_INTERRUPT
 * #define  IRQ27 CHIP_INTR__CRMU_MPROC_SPL_FREQ_INTERRUPT
 * #define  IRQ26 CHIP_INTR__CRMU_MPROC_SPRU_ALARM_EVENT_INTERRUPT
 * #define  IRQ25 CHIP_INTR__CRMU_MPROC_SPRU_RTC_EVENT_INTERRUPT
 * #define  IRQ24 CHIP_INTR__CRMU_MPROC_IHOST_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ23 CHIP_INTR__CRMU_MPROC_BBL_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ22 CHIP_INTR__CRMU_MPROC_CRMU_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ21 CHIP_INTR__CRMU_MPROC_CRMU_FID_TAMPER_INTR
 * #define  IRQ20 CHIP_INTR__CRMU_MPROC_CHIP_FID_TAMPER_INTR
 * #define  IRQ19 CHIP_INTR__CRMU_MPROC_RTIC_TAMPER_INTR
 * #define  IRQ18 CHIP_INTR__CRMU_MPROC_CRMU_ECC_TAMPER_INTR
 * #define  IRQ17 CHIP_INTR__CRMU_MPROC_VOLTAGE_GLITCH_TAMPER_INTR
 * #define  IRQ16 CHIP_INTR__CRMU_MPROC_INTERRUPT
 * #define  IRQ15 CHIP_INTR__MPROC_DMA_ERR0_INTERRUPT
 * #define  IRQ14 CHIP_INTR__MPROC_DMA_TC0_INTERRUPT
 * #define  IRQ13 CHIP_INTR__MPROC_IDC_LPERROR_INTERRUPT
 * #define  IRQ12 CHIP_INTR__MPROC_IDC_HPERROR_INTERRUPT
 * #define  IRQ11 CHIP_INTR__MPROC_ROM_WRINVALID_ERROR
 * #define  IRQ10 CHIP_INTR__MPROC_ROM_DECODER_ERROR
 * #define  IRQ9  CHIP_INTR__MPROC_SMU_SPI_SLV_ERROR
 * #define  IRQ8  CHIP_INTR__MPROC_SMU_AUTH_ERROR
 * #define  IRQ7  CHIP_INTR__MPROC_SMU_PARITY_ERROR
 * #define  IRQ6  CHIP_INTR__MPROC_SMU_INTERRUPT
 * #define  IRQ5  CHIP_INTR__MPROC_SRAM_ACCESS_VIO_INTERRUPT
 * #define  IRQ4  CHIP_INTR__MPROC_SRAM_MEM_OUT_OF_RANGE_INTERRUPT
 * #define  IRQ3  CHIP_INTR__SRAM_AXI_ERR_INTERRUPT
 * #define  IRQ2  CHIP_INTR__MPROC_SRAM_MEM_UNCORRECTABLE_INTERRUPT
 * #define  IRQ1  CHIP_INTR__MPROC_SRAM_MEM_CORRECTABLE_INTERRUPT
 * #define  IRQ0  CHIP_INTR__MPROC_PKA_DONE_INTERRUPT
 *
 *
 * @param None
 *
 * @return None
 */

void bcm_sync(void)
{
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
}
/**-------------------------------------------------------------------------*
 *  DMU data structures & functions brought over from Lynx - End.
 *--------------------------------------------------------------------------*/

#ifdef MPROC_PM_DEBUG
#define mproc_dbg(format, arg...) \
	SYS_LOG_DBG("[PM@%4u] "format, __LINE__, ##arg)
#define mproc_err(format, arg...) \
	SYS_LOG_ERR("[PM@%4u] "format, __LINE__, ##arg)
#define mproc_prt(format, arg...) \
	SYS_LOG_INF("[PM@%4u] "format, __LINE__, ##arg)
#else
#define mproc_dbg(format, arg...)	do {} while (0)
#define mproc_prt(format, arg...)	SYS_LOG_INF(format, ##arg)
#define mproc_err(format, arg...)	SYS_LOG_ERR(format, ##arg)
#endif

#define CITADEL_SHOW_LINE()			mproc_dbg("");

#define CITADEL_PM_DO_POLICY		0

/* Don't use jtag as setting breakpoint will change the mem of first
 * bytes at breakpoint. MCU jtag will not stop at breakpoint if usr
 * bin re-copied, so, skip checking the first bytes
 */
#define CITADEL_PM_CHECK_MCU_ISR
/* #undef CITADEL_PM_CHECK_MCU_ISR */
#define CITADEL_PM_CHECK_MCU_ISR_IGNORE_LEN		4

typedef enum mproc_console_baudrate {
	MPROC_CONSOLE_BAUDRATE__UNCHANGE = 0,
	MPROC_CONSOLE_BAUDRATE__19200	= 19200,
	MPROC_CONSOLE_BAUDRATE__38400	= 38400,
	MPROC_CONSOLE_BAUDRATE__115200	= 115200,
} MPROC_CONSOLE_BAUDRATE_e;

#endif /* _MPROC_PM_H */
