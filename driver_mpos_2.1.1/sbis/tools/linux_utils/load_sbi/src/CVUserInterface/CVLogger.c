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
/*
 * Include files
 */
#include <sys/stat.h> /* struct stat, fchmod (), stat (), S_ISREG, S_ISDIR */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "CVLogger.h"
#include "cvapi.h"
#include "linuxifc.h"

/*Purpose:
     To close the handles safely. This MACRO tries to close the NON-NULL 
	handles and set the handle to NULL after closing.
*/
#define CloseHandle_s(handle) if(handle != NULL)\
{\
	CloseHandle(handle);\
	handle = NULL;\
}\

/*
 * Variable declarations global to this file
 */

char			szCVUsrLibLogFileName[] = "CVUsrIfcLog.txt";

/*
 * Function:
 *      getSytemDateTime
 * Purpose:
 *      Returns the system date and time
 * Parameters:
 *      szDate          <-- date buffer
 *              szTime      <-- time buffer
 */
void
getDateTime(
			OUT     char *szDateTime
			)
{
	struct tm *ptr;
	time_t	lt;

	/* get the local system time */
	lt = time(NULL);
	ptr = localtime(&lt);
	strcpy(szDateTime, asctime(ptr));
}


/*
 * Function:
 *      _logMessage
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      sFmt            --> error string format
 */
void _logMessage(IN char *message)
{
	char*			szHostStorePath		= "/var/log";

		FILE *fpLogMsgFile;             /* Log File Handle*/
		char WriteBuff[LOGMESSAGELENGTH];   /* For consolidated log msg*/
		char szDateTime[32];                /* dateTime format: day mon hh:mm:ss year */
		char fileName[MAX_PATH];

			sprintf(fileName, "%s/%s", szHostStorePath, szCVUsrLibLogFileName);

			fpLogMsgFile = fopen(fileName, "a+");
			/* Non 0 Means Failure */
			if (fpLogMsgFile) {
				/* Get the system date and time when error occured */
				getDateTime(szDateTime);
				szDateTime[strlen(szDateTime)-1]=0;
				memset(WriteBuff, 0, sizeof(WriteBuff));
				sprintf(WriteBuff, "%s: %s", szDateTime, message);
				fputs(WriteBuff, fpLogMsgFile);
				/*Close the File*/
				fclose(fpLogMsgFile);
			}
			else
			{
				printf("Failed to open log file: %s\n", fileName);
				printf("_logMessage: %s", message);
			}
}

/*
 * Function:
 *      logException
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      dwCode          --> message code
 *              szFileName  --> file name
 *              szFunction --> function Name
 *      nLineNumber    --> line number
 */
DWORD
logException(
                IN      DWORD dwCode,
                IN      char *szFileName,
                IN      char *szFunction,
                IN      int nLineNumber
                )
{
	return 0;
}


/*
 * Function:
 *      logError
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      dwCode          --> message code
 *              szFileName  --> file name
 *              szFunction --> function Name
 *      nLineNumber    --> line number
 */
void
logError(
		 IN	DWORD dwCode, 
		 IN	char *szFileName, 
		 IN	char *szFunction, 
		 IN	int nLineNumber
		 )
{
	logMessage("code='0x%x' %s\\%s Ln: %d\n",
                           dwCode, szFileName, szFunction, nLineNumber);
}

/*
 * Function:
 *      logErrorMessage
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *              szLogMessage   --> error message
 *              szFileName  --> file name
 *              szFunction --> function Name
 *      nLineNumber    --> line number
 */
void
logErrorMessage(
                IN      char *szLogMessage,
                IN      char *szFileName,
                IN      char *szFunction,
                IN      int nLineNumber
                )
{
	/* log the error message */
	logMessage("%s:%d %s: %s\n", szFileName, nLineNumber, szFunction, szLogMessage);
}


/*
 * Function:
 *      logMessage
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      sFmt            --> error string format
 */
#ifdef ENABLE_DEBUG_LOGGING
void logMessage_func(const char *sFmt, ...)
{	
	char buff[1024];
	va_list args;
	va_start(args, sFmt);
	vsprintf(buff, sFmt, args);
	_logMessage(buff);
}
#endif


/*
* Function:
*   getLogDataPath
* Purpose:
*   Get the Host Command Buffer log path
* Parameters:
*	HostCmdBufferPath	:	Path for logging the Encapsulted command buffer
*
* Returns:
*	boolean: true in case of Success / false in case of failure
*
*/
BOOL
getLogDataPath(
			OUT	char* LogDataPath
			)
{
	return 1;
}

/*
 * Function:
 *      LogDataToFile
 * Purpose:
 *      This function will write the data in Hexa decimal notation (optional) to the file.
 * Parameters:
 *		sFileName		: Log file name
 *		sCaption		: Log Caption, any useful info.
 *		nCaptionSize	: Size of the Caption
 *		Data			: Actual data to be logged
 *		NoOfBytes		: Size of the Data
 *		bHexOutput		: Boolean parameter indicates whether data to be written in Hex format or not
 *
 * ReturnValue			: void
 */

void LogDataToFile(unsigned char *sFileName, unsigned char *sCaption, size_t nCaptionSize, unsigned char *Data, DWORD NoOfBytes, BOOL bHexOutput)
{
}

/*
 * Function:
 *      enableLogging
 * Purpose:
 *      This function will check registry setting and enable/disable logging.
 * Parameters:
 *						: void
 * ReturnValue			: void
 */

void enableLogging(void)
{
}
