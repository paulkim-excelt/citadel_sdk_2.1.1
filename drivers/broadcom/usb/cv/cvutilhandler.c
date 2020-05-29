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
 * cvutilhandler.c:  CVAPI Utility Handler
 */
/*
 * Revision History:
 *
 * 02/12/07 DPA Created.
*/
#include "cvmain.h"
#include "fmalloc.h"
#include "console.h"
#ifdef CV_TEST_TPM
#include "tcpa_user.h"
#include "tcpa_ush_remap.h"
#endif
#ifdef PHX_POA_BASIC
#include "fp_spi_enum.h"
#include "../pwr/poa.h"
#endif

cv_status
cv_open (cv_options options, cv_app_id *appID, cv_user_id *userID,
			cv_obj_value_passphrase *objAuth, cv_handle *pCvHandle)
{
#define DEBUG_CVOPEN 1
#ifdef DEBUG_CVOPEN 
        printf("%s: enter\n",__func__);
#endif
	/* This routine allocates a session structure in scratch RAM, fills in the indicated */
	/* information and returns the session address as the CV handle.  A hash of the (appID || userID) */
	/* is made to store in the session.  Also, if the CV policy in the persistent data indicates privacy */
	/* keys are permitted and one has been supplied as a parameter, a PRF (TBD which one) is used to */
	/* create the privacy key from the objAuth parameter.  The secure session information is filled in */
	/* by the CV Manager upon return */
	cv_session *session;
	cv_status retVal = CV_SUCCESS;
#if 0
/* temporarily don't allow privacy key */
	uint8_t key[AES_128_BLOCK_LEN];
#endif
	uint8_t hash[MAX_CV_OBJ_SIZE];

	/* validate parameters */
	if (options & ~CV_OPTIONS_MASK)
	{
		retVal = CV_INVALID_INPUT_PARAMETER;
#ifdef DEBUG_CVOPEN 
        printf("%s: invalid input\n",__func__);
#endif
		goto err_exit;
	}

	/* first allocate memory for session.  if secure session set up, memory has already been allocated and pointer */
	/* was stored in volatile memory by cvManager */
	if (options & CV_SECURE_SESSION)
	{
#ifdef DEBUG_CVOPEN 
        printf("%s: secure session\n",__func__);
#endif
		if (CV_VOLATILE_DATA->secureSession == NULL)
		{
			retVal = CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
#ifdef DEBUG_CVOPEN 
        printf("%s: VOLATILE DATA is NULL\n",__func__);
#endif
			goto err_exit;
		}
		session = CV_VOLATILE_DATA->secureSession;
		CV_VOLATILE_DATA->secureSession = NULL;
	} else {
		if ((session = (cv_session *)cv_malloc(sizeof(cv_session))) == NULL)
		{
			retVal = CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
#ifdef DEBUG_CVOPEN 
        printf("%s: session malloc fail\n",__func__);
#endif
			goto err_exit;
		}
		memset(session,0,sizeof(cv_session));
	}

	/* save session flags */
	session->flags = options;

	/* set magic number to identify session */
	memcpy((uint8_t *)&session->magicNumber,"SeSs", sizeof(session->magicNumber));

	/* now hash the appID and userID.  if both are null, set hash to all zeros */
	if (appID->appIDLen == 0 && userID->userIDLen == 0)
		memset(session->appUserID,0,SHA1_LEN);
	else
	{
		/* change this to use single stage hash, because multi-stage hash requires update stages to be min of 64 bytes */
		if ((appID->appIDLen + userID->userIDLen) > sizeof(hash))
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		memcpy(&hash[0], appID->appID, appID->appIDLen);
		memcpy(&hash[appID->appIDLen], userID->userID, userID->userIDLen);
		if ((retVal = cvAuth(&hash[0], appID->appIDLen + userID->userIDLen, NULL, session->appUserID, SHA1_LEN, NULL, 0, 0))!= CV_SUCCESS)
			goto err_exit;
	}

	/* check to see if an object auth password is provided for use as a privacy key.  if so, check to see */
	/* if policy allows this */
/* temporarily don't allow privacy key */
#if 0
	if (objAuth->passphraseLen != 0)
	{
		/* check policy */
		if (!(CV_PERSISTENT_DATA->cvPolicy & CV_ALLOW_SESSION_AUTH))
		{
			retVal = CV_POLICY_DISALLOWS_SESSION_AUTH;
			goto err_exit;
		}
		/* now compute key */
		/* compute key for PRF by hashing passphrase */
		if ((retVal = cvAuth(objAuth->passphrase, objAuth->passphraseLen, NULL, key, SHA1_LEN, NULL, 0, 0)) != CV_SUCCESS)
			goto err_exit;
		/* now compute privacy key */
		if ((retVal = ctkipPrfAes(key, objAuth->passphrase, objAuth->passphraseLen, AES_128_BLOCK_LEN, session->privacyKey)) != CV_SUCCESS)
			goto err_exit;
		session->flags |= CV_HAS_PRIVACY_KEY;
	}
#endif
	*pCvHandle = (cv_handle)session;

err_exit:
	return retVal;
}

cv_status
cv_close (cv_handle cvHandle)
{
	/* this routine closes the indicated session and removes any volatile objects associated with it */
	uint32_t volDirIndex;
	cv_obj_hdr *objPtr;
	cv_session *session = (cv_session *)cvHandle;

	/* make sure this is a valid session handle.  if not, just return success, since no session to close */
	if (cvValidateSessionHandle(cvHandle) != CV_SUCCESS)
		return CV_SUCCESS;

	/* first search volatile object directory for objects created by this session (unless flag indicates to retain) */
	if (!(session->flags & CV_RETAIN_VOLATILE_OBJS))
	{
		for (volDirIndex=0;volDirIndex<MAX_CV_NUM_VOLATILE_OBJS;volDirIndex++)
		{
			if (CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hSession == (cv_session *)cvHandle)
			{
				/* if object exists... free memory for this volatile object and free entry in directory */
				if(CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hObj) {
					objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj;
					/* don't delete volatile auth session objects.  these need to persist across sessions */
					if (objPtr->objType != CV_TYPE_AUTH_SESSION)
					{
						cv_free(CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj, objPtr->objLen);
						CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hObj = 0;
					}
				}
			}
		}
	}

	/* now free session memory */
	/* check to make sure this is a valid address.  if not, just fail silently */
	if (session >= (cv_session *)&CV_HEAP[0] && session < (cv_session *)&CV_HEAP[CV_VOLATILE_OBJECT_HEAP_LENGTH])
	{
		/* first clear magic number */
		memset((uint8_t *)&session->magicNumber,0,sizeof(session->magicNumber));
		cv_free(session, sizeof(cv_session));
	} else {
		printf("cv_close: invalid session handle 0x%x\n",cvHandle);
	}
	return CV_SUCCESS;
}

void
cvSetConfigDefaults(void)
{
	/* this routine sets the defaults for all config params */
	/* Default shared secret... HostControlService should use the same value unless using secure session, */
	/* in which the shared secret is re-negotiated */
	uint8_t defaultSharedSecret[SHA1_LEN] = { 0x6D, 0x16, 0x30, 0x95, 0x7A, 0x00, 0x78, 0x3D, 0x26, 0xD6,
											  0x30, 0xF0, 0xC9, 0xAC, 0x2D, 0x38, 0x0A, 0x33, 0xA8, 0x37 };

	CV_PERSISTENT_DATA->DAAuthFailureThreshold = DEFAULT_CV_DA_AUTH_FAILURE_THRESHOLD;
	CV_PERSISTENT_DATA->DAInitialLockoutPeriod = DEFAULT_CV_DA_INITIAL_LOCKOUT_PERIOD;
	CV_PERSISTENT_DATA->cvPolicy = 0;
	CV_PERSISTENT_DATA->persistentFlags &= ~CV_RTC_TIME_CONFIGURED;
	CV_PERSISTENT_DATA->persistentFlags &= ~(CV_USE_KDIX_DSA | CV_USE_KDIX_ECC);
	CV_PERSISTENT_DATA->persistentFlags &= ~CV_ANTI_REPLAY_OVERRIDE;
	CV_PERSISTENT_DATA->persistentFlags |= CV_DA_THRESHOLD_DEFAULT;  /* indicate default has been set */
	CV_PERSISTENT_DATA->identifyUserTimeout = CV_DEFAULT_IDENTIFY_USER_TIMEOUT;
	CV_PERSISTENT_DATA->fpPollingInterval = FP_DEFAULT_SW_FDET_POLLING_INTERVAL;
	CV_PERSISTENT_DATA->FARval = FP_DEFAULT_FAR;
	CV_PERSISTENT_DATA->fpPersistence = FP_DEFAULT_PERSISTENCE;
	CV_PERSISTENT_DATA->fpAntiReplayHMACType = CV_SHA256;
	CV_PERSISTENT_DATA->persistentFlags &= ~CV_HAS_SUPER_PARAMS;
	CV_PERSISTENT_DATA->persistentFlags &= ~CV_HAS_SECURE_SESS_ECC_KEY;

	/* Set default shared secret */
	memcpy((uint8_t *)&CV_PERSISTENT_DATA->secureAppSharedSecret[0], &defaultSharedSecret[0], SHA1_LEN);
}

