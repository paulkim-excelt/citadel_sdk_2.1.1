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
#ifndef _PM_H_
#define _PM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <device.h>
#include "zephyr/types.h"

/**
 * @brief Reset system when A7 wdog timeout
 */
#define MPROC_PM__A7_WDOG_RESET

/**
 * @brief Forward smbus interrupt on M0 to A7
 */
#define MPROC_PM__A7_SMBUS_ISR

/**
 * @brief Additional 187.5MHz A7 clock for AES DPA
 */
#define MPROC_PM__DPA_CLOCK

/**
 * @brief ADVANCED Sleep, power off more components than basic Sleep
 */
#define MPROC_PM__ADVANCED_S
#ifdef MPROC_PM__ADVANCED_S
#define MPROC_PM__ADV_S_POWER_OFF_PLL  /**< power off PLL and enter ULP */
#endif

/**
 * @brief ADVANCED DeepSleep, power off more components than basic DeepSleep
 */
#define MPROC_PM__ADVANCED_DS

/**
 * Power Optimization 2: Turn 26MHz Crystal Off
 *
 * @brief Switch M0 clock source from xtal to SPL or BBL, and power down xtal
 * @details M0 becomes very slow in ULP mode, don't use delay too much
 *
 * This feature makes DRIPS unstable in that after few attempts system fails
 * to wake up from DRIPS.
 */
#define MPROC_PM__ULTRA_LOW_POWER

#define CITADEL_WFI_SLEEP_ENABLED
#undef CITADEL_WFI_SLEEP_ENABLED

/**
 * Power Optimization 3: Turn NFC Power Off
 *
 * This appears benign in terms of DRIPS stability, but will require
 * stress testing once the facility to use NFC as a wake-up source is available.
 */
#define CITADEL_A7_POWER_OFF_NFC
#undef CITADEL_A7_POWER_OFF_NFC

/**
 * Power Optimization 4: Disable A7 Clocks
 *
 * This appears benign in terms of DRIPS stability, but will require
 * stress testing for variety of wake-up modes.
 */
#define CITADEL_A7_DISABLE_CLOCKS

/**
 * Power Optimization 5: Turn ADCs Power Off
 *
 * This appears benign in terms of DRIPS stability, but will require
 * stress testing with applications using ADCs such as Magnetic Strip reader.
 */
#define CITADEL_A7_POWER_OFF_ADC

/**
 * Power Optimization 6: Power Off PLLs
 *
 * Can't wake up from DRIPS when this is enabled.
 */
#define CITADEL_A7_POWER_OFF_PLLS

/**
 * Power Optimization 7: Turn A7 GPIO Ring Power
 *
 * Per se, appears benign, but needs stress testing
 * with production board, where the desired hardware controls
 * (i.e. controlling IO ring power with AON GPIO #2 for CS or
 * equivalent in MPOS applications.) is present.
 */
#define CITADEL_A7_GPIO_RING_POWER_OFF

/**
 * Power Optimization 8: Turn A7 USB Phy power Off
 *
 * This appears benign in terms of DRIPS stability, but will require
 * stress testing for variety of wake-up modes, mainly wake up
 * upon USB Host interrupt, which will require a Finger Print scanner
 * or a thumb drive etc. and wake up upon USB device Suspend / Resume
 * sequence.
 */
#define CITADEL_A7_USB_PHY_POWER_OFF

/**
 * @brief Clear AON_GPIO interrupt in M0 ISR to avoid rare race condition
 *        which M0 will keep calling AON_GPIO ISR
 */
#define IPROC_PM_CLEAR_AON_GPIO_INTR_IN_M0_ISR

/**
 * @brief Wait for GP_DATA_IN reg stable in AON GPIO ISR
 * @details If GPIO not pull up/down, the data_in reg will not be stable in ISR
 */
#define AON_GPIO_EDGE_SENSITIVE_WORKAROUND
#undef AON_GPIO_EDGE_SENSITIVE_WORKAROUND

/**
 * @brief Wakeup system when tamper attack occurs
 */
#define MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
#define IPROC_PM_GATHER_TAMPER_INFO_IN_M0_ISR
#define IPROC_PM_CLEAR_TAMPER_INTR_IN_M0_ISR

/**
 * @brief Wakeup system when specified RTC timeout occurs
 */
#define MPROC_PM_USE_RTC_AS_WAKEUP_SRC

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
#define MPROC_PM_DEFAULT_RTC_WAKEUP_TIME	5
#else
#define MPROC_PM_DEFAULT_RTC_WAKEUP_TIME	0
#endif

