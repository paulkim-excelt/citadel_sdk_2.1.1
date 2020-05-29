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
#include "broadcom/dma.h"
#include "adc/adc.h"
#include "adc_regs.h"
#include "sotp.h"
#include "assert.h"
#include <stdbool.h>
#include <errno.h>
#include "logging/sys_log.h"
#include "misc/util.h"
#include <board.h>
/*
 * TODO: Annotate all functions with explanation of algorithm
 */
/* This header includes the arch specific sys_io.h for register operations */
/* Pull in the arch-specific implementations */
#include <arch/cpu.h>

#define INT_SRC_ADC5 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT5
#define INT_SRC_ADC4 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT4
#define INT_SRC_ADC3 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT3
#define INT_SRC_ADC2 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT2
#define INT_SRC_ADC1 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT1
#define INT_SRC_ADC0 CHIP_INTR__IOSYS_ADC_INTERRUPT_BIT0

#define INT_SRC_ADC0_PRIORITY_LEVEL 0
#define INT_SRC_ADC1_PRIORITY_LEVEL 0
#define INT_SRC_ADC2_PRIORITY_LEVEL 0
#define INT_SRC_ADC3_PRIORITY_LEVEL 0
#define INT_SRC_ADC4_PRIORITY_LEVEL 0
#define INT_SRC_ADC5_PRIORITY_LEVEL 0

#define ADC_C(addr, adcno, ch) ((addr) + (adcno) * 0x80 + (ch) * 0x20)
#define ADC_N(addr, adcno) ((addr) + (adcno) * 0x80)

/** ADC Device Private data structure */
struct bcm5820x_adc_dev_data_t {
	mem_addr_t adc0_base; /**< ADC0 Base Address */
	mem_addr_t adc1_base; /**< ADC1 Base Address */
	mem_addr_t adc2_base; /**< ADC2 Base Address */
	/**< ADC Ch IRQ Callback function pointer */
	adc_irq_callback_t adc_irq_cb[adc_max][adc_chmax];
};

/* convenience defines */
#define ADC_DEV_DATA(dev) ((struct bcm5820x_adc_dev_data_t *)(dev)->driver_data)

/******************************************************************************
 * P R I V A T E  F U N C T I O N S for Installing & Invoking ISR Callbacks
 ******************************************************************************/
static void bcm5820x_adc_adc0_ch0_isr_init(struct device *dev);
static void bcm5820x_adc_adc0_ch1_isr_init(struct device *dev);
static void bcm5820x_adc_adc1_ch0_isr_init(struct device *dev);
static void bcm5820x_adc_adc1_ch1_isr_init(struct device *dev);
static void bcm5820x_adc_adc2_ch0_isr_init(struct device *dev);
static void bcm5820x_adc_adc2_ch1_isr_init(struct device *dev);
/****************************************************************
 * Interrupt Service Routines Callback Installers:
 *
 * These functions actually initialize the IRQ callback functions
 * for the device in the ADC device private data structure.
 ****************************************************************/
/**
 * @brief Set the callback function pointer for ADC IRQ.
 *        Actual instance for bcm5820x (Citadel). This is invoked from
 *        public installer adc_set_adc_irq_callback
 *
 * @param dev PM device struct
 * @param adcno ADC Number
 * @param ch ADC Channel Number
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void bcm5820x_set_adc_irq_callback(struct device *dev,
		adc_no adcno, adc_ch ch, adc_irq_callback_t cb)
{
	struct bcm5820x_adc_dev_data_t *const data = ADC_DEV_DATA(dev);

	if ((adcno < adc_max) && (ch < adc_chmax)) data->adc_irq_cb[adcno][ch] = cb;
}

/****************************************************************
 ***** Interrupt Service Routines Wrappers (invokers)************
 ****************************************************************/
/**
 * @brief ADC0 Ch0 IRQ ISR invoker(wrapper).
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg pointer to device object.
 * @param adcno ADC Number.
 * @param ch ADC Channel Number.
 * @param count Resulting ADC count.
 *
 * @return N/A
 */
static void bcm5820x_adc_irq_isr(void *arg, adc_no adcno,
		adc_ch ch, s32_t count)
{
	struct device *dev = arg;
	struct bcm5820x_adc_dev_data_t *const data = ADC_DEV_DATA(dev);

	if ((adcno < adc_max) && (ch < adc_chmax))
	{
		if (data->adc_irq_cb[adcno][ch]) data->adc_irq_cb[adcno][ch](dev, adcno, ch, count);
	}
}

