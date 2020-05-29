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
 * Include files
*/
#include <semaphore.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "CVUIGlobalDefinitions.h"
#include "CVUserInterfaceUtils.h"
#include "CVUIThreadHandlers.h"
#include "CVLogger.h"
#include "linuxifc.h"

//For subdividing buffer into 64 byte pkts when sending Small SBI in SBL
#define CV_ENCAP_COMMAND_CMD_OFFSET     8
#define MAX_BUF_SIZE_FOR_SBL            64
//Tell driver this is for FW upgrade in SBL, no packet can be this long, just flags
#define FW_UPGRADE_IN_SBL_MASK          0x80000000
#define FW_UPGRADE_IN_SBL_LAST_PKT_MASK 0x40000000

static sem_t 			*pCvSem;

/*
 * defintions used
 */
#define INITIAL_SEMAPHORES_COUNT	1			/* initial semaphore count */
#define MAXIMUM_SEMAPHORES_COUNT	1			/* maximum number of semaphore resources */
#define SEMAPHORE_RELEASE_COUNT		1			/* semaphore release count */
/* 
 * Variable declarations global to this file
 */
uint32_t	uPreviousCount = 0;					/* semaphore count */
int			hSemaphore;	/* semaphore handle */
key_t		semKey;
cv_bool		asyncFPCaptureRequest = FALSE;

static cv_fp_callback_ctx cvFPCallbackCtx;

void printBuffer(uint8_t *buf, int len, uint8_t addr)

{

	uint8_t		*pchar=(uint8_t*)buf, data;

	uint32_t	idx;



	if (len>0x100) {

		printf("Buffer length: 0x%x, only printing 0x100\n", len);

		len = 0x100;

	}

	for (idx=0; idx<len; idx++)

	{

		if ( (idx%8) == 0 && addr)

			printf("%p:  ", pchar );

		data = *pchar++; 

		printf("%02x ", data );

		if ( (idx%8) == 7 )

			printf( "\n" );

	}
	if ( (idx%8) != 0 )
		printf( "\n" );
}



void logBuffer(uint8_t *buf, int len, uint8_t addr)

{

	char		tmpBuf[128], byteBuf[16];

	uint8_t		*pchar=(uint8_t*)buf, data;

	uint32_t	idx;



	if (len>0x100) {

		logMessage("Buffer length: 0x%x, only printing 0x100\n", len);

		len = 0x100;

	}

	memset(tmpBuf, 0, 128);

	for (idx=0; idx<len; idx++)

	{
		if ( (idx%8) == 0 && addr) {
			sprintf(byteBuf, "%p:  ", pchar);
			strcat(tmpBuf, byteBuf);
		}
		data = *pchar++; 

		sprintf(byteBuf, "%02x ", data );

		strcat(tmpBuf, byteBuf);

		if ( (idx%8) == 7 ) {

			strcat(tmpBuf, "\n" );

			logMessage(tmpBuf);

			memset(tmpBuf, 0, 128);

		}

	}
	if ( (idx%8) != 0 )
		strcat(tmpBuf, "\n" );
		logMessage(tmpBuf);
}



/*
 * Function:
 *      initializeData

 * Purpose:
 *      Initializes the global variables

 * Parameters:
 *		callbackInfo <-> callback info
 */
void
initializeData(IN	cv_fp_callback_ctx **callbackInfo)
{
	pCvSem = sem_open(CV_SEM_NAME, O_CREAT, S_IRUSR|S_IWUSR, 1);

	if ( pCvSem == SEM_FAILED )

	{
		// failed to initialize
		logMessage("%s: sem_open() failed 0x%x\n", __FUNCTION__, error);

	}
	logMessage("%s: sem_open() pCvSem: 0x%x\n", __FUNCTION__, pCvSem);


	if (*callbackInfo == NULL)
	{
		*callbackInfo = &cvFPCallbackCtx;
		(*callbackInfo)->callback = NULL;
	}

	asyncFPCaptureRequest = FALSE;
}

/*
 * Function:
 *      isResubmitted
 * Purpose:
 *      Checks whether the command is resubmitted or not 
 * Parameters:
 *      ptr_cv_encap_command	--> encapsulated command 
 * Returns:
 *      BOOL					<-- TRUE if resubmitted
 *									FALSE otherwise
 */
