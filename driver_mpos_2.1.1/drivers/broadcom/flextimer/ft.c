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
#include "ft_regs.h"
/* This header includes the arch specific sys_io.h for register operations
 * Weirdly this also includes gcc.h which provides ARG_UNUSED macro.
 */
#include "broadcom/dma.h"
#include "flextimer/flextimer.h"
#include "assert.h"
#include <stdbool.h>
#include <errno.h>
#include "logging/sys_log.h"
#include "misc/util.h"
#include <board.h>
#include <pinmux.h>

#define FLEX_TIMER_CLK_PRESCALE 0x440c0800
#define FLEX_TIMER_BASE FLEX_TIMER_CLK_PRESCALE

#define CHIP_INTR__IOSYS_FLEX_TIMER_INTERRUPT 68
#define INT_SRC_FLEX_TIMER CHIP_INTR__IOSYS_FLEX_TIMER_INTERRUPT
#define INT_SRC_FLEX_TIMER_PRIORITY_LEVEL 0

#define CDRU_IOMUX_CTRL12 0x3001d104
#define CDRU_IOMUX_CTRL13 0x3001d108
#define CDRU_IOMUX_CTRL20 0x3001d124

#define CDRU_IOMUX_CTRL22 0x3001d12c
#define CDRU_IOMUX_CTRL23 0x3001d130
#define CDRU_IOMUX_CTRL24 0x3001d134

#define FT_MODE2_IO0_PIN 12
#define FT_MODE2_IO1_PIN 13
#define FT_MODE2_IO2_PIN 20
#define FT_MODE2_IO0_PIN_ADDR CDRU_IOMUX_CTRL12
#define FT_MODE2_IO1_PIN_ADDR CDRU_IOMUX_CTRL13
#define FT_MODE2_IO2_PIN_ADDR CDRU_IOMUX_CTRL20
#define FT_MODE2_IO0_PIN_FUNC PINMUX_FUNC_C
#define FT_MODE2_IO1_PIN_FUNC PINMUX_FUNC_C
#define FT_MODE2_IO2_PIN_FUNC PINMUX_FUNC_C

#define FT_MODE1_IO0_PIN 22
#define FT_MODE1_IO1_PIN 23
#define FT_MODE1_IO2_PIN 24
#define FT_MODE1_IO0_PIN_ADDR CDRU_IOMUX_CTRL22
#define FT_MODE1_IO1_PIN_ADDR CDRU_IOMUX_CTRL23
#define FT_MODE1_IO2_PIN_ADDR CDRU_IOMUX_CTRL24
#define FT_MODE1_IO0_PIN_FUNC PINMUX_FUNC_B
#define FT_MODE1_IO1_PIN_FUNC PINMUX_FUNC_B
#define FT_MODE1_IO2_PIN_FUNC PINMUX_FUNC_B

/** Flex Timer Device Private data structure */
struct bcm5820x_ft_dev_data_t {
	mem_addr_t ft_base; /**< Flex Timer Base Address */
	/**< Flex Timer Ch IRQ Callback function pointer */
	ft_irq_callback_t ft_irq_cb[ft_max];
	/**< The MFIO pins used for FT:
	 * Per Pin Number:
	 * [0]: Pin Number
	 * [1]: Function values to program
	 * [2]: Previously programmed value (Optionally to restore to after job).
	 */
	u32_t ft_iomux_pins[ft_max][3];
};

/* convenience defines */
#define FT_DEV_DATA(dev) ((struct bcm5820x_ft_dev_data_t *)(dev)->driver_data)

/******************************************************************************
 * P R I V A T E  F U N C T I O N S for Installing & Invoking ISR Callbacks
 ******************************************************************************/
static void bcm5820x_ft_isr_init(struct device *dev);

