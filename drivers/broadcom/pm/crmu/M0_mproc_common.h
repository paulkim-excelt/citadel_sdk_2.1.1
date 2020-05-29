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

#ifndef _M0_MPROC_COMMON_H
#define _M0_MPROC_COMMON_H

/* Including the pm.h api header file from Zephyr repo
 * results in the inclusion of device.h which in turn
 * includes a lot of zephyr system header files which
 * result in compile errors, since not all zephyr env/config
 * make variables are set for M0 build. To avoid this, we
 * define ZEPHYR_INCLUDE_DEVICE_H_, which will skip the
 * inclusion of device.h contents. The only definition that
 * is really needed from device.h for pm.h to compile is
 * the definition for struct device, which we define locally
 * here
 */
#ifdef DEFINE_LOCAL_STRUCT_DEVICE
#define ZEPHYR_INCLUDE_DEVICE_H_
struct device {
	struct device_config *config;
	const void *driver_api;
	void *driver_data;
};
#endif

#include "pm_regs.h"
#include "zephyr/types.h"
#include <arch/cpu.h>
#include "pm.h"
#include "ihost_config.h"
#include <misc/util.h>

#define cpu_reg32_getbit(addr, bit) \
		((sys_read32(addr) >> (bit)) & 0x1)

/*===================================================================*
 *  Mailbox Commands to A7 from M0  *
 * FIXME: Reconcile these codes (applicable for CS Application) with
 * existing codes for other application. Update M3 to A7 after
 * discussing with Shuanglin.
 *===================================================================*/
#define MAILBOX_CODE1_NOTIFY_M3                0xFFFF0000
#define MAILBOX_CODE1_PM                       0x0000FFFF
#define MAILBOX_CODE0_mProcPowerCMDMask        0xC7FFFFFF
#define MAILBOX_CODE0_mProcWakeup              0x10000000
#define MAILBOX_CODE0_mProcSleep               0x20000000
#define MAILBOX_CODE0_mProcDeepSleep           0x08000000
#define MAILBOX_CODE0_mProcSoftResetAndResume  0x40000000
#define MAILBOX_CODE0_mProcSMBUS               0x80000000
#define MAILBOX_CODE0_mProcFP                  0x04000000

/** MPROC (A7) -> M0 codes:
 * All these signals are from A7 to the M0 using mailbox messages.
 * Every signal takes two codes Code0 & Code1.
 */
/* Run the A7 at 1GHz */
#define MAILBOX_CODE0__RUN_1000                0x000a1001
/* Run the A7 at 500MHz */
#define MAILBOX_CODE0__RUN_500                 0x000a1002
/* Run the A7 at 200MHz */
#define MAILBOX_CODE0__RUN_200                 0x000a1003
#ifdef MPROC_PM__DPA_CLOCK
/* For Preventing Differential Power Analysis Attacks */
/* Run the A7 at 187.5MHz */
#define MAILBOX_CODE0__RUN_187                 0x000a1004
#endif
/* Sleep Mode: Clock Gated */
#define MAILBOX_CODE0__SLEEP                   0x000a1010
/* DRIPS Mode: Clock Gated - Low Leakage*/
#define MAILBOX_CODE0__DRIPS                   0x000a1020
/* DeepSleep Mode: All Pwrd Dwn */
#define MAILBOX_CODE0__DEEPSLEEP               0x000a1030
/* Common Code 1 for all PM Messages */
#define MAILBOX_CODE1__PM                      0x10000000

/* Special Mailbox Codes To Issue System Reboot */
#define MAILBOX_CODE0_SYSTEM_REBOOT            0xdeadbeef
#define MAILBOX_CODE1_SYSTEM_REBOOT            0xdeadbeef

/* Special Mailbox Codes To Issue System Reset - Used by SBL */
#define MAILBOX_CODE0_SysSoftReset             0x00000000
#define MAILBOX_CODE1_SysSoftReset             0xffffffff

#define MAILBOX_CODE0_CrmuWdtReset_L1          0x00000000
#define MAILBOX_CODE1_CrmuWdtReset_L1          0xfffffffe

#define MAILBOX_CODE0_SWWarmReboot_L0          0x00000000
#define MAILBOX_CODE1_SWWarmReboot_L0          0xfffffffd

#define MAILBOX_CODE0_SWColdReboot_L0          0x00000000
#define MAILBOX_CODE1_SWColdReboot_L0          0xfffffffc

/** M0 -> MPROC (A7) codes:
 * All these signals are from M0 to the A7 using mailbox messages.
 * Every signal takes two codes Code0 & Code1.
 *
 * 1. TODO: What other wake-up sources must be added to CRMU?
 * 2. TODO: What other MailBox Messages must be added (both ways)?
 *
 * A7 Watchdog Interrupt: When the system wide Wdog Timer expires it issues
 * two signals: (i) an Interrupt that is connected to the GIC IRQ lines and
 * therefore eventually arrives at A7 as an IRQ. (ii) a Reset Signal that is
 * connected to the IRQ line of CRMU / M0 (i.e. via NVIC). So none of these
 * signals directly trigger a reset event, however, the CRMU, as a part of
 * the handling of this interrupt from the WDT, can issue a Reset to the A7.
 *
 * The idea is to reset A7 gracefully if at all. But since A7 is not able
 * to process interrupts gracefully due to either it being asleep, in DRIPS
 * etc., the idea is to have M0 start the POR sequence on A7 and eventually,
 * send a mail-box interrupt to A7 informing A7 of the cause of the reset.
 *
 * 2. The SMBUS interrupt is actually an M0 interrupt, but why is it being
 *    handled by A7?
 * The reason is M0 doesn't have sufficient processing power. The SMBUS
 * interrupt comes from some peripheral upon detection os some events.
 * e.g. A motion sensor to trigger camera recording. M0 signals the A7
 * to start a full fledged processing.
 *
 */
#define MAILBOX_CODE0__NOTHING                 0x00000000
#define MAILBOX_CODE1__NOTHING                 0x00000000

/* Message Codes for All Wake Up Sources */
/* Common Code 0 for Wake Up from LP mode */
#define MAILBOX_CODE0__WAKEUP                  0x11223344
/* Code 1 Indicates the Wake Up Source */
#define MAILBOX_CODE1__WAKEUP_TEST             0x44332211
/* The following 11 AON GPIOs are mapped to external interrupts
 * to the M0 on any of the AON GPIO lines.
 * which gets triggered when CRMU_MCU_INTR_MASK[MCU_AON_GPIO_INTR = 2]
 * is clear.
 */
