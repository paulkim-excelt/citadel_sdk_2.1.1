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

const u32_t *m0_isr_vector[33] __attribute__ ((section (".start_vector"))) =
{
/*---------------- 0 ~   7 --------------*/
NULL,
NULL,
(void *)m0_aon_gpio_handler, /* 2. AON GPIO Interrupt */
NULL,
NULL,
NULL,
(void *)m0_smbus_handler, /* 6. SMBUS interrupt Handler */
NULL, /* 7. MCU Timer Interrupt        */
/*--------------- 8 ~ 15 ---------------*/
NULL,
NULL,
NULL,
NULL,
NULL, /* 12. MCU_NVIC_VOLTAGE_GLITCH_INTR */
NULL, /* 13. MCU_NVIC_CLK_GLITCH_INTR */
NULL,
(void *)m0_usb_handler, /* 15. USBPHY_WAKE_EVENT */
/*-------------  16 ~ 23  ---------------*/
NULL, /* 16. CRMU_EVENT0 - NFC */
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
/*-------------- 24 ~ 25  ---------------*/
NULL,
NULL,
(void *)m0_tamper_handler, /* 26. SPRU Alarm Tamper Event */
(void *)m0_rtc_handler, /* 27. RTC Event */
(void *)m0_rtc_handler, /* 28. RTC Periodic Event */
NULL,
(void *)m0_wfi_handler, /* 30. StandByWFI event from A7 to MCU */
(void *)user_mbox_handler, /* 31. Mailbox interrupt  */
#ifdef MPROC_PM_FASTXIP_SUPPORT
(void *)user_fastxip_workaround /* 32. FastXIP workaround */
#else
NULL /* 32. NULL */
#endif
};

/*****************************************************************************
 * P U B L I C   U T I L I T Y   A P I s
 *****************************************************************************/
/**
 * @brief MCU_SYNC
 *
 * @param none
 *
 * @return N/A
 */
#ifdef M0TOOL_ARMCC
__asm void MCU_SYNC(void)
{
	DSB
	ISB
}
#else /* M0TOOL_GCC */
void MCU_SYNC(void)
{
	asm volatile("dsb");
	asm volatile("isb");
}
#endif

/**
 * @brief mcu_getbit
 *
 * @param data
 * @param bitno
 *
 * @return bit value
 */
u32_t mcu_getbit(u32_t data, u32_t bitno)
{
	return ((data >> bitno) & 0x1);
}

/**
 * @brief mcu_setbit
 *
 * @param data
 * @param bitno
 *
 * @return result
 */
u32_t mcu_setbit(u32_t data, u32_t bitno)
{
	return (data |= BIT(bitno));
}

/**
 * @brief mcu_clrbit
 *
 * @param data
 * @param bitno
 *
 * @return result
 */
u32_t mcu_clrbit(u32_t data, u32_t bitno)
{
	return (data &= (~BIT(bitno)));
}

/**
 * @brief cpu_reg32_set_masked_bits
 * set specified bits to 1, other bits unchange
 *
 * @param addr
 * @param mask
 *
 * @return none
 */
void cpu_reg32_set_masked_bits(u32_t addr, u32_t mask)
{
	u32_t regval = sys_read32(addr);

	regval |= mask;

	sys_write32(regval, addr);
}

/**
 * @brief cpu_reg32_clr_masked_bits
 * Clear specified bits to 0, other bits unchange
 *
 * @param addr
 * @param mask
 *
 * @return none
 */
void cpu_reg32_clr_masked_bits(u32_t addr, u32_t mask)
{
	u32_t regval = sys_read32(addr);

	regval &= ~mask;

	sys_write32(regval, addr);
}

/**
 * @brief cpu_reg32_wr_masked_bits
 * Write specified bits to target val, other bits unchange
 *
 * @param addr
 * @param mask
 * @param val
 *
 * @return none
 */