/* 
 Purpose: Check the POA capacity
 Return : CV_CAPABILITY_POA_BASIC : NEXT FP sensor and EC support POA
          0                       : no POA support
 */
 

uint32_t CV_GetCapabilities()
{
#ifndef PHX_POA_BASIC
    return CV_CAPABILITY_POA_BASIC;
#else
    uint8_t cmd = POA_CMD_MASK | POA_CMD_GET_STATUS; 
    uint8_t status = 0xff;
	uint32_t retVal = 0;

    /* check if EC support POA */
    ush_poa_send_data (&cmd, 1, 1, &status);

    /* EC doesn't support POA */
    if (status == 0xff ) 
    {
        return retVal;
    }

    /* Only NEXT sensor support POA @05/19/2016 */
    if ( (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_SENSOR_FOUND)  &&   /* detected fp sensor */
         (VOLATILE_MEM_PTR->fp_spi_flags & (FP_SPI_NEXT_TOUCH | FP_SPI_DP_TOUCH | FP_SPI_SYNAPTICS_TOUCH))       /* NEXT or DP sensor  */
       )
    {
    	  retVal |= CV_CAPABILITY_POA_BASIC;
		  if(POA_IsPOAEnabled()== POA_MODE_ENABLED && ush_get_fp_template_count() > 0)
		  {
			  retVal |= CV_CAPABILITY_POA_OPERATIONAL;
		  }
    }
    else  
    {
        printf("CV_GetCapabilities: no valid fingerprint sensor detected.\n");
    }
	return retVal;
#endif
}

cv_status
cv_get_status (cv_handle cvHandle, cv_status_type statusType, void *pStatus)
{
	/* this routine gets the indicated status information */
	cv_version version;
	cv_time time;
	cv_operating_status operatingStatus;
	cv_status retVal = CV_SUCCESS;
	cv_da_params daParams;
#ifdef ST_AUTH
	cv_st_challenge stChallenge;
#endif

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	switch (statusType)
	{
	case CV_STATUS_VERSION:
		cvGetVersion(&version);
		memcpy(pStatus, &version, sizeof(cv_version));
		break;

	case CV_STATUS_OP:
		if (CV_VOLATILE_DATA->PHX2DeviceStatus & CV_PHX2_OPERATIONAL)
			operatingStatus = CV_OK;
		else if (CV_VOLATILE_DATA->PHX2DeviceStatus & CV_PHX2_MONOTONIC_CTR_FAIL)
			operatingStatus = CV_MONOTONIC_COUNTER_FAILURE;
		else if (CV_VOLATILE_DATA->PHX2DeviceStatus & CV_PHX2_FLASH_INIT_FAIL)
			operatingStatus = CV_FLASH_INIT_FAILURE;
		else if (CV_VOLATILE_DATA->PHX2DeviceStatus & CV_PHX2_FP_DEVICE_FAIL)
			operatingStatus = CV_FP_DEVICE_FAILURE;
		else
			operatingStatus = CV_DEVICE_FAILURE;
		memcpy(pStatus, &operatingStatus, sizeof(cv_operating_status));
		break;

	case CV_STATUS_SPACE:
		if (!(CV_VOLATILE_DATA->PHX2DeviceStatus & CV_PHX2_OPERATIONAL))
		{
			retVal = CV_INTERNAL_DEVICE_FAILURE;
			goto err_exit;
		}
		CV_VOLATILE_DATA->space.remainingNonvolatileSpace = phx_fheap_remspace();
		memcpy(pStatus, &CV_VOLATILE_DATA->space, sizeof(cv_space));
		cvPrintOpenSessions();
		break;

	case CV_STATUS_TIME:
		if ((retVal = get_time(&time)) != CV_SUCCESS)
			goto err_exit;
		memcpy(pStatus, &time, sizeof(cv_time));
		break;
	case CV_STATUS_FP_POLLING_INTERVAL:
		memcpy(pStatus, &CV_PERSISTENT_DATA->fpPollingInterval, sizeof(uint32_t));
		break;
	case CV_STATUS_FP_PERSISTANCE:
		memcpy(pStatus, &CV_PERSISTENT_DATA->fpPersistence, sizeof(uint32_t));
		break;
	case CV_STATUS_FP_HMAC_TYPE:
		memcpy(pStatus, &CV_PERSISTENT_DATA->fpAntiReplayHMACType, sizeof(cv_hash_type));
		break;
	case CV_STATUS_FP_FAR:
		memcpy(pStatus, &CV_PERSISTENT_DATA->FARval, sizeof(uint32_t));
		break;
	case CV_STATUS_DA_PARAMS:
		daParams.lockoutDuration = CV_PERSISTENT_DATA->DAInitialLockoutPeriod / 1000;   /* convert ms to sec */
		daParams.maxFailedAttempts = CV_PERSISTENT_DATA->DAAuthFailureThreshold;
		memcpy(pStatus, &daParams, sizeof(cv_da_params));
		break;
	case CV_STATUS_POLICY:
		memcpy(pStatus, &CV_PERSISTENT_DATA->cvPolicy, sizeof(cv_policy));
		break;
	case CV_STATUS_HAS_CV_ADMIN:
		*((cv_bool *)pStatus) = (CV_PERSISTENT_DATA->persistentFlags & CV_HAS_CV_ADMIN) ? TRUE : FALSE;
		break;
	case CV_STATUS_IDENTIFY_USER_TIMEOUT:
		memcpy(pStatus, &CV_PERSISTENT_DATA->identifyUserTimeout, sizeof(uint32_t));
		break;
	case CV_STATUS_ERROR_CODE_MORE_INFO:
		memcpy(pStatus, &CV_VOLATILE_DATA->moreInfoErrorCode, sizeof(uint32_t));
		break;
	case CV_STATUS_SUPER_CHALLENGE:
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_SUPER_PARAMS))
		{
			retVal = CV_SUPER_AUTH_TYPE_NOT_CONFIGURED;
			goto err_exit;
		}
		break;
	case CV_STATUS_IN_DA_LOCKOUT:
		if (CV_VOLATILE_DATA->CVInternalState & CV_IN_DA_LOCKOUT)
			*((uint32_t *)pStatus) = TRUE;
		else
			*((uint32_t *)pStatus) = FALSE;
		break;

	case CV_STATUS_DEVICES_ENABLED:
		*((uint32_t *)pStatus) = CV_PERSISTENT_DATA->deviceEnables;
		/* now get volatile version of CV-only radio mode */
		printf("CV: cv_get_status: __is_cv %d\n", __IS_CV_ONLY_RADIO_MODE_CV());
		if (__IS_CV_ONLY_RADIO_MODE_CV())
			*((uint32_t *)pStatus) |= CV_CV_ONLY_RADIO_MODE;
		else
			*((uint32_t *)pStatus) &= ~CV_CV_ONLY_RADIO_MODE;
		break;

	case CV_STATUS_ANTI_REPLAY_OVERRIDE_OCCURRED:
		if (CV_PERSISTENT_DATA->persistentFlags & CV_ANTI_REPLAY_OVERRIDE)
			*((uint32_t *)pStatus) = TRUE;
		else
			*((uint32_t *)pStatus) = FALSE;
		break;

	case CV_STATUS_RFID_PARAMS_ID:
		*((uint32_t *)pStatus) = cvGetRFIDParamsID();
		break;
