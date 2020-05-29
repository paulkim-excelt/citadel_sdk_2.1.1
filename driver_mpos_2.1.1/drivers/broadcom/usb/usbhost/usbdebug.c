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

#include "cfeglue.h"

#include "usbchap9.h"
#include "usbd.h"

/* #define _OHCI_DEBUG_ */

#ifdef _OHCI_DEBUG_
void usb_dbg_dumpportstatus(int port, usb_port_status_t *portstatus, int level)
{
	int idx;
	u16_t x;

	for (idx = 0; idx < level; idx++)
		printf("  ");

	printf("PORT %d STATUS\n", port);

	for (idx = 0; idx < level; idx++)
		printf("  ");

	x = GETUSBFIELD((portstatus), wPortStatus);
	printf("wPortStatus     = %04X  ", x);
	if (x & 1)
		printf("DevicePresent ");
	if (x & 2)
		printf("Enabled ");
	if (x & 4)
		printf("Suspend ");
	if (x & 8)
		printf("OverCurrent ");
	if (x & 16)
		printf("InReset ");
	if (x & 256)
		printf("Powered ");
	if (x & 512)
		printf("LowSpeed ");
	printf("\n");

	for (idx = 0; idx < level; idx++)
		printf("  ");
	x = GETUSBFIELD((portstatus), wPortChange);
	printf("wPortChange     = %04X  ", x);
	if (x & 1)
		printf("ConnectChange ");
	if (x & 2)
		printf("EnableChange ");
	if (x & 4)
		printf("SuspendChange ");
	if (x & 8)
		printf("OverCurrentChange ");
	if (x & 16)
		printf("ResetChange ");
	printf("\n");
}

static char *getstringmaybe(usbdev_t *dev, unsigned int string)
{
	static char buf[256];

	if (string == 0) {
		strcpy(buf, "");
		return buf;
	}

	memset((void *)buf, 0, sizeof(buf));
	usb_get_string(dev, string, buf, sizeof(buf));

	return buf;
}

