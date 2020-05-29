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

#include <arch/cpu.h>
#include <board.h>
#include <string.h>
#include <sotp.h>
#include <post_log.h>
#include <regs.h>

#define CIMG_VERSION_STRING "convert_img"  " (" SBI_DATE " - " SBI_TIME ")"

int single_step_convert();

void reset_iproc(void)
{
    sys_write32(0x0, IPROC_CRMU_MAIL_BOX0);
    sys_write32(0xFFFFFFFF, IPROC_CRMU_MAIL_BOX1);
}

int sbi_main(void)
{
    sotp_bcm58202_status_t status = IPROC_OTP_VALID;
    struct sotp_bcm58202_dev_config config;

#ifdef SBI_DEBUG
    post_log("%s\n", CIMG_VERSION_STRING);
    post_log("compiled by %s\n", LINUX_COMPILE_BY);
    post_log("compile host %s\n",LINUX_COMPILE_HOST);
    post_log("compiler %s\n",LINUX_COMPILER);
    post_log("In %s()\n", __FUNCTION__);
#endif

#ifdef MPOS_CONVERT
    if (IPROC_OTP_VALID == sotp_is_unassigned()) {
        status = sotp_read_dev_config(&config);
        if (status != IPROC_OTP_VALID) {
            post_log("Read OTP failed!!!\n");
        } else {
            post_log("BRCMRevisionID = 0x%x, ProductID = 0x%x\n",
                      config.BRCMRevisionID, config.ProductID);
            post_log("Convert from unassigned to ABproduct\n");
            single_step_convert();
            post_log("Perform soft reset\n");
            reset_iproc();
        }
    } else {
        post_log("Skipping Convert because the part is not unassigned\n");
    }
#endif

BOOT_ERROR:
    while(1);
    return 0;
}
