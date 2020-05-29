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
 * cvfphandler.c:  CVAPI Fingerprint Device Handler
 */
/*
 * Revision History:
 *
 * 07/18/07 DPA Created.
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
#include "fp_spi_enum.h"
#include "fp_spi_api.h"
#include "drv_synfp_spi.h"
#include "../validity/include/fpErrorValidity.h"
#include "../dmu/crmu_apis.h"
#include "pwr_mgmt.h"

#undef DEBUG_WO_HOST_AUTH
#define DP_TIMING
#define SYNAPTICS_MATCHER_TIMING
#undef NEXT_DEBUG_OUTPUT
#undef SYNAPTICS_DEBUG_OUTPUT
#undef PRINT_LARGE_HEAP

#define FP_UART_OFF

#define CHIP_IS_5882_OR_LYNX (1)

#define FP_SUPPORT_INTR() (!((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) || is_fp_synaptics_present_with_polling_support() || (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH )))

cv_status
cvXlateDPRetCode(DPFR_RESULT result)
{
	/* this routine translates DP return codes into CV return codes */
	/* DP return code is stored so it can be retrieved by cv_get_status */
	/* and general error code is returned */
	if (result == FR_OK)
		return CV_SUCCESS;
	else {
		CV_VOLATILE_DATA->moreInfoErrorCode = (uint32_t)result;
		/* determine if it is bad swipe */
		if ((result & DP_BAD_SWIPE_MASK) == DP_BAD_SWIPE_MASK)
			return CV_FP_BAD_SWIPE;
		else if ((result == DP_FR_ERR_IO) || (result == DP_FR_ERR_INVALID_IMAGE))
			return CV_FP_BAD_SWIPE;
		else	
			return CV_FP_MATCH_GENERAL_ERROR;
	}
}

cv_status
cvXlatefpRetCode(fp_status result)
{
	if (result != FP_OK)
	{
		printf("result: %d\n", result);
	}
	/* this routine translates fp return codes into CV return codes. */
	/* device specific error is stored so it can be retrieved by cv_get_status and general error code is returned */
	if (result == FP_OK)
		return CV_SUCCESS;
	else if (result == FP_STATUS_TIMEOUT)
		return CV_FP_USER_TIMEOUT;
	else if (result == FP_STATUS_OPERATION_CANCELLED)
		return CV_CANCELLED_BY_USER;
	else if (result == FP_SENSOR_RESET_REQUIRED)
		return CV_FP_RESET;
		
	/* Check Validity return code */
	else if (CV_PERSISTENT_DATA->persistentFlags & CV_HAS_VALIDITY_SWIPE) {
		if (result == FP_VALIDITY_IMAGE_BAD) {
			printf("Image bad\n");
			return CV_FP_BAD_SWIPE;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_FAST) {
			printf("Swipe too fast\n");
			return CV_FP_BAD_SWIPE;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_SHORT) {
			printf("Swipe too short\n");
			return CV_FP_BAD_SWIPE;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_SLOW) {
			printf("Swipe too slow\n");
			return CV_FP_BAD_SWIPE;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_SKEW_TOO_LARGE) {
			printf("Skew too large\n");
			return CV_FP_BAD_SWIPE;
		}
		else if	(result == FP_VALIDITY_ERR_IMAGE_QUALITY_NOT_A_FINGER_SWIPE) {
			printf("Not a finger swipe\n");
			return CV_FP_BAD_SWIPE;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_BAD_SWIPE) {
			printf("Image quality bad\n");
			return CV_FP_BAD_SWIPE;
		}
	}
		
	CV_VOLATILE_DATA->moreInfoErrorCode = (uint32_t)result;
	return CV_FP_DEVICE_GENERAL_ERROR;
}

cv_capture_result_type
cvXlatefp2wbfRetCode(uint32_t result)
{
	/* this routine translates fp return codes into return codes compatible with wbf. */
	/* device specific error is stored so it can be retrieved by cv_get_status and general error code is returned */
	if (result == FP_OK)
		return CV_CAP_RSLT_SUCCESSFUL;
	else if (result == FP_SENSOR_RESET_REQUIRED)
		return CV_CAP_RSLT_FP_DEV_RESET;
		
	/* Check Validity return code */
	else if (CV_PERSISTENT_DATA->persistentFlags & CV_HAS_VALIDITY_SWIPE) {
		if (result == FP_VALIDITY_IMAGE_BAD) {
			printf("Image bad\n");
			return CV_CAP_RSLT_POOR_QUALITY;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_FAST) {
			printf("Swipe too fast\n");
			return CV_CAP_RSLT_TOO_FAST;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_SHORT) {
			printf("Swipe too short\n");
			return CV_CAP_RSLT_TOO_SHORT;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_TOO_SLOW) {
			printf("Swipe too slow\n");
			return CV_CAP_RSLT_TOO_SLOW;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_SKEW_TOO_LARGE) {
			printf("Skew too large\n");
			return CV_CAP_RSLT_TOO_SKEWED;
		}
		else if	(result == FP_VALIDITY_ERR_IMAGE_QUALITY_NOT_A_FINGER_SWIPE) {
			printf("Not a finger swipe\n");
			return CV_CAP_RSLT_POOR_QUALITY;
		}
		else if (result == FP_VALIDITY_ERR_IMAGE_QUALITY_BAD_SWIPE) {
			printf("Image quality bad\n");
			return CV_CAP_RSLT_POOR_QUALITY;
		}
	}
		
	return CV_CAP_RSLT_POOR_QUALITY;
}

fp_status dummyFingerDetectCallback(fp_callback_type status, uint32_t extraDataLength, void* extraData, void* arg)
{
    return FP_OK;
}


void
cvFpAsyncCapture(void)
{
	/* This task is created by cv_fingerprint_start_capture, but waits on an event triggered by a finger detection before doing the FP capture */
	uint32_t imageWidth;
	fp_status fpRetVal = FP_OK;
	cv_status retVal = CV_SUCCESS;
	cv_bool isCancelled = FALSE;
	uint32_t timeout =0;

wait_on_mutex:
	printf("cvFpAsyncCapture: task start\n");
	if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
	{
		if( ((CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_USED_MASK) == CV_POA_AUTH_USED_MASK) || (cv_get_timer2_elapsed_time(CV_VOLATILE_DATA->poaAuthTimeout) > CV_POA_AUTH_TIMEOUT))
		{
			CV_VOLATILE_DATA->poaAuthSuccessFlag = CV_POA_AUTH_NOT_DONE;
			CV_VOLATILE_DATA->poaAuthSuccessTemplate = 0;
			printf("POA SSO: Timeout happened or Retried 3 times, so clean the cache \n");
		}   else 
        {
            // mark the credential has been used one more time
            CV_VOLATILE_DATA->poaAuthSuccessFlag = (CV_VOLATILE_DATA->poaAuthSuccessFlag << 1) | CV_POA_AUTH_SUCCESS_COMPLETE;
        }
	}
	
	/* wait for h/w finger detect event */
	do {
		;
	} while	((CV_VOLATILE_DATA->poaAuthSuccessFlag == CV_POA_AUTH_NOT_DONE) && MUTEX_TAKE_BLOCKED(CV_VOLATILE_DATA->fingerDetectMutexHandle, portMAX_DELAY) != pdPASS);

	/* disable interrupt */
	if ( FP_SUPPORT_INTR() ) lynx_set_fp_det_interrupt(0);

	if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
	{
		CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
		printf("POA SSO: Ignore Mutex trigger - fpCaptureStatus 0x%x\n", CV_VOLATILE_DATA->fpCaptureStatus);
	}
	else
	{
		printf("cvFpAsyncCapture: mutex triggered - fpCaptureStatus 0x%x\n", CV_VOLATILE_DATA->fpCaptureStatus);
	}
	
	/* just end task and let FP capture get restarted */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_TERMINATE_ASYNC)
	{
		CV_VOLATILE_DATA->fpCaptureStatus = 0;
		isCancelled = TRUE;
		goto err_exit;
	}

	/* first check to see if this is triggered during a reinit.  if so, cancel any outstanding FP operation and terminate task */
	/* this could also happen if FP_STATUS_OPERATION_CANCELLED is returned to FP detect callback routine */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_REINIT_CANCELLATION) {
		printf("cvFpAsyncCapture: in system restart, just terminate async FP capture task\n");
		/* check to see if this is getting executed in the context of a command.  if not, just defer it until the next */
		/* command is executed, because FP device might not have been initialized yet */
		if (!(CV_VOLATILE_DATA->CVDeviceState & CV_COMMAND_ACTIVE))
		{
			/* defer cancellation */
			printf("cvFpAsyncCapture: defer cancellation until in context of command\n");
            /* Enable interrupt */
            lynx_set_fp_det_interrupt(1);
			goto wait_on_mutex;
		}
		fpRetVal = fpCancel();
		/* check for ESD case */
		if (fpRetVal == FP_SENSOR_RESET_REQUIRED)
		{
			/* reset sensor */
			printf("cvFpAsyncCapture: ESD occurred during fpCancel\n");
			fpSensorReset();
		}
		isCancelled = TRUE;
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_REINIT_CANCELLATION;
		goto err_exit;
	}

	/* check to see if some fault occurred during detection.  if so, call fpCancel to clear error and then restart */
	/* detection */
	if (CV_VOLATILE_DATA->fpCaptureStatus == FP_VALIDITY_ERR_IMAGE_QUALITY_BAD_SWIPE)
	{
		printf("cvFpAsyncCapture: error occurred during FP detection, restart\n");
		fpCancel();
		/* restart capture */
		if (cvUSHFpEnableFingerDetect(TRUE) == CV_FP_RESET)
		{
			/* reset sensor and exit (FP capture will be restarted after FP sensor init completes) */
			printf("cvFpAsyncCapture: ESD occurred during cvUSHFpEnableFingerDetect, restart\n");
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
			fpSensorReset();
		}
        /* Enable interrupt */
        lynx_set_fp_det_interrupt(1);
		goto wait_on_mutex;
	}

	/* check to see if ESD fault occurred during detection.  if so, call fpCancel to clear error and then restart */
	/* detection */
	if (CV_VOLATILE_DATA->fpCaptureStatus == FP_SENSOR_RESET_REQUIRED)
	{
		printf("cvFpAsyncCapture: ESD occurred during FP detection, restart\n");
		/* restart capture, but don't re-enable h/w finger detect until after FP device has been reinitialized */
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
		fpSensorReset();
        /* Enable interrupt */
        lynx_set_fp_det_interrupt(1);
		goto wait_on_mutex;
	}

	/* check to see if this is a cancellation */
	if (CV_VOLATILE_DATA->fpCaptureStatus == FP_STATUS_OPERATION_CANCELLED)
	{
		/* yes, do cancellation here */
		fpRetVal = fpCancel();
		/* check for ESD case */
		if (fpRetVal == FP_SENSOR_RESET_REQUIRED)
		{
			/* reset sensor */
			printf("cvFpAsyncCapture: ESD occurred during fpCancel\n");
			fpSensorReset();
		}
		printf("cvFpAsyncCapture: called fpCancel to do cancel  0x%x\n",fpRetVal);
		isCancelled = TRUE;
		goto err_exit;
	}

	/* If poa auth success flag is enabled then do not kick off the grab */
	if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
	{
		printf("POA SSO: Ignore the fpgrab\n");
		retVal = CV_SUCCESS;
		CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
		goto err_exit;
	}

	/* do capture */
	if((VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH ) || is_fp_synaptics_present_with_polling_support())
		timeout = PBA_FP_DETECT_TIMEOUT; 
	
	fpRetVal = fpGrab(&CV_VOLATILE_DATA->fpPersistedFPImageLength, &imageWidth,timeout ,NULL, NULL, 0, FP_GRAB);
	retVal = cvXlatefpRetCode(fpRetVal);
	printf("cvFpAsyncCapture: fpGrab result raw: 0x%x cv: 0x%x\n", fpRetVal, retVal);
	printf("cvFpAsyncCapture: fpGrab length: 0x%x width: 0x%x\n", CV_VOLATILE_DATA->fpPersistedFPImageLength, imageWidth);
	CV_VOLATILE_DATA->fpCaptureStatus = fpRetVal;
 	/* check to see if got success return from fpGrab, but length and width are 0.  if so, consider this a bad swipe */
 	if (retVal == CV_SUCCESS && CV_VOLATILE_DATA->fpPersistedFPImageLength == 0 && imageWidth == 0)
 	{
 		/* this result will get translated to CV_CAP_RSLT_POOR_QUALITY */
 		CV_VOLATILE_DATA->fpCaptureStatus = FP_STATUS_OPERATION_CANCELLED;
 		retVal = CV_FP_RSLT_POOR_QUALITY;
 	}

	/* check to see if grab was successful, but length or width is 0.  handle this as a bad swipe */
	if (retVal == CV_SUCCESS && (!CV_VOLATILE_DATA->fpPersistedFPImageLength || !imageWidth))
	{
		retVal = CV_CANCELLED_BY_USER;
		CV_VOLATILE_DATA->fpCaptureStatus = FP_STATUS_OPERATION_CANCELLED;
	}

	if (retVal == CV_SUCCESS)
	{
		/* capture successful.  set flags and persistence time to indicate image available */
		CV_VOLATILE_DATA->CVDeviceState |= (CV_FP_CAPTURED_IMAGE_VALID | CV_HAS_CAPTURE_ID);
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURE_IN_PROGRESS;
		get_ms_ticks(&CV_VOLATILE_DATA->fpPersistanceStart);
	} else {
		/* If asked to reset the FP sensor, do so */
		if (retVal == CV_FP_RESET) {
			/* restart capture, but don't re-enable h/w finger detect until after FP device has been reinitialized */
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
			fpSensorReset();
            /* Enable interrupt */
            lynx_set_fp_det_interrupt(1);
			goto wait_on_mutex;
		}
		/* capture not successful, determine if capture needs to be terminated or restarted */
		fpCancel();
		if (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_TERMINATE)
		{
			/* terminate capture */
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURE_IN_PROGRESS;
		} 
	}

	/* delete this task and mutex */
err_exit:
	/* schedule post-processing of image (if any required) and INT IN to host */
	/* don't do this if FP reset underway, since that will restart capture */
	/* also don't do this for internal FP captures, which are due to PBA */
	if (!isCancelled && retVal != CV_FP_RESET && 
		!(CV_VOLATILE_DATA->fpCaptureStatus != FP_OK && (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL)))
	{
		printf("cvFpAsyncCapture: schedule FEI and terminate capture (will be restarted if enabled)\n");
		queue_cmd(SCHED_FP_FEI_CMD, QUEUE_FROM_TASK, NULL);
	} else
    {
        // Allow the sleep mode, if capture failed or cancelled
        lynx_config_sleep_mode(1);
    }
	if (CV_VOLATILE_DATA->fingerDetectMutexHandle != NULL)
	{
		printf("cvFpAsyncCapture: delete mutex\n");
		QUEUE_DELETE(CV_VOLATILE_DATA->fingerDetectMutexHandle);
		CV_VOLATILE_DATA->fingerDetectMutexHandle = NULL;
	}
	printf("cvFpAsyncCapture: delete task\n");
	CV_VOLATILE_DATA->highPrioFPCaptureTaskHandle = NULL;
	TASK_DELETE(NULL);
}

void
cvFpAsyncCapturePoll(void)
{
	/* This routine is called from the scheduler as a result of a s/w finger detect polling interrupt.  */
	/* It will check to see if a finger is present and if so, do the FP capture and associated post-processing */
	uint32_t imageWidth;
	fp_status fpRetVal = FP_OK;
	cv_status retVal = CV_SUCCESS;
	uint32_t timeout = 0;

	/* first detect if finger present */
	printf("cvFpAsyncCapturePoll: entry\n");

    if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
    {
        if( ((CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_USED_MASK) == CV_POA_AUTH_USED_MASK) || (cv_get_timer2_elapsed_time(CV_VOLATILE_DATA->poaAuthTimeout) > CV_POA_AUTH_TIMEOUT))
        {
            CV_VOLATILE_DATA->poaAuthSuccessFlag = CV_POA_AUTH_NOT_DONE;
            CV_VOLATILE_DATA->poaAuthSuccessTemplate = 0;
            printf("POA SSO: Timeout happened or Retried 3 times, so clean the cache \n");
        }   else
        {
            // mark the credential has been used one more time
            CV_VOLATILE_DATA->poaAuthSuccessFlag = (CV_VOLATILE_DATA->poaAuthSuccessFlag << 1) | CV_POA_AUTH_SUCCESS_COMPLETE;
            printf("POA SSO: Skip fp detection and capture\n");
            goto do_postproc;
        }
    }

	printf("Calling fpGrab\n");
    fpRetVal = fpGrab((uint32_t *)NULL, (uint32_t *)NULL, PBA_FP_DETECT_TIMEOUT, 
        	NULL, NULL, FP_POLLING_PERIOD, FP_FINGER_DETECT);
	printf("cvFpAsyncCapturePoll: fpGrab complete 0x%x\n", fpRetVal);
	if (fpRetVal == FP_SENSOR_RESET_REQUIRED)
	{
		printf("cvFpAsyncCapturePoll: ESD occurred during fpGrab, restart\n");
		if (!((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) || is_fp_synaptics_present_with_polling_support() || (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH )) ) {
			disable_low_res_timer_int(cvFpSwFdetTimerISR);				// If upek touch sensor, do not stop timer.
		}
		fpSensorReset();
		retVal = cvXlatefpRetCode(fpRetVal);
		CV_VOLATILE_DATA->fpCaptureStatus = fpRetVal;
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
		fpSensorReset();
		return;
	} else if (fpRetVal != FP_OK)
		return;

	/* do capture */
	printf("cvFpAsyncCapturePoll: finger detected\n");
	if((VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH ) || is_fp_synaptics_present_with_polling_support())
		timeout = PBA_FP_DETECT_TIMEOUT; 
	
	fpRetVal = fpGrab(&CV_VOLATILE_DATA->fpPersistedFPImageLength, &imageWidth,timeout ,NULL, NULL, 0, FP_GRAB);
	retVal = cvXlatefpRetCode(fpRetVal);
	CV_VOLATILE_DATA->fpCaptureStatus = fpRetVal;
	/* check to see if got success return from fpGrab, but length or width are 0.  if so, ignore this poll */
	if (retVal == CV_SUCCESS && (CV_VOLATILE_DATA->fpPersistedFPImageLength == 0 || imageWidth == 0))
	{
		printf("cvFpAsyncCapturePoll: spurious finger detect, ignore\n");
		return;
	}

	if (retVal == CV_SUCCESS)
	{
		printf("cvFpAsyncCapturePoll: capture successful\n");
		/* capture successful.  set flags and persistence time to indicate image available */
		CV_VOLATILE_DATA->CVDeviceState |= (CV_FP_CAPTURED_IMAGE_VALID | CV_HAS_CAPTURE_ID);
		get_ms_ticks(&CV_VOLATILE_DATA->fpPersistanceStart);
		/* disable finger detect */
		disable_low_res_timer_int(cvFpSwFdetTimerISR);
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURE_IN_PROGRESS;
	} else {
		printf("cvFpAsyncCapturePoll: capture unsuccessful 0x%x\n", retVal);
		/* If asked to reset the FP sensor, do so */
		if (retVal == CV_FP_RESET) {
			disable_low_res_timer_int(cvFpSwFdetTimerISR);
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
			fpSensorReset();
			return;
		}
		/* capture not successful, determine if capture needs to be terminated or restarted */
		if (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_TERMINATE)
		{
			/* terminate capture */
			disable_low_res_timer_int(cvFpSwFdetTimerISR);
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURE_IN_PROGRESS;
		} 
	}

	/* do post-processing of image (if any required) and INT IN to host */
do_postproc:
	cvFpFEandInt();
}

void
cvFpCancel(cv_bool fromISR)
{
	/* This routine will cancel any FP capture */

	printf("cvFpCancel: %x\n",fromISR);
	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS))
	{
		/* clear reinit flag,in case this is why cvFpCancel was called */
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_REINIT_CANCELLATION;
		/* don't clear capture status if a cancel was previously done, because the async FP task */
		/* may not have run yet to process the cancellation */
		if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_CANCELLED))
			CV_VOLATILE_DATA->fpCaptureStatus = 0;
		return;
	}

	/* check to see if device supports h/w finger detect */
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURE_CANCELLED;
	if (!((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) || is_fp_synaptics_present_with_polling_support() || (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH)))
	{
		/* h/w finger detect. trigger async FP capture task to run to do cancellation */
		if (CV_VOLATILE_DATA->fingerDetectMutexHandle != NULL)
		{
			CV_VOLATILE_DATA->fpCaptureStatus = FP_STATUS_OPERATION_CANCELLED;
			if (fromISR)
			{
				printf("cvFpCancel: trigger mutex from ISR\n",fromISR);
				MUTEX_GIVE_FROM_ISR(CV_VOLATILE_DATA->fingerDetectMutexHandle, pdFALSE);
			} else {
				printf("cvFpCancel: trigger mutex not from ISR\n",fromISR);
				MUTEX_GIVE(CV_VOLATILE_DATA->fingerDetectMutexHandle);
			}
		}
	} else {
		/* no h/w finger detect */
		disable_low_res_timer_int(cvFpSwFdetTimerISR);
	}
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURE_IN_PROGRESS;
	/* clear flag to restart h/w finger detect, in case it was set */
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_RESTART_HW_FDET;
}


uint8_t
cvFpSwFdetTimerISR(void)
{
	/* this is the ISR for the timer interrupt used for software polling of FP sensor */
	uint32_t xTaskWoken = FALSE;
	uint8_t  status = USH_RET_TIMER_INT_OK;

	push_glb_regs();
	reinitialize_function_table();

	xTaskWoken = queue_cmd( SCHED_FP_CMD, QUEUE_FROM_ISR, xTaskWoken);
	/* If a task needs be scheduled, we may need to switch to another task. */
	status = USH_RET_TIMER_INT_SCHEDULE_EVENT;

	pop_glb_regs();
	return status;
}

uint8_t
fpFingerDetectCallback(uint8_t status, void *context)
{
    /* disable FP interrupt. could be disabled already. no hurt to do it again */
    if( FP_SUPPORT_INTR() ) 
    {
        printf("fpFingerDetectCallback: disable fp interrupt\n");
        lynx_set_fp_det_interrupt(0);
    }

	/* this callback is in response to a h/w finger detect interrupt */
	/* and will wake up the async FP capture task, using a mutex */
	
	printf("fpFingerDetectCallback: 0x%x\n",status);
	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS))
	{
		/* no active capture, just ignore */
		printf("fpFingerDetectCallback: no active capture\n");
        /* allow the sleep mode */
        lynx_config_sleep_mode(1);
		return 0;
	}
	/* now determine what action to take based on the status */
	switch (status)
	{
	case FP_OK:
		/* this is the normal case */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
		break;
	case FP_SENSOR_RESET_REQUIRED:
	case FP_STATUS_OPERATION_CANCELLED:
	case FP_NO_FINGER_DETECTED:
		CV_VOLATILE_DATA->fpCaptureStatus = FP_SENSOR_RESET_REQUIRED;
		/* check to see if in reset.  if so, skip this one and let the one in progress finish */
		if (usb_fp_dev() == NULL)
		{
			printf("fpFingerDetectCallback: FP reset in progress, ignore\n");
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_TERMINATE_ASYNC;
		}
		break;
	case FP_VALIDITY_ERR_IMAGE_QUALITY_BAD_SWIPE:
		/* this is for other failures that might occur during detection.  need to cancel and restart detection */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_VALIDITY_ERR_IMAGE_QUALITY_BAD_SWIPE;
		break;
	default:
		/* just handle other cases as normal */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
	}

	/* defer wakeup of async FP capture until return from USB BG poll, to avoid reentancy problems */
	CV_VOLATILE_DATA->fingerDetectStatusForWakeup = CV_VOLATILE_DATA->fpCaptureStatus;
	CV_VOLATILE_DATA->fingerDetectMutexHandleForWakeup = CV_VOLATILE_DATA->fingerDetectMutexHandle;

	return 0;
}

