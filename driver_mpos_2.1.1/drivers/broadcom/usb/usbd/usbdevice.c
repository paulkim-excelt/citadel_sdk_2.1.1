/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
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
 * @file usbdevce.c
 *
 * @brief  USB Device internal
 */


#include "usbdevice_internal.h"
#include "usbregisters.h"
#include "usb.h"

#define DISABLE_SUSPEND_SLEEP  /* FIXME: LYCX_CS_PORT */
#if !defined DISABLE_SUSPEND_SLEEP && defined USH_BOOTROM
/* AAI */
#include "mproc_pm.h"
#endif

/* #include "pwr_mgmt.h" */

#ifdef LYNX_C_MEM_MODE
#include "idc_cpuapi.h"
#endif

#include "usbd.h"

/* Changes for USH_BOOTROM */
#ifdef USH_BOOTROM  /* AAI */
/* #include <irq.h>  */
#include <toolchain/gcc.h>
#include "sched_cmd.h"
#endif
#include <arch/cpu.h>
#include <board.h>
#include <broadcom/clock_control.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>

#include "stub.h"

/* Maximum per DMA descriptor */
/* #define MAX_USB_DMA_TX	(USB_DMA_DESC_TX_BYTES & ~(CV_USB_MAXPACKETSIZE-1)) */

#define MAX_USB_DMA_TX	(USB_DMA_DESC_TX_BYTES & ~(USBD_MAX_PKT_SIZE-1))

bool canUsbWait(void);
bool cv_card_polling_active(void);

extern void usbd_hardware_reinit(tDEVICE *pDevice);
extern void usbd_client_reset(void);


void usbd_dump_endpoint(EPSTATUS *ep)
{
	volatile tUSBDMA_DATA_DESC *pDesc;
	uint32_t control;
	uint32_t status;

	char *dir;

	ARG_UNUSED(dir);
	ARG_UNUSED(pDesc);

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
	printf("ep%d %s @ %p state=%d\n",
		ep->number, dir, ep, ep->state);
	printf("\t%s control  = 0x%08x", dir, control);
	if (control & USB_ENDP_CONTROL_STALL)
		printf(" STALL");
	if (control & USB_ENDP_CONTROL_FLUSH_TXFIFO)
		printf(" FLUSH-TXFIFO");
	if (control & USB_ENDP_CONTROL_SNOOP)
		printf(" SNOOP");
	if (control & USB_ENDP_CONTROL_POLL_DEMAND)
		printf(" POLL");
	if (control & USB_ENDP_CONTROL_NAK)
		printf(" NAK");
	printf("\n");
	printf("\t%s status   = 0x%08x", dir, status);
	if (status & USB_ENDP_STATUS_IN)
		printf(" IN");
	if (status & USB_ENDP_STATUS_BUF_NOTAVAIL)
		printf(" BNA");
	if (status & USB_ENDP_STATUS_AHB_ERR)
		printf(" AHB");
	if (status & USB_ENDP_STATUS_TX_DMA_DONE)
		printf(" TDC");
	printf("\n");
	if (is_in(ep)) {
		printf("\tin bufsize  = 0x%08x\n", *USB_UDCAHB_IN_ENDP_BUFSIZE(ep->number));
		printf("\tin maxpkt   = 0x%08x\n", *USB_UDCAHB_IN_ENDP_MAXPKTSIZE(ep->number));
		printf("\tin desc     = 0x%08x\n", pDesc);
		if (*USB_UDCAHB_ENDP_INTMASK & USB_DEVICE_ENDP_INT_IN(ep->number))
	    {		printf("\tinterrupt masked\n");  }
		else
		{	printf("\tinterrupt enabled\n");     }
	}
	else {
		printf("\tout frame#   = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_FRAMENUMBER(ep->number));
		printf("\tout maxpkt   = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(ep->number));
		printf("\tout setupbf  = 0x%08x\n", *USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number));
		printf("\tout desc     = 0x%08x\n", pDesc);
		if (*USB_UDCAHB_ENDP_INTMASK & USB_DEVICE_ENDP_INT_OUT(ep->number))
	    {		printf("\tinterrupt masked\n");  }
		else
		{	printf("\tinterrupt enabled\n");     }
	}
	printf("\t\tlen = %d xfrd = %d\n",
		ep->dma.len, ep->dma.transferred);
	printf("\t\tdwStatus   = 0x%08x\n", pDesc->dwStatus);
	printf("\t\tdwReserved = 0x%08x\n", pDesc->dwReserved);
	printf("\t\tpBuffer    = 0x%08x\n", pDesc->pBuffer);
	printf("\t\tpNext      = 0x%08x\n", pDesc->pNext);
}

void usbd_dump_control_status(void)
{
	uint32_t control;
	uint32_t status;

	control = *USB_UDCAHB_DEVICE_CONTROL;
	printf("*USB_UDCAHB_DEVICE_CONTROL = 0x%08x", control);
	if (control & USB_DEVICE_CONTROL_RESUME)
		printf(" RESUME");
	if (control & USB_DEVICE_CONTROL_RXDMA_EN)
		printf(" RXDMA");
	if (control & USB_DEVICE_CONTROL_TXDMA_EN)
		printf(" TXDMA");
	printf("\n");
	status = *USB_UDCAHB_DEVICE_STATUS;
	printf("*USB_UDCAHB_DEVICE_STATUS  = 0x%08x", status);
	if (status & USB_DEVICE_STATUS_RXFIFO_EMPTY)
		printf(" RXFIFO-EMPTY");
	printf("\n");
	printf("*USB_UDCAHB_ENDP_INT       = 0x%08x\n", *USB_UDCAHB_ENDP_INT);
	printf("*USB_UDCAHB_DEVICE_INT     = 0x%08x\n", *USB_UDCAHB_DEVICE_INT);
	printf("*USB_UDCAHB_RELEASE_NUMBER = 0x%08x\n", *USB_UDCAHB_RELEASE_NUMBER);
	printf("*USB_UDCAHB_DEVICE_CONFIG  = 0x%08x\n", *USB_UDCAHB_DEVICE_CONFIG);	
}

void usbd_dump(void)
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
	bool result = 0;
	/* Check state */
	if (!isUsbdEnumDone())
	{
		printf("usbd: ep%d %s skipped in complete\n", ep->number, is_in(ep) ? "IN" : "OUT");
		result = is_avail(ep);
		goto error_ret;
	}


#ifndef USH_BOOTROM  /* SBI */
	/* #define CV_USBBOOT_TIMEOUT	10*1000 */
	/* volatile uint32_t count = 0x10000;  */
	/* FIXME */
#if 0
	phx_set_timer_milli_seconds(PHX_BLOCKED_DELAY_TIMER_CV_USBBOOT, ticks, TIMER_MODE_FREERUN, TRUE);
	while((phx_timer_get_current_value(PHX_BLOCKED_DELAY_TIMER_CV_USBBOOT)) && (!is_avail(ep)))
	{
		usbdevice_isr();
	}
#endif

	while (ticks) {
		usbdevice_isr(NULL);
		result = is_avail(ep);
		if (result)
			break;
		ticks--;
	}
	result = is_avail(ep);
	if (!result)
	{
		printk("usbd: ep%d %s timeout (%d)\n", ep->number, is_in(ep) ? "in" : "out", ticks);
		usbd_dump_endpoint(ep);
	}
error_ret:
	return result;

#else /* AAI */

#if 0 /* SB FIXME */
	/*tick value is in ms */
	if (ticks) {
		uint32_t startTime;
		get_ms_ticks(&startTime);
		do {
			if (is_avail(ep))
				break;
			usbd_check_endpoint(ep);

			/* Check state */
			if (!canUsbWait()) {
				printf("usbd: ep%d %s skipped in complete loop \n", ep->number, is_in(ep) ? "IN" : "OUT");
				result = is_avail(ep);
				goto error_ret;
			}
		} while (cvGetDeltaTime(startTime) < ticks);
	}
#endif
	if (ticks) {
		do {
				usbd_check_endpoint(ep);

				if (is_avail(ep))
					break;

				/* Check state */
				if (!canUsbWait()) {
					printf("usbd: ep%d %s skipped in complete loop \n", ep->number, is_in(ep) ? "IN" : "OUT");
					result = is_avail(ep);
					goto error_ret;
				}
			} while (ticks--);
	}

	result = is_avail(ep);
	if (!result) {
		if (ticks > 100) {
			/* small ticks are basically polls and they are not timeouts */
			printf("usbd: ep %d %s timeout (%d) \n", ep->number, is_in(ep) ? "in" : "out", ticks);
			usbd_dump_endpoint(ep);
		}
	}
error_ret:
	return result;
#endif
}

