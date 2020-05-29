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

#include "CVList.h"
#include "CVUsrLibGlobals.h"
#include "CVLogger.h"
#include "SetupSecureSession.h"
#include <stdlib.h>
#include <unistd.h>

#define ENCAP_CMD_BUFF			"EncapCmdBuff.txt"
#define ENCAP_CMD_BLOB_B4_ENC	"EncapCmdBuffB4Enc.txt"
#define SECURE_SESSION_KEY		"SecureSessionAESKey.txt"
#define HOST_NONCE				"HostNonce.txt"
#define CV_NONCE				"CVNonce.txt"
#define AES_ENC_IV				"AES_Enc_IV.txt"
#define OUTBOUND_HMAC_CALCULATED	"OutBound_HMAC_Calculated.txt"
#define INBOUND_HMAC_CALCULATED	"InBound_HMAC_Calculated.txt"
#define INBOUND_HMAC_FROM_CV	"InBound_HMAC_From_CV.txt"
#define INPUT_FOR_RESULT_HMAC	"InputForResultHMAC.txt"
#define HOST_PUBLIC_KEY			"HostPublicKey.txt"

#define PACKING_SIZE 4
#define TempParamSize 1

/* Global declarations for Remote Session*/
handle_t			hRemoteBinding = NULL;
cv_version		version = {CV_MAJOR_VERSION, CV_MINOR_VERSION}; // CV version set by app when DLL is loaded

BOOL bKDICertiVerificationDone = FALSE ;

static DWORD	ulAntiHammeringDelayMed = ANTI_HAMMERING_DELAY_MED_DEFAULT, 
				ulAntiHammeringDelayHigh = ANTI_HAMMERING_DELAY_HIGH_DEFAULT ;


/*
* Function:
*      cvhcvHandleRequired
* Purpose:
*		Some commands to not require CV Handle to communicate with USH.
*       This api will return 0 for those commands that do not require handles
*
* Parameters:
*		cmd_id				: Command ID of the operation
*
* ReturnValue:
*      1 - require cvhandle
*      0 - Don't require cvhandle
*/
BOOL
cvhCvHandleRequired (	IN	cv_command_id cmd_id
					)
{
	if (cmd_id == CV_CMD_LOAD_SBI
		|| cmd_id == CV_CMD_FLASH_PROGRAMMING
		|| cmd_id == CV_CMD_FW_UPGRADE_START
		|| cmd_id == CV_CMD_FW_UPGRADE_UPDATE
		|| cmd_id == CV_CMD_FW_UPGRADE_COMPLETE
		|| cmd_id == CV_CMD_USH_VERSIONS
		|| cmd_id == CV_CMD_REBOOT_TO_SBI
		|| cmd_id == CV_CMD_REBOOT_USH
		|| cmd_id == CV_CMD_SEND_BLOCKDATA
		|| cmd_id == CV_CMD_RECEIVE_BLOCKDATA
		|| cmd_id == CV_CMD_GET_MACHINE_ID
		|| cmd_id == CV_CMD_FINGERPRINT_CAPTURE_CANCEL
		|| cmd_id == CV_CMD_FINGERPRINT_DISCARD_ENROLLMENT
		|| cmd_id == CV_CMD_FINGERPRINT_RESET
		|| cmd_id == CV_CMD_TEST_TPM) {
			return 0;
	} else {
		return 1;
	}
}


