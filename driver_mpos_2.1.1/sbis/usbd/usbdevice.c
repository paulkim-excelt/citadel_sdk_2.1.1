/*
 * $Copyright Broadcom Corporation$
 *
 */
/*******************************************************************
 *
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

#include "usbdevice_internal.h"
#include "usbdebug.h"
#include "usbregisters.h"
#include "usb.h"
#include <string.h>

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

/* Changes for CV_USBBOOT */


/* Maximum per DMA descriptor */
#define MAX_USB_DMA_TX	(USB_DMA_DESC_TX_BYTES & ~(64-1))

void usbd_dump_endpoint(EPSTATUS *ep)
{
	volatile tUSBDMA_DATA_DESC *pDesc;
	uint32_t control;
	uint32_t status;
	char *dir;

	if (is_in(ep)) {
		control = *USB_UDCAHB_IN_ENDP_CONTROL(ep->number);
		status = *USB_UDCAHB_IN_ENDP_STATUS(ep->number);
		pDesc = (tUSBDMA_DATA_DESC *)*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number);
		dir = "in";
	}
	else {
		control = *USB_UDCAHB_OUT_ENDP_CONTROL(ep->number);
		status = *USB_UDCAHB_OUT_ENDP_STATUS(ep->number);
		pDesc = (tUSBDMA_DATA_DESC *)*USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number);
		dir = "out";
	}
	trace("ep%d %s @ 0x%x state=%d\n",
		ep->number, dir, ep, ep->state);
	trace("\t%s control  = 0x%08x", dir, control);
	if (control & USB_ENDP_CONTROL_STALL)
		trace(" STALL");
	if (control & USB_ENDP_CONTROL_FLUSH_TXFIFO)
		trace(" FLUSH-TXFIFO");
	if (control & USB_ENDP_CONTROL_SNOOP)
		trace(" SNOOP");
	if (control & USB_ENDP_CONTROL_POLL_DEMAND)
		trace(" POLL");
	if (control & USB_ENDP_CONTROL_NAK)
		trace(" NAK");
	trace("\n");
	trace("\t%s status   = 0x%08x", dir, status);
	if (status & USB_ENDP_STATUS_IN)
		trace(" IN");
	if (status & USB_ENDP_STATUS_BUF_NOTAVAIL)
		trace(" BNA");
	if (status & USB_ENDP_STATUS_AHB_ERR)
		trace(" AHB");
	if (status & USB_ENDP_STATUS_TX_DMA_DONE)
		trace(" TDC");
	trace("\n");
	if (is_in(ep)) {
		trace("\tin bufsize  = 0x%08x\n", *USB_UDCAHB_IN_ENDP_BUFSIZE(ep->number));
		trace("\tin maxpkt   = 0x%08x\n", *USB_UDCAHB_IN_ENDP_MAXPKTSIZE(ep->number));
		trace("\tin desc     = 0x%08x\n", pDesc);
		if (*USB_UDCAHB_ENDP_INTMASK & USB_DEVICE_ENDP_INT_IN(ep->number))
			trace("\tinterrupt masked\n");
		else
			trace("\tinterrupt enabled\n");
	}
	else {
		trace("\tout frame#   = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_FRAMENUMBER(ep->number));
		trace("\tout maxpkt   = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(ep->number));
		trace("\tout setupbf  = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number));
		trace("\tout desc     = 0x%08x\n", pDesc);
		if (*USB_UDCAHB_ENDP_INTMASK & USB_DEVICE_ENDP_INT_OUT(ep->number))
			trace("\tinterrupt masked\n");
		else
			trace("\tinterrupt enabled\n");
	}
	trace("\t\tlen = %d xfrd = %d\n",
		ep->dma.len, ep->dma.transferred);
	trace("\t\tdwStatus   = 0x%08x\n", pDesc->dwStatus);
	trace("\t\tdwReserved = 0x%08x\n", pDesc->dwReserved);
	trace("\t\tpBuffer    = 0x%08x\n", pDesc->pBuffer);
	trace("\t\tpNext      = 0x%08x\n", pDesc->pNext);
}

void
usbd_dump_control_status()
{
	uint32_t control;
	uint32_t status;

	control = *USB_UDCAHB_DEVICE_CONTROL;
	trace("*USB_UDCAHB_DEVICE_CONTROL = 0x%08x", control);
	if (control & USB_DEVICE_CONTROL_RESUME)
		trace(" RESUME");
	if (control & USB_DEVICE_CONTROL_RXDMA_EN)
		trace(" RXDMA");
	if (control & USB_DEVICE_CONTROL_TXDMA_EN)
		trace(" TXDMA");
	trace("\n");
	status = *USB_UDCAHB_DEVICE_STATUS;
	trace("*USB_UDCAHB_DEVICE_STATUS  = 0x%08x", status);
	if (status & USB_DEVICE_STATUS_RXFIFO_EMPTY)
		trace(" RXFIFO-EMPTY");
	trace("\n");
	trace("*USB_UDCAHB_ENDP_INT       = 0x%08x\n", *USB_UDCAHB_ENDP_INT);
	trace("*USB_UDCAHB_DEVICE_INT     = 0x%08x\n", *USB_UDCAHB_DEVICE_INT);
}

void
usbd_dump()
{
	int i;

	for (i = 0; i < USBDevice.num_endpoints; i++) {
		usbd_dump_endpoint(&USBDevice.ep[i]);
	}
	usbd_dump_control_status();
}

/*
 * Return true if last endpoint request has completed, false otherwise
 *
 * If ticks==0, return the status immediately
 *
 * If ticks!=0, wait up to 'ticks' for the condition to become true, then return
 * the condition.
 */
bool usbTransferComplete(EPSTATUS *ep, int ticks)
{
	uint32_t startTime;
	bool result;
	
	volatile uint32_t count = 0x10000;

	/* Check state */
	if (!isUsbdEnumDone()) {
		trace("usb is not enum done\n");
		result = is_avail(ep);
		goto error_ret;
	}

#if 1 //def CV_USBBOOT
#define CYGNUS_BLOCKED_DELAY_TIMER_CV_USBBOOT	1
#define CYGNUS_TIMER_MODE_FREERUN				0

    while((count--) && (!is_avail(ep))) {
		usbdevice_isr();
	}
#else
	if (ticks) {
		get_ms_ticks(&startTime);
		do {
			if (is_avail(ep))
				break;
			usbd_check_endpoint(ep);
			/* Check state */
			if (!isUsbdEnumDone()) {
				result = is_avail(ep);
				goto error_ret;
			}
			if (smbus_poll_data()) {
				result = is_avail(ep);
				goto error_ret;
			}
			if (ISSET_SYSTEM_STATE(CUR_SYS_STATE_LOW_PWR)) {
				result = is_avail(ep);
				goto error_ret;
			}

		} while (cvGetDeltaTime(startTime) < ticks);
	}
#endif

	result = is_avail(ep);
#if 1
	if (!result) {
		usbd_dump_endpoint(ep);
	}
#endif
error_ret:
	return result;
}

void usbd_cancel(EPSTATUS *ep)
{
	if (is_in(ep)) {
		switch (ep->state) {
		case USBD_EPS_IDLE:
			trace("IN transfer canceled (idle state)\n");
			break;

		case USBD_EPS_PENDING:
#ifdef USBD_USE_IN_ENDPOINT_INTMASK
			/* disable endpoint interrupt here */
			*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
			ep_set_idle(ep);
			/* clear out the old transfer info */
			ep->dma.len = 0;
			ep->dma.transferred = 0;

			trace("IN transfer canceled (pending state)\n");
			usbd_dump_endpoint(ep);
			break;

		case USBD_EPS_ACTIVE:
#ifdef USBD_USE_IN_ENDPOINT_INTMASK
			/* disable endpoint interrupt here */
			*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
			ep_set_idle(ep);
			/* Flush the TX FIFO, take back control of the endpoint */
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_FLUSH_TXFIFO;
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) &= ~USB_ENDP_CONTROL_POLL_DEMAND;
			ep->dma.dma.dwStatus = USB_DMA_DESC_BUFSTAT_DMA_DONE | USB_DMA_DESC_LAST;
			/* Flush the TX FIFO, take back control of the endpoint */
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_FLUSH_TXFIFO;
			/* clear out the old transfer info */
			ep->dma.len = 0;
			ep->dma.transferred = 0;

			trace("IN transfer canceled (active state)\n");
			usbd_dump_endpoint(ep);
			break;
		}
	}
}

uint32_t usbd_advance_pointers(EPSTATUS *ep)
{
	uint32_t len;
	usb_dma_info_t *pDmaInfo = &ep->dma;
	volatile tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

	if (is_in(ep))
		len = pDesc->dwStatus & USB_DMA_DESC_TX_BYTES;
	else
		len = pDesc->dwStatus & USB_DMA_DESC_RX_BYTES;

	pDmaInfo->transferred += len;
	pDmaInfo->pXfr += len;
	return len;
}

void usbd_set_nak(EPSTATUS *ep)
{
	volatile uint32_t *endp_control;

	if (is_in(ep))
		endp_control = USB_UDCAHB_IN_ENDP_CONTROL(ep->number);
	else
		endp_control = USB_UDCAHB_OUT_ENDP_CONTROL(ep->number);
	*endp_control |= USB_ENDP_CONTROL_SET_NAK;
}

void usbd_clear_nak(EPSTATUS *ep)
{
	volatile uint32_t *endp_control;

	if (is_in(ep))
		endp_control = USB_UDCAHB_IN_ENDP_CONTROL(ep->number);
	else
		endp_control = USB_UDCAHB_OUT_ENDP_CONTROL(ep->number);
	*endp_control |= USB_ENDP_CONTROL_CLEAR_NAK;
}

/*
 * Re-enable RX dma, probably after receiving and processing an OUT packet.
 *
 * Only enable if
 *  1) Not in interrupt processing
 *  2) All non-control endpoints are ready to receive
 */
void usbd_rxdma_enable()
{
	int i;

	if (USBDevice.flags & USBD_FLAG_INTERRUPT) {
		return;
	}

	if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
		return;
	}

	for (i = 0; i < USBDevice.num_endpoints; i++) {
		EPSTATUS *ep = &USBDevice.ep[i];

		if (is_out(ep) && !is_ctrl(ep)) {
			volatile tUSBDMA_DATA_DESC *pDesc = &ep->dma.dma;
			uint32_t status = pDesc->dwStatus & USB_DMA_DESC_BUFSTAT_MASK;

#ifdef USBD_RXDMA_IFF_ALL_OUT_ENDPOINTS_READY
			if (status != USB_DMA_DESC_BUFSTAT_HOST_READY) {
				/* An OUT endpoint isn't active, don't enable RXDMA */
				return;
			}
#endif
			if (status == USB_DMA_DESC_BUFSTAT_HOST_READY) {
				usbd_clear_nak(ep);
			}
		}
	}
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_RXDMA_EN;
}