/****************************************************************
 **************** Interrupt Service Routines ********************
 ****************************************************************/
/**
 * @brief The actual ADC0 Channel 0 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc0_ch0_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	/* TODO: Are these bundled into the same IRQ?
	 * Bit0: Water Mark Interrupt
	 * Bit1: Buffer Full Interrupt
	 * Bit2: Buffer Empty Interrupt
	 */
	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 0, 0));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 0, 0),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	/* The Channels are typically programmed with 1/2 water mark
	 * using the ADC0_CHANNEL0_CNTRL2 register[5:0] which is for
	 * watermark level. So hopefully that will be OK.
	 */
	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 0, 0, counts);
	}

	/* Write the same status back to the status register to clear
	 * the interrupts
	 */
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 0, 0));
}

/**
 * @brief The main ADC0 Channel 1 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc0_ch1_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 0, 1));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 0, 1),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 0, 1, counts);
	}
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 0, 1));
}

/**
 * @brief The main ADC1 Channel 0 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc1_ch0_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 1, 0));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 1, 0),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 1, 0, counts);
	}
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 1, 0));
}

/**
 * @brief The main ADC1 Channel 1 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc1_ch1_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 1, 1));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 1, 1),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 1, 1, counts);
	}
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 1, 1));
}

/**
 * @brief The main ADC2 Channel 0 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc2_ch0_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 2, 0));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 2, 0),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 2, 0, counts);
	}
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 2, 0));
}

/**
 * @brief The main ADC2 Channel 1 IRQ Interrupt service routine.
 *
 * @param dev device
 *
 * @return N/A
 */
static void adc2_ch1_isr(void *dev)
{
	s32_t counts;
	u32_t status;

	status = sys_read32(ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 2, 1));

	counts = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, 2, 1),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH);

	if (counts) {
		/* Invoke the user provided callback function */
		bcm5820x_adc_irq_isr(dev, 2, 1, counts);
	}
	sys_write32(status, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, 2, 1));
}

static void bcm5820x_adc_isr_init(struct device *dev)
{
	/* Enable ADC Interrupts */
	bcm5820x_adc_adc0_ch0_isr_init(dev);
	bcm5820x_adc_adc0_ch1_isr_init(dev);
	bcm5820x_adc_adc1_ch0_isr_init(dev);
	bcm5820x_adc_adc1_ch1_isr_init(dev);
	bcm5820x_adc_adc2_ch0_isr_init(dev);
	bcm5820x_adc_adc2_ch1_isr_init(dev);
}

/*****************************************************************************
 ************************* P U B L I C   A P I s *****************************
 *****************************************************************************/

s32_t bcm5820x_adc_get_snapshot(struct device *dev, adc_no adcno,
		adc_ch ch, u16_t *value)
{
	u32_t status = 0;
	ARG_UNUSED(dev);

	if ((adcno >= adc_max) || (ch >= adc_chmax))
		return -EINVAL;

	sys_clear_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_mode);
	sys_set_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_enable);

	while (!reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, adcno, ch),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__total_enteries_WIDTH))
		;

	*value = sys_read32(ADC_C(ADC0_CHANNEL0_DATA, adcno, ch));

	sys_clear_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_enable);

	return status;
}

s32_t bcm5820x_adc_powerdown(struct device *dev)
{
	ARG_UNUSED(dev);
	sys_write32(0, CRMU_ADC_LDO_PU_CTRL);
	sys_write32(1, ADC_ISO_CONTROL);

	return 0;
}

s32_t bcm5820x_adc_enable_nrounds(struct device *dev, adc_no adcno,
		adc_ch ch, s32_t rounds)
{
	ARG_UNUSED(dev);

	if ((adcno < adc_max) && (ch < adc_chmax) && (rounds > 0) && (rounds < 0x3f))
	{
		reg_bits_set(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
			ADC0_CHANNEL0_CNTRL1__channel_rounds_R,
			ADC0_CHANNEL0_CNTRL1__channel_rounds_WIDTH, rounds);
		sys_set_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
			ADC0_CHANNEL0_CNTRL1__channel_mode);
		sys_set_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
			ADC0_CHANNEL0_CNTRL1__channel_enable);
	} else {
		return -EINVAL;
	}

	return 0;
}

