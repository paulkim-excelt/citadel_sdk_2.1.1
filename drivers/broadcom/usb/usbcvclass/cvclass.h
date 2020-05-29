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

#ifndef  _CVCLASS_H
#define  _CVCLASS_H

#include "phx.h"
#include "usbdesc.h"
#include "usbdevice.h"

#pragma pack(1)
typedef struct 
{
          uint8_t 	bLength;
          uint8_t 	bDescriptorType;
          uint8_t 	breserve[14];
          
} tCVCLASS_DESCRIPTOR;

#pragma pack()

typedef struct  CVClassDeviceConfigurationDescriptorCollection
{
	 tCONFIGURATION_DESCRIPTOR  ConfigurationDescriptor ;
	 tINTERFACE_DESCRIPTOR InterfaceDescriptor ;
 	 tCVCLASS_DESCRIPTOR   CVClassDescriptor;
	 tENDPOINT_DESCRIPTOR  BulkInEndPDescriptor ;
	 tENDPOINT_DESCRIPTOR  BulkOutEndPDescriptor;
	 tENDPOINT_DESCRIPTOR  IntrInEndPDescriptor;
} tCVClASSDDEVICE_DESC_COLLECTION;

typedef struct 
{
	EPSTATUS *BulkInEp;
	EPSTATUS *BulkOutEp;
	EPSTATUS *IntrInEp;
} tCVINTERFACE;

void CvClassCallBackHandler(EPSTATUS *ep);
void InitialCvClass(uint8_t *pbuffer, uint16_t wlen);
void cv_usbd_reset(void);

void cvclass_dump_cmd(uint8_t *buf, int len);


#endif
