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
 *  1.	 This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.	 TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.	 TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/*
 *  Broadcom Common Firmware Environment (CFE)
 *
 *  ehci defs	File: ehci.c
 *
 *  Enhanced Host Controller Interface definitions
 *
 *  Author:  Johnny Chen
 *
 */

/*CFE includes */
#include "cfeglue.h"
/*USB includes */
#include "usbchap9.h"
#include "usbd.h"
#include "ehci.h"
#include "lib_queue.h"

/*  Local defines */
#ifdef _EHCI_DEBUG
#define EHCI_DEBUG_LOG(x, args...) printf(x, ##args)
#define EHCI_ASSERT(fmt, ...)\
	do { if (_EHCI_DEBUG) printf("%s:%d:%s(): " fmt, __FILE__, \
		 __LINE__, __func__, __VA_ARGS__); } while (0)
#else
#define EHCI_DEBUG_LOG(x, args...)
#define EHCI_ASSERT(fmt, ...)	 {}
#endif

#define EHCI_DEBUG_LOG_CRIT(x, args...) printf(x, ##args)

#define ALIGNED_BYTES(bytes)	__aligned(bytes)

u32_t usbdOhciEhciCardRemoved;

#define PERIODIC_SCHEDULE 0
#define ASYNC_SCHEDULE 1

#define DATA_TOGGLE_0	(0)
#define DATA_TOGGLE_1	(1)
#define LINK_TERMINATE	(1)

#define MULT_1			 (1)
#define MULT_2			 (2)
#define MULT_3			 (3)
#define TRANSACTIONS_PER_ITD	 (8)

/*Memory allocations for ehci structures */
static ehci_endpoint_t ALIGNED_BYTES(EHCI_EP_ALIGN)
	ehci_endp_array[EHCI_MAX_NUM_EPT_PER_PORT];
static ehci_qTD_t ALIGNED_BYTES(EHCI_QTD_ALIGN)
	ehci_qtd_array[EHCI_MAX_NUM_TD_PER_XFER];
static ehci_qH_t ALIGNED_BYTES(EHCI_QH_ALIGN)
	ehci_qh_array[EHCI_MAX_NUM_EPT_PER_PORT];
static ehci_flist_t ALIGNED_BYTES(EHCI_FL_ALIGN)
	ehci_flist_array[FRAME_LIST_SIZE];
static ehci_iTD_t ALIGNED_BYTES(EHCI_ITD_ALIGN)
	ehci_iTD_array[EHCI_MAX_NUM_ITD];

/*Macros to use the data structures due to cached and uncached memory locations */
/*Usb hardware expects uncached address */
#define get_ehci_ep(x)	  (&ehci_endp_array[x])
#define get_ehci_qtd(x)	 (addr2uncached(&ehci_qtd_array[x]))
#define get_ehci_qh(x)	  (addr2uncached(&ehci_qh_array[x]))
#define get_ehci_fl(x)	  (addr2uncached(&ehci_flist_array[x]))
#define get_ehci_itd(x)	 (addr2uncached(&ehci_iTD_array[x]))

#define MIN(x, y)   (((x) < (y)) ? (x) : (y))
#define MAX(x, y)   (((x) > (y)) ? (x) : (y))

static usbbus_t ehci_bus;
static ehci_softc_t ehci_soft;

static ehci_qTD_p ehci_qtd_create(u32_t total_bytes, u32_t ioc,
				  u32_t c_page, u32_t cerr, u32_t active,
				  u32_t flag, u8_t *data_buffer_addr,
				  unsigned char qtd_num);
static void ehci_init_qH(ehci_qH_p qH, ehci_endpoint_p ept);
static void ehci_init_fList(void);
static int ehci_queueiTDs(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur);
void ehci_setupiTD(ehci_endpoint_t *ept, u32_t mps, u8_t *dataBuff,
		   u32_t xferLen, ehci_iTD_p iTDptr, u8_t records,
		   u8_t mult, u8_t ioc);
static void ehci_init_ItdList(void);
static void ehci_disableSchedule(usbbus_t *bus, u8_t type);
static void ehci_enableSchedule(usbbus_t *bus, u8_t type);
static usbbus_t *ehci_create(physaddr_t addr);
static void ehci_delete(usbbus_t *bus);
static int ehci_start(usbbus_t *bus);
static void ehci_stop(usbbus_t *bus);
static int ehci_intr(usbbus_t *bus);
static usb_ept_t *ehci_ept_create(usbbus_t *bus, int usbaddr, int eptnum,
				  int mps, int flags);
static void ehci_ept_delete(usbbus_t *bus, usb_ept_t *uept);
static void ehci_ept_setmps(usbbus_t *bus, usb_ept_t *uept, int mps);
static void ehci_ept_setaddr(usbbus_t *bus, usb_ept_t *uept, int usbaddr);
static int ehci_xfer(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur);
static int ehci_cancel_xfer(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur);

/*Driver structure */
const usb_hcdrv_t ehci_driver = {
	ehci_create,
	ehci_delete,
	ehci_start,
	ehci_stop,
	ehci_intr,
	ehci_ept_create,
	ehci_ept_delete,
	ehci_ept_setmps,
	ehci_ept_setaddr,
	NULL,
	ehci_xfer,
	ehci_cancel_xfer
};

static const usb_endpoint_descr_t ehci_root_epdsc = {
	sizeof(usb_endpoint_descr_t),	/*bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,	/*bDescriptorType */
	(USB_ENDPOINT_DIRECTION_IN | 1),	/*bEndpointAddress */
	USB_ENDPOINT_TYPE_INTERRUPT,	/*bmAttributes */
	8,			/*wMaxPacketSizeLow */
	0,			/*wMaxPacketSizeHigh */
	255			/*bInterval */
};

static const usb_device_descr_t ehci_root_devdsc = {
	sizeof(usb_device_descr_t),	/*bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,	/*bDescriptorType */
	USBWORD(0x0100),	/*bcdUSB */
	USB_DEVICE_CLASS_HUB,	/*bDeviceClass */
	0,			/*bDeviceSubClass */
	0,			/*bDeviceProtocol */
	64,			/*bMaxPacketSize0 */
	USBWORD(0),		/*idVendor */
	USBWORD(0),		/*idProduct */
	USBWORD(0x0100),	/*bcdDevice */
	1,			/*iManufacturer */
	2,			/*iProduct */
	0,			/*iSerialNumber */
	1			/*bNumConfigurations */
};

static const usb_config_descr_t ehci_root_cfgdsc = {
	sizeof(usb_config_descr_t),	/*bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,	/*bDescriptorType */
	USBWORD(sizeof(usb_config_descr_t) +
		sizeof(usb_interface_descr_t) +
		sizeof(usb_endpoint_descr_t)),	/*wTotalLength */
	1,			/*bNumInterfaces */
	1,			/*bConfigurationValue */
	0,			/*iConfiguration */
	USB_CONFIG_SELF_POWERED,	/*bmAttributes */
	0			/*MaxPower */
};

usb_interface_descr_t ehci_root_ifdsc = {
	sizeof(usb_interface_descr_t),	/*bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,	/*bDescriptorType */
	0,			/*bInterfaceNumber */
	0,			/*bAlternateSetting */
	1,			/*bNumEndpoints */
	USB_INTERFACE_CLASS_HUB,	/*bInterfaceClass */
	0,			/*bInterfaceSubClass */
	0,			/*bInterfaceProtocol */
	0			/*iInterface */
};

usb_hub_descr_t ehci_root_hubdsc = {
	USB_HUB_DESCR_SIZE,	/*bLength */
	USB_HUB_DESCRIPTOR_TYPE,	/*bDescriptorType */
	0,			/*bNumberOfPorts */
	USBWORD(0),		/*wHubCharacteristics */
	0,			/*bPowreOnToPowerGood */
	0,			/*bHubControl Current */
	{0}			/*bRemoveAndPowerMask */
};

extern const usb_hcdrv_t ehci_driver;

