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

#ifndef _USBDEVICE_H_
#define _USBDEVICE_H_

#include <platform.h>
#include <usb.h>
#include <usbdesc.h>

/*
 * Interface keys. These aren't the actual USB interface numbers,
 * those will be assigned dynamically based on which interfaces are
 * enabled.
 */
#define USBD_CV_INTERFACE		1000
#if 0 //ndef CV_USBBOOT
#define USBD_CT_INTERFACE		1001
#define USBD_CL_INTERFACE		1002
#define USBD_KEYBOARD_INTERFACE		1003
#endif /*!CV_USBBOOT*/

/*
 * Pick type and direction values that 0x00 is never valid
 * Note that endpoints can be bi-endian.
 */
#define MASK_TYPE		0x0f
#define CTRL_TYPE 		0x01
#define ISO_TYPE		0x02
#define BULK_TYPE       0x04
#define INTR_TYPE       0x08

#define DIR_MASK		0x30
#define IN_DIR			0x10
#define OUT_DIR 		0x20

#define is_in(ep)			((ep)->flags & IN_DIR)
#define is_out(ep)			((ep)->flags & OUT_DIR)
#define is_ctrl(ep)			(((ep)->flags & MASK_TYPE) == CTRL_TYPE)
#define is_bulk(ep)			(((ep)->flags & MASK_TYPE) == BULK_TYPE)
#define is_intr(ep)			(((ep)->flags & MASK_TYPE) == INTR_TYPE)
#define is_ctrlin(ep)		((ep)->flags == (CTRL_TYPE | IN_DIR))
#define is_ctrlout(ep)		((ep)->flags == (CTRL_TYPE | OUT_DIR))
#define	is_bulkin(ep)		((ep)->flags == (BULK_TYPE | IN_DIR))
#define is_bulkout(ep)		((ep)->flags == (BULK_TYPE | OUT_DIR))
#define is_intrin(ep)		((ep)->flags == (INTR_TYPE | IN_DIR))

/* Endpoint state */
#define is_idle(ep)			((ep)->state == USBD_EPS_IDLE)
#define is_avail(ep)		(is_idle(ep))
#define is_pending(ep)		((ep)->state == USBD_EPS_PENDING)
#define is_active(ep)		((ep)->state == USBD_EPS_ACTIVE)
#define ep_set_idle(ep)		((ep)->state = USBD_EPS_IDLE)
#define ep_set_pending(ep)	((ep)->state = USBD_EPS_PENDING)
#define ep_set_active(ep)	((ep)->state = USBD_EPS_ACTIVE)

/* Endpoint state */
typedef enum {
	USBD_EPS_IDLE,		/* nothing to transfer */
	USBD_EPS_PENDING,	/* waiting for IN token (IN endpoints only) */
	USBD_EPS_ACTIVE		/* transfer in progress */
} usbd_eps_t;

/* Endpoint events */
typedef enum {
	USBD_EPE_IN_TOKEN,
	USBD_EPE_BUF_NOTAVAIL,
	USBD_EPE_RXERROR
} usbd_event_t;

/* Synopsys UDC DMA Data Structures */
// Setup Data
typedef struct USBDMA_SETUP_DESC {
	uint32_t dwStatus;
	uint32_t dwReserved;
	uint32_t dwData[2];	// two dw words of setup data usb protocol: 8 bytes of setup data 
} tUSBDMA_SETUP_DESC;

// General Data
typedef struct USBDMA_DATA_DESC {
	uint32_t dwStatus;
	uint32_t dwReserved;
	uint8_t  *pBuffer;
	struct USBDMA_DATA_DESC *pNext;
} tUSBDMA_DATA_DESC;

typedef struct usb_dma_info_s {
	uint32_t		len;			/* number of bytes to transfer */
	uint32_t		transferred;	/* number of bytes transferred so far */
	uint32_t		desc_len;		/* number of bytes to xfer in current descriptor */
	uint8_t			*pBuffer;		/* in/out data begin pointer */
	uint8_t			*pXfr;			/* data updated pointer */
	tUSBDMA_DATA_DESC	dma;
} usb_dma_info_t;