BOOL 
isResubmitted(
			 IN	cv_encap_command *ptr_cv_encap_command
			 )
{
	BOOL bResult = FALSE;						/* fresh command */
	BEGIN_TRY
	{
		if ((ptr_cv_encap_command != NULL) &&	/* valid command? */
			(ptr_cv_encap_command->flags.returnType == CV_RET_TYPE_RESUBMITTED))
		{
			bResult = TRUE;
		}
	}
	END_TRY

	return bResult;
}
/*
 * Function:
 *      isCallbackPresent
 * Purpose:
 *      Checks whether the command callback is present or not 
 * Parameters:
 *      pCommand	--> encapsulated command 
 * Returns:
 *      BOOL		<-- TRUE if callback present
 *					    FALSE otherwise
 */
BOOL
isCallbackPresent(
				IN	cv_encap_command *pCommand
				)
{
	BOOL bPresent = FALSE;
	BEGIN_TRY
	{
		bPresent = pCommand->flags.completionCallback ? TRUE : FALSE;
	}
	END_TRY

	return bPresent;
}
/*
 * Function:
 *      isPINRequired
 * Purpose:
 *      Checks whether status requires PIN? 
 * Parameters:
 *      status	--> status
 * Returns:
 *      BOOL	<-- TRUE if requires PIN
 *					FALSE otherwise
 */
BOOL
isPINRequired(
			  IN	cv_status status
			  )
{
	switch(status)
	{
	/*case CV_PROMPT_PIN_AND_SMART_CARD:
	case CV_PROMPT_PIN_AND_CONTACTLESS:
		break;*/
	case CV_PROMPT_PIN:
		return TRUE;
	default:
		return FALSE;
	}
	return FALSE;
}


/*
 * Function:
 *      interrupt_read
 * Purpose:
 *     waits for the interrupt 
 * Parameters:
 *		NULL       
 * Returns:
 *    NULL
 */
void *
interrupt_read(void * Param)
{
	int *desc = (int *)Param;
	unsigned char intrINBuf[8];
	int iStat;
	logMessage("Start interrupt_read \n");

	while(1){
        memset(intrINBuf, 0 ,sizeof(intrINBuf));
        iStat = ioctl(*desc, CV_GET_LATEST_COMMAND_STATUS, intrINBuf);
		logMessage("interrupt_read after ioctl\n");

        if(iStat == 0)
            break;
    }
	return 0;
}

static int find_brcm_dev(libusb_device *dev, uint16_t *pidProduct, unsigned char* pEpBulkAddr, unsigned char* pEpIntAddr) {
	struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config;
    const struct libusb_interface *inter;
	const struct libusb_interface_descriptor *interdesc;
	const struct libusb_endpoint_descriptor *epdesc;
    int    result = 0, i;
    BOOL   bulk_addr_set = FALSE, int_addr_set = FALSE;

    if (!pidProduct) {
        logMessage("Error: pidProduct is NULL\n");
        return result;
    }

    if (!pEpBulkAddr) {
        logMessage("Error: pEpBulkAddr is NULL\n");
        return result;
    }

    if (!pEpIntAddr) {
        logMessage("Error: pEpIntAddr is NULL\n");
        return result;
    }

    int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		return result;
	}

    if (desc.idVendor == USB_CV_VENDOR_ID &&
        (desc.idProduct == USB_CV_PID_5840 ||
         desc.idProduct == USB_CV_PID_5841 ||
         desc.idProduct == USB_CV_PID_5842 ||
         desc.idProduct == USB_CV_PID_5843 ||
         desc.idProduct == USB_CV_PID_5844 ||
         desc.idProduct == USB_CV_PID_5845)) {
        *pidProduct = desc.idProduct;
        libusb_get_config_descriptor(dev, 0, &config);
        inter = &config->interface[0];
        interdesc = &inter->altsetting[0];

        for(i=0; i<(int)interdesc->bNumEndpoints; i++) {
			epdesc = &interdesc->endpoint[i];
            switch (epdesc->bmAttributes & 3) {
            case LIBUSB_TRANSFER_TYPE_BULK:
                if(!bulk_addr_set) *pEpBulkAddr = (int)epdesc->bEndpointAddress & 0x7F;
                bulk_addr_set = TRUE;
                break;
            case LIBUSB_TRANSFER_TYPE_INTERRUPT:
                if(!int_addr_set) *pEpIntAddr = (int)epdesc->bEndpointAddress & 0x7F;
                int_addr_set = TRUE;
                break;
            default:
                break;
            }
		}
        
        libusb_free_config_descriptor(config);

        if (bulk_addr_set && int_addr_set) result = 1;

    }

    return result;
}