void usbd_rxdma_disable()
{
	if (!(*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN))
		return;

	*USB_UDCAHB_DEVICE_CONTROL &= ~USB_DEVICE_CONTROL_RXDMA_EN;
}

/*
 * Transmit side code
 */
uint32_t set_txdata(EPSTATUS *ep)
{
	uint32_t txsize;
	usb_dma_info_t *pDmaInfo = &ep->dma;
	volatile tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

	txsize = pDmaInfo->len - pDmaInfo->transferred;
	if (txsize > MAX_USB_DMA_TX)
		txsize = MAX_USB_DMA_TX;
	pDmaInfo->desc_len = txsize;
	pDesc->pBuffer = pDmaInfo->pXfr;
#ifdef USBD_USE_IN_PENDING_STATE
	pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_HOST_BUSY
				| USB_DMA_DESC_LAST
				| (txsize & USB_DMA_DESC_TX_BYTES);
#else
	pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_HOST_READY
				| USB_DMA_DESC_LAST
				| (txsize & USB_DMA_DESC_TX_BYTES);
#endif
	// Clear any left-over error status from before there was
	// anything to transmit.
	*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;
	*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_BUF_NOTAVAIL;
#ifdef USBD_USE_IN_PENDING_STATE
	ep_set_pending(ep);
#else
	ep_set_active(ep);
	*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_POLL_DEMAND;
#endif
	usbd_clear_nak(ep);
	return txsize;
}

