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

/* @file usbinit.c
 *
 * Handlers for USB device initialization and enumeration stages.
 * FIXME Remove later
 *
 */


#include "usbdevice.h"
#include "usbregisters.h"
#include "usb.h"
#include "usbdevice_internal.h"
#include "usbd.h"
#include "usbd_descriptors.h"

#ifdef USH_BOOTROM /* AAI */
/* #include <irq.h> */
#include <board.h>
#include <toolchain/gcc.h>
#include <irq.h>
#endif

#include "stub.h"

/*Logical numbers for Bulk and interrupt end points*/
#define BULKEP_START 1
#define INTREP_START 5

/* missed in lynx_reg.h will probably by missed in citadel_socregs also */
#define CDRU_USBPHY_D_P1CTRL__afe_non_driving (0)
#define CDRU_USBPHY_D_P1CTRL__soft_resetb (1)


void usbd_trace_timestamp(uint32_t startTime);

/* 
 * Used by App to register a vendor specific handler e.g. NFC
 */
void usbd_registervenhndlr(tDEVICE *pDevice,
									usbd_handleVendRequest_handler_t hndlr)
{
	pDevice->vendor_req_handler = hndlr;
}

/*
 * Setup the specificed string descriptor response value, returning
 * the length in bytes
 */
int usbdGetStringDescriptor(int idx, int req_len)
{
	int len = 2;
	char *ptr = (char *)DescriptorBuffer;

	if (idx == 0) {
		/* language string gets special handling */
		ptr[len++] = 0x09;
		ptr[len++] = 0x04;
	}
	else {
		const char *str = stringDescriptors[idx-1];
		/* CV String is dynamically determined */
		if (idx == CV_STRING_IDX)
			str = USBDevice.cv_string;
		while (*str) {
			ptr[len++] = *str++;
			ptr[len++] = 0;
		}
	}
	DescriptorBuffer[0] = len;
	DescriptorBuffer[1] = 0x03;	/* string type */

	return (len < req_len) ? len : req_len;
}

/*
 * Build the device descriptor.
 */
int usbdGetDeviceDescriptor(uint8_t *buf, int rlen)
{
	int len = 0;
	uint8_t *ptr = buf;

	memcpy(ptr, &usbd_device_descriptor, sizeof(usbd_device_descriptor));
	((tDEVICE_DESCRIPTOR *)ptr)->idProduct = USBDevice.pid;
	/*FIXME Remove bcd should be defined in the descriptor why is this here?*/
	if (USBDevice.num_interfaces == 1)
	{
		((tDEVICE_DESCRIPTOR *)ptr)->bcdDevice = 0x0102;
	}
	ptr += sizeof(usbd_device_descriptor);

	len = ptr - buf;
	return (len < rlen) ? len : rlen;
}

/*
 * Build the BOS descriptor.
 */
int usbdGetBOSDescriptor(uint8_t *buf, int rlen)
{
	uint8_t *ptr = buf;

	ARG_UNUSED(rlen);

	memcpy(ptr, &usbd_bos_descriptor, sizeof(usbd_bos_descriptor));
	memcpy(ptr+sizeof(usbd_bos_descriptor), &usbd_2_0_ext_descriptor, sizeof(usbd_2_0_ext_descriptor));
	return (sizeof(usbd_bos_descriptor) + sizeof(usbd_2_0_ext_descriptor));
}

/*
 * Build the device qualifier descriptor.
 */
int usbdGetDeviceQualifierDescriptor(uint8_t *buf, int rlen)
{
	int len = 0;
	uint8_t *ptr = buf;

	memcpy(ptr, &usbd_device_qualifier_descriptor, sizeof(usbd_device_qualifier_descriptor));
	((tDEVICE_DESCRIPTOR *)ptr)->idProduct = USBDevice.pid;
	/*FIXME Remove bcd should be defined in the descriptor why is this here?*/
	if (USBDevice.num_interfaces == 1)
	{
		((tDEVICE_DESCRIPTOR *)ptr)->bcdDevice = 0x0102;
	}
	ptr += sizeof(usbd_device_qualifier_descriptor);

	len = ptr - buf;
	return (len < rlen) ? len : rlen;
}

/*
 * Helper routine to write an endpoint descriptor entry
 * returns the length written (7 bytes)
 */
