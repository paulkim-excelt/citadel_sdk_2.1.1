
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

/* @file usb_host.c
 *
 * USB host protocol stack. This driver provides generic APIs to
 * interface with USB host.
 *
 */

#include <board.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <broadcom/dma.h>
#include <usbh/usb_host.h>
#include "usb_host_bcm5820x.h"
#include "host/ohci.h"
#include "host/ehci.h"

void usb_dbg_dumpportstatus(int port, struct usb_port_status *portstatus,
			    int level)
{
	int idx;
	u16_t x;

	for (idx = 0; idx < level; idx++)
		printk("  ");

	printk("PORT %d STATUS\n", port);

	for (idx = 0; idx < level; idx++)
		printk("  ");

	x = GETUSBFIELD((portstatus), wPortStatus);
	printk("wPortStatus     = %04X  ", x);
	if (x & 1)
		printk("DevicePresent ");
	if (x & 2)
		printk("Enabled ");
	if (x & 4)
		printk("Suspend ");
	if (x & 8)
		printk("OverCurrent ");
	if (x & 16)
		printk("InReset ");
	if (x & 256)
		printk("Powered ");
	if (x & 512)
		printk("LowSpeed ");
	printk("\n");

	for (idx = 0; idx < level; idx++)
		printk("  ");
	x = GETUSBFIELD((portstatus), wPortChange);
	printk("wPortChange     = %04X  ", x);
	if (x & 1)
		printk("ConnectChange ");
	if (x & 2)
		printk("EnableChange ");
	if (x & 4)
		printk("SuspendChange ");
	if (x & 8)
		printk("OverCurrentChange ");
	if (x & 16)
		printk("ResetChange ");
	printk("\n");
}

static char *getstringmaybe(struct usbdev *usbdev, unsigned int string)
{
	static char buf[256];

	if (string == 0) {
		strcpy(buf, "");
		return buf;
	}

	memset((void *)buf, 0, sizeof(buf));
	usb_get_string(usbdev, string, buf, sizeof(buf));

	return buf;
}

