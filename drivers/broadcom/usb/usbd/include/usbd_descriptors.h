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

/* @file usbd_descriptors.h
 *
 * This file contains the common definitions for different USB device 
 * descriptors.
 *
 *
 */

#ifndef _USBD_DESCRIPTORS_H_
#define _USBD_DESCRIPTORS_H_

#define CV_CONF_NUM					1

#define MANUFACTURER_STRING			"Broadcom Corp"

#define PRODUCT_STRING				"58200"

#define SERIAL_NUMBER_STRING		"0123456789ABCD"

#define CV_STRING					"Broadcom USH"
#define CT_STRING					"Contacted SmartCard"
#define CL_STRING					"Contactless SmartCard"
#define NFP_STRING					"Broadcom NFP"

#define CV_STRING_IDX				4
#define CT_STRING_IDX				5
#define CL_STRING_IDX				6
#define NFP_STRING_IDX				8

#ifndef USH_BOOTROM /* SBI */
#define PID_FP_NONE					0x5841
#else
/* Keep 0x5841 as the USH Host/PC driver expects additional interface support for 0x5842 */
#define PID_FP_NONE					0x5841
/* #define PID_FP_NONE					0x5842*/
#endif

/*TODO once decided these have to be changed */
#define PID_FP_SWIPE				0x5801
#define PID_FP_TOUCH				0x5833
#define PID_FP_KEYBOARD				0x5803
#define PID_FP_NFP_SWIPE			0x5804
#define PID_FP_NFP_TOUCH			0x5834

#define CV_FP_NONE					"Broadcom USH"
#define CV_FP_SWIPE					"Broadcom USH w/swipe sensor"
#define CV_FP_TOUCH					"Broadcom USH w/touch sensor"
/*
Endpoint description from the HW engineers:

1 Configuration, 4 Interfaces each with a single Alternate setting
Default Control Endpoint (DCE) 0
8 IN physical endpoints, 4 OUT physical endpoints, total of 13 including DCE
4 Bidirectional Logical Endpoints (LE1-4)
Logical Endpoint 1 =  Endpoint Number 1, IN & OUT (BULK)
Logical Endpoint 2 =  Endpoint Number 2, IN & OUT (BULK)
Logical Endpoint 3 =  Endpoint Number 3, IN & OUT (BULK)
Logical Endpoint 4 =  Endpoint Number 4, IN & OUT (BULK)
Logical Endpoint 5 =  Endpoint Number 5, IN only  (INTR)
Logical Endpoint 6 =  Endpoint Number 6, IN only  (INTR)
Logical Endpoint 7 =  Endpoint Number 7, IN only  (INTR)
Logical Endpoint 8 =  Endpoint Number 8, IN only  (INTR)
Common DMA engine in DMA mode for transfers from Rx/Tx FIFOs
Single Receive (OUT) 64 byte FIFO
Individual buffers for Transmit (IN) endpoints allocated in a single RAM
*/

#define USB_BYTE(x)					((x) & 0xff)
#define USB_WORD(x)					((x) & 0xff), (((x) >> 8) & 0xff)
#define USB_DWORD(x)				((x) & 0xff), (((x) >> 8) & 0xff), (((x) >> 16) & 0xff), (((x) >> 24) & 0xff)

/* Base configuration Decriptor */

static const uint8_t usb_configuration_descriptor[] = {
		0x09,		/* bLength - 9 bytes */
		0x02,		/* bDescriptorType - CONFIGURATION */
		0x00,		/* wTotalLength (filled in later) */
		0x00,		/* wTotalLength - upper byte */
		0x00,		/* bNumInterfaces  (filled in later) */
		CV_CONF_NUM,		/* bConfgurationValue */
		0x00,		/* iConfiguration */
#ifdef USH_BOOTROM /*AAI */
		0x80 | 0x40 | 0x20,	/* bmAttributes: Reserved set to 1, self-powered, remote wake */
#else /* SBI */
		0x80 | 0x40,		/* bmAttributes: Reserved set to 1, self-powered */
#endif
		0x32			/* MaxPower - 2 mA */
};

