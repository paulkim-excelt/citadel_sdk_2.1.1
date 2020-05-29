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
 * cvpbahandler.c:  CVAPI Preboot Authentication Handler
 */
/*
 * Revision History:
 *
 * 07/18/07 DPA Created.
*/
#include "cvmain.h"
#include "HID/brfd_api.h"
#include "console.h"
#include "qspi_flash.h"

extern const uint8_t dsa_default_p[];
extern const uint8_t dsa_default_q[];
extern const uint8_t dsa_default_g[];

#define CHIP_IS_5882_OR_LYNX (1)

cv_status
cv_bind (cv_handle cvHandle, cv_obj_value_hash *sharedSecret)
{
	/* This routine is used to establish a shared secret which is stored persistently in the CV.  This shared */
	/* secret is subsequently provided to the system BIOS in a secure way, in order to create a binding. */
	cv_status retVal = CV_SUCCESS;
	uint8_t *g, *p, *priv;
	uint32_t pubkey[DSA1024_KEY_LEN];
	uint32_t result_len;
    uint8_t cryptoHandleBuf[PHX_CRYPTO_LIB_HANDLE_SIZE] = {0};
	crypto_lib_handle *cryptoHandle = (crypto_lib_handle *)cryptoHandleBuf;
	uint8_t nonce[CV_NONCE_LEN];
	uint8_t hmacKey[SHA1_LEN];

	/* first compute Kdipub */
	phx_crypto_init(cryptoHandle);
	p = (uint8_t *)dsa_default_p;
	g = (uint8_t *)dsa_default_g;
	priv = NV_6TOTP_DATA->Kdi_priv;

	if ((phx_math_accelerate (cryptoHandle, PHX_CRYPTO_MATH_MODEXP,
				      (uint8_t *)p, DSA1024_KEY_LEN_BITS,
				      (uint8_t *)g, DSA1024_KEY_LEN_BITS,
				      priv, DSA1024_PRIV_LEN*8,
                      (uint8_t *)&pubkey[0], &result_len,
                      NULL, NULL)) != PHX_STATUS_OK)
	{
		retVal = CV_CRYPTO_FAILURE;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* now hash Kdipub, create a nonce, and do HMAC of nonce using Kdipub as key */
	if ((retVal = cvAuth((uint8_t *)&pubkey[0], result_len, NULL, (uint8_t *)&hmacKey[0], SHA1_LEN,
				NULL, 0, 0)) != CV_SUCCESS)
			goto err_exit;

	if ((retVal = cvRandom(CV_NONCE_LEN, (uint8_t *)&nonce[0])) != CV_SUCCESS)
		goto err_exit;

	if ((retVal = cvAuth((uint8_t *)&nonce[0], SHA1_LEN, (uint8_t *)&hmacKey[0], (uint8_t *)&CV_PERSISTENT_DATA->BIOSSharedSecret[0], SHA1_LEN,
				NULL, 0, 0)) != CV_SUCCESS)
		goto err_exit;

	CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_BIOS_SHARED_SECRET;

	/* update persistent data */
	if ((retVal = cvWritePersistentData(TRUE)) != CV_SUCCESS)
		goto err_exit;

	sharedSecret->hashType = CV_SHA1;
	memcpy((uint8_t *)&sharedSecret->hash[0], (uint8_t *)&CV_PERSISTENT_DATA->BIOSSharedSecret[0], SHA1_LEN);

err_exit:
	return retVal;
}

cv_status
cv_bind_challenge (cv_handle cvHandle, cv_obj_value_hash *bindChallenge, cv_obj_value_hash *bindResponse)
{
	/* This routine is used to generate a response to a challenge, using the BIOS shared secret that was previously */
	/* configured with cv_bind (an error is returned if no shared secret has been configured). */
	cv_status retVal = CV_SUCCESS;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_BIOS_SHARED_SECRET))
	{
		retVal = CV_NO_BIOS_SHARED_SECRET;
		goto err_exit;
	}
	/* validate input hash type */
	if (bindChallenge->hashType != CV_SHA1)
	{
		retVal = CV_INVALID_HASH_TYPE;
		goto err_exit;
	}
	/* now do HMAC of challenge using shared secret as key */
	if ((retVal = cvAuth((uint8_t *)&bindChallenge->hash, SHA1_LEN, (uint8_t *)&CV_PERSISTENT_DATA->BIOSSharedSecret[0],
			(uint8_t *)&bindResponse->hash, SHA1_LEN, NULL, 0, 0)) != CV_SUCCESS)
		goto err_exit;
	bindResponse->hashType = CV_SHA1;

