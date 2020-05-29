/***
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
 ***/
#include <stdbool.h>
#include <kernel.h>
#include "cfeglue.h"

#include "usbchap9.h"
#include "usbd.h"
#include "ohci.h"


#define K_USB_DEVICENOTRESPONDING	5 /*K_OHCI_CC_DEVICENOTRESPONDING */

#define MIN(x, y)    ((x) > (y) ? (y) : (x))
#define MAX(x, y)    ((x) > (y) ? (x) : (y))

//static usbpipe_t __aligned(ALIGNMENT)usb_pipe[USB_MAX_DEVICES][UD_MAX_PIPES_PER_DEV] = { 0 };
/* 0 - usbhub, 1~8 - usbhub port*/
//#define get_usb_pipe(x, y)  (addr2uncached(&usb_pipe[x][y]))

static usbdev_t __aligned(ALIGNMENT)usb_device[USB_MAX_DEVICES];
static usbpipe_t __aligned(ALIGNMENT)usb_pipe_dev0[UD_MAX_PIPES_PER_DEV];
static usbpipe_t __aligned(ALIGNMENT)usb_pipe_dev1[UD_MAX_PIPES_PER_DEV];


#define get_usb_device(idx) (addr2uncached(&usb_device[idx]))

u8_t __aligned(ALIGNMENT) respbuf_s[1024] = { 0 };
u8_t __aligned(ALIGNMENT) databuf_s[1024] = { 0 };
u8_t __aligned(ALIGNMENT) req_std_s[64] = { 0 };

static usbreq_t ur_req[2];

/*
 *  Globals
 */

#ifdef _USBREQTRACE_
#define REQTRACE(x) x

static char *devname(usbreq_t *ur)
{
	if (ur->ur_dev == NULL)
		return "unkdev";
	if (ur->ur_dev->ud_drv == NULL)
		return "unkdrv";
	return ur->ur_dev->ud_drv->udrv_name;
}

static char *eptname(usbreq_t *ur)
{
	static char buf[16];

	if (ur->ur_pipe == NULL)
		return "unkpipe";
	sprintf(buf, "pipe%02X/%02X", ur->ur_pipe->up_flags,
		ur->ur_pipe->up_num);
	return buf;
}

#else
#define REQTRACE(x)
#endif

int usb_device_descriptor_dump(usb_device_descr_t *dscr)
{
	printf("dscr: len=%d\n"
		"     type=%d\n"
		"      low=%d\n"
		"     high=%d\n"
		"     clas=%d\n"
		"     subc=%d\n"
		"     prot=%d\n"
		"     size=%d\n"
		"     vlow=%d\n"
		"     vhig=%d\n"
		"     plow=%d\n"
		"     phig=%d\n"
		"     dlow=%d\n"
		"     dhig=%d\n"
		"     manu=%d\n"
		"     ipro=%d\n"
		"     iser=%d\n"
		"     numb=%d\n",
		dscr->bLength,
		dscr->bDescriptorType,
		dscr->bcdUSBLow,
		dscr->bcdUSBHigh,
		dscr->bDeviceClass,
		dscr->bDeviceSubClass,
		dscr->bDeviceProtocol,
		dscr->bMaxPacketSize0,
		dscr->idVendorLow,
		dscr->idVendorHigh,
		dscr->idProductLow,
		dscr->idProductHigh,
		dscr->bcdDeviceLow,
		dscr->bcdDeviceHigh,
		dscr->iManufacturer,
		dscr->iProduct, dscr->iSerialNumber, dscr->bNumConfigurations);

	return 0;
}

extern usb_driver_t usbroothub_driver;

/*
 *  usb_create_pipe(dev,epaddr,mps,flags)
 *
 *  Create a pipe, causing the corresponding endpoint to
 *  be created in the host controller driver.  Pipes form the
 *  basic "handle" for unidirectional communications with a
 *  USB device.
 *
 *  Input parameters:
 *   dev - device we're talking about
 *   epaddr - endpoint address open, usually from the endpoint
 *	      descriptor
 *   mps - maximum packet size understood by the device
 *   flags - flags for this pipe (UP_xxx flags)
 *
 *  Return value:
 *   <0 if error
 *   0 if ok
 */
int usb_create_pipe(usbdev_t *dev, int epaddr, int mps, int flags)
{
	usbpipe_t *pipe;
	int usbaddr;
	int pipeidx;

	usbaddr = dev->ud_address;
	pipeidx = USB_EPADDR_TO_IDX(epaddr);


	if (usbaddr == 0)
	{
		//HUB
		pipe = (usbpipe_t *)(usb_pipe_dev0);
	}
	else
	{
		//DEVICE
		pipe = (usbpipe_t *)(usb_pipe_dev1);
	}

	if (dev->ud_pipes[pipeidx] != NULL) {
		printf("Trying to create a pipe that was already created!\n");
		return 0;
	}
#if 0
	/*pipe = &usb_pipe[usbaddr][pipeidx] ;*/
	pipe = get_usb_pipe(usbaddr, pipeidx);
#endif

	pipe->up_flags = flags;
	pipe->up_num = pipeidx;
	pipe->up_mps = mps;
	pipe->up_dev = dev;
	if (dev->ud_flags & UD_FLAG_LOWSPEED)
		flags |= UP_TYPE_LOWSPEED;

#ifdef CONFIG_DATA_CACHE_SUPPORT
		/*Do it for the full field because each object is not aligned for CACHELINE size*/
		if (usbaddr == 0)
		{
			//HUB
			clean_n_invalidate_dcache_by_addr((u32_t)usb_pipe_dev0 ,sizeof(usb_pipe_dev0));
		}
		else
		{
			//DEVICE
			clean_n_invalidate_dcache_by_addr((u32_t)usb_pipe_dev1 ,sizeof(usb_pipe_dev1));
		}
#endif



	pipe->up_hwendpoint = UBEPTCREATE(dev->ud_bus,
					  dev->ud_address,
					  USB_ENDPOINT_ADDRESS(epaddr),
					  mps, flags);
	dev->ud_pipes[pipeidx] = pipe;

	return 0;
}

