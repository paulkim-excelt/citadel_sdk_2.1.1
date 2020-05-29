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
/**********************************************************
    includes
**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>    // For sleep functions
#include "cvapi.h"     //For defines such as uint32_t etc.
#include "cvmain.h"
#include "cnsl_utils.h"
#include "cv_if.h"

/**********************************************************
    defines
**********************************************************/

#define SbiSegSize              0x10000
#define MaxDownloadSize         0xc00
#define FlashUpgradeStartLen    0x2b0
#define FlashUpgradeUpdateLen   0xc00
#define FlashUpgradeCompleteLen 0x100
#define MAX_ENCAP_RESULT_BUFFER 4096
#define AhResetStr			"USH_REBOOT_CNT:"
#define AhResetZeroStr			"USH_REBOOT_CNT:00000000"
#define AhFlshWrtStr			"USH_FLSHWR_CNT:"
#define AhFlshWrtZeroStr		"USH_FLSHWR_CNT:00000000"
#define ReprogramSuccessStr		"REPROGRAM_SUCCESS:00000001"
#define SBIModeStr			"USH_SBI_MODE:00000001"
#define AESKeyValidStr			"USH_AES_KEY_VALID:00000001"

#define VersionFileName	"bcm_cv_current_version.txt"

/**********************************************************
    vars
**********************************************************/

uint8_t		ushVerBuf[VerBufLen];
uint32_t    clrObjFlags = CV_OBJ_AUTH_READ_PUB|CV_OBJ_AUTH_READ_PRIV|CV_OBJ_AUTH_WRITE_PUB|
                        CV_OBJ_AUTH_WRITE_PRIV|CV_OBJ_AUTH_DELETE|CV_OBJ_AUTH_ACCESS|
                        CV_OBJ_AUTH_CHANGE_AUTH|CV_ADMINISTRATOR_AUTH;
bool		gChipIs5882 = true;
cv_status	gCvRetStatus = CV_SUCCESS;
uint8_t     gInitBuffer[64];

/**********************************************************
    routine definitions
**********************************************************/

/* 
* Function:
*       cvif_display_status_str
* Purpose:
*       displays the error string
* Parameters:
*/
void
cvif_display_status_str(
            uint32_t    status
            )
{
    if (status == CV_ANTIHAMMERING_PROTECTION)
        cnslInfo(CNSL_ERR, "Error: Antihammering protection triggered\n");
}

/* 
* Function:
*       cv_load_sbi
* Purpose:
*       This loads the sbi
*
* Parameters:
*       IN filename     : sbi filename
* ReturnValue:
*       cv_status
*/

cv_status
cvif_load_sbi (
            char *filename
            )
{
	cv_status	status;
    
    status = cv_load_sbi((char*)filename);
	// save return status
	gCvRetStatus = status;
    if (status != 0) {
		cnslInfo(CNSL_ERR, "cv_load_sbi returned 0x%x\n", status);
        return status;
    }
	return status;
}

extern cv_version version;
void BIP_SetVersion (
/* in */ cv_version appVersion)
{
        version.versionMajor = appVersion.versionMajor;
        version.versionMinor = appVersion.versionMinor;
}

cv_status
cvif_get_version (
            cv_handle cvHandle,
            int buffLen,
            void *buff
            )
{
	cv_status	status;
	
	status = cv_get_ush_ver((cv_handle)cvHandle, buffLen, (uint8_t*)buff);
	// save return status
	gCvRetStatus = status;
	if (status==CV_INVALID_VERSION) {
		cv_version backVer = {1,0};
		//uint16_t	versionMajor;	/* major version of CV */
		//uint16_t	versionMinor;	/* minor version of CV */
		BIP_SetVersion(backVer);
		status = cv_get_ush_ver((cv_handle)cvHandle, buffLen, (uint8_t*)buff);
		// save return status
		gCvRetStatus = status;
	}
    if (status != 0) {
		cnslInfo(CNSL_ERR, "Error: 0x%x from cv_get_ush_ver()\n", status);
    }
	return status;
}


/* 
* Function:
*       cv_load_flash
* Purpose:
*       This loads the flash image
*
* Parameters:
*       IN  *filename   : flash filename
*       IN  offset      : offset into flash where image is to be loaded
* ReturnValue:
*       cv_status
*/

