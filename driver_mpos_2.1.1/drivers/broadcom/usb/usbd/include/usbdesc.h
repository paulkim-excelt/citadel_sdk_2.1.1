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
 * @file usbdesc.h
 *
 * @brief  USB Descriptor type definations.
 */


#ifndef _USBDESC_H
#define _USBDESC_H

#include "phx.h"
#include "usbd.h"

/* SetupPacket Request Type bit field */

#define SETUPKT_DIRECT   	 0x80   
#define SETUPKT_TYPEMSK  	 0x60   
#define SETUPKT_STDREQ    	 0x00
#define SETUPKT_CLASSREQ  	 0x20
#define SETUPKT_VENDREQ   	 0x40
#define SETUPKT_STDEVIN   	 0x80
#define SETUPKT_STDEVOUT 	 0x00

/* Setup Packet */

typedef struct {
	uint8_t bRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} tSETUP_PACKET;

/*********************************************************************************/

/* Standard Descriptor Types */

#define DSCT_DEVICE          	0x01
#define DSCT_CONFIG          	0x02
#define DSCT_STRING          	0x03
#define DSCT_INTERFACE        	0x04
#define DSCT_ENDPOINT    		0x05
#define DSCT_DEVICE_QUALIFIER	0x06
#define DSCT_OTHER_SPEED_CONF   0x07
#define DSCT_POWER    			0x08
#define DSCT_REPORT				0x22
#define DSCT_BOS				0x0F
#define DSCT_DEVCAP				0x10
#define DSCT_MASK               0xFF00

#define DSCT_ENDPOINT_LEN       0x07


/* device capability types */
#define DEVCAP_2_0EXT           0x02

/* Endpoint Transfer Types */

#define EP_TSFRMASK		0x03
#define EP_CONTROL		0x00
#define EP_ISO			0x01
#define EP_BULK			0x02
#define EP_INTR			0x03


#pragma pack(1)
/* Descriptor Used in USB Enumeration */

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	bcdUsb;
    	uint8_t	bDeviceClass;
    	uint8_t	bDeviceSubClass;
    	uint8_t	bDeviceProtocol;
    	uint8_t	bMaxPacketSize0;
    	uint16_t	idVendor;
    	uint16_t	idProduct;
    	uint16_t	bcdDevice;
    	uint8_t	iManufacturer;
    	uint8_t	iProduct;
    	uint8_t	iSerialNumber;
    	uint8_t	bNumConfigurations;
} tDEVICE_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t wTotalLength;
    	uint8_t	bNumDeviceCaps;
} tBOS_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint8_t bDevCapabilityType;
    	uint8_t	bmAttributes[4];
} tUSB_2_0_EXT_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	bcdUsb;
    	uint8_t	bDeviceClass;
    	uint8_t	bDeviceSubClass;
    	uint8_t	bDeviceProtocol;
    	uint8_t	bMaxPacketSize0;
    	uint8_t	bNumConfigurations;
		uint8_t bReserved;
} tDEVICE_QUALIFIER_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint8_t	bEndpointAddress;
    	uint8_t	bmAttributes;
    	uint16_t wMaxPacketSize;
    	uint8_t	bInterval;
} tENDPOINT_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint8_t	bInterfaceNumber;
    	uint8_t	bAlternateSetting;
    	uint8_t	bNumEndpoints;
    	uint8_t	bInterfaceClass;
    	uint8_t	bInterfaceSubClass;
    	uint8_t	bInterfaceProtocol;
    	uint8_t	iInterface;
} tINTERFACE_DESCRIPTOR;


typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	wTotalLength;
    	uint8_t	bNumInterface;
    	uint8_t	bConfigurationValue;
    	uint8_t	iConfiguration;
    	uint8_t	bmAttributes;
    	uint8_t	bMaxPower;
} tCONFIGURATION_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	bString[32]; /* fye: use fixed length */
} tSTRING_DESCRIPTOR;

#pragma pack()


/* ***************End of Descriptors ********************* */


typedef struct {
    	tENDPOINT_DESCRIPTOR	*descriptor;
} EndpointType;


typedef struct {
    	tINTERFACE_DESCRIPTOR	*descriptor;
    	tSTRING_DESCRIPTOR	*interface_string;
    	//tCCIDCLASS_DESCRIPTOR *ccidclassdescriptor;
    //	EndpointType    	  *endpoints[ENDPOINT_NUMBER+1]; /* fye: use fixed length */
} InterfaceAlternateSettingType;


typedef struct {
    	InterfaceAlternateSettingType	*active_setting;
    	InterfaceAlternateSettingType	*alternate_settings[2]; /* fye: use fixed length */
} InterfaceType;


typedef struct {
    tCONFIGURATION_DESCRIPTOR	*descriptor;
    tSTRING_DESCRIPTOR		*configuration_string;
    InterfaceType			*interfaces[2]; /* fye: use fixed length */
} ConfigurationType;


typedef struct {
    tDEVICE_DESCRIPTOR	*descriptor;
    tSTRING_DESCRIPTOR	*language_id_string;
    tSTRING_DESCRIPTOR    	*manufacturer_string;
    tSTRING_DESCRIPTOR    	*product_string;
    tSTRING_DESCRIPTOR	*serial_number_string;
    ConfigurationType		*active_configuration;
    ConfigurationType		*configurations[2]; //fye: use fixed length
} DeviceType;

#endif
