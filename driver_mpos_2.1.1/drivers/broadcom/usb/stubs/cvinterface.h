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

#include"cvapi.h"
#include"cvinternal.h"

cv_status usbCvBulkIn(cv_encap_command *cmd, uint32_t len);
cv_status usbCvBulkOut(cv_encap_command *cmd);
cv_status usbCvCommandComplete(uint8_t *pBuffer);
cv_bool usbCvPollCmdRcvd(void);
cv_bool usbCvPollCmdSent(void);
cv_bool usbCvPollIntrSent(void);
cv_status usbCvInterruptIn(cv_usb_interrupt *interrupt);

/*
 * Queue up a BulkIn, send an IntrIn to let the host know it's available,
 * and wait up to 'ticks' ticks for the command to complete.  Returns
 * command completion status.
 */
cv_bool usbCvSend(cv_usb_interrupt *interrupt, uint8_t *pBuffer, uint32_t len, int ticks);

/*
 * check for completion of last queued Bulk-In and Intr-In transfers
 * If ticks==0, return the current condition immediately
 * if ticks!=0, wait up to that many ticks for the condition to become true.
 */
cv_bool usbCvBulkOutComplete(int ticks);
cv_bool usbCvBulkInComplete(int ticks);
cv_bool usbCvIntrInComplete(int ticks);

#endif
