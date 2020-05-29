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
 * cvinit.c:  CV initialization handler
 */
/*
 * Revision History:
 *
 * 02/20/07 DPA Created.
*/
#include "cvmain.h"
#include "ush_version.h"
#include "sched_cmd.h"
#include "console.h"
#include "nvm_cpuapi.h"
#include "pwr.h"
#include "sbi_ush_mbox.h"
#include "cvfpstore.h"
#include "spi_cpuapi.h"
#include "fp_spi_enum.h"
#include "../nci/nci.h"
#include "sotp_apis.h"
#include "device_dsc.h"
#ifdef PHX_POA_BASIC
#include "../pwr/poa.h"
#endif
#undef CV_DBG

cv_status
cvDetectFPDevice(void)
{
	/* this routine detects the fingerprint capture device.  it should only need to be run once, because it */
	/* saves the detected device in persistent data */
	cv_status retVal = CV_SUCCESS;
	uint16_t pVID, pPID;

#ifdef PHX_REQ_FP
	/* note: USB host should be already initialized */
	pVID = usb_fp_vid();
	pPID = usb_fp_pid();

	/* Detecting sensor at power-up, so clear old flags */
	CV_PERSISTENT_DATA->persistentFlags &= CV_FP_SENSOR_CLEAR_MASK;
	/* now check to see which FP device is attached */
	if (pVID == FP_UPKS_VID && pPID == FP_UPKS_PID)
		/* UPEK swipe sensor */
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_FP_UPEK_SWIPE;
	else if (pVID == FP_UPKA_VID && pPID == FP_UPKA_PID)
		/* UPEK area sensor */
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_FP_UPEK_TOUCH;
	else if (pVID == FP_AT_VID)
		/* AT swipe sensor */
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_FP_AT_SWIPE;
	else if ((pVID == FP_VALIDITY_VID) && (pPID == FP_VALIDITY_PID_VFS301 || 
		                                   pPID == FP_VALIDITY_PID_VFS5111))
		/* Validity swipe sensor */
		CV_PERSISTENT_DATA->persistentFlags |= CV_HAS_VALIDITY_SWIPE;
	else			
		retVal = CV_FP_DEVICE_GENERAL_ERROR;		/* Should really have a code to indicate presence of fp device */

#endif //PHX_REQ_FP
	return retVal;
}

cv_status
cvInitFPDevice(void)
{
	/* this routine initializes the fingerprint capture device. */
	cv_status retVal = CV_SUCCESS;

	unsigned int width; 

	printf("cvInitFPDevice fpInit\n");
	/* initialize and open the attached device */
	if ((retVal = fpInit(FP_CAPTURE_SMALL_HEAP_SIZE, FP_CAPTURE_SMALL_HEAP_PTR)) != CV_SUCCESS)
	{
		printf("cvInitFPDevice fpInit failed:%d\n", retVal);
		return retVal;
	}

	printf("cvInitFPDevice fpOpen\n");
	if ((retVal = fpOpen(FP_CAPTURE_LARGE_HEAP_SIZE, FP_CAPTURE_LARGE_HEAP_PTR)) != CV_SUCCESS)
	{
		printf("cvInitFPDevice fpOpen failed:%d\n", retVal);
		return retVal;
	}
	printf("cvInitFPDevice fpOpen success\n");

	// calling rebaseline here in advance, so the user will not be required to
	// remove and replace the finger later during authentication
	fpNextSensorDoBaseline();
	VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily = 1;

	if ( is_fp_synaptics_present() )
	{
		printf("cvInitFPDevice fpCalibrate\n");
		if ((retVal = fpCalibrate()) != CV_SUCCESS)
		{
			printf("cvInitFPDevice fpCalibrate failed:%d. resetting.\n", retVal);
			fp_spi_nreset(0);
			fp_spi_msleep(3);
			fp_spi_nreset(1);
			fp_spi_msleep(50);
			if ((retVal = fpCalibrate()) != CV_SUCCESS)
			{
				printf("cvInitFPDevice fpCalibrate failed again:%d\n", retVal);
				return retVal;
			}
		}
	}

   // init h/w finger detect to disabled (unless flag indicates to re-enable)
   printf("cvInitFPDevice: fp_dev:  0x%x\n",fp_dev());
   fpCancel();
	// if this is following a resume, clear FP capture status so that PBA won't detect it as a bad swipe
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CHECK_CALIBRATION)
		CV_VOLATILE_DATA->fpCaptureStatus = 0;
	
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CHECK_CALIBRATION;
   // restart capture if in progress, except for internal (PBA) captures, which will be restarted by host

   // let all FP captures be started by host, except for cases where restart required.
   if ((CV_VOLATILE_DATA->CVDeviceState & CV_FP_RESTART_HW_FDET) && !(CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL))
   {
		/* first, check to see if previous async FP task needs to be terminated */
		prvCheckTasksWaitingTermination();
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_TERMINATE_ASYNC;
		if (cvUSHFpEnableFingerDetect(TRUE) == CV_FP_RESET)
		{
			/* reset sensor */
			printf("cvInitFPDevice: ESD occurred during cvUSHFpEnableFingerDetect, schedule FP reset cmd\n");
			queue_cmd(SCHED_FP_RESET, QUEUE_FROM_TASK, NULL);
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
			retVal = CV_SUCCESS;
		} else {
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_RESTART_HW_FDET;
		}
   } else
		retVal = cvUSHFpEnableFingerDetect(FALSE);
	
	return retVal;
}

