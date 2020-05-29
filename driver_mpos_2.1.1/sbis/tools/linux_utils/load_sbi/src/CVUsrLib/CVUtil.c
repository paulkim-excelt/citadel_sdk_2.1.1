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
#include "CVUsrLibGlobals.h"
#include "CVList.h"
#include "HelperFunctions.h"
#include "CVLogger.h"

//dgb
cv_bool		asyncFPCaptureRequest = FALSE;

/*CVUtility Routines*/

/*
* Function:
*      cv_open
* Purpose:
*      Opens CV Session for Application and User
*
* Parameters:
*		options :	options relating to the new session to be established.
*		appID	:	identifier allowing the CV manager to partition
*					CV objects based on the applications which create them.
*		userID	:	identifier allowing the CV manager to partition
*					the CV objects based on the users which create them.
*		objAuth :	optional authorization associated with all objects
*					created/accessed by this session. It adds an additional
*					encryption wrapper to objects.
*		cvHandle:	a pointer to an opaque structure which will allow
*					the CV to manage communications with the app
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_open (
		 IN	cv_options options,
		 IN	cv_app_id *appID,
		 IN	cv_user_id *userID,
		 IN	cv_obj_value_passphrase *objAuth,
		 OUT	cv_handle *cvHandle
		 )
{

	const short MAX_OUT_PARAM = 4;
	const short MAX_IN_PARAM = 1;

	void		*inParams[1]= {NULL};
	cv_status	status = CV_SUCCESS;

	//cv_xxx Index of the cv_auth_list
	uint32_t	auth_list_idx = 0;

	cv_param_list_entry *out_param_list_entry[4] = {NULL};
	cv_param_list_entry *in_param_list_entry[1] = {NULL};
	uint32_t retryCount = 0;

	//entry for testing
	logErrorMessage("Inside cv_open....", ERROR_LOCATION);

	BEGIN_TRY
	{
		// Validating for Inbound and Outbound parameters
		if(options == 0)
		{
			logErrorMessage("cv_option Parameter is null", ERROR_LOCATION);
			return CV_INVALID_OPTIONS;
		}

		//Validating the return value
		if(cvHandle == NULL)
		{
			status = CV_INVALID_INPUT_PARAMETER;
			goto clean_up;
		}

		//Adding value to Outbound/Inbound parameter list
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(options), &options, &out_param_list_entry[0])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the Outbound List into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//appid is optional parameter user may or may not pass this parameter
		if(appID != NULL && appID->appIDLen != 0)
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC, appID->appIDLen, appID->appID, &out_param_list_entry[1]);
		else
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC,0, NULL, &out_param_list_entry[1]);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the Inbound List into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//userid is optional parameter user may or may not pass this parameter
		if(userID != NULL &&  userID->userIDLen != 0)
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC, userID->userIDLen, userID->userID, &out_param_list_entry[2]);
		else
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC, 0, NULL, &out_param_list_entry[2]);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the Inbound List into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//cv_obj_value_passphrase is optional parameter user may or may not pass this parameter
		if(objAuth != NULL && objAuth->passphraseLen != 0)
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC, objAuth->passphraseLen, objAuth->passphrase, &out_param_list_entry[3]);
		else
			status = InitParam_List(CV_ENCAP_LENVAL_STRUC, 0, NULL, &out_param_list_entry[3]);
		if(status!= CV_SUCCESS)
		{
			logErrorMessage("Error while copying the Object Auth parameter into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//Inbound list param
		if((status = InitParam_List(CV_ENCAP_STRUC,sizeof(cv_handle), cvHandle, &in_param_list_entry[0]))!= CV_SUCCESS)
		{
			logErrorMessage("Error while copying the cv_handle paramter into Object Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//calling cvhManageCVAPI call
		do 
		{
			status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry,  MAX_IN_PARAM, in_param_list_entry, NULL, 0 /* Handle is NULL for cv_open()*/, auth_list_idx, CV_CMD_OPEN);
			if(status!= CV_SUCCESS)
			{
				/* check to see if the error is due to a failure that may have occurred because CV */
				/* wasn't ready to accept commands yet.  if so, wait for CV to be up and if successful, */
				/* do a retry */
				/* don't do retry if async FP capture request, just fail (app level will retry) */
				if (status == CV_UI_FAILED_COMMAND_PROCESS && !asyncFPCaptureRequest)
				{
					logErrorMessage("cv_open: detected CV_UI_FAILED_COMMAND_PROCESS", ERROR_LOCATION);
					retryCount = 1;
				} else if (status == CV_INVALID_HANDLE) {
					/* this may happen if hibernate occurs during setting up a secure session and indicates */
					/* that CV is up, but cv_open needs to be resubmitted */
					logErrorMessage("cv_open: detected CV_INVALID_HANDLE", ERROR_LOCATION);
					retryCount = 1;					
				} else {
					logErrorMessage("Error returned by cvhManageCVAPICall", ERROR_LOCATION);
					goto clean_up;
				}
			}
		} while (retryCount--);

		//Intialization of return values
		*cvHandle = 0;

		//calling cvhSaveReturnValues and status as 0
		inParams[0] = cvHandle;
		status = cvhSaveReturnValues(inParams, MAX_IN_PARAM, status, in_param_list_entry);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhSaveReturnValue", ERROR_LOCATION);
			goto clean_up;
		}

		// Freeing the memory of OutBound and InBound params