void usbd_cancel(EPSTATUS *ep)
{
	if (is_in(ep)) {
		ENTER_CRITICAL();
		switch (ep->state) {
		case USBD_EPS_IDLE:
			trace_ep(ep, "IN transfer canceled (idle state)\n");
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

			EXIT_CRITICAL(); /* don't block ints for debug prints */
			error_ep(ep, "IN transfer canceled (pending state)\n");
			usbd_dump_endpoint(ep);
			ENTER_CRITICAL();
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

			EXIT_CRITICAL(); /* don't block ints for debug prints */
			error_ep(ep, "IN transfer canceled (active state)\n");
			usbd_dump_endpoint(ep);
			ENTER_CRITICAL();
			break;
		}
		EXIT_CRITICAL();
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

	/* In case there is a pending request to clear the NAK later, canceling that request here
	   because the NAK should be set in this function */
	if (is_in(ep))
		USBDevice.delayed_CNAK_mask &= ~USB_DEVICE_ENDP_INT_IN(ep->number);
	else
		USBDevice.delayed_CNAK_mask &= ~USB_DEVICE_ENDP_INT_OUT(ep->number);
}

/* Check if RxFIFO is empty */
bool is_rxfifo_empty(void)
{
	uint32_t USBdeviceStatus;
	USBdeviceStatus = *USB_UDCAHB_DEVICE_STATUS;

	if (USBdeviceStatus & USB_DEVICE_STATUS_RXFIFO_EMPTY)
	{
		return TRUE;
	}

	return FALSE;
}

void usbd_clear_nak(EPSTATUS *ep)
{
	volatile uint32_t *endp_control;

	if (is_in(ep))
		endp_control = USB_UDCAHB_IN_ENDP_CONTROL(ep->number);
	else
		endp_control = USB_UDCAHB_OUT_ENDP_CONTROL(ep->number);
	*endp_control |= USB_ENDP_CONTROL_CLEAR_NAK;

	if (!is_rxfifo_empty())
	{
		/* According to the documentation cannot set CNAK when RxFIFO is not empty */
		/* Mark a clear request in delayed_CNAK_mask so that the NAK will be cleared later */
		if (is_in(ep))
			USBDevice.delayed_CNAK_mask |= USB_DEVICE_ENDP_INT_IN(ep->number);
		else
			USBDevice.delayed_CNAK_mask |= USB_DEVICE_ENDP_INT_OUT(ep->number);
	}
}

/*
 * Re-enable RX dma, probably after receiving and processing an OUT packet.
 *
 * Only enable if
 *  1) Not in interrupt processing
 *  2) All non-control endpoints are ready to receive
 */
void usbd_rxdma_enable(void)
{
	int i;

	ENTER_CRITICAL();

	if (USBDevice.flags & USBD_FLAG_INTERRUPT) {
		EXIT_CRITICAL();
		return;
	}

	if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
		EXIT_CRITICAL();
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
				EXIT_CRITICAL();
				return;
			}
#endif
			if (status == USB_DMA_DESC_BUFSTAT_HOST_READY) {
				usbd_clear_nak(ep);
			}
		}
	}
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_RXDMA_EN;
	EXIT_CRITICAL();
}

