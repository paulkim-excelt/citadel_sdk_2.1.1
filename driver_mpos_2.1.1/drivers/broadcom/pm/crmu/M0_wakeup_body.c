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
#include "M0.h"

/******************************************************************************
 * P R I V A T E  F U N C T I O N s
 *****************************************************************************/
/**
 * @brief crmu_cfg_xtal_pwr_on
 *
 * Power On The Crystal Oscillator (26MHz)
 *
 * @param None
 *
 * @return N/A
 */
static void crmu_cfg_xtal_pwr_on(void)
{
	u32_t count = 0, rd_data;

	/* Enable Timer1 : Timer Enable, size : 32 */
	sys_write32(0x82, CRMU_TIM_TIMER1Control);

	/* 1. CRMU_XTAL_CHANNEL_CONTROL1[28]
	 * (CRMU_XTAL_PWR_DET_ENA_REG) to 1
	 */
	sys_write32(sys_read32(CRMU_XTAL_CHANNEL_CONTROL1) | (1U << 28), CRMU_XTAL_CHANNEL_CONTROL1);
	/* cpu_reg32_wr(CRMU_SLEEP_EVENT_LOG_0,
	 *      cpu_rd_single(CRMU_XTAL_CHANNEL_CONTROL1,4), 4);
	 */

	/* cfg_xtal_pwr_dn(1, 1, 1, 0); powering up the XTAL */
	sys_write32(0x7, CRMU_XTAL_POWER_DOWN);
	/* cfg_xtal_pwr_dn(1, 0, 0, 0); disabling XTAL isolation, bufpd =0 */
	sys_write32(0x1, CRMU_XTAL_POWER_DOWN);

	/* TODO: Consider increasing this delay to 30ms.
	 * First Clear the Timer Interrupt before starting.
	 */
	sys_write32(0x1, CRMU_TIM_TIMER1IntClr);
	sys_write32(PLL_PWRON_WAIT, CRMU_TIM_TIMER1Load);
	while(!sys_read32(CRMU_TIM_TIMER1RIS));

	/* setting XTAL */
	/* cken=1, seltest=0, resetb=0, test_pwrdet=1 */
	sys_write32(0x12034207, CRMU_XTAL_CHANNEL_CONTROL1);
	/* cken=1, seltest=1, resetb=1, test_pwrdet=1 */
	sys_write32(0x1a074207, CRMU_XTAL_CHANNEL_CONTROL1);
}

/**
 * @brief bcm_set_mhost_pll_max_freq
 *
 * Description :
 *  - Configure all GENPLL channels with the default Max settings.
 *  - Enable all clocks for all devices in PD_SYS
 *  - Switch clock sources to GENPLL for devices in PD_SYS
 *
 * TODO: Confirm the steps with DV code for setting max PLL Frequency
 *
 * @param None
 *
 * @return N/A
 */
static void bcm_set_mhost_pll_max_freq(void)
{
	/* Enable Timer1 : Timer Enable, size : 32 */
	sys_write32(0x82, CRMU_TIM_TIMER1Control);

	/* The default GENPLL setting for the A7 Max Clock frequency */
	sys_write32(0x0200003d, CRMU_GENPLL_CONTROL3);
	sys_write32(0x00040006, CRMU_GENPLL_CONTROL4);
	sys_write32(0x00140006, CRMU_GENPLL_CONTROL5);
	sys_write32(0x00080008, CRMU_GENPLL_CONTROL6);
	sys_write32(0x8200003d, CRMU_GENPLL_CONTROL3);

	sys_write32(0x1, CRMU_TIM_TIMER1IntClr); // Interrupt clear
	sys_write32(PLL_PWRON_WAIT, CRMU_TIM_TIMER1Load);
	while((sys_read32(CRMU_GENPLL_STATUS) & 0x1) != 0x1)
		;

	/* Creating pulse on Bit<18 ~ 23> for Channels 1 ~ 6
	 * disable PLL channels 1 ~ 6 followed by enable
	 */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) | 0x00FC0000, CRMU_GENPLL_CONTROL1);
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) & 0xFF03FFFF, CRMU_GENPLL_CONTROL1);

	/* Remove ISO from pll_lock if PLL gets lock
	 * TODO:
	 * Bit 12: PDSYS; This was left Functional while entry, why unlock again?
	 */
	sys_write32(sys_read32(CRMU_ISO_CELL_CONTROL) & ~(1 << CRMU_ISO_CELL_CONTROL__CRMU_ISO_PDSYS_PLL_LOCKED), CRMU_ISO_CELL_CONTROL);

	/* FIXME: Why don't we restore SYS_CLK_SOURCE_SEL_CTRL which was
	 * programmed during LP entry?
	 */

	/* Enable clocks
	 * FIXME: usbh still disabled, why? This was set to 0
	 * in ultra low power exit function.
	 * Could this be causing the instability in Dell CS
	 * where the instability is seen when USBH driver is enabled?
	 */
	sys_write32(0x00800000, CDRU_CLK_DIS_CTRL);
	return;
}

