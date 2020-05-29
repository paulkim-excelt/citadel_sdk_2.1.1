/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc and/or its subsidiaries.
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

#ifndef _CV_IF_H_
#define _CV_IF_H_

/**********************************************************
    includes
**********************************************************/

/**********************************************************
    defines
**********************************************************/

#define MaxAppIdLen         32
#define MaxUserIdLen        32

#define WaitSec				1000   //Number of milliseconds in a second
#define MaxUshWaitSecs		30     //30 seconds
#define MaxSBIWaitSecs		10     //10 seconds
#define MaxResanWaitSecs	20     //20 seconds
#define InitialWaitSecsAfterUpgradeInSBL 5   //5 seconds
#define InitialWaitSecsAfterReset 15   //15 seconds
#define WaitSecsAfterDriverReload  3   //3 seconds

#define RSA_SIZE_SIGNATURE_DEFAULT 256
#define RSA_SIZE_MODULUS_DEFAULT 256

typedef struct{
    uint32_t    chipId;
    uint32_t    custId;
} cvChipInfo;

extern cv_status	gCvRetStatus;

/**********************************************************
    routine prototype
**********************************************************/

cv_status cvif_get_version (
				cv_handle cvHandle,
				int buffLen,
				void *buff
				);
cv_status cvif_load_sbi(char *filename);
cv_status
cvif_load_flash (
				char     *filename,
				uint32_t offset,
				uint32_t iSbiCidChk
				);
cv_status cvif_reboot_to_sbi(void);
enum ush_chip_type_e
cvif_get_chip_type(DWORD nCIDToUse, bool bGetVer);
bool cvif_IsUshThere(void);

bool cvif_IsUshThere_TryAFewTimes(void);

bool cvif_UshBootWait(int waitTime);
int cvif_get_machine_id(uint8_t *txBuf, uint8_t *rxBuf);

bool cvif_GetCurrentUshVersion(char *pVerStr, int strBufLen);

cv_status cvif_GetLastError(void);

bool cvif_WasCVInSBLMode();
bool cvif_WasCVInErrorState();
bool cvif_WasCVWorkingInSBIOrAAI();


#endif //cv_if.h
