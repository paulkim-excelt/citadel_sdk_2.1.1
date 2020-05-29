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
#include "linuxifc.h"
#include "cvapi.h"

/*
 * Defintions - for logging error messges
 */
//#define ENABLE_DEBUG_LOGGING					/* Uncomment to enable */
#define MIN_LOG_SEMAPHORES_COUNT	1			/* initial semaphore count for log */
#define MAX_LOG_SEMAPHORES_COUNT	1			/* maximum number of semaphore resources */
#define LOG_SEMAPHORE_RELEASE_COUNT	1			/* semaphore release count */
#define LOGMESSAGELENGTH			1024		/* log message length */
#define ERROR_LOCATION	__FILE__, (char*)__FUNCTION__, __LINE__
#define BEGIN_TRY 
#define END_TRY 

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Function:
 *      getSytemDateTime
 * Purpose:
 *      To get the system date and time
 * Parameters:
 *      szDateTime		<-- date buffer
 */
void getDateTime(char *szDateTime);
/*
 * Function:
 *      logException
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      dwCode		--> message code
 *		szFileName  --> file name
 *		szFunction --> function Name
 *      nLineNumber    --> line number
 */
DWORD
logException(DWORD dwCode,
			 char *szFileName,
			 char *szFunction,
			 int nLineNumber);
/*
 * Function:
 *      logError
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      dwCode		--> message code
 *		szFileName  --> file name
 *		szFunction --> function Name
 *      nLineNumber    --> line number
 */
void
logError(DWORD dwCode,
		 char *szFileName,
		 char *szFunction,
		 int nLineNumber);
/*
 * Function:
 *      logErrorMessage
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *		szLogMessage   --> error message
 *		szFileName  --> file name
 *		szFunction --> function Name
 *      nLineNumber    --> line number

 */
void
logErrorMessage(char *szLogMessage,
				char *szFileName,
				char *szFunction,
				int nLineNumber);
/*
 * Function:
 *      logMessage
 * Purpose:
 *      Creates/appends log messages into the log file
 * Parameters:
 *      sFmt		--> error string format
 */
#ifdef ENABLE_DEBUG_LOGGING
#define logMessage(...)    logMessage_func(__VA_ARGS__)
#else
#define logMessage(...)
#endif

#ifdef _CVHOSTCTRLLOG
#define HostControlDbgMsg logMessage
#endif

#ifdef _CVHOSTSTORAGELOG
#define HostStorageDbgMsg logMessage
#endif


/*
* Function:
*   getLogDataPath
* Purpose:
*   Get the Host Command Buffer log path
* Parameters:
*	LogDataPath	:	Path of a file for logging the required data.
*
* Returns:
*	boolean: true in case of Success / false in case of failure
*
*/

BOOL
getLogDataPath(
			OUT char* LogDataPath
			);

/*
* Function:
*      LogDataToFile
* Purpose:
*      This function will write the data in Hexa decimal notation (optional) to the file.
* Parameters:
*		strFileName		: Log file name
*		strCaption		: Log Caption, any useful info.
*		nCaptionSize	: Size of the Caption
*		Data			: Actual data to be logged
*		NoOfBytes		: Size of the Data
*		HexOutput		: Boolean parameter indicates whether data to be written in Hex format or not
*
* ReturnValue			: void
*/
void
LogDataToFile(
			  unsigned char *sFileName,
			  unsigned char *sCaption,
			  size_t nCaptionSize,
			  unsigned char *Data,
			  DWORD NoOfBytes,
			  BOOL HexOutput
			  );

/*
 * Function:
 *      enableLogging
 * Purpose:
 *      This function will check registry setting and enable/disable logging.
 * Parameters:
 *						: void
 * ReturnValue			: void
 */

void enableLogging(void);

#ifdef __cplusplus
}
#endif