void dump_itd(ehci_iTD_p itdptr)
{
	u8_t i = 0;

	printf("\n  == %s itdPtr:0x%p", __func__, itdptr);
	for (i = 0; i < 8; i++) {
		printf
		    ("\n  = = %d == Active:%d DBE:%d BD:%d TE: %d Length:0x%x ioc: 0x%x PG: %d Offset:0x%x",
		     i, itdptr->Transaction[i].Active,
		     itdptr->Transaction[i].BufferError,
		     itdptr->Transaction[i].Babble,
		     itdptr->Transaction[i].XactError,
		     itdptr->Transaction[i].Length, itdptr->Transaction[i].ioc,
		     itdptr->Transaction[i].PG, itdptr->Transaction[i].Offset);
	}

	for (i = 0; i < 7; i++) {
		printf("\n  == %d == Buf_Ptr: 0x%x %x ", i,
		       (itdptr->Buffer_Pointer[i] & ~0xFFF) >> 12,
		       (itdptr->Buffer_Pointer[i] & 0xFFF));
	}
}

#ifdef _EHCI_DEBUG
void dump_itd_address(void)
{
	u32_t i = 0;
	ehci_iTD_p itdptr;

	for (i = 0; i < EHCI_MAX_NUM_ITD; i++) {
		itdptr = get_ehci_itd(i);
		EHCI_DEBUG_LOG("\n %d==>>itdptr 0x%x *itdptr =>> 0x%x ", i,
			       itdptr, *itdptr);
	}
}

void dump_fl_address(void)
{
	u32_t i = 0;
	ehci_flist_p fp;

	for (i = 0; i < FRAME_LIST_SIZE; i++) {
		fp = get_ehci_fl(i);
		EHCI_DEBUG_LOG("\n %d==>>fp 0x%x *fp =>> 0x%x ", i, fp, *fp);
	}
}
#endif
/*  Function definitions */
static void ehci_disableSchedule(usbbus_t *bus, u8_t type)
{
	u32_t CMD = 0;
	u32_t USB_STS;
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	CMD = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBCMD);
	CMD &= ~((type == ASYNC_SCHEDULE) ?
		 V_EHCI_ASYNCHRONOUS_SCHEDULE_ENABLE(1) :
		 V_EHCI_PERIODIC_SCHEDULE_ENABLE(1));

	EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_USBCMD, CMD);
	if (type == ASYNC_SCHEDULE) {
		do {
			USB_STS =
			    EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
		} while (USB_STS & M_EHCI_ASYNCHRONOUS_SCHEDULE_STATUS);
	} else {
		do {
			USB_STS =
			    EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
		} while (USB_STS & M_EHCI_PERIODIC_SCHEDULE_STATUS);
	}
}

static void ehci_enableSchedule(usbbus_t *bus, u8_t type)
{
	u32_t CMD = 0;
	u32_t USB_STS;
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_n_invalidate_dcache_by_addr((u32_t)ehci_endp_array,
					 (u32_t)sizeof(ehci_endp_array));
	clean_n_invalidate_dcache_by_addr((u32_t)ehci_qtd_array,
					 (u32_t)sizeof(ehci_qtd_array));
	clean_n_invalidate_dcache_by_addr((u32_t)ehci_qh_array,
					 (u32_t)sizeof(ehci_qh_array));
	clean_n_invalidate_dcache_by_addr((u32_t)ehci_flist_array,
					 (u32_t)sizeof(ehci_flist_array));
	clean_n_invalidate_dcache_by_addr((u32_t)ehci_iTD_array,
					 (u32_t)sizeof(ehci_iTD_array));
#endif
	CMD = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBCMD);
	CMD |= ((type == ASYNC_SCHEDULE) ?
		V_EHCI_ASYNCHRONOUS_SCHEDULE_ENABLE(1) :
		V_EHCI_PERIODIC_SCHEDULE_ENABLE(1));

	CMD |= V_EHCI_INTERRUPT_THRESHOLD_CONTROL(1) | V_EHCI_RUN_STOP(1);
	EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_USBCMD, CMD);
	if (type == ASYNC_SCHEDULE) {
		do {
			USB_STS =
			    EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
		} while (!(USB_STS & M_EHCI_ASYNCHRONOUS_SCHEDULE_STATUS));
	} else {
		do {
			USB_STS =
			    EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
		} while (!(USB_STS & M_EHCI_PERIODIC_SCHEDULE_STATUS));
	}
}

/*
 *  ehci_create(addr)
 *
 *  Create a USB bus structure and associate it with our HCI
 *  controller device.
 *
 *  Input parameters:
 *		 addr - physical address of controller
 *
 *  Return value:
 *		 usbbus structure pointer
 */
static usbbus_t *ehci_create(physaddr_t addr)
{
	ehci_softc_t *softc;
	usbbus_t *bus;

	softc = &ehci_soft;
	bus = &ehci_bus;

	softc->ehci_bus = bus;

	memset((void *)softc, 0, sizeof(ehci_softc_t));
	memset((void *)bus, 0, sizeof(usbbus_t));

	bus->ub_hwsoftc = (usb_hc_t *)softc;
	bus->ub_hwdisp = (usb_hcdrv_t *)&ehci_driver;

#ifdef _CFE_
	softc->ehci_regs = addr;
#else
	softc->ehci_regs = (volatile u32_t *)addr;
#endif

	softc->ehci_rh_newaddr = -1;

	return bus;
}

static void ehci_delete(usbbus_t *bus)
{
	ehci_softc_t *softc;

	softc = &ehci_soft;
	memset((void *)softc, 0, sizeof(ehci_softc_t));
	memset((void *)bus, 0, sizeof(usbbus_t));
}

static int ehci_start(usbbus_t *bus)
{
	u32_t regVal;
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	/*clear port change detect status before power on it. */
	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_USBSTS,
		      V_EHCI_PORT_CHANGE_DETECT(1));

	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_PORTSC, V_EHCI_PORT_POWER(1));
	EHCI_DEBUG_LOG
	    ("EHCI PORT 0 powering up and waiting for a device inserted.\n");

	do {
		regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
	} while ((regVal & M_EHCI_PORT_CHANGE_DETECT) == 0);
	EHCI_DEBUG_LOG("EHCI PORT 0 power change detected\n");

	/*t3(TATTDB) - debounce interval, at least 100ms from usb2.0 spec. */
	usb_delay_ms(100);

	/* if the usb device is low speed[line status:
	 * 'D+'= 0, 'D-'= 1], return error.
	 */
	if (EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_PORTSC) & 0x0400) {
		EHCI_DEBUG_LOG
		    ("\n***ERROR! EHCI PORT 0 Low-Speed device detected ***\n");
		EHCI_DEBUG_LOG
		    ("\n***Please use 'usbhost_test_main' command to test the Low-Speed device ***\n\n");
		return -1;
	}

	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_USBSTS,
		      V_EHCI_PORT_CHANGE_DETECT(1));
	EHCI_UNSET_BITS(softc->ehci_regs, R_EHCI_PORTSC,
			V_EHCI_PORT_ENABLE_DISABLE(1));
	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_PORTSC, V_EHCI_PORT_RESET(1));
	EHCI_DEBUG_LOG("EHCI PORT 0 reset\n");

	/*  t5(TDRST) - at least 50ms for High Speed mode, from usb2.0 spec. */
	usb_delay_ms(50);

	EHCI_UNSET_BITS(softc->ehci_regs, R_EHCI_PORTSC, V_EHCI_PORT_RESET(1));

	/*
	 *Remember how many ports we have
	 *
	 *regVal = EHCI_READ_CAPR(softc->ehci_regs, R_EHCI_HCSPARAMS);
	 *softc->ehci_ndp = GET_EHCI_HCSPRMS_NPORTS(regVal);
	 *
	 *-- We are concerned with one port only lets keep it simple --
	 */
	softc->ehci_ndp = 1;

	do {
		regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_PORTSC);
	} while ((regVal & M_EHCI_PORT_ENABLE_DISABLE) == 0);
	EHCI_DEBUG_LOG("EHCI PORT 0 enabled\n");

	do {
		regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_PORTSC);
	} while (regVal & M_EHCI_PORT_RESET);
	EHCI_DEBUG_LOG("EHCI PORT 0 reset disabled\n");

	do {
		regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_PORTSC);
	} while ((regVal & M_EHCI_CONNECT_STATUS_STATUS) == 0);
	EHCI_DEBUG_LOG("EHCI PORT 0 device connected\n");