clean_up:
		Free((void *)out_param_list_entry, MAX_OUT_PARAM);
		Free((void *)in_param_list_entry, MAX_IN_PARAM);
		inParams[0] = NULL;

	}
	END_TRY

		logErrorMessage("Exiting from cv_open", ERROR_LOCATION);

	return status;
}

/*
* Function:
*      cv_close
*
* Purpose:
*      Close the CV Session
*
* Parameters:
*      cvHandle	: Close the CV handle created by session
*
* ReturnValue:
*		cv_status
*
*/
CVUSERLIBIFC_API cv_status
cv_close(
		 IN	cv_handle cvHandle
		 )
{

	const uint32_t MAX_OUT_PARAM = 1;
	const uint32_t MAX_IN_PARAM = 0;

	cv_param_list_entry		*out_param_list_entry[1] = {NULL};
	cv_status				status = CV_SUCCESS;
	cv_handle				cvInternalHandle = 0;
	cv_bool					retVal = FALSE;

	//cv_xxx Index of the cv_auth_list
	uint32_t				auth_list_idx = 0;
	uint32_t retryCount = 0;

	//entry for testing
	logErrorMessage("Inside cv_close", ERROR_LOCATION);

	BEGIN_TRY
	{
		// Validating the Input parameter
		if(!is_valid_handle(cvHandle))
		{
			logErrorMessage("Invalid CV Handle Value", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			goto clean_up;
		}

		//get the CV internal handle
		if((cvInternalHandle = GetCVInternalHandle(cvHandle)) == 0)
		{
			logErrorMessage("Invalid Handle Value", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			goto clean_up;
		}

		//Adding value to Outbound/Inbound parameter list
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(uint32_t), &cvInternalHandle, &out_param_list_entry[0]))!= CV_SUCCESS)
		{
			logErrorMessage("Error while copying the Not copying the values to cvh_Param_List entry", ERROR_LOCATION);
			goto clean_up;
		}

		//calling cvhManageCVAPI call
		do 
		{
			status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry, MAX_IN_PARAM, NULL, NULL, cvHandle, auth_list_idx, CV_CMD_CLOSE);
			if(status!= CV_SUCCESS)
			{
				/* check to see if the error is due to a failure that may have occurred because CV */
				/* wasn't ready to accept commands yet.  if so, wait for CV to be up and if successful, */
				/* do a retry */
				/* don't do retry if async FP capture request, just fail (app level will retry) */
				if (status == CV_UI_FAILED_COMMAND_PROCESS && !asyncFPCaptureRequest)
				{
					logErrorMessage("cv_close: detected CV_UI_FAILED_COMMAND_PROCESS", ERROR_LOCATION);
					retryCount = 1;
				} else {
					logErrorMessage("Error returned by cvhManageCVAPICall", ERROR_LOCATION);
					goto clean_up;
				}
			}
		} while (retryCount--);


		//Free the Session list handle
		retVal = DeleteSessionHandle(cvHandle);
		if(retVal != TRUE)
		{
			logErrorMessage("Error returned by DeleteSessionHandle", ERROR_LOCATION);
			goto clean_up;
		}

		// Freeing the memory of OutBound and InBound params
clean_up:
		Free((void *)out_param_list_entry, MAX_OUT_PARAM);
	}
	END_TRY

		logErrorMessage("Exiting cv_close", ERROR_LOCATION);

	return status;
}