void usbd_rxdma_disable()
{
	if (!(*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN))
		return;

	ENTER_CRITICAL();
	*USB_UDCAHB_DEVICE_CONTROL &= ~USB_DEVICE_CONTROL_RXDMA_EN;
	EXIT_CRITICAL();
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

	ENTER_CRITICAL();

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

	/* Clear any left-over error status from before there was anything to transmit.*/
	*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_BUF_NOTAVAIL;

#ifdef USBD_USE_IN_PENDING_STATE
	ep_set_pending(ep);
#else
	ep_set_active(ep);
	*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_POLL_DEMAND;
#endif

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* CPU WR Hw RD */
	clean_dcache_by_addr((u32_t)ep, sizeof(EPSTATUS));
	clean_dcache_by_addr((u32_t)pDesc, sizeof(tUSBDMA_DATA_DESC));
	clean_dcache_by_addr((u32_t)pDesc->pBuffer, pDmaInfo->desc_len);
#endif

	/* SYS_LOG_DBG(" len: %d dwstatus: 0x%04x pBuffer: 0x%04x",(u32_t)pDmaInfo->len,(u32_t)pDesc->dwStatus,(u32_t)pDesc->pBuffer);*/

	usbd_clear_nak(ep);


	EXIT_CRITICAL();
	return txsize;
}

void sendEndpointData(EPSTATUS *ep, uint8_t *pbuffer, uint32_t len)
{

	usb_dma_info_t *pDmaInfo = &ep->dma;
    volatile tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;


	if (!is_in(ep)) {
		printf("ep%d: ERROR cannot send on non-IN endpoint\n", ep->number);
		return;
	}
	trace_ep(ep, "sendEndpointData %p %d\n", pbuffer, len);

	if (!is_avail(ep) && !is_ctrl(ep)) {
		printf("ep%d: sendEndpointData while endpoint not available\n", ep->number);
		/* Stop the transfer in progress */
		usbd_cancel(ep);
	}
	if (!isUsbdEnumDone() && !is_ctrl(ep)) {
		printf("ep%d: sendEndpointData while endpoint enum not complete\n", ep->number);
		return;
	}

#ifdef CONFIG_DATA_CACHE_SUPPORT
		if ((u32_t)pbuffer & (DCACHE_LINE_SIZE - 1)) {
			printf("usbd sendEndpointData: Address %p is not aligned \n",pbuffer);
		}
#endif

	ENTER_CRITICAL();

	pDmaInfo->pBuffer = pbuffer;
	pDmaInfo->len = len;
	pDmaInfo->transferred = 0;
	pDmaInfo->pXfr = pbuffer;

#ifdef USBD_USE_IN_PENDING_STATE
	ep_set_pending(ep);
#else
	ep_set_active(ep);
#endif

	*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;

	set_txdata(ep);

#ifdef USBD_USE_IN_ENDPOINT_INTMASK
    /* enable endpoint interrupt here */
    *USB_UDCAHB_ENDP_INTMASK &= ~USB_DEVICE_ENDP_INT_IN(ep->number);
#endif
	EXIT_CRITICAL();
}

void sendCtrlEpData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
 	sendEndpointData(ep, pData, len);
}

void sendCtrlEndpointData(uint8_t *pBuffer, uint32_t len)
{
	EPSTATUS *ep = USBDevice.controlIn;

	if (len > sizeof(DescriptorBuffer)) {
		trace_ep(ep, "sendCtrlEndpointData: excessive length\n");
		len = sizeof(DescriptorBuffer);
	}
	/* copy to RAM is required, as we can't DMA from ROM */
	memcpy(DescriptorBuffer, pBuffer, len);
	sendCtrlEpData(ep, DescriptorBuffer, len);
}

void usbdEpInInterrupt(EPSTATUS *ep)
{
	uint32_t status = *USB_UDCAHB_IN_ENDP_STATUS(ep->number);

	volatile tUSBDMA_DATA_DESC *pDesc;

	pDesc = (tUSBDMA_DATA_DESC *) *USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number);

	/* SYS_LOG_DBG(" >> IN addr: 0x%08x no: %d status:0x%08x",(unsigned int)ep,(unsigned int)ep->number,(unsigned int)status);*/

	if (status & USB_ENDP_STATUS_AHB_ERR) {
		error_ep(ep, "IN AHB error\n");
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_AHB_ERR;
		status &= ~USB_ENDP_STATUS_AHB_ERR;
		usbd_cancel(ep);
	}

	if (status & USB_ENDP_STATUS_BUF_NOTAVAIL) {
		error_ep(ep, "IN buf notavail error, avail=%d\n", is_avail(ep));
		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_BUF_NOTAVAIL;
		status &= ~USB_ENDP_STATUS_BUF_NOTAVAIL;
		usbd_cancel(ep);
		if (ep->eventCallback)
			(*ep->eventCallback)(ep, USBD_EPE_BUF_NOTAVAIL);
	}

	if (status & USB_ENDP_STATUS_TX_DMA_DONE) {
		int success = 1;
		int more = 0;
		char *result = "SUCCESS";
		uint32_t len = ep->dma.dma.dwStatus & USB_DMA_DESC_TX_BYTES;

		ARG_UNUSED(result);

		*USB_UDCAHB_IN_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_TX_DMA_DONE;
		status &= ~USB_ENDP_STATUS_TX_DMA_DONE;

		printf("pDesc->dwStatus:0x%08x", pDesc->dwStatus);

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
#if 1   /* FIXME Why is this kept*/
			/* Sometimes, the tx-bytes field in the dma descriptor is corrupted
			 * even though all the data has been transmitted. This can mess up
			 * the pointer calculations and other termination tests.
			 * Trust the DMA status, and patch up the length field here.
			 */
			len = ep->dma.desc_len;
			ep->dma.dma.dwStatus = (ep->dma.dma.dwStatus & ~USB_DMA_DESC_TX_BYTES) | len;
#endif
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
				error_ep(ep, "wrong tx len %d, expected %d\n", len, ep->dma.desc_len);
				more = 0;
				usbd_dump_endpoint(ep);
				ush_panic("wrong tx length");
			}
		}
		else {
			usbd_dump_endpoint(ep);
		}

		/* Now that we've figured out what's happened, and what we'll do next,
		 * log the information to the serial console (if requested).
		 */
		trace_ep(ep, "TX DMA-done %s len = %d dwStatus = 0x%08x more = %d\n",
			result, len, pDesc->dwStatus, more);
		printf("TX DMA-done len = %d dwStatus = 0x%08x more = %d \n", len, pDesc->dwStatus, more);
		if (is_avail(ep))
			error_ep(ep, "TX-DMA done on avail endpoint\n");

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
			printf("IN token (idle) -> disable EPINTR\n");
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
			/* SYS_LOG_DBG("IN token (pending) -> active\n"); */
			ep->dma.dma.dwStatus = ((ep->dma.dma.dwStatus & ~USB_DMA_DESC_BUFSTAT_MASK)
						| USB_DMA_DESC_BUFSTAT_HOST_READY);