/*
 *  usb_open_pipe(dev,epdesc)
 *
 *  Open a pipe given an endpoint descriptor - this is the
 *  normal way pipes get open, since you've just selected a
 *  configuration and have the descriptors handy with all
 *  the information you need.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   epdesc - endpoint descriptor
 *
 *  Return value:
 *   <0 if error
 *   else endpoint address (from descriptor)
 */

int usb_open_pipe(usbdev_t *dev, usb_endpoint_descr_t *epdesc)
{
	int res;
	int flags = 0;

	printf("\n#### %s ####### bmAttributes: 0x%x ####### bEndpointAddress: 0x%x",
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

	res =
	    usb_create_pipe(dev, epdesc->bEndpointAddress,
			    GETUSBFIELD(epdesc, wMaxPacketSize), flags);

	if (res < 0)
		return res;

	return epdesc->bEndpointAddress;
}

/*
 *  usb_destroy_pipe(dev,epaddr)
 *
 *  Close(destroy) an open pipe and remove endpoint descriptor
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   epaddr - pipe to close
 *
 *  Return value:
 *   nothing
 */

void usb_destroy_pipe(usbdev_t *dev, int epaddr)
{
	usbpipe_t *pipe;
	int pipeidx;

	pipeidx = USB_EPADDR_TO_IDX(epaddr);

	pipe = dev->ud_pipes[pipeidx];
	if (!pipe)
		return;

	if (dev->ud_pipes[pipeidx])
		UBEPTDELETE(dev->ud_bus, dev->ud_pipes[pipeidx]->up_hwendpoint);

	dev->ud_pipes[pipeidx] = NULL;
}

/*
 *  usb_destroy_all_pipes(dev)
 *
 *  Destroy all pipes related to this device.
 *
 *  Input parameters:
 *   dev - device we're clearing out
 *
 *  Return value:
 *   nothing
 */
extern unsigned int usbhub_ipipe;
void usb_destroy_all_pipes(usbdev_t *dev)
{
	unsigned int idx;

	for (idx = 0; idx < UD_MAX_PIPES_PER_DEV; idx++) {
		/*CS-3381, don't delete this pipe as it is used by usbhub. */
		if (idx == USB_EPADDR_TO_IDX(usbhub_ipipe))
			continue;

		if (dev->ud_pipes[idx]) {
			UBEPTDELETE(dev->ud_bus,
				    dev->ud_pipes[idx]->up_hwendpoint);
			dev->ud_pipes[idx] = NULL;
		}
	}
}

/*
 *  usb_destroy_device(dev)
 *
 *  Delete an entire USB device, closing its pipes and freeing
 *  the device data structure
 *
 *  Input parameters:
 *   dev - device to destroy
 *
 *  Return value:
 *   nothing
 */

void usb_destroy_device(usbdev_t *dev)
{
	usb_destroy_all_pipes(dev);

	dev->ud_bus->ub_devices[dev->ud_address] = NULL;

	dev = NULL;
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
 *   bus - bus to create device on
 *   lowspeed - true if it's a lowspeed device (the hubs tell
 *     us these things)
 *
 *  Return value:
 *   usb device structure, or NULL
 */
usbdev_t *usb_create_device(usbbus_t *bus, int idx, int lowspeed)
{
	usbdev_t *dev;
	int pipeflags;

	/*
	 *Create the device structure.
	 */
	printf("%s idx:0x%x\n", __func__, idx);

	if (idx < USB_MAX_DEVICES) {
		/*dev = &usb_device[idx];*/
		dev = get_usb_device(idx);
		memset((void *)dev, 0, sizeof(usbdev_t));
	} else {
		printf("usb_create_device, too many device[%d > %d] \r\n", idx,
			USB_MAX_DEVICES - 1);
		return NULL;
	}
	dev->ud_bus = bus;
	dev->ud_address = 0;	/*default address */
	dev->ud_parent = NULL;
	dev->ud_flags = 0;

	/*
	 *Adjust things based on the target device speed
	 */

	pipeflags = UP_TYPE_CONTROL;
	if (lowspeed) {
		pipeflags |= UP_TYPE_LOWSPEED;
		dev->ud_flags |= UD_FLAG_LOWSPEED;
	}

	/*
	 *Create the control pipe.
	 */
	usb_create_pipe(dev, 0, USB_CONTROL_ENDPOINT_MIN_SIZE, pipeflags);

	return dev;
}

/*
 *  usb_make_request(dev,epaddr,buf,len,flags)
 *
 *  Create a template request structure with basic fields
 *  ready to go.  A shorthand routine.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *   usbreq_t pointer, or NULL
 */

#define OHCI_READCSR(x) \
	(*((volatile u32_t *) ((0x46000800) + (x))))
usbreq_t *usb_make_request(usbdev_t *dev, int epaddr, u8_t *buf,
			   int length, int flags)
{
	usbreq_t *ur;
	usbpipe_t *pipe;
	int pipeidx;

	pipeidx = USB_EPADDR_TO_IDX(epaddr);

	pipe = dev->ud_pipes[pipeidx];

	if (pipe == NULL)
		return NULL;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	if ((u32_t)buf & (DCACHE_LINE_SIZE - 1)) {
		printf("usb_make_request: Address %p is not aligned \n",buf);
	}
#endif

	/* if endpoint address belongs to usbhub assign the rh ur_req.*/
	if(usbhub_ipipe == (unsigned int)epaddr)
	{
		ur = (usbreq_t*)(&ur_req[0]);
	}
	else //all other cases assign the normal ur_req
	{
		ur = (usbreq_t *)(&ur_req[1]);
	}
#if 0
	if (usbhub_ipipe == (u32_t)epaddr)
	/*if endpoint address belongs to usbhub assign the rh ur_req.*/
		ur = &rh_ur_req);
	else /*all other cases assign the normak ur_req.*/
		ur = (&ur_req);
#endif

	memset(ur, 0, sizeof(usbreq_t));

	ur->ur_dev = dev;
	ur->ur_pipe = pipe;
	ur->ur_buffer = buf;
	ur->ur_length = length;
	ur->ur_flags = flags;
	ur->ur_callback = NULL;
	/*printf("make req: pipeidx:0x%x ur_flags:0x%x",pipeidx,ur->ur_flags);*/
	return ur;
}