#if defined(MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC) || defined(MPROC_PM_USE_RTC_AS_WAKEUP_SRC) || defined(MPROC_PM__VDS)
#define MPROC_PM_ACCESS_BBL
#endif

/**
 * @brief Support FastXIP in system wakeup
 * @details When FastXIP enabled, wakeup progress directly restore
 *		SMAU config, and SBI bypassed
 */
#define MPROC_PM_FASTXIP_SUPPORT
/* FIXME:
 * This is a temporary fix to get going with SMAU configuration saving
 * carried out by the AAI itself.
 * If & When Intermediate bootloader comes along to do the saving of the
 * SMAU config this flag will no longer be required.
 *
 * #undef MPROC_PM_FASTXIP_SUPPORT
 */
#ifdef MPROC_PM_FASTXIP_SUPPORT
/**< A0 chip has trouble to access BBL for fast XIP.  If it is a A0 chip,
 * we have to disable the fast XIP mode and simply reset M3.
 */
#define MPROC_PM_FASTXIP_DISABLED_ON_A0
#undef MPROC_PM_FASTXIP_DISABLED_ON_A0

/**< SMAU info saved in scratch ram*/
#define MPROC_PM_FASTXIP_SCRATCH_RAM
#endif

/**
 * @brief Let A7 wait M0's job done in PM progress
 */
#define MPROC_PM_WAIT_FOR_MCU_JOB_DONE
#undef MPROC_PM_WAIT_FOR_MCU_JOB_DONE

/**
 * @brief Use a special IDRAM to circularly log M0 status for PM
 * debug
 * #define MPROC_PM_TRACK_MCU_STATE
 */

/**
 * @brief Citadel PM mode enum
 * @details All supported PM modes
 */
typedef enum mproc_pm_mode {
	MPROC_PM_MODE__RUN_1000 = 0,    /**< run at 1000MHz */
	MPROC_PM_MODE__RUN_500,         /**< run at 500MHz */
	MPROC_PM_MODE__RUN_200,         /**< run at 200MHz */
/**
 * This is only for Software DPA Implementation, which requires
 * programming the CPU A7's Gen PLL to 187MHz. The other option
 * is hardware DPA (using the Crypto Block), which uses the System
 * Gen PLL residing inside CRMU. Please make sure that CPU Pll's are
 * programmed for software DPA.
 *
 * Further there should be a dedicated compile flag to choose Software DPA.
 */
#ifdef MPROC_PM__DPA_CLOCK
	MPROC_PM_MODE__RUN_187,     /**< run at 187.5MHz*/
#endif
	MPROC_PM_MODE__SLEEP,       /**< WFI sleep mode - Test Purpose Only */
	MPROC_PM_MODE__DRIPS,       /**< DRIPS mode */
	MPROC_PM_MODE__DEEPSLEEP,  /**< deepsleep mode */
	MPROC_PM_MODE__END
} MPROC_PM_MODE_e;

/**
 * What is the purpose of this?
 * Ans. These are GPIOs on CRMU, which can wake-up the CRMU and
 * subsequently, if needed the A7. There are No AON GPIOs on MPROC.
 * There are 11 AON GPIOs only on CRMU.
 *
 */
typedef enum mproc_aon_gpio_id {
	MPROC_AON_GPIO0 = 0,
	MPROC_AON_GPIO1 = 1,
	MPROC_AON_GPIO2 = 2,
	MPROC_AON_GPIO3 = 3,
	MPROC_AON_GPIO4 = 4,
	MPROC_AON_GPIO5 = 5,
	MPROC_AON_GPIO6 = 6,
	MPROC_AON_GPIO7 = 7,
	MPROC_AON_GPIO8 = 8,
	MPROC_AON_GPIO9 = 9,
	MPROC_AON_GPIO10 = 10,
	MPROC_AON_GPIO_END,
} MPROC_AON_GPIO_ID_e;

typedef enum mproc_usb_id {
	MPROC_USB0_WAKE  = 0,
	MPROC_USB0_FLTR = 1,
	MPROC_USB1_WAKE  = 2,
	MPROC_USB1_FLTR = 3,
	MPROC_USB_END,
} MPROC_USB_ID_e;

/**
 * @brief Default PM mode
 * @details Default PM mode, should be MPROC_PM_MODE__RUN_*
 */
#ifdef CONFIG_BCM58202
#define MPROC_DEFAULT_PM_MODE MPROC_PM_MODE__RUN_200
#else
#define MPROC_DEFAULT_PM_MODE MPROC_PM_MODE__RUN_200
#endif