cv_status cvVirginInit(uint32_t initType)
{
	/* this routine initializes indicated structures and writes them out to flash */
	cv_status retVal = CV_SUCCESS;
	uint32_t count;

	/* initialize flash heap if TLD or DIR0 to be initialized */
	if (initType & (CV_INIT_TOP_LEVEL_DIR | CV_INIT_DIR_0))
#ifdef DISABLE_PERSISTENT_STORAGE
		printf("%s: FIXME persistent storage is not working\n",__func__);
#else
		init_fheap();
#endif

	if (initType & CV_INIT_PERSISTENT_DATA)
	{
		/* init persistent data, but don't write till end, since other inits will update */
		memset(CV_PERSISTENT_DATA,0,sizeof(cv_persistent));
#ifndef CV_DBG		
		/* create UCK key */
		if ((retVal = cvRandom(AES_128_BLOCK_LEN, &CV_PERSISTENT_DATA->UCKKey[0])) != CV_SUCCESS)
			goto err_exit;
#endif
		/* set configuration defaults */
		cvSetConfigDefaults();
		
		/* persistant flags which does not get cleared by a cvinit */
		CV_PERSISTENT_DATA->deviceEnables = CV_RF_ENABLED | CV_SC_ENABLED | CV_FP_ENABLED;
#ifdef PHX_POA_BASIC
        CV_PERSISTENT_DATA->poa_enable           = POA_DEFAULT_MODE;  // POA disabled by default
        CV_PERSISTENT_DATA->poa_timeout          = POA_DEFAULT_TIMEOUT; 
   #ifdef POA_MAX_FAILED_MATCHES
        CV_PERSISTENT_DATA->poa_max_failedmatch  = POA_MAX_FAILED_MATCH;
   #endif
#endif
#ifdef PHX_NEXT_OTP
        CV_PERSISTENT_DATA->next_otp_valid = 0;
#endif
		CV_PERSISTENT_DATA->persistentFlags = CV_UCK_EXISTS;
		CV_PERSISTENT_DATA->pwrSaveStartIdleTimeMs = PWR_DEFAULT_AUTO_PWRSAVE_MS;
		CV_PERSISTENT_DATA->USH_FW_version = get_ush_version_by_index(USH_REL_UPGRADE_VER);

	} else if ((initType & CV_INIT_TOP_LEVEL_DIR) || (initType & CV_INIT_DIR_0)) {
		/* only bump monotonic counter if persistent data won't be written, but top level dir */
		/* or dir 0 will be, because it will be bumped there */
		if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
			goto err_exit;
		/* inform TPM that monotonic count updated */
		tpmMonotonicCounterUpdated();
	}

	if (initType & CV_INIT_TOP_LEVEL_DIR)
	{
		/* init top level directory */
		memset(CV_TOP_LEVEL_DIR,0,sizeof(cv_top_level_dir));
		CV_TOP_LEVEL_DIR->header.thisDirPage.hDirPage = CV_TOP_LEVEL_DIR_HANDLE;
		CV_TOP_LEVEL_DIR->header.antiHammeringCredits = CV_AH_INITIAL_CREDITS;
		CV_TOP_LEVEL_DIR->persistentData.len = sizeof(cv_persistent);
		CV_TOP_LEVEL_DIR->persistentData.entry.hDirPage = CV_PERSISTENT_DATA_HANDLE;
		/* write to flash here (unless persistent data will be written, which also writes top level dir) */
		if (!(initType & CV_INIT_PERSISTENT_DATA))
		{
#ifdef DISABLE_PERSISTENT_STORAGE
			printf("%s: FIXME persistent storage is not working\n",__func__);
#else
			if ((retVal = cvWriteTopLevelDir(FALSE)) != CV_SUCCESS)
				goto err_exit;
#endif
		}
	}
#ifdef CV_ANTIHAMMERING
//	printf("ah flash size: %d flash_cv %d\n", sizeof(cv_flash_antihammering_credit_data), sizeof(flash_cv));
	if (initType & CV_INIT_ANTIHAMMERING_CREDITS_DATA)
	{
		/* initialize antihammering credits data */
		CV_TOP_LEVEL_DIR->header.antiHammeringCredits = CV_AH_INITIAL_CREDITS;
		if ((retVal = cvWriteAntiHammeringCreditData()) != CV_SUCCESS)
			goto err_exit;
	}
#endif
	if (initType & CV_INIT_DIR_0)
	{
		/* init directory 0, but don't mirror to HD.  this is to support the case where */
		/* a HD is moved to a new machine.  mirrored dir 0 will get written the first time */
		/* an object is added */
		memset(CV_DIR_PAGE_0,0,sizeof(cv_dir_page_0));
#ifndef CV_DBG
#ifdef DISABLE_PERSISTENT_STORAGE
        printf("%s: FIXME persistent storage is not working\n",__func__);
#else
		if ((retVal = cvWriteDirectoryPage0(FALSE)) != CV_SUCCESS)
			goto err_exit;
		/* also write top level dir because cvWriteDirectoryPage0 bumps the multibuffer index */
		if ((retVal = cvWriteTopLevelDir(FALSE)) != CV_SUCCESS)
			goto err_exit;
#endif
#endif
	}
	if (initType & CV_INIT_PERSISTENT_DATA)
	{
#ifndef CV_DBG
#ifdef DISABLE_PERSISTENT_STORAGE
        printf("%s: FIXME persistent storage is not working\n",__func__);
#else
		if ((retVal = cvWritePersistentData(FALSE)) != CV_SUCCESS)
			goto err_exit;
#endif
#endif	
	}
err_exit:
	return retVal;
}