cv_status
cvif_load_flash (
            char     *filename,
            uint32_t offset,
            uint32_t iSbiCidChk
            )
{
    cv_status   status = CV_SUCCESS;
    FILE        *flshFp;
    char		flshFn[MaxFileNamelen];

    // Validating filename
    if ( filename == 0) {
        cnslInfo(CNSL_ERR, "cv_load_flash filename is null\n");
        return CV_INVALID_INPUT_PARAMETER;
    }

	flshFp = cvOpenFile(filename, "rb", flshFn);
    if ( flshFp==NULL ) {
	    cnslInfo(CNSL_ERR, "Can not open flash file (%s)\n", filename);
		return CV_INVALID_INPUT_PARAMETER;
    }

	cnslInfo(CNSL_DATA, "Writing %s to flash\n", flshFn);
        //Added casting to cv_handle to get rid of compilation error
	status = cv_flash_update((cv_handle)0, (char*)flshFn, offset);
	// save return status
	gCvRetStatus = status;
	if (status != CV_SUCCESS) {
		cnslInfo(CNSL_ERR, "Error: 0x%x from cv_flash_update()\n", status);
	}
	else {
		cnslInfo(CNSL_DATA, "cv_flash_update() successful\n");
	}

	return status;
}

/* 
* Function:
*       cv_reboot_to_sbi
* Purpose:
*       This reboots the ush to sbi
*
* Parameters:
* ReturnValue:
*       cv_status
*/

cv_status
cvif_reboot_to_sbi (
                    void
					)
{
	cv_status	status;
    
	// get console data
	status = cv_reboot_to_sbi();
	// save return status
	gCvRetStatus = status;
	if (status != CV_SUCCESS) {
		//display console data
		cnslInfo(CNSL_ERR, "Could not reset to SBI (0x%x)\n", status);
    }

	return status;
}

enum ush_chip_type_e
cvif_get_chip_type(DWORD nCIDToUse, bool bGetVer)
{
    cv_status       status = CV_SUCCESS;

    // This function may be called with the ushVerBuf already filled
    if (bGetVer) {
        memset(ushVerBuf, 0, VerBufLen);
        status = cvif_get_version( 0, VerBufLen, (void*)ushVerBuf);
        if ( status != CV_SUCCESS )
        {
                if (nCIDToUse == 1) {
                        return USH_CHIP_TYPE_CITADEL_A0_CID1;
                }
                else if (nCIDToUse == 7) {
                        return USH_CHIP_TYPE_CITADEL_A0_CID7;
                }
                else if (nCIDToUse == 0xFE) {
                        cnslInfo(CNSL_ERR, "CID FE is currently not supported\n");
                        return USH_CHIP_TYPE_UNKNOWN;
                }            
                return USH_CHIP_TYPE_UNKNOWN;
        }
    }

    // PKB to be removed later when all parts are properly configured with ChIp ID 
    if ( strstr( (char*)ushVerBuf, ChipTypeCitadelA0Str) || strstr( (char*)ushVerBuf, ChipTypeCitadelUnconfigStr)) {
            if (strstr((char*)ushVerBuf, ChipCID_CitadelCID1Str))
            {
                    cnslInfo(CNSL_DATA, "Citadel A0 CID1 Chip Found....\n");
                    return USH_CHIP_TYPE_CITADEL_A0_CID1;
            }
            if (strstr((char*)ushVerBuf, ChipCID_CitadelCID7Str))
            {
                    cnslInfo(CNSL_DATA, "Citadel A0 CID7 Chip Found....\n");
                    return USH_CHIP_TYPE_CITADEL_A0_CID7;
            }
            if (strstr((char*)ushVerBuf, ChipCID_CitadelUnasignStr))
            {
                    cnslInfo(CNSL_DATA, "Citadel A0 Unassigned Chip Found....\n");
                    return USH_CHIP_TYPE_CITADEL_A0_UNASSIGNED;
            }

    }
    return USH_CHIP_TYPE_UNKNOWN;
}

bool
cvif_IsUshThere(
				void
				)
{
	cv_status	status;
	cv_handle	cvhandle=0;
	uint32_t	buflen=VerBufLen;
	
	memset(ushVerBuf, 0, buflen);
	status = cv_get_ush_ver(cvhandle, buflen, ushVerBuf);
	cnslInfo(CNSL_DATA, "In cvif_IsUshThere(), cv_get_ush_ver() status: (0x%x)\n", status);
	// save return status
	gCvRetStatus = status;
	if (status==CV_INVALID_VERSION) {
		cv_version backVer = {1,0};
		//uint16_t	versionMajor;	/* major version of CV */
		//uint16_t	versionMinor;	/* minor version of CV */
		BIP_SetVersion(backVer);
		status = cv_get_ush_ver(cvhandle, buflen, ushVerBuf);
		// save return status
		gCvRetStatus = status;
	}
	if (status==CV_SUCCESS || status==CV_USH_BOOT_FAILURE)
        return TRUE;

	return FALSE;
}

