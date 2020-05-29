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

/* @file usbhub.c
 *
 * USB HUB class driver.
 * Contains routines to enumerate usb devices starting with the roothub.
 *
 */

#include <kernel.h>
#include <string.h>
#include <logging/sys_log.h>
#include <broadcom/dma.h>
#include <usbh/usb_host.h>
#include "usb_host_bcm5820x.h"
#include "host/ohci.h"

/* Enable for hub debug messages */
/*#define HUB_DEBUG */

/* Macros for common hub requests */
#define usbhub_set_port_feature(dev, port, feature) \
	usb_simple_request(dev, 0x23, USB_HUBREQ_SET_FEATURE, feature, port)

#define usbhub_set_hub_feature(dev, feature) \
	usb_simple_request(dev, 0x20, USB_HUBREQ_SET_FEATURE, feature, 0)

#define usbhub_clear_port_feature(dev, port, feature) \
	usb_simple_request(dev, 0x23, USB_HUBREQ_CLEAR_FEATURE, feature, port)

#define usbhub_clear_hub_feature(dev, feature) \
	usb_simple_request(dev, 0x20, USB_HUBREQ_CLEAR_FEATURE, feature, 0)

#define usb_noisy		0
#define dump_desc		0

/* Forward declarations */
static int usbhub_attach(struct usbdev *dev, struct usb_driver *drv);
static int usbhub_detach(struct usbdev *dev);
static void usbhub_markdetached(struct usbdev *dev);

/* Hub-specific data structures */
#define UHUB_FLG_NEEDSCAN	1

struct usbhub_softc {
	struct usb_hub_descr uhub_descr;
	struct usb_hub_status uhub_status;
	int uhub_ipipe;
	int uhub_ipipemps;
	int uhub_nports;
	unsigned int uhub_flags;
	u8_t __aligned(64) uhub_imsg[8];
	struct usbdev *uhub_devices[UHUB_MAX_DEVICES];
};

const struct usb_driver usbhub_driver = {
	"USB Hub",
	usbhub_attach,
	usbhub_detach
};

const struct usb_driver usbroothub_driver = {
	"Root Hub",
	usbhub_attach,
	usbhub_detach
};

/*
 *  usbhub_ireq_callback(ur)
 *
 *  this routine is called when the transfer we queued to the
 *  interrupt endpoint on the hub completes.  It means that
 *  *some* port on the hub needs attention.  The data indicates
 *  which port, but for our purposes we don't really care - if
 *  we get this callback, we'll set a flag and re-probe the bus.
 *
 *  Input parameters:
 *	ur - usbreq that completed
 *
 *  Return value: 0
 *
 */

static int usbhub_ireq_callback(struct usbreq *ur)
{
	int idx;
	struct usbhub_softc *uhub = (ur->ur_dev->ud_private);

	/*
	 * Check to see if the request was cancelled by someone
	 * deleting our endpoint.  We also check for "device not responding"
	 * which typically happens when a device is removed.
	 *
	 * TODO this is not correct, it appears that hubs sometimes do
	 * return this error.  We'll need to redo the whole way
	 * surprise detach works eventually...
	 */

	if ((ur->ur_status == UR_ERR_CANCELLED) ||
	    (ur->ur_status == UR_ERR_DEVICENOTRESPONDING)) {
		usb_free_request(ur);
		return 0;
	}

	/*
	 * Check to see if any of our ports need attention
	 */

	for (idx = 1; idx <= uhub->uhub_nports; idx++) {
		if (ur->ur_buffer[0] & (1<<idx)) {
			/*
			 * Mark the hub as needing a scan,
			 * and mark the bus as well..
			 * so the top-level polling will notice.
			 */
			uhub->uhub_flags |= UHUB_FLG_NEEDSCAN;
			ur->ur_dev->ud_bus->ub_flags |= UB_FLG_NEEDSCAN;
		}
	}

	/*
	 * Do NOT requeue the request here.  We will do this
	 * during exploration.
	 */
	usb_free_request(ur);

	return 0;
}