/*
 *  usb_poll(bus)
 *
 *  Handle device-driver polling - simply vectors to host controller
 *  driver.
 *
 *  Input parameters:
 *   bus - bus structure
 *
 *  Return value:
 *   nothing
 */

void usb_poll(usbbus_t *bus)
{
	ARG_UNUSED(bus);

#ifndef USBHOST_INTERRUPT_MODE
	UBINTR(bus);		/* ehci_intr, ohci_intr*/
#endif
}

/*
 *  usb_daemon(bus)
 *
 *  Polls for topology changes and initiates a bus scan if
 *  necessary.
 *
 *  Input parameters:
 *   bus - bus to  watch
 *
 *  Return value:
 *   nothing
 */

void usb_daemon(usbbus_t *bus)
{
	/*
	 *Just see if someone flagged a need for a scan here
	 *and start the bus scan if necessary.
	 *
	 *The actual scanning is a hub function, starting at the
	 *root hub, so the code for that is over there.
	 */

	if (bus->ub_flags & UB_FLG_NEEDSCAN) {
		bus->ub_flags &= ~UB_FLG_NEEDSCAN;
		usb_scan(bus);
	}
}

/*
 *  usb_cancel_request(ur)
 *
 *  Cancel a pending usb transfer request.
 *
 *  Input parameters:
 *   ur - request to cancel
 *
 *  Return value:
 *   0 if ok
 *   else error (could not find request)
 */

