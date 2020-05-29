/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

#ifndef _USBCHAP9_H_
#define _USBCHAP9_H_

#define MAXIMUM_USB_STRING_LENGTH	255

/*
 * values for the bits returned by the USB GET_STATUS command
 */
#define USB_GETSTATUS_SELF_POWERED                0x01
#define USB_GETSTATUS_REMOTE_WAKEUP_ENABLED       0x02

#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05
#define USB_POWER_DESCRIPTOR_TYPE                 0x06
#define USB_HID_DESCRIPTOR_TYPE                   0x21
#define USB_CS_INTERFACE_TYPE                     0x24
#define USB_HUB_DESCRIPTOR_TYPE                   0x29

#define USB_DESCRIPTOR_TYPEINDEX(d, i) ((u16_t)((u16_t)(d)<<8 | (i)))

/*
 * Values for bmAttributes field of an
 * endpoint descriptor
 */

#define USB_ENDPOINT_TYPE_MASK                    0x03

#define USB_ENDPOINT_TYPE_CONTROL                 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01
#define USB_ENDPOINT_TYPE_BULK                    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03

/*
 * definitions for bits in the bmAttributes field of a
 * configuration descriptor.
 */
#define USB_CONFIG_POWERED_MASK                   0xc0

#define USB_CONFIG_BUS_POWERED                    0x80
#define USB_CONFIG_SELF_POWERED                   0x40
#define USB_CONFIG_REMOTE_WAKEUP                  0x20

/*
 * Endpoint direction bit, stored in address
 */

#define USB_ENDPOINT_DIRECTION_MASK               0x80
#define USB_ENDPOINT_DIRECTION_IN		  0x80 /* bit set means IN */

/*
 * test direction bit in the bEndpointAddress field of
 * an endpoint descriptor.
 */
#define USB_ENDPOINT_DIR_OUT(addr) (!((addr) & USB_ENDPOINT_DIRECTION_MASK))
#define USB_ENDPOINT_DIR_IN(addr)  ((addr) & USB_ENDPOINT_DIRECTION_MASK)

#define USB_ENDPOINT_TYPE_MASK		0x03
#define USB_ISENDPOINT_CTRL(attrib) \
	(((attrib) & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_CONTROL)
#define USB_ISENDPOINT_ISOC(attrib) \
	(((attrib) & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS)
#define USB_ISENDPOINT_BULK(attrib) \
	(((attrib) & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)
#define USB_ISENDPOINT_INT(attrib) \
	(((attrib) & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT)

#define USB_ENDPOINT_ADDRESS(addr)              ((addr) & 0x0F)

/*
 * USB defined request codes
 * see chapter 9 of the USB 1.0 specifcation for
 * more information.
 */

/*
 * These are the correct values based on the USB 1.0
 * specification
 */

#define USB_REQUEST_GET_STATUS                    0x00
#define USB_REQUEST_CLEAR_FEATURE                 0x01

#define USB_REQUEST_SET_FEATURE                   0x03

#define USB_REQUEST_SET_ADDRESS                   0x05
#define USB_REQUEST_GET_DESCRIPTOR                0x06
#define USB_REQUEST_SET_DESCRIPTOR                0x07
#define USB_REQUEST_GET_CONFIGURATION             0x08
#define USB_REQUEST_SET_CONFIGURATION             0x09
#define USB_REQUEST_GET_INTERFACE                 0x0A
#define USB_REQUEST_SET_INTERFACE                 0x0B
#define USB_REQUEST_SYNC_FRAME                    0x0C

/*
 * defined USB device classes
 */

#define USB_DEVICE_CLASS_RESERVED           0x00
#define USB_DEVICE_CLASS_AUDIO              0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS     0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE    0x03
#define USB_DEVICE_CLASS_MONITOR            0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE 0x05
#define USB_DEVICE_CLASS_IMAGE              0x06
#define USB_DEVICE_CLASS_PRINTER            0x07
#define USB_DEVICE_CLASS_STORAGE            0x08
#define USB_DEVICE_CLASS_HUB                0x09
#define USB_DEVICE_CLASS_VIDEO              0xEF
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC    0xFF

/*
 * USB defined Feature selectors
 */

#define USB_FEATURE_ENDPOINT_STALL          0x0000
#define USB_FEATURE_REMOTE_WAKEUP           0x0001
#define USB_FEATURE_POWER_D0                0x0002
#define USB_FEATURE_POWER_D1                0x0003
#define USB_FEATURE_POWER_D2                0x0004
#define USB_FEATURE_POWER_D3                0x0005

/*
 * USB Device descriptor.
 * To reduce problems with compilers trying to optimize
 * this structure, all the fields are bytes.
 */

#define USBWORD(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)

#define USB_CONTROL_ENDPOINT_MIN_SIZE	8

typedef struct usb_device_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bcdUSBLow, bcdUSBHigh;
	u8_t bDeviceClass;
	u8_t bDeviceSubClass;
	u8_t bDeviceProtocol;
	u8_t bMaxPacketSize0;
	u8_t idVendorLow, idVendorHigh;
	u8_t idProductLow, idProductHigh;
	u8_t bcdDeviceLow, bcdDeviceHigh;
	u8_t iManufacturer;
	u8_t iProduct;
	u8_t iSerialNumber;
	u8_t bNumConfigurations;
} usb_device_descr_t;

typedef struct usb_endpoint_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bEndpointAddress;
	u8_t bmAttributes;
	u8_t wMaxPacketSizeLow, wMaxPacketSizeHigh;
	u8_t bInterval;
} usb_endpoint_descr_t;

/*
 * values for bmAttributes Field in
 * USB_CONFIGURATION_DESCRIPTOR
 */

#define CONFIG_BUS_POWERED           0x80
#define CONFIG_SELF_POWERED          0x40
#define CONFIG_REMOTE_WAKEUP         0x20

typedef struct usb_config_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t wTotalLengthLow, wTotalLengthHigh;
	u8_t bNumInterfaces;
	u8_t bConfigurationValue;
	u8_t iConfiguration;
	u8_t bmAttributes;
	u8_t MaxPower;
} usb_config_descr_t;

#define USB_INTERFACE_CLASS_HUB		0x09

typedef struct usb_interface_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bInterfaceNumber;
	u8_t bAlternateSetting;
	u8_t bNumEndpoints;
	u8_t bInterfaceClass;
	u8_t bInterfaceSubClass;
	u8_t bInterfaceProtocol;
	u8_t iInterface;
} usb_interface_descr_t;