int usbd_build_endpoint_descriptor(EPSTATUS *ep, uint8_t *buf)
{
	uint16_t epMaxPktSize = ep->maxPktSize;

	/* bLength */
	buf[0] = DSCT_ENDPOINT_LEN;

	/* bDescriptorType */
	buf[1] = DSCT_ENDPOINT;
	/* Bit 3...0: The endpoint number */
	/* bEndpointAddress */
	/* Bit 6...4: Reserved */
	/* Bit 7: Direction */
	if (is_in(ep))
		buf[2] = 0x80 | ep->number;
	else
		buf[2] = ep->number;

	/* bmAttributes */
	if (is_intr(ep))
		buf[3] = EP_INTR;
	else if (is_bulk(ep))
		buf[3] = EP_BULK;

	/* wMaxPacketSize */
	/* maxpacketsize could be larger than 0xff in USB2.0 mode!!! */
	/* Bits 10..0 = max. packet size (in bytes) */
	/* Bits 12..11 = number of additional transaction opportunities per micro-frame */
	/* Bits 15..13 are reserved and must be set to zero */
	buf[4] = (epMaxPktSize) & 0xff;
	buf[5] = (epMaxPktSize >> 8 );

	/* bInterval */
	if (is_intr(ep))
		/* 1ms */
		buf[6] = 1;
	else
		buf[6] = 0;

	return 7;
}

/*
 * Build the configuration descriptor based on what interfaces are enabled.
 */
int usbdGetConfigurationDescriptor(uint8_t *buf, int rlen)
{
	uint32_t i, j;
	int len = 0;
	uint32_t interfaces = 0;
	uint8_t *ptr = buf;

	memcpy(ptr, &usb_configuration_descriptor, sizeof(usb_configuration_descriptor));
	ptr += sizeof(usb_configuration_descriptor);

	/* For each interface, write out the interface descriptor,
	 * followed by the associated endpoint descriptors.
	 */
	for (i = 0; i < USBDevice.num_interfaces; ++i) {
		usbd_interface_t *interface = &USBDevice.interface[i];

		if (interface->get_interface_descriptor) {
				ptr += (*interface->get_interface_descriptor)((char*)ptr, i);
		}
		else {
			memcpy(ptr, interface->descriptor, interface->descriptor_len);
			if (ptr[2] == 0)	// Leave WBDI/NFP at interface 3
			{
				// printf("%s: Dynamic setting the interface to %d \n",__FUNCTION__,i);
				ptr[2] = i;
			}
			ptr += interface->descriptor_len;
		}
		interfaces++;

		for (j = 0; j < NUM_ENDPOINTS_PER_INTERFACE; ++j) {
			EPSTATUS *ep = interface->ep[j];

			if (ep) {
				len = usbd_build_endpoint_descriptor(ep, ptr);
				ptr += len;
			}
		}
	}

	/* Program the length */
	len = ptr - buf;
	buf[2] = len & 0xff;
	buf[3] = (len >> 8) & 0xff;

	/* program the number of interfaces */
	buf[4] = interfaces;

	return (len < rlen) ? len : rlen;
}

EPSTATUS *usbd_find_endpoint(int key, int flags)
{
	int i;
	int interface = -1;
	usbd_interface_t *interfaces = &USBDevice.interface[0];

	/*was the interface type configured */
	for(i=0;i<NUM_INTERFACES;i++)
	{
		interfaces = &USBDevice.interface[i];
		if(key == interfaces->type)
		{
			printf("interface found %d\n",i);
			interface = i;
			break;
		}
	}

	if(key!= interfaces->type)
		return NULL;

	for (i = 0; i < ENDP_NUMBER; ++i) {
		EPSTATUS *ep = &USBDevice.ep[i];
		if ((ep->interface == interface) && (ep->flags == flags))
		{
			printf("find ep ep->number: %d\n",ep->number);
			return ep;
		}
	}

	post_log("ERROR: endpoint not found\n");
	return NULL;
}


void USBPullUp(void)
{

}


void USBPullDown(void)
{
	phx_set_timer_milli_seconds(USB_BLOCK_TIMER, 600, TIMER_MODE_FREERUN, TRUE);
	while (phx_timer_get_current_value(USB_BLOCK_TIMER));
}


void usbd_udc20_init(int regno, EPSTATUS *ep)
{
	int config = CV_CONF_NUM;
	int alternate = 0;
	uint32_t dwSet = ep->number
			| (config << USB_UDC_ENDP_CONFIG_SHIFT)
			| (ep->interface << USB_UDC_ENDP_INTERFACE_SHIFT)
			| (alternate << USB_UDC_ENDP_ALTERNATE_SHIFT)
			| (ep->maxPktSize << USB_UDC_ENDP_MAX_PKTSIZE_SHIFT);

	if (is_out(ep))
		dwSet |= USB_UDC_ENDP_DIR_OUT;
	if (is_in(ep))
		dwSet |= USB_UDC_ENDP_DIR_IN;
	switch (ep->flags & MASK_TYPE) {
	case CTRL_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_CONTROL;
		break;
	case BULK_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_BULK;
		break;
	case INTR_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_INTERRUPT;
		break;
	}
	*USB_UDC_ENDP_NE(regno) = dwSet;
}