void
cvFpFEandInt(void)
{
	/* This routine is run as part of normal command processing either called directly from the */
	/* scheduler immediately after an asynchronous FP capture has been completed or when called by */
	/* cvFpAsyncCapturePoll (FP capture for s/w finger detect)*/
	cv_status retVal = CV_SUCCESS;

	/* If poa auth success flag is enabled then do not kick off the grab */
	if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
	{
		printf("POA SSO: cvFpFEandIn Ignore the Extraction %x \n",CV_VOLATILE_DATA->fpCapControl);
		cvHostFPAsyncInterrupt();
        CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURE_IN_PROGRESS);
		return;
	}

	/* first check to see if last capture was successful and FE is requested */
	printf("cvFpFEandInt: start - cap status 0x%x\n", CV_VOLATILE_DATA->fpCaptureStatus);
	if ((CV_VOLATILE_DATA->fpCaptureStatus == FP_OK) && (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_DO_FE))
	{
		if (CHIP_IS_5882_OR_LYNX)
		{
			printf("cvFpFEandInt: do on-chip FE - image length 0x%x\n",CV_VOLATILE_DATA->fpPersistedFPImageLength);
			CV_VOLATILE_DATA->fpPersistedFeatureSetLength = MAX_FP_FEATURE_SET_SIZE;

			if (is_fp_synaptics_present())
			{
				retVal = cvSynapticsOnChipFeatureExtraction(&CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP);
			}
			else
			{
				retVal = cvDPFROnChipFeatureExtraction(CV_VOLATILE_DATA->fpPersistedFPImageLength,
					CV_VOLATILE_DATA->fpCapControl & (CV_FE_CONTROL_PURPOSE_ENROLLMENT | CV_FE_CONTROL_PURPOSE_VERIFY | CV_FE_CONTROL_PURPOSE_IDENTIFY), /* only purpose bits are used */
					&CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP);
			}
		} else
			retVal = cvHostFeatureExtraction(CV_VOLATILE_DATA->fpPersistedFPImageLength, 
				&CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP);
		/* Clear bit CV_FP_CAPTURED_IMAGE_VALID in device status, since the feature extraction process invalidates the persisted FP image */
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID;
		/* set CV_FP_CAPTURED_FEATURE_SET_VALID if FE succeeded */
		printf("cvFpFEandInt: FE result 0x%x\n", retVal);
		if (retVal == CV_SUCCESS)
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURED_FEATURE_SET_VALID;
	}

	/* set up status for host, if not success */
	if (CV_VOLATILE_DATA->fpCaptureStatus != FP_OK || retVal != CV_SUCCESS)
	{
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID; 
		/* set failed status to return to host */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_STATUS_OPERATION_CANCELLED; /* this will get translated to CV_CAP_RSLT_POOR_QUALITY */
	}


	/* Call cvHostFPAsyncInterrupt to send the INT IN to the host */
	/* don't do this for internal async FP capture */
	if (!(CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL)) {
    
       if( usbdResumeWithTimeout() )
       {
         /* If cannot resume the usbd device, won't send msg to host */
    		cvHostFPAsyncInterrupt();
       }
       // Allow the sleep mode after talking to the host
       lynx_config_sleep_mode(1);
    }
	/* check to see if capture needs to be restarted */
	if (CV_VOLATILE_DATA->fpCaptureStatus != FP_OK || retVal != CV_SUCCESS)
	{
		/* check to see if capture needs to start over */
		if (!(CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_TERMINATE))
		{
			/* restart capture */
			printf("cvFpFEandInt: restart capture\n");
			/* enable finger detect */
			/* first, check to see if previous async FP task needs to be terminated */
			prvCheckTasksWaitingTermination();
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURE_IN_PROGRESS;
			/* reinitialize capture status.  do this before call to cvUSHFpEnableFingerDetect, because  */
			/* cvUSHFpEnableFingerDetect might trigger FP callback and set the FP capture status */
			CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
			if (cvUSHFpEnableFingerDetect(TRUE) == CV_FP_RESET)
			{
				/* reset sensor */
				printf("cvFpFEandInt: ESD occurred during cvUSHFpEnableFingerDetect, restart\n");
				CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
				fpSensorReset();
				retVal = CV_SUCCESS;
			}
		} else {
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_HAS_CAPTURE_ID;
		}
	} else
		/* reinitialize capture status */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
}

cv_status
cvUSHXmit(cv_host_control_header_in *hdrIn, cv_host_control_header_out *hdrOut, cv_usb_interrupt *usbInt)
{
	/* this routine transmits a host control request to the host and receives the result */
	cv_status retVal = CV_SUCCESS;
	uint8_t *HMACptr = (uint8_t *)(hdrIn + 1);
	uint32_t HMACLen = hdrIn->HMACLen;
	uint8_t *bufPtr = HMACptr + HMACLen;  /* points to nonce initially */
	uint8_t HMAC[SHA256_LEN], HMACRes[SHA256_LEN];
	uint32_t startTime;
	uint32_t headerInLen = hdrIn->transportLen;
	uint32_t hdrWoAuth = sizeof(cv_host_control_feature_extraction_in) + HMACLen;
	uint32_t hdrInWoAuth = hdrWoAuth + sizeof(uint32_t);
	uint32_t intXferSize = 0;

	/* zero HMAC field (for HMAC computation) */
	memset(HMACptr,0,HMACLen);
	/* compute nonce to be used for this request */
	if ((retVal = cvRandom(HMACLen, (uint8_t *)&CV_VOLATILE_DATA->fpHostControlNonce[0])) != CV_SUCCESS)
		goto err_exit;
	memcpy(bufPtr, (uint8_t *)&CV_VOLATILE_DATA->fpHostControlNonce[0], HMACLen);
	/* now compute HMAC of transfer, using shared secret as key */
	if ((retVal = cvAuth((uint8_t *)(hdrIn) + hdrWoAuth, (hdrIn->transportLen-hdrWoAuth), CV_PERSISTENT_DATA->secureAppSharedSecret, HMAC, HMACLen, 
		NULL, 0, 0)) != CV_SUCCESS)
			goto err_exit;
	/* copy HMAC into buffer */
	memcpy(HMACptr, HMAC, HMACLen);

	/* first post buffer to host to receive data from host.  it's ok to use the same buffer that is posted */
	/* as output to host, because no data from host will be received until host storage request has been processed */
	/* set up interrupt after command */

	usbInt->interruptType = CV_HOST_CONTROL_INTERRUPT;
	usbInt->resultLen = hdrIn->transportLen;
	
	/* before switching to open mode, need to make sure open window covers transfer.  There are 2 cases: */
	/* 1. i/o starts at start of FP i/o buf -> clear portion after i/o transfer */
	/* 2. i/o starts beyond start of FP i/o buf -> clear portion before i/o */
	/* in both cases, it's necessary to move the end window to end of i/o */
	/* additionally, if this transfer occurs as part of a CV command event (vs an async FP event), it will */
	/* be necessary to move the start of the async FP buffer, because the CV io buf may be in secure mode */
	set_smem_openwin_end_addr(CV_IO_OPEN_WINDOW, (uint8_t *)hdrIn + headerInLen);
	/* don't zero INT IN buf if it's appended to FP async buf */
	if ((uint8_t *)usbInt > CV_SECURE_IO_FP_ASYNC_BUF && (uint8_t *)usbInt < (CV_SECURE_IO_FP_ASYNC_BUF + CV_IO_COMMAND_BUF_SIZE))
		intXferSize = sizeof(cv_usb_interrupt);
	if ((uint8_t *)hdrIn == CV_SECURE_IO_FP_ASYNC_BUF)
	{
		/* case 1 */
		memset(CV_SECURE_IO_FP_ASYNC_BUF + headerInLen + intXferSize, 0, CV_IO_COMMAND_BUF_SIZE - (headerInLen + intXferSize));
	} else if ((uint8_t *)hdrIn > CV_SECURE_IO_FP_ASYNC_BUF) {
		/* case 2 */
		//memset(CV_SECURE_IO_FP_ASYNC_BUF, 0, CV_IO_COMMAND_BUF_SIZE - headerInLen);
	} else {
		/* error case */
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	/* now check to see if processing a CV command.  if so, move start of io window to start of FP async buf */
	/* also, set window to open (if not processing CV command, windows will already be open) */
	if (!(CV_VOLATILE_DATA->asyncFPEventProcessing))
	{
		set_smem_openwin_base_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_FP_ASYNC_BUF);
		set_smem_openwin_open(CV_IO_OPEN_WINDOW);
	}
	
	if (!usbCvSend(usbInt, (uint8_t *)hdrIn, headerInLen, HOST_CONTROL_REQUEST_TIMEOUT)) {
		retVal = CV_HOST_CONTROL_REQUEST_TIMEOUT;
		goto err_exit;
	}

	usbCvBulkOut((cv_encap_command *)hdrOut);

	/* now wait for result from host */
	get_ms_ticks(&startTime);
	do
	{
		if(cvGetDeltaTime(startTime) >= HOST_CONTROL_REQUEST_TIMEOUT)
		{
			retVal = CV_HOST_CONTROL_REQUEST_TIMEOUT;
			goto err_exit;
		}
		yield_and_delay(100);
		
		retVal = usbCvPollCmdRcvd();
		if (retVal)
			break;
	} while (TRUE);

	/* check to see if request was successful */
	if (hdrOut->status != FR_OK)
	{
		retVal = cvXlateDPRetCode(hdrOut->status);
		goto err_exit;
	}
	/* check integrity of result before using */
	if (hdrOut->HMACLen != HMACLen)
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	bufPtr = (uint8_t *)(hdrOut + 1);
	memcpy(HMACRes, bufPtr, HMACLen);
	memset(bufPtr,0,HMACLen);
	/* now compute HMAC of transfer, using shared secret as key */
	if ((retVal = cvAuth((uint8_t *)(hdrOut) + hdrInWoAuth, (hdrOut->transportLen-hdrInWoAuth), CV_PERSISTENT_DATA->secureAppSharedSecret, HMAC, HMACLen, 
		NULL, 0, 0)) != CV_SUCCESS)
			goto err_exit;

#ifndef DEBUG_WO_HOST_AUTH			
	if (memcmp(HMAC, HMACRes, HMACLen))
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}

	bufPtr += HMACLen;
	/* make sure the nonce is the same */
	if (memcmp(bufPtr, (uint8_t *)&CV_VOLATILE_DATA->fpHostControlNonce[0], HMACLen))
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
#endif	
	
err_exit:
	/* now check to see if processing a CV command.  if so, move start of io window back to start of CV io buf */
	if (!(CV_VOLATILE_DATA->asyncFPEventProcessing))
	{
		set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
		set_smem_openwin_base_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF);
	}
	/* set end window back to end of CV io buf */
	set_smem_openwin_end_addr(CV_IO_OPEN_WINDOW, CV_SECURE_IO_COMMAND_BUF + CV_IO_COMMAND_BUF_SIZE);
	return retVal;
}

cv_status
cvHostFPAsyncInterrupt(void)
{
    /* resume the usb interface for polling sensor */
    if ( !fpSensorSupportInterrupt() ) usbdResume();

    /* this routine sends an interrupt to host for an async fingerprint detection */
	cv_usb_interrupt *usbInt = (cv_usb_interrupt *)CV_SECURE_IO_FP_ASYNC_BUF;
	cv_capture_result_type *resultType;

	printf("cvHostFPAsyncInterrupt: start - capture status 0x%x\n", CV_VOLATILE_DATA->fpCaptureStatus);
	usbInt->interruptType = CV_FINGERPRINT_EVENT_INTERRUPT;
	/* set resultLen field with FP capture status */
	resultType = (cv_capture_result_type *)&usbInt->resultLen;
	*resultType = cvXlatefp2wbfRetCode(CV_VOLATILE_DATA->fpCaptureStatus);
	/* check to see if capture was cancelled */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_CANCELLED)
		*resultType = CV_CAP_RSLT_CANCELLED;
	usbCvInterruptIn(usbInt);
//  Remove the delay for better FP performance
//	ms_wait(100);
	printf("cvHostFPAsyncInterrupt: end\n");
	return CV_SUCCESS;
}

cv_status
cvHostFeatureExtraction(uint32_t imageLength, uint32_t *featureSetLength, uint8_t *featureSet)
{
	/* this routine sends image to host and receives resultant feature set */
	/* image pointer isn't passed in, because image is always save at same RAM location */
	cv_host_control_feature_extraction_in *feReq;
	cv_host_control_feature_extraction_out *feRes = (cv_host_control_feature_extraction_out *)CV_SECURE_IO_FP_ASYNC_BUF;
	uint32_t HMACLen = (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS) ? SHA256_LEN : SHA1_LEN;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;
	uint8_t *bufPtr;
	uint32_t *imageLenPtr;
	uint32_t headerInLen;

	/* set up host control request */
	/* compute header length.  header will be placed just before image start */
	headerInLen = sizeof(cv_host_control_feature_extraction_in) + sizeof(uint32_t) + 2*HMACLen;
	feReq = (cv_host_control_feature_extraction_in *)(FP_CAPTURE_LARGE_HEAP_PTR - ALIGN_DWORD(headerInLen));
	feReq->hostControlHdr.transportType = CV_TRANS_TYPE_HOST_CONTROL;
	feReq->hostControlHdr.commandID = CV_HOST_CONTROL_FEATURE_EXTRACTION;
	feReq->hostControlHdr.transportLen = headerInLen + imageLength;
	feReq->hostControlHdr.HMACLen = HMACLen;
	imageLenPtr = (uint32_t *)(FP_CAPTURE_LARGE_HEAP_PTR - sizeof(uint32_t));
	*imageLenPtr = imageLength;

	/* now transmit buffer */
	usbInt = (cv_usb_interrupt *)((uint8_t *)FP_CAPTURE_LARGE_HEAP_PTR - ALIGN_DWORD(headerInLen) - sizeof(cv_usb_interrupt));
	if ((retVal = cvUSHXmit(&feReq->hostControlHdr, &feRes->hostControlHdr, usbInt)) != CV_SUCCESS)
		goto err_exit;
	/* process host control result */
	if (feRes->hostControlHdr.transportLen <= sizeof(cv_host_control_feature_extraction_out))
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	bufPtr = (uint8_t *)(feRes + 1) + 2*HMACLen;
	*featureSetLength = *((uint32_t *)bufPtr);
	if (*featureSetLength > MAX_FP_FEATURE_SET_SIZE)
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	bufPtr += sizeof(uint32_t);
	memcpy(featureSet, bufPtr, *featureSetLength);

err_exit:
	return retVal;

}

cv_status
cvHostIdentify(uint32_t templatesLength, uint32_t *identifiedTemplateIndex, uint8 **hint)
{
	/* this routine sends a feature set and set of templates to host and receives template */
	/* identified as matching one */
	cv_host_control_identify_and_hint_creation_in *idReq;
	cv_host_control_identify_and_hint_creation_out *idRes = (cv_host_control_identify_and_hint_creation_out *)CV_SECURE_IO_FP_ASYNC_BUF;
	uint32_t HMACLen = (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS) ? SHA256_LEN : SHA1_LEN;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;
	uint8_t *bufPtr;
	uint32_t headerInLen;
	uint32_t hintLength;

	/* set up host control request */
	/* compute header length.  header will be placed just before image start */
	headerInLen = sizeof(cv_host_control_identify_and_hint_creation_in) + 2*HMACLen;
	idReq = (cv_host_control_identify_and_hint_creation_in *)(FP_CAPTURE_LARGE_HEAP_PTR - headerInLen);
	idReq->hostControlHdr.transportType = CV_TRANS_TYPE_HOST_CONTROL;
	idReq->hostControlHdr.commandID = CV_HOST_CONTROL_IDENTIFY_AND_HINT_CREATION;
	idReq->hostControlHdr.transportLen = headerInLen + templatesLength;
	idReq->hostControlHdr.HMACLen = HMACLen;
	/* now transmit buffer */
	usbInt = (cv_usb_interrupt *)((uint8_t *)idReq - sizeof(cv_usb_interrupt));
	if ((retVal = cvUSHXmit(&idReq->hostControlHdr, &idRes->hostControlHdr, usbInt)) != CV_SUCCESS)
		goto err_exit;
	/* process host control result */
	if (idRes->hostControlHdr.transportLen <= sizeof(cv_host_control_identify_and_hint_creation_out))
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	bufPtr = (uint8_t *)(idRes + 1) + 2*HMACLen;
	hintLength = *((uint32_t *)bufPtr);
	/* verify hint length not too long */
	if (hintLength > CV_MAX_DP_HINT)
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	*hint = bufPtr;			/* Point to start of hint... which is hint length */
	bufPtr += sizeof(uint32_t);
	bufPtr += hintLength;
	*identifiedTemplateIndex = *((uint32_t *)bufPtr);

err_exit:
	return retVal;
}

