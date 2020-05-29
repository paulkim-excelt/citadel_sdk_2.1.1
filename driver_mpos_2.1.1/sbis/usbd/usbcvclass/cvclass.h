/*
 * $Copyright Broadcom Corporation$
 *
 */
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

#include <usbdesc.h>
#include <usbdevice.h>
#include <platform.h>
#include <cvinterface.h>

typedef struct 
{
	EPSTATUS *BulkInEp;
	EPSTATUS *BulkOutEp;
	EPSTATUS *IntrInEp;
} tCVINTERFACE;

typedef struct CVCLASS_MEM_BLOCK
{
	// cvclass
	tCVINTERFACE   CVClassInterface;
	uint32_t dwDataLen;
	uint32_t dwlength;
	uint16_t wRcvLength;
	uint16_t wflags;
	uint8_t  bnotified;
	uint8_t  bRcvStatus;
	uint8_t  bcmdRcvd;			/* Flag to indicate whether requested BulkOut cmd is recvd from host */
	uint8_t  bcvCmdInProgress;	/* Flag to indicate cv command in progress */
	void	*cmd;			/* current command */
} tCVCLASS_MEM_BLOCK;

void CvClassCallBackHandler(EPSTATUS *ep);
void InitialCvClass(uint8_t *pbuffer, uint16_t wlen);

#endif
