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
#ifndef __CLOCK_A7_H__
#define __CLOCK_A7_H__

#include <broadcom/clock_control.h>
#include <logging/sys_log.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the rate of the A7 CPU
 *
 * @param rate pointer to the variable to which the rate will be written (in Hz)
 *
 * @return error
 */
static inline int clk_a7_get(u32_t *rate)
{
	struct device *clk;
	int rc;

	clk = device_get_binding(A7_CLOCK_CONTROL_NAME);
	if (!clk) {
		SYS_LOG_ERR("Cannot get clock device");
		return -ENODEV;
	}

	rc = clock_control_get_rate(clk, NULL, rate);
	if (rc) {
		SYS_LOG_ERR("Cannot query clock speed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Set the rate of the A7 CPU
 *
 * @param rate Variable containing the rate which will be written (in Hz)
 *
 * @return error
 */
static inline int clk_a7_set(u32_t rate)
{
	struct device *clk;
	int rc;

	clk = device_get_binding(A7_CLOCK_CONTROL_NAME);
	if (!clk) {
		SYS_LOG_ERR("Cannot get clock device");
		return -ENODEV;
	}

	rc = clock_control_set_rate(clk, NULL, rate);
	if (rc) {
		SYS_LOG_ERR("Cannot query clock speed");
		return -EIO;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __CLOCK_A7_H__ */