void sendEndpointData(EPSTATUS *ep, uint8_t *pbuffer, uint32_t len)
{
	usb_dma_info_t *pDmaInfo = &ep->dma;

	if (!is_in(ep)) {
		trace("ep%d: ERROR cannot send on non-IN endpoint\n", ep->number);
		return;
	}
	trace("sendEndpointData %p %d\n", pbuffer, len);

	if (!is_avail(ep) && !is_ctrl(ep)) {
		/* Stop the transfer in progress */
		usbd_cancel(ep);
	}
	if (!isUsbdEnumDone() && !is_ctrl(ep)) {
		return;
	}

#ifdef USBD_USE_IN_ENDPOINT_INTMASK
	/* enable endpoint interrupt here */
	*USB_UDCAHB_ENDP_INTMASK &= ~USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
	pDmaInfo->pBuffer = pbuffer;
	pDmaInfo->len = len;
	pDmaInfo->transferred = 0;
	pDmaInfo->pXfr = pbuffer;
#ifdef USBD_USE_IN_PENDING_STATE
	ep_set_pending(ep);
#else
	ep_set_active(ep);
#endif
	set_txdata(ep);
}

void sendCtrlEpData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
 	sendEndpointData(ep, pData, len);
}

void sendCtrlEndpointData(uint8_t *pBuffer, uint32_t len)
{
	EPSTATUS *ep = USBDevice.controlIn;

	if (len > sizeof(DescriptorBuffer)) {
		len = sizeof(DescriptorBuffer);
	}
	/* copy to RAM is required, as we can't DMA from ROM */
	memcpy((void*)DescriptorBuffer, (void*)pBuffer, len);
	sendCtrlEpData(ep, DescriptorBuffer, len);
}