void usbd_buffer_init(EPSTATUS *ep)
{

	usb_dma_info_t *pDmaInfo = &ep->dma;
	tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

	pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_DMA_DONE |
				      USB_DMA_DESC_LAST;
	pDesc->pBuffer = pDmaInfo->pBuffer;
	pDesc->pNext = NULL;  // we are currenting handle one buffer trunk


	if (is_in(ep))
	{
		ep->intmask |= USB_DEVICE_ENDP_INT_IN(ep->number);
		ep->eventHandler = usbdEpInInterrupt;

#ifdef CONFIG_DATA_CACHE_SUPPORT
		clean_n_invalidate_dcache_by_addr ((u32_t) ep,
											 sizeof(EPSTATUS));
		clean_n_invalidate_dcache_by_addr ((u32_t) pDesc,
											 sizeof(tUSBDMA_DATA_DESC));
#endif

		*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;
		/*SYS_LOG_DBG(" IN epno:%d intmask:0x%08x USB_UDCAHB_IN_ENDP_DESCRIPTOR:0x%08x  pdesc:0x%08x",(unsigned int)ep->number,(unsigned int)ep->intmask,(unsigned int)*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number),(unsigned int)pDesc); */

		if (is_ctrl(ep))
		{
			USBDevice.controlIn = ep;
		}

		USBDevice.delayed_CNAK_mask &= ~USB_DEVICE_ENDP_INT_IN(ep->number);
	}

	if (is_out(ep))
	{
		ep->intmask |= USB_DEVICE_ENDP_INT_OUT(ep->number);
		ep->eventHandler = usbdEpOutInterrupt;


#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Flush to SRAM */
		clean_dcache_by_addr ((u32_t) ep, sizeof(EPSTATUS));
		clean_dcache_by_addr ((u32_t) pDesc, sizeof(tUSBDMA_DATA_DESC));
#endif

		*USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;
		/* SYS_LOG_DBG(" OUT epno:%d intmask:0x%08x USB_UDCAHB_OUT_ENDP_DESCRIPTOR:0x%08x pdesc:0x%08x",(unsigned int)ep->number,(unsigned int)ep->intmask,(unsigned int)*USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number),(unsigned int)pDesc); */

		if (is_ctrl(ep))
		{
			USB_DMA_DESC_SET_READY((tUSBDMA_SETUP_DESC *)&SetupDesc);

#ifdef CONFIG_DATA_CACHE_SUPPORT
			clean_n_invalidate_dcache_by_addr ((u32_t) &SetupDesc,
												 sizeof(tUSBDMA_SETUP_DESC));
			clean_n_invalidate_dcache_by_addr ((u32_t)pusb_mem_block->ctrl_buffer,
												 ep->maxPktSize);
#endif
			*USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number) = (uint32_t)&SetupDesc;
			USBDevice.controlOut = ep;
		}

		USBDevice.delayed_CNAK_mask &= ~USB_DEVICE_ENDP_INT_OUT(ep->number);
	}
}

void initUDCAHBGlobalRegister(void)
{
	/* device config register */
	*USB_UDCAHB_DEVICE_CONFIG = USB_DEVICE_CONFIG_SPD_HS
				/* 	| USB_DEVICE_CONFIG_SPD_FS */
					| USB_DEVICE_CONFIG_LPM_EN
					| USB_DEVICE_CONFIG_LPM_AUTO
					| USB_DEVICE_CONFIG_REMOTE_WKUP
					| USB_DEVICE_CONFIG_PHY_8BIT;

	/* device control register */
	*USB_UDCAHB_DEVICE_CONTROL = USB_DEVICE_CONTROL_LITTLEENDIAN
#if (DMA_MODE == DMA_MODE_BUFFER_FILL)
					| USB_DEVICE_CONTROL_BUFF_FILL
#endif
				/* TODO: scale down for emulation only */
				/* 	| USB_DEVICE_CONTROL_SCALE_DOWN */
					| USB_DEVICE_CONTROL_MODE_DMA
					| USB_DEVICE_CONTROL_RXDMA_EN
					| USB_DEVICE_CONTROL_TXDMA_EN;

	/* unmasked desired interrupts */
	*USB_UDCAHB_DEVICE_INTMASK = ~(USB_DEVICE_INT_RESET
					| USB_DEVICE_INT_LPM_TKN
				/* 	| USB_DEVICE_INT_SLEEP */
				/* 	| USB_DEVICE_INT_EARLY_SLEEP */
					| USB_DEVICE_INT_IDLE
					| USB_DEVICE_INT_SUSPEND
					| USB_DEVICE_INT_SET_CONFIG
				/* 	| USB_DEVICE_INT_ENUM_SPEED */ /* add for debug */
				/* 	| USB_DEVICE_INT_SOF */  /* add for debug */
					| USB_DEVICE_INT_SET_INTERFACE);

	/* clear all pending interrupts */
	/* TODO may not needed, WSL */ 
	printf("------------ USB_UDCAHB_ENDP_INT 0x%08x\n", (u32_t)*USB_UDCAHB_ENDP_INT);
	*USB_UDCAHB_ENDP_INT = 0xffffffff;
	/* enable all endpoint interrupts */
	*USB_UDCAHB_ENDP_INTMASK = 0;
}