/*
* Function:
*      cv_get_status
* Purpose:
*      Gets status or configuration information for CVAPI
*
* Parameters:
*		cvHandle	: Previously supplied CV handle
*		statusType	: Type of status requested
*		pStatus		: Pointer to buffer to store status value
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_get_status (
			   IN	cv_handle cvHandle,
			   IN	cv_status_type statusType,
			   OUT	void *pStatus
			   )
{
	const short MAX_OUT_PARAM=2;
	const short MAX_IN_PARAM=1;
	cv_status					status	= CV_SUCCESS;
	cv_param_list_entry			*out_param_list_entry[2] = {NULL};
	cv_param_list_entry			*in_param_list_entry[1] = {NULL};
	uint32_t					pLen = 0;
	void						*inParams[1]= {NULL};
	cv_handle					cvInternalHandle = 0;

	//cv_xxx Index of the cv_auth_list
	uint32_t					auth_list_idx = 0;

	logErrorMessage("Inside cv_get_status", ERROR_LOCATION);

	BEGIN_TRY
	{

		// Validating for Inbound and Outbound parameters
		if(! is_valid_handle(cvHandle))
		{
			logErrorMessage("Invalid CV Handle", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			goto clean_up;
		}

		//get the CV internal handle
		if((cvInternalHandle = GetCVInternalHandle(cvHandle)) == 0)
		{
			logErrorMessage("Invalid Handle Value", ERROR_LOCATION);
			status = CV_INVALID_HANDLE;
			goto clean_up;
		}

		if((status  = is_valid_status_type(statusType))!=CV_SUCCESS)
		{
			logErrorMessage("Invalid Status Type is being passed", ERROR_LOCATION);
			return status;
		}

		//Validating the return value
		if(pStatus == NULL)
		{
			status = CV_INVALID_INPUT_PARAMETER;
			goto clean_up;
		}

		//Adding value to Outbound/Inbound parameter list
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(uint32_t), &cvInternalHandle, &out_param_list_entry[0])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the cv handle into Param Blobl", ERROR_LOCATION);
			goto clean_up;
		}
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(statusType), &statusType, &out_param_list_entry[1])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying status type parameter into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//Getting the size of the valid status type
		if((pLen = GetValidStatusLen(statusType)) == CV_INVALID_STATUS_TYPE)
		{
			logErrorMessage("Error while getting the size of valid status type", ERROR_LOCATION);
			goto clean_up;
		}

		//inbound list param
		if((status = InitParam_List(CV_ENCAP_STRUC, pLen, &pStatus, &in_param_list_entry[0])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying out parameter into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		//calling cvhManageCVAPI call
		status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry,  MAX_IN_PARAM, in_param_list_entry, NULL, cvHandle, auth_list_idx, CV_CMD_GET_STATUS);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
			goto clean_up;
		}

		//Intialization of return values
		if(pLen > 0)
		{
			memset(pStatus, 0x00, pLen);
		}

		//calling cvhSaveReturnValues
		inParams[0] = pStatus;
		status = cvhSaveReturnValues(inParams, MAX_IN_PARAM,status,in_param_list_entry);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by by cvhSaveReturnValue", ERROR_LOCATION);
			goto clean_up;
		}

		// Freeing the memory of OutBound and InBound params
clean_up:
		Free((void*)out_param_list_entry, MAX_OUT_PARAM);
		Free((void*)in_param_list_entry, MAX_IN_PARAM);
		inParams[0] = NULL;
	}
	END_TRY

		logErrorMessage("Exiting cv_get_status", ERROR_LOCATION);
	return status;
}

/*
* Function:
*      cv_get_session_count
* Purpose:
*      This function Retrieves a counter which the CVAPI maintains for each session
*
* Parameters:
*		cvHandle	:	Previously supplied CV handle
*		sessionCount:	Count of number of commands sent for this session
* ReturnValue:
*		cv_status
*
*/


CVUSERLIBIFC_API cv_status
cv_get_session_count(
					 IN	cv_handle cvHandle,
					 OUT	uint32_t *SessionCount
					 )
{
	cv_status				status					= CV_SUCCESS;
	cv_session				*stSession				= NULL;
	cv_session_details		*stSessionDetails		= NULL;

	logErrorMessage("Enter in cv_get_session_count", ERROR_LOCATION);

	BEGIN_TRY
	{
		if(!is_valid_handle(cvHandle))
		{
			logErrorMessage("Invalid Handle Value", ERROR_LOCATION);
			return CV_INVALID_HANDLE;
		}

		//Validating the return value
		if(SessionCount == NULL)
		{
			return CV_INVALID_INPUT_PARAMETER;
		}

		/*Get the address of the Sessions structure associated with the this handle*/
		stSessionDetails = (cv_session_details*)GetSessionOfTheHandle(cvHandle);
		if(stSessionDetails == NULL)
		{
			logErrorMessage("Session Handle not found", ERROR_LOCATION);
			//status = ???;
			return status;
		}

		/*Point to to address of the cv session in cv_session_details*/
		stSession = &stSessionDetails->stCVSession;
		if(stSession == NULL)
		{
			logErrorMessage("Invalid Session structure", ERROR_LOCATION);
			//status = ???;
			return status;
		}

		/*Return the secure session usage count*/
		*SessionCount = stSession->secureSessionUsageCount;
	}
	END_TRY

		logErrorMessage("Exit in cv_get_session_count", ERROR_LOCATION);

	return status;
}


