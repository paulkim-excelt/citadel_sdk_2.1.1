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

#include "cvclass.h"
#include <usbregisters.h>
#include <cv/cvapi.h>
#include <cv/cvinternal.h>
#include <usbdevice.h>
#include <usbbootinterface.h>
#include <usbd_if.h>
#include <string.h>

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

#define USE_CVCALLBACK

#define CV_CMD_STATE  0
#define CV_DAT_STATE  1
#define CV_EXE_STATE  2

#define cvclass_mem_block ((tCVCLASS_MEM_BLOCK *)IPROC_BTROM_GET_USB_cvclass_mem_block())
#define usb_device_ctx ((usb_device_ctx_t *)IPROC_BTROM_GET_USB_usb_device_ctx_t())
#define epInterfaceBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceBuffer())


#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)cvclass_mem_block)

#define CVClassInterface (pcvclass_mem_block->CVClassInterface)
#define wRcvLength    (pcvclass_mem_block->wRcvLength)
#define bRcvStatus (pcvclass_mem_block->bRcvStatus)
#define dwDataLen (pcvclass_mem_block->dwDataLen)
#define dwlength (pcvclass_mem_block->dwlength)
#define bcmdRcvd (pcvclass_mem_block->bcmdRcvd)
#define bcvCmdInProgress (pcvclass_mem_block->bcvCmdInProgress)
#define wflags    (pcvclass_mem_block->wflags)

extern void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
extern EPSTATUS *usbd_find_endpoint( int interface, int flags );
extern void cvCmdReply(void);

// Receive wlen bytes data, then invoke callback
void usbCvBulkOutSetUp(uint16_t wlen)
{	
	recvEndpointData(CVClassInterface.BulkOutEp, epInterfaceBuffer, wlen);
}

void InitialCvClass(uint8_t *pbuffer, uint16_t wlen)
{
	//bcvCmdInProgress = FALSE;
	/* Set initial state to CMD_STATE */
	//bRcvStatus = CV_CMD_STATE;
	recvEndpointData(CVClassInterface.BulkOutEp, pbuffer, wlen);
}

/*
 * Called from the main task thread by USBD after a reset has been detected
 * and the HW & endpoint data structures have been reinitialized.
 * Reset any machnine state necessary, and initialize the endpoints as needed.
 */
void
cv_usbd_reset(void)
{
	DEBUG_LOG("[%s():%d],begin\n",__func__,__LINE__);
	memset((void *)&CVClassInterface, 0, sizeof(tCVINTERFACE));
	/* acquire endpoint handles */
	CVClassInterface.BulkInEp =  usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | IN_DIR);
	CVClassInterface.BulkOutEp = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | OUT_DIR);
	CVClassInterface.IntrInEp =  usbd_find_endpoint(USBD_CV_INTERFACE, INTR_TYPE | IN_DIR);


	CVClassInterface.BulkOutEp->appCallback = CvClassCallBackHandler;
	/* initialize the cv class after setting up the bulk endpoints */
	InitialCvClass(epInterfaceBuffer, CVClassInterface.BulkOutEp->maxPktSize);
	DEBUG_LOG("[%s():%d],end\n",__func__,__LINE__);
}


void CvClassCallBackHandler(EPSTATUS *ep)
{
	int len =ep->dma.transferred ;

	//DEBUG_LOG("[%s():%d],rcv dataLen=%d\n",__func__,__LINE__,ep->dma.transferred);
	usb_device_ctx->cur_read_offset = (uint32_t)epInterfaceBuffer;
	usb_device_ctx->total_rem_len = len;

}