/*
 * Function:
 *      processCommand
 * Purpose:
 *      Gets the device handle 
 * Parameters:
 *		dwIOCtrlCode	--> device open handle
 *		lpInBuffer		--> input buffer
 *		dwInBufferSize	--> input buffer length
 *		lpOutBuffer		--> output buffer
 *		dwOutBufferSize	--> output buffer length
 * Returns:
 *      BOOL			<-- TURE on success, FALSE otherwise
 */
BOOL
processCommand(
			   IN	DWORD dwIOCtrlCode,
			   IN	LPVOID lpInBuffer,
			   IN	DWORD dwInBufferSize,
			   OUT	LPVOID *lpOutBuffer,
			   OUT	DWORD dwOutBufferSize
			  )
{
	/* Changed retBufSize to long to match the ioctol return change in USB driver */
	// long retBufSize;
	BOOL retStatus=FALSE;
	unsigned long BufSize;
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_device_handle *dev_handle; //a device handle
	libusb_context *ctx = NULL; //a libusb session
	int r, actual, brcm_dev_found;  //r for return values int, actual for how many bytes were transferred
	ssize_t i, cnt;      //holding number of devices in list
    uint16_t idProduct;
    unsigned char data_in[USB_CV_MAX_INT_DATA_IN_LEN], ep_bulk_addr , ep_int_addr;

	write_buffer *stwBuf = NULL;
	logMessage("Inside processCommand\n");
	logMessage("dwIOCtrlCode: %ld\n", dwIOCtrlCode);

	r = libusb_init(&ctx); //initialize the library for the session we just declared
	if(r < 0) {
        logMessage("libusb init error\n");
        goto clean_up;
	}

	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if(cnt < 0) {
        logMessage("Get Device Error\n");
        goto clean_up;
	}
	
    brcm_dev_found = 0;	
	for(i = 0; i < cnt; i++) {
        if(find_brcm_dev(devs[i], &idProduct, &ep_bulk_addr, &ep_int_addr)) {
            brcm_dev_found++;
        }
	}

    if (brcm_dev_found != 1) {
        if (brcm_dev_found == 0) {
            logMessage("Broadcom device not found\n");
        }

        if (brcm_dev_found > 1) {
            logMessage("Error: More than 1 Broadcom devices found: %d\n", brcm_dev_found);
        }

        libusb_free_device_list(devs, 1); //free the list, unref the devices in it
        goto clean_up;
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, USB_CV_VENDOR_ID, idProduct);
	if(dev_handle == NULL) {
		logMessage("Cannot open device\n");
        libusb_free_device_list(devs, 1); 
        goto clean_up;
    }

	libusb_free_device_list(devs, 1);
	
    //find out if kernel driver is attached
	if(libusb_kernel_driver_active(dev_handle, 0) == 1) {
		logMessage("Kernel Driver already attached to device\n");
		if(libusb_detach_kernel_driver(dev_handle, 0) == 0) {
            //detach it
			logMessage("Kernel Driver detached!\n");
        } else {
            logMessage("Could not detach active kernel Driver!\n");

            // Need to exit?
            // goto clean_up;
        }
	}

	r = libusb_claim_interface(dev_handle, 0); //claim interface 0 (the first) of device

	if(r < 0) {
		logMessage("Cannot Claim Interface\n");
		goto clean_up;
	}

	BufSize = dwInBufferSize > dwOutBufferSize ? dwInBufferSize:dwOutBufferSize;
	logMessage("processCommand() io BufSize: 0x%lx\n", BufSize);

	stwBuf = (write_buffer *)malloc(sizeof(write_buffer)+BufSize-sizeof(unsigned char));

	if(NULL == stwBuf)
	{
		goto clean_up;
	}

	// FW Upgrade in SBI/AAI (data is less than 4KB)
	stwBuf->cmd_length = dwInBufferSize;

	memcpy(stwBuf->cmd_buffer, lpInBuffer, dwInBufferSize);

	logMessage("Input Buffer : 0x%lx\n",dwInBufferSize);
	logBuffer(stwBuf->cmd_buffer, dwInBufferSize, 0);

	r = libusb_bulk_transfer(dev_handle, (ep_bulk_addr | LIBUSB_ENDPOINT_OUT), stwBuf->cmd_buffer, stwBuf->cmd_length, &actual, MAX_WAIT_TX_TIME); 
	if(!(r == 0 && actual == stwBuf->cmd_length)) {
		logMessage("USB Bulk Write Error\n");
        goto clean_up_w_libusb;
    }

    /* Not expecting response data. */
    if (dwOutBufferSize == 0) {
        retStatus = TRUE;
        goto clean_up_w_libusb;
    }

    r = libusb_interrupt_transfer(dev_handle, (ep_int_addr | LIBUSB_ENDPOINT_IN), data_in, sizeof(data_in), &actual, MAX_WAIT_RX_TIME);
    if(r != 0) {
		logMessage("USB Interrupt Read Error\n");
        goto clean_up_w_libusb;
    }

    r = libusb_bulk_transfer(dev_handle, (ep_bulk_addr | LIBUSB_ENDPOINT_IN), stwBuf->cmd_buffer, BufSize, &actual, MAX_WAIT_RX_TIME);
    if(r != 0) {
       logMessage("USB Bulk Read Error\n");
       goto clean_up_w_libusb;
    }

    dwOutBufferSize = actual;

	logMessage("Outbuffer size: 0x%x\n", dwOutBufferSize);

    memcpy(*lpOutBuffer, stwBuf->cmd_buffer, dwOutBufferSize);
	logMessage("*lpOutBuffer:\n");
	logBuffer((uint8_t *)*lpOutBuffer, dwOutBufferSize, 0);
	logMessage("exit process command dwOutBufferSize  = %ld\n",dwOutBufferSize);

	retStatus = TRUE;

clean_up_w_libusb:
    r = libusb_release_interface(dev_handle, 0); //release the claimed interface
	if(r !=0 ) {
		logMessage("Cannot Release Interface\n");
	}

	libusb_close(dev_handle); //close the device we opened
	libusb_exit(ctx); //needs to be called to end the

clean_up:
	if (stwBuf)
		free(stwBuf);

	return retStatus;
}


