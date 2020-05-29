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
 * cvrfhandler.c:  CVAPI RF Device Handler
 */
/*
 * Revision History:
 *
 * 11/20/07 DPA Created.
*/
#include "cvmain.h"
#include "isr.h"
#include "sched_cmd.h"
#include "phx_upgrade.h"
#include "tmr.h"
#include "usbhost/usbhost_glb.h"
#include "usbhost/usbhost_mem.h"
#include "console.h"
#include "qspi_flash.h"
#include "cvfpstore.h"
#include "HID/brfd_api.h"
#include "rfid/rfid_hw_api.h"
#include "board.h"
#include "ush_flash_conf.h"
#include "ushx_api.h"
#include "easyzlib.h"
#include "../nci/nci.h"
#include "../nci/nci_spi.h"
#include "nfpmemblock.h"
#include "fp_spi_enum.h"
#include "../dmu/crmu_apis.h"

#ifdef PHX_POA_BASIC
#include "../pwr/poa.h"
#include "../smbus/smbusdriver.h"
#endif

#undef ENABLE_RFID
#define ENABLE_RFID		1

extern const uint32_t cust_flash_block_size[];
extern const uint32_t cust_flash_block_size[];

cv_status cv_clear_flash_data(uint32_t flash_id);

cv_bool
isRadioEnabled(uint32_t bCheckOverride)
{
	/* this function returns the status of the radio */
	cv_bool radioEnabled = FALSE;

	/* if radio enabled and not in CV-only radio mode, return true.  the override allows the radio to be temporarily */
	/* enabled */
	if ((CV_PERSISTENT_DATA->deviceEnables & CV_RF_ENABLED) && (!(IS_CV_ONLY_RADIO_MODE())
		|| ((bCheckOverride == TRUE) && (CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_OVERRIDE))))
		radioEnabled = TRUE;
	return radioEnabled;
}

cv_bool
isRadioPresent(void)
{
	/* this function returns the presence status of the radio */
	return ((CV_PERSISTENT_DATA->deviceEnables & CV_RF_PRESENT) ? TRUE : FALSE);
}

cv_bool
isNotCvOnlyRadioMode(void)
{
	return ( isRadioPresent() && !(IS_CV_ONLY_RADIO_MODE())) ;
}

uint32_t
cvGetRFIDParamsID(void)
{
	/* this function returns the RFID params ID */
	cv_rfid_params_flash_blob paramHeader;

	if ((ushx_flash_read_write(CV_BRCM_RFID_PARAMS_CUST_ID, 0, sizeof(paramHeader), (uint8_t *)&paramHeader, 0)) != PHX_STATUS_OK)
		/* flash read failure */
		return 0;
	/* check magic number */
	if (memcmp((uint8_t *)&paramHeader.magicNumber,"rFiD", sizeof(paramHeader.magicNumber)))
		/* not valid */
		return 0;

	return paramHeader.paramsID;
}

cv_status
cvGetRFIDParams(uint32_t *paramsLength, uint8_t *params)
{
	/* this routine returns the RFID parameters */
	cv_status retVal = CV_SUCCESS;
	cv_rfid_params_flash_blob paramHeader;

	/* first read header and validate */
	if ((ushx_flash_read_write(CV_BRCM_RFID_PARAMS_CUST_ID, 0, sizeof(paramHeader), (uint8_t *)&paramHeader, 0)) != PHX_STATUS_OK)
	{
		/* flash read failure */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}
	/* check magic number */
	if (memcmp((uint8_t *)&paramHeader.magicNumber,"rFiD", sizeof(paramHeader.magicNumber)))
	{
		/* not valid */
		retVal = CV_INVALID_RFID_PARAMETERS;
		goto err_exit;
	}
	/* check length provided */
	if (*paramsLength < paramHeader.length)
	{
		retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
		goto err_exit;
	}
	/* now read in params */
	if ((ushx_flash_read_write(CV_BRCM_RFID_PARAMS_CUST_ID, sizeof(paramHeader), *paramsLength, params, 0)) != PHX_STATUS_OK)
	{
		/* flash read failure */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}
	*paramsLength = paramHeader.length;

err_exit:
	return retVal;
}


cv_bool
isSCEnabled(void)
{
	return (CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED) ? TRUE : FALSE;
}

cv_bool
isSCPresent(void)
{
	return (CV_PERSISTENT_DATA->deviceEnables & CV_SC_PRESENT) ? TRUE : FALSE;
}


cv_bool
isNFCEnabled(void)
{
	return (CV_PERSISTENT_DATA->deviceEnables & CV_NFC_ENABLED) ? TRUE : FALSE;
}

cv_bool
isNFCPresent(void)
{
	return (CV_PERSISTENT_DATA->deviceEnables & CV_NFC_PRESENT) ? TRUE : FALSE;
}


cv_bool
isFPEnabled(void)
{
	return (CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED) ? TRUE : FALSE;
}

cv_status
cvReadAndHashCACPIVCardID(cv_cac_piv_card_type cardType, uint8_t *hash)
{
	/* this routine reads and hashs the cert or CHUID from CAC or PIV card, respectively, and returns SHA1 hash */
	uint8_t *buf;
	uint8_t bufRaw[CV_MAX_CAC_CERT_SIZE];
	uint8_t bufCHUID[CV_MAX_CHUID_SIZE];
	uint32_t len;
	cv_status retVal = CV_SUCCESS;
	long ezLen;
	uint8_t *bufPtr = NULL;

	if (cardType == CVSC_CAC_CARD)
	{
		bufPtr = &bufRaw[0];
		len = sizeof(bufRaw);
		if (scGetCACCardCert(&len, &bufRaw[0]) != CVSC_OK)
			return CV_IDENTIFY_USER_FAILED;
		/* now decompress, if required */
		if (bufRaw[0] == 0x1)
		{
			/* not allowed if FP image buf not available.  s/b ok, because this routine only called in PBA */
			if (!isFPImageBufAvail())
				return CV_FP_DEVICE_BUSY;
			init_heap(FP_MATCH_HEAP_SIZE, FP_MATCH_HEAP_PTR);
			ezLen = CV_MAX_CHUID_SIZE;
			/* note that CV_MAX_CHUID_SIZE is > 3*CV_MAX_CAC_CERT_SIZE and 3X is max compression ratio for certs */
			buf = (uint8_t *)pvPortMalloc_in_heap(CV_MAX_CHUID_SIZE, FP_MATCH_HEAP_PTR);
			if (ezuncompress( &buf[0], &ezLen, &bufRaw[1], len - 1 ) != 0)
				return CV_IDENTIFY_USER_FAILED;
			len = ezLen;
			bufPtr = buf;
		} else {
			/* point past decompress flag */
			bufPtr++;
			len--;
		}
	} else if (cardType == CVSC_PIV_CARD) {
		len = sizeof(bufCHUID);
		if (scGetPIVCardCHUID(&len, &bufCHUID[0]) != CVSC_OK)
			return CV_IDENTIFY_USER_FAILED;
		bufPtr = &bufCHUID[0];
	}
	retVal = cvAuth(bufPtr, len, NULL, hash, SHA1_LEN, NULL, 0, 0);

	return retVal;
}