/*
 *  usbhub_get_hub_descriptor(dev,dscr,idx,maxlen)
 *
 *  Obtain the hub descriptor (special for hubs) from the
 *  device.
 *
 *  Input parameters:
 *	dev - usb device
 *	dscr - place to put hub descriptor
 *	idx - which hub descriptor to get (usually zero)
 *	maxlen - max # of bytes to return
 *
 *  Return value:
 *	result status
 */
static int usbhub_get_hub_descriptor(struct usbdev *dev,
				     struct usb_hub_descr *dscr,
				     int idx, int maxlen)
{
	ARG_UNUSED(idx);
	int res;
	u8_t *respbuf;
	int len;

	/*
	 * Get the hub descriptor.  Get the first 8 bytes first, then get
	 * the rest if there is more.
	 */
	respbuf = cache_line_aligned_alloc(sizeof(USB_HUB_DESCR_SIZE));
	res = usb_std_request(dev, 0xA0, USB_HUBREQ_GET_DESCRIPTOR,
			      0, 0, respbuf, USB_HUB_DESCR_SIZE);

	len = ((struct usb_hub_descr *)respbuf)->bDescriptorLength;
	if (len > USB_HUB_DESCR_SIZE) {
		respbuf = cache_line_aligned_alloc(sizeof(len));
		res = usb_std_request(dev, 0xA0, USB_HUBREQ_GET_DESCRIPTOR,
				      0, 0, respbuf, len);
	}
	memcpy(dscr, respbuf, (len <= maxlen ? len : maxlen));

	return res;
}

/*
 *  usbhub_get_port_status(dev,port,status)
 *
 *  Obtain the port status for a particular port from
 *  device.
 *
 *  Input parameters:
 *	dev - usb device
 *	port - 1-based port number
 *	status - where to put port status structure
 *
 *  Return value:
 *	# of bytes returned
 *
 */

static int usbhub_get_port_status(struct usbdev *dev, int port,
				  struct usb_port_status *status)
{
	return usb_std_request(dev, 0xA3, 0, 0, port, (u8_t *)status,
			       sizeof(struct usb_port_status));
}


/*
 *  usbhub_queue_intreq(dev,softc)
 *
 *  Queue the transfer to the interrupt pipe that will catch
 *  the hub's port status changes
 *
 *  Input parameters:
 *	dev - usb device
 *	softc - hub-specific data
 *
 *  Return value:
 *	nothing
 */
static void usbhub_queue_intreq(struct usbdev *dev, struct usbhub_softc *softc)
{
	struct usbreq *ur;

	ur = usb_make_request(dev, softc->uhub_ipipe,
			      softc->uhub_imsg, softc->uhub_ipipemps,
			      UR_FLAG_IN | UR_FLAG_SHORTOK);

	ur->ur_callback = usbhub_ireq_callback;

	usb_queue_request(ur);
}

/*
 *  usbhub_attach(dev,drv)
 *
 *  This routine is called when the hub attaches to the system.
 *  We complete initialization for the hub and set things up so
 *  that an explore will happen soon.
 *
 *  Input parameters:
 *	dev - usb device
 *	drv - driver structure
 *
 *  Return value: 0
 *
 */

