
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

/*
 * @file sc.h
 * @brief Public APIs for the Smart card host drivers
 */

#ifndef _SC_H_
#define _SC_H_

#include <kernel.h>
#include <device.h>
#include <sc/sc_datatypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef s32_t (*sc_api_channel_open)(struct device *dev, u32_t channel,
				     struct sc_channel_param *ch_param);

typedef s32_t (*sc_api_channel_close)(struct device *dev, u32_t channel);

typedef s32_t (*sc_api_channel_deactivate)(struct device *dev, u32_t channel);

typedef s32_t (*sc_api_channel_enable_interrupts)(struct device *dev,
								 u32_t channel);

typedef s32_t (*sc_api_channel_param_set)(struct device *dev, u32_t channel,
					  struct sc_channel_param *ch_param);

typedef s32_t (*sc_api_channel_param_get)(struct device *dev, u32_t channel,
					  enum param_type type,
					  struct sc_channel_param *ch_param);

typedef s32_t (*sc_api_channel_set_vcc_level)(struct device *dev, u32_t channel,
					      enum sc_vcc_level);

typedef s32_t (*sc_api_channel_status_get)(struct device *dev, u32_t channel,
					   struct sc_status *status);

typedef s32_t (*sc_api_channel_time_set)(struct device *dev,
					 struct sc_channel_param *ch_param,
					 enum time_type type,
					 struct sc_time *time);

typedef s32_t (*sc_api_channel_read_atr)(struct device *dev, bool from_hw,
					 struct sc_transceive *trx);

typedef s32_t (*sc_api_channel_transceive)(struct device *dev,
					   enum pdu_type pdu,
					   struct sc_transceive *trx);

typedef s32_t (*sc_api_channel_interface_reset)(struct device *dev,
						u32_t channel,
						enum sc_reset_type type);

typedef s32_t (*sc_api_channel_card_reset)(struct device *dev, u32_t channel,
					   u8_t decode_atr);

typedef s32_t (*sc_api_channel_perform_pps)(struct device *dev, u32_t channel);

typedef s32_t (*sc_api_channel_card_power_up)(struct device *dev,
					      u32_t channel);

typedef s32_t (*sc_api_channel_card_power_down)(struct device *dev,
						u32_t channel);

typedef void (*sc_api_card_detect_cb)(void *arg);

typedef s32_t (*sc_api_channel_card_detect_cb_set)(struct device *dev,
						   u32_t channel,
						   enum sc_card_present status,
						   sc_api_card_detect_cb cb);

typedef s32_t (*sc_api_channel_set_card_type)(struct device *dev, u32_t channel,
					     u32_t card_type);

typedef s32_t (*sc_api_channel_card_detect)(struct device *dev, u32_t channel,
					    enum sc_card_present status,
					    bool block);

typedef s32_t (*sc_api_channel_apdu_commands)(struct device *dev, u32_t channel,
					      u32_t apducmd, u8_t *buf,
					      u32_t len);

typedef s32_t (*sc_api_channel_apdu_set_objects)(struct device *dev,
						 u32_t channel, s8_t *obj_id,
						 u32_t obj_id_len);

typedef s32_t (*sc_api_channel_apdu_get_objects)(struct device *dev,
						 u32_t channel, s8_t *obj_id,
						 u32_t obj_id_len);

typedef s32_t (*sc_api_adjust_param_with_f_d)(struct device *dev,
					      struct sc_channel_param *ch_param,
					      u8_t f_val, u8_t d_val);

struct sc_driver_api {
	sc_api_channel_open channel_open;
	sc_api_channel_close channel_close;
	sc_api_channel_deactivate channel_deactivate;
	sc_api_channel_enable_interrupts channel_enable_interrupts;
	sc_api_channel_param_set channel_param_set;
	sc_api_channel_param_get channel_param_get;
	sc_api_channel_set_vcc_level channel_set_vcc_level;
	sc_api_channel_status_get channel_status_get;
	sc_api_channel_time_set channel_time_set;
	sc_api_channel_read_atr channel_read_atr;
	sc_api_channel_interface_reset channel_interface_reset;
	sc_api_channel_interface_reset channel_card_reset;
	sc_api_channel_perform_pps channel_perform_pps;
	sc_api_channel_card_power_up channel_card_power_up;
	sc_api_channel_card_power_down channel_card_power_down;
	sc_api_channel_transceive channel_transceive;
	sc_api_channel_card_detect_cb_set channel_card_detect_cb_set;
	sc_api_channel_card_detect channel_card_detect;
	sc_api_channel_set_card_type channel_set_card_type;
	sc_api_channel_apdu_commands channel_apdu_commands;
	sc_api_channel_apdu_set_objects channel_apdu_set_objects;
	sc_api_channel_apdu_get_objects channel_apdu_get_objects;
	sc_api_adjust_param_with_f_d adjust_param_with_f_d;
};