#ifdef USBD_CV_INTERFACE
/* CV Interface Descriptor */
static const uint8_t usb_cv_interface_descriptor[] = {
		0x09,	/* bLength - 9 bytes */
		0x04,	/* bDescriptorType - INTERFACE */
		0x00,	/* interface Number - TBD */
		0x00,	/* Alternate Setting - 0 */
		0x03,	/* bNumEndpoints - 3 */
		0xFE,	/* interfaceClass - Class Code */
		0x00,	/* interfaceSubclass -SubClass Code */
		0x00,	/* interface Protocol - Bulk-Only Transport */
		CV_STRING_IDX,   /* iInterface- Index of string descriptor this interface */
/*		const tCCIDCLASS_DESCRIPTOR CvClassDescriptor =  */
		0x10, // Length
		0x25, // Type=CV class
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
};

#endif

#ifdef USBD_CT_INTERFACE
/* CT Interface Descriptor */
static const uint8_t usb_ct_interface_descriptor[] = {
		0x09,			/* bLength - 9 bytes */
		0x04,			/* bDescriptorType - INTERFACE */
		0x00,			/* interface Number - TBD */
		0x00,			/* Alternate Setting - 0 */
		0x03,			/* bNumEndpoints - 3 */
		0x0B,			/* interfaceClass - CCID Storage */
		0x00,			/* interfaceSubclass - SubClass Code */
		0x00,			/* interface Protocol - Bulk-Only Transport */
		CT_STRING_IDX,	/* iInterface- Index of string descriptor this interface */

/*		const tCCIDCLASS_DESCRIPTOR CCIDClassDescriptor =  */

/* 0 */			0x36, // Length
/* 1 */			0x21, // Type=CCID
/* 2 */			0x00,
/* 3 */			0x01, // CCID spec release number
/* 4 */			0x00, // Max slot index
/* 5 */			0x07, // 5.0V, 3.0V, 1.8V supported
/* 6 */			0x03, // Protocol T=0, T=1
/* 7 */			0x00,
/* 8 */			0x00,
/* 9 */			0x00,
/* 10 */		0xa0, // default clock frequency = 4M Hz
/* 11 */		0x0f,
/* 12 */		0x00,
/* 13 */		0x00,

/* 14 */		0xa0, // max clock frequency = 4M Hz
/* 15 */		0x0f,
/* 16 */		0x00,
/* 17 */		0x00,
/* 18 */		0x00, // f18
/* 19 */		0x80, // dwDataRate = 9600 bps
/* 20 */		0x25,
/* 21 */		0x00,
/* 22 */		0x00,
/* 23 */		0x90, // dwMaxDataRate = 115200 bps
/* 24 */		0xD0,
/* 25 */		0x03,
/* 26 */		0x00,
/* 27 */		0x00,
/* 28 */		0xF7, // dwMaxIFSD = 247
/* 29 */		0x00,

/* 30 */		0x00,
/* 31 */		0x00,
/* 32 */		0x00, // dwSynchProtocols
/* 33 */		0x00,
/* 34 */		0x00,
/* 35 */		0x00,
/* 36 */		0x00, // dwMechanical
/* 37 */		0x00,
/* 38 */		0x00,
/* 39 */		0x00,
#if 0
/* 40 */		0x3A, // dwFeatures
#else
/* 40 */		0xBA, // dwFeatures
#endif
/* 41 */		0x02,
/* 42 */		0x01, // TPDU level exchanges
/* 43 */		0x00,
/* 44 */		0x0F, // dwMaxCCIDMsgLength
/* 45 */		0x01,

/* 46 */		0x00,
/* 47 */		0x00,
/* 48 */		0x00, // bClassGetResponse
/* 49 */		0x00, // bClassEnvelope
/* 50 */		0x00, // wLcdLayout = no LCD
/* 51 */		0x00,
/* 52 */		0x00, // bPinSupport = No PIN
/* 53 */		0x01, // bMaxCCIDBusySlots = 1
};

