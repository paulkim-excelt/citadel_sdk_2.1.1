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

/* @file usb.c
 *
 * USB device Host request handers
 *
 *
 */

#include "usbdevice_internal.h"
/* #include "usbdebug.h" */
#include "phx.h"
#include "usb.h"
#include "usbdesc.h"
#include "usbdevice.h"
#include "usbregisters.h"
#include "usbd_descriptors.h"

#ifdef USH_BOOTROM /*AAI */
#include <toolchain/gcc.h>
#endif

#include "stub.h"

/*
 * TODO:
 *
 * Track enumeration process
 * powered/reset
 * address set
 * configured
 *
 * until configured point, can only draw up to 100mA from Vbus. After configured,
 * may draw up to amount specified in configuration.
 *
 * Note that SET_ADDRESS has a required response time (50ms)
 */
void stallEndpoint(uint8_t EndpointNum)
{
	ARG_UNUSED(EndpointNum);
}

void resetEndpointPacketId(uint8_t EpNum)
{
	ARG_UNUSED(EpNum);
}

void enableRemoteWake(bool wake_ok)
{
	ARG_UNUSED(wake_ok);
}

void UsbShutdown(void)
{
	USBPullDown();

	/* Reset the state */
	//printf("usbd: Shutdown state\n");
	USBDevice.interface_state = USBD_STATE_SHUTDOWN;
	USBDevice.enum_count = 0;
}

bool UsbResume(uint8_t ucState)
{
	ARG_UNUSED(ucState);
	return FALSE;
}

void clearFeature(SetupPacket_t *req)
{
	ARG_UNUSED(req);
}

void getConfiguration(SetupPacket_t *req)
{
	ARG_UNUSED(req);
}

void getDescriptor(SetupPacket_t *req)
{
	uint8_t idx;
	int dlen;
	int rlen;

	// Get required values from SetupPacket_t
	rlen = req->wLength;
	idx = (uint8_t) req->wValue;

	// Determine descriptor type
	// Send requested amount of requested descriptor
	// (USB 2.0 9.6.2 & Table 9-8)
	switch ((uint8_t) (req->wValue>>8)) {
	case DSCT_DEVICE:
		printk(" GET_DESCRIPTOR (device) \n");
		dlen = usbdGetDeviceDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		return;

	case DSCT_CONFIG:
		printk(" GET_DESCRIPTOR (config) \n");
		dlen = usbdGetConfigurationDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		return;

	case DSCT_STRING:
		printk(" GET DESCRIPTOR (string %d) \n", idx);
		if (idx < stringDescriptorsNum)
		{
			dlen = usbdGetStringDescriptor(idx, rlen);
			sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		}
		else
		{
			if (idx == 0xEE)
			{
				/* Send Stall on MS OS Descriptor */
				trace_enum("MS OS String Descriptor, stall \n");
				stallControlEP(USBDevice.controlIn);
			}
			//stallEndpoint(0);
			
		}
		return;

	case DSCT_BOS:
		printk(" GET_DESCRIPTOR (bos) \n");
		//stallControlEP(USBDevice.controlIn);
		dlen = usbdGetBOSDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		return;

	case DSCT_DEVICE_QUALIFIER:
		printk("GET_DESCRIPTOR (device qualifier) \n");
	       	dlen = usbdGetDeviceQualifierDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		return;

	case DSCT_OTHER_SPEED_CONF:
		printk(" GET_DESCRIPTOR (other speed configuration) \n");
		dlen = usbdGetConfigurationDescriptor(DescriptorBuffer, rlen);
		DescriptorBuffer[1] = DSCT_OTHER_SPEED_CONF;
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		return;
	break;
	default:
		printk("unknown descriptor request 0x%02x\n", (uint8_t) (req->wValue>>8));
	}

	/* default reply */
	sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
}

void getInterface(SetupPacket_t *req)
{
	ARG_UNUSED(req);
	printk(" getInterface \n");
}

void stallRequestedEndpoint(uint16_t wIndex)
{
	ARG_UNUSED(wIndex);
	printk(" stallRequestedEndpoint \n");
}

void getStatus(SetupPacket_t *req)
{
	ARG_UNUSED(req);
	printk(" GET_STATUS\n");

}

void setAddress(SetupPacket_t *req)
{
	ARG_UNUSED(req);
	printk(" SET_ADDRESS\n");
}