void usbdEpInInterrupt(EPSTATUS *ep)
{
	uint32_t status = *USB_UDCAHB_IN_ENDP_STATUS(ep->number);
	volatile tUSBDMA_DATA_DESC *pDesc = (tUSBDMA_DATA_DESC *) *USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number);

	trace("usbdEpInInterrupt\n");
	if (status & USB_ENDP_STATUS_AHB_ERR) {
		trace("IN AHB error\n");
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_AHB_ERR;
		status &= ~USB_ENDP_STATUS_AHB_ERR;
		usbd_cancel(ep);
	}

	if (status & USB_ENDP_STATUS_BUF_NOTAVAIL) {
		trace("IN buf notavail error, avail=%d\n", is_avail(ep));
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_BUF_NOTAVAIL;
		status &= ~USB_ENDP_STATUS_BUF_NOTAVAIL;
	}

	if (status & USB_ENDP_STATUS_TX_DMA_DONE) {
		int success = 1;
		int more = 0;
		char *result = "SUCCESS";
		uint32_t len = ep->dma.dma.dwStatus & USB_DMA_DESC_TX_BYTES;

		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_TX_DMA_DONE;
		status &= ~USB_ENDP_STATUS_TX_DMA_DONE;

		switch (pDesc->dwStatus & USB_DMA_DESC_RXTXSTAT_MASK) {
		case USB_DMA_DESC_RXTXSTAT_SUCCESS:
			break;
		case USB_DMA_DESC_RXTXSTAT_DESERR:
			success = 0;
			result = "DESERR";
			break;
		case USB_DMA_DESC_RXTXSTAT_RESERVED:
			success = 0;
			result = "RESERVED";
			break;
		case USB_DMA_DESC_RXTXSTAT_BUFERR:
			success = 0;
			result = "DBUFERR";
			break;
		}
		if (success) {
			/* Sometimes, the tx-bytes field in the dma descriptor is corrupted
			 * even though all the data has been transmitted. This can mess up
			 * the pointer calculations and other termination tests.
			 * Trust the DMA status, and patch up the length field here.
			 */
			len = ep->dma.desc_len;
			ep->dma.dma.dwStatus = (ep->dma.dma.dwStatus & ~USB_DMA_DESC_TX_BYTES) | len;
			usbd_advance_pointers(ep);
			/* If there's more to transmit,
			 * or if we just transmitted data, and it was a multiple of the maxPktSize
			 * then setup the next TX descriptor
			 */
			if (ep->dma.transferred != ep->dma.len)
				more = 1;
			if (len && !(len % ep->maxPktSize))
				more = 1;
			if (len != ep->dma.desc_len) {
				trace("wrong tx len %d, expected %d\n", len, ep->dma.desc_len);
				more = 0;
				usbd_dump_endpoint(ep);
			}
		}
		else {
			usbd_dump_endpoint(ep);
		}

		/* Now that we've figured out what's happened, and what we'll do next,
		 * log the information to the serial console (if requested).
		 */
		trace("TX DMA-done %s len = %d dwStatus = 0x%08x more = %d\n",
			result, len, pDesc->dwStatus, more);
		if (is_avail(ep))
			trace("TX-DMA done on avail endpoint\n");

		if (more) {
			set_txdata(ep);
		}
		else {
			ep_set_idle(ep);
#ifdef USBD_USE_IN_ENDPOINT_INTMASK
			/* disable endpoint interrupt here */
			*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
		}
		/*
		 * Clear IN, just in case it was queued up.
		 * We don't want to set NAK until the next IN token after DMA complete.
		 */
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_IN;
		status &= ~USB_ENDP_STATUS_IN;
	}
	if (status & USB_ENDP_STATUS_IN) {
		if (ep->eventCallback)
			(*ep->eventCallback)(ep, USBD_EPE_IN_TOKEN);
		switch (ep->state) {
		case USBD_EPS_IDLE:
#if defined(USBD_USE_IN_ENDPOINT_NAK) || defined(USBD_USE_IN_ENDPOINT_INTMASK)
			trace("IN token (idle state)\n");
#endif
#ifdef USBD_USE_IN_ENDPOINT_INTMASK
			/* disable endpoint interrupt here */
			*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
#ifdef USBD_USE_IN_ENDPOINT_NAK
			/* Set NAK here */
			usbd_set_nak(ep);
#endif
			break;

		case USBD_EPS_PENDING:
			trace("IN token (pending state)\n");
			ep->dma.dma.dwStatus = ((ep->dma.dma.dwStatus & ~USB_DMA_DESC_BUFSTAT_MASK)
						| USB_DMA_DESC_BUFSTAT_HOST_READY);
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_POLL_DEMAND;
			ep_set_active(ep);
			break;

		case USBD_EPS_ACTIVE:
			/* This case should not happen */
			trace("IN token on ACTIVE endpoint\n");
#ifdef USBD_USE_IN_ENDPOINT_NAK
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_POLL_DEMAND;
			usbd_clear_nak(ep);
#endif
			break;
		}

		/*
		 * Another IN token may have arrived while we were doing all this,
		 * and we don't want to hear about it, so re-clear the interrupt
		 * status bits now
		 */
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_IN;
		status &= ~USB_ENDP_STATUS_IN;
	}
}