cv_status
cv_initialize_tld(void)
{
	uint32_t count;
#ifdef HDOTP_SKIP_TABLES
	uint32_t i, predictedCount;
#else
	uint32_t local_count = 1;
#endif
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL}, *objDirPtrsOut[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen, padLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_status retVal = CV_SUCCESS, retValBuf0 = CV_SUCCESS, retValBuf1 = CV_SUCCESS;
	uint8_t *objStoragePtr = FP_CAPTURE_LARGE_HEAP_PTR; /* scratch buffer */
	cv_bool enable2T = TRUE;
	uint32_t cfgRegVal, chipID;
	cv_hdotp_skip_table *hdotpSkipTable = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
#define USH_CHIPID_REV_A0 0x58800100
#define USH_CHIPID_REV_B0 0x58800200
#define USH_CHIPID_REV_C0 0x58800300
	cv_enables_2TandAH enables;
	uint16_t cookie;
	OTP_STATUS status;
	uint8_t kaes[AES_256_BLOCK_LEN];
	uint16_t key_size;
	uint8_t khmac_sha256[SHA256_LEN];

	/* clear volatile memory */
	memset(CV_VOLATILE_DATA,0,sizeof(cv_volatile));
	
	/* Initialize the version */
	cvGetVersion(&CV_VOLATILE_DATA->curCmdVersion);

	enable2T = FALSE;
	printf("ChipId: 0x%x \n",phx_dcfg_get_chipid());

	VOLATILE_MEM_PTR->is_aes_key_valid = 0;

	// read aes key into OTP structure
	status = readKeys(&kaes[0],&key_size,OTPKey_AES);
	if(status != IPROC_OTP_VALID)
	{
		printf("readKeys(Kaes) failed [0x%X]\n", status);
		// just set aes key to zero
		memset(&NV_6TOTP_DATA->Kaes, 0, AES_128_BLOCK_LEN);
	} else {
		// copy first half of 256 bit AES key for persistent 
		// storage 128 bit AES
		printf("readKeys AES done\n");
		memcpy(&NV_6TOTP_DATA->Kaes, &kaes[0], AES_128_BLOCK_LEN);
		
		// read HMAC also to verify that it is also good and set a flag to the host if everything is good
		status = readKeys(&khmac_sha256[0],&key_size,OTPKey_HMAC_SHA256);
		if(status != IPROC_OTP_VALID)
		{
			printf("readKeys(Khmac) failed [0x%X]\n", status);
		}
		else
		{
			printf("readKeys HMAC done\n");
			VOLATILE_MEM_PTR->is_aes_key_valid = 1;
		}
	}
	
#if 0
	printf("%s: KAES:\n",__func__);
	dumpHex(&NV_6TOTP_DATA->Kaes,AES_128_BLOCK_LEN);
#endif

	/* first read top level directory */
	memset(&objAttribs, 0, sizeof(objAttribs));
	objDirPtrs[2] = objStoragePtr;
	objDirPtrsOut[2] = (uint8_t *)CV_TOP_LEVEL_DIR;
	objDirHandles[2] = CV_TOP_LEVEL_DIR_HANDLE;
	/* if 2T is disabled, need to read in TLD multibuf index from flash */
	flashEntry.pObj = (uint8_t *)CV_TOP_LEVEL_DIR_FLASH_ADDR;
	objLen = sizeof(cv_top_level_dir) + sizeof(cv_obj_dir_page_blob) + SHA256_LEN;
	padLen = GET_AES_128_PAD_LEN(objLen);
	flashEntry.objLen = objLen + padLen;
	objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
	/* if 2T not enabled, need to read in multibuffer index from flash */
	if (!enable2T)
	{
		ush_read_flash((uint32_t)CV_TLD_MULTIBUF_INDX_FLASH_ADDR, sizeof(uint32_t), (uint8_t *)&CV_VOLATILE_DATA->tldMultibufferIndex);
		/* check to make sure it's a valid index (maybe not the first time) */
		if (CV_VOLATILE_DATA->tldMultibufferIndex == 1)
			flashEntry.pObj += CV_TOP_LEVEL_DIR_FLASH_OFFSET;
		else
			/* if anything other that zero (eg first time), just set to 0 */
			CV_VOLATILE_DATA->tldMultibufferIndex = 0;
		if ((retVal = cvGetFlashObj(&flashEntry, objStoragePtr)) != CV_SUCCESS)
			goto err_exit;
		if ((retVal =  cvUnwrapObj(NULL, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
		{
			/* unable to read TLD, must do virgin init */
			CV_VOLATILE_DATA->initType = CV_INIT_TOP_LEVEL_DIR | CV_INIT_PERSISTENT_DATA | CV_INIT_DIR_0;
#ifdef CV_ANTIHAMMERING
			CV_VOLATILE_DATA->initType |= CV_INIT_ANTIHAMMERING_CREDITS_DATA;
#endif
		}
	}
err_exit:
	return retVal;
}

cv_status
cv_initialize(void)
{
	/* this routine initializes the CV at boot (power on or reset) */
	/* need to read persistent data from flash */
	cv_obj_handle objDirHandles[3] = {0, 0, 0};
	uint8_t *objDirPtrs[3] = {NULL, NULL, NULL}, *objDirPtrsOut[3] = {NULL, NULL, NULL};
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen, padLen;
	cv_dir_page_entry_obj_flash flashEntry;
	cv_status retVal = CV_SUCCESS;
	uint8_t *objStoragePtr = FP_CAPTURE_LARGE_HEAP_PTR; /* scratch buffer */
	uint32_t i;
	cv_phx2_device_status devStatus = 0;
#ifdef CV_ANTIHAMMERING
	cv_antihammering_credit creditData;
#endif
	cv_bool writePersistentData = FALSE;
	int nfc_presence = 0;

	/* get device status from cv_initialize_tld */
	devStatus = CV_VOLATILE_DATA->PHX2DeviceStatus;

	/* read persistent data */
	if (CV_VOLATILE_DATA->initType == 0)
	{
		objDirPtrs[2] = objStoragePtr;
		objDirPtrsOut[2] = (uint8_t *)CV_PERSISTENT_DATA;
		objDirHandles[2] = CV_PERSISTENT_DATA_HANDLE;
		objLen = CV_TOP_LEVEL_DIR->persistentData.len + sizeof(cv_obj_dir_page_blob) + SHA256_LEN;
		padLen = GET_AES_128_PAD_LEN(objLen);
		flashEntry.objLen = objLen + padLen;
		objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
		/* compute flash address using base and offset for multi-buffering index from top level dir */
		flashEntry.pObj = (uint8_t *)CV_PERSISTENT_DATA_FLASH_ADDR + CV_PERSISTENT_DATA_FLASH_OFFSET*CV_TOP_LEVEL_DIR->persistentData.multiBufferIndex;
		objLen = CV_TOP_LEVEL_DIR->persistentData.len + sizeof(cv_obj_dir_page_blob) + SHA256_LEN;
		padLen = GET_AES_128_PAD_LEN(objLen);
		flashEntry.objLen = objLen + padLen;
		objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
#ifndef DISABLE_PERSISTENT_STORAGE
		if ((retVal = cvGetFlashObj(&flashEntry, objStoragePtr)) == CV_SUCCESS)
		{
			retVal = cvUnwrapObj(NULL, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0]);
		}
#else
        printf("%s: FIXME cv persistent storage is not working\n",__func__);
        retVal = CV_FEATURE_NOT_SUPPORTED;
#endif
		if (retVal != CV_SUCCESS)
				CV_VOLATILE_DATA->initType |= CV_INIT_PERSISTENT_DATA | CV_INIT_DIR_0;
	}

	/* now read directory page 0 */
	if (CV_VOLATILE_DATA->initType == 0)
	{
		objDirPtrs[2] = objStoragePtr;
		objDirPtrsOut[2] = (uint8_t *)CV_DIR_PAGE_0;
		objDirHandles[2] = CV_DIR_0_HANDLE;
		objLen = sizeof(cv_dir_page_0) + sizeof(cv_obj_dir_page_blob_enc) + SHA256_LEN;
		padLen = GET_AES_128_PAD_LEN(objLen);
		flashEntry.objLen = objLen + padLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
		objAttribs.storageType = CV_STORAGE_TYPE_FLASH_NON_OBJ;
		/* compute flash address using base and offset for multi-buffering index from top level dir */
		flashEntry.pObj = (uint8_t *)CV_DIRECTORY_PAGE_0_FLASH_ADDR + CV_DIR_PAGE_0_FLASH_OFFSET*CV_TOP_LEVEL_DIR->dir0multiBufferIndex;
		if ((retVal = cvGetFlashObj(&flashEntry, objStoragePtr)) == CV_SUCCESS)
			retVal = cvUnwrapObj(NULL, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0]);
		if (retVal != CV_SUCCESS)
				CV_VOLATILE_DATA->initType = CV_INIT_DIR_0;
	}
#ifdef CV_ANTIHAMMERING
	/* now read antihammering credit data */
	if (CV_VOLATILE_DATA->initType == 0)
	{
		objDirPtrs[2] = objStoragePtr;
		objDirPtrsOut[2] = (uint8_t *)&creditData;
		objDirHandles[2] = CV_ANTIHAMMERING_CREDIT_DATA_HANDLE;
		objLen = sizeof(cv_antihammering_credit) + sizeof(cv_obj_dir_page_blob) + SHA256_LEN;
		padLen = GET_AES_128_PAD_LEN(objLen);
		flashEntry.objLen = objLen + padLen;
		objAttribs.storageType = CV_STORAGE_TYPE_FLASH_PERSIST_DATA;
		flashEntry.pObj = (uint8_t *)CV_AH_CREDIT_FLASH_ADDR;
		if ((retVal = cvGetFlashObj(&flashEntry, objStoragePtr)) == CV_SUCCESS)
		{
			retVal = cvUnwrapObj(NULL, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0]);
		}
		if (retVal == CV_SUCCESS)
		{
			/* now check to see if the 2T count in this data is the same as the TLD.  if so, use this */
			/* value of the antihammering credits, because it means this data was written after the TLD */
			/* since the 2T count gets bumped when the TLD is written */
			if (creditData.counter == CV_TOP_LEVEL_DIR->header.thisDirPage.counter)
				CV_TOP_LEVEL_DIR->header.antiHammeringCredits = creditData.credits;
		} else
			CV_VOLATILE_DATA->initType |= CV_INIT_ANTIHAMMERING_CREDITS_DATA;
	}
#endif
	/* Fingerprint device initialization/removal is now done on notification by usbhost by cvFpEnum
	   and cvFpDenum */
	/* for AT 2810, will need calibration after power up */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CHECK_CALIBRATION;

	/* check to see if any attempts to read flash failed.  if so, need to init the indicated structure */
	if (CV_VOLATILE_DATA->initType)
		if ((retVal = cvVirginInit(CV_VOLATILE_DATA->initType)) != CV_SUCCESS)
		{
			if (retVal == CV_MONOTONIC_COUNTER_FAIL)
				devStatus |= CV_PHX2_MONOTONIC_CTR_FAIL;
			else
				devStatus |= CV_PHX2_FLASH_INIT_FAIL;
		}
		
	/* set up base addresses for open windows for host IO */
	set_smem_openwin_base_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF);
	set_smem_openwin_end_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF + CV_IO_COMMAND_BUF_SIZE);

	/* clear volatile object directory and init volatile data */
	memset(CV_VOLATILE_OBJ_DIR,0,sizeof(cv_volatile_obj_dir));
	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
		CV_VOLATILE_DATA->objCacheMRU[i] = i;
	for (i=0;i<MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES;i++)
		CV_VOLATILE_DATA->level1DirCacheMRU[i] = i;
	for (i=0;i<MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES;i++)
		CV_VOLATILE_DATA->level2DirCacheMRU[i] = i;
	CV_VOLATILE_DATA->space.totalNonvolatileSpace = CV_PERSISTENT_OBJECT_HEAP_LENGTH - CV_FLASH_ALLOC_HEAP_OVERHEAD;
	CV_VOLATILE_DATA->space.remainingNonvolatileSpace = CV_PERSISTENT_OBJECT_HEAP_LENGTH - CV_FLASH_ALLOC_HEAP_OVERHEAD;
	CV_VOLATILE_DATA->space.totalVolatileSpace = CV_VOLATILE_OBJECT_HEAP_LENGTH - CV_MALLOC_HEAP_OVERHEAD;
	CV_VOLATILE_DATA->space.remainingVolatileSpace = CV_VOLATILE_OBJECT_HEAP_LENGTH - CV_MALLOC_HEAP_OVERHEAD;
	printf("%s: initialized cv heap space to : %d\n",__func__,CV_VOLATILE_DATA->space.remainingVolatileSpace);
	/* for smartcard, check to see if command has set value.  if so, don't use the 'present' default */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_SC_PRESENT_SET))
			CV_PERSISTENT_DATA->deviceEnables |= CV_SC_PRESENT;

	
	if(!is_there_sc_cage())
	{
		printf("%s:Smartcard cage not present\n",__FUNCTION__);
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_SC_PRESENT;
	}
	