// Added the below functions from BCM code as part of merging - 5th Dec 2007
/*
* Function:
*		cv_get_ush_ver
* Purpose:
*		gets the ush version string
*		results are returned in pResults
*
* Parameters:
*		cvHandle	: Previously supplied CV handle
*		grpmask		: bit mask of the test being issued
*		pResult		: Pointer to uint32 which holds tests return status
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_get_ush_ver (
			   IN	cv_handle cvHandle,
			   IN	uint32_t	bufLen,
			   OUT  uint8_t	*pBuf
)
{
	const short MAX_OUT_PARAM=1;
	const short MAX_IN_PARAM=1;
	cv_status					status	= CV_SUCCESS;
	cv_param_list_entry			*out_param_list_entry[1] = {NULL};
	cv_param_list_entry			*in_param_list_entry[1] = {NULL};
    void						*inParams[1]= {NULL};
	//cv_xxx Index of the cv_auth_list
	uint32_t					auth_list_idx = 0;
	uint32_t                    verParam=0;

	logErrorMessage("Inside cv_get_ush_ver", ERROR_LOCATION);

	BEGIN_TRY
	{

		//Adding value to Outbound/Inbound parameter list
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(verParam), &verParam, &out_param_list_entry[0])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the version parameter into Param Blob", ERROR_LOCATION);
			goto clean_up;
		}

		if((status = InitParam_List(CV_ENCAP_LENVAL_STRUC, bufLen, pBuf, &in_param_list_entry[0])) != CV_SUCCESS)
        {
            logErrorMessage("Error while copying in parameter into Param Blob", ERROR_LOCATION);
            goto clean_up;
        }

		//calling cvhManageCVAPI call
		status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry,  MAX_IN_PARAM, in_param_list_entry, NULL, cvHandle, auth_list_idx, CV_CMD_USH_VERSIONS);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
			goto clean_up;
		}

		//calling cvhSaveReturnValues
        inParams[0] = (void*)pBuf;
		status = cvhSaveReturnValues(inParams, MAX_IN_PARAM, status, in_param_list_entry);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by by cvhSaveReturnValue", ERROR_LOCATION);
			goto clean_up;
		}

		// Freeing the memory of OutBound and InBound params
clean_up:
		Free((void*)out_param_list_entry, MAX_OUT_PARAM);
		Free((void*)in_param_list_entry, MAX_IN_PARAM);
		inParams[0] = NULL;
	}
	END_TRY

	logErrorMessage("Exiting cv_get_ush_ver", ERROR_LOCATION);

	return status;
}

/*
* Function:
*		cv_flash_update
* Purpose:
*		This api will transmit the flash file to ush, which will inturn burn into flash at the specified offet.
*		results are returned in pResults
*
* Parameters:
*		cvHandle	: Previously supplied CV handle
*		flashPath 	: Flash file path
*		offset		: Flash offset for beginning of flash file
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_flash_update (
			   IN	cv_handle cvHandle,
			   IN	char *flashPath,
			   IN   uint32_t offset
)
{
	#define MaxDownloadSize     0x20000
	const short					MAX_OUT_PARAM=1;
	cv_status					status	= CV_SUCCESS;
	cv_param_list_entry			*out_param_list_entry[1] = {NULL};
	cv_handle					cvInternalHandle = 0;
	//cv_xxx Index of the cv_auth_list
	uint32_t					auth_list_idx = 0;
    FILE						*flashFp;
    uint32_t					extrabytes, buflen, bytesleft;
    uint8_t						*pFileData=NULL, *fdPtr, *pBuffer=NULL;
    uint32_t					*pParams;
    int							filelen, idx, numpks;
	chip_spec_hdr_t				chip_spec_hdr;


	chip_spec_hdr.cmd_magic = USBD_COMMAND_MAGIC;
	chip_spec_hdr.cmd = CMD_DOWNLOAD;
	extrabytes = sizeof(chip_spec_hdr_t);

	logErrorMessage("Inside cv_flash_update", ERROR_LOCATION);

	BEGIN_TRY
	{

		//// Validating for Inbound and Outbound parameters
		//if(!is_valid_handle(cvHandle))
		//{
		//	logErrorMessage("Invalid CV Handle", ERROR_LOCATION);
		//	status = CV_INVALID_HANDLE;
		//	goto clean_up;
		//}

		////get the CV internal handle
		//if((cvInternalHandle = GetCVInternalHandle(&cvHandle)) == 0)
		//{
		//	logErrorMessage("Invalid Handle Value", ERROR_LOCATION);
		//	status = CV_INVALID_HANDLE;
		//	goto clean_up;
		//}

        // Validating filename
        if ( flashPath == 0)
        {
            logErrorMessage("Invalid flashPath Value", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // check file
        flashFp = fopen(flashPath, "rb");
        if ( flashFp==NULL )
        {
            logErrorMessage("Can not open flashPath", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // get file length
        fseek(flashFp, 0, SEEK_END);
        filelen = ftell(flashFp);
        //check file length
        if ( filelen<=0 )
        {
            logErrorMessage("flashPath file is empty", ERROR_LOCATION);
			fclose(flashFp);
			return CV_INVALID_INPUT_PARAMETER;
        }
        // go to beginning of file
        fseek(flashFp, 0, SEEK_SET);

        // read in data
        pFileData = (uint8_t*)malloc(filelen);
        if (pFileData==NULL)
        {
            logErrorMessage("Cannot allocate memory for flashPath", ERROR_LOCATION);
			fclose(flashFp);
            return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
        }
        // copy filename data into buffer
        fread(pFileData, sizeof(char), filelen, flashFp);
        fdPtr=pFileData;
		fclose(flashFp);
		
        // find out how many packets to tx
        numpks=filelen/MaxDownloadSize;
        if ( filelen%MaxDownloadSize )
            numpks++;

        // calculate buffer len
        buflen=MaxDownloadSize+extrabytes;
        // allocate memory for file data
        pBuffer = (uint8_t*)malloc(buflen);
        if (pBuffer==NULL)
        {
            logErrorMessage("Cannot allocate memory for tx buffer", ERROR_LOCATION);
            free(pFileData);
			return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
        }

        bytesleft=filelen;
        //logMessage("Writing %s to flash (%d packets; '.'=1 packets)\n", flashPath, numpks);
        for (idx=0; idx<numpks; idx++)
        {
	    uint32_t data_size;
            uint32_t tx_length;
            pParams=(uint32_t*)pBuffer;
	   
            data_size=bytesleft;
            if (bytesleft > MaxDownloadSize)
                data_size = MaxDownloadSize;

  	    if (filelen % MaxDownloadSize)
	    	tx_length = (filelen & ~(MaxDownloadSize - 1)) + MaxDownloadSize;
	    else
	    	tx_length = filelen;

	    chip_spec_hdr.data[0] = idx * MaxDownloadSize; /* offset */	
            chip_spec_hdr.data[1] = MaxDownloadSize; /* size */	
            chip_spec_hdr.data[2] = tx_length; /* total size */	
            memcpy(&pParams[0], &chip_spec_hdr, sizeof(chip_spec_hdr_t));
            // copy filename data into buffer
            memcpy(&pParams[(sizeof(chip_spec_hdr_t)/4)], fdPtr, data_size);

	    bytesleft -= data_size;
	    fdPtr += data_size;

	    //Adding value to Outbound/Inbound parameter list
            if ((status = InitParam_List(CV_ENCAP_LENVAL_PAIR, (MaxDownloadSize + extrabytes), pBuffer, &out_param_list_entry[0])) != CV_SUCCESS)
            {
                logErrorMessage("Error while copying the outbound data into Param Blob", ERROR_LOCATION);
			    goto clean_up;
            }

			//calling cvhManageCVAPI call
			status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry, 0, NULL, NULL, cvInternalHandle, auth_list_idx, CV_CMD_FLASH_PROGRAMMING);
			if(status != CV_SUCCESS)
			{
				logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
				Free((void*)out_param_list_entry, MAX_OUT_PARAM);
				goto clean_up;
			}
			Free((void*)out_param_list_entry, MAX_OUT_PARAM);

            //logMessage(".");
		}
        //logMessage("\n");

		// Freeing the memory of OutBound and InBound params
