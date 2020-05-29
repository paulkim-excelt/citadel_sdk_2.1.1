/*
 * $Copyright Broadcom Corporation$
 *
 */
/*******************************************************************
 *
 *  Copyright 2004
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  Irvine CA 92619-7013
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************/

#include <platform.h>
#include "usbdevice.h"
#include "usbregisters.h"
#include "usb.h"
#include "usbdebug.h"
#include "usbdevice_internal.h"
#include "include/usbd_if.h"

#if 1 //def PHX_REQ_CV
#include "cv/cvapi.h"
#include "cv/cvinternal.h" /* for CV_SECURE_IO_COMMAND_BUF */
#endif
#include <string.h>

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

#define ENTER_CRITICAL()	do { } while (0)
#define EXIT_CRITICAL()		do { } while (0)

#if 1 //def CV_USBBOOT
#define cvGetDeltaTime(x) 0
#define get_ms_ticks(x)
#ifdef PHXII_BOOTSTRAP
#define fp_sensor_type()	FP_SENSOR_NONE
#else	/* sbi */
#define fp_sensor_type() 0
#endif
#endif

#define	MANUFACTURER_STRING		"Broadcom Corp"
#define PRODUCT_STRING			"58201"
#define	SERIAL_NUMBER_STRING	"0123456789ABCD"
#define CV_STRING				"Broadcom USH"

#define CV_STRING_IDX			4
//#define PID_FP_NONE				0x5892
//#define PID_FP_NONE				0x5820
//#define PID_FP_NONE				0x5820
#define PID_FP_NONE                           0x5841

#define CV_FP_NONE				"Broadcom USH"


extern void ccidcl_class_request(SetupPacket_t *req, EPSTATUS *ctrl_in, EPSTATUS *ctrl_out);
#define usbDevHw_REG_P  ((volatile usbDevHw_REG_t *)IPROC_BTROM_GET_usbDevHw_REG_P())

/*****************************************************************************
* See usbDevHw.h for API documentation
*****************************************************************************/
static void usbDevHw_DeviceBusConnect(void)
{
#ifdef DBG_SRINI
	printk("usbDevHw_DeviceBusConnect: enter\n");
#endif
	REG_MOD_AND(usbDevHw_REG_P->devCtrl, ~usbDevHw_REG_DEV_CTRL_DISCONNECT_ENABLE);
}

static void usbDevHw_DeviceBusDisconnect(void)
{
#ifdef DBG_SRINI
	printk("usbDevHw_DeviceBusDisconnect: enter\n");
#endif
	REG_MOD_OR(usbDevHw_REG_P->devCtrl, usbDevHw_REG_DEV_CTRL_DISCONNECT_ENABLE);
}


#define usbDevHw_FIFO_SIZE_UINT32(maxPktSize)   (((maxPktSize) + 3) / sizeof(uint32_t))

#define usbDevHw_FIFO_SIZE_UINT8(maxPktSize)	(usbDevHw_FIFO_SIZE_UINT32(maxPktSize) * sizeof(uint32_t))

/*****************************************************************************
* See usbDevHw.h for API documentation
*****************************************************************************/
static void usbDevHw_EndptIrqClear(unsigned num, unsigned dirn)
{
	if (dirn == usbDevHw_ENDPT_DIRN_OUT) {
		REG_WR(usbDevHw_REG_P->eptIntrStatus, (1 << num) << usbDevHw_REG_ENDPT_INTR_OUT_SHIFT);
	}
	else {
		REG_WR(usbDevHw_REG_P->eptIntrStatus, (1 << num) << usbDevHw_REG_ENDPT_INTR_IN_SHIFT);
	}
}

static void usbDevHw_EndptIrqDisable(unsigned num, unsigned dirn)
{
	if (dirn == usbDevHw_ENDPT_DIRN_OUT) {
#ifdef DBG_SRINI
		printk("udc: ep%d OUT disable\n", num);
#endif
		REG_MOD_OR(usbDevHw_REG_P->eptIntrMask, ((1 << num) << usbDevHw_REG_ENDPT_INTR_OUT_SHIFT));
	}
	else {
#ifdef DBG_SRINI
		printk("udc: ep%d IN disable\n", num);
#endif
		REG_MOD_OR(usbDevHw_REG_P->eptIntrMask, ((1 << num) << usbDevHw_REG_ENDPT_INTR_IN_SHIFT));
	}
}

