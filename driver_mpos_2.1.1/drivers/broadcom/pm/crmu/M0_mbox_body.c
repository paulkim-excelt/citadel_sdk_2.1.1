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
 * @brief crmu_cfg_xtal_pwr_off
 *
 * Power Off The Crystal Oscillator (26MHz)
 *
 * @param None
 *
 * @return N/A
 */
static void crmu_cfg_xtal_pwr_off(void)
{
	/* ]. Power Down XTAL
	 *
	 * CRMU_XTAL_CHANNEL_CONTROL1[28] (CRMU_XTAL_PWR_DET_ENA_REG) to 0,
	 * CRMU_XTAL_CHANNEL_CONTROL1[27] (CRMU_XTAL_RESETB) to 0
	 * (0 - XTAL Channels control bits reset, 1 - XTAL Channels active)
	 */
	sys_write32(sys_read32(CRMU_XTAL_CHANNEL_CONTROL1) & (~(3 << 27)), CRMU_XTAL_CHANNEL_CONTROL1);

	/* [1:0] =2'b00, [3:2] unused
	 * CRMU_XTAL_CMOS_CH_EN	3:0	Active high enables for XTAL clock.
	 * 0 - Gen PLL ref clock, 1 - USB Phy ref clock, Rest- Unused.
	 * When en=0, that particular xtal channel is gated off.
	 * Default: 0x3 (Both Gen PLL & USB Phy Ref clock channels are ON)
	 */
	sys_write32(sys_read32(CRMU_XTAL_CHANNEL_CONTROL1) & (~0xF), CRMU_XTAL_CHANNEL_CONTROL1);

	/* Field                  View    Bit  Description                    Default
	 *------------------------------------------------------------------------------
	 * CRMU_XTAL_PD_PW_DET    Common	3	Power down for power detector   0
	 * CRMU_XTAL_BUF_PD_REG   Common	2	XTAL Buf PD register control.   0
	 * CRMU_XTAL_ISO_EN       Common	1	ISOLATION enable for the output 0
	 * of the XTAL when powered down. 1-> Isolation enabled and output clamped to 0,
	 *                                0-> isolation disabled.
	 * CRMU_XTAL_OSC_POWER_ON Common	0	Power down the XTAL OSC. Must   1
	 * be set to 1 only when CRMU is not running of the XTAL clock.
	 * 1 - XTAL OSC is functional
	 * 0 - XTAL OSC is powered down
	 *
	 * Step 1: Enable XTAL isolation, bufpd=1
	 */
	sys_write32(0xF, CRMU_XTAL_POWER_DOWN);

	/* powering down the XTAL */
	sys_write32(0xE, CRMU_XTAL_POWER_DOWN);
}

/**
 * @brief MCU_ultra_low_power_enter
 *
 * Makes the MCU enter the ULP mode.
 *
 * @param None
 *
 * @return N/A
 */
static void MCU_ultra_low_power_enter(void)
{
	/* enter ULP mode, switch to BBL 32KHz clock from SPRU,
	 * power down the 26MHz XTAL OSC
	 */
#ifdef MPROC_PM__ULTRA_LOW_POWER
#ifdef MPROC_PM_ACCESS_BBL
	/* switch to BBL 32K clock from SPRU */
	sys_write32(0x2, CRMU_ULTRA_LOW_POWER_CTRL);
	while (sys_read32(CRMU_CLOCK_SWITCH_STATUS) != 0x4)
		;
#else
   /* switch to to SPL RO CLOCK */
	sys_write32(0x3, CRMU_ULTRA_LOW_POWER_CTRL);
	while (sys_read32(CRMU_CLOCK_SWITCH_STATUS) != 0x2)
	;
#endif

	/* Power Off XTAL */
	/* TODO : Verify if this is the case with Citadel
	 * seems cannot stably wakeup using this
	 */
	crmu_cfg_xtal_pwr_off();
#endif
}

/**
 * @brief citadel_sleep_enter
 *
 * TODO: Add Detailed Description (What does thsi function do?)
 * Description :
 *   - Switch clock from PLL to SPL
 *   - Power  off PLL
 *   - XTAL Power control and clock switch.
 *
 * @param None
 *
 * @return N/A
 */