int usb_cancel_request(usbreq_t *ur)
{
#ifdef USBHOST_INTERRUPT_MODE
	ohci_enter_critical();
#endif
	/*remove request from OHCI/EHCI queues */
	UBCANCELXFER(ur->ur_dev->ud_bus, ur->ur_pipe->up_hwendpoint, ur);
#ifdef USBHOST_INTERRUPT_MODE
	ohci_exit_critical();
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
 *   ur - request to return
 *
 *  Return value:
 *   nothing
 */

void usb_free_request(usbreq_t *ur)
{
	REQTRACE(printf("Free %p (%s,%s)\n", ur, eptname(ur), devname(ur)));

	if (ur->ur_inprogress) {
		printf("Yow!  Tried to free request %p that was in progress!\n",
			ur);
		return;
	}
	ur = NULL;
}

/*
 *  usb_delay_ms(ms)
 *
 *  Wait a while, calling the polling routine as we go.
 *
 *  Input parameters:
 *   bus - bus we're talking to
 *   ms - how long to wait
 *
 *  Return value:
 *   nothing
 */

void usb_delay_ns(int cnt)
{
	volatile int i;

	for (i = 0; i < (cnt / 12); i++)
		;
}

void usb_delay_us(int cnt)
{
	k_busy_wait(cnt);
}

void usb_delay_ms(int ms);
void usb_delay_ms(int ms)
{
	volatile int i;

	for (i = 0; i < 1000; i++)
		usb_delay_us(ms);
}

/*
 *  usb_queue_request(ur)
 *
 *  Call the transfer handler in the host controller driver to
 *  set up a transfer descriptor
 *
 *  Input parameters:
 *   ur - request to queue
 *
 *  Return value:
 *   0 if ok
 *   else error
 */

int usb_queue_request(usbreq_t *ur)
{
	int res;

	REQTRACE(printf("Queue %p (%s,%s)\n", ur, eptname(ur), devname(ur)));

	ur->ur_inprogress = 1;
	ur->ur_xferred = 0;
#ifdef USBHOST_INTERRUPT_MODE
	ohci_enter_critical();
#endif
	res = UBXFER(ur->ur_dev->ud_bus,	/*ohci_xfer, ehci_xfer */
		     ur->ur_pipe->up_hwendpoint, ur);

	if (res == -1) {
		/*something is wrong.*/
		ur->ur_inprogress = 0;
		ur->ur_status = -1;
	}
#ifdef USBHOST_INTERRUPT_MODE
	ohci_exit_critical();
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
 *   ur - request to wait for
 *
 *  Return value:
 *   request status
 */

int usb_wait_request(usbreq_t *ur)
{
	while ((volatile int)(ur->ur_inprogress))
		usb_poll(ur->ur_dev->ud_bus);

	return ur->ur_status;
}

/*
 *  usb_sync_request(ur)
 *
 *  Synchronous request - call usb_queue and then usb_wait
 *
 *  Input parameters:
 *   ur - request to submit
 *
 *  Return value:
 *   status of request
 */

int usb_sync_request(usbreq_t *ur)
{
	usb_queue_request(ur);
	return usb_wait_request(ur);
}

/*
 *  usb_make_sync_request(dev,epaddr,buf,len,flags)
 *
 *  Create a request, wait for it to complete, and delete
 *  the request.  A shorthand**2 routine.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *   status of request
 */

int usb_make_sync_request(usbdev_t *dev, int epaddr, u8_t *buf, int length,
			  int flags)
{
	usbreq_t *ur;
	int res;

	if (dev == NULL)
		return K_USB_DEVICENOTRESPONDING;
	ur = usb_make_request(dev, epaddr, buf, length, flags);
	if (ur == NULL)
		return K_USB_DEVICENOTRESPONDING;
	res = usb_sync_request(ur);
	usb_free_request(ur);
	return res;
}

/*
 *  usb_make_sync_request_xferred(dev,epaddr,buf,len,flags,xferred)
 *
 *  Create a request, wait for it to complete, and delete
 *  the request, returning the amount of data transferred.
 *  A shorthand**3 routine.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   flags - transfer direction, etc. (UR_xxx flags)
 *      xferred - pointer to where to store xferred data length
 *
 *  Return value:
 *   status of request
 */
int usb_make_sync_request_xferred(usbdev_t *dev, int epaddr, u8_t *buf,
				  int length, int flags, int *xferred)
{
	usbreq_t *ur;
	int res;

	if (dev == NULL)
		return K_USB_DEVICENOTRESPONDING;
	ur = usb_make_request(dev, epaddr, buf, length, flags);
	if (ur == NULL)
		return K_USB_DEVICENOTRESPONDING;
	res = usb_sync_request(ur);
	*xferred = ur->ur_xferred;
	usb_free_request(ur);
	return res;
}

/*
 *  usb_simple_request(dev,reqtype,bRequest,wValue,wIndex)
 *
 *  Handle a simple USB control pipe request.  These are OUT
 *  requests with no data phase.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   reqtype - request type (bmRequestType) for descriptor
 *   wValue - wValue for descriptor
 *   wIndex - wIndex for descriptor
 *
 *  Return value:
 *   0 if ok
 *   else error
 */

int usb_simple_request(usbdev_t *dev, u8_t reqtype, int bRequest,
			int wValue, int wIndex)
{
	u8_t dbg_buffer[8] = { 0 };

	/*return usb_std_request(dev,reqtype,bRequest,wValue,wIndex,NULL,0);*/
		return usb_std_request(dev, reqtype, bRequest, wValue, wIndex,
					   (u8_t*)dbg_buffer, 0);

#if 0
		/*return usb_std_request(dev,reqtype,bRequest,wValue,wIndex,NULL,0);*/
		return usb_std_request(dev, reqtype, bRequest, wValue, wIndex,
				   USH_ADDR_2_UNCACHED(*&dbg_buffer), 0);
#endif
}

/*
 *  usb_set_configuration(dev,config)
 *
 *  Set the current configuration for a USB device.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   config - bConfigValue for the device
 *
 *  Return value:
 *   request status
 */

int usb_set_configuration(usbdev_t *dev, int config)
{
	int res;

	res =
	    usb_simple_request(dev, 0x00, USB_REQUEST_SET_CONFIGURATION, config,
			       0);

	return res;
}

/*
 *  usb_set_feature(dev,feature)
 *
 *  Set the a feature for a USB device.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   feature - feature for the device
 *
 *  Return value:
 *   request status
 */

int usb_set_feature(usbdev_t *dev, int feature)
{
	int res;

	res =
	    usb_simple_request(dev, 0x00, USB_REQUEST_SET_FEATURE, feature, 0);

	return res;
}

/*
 *  usb_new_address(bus)
 *
 *  Return the next available address for the specified bus
 *
 *  Input parameters:
 *   bus - bus to assign an address for
 *
 *  Return value:
 *   new address, <0 if error
 */

int usb_new_address(usbbus_t *bus)
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
 *  usb_set_address(dev,address)
 *
 *  Set the address of a device.  This also puts the device
 *  in the master device table for the bus and reconfigures the
 *  address of the control pipe.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   address - new address (1..127)
 *
 *  Return value:
 *   request status
 */

int usb_set_address(usbdev_t *dev, int address)
{
	int res;
	int idx;
	usbpipe_t *pipe;

	if ((address < 0) || (address >= USB_MAX_DEVICES))
		return 0;
	res =
	    usb_simple_request(dev, 0x00, USB_REQUEST_SET_ADDRESS, address, 0);

	if (res == 0) {
		dev->ud_bus->ub_devices[address] = dev;
		dev->ud_address = address;
		for (idx = 0; idx < UD_MAX_PIPES_PER_DEV; idx++) {
			pipe = dev->ud_pipes[idx];
			if (pipe && pipe->up_hwendpoint)
				/* ohci_ept_setadr() */
				UBEPTSETADDR(dev->ud_bus, pipe->up_hwendpoint, address);
		}
	}

	return res;
}

/*
 *  usb_set_ep0mps(dev,mps)
 *
 *  Set the maximum packet size of endpoint zero (mucks with the
 *  endpoint in the host controller)
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   mps - max packet size for endpoint zero
 *
 *  Return value:
 *   request status
 */

int usb_set_ep0mps(usbdev_t *dev, int mps)
{
	usbpipe_t *pipe;

	pipe = dev->ud_pipes[0];
	if (pipe && pipe->up_hwendpoint) /* ohci_ept_setmps */
		UBEPTSETMPS(dev->ud_bus, pipe->up_hwendpoint, mps);
	if (pipe)
		pipe->up_mps = mps;

	return 0;
}

/*
 *  usb_clear_stall(dev,epaddr)
 *
 *  Clear a stall condition on the specified pipe
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   pipenum - pipe number
 *
 *  Return value:
 *   0 if ok
 *   else error
 */
int usb_clear_stall(usbdev_t *dev, int pipenum)
{
	u8_t requestbuf[32] = { 0 };
	usb_device_request_t *req;
	usbreq_t *ur;
	int pipeidx;

	/*
	 *Clear the stall in the hardware.
	 */

	pipeidx = pipenum;

	UBEPTCLEARTOGGLE(dev->ud_bus, dev->ud_pipes[pipeidx]->up_hwendpoint);

	/*
	 *Do the "clear stall" request.   Note that we should do this
	 *without calling usb_simple_request, since usb_simple_request
	 *may itself stall.
	 */

	req = (usb_device_request_t *) requestbuf;

	req->bmRequestType = 0x02;
	req->bRequest = USB_REQUEST_CLEAR_FEATURE;
	PUTUSBFIELD(req, wValue, 0);	/*ENDPOINT_HALT */
	PUTUSBFIELD(req, wIndex, pipenum);
	PUTUSBFIELD(req, wLength, 0);

	ur = usb_make_request(dev, 0, requestbuf,
			      sizeof(usb_device_request_t), UR_FLAG_SETUP);
	usb_sync_request(ur);
	usb_free_request(ur);
	usb_make_request(dev, 0, requestbuf, 0, UR_FLAG_STATUS_IN);
	usb_sync_request(ur);
	usb_free_request(ur);

	return 0;
}

/*
 *  usb_std_request(dev,bmRequestType,bRequest,wValue,
 *                   wIndex,buffer,length)
 *
 *  Do a standard control request on the control pipe,
 *  with the appropriate setup, data, and status phases.
 *
 *  Input parameters:
 *   dev - dev we're talking to
 *   bmRequestType,bRequest,wValue,wIndex - fields for the
 *            USB request structure
 *   buffer - user buffer
 *   length - length of user buffer
 *
 *  Return value:
 *   number of bytes transferred
 */

int usb_std_request(usbdev_t *dev, u8_t bmRequestType,
		    u8_t bRequest, u16_t wValue,
		    u16_t wIndex, u8_t *buffer, int length)
{
	usbpipe_t *pipe = dev->ud_pipes[0];
	usbreq_t *ur;
	int res;
	usb_device_request_t *req;
	usb_device_descr_t *dev_desc;
#if 0
	u8_t databuf_s[1024] = { 0 };
	u8_t req_std_s[32] = { 0 };
#endif
	u8_t *databuf = (databuf_s);
	u8_t *req_std = (req_std_s);

#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t) databuf, 1024);
	clean_n_invalidate_dcache_by_addr((u32_t) req_std, 32);
	clean_n_invalidate_dcache_by_addr((u32_t) buffer, (u32_t)length);
#endif
	req = (usb_device_request_t *) req_std;

	if ((buffer != NULL) && (length != 0)) {
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
	ur = usb_make_request(dev, 0, (u8_t *) req,
			      sizeof(usb_device_request_t), UR_FLAG_SETUP);
	/* FIXME: integrate this into structure*/
	ur->ur_response = buffer;
	ur->ur_reslength = length;
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)ur, (u32_t)sizeof(usbreq_t));
#endif
	res = usb_sync_request(ur);

	/*Obtain bcdUSB from device descriptor for USB2.0 device. */
	if ((bRequest == USB_REQUEST_GET_DESCRIPTOR)
	    && ((wValue >> 8) == USB_DEVICE_DESCRIPTOR_TYPE)) {
/*            usbHostVer = *(ur->ur_response + 3);*/
		dev_desc = (usb_device_descr_t *) (ur->ur_response);
		usbHostVer = dev_desc->bcdUSBHigh;
	}

	usb_free_request(ur);

	/* This is only for USB1.x, as USB2.0 setup, data and status
	 * phases are done in ehci_xfer() together.
	 */
	if ((usbHostVer != 2) || (usbHostOhci == 1)) {
		/* #2 data packet */
		if (length != 0) {
			if (bmRequestType & USBREQ_DIR_IN) {
				ur = usb_make_request(dev, 0, databuf, length,
						      UR_FLAG_IN);
			} else {
				ur = usb_make_request(dev, 0, databuf, length,
						      UR_FLAG_OUT);
			}
			/* FIXME: integrate this into structure*/
			ur->ur_response = buffer;
			ur->ur_reslength = length;

			res = usb_sync_request(ur);

			if (res == K_OHCI_CC_STALL) {	/*STALL */
				usb_clear_stall(dev, pipe->up_num);
				usb_free_request(ur);
				return 0;
			}

			length = ur->ur_xferred;
			usb_free_request(ur);
		}

		if ((length != 0) && (bmRequestType & USBREQ_DIR_IN))
			memcpy(buffer, databuf, length);

		/* #3 status packet */
		if (bmRequestType & USBREQ_DIR_IN)
			ur = usb_make_request(dev, 0, (u8_t *) req, 0,
					      UR_FLAG_STATUS_OUT);
		else
			ur = usb_make_request(dev, 0, (u8_t *) req, 0,
					      UR_FLAG_STATUS_IN);

		/* FIXME: integrate this into structure*/
		ur->ur_response = buffer;
		ur->ur_reslength = length;
#ifdef CONFIG_DATA_CACHE_SUPPORT
		clean_n_invalidate_dcache_by_addr((u32_t)ur,
						 (u32_t)sizeof(usbreq_t));
#endif
		res = usb_sync_request(ur);
		usb_free_request(ur);

		if (res == K_OHCI_CC_STALL) {	/*STALL */
			usb_clear_stall(dev, pipe->up_num);
			return 0;
		}
	}

	return length;
}

/*
 *  usb_get_descriptor(dev,reqtype,dsctype,dscidx,respbuf,buflen)
 *
 *  Request a descriptor from the device.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   reqtype - bmRequestType field for descriptor we want
 *   dsctype - descriptor type we want
 *   dscidx - index of descriptor we want (often zero)
 *   respbuf - response buffer
 *   buflen - length of response buffer
 *
 *  Return value:
 *   number of bytes transferred
 */

int usb_get_descriptor(usbdev_t *dev, u8_t reqtype, int dsctype, int dscidx,
		       u8_t *respbuf, int buflen)
{
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)respbuf, (u32_t)buflen);
#endif
	return usb_std_request(dev,
			       reqtype, USB_REQUEST_GET_DESCRIPTOR,
			       USB_DESCRIPTOR_TYPEINDEX(dsctype, dscidx),
			       0, respbuf, buflen);
}

