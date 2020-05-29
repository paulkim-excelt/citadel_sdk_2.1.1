/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

#ifndef __BBL_H__
#define __BBL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <device.h>

/*
 * Tamper Detection Mechanisms
 */
#define BBL_TAMPER_DETECT_FREQUENCY_MONITOR	0x1
#define BBL_TAMPER_DETECT_VOLTAGE_MONITOR	0x2
#define BBL_TAMPER_DETECT_TEMP_MONITOR		0x3
#define BBL_TAMPER_DETECT_INTERNAL_MESH		0x4
#define BBL_TAMPER_DETECT_EXTERNAL_MESH		0x5

/* BBL Tamper pins */
#define BBL_TAMPER_N_0	0x0
#define BBL_TAMPER_N_1	0x1
#define BBL_TAMPER_N_2	0x2
#define BBL_TAMPER_N_3	0x3
#define BBL_TAMPER_N_4	0x4
#define BBL_TAMPER_P_0	0x5
#define BBL_TAMPER_P_1	0x6
#define BBL_TAMPER_P_2	0x7
#define BBL_TAMPER_P_3	0x8
#define BBL_TAMPER_P_4	0x9

/*
 * Get mask for static and dynamic mesh pins
 */
#define BBL_TAMPER_STATIC_INPUT_MASK_N(PIN)	BIT(PIN)
#define BBL_TAMPER_STATIC_INPUT_MASK_P(PIN)	BIT(BBL_TAMPER_P_0 + PIN)
#define BBL_TAMPER_DYN_MESH_MASK(PIN)		BIT(PIN - 1)

/* BBL Temperature monitor low volatge detect threshold levels */
#define BBL_TMON_LVD_THRESH_2_2_V	0	/* 2.2 Volts */
#define BBL_TMON_LVD_THRESH_2_1_V	1	/* 2.1 Volts */
#define BBL_TMON_LVD_THRESH_2_0_V	2	/* 2.0 Volts (default) */
#define BBL_TMON_LVD_THRESH_1_9_V	3	/* 1.9 Volts */

/* BBL Temperature monitor high volatge detect threshold levels */
#define BBL_TMON_HVD_THRESH_3_7_V	0	/* 3.7 Volts */
#define BBL_TMON_HVD_THRESH_3_6_V	1	/* 3.6 Volts (default) */
#define BBL_TMON_HVD_THRESH_3_5_V	2	/* 3.5 Volts */
#define BBL_TMON_HVD_THRESH_3_4_V	3	/* 3.4 Volts */

/* BBL BBRAM Bit Flip rates */
#define BBL_BBRAM_BFR_MASK		0x3
#define BBL_BBRAM_BFR_9_HOURS		0
#define BBL_BBRAM_BFR_18_HOURS		1
#define BBL_BBRAM_BFR_36_HOURS		2
#define BBL_BBRAM_BFR_72_HOURS		3

/* BBL tamper event sources */
#define BBL_TAMPER_EVENT_N_0		BIT(0)
#define BBL_TAMPER_EVENT_N_1		BIT(1)
#define BBL_TAMPER_EVENT_N_2		BIT(2)
#define BBL_TAMPER_EVENT_N_3		BIT(3)
#define BBL_TAMPER_EVENT_N_4		BIT(4)
#define BBL_TAMPER_EVENT_P_0		BIT(5)
#define BBL_TAMPER_EVENT_P_1		BIT(6)
#define BBL_TAMPER_EVENT_P_2		BIT(7)
#define BBL_TAMPER_EVENT_P_3		BIT(8)
#define BBL_TAMPER_EVENT_P_4		BIT(9)
#define BBL_TAMPER_EVENT_TMON_LOW	BIT(10)
#define BBL_TAMPER_EVENT_TMON_HIGH	BIT(11)
#define BBL_TAMPER_EVENT_TMON_ROC	BIT(12)
#define BBL_TAMPER_EVENT_VMON_LVD	BIT(13)
#define BBL_TAMPER_EVENT_VMON_HVD	BIT(14)
#define BBL_TAMPER_EVENT_FMON_LOW	BIT(15)
#define BBL_TAMPER_EVENT_FMON_HIGH	BIT(16)
#define BBL_TAMPER_EVENT_IMESH_FAULT	BIT(17)
#define BBL_TAMPER_EVENT_EMESH_FAULT	BIT(18)
#define BBL_TAMPER_EVENT_EMESH_OPEN	BIT(19)