void usb_dbg_dumpdescriptors(usbdev_t *dev, u8_t *ptr, int len)
{
	u8_t *endptr;
	usb_config_descr_t  *cfgdscr;
	usb_interface_descr_t *ifdscr;
	usb_device_descr_t *devdscr;
	usb_endpoint_descr_t *epdscr;
	usb_hid_descr_t *hiddscr;
	usb_hub_descr_t *hubdscr;
	usb_cs_descr_t  *csdscr;
	static char *eptattribs[4] = {"Control",
					"Isoc",
					"Bulk",
					"Interrupt"};
	int idx;

	endptr = ptr + len;

	while (ptr < endptr) {
		cfgdscr = (usb_config_descr_t *) ptr;

		switch (cfgdscr->bDescriptorType) {
		case USB_DEVICE_DESCRIPTOR_TYPE:
			devdscr = (usb_device_descr_t *) ptr;
			printf("---------------------------------------------------\n");
			printf("DEVICE DESCRIPTOR\n");
			printf("bLength         = %02X\n",  devdscr->bLength);
			printf("bDescriptorType = %02X\n", devdscr->bDescriptorType);
			printf("bcdUSB          = %04X\n", GETUSBFIELD(devdscr, bcdUSB));
			printf("bDeviceClass    = %02X\n", devdscr->bDeviceClass);
			printf("bDeviceSubClass = %02X\n", devdscr->bDeviceSubClass);
			printf("bDeviceProtocol = %02X\n", devdscr->bDeviceProtocol);
			printf("bMaxPktSize0    = %02X\n", devdscr->bMaxPacketSize0);

			if (endptr-ptr <= 8)
				break;

			printf("idVendor        = %04X (%d)\n",
			       GETUSBFIELD(devdscr, idVendor),
			       GETUSBFIELD(devdscr, idVendor));
			printf("idProduct       = %04X (%d)\n",
			       GETUSBFIELD(devdscr, idProduct),
			       GETUSBFIELD(devdscr, idProduct));
			printf("bcdDevice       = %04X\n", GETUSBFIELD(devdscr, bcdDevice));
			printf("iManufacturer   = %04X (%s)\n",
			       devdscr->iManufacturer,
			       getstringmaybe(dev, devdscr->iManufacturer));
			printf("iProduct        = %04X (%s)\n",
			       devdscr->iProduct,
			getstringmaybe(dev, devdscr->iProduct));
			printf("iSerialNumber   = %04X (%s)\n",
			       devdscr->iSerialNumber,
			       getstringmaybe(dev, devdscr->iSerialNumber));
			printf("bNumConfigs     = %04X\n", devdscr->bNumConfigurations);
			break;

		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			cfgdscr = (usb_config_descr_t *) ptr;
			printf("---------------------------------------------------\n");
			printf("CONFIG DESCRIPTOR\n");

			printf("bLength         = %02X\n", cfgdscr->bLength);
			printf("bDescriptorType = %02X\n", cfgdscr->bDescriptorType);
			printf("wTotalLength    = %04X\n", GETUSBFIELD(cfgdscr, wTotalLength));
			printf("bNumInterfaces  = %02X\n", cfgdscr->bNumInterfaces);
			printf("bConfigValue    = %02X\n", cfgdscr->bConfigurationValue);
			printf("iConfiguration  = %02X (%s)\n",
			       cfgdscr->iConfiguration,
			       getstringmaybe(dev, cfgdscr->iConfiguration));
			printf("bmAttributes    = %02X\n", cfgdscr->bmAttributes);
			printf("MaxPower        = %d (%dma)\n", cfgdscr->MaxPower, cfgdscr->MaxPower*2);
			break;

		case USB_INTERFACE_DESCRIPTOR_TYPE:
			printf("---------------------------------------------------\n");
			printf("INTERFACE DESCRIPTOR\n");

			ifdscr = (usb_interface_descr_t *) ptr;

			printf("bLength         = %02X\n", ifdscr->bLength);
			printf("bDescriptorType = %02X\n", ifdscr->bDescriptorType);
			printf("bInterfaceNum   = %02X\n", ifdscr->bInterfaceNumber);
			printf("bAlternateSet   = %02X\n", ifdscr->bAlternateSetting);
			printf("bNumEndpoints   = %02X\n", ifdscr->bNumEndpoints);
			printf("bInterfaceClass = %02X\n", ifdscr->bInterfaceClass);
			printf("bInterSubClass  = %02X\n", ifdscr->bInterfaceSubClass);
			printf("bInterfaceProto = %02X\n", ifdscr->bInterfaceProtocol);
			printf("iInterface      = %02X (%s)\n",
			       ifdscr->iInterface,
			       getstringmaybe(dev, ifdscr->iInterface));
			break;

		case USB_ENDPOINT_DESCRIPTOR_TYPE:
			printf("---------------------------------------------------\n");
			printf("ENDPOINT DESCRIPTOR\n");

			epdscr = (usb_endpoint_descr_t *) ptr;

			printf("bLength         = %02X\n", epdscr->bLength);
			printf("bDescriptorType = %02X\n", epdscr->bDescriptorType);
			printf("bEndpointAddr   = %02X (%d,%s)\n",
			       epdscr->bEndpointAddress,
			       epdscr->bEndpointAddress & 0x0F,
			       (epdscr->bEndpointAddress & USB_ENDPOINT_DIRECTION_IN) ? "IN" : "OUT"
			       );
			printf("bmAttrbutes     = %02X (%s)\n",
			       epdscr->bmAttributes,
			       eptattribs[epdscr->bmAttributes&3]);
			printf("wMaxPacketSize  = %04X\n", GETUSBFIELD(epdscr, wMaxPacketSize));
			printf("bInterval       = %02X\n", epdscr->bInterval);
			break;

		case USB_HID_DESCRIPTOR_TYPE:
			printf("---------------------------------------------------\n");
			printf("HID DESCRIPTOR\n");

			hiddscr = (usb_hid_descr_t *) ptr;

			printf("bLength         = %02X\n", hiddscr->bLength);
			printf("bDescriptorType = %02X\n", hiddscr->bDescriptorType);
			printf("bcdHID          = %04X\n", GETUSBFIELD(hiddscr, bcdHID));
			printf("bCountryCode    = %02X\n", hiddscr->bCountryCode);
			printf("bNumDescriptors = %02X\n", hiddscr->bNumDescriptors);
			printf("bClassDescrType = %02X\n", hiddscr->bClassDescrType);
			printf("wClassDescrLen  = %04X\n", GETUSBFIELD(hiddscr, wClassDescrLength));
			break;

		case USB_HUB_DESCRIPTOR_TYPE:
			printf("---------------------------------------------------\n");
			printf("HUB DESCRIPTOR\n");

			hubdscr = (usb_hub_descr_t *) ptr;

			printf("bLength         = %02X\n", hubdscr->bDescriptorLength);
			printf("bDescriptorType = %02X\n", hubdscr->bDescriptorType);
			printf("bNumberOfPorts  = %02X\n", hubdscr->bNumberOfPorts);
			printf("wHubCharacters  = %04X\n", GETUSBFIELD(hubdscr, wHubCharacteristics));
			printf("bPowerOnToPwrGd = %02X\n", hubdscr->bPowerOnToPowerGood);
			printf("bHubControlCurr = %02X (ma)\n", hubdscr->bHubControlCurrent);
			printf("bRemPwerMask[0] = %02X\n", hubdscr->bRemoveAndPowerMask[0]);
			break;

		case USB_CS_INTERFACE:
			printf("---------------------------------------------------\n");
			printf("CLASS SPECIFIC\n");

			csdscr = (usb_cs_descr_t *) ptr;

			printf("bLength         = %02X\n", csdscr->bDescriptorLength);
			printf("bDescriptorType = %02X\n", csdscr->bDescriptorType);
			printf("bDescriptorSubType = %02X\n", csdscr->bDescriptorSubType);
			for (idx = 3; idx < csdscr->bDescriptorLength; idx++)
				printf("%02X ", ptr[idx]);
			printf("\n");
			break;

		default:
			printf("---------------------------------------------------\n");
			printf("UNKNOWN DESCRIPTOR\n");
			printf("bLength         = %02X\n", cfgdscr->bLength);
			printf("bDescriptorType = %02X\n", cfgdscr->bDescriptorType);
			printf("Data Bytes      = ");
			for (idx = 2; idx < cfgdscr->bLength; idx++)
				printf("%02X ", ptr[idx]);
			printf("\n");

		}
		/* watch out for infinite loops */
		if (!cfgdscr->bLength)
			break;

		ptr += cfgdscr->bLength;
	}
}