void usb_dbg_dumpdescriptors(struct usbdev *usbdev, u8_t *ptr, int len)
{
	u8_t *endptr;
	struct usb_config_descr  *cfgdscr;
	struct usb_interface_descr *ifdscr;
	struct usb_device_descr *devdscr;
	struct usb_endpoint_descr *epdscr;
	struct usb_hid_descr *hiddscr;
	struct usb_hub_descr *hubdscr;
	struct usb_cs_descr  *csdscr;
	static char *eptattribs[4] = {"Control",
					"Isoc",
					"Bulk",
					"Interrupt"};
	int idx;

	endptr = ptr + len;

	while (ptr < endptr) {
		cfgdscr = (struct usb_config_descr *) ptr;

		switch (cfgdscr->bDescriptorType) {
		case USB_DEVICE_DESCRIPTOR_TYPE:
			devdscr = (struct usb_device_descr *) ptr;
			printk("---------------------------------------\n");
			printk("DEVICE DESCRIPTOR\n");
			printk("bLength         = %02X\n",
					devdscr->bLength);
			printk("bDescriptorType = %02X\n",
					devdscr->bDescriptorType);
			printk("bcdUSB          = %04X\n",
					GETUSBFIELD(devdscr, bcdUSB));
			printk("bDeviceClass    = %02X\n",
					devdscr->bDeviceClass);
			printk("bDeviceSubClass = %02X\n",
					devdscr->bDeviceSubClass);
			printk("bDeviceProtocol = %02X\n",
					devdscr->bDeviceProtocol);
			printk("bMaxPktSize0    = %02X\n",
					devdscr->bMaxPacketSize0);

			if (endptr-ptr <= 8)
				break;

			printk("idVendor        = %04X (%d)\n",
			       GETUSBFIELD(devdscr, idVendor),
			       GETUSBFIELD(devdscr, idVendor));
			printk("idProduct       = %04X (%d)\n",
			       GETUSBFIELD(devdscr, idProduct),
			       GETUSBFIELD(devdscr, idProduct));
			printk("bcdDevice       = %04X\n",
				 GETUSBFIELD(devdscr, bcdDevice));
			printk("iManufacturer   = %04X (%s)\n",
			       devdscr->iManufacturer,
			       getstringmaybe(usbdev, devdscr->iManufacturer));
			printk("iProduct        = %04X (%s)\n",
			       devdscr->iProduct,
			getstringmaybe(usbdev, devdscr->iProduct));
			printk("iSerialNumber   = %04X (%s)\n",
			       devdscr->iSerialNumber,
			       getstringmaybe(usbdev, devdscr->iSerialNumber));
			printk("bNumConfigs     = %04X\n",
				 devdscr->bNumConfigurations);
			break;

		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			cfgdscr = (struct usb_config_descr *) ptr;
			printk("---------------------------------------\n");
			printk("CONFIG DESCRIPTOR\n");

			printk("bLength         = %02X\n",
					cfgdscr->bLength);
			printk("bDescriptorType = %02X\n",
					cfgdscr->bDescriptorType);
			printk("wTotalLength    = %04X\n",
					GETUSBFIELD(cfgdscr, wTotalLength));
			printk("bNumInterfaces  = %02X\n",
					cfgdscr->bNumInterfaces);
			printk("bConfigValue    = %02X\n",
					cfgdscr->bConfigurationValue);
			printk("iConfiguration  = %02X (%s)\n",
			       cfgdscr->iConfiguration,
			       getstringmaybe(usbdev, cfgdscr->iConfiguration));
			printk("bmAttributes    = %02X\n",
					cfgdscr->bmAttributes);
			printk("MaxPower        = %d (%dma)\n",
					cfgdscr->MaxPower, cfgdscr->MaxPower*2);
			break;

		case USB_INTERFACE_DESCRIPTOR_TYPE:
			printk("---------------------------------------\n");
			printk("INTERFACE DESCRIPTOR\n");

			ifdscr = (struct usb_interface_descr *) ptr;

			printk("bLength         = %02X\n",
					ifdscr->bLength);
			printk("bDescriptorType = %02X\n",
					ifdscr->bDescriptorType);
			printk("bInterfaceNum   = %02X\n",
					ifdscr->bInterfaceNumber);
			printk("bAlternateSet   = %02X\n",
					ifdscr->bAlternateSetting);
			printk("bNumEndpoints   = %02X\n",
					ifdscr->bNumEndpoints);
			printk("bInterfaceClass = %02X\n",
					ifdscr->bInterfaceClass);
			printk("bInterSubClass  = %02X\n",
					ifdscr->bInterfaceSubClass);
			printk("bInterfaceProto = %02X\n",
					ifdscr->bInterfaceProtocol);
			printk("iInterface      = %02X (%s)\n",
			       ifdscr->iInterface,
			       getstringmaybe(usbdev, ifdscr->iInterface));
			break;

		case USB_ENDPOINT_DESCRIPTOR_TYPE:
			printk("--------------------------------------\n");
			printk("ENDPOINT DESCRIPTOR\n");

			epdscr = (struct usb_endpoint_descr *) ptr;

			printk("bLength         = %02X\n", epdscr->bLength);
			printk("bDescriptorType = %02X\n",
					epdscr->bDescriptorType);
			printk("bEndpointAddr   = %02X (%d,%s)\n",
			       epdscr->bEndpointAddress,
			       epdscr->bEndpointAddress & 0x0F,
			       (epdscr->bEndpointAddress &
				USB_ENDPOINT_DIRECTION_IN) ?
				"IN" : "OUT");
			printk("bmAttrbutes     = %02X (%s)\n",
			       epdscr->bmAttributes,
			       eptattribs[epdscr->bmAttributes&3]);
			printk("wMaxPacketSize  = %04X\n",
					GETUSBFIELD(epdscr, wMaxPacketSize));
			printk("bInterval       = %02X\n", epdscr->bInterval);
			break;

		case USB_HID_DESCRIPTOR_TYPE:
			printk("--------------------------------------\n");
			printk("HID DESCRIPTOR\n");
			printk("HID DESCRIPTOR\n");

			hiddscr = (struct usb_hid_descr *) ptr;

			printk("bLength         = %02X\n",
					hiddscr->bLength);
			printk("bDescriptorType = %02X\n",
					hiddscr->bDescriptorType);
			printk("bcdHID          = %04X\n",
					GETUSBFIELD(hiddscr, bcdHID));
			printk("bCountryCode    = %02X\n",
					hiddscr->bCountryCode);
			printk("bNumDescriptors = %02X\n",
					hiddscr->bNumDescriptors);
			printk("bClassDescrType = %02X\n",
					hiddscr->bClassDescrType);
			printk("wClassDescrLen  = %04X\n",
				GETUSBFIELD(hiddscr, wClassDescrLength));
			break;

		case USB_HUB_DESCRIPTOR_TYPE:
			printk("--------------------------------------\n");
			printk("HUB DESCRIPTOR\n");

			hubdscr = (struct usb_hub_descr *) ptr;

			printk("bLength         = %02X\n",
					hubdscr->bDescriptorLength);
			printk("bDescriptorType = %02X\n",
					hubdscr->bDescriptorType);
			printk("bNumberOfPorts  = %02X\n",
					hubdscr->bNumberOfPorts);
			printk("wHubCharacters  = %04X\n",
				GETUSBFIELD(hubdscr, wHubCharacteristics));
			printk("bPowerOnToPwrGd = %02X\n",
					hubdscr->bPowerOnToPowerGood);
			printk("bHubControlCurr = %02X (ma)\n",
					hubdscr->bHubControlCurrent);
			printk("bRemPwerMask[0] = %02X\n",
					hubdscr->bRemoveAndPowerMask[0]);
			break;

		case USB_CS_INTERFACE_TYPE:
			printk("--------------------------------------\n");
			printk("CLASS SPECIFIC\n");

			csdscr = (struct usb_cs_descr *) ptr;

			printk("bLength         = %02X\n",
					csdscr->bDescriptorLength);
			printk("bDescriptorType = %02X\n",
					csdscr->bDescriptorType);
			printk("bDescriptorSubType = %02X\n",
					csdscr->bDescriptorSubType);
			for (idx = 3; idx < csdscr->bDescriptorLength; idx++)
				printk("%02X ", ptr[idx]);
			printk("\n");
			break;

		default:
			printk("---------------------------------------------------\n");
			printk("UNKNOWN DESCRIPTOR\n");
			printk("bLength         = %02X\n", cfgdscr->bLength);
			printk("bDescriptorType = %02X\n",
					 cfgdscr->bDescriptorType);
			printk("Data Bytes      = ");
			for (idx = 2; idx < cfgdscr->bLength; idx++)
				printk("%02X ", ptr[idx]);
			printk("\n");

		}
		/* watch out for infinite loops */
		if (!cfgdscr->bLength)
			break;

		ptr += cfgdscr->bLength;
	}
}

void usb_dbg_dumpcfgdescr(struct usbdev *dev, unsigned int index)
{
	u8_t buffer[512];
	int res;
	int len;
	struct usb_config_descr *cfgdscr;

	memset((void *)buffer, 0, sizeof(buffer));

	cfgdscr = (struct usb_config_descr *)&buffer[0];

	res = usb_get_config_descriptor(dev, cfgdscr, index,
					sizeof(struct usb_config_descr));
	if (res != sizeof(struct usb_config_descr))
		printk("[a] usb_get_config_descriptor returns %d\n", res);

	len = GETUSBFIELD(cfgdscr, wTotalLength);

	res = usb_get_config_descriptor(dev, cfgdscr, 0, len);
	if (res != len)
		printk("[b] usb_get_config_descriptor returns %d\n", res);

	usb_dbg_dumpdescriptors(dev, &buffer[0], res);
}