#endif


#ifdef USBD_CL_INTERFACE

/* CL Interface Descriptor */
static const uint8_t usb_cl_interface_descriptor[] = {
		USB_BYTE(9),	/* bLength - 9 bytes */
		USB_BYTE(4),	/* bDescriptorType - INTERFACE */
		USB_BYTE(0),	/* bInterface Number - Not fixed at 2 keep it dynamic */
		USB_BYTE(0),	/* Alternate Setting - 0 */
		USB_BYTE(3),	/* bNumEndpoints - 3 */
		USB_BYTE(0x0B),	/* bInterfaceClass - Smart Card Device Class */
		USB_BYTE(0),	/* bInterfaceSubclass - SubClass Code */
		USB_BYTE(0),	/* bInterface Protocol - For Integrated Circuit Cards Interface Devices CCID */
		USB_BYTE(CL_STRING_IDX), /* iInterface- Index of string descriptor this interface */

/*		const tCCIDCLASS_DESCRIPTOR CCIDClassDescriptor =  */

		USB_BYTE(0x36), /* Length */
		USB_BYTE(0x21), /* Functional Descriptor Type  */
		USB_WORD(0x0101), /* CCID spec release number 1.10  */
		USB_BYTE(0), /* Max slot index  */
		USB_BYTE(1), /* 5V supported  */
		USB_DWORD(3), /* dwProtocols = T=0 & T=1 */

		USB_DWORD(3580), /* default clock 3.58 MHz */
		USB_DWORD(3580), /* maximum clock 3.58 MHz */
		USB_BYTE(1), /* number of clock frequencies */

		USB_DWORD(9600), /* dwDataRate */
		USB_DWORD(9600), /* dwMaxDataRate */
		USB_BYTE(1), /* bNumDataRatesSupported */

		USB_DWORD(254), /* max IFSD  */
		USB_DWORD(0), /* dwSynchProtocols, not supported  */
		USB_DWORD(0), /* dwMechanical, not supported  */

#if 0 //FIXME: LYCX_CS_PORT
		USB_DWORD(HIDRF_CCID_FEATURES), /* dwFeatures  */
#else
		/*HID Dependencies are available currently */
		/*Use the value from 1_0 branch */
		USB_DWORD(0x0204ba), /* dwFeatures  */
#endif

	/* FIXME: we want true extended APDUs at a later stage */
		USB_DWORD(512), /* dwMaxCCIDMessageLength */

		USB_BYTE(0x00), /* bClassGetResponse */
		USB_BYTE(0x00), /*bClassEnvelope */

		USB_WORD(0), /* wLcdLayout = no LCD  */
		USB_BYTE(0), /* bPinSupport = No PIN  */
		USB_BYTE(1), /* bMaxCCIDBusySlots = 1  */
};

#endif

#ifdef USBD_NFP_INTERFACE

/* NFP Interface Descriptor */
static const uint8_t usb_nfp_interface_descriptor[] = {
		0x09,	/* bLength - 9 bytes */
		0x04,	/* bDescriptorType - INTERFACE */
		0x03,	/* interface Number - Fixed to interface 3 */
		0x00,	/* Alternate Setting - 0 */
		0x03,	/* bNumEndpoints - 3 */
	//0x02,	/* bNumEndpoints - 2 */
		0xFF,	/* interfaceClass - Class Code */
		0x00,	/* interfaceSubclass -SubClass Code */
		0x00,	/* interface Protocol - Bulk-Only Transport */
		NFP_STRING_IDX,   /* iInterface- Index of string descriptor this interface */

/*		const tCCIDCLASS_DESCRIPTOR CvClassDescriptor =  */
		0x10, // Length
		0x26, // Type=CV class
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
};