cv_status
cvReadContactlessID(cv_post_process_piv_chuid postProcess, uint32_t PINLen, uint8_t *PIN, cv_contactless_type *type, uint32_t *len, uint8_t *id)
{
	/* this routine reads the ID from a contactless card */
	cv_status retVal = CV_SUCCESS;
	uint8_t buf[CV_MAX_CHUID_SIZE];		/* this is the maximum size for a PIV card CHUID field.  it's the maximum of all contactless id lengths */
	uint32_t bufLen = sizeof(buf);
#define PIV_FASCN_TAG 0x30
#define PIV_FASCN_ID_LEN 14 /* length of first 3 fields of FASC-N */
	typedef struct tlv_short_e {
		uint8	tag;
		uint8	length;
	} tlv_short;
	typedef struct tlv_long_e {
		uint8	tag;
		uint8	constFF;
		uint16	length;
	} tlv_long;
	tlv_long *tlv;
	uint8_t *bufPtr, *bufEnd;
	uint8_t hash1[SHA1_LEN];
	uint8_t hash2[SHA256_LEN];
	cv_bool useSHA256 = FALSE;

	if ((retVal = rfGetHIDCredential(PINLen, PIN, &bufLen, &buf[0], type)) != CV_SUCCESS)
		goto err_exit;
	/* here if read value ok.  need to validate length before copying */
	/* if PIV card, need to post-process to just get hash of CHUID */
	if (*type == CV_CONTACTLESS_TYPE_PIV)
	{
		if (postProcess == CV_POST_PROCESS_FASCN)
		{
			/* search CHUID record for FASC-N and return first 3 fields */
			bufPtr = &buf[0];
			bufEnd = bufPtr + bufLen;
			retVal = CV_INVALID_CONTACTLESS_CARD_TYPE;
			do {
				/* get tag and check */
				tlv = (tlv_long *)bufPtr;
				if (tlv->tag == PIV_FASCN_TAG)
				{
					/* this is the FASC-N */
					/* check to make sure input length is sufficient */
					if (*len < PIV_FASCN_ID_LEN)
					{
						retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
						goto err_exit;
					}
					*len = PIV_FASCN_ID_LEN;
					memcpy(id, bufPtr, PIV_FASCN_ID_LEN);
					retVal = CV_SUCCESS;
					break;
				}
				if (tlv->constFF == 0xff)
					bufPtr += sizeof(tlv_long) + tlv->length;
				else
					bufPtr += sizeof(tlv_short) + ((tlv_short *)tlv)->length;
			} while (bufPtr <= bufEnd);
		} else if (postProcess == CV_POST_PROCESS_SHA1) {
			if ((retVal = cvAuth(&buf[0], bufLen, NULL, &hash1[0], sizeof(hash1), NULL, 0, 0)) != CV_SUCCESS)
				goto err_exit;
			*len = sizeof(hash1);
			memcpy(id, &hash1[0], sizeof(hash1));
			/* now do SHA256 hash to cache for authentication */ 
			useSHA256 = TRUE;
			if ((retVal = cvAuth(&buf[0], bufLen, NULL, &hash2[0], sizeof(hash2), NULL, 0, 0)) != CV_SUCCESS)
				goto err_exit;
		} else if (postProcess == CV_POST_PROCESS_SHA256) {
			if ((retVal = cvAuth(&buf[0], bufLen, NULL, &hash2[0], sizeof(hash2), NULL, 0, 0)) != CV_SUCCESS)
				goto err_exit;
			*len = sizeof(hash2);
			memcpy(id, &hash2[0], sizeof(hash2));
		} else
			/* unrecognized postProcessing field, just return 0 length */
			*len = 0;
	} else {
		/* for all other types, just check that output length is sufficient */
		if (*len < bufLen)
		{
			*len = bufLen;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*len = bufLen;
		memcpy(id, &buf[0], bufLen);
	}
	/* now cache credential for authorization */
	if (*len <= CV_MAX_CACHED_CREDENTIAL_LENGTH)
	{
		if (useSHA256)
			memcpy(CV_VOLATILE_DATA->identifyUserCredential, &hash2[0], sizeof(hash2));
		else
			memcpy(CV_VOLATILE_DATA->identifyUserCredential, id, *len);
		CV_VOLATILE_DATA->credentialType = *type;
		CV_VOLATILE_DATA->HIDCredentialPtr = &CV_VOLATILE_DATA->identifyUserCredential[0];
	}

err_exit:
	return retVal;
}

#define MAX_TIME_BETWEEN_WINBIOIDENTIFY_AND_CONTROL_UNIT_CALL_IN_SECONDS 5
#define MAX_VALUE_FOR_OBJECT_ACCESS_TIMEOUT_IN_SECONDS 300 // 5 minutes

cv_status
cv_wbf_get_session_id (uint32_t timeoutForObjectAccess, cv_wbf_getsessionid *getSessionID)
{
	int nSecondsPassed = 0;

	printf("cv_wbf_get_session_id\n");

	/* check that call is allowed */

	if (CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess)
	{
		printf("Failing control unit API. The control unit API has been called already for this finger identification.\n");
		return CV_AUTH_FAIL;
	}

#ifdef USE_CRMU_TIMER_FOR_TIME_BETWEEN_MATCH_AND_CONTROLUNIT_API /* The CRMU timer to keep tracks of time including time spent in selective suspend */

	if (CV_VOLATILE_DATA->wbfProtectData.didStartCRMUTimer == 0)
	{
		printf("Failing control unit API. Timer did not start (WinBioIdentify/WinBioVerify was not called).\n");
		return CV_AUTH_FAIL;
	}

	nSecondsPassed = cv_get_timer2_elapsed_time(CV_VOLATILE_DATA->wbfProtectData.CRMUElapsedTime);
	if (nSecondsPassed > MAX_TIME_BETWEEN_WINBIOIDENTIFY_AND_CONTROL_UNIT_CALL_IN_SECONDS)
	{
		printf("Failing control unit API. Too much time passed (%d seconds) since the last call to WinBioIdentify/WinBioVerify.\n", nSecondsPassed);
		return CV_AUTH_FAIL;
	}

#else /* If MAX_TIME_BETWEEN_WINBIOIDENTIFY_AND_CONTROL_UNIT_CALL_IN_SECONDS is small then it's ok to use a simple timer */

	if (!CV_VOLATILE_DATA->wbfProtectData.startTimeCountForSimpleTimer)
	{
		printf("cvAuthFingerprint Timer did not start (did not call control unit API).\n");
		return CV_AUTH_FAIL;
	}

	uint32_t endTime;
	get_ms_ticks(&endTime);
	nSecondsPassed = cvGetDeltaTime(CV_VOLATILE_DATA->wbfProtectData.startTimeCountForSimpleTimer) / 1000;
	if (nSecondsPassed > MAX_TIME_BETWEEN_WINBIOIDENTIFY_AND_CONTROL_UNIT_CALL_IN_SECONDS)
	{
		printf("Failing control unit API. Too much time passed (at least %d seconds) since the last call to WinBioIdentify/WinBioVerify.\n", nSecondsPassed);
		return CV_AUTH_FAIL;
	}

#endif


	printf("timeoutForObjectAccess: %d\n", timeoutForObjectAccess);

	if (timeoutForObjectAccess > MAX_VALUE_FOR_OBJECT_ACCESS_TIMEOUT_IN_SECONDS)
	{
		printf("Failing control unit API. Bad input parameter - timeout for object access is too large.\n");
		return CV_AUTH_FAIL;
	}

	/* save the timeout parameter that was passed in the API call to volatile storage */
	CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess = timeoutForObjectAccess;
	
	/* return output to the caller */
	getSessionID->template_id_of_identified_fingerprint = CV_VOLATILE_DATA->wbfProtectData.template_id_of_identified_fingerprint;
	memcpy(getSessionID->sessionID, CV_VOLATILE_DATA->wbfProtectData.sessionID, WBF_CONTROL_UNIT_SESSION_ID_SIZE);

	/* restart the timer to count the time for object access now */
	cvStartTimerForWBFObjectProtection();

	printf("cv_wbf_get_session_id finished\n");

	return CV_SUCCESS;
}

cv_status
cv_read_contactless_card_id(cv_handle cvHandle, uint32_t authListLength, cv_auth_list *pAuthList,
					cv_contactless_type *type, uint32_t *idLen, uint8_t *idValue,
					cv_callback *callback, cv_callback_ctx context)
{
	/* this routine reads the ID value from a contactless card and returns the value and type */
	cv_obj_properties objProperties;
	cv_status retVal = CV_SUCCESS;
	const uint32_t WAIT_FOR_INSERTION_WITH_NFC_TIMEOUT_MS = 3000; 

	/* check to make sure radio present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_RF_PRESENT)) {
		retVal = CV_RADIO_NOT_PRESENT;
		goto err_exit;
	}
	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* first check to see if there's a PIN value in the auth list */
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	cvFindPINAuthEntry(pAuthList, &objProperties);
	
	if (isNFCEnabled() && pnci_mem_block->bNFCDriverIsAvailableOnTheSystem)
	{
		if (!pnci_mem_block->bNFCChipPowerOn)
		{
			printf("NFC available but power is off\n");
			return CV_RADIO_NOT_PRESENT;
		}
	}

	/* now prompt for card, if necessary */
	if (isNFCPresent())
	{
		if ((retVal = cvWaitInsertion(callback, context, CV_PROMPT_FOR_CONTACTLESS, WAIT_FOR_INSERTION_WITH_NFC_TIMEOUT_MS)) != CV_SUCCESS)
			goto err_exit;
	}
	else
	{
		if ((retVal = cvWaitInsertion(callback, context, CV_PROMPT_FOR_CONTACTLESS, USER_PROMPT_TIMEOUT)) != CV_SUCCESS)
			goto err_exit;
	}

	/* read value from card */
	if ((retVal = cvReadContactlessID(CV_POST_PROCESS_FASCN, objProperties.PINLen, objProperties.PIN, type, idLen, idValue)) != CV_SUCCESS)
		goto err_exit;

	/* check to see if PIN required but not supplied */
	if (retVal == CV_PROMPT_PIN_AND_CONTACTLESS)
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
	
	/* Removed the below dead code as per the Coverity */
	/*else if (retVal != CV_SUCCESS)
		goto err_exit;
	*/

err_exit:
	return retVal;
}

cv_status
cv_set_nfc_max_power(void)
{
    cv_status ret = CV_SUCCESS;

    /* Disable the ISR */
    nci_disable_isr();

    /* Initialize NFCC if not already done */
    nfc_initialize();

    /* Keey NFCC awake */
    nci_ctrl_nfc_wake(1);

    nci_reset(0);
    nci_init(0);

    printf("Set Max Power and start discovery\n");
    if (nci_set_maxpowerlevel()
        || nci_discovery_map(0)
        || nci_discover(0))
    {
        ret = CV_FAILURE;
    }

    /* Re-enable the ISR */
    nci_enable_isr();

    printf("Done, ret=%d\n", ret);
    return ret;
}

cv_status
cv_read_nfc_card_uuid(cv_handle cvHandle, uint32_t authListLength, cv_auth_list *pAuthList,
					uint32_t *type, uint32_t *uuidLen, uint8_t *uuidValue,
					cv_callback *callback, cv_callback_ctx context)
{
    int fail = 0;
    cv_status ret = CV_SUCCESS;
    int ticks = 0;

    printf("cv_read_nfc_card_uuid\n");

    /* Disable the ISR */
    nci_disable_isr();

    /* Initialize NFCC if not already done */
    nfc_initialize();

    /* Keey NFCC awake */
    nci_ctrl_nfc_wake(1);

    nci_reset(0);
    nci_init(0);

    printf("Start Discovery\n");
    fail = nci_discovery_map(0);
    if (!fail) {
        nci_discover(0);

        /* Wait 30 seconds for a notification */
        while (nci_spi_int_state() != 0) {
            yield_and_delay(100);
            if (ticks++ >= 300)     /* 30 seconds */
                break;
        }
        if (nci_spi_int_state() != 0) {
            ret = CV_IDENTIFY_USER_TIMEOUT;
        }
        else {
            printf("cv_read_nfc_card_uuid Reading card\n");
            uint8_t rsp[255];
            uint16_t rlen = sizeof(rsp);
            nci_notification(rsp, &rlen);
            if (nci_parse_notification(rsp, rlen, type, uuidLen, uuidValue) != RF_INTF_ACTIVATED_NTF) {
                fail = 1;
            }
        }
    }

    nci_reset(0);

    /* Re-enable the ISR */
    nci_enable_isr();

    if (fail) {
        printf("cv_read_nfc_card_uuid End - Fail\n");
        return CV_FAILURE;
    } else {
        if (ret == 0) {
            printf("cv_read_nfc_card_uuid End\n");
        } else {
            printf("cv_read_nfc_card_uuid End Fail(%d)\n", ret);
        }
        return ret;
    }
}

cv_status
cv_enable_radio(cv_handle cvHandle, cv_bool radioEnable)
{
	return CV_FEATURE_NOT_AVAILABLE;
}


cv_status
cv_set_EC_presence(cv_handle cvHandle, cv_bool ecEnable)
{
	/* this routine configures the Auto pwr save time */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;

	/* check to see there is change in configuration in flash*/
	printf("CV: cv_set_EC_presence %x and set= %d \n",CV_PERSISTENT_DATA->deviceEnables,ecEnable);
	if (ecEnable == TRUE) 
	{
		if(!(CV_PERSISTENT_DATA->deviceEnables &  CV_EC_IS_PRESENT))
		{
			/* Write the new value */
			CV_PERSISTENT_DATA->deviceEnables |= CV_EC_IS_PRESENT;
			CV_PERSISTENT_DATA->deviceEnables &=~CV_EC_IS_NOT_PRESENT;
			writePersistentData = TRUE;
		}
	} 
	else
	{
		if(!(CV_PERSISTENT_DATA->deviceEnables &  CV_EC_IS_NOT_PRESENT))
		{
			/* Write the new value */
			CV_PERSISTENT_DATA->deviceEnables |= CV_EC_IS_NOT_PRESENT;
			CV_PERSISTENT_DATA->deviceEnables &=~CV_EC_IS_PRESENT;
			writePersistentData = TRUE;
		}
	}
	
	/* if enable state changed, write persistent data */
	if (writePersistentData) 
	{
		retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
		retVal |= cvWritePersistentData(TRUE);
	}
	return retVal;
}



cv_status
cv_enable_smartcard(cv_handle cvHandle, cv_bool scEnable)
{
	/* this routine enables/disables the smart card */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;
	if (scEnable)
	{
		/* check to see if sc enabled, if not, enable */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED))
		{
			/* not enabled, enable here */
			CV_PERSISTENT_DATA->deviceEnables |= CV_SC_ENABLED;

			writePersistentData = TRUE;
		}

	} else {
		/* check to see if sc disabled, if not, disable */
		if (CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)
		{
			/* not disabled, disable here */
			CV_PERSISTENT_DATA->deviceEnables &= ~CV_SC_ENABLED;

			writePersistentData = TRUE;
		}

	}
	/* if enable state changed, write persistent data */
	if (writePersistentData) {
		retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
		retVal |= cvWritePersistentData(TRUE);
	}
	return retVal;
}

