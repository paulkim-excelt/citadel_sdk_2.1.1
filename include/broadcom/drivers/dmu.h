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

/* @file dmu.h
 *
 * @brief Public APIs for the DMU drivers.
 */

#ifndef _DMU_H_
#define _DMU_H_

#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief DMU common space block configuration structure.
 *
 * Please refer the data sheet for detailed register description
 */
struct dmu_regs {
	u32_t crmu_strap_data;
	u32_t crmu_iomux_control;
	u32_t unused0[6]; /* gpio driver will use this */
	u32_t aon_uart_tx_control;
	u32_t aon_bsc_io_control;
	u32_t iproc_crmu_mail_box0;
	u32_t iproc_crmu_mail_box1;
	u32_t crmu_iproc_mail_box0;
	u32_t crmu_iproc_mail_box1;
	u32_t crmu_usbphy0_wake_intr_gen;
	u32_t crmu_usbphy1_wake_intr_gen;
	u32_t crmu_ihost_power_status;
	u32_t crmu_ihost_power_config;
	u32_t crmu_power_state_reg;
	u32_t crmu_ihost_wfe_wake_event;
	u32_t crmu_mcu_intr_status;
	u32_t crmu_mcu_intr_mask;
	u32_t crmu_mcu_intr_clear;
	u32_t crmu_mcu_event_status;
	u32_t crmu_mcu_event_clear;
	u32_t crmu_mcu_event_mask;
	u32_t crmu_iproc_intr_status;
	u32_t crmu_iproc_intr_mask;
	u32_t crmu_iproc_intr_clear;
	u32_t crmu_reset_event_log_register;
	u32_t crmu_power_event_log_register;
	u32_t crmu_error_event_log_register;
	u32_t crmu_mcu_axi_dec_err_stat;
	u32_t crmu_mcu_axi_dec_err_wr_addr;
	u32_t crmu_mcu_axi_dec_err_rd_addr;
	u32_t crmu_iproc_axi_dec_err_stat;
	u32_t crmu_iproc_axi_dec_err_wr_addr;
	u32_t crmu_iproc_axi_dec_err_rd_addr;
	u32_t crmu_mcu_clock_glitch_tamper_stat;
	u32_t crmu_mcu_security_stat;
	u32_t crmu_soft_reset_ctrl;
	u32_t crmu_master_axi_config;
	u32_t crmu_spl_reset_control;
	u32_t crmu_usbphy_wake_control;
	u32_t usbphy_resume_event_log;
	u32_t crmu_spl_event_log;
	u32_t cgm_cnt_cntl_25m;
	u32_t cgm_dly_cntl_25m;
	u32_t cgm_cnt_thres_25m;
	u32_t cgm_cnt_cntl_32k;
	u32_t cgm_dly_cntl_32k;
	u32_t cgm_cnt_thres_32k;
	u32_t unused1[6];
	u32_t crmu_ram_ecc_status;
	u32_t unused11[709];
	u32_t crmu_mcu_access_control;
	u32_t global_lock_out_control;
	u32_t bbram_lock_out_control;
	u32_t iproc_lock_out_control;
	u32_t unused2[16];
	u32_t crmu_ihost_por_wakeup_flag;
	u32_t crmu_ihost_sw_persistent_reg0;
	u32_t crmu_ihost_sw_persistent_reg1;
	u32_t crmu_ihost_sw_persistent_reg2;
	u32_t crmu_ihost_sw_persistent_reg3;
	u32_t crmu_ihost_sw_persistent_reg4;
	u32_t crmu_ihost_sw_persistent_reg5;
	u32_t crmu_ihost_sw_persistent_reg6;
	u32_t crmu_ihost_sw_persistent_reg7;
	u32_t crmu_ihost_sw_persistent_reg8;
	u32_t crmu_ihost_sw_persistent_reg9;
	u32_t crmu_ihost_sw_persistent_reg10;
	u32_t crmu_ihost_sw_persistent_reg11;
	u32_t crmu_bbl_auth_code;
	u32_t crmu_bbl_auth_check;
	u32_t crmu_config_ecc_rtic_enable;
	u32_t crmu_bbl_clear_enable;
	u32_t crmu_sotp_neutralize_enable;
	u32_t smbus_alert_program;
	u32_t smbus_alert_config;
};

/**
 * @brief Chip Device Resource Unit block configuration structure.
 *
 * Please refer the data sheet for detailed register description
 */
