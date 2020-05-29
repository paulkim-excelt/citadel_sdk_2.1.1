/*
 *  Copyright 2004
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  Irvine CA 92619-7013
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************/

#include "cvapi.h"
#include "cvinternal.h"
#include "usbdevice.h"
#include "cvclass.h"
#include "cvmemblock.h"
#include "cvinterface.h"
#ifdef USH_BOOTROM  /*AAI */
#include "volatile_mem.h"
#endif
#include "console.h"
#include "ushx_api.h"
#include "phx_osl.h"

#ifdef LYNX_C_MEM_MODE
#include "idc_cpuapi.h"
#endif

#include "stub.h"

#ifdef USH_BOOTROM  /*AAI */
//#define pcvclass_mem_block ((tCVCLASS_MEM_BLOCK *)VOLATILE_MEM_PTR->cvclass_mem_block)
#else
extern tCVCLASS_MEM_BLOCK cvclass_mem_block;
#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)&cvclass_mem_block)
#endif

#ifdef USH_BOOTROM /*AAI */
#include <toolchain/gcc.h>
#endif

#include "stub.h"

#define CVClassInterface (pcvclass_mem_block->CVClassInterface)
#define wRcvLength    ( pcvclass_mem_block->wRcvLength)
#define bcmdRcvd (pcvclass_mem_block->bcmdRcvd)
#define bcvCmdInProgress (pcvclass_mem_block->bcvCmdInProgress)

// CV has data in TxBuffer to send to the host
cv_status usbCvBulkIn(cv_encap_command *cmd, uint32_t len)
{
	sendEndpointData(CVClassInterface.BulkInEp, (uint8_t *)cmd, len);
	
	/* Prepare endpoint for the next command */
	//prepareRcvDataEndpoint(CV_BULKOUT, CVClassRxBuffer,64);
	return CV_SUCCESS;

}

// Prepares RX Endpoint to receive a new BulkOut message and terminate CV command processing
cv_status usbCvCommandComplete(uint8_t *pBuffer)
{
	bcvCmdInProgress = FALSE;
	bcmdRcvd = FALSE;
	pcvclass_mem_block->command_complete_called = 1;
	recvEndpointData(CVClassInterface.BulkOutEp, pBuffer,
			CVClassInterface.BulkOutEp->maxPktSize);
	return CV_SUCCESS;
}
	
// CV receives data in RxBuffer
cv_status usbCvBulkOut(cv_encap_command *cmd)
{	
	bcmdRcvd = FALSE;
	recvEndpointData(CVClassInterface.BulkOutEp, (uint8_t *)cmd,
			CVClassInterface.BulkOutEp->maxPktSize);
	return CV_SUCCESS;
}

/*
 * Helper routines for checking the status of the CV endpoints
 */
cv_bool usbCvBulkOutComplete(int ticks)
{
	return usbTransferComplete(CVClassInterface.BulkOutEp, ticks);
}

cv_bool usbCvBulkInComplete(int ticks)
{
	cv_bool status = 0;
	uint32_t startTime;
#ifndef USH_BOOTROM  /* SBI */
	ARG_UNUSED(status);
	ARG_UNUSED(startTime);
	return usbTransferComplete(CVClassInterface.BulkInEp, (ticks));
#else
	get_ms_ticks(&startTime);
	do {
		status = usbTransferComplete(CVClassInterface.BulkInEp, (ticks/1000));
		if (!status) {
			//printf("yielding ... %u \n", cvGetDeltaTime(startTime) );
			PROCESS_YIELD_CMDS();
		} else {
			break;
		}
	} while ((cvGetDeltaTime(startTime) < ticks) && canUsbWait());

	if (!status) {
		printf("cvusbd: CvBulkIn timeout (%d)\n", ticks);
		usbd_dump_endpoint(CVClassInterface.BulkInEp);
	}

	return status;
#endif
}

