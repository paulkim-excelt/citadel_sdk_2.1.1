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

/* @file usb_host.h
 *
 * USB Host protocol stack.
 *
 * Contains data structures and function prototypes
 * to interface with usb host controller, usb root hub
 * and class drivers.
 *
 */

#ifndef __USB_HOST_H__
#define __USB_HOST_H__

#include <kernel.h>
#include "usbchap9.h"

/*  USB Request error codes */
#define UR_ERR_NOERROR			0
#define UR_ERR_CRC			1
#define UR_ERR_BITSTUFFING		2
#define UR_ERR_DATATOGGLEMISMATCH	3
#define UR_ERR_STALL			4
#define UR_ERR_DEVICENOTRESPONDING	5
#define UR_ERR_PIDCHECKFAILURE		6
#define UR_ERR_UNEXPECTEDPID		7
#define UR_ERR_DATAOVERRUN		8
#define UR_ERR_DATAUNDERRUN		9
#define UR_ERR_BUFFEROVERRUN		12
#define UR_ERR_BUFFERUNDERRUN		13
#define UR_ERR_NOTACCESSED		15
#define UR_ERR_CANCELLED		0xFF

/* USB Bus structure
 * One per host controller.
 */
#define USB_MAX_BUS		4
#define UHUB_MAX_DEVICES	2
#define USB_MAX_DEVICES		(UHUB_MAX_DEVICES + 1)
#define USB_ROOT_HUB_DEV_NUM	0
#define USB_PORT_1_DEV_NUM	1
#define USB_PORT_2_DEV_NUM	2
#define USB_PORT_3_DEV_NUM	3
#define USB_PORT_4_DEV_NUM	4
#define USB_PORT_5_DEV_NUM	5
#define USB_PORT_6_DEV_NUM	6
#define USB_PORT_7_DEV_NUM	7
#define USB_PORT_8_DEV_NUM	8

/* Flag indicating some device on bus needs scanning */
#define UB_FLG_NEEDSCAN 1

struct usbbus {
	struct usbbus *ub_next;
	void *ub_hwsoftc;
	struct usb_hcd *ub_hwdisp;
	struct usbdev *ub_roothub;
	struct usbdev *ub_devices[USB_MAX_DEVICES];
	unsigned int ub_flags;
	int ub_num;
	struct device *dev;
};

/* USB Pipe structure
 * One per unidirectional channel to an endpoint on a USB device
 */

#define UP_TYPE_CONTROL	0x0001
#define UP_TYPE_BULK	0x0002
#define UP_TYPE_INTR	0x0004
#define UP_TYPE_ISOC	0x0008

#define UP_TYPE_LOWSPEED	0x0010
#define UP_TYPE_FULLSPEED	0x0020
#define UP_TYPE_HIGHSPEED	0x0040

#define UP_TYPE_IN	0x0080
#define UP_TYPE_OUT	0x0100

#define UD_MAX_PIPES_PER_DEV	32
#define USB_EPADDR_TO_IDX(epaddr) (((epaddr & 0x80) >> 3) | (epaddr & 0x0F))

struct usbpipe {
	void *up_hwendpoint;
	struct usbdev *up_dev;
	int up_num;
	int up_mps;
	int up_flags;
};

/* USB device structure
 * One per device attached to the USB
 * This is the basic structure applications will use to
 * refer to a device.
 */

#define UD_FLAG_HUB	0x0001	/* hub device */
#define UD_FLAG_ROOTHUB	0x0002	/* root hub device */
#define UD_FLAG_LOWSPEED 0x0008	/* lowspeed device */
#define UD_FLAG_REMOVING 0x0010	/* device being removed */

struct usbdev {
	struct usb_driver *ud_drv;
	struct usbbus *ud_bus;
	int ud_address;
	struct usbpipe *ud_pipes[UD_MAX_PIPES_PER_DEV];
	struct usbdev *ud_parent;
	int ud_flags;
	void *ud_private;
	struct usb_device_descr ud_devdescr;
	struct usb_config_descr *ud_cfgdescr;
	struct k_mutex ud_mutex;
};