#ifdef ST_AUTH
	case CV_STATUS_ST_CHALLENGE:
		/* get challenge row number (1-256) and nonce */
		/* row number */
		if ((retVal = cvRandom(sizeof(uint32_t), (uint8_t *)&stChallenge.rowNumber)) != CV_SUCCESS)
			goto err_exit;
		/* now mask row number to between 1 and 256 */
		stChallenge.rowNumber = (stChallenge.rowNumber & 0xff) + 1;
		/* nonce */
		if ((retVal = cvRandom(CV_NONCE_LEN, (uint8_t *)&stChallenge.nonce)) != CV_SUCCESS)
			goto err_exit;
		memcpy(pStatus, &stChallenge, sizeof(cv_st_challenge));

		/* now store in volatile memory for comparison with response */
		memcpy((uint8_t *)&CV_VOLATILE_DATA->stChallenge,(uint8_t *)&stChallenge,sizeof(cv_st_challenge));
		break;
#endif

	case CV_STATUS_IS_BOUND:
		*((cv_bool *)pStatus) = (CV_PERSISTENT_DATA->persistentFlags & CV_HAS_BIOS_SHARED_SECRET) ? TRUE : FALSE;
		break;

	case CV_STATUS_HAS_SUPER_REGISTRATION:
		*((cv_bool *)pStatus) = (CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE) ? TRUE : FALSE;
		break;
	
	case CV_STATUS_POA_TEST_RESULT:
		*((cv_bool *)pStatus) = (POA_GetStatus()) ? TRUE : FALSE;
		break;
#ifdef PHX_POA_BASIC
	case CV_STATUS_IS_POA_ENABLED:
		*((cv_bool *)pStatus) = (POA_IsPOAEnabled()) ? TRUE : FALSE;
		break;

	case CV_STATUS_CAPABILITIES:
		*((uint32_t *)pStatus) = CV_GetCapabilities();
		break;
#endif
	case 0xffff:
		break;
	default:
		retVal = CV_INVALID_INPUT_PARAMETER;
	}

err_exit:
	return retVal;
}