struct cdru_regs {
	u32_t crmu_genpll_control0;
	u32_t crmu_genpll_control1;
	u32_t crmu_genpll_control2;
	u32_t crmu_genpll_control3;
	u32_t crmu_genpll_control4;
	u32_t crmu_genpll_control5;
	u32_t crmu_genpll_control6;
	u32_t crmu_genpll_control7;
	u32_t crmu_genpll_control8;
	u32_t crmu_genpll_status;
	u32_t crmu_pll_iso_ctrl;
	u32_t crmu_keypad_clk_div;
	u32_t crmu_pwm_clk_div;
	u32_t crmu_adc_clk_div;
	u32_t sys_clk_source_sel_ctrl;
	u32_t sys_clk_switch_ready_stat;
	u32_t crmu_sw_por_reset_ctrl;
	u32_t crmu_reset_ctrl;
	u32_t crmu_chip_por_ctrl;
	u32_t crmu_chip_strap_ctrl;
	u32_t crmu_chip_strap_ctrl2;
	u32_t crmu_chip_io_pad_control;
	u32_t crmu_chip_strap_data;
	u32_t crmu_chip_debug_test_strap;
	u32_t sotp_ready_status;
	u32_t sys_padring_status;
	u32_t crmu_sotp_status;
	u32_t cdru_rom_ctrl1;
	u32_t cdru_rom_stat;
	u32_t cdru_sram_ctrl1;
	u32_t cdru_arm_m3_ctrl;
	u32_t cdru_idm_timeout;
	u32_t cdru_arm_m3_status;
	u32_t cdru_qspi_ctrl;
	u32_t cdru_qspi_status;
	u32_t cdru_clk_dis_ctrl;
	u32_t cdru_sw_reset_ctrl;
	u32_t cdru_uart_misc;
	u32_t cdru_sci_misc;
	u32_t cdru_usbd_io_ctrl;
	u32_t cdru_usbd_misc;
	u32_t cdru_usbd_status0;
	u32_t cdru_usbh_io_ctrl;
	u32_t cdru_usbh_misc;
	u32_t cdru_usbh_status0;
	u32_t cdru_usbphy_h_ctrl1;
	u32_t cdru_usbphy_h_status;
	u32_t cdru_usbphy_h_p1ctrl;
	u32_t cdru_usbphy_h_ctrl2;
	u32_t cdru_usbphy_d_ctrl1;
	u32_t cdru_usbphy_d_status;
	u32_t cdru_usbphy_d_p1ctrl;
	u32_t cdru_usbphy_d_ctrl2;
	u32_t iomux_ctrl[69];
	u32_t cdru_smc_address_match_mask;
};

/**
 * @brief CRMU Device Resource Unit block configuration structure.
 *
 * Please refer the data sheet for detailed register description
 */
struct dru_regs {
	u32_t crmu_xtal_channel_control1;
	u32_t crmu_xtal_channel_control2;
	u32_t crmu_ldo_ctrl;
	u32_t crmu_main_ldo_pu_ctrl;
	u32_t crmu_adc_ldo_pu_ctrl;
	u32_t crmu_ldo_ctrl_reg_28_0;
	u32_t crmu_ldo_ctrl_reg_39_31;
	u32_t crmu_swreg_ctrl;
	u32_t crmu_pll_aon_ctrl;
	u32_t crmu_usbphy_d_ctrl;
	u32_t crmu_usbphy_h_ctrl;
	u32_t crmu_usbphy_d_status;
	u32_t crmu_usbphy_h_status;
	u32_t crmu_pwr_good_status;
	u32_t adc_iso_control;
	u32_t crmu_power_req_cfg;
	u32_t crmu_power_poll;
	u32_t crmu_iso_cell_control;
	u32_t crmu_cdru_apb_reset_ctrl;
	u32_t crmu_spru_source_sel_stat;
	u32_t crmu_clock_gate_control;
	u32_t crmu_spl_clk_switch_timeout;
	u32_t crmu_ultra_low_power_ctrl;
	u32_t crmu_clock_switch_status;
	u32_t crmu_xtal_power_down;
	u32_t crmu_chip_otpc_rst_cntrl;
	u32_t crmu_chip_otpc_status;
	u32_t otp_feature_disable_ctrl0;
	u32_t otp_feature_disable_ctrl1;
	u32_t otp_override_code;
	u32_t otp_override_valid;
	u32_t otp_override_data0;
	u32_t otp_override_data1;
	u32_t otp_override_data2;
	u32_t otp_override_data3;
	u32_t otp_override_data6;
	u32_t otp_override_data7;
	u32_t otp_lvm_override_select;
	u32_t crmu_id_scr_mem_cofig;
	u32_t crmu_bisr_pdg_mask;
	u32_t crmu_aon_gpio_otpc_config;
};