/*
 * Function:
 *      saveCallbackInfo
 * Purpose:
 *      Returns the callback information from command buffer
 * Parameters:
 *		ptr_encap_cmd		--> encapsulated command
 *								SYNCHRONOUS/ASYNCHRONOUS
 *		callbackInfo		<-> callback information
 */
void
saveCallbackInfo(
			IN	cv_encap_command *ptr_encap_cmd,
			OUT	cv_fp_callback_ctx **callbackInfo
			)
{
	BEGIN_TRY
	{
		cv_fp_callback_ctx *CallbackInfo = *callbackInfo;
		CallbackInfo->callback = NULL;
		CallbackInfo->context = NULL;
		/* get the callback and callback context from the command */
		/* and store it locally */
		if (isCallbackPresent(ptr_encap_cmd) != FALSE) {
			logErrorMessage("flags.completionCallback = TRUE", ERROR_LOCATION);
			/* copy the callback function pointer and context info */
			memcpy(&CallbackInfo->callback,
				(((byte*)ptr_encap_cmd) + ptr_encap_cmd->transportLen),
				sizeof(CallbackInfo->callback));
			memcpy(&CallbackInfo->context,
				(((byte*)ptr_encap_cmd) + ptr_encap_cmd->transportLen +
				sizeof(CallbackInfo->callback)), sizeof(cv_callback_ctx));
		}
	}
	END_TRY
}
/*
 * Function:
 *      retrieveCommandIndex
 * Purpose:
 *      Returns the command index stored in the command
 * Parameters:
 *		pCommand			--> encapsulated command
 *		uCmdIndex			<-> command index
 */