cv_status
cvHostCreateTemplate(uint32_t featureSet1Length, uint8_t *featureSet1,
					 uint32_t featureSet2Length, uint8_t *featureSet2,
					 uint32_t featureSet3Length, uint8_t *featureSet3,
					 uint32_t featureSet4Length, uint8_t *featureSet4,
					 uint32_t *templateLength, cv_obj_value_fingerprint **createdTemplate)
{
	/* this routine sends feature sets to host and receives template */
	cv_host_control_create_template_in *ctReq = (cv_host_control_create_template_in *)CV_SECURE_IO_FP_ASYNC_BUF;
	cv_host_control_create_template_out *ctRes = (cv_host_control_create_template_out *)CV_SECURE_IO_FP_ASYNC_BUF;
	uint32_t HMACLen = (CV_PERSISTENT_DATA->persistentFlags & CV_USE_SUITE_B_ALGS) ? SHA256_LEN : SHA1_LEN;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;
	uint8_t *bufPtr;
	uint32_t headerInLen;
	uint32_t templateResultLen;
	cv_obj_value_fingerprint *objPtr;

	/* set up host control request */
	/* compute header length */
	headerInLen = sizeof(cv_host_control_create_template_in) + 2*HMACLen;
	ctReq->hostControlHdr.transportType = CV_TRANS_TYPE_HOST_CONTROL;
	ctReq->hostControlHdr.commandID = CV_HOST_CONTROL_CREATE_TEMPLATE;
	ctReq->hostControlHdr.transportLen = headerInLen + 4*sizeof(uint32_t) + featureSet1Length + featureSet2Length
		 + featureSet3Length + featureSet4Length;
	ctReq->hostControlHdr.HMACLen = HMACLen;
	bufPtr = (uint8_t *)ctReq + headerInLen;
	/* now copy feature sets into buffer */
	*((uint32_t *)bufPtr) = featureSet1Length;
	bufPtr += sizeof(uint32_t);
	memcpy(bufPtr, featureSet1, featureSet1Length);
	bufPtr += featureSet1Length;
	*((uint32_t *)bufPtr) = featureSet2Length;
	bufPtr += sizeof(uint32_t);
	memcpy(bufPtr, featureSet2, featureSet2Length);
	bufPtr += featureSet2Length;
	*((uint32_t *)bufPtr) = featureSet3Length;
	bufPtr += sizeof(uint32_t);
	memcpy(bufPtr, featureSet3, featureSet3Length);
	bufPtr += featureSet3Length;
	*((uint32_t *)bufPtr) = featureSet4Length;
	bufPtr += sizeof(uint32_t);
	memcpy(bufPtr, featureSet4, featureSet4Length);
	bufPtr += featureSet4Length;
	/* now transmit buffer */
	usbInt = (cv_usb_interrupt *)((uint8_t *)ctReq + ALIGN_DWORD(ctReq->hostControlHdr.transportLen));
	if ((retVal = cvUSHXmit(&ctReq->hostControlHdr, &ctRes->hostControlHdr, usbInt)) != CV_SUCCESS)
		goto err_exit;
	/* process host control result */
	if (ctRes->hostControlHdr.transportLen <= sizeof(cv_host_control_create_template_out))
	{
		retVal = CV_HOST_CONTROL_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	bufPtr = (uint8_t *)(ctRes + 1) + 2*HMACLen;
	templateResultLen = *((uint32_t *)bufPtr);
	bufPtr += sizeof(uint32_t);
	/* now put template object header in just before template */
	objPtr = (cv_obj_value_fingerprint *)(bufPtr - (sizeof(cv_obj_value_fingerprint) - 1));
	objPtr->type = CV_FINGERPRINT_TEMPLATE;
	objPtr->fpLen = templateResultLen;
	*createdTemplate = objPtr;
	*templateLength = templateResultLen;

err_exit:
	return retVal;
}

					 
cv_status
localCallback(fp_callback_type callbackType, uint32_t extraDataLength, void *extraData, void *arg)
{
	/* this is the callback from the fpGrab routine.  need to xlate status into CV status */
	cv_fp_callback_ctx *savedContext;
	uint32_t toHostStatus, fromHostStatus, toFPDeviceStatus;

	savedContext = (cv_fp_callback_ctx *)arg;
	/* just return if no callback */
	if (savedContext->callback == NULL)
		return CV_SUCCESS;

	if (callbackType == FP_GRAB || callbackType == FP_FINGER_DETECT)
	{
		if (CV_PERSISTENT_DATA->persistentFlags & (CV_HAS_FP_AT_SWIPE | CV_HAS_FP_UPEK_SWIPE | CV_HAS_VALIDITY_SWIPE)) {
			switch(savedContext->swipe)
			{
			case CV_SWIPE_NORMAL:
				toHostStatus = CV_PROMPT_FOR_FINGERPRINT_SWIPE;
				break;
			case CV_SWIPE_FIRST:
				toHostStatus = CV_PROMPT_FOR_FIRST_FINGERPRINT_SWIPE;
				break;
			case CV_SWIPE_SECOND:
				toHostStatus = CV_PROMPT_FOR_SECOND_FINGERPRINT_SWIPE;
				break;
			case CV_SWIPE_THIRD:
				toHostStatus = CV_PROMPT_FOR_THIRD_FINGERPRINT_SWIPE;
				break;
			case CV_SWIPE_FOURTH:
				toHostStatus = CV_PROMPT_FOR_FOURTH_FINGERPRINT_SWIPE;
				break;
			case CV_RESWIPE:
				toHostStatus = CV_PROMPT_FOR_RESAMPLE_SWIPE;
				break;
			case CV_SWIPE_TIMEOUT:
				toHostStatus = CV_PROMPT_FOR_RESAMPLE_SWIPE_TIMEOUT;
				break;
			default:
				toHostStatus = CV_REMOVE_PROMPT;	/* This should not happen, just remove prompt */
				break;
			}
		}
		else if ((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) || (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH) ||
			     (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_NEXT_TOUCH) || (is_fp_synaptics_present()))
			switch(savedContext->swipe)
			{
			case CV_SWIPE_NORMAL:
				toHostStatus = CV_PROMPT_FOR_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_FIRST:
				toHostStatus = CV_PROMPT_FOR_FIRST_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_SECOND:
				toHostStatus = CV_PROMPT_FOR_SECOND_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_THIRD:
				toHostStatus = CV_PROMPT_FOR_THIRD_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_FOURTH:
				toHostStatus = CV_PROMPT_FOR_FOURTH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_FIFTH:
				toHostStatus = CV_PROMPT_FOR_FIFTH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_SIXTH:
				toHostStatus = CV_PROMPT_FOR_SIXTH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_SEVENTH:
				toHostStatus = CV_PROMPT_FOR_SEVENTH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_EIGHTH:
				toHostStatus = CV_PROMPT_FOR_EIGHTH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_NINETH:
				toHostStatus = CV_PROMPT_FOR_NINETH_FINGERPRINT_TOUCH;
				break;
			case CV_SWIPE_TENTH:
				toHostStatus = CV_PROMPT_FOR_TENTH_FINGERPRINT_TOUCH;
				break;
			case CV_RESWIPE:
				toHostStatus = CV_PROMPT_FOR_RESAMPLE_TOUCH;
				break;
			case CV_SWIPE_TIMEOUT:
				toHostStatus = CV_PROMPT_FOR_RESAMPLE_TOUCH_TIMEOUT;
				break;
			default:
				toHostStatus = CV_REMOVE_PROMPT;	/* This should not happen, just remove prompt */
				break;
			}
		else
			/* this shouldn't happen, because one of the FP devices should be present */
			toHostStatus = CV_REMOVE_PROMPT;
	} else
		/* if unexpected error occurs, just remove prompt, if any */
		toHostStatus = CV_REMOVE_PROMPT;
	fromHostStatus = (*savedContext->callback)(toHostStatus, 0, NULL, savedContext->context);
	/* now xlate back to FP device */
	if (fromHostStatus == CV_SUCCESS)
		toFPDeviceStatus = FP_OK;
	else if (fromHostStatus == CV_CANCELLED_BY_USER)
	{
		toFPDeviceStatus = FP_STATUS_OPERATION_CANCELLED;
		fpCancel();
	}
	else
	{
		/* if unexpected error occurs, just cancel */
		toFPDeviceStatus = FP_STATUS_OPERATION_CANCELLED;
		fpCancel();
	}
	return toFPDeviceStatus;
}


cv_status
cvUSHfpCapture(cv_bool doGrabOnly, cv_bool doFingerDetect, cv_fp_control fpControl, 
			   uint8_t * image, uint32_t *imageLength, uint32_t *imageWidth, uint32_t swipe,
			   cv_callback *callback, cv_callback_ctx context)
{
	/* This routine is invoked for doing a fingerprint capture.  It can be invoked to do the */
	/* capture and feature extraction, or it can just do the capture, in the case where the capture */
	/* is being done as part of an identify or verify function. */
	cv_status retVal = CV_SUCCESS;
	fp_status fpRetVal = FP_OK;
	cv_fp_callback_ctx savedContext;
	uint32_t outputBufferLength = *imageLength;
    uint32_t timeout;

	/* save info for local callback */
	savedContext.callback = callback;
	savedContext.context = context;
	savedContext.swipe = swipe;

	/* first, check to see if there's a valid persisted feature set.  if so, use it */
	if (!doFingerDetect && (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID) && 
#ifdef PRINT_LARGE_HEAP
		(1)) {
#else
		(cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) <= (CV_PERSISTENT_DATA->fpPersistence))) {
#endif

			if (*imageLength && *imageLength < CV_VOLATILE_DATA->fpPersistedFeatureSetLength)
			{
				*imageLength = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
				retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
				goto err_exit;
			}
			memcpy(image, CV_PERSISTED_FP, CV_VOLATILE_DATA->fpPersistedFeatureSetLength);
			*imageLength = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
	} else {
		/* do capture */
		/* invoke indicated routine */
		
		/* doFingerDetect is being used a little differently. doFingerDetect now implies that 
		detection has already been done, so skip detection and grab the image */
		/* check to make sure no async FP capture is active */
		if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS)
		{
			/* if this was an internal FP capture request, just cancel it */
			if (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL)
			{
				printf("cvUSHfpCapture: calling cvFpCancel\n");
				cvFpCancel(FALSE);
			} else {
				retVal = CV_FP_DEVICE_BUSY;
				goto err_exit;
			}
		}
		/* if CV_USE_PERSISTED_IMAGE_ONLY is set, need to fail here, because persisted image wasn't available */
		/* also, don't start a capture if this is part of a command using async FP capture */
		if ((fpControl & CV_USE_PERSISTED_IMAGE_ONLY) || (fpControl & CV_ASYNC_FP_CAPTURE_ONLY))
		{
			printf("CV_NO_VALID_FP_IMAGE fpControl: %d\n", fpControl);
			retVal = CV_NO_VALID_FP_IMAGE;
			goto err_exit;
		}
#ifdef PHX_REQ_FP
        /* do capture */
        /*
           FP_POLLING_TIMEOUT = 300000 is too long for sync capture or detection.
           NEXT library seems like set a maximum polling time to about 475ms(?). 
           DP library has no limitaiton on the maximum polling time.
           It simply takes the timeout parameter of grab; for DP sensor,
           PBA_FP_DETECT_TIMEOUT = 400ms is more reasonable value for polling cycle.
        */
        if((VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH ) || is_fp_synaptics_present())
             timeout = PBA_FP_DETECT_TIMEOUT;
        else timeout = FP_POLLING_TIMEOUT;

		if (!doFingerDetect) {
			usbCvBulkOut(CV_SECURE_IO_COMMAND_BUF); //Make sure that the usb interface can receive 
			ush_CreateHighPriProcessTask();
			if ((fpRetVal = fpGrab((uint32_t *)NULL, 
						(uint32_t *)NULL, 
						FP_POLLING_TIMEOUT, 
						(fpapi_callback *) &localCallback, 
						(void *)           &savedContext, 
						CV_VOLATILE_DATA->fp_polling_period,
						FP_FINGER_DETECT)) != FP_OK) {
				            ush_StopHighPriQueueing();
	                        retVal = cvXlatefpRetCode(fpRetVal);
	                        goto err_exit;
	                }
            ush_StopHighPriQueueing(); 
	    }

		if ((fpRetVal = fpGrab (imageLength, imageWidth, timeout, //FP_POLLING_TIMEOUT,
					NULL, NULL, 
					0, FP_GRAB)) != FP_OK){
			retVal = cvXlatefpRetCode(fpRetVal);
			goto err_exit;
		}
#endif
		/* now determine if image should be post processed to feature set */
		printf("cvUSHfpCapture: fpGrab length: 0x%x width: 0x%x \n", *imageLength, *imageWidth);
		if (doGrabOnly)
			printf("cvUSHfpCapture: no feature extraction\n");
		/* check to see if got success return from fpGrab, but length and width are 0.  if so, consider this a bad swipe */
		if (*imageLength == 0 && *imageWidth == 0)
		{
			retVal = CV_FP_BAD_SWIPE;
			goto err_exit;
		}
		if (!doGrabOnly)
		{
			/* yes, create feature set.  put directly into persistent storage and then copy to 'image', if not null */
			if (CHIP_IS_5882_OR_LYNX) {
				if (is_fp_synaptics_present())
				{
                    *imageLength = MAX_FP_FEATURE_SET_SIZE; /* Init to MAX_FP_FEATURE_SET_SIZE */
					if ((retVal = cvSynapticsOnChipFeatureExtraction(imageLength, CV_PERSISTED_FP)) != CV_SUCCESS)
						goto err_exit;
				}
				else
				{
					if ((retVal = cvDPFROnChipFeatureExtraction(*imageLength, 0, imageLength, CV_PERSISTED_FP)) != CV_SUCCESS)
						goto err_exit;
				}
			}
			else {
				if ((retVal = cvHostFeatureExtraction(*imageLength, imageLength, CV_PERSISTED_FP)) != CV_SUCCESS)
					goto err_exit;
			}

			if (image != NULL)
			{
				if (*imageLength && outputBufferLength < *imageLength)
				{
					/* output buffer size insufficient, fail */
					retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
					goto err_exit;
				}
				memcpy(image, CV_PERSISTED_FP, *imageLength);
			}
			/* now set up remaining values for persisted feature set */
			CV_VOLATILE_DATA->fpPersistedFeatureSetLength = *imageLength;
			get_ms_ticks(&CV_VOLATILE_DATA->fpPersistanceStart);
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURED_FEATURE_SET_VALID; 
		} else {
			/* if image returned instead of feature set, the image isn't copied, since the calling app will */
			/* know the location of the image buffer */
			/* check size of output buffer available */
			//*imageLength = 32 * 1024;
			if (outputBufferLength < *imageLength)
			{
				/* output buffer size insufficient, fail */
				retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
				goto err_exit;
			}
		}
	}


err_exit:
	/* If asked to reset the FP sensor, do so */
	if (retVal == CV_FP_RESET) {
		/*fpClose();*/
		fpSensorReset();
	}
	/* reenable h/w or s/w finger detection if capture done, but not if just doing finger detect, because */
	/* may be in polling loop with finger detect turned off */
#if 0
	if (!doFingerDetect)
		cvUSHfpEnableHWFingerDetect(TRUE);
#endif

	ush_DeleteHighPriTask();

	return retVal;
}

cv_status
fpVerify(uint8_t *hint, int32_t FARvalue, uint32_t featureSetLength, uint8_t *featureSet, uint32_t templateLength, uint8_t *fpTemplate, cv_bool *match)
{
	/* This routine is a wrapper for the DP FP match routines. */
	/* Because of the memory requirements for the FP match algorithm, this process is */
	/* mutually exclusive with the following functions: */
	/*	- FP capture */
	/*  - HID (RF) */
	/* Thus, the contexts associated with those functions will be closed before creating */
	/* the context for FP matching and then re-established when the match process is complete */
	uint32_t hintLength;
	DPFR_HANDLE hContext = NULL, hFeatureSet, hAlignment, hTemplate, hComparisonOper;
	DPFR_RESULT retVal = FR_OK;
	const size_t memory_block_control_structure_size = 8; // TODO: put correct value
	DPFR_MEMORY_REQUIREMENTS mem_req = {0};
	DPFR_CONTEXT_INIT ctx_init = {
		sizeof(DPFR_CONTEXT_INIT), // length
		FP_MATCH_HEAP_PTR,         // heapContext
		&pvPortMalloc_in_heap,     // malloc
		&vPortFree_in_heap,        // free
		(unsigned int (*)(void))&get_current_tick_in_ms    // get_current_tick_in_ms
	};
	size_t total_heap_size;

	/* shouldn't need to close context for FP capture, because FP match will only use the image portion of FP capture heap */
//	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURE_CONTEXT_OPEN | CV_HID_CONTEXT_OPEN); 
//	fpClose();

	hintLength = *((uint8_t *)hint);	/* Get hint length */
	hint += sizeof(uint32_t);			/* Point to start of hint */
	/* set up FP_MATCH_HEAP heap */
	init_heap(FP_MATCH_HEAP_SIZE, FP_MATCH_HEAP_PTR);

	/* now invoke DP routines */
	// overhead for each memory block allocated on the heap
	if ((retVal = DPFRGetMemoryRequirements(sizeof(mem_req), &mem_req)) != FR_OK)
		goto err_exit;
	total_heap_size = 1 * mem_req.context + 1 * mem_req.fpFeatureSet
		+ 1 * mem_req.fpTemplate + 1 * mem_req.alignmentData + 1 * mem_req.comparisonOperation
		+ 5 * memory_block_control_structure_size;
	if (total_heap_size > FP_MATCH_HEAP_SIZE) 
	{
		retVal = FR_ERR_NO_MEMORY;
		goto err_exit;
	}
	ENTER_CRITICAL();
	if ((retVal = DPFRCreateContext(&ctx_init, &hContext)) != FR_OK)
		goto err_exit;
	/* now import feature set, template and alignment data */
	if ((retVal = DPFRImport(hContext, featureSet, featureSetLength, DPFR_DT_DP_FEATURE_SET, DPFR_PURPOSE_VERIFICATION, &hFeatureSet)) != FR_OK)
		goto err_exit;
	if ((retVal = DPFRImport(hContext, fpTemplate, templateLength, DPFR_DT_DP_TEMPLATE, DPFR_PURPOSE_VERIFICATION, &hTemplate)) != FR_OK)
		goto err_exit;
	//if ((retVal = DPFRImport(hContext, fpTemplate, templateLength, DPFR_DT_DP_TEMPLATE_1640, DPFR_PURPOSE_VERIFICATION, &hTemplate)) != FR_OK)
	//	goto err_exit;
	if ((retVal = DPFRImport(hContext, hint, hintLength, DPFR_DT_DP_ALIGNMENT, DPFR_PURPOSE_VERIFICATION, &hAlignment)) != FR_OK)
		goto err_exit;
	/* now create comparison objectt */
	if ((retVal = DPFRCreateComparisonWithAlignment(hFeatureSet, hTemplate, hAlignment, 0, &hComparisonOper)) != FR_OK)
		goto err_exit;
	/* now get verification result */
	retVal = DPFRGetVerificationResult(hComparisonOper, FARvalue);
	if (retVal == FR_OK)
		*match = TRUE;
	else if (retVal == FR_FALSE) {
		*match = FALSE;
		retVal = CV_SUCCESS;	/* Match fails but the command is successful */
	}

err_exit:
    EXIT_CRITICAL();
	return cvXlateDPRetCode(retVal);
}

cv_status
cvUSHfpIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl, 
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context)
{
	/* This routine is invoked for doing a fingerprint identify.  It can either have a feature set provided, or it */
	/* can do a capture to get the FP image.  In the latter case, the image is sent to the host to get the feature set. */
	/* The feature set is sent along with the templates to the host to get the candidate template for verification */
	cv_status retVal = CV_SUCCESS;
	uint8_t featureSetLocal[MAX_FP_FEATURE_SET_SIZE];
	uint32_t imageLength, imageWidth;
	uint32_t i;
	uint8_t *bufPtr, *bufEnd;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_value_fingerprint *retTemplate;
	uint32_t identifiedTemplateIndex = 0;
	uint8_t *hint;
	uint8_t *templatePtrs[MAX_NUM_FP_TEMPLATES];
	uint32_t templateLengths[MAX_NUM_FP_TEMPLATES];
	uint32_t objCacheEntry;
	uint32_t index;
	
	if ( templateCount > MAX_NUM_FP_TEMPLATES )
	{
		printf("cvUSHfpIdentify() templateCount (%d) too large; can only handle %d\n", templateCount, MAX_NUM_FP_TEMPLATES);
		return CV_INVALID_INPUT_PARAMETER;
	}

	/* check to see if feature set provided or if capture necessary */
	if (!featureSetLength)
	{
		imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Init to MAX_FP_FEATURE_SET_SIZE */
		
		printf("cvUSHfpIdentify: cvUSHfpCapture\n");
		/* feature set not provided, do capture */
		if ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
					CV_SWIPE_NORMAL, callback, context)) == CV_FP_BAD_SWIPE) 
		{
			if (!fingerDetected) {
				imageLength = MAX_FP_FEATURE_SET_SIZE;		/* Reinit to MAX_FP_FEATURE_SET_SIZE */					
				while ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
						CV_RESWIPE, callback, context)) == CV_FP_BAD_SWIPE)
					imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Reinit to MAX_FP_FEATURE_SET_SIZE */
				}
				else
				return 	retVal;		
		}
		featureSet = featureSetLocal;
		featureSetLength = imageLength;
		
		if (retVal != CV_SUCCESS)
			goto err_exit;
			
	} else {
		/* copy feature set locally anyway, because may be stored in object which will get reused below for templates */
		if (featureSetLength > sizeof(featureSetLocal))
		{
			retVal = CV_INVALID_INPUT_PARAMETER;
			goto err_exit;
		}
		memcpy(featureSetLocal, featureSet, featureSetLength);
		featureSet = featureSetLocal;
	}

	/* now retrieve the templates and send to host along with feature set to do identify */
	/* use image buffer for templates */
	bufPtr = FP_CAPTURE_LARGE_HEAP_PTR;
	bufEnd = bufPtr + FP_IMAGE_MAX_SIZE;
	/* first put FAR and feature set in buffer */
	*((int32_t *)bufPtr) = FARvalue;
	bufPtr += sizeof(int32_t);
	*((uint32_t *)bufPtr) = featureSetLength;
	bufPtr += sizeof(uint32_t);
	memcpy(bufPtr, featureSet, featureSetLength);
	bufPtr += featureSetLength;
	*((uint32_t *)bufPtr) = templateCount;
	bufPtr += sizeof(uint32_t);
	/* check to see if template provided directly or if template handle list provided */
	if (templateLength)
	{
		/* this is the verify case where a single template is provided.  it still needs to be sent to the host */
		/* to have the hint generated */
		*((uint32_t *)bufPtr) = templateLength;
		bufPtr += sizeof(uint32_t);
		memcpy(bufPtr, pTemplate, templateLength);
		bufPtr += templateLength;
		templatePtrs[0] = pTemplate;
		templateLengths[0] = templateLength;
		// Setup object cache for raw template data
		cvObjCacheGetLRU(&objCacheEntry, (uint8_t **)&objPtr);
    	objProperties.objHandle = 0;	
	} else {
		for (i=0;i<templateCount;i++)
		{
			memset(&objProperties,0,sizeof(cv_obj_properties));
			objProperties.session = (cv_session *)cvHandle;
			objProperties.objHandle = templateHandles[i];
			if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
				goto err_exit;
			/* bump handle object to LRU so same buffer gets reused.  Cache only has entries for 4 objects */
			cvUpdateObjCacheLRUFromHandle(objProperties.objHandle);
			cvFindObjPtrs(&objProperties, objPtr);
			retTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
			if (retTemplate->type != getExpectedFingerprintTemplateType())
			{
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
			if ((retTemplate->fpLen + bufPtr) > bufEnd)
			{
				/* ran out of buffer space.  this shouldn't happen with expected max num of templates and max template size */
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
			/* copy template length and template into buffer */
			*((uint32_t *)bufPtr) = retTemplate->fpLen;
			bufPtr += sizeof(uint32_t);
			memcpy(bufPtr, retTemplate->fp, retTemplate->fpLen);
			templatePtrs[i] = bufPtr;
			templateLengths[i] = retTemplate->fpLen;
			bufPtr += retTemplate->fpLen;
		}
	}
	/* now send feature set and templates to host and get identified template */
	if ((retVal = cvHostIdentify((uint32_t)(bufPtr - FP_CAPTURE_LARGE_HEAP_PTR), &identifiedTemplateIndex, &hint)) != CV_SUCCESS)
		goto err_exit;

	/* copy matching template locally, so that we can use the FP capture buffer for the DP heap.  use object cache */
	/* Invalidate cache entry to reuse the buffer */
	cvFlushCacheEntry(objProperties.objHandle);
	if (cvFindVolatileDirEntry(objProperties.objHandle,&index)) 
 	{ 
		cvObjCacheGetLRU(&objCacheEntry, (uint8 **)&objPtr);
    	objProperties.objHandle = 0;
	}	
	memcpy(objPtr, templatePtrs[identifiedTemplateIndex], templateLengths[identifiedTemplateIndex]);

	/* if template identified (ie, ptr isn't null), do match */
	if ((retVal = fpVerify(hint, FARvalue, featureSetLength, featureSet, templateLengths[identifiedTemplateIndex],
		(uint8_t *)objPtr, match)) != CV_SUCCESS)
		goto err_exit;

	
	/* If match succedded, return template handle, if list of templates provided */	
	if (*match && (!templateLength && (identifiedTemplateIndex <= (templateCount - 1)))) {
			*identifiedTemplate = templateHandles[identifiedTemplateIndex];
	}
	else
			*identifiedTemplate = 0;
	
err_exit:
	return retVal;
}

cv_status
cvUSHfpCreateTemplate(uint32_t featureSet1Length, uint8_t *featureSet1,
					 uint32_t featureSet2Length, uint8_t *featureSet2,
					 uint32_t featureSet3Length, uint8_t *featureSet3,
					 uint32_t featureSet4Length, uint8_t *featureSet4,
					 uint32_t *templateLength, cv_obj_value_fingerprint **createdTemplate)
{
	/* This routine is invoked to create a template (currently only on host) */
	cv_status retVal = CV_SUCCESS;

	retVal = cvHostCreateTemplate(featureSet1Length, featureSet1,
					 featureSet2Length, featureSet2,
					 featureSet3Length, featureSet3,
					 featureSet4Length, featureSet4,
					 templateLength, createdTemplate);
	return retVal;
}

cv_status
cvUSHFpEnableFingerDetect(cv_bool enable)
{
	/* this routine enables or disables finger detect.  if FP device doesn't support h/w finger detect, */
	/* enabling will set up polling (s/w finger detect). */
	cv_status retVal = CV_SUCCESS;
	fp_status fpStatus = FP_OK;
	uint32_t flag = 0;

	if (enable)
	{
		printf("cvUSHFpEnableFingerDetect enable\n");
		/* this is enable case.  check to see if device supports h/w or s/w finger detect */
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_FINGER_DETECT_ENABLED;
		if (!((CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FP_UPEK_TOUCH) || is_fp_synaptics_present_with_polling_support() || (VOLATILE_MEM_PTR->fp_spi_flags & FP_SPI_DP_TOUCH )))
		{
			/* h/w finger detect */
			/* check to see if task/mutex already created, if not, create */
			if (CV_VOLATILE_DATA->highPrioFPCaptureTaskHandle == NULL)
			{
				if (CV_VOLATILE_DATA->fingerDetectMutexHandle == NULL)
				{
					MUTEX_CREATE(CV_VOLATILE_DATA->fingerDetectMutexHandle);
					if (CV_VOLATILE_DATA->fingerDetectMutexHandle == NULL)
					{
						retVal = CV_MUTEX_CREATION_FAILURE;
						goto err_exit;
					}
					/* take mutex here, so that when task starts it will block */
					MUTEX_TAKE_BLOCKED(CV_VOLATILE_DATA->fingerDetectMutexHandle, portMAX_DELAY);
				}
				if (TASK_CREATE( cvFpAsyncCapture, "async_fp_capture", CV_ASYNC_FP_CAPTURE_TASK_STACK_SIZE, 
					ASYNC_FP_CAP_TASK_PRIORITY, &CV_VOLATILE_DATA->highPrioFPCaptureTaskHandle, CV_ASYNC_FP_CAPTURE_TASK_STACK ) != pdPASS)
				{
					printf("cvUSHFpEnableFingerDetect: task creation failed\n");
					retVal = CV_TASK_CREATION_FAILURE;
					goto err_exit;
				}
				printf("cvUSHFpEnableFingerDetect: task creation succesful\n");
			}

#ifdef PHX_REQ_FP

#ifdef FP_UART_OFF
phx_uart_disable();
#endif
			flag = 0;
			if (is_fp_synaptics_present())
			{
				if (CV_VOLATILE_DATA->doingFingerprintEnrollmentNow)
				{
					flag = FP9_FINGER_DETECT_ENROLL;
					printf("Calling fpEnableHWFingerDetect for enrollment\n");
				}
				else
				{
					flag = FP9_FINGER_DETECT_VERIFY;
					printf("Calling fpEnableHWFingerDetect for verification\n");
				}
			}

			//
			// we are handling rebaseline - need to do this before starting finger detection
			//

			if (CV_VOLATILE_DATA->doingFingerprintEnrollmentNow) //For enrollment we should always do rebaseline
			{
				VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily = 0;
			}

			if (!(VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily))
			{
				fpNextSensorDoBaseline();
			}
			else
			{
				printf("Rebaseline done earlier\n");
			}

            /* If poa auth success flag is enabled, then do not kick off the detection  */
            if(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE)
            {
                printf("POA SSO: Ignore the finger detection\n");
                retVal = CV_SUCCESS;
                goto err_exit;
            }
			
            /* Enable FP interrupt for Next Sensor */
            if( !is_fp_synaptics_present() ) lynx_set_fp_det_interrupt(1);

			fpStatus = fpEnableHWFingerDetect(flag, fpFingerDetectCallback, 0);

			if (!(VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily))
			{
				printf("baseline fp capture started\n");
			}
			else
			{
				printf("fp capture started\n");
			}
			
			if (fpStatus != FP_OK)
			{
            	retVal = cvXlatefpRetCode(fpStatus);
                /* disable FP interrupt */
                lynx_set_fp_det_interrupt(0);
				goto err_exit;
			}
            /* Enable the interrupt for synaptics sensor */
            if( is_fp_synaptics_present() ) lynx_set_fp_det_interrupt(1);
#endif
		} else {
			/* s/w finger detect.  set up timer based task to poll for finger placement */
			/* first disable to make sure more than one timer not enabled */
			disable_low_res_timer_int(cvFpSwFdetTimerISR);
			if ((retVal = enable_low_res_timer_int(CV_PERSISTENT_DATA->fpPollingInterval, cvFpSwFdetTimerISR)) != CV_SUCCESS)
				goto err_exit;
		}
	} else {
		/* disable case */
		printf("cvUSHFpEnableFingerDetect disable: calling cvFpCancel\n");
        /*disable FP interrupt */
        if( FP_SUPPORT_INTR() ) lynx_set_fp_det_interrupt(0);
		cvFpCancel(FALSE);
		CV_VOLATILE_DATA->doingFingerprintEnrollmentNow = 0;
	}

err_exit:
#ifdef FP_UART_OFF
phx_uart_enable();
#endif

    if ( enable && (retVal == 0) )
    {   
            if( fpSensorSupportInterrupt() ) lynx_config_sleep_mode(1); // allow the sleep mode
            else lynx_config_sleep_mode(0);                             // block the sleep mode
    }
	printf("cvUSHFpEnableFingerDetect %d\n", retVal);
	return retVal;
}

cv_bool
isFPImageBufAvail(void)
{
	/* this function determines if the FP image buffer is available for non-FP capture use */
	
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS)
		return FALSE;

	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_IMAGE_VALID)
	{
		/* check to see if FP persistence has timed out */
		if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
		{
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID;
		} else
			return FALSE;
	}
	return TRUE;
}

