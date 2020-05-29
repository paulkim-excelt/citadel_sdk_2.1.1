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
#ifndef _CVUIGLOBALDEFINITIONS_H_
#define _CVUIGLOBALDEFINITIONS_H_ 1
#ifdef _DEBUG
#include <io.h>
#include <tchar.h>
#include <strsafe.h>
#endif											/* #ifdef _DEBUG */
#include "cvapi.h"
#include "CVUITypesDefs.h"
#include "resource.h"
#include "CVUserInterfaceUtils.h"
#include "cvcommon.h"
#include <sys/sem.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <error.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include        <stdarg.h>              

/*
 * Macro defintions - for command processing & user prompt
 */
#define COMMAND_INDEX_START			0			/* command index starts with */
#define USER_PROMPT_NODISPLAY		0			/* no user prompt display */
#define USER_PROMPT_DISPLAYING		1			/* already displaying a prompt */
#define USER_PROMPT_CANCELLED		2			/* prompt cancelled by user */
#define USER_PROMPT_REMOVE			3			/* remove the user prompt */

#define MAX_DEVPATH_LENGTH			256			/* device path length */
#define BUFFER_SIZE					1024		/* string buffer length */
#define RESUBMISSION_TIMEOUT		10000		/* resubmission time out */
#define TIMEOUT_THREAD_EXITCODE		0			/* exit code for time out error */
/*
 * External variable definitions. Actual variables are defined in respective
 * implementation files. These variables can be referred any where in UI.
 */
extern uint32_t			uCommandIndex;			/* unique command index */
extern uint32_t			uPreviousPromptCode;	/* previous prompt code */
extern uint32_t			userPromptStatus;		/* prompt status */
extern prompt_params	promptInfo;				/* prompt details */

extern uint32_t			uPreviousCount;			/* previous semaphore count */
extern HANDLE			hTimeOutThread;			/* time out thread handle */
extern int				hSemaphore;				/* semaphore handle */
extern HANDLE			hThreadWait;			/* wait thread handle */
extern cv_fp_callback_ctx	*callbackInfo;		/* callback information */

typedef struct _write_buffer  {
        unsigned long cmd_length;                
        unsigned char cmd_buffer[1];
}write_buffer;

#endif											/* end of _CVUIGLOBALDEFINITIONS_H_ */