static void citadel_sleep_enter(void)
{
#ifdef CITADEL_WFI_SLEEP_ENABLED
	u32_t rd_data;
	/* 1. Enable the LDO control */
	sys_write32(0x1, CRMU_LDO_CTRL);

	/* 2. Disable NFC, drive REG_PU low
	 * TODO: Is this required on Citadel?
	 */
	sys_write32(0x0, NFC_REG_PU_CTRL);

	/* 3. suspend USBPHY_D bit[27] pll_suspend_en */
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) & (~(1 << 27)), CDRU_USBPHY_D_CTRL1); /* bit1 = 0; */
	sys_write32(sys_read32(CRMU_USBPHY_D_CTRL) | 0x100000, CRMU_USBPHY_D_CTRL); /* phy_iso = 1 */
	/* 4. standby tx_stby and rx_stby */
	sys_write32(0xc, CDRU_USBD_MISC);

#ifdef MPROC_PM__ADV_S_POWER_OFF_PLL
	/* 5. CRMU_CLOCK_GATE_CONTROL, GATE UART, OTP and SOTP */
	rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
#ifdef MPROC_PM_ACCESS_BBL
	/* 6. Don't gate NIC, RTC & Tamper use it
	 * TODO: Use #defines after clarification
	 */
	rd_data = rd_data & (~(0xC000071D));
#else
	rd_data = rd_data & (~(0xC000071F));
