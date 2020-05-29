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

#ifndef _CFEGLUE_H_
#define _CFEGLUE_H_

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <logging/sys_log.h>
#ifdef CONFIG_DATA_CACHE_SUPPORT
#include <cortex_a/cache.h>
#endif
#include <socregs.h>

#define _CFE_
#define CPUCFG_COHERENT_DMA	1
#define CFE_HZ			1000

#undef _OHCI_DEBUG_
#undef _USBREQTRACE_
#define usb_noisy		0
#define ohcidebug		1
#define dump_desc		0

#define console_log printf
#define cfe_sleep _usb_delay_ms

#ifndef _physaddr_t_defined_
#define _physaddr_t_defined_
typedef u32_t physaddr_t;
#endif

extern int usbHostVer;
extern int usbHostOhci;

void _usb_delay_ms(unsigned int ms);
void cfe_bg_add(void (*func)(void *), void *arg);
void *usb_malloc(u32_t len, u32_t align);
void *usb_malloc_unaligned(u32_t len);
void usb_free(void *ptr);
void usb_free_unaligned(void *ptr);
void usbhost_reset(void);
void usbhost_isr(void *arg);

/*  Macros to access to registers */
/*or with current register value*/
#define EHCI_SET_BITS(ehci_regs, x, y) \
	(*((volatile u32_t *) (ehci_regs + (x))) |= (y))

/*compliment with current register value*/
#define EHCI_UNSET_BITS(ehci_regs, x, y) \
	(*((volatile u32_t *) (ehci_regs + (x))) &= (~(y)))

/*write capability register */
#define EHCI_WRITE_CAPR(ehci_regs, x, y) \
	(*((volatile u32_t *) (ehci_regs + (x))) = (y))
/*read capability register */
#define EHCI_READ_CAPR(ehci_regs, x) \
	(*((volatile u32_t *) (ehci_regs + (x))))

/*write operational register */
#define EHCI_WRITE_OPRR(ehci_regs, x, y) \
	(*((volatile u32_t *) (ehci_regs + (x))) = (y))

/*read operational register */
#define EHCI_READ_OPRR(ehci_regs, x) \
	(*((volatile u32_t *) (ehci_regs + (x))))

/*write operational register */
#define EHCI_WRITE_CSR(ehci_regs, x, y) \
	(*((volatile u32_t *) (ehci_regs + (x))) = (y))

/*read operational register */
#define EHCI_READ_CSR(ehci_regs, x) \
	(*((volatile u32_t *) (ehci_regs + (x))))

#define USBHOST_INTERRUPT_MODE
#ifndef printf
#include <misc/printk.h>
#define printf(fmt, ...) printk(fmt, ##__VA_ARGS__)
#endif

#ifdef USBHOST_INTERRUPT_MODE
void usbhost_register_isr(void);
void usbhost_unregister_isr(void);
void ohci_enter_critical(void);
void ohci_exit_critical(void);
#endif

#define is_cached_addr(addr)        ((u32_t)(addr) < 0x20000000)
#define addr2uncached(addr)         (is_cached_addr(addr) ? \
		(void *)((u32_t)(addr)+0x10000000) : (void *)(addr))
#endif /* _CFEGLUE_H_ */
