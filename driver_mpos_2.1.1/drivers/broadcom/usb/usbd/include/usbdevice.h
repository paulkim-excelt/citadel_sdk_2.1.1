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
  *  intellectual property rights therein.	IF YOU HAVE NO AUTHORIZED LICENSE,
  *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
  *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
  *
  *  Except as expressly set forth in the Authorized License,
  *
  *  1. 	This program, including its structure, sequence and organization,
  *  constitutes the valuable trade secrets of Broadcom, and you shall use all
  *  reasonable efforts to protect the confidentiality thereof, and to use this
  *  information only in connection with your use of Broadcom integrated circuit
  *  products.
  *
  *  2. 	TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
  *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
  *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
  *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
  *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
  *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
  *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
  *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
  *
  *  3. 	TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
  *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
  *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
  *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
  *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
  *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
  *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
  *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
  ******************************************************************************/
 
 /* @file usbdevice.h
  *
  * This file contains the common definitions for different USB interfaces, 
  * packet sizes for different endpoints and structure definations for internal
  * USB Device structures.
  *
  *
  */

#ifndef _USBDEVICE_H_
#define _USBDEVICE_H_

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include "usbd_types.h"
#include "usb.h"
#include "usbdesc.h"

#include "usbdevice.h"

/*
 * Interface keys. These aren't the actual USB interface numbers,
 * those will be assigned dynamically based on which interfaces are
 * enabled.
 */
#define USBD_CV_INTERFACE		1000

#ifdef USH_BOOTROM  /*AAI */
/* Not supported in mpos builds */
//#define USBD_CT_INTERFACE		1001
//#define USBD_CL_INTERFACE		1002
//#define USBD_NFP_INTERFACE		1005
#endif /* USH_BOOTROM*/

#ifdef CONFIG_DATA_CACHE_SUPPORT
#define ALIGNMENT 64
#else
#define ALIGNMENT 4
#endif

#define USBD_INVALID_INTERFACE  0xFF

//#define USB1_1
#if defined (USB1_1)
/* for usb1.1 FS packet sizes */
#define     CV_BCDUSB  0x0110
#define     CV_USB_MAXPACKETSIZE        0x40

#define     USBD_CNTL_MAX_PKT_SIZE      64
#define     USBD_BULK_MAX_PKT_SIZE      64
#define     USBD_INTR_MAX_PKT_SIZE      16
#define     USBD_ISOC_MAX_PKT_SIZE      64	//not supported

#define     USBD_MAX_PKT_SIZE           64
#else
/* for usb2.0 HS packet sizes */
#define     CV_BCDUSB  0x0200
#define     CV_USB_MAXPACKETSIZE        0x200

#define     USBD_CNTL_MAX_PKT_SIZE      64
#define     USBD_BULK_MAX_PKT_SIZE      512
#define     USBD_INTR_MAX_PKT_SIZE      1024
#define     USBD_ISOC_MAX_PKT_SIZE      1024	//not supported

#define     USBD_MAX_PKT_SIZE           1024
#endif

/*
 * Pick type and direction values that 0x00 is never valid
 * Note that endpoints can be bi-endian.
 */
#define MASK_TYPE	0x0f
#define CTRL_TYPE 	0x01
#define ISO_TYPE	0x02
#define BULK_TYPE       0x04
#define INTR_TYPE       0x08

#define DIR_MASK	0x30
#define IN_DIR		0x10
#define OUT_DIR 	0x20

#define is_in(ep)	((ep)->flags & IN_DIR)
#define is_out(ep)	((ep)->flags & OUT_DIR)
#define is_ctrl(ep)	(((ep)->flags & MASK_TYPE) == CTRL_TYPE)
#define is_bulk(ep)	(((ep)->flags & MASK_TYPE) == BULK_TYPE)
#define is_intr(ep)	(((ep)->flags & MASK_TYPE) == INTR_TYPE)
#define is_ctrlin(ep)	((ep)->flags == (CTRL_TYPE | IN_DIR))
#define is_ctrlout(ep)	((ep)->flags == (CTRL_TYPE | OUT_DIR))
#define	is_bulkin(ep)	((ep)->flags == (BULK_TYPE | IN_DIR))
#define is_bulkout(ep)	((ep)->flags == (BULK_TYPE | OUT_DIR))
#define is_intrin(ep)	((ep)->flags == (INTR_TYPE | IN_DIR))