#ifdef PHX_REQ_FP_SPI
    fp_spi_enumerate();
#endif

	if (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_SENSOR_FOUND)
	{
		CV_PERSISTENT_DATA->deviceEnables |= CV_FP_PRESENT;
		if (!is_msg_queued(SCHED_FP_ENUM)) {
			queue_cmd(SCHED_FP_ENUM, QUEUE_FROM_TASK, FALSE);
		}
	}
	else	
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_FP_PRESENT;

	nfc_presence = init_get_systemID();
	if (nfc_presence != 2)
	{
		CV_PERSISTENT_DATA->deviceEnables &= ~(CV_NFC_PRESENT | CV_NFC_ENABLED);
		printf("NFC is not present\n");
	}
	else
	{
		CV_PERSISTENT_DATA->deviceEnables |= CV_NFC_PRESENT;

		/* CV only radio mode should always be disabled for NFC platforms */
		CV_PERSISTENT_DATA->persistentFlags |= CV_CV_ONLY_RADIO_MODE_DEFAULT;
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE;
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE_PERSISTENT;
		
		rfid_carrier_control(RFID_TXPOWER_OFF);
		
		printf("NFC is present\n");
	}

	if (3 == nfc_presence)
	{
		/* CV only radio mode should always be disabled for NFC platforms */
		CV_PERSISTENT_DATA->persistentFlags |= CV_CV_ONLY_RADIO_MODE_DEFAULT;
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE;
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE_PERSISTENT;
	}

	/* Set default polling period for fingerprint capture */ 
	CV_VOLATILE_DATA->fp_polling_period = FP_POLLING_PERIOD;
	/* for RF, check to see if command has set value.  if so, don't use the 'present' default */
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_RF_PRESENT_SET))
			CV_PERSISTENT_DATA->deviceEnables |= CV_RF_PRESENT;
	/* indicate if failures detected during init */