#endif
	sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);

	/* 7. Gate MPROC clocks */
	sys_write32(0xFFFFFFFF, CDRU_CLK_DIS_CTRL);

	/* 8. Turn off ADC LDO: ADC0_ANALOG_CONTROL
	 * set bit no 10 for all 3 ADCs
	 * ADC_ISO_CONTROL  => 0x7
	 */
	rd_data = sys_read32(ADC0_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC0_ANALOG_CONTROL);
	rd_data = sys_read32(ADC1_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC1_ANALOG_CONTROL);
	rd_data = sys_read32(ADC2_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC2_ANALOG_CONTROL);

	sys_write32(0x7, ADC_ISO_CONTROL);
	sys_write32(0x0, CRMU_ADC_LDO_PU_CTRL);

	/* 9. disable PLL channels */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) | 0x00FC0000, CRMU_GENPLL_CONTROL1);

	/* 10. Enable ISO from pll_lock, bit 12, bit 16 */
	sys_write32(sys_read32(CRMU_ISO_CELL_CONTROL) | (~0xFFFEEFFF), CRMU_ISO_CELL_CONTROL);

	/* 11. Enable PLL ISO */
	sys_write32(1, CRMU_PLL_ISO_CTRL);

	/* 12. Power Off PLL */
	sys_write32(((1U << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN)
		| (1U << CRMU_PLL_AON_CTRL__GENPLL_PWRDN)
		| (1U << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);
	/* 13. Power off XTAL */
	MCU_ultra_low_power_enter();

#else
	/* 14. Only switch CLK21/41/81 from XTAL REFCLK 25MHz to SPL,
	 * and wait stable
	 */
	sys_write32(0x092, SYS_CLK_SOURCE_SEL_CTRL);
	while (sys_read32(SYS_CLK_SWITCH_READY_STAT) != 0x00000007)
		;
#endif

	/* 15. Lower the main LDO output voltage;
	 * i.e. lower PD_SYS VDD, after entering sleep  mode.
	 * The lowest voltage possible with the main ldo is 1.116V
	 * Set CRMU_LDO_CTRL_REG_28_0[22:18] (MAINLDO Vout) = 5'b11001
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFF83FFFF) | 0x00640000, CRMU_LDO_CTRL_REG_28_0);

	/* 16. Lower the AON LDO output voltage after entering sleep mode.
	 *   Set CRMU_LDO_CTRL_REG_28_0[3:0] (AONLDO Vout) = 4'b1011 (1.10V)
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFFFFFFF0) | 0xB, CRMU_LDO_CTRL_REG_28_0);
#endif
}

/** Turning OFF PD_CPU
 */
static s32_t ihost_pwr_down_seq(void)
{
	volatile u32_t local_spare0;
	volatile u32_t local_spare1;

	/* Step1 : Assert cluster and L2 memory isolation
	 * by programming DOMAIN_4_CTRL to 0x49
	 */
	 /* L2 memory isolation */
	sys_write32(sys_read32(A7_CRM_DOMAIN_4_CONTROL) | (1 << A7_CRM_DOMAIN_4_CONTROL__DOMAIN_4_ISO_MEM), A7_CRM_DOMAIN_4_CONTROL);
	/* Cluster Isolation */
	sys_write32(sys_read32(A7_CRM_DOMAIN_4_CONTROL) | (1 << A7_CRM_DOMAIN_4_CONTROL__DOMAIN_4_ISO_I_O), A7_CRM_DOMAIN_4_CONTROL);
	/* Cluster Isolation */
	sys_set_bit(A7_CRM_DOMAIN_4_CONTROL, A7_CRM_DOMAIN_4_CONTROL__DOMAIN_4_ISO_DFT);
	/* Step2 : Put PLL channel 0 and 4 in bypass enable
	 * by programming PLL_CHANNEL_BYPASS_ENABLE register.
	 */
	sys_set_bit(A7_CRM_PLL_CHANNEL_BYPASS_ENABLE, A7_CRM_PLL_CHANNEL_BYPASS_ENABLE__PLL_0_CHANNEL_4_BYPASS_ENABLE);
	sys_set_bit(A7_CRM_PLL_CHANNEL_BYPASS_ENABLE, A7_CRM_PLL_CHANNEL_BYPASS_ENABLE__PLL_0_CHANNEL_0_BYPASS_ENABLE);
	/* Step3 : Assert top AON isolation . */
	/* Remove AON isolation*/
	sys_set_bit(A7_CRM_DOMAIN_4_CONTROL, A7_CRM_DOMAIN_4_CONTROL__DOMAIN_4_ISO_AON );
	/* Step4 : Re-Assert all reset for next pwr cycle
	 * by programming Soft resetin 0/1 registers
	 */
	sys_write32(0x0, A7_CRM_SOFTRESETN_0);
	sys_write32(0x0, A7_CRM_SOFTRESETN_1);
	/* Step5 : switch off PLL.LDO and PLL logic by programming
	 * PLL_PWR_ON register. Check both modes wherein LDO & PLL
	 * are up and both down.
	 */
	sys_clear_bit(A7_CRM_PLL_PWR_ON, A7_CRM_PLL_PWR_ON__PLL0_PWRON_PLL);
	sys_clear_bit(A7_CRM_PLL_PWR_ON, A7_CRM_PLL_PWR_ON__PLL0_PWRON_LDO);

	/* Step6 : Powerdown strong switches followed by weak
	 * switches by programming PWRON_IN_DOMAIN_4 / PWROK_IN_DOMAIN_4
	 * registers.
	 */
	/* Strong switch programming */
	local_spare0 = 0xFe;
	while (local_spare0 != 0x0) {
		sys_write32(local_spare0, A7_CRM_PWROK_IN_DOMAIN_4);
		while ((sys_read32(A7_CRM_PWROK_OUT_DOMAIN_4) & 0xff)
			!=local_spare0)
			;
		local_spare0 = ((local_spare0 << 1 ) & 0xff);
	}

	/* Weak switch programming */
	local_spare1 = 0xFe;
	while (local_spare1 != 0x0) {
		sys_write32(local_spare1, A7_CRM_PWRON_IN_DOMAIN_4);
		while ((sys_read32(A7_CRM_PWRON_OUT_DOMAIN_4) & 0xff)
			!=local_spare1)
			;
		local_spare1 = ((local_spare1 << 1 ) & 0xff);
	}

	return 1;
}

static void citadel_power_off_IO_ring(void)
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
	/* Set Bit 2 of GP_DATA_OUT register to '1' to Turn Off
	 * Power to all of A7's GPIOs (non-AON).
	 */
	sys_write32((sys_read32(GP_DATA_OUT)) | 0x4, GP_DATA_OUT);
#endif
}

static void citadel_power_off_usb(void)
{
	/* Notes: Existing Sequence from SV & Lynx Code.
	 * FIXME: Compare the impact of the following sequence
	 * on power and stability of recovery using usb as wake-up
	 * source (both device & host).
	 *
	 * usb0 device phy power down sequence:
	 *--------------------------------------
	 * reg32_write(CDRU_USBPHY_D_CTRL1, 0x0224098b);
	 * reg32_write(CDRU_USBPHY_D_CTRL2, 0x00000480);
	 * reg32_write(CDRU_USBPHY_D_P1CTRL,0x00000005);
	 * reg32_write(CRMU_USBPHY_D_CTRL, 0x00166666);
	 *
	 * usb0 host phy power down sequence:
	 *------------------------------------
	 * reg32_write(CDRU_USBPHY_H_CTRL1, 0x0224098b);
	 * reg32_write(CDRU_USBPHY_H_CTRL2, 0x00000480);
	 * reg32_write(CDRU_USBPHY_H_P1CTRL,0x00000005);
	 * reg32_write(CRMU_USBPHY_H_CTRL, 0x00166666);
	 */
#ifdef CITADEL_A7_USB_PHY_POWER_OFF
	/* 2. Power Down USBD PHY
	 *-------------------------
	 *
	 * 3. suspend USBPHY_D bit[27] pll_suspend_en
	 *--------------------------------------------
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
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) & (~(1 << 27)), CDRU_USBPHY_D_CTRL1); /* bit27 = 0; */
	sys_write32(sys_read32(CDRU_USBPHY_D_CTRL1) & ~(0x3), CDRU_USBPHY_D_CTRL1); /* bits[1:0] = 0x0; */

	/* CRMU_USBPHY_D_CTRL
	 * 31:21 Reserved ro Reserved, Default: 0
	 * 20 phy_iso rw Used for Isolation of PHY outputs to the Core. Default: 1
	 * 19:0 pll_ndiv_frac rw PLL feedback divider fraction control. Default: 0x66666
	 *
	 * FIXME: Why are we programming default values here?
	 */
	sys_write32(sys_read32(CRMU_USBPHY_D_CTRL) | 0x100000, CRMU_USBPHY_D_CTRL); /* phy_iso = 1 */

	/* CDRU_USBPHY_D_CTRL2
	 * 31:11: Reserved     ro Reserved, Default: 0
	 * 10   : iddq         rw Active High. PHY is completely shut-down. Default: 1
	 * 9    : pll_bypass   rw 0 - afe_oclk96: internal oclk96 1 - afe_oclk96: afe_fref_cmos. Default: 0
	 * 8:5  : ldobg_outadj rw Bandgap current reference adjust bits. Default: 0x4
	 * 4:2  : afeldocntl   rw LDO Control 0x0
	 * 1    : afeldocntlen rw LDO Control Enable. This should be HIGH for LDO operation. Default: 0
	 * 0    : ldo_pwrdwnb  rw Power Down. 0
	 *
	 * FIXME: Why are we programming default values here? Is this required?
	 *
	 * 	sys_write32(0x080, CDRU_USBPHY_D_CTRL2);
	 */

	/* 2.1. standby tx_stby and rx_stby
	 * 31:4 - Reserved ro Reserved 0
	 * 3 - tx_stby rw 1 - Puts USBD Tx memory in standby 0 - Moves USBD Tx memory out of standby state 0
	 * 2 - rx_stby rw 1 - Puts USBD Rx memory in standby 0 - Moves USBD Rx memory out of standby state 0
	 * 1 - override_l1_suspend_select rw When set, value in bit1 (utmi_l1_suspendm) of CDRU_USBPHY_D_CTRL1
	 * register drives L1 suspend signals to PHY, Else USB Device Controller drives L1 suspend to PHY. 0
	 * 0 - override_l1_sleep_ovr_select rw When set, value in bit0 (utmi_l1_sleepm) of CDRU_USBPHY_D_CTRL1
	 * register drives L1 suspend signals to PHY, Else USB Device Controller drives L1 suspend to PHY. 0
	 */
	sys_write32(sys_read32(CDRU_USBD_MISC) | 0xF, CDRU_USBD_MISC);
#endif
}

static void citadel_power_off_nfc(void)
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
	/* Power OFF */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) & ~0x40, NFC_REG_PU_CTRL);

	/* Clear Bit 4: Hysterisis Enable; Bit 1: Slew Rate Control */
	sys_write32(sys_read32(NFC_CLK_REQ_CTRL) & ~0x12, NFC_CLK_REQ_CTRL);

	/* Clear Bit 4: Hysterisis Enable; Bit 1: Slew Rate Control */
	sys_write32(sys_read32(NFC_HOST_WAKE_CTRL) & ~0x12, NFC_HOST_WAKE_CTRL);

	/* Clear Bits 3 (Drive 2) - 5 (Drive 0) */
	sys_write32(sys_read32(NFC_NFC_WAKE_CTRL) & ~0x38, NFC_NFC_WAKE_CTRL);

	/* NFC pins enable drive strength and Hysteresis
	 * Clear Bits 3 (Drive 2) - 5 (Drive 0)
	 */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) & ~0x38, NFC_REG_PU_CTRL);

	/* Make SPI_INT PAd as input pin */
	/* Enable Hysteresis and Slewrate */
	sys_write32((sys_read32(NFC_SPI_INT_CTRL) | 0x1) & ~0x12, NFC_SPI_INT_CTRL);

	/* make Reg PU OEB to 0 */
	sys_write32(sys_read32(NFC_REG_PU_CTRL) | 0x01, NFC_REG_PU_CTRL);