void usbd_pid_config (tDEVICE *pDevice)
{
	//USB should not be concerned with the Fp sensor.
	//App init is a better placed to check and update the pid and vid.
	//configure it once during lifetime of program
	pDevice->pid = PID_FP_NONE;
	pDevice->cv_string = CV_FP_NONE;
	return;
}

unsigned int usbd_get_pid (void)
{
	return (uint32_t) USBDevice.pid;
}

void usbd_endpoint_config (EPSTATUS *ep, uint32_t number,
									 uint32_t flags, uint32_t interface)
{
	uint32_t dwSet = 0;

	SYS_LOG_DBG(" number = %d, flags =%d, interface=%d ",
				 (u32_t)number, (u32_t)flags, (u32_t)interface);
	ep->number = number;
	ep->flags = flags;
	ep->interface = interface;
	ep_set_idle(ep);
	ep->maxPktSize = 0;
	ep->intmask = 0;
	ep->appCallback = NULL;
	ep->eventHandler = NULL;
	ep->dma.transferred = 0;
	ep->dma.len = USB_MAX_DMA_BUF_LEN;
	//	ep->dma.len = 0x40; //CV_USB_MAXPACKETSIZE;
	ep->dma.pBuffer = NULL;

	if (is_ctrl(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_CONTROL;
		ep->maxPktSize = USBD_CNTL_MAX_PKT_SIZE;
	}
	if (is_bulk(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_BULK;
		ep->maxPktSize = USBD_BULK_MAX_PKT_SIZE;
	}
	if (is_intr(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_INTERRUPT;
		ep->maxPktSize = USBD_INTR_MAX_PKT_SIZE; 
	}

	/* Largest packet type supported is BULK */
#if 0
	if(ep->dma.len < CV_USB_MAXPACKETSIZE)
		ep->dma.len = CV_USB_MAXPACKETSIZE;
#endif

	/* maxpacket size is in words */
	if (is_in(ep)) {
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) = dwSet;
		*USB_UDCAHB_IN_ENDP_MAXPKTSIZE(ep->number) = ep->maxPktSize;
		*USB_UDCAHB_IN_ENDP_BUFSIZE(ep->number) = ep->maxPktSize >> 2;
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |=
											 USB_ENDP_CONTROL_FLUSH_TXFIFO;
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) &=
											 ~USB_ENDP_CONTROL_FLUSH_TXFIFO;

#ifdef USBD_USE_IN_ENDPOINT_NAK
		/* Nothing to send yet, so set NAK */
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_SET_NAK;
#endif
	}

	if (is_out(ep)) {
		*USB_UDCAHB_OUT_ENDP_CONTROL(ep->number) = dwSet;
		*USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(ep->number) =
								 ((ep->maxPktSize >> 2) << 16) | ep->maxPktSize;
	}

	/* If it's a bulk or interrupt endpoint, store it in the corresponding
	 * interface datastructure.
	 */
	if (!is_ctrl(ep)) {
		int i;

		for (i = 0; i < NUM_ENDPOINTS_PER_INTERFACE; ++i) {
			if (USBDevice.interface[interface].ep[i] == ep)
				break;
			if (!USBDevice.interface[interface].ep[i]) {
				USBDevice.interface[interface].ep[i] = ep;
				break;
			}
		}
	}
}

int usbd_reginterface(tDEVICE *pDevice,usbd_interface_t intf)
{
	int i;
	int bulk = BULKEP_START;
	int intr = INTREP_START;
	int interface = 0;
	EPSTATUS *ep = &pDevice->ep[0];
	usbd_interface_t *interfaces = &pDevice->interface[0];

	if(pDevice->bulk_epno)
		bulk = pDevice->bulk_epno;
	if(pDevice->intr_epno)
		intr = pDevice->intr_epno;

	/*Find a free interface */
	for (interface = 0; interface < NUM_INTERFACES; ++interface)
	{
		if(interfaces[interface].type == USBD_INVALID_INTERFACE)
			break;
	}

	if(interfaces[interface].type!=0xFF)
		/*all interface enties occupied !!! */
		return 1;

	post_log("Enumerating interface %d\n",interface);

	/*setup the interface parameters */
	interfaces[interface].type = intf.type;
	interfaces[interface].class_request_handler = intf.class_request_handler;
	interfaces[interface].reset = intf.reset;
	interfaces[interface].descriptor = intf.descriptor;
	interfaces[interface].descriptor_len = intf.descriptor_len;

	/*find a free ep entry */
	for (i = 0; i < ENDP_NUMBER; ++i)
	{
		ep = &pDevice->ep[i];
		if(ep->interface == USBD_INVALID_INTERFACE)
			break;
	}

	if(ep->interface!=USBD_INVALID_INTERFACE)
	{
		/*all ep enties occupied !!! */
		return 1;
	}

	printf("Setting up eps for interface %d\n",interface);
	/* Got a free entry */

	/* Should be made more dynamic ok for now */
	if(ep->interface == USBD_INVALID_INTERFACE)
	{
		/*Setup the BULK IN BULK OUT ep */
		usbd_endpoint_config(ep++, bulk,    BULK_TYPE | IN_DIR, interface);
		usbd_endpoint_config(ep++, bulk++,  BULK_TYPE | OUT_DIR, interface);
		/*Setup the INTR IN ep */
		usbd_endpoint_config(ep++, intr++,  INTR_TYPE | IN_DIR, interface);
	}

	/* save the int_ep and bulk_ep numbers allocated for the
	 * next interface setup
	 */
	pDevice->bulk_epno = bulk;
	pDevice->intr_epno = intr;

	/*Update and save the num_interface */
	pDevice->num_interfaces += 1;

	/* Calculate and save the number of endpoint entries used*/
	pDevice->num_endpoints = ep - &pDevice->ep[0];
	pDevice->delayed_CNAK_mask = 0;

	/* Calculate the number of endpoint entries used */
	pDevice->num_endpoints = ep - &pDevice->ep[0];

	post_log("*** allocated bulkno %d intrno %d \n", pDevice->bulk_epno,
			 pDevice->intr_epno);
	post_log("*** pDevice->num_interfaces:%d \n", pDevice->num_interfaces);
	post_log("*** pDevice->num_endpoints :%d \n", pDevice->num_endpoints);

	return 0;
}

/* Initialization Functions Begin */
void usbd_config(tDEVICE *pDevice)
{
	int i;

	EPSTATUS *ep;
	usbd_interface_t *interfaces = &pDevice->interface[0];

	/*Set the num_interfaces to 0*/
	pDevice->num_interfaces = 0;

	/*Clear all the stale interface*/
	memset(interfaces, 0, sizeof(*interfaces) * NUM_INTERFACES);

	/*first time allocated number is the starting logical numbers */
	pDevice->bulk_epno = BULKEP_START;
	pDevice->intr_epno = INTREP_START;

	/*set interface entries to INVALID */
	for (i = 0; i < NUM_INTERFACES; ++i)
	{
		usbd_interface_t *interfaces = &pDevice->interface[i];
		interfaces->type = USBD_INVALID_INTERFACE;
	}

	/*set ep entries to INVALID */
	for (i = 0; i < ENDP_NUMBER; ++i)
	{
		EPSTATUS *ep = &pDevice->ep[i];
		ep->interface = USBD_INVALID_INTERFACE;
	}

	/*CTRL always 0 */
	ep = &pDevice->ep[0];
	usbd_endpoint_config(ep++, 0, CTRL_TYPE | IN_DIR, 0);
	usbd_endpoint_config(ep++, 0, CTRL_TYPE | OUT_DIR, 0);

	/* end of control endpoint configuration */

	/* Calculate the number of endpoint entries used and save*/
	pDevice->num_endpoints = ep - &pDevice->ep[0];
	pDevice->delayed_CNAK_mask = 0;

	post_log("*** pDevice->num_interfaces:%d \n" ,pDevice->num_interfaces);
	post_log("*** pDevice->num_endpoints :%d \n",pDevice->num_endpoints);

}

void usbd_hardware_init(tDEVICE *pDevice)
{
	int i;
	int regno = 0;
	EPSTATUS *ep;

	initUDCAHBGlobalRegister();

	for (i = 0; i < 16; ++i)
		*USB_UDC_ENDP_NE(i) = 0;
	for (i = 0; i < pDevice->num_endpoints; i++)
	{
		ep = &pDevice->ep[i];

		if (ep->interface == 0xff)
			break;

		/* Now init the buffers */
		usbd_buffer_init(ep);

		/* Only 2nd CTRL entry gets a UDC20 register */
		if (is_ctrl(ep) && !i)
			continue;
		usbd_udc20_init(regno++, ep);
	}

	/* tell UDC-AHB the programming of UDC is done */
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_UDCCSR_DONE;
	/* ready to receive control messages */
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_RXDMA_EN;
}

void usbd_hardware_reinit(tDEVICE *pDevice)
{
	int i;
	SYS_LOG_DBG(" ----- interface_state: %d flags: %x",
				 USBDevice.interface_state,
				 (unsigned int)USBDevice.flags);

	ENTER_CRITICAL();

	*USB_UDCAHB_DEVICE_CONTROL &= ~USB_DEVICE_CONTROL_RXDMA_EN;

	usbd_pid_config(pDevice);

	/* Re-set the buffers for non-control endpoints. */
	for (i = 0; i < pDevice->num_endpoints; i++) {
		EPSTATUS *ep = &pDevice->ep[i];

		usbd_endpoint_config(ep, ep->number, ep->flags, ep->interface);
		usbd_buffer_init(ep);
	}
	/* No point of for loop here as we have only one CTRL_IN */
	for (i = 0; i < pDevice->num_endpoints; i++) {
		EPSTATUS *ep = &pDevice->ep[i];

		if (is_ctrl(ep) && is_out(ep))
		{
			memset((void *)(pusb_mem_block->ctrl_buffer), 0x0, ep->maxPktSize);
			recvEndpointData(ep, pusb_mem_block->ctrl_buffer, ep->maxPktSize);
		}
	}
	EXIT_CRITICAL();

	pDevice->interface_state = USBD_STATE_RESET;

}


void usbd_phyInit(void)
{
	uint32_t regVal;

#ifndef USH_BOOTROM /* SBI */
	/* SBL will shutdown the USB PHY always */
	/* See iproc_usb_device_shutdown */
	/* Enable USB */
	regVal = AHB_READ_SINGLE(CDRU_SW_RESET_CTRL, AHB_BIT32);
	regVal |= (1 << CDRU_SW_RESET_CTRL__usbd_sw_reset_n);
	AHB_WRITE_SINGLE(CDRU_SW_RESET_CTRL, regVal, AHB_BIT32);
#endif

	/* Disable LDO */
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL2, AHB_BIT32);
	regVal |= (1 << CDRU_USBPHY_D_CTRL2__ldo_pwrdwnb);
	regVal |= (1 << CDRU_USBPHY_D_CTRL2__afeldocntlen);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL2, regVal, AHB_BIT32);

	/* Disable Isolation */
	phx_blocked_delay_us(500);
	regVal = AHB_READ_SINGLE(CRMU_USBPHY_D_CTRL, AHB_BIT32);
	regVal &= ~(1 << CRMU_USBPHY_D_CTRL__phy_iso);
	AHB_WRITE_SINGLE(CRMU_USBPHY_D_CTRL, regVal, AHB_BIT32);

	/* Enable IDDQ current */
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL2, AHB_BIT32);
	regVal &= ~(1 << CDRU_USBPHY_D_CTRL2__iddq);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL2, regVal, AHB_BIT32);

	/* Disable PHY reset */
	phx_blocked_delay_us(150);
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL1, AHB_BIT32);
	regVal |= (1 << CDRU_USBPHY_D_CTRL1__resetb);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL1, regVal, AHB_BIT32);

	/* Suspend enable */
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL1, AHB_BIT32);
	regVal |= (1 << CDRU_USBPHY_D_CTRL1__pll_suspend_en);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL1, regVal, AHB_BIT32);

	/* UTMI CLK for 60 Mhz */
	phx_blocked_delay_us(150);
	regVal = AHB_READ_SINGLE(CRMU_USBPHY_D_CTRL, AHB_BIT32) & 0xfff00000;
	regVal |= 0xec4ec; //'d967916
	AHB_WRITE_SINGLE(CRMU_USBPHY_D_CTRL, regVal, AHB_BIT32);
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL1, AHB_BIT32) & 0xfc000007;
	regVal |= (0x03 << CDRU_USBPHY_D_CTRL1__ka_R); // ka 'd3
	regVal |= (0x03 << CDRU_USBPHY_D_CTRL1__ki_R); // ki 'd3
	regVal |= (0x0a << CDRU_USBPHY_D_CTRL1__kp_R); // kp 'hA
	regVal |= (0x24 << CDRU_USBPHY_D_CTRL1__ndiv_int_R ); //ndiv 'd36
	regVal |= (0x01 << CDRU_USBPHY_D_CTRL1__pdiv_R ); //pdiv 'd1
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL1, regVal, AHB_BIT32);

	/* Disable PLL reset */
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_CTRL1, AHB_BIT32);
	regVal |= (1 << CDRU_USBPHY_D_CTRL1__pll_resetb);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_CTRL1, regVal, AHB_BIT32);

	/* Disable software reset */
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_P1CTRL, AHB_BIT32);
	regVal |= (1 << CDRU_USBPHY_D_P1CTRL__soft_resetb);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_P1CTRL, regVal, AHB_BIT32);