cv_status
cv_enable_nfc(cv_handle cvHandle, cv_bool nfcEnable)
{
	/* this routine enables/disables the NFC endpoing */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;
	int nfc_presence = 0;
	

	if(nfcEnable && !(CV_PERSISTENT_DATA->deviceEnables & CV_NFC_PRESENT)) {
		printf("CV: cv_enable_nfc Cannot enable. NFC not present\n");
		return CV_NFC_NOT_PRESENT;
	}

	printf("CV: cv_enable_nfc nfcEnable: %d\n", nfcEnable);

	if (nfcEnable)
	{
		/* check to see if radio enabled, if not, enable */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_NFC_ENABLED))
		{
			printf("CV: cv_enable_nfc NFC disabled. Enabling NFC\n");
			/* not enabled, enable here */
			CV_PERSISTENT_DATA->deviceEnables |= CV_NFC_ENABLED;
			writePersistentData = TRUE;
		}
	} else {
		/* check to see if radio disabled, if not, disable */
		if (CV_PERSISTENT_DATA->deviceEnables & CV_NFC_ENABLED)
		{
			printf("CV: cv_enable_nfc NFC enabled. Disabling NFC\n");
			/* not disabled, disable here */
			CV_PERSISTENT_DATA->deviceEnables &= ~CV_NFC_ENABLED;
			writePersistentData = TRUE;
		}
	}

	/* if enable state changed, write persistent data */
	if (writePersistentData) {
		retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
		retVal |= cvWritePersistentData(TRUE);
	}
	return retVal;
}