#ifdef USBHOST_INTERRUPT_MODE
	regVal = (EHCI_USBINTR_IntAsyncAdvanceEnable |
		  EHCI_USBINTR_HostSystemErrorEnable |
		  EHCI_USBINTR_FrameListRolloverEnable |
		  EHCI_USBINTR_PortChangeIntEnable |
		  EHCI_USBINTR_UsbErroIntEnable | EHCI_USBINTR_UsbIntEnable);

	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_USBINTR, regVal);
	EHCI_DEBUG_LOG("EHCI USB interrupt enabled\n");
#endif

	return EHCI_ERR_NOERROR;
}

static void ehci_stop(usbbus_t *bus)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_PORTSC, V_EHCI_PORT_RESET(1));
}

static int ehci_usbIntIsr(usbbus_t *bus)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	u32_t Idx;
	int status = -1;
	u32_t ioc = 0;
	u32_t no_itdfound = 0;
	ehci_flist_p FLptr = NULL;
	ehci_iTD_p iTDp;

	u32_t active = 0;


	EHCI_DEBUG_LOG("\n %s", __func__);

	/*PERIODIC */
	if (softc->usb_Reqtype == UP_TYPE_ISOC) {
		EHCI_DEBUG_LOG(" ==> ISOC IOC\n");
		for (Idx = 0; Idx < FRAME_LIST_SIZE; Idx++) {
			/*get the F-List base */
			FLptr = get_ehci_fl(Idx);

			/*link is valid and type is a ITD */
			if ((FLptr->T != LINK_TERMINATE) &&
			    (FLptr->Typ == EHCI_LP_TYPE_ITD)) {
				no_itdfound += 1;
				/*derieve Itd from F-List link pointer */
				iTDp = (ehci_iTD_p) (FLptr->LP & 0xFFFFFFE0);

				EHCI_DEBUG_LOG
				    ("\n itdsfound %d ITD at %d FLptr: 0x%x FLptr->LP:0x%x pItd:0x%x ",
				     no_itdfound, Idx, FLptr, FLptr->LP, iTDp);
				active = 0;
				active =
				    (iTDp->Transaction[0].Active | iTDp->
				     Transaction[1].Active | iTDp->
				     Transaction[2].Active | iTDp->
				     Transaction[3].Active | iTDp->
				     Transaction[4].Active | iTDp->
				     Transaction[5].Active | iTDp->
				     Transaction[6].Active | iTDp->
				     Transaction[7].Active);
#ifdef EHCI_ISR_DEBUG
				/* -- Following section is to check for transaction level errors */
				BufferError = 0;
				BufferError =
				    (iTDp->Transaction[0].BufferError | iTDp->
				     Transaction[1].BufferError | iTDp->
				     Transaction[2].BufferError | iTDp->
				     Transaction[3].BufferError | iTDp->
				     Transaction[4].BufferError | iTDp->
				     Transaction[5].BufferError | iTDp->
				     Transaction[6].BufferError | iTDp->
				     Transaction[7].BufferError);

				XactError = 0;
				XactError =
				    (iTDp->Transaction[0].XactError | iTDp->
				     Transaction[1].XactError | iTDp->
				     Transaction[2].XactError | iTDp->
				     Transaction[3].XactError | iTDp->
				     Transaction[4].XactError | iTDp->
				     Transaction[5].XactError | iTDp->
				     Transaction[6].XactError | iTDp->
				     Transaction[7].XactError);

				Babble = 0;
				/* -- When a device transmits more data on the USB than the host controller
				 *  is expecting for this transaction, it is defined to be babbling.
				 *  In general, this is called a Packet Babble.--
				 */
				Babble =
				    (iTDp->Transaction[0].Babble | iTDp->
				     Transaction[1].Babble | iTDp->
				     Transaction[2].Babble | iTDp->
				     Transaction[3].Babble | iTDp->
				     Transaction[4].Babble | iTDp->
				     Transaction[5].Babble | iTDp->
				     Transaction[6].Babble | iTDp->
				     Transaction[7].Babble);
				if (BufferError || XactError || Babble)
					EHCI_DEBUG_LOG_CRIT
					    ("\n BufferError:%x XactError:%x Babble:%x ",
					     BufferError, XactError, Babble);
#endif
				ioc = 0;
				ioc =
				    (iTDp->Transaction[0].ioc | iTDp->
				     Transaction[1].ioc | iTDp->Transaction[2].
				     ioc | iTDp->Transaction[3].ioc | iTDp->
				     Transaction[4].ioc | iTDp->Transaction[5].
				     ioc | iTDp->Transaction[6].ioc | iTDp->
				     Transaction[7].ioc);

				if (!active) {
					/*found a completed Itd */
					if (ioc)
						status = 0;

					/*-- Release executed ITD --*/
					/*-- Not required as we do it during setup itd --*/
					iTDp->nextLinkPointer.T =
					    LINK_TERMINATE;

					/*-- free the Flist entry--*/
					FLptr->LP = 0x00;
					FLptr->T = LINK_TERMINATE;
				}

				/*Check if the number of handled and the number of Itds allocated is same */
				/*No need to traverse the frame list anymore */
				if (softc->no_itdalloc == no_itdfound)
					break;
			}
		}

		softc->usb_Reqtype = 0;

		/*signal the upper layer */
		usb_complete_request(softc->qReq, status);

	}

	/*ASYNCHRONOUS */
	if (softc->usb_Reqtype & (UP_TYPE_CONTROL | UP_TYPE_BULK)) {
		ehci_qTD_t **qTD = softc->qTD;

		/*disable the ASYNCHRONOUS schedule */
		ehci_disableSchedule(bus, ASYNC_SCHEDULE);

		EHCI_DEBUG_LOG(" ==> CTRL|BULK IOC\n");
		usb_complete_request(softc->qReq, status);

		/*cleanup the Qtd area */
		for (Idx = 0; Idx < EHCI_MAX_NUM_TD_PER_XFER; Idx++) {
			if (qTD[Idx])
				qTD[Idx] = NULL;
		}

		softc->usb_Reqtype = 0;

		/*Cleanup Qtd Area */
		memset((void *)qTD, 0,
		       sizeof(ehci_qTD_t *) * EHCI_MAX_NUM_TD_PER_XFER);
	}

	return 0;
}

static void ehci_usbErrorIntIsr(usbbus_t *bus, u32_t intStatus)
{
	ARG_UNUSED(bus);
	ARG_UNUSED(intStatus);
	/*
	 *General error indicator - expected to be processed by more
	 *specific handler, i.e. ehci_intr_transaction_done
	 */
	SYS_LOG_ERR("\nUSB_ERR!!! 0x%x bus: %d\n", intStatus, bus->ub_num);

}

static int ehci_portChangeDetIsr(usbbus_t *bus)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	u32_t regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_PORTSC);
	static int flag = 1;

	/* if device is removed, disable host controller and
	 * port power then exit.
	 */
	if (((regVal & EHCI_PORTSC_CurrentConnectStatus) == 0) &&
	    (usbdOhciEhciCardRemoved == 0)) {
		EHCI_DEBUG_LOG("EHCI PORT 0 device removed\n\n");
		/*this usbd_ehci_task need to exit with the device is
		 *removed as it currently don't support hot-plug.
		 *Mabye user will want to test the low speed device
		 *with 'usbhost_test_main' command nextly.
		 */
		usbdOhciEhciCardRemoved = 1;
		return -1;
	}

	if ((regVal & EHCI_PORTSC_Suspend) &&
	    (regVal & EHCI_PORTSC_ForcePortResume)) {
		usb_delay_ms(100);
		if (flag) {
			printf("EHCI Resume portsc=%x\n", ehci_reg->portsc_0);
			regVal &= ~V_EHCI_FORCE_PORT_RESUME(1);
			EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_PORTSC,
					regVal);
			usb_delay_ms(20);
			printf("EHCI portsc=%x end\n", ehci_reg->portsc_0);
		}
		flag = 0;
	} else {
		flag = 1;
	}

	return 0;
}

static void ehci_hostSystemErrorIsr(void)
{
	u32_t itdcnt = 0;
	ehci_iTD_p iTDp;

	EHCI_DEBUG_LOG_CRIT("\n Host System Error !!!");
	ehci_reg_dump(ehci_reg);
	for (itdcnt = 0; itdcnt < EHCI_MAX_NUM_ITD; itdcnt++) {
		iTDp = get_ehci_itd(itdcnt);
		if (iTDp->Buffer_Pointer[1] & 0x800)
			dump_itd(iTDp);
	}

	while (1)
		;
}