/*
 * Receive side code
 *
 * How much we'll receive depends on the DMA mode we're in.
 * Assume PPBNDU or PPBDU - so up to maxPktSize.
 * The RX DMA descriptor can't limit the amount of data to
 * receive. That's handled by the DMA mode.
 */
void set_rxdata(EPSTATUS *ep)
{
	usb_dma_info_t *pDmaInfo = &ep->dma;
	volatile tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

	if (pDmaInfo->len == pDmaInfo->transferred)
		return;

	// Clear any left-over error status from before there was
	// someplace to receive to.
	*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = ~0;

	pDesc->pBuffer = pDmaInfo->pXfr;
	pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_HOST_READY | USB_DMA_DESC_LAST;
	*USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;
	ep_set_active(ep);
	usbd_clear_nak(ep);
	usbd_rxdma_enable();
}

void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
	usb_dma_info_t *pDmaInfo = &ep->dma;

	if (!is_out(ep)) {
		trace("ERROR cannot recv on non-OUT endpoint\n");
		return;
	}
	trace("recvEndpointData %p %d\n", pData, len);
	if (len > 256*1024) {
		trace(ep, "ERROR bad length: recvEndpointData %p %d\n", pData, len);
	}
	if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
		trace("%s: RXDMA enabled\n", __FUNCTION__);
	} 
#ifdef USBD_USE_OUT_ENDPOINT_INTMASK
	/* enable endpoint interrupt here */
	*USB_UDCAHB_ENDP_INTMASK &= ~USB_DEVICE_ENDP_INT_OUT(ep->number);