void cpu_reg32_wr_masked_bits(u32_t addr, u32_t mask, u32_t val)
{
	u32_t regval = sys_read32(addr);

	regval &= ~mask;
	regval |= (val & mask);

	sys_write32(regval, addr);
}

/**
 * @brief cpu_reg32_wr_mask
 * Write specified bits to target val, other bits unchange
 *
 * @param addr
 * @param rightbit
 * @param len
 * @param value
 *
 * @return none
 */
void cpu_reg32_wr_mask(u32_t addr, u32_t rightbit, u32_t len, u32_t value)
{
	u32_t mask       = BIT(len) - 1;
	u32_t mask_shift = mask << rightbit;
	u32_t curvalue = sys_read32(addr);
	u32_t tmpvalue = curvalue & (~mask_shift);
	u32_t newvalue = tmpvalue | ((value & mask) << rightbit);

	sys_write32(newvalue, addr);
}

/**
 * @brief MCU_memset
 *
 * @param address
 * @param fill character
 * @param count
 *
 * @return destination address
 */
void *MCU_memset(void *s, s32_t c, size_t count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;

	return s;
}

/**
 * @brief MCU_memcpy
 *
 * @param dest
 * @param source
 * @param count
 *
 * @return destination address
 */
void *MCU_memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

/**
 * @brief MCU_delay
 *
 * TODO: Find out what 200000 stand for and create a #define.
 * TODO: Find out what the constant 9 mean and create a #define.
 *
 * @param count
 *
 * @return none
 */
void MCU_delay(u32_t cnt)
{
	volatile u32_t count = cnt ? cnt : 200000;

#ifdef MPROC_PM__ULTRA_LOW_POWER
	/* if clock source is spl or bbl instead of xtal, then set delay
	 * count to a much smaller value.
	 */
	if (sys_read32(CRMU_CLOCK_SWITCH_STATUS) != 1)
		/* precise divider should be 25000000 / 32768 = 762 */
		count = (count >> 9);
#endif

	do {} while (count--);
}

/**
 * @brief MCU_die
 *
 * @param none
 *
 * @return none
 */
void MCU_die(void)
{
	do {} while (1);
}

/**
 * @brief MCU_get_timer_count
 *
 * TODO: Find out what the constants mean and create #define.
 * The 25MHz need to changed to 26MHz. Confirm from HW.
 *
 * get the timer count for specified micro-seconds
 * counts = us * 1000 / unit_in_ns
 * @param us
 *
 * @return result
 */
u32_t MCU_get_timer_count(u32_t us)
{
	u32_t clk_src = sys_read32(CRMU_CLOCK_SWITCH_STATUS);
	u32_t counts = 0;

	if (clk_src == 0x1) /* 25MHz xtal, unit is 40ns */
		counts = us * 26;
	/* TODO: Is this valid for Citadel? */
	else if (clk_src == 0x2) /* 11.5MHz spl ro, unit is 87ns */
		counts = us * 11;
	else if (clk_src == 0x4) /* 32KHz bbl, unit is 31.25us, use 32us */
		counts = (us >> 5);

	if (!counts)
		counts = 1;

	return counts;
}

/**
 * @brief MCU_timer_udelay
 *
 * 1. use crmu_timer2 to delay as SBL is using crmu_timer1
 * 2. when m0 clock source is 25MHz XTAL: time unit is 40ns,
 *    so real max delay is about 171s
 * 3. when m0 clock source is 11.5MHz SPL RO: time unit is 87ns,
 *    so real max delay is about 371s
 * 4. when m0 clock source is 32KHz BBL: time unit is 31.25us,
 *    so real max delay is about 134217s
 *
 * @param us
 *
 * @return none
 */
