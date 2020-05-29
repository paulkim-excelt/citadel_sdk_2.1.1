/*
 * $Copyright Broadcom Corporation$
 *
 */
#include "config.h"

typedef int iproc_usb_status_t;

typedef iproc_usb_status_t USB_INIT(void);
typedef iproc_usb_status_t USB_EARLY_INIT();
typedef iproc_usb_status_t USB_READ_DATA(uint32_t offset, uint8_t *dst, uint32_t dst_len);
typedef iproc_usb_status_t USB_SHUT_DOWN(uint32_t do_pulldown_and_wait);


extern iproc_usb_status_t usb_init(void);
extern iproc_usb_status_t usb_read_data(uint32_t offset, uint8_t *dst, uint32_t dst_len);
extern iproc_usb_status_t usb_shutdown(uint32_t do_pulldown_and_wait);
