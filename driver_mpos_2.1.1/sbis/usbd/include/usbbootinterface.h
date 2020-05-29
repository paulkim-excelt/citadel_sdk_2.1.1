/*
 * $Copyright Broadcom Corporation$
 *
 */
/*******************************************************************
 *
 *  Copyright 2007
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
 *******************************************************************
 *
 *  Broadcom Phoenix Project
 *  File: usbbootinterface.h
 *  Description: the USB header file for USB Global context
 *
 *  $Copyright (C) 2005 Broadcom Corporation$
 *
 *  Version: $Id: $
 *
 *****************************************************************************/

#ifndef __USBBOOTINTERFACE_H
#define __USBBOOTINTERFACE_H

#include <platform.h>
#include "usbd_if.h"

/* function declarations */

iproc_status_t usb_init(void);
iproc_status_t usb_read_data(uint32_t offset, uint8_t *dst, uint32_t dst_len);
iproc_status_t usb_shutdown(uint32_t do_pulldown_and_wait);

void UsbbootCallBackHandler(void);

typedef struct usb_device_ctx_s {
    uint32_t usb_glb_error;
	uint8_t  *usb_sbi_ptr;
	uint32_t cur_read_offset;
	uint32_t total_rem_len;
    uint32_t sbi_read_offset;
    uint32_t to_read_offset;
	uint32_t to_read_len;
    //uint32_t ref_clk_val;    /* the value of the ref clock, e.g. 24, 27, 48 */
} usb_device_ctx_t;

#define usb_device_ctx ((usb_device_ctx_t *)IPROC_BTROM_GET_USB_usb_device_ctx_t())

#endif /* __USBBOOTINTERFACE_H */

