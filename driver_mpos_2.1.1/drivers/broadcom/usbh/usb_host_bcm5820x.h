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

/* @file usb_host_bcm5820x.h
 *
 * USB Host driver private header.
 *
 */

#ifndef __USB_HOST_BCM5820X_H__
#define __USB_HOST_BCM5820X_H__

#include <kernel.h>
#include <usbh/usb_host.h>

#define USBHOST_INTERRUPT_MODE 1

#define EHCI_WRITECSR(ehci_regs, x, y) \
			(*((volatile u32_t *) (USB_EHCI_BASE + (x))) = (y))
#define EHCI_READCSR(ehci_regs, x) \
			(*((volatile u32_t *) (USB_EHCI_BASE + (x))))
#define EHCI_SET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (USB_EHCI_BASE + (x))) |= (y))
#define EHCI_UNSET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (USB_EHCI_BASE + (x))) &= (~(y)))
#define OHCI_WRITECSR(ohci_regs, x, y) \
			(*((volatile u32_t *) (USB_OHCI_BASE + (x))) = (y))
#define OHCI_READCSR(ohci_regs, x) \
			(*((volatile u32_t *) (USB_OHCI_BASE + (x))))

enum usbhost {
	OHCI = 1,
	EHCI
};

#define USBH_IRQ_PRI	CONFIG_USB2H_DRIVER_INIT_PRIORITY

/* USB host Phy init register definitions */
#define CDRU_USBPHY_H_CTRL2			0x3001d0c0
#define CDRU_USBPHY_H_CTRL2__ldo_pwrdwnb	0
#define CDRU_USBPHY_H_CTRL2__afeldocntlen	1
#define CDRU_USBPHY_H_CTRL2__iddq		10

#define CRMU_USBPHY_H_CTRL			0x3001c028
#define CRMU_USBPHY_H_CTRL__phy_iso		20

#define CDRU_USBPHY_H_CTRL1			0x3001d0b4
#define CDRU_USBPHY_H_CTRL1__ka_R		16
#define CDRU_USBPHY_H_CTRL1__ki_R		19
#define CDRU_USBPHY_H_CTRL1__kp_R		22
#define CDRU_USBPHY_H_CTRL1__ndiv_int_R		6
#define CDRU_USBPHY_H_CTRL1__pdiv_R		3
#define CDRU_USBPHY_H_CTRL1__resetb		2
#define CDRU_USBPHY_H_CTRL1__pll_resetb		26

#define CDRU_USBPHY_H_P1CTRL			0x3001d0bc
#define CDRU_USBPHY_H_P1CTRL__afe_non_driving	0
#define CDRU_USBPHY_H_P1CTRL__soft_resetb	1
#define CDRU_USBPHY_H_STATUS			0x3001d0b8
#define CDRU_USBPHY_H_STATUS__pll_lock		0

void usb_delay_ms(int ms);
#endif /* __USB_HOST_BCM5820X_H__ */