/*****************************************************************************
* See usbDevHw.h for API documentation
*****************************************************************************/
static uint32_t usbDevHw_EndptStatusActive(unsigned num, unsigned dirn)
{
	if (dirn == usbDevHw_ENDPT_DIRN_OUT) {
		return(usbDevHw_REG_P->eptFifoOut[num].status);  /* End Point Status register */
	}
	return(usbDevHw_REG_P->eptFifoIn[num].status);
}

static void usbDevHw_EndptStatusClear(unsigned num, unsigned dirn, uint32_t mask)
{
	if (dirn == usbDevHw_ENDPT_DIRN_OUT) {
		REG_WR(usbDevHw_REG_P->eptFifoOut[num].status, mask);
	}
	else {
		REG_WR(usbDevHw_REG_P->eptFifoIn[num].status, mask);
	}
}

/*
Endpoint description from the HW engineers:

1 Configuration, 4 Interfaces each with a single Alternate setting
Default Control Endpoint (DCE) 0
8 IN physical endpoints, 4 OUT physical endpoints, total of 13 including DCE
4 Bidirectional Logical Endpoints (LE1-4)
Logical Endpoint 1 =  Endpoint Number 1, IN & OUT (BULK)
Logical Endpoint 2 =  Endpoint Number 2, IN & OUT (BULK)
Logical Endpoint 3 =  Endpoint Number 3, IN & OUT (BULK)
Logical Endpoint 4 =  Endpoint Number 4, IN & OUT (BULK)
Logical Endpoint 5 =  Endpoint Number 5, IN only  (INTR)
Logical Endpoint 6 =  Endpoint Number 6, IN only  (INTR)
Logical Endpoint 7 =  Endpoint Number 7, IN only  (INTR)
Logical Endpoint 8 =  Endpoint Number 8, IN only  (INTR)
Common DMA engine in DMA mode for transfers from Rx/Tx FIFOs
Single Receive (OUT) 64 byte FIFO
Individual buffers for Transmit (IN) endpoints allocated in a single RAM
*/

#define	USB_BYTE(x) \
	((x) & 0xff)
#define	USB_WORD(x) \
	((x) & 0xff), (((x) >> 8) & 0xff)
#define	USB_DWORD(x) \
	((x) & 0xff), (((x) >> 8) & 0xff), (((x) >> 16) & 0xff), (((x) >> 24) & 0xff)

extern void cv_usbd_reset(void);

const uint8_t usb_configuration_descriptor[] = {
	0x09,		/* bLength - 9 bytes */
	0x02,		/* bDescriptorType - CONFIGURATION */
	0x00,		/* wTotalLength (filled in later) */
	0x00,		/* wTotalLength - upper byte */
	0x01,		/* bNumInterfaces  (filled in later) */
#ifdef CONFIG_LYNX_USBD_SIMULATION
	0x01,		/* bConfgurationValue */ // for simulation (LYNX-405)
#else
	0x00,		/* bConfgurationValue */
#endif
	0x00,		/* iConfiguration */
	0x80 | 0x40,	/* bmAttributes: self-powered */
	0x32		/* MaxPower - 2 mA */
};

#ifdef USBD_CV_INTERFACE
const uint8_t usb_cv_interface_descriptor[] = {
	0x09,	/* bLength - 9 bytes */
	0x04,	/* bDescriptorType - INTERFACE */
	0x00,	/* interface Number - TBD */
	0x00,	/* Alternate Setting - 0 */
	0x03,	/* bNumEndpoints - 3 */
	0xFF,//0xFE,	/* interfaceClass - Class Code */
	0x00,	/* interfaceSubclass -SubClass Code */
	0x00,	/* interface Protocol - Bulk-Only Transport */
	CV_STRING_IDX,   /* iInterface- Index of string descriptor this interface */
#if 0
	0x10, 	// Length
	0x25, 	// Type=CV class
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
#endif
};
#endif

