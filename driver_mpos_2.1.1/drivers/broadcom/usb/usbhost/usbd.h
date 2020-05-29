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

#ifndef __U_USBD_H
#define __U_USBD_H

#include "usbchap9.h"
#include "lib_queue.h"

#ifndef _physaddr_t_defined_
#define _physaddr_t_defined_
typedef u32_t physaddr_t;
#endif

typedef struct usb_hc_s usb_hc_t;
typedef struct usb_ept_s usb_ept_t;
typedef struct usb_hcdrv_s usb_hcdrv_t;
typedef struct usbdev_s usbdev_t;
typedef struct usb_driver_s usb_driver_t;

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

/*  USB Bus structure - one of these per host controller */
#define UHUB_MAX_DEVICES     2
#define USB_MAX_DEVICES      (UHUB_MAX_DEVICES + 1)
#define USB_ROOT_HUB_DEV_NUM 0
#define USB_PORT_1_DEV_NUM   1
#define USB_PORT_2_DEV_NUM   2
#define USB_PORT_3_DEV_NUM   3
#define USB_PORT_4_DEV_NUM   4
#define USB_PORT_5_DEV_NUM   5
#define USB_PORT_6_DEV_NUM   6
#define USB_PORT_7_DEV_NUM   7
#define USB_PORT_8_DEV_NUM   8

typedef struct usbbus_s {
	struct usbbus_s *ub_next;/*link to other buses */
	usb_hc_t *ub_hwsoftc;	/*bus driver softc */
	usb_hcdrv_t *ub_hwdisp;	/*bus driver dispatch */
	usbdev_t *ub_roothub;	/*root hub device */
	usbdev_t *ub_devices[USB_MAX_DEVICES];	/* idx by address */
	unsigned int ub_flags;	/*flag bits */
	int ub_num;		/*bus number */
} usbbus_t;

#define UB_FLG_NEEDSCAN	1	/*some device on bus needs scanning */

/*  USB Pipe structure - one of these per unidirectional channel
 *  to an endpoint on a USB device
 */

#define UP_TYPE_CONTROL	    0x0001
#define UP_TYPE_BULK        0x0002
#define UP_TYPE_INTR        0x0004
#define UP_TYPE_ISOC        0x0008

#define UP_TYPE_LOWSPEED    0x0010
#define UP_TYPE_FULLSPEED   0x0020
#define UP_TYPE_HIGHSPEED   0x0040

#define UP_TYPE_IN          0x0080
#define UP_TYPE_OUT         0x0100

typedef struct usbpipe_s {
	usb_ept_t *up_hwendpoint;/*OHCI-specific endpoint pointer */
	usbdev_t *up_dev;	/*our device info */
	int up_num;		/*pipe number */
	int up_mps;		/*max packet size */
	int up_flags;
} usbpipe_t;

/*  USB device structure - one per device attached to the USB
 *  This is the basic structure applications will use to
 *  refer to a device.
 */

#define UD_FLAG_HUB	0x0001	/*this is a hub device */
#define UD_FLAG_ROOTHUB	0x0002	/*this is a root hub device */
#define UD_FLAG_LOWSPEED 0x0008	/*this is a lowspeed device */
#define UD_FLAG_REMOVING 0x0010	/*device is being removed */

#define UD_MAX_PIPES_PER_DEV	32
#define USB_EPADDR_TO_IDX(epaddr) (((epaddr & 0x80) >> 3) | (epaddr & 0x0F))

struct usbdev_s {
	usb_driver_t *ud_drv;	/*Driver's methods */
	usbbus_t *ud_bus;	/*owning bus */
	int ud_address;		/*USB address */
	usbpipe_t *ud_pipes[UD_MAX_PIPES_PER_DEV];/*pipes, 0 is ctrl pipe */
	struct usbdev_s *ud_parent;	/*used for hubs */
	int ud_flags;
	void *ud_private;	/*private data for device driver */
	usb_device_descr_t ud_devdescr;	/*device descriptor */
	usb_config_descr_t *ud_cfgdescr;/*config, interface, and ep descs */
};

/*
 *  USB Request - basic structure to describe an in-progress
 *  I/O request.  It associates buses, pipes, and buffers
 *  together.
 */

#define UR_FLAG_SYNC		0x8000
#define UR_FLAG_SETUP		0x0001
#define UR_FLAG_IN		0x0002
#define UR_FLAG_OUT		0x0004
#define UR_FLAG_STATUS_IN	0x0008	/*status phase of a control WRITE */
#define UR_FLAG_STATUS_OUT	0x0010	/*status phase of a control READ */
#define UR_FLAG_SHORTOK		0x0020	/*short transfers are ok */

typedef struct usbreq_s {
	queue_t ur_qblock;

	/*
	 *pointers to our device and pipe
	 */

	usbdev_t *ur_dev;
	usbpipe_t *ur_pipe;

	/*
	 *stuff to keep track of the data we transfer
	 */

	u8_t *ur_buffer;
	int ur_length;
	int ur_xferred;
	int ur_status;
	int ur_flags;

	u8_t *ur_response;
	int ur_reslength;

	/*
	 *Stuff needed for the callback
	 */
	void *ur_ref;
	/*accessed from interrupt context */
	volatile int ur_inprogress;
	int (*ur_callback)(struct usbreq_s *req);

	/*
	 *For use inside the ohci driver
	 */
	void *ur_tdqueue;
	int ur_tdcount;
} usbreq_t, *usbreq_p;

/* Prototypes */

