/*
 * $Copyright Broadcom Corporation$
 *
 */
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

#include "cv/cvapi.h"
#include "cv/cvinternal.h"
#include "cvclass.h"
#include "cvinterface.h"
#include "usbd_if.h"

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif


#define epInterfaceSendBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceSendBuffer())

#define cvclass_mem_block ((tCVCLASS_MEM_BLOCK *)IPROC_BTROM_GET_USB_cvclass_mem_block())

#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)cvclass_mem_block)
#define CVClassInterface (pcvclass_mem_block->CVClassInterface)
#define wRcvLength    ( pcvclass_mem_block->wRcvLength)
#define bcmdRcvd (pcvclass_mem_block->bcmdRcvd)
#define bcvCmdInProgress (pcvclass_mem_block->bcvCmdInProgress)

extern void sendEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
extern void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);

bool usbCvBulkIn(uint8_t *pBuffer, uint32_t len)
{
	//DEBUG_LOG("[%s():%d],begin,len=%d\n",__func__,__LINE__,len);

	/*if(len>IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size){
		len = IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size;
	}
	memcpy(epInterfaceSendBuffer,pBuffer,len);
	sendEndpointData(CVClassInterface.BulkInEp, epInterfaceSendBuffer, len);*/
	sendEndpointData(CVClassInterface.BulkInEp, pBuffer, len);
	usbd_delay_ms(5); // wait for DMA
	 
	return 1;
}

bool usbCvInterruptIn(uint8_t *pBuffer, uint32_t len)
{
	//DEBUG_LOG("[%s():%d],begin,len=%d\n",__func__,__LINE__,len);	
	
	/*if(len>IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size){
		len = IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size;
	}
	memcpy(epInterfaceSendBuffer,pBuffer,len);
	sendEndpointData(CVClassInterface.IntrInEp, epInterfaceSendBuffer, len);*/
	sendEndpointData(CVClassInterface.IntrInEp, pBuffer, len);
	usbd_delay_ms(5); // wait for DMA
	 
	return 1;
}

// Prepares RX Endpoint to receive a new BulkOut message
cv_status usbCvPrepareRxEpt(uint8_t *pBuffer)
{
	bcmdRcvd = FALSE;
	recvEndpointData(CVClassInterface.BulkOutEp, pBuffer, 64);

	return CV_SUCCESS;

}

// Prepares RX Endpoint to receive a new BulkOut message and terminate CV command processing
cv_status usbCvCommandComplete(uint8_t *pBuffer)
{
	bcvCmdInProgress = FALSE;
	bcmdRcvd = FALSE;
	recvEndpointData(CVClassInterface.BulkOutEp, pBuffer, 64);

	return CV_SUCCESS;
}
	
// CV receives data in RxBuffer
cv_status usbCvBulkOut(cv_encap_command *cmd)
{	
	bcmdRcvd = FALSE;
	recvEndpointData(CVClassInterface.BulkOutEp, (uint8_t *)cmd, sizeof(cv_encap_command));

	return CV_SUCCESS;
}


/*
 * Helper routines for checking the status of the CV endpoints
 */
bool usbCvBulkOutComplete(int ticks)
{
	return 1;
}

bool usbCvBulkInComplete(int ticks)
{
	return 1;
}

bool usbCvIntrInComplete(int ticks)
{
	return 1;
}

// command comming from host, nonblock function, bulk in 
bool usbCvPollCmdRcvd(void)
{
	return bcmdRcvd;
}

// data from device to host send is done, nonblock function, bulk out
bool usbCvPollCmdSent(void)
{
	return 0;
}

bool usbCvPollIntrSent(void)
{
	return 0;
}

/*
 * Queue up a BulkIn, send an IntrIn to let the host know it's available,
 * and wait up to 'ticks' ticks for the command to complete.  Returns
 * command completion status.
 */
bool usbCvSend(cv_usb_interrupt *interrupt, uint8_t *pBuffer, uint32_t len, int ticks)
{
	sendEndpointData(CVClassInterface.IntrInEp, (uint8_t *)interrupt, sizeof(cv_usb_interrupt));
	if (!usbCvIntrInComplete(ticks))
		return 0;
	sendEndpointData(CVClassInterface.BulkInEp, pBuffer, len);

	return usbCvBulkInComplete(ticks);
}