/* BBL LFSR update periods */
#define BBL_LFSR_PERIOD_MASK		0x7
#define BBL_LFSR_PERIOD_2_MS		0	/* 2 ms */
#define BBL_LFSR_PERIOD_4_MS		1	/* 4 ms */
#define BBL_LFSR_PERIOD_8_MS		2	/* 8 ms */
#define BBL_LFSR_PERIOD_16_MS		3	/* 16 ms */
#define BBL_LFSR_PERIOD_31_25_MS	4	/* 31.25 ms */
#define BBL_LFSR_PERIOD_62_5_MS		5	/* 63.5 ms */
#define BBL_LFSR_PERIOD_125_MS		6	/* 125 ms */
#define BBL_LFSR_PERIOD_250_MS		7	/* 250 ms */

/* BBRAM Size (1280 bits) */
#define BBRAM_NUM_WORDS			(1280 / 32) /* 40 words */

/*
 * @brief BBL Tamper configuration for frequency monitor
 * @details Configuration to program the frequency monitor for tamper detection
 *
 * low_freq_detect - Set this flag to enable tamper event if the clock frequency
 *		     goes below 26.3 KHz
 * high_freq_detect - Set this flag to enable tamper event if the clock
 *		      frequency goes above 40 KHz.
 */
struct bbl_fmon_tamper_config {
	bool low_freq_detect;
	bool high_freq_detect;
};

/*
 * @brief BBL Tamper configuration for Voltage monitor
 * @details Configuration to program the Voltage monitor for tamper detection
 *
 * low_voltage_detect - Set this flag to enable low voltage detection on the
 *			battery.
 * high_voltage_detect - Set this flag to enable high voltage on the battery.
 * low_voltage_thresh - This is the lower limit for the low voltage detection.
 *			If the battery voltage drops below this value, a tamper
 *			event is generated. For allowed values see macros
 *			BBL_TMON_LVD_THRESH_XXX.
 * high_voltage_thresh - This is the upper limit for high voltage detection.
 *			 If the battery voltage goes above this value, a tamper
 *			 event is generated. For allowed values see macros
 *			 BBL_TMON_HVD_THRESH_XXX.
 */
struct bbl_vmon_tamper_config {
	bool low_voltage_detect;
	bool high_voltage_detect;
	u8_t low_voltage_thresh;
	u8_t high_voltage_thresh;
};

/*
 * @brief BBL Tamper configuration for Temperature monitor
 * @details Configuration to program the Temperature monitor for tamper
 *	    detection
 *
 * upper_limit - Max junction temperature allowed before a tamper event is
 *		 triggered
 *		 Range: -81 to +413
 *		 Units: Degree Celcius
 * lower limit - Min junction temperature below which a tamper event is
 *		 generated
 *		 Range: -81 to +413
 *		 Units: Degree Celcius
 * roc - Max temperature change rate allowed. If the temperature changes faster
 *	 than this limit a tamper event will be generated.
 *	 Range: -81 to +413
 *	 Units: Degree Celcius per second
 */
struct bbl_tmon_tamper_config {
	u32_t upper_limit;
	u32_t lower_limit;
	u32_t roc;
};

/*
 * @brief BBL Tamper configuration for internal mesh
 * @details Configuration to program the Internal mesh for tamper detection
 *
 * fault_check_enable Set this flag to enable the internal mesh fault check
 *		function. Setting this to false is same equivalent to
 *		passing a NULL value for this structure to
 *		bbl_tamper_detect_configure(BBL_TAMPER_DETECT_INTERNAL_MESH)
 */
struct bbl_int_mesh_config {
	bool fault_check_enable;
};

