/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/* @file keypad.h
 *
 * @brief Keypad APIs.
 */

#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef s32_t (*keypad_api_read)(struct device *dev, u8_t *key);
typedef s32_t (*keypad_api_read_nonblock)(struct device *dev, u8_t *buf,
					  u8_t len);
typedef void (*keypad_callback)();
typedef s32_t (*keypad_api_callback_register)(struct device *dev,
					      keypad_callback cb);
typedef s32_t (*keypad_api_callback_unregister)(struct device *dev);

struct keypad_driver_api {
	keypad_api_read read;
	keypad_api_read_nonblock read_nonblock;
	keypad_api_callback_register callback_register;
	keypad_api_callback_unregister callback_unregister;
};

/**
 * @brief Single key read from keypad
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param key - pointer to key
 *
 * @return 0
 *
 * @note The key ptr will be populated with the key from the key buffer. If the
 *       buffer is empty, it will wait for key press
 */
static inline s32_t keypad_read(struct device *dev, u8_t *key)
{
	const struct keypad_driver_api *api = dev->driver_api;

	return api->read(dev, key);
}

/**
 * @brief Keypad key read nonblock
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf - pointer to key receive buffer
 * @param len - length of the buffer
 *
 * @return Key count in the buffer
 *
 * @note The buffer will be filled with the detected keys from the key buffer
 *       and it is a nonblocking call
 */
static inline s32_t keypad_read_nonblock(struct device *dev, u8_t *buf,
					 u8_t len)
{
	const struct keypad_driver_api *api = dev->driver_api;

	return api->read_nonblock(dev, buf, len);
}

/**
 * @brief Keypad callback register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb - callback function
 *
 * @return 0
 */
static inline s32_t keypad_callback_register(struct device *dev,
					     keypad_callback cb)
{
	const struct keypad_driver_api *api = dev->driver_api;

	return api->callback_register(dev, cb);
}

/**
 * @brief Keypad callback unregister
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0
 */
static inline s32_t keypad_callback_unregister(struct device *dev)
{
	const struct keypad_driver_api *api = dev->driver_api;

	return api->callback_unregister(dev);
}
#ifdef __cplusplus
}
#endif

#endif /* _KEYPAD_H_ */