/* Reads data out of the the ADC FIFO Buffer. "len" is the desired
 * number of entries to be read. However, if the number of valid
 * entries is more than what is requested, then only the requested
 * number of entries are read. If number of valid entries is less
 * then only that many entries are read.
 *
 * TODO: The default value of total entries is 0x40 (64). Does that
 * mean the FIFO buffer is 64 entries big?
 *
 * returns the number of entries read out.
 */
s32_t bcm5820x_adc_get_data(struct device *dev, adc_no adcno,
		adc_ch ch, u16_t *buf, s32_t len)
{
	s32_t i;
	s32_t entries = 0;
	ARG_UNUSED(dev);

	if ((adcno >= adc_max) || (ch >= adc_chmax))
		return -EINVAL;

	entries = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, adcno, ch),
	ADC0_CHANNEL0_STATUS__valid_enteries_R,
	ADC0_CHANNEL0_STATUS__valid_enteries_WIDTH);

	if (entries > len)
		entries = len;

	for (i = 0; i < entries; i++) {
		buf[i] = sys_read32(ADC_C(ADC0_CHANNEL0_DATA, adcno, ch));
	}

	return entries;
}

/* ADC Clock Divisor:
 * The value in low_div determines the width of the low part (low voltage)
 * of the ADC clock waveform and the value in high_div determines the width
 * of the high part of the ADC Clock waveform.
 */
s32_t bcm5820x_adc_set_clk_div(struct device *dev, u32_t lodiv, u32_t hidiv)
{
	u32_t divisor = 0x80000000;
	ARG_UNUSED(dev);

	if(lodiv > MAX_ADC_CLK_DIV || hidiv > MAX_ADC_CLK_DIV)
		return -EINVAL;

	divisor |= (lodiv | (hidiv << 16));
	/* Program the ADC Clock Divisor: */
	sys_write32(divisor, CRMU_ADC_CLK_DIV);

	return 0;
}

/* Retrieve ADC Clock Divisor:
 * The value in low_div determines the width of the low part (low voltage)
 * of the ADC clock waveform and the value in high_div determines the width
 * of the high part of the ADC Clock waveform.
 */
s32_t bcm5820x_adc_get_clk_div(struct device *dev)
{
	u32_t divisor;
	ARG_UNUSED(dev);

	/* Program the ADC Clock Divisor: */
	divisor = sys_read32(CRMU_ADC_CLK_DIV);

	return divisor;
}

/* ADCx_ANALOG_CONTROL: Per ADC Analog Control
 *
 *-------------------------------------------------------------------------
 * Field        View    Bit(s)  Description                         Default
 *-------------------------------------------------------------------------
 * data_rate     Common  15:13   Data rate control for the ADC.        0x0
 *-----------------------------------------------------------------------
 * TODO: Why do we program the divisor into this field?
 *
 * This controls the data rate of both the channels.
 * 'f' is the adc clock freq; n is the number of channels.
 * 000 : data rate f/14*n
 * 001 : data rate f/14*2n
 * 010 : data rate f/14*3n
 * 011 : data rate f/14*4n
 * 100 : data rate f/14*5n
 * 101 : data rate f/14*6n
 * Special case:
 * 111 : Max data rate f/14*m : where m is number of enabled channels
 *-----------------------------------------------------------------------
 * logic_resetb  Common  12      Write ‘1’ to bring ADC out of reset.  0
 *
 * Note: We are clearing this bit, then setting it after 1000us to
 * bring ADC out of reset.
 *-----------------------------------------------------------------------
 * testbuf_en    Common  11                                            0
 *
 * Note: When 1, it brings the reference buffer output voltage to in_ch1
 *-----------------------------------------------------------------------
 * adc_pd        Common  10      ADC Power Down                        1
 *
 * Note: Need to clear adc_pd to enable ADC.
 *-----------------------------------------------------------------------
 * extclk_en     Common  9                                             0
 * comp_en       Common  8                                             1
 * cmpctrl       Common  7:6                                         0x2
 * bufctrl       Common  5:4                                         0x2
 * buf_vout_ctrl Common  3:2                                         0x2
 * bg_buf_en     Common  1                                             1
 * adc_digin_en  Common  0                                             0
 *-----------------------------------------------------------------------
 *
 */