unsigned int usbhub_ipipe; /* usbhub interrupt pipe */
static int usbhub_attach(struct usbdev *usbdev, struct usb_driver *drv)
{
	struct usb_device_status devstatus;
	struct usb_config_descr *cfgdscr;
	struct usb_endpoint_descr *epdscr;
	struct usbhub_softc *softc;

	/*
	 * Remember the driver dispatch.
	 */
	printk("USB: usbhub_attach\n");

	usbdev->ud_drv = drv;

	softc = (struct usbhub_softc *)cache_line_aligned_alloc(
				sizeof(struct usbhub_softc));
	memset((void *)softc, 0, sizeof(struct usbhub_softc));
	usbdev->ud_private = softc;

	/*
	 * Dig out the data from the configuration descriptor
	 * (we got this from the device before attach time)
	 */

	cfgdscr = usbdev->ud_cfgdescr;
	epdscr = usb_find_cfg_descr(usbdev, USB_ENDPOINT_DESCRIPTOR_TYPE, 0);

	/*
	 * Get device status (is this really necessary?)
	 */

	usb_get_device_status(usbdev, &devstatus);

	/*
	 * Set us to configuration index 0
	 */

	usb_set_dev_configuration(usbdev, cfgdscr->bConfigurationValue);

	/*
	 * Get the hub descriptor.
	 */
	usbhub_get_hub_descriptor(usbdev, &(softc->uhub_descr), 0,
				  sizeof(struct usb_hub_descr));

	/*
	 * remember stuff from the hub descriptor
	 */

	softc->uhub_nports = softc->uhub_descr.bNumberOfPorts;

	/*
	 * Open the interrupt pipe
	 */

	softc->uhub_ipipemps = GETUSBFIELD(epdscr, wMaxPacketSize);
	softc->uhub_ipipe = usb_open_pipe(usbdev, epdscr);

	/* CS-3381, record usbhub interrupt pipe in case of destroy by
	 * device removal.
	 */
	usbhub_ipipe = softc->uhub_ipipe;

	/*
	 * Mark the bus and the hub as needing service.
	 */

	softc->uhub_flags |= UHUB_FLG_NEEDSCAN;
	usbdev->ud_bus->ub_flags |= UB_FLG_NEEDSCAN;

	/*
	 * Okay, that's it.  The top-level USB daemon will notice
	 * that the bus needs service and will invoke the exploration code.
	 * This may in turn require additional explores until
	 * everything settles down.
	 */

	return 0;
}


/*
 *  usbhub_detach(dev)
 *
 *  Called when a hub is removed from the system - we remove
 *  all subordinate devices.
 *
 *  Input parameters:
 *	dev - device (hub) that was removed
 *
 *  Return value: 0
 *
 */

static int usbhub_detach(struct usbdev *dev)
{
	struct usbhub_softc *hub;
	struct usbdev *deldev;
	int idx;

	if (!IS_HUB(dev))
		return 0; /* should not happen */

	hub = dev->ud_private;

	for (idx = 0; idx < UHUB_MAX_DEVICES; idx++) {
		deldev = hub->uhub_devices[idx];
		if (deldev) {
			if (usb_noisy > 0)
				printk("USB: Removing device attached to bus %d hub %d port %d\n",
				       dev->ud_bus->ub_num,
				       dev->ud_address, idx+1);
			if (deldev->ud_drv) {
				/* close open pipes, cancel reqs */
				usb_destroy_all_pipes(deldev);
				/*
				 * Try to process the done queue.
				 * This will complete any requests
				 * that made it out of the pipes while
				 * we were doing the stuff above.
				 */
				usb_poll(deldev->ud_bus);
				/* Call detach method,
				 * clean up device softc
				 */
				if (deldev->ud_drv->udrv_detach)
					(*(deldev->ud_drv->udrv_detach))(deldev);
			} else {
				if (usb_noisy > 0) {
					printk("USB: Detached device on bus %d hub %d port %d has no methods\n",
					       dev->ud_bus->ub_num,
					       dev->ud_address, idx+1);
				}
			}
			if (deldev->ud_cfgdescr)
				deldev->ud_cfgdescr = NULL;
			usb_destroy_device(deldev);
		}
	}

	hub = NULL; /* remove softc */

	return 0;
}

/*
 *  usbhub_map_tree1(dev,level,func,arg)
 *
 *  This routine is used in recursive device tree exploration.
 *  We call 'func' for each device at this tree, and descend
 *  when we run into hubs
 *
 *  Input parameters:
 *	dev - current device pointer
 *	level - current nesting level
 *	func - function to call
 *	arg - argument to pass to function
 *
 *  Return value: nothing
 *
 */

static void usbhub_map_tree1(struct usbdev *dev, int level,
			     int (*func)(struct usbdev *dev, void *arg),
			     void *arg)
{
	struct usbhub_softc *hub;
	int idx;

	(*func)(dev, arg);

	if (IS_HUB(dev)) {
		hub = dev->ud_private;
		for (idx = 0; idx < UHUB_MAX_DEVICES; idx++) {
			if (hub->uhub_devices[idx])
				usbhub_map_tree1(hub->uhub_devices[idx],
					level+1, func, arg);
		}
	}
}