#ifndef CV_5890_DBG
	if (devStatus)
		CV_VOLATILE_DATA->PHX2DeviceStatus = devStatus;
	else
#endif
		CV_VOLATILE_DATA->PHX2DeviceStatus = CV_PHX2_OPERATIONAL;

	/* set up CV heap */
	init_heap(CV_VOLATILE_OBJECT_HEAP_LENGTH, CV_HEAP);
	/* set up timer routine to check antihammering credits */
	enable_low_res_timer_int(1000*60*2/* two minutes */, cvUpdateAntiHammeringCredits);

	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
#ifdef CV_ANTIHAMMERING
	/* now check to see if need to reset antihammering credits.  this could happen if CV_AH_INITIAL_CREDITS changes and need to change */
	/* persistent value */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_AH_CREDITS_RESET)
		|| !(CV_PERSISTENT_DATA->persistentFlags & CV_AH_CREDITS_RESET_1))
	{
		/* need to reset antihammering credits, if initial value is larger than current credits */
		if (CV_TOP_LEVEL_DIR->header.antiHammeringCredits < CV_AH_INITIAL_CREDITS)
			CV_TOP_LEVEL_DIR->header.antiHammeringCredits = CV_AH_INITIAL_CREDITS;
		/* now set persistent flag so reset won't happen again */
		CV_PERSISTENT_DATA->persistentFlags |= CV_AH_CREDITS_RESET;
		CV_PERSISTENT_DATA->persistentFlags |= CV_AH_CREDITS_RESET_1;
		retVal = cvWritePersistentData(TRUE);
	}