cv_status
cvFpGenTemplateList (cv_handle cvHandle, 
					uint32_t hashListLen, uint8_t *pHashList,
					uint32_t *pTemplateListLen, cv_obj_handle *pTemplateList)
{
	/* This function will build a template list of the fingerprints to compair against  */
	cv_status 			retVal = CV_SUCCESS;
	uint32_t			numHashs = hashListLen/SHA1_LEN;
	cv_session			tempSession, *pSession = cvHandle;
	uint32_t 			objLen;
	cv_obj_handle		objHandleList[MAX_NUM_FP_TEMPLATES];
	uint32_t			numTemplates;
	uint32_t			numAvailTemplates = *pTemplateListLen;
	uint32_t			idx, enumTemps, foundTemps;
	bool				inlist;

	//printf("cvFpGenTemplateList() enter\n");
	//printf("cvFpGenTemplateList() hashListLen 0x%x\n", hashListLen);

	//DumpBuffer(sizeof(cv_session), (void*)cvHandle); 
	// initialize number of tempates to 0
	*pTemplateListLen = 0;

	if ( numHashs == 0 ) {
		// if no hashes then no duplicates return ok.
		printf("cvFpGenTemplateList() no template hashes\n");
		return CV_SUCCESS;
	}

	//printf("cvFpGenTemplateList() entered %d hashes\n", numHashs);
	//printf("cvFpGenTemplateList() Handle appUserID:\n");
	//DumpBuffer(SHA1_LEN, (void*)pSession->appUserID); 

	// save cv_session
	memcpy((void*)&tempSession, (void*)cvHandle, sizeof(cv_session));
	for ( idx=0; idx<numHashs; idx++ ) {
		//printf("cvFpGenTemplateList() getting FP objects for user\app ID hash %d\n", idx);

		// copy cv_session
		memcpy((void*)cvHandle, (void*)&tempSession, sizeof(cv_session));
		// load appUserID hash
		//printf("cvFpGenTemplateList() pHashList:\n");
		//DumpBuffer(SHA1_LEN, (void*)&pHashList[idx*SHA1_LEN]); 
		memcpy((void*)pSession->appUserID, (void*)&pHashList[idx*SHA1_LEN], SHA1_LEN);
		//printf("cvFpGenTemplateList() temp session:\n");
		//DumpBuffer(sizeof(cv_session), (void*)&tempSession); 

		objLen = MAX_NUM_FP_TEMPLATES*sizeof(cv_obj_handle);

		retVal = cv_enumerate_objects (cvHandle, CV_TYPE_FINGERPRINT, &objLen, objHandleList);
		if ( retVal != CV_SUCCESS ) {
			printf("cvFpGenTemplateList() cv_enumerate_objects returned 0x%x\n", retVal);
			// restore cv_session
			memcpy((void*)cvHandle, (void*)&tempSession, sizeof(cv_session));
			return retVal;
		}

		numTemplates = objLen/sizeof(cv_obj_handle);

		//printf("cvFpGenTemplateList() enumerated %d FP Objects\n", numTemplates);

		for ( enumTemps=0; enumTemps<numTemplates; enumTemps++ ) {

			inlist=false;

			//printf("cvFpGenTemplateList() enumerated template %d: 0x%x\n", enumTemps, objHandleList[enumTemps]);

			// for all enumerated templates:
			// make sure it is not already in list
			for ( foundTemps=0; foundTemps<*pTemplateListLen; foundTemps++ ) {
				if (objHandleList[enumTemps] == pTemplateList[foundTemps] )
					inlist = true;
			}
			if ( !inlist ) {
				//printf("cvFpGenTemplateList() found new template\n");
				// increment number of templates
				if ( numAvailTemplates > 0 ) {
					//printf("cvFpGenTemplateList() adding to template list\n");
					pTemplateList[*pTemplateListLen] = objHandleList[enumTemps];
					numAvailTemplates--;
				}
				else {
					printf("cvFpGenTemplateList() WARNING! NOT enough room for all templates in template list\n");
					// restore cv_session
					memcpy((void*)cvHandle, (void*)&tempSession, sizeof(cv_session));
					return retVal;
				}
				(*pTemplateListLen)++;
				//printf("cvFpGenTemplateList() *pTemplateListLen: 0x%x\n", *pTemplateListLen);
			}
		}
	}
	// restore cv_session
	memcpy((void*)cvHandle, (void*)&tempSession, sizeof(cv_session));

	//printf("cvFpGenTemplateList() exit 0x%X\n", retVal);
	return retVal;
}

cv_status
cvFpCheckDuplicate (cv_handle cvHandle,
			uint32_t featureSetLength, byte *pFeatureSet,
			uint32_t hashListLen, uint8_t *pHashListBuf,
			cv_obj_handle *pTemplateHandle )
{
	/* This function will check for duplicate FPs */
	/* It will compair the input feature set to a generated list of templates. */
	/* If a duplicate fingerprint is found then CV_FP_DUPLICATE is returned */
	cv_status				retVal = CV_SUCCESS;
	int32_t					FARvalParam = 0x53E2;
	cv_fp_control			fpControl = CV_USE_PERSISTED_IMAGE_ONLY | CV_USE_FAR;
	uint32_t				templateListLen = MAX_NUM_FP_TEMPLATES;
	cv_obj_handle			templateList[MAX_NUM_FP_TEMPLATES];
	cv_obj_handle			resultTemplateHandle;
	cv_bool					match=false;
	int						index;

	//printf("cvFpCheckDuplicate() enter\n");

	//printf("cvFpCheckDuplicate() hashListLen: 0x%x\n", hashListLen);
	// initialize handle
	if (pTemplateHandle)
		*pTemplateHandle = NULL;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
	{
		printf("cvFpCheckDuplicate() invalid handle\n");
		return retVal;
	}

	//printf("cvFpCheckDuplicate() check parameters length: 0x%x\n", featureSetLength);

	/* determine if feature set provided, either directly of via CV handle */
	if (featureSetLength == 0 || pFeatureSet == NULL)
	{
		printf("cvFpCheckDuplicate() invalid parameter\n");
		return CV_INVALID_INPUT_PARAMETER_LENGTH;
	}

	if (hashListLen==0)
	{
		printf("cvFpCheckDuplicate() no hash list, so no templates or duplicates\n");
		return CV_SUCCESS;
	}

	//printf("cvFpCheckDuplicate() build template list\n");
	/* find number of templates and build template list */
	retVal = cvFpGenTemplateList (cvHandle,
								hashListLen, pHashListBuf,
								&templateListLen, templateList);
	if (retVal != CV_SUCCESS)
	{
		printf("cvFpCheckDuplicate() failed to build template list\n");
		return retVal;
	}

	if (templateListLen==0)
	{
		printf("cvFpCheckDuplicate() no tempates, so no duplicate\n");
		return CV_SUCCESS;
	}

	for (index=0; index<templateListLen; index++)
		printf("cvFpCheckDuplicate() template[%02d]: 0x%08x\n", index, templateList[index]);

	//printf("cvFpCheckDuplicate() check identify\n");
	/* now, do identify */
	if (CHIP_IS_5882_OR_LYNX)
	{
		if ((retVal = cvFROnChipIdentify(cvHandle, FARvalParam, fpControl, TRUE,
					featureSetLength, pFeatureSet,
					templateListLen, templateList,
					0, NULL,
					&resultTemplateHandle, FALSE, &match,
					NULL, NULL)) != CV_SUCCESS)
			goto err_exit;
	}
	else
	{
		if ((retVal = cvUSHfpIdentify(cvHandle, FARvalParam, fpControl,
				featureSetLength, pFeatureSet,
				templateListLen, templateList,
				0, NULL,
				&resultTemplateHandle, FALSE, &match,
				NULL, NULL)) != CV_SUCCESS)
			goto err_exit;
	}

	/* now set result based on whether a match occurred or not */
	if (match) {
		printf("cvFpCheckDuplicate() found duplicate template 0x%x\n", resultTemplateHandle);
		if (pTemplateHandle)
			*pTemplateHandle = resultTemplateHandle;
		retVal = CV_FP_DUPLICATE;
	}

err_exit:
	printf("cvFpCheckDuplicate() exit 0x%X\n", retVal);
	return retVal;
}

