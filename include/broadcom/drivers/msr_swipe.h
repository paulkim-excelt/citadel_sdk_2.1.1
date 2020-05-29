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
#ifndef __MSR_SWIPE_H__
#define __MSR_SWIPE_H__

#include <stddef.h>
#include <device.h>
#include <logging/sys_log.h>
/* This header includes the arch specific sys_io.h for register operations
 * Weirdly this also includes gcc.h which provides ARG_UNUSED macro.
 */
/* Pull in the arch-specific implementations */
#include <arch/cpu.h>

#define msrdebug printk

#define CONFIG_ADC_MSR

#ifdef TRACE_ADC_DATA
/* 
 * !!! Have to enlarge this ADC_MSR_BSIZE size if want to capture the whole ADC samples... 
 * If no enough memory in the system, try to disable BT and NFC features in Makefile to release more buffer.
 */
#define ADC_MSR_BSIZE (18*1024)
#else
/* FIXME:
 * In Lynx, the RB size was same as that in TRACE_DC_DATA
 * case, i.e. (18*1024) - 36KB which is way too huge.
 * The MSR seems to work with 2KB per track.
 * However, this needs some more experiments to find out
 * the optimum size of RB to make MSR work.
 *
 * In addition the RB should be allowed to be written to
 * even when it is full by keeping latest samples.
 */
#define ADC_MSR_BSIZE (1024)
#endif


/* MSR_DEVELOPMENT_TEST - testing the code against a gold card */
#define MSR_DEVELOPMENT_TEST  1

/* */
#define MSR_SWIPE_EDGE_DOWN 0
#define MSR_SWIPE_EDGE_UP   1

#define MSR_NUM_TRACK   3 /* three tracks */

#define MSR_AVG_DATA_NUM 5 /* avarage data number for each point */

#define BROADCOM_SVK_BOARD
/* #define CLOVER_BOARD */

#define MSR_ADC_MIN_VALUE_10B 0
#define MSR_ADC_MAX_VALUE_10B 1023

/*
 * Tuning parameter
 */

/* #define VTH1 34  // each step is ~1.17mV
 * #define VTH2 34  // (~40mV)
 * #define VTH3 34  //
 */

/*
 * Note - The following parameters maybe need
 * some adaptions in different hardware design.
 *
 * This settings are based on BCM958100SVK board.
 *
 * FIXME: Need to fix this for the Citadel Boards.
 */
/* noise amplitude level.*/
#if defined BROADCOM_SVK_BOARD
#define MSR_VTH_NOISE_FILTER_THRESHOLD 5 /* 8 *1.17mv */
#elif defined CLOVER_BOARD
#define MSR_VTH_NOISE_FILTER_THRESHOLD 20 /* 20 *1.17mv */
#endif

/* Low/mid/high threshold.*/
#define MSR_VTH_LOW_THRESHOLD  10 /* 10 *1.17mv */
#define MSR_VTH_MID_THRESHOLD  30 /* 30 *1.17mv */
#define MSR_VTH_HIGH_THRESHOLD 50 /* 50 *1.17mv */

/* Low/mid/high threshold margin.*/
#define MSR_VTH_LOW_MARGIN  30 /* 30 *1.17mv */
#define MSR_VTH_MID_MARGIN  60 /* 60 *1.17mv */
#define MSR_VTH_HIGH_MARGIN 120 /* 120 *1.17mv */

#define MSR_Z_LIMIT 3

/* FIXME: Why is this buffer size 256? */
#define MSR_BUF_SZ  256

/* TODO: How are these codes formed?
 *
 * Magnetic Stripe cards uses DEC 6-bit code:
 * DEC Six-bit, was simply the 64 basic ASCII characters, with codes
 * from 32-95 (decimal) or 20-5F (hexadecimal) or 40-77 (octal), in
 * exactly the same order, but with 32 (decimal) subtracted from
 * their codes, so they ran from 0 to 63.
 * But still can't understand how are these formed.
 */

/* Start Sentinel (SS) : B ;*/
#define MSR_T1_SS   0x51 /* '%' = 101_0001 (b0...b6 + Parity at 7th bit pos) */
/* End Sentinel (ES) : */
#define MSR_T1_ES   0x7C /* '?' = 111_1100 */
#define MSR_T23_SS  0x1A /* ';' = 1_1010 (b0..b3 + Parity at 4th bit pos) */
#define MSR_T23_ES  0x1F /* '?' = 1_1111 (b0..b3 + Parity at 4th bit pos) */

#define RV_MSR_T1_SS   0x45 /* % = 101_0001 (b0...b6 + P) */
#define RV_MSR_T1_ES   0x1F /* ? = 111_1100 */

#define RV_MSR_T23_SS   0xB /* ; = 1_1010 (b0..b3 + P) */
#define RV_MSR_T23_ES   0x1F /* ? = 1_1111 */