clean_up:
		if (pFileData!=NULL)
            free(pFileData);
        if (pBuffer!=NULL)
            free(pBuffer);
	}
	END_TRY

	logErrorMessage("Exiting cv_flash_update", ERROR_LOCATION);

	return status;
}


/*
* Function:
*      cv_reboot_to_sbi
*
* Purpose:
*      this command will force the CV to reboot to SBI code
*
* Parameters:
*
* ReturnValue:
*		cv_status
*
*/
CVUSERLIBIFC_API cv_status
cv_reboot_to_sbi(
		 void
		 )
{
	const uint32_t MAX_OUT_PARAM = 1;
	const uint32_t MAX_IN_PARAM = 0;
	cv_status				status = CV_SUCCESS;
	cv_param_list_entry		*out_param_list_entry[1] = {NULL};
	cv_handle				cvHandle = 0;
	uint32_t                rebootParam=0;
	//cv_xxx Index of the cv_auth_list
	uint32_t				auth_list_idx = 0;

	//entry for testing
	logErrorMessage("Inside cv_reboot_to_sbi", ERROR_LOCATION);

	BEGIN_TRY
	{
		//Adding value to Outbound/Inbound parameter list
		if((status = InitParam_List(CV_ENCAP_STRUC, sizeof(rebootParam), &rebootParam, &out_param_list_entry[0])) != CV_SUCCESS)
		{
			logErrorMessage("Error while copying the version parameter into Param Blob", ERROR_LOCATION);
			goto Exit;
		}

		//calling cvhManageCVAPI call
		status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry, MAX_IN_PARAM, NULL, NULL, cvHandle, auth_list_idx, CV_CMD_REBOOT_TO_SBI);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
		}

		Free((void*)out_param_list_entry, MAX_OUT_PARAM);
	}
	END_TRY