cv_status
cv_fingerprint_enroll_imp (cv_handle cvHandle,              
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t userAppHashListLength, uint8_t *pUserAppHashListBuf,
			uint32_t *pTemplateLength, uint8_t *pTemplate,
			cv_obj_handle *pTemplateHandle,
			cv_callback *callback, cv_callback_ctx context, cv_bool shouldDoDupCheck)
{
	uint8_t featureSets[4][MAX_FP_FEATURE_SET_SIZE];
	uint32_t i;
	cv_status retVal = CV_SUCCESS;
	uint32_t imageLength[4], imageWidth;
	uint32_t retTemplateLength = 0;
	cv_obj_value_fingerprint *retTemplate = NULL;
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_auth_entry_PIN *PIN;
	cv_auth_hdr *PINhdr;
	uint32_t pinLen, remLen;
	uint32_t temp;
	cv_bool isTempUsed = FALSE;
	uint8_t *pSynapticsTemplate = NULL;
	uint32_t synapticsTemplateSize = 0;
	uint32_t enrollComplete = 0;
	cv_obj_handle dupTemplateHandle;

	printf("cv_fingerprint_enroll\n");

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
	
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;


	if (is_fp_synaptics_present())
	{
        int action = CV_SWIPE_NORMAL;
		do
		{
			/* Capture fingerprint */
			/* Using featureSets[0] because any feature set can be used here */
			CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
            imageLength[0] = MAX_FP_FEATURE_SET_SIZE;   /* Re-init to MAX_FP_FEATURE_SET_SIZE */
			if ((retVal = cvUSHfpCapture(FALSE, FALSE, 0, (uint8_t *)&featureSets[0], &imageLength[0], &imageWidth, action,
						callback, context)) == CV_FP_BAD_SWIPE) {
					imageLength[0] = MAX_FP_FEATURE_SET_SIZE;	/* Re-init to MAX_FP_FEATURE_SET_SIZE */						
					while ((retVal = cvUSHfpCapture(FALSE, FALSE, 0, (uint8_t *)&featureSets[0], &imageLength[0], &imageWidth, CV_RESWIPE,
						callback, context)) == CV_FP_BAD_SWIPE)
						imageLength[0] = MAX_FP_FEATURE_SET_SIZE;	/* Re-init to MAX_FP_FEATURE_SET_SIZE */
			}

            action = CV_RESWIPE;

            if (retVal == CV_FP_DEVICE_GENERAL_ERROR ) {
                printf("cvUSHfpCapture general faillure, reswipe finger again.\n");
                continue;
            } 

			if (retVal != CV_SUCCESS)
				goto err_exit;


			/* Add it to the enrollment template */
			retVal = cvSynapticsOnChipCreateTemplate(featureSets[0], imageLength[0], &pSynapticsTemplate, &synapticsTemplateSize, &enrollComplete);
			if (retVal != 0)
			{
				printf("cvSynapticsOnChipCreateTemplate returned %d\n", retVal);
				goto err_exit;
			}

		}while (enrollComplete == 0);

		printf("Captured all fingers\n");
		/* Captured all ~7 fingers now */

		if (shouldDoDupCheck)
		{
			// check for duplicate fp
			printf("Check for duplicate fingerprint\n");
			retVal = cvFpCheckDuplicate (cvHandle, imageLength[0], featureSets[0],
										userAppHashListLength, pUserAppHashListBuf, &dupTemplateHandle);
			if (retVal == CV_FP_DUPLICATE)
			{
				*pTemplateLength = 0;
				*pTemplateHandle = dupTemplateHandle;
				printf("cv_fingerprint_enroll_dup_check() found duplicate fingerprint template: 0x%x\n", dupTemplateHandle);
				goto err_exit;
			}
		}

		/* Prepare a cv_obj_value_fingerprint structure around pSynapticsTemplate so it can be used as a pObjValue */
		retTemplate = (cv_obj_value_fingerprint *)((uint32_t*)pSynapticsTemplate - 1);
		retTemplateLength = synapticsTemplateSize;
		temp = *((uint32_t*)retTemplate);
		isTempUsed = TRUE;
		retTemplate->type = CV_FINGERPRINT_TEMPLATE_SYNAPTICS;
		retTemplate->fpLen = synapticsTemplateSize;
	}
	else /* not Synaptics */
	{

		/* now do 4 FP captures to be used to generate template */
		for (i=0;i<4;i++)
		{		
			imageLength[i] = MAX_FP_FEATURE_SET_SIZE;	/* Init to MAX_FP_FEATURE_SET_SIZE */
			
			/* invalidate any persisted FP first */
			CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
			if ((retVal = cvUSHfpCapture(FALSE, FALSE, 0, (uint8_t *)&featureSets[i], &imageLength[i], &imageWidth, i + 1,
						callback, context)) == CV_FP_BAD_SWIPE) {
					imageLength[i] = MAX_FP_FEATURE_SET_SIZE;	/* Re-init to MAX_FP_FEATURE_SET_SIZE */						
					while ((retVal = cvUSHfpCapture(FALSE, FALSE, 0, (uint8_t *)&featureSets[i], &imageLength[i], &imageWidth, CV_RESWIPE,
						callback, context)) == CV_FP_BAD_SWIPE)
						imageLength[i] = MAX_FP_FEATURE_SET_SIZE;	/* Re-init to MAX_FP_FEATURE_SET_SIZE */
			}						
			if (retVal != CV_SUCCESS)							
				goto err_exit;		
		}

		if (shouldDoDupCheck)
		{
			// check for duplicate fp
			printf("Check for duplicate fingerprint\n");
			retVal = cvFpCheckDuplicate (cvHandle, imageLength[0], featureSets[0],
										userAppHashListLength, pUserAppHashListBuf, &dupTemplateHandle);
			if (retVal == CV_FP_DUPLICATE)
			{
				*pTemplateLength = 0;
				*pTemplateHandle = dupTemplateHandle;
				printf("cv_fingerprint_enroll_dup_check() found duplicate fingerprint template: 0x%x\n", dupTemplateHandle);
				goto err_exit;
			}
		}

		/* now create template */
		if (CHIP_IS_5882_OR_LYNX) {
			if ((retVal = cvDPFROnChipCreateTemplate(imageLength[0], (uint8_t *)&featureSets[0],
						 imageLength[1], (uint8_t *)&featureSets[1],
						 imageLength[2], (uint8_t *)&featureSets[2],
						 imageLength[3], (uint8_t *)&featureSets[3],
						 &retTemplateLength, &retTemplate)) != CV_SUCCESS)
					 goto err_exit;
				 }
		else {
			if ((retVal = cvUSHfpCreateTemplate(imageLength[0], (uint8_t *)&featureSets[0],
						 imageLength[1], (uint8_t *)&featureSets[1],
						 imageLength[2], (uint8_t *)&featureSets[2],
						 imageLength[3], (uint8_t *)&featureSets[3],
						 &retTemplateLength, &retTemplate)) != CV_SUCCESS)
					 goto err_exit;
				 }
	}

	/* check to see if template should be returned or a CV object should be created and the handle returned */
	if (*pTemplateLength == 0)
	{
		/* save as object */
		/* validate new auth lists provided */
		if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
			goto err_exit;
		if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
			goto err_exit;
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objAttributesLength = objAttributesLength;
		objProperties.pObjAttributes = pObjAttributes;
		objProperties.authListsLength = authListsLength;
		objProperties.pAuthLists = pAuthLists;
		objProperties.objValueLength = retTemplateLength + sizeof(cv_obj_value_fingerprint);
		objProperties.pObjValue = retTemplate;
		/* check to make sure object not too large */
		if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
		{
			retVal = CV_INVALID_OBJECT_SIZE;
			goto err_exit;
		}
		objProperties.objHandle = 0;
		objPtr.objType = CV_TYPE_FINGERPRINT;
		/* search to see if PIN supplied in auth list.  it must be in the first auth */
		/* list, if present.  this PIN is used to access the destination device for storing the object */
		/* and is removed from the auth list which is stored with the object */
		if (authListsLength && (PIN = cvFindPINAuthEntry(pAuthLists, &objProperties)) != NULL)
		{
			/* PIN found.  now remove from auth list */
			PINhdr = (cv_auth_hdr *)((uint8_t *)PIN - sizeof(cv_auth_hdr));
			pinLen = PIN->PINLen + sizeof(cv_auth_entry_PIN) - sizeof(uint8_t);
			remLen = authListsLength - (uint32_t)((uint8_t *)PINhdr - (uint8_t *)pAuthLists);
			memmove(PINhdr, (uint8_t *)PIN + pinLen, remLen);
			objProperties.authListsLength -= (pinLen + sizeof(cv_auth_hdr));
			/* now decrement auth list entry count */
			pAuthLists->numEntries--;
		}
		objPtr.objLen = cvComputeObjLen(&objProperties);
		/* now save object */
		retVal = cvHandlerPostObjSave(&objProperties, &objPtr, callback, context);
		*pTemplateHandle = objProperties.objHandle;
	} else {
		/* return template to calling app */
		if (retTemplate->fpLen > *pTemplateLength)
		{
			/* buffer too small, return error and correct length */
			*pTemplateLength = retTemplate->fpLen;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pTemplateLength = retTemplate->fpLen;
		memcpy(pTemplate, &retTemplate->fp, retTemplate->fpLen);
	}

err_exit:
	if (isTempUsed)
	{
		*((uint32_t*)retTemplate) = temp;
	}
	return retVal;
}

cv_status
cv_fingerprint_enroll (cv_handle cvHandle,              
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pTemplateLength, uint8_t *pTemplate,
			cv_obj_handle *pTemplateHandle,
			cv_callback *callback, cv_callback_ctx context)
{

	return cv_fingerprint_enroll_imp (cvHandle,              
				objAttributesLength, pObjAttributes, 
				authListsLength, pAuthLists,
				0, NULL,
				pTemplateLength, pTemplate,
				pTemplateHandle,
				callback, context, 0);
}

cv_status
cv_fingerprint_enroll_dup_check (cv_handle cvHandle,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t userAppHashListLength, uint8_t *pUserAppHashListBuf,
			uint32_t *pTemplateLength, uint8_t *pTemplate,
			cv_obj_handle *pTemplateHandle,
			cv_callback *callback, cv_callback_ctx context)
{
	return cv_fingerprint_enroll_imp (cvHandle,              
				objAttributesLength, pObjAttributes, 
				authListsLength, pAuthLists,
				userAppHashListLength, pUserAppHashListBuf,
				pTemplateLength, pTemplate,
				pTemplateHandle,
				callback, context, 1);
}

cv_status
cvDoTemplateHash(cv_handle cvHandle, cv_obj_handle fpTemplate, uint32_t hashLen, uint8_t *hash)
{
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_status retVal = CV_SUCCESS;
	cv_obj_value_hash *hashObj;
	uint8_t obj[MAX_CV_OBJ_SIZE];

	if (!(hashLen == SHA1_LEN || hashLen == SHA256_LEN))
		return CV_INVALID_HASH_TYPE;
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.objHandle = fpTemplate;
	objProperties.session = (cv_session *)cvHandle;

	/* just get object w/o auth (this won't work for objects in removable devices) */
	if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
		goto err_exit;

	/* copy value locally in order to change from internal to external format */
	cvCopyObjValue(objPtr->objType, objProperties.objValueLength, (void *)obj, objProperties.pObjValue, TRUE, FALSE);

	/* now create hash */
	retVal = cvAuth(obj, objProperties.objValueLength, NULL, hash, hashLen, NULL, 0, CV_SHA/*TBD*/);

err_exit:
	return retVal;
}

cv_status
cvDoFingerprintVerify (cv_handle cvHandle, cv_fp_control fpControl,
		    int32_t FARval,
			uint32_t featureSetLength, byte *pFeatureSet,
			cv_obj_handle featureSetHandle,
			uint32_t templateLength, byte *pTemplate,   
			cv_obj_handle templateHandle,
			cv_fp_result *pFpResult, uint32_t hashLen, uint8_t *hash,
			cv_callback *callback, cv_callback_ctx context)
{

		/* This function does a verify operation involving a fingerprint template and a feature set.  */
	/* The feature set is either captured, supplied directly (if featureSetLength is non-zero) or */
	/* supplied as a CV object handle.  The template can also be supplied directly (if templateLength */
	/* is non-zero) or as a CV object handle. */
	int32_t FARvalParam; 
	uint32_t featureSetLengthParam = 0;
	uint8_t *pFeatureSetParam = NULL;
	cv_status retVal = CV_SUCCESS;
	cv_bool matchFound;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_value_fingerprint *retTemplate;
	cv_obj_handle pResultTemplateHandle;

	printf("cvDoFingerprintVerify: templateHandle 0x%x\n",templateHandle);
	/* Clear result */
	*pFpResult = 0;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
	
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* validate fpControl */
	if (fpControl & ~CV_FP_CONTROL_MASK)
	{
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}

	/* first, determine where to get the FAR value */
	/* check to see if parameter value should be used */
	if (fpControl & CV_USE_FAR)
	{
		/* check to make sure CV admin allows FAR to be supplied as param */
		if (CV_PERSISTENT_DATA->cvPolicy & CV_DISALLOW_FAR_PARAMETER)
		{
			retVal = CV_FAR_PARAMETER_DISALLOWED;
			goto err_exit;
		}
		FARvalParam = FARval;
	} else
		FARvalParam = CV_PERSISTENT_DATA->FARval;		/* Use default or configured FAR value */

	/* determine if feature set provided, either directly of via CV handle */
	if (featureSetLength != 0)
	{
		/* feature set provided */
		featureSetLengthParam = featureSetLength;
		pFeatureSetParam = pFeatureSet;
	} else if (featureSetHandle != 0) {
		/* feature set handle provided.  get object, but don't do auth, since fp templates can be used w/o auth */
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objHandle = featureSetHandle;
		if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		cvFindObjPtrs(&objProperties, objPtr);
		retTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
		if (retTemplate->type != getExpectedFingerprintFeatureSetType())
		{
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
		featureSetLengthParam = retTemplate->fpLen;
		pFeatureSetParam = retTemplate->fp;
	} /* no feature set provided, do capture */

	/* now, do verify */
	/* check to see if any persisted FP should be invalidated */
	if (fpControl & CV_INVALIDATE_CAPTURE)
		CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

#ifdef DEMO_BOARD				
	if ((retVal = cvFROnChipIdentify(cvHandle, FARvalParam, fpControl, FALSE, 
			featureSetLengthParam, pFeatureSetParam,
			1, &templateHandle,
			templateLength, pTemplate,   
			&pResultTemplateHandle, FALSE, &matchFound,
			callback, context)) != CV_SUCCESS)
		goto err_exit;
#else			
	if (CHIP_IS_5882_OR_LYNX) {	
		if ((retVal = cvFROnChipIdentify(cvHandle, FARvalParam, fpControl, FALSE,
				featureSetLengthParam, pFeatureSetParam,
				1, &templateHandle,
				templateLength, pTemplate,   
				&pResultTemplateHandle, FALSE, &matchFound,
				callback, context)) != CV_SUCCESS)
			goto err_exit;
		}
	else {
		/* here, cvUSHfpIdentify is used, because it's really just a special case of identify */
		if ((retVal = cvUSHfpIdentify(cvHandle, FARvalParam, fpControl, 
				featureSetLengthParam, pFeatureSetParam,
				1, &templateHandle,
				templateLength, pTemplate,   
				&pResultTemplateHandle, FALSE, &matchFound,
				callback, context)) != CV_SUCCESS)
			goto err_exit;
		}
#endif		
	if (matchFound)
	{
		*pFpResult |= CV_MATCH_FOUND;
		/* now compute hash, if requested */
		if (hash != NULL)
			retVal = cvDoTemplateHash(cvHandle, templateHandle, hashLen, hash);
	} else
		*pFpResult &= ~CV_MATCH_FOUND;

err_exit:
	return retVal;
}

cv_status
cv_fingerprint_verify (cv_handle cvHandle, cv_fp_control fpControl,
		    int32_t FARval,
			uint32_t featureSetLength, byte *pFeatureSet,
			cv_obj_handle featureSetHandle,
			uint32_t templateLength, byte *pTemplate,   
			cv_obj_handle templateHandle,
			cv_fp_result *pFpResult,
			cv_callback *callback, cv_callback_ctx context)
{
	return cvDoFingerprintVerify(cvHandle, fpControl,
		    FARval,
			featureSetLength, pFeatureSet,
			featureSetHandle,
			templateLength, pTemplate,   
			templateHandle,
			pFpResult, 0, NULL,
			callback, context);
}

cv_status
cv_fingerprint_verify_w_hash (cv_handle cvHandle, cv_fp_control fpControl,
		    int32_t FARval,
			uint32_t featureSetLength, byte *pFeatureSet,
			cv_obj_handle featureSetHandle,
			uint32_t templateLength, byte *pTemplate,   
			cv_obj_handle templateHandle,
			cv_fp_result *pFpResult, uint32_t hashLen, uint8_t *hash,
			cv_callback *callback, cv_callback_ctx context)
{
	return cvDoFingerprintVerify(cvHandle, fpControl,
		    FARval,
			featureSetLength, pFeatureSet,
			featureSetHandle,
			templateLength, pTemplate,   
			templateHandle,
			pFpResult, hashLen, hash,
			callback, context);
}

cv_status
cv_fingerprint_capture (cv_handle cvHandle, cv_bool captureOnly,            
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pFeatureSetLength, byte *pFeatureSet,
			cv_obj_handle *pFeatureSetHandle,
			cv_callback *callback, cv_callback_ctx context)
{
	uint8_t featureSet[MAX_FP_FEATURE_SET_SIZE];
	cv_status retVal = CV_SUCCESS;
	uint32_t imageLength, imageWidth, imageLengthLocal;
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_auth_entry_PIN *PIN;
	cv_auth_hdr *PINhdr;
	uint32_t pinLen, remLen;
	uint8_t *pImage;
	cv_obj_value_fingerprint *fpObjHdr;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
	
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* always require a new swipe (ie, invalidate persisted feature set) */
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

	/* now do capture.  determine if feature set/image will be returned or saved as CV object */
	if (*pFeatureSetLength)
	{
		/* return captured feature set/image */
        imageLength = *pFeatureSetLength;
		imageLengthLocal = imageLength;
		pImage = pFeatureSet;
	} else {
		/* save feature set as CV object */
		/* don't allow images to be saved */
		if (captureOnly)
		{
			retVal = CV_INVALID_OBJECT_PERSISTENCE;
			goto err_exit;
		}
        imageLength = MAX_FP_FEATURE_SET_SIZE;
		pImage = (uint8_t *)(&featureSet[0]);
	}
	if ((retVal = cvUSHfpCapture(captureOnly, FALSE, 0, pImage, &imageLength, &imageWidth, CV_SWIPE_NORMAL,
				callback, context)) == CV_FP_BAD_SWIPE) {
			/* set buffer size for image length based on whether feature set being captured or raw image */
			if (captureOnly)
				imageLength = imageLengthLocal;
			else
				imageLength = MAX_FP_FEATURE_SET_SIZE;
			while ((retVal = cvUSHfpCapture(captureOnly, FALSE, 0, pImage, &imageLength, &imageWidth, CV_RESWIPE,
					callback, context)) == CV_FP_BAD_SWIPE)
			{
				/* set buffer size for image length based on whether feature set being captured or raw image */
				if (captureOnly)
					imageLength = imageLengthLocal;
				else
					imageLength = MAX_FP_FEATURE_SET_SIZE;
			}
	}
	if (retVal != CV_SUCCESS)
			goto err_exit;

	/* check to see if a CV object should be created and the handle returned */
	if (*pFeatureSetLength == 0)
	{
		/* save as object */
		/* insert header */
		memmove(pImage + sizeof(cv_obj_value_fingerprint) - 1, pImage, imageLength);
		fpObjHdr = (cv_obj_value_fingerprint *)pImage;
		fpObjHdr->type = getExpectedFingerprintFeatureSetType();
		fpObjHdr->fpLen = imageLength;
		imageLength += (sizeof(cv_obj_value_fingerprint) - 1);
		/* validate new auth lists provided */
		if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
			goto err_exit;
		if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
			goto err_exit;
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objAttributesLength = objAttributesLength;
		objProperties.pObjAttributes = pObjAttributes;
		objProperties.authListsLength = authListsLength;
		objProperties.pAuthLists = pAuthLists;
		objProperties.objValueLength = imageLength;
		objProperties.pObjValue = pImage;
		/* check to make sure object not too large */
		if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
		{
			retVal = CV_INVALID_OBJECT_SIZE;
			goto err_exit;
		}
		objProperties.objHandle = 0;
		objPtr.objType = CV_TYPE_FINGERPRINT;
		/* search to see if PIN supplied in auth list.  it must be in the first auth */
		/* list, if present.  this PIN is used to access the destination device for storing the object */
		/* and is removed from the auth list which is stored with the object */
		if (authListsLength && (PIN = cvFindPINAuthEntry(pAuthLists, &objProperties)) != NULL)
		{
			/* PIN found.  now remove from auth list */
			PINhdr = (cv_auth_hdr *)((uint8_t *)PIN - sizeof(cv_auth_hdr));
			pinLen = PIN->PINLen + sizeof(cv_auth_entry_PIN) - sizeof(uint8_t);
			remLen = authListsLength - (uint32_t)((uint8_t *)PINhdr - (uint8_t *)pAuthLists);
			memmove(PINhdr, (uint8_t *)PIN + pinLen, remLen);
			objProperties.authListsLength -= (pinLen + sizeof(cv_auth_hdr));
			/* now decrement auth list entry count */
			pAuthLists->numEntries--;
		}
		objPtr.objLen = cvComputeObjLen(&objProperties);
		/* now save object */
		retVal = cvHandlerPostObjSave(&objProperties, &objPtr, callback, context);
		*pFeatureSetHandle = objProperties.objHandle;
	}

err_exit:
	if (*pFeatureSetLength != 0 && retVal == CV_SUCCESS)
		/* save actual length, if feature set returned */
		*pFeatureSetLength = imageLength;
	return retVal;
}

cv_status
cvDoFingerprintIdentify(cv_handle cvHandle, cv_fp_control fpControl,
			int32_t FARval,
			cv_obj_handle featureSetHandle,             
			uint32_t numTemplates, cv_obj_handle *pTemplateHandles,             
			cv_fp_result *pFpResult,
			cv_obj_handle *pResultTemplateHandle, uint32_t hashLen, uint8_t *hash,			
			cv_callback *callback, cv_callback_ctx context)
{
	/* This function does an identify operation (1 to few) involving one or more fingerprint templates and a */
	/* feature set.  The feature set is either captured or supplied as a CV object handle.  The list of templates */
	/* is only specified via CV object handles.  This API, unlike  cv_fingerprint_capture and cv_fingerprint_verify, */
	/* only supports the model where the FP objects are stored in the CV.  The reason for this is because in the */
	/* model where the FP objects are stored on the host, the identify function can be run directly on the host without calling CV */
	int32_t FARvalParam; 
	uint32_t featureSetLengthParam = 0;
	uint8_t *pFeatureSetParam = NULL;
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	cv_obj_value_fingerprint *retTemplate;
	cv_bool match=0;

	/* Clear result */
	*pFpResult = 0;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
			
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
	{
		printf("%s: failed session handle \n",__FUNCTION__);
		return retVal;
	}

	/* numtemplates is actually the length of all template handles */
	/* divide by 4 to get the number of templates */
	numTemplates = numTemplates >> 2;

	/* validate fpControl */
	if (fpControl & ~CV_FP_CONTROL_MASK)
	{
		retVal = CV_INVALID_INPUT_PARAMETER;
		goto err_exit;
	}

	/* first, determine where to get the FAR value */
	/* check to see if parameter value should be used */
	if (fpControl & CV_USE_FAR)
	{
		/* check to make sure CV admin allows FAR to be supplied as param */
		if (CV_PERSISTENT_DATA->cvPolicy & CV_DISALLOW_FAR_PARAMETER)
		{
			retVal = CV_FAR_PARAMETER_DISALLOWED;
			goto err_exit;
		}
		FARvalParam = FARval;
	} else {
		/* no FAR param, must use configured value, if available */
		if (!(CV_PERSISTENT_DATA->persistentFlags & CV_HAS_FAR_VALUE))
		{
			/* no FAR value configured */
			retVal = CV_FAR_VALUE_NOT_CONFIGURED;
			goto err_exit;
		}
		FARvalParam = CV_PERSISTENT_DATA->FARval;
	}

	/* determine if feature set provided via CV handle */
	if (featureSetHandle != 0) {
		/* feature set handle provided.  get object, but don't do auth, since fp templates can be used w/o auth */
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objHandle = featureSetHandle;
		if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
			goto err_exit;
		cvFindObjPtrs(&objProperties, objPtr);
		retTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
		if (retTemplate->type != getExpectedFingerprintFeatureSetType())
		{
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
		featureSetLengthParam = retTemplate->fpLen;
		pFeatureSetParam = retTemplate->fp;
	} /* no feature set provided, do capture */

	/* now, do identify */
	/* check to see if any persisted FP should be invalidated */
	if (fpControl & CV_INVALIDATE_CAPTURE)
		CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

	if(!(CV_VOLATILE_DATA->poaAuthSuccessFlag & CV_POA_AUTH_SUCCESS_COMPLETE))
	{
		if (CHIP_IS_5882_OR_LYNX) {
			if ((retVal = cvFROnChipIdentify(cvHandle, FARvalParam, fpControl, TRUE,
						featureSetLengthParam, pFeatureSetParam, 
						numTemplates, pTemplateHandles, 
						0, NULL,
						pResultTemplateHandle, FALSE, &match, 
						callback, context)) != CV_SUCCESS)
				goto err_exit;
			}
		else {		
			if ((retVal = cvUSHfpIdentify(cvHandle, FARvalParam, fpControl, 
					featureSetLengthParam, pFeatureSetParam,
					numTemplates, pTemplateHandles,
					0, NULL,
					pResultTemplateHandle, FALSE, &match,
					callback, context)) != CV_SUCCESS)
				goto err_exit;
			}
	}
	else
	{
		if(cv_get_timer2_elapsed_time(CV_VOLATILE_DATA->poaAuthTimeout) <= CV_POA_AUTH_TIMEOUT)
		{
			if(featureSetHandle)
			{
				if(featureSetHandle == CV_VOLATILE_DATA->poaAuthSuccessTemplate)
				{
					printf("%s: template matched \n",__FUNCTION__);
					match=1;
				}
			}
			else if(numTemplates)
			{
				int  i;
				for (i=0;i<numTemplates;i++)
				{
					if(pTemplateHandles[i] == CV_VOLATILE_DATA->poaAuthSuccessTemplate)
					{
						printf("%s: template matched = %d \n",__FUNCTION__,i);
						match=1 ;
						break;
					}
				}
			}
		}
		printf("POA SSO: Match Done and cleared cache \n");
		if(match == 1)
		{
			*pResultTemplateHandle = CV_VOLATILE_DATA->poaAuthSuccessTemplate;
		}
		CV_VOLATILE_DATA->poaAuthSuccessTemplate = 0;
		CV_VOLATILE_DATA->poaAuthSuccessFlag = CV_POA_AUTH_NOT_DONE;
	}
		
	/* now set result based on whether a match occurred or not */
	if (match)
	{
		*pFpResult |= CV_MATCH_FOUND;
		/* now compute hash, if requested */
		if (hash != NULL)
			retVal = cvDoTemplateHash(cvHandle, *pResultTemplateHandle, hashLen, hash);
	}

err_exit:
	return retVal;
}

cv_status
cv_fingerprint_identify (cv_handle cvHandle, cv_fp_control fpControl,
			int32_t FARval,
			cv_obj_handle featureSetHandle,             
			uint32_t numTemplates, cv_obj_handle *pTemplateHandles,             
			cv_fp_result *pFpResult,
			cv_obj_handle *pResultTemplateHandle,			
			cv_callback *callback, cv_callback_ctx context)
{
	return cvDoFingerprintIdentify(cvHandle, fpControl,
			FARval,
			featureSetHandle,             
			numTemplates, pTemplateHandles,             
			pFpResult,
			pResultTemplateHandle, 0, NULL,			
			callback, context);
}

cv_status
cv_fingerprint_identify_w_hash (cv_handle cvHandle, cv_fp_control fpControl,
			int32_t FARval,
			cv_obj_handle featureSetHandle,             
			uint32_t numTemplates, cv_obj_handle *pTemplateHandles,             
			cv_fp_result *pFpResult,
			cv_obj_handle *pResultTemplateHandle, uint32_t hashLen, uint8_t *hash,			
			cv_callback *callback, cv_callback_ctx context)
{
	return cvDoFingerprintIdentify(cvHandle, fpControl,
			FARval,
			featureSetHandle,             
			numTemplates, pTemplateHandles,             
			pFpResult,
			pResultTemplateHandle, hashLen, hash,			
			callback, context);
}

cv_status
cv_fingerprint_configure (cv_handle cvHandle, 
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_fp_config_data_type cvFPConfigType,            
			uint32_t offset, uint32_t *pDataLength, byte *pData, 
			cv_bool read, cv_callback *callback, cv_callback_ctx context)
{
	/* This routine is provided to allow for configuration of fingerprint devices and the */
	/* fingerprint matching algorithm.   The determination of which of those 2 is the destination */
	/* is made by the parameter, cvFPConfigType */
#if 0
	/* CV_FP_CONFIG_DATA_MATCHING disabled for now */
	cv_obj_properties objProperties;
	cv_obj_auth_flags authFlags;
#endif
	cv_status retVal = CV_SUCCESS;
	cv_input_parameters inputParameters;
	fp_status fpStatus = FP_OK;
	uint32_t interval, minInterval;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
	
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* don't allow flash update more often that every 5 minutes, to prevent flash burnout */
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_ANTIHAMMERING_ACTIVE) && !read)
	{
		interval = cvGetDeltaTime(CV_VOLATILE_DATA->timeLastFPConfigWrite);
		/* workaround to account for Atmel flash requiring 2 writes for each page write */
		if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
			minInterval = 2*CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		else
			minInterval = CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
		/* for first time through (ie, last time is 0) need to init start time */
		if ((interval < minInterval) && (CV_VOLATILE_DATA->timeLastFPConfigWrite != 0))
		{
			if (interval > CV_FP_CONFIGURE_DURATION)
			{
				/* here if PBA code download should have completed.  if not, return antihammering status */
				retVal = CV_ANTIHAMMERING_PROTECTION;
				goto err_exit;
			}
		} else
			/* here if initiating sequence of commands to write PBA flash.  initialize start time */
			get_ms_ticks(&CV_VOLATILE_DATA->timeLastFPConfigWrite);
	}

	/* set up parameter list pointers, in case used by auth */
	cvSetupInputParameterPtrs(&inputParameters, 7,  
		sizeof(uint32_t), &authListLength, authListLength, pAuthList,
		sizeof(cv_fp_config_data_type), &cvFPConfigType, sizeof(uint32_t), &offset, 
		sizeof(uint32_t), pDataLength, *pDataLength, pData, sizeof(cv_bool), &read, 0, NULL, 0, NULL, 0, NULL);

	/* don't include pData as input if read is TRUE.  also, validate read block size */
	if (read) {
		inputParameters.paramLenPtr[6].paramLen = 0;
		if (*pDataLength > MAX_CONFIG_READWRITE_BLOCK_SIZE)
		{
			/* block too large */
			*pDataLength = MAX_CONFIG_READWRITE_BLOCK_SIZE;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
	}

	/* first check to see if this is for the fingerprint device or matching algorithm */
	if (cvFPConfigType == CV_FP_CONFIG_DATA_MATCHING)
	{
		/* this type is not implemented for now */
		return CV_FEATURE_NOT_AVAILABLE;
#if 0
		/* data for matching algorithm, must have CV admin auth */
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
		if (!(authFlags & (CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV)))
		{
			/* correct auth not granted, fail */
			retVal = CV_AUTH_FAIL;
			goto err_exit;
		}

		/* auth ok, do read/write to flash here */
		if ((ushx_flash_read_write(CV_DP_FLASH_BLOCK_CUST_ID, offset, *pDataLength, pData, (read) ? 0 : 1)) != PHX_SUCCESS)
		{
			/* read/write failure, translate to CVAPI return code */
			retVal = CV_FLASH_ACCESS_FAILURE;
			goto err_exit;
		}
#endif
	} else if (cvFPConfigType == CV_FP_CONFIG_DATA_DEVICE) {
		/* just call fpConfig() */
#ifdef PHX_REQ_FP
		fpStatus = fpConfig(offset, *pDataLength, pData, read);
#endif
		retVal = cvXlatefpRetCode(fpStatus);
	} else
		/* invalid fp config type */
		retVal = CV_INVALID_INPUT_PARAMETER;
		
err_exit:
	return retVal;
}

cv_status
cv_fingerprint_test (cv_handle cvHandle,             
			uint32_t tests, uint32_t *pResults, 
			uint32_t extraDataLength, byte *pExtraData)
{
	/* This routine is provided to allow for testing of the fingerprint device.  It just calls the */
	/* fpTest routine for the appropriate FP device library */
	fp_status fpStatus = FP_OK;
	cv_status retVal = CV_SUCCESS;
	unsigned int i = 0;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		return CV_FP_NOT_ENABLED;
	}

	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT))
		return CV_FP_NOT_PRESENT;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;
	/* if extraDataLength is zero, set pExtraData to NULL pointer */
	if (!extraDataLength)
		pExtraData = NULL;

	/* not allowed if FP image buf not available, which implies an async FP capture is active */
	if (!isFPImageBufAvail())
		return CV_FP_DEVICE_BUSY;
	/* Clear results */
	*pResults = 0;

	if ( is_fp_synaptics_present() && 2 == tests)
	{
		printf("Synaptics - Clearing reset pin\n");
		fp_spi_nreset(0);
	}
	else
	{
	/* invoke indicated routine */
#ifdef PHX_REQ_FP
		printf("calling fpTest tests=%d pResults=%d extraDataLength=%d pExtraData=%d\n", tests, pResults, extraDataLength, pExtraData);
		fpStatus = fpTest(tests, pResults, extraDataLength, pExtraData);
		printf("fpTest returns %d results:%d\n", fpStatus, *pResults);
		if (pExtraData != NULL)
		{
			for (i=0;i<extraDataLength;++i)
			{
				printf("pExtraData[%d]=%d\n", i, pExtraData[i]);
			}
		}
#endif
	}
	retVal = cvXlatefpRetCode(fpStatus);
	/* If asked to reset the FP sensor, do so */
	if (retVal == CV_FP_RESET) {
		/*fpClose();*/
		fpSensorReset();
	}
	return retVal;
}

cv_status
cv_fingerprint_calibrate(cv_handle cvHandle)
{
	uint16_t pVID, pPID;
	/* This routine is provided to allow for calibration of the fingerprint device.  It just calls */
	/* the fpCalibrate routine for the appropriate FP device library */
	fp_status fpStatus = FP_OK;
	cv_status retVal = CV_SUCCESS;
	uint32_t interval, minInterval;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		return CV_FP_NOT_ENABLED;
	}
		
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT))
		return CV_FP_NOT_PRESENT;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* not allowed if FP image buf not available.  s/b ok, because no capture s/b active while this routine called */
	if (!isFPImageBufAvail())
		return CV_FP_DEVICE_BUSY;

	pVID = usb_fp_vid();
	pPID = usb_fp_pid();
	
