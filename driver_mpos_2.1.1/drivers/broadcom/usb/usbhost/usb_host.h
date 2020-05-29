/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

#ifndef _USBH_OHCI_H_
#define _USBH_OHCI_H_

#include "usbchap9.h"

typedef volatile struct USB_GLOBAL_STRUCT_t {
	usb_device_descr_t usb_device_descr_g;
	usb_config_descr_t usb_config_descr_g;
	usb_interface_descr_t usb_interface_descr_g;
	usb_endpoint_descr_t usb_IN_endpoint_descr_g;
	usb_endpoint_descr_t usb_OUT_endpoint_descr_g;
	u32_t usb_speed;
	u8_t usb_pid_array[2];
	u8_t usb_IN_pid;
	u8_t usb_OUT_pid;
	u32_t usb_tag;
	u8_t usb_inquiry_data[36];
	u32_t usb_last_lba;
	u32_t usb_block_length;
	u32_t usb_starting_sector;
	u32_t usb_partition_size;
	u32_t dev_id;

} USB_GLOBAL_STRUCT_t;

typedef struct CommandBlockWrapper_t {
	unsigned char Signature[4];	/* contains 'USBC' */
	unsigned int Tag;	/* unique per command id */
	unsigned char DataTransferLength[4];	/* size of data */
	unsigned char Flags;	/* direction in bit 0 */
	unsigned char Lun;	/* LUN */
	unsigned char Length;	/* of of the CDB */
	unsigned char ScsiCommand[16];	/* SCSI command */
} CommandBlockWrapper_t;

typedef struct CommandStatusWrapper_t {
	unsigned char Signature[4];	/* contains 'USBC' */
	unsigned int Tag;	/* unique per command id */
	unsigned int Residue;	/* amount not transferred */
	unsigned char Status;	/* see below */
} CommandStatusWrapper_t;

typedef enum BoardType {
	LOW_CURRENT_BOARD = 0,
	HIGH_CURRENT_BOARD
} BoardType;

typedef enum UsbOtgMode {
	USB_OTG_HOST_MODE = 0,
	USB_OTG_DEVICE_MODE,
	USB_OTG_UNKNOWN_MODE
} UsbOtgMode;

/* ############### ATTENTION #######################             */
/* DONOT CHANGE BELOW. IF REQUIRED REFER .scat files also        */
/* #################################################             */

#define SECURE_CopyToSecureBuffer_INDEX 0
#define SECURE_err_handle_INDEX 1

#endif