cv_bool usbCvIntrInComplete(int ticks)
{
#ifndef USH_BOOTROM  /* SBI */
	return usbTransferComplete(CVClassInterface.IntrInEp, (ticks));
#else
	cv_bool status = 0;
	uint32_t startTime;

	get_ms_ticks(&startTime);
	do {
		status = usbTransferComplete(CVClassInterface.IntrInEp, (ticks/1000));
		if (!status) {
			//printf("yielding... %u \n", cvGetDeltaTime(startTime) );
			PROCESS_YIELD_CMDS();
		} else {
			break;
		}
	} while ((cvGetDeltaTime(startTime) < ticks) && canUsbWait());

	if (!status) {
		printf("cvusbd: CvIntrIn timeout (%d)\n", ticks);
		usbd_dump_endpoint(CVClassInterface.IntrInEp);
	}

	return status;
#endif
}

// command comming from host, nonblock function, bulk in 
cv_bool usbCvPollCmdRcvd(void)
{
	return bcmdRcvd;
	//return (CVClassInterface.BulkInEp->avail);
}

unsigned char * usbCvPollCmdPtr(void) 
{
	return &bcmdRcvd;
}

// data from device to host send is done, nonblock function, bulk out
cv_bool usbCvPollCmdSent(void)
{
	return is_avail(CVClassInterface.BulkOutEp);
}

cv_bool usbCvPollIntrSent(void)
{
	return is_avail(CVClassInterface.IntrInEp);
}

// send interrupt in data to host
cv_status usbCvInterruptIn(cv_usb_interrupt *interrupt)
{
	sendEndpointData(CVClassInterface.IntrInEp, (uint8_t *)interrupt, sizeof(cv_usb_interrupt));
	return CV_SUCCESS;
}

/*
 * Queue up a BulkIn, send an IntrIn to let the host know it's available,
 * and wait up to 'ticks' ticks for the command to complete.  Returns
 * command completion status.
 */
cv_bool usbCvSend(cv_usb_interrupt *interrupt, uint8_t *pBuffer, uint32_t len, int ticks)
{
	cv_bool result;

#ifdef CV_TRACE
	printf("usbCvSend: %d %d bytes @ %p\n", *(int *)interrupt, len, pBuffer);
#endif
	usbd_cancel(CVClassInterface.BulkInEp);
	usbd_cancel(CVClassInterface.IntrInEp);

#ifdef  LYNX_C_MEM_MODE
    if( USH_IS_CACHED_ADDR( interrupt) )
    {
        printf("usbcvinterface: interrupt %p\n", interrupt);
        /* flush cache */
        phx_idc_cache_wrback (IDC_TARGET_DCACHE);
        /* convert address */
        interrupt = USH_ADDR_2_UNCACHED(interrupt);
    }
#endif

	/*
	 * Work around Dell bios bug by incrementing size if it's an
	 * exact multiple of the max pipe size.
	 */
	if (!(len % CVClassInterface.BulkInEp->maxPktSize)) {
		++len;
		interrupt->resultLen = len;
		printf("usbCvSend: BIOS workaround #2\n");
	}

	ENTER_CRITICAL();
	usbd_rxdma_disable();
	EXIT_CRITICAL();


	sendEndpointData(CVClassInterface.IntrInEp, (uint8_t *)interrupt, sizeof(cv_usb_interrupt));
	result = usbCvIntrInComplete(ticks);
	if (!result) {
		usbd_cancel(CVClassInterface.IntrInEp);
		post_log("usbCvSend: IntrIn timeout\n");
		cvclass_dump_cmd((uint8_t *)interrupt, sizeof(cv_usb_interrupt));
		return 0;
	}
	sendEndpointData(CVClassInterface.BulkInEp, pBuffer, len);
	result = usbCvBulkInComplete(ticks);
	if (!result) {
		usbd_cancel(CVClassInterface.BulkInEp);
		post_log("usbCvSend: BulkIn timeout\n");
		cvclass_dump_cmd(pBuffer, len);
	}
	return result;
}
