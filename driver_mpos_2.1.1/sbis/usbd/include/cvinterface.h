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

#ifndef _CvInterface_H
#define _CvInterface_H

#include"cv/cvapi.h"
#include"cv/cvinternal.h"

void usbCvBulkOutSetUp(uint16_t wlen);
bool usbCvBulkIn(uint8_t *pBuffer, uint32_t len);
bool usbCvInterruptIn(uint8_t *pBuffer, uint32_t len);
cv_status usbCvBulkOut(cv_encap_command *cmd);
cv_status usbCvCommandComplete(uint8_t *pBuffer);
bool usbCvPollCmdRcvd(void);
bool usbCvPollCmdSent(void);
bool usbCvPollIntrSent(void);

/*
 * Queue up a BulkIn, send an IntrIn to let the host know it's available,
 * and wait up to 'ticks' ticks for the command to complete.  Returns
 * command completion status.
 */
bool usbCvSend(cv_usb_interrupt *interrupt, uint8_t *pBuffer, uint32_t len, int ticks);

/*
 * check for completion of last queued Bulk-In and Intr-In transfers
 * If ticks==0, return the current condition immediately
 * if ticks!=0, wait up to that many ticks for the condition to become true.
 */
bool usbCvBulkOutComplete(int ticks);
bool usbCvBulkInComplete(int ticks);
bool usbCvIntrInComplete(int ticks);

#endif