const tDEVICE_DESCRIPTOR usbd_device_descriptor = {
	0x12,			/* bLength - 18 bytes */
	DSCT_DEVICE,	/* bDescriptorType - Device */
	0x0200, 		/* bcdUsb - 2.0.0 */
	0x00,			/* bDeviceClass */
	0x00,			/* bDeviceSubClass */
	0x00,			/* bDeviceProtocol */
	0x40, 			/* bMaxPacketSize0 - 64 bytes */
	0x0a5c, 		/* Broadcom. */
	0,				/* Default device ID (filled in dynamically) */
	0x0000,			/* bcdDevice */
	0x01,			/* iManufacturer (string descriptor index) */
	0x02,			/* iProduct (string descriptor index) */
	0x03,			/* iSerialNumber (string descriptor index) */
	0x01			/* bNumConfigurations - 1 */
};

/*
 * String descriptors
 * Descriptor 0 (LangID) gets special handling, so don't include it here.
 * Store the string descriptors as strings, they've be converted into descriptor
 * format when requested.
 */
const char * const stringDescriptors[] = {
	MANUFACTURER_STRING,
	PRODUCT_STRING,
	SERIAL_NUMBER_STRING
#ifdef USBD_CV_INTERFACE
	,
	CV_STRING	/* CV Product String */
#endif
};

/* Also count the langId in the total */
const int stringDescriptorsNum = sizeof(stringDescriptors)/sizeof(stringDescriptors[0]) + 1;

/*
 * Setup the specificed string descriptor response value, returning
 * the length in bytes
 */
int usbdGetStringDescriptor(int idx, int req_len)
{
	int len = 2;
	char *ptr = (char *)DescriptorBuffer;

	if (idx == 0) {
		/* language string gets special handling */
		ptr[len++] = 0x09;
		ptr[len++] = 0x04;
	}
	else {
		const char *str = stringDescriptors[idx-1];

#ifdef USBD_CV_INTERFACE
		/* CV String is dynamically determined */
		if (idx == CV_STRING_IDX) {
			str = USBDevice.cv_string;
		}
#endif
		while (*str) {
			ptr[len++] = *str++;
			ptr[len++] = 0;
		}
	}
	DescriptorBuffer[0] = len;
	DescriptorBuffer[1] = 0x03;	/* string type */
	return (len < req_len) ? len : req_len;
}

/*
 * Build the device descriptor.
 */
int usbdGetDeviceDescriptor(uint8_t *buf, int rlen)
{
	int len = 0;
	uint8_t *ptr = buf;

	memcpy((void*)ptr, (void*)(&usbd_device_descriptor), sizeof(usbd_device_descriptor));
	((tDEVICE_DESCRIPTOR *)ptr)->idProduct = USBDevice.pid;
	if (USBDevice.num_interfaces == 1) {
		((tDEVICE_DESCRIPTOR *)ptr)->bcdDevice = 0x0101;
	}
	ptr += sizeof(usbd_device_descriptor);

	len = ptr - buf;
	return (len < rlen) ? len : rlen;
}

/*
 * Helper routine to write an endpoint descriptor entry
 * returns the length written (7 bytes)
 */
int usbd_build_endpoint_descriptor(EPSTATUS *ep, uint8_t *buf)
{
	buf[0] = 0x07;
	buf[1] = DSCT_ENDPOINT;
	if (is_in(ep)) {
		buf[2] = 0x80 | ep->number;
	} else {
		buf[2] = ep->number;
	}
	if (is_intr(ep)) {
		buf[3] = EP_INTR;
	} else if (is_bulk(ep)) {
		buf[3] = EP_BULK;
	}
	buf[4] = ep->maxPktSize & 0xFF;
	buf[5] = (ep->maxPktSize >> 8 ) & 0xFF; //buf[5] = 0;

	if (is_intr(ep)) {
		buf[6] = 2; //2ms  or 1;		/* 1ms */
	} else {
		buf[6] = 0;
	}

	return 7;
}

/*
 * Build the configuration descriptor based on what interfaces are enabled.
 */
