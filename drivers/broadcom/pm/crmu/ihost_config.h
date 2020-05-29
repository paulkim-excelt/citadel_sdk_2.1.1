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

#ifndef IHOST_CONFIG_H
#define IHOST_CONFIG_H

#include "pm_regs.h"
#include "regaccess.h"

#define  PLL_PWRON_WAIT  0x250
#define  PLL_CONFIG_WAIT 0x250

#ifdef SMALL_TIMEOUT

#define  OTP_TIMEOUT          0x20
#define  OTP_TIMEOUT_2        0x40
#define  POWER_GOOD_TIMEOUT   0x50
#define  POWER_GOOD_TIMEOUT_2 0x100
#define  PLL_TIMEOUT          0x400
#define  PLL_TIMEOUT_2        0x400
#define  ARM_PLL_TIMEOUT      0x28A /* 25us */
#define  ARM_PLL_TIMEOUT_2    0x514 /* 50us */

#else
/* Timeout to be reviewd for 40nm */
#define  OTP_TIMEOUT          0x200 /* OTP fdone timeout = 1190ns  ~ 2x */
#define  OTP_TIMEOUT_2        0x200
#define  POWER_GOOD_TIMEOUT   0x1388 /* PDSYS_POWER_GOOD = 200us = ~0x1388 */
#define  POWER_GOOD_TIMEOUT_2 0x3FFFFF
#define  PLL_TIMEOUT          0x6000 /* PLL = 500us ~= 2x = 61A8 */
#define  PLL_TIMEOUT_2        0x286A0
#define  ARM_PLL_TIMEOUT      0x514 /* 50us */
#define  ARM_PLL_TIMEOUT_2    0x1450 /* 200us */

#endif

#define ENABLE_UART_TX 0xFFFFFFFE
#define ENABLE_UART_RX 0x3
#define OTP_GENPLL_CFG 0x00000002

#define IHOST_PLL_PWRON_WAIT 0xA2C /* (50us/0.0384us = 1302 = 1302*2) */
#define IHOST_SWITCH_PROG_TIMEOUT 0xCC /* 10us */

#define IHOST0_CONFIG_ROOT 0x00000000

#define CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_MASK \
		(0x7 << CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_R)
#define CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_FUNCTIONALTEST \
		(0x1 << CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_R)
#define isFunctionalTestMode() \
		(((RdReg(CRMU_STRAP_DATA) & \
		CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_MASK) \
		== CRMU_STRAP_DATA__CRMU_LATCHED_STRAPS_FUNCTIONALTEST))

#define POWER_ON_PLL ((0x1 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | \
                     (0x0 << CRMU_PLL_AON_CTRL__GENPLL_PWRDN ) | \
                     (0x0 << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN ))

#define REMOVE_PLL_ISO ((0x0 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN) | \
                      (0x0 << CRMU_PLL_AON_CTRL__GENPLL_PWRDN) | \
                      (0x0 << CRMU_PLL_AON_CTRL__GENPLL_FREF_PWRDN))

#define CRMU_GENPLL_CONTROL2__i_ndiv_frac_MASK \
          (0xFFFFFF << CRMU_GENPLL_CONTROL2__i_ndiv_frac_R)
#define CRMU_GENPLL_CONTROL3__i_ndiv_int_MASK \
          (0x1FF << CRMU_GENPLL_CONTROL3__i_ndiv_int_R)

#define CRMU_GENPLL_CONTROL3__i_p1div_MASK \
              (0xF << CRMU_GENPLL_CONTROL3__i_p1div_R)

#define CRMU_GENPLL_CONTROL4__i_m1div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL4__i_m1div_R)
#define CRMU_GENPLL_CONTROL4__i_m2div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL4__i_m2div_R)

#define CRMU_GENPLL_CONTROL5__i_m3div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL5__i_m3div_R)
#define CRMU_GENPLL_CONTROL5__i_m4div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL5__i_m4div_R)

#define CRMU_GENPLL_CONTROL6__i_m5div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL6__i_m5div_R)
#define CRMU_GENPLL_CONTROL6__i_m6div_MASK \
              (0xFF << CRMU_GENPLL_CONTROL6__i_m6div_R)

#define  OTP_STATUS             0x1
#define  POWER_GOOD_STATUS      0x2
#define  PLL_STATUS             0x4
#define  IHOST_SWITCH_STATUS    0x8
#define  IHOST_PLL_LOCK_STATUS  0x10

void ihost_config(void);

#endif
