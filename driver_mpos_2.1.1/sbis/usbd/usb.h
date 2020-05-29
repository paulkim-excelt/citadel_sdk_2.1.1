/*
 * $Copyright Broadcom Corporation$
 *
 */
#ifndef _USB_H
#define _USB_H

#include <platform.h>

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

#define GET_STATUS			0x00
#define CLEAR_FEATURE		0x01
#define SET_FEATURE			0x03
#define SET_ADDRESS			0x05
#define GET_DESCRIPTOR		0x06
#define SET_DESCRIPTOR		0x07
#define GET_CONFIGURATION	0x08
#define SET_CONFIGURATION	0x09
#define GET_INTERFACE		0x0A
#define SET_INTERFACE		0x0B
#define SYNCH_FRAME			0x0C

/* HID Request Codes */

#define HID_GET_REPORT		0x01
#define HID_GET_IDLE		0x02
#define HID_GET_PROTOCOL	0x03
#define HID_SET_REPORT		0x09
#define HID_SET_IDLE		0x0a
#define HID_SET_PROTOCOL	0x0b

/* CCID Request Codes */

#define CCID_ABORT					0x01
#define CCID_GET_CLOCK_FREQUENCIES	0x02
#define CCID_GET_DATA_RATES			0x03

/*  global function  */
void handleStdRequest(SetupPacket_t *req);
void handleClassRequest(SetupPacket_t *req);
void handleVendRequest(SetupPacket_t *req);
void handleUsbRequest(SetupPacket_t *req);
unsigned int isUsbdEnumDone(void);
void UsbShutdown(void);
#endif

