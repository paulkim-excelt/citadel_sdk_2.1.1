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
#include "cvapi.h"
#include "cvcommon.h"
#ifndef _CVUITYPEDEFS_H_
#define _CVUITYPEDEFS_H_ 1
/*
 * command procesing mode
 */
typedef unsigned int cv_processing_mode;
enum cv_processing_mode_e{
	SYNCHRONOUS,                 /* synchronous mode */
	ASYNCHRONOUS                 /* asynchronous mode */
};

/*
 * prompt message type
 */
typedef unsigned short cv_prompt_type;
enum cv_prompt_type_e {
	PROMPT_INFORMATIVE,          /* informative user prompt id */
	PROMPT_CONFIRMATIVE,         /* confirmative user prompt id */
	PROMPT_ERROR                 /* error prompt id */
};

/*
 * prompt parameters - used by prompt dialog thread handlers
 */
typedef struct td_prompt_params {
	wchar_t* promptMessage;		/* message */		
	wchar_t* promptCaption;		/* message title */
	uint32_t uErrorCode;		/* message code */
	cv_prompt_type uErrorType;	/* message type */
	uint32_t uExtraDataLength;	/* length of extra data to be displayed */
	byte* pExtraData;		/* extra data to be displayed */
} PACKED prompt_params;
/*
 * thread parameters - used by thread handlers
 */
typedef struct td_thread_param {
	cv_fp_callback_ctx *callbackInfo;			/* callback function pointer */
	cv_encap_command *cmd;						/* pointer to the encapsulated command */
	cv_encap_command *res;						/* pointer to encapsulated result */	
	cv_processing_mode mode;
} PACKED thread_param;

#endif											/* #ifndef _CVUITYPEDEFS_H_ */