#endif
	pDmaInfo->pBuffer = pData;
	pDmaInfo->len = len;
	pDmaInfo->transferred = 0;
	pDmaInfo->pXfr = pData;
	set_rxdata(ep);
}

/*
 * Handle endpoint OUT interrupts.
 *
 * Note that when an OUT interrupt is asserted, the RX DMA engine is automatically
 * disabled by the HW and must be explicitly re-enabled.
 */
void usbdEpOutInterrupt(EPSTATUS *ep)
{
	uint32_t status = *USB_UDCAHB_OUT_ENDP_STATUS(ep->number);
	volatile tUSBDMA_DATA_DESC *pDesc = (tUSBDMA_DATA_DESC *) *USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number);

	if (status & USB_ENDP_STATUS_AHB_ERR) {
		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_AHB_ERR;
		trace("OUT AHB error\n");
		DEBUG_LOG("[%s():%d],OUT AHB error\n",__func__,__LINE__);
		usbd_dump_endpoint(ep);
		if (is_ctrl(ep)) {
			/* Something is not happy, start over */
			USBDevice.flags |= USBD_FLAG_RESTART;
		}
	}
	/*
	 * Buffer not available.
	 * DMA descriptor status is/was either HOST_BUSY or DMA_DONE
	 * when the DMA unit tried to access it. Usually harmless, as
	 * it just means we weren't ready to receive anything at the time.
	 */
	if (status & USB_ENDP_STATUS_BUF_NOTAVAIL) {
		trace("OUT buf notavail error\n");
		DEBUG_LOG("[%s():%d],OUT buf notavail error\n",__func__,__LINE__);
		if (is_ctrl(ep)) {
			/* Something is not happy, start over */
			USBDevice.flags |= USBD_FLAG_RESTART;
		}
		if (ep->eventCallback)
			(*ep->eventCallback)(ep, USBD_EPE_BUF_NOTAVAIL);
		/* recover by re-setting up the receive descriptor */
		set_rxdata(ep);

		return;
	}

	if ((status & USB_ENDP_STATUS_OUT_MASK) == USB_ENDP_STATUS_OUT_SETUP) {
		SetupPacket_t  req;
		tUSBDMA_SETUP_DESC *pSetupDesc = (tUSBDMA_SETUP_DESC *) *USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number);

		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_OUT_SETUP;
		trace("RX setup packet @ %p\n", pSetupDesc);

		// Copy data from SetupBuffer to USBRequest
		memcpy((void*)(&req), (void*)(pSetupDesc->dwData), sizeof(SetupPacket_t));
		// here, the Protocol layer put IN data in System memory but hold it until the host ask it to be
		handleUsbRequest(&req);
		// here we have three possible endpoint states, either:
		// 1. TX :  host requests data
		// 2. RX:   host is going to send data
		// 3. IDLE: host is either send invalid setup up token and is stalled or setup token with no data transaction

		// In case we need to receive anything (even a 0-length packet)
		recvEndpointData(ep, pusb_mem_block->ctrl_buffer, ep->maxPktSize);

		// For debugging
		//memset(pSetupDesc, 0, sizeof(*pSetupDesc));
	}

	//
	// Got an out packet.
	// May be done
	//    call callback routine
	// May be done with descriptor, but not with overall request
	//    queue up next request
	// May not be done with current descriptor
	//    just continue
	//
	if ((status & USB_ENDP_STATUS_OUT_MASK) == USB_ENDP_STATUS_OUT_DATA) {
		int success = 1;
		volatile tUSBDMA_DATA_DESC *pDesc = &ep->dma.dma;

		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_OUT_DATA;
		if ((pDesc->dwStatus & USB_DMA_DESC_BUFSTAT_MASK) == USB_DMA_DESC_BUFSTAT_DMA_DONE) {
			int more;
			uint32_t len;
			char *result = "";

			switch (pDesc->dwStatus & USB_DMA_DESC_RXTXSTAT_MASK) {
			case USB_DMA_DESC_RXTXSTAT_SUCCESS:
				result = "SUCCESS";
				break;
			case USB_DMA_DESC_RXTXSTAT_DESERR:
				/* DMA descriptor status was something other than HOST_READY. */
				result = "DESERR";
				success = 0;
				break;
			case USB_DMA_DESC_RXTXSTAT_RESERVED:
				result = "RESERVED";
				success = 0;
				break;
			case USB_DMA_DESC_RXTXSTAT_BUFERR:
				result = "DBUFERR";
				success = 0;
				break;
			}
			if (!success) {
				trace("RX DMA-Done %s dwStatus=0x%08x status=0x%08x\n",
					result, pDesc->dwStatus, status);
				more = 0;
				usbd_dump_endpoint(ep);
			}
			if (success) {
				// Advance pointers
				len = usbd_advance_pointers(ep);
				trace("RX DMA-Done %s dwStatus=0x%08x status=0x%08x pXfr=%p, len=%d\n",
					result, pDesc->dwStatus, status, ep->dma.pXfr - len, len);

				// See if the DMA engine overran the descriptor
				if (ep->dma.transferred > ep->dma.len) {
					trace("RX too much, len = %d transferred = %d\n",
						ep->dma.len, ep->dma.transferred);
					//replace with the DEBUG_LOG above: ush_panic("usbd: RX too much\n");
				}
				// Decide if there's more to receive.
				more = (ep->dma.transferred < ep->dma.len) && (!len || (len == ep->maxPktSize));
			}
			if (more) {
				// DMA done, but more to transfer.
				// Setup the next descriptor
				set_rxdata(ep);
			}
			else {
				// All done, invoke the callback
				ep_set_idle(ep);
				usbd_set_nak(ep);
#ifdef USBD_USE_OUT_ENDPOINT_INTMASK
				/* disable endpoint interrupt here */
				*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_OUT(ep->number);
#endif
				if (success) {
					trace("RX complete, func=%p len=%d\n", ep->appCallback, ep->dma.transferred);
					if (ep->appCallback != NULL)
						(*ep->appCallback)(ep);
				}
				else {
					trace("RX complete, ERROR\n");
					if (ep->eventCallback)
						(*ep->eventCallback)(ep, USBD_EPE_RXERROR);
				}
			}
		}
	}
}