#define MAILBOX_CODE1__WAKEUP_AON_GPIO0        0x00000000
#define MAILBOX_CODE1__WAKEUP_AON_GPIO1        0x00000001
#define MAILBOX_CODE1__WAKEUP_AON_GPIO2        0x00000002
#define MAILBOX_CODE1__WAKEUP_AON_GPIO3        0x00000003
#define MAILBOX_CODE1__WAKEUP_AON_GPIO4        0x00000004
#define MAILBOX_CODE1__WAKEUP_AON_GPIO5        0x00000005
#define MAILBOX_CODE1__WAKEUP_AON_GPIO6        0x00000006
#define MAILBOX_CODE1__WAKEUP_AON_GPIO7        0x00000007
#define MAILBOX_CODE1__WAKEUP_AON_GPIO8        0x00000008
#define MAILBOX_CODE1__WAKEUP_AON_GPIO9        0x00000009
#define MAILBOX_CODE1__WAKEUP_AON_GPIO10       0x0000000a
/* FIXME: Don't know how this is received by the M0 */
#define MAILBOX_CODE1__WAKEUP_NFC              0x0000000b
/* We will send USBD0 Event Message Upon:
 * USBPHY0 Filter match event (MCU_USBPHY0_FILTER_EVENT = 13)
 * OR USBPHY0 Wake event (MCU_USBPHY0_WAKE_EVENT = 12)
 *
 * We will send USBD1 Event Message Upon:
 * USBPHY1 Filter match event (MCU_USBPHY1_FILTER_EVENT = 15)
 * OR USBPHY1 Wake event (MCU_USBPHY1_WAKE_EVENT = 14)
 */
#define MAILBOX_CODE1__WAKEUP_USB0_WAKE        0x0000000c
#define MAILBOX_CODE1__WAKEUP_USB0_FLTR        0x0000000d
#define MAILBOX_CODE1__WAKEUP_USB1_WAKE        0x0000000e
#define MAILBOX_CODE1__WAKEUP_USB1_FLTR        0x0000000f
/* FIXME: Don't know how this is received by the M0 */
#define MAILBOX_CODE1__WAKEUP_EC_COMMAND       0x00000010
/* SPRU Alarm */
#define MAILBOX_CODE1__WAKEUP_TAMPER           0x00000011
/* SMBUS INTR Interrupt to MCU message upon:
 * MCU_SMBUS_INTR = 6
 */
#define MAILBOX_CODE1__WAKEUP_SMBUS            0x00000012
/* SPRU event Message is sent upon:
 * MCU_SPRU_RTC_EVENT = 27 or MCU_SPRU_RTC_PERIODIC_EVENT = 28
 */
#define MAILBOX_CODE1__WAKEUP_RTC              0x00000013

/**
 * TODO (Clarify from Kavya): What is the difference between
 * the two types of Messages involving AON GPIOs.
 *
 * We can create separate messages for each GPIO separately.
 * e.g. GPIOx can have the GPIO number encoded in the message.
 * Explore / Discuss and implement.
 */
#define MAILBOX_CODE0__NONE_WAKEUP_AON_GPIO    0x4f495047
#define MAILBOX_CODE1__NONE_WAKEUP_AON_GPIO    0x00000001
#define MAILBOX_CODE0__AON_GPIO_INTR_IN_RUN    0x11003300
#define MAILBOX_CODE1__AON_GPIO_INTR_IN_RUN    0x44002200

#define MAILBOX_CODE0__USB_INTR_IN_RUN         0x22005500
#define MAILBOX_CODE1__USB_INTR_IN_RUN         0x77003300
/* Clarify BBL stuff */
#define MAILBOX_CODE0__TAMPER_INTR_IN_RUN      0x21436587
#define MAILBOX_CODE1__TAMPER_INTR_IN_RUN      0x78563412
#ifdef MPROC_PM__A7_WDOG_RESET
#define MAILBOX_CODE0__SMBUS_ISR               0xaabbccdd
#define MAILBOX_CODE1__SMBUS_ISR               0x00000001
#endif

/** MCU job status: TODO: What is the purpose of these?
 * These don't seem to be accessed anywhere.
 */
#ifdef MPROC_PM_WAIT_FOR_MCU_JOB_DONE
#define MCU_JOB_PROGRESS__DONE                 0x00000000
#define MCU_JOB_PROGRESS__REBOOT_START         0x00000001
#endif

/**
 *----------------------------------------------------------------------------*
 *---FIXME: Start of Definitions Brought Over from erstwhile DMU.h in Lynx ---*
 *----------------------------------------------------------------------------*
 */

/**
 * @brief   Citadel chip version number
 * @details Citadel chip version number
 */
typedef enum citadel_revision {
	CITADEL_REV__UNKNOWN = 0,
	CITADEL_REV__AX,
	CITADEL_REV__BX,
} CITADEL_REVISION_e;

/**
 * @brief   Citadel genpll channel id enum
 * @details Only six channels (Basically independently controllable clocks)
 */
typedef enum citadel_genpll_channel {
	CITADEL_GENPLL_CH1 = 0, /**< CLK21, CPUCLK */
	CITADEL_GENPLL_CH2,     /**< CLK41, PCLK */
	CITADEL_GENPLL_CH3,
	CITADEL_GENPLL_CH4,     /**< QSPI */
	CITADEL_GENPLL_CH5,
	CITADEL_GENPLL_CH6
} CITADEL_GENPLL_CH_e;

/**
 *----------------------------------------------------------------------------*
 *---FIXME: End of Definitions Brought Over from erstwhile DMU.h in Lynx -----*
 *----------------------------------------------------------------------------*
 */

/* Persistent Register that indicates:
 * Bits 0 - 7:  Power Manager State
 * Bits 8 - 15: Whether Low Power Mode Entry or Exit (0x55: Exit, 0xAA: Entry)
 *
 */
#define DRIPS_OR_DEEPSLEEP_REG	 CRMU_IHOST_SW_PERSISTENT_REG11
#define ENTRY_EXIT_MASK_SET		(0x0000FF00)
#define PM_STATE_MASK_SET		(0x000000FF)
#define ENTRY_EXIT_MASK 		(~(0x0000FF00))
#define PM_STATE_MASK			(~(0x000000FF))
#define EXITING_LP  			(0x00005500)
#define ENTERING_LP 			(0x0000AA00)
/* To implement a handshake signal to indicate
 * that A7 has woken up from LP mode fully.
 * M0 would wait on this register before attempting
 * to send mailbox message.
 */
#define A7_WOKEN_UP CRMU_SPARE_REG_0