void usb_dbg_dumpdevice(struct usbdev *dev)
{
	struct usb_device_descr *devdscr;
	int i;

	devdscr = &dev->ud_devdescr;

	printk("Device %d\n", dev->ud_address);
	printk(" Vendor: %s (%04x)\n",
	       getstringmaybe(dev, devdscr->iManufacturer),
	       GETUSBFIELD(devdscr, idVendor));
	printk(" Product: %s (%04x)\n",
	       getstringmaybe(dev, devdscr->iProduct),
	       GETUSBFIELD(devdscr, idProduct));

	usb_dbg_dumpdescriptors(dev, (u8_t *)devdscr,
				sizeof(struct usb_device_descr));

	for (i = 0; i < devdscr->bNumConfigurations; i++)
		usb_dbg_dumpcfgdescr(dev, i);
}

void usb_dbg_showdevice(struct usbdev *dev)
{
	printk("USB bus %d device %d: vendor %X product %X class %d [%s]\n",
	       dev->ud_bus->ub_num, dev->ud_address,
	       GETUSBFIELD(&(dev->ud_devdescr), idVendor),
	       GETUSBFIELD(&(dev->ud_devdescr), idProduct),
	       dev->ud_devdescr.bDeviceClass,
	       (IS_HUB(dev) ? "HUB" : "DEVICE"));
}
/*
 *  usb_poll(bus)
 *
 *  Handle device-driver polling - simply vectors to host controller
 *  driver.
 *
 *  Input parameters:
 *	bus - bus structure
 *
 *  Return value:
 *	nothing
 */
void usb_poll(struct usbbus *bus)
{

	ARG_UNUSED(bus);

#ifndef USBHOST_INTERRUPT_MODE
	UBINTR(bus); /* ehci_intr, ohci_intr */
#endif
}

/*
 *  usb_create_pipe(usbdev, epaddr, mps, flags)
 *
 *  Create a pipe, causing the corresponding endpoint to
 *  be created in the host controller driver.  Pipes form the
 *  basic "handle" for unidirectional communications with a
 *  USB device.
 *
 *  Input parameters:
 *	usbdev - device we're talking about
 *	epaddr - endpoint address open, usually from the endpoint
 *		 descriptor
 *	mps - maximum packet size understood by the device
 *	flags - flags for this pipe (UP_xxx flags)
 *
 *  Return value:
 *	< 0 if error
 *	0 if ok
 */
int usb_create_pipe(struct usbdev *usbdev, int epaddr, int mps, int flags)
{
	struct usbpipe *pipe;
	int usbaddr;
	int pipeidx;

	usbaddr = usbdev->ud_address;
	pipeidx = USB_EPADDR_TO_IDX(epaddr);

	if (usbdev->ud_pipes[pipeidx] != NULL) {
		SYS_LOG_ERR("pipe already exists\n");
		return 0;
	}

	pipe = (struct usbpipe *)cache_line_aligned_alloc
					(sizeof(struct usbpipe));

	pipe->up_flags = flags;
	pipe->up_num = pipeidx;
	pipe->up_mps = mps;
	pipe->up_dev = usbdev;
	if (usbdev->ud_flags & UD_FLAG_LOWSPEED)
		flags |= UP_TYPE_LOWSPEED;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)pipe, sizeof(struct usbpipe));
#endif

	pipe->up_hwendpoint = UBEPTCREATE(usbdev->ud_bus,
					  usbdev->ud_address,
					  USB_ENDPOINT_ADDRESS(epaddr),
					  mps, flags);
	if (pipe->up_hwendpoint == NULL)
		return -1;

	usbdev->ud_pipes[pipeidx] = pipe;

	return 0;
}

/*
 *  usb_open_pipe(usbdev, epdesc)
 *
 *  Open a pipe given an endpoint descriptor - this is the
 *  normal way pipes get open, since you've just selected a
 *  configuration and have the descriptors handy with all
 *  the information you need.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	epdesc - endpoint descriptor
 *
 *  Return value:
 *	< 0 if error
 *	else endpoint address (from descriptor)
 */
int usb_open_pipe(struct usbdev *usbdev, struct usb_endpoint_descr *epdesc)
{
	int res;
	int flags = 0;

	SYS_LOG_DBG("\n## %s ## bmAttributes: 0x%x ## bEndpointAddress: 0x%x",
		__func__, epdesc->bmAttributes, epdesc->bEndpointAddress);
	if (USB_ENDPOINT_DIR_IN(epdesc->bEndpointAddress))
		flags |= UP_TYPE_IN;
	else
		flags |= UP_TYPE_OUT;

	switch (epdesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) {
	case USB_ENDPOINT_TYPE_CONTROL:
		flags |= UP_TYPE_CONTROL;
		break;
	case USB_ENDPOINT_TYPE_ISOCHRONOUS:
		flags |= UP_TYPE_ISOC;
		break;
	case USB_ENDPOINT_TYPE_BULK:
		flags |= UP_TYPE_BULK;
		break;
	case USB_ENDPOINT_TYPE_INTERRUPT:
		flags |= UP_TYPE_INTR;
		break;
	}

	res = usb_create_pipe(usbdev, epdesc->bEndpointAddress,
			      GETUSBFIELD(epdesc, wMaxPacketSize), flags);

	if (res < 0)
		return res;

	return epdesc->bEndpointAddress;
}