int usb_create_pipe(usbdev_t *dev, int pipenum, int mps, int flags);
void usb_destroy_pipe(usbdev_t *dev, int pipenum);
int usb_set_address(usbdev_t *dev, int addr);
usbdev_t *usb_create_device(usbbus_t *bus, int idx, int lowspeed);
void usb_destroy_device(usbdev_t *dev);
void usb_destroy_all_pipes(usbdev_t *dev);
usbreq_t *usb_make_request(usbdev_t *dev, int pipenum, u8_t *buf,
			   int length, int flags);
void usb_poll(usbbus_t *bus);
void usb_daemon(usbbus_t *bus);
int usb_cancel_request(usbreq_t *ur);
void usb_free_request(usbreq_t *ur);
int usb_queue_request(usbreq_t *ur);
int usb_wait_request(usbreq_t *ur);
int usb_sync_request(usbreq_t *ur);
int usb_make_sync_request(usbdev_t *dev, int pipenum, u8_t *buf,
			  int length, int flags);
int usb_make_sync_request_xferred(usbdev_t *dev, int pipenum, u8_t *buf,
				  int length, int flags, int *xferred);
int usb_get_descriptor(usbdev_t *dev, u8_t reqtype, int dsctype, int dscidx,
		       u8_t *buffer, int buflen);
int usb_get_config_descriptor(usbdev_t *dev, usb_config_descr_t *dscr,
			      int idx, int maxlen);
int usb_get_device_status(usbdev_t *dev, usb_device_status_t *status);
int usb_set_configuration(usbdev_t *dev, int config);
int usb_set_feature(usbdev_t *dev, int feature);
int usb_open_pipe(usbdev_t *dev, usb_endpoint_descr_t *epdesc);
int usb_simple_request(usbdev_t *dev, u8_t reqtype, int bRequest,
		       int wValue, int wIndex);
void usb_complete_request(usbreq_t *ur, int status);
int usb_get_device_descriptor(usbdev_t *dev, usb_device_descr_t *dscr,
			      int smallflg);
int usb_set_ep0mps(usbdev_t *dev, int mps);
int usb_new_address(usbbus_t *bus);
int usb_get_string(usbdev_t *dev, int id, char *buf, int maxlen);
int usb_std_request(usbdev_t *dev, u8_t bmRequestType,
		    u8_t bRequest, u16_t wValue,
		    u16_t wIndex, u8_t *buffer, int length);
void *usb_find_cfg_descr(usbdev_t *dev, int dtype, int idx);
void usb_delay_ms(int ms);
int usb_clear_stall(usbdev_t *dev, int pipe);

void usb_scan(usbbus_t *bus);
void usbhub_map_tree(usbbus_t *bus, int (*func)(usbdev_t *dev, void *arg),
		     void *arg);
void usbhub_map_from_device(usbdev_t *dev,
			    int (*func)(usbdev_t *dev, void *arg), void *arg);
void usbhub_dumpbus(usbbus_t *bus, u32_t verbose);

void usb_initroot(usbbus_t *bus);

/*
 *  Host Controller Driver
 *  Methods for abstracting the USB host controller from the
 *  rest of the goop.
 */

struct usb_hcdrv_s {
	usbbus_t *(*hcdrv_create)(physaddr_t regaddr);
	void (*hcdrv_delete)(usbbus_t *bus);
	int (*hcdrv_start)(usbbus_t *bus);
	void (*hcdrv_stop)(usbbus_t *bus);
	int (*hcdrv_intr)(usbbus_t *bus);
	usb_ept_t *(*hcdrv_ept_create)(usbbus_t *bus, int usbaddr, int eptnum,
					int mps, int flags);
	void (*hcdrv_ept_delete)(usbbus_t *bus, usb_ept_t *ep);
	void (*hcdrv_ept_setmps)(usbbus_t *bus, usb_ept_t *ep, int mps);
	void (*hcdrv_ept_setaddr)(usbbus_t *bus, usb_ept_t *ep, int addr);
	void (*hcdrv_ept_cleartoggle)(usbbus_t *bus, usb_ept_t *ep);
	int (*hcdrv_xfer)(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur);
	int (*hcdrv_cancel_xfer)(usbbus_t *, usb_ept_t *uept, usbreq_t *ur);
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

struct usb_driver_s {
	char *udrv_name;
	int (*udrv_attach)(usbdev_t *dev, usb_driver_t *drv);
	int (*udrv_detach)(usbdev_t *dev);
};

typedef struct usb_drvlist_s {
	int udl_class;
	int udl_vendor;
	int udl_product;
	usb_driver_t *udl_disp;
} usb_drvlist_t;

extern usb_driver_t *usb_find_driver(usbdev_t *dev);

#define CLASS_ANY	-1
#define VENDOR_ANY	-1
#define PRODUCT_ANY	-1

#define IS_HUB(dev) ((dev)->ud_devdescr.bDeviceClass == USB_DEVICE_CLASS_HUB)

/* Error codes */
#define USBD_ERR_OK		0	/*Request ok */
#define USBD_ERR_STALLED	-1	/*Endpoint is stalled */
#define USBD_ERR_IOERROR	-2	/*I/O error */
#define USBD_ERR_HWERROR	-3	/*Hardware failure */
#define USBD_ERR_CANCELED	-4	/*Request canceled */
#define USBD_ERR_NOMEM		-5	/*Out of memory */
#define USBD_ERR_TIMEOUT	-6	/*Request timeout */

/*  Debug routines */
void usb_dbg_dumpportstatus(int port, usb_port_status_t *portstatus,
			    int level);
void usb_dbg_dumpdescriptors(usbdev_t *dev, u8_t *ptr, int len);
void usb_dbg_dumpcfgdescr(usbdev_t *dev, unsigned int index);
void usb_dbg_dumpeptdescr(usb_endpoint_descr_t *epdscr);
void usb_dbg_dumpdevice(usbdev_t *dev);
void usb_dbg_showdevice(usbdev_t *dev);

#endif /* _USBD_H_ */