typedef struct usb_string_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bString[1];
} usb_string_descr_t;

/*
 * USB power descriptor added to core specification
 * XXX Note u16_t fields.  Not currently used by CFE.
 */

#define USB_SUPPORT_D0_COMMAND      0x01
#define USB_SUPPORT_D1_COMMAND      0x02
#define USB_SUPPORT_D2_COMMAND      0x04
#define USB_SUPPORT_D3_COMMAND      0x08

#define USB_SUPPORT_D1_WAKEUP       0x10
#define USB_SUPPORT_D2_WAKEUP       0x20

typedef struct usb_power_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bCapabilitiesFlags;
	u16_t EventNotification;
	u16_t D1LatencyTime;
	u16_t D2LatencyTime;
	u16_t D3LatencyTime;
	u8_t PowerUnit;
	u16_t D0PowerConsumption;
	u16_t D1PowerConsumption;
	u16_t D2PowerConsumption;
} usb_power_descr_t;

typedef struct usb_common_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
} usb_common_descr_t;

#define USB_DEV_STATUS_SELF_POWERED	(1<<0)
#define USB_DEV_STATUS_REMOTE_WAKEUP	(1<<1)
#define REMOTE_WAKEUP_SUPPORTED(st) \
	((st).wDeviceStatusLow & USB_DEV_STATUS_REMOTE_WAKEUP)
typedef struct usb_device_status_s {
	u8_t wDeviceStatusLow, wDeviceStatusHigh;
} usb_device_status_t;

/*
 * Standard USB HUB definitions
 *
 * See Chapter 11
 */

#define USB_HUB_DESCR_SIZE	8
typedef struct usb_hub_descr_s {
	u8_t bDescriptorLength;	/* Length of this descriptor */
	u8_t bDescriptorType;	/* Hub configuration type */
	u8_t bNumberOfPorts;	/* number of ports on this hub */
	u8_t wHubCharacteristicsLow;	/* Hub Charateristics */
	u8_t wHubCharacteristicsHigh;
	u8_t bPowerOnToPowerGood;/* port power on till power good in 2ms */
	u8_t bHubControlCurrent;	/* max current in mA */
	/* room for 255 ports power control and removable bitmask */
	u8_t bRemoveAndPowerMask[64];
} usb_hub_descr_t;