/* USB Request
 * Basic structure to describe an in-progress
 * I/O request.  It associates buses, pipes, and buffers
 * together.
 */

#define UR_FLAG_SYNC		0x8000
#define UR_FLAG_SETUP		0x0001
#define UR_FLAG_IN		0x0002
#define UR_FLAG_OUT		0x0004
#define UR_FLAG_STATUS_IN	0x0008	/*status phase of a control WRITE */
#define UR_FLAG_STATUS_OUT	0x0010	/*status phase of a control READ */
#define UR_FLAG_SHORTOK		0x0020	/*short transfers are ok */

struct usbreq {
	struct k_fifo ur_qblock;
	struct usbdev *ur_dev;
	struct usbpipe *ur_pipe;
	u8_t *ur_buffer;
	int ur_length;
	int ur_xferred;
	int ur_status;
	int ur_flags;
	u8_t *ur_response;
	int ur_reslength;
	void *ur_ref;
	volatile int ur_inprogress;
	int (*ur_callback)(struct usbreq *req);
	void *ur_tdqueue;
	int ur_tdcount;
};

/*
 *  Host Controller structure and methods for
 *  abstracting the USB host controller.
 */

struct usb_hcd {
	struct usbbus *(*hcdrv_create)(u32_t regaddr);
	void (*hcdrv_delete)(struct usbbus *bus);
	int (*hcdrv_start)(struct usbbus *bus);
	void (*hcdrv_stop)(struct usbbus *bus);
	int (*hcdrv_intr)(struct usbbus *bus);
	void *(*hcdrv_ept_create)(struct usbbus *bus, int usbaddr, int eptnum,
				  int mps, int flags);
	void (*hcdrv_ept_delete)(struct usbbus *bus, void *ep);
	void (*hcdrv_ept_setmps)(struct usbbus *bus, void *ep, int mps);
	void (*hcdrv_ept_setaddr)(struct usbbus *bus, void *ep, int addr);
	void (*hcdrv_ept_cleartoggle)(struct usbbus *bus, void *ep);
	int (*hcdrv_xfer)(struct usbbus *bus, void *uept, struct usbreq *ur);
	int (*hcdrv_cancel_xfer)(struct usbbus *bus, void *uept,
				 struct usbreq *ur);
};

#define UBCREATE(driver, addr) (*((driver)->hcdrv_create))(addr)
#define UBDELETE(bus) (*((bus)->ub_hwdisp->hcdrv_delete))(bus)
#define UBSTART(bus) (*((bus)->ub_hwdisp->hcdrv_start))(bus)
#define UBSTOP(bus) (*((bus)->ub_hwdisp->hcdrv_stop))(bus)
#define UBINTR(bus) (*((bus)->ub_hwdisp->hcdrv_intr))(bus)
#define UBEPTCREATE(bus, addr, num, mps, flags) \
	(*((bus)->ub_hwdisp->hcdrv_ept_create))(bus, addr, num, mps, flags)
#define UBEPTDELETE(bus, ept) (*((bus)->ub_hwdisp->hcdrv_ept_delete))(bus, ept)
#define UBEPTSETMPS(bus, ept, mps) \
	 (*((bus)->ub_hwdisp->hcdrv_ept_setmps))(bus, ept, mps)
#define UBEPTSETADDR(bus, ept, addr) \
	(*((bus)->ub_hwdisp->hcdrv_ept_setaddr))(bus, ept, addr)
#define UBEPTCLEARTOGGLE(bus, ept) \
	(*((bus)->ub_hwdisp->hcdrv_ept_cleartoggle))(bus, ept)
#define UBXFER(bus, ept, xfer) \
	(*((bus)->ub_hwdisp->hcdrv_xfer))(bus, ept, xfer)
