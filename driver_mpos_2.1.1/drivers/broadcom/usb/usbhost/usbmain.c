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

/**********************************************************************
 *  Broadcom Common Firmware Environment (CFE)
 *
 *  Main Module				File: usbmain.c
 *
 *  Main module that invokes the top of the USB stack from CFE.
 *
 *  Author:  Mitch Lichtenberg
 *
 *********************************************************************/


#include "cfeglue.h"

/* SFE includes */
#include "usbhost_mem.h"

/* USB includes */
#include "usbchap9.h"
#include "usbd.h"
#include "ohci.h"
#include "ehci.h"
#include "mpu_wrappers.h"
#include <board.h>
#include <irq.h>

tUSBHOST_MEM_BLOCK usbhost_mem_block;

#define pUsbHost_mem_block ((volatile tUSBHOST_MEM_BLOCK *)&usbhost_mem_block)

/* map 'globals' into pUsbHost_mem_block members */
#define usb_buscnt		pUsbHost_mem_block->usb_buscnt
#define	usb_buses		pUsbHost_mem_block->usb_buses
#define	in_poll			pUsbHost_mem_block->in_poll
#define	initdone		pUsbHost_mem_block->initdone

/* 'ohci' and 'ehci' task share one taskid as they share
 * the same usb host controller from hardware view.
 */
static u32_t usbdOhciEhciTaskIsRunning; /* running - 1, exit - 0 */

/* card removed or not */
extern u32_t usbdOhciEhciCardRemoved;

/* Local overwrite macros */
#undef EHCI_WRITECSR
#define EHCI_WRITECSR(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) = (y))
#undef EHCI_READCSR
#define EHCI_READCSR(ehci_regs, x) \
			(*((volatile u32_t *) (ehci_regs + (x))))
#undef EHCI_SET_BITS
#define EHCI_SET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) |= (y))
#undef EHCI_UNSET_BITS
#define EHCI_UNSET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) &= (~(y)))

/* Externs */

extern const usb_hcdrv_t ehci_driver;	/* EHCI Driver dispatch */
extern const usb_hcdrv_t ohci_driver;	/* OHCI Driver dispatch */

int usbhost_initDataStruct(u32_t addr); /* forward */
int ui_init_usbcmds(void);	/* forward */


#ifdef USBHOST_INTERRUPT_MODE
void usbhost_isr(void *arg)
{
	ARG_UNUSED(arg);
	int idx;

	irq_disable(SPI_IRQ(USB2H_IRQ));
	for (idx = 0; idx < usb_buscnt; idx++) {
		if (usb_buses[idx])
			UBINTR(usb_buses[idx]);
	}
	irq_enable(SPI_IRQ(USB2H_IRQ));
}

void usbhost_register_isr(void)
{
	irq_disable(SPI_IRQ(USB2H_IRQ));
	IRQ_CONNECT(SPI_IRQ(USB2H_IRQ), CONFIG_USBH_IRQ_PRI, usbhost_isr,
		    NULL, 0);
	irq_enable(SPI_IRQ(USB2H_IRQ));
}

void usbhost_unregister_isr(void)
{
	irq_disable(SPI_IRQ(USB2H_IRQ));
}
#endif

/*
 *  usb_init_one_ehci(addr)
 *
 *  Initialize one EHCI USB controller.
 *
 *  Input parameters:
 *	addr - physical address of EHCI registers
 *
 *  Return value:
 *	0 if ok
 *	else error
 */

extern usbdev_t *usbhid_dev ;	/* XX hack for testing only */
extern usbdev_t *usbmass_dev;

