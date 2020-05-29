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
#include "cvcommon.h"
#include "cvapi.h"      //For defines such as uint32_t etc.
#include "cvmain.h"
#include "cv_if.h"
#include "cnsl_utils.h"
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>

/**********************************************************
    defines
**********************************************************/

#define SYSTEM_ID_AMG           0x252
#define SYSTEM_ID_ENVY          0x2C8
#define SYSTEM_ID_MINICOOPER    0x277
#define SCD_OFFSET_CITADEL      0xfffc00
#define SCD2_OFFSET             0xfd50

/**********************************************************
    externs
**********************************************************/

extern uint32_t removeFlashWarnings;

/**********************************************************
    vars
**********************************************************/
const int DEFAULT_CID = 7;
char		*unknownStr = "Unknown";
char		tmpBPAfilename[] = "tmppbafl.bin";
int         bootstrap=FALSE;
char		*pbaAppFn="pbaapp.bin";
char		*pbaApp2Fn="pbaapp2.bin";
char		*pChipTypeStr[USH_CHIP_TYPE_MAX] =
			{	"Unknown",
				"CITADEL A0 CID1",
				"CITADEL A0 CID7",
				"CITADEL A0 Unassigned"};
char		*sbiFileNames[USH_CHIP_TYPE_MAX] =
             {  "",
				"bcmsbiCitadelA0_1.otp",
				"bcmsbiCitadelA0_7.otp",
				"bcmsbiCitadelA0_Unassigned.otp"};
char		*bcmFileNames[USH_CHIP_TYPE_MAX] =
            {   "",
				"bcmCitadel_1.otp",
				"bcmCitadel_7.otp",
				"bcmCitadel_1.otp",};
DWORD		cidNumbers[USH_CHIP_TYPE_MAX] = 
			{	0,
				1,
				7,
				1};
char		*broadcomStr = "broadcom";
char		*clearScdFileName = "bcm_cv_clearscd.bin";
char		szFWFolderPath[MaxFileNamelen+1] = {'\0'};   //Init to NULL path
char		sbi_file[MaxFileNamelen + 1] = {'\0'};
char		image_file[MaxFileNamelen + 1] = {'\0'};

/**********************************************************
    routine prototypes
**********************************************************/

int	cvLoadSbi(DWORD nCidToUse);
enum ush_chip_type_e cvCalculateChipTypeAfterSBILoadBasedONCIDToUseAndPID(DWORD nCidToUse, int pid);
void cvLoadCvDriver(void);

/**********************************************************
routine definitions
**********************************************************/

bool isTheCurrentSBIVersionUpToDate(char* currentVer, char* proposedVer)
{

	size_t currentVerLen = strlen(currentVer);
	size_t proposedVerLen = strlen(proposedVer);
	if ((currentVerLen == proposedVerLen) && (!strncmp(currentVer, proposedVer, currentVerLen)))
	{
		cnslInfo(CNSL_DATA, "SBI version matches - it is up do date\n");
		return true;
	}

	return false;
}

/* 
* Function:
*       main
* Purpose:
*       This is the main function calling cvFWUpgradeMain
* Parameters:
*       IN  
*		OUT 
*		OUT 
* ReturnValue:
* 
*/ 
int main(int argc, char *argv[])
{
	enum ush_detailed_upgrade_result_e detailedUpgradeResult;
	DWORD nCIDAfterASuccessfulUpgrade;
	mainParams_t mainParams;
	DWORD nCID = DEFAULT_CID;
	bool forceUpdate = false;

	mainParams.op_mode = 0;

	/* Get the current working directory */
	getcwd(szFWFolderPath, sizeof(szFWFolderPath));

	if(argc == 2)
	{
		if(!strcasecmp(argv[1], "-h") || !strcasecmp(argv[1], "-help"))  
		{
			cnslPrintf("Usage:\n");
			cnslPrintf("Upgrading ControlVault firmware: sudo CVFWUpgrade <Sbi image> <Image>\n");
			return 0;
		}
		else
		{
			cnslInfo(CNSL_ERR, "Input parameter not supported, for usage help, enter: CVFWUpgrade -h\n");
			return 3;
		}
	} else if (argc == 3) {
		struct stat stats;

		/* Check for sbi file */
		if (argv[1][0] == '/') {
			strcpy(sbi_file, argv[1]);
		} else {
			strcpy(sbi_file, szFWFolderPath);
			strcat(sbi_file, "/");
			strcat(sbi_file, argv[1]);
		}
		if (stat(sbi_file, &stats) != 0) {
			printf("Please check whether '%s' file exists.\n", sbi_file);
			return 5;
		}

		/* Check for image file */
		if (argv[2][0] == '/') {
			strcpy(image_file, argv[2]);
		} else {
			strcpy(image_file, szFWFolderPath);
			strcat(image_file, "/");
			strcat(image_file, argv[2]);
		}
		if (stat(image_file, &stats) != 0) {
			printf("Please check whether '%s' file exists.\n", image_file);
			return 5;
		}
	}
	else if(argc != 1)
	{
		cnslPrintf("Wrong number of arguments, for usage help, enter: CVFWUpgrade -h\n");
		return 4;
	}

	//Unlink to remove the CV semaphore
	sem_unlink(CV_SEM_NAME);

    // Remove the file in case it's there due to last abnomral termination of the program
    remove(CV_SEM_FILE_NAME);

    // Init to default CID 
    nCID = DEFAULT_CID;

	//The other member (cvAdminPassword) in the structure is not used.    
    if (nCID == VAL_CID1) { mainParams.op_mode |= OP_CID1; }
    if (nCID == VAL_CID7) { mainParams.op_mode |= OP_CID7; }
    if (nCID == VAL_CIDFE) { mainParams.op_mode |= OP_CIDFE; }

	cvFWUpgradeMain(&mainParams, forceUpdate, &nCIDAfterASuccessfulUpgrade, &detailedUpgradeResult);

    sem_unlink(CV_SEM_NAME);
	
	return 0;
}