int usbdGetConfigurationDescriptor(uint8_t *buf, int rlen)
{
	int i, j;
	int len = 0;
	int interfaces = 0;
	uint8_t *ptr = buf;

	memcpy((void*)ptr, (void*)usb_configuration_descriptor, sizeof(usb_configuration_descriptor));
	ptr += sizeof(usb_configuration_descriptor);

	/* For each interface, write out the interface descriptor,
	 * followed by the associated endpoint descriptors.
	 */
	for (i = 0; i < USBDevice.num_interfaces; ++i) {
		usbd_interface_t *interface = &USBDevice.interface[i];

		if (interface->get_interface_descriptor) {
			ptr += (*interface->get_interface_descriptor)((char *)ptr, i);
		} else {
			memcpy((void*)ptr, (void*)interface->descriptor, interface->descriptor_len);
			ptr[2] = i;
			ptr += interface->descriptor_len;
		}
		interfaces++;

		for (j = 0; j < NUM_ENDPOINTS_PER_INTERFACE; ++j) {
			EPSTATUS *ep = interface->ep[j];

			if (ep) {
				len = usbd_build_endpoint_descriptor(ep, ptr);
				ptr += len;
			}
		}
	}

	/* Program the length */
	len = ptr - buf;

	buf[2] = len & 0xff;
	buf[3] = (len >> 8) & 0xff;

	/* program the number of interfaces */
	buf[4] = interfaces;
	return  (len < rlen) ? len : rlen;
}

EPSTATUS *usbd_find_endpoint(int key, int flags)
{
	int i;
	int interface;

	switch (key) {
#ifdef USBD_CV_INTERFACE
	case USBD_CV_INTERFACE:
		interface = USBDevice.cv_interface;
		break;
#endif
	default:
		interface = -1;
	}
	for (i = 0; i < ENDP_NUMBER; ++i) {
		EPSTATUS *ep = &USBDevice.ep[i];
		if ((ep->interface == interface) && (ep->flags == flags)) {
			return ep;
		}
	}
	return NULL;
}

void USBPullUp(void)
{
	usbDevHw_DeviceBusConnect();
}

void USBPullDown(void)
{
    usbDevHw_DeviceBusDisconnect();
}

void usbd_udc20_init(int regno, EPSTATUS *ep)
{
#ifdef CONFIG_LYNX_USBD_SIMULATION
	int config = 1;	// for simulation (LYNX-405)
#else
	int config = 0;
#endif
	int alternate = 0;
	uint32_t dwSet = ep->number
			| (config << USB_UDC_ENDP_CONFIG_SHIFT)
			| (ep->interface << USB_UDC_ENDP_INTERFACE_SHIFT)
			| (alternate << USB_UDC_ENDP_ALTERNATE_SHIFT)
			| (ep->maxPktSize << USB_UDC_ENDP_MAX_PKTSIZE_SHIFT);

	if (is_out(ep))
		dwSet |= USB_UDC_ENDP_DIR_OUT;
	if (is_in(ep))
		dwSet |= USB_UDC_ENDP_DIR_IN;
	switch (ep->flags & MASK_TYPE) {
	case CTRL_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_CONTROL;
		break;
	case BULK_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_BULK;
		break;
	case INTR_TYPE:
		dwSet |= USB_UDC_ENDP_TYPE_INTERRUPT;
		break;
	}
	*USB_UDC_ENDP_NE(regno) = dwSet;
}

void usbd_buffer_init(EPSTATUS *ep)
{
	if (is_in(ep)) {
		usb_dma_info_t *pDmaInfo = &ep->dma;
		tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;


		pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_DMA_DONE
							| USB_DMA_DESC_LAST;
		pDesc->pBuffer = pDmaInfo->pBuffer;
		pDesc->pNext = NULL;  // we are currenting handle one buffer trunk
		ep->intmask |= USB_DEVICE_ENDP_INT_IN(ep->number);
		ep->eventHandler = usbdEpInInterrupt;
		*USB_UDCAHB_IN_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;

		if (is_ctrl(ep)) {
			USBDevice.controlIn = ep;
		}
	}

	if (is_out(ep)) {
		usb_dma_info_t *pDmaInfo = &ep->dma;
		tUSBDMA_DATA_DESC *pDesc = &pDmaInfo->dma;

		pDesc->dwStatus = USB_DMA_DESC_BUFSTAT_DMA_DONE
						| USB_DMA_DESC_LAST;
		pDesc->pBuffer = pDmaInfo->pBuffer;
		pDesc->pNext = NULL;  // we are currenting handle one buffer trunk
		ep->intmask |= USB_DEVICE_ENDP_INT_OUT(ep->number);
		ep->eventHandler = usbdEpOutInterrupt;
		*USB_UDCAHB_OUT_ENDP_DESCRIPTOR(ep->number) = (uint32_t)pDesc;

		if (is_ctrl(ep)) {
			USB_DMA_DESC_SET_READY((tUSBDMA_SETUP_DESC *)&SetupDesc);
			*USB_UDCAHB_OUT_ENDP_SETUPBUF(ep->number) = (uint32_t)&SetupDesc;
			USBDevice.controlOut = ep;
		}
	}
}