/*top level Ehci handler */
static int ehci_intr(usbbus_t *bus)
{
	u32_t intStatus;
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	intStatus = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
	if (intStatus == 0)
		return 0;

	if (intStatus & EHCI_USBSTS_HostSystemError)
		ehci_hostSystemErrorIsr();

	if (intStatus & EHCI_USBSTS_UsbErrorInt) {
		ehci_usbErrorIntIsr(bus, intStatus);
		/* -- timeout condition -- */
		if (!(intStatus & EHCI_USBSTS_UsbInt))
			intStatus |= EHCI_USBSTS_UsbInt;
	}

	if (intStatus & EHCI_USBSTS_UsbInt)
		/*received a ioc */
		ehci_usbIntIsr(bus);

	if (intStatus & EHCI_USBSTS_PortChangeDetect)
		ehci_portChangeDetIsr(bus);

	if (intStatus & EHCI_USBSTS_HCHalted)
		EHCI_DEBUG_LOG_CRIT("\n hcHalt");

	EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_USBSTS, intStatus);

	return 0;
}

static usb_ept_t *ehci_ept_create(usbbus_t *bus,
				  int usbaddr, int eptnum, int mps, int flags)
{
	ehci_endpoint_t *ept;
	ehci_qH_t *qH;
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;

	ept = get_ehci_ep(eptnum);
	ept->ep_flags = flags;
	ept->ep_mps = mps;
	ept->ep_num = eptnum;
	ept->ep_dev_addr = usbaddr;

	if (softc->ehci_rh_addr != usbaddr) {
		/* -- ISOC/INT -- */
		if ((flags & UP_TYPE_ISOC) || (flags & UP_TYPE_INTR)) {
			ept->ep_flags |= UP_TYPE_HIGHSPEED;
			/* -- Init the F-List -- */
			ehci_init_fList();
			if (flags & UP_TYPE_ISOC)
				/*-- Init the Itd-List -- */
				ehci_init_ItdList();

			/* -- Assign frame list to PERIODIC BASE -- */
			EHCI_WRITE_OPRR(softc->ehci_regs,
					R_EHCI_PERIODICLISTBASE,
					(unsigned int)get_ehci_fl(0));

			/* -- Kick-off periodic schedule -- */
			ehci_enableSchedule(bus, PERIODIC_SCHEDULE);

		} else {
			qH = get_ehci_qh(eptnum);
			ehci_init_qH(qH, ept);
			ept->qH = qH;
		}
	}
	softc->ept[eptnum] = ept;

	EHCI_DEBUG_LOG
	    ("\n%s bus = 0x%x ept = 0x%x usbaddr = 0x%x, eptnum = 0x%x, mps = 0x%x, flags = 0x%x",
	     __func__, bus, ept, ept->ep_dev_addr, ept->ep_num, ept->ep_mps,
	     ept->ep_flags);

	return (usb_ept_t *) ept;
}

static void ehci_ept_delete(usbbus_t *bus, usb_ept_t *uept)
{
	ehci_endpoint_p ept = (ehci_endpoint_p) uept;
	ehci_softc_p softc = (ehci_softc_p) bus->ub_hwsoftc;

	if ((ept->ep_flags & UP_TYPE_ISOC) || (ept->ep_flags & UP_TYPE_INTR)) {
		ehci_init_fList();
		ehci_init_ItdList();
	}

	if (ept->ep_flags & UP_TYPE_BULK)
		ehci_init_qH(ept->qH, ept);

	if (ept->ep_flags & UP_TYPE_CONTROL) {
		ehci_init_qH(ept->qH, ept);
	} else {
		EHCI_DEBUG_LOG("%s UNKNOWN TYPE:0x%x", ept->ep_flags);
		return;
	}

	/*release the softc */
	softc->ept[ept->ep_num] = NULL;

	/*clean the ep */
	memset(get_ehci_ep(ept->ep_num), 0, sizeof(ehci_endpoint_t));

}

static void ehci_ept_setmps(usbbus_t *bus, usb_ept_t *uept, int mps)
{
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;
	ehci_qH_t *qH = ept->qH;

	EHCI_DEBUG_LOG("\n%s ", __func__);
	ept->ep_mps = mps;
	if (qH) {
		printf(" mps: %d. bus: %d\n", mps, bus->ub_num);
		qH->max_pkt_len = mps;
	}
}

static void ehci_ept_setaddr(usbbus_t *bus, usb_ept_t *uept, int usbaddr)
{
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;

	printf("setting address for ept:%d, bus: %d\n",
	       ept->ep_num, bus->ub_num);
	ept->ep_dev_addr = usbaddr;

}

static int ehci_roothub_strdscr(u8_t *ptr, char *str)
{
	u8_t *p = ptr;

	*p++ = strlen(str) * 2 + 2;	/*Unicode strings */
	*p++ = USB_STRING_DESCRIPTOR_TYPE;
	while (*str) {
		*p++ = *str++;
		*p++ = 0;
	}
	return (p - ptr);
}