#define UBCANCELXFER(bus, ept, xfer) \
	(*((bus)->ub_hwdisp->hcdrv_cancel_xfer))(bus, ept, xfer)
/*
 *  Devices - methods for abstracting things that _use_ USB
 *  (devices you can plug into the USB) - the entry points
 *  here are basically just for device discovery, since the top half
 *  of the actual driver will be device-specific.
 */

struct usb_driver {
	char *udrv_name;
	int (*udrv_attach)(struct usbdev *dev, struct usb_driver *drv);
	int (*udrv_detach)(struct usbdev *dev);
};

struct usb_drv_list {
	int udl_class;
	int udl_vendor;
	int udl_product;
	struct usb_driver *udl_disp;
};

#define CLASS_ANY	-1
#define VENDOR_ANY	-1
#define PRODUCT_ANY	-1

#define IS_HUB(dev) ((dev)->ud_devdescr.bDeviceClass == USB_DEVICE_CLASS_HUB)
#define CONFIG_DESC_SIZE	64

struct usb2h_data {
	bool init;	/* Host initialized */
	int cur_host;	/* OHCI or EHCI */
	int host_ver;	/* Version */
	u32_t usbh_removed;
	u32_t usb_buscnt;
	struct usbbus *usb_buses[USB_MAX_BUS];
	struct usbdev *usbdev;
	struct usbpipe *pipe_rh[UD_MAX_PIPES_PER_DEV];
	struct usbpipe *pipe_dev[UD_MAX_PIPES_PER_DEV];
	k_tid_t tid;
};

/*
 ************************
 * USB host driver APIs *
 ************************
 */

/**
 *  @brief usb_create_device(bus,idx,lowspeed)
 *
 *  @details
 *	Create a new USB device.  This device will be set to
 *	communicate on address zero (default address) and will be
 *	ready for basic stuff so we can figure out what it is.
 *	The control pipe will be open, so you can start requesting
 *	descriptors right away.
 *
 *  @param bus - bus to create device on
 *  @param idx - usb device number
 *  @param lowspeed - true if it's a lowspeed device (the hubs tell
 *		      us these things)
 *
 *  @return
 *	usb device structure, or NULL
 */
struct usbdev *usb_create_device(struct usbbus *bus, int idx, int lowspeed);

/**
 *  @brief usb_destroy_device(usbdev)
 *
 *  @details
 *	Delete an entire USB device, closing its pipes and freeing
 *	the device data structure
 *
 *  @param usbdev - device to destroy
 *
 *  @return
 *	nothing
 */
void usb_destroy_device(struct usbdev *usbdev);

/**
 *  @brief usb_poll(bus)
 *
 *  @details
 *	Handle device-driver polling - simply vectors to host controller
 *	driver.
 *
 *  @param bus - bus structure
 *
 *  @return
 *	nothing
 */
void usb_poll(struct usbbus *bus);

/**
 *  @brief usbhost_init(dev)
 *
 *  @details
 *	Initialize host controller
 *
 *  @param dev - pointer to usbhost device
 *  @param host - 0 for OHCI, 1 for EHCI
 *
 *  @return
 *	0 if ok else ERROR
 */
int usbhost_init(struct device *dev, int host);

/**
 *  @brief usb_clear_stall(usbdev, pipenum)
 *
 *  @details Clear a stall condition on the specified pipe
 *
 *  @param usbdev - device we're talking to
 *  @param pipe - pipe number
 *
 *  @return
 *	0 if ok
 *	else error
 */
int usb_clear_stall(struct usbdev *usbdev, int pipe);

/*
 * USB Pipe APIs
 */