/*
 *  usbhub_map_tree(bus,func,arg)
 *
 *  Call a function for each device in the tree
 *
 *  Input parameters:
 *	bus - bus to scan
 *	func - function to call
 *	arg - argument to pass to function
 *
 *  Return value: nothing
 *
 */

void usbhub_map_tree(struct usbbus *bus, int (*func)(struct usbdev *dev,
		     void *arg), void *arg)
{
	usbhub_map_tree1(bus->ub_roothub, 0, func, arg);
}

void usbhub_map_from_device(struct usbdev *dev, int (*func)(struct usbdev *dev,
			    void *arg), void *arg)
{
	usbhub_map_tree1(dev, 0, func, arg);
}

#ifdef USB_DEBUG
/*
 *  usbhub_dumpbus1(dev,arg)
 *
 *  map function to dump devices in the device tree
 *
 *  Input parameters:
 *	dev - device we're working on
 *	arg - argument from map_tree call
 *
 *  Return value: 0
 *
 */

static int usbhub_dumpbus1(struct usbdev *dev, void *arg)
{
	u32_t *verbose = (u32_t *) arg;

	if ((*verbose & 0x00FF) && (dev->ud_address != (*verbose & 0x00FF)))
		return 0;

	if (*verbose & 0x100)

	usb_dbg_showdevice(dev);

	if (*verbose & 0x100) {
		usb_dbg_dumpdescriptors(dev, (u8_t *)&(dev->ud_devdescr),
					dev->ud_devdescr.bLength);
		usb_dbg_dumpcfgdescr(dev, 0);
	}

	return 0;
}


/*
 *  usbhub_dumpbus(bus,verbose)
 *
 *  Dump information about devices on the USB bus.
 *
 *  Input parameters:
 *	bus - bus to dump
 *	verbose - nonzero to display more info, like descriptors
 *
 *  Return value: nothing
 *
 */

void usbhub_dumpbus(struct usbbus *bus, u32_t verbose)
{
	usbhub_map_tree(bus, usbhub_dumpbus1, &verbose);
}
#endif

/*
 *  usbhub_reset_device(dev,port,status)
 *
 *  Reset a device on a hub port.  This routine does a
 *  USB_PORT_FEATURE_RESET on the specified port, waits for the
 *  bit to clear, and returns.  It is used to get a device into the
 *  DEFAULT state according to the spec.
 *
 * Input parameters:
 *	dev - hub device
 *	port - port number(1-based)
 *	status - place to return port_status structure after
 *		reset completes
 *
 *  Return value: nothing
 *
 */

void usbhub_reset_device(struct usbdev *dev, int port,
			 struct usb_port_status *portstatus)
{
	if (usb_noisy > 0)
		printk("USB: Resetting device on bus %d port %d\n",
		       dev->ud_bus->ub_num, port);

	usbhub_set_port_feature(dev, port, USB_PORT_FEATURE_RESET);

	usbhub_get_port_status(dev, port, portstatus);

	for (; ;) {
		usbhub_get_port_status(dev, port, portstatus);
		if ((GETUSBFIELD((portstatus), wPortStatus) &
		     USB_PORT_STATUS_RESET) == 0)
			break;
		usb_delay_ms(250);
	}

	usb_delay_ms(250);

	usbhub_clear_port_feature(dev, port, USB_PORT_FEATURE_C_PORT_RESET);
}

/*
 *  usbhub_scan_ports(dev,arg)
 *
 *  Scan the ports on this hub for new or removed devices.
 *
 *  Input parameters:
 *	dev - hub device
 *	arg - passed from bus scan main routines
 *
 *  Return value: nothing
 *
 */

/* camera returns a 1069 byte descriptor */
/* static u8_t buf_cfgdescr[1088] = {0}; */