cv_status
cv_set_t4_cards_routing(cv_handle cvHandle, uint32_t routeCard)
{
	cv_status retVal = CV_SUCCESS;

    routeCard &=  CV_ROUTE_MASK; /* only care about certain bits */
	CV_PERSISTENT_DATA->deviceEnables &= ~(CV_ROUTE_MASK); /* First reset */
	CV_PERSISTENT_DATA->deviceEnables |= routeCard;  /* Next set */

	printf("%s: routecard =  %x \n",__FUNCTION__,CV_PERSISTENT_DATA->deviceEnables);
	
	retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
	retVal |= cvWritePersistentData(TRUE);
		
	return retVal;
}

cv_status
cv_enable_fingerprint(cv_handle cvHandle, cv_bool fpEnable)
{
	/* this routine enables/disables the RF radio */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;

	if (fpEnable)
	{
		/* check to see if radio enabled, if not, enable */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED))
		{
			/* not enabled, enable here */
			CV_PERSISTENT_DATA->deviceEnables |= CV_FP_ENABLED;
			writePersistentData = TRUE;
		}
	} else {
		/* check to see if radio disabled, if not, disable */
		if (CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)
		{
			/* not disabled, disable here */
			CV_PERSISTENT_DATA->deviceEnables &= ~CV_FP_ENABLED;
			writePersistentData = TRUE;
		}
	}
	/* if enable state changed, write persistent data */
	if (writePersistentData) {
		retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
		retVal |= cvWritePersistentData(TRUE);
	}
	return retVal;
}

cv_status
cv_enable_and_lock_radio(cv_handle cvHandle, cv_bool radioEnable)
{
	return CV_FEATURE_NOT_AVAILABLE;
}

cv_status
cv_set_radio_present(cv_handle cvHandle, cv_bool radioPresent)
{
	return CV_FEATURE_NOT_AVAILABLE;
}

cv_status
cv_set_smartcard_present(cv_handle cvHandle, cv_bool smartcardPresent)
{
	return CV_FEATURE_NOT_AVAILABLE;
}