/**
 *  @brief usb_create_pipe(usbdev, epaddr, mps, flags)
 *
 *  @details
 *	Create a pipe, causing the corresponding endpoint to
 *	be created in the host controller driver.  Pipes form the
 *	basic "handle" for unidirectional communications with a
 *	USB device.
 *
 *  @param usbdev - device we're talking about
 *  @param epaddr - endpoint address open, usually from the endpoint
 *		    descriptor
 *  @param mps - maximum packet size understood by the device
 *  @param flags - flags for this pipe (UP_xxx flags)
 *
 *  @return
 *	< 0 if error
 *	0 if ok
 */
int usb_create_pipe(struct usbdev *usbdev, int epaddr, int mps, int flags);


/**
 *  @brief usb_open_pipe(usbdev, epdesc)
 *
 *  @details
 *	Open a pipe given an endpoint descriptor - this is the
 *	normal way pipes get open, since you've just selected a
 *	configuration and have the descriptors handy with all
 *	the information you need.
 *
 *  @param usbdev - device we're talking to
 *  @param epdesc - endpoint descriptor
 *
 *  @return
 *	< 0 if error
 *	else endpoint address (from descriptor)
 */
int usb_open_pipe(struct usbdev *usbdev, struct usb_endpoint_descr *epdesc);


/**
 *  @brief usb_destroy_pipe(usbdev, epaddr)
 *
 *  @details Close(destroy) an open pipe and remove endpoint descriptor
 *
 *  @param usbdev - device we're talking to
 *  @param epaddr - pipe to close
 *
 *  @return
 *	nothing
 */
void usb_destroy_pipe(struct usbdev *usbdev, int epaddr);


/*
 *  @brief usb_destroy_all_pipes(usbdev)
 *
 *  @details Destroy all pipes related to this device.
 *
 *  @param usbdev - device we're clearing out
 *
 *  @return
 *	nothing
 */
void usb_destroy_all_pipes(struct usbdev *usbdev);

/*
 * USB Request APIs
 */

/**
 *  @brief usb_make_request(usbdev, pipenum, buf, len, flags)
 *
 *  @details
 *	Create a template request structure with basic fields
 *	ready to go. A shorthand routine.
 *
 *  @param usbdev- device we're talking to
 *  @param pipenum - pipenum from usb_open_pipe()
 *  @param buf - user buffer
 *  @param length - buffer length
 *  @param flags - transfer direction, etc. (UR_xxx flags)
 *
 *  @return
 *	struct usbreq pointer, or NULL
 */
struct usbreq *usb_make_request(struct usbdev *usbdev, int pipenum,
				u8_t *buf, int length, int flags);

/**
 *  @brief usb_cancel_request(ur)
 *
 *  @details Cancel a pending usb transfer request.
 *
 *  @param ur - request to cancel
 *
 *  @return
 *	0 if ok
 *	else error (could not find request)
 */
int usb_cancel_request(struct usbreq *ur);


/**
 *  @brief usb_free_request(ur)
 *
 *  @details Return a transfer request to the free pool.
 *
 *  @param ur - request to return
 *
 *  @return
 *	nothing
 */
void usb_free_request(struct usbreq *ur);


/**
 *  @brief usb_queue_request(ur)
 *
 *  @details
 *	Call the transfer handler in the host controller driver to
 *	set up a transfer descriptor
 *
 *  @param
 *	ur - request to queue
 *
 *  @return
 *	0 if ok
 *	else error
 */
int usb_queue_request(struct usbreq *ur);


/**
 *  @brief usb_wait_request(ur)
 *
 *  @details
 *	Wait until a request completes, calling the polling routine
 *	as we wait.
 *
 *  @param
 *	ur - request to wait for
 *
 *  @return
 *	request status
 */
int usb_wait_request(struct usbreq *ur);


/**
 *  @brief usb_sync_request(ur)
 *
 *  @details Synchronous request - call usb_queue and then usb_wait
 *
 *  @param
 *	ur - request to submit
 *
 *  @return
 *	status of request
 */
int usb_sync_request(struct usbreq *ur);


