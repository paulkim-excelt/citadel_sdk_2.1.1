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

/**
 * @file
 * @brief Utility api's
 *
 *
 */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <zephyr/types.h>
#include <kernel.h>

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/

/**
 * @brief Align to the power of 2
 * @details This Api is used to align the given value to the power of 2.
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] val The value to be aligned to the power of two
 * @retval 0 if successful.
 */
u32_t powerof2align(u32_t val)
{
	u32_t powerof2 = 1;

	while (powerof2 < val)
		powerof2 *= 2;

	return (val > 0 ? powerof2 : 0);
}

/**
 * @brief Align to the 256
 * @details This Api is used to align to the 256 byte/bit boundary.
 * The calculated aligned value will be returned to the caller
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] val The value to be aligned to the 256 byte/bit boundary
 * @retval 0 if successful.
 */
u32_t align256(u32_t val)
{
	return((val + 0xFF) & ~0xFF);
}

/**
 * @brief Align to the power of 2 and determine the window size mask
 * @details This Api is used to align the given value to the power of 2 and
 *		also calculate the window size mask value.
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] val The value to be aligned to the power of two
 * @param[out] win_size The window size mask value
 * @retval 0 if successful.
 */
u32_t winsize_powerof2align(u32_t val, u32_t *win_size)
{
	u32_t powerof2 = 1;
	u32_t win_size_mask = 0;
	u32_t bit = 0;

	while (powerof2 < val) {
		powerof2 *= 2;
		win_size_mask |= 0x1 << bit;
		bit++;
	}

	*win_size = (val > 0 ? ~win_size_mask : 0xffffffff);

	return (val > 0 ? powerof2 : 0);
}

/**
 * @brief Align to the given window base
 * @details This Api is used to align the given window base to the given size.
 *		This api also returns the padding value calculated to align.
 * @param[in]	dev \	Pointer to the device structure for the smau driver
 *			instance
 * @param[in] win_base Window base which has to be aligned to the given size
 * @param[in] win_size Window size
 * @param[out] pad_size_ret Calculated padding size
 * @retval 0 if successful.
 */
u32_t calc_aligned_win_base(u32_t win_base, u32_t win_size,
		u32_t *pad_size_ret)
{
	u32_t pad_size = 0;

	if (win_base > 0 && win_size > 0) {
		if ((win_base < win_size) || (win_base % win_size))
			pad_size += win_size - (win_base % win_size);
	}

	*pad_size_ret = pad_size;
	return (pad_size + win_base);
}

/**
 * @brief Check if we are in interrupt mode
 *
 * This routine returns true if we are executing an ISR
 *
 * @return true if executing in ISR mode, false otherwise
 */
bool k_is_in_isr(void)
{
       u32_t cpsr;

       __asm__ volatile("mrs %[cpsr], cpsr\n\t" : [cpsr] "=r" (cpsr));

       cpsr &= 0x1F;

       /* Neither SYS nor USR mode */
       if ((cpsr != 0x1F) && (cpsr != 0x10))
               return true;

       return false;
}

