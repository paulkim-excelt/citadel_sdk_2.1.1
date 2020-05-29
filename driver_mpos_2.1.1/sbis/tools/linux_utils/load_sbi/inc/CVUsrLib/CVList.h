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
#ifndef _CVLIST_H_
#define _CVLIST_H_ 1

/*
* Include file 
*/
#include "cvapi.h"
#include "cvusrlib.h"

/*
* List item that holds data
*/
typedef struct st_SessionList
{
	void *pAddressOfCVSession;
	struct st_SessionList *pNext;
}SessionList;

/*Declarations*/

/*
* Function:
*		AddNewSession
* Description:
*		Create and Add the new handle in the list
* Parameter
*		cvSession		: Store the input data 
* Return
*		Bool	
*
*
*/
cv_bool AddNewSession(uint32_t *cvSession);

/*
* Function:
*		DeleteSessionHandle
* Description:
*		Delete the handle in the List
* Parameter
*		cvHandle		: Handle as a input data 
* Return
*		Bool	
*/
cv_bool DeleteSessionHandle(cv_handle cvHandle);

/*
* Function:
*		DeleteAllButSessionHandle
* Description:
*		Delete all the handles in the List but the one provided
* Parameter
*		cvHandle		: CV internal Handle as a input data 
* Return
*		Bool	
*/
cv_bool DeleteAllButSessionHandle(cv_handle cvInternalHandle);

/*
* Function:
*		GetSessionOfTheHandle
* Description:
*		Find the associated Session Structure of the cvHandle in the Session List
* Parameter
*		cvHandle		: Handle as a input data 
* Return
*		void *	
*/
void *GetSessionOfTheHandle(cv_handle cvHandle);

/*
* Function:
*		GetSessionOfTheCVInternalHandle
* Description:
*		Find the associated Session Structure of the cvInternalHandle in the Session List
* Parameter
*		cvInternalHandle		: cvInternalHandle as a input data 
* Return
*		void *	
*/
void *GetSessionOfTheCVInternalHandle(cv_handle cvInternalHandle);

/*
* Function:
*		IsSessionListEmpty
* Description:
*		Checks whether SessionList is empty or not.
* Parameter:
*		NIL
*		
* Return
*		Bool	(Found or not)
*
*
*/
cv_bool IsSessionListEmpty();
#endif