s32_t bcm5820x_adc_set_rate(struct device *dev, adc_no adcno, s32_t div)
{
	ARG_UNUSED(dev);

	if (adcno >= adc_max)
		return -EINVAL;

	reg_bits_set(ADC_N(ADC0_ANALOG_CONTROL, adcno),
		ADC0_ANALOG_CONTROL__data_rate_R,
		ADC0_ANALOG_CONTROL__data_rate_WIDTH, div);
	return 0;
}

/* From the ADC Controller Document:
 * 4.2.3 Channel Block:
 * This block holds all the channel specific control and status information:
 * Channel enable : Indicates the channel is active.
 * Channel mode : Each channel can either work in SnapShot Mode or TDM mode.
 * - Snapshot Mode - :
 * In snapshot mode, it initiates one conversion from the ADC, captures the
 * data and disables the channel.
 *
 * - TDM mode - :
 * The number of samples to convert and capture is indicated by
 * ‘Channel Rounds’ variable. After the ADC has done conversion of these
 * many data, the channel is disabled.
 *
 * Channel rounds: This is to control to the rate of conversion for the
 * particular channel.
 *
 * In special case where Rounds setting in ADC Control Register is
 * (6’b111111) the TDM mode work in continuous infinite fashion until
 * the channel is disabled by software.
 *
 * This block is also responsible for writing the digital data in the scratch
 * RAM.  Each channel has a dedicated 64entries space in the RAM. This enables
 * us to keep all the channel data together and saves the overhead of separating
 * it later. The ‘read pointer’ and ‘write pointer’ of the channel is maintained
 * by this block. The pointers are used to know the status of the buffer –full,
 * empty, water mark level and generate maskable interrupts to software to act
 * on it. All the APB reads from the HOST to the scratch RAM is routed through
 * this block as this maintains the pointers. Each channel has one 32bit Address
 * corresponding to its 64enteries space in the RAM. All the reads to this
 * address from Host will return the Buffer data and increments the read pointer.
 * Since scratch RAM is a single RAM shared across all the ADC channel blocks
 * and flex timers , there might be conflict in accessing the RAM. To avoid the
 * loss of any adc data, each channel block has a 2-enteries deep fifo. Any ADC
 * converted data is first written to this fifo and then read back and stored
 * in the RAM. This block works mostly on the ‘host clock’ (100MHz) while ADC
 * works on a much smaller clock ‘adc clock’ (1.5MHz) Thus a 2 entries FIFO
 * should be enough to hold the data till the channel gets access to the RAM.
 * This block generates the “Enable” signal for the Controller block and
 * receives the ‘Done’ pulse when the conversion is done and ADC data is
 * ready. Then it samples the ADC output from the Analog IP and synchronize
 * it to the host clock before writing the data to the 2-entry FIFO.
 *
 */
s32_t bcm5820x_adc_empty_fifo(struct device *dev, adc_no adcno, adc_ch ch)
{
	s32_t entries = 0;
	s32_t i;
	ARG_UNUSED(dev);

	if ((adcno >= adc_max) || (ch >= adc_chmax))
		return -EINVAL;

	entries = reg_bits_read(ADC_C(ADC0_CHANNEL0_STATUS, adcno, ch),
		ADC0_CHANNEL0_STATUS__valid_enteries_R,
		ADC0_CHANNEL0_STATUS__valid_enteries_WIDTH);
	for (i = 0; i < entries; i++) {
		sys_read32(ADC_C(ADC0_CHANNEL0_DATA, adcno, ch));
	}
	return 0;
}