static int usb_init_one_ehci(u32_t addr)
{
	usbbus_t *bus;
	int res;
	u32_t ehci_regs = addr;
	u32_t regVal;

	printf("\n ---  EHCI start  ---   %s\n", __func__);
	printf("EHCI regs=%08x\n", ehci_regs);

#ifdef USBHOST_INTERRUPT_MODE
	usbhost_register_isr();
#endif

	/* EHCI configuration */
	EHCI_SET_BITS(ehci_regs, R_EHCI_USBCMD, V_EHCI_RUN_STOP(1));
	EHCI_SET_BITS(ehci_regs, R_EHCI_CONFIGFLAG, V_EHCI_CONFIGURE_FLAG(1));

	do {
		regVal = EHCI_READCSR(ehci_regs, R_EHCI_USBSTS);
	} while (regVal & M_EHCI_HC_HALTED);

	printf("EHCI create...\n");
	bus = UBCREATE(&ehci_driver, addr); /* ehci_create */

	if (bus == NULL) {
		printf("USBH Could not create EHCI driver structure for controller at 0x%08X\n",
		       addr);
		return -1;
	}

	bus->ub_num = usb_buscnt;

	printf("EHCI start...\n");
	res = UBSTART(bus); /* ehci_start */

	if (res != 0) {
		printf("USBH Could not init EHCI controller at 0x%08X\n", addr);
		UBSTOP(bus);
		return -1;
	}

	/* give enough time for device reset */
	usb_delay_ms(200);
	usb_buses[usb_buscnt++] = bus;
	usbHostOhci = 0;
	usb_initroot(bus);
#ifndef USBHOST_INTERRUPT_MODE
	while (1) {
		usb_poll(bus);
		usb_daemon(bus);
		usb_delay_ms(20);
#ifdef CONFIG_USBMS
		if (usbmass_dev)
			break;
#endif
	}
#else /* USBHOST_INTERRUPT_MODE */
	usb_daemon(bus);
#endif

#ifdef CONFIG_USBMS
	if (usbmass_dev) {
		printf("usbmass_dev detected\n");
		res = UsbDeviceInit(usbmass_dev);
		usb_delay_ms(10);
		iproc_usb_host_init();
	}
#endif
	return 0;
}

usbbus_t *bus;
ohci_softc_t *ohci;

/*
 *  usb_init_one_ohci(addr)
 *
 *  Initialize one USB controller.
 *
 *  Input parameters:
 *	addr - physical address of OHCI registers
 *
 *  Return value:
 *	0 if ok
 *	else error
 */
int usb_init_one_ohci(u32_t addr)
{
	int res;

/* Create the OHCI driver instance. */
	printf("\n ---  OHCI start  ---   %s\n", __func__);
	usb_delay_ms(10);

#ifdef USBHOST_INTERRUPT_MODE
	usbhost_register_isr();
#endif

	bus = UBCREATE(&ohci_driver, addr); /* ohci_create */

	bus->ub_num = usb_buscnt;

	/* Hack: retrieve copy of softc for our exception handler */
	ohci = (ohci_softc_t *) bus->ub_hwsoftc;

	/* allocate to usb_buses before the interrupts are enabled*/
	usb_buses[usb_buscnt++] = bus;

	/* Start the controller. */
	res = UBSTART(bus); /* ohci_start */
	if (res != 0) {
		printf("Could not init hardware\n");
		UBSTOP(bus);
	}

	usbHostOhci = 1;

	/* Init the root hub */
	usb_delay_ms(10);
	usb_initroot(bus);

	/* Main loop - just call interrupt routine to poll */
	usb_delay_ms(10);

	while (1) {
		usb_poll(bus);
		usb_daemon(bus);
		usb_delay_ms(20);
#ifdef CONFIG_USBMS
		if (usbmass_dev)
			break;
#endif
	}

#ifdef CONFIG_USBMS
	if (usbmass_dev) {
		res = UsbDeviceInit(usbmass_dev);
		usb_delay_ms(10);
		iproc_usb_host_init();
	}
#endif
	return 0;
}

/*
 *  usbhost_initDataStruct()
 *
 *  Initialize the USB subsystem
 *
 *  Input parameters:
 *	addr - physical address of OHCI registers
 *		(0 if all registers are to be discovered dynamically)
 *
 *  Return value:
 *	0 if ok
 *	else error code
 */

int usbhost_initDataStruct(u32_t addr)
{
	ARG_UNUSED(addr);

	printf("USBH Initializing...\n");

	usbdOhciEhciCardRemoved = 0;
	usb_buscnt = 0;

	/* if the task still there, delete it. only one task can run to avoid
	 * resource conflicted as usb controller is shared.
	 */
	if (usbdOhciEhciTaskIsRunning) {
		usbdOhciEhciTaskIsRunning = 0;
	}

	if (usbHostOhci) { /* ohci */
		usbdOhciEhciTaskIsRunning = 1;
		if (usb_init_one_ohci(USB_OHCI_BASE) == -1)
			/* in case of task exit with some errors. */
			usbdOhciEhciTaskIsRunning = 0;
	} else { /* ehci */
		usbdOhciEhciTaskIsRunning = 1;
		if (usb_init_one_ehci(USB_EHCI_BASE) == -1)
			/* in case of task exit with some errors. */
			usbdOhciEhciTaskIsRunning = 0;
	}

	return 0;
}