/*
 *  usb_destroy_pipe(usbdev, epaddr)
 *
 *  Close(destroy) an open pipe and remove endpoint descriptor
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	epaddr - pipe to close
 *
 *  Return value:
 *	nothing
 */
void usb_destroy_pipe(struct usbdev *usbdev, int epaddr)
{
	struct usbpipe *pipe;
	int pipeidx;

	pipeidx = USB_EPADDR_TO_IDX(epaddr);

	pipe = usbdev->ud_pipes[pipeidx];
	if (!pipe)
		return;

	if (usbdev->ud_pipes[pipeidx])
		UBEPTDELETE(usbdev->ud_bus,
			    usbdev->ud_pipes[pipeidx]->up_hwendpoint);

	usbdev->ud_pipes[pipeidx] = NULL;
}

/*
 *  usb_destroy_all_pipes(usbdev)
 *
 *  Destroy all pipes related to this device.
 *
 *  Input parameters:
 *	usbdev - device we're clearing out
 *
 *  Return value:
 *	nothing
 */
/*TODO: Should use a structure to store this variable */
extern unsigned int usbhub_ipipe;
void usb_destroy_all_pipes(struct usbdev *usbdev)
{
	unsigned int idx;

	for (idx = 0; idx < UD_MAX_PIPES_PER_DEV; idx++) {
		/*CS-3381, don't delete this pipe as it is used by usbhub. */
		if (idx == USB_EPADDR_TO_IDX(usbhub_ipipe))
			continue;

		if (usbdev->ud_pipes[idx]) {
			UBEPTDELETE(usbdev->ud_bus,
				    usbdev->ud_pipes[idx]->up_hwendpoint);
			usbdev->ud_pipes[idx] = NULL;
		}
	}
}

/*
 *  usb_create_device(bus,idx,lowspeed)
 *
 *  Create a new USB device.  This device will be set to
 *  communicate on address zero (default address) and will be
 *  ready for basic stuff so we can figure out what it is.
 *  The control pipe will be open, so you can start requesting
 *  descriptors right away.
 *
 *  Input parameters:
 *	bus - bus to create device on
 *	lowspeed - true if it's a lowspeed device (the hubs tell
 *		   us these things)
 *
 *  Return value:
 *	usb device structure, or NULL
 */
struct usbdev *usb_create_device(struct usbbus *bus, int idx, int lowspeed)
{
	struct usbdev *usbdev;
	int pipeflags;
	int ret;

	/*
	 *Create the device structure.
	 */

	if (idx < USB_MAX_DEVICES) {
		usbdev = (struct usbdev *)cache_line_aligned_alloc
						(sizeof(struct usbdev));
		memset((void *)usbdev, 0, sizeof(struct usbdev));
	} else {
		SYS_LOG_ERR("too many device[%d > %d]\r\n", idx,
			    USB_MAX_DEVICES - 1);
		return NULL;
	}
	usbdev->ud_bus = bus;
	usbdev->ud_address = 0;	/*default address */
	usbdev->ud_parent = NULL;
	usbdev->ud_flags = 0;


	k_mutex_init(&usbdev->ud_mutex);

	for (int i = 0; i < UD_MAX_PIPES_PER_DEV; i++)
		usbdev->ud_pipes[i] = NULL;

	/*
	 *Adjust things based on the target device speed
	 */

	pipeflags = UP_TYPE_CONTROL;
	if (lowspeed) {
		printk("setting up lowspeed flags\n");
		pipeflags |= UP_TYPE_LOWSPEED;
		usbdev->ud_flags |= UD_FLAG_LOWSPEED;
	}

	/*
	 *Create the control pipe.
	 */
	ret = usb_create_pipe(usbdev, 0, USB_CONTROL_ENDPOINT_MIN_SIZE,
				 pipeflags);

	if (ret) {
		SYS_LOG_ERR("Error creating usbdev idx:%d\n", idx);
		return NULL;
	}

	return usbdev;
}

/*
 *  usb_destroy_device(usbdev)
 *
 *  Delete an entire USB device, closing its pipes and freeing
 *  the device data structure
 *
 *  Input parameters:
 *	usbdev - device to destroy
 *
 *  Return value:
 *	nothing
 */
void usb_destroy_device(struct usbdev *usbdev)
{
	usb_destroy_all_pipes(usbdev);

	usbdev->ud_bus->ub_devices[usbdev->ud_address] = NULL;

	usbdev = NULL;
}

/*
 *  usb_make_request(usbdev, epaddr, buf, len, flags)
 *
 *  Create a template request structure with basic fields
 *  ready to go. A shorthand routine.
 *
 *  Input parameters:
 *	usbdev- device we're talking to
 *	epaddr - endpoint address, from usb_open_pipe()
 *	buf, length - user buffer and buffer length
 *	flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *	struct usbreq pointer, or NULL
 */
/*TODO: usbhub_ipipe should be a part of hub structure that
 * can be retrieved.
 */
extern unsigned int usbhub_ipipe;
struct usbreq *usb_make_request(struct usbdev *usbdev, int epaddr, u8_t *buf,
			   int length, int flags)
{
	struct usbreq *ur;
	struct usbpipe *pipe;
	int pipeidx;

	pipeidx = USB_EPADDR_TO_IDX(epaddr);

	pipe = usbdev->ud_pipes[pipeidx];

	if (pipe == NULL) {
		SYS_LOG_ERR("Could not get pipe\n");
		return NULL;
	}

#ifdef CONFIG_DATA_CACHE_SUPPORT
	if ((u32_t)buf & (DCACHE_LINE_SIZE - 1)) {
		printk("usb_make_request: Address %p is not aligned\n",
		       buf);
	}
#endif
	ur = (struct usbreq *)cache_line_aligned_alloc(sizeof(struct usbreq));

	memset(ur, 0, sizeof(struct usbreq));

	ur->ur_dev = usbdev;
	ur->ur_pipe = pipe;
	ur->ur_buffer = buf;
	ur->ur_length = length;
	ur->ur_flags = flags;
	ur->ur_callback = NULL;

	SYS_LOG_DBG("pipeidx:0x%x ur_flags:0x%x", pipeidx, ur->ur_flags);

	return ur;
}