#define MSR_T1_BPC  7
#define MSR_T23_BPC 5

/* Swipe Direction */
#define MSR_FWD_SWIPE   0
#define MSR_BWD_SWIPE  1
#define MSR_SWIPE_ABORT 2

/* Number of Bytes in each Track */
#define MSR_TRACK_1_BYTE_NUM 79
#define MSR_TRACK_2_BYTE_NUM 40
#define MSR_TRACK_3_BYTE_NUM 107

/* Using unsigned short as the unit for TS (Time Stamp)
 * because in computation (e.g. in ts_diff()) the max
 * used for rollover is 0xFFFF.
 * FIXME: Confirm why Lynx used u32_t
 */
#define MSR_TS_t             u16_t

/**
 * @brief MSR Swipe device configuration.
 *
 * @param
 */
struct msr_swipe_device_config {
	u8_t adc_flextimer; /* 0: Use ADC; 1: Use Flex Timer */
	u8_t numtracks; /* Number of tracks spported */
};

/** @brief MSR SWIPE Driver API structure. */
struct msr_swipe_driver_api {
	/* Configure ADC & Channel Information per MSR Track */
	s32_t (*configure_adc_info)(struct device *msr, u8_t trk1adc, u8_t trk1ch, u8_t trk2adc, u8_t trk2ch, u8_t trk3adc, u8_t trk3ch);

	/* Dummy Init */
	s32_t (*init)(struct device *dev);

	/* Configure the MSR Swipe device with ADC Channel & Initialize it */
	s32_t (*init_with_adc)(struct device *msr, struct device *adc);

	/* Disable the ADC Channels MSR Swipe device is connected with */
	s32_t (*disable_adc)(struct device *msr, struct device *adc);

	/* Gather MSR Swipe Data & Display Output */
	s32_t (*gather_data)(struct device *msr, struct device *adc);
};

/**
 * @brief Configure the ADC no. & Channel No. for each MSR Swipe
 *  	  device tracks.
 * @details This API is used to configure the ADC number & ADC
 *  		Channel number for the MSR tracks
 * @param[in] msr pointer to msr swipe device
 * @param[in] trk1adc : ADC number for Track 1
 * @param[in] trk1ch  : ADC Channel number for Track 1
 * @param[in] trk2adc : ADC number for Track 2
 * @param[in] trk2ch  : ADC Channel number for Track 2
 * @param[in] trk3adc : ADC number for Track 3
 * @param[in] trk3ch  : ADC Channel number for Track 3
 *
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t msr_swipe_configure_adc_info(struct device *msr,
	u8_t trk1adc, u8_t trk1ch, u8_t trk2adc, u8_t trk2ch, u8_t trk3adc, u8_t trk3ch)
{
	const struct msr_swipe_driver_api *msrapi = msr->driver_api;

	if (msrapi->configure_adc_info)
		return msrapi->configure_adc_info(msr, trk1adc, trk1ch, trk2adc, trk2ch, trk3adc, trk3ch);
	return 0;
}

/**
 * @brief Configure the MSR Swipe device with ADC Channel & Initialize it.
 * @details Hook up a specific ADC Channel device to the MSR Swipe Device
 * @param[in] msr pointer to msr swipe device
 * @param[in] adc pointer to ADC device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t msr_swipe_init_with_adc(struct device *msr,
		struct device *adc)
{
	const struct msr_swipe_driver_api *msrapi = msr->driver_api;

	if (msrapi->init_with_adc)
		return msrapi->init_with_adc(msr, adc);
	return 0;
}

/**
 * @brief Disable the ADC Channels MSR Swipe device is connected with.
 * @details Disable the ADC Channels MSR Swipe device is connected with
 * @param[in] msr pointer to msr swipe device
 * @param[in] adc pointer to ADC device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t msr_swipe_disable_adc(struct device *msr,
		struct device *adc)
{
	const struct msr_swipe_driver_api *msrapi = msr->driver_api;

	if (msrapi->disable_adc)
		return msrapi->disable_adc(msr, adc);
	return 0;
}

/**
 * @brief Start gathering data & analyse.
 * @details Start gathering data, while continually analysing them to detect
 *          events such as start of swipe, end of swipe, whether it is a
 *          forward swipe or backward swipe etc. This function eventually extracts
 *          the information on the magnetic strip in a human readbale format.
 * @param[in] msr pointer to msr swipe device
 * @param[in] adc pointer to ADC device
 * @retval 0:OK
 * @retval -1:ERROR
 */
static inline s32_t msr_swipe_gather_data(struct device *msr, struct device *adc)
{
	const struct msr_swipe_driver_api *msrapi = msr->driver_api;

	if (msrapi->gather_data)
		return msrapi->gather_data(msr, adc);
	return 0;
}

#endif /* __MS_SWIPE_H__ */