#ifndef CONFIG_CITADEL_EMULATION
	/* PLL locking loop */
	do {
		regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_STATUS, AHB_BIT32);
	} while ((regVal & (1 << CDRU_USBPHY_D_STATUS__pll_lock)) == 0);
	printf("USB device PHY PLL locked\n");

	/* Disable non-driving */
	phx_blocked_delay_us(10);
	regVal = AHB_READ_SINGLE(CDRU_USBPHY_D_P1CTRL, AHB_BIT32);
	regVal &= ~(1 << CDRU_USBPHY_D_P1CTRL__afe_non_driving);
	AHB_WRITE_SINGLE(CDRU_USBPHY_D_P1CTRL, regVal, AHB_BIT32);

#endif //CONFIG_CITADEL_EMULATION


	/* Flush Device RxFIFO for stale contents*/
	/* Required for enumlation setup good change to have */
	regVal = *USB_UDCAHB_DEVICE_CONTROL;
	regVal |= (1 << DEVCTRL__DEVNAK);
	*USB_UDCAHB_DEVICE_CONTROL = regVal;

	regVal = *USB_UDCAHB_DEVICE_CONTROL;
	regVal |= (1 << DEVCTRL__SRX_FLUSH);
	*USB_UDCAHB_DEVICE_CONTROL = regVal;

	regVal = *USB_UDCAHB_DEVICE_CONTROL;
	regVal &= ~(1 << DEVCTRL__DEVNAK);
	*USB_UDCAHB_DEVICE_CONTROL = regVal;

	phx_blocked_delay_us(1);


}