s32_t bcm5820x_adc_enable_tdm(struct device *dev, adc_no adcno,
		adc_ch ch, adc_irq_callback_t cb)
{
	if ((adcno >= adc_max) || (ch >= adc_chmax))
		return -EINVAL;

	/* FIXME: May be the #rounds should be programmable by user.
	 * Here TDM rounds are programmed to Continuous mode until
	 * disabled by software.
	 *
	 */
	reg_bits_set(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_rounds_R,
		ADC0_CHANNEL0_CNTRL1__channel_rounds_WIDTH, 0x3f);
	/* Set Channel Mode to be in TDM mode */
	sys_set_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_mode);
	/* Finally Enable the channel itself */
	sys_set_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_enable);

	bcm5820x_adc_empty_fifo(dev, adcno, ch);

	if (cb) {
		/* adccb[adcno][ch] = cb; */
		bcm5820x_set_adc_irq_callback(dev, adcno, ch, cb);

		/* config & enable watermark 1/2 buffer
		 * FIXME: May be the watermark should be programmable.
		 */
		reg_bits_set(ADC_C(ADC0_CHANNEL0_CNTRL2, adcno, ch),
		ADC0_CHANNEL0_CNTRL2__watermark_R,
		ADC0_CHANNEL0_CNTRL2__watermark_WIDTH, 16);
		/* Clear the interrupts by writing '1' */
		sys_write32(0x7, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, adcno, ch));
		/* Unmask the Full & watermark interrupt */
		sys_write32(0x3, ADC_C(ADC0_CHANNEL0_INTERRUPT_MASK, adcno, ch));
	} else {
		/* adccb[adcno][ch] = NULL; */
		bcm5820x_set_adc_irq_callback(dev, adcno, ch, NULL);
		/* Mask all interrupts since there is no Callback function
		 * to handle the interrupt.
		 */
		sys_write32(0, ADC_C(ADC0_CHANNEL0_INTERRUPT_MASK, adcno, ch));
	}

	return 0;
}

s32_t bcm5820x_adc_disable_tdm(struct device *dev, adc_no adcno, adc_ch ch)
{
	if ((adcno >= adc_max) || (ch >= adc_chmax))
		return -EINVAL;

	/* Mask all interrupts */
	sys_write32(0, ADC_C(ADC0_CHANNEL0_INTERRUPT_MASK, adcno, ch));
	/* Clear all interrupts */
	sys_write32(0x7, ADC_C(ADC0_CHANNEL0_INTERRUPT_STATUS, adcno, ch));

	/* Clear Mode bit to be in snapshot mode */
	sys_clear_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_mode);
	/* Finally disable the channel itself */
	sys_clear_bit(ADC_C(ADC0_CHANNEL0_CNTRL1, adcno, ch),
		ADC0_CHANNEL0_CNTRL1__channel_enable);

	bcm5820x_adc_empty_fifo(dev, adcno, ch);

	bcm5820x_set_adc_irq_callback(dev, adcno, ch, NULL);

	return 0;
}