/* DMU common registers*/
enum dmu_common_regs {
	CIT_CRMU_STRAP_DATA = offsetof(struct dmu_regs, crmu_strap_data),
	CIT_CRMU_IOMUX_CONTROL = offsetof(struct dmu_regs, crmu_iomux_control),
	CIT_AON_UART_TX_CONTROL = offsetof(struct dmu_regs,
					   aon_uart_tx_control),
	CIT_AON_BSC_IO_CONTROL = offsetof(struct dmu_regs, aon_bsc_io_control),
	CIT_IPROC_CRMU_MAIL_BOX0 = offsetof(struct dmu_regs,
					    iproc_crmu_mail_box0),
	CIT_IPROC_CRMU_MAIL_BOX1 = offsetof(struct dmu_regs,
					    iproc_crmu_mail_box1),
	CIT_CRMU_IPROC_MAIL_BOX0 = offsetof(struct dmu_regs,
					    crmu_iproc_mail_box0),
	CIT_CRMU_IPROC_MAIL_BOX1 = offsetof(struct dmu_regs,
					    crmu_iproc_mail_box1),
	CIT_CRMU_USBPHY0_WAKE_INTR_GEN = offsetof(struct dmu_regs,
						  crmu_usbphy0_wake_intr_gen),
	CIT_CRMU_USBPHY1_WAKE_INTR_GEN = offsetof(struct dmu_regs,
						  crmu_usbphy1_wake_intr_gen),
	CIT_CRMU_IHOST_POWER_STATUS = offsetof(struct dmu_regs,
					       crmu_ihost_power_status),
	CIT_CRMU_IHOST_POWER_CONFIG = offsetof(struct dmu_regs,
					       crmu_ihost_power_config),
	CIT_CRMU_POWER_STATE_REG = offsetof(struct dmu_regs,
					    crmu_power_state_reg),
	CIT_CRMU_IHOST_WFE_WAKE_EVENT = offsetof(struct dmu_regs,
						 crmu_ihost_wfe_wake_event),
	CIT_CRMU_MCU_INTR_STATUS = offsetof(struct dmu_regs,
					    crmu_mcu_intr_status),
	CIT_CRMU_MCU_INTR_MASK = offsetof(struct dmu_regs, crmu_mcu_intr_mask),
	CIT_CRMU_MCU_INTR_CLEAR = offsetof(struct dmu_regs,
					   crmu_mcu_intr_clear),
	CIT_CRMU_MCU_EVENT_STATUS = offsetof(struct dmu_regs,
					     crmu_mcu_event_status),
	CIT_CRMU_MCU_EVENT_CLEAR = offsetof(struct dmu_regs,
					    crmu_mcu_event_clear),
	CIT_CRMU_MCU_EVENT_MASK = offsetof(struct dmu_regs,
					   crmu_mcu_event_mask),
	CIT_CRMU_IPROC_INTR_STATUS = offsetof(struct dmu_regs,
					      crmu_iproc_intr_status),
	CIT_CRMU_IPROC_INTR_MASK = offsetof(struct dmu_regs,
					    crmu_iproc_intr_mask),
	CIT_CRMU_IPROC_INTR_CLEAR = offsetof(struct dmu_regs,
					     crmu_iproc_intr_clear),
	CIT_CRMU_RESET_EVENT_LOG_REGISTER = offsetof(struct dmu_regs,
					     crmu_reset_event_log_register),
	CIT_CRMU_POWER_EVENT_LOG_REGISTER = offsetof(struct dmu_regs,
					 crmu_power_event_log_register),
	CIT_CRMU_ERROR_EVENT_LOG_REGISTER = offsetof(struct dmu_regs,
					 crmu_error_event_log_register),
	CIT_CRMU_MCU_AXI_DEC_ERR_STAT = offsetof(struct dmu_regs,
						 crmu_mcu_axi_dec_err_stat),
	CIT_CRMU_MCU_AXI_DEC_ERR_WR_ADDR = offsetof(struct dmu_regs,
						 crmu_mcu_axi_dec_err_wr_addr),
	CIT_CRMU_MCU_AXI_DEC_ERR_RD_ADDR = offsetof(struct dmu_regs,
						 crmu_mcu_axi_dec_err_rd_addr),
	CIT_CRMU_IPROC_AXI_DEC_ERR_STAT = offsetof(struct dmu_regs,
						 crmu_iproc_axi_dec_err_stat),
	CIT_CRMU_IPROC_AXI_DEC_ERR_WR_ADDR = offsetof(struct dmu_regs,
					crmu_iproc_axi_dec_err_wr_addr),
	CIT_CRMU_IPROC_AXI_DEC_ERR_RD_ADDR = offsetof(struct dmu_regs,
					crmu_iproc_axi_dec_err_rd_addr),
	CIT_CRMU_MCU_CLOCK_GLITCH_TAMPER_STAT = offsetof(struct dmu_regs,
					crmu_mcu_clock_glitch_tamper_stat),
	CIT_CRMU_MCU_SECURITY_STAT = offsetof(struct dmu_regs,
					      crmu_mcu_security_stat),
	CIT_CRMU_SOFT_RESET_CTRL = offsetof(struct dmu_regs,
					    crmu_soft_reset_ctrl),
	CIT_CRMU_MASTER_AXI_CONFIG = offsetof(struct dmu_regs,
					      crmu_master_axi_config),
	CIT_CRMU_SPL_RESET_CONTROL = offsetof(struct dmu_regs,
					      crmu_spl_reset_control),
	CIT_CRMU_USBPHY_WAKE_CONTROL = offsetof(struct dmu_regs,
						crmu_usbphy_wake_control),
	CIT_USBPHY_RESUME_EVENT_LOG = offsetof(struct dmu_regs,
					       usbphy_resume_event_log),
	CIT_CRMU_SPL_EVENT_LOG = offsetof(struct dmu_regs, crmu_spl_event_log),
	CIT_CGM_CNT_CNTL_25M = offsetof(struct dmu_regs, cgm_cnt_cntl_25m),
	CIT_CGM_DLY_CNTL_25M = offsetof(struct dmu_regs, cgm_dly_cntl_25m),
	CIT_CGM_CNT_THRES_25M = offsetof(struct dmu_regs, cgm_cnt_thres_25m),
	CIT_CGM_CNT_CNTL_32K = offsetof(struct dmu_regs, cgm_cnt_cntl_32k),
	CIT_CGM_DLY_CNTL_32K = offsetof(struct dmu_regs, cgm_dly_cntl_32k),
	CIT_CGM_CNT_THRES_32K = offsetof(struct dmu_regs, cgm_cnt_thres_32k),
	CIT_CRMU_RAM_ECC_STATUS = offsetof(struct dmu_regs,
					   crmu_ram_ecc_status),
	CIT_CRMU_MCU_ACCESS_CONTROL = offsetof(struct dmu_regs,
					       crmu_mcu_access_control),
	CIT_GLOBAL_LOCK_OUT_CONTROL = offsetof(struct dmu_regs,
					       global_lock_out_control),
	CIT_BBRAM_LOCK_OUT_CONTROL = offsetof(struct dmu_regs,
					      bbram_lock_out_control),
	CIT_IPROC_LOCK_OUT_CONTROL = offsetof(struct dmu_regs,
					      iproc_lock_out_control),
	CIT_CRMU_IHOST_POR_WAKEUP_FLAG = offsetof(struct dmu_regs,
						  crmu_ihost_por_wakeup_flag),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG0 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg0),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG1 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg1),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG2 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg2),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG3 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg3),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG4 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg4),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG5 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg5),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG6 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg6),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG7 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg7),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG8 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg8),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG9 = offsetof(struct dmu_regs,
					crmu_ihost_sw_persistent_reg9),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG10 = offsetof(struct dmu_regs,
					 crmu_ihost_sw_persistent_reg10),
	CIT_CRMU_IHOST_SW_PERSISTENT_REG11 = offsetof(struct dmu_regs,
					 crmu_ihost_sw_persistent_reg11),
	CIT_CRMU_BBL_AUTH_CODE = offsetof(struct dmu_regs, crmu_bbl_auth_code),
	CIT_CRMU_BBL_AUTH_CHECK = offsetof(struct dmu_regs,
					   crmu_bbl_auth_check),
	CIT_CRMU_CONFIG_ECC_RTIC_ENABLE = offsetof(struct dmu_regs,
					crmu_config_ecc_rtic_enable),
	CIT_CRMU_BBL_CLEAR_ENABLE = offsetof(struct dmu_regs,
					     crmu_bbl_clear_enable),
	CIT_CRMU_SOTP_NEUTRALIZE_ENABLE = offsetof(struct dmu_regs,
					crmu_sotp_neutralize_enable),
	CIT_SMBUS_ALERT_PROGRAM = offsetof(struct dmu_regs,
					   smbus_alert_program),
	CIT_SMBUS_ALERT_CONFIG = offsetof(struct dmu_regs, smbus_alert_config),
};