void initUDCAHBGlobalRegister(void)
{
	uint32_t val = 0;
	/* device config register */ 
	val = USB_DEVICE_CONFIG_SPD_HS | /* USB_DEVICE_CONFIG_SPD_FS  |  USB_DEVICE_CONFIG_SPD_FS48  |*/
		  USB_DEVICE_CONFIG_REMOTE_WKUP |
		  USB_DEVICE_CONFIG_SET_DESC |
		  USB_DEVICE_CONFIG_PHY_8BIT |
		  USB_DEVICE_CONFIG_SELF_POWER |
		  usbDevHw_REG_DEV_CFG_CSR_PROGRAM_ENABLE;

	*USB_UDCAHB_DEVICE_CONFIG = val;
	/* device control register */
	*USB_UDCAHB_DEVICE_CONTROL = usbDevHw_REG_DEV_CTRL_LE_ENABLE
			| usbDevHw_REG_DEV_CTRL_DMA_MODE_ENABLE  
			| usbDevHw_REG_DEV_CTRL_DMA_IN_ENABLE  
			| usbDevHw_REG_DEV_CTRL_DMA_OUT_ENABLE 
			| usbDevHw_REG_DEV_CTRL_DMA_DESC_UPDATE_ENABLE
			/*
			 *^	| usbDevHw_REG_DEV_CTRL_DMA_DESC_UPDATE_ENABLE
			 *^	| usbDevHw_REG_DEV_CTRL_CSR_DONE
			 *^ | usbDevHw_REG_DEV_CTRL_OUT_NAK_ALL_ENABLE
			 */
			| usbDevHw_REG_DEV_CTRL_DMA_OUT_THRESHOLD_LEN_MASK
			| usbDevHw_REG_DEV_CTRL_DMA_BURST_LEN_MASK
			| usbDevHw_REG_DEV_CTRL_DMA_BURST_ENABLE
#if 0 //!usbDevHw_REG_MULTI_RX_FIFO
			| usbDevHw_REG_DEV_CTRL_OUT_FIFO_FLUSH_ENABLE
#endif
			;

	/* device interrupt mask register */

	*USB_UDCAHB_DEVICE_INTMASK = ~( USB_DEVICE_INT_RESET
									| USB_DEVICE_INT_IDLE
									| USB_DEVICE_INT_SUSPEND
									| USB_DEVICE_INT_SET_CONFIG
									| USB_DEVICE_INT_SOF
									| USB_DEVICE_INT_SET_INTERFACE);

	/* enable all endpoint interrupts */
	*USB_UDCAHB_ENDP_INTMASK = 0;
}

void usbd_pid_config(tDEVICE *pDevice)
{
	/* Determine which device to advertise as */
	switch (fp_sensor_type()) {
	default:
		USBDevice.pid = PID_FP_NONE;
		USBDevice.cv_string = CV_FP_NONE;
		return;
	}
}