s32_t bcm5820x_adc_init(struct device *dev, adc_no adcno)
{
	u32_t value;
	u32_t otp_bvc; /* otp_buf_vout_ctrl */

	if (adcno >= adc_max)
	{
		SYS_LOG_ERR("Invalid ADC number!!");
		return -EINVAL;
	}

	otp_bvc = read_bvc_from_chip_otp();

	/* Turn the main ADC LDO On.
	 * FIXME: Ideally when we are doing per ADC initialization
	 * This should be done only once.
	 */
	sys_write32(1, CRMU_ADC_LDO_PU_CTRL);
	/* Remove Isolation for ADCs 0, 1 & 2.
	 * FIXME: Again inside the per ADC Initialization,
	 * we should ideally be removing Isolation only for the
	 * desired ADC not for all.
	 * Bit 0: ADC 0; Bit 1: ADC 1; Bit 2: ADC 2
	 */
	sys_write32(0, ADC_ISO_CONTROL);

	/* TODO: Integrate DMA feature into ADC */
	/* per adc - Clear Power Down bit */
	sys_clear_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno), ADC0_ANALOG_CONTROL__adc_pd);
	/* per adc - Clear Logic Reset */
	sys_clear_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno), ADC0_ANALOG_CONTROL__logic_resetb);
	/* k_sleep crashes as it attempts to suspend the current thread */
	k_busy_wait(1000);
	/* per adc - Set Logic Reset to bring it out of reset */
	sys_set_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno), ADC0_ANALOG_CONTROL__logic_resetb);

	k_busy_wait(1000);

	/* TODO: Check if these are still needed
	 * adc_disable_tdm(adcno, adc_ch0);
	 * adc_disable_tdm(adcno, adc_ch1);
	 */
	/* WAR for Parts with otp buf_vout_ctrl:
	 * (otp_bvc == 1 ) : Seems to have a significantly higher ADC0 gain code(854).
	 *
	 * (otp_bvc == 0 ) : Have gain code (790+).
	 * So change the reference voltage for ADC0.
	 *
	 * ATE Pattern As follows:
	 *
	 * if(adc_no == 0) begin
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC0_ANALOG_CONTROL, rd_data);
	 *    rd_data[`ADC0_ANALOG_CONTROL__bufctrl] = 'h3;
	 *    rd_data[`ADC0_ANALOG_CONTROL__cmpctrl] = 'h3;
	 *    rd_data[`ADC0_ANALOG_CONTROL__buf_vout_ctrl] = 'h2;
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC0_ANALOG_CONTROL, rd_data);
	 * end
	 *
	 * if(adc_no == 1) begin
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC1_ANALOG_CONTROL, rd_data);
	 *    rd_data[`ADC1_ANALOG_CONTROL__bufctrl] = 'h3;
	 *    rd_data[`ADC1_ANALOG_CONTROL__cmpctrl] = 'h3;
	 *    rd_data[`ADC1_ANALOG_CONTROL__buf_vout_ctrl] = 'h0;
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC1_ANALOG_CONTROL, rd_data);
	 * end
	 *
	 * if(adc_no == 2) begin
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC2_ANALOG_CONTROL, rd_data);
	 *    rd_data[`ADC2_ANALOG_CONTROL__bufctrl] = 'h3;
	 *    rd_data[`ADC2_ANALOG_CONTROL__cmpctrl] = 'h3;
	 *    rd_data[`ADC2_ANALOG_CONTROL__buf_vout_ctrl] = 'h0;
	 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC2_ANALOG_CONTROL, rd_data);
	 * end
	 *
	 * 3. Need to differentiate between otp_bvc == 1 & otp_bvc == 0 parts by reading OTP bits.
	 */
	value = sys_read32(ADC_N(ADC0_ANALOG_CONTROL, adcno));
	value = value | 0xf0; /* ADC 0, 1 & 2 set both bufctrl & cmpctrl to 3 */
	value = value & ~0xc; /* Clear bits 2 & 3 for buf_vout_ctrl */
	if (adcno == 0 && otp_bvc)
		value = value | 0x8; /* ADC 0 */
	sys_write32(value, ADC_N(ADC0_ANALOG_CONTROL, adcno));

	/* TODO: Check if the IRQ need to be enabled
	 * and hooked to ISRs at this point
	 *
	 * Enable ADC Interrupts
	 *
	 * FIXME: Should this be specific to the ADC only?
	 * Why old code does this for all ADCs even though the function
	 * is meant to init only one ADC.
	 */
	bcm5820x_adc_isr_init(dev);

	value = sys_read32(ADC_DMA_CHANNEL_SEL);
	value |= 0xf << (adcno * 4);
	sys_write32(value, ADC_DMA_CHANNEL_SEL);

	/* Bit 0: Enable Channel - Leave the default (Disabled)
	 * The channel will be enabled only when required.
	 *
	 * Bit 1: 1: TDM / 0: Snapshot: Leave the default
	 *
	 * Bits [7:2]: Number of rounds for TDM
	 *
	 * Disable the DMA BREQ/SREQ Enable bit - No DMA for now.
	 */
	sys_clear_bit(ADC_N(ADC0_CHANNEL0_CNTRL1, adcno), ADC0_CHANNEL0_CNTRL1__dma_en);

	SYS_LOG_DBG("Initialized ADC driver...");

	return 0;
}