void MCU_timer_udelay(u32_t us)
{
	u32_t counts = MCU_get_timer_count(us);

	sys_write32(counts, CRMU_TIM_TIMER2Load);
	/* Interrupt clear        */
	sys_write32(0x1, CRMU_TIM_TIMER2IntClr);

	/* Only enable crmu timer */
	sys_write32(0x83, CRMU_TIM_TIMER2Control);

	while (!sys_read32(CRMU_TIM_TIMER2RIS))
		;   /* Wait for timeout       */

	sys_write32(0x1, CRMU_TIM_TIMER2IntClr); /* Interrupt clear */
	sys_write32(0xFFFFFFFF, CRMU_TIM_TIMER2Load);
	sys_write32(0x0, CRMU_TIM_TIMER2Control); /* Disable timer */
}

void MCU_timer_raw_delay(u32_t counts)
{
	sys_write32(counts, CRMU_TIM_TIMER2Load);
	/* Interrupt clear        */
	sys_write32(0x1, CRMU_TIM_TIMER2IntClr);
	/* Only enable crmu timer */
	sys_write32(0x83, CRMU_TIM_TIMER2Control);
	while (!sys_read32(CRMU_TIM_TIMER2RIS))
		;   /* Wait for timeout       */

	sys_write32(0x1, CRMU_TIM_TIMER2IntClr); /* Interrupt clear */
	sys_write32(0xFFFFFFFF, CRMU_TIM_TIMER2Load);
	sys_write32(0x0, CRMU_TIM_TIMER2Control); /* Disable timer */
}

#ifdef MPROC_PM_WAIT_FOR_MCU_JOB_DONE
u32_t MCU_get_mcu_job_progress(void)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	return pm_args->progress;
}

void MCU_set_mcu_job_progress(u32_t pro)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	pm_args->progress = pro;
}
#endif

#ifdef MPROC_PM_TRACK_MCU_STATE
void MCU_USER_ISR_SET_STATE(u32_t state)
{
	/* get log pointer */
	u32_t mcu_log_addr = sys_read32(MPROC_MCU_LOG_PTR);

	/* pointer not init */
	if (mcu_log_addr >= MPROC_MCU_LOG_END_ADDR
		|| mcu_log_addr < MPROC_MCU_LOG_START_ADDR)
		return;

	sys_write32(state, mcu_log_addr); /* log state */

	mcu_log_addr += 4;

	if (mcu_log_addr >= MPROC_MCU_LOG_END_ADDR)
		/* wrapper to start addr */
		mcu_log_addr = MPROC_MCU_LOG_START_ADDR;

	/* update log pointer */
	sys_write32(mcu_log_addr, MPROC_MCU_LOG_PTR);
}
#endif

CITADEL_REVISION_e MCU_get_soc_rev(void)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	return pm_args->soc_rev;
}

u32_t MCU_get_irq(DMU_MCU_IRQ_ID_e id)
{
	return (sys_read32(CRMU_MCU_INTR_MASK) & BIT(id));
}

void MCU_enable_irq(DMU_MCU_IRQ_ID_e id, s32_t en)
{
	if (en)
		/* unmasked */
		sys_clear_bit(CRMU_MCU_INTR_MASK, id);
	else
		/* masked*/
		sys_set_bit(CRMU_MCU_INTR_MASK, id);
}

s32_t MCU_check_irq(DMU_MCU_IRQ_ID_e id)
{
	return cpu_reg32_getbit(CRMU_MCU_INTR_STATUS, id);
}

void MCU_clear_irq(DMU_MCU_IRQ_ID_e id)
{
	/* ONLY write 1 to id bit to clear interrupt,
	 * other bits to 0, no effect
	 */
	sys_write32(BIT(id), CRMU_MCU_INTR_CLEAR);
}

u32_t MCU_get_event(DMU_MCU_EVENT_ID_e id)
{
	return (sys_read32(CRMU_MCU_EVENT_MASK) & BIT(id));
}

void MCU_enable_event(DMU_MCU_EVENT_ID_e id, s32_t en)
{
	if (en)
		/* unmasked */
		sys_clear_bit(CRMU_MCU_EVENT_MASK, id);
	else
		/* masked */
		sys_set_bit(CRMU_MCU_EVENT_MASK, id);
}