static s32_t ft_set_timeout(s32_t enable)
{
	if (enable) {
		sys_set_bit(FLEX_TIMER_CTRL1, FLEX_TIMER_CTRL1__timeout_en);
	} else {
		sys_clear_bit(FLEX_TIMER_CTRL1, FLEX_TIMER_CTRL1__timeout_en);
	}
	return 0;
}

static s32_t ft_set_watermark(s32_t depth)
{
	reg_bits_set(FLEX_TIMER_CTRL2, FLEX_TIMER_CTRL2__watermark0_R, FLEX_TIMER_CTRL2__watermark0_WIDTH, depth);
	reg_bits_set(FLEX_TIMER_CTRL2, FLEX_TIMER_CTRL2__watermark1_R, FLEX_TIMER_CTRL2__watermark1_WIDTH, depth);
	reg_bits_set(FLEX_TIMER_CTRL2, FLEX_TIMER_CTRL2__watermark2_R, FLEX_TIMER_CTRL2__watermark2_WIDTH, depth);
	return 0;
}

static s32_t ft_set_glitch(s32_t enable, s32_t extra_cycle)
{
	if (enable) {
		sys_clear_bit(FLEX_TIMER_CTRL2, FLEX_TIMER_CTRL2__glitch_filter_disable);
		if (extra_cycle) {
			reg_bits_set(FLEX_TIMER_CTRL2, 28, 2, extra_cycle - 1);
			sys_set_bit(FLEX_TIMER_CTRL2, 27);
		} else {
			sys_clear_bit(FLEX_TIMER_CTRL2, 27);
		}
	} else {
		sys_set_bit(FLEX_TIMER_CTRL2, FLEX_TIMER_CTRL2__glitch_filter_disable);
	}

	return 0;
}

static void ft_isr(void *dev)
{
	u32_t status = 0;
	s32_t entries = 0;
	struct bcm5820x_ft_dev_data_t *const ftdata = FT_DEV_DATA((struct device *)dev);

	status = sys_read32(FLEX_TIMER_INTR_STATUS);
	if (status & (1 << FLEX_TIMER_INTR_STATUS__sos)) {
		/* start swipe */
		if (ftdata->ft_irq_cb[0])
			ftdata->ft_irq_cb[0](dev, 0, 0, FT_START);
		ft_set_timeout(1);
	}

	entries = reg_bits_read(FLEX_TIMER_STATUS0, FLEX_TIMER_STATUS0__valid_enteries_R,
				FLEX_TIMER_STATUS0__valid_enteries_WIDTH);
	if (ftdata->ft_irq_cb[0] && entries)
		ftdata->ft_irq_cb[0](dev, 0, entries, FT_DATA);

	entries = reg_bits_read(FLEX_TIMER_STATUS1, FLEX_TIMER_STATUS1__valid_enteries_R,
				FLEX_TIMER_STATUS1__valid_enteries_WIDTH);
	if (ftdata->ft_irq_cb[1] && entries)
		ftdata->ft_irq_cb[1](dev, 1, entries, FT_DATA);

	entries = reg_bits_read(FLEX_TIMER_STATUS2, FLEX_TIMER_STATUS2__valid_enteries_R,
				FLEX_TIMER_STATUS2__valid_enteries_WIDTH);
	if (ftdata->ft_irq_cb[2] && entries)
		ftdata->ft_irq_cb[2](dev, 2, entries, FT_DATA);

	if (status & (1 << FLEX_TIMER_INTR_STATUS__timeout)) {
		ft_set_timeout(0);
		ftdata->ft_irq_cb[0](dev, 0, 0, FT_END);
	}

	sys_write32(status, FLEX_TIMER_INTR_STATUS);
}

/**
 * @brief Set the callback function pointer for FT IRQ.
 *
 * @param dev PM device struct
 * @param ftno FT Number
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static s32_t bcm5820x_ft_set_irq_callback(struct device *dev, ft_no ft, ft_irq_callback_t cb)
{
	struct bcm5820x_ft_dev_data_t *const data = FT_DEV_DATA(dev);

	data->ft_irq_cb[ft] = cb;

	return 0;
}

/*****************************************************************************
 ************************* P U B L I C   A P I s *****************************
 *****************************************************************************/