/* Chip Dru registers enum*/
enum dmu_cdru_regs {
	CIT_CRMU_PLL_ISO_CTRL = offsetof(struct cdru_regs, crmu_pll_iso_ctrl),
	CIT_CRMU_KEYPAD_CLK_DIV = offsetof(struct cdru_regs,
					   crmu_keypad_clk_div),
	CIT_CRMU_PWM_CLK_DIV = offsetof(struct cdru_regs, crmu_pwm_clk_div),
	CIT_CRMU_ADC_CLK_DIV = offsetof(struct cdru_regs, crmu_adc_clk_div),
	CIT_SYS_CLK_SOURCE_SEL_CTRL = offsetof(struct cdru_regs,
					       sys_clk_source_sel_ctrl),
	CIT_SYS_CLK_SWITCH_READY_STAT = offsetof(struct cdru_regs,
						 sys_clk_switch_ready_stat),
	CIT_CRMU_SW_POR_RESET_CTRL = offsetof(struct cdru_regs,
					      crmu_sw_por_reset_ctrl),
	CIT_CRMU_RESET_CTRL = offsetof(struct cdru_regs, crmu_reset_ctrl),
	CIT_CRMU_CHIP_POR_CTRL = offsetof(struct cdru_regs, crmu_chip_por_ctrl),
	CIT_CRMU_CHIP_STRAP_CTRL = offsetof(struct cdru_regs,
					    crmu_chip_strap_ctrl),
	CIT_CRMU_CHIP_STRAP_CTRL2 = offsetof(struct cdru_regs,
					     crmu_chip_strap_ctrl2),
	CIT_CRMU_CHIP_IO_PAD_CONTROL = offsetof(struct cdru_regs,
						crmu_chip_io_pad_control),
	CIT_CRMU_CHIP_STRAP_DATA = offsetof(struct cdru_regs,
					    crmu_chip_strap_data),
	CIT_CRMU_CHIP_DEBUG_TEST_STRAP = offsetof(struct cdru_regs,
						  crmu_chip_debug_test_strap),
	CIT_SOTP_READY_STATUS = offsetof(struct cdru_regs, sotp_ready_status),
	CIT_SYS_PADRING_STATUS = offsetof(struct cdru_regs, sys_padring_status),
	CIT_CRMU_SOTP_STATUS = offsetof(struct cdru_regs, crmu_sotp_status),
	CIT_CDRU_ROM_CTRL1 = offsetof(struct cdru_regs, cdru_rom_ctrl1),
	CIT_CDRU_ROM_STAT = offsetof(struct cdru_regs, cdru_rom_stat),
	CIT_CDRU_SRAM_CTRL1 = offsetof(struct cdru_regs, cdru_sram_ctrl1),
	CIT_CDRU_ARM_M3_CTRL = offsetof(struct cdru_regs, cdru_arm_m3_ctrl),
	CIT_CDRU_IDM_TIMEOUT = offsetof(struct cdru_regs, cdru_idm_timeout),
	CIT_CDRU_ARM_M3_STATUS = offsetof(struct cdru_regs, cdru_arm_m3_status),
	CIT_CDRU_QSPI_CTRL = offsetof(struct cdru_regs, cdru_qspi_ctrl),
	CIT_CDRU_QSPI_STATUS = offsetof(struct cdru_regs, cdru_qspi_status),
	CIT_CDRU_CLK_DIS_CTRL = offsetof(struct cdru_regs, cdru_clk_dis_ctrl),
	CIT_CDRU_SW_RESET_CTRL = offsetof(struct cdru_regs, cdru_sw_reset_ctrl),
	CIT_CDRU_UART_MISC = offsetof(struct cdru_regs, cdru_uart_misc),
	CIT_CDRU_SCI_MISC = offsetof(struct cdru_regs, cdru_sci_misc),
	CIT_CDRU_USBD_IO_CTRL = offsetof(struct cdru_regs, cdru_usbd_io_ctrl),
	CIT_CDRU_USBD_MISC = offsetof(struct cdru_regs, cdru_usbd_misc),
	CIT_CDRU_USBD_STATUS0 = offsetof(struct cdru_regs, cdru_usbd_status0),
	CIT_CDRU_USBH_IO_CTRL = offsetof(struct cdru_regs, cdru_usbh_io_ctrl),
	CIT_CDRU_USBH_MISC = offsetof(struct cdru_regs, cdru_usbh_misc),
	CIT_CDRU_USBH_STATUS0 = offsetof(struct cdru_regs, cdru_usbh_status0),
	CIT_CDRU_USBPHY_H_CTRL1 = offsetof(struct cdru_regs,
							 cdru_usbphy_h_ctrl1),
	CIT_CDRU_USBPHY_H_STATUS = offsetof(struct cdru_regs,
					    cdru_usbphy_h_status),
	CIT_CDRU_USBPHY_H_P1CTRL = offsetof(struct cdru_regs,
					    cdru_usbphy_h_p1ctrl),
	CIT_CDRU_USBPHY_H_CTRL2 = offsetof(struct cdru_regs,
					   cdru_usbphy_h_ctrl2),
	CIT_CDRU_USBPHY_D_CTRL1 = offsetof(struct cdru_regs,
					   cdru_usbphy_d_ctrl1),
	CIT_CDRU_USBPHY_D_STATUS = offsetof(struct cdru_regs,
					    cdru_usbphy_d_status),
	CIT_CDRU_USBPHY_D_P1CTRL = offsetof(struct cdru_regs,
					    cdru_usbphy_d_p1ctrl),
	CIT_CDRU_USBPHY_D_CTRL2 = offsetof(struct cdru_regs,
					   cdru_usbphy_d_ctrl2),
	CIT_CDRU_SMC_ADDRESS_MATCH_MASK = offsetof(struct cdru_regs,
						   cdru_smc_address_match_mask),
};