void usb_dbg_dumpcfgdescr(usbdev_t *dev, unsigned int index)
{
	u8_t buffer[512];
	int res;
	int len;
	usb_config_descr_t *cfgdscr;

	memset((void *)buffer, 0, sizeof(buffer));

	cfgdscr = (usb_config_descr_t *)&buffer[0];

	res = usb_get_config_descriptor(dev, cfgdscr, index,
					sizeof(usb_config_descr_t));
	if (res != sizeof(usb_config_descr_t))
		printf("[a] usb_get_config_descriptor returns %d\n", res);

	len = GETUSBFIELD(cfgdscr, wTotalLength);

	res = usb_get_config_descriptor(dev, cfgdscr, 0, len);
	if (res != len)
		printf("[b] usb_get_config_descriptor returns %d\n", res);

	usb_dbg_dumpdescriptors(dev, &buffer[0], res);
}

void usb_dbg_dumpdevice(usbdev_t *dev)
{
	usb_device_descr_t *devdscr;
	int i;

	devdscr = &dev->ud_devdescr;

	printf("Device %d\n", dev->ud_address);
	printf(" Vendor: %s (%04x)\n",
	       getstringmaybe(dev, devdscr->iManufacturer),
	       GETUSBFIELD(devdscr, idVendor));
	printf(" Product: %s (%04x)\n",
	       getstringmaybe(dev, devdscr->iProduct),
	       GETUSBFIELD(devdscr, idProduct));

	usb_dbg_dumpdescriptors(dev, (u8_t *)devdscr,
				sizeof(usb_device_descr_t));

	for (i = 0; i < devdscr->bNumConfigurations; i++)
		usb_dbg_dumpcfgdescr(dev, i);
}


void usb_dbg_showdevice(usbdev_t *dev)
{
	printf("USB bus %d device %d: vendor %04X product %04X class %d  [%s]\n",
	       dev->ud_bus->ub_num, dev->ud_address,
	       GETUSBFIELD(&(dev->ud_devdescr), idVendor),
	       GETUSBFIELD(&(dev->ud_devdescr), idProduct),
	       dev->ud_devdescr.bDeviceClass,
	       (IS_HUB(dev) ? "HUB" : "DEVICE"));
}
#endif
