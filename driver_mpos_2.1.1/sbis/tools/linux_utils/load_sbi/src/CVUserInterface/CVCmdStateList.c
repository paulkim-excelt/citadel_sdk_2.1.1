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
 * Include file list
 */
#include "CVCmdStateList.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>


CVCommandState *g_pCVCmdState_Head = NULL;
CVCommandState *g_pCVCmdState_Tail = NULL;
cv_bool bIsCVCmdStateHeadNode = TRUE;


cv_bool SetCmdState(uint32_t uCmdId, uint32_t uCommandIndex, uint32_t uState)
{
	CVCommandState *pNewNode = NULL;
	CVCommandState *pCmdNode = NULL;

	/*Check whether the Node is already existing*/
	pCmdNode = GetCmdNode(uCmdId, uCommandIndex);
	if(pCmdNode != NULL) /*Node is already existing!*/
	{
		pCmdNode->uState = uState; /*Update the State only*/
		return TRUE;
	}
	else /*New List*/
	{
		/*Allocate mem for new List*/
		pNewNode = (CVCommandState *) malloc(sizeof(CVCommandState));
		if(pNewNode == NULL)
			return FALSE;

		memset(pNewNode,0,sizeof(CVCommandState));

		/*Copy the data*/
		pNewNode->uCmdId = uCmdId;
		pNewNode->uCommandIndex = uCommandIndex;
		pNewNode->uState = uState;
		pNewNode->pNext = NULL;


		if(bIsCVCmdStateHeadNode == TRUE)
		{
			g_pCVCmdState_Head = g_pCVCmdState_Tail = pNewNode;
			bIsCVCmdStateHeadNode = FALSE;
		}
		else
		{
			g_pCVCmdState_Tail->pNext = pNewNode; /*Previous Node's Next*/
			g_pCVCmdState_Tail = pNewNode; /*Now Tail is pointing to new node*/
		}
	}
	
	return TRUE;
}

uint32_t GetCmdState(uint32_t uCmdId, uint32_t uCommandIndex)
{
	uint32_t State = CV_COMMAND_INVALID_STATE;
	CVCommandState *pCmdNode = NULL;

	/*Check whether the Node is already existing*/
	pCmdNode = GetCmdNode(uCmdId, uCommandIndex);
	if(pCmdNode != NULL) /*Node is already existing!*/
	{
		State = pCmdNode->uState;
	}

	return State;
}

void DeleteCmdState(uint32_t uCmdId, uint32_t uCommandIndex)
{
	CVCommandState *pTmpNode = g_pCVCmdState_Head;
	CVCommandState *pPrvNode = NULL;

	if(pTmpNode == NULL)
		return;

	while(pTmpNode != NULL)
	{
		if((pTmpNode->uCmdId == uCmdId) && (pTmpNode->uCommandIndex == uCommandIndex))
		{
			if(pTmpNode == g_pCVCmdState_Head) /*Deleting the Head node*/
			{
				g_pCVCmdState_Head = g_pCVCmdState_Head->pNext;
				if(g_pCVCmdState_Head == NULL)
				{
					bIsCVCmdStateHeadNode = TRUE;
					g_pCVCmdState_Tail = NULL;
				}
			}
			else
			{
				pPrvNode->pNext = pTmpNode->pNext;
				if(pPrvNode->pNext == NULL)
					g_pCVCmdState_Tail = pPrvNode;
			}

			free(pTmpNode);
			break;
		}
		else
		{
			pPrvNode = pTmpNode;
			pTmpNode = pTmpNode->pNext;
		}
	}
}

void DeleteAllCmdState()
{
	CVCommandState *pTmpNode = g_pCVCmdState_Head;
	CVCommandState *pNxtNode = NULL;

	while(pTmpNode != NULL)
	{
		pNxtNode = pTmpNode->pNext;
		free(pTmpNode);
		pTmpNode = pNxtNode;
	}

	/*Initilize the Below data*/
	g_pCVCmdState_Head = NULL;
	bIsCVCmdStateHeadNode = TRUE;
}



CVCommandState *GetCmdNode(uint32_t uCmdId, uint32_t uCommandIndex)
{
	CVCommandState *pCmdNode = g_pCVCmdState_Head;

	while(pCmdNode != NULL)
	{
		if((pCmdNode->uCmdId == uCmdId) && (pCmdNode->uCommandIndex == uCommandIndex))
			break;

		pCmdNode = pCmdNode->pNext;
	}

	return pCmdNode;
}
