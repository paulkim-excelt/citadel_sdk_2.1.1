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

#include "cfeglue.h"
#include "usbhost_mem.h"
#include "ohci.h"
#include "ehci.h"
#include <board.h>
#include <irq.h>

/* map 'globals' into pUsbHost_mem_block members */
extern tUSBHOST_MEM_BLOCK usbhost_mem_block;
#define pUsbHost_mem_block ((volatile tUSBHOST_MEM_BLOCK *)&usbhost_mem_block)

#define writel(reg, val)	(*(volatile unsigned int *)(reg) = (val))
#define readl(reg)	(*(volatile unsigned int *)(reg))

volatile ohci_reg_t *ohci_reg = ((volatile ohci_reg_t *)(USB_OHCI_BASE));
volatile ehci_reg_t *ehci_reg = ((volatile ehci_reg_t *)(USB_EHCI_BASE));

int usbHostVer = -1;
int usbHostOhci = -1;

/* Local overwrite macros */
#undef EHCI_WRITECSR
#define EHCI_WRITECSR(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) = (y))
#undef EHCI_READCSR
#define EHCI_READCSR(ehci_regs, x) \
			(*((volatile u32_t *) (ehci_regs + (x))))
#undef EHCI_SET_BITS
#define EHCI_SET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) |= (y))
#undef EHCI_UNSET_BITS
#define EHCI_UNSET_BITS(ehci_regs, x, y) \
			(*((volatile u32_t *) (ehci_regs + (x))) &= (~(y)))

#undef OHCI_WRITESCSR
#define OHCI_WRITECSR(ohci_regs, x, y) \
			(*((volatile u32_t *) ((ohci_regs) + (x))) = (y))
#undef OHCI_READCSR
#define OHCI_READCSR(ohci_regs, x) \
			(*((volatile u32_t *) ((ohci_regs) + (x))))

extern usbdev_t *usbmass_dev;
extern usbdev_t *usbhid_dev;
extern int usbhost_initDataStruct(u32_t addr);

/* Access methods */
void usb_bg_call(void)
{
	volatile tUSBHOST_MEM_BLOCK *usbp = pUsbHost_mem_block;

	if (usbp->usb_bg_func)
		(*usbp->usb_bg_func)(usbp->usb_bg_arg);
}

void _usb_delay_ms(unsigned int millisec)
{
	do {
		usb_delay_ms(1);
		usb_bg_call();
	} while (millisec-- > 0);
}

void cfe_bg_add(void (*func)(void *), void *arg)
{
	volatile tUSBHOST_MEM_BLOCK *usbp = pUsbHost_mem_block;

	if (usbp->usb_bg_func)
		printf("cfe_bg_add(%p, %p) overwrite\n", func, arg);

	usbp->usb_bg_func = func;
	usbp->usb_bg_arg = arg;
}

#ifdef USBHOST_INTERRUPT_MODE
/*
 * These routines are used to protect critical sections from operations
 * that may happen in callback completion routines (which are called from
 * the interrupt handler.
 *
 * Only enable/disable the OHCI interrupts so as not to impact the
 * rest of the system.
 */

void ohci_enter_critical(void)
{
	OHCI_WRITECSR(USB_OHCI_BASE, R_OHCI_INTDISABLE, M_OHCI_INT_MIE);
	++pUsbHost_mem_block->critical_nesting;
}

void ohci_exit_critical(void)
{
	pUsbHost_mem_block->critical_nesting--;

	if (pUsbHost_mem_block->critical_nesting == 0)
		OHCI_WRITECSR(USB_OHCI_BASE, R_OHCI_INTENABLE, M_OHCI_INT_MIE);
}
#endif

void usbhost_configure(u32_t usbhostBaseAddr, u32_t ochiOffset)
{
	u32_t ehci_regs = usbhostBaseAddr;
	u32_t ohci_regs = usbhostBaseAddr + ochiOffset;
	u32_t fminterval = 0x2edf;
	u32_t regVal;

	/* EHCI configuration */
	EHCI_SET_BITS(ehci_regs, R_EHCI_USBCMD, V_EHCI_RUN_STOP(1));
	EHCI_SET_BITS(ehci_regs, R_EHCI_CONFIGFLAG, V_EHCI_CONFIGURE_FLAG(1));

	do {
		regVal = EHCI_READCSR(ehci_regs, R_EHCI_USBSTS);
	} while (regVal & M_EHCI_HC_HALTED);

	/* OHCI configuration (in case of FS and LS) */
	OHCI_WRITECSR(ohci_regs, R_OHCI_PERIODICSTART, (fminterval * 9) / 10);
	fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
	OHCI_WRITECSR(ohci_regs, R_OHCI_FMINTERVAL, fminterval);
	OHCI_WRITECSR(ohci_regs, R_OHCI_LSTHRESHOLD, 0x628);
}