#endif
	/* now check to see if CV only radio mode default has been set to enabled */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_CV_ONLY_RADIO_MODE_DEFAULT))
	{
		/* need to set default */
		CV_PERSISTENT_DATA->deviceEnables |= CV_CV_ONLY_RADIO_MODE;
		/* now set persistent flag so reset won't happen again */
		CV_PERSISTENT_DATA->persistentFlags |= CV_CV_ONLY_RADIO_MODE_DEFAULT;
#ifdef DISABLE_PERSISTENT_STORAGE
        printf("%s: FIXME persistent storage is not working\n",__func__);
#else
		retVal = cvWritePersistentData(TRUE);
#endif
	}
	/* set persistent status of CV only radio mode.  This bit in the device enables will always reflect */
	/* the persistent status regardless what the volatile status is */
	if (CV_PERSISTENT_DATA->deviceEnables & CV_CV_ONLY_RADIO_MODE)
		CV_PERSISTENT_DATA->deviceEnables |= CV_CV_ONLY_RADIO_MODE_PERSISTENT;
	else
		CV_PERSISTENT_DATA->deviceEnables &= ~CV_CV_ONLY_RADIO_MODE_PERSISTENT;
	/* now check to see if new DA default threshold has been set */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_DA_THRESHOLD_DEFAULT))
	{
		/* need to set default */
		CV_PERSISTENT_DATA->DAAuthFailureThreshold = DEFAULT_CV_DA_AUTH_FAILURE_THRESHOLD;
		/* now set persistent flag so reset won't happen again */
		CV_PERSISTENT_DATA->persistentFlags |= CV_DA_THRESHOLD_DEFAULT;
		writePersistentData = TRUE;
	}
	/* now check to see if new FP persistence has been set */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_FP_PERSIST_DEFAULT2))
	{
		/* need to set default */
		CV_PERSISTENT_DATA->fpPersistence = FP_DEFAULT_PERSISTENCE;
		/* now set persistent flag so reset won't happen again */
		CV_PERSISTENT_DATA->persistentFlags |= CV_FP_PERSIST_DEFAULT2;
		writePersistentData = TRUE;
	}
	/* now set volatile value of CV-only radio mode to persistent value */
	if (CV_PERSISTENT_DATA->deviceEnables & CV_CV_ONLY_RADIO_MODE)
		CV_VOLATILE_DATA->CVDeviceState |= CV_CV_ONLY_RADIO_MODE_VOL;
	else
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_MODE_VOL;
	/* indicate that volatile version of CV-only radio mode can be used */
	CV_VOLATILE_DATA->CVDeviceState |= CV_CV_ONLY_RADIO_MODE_VOL_ACTIVE;

	/* if  not in managed mode, set default FAR value here, because it's disabled in cv_set_config */
	if (!(CV_PERSISTENT_DATA->persistentFlags & CV_MANAGED_MODE))
		CV_PERSISTENT_DATA->FARval = FP_DEFAULT_FAR;

	/* Handle FW default change definitions between fw upgrades here */
	if ( CV_PERSISTENT_DATA->USH_FW_version != get_ush_version_by_index(USH_REL_UPGRADE_VER)) {
		CV_PERSISTENT_DATA->pwrSaveStartIdleTimeMs = PWR_DEFAULT_AUTO_PWRSAVE_MS;
		CV_PERSISTENT_DATA->USH_FW_version = get_ush_version_by_index(USH_REL_UPGRADE_VER);
		writePersistentData = TRUE;
		printf("CV: persistant data upgrade for new firmware version 0x%08x\n", CV_PERSISTENT_DATA->USH_FW_version);
	}

	/* check HDOTP read limit based on monotonic counter being read during init. increment count and */
	/* determine if exceeds limit.  if so, need to write persistent data in order to bump the count */
