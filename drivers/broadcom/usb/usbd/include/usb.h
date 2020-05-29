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

/* @file usb.c
 *
 * USB device Host request handers
 *
 *
 */


#ifndef _USB_H
#define _USB_H

#include <stdint.h>

#include "phx.h"

typedef struct {
    uint8_t  bDevState;
    uint8_t  bDevAddress;
    uint8_t  bDevConfiguration;
    uint8_t  bUSBInterface;
    uint8_t  bUSBAlternate;
} DeviceState_t;

/* SetupPacket Request Type bit field */

#define SETUPKT_DIRECTION	0x80   
#define SETUPKT_DEVICE_TO_HOST	0x80
#define SETUPKT_HOST_TO_DEVICE	0x00
#define SETUPKT_TYPEMSK		0x60   
#define SETUPKT_STDREQ		0x00
#define SETUPKT_CLASSREQ	0x20
#define SETUPKT_VENDREQ		0x40
#define SETUPKT_RECIPIENT	0x1f

/* Setup Packet */
#define SETUP_PKT_SIZE		0x32

typedef struct {
	uint8_t bRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} SetupPacket_t;

/*********************************************************************************/

/* Standard Request Codes */

#define GET_STATUS		0x00
#define CLEAR_FEATURE		0x01
#define SET_FEATURE		0x03
#define SET_ADDRESS		0x05
#define GET_DESCRIPTOR		0x06
#define SET_DESCRIPTOR		0x07
#define GET_CONFIGURATION	0x08
#define SET_CONFIGURATION	0x09
#define GET_INTERFACE		0x0A
#define SET_INTERFACE		0x0B
#define SYNCH_FRAME		0x0C

/* HID Request Codes */

#define HID_GET_REPORT		0x01
#define HID_GET_IDLE		0x02
#define HID_GET_PROTOCOL	0x03
#define HID_SET_REPORT		0x09
#define HID_SET_IDLE		0x0a
#define HID_SET_PROTOCOL	0x0b

/* CCID Request Codes */

#define CCID_ABORT			0x01
#define CCID_GET_CLOCK_FREQUENCIES	0x02
#define CCID_GET_DATA_RATES		0x03

/* vendor request codes */
#define VEND_CANCEL_FP_CAPTURE   0x1
#define VEND_CANCEL_CV_CMD       0x2
#define VEND_REBOOT_CV_CMD       0xa


#define VEND_NFC_REQ_POWER_CONTROL	0x0
#define VEND_NFC_REQ_SLEEP_CONTROL	0x1
#define VEND_NFC_REQ_INT_CONTROL	0x2
#define VEND_NFC_CONTROL_OFF		0x0
#define VEND_NFC_CONTROL_ON			0x1
#define NFC_INTERFACE				0x3

/*  global function  */
void handleStdRequest(SetupPacket_t *req);
void handleClassRequest(SetupPacket_t *req);
void handleUsbRequest(SetupPacket_t *req);
void handleVendRequest(SetupPacket_t *req);

void UsbShutdown(void);

#endif