/*
 * usbDeviceInit (void)
 *
 * Entry point for USB device initialization.
 * Set up descriptors, interfaces, internal data structures, etc
  */
void usbDeviceInit(void)
{
	usbd_phyInit();

	memset(pusb_mem_block, 0, sizeof(*pusb_mem_block));
	get_ms_ticks(&pusb_mem_block->resetTime);

#if 0
	usbd_hardware_init(&USBDevice);
#endif

}

/*
 * Notify USBD clients of a 'reset' event
 */
void usbd_client_reset(void)
{
	uint32_t i;

	printf("usbd_client_reset \n");

	for (i = 0; i < USBDevice.num_interfaces; ++i) {
		usbd_interface_t *interface = &USBDevice.interface[i];

		if (interface->reset)
			(*interface->reset)();
	}
}

/*
 * Idle handler called from sched_cmd.c
 * used to handle USB bus reset and other events.
 */
void usbd_idle(void)
{
	uint32_t flags;
	volatile uint32_t reg;

	ENTER_CRITICAL();
	flags = USBDevice.flags;
	USBDevice.flags = 0;
	EXIT_CRITICAL();

	ARG_UNUSED(reg);

#ifndef USBD_USE_IN_ENDPOINT_INTMASK
	reg = *USB_UDCAHB_ENDP_INTMASK;
	if (reg != 0) {
		printf("usbd: damaged endpoint intmask 0x%x\n", reg);
		//ush_panic("usbd damaged endpoint intmask");
	}
#endif
	reg = *USB_UDCAHB_DEVICE_INT;

#ifdef USBD_RXDMA_IFF_ALL_OUT_ENDPOINTS_READY
	usbd_rxdma_enable();
#endif
	usbd_check_endpoints();

	/* now handle the flags as needed */
	if (flags & USBD_FLAG_RESTART) {
		printf("USBD_FLAG_RESTART\n");
		USBPullDown();
		phx_blocked_delay_ms(100);

		/* Reset the state */
		printf("usbd: Reset state\n");
		USBDevice.interface_state= USBD_STATE_RESET;

		USBPullUp();
	}

	if (flags & USBD_FLAG_RESET) {
		usbd_trace_timestamp(pusb_mem_block->resetTime);
		printf("USBD_FLAG_RESET\n");
		get_ms_ticks(&pusb_mem_block->resetTime);

#if 1
		/* reset the USBD dma state and endpoints */
		usbd_hardware_reinit(&USBDevice);
		/* notify the client(s) */
		usbd_client_reset();
#endif

	}

#ifdef USH_BOOTROM  /*AAI */
	/*
	 * This should never be non-zero outside of the ISR
	 */
	reg = *USB_UDCAHB_ENDP_INT;
	if (reg != 0) {
		/*
		 * There's a potential race condition in reading USB_UDCAHB_ENDP_INT
		 * from task mode
		 * keep track of how long this bit is set. If cnt==0, then it might
		 * have been the race.
		 * If cnt>0, then something is definitely wrong
		 */
		if (*USB_UDCAHB_ENDP_INT) {
			/* don't even complain unless we see it twice */
			printf("usbd: endpoint interrupts pending but not serviced 0x%x\n",
					 reg);
		}
	}
#endif
}

