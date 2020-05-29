/*
 * $Copyright Broadcom Corporation$
 *
 */
#ifndef _USBDESC_H
#define _USBDESC_H

#include <platform.h>

/* SetupPacket Request Type bit field */

#define SETUPKT_DIRECT   	 0x80   
#define SETUPKT_TYPEMSK  	 0x60   
#define SETUPKT_STDREQ    	 0x00
#define SETUPKT_CLASSREQ  	 0x20
#define SETUPKT_VENDREQ   	 0x40
#define SETUPKT_STDEVIN   	 0x80
#define SETUPKT_STDEVOUT 	 0x00

/* Setup Packet */

typedef struct {
	uint8_t bRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} tSETUP_PACKET;

/*********************************************************************************/

/* Standard Descriptor Types */

#define DSCT_DEVICE          	0x01
#define DSCT_CONFIG          	0x02
#define DSCT_STRING          	0x03
#define DSCT_INTERFACE        	0x04
#define DSCT_ENDPOINT    	0x05
#define DSCT_POWER    		0x08
#define DSCT_REPORT		0x22
#define DSCT_MASK               0xFF00

/* Endpoint Transfer Types */

#define EP_TSFRMASK		0x03
#define EP_CONTROL		0x00
#define EP_ISO			0x01
#define EP_BULK			0x02
#define EP_INTR			0x03


#pragma pack(1)
/* Descriptor Used in USB Enumeration */

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	bcdUsb;
    	uint8_t	bDeviceClass;
    	uint8_t	bDeviceSubClass;
    	uint8_t	bDeviceProtocol;
    	uint8_t	bMaxPacketSize0;
    	uint16_t	idVendor;
    	uint16_t	idProduct;
    	uint16_t	bcdDevice;
    	uint8_t	iManufacturer;
    	uint8_t	iProduct;
    	uint8_t	iSerialNumber;
    	uint8_t	bNumConfigurations;
} tDEVICE_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint8_t	bEndpointAddress;
    	uint8_t	bmAttributes;
    	uint16_t wMaxPacketSize;
    	uint8_t	bInterval;
} tENDPOINT_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint8_t	bInterfaceNumber;
    	uint8_t	bAlternateSetting;
    	uint8_t	bNumEndpoints;
    	uint8_t	bInterfaceClass;
    	uint8_t	bInterfaceSubClass;
    	uint8_t	bInterfaceProtocol;
    	uint8_t	iInterface;
} tINTERFACE_DESCRIPTOR;


typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	wTotalLength;
    	uint8_t	bNumInterface;
    	uint8_t	bConfigurationValue;
    	uint8_t	iConfiguration;
    	uint8_t	bmAttributes;
    	uint8_t	bMaxPower;
} tCONFIGURATION_DESCRIPTOR;

typedef struct {
    	uint8_t	bLength;
    	uint8_t	bDescriptorType;
    	uint16_t	bString[32]; /* fye: use fixed length */
} tSTRING_DESCRIPTOR;

#pragma pack()


/* ***************End of Descriptors ********************* */


typedef struct {
    	tENDPOINT_DESCRIPTOR	*descriptor;
} EndpointType;


typedef struct {
    	tINTERFACE_DESCRIPTOR	*descriptor;
    	tSTRING_DESCRIPTOR	*interface_string;
    	//tCCIDCLASS_DESCRIPTOR *ccidclassdescriptor;
    //	EndpointType    	  *endpoints[ENDPOINT_NUMBER+1]; /* fye: use fixed length */
} InterfaceAlternateSettingType;


typedef struct {
    	InterfaceAlternateSettingType	*active_setting;
    	InterfaceAlternateSettingType	*alternate_settings[2]; /* fye: use fixed length */
} InterfaceType;


typedef struct {
    tCONFIGURATION_DESCRIPTOR	*descriptor;
    tSTRING_DESCRIPTOR		*configuration_string;
    InterfaceType			*interfaces[2]; /* fye: use fixed length */
} ConfigurationType;


typedef struct {
    tDEVICE_DESCRIPTOR	*descriptor;
    tSTRING_DESCRIPTOR	*language_id_string;
    tSTRING_DESCRIPTOR    	*manufacturer_string;
    tSTRING_DESCRIPTOR    	*product_string;
    tSTRING_DESCRIPTOR	*serial_number_string;
    ConfigurationType		*active_configuration;
    ConfigurationType		*configurations[2]; //fye: use fixed length
} DeviceType;

#endif