/*
 *  usb_cancel_request(ur)
 *
 *  Cancel a pending usb transfer request.
 *
 *  Input parameters:
 *	ur - request to cancel
 *
 *  Return value:
 *	0 if ok
 *	else error (could not find request)
 */
int usb_cancel_request(struct usbreq *ur)
{
#ifdef USBHOST_INTERRUPT_MODE
	int key = irq_lock();
#endif
	/*remove request from OHCI/EHCI queues */
	UBCANCELXFER(ur->ur_dev->ud_bus, ur->ur_pipe->up_hwendpoint, ur);

#ifdef USBHOST_INTERRUPT_MODE
	irq_unlock(key);
#endif
	/*request is no longer in progress */
	return 0;
}

/*
 *  usb_free_request(ur)
 *
 *  Return a transfer request to the free pool.
 *
 *  Input parameters:
 *	ur - request to return
 *
 *  Return value:
 *	nothing
 */
void usb_free_request(struct usbreq *ur)
{
	if (ur->ur_inprogress) {
		printk("Yow! Tried to free request %p that was in progress!\n",
			ur);
		return;
	}
	ur = NULL;
}

/*
 *  usb_queue_request(ur)
 *
 *  Call the transfer handler in the host controller driver to
 *  set up a transfer descriptor
 *
 *  Input parameters:
 *	ur - request to queue
 *
 *  Return value:
 *	0 if ok
 *	else error
 */
int usb_queue_request(struct usbreq *ur)
{
	int res;

	SYS_LOG_DBG("Queue %p\n", ur);

	ur->ur_inprogress = 1;
	ur->ur_xferred = 0;

#ifdef USBHOST_INTERRUPT_MODE
	int key = irq_lock();
#endif

	res = UBXFER(ur->ur_dev->ud_bus, /* ohci/ehci xfer */
		     ur->ur_pipe->up_hwendpoint, ur);
	if (res == -1) {
		/*something is wrong.*/
		ur->ur_inprogress = 0;
		ur->ur_status = -1;
	}

#ifdef USBHOST_INTERRUPT_MODE
	irq_unlock(key);
#endif
	return res;
}

/*
 *  usb_wait_request(ur)
 *
 *  Wait until a request completes, calling the polling routine
 *  as we wait.
 *
 *  Input parameters:
 *	ur - request to wait for
 *
 *  Return value:
 *	request status
 */
int usb_wait_request(struct usbreq *ur)
{
	while ((volatile int)(ur->ur_inprogress))
#ifndef USBHOST_INTERRUPT_MODE
		usb_poll(ur->ur_dev->ud_bus);
#else
		;
#endif
	return ur->ur_status;
}

/*
 *  usb_sync_request(ur)
 *
 *  Synchronous request - call usb_queue and then usb_wait
 *
 *  Input parameters:
 *	ur - request to submit
 *
 *  Return value:
 *	status of request
 */
int usb_sync_request(struct usbreq *ur)
{
	usb_queue_request(ur);

	return usb_wait_request(ur);
}

/*
 *  usb_make_sync_request(usbdev, epaddr, buf, len, flags)
 *
 *  Create a request, wait for it to complete, and delete
 *  the request.  A shorthand**2 routine.
 *
 *  Input parameters:
 *	usbdev- device we're talking to
 *	epaddr - endpoint address, from usb_open_pipe()
 *	buf,length - user buffer and buffer length
 *	flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *	status of request
 */
int usb_make_sync_request(struct usbdev *usbdev, int epaddr,
			  u8_t *buf, int length, int flags)
{
	struct usbreq *ur;
	int res;

	if (usbdev == NULL)
		return -ENODEV;

	ur = usb_make_request(usbdev, epaddr, buf, length, flags);
	if (ur == NULL) {
		SYS_LOG_ERR("usb_make_request failed\n");
		return -EFAULT;
	}

	res = usb_sync_request(ur);

	usb_free_request(ur);

	return res;
}

/*
 *  usb_make_sync_request_xferred(usbdev, epaddr, buf, len, flags, xferred)
 *
 *  Create a request, wait for it to complete, and delete
 *  the request, returning the amount of data transferred.
 *
 *  Input parameters:
 *	usbdev- device we're talking to
 *	epaddr - endpoint address, from usb_open_pipe()
 *	buf,length - user buffer and buffer length
 *	flags - transfer direction, etc. (UR_xxx flags)
 *      xferred - pointer to where to store xferred data length
 *
 *  Return value:
 *	status of request
 */
int usb_make_sync_request_xferred(struct usbdev *usbdev, int epaddr, u8_t *buf,
				  int length, int flags, int *xferred)
{
	struct usbreq *ur;
	int res;

	if (usbdev == NULL)
		return -ENODEV;

	ur = usb_make_request(usbdev, epaddr, buf, length, flags);
	if (ur == NULL) {
		SYS_LOG_ERR("usb_make_request failed\n");
		return -EFAULT;
	}

	res = usb_sync_request(ur);

	*xferred = ur->ur_xferred;

	usb_free_request(ur);

	return res;
}

/*
 *  usb_simple_request(usbdev, reqtype, bRequest, wValue, wIndex)
 *
 *  Handle a simple USB control pipe request.  These are OUT
 *  requests with no data phase.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	reqtype - request type (bmRequestType) for descriptor
 *	wValue - wValue for descriptor
 *	wIndex - wIndex for descriptor
 *
 *  Return value:
 *	0 if ok
 *	 else error
 */