/**
 * @typedef pm_aon_gpio_irq_callback_t
 * @brief AON GPIO Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_aon_gpio_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_nfc_lptd_irq_callback_t
 * @brief NFC LPTD Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_nfc_lptd_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_usb_irq_callback_t
 * @brief USB Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_usb_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_ec_cmd_irq_callback_t
 * @brief EC Command Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_ec_cmd_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_tamper_irq_callback_t
 * @brief Tamper Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_tamper_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_aon_gpio_run_irq_callback_t
 * @brief AON gpio Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_aon_gpio_run_irq_callback_t)(struct device *dev,
					       u32_t int_stat);

/**
 * @typedef pm_smbus_irq_callback_t
 * @brief SMBUS Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_smbus_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_rtc_irq_callback_t
 * @brief RTC Interrupt Handler prototype.
 *
 * @param device struct for the PM device.
 */
typedef void (*pm_rtc_irq_callback_t)(struct device *dev);

/**
 * @typedef pm_irq_config_func_t
 * @brief For configuring IRQ on PM.
 *
 * @internal (device struct for the PM device.)
 */
typedef void (*pm_irq_config_func_t)(struct device *dev);

/**
 * @brief PM device configuration.
 *
 * @param base Memory mapped base address
 */
struct pm_device_config {
	u8_t *base;
	pm_irq_config_func_t irq_config_func;
};

/** @brief Driver API structure. */
struct pm_driver_api {
	/** Put the system into Default PM Mode */
	s32_t (*enter_default_state)(struct device *dev, u32_t init_phase);

	/** Put system into target PM mode */
	s32_t (*enter_target_state)(struct device *dev, MPROC_PM_MODE_e state);

	/** Get the current PM state/mode
	 */
	MPROC_PM_MODE_e (*get_pm_state)(struct device *dev);

	/** Get the string to print describing current PM state/mode,
	 * i.e. "run1000", "run200" ...  "deepsleep"
	 */
	char* (*get_pm_state_desc)(struct device *dev, MPROC_PM_MODE_e state);

	/** Get the current configured PM auto wakeup time for RTC */
	u32_t (*get_rtc_wakeup_time)(struct device *dev);

	/** Set a particular AON GPIO as a wake-up source
	 * There are 11 AON GPIOs in total
	 */
	u32_t (*set_aongpio_wakeup_source)(struct device *dev,
		MPROC_AON_GPIO_ID_e aongpio, u32_t int_type, u32_t int_de, u32_t int_edge, u32_t enable);

	/** Set NFC LPTD as a Wake Up Source */
	u32_t (*set_nfc_lptd_wakeup_source)(struct device *dev);

	/** Set USB as a Wake Up Source - Two USBs in total */
	u32_t (*set_usb_wakeup_source)(struct device *dev, MPROC_USB_ID_e usbno);

	/** Set EC Command as a Wake Up Source */
	u32_t (*set_ec_cmd_wakeup_source)(struct device *dev);

	/** Set SMBUS as a Wake Up Source */
	u32_t (*set_smbus_wakeup_source)(struct device *dev);

	/** Set RTC as a wakeup source */
	u32_t (*set_rtc_wakeup_source)(struct device *dev, u32_t sec);

	/** Set Tamper as a wakeup source */
	u32_t (*set_tamper_wakeup_source)(struct device *dev);

	/** Set the callback function - Register ISR for AON GPIO
	 * This is for AON GPIO. e.g. to handle push button press event
	 */
	void (*set_aon_gpio_irq_callback)(struct device *dev,
		pm_aon_gpio_irq_callback_t cb, MPROC_AON_GPIO_ID_e aongpio);

	/** Set the callback function - Register ISR for NFC LPTD
	 * This is for NFC LPTD. e.g. When presence of NFC device detected
	 * Low Power Target Detection
	 */
	void (*set_nfc_lptd_irq_callback)(struct device *dev,
		pm_nfc_lptd_irq_callback_t cb);

	/** Set the callback function - Register ISR for USB
	 * This is for USB. e.g. insertion of usb connector event
	 */
	void (*set_usb_irq_callback)(struct device *dev,
		pm_usb_irq_callback_t cb, MPROC_USB_ID_e usbno);

	/** Set the callback function - Register ISR for EC Command
	 * This is for EC Command from Host.
	 */
	void (*set_ec_cmd_irq_callback)(struct device *dev,
		pm_ec_cmd_irq_callback_t cb);

	/** Set the callback function - Register ISR for PM SMBUS
	 * This is for I2C events. e.g. from some external sensor
	 */
	void (*set_smbus_irq_callback)(struct device *dev,
		pm_smbus_irq_callback_t cb);