void usbd_endpoint_config(EPSTATUS *ep, uint32_t number, uint32_t flags, uint32_t interface)
{
	volatile uint32_t * reg, val;
	uint32_t dwSet = 0;

	ep->number = number;
	ep->flags = flags;
	ep->interface = interface;
	ep_set_idle(ep);
	ep->maxPktSize = 0;
	ep->intmask = 0;
	ep->appCallback = NULL;
	ep->eventHandler = NULL;
	ep->dma.transferred = 0;
	ep->dma.len = USB_MAX_DMA_BUF_LEN;
	ep->dma.pBuffer = NULL;

	if (is_ctrl(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_CONTROL;		// 5:4 2'b00 control endpoint
		ep->maxPktSize = USBD_CNTL_MAX_PKT_SIZE;
	}
	if (is_bulk(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_BULK;		// 5:4 2'b01 Bulk endpoint
		ep->maxPktSize = USBD_BULK_MAX_PKT_SIZE;
	}
	if (is_intr(ep)) {
		dwSet |= USB_ENDP_CONTROL_TYPE_INTERRUPT;	// 5:4 2'b11 Interrupt endpoint
		ep->maxPktSize = USBD_INTR_MAX_PKT_SIZE;
	}

	/* maxpacket size is in words */
	if (is_in(ep)) {
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) = dwSet;
		*USB_UDCAHB_IN_ENDP_MAXPKTSIZE(ep->number) = ep->maxPktSize;
		*USB_UDCAHB_IN_ENDP_BUFSIZE(ep->number) = ep->maxPktSize;// >> 2;
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_FLUSH_TXFIFO;
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) &= ~USB_ENDP_CONTROL_FLUSH_TXFIFO;
#ifdef USBD_USE_IN_ENDPOINT_NAK
		/* Nothing to send yet, so set NAK */
		*USB_UDCAHB_IN_ENDP_CONTROL(ep->number) |= USB_ENDP_CONTROL_SET_NAK;
#endif
		
	}
	if (is_out(ep)) {
		*USB_UDCAHB_OUT_ENDP_CONTROL(ep->number) = dwSet;
		*USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(ep->number) = ep->maxPktSize;//((ep->maxPktSize >> 2) << 16) | ep->maxPktSize;
	}

	/* If it's a bulk or interrupt endpoint, store it in the corresponding
	 * interface data structure.
	 */
	if (!is_ctrl(ep)) {
		int i;

		for (i = 0; i < NUM_ENDPOINTS_PER_INTERFACE; ++i) {
			if (USBDevice.interface[interface].ep[i] == ep)
				break;
			if (!USBDevice.interface[interface].ep[i]) {
				USBDevice.interface[interface].ep[i] = ep;
				break;
			}
		}
	}
}

/* Initialization Functions Begin */
void usbd_config(tDEVICE *pDevice)
{
	int i;
	int bulk = 1;
	int intr = 5;
	int interface = 0;
	EPSTATUS *ep;
	usbd_interface_t *interfaces = &USBDevice.interface[0];
	tDEVICE *usbdp = &USBDevice;

	usbd_pid_config(pDevice);
	USBDevice.num_interfaces = 0;
	memset((void *)interfaces, 0, sizeof(usbd_interface_t) * NUM_INTERFACES);

	usbdp->cv_interface = -1;
	usbdp->ct_interface = -1;
	usbdp->cl_interface = -1;
	usbdp->kbd_interface = -1;

	for (i = 0; i < ENDP_NUMBER; ++i) {
		EPSTATUS *ep = &usbdp->ep[i];
		ep->interface = 0xff;
	}

	ep = &pDevice->ep[0];
	usbd_endpoint_config(ep++, 0, CTRL_TYPE | OUT_DIR, 0);
	usbd_endpoint_config(ep++, 0, CTRL_TYPE | IN_DIR, 0);
#ifdef USBD_CV_INTERFACE
	usbdp->cv_interface = interface;
	interfaces[interface].class_request_handler = NULL;
	interfaces[interface].reset = cv_usbd_reset;
	interfaces[interface].descriptor = usb_cv_interface_descriptor;
	interfaces[interface].descriptor_len = sizeof(usb_cv_interface_descriptor);

	usbd_endpoint_config(ep++, bulk,	BULK_TYPE | IN_DIR,	interface);
	usbd_endpoint_config(ep++, bulk++,	BULK_TYPE | OUT_DIR,interface);
	usbd_endpoint_config(ep++, intr++,	INTR_TYPE | IN_DIR,	interface++);
#endif
	usbdp->num_interfaces = interface;
	/* Calculate the number of endpoint entries used */
	usbdp->num_endpoints = ep - &pDevice->ep[0];
}