/**
 *  @brief usb_make_sync_request(usbdev, pipenum, buf, len, flags)
 *
 *  @details
 *	Create a request, wait for it to complete, and delete
 *	the request.  A shorthand**2 routine.
 *
 *  @param usbdev- device we're talking to
 *  @param pipenum - pipe numeber of endpoint from usb_open_pipe()
 *  @param buf - user buffer
 *  @param length - buffer length
 *  @param flags - transfer direction, etc. (UR_xxx flags)
 *
 *  @return
 *	status of request
 */
int usb_make_sync_request(struct usbdev *usbdev, int pipenum, u8_t *buf,
			  int length, int flags);


/**
 *  @brief usb_make_sync_request_xferred(usbdev, pipenum, buf, len,
 *					 flags, xferred)
 *
 *  @details
 *	Create a request, wait for it to complete, and delete
 *	the request, returning the amount of data transferred.
 *
 *  @param usbdev- device we're talking to
 *  @param pipenum - pipe numeber of endpoint from usb_open_pipe()
 *  @param buf - user buffer
 *  @param length - buffer length
 *  @param flags - transfer direction, etc. (UR_xxx flags)
 *  @param xferred - pointer to where to store xferred data length
 *
 *  @return
 *	status of request
 */
int usb_make_sync_request_xferred(struct usbdev *usbdev, int pipenum, u8_t *buf,
				  int length, int flags, int *xferred);


/**
 *  @brief usb_simple_request(usbdev, reqtype, bRequest, wValue, wIndex)
 *
 *  @details
 *	Handle a simple USB control pipe request.  These are OUT
 *	requests with no data phase.
 *
 *  @param usbdev - device we're talking to
 *  @param reqtype - request type (bmRequestType) for descriptor
 *  @param bRequest - request for descriptor
 *  @param wValue - wValue for descriptor
 *  @param wIndex - wIndex for descriptor
 *
 *  @return
 *	0 if ok
 *	 else error
 */
int usb_simple_request(struct usbdev *usbdev, u8_t reqtype, int bRequest,
		       int wValue, int wIndex);


/**
 *  @brief usb_complete_request(ur, status)
 *
 *  @details
 *	Called when a usb request completes - pass status to
 *	caller and call the callback if there is one.
 *
 *  @param ur - struct usbreq to complete
 *  @param status - completion status
 *
 *  @return
 *	nothing
 */
void usb_complete_request(struct usbreq *ur, int status);

/*
 * Configuration APIs
 */


/**
 *  @brief usb_get_descr(usbdev, reqtype, dsctype, dscidx, respbuf, buflen)
 *
 *  @details Request a descriptor from the device.
 *
 *  @param usbdev - device we're talking to
 *  @param reqtype - bmRequestType field for descriptor we want
 *  @param dsctype - descriptor type we want
 *  @param dscidx - index of descriptor we want (often zero)
 *  @param respbuf - response buffer
 *  @param buflen - length of response buffer
 *
 *  @return
 *	number of bytes transferred
 */
int usb_get_desc(struct usbdev *usbdev, u8_t reqtype, int dsctype, int dscidx,
		 u8_t *respbuf, int buflen);


/**
 *  @brief usb_get_config_descriptor(usbdev, dscr, idx, maxlen)
 *
 *  @details Request the configuration descriptor from the device.
 *
 *  @param usbdev - device we're talking to
 *  @param dscr - descriptor buffer (receives data from device)
 *  @param idx - index of config we want (usually zero)
 *  @param maxlen - total size of buffer to receive descriptor
 *
 *  @return
 *	number of bytes copied
 *
 */
int usb_get_config_descriptor(struct usbdev *usbdev,
			      struct usb_config_descr *dscr,
			      int idx, int maxlen);

/**
 *  @brief usb_get_device_descriptor(usbdev, dscr, smallflg)
 *
 *  @details
 *	Request the device descriptor for the device. This is often
 *	the first descriptor requested, so it needs to be done in
 *	stages so we can find out how big the control pipe is.
 *
 *  @param usbdev - device we're talking to
 *  @param dscr - pointer to buffer to receive descriptor
 *  @param smallflg - TRUE to request just 8 bytes.
 *
 *  @return
 *	number of bytes copied
 */