cv_status
cv_manage_poa(cv_handle cvHandle, cv_bool POAEnable, uint32_t inactivityTimeout, uint32_t maxFailedMatches, cv_bool updatePersistentData)
{
 #ifndef PHX_POA_BASIC
    printf("cv_manage_poa dummy\n");
    printf("POAEnable: %d\n", POAEnable);
    printf("inactivityTimeout: %d\n", inactivityTimeout);
    printf("maxFailedMatches: %d\n", maxFailedMatches);
    return CV_SUCCESS;
 #else
    cv_status retVal = CV_SUCCESS;
    cv_bool writePersistentData = FALSE;
    uint8_t cmd[2];
    uint8_t status=0xff;
    uint8_t len; 

	printf("%s:POAEnable: %d %d,Timeout: %d %d \n",__FUNCTION__,POAEnable, CV_PERSISTENT_DATA->poa_enable,
		inactivityTimeout, CV_PERSISTENT_DATA->poa_timeout);
#ifdef POA_MAX_FAILED_MATCHES   // Disable it 
	printf("\tmaxFailedMatches: %d %d\n", maxFailedMatches, CV_PERSISTENT_DATA->poa_max_failedmatch );
#endif
   /* if the FP sensor is present */

    if ( updatePersistentData == TRUE && !(VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_SENSOR_FOUND))
    {
        printf("CV: No FP sensor. Failed to change the POA mode.\n");
        return CV_FP_NOT_PRESENT;
    }

    if ( updatePersistentData && (CV_PERSISTENT_DATA->poa_enable != (uint8_t)POAEnable & 0x01))
    {   
        writePersistentData = TRUE;
        CV_PERSISTENT_DATA->poa_enable   = (uint8_t)POAEnable & 0x01;
    }

    /* POAEnable == true, to check more parameters */
    if( POAEnable && updatePersistentData )
    {
        if (CV_PERSISTENT_DATA->poa_timeout != (uint8_t)inactivityTimeout)
        {
            writePersistentData = TRUE;
            /* if Zero, use the default one */
            if ( inactivityTimeout == 0 )
                CV_PERSISTENT_DATA->poa_timeout = POA_DEFAULT_TIMEOUT;
            else CV_PERSISTENT_DATA->poa_timeout  = (uint8_t)inactivityTimeout;
        }
#ifdef POA_MAX_FAILED_MATCHES
        if (CV_PERSISTENT_DATA->poa_max_failedmatch  != maxFailedMatches)
        { 
            writePersistentData = TRUE;
            /* If zero, use the default one */
            if( maxFailedMatches == 0 )
                  CV_PERSISTENT_DATA->poa_max_failedmatch  = POA_MAX_FAILED_MATCH;
            else  CV_PERSISTENT_DATA->poa_max_failedmatch  = (uint8_t)maxFailedMatches;
        }
#endif
    }

    /* Enable/disable POA feature on EC */
    if ( POAEnable && ush_get_fp_template_count() > 0 )
    {
        cmd[0] = POA_CMD_MASK | POA_CMD_ENABLE_POA;
        cmd[1] = CV_PERSISTENT_DATA->poa_timeout;
        len = 2;
    } else
    {
        cmd[0] = POA_CMD_MASK | POA_CMD_DISABLE_POA;
        len = 1;
    }

    /* send command to EC */
    ush_poa_send_data(cmd, len, 1, &status);

    /* check if return the expected status */
    if( (status == USH_POA_MODE_DISABLED) || (status == USH_POA_MODE_ENABLED) )
    {
        /* if enable state changed, write persistent data */
        if (writePersistentData && updatePersistentData) {
            retVal = cvWritePersistentData(TRUE);
        }
        return CV_SUCCESS;
    } else
    {
        printf("cv: cv_manage_poa failed.\n");
        return CV_FAILURE; 
    }
#endif
}

cv_status
cv_set_nfc_present(cv_handle cvHandle, cv_bool nfcPresent)
{
	/* this routine sets the nfc present */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;
	int nfc_presence = 0;

	// Get systemID
	nfc_presence = init_get_systemID();
	
	/* If NFC device is not present on the system, make NFC not present */
	if (nfc_presence != 2)
		nfcPresent = 0;	

	printf("CV: cv_set_nfc_present nfcPresent: %d\n", nfcPresent);

	if (nfcPresent)
	{
		/* check to see if nfc is present, if not, make present */
		if (!(CV_PERSISTENT_DATA->deviceEnables & CV_NFC_PRESENT))
		{
			printf("CV: cv_set_nfc_present NFC is not present. Making NFC present\n");
			/* not present, make present here */
			CV_PERSISTENT_DATA->deviceEnables |= CV_NFC_PRESENT;
			writePersistentData = TRUE;
		}
	} else {
		/* check to see if nfc is present, if it is then make not present */
		if ((CV_PERSISTENT_DATA->deviceEnables & CV_NFC_PRESENT))
		{
			printf("CV: cv_set_nfc_present NFC is present. Making NFC not present and not enabled\n");
			/* present, make not present and disabled here */
			CV_PERSISTENT_DATA->deviceEnables &= ~(CV_NFC_PRESENT | CV_NFC_ENABLED);
			writePersistentData = TRUE;
		}
	}

	/* if present state changed, write persistent data */
	if (writePersistentData) {
		retVal = ushx_set_deviceEnables(CV_PERSISTENT_DATA->deviceEnables);
		retVal |= cvWritePersistentData(TRUE);
	}

	return retVal;
}

cv_status
cv_set_cv_only_radio_mode(cv_handle cvHandle, cv_bool cvOnlyRadioMode)
{
	return CV_RADIO_NOT_PRESENT;
}

cv_status
cv_write_pending_cv_only_radio_mode(void)
{
	cv_status retVal = CV_SUCCESS;

	/* this routine will write the persistent value of CV only radio mode if USB re-enum has completed */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_MODE_PEND)
	{
		printf("cv_write_pending_cv_only_radio_mode: write pending \n");
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE;
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_MODE_PEND;
		retVal = cvWritePersistentData(TRUE);
	}
	return retVal;
}


cv_status
cv_set_cv_only_radio_mode_volatile(cv_handle cvHandle, cv_bool cvOnlyRadioMode)
{
	return CV_RADIO_NOT_PRESENT;
}

cv_status
cv_reenable_fp_poll(void)
{
	cv_status retVal = CV_SUCCESS;
	/* if FP polling has been temporarily disabled, re-enable */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS) && ((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) ||
		is_fp_synaptics_present_with_polling_support() ||
		(VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH)) )
	{
		retVal = cvUSHFpEnableFingerDetect(TRUE);
		printf("cv_reenable_fp_poll: re-enable FP polling\n");
	}
        return retVal;
}

cv_status
cv_notify_system_power_state(cv_system_power_state cvSystemPowerState)
{
	/* this routine is a place-holder for any processing that needs to happen at system power */
	/* state transitions */

	return CV_SUCCESS;

}

cv_status
cv_set_rfid_params(cv_handle cvHandle, uint32_t paramsID, uint32_t paramsLength, uint8_t *params, uint32_t bEraseHID)
{
	/* this routine allows system ID to be supplied and saved persistently */
	cv_status retVal = CV_SUCCESS;
	cv_rfid_params_flash_blob paramHeader;
	uint32_t interval, minInterval;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* validate that length isn't too large */
	if (paramsLength > CV_MAX_RFID_PARAMS_LENGTH)
		return CV_INVALID_INPUT_PARAMETER_LENGTH;

	/* don't allow flash update more often that every 5 minutes, to prevent flash burnout */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_ANTIHAMMERING_ACTIVE))
	{
		interval = cvGetDeltaTime(CV_VOLATILE_DATA->timeLastRFIDParamsWrite);
		/* workaround to account for Atmel flash requiring 2 writes for each page write */
		if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
			minInterval = 2*CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		else
			minInterval = CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		/* for first time through (ie, last time is 0) need to init start time */
		if ((interval < minInterval) && (CV_VOLATILE_DATA->timeLastRFIDParamsWrite != 0))
		{
			/* attempt to initiate command within disallowed interval, return antihammering status */
			retVal = CV_ANTIHAMMERING_PROTECTION;
			goto err_exit;
		} else
			/* here if command can be executed.  intialize or reinitialize start time */
			get_ms_ticks(&CV_VOLATILE_DATA->timeLastRFIDParamsWrite);
	}

	/* now write to flash, first header, then param data */
	memcpy((uint8_t *)&paramHeader.magicNumber,"rFiD", sizeof(paramHeader.magicNumber));
	paramHeader.paramsID = paramsID;
	paramHeader.length = paramsLength;
	if ((ushx_flash_read_write(CV_BRCM_RFID_PARAMS_CUST_ID, 0, sizeof(paramHeader), (uint8_t *)&paramHeader, 1)) != PHX_STATUS_OK)
	{
		/* flash write failure */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}
	if ((ushx_flash_read_write(CV_BRCM_RFID_PARAMS_CUST_ID, sizeof(paramHeader), paramsLength, params, 1)) != PHX_STATUS_OK)
	{
		/* flash write failure */
		retVal = CV_FLASH_ACCESS_FAILURE;
		goto err_exit;
	}

