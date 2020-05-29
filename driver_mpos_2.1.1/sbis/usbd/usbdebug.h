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
 *******************************************************************/

#ifndef __USB_DEBUG_H__
#define __USB_DEBUG_H__
#include "usbdevice_internal.h"

extern void serial_puts(const char *s);
#define uart_print_string serial_puts


#undef trace
#define trace(...)		do { } while (0)

#undef trace_enum
#define trace_enum(...)		do { } while (0)

#endif /* __USB_DEBUG_H__ */