cv_status
cv_init (cv_handle cvHandle, cv_bool clearObjects, cv_bool cryptoSuiteB,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_obj_value_aes_key *pGCK,
			uint32_t newAuthListsLength, cv_auth_list *pNewAuthLists,
			cv_callback *callback, cv_callback_ctx context)
{
	/* This routine has several functions associated with CV administrator authorization.  If there is existing */
	/* CV administrator authorization (in the persistent data), authorization is performed to determine if the correct */
	/* authorization was provided.  If there is no existing CV administrator authorization, the auth lists provided are */
	/* saved	*/
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool persistentDataWriteRequired = FALSE;
	cv_input_parameters inputParameters;
	uint32_t i;
	cv_obj_hdr *objPtr;
	uint32_t gcklen = 0;
	uint32_t credits;
	cv_dir_page_entry_persist savedPersistEntry;
	cv_dir_page_entry_tpm savedTPMEntry;
	uint32_t savedDir0Counter;
	cv_admin_auth_permission permission;
	cv_obj_handle *objHandle;
	cv_obj_value_key *key;
	uint8_t buf[MAX_CV_OBJ_SIZE];
	cv_auth_list *mergedAuthLists = (cv_auth_list *)&buf[0];
	uint32_t mergedAuthListsLen = 0;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* do some validation first */
	/* only allow GCK provisioning in managed mode */
	if (pGCK != NULL && pGCK->keyLen && !(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
		return CV_MANAGED_MODE_REQUIRED;

	/* set up parameter list pointers, in case used by auth */
	if (pGCK != NULL)
		gcklen = pGCK->keyLen/8 + sizeof(pGCK->keyLen);
	cvSetupInputParameterPtrs(&inputParameters, 7, sizeof(clearObjects), &clearObjects,
		sizeof(clearObjects), &clearObjects, sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		gcklen, pGCK, sizeof(uint32_t), &newAuthListsLength,
		newAuthListsLength, pNewAuthLists,
		0, NULL, 0, NULL, 0, NULL);

	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);

	/* first check to see if CV admin auth exists.  if not, auth lists should be provided */
	if (CV_PERSISTENT_DATA->persistentFlags & CV_HAS_CV_ADMIN)
	{
		/* existing auth, must verify auth.  if in managed mode, only allow physical presence auth if also clearing objects */
		/* also, don't allow GCK to be provisioned with pp auth */
		if ((clearObjects || !(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE)) && (gcklen == 0))
			permission = CV_REQUIRE_ADMIN_AUTH_ALLOW_PP_AUTH;
		else
			permission = CV_REQUIRE_ADMIN_AUTH_ALLOW_SUPER_AUTH;
		/* now make sure all the objects necessary for doing auth are available */
		if ((retVal = cvHandlerDetermineAuthAvail(permission, &objProperties, authListLength, pAuthList,
			callback, context)) != CV_SUCCESS)
			goto err_exit;

		/* now should be able to do auth */
		retVal = cvDoAuth(permission, &objProperties, authListLength, pAuthList, &authFlags, &inputParameters,
			callback, context, FALSE,
			NULL, NULL);
		/* now check to see if auth succeeded */
		if (retVal != CV_SUCCESS && retVal != CV_OBJECT_UPDATE_REQUIRED)
			/* no, fail */
			goto err_exit;
		if (retVal == CV_OBJECT_UPDATE_REQUIRED)
			/* this is the case if count type auth is used.  CV admin auth is in persistent data and must be updated */
			persistentDataWriteRequired = TRUE;
		retVal = CV_SUCCESS;

		/* auth succeeded, but need to make sure correct auth granted (write auth, since persistent data will be changed) */
		if (!(authFlags & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)))
		{
			/* correct auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		/* check to see if clear auth requested */
		if (authFlags & CV_CLEAR_ADMIN_AUTH)
		{
			/* now check to see if anti-hammering will allow 2T usage. */
			if ((retVal = cvCheckAntiHammeringStatusPreCheck()) != CV_SUCCESS)
				goto err_exit;
			/* make sure authorization to change auth provided */
			if (!(authFlags & CV_OBJ_AUTH_CHANGE_AUTH))
			{
				/* correct auth not granted, fail */
				retVal = CV_AUTH_FAIL;
				goto err_exit;
			}
			/* yes, clear CV admin auth and managed mode */
			CV_PERSISTENT_DATA->persistentFlags &= ~(CV_HAS_CV_ADMIN | CV_MANAGED_MODE);
			persistentDataWriteRequired = TRUE;
		}
	} else {
		/* no existing auth, automatically allow change.  check to see if auth lists provided */
		authFlags = CV_OBJ_AUTH_CHANGE_AUTH;
		if (newAuthListsLength == 0)
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
	}

	/* here should be CV administrator authorized, determine what action required */
	/* check to see if new auth lists provided */
	if (newAuthListsLength)
	{
		/* now check to see if anti-hammering will allow 2T usage. */
		if ((retVal = cvCheckAntiHammeringStatusPreCheck()) != CV_SUCCESS)
			goto err_exit;

		/* make sure authorization to change auth provided */
		if (!(authFlags & CV_OBJ_AUTH_CHANGE_AUTH))
		{
			/* correct auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
		/* check to see if this is a partial auth lists update */
		if (pNewAuthLists->flags & CV_CHANGE_AUTH_PARTIAL)
		{
			/* yes, validate auth lists */
			if (pNewAuthLists->numEntries != 0)
			{
				retVal = CV_INVALID_AUTH_LIST;
				goto err_exit;
			}
			/* bump pointer past initial empty auth list */
			pNewAuthLists++;
			newAuthListsLength -= sizeof(cv_auth_list);
			if ((retVal = cvValidateAuthLists(CV_REQUIRE_ADMIN_AUTH, newAuthListsLength, pNewAuthLists)) != CV_SUCCESS)
				goto err_exit;
			if ((retVal = cvDoPartialAuthListsChange(newAuthListsLength, pNewAuthLists, CV_PERSISTENT_DATA->CVAdminAuthListLen,
					&CV_PERSISTENT_DATA->CVAdminAuthList, &mergedAuthListsLen, mergedAuthLists)) != CV_SUCCESS)
				goto err_exit;
			newAuthListsLength = mergedAuthListsLen;
			pNewAuthLists = mergedAuthLists;
		}

		/* yes, validate them and save in persistent storage */
		if (newAuthListsLength == 0 || (newAuthListsLength > MAX_CV_ADMIN_AUTH_LISTS_LENGTH))
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
		if ((retVal = cvValidateAuthLists(CV_REQUIRE_ADMIN_AUTH, newAuthListsLength, pNewAuthLists)) != CV_SUCCESS)
			goto err_exit;

		/* make sure authorization to change auth provided.  Also, write auth must be provided, since persistent data will be written */
		if (!cvHasIndicatedAuth(pNewAuthLists, newAuthListsLength,(CV_OBJ_AUTH_CHANGE_AUTH | CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)))
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
		memcpy(&CV_PERSISTENT_DATA->CVAdminAuthList, pNewAuthLists, newAuthListsLength);
		CV_PERSISTENT_DATA->CVAdminAuthListLen = newAuthListsLength;
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_CV_ADMIN;
		persistentDataWriteRequired = TRUE;
	} else {
		/* check to see if CV admin auth doesn't exist.  if so, it's required (unless just did clear auth) */
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_CV_ADMIN) && !(authFlags & CV_CLEAR_ADMIN_AUTH))
		{
			retVal = CV_INVALID_AUTH_LIST;
			goto err_exit;
		}
	}

	/* check to see if should clear objects */
	if (clearObjects)
	{
		/* now check to see if anti-hammering will allow 2T usage. */
		if ((retVal = cvCheckAntiHammeringStatusPreCheck()) != CV_SUCCESS)
			goto err_exit;

		/* send request to host storage to delete object files */
		/* first, check to see if there are any */
		for (i=0;i<MAX_DIR_PAGES/8;i++)
		{
			if (CV_TOP_LEVEL_DIR->pageMapExists[i])
			{
				if ((retVal = cvDelAllHostObjs()) != CV_SUCCESS)
					goto err_exit;
				break;
			}
		}

		/* init top level directory */
		/* don't zero anti-hammering credits, or some other entries for persistent data and TPM */
		credits = CV_TOP_LEVEL_DIR->header.antiHammeringCredits;
		savedDir0Counter = CV_TOP_LEVEL_DIR->dirEntries[0].counter;
		memcpy(&savedPersistEntry, &CV_TOP_LEVEL_DIR->persistentData, sizeof(cv_dir_page_entry_persist));
		memcpy(&savedTPMEntry, &CV_TOP_LEVEL_DIR->dirEntryTPM, sizeof(cv_dir_page_entry_tpm));

		memset(CV_TOP_LEVEL_DIR,0,sizeof(cv_top_level_dir) - sizeof(cv_dir_page_entry_tpm));
		/* now restore some entries */
		memcpy(&CV_TOP_LEVEL_DIR->persistentData, &savedPersistEntry, sizeof(cv_dir_page_entry_persist));
		memcpy(&CV_TOP_LEVEL_DIR->dirEntryTPM, &savedTPMEntry, sizeof(cv_dir_page_entry_tpm));
		CV_TOP_LEVEL_DIR->dirEntries[0].counter = savedDir0Counter;
		CV_TOP_LEVEL_DIR->header.antiHammeringCredits = credits;
		CV_TOP_LEVEL_DIR->header.thisDirPage.hDirPage = CV_TOP_LEVEL_DIR_HANDLE;
		CV_TOP_LEVEL_DIR->persistentData.len = sizeof(cv_persistent);
		CV_TOP_LEVEL_DIR->persistentData.entry.hDirPage = CV_PERSISTENT_DATA_HANDLE;

#if 0 /* Duplicate writing to top level dir causing inconsistent Flash data ...issue reported by Dell on AMG mfg */
		/* write to flash here (unless persistent data will be written, which also writes top level dir) */
		if ((retVal = cvWriteTopLevelDir(FALSE)) != CV_SUCCESS)
			goto err_exit;
#endif

		/* Init directory 0 */
		memset(CV_DIR_PAGE_0,0,sizeof(cv_dir_page_0));
		if ((retVal = cvWriteDirectoryPage0(FALSE)) != CV_SUCCESS)
			goto err_exit;

		/* Now initialize flash memory heap */
		cv_flash_heap_init();
		CV_VOLATILE_DATA->space.remainingNonvolatileSpace = CV_PERSISTENT_OBJECT_HEAP_LENGTH - CV_FLASH_ALLOC_HEAP_OVERHEAD;

		/* delete volatile objects  */
		for (i=0;i<MAX_CV_NUM_VOLATILE_OBJS;i++)
		{
			if (CV_VOLATILE_OBJ_DIR->volObjs[i].hObj != 0)
			{
				objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[i].pObj;
				cv_free(CV_VOLATILE_OBJ_DIR->volObjs[i].pObj, objPtr->objLen);
			}
		}
		/* close all open sessions, except the one used by this command */
		cvCloseOpenSessions(cvHandle);

		memset(CV_VOLATILE_OBJ_DIR,0,sizeof(cv_volatile_obj_dir));
		memset(CV_VOLATILE_DATA->objCache,0,sizeof(cv_obj_handle)*MAX_CV_OBJ_CACHE_NUM_OBJS);
		memset(CV_VOLATILE_DATA->level1DirCache,0,sizeof(cv_obj_handle)*MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES);
		memset(CV_VOLATILE_DATA->level2DirCache,0,sizeof(cv_obj_handle)*MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES);
		for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
			CV_VOLATILE_DATA->objCacheMRU[i] = i;
		for (i=0;i<MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES;i++)
			CV_VOLATILE_DATA->level1DirCacheMRU[i] = i;
		for (i=0;i<MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES;i++)
			CV_VOLATILE_DATA->level2DirCacheMRU[i] = i;

		/* now reset config params to defaults */
		cvSetConfigDefaults();
		persistentDataWriteRequired = TRUE;

		/* if managed mode, this is the decommissioning scenario so clear everything */
		if (CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE)
		{
			/* decommissioning mode */
			/* clear CV admin auth (unless new auth lists provided), SUPER and GCK */
			CV_PERSISTENT_DATA->persistentFlags &= ~(CV_MANAGED_MODE | CV_GCK_EXISTS);
			memset(&CV_PERSISTENT_DATA->GCKKey,0,AES_128_BLOCK_LEN);
			memset(&CV_PERSISTENT_DATA->superConfigParams,0,sizeof(cv_super_config));
			memset(&CV_PERSISTENT_DATA->superRSN,0,CV_SUPER_RSN_LEN);
			CV_PERSISTENT_DATA->superCtr = 0;
			/* leave CV admin auth existant if new auth lists provided with command */
			if (!newAuthListsLength)
			{
				memset(&CV_PERSISTENT_DATA->authList,0,MAX_CV_ADMIN_AUTH_LISTS_LENGTH);
				CV_PERSISTENT_DATA->persistentFlags &= ~CV_HAS_CV_ADMIN;
			}
		}	

		/* set cryptoSuiteB value (only if object clear performed) */
		if (cryptoSuiteB && !(CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS))
		{
			CV_PERSISTENT_DATA->persistentFlags |= CV_USE_SUITE_B_ALGS;
			persistentDataWriteRequired = TRUE;
		} else if (!cryptoSuiteB && (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS)){
			CV_PERSISTENT_DATA->persistentFlags &= ~CV_USE_SUITE_B_ALGS;
			persistentDataWriteRequired = TRUE;
		}
	}

	/* check to see if GCK provided */
	if (gcklen)
	{
		if (pGCK->keyLen != CV_GCK_KEY_LEN && pGCK->keyLen != CV_GCK_KEY_HANDLE_LEN)
		{
			retVal = CV_INVALID_GCK_KEY_LENGTH;
			goto err_exit;
		}
		/* check to see if the key is provided or a handle to the key object */
		if (pGCK->keyLen == CV_GCK_KEY_HANDLE_LEN)
		{
			objHandle = (cv_obj_handle *)&pGCK->key;
			/* check to see if GCK being removed (NULL handle) */
			if (*objHandle == 0)
				/* flag GCK as not exist */
				CV_PERSISTENT_DATA->persistentFlags &= ~CV_GCK_EXISTS;
			else 
			{
				/* get GCK from object */
				objProperties.objHandle = *objHandle;
				if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
					goto err_exit;
				key = (cv_obj_value_key *)objProperties.pObjValue;
				if (objPtr->objType != CV_TYPE_AES_KEY || key->keyLen != 128)
				{
					retVal = CV_INVALID_GCK_KEY_LENGTH;
					goto err_exit;
				}
				memcpy(CV_PERSISTENT_DATA->GCKKey, key->key, CV_GCK_KEY_LEN/8);
				CV_PERSISTENT_DATA->persistentFlags |= CV_GCK_EXISTS;
			}
		} else {
			memcpy(CV_PERSISTENT_DATA->GCKKey, pGCK->key, CV_GCK_KEY_LEN/8);
			CV_PERSISTENT_DATA->persistentFlags |= CV_GCK_EXISTS;
		}
		persistentDataWriteRequired = TRUE;
	}

    if( clearObjects && CV_PERSISTENT_DATA->poa_enable )
    {
       /* when POA is enabled, if clear all objects, need to deactivate POA in EC */
        ush_poa_deactivate();
    }

err_exit:
	/* write persistent data even if an error occurred subsequently */
	if (persistentDataWriteRequired)
		retVal = cvWritePersistentData(TRUE);

	return retVal;
}

