
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

/* @file usb_host_bcm5820x.c
 *
 * USB host driver for Broadcom Citadel.
 * Initializes OHCI/EHCI bus and root hub.
 * Enumerates any connected device.
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

struct usb2h_config {
	mem_addr_t ehci_base;	/* EHCI base addr */
	mem_addr_t ohci_base;	/* OHCI base addr */
};

/* This API structure is added for testing purposes */
typedef int (*usb2h_host_init)(struct device *dev, int host);

struct usb2h_driver_api {
	usb2h_host_init	init;
};

static void usb_daemon(struct usbbus *bus)
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

void usb_delay_us(int cnt)
{
	k_busy_wait(cnt);
}

void usb_delay_ms(int ms)
{
	volatile int i;

	for (i = 0; i < 1000; i++)
		usb_delay_us(ms);
}

static void usb_roothub_init(struct device *dev, struct usbbus *bus)
{
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;
	struct usbdev *usbdev;
	struct usb_driver *drv;
	int addr;
	int res;
	u8_t *buf;
	int len;
	struct usb_config_descr cfgdescr;

	printk("%s!\n", __func__);

	/*
	 * Create a device for the root hub.
	 */
	usbdev = usb_create_device(bus, USB_ROOT_HUB_DEV_NUM, 0);
	bus->ub_roothub = usbdev;

	if (data->cur_host == OHCI) {
		/*
		 * Get the device descriptor.  Make sure it's a hub.
		 */
		res = usb_get_device_descriptor(usbdev,
			 &(usbdev->ud_devdescr), true);

		if (usbdev->ud_devdescr.bDeviceClass != USB_DEVICE_CLASS_HUB) {
			SYS_LOG_ERR("Error! Root device is not a hub!\n");
			return;
		}
	}

	/*
	 * Set up the max packet size for the control endpoint,
	 * then get the rest of the descriptor.
	 */

	usb_set_ep0mps(usbdev, usbdev->ud_devdescr.bMaxPacketSize0);
	res = usb_get_device_descriptor(usbdev,
			 &(usbdev->ud_devdescr), false);

	/*
	 * Obtain a new address and set the address of the
	 * root hub to this address.
	 */

	addr = usb_new_address(usbdev->ud_bus);
	res = usb_set_address(usbdev, addr);

	/*
	 * Get the configuration descriptor and all the
	 * associated interface and endpoint descriptors.
	 */
	res = usb_get_config_descriptor(usbdev, &cfgdescr, 0,
				sizeof(struct usb_config_descr));
	if (res != sizeof(struct usb_config_descr))
		SYS_LOG_ERR("[a]usb_get_config_descriptor returns %d\n", res);

	len = GETUSBFIELD(&cfgdescr, wTotalLength);
	buf = cache_line_aligned_alloc(len);

	res = usb_get_config_descriptor(usbdev,
		(struct usb_config_descr *) buf, 0, len);
	if (res != len) {
		SYS_LOG_ERR
		    ("[b]usb_get_config_descriptor returns %d, len=%d\n",
		     res, len);
	}

	usbdev->ud_cfgdescr = (struct usb_config_descr *) buf;
	/*
	 * Select the configuration.  Not really needed for our poor
	 * imitation root hub, but it's the right thing to do.
	 */

	usb_set_dev_configuration(usbdev, cfgdescr.bConfigurationValue);

	/*
	 * Find the driver for this.  It had better be the hub
	 * driver.
	 */

	drv = usb_find_driver(usbdev);

	/*
	 * Call the attach method.
	 */
	if (drv->udrv_attach)
		(*(drv->udrv_attach)) (usbdev, drv);

	/*
	 * Hub should now be operational.
	 */
}

static void usbhost_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;
	struct usbbus *bus = NULL;
	int idx;

	irq_disable(SPI_IRQ(USBH_IRQ));
	for (idx = 0; idx < data->usb_buscnt; idx++) {
		bus = data->usb_buses[idx];
		if (bus) {
			UBINTR(bus);
		}
	}
	irq_enable(SPI_IRQ(USBH_IRQ));
}

DEVICE_DECLARE(usb2h);
static void usbhost_register_isr(void)
{
	irq_disable(SPI_IRQ(USBH_IRQ));
	IRQ_CONNECT(SPI_IRQ(USBH_IRQ), USBH_IRQ_PRI, usbhost_isr,
		    DEVICE_GET(usb2h), 0);
	irq_enable(SPI_IRQ(USBH_IRQ));
}