/**
 * @brief crmu_cfg_genpll_pwr_on
 *
 * Description :
 *  - Power on GENPLL sequence
 *  - GENPLL is clocked and ready for use.
 *
 * TODO: Confirm the steps with DV code for configuring GENPLL Power On
 *
 * @param None
 *
 * @return none
 */
static void crmu_cfg_genpll_pwr_on(void)
{
	/* Enable Timer1 : Timer Enable, size : 32 */
	sys_write32(0x82, CRMU_TIM_TIMER1Control);

	/* Basic PLL configuration : GENPLL
	 * Power on the PLL(s)
	 */
	sys_write32(((1 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | (0 << CRMU_PLL_AON_CTRL__GENPLL_PWRDN) | (0 << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);

	/* Remove PLL ISO */
	sys_write32(0, CRMU_PLL_ISO_CTRL);

	/* PLL reset & post channel reset
	 * FIXME: Why was there no action on this register in LP entry?
	 * This register needs to be programmed to continue normal PLL operation.
	 */
	sys_write32(0x0, CRMU_GENPLL_CONTROL0);

	/* Remove Gen PLL Isolation */
	sys_write32(((0 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | (0 << CRMU_PLL_AON_CTRL__GENPLL_PWRDN) | (0 << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);

	/* PLL i_areset de-assert */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL0) & 0x4, CRMU_GENPLL_CONTROL0);

	/* Wait for config */
	sys_write32(0x1, CRMU_TIM_TIMER1IntClr);
	sys_write32(PLL_PWRON_WAIT, CRMU_TIM_TIMER1Load);
	while(!sys_read32(CRMU_TIM_TIMER1RIS));

	while((sys_read32(CRMU_GENPLL_STATUS) & 0x1) != 0x1)
		;
}

/**
 * @brief MCU_ultra_low_power_exit
 *
 * Description :
 *  exit ULP mode, switch to 26MHz XTAL clock
 *
 * TODO: Confirm the steps with DV code
 *
 * @param None
 *
 * @return none
 */
static void MCU_ultra_low_power_exit(void)
{
	u32_t rd_data;

	/* Exit ULP mode, switch to 26MHz XTAL clock */
#ifdef MPROC_PM__ULTRA_LOW_POWER
	/* TODO: Clarify from DV / Fong what among below code applies to Citadel */
#if 0
	cpu_reg32_wr(CRMU_XTAL_POWER_DOWN, 0x1);
	/* Mittal: delay 0x400*30us = 32ms under BBL clock, too long? */
	/* MCU_timer_raw_delay(0x400); */
	/* Fong said XTAL OSC up needs 3ms, but 100us seems OK */
	MCU_timer_udelay(100);
#else /* seems cannot stably wakeup using following sequences */
	crmu_cfg_xtal_pwr_on();
#endif

	/* switch to xtal 26M clock */
	sys_write32(0x1, CRMU_ULTRA_LOW_POWER_CTRL);
	while (sys_read32(CRMU_CLOCK_SWITCH_STATUS) != 0x1)
		;
#endif

/* FIXME: This sequence possibly contributes to DRIPS
 * wake-up instability.
 */
#ifdef CITADEL_A7_DISABLE_CLOCKS
	/* 0.1. CRMU_CLOCK_GATE_CONTROL, Un-GATE UART, OTP and SOTP
	 * rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
	 * rd_data = rd_data | (0xC000071F);
	 * sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);
	 */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_BSPI_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_UART_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SOTP_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SMBUS_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_OTPC_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	/* FIXME: Why was SPRU_CRMU_REF_CLK not included in Lynx? What are the repercusions?
	 * It seems to work fine with RTC as wakeup in DRIPS.
	 */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SPRU_CRMU_REF_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_CDRU_CLK_25_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_TIM_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_WDT_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_MCU_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
#ifndef MPROC_PM_ACCESS_BBL
	/* FIXME: When NIC is gated SPRU BBL RTC timer fails. Why? */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) | (1 << CRMU_CLOCK_GATE_CONTROL__CRMU_NIC_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
#endif

	/* 7. Un-Gate MPROC clocks */
	sys_write32(0x0, CDRU_CLK_DIS_CTRL);
#endif
}

/* FIXME: This sequence possibly contributes to DRIPS
 * wake-up instability.
 */
static void citadel_power_on_IO_ring(void)
{
#ifdef CITADEL_A7_GPIO_RING_POWER_OFF
	/* FIXME:
	 * For Dell, the switchable power supply that powers the IO ring
	 * is controlled via AON GPIO 2. For other customers this may be
	 * different. On SVK it is AON GPIO 1.
	 *
	 * Bit 335 of OTP determines whether the AON GPIO 2 should be driven
	 * High / Low. For now, if this OTP bit 335 is set then we need to
	 * drive AON GPIO 2 High to turn IO Ring Power Off. Once OTP is
	 * programmed this need to be programmed accordingly.
	 *
	 */
	/* Set Bit 2 of GP_OUT_EN register to enable output on AON GPIO 2 */
	sys_write32((sys_read32(GP_OUT_EN)) | 0x4, GP_OUT_EN);
	/* Clear Bit 2 of GP_RES_EN register to disable pull-up / pull-down
	 * of On-Chip GPIO Pad Resister for AON GPIO 2.
	 */
	sys_write32((sys_read32(GP_RES_EN)) & 0x7fb, GP_RES_EN);
	/* Set Bit 2 of GP_DATA_OUT register to '0' to Turn On Power to
	 * A7's GPIOs.
	 */
	sys_write32((sys_read32(GP_DATA_OUT)) & 0x7fb, GP_DATA_OUT);
#endif
}

static void citadel_power_on_usb(void)
{
#ifdef CITADEL_A7_USB_PHY_POWER_OFF
	/* Turn USBD PHY On */
	/* 24. Enable USBD memory */
	/* 2.1. standby tx_stby and rx_stby
	 * 31:4 - Reserved ro Reserved 0
	 * 3 - tx_stby rw 1 - Puts USBD Tx memory in standby 0 - Moves USBD Tx memory out of standby state 0
	 * 2 - rx_stby rw 1 - Puts USBD Rx memory in standby 0 - Moves USBD Rx memory out of standby state 0
	 * 1 - override_l1_suspend_select rw When set, value in bit1 (utmi_l1_suspendm) of CDRU_USBPHY_D_CTRL1
	 * register drives L1 suspend signals to PHY, Else USB Device Controller drives L1 suspend to PHY. 0
	 * 0 - override_l1_sleep_ovr_select rw When set, value in bit0 (utmi_l1_sleepm) of CDRU_USBPHY_D_CTRL1
	 * register drives L1 suspend signals to PHY, Else USB Device Controller drives L1 suspend to PHY. 0
	 */
	sys_write32(sys_read32(CDRU_USBD_MISC) & ~(0xF), CDRU_USBD_MISC);
	/* CRMU_USBPHY_D_CTRL
	 * 31:21 Reserved ro Reserved, Default: 0
	 * 20 phy_iso rw Used for Isolation of PHY outputs to the Core. Default: 1
	 * 19:0 pll_ndiv_frac rw PLL feedback divider fraction control. Default: 0x66666
	 *
	 * FIXME: Why are we programming default values here?
	 */
	sys_write32(sys_read32(CRMU_USBPHY_D_CTRL) & (~0x100000), CRMU_USBPHY_D_CTRL);
	/* 3. Resume USBPHY_D bit[27] pll_suspend_en
	 * 27 - pll_suspend_en rw Disables the PLL power-down during suspend mode. 0
	 * 26 - pll_resetb rw Active-low. PLL reset. 0
	 * 25:22 - kp rw PLL P/I loop filter proportional path gain, default to 4b1010 0x8
	 * 21:19 - ki rw PLL P/I loop filter integrator path gain, default to 3b011 0x4
	 * 18:16 - ka rw PLL loop-gain, default to 3b011 0x4
	 * 15:6 - ndiv_int rw PLL feedback divider integer control 0x26
	 * 5:3 - pdiv rw PLL input reference clock pre-divider control 0x1
	 * 2 - resetb rw Active-low asynchronous reset. This is the main reset and shuts the entire IP when asserted low. 0
	 * 1 - utmi_l1_suspendm rw Active Low.USB Device PHY L1 suspend mode, shallow mode. PLL is off. 1
	 * 0 - utmi_l1_sleepm rw Active Low.USB Device PHY L1 sleep mode, shallow mode. PLL is off. HS TX/RX are disabled. 1
	 */
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) | (1 << 27), CDRU_USBPHY_D_CTRL1); /* bit1 = 1 */
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) | (0x3), CDRU_USBPHY_D_CTRL1); /* bits[1:0] = 0x3; */
#endif
}