/*
 *  usb_get_string_descriptor(dev,reqtype,dsctype,dscidx,respbuf,buflen)
 *
 *  Request a descriptor from the device.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   dscidx - index of descriptor we want
 *   respbuf - response buffer
 *   buflen - length of response buffer
 *
 *  Return value:
 *   number of bytes transferred
 */

static int usb_get_string_descriptor(usbdev_t *dev, int dscidx,
				     u8_t *respbuf, int buflen)
{
	return usb_std_request(dev, USBREQ_DIR_IN,
			       USB_REQUEST_GET_DESCRIPTOR,
			       USB_DESCRIPTOR_TYPEINDEX(USB_STRING_DESCRIPTOR_TYPE,
			       dscidx), 0x0409, /* Microsoft lang code for Eng */
			       respbuf, buflen);
}

/*
 *  usb_get_string(dev,id,buf,maxlen)
 *
 *  Request a string from the device, converting it from
 *  unicode to ascii (brutally).
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   id - string ID
 *   buf - buffer to receive string (null terminated)
 *   maxlen - length of buffer
 *
 *  Return value:
 *   number of characters in returned string
 */

int usb_get_string(usbdev_t *dev, int id, char *buf, int maxlen)
{
	int amtcopy;
	u8_t *respbuf;
	int idx;
	usb_string_descr_t *sdscr;
	u8_t databuf[64] = { 0 };

	respbuf = addr2uncached(databuf);
	sdscr = (usb_string_descr_t *) respbuf;

	/*
	 *First time just get the header of the descriptor so we can
	 *get the string length
	 */

	amtcopy = usb_get_string_descriptor(dev, id, respbuf, 2);

	/*
	 *now do it again to get the whole string.
	 */

	if (maxlen > sdscr->bLength)
		maxlen = sdscr->bLength;

	amtcopy = usb_get_string_descriptor(dev, id, respbuf, maxlen);

	*buf = '\0';
	amtcopy = sdscr->bLength - 2;
	if (amtcopy <= 0)
		return amtcopy;

	for (idx = 0; idx < amtcopy; idx += 2)
		*buf++ = sdscr->bString[idx];

	*buf = '\0';

	return amtcopy;
}