static int usb_init_ehci(struct device *dev)
{
	int ret = 0;
	struct usbbus *bus;
	u32_t cnt, val;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;

	printk("%s: EHCI Init..\n", __func__);
#ifdef USBHOST_INTERRUPT_MODE
	usbhost_register_isr();
#endif

	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_USBCMD, V_EHCI_RUN_STOP(1));
	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_CONFIGFLAG,
		      V_EHCI_CONFIGURE_FLAG(1));
	do {
		val = EHCI_READCSR(USB_EHCI_BASE, R_EHCI_USBSTS);
	} while (val & M_EHCI_HC_HALTED);

	printk("%s: EHCI Create..\n", __func__);
	bus = UBCREATE(&ehci_driver, USBH_EHCI_BASE_ADDR);
	if (bus == NULL) {
		SYS_LOG_ERR("Error creating USB device\n");
		return -ENXIO;
	}

	bus->dev = dev;
	cnt = data->usb_buscnt;
	bus->ub_num = data->usb_buscnt;

	data->usb_buses[cnt++] = bus;
	data->usb_buscnt = cnt;

	/* Start the controller */
	ret = UBSTART(bus);
	if (ret) {
		SYS_LOG_ERR("Could not init hardware");
		UBSTOP(bus);
		return ret;
	}
	data->init = true;

	/* Initialize the root hub */
	usb_delay_ms(200);
	printk("%s: Root HUB init..\n", __func__);
	usb_roothub_init(dev, bus);

	usb_delay_ms(10);

	/*Scan for any hubevent */

	usb_daemon(bus);
	usb_delay_ms(20);

	return ret;
}

static int usb_init_ohci(struct device *dev)
{
	int ret = 0;
	struct usbbus *bus;
	u32_t cnt;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;

#ifdef USBHOST_INTERRUPT_MODE
	usbhost_register_isr();
#endif
	bus = UBCREATE(&ohci_driver, USBH_OHCI_BASE_ADDR);

	bus->dev = dev;
	cnt = data->usb_buscnt;
	bus->ub_num = data->usb_buscnt;

	data->usb_buses[cnt++] = bus;
	data->usb_buscnt = cnt;

	/* Start the controller */
	ret = UBSTART(bus);
	if (ret) {
		SYS_LOG_ERR("Could not init hardware");
		UBSTOP(bus);
	}

	data->init = true;

	/* Initialize the root hub */
	usb_delay_ms(10);
	usb_roothub_init(dev, bus);

	/* Scan for any hubevent */

	usb_daemon(bus);
	usb_delay_ms(20);

	return ret;
}

#define TASK_STACK_SIZE 2048
static K_THREAD_STACK_DEFINE(usbhost_task_stack, TASK_STACK_SIZE);
static struct k_thread usbhost_task;

static void usbhost_task_fn(void *p1, void *p2, void *p3)
{
	struct device *dev = (struct device *)p1;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;
	struct usbbus *bus = data->usb_buses[0];

	if (!data->init)
		return;

	while (1) {
		usb_poll(bus);
		usb_daemon(bus);
		k_sleep(20);
	}
}

static int usbhost_init_controller(struct device *dev)
{
	int ret = 0;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;

	printk("USBH: Initializing %s...\n",
	       (data->cur_host == EHCI) ? "EHCI" : "OHCI");

	data->usbh_removed = 0;
	data->usb_buscnt = 0;

	if (data->cur_host == OHCI) {
		ret = usb_init_ohci(dev);
	} else {
		ret = usb_init_ehci(dev);
	}

	return ret;
}