	/** Set the callback function - Register ISR for RTC
	 * This is for RTC events. i.e. Periodic / One Shot
	 */
	void (*set_rtc_irq_callback)(struct device *dev,
		pm_rtc_irq_callback_t cb);

	/** Set the callback function - Register ISR for Tamper
	 * This is to facilitate an User (Customer) define action upon
	 * detection of a tamper event
	 */
	void (*set_tamper_irq_callback)(struct device *dev,
		pm_tamper_irq_callback_t cb);

	/** Set the callback function - Register ISR for gpio on run mode
	 * This is to facilitate user define action upon
	 * aon gpio interrupt with system in run state
	 */
	void (*set_aon_gpio_run_irq_callback)(struct device *dev,
		pm_aon_gpio_run_irq_callback_t cb);
};

/******************************************
 * APIs to enter Low Power States
 *******************************************/

/**
 * @brief Put system into default PM mode
 *
 * @details This puts system into MPROC_DEFAULT_PM_MODE after PM init done at
 *          AAI startup, can only be called once.
 *
 * @param dev PM device structure.
 * @param init_phase Initialization Phase: should be set to '1' if being called
 *        during system boot up.
 *
 * @retval 0:OK, -1:ERROR
 */
static inline s32_t pm_enter_default_state(struct device *dev, u32_t init_phase)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->enter_default_state)
		return api->enter_default_state(dev, init_phase);
	return 0;
}

/**
 * @brief Put system into target PM mode
 *
 * @details Put system into target PM mode, can be called at anytime
 *
 * @param[in] dev PM device structure.
 * @param[in] state: target PM mode
 *
 * @retval 0:OK, -1:ERROR
 */
static inline s32_t pm_enter_target_state(struct device *dev,
		MPROC_PM_MODE_e state)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->enter_target_state)
		return api->enter_target_state(dev, state);
	return 0;
}

/******************************************
 * Utilitarian APIs
 *******************************************/

/**
 * @brief Get the PM mode
 *
 * @details Get the PM mode
 *
 * @param[in] dev PM device structure.
 *
 * @retval current PM state
 */
static inline MPROC_PM_MODE_e pm_get_pm_state(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->get_pm_state_desc)
		return api->get_pm_state(dev);
	return 0;
}

/**
 * @brief Get the description of PM mode
 *
 * @details Get the description string of target PM mode to print
 *
 * @param[in] dev PM device structure.
 * @param[in] state: target PM mode
 *
 * @retval String describing current PM state,
 *         i.e. "run250", "run200" ...  "verydeepsleep"
 */
static inline char *pm_get_pm_state_desc(struct device *dev,
			MPROC_PM_MODE_e state)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->get_pm_state_desc)
		return api->get_pm_state_desc(dev, state);
	return 0;
}

/**
 * @brief Get the RTC wakeup time
 *
 * @details Get the current configured PM auto wakeup time for RTC
 *
 * @param[in] dev PM device structure.
 *
 * @retval Auto wakeup time in second, 0 means RTC wakeup disabled
 */
static inline u32_t pm_get_rtc_wakeup_time(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->get_rtc_wakeup_time)
		return api->get_rtc_wakeup_time(dev);
	return 0;
}

/******************************************
 * Setting / Configuring Wake Up Sources
 *******************************************/

/**
 * @brief Set AON GPIO sources for Wake Up
 *
 * @details Configure AON GPIO sources for Wake Up
 *
 * @param[in] dev PM device structure.
 * @param[in] aongpio the desired GPIO number
 * @param[in] int_type 1=level, 0=edge
 * @param[in] int_de 1=both edge, 0=single edge
 * @param[in] int_edge 1=rising edge or high level, 0=falling edge or low level
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_aongpio_wakeup_source(struct device *dev,
		u32_t aongpio, u32_t int_type, u32_t int_de, u32_t int_edge, u32_t enable)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_aongpio_wakeup_source)
		return api->set_aongpio_wakeup_source(dev, aongpio, int_type, int_de, int_edge, enable);
	return 0;
}

/**
 * @brief Set NFC LPTD as a Wake Up Source
 *
 * @details Set NFC LPTD as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_nfc_lptd_wakeup_source(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_nfc_lptd_wakeup_source)
		return api->set_nfc_lptd_wakeup_source(dev);
	return 0;
}

/**
 * @brief Set USB as a Wake Up Source
 *
 * @details Configure USB port as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 * @param[in] usbno 0: USB0, 1: USB1
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_usb_wakeup_source(struct device *dev, u32_t usbno)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_usb_wakeup_source)
		return api->set_usb_wakeup_source(dev, usbno);
	return 0;
}

/**
 * @brief Set EC Command as a Wake Up Source
 *
 * @details Set EC Command as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_ec_cmd_wakeup_source(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_ec_cmd_wakeup_source)
		return api->set_ec_cmd_wakeup_source(dev);
	return 0;
}

/**
 * @brief Set Tamper as a Wake Up Source
 *
 * @details Set Tamper as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_tamper_wakeup_source(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_tamper_wakeup_source)
		return api->set_tamper_wakeup_source(dev);
	return 0;
}

/**
 * @brief Set smbus as a Wake Up Source
 *
 * @details Set smbus as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline u32_t pm_set_smbus_wakeup_source(struct device *dev)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_smbus_wakeup_source)
		return api->set_smbus_wakeup_source(dev);
	return 0;
}

/**
 * @brief Set the RTC as a wakeup source and configure wakeup
 *  	  time
 *
 * @details Set the target PM auto wakeup time for RTC
 *
 * @param[in] dev PM device structure.
 * @param[in] sec: seconds
 *
 * @retval The actual configured time: Success
 * @retval 0: Failure in setting desired configured time (possibly because
 *         no BBL access
 */