/* Endpoint state */
#define is_idle(ep)		((ep)->state == USBD_EPS_IDLE)
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
/* Setup Data */
typedef struct USBDMA_SETUP_DESC {
	uint32_t dwStatus;
	uint32_t dwReserved;
	uint32_t dwData[2];	// two dw words of setup data usb protocol: 8 bytes of setup data 
} __attribute__((aligned(ALIGNMENT))) tUSBDMA_SETUP_DESC;

/* General Data */
typedef struct USBDMA_DATA_DESC {
	uint32_t dwStatus;
	uint32_t dwReserved;
	uint8_t  *pBuffer;
	struct USBDMA_DATA_DESC *pNext;
} tUSBDMA_DATA_DESC;

/* Dma Information */
typedef struct usb_dma_info_s {
	uint32_t			len;		/* number of bytes to transfer */
	uint32_t			transferred;	/* number of bytes transferred so far */
	uint32_t			desc_len;	/* number of bytes to xfer in current descriptor */
	uint8_t				*pBuffer;	/* in/out data begin pointer */
	uint8_t				*pXfr;		/* data updated pointer */
	tUSBDMA_DATA_DESC	__aligned(ALIGNMENT) dma;
} usb_dma_info_t;

/* Endpoint Information */
typedef struct epstatus_s {
	uint8_t		number;		/* physical number */
	uint8_t		flags;		/* type & direction */
	uint8_t		interface;
	uint8_t		alternate;
	uint8_t		config;
	uint16_t	maxPktSize;
	uint32_t	intmask;
	usbd_eps_t	state;
	usb_dma_info_t	__aligned(ALIGNMENT) dma;
	void			(*eventHandler)(struct epstatus_s *);	/* ISR/event handler */
	void			(*appCallback)(struct epstatus_s *);	/* Application callback */
	void			(*eventCallback)(struct epstatus_s *, usbd_event_t);
} EPSTATUS;


/* Endpoint flags */
#define USBD_ENDP_DIR_OUT			0x0
#define USBD_ENDP_DIR_IN			0x1
#define USBD_ENDP_CONTROL_TYPE		0x0
#define USBD_ENDP_ISOCHRONOUS_TYPE 	0x1
#define USBD_ENDP_BULK_TYPE 		0x2
#define USBD_ENDP_INTERRUPT_TYPE 	0x3

#define USBD_MAX_BUF_LEN			64

#define	ENDP_NUMBER					21	// Lynx requirement: 20 programmable Endpoints and one Control Endpoint

#define NUM_INTERFACES				4
#define NUM_ENDPOINTS_PER_INTERFACE	3

typedef void (*usbd_class_request_handler_t)(SetupPacket_t *req, EPSTATUS *ctrl_in, EPSTATUS *ctrl_out);
typedef void (*usbd_handleVendRequest_handler_t)(SetupPacket_t *req);

/* USBD Interface structure */
typedef struct {
	EPSTATUS						*ep[NUM_ENDPOINTS_PER_INTERFACE];
	int 							type;
	usbd_class_request_handler_t	class_request_handler;
	void							(*reset)(void);
	int								(*get_interface_descriptor)(char *buf, int num);
	const uint8_t					*descriptor;
	int								descriptor_len;
} usbd_interface_t;

