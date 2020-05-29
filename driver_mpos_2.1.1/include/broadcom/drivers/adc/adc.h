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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <device.h>

#define reg_bits_read(a, off, nbits) \
	(((*(volatile u32_t *)(a)) & (((1 << (nbits)) - 1) << (off))) >> (off))
#define reg_bits_clr(a, off, nbits) \
	do { *(volatile u32_t *)(a) &= ~(((1 << (nbits)) - 1) << (off)); } \
	while (0)
#define reg_bits_set(a, off, nbits, v) \
	do { reg_bits_clr(a, off, nbits); \
	*(volatile u32_t *)(a) |= (((1 << (nbits)) - 1) & (v)) << (off); } \
	while (0)

#define MAX_ADC_CLK_DIV 1023

/**
 * @brief   ADC number enum
 * @details Three ADC form 0 to 2.
 */
typedef enum {
	adc_0 = 0, /*< adc 0 */
	adc_1 = 1, /*< adc 1 */
	adc_2 = 2, /*< adc 2 */
	adc_max = 3, /*< max adc */
} adc_no;

/**
 * @brief   ADC channel number enum
 * @details Every ADC have 2 channel: 0 - 1.
 */
typedef enum {
	adc_ch0 = 0, /*< adc channel 0 */
	adc_ch1 = 1, /*< adc channel 1 */
	adc_chmax = 2, /*< max adc channel */
} adc_ch;

/**
 * @typedef adc_irq_callback_t
 * @brief ADC Interrupt Handler prototype.
 *
 * @param device struct for the ADC device.
 */
typedef void (*adc_irq_callback_t)(struct device *dev,
		adc_no adc, adc_ch ch, s32_t count);

/**
 * @typedef adc_irq_config_func_t
 * @brief For configuring IRQs on ADC.
 *
 * @internal (device struct for the ADC device.)
 */
typedef void (*adc_irq_config_func_t)(struct device *dev);

/**
 * @brief ADC device configuration.
 *
 * @param base Memory mapped base address
 */
struct adc_device_config {
	adc_irq_config_func_t irq_config_func;
};

/** @brief Driver API structure. */
struct adc_driver_api {
	/** Init Specific ADC Device */
	s32_t (*init)(struct device *dev, adc_no adcno);

	/** Init All ADC Devices */
	s32_t (*init_all)(struct device *dev);

	/** Power Down all ADC Devices */
	s32_t (*powerdown)(struct device *dev);

	/** Set adc sample rate */
	s32_t (*set_rate)(struct device *dev, adc_no adcno, s32_t div);

	/** Get oneshot adc data */
	s32_t (*get_snapshot)(struct device *dev, adc_no adcno, adc_ch ch,
		u16_t *value);

	/** Enable adc to sample n rounds, then stop. */
	s32_t (*enable_nrounds)(struct device *dev, adc_no adcno, adc_ch ch,
		s32_t rounds);

	/** Get adc data from hardware fifo */
	s32_t (*get_data)(struct device *dev, adc_no adcno, adc_ch ch,
		u16_t *buf, s32_t len);

	/** Empty hardware fifo */
	s32_t (*empty_fifo)(struct device *dev, adc_no adcno, adc_ch ch);

	/** Enable adc TDM(continues) mode  */
	s32_t (*enable_tdm)(struct device *dev, adc_no adcno, adc_ch ch,
		adc_irq_callback_t cb);

	/** Disable adc TDM(continues) mode */
	s32_t (*disable_tdm)(struct device *dev, adc_no adcno, adc_ch ch);

	/** Set the callback function - Register ISR for ADC IRQ */
	void (*set_adc_irq_callback)(struct device *dev,
		adc_no adcno, adc_ch ch, adc_irq_callback_t cb);

	/*  Set adc Clock Divisor */
	s32_t (*set_clk_div)(struct device *dev, u32_t lodiv, u32_t hidiv);

	/*  Get adc Clock Divisor */
	s32_t (*get_clk_div)(struct device *dev);
};