s32_t MCU_check_event(DMU_MCU_EVENT_ID_e id)
{
	return cpu_reg32_getbit(CRMU_MCU_EVENT_STATUS, id);
}

void MCU_clear_event(DMU_MCU_EVENT_ID_e id)
{
	/* ONLY write 1 to id bit to clear interrupt,
	 * other bits to 0, no effect
	 */
	sys_write32(BIT(id), CRMU_MCU_EVENT_CLEAR);
}

void MCU_send_msg_to_mproc(u32_t code0, u32_t code1)
{
	sys_write32(code0, CRMU_IPROC_MAIL_BOX0);
	sys_write32(code1, CRMU_IPROC_MAIL_BOX1);
}

void MCU_send_msg_to_mcu(u32_t code0, u32_t code1)
{
	sys_write32(code0, IPROC_CRMU_MAIL_BOX0);
	sys_write32(code1, IPROC_CRMU_MAIL_BOX1);
}

/*****************************************************************************
 * P U B L I C   A P I s
 *****************************************************************************/
/**
 * @brief MCU_set_irq_user_vector
 *
 * Description : Program the location
 * of the ISR into the M0 IRQ vector.
 *
 * @param irq
 * @param addr
 *
 * @return none
 */
void MCU_set_irq_user_vector(u32_t irq, u32_t addr)
{
	if (irq < MCU_NVIC_MAXIRQs)
		sys_write32(CRMU_IRQx_vector_USER(irq), addr);
}

/**
 * @brief MCU_set_pdsys_master_clock_gating
 *
 * Description :
 *
 *
 * @param en - 1: enabled 0: disabled
 *
 * @return none
 */
void MCU_set_pdsys_master_clock_gating(s32_t en)
{
	if (en) {
		/*disable master clock for PDSYS clocks, except BBL and MCU self*/
		sys_write32(0x3f, CRMU_CLOCK_GATE_CONTROL);
		/*gate most MPROC clocks, don't gate wakeup sources, i.e. gpio*/
		sys_write32(0xFFFFFFF7, CDRU_CLK_DIS_CTRL);
	} else {
		/*enable master clock for PDSYS clocks*/
		sys_write32(0xffffffff, CRMU_CLOCK_GATE_CONTROL);
		/*enable MPROC clocks clock*/
		sys_write32(0x0, CDRU_CLK_DIS_CTRL);
	}
}

/**
 * @brief MCU_setup_aongpio
 *
 * Description:
 *
 *
 * @param en - 1: enabled 0: disabled
 *
 * @return none
 */
void MCU_setup_aongpio(MPROC_LOW_POWER_OP_e op)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();
	MPROC_AON_GPIO_WAKEUP_SETTINGS_s *aon_gpio_setting;
	u32_t wakeup_src = 0, int_type = 0, int_de = 0, int_edge = 0;

	aon_gpio_setting = &pm_args->aon_gpio_settings;
	if (op == MPROC_ENTER_LP) { /*Set AON_GPIO interrupt trigger mode*/
		wakeup_src = aon_gpio_setting->wakeup_src;
		int_type   = aon_gpio_setting->int_type;
		int_de	   = aon_gpio_setting->int_de;
		int_edge   = aon_gpio_setting->int_edge;

		/* Program the AON GPIOs only if the AON GPIO
		 * wakeup source is non-empty
		 */
		if (wakeup_src)
			sys_set_bit(CRMU_IOMUX_CONTROL,
			CRMU_IOMUX_CONTROL__CRMU_OVERRIDE_UART_RX);
		/* FIXME: Are the following not supposed to be
		 * done only if wakeup source is non-empty?
		 *
		 * Is this kept out for non-wakeup source AONGPIOs?
		 */
		/* Mask Temporarily */
		MCU_enable_irq(MCU_AON_GPIO_INTR, 0);
		/*1=level, 0=edge*/
		cpu_reg32_wr_masked_bits(GP_INT_TYPE, wakeup_src, int_type);
		/*1=both edge, 0=single edge*/
		cpu_reg32_wr_masked_bits(GP_INT_DE, wakeup_src, int_de);
		/*1=rising edge or a high level, 0=falling edge or low level*/
		cpu_reg32_wr_masked_bits(GP_INT_EDGE, wakeup_src, int_edge);

		/*1=pull up, 0=pull down*/
		/* cpu_reg32_set_masked_bits(GP_PAD_RES, wakeup_src); */
		/*1=enable pull up/down, 0=disable pull up/down*/
		/* cpu_reg32_set_masked_bits(GP_RES_EN,  wakeup_src); */

		/*1=output, 0=input*/
		cpu_reg32_clr_masked_bits(GP_OUT_EN, wakeup_src);
		/* cpu_reg32_wr(GP_INT_CLR, 0x3F); */
		/*1=unmask, 0=mask*/
		cpu_reg32_set_masked_bits(GP_INT_MSK, wakeup_src);

		MCU_SYNC();
		/* TODO: 11 AON GPIO interrupts possible in Citadel */
		sys_write32(AON_GPIO_INT_MASK, GP_INT_CLR);
		/* UnMask Finally */
		MCU_enable_irq(MCU_AON_GPIO_INTR, 1);
	}
	else {
		/* FIXME: Shouldn't the programming above be undone?
		 * Confirm requirements.
		 */
	}
}