#ifdef CONFIG_DATA_CACHE_SUPPORT
			usb_dma_info_t *pDmaInfo = &ep->dma;
			volatile tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

			clean_n_invalidate_dcache_by_addr((u32_t) pDmaInfo->pBuffer, pDmaInfo->len);

			/* Clean and invalidate the dcache before setting poll demand */
			clean_n_invalidate_dcache_by_addr((u32_t) pDesc, sizeof(tUSBDMA_DATA_DESC));

#endif
			*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_POLL_DEMAND;
			ep_set_active(ep);
			break;

		case USBD_EPS_ACTIVE:
			/* This case should not happen */
			/* SYS_LOG_DBG("IN token (active)  -> cnak\n"); */
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


	/* trace_ep(ep, "set_rxdata: buffer=%p\n", pDmaInfo->pXfr);.*/
	ENTER_CRITICAL();

	/* Clear any left-over error status from before there was.*/
	/* someplace to receive to.*/
	*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = ~0;

	pDesc->pBuffer = pDmaInfo->pXfr;
	pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_HOST_READY | USB_DMA_DESC_LAST;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Hw will write so clean*/
	clean_dcache_by_addr((u32_t)ep, sizeof(EPSTATUS));
	clean_dcache_by_addr((u32_t)pDesc, sizeof(tUSBDMA_DATA_DESC));
	clean_dcache_by_addr((u32_t)pDmaInfo->pBuffer, pDmaInfo->len);
#endif

	/* SYS_LOG_DBG(" len: %d dwstatus: 0x%04x pBuffer: 0x%04x",(u32_t)pDmaInfo->len,(u32_t)pDesc->dwStatus,(u32_t)pDesc->pBuffer);.*/

	ep_set_active(ep);
	usbd_clear_nak(ep);
	usbd_rxdma_enable();
	EXIT_CRITICAL();
}

void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
	usb_dma_info_t *pDmaInfo = &ep->dma;

	if (!is_out(ep)) {
		error_ep(ep, "ERROR cannot recv on non-OUT endpoint\n");
		return;
	}
	trace_ep(ep, "recvEndpointData %p %d\n", pData, len);
	if (len > 256*1024) {
		error_ep(ep, "ERROR bad length: recvEndpointData %p %d\n", pData, len);
		ush_panic("recvEndpointData: bad length");
	}
#if 0
	if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
		printf("%s: RXDMA enabled\n", __FUNCTION__);
	}
#endif

#ifdef CONFIG_DATA_CACHE_SUPPORT
	if ((u32_t)pData & (DCACHE_LINE_SIZE - 1)) {
		printf("usbd recvEndpointData: Address %p is not aligned \n",pData);
	}
	if ((u32_t)len % DCACHE_LINE_SIZE != 0) {
		printf("usbd recvEndpointData: len %x is not aligned \n",len);
	}
#endif

#ifdef LYNX_C_MEM_MODE
   /* Save the address in the reserved word */
    pDmaInfo->dma.dwReserved = (uint32_t)pData;

   /* Ensure data buffer in uncached space for DMA operation */
    if( USH_IS_CACHED_ADDR(pData) )
    {
        printf("usbd: recvEndpointData %p %d .\n", pData, len);

        /* flush cache */
        phx_idc_cache_wrback (IDC_TARGET_DCACHE);
        /* drain the write buffer */
        phx_idc_cache_drainwb(IDC_TARGET_DCACHE);
        /* set the write-through mode */
        phx_idc_enable (IDC_DCACHE, IDC_ENABLE, IDC_ENABLE, IDC_WRTHRU_POLICY, IDC_FULLSECURE);

        /* convert to bus address */
        pData = USH_ADDR_2_UNCACHED(pData);
    }
#endif
	ENTER_CRITICAL();
#ifdef USBD_USE_OUT_ENDPOINT_INTMASK
	/* enable endpoint interrupt here */
	*USB_UDCAHB_ENDP_INTMASK &= ~USB_DEVICE_ENDP_INT_OUT(ep->number);