/*  DRU Registers enum*/
enum dmu_dru_reg {
	CIT_CRMU_XTAL_CHANNEL_CONTROL1 = offsetof(struct dru_regs,
						  crmu_xtal_channel_control1),
	CIT_CRMU_XTAL_CHANNEL_CONTROL2 = offsetof(struct dru_regs,
						  crmu_xtal_channel_control2),
	CIT_CRMU_LDO_CTRL = offsetof(struct dru_regs, crmu_ldo_ctrl),
	CIT_CRMU_MAIN_LDO_PU_CTRL = offsetof(struct dru_regs,
					     crmu_main_ldo_pu_ctrl),
	CIT_CRMU_ADC_LDO_PU_CTRL = offsetof(struct dru_regs,
					    crmu_adc_ldo_pu_ctrl),
	CIT_CRMU_LDO_CTRL_REG_28_0 = offsetof(struct dru_regs,
					      crmu_ldo_ctrl_reg_28_0),
	CIT_CRMU_LDO_CTRL_REG_39_31 = offsetof(struct dru_regs,
					       crmu_ldo_ctrl_reg_39_31),
	CIT_CRMU_SWREG_CTRL = offsetof(struct dru_regs, crmu_swreg_ctrl),
	CIT_CRMU_PLL_AON_CTRL = offsetof(struct dru_regs, crmu_pll_aon_ctrl),
	CIT_CRMU_USBPHY_D_CTRL = offsetof(struct dru_regs, crmu_usbphy_d_ctrl),
	CIT_CRMU_USBPHY_H_CTRL = offsetof(struct dru_regs, crmu_usbphy_h_ctrl),
	CIT_CRMU_USBPHY_D_STATUS = offsetof(struct dru_regs,
					    crmu_usbphy_d_status),
	CIT_CRMU_USBPHY_H_STATUS = offsetof(struct dru_regs,
					    crmu_usbphy_h_status),
	CIT_CRMU_PWR_GOOD_STATUS = offsetof(struct dru_regs,
					    crmu_pwr_good_status),
	CIT_ADC_ISO_CONTROL = offsetof(struct dru_regs, adc_iso_control),
	CIT_CRMU_POWER_REQ_CFG = offsetof(struct dru_regs, crmu_power_req_cfg),
	CIT_CRMU_POWER_POLL = offsetof(struct dru_regs, crmu_power_poll),
	CIT_CRMU_ISO_CELL_CONTROL = offsetof(struct dru_regs,
					     crmu_iso_cell_control),
	CIT_CRMU_CDRU_APB_RESET_CTRL = offsetof(struct dru_regs,
						crmu_cdru_apb_reset_ctrl),
	CIT_CRMU_SPRU_SOURCE_SEL_STAT = offsetof(struct dru_regs,
						 crmu_spru_source_sel_stat),
	CIT_CRMU_CLOCK_GATE_CONTROL = offsetof(struct dru_regs,
					       crmu_clock_gate_control),
	CIT_CRMU_SPL_CLK_SWITCH_TIMEOUT = offsetof(struct dru_regs,
						   crmu_spl_clk_switch_timeout),
	CIT_CRMU_ULTRA_LOW_POWER_CTRL = offsetof(struct dru_regs,
						 crmu_ultra_low_power_ctrl),
	CIT_CRMU_CLOCK_SWITCH_STATUS = offsetof(struct dru_regs,
						crmu_clock_switch_status),
	CIT_CRMU_XTAL_POWER_DOWN = offsetof(struct dru_regs,
					    crmu_xtal_power_down),
	CIT_CRMU_CHIP_OTPC_RST_CNTRL = offsetof(struct dru_regs,
						crmu_chip_otpc_rst_cntrl),
	CIT_CRMU_CHIP_OTPC_STATUS = offsetof(struct dru_regs,
					     crmu_chip_otpc_status),
	CIT_OTP_FEATURE_DISABLE_CTRL0 = offsetof(struct dru_regs,
						 otp_feature_disable_ctrl0),
	CIT_OTP_FEATURE_DISABLE_CTRL1 = offsetof(struct dru_regs,
						 otp_feature_disable_ctrl1),
	CIT_OTP_OVERRIDE_CODE = offsetof(struct dru_regs, otp_override_code),
	CIT_OTP_OVERRIDE_VALID = offsetof(struct dru_regs, otp_override_valid),
	CIT_OTP_OVERRIDE_DATA0 = offsetof(struct dru_regs, otp_override_data0),
	CIT_OTP_OVERRIDE_DATA1 = offsetof(struct dru_regs, otp_override_data1),
	CIT_OTP_OVERRIDE_DATA2 = offsetof(struct dru_regs, otp_override_data2),
	CIT_OTP_OVERRIDE_DATA3 = offsetof(struct dru_regs, otp_override_data3),
	CIT_OTP_OVERRIDE_DATA6 = offsetof(struct dru_regs, otp_override_data6),
	CIT_OTP_OVERRIDE_DATA7 = offsetof(struct dru_regs, otp_override_data7),
	CIT_OTP_LVM_OVERRIDE_SELECT = offsetof(struct dru_regs,
					       otp_lvm_override_select),
	CIT_CRMU_ID_SCR_MEM_COFIG = offsetof(struct dru_regs,
					     crmu_id_scr_mem_cofig),
	CIT_CRMU_BISR_PDG_MASK = offsetof(struct dru_regs, crmu_bisr_pdg_mask),
	CIT_CRMU_AON_GPIO_OTPC_CONFIG = offsetof(struct dru_regs,
						 crmu_bisr_pdg_mask)
};