/**
 * @brief   	Init and enable flextimer
 * @details 	Init and enable flextimer
 * @param[in]   dev		FT device object
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
s32_t bcm5820x_ft_init(struct device *dev)
{
	bcm5820x_ft_isr_init(dev);
	SYS_LOG_DBG("FT ISR Configured.");

	/* Set the Pre-Scale to Divide By 10. */
	sys_write32(10, FLEX_TIMER_CLK_PRESCALE);

	/* Disable Time-out for Swiping Card. i.e. Wait Indefinitely.
	 *
	 * FIXME: Does this mean the "timeout" interrupt in the register
	 * FLEX_TIMER_INTR_STATUS will never be set when this is disabled
	 * in FLEX_TIMER_CTRL1?
	 */
	ft_set_timeout(0);

	/* enable H-L L-H, timeout max
	 * No Point in setting time out val, when time out itself is disabled.
	 * time-out detection disabled, DMA disabled.
	 *
	 * FIXME: Is the value 0b111 right value for L->H and H->L transitions?
	 */
	sys_write32(0xfffff077, FLEX_TIMER_CTRL1);

	ft_set_glitch(1, 0);

	/* Set Default Water-mark. Currently this is not used. */
	ft_set_watermark(0x3F);

	SYS_LOG_DBG("FT Initialized.");

	return 0;
}

/**
 * @brief Power Down Flex Timer Device
 * @details Power Down Flex Timer Device
 * @param[in] dev pointer to ft device
 * @retval 0:OK
 * @retval -1:ERROR
 */
s32_t bcm5820x_ft_powerdown(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/**
 * @brief   	Get flextimer data
 * @details 	Get flextimer data form hardware fifo
 * @param[in]   ftno 	flextimer number
 * @param[out]	buf		input buffer
 * @param[in]	len		input buffer length
 * @retval  	number of data copied to buffer
 */
s32_t bcm5820x_ft_get_data(struct device *dev, ft_no ftno, u16_t *buf, s32_t len)
{
	s32_t i;
	s32_t entries = 0;
	ARG_UNUSED(dev);

	entries = reg_bits_read(FLEX_TIMER_STATUS0 + ftno * 4, FLEX_TIMER_STATUS0__valid_enteries_R,
						FLEX_TIMER_STATUS0__valid_enteries_WIDTH);

	if (entries > len)
		entries = len;

	for (i = 0; i < entries; i++) {
		buf[i] = sys_read32(FLEX_TIMER_DATA_REG0 + ftno * 4);
	}

	return entries;
}

/**
 * @brief Power Down Flex Timer Device
 * @details Power Down Flex Timer Device
 * @param[in] dev pointer to ft device
 * @retval 0:OK
 * @retval -1:ERROR
 */
s32_t bcm5820x_ft_empty_fifo(struct device *dev, ft_no ftno)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ftno);

	return 0;
}

/**
 * @brief   	Disable flextimer
 * @details 	Disable all 3 flextimer
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
s32_t bcm5820x_ft_disable(struct device *dev)
{
	struct bcm5820x_ft_dev_data_t *const data = FT_DEV_DATA(dev);
	struct device *pinmuxdev;
	s32_t ret;

	/* disable INTR */
	sys_write32(0, FLEX_TIMER_INTR_MASK);

	data->ft_irq_cb[0] = NULL;
	data->ft_irq_cb[1] = NULL;
	data->ft_irq_cb[2] = NULL;

	pinmuxdev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	ret = pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[0][0], data->ft_iomux_pins[0][2])
	|| pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[1][0], data->ft_iomux_pins[1][2])
	|| pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[2][0], data->ft_iomux_pins[2][2]);

	if(ret) {
		SYS_LOG_ERR("Could not program IOMUX for FT on desired IO Pins.");
		return ret;
	}

	return 0;
}