err_exit:
	return retVal;;
}

cv_status
cv_identify_user (cv_handle cvHandle,
				uint32_t authListLength, cv_auth_list *pAuthList,
				uint32_t handleListLength, cv_obj_handle *handleList,
				cv_identify_type *pIdentifyType, uint32_t *pIdentifyLength,
				uint8_t *pIdentifyInfo)
{
	/* This routine is used to attempt to identify a user via one of the USH authentication devices */
	/* (fingerprint, smart card, contactless).  The mechanism used is to poll for each of the types of */
	/* authentication device and access the first one available.  If no suitable authentication information */
	/* is found within the timeout period configured in CV persistent data, a timeout error is returned. */
	cv_status retVal = CV_SUCCESS;
	uint32_t imageLength = 0, imageWidth;
	cv_obj_handle identifiedTemplate;
	cv_bool match;
	cvsc_card_type cardType;
	uint8_t userId[CV_MAX_USER_ID_LEN];
	cv_obj_properties objProperties;
	uint32_t userIdLen = CV_MAX_USER_ID_LEN;
	uint8_t pubKey[RSA2048_KEY_LEN];
	uint32_t pubKeyLen = RSA2048_KEY_LEN;
	uint8_t pubKeyHash[SHA1_LEN];
	uint32_t startTime, lastSCPoll = 0, lastRFPoll = 0;
#define SC_POLL_INTERVAL 1000 /* smart card poll interval in ms */
#define RF_POLL_INTERVAL 1000 /* smart card poll interval in ms */
	cv_contactless_identifier *cID = (cv_contactless_identifier *)pIdentifyInfo;
	uint32_t clIDLen = *pIdentifyLength - sizeof(cID->type);		/* this is just the length of the ID field */
	cv_nonce captureID;
	cv_bool doFPIdentify = FALSE;
	cv_bool fingerDetected = FALSE;
	cv_fp_control cvFpControl = 0;
	fp_status fpRetVal = FP_OK;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* check to see if PIN supplied in auth list */
	if (pAuthList != NULL)
	{
		memset(&objProperties,0,sizeof(cv_obj_properties));
		cvFindPINAuthEntry(pAuthList, &objProperties);
	}
	/* now set up loop to check all devices for identifictation presented by the user */
	/* turn off h/w finger detect, because we'll be polling here */
	if (!CHIP_IS_5882_OR_LYNX)
		cvUSHFpEnableFingerDetect(FALSE);
	/* set up timeout */
	/*get_ms_ticks(&startTime);*/
	
	/* Just try devices in this order once... fp, smartcard, fp, rfid. PBA will periodically call cv_identify_user */
	do {
		/* 1st check for FP device for finger detect (if at least one template handle provided) */
		if ((CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && (CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)
			&& handleListLength)
		{
			/* if 5882, do async FP capture here */
			if (CHIP_IS_5882_OR_LYNX)
			{
				/* see if a capture was started as a result of an OS-present request.  if so, cancel it */
				if ((CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS) && !(CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL))
				{
					cvFpCancel(0);
					CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
				}
				/* check to see if this is the first time in cv_identify_user after resume/restart.  if so, */
				/* clear any residual FP capture status condition */
				if (CV_VOLATILE_DATA->CVDeviceState & CV_FIRST_PASS_PBA)
				{
					CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
					CV_VOLATILE_DATA->CVDeviceState &= ~CV_FIRST_PASS_PBA;
				}
				/* check to see if capture in progress, if so, just continue to poll other devices */
				if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS))
				{
					printf("cv_identify_user: no capture in progress\n");
					/* check to see if there was a FP bad swipe.  if so, return error and let host restart capture */
					if (CV_VOLATILE_DATA->fpCaptureStatus != FP_OK)
					{
						printf("cv_identify_user: bad swipe detected 0x%x\n", CV_VOLATILE_DATA->fpCaptureStatus);
						CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
						retVal = CV_IDENTIFY_USER_FAILED;
						goto err_exit;
					}
					if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID)
					{
						/* check to see if persistence timer has timed out */
						if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
							/* timer has elapsed, invalidate FP image */
							CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
					}
					/* check to see if capture needs to be started */
					if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID))
					{
						printf("cv_identify_user: do async capture\n");
						retVal = cv_fingerprint_capture_start(cvHandle, 0, CV_CAP_CONTROL_DO_FE | CV_CAP_CONTROL_INTERNAL | CV_CAP_CONTROL_TERMINATE, &captureID,
							NULL, 0);
					} else {
						/* have a feature set, do identify */
						printf("cv_identify_user: has feature set\n");
						cvFpControl |= CV_ASYNC_FP_CAPTURE_ONLY;
						doFPIdentify = TRUE;
					}
				}

			} else {
				/* 5880 processing using sync FP capture */
        		fpRetVal = fpGrab((uint32_t *)NULL, (uint32_t *)NULL, PBA_FP_DETECT_TIMEOUT, 
        		 		NULL, NULL, FP_POLLING_PERIOD, FP_FINGER_DETECT);
				if (fpRetVal == FP_SENSOR_RESET_REQUIRED) {
					printf("cv_identify_user: ESD occurred during fpGrab, restart\n");
					fpSensorReset();
				} else if (fpRetVal == FP_OK) {
						doFPIdentify = TRUE;
						fingerDetected = TRUE;
				}
			}
			if (doFPIdentify)
			{
				printf("cv_identify_user: do identify\n");
				/* finger detected, need to do identify */
				if (CHIP_IS_5882_OR_LYNX) {	
					retVal = cvFROnChipIdentify(cvHandle, CV_PERSISTENT_DATA->FARval, cvFpControl, TRUE,
								0, NULL, handleListLength/sizeof(cv_obj_handle), handleList,
								0, NULL, &identifiedTemplate, fingerDetected, &match,
								NULL, 0);
				}
				else {
					retVal = cvUSHfpIdentify(cvHandle, CV_PERSISTENT_DATA->FARval, 0, 
								0, NULL, handleListLength/sizeof(cv_obj_handle), handleList,
								0, NULL, &identifiedTemplate, TRUE, &match,
								NULL, 0);
				}
				if (retVal == CV_FP_BAD_SWIPE) {
					/* new capture can be started (async FP case) */
					CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
					continue;
				} else if (retVal != CV_SUCCESS) {
					/* exit with failure */
					CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
					goto err_exit;
				}
				/* identify succeeded, but need to check if match found */
				if (!match)
				{
					/* no match, return error and indicate FP match was identify type */
					CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
					retVal = CV_IDENTIFY_USER_FAILED;
					*pIdentifyType = CV_ID_TYPE_TEMPLATE;
					*pIdentifyLength = 0;
					goto err_exit;
				}
				/* user identified, return template handle */
				if (*pIdentifyLength < sizeof(cv_obj_handle))
				{
					/* return length not sufficient, return error */
					*pIdentifyLength = sizeof(cv_obj_handle);
					retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
					goto err_exit;
				}
				*pIdentifyType = CV_ID_TYPE_TEMPLATE;
				*pIdentifyLength = sizeof(cv_obj_handle);
				memcpy(pIdentifyInfo, &identifiedTemplate, sizeof(cv_obj_handle));
				break;
			}
		}
		/* check to see if smart card present */
		if ((CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT) && (CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED))
			/*&& cvGetDeltaTime(lastSCPoll) >= SC_POLL_INTERVAL)*/
		{
			/*get_ms_ticks(&lastSCPoll);*/
			if (scIsCardPresent())
			{
				/* yes, determine type and read identifying info from card */
				*pIdentifyType = CV_ID_TYPE_USER_ID;
				if (scGetCardType(&cardType) != CVSC_OK)
				{
					/* detection failed, return error */
					retVal = CV_IDENTIFY_USER_FAILED;
					*pIdentifyLength = 0;
					goto err_exit;
				}
				/* succeeded.  read indicated identifier from card */
				if (cardType == CVSC_DELL_JAVA_CARD)
				{
					/* read user ID from card.  shouldn't be PIN protected */
					if (scGetUserID(&userIdLen, (uint8_t *)&userId[0]) != CVSC_OK)
					{
						/* read failed, return error */
						retVal = CV_IDENTIFY_USER_FAILED;
						*pIdentifyLength = 0;
						goto err_exit;
					}
					/* succeeded.  save hash of user ID */
					if (*pIdentifyLength < userIdLen)
					{
						/* return length not sufficient, return error */
						*pIdentifyLength = SHA1_LEN;
						retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
						goto err_exit;
					}
					if ((retVal = cvAuth((uint8_t *)&userId[0], userIdLen, NULL, (uint8_t *)pIdentifyInfo, SHA1_LEN, NULL, 0, 0)) != CV_SUCCESS)
						goto err_exit;
					*pIdentifyLength = SHA1_LEN;
					break;
				} else if (cardType == CVSC_PKI_CARD) {
					/* PKI card.  read pubkey and hash */
					if (scGetPubKey(&pubKeyLen, (uint8_t *)&pubKey[0]) != CVSC_OK)
					{
						/* read failed, return error */
						retVal = CV_IDENTIFY_USER_FAILED;
						*pIdentifyLength = 0;
						goto err_exit;
					}
					/* succeeded.  do hash of pubkey and return */
					if ((retVal = cvAuth((uint8_t *)&pubKey[0], pubKeyLen, NULL, (uint8_t *)&pubKeyHash[0], SHA1_LEN,
								NULL, 0, 0)) != CV_SUCCESS)
							goto err_exit;
					if (*pIdentifyLength < SHA1_LEN)
					{
						/* return length not sufficient, return error */
						*pIdentifyLength = SHA1_LEN;
						retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
						goto err_exit;
					}
					*pIdentifyLength = SHA1_LEN;
					memcpy(pIdentifyInfo, (uint8_t *)&pubKeyHash, SHA1_LEN);
					break;
				} else if (cardType == CVSC_CAC_CARD || cardType == CVSC_PIV_CARD) {
					/* CAC or PIV card.  read cert or CHUID and hash */
					if (cvReadAndHashCACPIVCardID(cardType, pIdentifyInfo) != CVSC_OK)
					{
						/* read failed, return error */
						retVal = CV_IDENTIFY_USER_FAILED;
						*pIdentifyLength = 0;
						goto err_exit;
					}
					*pIdentifyLength = SHA1_LEN;
					break;
				} else {
					/* error, unidentified type */
					retVal = CV_IDENTIFY_USER_FAILED;
					*pIdentifyLength = 0;
					goto err_exit;
				}
			}
		}
		
		/* check FP device for finger detect (if at least one template handle provided) */
		if ((CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && (CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)
			&& handleListLength && !CHIP_IS_5882_OR_LYNX)
		{
			/* check to make sure no async FP capture is active */
			if (!isFPImageBufAvail())
			{
				retVal = CV_FP_DEVICE_BUSY;
				goto err_exit;
			}

       		if ((fpRetVal = fpGrab((uint32_t *)NULL, (uint32_t *)NULL, PBA_FP_DETECT_TIMEOUT, 
        		 	NULL, NULL, FP_POLLING_PERIOD, FP_FINGER_DETECT)) == FP_OK) {
				
        	/* finger detected, need to do identify */
				if (CHIP_IS_5882_OR_LYNX) {	
					retVal = cvFROnChipIdentify(cvHandle, CV_PERSISTENT_DATA->FARval, 0, TRUE,
								0, NULL, handleListLength/sizeof(cv_obj_handle), handleList,
								0, NULL, &identifiedTemplate, TRUE, &match,
								NULL, 0);
				}
				else {
					retVal = cvUSHfpIdentify(cvHandle, CV_PERSISTENT_DATA->FARval, 0,
								0, NULL, handleListLength/sizeof(cv_obj_handle), handleList,
								0, NULL, &identifiedTemplate, TRUE, &match,
								NULL, 0);
				}
				if (retVal == CV_FP_BAD_SWIPE)
					continue;
				else if (retVal != CV_SUCCESS)
					goto err_exit;
				/* identify succeeded, but need to check if match found */
				if (!match)
				{
					/* no match, return error and indicate FP match was identify type */
					retVal = CV_IDENTIFY_USER_FAILED;
					*pIdentifyType = CV_ID_TYPE_TEMPLATE;
					*pIdentifyLength = 0;
					goto err_exit;
				}
				/* user identified, return template handle */
				if (*pIdentifyLength < sizeof(cv_obj_handle))
				{
					/* return length not sufficient, return error */
					*pIdentifyLength = sizeof(cv_obj_handle);
					retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
					goto err_exit;
				}
				*pIdentifyType = CV_ID_TYPE_TEMPLATE;
				*pIdentifyLength = sizeof(cv_obj_handle);
				memcpy(pIdentifyInfo, &identifiedTemplate, sizeof(cv_obj_handle));
				break;
			} else if (fpRetVal == FP_SENSOR_RESET_REQUIRED) {
                printf("cv_identify_user: ESD occurred during fpGrab, restart\n");
				fpSensorReset();
			}
		}
		
		/* now check contactless */
		if ((CV_PERSISTENT_DATA->deviceEnables & CV_RF_PRESENT) && (CV_PERSISTENT_DATA->deviceEnables & CV_RF_ENABLED))
			/*&& cvGetDeltaTime(lastRFPoll) >= RF_POLL_INTERVAL)*/
		{
			/*get_ms_ticks(&lastRFPoll);*/
			if (rfIsCardPresent())
			{
				retVal = cvReadContactlessID(CV_POST_PROCESS_SHA1, objProperties.PINLen, objProperties.PIN, &cID->type, &clIDLen, &cID->id[0]);
				if (retVal == CV_CONTACTLESS_COM_ERR) {
					retVal = CV_IDENTIFY_USER_TIMEOUT;
					goto err_exit;
				}
				if (retVal != CV_SUCCESS)
					goto err_exit;
				/* make sure buffer provided is at least large enough for contactless type */
				if (*pIdentifyLength < (sizeof(cID->type) + clIDLen))
				{
					retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
					goto err_exit;
				}
				*pIdentifyType = CV_ID_TYPE_USER_ID;
				*pIdentifyLength = clIDLen + sizeof(cID->type);
				break;
			}
		}
		retVal = CV_IDENTIFY_USER_TIMEOUT;
			goto err_exit;
			
	} while (TRUE);