#ifdef DISABLE_PERSISTENT_STORAGE
        printf("%s: FIXME persistent storage is not working\n",__func__);
#else

	/* Apply the logging config */
	cv_logging_enable_apply(CV_PERSISTENT_DATA->loggingEnables);	

	/* if HDOTP read count limit exceeded, write persistent data here */
	if (writePersistentData)
#ifdef DISABLE_PERSISTENT_STORAGE
        printf("%s: FIXME persistent storage is not working\n",__func__);
#else
		retVal = cvWritePersistentData(FALSE);
#endif

//	cvPrintMemSizes();
	printf("CV: cv_initialize: per %08x vol %08x\n", CV_PERSISTENT_DATA->deviceEnables, CV_VOLATILE_DATA->CVDeviceState);

	return retVal;
}

void
cv_reinit(void)
{
	int i = 0;
	/* this routine does any cleanup necessary during system reboot. */

	/* close all open sessions */
	printf("cv_reinit: entry\n");
	cvCloseOpenSessions(0);
	/* clear states */
	CV_VOLATILE_DATA->retryWrites = 0;
	CV_VOLATILE_DATA->CVInternalState = 0;
	/* reset dictionary attack parameters */
	CV_VOLATILE_DATA->DAFailedAuthCount = 0;
	CV_VOLATILE_DATA->DALockoutCount = 0;
	/* invalidate FP image */
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_IMAGE_VALID | CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
	/* indicate that CV command is not active */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_COMMAND_ACTIVE;
	/* cancel any active FP capture */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_REINIT_CANCELLATION;
	/* set first pass flag for PBA */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FIRST_PASS_PBA;
	/* clear any pending CV only radio mode changes */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_MODE_PEND;
	/* clear internal FP capture flag (set when capture initiated by cv_identify_user) */
	CV_VOLATILE_DATA->fpCapControl &= ~CV_CAP_CONTROL_INTERNAL;
	/* clear FP restart flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_RESTART_HW_FDET;
	/* clear contactless present flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CONTACTLESS_SC_PRESENT;
	/* clear first command flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_USH_INITIALIZED;
	memset(&(CV_VOLATILE_DATA->wbfProtectData), 0, sizeof(CV_VOLATILE_DATA->wbfProtectData));
	CV_VOLATILE_DATA->doingFingerprintEnrollmentNow = 0;
}

void
cv_reinit_resume(void)
{
	/* this routine does any cleanup necessary during resume from S3/S4 */
	rfid_nci_hw* pHW = (rfid_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;

	/* clear states */
	printf("cv_reinit_resume: entry\n");
	/* close all open sessions */
	cvCloseOpenSessions(0);
	CV_VOLATILE_DATA->retryWrites = 0;
	CV_VOLATILE_DATA->CVInternalState = 0;
	/* reset dictionary attack parameters */
	CV_VOLATILE_DATA->DAFailedAuthCount = 0;
	CV_VOLATILE_DATA->DALockoutCount = 0;
	/* invalidate FP image */
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_IMAGE_VALID | CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
	/* indicate that CV command is not active */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_COMMAND_ACTIVE;
	/* cause FP device to be calibrated, if necessary */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CHECK_CALIBRATION;
	/* ignore any async FP capture task processing while in resume */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_TERMINATE_ASYNC;
	/* set first pass flag for PBA */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FIRST_PASS_PBA;
	/* clear any pending CV only radio mode changes */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_MODE_PEND;
	/* clear internal FP capture flag (set when capture initiated by cv_identify_user) */
	CV_VOLATILE_DATA->fpCapControl &= ~CV_CAP_CONTROL_INTERNAL;
	/* clear FP restart flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_RESTART_HW_FDET;
	/* clear contactless present flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_CONTACTLESS_SC_PRESENT;
	/* clear first command flag */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_USH_INITIALIZED;

	pHW->bIsRunningTransceive = FALSE; // A workaround for an HID problem. They keep it as set for some reason if a card is on the reader during S3.	

	/* Enable the NFC interrupts */
	nci_clear_isr();
	nci_enable_isr();
}

void 
cv_abort(cv_bool hostRequest)
{
	/* this routine is called by the scheduler or USB device interface when the USB driver on the host issues an abort */
	/* or if the scheduler times out.  the purpose is to do any necessary command cleanup */
	cv_version version;
	cv_encap_command *result = (cv_encap_command *)CV_SECURE_IO_COMMAND_BUF;
	cv_usb_interrupt *usbInt;

	/* first put io window into secure mode, in case abort happened while in open mode */
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);

	/* reset any pending states */
	CV_VOLATILE_DATA->CVInternalState  &= ~(CV_PENDING_WRITE_TO_SC | CV_PENDING_WRITE_TO_CONTACTLESS | CV_WAITING_FOR_PIN_ENTRY);
	CV_VOLATILE_DATA->HIDCredentialPtr = NULL;
	CV_VOLATILE_DATA->secureSession = NULL;

	/* now if host sent abort request, sent return status for abort */
	if (hostRequest)
	{
    
		cvGetVersion(&version);
		memset(CV_SECURE_IO_COMMAND_BUF, 0, CV_IO_COMMAND_BUF_SIZE); 
		result->transportType = CV_TRANS_TYPE_ENCAPSULATED;
		result->transportLen = sizeof(cv_encap_command) - sizeof(result->parameterBlob);
		result->version.versionMajor = version.versionMajor;
		result->version.versionMinor = version.versionMinor;
		result->returnStatus = CV_SUCCESS;
		result->commandID = CV_ABORT;
		/* set up interrupt after command */
		usbInt = (cv_usb_interrupt *)((uint8_t *)result + result->transportLen);
		usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
		usbInt->resultLen = result->transportLen;
		/* send command and interrupt to USB */
		set_smem_openwin_open(CV_IO_OPEN_WINDOW);
		usbCvSend(usbInt, (uint8_t *)result, result->transportLen, HOST_CONTROL_REQUEST_TIMEOUT);
	} else
		/* abort from scheduler, just set window open, in case was secure when abort occurred */
		set_smem_openwin_open(CV_IO_OPEN_WINDOW);

}


unsigned int get_pwrSaveStartIdleTimeMs() {
	return CV_PERSISTENT_DATA->pwrSaveStartIdleTimeMs;
}
unsigned int get_pwrMode() {
	return VOLATILE_MEM_PTR->pwr_mode;
}
unsigned int get_isAESKeyValid() {
	return VOLATILE_MEM_PTR->is_aes_key_valid;
}

