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

#include "cvmain.h"
#include "nfpclass.h"
#include "usbdevice.h"
#include "usbregisters.h"
#include "nfpmemblock.h"
#include "console.h"
#include "volatile_mem.h"
#include "cvapi.h"
#include "cvinternal.h"
#include "sched_cmd.h"
#include "phx_upgrade.h"
#include "nci.h"

#define NFP_STRING_IDX		8

#define NFPClassInterface		(pnfpclass_mem_block->NFPClassInterface)
#define bPktInProgress			(pnfpclass_mem_block->bPktInProgress)
#define dwTotalLen				(pnfpclass_mem_block->dwTotalLen)
#define bytesReceived			(pnfpclass_mem_block->bytesReceived)
#define rxBuffer				(pnfpclass_mem_block->nfpbuffer)
#define rxbIndex				(pnfpclass_mem_block->rxIndex)
#define pktBeingProcessed		(pnfpclass_mem_block->pktBeingProcessed)
#define nfpRxbuffer				(pnfpclass_mem_block->nfpRxbuffer)


void
nfpclass_dump_cmd(uint8_t *buf, int len)
{
	printf("NFC: dump (%d bytes) \n", len);
	dumpHex(buf,len);
}


void NFPClassCallBackHandler(EPSTATUS *ep)
{
	unsigned int xTaskWoken = FALSE;
	nfpPacketHeader *nfpPacket;
	unsigned int length= 0;
	
	recvEndpointData(ep,pnfpclass_mem_block->nfpbuffer, ep->maxPktSize);
	
	nfpPacket = (nfpPacketHeader *)rxBuffer;

	if(bPktInProgress == FALSE)
	{
		// printf("%s:nfpPacket->bPayloadLen =%d and pktQueueIndex = %d \n",__FUNCTION__,nfpPacket->bPayloadLen,pnfpclass_mem_block->pktQueueIndex);	
		dwTotalLen = nfpPacket->bPayloadLen + 4;
		if (dwTotalLen > NFP_RX_TX_BUFFER_SIZE)
		{
			printf("Packet too large. Len: %d\n", dwTotalLen);
			return;
		}

		/* Check to see if Queue has empty slot */
		ENTER_CRITICAL();
		if(pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktQueueIndex] != 0)
		{
			EXIT_CRITICAL();
			printf("%s: Error NFC Queue is full, dropping the packet \n",__FUNCTION__);
			return;
		}
		EXIT_CRITICAL();
		
		bPktInProgress = TRUE;
		bytesReceived = 0;
		rxbIndex = 0;
	}

	if( (dwTotalLen - bytesReceived) > CV_USB_MAXPACKETSIZE)
		length = CV_USB_MAXPACKETSIZE;
	else
		length = dwTotalLen - bytesReceived;

	bytesReceived += length;

	memcpy((&nfpRxbuffer[pnfpclass_mem_block->pktQueueIndex][rxbIndex]), pnfpclass_mem_block->nfpbuffer, length);
	rxbIndex += length;

	if(dwTotalLen == bytesReceived)
	{
		/* Move the Queue Counter */
		ENTER_CRITICAL();
		pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktQueueIndex] = dwTotalLen;
		EXIT_CRITICAL();

		/* Schedule task to send response */
		xTaskWoken = queue_nfc_cmd(SCHED_NFCUSB_RESP, xTaskWoken);
		
		// Turn SPI int off
		nci_disable_isr();
		
		/* Increment the queue index and initialize the buffer parameters */
		pnfpclass_mem_block->pktQueueIndex = (pnfpclass_mem_block->pktQueueIndex+1)%NFP_RX_QUEUE_LEN;
		bPktInProgress = FALSE;
		rxbIndex = 0;
		bytesReceived = 0;
		
	}
	return;		
}

void nfp_eventCallback(EPSTATUS *ep, usbd_event_t event)
{
	printf("NFC ep%d-%s event %d\n", ep->number, is_in(ep) ? "in" : "out", event);
}

void InitialNFPClass(uint8_t *pbuffer, uint16_t wlen)
{
	printf("%s: Init the NFP Class \n",__FUNCTION__);
	bPktInProgress = FALSE;
	pnfpclass_mem_block->bNCIResponseAvailable = 0;
	rxbIndex = 0;

	/* cleanup the packet queue */
	nfc_cleanup_pkt_queue();
	
	/*printf("SNCIResp %d\n", pnfpclass_mem_block->bNCIResponseAvailable);*/

	//if (NFPClassInterface.BulkOutEp)
	recvEndpointData(NFPClassInterface.BulkOutEp, pbuffer, wlen);
	
	/* Enable NFC timer to check for SPI responses */
//	disable_low_res_timer_int(cvNFCTimerISR); 			/* disable first in case there's already a timer enabled */
//	enable_low_res_timer_int(100, cvNFCTimerISR);	
}

/*
 * Called from the main task thread by USBD after a reset has been detected
 * and the HW & endpoint data structures have been reinitialized.
 * Reset any machnine state necessary, and initialize the endpoints as needed.
 */
void
nfp_usbd_reset(void)
{
	printf("nfp:rst\n");
	memset(&NFPClassInterface, 0, sizeof(NFPClassInterface));
	
	/* acquire endpoint handles */
	NFPClassInterface.BulkInEp =  usbd_find_endpoint(USBD_NFP_INTERFACE, BULK_TYPE | IN_DIR);
	NFPClassInterface.BulkOutEp = usbd_find_endpoint(USBD_NFP_INTERFACE, BULK_TYPE | OUT_DIR);
	NFPClassInterface.IntrInEp =  usbd_find_endpoint(USBD_NFP_INTERFACE, INTR_TYPE | IN_DIR);
	NFPClassInterface.BulkOutEp->appCallback = NFPClassCallBackHandler;
	//NFPClassInterface.BulkOutEp->eventCallback = nfp_eventCallback;

	/* initialize the nfp class after setting up the bulk endpoints */
	InitialNFPClass(pnfpclass_mem_block->nfpbuffer, NFPClassInterface.BulkOutEp->maxPktSize);
}