/**
 * @brief open the channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Channel to open
 * @param ch_param Pointer of channel parameters to set. If NULL
 *                 default parameters will be applied.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_open(struct device *dev,  u32_t channel,
				    struct sc_channel_param *ch_param)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_open(dev, channel, ch_param);
}

/**
 * @brief close the channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Channel to close
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_close(struct device *dev,  u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_close(dev, channel);
}

/**
 * @brief Deactivate the channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Channel to deactivate
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_deactivate(struct device *dev,  u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_deactivate(dev, channel);
}

/**
 * @brief Enable interrupts for the channel
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Channel number
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_enable_interrupts(struct device *dev,
						 u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_enable_interrupts(dev, channel);
}

/**
 * @brief Change the channel parameters
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 * @param ch_param Channel parameters to configure
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_param_set(struct device *dev,  u32_t channel,
					 struct sc_channel_param *ch_param)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_param_set(dev, channel, ch_param);
}

/**
 * @brief Get the channel parameters
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param type Param type to get. Current/negotiated/default settings
 * @param ch_param Pointer of channel parameters to get
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_param_get(struct device *dev,  u32_t channel,
					 enum param_type type,
					 struct sc_channel_param *ch_param)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_param_get(dev, channel, type, ch_param);
}

/**
 * @brief Set the VCC level
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param level Vcc level
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_set_vcc_level(struct device *dev,  u32_t channel,
					     enum sc_vcc_level level)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_set_vcc_level(dev, channel, level);
}

/**
 * @brief Get the channel status
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param status Pointer of status to get
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_status_get(struct device *dev,  u32_t channel,
					  struct sc_status *status)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_status_get(dev, channel, status);
}

/**
 * @brief Perform PPS (Protocol and Parameters Selection)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_perform_pps(struct device *dev,  u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_perform_pps(dev, channel);
}

/**
 * @brief Power up the card
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_card_power_up(struct device *dev,  u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_card_power_up(dev, channel);
}

/**
 * @brief Power down the card
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_card_power_down(struct device *dev,
					       u32_t channel)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_card_power_down(dev, channel);
}

/**
 * @brief transceive data through the channel
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param pdu Pdu type TPDU/APDU
 * @param trx Transceive info
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_transceive(struct device *dev,
					  enum pdu_type pdu,
					  struct sc_transceive *trx)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_transceive(dev, pdu, trx);
}

/**
 * @brief Reset the channel
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param type Reset type cold or warm
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_interface_reset(struct device *dev,
					       u32_t channel,
					       enum sc_reset_type type)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_interface_reset(dev, channel, type);
}

/**
 * @brief Reset the card
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param decode_atr Decode the atr if true
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_card_reset(struct device *dev, u32_t channel,
					  u8_t decode_atr)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_card_reset(dev, channel, decode_atr);
}

/**
 * @brief Card status callback set
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param status Either for inserted or removed
 * @param cb Callback function for card status change
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_card_detect_cb_set(struct device *dev,
						  u32_t channel,
						  enum sc_card_present status,
						  sc_api_card_detect_cb cb)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_card_detect_cb_set(dev, channel, status, cb);
}

/**
 * @brief Card detect
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param status Check for either inserted or removed
 * @param block Block for insert/remove
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_card_detect(struct device *dev, u32_t channel,
					   enum sc_card_present status,
					   bool block)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_card_detect(dev, channel, status, block);
}

/**
 * @brief Channel interface time parameter set
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param ch_param Pointer to the channel param
 * @param type Timer type
 * @param time time structure pointer
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_time_set(struct device *dev,
					struct sc_channel_param *ch_param,
					enum time_type type,
					struct sc_time *time)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_time_set(dev, ch_param, type, time);
}

/**
 * @brief Channel ATR read
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param from_hw Read from hardware
 * @param trx trx pointer. Only rx pointer will be filled.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_read_atr(struct device *dev, bool from_hw,
					struct sc_transceive *trx)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_read_atr(dev, from_hw, trx);
}

/**
 * @brief Set card type
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param card_type Card type.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_set_card_type(struct device *dev,
					     u32_t channel, u32_t card_type)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_set_card_type(dev, channel, card_type);
}

/**
 * @brief Set apdu object
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param obj_id Object id pointer
 * @param obj_id_len Object id length
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_apdu_set_objects(struct device *dev,
						u32_t channel, s8_t *obj_id,
						u32_t obj_id_len)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_apdu_set_objects(dev, channel, obj_id, obj_id_len);
}

/**
 * @brief Get apdu object
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param obj_id Object id pointer
 * @param obj_id_len Object id length
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_apdu_get_objects(struct device *dev,
						u32_t channel, s8_t *obj_id,
						u32_t obj_id_len)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_apdu_get_objects(dev, channel, obj_id, obj_id_len);
}

/**
 * @brief Apdu commands
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel Numeric identification of the channel to configure
 * @param apducmd Apdu Command
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline s32_t sc_channel_apdu_commands(struct device *dev, u32_t channel,
					   u32_t apducmd, u8_t *buf, u32_t len)
{
	const struct sc_driver_api *api = dev->driver_api;

	return api->channel_apdu_commands(dev, channel, apducmd, buf, len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _SC_H_ */
