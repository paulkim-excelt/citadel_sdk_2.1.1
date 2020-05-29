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
#ifndef __FLEXTIMER_H__
#define __FLEXTIMER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define reg_bits_read(a, off, nbits) \
	(((*(volatile u32_t *)(a)) & (((1 << (nbits)) - 1) << (off))) >> (off))
#define reg_bits_clr(a, off, nbits) \
	do { *(volatile u32_t *)(a) &= ~(((1 << (nbits)) - 1) << (off)); } \
	while (0)
#define reg_bits_set(a, off, nbits, v) \
	do { reg_bits_clr(a, off, nbits); \
	*(volatile u32_t *)(a) |= (((1 << (nbits)) - 1) & (v)) << (off); } \
	while (0)

/* FIXME: Move to proper place, should not be here*/
s32_t ft_decode(u16_t *p, s32_t *size, s32_t ch, u8_t *out, s32_t outlen);

/**
 * @brief   Flex Timer number enum
 * @details Three Flex Timers 0 to 2.
 */
typedef enum {
	ft_0 = 0, /*< ft 0 */
	ft_1 = 1, /*< ft 1 */
	ft_2 = 2, /*< ft 2 */
	ft_max = 3, /*< max ft */
} ft_no;

/**
 * @brief   Flextimer event
 * @details Define Flextimer isr callback event
 */
typedef enum {
	FT_START = 0, /*< start of swipe event */
	FT_END = 1, /*< end of swipe event */
	FT_DATA = 2, /*< data ready event */
} ft_event;

/**
 * @brief   Flextimer IOMUX Mode
 * @details Define Flextimer IOMUX Mode
 */
typedef enum {
	FT_IOMUX_MODE1 = 0, /*< MFIO 12, 13 & 20 used for FT */
	FT_IOMUX_MODE2 = 1, /*< MFIO 22, 23 & 24 used for FT */
} ft_iomux_mode;

/**
 * @typedef ft_irq_callback_t
 * @brief FT Interrupt Handler prototype.
 *
 * @param device struct for the FT device.
 */
typedef void (*ft_irq_callback_t)(struct device *dev, ft_no ft, s32_t count, ft_event e);

/**
 * @typedef ft_irq_config_func_t
 * @brief For configuring IRQs for FT.
 *
 * @internal (device struct for the FT device.)
 */
typedef void (*ft_irq_config_func_t)(struct device *dev);

/**
 * @brief FT device configuration.
 *
 * @param Function for configuring IRQs
 */
struct ft_device_config {
	ft_irq_config_func_t irq_config_func;
};

/** @brief Driver API structure. */
struct ft_driver_api {
	/** Init FT Devices */
	s32_t (*init)(struct device *dev);

	/** Power Down FT Devices */
	s32_t (*powerdown)(struct device *dev);

	/** Get FT data from hardware fifo */
	s32_t (*get_data)(struct device *dev, ft_no ftno, u16_t *buf, s32_t len);

	/** Empty hardware fifo */
	s32_t (*empty_fifo)(struct device *dev, ft_no ftno);

	/** Set the callback function - Register ISR for FT IRQ */
	s32_t (*set_ft_irq_callback)(struct device *dev, ft_no ftno, ft_irq_callback_t cb);

	/** Disable Flex Timer */
	s32_t (*disable)(struct device *dev);

	/** Enable Flex Timer */
	s32_t (*enable)(struct device *dev, s32_t inten);

	/** Configure Flex Timer IOMUX Mode */
	s32_t (*set_iomux_mode)(struct device *dev, ft_iomux_mode ftiomode);

	/** Wait for Start of Swipe Event */
	s32_t (*wait_for_sos)(struct device *dev);
};

/**
 * @brief   	Init and enable flextimer
 * @details 	Init and enable flextimer
 * @param[in]   dev	FT device object
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_init(struct device *dev)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->init)
		return api->init(dev);
	return 0;
}

/**
 * @brief Power Down Flex Timer Device
 * @details Power Down Flex Timer Device
 * @param[in] dev pointer to ft device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t ft_powerdown(struct device *dev)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->powerdown)
		return api->powerdown(dev);
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
static inline s32_t ft_get_data(struct device *dev, ft_no ft, u16_t *buf, s32_t len)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->get_data)
		return api->get_data(dev, ft, buf, len);
	return 0;
}

/**
 * @brief   	Set FT IRQ Callback
 * @details 	Set FT IRQ Callback for a desired FT
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_set_ft_irq_callback(struct device *dev, ft_no ft, ft_irq_callback_t cb)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->set_ft_irq_callback)
		return api->set_ft_irq_callback(dev, ft, cb);
	return 0;
}

/**
 * @brief   	Disable flextimer
 * @details 	Disable all 3 flextimer
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_disable(struct device *dev)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->disable)
		return api->disable(dev);
	return 0;
}

/**
 * @brief   	Enable flextimer
 * @details 	Enable all 3 flextimer
 * @param[in]   inten 	enable interrupt
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_enable(struct device *dev, s32_t inten)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->enable)
		return api->enable(dev, inten);
	return 0;
}

/**
 * @brief   	Set flextimer IOMUX Mode
 * @details 	Choose which MFIOs to be used for FT
 * @param[in]   dev FT device object
 * @param[in]   ftiomode IO Mux Mode for FT
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_set_iomux_mode(struct device *dev, ft_iomux_mode ftiomode)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->set_iomux_mode)
		return api->set_iomux_mode(dev, ftiomode);
	return 0;
}

/**
 * @brief   	Wait for Start of Swipe Event
 * @details 	Wait for Start of Swipe Event
 * @param[in]   dev FT device object
 * @retval  	0:OK
 * @retval  	-1:ERROR
 */
static inline s32_t ft_wait_for_sos(struct device *dev)
{
	const struct ft_driver_api *api = dev->driver_api;

	if (api->wait_for_sos)
		return api->wait_for_sos(dev);
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