#endif
	pDmaInfo->pBuffer = pData;
	pDmaInfo->len = len;
	pDmaInfo->transferred = 0;
	pDmaInfo->pXfr = pData;
	set_rxdata(ep);
	EXIT_CRITICAL();
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
	tUSBDMA_SETUP_DESC *pSetupDesc = (tUSBDMA_SETUP_DESC *) *USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number);

	/* SYS_LOG_DBG(" << OUT addr: 0x%08x no: %d status:0x%08x",(unsigned int)ep,(unsigned int)ep->number,(unsigned int)status); */

	if (status & USB_ENDP_STATUS_AHB_ERR) {
		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_AHB_ERR;
		error_ep(ep, "OUT AHB error\n");
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
		error_ep(ep, "OUT buf notavail error\n");
#if 1   /* FIXME Why is this kept*/
		usbd_dump_endpoint(ep);
#endif
		if (is_ctrl(ep)) {
			/* Something is not happy, start over */
			USBDevice.flags |= USBD_FLAG_RESTART;
		}
		if (ep->eventCallback)
			(*ep->eventCallback)(ep, USBD_EPE_BUF_NOTAVAIL);
		/* recover by re-setting up the receive descriptor */
		set_rxdata(ep);
#if 1   /* FIXME Why is this kept*/
		printf("Endpoint after BNA fixup:\n");
		usbd_dump_endpoint(ep);
		/* ush_panic("RX BNA"); */
#endif
		return;
	}

	if ((status & USB_ENDP_STATUS_OUT_MASK) == USB_ENDP_STATUS_OUT_SETUP) {
		SetupPacket_t  req;

		/* SYS_LOG_DBG(" << pSetupDesc : 0x%x dwStatus %x dw0 %x dw1 %x ",(unsigned int)pSetupDesc,(unsigned int)pSetupDesc->dwStatus, (unsigned int)pSetupDesc->dwData[0],(unsigned int)pSetupDesc->dwData[1]);.*/

		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_OUT_SETUP;

#ifdef CONFIG_DATA_CACHE_SUPPORT
		invalidate_dcache_by_addr((u32_t)ep, sizeof(EPSTATUS));
		invalidate_dcache_by_addr((u32_t) pSetupDesc, sizeof(tUSBDMA_SETUP_DESC));
#endif

		/* Copy data from SetupBuffer to USBRequest */
		memcpy((uint8_t *) &req, (uint8_t *)(pSetupDesc->dwData), sizeof(SetupPacket_t));

		/* here, the Protocol layer put IN data in System memory but hold it until the host ask it to be.*/
		handleUsbRequest(&req);
		/* here we have three possible endpoint states, either:
		 * 1. TX :  host requests data
		 * 2. RX:   host is going to send data
		 * 3. IDLE: host is either send invalid setup up token and is stalled or setup token with no data transaction.
		 */

		// In case we need to receive anything (even a 0-length packet)
		recvEndpointData(ep, pusb_mem_block->ctrl_buffer, ep->maxPktSize);

		/* For debugging */
		/* memset(pSetupDesc, 0, sizeof(*pSetupDesc)); */
	}

	/*
	 * Got an out packet.
	 * May be done
	 * call callback routine
	 * May be done with descriptor, but not with overall request
	 * queue up next request
	 * May not be done with current descriptor
	 * just continue
	 */
	if ((status & USB_ENDP_STATUS_OUT_MASK) == USB_ENDP_STATUS_OUT_DATA) {
		int success = 1;
		volatile tUSBDMA_DATA_DESC *pDesc = &ep->dma.dma;

#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Hw has written CPU will read so invalidate*/
		usb_dma_info_t *pDmaInfo = &ep->dma;
		invalidate_dcache_by_addr((u32_t)ep, sizeof(EPSTATUS));
		invalidate_dcache_by_addr((u32_t)pDesc, sizeof(tUSBDMA_DATA_DESC));
		invalidate_dcache_by_addr((u32_t)pDmaInfo->pBuffer, pDmaInfo->len);
#endif
		/* SYS_LOG_DBG("USB_ENDP_STATUS_OUT_DATA pDesc->dwStatus =%x\n",(u32_t)pDesc->dwStatus);*/

		*USB_UDCAHB_OUT_ENDP_STATUS(ep->number) = USB_ENDP_STATUS_OUT_DATA;
		if ((pDesc->dwStatus & USB_DMA_DESC_BUFSTAT_MASK) == USB_DMA_DESC_BUFSTAT_DMA_DONE) {
			int more;
			uint32_t len;
			char *result = "";

			ARG_UNUSED(result);

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
				error_ep(ep, "RX DMA-Done %s dwStatus=0x%08x status=0x%08x\n",
					result, pDesc->dwStatus, status);
				more = 0;
				usbd_dump_endpoint(ep);
			}
			if (success) {
				/* Advance pointers*/
				len = usbd_advance_pointers(ep);
				/* trace_ep(ep, "RX DMA-Done %s dwStatus=0x%08x status=0x%08x pXfr=%p, len=%d\n",
				 * result, pDesc->dwStatus, status, ep->dma.pXfr - len, len);
				 */

				/* See if the DMA engine overran the descriptor */
				if (ep->dma.transferred > ep->dma.len) {
					error_ep(ep, "RX too much, len = %d transferred = %d\n",
						ep->dma.len, ep->dma.transferred);
					printf("usbd: RX too much\n");
					/* replace with the printf above: ush_panic("usbd: RX too much\n");*/
				}
				/* Decide if there's more to receive.*/
				more = (ep->dma.transferred < ep->dma.len) && (!len || (len == ep->maxPktSize));

				/* SYS_LOG_DBG("RX DMA-Done %s dwStatus=0x%08x status=0x%08x pXfr=%p, len=%d more =%d\n", result, pDesc->dwStatus, status, ep->dma.pXfr - len, len,more); */

			}
			if (more) {
				/* DMA done, but more to transfer.
				 * Setup the next descriptor
				 */
				set_rxdata(ep);
			}
			else {
				/* All done, invoke the callback */
				ep_set_idle(ep);
				usbd_set_nak(ep);
#ifdef USBD_USE_OUT_ENDPOINT_INTMASK
				/* disable endpoint interrupt here */
				*USB_UDCAHB_ENDP_INTMASK |= USB_DEVICE_ENDP_INT_OUT(ep->number);
#endif
#ifdef LYNX_C_MEM_MODE
				/* The received buffer is in cachable space, need the cache sync  */
				if ( USH_IS_CACHED_ADDR(ep->dma.dma.dwReserved) )
				{
					/* Invalidate the dcache in case of cached data changed by DMA */
					phx_idc_cache_inval (IDC_TARGET_DCACHE);
					/* Set back the dcache to the write-back mode */
					phx_idc_enable (IDC_DCACHE, IDC_ENABLE, IDC_ENABLE, IDC_WRBACK_POLICY, IDC_FULLSECURE);
				}
#endif

				if (success) {
					/* SYS_LOG_DBG("RX complete, func=%p len=%d\n", ep->appCallback, ep->dma.transferred); */

					if (ep->appCallback != NULL)
						(*ep->appCallback)(ep);
				}
				else {
					SYS_LOG_DBG("RX complete, ERROR\n");
					if (ep->eventCallback)
						(*ep->eventCallback)(ep, USBD_EPE_RXERROR);
				}
			}
		}
	}
}