void usbd_trace_timestamp(uint32_t startTime)
{
	ARG_UNUSED(startTime);

#if 0 //FIXME: LYCX_CS_PORT
	sprintf((char *)pusb_mem_block->tmp_buf, "%d: ", cvGetDeltaTime(startTime));
	uart_print_string(pusb_mem_block->tmp_buf);
#endif

}

void usbd_idle2(void)
{
	usbd_check_endpoints2();
}

void usbDeviceDumpEndpoints()
{
	usbd_dump();
}

/*
 * usbDeviceStart()
 *
 * Pull up to indicate to host that we're ready.
 */
void usbDeviceStart(void)
{
	usbd_client_reset();
	USBPullUp();
}

//
// Resumes the device if selectively suspended, otherwise does nothing.
//
void usbdResume(void)
{
	volatile uint32_t reg;
	uint32_t status;
	
	// If selectively suspended, resume the device
	status = *USB_UDCAHB_DEVICE_STATUS;
	
	if (USBDSSuspend.suspend_state)
	{
		// Attempt to resume
		reg = *USB_UDCAHB_DEVICE_CONTROL;
		reg |= USB_DEVICE_CONTROL_RESUME;		/* Set resume signaling bit */
		*USB_UDCAHB_DEVICE_CONTROL = reg;

		/* Wait until suspened status is cleared */
		while (status & USB_DEVICE_STATUS_SUSPENDED)
		{
			phx_blocked_delay_ms(5);
			status = *USB_UDCAHB_DEVICE_STATUS;
		}
		
		/* Clear resume bit */
		reg = *USB_UDCAHB_DEVICE_CONTROL;
		reg &= ~USB_DEVICE_CONTROL_RESUME;		/* Clear resume bit */
		*USB_UDCAHB_DEVICE_CONTROL = reg;
		
		USBDSSuspend.suspend_state = 0;
		USBDSSuspend.idle_state = 0;
	}
	return;
}