/* 
* Function:
*      cvhManageCVAPICall
* Purpose:
*		The manager rotuine which is called by all cv_xxx functions. which internally creates
*		New Session handle, calls Encapsulate command, DLL(Transmit), Decapsulate command and 
*		Save the return values.
*
* Parameters:
*		numOutputParams		: Number of parameters to be sent to CV
*		outBoundList		: List of Outbound parameters 
*		numInputParams		: Number of result parameters expected from CV
*		inBoundList			: List of inbound parameters
*		ptr_callback_ctx	: Pointer to application callback and context
*		cvHandle			: Address of the cv_handle
*		auth_list_idx		: Index number of the authorization list
*		cmd_id				: Command ID of the operation
*
* ReturnValue:
*      cv_status
*/
cv_status 
cvhManageCVAPICall (
					IN	uint32_t numOutputParams,
					IN	cv_param_list_entry **outBoundList,
					IN	uint32_t numInputParams,
					IN	cv_param_list_entry **inBoundList,
					IN	cv_fp_callback_ctx *ptr_callback_ctx,
					IN	cv_handle cvHandle,
					IN	uint32_t auth_list_idx,
					IN	cv_command_id cmd_id
					)
{
	cv_status					status = CV_SUCCESS;
	cv_status					nRetStatus = CV_SUCCESS; //transmit purpose
	cv_status					nResubmitStatus		= CV_SUCCESS; //Resubmit purpose
	uint32_t					nBlobSize = 0;
	cv_param_list_entry			*stParamListBlob = NULL;
	cv_encap_command			*stInEncapCmd = NULL;
	cv_encap_command			*stEncapResult = NULL;  //transmit purpose
	uint32_t					nTmpEncapSize = 0;
	cv_session_details			*stSessionDetails = NULL;
	cv_session					*stSession = NULL;
	BOOL						bOSVista = FALSE;
	BOOL						b64Bit = FALSE;
	// for RemoteAccess
	char						*pIPAddr = NULL;
	uint32_t					nTotalSessionSize = 0;
	uint32_t					nExtraSessionSize = 0;
	uint32_t					nTmpAuthEncapSize = 0;
	uint32_t					nEncryptedSize = 0;
	uint32_t					stInEncpCmdSize = 0;
	void						*pCVRemoteSessionOffset = NULL;
	uint32_t					nResultBufferSize = 0;
	uint32_t					tempHandle;
	cv_session_details			*duplicateSession;

	BEGIN_TRY
	{

		// Validate the OS(VISTA/XP) and Process Architecture (64/32bit)
		if(get_SystemOS_ProcessorArchitecture(&bOSVista,&b64Bit) != CV_SUCCESS)
		{
			logErrorMessage("Invalid System OS and Processor Architecture ", ERROR_LOCATION);
			return CV_GENERAL_ERROR;
		}

		// Prepare the Blob from outBoundList info
		status = ConstructBlob(outBoundList, numOutputParams, &stParamListBlob, &nBlobSize);
		if(status != CV_SUCCESS)
			goto clean_up;

		//Create New Session Structure for CV_CMD_OPEN and add extra 32 bit
		//if it is CV_CMD_OPEN_REMOTE plus an additional 32 /64 bits to be added
		//modified the below if condition as per BCM changes as part of merging - 5th Dec 2007
		if(cmd_id == CV_CMD_OPEN || cmd_id == CV_CMD_OPEN_REMOTE)
		{
			uint32_t			nOptions = 0;

			/*Construct the Session Structure*/
			/*Calculate the size fo the extra bytes required for the Session structure*/
			nExtraSessionSize = (cmd_id == CV_CMD_OPEN_REMOTE)? ULSIZE: 0;

			/*Calculate the Total Session Size*/
			nTotalSessionSize = sizeof(cv_session_details) + nExtraSessionSize;

			// Get the Options Value -  modified the BCM changes as part of merging - 5th Dec 2007
			memcpy(&nOptions, (((byte*)outBoundList[0]) + sizeof(cv_param_list_entry)), sizeof(nOptions));

			//allocating the size of session
			stSessionDetails = (cv_session_details*)malloc(nTotalSessionSize);
			if( NULL == stSessionDetails)
			{
				logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
				status =  CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}

			memset(stSessionDetails, 0x00, nTotalSessionSize);

			//Create a new list for storing the address of the session
			AddNewSession((uint32_t *)stSessionDetails);

			/*Point to to address of the cv session in cv_session_details*/
			stSession = &stSessionDetails->stCVSession;

			// Get the Options Value
			stSession->flags = nOptions;

			if(cmd_id == CV_CMD_OPEN_REMOTE)
			{
				/*Initilize the below pointer to its offset address in the cv session*/
				pCVRemoteSessionOffset = (((byte *)(stSessionDetails)) + sizeof(cv_session_details));

				/*Set the flag as remote session.*/
				stSession->flags |= CV_REMOTE_SESSION;

				/*Get the Remote session handle only if it is null*/
				if(hRemoteBinding == NULL)
				{
					/*Get the IP Address*/
					pIPAddr = (char *)malloc(outBoundList[4]->paramLen + 1);
					if( NULL == pIPAddr)
					{
						logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
						status =  CV_MEMORY_ALLOCATION_FAIL;
						goto clean_up;
					}
					memset(pIPAddr, 0, outBoundList[4]->paramLen + 1);
					memcpy(pIPAddr, (char *)outBoundList[4] + sizeof(cv_param_list_entry), outBoundList[4]->paramLen);

					/*Establish the Remote Access*/
//dgb					status  = cvhOpenRemoteSession(pIPAddr, &hRemoteBinding);
					if(status != CV_SUCCESS)
					{
						goto clean_up;
					}

					/*copy the Remote binding handle into the present session structure*/
					if(hRemoteBinding != NULL)
					{
						memcpy(pCVRemoteSessionOffset, hRemoteBinding, (int)sizeof(handle_t));
					}
				}
			}

			//check the session flags indicate the secure session then call cvhSetupSecureSession
			if((stSession->flags & CV_SECURE_SESSION) == CV_SECURE_SESSION)
			{
				//Fill the neccessary elements in cv_session structure for secure session key and HMAC key
				//calling cvhsetupsecuresession before creating new session key
				status = GetCVSecureSessioncmd(outBoundList, bOSVista, stSession);
				if(status != CV_SUCCESS)
				{
					goto clean_up;
				}
			}
		}
		// check for commands that do not require cvhandle 
		else if ( !cvhCvHandleRequired(cmd_id) )
		{
			nTotalSessionSize = sizeof(cv_session_details);

			//allocating the size of session
			stSessionDetails = (cv_session_details*)malloc(nTotalSessionSize);
			if( NULL == stSessionDetails)
			{
				logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
				status =  CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}

			memset(stSessionDetails, 0x00, nTotalSessionSize);

			/*Point to to address of the cv session in cv_session_details*/
			stSession = &stSessionDetails->stCVSession;

			// Get the Options Value
			stSession->flags = CV_SYNCHRONOUS;
		}
		/*Capture the Session Handle for all the APIs except CV_CMD_OPEN and CV_CMD_OPEN_REMOTE*/
		else
		{
			/*Find out the corrosponding CVSession address for the CVHandle*/
			stSessionDetails = GetSessionOfTheHandle(cvHandle);
			if( NULL == stSessionDetails)
			{
				logErrorMessage("CVSession Could not be found", ERROR_LOCATION);
				status =  CV_INVALID_HANDLE;
				goto clean_up;
			}

			/*Point to to address of the cv session in cv_session_details*/
			stSession = &stSessionDetails->stCVSession;

			/*If it is a CV_REMOTE_SESSION, copy the Binding handle from the current session*/
			if((stSession->flags & CV_REMOTE_SESSION) == CV_REMOTE_SESSION)
			{
				/*Initilize the below pointer to it's offset address in the cv session details*/
				pCVRemoteSessionOffset = (((byte *)(stSessionDetails)) + sizeof(cv_session_details));
				memcpy(hRemoteBinding, pCVRemoteSessionOffset, (int)sizeof(handle_t));
			}
		}

		/*Calculating the size of Encapsulate command based on
		flags (Securesession / callback&context)*/
		stInEncpCmdSize = (sizeof(cv_encap_command)- sizeof(stInEncapCmd->parameterBlob));

		//Know the size to allocate the memory for Encapsultate buffer
		nTmpEncapSize += stInEncpCmdSize;

		//Check whether application is set CV_INTERMEDIATE_CALLBACKS / CV_FP_INTERMEDIATE_CALLBACKS session flag
		if( ptr_callback_ctx != NULL )
		{
			// Calculating extra space for callback in Encapsulate command based on the 32/64 bit
			nTmpEncapSize += ULSIZE + ULSIZE ;
		}

		//storing the nTmpAuthEncapSize for the purpose of resubmit cmd (userpromptpin)
		if(auth_list_idx != 0)
		{
			nTmpAuthEncapSize = nTmpEncapSize;
		}

		//calculating the extra size for the secure session, adding SHA1_len, padding and extra byte for encrypt command
		if((stSession->flags & CV_SECURE_SESSION) == CV_SECURE_SESSION)
		{
			uint32_t nPaddSize = 0, nBytesToPad = 0;

			if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B)
			{
				//Adding SHA1_Len for 20 bytes
				nEncryptedSize = nEncryptedSize + nBlobSize + SHA256_LEN;
			}
			else
			{
				//Adding SHA1_Len for 20 bytes
				nEncryptedSize = nEncryptedSize + nBlobSize + SHA1_LEN;
			}

			//Padd 0's if the length is not 16 bytes packed
			nPaddSize = nEncryptedSize % AES_128_BLOCK_LEN ;
			if(nPaddSize != 0)
			{
				nBytesToPad = (AES_128_BLOCK_LEN - nPaddSize);
			}

			//nBytesToPad = 16 bytes packed (Padd 0's for the nBytesToPad before Encryption)
			nEncryptedSize =  nEncryptedSize + nBytesToPad + AES_128_BLOCK_LEN ;

			//allocation of Total Encapbuffer size (extra 8 bytes of ptrcallback)
			nTmpEncapSize = nTmpEncapSize + nEncryptedSize;

			//Know the length of the Transport len of Encapsulate buffer with Secure Session
			stInEncpCmdSize = stInEncpCmdSize + nEncryptedSize;
		}
		else //if it is not secure session
		{
			//Adding the blob size
			nTmpEncapSize = nTmpEncapSize + nBlobSize;

			//Know the length of the Transport len of Encapsulate buffer WITHOUT Secure Session
			stInEncpCmdSize = stInEncpCmdSize + nBlobSize;
		}

		/* This extra 4 bytes required by User interface to store the unique command index*/
		nTmpEncapSize += sizeof(uint32_t);

		// Memory allocation for Encapsulated command
		stInEncapCmd = (cv_encap_command*)malloc(nTmpEncapSize);
		if( NULL == stInEncapCmd)
		{
			logErrorMessage("Memory Allocation Failed", ERROR_LOCATION);
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}
		memset(stInEncapCmd, 0x00, nTmpEncapSize);

		//calling cvhEncapsulated cmd
		status = cvhEncapsulateCmd(stParamListBlob,nBlobSize,inBoundList,numInputParams, ptr_callback_ctx, stSessionDetails, cmd_id, b64Bit, stInEncpCmdSize,stInEncapCmd);
		if(status != CV_SUCCESS)
		{
			goto clean_up;
		}

		/* CV_CMD_FINGERPRINT_CAPTURE command will have the result of MAX_ENCAP_FP_RESULT_BUFFER size.
		For all other cv command's result size will be MAX_ENCAP_RESULT_BUFFER */
		//nResultBufferSize = (stInEncapCmd->commandID == CV_CMD_FINGERPRINT_CAPTURE) ? MAX_ENCAP_FP_RESULT_BUFFER : MAX_ENCAP_RESULT_BUFFER;
		if (stInEncapCmd->commandID == CV_CMD_FINGERPRINT_CAPTURE)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (stInEncapCmd->commandID == CV_CMD_RECEIVE_BLOCKDATA)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (stInEncapCmd->commandID == CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (stInEncapCmd->commandID == CV_CMD_FINGERPRINT_CAPTURE_WBF)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else
			nResultBufferSize = MAX_ENCAP_RESULT_BUFFER;

		//Allocation memory for cv_encap result
		logMessage("cvhManageCVAPICall() cmd: 0x%x; nResultBufferSize: 0x%x\n", stInEncapCmd->commandID, nResultBufferSize);
		stEncapResult = (cv_encap_command*)malloc(nResultBufferSize);
		if( NULL == stEncapResult)
		{
			logErrorMessage("Memory Allocation Failed", ERROR_LOCATION);
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}

		do
		{
			memset(stEncapResult, 0, nResultBufferSize);
			//LogDataToFile(ENCAP_CMD_BUFF, "Encap Command", strlen("Encap Command"), (uint8_t*) stInEncapCmd, stInEncapCmd->transportLen, TRUE);

			/*Load interface DLL and Call to cvhTransmit() or cvhTransmitClient()
			based on the remotesession flage set or not*/
			status = ProcessTransmitCmd( stSession, &stInEncapCmd, &nRetStatus, &stEncapResult);
			if(status != CV_SUCCESS)
			{
				logErrorMessage("ProcessTransmitCmd Failed", ERROR_LOCATION);
				goto clean_up;
			}

			//LogDataToFile(ENCAP_CMD_BUFF, "Encap Result", strlen("Encap Result"), (uint8_t*)stEncapResult, stEncapResult->transportLen, TRUE);

			if((nRetStatus == CV_ANTIHAMMERING_PROTECTION_DELAY_MED  || nRetStatus == CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH ))
			{
				unsigned long AntiHammeringDelay = 0 ;

				if( (ulAntiHammeringDelayMed == ANTI_HAMMERING_DELAY_MED_DEFAULT) &&
					(ulAntiHammeringDelayHigh == ANTI_HAMMERING_DELAY_HIGH_DEFAULT) )
				{
					if (!setAntiHammerProtectionDelay())
					{
						logErrorMessage("Error while setting the Anit Hammering Delays from Registry", ERROR_LOCATION);
					}
				}

				AntiHammeringDelay = getAntiHammeringProtectionDelay(nRetStatus);
				// 1 sec = 1000 ms
				usleep(AntiHammeringDelay * 1000 * 1000);
				//logMessage("Re-submitting the command after %d secs. \n", AntiHammeringDelay);
			}

			// check whether cvhTransmit() or cvhTransmitClient() are returned CV_PROMPT_PIN
			while((nRetStatus == CV_PROMPT_PIN) || (nRetStatus == CV_PROMPT_PIN_AND_SMART_CARD) || (nRetStatus == CV_PROMPT_PIN_AND_CONTACTLESS))
			{
				if ((stSession->flags & CV_SUPPRESS_UI_PROMPTS) == CV_SUPPRESS_UI_PROMPTS)
				{
					status = nRetStatus;
					goto clean_up;
				}
				// Display the Prompt for PIN entry and prepare the encap command for re-submission.
				// + 32 bits for unique value sent back from the UI to be attached to it.

				//Identify the api which contains the Index of the authlist & modify the outboundlist
				if(auth_list_idx != 0)
				{
					uint32_t uCmdIndex = 0;
					uint32_t nCallBackSize = 0;

					//get the unique command index from stInEncapCmd added by user interface dll
					status = GetCommandIndex(stInEncapCmd, &uCmdIndex);
					if(status != CV_SUCCESS)
					{
						logErrorMessage("GetCommandIndex Failed ", ERROR_LOCATION);
						goto clean_up;
					}

					//call getpromptuserpincmd to modify the paramlist blob and know the blob size
					status = GetPromptUserPinCmd(nRetStatus, auth_list_idx,outBoundList, numOutputParams,&stParamListBlob, &nBlobSize);

					//resubmitting the cmd either cv_success or (status == CV_CANCELLED_BY_USER)
					if((status == CV_SUCCESS) || (status == CV_CANCELLED_BY_USER))
					{
						nEncryptedSize = 0;
						stInEncpCmdSize = 0;
						stInEncpCmdSize = (sizeof(cv_encap_command)- sizeof(stInEncapCmd->parameterBlob));

						//calculating the extra size for the secure session, adding SHA1_len, padding and extra byte for encrypt command
						if((stSession->flags & CV_SECURE_SESSION) == CV_SECURE_SESSION)
						{
							uint32_t nPaddSize = 0, nBytesToPad = 0;

							//Adding SHA1_Len for 20 bytes
							nEncryptedSize = nEncryptedSize + nBlobSize + SHA1_LEN;

							//Padd 0's if the length is not 16 bytes packed
							nPaddSize = nEncryptedSize % AES_128_BLOCK_LEN ;
							if(nPaddSize != 0)
							{
								nBytesToPad = (AES_128_BLOCK_LEN - nPaddSize);
							}

							//nBytesToPad = 16 bytes packed (Padd 0's for the nBytesToPad before Encryption)
							nEncryptedSize =  nEncryptedSize + nBytesToPad + AES_128_BLOCK_LEN ;

							//Adding the blob size, ntmpAuthEncapSize( size of securesession, ptrcallback), 4 bytes(unique cmdidx)
							nTmpAuthEncapSize = nTmpAuthEncapSize + nEncryptedSize + sizeof(uint32_t);

							//Know the length of the Transport len of Encapsulate buffer with Secure Session
							stInEncpCmdSize = stInEncpCmdSize + nEncryptedSize;
						}
						else //if it is not secure session
						{
							//Adding the blob size
							nTmpAuthEncapSize = nTmpAuthEncapSize + nBlobSize + sizeof(uint32_t);

							//Know the length of the Transport len of Encapsulate buffer WITHOUT Secure Session
							stInEncpCmdSize = stInEncpCmdSize + nBlobSize;
						}

						//Adding the blob size, ntmpAuthEncapSize( size of securesession, ptrcallback), 4 bytes(unique cmdidx)
						//nTmpAuthEncapSize = nTmpAuthEncapSize + nBlobSize + sizeof(uint32_t);

						// Memory Reallocation for Encapsulated command
						stInEncapCmd = realloc(stInEncapCmd,nTmpAuthEncapSize);
						memset(stInEncapCmd, 0x00, nTmpAuthEncapSize);

						// Increment the secure session usage count, since it didn't get incremented because 
						// this command wasn't decapsulated
						stSession->secureSessionUsageCount++ ;

						//setting the flag to CV_RET_TYPE_RESUBMITTED
						stInEncapCmd->flags.returnType = CV_RET_TYPE_RESUBMITTED;

						//calling cvhEncapsulated cmd with updated paramlist blob
						status = cvhEncapsulateCmd(stParamListBlob,nBlobSize,inBoundList,numInputParams, ptr_callback_ctx,
							stSessionDetails, cmd_id, b64Bit, stInEncpCmdSize, stInEncapCmd);
						if(status != CV_SUCCESS)
						{
							logErrorMessage("cvhEncapsulateCmd Failed ", ERROR_LOCATION);
							goto clean_up;
						}

						//Determine the sizeof callback and context
						if( ptr_callback_ctx != NULL)
						{
							nCallBackSize = ULSIZE + ULSIZE;
						}

						//Add the unique command index at the end of the encapsulatecmd
						memcpy((((byte*)stInEncapCmd) + stInEncapCmd->transportLen) + nCallBackSize,
							&uCmdIndex, sizeof(uint32_t));

						//Clean the previous data in Encap result buffer.
						memset(stEncapResult, 0, MAX_ENCAP_RESULT_BUFFER);

						//Resubmit the process transmit cmd with updated stInEncapCmd
						status = ProcessTransmitCmd( stSession, &stInEncapCmd, &nResubmitStatus, &stEncapResult);
						if(status != CV_SUCCESS)
						{
							logErrorMessage("ProcessTransmitCmd Failed ", ERROR_LOCATION);
							goto clean_up;
						}

						//copy the status and if it is cv_success, call decapsulate cmd()
						nRetStatus = nResubmitStatus;
					}
					else
					{
						logErrorMessage("GetPromptUserPinCmd Failed ", ERROR_LOCATION);
						goto clean_up;
					}
				}
				else
				{
					logErrorMessage("auth_list_idx is not proper ", ERROR_LOCATION);
					goto clean_up;
				}
			}

		}while((nRetStatus == CV_ANTIHAMMERING_PROTECTION_DELAY_MED  || nRetStatus == CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH ));

		// On completion result, status is CV_SUCCESS or CV_INVALID_OUTPUT_PARAMETER_LENGTH or CV_ENUMERATION_BUFFER_FULL
		//call  cvhDecapsulatedcmd()
		if((nRetStatus == CV_SUCCESS) ||(nRetStatus == CV_INVALID_OUTPUT_PARAMETER_LENGTH) ||(nRetStatus == CV_ENUMERATION_BUFFER_FULL) 
		    || (nRetStatus == CV_FP_DUPLICATE))
		{
			//Calling Decapsulate cmd
			status = cvhDecapsulateCmd(stEncapResult, stSession, &numInputParams, inBoundList);
			if(status != CV_SUCCESS)
			{
				logErrorMessage("Decapsulatecmd Failed ", ERROR_LOCATION);
				goto clean_up;
			}

			// 27th Mar 2008 - Increment the Session Usage Count for the subsequent commands
			stSession->secureSessionUsageCount++ ;


			/*Store the cv handle value and an unique number in the session structure.
			Unique number will be used by application as a cv handle and the same will
			be inputed in the other cv_XXX calls*/
			if(cmd_id == CV_CMD_OPEN || cmd_id == CV_CMD_OPEN_REMOTE)
			{
				uint32_t uUniqueHandleNo = 0;
				void *ppTmpParam[1] = {NULL};

				/*Generate a unique no*/
				uUniqueHandleNo = cvhGetUniqueHandleNo();

				/*Store the unique no in the Session structure*/
				memcpy(&stSessionDetails->cvHandle, &uUniqueHandleNo, sizeof(uint32_t));

				/*Store the cv handle value in the Session structure*/
				ppTmpParam[0] = &tempHandle;
				status = cvhSaveReturnValues(ppTmpParam, 1, 0, inBoundList);
				if(status != CV_SUCCESS)
				{
					logErrorMessage("Save Return Values Failed ", ERROR_LOCATION);
					goto clean_up;
				}
				/* first check to see if this handle is a duplicate of any handle on the list. */
				/* this could happen due to resume from S3/S4 with open session.  if so, delete */
				/* existing session */
				if ((duplicateSession = GetSessionOfTheCVInternalHandle(tempHandle)) != NULL)
				{
					logErrorMessage("Duplicate session found ", ERROR_LOCATION);
					if (!DeleteSessionHandle(duplicateSession->cvHandle))
					{
						logErrorMessage("Unable to delete duplicate session ", ERROR_LOCATION);
					}
				}
				stSessionDetails->cvInternalHandle = tempHandle;

				/*And return this unique no as cv handle to the application
				throuth inBoundList*/
				memcpy(((byte *)inBoundList[0]) + sizeof(cv_param_list_entry),
					&uUniqueHandleNo, sizeof(uint32_t));
			}
			else if(cmd_id == CV_CMD_CLOSE)
			{
				//cvhCloseRemoteSession(hRemoteBinding);
			}

			//send the invalid length return status to the cvapiXXXX
			status = nRetStatus;
			goto clean_up;
		}
		else 
		{
			// 27th Mar 2008 - Increment the Session Usage Count for the subsequent commands
			stSession->secureSessionUsageCount++ ;

			if(cmd_id == CV_CMD_OPEN || cmd_id == CV_CMD_OPEN_REMOTE)
			{
				// CV_CMD_OPEN or CV_CMD_OPEN_REMOTE failed
				logErrorMessage("CV_CMD_OPEN or CV_CMD_OPEN_REMOTE failed.", ERROR_LOCATION);
				if(DeleteSessionHandle(0) != TRUE)
				{
					logErrorMessage("Error returned by DeleteSessionHandle(0)", ERROR_LOCATION);
					goto clean_up;
				}
			}
		}

		/*Default return error code are not handled here and hence will be returned to upper stack*/
		status = nRetStatus;

clean_up:
		// Freeing the memory
		if (pIPAddr)
			free(pIPAddr);
		Free((void *)&stParamListBlob, TempParamSize);
		/*Need to free host callback structure before freeing the stInEncapCmd*/
		if( ptr_callback_ctx != NULL )
		{
			if(stInEncapCmd != NULL)
			{
				Free((void*)((byte*)stInEncapCmd + stInEncapCmd->transportLen + ULSIZE), 1);
			}
		}
		if (stInEncapCmd)
			free(stInEncapCmd);
		if (stEncapResult)
			free(stEncapResult);
		if ( !cvhCvHandleRequired(cmd_id) )
		{
			if ( stSessionDetails )
				free(stSessionDetails);
		}
	}
	END_TRY

	return status;
}