static void citadel_power_on_nfc(void)
{
	/* Powering Off NFC: This code is experimental
	 * and still doesn't work as desired. It brings
	 * down the voltage across the resistor R425, which
	 * measures current drawn by DUT_NFC_VBAT, from 0.121mV
	 * to ~0.08mV, but the expectation is ~0.003mV.
	 *
	 * FIXME: This code must undergo regression testing
	 * with NFC as a wake-up source.
	 */
#ifdef CITADEL_A7_POWER_OFF_NFC
	/* PD_NFC power Up */
	/* make Reg PU OEB to 0 */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) & ~0x01, NFC_REG_PU_CTRL);

	/* Make SPI_INT PAd as input pin */
	/* Enable Hysteresis and Slewrate */
	sys_write32((sys_read32(NFC_SPI_INT_CTRL) & ~0x1) | 0x12, NFC_SPI_INT_CTRL);

	/* NFC pins enable drive strength and Hysteresis */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) | 0x38, NFC_REG_PU_CTRL);

	sys_write32(sys_read32(NFC_NFC_WAKE_CTRL) | 0x38, NFC_NFC_WAKE_CTRL);

	/* Set Bit 4: Hysterisis Enable; Bit 1: Slew Rate Control */
	sys_write32(sys_read32(NFC_HOST_WAKE_CTRL) | 0x12, NFC_HOST_WAKE_CTRL);

	/* Set Bit 4: Hysterisis Enable; Bit 1: Slew Rate Control */
	sys_write32(sys_read32(NFC_CLK_REQ_CTRL) | 0x12, NFC_CLK_REQ_CTRL);

	/* Power ON */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) | 0x40, NFC_REG_PU_CTRL);
