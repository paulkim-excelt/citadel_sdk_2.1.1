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

/* @file usbdevice_internal.h
 *
 * Contains Internal USBD hardware feature control flags.
 *
 *
 */


#ifndef _USBDEVICE_INTERNAL_H_
#define _USBDEVICE_INTERNAL_H_

#include "usbdevice.h"

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
#undef USBD_USE_IN_ENDPOINT_NAK

/*
 * Use pending state for IN endpoints, wait for IN token to enable transmit
 */
#define USBD_USE_IN_PENDING_STATE

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

tUSB_MEM_BLOCK usb_mem_block;
#define  pusb_mem_block  ((tUSB_MEM_BLOCK *)&usb_mem_block)

#define USBDevice		(pusb_mem_block->USBDevice)
#define SetupDesc		(pusb_mem_block->SetupDesc)
#define DescriptorBuffer	(pusb_mem_block->DescriptorBuffer)
#define USBDSSuspend		(pusb_mem_block->USBDSelectiveSuspend)

/* ************************************************ */
uint32_t set_txdata (EPSTATUS *ep);
void set_rxdata (EPSTATUS *ep);
void usbd_clear_nak(EPSTATUS *ep);
void usbd_check_endpoints(void);
void usbd_check_endpoints2(void);
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
extern int usbdGetDeviceQualifierDescriptor(uint8_t *buf, int req_len);
extern int usbdGetBOSDescriptor(uint8_t *buf, int req_len);

#endif