/*
* Function:
*      cvhEncapsulateCmd
* Purpose:
*     Encapsulates the out bound parameters into Encapsulation structure
*     Inbound list is encapsulated and stored for Host callback structure
* Parameters:
*		stParamListBlob				:	Outbound Parameters
*		nBlobLen			:   Length of the Outbound Parameters
*		InBoundlist			:	Inbound Parameters
*		nInbound			:	number of Inbound parameters
*		ptr_callback_ctx	:	Pointer to Callback and context structure
*		stSession			:	Pointer to Session structure
*		cmd_id				:	Command ID for CV operation
*		stInEncapCmd		:	Allocated Encapsulated Structure
*
* ReturnValue:
*		stInEncapCmd		:	filled Encapsulated structure based on securesession / callback & context
*		cv_status
*/
cv_status
cvhEncapsulateCmd (
				   IN	cv_param_list_entry *stParamListBlob,
				   IN	uint32_t nBlobLen,
				   IN	cv_param_list_entry **InBoundlist,
				   IN	uint32_t nInbound,
				   IN	cv_fp_callback_ctx *ptr_callback_ctx,
				   IN	cv_session_details *stSessionDetails,
				   IN	cv_command_id cmd_id,
				   IN	BOOL b64Bit,
				   IN	uint32_t stInEncapCmdSize,
				   OUT	cv_encap_command *stInEncapCmd
				   )
{
	cvh_host_callback	*ptr_host_callback		= NULL;
	cv_status			status					= CV_SUCCESS;
	uint8_t				*pbIV					= NULL;
	uint8_t				*pbEncryptedParamBlob	= NULL;
	cv_session			*stSession				= NULL;
	//cv_encap_command *stInEncapCmd			= *ppstInEncapCmd;
	unsigned int		bResult					= SUCCESS;

	BEGIN_TRY
	{
		// Constructing Encapsulated Structure
		stInEncapCmd->transportType = CV_TRANS_TYPE_ENCAPSULATED;
		//if it is not secure session, Add only BlobSize in Transportlen
		//stInEncapCmd->transportLen = (sizeof(cv_encap_command)-sizeof(stInEncapCmd->parameterBlob) + nBlobLen);
		stInEncapCmd->transportLen = stInEncapCmdSize;
		stInEncapCmd->commandID = cmd_id;
		stInEncapCmd->version.versionMajor = version.versionMajor;
		stInEncapCmd->version.versionMinor = version.versionMinor;
		stInEncapCmd->flags.returnType = 0;
		if( ptr_callback_ctx != NULL )
		{
			stInEncapCmd->flags.completionCallback = 1;
		}
		stInEncapCmd->flags.USBTransport = 1;
		stInEncapCmd->flags.host64Bit = b64Bit; //whether it is 64 bit or not
		stInEncapCmd->flags.secureSession = 0;
		stInEncapCmd->flags.secureSessionProtocol = 0;
		stInEncapCmd->flags.spare = 0;
		if((stInEncapCmd->commandID == CV_CMD_OPEN) || (stInEncapCmd->commandID == CV_CMD_OPEN_REMOTE))
		{
			stInEncapCmd->hSession = stSessionDetails->stCVSession.magicNumber;
		}
		else
		{
			memcpy(&stInEncapCmd->hSession, &stSessionDetails->cvInternalHandle, sizeof(uint32_t));
		}
		stInEncapCmd->returnStatus = 0;
		stInEncapCmd->parameterBlobLen = nBlobLen;
		memcpy(&stInEncapCmd->parameterBlob, stParamListBlob, nBlobLen);

		/*Get the Session Structure*/
		stSession = &stSessionDetails->stCVSession;
		if ((stSession->flags & CV_SUPPRESS_UI_PROMPTS) == CV_SUPPRESS_UI_PROMPTS)
			//Set the Suppress UI Prompt flag
			stInEncapCmd->flags.suppressUIPrompts = 1;

		//If it is Secure Session, Generate Random number, compute the HMAC and
		//append at the end of paramblob and encrypt the parameter
		if((stSession->flags & CV_SECURE_SESSION) == CV_SECURE_SESSION)
		{
			DWORD nEncryptedBlobLen = 0; //length of encrypted param blob len

			//Set the Secure Session flag
			stInEncapCmd->flags.secureSession = 1;

			//Allocating memory for generate random number
			pbIV =(uint8_t*) malloc(AES_128_BLOCK_LEN);
			if( NULL == pbIV)
			{
				logErrorMessage("Memory Allocation is Fail", ERROR_LOCATION);
				status =  CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;

			}
			memset(pbIV, 0x00,AES_128_BLOCK_LEN);

			//call MSCAPI_GenerateRandom to generate random number for IV
//dgb			bResult = MSCAPI_GenerateRandom(pbIV, AES_128_BLOCK_LEN);
			if(bResult != SUCCESS)
			{
				logErrorMessage("Invalid Status MSCAPI_GenerateRandom ", ERROR_LOCATION);
				status =  CV_SECURE_SESSION_FAILURE;
				goto clean_up;
			}

			//Set the value of MSCAPI GenerateRandom IV in stInEncapCmd
			memcpy(&stInEncapCmd->IV[0], pbIV, AES_128_BLOCK_LEN);

			//calling cvhEncryptpcmd
			status = cvhEncryptCmd(stInEncapCmd, stSession, stParamListBlob,nBlobLen,pbIV,
						&pbEncryptedParamBlob, &nEncryptedBlobLen);
			if(status != CV_SUCCESS)
			{
				logErrorMessage("Invalid status cvhEncryptpCmd", ERROR_LOCATION);
				goto clean_up;

			}

			//Set the value to MSCAPI Encrypt ParamBlob in stInEncapCmd
			memcpy(&stInEncapCmd->parameterBlob, pbEncryptedParamBlob, nEncryptedBlobLen);

		}
		else //if it is not secure session then increment the session usuage count
		{
		}

		//Check whether Callback is Available and append cvhmanagecallback and host callback strucuture at the end of
		//Encapsulated buffer
		if( ptr_callback_ctx != NULL )
		{
			uint32_t nParams = 0;
			uint32_t nParmSize = 0;

			PFUNMANAGECALLBACK pTmpManageCallback;

			//Set the Completion Callback flag in stInEncapCmd

			pTmpManageCallback = (void *)cvhManageCallback;

			memcpy((((byte*)stInEncapCmd) + stInEncapCmd->transportLen),
				&pTmpManageCallback, ULSIZE);

			// Prepare HostCallback Structure
			for(nParams=0; nParams < nInbound; nParams++)
			{
				//based on the 32/64 bit
				nParmSize += (ULSIZE + sizeof(InBoundlist[nParams]->paramLen));  // Rajesh 19thSep2008
			}

			//Calculating extra space for Host callback structure
			ptr_host_callback = (cvh_host_callback *)malloc(sizeof(cvh_host_callback) + nParmSize);
			if(ptr_host_callback == NULL)
			{
				logErrorMessage("Memory Allocation is Fail", ERROR_LOCATION);
				status =  CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}
			memset(ptr_host_callback, 0, sizeof(cvh_host_callback) + nParmSize);

			// Prepare the Host Callback Strucuture and store the callback, context and number of inbound params
			if(ptr_callback_ctx->callback != NULL)
				ptr_host_callback->callback = ptr_callback_ctx->callback;
			else
				ptr_host_callback->callback = 0;

			if(ptr_callback_ctx->context != NULL)
				ptr_host_callback->context = ptr_callback_ctx->context;
			else
				ptr_host_callback->context = 0;

			ptr_host_callback->numParams = nInbound;

			// Initially point to the parameter start location
			nParmSize = sizeof(cvh_host_callback);

			// Store the address of actual parametes of cv_xxx()
			for(nParams=0; nParams < nInbound; nParams++)
			{
				uint32_t *nVarAdd = 0;
				uint32_t nVarLen = 0;

				// Get the parameter length
				nVarLen = sizeof(InBoundlist[nParams]->paramLen);
				memcpy(((byte *)ptr_host_callback) + nParmSize, &nVarLen, sizeof(nVarLen));

				//Increment the pointer to store the address
				nParmSize += sizeof(InBoundlist[nParams]->paramLen);

				// Get the address of the parameter
				nVarAdd = (uint32_t*)(((byte *)(InBoundlist[nParams])) + sizeof(cv_param_list_entry));
				memcpy((((byte *)(ptr_host_callback)) + nParmSize), &nVarAdd, ULSIZE);


				//Increment pointer to store the next length // Rajesh 19thSep2008
				nParmSize += sizeof(InBoundlist[nParams]->paramLen);

				// Reset these varaibles for next use in the loop
				nVarAdd = NULL;
				nVarLen = 0;
			}

			// Now store the address of Host callback Memory location
			memcpy((((byte*)stInEncapCmd) + stInEncapCmd->transportLen + ULSIZE),
				&ptr_host_callback, ULSIZE);
		}

		//*ppstInEncapCmd = stInEncapCmd;
		//Don't free host callback structure
clean_up:
		//free the allocated memory
		Free((void *)&pbIV,TempParamSize);
		Free((void *)&pbEncryptedParamBlob,1);
	}
	END_TRY

		return status;
}