void stallControlEP(EPSTATUS *ep)
{
	/* Set IN and OUT STALL
	 * but how to set it, check with hardware / verification engineers
	 * before set this bit, the application must check for RXFIFO emptiness, but how?
	 */
	*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_STALL;
	/* need to purge FIFO ?
	 * ready for the next setup transfer
	 */

	/* synopsys AHB p 210 chapter 13.4  clear OUT NAK bit */
	usbd_clear_nak(ep);
	set_rxdata(ep);
}

void usbd_debug(void)
{
	EPSTATUS *ep;
	ARG_UNUSED(ep);

	printf("usbd: debug\n");
	/*Got to go */
#ifdef USBD_CV_INTERFACE
	ep = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | IN_DIR);
	printf("usbd: CV ep%d %s\n", ep->number, is_in(ep) ? "in" : "out");
	usbd_dump_endpoint(ep);
	ep = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | OUT_DIR);
	printf("usbd: CV ep%d %s\n", ep->number, is_in(ep) ? "in" : "out");
	usbd_dump_endpoint(ep);
#endif
}

void usbd_check_endpoints2(void)
{
	EPSTATUS *ep;
	ARG_UNUSED(ep);
	/*Got to go */
#ifdef USBD_CV_INTERFACE
	ep = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | OUT_DIR);
	if(ep)
	{
	
	}
#endif
}

void usbd_check_endpoint(EPSTATUS *ep)
{
	volatile int do_dump = 0;

	if (is_ctrl(ep))
		return;
	ENTER_CRITICAL();
	if (is_out(ep)) {
	 	volatile tUSBDMA_DATA_DESC *pDesc = &ep->dma.dma;
	 	uint32_t status = pDesc->dwStatus & USB_DMA_DESC_BUFSTAT_MASK;

	 	if (status == USB_DMA_DESC_BUFSTAT_HOST_READY) {
	 		if (!is_active(ep)) {
	 			error_ep(ep, "dma buffer host-ready, but endpoint not active\n");
				do_dump = 1;
	 		}
	 		if (*USB_UDCAHB_OUT_ENDP_CONTROL(ep->number) & USB_ENDP_CONTROL_NAK) {
		 		usbd_clear_nak(ep);
	 		}
	 	}
	}
	if (do_dump)
		usbd_dump_endpoint(ep);
	EXIT_CRITICAL();
}

void usbd_check_endpoints(void)
{
	int i;

	/* Check for active endpoints that have NAK set */
	for (i = 0; i < USBDevice.num_endpoints; i++) {
		usbd_check_endpoint(&USBDevice.ep[i]);
	}
}