/**
 * The following are 32 IRQs to M0 NVIC which are distributed
 * between M0's ~20 events and ~15 interrupts.
 *
 * ISR Table Base: 0x30018000 (starts at offset 0x0F80 and goes till 0x0FFC)
 *
 */
typedef enum mcu_nvic_irq_id {
	MCU_NVIC_MAXIRQs				= 32,
	MCU_NVIC_MAILBOX_EVENT			= 31, /* [0x0FFC] = MCU_MAILBOX_EVENT */
	MCU_NVIC_IPROC_STANDBYWFI_EVENT = 30, /* [0x0FF8] Tied to zero */
	MCU_NVIC_IPROC_STANDBYWFE_EVENT	= 29, /* [0x0FF4] Tied to zero */
	MCU_NVIC_SPRU_RTC_PERIODIC_EVENT= 28, /* [0x0FF0] */
	MCU_NVIC_SPRU_RTC_EVENT			= 27, /* [0x0FEC] */
	MCU_NVIC_SPRU_ALARM_EVENT		= 26, /* [0x0FE8] Tamper Event */
	MCU_NVIC_SPL_FREQ_EVENT			= 25, /* [0x0FE4] */
	MCU_NVIC_SPL_PVT_EVENT			= 24, /* [0x0FE0] */
	MCU_NVIC_SPL_RST_EVENT			= 23, /* [0x0FDC] */
	MCU_NVIC_SPL_WDOG_EVENT			= 22, /* [0x0FD8] */
	MPROC_CRMU_EVENT5				= 21, /* [0x0FD4] Tied to zero */
	MPROC_CRMU_EVENT4				= 20, /* [0x0FD0] Tied to zero */
	MPROC_CRMU_EVENT3				= 19, /* [0x0FCC] */
	MPROC_CRMU_EVENT2				= 18, /* [0x0FC8] */
	MPROC_CRMU_EVENT1				= 17, /* [0x0FC4] */
	MPROC_CRMU_EVENT0				= 16, /* [0x0FC0] */
	/* (CRMU_MCU_EVENT_STATUS__MCU_USBPHY1_RESUME_WAKE_EVENT
	 * | CRMU_MCU_EVENT_STATUS__MCU_USBPHY1_FILTER_WAKE_EVENT
	 * | CRMU_MCU_EVENT_STATUS__MCU_USBPHY0_RESUME_WAKE_EVENT
	 * | CRMU_MCU_EVENT_STATUS__MCU_USBPHY0_FILTER_WAKE_EVENT)
	 */
	USBPHY_WAKE_EVENT				= 15, /* [0x0FBC] Any USB Event */
	MCU_NVIC_SECURITY_INTR			= 14, /* [0x0FB8] */
	MCU_NVIC_CLK_GLITCH_INTR		= 13, /* [0x0FB4] */
	MCU_NVIC_VOLTAGE_GLITCH_INTR	= 12, /* [0x0FB0] */
	MCU_NVIC_RESET_LOG_INTR			= 11, /* [0x0FAC] */
	MCU_NVIC_POWER_LOG_INTR			= 10, /* [0x0FA8] */
	MCU_NVIC_ERROR_LOG_INTR			= 9,  /* [0x0FA4] */
	MCU_NVIC_WDOG_INTR				= 8,  /* [0x0FA0] */
	MCU_NVIC_TIMER_INTR				= 7,  /* [0x0F9C] */
	/* ORing of interrupts from smbus,smbus1,smbus2 */
	MCU_NVIC_SMBUS_INTR				= 6,  /* [0x0F98] */
	/* a7_intr_bus interrupt from ihost
	 * ((STANDBYWFIL2 | STANDBYWFE | EVENTO | EVENTI_ack
	 * | nCNTPSIRQ | nCNTPNSIRQ | nCNTHPIRQ | nCNTVIRQ
	 * | nPMUIRQ | nCOMMIRQ | nINTERRIRQ | crm_interrupt))
	 */
	MPROC_CRMU_INTR2				= 5,  /* [0x0F94] */
	/*  nEXTERRIRQ interrupt from ihost */
	MPROC_CRMU_INTR1				= 4,  /* [0x0F90] */
	/* STANDBYWFI interrupt from ihost */
	MPROC_CRMU_INTR0				= 3,  /* [0x0F8C] */
	MCU_NVIC_AON_GPIO_INTR			= 2,  /* [0x0F88] = MCU_AON_GPIO_INTR */
	MCU_NVIC_AON_UART_INTR			= 1,  /* [0x0F84] */
	MCU_NVIC_DEC_ERR_INTR			= 0,  /* [0x0F80] Decode Error = MCU_DEC_ERR_INTR */
} MCU_NVIC_IRQ_ID_e;

/**
 * @brief   MCU (CRMU / M0) irq id enum
 * @details All MCU irq id (These are Inputs to MCU/M0)
 *
 * CRMU_MCU_INTR_STATUS = 0x30024050
 * CRMU_MCU_INTR_CLEAR = 0x30024058
 * CRMU_MCU_INTR_MASK = 0x30024054
 */
typedef enum dmu_mcu_irq_id {
	MCU_SMBUS2_INTR = 16, /**< MCU SMBUS 2 interrupt */
	MCU_SMBUS1_INTR = 15, /**< MCU SMBUS 1 interrupt */
	MCU_SECURITY_INTR = 14, /**< MCU Security interrupt */
	MCU_CLK_GLITCH_INTR = 13, /**< MCU CLK glitch interrupt */
	MCU_VOLTAGE_GLITCH_INTR = 12, /**< MCU Voltage glitch interrupt */
	MCU_RESET_LOG_INTR = 11, /**<  MCU Reset intr */
	MCU_POWER_LOG_INTR = 10, /**<  MCU Power intr */
	MCU_ERROR_LOG_INTR = 9, /**<  ERROR intr to MCU. */
	MCU_WDOG_INTR = 8, /**<  WDOG Interrupt to MCU. */
	MCU_TIMER_INTR = 7, /**<  Timer Interrupt to MCU. */
	MCU_SMBUS_INTR = 6, /**<  SMBUS INTR Interrupt to MCU. */
	MCU_MPROC_CRMU_INTR2 = 5, /**<  mproc_crmu_intr[2] Interrupt to MCU. */
	MCU_MPROC_CRMU_INTR1 = 4, /**<  mproc_crmu_intr[1] Interrupt to MCU. */
	MCU_MPROC_CRMU_INTR0 = 3, /**<  mproc_crmu_intr[0] to MCU. */
	MCU_AON_GPIO_INTR = 2, /**<  External interrupt from AON GPIO to MCU. */
	MCU_AON_UART_INTR = 1, /**<  AON SPI interrupt */
	MCU_DEC_ERR_INTR = 0 /**<  DEC ERR interrupts to MCU. */
} DMU_MCU_IRQ_ID_e;