static int ehci_roothub_req(ehci_softc_t *softc, usb_device_request_t *req)
{
	u8_t *ptr;
	u16_t wValue;
#ifdef RH_DBG
	u16_t wIndex;
#endif
	usb_port_status_t ups;
	usb_hub_descr_t hdsc;
	u32_t status;
	u32_t statport;
	u32_t tmpval;
	int res = 0;

	ptr = softc->ehci_rh_buf;

	wValue = GETUSBFIELD(req, wValue);
#ifdef RH_DBG
	wIndex = GETUSBFIELD(req, wIndex);
#endif

	switch (REQSW(req->bRequest, req->bmRequestType)) {
	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    (USB_GETSTATUS_SELF_POWERED & 0xFF);
		*ptr++ = (USB_GETSTATUS_SELF_POWERED >> 8);
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		*ptr++ =
		    0;
		*ptr++ = 0;
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		status =
		    EHCI_READ_OPRR(softc->ehci_regs, (R_EHCI_PORTSCx(wIndex)));
#ifdef RH_DBG
		EHCI_DEBUG_LOG("\nEHCI_RH: wIndex %d status: 0x%x wValue: 0x%x",
			       wIndex, status, wValue);
#endif
		/*Translate the EHCI PORTSC fields into USB chapter 11
		 *wPortStatus and wPortChange fields
		 */
		tmpval = 0;
		if (GET_EHCI_PORTSC_CCS(status))
			tmpval |= USB_PORT_STATUS_CONNECT;
		if (GET_EHCI_PORTSC_PE(status))
			tmpval |= USB_PORT_STATUS_ENABLED;
		if (GET_EHCI_PORTSC_OCA(status))
			tmpval |= USB_PORT_STATUS_OVERCUR;
		if (GET_EHCI_PORTSC_S(status))
			tmpval |= USB_PORT_STATUS_SUSPEND;
#ifdef RH_DBG
		EHCI_DEBUG_LOG("\nEHCI_RH: tmpval -> wPortStatus: 0x%x",
			       tmpval);
#endif
		PUTUSBFIELD((&ups), wPortStatus, (tmpval));
#ifdef RH_DBG
		EHCI_DEBUG_LOG("\nEHCI_RH: tmpval -> wPortChange: 0x%x",
			       tmpval);
#endif
		PUTUSBFIELD((&ups), wPortChange, (tmpval));

		memcpy(ptr, &ups, sizeof(ups));
		ptr += sizeof(ups);
#ifdef RH_DBG
		EHCI_DEBUG_LOG
		    ("\nEHCI_RH: wPortStatusLow %x wPortStatusHigh %x  wPortChangeLow %x  wPortChangeHigh %x",
		     ups.wPortStatusLow, ups.wPortStatusHigh,
		     ups.wPortChangeLow, ups.wPortChangeHigh);
#endif
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		break;

	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		/*do nothing, not supported */
		break;

	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		statport =
		    EHCI_READ_OPRR(softc->ehci_regs, (R_EHCI_PORTSCx(wIndex)));

		switch (wValue) {
		case USB_PORT_FEATURE_CONNECTION:
			break;
		case USB_PORT_FEATURE_ENABLE:
#ifdef RH_DBG
			EHCI_DEBUG_LOG
			    ("%sUSB_REQUEST_CLEAR_FEATURE USB_PORT_FEATURE_ENABLE not implemented\n",
			     __func__);
#endif
			break;
		case USB_PORT_FEATURE_SUSPEND:
#ifdef RH_DBG
			EHCI_DEBUG_LOG
			    ("%sUSB_REQUEST_CLEAR_FEATURE USB_PORT_FEATURE_SUSPEND not implemented\n",
			     __func__);
#endif
			break;
		case USB_PORT_FEATURE_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_RESET:
			break;
		case USB_PORT_FEATURE_POWER:
#ifdef RH_DBG
			EHCI_DEBUG_LOG
			    ("%sUSB_REQUEST_CLEAR_FEATURE USB_PORT_FEATURE_POWER not implemented\n",
			     __func__);
#endif
			break;
		case USB_PORT_FEATURE_LOW_SPEED:
			break;
		case USB_PORT_FEATURE_C_PORT_CONNECTION:
			SET_EHCI_PORTSC_CSC(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			break;
		case USB_PORT_FEATURE_C_PORT_ENABLE:
			SET_EHCI_PORTSC_PEC(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			break;
		case USB_PORT_FEATURE_C_PORT_SUSPEND:
			/*No port suspend change status in EHCI PORTSC */
			break;
		case USB_PORT_FEATURE_C_PORT_OVER_CURRENT:
			SET_EHCI_PORTSC_OCC(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			break;
		case USB_PORT_FEATURE_C_PORT_RESET:
			/*No port reset change status in EHCI PORTSC */
			break;
		}
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		res =
		    -1;
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		/*nothing */
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		statport =
		    EHCI_READ_OPRR(softc->ehci_regs, (R_EHCI_PORTSCx(wIndex)));
		switch (wValue) {
		case USB_PORT_FEATURE_CONNECTION:
			break;
		case USB_PORT_FEATURE_ENABLE:
			SET_EHCI_PORTSC_PE(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			/*USB 2.0 spec states 10ms for port reset */
			usb_delay_ms(10);
			break;
		case USB_PORT_FEATURE_SUSPEND:
			SET_EHCI_PORTSC_S(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			break;
		case USB_PORT_FEATURE_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_RESET:
			/*
			 *Execute port reset sequence
			 */
			SET_EHCI_PORTSC_PR(statport, 1);
			SET_EHCI_PORTSC_PE(statport, 0);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			/*USB 2.0 spec states 10ms for port reset */
			usb_delay_ms(10);
			statport =
			    EHCI_READ_OPRR(softc->ehci_regs,
					   (R_EHCI_PORTSCx(wIndex)));
			SET_EHCI_PORTSC_PR(statport, 0);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			for (;;) {
				usb_delay_ms(100);
				statport =
				    EHCI_READ_OPRR(softc->ehci_regs,
						   (R_EHCI_PORTSCx(wIndex)));
				if (!(GET_EHCI_PORTSC_PR(statport)))
					break;
			}
			break;
		case USB_PORT_FEATURE_POWER:
			statport =
			    EHCI_READ_OPRR(softc->ehci_regs,
					   (R_EHCI_PORTSCx(wIndex)));
			/*Setting a 1 would clear the connect status change -
			 *we don't want to do that
			 */
			SET_EHCI_PORTSC_CSC(statport, 0);
			/*Enable power */
			SET_EHCI_PORTSC_PP(statport, 1);
			EHCI_WRITE_OPRR(softc->ehci_regs,
					(R_EHCI_PORTSCx(wIndex)), statport);
			break;
		case USB_PORT_FEATURE_LOW_SPEED:
			break;
		case USB_PORT_FEATURE_C_PORT_CONNECTION:
			break;
		case USB_PORT_FEATURE_C_PORT_ENABLE:
			break;
		case USB_PORT_FEATURE_C_PORT_SUSPEND:
			break;
		case USB_PORT_FEATURE_C_PORT_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_C_PORT_RESET:
			break;
		}
		break;

	case REQCODE(USB_REQUEST_SET_ADDRESS, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		softc->ehci_rh_newaddr =
		    wValue;
		break;

	case REQCODE(USB_REQUEST_GET_DESCRIPTOR, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		switch (wValue >> 8) {
		case USB_DEVICE_DESCRIPTOR_TYPE:
			memcpy(ptr, &ehci_root_devdsc,
			       sizeof(ehci_root_devdsc));
			ptr += sizeof(ehci_root_devdsc);
			break;
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			memcpy(ptr, &ehci_root_cfgdsc,
			       sizeof(ehci_root_cfgdsc));
			ptr += sizeof(ehci_root_cfgdsc);
			memcpy(ptr, &ehci_root_ifdsc, sizeof(ehci_root_ifdsc));
			ptr += sizeof(ehci_root_ifdsc);
			memcpy(ptr, &ehci_root_epdsc, sizeof(ehci_root_epdsc));
			ptr += sizeof(ehci_root_epdsc);
			break;
		case USB_STRING_DESCRIPTOR_TYPE:
			switch (wValue & 0xFF) {
			case 1:
				ptr += ehci_roothub_strdscr(ptr, "Generic");
				break;
			case 2:
				ptr += ehci_roothub_strdscr(ptr, "Root Hub");
				break;
			default:
				*ptr++ = 0;
				break;
			}
			break;
		default:
			res = -1;
			break;
		}
		break;

	case REQCODE(USB_REQUEST_GET_DESCRIPTOR, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		memcpy(&hdsc, &ehci_root_hubdsc,
		       sizeof(hdsc));
		hdsc.bNumberOfPorts = softc->ehci_ndp;
		tmpval = EHCI_READ_CAPR(softc->ehci_regs, R_EHCI_HCSPARAMS);
		if (GET_EHCI_HCSPRMS_PPC(tmpval) == 1)
			/*Independent per-port power control */
			tmpval |= USB_HUBCHAR_PWR_IND;
		else
			tmpval |= USB_HUBCHAR_PWR_GANGED;

		PUTUSBFIELD((&hdsc), wHubCharacteristics, tmpval);
		hdsc.bPowerOnToPowerGood = 5;
		hdsc.bDescriptorLength = USB_HUB_DESCR_SIZE + 1;
		hdsc.bRemoveAndPowerMask[0] = 0xE;
		memcpy(ptr, &hdsc, sizeof(hdsc));
		ptr += sizeof(hdsc);
#ifdef RH_DBG
		EHCI_DEBUG_LOG
		    ("\nEHCI_RH: ROOTHUB: wHubCharacteristicsLow %x wHubCharacteristicsHigh %x",
		     hdsc.wHubCharacteristicsHigh, hdsc.wHubCharacteristicsLow);
#endif
		break;

	case REQCODE(USB_REQUEST_SET_DESCRIPTOR, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		/*nothing */
		break;

	case REQCODE(USB_REQUEST_GET_CONFIGURATION, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    softc->ehci_rh_conf;
		break;

	case REQCODE(USB_REQUEST_SET_CONFIGURATION, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		softc->ehci_rh_conf =
		    wValue;
		break;

	case REQCODE(USB_REQUEST_GET_INTERFACE, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		*ptr++ =
		    0;
		break;

	case REQCODE(USB_REQUEST_SET_INTERFACE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		/*nothing */
		break;

	case REQCODE(USB_REQUEST_SYNC_FRAME, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		/*nothing */
		break;
	}

	softc->ehci_rh_ptr = softc->ehci_rh_buf;
	softc->ehci_rh_len = ptr - softc->ehci_rh_buf;

	return res;
}

static int ehci_roothub_xfer(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;
	int res;

	switch (ept->ep_num) {
		/*CONTROL ENDPOINT */
	case 0:
		/*Three types of transfers:  OUT (SETUP), IN (data), or STATUS.
		 *figure out which is which.
		 */
		if (ur->ur_flags & UR_FLAG_SETUP) {
			/*
			 *SETUP packet - this is an OUT request to the control
			 *pipe.  We emulate the hub request here.
			 */
			usb_device_request_t *req;

			req = (usb_device_request_t *) ur->ur_buffer;
			res = ehci_roothub_req(softc, req);
			if (res != 0) {
				SYS_LOG_ERR("Root hub request error\n");
			}

			ur->ur_xferred = ur->ur_length;
			ur->ur_status = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_STATUS_IN) {
			/*
			 *STATUS IN : it's sort of like a dummy IN request
			 *to acknowledge a SETUP packet that otherwise has no
			 *status.      Just complete the usbreq.
			 */
			if (softc->ehci_rh_newaddr != -1) {
				softc->ehci_rh_addr = softc->ehci_rh_newaddr;
				softc->ehci_rh_newaddr = -1;
			}

			ur->ur_status = 0;
			ur->ur_xferred = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_STATUS_OUT) {
			/*STATUS OUT : it's sort of like a dummy OUT request */
			ur->ur_status = 0;
			ur->ur_xferred = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_IN) {
			/*IN : return data from the root hub */
			int amtcopy;

			amtcopy = softc->ehci_rh_len;
			if (amtcopy > ur->ur_length)
				amtcopy = ur->ur_length;
			memcpy(ur->ur_buffer, softc->ehci_rh_ptr, amtcopy);

			softc->ehci_rh_ptr += amtcopy;
			softc->ehci_rh_len -= amtcopy;

			ur->ur_status = 0;
			ur->ur_xferred = amtcopy;

			usb_complete_request(ur, 0);
		} else {
			EHCI_DEBUG_LOG("Unknown root hub transfer type\n");
			return -1;
		}
		break;

		/*INTERRUPT ENDPOINT */
	case 1:		/*interrupt pipe */
		/*Nothing to do for EHCI (?) */
		break;
	}

	return EHCI_ERR_NOERROR;
}

static int ehci_xfer_ctrl(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;
	ehci_qTD_t **qTD = softc->qTD;
	u32_t regVal, qTDcnt = 0, i;
	u8_t bmRequestType =
	    ((usb_device_request_t *) ur->ur_buffer)->bmRequestType;

	EHCI_DEBUG_LOG("\n==CTRL=>>");
	softc->usb_Reqtype = UP_TYPE_CONTROL;

	ehci_qH_t *qH = ept->qH;

	if (ur->ur_length > EHCI_MAX_LEN_PER_QTD)
		return -1;

	softc->qReq = ur;

	/* --TODO: figure out what is going wrong here why do we need to
	 *re-init crashes after ehci_ept_setmps --
	 */
	ehci_init_qH(qH, ept);

	if (ur->ur_buffer != NULL) {
		qTD[qTDcnt] =
		    ehci_qtd_create(ur->ur_length, 0, 0, 3, EHCI_STATUS_ACTIVE,
				    UR_FLAG_SETUP, ur->ur_buffer, qTDcnt);
		qTDcnt++;
	}

	if (ur->ur_reslength != 0) {
		qTD[qTDcnt] =
		    ehci_qtd_create(ur->ur_reslength, 0, 0, 3,
				    EHCI_STATUS_ACTIVE,
				    (bmRequestType & USBREQ_DIR_IN) ? UR_FLAG_IN
				    : UR_FLAG_OUT, ur->ur_response, qTDcnt);
		qTDcnt++;
	}

	if (ur->ur_buffer != NULL) {
		qTD[qTDcnt] = ehci_qtd_create(0, 1, 0, 3, EHCI_STATUS_ACTIVE,
					      (bmRequestType & USBREQ_DIR_IN) ?
					      UR_FLAG_OUT : UR_FLAG_IN,
					      ur->ur_response, qTDcnt);
		qTDcnt++;
	}

	if (qTDcnt == 0)
		return -1;

	for (i = 0; i < qTDcnt - 1; i++) {
		qTD[i]->next_qTD_ptr = (u32_t) ((u32_t) (qTD[i + 1]) >> 5);
		qTD[i]->next_qTD_T = 0;
	}

	qTD[qTDcnt - 1]->ioc = 1;
	ept->qH->Current_qTD_ptr = 0;

	ept->qH->next_qTD_ptr = (u32_t) ((u32_t) qTD[0]) >> 5;
	ept->qH->next_qTD_ptrT = 0;

	/* -- assign the address of QH to hw -- */
	EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_ASYNCLISTADDR,
			(unsigned int)qH);

	/* -- clean status and then kick-off async schedule -- */
	regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_USBSTS, (regVal & 0x3F));

	/* -- Kick-off async schedule -- */
	ehci_enableSchedule(bus, ASYNC_SCHEDULE);

	return EHCI_ERR_NOERROR;

}

static int ehci_xfer_bulk(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;
	ehci_qTD_t **qTD = softc->qTD;
	u32_t regVal, qTDcnt = 0, i;
	ehci_qH_t *qH = ept->qH;

	EHCI_DEBUG_LOG("\n==BULK=>>");
	EHCI_DEBUG_LOG
	    ("usbaddr = 0x%x, eptnum = 0x%x, mps = 0x%x, flags = 0x%x ",
	     ept->ep_dev_addr, ept->ep_num, ept->ep_mps, ept->ep_flags);

	if (ur->ur_length > EHCI_MAX_LEN_PER_QTD)
		return -1;

	/* -- save the request type -- */

	softc->usb_Reqtype = UP_TYPE_BULK;
	softc->qReq = ur;

	if ((ur->ur_buffer != NULL) && (ept->ep_flags & (UP_TYPE_OUT))) {
		qTD[qTDcnt] =
		    (ehci_qTD_t *) ehci_qtd_create(ur->ur_length, 1, 0, 3,
						   EHCI_STATUS_ACTIVE,
						   UR_FLAG_OUT, ur->ur_buffer,
						   qTDcnt);
		qTDcnt++;
	}

	if ((ur->ur_buffer != NULL) && (ept->ep_flags & (UP_TYPE_IN))) {
		qTD[qTDcnt] =
		    (ehci_qTD_t *) ehci_qtd_create(ur->ur_length, 1, 0, 3,
						   EHCI_STATUS_ACTIVE,
						   UR_FLAG_IN, ur->ur_buffer,
						   qTDcnt);
		qTDcnt++;
	}

	if (qTDcnt == 0)
		return -1;

	/*keep same approach as ctrl */
	/*later make the bulk request as part of 1 transaction */
	for (i = 0; i < qTDcnt - 1; i++) {
		qTD[i]->next_qTD_ptr = (u32_t) ((u32_t) (qTD[i + 1]) >> 5);
		qTD[i]->next_qTD_T = 0;
	}

	/* -- setup interrupt on complete -- */
	softc->qTD[qTDcnt - 1]->ioc = 1;

	/* -- assign qTD to qH -- */
	qH->Current_qTD_ptr = 0;

	ept->qH->next_qTD_ptr = (u32_t) ((u32_t) qTD[0]) >> 5;
	ept->qH->next_qTD_ptrT = 0;

	/* -- clear left over status qH -- */
	qH->status = 0;

	/* -- assign the address of qH to ASYNCLISTADDR -- */
	EHCI_WRITE_OPRR(softc->ehci_regs, R_EHCI_ASYNCLISTADDR,
			(unsigned int)qH);

	/* -- clean status -- */
	regVal = EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_USBSTS);
	EHCI_SET_BITS(softc->ehci_regs, R_EHCI_USBSTS, (regVal & 0x3F));

	/* -- Kick-off async schedule -- */
	ehci_enableSchedule(bus, ASYNC_SCHEDULE);

	return EHCI_ERR_NOERROR;

}

static int ehci_xfer_isoc(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;

	if (ept->ep_flags & UP_TYPE_HIGHSPEED) {
		/* -- save the ur to the softc -- */
		softc->qReq = ur;
		softc->usb_Reqtype = UP_TYPE_ISOC;

		/*-- Highspeed ISO --*/
		if (ehci_queueiTDs(bus, uept, ur) ==
		    EHCI_ERR_DEVICENOTRESPONDING) {
			EHCI_DEBUG_LOG("ISOC Transaction failed *****");
			return EHCI_ERR_DEVICENOTRESPONDING;
		}
	} else {
		/*-- Full/Low Speed ISO not supported--*/
		return EHCI_ERR_DEVICENOTRESPONDING;
	}
	return EHCI_ERR_NOERROR;
}

static int ehci_xfer_intr(void)
{
	EHCI_DEBUG_LOG("\n==INTR=>> Not supported");
	return -1;
}

static int ehci_xfer(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;

	if (ur->ur_dev->ud_address == softc->ehci_rh_addr)
		/*only root hub requests */
		return ehci_roothub_xfer(bus, uept, ur);

	/*handlers for each. */
	if (ept->ep_flags & UP_TYPE_CONTROL)
		return ehci_xfer_ctrl(bus, uept, ur);
	if (ept->ep_flags & UP_TYPE_BULK)
		return ehci_xfer_bulk(bus, uept, ur);
	if (ept->ep_flags & UP_TYPE_ISOC)
		return ehci_xfer_isoc(bus, uept, ur);
	if (ept->ep_flags & UP_TYPE_INTR)
		return ehci_xfer_intr();

	EHCI_DEBUG_LOG("\n ERROR Unknown type 0x%x", ept->ep_flags);
	return -1;
}

static int ehci_cancel_xfer(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;
	ehci_qTD_t **qTD = softc->qTD;
	int i;

	ARG_UNUSED(ept);
	EHCI_DEBUG_LOG("ehci_cancel_xfer (ur=%p). qH=%p. qTD=%p\n", ur, ept->qH,
		       qTD);

	usb_complete_request(ur, EHCI_ERR_CANCELLED);

	for (i = 0; i < EHCI_MAX_NUM_TD_PER_XFER; i++)
		if (qTD[i])
			qTD[i] = NULL;

	return EHCI_ERR_NOERROR;
}

/*  Local functions */
static ehci_qTD_p ehci_qtd_create(u32_t total_bytes, u32_t ioc, u32_t c_page,
				  u32_t cerr, u32_t active, u32_t flag,
				  u8_t *data_buffer_addr,
				  unsigned char qtd_num)
{
	ehci_qTD_p qTD;

	qTD = get_ehci_qtd(qtd_num);	/* 32B align */

	memset((void *)qTD, 0, sizeof(ehci_qTD_t));

	qTD->next_qTD_T = 1;	/*Termination default */
	qTD->altn_next_qTD_T = 1;	/*Termination default */
	qTD->next_qTD_ptr = 0;

	qTD->ioc = ioc;		/* 3 */
	qTD->total_bytes = total_bytes;
	qTD->status |= active ? EHCI_STATUS_ACTIVE : 0;
	qTD->cerr = cerr;
	qTD->dt = (flag & UR_FLAG_SETUP) ? 0 : 1;
	qTD->c_page = c_page;

	/*  set PID code according to ur_flag */
	if (flag & UR_FLAG_OUT)
		qTD->pid_code = EHCI_PID_CODE_OUT;
	else if (flag & UR_FLAG_IN)
		qTD->pid_code = EHCI_PID_CODE_IN;
	else if (flag & UR_FLAG_SETUP)
		qTD->pid_code = EHCI_PID_CODE_SETUP;

	qTD->buffer_pointer_0 = (u32_t) data_buffer_addr;

	return qTD;
}

static void ehci_init_qH(ehci_qH_p qH, ehci_endpoint_p ept)
{
	/*clear the QH */
	memset((void *)qH, 0, sizeof(ehci_qH_t));

	qH->T = 0;

	/*at qh initialization the qh references to itself. */
	qH->Typ = EHCI_LP_TYPE_QH;
	qH->QH_HLP = ((u32_t) qH) >> 5;

	/*---------- Word 1 ----------*/
	qH->device_address = ept->ep_dev_addr;
	qH->I = 0;
	qH->EndPt = ept->ep_num;
	qH->EPS = ((ept->ep_flags & UP_TYPE_LOWSPEED) ?
		   EHCI_ESP_LS : EHCI_ESP_HS);
	qH->dtc = 1;
	/*Software should only set a queue head's H-bit if the queue head is
	 *in the asynchronous schedule
	 */
	qH->H = ((ept->ep_flags & (UP_TYPE_CONTROL | UP_TYPE_BULK)) ? 1 : 0);
	qH->max_pkt_len = ept->ep_mps;
	qH->C = ((ept->ep_flags & UP_TYPE_LOWSPEED)
		 && (ept->ep_flags & UP_TYPE_CONTROL)) ? 1 : 0;
	qH->RL = 0;

	/*---------- Word 2 ----------*/
	qH->uFrameSMask = (ept->ep_flags & UP_TYPE_INTR);
	/*qH->Hub_Addr  TBD */
	qH->Mult = 1;

	/*---------- Word 3 ----------*/
	qH->Current_qTD_ptr = 0;

	/*---------- Word 4 ----------*/
	qH->next_qTD_ptrT = 1;

#ifdef EHCI_DBG
	EHCI_DEBUG_LOG("\n qH->QH_HLP: 0x%x === qH: 0x%x ===", qH->QH_HLP, qH);
#endif

}

static void ehci_init_fList(void)
{
	u32_t Idx;
	ehci_flist_p FLptr = NULL;

	for (Idx = 0; Idx < FRAME_LIST_SIZE; Idx++) {
		FLptr = get_ehci_fl(Idx);
		memset(FLptr, 0, sizeof(ehci_flist_t));
		FLptr->T = LINK_TERMINATE;
	}
}

static void ehci_init_ItdList(void)
{
	u32_t Idx;
	ehci_iTD_p iTDp = NULL;

	for (Idx = 0; Idx < EHCI_MAX_NUM_ITD; Idx++) {
		/* --all elements of iTD list terminated -- */
		iTDp = get_ehci_itd(Idx);
		memset(iTDp, 0, sizeof(ehci_iTD_t));
		iTDp->nextLinkPointer.T = LINK_TERMINATE;
	}
}

static inline void ehci_insert2flist(ehci_iTD_p iTDp, u8_t type, u32_t frameIdx)
{
	u32_t Idx;
	ehci_flist_p FLptr = NULL;

	/* -- diff frames before the current frame index location -- */
	if (frameIdx >= FRAME_LIST_SIZE) {
		/* -- Rollover -- */
		/*frameIdx = 1024 then idx = 0 */
		/*frameIdx = 1025 then idx = 1 and so on */
		Idx = frameIdx - FRAME_LIST_SIZE;
	} else {
		Idx = frameIdx;
	}

	FLptr = get_ehci_fl(Idx);
	FLptr->LP = (u32_t) (((u32_t) iTDp) & 0xFFFFFFE0UL);
	FLptr->Typ = type;
	FLptr->T = 0;

	EHCI_DEBUG_LOG("\n add2flist: Idx:%d FLptr:0x%x iTD:0x%x frameIdx:%d",
		       Idx, FLptr, iTDp, frameIdx);
}

static int ehci_queueiTDs(usbbus_t *bus, usb_ept_t *uept, usbreq_t *ur)
{
	u32_t frameIdx;
	u32_t maxDataPeriTD;
	u32_t total_urSize;
	u8_t *urBuffp;
	u32_t mps;
	u8_t mult;
	u32_t ITD_records = TRANSACTIONS_PER_ITD;/* 8 transactions per itd */
	u32_t itdcnt = 0;
	u32_t no_itdalloc = 0;
	u32_t iTDferLen = 0;
	u8_t mult_array[3] = { MULT_1, MULT_2, MULT_3 };
	ehci_iTD_p iTDp;

	ehci_softc_t *softc = (ehci_softc_t *) bus->ub_hwsoftc;
	ehci_endpoint_t *ept = (ehci_endpoint_t *) uept;

	EHCI_DEBUG_LOG("\nEHCI: %s , ur->ur_length: %d ", __func__,
		       ur->ur_length);

	mps = (ept->ep_mps) & 0x7FF;
	mult = mult_array[((ept->ep_mps >> 11) & MULT_3)];

	urBuffp = ur->ur_buffer;
	total_urSize = ur->ur_length;
	/*(M) maximum data that can be handled per Itd */
	maxDataPeriTD = mps * mult * ITD_records;

	EHCI_DEBUG_LOG
	    ("\nur->ur_length:%d	total_urSize %d (mps):%d (mult):%d",
	     ur->ur_length, total_urSize, mps, mult);
	EHCI_DEBUG_LOG("\n EHCI maxDataPeriTD:%d total_urSize:%d ",
		       maxDataPeriTD, total_urSize);

	if (total_urSize <= (maxDataPeriTD * EHCI_MAX_NUM_ITD)) {
		/*start mapping the buffer to itd */
		while (total_urSize) {
			/*-- Amount of data handled by iTD is minimum of
			 *the total transfer length or maxDataPeriTD
			 */
			iTDferLen = MIN(total_urSize, maxDataPeriTD);

			EHCI_DEBUG_LOG
			    ("\nEHCI: ---- urBuffp 0x%x total_urSize:%d iTDferLen:%d ",
			     urBuffp, total_urSize, iTDferLen);

			/*-- decrement the total_DataLen */
			total_urSize -= iTDferLen;

			/*-- Allocate and Setup the ITD --*/
			iTDp = get_ehci_itd(itdcnt);
			ehci_setupiTD(ept, mps, urBuffp, iTDferLen, iTDp,
				      ITD_records, mult,
				      (total_urSize ? 0 : 1));

			if (itdcnt < EHCI_MAX_NUM_ITD)
				itdcnt += 1;

			/*-- Increment the buffer location --*/
			urBuffp += iTDferLen;

			/*-- Increment the number of iTD --*/
			no_itdalloc += 1;
		}

		/*at this point all iTD have been setup */
		EHCI_DEBUG_LOG("\nITD SETUP COMPLETE: no_itdalloc:%d",
			       no_itdalloc);

		/*-- Get the Frame List current index */
		/*-- Lets be concerned with Frame number not microframes */
		frameIdx =
		    EHCI_READ_OPRR(softc->ehci_regs, R_EHCI_FRINDEX) >> 3;
		frameIdx &= 0x3FF;
		EHCI_DEBUG_LOG("\nframeIdx:%d", frameIdx);

		/*-- Link the ITDs to Frame List at position + framePeriod
		 * slots in the future--
		 *-- this approach has benefits that all itds will be sent
		 *as part of a --
		 *-- single USB transfer --
		 *-- Insert to '+mult'in the future --
		 */
		frameIdx = ((frameIdx + mult) % FRAME_LIST_SIZE);

		itdcnt = 0;
		for (itdcnt = 0; itdcnt < no_itdalloc; itdcnt++) {
			iTDp = get_ehci_itd(itdcnt);
			ehci_insert2flist(iTDp, EHCI_LP_TYPE_ITD,
					  (frameIdx + itdcnt));
		}

		EHCI_DEBUG_LOG("\nITD LINKING TO FLIST COMPLETE");

		/*-- Save the number of Itd allocated */
		softc->no_itdalloc = no_itdalloc;
	} else {
		/*-- Data length overflow --*/
		EHCI_DEBUG_LOG_CRIT
		    ("\nERROR: the transfer data length will overflow the itd handling list");
		EHCI_DEBUG_LOG_CRIT
		    ("\nERROR: total_urSize:%d maxDataPeriTD:%d MAX_NUM_ITD:%d",
		     total_urSize, maxDataPeriTD, EHCI_MAX_NUM_ITD);
		return EHCI_ERR_DEVICENOTRESPONDING;
	}

	EHCI_DEBUG_LOG("\nEHCI: ---- %s DONE", __func__);

	return EHCI_ERR_NOERROR;
}

void ehci_setupiTD(ehci_endpoint_t *ept, u32_t mps, u8_t *dataBuff,
		   u32_t xferLen, ehci_iTD_p iTDp, u8_t records,
		   u8_t mult, u8_t ioc)
{
	u32_t i = 0;
	u32_t perTransactionLen;
	u8_t pg_no = 0;

	/* -- clean the stale contents */
	memset(iTDp, 0, sizeof(ehci_iTD_t));

	EHCI_DEBUG_LOG
	    ("\nsetupiTD: Buff:0x%x xferLen:%d records:%d mps: %d mult: %d ioc: %d",
	     dataBuff, xferLen, records, mps, mult, ioc);
	for (i = 0; (xferLen > 0 && i < records); i += 1) {
		/*-- Take minimum of xferLen and (mps *mult) --*/
		perTransactionLen = MIN(xferLen, (mps * mult));

		xferLen -= perTransactionLen;

		/*-- Transaction 0-7 --*/
		iTDp->Transaction[i].Offset = ((u32_t) dataBuff & 0xFFF);
		iTDp->Transaction[i].PG = pg_no;
		iTDp->Transaction[i].ioc = (ioc && xferLen == 0) ? 1 : 0;
		iTDp->Transaction[i].Length = perTransactionLen;
		iTDp->Transaction[i].Active = 1;

		if (pg_no < 7)
			iTDp->Buffer_Pointer[pg_no] |=
			    ((u32_t) dataBuff & 0xFFFFF000);

		/*Next transaction will cross a 4k page boundary */
		/*do a page increment */
		if (((u32_t) (dataBuff + perTransactionLen) & 0xFFFFF000) -
		    ((u32_t) dataBuff & 0xFFFFF000))
			pg_no += 1;

		dataBuff += perTransactionLen;
	}

	/*these fields are set on ehci_init_ItdList once for all elements */
	/*-- I/O --*/
	iTDp->nextLinkPointer.T = LINK_TERMINATE;

	/*-- Endpoint number --*/
	iTDp->Buffer_Pointer[0] |= ((ept->ep_num << 8) & 0xF00);
	/*-- Device address --*/
	iTDp->Buffer_Pointer[0] |= (ept->ep_dev_addr & 0x7F);

	/*I/O flag */
	if (ept->ep_flags & UP_TYPE_IN)
		iTDp->Buffer_Pointer[1] |= 0x800;	/* 1<<11 */

	/*-- Maximum Packet Size --*/
	/*-- Sw should not set a value larger than 1024 40h */
	iTDp->Buffer_Pointer[1] |= MIN(0x400, mps);
	/*-- Mult --*/
	iTDp->Buffer_Pointer[2] |= (mult & 0x3);
}

void ehci_reg_dump(volatile ehci_reg_t *ehci_reg)
{
	printf("\n==================EHCI REG DUMP====================\n");
	printf("hccapbase	   (%p)=%x\n", &ehci_reg->hccapbase,
	       ehci_reg->hccapbase);
	printf("hcsparams	   (%p)=%08x\n", &ehci_reg->hcsparams,
	       ehci_reg->hcsparams);
	printf("hccparams	   (%p)=%08x\n", &ehci_reg->hccparams,
	       ehci_reg->hccparams);
	printf("usbcmd		  (%p)=%08x\n", &ehci_reg->usbcmd,
	       ehci_reg->usbcmd);
	printf("usbsts		  (%p)=%08x\n", &ehci_reg->usbsts,
	       ehci_reg->usbsts);
	printf("usbintr		 (%p)=%08x\n", &ehci_reg->usbintr,
	       ehci_reg->usbintr);
	printf("frindex		 (%p)=%08x\n", &ehci_reg->frindex,
	       ehci_reg->frindex);
	printf("ctrldssegment   (%p)=%08x\n", &ehci_reg->ctrldssegment,
	       ehci_reg->ctrldssegment);
	printf("periodiclistbase(%p)=%08x\n", &ehci_reg->periodiclistbase,
	       ehci_reg->periodiclistbase);
	printf("asynclistaddr   (%p)=%08x\n", &ehci_reg->asynclistaddr,
	       ehci_reg->asynclistaddr);
	printf("configflag	  (%p)=%08x\n", &ehci_reg->configflag,
	       ehci_reg->configflag);
	printf("portsc_0		(%p)=%08x\n", &ehci_reg->portsc_0,
	       ehci_reg->portsc_0);
	printf("portsc_1		(%p)=%08x\n", &ehci_reg->portsc_1,
	       ehci_reg->portsc_1);
	printf("portsc_2		(%p)=%08x\n", &ehci_reg->portsc_2,
	       ehci_reg->portsc_2);
	printf("insnreg00	   (%p)=%08x\n", &ehci_reg->insnreg00,
	       ehci_reg->insnreg00);
	printf("insnreg01	   (%p)=%08x\n", &ehci_reg->insnreg01,
	       ehci_reg->insnreg01);
	printf("insnreg02	   (%p)=%08x\n", &ehci_reg->insnreg02,
	       ehci_reg->insnreg02);
	printf("insnreg03	   (%p)=%08x\n", &ehci_reg->insnreg03,
	       ehci_reg->insnreg03);
	printf("insnreg04	   (%p)=%08x\n", &ehci_reg->insnreg04,
	       ehci_reg->insnreg04);
	printf("insnreg05	   (%p)=%08x\n", &ehci_reg->insnreg05,
	       ehci_reg->insnreg05);
	printf("insnreg06	   (%p)=%08x\n", &ehci_reg->insnreg06,
	       ehci_reg->insnreg06);
	printf("insnreg07	   (%p)=%08x\n", &ehci_reg->insnreg07,
	       ehci_reg->insnreg07);
	printf("insnreg08	   (%p)=%08x\n", &ehci_reg->insnreg08,
	       ehci_reg->insnreg08);
	printf("===================================================\n");
}