#ifdef PHX_REQ_FP
	/* If upek area sensor, we use calibrate to calibrate command to re-sync sensor to board
	   when sensors are swapped */
	if (pVID == FP_UPKA_VID && pPID == FP_UPKA_PID)
	  {
		/* don't allow flash update more often that every 5 minutes, to prevent flash burnout */
		if ((CV_VOLATILE_DATA->CVDeviceState & CV_ANTIHAMMERING_ACTIVE))
		{
			interval = cvGetDeltaTime(CV_VOLATILE_DATA->timeLastFPConfigWrite);
			/* workaround to account for Atmel flash requiring 2 writes for each page write */
			if (VOLATILE_MEM_PTR->flash_device == PHX_SERIAL_FLASH_ATMEL)
				minInterval = 2*CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
			else
				minInterval = CV_MIN_TIME_BETWEEN_FLASH_UPDATES;
			/* for first time through (ie, last time is 0) need to init start time */
			if ((interval < minInterval) && (CV_VOLATILE_DATA->timeLastFPConfigWrite != 0))
			{
				if (interval > CV_FP_CONFIGURE_DURATION)
				{
					/* here if PBA code download should have completed.  if not, return antihammering status */
					retVal = CV_ANTIHAMMERING_PROTECTION;
					goto err_exit_ah;
				}
			} else
				/* here if initiating sequence of commands to write PBA flash.  initialize start time */
				get_ms_ticks(&CV_VOLATILE_DATA->timeLastFPConfigWrite);
		}
		fpClose();
		if ((fpStatus = fpInit(FP_CAPTURE_SMALL_HEAP_SIZE, FP_CAPTURE_SMALL_HEAP_PTR)) != CV_SUCCESS)
			goto err_exit;
		memset(FP_CAPTURE_LARGE_HEAP_PTR, 0, 4);
		if ((fpStatus = fp1Config(4, 4, FP_CAPTURE_LARGE_HEAP_PTR, 0)) != FP_OK)
			goto err_exit;
		if ((fpStatus = fp1Config(4, 4, FP_CAPTURE_LARGE_HEAP_PTR, 1)) != FP_OK)
			goto err_exit;
		if ((fpStatus = fpOpen(FP_CAPTURE_LARGE_HEAP_SIZE, FP_CAPTURE_LARGE_HEAP_PTR)) != FP_OK)
			goto err_exit;
	  }

	/* invoke indicated routine */
	else
		fpStatus = fpCalibrate(); 	/* On upek request do not calibrate sensor from FW, 
									   Upek uses host software for calibration */
#endif
err_exit:
	retVal = cvXlatefpRetCode(fpStatus);
	/* If asked to reset the FP sensor, do so */
	if (retVal == CV_FP_RESET) {
		/*fpClose();*/
		fpSensorReset();
	}
err_exit_ah:
	return retVal;
}

/* This routine gets called by the usb host driver when fp device is enumerated */
void
cvFpEnum(void)
{
	cv_phx2_device_status fpDevStatus;

	/* Detect and initialize fp sensor */
		fpDevStatus = cvInitFPDevice();

	if (fpDevStatus != CV_SUCCESS) {
		CV_VOLATILE_DATA->PHX2DeviceStatus |= CV_PHX2_FP_DEVICE_FAIL;	/* Set failure bit */
		CV_PERSISTENT_DATA->deviceEnables &= ~(CV_FP_PRESENT);			/* Clear CV_FP_PRESENT bit */
	}
	else {
		CV_VOLATILE_DATA->PHX2DeviceStatus &= ~(CV_PHX2_FP_DEVICE_FAIL);	/* Clear failure bit */
		CV_PERSISTENT_DATA->deviceEnables |= CV_FP_PRESENT;					/* Set CV_FP_PRESENT bit */
		CV_VOLATILE_DATA->fpSensorState |= CV_FP_WAS_PRESENT;				/* Set was present bit */
		CV_VOLATILE_DATA->fpSensorState |= CV_FP_ENUMERATED;			/* Set enumerated bit */
	}
}		

/* This routine gets called by the usb host driver when fp device is de-enumerated */
void
cvFpDenum(void)
{
	cvDetectFPDevice();		/* This clears sensor detect bits if device is not detected */
	CV_VOLATILE_DATA->PHX2DeviceStatus |= CV_PHX2_FP_DEVICE_FAIL;
	CV_PERSISTENT_DATA->deviceEnables &= ~(CV_FP_PRESENT);				/* Clear CV_FP_PRESENT bit */	
}

/*  Routine to create DPFR Context for use with feature extraction, enrollment and verification */
DPFR_HANDLE DPCreateContext(unsigned char * heap_ptr, uint32_t heap_size) {
  
	// overhead for each memory block allocated on the heap
	const size_t memory_block_control_structure_size = 8;
	const size_t heap_overhead = 36;
	int status = 0;
	DPFR_RESULT rc = FR_OK;
	DPFR_MEMORY_REQUIREMENTS mem_req = {0};
  
  
	size_t total_heap_size;
  
	DPFR_CONTEXT_INIT ctx_init = {
    
		sizeof(DPFR_CONTEXT_INIT), 	// length
		heap_ptr,         			// heapContext
		&pvPortMalloc_in_heap,     	// malloc
		&vPortFree_in_heap,        	// free
		(unsigned int (*)(void))&get_current_tick_in_ms    	// get_current_tick_in_ms
	};
  
	DPFR_HANDLE h_context = NULL;

	rc = DPFRGetMemoryRequirements(sizeof(mem_req), &mem_req);
  
	if (!DPFR_SUCCESS(rc)) 
  		return NULL;
  
  	total_heap_size = 5 * mem_req.handle + 1 * mem_req.context + 1 * mem_req.fpFeatureSet
    	+ 1 * mem_req.fpTemplate + 1 * mem_req.alignmentData + 1 * mem_req.comparisonOperation
    	+ 10 * memory_block_control_structure_size + heap_overhead;
  
    if (total_heap_size > heap_size) 
  		return NULL;
  		
  	status = init_heap(heap_size, heap_ptr);
  
  	if (status == FALSE) 
  		return NULL;
  		
  	rc = DPFRCreateContext(&ctx_init, &h_context);
  	if (!DPFR_SUCCESS(rc)) 
  		return NULL;

  	return h_context;
}

#if defined(SYNAPTICS_DEBUG_OUTPUT) || defined(NEXT_DEBUG_OUTPUT) || defined (PRINT_LARGE_HEAP)

static void printBuffer(uint8_t* p, uint32_t size)
{
    uint32_t i = 0;
    
    for(i = 0; i < size; i ++)
    {
        if(p[i] < 16)
        {
            //Print an extra zero
            printf("0");
        }
        printf("%x", p[i]);
    }
    printf("\n");
}

#endif /* defined(SYNAPTICS_DEBUG_OUTPUT) || defined(NEXT_DEBUG_OUTPUT) || defined (PRINT_LARGE_HEAP) */

#ifdef NEXT_DEBUG_OUTPUT

/*Next Biometrics Defines*/
#define IMAGE_LENGTH  180
#define IMAGE_WIDTH_12_X_17   256
#define IMAGE_WIDTH_12_X_12   180


typedef struct
{
	uint8_t formatId[4];
	uint8_t versionNo[4];
	uint8_t recordLength[6];//36 + Number Views*(14bytes + DataLength)
	uint8_t cbeffPID[4];
	uint8_t captureDeviceId[2];
	uint16_t imgAcquisitionLvl;
	uint8_t numFingers;
	uint8_t scaleUnits;
	uint16_t scanHorizRes;
	uint16_t scanVertRes;
	uint16_t imagHorizRes;
	uint16_t imageVertRes;
	uint8_t pixelDepth;
	uint8_t imgComprAlg;
	uint8_t reserved[2];
} GENERAL_RECORD_t;


typedef struct
{
    //allocating higher width so that 12x12 or 12x17 images can be supported in here
    uint8_t col[IMAGE_WIDTH_12_X_17];
}ROW_t;

typedef struct
{

    ROW_t rows[IMAGE_LENGTH];
} SCAN_IMAGE_t;

typedef struct
{
    uint32_t length;
    uint8_t  fingerPosition;
    uint8_t  countOfViews;
    uint8_t  viewNumber;
    uint8_t  fingerImgQuality;
    uint8_t  impressiontType;
    uint16_t horizontalLineLen;
    uint16_t verticalLineLen;
    uint8_t  reserved;
    union
    {
        uint8_t image1D[IMAGE_LENGTH* IMAGE_WIDTH_12_X_17];
        SCAN_IMAGE_t image2D;
    };
} FINGER_RECORD_t;

typedef struct
{
	GENERAL_RECORD_t generalRecord;
	FINGER_RECORD_t fingerprintRecord; 
} IMAGE_t;


static void printImage(IMAGE_t * image)
{
    uint32_t i = 0;
    uint32_t imageWidth = 256;
    
    printf("Printing Image...\n");

    printf("image width value %u\n", imageWidth);
    for(i = 0; i < 180* imageWidth; i ++)
    {
        if(image->fingerprintRecord.image1D[i] < 16)
        {
            //Print an extra zero
            printf("0");
        }
            printf("%x", image->fingerprintRecord.image1D[i]);
    }
    printf("\n");
    printf("End of Image\n");
}

static void printEntireSmallHeap(IMAGE_t * image)
{
	uint8_t* p = (uint8_t*)image;
    
    printf("Printing Small Heap...\n");

	printBuffer(p, FP_CAPTURE_SMALL_HEAP_SIZE);

	printf("End of Small Heap\n");
}
#endif // NEXT_DEBUG_OUTPUT

#ifdef PRINT_LARGE_HEAP

static void printEntireLargeHeap(uint8_t* p)
{
    printf("Printing Large Heap...\n");

	printBuffer(p, FP_CAPTURE_LARGE_HEAP_SIZE);

    printf("End of Large Heap\n");
}

#endif // PRINT_LARGE_HEAP