/* 
* Function:
*       cvFWUpgradeMain
* Purpose:
*       This is the main firmware upgrade util
* Parameters:
*       IN  pMainParams - the command parameters
*		OUT nCIDAfterASuccessfulUpgrade - After an upgrade return the CID of the chip
*   	OUT detailedUpgradeResult - Specific result of upgrade
* ReturnValue:
*       n/a
*/
int cvFWUpgradeMain(mainParams_t *pMainParams, bool forceUpdate, DWORD *nCIDAfterASuccessfulUpgrade, enum ush_detailed_upgrade_result_e *detailedUpgradeResult)
{
    cv_status           status;
	char				*pFlName = image_file;
	char				ushSbiVer[64];
	char				fileSbiVer[64];
	DWORD				nCIDToUse = 0;
	bool				inSBLMode = false;

	*nCIDAfterASuccessfulUpgrade = 0;

	if(cvif_WasCVInSBLMode()) 
	{
		inSBLMode = true;
	}

	if (!inSBLMode && !cvif_IsUshThere_TryAFewTimes()) {
		cnslInfo(CNSL_ERR, "Could NOT detect USH.\n");
		return USH_ERROR_USB;
	}

    // For SBL mode, the ushVerBuf is already filled when SBL mode was determined, don't get it again
    if (!inSBLMode) {
        cnslInfo(CNSL_DATA, "Control Vault getting chip type\n");
    }
    
	if (pMainParams->op_mode & OP_CID1) { nCIDToUse = VAL_CID1; }
	if (pMainParams->op_mode & OP_CID7) { nCIDToUse = VAL_CID7; }
	if (pMainParams->op_mode & OP_CIDFE) { nCIDToUse = VAL_CIDFE; }

	cnslInfo(CNSL_DATA, "Event: FwUpgradeStarted\n");

	if (inSBLMode) {
		// in boot strap 
		cnslInfo(CNSL_DATA, "Control Vault executing from Boot Strap - Loading SBI\n");
        if (!cvLoadSbi(nCIDToUse)) {
			cnslInfo( CNSL_ERR, "ERROR: Failed to load SBI\n");
            return USH_RC_LOADSBI_ERR;
        }
		bootstrap=TRUE;
	}
    else if(!isTheCurrentSBIVersionUpToDate(ushSbiVer,fileSbiVer) || forceUpdate) // Check if the USH Sbi versions are mismatched 
	{
	   // If yes then we need to upgrade the SBI also
	   // Do a clear SCD for the SBI and go back to the SBI mode 	   
	   cnslInfo( CNSL_DATA, "Upgrade the SBI from %s to %s, first clear SCD \n",ushSbiVer,fileSbiVer);
	   status = cvif_load_flash(clearScdFileName, SCD_OFFSET_CITADEL, FALSE);
	   if ( status != CV_SUCCESS ) {
	        cnslInfo( CNSL_ERR, "ERROR: Failed to clear SCD, status: 0x%x\n", status );
		    return USH_RC_CLR_SCD_ERR;
	   }
	   cnslInfo( CNSL_ERR, "Reset to SBI to update the SBI....\n" );
       cvif_reboot_to_sbi();
	   if ( !cvif_UshBootWait(5) ) {
			cnslInfo( CNSL_ERR, "ERROR: Control Vault did not comeback from reset to SBI \n" );
			return USH_RC_NOUSH_ERR;
	   }
       bootstrap = TRUE;
	}

	//
	// flash SBI
    //
    if(bootstrap == TRUE )
    {
		cnslInfo( CNSL_DATA, "Going to update SBI (%s)\n", pFlName );
		status = cvif_load_flash(pFlName, 0, FALSE);
		if ( status != CV_SUCCESS ) {
			cnslInfo( CNSL_ERR, "ERROR: Failed to update SBI, status: 0x%x\n", status );
			if ( status == CV_FEATURE_NOT_AVAILABLE )
				return USH_RC_TPM_ERR;
			else if ( status == CV_ANTIHAMMERING_PROTECTION_FLSHPRG )
				return USH_RC_AH_ERR;
			else if ( status == CV_INVALID_VERSION )
				return USH_RC_ERROR;
			else
				return USH_RC_ERROR;
		}
	}
	cnslInfo( CNSL_DATA, "SUCCESS\n");
	return USH_RC_SUCCESS;
}

