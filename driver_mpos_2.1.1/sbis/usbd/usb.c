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
/************************************************************************

	Function:	This is a driver for the USB peripheral. This file 
	contains routines more involved in the USB protocol itself. See 


************************************************************************/

#include "usbdevice_internal.h"
#include "usbdebug.h"
#include "usb.h"
#include "usbdesc.h"
#include "usbregisters.h"
#include <platform.h>
#include "usbdevice.h"


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
}

void resetEndpointPacketId(uint8_t EpNum)
{
}

void enableRemoteWake(bool wake_ok)
{
}
	
void UsbShutdown(void)
{
	USBPullDown();

	/* Reset the state */
	//DEBUG_LOG("usbd: Shutdown state\n");
	USBDevice.interface_state = USBD_STATE_SHUTDOWN;
	USBDevice.enum_count = 0;
}

bool UsbResume(uint8_t ucState)
{
	return FALSE;
}

void clearFeature(SetupPacket_t *req)
{
}

void getConfiguration(SetupPacket_t *req)
{
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
		trace_enum("GET_DESCRIPTOR (device)\n");
	       	dlen = usbdGetDeviceDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
	  	return;

	case DSCT_CONFIG:
		trace_enum("GET_DESCRIPTOR (config)\n");
	       	dlen = usbdGetConfigurationDescriptor(DescriptorBuffer, rlen);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);


		return;

	case DSCT_STRING:
		trace_enum("GET DESCRIPTOR (string %d)\n", idx);
		if (idx < stringDescriptorsNum) {
			dlen = usbdGetStringDescriptor(idx, rlen);
			sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, dlen);
		}
		else {
			stallEndpoint(0);
		}
		return;

	case DSCT_REPORT:
		trace_enum("GET DESCRIPTOR (report %d)\n", req->wIndex);
#ifdef USBD_KEYBOARD_INTERFACE
		if (req->wIndex == USBDevice.kbd_interface) {
			kbd_get_report_descriptor(rlen);
			return;
		}
#endif
		trace_enum("get report descriptor\n");
		break;
	default:
		trace_enum("unknown descriptor request 0x%02x\n", (uint8_t) (req->wValue>>8));
		
	}
	/* default reply */
	sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
}

void getInterface(SetupPacket_t *req)
{
}

void stallRequestedEndpoint(uint16_t wIndex)
{
}

void getStatus(SetupPacket_t *req)
{
}

void setAddress(SetupPacket_t *req)
{
}

void setConfiguration(SetupPacket_t *req)
{
	/* Init the state */
	//printf("usbd: Enable state\n");
	USBDevice.interface_state= USBD_STATE_ENUM_DONE;
	USBDevice.enum_count++;
}

void setDescriptor(SetupPacket_t *req)
{
	sendCtrlEndpointData(NULL,0);
}

void setFeature(SetupPacket_t *req)
{
}

void setInterface(SetupPacket_t *req)
{
}

void synchFrame(SetupPacket_t *req)
{
}

// Alert TODO need to define later according to USB protocol

void handleUsbRequest(SetupPacket_t *req)
{
	//trace("handleUsbRequest: type %02x request %02x\n", req->bRequestType, req->bRequest);
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
		trace("vendor request\n");
		handleVendRequest(req);
		break;
	default:
		stallEndpoint(0);
	}
}

void handleClassRequest(SetupPacket_t *req)
{
	int interface = req->wIndex;
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
#ifdef USBD_CT_INTERFACE
	if (USBDevice.ct_interface == interface) {
		switch (req->bRequest) {
		case CCID_ABORT:
			trace("CCID1: CCID_ABORT\n");
			break;
		case CCID_GET_CLOCK_FREQUENCIES:
			trace("CCID1: CCID_GET_CLOCK_FREQUENCIES\n");
			break;
		case CCID_GET_DATA_RATES:
			trace("CCID1: CCID_GET_DATA_RATES\n");
			break;
		default:
			trace("class request: CT: unknown request 0x%02x\n", req->bRequest);
			break;
		}
	}
#endif
	trace("default class request response\n");
	sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
}

void handleVendRequest(SetupPacket_t *req)
{
	/* HID requests come here */
	/* Set_Idle */
	switch (req->bRequest) {
	default:
		trace("vendor request(UNKNOWN 0x%02x)\n", req->bRequest);
		sendCtrlEpData(USBDevice.controlIn, DescriptorBuffer, 0);
		break;
	} 
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
		stallEndpoint(0);
	}
}

unsigned int getUsbdInterfaceState() {
	return (USBDevice.interface_state);
}
unsigned int isUsbdEnumDone() {
	return ((USBDevice.interface_state == USBD_STATE_ENUM_DONE)?1:0);
}
unsigned int isUsbdShutdown() {
	return ((USBDevice.interface_state == USBD_STATE_SHUTDOWN)?1:0);
}

unsigned int getUsbEnumCount() {
	return (USBDevice.enum_count);
}
