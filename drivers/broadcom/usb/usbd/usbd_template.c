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

/* @file usbd_template.c
 *
 * Test file for initializing the USB Device.
 *
 *
 */




#include "usbdevice.h"
#include "usbregisters.h"
#include "usb.h"

#include "usbdevice_internal.h"
#include "usbd.h"
#include "usbd_descriptors.h"
#include "stub.h"
#include "cvclass.h"

#ifdef USH_BOOTROM /*AAI */
#include <arch/cpu.h>
#include <board.h>
#include <broadcom/clock_control.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>

struct usbd_bcm58202_config {
	u32_t base;
};

static struct usbd_bcm58202_config usbd_bcm58202_cfg = {
	.base = USBD_BASE_ADDR
};

int usbd_bcm58202_init(struct device *dev)
{
	ARG_UNUSED(dev);

	usbd_interface_t intf;

	SYS_LOG_DBG("===============");
	SYS_LOG_DBG(" Initializing USBD...\n");

	/* Disable the IRQ for USBD Block before anything*/
	SYS_LOG_DBG("usbd_disable_irq");
	usbd_disable_irq();

	/* phx_blocked_delay_us(1000); */

	/* Initialize the USB device subsystem */
	/*Phy Init and memory region clear*/
	SYS_LOG_DBG("usbDeviceInit");
	usbDeviceInit();

	/*TODO better method for configuring PID and cv_string */
	USBDevice.pid = PID_FP_NONE;
	USBDevice.cv_string = CV_FP_NONE;

	/*Register vendor request handler if any */
	//usbd_registervenhndlr(vendor_hndlr);
	SYS_LOG_DBG("usbd_registervenhndlr");
	usbd_registervenhndlr(&USBDevice,NULL);

	/*Setup the interfaces and ep start*/
	/* setup the control*/
	SYS_LOG_DBG("usbd_config\n");
	usbd_config(&USBDevice);

	/*Interface 1 common for both modes of operation*/
	SYS_LOG_DBG("usbd_reginterface");
	memset(&intf, 0, sizeof(usbd_interface_t));
	intf.type = USBD_CV_INTERFACE;
	intf.class_request_handler = NULL;
	intf.reset = cv_usbd_reset;
	intf.descriptor = usb_cv_interface_descriptor;
	intf.descriptor_len = sizeof(usb_cv_interface_descriptor);
	usbd_reginterface(&USBDevice,intf);

	/*Setup the interfaces and ep END*/
	SYS_LOG_DBG("usbd_hardware_init");
	usbd_hardware_init(&USBDevice);

	/*set the state as IDLE starting state*/
	USBDevice.interface_state = USBD_STATE_RESET;

	SYS_LOG_DBG("Initializing USBD Done...\n");

	/* Do a cv reset */
	SYS_LOG_DBG("cv_usbd_reset");
	cv_usbd_reset();


#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)pusb_mem_block,
										sizeof(tUSB_MEM_BLOCK));
#endif

	/* Start USB Device running Make device visible on Bus*/
	SYS_LOG_DBG("usbDeviceStart");
	usbDeviceStart();

	/* Register the ISR */
	SYS_LOG_DBG("usbd_register_isr");
	usbd_register_isr();


	/* clear all pending interrupts */
	usbd_clear_allpend();

	/* Enable the IRQ */
	SYS_LOG_DBG("usbd_enable_irq");
	usbd_enable_irq();

	return 0;
}


DEVICE_AND_API_INIT(usbd, CONFIG_USBD_BCM58202_DEV_NAME,
		    usbd_bcm58202_init, NULL, &usbd_bcm58202_cfg,
		    POST_KERNEL, CONFIG_USBD_DRIVER_INIT_PRIORITY, NULL);

#else /* SBI MODE */
int test_app(void)
{

	usbd_interface_t intf;

	printk("test_app: usbDeviceInit");
	phx_blocked_delay_us(1000);

	/* Initialize the USB device subsystem */
	/* Phy Init and memory region clear*/
	usbDeviceInit();

	/* TODO better method for configuring PID and cv_string */
	USBDevice.pid = PID_FP_NONE;
	USBDevice.cv_string = CV_FP_NONE;

	printk("test_app: usbd_registervenhndlr");
	/* Register vendor request handler if any */
	/* usbd_registervenhndlr(vendor_hndlr); */
	usbd_registervenhndlr(&USBDevice, NULL);

	printk("test_app: usbd_config\n");
	/* Setup the interfaces and ep start*/
	/* setup the control*/
	usbd_config(&USBDevice);

	printk("test_app: usbd_reginterface");

	/* Interface 1 common for both modes of operation*/
	memset(&intf, 0, sizeof(usbd_interface_t));
	intf.type = USBD_CV_INTERFACE;
	intf.class_request_handler = NULL;
	intf.reset = cv_usbd_reset;
	intf.descriptor = usb_cv_interface_descriptor;
	intf.descriptor_len = sizeof(usb_cv_interface_descriptor);
	usbd_reginterface(&USBDevice, intf);

	/* Setup the interfaces and ep END*/
	printk("test_app: usbd_hardware_init");
	usbd_hardware_init(&USBDevice);

	/*set the state as IDLE starting state*/
	USBDevice.interface_state = USBD_STATE_RESET;

	/* POLL MODE */
	/* Start USB Device mode running */

	/* Do a cv reset */
	printk("test_app: cv_usbd_reset");

	cv_usbd_reset();

	printk("test_app: usbDeviceStart");
	usbDeviceStart();

	printk("test_app: Device init complete start polling \n === \n\n");

	do{
#if 0
		usbdevice_isr(NULL);
		usbd_idle();
#endif
		usbd_isr(NULL);
	} while (1);


}

#endif /* SBI MODE */