Exit:
	//exit for testing
	logErrorMessage("Exiting cv_reboot_to_sbi", ERROR_LOCATION);

	return status;
}


/*
* Function:
*      cv_reboot_to_ush
*
* Purpose:
*      this command will force the CV to reboot to ush
*
* Parameters:
*
* ReturnValue:
*		cv_status
*
*/
CVUSERLIBIFC_API cv_status
cv_reboot_to_ush(
		 void
		 )
{
	const uint32_t MAX_OUT_PARAM = 0;
	const uint32_t MAX_IN_PARAM = 0;
	cv_status				status = CV_SUCCESS;
	cv_handle				cvHandle = 0;
	//cv_xxx Index of the cv_auth_list
	uint32_t				auth_list_idx = 0;

	//entry for testing
	logErrorMessage("Inside cv_reboot_to_ush", ERROR_LOCATION);

	BEGIN_TRY
	{
		//calling cvhManageCVAPI call
		status = cvhManageCVAPICall(MAX_OUT_PARAM, NULL, MAX_IN_PARAM, NULL, NULL, cvHandle, auth_list_idx, CV_CMD_REBOOT_USH);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
		}

	}
	END_TRY

	//exit for testing
	logErrorMessage("Exiting cv_reboot_to_sbi", ERROR_LOCATION);

	return status;
}