#endif
}

/**
 * @brief citadel_sleep_exit
 *
 * Description :
 * - XTAL Power control and clock switch.
 * - Power On PLL and recover the normal config.
 * - Switch PD_SYS clock to GENPLL
 * - Send mailbox command to A7 to wakeup it.
 *
 * TODO: Confirm the steps with DV code
 *
 * @param None
 *
 * @return none
 */
void citadel_sleep_exit(u32_t event, MPROC_PM_MODE_e prv_pm_state)
{
#ifdef CITADEL_WFI_SLEEP_ENABLED
	u32_t rd_data = 0;

	sys_write32(0x1, CRMU_LDO_CTRL);

	/* 1. Lower the main LDO output voltage;
	 * i.e. lower PD_SYS VDD, after entering sleep mode.
	 * The lowest voltage possible with the main ldo is
	 * 1.116V in sleep mode
	 * Set CRMU_LDO_CTRL_REG_28_0[22:18] (MAINLDO Vout)
	 * = 5'b00000 upon wakeup
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFF83FFFF), CRMU_LDO_CTRL_REG_28_0);

	/* 2. Lower the AON LDO output voltage after entering
	 * sleep mode.
	 * Set CRMU_LDO_CTRL_REG_28_0[3:0] (AONLDO Vout) back
	 * to 4'b0000
	 * upon wakeup before resuming any activity.
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFFFFFFF0), CRMU_LDO_CTRL_REG_28_0);

	/* USBD PHY */
	/* standby tx_stby and rx_stby */
	sys_write32(0x0, CDRU_USBD_MISC);
	sys_write32(sys_read32(CRMU_USBPHY_D_CTRL) & (~0x100000), CRMU_USBPHY_D_CTRL);
	/* resume USBPHY_D  bit[27] pll_suspend_en */
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) | (1 << 27), CDRU_USBPHY_D_CTRL1); /* bit1 = 1 */

#ifdef MPROC_PM__ADV_S_POWER_OFF_PLL  /* test WFI */
	/*---------------------------------------------------------
	 * PD_AON XTAL Power ON and restore default settings
	 *---------------------------------------------------------
	 */
	MCU_ultra_low_power_exit();

	/*---------------------------------------------------------
	 * GenPLL: Power On and restore the settings with max clock freq
	 *---------------------------------------------------------
	 */
	crmu_cfg_genpll_pwr_on();

	/*---------------------------------------------------------
	 * restore the max clock speed for A7
	 *---------------------------------------------------------
	 */
	bcm_set_mhost_pll_max_freq();

	/*---------------------------------------------------------
	 * restore the previous clock gating status
	 *---------------------------------------------------------
	 */
	rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
	rd_data = rd_data | 0xC000071F;
	sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);
	sys_write32(0, CDRU_CLK_DIS_CTRL);

#else
	/* Switch CLK21/41/81 to XTAL */
	sys_write32(0x0, SYS_CLK_SOURCE_SEL_CTRL);
	while ((sys_read32(SYS_CLK_SWITCH_READY_STAT) & 0x7) != 0x07)
		;
#endif

#if 0
	/* trigger an interrupt on A7 to wakeup A7 and resume device USB */
	cpu_reg32_wr(CRMU_IPROC_MAIL_BOX0,
	 (sys_read32(CRMU_IPROC_MAIL_BOX0) & MAILBOX_CODE0_mProcPowerCMDMask)
	 | (event & 0xffffff) | MAILBOX_CODE0_mProcWakeup, 4);
	/* MAILBOX_CODE0_mProcSoftResetAndResume, 4); */
	cpu_reg32_wr(CRMU_IPROC_MAIL_BOX1, MAILBOX_CODE1_PM);
#endif
#endif
}

/**
 * @brief ihost_pwr_up_seq
 *
 * Description : This function turn PD_CPU ON
 *
 * @param None
 *
 * @return none
 */
volatile u32_t rd_data;

static int ihost_pwr_up_seq(void)
{
	volatile u32_t i_loop ;

	if (sys_read32(CRMU_IHOST_SW_PERSISTENT_REG0) == 0x0)
		sys_write32(0x1, CRMU_IHOST_SW_PERSISTENT_REG2);

	ihost_config();

	for (i_loop = 0; i_loop < 10; i_loop++)
		;

	return 1;
}

/**
 * @brief citadel_DRIPS_exit
 *
 * Description :
 * - XTAL Power control and clock switch.
 * - Power On PLL and recover the normal config.
 * - Switch PD_SYS clock to GENPLL
 * - Send mailbox command to A7 to wakeup it.
 *
 * TODO: Confirm the steps with DV code
 *
 * @param None
 *
 * @return none
 */