cv_status
cvSynapticsOnChipFeatureExtraction(uint32_t *featureSetLength, uint8_t *featureSet)
{
	int rc = 0;
	SynaFeatureSet featureSetInSynapticsHeap = NULL;
#ifdef SYNAPTICS_MATCHER_TIMING		
	unsigned int start, end;
#endif	


#ifdef PRINT_LARGE_HEAP
		printf("About to print large heap (before feature extraction)\n");
		printEntireLargeHeap(FP_CAPTURE_LARGE_HEAP_PTR);
		printf("Done printing large heap.\n");
#endif

#ifdef SYNAPTICS_MATCHER_TIMING	
	phx_start_time_timer(60000);  	
	start= get_time_timer_val();
#endif

	printf("SynaExtractFeatureSet start\n");
	rc = SynaExtractFeatureSet(&featureSetInSynapticsHeap, featureSetLength);
	printf("SynaExtractFeatureSet end rc=%d\n", rc);

#ifdef SYNAPTICS_DEBUG_OUTPUT
	printf("Featureset after extraction length:%d\n", *featureSetLength);
	printBuffer(featureSetInSynapticsHeap, *featureSetLength);
	printf("End featureset after extraction\n");
#endif

#ifdef SYNAPTICS_MATCHER_TIMING	    
	end = get_time_timer_val();
	printf("Feature Extraction Time: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	

	if (*featureSetLength > MAX_FP_FEATURE_SET_SIZE)
	{
		printf("Feature set too large %d\n", *featureSetLength);
		return CV_FP_BAD_SWIPE;
	}

	memcpy(featureSet, featureSetInSynapticsHeap, *featureSetLength);

#ifdef SYNAPTICS_DEBUG_OUTPUT
	printf("Featureset after copying\n");
	printBuffer(featureSet, *featureSetLength);
	printf("End featureset after copying\n");
#endif

	if (rc != 0)
	{
		printf("SynapticsOnChipFeatureExtraction error\n");
		return CV_FP_BAD_SWIPE;
	}

	return CV_SUCCESS;
}

cv_status
cvDPFROnChipFeatureExtraction(uint32_t imageLength, cv_fp_fe_control cvFpFeControl, uint32_t *featureSetLength, uint8_t *featureSet)
{
#ifdef DP_TIMING		
	unsigned int start, end;
#endif	
  	DPFR_RESULT retVal = FR_OK;
  	DPFR_HANDLE hCtx;
  	DPFR_HANDLE hFeatureSet = NULL;
	DPFR_PURPOSE purpose = 0;
  	
#ifdef DP_TIMING	
  	printf("DP OnChip feature extraction\n");
 	
	phx_start_time_timer(60000);  	
	start= get_time_timer_val();
#endif
 	
#ifdef DP_TIMING	
	printf("DPCreateContext start\n");
#endif
  	hCtx = DPCreateContext(FP_MATCH_HEAP_PTR, FP_MATCH_HEAP_SIZE);
#ifdef DP_TIMING
	printf("DPCreateContext end\n");
#endif
	/* if any purpose bits set, use them, otherwise default to 'all' */
	if (cvFpFeControl)
	{
		if (cvFpFeControl & CV_FE_CONTROL_PURPOSE_ENROLLMENT) purpose |= DPFR_PURPOSE_ENROLLMENT; 
 		if (cvFpFeControl & CV_FE_CONTROL_PURPOSE_VERIFY) purpose |= DPFR_PURPOSE_VERIFICATION; 
		if (cvFpFeControl & CV_FE_CONTROL_PURPOSE_IDENTIFY) purpose |= DPFR_PURPOSE_IDENTIFICATION; 
	} else
		purpose = DPFR_PURPOSE_ALL;
 	
	ENTER_CRITICAL();  
  	if (hCtx != NULL) {

#ifdef NEXT_DEBUG_OUTPUT
		printf("About to print image\n");
		printImage(FP_CAPTURE_LARGE_HEAP_PTR);
		printf("Done printing image.\n");

		printf("About to print small heap\n");
		printEntireSmallHeap(FP_CAPTURE_SMALL_HEAP_PTR);
		printf("Done printing small heap.\n");
#endif

#ifdef PRINT_LARGE_HEAP
		printf("About to print large heap\n");
		printEntireLargeHeap(FP_CAPTURE_LARGE_HEAP_PTR);
		printf("Done printing large heap.\n");
#endif

#ifdef DP_TIMING
		printf("DPFRCreateFeatureSetInPlace start. imageLength:%d\n", imageLength);
#endif
  		if ((retVal = DPFRCreateFeatureSetInPlace(hCtx, FP_CAPTURE_LARGE_HEAP_PTR, imageLength, 
  			DPFR_DT_ANSI_381_SAMPLE, purpose, 0, &hFeatureSet)) != FR_OK)
		{
  			printf("cvDPFROnChipFeatureExtraction: DPFRCreateFeatureSetInPlace failed 0x%x\n", retVal);
			VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily = 0; // Don't want to keep this temporary state if extraction failed
			goto err_exit;
		}
#ifdef DP_TIMING
		printf("DPFRCreateFeatureSetInPlace end\n");
		printf("DPFRCloseHandle start\n");
#endif
 		if ((retVal = DPFRCloseHandle(hCtx)) != FR_OK)
		{
    		printf("cvDPFROnChipFeatureExtraction: DPFRCloseHandle failed 0x%x\n", retVal);
			goto err_exit;
		}
#ifdef DP_TIMING	
		printf("DPFRCloseHandle end\n");
#endif
	}
	
  	if(hFeatureSet != NULL) {
#ifdef DP_TIMING
		printf("DPFRExport start\n");
#endif
		if ((retVal = DPFRExport(hFeatureSet, DPFR_DT_DP_FEATURE_SET, featureSet, featureSetLength)) != FR_OK)
		{
     		printf("cvDPFROnChipFeatureExtraction: DPFRExport failed 0x%x\n", retVal);
 			goto err_exit;
		}
#ifdef DP_TIMING		
		printf("DPFRExport end\n");
  		
		printf("DPFRCloseHandle start\n");
#endif
  		if ((retVal = DPFRCloseHandle(hFeatureSet)) != FR_OK)
		{
      		printf("cvDPFROnChipFeatureExtraction: DPFRCloseHandle2 failed 0x%x\n", retVal);
 			goto err_exit;
		}
#ifdef DP_TIMING	
		printf("DPFRCloseHandle end\n");
#endif
	}			
		
	retVal = CV_SUCCESS;	

err_exit:
    EXIT_CRITICAL();
    
#ifdef DP_TIMING	    
	end = get_time_timer_val();
	printf("Feature Extraction Time: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	

	return cvXlateDPRetCode(retVal);
}

static void
cvSynapticsMatcherCleanup()
{
	if (is_fp_synaptics_present())
	{
		printf("SynaMatcherCleanup\n");
		SynaMatcherCleanup();
		printf("SynaMatcherCleanup done\n");
	}
}

cv_status
cvSynapticsOnChipCreateTemplate(uint8_t* featureSet, uint32_t featureSetSize, uint8_t **pTemplate, uint32_t *templateSize, uint32_t *enrollComplete)
{
	int rc = 0;
#ifdef SYNAPTICS_MATCHER_TIMING		
	unsigned int start, end;
#endif	


#ifdef SYNAPTICS_MATCHER_TIMING	
	phx_start_time_timer(60000);  	
	start= get_time_timer_val();
#endif

#ifdef SYNAPTICS_DEBUG_OUTPUT
	printf("Featureset for enrollment\n");
	printBuffer(featureSet, featureSetSize);
	printf("End featureset for enrollment\n");
#endif

#ifdef SYNAPTICS_DEBUG_OUTPUT
	printf("SynaEnrollFeatureSet start pTemplate=%08x pFeatureSet=%08x featureSetSize=%d\n", *pTemplate, featureSet, featureSetSize);
#else
	printf("SynaEnrollFeatureSet start\n");
#endif
	rc = SynaEnrollFeatureSet((SynaTemplate*)pTemplate, templateSize, (SynaFeatureSet)featureSet, featureSetSize, enrollComplete);
	printf("SynaEnrollFeatureSet end rc=%d templateSize=%d enrollComplete=%d\n", rc, *templateSize, *enrollComplete);
	
#ifdef SYNAPTICS_MATCHER_TIMING	    
	end = get_time_timer_val();
	printf("Enrollment Time of one image: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	

#ifdef SYNAPTICS_DEBUG_OUTPUT
	if (*enrollComplete)
	{
		printf("Buffer after enrollment from Synaptics\n");
		printBuffer(*pTemplate, *templateSize);
		printf("End buffer after enrollment from Synaptics\n");
	}
#endif

	if (rc != 0)
	{
		return rc;
	}

	return CV_SUCCESS;
}

cv_status
cvDPFROnChipCreateTemplate(uint32_t featureSet1Length, uint8_t *featureSet1,
					 uint32_t featureSet2Length, uint8_t *featureSet2,
					 uint32_t featureSet3Length, uint8_t *featureSet3,
					 uint32_t featureSet4Length, uint8_t *featureSet4,
					 uint32_t *templateLength, cv_obj_value_fingerprint **createdTemplate)
{
	/* This routine is invoked to create a template (currently only on host) */
	cv_status retVal = CV_SUCCESS;
#ifdef DP_TIMING	
	unsigned int start, end;
#endif	
	DPFR_ENROLLMENT_STATUS dpEnrollStatus;

	DPFR_HANDLE hCtx;
	DPFR_HANDLE tempHandle;

	DPFR_HANDLE hEnrollment = NULL;
	DPFR_HANDLE hTemplate = NULL;
	size_t *tmplLength;
	uint8_t *bufPtr;
	cv_obj_value_fingerprint *objPtr;	

#ifdef DP_TIMING			
	printf("DP OnChip enrollment\n");
#endif	
	
	objPtr = (cv_obj_value_fingerprint *)FP_MATCH_HEAP_PTR;
	bufPtr = (uint8_t *)FP_MATCH_HEAP_PTR + sizeof(uint32_t);
	tmplLength = (size_t *)FP_MATCH_HEAP_PTR;

#ifdef DP_TIMING			
	phx_start_time_timer(60000);
	start= get_time_timer_val();
#endif	
	
  	hCtx = DPCreateContext(FP_MATCH_HEAP_PTR + MAX_FP_TEMPLATE_SIZE, (FP_MATCH_HEAP_SIZE - MAX_FP_TEMPLATE_SIZE));
  		  	
	ENTER_CRITICAL();
	 	
	if ((retVal = DPFRCreateEnrollment(hCtx, DPFR_ENROLLMENT_WITH_4_FEATURES_SETS, &hEnrollment)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRCreateEnrollment failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	if ((retVal = DPFRImport(hCtx, featureSet1, featureSet1Length,
			DPFR_DT_DP_FEATURE_SET,DPFR_PURPOSE_ENROLLMENT, &tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRImport 1 failed 0x%x\n",retVal);
		goto err_exit;
	}

	if ((retVal = DPFRAddFeatureSetToEnrollment(hEnrollment, tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRAddFeatureSetToEnrollment 1 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	DPFRGetEnrollmentStatus(hEnrollment, &dpEnrollStatus);
	/*printf("DP Enrollment Status: %x\n", dpEnrollStatus);*/
		
	if ((retVal = DPFRImport(hCtx, featureSet2, featureSet2Length, 
			DPFR_DT_DP_FEATURE_SET,DPFR_PURPOSE_ENROLLMENT,&tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRImport 2 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	if ((retVal = DPFRAddFeatureSetToEnrollment(hEnrollment, tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRAddFeatureSetToEnrollment 2 failed 0x%x\n",retVal);
		goto err_exit;
	}

	DPFRGetEnrollmentStatus(hEnrollment, &dpEnrollStatus);
	/*printf("DP Enrollment Status: %x\n", dpEnrollStatus);*/
		
	if ((retVal = DPFRImport(hCtx, featureSet3, featureSet3Length,
			DPFR_DT_DP_FEATURE_SET,DPFR_PURPOSE_ENROLLMENT,&tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRImport 3 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	if ((retVal = DPFRAddFeatureSetToEnrollment(hEnrollment, tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRAddFeatureSetToEnrollment 3 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	DPFRGetEnrollmentStatus(hEnrollment, &dpEnrollStatus);
		
	if ((retVal = DPFRImport(hCtx, featureSet4, featureSet4Length,
			DPFR_DT_DP_FEATURE_SET,DPFR_PURPOSE_ENROLLMENT,&tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRImport 4 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	if ((retVal = DPFRAddFeatureSetToEnrollment(hEnrollment, tempHandle)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRAddFeatureSetToEnrollment 4 failed 0x%x\n",retVal);
		goto err_exit;
	}
		
	DPFRGetEnrollmentStatus(hEnrollment, &dpEnrollStatus);
		
	if ((retVal = DPFRCreateTemplate(hEnrollment, DPFR_PURPOSE_RECOGNITION, &hTemplate)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRCreateTemplate failed 0x%x\n",retVal);
		goto err_exit;
	}

    if ((retVal = DPFRExport(hTemplate, DPFR_DT_DP_TEMPLATE_1640, bufPtr, tmplLength)) != FR_OK)
	{
		printf("cvDPFROnChipCreateTemplate: DPFRExport failed 0x%x\n",retVal);
		goto err_exit;
	}
    	
	DPFRCloseHandle(hCtx);
	
	objPtr->fpLen = *tmplLength;
	objPtr->type = CV_FINGERPRINT_TEMPLATE;
	*createdTemplate = objPtr;
	*templateLength = objPtr->fpLen;
	
err_exit:
    EXIT_CRITICAL();
    
#ifdef DP_TIMING	    
	end = get_time_timer_val();
	printf("Enrollment Time: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	
    
	return cvXlateDPRetCode(retVal);	
}

void cvStartTimerForWBFObjectProtection()
{
	CV_VOLATILE_DATA->wbfProtectData.didStartCRMUTimer = 1;
	CV_VOLATILE_DATA->wbfProtectData.CRMUElapsedTime = cv_start_timer2(); /* Store the elapsed time */
}

cv_status cvProcessingForUsingWBFForProtectingObjectsAfterFingerprintMatch(cv_obj_handle identifiedTemplate)
{
	cv_status retVal = CV_SUCCESS;
	/* generate random in CV_VOLATILE_DATA->wbfProtectData.sessionID */
	if ((retVal = cvRandom(sizeof(CV_VOLATILE_DATA->wbfProtectData.sessionID), CV_VOLATILE_DATA->wbfProtectData.sessionID)) != CV_SUCCESS)
	{
		printf("random generation failed with error %d\n", retVal);
		return retVal;
	}

	CV_VOLATILE_DATA->wbfProtectData.template_id_of_identified_fingerprint = identifiedTemplate;
	CV_VOLATILE_DATA->wbfProtectData.timeoutForObjectAccess = 0;
	#ifdef USE_CRMU_TIMER_FOR_TIME_BETWEEN_MATCH_AND_CONTROLUNIT_API 
		cvStartTimerForWBFObjectProtection();
	#else  /* If MAX_TIME_BETWEEN_WINBIOIDENTIFY_AND_CONTROL_UNIT_CALL_IN_SECONDS is small then it's ok to use a simple timer */
		get_ms_ticks(&CV_VOLATILE_DATA->wbfProtectData.startTimeCountForSimpleTimer);
	#endif

	return retVal;
}

cv_status
cvFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl, cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context)
{
	if (is_fp_synaptics_present())
	{
		return cvSynapticsFROnChipIdentify(cvHandle, FARvalue, fpControl, isIdentifyPurpose,
						featureSetLength, featureSet,
						templateCount, templateHandles,
						templateLength, pTemplate,
						identifiedTemplate, fingerDetected, match,
						callback, context);
	}
	else
	{
		return cvDPFROnChipIdentify(cvHandle, FARvalue, fpControl, isIdentifyPurpose,
						featureSetLength, featureSet,
						templateCount, templateHandles,
						templateLength, pTemplate,
						identifiedTemplate, fingerDetected, match,
						callback, context);
	}
}

cv_status
cvSynapticsFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl, cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context)
{
#ifdef SYNAPTICS_MATCHER_TIMING
	unsigned int start, end;
#endif	
	cv_status retVal = CV_SUCCESS;
	uint8_t featureSetLocal[MAX_FP_FEATURE_SET_SIZE];
	uint32_t imageLength, imageWidth;
	uint32_t synapticsMatch = 0;
	cv_obj_properties objProperties;
	cv_obj_hdr *objPtr;
	SynaTemplate synapticsTemplate;
	cv_obj_value_fingerprint *retTemplate;
	int rc = 0;
	uint32_t i;

#ifdef SYNAPTICS_MATCHER_TIMING		
	printf("Synaptics Onchip Identify\n");
#endif	

	*match = FALSE;
	*identifiedTemplate = NULL;

	/* check to see if feature set provided or if capture is necessary */
	if (!featureSetLength)
	{
		printf("Feature set not provided, starting capture\n");
		imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Init to MAX_FP_FEATURE_SET_SIZE */
		/* feature set not provided, do capture */
		if ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
					CV_SWIPE_NORMAL, callback, context)) == CV_FP_BAD_SWIPE) 
		{
			if (!fingerDetected) {
				imageLength = MAX_FP_FEATURE_SET_SIZE;		/* Reinit to MAX_FP_FEATURE_SET_SIZE */					
				while ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
						CV_RESWIPE, callback, context)) == CV_FP_BAD_SWIPE)
					imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Reinit to MAX_FP_FEATURE_SET_SIZE */
				}
				else
					return retVal;		
		}
		featureSet = featureSetLocal;
		featureSetLength = imageLength;
		printf("cvSynapticsFROnChipIdentify capture done %d\n", retVal);
		if (retVal != CV_SUCCESS)
			return retVal;
	} 

#ifdef SYNAPTICS_MATCHER_TIMING	
	phx_start_time_timer(60000);  	
	start= get_time_timer_val();
#endif	

	ENTER_CRITICAL();

	if (templateLength)
	{
		/* verify agains only one template that is passed in as a parameter*/
		printf("identifying one template only\n");
		
		synapticsTemplate = (SynaTemplate)pTemplate;

		printf("SynaMatchFeatureSet start featureset: 0x%08x featuresetlength: %d\n", featureSet, featureSetLength);
		rc = SynaMatchFeatureSet(synapticsTemplate, templateLength, (SynaFeatureSet)featureSet, featureSetLength, &synapticsMatch);
		printf("SynaMatchFeatureSet end rc=%d synapticsMatch=%d\n", rc, synapticsMatch);
		
		if (rc != 0)
		{
			goto err_exit;
		}

		if (synapticsMatch != SYNAPTICS_MATCH_RESULT_MATCH_AND_LIVE_FINGER)
		{
			printf("  Did not match fingerprint\n");
			goto err_exit;
		}

		*match = TRUE;
		printf("  Matched fingerprint\n");

		/* finish */
		goto err_exit;
	}

#ifdef FP_UART_OFF
	phx_uart_disable();
#endif

	/* have templates to authenticate against */
	for (i=0;i<templateCount;i++)
	{
		printf("Going to check template %d out of %d\n", i+1, templateCount);
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objHandle = templateHandles[i];
		if ((retVal = cvGetObj(&objProperties, &objPtr)) != CV_SUCCESS)
			continue;
		cvUpdateObjCacheLRUFromHandle(objProperties.objHandle);
		cvFindObjPtrs(&objProperties, objPtr);
		retTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
		if (retTemplate->type != CV_FINGERPRINT_TEMPLATE_SYNAPTICS)
		{
			printf("Template type is not Synaptics\n");
			continue;
		}

		synapticsTemplate = (SynaTemplate)(retTemplate->fp);

#ifdef SYNAPTICS_DEBUG_OUTPUT
		printf("Template for verification (length: %d)\n", retTemplate->fpLen);
		printBuffer(synapticsTemplate, retTemplate->fpLen);
		printf("End template for verification\n");
#endif

		printf("SynaMatchFeatureSet start featureset: 0x%08x featuresetlength: %d\n", featureSet, featureSetLength);
		rc = SynaMatchFeatureSet(synapticsTemplate, retTemplate->fpLen, (SynaFeatureSet)featureSet, featureSetLength, &synapticsMatch);
		printf("SynaMatchFeatureSet end rc=%d synapticsMatch=%d\n", rc, synapticsMatch);

		if (rc != 0)
		{
			continue;
		}
		if (synapticsMatch == SYNAPTICS_MATCH_RESULT_MATCH_AND_LIVE_FINGER)
		{
			*match = TRUE;
			*identifiedTemplate = templateHandles[i];
			break;
		}
	}

#ifdef FP_UART_OFF
	phx_uart_enable();
#endif

	cvSynapticsMatcherCleanup();

	if ((*match) == TRUE)
	{
		printf("  Matched fingerprint\n");
	}
	else
	{
		printf("  Did not match fingerprint\n");
		goto err_exit;
	}

	retVal = cvProcessingForUsingWBFForProtectingObjectsAfterFingerprintMatch(*identifiedTemplate);

err_exit:	
    EXIT_CRITICAL();
				
#ifdef SYNAPTICS_MATCHER_TIMING		
	end = get_time_timer_val();
	printf("Identify Time: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	


	return cvXlateDPRetCode(retVal);
}



cv_status
cvDPFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl, cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context)
{
#ifdef DP_TIMING
	unsigned int start, end;
#endif	
	cv_status retVal = CV_SUCCESS;
	cv_status matchStatus;
	uint8_t featureSetLocal[MAX_FP_FEATURE_SET_SIZE];
	uint32_t imageLength, imageWidth;
	PDPFR_STORAGE pDPStore = NULL;
	DPFR_HANDLE hCtx;
	DPFR_HANDLE hFtrSet = NULL;
	DPFR_HANDLE hIdOper = NULL;
	DPFR_HANDLE hCandidate = NULL;
  	uint32_t identifiedTemplateIndex;
	DPFR_SUBJECT_ID subjectID = {0};
	DPFR_PURPOSE purpose = (isIdentifyPurpose) ? DPFR_PURPOSE_IDENTIFICATION : DPFR_PURPOSE_VERIFICATION;
	
#ifdef DP_TIMING		
	printf("DP Onchip Identify\n");
#endif	
	printf("cvDPFROnChipIdentify\n");
	/* check to see if feature set provided or if capture is necessary */
	if (!featureSetLength)
	{
		printf("Feature set not provided, starting capture\n");
		imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Init to MAX_FP_FEATURE_SET_SIZE */
		/* feature set not provided, do capture */
		if ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
					CV_SWIPE_NORMAL, callback, context)) == CV_FP_BAD_SWIPE) 
		{
			if (!fingerDetected) {
				imageLength = MAX_FP_FEATURE_SET_SIZE;		/* Reinit to MAX_FP_FEATURE_SET_SIZE */					
				while ((retVal = cvUSHfpCapture(FALSE, fingerDetected, fpControl, featureSetLocal, &imageLength, &imageWidth,
						CV_RESWIPE, callback, context)) == CV_FP_BAD_SWIPE)
					imageLength = MAX_FP_FEATURE_SET_SIZE;	/* Reinit to MAX_FP_FEATURE_SET_SIZE */
				}
				else
					return retVal;		
		}
		featureSet = featureSetLocal;
		featureSetLength = imageLength;
		printf("cvDPFROnChipIdentify capture done %d\n", retVal);
		if (retVal != CV_SUCCESS)
			return retVal;
	} 
#if 0
	/* check to see if template provided directly or if template handle list provided */
	/* This is special verify case */
	/* If so, create a temporary volatile cv object for template */
	if (templateLength)
	{
		printf("Creating cv volatile object\n");
		/* Re-using capture buffer temporarily to store template object */
		fpObjPtr = (cv_obj_value_fingerprint *)FP_MATCH_HEAP_PTR;	
		fpObjPtr->fpLen = templateLength;
		fpObjPtr->type = CV_FINGERPRINT_TEMPLATE;
		memcpy(fpObjPtr->fp, pTemplate, templateLength);
		
		/* save as temporary volatile object */
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		/*objProperties.objAttributesLength = 0;
		objProperties.pObjAttributes = NULL;*/
		objProperties.authListsLength = 0;
		objProperties.pAuthLists = NULL;
		objProperties.objAttributesLength = 8;
		objProperties.pObjAttributes = &objAttributes;
		objAttributes.attribLen = 4;
		objAttributes.attribType = 0;
						
		objProperties.objValueLength = templateLength + sizeof(cv_obj_value_fingerprint);
		objProperties.pObjValue = fpObjPtr;
		/* check to make sure object not too large */
		if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
		{
			retVal = CV_INVALID_OBJECT_SIZE;
			return retVal;
		}
		objProperties.objHandle = 0;
		objPtr.objType = CV_TYPE_FINGERPRINT;
		objPtr.objLen = cvComputeObjLen(&objProperties);
		/* now save object */
		if ((retVal = cvHandlerPostObjSave(&objProperties, &objPtr, callback, context) != CV_SUCCESS))
			return retVal;
		*templateHandles = objProperties.objHandle;
	}
#endif
#ifdef DP_TIMING		
	printf("Allocating DPstore\n");
#endif	
							
	/* Allocate DP Store area */
	pDPStore = (PDPFR_STORAGE)cv_malloc(sizeof(DPFR_STORAGE));
	if (pDPStore == NULL)
		return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
		
	/* Initialize the structure */
	pDPStore->length = sizeof(DPFR_STORAGE);
	pDPStore->get_sizes = cvFPSAGetSizes;
	pDPStore->locate = (DPFR_RESULT (*)(void *, const unsigned int *, DPFR_FINGER_POSITION))cvFPSALocate;
	pDPStore->move_to = (DPFR_RESULT (*)(void *, size_t))cvFPSAMoveToTemplate;
	pDPStore->next = cvFPSANextTemplate;
	pDPStore->get_current_finger = (DPFR_RESULT (*)(void *,  unsigned int *, DPFR_FINGER_POSITION *))cvFPSAGetCurrentFinger;
	pDPStore->get_current_template = cvFPSAGetCurrentTemplate;
	pDPStore->hint_current_cache = cvFPSAHintCurrentCache;
	pDPStore->update_current_template = cvFPSAUpdateCurrentTemplate;

#ifdef DP_TIMING	
	phx_start_time_timer(60000);  	
	start= get_time_timer_val();

	printf("Initializing DPStore\n");
#endif	
	/* Initialize storage API */
	printf("cvFPSAInit templateCount=%d\n", templateCount);
	if ((retVal = cvFPSAInit(cvHandle, templateCount, pDPStore, templateHandles, templateLength, pTemplate)) != FR_OK)
	{
		printf("cvFPSAInit returned %d\n", retVal);
		return retVal;
	}
		
	/*printf("Calling DP functions\n");*/
	ENTER_CRITICAL();
	/* Create DPFR Context */
  	hCtx = DPCreateContext(FP_MATCH_HEAP_PTR, FP_MATCH_HEAP_SIZE);
  	if (hCtx != NULL) 
  	{
		if ((retVal = DPFRImport(hCtx, featureSet, featureSetLength,
			DPFR_DT_DP_FEATURE_SET, purpose, &hFtrSet)) != FR_OK)
			goto err_exit;
		
		DPFRCloseHandle(hCtx);
		  	
		if ((retVal = DPFRCreateIdentificationFromStorage(hFtrSet, pDPStore, 
			FARvalue, 0, &hIdOper)) != FR_OK)
			goto err_exit;
		
		DPFRCloseHandle(hFtrSet);
	
		matchStatus = DPFRGetNextCandidate(hIdOper, DPFR_INFINITE, &hCandidate);
			
  		if (matchStatus == FR_OK) {
  			*match = TRUE;
		}
  		else if (matchStatus == FR_WRN_END_OF_LIST) {
  			*match = FALSE;
  			retVal = FR_OK;
		}
		else {
			retVal = matchStatus;
			printf("DPFRGetNextCandidate error: %x\n", retVal);
			goto err_exit;
		}
		
		if (*match) {
			/* Matched, get subjectId */				
			if ((retVal = DPFRGetSubjectID(hCandidate, subjectID)) != FR_OK)
				goto err_exit;

			identifiedTemplateIndex = subjectID[0];	
			*identifiedTemplate = templateHandles[identifiedTemplateIndex];
			printf("  Matched fingerprint\n");

			retVal = cvProcessingForUsingWBFForProtectingObjectsAfterFingerprintMatch(*identifiedTemplate);
			if (retVal != CV_SUCCESS)
			{
				goto err_exit;
			}
		}
		else
		{
			*identifiedTemplate = NULL;
			printf("  Did not match fingerprint\n");
		}
		  			  		
		DPFRCloseHandle(hCandidate);
		DPFRCloseHandle(hIdOper);
	} else
		retVal = (uint32_t)FR_ERR_CANNOT_CREATE_DB;

	// don't keep this temporary state after match completes
	VOLATILE_MEM_PTR->dontCallNextFingerprintRebaseLineTemporarily = 0;

err_exit:	
    EXIT_CRITICAL();
    /* Free DP Storage template list */
	cvFPSAFreeTemplatesList();
				
	/* Free DP Store */
	if (pDPStore)
		cv_free(pDPStore, sizeof(DPFR_STORAGE));
#if 0	
	/* If temporary object created, delete object */
	if ((templateLength) && (objProperties.objHandle))
		retVal = cvDelObj(&objProperties, FALSE);
#endif
		
#ifdef DP_TIMING		
	end = get_time_timer_val();
	printf("Identify Time: %d; [ %u - %u]\n", (start - end)/(TIMER_CLK_HZ/1000), start , end);
#endif	

	return cvXlateDPRetCode(retVal);
}

cv_status
cv_fingerprint_capture_start(cv_handle cvHandle,
			uint32_t enrollCount, cv_fp_cap_control fpCapControl,
			cv_nonce *captureID,
			cv_callback *callback, cv_callback_ctx context)
{
	/* this routine starts an asynchronous FP capture */
	uint16_t pVID;
	cv_status retVal = CV_SUCCESS;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* check to make sure FP device is enabled */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
		retVal = CV_FP_NOT_ENABLED;
		goto err_exit;
	}
	
	/* check to make sure FP device present */
	if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT)) {
		retVal = CV_FP_NOT_PRESENT;
		goto err_exit;
	}

	/* check to see if need to restart FP capture */
	if (fpCapControl & CV_CAP_CONTROL_RESTART_CAPTURE)
	{
		printf("cv_fingerprint_capture_start:resume from a sleep... restart capture \n");
	}

    /* Check if to skip the session validation */
    if( !(CV_CAP_CONTROL_NOSESSION_VALIDATION & fpCapControl) )
    {
	    /* make sure this is a valid session handle */
	    if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
	    {
		    printf("cv_fingerprint_capture_start: session validation failed \n");
		    return retVal;
	    }
    }

	/* first check to see if there's a capture already in progress or a valid FP image buffer */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURE_IN_PROGRESS)
	{
		/* if this was an internal FP capture request, just cancel it */
		if (CV_VOLATILE_DATA->fpCapControl & CV_CAP_CONTROL_INTERNAL)
		{
			printf("cv_fingerprint_capture_start: calling cvFpCancel\n");
			cvFpCancel(FALSE);
		} else
		{
			printf("cv_fingerprint_capture_start:FP device busy \n");
			return CV_FP_DEVICE_BUSY;
		}
	}

	if (fpCapControl & CV_CAP_CONTROL_OVERRIDE_PERSISTENCE)
	{
		/* invalidate persisted image and feature set */
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID;
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
	} else {
		if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_IMAGE_VALID)
		{
			/* check to see if persistence timer has timed out */
			if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) <= (CV_PERSISTENT_DATA->fpPersistence))
				return CV_PERSISTED_FP_AVAILABLE;
			else
				/* timer has elapsed, invalidate FP image */
				CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID;
		}
		/* if capture will automatically do FE, also check to a feature set is available */
		if ((fpCapControl & CV_CAP_CONTROL_DO_FE) && (CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID))
		{
			/* check to see if persistence timer has timed out */
			if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) <= (CV_PERSISTENT_DATA->fpPersistence))
				return CV_PERSISTED_FP_AVAILABLE;
			else
				/* timer has elapsed, invalidate feature set */
				CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
		}
	}

	/* create capture ID */
	if(!(fpCapControl & CV_CAP_CONTROL_RESTART_CAPTURE))
	{
		if ((retVal = cvRandom(sizeof(cv_nonce),(uint8_t *)&CV_VOLATILE_DATA->captureID.nonce[0])) != CV_SUCCESS)
		{
			printf("cv_fingerprint_capture_start: cvRandom failed 0x%x\n", retVal);
			goto err_exit;
		}
		memcpy(captureID, (uint8_t *)&CV_VOLATILE_DATA->captureID.nonce[0], sizeof(cv_nonce));
	}
	else
	{
        if( captureID != NULL ) {
		    printf("cv_fingerprint_capture_start: Restart mode - copy the capture to cv colatile data\n");
		    memcpy((uint8_t *)&CV_VOLATILE_DATA->captureID.nonce[0],captureID, sizeof(cv_nonce));
        }
	}

	/* initialize capture variables */
	CV_VOLATILE_DATA->fpCaptureStatus = FP_OK;
	CV_VOLATILE_DATA->fpCapControl = fpCapControl;
	/* if this isn't an internal request, save fpCapControl flags in case of FP cap restart */
	if (!(fpCapControl & CV_CAP_CONTROL_INTERNAL))
		CV_VOLATILE_DATA->fpCapControlLastOSPresent = fpCapControl;
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURE_CANCELLED | CV_FP_REINIT_CANCELLATION);

	/* enable finger detect */
	/* if this is the first time after power up, AT 2810 will need calibration */
	pVID = usb_fp_vid();
	if ((CV_VOLATILE_DATA->CVDeviceState & CV_FP_CHECK_CALIBRATION) && (pVID == FP_AT_VID))
	{
		printf("cv_fingerprint_capture_start: do calibration\n");
		if (fpCalibrate() == FP_SENSOR_RESET_REQUIRED)
		{
			/* reset sensor */
			printf("cv_fingerprint_capture_start: ESD occurred during fpCalibrate, restart\n");
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
			fpSensorReset();
			retVal = CV_SUCCESS;
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURE_IN_PROGRESS;
			goto err_exit;
		}
	}
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CHECK_CALIBRATION;

restart_capture:
	/* first, check to see if previous async FP task needs to be terminated */
	prvCheckTasksWaitingTermination();
    CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_TERMINATE_ASYNC;
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURE_IN_PROGRESS;
	if ((retVal = cvUSHFpEnableFingerDetect(TRUE)) == CV_FP_RESET)
	{
		/* reset sensor */
		printf("cv_fingerprint_capture_start: ESD occurred during cvUSHFpEnableFingerDetect, restart\n");
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_RESTART_HW_FDET;
		fpSensorReset();
		retVal = CV_SUCCESS;
	}
	if (retVal == CV_SUCCESS)
		CV_VOLATILE_DATA->CVDeviceState |= CV_FP_CAPTURE_IN_PROGRESS;

err_exit:
	return retVal;
}

cv_status
cv_fingerprint_capture_cancel(void)
{
	/* This routine will cancel any FP capture in progress and also invalidate any persisted */
	/* FP image and feature set */
	cv_status retVal = CV_SUCCESS;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* disable finger detect and cancel capture */
	if ((retVal = cvUSHFpEnableFingerDetect(FALSE)) != CV_SUCCESS)
		goto err_exit;

	/* clear device status bits associated with capture */
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_IMAGE_VALID | CV_FP_CAPTURE_IN_PROGRESS 
		| CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

err_exit:

    /* if need to restart fp capture after s3 resume, cancel this request. */
    if( lynx_get_fp_sensor_state() )
    {
        lynx_save_fp_sensor_state();  // clear the sensor state in M0_SRAM
    }

	return retVal;
}

cv_status
cv_fingerprint_capture_get_result (cv_handle cvHandle, cv_bool getRawImage,
			cv_nonce *captureID,
			uint32_t *pResultLength, byte *pResult)
{
	/* This routine will return either the persisted FP image or feature set */
	cv_status retVal = CV_SUCCESS;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* check to see if FP image requested */
	if (getRawImage)
	{
		if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_IMAGE_VALID))
			return CV_NO_VALID_FP_IMAGE;

		/* check to see if image still valid */
		if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
		{
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID;
			return CV_NO_VALID_FP_IMAGE;
		}

		/* make sure that captureID matches one saved when capture initiated */
		if (memcmp(captureID, &CV_VOLATILE_DATA->captureID, sizeof(cv_nonce)))
			return CV_INVALID_CAPTURE_ID;

		if (*pResultLength < CV_VOLATILE_DATA->fpPersistedFPImageLength)
		{
			/* output buffer size insufficient, fail */
			*pResultLength = CV_VOLATILE_DATA->fpPersistedFPImageLength;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pResultLength = CV_VOLATILE_DATA->fpPersistedFPImageLength;

		/* don't actually copy buffer here, because FP image will be transferred directly from capture buffer */
	} else {
		/* here to get feature set */
		if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID))
			return CV_NO_VALID_FP_IMAGE;

		/* check to see if image still valid */
		if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
		{
			CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
			return CV_NO_VALID_FP_IMAGE;
		}

		/* make sure that captureID matches one saved when capture initiated */
		if (memcmp(captureID, &CV_VOLATILE_DATA->captureID, sizeof(cv_nonce)))
			return CV_INVALID_CAPTURE_ID;

		if (*pResultLength < CV_VOLATILE_DATA->fpPersistedFeatureSetLength)
		{
			/* output buffer size insufficient, fail */
			*pResultLength = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pResultLength = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
		memcpy(pResult, CV_PERSISTED_FP, CV_VOLATILE_DATA->fpPersistedFeatureSetLength);
	}
	/* clear device status bits associated with persisted image */
	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_IMAGE_VALID | CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);

err_exit:
	return retVal;
}

cv_status
cv_fingerprint_create_feature_set(cv_handle cvHandle, cv_fp_fe_control cvFpFeControl)
{
	/* This command just causes the persisted FP image to be used to create and persist a FP feature set */
	cv_status retVal = CV_SUCCESS;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_IMAGE_VALID))
		return CV_NO_VALID_FP_IMAGE;
	CV_VOLATILE_DATA->fpPersistedFeatureSetLength = MAX_FP_FEATURE_SET_SIZE;
	if (CHIP_IS_5882_OR_LYNX) {
		if (is_fp_synaptics_present())
		{
			if ((retVal = cvSynapticsOnChipFeatureExtraction(&CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP)) != CV_SUCCESS)
				goto err_exit;
		}
		else
		{
			if ((retVal = cvDPFROnChipFeatureExtraction(CV_VOLATILE_DATA->fpPersistedFPImageLength,
				cvFpFeControl, &CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP)) != CV_SUCCESS)
				goto err_exit;
		}
	}
	else {
		if ((retVal = cvHostFeatureExtraction(CV_VOLATILE_DATA->fpPersistedFPImageLength, 
			&CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP)) != CV_SUCCESS)
			goto err_exit;
	}

	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_IMAGE_VALID; 
	CV_VOLATILE_DATA->CVDeviceState |= (CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID); 
	/* replace the purpose bits saved at capture time with these */
	CV_VOLATILE_DATA->fpCapControl &= ~CV_PURPOSE_MASK;
	CV_VOLATILE_DATA->fpCapControl |= (cvFpFeControl & CV_PURPOSE_MASK);

err_exit:
	if (retVal != CV_SUCCESS)
		/* set failed status to return to host */
		CV_VOLATILE_DATA->fpCaptureStatus = FP_STATUS_OPERATION_CANCELLED; /* this will get translated to CV_CAP_RSLT_POOR_QUALITY */
	return retVal;
}

cv_status
cv_fingerprint_commit_feature_set (cv_handle cvHandle, cv_nonce *captureID,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, uint32_t authListsLength, cv_auth_list *pAuthLists,
			cv_obj_handle *pFeatureSetHandle)
{
	/* This routine will create a CV object from the persisted feature set if available */
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_obj_value_fingerprint *fpObjHdr;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID))
		return CV_NO_VALID_FP_IMAGE;

	/* check to see if image still valid */
	if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
	{
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
		return CV_NO_VALID_FP_IMAGE;
	}

	/* make sure that captureID matches one saved when capture initiated */
	if (memcmp(captureID, &CV_VOLATILE_DATA->captureID, sizeof(cv_nonce)))
		return CV_INVALID_CAPTURE_ID;

	CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_IMAGE_VALID | CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID); 

	/* now save as object */
	/* insert header */
	memmove(CV_PERSISTED_FP + sizeof(cv_obj_value_fingerprint) - 1, CV_PERSISTED_FP, CV_VOLATILE_DATA->fpPersistedFeatureSetLength);
	fpObjHdr = (cv_obj_value_fingerprint *)CV_PERSISTED_FP;
	fpObjHdr->type = getExpectedFingerprintFeatureSetType();
	fpObjHdr->fpLen = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
	/* validate new auth lists provided */
	if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
		goto err_exit;
	if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
		goto err_exit;
	memset(&objProperties,0,sizeof(cv_obj_properties));
	objProperties.session = (cv_session *)cvHandle;
	objProperties.objAttributesLength = objAttributesLength;
	objProperties.pObjAttributes = pObjAttributes;
	objProperties.authListsLength = authListsLength;
	objProperties.pAuthLists = pAuthLists;
	objProperties.objValueLength = CV_VOLATILE_DATA->fpPersistedFeatureSetLength + sizeof(cv_obj_value_fingerprint) - 1;
	objProperties.pObjValue = CV_PERSISTED_FP;
	/* check to make sure object not too large */
	if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
	{
		retVal = CV_INVALID_OBJECT_SIZE;
		goto err_exit;
	}
	objProperties.objHandle = 0;
	objPtr.objType = CV_TYPE_FINGERPRINT;
	objPtr.objLen = cvComputeObjLen(&objProperties);
	/* now save object */
	retVal = cvHandlerPostObjSave(&objProperties, &objPtr, NULL, 0);
	*pFeatureSetHandle = objProperties.objHandle;