/*
* Function:
*      cvhEncryptCmd
* Purpose:
*      Function to Encrypt the  Outbound params
* Parameters:
*		stInEncapCmd			- Encapsulated Command for Encryption
*		stSession				- Session structure
*		stParamListBlob			- ParamBlob to be Encrypted
*		nBlobLen				- ParamBlob length
*		pbIV					- Initial Vector
*		pbSecureSessionHMAC		- Session HMAC of the complete Encap command
*		pbEncryptedParamBlob	- Encrypted Blob
* Returns:
*      cv_status
*/
cv_status
cvhEncryptCmd(
			  IN	cv_encap_command *stInEncapCmd,
			  IN	cv_session *stSession,
			  IN	cv_param_list_entry *stParamListBlob,
			  IN	DWORD nBlobLen,
			  IN	uint8_t *pbIV,
			  OUT	uint8_t **pbEncryptedParamBlob,
			  OUT	DWORD *pbdwBlobLen
			  )
{

	uint32_t		nEncryptEncapSize		= 0;
	uint8_t			*pTempstInEncapCmd		= NULL;
	uint8_t			*pbParamBlob			= NULL;
	unsigned int	bResult					= SUCCESS;
	cv_status		status					= CV_SUCCESS;
	uint32_t		nBlobLenWithHMAC		= 0;
	uint32_t		nModBlobLenWithHMACPacked	= 0;
	uint32_t		nBytesToPad				= 0;
	uint8_t			*pbSecureSessionHMAC	= NULL;

	BEGIN_TRY
	{

		//nbloblen - Actual blob length
        nEncryptEncapSize = (sizeof(cv_encap_command)- sizeof(stInEncapCmd->parameterBlob) + nBlobLen + sizeof(stSession->secureSessionUsageCount));

		pTempstInEncapCmd =(uint8_t*) malloc(nEncryptEncapSize);
		if( NULL == pTempstInEncapCmd)
		{
			logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
			status =  CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}

		memset(pTempstInEncapCmd, 0, nEncryptEncapSize);
		memcpy(pTempstInEncapCmd, (uint8_t *)stInEncapCmd, (nEncryptEncapSize - sizeof(stSession->secureSessionUsageCount)));
		memcpy((pTempstInEncapCmd + nEncryptEncapSize - sizeof(stSession->secureSessionUsageCount)),
			&(stSession->secureSessionUsageCount), sizeof(stSession->secureSessionUsageCount));

		//call Generate Secure Session HMAC
		if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B) // SuiteB - use CNG SHA256 HMAC
		{
			if(bResult != SUCCESS)
			{
				logErrorMessage(" Invalid status MSCAPI_GenerateSecureSessionHMAC", ERROR_LOCATION);
				status = CV_SECURE_SESSION_FAILURE;
				goto clean_up;
			}
			nBlobLenWithHMAC = nBlobLen + SHA256_LEN;
		}
		else // Non-SuiteB - Use MSCAPI - SHA1 HMAC
		{
			if(bResult != SUCCESS)
			{
				logErrorMessage(" Invalid status MSCAPI_GenerateSecureSessionHMAC", ERROR_LOCATION);
				status = CV_SECURE_SESSION_FAILURE;
				goto clean_up;
			}
			//nBlobLen = Size of paramblob, SHA1_LEN = size of HMAC secure session,
			nBlobLenWithHMAC = nBlobLen + SHA1_LEN;


		}

		//Padd 0's if the length is not 16 bytes packed
		nModBlobLenWithHMACPacked = nBlobLenWithHMAC % AES_128_BLOCK_LEN ;
		if(nModBlobLenWithHMACPacked != 0)
		{
			nBytesToPad = (AES_128_BLOCK_LEN - nModBlobLenWithHMACPacked);
		}

		//nBytesToPad = 16 bytes packed (Padd 0's for the nBytesToPad before Encryption)
		nBlobLenWithHMAC += nBytesToPad;

		pbParamBlob = (uint8_t*) malloc(nBlobLenWithHMAC);
		if(NULL == pbParamBlob)
		{
			logErrorMessage("Memory Allocation is Fail ", ERROR_LOCATION);
			status =  CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}

		//copy paramblob and pbsecuresession Hmac
		memset(pbParamBlob, 0x00, nBlobLenWithHMAC);
		memcpy(pbParamBlob, stParamListBlob,(ULONG)nBlobLen);
		if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B) // SuiteB
			memcpy((uint8_t*)(pbParamBlob + nBlobLen), pbSecureSessionHMAC, SHA256_LEN);
		else
			memcpy((uint8_t*)(pbParamBlob + nBlobLen), pbSecureSessionHMAC, SHA1_LEN);

	}
	END_TRY