void citadel_DRIPS_exit(void)
{
	/* Power On the IO Ring First */
	citadel_power_on_IO_ring();

	/* Power On USB PHY */
	citadel_power_on_usb();

	/* FIXME:
	 * Verify if the following writes to the POWER_CONFIG register
	 * & the Persistent Register 11 will work before power up sequence.
	 *
	 * They ideally should, because they are CRMU registers.
	 *
	 * FIXME: This sequence to turn 26MHz On possibly contributes to DRIPS
	 * wake-up instability.
	 */
	MCU_ultra_low_power_exit();

/* PLL power on sequence succeeds Clock enable sequence
 *
 * FIXME: As of now, Wake-up from DRIPS fails using the following
 * sequence to power off PLLs.
 */
#ifdef CITADEL_A7_POWER_OFF_PLLS
	/*---------------------------------------------------------
	 * GenPLL: Power On and restore the settings with max clock freq
	 *---------------------------------------------------------
	 */
	crmu_cfg_genpll_pwr_on();
	/* restore the GENPLL settings */
	bcm_set_mhost_pll_max_freq();
#endif

#ifdef CITADEL_A7_POWER_OFF_ADC
	/* Power Up ADCs */
#endif

	citadel_power_on_nfc();

	/* Ensure Deep sleep status because we are muxing with Deep Sleep
	 * to Jump to the Jump Address.
	 * Will be read by SBL to know the power state, which will be Deep Sleep
	 * But yet another persistent register will tell the SBI or the
	 * Application whether it is coming out of "Deep Sleep" or "DRIPS".
	 * 0b'11 - deep sleep
	 */
	sys_write32(CPU_PW_STAT_DEEPSLEEP, CRMU_IHOST_POWER_CONFIG);

	/* For Now, Use Persistent Register 11 to indicate
	 * the SBI or Application code that this is a wakeup
	 * from DRIPS (0x10).
	 * 0b'11 - deep sleep
	 * 0b'10 - DRIPS
	 */
	sys_write32(CPU_PW_STAT_DRIPS, DRIPS_OR_DEEPSLEEP_REG);

	/* Bare Minimum Power Optimization For DRIPS:
	 * Turn Off IHOST Power. This powers (on / off)
	 * A7, GIC, DMA, GenTimer etc.
	 */
	ihost_pwr_up_seq();

	/* reset A7 - From SV Code
	 * FIXME: Shouldn't this be part of the IHOST power Up sequence?
	 */
	sys_write32(0x1, CRMU_RESET_CTRL);
	sys_write32(0x1, CRMU_CHIP_POR_CTRL);
}

/**
 * @brief citadel_deep_sleep_exit
 *
 * Description :
 * - XTAL Power control and clock switch.
 * - Power On PLL and recover the normal config.
 * - Switch PD_SYS clock to GENPLL
 * - Send mailbox command to M3 to wakeup it.
 *
 * TODO: Confirm the steps with DV code.
 * Verify the additional steps.
 *
 * @param None
 *
 * @return none
 */