/**
 * @brief   MCU (CRMU / M0) event id enum
 * @details All MCU event id (These are Inputs to MCU/M0)
 *
 * TODO: How are the events different from Interrupts?
 *
 * Apparently from hardware point of view there is no difference between
 * event and interrupt. In this case, some events are clubbed together
 * to form an interrupt.
 *
 * CRMU_MCU_EVENT_STATUS = 0x3002405c
 * CRMU_MCU_EVENT_CLEAR = 0x30024060
 * CRMU_MCU_EVENT_MASK = 0x30024064
 */
typedef enum dmu_mcu_event_id {
	MCU_MAILBOX_EVENT = 31, /**< Mail box event */
	/**< StandBy WFI event from MPROC to MCU */
	MCU_MPROC_STANDBYWFI_EVENT = 30,
	/**< StandBy WFIEevent from MPROC to MCU */
	MCU_MPROC_STANDBYWFE_EVENT = 29,
	/**< RTC event */
	MCU_SPRU_RTC_PERIODIC_EVENT = 28,
	/**< RTC event */
	MCU_SPRU_RTC_EVENT = 27,
	/**< SPRU event */
	MCU_SPRU_ALARM_EVENT = 26,
	MCU_SPL_FREQ_EVENT = 25, /**< SPLFREQ event */
	MCU_SPL_PVT_EVENT = 24, /**< SPL PVY event */
	MCU_SPL_RST_EVENT = 23, /**< SPL RST event */
	MCU_SPL_WDOG_EVENT = 22, /**< SPL WDOG event */
	MCU_MPROC_CRMU_EVENT5 = 21, /**< MPROC CRMU EVENT5 */
	MCU_MPROC_CRMU_EVENT4 = 20, /**< MPROC CRMU EVENT4 */
	MCU_MPROC_CRMU_EVENT3 = 19, /**< MPROC CRMU EVENT3 */
	MCU_MPROC_CRMU_EVENT2 = 18, /**< MPROC CRMU EVENT2 */
	MCU_MPROC_CRMU_EVENT1 = 17, /**< MPROC CRMU EVENT1 */
	MCU_MPROC_CRMU_EVENT0 = 16, /**< MPROC CRMU EVENT0 */
	/**< USBPHY1 Filter1 match event */
	MCU_USBPHY1_FILTER_EVENT = 15,
	/**< USBPHY1 Wake event */
	MCU_USBPHY1_WAKE_EVENT = 14,
	/**< USBPHY0 Filter match event */
	MCU_USBPHY0_FILTER_EVENT = 13,
	/**< USBPHY0 Wake event */
	MCU_USBPHY0_WAKE_EVENT = 12
} DMU_MCU_EVENT_ID_e;

/** This a 32-bit mask of the events to wake up the MPROC (A7) from MCU (M0):
 *
 * TODO: Should we add SMBUS, Tamper etc. to this list?
 *
 * What is difference between SPRU_ALARM event & SMBUS Interrupt?
 * Ans.
 * SPRU_ALARM event come from BBL, upon expiry of a Timer programmed
 * using RTC (resident in BBL block). The 32KHz RTC can measure time
 * upto sme 137years.
 *
 * SPRU_ALARM event also comes from BBL, upon tamper event.
 *
 * The SMBUS Interrupt, on the other hand, comes from peripherals
 * connected using I2C.
 *
 * The M0 in CRMU executes WFI before going to sleep. What that means
 * is the M0 CPU clocks are gated and the M0 runs off a slow clock to
 * saving few mw of power. That how the WFI is different from running
 * a infinite loop. In such a state M0 can wake up upon external IRQs only.
 *
 */
#define MCU_WAKEUP_SOURCE    (\
	BIT(MCU_NVIC_AON_GPIO_INTR) | \
	BIT(MCU_NVIC_WDOG_INTR) | \
	BIT(USBPHY_WAKE_EVENT) | \
	BIT(MCU_NVIC_SPRU_ALARM_EVENT) | \
	BIT(MCU_NVIC_SPRU_RTC_PERIODIC_EVENT))

/**
 * @brief   Citadel clock policy enum
 * @details The available clock policies
 */
/** TODO:
 * Discuss with Fong / Alfonso about Citadel Clock Policy. Since currently
 * our highest freq is 1GHz Medium & Low will change accordingly.
 */
typedef enum citadel_clock_policy {
	CITADEL_CLK_POLICY_MAX = 0, /**< 1000MHz, Fastest running */
#ifdef MPROC_PM__DPA_CLOCK
	CITADEL_CLK_POLICY_DPA, /**< 187.5MHz, DPA running */
#endif
	CITADEL_CLK_POLICY_MEDIUM, /**< 200MHz, Medium running */
	CITADEL_CLK_POLICY_LOW, /**< 100MHz, Low running */
	CITADEL_CLK_POLICY_BCM5810X_L, /**< 100MHz, BCM5810X_L running */
	CITADEL_CLK_POLICY_ULTRA_LOW, /**< 25MHz, Ultra Low running */
	CITADEL_CLK_POLICY_DEFAULT, /**< 200MHz, Default POR state */
} CITADEL_CLK_POLICY_e;

/** TODO:
 * What is the purpose of this?
 */
typedef enum mproc_enable_disable {
	MPROC_DISABLED = 0,
	MPROC_ENABLED  = 1,
} MPROC_EN_DIS_e;

/** Used to indicate whether a LP
 * is being entered into or being exited out of.
 */
typedef enum mproc_low_power_op {
	MPROC_EXIT_LP  = 0,
	MPROC_ENTER_LP = 1,
} MPROC_LOW_POWER_OP_e;

/** TODO: What is the purpose of this?
 *
 * These are the codes that the A7 must check
 * to know which state it is waking from.
 *
 */
typedef enum mproc_cpu_pw_stat {
	CPU_PW_NORMAL_MODE = 0x0, /* same with sku rom code */
	CPU_PW_STAT_SLEEP = 0x1,
	/* Work-Around for DRIPS: The SBL does not recognize
	 * this new DRIPS mode. So we are mux-ed with Sleep.
	 * To distinguish between Sleep & DRIPS, we will use
	 * a persistent register that is yet TBD.
	 */
	CPU_PW_STAT_DRIPS = 0x2,
	CPU_PW_STAT_DEEPSLEEP = 0x3,
} MPROC_CPU_PW_STAT_e;