void
retrieveCommandIndex(
			OUT	cv_encap_command *pCommand,
			OUT	uint32_t *uCmdIndex
			)
{
	BEGIN_TRY
	{
		uint32_t nSize = 0;

		if (isCallbackPresent(pCommand) == TRUE) {
			/* determine the extra buffer size */
			nSize = sizeof(cv_fp_callback_ctx);
		}

		/* copy the callback function pointer and context info */
		memcpy(uCmdIndex,
			(((byte*)pCommand) + pCommand->transportLen + nSize),
			sizeof(*uCmdIndex));
	}
	END_TRY
}
/*
 * Function:
 *      appendCallback
 * Purpose:
 *      Appends the callback context at the end of command
 * Parameters:
 *		pptr_encap_result	<-> encapsulated result
 *		context				-->	callback context
 */
void
appendCallback(
			   OUT	cv_encap_command **pptr_encap_result,
			   IN	cv_callback_ctx context
			   )
{
	BEGIN_TRY
	{
		uint32_t nSize = 0;
		cv_encap_command	*ptr_encap_result = *pptr_encap_result;

		if (isCallbackPresent(ptr_encap_result) == FALSE) {
			return;
		}

		/* determine the extra buffer size */
		nSize = sizeof(cv_callback_ctx);

		/* copy the callback function pointer and context info */
		memcpy((((byte*)ptr_encap_result) + ptr_encap_result->transportLen),
			&context, nSize);
	}
	END_TRY
}
/*
 * Function:
 *      appendCommandIndex
 * Purpose:
 *      Appends the unique command index on encapsulated command
 * Parameters:
 *		ppCommand		<-> encapsulated command
 */
void
appendCommandIndex(
				 OUT	cv_encap_command **ppCommand
				 )
{
	BEGIN_TRY
	{
		uint32_t nSize = 0;
		uint32_t uCmdIndex = getCommandIndex(FALSE);
		cv_encap_command	*pCommand = *ppCommand;

		if (isCallbackPresent(pCommand) == TRUE) {
			/* determine the extra buffer size */
			nSize = sizeof(cv_fp_callback_ctx);
		}

		/* copy the callback function pointer and context info */
		memcpy((((byte*)pCommand) + pCommand->transportLen + nSize),
			&uCmdIndex, sizeof(uCmdIndex));

	}
	END_TRY
}
/*
 * Function:
 *      cv_callback_status
 * Purpose:
 *      Used to invoke CV USB driver to send callback status
 * Parameters:
 *		ppCommandResult		<-> encapsulated result
 *		lpOverlapped		--> pointer to the overlapped object
 * Returns:
 *      cv_status			<-- status of the processed command
 */
cv_status 
cv_callback_status(
				   OUT	cv_encap_command **ppCommandResult, 
				   IN	LPOVERLAPPED lpOverlapped
				   )
{
	cv_encap_command *pCommandResult = *ppCommandResult;
	uint32_t nResultBufferSize = 0;

	BEGIN_TRY
	{
		/* CV_CMD_FINGERPRINT_CAPTURE command will have the result of MAX_ENCAP_FP_RESULT_BUFFER size.
		For all other cv command's result size will be MAX_ENCAP_RESULT_BUFFER */
		if (pCommandResult->commandID == CV_CMD_FINGERPRINT_CAPTURE)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommandResult->commandID == CV_CMD_RECEIVE_BLOCKDATA)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommandResult->commandID == CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else if (pCommandResult->commandID == CV_CMD_FINGERPRINT_CAPTURE_WBF)
			nResultBufferSize = MAX_ENCAP_FP_RESULT_BUFFER;
		else
			nResultBufferSize = MAX_ENCAP_RESULT_BUFFER;

		pCommandResult->commandID = CV_CALLBACK_STATUS;	/* callback status */
		/* process the command */
		if (processCommand(CV_GET_COMMAND_STATUS,
			pCommandResult, pCommandResult->transportLen, /*NULL, 0,*/
			(LPVOID*)&pCommandResult, nResultBufferSize) == FALSE)
		{
			logErrorMessage("Failed to get callback status", ERROR_LOCATION);
			pCommandResult->returnStatus = CV_UI_FAILED_COMMAND_PROCESS;
		}
	}
	END_TRY
	/* return the status*/
	return pCommandResult->returnStatus;
}




/*
 * Function:
 *      cv_abort
 * Purpose:
 *      Aborts the currently executing command
 * Parameters:
 *		ppCommandResult		<-> encapsulated result
 * Returns:
 *      cv_status			<-- status of the processed command
 */