static inline u32_t pm_set_rtc_wakeup_source(struct device *dev, u32_t sec)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_rtc_wakeup_source)
		return api->set_rtc_wakeup_source(dev, sec);
	return 0;
}

/***************************************************
 * Installing Callback Functions for Wake Up Events
 ***************************************************/

/**
 * @brief Set the callback function - Register ISR for AON GPIO
 *
 * @details This is for AON GPIO events. e.g. to detect a push button
 *          action while A7 is sleeping
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 * @param[in] aongpio: aongpio number from 0 to 10
 *
 * @retval None
 */
static inline void pm_set_aon_gpio_irq_callback(struct device *dev,
				pm_aon_gpio_irq_callback_t cb, u32_t aongpio)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_aon_gpio_irq_callback)
		api->set_aon_gpio_irq_callback(dev, cb, aongpio);
}

/**
 * @brief Set the callback function - Register ISR for NFC LPTD
 *
 * @details This is for NFC LPTD events. e.g. to detect presence of
 *          a NFC device in proximity
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_nfc_lptd_irq_callback(struct device *dev,
				pm_nfc_lptd_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_nfc_lptd_irq_callback)
		api->set_nfc_lptd_irq_callback(dev, cb);
}

/**
 * @brief Set the callback function - Register ISR for USB
 *
 * @details This is for USB events. e.g. to detect insertion of USB device
 *          while A7 is sleeping
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 * @param[in] usbno: USB number, 0 or 1
 *
 * @retval None
 */
static inline void pm_set_usb_irq_callback(struct device *dev,
				pm_usb_irq_callback_t cb, u32_t usbno)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_usb_irq_callback)
		api->set_usb_irq_callback(dev, cb, usbno);
}

/**
 * @brief Set the callback function - Register ISR for EC
 *  	  Command
 *
 * @details This is for EC Command events from Host.
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_ec_cmd_irq_callback(struct device *dev,
				pm_ec_cmd_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_ec_cmd_irq_callback)
		api->set_ec_cmd_irq_callback(dev, cb);
}

/**
 * @brief Set the callback function - Register ISR for tamper
 *
 * @details This is for tamper events. e.g. as received from BBL
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_tamper_irq_callback(struct device *dev,
				pm_tamper_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_tamper_irq_callback)
		api->set_tamper_irq_callback(dev, cb);
}

/**
 * @brief Set callback function - Register ISR for AON gpio in run mode
 *
 * @details This is for gpio events in run mode.
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_aon_gpio_run_irq_callback(struct device *dev,
					pm_aon_gpio_run_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_aon_gpio_run_irq_callback)
		api->set_aon_gpio_run_irq_callback(dev, cb);
}

/**
 * @brief Set the callback function - Register ISR for PM SMBUS
 *
 * @details This is for I2C events. e.g. from some external sensor
 *          that user/customer might want to handle
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_smbus_irq_callback(struct device *dev,
				pm_smbus_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_smbus_irq_callback)
		api->set_smbus_irq_callback(dev, cb);
}

/**
 * @brief Set the callback function - Register ISR for RTC
 *
 * @details This is for RTC wakeup event.
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static inline void pm_set_rtc_irq_callback(struct device *dev,
				pm_rtc_irq_callback_t cb)
{
	const struct pm_driver_api *api = dev->driver_api;

	if (api->set_rtc_irq_callback)
		api->set_rtc_irq_callback(dev, cb);
}

#ifdef __cplusplus
}
#endif

#endif /*_PM_H_*/