/** The MCU Interrupts upon which to wake up the MPROC(A7)
 * There are 32 IRQs to M0 upon which wakeup is possible.
 * Here we are listing the currently allowed wakeup sources.
 *
 * Note that some wake up sources are multiplexed into the
 * same IRQ line on NVIC. e.g. the 4 possible USB events
 * are mapped to only one irq.
 *
 * We use these bit offsets to prepare a mask of wakeup sources
 * and send it to M0 to inform it about on what events M0 needs
 * wake up A7.
 *
 * Note: New permitted wakeup sources need to be added here.
 */
typedef enum mproc_wakeup_src_mcu_irq {
	ws_end       = 32,
	ws_rtc_periodic = MCU_NVIC_SPRU_RTC_PERIODIC_EVENT,
	ws_rtc = MCU_NVIC_SPRU_RTC_EVENT,
	ws_tamper = MCU_NVIC_SPRU_ALARM_EVENT,
	ws_usb = USBPHY_WAKE_EVENT,
	ws_wdog = MCU_NVIC_WDOG_INTR,
	ws_smbus = MCU_NVIC_SMBUS_INTR,
	ws_aon_gpio = MCU_NVIC_AON_GPIO_INTR,
	ws_aon_uart = MCU_NVIC_AON_UART_INTR,
	ws_none      = 0,
} MPROC_WAKEUP_SRC_MCU_e;

/** Characteristics of AON GPIO Interrupt to wake up the MPROC(A7):
 * e.g. GPIO can be used to wake up on toggling a switch.
 * To be Clarified - TODO
 * Details of each of these fields?
 * Should wakeup_src be the GPIO # on which the interrupt has been received..?
 */
typedef struct mproc_aon_gpio_wakeup {
	u32_t wakeup_src; /* bitmap: 1=used, 0=not used*/
	u32_t int_type; /*1=level, 0=edge*/
	u32_t int_de; /*1=both edge, 0=single edge*/
	u32_t int_edge; /*1=rising edge or high level, 0=falling edge or low level*/
} MPROC_AON_GPIO_WAKEUP_SETTINGS_s;

/** Characteristics of USB to wake up the MPROC(A7):
 * e.g. Inserting an USB device GPIO can be used to wake up.
 */
typedef struct mproc_usb_wakeup {
	u32_t wakeup_src; /* bitmap: 1=used, 0=not used*/
} MPROC_USB_WAKEUP_SETTINGS_s;

/** Characteristics of Wake-up Source to wake up the MPROC(A7):
 * TODO:
 *
 * What fields should be added if we decide to add wake-up sources such
 * SMBUS Interrupt?
 *
 * How are these fields used?
 * Ans. This is copied as ISR arguments into M0 IDRAM space from where
 * they are read by M0 ISRs.
 */
typedef struct mproc_pm_wakeup_code {
	MPROC_WAKEUP_SRC_MCU_e wakeup_src;
	union {
		struct {
			u32_t src; /* AON GPIO # Bit Map */
		} aon_gpio;

		struct {
			u32_t usbno; /* USB # Bit map */
		} usbno;

		struct {
			u32_t time; /* Time when the tamper event happened */
			u32_t src;
			u32_t src1;
		} tamper;
	} info;
} MPROC_PM_WAKEUP_CODE_s;

/** Arguments to be used by MPROC(A7) PM ISRs:
 * To be Clarified - TODO
 * Details of each of these fields?
 * How are these fields used?
 */
typedef struct mproc_pm_isr_args {
	/* SBL expects a 0xFF at the very beginning of IDRAM.
	 * This need to be set to 0xFF before entering any LP mode.
	 */
	u32_t pm_state_reserved;

	MPROC_PM_MODE_e tgt_pm_state; /* target pm mode to enter */
#ifdef MPROC_PM__ADV_S_POWER_OFF_PLL
	MPROC_PM_MODE_e prv_pm_state; /* pm mode before enter target pm mode */
#endif
	/* filled by A7, readonly by MCU */
	MPROC_AON_GPIO_WAKEUP_SETTINGS_s aon_gpio_settings;
	/* filled by A7, readonly by MCU */
	MPROC_USB_WAKEUP_SETTINGS_s usb_settings;

#ifdef MPROC_PM_ACCESS_BBL
	u32_t bbl_access_ok; /* filled by mproc, readonly by MCU */
	u32_t vBBL_INTERRUPT_EN; /* same above */
	u32_t vBBL_CONTROL; /* same above */
#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
	/* rtc interrupt count of origin sleep time,
	 * if 0, skip rtc wakeup system, readonly by MCU
	 */
	u32_t seconds_to_wakeup;
	/* rtc interrupt count of left time, wakeup system when reachs 0 */
	u32_t seconds_left;
	/* used by MCU to save & restore A7 reg val, readonly by A7 */
	u32_t vBBL_RTC_PER;
#endif
#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
	u32_t vBBL_TAMPER_SRC_ENABLE; /* same above */
	u32_t vBBL_TAMPER_SRC_ENABLE_1; /* same above */
#endif
#endif
	u32_t do_policy; /* filled by mproc(A7), readonly by MCU */
	MPROC_PM_WAKEUP_CODE_s wakeup; /* filled by MCU, readonly by A7 */

#ifdef MPROC_PM_WAIT_FOR_MCU_JOB_DONE
	u32_t progress; /* filled by mcu, readonly by mproc */
#endif
	CITADEL_REVISION_e soc_rev; /* filled by mproc, readonly by MCU */
} CITADEL_PM_ISR_ARGS_s;

/** To be Clarified - TODO
 * Details of each of these fields?
 * How are these fields used?
 */
typedef enum mproc_pm_rtc_wakeup_step {
	MPROC_PM_RTC_STEP__125MS = 0x1,
	MPROC_PM_RTC_STEP__250MS = 0x2,
	MPROC_PM_RTC_STEP__500MS = 0x4,
	MPROC_PM_RTC_STEP__1S    = 0x8,
} MPROC_PM_RTC_WAKUP_STEP_e;

#define MPROC_PM_RTC_STEP MPROC_PM_RTC_STEP__1S

typedef enum mcu_idram_bin_type {
	MCU_IDRAM__ISR_BIN = 0,
	MCU_IDRAM__NORMAL_BIN = 32,
	MCU_IDRAM__END_BIN = 0xFF,
} MCU_IDRAM_BIN_TYPE_e;

/** To be Clarified - TODO
 * Details of each of these fields?
 * How are these fields used?
 */
