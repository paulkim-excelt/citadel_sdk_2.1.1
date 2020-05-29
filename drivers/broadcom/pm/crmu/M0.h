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
#ifndef _M0_H
#define _M0_H

#include "M0_mproc_common.h"

/* AON GPIOs Mask */
#define AON_GPIO_INT_MASK 0x7ff

#define  PWRDN_WAIT           0x50

#define  PMU_TIMEOUT          0x200
#define  PMU_TIMEOUT_2        0x3FFFFFFF
#define  PLL_TIMEOUT          0x6000

#define  iHOST_PWR_TIMEOUT    0x20
#define  DFT_BISR_TIMEOUT     0x6000
#define  DFT_BISR_TIMEOUT_2   0xFFFFFFFF

void MCU_SYNC(void);
u32_t mcu_getbit(u32_t data, u32_t bitno);
u32_t mcu_setbit(u32_t data, u32_t bitno);
u32_t mcu_clrbit(u32_t data, u32_t bitno);

#ifdef MPROC_PM_TRACK_MCU_STATE
void MCU_USER_ISR_SET_STATE(u32_t state);
#define MCU_SET_STATE_IN_COMMON_C() \
		MCU_USER_ISR_SET_STATE(0x01000000 | __LINE__)
#define MCU_SET_STATE_IN_MBOX_ENTRY_C() \
		MCU_USER_ISR_SET_STATE(0x02000000 | __LINE__)
#define MCU_SET_STATE_IN_MBOX_BODY_C() \
		MCU_USER_ISR_SET_STATE(0x03000000 | __LINE__)
#define MCU_SET_STATE_IN_WAKEUP_ENTRY_C() \
		MCU_USER_ISR_SET_STATE(0x04000000 | __LINE__)
#define MCU_SET_STATE_IN_WAKEUP_BODY_C() \
		MCU_USER_ISR_SET_STATE(0x05000000 | __LINE__)
#else
#define MCU_USER_ISR_SET_STATE(state)
#define MCU_SET_STATE_IN_COMMON_C()
#define MCU_SET_STATE_IN_MBOX_ENTRY_C()
#define MCU_SET_STATE_IN_MBOX_BODY_C()
#define MCU_SET_STATE_IN_WAKEUP_ENTRY_C()
#define MCU_SET_STATE_IN_WAKEUP_BODY_C()
#endif

void *MCU_memset(void *s, s32_t c, size_t count);
void *MCU_memcpy(void *dest, const void *src, size_t count);
void MCU_die(void);
void MCU_delay(u32_t cnt);
void MCU_timer_udelay(u32_t time);
void MCU_timer_raw_delay(u32_t counts);
void MCU_wdog_delay(u32_t time);

#ifdef MPROC_PM_WAIT_FOR_MCU_JOB_DONE
u32_t MCU_get_mcu_job_progress(void);
void MCU_set_mcu_job_progress(u32_t pro);
#endif

CITADEL_REVISION_e MCU_get_soc_rev(void);
void MCU_enable_irq(DMU_MCU_IRQ_ID_e id, s32_t en);
s32_t MCU_check_irq(DMU_MCU_IRQ_ID_e id);
void MCU_clear_irq(DMU_MCU_IRQ_ID_e id);
void MCU_enable_event(DMU_MCU_EVENT_ID_e id, s32_t en);
s32_t MCU_check_event(DMU_MCU_EVENT_ID_e id);
void MCU_clear_event(DMU_MCU_EVENT_ID_e id);

void MCU_send_msg_to_mproc(u32_t code0, u32_t code1);
void MCU_send_msg_to_mcu(u32_t code0, u32_t code1);

void MCU_set_irq_user_vector(u32_t irq, u32_t addr);

#ifdef MPROC_PM_ACCESS_BBL
#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
void MCU_setup_rtc(MPROC_LOW_POWER_OP_e op);
#endif
#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
void MCU_setup_tamper(MPROC_LOW_POWER_OP_e op);
#endif
#endif

void MCU_set_wakeup_aon_gpio_output(s32_t out);
void MCU_set_pdsys_master_clock_gating(s32_t en);
void MCU_setup_wakeup_src(MPROC_LOW_POWER_OP_e op);

void cpu_reg32_set_masked_bits(u32_t addr, u32_t mask);
void cpu_reg32_clr_masked_bits(u32_t addr, u32_t mask);
void cpu_reg32_wr_masked_bits(u32_t addr, u32_t mask, u32_t val);
void cpu_reg32_wr_mask(u32_t addr, u32_t rightbit, u32_t len, u32_t value);

u32_t spru_reg_read(u32_t addr);
u32_t spru_reg_write(u32_t addr, u32_t wr_data);

s32_t bbl_read_reg(u32_t regAddr, u32_t *p_dat);
s32_t bbl_write_reg(u32_t regAddr, u32_t wrDat);

s32_t MCU_SoC_do_policy(void);
s32_t MCU_SoC_Run_Handler(MPROC_PM_MODE_e state);
s32_t MCU_SoC_Sleep_Handler(void);
s32_t MCU_SoC_DRIPS_Handler(void);
s32_t MCU_SoC_DeepSleep_Handler(void);

int m0_wfi_handler(void);
int m0_usb_handler(void);
int m0_aon_gpio_handler(void);
int m0_rtc_handler(void);
int m0_tamper_handler(void);
int m0_aon_phy0_resume_handler(void);
s32_t user_mbox_handler(void);
s32_t user_wakeup_handler(void);

void citadel_sleep_exit(u32_t event, MPROC_PM_MODE_e prv_pm_state);
s32_t citadel_deep_sleep_exit(void);
s32_t m0_smbus_handler(void);
void user_fastxip_workaround(void);
void citadel_DRIPS_exit(void);

s32_t MCU_SoC_Wakeup_Handler(MPROC_PM_MODE_e cur_pm_state,
					MPROC_PM_MODE_e prv_pm_state,
					u32_t wakeup_msg_code0,
					u32_t wakeup_msg_code1);

#endif /* _M0_H */