/**
 * @brief MCU_setup_usb
 *
 * Description:
 *
 *
 * @param en - 1: enabled 0: disabled
 *
 * @return none
 */
void MCU_setup_usb(MPROC_LOW_POWER_OP_e op)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();
	MPROC_USB_WAKEUP_SETTINGS_s *usb_setting;
	u32_t wakeup_src = 0;

	usb_setting = &pm_args->usb_settings;
	if (op == MPROC_ENTER_LP) { /*Set AON_GPIO interrupt trigger mode*/
		wakeup_src = usb_setting->wakeup_src;

		/* Enable wake-up on USB only if the USB
		 * wakeup source is non-empty
		 */
		if (wakeup_src) {
			/* UnMask USB */
			/**< USBPHY0 Wake event */
			MCU_enable_event(MCU_USBPHY0_WAKE_EVENT, 1);
			/**< USBPHY0 Filter match event */
			MCU_enable_event(MCU_USBPHY0_FILTER_EVENT, 1);
			/**< USBPHY1 Wake event */
			MCU_enable_event(MCU_USBPHY1_WAKE_EVENT, 1);
			/**< USBPHY1 Filter1 match event */
			MCU_enable_event(MCU_USBPHY1_FILTER_EVENT, 1);
			MCU_SYNC();
		}
	}
	else {
		/* FIXME: Shouldn't the programming above be undone?
		 * Confirm requirements.
		 */
	}
}

#ifdef MPROC_PM_ACCESS_BBL
/**
 * @brief SPRU Write Data
 * @details register Write procedure, write wrDat to register SPRU_BBL_WDATA
 *
 * @param data
 *
 * @return None
 */
static void spru_wr_data(u32_t data)
{
	sys_write32(data, SPRU_BBL_WDATA);
}

/**
 * @brief SPRU Write Command
 * @details insert write command to SPRU_BBL_CMD
 *
 * @param address
 *
 * @return None
 */
static void spru_wr_cmd(u32_t addr)
{
	sys_write32(((1 << SPRU_BBL_CMD__IND_SOFT_RST_N)
				| (1 << SPRU_BBL_CMD__IND_WR)
				| (addr << SPRU_BBL_CMD__BBL_ADDR_R)),
				SPRU_BBL_CMD);
}

/**
 * @brief SPRU Read Data
 * @details register read procedure, read data from SPRU_BBL_RDATA
 *
 * @param None
 *
 * @return Reg value
 */
static void spru_rd_data(u32_t *data)
{
	*data = sys_read32(SPRU_BBL_RDATA);
}

/**
 * @brief insert read command to SPRU_BBL_CMD
 * @details insert read command to SPRU_BBL_CMD
 *
 * @param address
 *
 * @return None
 */