#if 0
	/* Removed from code review */
	if (bEraseHID)
	{
		printf("Erase HID flash area\n");
		retVal = cv_clear_flash_data(CV_HID2_FLASH_BLOCK_CUST_ID);
	}
#endif

err_exit:
	return retVal;
}


/* This function does not take into account of anti hammering, so should be called within other function
   INPUT: flash_id: currently only tested with CV_HID2_FLASH_BLOCK_CUST_ID
   OUTPUT: CV_SUCCESS, CV_FLASH_ACCESS_FAILURE
*/
cv_status
cv_clear_flash_data(uint32_t flash_id)
{
	uint32_t  nLen;
	uint8_t   tmpData[1024];
	cv_status retVal = CV_SUCCESS;
	uint32_t  nOffset = 0;

	memset(tmpData, 0, sizeof(tmpData));

	/* now write to flash */
	while(1)
	{
		nLen = cust_flash_block_size[flash_id] - nOffset;
		if (nLen == 0) break;

		nLen = (nLen > sizeof(tmpData)) ? sizeof(tmpData) : nLen;

		if ((ushx_flash_read_write(flash_id, nOffset, nLen, tmpData, 1)) != PHX_STATUS_OK)
		{
			/* flash write failure */
			retVal = CV_FLASH_ACCESS_FAILURE;
			goto err_exit;
		}

		nOffset += nLen;
	}

err_exit:
	return retVal;
}

cv_status
cv_pwrSaveStart_conf(uint32_t pwrSaveStart)
{
	/* this routine configures the Auto pwr save time */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;

	/* check to see there is change in configuration in flash*/
	if (CV_PERSISTENT_DATA->pwrSaveStartIdleTimeMs !=  pwrSaveStart)
	{
		/* Write the new value */
		CV_PERSISTENT_DATA->pwrSaveStartIdleTimeMs = pwrSaveStart;
		writePersistentData = TRUE;
		printf("CV: PWR Save Start idle time %u ms [new value]\n", pwrSaveStart);
	} else {
		printf("CV: PWR Save Start idle time %u ms [no change]\n", pwrSaveStart);
	}

	/* if enable state changed, write persistent data */
	if (writePersistentData)
		retVal = cvWritePersistentData(TRUE);

	return retVal;
}
void cv_logging_enable_apply(uint32_t bitset)
{
	if((bitset & CV_HID_LOGGING_ENABLED) == CV_HID_LOGGING_ENABLED)
	{
		printf("%s: Enabling HID Logs to LOG_MASK_ALL\n",__FUNCTION__);
		HIDSetLogLevel(LOG_MASK_ALL, LOG_DEBUG);
	}
	else
	{
		printf("%s: Disabling HID Logs\n",__FUNCTION__);
		HIDSetLogLevel(LOG_MASK_ALL, 0);
	}
	if((bitset & CV_NFC_LOGGING_ENABLED) == CV_NFC_LOGGING_ENABLED)
	{
		printf("%s: Enabling NFC packet Logs \n",__FUNCTION__);
	}
	
}


cv_status
cv_logging_enable_set(cv_handle *handle, uint32_t bitset)
{
	/* this routine configures the Auto pwr save time */
	cv_status retVal = CV_SUCCESS;
	cv_bool writePersistentData = FALSE;

	/* check to see there is change in configuration in flash*/
	if (CV_PERSISTENT_DATA->loggingEnables !=  bitset)
	{
		/* Write the new value */
		CV_PERSISTENT_DATA->loggingEnables = bitset;
		writePersistentData = TRUE;
		printf("CV: cv_logging_enable_set %x [new value]\n", bitset);
		cv_logging_enable_apply(bitset);
	} else {
		printf("CV: cv_logging_enable_set %x [no change]\n", bitset);
	}

	/* if enable state changed, write persistent data */
	if (writePersistentData)
		retVal = cvWritePersistentData(TRUE);

	return retVal;
}


void
cv_reenum(void)
{
#ifdef USH_BOOTROM /* AAI */
	/* Start USB Device mode running */
	printf("CV: usb device Reenum\n");
	phx_blocked_delay_ms(500);
	disable_isr(INT_SRC_USBD);
	UsbShutdown();
	printf("CV: usb device init\n");
	phx_blocked_delay_ms(2*1000);
	usbDeviceInit();
	enable_isr(INT_SRC_USBD);
	printf("CV: usb device start\n");
	usbDeviceStart();
	printf("CV: usb device Reenum done\n");
#endif
}

void
cvCLCardDetectPresence(void)
{
	volatile cv_bool present = CV_VOLATILE_DATA->contactlessCardDetected;
	/* check to see if card was presented */
	if (present && !(CV_VOLATILE_DATA->CVDeviceState & CV_CONTACTLESS_SC_PRESENT))
	{
		printf("cvCLCardDetectPresence: CLSC arrival\n");
		CV_VOLATILE_DATA->CVDeviceState |= CV_CONTACTLESS_SC_PRESENT;
		cvHostCLAsyncInterrupt(TRUE);
	}
	/* check to see if card was removed */
	if (!present && (CV_VOLATILE_DATA->CVDeviceState & CV_CONTACTLESS_SC_PRESENT))
	{
		printf("cvCLCardDetectPresence: CLSC removal\n");
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_CONTACTLESS_SC_PRESENT;
		cvHostCLAsyncInterrupt(FALSE);
	}
}

/* Schedule the notify task */
void cvClCardDetectNofity(bool detected)
{
	/* this schedule a task to send contactless card present to host driver */
	uint32_t xTaskWoken = FALSE;
	if(CV_VOLATILE_DATA->contactlessCardDetectEnable == TRUE)
	{
		CV_VOLATILE_DATA->contactlessCardDetected = detected;
		xTaskWoken = queue_cmd( SCHED_CL_CMD, QUEUE_FROM_ISR, xTaskWoken);
		SCHEDULE_FROM_ISR(xTaskWoken);
	}
	return;
}

uint8_t
cvNFCTimerISR(void)
{
	/* this is the ISR for the timer interrupt used for polling for NFC activate notification */
	uint32_t xTaskWoken = FALSE;
	uint8_t  status = USH_RET_TIMER_INT_OK;

	push_glb_regs();
	reinitialize_function_table();

	ENTER_CRITICAL();
	xTaskWoken = queue_nfc_cmd( SCHED_NFCPOLL_CMD, xTaskWoken);
	EXIT_CRITICAL();
	
	/* If a task needs be scheduled, we may need to switch to another task. */
	status = USH_RET_TIMER_INT_SCHEDULE_EVENT;

	pop_glb_regs();
	return status;
}