typedef enum mcu_idram_content_id {
	MCU_IDRAM_CONTENT__START = 0,
	MCU_IDRAM_CONTENT__USR,
	MCU_IDRAM_CONTENT__ISR_ARGS,
	MCU_IDRAM_CONTENT__RTC_OP_MUTEX,
	MCU_IDRAM_CONTENT__MAILBOX_ISR,
	MCU_IDRAM_CONTENT__WAKEUP_ISR,
	MCU_IDRAM_CONTENT__FASTXIP_WORKAROUND_BIN,
	MCU_IDRAM_CONTENT__M0_IMAGE_BIN,
} MCU_IDRAM_CONTENT_ID_e;

/** To be Clarified - TODO
 * Details of each of these types?
 * How are these types used?
 */
typedef enum mcu_scram_content_id {
	MCU_SCRAM_CONTENT__START = 0,
	MCU_SCRAM_CONTENT__USR,
	MCU_SCRAM_CONTENT__SBL,
	MCU_SCRAM_CONTENT__SMAU,
	MCU_SCRAM_CONTENT__USB,
} MCU_SCRAM_CONTENT_ID_e;

typedef struct mcu_idram_content_info {
	MCU_IDRAM_CONTENT_ID_e cid;
	u32_t start_addr;
	u32_t maxlen;
} MCU_IDRAM_CONTENT_INFO_s;

typedef struct mcu_scram_content_info {
	MCU_SCRAM_CONTENT_ID_e cid;
	u32_t start_addr;
	u32_t maxlen;
} MCU_SCRAM_CONTENT_INFO_s;

/** To be Clarified - TODO
 * Details of each of these fields?
 * How are these fields used?
 */
#ifdef MPROC_PM_FASTXIP_SCRATCH_RAM
typedef struct mcu_scram_fast_xip {
	/* Offset: 0x0 */
	u32_t sbl[4]; /* Reserved for SBL (same as Cygnus) */
	/* Offset: 0x10 */
	u32_t res[4]; /* Reserved */
	/* Offset: 0x20 = TAG_VALID_SCR */
	/* Bit[0] = 0x1 indicates FastXIP (read by SKU ROM
	 * and passed to SBL; used in SBL for fast XIP).
	 * Bit[1] = 0x1 indicates FastXIP Config present
	 * in AON Scratch RAM starting at offset 0x24.
	 * Bit[1] = 0x0 indicates FastXIP Config present
	 * in BBRAM offset 0x24.
	 * Bit[2] = 0x1 indicates checksum present at
	 * location 0x80.
	 */
	u32_t fast_xip_tag;
	/* SMAU encryption and authentication algorithm configuration
	 * (configuration register 0)
	 */
	u32_t smau_cfg0; /* Offset 0x24: SMAU_CFG0 */
	u32_t xip_addr; /* Offset 0x28: Address to jump to for XIP. */

	/* Notes on SMAU (Secure Memory Access Unit):
	 * ------------------------------------------
	 * Reading of Program code or Instructions from QSPI Flash
	 * memory in Citadel can be done in two ways:
	 * 1. When SMAU  is disabled, all the reads from Flash happen by issuing
	 * read access to any address in the range 0x63000000 - 0x66FFFFFF, i.e.
	 * entire flash being 64MB In this case, we call it one single unsecure
	 * window of 64MB. When reads are issued to these addresses the content
	 * is presented back to the CPU as it is in clear text. There is no
	 * encryption or Decryption involved.
	 *
	 * 2. When SMAU is enabled, the SMAU controller allows user to program
	 * 4 different windows that are simply 4 regions of memory residing in
	 * this 64MB address space. (ideally non-overlapping to avoid aliasing)
	 *
	 * There are SMAU Registers to program the base and size of each
	 * of these windows. e.g. SMU_WIN_BASE_0 & SMU_WIN_SIZE_0 are two registers
	 * to program the base & size of Window 0.
	 *
	 * What is programmed into these registers is the physical address (actual
	 * Flash address) that needs to be mapped to either of the 4 possible
	 * values, which are 0x63000000 for Window 0, 0x64000000 for Window 1
	 * 0x65000000 for Window 2 & 0x66000000 for Window 3. These values are
	 * called virtual addresses and these are the addresses to which "reads"
	 * are issued by the CPU.
	 *
	 * What does this mean?
	 * For example, if we have SMU_WIN_BASE_1 = 0x63200000, then when CPU issues
	 * a read from address 0x64000000 then the read is made from 0x63200000 of
	 * the Flash, decrypted on the fly using the encryption keys for this window
	 * (again programmed by user through SMAU registers) and then presented to the
	 * CPU. So on for any other offset.
	 *
	 * Now if SMU_WIN_BASE_2 = 0x63200000, then this means that the locations
	 * 0x63200000 + x can be accessed by either issuing reads to 0x6400000 or
	 * 0x65000000, essentially creating aliases.
	 *
	 * Example:
	 *
	 *     QSPI Flash PoV         CPU PoV
	 *-----------------------------------------
	 * Win0: 0x63000000             0x63000000
	 * Win1: 0x63100000             0x64000000
	 * Win2: 0x63200000             0x65000000
	 * Win3: 0x63300000             0x66000000
	 *
	 */
	u32_t win; /* Offset 0x2c: Window Number */

	/* Base address of Flash Memory for cleartext window 0
	 * Depending on which Window Configuration we are going to save
	 * we use win parameter above to save the corresponding configuration.
	 */
	u32_t smau_win_base; /* Offset: 0x30 - (SMU_WIN_BASE_0 + (win * 4)) */
	/* Set the size of Flash Memory for cleartext window 0 */
	u32_t smau_win_size; /* Offset: 0x34 - (SMU_WIN_SIZE_0 + (win * 4)) */
	/* Location of authentication signatures for window 0 */
	u32_t smau_hmac_base;/* Offset: 0x38 - (SMU_HMAC_BASE_0 + (win * 4)) */

	/* Offset 0x3C: This is no longer used by SBL - Reserved */
	u32_t smc_cfg; /* Reserved for SMC Controller Config */

	/* 256-bit AES Key for Window # win */
	/* Offset: 0x40 */
	u32_t aes_key_31_0;  /*AES-0: (SMAU_AES_KEY_WORD_0 + (win * 0x20))*/
	u32_t aes_key_63_32; /*AES-0: (SMAU_AES_KEY_WORD_1 + (win * 0x20))*/
	u32_t aes_key_95_64; /*AES-0: (SMAU_AES_KEY_WORD_2 + (win * 0x20))*/
	u32_t aes_key_127_96; /*AES-0 (SMAU_AES_KEY_WORD_3 + (win * 0x20))*/
	/* Offset: 0x50 */
	u32_t aes_key_159_128; /*AES-1: (SMAU_AES_KEY_WORD_4 + (win * 0x20))*/
	u32_t aes_key_191_160; /*AES-1: (SMAU_AES_KEY_WORD_5 + (win * 0x20))*/
	u32_t aes_key_223_192; /*AES-1: (SMAU_AES_KEY_WORD_6 + (win * 0x20))*/
	u32_t aes_key_255_224; /*AES-1: (SMAU_AES_KEY_WORD_7 + (win * 0x20))*/
	/* 256-bit HMAC Key for Window # win */
	/* Offset: 0x60 */
	u32_t hmac_key_31_0;   /*HMAC-0: (SMAU_AUTH_KEY_WORD_0 + (win * 0x20))*/
	u32_t hmac_key_63_32;  /*HMAC-0: (SMAU_AUTH_KEY_WORD_1 + (win * 0x20))*/
	u32_t hmac_key_95_64;  /*HMAC-0: (SMAU_AUTH_KEY_WORD_2 + (win * 0x20))*/
	u32_t hmac_key_127_96; /*HMAC-0: (SMAU_AUTH_KEY_WORD_3 + (win * 0x20))*/
	/* Offset: 0x70 */
	u32_t hmac_key_159_128; /*HMAC-1: (SMAU_AUTH_KEY_WORD_4 + (win * 0x20))*/
	u32_t hmac_key_191_160; /*HMAC-1: (SMAU_AUTH_KEY_WORD_5 + (win * 0x20))*/
	u32_t hmac_key_223_192; /*HMAC-1: (SMAU_AUTH_KEY_WORD_6 + (win * 0x20))*/
	u32_t hmac_key_255_224; /*HMAC-1: (SMAU_AUTH_KEY_WORD_7 + (win * 0x20))*/
	/* Offset: 0x80 */
	/* Scratch RAM Checksum (optional).
	 * For [fast_xip_tag, hmac_key_255_224]
	 */
	u32_t scram_cksum;
	/* Ideally, the SMAU configurations for all the 4 windows need to be
	 * saved upon LP entry & restored upon LP exit. The ideal location to
	 * store these configurations is the SCRAM, but it may not have enough
	 * space.
	 *
	 * The SBL does restore window 0, so only the following need to be
	 * restored for windows 1, 2 & 3.
	 *
	 * FIXME: Verify if there is sufficient space in SCRAM for 3 more
	 * full set of configuration parameters, i.e. with all keys.
	 *
	 * For now, make do with only the win base/size and hmac bases.
	 * Note that these should be for windows 1, 2 & 3 as window 0 is already
	 * taken care of above. Note that the "win" parameter above at offset 0x2c
	 * will always be 0 for now (as long as this WAR is in force).
	 *
	 * Currently saving only three SMAU configuration parameters for
	 * windows 1, 2 & 3. These parameters are the base, size and hmac
	 * base of these windows.
	 *
	 * TODO: How this will change for MPOS is yet to be decided.
	 * So this is likely to go away and get replaced with 4 complete instances
	 * of the SMAU configuration structure.
	 */
#ifdef MPROC_PM_FASTXIP_SUPPORT
	u32_t smau_cfg1;
	u32_t smau_win_base_a[3];
	u32_t smau_win_size_a[3];
	u32_t smau_hmac_base_a[3];
#endif
} MCU_SCRAM_FAST_XIP_s;
#endif