int usb_get_device_descriptor(struct usbdev *usbdev,
			      struct usb_device_descr *dscr,
			      int smallflg);


/**
 *  @brief usb_find_cfg_descr(usbdev, dtype, idx)
 *
 *  @details
 *	Find a configuration descriptor - we retrieved all the config
 *	descriptors during discovery, this lets us dig out the one
 *	we want.
 *
 *  @param usbdev - device we are talking to
 *  @param dtype - descriptor type to find
 *  @param idx - index of descriptor if there's more than one
 *
 *  @return
 *	pointer to descriptor or NULL if not found
 */
void *usb_find_cfg_descr(struct usbdev *usbdev, int dtype, int idx);


/**
 *  @brief usb_get_device_status(usbdev, status)
 *
 *  @details Request status from the device (status descriptor)
 *
 *  @param usbdev - device we're talking to
 *  @param status - receives device_status structure
 *
 *  @return
 *	number of bytes returned
 */
int usb_get_device_status(struct usbdev *usbdev,
			  struct usb_device_status *status);

/**
 *  @brief usb_set_address(usbdev, address)
 *
 *  @details
 *	Set the address of a device.  This also puts the device
 *	in the master device table for the bus and reconfigures the
 *	address of the control pipe.
 *
 *  @param usbdev - device we're talking to
 *  @param addr - new address (1..127)
 *
 *  @return
 *	request status
 */
int usb_set_address(struct usbdev *usbdev, int addr);

/**
 *  @brief usb_set_dev_configuration(usbdev, config)
 *
 *  @details Set the current configuration for a USB device.
 *
 *  @param usbdev - device we're talking to
 *  @param config - bConfigValue for the device
 *
 *  @return
 *	request status
 */
int usb_set_dev_configuration(struct usbdev *usbdev, int config);

/**
 *  @brief usb_set_feature(dev, feature)
 *
 *  @details Set the a feature for a USB device.
 *
 *  @param usbdev - device we're talking to
 *  @param feature - feature for the device
 *
 *  @return
 *	request status
 */
int usb_set_feature(struct usbdev *usbdev, int feature);

/**
 *  @brief usb_set_ep0mps(dev, mps)
 *
 * @details
 *	Set the maximum packet size of endpoint zero (mucks with the
 *	endpoint in the host controller)
 *
 *  @param usbdev - device we're talking to
 *  @param mps - max packet size for endpoint zero
 *
 *  @return
 *	request status
 */
int usb_set_ep0mps(struct usbdev *usbdev, int mps);

/**
 *  @brief usb_new_address(bus)
 *
 *  @details Return the next available address for the specified bus
 *
 *  @param
 *	bus - bus to assign an address for
 *
 *  @return
 *	new address, < 0 if error
 */
int usb_new_address(struct usbbus *bus);

/**
 *  @brief usb_get_string_descriptor(usbdev, dscidx, respbuf, buflen)
 *
 *  @details Request a descriptor from the device.
 *
 *  @param usbdev - device we're talking to
 *  @param dscidx - index of descriptor we want
 *  @param respbuf - response buffer
 *  @param len - length of response buffer
 *
 *  @return
 *	number of bytes transferred
 */
int usb_get_string(struct usbdev *usbdev, int dscidx, u8_t *respbuf, int len);


/**
 *  @brief usb_std_request(usbdev, bmRequestType, bRequest,wValue,
 *                  wIndex, buffer, length)
 *  @details
 *	Do a standard control request on the control pipe,
 *	with the appropriate setup, data, and status phases.
 *
 *  @param usbdev - dev we're talking to
 *  @param bmRequestType - bmRequestType field
 *  @param bRequest - bRequest field
 *  @param wValue - wValue field
 *  @param wIndex - wIndex field for the USB request structure
 *  @param buffer - user buffer
 *  @param length - length of user buffer
 *
 *  @return
 *	number of bytes transferred
 */