static void usbhub_scan_ports(struct usbdev *dev, void *arg)
{
	ARG_UNUSED(arg);
	u16_t current;
	u16_t changed;
	struct usbhub_softc *softc;
	int idx;
	int res;
	int len;
	u8_t *buf;
	struct usbdev *newdev;
	struct usb_driver *newdrv;
	int addr;
	struct usb_port_status *portstatus;
	struct usb_config_descr cfgdescr;
	unsigned int powerondelay;

	if (!IS_HUB(dev))
		return; /* should not happen.  */

	portstatus = (struct usb_port_status *)cache_line_aligned_alloc(
				sizeof(struct usb_port_status));
	if (portstatus == NULL)
		return;

	/*
	 * We know this is a hub.  Get the softc back.
	 */

	softc = (struct usbhub_softc *) dev->ud_private;
	powerondelay = ((u32_t)softc->uhub_descr.bPowerOnToPowerGood) * 2 + 20;

	/*
	 * Turn on the power to the ports whose power is not yet on.
	 */
#ifdef HUB_DEBUG
	printk("\nUSBH usbhub_scan_port: start\n");
#endif

	for (idx = 0; idx < softc->uhub_nports; idx++) {
		printk("hub_nports: %d\n", softc->uhub_nports);
		usbhub_get_port_status(dev, idx+1, portstatus);

		current = GETUSBFIELD(portstatus, wPortStatus);
		changed = GETUSBFIELD(portstatus, wPortChange);

		if (usb_noisy > 1)
			printk("BeforePowerup: port %d status %04X changed %04X\n",
			       idx+1, current, changed);

	if (!(current & USB_PORT_STATUS_POWER)) {
		if (usb_noisy > 1)
			printk("USB: Powering up bus %d port %d\n",
			       dev->ud_bus->ub_num, idx+1);
			usbhub_set_port_feature(dev, idx+1, USB_PORT_FEATURE_POWER);
			usb_delay_ms(powerondelay);
		}
	}

	/*
	 * Begin exploration at this level.
	 */
	for (idx = 0; idx < softc->uhub_nports; idx++) {
		usbhub_get_port_status(dev, idx+1, portstatus);

		current = GETUSBFIELD(portstatus, wPortStatus);
		changed = GETUSBFIELD(portstatus, wPortChange);
		if (usb_noisy > 0) {
			printk("USB: Explore: Bus %d Hub %d port %d status %04X changed %04X\n",
			       dev->ud_bus->ub_num,
			       dev->ud_address, idx+1, current, changed);
			usb_dbg_dumpportstatus(idx+1, portstatus, 1);
		}
#ifdef HUB_DEBUG
		printk("USB: Explore: Bus %d Hub %d port %d status %04X changed %04X\n",
		       dev->ud_bus->ub_num, dev->ud_address, idx + 1,
		       current, changed);
#endif

		if (changed & USB_PORT_STATUS_RESET) {
		}

		if (changed & USB_PORT_STATUS_OVERCUR) {
#ifdef HUB_DEBUG
			printk("hub %d port %d overcurrent\n", dev->ud_bus->ub_num, idx+1);
#endif
			usbhub_reset_device(dev, idx+1, portstatus);
			current = GETUSBFIELD(portstatus, wPortStatus);
			changed = GETUSBFIELD(portstatus, wPortChange);
#ifdef HUB_DEBUG
			printk("USB: Explore-overcurrent: Bus %d Hub %d port %d status %04X changed %04X\n",
			       dev->ud_bus->ub_num, dev->ud_address,
			       idx+1, current, changed);
#endif
		}

		if (changed & USB_PORT_STATUS_ENABLED)
			usbhub_clear_port_feature(dev, idx+1,
					  USB_PORT_FEATURE_C_PORT_ENABLE);

		if (changed & USB_PORT_STATUS_CONNECT) {
			int do_connect = (current & USB_PORT_STATUS_CONNECT);
			int do_disconnect = !(current & USB_PORT_STATUS_CONNECT);

			if (do_connect && softc->uhub_devices[idx]) {
#ifdef HUB_DEBUG
				printk("re-connect\n");
#endif
				/* device was disconnected,
				 * then reconnected before we
				 * process the disconnect.
				 */
				do_disconnect = 1;
			}
#ifdef HUB_DEBUG
			if (do_disconnect && !softc->uhub_devices[idx])
				printk("re-disconnect\n");

			if (do_connect)
				printk("do_connect\n");

			if (do_disconnect)
				printk("do_disconnect\n");
#endif
			/*
			 * A device was either connected or disconnected.
			 * Clear the status change first.
			 */
			usbhub_clear_port_feature(dev, idx+1,
				  USB_PORT_FEATURE_C_PORT_CONNECTION);

			/* If disconnect,
			 * or (connect & something already there),
			 * then disconnect
			 */
			if (do_disconnect) {
				/*
				 * The device has been DISCONNECTED.
				 */
				printk("USBH: Device disconnected from bus %d hub %d port %d\n",
				       dev->ud_bus->ub_num,
				       dev->ud_address, idx+1);

				/*
				 * Recover pointer to device below hub
				 * and clear this pointer.
				 */
				/* Get device pointer */
				newdev = softc->uhub_devices[idx];

				/* mark device and all subordinate
				 * devices as "removing"
				 */
				usbhub_markdetached(newdev);

				/* remove device from hub */
				softc->uhub_devices[idx] = NULL;

				/*
				 * Deassign the USB device's address and then
				 * call detach method to free resources.  Devices that
				 * do not have drivers will not have any methods.
				 */

				if (newdev) {
					if (newdev->ud_drv) {
						/* close open pipes, cancel reqs */
						usb_destroy_all_pipes(newdev);
						/*
						 * Try to process the done queue.  This will complete any
						 * requests that made it out of the pipes while we were
						 * doing the stuff above.
						 */
						usb_poll(newdev->ud_bus);
						/* Call detach method, clean up device softc */
						if (newdev->ud_drv->udrv_detach)
							(*(newdev->ud_drv->udrv_detach))(newdev);
					} else {
						if (usb_noisy > 0) {
							printk("USB: Detached device on bus %d hub %d port %d "
							"has no methods\n",
							       dev->ud_bus->ub_num,
							       dev->ud_address, idx+1);
						}
					}

					if (newdev->ud_cfgdescr)
						newdev->ud_cfgdescr = NULL;

					usb_destroy_device(newdev);
				}
			}

			if (do_connect) {
				/*
				 * The device has been CONNECTED.
				 */

				printk("USB: New device connected to bus %d hub %d port %d\n",
				       dev->ud_bus->ub_num,
				       dev->ud_address, idx+1);

				/*
				 * Reset the device.  Reuse our old port status structure
				 * so we get the latest status.  Some devices do not report
				 * lowspeed until they are reset.
				 */
				usbhub_reset_device(dev, idx+1, portstatus);
#ifdef HUB_DEBUG
				printk("after reset.\n");
#endif

				current = GETUSBFIELD(portstatus, wPortStatus);
				changed = GETUSBFIELD(portstatus, wPortChange);
				usb_delay_ms(100);

				if ((current & USB_PORT_STATUS_RESET) != 0) {
					SYS_LOG_ERR("Error resetting Port\n");
					goto do_connect_end;
				}

				/*
				 * Create a device for this port.
				 */
#ifdef HUB_DEBUG
				printk("create usb device.\n");
#endif

				newdev = usb_create_device(dev->ud_bus, idx+1,
						(current & USB_PORT_STATUS_LOWSPD) ? 1 : 0);

				/*
				 * Get the device descriptor.
				 */
#ifdef HUB_DEBUG
				printk("get max packet size.\n");
#endif
				res = usb_get_device_descriptor(newdev,
					&(newdev->ud_devdescr), true);

				if (usb_noisy > 1)
					usb_dbg_dumpdescriptors(newdev,
						(u8_t *) &(newdev->ud_devdescr), 8);

				/*
				 * Set up the max packet size for the control endpoint,
				 * then get the rest of the descriptor.
				 */
#ifdef HUB_DEBUG
				printk("get device descriptor\n");
#endif
				usb_set_ep0mps(newdev, newdev->ud_devdescr.bMaxPacketSize0);
				printk("set ep0mps.. Done\n");

				res = usb_get_device_descriptor(newdev,
					&(newdev->ud_devdescr), false);

				/*
				 * Obtain a new address and set the address of the
				 * root hub to this address.
				 */

				addr = usb_new_address(newdev->ud_bus);
				res = usb_set_address(newdev, addr);

				/*
				 * Get the configuration descriptor and all the
				 * associated interface and endpoint descriptors.
				 */
				res = usb_get_config_descriptor(newdev, &cfgdescr, 0,
							sizeof(struct usb_config_descr));

				if (res != sizeof(struct usb_config_descr))
					printk("[a] usb_get_config_descriptor returns %d\n", res);

				len = GETUSBFIELD(&cfgdescr, wTotalLength);
				buf = cache_line_aligned_alloc(len);

				res = usb_get_config_descriptor(newdev, (struct usb_config_descr *)buf,
								0, len);
				if (res != len)
					printk("[b] usb_get_config_descriptor returns %d\n", res);

				newdev->ud_cfgdescr = (struct usb_config_descr *)buf;

				if (dump_desc)
					usb_dbg_dumpdescriptors(newdev, buf, len);

				/*
				 * Point the hub at the devices it owns
				 */

				softc->uhub_devices[idx] = newdev;

				/*
				 * Find the driver for this. It had better
				 * be the hub driver.
				 */

				newdrv = usb_find_driver(newdev);

				/*
				 * Call the attach method.
				 */
				if (newdrv) {
					/* remember driver dispatch in device */
					newdev->ud_drv = newdrv;
					if (newdrv->udrv_attach)
						(*(newdrv->udrv_attach))(newdev, newdrv);
				}
			}
		}
	}

do_connect_end:
	/*
	 * Queue up a request for the interrupt pipe.
	 * This will catch futher changes at this port.
	 */

	usbhub_queue_intreq(dev, softc);

#ifdef HUB_DEBUG
	printk("\nUSBH usbhub_scan_port: finish\n");
#endif
}