void setConfiguration(SetupPacket_t *req)
{
	ARG_UNUSED(req);

	/* Init the state */
	//printf("usbd: Enable state\n");
	USBDevice.interface_state= USBD_STATE_ENUM_DONE;
	USBDevice.enum_count++;
}

void setDescriptor(SetupPacket_t *req)
{
	ARG_UNUSED(req);

	sendCtrlEndpointData(NULL , 0);
}

void setFeature(SetupPacket_t *req)
{
	ARG_UNUSED(req);
}

void setInterface(SetupPacket_t *req)
{
	ARG_UNUSED(req);
}

void synchFrame(SetupPacket_t *req)
{
	ARG_UNUSED(req);
}

// Alert TODO need to define later according to USB protocol
void handleUsbRequest(SetupPacket_t *req)
{
	trace("H->D handleUsbRequest: type %02x request %02x\n", req->bRequestType, req->bRequest);
	switch (req->bRequestType & SETUPKT_TYPEMSK) {
	case SETUPKT_STDREQ:
		//trace("std request\n");
		handleStdRequest(req);
		break;
	case SETUPKT_CLASSREQ:
		trace("class request\n");
		handleClassRequest(req);
		break;
	case SETUPKT_VENDREQ:
		/*printf("VendorReq Type: %02x Request: %02x\n", req->bRequestType, req->bRequest);*/
		trace("vendor request\n");
		handleVendRequest(req);
		break;
	default:
		printf("bad USB request\n");
		stallEndpoint(0);
	}
}

void handleClassRequest(SetupPacket_t *req)
{
	uint32_t interface = req->wIndex;
	usbd_class_request_handler_t handler;
	
	/* check for bogus interface numbers */
	if (interface >= USBDevice.num_interfaces) {
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
		return;
	}

	handler = USBDevice.interface[interface].class_request_handler;
	if (handler) {
		(*handler)(req, USBDevice.controlIn, USBDevice.controlOut);
		return;
	}

	trace("default class request response\n");
	sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
}

void handleVendRequest(SetupPacket_t *req)
{
	usbd_handleVendRequest_handler_t handler;
	handler = USBDevice.vendor_req_handler;

	/* USB should handle as part of a registered callback from the app */
	if (handler)
	{
		(*handler)(req);
		return;
	}
	printf("No vendor specific handler found\n");
}


void handleStdRequest(SetupPacket_t *req)
{
	//trace("std request %d\n", req->bRequest);
	switch (req->bRequest) {
	case CLEAR_FEATURE:
		trace_enum("CLEAR_FEATURE\n");
		clearFeature(req);
		break;
	case GET_CONFIGURATION:
		trace_enum("GET_CONFIGURATION\n");
		getConfiguration(req);
		break;
	case GET_DESCRIPTOR:
		//trace_enum("GET_DESCRIPTOR\n");
		getDescriptor(req);
		break;
	case GET_INTERFACE:
		trace_enum("GET_INTERFACE\n");
		getInterface(req);
		break;
	case GET_STATUS:
		trace_enum("GET_STATUS\n");
		getStatus(req);
		break;
	case SET_ADDRESS:
		trace_enum("SET_ADDRESS\n");
		setAddress(req);
		break;
	case SET_CONFIGURATION:
		trace_enum("SET_CONFIGURATION\n");
		setConfiguration(req);
		break;
	case SET_DESCRIPTOR:
		trace_enum("SET_DESCRIPTOR\n");
		setDescriptor(req);
		break;
	case SET_FEATURE:
		trace_enum("SET_FEATURE\n");
		setFeature(req);
		break;
	case SET_INTERFACE:
		trace_enum("SET_INTERFACE\n");
		setInterface(req);
		break;
	case SYNCH_FRAME:
		trace_enum("SYNCH_FRAME\n");
		synchFrame(req);
		break;
	default:
		printf("unknown std request 0x%02x\n", req->bRequest);
		stallEndpoint(0);
	}
}

void setUsbdInterfaceState(usbd_if_state_enum_t state)
{
	USBDevice.interface_state = state;
}
unsigned int getUsbdInterfaceState(void)
{
	return (USBDevice.interface_state);
}
unsigned int isUsbdEnumDone(void)
{
	return ((USBDevice.interface_state == USBD_STATE_ENUM_DONE)?1:0);
}
unsigned int isUsbdShutdown(void)
{
	return ((USBDevice.interface_state == USBD_STATE_SHUTDOWN)?1:0);
}
unsigned int getUsbEnumCount(void)
{
	return (USBDevice.enum_count);
}