int usb_simple_request(struct usbdev *usbdev, u8_t reqtype, int bRequest,
		       int wValue, int wIndex)
{
	u8_t dbg_buffer[8] = { 0 };

	return usb_std_request(usbdev, reqtype, bRequest, wValue, wIndex,
			       (u8_t *)dbg_buffer, 0);
}

/*
 *  usb_complete_request(ur, status)
 *
 *  Called when a usb request completes - pass status to
 *  caller and call the callback if there is one.
 *
 *  Input parameters:
 *	ur - struct usbreq to complete
 *	status - completion status
 *
 *  Return value:
 *	nothing
 */
void usb_complete_request(struct usbreq *ur, int status)
{
	SYS_LOG_DBG("Complete %p Status=%d\n", ur, status);

	ur->ur_status = status;
	ur->ur_inprogress = 0;

	/* usbhub_ireq_callback, usbhid_ireq_callback */
	if (ur->ur_callback)
		ur->ur_status = (*(ur->ur_callback)) (ur);

	/* Free the usbrequest after completion of request */
	ur = NULL;
}

/*
 *  usb_get_descr(usbdev, reqtype, dsctype, dscidx, respbuf, buflen)
 *
 *  Request a descriptor from the device.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	reqtype - bmRequestType field for descriptor we want
 *	dsctype - descriptor type we want
 *	dscidx - index of descriptor we want (often zero)
 *	respbuf - response buffer
 *	buflen - length of response buffer
 *
 *  Return value:
 *	number of bytes transferred
 */
int usb_get_descr(struct usbdev *usbdev, u8_t reqtype, int dsctype, int dscidx,
		       u8_t *respbuf, int buflen)
{
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)respbuf, (u32_t)buflen);
#endif
	return usb_std_request(usbdev,
			       reqtype, USB_REQUEST_GET_DESCRIPTOR,
			       USB_DESCRIPTOR_TYPEINDEX(dsctype, dscidx),
			       0, respbuf, buflen);
}

/*
 *  usb_get_config_descriptor(usbdev, dscr, idx, maxlen)
 *
 *  Request the configuration descriptor from the device.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	dscr - descriptor buffer (receives data from device)
 *	idx - index of config we want (usually zero)
 *	maxlen - total size of buffer to receive descriptor
 *
 *  Return value:
 *	number of bytes copied
 */
int usb_get_config_descriptor(struct usbdev *usbdev,
			      struct usb_config_descr *dscr,
			      int idx, int maxlen)
{
	int res;
	u8_t *respbuf;

	respbuf = cache_line_aligned_alloc(sizeof(maxlen));
	res = usb_get_descr(usbdev, USBREQ_DIR_IN,
				 USB_CONFIGURATION_DESCRIPTOR_TYPE, idx,
				 respbuf, maxlen);
	memcpy(dscr, respbuf, maxlen);

	cache_line_aligned_free(respbuf);

	return res;
}

/*
 *  usb_get_device_descriptor(usbdev, dscr, smallflg)
 *
 *  Request the device descriptor for the device. This is often
 *  the first descriptor requested, so it needs to be done in
 *  stages so we can find out how big the control pipe is.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	dscr - pointer to buffer to receive descriptor
 *	smallflg - TRUE to request just 8 bytes.
 *
 *  Return value:
 *	number of bytes copied
 */
int usb_get_device_descriptor(struct usbdev *usbdev,
			      struct usb_device_descr *dscr,
			      int smallflg)
{
	int res;
	u8_t *respbuf;
	int amtcopy;

	/*
	 * Smallflg truncates the request to 8 bytes.  We need to do this for
	 * the very first transaction to a USB device in order to determine
	 * the size of its control pipe.  Bad things will happen if you
	 * try to retrieve more data than the control pipe will hold.
	 *
	 * So, be conservative at first and get the first 8 bytes of the
	 * descriptor. Byte 7 is bMaxPacketSize0, the size of the control
	 * pipe.  Then you can go back and submit a bigger request for
	 * everything else.
	 */

	respbuf = (u8_t *)cache_line_aligned_alloc(256);
	amtcopy = smallflg ? USB_CONTROL_ENDPOINT_MIN_SIZE :
			     sizeof(struct usb_device_descr);

	res = usb_get_descr(usbdev, USBREQ_DIR_IN,
				 USB_DEVICE_DESCRIPTOR_TYPE, 0, respbuf,
				 amtcopy);
	memcpy(dscr, respbuf, amtcopy);

	cache_line_aligned_free(respbuf);

	return res;
}

/*
 *  usb_find_cfg_descr(usbdev, dtype, idx)
 *
 *  Find a configuration descriptor - we retrieved all the config
 *  descriptors during discovery, this lets us dig out the one
 *  we want.
 *
 *  Input parameters:
 *	usbdev - device we are talking to
 *	dtype - descriptor type to find
 *	idx - index of descriptor if there's more than one
 *
 *  Return value:
 *	pointer to descriptor or NULL if not found
 */
void *usb_find_cfg_descr(struct usbdev *usbdev, int dtype, int idx)
{
	u8_t *endptr;
	u8_t *ptr;
	struct usb_config_descr *cfgdscr;

	if (usbdev->ud_cfgdescr == NULL)
		return NULL;

	ptr = (u8_t *) usbdev->ud_cfgdescr;
	endptr = ptr + GETUSBFIELD((usbdev->ud_cfgdescr), wTotalLength);
	while (ptr < endptr) {

		cfgdscr = (struct usb_config_descr *) ptr;
		if (cfgdscr->bDescriptorType == dtype) {
			if (idx == 0)
				return (void *)ptr;
			idx--;
		}
		/*watch for infinite loops */
		if (!cfgdscr->bLength)
			break;
		ptr += cfgdscr->bLength;
	}

	return NULL;
}