/*
 *  usb_get_device_descriptor(dev,dscr,smallflg)
 *
 *  Request the device descriptor for the device. This is often
 *  the first descriptor requested, so it needs to be done in
 *  stages so we can find out how big the control pipe is.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   dscr - pointer to buffer to receive descriptor
 *   smallflg - TRUE to request just 8 bytes.
 *
 *  Return value:
 *   number of bytes copied
 */

int usb_get_device_descriptor(usbdev_t *dev, usb_device_descr_t *dscr,
			      int smallflg)
{
	int res;
#if 0
	u8_t respbuf_s[256] = { 0 };
#endif
	u8_t *respbuf = addr2uncached(respbuf_s);
	int amtcopy;

	/*
	 *Smallflg truncates the request 8 bytes.  We need to do this for
	 *the very first transaction to a USB device in order to determine
	 *the size of its control pipe.  Bad things will happen if you
	 *try to retrieve more data than the control pipe will hold.
	 *
	 *So, be conservative at first and get the first 8 bytes of the
	 *descriptor.   Byte 7 is bMaxPacketSize0, the size of the control
	 *pipe.  Then you can go back and submit a bigger request for
	 *everything else.
	 */

	amtcopy =
	    smallflg ? USB_CONTROL_ENDPOINT_MIN_SIZE :
	    sizeof(usb_device_descr_t);

	res =
	    usb_get_descriptor(dev, USBREQ_DIR_IN, USB_DEVICE_DESCRIPTOR_TYPE,
			       0, respbuf, amtcopy);
	memcpy(dscr, respbuf, amtcopy);

	return res;

}