/*
* Function:
*		cv_firmware_upgrade_tx
* Purpose:
*		This api will transmit the specified flash data to ush.
*
* Parameters:
*		cvHandle	: Previously supplied CV handle
*		flashPath 	: Flash file path
*		offset		: Flash offset for beginning of flash file
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_firmware_upgrade_tx (
			   IN	uint8_t *pBuffer,
			   IN	uint8_t *pData,
			   IN	uint32_t len,
			   IN   uint32_t offset,
			   IN	cv_command_id cmd
)
{
	const short					MAX_OUT_PARAM=1;
	cv_status					status	= CV_SUCCESS;
	cv_param_list_entry			*out_param_list_entry[1] = {NULL};
	cv_handle					cvInternalHandle = 0;
	uint32_t					auth_list_idx = 0;
    uint32_t					extrabytes=2*sizeof(uint32_t), tmpLen;
    uint32_t					*pParams;

	if (cmd!=CV_CMD_FLASH_PROGRAMMING && cmd!=CV_CMD_FW_UPGRADE_START && cmd!=CV_CMD_FW_UPGRADE_UPDATE && cmd!=CV_CMD_FW_UPGRADE_COMPLETE)
    {
	    logErrorMessage("Error unexpected firmware upgrade command", ERROR_LOCATION);
		return CV_INVALID_COMMAND;
	}

	pParams=(uint32_t*)pBuffer;
	pParams[0]=len;
	pParams[1]=offset;

	// copy filename data into buffer
	memcpy(&pParams[2], pData, len);
	tmpLen = len+extrabytes;

	//Adding value to Outbound/Inbound parameter list
	if ((status = InitParam_List(CV_ENCAP_LENVAL_PAIR, tmpLen, pBuffer, &out_param_list_entry[0])) != CV_SUCCESS)
    {
	    logErrorMessage("Error while copying the outbound data into Param Blob", ERROR_LOCATION);
		return status;
	}

	//calling cvhManageCVAPI call
	status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry, 0, NULL, NULL, cvInternalHandle, auth_list_idx, cmd);
	if(status != CV_SUCCESS)
	{
		logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
	}

	Free((void*)out_param_list_entry, MAX_OUT_PARAM);

	return status;
}


/*
* Function:
*		cv_firmware_upgrade
* Purpose:
*		This api will transmit the flash file to ush at the specified offset.
*		If the offset is 0, then it will update the SBI image (first 64KB) using CV_CMD_FLASH_PROGRAMMING cmd.
*			If the filelength is greater than 64KB, the remaining data will be updated using the
*			CV_CMD_FW_UPGRADE_START(688B), CV_CMD_FW_UPGRADE_UPDATE, CV_CMD_FW_UPGRADE_COMPLETE(last 256B) cmds. 
*		If the offset is 10000, then the file will be updated using the CV_CMD_FW_UPGRADE_START(688B),
*			CV_CMD_FW_UPGRADE_UPDATE and CV_CMD_FW_UPGRADE_COMPLETE(last 256B) cmds. 
*		If the offset is any other value an ERROR is returned.
*		results are returned in pResults
*
* Parameters:
*		cvHandle	: Previously supplied CV handle
*		fwPath 		: Firmware file path
*		offset		: Flash offset for beginning of flash file
*		progress	: Display upgrade progress
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_firmware_upgrade (
			   IN	cv_handle cvHandle,
			   IN	char *fwPath,
			   IN   uint32_t offset,
			   IN   uint8_t progress
)
{
	#define SbiSegSize			0x10000
	#ifdef MaxDownloadSize
	#undef MaxDownloadSize
	#endif
	#define MaxDownloadSize		0xF00
	#define FlashUpgradeStartLen	0x2b0
	#define FlashUpgradeUpdateLen	0xF00
	#define FlashUpgradeCompleteLen	0x100
	cv_status					status	= CV_SUCCESS;
    FILE						*flashFp;
    uint32_t					extrabytes=2*sizeof(uint32_t), buflen, bytesleft, databytesleft, len;
    uint8_t						*pFileData=NULL, *fdPtr, *pBuffer=NULL;
    int							filelen, idx, numpks;


	logErrorMessage("Inside cv_firmware_upgrade", ERROR_LOCATION);

	BEGIN_TRY
	{

        // Validating filename
        if ( fwPath == 0)
        {
            logErrorMessage("Invalid fwPath value", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // check file
        flashFp = fopen(fwPath, "rb");
        if ( flashFp==NULL )
        {
            logErrorMessage("Can not open fwPath", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // Validating offset
        if ( offset != 0 && offset != 0x10000 && offset != 0x20000)
        {
            logErrorMessage("Invalid offset value (must be either 0 or 0x10000 or 0x20000)", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // get file length
        fseek(flashFp, 0, SEEK_END);
        filelen = ftell(flashFp);
        //check file length
        if ( filelen<=0 )
        {
            logErrorMessage("fwPath file is empty", ERROR_LOCATION);
			fclose(flashFp);
			return CV_INVALID_INPUT_PARAMETER;
        }
        // go to beginning of file
        fseek(flashFp, 0, SEEK_SET);

        // read in data
        pFileData = (uint8_t*)malloc(filelen);
        if (pFileData==NULL)
        {
            logErrorMessage("Cannot allocate memory for fwPath", ERROR_LOCATION);
			fclose(flashFp);
            return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
        }

		// copy filename data into buffer
        fread(pFileData, sizeof(char), filelen, flashFp);
        fdPtr=pFileData;
		fclose(flashFp);
		
        // calculate buffer len
        buflen=MaxDownloadSize+extrabytes;
        // allocate memory for file data
        pBuffer = (uint8_t*)malloc(buflen);
        if (pBuffer==NULL)
        {
            logErrorMessage("Cannot allocate memory for tx buffer", ERROR_LOCATION);
            free(pFileData);
			return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
        }

        bytesleft=filelen;

		// check if need to update SBI image.
		if (offset == 0)
		{
	        if ( filelen<SbiSegSize )
		    {
				logErrorMessage("fwPath file SBI segment is not large enough (<0x10000B)", ERROR_LOCATION);
				status = CV_INVALID_INPUT_PARAMETER;
				goto clean_up;
			}
			// find out how many packets to tx
		    numpks=SbiSegSize/MaxDownloadSize;

			bytesleft=SbiSegSize;
			if ( progress )
			{
				//logMessage("Upgrading SBI image:\n");
			}
		    for (idx=0; idx<numpks; idx++)
			{
				len=bytesleft;
			    if (len>MaxDownloadSize)
				    len=MaxDownloadSize;
		
				status = cv_firmware_upgrade_tx( pBuffer, fdPtr, FlashUpgradeStartLen, offset+idx*MaxDownloadSize, CV_CMD_FLASH_PROGRAMMING);
				if(status != CV_SUCCESS)
				{
					logErrorMessage("Error transmitting SBI data", ERROR_LOCATION);
					goto clean_up;
				}

				// calculate number of bytes left
				bytesleft-=len;
				// advance filedata pointer 
				fdPtr+=len;

				if ( progress )
				{
					if ( !(idx%5) )
					{
						//logMessage(".");
					}
				}
			}

			bytesleft=filelen-SbiSegSize;
			if ( progress )
			{
				//logMessage("\n");
			}
		}

		// now check for the ush image
		if ( bytesleft < (FlashUpgradeStartLen+FlashUpgradeCompleteLen) )
		{
			logErrorMessage("fwPath file USH segment is not large enough (<0x3b0)", ERROR_LOCATION);
			goto clean_up;
		}

		// transmit upgrade start cmd
		status = cv_firmware_upgrade_tx( pBuffer, fdPtr, FlashUpgradeStartLen, 0, CV_CMD_FW_UPGRADE_START);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error transmitting Firmware Upgrade Start", ERROR_LOCATION);
			goto clean_up;
		}
		
		// update data pointer
		fdPtr+=FlashUpgradeStartLen;
		bytesleft-=FlashUpgradeStartLen;

		// find out how many packets to tx
		databytesleft=bytesleft-FlashUpgradeCompleteLen;
        numpks=databytesleft/FlashUpgradeUpdateLen;
        if ( databytesleft%FlashUpgradeUpdateLen )
            numpks++;

		if ( progress )
		{
			//logMessage("Upgrading USH image:\n");
		}
		for (idx=0; idx<numpks; idx++)
        {
			len = databytesleft;
		    if ( len>FlashUpgradeUpdateLen )
			    len=FlashUpgradeUpdateLen;

			status = cv_firmware_upgrade_tx( pBuffer, fdPtr, len, 0, CV_CMD_FW_UPGRADE_UPDATE);
			if(status != CV_SUCCESS)
			{
				logErrorMessage("Error transmitting Firmware Upgrade Update", ERROR_LOCATION);
				break;
			}
		
			// update dataptr and len
			fdPtr+=len;
			databytesleft-=len;
			if ( progress )
			{
				if ( !(idx%5) )
				{
					//logMessage(".");
				}
			}
		}
		if ( progress )
		{
			//logMessage("\n");
		}

		// transmit upgrade complete cmd
		status = cv_firmware_upgrade_tx( pBuffer, fdPtr, FlashUpgradeCompleteLen, 0, CV_CMD_FW_UPGRADE_COMPLETE);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error transmitting Firmware Upgrade Complete", ERROR_LOCATION);
		}
		
		// Freeing the memory of OutBound and InBound params
clean_up:
		if (pFileData!=NULL)
            free(pFileData);
        if (pBuffer!=NULL)
            free(pBuffer);
	}
	END_TRY

	logErrorMessage("Exiting cv_firmware_upgrade", ERROR_LOCATION);

	return status;
}


/*
* Function:
*		cv_load_sbi
* Purpose:
*		This api will transmit the sbi file to ush, which will inturn burn into RAM and then execute it.
*		results are returned in pResults
*
* Parameters:
*		sbiPath 	: Flash file path
*
* ReturnValue:
*		cv_status
*/