/*
 *  usb_get_device_status(usbdev, status)
 *
 *  Request status from the device (status descriptor)
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	status - receives device_status structure
 *
 *  Return value:
 *	number of bytes returned
 */
int usb_get_device_status(struct usbdev *usbdev,
			  struct usb_device_status *status)
{
	return usb_std_request(usbdev,
			       USBREQ_DIR_IN,
			       0,
			       0,
			       0,
			       (u8_t *) status,
			       sizeof(struct usb_device_status));
}

/*
 *  usb_set_address(usbdev, address)
 *
 *  Set the address of a device.  This also puts the device
 *  in the master device table for the bus and reconfigures the
 *  address of the control pipe.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	address - new address (1..127)
 *
 *  Return value:
 *	request status
 */
int usb_set_address(struct usbdev *usbdev, int address)
{
	int res;
	int idx;
	struct usbpipe *pipe;

	if ((address < 0) || (address >= USB_MAX_DEVICES))
		return 0;

	res = usb_simple_request(usbdev, 0x00, USB_REQUEST_SET_ADDRESS,
				 address, 0);

	if (res == 0) {
		usbdev->ud_bus->ub_devices[address] = usbdev;
		usbdev->ud_address = address;
		for (idx = 0; idx < UD_MAX_PIPES_PER_DEV; idx++) {
			pipe = usbdev->ud_pipes[idx];
			if (pipe && pipe->up_hwendpoint)
				UBEPTSETADDR(usbdev->ud_bus,
					     pipe->up_hwendpoint,
					     address);
		}
	}

	return res;
}

/*
 *  usb_set_dev_configuration(usbdev, config)
 *
 *  Set the current configuration for a USB device.
 *
 *  Input parameters:
 *	dev - device we're talking to
 *	config - bConfigValue for the device
 *
 *  Return value:
 *	request status
 */
int usb_set_dev_configuration(struct usbdev *usbdev, int config)
{
	return(usb_simple_request(usbdev, 0x00, USB_REQUEST_SET_CONFIGURATION,
				  config, 0));
}

/*
 *  usb_set_feature(dev, feature)
 *
 *  Set the a feature for a USB device.
 *
 *  Input parameters:
 *	dev - device we're talking to
 *	feature - feature for the device
 *
 *  Return value:
 *	request status
 */
int usb_set_feature(struct usbdev *usbdev, int feature)
{
	return(usb_simple_request(usbdev, 0x00, USB_REQUEST_SET_FEATURE,
				  feature, 0));
}

/*
 *  usb_set_ep0mps(dev, mps)
 *
 *  Set the maximum packet size of endpoint zero (mucks with the
 *  endpoint in the host controller)
 *
 *  Input parameters:
 *	dev - device we're talking to
 *	mps - max packet size for endpoint zero
 *
 *  Return value:
 *	request status
 */
int usb_set_ep0mps(struct usbdev *usbdev, int mps)
{
	struct usbpipe *pipe;

	pipe = usbdev->ud_pipes[0];
	if (pipe && pipe->up_hwendpoint)
		UBEPTSETMPS(usbdev->ud_bus, pipe->up_hwendpoint, mps);
	if (pipe)
		pipe->up_mps = mps;

	return 0;
}

/*
 *  usb_new_address(bus)
 *
 *  Return the next available address for the specified bus
 *
 *  Input parameters:
 *	bus - bus to assign an address for
 *
 *  Return value:
 *	new address, < 0 if error
 */
int usb_new_address(struct usbbus *bus)
{
	int idx;
	/*e.g. device 1 is internal root hub emulation.*/
	/*USBH bus 0 device 1: vendor 0000 product 0000 class 09: USB Hub*/
	/*USBH bus 0 device 2: vendor 0930 product 6544 class 08:
	 * Mass-Storage Device
	 */
	for (idx = 1; idx < USB_MAX_DEVICES; idx++) {
		if (bus->ub_devices[idx] == NULL)
			return idx;
	}

	return -1;
}

/*
 *  usb_get_string_descriptor(usbdev, dscidx, respbuf, buflen)
 *
 *  Request a descriptor from the device.
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	dscidx - index of descriptor we want
 *	respbuf - response buffer
 *	len - length of response buffer
 *
 *  Return value:
 *	number of bytes transferred
 */
int usb_get_string(struct usbdev *usbdev, int dscidx, u8_t *respbuf, int len)
{
	return usb_std_request(usbdev, USBREQ_DIR_IN,
			       USB_REQUEST_GET_DESCRIPTOR,
			       USB_DESCRIPTOR_TYPEINDEX(
			       USB_STRING_DESCRIPTOR_TYPE, dscidx),
			       0x0409, /* Microsoft lang code for Eng */
			       respbuf, len);
}

/*
 *  usb_std_request(usbdev, bmRequestType, bRequest,wValue,
 *                  wIndex, buffer, length)
 *
 *  Do a standard control request on the control pipe,
 *  with the appropriate setup, data, and status phases.
 *
 *  Input parameters:
 *	usbdev - dev we're talking to
 *	bmRequestType,bRequest,wValue,wIndex - fields for the
 *		USB request structure
 *	buffer - user buffer
 *	length - length of user buffer
 *
 *  Return value:
 *	number of bytes transferred
 */