int usb_std_request(struct usbdev *usbdev, u8_t bmRequestType,
		    u8_t bRequest, u16_t wValue,
		    u16_t wIndex, u8_t *buffer, int length);

/*
 * HUB APIs
 */

/**
 *  @brief usb_scan(bus)
 *
 *  @details Scan the bus looking for new or removed devices
 *
 *  @param
 *	bus - bus to scan
 *
 *  @return nothing
 *
 */
void usb_scan(struct usbbus *bus);

/**
 *  @brief usbhub_map_tree(bus, func, arg)
 *
 *  @details Call a function for each device in the tree
 *
 *  @param bus - bus to scan
 *  @param func - function to call
 *  @param arg - argument to pass to function
 *
 *  @return nothing
 *
 */
void usbhub_map_tree(struct usbbus *bus,
		     int (*func)(struct usbdev *usbdev, void *arg),
		     void *arg);


/**
 *  @brief usbhub_map_from_device(usbdev, func, arg)
 *
 *  @details Call a function for each device in the tree
 *
 *  @param usbdev - usb device to scan
 *  @param func - function to call
 *  @param arg - argument to pass to function
 *
 *  @return nothing
 *
 */
void usbhub_map_from_device(struct usbdev *usbdev,
			    int (*func)(struct usbdev *usbdev, void *arg),
			    void *arg);

/**
 *  @brief usbhub_dumpbus(bus, verbose)
 *
 *  @details map function to dump devices in the device tree
 *
 *  @param bus - poiner to usb bus
 *  @param verbose - 1 for verbose prints
 *
 *  @return 0
 *
 */
void usbhub_dumpbus(struct usbbus *bus, u32_t verbose);


/*
 * USB device driver APIs
 */

/**
 *  @brief usb_find_driver(dev)
 *
 *  @details Find a suitable device driver to handle the specified
 *	     class, vendor, or product.
 *
 *  @param dev - pointer to usb device
 *
 *  @return
 *	pointer to device driver or NULL
 */
struct usb_driver *usb_find_driver(struct usbdev *dev);

/*
 * Debug APIs
 */

/**
 *  @brief usb_dbg_dumpportstatus_
 *
 *  @details dump port status
 *
 *  @param port - usb port number
 *  @param portstatus - pointer to port status structure
 *  @param level - debug level
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_dumpportstatus(int port, struct usb_port_status *portstatus,
			    int level);
/**
 *  @brief usb_dbg_dumpdescriptors
 *
 *  @details dump all descriptors
 *
 *  @param dev - pointer to usbdev structure
 *  @param ptr - pointer to buffer
 *  @param len - length of the descriptor to dump
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_dumpdescriptors(struct usbdev *dev, u8_t *ptr, int len);

/**
 *  @brief usb_dbg_
 *
 *  @details
 *
 *  @param dev - pointer to usb device structure
 *  @param index - usb device number
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_dumpcfgdescr(struct usbdev *dev, unsigned int index);

/**
 *  @brief usb_dbg_dumpeptdescr
 *
 *  @details dump endpoint descriptor fields
 *
 *  @param epdscr - pointer to endpoint descriptor structure
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_dumpeptdescr(struct usb_endpoint_descr *epdscr);

/**
 *  @brief usb_dbg_dumpdvice
 *
 *  @details dump device structure fields
 *
 *  @param dev pointer to usbdev structure
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_dumpdevice(struct usbdev *dev);

/**
 *  @brief usb_dbg_showdevice
 *
 *  @details show details of struct usbdev
 *
 *  @param dev - pointer to usbdev structure
 *
 *  @return
 *	nothing
 *
 */
void usb_dbg_showdevice(struct usbdev *dev);

#endif /* __USB_HOST_H__ */