/*
 *  usb_get_config_descriptor(dev,dscr,idx,maxlen)
 *
 *  Request the configuration descriptor from the device.
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   dscr - descriptor buffer (receives data from device)
 *   idx - index of config we want (usually zero)
 *   maxlen - total size of buffer to receive descriptor
 *
 *  Return value:
 *   number of bytes copied
 */

int usb_get_config_descriptor(usbdev_t *dev, usb_config_descr_t *dscr,
			      int idx, int maxlen)
{
	int res;
#if 0
	u8_t respbuf_s[1088] = { 0 };
#endif
	u8_t *respbuf = addr2uncached(respbuf_s);
	maxlen = MIN(maxlen, (int)sizeof(respbuf_s));
	res = usb_get_descriptor(dev, USBREQ_DIR_IN,
				 USB_CONFIGURATION_DESCRIPTOR_TYPE, idx,
				 respbuf, maxlen);
	memcpy(dscr, respbuf, MIN(maxlen, res));
	return res;

}

/*
 *  usb_get_device_status(dev,status)
 *
 *  Request status from the device (status descriptor)
 *
 *  Input parameters:
 *   dev - device we're talking to
 *   status - receives device_status structure
 *
 *  Return value:
 *   number of bytes returned
 */

int usb_get_device_status(usbdev_t *dev, usb_device_status_t *status)
{
	return usb_std_request(dev,
			       USBREQ_DIR_IN,
			       0,
			       0,
			       0,
			       (u8_t *) status, sizeof(usb_device_status_t));
}

/*
 *  usb_complete_request(ur,status)
 *
 *  Called when a usb request completes - pass status to
 *  caller and call the callback if there is one.
 *
 *  Input parameters:
 *   ur - usbreq_t to complete
 *   status - completion status
 *
 *  Return value:
 *   nothing
 */

void usb_complete_request(usbreq_t *ur, int status)
{
	REQTRACE(printf
		 ("Complete %p (%s,%s) Status=%d\n", ur, eptname(ur),
		  devname(ur), status));

	ur->ur_status = status;
	ur->ur_inprogress = 0;

	/* usbhub_ireq_callback, usbhid_ireq_callback*/
	if (ur->ur_callback)
		ur->ur_status = (*(ur->ur_callback)) (ur);

	/*de-initialize the UR after completion of request*/
	ur = NULL;
}

/*
 *  usb_initroot(bus)
 *
 *  Initialize the root hub for the bus - we need to do this
 *  each time a bus is configured.
 *
 *  Input parameters:
 *   bus - bus to initialize
 *
 *  Return value:
 *   nothing
 */
static u8_t buf_root[64];
void usb_initroot(usbbus_t *bus)
{
	usbdev_t *dev;
	usb_driver_t *drv;
	int addr;
	int res;
	u8_t *buf;
	int len;
	usb_config_descr_t cfgdescr;

	printf("%s!\n", __func__);

	/*
	 *Create a device for the root hub.
	 */
	dev = usb_create_device(bus, USB_ROOT_HUB_DEV_NUM, 0);
	bus->ub_roothub = dev;

	/*there is no usbhub simulation for ehci controller. */
	if (usbHostOhci) {
		/*
		 *Get the device descriptor.  Make sure it's a hub.
		 */
		res = usb_get_device_descriptor(dev, &(dev->ud_devdescr), true);

		if (dev->ud_devdescr.bDeviceClass != USB_DEVICE_CLASS_HUB) {
			printf("Error! Root device is not a hub!\n");
			return;
		}
	}

	/*
	 *Set up the max packet size for the control endpoint,
	 *then get the rest of the descriptor.
	 */

	usb_set_ep0mps(dev, dev->ud_devdescr.bMaxPacketSize0);
	res = usb_get_device_descriptor(dev, &(dev->ud_devdescr), false);
	/*
	 *Obtain a new address and set the address of the
	 *root hub to this address.
	 */

	addr = usb_new_address(dev->ud_bus);
	res = usb_set_address(dev, addr);

	/*
	 *Get the configuration descriptor and all the
	 *associated interface and endpoint descriptors.
	 */
	res = usb_get_config_descriptor(dev, &cfgdescr, 0,
					sizeof(usb_config_descr_t));
	if (res != sizeof(usb_config_descr_t))
		printf("[a]usb_get_config_descriptor returns %d\n", res);

	len = GETUSBFIELD(&cfgdescr, wTotalLength);
	buf = buf_root;

	res =
	    usb_get_config_descriptor(dev, (usb_config_descr_t *) buf, 0, len);
	if (res != len) {
		printf
		    ("[b]usb_get_config_descriptor returns resLen=%d, len=%d\n",
		     res, len);
	}

	dev->ud_cfgdescr = (usb_config_descr_t *) buf;

	/*
	 *Select the configuration.  Not really needed for our poor
	 *imitation root hub, but it's the right thing to do.
	 */

	usb_set_configuration(dev, cfgdescr.bConfigurationValue);

	/*
	 *Find the driver for this.  It had better be the hub
	 *driver.
	 */

	drv = usb_find_driver(dev);

	/*
	 *Call the attach method.
	 */
	if (drv->udrv_attach)
		(*(drv->udrv_attach)) (dev, drv); /* usbhub_attach*/

	/*
	 *Hub should now be operational.
	 */

}