/*
 * @brief BBL Tamper configuration for external mesh
 * @details Configuration to program the external mesh for tamper detection
 *	    The external mesh provides 10 pins (5 p/n pairs) which can be
 *	    configured either as a static digital tamper input pins or as a
 *	    dynamic mesh.
 *
 *	    1) Static mode: In this mode all the tamper pins are configured as
 *	       inputs and the tamper_p pins that are enabled are required to be
 *	       tied to logic '0' and the tamper_n pins that are enabled are
 *	       required to be tied to logic '1'. A tamper event will be
 *	       generated if the tamper input pins do not detect the expected
 *	       logic level. In this mode each tamper pin can be individually
 *	       enabled or disabled.
 *
 *	    2) Dynamic mode: In this mode the tamper pins tamper_p[0] and
 *	       tamper_n[0] are configured are inputs identical to static mode
 *	       The remaining 4 pairs are expected to be connected pair wise.
 *	       p[1] to n[1], p[2] to n[2] and so on. The tamper_n pins are
 *	       configured as outputs, while the tamper_p pins are configured
 *	       as inputs. An LFSR sequence is output on the tamper_n outputs and
 *	       the tamper_p signals are sampled and compared against the
 *	       corresponding tamper_n outputs. If an open fault is detected a
 *	       tamper event is generated. In this mode p[0] and n[0] can be
 *	       enabled/disabled individually, whereas the other tamper pins can
 *	       be enabled/disabled in pairs.
 *
 * dyn_mesh_enable - Set this to choose Dynamic mesh mode.
 * static_pin_mask - This is the bitmask which determines which digital tamper
 *		     inputs should be enabled. Bits [0 .. 4] correspond to
 *		     tamper_n lines [0 .. 4] and Bits [5 .. 9] correspond to
 *		     tamper_p lines [0 .. 4]. Use macro
 *		     BBL_TAMPER_STATIC_INPUT_MASK_X() to get the mask for a
 *		     given tamper line. Note that if dynamic mesh is enabled
 *		     then only Bit[0] and Bit[5] will be considered. The others
 *		     will be ignored since they will be configured as dynamic
 *		     mesh pairs.
 * dyn_mesh_mask - This bit mask determines which tamper line pairs will be
 *		   enabled as a tamper source. Bits [0 .. 3] correspond to
 *		   tamper line pairs [1 .. 4]. This field is ignored if
 *		   dyn_mesh_enable is set to false. Use macro
 *		   BBL_TAMPER_DYN_MESH_MASK() to get the mask for a given pair
 */
struct bbl_ext_mesh_config {
	bool dyn_mesh_enable;
	u32_t static_pin_mask;
	u8_t dyn_mesh_mask;
};

/*
 * @brief BBL Tamper detection configuration
 * @details Configuration to program the BBL tamper detection logic
 *	    and setup the tamper sources
 *
 * tamper_type - The tamper detection mechanism that needs to be configured.
 *		 Use BBL_TAMPER_DETECT_XXX macros for specifying the tamper type
 * tamp_cfg - Pointer to the tamper configuration for the tamper detection
 *	      mechanism specified. It points to one of the bbl_xxx_tamper_config
 *	      structures.
 */
struct bbl_tamper_config {
	u8_t tamper_type;
	void *tamper_cfg;
};

/*
 * @brief BBRAM security configuration
 * @details Configuration to protect the Battery backed RAM
 *
 * bit_flip_enable - Set this flag to enable periodic flipping of the BBRAM
 *		     memory bits.
 * bit_flip_rate - When bit_flip_enable is set, this field determines the period
 *		   of bit flipping. For allowed values see BBL_BBRAM_BFR_XXX
 */
struct bbl_bbram_config {
	bool bit_flip_enable;
	u8_t bit_flip_rate;
};