enum dmu_block {
	DMU_SYSTEM = 0,
	DMU_DR_UNIT,
	DMU_SP_LOGIC
};

enum dmu_action {
	DMU_DEVICE_RESET = 0,
	DMU_DEVICE_SHUTDOWN,
	DMU_DEVICE_BRINGUP
};

enum dmu_device {
	DMU_ADC = 0,
	DMU_SOFT_PWR = 0,
	DMU_SPL = 0,

	DMU_PKA = 1,
	DMU_SOFT_SYS = 1,

	DMU_TRNG = 2,
	DMU_GPIO = 3,
	DMU_SCI = 4,
	DMU_TIMER1 = 5,
	DMU_TIMER0 = 6,
	DMU_MDIO = 7,
	DMU_PWM = 8,
	DMU_FSK = 9,
	DMU_UART3 = 10,
	DMU_UART2 = 11,
	DMU_UART1 = 12,
	DMU_UART0 = 13,
	DMU_SPI5 = 14,
	DMU_SPI4 = 15,
	DMU_SPI3 = 16,
	DMU_SPI2 = 17,
	DMU_SPI1 = 18,
	DMU_QSPI = 19,
	DMU_ROM = 20,
	DMU_SRAM = 21,
	DMU_SMC = 22,
	DMU_USBH = 23,
	DMU_USBD = 24,
	DMU_DMA = 25,
	DMU_SMAU = 26,
	DMU_IDC = 27,
	DMU_CM3 = 28,
	DMU_WDT = 29,
	DMU_SEU = 30
};

