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
#include "M0.h"
/* FIXME: Once this driver is migrated to Zephyr, socregs.h will not be
 * available and the following define can be removed and smau.h can be included
 * Including smau.h now, results in redefinition errors.
 */
#ifndef SMU_CFG_NUM_WINDOWS
#define SMU_CFG_NUM_WINDOWS 4
#endif


#ifdef MPROC_PM_FASTXIP_SUPPORT
/**
 * @brief user_fastxip_workaround
 *
 * Description:
 * 1. SBL currently does not restore SMU_CFG_1 register, nor does it restore
 * the configuration for other windows. It only restores Window 0 and programs
 * the SMU_CFG_1 register to 1. This register controls important SMAU attributes
 * such as cacheability and swapability (i.e. across endianness).
 *
 * 2. The above configurations has to be restored before the AAI can be
 * executed. Note that this will not cause any problem if (i) the SMAU windows
 * are configured in By-Pass Mode, i.e. there is no encryption or authentication
 * involved.
 *
 * 3. This is running on A7 rather than M0, though this code is compiled for M0
 * and resides on IDRAM. The reason it is kept on IDRAM, is that code resding on
 * flash can't access SMAU registers.
 *
 * 4. With this Workaround, SBL has only one place to jump to, which is this
 * function. Since SBL picks up the Jump Address from Offset 0x28 of the SMAU
 * configuration structure, the address of this function has to be stored
 * at that offset, i.e. xip_addr.
 *
 * 5. After control reaches this function, this function
 * (i) Restore the setting of SMU_CFG_1
 * (ii) picks the TRUE (i.e. intended) destination address and transfers control.
 *
 * Power Off The Crystal Oscillator (26MHz)
 *
 * @param None
 *
 * @return N/A
 *
 */
void user_fastxip_workaround(void)
{
	u32_t win;
	MCU_SCRAM_FAST_XIP_s *fastxip = CRMU_GET_FAST_XIP_SCRAM_ENTRY();

	/* set to 0 to allow changing on windows */
	sys_write32(0 , SMU_CFG_1);

	/* the other windows */
	for (win = 1; win < SMU_CFG_NUM_WINDOWS; win++)
	{
		sys_write32(fastxip->smau_win_base_a[win - 1], SMU_WIN_BASE_0 + (win * 4));
		sys_write32(fastxip->smau_win_size_a[win - 1], SMU_WIN_SIZE_0 + (win * 4));
		sys_write32(fastxip->smau_hmac_base_a[win - 1], SMU_HMAC_BASE_0 + (win * 4));
	}

	/* restore SMU_CFG_1:
	 * Old SMU_CFG_1 value saved at CRMU_M0_SCRAM_START+0x2C
	 *
	 * sys_write32(fastxip->qspi_ctrl_cfg, SMU_CFG_1);
	 */
	sys_write32(fastxip->smau_cfg1, SMU_CFG_1);

	/* jump to real AAI image XIP address in flash
	 * AAI entry saved at CRMU_M0_SCRAM_START+0x3C
	 */
	((void (*)(void))fastxip->smc_cfg)();

	while (1)
		; /* no return */
}
#endif