static void spru_rd_cmd(u32_t addr)
{
	sys_write32(((1 << SPRU_BBL_CMD__IND_SOFT_RST_N)
				| (1 << SPRU_BBL_CMD__IND_RD)
				| (addr << SPRU_BBL_CMD__BBL_ADDR_R)),
				SPRU_BBL_CMD);
}

/**
 * @brief MCU_bbl_access_ok
 *
 * Description:
 * 1: issue soft reset
 * 2: Wait for few 32KHz cycles
 * 3: Release the reset
 * 4: Wait for few 32 KHz cycles.
 *
 * @param none
 *
 * @return none
 */
void spru_interface_reset(void)
{
	sys_write32(0x0, SPRU_BBL_CMD); /* reset */
	MCU_timer_udelay(320);
	/* release reset */
	sys_write32(BIT(SPRU_BBL_CMD__IND_SOFT_RST_N), SPRU_BBL_CMD);
	MCU_timer_udelay(320);
}

/**
 * @brief Read SPRU Poll Status
 * @details Read SPRU Poll Status
 * check indirect access done for the register access procedure
 * register SPRU_BBL_STATUS
 * return 0 on success, return -1 if failed on delay_us
 *
 * @param enable / disable
 * @param Number of attempts
 *
 * @return None
 *
 * Note: Due to unreliability of the CRMU Timer, switched to
 * number of attempts instead of CRMU timer based delay.
 */
static s32_t spru_poll_status(u32_t en, u32_t num_tries)
{
	s32_t result = -1;

	do {
		if (((sys_read32(SPRU_BBL_STATUS) >> SPRU_BBL_STATUS__ACC_DONE)
			 & 0x00000001) == en) {
			result = 0;
			break;
		}
	} while (num_tries--); /* Number of Tries */

	return result;
}

/**
 * @brief Write BBL Register
 * @details Write BBL Register
 * indirect BBL register access, write to regAddrSel with wrDat
 * return 0 on success, return -1 on fail
 *
 * -- TODO -- Confirm if the algorithm is correct with H/W.
 *
 * @param register address
 * @param data to be written
 *
 * @return result (Success / Failure)
 */
s32_t bbl_write_reg(u32_t regAddr, u32_t wrDat)
{
	s32_t result;
	s32_t i = 0;
	s32_t rc = -1;
	int alarm_evt;

	alarm_evt  = MCU_get_event(MCU_SPRU_ALARM_EVENT);

	if (alarm_evt != 0)
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 0);

	spru_wr_data(wrDat);
	for (i = 0; i < 3; i++) {
		spru_wr_cmd(regAddr);
		result = spru_poll_status(1, 500);
		if (result == 0) {
			rc = 0;
			break;
		}
		spru_interface_reset();
	}
	if (alarm_evt != 0)
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 1);

	return rc;
}

/**
 * @brief Read BBL Register
 * @details Read BBL Register
 * indirect BBL register access, read regAddrSel
 * *p_dat will point to value and return 0 on success , return -1 on fail
 *
 * -- TODO -- Confirm if the algorithm is correct with H/W.
 *
 * @param register address
 * @param return location to hold data read
 *
 * @return result (Success / Failure)
 */
s32_t bbl_read_reg(u32_t regAddr, u32_t *p_dat) {
	s32_t result;
	s32_t i = 0;
	s32_t rc = -1;
	int alarm_evt;

	alarm_evt  = MCU_get_event(MCU_SPRU_ALARM_EVENT);
	if (alarm_evt != 0)
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 0);

	for (i = 0; i < 3; i++) {
		spru_rd_cmd(regAddr);
		result = spru_poll_status(1, 500);
		if (result == 0) {
			spru_rd_data(p_dat);
			rc = 0;
			break;
		}
		spru_interface_reset();
	}
	if (alarm_evt != 0)
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 1);

	return rc;
}