err_exit:
	/* turn h/w finger detect back on */
	/* cvUSHfpEnableHWFingerDetect(TRUE);*/
	return retVal;
}

cv_status
cv_access_PBA_code_flash (cv_handle cvHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			uint32_t offset, uint32_t *pDataLength, uint8_t *pData,
			cv_bool read, cv_callback *callback, cv_callback_ctx context)
{
	/* This routine is provided to allow code that is used in PBA to be written/read to/from flash.  */
	/* To access flash in this case, the ushx_flash_read_write routine in the 5880 SDK will be invoked */
	/* with the appropriate identifier for the PBA code, in order to access the appropriate region of flash. */
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	cv_obj_auth_flags authFlags;
	cv_input_parameters inputParameters;
	uint32_t interval, minInterval;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* don't allow flash update more often that every 5 minutes, to prevent flash burnout */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_ANTIHAMMERING_ACTIVE) && !read)
	{
		interval = cvGetDeltaTime(CV_VOLATILE_DATA->timeLastPBAImageWrite);
		/* workaround to account for Atmel flash requiring 2 writes for each page write */
		if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
			minInterval = 2*CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		else
			minInterval = CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		/* for first time through (ie, last time is 0) need to init start time */
		if ((interval < minInterval) && (CV_VOLATILE_DATA->timeLastPBAImageWrite != 0))
		{
			if (interval > CV_PBA_CODE_UPDATE_DURATION)
			{
				/* here if PBA code download should have completed.  if not, return antihammering status */
				retVal = CV_ANTIHAMMERING_PROTECTION;
				goto err_exit;
			}
		} else {
			/* here if initiating sequence of commands to write PBA flash.  initialize start time */
			get_ms_ticks(&CV_VOLATILE_DATA->timeLastPBAImageWrite);
		}
	}

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 6, 
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		sizeof(uint32_t), &offset,
		sizeof(uint32_t), pDataLength, *pDataLength, pData, sizeof(cv_bool), &read, 0, NULL, 0, NULL, 0, NULL, 0, NULL);

	/* don't include pData as input if read is TRUE.  also, validate read block size */
	if (read) {
		inputParameters.paramLenPtr[5].paramLen = 0;
		if (*pDataLength > MAX_CONFIG_READWRITE_BLOCK_SIZE)
		{
			/* block too large */
			*pDataLength = MAX_CONFIG_READWRITE_BLOCK_SIZE;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
	}

	/* for write to flash, must have CV admin auth */
	if (!read)
	{
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
			if ((retVal = cvWritePersistentData(TRUE)) != CV_SUCCESS)
				goto err_exit;
		retVal = CV_SUCCESS;

		/* auth succeeded, but need to make sure correct auth granted (write auth, since persistent data will be changed) */
		if (!((authFlags & CV_ADMIN_PBA_BIOS_UPDATE) || (authFlags & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV))))
		{
			/* correct auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}
	}

	/* auth ok, do read/write to flash here */
	if ((ushx_flash_read_write(CV_DPPBA_FLASH_BLOCK_CUST_ID, offset, *pDataLength, pData, (read) ? 0 : 1)) != PHX_STATUS_OK)
	{
		/* read/write failure, translate to CVAPI return code */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}

err_exit:
	return retVal;
}