void usbd_hardware_init(tDEVICE *pDevice)
{
	int i;
	int regno = 0;
	EPSTATUS *ep;
	volatile uint32_t *reg, val;

	/* assign interfaces and endpoints */
	usbd_config(pDevice);
	initUDCAHBGlobalRegister();
	for (i = 0; i < 10; ++i) {
		/* Initialization usb device endpoint registers */
		usbDevHw_EndptIrqDisable(i, usbDevHw_ENDPT_DIRN_IN);
		usbDevHw_EndptIrqClear(i, usbDevHw_ENDPT_DIRN_IN);
		usbDevHw_EndptStatusClear(i, usbDevHw_ENDPT_DIRN_IN, usbDevHw_EndptStatusActive(i, usbDevHw_ENDPT_DIRN_IN));

		usbDevHw_EndptIrqDisable(i, usbDevHw_ENDPT_DIRN_OUT);
		usbDevHw_EndptIrqClear(i, usbDevHw_ENDPT_DIRN_OUT);
		usbDevHw_EndptStatusClear(i, usbDevHw_ENDPT_DIRN_OUT, usbDevHw_EndptStatusActive(i, usbDevHw_ENDPT_DIRN_OUT));
		//*USB_UDC_ENDP_NE(i) = 0;
	}


	for (i = 0; i < USBDevice.num_endpoints; i++) {
		ep = &pDevice->ep[i];
		if (ep->interface == 0xff)
			break;

		/* Now init the buffers */
		usbd_buffer_init(ep);

		/* Only 2nd CTRL entry gets a UDC20 register */
		if (is_ctrl(ep) && !i)
			continue;
		usbd_udc20_init(regno++, ep);
	}
	/* tell UDC-AHB the programming of UDC is done */
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_UDCCSR_DONE;
	/* ready to receive control messages */
	*USB_UDCAHB_DEVICE_CONTROL |= USB_DEVICE_CONTROL_RXDMA_EN;
}

void usbd_hardware_reinit(tDEVICE *pDevice)
{
	int i;

	ENTER_CRITICAL();
	*USB_UDCAHB_DEVICE_CONTROL &= ~USB_DEVICE_CONTROL_RXDMA_EN;

	// Figure out what type of device we are.
	usbd_pid_config(pDevice);

	// Re-set the buffers for non-control endpoints.
	for (i = 0; i < USBDevice.num_endpoints; i++) {
		EPSTATUS *ep = &pDevice->ep[i];

		usbd_endpoint_config(ep, ep->number, ep->flags, ep->interface);
		usbd_buffer_init(ep);
	}
	for (i = 0; i < USBDevice.num_endpoints; i++) {
		EPSTATUS *ep = &pDevice->ep[i];

		if (is_ctrl(ep) && is_out(ep)) {
			memset((void *)(pusb_mem_block->ctrl_buffer), 0x0, ep->maxPktSize);
			recvEndpointData(ep, pusb_mem_block->ctrl_buffer, ep->maxPktSize);
		}
	}
	EXIT_CRITICAL();

	USBDevice.interface_state = USBD_STATE_RESET;
}


void usbDeviceRegStructInit(void)
{
	int i = 0;

	for (i = 0; i < usbDevHw_REG_ENDPT_CNT; i++) {
		usbDevHw_REG_P->eptFifoIn[i].ctrl = (USB_DEVICE_BASE) + i*0x20 + 0x00;
		usbDevHw_REG_P->eptFifoIn[i].status = (USB_DEVICE_BASE) + i*0x20 + 0x04;
		usbDevHw_REG_P->eptFifoIn[i].size1 = (USB_DEVICE_BASE) + i*0x20 + 0x08;
		usbDevHw_REG_P->eptFifoIn[i].size2 = (USB_DEVICE_BASE) + i*0x20 + 0x0C;
		usbDevHw_REG_P->eptFifoIn[i].dataDescAddr = (USB_DEVICE_BASE) + i*0x20 + 0x14;

		usbDevHw_REG_P->eptFifoOut[i].ctrl = (USB_DEVICE_BASE) + i*0x20 + 0x200;
		usbDevHw_REG_P->eptFifoOut[i].status = (USB_DEVICE_BASE) + i*0x20 + 0x204;
		usbDevHw_REG_P->eptFifoOut[i].size1 = (USB_DEVICE_BASE) + i*0x20 + 0x208;
		usbDevHw_REG_P->eptFifoOut[i].size2 = (USB_DEVICE_BASE) + i*0x20 + 0x20C;
		usbDevHw_REG_P->eptFifoOut[i].setupBufAddr = (USB_DEVICE_BASE) + i*0x20 + 0x210;
		usbDevHw_REG_P->eptFifoOut[i].dataDescAddr = (USB_DEVICE_BASE) + i*0x20 + 0x214;
	}
	usbDevHw_REG_P->devCfg = USB_DEVICE_BASE + 0x400;
	usbDevHw_REG_P->devCtrl = USB_DEVICE_BASE + 0x404;
	usbDevHw_REG_P->devStatus =  USB_DEVICE_BASE + 0x408;
	usbDevHw_REG_P->devIntrStatus =  USB_DEVICE_BASE + 0x40C;
	usbDevHw_REG_P->devIntrMask =  USB_DEVICE_BASE + 0x410;
	usbDevHw_REG_P->eptIntrStatus =  USB_DEVICE_BASE + 0x414;
	usbDevHw_REG_P->eptIntrMask =  USB_DEVICE_BASE + 0x418;
	
	usbDevHw_REG_P->releaseNum =  USB_DEVICE_BASE + 0x420;
	usbDevHw_REG_P->LpmCtrlStatus =  USB_DEVICE_BASE + 0x424;
	
	for (i = 0; i < usbDevHw_REG_ENDPT_CNT; i++) {
		usbDevHw_REG_P->eptCfg[i] = ((USB_DEVICE_BASE + 0x500) + 0x04) + (i)* 0x04;
	}
}