/*
 * @brief LFSR configruation parameters
 * @details Configuration parameters for the linear feedback shift register
 *	    used to drive the internal mesh and external mesh (in dynamic mode)
 *
 * lfsr_update - Set this flag if the LFSR should be updated periodically
 *		 Note that the updated LSFR parameters will apply to both
 *		 internal mesh and external mesh (in dynamic mode)
 * lfsr_seed - Value to seed the Linear feedback shift register (LFSR) that is
 *	       used to generate the pattern on the outputs of the mesh.
 * lfsr_period - The rate at which the LFSR sequence should be updated. See
 *		 BBL_LFSR_PERIOD_XXX macros for values. This parameter does not
 *		 take effect if lfsr_update is set to false.
 * mfm_enable - Set this flag to enable MFM (Modified frequency modulation)
 *		encoding on the dynamic mesh outputs.
 */
struct bbl_lfsr_params {
	bool lfsr_update;
	u32_t lfsr_seed;
	u32_t lfsr_period;
	bool mfm_enable;
};

/**
 * @brief Callback function for reporting a tamper event
 *
 * @param tamper_event - Tamper event mask (See BBL_TAMPER_EVENT_XXX macros)
 *
 * Note: This callback will be called from an interrupt context. So the callback
 *	 should not call any blocking functions or OS service apis.
 */
typedef void (*bbl_tamper_detect_cb)(u32_t tamper_event, u32_t timestamp);

/**
 * @typedef bbl_api_tamper_detect_configure
 * @brief Callback API for configuring BBL tamper detection
 * See bbl_tamper_detect_configure() for argument descriptions
 */
typedef int (*bbl_api_tamper_detect_configure)(
		struct device *dev,
		struct bbl_tamper_config *config);

/**
 * @typedef bbl_api_set_lfsr_params
 * @brief Callback API for setting LFSR parameters
 * See bbl_set_lfsr_params() for argument descriptions
 */
typedef int (*bbl_api_set_lfsr_params)(
		struct device *dev,
		struct bbl_lfsr_params *cfg);

/**
 * @typedef bbl_api_get_tamper_pin_state
 * @brief Callback API for getting tamper pin state
 * See bbl_get_tamper_pin_state() for argument descriptions
 */
typedef int (*bbl_api_get_tamper_pin_state)(
		struct device *dev,
		u8_t pin,
		u8_t *state);

/**
 * @typedef bbl_api_bbram_configure
 * @brief Callback API for configuring BBRAM
 * See bbl_bbram_configure() for argument descriptions
 */
typedef int (*bbl_api_bbram_configure)(
		struct device *dev,
		struct bbl_bbram_config *config);

/**
 * @typedef bbl_api_bbram_read
 * @brief Callback API for reading the secure BBRAM memory
 * See bbl_bbram_read() for argument descriptions
 */
typedef int (*bbl_api_bbram_read)(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length);

/**
 * @typedef bbl_api_bbram_write
 * @brief Callback API for writing to secure BBRAM memory
 * See bbl_bbram_write() for argument descriptions
 */
typedef int (*bbl_api_bbram_write)(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length);

/**
 * @typedef bbl_api_bbram_clear
 * @brief Callback API for clearing to secure BBRAM memory
 * See bbl_bbram_clear() for argument descriptions
 */
typedef int (*bbl_api_bbram_clear)(struct device *dev);

/**
 * @typedef bbl_api_bbram_set_access
 * @brief Callback API for setting write access to secure BBRAM memory
 * See bbl_bbram_set_access() for argument descriptions
 */
typedef int (*bbl_api_bbram_set_access)(struct device *dev, bool enable_write);

/**
 * @typedef bbl_api_register_tamper_detect_cb
 * @brief Callback API for installing the tamper detect callback
 * See bbl_register_tamper_detect_cb() for argument descriptions
 */
typedef int (*bbl_api_register_tamper_detect_cb)(
		struct device *dev,
		bbl_tamper_detect_cb cb);

/**
 * @brief BBL driver API
 */
