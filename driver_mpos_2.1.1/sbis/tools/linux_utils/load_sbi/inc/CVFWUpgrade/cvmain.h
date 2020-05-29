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
#ifndef __CVMAIN_H__
#define __CVMAIN_H__

/**********************************************************
    includes
**********************************************************/

#include "ushupgrade.h"
#include <stdio.h>
#include <stdbool.h>

/**********************************************************
    defines
**********************************************************/

#define VerBufLen      2048

#define ChipTypeCitadelUnconfigStr	"USH_CHIPID:00000000"
#define ChipTypeCitadelA0Str		"USH_CHIPID:05820201"
#define ChipCID_CitadelCID1Str		"USH_CUST_ID:60000002"
#define ChipCID_CitadelCID7Str		"USH_CUST_ID:60000001"
#define ChipCID_CitadelUnasignStr	"USH_CUST_ID:00000000"
#define ChipCID_CitadelSBLStr		"USH_SBL_MODE: 00000001"
#define ChipCID_CitadelSBIStr		"USH_SBI_MODE:00000001"

enum ush_chip_type_e {
	USH_CHIP_TYPE_UNKNOWN,			// 0
	USH_CHIP_TYPE_CITADEL_A0_CID1,		// 1
	USH_CHIP_TYPE_CITADEL_A0_CID7,		// 2
	USH_CHIP_TYPE_CITADEL_A0_UNASSIGNED,	// 3
	USH_CHIP_TYPE_MAX				// 4
};

enum ush_detailed_upgrade_result_e {
	UPGRADE_RESULT_UNSPECIFIED,
	UPGRADE_RESULT_FIRMWARE_ALREADY_UPTODATE
};

/**********************************************************
    routine prototypes
**********************************************************/

int cvFWUpgradeMain(mainParams_t *pMainParams, bool forceUpdate, DWORD *nCIDAfterASuccessfulUpgrade, enum ush_detailed_upgrade_result_e *detailedUpgradeResult);
int cvRemainingFwUpgardeSteps();
char *cvRetSbiFileName(int chiptype);
char *cvRetBcmFileName(void);
int cvFilesPresent(int chipType);
FILE *cvOpenFile(char *inFileName, char* fileAttr, char *outFileName);

#endif //__CVMAIN_H__