/**
 * @brief Initialize a particular ADC Device
 * @details Initialize a specific ADC Device
 * @param[in] dev pointer to adc device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_init(struct device *dev, adc_no adcno)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->init)
		return api->init(dev, adcno);
	return 0;
}

/**
 * @brief Initialize all
 * @details Power Down Entire ADC Device
 * @param[in] dev pointer to adc device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_init_all(struct device *dev)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->init_all)
		return api->init_all(dev);
	return 0;
}

/**
 * @brief Power Down All ADC Devices
 * @details Power Down All ADC Devices
 * @param[in] dev pointer to adc device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_powerdown(struct device *dev)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->powerdown)
		return api->powerdown(dev);
	return 0;
}

/**
 * @brief Set adc Clock Divisor
 * @details Set adc Clock Divisor
 * @param[in] dev pointer to adc device
 * @param[in] lodiv Divisor for the low half of adc clock
 * @param[in] hidiv Divisor for the high half of adc clock
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_set_clk_div(struct device *dev, u32_t lodiv, u32_t hidiv)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->set_clk_div)
		return api->set_clk_div(dev, lodiv, hidiv);
	return 0;
}

/**
 * @brief Get adc Clock Divisor
 * @details Get adc Clock Divisor
 * @param[in] dev pointer to adc device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_get_clk_div(struct device *dev)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->get_clk_div)
		return api->get_clk_div(dev);
	return 0;
}

/**
 * @brief Set adc sample rate
 * @details Set adc sample rate
 * @param[in] dev Pointer to adc device
 * @param[in] adcno adc number
 * @param[in] div clock divider
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t adc_set_rate(struct device *dev, adc_no adcno, s32_t div)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->set_rate)
		return api->set_rate(dev, adcno, div);
	return 0;
}

/**
 * @brief Get oneshot adc data
 * @details Get current adc channel data
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @param[out]  value	12bits adc data
 * @retval		0:OK
 * @retval		-1:ERROR
 */
static inline s32_t adc_get_snapshot(struct device *dev, adc_no adcno, adc_ch ch,
		u16_t *value)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->get_snapshot)
		return api->get_snapshot(dev, adcno, ch, value);
	return 0;
}

/**
 * @brief		Enable adc to sample n rounds
 * @details		Enable adc to sample n rounds, then stop.
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @param[in]	rounds 	[1-62]: round number
 * @retval		0:OK
 * @retval		-1:ERROR
 */
static inline s32_t adc_enable_nrounds(struct device *dev, adc_no adcno,
		adc_ch ch, s32_t rounds)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->enable_nrounds)
		return api->enable_nrounds(dev, adcno, ch, rounds);
	return 0;
}

/**
 * @brief		Get adc data
 * @details		Get adc data form hardware fifo
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @param[out]	buf		input buffer
 * @param[in]	len		input buffer length
 * @retval		number of data copied to buffer
 */
static inline s32_t adc_get_data(struct device *dev, adc_no adcno, adc_ch ch,
		u16_t *buf, s32_t len)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->get_data)
		return api->get_data(dev, adcno, ch, buf, len);
	return 0;
}

/**
 * @brief		Empty hardware fifo
 * @details		Empty hardware fifo
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @retval		0:OK
 * @retval		-1:ERROR
 */
static inline s32_t adc_empty_fifo(struct device *dev, adc_no adcno, adc_ch ch)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->empty_fifo)
		return api->empty_fifo(dev, adcno, ch);
	return 0;
}

/**
 * @brief		Enable adc TDM(continues) mode
 * @details		Enable adc TDM(continues) mode
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @param[in]   cb		optional interrupt callback.
 *              If NULL, interrupt won't enable
 * @retval		0:OK
 * @retval		-1:ERROR
 */
static inline s32_t adc_enable_tdm(struct device *dev, adc_no adcno, adc_ch ch,
		adc_irq_callback_t cb)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->enable_tdm)
		return api->enable_tdm(dev, adcno, ch, cb);
	return 0;
}

/**
 * @brief		Disable adc TDM(continues) mode
 * @details		Disable adc TDM(continues) mode
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno	adc number
 * @param[in]   ch		channel number
 * @retval		0:OK
 * @retval		-1:ERROR
 */
static inline s32_t adc_disable_tdm(struct device *dev, adc_no adcno, adc_ch ch)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->disable_tdm)
		return api->disable_tdm(dev, adcno, ch);
	return 0;
}

/****************************************************************
 * Interrupt Service Routines Installer Wrappers:
 * These routines invoke the actual (device specific) ISR
 * installers to install the ISRs provided by user.
 ****************************************************************/
/**
 * @brief Set the callback function - Register ISR callback for ADC IRQ
 * @details Set the callback function - Register ISR for ADC IRQ
 * @param[in]   dev	Pointer to ADC device
 * @param[in]   adcno ADC Number
 * @param[in]   ch Channel Number
 * @param[in]   cb callback function
 * @retval None
 */
static inline void adc_set_adc_irq_callback(struct device *dev,
		adc_no adcno, adc_ch ch, adc_irq_callback_t cb)
{
	const struct adc_driver_api *api = dev->driver_api;

	if (api->set_adc_irq_callback)
		return api->set_adc_irq_callback(dev, adcno, ch, cb);
}

#ifdef __cplusplus
}
#endif

#endif