/**
 * @brief MCU_bbl_access_ok
 *
 * Description:
 *
 *
 * @param none
 *
 * @return none
 */
s32_t MCU_bbl_access_ok(void)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	if (!pm_args->bbl_access_ok)
		return -1;

	return 0;
}

/**
 * @brief spru_reg_read
 *
 * Description:
 * bbl clock is 32.768KHz
 * when accessing bbl configuration registers, max delay for acc_done bit
 * set is 10 cycles, about 10 * 30 us. when access bbl ram, max delay for
 * acc_done bit set is 10+ 40 cycles, close to 50 *30 us. Please note that
 * this is the worst case time when you are accessing BBRAM.
 *
 * @param addr
 *
 * @return result
 */
u32_t spru_reg_read(u32_t addr)
{
	u32_t regval = 0;

	bbl_read_reg(addr, &regval);

	return regval;
}

/**
 * @brief spru_reg_write
 *
 * Description:
 * bbl clock is 32.768KHz
 * when accessing bbl configuration registers, max delay for acc_done bit
 * set is 10 cycles, about 10 * 30 us. when access bbl ram, max delay for
 * acc_done bit set is 10+ 40 cycles, close to 50 *30 us. Please note that
 * this is the worst case time when you are accessing BBRAM.
 *
 * @param addr
 * @param data
 *
 * @return result
 */
u32_t spru_reg_write(u32_t addr, u32_t data)
{
	bbl_write_reg(addr, data);

	return 0;
}

#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
/**
 * @brief MCU_setup_tamper
 *
 * Description:
 *
 * @param op
 *
 * @return none
 */
void MCU_setup_tamper(MPROC_LOW_POWER_OP_e op)
{
	if (op == MPROC_ENTER_LP) {
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 1);
		spru_reg_write(BBL_INTERRUPT_clr, 0xfc);
	} else {
		MCU_enable_event(MCU_SPRU_ALARM_EVENT, 0);
	}
}
#endif

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
/**
 * @brief MCU_setup_rtc
 *
 * Description:
 *
 * @param op
 *
 * @return none
 */
void MCU_setup_rtc(MPROC_LOW_POWER_OP_e op)
{
	if (MCU_bbl_access_ok() < 0)
		return;

	if (op == MPROC_ENTER_LP) {
		/* enable rtc intr now */
		MCU_enable_event(MCU_SPRU_RTC_PERIODIC_EVENT, 1);
	} else {
		/* disable rtc intr now */
		MCU_enable_event(MCU_SPRU_RTC_PERIODIC_EVENT, 0);
	}
	spru_reg_write(BBL_INTERRUPT_clr, BIT(BBL_INTERRUPT_clr__bbl_period_intr_clr));
	MCU_clear_event(MCU_SPRU_RTC_PERIODIC_EVENT);
}
#endif
#endif

/**
 * @brief MCU_setup_wakeup_src
 *
 * Description:
 *
 * @param op
 *
 * @return none
 */
void MCU_setup_wakeup_src(MPROC_LOW_POWER_OP_e op)
{
#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();
#endif

	if (op == MPROC_ENTER_LP) { /* go to low power mode */
		MCU_setup_aongpio(MPROC_ENTER_LP);

		/* Prepare USB wake-Up Sources if any */
		MCU_setup_usb(MPROC_ENTER_LP);

#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
		MCU_setup_tamper(MPROC_ENTER_LP);
#endif

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
		if (pm_args->seconds_to_wakeup)
			MCU_setup_rtc(MPROC_ENTER_LP);
		else
			MCU_setup_rtc(MPROC_EXIT_LP);
#endif
	} else { /* exit from low power mode */
		MCU_setup_aongpio(MPROC_EXIT_LP);

		/* Restore USB wake-Up Sources if any */
		MCU_setup_usb(MPROC_EXIT_LP);

#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
		MCU_setup_tamper(MPROC_EXIT_LP);
#endif

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
		MCU_setup_rtc(MPROC_EXIT_LP);
#endif
	}
}