s32_t bcm5820x_adc_init_all(struct device *dev)
{
	u32_t value;
	u32_t otp_bvc; /* otp_buf_vout_ctrl */

	sys_write32(1, CRMU_ADC_LDO_PU_CTRL);
	sys_write32(0, ADC_ISO_CONTROL);

	otp_bvc = read_bvc_from_chip_otp();

	/* TODO: Integrate DMA feature into ADC */
	adc_no adcno;
	for (adcno = adc_0; adcno < adc_max; adcno++) {
		/* per adc */
		sys_clear_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno),
		ADC0_ANALOG_CONTROL__adc_pd);
		sys_clear_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno),
		ADC0_ANALOG_CONTROL__logic_resetb);
		/* k_sleep crashes as it attempts to suspend
		 * the current thread
		 */
		k_busy_wait(1000);

		sys_set_bit(ADC_N(ADC0_ANALOG_CONTROL, adcno),
		ADC0_ANALOG_CONTROL__logic_resetb);

		k_busy_wait(1000);

		/* TODO: Check if these are still needed
		 * adc_disable_tdm(adcno, adc_ch0);
		 * adc_disable_tdm(adcno, adc_ch1);
		 */

		/* TODO: Check if the IRQ need to be enabled
		 * and hooked to ISRs at this point
		 */

	        /* WAR for Parts with otp buf_vout_ctrl:
                 * (otp_bvc == 1 ) : Seems to have a significantly higher ADC0 gain code(854).
                 *
                 * (otp_bvc == 0 ) : Have gain code (790+).
                 * So change the reference voltage for ADC0.
		 *
		 * Cmpctrl - 0x3
		 * Bufctrl -0x3
		 * Buf_vout_ctrl - 0x0 ( for otp_bvc & adc0 alone 0x1)
		 *
		 * ATE Pattern As follows:
		 *
		 * if(adc_no == 0) begin
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC0_ANALOG_CONTROL, rd_data);
		 *    rd_data[`ADC0_ANALOG_CONTROL__bufctrl] = 'h3;
		 *    rd_data[`ADC0_ANALOG_CONTROL__cmpctrl] = 'h3;
		 *    rd_data[`ADC0_ANALOG_CONTROL__buf_vout_ctrl] = 'h2;
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC0_ANALOG_CONTROL, rd_data);
		 * end
		 *
		 * if(adc_no == 1) begin
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC1_ANALOG_CONTROL, rd_data);
		 *    rd_data[`ADC1_ANALOG_CONTROL__bufctrl] = 'h3;
		 *    rd_data[`ADC1_ANALOG_CONTROL__cmpctrl] = 'h3;
		 *    rd_data[`ADC1_ANALOG_CONTROL__buf_vout_ctrl] = 'h0;
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC1_ANALOG_CONTROL, rd_data);
		 * end
		 *
		 * if(adc_no == 2) begin
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_rd_single(`ADC2_ANALOG_CONTROL, rd_data);
		 *    rd_data[`ADC2_ANALOG_CONTROL__bufctrl] = 'h3;
		 *    rd_data[`ADC2_ANALOG_CONTROL__cmpctrl] = 'h3;
		 *    rd_data[`ADC2_ANALOG_CONTROL__buf_vout_ctrl] = 'h0;
		 *    soc.tbc.driver_host[iproc_common_pkg::cpu_host_e'(JTAG)].base.cpu_wr_single(`ADC2_ANALOG_CONTROL, rd_data);
		 * end
		 *
		 * 3. Need to differentiate between otp_bvc == 1 & otp_bvc == 0 by reading OTP bits.
		 */
		value = sys_read32(ADC_N(ADC0_ANALOG_CONTROL, adcno));
		value = value | 0xf0; /* ADC 0, 1 & 2 set both bufctrl & cmpctrl to 3 */
		value = value & ~0xc; /* Clear bits 2 & 3 for buf_vout_ctrl */
		if (adcno == 0 && otp_bvc) {
			value = value | 0x8; /* ADC 0 */
		}
		sys_write32(value, ADC_N(ADC0_ANALOG_CONTROL, adcno));

		value = sys_read32(ADC_DMA_CHANNEL_SEL);
		value |= 0xf << (adcno * 4);
		sys_write32(value, ADC_DMA_CHANNEL_SEL);

		sys_clear_bit(ADC_N(ADC0_CHANNEL0_CNTRL1, adcno),
		ADC0_CHANNEL0_CNTRL1__dma_en);
	}

	/* Enable ADC Interrupts */
	bcm5820x_adc_isr_init(dev);

	SYS_LOG_DBG("Initialized ADC driver...");

	return 0;
}

/** Instance of the ADC Device Template
 *  Contains the default values
 */
static const struct adc_driver_api bcm5820x_adc_driver_api = {
	.init_all = bcm5820x_adc_init_all,
	.init = bcm5820x_adc_init,
	.powerdown = bcm5820x_adc_powerdown,
	.set_clk_div = bcm5820x_adc_set_clk_div,
	.get_clk_div = bcm5820x_adc_get_clk_div,
	.set_rate = bcm5820x_adc_set_rate,
	.get_snapshot = bcm5820x_adc_get_snapshot,
	.enable_nrounds = bcm5820x_adc_enable_nrounds,
	.get_data = bcm5820x_adc_get_data,
	.empty_fifo = bcm5820x_adc_empty_fifo,
	.enable_tdm = bcm5820x_adc_enable_tdm,
	.disable_tdm = bcm5820x_adc_disable_tdm,
	.set_adc_irq_callback = bcm5820x_set_adc_irq_callback,
};