/**
 * @brief   	Enable flextimer
 * @details 	Enable all 3 flextimer
 * @param[in]   inten 	enable interrupt
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
s32_t bcm5820x_ft_enable(struct device *dev, s32_t inten)
{
	u32_t ft_int_stat;
	s32_t entries = 0;
	struct bcm5820x_ft_dev_data_t *const data = FT_DEV_DATA(dev);
	struct device *pinmuxdev;
	s32_t ret;
	ARG_UNUSED(dev);

	/* disable INTR */
	sys_write32(0, FLEX_TIMER_INTR_MASK);

	/* Temporarily Set the IOMUXes for Default Pins for Flex Timer */
	pinmuxdev = device_get_binding(CONFIG_PINMUX_CITADEL_0_NAME);
	/* Save Original Functions of the Pins */
	pinmux_pin_get(pinmuxdev, data->ft_iomux_pins[0][0], &data->ft_iomux_pins[0][2]);
	pinmux_pin_get(pinmuxdev, data->ft_iomux_pins[1][0], &data->ft_iomux_pins[1][2]);
	pinmux_pin_get(pinmuxdev, data->ft_iomux_pins[2][0], &data->ft_iomux_pins[2][2]);

	/* Now try to program the desired functions */
	ret = pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[0][0], data->ft_iomux_pins[0][1])
	|| pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[1][0], data->ft_iomux_pins[1][1])
	|| pinmux_pin_set(pinmuxdev, data->ft_iomux_pins[2][0], data->ft_iomux_pins[2][1]);

	if(ret) {
		SYS_LOG_ERR("Could not program IOMUX for FT on desired IO Pins.");
		return ret;
	}

	entries = reg_bits_read(FLEX_TIMER_STATUS0, FLEX_TIMER_STATUS0__valid_enteries_R,
				FLEX_TIMER_STATUS0__valid_enteries_WIDTH);
	while (entries--)
		sys_read32(FLEX_TIMER_DATA_REG0);
	entries = reg_bits_read(FLEX_TIMER_STATUS1, FLEX_TIMER_STATUS1__valid_enteries_R,
				FLEX_TIMER_STATUS1__valid_enteries_WIDTH);
	while (entries--)
		sys_read32(FLEX_TIMER_DATA_REG1);
	entries = reg_bits_read(FLEX_TIMER_STATUS2, FLEX_TIMER_STATUS2__valid_enteries_R,
				FLEX_TIMER_STATUS2__valid_enteries_WIDTH);
	while (entries--)
		sys_read32(FLEX_TIMER_DATA_REG2);

	if (inten) {
		/* Clear all Interrupts by writing same value */
		ft_int_stat = sys_read32(FLEX_TIMER_INTR_STATUS);
		sys_write32(ft_int_stat, FLEX_TIMER_INTR_STATUS);
		/* Disable all except SoS Interrupt */
		sys_write32(BIT(FLEX_TIMER_INTR_STATUS__sos), FLEX_TIMER_INTR_MASK);
	}

	return 0;
}

