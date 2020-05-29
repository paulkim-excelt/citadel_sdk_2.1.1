/*
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

#ifndef  _NFPCLASS_H
#define  _NFPCLASS_H

#include "phx.h"
#include "usbdesc.h"
#include "usbdevice.h"

#define NFP_REQUEST_TIMEOUT 200

#define NFP_PACKET_HCI	0x01
#define NFP_PACKET_NCI	0x10

#define NFP_MAX_PACKET_SIZE 263 /* 4 (SPI header bytes including length word) +
                                   1 (transport prefix byte) +
                                   3 (NCI header including payload length byte) +
                                   255 (max payload size) */

#pragma pack(1)
typedef struct 
{
          uint8_t 	bLength;
          uint8_t 	bDescriptorType;
          uint8_t 	breserve[14];
          
} tNFPCLASS_DESCRIPTOR;

#pragma pack()

typedef struct  NFPClassDeviceConfigurationDescriptorCollection
{
	 tCONFIGURATION_DESCRIPTOR  ConfigurationDescriptor ;
	 tINTERFACE_DESCRIPTOR InterfaceDescriptor ;
 	 tNFPCLASS_DESCRIPTOR  NFPClassDescriptor;
	 tENDPOINT_DESCRIPTOR  BulkInEndPDescriptor ;
	 tENDPOINT_DESCRIPTOR  BulkOutEndPDescriptor;
	 tENDPOINT_DESCRIPTOR  IntrInEndPDescriptor;
} tNFPClASSDDEVICE_DESC_COLLECTION;

typedef struct 
{
	EPSTATUS *BulkInEp;
	EPSTATUS *BulkOutEp;
	EPSTATUS *IntrInEp;
} tNFPINTERFACE;

typedef struct _nfpPacketHeader
{
	uint8_t	bPacketType;
	uint8_t bPacketRes1;
	uint8_t bPacketRes2;
	uint8_t bPayloadLen;
} nfpPacketHeader;

void nfp_usbd_reset(void);
void NfpClassCallBackHandler(EPSTATUS *ep);
void InitialNfpClass(uint8_t *pbuffer, uint16_t wlen);

#endif