/*
 * usbDeviceInit (void)
 *
 * Entry point for USB device initialization.
 * Set up descriptors, interfaces, internal data structures, etc
  */
void usbDeviceInit(void)
{
	memset((void *)pusb_mem_block, 0, sizeof(tUSB_MEM_BLOCK));
	get_ms_ticks(&pusb_mem_block->resetTime);
	usbDeviceRegStructInit();
	usbd_hardware_init(&USBDevice);
	USBDevice.interface_state = USBD_STATE_RESET;
}

/*
 * Notify USBD clients of a 'reset' event
 */
void usbd_client_reset(void)
{
	int i;

	DEBUG_LOG("[%s():%d],begin\n",__func__,__LINE__);
	
	for (i = 0; i < USBDevice.num_interfaces; ++i) {
		usbd_interface_t *interface = &USBDevice.interface[i];
		if (interface->reset)
			(*interface->reset)();
	}
	DEBUG_LOG("[%s():%d],end\n",__func__,__LINE__);
}

/*
 * Idle handler called from sched_cmd.c
 * used to handle USB bus reset and other events.
 */
void usbd_idle(void)
{
	int i;
	uint32_t flags;
	uint32_t mask;
	volatile uint32_t reg;

	ENTER_CRITICAL();
	flags = USBDevice.flags;
	USBDevice.flags = 0;
	EXIT_CRITICAL();



	reg = *USB_UDCAHB_DEVICE_INT;

#ifdef USBD_RXDMA_IFF_ALL_OUT_ENDPOINTS_READY
	usbd_rxdma_enable();
#endif
	usbd_check_endpoints();

	/* now handle the flags as needed */
	if (flags & USBD_FLAG_RESTART) {
		USBPullDown();

		/* Reset the state */
		USBDevice.interface_state= USBD_STATE_RESET;

		USBPullUp();
	}
	if (flags & USBD_FLAG_RESET) {
		get_ms_ticks(&pusb_mem_block->resetTime);


		/* Handle it in the isr ...too late to handle it here */
		/* Reset the state */
		USBDevice.interface_state= USBD_STATE_RESET;

		/* reset the USBD dma state and endpoints */
		usbd_hardware_reinit(&USBDevice);
		/* notify the client(s) */
		DEBUG_LOG("[%s():%d],call usbd_client_reset()\n",__func__,__LINE__);
		usbd_client_reset();
	}
}

void usbDeviceDumpEndpoints()
{
	usbd_dump();
}

/*
 * usbDeviceStart()
 *
 * Pull up to indicate to host that we're ready.
 */
void usbDeviceStart(void)
{
	DEBUG_LOG("[%s():%d],begin\n",__func__,__LINE__);
	usbd_client_reset();
	USBPullUp();
	DEBUG_LOG("[%s():%d],end\n",__func__,__LINE__);
}