#endif
}

/**
 * @brief citadel_DRIPS_enter
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description : This function will essentially turn PD_CPU off
 *	It also shuts down the A7 PLL and the LDO supplying power to this PLL.
 * @param None
 *
 * @return N/A
 */
static s32_t citadel_DRIPS_enter(void)
{
	u32_t rd_data;

	/* Bare Minimum Power Optimization For DRIPS:
	 * Turn Off IHOST Power. This powers (on / off)
	 * A7, GIC, DMA, GenTimer etc.
	 */
	ihost_pwr_down_seq();

	/* 1. PD_NFC power down *
	 * Observation: Using this line of code, makes the DRIPS very unstable
	 * sometimes can't wake-up from DRIPS. When such a situation happens
	 * it is noticed that the voltage across the 0.02ohm resistor is very
	 * high, ~1.3mV instead of 0.4mV. Don't know if this is board issue
	 * or somehow this control is creating havoc. The problem happens with
	 * both the lines of code.
	 *
	 * Disable NFC, drive REG_PU low
	 * sys_write32(sys_read32(NFC_REG_PU_CTRL) & ~(0x1 << NFC_REG_PU_CTRL__DATA_OUT), NFC_REG_PU_CTRL);
	 * OR
	 * sys_write32(0x0, NFC_REG_PU_CTRL);
	 */
	citadel_power_off_nfc();

#ifdef CITADEL_A7_POWER_OFF_ADC
	/* Turn off ADC LDO: ADC0_ANALOG_CONTROL
	 * set bit no 10 for all 3 ADCs
	 * ADC_ISO_CONTROL  => 0x7
	 */
	sys_write32(sys_read32(ADC0_ANALOG_CONTROL) & (~(0x1 << 10)), ADC0_ANALOG_CONTROL);
	sys_write32(sys_read32(ADC1_ANALOG_CONTROL) & (~(0x1 << 10)), ADC1_ANALOG_CONTROL);
	sys_write32(sys_read32(ADC2_ANALOG_CONTROL) & (~(0x1 << 10)), ADC2_ANALOG_CONTROL);

	sys_write32(0x7, ADC_ISO_CONTROL);
	sys_write32(0x0, CRMU_ADC_LDO_PU_CTRL);
#endif

/* PLL power off sequence is preceding Clock disable sequence
 *
 * FIXME: As of now, Wake-up from DRIPS fails using the following
 * sequence to power off PLLs.
 */
#ifdef CITADEL_A7_POWER_OFF_PLLS
	/* 9. disable PLL channels
	 * Disable Gen PLL channels 1 ~ 6 by setting bits 18 ~ 23 of CRMU_GENPLL_CONTROL1.
	 * FIXME: Why there is not corresponding programming of this register
	 * in LP exit?
	 */
	sys_write32(sys_read32(CRMU_GENPLL_CONTROL1) | 0x00FC0000, CRMU_GENPLL_CONTROL1);

	/* 10. Enable Isolation of PDSYS_PLL, bit 12
	 * FIXME: Why is PDSYS Isolation being enabled?
	 * FIXME: Why are we not enabling the Isolation of PDBBL, bit 16?
	 * Is it because we want PDBBL to be functional since we are in LP mode?
	 *
	 * The corresponding programming of this register in LP exit, to make PDSYS
	 * Functional is done inside the function bcm_set_mhost_pll_max_freq().
	 */
	sys_write32(sys_read32(CRMU_ISO_CELL_CONTROL) | (1 << CRMU_ISO_CELL_CONTROL__CRMU_ISO_PDSYS_PLL_LOCKED), CRMU_ISO_CELL_CONTROL);

	/* 11. Enable PLL ISO
	 * When enabled or set to '1' the outputs of the CRMU PLLs
	 * are isolated.
	 * FIXME: Why SV Code repeats the following two lines?
	 */
	sys_write32(1, CRMU_PLL_ISO_CTRL);
	/* 12. Power Off PLL
	 * Bit 2: GENPLL_FREF_PWRDN -> Enable CMOS Reference Clock; 0: normal operation; 1: shutdown low-power mode
	 * Bit 1: GENPLL_PWRDN -> PLL + LDO Power Down; 0: Normal Operation; 1: Shutdown low-power mode
	 * Bit 0: GENPLL_ISO_IN -> GENPLL Input isolation enable
	 */
	sys_write32(((1U << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN)
		| (1U << CRMU_PLL_AON_CTRL__GENPLL_PWRDN)
		| (1U << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN)), CRMU_PLL_AON_CTRL);
#endif

#ifdef CITADEL_A7_DISABLE_CLOCKS
	/* 0.1. CRMU_CLOCK_GATE_CONTROL, GATE UART, OTP and SOTP
	 * The following doesn't work with RTC failing.
	 * rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
	 * rd_data = rd_data & (~(0xC000071F));
	 * sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);
	 */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_BSPI_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_UART_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SOTP_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SMBUS_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_OTPC_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	/* FIXME: Why was SPRU_CRMU_REF_CLK not included in Lynx? What are the
	 * repercusions? It seems to work fine with RTC as wakeup in DRIPS though
	 * from name it appears to be the clock responsible for RTC.
	 */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_SPRU_CRMU_REF_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_CDRU_CLK_25_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_TIM_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_WDT_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_MCU_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
#ifndef MPROC_PM_ACCESS_BBL
	/* FIXME: Don't enable gating of NIC Clock, because RTC & Tamper depend
	 * on it. Is this still TRUE in Citadel?
	 */
	sys_write32(sys_read32(CRMU_CLOCK_GATE_CONTROL) & ~(1 << CRMU_CLOCK_GATE_CONTROL__CRMU_NIC_CLK_ENABLE), CRMU_CLOCK_GATE_CONTROL);
#endif

#ifdef MPROC_PM_ACCESS_BBL
	/* MPROC Clocks Switch to BBL Clock (1)
	 * Reg: SYS_CLK_SOURCE_SEL_CTRL (0 0100 1001)
	 */
	sys_write32(0x49, SYS_CLK_SOURCE_SEL_CTRL);
#else
	/* MPROC Clocks Switch to SPL RO Clock (2)
	 * Reg: SYS_CLK_SOURCE_SEL_CTRL (0 1001 0010)
	 */
	sys_write32(0x92, SYS_CLK_SOURCE_SEL_CTRL);
#endif
	/* FIXME: Why is it not there in SV code?
	 * This line causes general DRIPS wakeup failure, Why?
	 *
	 * while (sys_read32(SYS_CLK_SWITCH_READY_STAT) & 0x7 != 0x7);
	 *
	 * By the definition of the register, it is supposed to
	 * give the status of Clock switch, right?
	 */

	/* 7. Gate MPROC clocks */
	sys_write32(0xFFFFFFFF, CDRU_CLK_DIS_CTRL);
#endif

	/* The following partly works and reduces from 4.1mV to 1.2mV (i.e. 65mA).
	 * FIXME: Currently when 26MHz clock is turned OFF upon entry to LP Mode
	 * and turned back on upon exit, the CRMU_TIM_TIMER2 doesn't work. Since
	 * this is the timer which the SPRU reg reads & writes depend on, the
	 * DRIPS mode exit gets stuck waiting for this timer to expire which
	 * never happens. Note that when RTC & Tamper are enabled DRIPS exit
	 * involves some SPRU reg reads and writes.
	 *
	 * Consecutive & Multiple DRIPS exit doesn't work, most likely because
	 * the clock source to the CRMU_TIM_TIMER2 is off.
	 *
	 * However, since Dell doesn't have BBL, turning this feature OFF for
	 * now so that Dell integration may proceed. But this needs to be FIXED
	 * eventually.
	 *
	 * FIXME: This sequence possibly results in DRIPS instability.
	 */
	MCU_ultra_low_power_enter();

	/* Power Off USB */
	citadel_power_off_usb();

	/* Power the IO Ring OFF
	 * FIXME: This sequence possibly contributes to DRIPS
	 * wake-up instability.
	 */
	citadel_power_off_IO_ring();

	return 0;
}