clean_up:
	//free the allocated memory
	Free((void *)&pTempstInEncapCmd,TempParamSize);
	Free((void *)&pbParamBlob, TempParamSize);
	Free((void *)&pbSecureSessionHMAC, 1);

	return status;
}


/*
* Function:
*      cvhDecapsulateCmd
* Purpose:
*      Decapsulate the resulted information from CV
* Parameters:
*		stEncapResult	:	Structure of the Encapsulated command
*		NoOfParms		:	Number of params
*		retDecapParams	:	Allocated encapsulate structure
* ReturnValue:
*		retDecapParams	:	List of inbound params
*		cv_status
*/
cv_status
cvhDecapsulateCmd(
				  IN	cv_encap_command *stEncapResult,
				  IN	cv_session *stSession,
				  IN	uint32_t *NoOfParms,
				  OUT	cv_param_list_entry **retDecapParams
				  )
{
	uint32_t		nParamLen = 0;
	uint32_t		nParams = 0;
	ULONG_PTR		nUpperBound = 0;
	cv_status		status = CV_SUCCESS;
	uint8_t			*pbDecryptedParamBlob = NULL;
	byte			*nLowerBound = NULL;
	uint8_t			*actualParamBlob = NULL ;
	uint8_t			*resultHMACFromCV = NULL ;
	uint8_t			*resultEncapCmdHdr	= NULL ;
	uint32_t		resultEncapCmdHdrLen = 0;
	uint8_t			*pbResultHMAC = NULL ;
	unsigned int	bResult	= SUCCESS;
	uint8_t *pbKeyToHMAC = NULL;

	BEGIN_TRY
	{

		//check whether it is secure session and call Decrypt command
		if(stEncapResult->flags.secureSession == TRUE)
		{
			DWORD		dwBlobLen = 0;

			if ( stEncapResult->commandID != CV_CMD_CLOSE )
			{
				//call DecryptCmd
				status = cvhDecryptCmd(stSession, stEncapResult, &pbDecryptedParamBlob, &dwBlobLen);
				if(status != CV_SUCCESS)
				{
					logErrorMessage(" Invalid status cvhDecryptCmd", ERROR_LOCATION);
					/*return status;*/
					goto cleanup;
				}
				actualParamBlob = malloc(stEncapResult->parameterBlobLen);
				if( NULL == actualParamBlob)
				{
					logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
					/*return status;*/
					status = CV_MEMORY_ALLOCATION_FAIL ;
					goto cleanup;
				}
				memset(actualParamBlob, 0x00, stEncapResult->parameterBlobLen);
				memcpy(actualParamBlob, pbDecryptedParamBlob, stEncapResult->parameterBlobLen);
				if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B)
				{
					resultHMACFromCV = (PBYTE) malloc(SHA256_LEN);
					if( NULL == resultHMACFromCV)
					{
						logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
						/*return status;*/
						status = CV_MEMORY_ALLOCATION_FAIL ;
						goto cleanup;
					}
					memset(resultHMACFromCV, 0x00, SHA256_LEN);
					memcpy(resultHMACFromCV, (uint8_t*)(pbDecryptedParamBlob + stEncapResult->parameterBlobLen), SHA256_LEN);

					pbKeyToHMAC = (uint8_t *)malloc(SHA256_LEN);
					if(pbKeyToHMAC == NULL)
					{
						logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
						status = CV_MEMORY_ALLOCATION_FAIL ;
						goto cleanup;
					}
					memset(pbKeyToHMAC, 0, SHA256_LEN);

					memcpy(pbKeyToHMAC, stSession->secureSessionHMACKey.SHA256, SHA256_LEN);
				}
				else //Non-SuiteB
				{
					resultHMACFromCV = (PBYTE) malloc(SHA1_LEN);
					if( NULL == resultHMACFromCV)
					{
						logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
						/*return status;*/
						status = CV_MEMORY_ALLOCATION_FAIL ;
						goto cleanup;
					}
					memset(resultHMACFromCV, 0x00, SHA1_LEN);
					memcpy(resultHMACFromCV, (uint8_t*)(pbDecryptedParamBlob + stEncapResult->parameterBlobLen), SHA1_LEN);

					pbKeyToHMAC = (uint8_t *)malloc(SHA1_LEN);
					if(pbKeyToHMAC == NULL)
					{
						logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
						status = CV_MEMORY_ALLOCATION_FAIL ;
						goto cleanup;
					}
					memset(pbKeyToHMAC, 0, SHA1_LEN);

					memcpy(pbKeyToHMAC, stSession->secureSessionHMACKey.SHA1, SHA1_LEN);
				}



				if(stEncapResult->parameterBlobLen >  0)
				{
					//Get the Lower and upper Paramblob from encapsulated result
					nLowerBound = actualParamBlob;
					nUpperBound = (ULONG_PTR)(actualParamBlob + stEncapResult->parameterBlobLen);

				}
				else
				{
					nLowerBound = (byte *)&stEncapResult->parameterBlob;
					nUpperBound = (ULONG_PTR)((ULONG_PTR)&stEncapResult->parameterBlob + stEncapResult->parameterBlobLen);

				}


				//Verification of HMAC
				resultEncapCmdHdrLen = (sizeof(cv_encap_command)- sizeof(stEncapResult->parameterBlob) + stEncapResult->parameterBlobLen + sizeof(stSession->secureSessionUsageCount));
				resultEncapCmdHdr = (uint8_t*) malloc( resultEncapCmdHdrLen);
				if( NULL == resultEncapCmdHdr)
				{
					logErrorMessage(" Error while allocating memory", ERROR_LOCATION);
					/*return status;*/
					status = CV_MEMORY_ALLOCATION_FAIL ;
					goto cleanup;
				}

				memset(resultEncapCmdHdr, 0x00, resultEncapCmdHdrLen);
				memcpy(resultEncapCmdHdr, stEncapResult, (sizeof(cv_encap_command)- sizeof(stEncapResult->parameterBlob)));
				memcpy((uint8_t*)(resultEncapCmdHdr + (sizeof(cv_encap_command)- sizeof(stEncapResult->parameterBlob))), actualParamBlob, stEncapResult->parameterBlobLen);
				memcpy((uint8_t*) (resultEncapCmdHdr + (sizeof(cv_encap_command)- sizeof(stEncapResult->parameterBlob)) + stEncapResult->parameterBlobLen), &stSession->secureSessionUsageCount, sizeof(uint32_t));

				if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B)
				{
					if ( memcmp(pbResultHMAC, resultHMACFromCV, SHA256_LEN) != 0)
					{
						logErrorMessage(" Verification of HMAC failed", ERROR_LOCATION);
						status = CV_HMAC_VERIFICATION_FAILURE;
						goto cleanup;
					}
				}
				else
				{
					if ( memcmp(pbResultHMAC, resultHMACFromCV, SHA1_LEN) != 0)
					{
						logErrorMessage(" Verification of HMAC failed", ERROR_LOCATION);
						status = CV_HMAC_VERIFICATION_FAILURE;
						goto cleanup;
					}
				}
			}
			else
			{
				//Get the Lower and upper Paramblob from encapsulated result
				nLowerBound = (byte *)&stEncapResult->parameterBlob;
				nUpperBound = (ULONG_PTR)((ULONG_PTR)&stEncapResult->parameterBlob + stEncapResult->parameterBlobLen);
			}
		}
		else //if it is not secure session
		{
			//Get the Lower and upper Paramblob from encapsulated result
			nLowerBound = (byte *)&stEncapResult->parameterBlob;
			nUpperBound = (ULONG_PTR)((ULONG_PTR)&stEncapResult->parameterBlob + stEncapResult->parameterBlobLen);

		}

		//Get the upperbound and lowerbound of the paramblob and decapsulate the result
		while(((ULONG_PTR)nLowerBound) < nUpperBound)
		{
			//validating the param type at encap result buffer
			status = is_valid_param_type(((cv_param_list_entry*)nLowerBound)->paramType);
			if(status != CV_SUCCESS)
			{
				goto cleanup;
			}

			//status = CV_SUCCESS (OR) status = CV_INVALID_OUTPUT_PARAMETER_LENGTH and not param type = cv encap inout lenval pair
			//copy type, len and data
			if((stEncapResult->returnStatus == CV_SUCCESS) || (stEncapResult->returnStatus == CV_ENUMERATION_BUFFER_FULL) ||
				((stEncapResult->returnStatus == CV_FP_DUPLICATE) && (((cv_param_list_entry*)nLowerBound)->paramType != CV_ENCAP_INOUT_LENVAL_PAIR)) ||
				((stEncapResult->returnStatus == CV_INVALID_OUTPUT_PARAMETER_LENGTH) && (((cv_param_list_entry*)nLowerBound)->paramType != CV_ENCAP_INOUT_LENVAL_PAIR)))
			{
				byte		*tmpParam = NULL;
				byte		*tmpParamAddrs = NULL;
				uint32_t	nDataSize = 0;
				uint32_t	nBytesLeft = 0;

				//length of the inbound param type
				nParamLen = ((cv_param_list_entry*)nLowerBound)->paramLen + sizeof(cv_param_list_entry);
				tmpParam = (uint8_t *)malloc(nParamLen);
				if ( NULL == tmpParam)
				{
					status =  CV_MEMORY_ALLOCATION_FAIL;
					Free((void *)&tmpParam, 1);
					goto cleanup;
				}
				memset(tmpParam, 0x00, nParamLen);

				tmpParamAddrs = tmpParam;

				// Calculate the datasize of max possible multiple of 4 bytes and bytes left
				nDataSize = (nParamLen%PACKING_SIZE)? (nParamLen/PACKING_SIZE)*PACKING_SIZE: nParamLen;
				nBytesLeft = (nParamLen%PACKING_SIZE);


				// copy the datasize of max possible multiple of 4 bytes
				if(nDataSize >= PACKING_SIZE)
				{
					memcpy(tmpParam, nLowerBound, nDataSize);
					tmpParam += nDataSize;
					nLowerBound += nDataSize;
				}

				// copy the bytes left if any
				if(nBytesLeft > 0)
				{
					// Padding method, eg: 12c40000
					memcpy(tmpParam, nLowerBound, nBytesLeft);
					nLowerBound += sizeof(uint32_t);
				}

				tmpParam = tmpParamAddrs;

				//copy the date to return parms
				memset(retDecapParams[nParams],0,ULSIZE); //setting null value based on OS
				memcpy(retDecapParams[nParams], tmpParam, nParamLen);

				if(tmpParam != NULL)
				{
					free(tmpParam);

				}
			}
			//if status = CV_INVALID_OUTPUT_PARAMETER_LENGTH and parmtype = CV_ENCAP_INOUT_LENVAL_PAIR
			//then this type will not have data, copy only paramtype amd len
			else if((stEncapResult->returnStatus == CV_INVALID_OUTPUT_PARAMETER_LENGTH) &&
				(((cv_param_list_entry*)nLowerBound)->paramType == CV_ENCAP_INOUT_LENVAL_PAIR))
			{
				//length of the inbound param type
				nParamLen = sizeof(cv_param_list_entry);

				//copy only type and len to return parms
				memset(retDecapParams[nParams],0,ULSIZE); //setting null value based on OS
				memcpy(retDecapParams[nParams], nLowerBound, nParamLen);
				nLowerBound += nParamLen;
			}
			//if status = CV_FP_DUPLICATE and parmtype = CV_ENCAP_INOUT_LENVAL_PAIR
			//then this type will not have data, copy only paramtype amd len
			else if((stEncapResult->returnStatus == CV_FP_DUPLICATE) &&
				(((cv_param_list_entry*)nLowerBound)->paramType == CV_ENCAP_INOUT_LENVAL_PAIR))
			{
				//length of the inbound param type
				nParamLen = sizeof(cv_param_list_entry);

				//copy only type and len to return parms
				memset(retDecapParams[nParams],0,ULSIZE); //setting null value based on OS
				memcpy(retDecapParams[nParams], nLowerBound, nParamLen);
				nLowerBound += nParamLen;
			}

			nParams++;

			// Limit the execution if InBoundParams are more then 24
			if(nParams > MaxInboundlist)
			{
				status = CV_PARAM_BLOB_INVALID_LENGTH;
				goto cleanup;
			}
		}
		*NoOfParms = nParams;
	}
	END_TRY