CVUSERLIBIFC_API cv_status
cv_load_sbi (
			   IN	char *sbiPath
)
{
	const short					MAX_OUT_PARAM=1;
	cv_status					status	= CV_SUCCESS;
	cv_param_list_entry			*out_param_list_entry[1] = {NULL};
	cv_handle					cvHandle = 0;
	//cv_xxx Index of the cv_auth_list
	uint32_t					auth_list_idx = 0;
    FILE						*sbiFp;
    int							filelen;
    uint8_t						*pFileData=NULL;
    

	logErrorMessage("Inside cv_load_sbi", ERROR_LOCATION);

	BEGIN_TRY
	{

        // Validating filename
        if ( sbiPath == 0)
        {
            logErrorMessage("Invalid sbiPath Value", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // check file
        sbiFp = fopen(sbiPath, "rb");
        if ( sbiFp==NULL )
        {
            logErrorMessage("Can not open sbiPath", ERROR_LOCATION);
			return CV_INVALID_INPUT_PARAMETER;
        }

        // get file length
        fseek(sbiFp, 0, SEEK_END);
        filelen = ftell(sbiFp);
        //check file length
        if ( filelen<=0 )
        {
            logErrorMessage("sbiPath file is empty", ERROR_LOCATION);
			fclose(sbiFp);
			return CV_INVALID_INPUT_PARAMETER;
        }
        // go to beginning of file
        fseek(sbiFp, 0, SEEK_SET);

        // read in data
        pFileData = (uint8_t*)malloc(filelen);
        if (pFileData==NULL)
        {
            logErrorMessage("Cannot allocate memory for sbiPath", ERROR_LOCATION);
			fclose(sbiFp);
            return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
        }
        // copy filename data into buffer
        fread(pFileData, sizeof(char), filelen, sbiFp);
        fclose(sbiFp);
		
        //logMessage("Writing %s\n", sbiPath);
		//Adding value to Outbound/Inbound parameter list
        if ((status = InitParam_List(CV_ENCAP_LENVAL_PAIR, filelen, pFileData, &out_param_list_entry[0])) != CV_SUCCESS)
        {
            logErrorMessage("Error while copying the SBI data into Param Blob", ERROR_LOCATION);
		    goto clean_up;
        }

        //calling cvhManageCVAPI call
		status = cvhManageCVAPICall(MAX_OUT_PARAM, out_param_list_entry, 0, NULL, NULL, cvHandle, auth_list_idx, CV_CMD_LOAD_SBI);
		if(status != CV_SUCCESS)
		{
			logErrorMessage("Error returned by cvhManageCVAPIcall", ERROR_LOCATION);
			goto clean_up;
		}

		// Freeing the memory of OutBound and InBound params
clean_up:
		Free((void*)out_param_list_entry, MAX_OUT_PARAM);
		if (pFileData!=NULL)
            free(pFileData);
	}
	END_TRY

	logErrorMessage("Exiting cv_load_sbi", ERROR_LOCATION);

	return status;
}