cv_status 
cv_abort(
		 IN	cv_encap_command **ppCommandResult
		 )
{
	cv_encap_command *pCommandResult = *ppCommandResult;
	BEGIN_TRY
	{
		cv_encap_command *pCommand = (cv_encap_command*) malloc(sizeof(cv_encap_command));
		if (pCommand == NULL) {
			return CV_MEMORY_ALLOCATION_FAIL;
		}

		/* initialize the abort command */
		memset(pCommand, 0x0, sizeof(cv_encap_command));
		pCommand->commandID = CV_ABORT;		/* abort command */

		/* process the command */

		if (processCommand(CV_SUBMIT_CMD, 

			pCommand, pCommand->transportLen, 

			(LPVOID*)&pCommandResult, pCommandResult->transportLen) == FALSE) 


		{
			logErrorMessage("Failed to abort command processing", ERROR_LOCATION);
			pCommandResult->returnStatus = CV_UI_FAILED_ABORT_COMMAND;
		}

		/* free abort command */
		if (pCommand != NULL) {
			free(pCommand);
		}
	}
	END_TRY

	/* return the status*/
	return pCommandResult->returnStatus;
}
/*
 * Function:
 *      releaseSemaphore
 * Purpose:
 *      Release the semaphore occupied by previous command
 * Returns:
 *      BOOL	<-- TRUE on success FALSE otherwise
 */
BOOL
releaseSemaphore(void)
{
	int s;


	logMessage("releaseSemaphore() enter\n");



	if ( pCvSem == SEM_FAILED ) {

		logMessage("releaseSemaphore(): semaphore not initialized\n");

		return FALSE;
	}


	s = sem_post(pCvSem);

	if (s == -1) {

		logMessage("releaseSemaphore(): sem_post error (0x%x)\n", error);

		return FALSE;

	}



	logMessage("releaseSemaphore() exit\n");

	return TRUE;



}



/*
 * Function:
 *      createSemaphore
 * Purpose:
 *      Creates the semaphore to allow single command to be processed ar a time
 * Returns:
 *      BOOL	<-- TRUE on success FALSE otherwise
 */
BOOL
createSemaphore(void)
{
	int s;


	logMessage("createSemaphore() enter\n");



	if ( pCvSem == SEM_FAILED ) {

		logMessage("createSemaphore(): semaphore not initialized\n");

		return FALSE;
	}



	while ((s = sem_wait(pCvSem)) == -1 && errno == EINTR)

		continue;       /* Restart when interrupted by handler */



	/* Check what happened */

	if (s == -1) {

		if (errno == ETIMEDOUT)

			logMessage("createSemaphore(): sem_wait() timed out\n");

		else {

			logMessage("createSemaphore(): sem_wait error (0x%x)\n", errno);

		}
		return FALSE;

	}
	logMessage("createSemaphore(): sem_wait() succeeded\n");



	logMessage("createSemaphore() exit\n");

	return TRUE;
}

BOOL
closeSemaphore(void)
{
	int s;

	if ( pCvSem == SEM_FAILED ) {

		logMessage("closeSemaphore(): semaphore not initialized\n");

		return FALSE;
	}

	s = sem_close(pCvSem);

	if (s == -1) {
		logMessage("closeSemaphore(): sem_close error (0x%x)\n", errno);
		return FALSE;
	}
		
	logMessage("closeSemaphore(): sem_close() succeeded\n");

	return TRUE;
}

/*
 * Function:
 *      getCommandIndex
 * Purpose:
 *      get the command index
 * Parameter:
 *		bNewIndex - Create new index
 * Returns:
 *		DWORD - unique ID for a command
 */
uint32_t getCommandIndex(BOOL bNewIndex)
{
	if (bNewIndex == TRUE)
	{
		uCommandIndex += 1;
	}
	return uCommandIndex;
}
/*
 * Function:
 *      createThread
 * Purpose:
 *      get the command index
 * Parameter:
 *		lpThreadProc - thread procedure
 *      lParam - thread parameter
 * Returns:
 *		HANDLE - thread handle
 */
int createThread(
                void* lpThreadProc,
                void* lParam
                )
{
	pthread_t hThread;
	int iRes;

	iRes = pthread_create(&hThread, NULL, lpThreadProc, lParam);
	if(iRes != 0)
		return CV_FAILURE;
	return CV_SUCCESS;	
}