void stallControlEP(EPSTATUS *ep)
{
	// Set IN and OUT STALL
	// but how to set it, check with hardware / verification engineers
	// before set this bit, the application must check for RXFIFO emptiness, but how?
	*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_STALL;
	// need to purge FIFO ?
	// ready for the next setup transfer

	// synopsys AHB p 210 chapter 13.4  clear OUT NAK bit
	usbd_clear_nak(ep);
	set_rxdata(ep);
}

void usbd_debug()
{
	EPSTATUS *ep;

#ifdef USBD_CV_INTERFACE
	ep = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | IN_DIR);
	usbd_dump_endpoint(ep);
	ep = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | OUT_DIR);
	usbd_dump_endpoint(ep);
#endif
}

void usbd_check_endpoint(EPSTATUS *ep)
{
	volatile int do_dump = 0;

	if (is_ctrl(ep))
		return;
	if (is_out(ep)) {
	 	volatile tUSBDMA_DATA_DESC *pDesc = &ep->dma.dma;
	 	uint32_t status = pDesc->dwStatus & USB_DMA_DESC_BUFSTAT_MASK;

	 	if (status == USB_DMA_DESC_BUFSTAT_HOST_READY) {
	 		if (!is_active(ep)) {
	 			trace("dma buffer host-ready, but endpoint not active\n");
				do_dump = 1;
	 		}
	 		if (*USB_UDCAHB_OUT_ENDP_CONTROL(ep->number) & USB_ENDP_CONTROL_NAK) {
		 		usbd_clear_nak(ep);
	 		}
	 	}
	}
	if (do_dump)
		usbd_dump_endpoint(ep);
}

void
usbd_check_endpoints()
{
	int i;

	/* Check for active endpoints that have NAK set */
	for (i = 0; i < USBDevice.num_endpoints; i++) {
		usbd_check_endpoint(&USBDevice.ep[i]);
	}
}

/* ************************************************
 * Interrupt Service Routine needs to handle different interrupts
 * the priority should be as follows:
 *
 *    1.  RESUME interrupt ---> resume mode
 *    2.  RESET  interrupt ---> reset mode
 *    3.  Control Endpoint interrupt ----> service control endpoint
 *    4.  IN interrupt       -----> service IN endpoint
 *    5.  OUT interrupt      ---> Service OUT endpoint
 *    6.  SUSPEND interrupt  ---> suspend mode
 *
 *************************************************/

