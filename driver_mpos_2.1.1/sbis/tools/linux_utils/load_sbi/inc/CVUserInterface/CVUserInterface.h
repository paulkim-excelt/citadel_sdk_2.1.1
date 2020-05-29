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
#ifndef _CVUSERINTERFACE_H_
#define _CVUSERINTERFACE_H_ 1

#include "CVUIGlobalDefinitions.h"

typedef void (*LPFNTRANSMIT)(cv_encap_command**, cv_status*,  cv_encap_command**);

typedef cv_status (*LPFNUSERPROMPT)(cv_status callback_status, uint32_t ext_data_length, 
								byte* ext_data);

typedef cv_status (*LPFNPROMPTUSERPIN)(uint32_t uPINMsgCode, char *pPINData);

typedef cv_status (*LPFNHOSTSTOREREQUEST)(cv_encap_command*, cv_status*,
							cv_encap_command**);

typedef cv_status (*LPFNHOSTCONTROLREQUEST)(cv_encap_command*, cv_status*,
							cv_encap_command**);

typedef cv_status (*LPFNFPEVENTREGISTRATION)(cv_callback *callback,
							cv_callback_ctx context);

/*
 * Function:
 *      cvhTransmit
 * Purpose:
 *      Transmits  the  encapsulated  command to the USB device and  reads the 
 *		response from it. Synchronous,  asynchronous or  intermediate callback
 *      command processing is handled at CV User Mode Interface
 * Parameters:
 *		ppCommand		--> pointer to the encapslated command buffer
 *		pReturnStatus	<-- command execution status
 *		ppCommandResult	<-> pointer to the encapslated result buffer
 *		nMode			--> flag indicates synchronous/asynchronous
 */
void 
cvhTransmit(cv_encap_command **ppCommand, 
			cv_status *pReturnStatus, 
			cv_encap_command **ppCommandResult);

/*
 * Function:
 *      cvhUserPrompt
 * Purpose:
 *      Displays the user prompt for given prompt code. Handles the user
 *		cancellation
 * Parameters:
 *		uPromptCode			--> prompt code
 *		uExtraDataLength	--> length of the extra data
 *		pExtraData			--> pointer to the extra data
 * Returns:
 *      cv_status			<-- status of the prompt
 */
cv_status
cvhUserPrompt(cv_status uPromptCode, 
			  uint32_t uExtraDataLength, 
			  byte* pExtraData);
#endif												/* end of _CVUSERINTERFACE_H_ */