int usbdResumeWithTimeout(void)
{
	volatile uint32_t reg;
	uint32_t status;
	uint32_t loop=0;
	/*printf("USBDSSuspend.suspend_state = %x\n", USBDSSuspend.suspend_state);*/
	/* If selectively suspended, resume the device */
	status = *USB_UDCAHB_DEVICE_STATUS;

	if (USBDSSuspend.suspend_state)
	{
		// Attempt to resume
		reg = *USB_UDCAHB_DEVICE_CONTROL;
		reg |= USB_DEVICE_CONTROL_RESUME;	/* Set resume signaling bit */
		*USB_UDCAHB_DEVICE_CONTROL = reg;

		/* Wait until suspened status is cleared */

		while (status & USB_DEVICE_STATUS_SUSPENDED)
		{
			phx_blocked_delay_ms(5);
			status = *USB_UDCAHB_DEVICE_STATUS;
			if( loop++ == 10) 
			{
				printf("UsbdResume Failed\n");
				return 0;
			}
		}

		// Clear resume bit
		reg = *USB_UDCAHB_DEVICE_CONTROL;
		reg &= ~USB_DEVICE_CONTROL_RESUME;	// Clear resume bit
		*USB_UDCAHB_DEVICE_CONTROL = reg;

		USBDSSuspend.suspend_state = 0;
		USBDSSuspend.idle_state = 0;
	}

	return 1;
}

void usbd_enable_irq(void)
{
#ifdef USH_BOOTROM  /* AAI */
    irq_enable(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT));
#endif
}

void usbd_disable_irq(void)
{
#ifdef USH_BOOTROM /* AAI */
    irq_disable(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT));
#endif
}

void usbd_clear_allpend(void)
{
#ifdef USH_BOOTROM /* AAI */
	/* clear all pending interrupts */
	SYS_LOG_DBG("------USB_UDCAHB_ENDP_INT current pend 0x%08x",
				 (unsigned int)*USB_UDCAHB_ENDP_INT);
	*USB_UDCAHB_ENDP_INT = 0xffffffff;
#endif
}
void usbd_isr(void *unused)
{
	ARG_UNUSED(unused);

	printf(" usbd_isr_idle\n");
	usbdevice_isr(NULL);

	/* TBD move to a thread once pm is available*/
	usbd_idle();

	/* TBD confirm if the bit is self clearing */
	/* irq_clear_pending(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT)); */

}

void usbd_register_isr(void)
{

#ifdef USH_BOOTROM /* AAI */
	IRQ_CONNECT(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT),
				CONFIG_USBD_IRQ_PRI, usbd_isr, NULL, 0);

/* IRQ_CONNECT(SPI_IRQ(CHIP_INTR__IOSYS_USBD_INTERRUPT),
 *				CONFIG_USBD_IRQ_PRI, usbdevice_isr, NULL, 0);
 */
#endif
}