#ifdef USH_BOOTROM  /*AAI */
void cvTasksToRunOnWakeUpFromSelectiveSuspend(void)
{
	/*Nothing related to cv should be in usb */
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
void usbdevice_isr(void *unused)
{
	int i;
	uint32_t device_status;
	uint32_t endpoint_status;
	int enable_rxdma = 0;
	unsigned int xTaskWoken = FALSE;

#if 0
	push_glb_regs();
	reinitialize_function_table();
	check_and_wake_system();
#endif

	ARG_UNUSED(unused);
	ARG_UNUSED(xTaskWoken);

	USBDevice.flags |= USBD_FLAG_INTERRUPT;

	/* Check if there is a pending request to clear the NAK and RxFIFO is now empty */
	if (USBDevice.delayed_CNAK_mask && is_rxfifo_empty())
	{
		for (i = 0; i < USBDevice.num_endpoints; i++) {
			EPSTATUS *ep = &USBDevice.ep[i];
			if (USBDevice.delayed_CNAK_mask & ep->intmask) {
				/* Cancel the pending request and attempt to clear the NAK now */
				USBDevice.delayed_CNAK_mask &= ~(ep->intmask);
				usbd_clear_nak(ep);
			}
		}
	}

	device_status = *USB_UDCAHB_DEVICE_INT;

	if (device_status) {
		trace("usbd: isr device_int=0x%08x \n", device_status);
	}

	/* INT_SET_CONFIG */
	if (device_status & USB_DEVICE_INT_SET_CONFIG) {

		/* get Set_Configuration command */
		/* SYS_LOG_DBG("SET_CONFIGURATION"); */
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_CONFIG;

		/* Init the state */
		/* printf("usbd: Enable state\n"); */
		USBDevice.interface_state = USBD_STATE_ENUM_DONE;
		USBDevice.enum_count++;

		/* queue a command to let CCID send out card slot change notification to pc host */
		xTaskWoken = queue_cmd( SCHED_USB_ENUM_DONE_CMD, QUEUE_FROM_ISR, xTaskWoken);
		SCHEDULE_FROM_ISR(xTaskWoken);

		USBDevice.flags |= USBD_FLAG_SET_CONFIGURATION;
	}

	/* INT_SET_INTERFACE */
	if (device_status & USB_DEVICE_INT_SET_INTERFACE) {
		/* get Set_Interface command */
		/* SYS_LOG_DBG("SET_INTERFACE"); */
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_INTERFACE;
	}

	/* INT_RESET */
	if (device_status & USB_DEVICE_INT_RESET) {
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_RESET;
		/* SYS_LOG_DBG("INT_RESET intf_st: %d",USBDevice.interface_state); */
		USBDevice.flags |= USBD_FLAG_RESET;

		/* Reset the state */
		/* printf("usbd: isr: Reset state\n"); */
		USBDevice.flags &= ~USBD_FLAG_SET_CONFIGURATION;
	
		/* Reset events handled here in the isr itself
		 * overhead stuff done on enum done
		 */
		/* Do hardware reinit only if we are not in reset state 
		 * This avoids doing reinit for multiple successive usb reset ints
		 */
		if (USBDevice.interface_state != USBD_STATE_RESET) {
			/* reset the USBD dma state and endpoints */
			usbd_hardware_reinit(&USBDevice);
			/* notify the client(s) */
			usbd_client_reset();
#if !defined DISABLE_SUSPEND_SLEEP
			if( lynx_sleep_state())
			{
				printf("USB_ISR: Clear the sleep mode - due to the reset before the transition to sleep..\n");
				lynx_set_pwr_state( CRMU_IHOST_POWER_CONFIG_NORMAL );
				handle_turnon_ush_command();
			}
#endif
		}

		USBDevice.interface_state = USBD_STATE_RESET;
		/* return; */
		goto exit_isr;

	}

	/* INT_SOF */
	if (device_status & USB_DEVICE_INT_SOF) {
		/* need to clear this bit even though we don't act on it */
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SOF;
	}

	/* INT_LPM_TKN */
	if (device_status & USB_DEVICE_INT_LPM_TKN) {
		/* get info from token */
		trace("LPM_TKN \n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_LPM_TKN;
	}

	/* INT_IDLE */
	if (device_status & USB_DEVICE_INT_IDLE) {
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_IDLE;
		USBDSSuspend.idle_state = 1;
		/* SYS_LOG_DBG("INT_IDLE"); */

#if 0
		if (USBDevice.flags & USBD_FLAG_SET_CONFIGURATION)
			phx_usbd_idle();
#endif
	}

	/* INT_SUSPEND */
	if (device_status & USB_DEVICE_INT_SUSPEND) {
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SUSPEND;
		/* SYS_LOG_DBG("INT_SUSPEND"); */
		if (USBDSSuspend.idle_state) {
			USBDSSuspend.suspend_state = 1;
			/* If already idle, then set selective suspended */
#if !defined DISABLE_SUSPEND_SLEEP
			printf("Try placing the system in selective suspend...");
			if ( lynx_sleep_mode_blocked() )
			{
				printf("sleep mode blocked...\n");
			} else
			if ( (CV_VOLATILE_DATA->CVDeviceState & CV_USH_INITIALIZED) ) {
				if(!cv_card_polling_active())
				{
					printf("done \n");
					lynx_put_lowpower_mode( CRMU_IHOST_POWER_CONFIG_SLEEP );
				}
				else
				{
					printf("card is being polled so skip. \n");
				}
			}
			else
			{
				printf("not ready yet. \n");
			}
#endif
		}
#if 0
		if (USBDevice.flags & USBD_FLAG_SET_CONFIGURATION)
			phx_usbd_suspend();
#endif
	}
	
	/* Reset suspend state if IDLE bit or suspend bit not set
	 * Do not know of another good way to indicate device has been woken up!
	 */
	if (USBDSSuspend.suspend_state)
	{
		if (!((device_status & USB_DEVICE_INT_IDLE) || (device_status & USB_DEVICE_INT_SUSPEND)))
		{
			USBDSSuspend.idle_state = 0;
			USBDSSuspend.suspend_state = 0;
#ifndef DISABLE_SUSPEND_SLEEP
			printf("Wake up from selective suspend \n");
			cvTasksToRunOnWakeUpFromSelectiveSuspend();
#endif
		}
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
		for (i = 16; i < 32; ++i)
			if (endpoint_status & (1 << i))
				++enable_rxdma;
		if (enable_rxdma > 1) {
			trace("usbd: multiple OUT endpoints requesting service \n");
			usbd_dump_control_status();
		}
		/* RXDMA might still not be off. Wait for it. */
		if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN) {
#if 0
			/* This appears to be spurious - due to SETUP packets? */
			printf("RXDMA set, but OUT interrupt pending\n");
			usbd_dump_control_status();
#endif
#if 0
			while (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN)
				;
			usbd_dump_control_status();
#endif
		}
	}

	if (endpoint_status != 0) {
		uint8_t found = 0;
		trace("endpoint_int=0x%08x \n", endpoint_status);

		/* Scan the endpoints, looking for something to do */
		for (i = 0; i < USBDevice.num_endpoints; i++) {
			EPSTATUS *ep = &USBDevice.ep[i];

			if (endpoint_status & ep->intmask) {
				found = 1;
				*USB_UDCAHB_ENDP_INT = (endpoint_status & ep->intmask);
				(*ep->eventHandler)(ep);
			}
		}
		if (!found) {
			printf("%s: clearing unsolicited endpoint interrupt 0x%08x\n",__func__,endpoint_status);
			*USB_UDCAHB_ENDP_INT = endpoint_status;
		}
	}
	/* Clear interrupt flag before enabling RX DMA */
	USBDevice.flags &= ~USBD_FLAG_INTERRUPT;
	/* re-enable RX DMA if needed */
	if (enable_rxdma) {
		/*ush_panic("usbd: RXDMA enabled in OUT ISR"); */
		usbd_rxdma_enable();
	}

exit_isr:
	/* FIXME SB why the isr becomes pending on SET_ADDRESS */
	/* Set to Pending till idle_loop executes */
	if (*USB_UDCAHB_DEVICE_INT || *USB_UDCAHB_ENDP_INT) {
		/*SYS_LOG_DBG("Pending: USB_UDCAHB_ENDP_INT=0x%08x ",(u32_t)*USB_UDCAHB_ENDP_INT); */
		gic400_irq_set_pend(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT));
	}

#if 0
	pop_glb_regs();
#endif

}
bool canUsbWait(void)
{

#if 0 /*FIXME */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_CANCEL_PENDING_CMD) {
		printf("usbd: CV cmd cancelled... skipped in complete loop\n");
		goto error_ret;
	}
	if (!isUsbdEnumDone()) {
		printf("usbd: Enum not done... skipped in complete loop\n");
		goto error_ret;
	}
#ifdef PHX_REQ_SMBUS			/* BD */
	if (smbus_poll_data()) {
		printf("usbd: SMBUS waiting... skipped in complete loop\n");
		goto error_ret;
	}
#endif
	if (ISSET_SYSTEM_STATE(CUR_SYS_STATE_LOW_PWR)) {
		printf("usbd: system in low pwr... skipped in complete loop\n");
		goto error_ret;
	}

	if (IS_SYSTEM_RESTARTING()) {
		printf("usbd: System restarting...  skipped in complete loop\n");
		goto error_ret;
	}
	if (get_num_queue_cmds()) {
#define MAX_CMD_WAIT_TIME_MS	10000
		if (cvGetDeltaTime(startTime) > MAX_CMD_WAIT_TIME_MS) {
			printf("usbd: skipped in complete loop [cmds waiting] %d:%d [max %d]\n",
				ticks, cvGetDeltaTime(startTime), ticks);
			goto error_ret;
		}
	}