cv_status
cvDoReplayOverride(void)
{
	/* this routine restores object directories and resets override counters for hard drive migration case */
	/* first check to see if top level directory and directory 0 empty.  if not, fail */
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL};
	uint8_t *objDirPtrsOut[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t i;
	uint32_t count;
	uint32_t objCacheEntry = INVALID_CV_CACHE_ENTRY;
	uint8_t *objStoragePtr;
	cv_obj_hdr *objPtr;
	uint8_t *flashPtr;
	uint32_t objLen;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
	{
		if (CV_TOP_LEVEL_DIR->dirEntries[i].hDirPage != 0)
		{
			retVal = CV_CV_NOT_EMPTY;
			goto err_exit;
		}
	}
	/* now check directory page 0 */
	for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
	{
		if (CV_DIR_PAGE_0->dir0Objs[i].hObj != 0)
		{
			retVal = CV_CV_NOT_EMPTY;
			goto err_exit;
		}
	}
	/* make sure there is a GCK */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_GCK_EXISTS))
	{
		/* no GCK, fail */
		retVal = CV_NO_GCK;
		goto err_exit;
	}
	/* first read in UCK wrapped with GCK */
	objDirHandles[2] = CV_UCK_OBJ_HANDLE;
	objDirPtrs[2] = &CV_PERSISTENT_DATA->UCKKey[0];
	if ((retVal = cvGetHostObj(objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;
	/* now unwrap object */
	objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_UCK;
	if ((retVal = cvUnwrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;

	/* read in mirrored top level directory */
	objDirHandles[2] = CV_MIRRORED_TOP_LEVEL_DIR;
	objDirPtrs[2] = (uint8_t *)CV_TOP_LEVEL_DIR;
	if ((retVal = cvGetHostObj(objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;
	/* now unwrap object */
	objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ;
	if ((retVal = cvUnwrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;
    /* update anti-replay count */
	if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
		goto err_exit;
	/* don't write top level directory here, because will get written when persistent data written */

	/* read in mirrored directory page 0 */
	objDirHandles[2] = CV_DIR_0_HANDLE;
	objDirPtrs[2] = (uint8_t *)CV_TOP_LEVEL_DIR;
	if ((retVal = cvGetHostObj(objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;
	/* now unwrap object */
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_NON_OBJ;
	if ((retVal = cvUnwrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
		goto err_exit;

	/* now need to read mirrored objects which were in flash and re-write to flash */
	/* step through directory page 0 */
	cvObjCacheGetLRU(&objCacheEntry, &objStoragePtr);
	objPtr = (cv_obj_hdr *)objStoragePtr;
	for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
	{
		if (CV_DIR_PAGE_0->dir0Objs[i].hObj != 0 && (CV_DIR_PAGE_0->dir0Objs[i].attributes.storageType & CV_STORAGE_TYPE_FLASH))
		{
			objDirHandles[2] = CV_DIR_PAGE_0->dir0Objs[i].hObj | CV_MIRRORED_OBJ_MASK;
			objDirPtrs[2] = objStoragePtr;
			if ((retVal = cvGetHostObj(objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
				goto err_exit;
			/* now unwrap object */
			objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ;
			if ((retVal = cvUnwrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
				goto err_exit;
			/* allocate space in flash */
			if ((flashPtr = (uint8_t *)cv_malloc_flash(cvGetWrappedSize(objPtr->objLen))) == NULL)
			{
				retVal = CV_FLASH_MEMORY_ALLOCATION_FAIL;
				goto err_exit;
			}
			/* update entry in directory page 0 with new address */
			CV_DIR_PAGE_0->dir0Objs[i].pObj = flashPtr;
			/* now write object to flash */
			objAttribs.storageType = CV_STORAGE_TYPE_FLASH;
			objLen = objPtr->objLen;
			if ((retVal = cvWrapObj(NULL, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
				goto err_exit;
			if ((retVal = cvPutFlashObj(&CV_DIR_PAGE_0->dir0Objs[i], CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				goto err_exit;
		}
	}
	/* now write directory page 0 to flash */
	if ((retVal = cvWriteDirectoryPage0(TRUE)) != CV_SUCCESS)
		goto err_exit;
	/* write persistent data (also writes top level directory) */
	retVal = cvWritePersistentData(TRUE);

err_exit:
	return retVal;
}

int32_t
cvComputeTimeZoneOffset(cv_time *time1, cv_time *time2)
{
	/* this routine computes the difference in hours time2 - time1 */
	/* convert hours component from BCD to hex and subtract */
	int32_t hour1, hour2, resTime;

	hour1 = ((time1->time[4] & 0xf0)>>4)*10 + (time1->time[4] & 0x0f);
	hour2 = ((time2->time[4] & 0xf0)>>4)*10 + (time2->time[4] & 0x0f);
	resTime = hour2 - hour1;
	/* need to convert to within +/- 12 */
	if (resTime > 12)
		resTime -= 24;
	return resTime;
}

cv_status
cv_set_config (cv_handle cvHandle, uint32_t authListLength,
				cv_auth_list *pAuthList, cv_config_type configType,
				void *pConfig,
				cv_callback *callback, cv_callback_ctx context)
{
	/* This function sets this indicated configuration information in the CVAPI.  This command */
	/* requires administrator authorization. */
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_bool persistentDataWriteRequired = FALSE;
	cv_input_parameters inputParameters;
	cv_da_params *daParams = (cv_da_params *)pConfig;
#if 0
/* temporarily don't support */
	cv_policy *policy = (cv_policy *)pConfig;
	cv_time_params *cvTime = (cv_time_params *)pConfig;
	cv_obj_handle *objHandle = (cv_obj_handle *)pConfig;
	cv_super_config *superConfig = (cv_super_config *)pConfig;
	cv_obj_hdr *objPtr;
	cv_obj_value_key *key;
	cv_ecc256 *eccKeyIn;
	cv_dsa1024_key *dsaKeyIn;
#endif
	uint32_t *paramValSigned = (uint32_t *)pConfig; /* changed int32 to uint32 to fix coverity issue */
	uint32_t *paramVal = (uint32_t *)pConfig;
	cv_hash_type *hashType = (cv_hash_type *)pConfig;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 4,
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		sizeof(uint32_t), &configType, 0, pConfig,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	/* need to set up length field for pConfig in inputParameters, since dependent on configType */
	switch (configType)
	{
	case CV_CONFIG_DA_PARAMS:
		inputParameters.paramLenPtr[4].paramLen = sizeof(cv_da_params);
		break;
	case CV_CONFIG_REPLAY_OVERRIDE:
		inputParameters.paramLenPtr[4].paramLen = 0;
		break;
	case CV_CONFIG_POLICY:
		inputParameters.paramLenPtr[4].paramLen = sizeof(cv_policy);
		break;
	case CV_CONFIG_TIME:
		inputParameters.paramLenPtr[4].paramLen = 2*sizeof(cv_time);
		break;
	case CV_CONFIG_KDIX:
		inputParameters.paramLenPtr[4].paramLen = sizeof(cv_obj_handle);
		break;
	case CV_CONFIG_FP_POLLING_INTERVAL:
		inputParameters.paramLenPtr[4].paramLen = sizeof(uint32_t);
		break;
	case CV_CONFIG_FP_PERSISTANCE:
		inputParameters.paramLenPtr[4].paramLen = sizeof(uint32_t);
		break;
	case CV_CONFIG_FP_HMAC_TYPE:
		inputParameters.paramLenPtr[4].paramLen = sizeof(cv_hash_type);
		break;
	case CV_CONFIG_FP_FAR:
		inputParameters.paramLenPtr[4].paramLen = sizeof(uint32_t);
		break;
	case CV_CONFIG_IDENTIFY_USER_TIMEOUT:
		inputParameters.paramLenPtr[4].paramLen = sizeof(uint32_t);
		break;
	case CV_CONFIG_SUPER:
		inputParameters.paramLenPtr[4].paramLen = sizeof(cv_super_config);
		break;
	case CV_CONFIG_RESET_ANTI_REPLAY_OVERRIDE:
		inputParameters.paramLenPtr[4].paramLen = sizeof(uint32_t);
		break;
	case 0xffff:
		break;
	default:
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);
	/* must verify auth */
	/* now make sure all the objects necessary for doing auth are available */
	if ((retVal = cvHandlerDetermineAuthAvail(CV_REQUIRE_ADMIN_AUTH, &objProperties, authListLength, pAuthList,
		callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* now should be able to do auth */
	retVal = cvDoAuth(CV_REQUIRE_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &authFlags, &inputParameters,
		callback, context, FALSE,
		NULL, NULL);
	/* now check to see if auth succeeded */
	if (retVal != CV_SUCCESS && retVal != CV_OBJECT_UPDATE_REQUIRED)
		/* no, fail */
		goto err_exit;
	if (retVal == CV_OBJECT_UPDATE_REQUIRED)
		/* this is the case if count type auth is used.  CV admin auth is in persistent data and must be updated */
		persistentDataWriteRequired = TRUE;
	retVal = CV_SUCCESS;

	/* auth succeeded, but need to make sure correct auth granted (write auth, since persistent data will be changed) */
	if (!(authFlags & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)))
	{
		/* correct auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* now determine configuration parameters and do appropriate action */
	switch (configType)
	{
	case CV_CONFIG_DA_PARAMS:
		/* first validate params */
		if (daParams->maxFailedAttempts > CV_DA_MAX_MAX_FAILED_ATTEMPTS
			|| daParams->maxFailedAttempts < CV_DA_MIN_MAX_FAILED_ATTEMPTS)
		{
			retVal = CV_INVALID_DA_PARAMS;
			goto err_exit;
		}
		if (daParams->lockoutDuration > CV_DA_MAX_INITIAL_LOCKOUT_PERIOD
			|| daParams->lockoutDuration < CV_DA_MIN_INITIAL_LOCKOUT_PERIOD)
		{
			retVal = CV_INVALID_DA_PARAMS;
			goto err_exit;
		}
		CV_PERSISTENT_DATA->DAAuthFailureThreshold = daParams->maxFailedAttempts;
		CV_PERSISTENT_DATA->DAInitialLockoutPeriod = daParams->lockoutDuration*1000;   /* convert to ms */
		persistentDataWriteRequired = TRUE;
		/* now reset volatile values if in managed mode */
		if (CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE)
		{
			CV_VOLATILE_DATA->DAFailedAuthCount = 0;
			CV_VOLATILE_DATA->DALockoutCount = 0;
			CV_VOLATILE_DATA->CVInternalState &= ~CV_IN_DA_LOCKOUT;
		}
		break;
	case CV_CONFIG_REPLAY_OVERRIDE:
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		/* allows replay protection of objects to be synchronized with monotonic counter in CV */
		/* (for migration of hard drive to new shell); no associated structure */
		cvDoReplayOverride();
		break;
#endif
	case CV_CONFIG_POLICY:
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		CV_PERSISTENT_DATA->cvPolicy = *policy;
		persistentDataWriteRequired = TRUE;
		break;
#endif
	case CV_CONFIG_TIME:
		/* set up RTC with GMT time */
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		if ((retVal = set_time(&cvTime->GMTTime)) == CV_SUCCESS)
		{
			/* indicate that time available and compute time zone */
			CV_PERSISTENT_DATA->persistentFlags |= CV_RTC_TIME_CONFIGURED;
			CV_PERSISTENT_DATA->localTimeZoneOffset = cvComputeTimeZoneOffset(&cvTime->GMTTime, &cvTime->localTime);
			persistentDataWriteRequired = TRUE;
		} else {
			retVal = CV_RTC_FAILURE;
			goto err_exit;
		}
		break;
#endif
	case CV_CONFIG_KDIX:
		/* need to configure CV to use key specified by handle */
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		objProperties.objHandle = *objHandle;
		/* if handle is null, revert to kdi */
		if (*objHandle == 0)
		{
			CV_PERSISTENT_DATA->persistentFlags &= ~(CV_USE_KDIX_DSA | CV_USE_KDIX_ECC);
			persistentDataWriteRequired = TRUE;
		} else {
			/* not a null handle.  check to see if can use key specified by handle */
			if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
				goto err_exit;
			/* now see if this is the right kind of key */
			cvFindObjPtrs(&objProperties, objPtr);
			key = (cv_obj_value_key *)objProperties.pObjValue;
			if (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS)
			{
				/* if suite B, this must be an ECC key */
				if (objPtr->objType != CV_TYPE_ECC_KEY || key->keyLen != ECC256_KEY_LEN_BITS)
				{
					retVal = CV_INVALID_KDIX_KEY;
					goto err_exit;
				}
				/* type ok, copy to persistent data */
				eccKeyIn = (cv_ecc256 *)&key->key;
				memcpy(CV_PERSISTENT_DATA->secureSessionSigningKey.eccKey.Q, eccKeyIn->Q, sizeof(eccKeyIn->Q));
				memcpy(CV_PERSISTENT_DATA->secureSessionSigningKey.eccKey.x, eccKeyIn->x, sizeof(eccKeyIn->x));
				CV_PERSISTENT_DATA->persistentFlags |= CV_USE_KDIX_ECC;
				persistentDataWriteRequired = TRUE;
			} else {
				/* not suite B this must be a DSA key */
				if (objPtr->objType != CV_TYPE_DSA_KEY || key->keyLen != DSA1024_KEY_LEN_BITS)
				{
					retVal = CV_INVALID_KDIX_KEY;
					goto err_exit;
				}
				/* type ok, copy to persistent data */
				dsaKeyIn = (cv_dsa1024_key *)&key->key;
				memcpy(CV_PERSISTENT_DATA->secureSessionSigningKey.dsaKey.x, dsaKeyIn->x, sizeof(dsaKeyIn->x));
				CV_PERSISTENT_DATA->persistentFlags |= CV_USE_KDIX_DSA;
				persistentDataWriteRequired = TRUE;
			}
		}
		break;
#endif
	case CV_CONFIG_FP_POLLING_INTERVAL:
		if (*paramVal >= FP_MIN_SW_FDET_POLLING_INTERVAL && *paramVal<= FP_MAX_SW_FDET_POLLING_INTERVAL)
		{
			CV_PERSISTENT_DATA->fpPollingInterval = *paramVal;
			persistentDataWriteRequired = TRUE;
		} else {
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_CONFIG_FP_PERSISTANCE:
		if (*paramVal >= FP_MIN_PERSISTENCE && *paramVal<= FP_MAX_PERSISTENCE)
		{
			CV_PERSISTENT_DATA->fpPersistence = *paramVal;
			persistentDataWriteRequired = TRUE;
		} else {
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_CONFIG_FP_HMAC_TYPE:
		if (*hashType == CV_SHA1 || *hashType == CV_SHA256)
		{
			CV_PERSISTENT_DATA->fpAntiReplayHMACType = *hashType;
			persistentDataWriteRequired = TRUE;
		} else {
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_CONFIG_FP_FAR:
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
			return CV_MANAGED_MODE_REQUIRED;
		if (/**paramValSigned >= FP_MIN_FAR  && */*paramValSigned <= FP_MAX_FAR)
		{
			CV_PERSISTENT_DATA->FARval = *paramVal;
			persistentDataWriteRequired = TRUE;
			CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_FAR_VALUE;
		} else {
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_CONFIG_IDENTIFY_USER_TIMEOUT:
		if (*paramVal >= FP_MIN_USER_TIMEOUT && *paramVal<= FP_MAX_USER_TIMEOUT)
		{
			CV_PERSISTENT_DATA->identifyUserTimeout = *paramVal;
			persistentDataWriteRequired = TRUE;
		} else {
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		break;
	case CV_CONFIG_SUPER:
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		memcpy(&CV_PERSISTENT_DATA->superConfigParams, superConfig, sizeof(cv_super_config));
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_SUPER_PARAMS;
		persistentDataWriteRequired = TRUE;
		break;
#endif
	case CV_CONFIG_RESET_ANTI_REPLAY_OVERRIDE:
/* temporarily don't allow this operation */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		if (*paramVal != TRUE)
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		if (CV_PERSISTENT_DATA->persistentFlags & CV_ANTI_REPLAY_OVERRIDE)
		{
			/* clear anti replay override flag */
			CV_PERSISTENT_DATA->persistentFlags &= ~CV_ANTI_REPLAY_OVERRIDE;
			persistentDataWriteRequired = TRUE;
		}
		break;
#endif
	case 0xffff:
		break;
	}

	/* check to see if need to write persistent data */
	if (persistentDataWriteRequired)
		retVal = cvWritePersistentData(TRUE);

err_exit:
	return retVal;
}

uint32_t computeChecksum(uint32_t len, uint8_t *buf)
{
	/* this function computes the checksum of the buffer provided (zero padded) */
	uint32_t i, computedChecksum = 0, remBytes, finalDword = 0;

	for (i=0;i<len/sizeof(uint32_t);i++)
		computedChecksum ^= ((uint32_t *)buf)[i];
	remBytes = len - i*sizeof(uint32_t);
	if (remBytes)
	{
		memcpy(&finalDword, &buf[len-remBytes], remBytes);
		computedChecksum ^= finalDword;
	}
	return computedChecksum;
}

cv_status
cv_send_blockdata(uint32_t payloadLen, uint8_t *payload, uint32_t checksum)
{
	/* this function just checks the checksum of the data sent and returns success if it matches */
	uint32_t computedChecksum;

	computedChecksum = computeChecksum(payloadLen, payload);
	if (checksum == computedChecksum)
		return CV_SUCCESS;
	else
		return CV_INVALID_INPUT_PARAMETER;
}

cv_status
cv_receive_blockdata(cv_bool random, uint32_t pattern, uint32_t *payloadLen, uint8_t *payload, uint32_t *checksum)
{
	/* this function fills the payload buffer either with random data, or pattern data */
	cv_status retVal = CV_SUCCESS;
	uint32_t i, remBytes;
	uint8_t *buf;

	/* not allowed if FP image buf not available.  s/b ok, because this is just a test routine */
	if (!isFPImageBufAvail())
		return CV_FP_DEVICE_BUSY;

	/* use FP buffer to store payload, since might be up to 90K */
	buf = FP_CAPTURE_LARGE_HEAP_PTR;
	/* check to make sure not too large */
	if (*payloadLen > FP_IMAGE_MAX_SIZE)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		*payloadLen = FP_IMAGE_MAX_SIZE;
		goto err_exit;
	}

	if (random)
	{
		if ((retVal = cvRandom(*payloadLen, buf)) != CV_SUCCESS)
			goto err_exit;
	} else {
		for (i=0;i<*payloadLen/sizeof(uint32_t);i++)
			((uint32_t *)buf)[i] = pattern;
		remBytes = *payloadLen - i*sizeof(uint32_t);
		if (remBytes)
			memcpy(&buf[*payloadLen-remBytes], &pattern, remBytes);
	}
	*checksum = computeChecksum(*payloadLen, buf);

err_exit:
	return retVal;

}

cv_status
cv_register_super ( 
/* in */ cv_handle cvHandle, 
/* in */ uint32_t authListLength,			/* length of auth list in bytes */
/* in */ cv_auth_list *pAuthList,				/* auth list with CV admin auth */
/* in */ uint32_t symKeyLen,				/* AES key length in bytes – must be 16*/
/* in */ uint8_t *symKey,					/* AES key for encrypting registration params */
/* out */ cv_super_registration_params *encRegistrationParams,	/* encrypted registration parameters*/
/* in */ cv_callback *callback,
/* in */ cv_callback_ctx context)
{
	/* this function registers SUPER parameters, if CV admin auth is satisfied */
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_input_parameters inputParameters;
	cv_super_registration_params regParams, regParamsEnc;
	uint32_t encryptedLength = AES_BLOCK_LENGTH;
	cv_bulk_params localBulkParams;
	uint8_t key[sizeof(cv_obj_value_key) - 1 + AES_128_BLOCK_LEN];
	cv_obj_value_key *pKey = (cv_obj_value_key *)&key[0];

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* validate parameters */
	if (symKeyLen != AES_BLOCK_LENGTH)
		return CV_INVALID_INPUT_PARAMETER;


	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 5,
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		sizeof(uint32_t), &symKeyLen, symKeyLen, symKey,
		0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	/* must verify auth */
	/* now make sure all the objects necessary for doing auth are available */
	if ((retVal = cvHandlerDetermineAuthAvail(CV_REQUIRE_ADMIN_AUTH, &objProperties, authListLength, pAuthList,
		callback, context)) != CV_SUCCESS)
		goto err_exit;

	/* now should be able to do auth */
	retVal = cvDoAuth(CV_REQUIRE_ADMIN_AUTH, &objProperties, authListLength, pAuthList, &authFlags, &inputParameters,
		callback, context, FALSE,
		NULL, NULL);
	/* now check to see if auth succeeded */
	if (retVal != CV_SUCCESS && retVal != CV_OBJECT_UPDATE_REQUIRED)
		/* no, fail */
		goto err_exit;
	retVal = CV_SUCCESS;

	/* auth succeeded, but need to make sure correct auth granted (change auth, since configuring SUPER is equivalent to creating CV admin) */
	if (!(authFlags & CV_OBJ_AUTH_CHANGE_AUTH))
	{
		/* correct auth not granted, fail */
		retVal = CV_AUTH_FAIL;
		goto err_exit;
	}

	/* Use random number generator to create the following quantities, which will be stored in cv_persistent data structure */
	retVal = cvRandom(sizeof(cv_super_config), (uint8_t *)&CV_PERSISTENT_DATA->superConfigParams);
	retVal = cvRandom(CV_SUPER_RSN_LEN, (uint8_t *)&CV_PERSISTENT_DATA->superRSN[0]);

	/* Initialize counter (32-bits, super_ctr, also in cv_persistent data structure) to 1 */
	CV_PERSISTENT_DATA->superCtr = 1;

	/* Set CV_MANAGED_MODE bit in cv_persistent_flags */
	CV_PERSISTENT_DATA->persistentFlags |= CV_MANAGED_MODE;

	/* Save persistent data */
	retVal = cvWritePersistentData(TRUE);

	/* Encrypt Z with R (CBC, null IV) */
	memset(&localBulkParams,0,sizeof(localBulkParams));
	localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
	localBulkParams.authOffset = 0;
	localBulkParams.bulkMode = CV_CBC;
	localBulkParams.encOffset = 0;
	localBulkParams.IVLen = AES_BLOCK_LENGTH; 
	pKey->keyLen = 128;
	memcpy(&pKey->key, &CV_PERSISTENT_DATA->superConfigParams.R, AES_BLOCK_LENGTH);
	if ((retVal = cvBulkEncrypt(&CV_PERSISTENT_DATA->superConfigParams.Z[0], AES_BLOCK_LENGTH, &regParams.encZ[0], &encryptedLength, NULL, NULL, 
		CV_BULK_ENCRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
		goto err_exit;

	/* Encrypt Z, X and unique hardware ID with AES key passed in as parameter (CBC, null IV) */
	memcpy(&regParams.RSN, &CV_PERSISTENT_DATA->superRSN[0], CV_SUPER_RSN_LEN);
	memcpy(&regParams.X, &CV_PERSISTENT_DATA->superConfigParams.X, AES_BLOCK_LENGTH);
	memcpy(&pKey->key, symKey, symKeyLen);
	encryptedLength = sizeof(regParams);
	if ((retVal = cvBulkEncrypt((uint8_t *)&regParams, sizeof(regParams), (uint8_t *)&regParamsEnc, &encryptedLength, NULL, NULL, 
		CV_BULK_ENCRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
		goto err_exit;

	/* Return encrypted blob as encRegistrationParams */
	memcpy(encRegistrationParams, &regParamsEnc, sizeof(regParamsEnc));

err_exit:
	return retVal;
}

cv_status
cv_challenge_super ( 
/* in */ cv_handle cvHandle, 
/* out */ cv_super_challenge *challenge)				/* challenge */ 
{
	/* this routine computes a challenge based on the existing SUPER parameters */
	cv_status retVal = CV_SUCCESS;
	cv_super_challenge retChallenge;
	uint8_t encCtr[AES_128_BLOCK_LEN];
	uint8_t aesCmacRes[AES_128_BLOCK_LEN];

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* make sure SUPER has been registered */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
		return CV_SUPER_NOT_REGISTERED;

	/* Create encrypted counter by encrypting super_ctr with X in cv_super_config structure (using CMAC, null IV)	*/
	if ((retVal = cvAESCMAC((uint8_t *)&CV_PERSISTENT_DATA->superCtr, sizeof(uint32_t), (uint8_t *)&CV_PERSISTENT_DATA->superConfigParams.X[0], 
			AES_128_BLOCK_LEN, &aesCmacRes[0])) != CV_SUCCESS)
		goto err_exit;
	memcpy(&encCtr[0],&aesCmacRes[0],sizeof(uint32_t));

	/* Return structure cv_super_challenge, as challenge and challengeLength, using super_ctr and MS 32-bits of encrypted counter */
	memcpy(&retChallenge.encCtr, &encCtr[0], sizeof(uint32_t));
	memcpy(&retChallenge.ctr, &CV_PERSISTENT_DATA->superCtr, sizeof(uint32_t));
	memcpy(&retChallenge.RSN, &CV_PERSISTENT_DATA->superRSN, CV_SUPER_RSN_LEN);
	memcpy(challenge, &retChallenge, sizeof(cv_super_challenge));

err_exit:
	return retVal;
}

cv_status
cvResponseSuper(uint8_t response[AES_128_BLOCK_LEN])
{
	cv_status retVal = CV_SUCCESS;
	uint32_t decryptedLength;
	cv_bulk_params localBulkParams;
	uint8_t key[sizeof(cv_obj_value_key) - 1 + AES_128_BLOCK_LEN];
	cv_obj_value_key *pKey = (cv_obj_value_key *)&key[0];
	uint8_t encZ[AES_128_BLOCK_LEN];
	uint8_t decZ[AES_128_BLOCK_LEN];

	/* Decrypt response with X (using CBC, null IV) to create encrypted Z */
	memset(&localBulkParams,0,sizeof(localBulkParams));
	localBulkParams.authAlgorithm = CV_AUTH_ALG_NONE;
	localBulkParams.authOffset = 0;
	localBulkParams.bulkMode = CV_ECB;
	localBulkParams.encOffset = 0;
	pKey->keyLen = 128;
	memcpy(&pKey->key, &CV_PERSISTENT_DATA->superConfigParams.X, AES_BLOCK_LENGTH);
	if ((retVal = cvBulkDecrypt(response, AES_128_BLOCK_LEN, &encZ[0], &decryptedLength, NULL, NULL, 
		CV_BULK_DECRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
		goto err_exit;

	/* Decrypt encrypted Z with R (using CBC, null IV) */
	memcpy(&pKey->key, &CV_PERSISTENT_DATA->superConfigParams.R, AES_BLOCK_LENGTH);
	if ((retVal = cvBulkDecrypt(&encZ[0], AES_128_BLOCK_LEN, &decZ[0], &decryptedLength, NULL, NULL, 
		CV_BULK_DECRYPT, &localBulkParams, CV_TYPE_AES_KEY, pKey, NULL)) != CV_SUCCESS)
		goto err_exit;

	/* test to see if decrypted Z matches persistent value of Z */
	if (!memcmp(&decZ[0], &CV_PERSISTENT_DATA->superConfigParams.Z, AES_BLOCK_LENGTH))
	{
		/* yes, update ctr and X */
		CV_PERSISTENT_DATA->superCtr++;
		if ((retVal = cvAESCMAC((uint8_t *)&CV_PERSISTENT_DATA->superConfigParams.X[0], AES_128_BLOCK_LEN, &CV_PERSISTENT_DATA->superConfigParams.X[0], 
				AES_128_BLOCK_LEN, &CV_PERSISTENT_DATA->superConfigParams.X[0])) != CV_SUCCESS)
			goto err_exit;
		/* Save persistent data */
		retVal = cvWritePersistentData(TRUE);
	} else
		retVal = CV_SUPER_RESPONSE_FAILURE;

err_exit:
	return retVal;
}

cv_status
cv_response_super ( 
/* in */ cv_handle cvHandle, 
/* in */ uint8_t response[AES_128_BLOCK_LEN])	/* response */ 
{
	/* this routine determines if a response to a previously issued challenge is successful */
	cv_status retVal = CV_SUCCESS;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* make sure SUPER has been registered */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
		return CV_SUPER_NOT_REGISTERED;

	retVal = cvResponseSuper(response);

	return retVal;
}

cv_status
cvHostCLAsyncInterrupt(cv_bool arrival)
{
	/* this routine sends an interrupt to host for an async contactless smartcard detection */
	cv_usb_interrupt *usbInt = (cv_usb_interrupt *)CV_SECURE_IO_FP_ASYNC_BUF;
	cv_cl_event_type *eventType;

	printf("cvHostCLAsyncInterrupt: start - arrival %d\n", arrival);
	usbInt->interruptType = CV_CONTACTLESS_EVENT_INTERRUPT;
	/* set resultLen field with CL event type */
	eventType = (cv_cl_event_type *)&usbInt->resultLen;
	if (arrival)
		*eventType = CV_CL_EVENT_ARRIVAL; /* arrival */
	else
		*eventType = CV_CL_EVENT_REMOVAL; /* removal */
	usbCvInterruptIn(usbInt);
	ms_wait(100);
	printf("cvHostCLAsyncInterrupt: end\n");
	return CV_SUCCESS;
}


cv_status
cv_enable_function (cv_function_enables functionEnables)
{
	/* this routine enables or disables the indicated function */
	cv_status retVal = CV_SUCCESS;

	switch (functionEnables)
	{
        case CV_ENABLE_CLSC_NOTIFICATION:
                  printf("setting the contactlessCardDetect to true \n");
                  CV_VOLATILE_DATA->contactlessCardDetectEnable = TRUE;
		break;

        case CV_DISABLE_CLSC_NOTIFICATION:
                printf("setting the contactlessCardDetect to false \n");
		CV_VOLATILE_DATA->contactlessCardDetectEnable = FALSE;
		break;

	case 0xffff:
		break;

	default:
		retVal = CV_INVALID_INPUT_PARAMETER;
	}

	return retVal;
}


#ifdef CV_TEST_TPM
cv_status
cv_test_tpm ( 
/* in */ uint32_t cmdLength,    /* command length */
/* in */ uint8_t *cmd,			/* command */ 
/* out */ uint32_t *respLength,  /* response length */
/* out */ uint8_t *resp)		/* response */ 
{
	/* this routine tests routes a TPM command to the TPM via the USB */
	cv_status retVal = CV_SUCCESS;
    intro *in = (intro *)MEM_BASE_IO;
    outro *out = (outro *)MEM_BASE_IO;

	memcpy((byte *)in, cmd, cmdLength);
	memcpy(MEM_BASE_IO_UNSWAPPED, (byte *)(in), BYTESWAPLONG(in->paramSize));
	doCmdSwitch(MEM_BASE_VMEM->xmem);
	memcpy(resp, (byte *)(out), BYTESWAPLONG(out->paramSize));
	*respLength = BYTESWAPLONG(out->paramSize);

	return retVal;
}
#endif

unsigned int cv_start_timer2(void)
{
	if(CV_VOLATILE_DATA->cvTimer2Start > 0)
	{
		/* stop the timer */
		CV_VOLATILE_DATA->cvTimer2ElapsedTime += lynx_crmu_get_timer2_sec();
		printf("%s: Stop the timer cvTimer2ElapsedTime  = %u \n",__FUNCTION__,CV_VOLATILE_DATA->cvTimer2ElapsedTime);
		lynx_crmu_stop_timer2();
	}
	else
	{
		CV_VOLATILE_DATA->cvTimer2Start = 1;
	}
	printf("%s: Start the timer \n",__FUNCTION__);
	lynx_crmu_start_timer2();
	return  CV_VOLATILE_DATA->cvTimer2ElapsedTime;
}

unsigned int cv_get_timer2_elapsed_time(unsigned int prevElapsedTime)
{
	unsigned int time1 = lynx_crmu_get_timer2_sec();

	if(CV_VOLATILE_DATA->cvTimer2Start > 0)
	{
		printf("%s: return %u,%u,%u \n",__FUNCTION__,time1,CV_VOLATILE_DATA->cvTimer2ElapsedTime,prevElapsedTime);	 	
		return ((time1 + CV_VOLATILE_DATA->cvTimer2ElapsedTime) - prevElapsedTime);
	}
	else
	{
		return 0;
	}
}