/**
 * @brief citadel_deep_sleep_enter
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 *
 * @param None
 *
 * @return N/A
 */
static s32_t citadel_deep_sleep_enter(void)
{
	u32_t rd_data = 0;

#ifdef MPROC_PM__ADVANCED_DS
	sys_write32(0x1, CRMU_LDO_CTRL); /* enable program pmu */

	/* 0. Gate MPROC clocks */
	sys_write32(0xFFFFFFFF, CDRU_CLK_DIS_CTRL);

	/* 0.1. CRMU_CLOCK_GATE_CONTROL , GATE UART, OTP and SOTP */
	rd_data = sys_read32(CRMU_CLOCK_GATE_CONTROL);
#ifdef MPROC_PM_ACCESS_BBL
	/* Don't gate NIC Clock, RTC & Tamper use it.
	 *
	 * FIXME: How about the CDRU Clock (bit 4)? Why are we gating it?
	 * Doesn't it have any impact of the SPRU usage?
	 *
	 * FIXME: How about the TIM Clock (bit 3)? Why are we gating it?
	 * The RTC & Tamper wake-up seem to use CRMU_TIMERs and it seems
	 * these timers use this clock, is that right?
	 *
	 */
	rd_data = rd_data & (~(0xC000071D));
#else
	rd_data = rd_data & (~(0xC000071F));
#endif
	sys_write32(rd_data, CRMU_CLOCK_GATE_CONTROL);

	/* 1. PD_NFC power down */
	sys_clear_bit(NFC_REG_PU_CTRL, NFC_REG_PU_CTRL__DATA_OUT);

	/* 2. Power Down USBD PHY */
	sys_write32(0x166666, CRMU_USBPHY_D_CTRL);
	sys_write32(0x080, CDRU_USBPHY_D_CTRL2);
	/* 2.1. standby tx_stby and rx_stby */
	sys_write32(0xC, CDRU_USBD_MISC);

	/* 2.2 Turn off ADC LDO: ADC0_ANALOG_CONTROL
	 * set bit no 10 for all 3 ADCs
	 *
	 * 2.3 ADC_ISO_CONTROL  => 0x7
	 */
	rd_data = sys_read32(ADC0_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC0_ANALOG_CONTROL);
	rd_data = sys_read32(ADC1_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC1_ANALOG_CONTROL);
	rd_data = sys_read32(ADC2_ANALOG_CONTROL);
	rd_data = rd_data & (~(1 << 10));
	sys_write32(rd_data, ADC2_ANALOG_CONTROL);

	sys_write32(0x7, ADC_ISO_CONTROL);
	sys_write32(0x0, CRMU_ADC_LDO_PU_CTRL);
#endif

	/* Basic DeepSleep */
	/* 3. CDRU_APB RESET */
	sys_write32(0x0, CRMU_CDRU_APB_RESET_CTRL);

	/* 4. Power down PLL */
	sys_write32(1, CRMU_PLL_ISO_CTRL);
	rd_data = sys_read32(CRMU_PLL_AON_CTRL);
	sys_write32(rd_data | 0x7, CRMU_PLL_AON_CTRL);

	/* 5. Enable PD_SYS ISO and PD_SYS_POWER_GOOD ISO */
	rd_data = sys_read32(CRMU_ISO_CELL_CONTROL);
	sys_write32(rd_data | 0x1101, CRMU_ISO_CELL_CONTROL);

	/* 6. Disable PD_SYS SWREG, Power down PD_SYS */
	sys_write32(0x0, CRMU_MAIN_LDO_PU_CTRL);

#ifdef MPROC_PM__ADVANCED_DS
	/* TODO: Is this the right place to turn GPIO power OFF?
	 *
	 * 7a. Make AON_GPIO_2 an Output so that it can drive
	 * the control for the GPIO Ring Power on A7
	 * sys_write32((sys_read32(GP_RES_EN) | 0x2, GP_OUT_EN);
	 */

	/* 7. Power Down PD_SYS I/O pads, AON_GPIO_1 = 0 */
	sys_write32((sys_read32(GP_DATA_OUT)) & 0x7fd, GP_DATA_OUT);

	/* 8. set USH_AWAKE# (AON_GPIO_5)to input pull up.
	 * FP_SPI_NAWAKE, fp_spi_config() will reset this
	 * pin when wakeup.
	 */
	sys_write32(0x20, GP_TEST_ENABLE);
	sys_write32(0x00, GP_TEST_INPUT);
#endif

	/* 9. XTAL power down */
	MCU_ultra_low_power_enter();

#ifdef MPROC_PM__ADVANCED_DS
	/* 10. Lower the AON LDO output voltage after entering deep sleep mode.
	 * Set CRMU_LDO_CTRL_REG_28_0[3:0] (AONLDO Vout) = 4'b1110 (1.00V)
	 */
	sys_write32((sys_read32(CRMU_LDO_CTRL_REG_28_0) & 0XFFFFFFF0) | 0xB, CRMU_LDO_CTRL_REG_28_0);

	/* Power the IO Ring OFF */
	citadel_power_off_IO_ring();
#endif

	return 0;
}