enum iproc_intr {
	MAILBOX,
	AXI_DEC_ERR,
	SPL_WDOG,
	SP_LOGIC_RST,
	SP_LOGIC_PVT,
	SP_LOGIC_FREQ,
	SP_RU_ALARM,
	SP_RU_RTC,
	SPRU_RTC_PERIODIC,
	M0_LOCKUP,
	IHOST_CLK_GLITCH_TAMPER,
	BBL_CLK_GLITCH_TAMPER,
	CRMU_CLK_GLITCH_TAMPER,
	CRMU_FID_TAMPER,
	CHIP_FID_TAMPER,
	RTIC_TAMPER,
	CRMU_ECC_TAMPER,
	VOLTAGE_GLITCH_TAMPER,
	USBPHY0_WAKE,
	USBPHY1_WAKE
};

enum crmu_event {
	USBPHY0_RESUME_WAKE = 12,
	USBPHY0_FILTER_WAKE,
	USBPHY1_RESUME_WAKE,
	USBPHY1_FILTER_WAKE,
	MPROC_CRMU_EVENT0,
	MPROC_CRMU_EVENT1,
	MPROC_CRMU_EVENT2,
	MPROC_CRMU_EVENT3,
	MPROC_CRMU_EVENT4,
	MPROC_CRMU_EVENT5,
	SPL_WDOG_EVENT,
	SPL_RST,
	SPL_PVT,
	SPL_FREQ,
	SPRU_ALARM,
	SPRU_RTC,
	IPROC_STANDBYWFE,
	IPROC_STANDBYWFI,
	MAILBOX_EVENT
};

typedef s32_t (*dmu_api_device_ctrl)(struct device *dev, enum dmu_block blk,
				    enum dmu_device device,
				    enum dmu_action action);

