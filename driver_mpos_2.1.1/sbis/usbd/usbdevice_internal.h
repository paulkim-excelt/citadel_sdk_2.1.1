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

#ifndef _USBDEVICE_INTERNAL_H_
#define _USBDEVICE_INTERNAL_H_

#include "usbdevice.h"
#include "usbd_if.h"

/*
 * Use the endpoint intmask to control interrupts from non-control IN endpoints.
 * There's an issue with this - according to the manual, if the interrupt is
 * masked, the interrupt bit won't even be set in the status register. This is
 * fairly nonstandard, and greatly increases the odds that an interrupt will be
 * dropped (as is currently observed).
 */
#define USBD_USE_IN_ENDPOINT_INTMASK

/* Same as above, but for OUT endpoints */
#define USBD_USE_OUT_ENDPOINT_INTMASK

/*
 * Use the NAK bit to control IN-token interrupts for non-control IN endpoints.
 */
#define USBD_USE_IN_ENDPOINT_NAK

/*
 * Use pending state for IN endpoints, wait for IN token to enable transmit
 */
//#define USBD_USE_IN_PENDING_STATE

/*
 * Don't enable RX DMA unless all OUT endpoints are ready.
 */
#define USBD_RXDMA_IFF_ALL_OUT_ENDPOINTS_READY

/*
 * DMA engine modes
 */
#define DMA_MODE_PPBNDU		1	/* packet-per-buffer no descriptor update */
#define DMA_MODE_PPBDU		2	/* packet-per-buffer w/ descriptor update */ 
#define DMA_MODE_BUFFER_FILL	3	/* buffer fill mode */

#define DMA_MODE		DMA_MODE_PPBDU

#define usb_mem_block ((tUSB_MEM_BLOCK *)IPROC_BTROM_GET_USB_usb_mem_block())
#define  pusb_mem_block  ((tUSB_MEM_BLOCK *)usb_mem_block)

#define USBDevice			(pusb_mem_block->USBDevice)
#define SetupDesc			(pusb_mem_block->SetupDesc)
#define DescriptorBuffer	(pusb_mem_block->DescriptorBuffer)


/* ************************************************ */
extern const tDEVICE_DESCRIPTOR DeviceDescriptor;

extern const int stringDescriptorsNum;
extern const char * const stringDescriptors[];

uint32_t set_txdata (EPSTATUS *ep);
void set_rxdata (EPSTATUS *ep);
void usbd_clear_nak(EPSTATUS *ep);
void usbd_check_endpoints(void);
void usbd_check_endpoint(EPSTATUS *ep);
void usbd_dump(void);

extern void USBPullUp(void);
extern void USBPullDown(void);

extern void handleControlEndpointOutIntr(EPSTATUS *ep);
extern void handleControlEndpointInIntr(EPSTATUS *ep);
extern void handleDataEndpointOutIntr(EPSTATUS *ep);
extern void handleDataEndpointInIntr(EPSTATUS *ep);

extern void usbdEpOutInterrupt(EPSTATUS *ep);
extern void usbdEpInInterrupt(EPSTATUS *ep);

extern int usbdGetConfigurationDescriptor(uint8_t *buf, int req_len);
extern int usbdGetDeviceDescriptor(uint8_t *buf, int req_len);
extern int usbdGetStringDescriptor(int idx, int req_len);

#endif