/*
 *  usbhub_markdetached(dev)
 *
 *  Mark a device and all devices below it as "removing" so we
 *  will change the status of pending requests to cancelled.
 *
 *  Input parameters:
 *	dev - device in the tree to start at
 *
 *  Return value: nothing
 *
 */

static int usbhub_markdetached1(struct usbdev *dev, void *arg)
{
	ARG_UNUSED(arg);

	dev->ud_flags |= UD_FLAG_REMOVING;

	return 0;
}

static void usbhub_markdetached(struct usbdev *dev)
{
	if (dev)
		usbhub_map_from_device(dev, usbhub_markdetached1, NULL);
}

/*
 *  usbhub_scan1(dev,arg)
 *
 *  Scan one device at this level, or descend if we run into a hub
 *  This is part of the device discovery code.
 *
 *  Input parameters:
 *	dev - current device, maybe a hub
 *	arg - passed from main scan routine
 *
 *  Return value: 0
 *
 */
static int usbhub_scan1(struct usbdev *dev, void *arg)
{
	struct usbhub_softc *hub;

	/*
	 * If the device is not a hub, we've reached the leaves of the
	 * tree.
	 */

	if (!IS_HUB(dev))
		return 0;

	/*
	 * Otherwise, scan the ports on this hub.
	 */

	hub = dev->ud_private;

	if (hub->uhub_flags & UHUB_FLG_NEEDSCAN) {
		hub->uhub_flags &= ~UHUB_FLG_NEEDSCAN;
		usbhub_scan_ports(dev, arg);
	}

	return 0;
}

/*
 *  usb_scan(bus)
 *
 *  Scan the bus looking for new or removed devices
 *
 *  Input parameters:
 *	bus - bus to scan
 *
 *  Return value: nothing
 *
 */

void usb_scan(struct usbbus *bus)
{
	/*
	 * Call our tree walker with the scan function.
	 */
	usbhub_map_tree(bus, usbhub_scan1, NULL);
}