typedef struct epstatus_s {
	uint8_t		number;		/* physical number */
	uint8_t		flags;		/* type & direction */
	uint8_t		interface;
	uint8_t		alternate;
	uint8_t		config;
	uint16_t	maxPktSize;
	uint32_t	intmask;
	usbd_eps_t	state;
	usb_dma_info_t	dma;
	void		(*eventHandler)(struct epstatus_s *);	/* ISR/event handler */
	void		(*appCallback)(struct epstatus_s *);	/* Application callback */
	void		(*eventCallback)(struct epstatus_s *, usbd_event_t);
} EPSTATUS;


#define USBD_ENDP_DIR_OUT			0x0
#define USBD_ENDP_DIR_IN			0x1
#define USBD_ENDP_CONTROL_TYPE		0x0
#define USBD_ENDP_ISOCHRONOUS_TYPE 	0x1
#define USBD_ENDP_BULK_TYPE 		0x2
#define USBD_ENDP_INTERRUPT_TYPE 	0x3
#define USBD_BULK_MAX_PKT_SIZE		512         //64
#define USBD_CNTL_MAX_PKT_SIZE		64
#define	USBD_INTR_MAX_PKT_SIZE		64
#define	USBD_ISO_MAX_PKT_SIZE		64
#define USBD_MAX_BUF_LEN		    512 	    //64  

#define	ENDP_NUMBER					10

#define NUM_INTERFACES				1
#define NUM_ENDPOINTS_PER_INTERFACE	3

typedef void (*usbd_class_request_handler_t)(SetupPacket_t *req, EPSTATUS *ctrl_in, EPSTATUS *ctrl_out);

typedef struct {
	EPSTATUS			*ep[NUM_ENDPOINTS_PER_INTERFACE];
	usbd_class_request_handler_t	class_request_handler;
	void				(*reset)(void);
	int					(*get_interface_descriptor)(char *buf, int num);
	const uint8_t		*descriptor;
	int					descriptor_len;
} usbd_interface_t;

/* Device flags */
#define USBD_FLAG_RESET				0x01		/* In reset */
#define USBD_FLAG_RESTART			0x02		/* detach/reattach from bus */
#define USBD_FLAG_INTERRUPT			0x04		/* in USBD interrupt service routine */
#define USBD_FLAG_SET_CONFIGURATION	0x08		/* set configuration seen */

typedef enum {
	USBD_STATE_SHUTDOWN=0,
	USBD_STATE_RESET=1,
	USBD_STATE_ENUM_DONE=2
} usbd_if_state_enum_t;

typedef struct {
	EPSTATUS		ep[ENDP_NUMBER];		/* endpoints */
	int				num_endpoints;			/* number of endpoints */
	usbd_interface_t	interface[NUM_INTERFACES];	/* interfaces */
	int				num_interfaces;
	EPSTATUS		*controlIn;
	EPSTATUS		*controlOut;
	uint32_t		flags;
	uint16_t		pid;			/* product ID (autodetected) */
	char			*cv_string;		/* CV interface string (autodetected) */
	int				cv_interface;
	int				ct_interface;
	int				cl_interface;
	int				kbd_interface;
	usbd_if_state_enum_t 	interface_state;
	int 			enum_count;	
} tDEVICE;

typedef struct {
	tDEVICE    			USBDevice;
	tUSBDMA_SETUP_DESC	SetupDesc;
	uint32_t			resetTime;	/* time of last USB bus reset */
	uint8_t __attribute__((aligned (4)))DescriptorBuffer[0x180];
	uint8_t __attribute__((aligned (4)))ctrl_buffer[64];
	uint8_t				tmp_buf[128];
} tUSB_MEM_BLOCK;


void usbDeviceInit(void);
void usbDeviceStart(void);
void usbConnectCV(void);
void usbConnectCCID(void);

void usbd_idle(void);

EPSTATUS *usbd_find_endpoint(int interface, int flags);

void usbdevice_isr(void);

void sendCtrlEpData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
void sendCtrlEndpointData(uint8_t *pBuffer, uint32_t len);

void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
void sendEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);

/*
 * check for completion of last queued transfer on the specified endpoint
 * If ticks==0, return the current condition immediately
 * if ticks!=0, wait up to that many ticks for the condition to become true.
 */
bool usbTransferComplete(EPSTATUS *ep, int ticks);

/* Cancel pending transfer on an endpoint */
void usbd_cancel(EPSTATUS *ep);

void usbd_hardware_reinit(tDEVICE *pDevice);

void usbd_client_reset(void);

void usbd_rxdma_enable(void);
#endif