s32_t citadel_deep_sleep_exit(void)
{
	u32_t rd_data = 0;

#ifdef MPROC_PM__ADVANCED_DS
	sys_write32(0x1, CRMU_LDO_CTRL);

	/* 1. Lower the AON LDO output voltage after entering sleep mode.
	 * Set CRMU_LDO_CTRL_REG_28_0[3:0] (AONLDO Vout) back to 4'b0000
	 * upon wakeup before resuming any activity.
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFFFFFFF0), CRMU_LDO_CTRL_REG_28_0);

	/* 2. CRMU_ISO_CELL_CONTROL[0] : CRMU_ISO_PDSYS; set to 0 */
	sys_write32((sys_read32(CRMU_ISO_CELL_CONTROL)) & ~(0x1), CRMU_ISO_CELL_CONTROL);
#endif

#ifdef MPROC_PM__ADVANCED_DS
	/* 4. power on PD_SYS I/O pads
	 * FIXME: Is this still required?
	 * sys_write32((sys_read32(GP_DATA_OUT)) | 0x2, GP_DATA_OUT);
	 */
	sys_write32(0x00, GP_TEST_ENABLE);

	/* Power the IO Ring First */
	citadel_power_on_IO_ring();
#endif

	/* 5. Enable PD_SYS SWREG */
	rd_data = sys_read32(CRMU_MAIN_LDO_PU_CTRL);
	sys_write32(0x1, CRMU_MAIN_LDO_PU_CTRL);

	/* 6. Disable PD_SYS Power good ISO */
	rd_data = sys_read32(CRMU_ISO_CELL_CONTROL);
	sys_write32(rd_data & 0xFFFFFEFF, CRMU_ISO_CELL_CONTROL);

	/* 7. poll for pdsys pmu_stable */
	do {
		rd_data = sys_read32(CRMU_POWER_POLL) & 0x1;
	} while (!rd_data);

	/* 8. Disable PD_SYS ISO */
	rd_data = sys_read32(CRMU_ISO_CELL_CONTROL);
	sys_write32(rd_data & 0xFFFFFFFE, CRMU_ISO_CELL_CONTROL);

	/* 3. Exit Ultra Low Power Mode
	 * AON clock gating and power on XTAL
	 */
	MCU_ultra_low_power_exit();

	/* 9. Restore clock */
	rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
	rd_data = rd_data | 0xC000071F;
	sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);
	sys_write32(0x0, CDRU_CLK_DIS_CTRL);

	/* 10. De-asssert reset */
	sys_write32(0x1, CRMU_CDRU_APB_RESET_CTRL);
	sys_write32(0x0, CRMU_SW_POR_RESET_CTRL);
	sys_write32(0x1, CRMU_SW_POR_RESET_CTRL);

	/* 11. Power On GENPLLs */
	/* 11.1 Power on the PLL(s) */
	sys_write32(((1U << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | (0U << CRMU_PLL_AON_CTRL__GENPLL_PWRDN) | (0U << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);

	/* 11.2 Remove PLL ISO */
	sys_write32(0, CRMU_PLL_ISO_CTRL);
	MCU_timer_udelay(100);

	/* 11.3 PLL reset & post channel reset */
	sys_write32(0x0, CRMU_GENPLL_CONTROL0);

	/* 11.4 Remove PLL ISO */
	sys_write32(((0U << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | (0U << CRMU_PLL_AON_CTRL__GENPLL_PWRDN) | (0U << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);

	/* 12. Configure all the clocks to default values */
	/* 12.1 Clear all MDIV and NDIV fields */
	WrReg(CRMU_GENPLL_CONTROL2, (RdReg(CRMU_GENPLL_CONTROL2) & ~(CRMU_GENPLL_CONTROL2__i_ndiv_frac_MASK)));
	WrReg(CRMU_GENPLL_CONTROL3, (RdReg(CRMU_GENPLL_CONTROL3) & ~(CRMU_GENPLL_CONTROL3__i_ndiv_int_MASK) & ~(CRMU_GENPLL_CONTROL3__i_p1div_MASK)));
	WrReg(CRMU_GENPLL_CONTROL4, (RdReg(CRMU_GENPLL_CONTROL4) & ~(CRMU_GENPLL_CONTROL4__i_m1div_MASK) &  ~(CRMU_GENPLL_CONTROL4__i_m2div_MASK)));
	WrReg(CRMU_GENPLL_CONTROL5, (RdReg(CRMU_GENPLL_CONTROL5) & ~(CRMU_GENPLL_CONTROL5__i_m3div_MASK) & ~(CRMU_GENPLL_CONTROL5__i_m4div_MASK)));
	WrReg(CRMU_GENPLL_CONTROL6, (RdReg(CRMU_GENPLL_CONTROL6) & ~(CRMU_GENPLL_CONTROL6__i_m5div_MASK) & ~(CRMU_GENPLL_CONTROL6__i_m6div_MASK)));
	/* 12.2 NDIV Configuration */
	WrReg(CRMU_GENPLL_CONTROL2, (RdReg(CRMU_GENPLL_CONTROL2) | (0x000000 << CRMU_GENPLL_CONTROL2__i_ndiv_frac_R)));
	WrReg(CRMU_GENPLL_CONTROL3, (RdReg(CRMU_GENPLL_CONTROL3) | (0x3D << CRMU_GENPLL_CONTROL3__i_ndiv_int_R) | (0x2  << CRMU_GENPLL_CONTROL3__i_p1div_R)));
	/* 12.3 MDIV Configuration */
	WrReg(CRMU_GENPLL_CONTROL4, (RdReg(CRMU_GENPLL_CONTROL4) | (0x6 << CRMU_GENPLL_CONTROL4__i_m1div_R) | (0x4 << CRMU_GENPLL_CONTROL4__i_m2div_R)));
	WrReg(CRMU_GENPLL_CONTROL5, (RdReg(CRMU_GENPLL_CONTROL5) | (0x6 << CRMU_GENPLL_CONTROL5__i_m3div_R) | (0x14 << CRMU_GENPLL_CONTROL5__i_m4div_R)));
	WrReg(CRMU_GENPLL_CONTROL6, (RdReg(CRMU_GENPLL_CONTROL6) | (0x8 << CRMU_GENPLL_CONTROL6__i_m5div_R) | (0x8 << CRMU_GENPLL_CONTROL6__i_m6div_R)));
	WrReg(CRMU_GENPLL_CONTROL3, (RdReg(CRMU_GENPLL_CONTROL3) | (0x1 << CRMU_GENPLL_CONTROL3__genpll_sel_sw_setting)));

	/* 13. PLL i_areset de-assert */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL0) & 0x4, CRMU_GENPLL_CONTROL0);
	/* MCU_timer_raw_delay(0x10); */

	/* 14. Wait until PLL locked */
	while ((sys_read32(CRMU_GENPLL_STATUS) & 0x1) != 1)
		;

	/* 15. Ensure Deep sleep status :
	 * Will be read by SBL to know the power state. But
	 * SBL doesn't handle DRIPS. So POWER_CONFIG will be
	 * 0x3 for DRIPS as well.
	 * 0b'11 - deep sleep
	 * 0b'10 - DRIPS
	 */
	sys_write32(CPU_PW_STAT_DEEPSLEEP, CRMU_IHOST_POWER_CONFIG);
	/* For Now, Use Persistent Register 11 to indicate
	 * the SBI or Application code that this is a wakeup
	 * from DRIPS (0x10).
	 */
	sys_write32(CPU_PW_STAT_DEEPSLEEP, DRIPS_OR_DEEPSLEEP_REG);

#ifdef MPROC_PM_FASTXIP_DISABLED_ON_A0
	if (MCU_get_soc_rev() < CITADEL_REV__BX)
		sys_write32(CPU_PW_NORMAL_MODE, CRMU_IHOST_POWER_CONFIG);
#endif

	/*-----------------------------------------------------------------
	 * 16. Enable Timer2   :  Timer Enable, size : 32
	 *-----------------------------------------------------------------
	 */
	sys_write32(0x82, CRMU_TIM_TIMER2Control);

	/* 17. Wait PLL locked */
	sys_write32(0x1, CRMU_TIM_TIMER2IntClr);
	sys_write32(0x50, CRMU_TIM_TIMER2Load);
	while (!sys_read32(CRMU_TIM_TIMER2RIS)) {
		if ((sys_read32(CRMU_GENPLL_STATUS) & 0x1) == 0x1)
			break;
	}

	/*-----------------------------------------------------------
	 * 18. Creating pulse on Bit<6~11>=LOAD enable for all channels
	 *-----------------------------------------------------------
	 */
	/* 18.1 disable PLL channels */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) | 0x00FC0000, CRMU_GENPLL_CONTROL1);

	/* 18.2 enable PLL channels */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) & 0xFF03FFFF, CRMU_GENPLL_CONTROL1);

	/*------------------------------------------------------------
	 * 19. Remove ISO from pll_lock if PLL gets lock
	 *------------------------------------------------------------
	 */
	sys_write32(sys_read32(CRMU_ISO_CELL_CONTROL) & 0xFFFFEFFF, CRMU_ISO_CELL_CONTROL);

	/* 20. Switch clocks to PLL */
	sys_write32(0x0, SYS_CLK_SOURCE_SEL_CTRL);

	/* 21. Wait CLK21/41/81 stable */
	sys_write32(0x1, CRMU_TIM_TIMER2IntClr);
	sys_write32(0x50, CRMU_TIM_TIMER2Load);
	while (!sys_read32(CRMU_TIM_TIMER2RIS)) {
		if ((sys_read32(SYS_CLK_SWITCH_READY_STAT) & 0x7) == 0x07)
			break;
	}

	/*---------------------------------------------------------------
	 * 22. Disable Timer2
	 *---------------------------------------------------------------
	 */
	sys_write32(0x0, CRMU_TIM_TIMER2Control);

	/*--------------------------------------------------------------
	 * 23. Enable clocks
	 *
	 * FIXME: Why is the USBH still disabled?
	 *--------------------------------------------------------------
	 */
	sys_write32(0x00800000, CDRU_CLK_DIS_CTRL); /* usbh still disabled */