bool bShouldSendToHID(rfid_card_type cardType)
{
	bool ret = TRUE; /* send to HID stack if ret = true */
	rfid_nci_hw* pHW = (rfid_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;
	pHW->bStaticRFIDRouting = FALSE;
	switch (cardType)
	{
		case ACT_ICLASS_LEGACY_ISO15693:
		case ACT_MIFARE_CLASSIC_14443A:
			ret = TRUE; /* Send to HID */
			break;
		case ACT_MIFARE_DESFIRE_14443A:
			if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_MIFARE_DESFIRE_14443A_TO_NFC)
				ret = FALSE;
			else if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_MIFARE_DESFIRE_14443A_TO_RFID)
				pHW->bStaticRFIDRouting = TRUE;
			break;
		case ACT_ISO14443A:
			if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_ISO14443A_TO_NFC)
				ret = FALSE;
			else if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_ISO14443A_TO_RFID)
				pHW->bStaticRFIDRouting = TRUE;
			break;		
		case ACT_ISO14443B:
			if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_ISO14443B_TO_NFC)
				ret = FALSE;
			else if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_ISO14443B_TO_RFID)
				pHW->bStaticRFIDRouting = TRUE;
			break;		
		case ACT_SONY_FELICA:
			if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_T3T_TO_NFC)
				ret = FALSE;
			else if(CV_PERSISTENT_DATA->deviceEnables & CV_ROUTE_T3T_TO_RFID)
				pHW->bStaticRFIDRouting = TRUE;
			break;		
		case ACT_OTHER:
		default:	
			ret = FALSE; /* send to NFC stack */
			break;
	}
	printf("bShouldSendToHID: cardType= %x and enable = %x and ret = %d and RFIDRouting = %d \n",cardType,CV_PERSISTENT_DATA->deviceEnables,ret,pHW->bStaticRFIDRouting);
	return ret;
}

bool bShouldNotifyToHID(rfid_card_type cardType)
{
	bool ret = FALSE; /* Notify to HID stack if ret = true */
	switch (cardType)
	{
	case ACT_SONY_FELICA:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}
	return ret;
}