void usbdevice_isr(void)
{
	int i;
	uint32_t device_status;
	uint32_t endpoint_status;
	int enable_rxdma = 0;

	USBDevice.flags |= USBD_FLAG_INTERRUPT;

	device_status = *USB_UDCAHB_DEVICE_INT;
	if (device_status) {
		//trace("usbd: isr device_int=0x%08x\n", device_status);
	}
	if (device_status & USB_DEVICE_INT_SET_CONFIG) {
		unsigned int xTaskWoken = FALSE;

		/* get Set_Configuration command */
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_CONFIG;

		/* Init the state */
		USBDevice.interface_state = USBD_STATE_ENUM_DONE;
		USBDevice.enum_count++;

		USBDevice.flags |= USBD_FLAG_SET_CONFIGURATION;
		*USB_UDCAHB_DEVICE_CONTROL |= usbDevHw_REG_DEV_CTRL_CSR_DONE;
	}
	if (device_status & USB_DEVICE_INT_SET_INTERFACE) {
		/* get Set_Interface command */
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_INTERFACE;
	}
	if (device_status & USB_DEVICE_INT_RESET) {
		DEBUG_LOG("[%s():%d],reset interupt\n",__func__,__LINE__);
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_RESET;
		USBDevice.flags |= USBD_FLAG_RESET;

		/* Reset the state */
		//DEBUG_LOG("usbd: Reset state\n");
		USBDevice.flags &= ~USBD_FLAG_SET_CONFIGURATION;
	
		// Reset events handled here in the isr itself.... overhead stuff done on enum done	

		/* Do hardware reinit only if we are not in reset state 
			This avoids doing reinit for multiple successive usb reset ints */
		if (USBDevice.interface_state != USBD_STATE_RESET) {
			/* reset the USBD dma state and endpoints */
			usbd_hardware_reinit(&USBDevice);
			/* notify the client(s) */
			DEBUG_LOG("[%s():%d],call usbd_client_reset()\n",__func__,__LINE__);
			usbd_client_reset();
		}

		USBDevice.interface_state = USBD_STATE_RESET;

		return;
	}
	// handle suspend interrupt
	if (device_status & USB_DEVICE_INT_IDLE) {
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_IDLE;

	}
	if (device_status & USB_DEVICE_INT_SUSPEND) {
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SUSPEND;

	}

	/* Endpoint interrupts */
	endpoint_status = *USB_UDCAHB_ENDP_INT;

	/*
	 * Looks like SETUP packets don't always disable RXDMA, even though
	 * section 11.2.3.1 implies that RXDMA will be disabled.
	 *
	 * So - waiting for RXDMA to clear would be bad
	 */

	if ((endpoint_status & 0xffff0000) != 0) {
		/* Count number of OUT endpoints requesting service */
		enable_rxdma = 0;
		for (i = 16; i < 32; ++i) {
			if (endpoint_status & (1 << i)) {
				++enable_rxdma;
			}
		}
		if (enable_rxdma > 1) {
			usbd_dump_control_status();
		}
		/* RXDMA might still not be off. Wait for it. */
		if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
#if 0
			/* This appears to be spurious - due to SETUP packets? */
			usbd_dump_control_status();

			while (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN)
				;
			usbd_dump_control_status();
#endif
		}
	}

	if (endpoint_status != 0) {
		//DEBUG_LOG("usbd: isr endpoint_int=0x%08x\n", endpoint_status);

		/* Scan the endpoints, looking for something to do */
		for (i = 0; i < USBDevice.num_endpoints; i++) {
			EPSTATUS *ep = &USBDevice.ep[i];

			if (endpoint_status & ep->intmask) {
				*USB_UDCAHB_ENDP_INT = (endpoint_status & ep->intmask);
				(*ep->eventHandler)(ep);
			}
		}
	}
	// Clear interrupt flag before enabling RX DMA
	USBDevice.flags &= ~USBD_FLAG_INTERRUPT;
	// re-enable RX DMA if needed
	if (enable_rxdma) {
		//ush_panic("usbd: RXDMA enabled in OUT ISR");
		usbd_rxdma_enable();
	}

}