int usb_std_request(struct usbdev *usbdev, u8_t bmRequestType,
		    u8_t bRequest, u16_t wValue,
		    u16_t wIndex, u8_t *buffer, int length)
{
	struct device *dev = usbdev->ud_bus->dev;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;
	struct usbpipe *pipe = usbdev->ud_pipes[0];
	struct usbreq *ur;
	int res;
	struct usb_device_request *req;
	struct usb_device_descr *dev_desc;
	u8_t *databuf = NULL;

	req = (struct usb_device_request *)cache_line_aligned_alloc(64);
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t) buffer, (u32_t)length);
#endif
	if ((buffer != NULL) && (length != 0)) {
		databuf = cache_line_aligned_alloc(1024);
		if (bmRequestType & USBREQ_DIR_IN) {
			/* IN*/
			memset(databuf, 0, length);
		} else {
			/* OUT*/
			memcpy(databuf, buffer, length);
		}
	}

	req->bmRequestType = bmRequestType;
	req->bRequest = bRequest;
	PUTUSBFIELD(req, wValue, wValue);
	PUTUSBFIELD(req, wIndex, wIndex);
	PUTUSBFIELD(req, wLength, length);

	/* #1 setup packet */
	ur = usb_make_request(usbdev, 0, (u8_t *) req,
			      sizeof(struct usb_device_request), UR_FLAG_SETUP);
	/* FIXME: integrate this into structure*/
	ur->ur_response = buffer;
	ur->ur_reslength = length;
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)ur,
			(u32_t)sizeof(struct usbreq));
#endif
	res = usb_sync_request(ur);

	/*Obtain bcdUSB from device descriptor for USB2.0 device. */
	if ((bRequest == USB_REQUEST_GET_DESCRIPTOR)
	    && ((wValue >> 8) == USB_DEVICE_DESCRIPTOR_TYPE)) {
		dev_desc = (struct usb_device_descr *) (ur->ur_response);
		data->host_ver = dev_desc->bcdUSBHigh;
	}

	usb_free_request(ur);

	/* This is only for USB1.x, as USB2.0 setup, data and status
	 * phases are done in ehci_xfer() together.
	 */
	if ((data->host_ver != 2) || (data->cur_host == OHCI)) {
		/* #2 data packet */
		if (length != 0) {
			if (bmRequestType & USBREQ_DIR_IN) {
				ur = usb_make_request(usbdev, 0,
						      databuf, length,
						      UR_FLAG_IN);
			} else {
				ur = usb_make_request(usbdev, 0,
						      databuf, length,
						      UR_FLAG_OUT);
			}
			/* FIXME: integrate this into structure*/
			ur->ur_response = buffer;
			ur->ur_reslength = length;

			res = usb_sync_request(ur);

			if (res == K_OHCI_CC_STALL) {	/*STALL */
				usb_clear_stall(usbdev, pipe->up_num);
				usb_free_request(ur);
				if (databuf)
					cache_line_aligned_free(databuf);
				cache_line_aligned_free(req);
				return 0;
			}

			length = ur->ur_xferred;
			usb_free_request(ur);
		}

		if ((length != 0) && (bmRequestType & USBREQ_DIR_IN))
			memcpy(buffer, databuf, length);

		/* #3 status packet */
		if (bmRequestType & USBREQ_DIR_IN)
			ur = usb_make_request(usbdev, 0, (u8_t *) req, 0,
					      UR_FLAG_STATUS_OUT);
		else
			ur = usb_make_request(usbdev, 0, (u8_t *) req, 0,
					      UR_FLAG_STATUS_IN);

		/* FIXME: integrate this into structure*/
		ur->ur_response = buffer;
		ur->ur_reslength = length;
#ifdef CONFIG_DATA_CACHE_SUPPORT
		clean_n_invalidate_dcache_by_addr((u32_t)ur,
				(u32_t)sizeof(struct usbreq));
#endif
		res = usb_sync_request(ur);
		usb_free_request(ur);

		if (res == K_OHCI_CC_STALL) {	/*STALL */
			usb_clear_stall(usbdev, pipe->up_num);
			if (databuf)
				cache_line_aligned_free(databuf);
			cache_line_aligned_free(req);
			return 0;
		}
	}

	if (databuf)
		cache_line_aligned_free(databuf);

	cache_line_aligned_free(req);

	return length;
}

/*
 *  usb_clear_stall(usbdev, pipenum)
 *
 *  Clear a stall condition on the specified pipe
 *
 *  Input parameters:
 *	usbdev - device we're talking to
 *	pipenum - pipe number
 *
 *  Return value:
 *	0 if ok
 *	else error
 */
int usb_clear_stall(struct usbdev *usbdev, int pipenum)
{
	u8_t requestbuf[32] = { 0 };
	struct usb_device_request *req;
	struct usbreq *ur;
	int pipeidx;

	/*
	 * Clear the stall in the hardware.
	 */

	pipeidx = pipenum;

	UBEPTCLEARTOGGLE(usbdev->ud_bus,
		usbdev->ud_pipes[pipeidx]->up_hwendpoint);

	/*
	 * Do the "clear stall" request.   Note that we should do this
	 * without calling usb_simple_request, since usb_simple_request
	 * may itself stall.
	 */

	req = (struct usb_device_request *)requestbuf;

	req->bmRequestType = 0x02;
	req->bRequest = USB_REQUEST_CLEAR_FEATURE;
	PUTUSBFIELD(req, wValue, 0);	/*ENDPOINT_HALT */
	PUTUSBFIELD(req, wIndex, pipenum);
	PUTUSBFIELD(req, wLength, 0);

	ur = usb_make_request(usbdev, 0, requestbuf,
			      sizeof(struct usb_device_request),
			      UR_FLAG_SETUP);
	usb_sync_request(ur);
	usb_free_request(ur);

	usb_make_request(usbdev, 0, requestbuf, 0, UR_FLAG_STATUS_IN);
	usb_sync_request(ur);
	usb_free_request(ur);

	return 0;
}
