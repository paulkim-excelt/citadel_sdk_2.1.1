/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/
/*
 * Broadcom Corporation Credential Vault
 */
/*
 * cvhandler.c:  CVAPI Handler support routines
 */
/*
 * Revision History:
 *
 * 02/18/07 DPA Created.
*/
#include "cvmain.h"

cv_status
cvWaitInsertion(cv_callback *callback, cv_callback_ctx context, cv_status prompt, uint32_t userPromptTimeout)
{
	/* this routine invokes the callback to cause a user prompt to be displayed */
	/* indicating a smart card insertion or contactless swipe is required.  a loop */
	/* is set up to check for 1 of 2 things to occur: insertion/swipe or user cancellation */
	uint32_t startTime, lastCallback;
	cv_status retVal = CV_SUCCESS;
	uint32_t localPrompt = (prompt == CV_READ_HID_CREDENTIAL) ? CV_PROMPT_FOR_CONTACTLESS : prompt;
	cv_bool promptDisplayed = FALSE;

	/* set up loop to send prompt and check for insertion/swipe */
	get_ms_ticks(&startTime);
	lastCallback = startTime;
	/* set up bulk out.  this is because the cvActualCallback routine expects this to be set the first time */
	/* it's called for prompting (based on FP capture yield processing) */
	usbCvBulkOut((cv_encap_command*)CV_SECURE_IO_COMMAND_BUF);
	do
	{
		/* first check if contactless requested and present */
		if ((localPrompt == CV_PROMPT_FOR_CONTACTLESS) && (rfIsCardPresent()))
		{
			retVal = CV_SUCCESS;
			/* send callback to give audible feedback */
			retVal = (*callback)(CV_PROMPT_CONTACTLESS_DETECTED, 0, NULL, context);

			/* now send callback to remove prompt */
			if (promptDisplayed)
				retVal = (*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
			break;
		}
		/* now call callback, if time to */
		if(cvGetDeltaTime(lastCallback) >= USER_PROMPT_CALLBACK_INTERVAL)
		{
			if ((retVal = (*callback)(localPrompt, 0, NULL, context)) != CV_SUCCESS)
				break;
			promptDisplayed = TRUE;
			get_ms_ticks(&lastCallback);
		}
		if ((localPrompt == CV_PROMPT_FOR_SMART_CARD) && (scIsCardPresent()))
		{
			retVal = CV_SUCCESS;
			/* now send callback to remove prompt */
			if (promptDisplayed)
				retVal = (*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
			/* now delay briefly to allow card to be completely inserted */
			get_ms_ticks(&lastCallback);
			do
			{
				if(cvGetDeltaTime(lastCallback) >= SMART_CARD_INSERTION_DELAY)
					break;
			} while (TRUE);
			break;
		}
		if(cvGetDeltaTime(startTime) >= userPromptTimeout)
		{
			/* now send callback to remove prompt */
			if (promptDisplayed)
				(void)(*callback)(CV_REMOVE_PROMPT, 0, NULL, context);
			retVal = CV_CALLBACK_TIMEOUT;
			goto err_exit;
		}
		yield();
	} while (TRUE);

err_exit:
	return retVal;
}

cv_status
cvHandlerPromptOrPin(cv_status status, cv_obj_properties *objProperties,
					 cv_callback *callback, cv_callback_ctx context)
{
	/* this routine determines if a prompt or PIN entry is required. */
	cv_status retVal = CV_SUCCESS;

	switch (status)
	{
	case CV_PROMPT_FOR_SMART_CARD:
	case CV_PROMPT_FOR_CONTACTLESS:
		/* wait for insertion/swipe */
		if ((retVal = cvWaitInsertion(callback, context, retVal, USER_PROMPT_TIMEOUT)) != CV_SUCCESS)
			goto err_exit;
		break;

	case CV_PROMPT_PIN:
	case CV_PROMPT_PIN_AND_SMART_CARD:
	case CV_PROMPT_PIN_AND_CONTACTLESS:
		/* a PIN is required.  first check to see if one was supplied in the auth list */
		if (objProperties->PINLen == 0)
		{
			/* PIN not found.  check to see if this was already requested once */
			if (CV_VOLATILE_DATA->CVInternalState & CV_WAITING_FOR_PIN_ENTRY)
			{
				/* yes, this means user has cancelled command */
				retVal = CV_CANCELLED_BY_USER;
				CV_VOLATILE_DATA->CVInternalState &= ~CV_WAITING_FOR_PIN_ENTRY;
				goto err_exit;
			}
			/* here if first time.  set internal state and exit */
			if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
				CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
			retVal = status;
			goto err_exit;
		}
		/* here if PIN supplied.  determine if need prompt */
		if (status != CV_PROMPT_PIN)
		{
			/* yes, do insertion/swipe prompt */
			retVal = (status == CV_PROMPT_PIN_AND_SMART_CARD) ? CV_PROMPT_FOR_SMART_CARD : CV_PROMPT_FOR_CONTACTLESS;
			if ((retVal = cvWaitInsertion(callback, context, retVal, USER_PROMPT_TIMEOUT)) != CV_SUCCESS)
				goto err_exit;
		}
		retVal = CV_SUCCESS;
		break;

	default:
		retVal = status;
		goto err_exit;
	case 0xffff:
		break;
	}
err_exit:
	return retVal;
}

cv_status
cvHandlerDetermineObjAvail(cv_obj_properties *objProperties, cv_callback *callback, cv_callback_ctx context)
{
	/* this is a CV API handler support routine to determine object availability */
	cv_status retVal = CV_SUCCESS;

	if ((retVal = cvDetermineObjAvail(objProperties))!= CV_SUCCESS)
		retVal = cvHandlerPromptOrPin(retVal, objProperties, callback, context);

	return retVal;
}

cv_status
cvHandlerDetermineAuthAvail(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties,
							uint32_t authListLength, cv_auth_list *pAuthList, cv_callback *callback, cv_callback_ctx context)
{
	/* this is a CV API handler support routine to determine the availability of objects for doing auth */
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_status statusList[MAX_CV_AUTH_PROMPTS];
	uint32_t statusCount = MAX_CV_AUTH_PROMPTS;
	uint32_t i;
	cv_status promptStatus = CV_SUCCESS;
	uint32_t LRUObj, objLen;

	if ((retVal = cvDoAuth(cvAdminAuthPermission, objProperties, authListLength, pAuthList, &authFlags, NULL, NULL, 0, TRUE,
		 &statusCount, &statusList[0])) != CV_SUCCESS)
		 goto err_exit;

	/* now parse status list and determine actions */
	for (i=0;i<statusCount;i++)
	{
		switch (statusList[i])
		{
		case CV_PROMPT_FOR_SMART_CARD:
		case CV_PROMPT_FOR_CONTACTLESS:
		case CV_READ_HID_CREDENTIAL:
			/* check to see if already have prompt, but different kind */
			if (promptStatus == CV_SUCCESS)
				/* no prompt yet, save this one */
				promptStatus = statusList[i];
			else
				/* there is a prompt already, same one? */
				if (promptStatus != statusList[i])
				{
					/* no, this scenario not handled, just exit */
					retVal = CV_INVALID_AUTH_LIST;
					goto err_exit;
				}
			break;

		case CV_PROMPT_PIN:
		case CV_PROMPT_PIN_AND_SMART_CARD:
		case CV_PROMPT_PIN_AND_CONTACTLESS:
			/* a PIN is required.  first check to see if one was supplied in the auth list */
			if (cvFindPINAuthEntry(pAuthList, objProperties) == NULL)
			{
				/* PIN not found.  check to see if this was already requested once */
				if (CV_VOLATILE_DATA->CVInternalState & CV_WAITING_FOR_PIN_ENTRY)
				{
					/* yes, this means user has cancelled command */
					retVal = CV_CANCELLED_BY_USER;
					CV_VOLATILE_DATA->CVInternalState &= ~CV_WAITING_FOR_PIN_ENTRY;
					goto err_exit;
				}
				/* here if first time.  set internal state and exit */
				if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
					CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
				goto err_exit;
			}
			/* here if PIN supplied.  determine if need prompt */
			if (statusList[i] != CV_PROMPT_PIN)
			{
				/* need insertion prompt */
				retVal = (statusList[i] == CV_PROMPT_PIN_AND_SMART_CARD) ? CV_PROMPT_FOR_SMART_CARD : CV_PROMPT_FOR_CONTACTLESS;
				/* check to see if already have prompt status */
				if (promptStatus == CV_SUCCESS)
					/* no prompt yet, save this one */
					promptStatus = retVal;
				else
					/* already have prompt, is it same one? */
					if (promptStatus != statusList[i])
					{
						/* no, this scenario not handled, just exit */
						retVal = CV_INVALID_AUTH_LIST;
						goto err_exit;
					}
			}
			retVal = CV_SUCCESS;
			break;

		case 0xffff:
		default:
			/* shouldn't get anything but prompt statuses here */
			retVal = statusList[i];
			goto err_exit;
		}
	}
	/* if found prompt status, wait for insertion/swipe */
	if (promptStatus != CV_SUCCESS)
		if ((retVal = cvWaitInsertion(callback, context, promptStatus, USER_PROMPT_TIMEOUT)) != CV_SUCCESS)
			goto err_exit;

	/* now check to see if contactless credential needs to be read in */
	if (promptStatus == CV_READ_HID_CREDENTIAL)
	{
		/* yes, get object cache entry and read in contactless credential */
		cvObjCacheGetLRU(&LRUObj, &CV_VOLATILE_DATA->HIDCredentialPtr);
		objLen = MAX_CV_OBJ_SIZE;
		/* get PIN if there is one */
		cvFindPINAuthEntry(pAuthList, objProperties);
		retVal = cvReadContactlessID(CV_POST_PROCESS_SHA256, objProperties->PINLen, objProperties->PIN, &CV_VOLATILE_DATA->credentialType, &objLen, CV_VOLATILE_DATA->HIDCredentialPtr);
		/* check to see if PIN required but not supplied */
		if (retVal == CV_PROMPT_PIN_AND_CONTACTLESS)  /* used to be CV_PROMPT_PIN */
		{
			/* PIN not found.  check to see if this was already requested once */
			if (CV_VOLATILE_DATA->CVInternalState & CV_WAITING_FOR_PIN_ENTRY)
			{
				/* yes, this means user has cancelled command */
				retVal = CV_CANCELLED_BY_USER;
				CV_VOLATILE_DATA->CVInternalState &= ~CV_WAITING_FOR_PIN_ENTRY;
				goto err_exit;
			}
			/* here if first time.  set internal state and exit */
			if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
				CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
			goto err_exit;
		} else if (retVal != CV_SUCCESS) {
			CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
			goto err_exit;
		}
	}

err_exit:
	return retVal;
}

cv_status
cvHandlerObjAuth(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties,
				 uint32_t authListLength, cv_auth_list *pAuthList, cv_input_parameters *inputParameters, cv_obj_hdr **objPtr,
				 cv_obj_auth_flags *authFlags, cv_bool *needObjWrite, cv_callback *callback, cv_callback_ctx context)
{
	/* this routine does object auth associated with API handler */
	cv_status retVal = CV_SUCCESS;

	/* determine if object is available */
	if ((retVal = cvHandlerDetermineObjAvail(objProperties, callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* object is available, get it */
	if ((retVal = cvGetObj(objProperties, objPtr)) != CV_SUCCESS)
		goto err_exit;

	/* now check see if this is the 2nd time calling this API, if 1st time exited waiting for PIN entry to */
	/* write object.  if so, just try to write object and exit */
	if ((CV_VOLATILE_DATA->CVInternalState & CV_WAITING_FOR_PIN_ENTRY) && (CV_VOLATILE_DATA->CVInternalState &
		(CV_PENDING_WRITE_TO_CONTACTLESS | CV_PENDING_WRITE_TO_SC)))
	{
		/* yes, attempt object write again */
		if ((retVal = cvPutObj(objProperties, *objPtr)) == CV_SUCCESS)
			/* succeeded, clear states */
			CV_VOLATILE_DATA->CVInternalState &=
				~(CV_WAITING_FOR_PIN_ENTRY | CV_PENDING_WRITE_TO_SC | CV_PENDING_WRITE_TO_CONTACTLESS);
		goto err_exit;
	}

	/* now make sure all the objects necessary for doing auth are available */
	if ((retVal = cvHandlerDetermineAuthAvail(cvAdminAuthPermission, objProperties, authListLength, pAuthList,
		callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* now should be able to do auth */
	retVal = cvDoAuth(cvAdminAuthPermission, objProperties, authListLength, pAuthList, authFlags, inputParameters,
		callback, context, FALSE,
		 NULL, NULL);
	/* check to see if prompting for PIN.  if null PIN entry sent more than once or after a failed PIN, consider */
	/* this a cancel case */
	if (retVal == CV_PROMPT_PIN)
	{
		/* are we waiting for a PIN entry already? */
		if (!(CV_VOLATILE_DATA->CVInternalState & CV_WAITING_FOR_PIN_ENTRY))
		{
			/* no, flag this state and return */
			if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
				CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
		} else {
			/* yes, check to see if NULL PIN sent */
			if (objProperties->PINLen == 0)
			{
				/* yes, this means user has cancelled command */
				retVal = CV_CANCELLED_BY_USER;
				CV_VOLATILE_DATA->CVInternalState &= ~CV_WAITING_FOR_PIN_ENTRY;
				goto err_exit;
			}
		}
	}

	/* now check to see if auth succeeded */
	if (retVal != CV_SUCCESS && retVal != CV_OBJECT_UPDATE_REQUIRED)
	{
		/* no, fail */
		goto err_exit;
	}
	if (retVal == CV_OBJECT_UPDATE_REQUIRED)
		*needObjWrite = TRUE;
	retVal = CV_SUCCESS;

err_exit:
	return retVal;
}


cv_status
cvHandlerPostObjSave(cv_obj_properties *objProperties, cv_obj_hdr *objPtr,
					 cv_callback *callback, cv_callback_ctx context)
{
	/* this routine handles writing an object back if auth type has caused it to be updated */
	cv_status retVal;

	if ((retVal = cvPutObj(objProperties, objPtr)) == CV_SUCCESS)
		goto success_exit;
	/* handle any prompting necessary */
	if ((retVal = cvHandlerPromptOrPin(retVal, objProperties, callback, context)) == CV_SUCCESS)
	{
		/* prompt was successful, try to write object again */
		if ((retVal = cvPutObj(objProperties, objPtr)) != CV_SUCCESS)
			goto err_exit;
		/* write succeeded, clear any prompt states */
		CV_VOLATILE_DATA->CVInternalState &= ~(CV_PENDING_WRITE_TO_SC | CV_PENDING_WRITE_TO_CONTACTLESS);
	}

err_exit:
success_exit:
	return retVal;
}