struct bbl_driver_api {
	bbl_api_tamper_detect_configure		td_config;
	bbl_api_set_lfsr_params			set_lfsr_params;
	bbl_api_get_tamper_pin_state		get_tamper_pin_state;
	bbl_api_bbram_configure			bbram_config;
	bbl_api_bbram_read			bbram_read;
	bbl_api_bbram_write			bbram_write;
	bbl_api_bbram_clear			bbram_clear;
	bbl_api_bbram_set_access		bbram_set_access;
	bbl_api_register_tamper_detect_cb	register_td_cb;
};

/**
 * @brief       Configure tamper detection
 * @details     This api configures the BBL tamper detection hardware based
 *		on the configruation parameters rovided
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to BBL tamper configiuration structure.
 *			 Setting config->tamper_cfg to NULL, causes the
 *			 tamper source indicated by config->tamper_type to be
 *			 disabled.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_tamper_detect_configure(
	struct device *dev,
	struct bbl_tamper_config *config)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->td_config(dev, config);
}

/**
 * @brief       Sets the LFSR parameters
 * @details     Sets the LFSR parameters used by internal and external meshes
 *		in dynamic mode
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cfg - Pointer to LFSR params structure
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_set_lfsr_params(
		struct device *dev,
		struct bbl_lfsr_params *cfg)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->set_lfsr_params(dev, cfg);
}

/**
 * @brief       Get tamper pin state
 * @details     Get the state of the digital tamper input pin
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   pin - Tamper input pin for which the state is required. Use
 *		      BBL_TAMPER_[P|N]_X macros to specify the pin.
 * @param[out]  state - 1 if the pin is in its active state, 0 otherwise
 *			Active state for tamper_p[] pins is 1
 *			Active state for tamper_n[] pins is 0
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_get_tamper_pin_state(
		struct device *dev,
		u8_t pin,
		u8_t *state)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->get_tamper_pin_state(dev, pin, state);
}

/**
 * @brief       Configure BBRAM
 * @details     Set the BBRAM bit flip settings
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to the BBRAM configuration structure
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_bbram_configure(
		struct device *dev,
		struct bbl_bbram_config *config)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->bbram_config(dev, config);
}

/**
 * @brief       Read the BBRAM contents
 * @details     Read the 1280 bit secure BBRAM memory contents
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   offset - Word offset into the BBRAM memory (0 - 39)
 * @param[out]  buffer - Buffer pointer to which the BBRAM contents should be
 *			 written.
 * @param[in]   length - Length of the data to be read in words. length + offset
 *		should be less than 40.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_bbram_read(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->bbram_read(dev, offset, buffer, length);
}

/**
 * @brief       Write to BBRAM
 * @details     Write to the 1280 bit secure BBRAM memory
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   offset - Word offset into the BBRAM memory (0 - 39)
 * @param[out]  buffer - Pointer to buffer with the data to be written.
 * @param[in]   length - Length of the data to be read in words. length + offset
 *		should be less than 40.
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_bbram_write(
		struct device *dev,
		u32_t offset,
		u32_t *buffer,
		u32_t length)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->bbram_write(dev, offset, buffer, length);
}

/**
 * @brief       Clear the BBRAM contents
 * @details     Erase the 1280 bit secure BBRAM memory contents
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_bbram_erase(struct device *dev)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->bbram_clear(dev);
}

/**
 * @brief	Set BBRAM write access
 * @details     Configure BBRAM as read-only or writable
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   enable_write - Set to true for read-write access and false for
 *		read-only access
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_bbram_set_access(struct device *dev, bool enable_write)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->bbram_set_access(dev, enable_write);
}

/**
 * @brief       Register tamper event callback
 * @details     API to register for a tamper event callback. The tamper event
 *		mask is passed to the callback to indicate which tamper sources
 *		triggered the tamper event
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   cb - Callback function to be called on a tamper event
 *
 * @return      0 on success
 * @return      -errno on failure
 */
static inline int bbl_register_tamper_detect_cb(
		struct device *dev,
		bbl_tamper_detect_cb cb)
{
	struct bbl_driver_api *api;

	api = (struct bbl_driver_api *)dev->driver_api;
	return api->register_td_cb(dev, cb);
}

#ifdef __cplusplus
}
#endif

#endif