typedef void (*dmu_api_write)(struct device *dev, u32_t reg, u32_t data);

typedef u32_t (*dmu_api_read)(struct device *dev, u32_t reg);

typedef void (*crmu_api_mcu_event_mask)(struct device *dev,
					 enum crmu_event id);

typedef void (*crmu_api_mcu_event_unmask)(struct device *dev,
					   enum crmu_event id);

typedef void (*crmu_api_mcu_event_clear)(struct device *dev,
					  enum crmu_event id);

typedef s32_t (*crmu_api_mcu_event_status_get)(struct device *dev,
					       enum crmu_event id);

typedef void (*crmu_api_iproc_intr_mask_set)(struct device *dev,
					      enum iproc_intr id);

typedef void (*crmu_api_iproc_intr_unmask)(struct device *dev,
					    enum iproc_intr id);

typedef void (*crmu_api_iproc_intr_clear)(struct device *dev,
					   enum iproc_intr id);

typedef s32_t (*crmu_api_iproc_intr_status_get)(struct device *dev,
					  enum iproc_intr id);

struct dmu_driver_api {
	dmu_api_write write;
	dmu_api_read read;
	dmu_api_device_ctrl device_ctrl;
	crmu_api_mcu_event_mask mcu_event_mask;
	crmu_api_mcu_event_unmask mcu_event_unmask;
	crmu_api_mcu_event_clear mcu_event_clear;
	crmu_api_mcu_event_status_get mcu_event_status_get;
	crmu_api_iproc_intr_mask_set iproc_intr_mask_set;
	crmu_api_iproc_intr_unmask iproc_intr_unmask;
	crmu_api_iproc_intr_clear iproc_intr_clear;
	crmu_api_iproc_intr_status_get iproc_intr_status_get;
};

/**
 * @brief Control the DMU device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to write
 * @param data - Data to write
 *
 * @return None
 *
 * @note The dev should be corresponding device dmu/cdru/dru.
 *       Need to check the register belogs to which device.
 *       For register field, use macros provided in dmu.h
 */
static inline void dmu_write(struct device *dev, u32_t reg, u32_t data)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->write(dev, reg, data);
}

/**
 * @brief Control the DMU device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg - register to read
 *
 * @return Data from the register
 *
 * @note The dev should be corresponding device dmu/cdru/dru.
 *       Need to check the register belogs to which device.
 *       For register field, use macros provided in dmu.h
 */
static inline s32_t dmu_read(struct device *dev, u32_t reg)
{
	const struct dmu_driver_api *api = dev->driver_api;

	return api->read(dev, reg);
}

/**
 * @brief Control the DMU device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param blk - DMU block
 * @param device - Device to control
 * @param action - Action for the device: reset, shut down or up
 *
 * @return 0 for success, error otherwise
 */
static inline s32_t dmu_device_ctrl(struct device *dev, enum dmu_block blk,
				    enum dmu_device device,
				    enum dmu_action action)
{
	const struct dmu_driver_api *api = dev->driver_api;

	return api->device_ctrl(dev, blk, device, action);
}

/**
 * @brief Mask the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static inline void crmu_iproc_intr_mask_set(struct device *dev,
					    enum iproc_intr id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->mcu_event_mask(dev, id);
}

/**
 * @brief Unmask the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static inline void crmu_iproc_intr_unmask(struct device *dev,
					  enum iproc_intr id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->mcu_event_unmask(dev, id);
}

/**
 * @brief Get the iproc interrupt status
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return status
 */
static inline s32_t crmu_iproc_intr_status_get(struct device *dev,
					       enum iproc_intr id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	return api->iproc_intr_status_get(dev, id);
}

/**
 * @brief clear the iproc interrupt
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Interrupt id
 *
 * @return none
 */
static inline void crmu_iproc_intr_clear(struct device *dev,
					 enum iproc_intr id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->iproc_intr_clear(dev, id);
}

/**
 * @brief Mask the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static inline void crmu_event_mask(struct device *dev, enum crmu_event id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->mcu_event_unmask(dev, id);
}

/**
 * @brief Unmask the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static inline void crmu_event_unmask(struct device *dev, enum crmu_event id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->mcu_event_unmask(dev, id);
}

/**
 * @brief Get the crmu event status
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return status
 */
static inline s32_t crmu_event_status_get(struct device *dev,
					  enum crmu_event id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	return api->mcu_event_status_get(dev, id);
}

/**
 * @brief clear the crmu event
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id Event id
 *
 * @return none
 */
static inline void crmu_event_clear(struct device *dev, enum crmu_event id)
{
	const struct dmu_driver_api *api = dev->driver_api;

	api->mcu_event_clear(dev, id);
}

#ifdef __cplusplus
}
#endif

#endif /* _DMU_H_ */