/**
 * @brief   	Set flextimer IOMUX Mode
 * @details 	Choose which MFIOs to be used for flextimer
 * @param[in]   dev device object
 * @param[in]   ftiomode IO Mode
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
s32_t bcm5820x_ft_set_iomux_mode(struct device *dev, ft_iomux_mode ftiomode)
{
	struct bcm5820x_ft_dev_data_t *const data = FT_DEV_DATA(dev);

	if(ftiomode == FT_IOMUX_MODE1) {
		data->ft_iomux_pins[0][0] = FT_MODE1_IO0_PIN;
		data->ft_iomux_pins[1][0] = FT_MODE1_IO1_PIN;
		data->ft_iomux_pins[2][0] = FT_MODE1_IO2_PIN;
		data->ft_iomux_pins[0][1] = FT_MODE1_IO0_PIN_FUNC;
		data->ft_iomux_pins[1][1] = FT_MODE1_IO1_PIN_FUNC;
		data->ft_iomux_pins[2][1] = FT_MODE1_IO2_PIN_FUNC;
	}
	else if(ftiomode == FT_IOMUX_MODE2) {
		data->ft_iomux_pins[0][0] = FT_MODE2_IO0_PIN;
		data->ft_iomux_pins[1][0] = FT_MODE2_IO1_PIN;
		data->ft_iomux_pins[2][0] = FT_MODE2_IO2_PIN;
		data->ft_iomux_pins[0][1] = FT_MODE2_IO0_PIN_FUNC;
		data->ft_iomux_pins[1][1] = FT_MODE2_IO1_PIN_FUNC;
		data->ft_iomux_pins[2][1] = FT_MODE2_IO2_PIN_FUNC;
	}

	return 0;
}

/**
 * @brief   	Wait for Start of Swipe Event
 * @details 	Wait for Start of Swipe Event
 * @param[in]   dev FT device object
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
s32_t bcm5820x_ft_wait_for_sos(struct device *dev)
{
	u32_t sos;
	ARG_UNUSED(dev);

	/* Wait for the Start of Swipe Interrupt */
	do {
		sos = sys_read32(FLEX_TIMER_INTR_STATUS);
	} while(!(sos & (0x1 << FLEX_TIMER_INTR_STATUS__sos)));

	SYS_LOG_DBG("-----> Detected Start of Swipe <-----\n");

	return 0;
}

/** Instance of the FT Device Template
 *  Contains the default values
 */
static const struct ft_driver_api bcm5820x_ft_driver_api = {
	.init = bcm5820x_ft_init,
	.powerdown = bcm5820x_ft_powerdown,
	.get_data = bcm5820x_ft_get_data,
	.empty_fifo = bcm5820x_ft_empty_fifo,
	.set_ft_irq_callback = bcm5820x_ft_set_irq_callback,
	.disable = bcm5820x_ft_disable,
	.enable = bcm5820x_ft_enable,
	.set_iomux_mode = bcm5820x_ft_set_iomux_mode,
	.wait_for_sos = bcm5820x_ft_wait_for_sos,
};

static const struct ft_device_config bcm5820x_ft_dev_cfg = {
	.irq_config_func = bcm5820x_ft_isr_init,
};

/** Private Data of the FT
 *  Also contain the default values
 */
static struct bcm5820x_ft_dev_data_t bcm5820x_ft_dev_data = {
	.ft_base = (mem_addr_t)FLEX_TIMER_BASE,
	.ft_irq_cb = {NULL, NULL, NULL},
	.ft_iomux_pins = {{FT_MODE2_IO0_PIN, FT_MODE2_IO0_PIN_FUNC, 0}, {FT_MODE2_IO1_PIN, FT_MODE2_IO1_PIN_FUNC, 0}, {FT_MODE2_IO2_PIN, FT_MODE2_IO2_PIN_FUNC, 0}},
};

DEVICE_AND_API_INIT(bcm5820x_ft, CONFIG_FT_NAME, &bcm5820x_ft_init, &bcm5820x_ft_dev_data,
		&bcm5820x_ft_dev_cfg, PRE_KERNEL_2, CONFIG_FT_DRIVER_INIT_PRIORITY,
		&bcm5820x_ft_driver_api);

/**
 * @brief Hook ISR to & Enable the FlexTimer IRQ.
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_ft_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 FT isr */
	SYS_LOG_DBG("Register Flex Timer ISR.");

	IRQ_CONNECT(SPI_IRQ(INT_SRC_FLEX_TIMER),
		INT_SRC_FLEX_TIMER_PRIORITY_LEVEL, ft_isr, DEVICE_GET(bcm5820x_ft), 0);

	irq_enable(SPI_IRQ(INT_SRC_FLEX_TIMER));
	SYS_LOG_DBG("Enabled Flex Timer Interrupt & Installed handler.");
}
