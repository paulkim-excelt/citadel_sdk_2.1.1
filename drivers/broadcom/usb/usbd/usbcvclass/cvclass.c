/******************************************************************************
 *
 * Copyright 2007
 * Broadcom Corporation
 * 16215 Alton Parkway
 * PO Box 57013
 * Irvine CA 92619-7013
 *
 *****************************************************************************/
/*
 * Broadcom Corporation
 *
 *
 */

#include <string.h>

#include "usbdevice.h"
#include "usbregisters.h"

#include "cvclass.h"

#include "stub.h"

extern tCVINTERFACE CVClassInterface;

extern uint16_t wcount;

extern char CVClassRxBuffer[];
extern char CVClassTxBuffer[];
extern char CVClassNotify[];

#define HOST_CONTROL_REQUEST_TIMEOUT (60*1000) /* milliseconds */

static void sendBulkInData(uint8_t *pBuffer, uint16_t len)
{
	sendEndpointData((CVClassInterface.BulkInEp), pBuffer, len);
};

static void sendIntrInData(uint8_t *pBuffer, uint16_t len)
{
	sendEndpointData((CVClassInterface.IntrInEp), pBuffer, len);
};

static void prepareBulkOutRcvData(uint8_t *pBuffer, uint16_t len)
{
	recvEndpointData((CVClassInterface.BulkOutEp), pBuffer, len);
}

void InitialCvClass(uint8_t *pBuffer, uint16_t wlen)
{
	prepareBulkOutRcvData((uint8_t *)pBuffer, wlen);
}

void cv_eventCallback(EPSTATUS *ep, usbd_event_t event)
{
#ifdef USH_BOOTROM  /*AAI */
	ARG_UNUSED(ep);
	ARG_UNUSED(event);
#endif
}


void cv_usbd_reset(void)
{
	printf("cv_usbd_reset\n");

	memset(&CVClassInterface, 0, sizeof(CVClassInterface));

	/* acquire endpoint handles */
	CVClassInterface.BulkInEp =  usbd_find_endpoint(USBD_CV_INTERFACE,
													(BULK_TYPE | IN_DIR));
	CVClassInterface.BulkOutEp = usbd_find_endpoint(USBD_CV_INTERFACE,
													(BULK_TYPE | OUT_DIR));
	CVClassInterface.IntrInEp =  usbd_find_endpoint(USBD_CV_INTERFACE,
													(INTR_TYPE | IN_DIR));

	/*Register the callbacks */
	CVClassInterface.BulkOutEp->appCallback = CvClassCallBackHandler;
	CVClassInterface.BulkOutEp->eventCallback = cv_eventCallback;

	/* Prep the Bulk OUT to receive command from DH*/
	InitialCvClass((uint8_t *)CVClassRxBuffer,
					CVClassInterface.BulkOutEp->maxPktSize);
}


/* Dummy CV call back handler sending back 1K data over BULK
 * In ep on receiving CV command
 */
void CvClassCallBackHandler(EPSTATUS *ep)
{
	u32_t rx_len;
	u32_t tx_len;

	tx_len = 1024;
	rx_len = (((u8_t *) ep->dma.pXfr)) - ((u8_t *) ep->dma.pBuffer);

	printk("CvClassCallBackHandler\n");
	printk("ep->number: %d flags: %x rx_len: %d\n",
			 ep->number, ep->flags, rx_len);

	/* Clear the BULK OUT Ep buffer */
	memset((void *)&CVClassTxBuffer[0x31], 0xFF, (tx_len - 0x30));

	CVClassTxBuffer[0] = 0x01;
	CVClassTxBuffer[1] = 0x00;
	CVClassTxBuffer[2] = 0x00;
	CVClassTxBuffer[3] = 0x00;

	CVClassTxBuffer[4] = (tx_len << 8) & 0xFF;
	CVClassTxBuffer[5] = (tx_len >> 8) & 0xFF;

	CVClassTxBuffer[6] = 0x00;
	CVClassTxBuffer[7] = 0x00;
	CVClassTxBuffer[8] = 0x39;
	CVClassTxBuffer[9] = 0x00;
	CVClassTxBuffer[10] = 0x40;
	CVClassTxBuffer[11] = 0x01;
	CVClassTxBuffer[12] = 0x02;
	CVClassTxBuffer[0x30] = 0x68;

	/* Clear the INTR IN Ep buffer */
	memset((uint8_t *)CVClassNotify, 0, USBD_INTR_MAX_PKT_SIZE);

	/*Response Length */
	CVClassNotify[4] = (tx_len << 8) & 0xFF;
	CVClassNotify[5] = (tx_len >> 8) & 0xFF;

	/* Send the notification fixed size of 8 bytes*/
	sendIntrInData((uint8_t *)CVClassNotify, 0x08);
	if (!usbTransferComplete(CVClassInterface.IntrInEp,
							 HOST_CONTROL_REQUEST_TIMEOUT))
		trace("sendIntrInData failed \n");

	/* Send response back */
	sendBulkInData((uint8_t *)CVClassTxBuffer, tx_len);
	if (!usbTransferComplete(CVClassInterface.BulkInEp,
							 HOST_CONTROL_REQUEST_TIMEOUT))
		trace("sendBulkInData failed \n");
	printk("ep->number: %d flags: %x tx_len: %d \n",
			 (CVClassInterface.BulkInEp->number),
			 (CVClassInterface.BulkInEp->flags),
			 tx_len);

	/* Prep the Bulk OUT to receive command from DH*/
	prepareBulkOutRcvData((uint8_t *)CVClassRxBuffer, USBD_BULK_MAX_PKT_SIZE);

	printk("CvClassCallBackHandler Done\n");

}




