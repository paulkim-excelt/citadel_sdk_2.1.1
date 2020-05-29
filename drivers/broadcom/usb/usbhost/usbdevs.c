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

/*
 * The list of drivers we support. If you add more drivers,
 * list them here.
 */
extern usb_driver_t usbhub_driver;
#ifdef CONFIG_USBMS
extern usb_driver_t usbmass_driver;
#endif
#ifdef CONFIG_USBHID
extern usb_driver_t usbhid_driver;
#endif
#ifdef CONFIG_USBCDC
extern usb_driver_t usbcdc_driver;
#endif
#ifdef CONFIG_UVC
extern usb_driver_t usbuvc_driver;
#endif

const usb_drvlist_t usb_drivers[] = {
	/* Hub driver */
	{USB_DEVICE_CLASS_HUB, VENDOR_ANY, PRODUCT_ANY,
	 &usbhub_driver},
#ifdef CONFIG_USBHID
	/* Keyboards and mice */
	{USB_DEVICE_CLASS_HUMAN_INTERFACE, VENDOR_ANY, PRODUCT_ANY,
	 &usbhid_driver},
#endif
#ifdef CONFIG_USBMS
	/* Mass storage devices */
	{USB_DEVICE_CLASS_STORAGE, VENDOR_ANY, PRODUCT_ANY,
	 &usbmass_driver},
#endif
#ifdef CONFIG_USBCDC
	/* Class communications */
	{USB_DEVICE_CLASS_COMMUNICATIONS, VENDOR_ANY, PRODUCT_ANY,
	 &usbcdc_driver},
#endif
#ifdef CONFIG_UVC
	/* Class Video */
	{USB_DEVICE_CLASS_VIDEO, VENDOR_ANY, PRODUCT_ANY,
	 &usbuvc_driver},
#endif
	{0, 0, 0, NULL}
};

/*
 * usb_find_driver(class,vendor,product)
 *
 * Find a suitable device driver to handle the specified
 * class, vendor, or product.
 *
 * Input parameters:
 *	devdescr - device descriptor
 *
 * Return value:
 *	pointer to device driver or NULL
 */

usb_driver_t *usb_find_driver(usbdev_t *dev)
{
	usb_device_descr_t *devdescr;
	usb_interface_descr_t *ifdescr;
	const usb_drvlist_t *list;
	int dclass, vendor, product;

	devdescr = &(dev->ud_devdescr);

	dclass = devdescr->bDeviceClass;
	if (dclass == 0) {
		ifdescr = usb_find_cfg_descr(dev,
				 USB_INTERFACE_DESCRIPTOR_TYPE, 0);
		if (ifdescr)
			dclass = ifdescr->bInterfaceClass;
	}

	vendor  = (int) GETUSBFIELD(devdescr, idVendor);
	product = (int) GETUSBFIELD(devdescr, idProduct);

	printf("USBH bus %d device %d: vendor 0x%04X product 0x%04X class 0x%04X: ",
	       dev->ud_bus->ub_num, dev->ud_address, vendor, product, dclass);

	if ((dclass == USB_DEVICE_CLASS_HUB) && (vendor == 0x0) &&
	   (product == 0x0)) {
		printf("%s\n", (&usbhub_driver)->udrv_name);
		return &usbhub_driver;
	}

#ifdef CONFIG_USBHID
	if ((dclass == USB_DEVICE_CLASS_HUB) && (vendor == 0x413c) &&
	   (product == 0x1010)) {
		printf("%s\n", (&usbhid_driver)->udrv_name);
		return &usbhid_driver;
	}
#endif
	list = usb_drivers;
	while (list->udl_disp) {
		if (((list->udl_class == dclass) ||
		    (list->udl_class == CLASS_ANY)) &&
		    ((list->udl_vendor == vendor) ||
		    (list->udl_vendor == VENDOR_ANY)) &&
		    ((list->udl_product == product) ||
		    (list->udl_product == PRODUCT_ANY))) {
			printf("%s\n", list->udl_disp->udrv_name);
			return list->udl_disp;
		}
		list++;
	}

	printf("Not found.\n");

	return NULL;
}