static void usbh_phy_init(void)
{
	u32_t val;

	/* Disable LDO */
	val = sys_read32(CDRU_USBPHY_H_CTRL2);
	val |= (1 << CDRU_USBPHY_H_CTRL2__ldo_pwrdwnb);
	val |= (1 << CDRU_USBPHY_H_CTRL2__afeldocntlen);
	sys_write32(val, CDRU_USBPHY_H_CTRL2);

	/* Disable Isolation */
	usb_delay_ms(5);
	val = sys_read32(CRMU_USBPHY_H_CTRL);
	val &= ~(1 << CRMU_USBPHY_H_CTRL__phy_iso);
	sys_write32(val, CRMU_USBPHY_H_CTRL);

	/* Enable IDDQ current */
	val = sys_read32(CDRU_USBPHY_H_CTRL2);
	val &= ~(1 << CDRU_USBPHY_H_CTRL2__iddq);
	sys_write32(val, CDRU_USBPHY_H_CTRL2);

	/* UTMI CLK for 60 Mhz: From Design team */
	usb_delay_ms(150);
	val = sys_read32(CRMU_USBPHY_H_CTRL) & 0xfff00000;
	val |= 0xec4ec; /* 'd967916 */
	sys_write32(val, CRMU_USBPHY_H_CTRL);

	val = sys_read32(CDRU_USBPHY_H_CTRL1) & 0xfc000007;
	val |= (0x03 << CDRU_USBPHY_H_CTRL1__ka_R); /* ka 'd3' */
	val |= (0x03 << CDRU_USBPHY_H_CTRL1__ki_R); /* ki 'd3' */
	val |= (0x0a << CDRU_USBPHY_H_CTRL1__kp_R); /* kp 'hA' */
	val |= (0x24 << CDRU_USBPHY_H_CTRL1__ndiv_int_R); /* ndiv 'd36' */
	val |= (0x01 << CDRU_USBPHY_H_CTRL1__pdiv_R); /* pdiv 'd1' */
	sys_write32(val, CDRU_USBPHY_H_CTRL1);

	/* Bring PHY out of reset */
	usb_delay_ms(1);
	val = sys_read32(CDRU_USBPHY_H_CTRL1);
	val |= (1 << CDRU_USBPHY_H_CTRL1__resetb);
	sys_write32(val, CDRU_USBPHY_H_CTRL1);

	/* Bring PLL out of reset */
	val = sys_read32(CDRU_USBPHY_H_CTRL1);
	val |= (1 << CDRU_USBPHY_H_CTRL1__pll_resetb);
	sys_write32(val, CDRU_USBPHY_H_CTRL1);

	/* Bring Port1 out of software reset */
	val = sys_read32(CDRU_USBPHY_H_P1CTRL);
	val |= (1 << CDRU_USBPHY_H_P1CTRL__soft_resetb);
	sys_write32(val, CDRU_USBPHY_H_P1CTRL);

	/* PLL locking loop */
	do {
		val = sys_read32(CDRU_USBPHY_H_STATUS);
		val &= 1 << CDRU_USBPHY_H_STATUS__pll_lock;
	} while (!val);

	SYS_LOG_INF("USBH PHY PLL locked\n");

	/* Disable non-driving */
	usb_delay_ms(1);
	val = sys_read32(CDRU_USBPHY_H_P1CTRL);
	val &= ~(1 << CDRU_USBPHY_H_P1CTRL__afe_non_driving);
	sys_write32(val, CDRU_USBPHY_H_P1CTRL);
}

static struct usb2h_config usb2h_dev_cfg = {
	.ehci_base = USBH_EHCI_BASE_ADDR,
	.ohci_base = USBH_OHCI_BASE_ADDR,
};

static struct usb2h_data usb2h_dev_data = {
	.init = false,
	.cur_host = OHCI,
};

static struct usb2h_driver_api usb2h_api;
static int bcm5820x_usbhost_init(struct device *dev)
{
	int ret = 0;
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;

	/* USB host Phy init */
	usbh_phy_init();

	if (data->init) { /* initialization already done */
		irq_disable(USBH_IRQ);
		if (data->tid)
			k_thread_abort(data->tid);
	}

	/* Reset EHCI/OHCI Host controllers */
	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_USBCMD,
		      V_EHCI_HOST_CONTROLLER_RESET(1));

	OHCI_WRITECSR(USB_OHCI_BASE, R_OHCI_CONTROL,
		      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_RESET));

	/* Disable EHCI host controller and Port  */
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_PORTSC, V_EHCI_PORT_POWER(1));
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_USBCMD, V_EHCI_RUN_STOP(1));
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_CONFIGFLAG,
			V_EHCI_CONFIGURE_FLAG(1));

	SYS_LOG_INF("USB Host driver initialized\n");

	return ret;
}


int usbhost_init(struct device *dev, int host)
{
	struct usb2h_data *data = (struct usb2h_data *)dev->driver_data;
	k_tid_t id;
	int ret;

	data->cur_host = host;

	ret = bcm5820x_usbhost_init(dev);
	if (ret) {
		SYS_LOG_ERR("Error initializing USB host\n");
		return ret;
	}

	/* Initialize USB host driver data structures */
	ret = usbhost_init_controller(dev);
	if (ret) {
		SYS_LOG_ERR("Error Initializing USB Host controller\n");
		return ret;
	}

	/* Create usbhost task for scanning hub changes */
	id = k_thread_create(&usbhost_task, usbhost_task_stack,
				TASK_STACK_SIZE, usbhost_task_fn, dev,
				NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
	if (id == NULL) {
		SYS_LOG_ERR("Error creating usbhost task\n");
		return -EPERM;
	}

	data->tid = id;

	return ret;
}

DEVICE_AND_API_INIT(usb2h, CONFIG_USB2H_DEV_NAME,
		    bcm5820x_usbhost_init, &usb2h_dev_data,
		    &usb2h_dev_cfg, POST_KERNEL,
		    CONFIG_USB2H_DRIVER_INIT_PRIORITY, &usb2h_api);