cleanup:

	//Free the memory
	Free((void *)&pbDecryptedParamBlob, 1);
	Free((void *)&pbKeyToHMAC, 1);
	Free((void *)&actualParamBlob, 1);
	Free((void *)&resultEncapCmdHdr, 1);
	Free((void *)&pbResultHMAC, 1);
	Free((void *)&resultHMACFromCV, 1);

	return status;
}


/*
* Function:
*      cvhDecryptCmd
* Purpose:`
*      Decrypt the resulted information from CV
* Parameters:
*		stInEncapCmd	:	Structure of the Encapsulated command
*
* ReturnValue:
*		pbDecryptedParamBlob	:	Decrypted param blob structure
*		pbdwBlobLen				:	Length of the inbound list
*		cv_status				:	return status
*/
cv_status cvhDecryptCmd(
						IN 	cv_session *pActualSession,
						cv_encap_command *stEncapResult,
						uint8_t **pbDecryptedParamBlob,
						DWORD *pbdwBlobLen)
{
	cv_session*			stSession = NULL;
	cv_session_details	*stSessionDetails = NULL;
	cv_status			status = CV_SUCCESS;
	uint8_t				*mDecryptedParamBlob = NULL;
	DWORD				mdwBlobLen = 0;
	unsigned int		bResult = SUCCESS;

	BEGIN_TRY
	{

		if(stEncapResult->commandID != CV_CMD_OPEN)
		{

			/*Get the address of the Sessions structure associated with the this handle*/
			stSessionDetails = (cv_session_details*)GetSessionOfTheCVInternalHandle((stEncapResult->hSession));
			if(stSessionDetails == NULL)
			{
				logErrorMessage("Session Handle not found", ERROR_LOCATION);
				status = CV_INVALID_HANDLE;
				return status;
			}

			/*Point to to address of the cv session in cv_session_details*/
			stSession = &stSessionDetails->stCVSession;
			if(stSession == NULL)
			{
				logErrorMessage("Invalid Session structure", ERROR_LOCATION);
				status = CV_INVALID_HANDLE;
				return status;
			}
		}
		else
		{
			/*copy the actual session*/
			stSession = pActualSession;
		}


		//Call DecryptParamBlob cmd to decrypt the encapsulted result
//dgb		bResult = MSCAPI_DecryptParamBlob(BlobData, dwEncryptedBlobLen,
//dgb			(uint8_t*)stEncapResult->IV, pbSecureSessionKey, &mDecryptedParamBlob, &mdwBlobLen);
		if(bResult != SUCCESS) {
			Free((void *)&mDecryptedParamBlob, 1 );
			return CV_SECURE_SESSION_FAILURE;
		}

		//copy to the return value
		*pbDecryptedParamBlob = mDecryptedParamBlob;
		*pbdwBlobLen = mdwBlobLen;
	}
	END_TRY

		return status;
}