err_exit:
	return retVal;
}

cv_status
cv_fingerprint_update_enrollment_synaptics_imp(cv_handle cvHandle, cv_nonce *captureID,
											   uint32_t userAppHashListLength, uint8_t *pUserAppHashListBuf,
											   cv_bool *enrollmentComplete, cv_nonce *enrollmentID,
											   cv_obj_handle *pDupTemplateHandle )
{
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle 	dupTemplateHandle;
	uint32_t enrollComplete = 0;
	uint8_t *pSynapticsTemplate;
	uint32_t synapticsTemplateSize = 0;


	*pDupTemplateHandle = 0;
	*enrollmentComplete = FALSE;
	pSynapticsTemplate = (uint8_t *)(CV_VOLATILE_DATA->enrollmentFeatureSetLens[1]);

	if (!CHIP_IS_5882_OR_LYNX) {
		return CV_FEATURE_NOT_AVAILABLE;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS) {
		return retVal;
	}

	/* make sure no temporary FP template already exists from prior enrollment */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_TEMPLATE_VALID) {
		printf("cv_fingerprint_update_enrollment_synaptics_imp template exists from prior enrollment\n");
		return CV_FP_TEMPLATE_EXISTS;
	}

	/* make sure there is a valid feature set available */
	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID)) {
		printf("cv_fingerprint_update_enrollment_synaptics_imp feature set is not available\n");
		return CV_NO_VALID_FP_IMAGE;
	}

#ifndef PRINT_LARGE_HEAP
	if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
	{
		printf("cv_fingerprint_update_enrollment_synaptics_imp too much time passed\n");
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
		return CV_NO_VALID_FP_IMAGE;
	}
#endif

	/* make sure that captureID matches one saved when capture initiated */
	if (memcmp(captureID, &CV_VOLATILE_DATA->captureID, sizeof(cv_nonce))) {
		printf("cv_fingerprint_update_enrollment_synaptics_imp capture id does not match\n");
		return CV_INVALID_CAPTURE_ID;
	}

	/* save latest feature set */
	CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
	memcpy((CV_ENROLLMENT_FEATURE_SET_BUF),
		(uint8_t *)CV_PERSISTED_FP, CV_VOLATILE_DATA->fpPersistedFeatureSetLength);

	/* Add capture to the enrollment template */
	retVal = cvSynapticsOnChipCreateTemplate(CV_ENROLLMENT_FEATURE_SET_BUF0, CV_VOLATILE_DATA->enrollmentFeatureSetLens[0], &pSynapticsTemplate, &synapticsTemplateSize, &enrollComplete);
	if (retVal != 0)
	{
		/* template creation unsuccessful */
		CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] = 0;
		CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
		printf ("cv_fingerprint_update_enrollment_synaptics_imp CV_FP_MATCH_GENERAL_ERROR\n");
		return CV_FP_MATCH_GENERAL_ERROR;
	}
	
	if (enrollComplete == 0)
	{
		printf("More fingers needed\n");
		
		CV_VOLATILE_DATA->enrollmentFeatureSetLens[1] = (uint32_t)pSynapticsTemplate;
		return CV_SUCCESS;
	}
	CV_VOLATILE_DATA->enrollmentFeatureSetLens[1] = 0;

	/* capture all ~7 fingers now */
	/* check for duplicate fp */
	printf("checking for duplicate fingerprint\n");
	retVal = cvFpCheckDuplicate (cvHandle,
								CV_VOLATILE_DATA->enrollmentFeatureSetLens[0],
								CV_ENROLLMENT_FEATURE_SET_BUF0,
								userAppHashListLength, pUserAppHashListBuf,
								&dupTemplateHandle);
	if (retVal == CV_FP_DUPLICATE)
	{
		printf("cv_fingerprint_update_enrollment_synaptics_imp found duplicate fingerprint template: 0x%x\n", dupTemplateHandle);
		*pDupTemplateHandle = dupTemplateHandle;
		CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] = 0;
		CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
		return CV_FP_DUPLICATE;
	}
	

	/* success, save new template in feature set buffer */
	if ((retVal = cvRandom(sizeof(cv_nonce), (uint8_t *)enrollmentID)) != CV_SUCCESS)
	{
		return retVal;
	}

	CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] = synapticsTemplateSize;
	memcpy(CV_ENROLLMENT_FEATURE_SET_BUF, pSynapticsTemplate, synapticsTemplateSize);
	CV_VOLATILE_DATA->CVDeviceState |= CV_FP_TEMPLATE_VALID;
	memcpy(&CV_VOLATILE_DATA->enrollmentID, enrollmentID, sizeof(cv_nonce));

	*enrollmentComplete = TRUE;
	printf("cv_fingerprint_update_enrollment_synaptics_imp Enrollment complete\n");
	return CV_SUCCESS;
}

cv_status
cv_fingerprint_update_enrollment(cv_handle cvHandle, cv_nonce *captureID,
								  uint32_t userAppHashListLength, uint8_t *pUserAppHashListBuf,
								  cv_bool *enrollmentComplete, cv_nonce *enrollmentID,
								  cv_obj_handle *pDupTemplateHandle )
{
	/* This CV command causes the persisted FP feature set which has previously been captured to be added */
	/* to the set of feature sets used for the template to be created.   Because the context of the DP template */
	/* creation application is not saved between calls to this function, it will be necessary to persist each of */
	/* the feature sets as they’re generated by successive swipes and defer the template creation until 4 feature */
	/* sets are available.  Because of the limited storage and the desire to have a small maximum number of */
	/* swipes required for enrollment, the DP enrollment will be initialized with the flag */
	/* DPFR_ENROLLMENT_WITH_4_FEATURES_SETS, which will allow creation of a template with 4 feature sets even if the */
	/* algorithm would have otherwise indicated more feature sets are required. */

	/* In order to validate the usage of the captured image, the calling application is required to provide the */
	/* captureID.  Similarly, when a template is created, an enrollmentID is returned, which will be required for */
	/* creation of a template CV object from the template. */

	cv_status retVal = CV_SUCCESS;
	uint32_t retTemplateLength = 0;
	cv_obj_value_fingerprint *retTemplate = NULL;
	uint32_t numFeatureSets = 1;
	uint32_t i;
	cv_obj_handle 	dupTemplateHandle;

	printf("cv_fingerprint_update_enrollment\n");
	
	if (is_fp_synaptics_present())
	{
		return cv_fingerprint_update_enrollment_synaptics_imp(cvHandle, captureID,
															   userAppHashListLength, pUserAppHashListBuf,
															   enrollmentComplete, enrollmentID,
															   pDupTemplateHandle );
	}

	if (!CHIP_IS_5882_OR_LYNX) {
		*pDupTemplateHandle = 0;
		return CV_FEATURE_NOT_AVAILABLE;
	}

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS) {
		*pDupTemplateHandle = 0;
		return retVal;
	}

	/* make sure no temporary FP template already exists from prior enrollment */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_FP_TEMPLATE_VALID) {
		*pDupTemplateHandle = 0;
		return CV_FP_TEMPLATE_EXISTS;
	}

	/* make sure there is a valid feature set available */
	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_CAPTURED_FEATURE_SET_VALID)) {
		*pDupTemplateHandle = 0;
		return CV_NO_VALID_FP_IMAGE;
	}

	if (cvGetDeltaTime(CV_VOLATILE_DATA->fpPersistanceStart) > (CV_PERSISTENT_DATA->fpPersistence))
	{
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_CAPTURED_FEATURE_SET_VALID;
		*pDupTemplateHandle = 0;
		return CV_NO_VALID_FP_IMAGE;
	}

	/* make sure that captureID matches one saved when capture initiated */
	if (memcmp(captureID, &CV_VOLATILE_DATA->captureID, sizeof(cv_nonce))) {
		*pDupTemplateHandle = 0;
		return CV_INVALID_CAPTURE_ID;
	}

	/* count the number of feature sets available */
	if (CV_VOLATILE_DATA->enrollmentFeatureSetLens[0]) numFeatureSets++;
	if (CV_VOLATILE_DATA->enrollmentFeatureSetLens[1]) numFeatureSets++;
	if (CV_VOLATILE_DATA->enrollmentFeatureSetLens[2]) numFeatureSets++;

	/* now check to see if enough feature sets (4) available to create template */
	if (numFeatureSets < 4)
	{
		/* not enough data for template yet.  save latest feature set */
		CV_VOLATILE_DATA->enrollmentFeatureSetLens[numFeatureSets - 1] = CV_VOLATILE_DATA->fpPersistedFeatureSetLength;
		memcpy((CV_ENROLLMENT_FEATURE_SET_BUF + (numFeatureSets - 1)*MAX_FP_FEATURE_SET_SIZE),
			(uint8_t *)CV_PERSISTED_FP, CV_VOLATILE_DATA->fpPersistedFeatureSetLength);
		*enrollmentComplete = FALSE;
	} else {
		// check for duplicate fp
        //printf("cv_fingerprint_update_enrollment() check for duplicate fingerprint\n");
		retVal = cvFpCheckDuplicate (cvHandle,
									CV_VOLATILE_DATA->enrollmentFeatureSetLens[0],
									CV_ENROLLMENT_FEATURE_SET_BUF0,
									userAppHashListLength, pUserAppHashListBuf,
									&dupTemplateHandle);
		if (retVal == CV_FP_DUPLICATE)
		{
			printf("cv_fingerprint_update_enrollment() found duplicate fingerprint template: 0x%x\n", dupTemplateHandle);
			*pDupTemplateHandle = dupTemplateHandle;
			for (i=0;i<3;i++) CV_VOLATILE_DATA->enrollmentFeatureSetLens[i] = 0;
			*enrollmentComplete = FALSE;
			CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
			return CV_FP_DUPLICATE;
		}
	
		/* 4 feature sets, create template */
		if (CHIP_IS_5882_OR_LYNX)
			retVal = cvDPFROnChipCreateTemplate(
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[0], CV_ENROLLMENT_FEATURE_SET_BUF0,
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[1], CV_ENROLLMENT_FEATURE_SET_BUF1,
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[2], CV_ENROLLMENT_FEATURE_SET_BUF2,
						CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP,
						&retTemplateLength, &retTemplate);
		else
			retVal = cvUSHfpCreateTemplate(
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[0], CV_ENROLLMENT_FEATURE_SET_BUF0,
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[1], CV_ENROLLMENT_FEATURE_SET_BUF1,
						CV_VOLATILE_DATA->enrollmentFeatureSetLens[2], CV_ENROLLMENT_FEATURE_SET_BUF2,
						CV_VOLATILE_DATA->fpPersistedFeatureSetLength, CV_PERSISTED_FP,
						&retTemplateLength, &retTemplate);

		if (retVal == CV_SUCCESS) {
			/* success, save new template in feature set buffer */
			if ((retVal = cvRandom(sizeof(cv_nonce), (uint8_t *)enrollmentID)) != CV_SUCCESS)
			{
				*pDupTemplateHandle = 0;
				return retVal;
			}
			CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] = retTemplateLength;
			memcpy(CV_ENROLLMENT_FEATURE_SET_BUF, &retTemplate->fp, retTemplate->fpLen);
			CV_VOLATILE_DATA->CVDeviceState |= CV_FP_TEMPLATE_VALID;
			memcpy(&CV_VOLATILE_DATA->enrollmentID, enrollmentID, sizeof(cv_nonce));
			*enrollmentComplete = TRUE;
		} else {
			/* template creation unsuccessful */
			for (i=0;i<3;i++) CV_VOLATILE_DATA->enrollmentFeatureSetLens[i] = 0;
			*enrollmentComplete = FALSE;
			CV_VOLATILE_DATA->CVDeviceState &= ~(CV_FP_CAPTURED_FEATURE_SET_VALID | CV_HAS_CAPTURE_ID);
			*pDupTemplateHandle = 0;
			return CV_FP_MATCH_GENERAL_ERROR;
		}
	}
	*pDupTemplateHandle = 0;
	return CV_SUCCESS;
}

cv_status
cv_fingerprint_discard_enrollment (void)
{
	/* This CV command causes the created, but not committed, FP enrollment template to be discarded */
	uint32_t i;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	if (CV_VOLATILE_DATA->enrollmentFeatureSetLens[1])
	{
		cvSynapticsMatcherCleanup();
	}

	/* clear persisted feature sets and/or template and flag template as invalid */
	for (i=0;i<3;i++) CV_VOLATILE_DATA->enrollmentFeatureSetLens[i] =0;
	CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_TEMPLATE_VALID;
	return CV_SUCCESS;
}

cv_status
cv_fingerprint_commit_enrollment (cv_handle cvHandle, cv_nonce *enrollmentID,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pTemplateLength, uint8_t * pTemplate, cv_obj_handle *pTemplateHandle)
{
	/* This CV command causes one of the following actions:  template is returned to caller if the value */
	/* pointed to by pTemplateLength is non-zero or a CV FP template object is created and the resulting */
	/* handle is returned to caller. An enrollmentID is used to ensure that the calling application which */
	/* created the temporary template is the one which is allowed to commit the template. */
	cv_status retVal = CV_SUCCESS;
	cv_obj_properties objProperties;
	cv_obj_hdr objPtr;
	cv_obj_value_fingerprint *fpObjHdr;
	uint32_t i;
	uint32_t templateLen = 0;

	if (!CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	if (!(CV_VOLATILE_DATA->CVDeviceState & CV_FP_TEMPLATE_VALID))
		return CV_NO_VALID_FP_TEMPLATE;

	/* make sure that enrollmentID matches one saved when enrollment initiated */
	if (memcmp(enrollmentID, &CV_VOLATILE_DATA->enrollmentID, sizeof(cv_nonce)))
		return CV_INVALID_ENROLLMENT_ID;

	/* check to see if template should be returned or a CV object should be created and the handle returned */
	if (*pTemplateLength == 0)
	{
		/* invalidate persisted template */
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_FP_TEMPLATE_VALID; 
		templateLen = CV_VOLATILE_DATA->enrollmentFeatureSetLens[0];
		/* reinit lengths */
		for (i=0;i<3;i++) CV_VOLATILE_DATA->enrollmentFeatureSetLens[i] = 0;

		/* now save as object */
		/* insert header */
		memmove(CV_ENROLLMENT_FEATURE_SET_BUF + sizeof(cv_obj_value_fingerprint) - 1, CV_ENROLLMENT_FEATURE_SET_BUF, templateLen);
		fpObjHdr = (cv_obj_value_fingerprint *)CV_ENROLLMENT_FEATURE_SET_BUF;
		fpObjHdr->type = getExpectedFingerprintTemplateType();
		fpObjHdr->fpLen = templateLen;
		/* validate new auth lists provided */
		if ((retVal = cvValidateAuthLists(CV_DENY_ADMIN_AUTH, authListsLength, pAuthLists)) != CV_SUCCESS)
			goto err_exit;
		if ((retVal = cvValidateAttributes(objAttributesLength, pObjAttributes)) != CV_SUCCESS)
			goto err_exit;
		memset(&objProperties,0,sizeof(cv_obj_properties));
		objProperties.session = (cv_session *)cvHandle;
		objProperties.objAttributesLength = objAttributesLength;
		objProperties.pObjAttributes = pObjAttributes;
		objProperties.authListsLength = authListsLength;
		objProperties.pAuthLists = pAuthLists;
		objProperties.objValueLength = templateLen + sizeof(cv_obj_value_fingerprint) - 1;
		objProperties.pObjValue = CV_ENROLLMENT_FEATURE_SET_BUF;

#ifdef SYNAPTICS_DEBUG_OUTPUT
		printf("Buffer for enrollment\n");
		printBuffer(objProperties.pObjValue, objProperties.objValueLength);
		printf("End buffer for enrollment\n");
#endif

		/* check to make sure object not too large */
		if ((cvComputeObjLen(&objProperties)) > MAX_CV_OBJ_SIZE)
		{
			printf("cv_fingerprint_commit_enrollment Bad size. templateLen:%d\n", templateLen);
			retVal = CV_INVALID_OBJECT_SIZE;
			goto err_exit;
		}
		objProperties.objHandle = 0;
		objPtr.objType = CV_TYPE_FINGERPRINT;
		objPtr.objLen = cvComputeObjLen(&objProperties);
		/* now save object */
		retVal = cvHandlerPostObjSave(&objProperties, &objPtr, NULL, 0);
		if (CV_SUCCESS != retVal)
		{
			printf("cv_fingerprint_commit_enrollment cvHandlerPostObjSave error %d\n", retVal);
		}
		*pTemplateHandle = objProperties.objHandle;
		printf("cv_fingerprint_commit_enrollment new object handle: %d\n", objProperties.objHandle);

        /* the first time fp enrolled, enable POA in EC */
        if( CV_PERSISTENT_DATA->poa_enable && ush_get_fp_template_count() == 1 )
        {
            cv_manage_poa(NULL, TRUE, CV_PERSISTENT_DATA->poa_timeout, CV_PERSISTENT_DATA->poa_max_failedmatch, FALSE);
        }
	} else {
		/* return template to calling app */
		if (CV_VOLATILE_DATA->enrollmentFeatureSetLens[0] > *pTemplateLength)
		{
			/* buffer too small, return error and correct length */
			*pTemplateLength = CV_VOLATILE_DATA->enrollmentFeatureSetLens[0];
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*pTemplateLength = CV_VOLATILE_DATA->enrollmentFeatureSetLens[0];
		memcpy(pTemplate, CV_ENROLLMENT_FEATURE_SET_BUF, CV_VOLATILE_DATA->enrollmentFeatureSetLens[0]);
	}

err_exit:
	cvSynapticsMatcherCleanup();
	return retVal;


}

cv_status
cv_fingerprint_create_template(cv_handle cvHandle, uint32_t featureSet1Length, uint8_t *featureSet1, 
							   uint32_t featureSet2Length, uint8_t *featureSet2, 
							   uint32_t featureSet3Length, uint8_t *featureSet3, 
							   uint32_t featureSet4Length, uint8_t *featureSet4, 
							   uint32_t *fpTemplateLength, uint8_t *fpTemplate)
{
	/* this is a 5880-only routine, which creates a template given 4 feature sets and returns it to the calling program */
	cv_status retVal = CV_SUCCESS;
	uint32_t retTemplateLength = 0;
	cv_obj_value_fingerprint *retTemplate = NULL;

	if (CHIP_IS_5882_OR_LYNX)
		return CV_FEATURE_NOT_AVAILABLE;

	/* make sure this is a valid session handle */
	if ((retVal = cvValidateSessionHandle(cvHandle)) != CV_SUCCESS)
		return retVal;

	/* 4 feature sets, create template */
	retVal = cvUSHfpCreateTemplate(
				featureSet1Length, featureSet1,
				featureSet2Length, featureSet2,
				featureSet3Length, featureSet3,
				featureSet4Length, featureSet4,
				&retTemplateLength, &retTemplate);

	if (retVal == CV_SUCCESS) {
		/* return template to calling app */
		if (retTemplate->fpLen > *fpTemplateLength)
		{
			/* buffer too small, return error and correct length */
			*fpTemplateLength = retTemplate->fpLen;
			retVal = CV_INVALID_OUTPUT_PARAMETER_LENGTH;
			goto err_exit;
		}
		*fpTemplateLength = retTemplate->fpLen;
		memcpy(fpTemplate, &retTemplate->fp, retTemplate->fpLen);
	}

err_exit:
	return retVal;;
}