void cvNFCTimerCommand(void)
{
	uint8_t rsp[NFP_RX_TX_BUFFER_SIZE - 4];
	uint8_t rsp1[9]={0x00,0x00,0x00,0x05,0x10,0x61,0x07,0x01,0x00};
    uint16_t rlen = sizeof(rsp);
	uint8_t *rsp_rfid;
	uint16_t rlen_rfid;
	rfid_card_type cardType;
	int status1;
	rfid_nci_hw* pHW = (rfid_nci_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;
	
	int i;
	
	/* Send response to USB first, if one is available */
	if (isNFCEnabled()) {
		if (pnfpclass_mem_block->bNCIResponseAvailable == 1) {
				usbNFPBulkSend(pnfpclass_mem_block->nfpTxBuffer, pnfpclass_mem_block->nciResponseLength, NFP_REQUEST_TIMEOUT);
				pnfpclass_mem_block->bNCIResponseAvailable = 0;
		}			
	}
	if (nci_spi_int_state()==0)
	{
		uint32_t type = 0;
		uint32_t uuidLen = 0;
		uint8_t uuidValue[NFP_RX_TX_BUFFER_SIZE - 4];
		int notificationId = 0;

		/* Disable SPI int */
		//nci_disable_isr();  It is being commented so additional messages being queued are not cleared.

		if(is_nfc_pkt_log_enabled())
			printf("**\n");

		if (pHW->bIsRunningTransceive)
		{
			//nci_enable_isr();
			return;
		}
		
		/*nci_notification(rsp, &rlen);*/
		nci_notification_common(rsp, &rlen);
		rsp_rfid = rsp + 2;
		rlen_rfid = rlen - 2;

		/* clear the isr here since we copied the packet */
		nci_clear_isr();
		
		notificationId = nci_parse_notification(rsp_rfid,rlen_rfid,&type,&uuidLen,&uuidValue);
		
        if (notificationId == RF_INTF_ACTIVATED_NTF)
		{
			cardType = nci_processCardActivation(rsp_rfid, rlen_rfid);
			pHW->ignoreSleep &= ~NCI_IGNORE_SLEEP_ENABLED; /* Reset */
			
			//Call activate function
			if (bShouldSendToHID(cardType))
			{	/* Send to HID */	
				if (pnci_mem_block->bSuspendedRFIDAndNeedToResume)
				{
					printf("RFID suspended. Not sending activation.\n");
				}
				else
				{
					if (pHW->fActivateNotificationFunction)
					{
						pHW->bIsActivated = TRUE;		
						if((status1=pHW->fActivateNotificationFunction(cardType, rsp_rfid))<0)
						{
						   printf("HID dropped the Activatiation %d \n",status1);
						   pHW->bIsActivated = FALSE;		
						}
					}
				}
			}
						
			else
			{	/* Send to NFC, Before that notify the HID stack for T1/T2/T3 cards */	
				if(bShouldNotifyToHID(cardType))
				{
					/* create a event to handle the notification */
					rfid_frame *rfFrame = (rfid_frame *)VOLATILE_MEM_PTR->rfid_frame_ptr;
					rfFrame->nDataLen = (rlen >= RFID_MAX_FRAME_LENGTH) ? (RFID_MAX_FRAME_LENGTH-1): rlen;
					memcpy(rfFrame->pData, rsp, rfFrame->nDataLen);
					queue_cmd(SCHED_HID_CARD_ACTIV_NOTIFY, QUEUE_FROM_TASK, NULL);
					pHW->bnotifiedNfcActivation = TRUE;
				}
			
				pHW->bIsActivated = FALSE;		
			
				pnfpclass_mem_block->nciResponseLength = rlen;
				memcpy(pnfpclass_mem_block->nfpTxBuffer, rsp, rlen);
			
				if (pnci_mem_block->bNCIToUsbActive) {
					pnfpclass_mem_block->bNCIResponseAvailable = 1;
				}
				else
					printf("Dropping response\n");
				if(is_nfc_pkt_log_enabled())
				{
					printf("ANCILen: %d\n", rlen);
                    printf("(BCMIR): ");
                    for (i = 5; i < rlen; i++)
                        printf("%02x", rsp[i]);
                    printf("\n");
				}
			}
		}
		
		else if ((pHW->fActivateNotificationFunction) && (pHW->bIsActivated))	/*If activated send to RFID */
		{ /* Send credit notification to RFID stack*/
			printf("Credit Notification\n");
			cardType = ACT_CREDIT_NOTIFICATION;
			pHW->fActivateNotificationFunction(cardType, rsp_rfid);
		}

		else 	/* Send to NFC stack */
		{
			pnfpclass_mem_block->nciResponseLength = rlen;
			memcpy(pnfpclass_mem_block->nfpTxBuffer, rsp, rlen);
			
			if (isNFCEnabled()) {
				pnfpclass_mem_block->bNCIResponseAvailable = 1;
			}
			else
				printf("NFC not enabled, dropping response\n");

			if(is_nfc_pkt_log_enabled())
			{
			    printf("%s: from NFCC len=%d, hdr:", __FUNCTION__, rlen);
			    for (i = 0; i < rlen && i < 5; i++)
			        printf(" %02x", rsp[i]);
			    printf("\n(BCMIR): ");
			    for (i = 5; i < rlen; i++)
			        printf("%02x", rsp[i]);
			    printf("\n");
			}

			if (pHW->bnotifiedNfcActivation == TRUE && parse_rf_deactivate_ntf_msg(rsp) == true)
			{
				queue_cmd(SCHED_HID_CARD_DEACTIV_NOTIFY, QUEUE_FROM_TASK, NULL);
				pHW->bnotifiedNfcActivation = FALSE;
			}

			if(rlen == sizeof(rsp1) && memcmp(rsp,rsp1,rlen)==0)
			{
				phx_blocked_delay_ms(10);	
			}

			if(rsp[3] == 0 && rlen > 4) // Check if the length of the packet which is 4th byte matches the read buffer length if not it is a SPI INT issue
			{
				// Dropping the packet 
				pnfpclass_mem_block->bNCIResponseAvailable = 0;
				printf("SPI INT remains in Low state \n");
				if(CV_VOLATILE_DATA->nfcRegPu != 2)
					nci_ctrl_nfc_wake(1);
			}
		}
	}
	
	if (isNFCEnabled()) {
		if (pnfpclass_mem_block->bNCIResponseAvailable == 1) 
		{
			/* If response is greate than 64 bytes, garbage received, drop packet */
			/*if (pnfpclass_mem_block->nciResponseLength > 64)
			{
				printf("NCI Resp too large, dropping\n");
				pnfpclass_mem_block->bNCIResponseAvailable == 0;
			}
			else*/ 
			{
				/* Resume usb device if selectively suspended */	
				usbdResume();
				
				usbNFPBulkSend(pnfpclass_mem_block->nfpTxBuffer, pnfpclass_mem_block->nciResponseLength, NFP_REQUEST_TIMEOUT);
				pnfpclass_mem_block->bNCIResponseAvailable = 0;
			}
		}
	}	
	/* Enable the Interrupt since all the processing is done */
	if (pHW->bIsActivated == FALSE && CV_VOLATILE_DATA->nfcRegPu != 2) /* added check to enable ISR only if regpu is on */
	{
		nci_enable_isr();
	}
}

#ifdef TYPE4_WORK
/* API to send the NFC content to the NFC host driver */
cv_status cv_nfc_send_data_host_driver (uint8_t *pBuffer, uint32_t len)
{
	if(isNFCEnabled() && len <= 264)
	{
		if(pnci_mem_block->bNCIToUsbActive)
		{
			pnfpclass_mem_block->nciResponseLength = len;
			memcpy(pnfpclass_mem_block->nfpTxBuffer, pBuffer, len);
			/* Resume usb device if selectively suspended */
			usbdResume();
		
			printf("%s:%d send to NFC host stack \n",__FUNCTION__,__LINE__);
			usbNFPBulkSend(pnfpclass_mem_block->nfpTxBuffer, pnfpclass_mem_block->nciResponseLength, NFP_REQUEST_TIMEOUT);
			return CV_SUCCESS;
		}
		/* Turn SPI interrupts on */
//		enable_nfc_int();
	}
	return CV_INTERNAL_DEVICE_FAILURE;
}

void BRCM_NFCC_DisconnectInWrongState(void)
{
	printf("%s:%d Called the big hammer from HID stack \n",__FUNCTION__,__LINE__);
}

/* Return Non-zero value to perform  static routing and not enable type-4 emulation in the HID stack */
/*
0		   use NDEF caching and T4T emulation
1		   detect NDEF and route _either_ to HID or to NFC
2		   don't detect NDEF and route always to HID
*/
int BRCM_IsType4StaticRouting(void)
{
	rfid_nci_hw* pHW = (rfid_nci_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;
	if(pHW->bStaticRFIDRouting == TRUE)
	{
		printf("%s: Static RFID routing \n",__FUNCTION__);
		return 2;
	}
	else
	{
		printf("%s: Perform NDEF detect and route to either NFC or RFID \n",__FUNCTION__);
		return 1; 
	}
}

/* HID passes 0 for  reset failed in reset_successful and 1 on reset succeeded */
void BRCM_TakeOverTag(rfid_card_type rfidCardType, char* activation_ntf, unsigned int activation_ntf_len, uint32_t reset_successful)
{
	int bType4TagProcess=0,bType3TagProcess=0;

	// BRFD_ChannelHandle channelHandle = (BRFD_ChannelHandle)hid_glb_ptr;
	rfid_nci_hw* pHW = (rfid_nci_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;

	switch (rfidCardType) {
		case ACT_ISO14443A:
		case ACT_ISO14443B:
		case ACT_MIFARE_DESFIRE_14443A:
			bType4TagProcess = 1;
			break;
		case ACT_SONY_FELICA:
			bType3TagProcess = 1;
			break;
		}

	printf("%s: reset_successful = %d and activated = %d \n",__FUNCTION__,reset_successful,pHW->bIsActivated);

	if(bType4TagProcess == 1 && reset_successful == 0)
	{
		printf("%s:[T4T] NFCC Warm reset failed so NCI_CMD_RESET is issued \n",__FUNCTION__);
		nfc_send_reset();
	}
	else if(bType4TagProcess == 1 && reset_successful == 2)
	{
	  printf("%s:[T4T] Ignore the actviation frame \n",__FUNCTION__);
	  pHW->ignoreSleep |= NCI_IGNORE_SLEEP_ENABLED;
	  nci_ctrl_nfc_wake(0);
	}
	else 
	{
		/* forward the activation message to the host */
		HIDNCI_SendResponse(activation_ntf, activation_ntf_len, "ACTIVATION (forwarded)");
		/* Reset T4T processing, routing all traffic back to BCM */
		printf("%s:[T4T] Routing traffic back to BRCM firmware for NFC host-stack communication.\n",__FUNCTION__);
	}
	pHW->bIsActivated = FALSE;
	pHW->bType4TagProcess = 0;
	pHW->bType3TagProcess = 0;
}

#endif


bool cv_card_polling_active(void)
{
	rfid_nci_hw* pHW = (rfid_nci_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;
	BRFD_ChannelHandle channelHandle;

	rfGetChannelHandle(&channelHandle);
	if(pHW)
	{
		if(pHW->bType4TagProcess == TRUE || pHW->bType3TagProcess == TRUE || HID_IsCardPresent(channelHandle) == TRUE || pnfpclass_mem_block->bNCIResponseAvailable == 1)
		{
			printf("%s:Returned True %d,%d,%d \n",__FUNCTION__,pHW->bType4TagProcess,pHW->bType3TagProcess,pnfpclass_mem_block->bNCIResponseAvailable);
			return TRUE;
		}
	}
	return FALSE;
}
 



#ifndef LIB_COMPILE
/* Dummy function to help HID library to compile */
/* HID uses this function in the SDK to schedule the timer task */
void cvNFCTimerTask(rfid_nci_hw* pHW)
{

}
#endif