void usbh_phyInit(void)
{
	u32_t regVal;

	/* Disable LDO */
	regVal = readl(CDRU_USBPHY_H_CTRL2);
	regVal |= (1 << CDRU_USBPHY_H_CTRL2__ldo_pwrdwnb);
	regVal |= (1 << CDRU_USBPHY_H_CTRL2__afeldocntlen);
	writel(CDRU_USBPHY_H_CTRL2, regVal);

	/* Disable Isolation */
	usb_delay_ms(5);
	regVal = readl(CRMU_USBPHY_H_CTRL);
	regVal &= ~(1 << CRMU_USBPHY_H_CTRL__phy_iso);
	writel(CRMU_USBPHY_H_CTRL, regVal);

	/* Enable IDDQ current */
	regVal = readl(CDRU_USBPHY_H_CTRL2);
	regVal &= ~(1 << CDRU_USBPHY_H_CTRL2__iddq);
	writel(CDRU_USBPHY_H_CTRL2, regVal);


	/*debug PLL*/
	/* UTMI CLK for 60 Mhz */
	usb_delay_ms(150);
	regVal = readl(CRMU_USBPHY_H_CTRL) & 0xfff00000;
	regVal |= 0xec4ec; //'d967916
	writel(CRMU_USBPHY_H_CTRL, regVal);

	regVal = readl(CDRU_USBPHY_H_CTRL1) & 0xfc000007;
	regVal |= (0x03 << CDRU_USBPHY_H_CTRL1__ka_R); // ka 'd3'
	regVal |= (0x03 << CDRU_USBPHY_H_CTRL1__ki_R); // ki 'd3'
	regVal |= (0x0a << CDRU_USBPHY_H_CTRL1__kp_R); // kp 'hA'
	regVal |= (0x24 << CDRU_USBPHY_H_CTRL1__ndiv_int_R ); //ndiv 'd36'
	regVal |= (0x01 << CDRU_USBPHY_H_CTRL1__pdiv_R ); //pdiv 'd1'
	writel(CDRU_USBPHY_H_CTRL1, regVal);

	/* Disable PHY reset */
	usb_delay_ms(1);
	regVal = readl(CDRU_USBPHY_H_CTRL1);
	regVal |= (1 << CDRU_USBPHY_H_CTRL1__resetb);
	writel(CDRU_USBPHY_H_CTRL1, regVal);

	/* Disable PLL reset */
	regVal = readl(CDRU_USBPHY_H_CTRL1);
	regVal |= (1 << CDRU_USBPHY_H_CTRL1__pll_resetb);
	writel(CDRU_USBPHY_H_CTRL1, regVal);

	/* Disable software reset */
#define CDRU_USBPHY_H_P1CTRL__soft_resetb (1) /* missed in lynx_reg.h */
	regVal = readl(CDRU_USBPHY_H_P1CTRL);
	regVal |= (1 << CDRU_USBPHY_H_P1CTRL__soft_resetb);
	writel(CDRU_USBPHY_H_P1CTRL, regVal);

#ifndef LYNX_EMU
	/* PLL locking loop */
	do {
		regVal = readl(CDRU_USBPHY_H_STATUS);
	} while ((regVal & (1 << CDRU_USBPHY_H_STATUS__pll_lock)) == 0);

	printf("USBH PHY PLL locked\n");

	/* Disable non-driving */
#define CDRU_USBPHY_H_P1CTRL__afe_non_driving (0)
	usb_delay_ms(1);
	regVal = readl(CDRU_USBPHY_H_P1CTRL);
	regVal &= ~(1 << CDRU_USBPHY_H_P1CTRL__afe_non_driving);
	writel(CDRU_USBPHY_H_P1CTRL, regVal);
#endif /* !LYNX_EMU */
}

/*
 * Create OHCI device, allocate memory resources, connect interrupts and
 * power up the port
 */
void usbhost_init(void)
{
	volatile tUSBHOST_MEM_BLOCK *usbp = pUsbHost_mem_block;

#ifdef USBHOST_INTERRUPT_MODE
	/* disable the host interrupt. */
	irq_disable(SPI_IRQ(USB2H_IRQ));
#endif

	/* reset EHCI host controller. */
	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_USBCMD,
		      V_EHCI_HOST_CONTROLLER_RESET(1));

	/* reset OHCI host controller. */
	OHCI_WRITECSR(USB_OHCI_BASE, R_OHCI_CONTROL,
		      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_RESET));

	/* disable EHCI host controller and port. */
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_PORTSC, V_EHCI_PORT_POWER(1));
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_USBCMD, V_EHCI_RUN_STOP(1));
	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_CONFIGFLAG,
			V_EHCI_CONFIGURE_FLAG(1));

	memset((void *)usbp, 0, sizeof(*usbp));
#ifdef CONFIG_USBMS
	usbmass_dev = NULL;
#endif
#ifdef CONFIG_USBHID
	usbhid_dev  = NULL;
#endif

	/* FIXME: need alternative enable in Lynx's CRMU?
	 * phx_dmu_block_enable((DMU_USH_PWR_ENABLE | DMU_USH_UCLK_PWR_ENABLE));
	 */

	/* configure any hcca parameters
	 * usbhost_configure(USB_EHCI_BASE, USB_HOST_OHCI_OFFSET_ADDRESS);
	 */

	/* Initialize internal data structures */
	usbhost_initDataStruct(USB_EHCI_BASE);

#ifndef USBHOST_INTERRUPT_MODE
	/* Let things settle before detecting devices */
#ifndef LYNX_EMU /* takes too long for emulator environment */
	usb_delay_ms(5);
#endif
	/*
	 * call the background function.
	 * This triggers the initial bus scan, enumeration,
	 * device discovery and driver assignment.
	 */
	usb_bg_call();
#endif
	usbp->initialized = 1;

#ifdef USBHOST_INTERRUPT_MODE
	irq_enable(SPI_IRQ(USB2H_IRQ));
#endif
	printf("usb host start\n");
}

void usbhost_reset(void)
{
	usbh_phyInit();
	usbhost_init();
}