/*
 *  usb_find_cfg_descr(dev,dtype,idx)
 *
 *  Find a configuration descriptor - we retrieved all the config
 *  descriptors during discovery, this lets us dig out the one
 *  we want.
 *
 *  Input parameters:
 *   dev - device we are talking to
 *   dtype - descriptor type to find
 *   idx - index of descriptor if there's more than one
 *
 *  Return value:
 *   pointer to descriptor or NULL if not found
 */

void *usb_find_cfg_descr(usbdev_t *dev, int dtype, int idx)
{
	u8_t *endptr;
	u8_t *ptr;
	usb_config_descr_t *cfgdscr;

	if (dev->ud_cfgdescr == NULL)
		return NULL;

	ptr = (u8_t *) dev->ud_cfgdescr;
	endptr = ptr + GETUSBFIELD((dev->ud_cfgdescr), wTotalLength);
	while (ptr < endptr) {

		cfgdscr = (usb_config_descr_t *) ptr;
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
 *  controlTransfer(usbdev_t *dev,u8_t bmRequestType,
 *		    u8_t bRequest,u16_t wValue,
 *                  u16_t wIndex,u8_t *buffer,int length)
 *
 *  Do a standard control request on the control pipe,
 *  with the appropriate setup, data, and status phases.
 *
 *  Input parameters:
 *   dev - dev we're talking to
 *   bmRequestType,bRequest,wValue,wIndex - fields for the
 *            USB request structure
 *   buffer - user buffer
 *   length - length of user buffer
 *
 *  Return value:
 *   number of bytes transferred
 */
int controlTransfer(usbdev_t *dev, u8_t bmRequestType, u8_t bRequest,
		    u16_t wValue, u16_t wIndex, u8_t *buffer,
		    int length)
{
	return usb_std_request(dev, bmRequestType, bRequest, wValue, wIndex,
			       buffer, length);
}

/*
 *  generalTransfer(usbdev_t *dev,int epaddr,u8_t *buf,int length,
 *                  int eptype, int flags)
 *
 *  Create a general transfer request(bulk, intr request.),
 *  wait for it to complete, and delete
 *  the request.  A shorthand**2 routine.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   eptype - endpoint type
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *   status of request
 */
int generalTransfer(usbdev_t *dev, int epaddr, u8_t *buf, int length,
		    int eptype, int flags)
{
	ARG_UNUSED(eptype);
	usbreq_t *ur;
	int res;

	if (dev == NULL)
		return K_USB_DEVICENOTRESPONDING;

	ur = usb_make_request(dev, epaddr, buf, length, flags);
	if (ur == NULL)
		return K_USB_DEVICENOTRESPONDING;
	res = usb_sync_request(ur);
	usb_free_request(ur);
	return res;
}

/*
 *  controlRW(usbdev_t *dev,u8_t bmRequestType, u8_t bRequest,u16_t wValue,
 *                  u16_t wIndex,u8_t *buffer,int length)
 *
 *  Common read/write routine for control transfer request.
 *
 *  Input parameters:
 *   dev - dev we're talking to
 *   bmRequestType,bRequest,wValue,wIndex - fields for the
 *            USB request structure
 *   buffer - user buffer
 *   length - length of user buffer
 *
 *  Return value:
 *   number of bytes transferred
 */
int controlRW(usbdev_t *dev, u8_t requestType, u8_t request,
	      u32_t value, u32_t index, u8_t *buf, u32_t len)
{
	return controlTransfer(dev, requestType, request, value, index, buf,
			       len);
}

/*
 *  bulkRW(usbdev_t *dev,int epaddr,u8_t *buf,int length,
 *                  int eptype, int flags)
 *
 *  Common read/write routine for bulk transfer request.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   eptype - endpoint type
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *   status of request
 */
int bulkRW(usbdev_t *dev, int epaddr, u8_t *buf, int length, int flags)
{
	return generalTransfer(dev, epaddr, buf, length, UP_TYPE_BULK, flags);
}

/*
 *  interruptRW(usbdev_t *dev,int epaddr,u8_t *buf,int length,
 *                  int eptype, int flags)
 *
 *  Common read/write routine for interrupt transfer request.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *   buf,length - user buffer and buffer length
 *   eptype - endpoint type
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *   status of request
 */
int interruptRW(usbdev_t *dev, int epaddr, u8_t *buf, int length,
		int flags)
{
	return generalTransfer(dev, epaddr, buf, length, UP_TYPE_INTR, flags);
}

/*
 *  isochronousRW(usbdev_t *dev,int epaddr,u8_t *buf,int length,
 *                  int eptype, int flags)
 *
 *  Common read/write routine for isochronous transfer request.
 *
 *  Input parameters:
 *   dev- device we're talking to
 *   epaddr - endpoint address, from usb_open_pipe()
 *	   buf,length - user buffer and buffer length
 *   flags - transfer direction, etc. (UR_xxx flags)
 *
 *  Return value:
 *	   status of request
 */
int isochronousRW(usbdev_t *dev, int epaddr, u8_t *buf, int length,
		  int flags, int (*ur_callback) (struct usbreq_s *req))
{
	usbreq_t *ur;
	int res;

	if (dev == NULL)
		return K_USB_DEVICENOTRESPONDING;
	ur = usb_make_request(dev, epaddr, buf, length, flags);
	if (ur_callback)
		ur->ur_callback = ur_callback;

	if (ur == NULL)
		return K_USB_DEVICENOTRESPONDING;
	res = usb_sync_request(ur);
	usb_free_request(ur);
	return res;
}
