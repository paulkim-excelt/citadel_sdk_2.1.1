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
#ifndef __GENPLL_H__
#define __GENPLL_H__

#include <broadcom/clock_control.h>
#include <logging/sys_log.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

enum citadel_genpll_chans {
	/* Unicam LP Clock */
	CITADEL_GENPLL_UNICAM	= 1,
	/* AXI/AHB and APB (divided by 2) */
	CITADEL_GENPLL_AHB	= 2,
	/* WDT, Timer, UART Baud, and SPI Clock */
	CITADEL_GENPLL_WTUS	= 3,
	/* QSPI Interface Clock */
	CITADEL_GENPLL_QSPI	= 4,
	/* Smartcard Clock */
	CITADEL_GENPLL_SC	= 5,
	/* SMC Clock */
	CITADEL_GENPLL_SMC	= 6,
};

/**
 * @brief Get the GENPLL rate of a given channel
 *
 * @param chan Number corresponding to which channel's rate is requested
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_genpll_get(enum citadel_genpll_chans chan, u32_t *rate)
{
	struct device *clk;
	int rc;

	clk = device_get_binding(CLOCK_CONTROL_NAME);
	if (!clk) {
		SYS_LOG_ERR("Cannot get clock device");
		return -ENODEV;
	}

	rc = clock_control_get_rate(clk, (void *)chan, rate);
	if (rc) {
		SYS_LOG_ERR("Cannot query clock speed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Get the rate of the Unicam LP clock (channel 1)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_get_unicam(u32_t *rate)
{
	return clk_genpll_get(CITADEL_GENPLL_UNICAM, rate);
}

/**
 * @brief Get the rate of the Advanced High-performance Bus (AHB) clock (chan 2)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_get_ahb(u32_t *rate)
{
	return clk_genpll_get(CITADEL_GENPLL_AHB, rate);
}

/**
 * @brief Get the rate of the Advanced Peripheral Bus (APB) clock (channel 2)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 *
 * NOTE: Advanced Peripheral Bus (APB) is half the value of AHB (Channel 2)
 */
static inline int clk_get_apb(u32_t *rate)
{
	int rc;

	rc = clk_genpll_get(CITADEL_GENPLL_AHB, rate);
	if (rc)
		return rc;

	/* Per documentation, PCLK = HCLK / 2, only if HCLK is driven by PLL at
	 * 200Mhz or above.  Otherwise, PCLK = HCLK.
	 */
	if (*rate >= MHZ(200))
		*rate /= 2;

	return 0;
}

/**
 * @brief Get the rate of the WDT, Timer, UART Baud, and SPI clock (channel 3)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 *
 * NOTE: There is nothing connected to the full channel 3
 */
static inline int clk_get_wtus(u32_t *rate)
{
	int rc;

	rc = clk_genpll_get(CITADEL_GENPLL_WTUS, rate);
	if (rc)
		return rc;

	*rate /= 2;

	return 0;
}

/**
 * @brief Get the rate of the QSPI Interface clock (channel 4)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_get_qspi(u32_t *rate)
{
	return clk_genpll_get(CITADEL_GENPLL_QSPI, rate);
}

/**
 * @brief Get the rate of the Smartcard clock (channel 5)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_get_sc(u32_t *rate)
{
	return clk_genpll_get(CITADEL_GENPLL_SC, rate);
}

/**
 * @brief Get the rate of the SMC clock (channel 6)
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_get_smc(u32_t *rate)
{
	return clk_genpll_get(CITADEL_GENPLL_SMC, rate);
}


/**
 * @brief Set the GENPLL rate of a given channel
 *
 * @param chan Number corresponding to which channel's rate is requested
 * @param rate The rate to set (in Hz)
 *
 * @return error
 */
static inline int clk_genpll_set(enum citadel_genpll_chans chan, u32_t rate)
{
	struct device *clk;
	int rc;

	clk = device_get_binding(CLOCK_CONTROL_NAME);
	if (!clk) {
		SYS_LOG_ERR("Cannot get clock device");
		return -ENODEV;
	}

	rc = clock_control_set_rate(clk, (void *)chan, rate);
	if (rc) {
		SYS_LOG_ERR("Cannot set clock speed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Set the rate of the Unicam LP clock (channel 1)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_set_unicam(u32_t rate)
{
	return clk_genpll_set(CITADEL_GENPLL_UNICAM, rate);
}

/**
 * @brief Set the rate of the Advanced High-performance Bus (AHB) clock (chan 2)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_set_ahb(u32_t rate)
{
	return clk_genpll_set(CITADEL_GENPLL_AHB, rate);
}

/**
 * @brief Set the rate of the Advanced Peripheral Bus (APB) clock (channel 2)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 *
 * NOTE: Advanced Peripheral Bus (APB) is half the value of AHB (Channel 2)
 */
static inline int clk_set_apb(u32_t rate)
{
	int rc;

	/* Per documentation, PCLK = HCLK / 2, only if HCLK is driven by PLL at
	 * 200Mhz or above.  Otherwise, PCLK = HCLK.
	 */
	if (rate >= MHZ(200))
		rate *= 2;

	rc = clk_genpll_set(CITADEL_GENPLL_AHB, rate);
	if (rc)
		return rc;

	return 0;
}

/**
 * @brief Set the rate of the WDT, Timer, UART Baud, and SPI clock (channel 3)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 *
 * NOTE: There is nothing connected to the full channel 3
 */
static inline int clk_set_wtus(u32_t rate)
{
	int rc;

	rate *= 2;

	rc = clk_genpll_set(CITADEL_GENPLL_WTUS, rate);
	if (rc)
		return rc;

	return 0;
}

/**
 * @brief Set the rate of the QSPI Interface clock (channel 4)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_set_qspi(u32_t rate)
{
	return clk_genpll_set(CITADEL_GENPLL_QSPI, rate);
}

/**
 * @brief Set the rate of the Smartcard clock (channel 5)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_set_sc(u32_t rate)
{
	return clk_genpll_set(CITADEL_GENPLL_SC, rate);
}

/**
 * @brief Set the rate of the SMC clock (channel 6)
 *
 * @param rate Rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_set_smc(u32_t rate)
{
	return clk_genpll_set(CITADEL_GENPLL_SMC, rate);
}

#ifdef __cplusplus
}
#endif

#endif /* __GENPLL_H__ */