/*
* Function:
*      cvhManageCallback
* Purpose:
*		If the return type of flags in Encapsulated result is either Intermediate callback or
*		Completion Callback then this routine is internally called by User Mode Interface.
* Parameters:
*		status			:	Return value of the Encapsulated Result
*		extraDataLength	:
*		extraData		:
*		ptr_encap_result:	Pointer to Encapusulated result
* ReturnValue:
*      cv_status
*/
cv_status
cvhManageCallback(
				  IN	uint32_t status,
				  IN	uint32_t extraDataLength,
				  IN	void *extraData,
				  OUT	void  *ptr_encap_result
				  )
{
	uint32_t				NoOfParams = 0;
	uint32_t				nParams=0;
	void				**ppTmpInbound		= {NULL};
	cv_param_list_entry		*pParamList[MaxInboundlist] = {NULL};
	cv_encap_command		*pResult					= (cv_encap_command*)ptr_encap_result;
	cv_session				*stSession					= NULL;
	cv_session_details		*stSessionDetails			= NULL;
	cv_fp_callback_ctx		*CallbackInfo				= NULL;
	cvh_host_callback		*ptr_host_callback			= NULL;
	byte					*pTmpHostCallback			= NULL;
	PFUNCALLBACK			funPtr;
	cv_status				nRetStatus					= CV_SUCCESS;
	cv_bool					doAppCB = FALSE, doCVCB = FALSE;

	BEGIN_TRY
	{
		/*Get the address of the Sessions structure associated with the this handle*/
		stSessionDetails = (cv_session_details*)GetSessionOfTheCVInternalHandle((pResult->hSession));
		if(stSessionDetails == NULL)
		{
			logErrorMessage("Session Handle not found", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			return status;
		}

		/*Point to to address of the cv session in cv_session_details*/
		stSession = &stSessionDetails->stCVSession;
		if(stSession == NULL)
		{
			logErrorMessage("Invalid Session structure", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			return status;
		}

		// Check whether Status is Completion callback
		if(pResult->flags.returnType == CV_RET_TYPE_RESULT)
		{

			//Get the Host callback context address from Encapresult
			memcpy(&ptr_host_callback,
				(((byte*)pResult) + pResult->transportLen), ULSIZE);

			if(ptr_host_callback == NULL)
			{
				logErrorMessage("Memory Allocation is Fail", ERROR_LOCATION);
				status = CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;

			}

			//store at temporary address
			pTmpHostCallback =(byte *)ptr_host_callback;

			//Get the number of input params
			NoOfParams = ptr_host_callback->numParams;

			//pointing to len at host callback structure
			pTmpHostCallback = pTmpHostCallback + sizeof(cvh_host_callback);

			//Allocating memory to store the address of inbound list based of no of params
			ppTmpInbound = (void**) malloc(NoOfParams * ULSIZE);
			if(ppTmpInbound == NULL)
			{
				logErrorMessage("Memory Allocation is Fail", ERROR_LOCATION);
				status = CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;

			}
			memset(ppTmpInbound, 0, (NoOfParams * ULSIZE));

			//Copy the address of Inbound list from host callback strucuture (cvh host callback + len(uint32_t))
			for(nParams = 0; nParams < NoOfParams; nParams++)
			{
				memcpy(&ppTmpInbound[nParams], (pTmpHostCallback + sizeof(uint32_t)), ULSIZE);
				pTmpHostCallback += ULSIZE + sizeof(uint32_t);
			}

			//Allocating memory for temp inbound param list to store the result
			*pParamList = (cv_param_list_entry*) malloc(pResult->parameterBlobLen);
			if(*pParamList == NULL)
			{
				logErrorMessage("Memory Allocation Failed", ERROR_LOCATION);
				status = CV_MEMORY_ALLOCATION_FAIL;
				goto clean_up;
			}
			memset(*pParamList, 0, pResult->parameterBlobLen);

			//call Decapsulate cmd to store the result
			status = cvhDecapsulateCmd(ptr_encap_result, stSession, &NoOfParams, pParamList);
			if (status != CV_SUCCESS)
			{
				logErrorMessage("cvhDecapsulateCmd is Fail", ERROR_LOCATION);
				goto clean_up;
			}


			//Call save return values to store the result at inbound list array
			//need to verify for cv_encap_inout_lenval_pair, by default passing 0
			status = cvhSaveReturnValues(ppTmpInbound, NoOfParams, 0, pParamList);
			if (status != CV_SUCCESS)
			{
				logErrorMessage("cvhDecapsulateCmd is Fail", ERROR_LOCATION);
				goto clean_up;
			}


			//call application callback with context
			funPtr = ptr_host_callback->callback;
			nRetStatus = funPtr(status, 0,NULL,*ppTmpInbound); //need to clarify
			status = nRetStatus;

			//free the context
			Free((void *)&ptr_host_callback,1);


		}


		// Check whether Status is Intermediate callback
		if(pResult->flags.returnType == CV_RET_TYPE_INTERMED_STATUS)
		{
			// Check the session flags if the Application can handle Intermediate Callbacks or not
			//copy the address of cv_fp_callback_ctx from encapresult (transportlen + managecallback)
			memcpy(&CallbackInfo, (((byte*)pResult) + pResult->transportLen), ULSIZE);
			if ((stSession->flags & CV_INTERMEDIATE_CALLBACKS) == CV_INTERMEDIATE_CALLBACKS)
			{
				// check to see if FP only callbacks are to be received
				if((stSession->flags & CV_FP_INTERMEDIATE_CALLBACKS) == CV_FP_INTERMEDIATE_CALLBACKS)
				{
					// check to see if this is an FP or general callback
					if (((pResult->returnStatus & FP_PROMPT_RETURN_CODE_MASK) == FP_PROMPT_RETURN_CODE_MASK))
					{
						// This is an FP callback and app will handle
						doAppCB = TRUE;
						stSessionDetails->hasAppCallbacks = TRUE;
					} else if((pResult->returnStatus & GENERAL_PROMPT_RETURN_CODE_MASK) == GENERAL_PROMPT_RETURN_CODE_MASK) {
						/* these prompts could be either FP specific or for non-FP prompting.  determine where it gets */
						/* handled based on how prior prompts in this session were handled */
						if (stSessionDetails->hasAppCallbacks)
							doAppCB = TRUE;
						else
							doCVCB = TRUE;
					} else
						// this is a non-FP callback
						doCVCB = TRUE;
				} else
					// app indicates that it handles all callbacks
					doAppCB = TRUE;
			} else 
				// app doesn't handle callbacks let CV handle it
				doCVCB = TRUE;
			if (doAppCB)
			{
				//call App callback
				if(CallbackInfo->callback != NULL)
				{
					funPtr = CallbackInfo->callback;
					nRetStatus = funPtr(status, 0, NULL, CallbackInfo->context); //need to clarify
					status = nRetStatus;
				}
			} else if (doCVCB && ((stSession->flags & CV_SUPPRESS_UI_PROMPTS) != CV_SUPPRESS_UI_PROMPTS)) {
				//call cvhUserPrompt to display userprompt
				nRetStatus = fncvhUserPrompt(status,0,NULL);

				//User action nRetstatus (success / cancel) uerprompt dlg sent to DLL
				status = nRetStatus;
			} else
				// suppressing prompts, just return success
				status = CV_SUCCESS;
		}
	}
	END_TRY

clean_up:
	// Freeing memory
	Free((void *)pParamList, NoOfParams);
	Free((void *)ppTmpInbound, NoOfParams); // Freeing the individual pointers
	Free((void**)&ppTmpInbound, 1); // Freeing the main pointer (array of pointers)

	return status;
}


/*
* Function:
*      cvhSaveReturnValues
* Purpose:
*       Saves the return values received from the CV
* Parameters:
*		Param			:	Pointer to param list to Save the return values
*		NoOfParams		:	Number of Params
*		nRetStatus		:   Return status of the CV to validate the CV_INVALID_OUTPUT_PARAMETER_LENGTH
*		inBoundList		:	List of Inbound received from CV
* ReturnValue:
*		Param			:	List of save return values
*		cv_status
*/
cv_status
cvhSaveReturnValues(
					IN	void **Param,
					IN	uint32_t NoOfParams,
					IN  unsigned int nRetStatus,
					OUT	cv_param_list_entry **inBoundList
					)
{
	uint32_t		nInBoundListCount = 0;
	cv_status		status = CV_SUCCESS;
	uint32_t		nInParamCount = 0;

	BEGIN_TRY
	{
		for(nInBoundListCount = 0; nInBoundListCount < NoOfParams; nInBoundListCount++)
		{
			switch(inBoundList[nInBoundListCount]->paramType)
			{
			case CV_ENCAP_STRUC:
				{
					memcpy(((byte *)Param[nInParamCount++]), ((byte *)inBoundList[nInBoundListCount]) + sizeof(cv_param_list_entry), inBoundList[nInBoundListCount]->paramLen);
					break;
				}
			case CV_ENCAP_LENVAL_STRUC:
				{
					memcpy((uint32_t *)Param[nInParamCount++], ((byte *)inBoundList[nInBoundListCount]) + sizeof(cv_param_list_entry), inBoundList[nInBoundListCount]->paramLen);
					break;
				}
			case CV_ENCAP_LENVAL_PAIR:
				{
					// code needs to be finalized and written for this type
					nInParamCount++;
					break;
				}
			case CV_ENCAP_INOUT_LENVAL_PAIR:
				{
					memcpy(Param[nInParamCount++], &inBoundList[nInBoundListCount]->paramLen, sizeof(uint32_t));
					//NO Data for this status code
					if(nRetStatus != CV_INVALID_OUTPUT_PARAMETER_LENGTH)
					{
						memcpy((uint16_t *)Param[nInParamCount++], ((byte *)inBoundList[nInBoundListCount]) + sizeof(cv_param_list_entry), inBoundList[nInBoundListCount]->paramLen);
					}
					else //increment the paramcount
					{
						nInParamCount++;
					}
					break;
				}
			default:
				{
					nInParamCount++;
					status = CV_INVALID_INBOUND_PARAM_TYPE;
					break;
				}
			}
		}


	}
	END_TRY

		return status;
}


/*
* Function:
*      cvhSetUpSecureSession
* Purpose:
*      Function to create the Secure Session between the Application & the CV.
* Parameters:
*		pbSecureSessionKey				- Secure Session key for Encrypt/Decrypting the
*										  OutBound/InBound Params
*		pbSecureSessionHMACKey			- Session HMAC Key for HMACing the Encapsulated Command
*
* Returns:
*      cv_status
*/
cv_status
cvhSetUpSecureSession(
					  IN	BOOL bWinVista,
					  IN	uint8_t **pbSecureSessionKey,
					  IN    cv_session *stSession,
					  OUT	uint8_t **pbSecureSessionHMACKey
					  )
{
	uint8_t      								pbAntiReplayNonce[ANTI_REPLAY_NONCE_SIZE] = {0};
	uint8_t      								pbHostNonce[HOST_NONCE_SIZE] = {0};
	BOOL										bSuiteB = FALSE ;
	cv_status									status = CV_SUCCESS ;
	uint8_t*									cmdParamBlob = NULL;
	uint8_t										*pbEncHostNonce = NULL;
	uint8_t										*pbECCHostPublicKey = NULL;
	cv_secure_session_get_pubkey_in*			cvSecureSessionPubKey = 0;
	cv_secure_session_get_pubkey_in_suite_b*	cvSecureSessionSuiteBPubKey = 0;
	cv_encap_command							*encapCmdGetPubKeyRequest = NULL;
	cv_encap_command							*encapCmdHostNonceRequest = NULL;
	cv_encap_command							*encapCmdResult = NULL;
	cv_status									nRetStatus = CV_SUCCESS;


	//Validate the CV_USE_SUITE_B is set / not
	if((stSession->flags & CV_USE_SUITE_B) == CV_USE_SUITE_B) //vista
	{
		bSuiteB = TRUE;
	}

	if(!bSuiteB) //not bSuiteB
	{
		//Function for generating the Replay Nonce
//dgb		if ((bResult = MSCAPI_GenerateRandom(pbAntiReplayNonce, ANTI_REPLAY_NONCE_SIZE))!= SUCCESS)
		{
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}
		if ((status = GetCVPublicKeyCmd(bSuiteB, pbAntiReplayNonce, &encapCmdGetPubKeyRequest))!= CV_SUCCESS)
		{
			//status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		encapCmdResult = (cv_encap_command*)malloc(MAX_ENCAP_RESULT_BUFFER);
		if( NULL == encapCmdResult )
		{
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}
		memset(encapCmdResult, 0x00,MAX_ENCAP_RESULT_BUFFER);

		// Submit the request to CV and get the result
		status = ProcessTransmitCmd(stSession, &encapCmdGetPubKeyRequest, &nRetStatus, &encapCmdResult);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("ProcessTransmitCmd is Fail ", ERROR_LOCATION);
			goto clean_up;
		}
		if(nRetStatus != CV_SUCCESS)
		{
			logErrorMessage("EncapCmdGetPubKeyRequest is Fail ", ERROR_LOCATION);
			status = nRetStatus;
			goto clean_up;
		}

		cmdParamBlob = (uint8_t*)malloc(encapCmdResult->parameterBlobLen);
		if( NULL == cmdParamBlob )
		{
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}
		// Parse the encapsulated result and get the Public Key
		memcpy(cmdParamBlob, &encapCmdResult->parameterBlob, encapCmdResult->parameterBlobLen);

		//Read all the values from the Blob
		cvSecureSessionPubKey = (cv_secure_session_get_pubkey_in*) malloc(encapCmdResult->parameterBlobLen);
		if( NULL == cvSecureSessionPubKey )
		{
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}

		memcpy(cvSecureSessionPubKey->pubkey, cmdParamBlob, RSA_PUBLIC_KEY_SIZE);
		memcpy(cvSecureSessionPubKey->sig, ((uint8_t*)cmdParamBlob + RSA_PUBLIC_KEY_SIZE), DSA_SIGNATURE_LENGTH);
		memcpy(cvSecureSessionPubKey->nonce, ((uint8_t*)cmdParamBlob + RSA_PUBLIC_KEY_SIZE + DSA_SIGNATURE_LENGTH), CV_NONCE_SIZE);
		memcpy(&cvSecureSessionPubKey->useSuiteB, ((uint8_t*)cmdParamBlob + RSA_PUBLIC_KEY_SIZE + DSA_SIGNATURE_LENGTH + CV_NONCE_SIZE), sizeof(uint32_t));


		// Check the existance of the Certificate. If exists, do the Signature Verification only one for all 
		// sessions in the same process
		if (!bKDICertiVerificationDone)
		{
			// Verify the RSA Signature of the Kdi Certificate using BRCM Certificate
//dgb			if ((bResult = MSCAPI_VerifyRSASignature()) != SUCCESS )
			{
				status = CV_SECURE_SESSION_FAILURE;
				goto clean_up;
			}
	
			bKDICertiVerificationDone = TRUE ;
		}

		// In case of Non-SuiteB, only Encrypted Host Nonce is sent to CV
		if ((status = GetHostEncryptNonceCmd(bSuiteB, pbEncHostNonce, NULL, &encapCmdHostNonceRequest)) != CV_SUCCESS )
		{
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		//Copy the handle to the encapCmdHostNonceRequest's hSession and pass to CV
		encapCmdHostNonceRequest->hSession = encapCmdResult->hSession;

		memset(encapCmdResult, 0x00, MAX_ENCAP_RESULT_BUFFER);

		// Submit the request to CV and get the result
		status = ProcessTransmitCmd( stSession, &encapCmdHostNonceRequest, &nRetStatus, &encapCmdResult);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("ProcessTransmitCmd is Fail ", ERROR_LOCATION);
			goto clean_up;
		}
		if(nRetStatus != CV_SUCCESS)
		{
			logErrorMessage("EncapCmdGetPubKeyRequest is Fail ", ERROR_LOCATION);
			status = nRetStatus;
			goto clean_up;
		}

		//Copy the hSession (handle) into the magicNumber element of cv_session
		stSession->magicNumber =  encapCmdResult->hSession;
	}
	else	//In case of SuiteB
	{
		//Function for generating the Anti-Replay Nonce
//dgb		if (!MSCNG_GenerateRandom(pbAntiReplayNonce, ANTI_REPLAY_NONCE_SIZE))
		{
			//logMessage("An error occurred while generating the Replay Nonce %x \n", GetLastError());
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		if ((status = GetCVPublicKeyCmd(bSuiteB, pbAntiReplayNonce, &encapCmdGetPubKeyRequest ))!= CV_SUCCESS)
		{
			//logMessage("An error occurred while retrieving the CV Public Key %x \n", GetLastError());
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		encapCmdResult = (cv_encap_command*)malloc(MAX_ENCAP_RESULT_BUFFER);
		if( NULL == encapCmdResult )
		{
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}
		memset(encapCmdResult, 0x00,MAX_ENCAP_RESULT_BUFFER);

		// Submit the request to CV and get the result
		status = ProcessTransmitCmd(stSession, &encapCmdGetPubKeyRequest, &nRetStatus, &encapCmdResult);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("ProcessTransmitCmd is Fail ", ERROR_LOCATION);
			goto clean_up;
		}
		if(nRetStatus != CV_SUCCESS)
		{
			logErrorMessage("EncapCmdGetPubKeyRequest is Fail ", ERROR_LOCATION);
			status = nRetStatus;
			goto clean_up;
		}

		cmdParamBlob = (uint8_t*)malloc(encapCmdResult->parameterBlobLen);
		if( NULL == cmdParamBlob )
		{
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}
		// Parse the encapsulated result and get the Public Key
		memcpy(cmdParamBlob, &encapCmdResult->parameterBlob, encapCmdResult->parameterBlobLen);

		cvSecureSessionSuiteBPubKey = (cv_secure_session_get_pubkey_in_suite_b*) malloc(encapCmdResult->parameterBlobLen);
		if(cvSecureSessionSuiteBPubKey == NULL)
		{
			logErrorMessage("Memory Allocation Failed ", ERROR_LOCATION);
			status = CV_MEMORY_ALLOCATION_FAIL;
			goto clean_up;
		}

		memcpy(cvSecureSessionSuiteBPubKey->pubkey, cmdParamBlob, ECC256_POINT);
		memcpy(cvSecureSessionSuiteBPubKey->sig, ((uint8_t*)cmdParamBlob + ECC256_POINT), ECC256_SIG_LEN);
		memcpy(cvSecureSessionSuiteBPubKey->nonce, ((uint8_t*)cmdParamBlob + ECC256_POINT + ECC256_SIG_LEN), CV_NONCE_SIZE);
		memcpy(&cvSecureSessionSuiteBPubKey->useSuiteB, ((uint8_t*)cmdParamBlob + ECC256_POINT + ECC256_SIG_LEN + CV_NONCE_SIZE), sizeof(uint32_t));

//dgb		if(!MSCNG_GenerateHostECCKey(&pbECCHostPublicKey))
		{
			logMessage("An error occurred while generating the Host ECC Key 0x%x \n", GetLastError());
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		// Check the existance of the Certificate. If exists, do the Signature Verification only one for all 
		// sessions in the same process
		if (!bKDICertiVerificationDone)
		{
			// Verify the RSA Signature of the Kdi Certificate using BRCM Certificate
//dgb			if ((bResult = MSCAPI_VerifyRSASignature()) != SUCCESS )
			{
				status = CV_SECURE_SESSION_FAILURE;
				goto clean_up;
			}
	
			bKDICertiVerificationDone = TRUE ;
		}		

		//Function for generating the Replay Nonce
//dgb		if (!MSCNG_GenerateRandom(pbHostNonce, HOST_NONCE_SIZE))
		{
			//logMessage("An error occurred while generating the Host nonce %x \n", GetLastError());
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		// In case of SuiteB, Host Nonce (UNENCRYPTED) along with the Host Public Key is sent to CV
		nRetStatus = GetHostEncryptNonceCmd(bSuiteB, pbHostNonce/*pbEncHostNonce*/, pbECCHostPublicKey, &encapCmdHostNonceRequest);
		if (nRetStatus != CV_SUCCESS)
		{
			status = CV_SECURE_SESSION_FAILURE;
			goto clean_up;
		}

		//Copy the handle to the encapCmdHostNonceRequest's hSession and pass to CV
		encapCmdHostNonceRequest->hSession = encapCmdResult->hSession;

		memset(encapCmdResult, 0x00,MAX_ENCAP_RESULT_BUFFER);

		// capturing the HostNonce command
		//LogDataToFile(ENCAP_CMD_BUFF, "Sending the HostNonce Encap Command", strlen("Sending the HostNonce Encap Command"), (uint8_t*) encapCmdHostNonceRequest, encapCmdHostNonceRequest->transportLen, TRUE);


		// Submit the request to CV and get the result
		status = ProcessTransmitCmd( stSession, &encapCmdHostNonceRequest, &nRetStatus, &encapCmdResult);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("ProcessTransmitCmd is Fail ", ERROR_LOCATION);
			goto clean_up;
		}
		if(nRetStatus != CV_SUCCESS)
		{
			logErrorMessage("EncapCmdGetPubKeyRequest is Fail ", ERROR_LOCATION);
			status = nRetStatus;
			goto clean_up;
		}

		// capturing the Host Nonce command response
		//LogDataToFile(ENCAP_CMD_BUFF, "Host nonce Request response", strlen("Host nonce Request response"), (uint8_t*) encapCmdResult, encapCmdResult->transportLen, TRUE);

		//Copy the hSession (handle) into the magicNumber element of cv_session
		stSession->magicNumber =  encapCmdResult->hSession;
	}


clean_up:
	Free((void *)&encapCmdResult,1);
	Free((void *)&cvSecureSessionPubKey,1);
	Free((void *)&cmdParamBlob,1);
	Free((void *)&encapCmdGetPubKeyRequest, 1);
	Free((void *)&encapCmdHostNonceRequest, 1);
	Free((void *)&pbEncHostNonce, 1);
	Free((void *)&cvSecureSessionSuiteBPubKey, 1);
	Free((void *)&pbECCHostPublicKey, 1);

	return status;
}

