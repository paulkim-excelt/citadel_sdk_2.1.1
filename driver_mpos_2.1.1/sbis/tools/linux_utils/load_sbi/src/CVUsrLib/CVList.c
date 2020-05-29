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
/*Include Files*/
#include <stdlib.h>
#include <string.h>
#include "CVList.h"
//#include "CVUsrLibGlobals.h"

/*Initilize the global variables*/
SessionList *pSessionHeadNode = NULL;
SessionList *pSessionTailNode = NULL;
cv_bool bIsSessionHeadNode = TRUE;


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
cv_bool AddNewSession(uint32_t *cvSession)
{
	SessionList *pNewNode = NULL;

	/*Allocate mem for new List*/
	pNewNode = (SessionList *) malloc(sizeof(SessionList));
	if(NULL == pNewNode)
	{
		return FALSE ;
	}

	memset(pNewNode,0,sizeof(SessionList));

	/*Copy the data*/

	pNewNode->pAddressOfCVSession = cvSession;
	pNewNode->pNext = NULL;

	/*Set new list into pSessionTailNode and pSessionTailNode->pNext*/
	if(bIsSessionHeadNode == TRUE) 
	{
		pSessionTailNode = pNewNode;
		pSessionHeadNode = pNewNode;
		bIsSessionHeadNode = FALSE;
	}
	else
	{
		pSessionTailNode->pNext = pNewNode;
		pSessionTailNode = pNewNode;
	}
	
	return TRUE;
}

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

cv_bool DeleteSessionHandle(cv_handle cvHandle)
{
	cv_bool retVal = FALSE;
	SessionList *ptmpSessionNode = pSessionHeadNode;
	SessionList *ptmpPrvSessionNode = pSessionHeadNode;
	void *SessionStartAddress = NULL;

	SessionStartAddress = GetSessionOfTheHandle(cvHandle);

	while(ptmpSessionNode != NULL)
	{
		if(ptmpSessionNode->pAddressOfCVSession == SessionStartAddress)
		{
			if(ptmpSessionNode == pSessionHeadNode)
			{
				pSessionHeadNode = ptmpSessionNode->pNext;

				//if ptmpSessionNode->pNext == NULL, reset the head and tail node 
				if(ptmpSessionNode->pNext == NULL)
				{
					pSessionTailNode = NULL;
					bIsSessionHeadNode = TRUE;
				}
			}
			else
			{
				//Append the pervious session node 
				ptmpPrvSessionNode->pNext = ptmpSessionNode->pNext;

				//Append the previous session node if the last node is not null
				if(NULL == ptmpPrvSessionNode->pNext)
                    pSessionTailNode = ptmpPrvSessionNode;

			}

			//Free the current Session node
			if(ptmpSessionNode->pAddressOfCVSession)
				free(ptmpSessionNode->pAddressOfCVSession);

			if(ptmpSessionNode)
				free(ptmpSessionNode);

			ptmpSessionNode = NULL;

			retVal = TRUE;
			break;
		}

		ptmpPrvSessionNode = ptmpSessionNode;
		ptmpSessionNode = ptmpSessionNode->pNext;
	}

	return retVal;
}

/*
* Function:
*		DeleteAllButSessionHandle
* Description:
*		Deletes all the handles in the List except the one passed in
* Parameter
*		cvHandle		: CV internal Handle as a input data 
* Return
*		Bool	
*/

cv_bool DeleteAllButSessionHandle(cv_handle cvHandle)
{
	SessionList *ptmpSessionNode = pSessionHeadNode;
	void *SessionStartAddress = NULL;
	cv_session_details	*stSessionDetails;

	// save session that won't be deleted
	SessionStartAddress = GetSessionOfTheHandle(cvHandle);
	if (SessionStartAddress == NULL)
		/* this shouldn't happen */
		return FALSE;
	//allocating the size of session
	stSessionDetails = (cv_session_details*)malloc(sizeof(cv_session_details));
	if( NULL == stSessionDetails)
		return FALSE;
	memcpy(stSessionDetails,SessionStartAddress,sizeof(cv_session_details));

	/* now delete all the sessions */
	while(ptmpSessionNode != NULL)
	{
		pSessionHeadNode = ptmpSessionNode->pNext;

		//if ptmpSessionNode->pNext == NULL, reset the head and tail node 
		if(ptmpSessionNode->pNext == NULL)
		{
			pSessionTailNode = NULL;
			pSessionHeadNode = NULL;
			bIsSessionHeadNode = TRUE;
		}

		//Free the current Session node
		if(ptmpSessionNode->pAddressOfCVSession)
			free(ptmpSessionNode->pAddressOfCVSession);

		if(ptmpSessionNode)
			free(ptmpSessionNode);

		ptmpSessionNode = pSessionHeadNode;
	}

	AddNewSession((uint32_t *)stSessionDetails);

	return TRUE;
}

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
	
void *GetSessionOfTheHandle(cv_handle cvHandle)
{
	SessionList *pSessionNode = pSessionHeadNode;

	/*In case of CV_OPEN, cvHandle will be NULL. So point to the recent session memory.*/
	if(cvHandle == (cv_handle)0)
		return pSessionTailNode->pAddressOfCVSession;

	while(pSessionNode != NULL)
	{
		if(((cv_session_details*)pSessionNode->pAddressOfCVSession)->cvHandle == cvHandle)
		{
			break;
		}
		pSessionNode = pSessionNode->pNext;
	}

	if(pSessionNode == NULL)
		return NULL;

	return pSessionNode->pAddressOfCVSession;
}

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
	
void *GetSessionOfTheCVInternalHandle(cv_handle cvInternalHandle)
{
	SessionList *pSessionNode = pSessionHeadNode;

	/*In case of CV_OPEN, cvHandle will be NULL. So point to the recent session memory.*/
	if(cvInternalHandle == (cv_handle)0)
		return pSessionTailNode->pAddressOfCVSession;

	while(pSessionNode != NULL)
	{
		if(((cv_session_details*)pSessionNode->pAddressOfCVSession)->cvInternalHandle == cvInternalHandle)
		{
			break;
		}
		pSessionNode = pSessionNode->pNext;
	}

	if(pSessionNode == NULL)
		return NULL;

	return pSessionNode->pAddressOfCVSession;
}

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

cv_bool IsSessionListEmpty()
{
	return (pSessionHeadNode == NULL) ? TRUE : FALSE;
}