#endif

/* DSCT Device Descriptor */
static const tDEVICE_DESCRIPTOR usbd_device_descriptor = {
		0x12,			/* bLength - 18 bytes */
		DSCT_DEVICE,		/* bDescriptorType - Device */
		CV_BCDUSB,		 /* bcdUsb - 1.1.0 */
		0x00,			/* bDeviceClass */
		0x00,			/* bDeviceSubClass */
		0x00,			/* bDeviceProtocol */
		0x40,			 /* bMaxPacketSize0 - 64 bytes */
		0x0a5c,		 /* Broadcom. */
		0,			/* Default device ID (filled in dynamically) */
		0x0101,			/* bcdDevice */
		0x01,			/* iManufacturer (string descriptor index) */
		0x02,			/* iProduct (string descriptor index) */
		0x03,			/* iSerialNumber (string descriptor index) */
		0x01				/* bNumConfigurations - 1 */
};

/* BOS Device Descriptor */
static const tBOS_DESCRIPTOR usbd_bos_descriptor = {
		0x05,			/* bLength - 5 bytes */
		DSCT_BOS,		/* bDescriptorType - BOS */
		0x000c,		 /* wTotalLength - BOS descriptor + USB 2.0 ext descriptor */
		0x01				/* bNumDeviceCaps */
};

/* USB2.0 ext Device Descriptor */
static const tUSB_2_0_EXT_DESCRIPTOR usbd_2_0_ext_descriptor = {
		0x07,			/* bLength - 7 bytes */
		DSCT_DEVCAP,	 /* bDescriptorType - device capability */
		DEVCAP_2_0EXT,   /* bDevCapabilityType - USB 2.0 ext descriptor */
		{0x02,			/* bmAttributes - LPM */
		0x00,
		0x00,
		0x00}
};

static const tDEVICE_QUALIFIER_DESCRIPTOR usbd_device_qualifier_descriptor = {
		0x0a,			/* bLength - 10 bytes */
		DSCT_DEVICE_QUALIFIER,	/* bDescriptorType - Device */
		CV_BCDUSB,		 /* bcdUsb - 1.1.0 */
		0x00,			/* bDeviceClass */
		0x00,			/* bDeviceSubClass */
		0x00,			/* bDeviceProtocol */
		0x40,			 /* bMaxPacketSize0 - 64 bytes */
		0x01,			/* bNumConfigurations - 1 */
		0x00				/* bReserved - must be 0 */
};

static const uint8_t usbd_bos_plus_usb_2_0_ext_descriptor[sizeof(tBOS_DESCRIPTOR) + sizeof(tUSB_2_0_EXT_DESCRIPTOR)] = {0,0,0,0,0,0,0,0,0,0,0,0};


/*
 * String descriptors
 * Descriptor 0 (LangID) gets special handling, so don't include it here.
 * Store the string descriptors as strings, they've be converted into descriptor
 * format when requested.
 */

static const char * const stringDescriptors[] = {
		MANUFACTURER_STRING,
		PRODUCT_STRING,
		SERIAL_NUMBER_STRING

#ifdef USBD_CV_INTERFACE
	,
		CV_STRING		/* CV Product String */
#endif
#ifdef USBD_CT_INTERFACE
	,
		CT_STRING		/* CCID Product String */
#endif
#ifdef USBD_CL_INTERFACE
	,
		CL_STRING		/* CCID Product String */
#endif
#ifdef USBD_NFP_INTERFACE
	,
		NFP_STRING			/* NFP Product String */
#endif

};

/* Also count the langId in the total */
static const int stringDescriptorsNum = sizeof(stringDescriptors)/sizeof(stringDescriptors[0]) + 1;

#endif /* USBDDESCRIPTORS_H */