typedef struct usb_cs_descr_s {
	u8_t bDescriptorLength;	/* Length of this descriptor */
	u8_t bDescriptorType;	/* Class specic configuration type */
	u8_t bDescriptorSubType;	/* CS sub type */
	u8_t bDescriptorData[255];	/* CS Data */
} usb_cs_descr_t;

#define USB_HUBCHAR_PWR_GANGED	0
#define USB_HUBCHAR_PWR_IND	1
#define USB_HUBCHAR_PWR_NONE	2

typedef struct usb_hub_status_s {
	u8_t wHubStatusLow, wHubStatusHigh;
	u8_t wHubChangeLow, wHubChangeHigh;
} usb_hub_status_t;

#define USB_PORT_STATUS_CONNECT	0x0001
#define USB_PORT_STATUS_ENABLED 0x0002
#define USB_PORT_STATUS_SUSPEND 0x0004
#define USB_PORT_STATUS_OVERCUR 0x0008
#define USB_PORT_STATUS_RESET   0x0010
#define USB_PORT_STATUS_POWER   0x0100
#define USB_PORT_STATUS_LOWSPD	0x0200

typedef struct usb_port_status_s {
	u8_t wPortStatusLow, wPortStatusHigh;
	u8_t wPortChangeLow, wPortChangeHigh;
} usb_port_status_t;

#define USB_HUBREQ_GET_STATUS		0
#define USB_HUBREQ_CLEAR_FEATURE	1
#define USB_HUBREQ_GET_STATE		2
#define USB_HUBREQ_SET_FEATURE		3
#define USB_HUBREQ_GET_DESCRIPTOR	6
#define USB_HUBREQ_SET_DESCRIPTOR	7

#define USB_HUB_FEATURE_C_LOCAL_POWER	0
#define USB_HUB_FEATURE_C_OVER_CURRENT	1

#define USB_PORT_FEATURE_CONNECTION		0
#define USB_PORT_FEATURE_ENABLE			1
#define USB_PORT_FEATURE_SUSPEND		2
#define USB_PORT_FEATURE_OVER_CURRENT		3
#define USB_PORT_FEATURE_RESET			4
#define USB_PORT_FEATURE_POWER			8
#define USB_PORT_FEATURE_LOW_SPEED		9
#define USB_PORT_FEATURE_C_PORT_CONNECTION	16
#define USB_PORT_FEATURE_C_PORT_ENABLE		17
#define USB_PORT_FEATURE_C_PORT_SUSPEND		18
#define USB_PORT_FEATURE_C_PORT_OVER_CURRENT	19
#define USB_PORT_FEATURE_C_PORT_RESET		20

#define GETUSBFIELD(s, f) (((s)->f##Low) | ((s)->f##High << 8))
#define PUTUSBFIELD(s, f, v) do { \
				(s)->f##Low = (v & 0xFF); \
				(s)->f##High = ((v)>>8 & 0xFF); \
			} while (0)

typedef struct usb_device_request_s {
	u8_t bmRequestType;
	u8_t bRequest;
	u8_t wValueLow, wValueHigh;
	u8_t wIndexLow, wIndexHigh;
	u8_t wLengthLow, wLengthHigh;
} usb_device_request_t;

/*
 * Values for the bmAttributes field of a request
 */
#define USBREQ_DIR_IN	0x80
#define USBREQ_DIR_OUT	0x00
#define USBREQ_TYPE_STD	0x00
#define USBREQ_TYPE_CLASS	0x20
#define USBREQ_TYPE_VENDOR	0x40
#define USBREQ_TYPE_RSVD	0x60
#define USBREQ_REC_DEVICE	0x00
#define USBREQ_REC_INTERFACE 0x01
#define USBREQ_REC_ENDPOINT 0x02
#define USBREQ_REC_OTHER	0x03

#define REQCODE(req, dir, type, rec) (((req) << 8) | (dir) | (type) | (rec))
#define REQSW(req, attr) (((req) << 8) | (attr))

/*
 *  HID stuff
 */

typedef struct usb_hid_descr_s {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bcdHIDLow, bcdHIDHigh;
	u8_t bCountryCode;
	u8_t bNumDescriptors;
	u8_t bClassDescrType;
	u8_t wClassDescrLengthLow, wClassDescrLengthHigh;
} usb_hid_descr_t;

#endif /* _USBCHAP9_H_ */
