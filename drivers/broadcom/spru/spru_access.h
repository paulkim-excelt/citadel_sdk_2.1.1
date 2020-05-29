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

/*
 * @file spru_access.c
 * @brief BBL register access routines using SPRU interface
 */

#ifndef __SPRU_ACCESS_H__
#define __SPRU_ACCESS_H__

#include <zephyr/types.h>

/**
 * @brief Write BBL Register
 * @details Write BBL Register using the spru interface
 *
 * @param addr register address
 * @param data Data to be written
 *
 * @return 0 in Success, -errno otherwise
 */
int bbl_write_reg(u32_t addr, u32_t data);

/**
 * @brief Read BBL Register
 * @details Read BBL Register using the spru interface
 *
 * @param addr register address
 * @param data Data to be Read
 *
 * @return 0 in Success, -errno otherwise
 */
int bbl_read_reg(u32_t addr, u32_t *data);

/**
 * @brief Write BBL Memory
 * @details Write BBL Memory using the SPRU interface
 *
 * @param addr Memory address
 * @param data Data to be written
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_write_mem(u32_t addr, u32_t data);

/**
 * @brief Read BBL Memory
 * @details Read BBL Memory using SPRU interface
 *
 * @param addr Memory address
 * @param data Location to write the read data to
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_read_mem(u32_t addr, u32_t *data);

/**
 * @brief Enable access to BBL registers/memory
 * @details Set BBL authentication code/check registers to enable BBL access.
 *	    This api must be called before calling any other BBL red/writes apis
 *
 * @return 0 on Success, -errno on failure
 */
int bbl_access_enable(void);

/**
 * @brief Reset SPRU Interface
 * @details Reset SPRU Interface by toggling the SPRU Soft reset bit
 *
 * @param delay_us Delay in toggling SPRU soft reset bit (in microseconds)
 *
 * @return none
 */
void spru_interface_reset(u32_t delay_us);

/**
 * @brief Get RTC time at system bootup
 * @details Retrieves the RTC time at startup.
 *
 * @return RTC time before SPRU interface is enabled
 */
u32_t spru_get_rtc_time_at_startup(void);

#endif /* __SPRU_ACCESS_H__ */