bool
cvif_IsUshThere_TryAFewTimes()
{
	int NUMBER_OF_SECONDS_TO_TRY = 5;
	int i;

	for (i = 0; i < NUMBER_OF_SECONDS_TO_TRY; ++i)
	{
		cvif_IsUshThere();
		if (!cvif_WasCVInErrorState())
		{
			return TRUE;
		}
		cnslSleepMS(1000);
	}

	return FALSE;
}

/* Citadel SBL supports CV_CMD_USH_VERSIONS command */
bool cvif_WasCVInSBLMode()
{
    cv_status       status = CV_SUCCESS;
    memset( ushVerBuf, 0, VerBufLen );
    status = cvif_get_version( 0, VerBufLen, (void*)ushVerBuf);
    if ( status == CV_SUCCESS )
    {
    	if(strstr((char*)ushVerBuf, ChipCID_CitadelSBLStr))
		{
			cnslInfo(CNSL_DATA, "Citadel A0 Chip Found and it is SBL mode....\n");
			return true;
		}
	}
	return false;
}

bool cvif_WasCVInErrorState()
{
	if (CV_USH_BOOT_FAILURE == gCvRetStatus || CV_SUCCESS == gCvRetStatus)
		return false;
	return true;
}

bool cvif_WasCVWorkingInSBIOrAAI()
{
	if (CV_SUCCESS == gCvRetStatus)
		return true;
	return false;
}

bool
cvif_UshBootWait(
				 int waitTime
				 )
{
	int idx;
	time_t start,end;
	double dif;
	int	secs;
	for (idx=0; idx<waitTime; idx++) {
		cnslSleepMS(WaitSec);
		time(&start);
		if ( cvif_IsUshThere() )
			return TRUE;
		time(&end);
		dif = difftime(end, start);
		secs = (int)dif;
		if (secs > 1)
			idx+=(secs-1);
	}

	// rescaned devices OK now wait to see if USH somes up.
	for (idx=0; idx<MaxResanWaitSecs; idx++) {
		cnslSleepMS(WaitSec);
		time(&start);
		if ( cvif_IsUshThere() )
			return TRUE;
		time(&end);
		dif = difftime(end, start);
		secs = (int)dif;
		if (secs > 1)
			idx+=(secs-1);
	}

	return FALSE;
}

// This function uses the results of a previous call to cvif_IsUshThere or cvif_CheckAndSaveCVState
bool
cvif_GetCurrentUshVersion(
						  char *pVerStr,
						  int strBufLen
						  )
{
	// The Current USH Version is obtained from CV Get USH Version.
	//	USH_REL_VER:01010600
	//	USH_REL_UPGRADE_VER:01020104
	//	USH_REL_BUILD_VER:00000000
	//	USH firmware verion is Bb.R.F.M
	// B is obtained from USH_REL_UPGRADE_VER[3]
	// b is obtained from USH_REL_UPGRADE_VER[2]
	// R is obtained from USH_REL_VER[1]
	// F is obtained from USH_REL_UPGRADE_VER[1..0]
	// M is obtained from USH_REL_BUILD_VER[3]
	uint32_t	verVal=0;
	char		*pVerBuf=(char*)ushVerBuf, *pVer;
	uint8_t		vb, vM;
	uint16_t	vF1, vF2;

	if (pVerStr==NULL) {
		// no string pointer
		return false;
	}

	// get b, F
	pVer = strstr( pVerBuf, "USH_REL_UPGRADE_VER:" );
	if ( pVer==NULL ) {
		return false;
	}
	sscanf(pVer, "USH_REL_UPGRADE_VER:%x", &verVal);
	vb = (verVal>>16)&0xff;
	vF1 = (verVal>>12)&0xf;
	vF2 = (verVal)&0x0fff;

	// get M
    pVer = strstr( pVerBuf, "USH_REL_BUILD_VER:" );
	if ( pVer==NULL ) {
		return false;
	}
	sscanf(pVer, "USH_REL_BUILD_VER:%x", &verVal);
	vM = (verVal>>24)&0xff;

	//check if 5882, if so then inc file vR
	pVer = strstr( pVerBuf, "USH_CHIPID:5882" );
	if ( pVer!=NULL ) {
		gChipIs5882 = true;
	}
	else {
		gChipIs5882 = false;
	}

	sprintf(pVerStr, "%x.%x.%x.%x", vb, vF1, vF2, vM);
	return true;
}

cv_status cvif_GetLastError(void)
{
	return gCvRetStatus;
}