char *cvRetSbiFileName(int chiptype)
{
    return sbiFileNames[chiptype];
}

char *cvRetBcmFileName(void)
{
    return bcmFileNames[cvif_get_chip_type(0, TRUE)];
}

int cvFilesPresent(int chipType)
{
	char			*pFileNamePtr;
    FILE			*fp;

    // check sbi file
	pFileNamePtr = sbiFileNames[chipType];
	fp = cvOpenFile(pFileNamePtr, "rb", NULL);
	if ( fp==NULL ) {
	    cnslInfo(CNSL_ERR, "Can not find SBI file (%s)\n", pFileNamePtr);
		return CV_INVALID_INPUT_PARAMETER;
    }
    fclose(fp);
	

	// check for clear scd
	pFileNamePtr = clearScdFileName;
	fp = cvOpenFile(pFileNamePtr, "rb", NULL);
	if ( fp==NULL ) {
	    cnslInfo(CNSL_ERR, "Can not find Clear SCD file (%s)\n", pFileNamePtr);
		return CV_INVALID_INPUT_PARAMETER;
    }
    fclose(fp);

	// check for bcm
	pFileNamePtr = bcmFileNames[chipType];
	fp = cvOpenFile(pFileNamePtr, "rb", NULL);
	if ( fp==NULL ) {
	    cnslInfo(CNSL_ERR, "Can not find BCM file (%s)\n", pFileNamePtr);
		return CV_INVALID_INPUT_PARAMETER;
    }
    fclose(fp);

	return CV_SUCCESS;
}

enum ush_chip_type_e cvCalculateChipTypeAfterSBILoadBasedONCIDToUseAndPID(DWORD nCidToUse, int pid)
{
	if (0x5840 == pid)
	{
		if (nCidToUse == 1) { return USH_CHIP_TYPE_CITADEL_A0_CID1; }
		if (nCidToUse == 7) { return USH_CHIP_TYPE_CITADEL_A0_CID7; }
	}

	return USH_CHIP_TYPE_UNKNOWN;
}

int cvLoadSbi(DWORD nCidToUse)
{
    char        *sbi = sbi_file;
    FILE        *sbiFp;
    cv_status   status;
	enum ush_chip_type_e chipType;
    char		sbiFn[MaxFileNamelen];

	chipType = cvif_get_chip_type(nCidToUse, TRUE);
	
    cnslInfo( CNSL_DATA, "Detected chipType 0x%x\n", chipType );

    sbiFp = cvOpenFile(sbi, "rb", sbiFn);
    if ( sbiFp==NULL ) {
        cnslInfo( CNSL_WARN, "Could not find SBI file %s\n", sbi);
        return FALSE;
    }
    fclose(sbiFp);
    cnslInfo( CNSL_DATA, "Going to load SBI (%s)\n", sbiFn );
    status = cvif_load_sbi(sbiFn);
	cnslInfo( CNSL_DATA, "cvif_load_sbi() returns status: %d\n", status );
 
	cnslSleepWTick(InitialWaitSecsAfterUpgradeInSBL);

	if (!cvif_WasCVWorkingInSBIOrAAI()) {
		cnslInfo(CNSL_WARN, "cv_get_ush_ver did not return CV_SUCCESS. Error: 0x%x\n", cvif_GetLastError());
		return FALSE;
    }

    return TRUE;
}

FILE *cvOpenFile(char *inFileName, char *fileAttr, char *outFileName)
{
	char  fileName[MaxFileNamelen + 1];
    FILE  *fp = NULL;

	if((strlen(szFWFolderPath) + strlen(inFileName)) > MaxFileNamelen)
	{
		cnslInfo(CNSL_ERR, "Combined FW folder and file path too long\n");
		return fp;
	}

	strcpy(fileName, inFileName);

	fp = fopen(fileName, fileAttr);
	if(fp) {
		cnslInfo(CNSL_DATAV, "Verified FW file %s exists\n", fileName);
		if (outFileName)
			strcpy(outFileName, fileName);		
	}

	return fp;
}