/*****************************************************************************
 * P U B L I C   A P I s
 *****************************************************************************/
/**
 * @brief MCU_SoC_do_policy
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 *
 * @param None
 *
 * @return N/A
 */
s32_t MCU_SoC_do_policy(void)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	if (pm_args->do_policy == 0 || pm_args->tgt_pm_state >= MPROC_PM_MODE__END)
		return 0;

	/* do all user defined policies... */

	return 1;
}

/**
 * @brief MCU_SoC_Run_Handler
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 *
 * Core = Clock lower
 *
 * @param State
 *
 * @return result
 */
s32_t MCU_SoC_Run_Handler(MPROC_PM_MODE_e state)
{
	s32_t ret = 0;

	/* A7 need not stay in WFI state, directly wakeup ihost in case */
	MCU_send_msg_to_mproc(MAILBOX_CODE0__NOTHING, MAILBOX_CODE0__NOTHING);

	return ret;
}

/**
 * @brief MCU_SoC_Sleep_Handler
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 *
 * Core = Clock Gate
 *
 * @param None
 *
 * @return result
 */
s32_t MCU_SoC_Sleep_Handler(void)
{
#ifdef MPROC_PM__ADVANCED_S
	citadel_sleep_enter();
#else
	MCU_set_pdsys_master_clock_gating(1); /* simple sleep */
#endif

	return 0;
}