#ifdef MPROC_PM__ADVANCED_DS
	/* Power On USB PHY */
	citadel_power_on_usb();
	sys_write32(0x1, CRMU_ADC_LDO_PU_CTRL);
#endif

	/* 25. Reset A7 */
	sys_write32(0x1, CRMU_RESET_CTRL);
	sys_write32(0x1, CRMU_CHIP_POR_CTRL);

	/* 26.Invoke ihost_config() */
	ihost_config();

	return 1;
}

/*****************************************************************************
 * P U B L I C   A P I s
 *****************************************************************************/
/**
 * @brief MCU_get_valid_aon_gpio_wakeup_interrupt
 *
 * Description :
 *
 * TODO: Confirm the steps with DV code.
 * Verify the additional steps.
 *
 *
 * @param aon_gpio_wakeup - The mask of aon GPIOs?
 *
 * @return result
 */
u32_t MCU_get_valid_aon_gpio_wakeup_interrupt(u32_t aon_gpio_wakeup)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();
	MPROC_PM_MODE_e cur_pm_state = pm_args->tgt_pm_state;

	volatile u32_t aon_gpio_input = 0;
	volatile u32_t aon_gpio_mstat = 0;
	volatile u32_t aon_gpio_type = 0;
	volatile u32_t aon_gpio_de = 0;
	volatile u32_t aon_gpio_edge = 0;
	u32_t i = 0, retval = 0;

	/* wait for GP_DATA_IN stable
	 *
	 * TODO: Should this include DRIPS too
	 *
	 * in deepsleep, mcu already works very slow
	 */
	if (cur_pm_state != MPROC_PM_MODE__DEEPSLEEP
		&& cur_pm_state != MPROC_PM_MODE__DRIPS)
		MCU_delay(0x10000);

	aon_gpio_mstat = sys_read32(GP_INT_MSTAT);
	aon_gpio_type  = sys_read32(GP_INT_TYPE);
	aon_gpio_de    = sys_read32(GP_INT_DE);
	aon_gpio_edge  = sys_read32(GP_INT_EDGE);
	aon_gpio_input = sys_read32(GP_DATA_IN);

	for (i = 0; i < MPROC_AON_GPIO_END; i++) {
		/* no intr or not wakeup src intr */
		if (!mcu_getbit(aon_gpio_wakeup, i)
			|| !mcu_getbit(aon_gpio_mstat, i))
			continue;

		if (mcu_getbit(aon_gpio_type, i)) { /* level */
		/* if(mcu_getbit(aon_gpio_edge, i)
		 * != mcu_getbit(aon_gpio_input, i))
		 * 	continue;
		 */
		} else { /* edge */
			if ((!mcu_getbit(aon_gpio_de, i)
				 && mcu_getbit(aon_gpio_edge, i))
				!= mcu_getbit(aon_gpio_input, i))
				continue;
		}

		retval = mcu_setbit(retval, i);
	}

	return retval;
}