/* Device flags */
#define USBD_FLAG_RESET			0x01		/* In reset */
#define USBD_FLAG_RESTART		0x02		/* detach/reattach from bus */
#define USBD_FLAG_INTERRUPT		0x04		/* in USBD interrupt service routine */
#define USBD_FLAG_ADDRESS       0x10        /* in address state */
#define USBD_FLAG_SET_CONFIGURATION	0x08		/* set configuration seen */

typedef struct {							/* Selective suspend */
	bool idle_state;						/* Idle detected */
	bool suspend_state;						/* Suspended */
} tUSBD_SS;

/* USBD States */
typedef enum {
	USBD_STATE_SHUTDOWN=0,
	USBD_STATE_RESET=1,
	USBD_STATE_ENUM_DONE=2
} usbd_if_state_enum_t;

/* USBD Device Structure */
typedef struct {
	EPSTATUS						 ep[ENDP_NUMBER];		/* endpoints */
	int								 num_endpoints;			/* number of endpoints */
	usbd_interface_t				 interface[NUM_INTERFACES];	/* interfaces */
	uint32_t						 num_interfaces;

	EPSTATUS						 *controlIn;
	EPSTATUS						 *controlOut;

	uint32_t						 flags;
	uint16_t						 pid;/* product ID (autodetected) */
	char							 *cv_string;/* CV interface string (autodetected) */
	uint8_t							 bulk_epno;
	uint8_t							 intr_epno;
	usbd_handleVendRequest_handler_t vendor_req_handler;
	usbd_if_state_enum_t			 interface_state;
	int 							 enum_count;
	uint32_t						 delayed_CNAK_mask;
} tDEVICE;

/* USBD Memory Structure */
typedef struct {
	/*DCACHE size and alignment requirements */
	tDEVICE    			__aligned(ALIGNMENT) USBDevice;
	tUSBDMA_SETUP_DESC	__aligned(ALIGNMENT) SetupDesc;
	uint8_t 			__aligned(ALIGNMENT) DescriptorBuffer[USBD_BULK_MAX_PKT_SIZE];
	uint8_t 			__aligned(ALIGNMENT) ctrl_buffer[USBD_CNTL_MAX_PKT_SIZE];
	uint8_t				tmp_buf[128];
	uint32_t			resetTime;	/* time of last USB bus reset */
	tUSBD_SS			USBDSelectiveSuspend;
} tUSB_MEM_BLOCK;

extern tUSB_MEM_BLOCK usb_mem_block;



/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/

void usbDeviceInit(void);
void usbDeviceStart(void);
void usbd_hardware_init(tDEVICE *pDevice);
int usbd_reginterface(tDEVICE *pDevice,usbd_interface_t intf);
void usbd_config(tDEVICE *pDevice);

void usbdResume(void);
int usbdResumeWithTimeout(void);
void usbConnectCV(void);
void usbConnectCCID(void);

void usbd_idle(void);
void usbd_idle2(void);

EPSTATUS *usbd_find_endpoint(int interface, int flags);

//void usbdevice_isr(void);
void usbdevice_th(void *arg);

void sendCtrlEpData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
void sendCtrlEndpointData(uint8_t *pBuffer, uint32_t len);

void recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);
void sendEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len);

void usbdevice_isr(void *unused);

/*
 * check for completion of last queued transfer on the specified endpoint
 * If ticks==0, return the current condition immediately
 * if ticks!=0, wait up to that many ticks for the condition to become true.
 */
bool usbTransferComplete(EPSTATUS *ep, int ticks);

/* Cancel pending transfer on an endpoint */
void usbd_cancel(EPSTATUS *ep);
void usbd_dump(void);
void usbd_rxdma_enable(void);
void usbd_rxdma_disable(void);
void stallControlEP(EPSTATUS *ep);
void usbd_registervenhndlr(tDEVICE *pDevice,usbd_handleVendRequest_handler_t hndlr);

void usbd_register_isr(void);
void usbd_enable_irq(void);
void usbd_disable_irq(void);
void usbd_clear_allpend(void);

void usbd_isr(void *unused);
#endif
