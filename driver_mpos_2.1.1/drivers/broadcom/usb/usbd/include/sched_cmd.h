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

/*
 * @file sched_cmd.h
 *
 * @brief  Dummy file added for compilation purposes.
 * FIXME remove later.
 */


#define queue_cmd(cmd,from,task_woken)		(0)

/* cur_sys_state of the sched cmd task */
#define CUR_SYS_STATE_BOOT		(1 << 0)
#define CUR_SYS_STATE_RESTART		(1 << 1)
#define CUR_SYS_STATE_TPM_BOOT_HW_DONE	(1 << 2)
#define CUR_SYS_STATE_ROM_RESTART	(1 << 3)
#define CUR_SYS_STATE_SBI_LPC_INIT_BOOT		(1 << 4)
#define CUR_SYS_STATE_RESTART_HANDLED		(1 << 5)
#define CUR_SYS_STATE_INIT_DONE		(1 << 6)
#define CUR_SYS_STATE_LOW_PWR		(1 << 7)
#define CUR_SYS_STATE_WAITING_RESTART	(1 << 8)
#define CUR_SYS_STATE_WAKEUP_BOOT	(1 << 9)
#define CUR_SYS_STATE_USB_REENUM	(1 << 10)
#define CUR_SYS_STATE_CHECK_BTOB_LOW_PWR_CMD	(1 << 11)
#define CUR_SYS_STATE_TPM_RESTART	(1 << 12)
#define CUR_SYS_STATE_TPM_RUN_BOOT_HW	(1 << 13)
#define CUR_SYS_STATE_TPM_RUN_TPM_PROCESS	(1 << 14)
#define CUR_SYS_STATE_TPM_RUN_TPM_HASH	(1 << 15)
#define CUR_SYS_STATE_TPM_RUN_TPM_HASH_END	(1 << 16)
#define CUR_SYS_STATE_IN_APP_INIT		(1 << 17)
#define CUR_SYS_STATE_WAIT_FP_REENUM		(1 << 18)
#define CUR_SYS_STATE_GOING_TO_LOW_PWR		(1 << 19)

#define CUR_SYS_STATE_TPM_RUN_CMDS  (CUR_SYS_STATE_TPM_RUN_BOOT_HW  |	\
					CUR_SYS_STATE_TPM_RUN_TPM_PROCESS |	\
					CUR_SYS_STATE_TPM_RUN_TPM_HASH |	\
					CUR_SYS_STATE_TPM_RUN_TPM_HASH_END )