/**
 * @brief MCU_SoC_Wakeup_Handler
 *
 * Description :
 *
 * TODO: Confirm the steps with DV code.
 * Verify the additional steps.
 *
 * TODO: Is n't the terminology of current & previous PM states misleading?
 *
 * @param cur_pm_state - The one we are entering into
 * @param prv_pm_state
 *
 * @return none
 */
s32_t MCU_SoC_Wakeup_Handler(MPROC_PM_MODE_e cur_pm_state,
			MPROC_PM_MODE_e prv_pm_state,
			u32_t wakeup_msg_code0, u32_t wakeup_msg_code1)
{
	volatile u32_t delay = 50000;

	if (cur_pm_state >= MPROC_PM_MODE__END)
		return -1;

#if 0
	MCU_enable_irq(MCU_AON_GPIO_INTR, 0);
	cpu_reg32_wr(GP_INT_CLR, 0x3f);
	/* enable crmu timer */
	cpu_reg32_wr(CRMU_TIM_TIMER2Control, 0x80);
	/* enable write other crmu wdog regs */
	cpu_reg32_wr(CRMU_WDT_WDOGLOCK, 0x1ACCE551);
	/* enable crmu wdog intr output, but no reset output,
	 * this will reset Citadel after wakeup
	 */
	cpu_reg32_wr(CRMU_WDT_WDOGCONTROL, 0x1);
#endif

	/* SOME times, the mailbox codes are not right... */
	if (cur_pm_state < MPROC_PM_MODE__SLEEP)
		/* just wakeup */
		MCU_send_msg_to_mproc(MAILBOX_CODE0__WAKEUP,
		MAILBOX_CODE1__WAKEUP_TEST);
	else if (cur_pm_state == MPROC_PM_MODE__SLEEP) {
#ifdef MPROC_PM__ADVANCED_S
		citadel_sleep_exit(0, prv_pm_state);
#else
		MCU_set_pdsys_master_clock_gating(0);
#endif
		/* Per DV, this must result in IPROC_MAILBOX_INTR
		 * bit to be pending in the CRMU_IPROC_INTR_STATUS
		 * register. Though this interrupt is triggered via
		 * SPI IRQ31 (IRQ 63), this won't be pending inside
		 * GIC, because GIC is powered OFF. So the restore
		 * routine must first make the mailbox interrupt
		 * pending in GIC after restoring the state of the
		 * A7 CPU.
		 *
		 */
		MCU_send_msg_to_mproc(wakeup_msg_code0, wakeup_msg_code1);
	}
	/* What Should be done when waking up from DRIPS mode?
	 * While waking out of DRIPS we are sending the same mailbox
	 * code as that of Sleep mode.
	 */
	else if (cur_pm_state == MPROC_PM_MODE__DRIPS) {
		/* Before sending the mailbox message to A7
		 * we need to power up PD_CPU.
		 */
		citadel_DRIPS_exit();
		/* FIXME (if possible):
		 * The following approaches have been tried:
		 * 1. Need to wait for a while before sending the mailbox message
		 * Possibly because though M0 is pending it in CRMU_IPROC_INTR_STATUS
		 * the GIC is not completely UP yet to register it.
		 *
		 * Somehow doing this before DRIPS exit() doesn't work.
		 * Alternate way is to wait on some ready register from A7.
		 *
		 * 2. Waiting Until A7 is Up & Running.
		 *
		 * Neither the 1.delay nor the 2.handshake mechanism
		 * to send a message to A7 work reliably. For some reason
		 * pending the interrupt in CRMU_IPROC_INTR_STATUS doesn't
		 * always result in the same pending on the GIC.
		 *
		 * volatile u32_t mproc_running = 0;
		 * sys_write32(mproc_running, A7_WOKEN_UP);
		 *
		 * while(mproc_running == 0)
		 *	mproc_running = sys_read32(A7_WOKEN_UP);
		 *
		 * MCU_send_msg_to_mproc(wakeup_msg_code0, wakeup_msg_code1);
		 */
	}
	else if (cur_pm_state == MPROC_PM_MODE__DEEPSLEEP) {
		/* Before sending the mailbox message to A7
		 * we need to power up PD_SYS.
		 */
		citadel_deep_sleep_exit();
		/* Need to wait for a while before sending the mailbox message
		 * Because though M0 is pending it in CRMU_IPROC_INTR_STATUS
		 * the GIC is not completely UP yet to register it.
		 *
		 * Waiting a bit longer than DRIPS.
		 *
		 * Alternate way is to wait on some ready register from A7.
		 */
		delay = 100000;
		while(delay--)
			;
		MCU_send_msg_to_mproc(wakeup_msg_code0, wakeup_msg_code1);
	}

	return 0;
}