error_ret:
    return 0;
#endif
	return 1;

}

#else /*SBI */

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
void usbdevice_isr(void *unused)
{
	int i;
	uint32_t device_status;
	uint32_t endpoint_status;
	int enable_rxdma = 0;
	
	ARG_UNUSED(unused);
	USBDevice.flags |= USBD_FLAG_INTERRUPT;

	/* Check if there is a pending request to clear the NAK and RxFIFO is now empty */
	if (USBDevice.delayed_CNAK_mask && is_rxfifo_empty())
	{
		for (i = 0; i < USBDevice.num_endpoints; i++) {
			EPSTATUS *ep = &USBDevice.ep[i];
			if (USBDevice.delayed_CNAK_mask & ep->intmask) {
				/* Cancel the pending request and attempt to clear the NAK now */
				USBDevice.delayed_CNAK_mask &= ~(ep->intmask);
				usbd_clear_nak(ep);
			}
		}
	}

	device_status = *USB_UDCAHB_DEVICE_INT;

	if (device_status & USB_DEVICE_INT_SET_CONFIG)
	{
		/* get Set_Configuration command */
		printf("SET_CONFIGURATION\n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_CONFIG;

		/* Init the state */
		/* printf("usbd: Enable state\n"); */
		USBDevice.interface_state = USBD_STATE_ENUM_DONE;
		USBDevice.enum_count++;
		USBDevice.flags |= USBD_FLAG_SET_CONFIGURATION;
	}

	if (device_status & USB_DEVICE_INT_ENUM_SPEED)
	{
		/* speed enum */
		printf("ENUM_SPEED\n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_ENUM_SPEED;
	}

	if (device_status & USB_DEVICE_INT_SET_INTERFACE)
	{
		/* get Set_Interface command */
		printf("SET_INTERFACE\n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SET_INTERFACE;
	}

	if (device_status & USB_DEVICE_INT_RESET)
	{
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_RESET;
		printf("RESET\n");
		USBDevice.flags |= USBD_FLAG_RESET;

		/* Reset the state */
		/* printf("usbd: isr: Reset state\n"); */
		USBDevice.flags &= ~USBD_FLAG_SET_CONFIGURATION;
	
		/* Reset events handled here in the isr itself
		.*.overhead stuff done on enum done
		 */
		/* Do hardware reinit only if we are not in reset state 
		 * This avoids doing reinit for multiple successive usb reset ints
		 */
		if (USBDevice.interface_state != USBD_STATE_RESET)
		{
			/* reset the USBD dma state and endpoints */
			usbd_hardware_reinit(&USBDevice);

			/* notify the client(s) */
			usbd_client_reset();
		}

		USBDevice.interface_state = USBD_STATE_RESET;

		return;
	}

	if (device_status & USB_DEVICE_INT_SOF)
	{
		/* need to clear this bit even though we don't act on it */
		printf("SOF\n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SOF;
	}

	/* handle LPM token interrupt */
	if (device_status & USB_DEVICE_INT_LPM_TKN)
	{
		/* get info from token */
		printf("LPM_TKN\n");
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_LPM_TKN;
	}

	if (device_status & USB_DEVICE_INT_IDLE)
	{
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_IDLE;
		printf("IDLE\n");
		USBDSSuspend.idle_state = 1;
	}

	/* handle suspend interrupt */
	if (device_status & USB_DEVICE_INT_SUSPEND)
	{
		*USB_UDCAHB_DEVICE_INT = USB_DEVICE_INT_SUSPEND;
		printf("SUSPEND\n");
	}

	/* Endpoint interrupts */
	endpoint_status = *USB_UDCAHB_ENDP_INT;
	/*
	 * Looks like SETUP packets don't always disable RXDMA, even though
	 * section 11.2.3.1 implies that RXDMA will be disabled.
	 *
	 * So - waiting for RXDMA to clear would be bad
	 */

	if ((endpoint_status & 0xffff0000) != 0)
	{
		/* Count number of OUT endpoints requesting service */
		enable_rxdma = 0;
		for (i = 16; i < 32; ++i)
			if (endpoint_status & (1 << i))
				++enable_rxdma;
		if (enable_rxdma > 1) {
			printf("usbd: multiple OUT endpoints requesting service\n");
			usbd_dump_control_status();
		}
		/* RXDMA might still not be off. Wait for it. */
		if (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN)
		{
#if 0
			/* This appears to be spurious - due to SETUP packets? */
			printf("RXDMA set, but OUT interrupt pending\n");
			usbd_dump_control_status();
#endif
#if 0
			while (*USB_UDCAHB_DEVICE_CONTROL & USB_DEVICE_CONTROL_RXDMA_EN)
				;
			usbd_dump_control_status();
#endif
		}
	}

	if (endpoint_status != 0)
	{
		uint8_t found = 0;
		printf("usbd: isr ep_int=0x%08x\n", endpoint_status);

		/* Scan the endpoints, looking for something to do */
		for (i = 0; i < USBDevice.num_endpoints; i++)
		{
			EPSTATUS *ep = &USBDevice.ep[i];

			if (endpoint_status & ep->intmask)
			{
				found = 1;
				*USB_UDCAHB_ENDP_INT = (endpoint_status & ep->intmask);
				(*ep->eventHandler)(ep);
			}
		}
		if (!found)
		{
			printf("%s: clearing unsolicited endpoint interrupt 0x%08x\n",__func__,endpoint_status);
			*USB_UDCAHB_ENDP_INT = endpoint_status;
		}
	}

	/* Clear interrupt flag before enabling RX DMA */
	USBDevice.flags &= ~USBD_FLAG_INTERRUPT;
	/* re-enable RX DMA if needed */
	if (enable_rxdma)
	{
		/* ush_panic("usbd: RXDMA enabled in OUT ISR"); */
		usbd_rxdma_enable();
	}
}

bool canUsbWait(void)
{
	return 0;
}

#endif


