/*
 * $Copyright Broadcom Corporation$
 *
 */
#ifndef USBD_IF_H__
#define USBD_IF_H__

#include <platform.h>

void usbd_delay_ms(int ms);

void iproc_usb_device_early_init(void);
void iproc_usb_device_init(void);
int iproc_usb_device_bulk_read( uint8_t *data, uint32_t readLen, uint32_t *actualReadLen);
int iproc_usb_device_bulk_write( uint8_t *data, uint32_t writeLen);
int iproc_usb_device_interrupt_write( uint8_t *data, uint32_t writeLen);
void iproc_usb_device_shutdown(void);

#define IPROC_USB_MEM_SIZE	(256*1024)
extern u8_t usb_mem[IPROC_USB_MEM_SIZE];

#define USB_START_ADDRESS	&usb_mem[0]
#define USB_DATA_MEMORY_BASE     USB_START_ADDRESS  /*USB_START_ADDRESS 0x02028400*/

#define IPROC_USB_BUF_SIZE  (128*1024)
#define IPROC_USB_BUF_ADDR  (USB_DATA_MEMORY_BASE + IPROC_USB_MEM_SIZE - IPROC_USB_BUF_SIZE) /* open/secure mode data exchange buffer */

#define SBI_IMAGE_BASE     USB_START_ADDRESS /*USB_START_ADDRESS 0x02028400*/
#define IPROC_BTROM_GET_USB_SBI_IMAGE()      ( uint8_t *)((uint32_t)SBI_IMAGE_BASE)
#define IPROC_BTROM_GET_SBI_MAX_size   (IPROC_USB_MEM_SIZE - IPROC_USB_BUF_SIZE )

#define IPROC_BTROM_GET_USB_epInterfaceSendBuffer()      ( uint8_t *)((uint32_t)IPROC_USB_BUF_ADDR)
#define IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size   (256)

#define IPROC_BTROM_GET_USB_epInterfaceBuffer()      ( uint8_t *)((uint32_t)IPROC_USB_BUF_ADDR + 256)
#define IPROC_BTROM_GET_USB_epInterfaceBuffer_size    (4096)

#define IPROC_BTROM_GET_USB_cvclass_mem_block()      ( tCVCLASS_MEM_BLOCK *)((uint32_t)IPROC_USB_BUF_ADDR + 256 + 4096)
#define IPROC_BTROM_GET_USB_cvclass_mem_block_size   (sizeof(tCVCLASS_MEM_BLOCK))  /*  32 */

#define IPROC_BTROM_GET_USB_usb_device_ctx_t()      ( usb_device_ctx_t *)((uint32_t)IPROC_USB_BUF_ADDR + 256 + 4096 + 64)
#define IPROC_BTROM_GET_USB_usb_device_ctx_size    (sizeof(usb_device_ctx_t))  /* 28 */

#define IPROC_BTROM_GET_USB_usb_mem_block()      ( tUSB_MEM_BLOCK *)((uint32_t)(IPROC_USB_BUF_ADDR + (5*1024)))
#define IPROC_BTROM_GET_USB_usb_mem_block_size  (sizeof(tUSB_MEM_BLOCK))  /* 0x6a8  = 1704 */

#define IPROC_BTROM_GET_usbDevHw_REG_P()      ((volatile usbDevHw_REG_t *)((uint32_t)(IPROC_USB_BUF_ADDR + (7*1024))))
#define IPROC_BTROM_GET_USB_usbDevHw_REG_P_size  (sizeof(usbDevHw_REG_t))  /* 0xb68  =  2920 */

#endif