static const struct adc_device_config bcm5820x_adc_dev_cfg = {
	.irq_config_func = bcm5820x_adc_isr_init,
};

/** Private Data of the ADC
 *  Also contain the default values
 */
static struct bcm5820x_adc_dev_data_t bcm5820x_adc_dev_data = {
	.adc0_base = (mem_addr_t)ADC0_CHANNEL0_CNTRL1,
	.adc1_base = (mem_addr_t)ADC1_CHANNEL0_CNTRL1,
	.adc2_base = (mem_addr_t)ADC2_CHANNEL0_CNTRL1,
	.adc_irq_cb = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL} },
};

DEVICE_AND_API_INIT(bcm5820x_adc, CONFIG_ADC_NAME, &bcm5820x_adc_init_all,
		&bcm5820x_adc_dev_data, &bcm5820x_adc_dev_cfg,
		PRE_KERNEL_2, CONFIG_ADC_DRIVER_INIT_PRIORITY,
		&bcm5820x_adc_driver_api);

/****************************************************************
 * Initializers for IRQs:
 *
 * These functions hook the main ISR to the IRQ & enable the IRQ.
 ****************************************************************/
/**
 * @brief Hook ISR to & Enable the ADC0 Ch0 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc0_ch0_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC0 Ch0 isr */
	SYS_LOG_DBG("Register ADC0 Ch0 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC0 Ch0 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC0),
		INT_SRC_ADC0_PRIORITY_LEVEL, adc0_ch0_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC0));
	SYS_LOG_DBG("Enabled ADC0 Ch0 Interrupt & Installed handler.");
}

/**
 * @brief Hook ISR to & Enable the ADC0 Ch1 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc0_ch1_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC0 Ch1 isr */
	SYS_LOG_DBG("Register ADC0 Ch1 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC0 Ch1 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC1),
		INT_SRC_ADC1_PRIORITY_LEVEL, adc0_ch1_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC1));
	SYS_LOG_DBG("Enabled ADC0 Ch1 Interrupt & Installed handler.");
}

/**
 * @brief Hook ISR to & Enable the ADC1 Ch0 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc1_ch0_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC1 Ch0 isr */
	SYS_LOG_DBG("Register ADC1 Ch0 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC1 Ch0 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC2),
		INT_SRC_ADC2_PRIORITY_LEVEL, adc1_ch0_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC2));
	SYS_LOG_DBG("Enabled ADC1 Ch0 Interrupt & Installed handler.");
}

/**
 * @brief Hook ISR to & Enable the ADC1 Ch1 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc1_ch1_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC1 Ch1 isr */
	SYS_LOG_DBG("Register ADC1 Ch1 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC1 Ch1 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC3),
		INT_SRC_ADC3_PRIORITY_LEVEL, adc1_ch1_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC3));
	SYS_LOG_DBG("Enabled ADC1 Ch1 Interrupt & Installed handler.");
}

/**
 * @brief Hook ISR to & Enable the ADC2 Ch0 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc2_ch0_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC2 Ch0 isr */
	SYS_LOG_DBG("Register ADC2 Ch0 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC2 Ch0 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC4),
		INT_SRC_ADC4_PRIORITY_LEVEL, adc2_ch0_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC4));
	SYS_LOG_DBG("Enabled ADC2 Ch0 Interrupt & Installed handler.");
}

/**
 * @brief Hook ISR to & Enable the ADC2 Ch1 IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_adc_adc2_ch1_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 ADC2 Ch1 isr */
	SYS_LOG_DBG("Register ADC2 Ch1 ISR.");

	SYS_LOG_DBG("Set & Cleared ADC2 Ch1 Interrupt.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_ADC5),
		INT_SRC_ADC5_PRIORITY_LEVEL, adc2_ch1_isr, DEVICE_GET(bcm5820x_adc), 0);

	irq_enable(SPI_IRQ(INT_SRC_ADC5));
	SYS_LOG_DBG("Enabled ADC2 Ch1 Interrupt & Installed handler.");
}