/* regs will be saved & restored by PM driver */
typedef struct mcu_platform_regs {
	u32_t aon_gpio_mask;
	/* u32_t clk_gate_ctrl; */
} MCU_PLATFORM_REGS_s;

/* FIXME: Multiple bins are no longer present. So
 * clean these definitions up.
 *
 *
 * start            end             len       usage
 * 0x30010000       0x300100FF       0x100    PM parameters
 * 0x30010100       0x300113FF       0x1300   PM maibox isr bin
 * 0x30011400       0x300127FF       0x1400   PM wakeup isr bin
 * 0x30012800       0x30012FFF       0x800    M0 heap, grows upwards
 * 0x30012E00       0x30012FFF       0x200    M0 track info, shared with heap
 * 0x30013000       0x3001301F       0x20     PM FastXIP workaround patch bin
 * 0x30013FFC       0x30013800       0x800    M0 stack, grows downwards
 * 0x30010000       0x30013FFF       0x4000   M0 Physical IDRAM, usable for SW
 * 0x30014000       0x30017FFF       0x4000   M0 Shadow IDRAM, unusable for SW
 */
/* Only 16KB of the 32KB space in IDRAM is usable */
#define CRMU_M0_IDRAM_START    (0x30010000)
#define CRMU_M0_IDRAM_END      (0x30013FFF)
#define CRMU_M0_IDRAM_LEN      (CRMU_M0_IDRAM_END - CRMU_M0_IDRAM_START + 1)
/* Offset to CRUM_M0_IDRAM_START */
#define MPROC_M0_USER_BASE             (0x0)

/* M0 new ISRs & args' Offsets to IDRAM */
/* Offset to CRUM_M0_IDRAM_START */
#define MPROC_M0_ISR_ARGS_OFFSET	   (MPROC_M0_USER_BASE)
#define MPROC_M0_ISR_ARGS_MAXLEN	   (0x100 - 4)

#define MPROC_PM_BBL_OP_MUTEX_OFFSET \
			(MPROC_M0_ISR_ARGS_OFFSET + MPROC_M0_ISR_ARGS_MAXLEN)
#define MPROC_PM_BBL_OP_MUTEX_MAXLEN   (4)

/* Start of M0 Code in IDRAM */
#define MPROC_M0_CODE_OFFSET \
	(MPROC_PM_BBL_OP_MUTEX_OFFSET + MPROC_PM_BBL_OP_MUTEX_MAXLEN)
#define MPROC_M0_CODE_MAXLEN       		(CRMU_M0_IDRAM_LEN - MPROC_M0_CODE_OFFSET)

/* Mbox ISR offset in IDRAM */
#define MPROC_MAILBOX_ISR_OFFSET \
	(MPROC_PM_BBL_OP_MUTEX_OFFSET + MPROC_PM_BBL_OP_MUTEX_MAXLEN)
#define MPROC_MAILBOX_ISR_MAXLEN       (0x1300)

/* WAKEUP ISR offset in IDRAM */
#define MPROC_WAKEUP_ISR_OFFSET \
		(MPROC_MAILBOX_ISR_OFFSET + MPROC_MAILBOX_ISR_MAXLEN)