/**
 * @brief MCU_SoC_DRIPS_Handler
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 * DRIPS = A7 Core is powered down, But system RAM (which is part of
 * MPROC-Ihost)power is preserved. Rest of MPROC (other than A7 & GIC)
 * is only clock gated. Wake-up source are same as that of Sleep. However,
 * Since SRAM is powered on, the content is not lost, but CPU & GIC are
 * powered off, so their content(state) has to be preserved somewhere and
 * restored upon wake-up from DRIPS. JTAG will be lost. No Code execution
 * is possible through JTAG because the core-sight slave inside the A7 CPU
 * is powered down. However, since the coresight master has access to the
 * buses some register of the components such as WDT, NIC, DMA etc. could
 * be read.
 *
 * TODO: This function need to be implemented
 *
 * Core = Clock Gate & A7 Power Gate
 *
 * @param None
 *
 * @return result
 */
s32_t MCU_SoC_DRIPS_Handler(void)
{
	s32_t res = 0;

#ifdef MPROC_PM__ADVANCED_S
	res = citadel_DRIPS_enter();
#else
	MCU_set_pdsys_master_clock_gating(1); /* simple sleep */
#endif

	return res;
}

/**
 * @brief MCU_SoC_DeepSleep_Handler
 *
 * TODO: Add Detailed Description (What does this function do?)
 * Description :
 *
 * Core = Power Off
 * DMU  = Policy
 * System = Power Off
 *
 * @param None
 *
 * @return result
 */
s32_t MCU_SoC_DeepSleep_Handler(void)
{
	s32_t res = 0;

	res = citadel_deep_sleep_enter();

	return res;
}