#define MPROC_WAKEUP_ISR_MAXLEN        (0x1400)

/* mcu heap, grows upwards */
#define MPROC_IDRAM_MCU_HEAP_OFFSET    (0x2800)
#define MPROC_IDRAM_MCU_HEAP_MAXLEN    (0x800)

#ifdef MPROC_PM_TRACK_MCU_STATE
#define MPROC_MCU_LOG_BUFF_OFFSET      (0x2E00)
/* LOG_BUFF don't exceeds 0x30013000!!*/
#define MPROC_MCU_LOG_BUFF_MAXLEN      (0x200)
#define MPROC_MCU_LOG_PTR \
	CRMU_M0_IDRAM_ADDR(MPROC_MCU_LOG_BUFF_OFFSET)
#define MPROC_MCU_LOG_START_ADDR \
	CRMU_M0_IDRAM_ADDR(MPROC_MCU_LOG_BUFF_OFFSET + 4)
#define MPROC_MCU_LOG_END_ADDR \
	CRMU_M0_IDRAM_ADDR(MPROC_MCU_LOG_BUFF_OFFSET + MPROC_MCU_LOG_BUFF_MAXLEN)
#endif

#define MPROC_FASTXIP_WORKAROUND_OFFSET \
	(MPROC_IDRAM_MCU_HEAP_OFFSET + MPROC_IDRAM_MCU_HEAP_MAXLEN)
#define MPROC_FASTXIP_WORKAROUND_MAXLEN (0x20)

#define MPROC_M0_USER_END \
	(MPROC_FASTXIP_WORKAROUND_OFFSET + MPROC_FASTXIP_WORKAROUND_MAXLEN)
#define MPROC_M0_USER_MAXLEN \
	(MPROC_M0_USER_END - MPROC_M0_USER_BASE + 1)


/* M0 stack offset to IDRAM */
/* mcu stack top, grows down */
#define MPROC_M0_STACK_TOP_ADDR        (0x30013FFC)
/* NOT real size, should reserve enough space */
#define MPROC_M0_STACK_SIZE            (0x800)

#define MPROC_M0_USER_MAX_TOTAL_SIZE  \
	(MPROC_M0_STACK_TOP_ADDR + 4 - CRMU_M0_IDRAM_START - MPROC_M0_STACK_SIZE)

#if ((MPROC_M0_USER_END - MPROC_M0_USER_BASE) > MPROC_M0_USER_MAX_TOTAL_SIZE)
#error "*************** IDRAM CONTENTS OVERSIZE **********!!!"
#endif

#define CRMU_M0_IDRAM_ADDR(x)	((CRMU_M0_IDRAM_START) + (x))

#define CRMU_GET_PM_ISR_ARGS() \
	((CITADEL_PM_ISR_ARGS_s *)CRMU_M0_IDRAM_ADDR(MPROC_M0_ISR_ARGS_OFFSET))

#if 0
/* 0x030100FC (0x02030000) */
#define MPROC_PM_BBL_OP_MUTEX_ADDR      CRMU_M0_IDRAM_ADDR(MPROC_PM_BBL_OP_MUTEX_OFFSET)
#define MPROC_PM_BBL_OP_LOCK            (0)
#define MPROC_PM_BBL_OP_UNLOCK          (1)
#define MPROC_PM_BBL_OP_INIT            (100)
#define MPROC_PM_BBL_FREE               (0)
#define MPROC_PM_BBL_BUSY               (1)
#endif

/* User info saved in M0 SCRAM */
/* Only 4KB of 8KB Scratch RAM usable */
#define CRMU_M0_SCRAM_START             (0x30018000)
#define CRMU_M0_SCRAM_END               (0x30018FFF)
#define CRMU_M0_SCRAM_LEN \
		(CRMU_M0_SCRAM_END - CRMU_M0_SCRAM_START + 1)

#define CRMU_M0_SCRAM__USER_BASE        (0x0)

#define CRMU_M0_SCRAM__SBL_OFFSET       (CRMU_M0_SCRAM__USER_BASE)
#define CRMU_M0_SCRAM__SBL_MAXLEN       (0x20)

#define CRMU_M0_SCRAM__SMAU_OFFSET \
	(CRMU_M0_SCRAM__SBL_OFFSET + CRMU_M0_SCRAM__SBL_MAXLEN)
#define CRMU_M0_SCRAM__SMAU_MAXLEN      (0x64)

#define CRMU_M0_SCRAM__USB_OFFSET \
	(CRMU_M0_SCRAM__SMAU_OFFSET + CRMU_M0_SCRAM__SMAU_MAXLEN)
#define CRMU_M0_SCRAM__USB_MAXLEN       (0x100)

#define CRMU_M0_SCRAM__USER_END \
	(CRMU_M0_SCRAM__USB_OFFSET + CRMU_M0_SCRAM__USB_MAXLEN)
#define CRMU_M0_SCRAM__USER_MAXLEN \
	(CRMU_M0_SCRAM__USER_END - CRMU_M0_SCRAM__USER_BASE + 1)

#define CITADEL_GET_USR_MCU_SCRAM_ADDR(x)  (CRMU_M0_SCRAM_START + (x))

#define CRMU_GET_FAST_XIP_SCRAM_ENTRY() ((MCU_SCRAM_FAST_XIP_s *)CITADEL_GET_USR_MCU_SCRAM_ADDR(CRMU_M0_SCRAM__USER_BASE))

/** The AON GPIOs and the interrupts arising out of these GPIOs
 *  have their own set of control (mask, enable etc.) inside CRMU.
 * These are log registers to register occurrence of an event for M0.
 */
/* Fixed usage */
/* Sleeping interrupt log */
#define CRMU_SLEEP_EVENT_LOG_0          (0x30018F60)
/* Sleeping interrupt log */
#define CRMU_SLEEP_EVENT_LOG_1          (0x30018F64)
/* Error Interrupt log    */
#define CRMU_ERROR_INTERRUPT_LOG        (0x30018F70)
/* Power Interrupt log    */
#define CRMU_POWER_INTERRUPT_LOG        (0x30018F74)
/* Reset Interrupt log    */
#define CRMU_RESET_INTERRUPT_LOG        (0x30018F78)

#define CRMU_IRQx_vector_USER_START     (0x30018F80)
#define CRMU_IRQx_vector_USER(irqno) \
		(CRMU_IRQx_vector_USER_START + 4*(irqno))
#define CRMU_IRQx_vector_USER_END       (CRMU_IRQx_vector_USER(32) - 1)

#endif /* _M0_MPROC_COMMON_H */
