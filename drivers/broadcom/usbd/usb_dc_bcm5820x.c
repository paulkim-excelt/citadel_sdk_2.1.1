/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

/*
 * @file usb_dc_bcm5820x.c
 * @brief Citadel USB device controller driver for Zephyr
 */

#include <arch/cpu.h>
#include <init.h>
#include <board.h>
#include <device.h>
#include <errno.h>
#include <kernel.h>
#include <misc/util.h>
#include <stdbool.h>
#include <dmu.h>
#include <broadcom/dma.h>
#include <string.h>

#define SYS_LOG_LEVEL	CONFIG_SYS_LOG_USBD_DRIVER_LEVEL
#include <logging/sys_log.h>

#include <usb/usb_dc.h>
#include "usb_regs.h"

/* Get endpoint index for ep address */
#define USBD_EP_IDX(EP_ADDR)		((EP_ADDR) & ~USB_EP_DIR_MASK)
/* Get end point direction from ep address */
#define USBD_EP_DIR_IN(EP_ADDR)		\
		(((EP_ADDR) & USB_EP_DIR_MASK) == USB_EP_DIR_IN)
#define USBD_EP_DIR_OUT(EP_ADDR)	\
		(((EP_ADDR) & USB_EP_DIR_MASK) == USB_EP_DIR_OUT)

/* EP Direction and Type checks */
#define is_in(EP)	USBD_EP_DIR_IN((EP)->cfg.ep_addr)
#define is_out(EP)	USBD_EP_DIR_OUT((EP)->cfg.ep_addr)
#define is_ctrl(EP)	(((EP)->cfg.ep_type) == USB_DC_EP_CONTROL)
#define is_isoc(EP)	(((EP)->cfg.ep_type) == USB_DC_EP_ISOCHRONOUS)
#define is_bulk(EP)	(((EP)->cfg.ep_type) == USB_DC_EP_BULK)
#define is_intr(EP)	(((EP)->cfg.ep_type) == USB_DC_EP_INTERRUPT)
#define is_ctrlin(EP)	(is_ctrl(EP) && is_in(EP))
#define is_ctrlout(EP)	(is_ctrl(EP) && is_out(EP))
#define	is_isocin(EP)	(is_isoc(EP) && is_in(EP))
#define is_isocout(EP)	(is_isoc(EP) && is_out(EP))
#define	is_bulkin(EP)	(is_bulk(EP) && is_in(EP))
#define is_bulkout(EP)	(is_bulk(EP) && is_out(EP))
#define is_intrin(EP)	(is_intr(EP) && is_in(EP))
#define is_introut(EP)	(is_intr(EP) && is_out(EP))

#if defined (USB_1_1)
/* Max packet sizes for USB1.1 FS */
#define USBD_CNTL_MAX_PKT_SIZE	64
#define USBD_BULK_MAX_PKT_SIZE	64
#define USBD_INTR_MAX_PKT_SIZE	16
#define USBD_ISOC_MAX_PKT_SIZE	64	/* not supported */

#define USBD_MAX_PKT_SIZE	64
#else
/* Max packet sizes for USB2.0 HS */
#define USBD_CNTL_MAX_PKT_SIZE	64
#define USBD_ISOC_MAX_PKT_SIZE	1024
#define USBD_BULK_MAX_PKT_SIZE	512
#define USBD_INTR_MAX_PKT_SIZE	1024

#define USBD_MAX_PKT_SIZE	1024
#endif

#define TOTAL_ENDPOINTS	(USBD_NUM_IN_ENDPOINTS + USBD_NUM_OUT_ENDPOINTS)

/* Reset times */
#define USBD_CORE_RESET_TIME		10000	/* 10 milli seconds */
#define USBD_FIFO_FLUSH_TIMEOUT		10000	/* 10 milli seconds */
#define USBD_PHY_SOFT_RESET_TIME	100	/* 100 micro seconds */
#define USBD_DMA_TIMEOUT		100	/* In iterations */

/* End point states */
#define USBD_EPS_IDLE		0	/* Nothing to transfer */
#define USBD_EPS_PENDING	1	/* Waiting for IN token (IN EPs only) */
#define USBD_EPS_ACTIVE		2	/* Transfer in progress */

/* Endpoint state check helper macros */
#define is_avail(EP)		(is_idle(EP))
#define is_idle(EP)		((EP)->state == USBD_EPS_IDLE)
#define is_pending(EP)		((EP)->state == USBD_EPS_PENDING)
#define is_active(EP)		((EP)->state == USBD_EPS_ACTIVE)
#define is_configured(EP)	\
	(USBD_EP_DIR_IN(EP) ? epc[USBD_EP_IDX(EP)].configured	\
		: epc[USBD_EP_IDX(EP) + USBD_NUM_IN_ENDPOINTS].configured)

/* Endpoint state set helper macros */
#define ep_set_idle(EP)		((EP)->state = USBD_EPS_IDLE)
#define ep_set_pending(EP)	((EP)->state = USBD_EPS_PENDING)
#define ep_set_active(EP)	((EP)->state = USBD_EPS_ACTIVE)

/* OUT end point context starts after the last IN endpoint context */
#define OUT_EPC_IDX(IDX)	(USBD_NUM_IN_ENDPOINTS + IDX)

#define EPC_IDX_TO_EP(EPC_IDX)	\
		(((EPC_IDX) < USBD_NUM_IN_ENDPOINTS) ?	\
		 ((EPC_IDX) | USB_EP_DIR_IN) :		\
		 (((EPC_IDX) - USBD_NUM_IN_ENDPOINTS) | USB_EP_DIR_OUT))

/* Default configuration parameters */
#define USBD_DEFAULT_CONFIG_INDEX	0x1
#define USBD_DEFAULT_INTERFACE_NUM	0x0
#define USBD_DEFAULT_ALTERNATE_SETTING	0x0

/* USB device endpoint information */
struct usbd_ep_context {
	bool configured;		/* Is endpoint configured? */
	bool ep_enabled;		/* Endpoint enabled? */
	struct usb_dc_ep_cfg_data cfg;	/* Endpoint configuration information */
	/* Endpoint data descriptor buffer */
	volatile struct usbd_dma_data_desc desc;
	usb_dc_ep_callback cb;		/* Endpoint callback function */

	union {
		struct {
			/* IN EP specific data - these fields are used to
			 * keep track of the tx DMAs when the tx packet length
			 * is larger than the max packet size and the DMAs need
			 * to be chained to complete the IN transaction
			 */
			/* EP state: idle, pending, active
			 *	idle - End point is available to transmit data
			 *	pending - The End point descriptor is configured
			 *		  with the size and buffer address and
			 *		  is waiting for the IN token to set the
			 *		  host ready flag and set poll demand
			 *	active - The tx DMA is in progress following the
			 *		 setting of the the host ready flag and
			 *		 is awaiting a tx dma complete event
			 */
			u8_t state;
			/* The address of the tx data that is ready to be sent*/
			u8_t *buffer;
			/* Number of IN DMAs left for the IN transaction to
			 * complete. Note that when if the tx length is equal
			 * to the max packet size, then a 0 length packet needs
			 * to be sent. So the number of in DMAs is always
			 * (len / mps) + 1
			 */
			u32_t num_in_dmas;
			/* This field indicates the DMA size of the last IN
			 * DMA and is always <= mps and can be 0.
			 */
			u32_t last_dma_size;
		} epi;

		/* OUT EP specific data */
		struct {
			/* Pointer to setup descriptor: Valid only for OUT EP */
			volatile struct usbd_dma_setup_desc *setup;
			/* Rx buffer in the data descriptor. This buffer
			 * is allocated when an endpoint is configured and
			 * released on usb_dc_reset() call. For a control
			 * endpoint, when a SETUP token is recevied, this buffer
			 * is updated with the 8 bytes from the SETUP packet.
			 * If the SETUP packet has an OUT data stage, then this
			 * buffer will hold the data from the OUT packet
			 */
			u8_t *buffer;
			/* Received packet length. This the size of the OUT
			 * packet received as indicated by the EP status
			 */
			u32_t rx_pkt_len;
			/* Data read by the application using usb_dc_ep_read()*/
			u8_t data_read;
			/* Flag to indicate if we are currently executing the
			 * EP setup callback. This is required by the ep_read()
			 * api to differentiate between and SETUP payload read
			 * and OUT data read.
			 */
			bool in_setup_callback;
		} epo;
	};
};

/* Endpoint state information */
static struct usbd_ep_context ep_context[TOTAL_ENDPOINTS];

/* Setup descriptor */
static struct usbd_dma_setup_desc setup_desc;

/* Uncached Pointer to ep_context[]
 * Since the ep_context[i].desc is written to and read by the USB device
 * controller, using the cached address for this structure will require
 * alignment/size restrictions to be in place. This will result in a memory
 * penalty of ((64 - 16)*32) = 1.5 KB. Using the uncached pointer will save
 * this memory and any cache maintenance operations on this descriptor memory
 */
static struct usbd_ep_context *epc;

#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
/* OUT endpoint interrupt mask fifo:
 * If CONFIG_SERVICE_EPOUT_INT_IN_THREAD is set, then the OUT ep interrupts
 * are not handled in the USBD ISR. This is done to prevent the user ep callback
 * from being called in the interrupt context. The user ep callback is expected
 * to call the usbd_dc_ep_read[|_wait]() api, which waits if on the OUT desc
 * status flag to change from DMA_BUSY to DMA_DONE. Allowing this to occur in
 * the interrupt context will increase interrupt latency for other interrupts
 * and context switches. To avoid this the out ep interrupt handling is
 * offloaded to the epout_int_service_thread()
 */
#define EPOUT_INT_FIFO_SIZE		16
struct epout_ist_fifo_entry {
	void *link_in_fifo; /* Used by Zephyr kernel */
	u32_t out_int_mask;
};

/* OUT EP interrupt service thread data */
#define EPOUT_IST_STACK_SIZE	2048
struct usbd_eout_int_service_thread_info {
	/* EPOUT IST thread */
	struct k_thread thread;
	/* EOUT IST stack */
	K_THREAD_STACK_DEFINE(stack, EPOUT_IST_STACK_SIZE);
	/* Fifo to send the epout int mask to the ep int service thread */
	struct k_fifo fifo;
	/* Fifo entries */
	struct epout_ist_fifo_entry msgs[EPOUT_INT_FIFO_SIZE];
	/* Fifo index */
	u8_t index;
};

static void epout_ist_fn(void *p1, void *p2, void *p3);
#endif /* CONFIG_SERVICE_EPOUT_INT_IN_THREAD */

/* USBD device data */
struct usbd_dev_data {
	/* Flag to indicate if the device is attached */
	bool attached;
	/* Endpoint status callback */
	usb_dc_status_callback cb;
	/* Uncached pointer to setup descriptor */
	struct usbd_dma_setup_desc *setup;
	/* Delayed clear nak mask - One bit per endpoint
	 * The bit for an ep is set if clear nak needs to be
	 * set, but the rx fifo is not empty, in which case,
	 * the cnak bit is set once the rx fifo becomes empty
	 */
	u32_t delayed_cnak;
	/* Running index count for UDC registers. This index determines the
	 * UDC entry for the next end point that will be configured. It is
	 * incremented after usb_dc_ep_configure().
	 */
	u8_t udc_reg_no;

	/* Current configuration index for the device */
	u8_t cur_config;

	/* Tx buffer */
	u8_t tx_buf[USBD_MAX_PKT_SIZE] __aligned(sizeof(u32_t));

#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
	/* End point interrupt service thread related data */
	usbd_eout_int_service_thread_info ept;
#endif
};

/* USBD driver private data */
static struct usbd_dev_data usbd_data;

/* USBD status callback includes a parameter compared to the
 * version in v1.9
 */
#ifdef USBD_CB_HAS_PARAM
#define USBD_STATUS_CB(UDATA, STAT)	UDATA.cb(STAT, NULL);
#else
#define USBD_STATUS_CB(UDATA, STAT)	UDATA.cb(STAT);
#endif

/* UDC-AHB register initialization */
static void udc_ahb_reg_init(void)
{
	/* Device config register */
	usbd->devcfg = USBD_CFG_LPM_EN |
#ifdef USB_1_1
		       USBD_CFG_SPD_FS |
#else
		       USBD_CFG_SPD_HS |
#endif
		       USBD_CFG_LPM_AUTO |
		       USBD_CFG_REMOTE_WKUP |
		       USBD_CFG_PHY_8BIT |
		       USBD_CFG_UDCCSR_PROGRAM;

	/* Device control register */
	usbd->devctrl = USBD_CTRL_LITTLE_ENDIAN |
			USBD_CTRL_MODE_DMA |
			USBD_CTRL_RXDMA_EN |
			USBD_CTRL_TXDMA_EN;

	/* Device interrupt mask register */
	usbd->devintmask = ~(USBD_INT_RESET |
			     USBD_INT_LPM_TKN |
			     USBD_INT_IDLE |
			     USBD_INT_SUSPEND |
			     USBD_INT_SET_CONFIG |
			     USBD_INT_ENUM_SPEED |
			     USBD_INT_SET_INTERFACE);

	/* Clear all pending endpoint interrupts */
	usbd->epint = 0xFFFFFFFF;
}

/* USB Hardware initialization */
static int usbd_hw_init(void)
{
	int i;

	/* Initialize UDC AHB registers */
	udc_ahb_reg_init();

	for (i = 0; i < USBD_NUM_UDC_EP_REGS; i++)
		usbd->udc_ep[i] = 0;

	/* Setup the data descriptor for all endpoints
	 * Use cached address as this is for the USBD controller to
	 * read from and the uncached map is only available to CPU
	 */
	for (i = 0; i < USBD_NUM_IN_ENDPOINTS; i++) {
		volatile struct usbd_dma_data_desc *desc = &ep_context[i].desc;
		struct usbd_dma_data_desc *uc_desc = UNCACHED_ADDR(desc);

		/* Initialize descriptor */
		memset((void *)uc_desc, 0, sizeof(*desc));
		uc_desc->status = USBD_DMADESC_BUFSTAT_HOST_BUSY;

		/* Program the cached address in descriptor as the USB
		 * controller does not understand the MMU mapped uncached
		 * address
		 */
		usbd->ep_in[i].desc = (u32_t)desc;
	}

	for (i = 0; i < USBD_NUM_OUT_ENDPOINTS; i++) {
		volatile struct usbd_dma_data_desc *desc =
				&ep_context[OUT_EPC_IDX(i)].desc;
		struct usbd_dma_data_desc *uc_desc = UNCACHED_ADDR(desc);

		/* Initialize descriptor */
		memset((void *)uc_desc, 0, sizeof(*desc));
		uc_desc->status = USBD_DMADESC_BUFSTAT_HOST_BUSY;

		/* Program the cached address in descriptor as the USB
		 * controller does not understand the MMU mapped uncached
		 * address
		 */
		usbd->ep_out[i].data_desc = (u32_t)desc;
	}

	return 0;
}

/* USBD hardware reset:
 * Toggles USBD core reset signal and USBD Phy soft reset signal and calls
 * usbd_hw_init()
 */
static int usbd_hw_reset(void)
{
	u32_t reg_val;
	struct device *cdru_dev;

	cdru_dev = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	if (cdru_dev == NULL)
		return -EIO;

	/* Assert USB core reset */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_SW_RESET_CTRL);
	reg_val &= ~BIT(CDRU_SW_RESET_CTRL__usbd_sw_reset_n);
	dmu_write(cdru_dev, CIT_CDRU_SW_RESET_CTRL, reg_val);
	k_busy_wait(USBD_CORE_RESET_TIME);

	/* De-assert core reset */
	reg_val |= BIT(CDRU_SW_RESET_CTRL__usbd_sw_reset_n);
	dmu_write(cdru_dev, CIT_CDRU_SW_RESET_CTRL, reg_val);

	/* FIXME: Perhaps the phy soft reset be this is not needed here? Or may
	 * be entire phy reset is needed?
	 */
	/* Assert Phy soft reset */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL);
	reg_val &= ~BIT(CDRU_USBPHY_D_P1CTRL__soft_resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL, reg_val);
	k_busy_wait(USBD_PHY_SOFT_RESET_TIME);

	/* De-assert Phy soft reset */
	reg_val |= BIT(CDRU_USBPHY_D_P1CTRL__soft_resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL, reg_val);

	return usbd_hw_init();
}

/* USBD enable PLL and wait for lock */
static int usbd_pll_enable(void)
{
	u32_t reg_val;
	struct device *cdru_dev;

	cdru_dev = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	if (cdru_dev == NULL)
		return -EIO;

	/* Disable PLL reset */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1);
	reg_val |= BIT(CDRU_USBPHY_D_CTRL1__pll_resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1, reg_val);

	/* PLL locking loop */
	do {
		reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_STATUS);
	} while ((reg_val & BIT(CDRU_USBPHY_D_STATUS__pll_lock)) == 0);

	SYS_LOG_DBG("USB device PHY PLL locked\n");

	return 0;
}

/* USBD disable PLL */
static int usbd_pll_disable(void)
{
	u32_t reg_val;
	struct device *cdru_dev;

	cdru_dev = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	if (cdru_dev == NULL)
		return -EIO;

	/* Disable PLL reset */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1);
	reg_val &= ~BIT(CDRU_USBPHY_D_CTRL1__pll_resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1, reg_val);

	return 0;
}

/* USBD phy initialization */
static int usbd_phy_init(void)
{
	u32_t reg_val;
	struct device *cdru_dev, *dru_dev;

	cdru_dev = device_get_binding(CONFIG_DMU_CDRU_DEV_NAME);
	if (cdru_dev == NULL)
		return -EIO;

	dru_dev = device_get_binding(CONFIG_DMU_DRU_DEV_NAME);
	if (dru_dev == NULL)
		return -EIO;

	/* Enable USB */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_SW_RESET_CTRL);
	reg_val |= BIT(CDRU_SW_RESET_CTRL__usbd_sw_reset_n);
	dmu_write(cdru_dev, CIT_CDRU_SW_RESET_CTRL, reg_val);

	/* Enable LDO */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL2);
	reg_val |= BIT(CDRU_USBPHY_D_CTRL2__ldo_pwrdwnb);
	reg_val |= BIT(CDRU_USBPHY_D_CTRL2__afeldocntlen);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL2, reg_val);

	/* Disable Isolation */
	k_busy_wait(500);
	reg_val = dmu_read(dru_dev, CIT_CRMU_USBPHY_D_CTRL);
	reg_val &= ~BIT(CRMU_USBPHY_D_CTRL__phy_iso);
	dmu_write(dru_dev, CIT_CRMU_USBPHY_D_CTRL, reg_val);

	/* Enable IDDQ current */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL2);
	reg_val &= ~BIT(CDRU_USBPHY_D_CTRL2__iddq);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL2, reg_val);

	/* Disable PHY reset */
	k_busy_wait(150);
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1);
	reg_val |= BIT(CDRU_USBPHY_D_CTRL1__resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1, reg_val);

	/* Suspend enable */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1);
	reg_val |= BIT(CDRU_USBPHY_D_CTRL1__pll_suspend_en);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1, reg_val);

	/* UTMI CLK for 60 Mhz */
	k_busy_wait(150);
	reg_val = dmu_read(dru_dev, CIT_CRMU_USBPHY_D_CTRL) & 0xfff00000;
	reg_val |= 0xec4ec; /* 'd967916 */
	dmu_write(dru_dev, CIT_CRMU_USBPHY_D_CTRL, reg_val);
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1) & 0xfc000007;
	reg_val |= (0x03 << CDRU_USBPHY_D_CTRL1__ka_R); /* ka 'd3 */
	reg_val |= (0x03 << CDRU_USBPHY_D_CTRL1__ki_R); /* ki 'd3 */
	reg_val |= (0x0a << CDRU_USBPHY_D_CTRL1__kp_R); /* kp 'hA */
	reg_val |= (0x24 << CDRU_USBPHY_D_CTRL1__ndiv_int_R); /* ndiv 'd36 */
	reg_val |= (0x01 << CDRU_USBPHY_D_CTRL1__pdiv_R); /* pdiv 'd1 */
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_CTRL1, reg_val);

	/* Disable software reset */
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL);
	reg_val |= BIT(CDRU_USBPHY_D_P1CTRL__soft_resetb);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL, reg_val);

	/* Enable PLL */
	usbd_pll_enable();

	/* Disable non-driving */
	k_busy_wait(10);
	reg_val = dmu_read(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL);
	reg_val &= ~BIT(CDRU_USBPHY_D_P1CTRL__afe_non_driving);
	dmu_write(cdru_dev, CIT_CDRU_USBPHY_D_P1CTRL, reg_val);

	/* Flush Device RxFIFO for stale contents*/
	/* Required for enumeration setup good change to have */
	usbd->devctrl |= USBD_CTRL_DEVICE_NAK;
	usbd->devctrl |= USBD_CTRL_SRX_FLUSH;
	usbd->devctrl &= ~USBD_CTRL_DEVICE_NAK;

	k_busy_wait(1);

	return 0;
}

/*
 * Dump USB endpoint info
 */
void usbd_dump_endpoint(u8_t ep)
{
	u32_t status;
	u32_t control;
	char *dir, *type;
	u8_t idx = USBD_EP_IDX(ep);
	struct usbd_ep_context *epc_entry = USBD_EP_DIR_IN(ep) ?
					    &epc[idx] : &epc[OUT_EPC_IDX(idx)];

	ARG_UNUSED(epc_entry);

	if (USBD_EP_DIR_IN(ep)) {
		control = usbd->ep_in[idx].control;
		status = usbd->ep_in[idx].status;
		dir = "IN";
	} else {
		control = usbd->ep_out[idx].control;
		status = usbd->ep_out[idx].status;
		dir = "OUT";
	}

	switch (control & USBD_EPCTRL_TYPE_MASK) {
	default:
	case USBD_EPCTRL_TYPE_CTRL:
		type = "CONTROL";
		break;
	case USBD_EPCTRL_TYPE_ISOC:
		type = "ISOCHRONOUS";
		break;
	case USBD_EPCTRL_TYPE_BULK:
		type = "BULK";
		break;
	case USBD_EPCTRL_TYPE_INTR:
		type = "INTERRUPT";
		break;
	}

	SYS_LOG_DBG("ep%d [%s] [%s]\n", idx, dir, type);

	SYS_LOG_DBG("\tcontrol   = 0x%08x", control);
	if (control & USBD_EPCTRL_STALL)
		SYS_LOG_DBG("\t\tSTALL");
	if (control & USBD_EPCTRL_FLUSHTXFIFO)
		SYS_LOG_DBG("\t\tFLUSH-TXFIFO");
	if (control & USBD_EPCTRL_SNOOP)
		SYS_LOG_DBG("\t\tSNOOP");
	if (control & USBD_EPCTRL_POLL_DEMAND)
		SYS_LOG_DBG("\t\tPOLL");
	if (control & USBD_EPCTRL_NAK)
		SYS_LOG_DBG("\t\tNAK");

	SYS_LOG_DBG("\tstatus   = 0x%08x", status);
	if (status & USBD_EPSTS_IN)
		SYS_LOG_DBG("\t\tIN");
	if (status & USBD_EPSTS_BUFNOTAVAIL)
		SYS_LOG_DBG("\t\tBNA");
	if (status & USBD_EPSTS_AHB_ERR)
		SYS_LOG_DBG("\t\tAHB");
	if (status & USBD_EPSTS_TX_DMA_DONE)
		SYS_LOG_DBG("\t\tTDC");

	if (USBD_EP_DIR_IN(ep)) {
		volatile struct usbd_ep_in_s *ep_in = &usbd->ep_in[idx];
		struct usbd_dma_data_desc *d =
			(struct usbd_dma_data_desc *)ep_in->desc;

		ARG_UNUSED(d);
		SYS_LOG_DBG("\tbufsize  = 0x%08x", ep_in->buff_size);
		SYS_LOG_DBG("\tmaxpkt   = 0x%08x", ep_in->max_pkt_size);
		SYS_LOG_DBG("\tdesc     = 0x%08x", ep_in->desc);
		SYS_LOG_DBG("\t\t0x%08x : 0x%08x : 0x%08x",
				d->status, (u32_t)d->buffer, (u32_t)d->next);
		if (usbd->epintmask & BIT(idx)) {
			SYS_LOG_DBG("\tinterrupt masked");
		} else {
			SYS_LOG_DBG("\tinterrupt enabled");
		}
		SYS_LOG_DBG("\tstate   = %d", epc_entry->epi.state);
	} else {
		volatile struct usbd_ep_out_s *ep_out = &usbd->ep_out[idx];
		struct usbd_dma_setup_desc *s =
			(struct usbd_dma_setup_desc *)ep_out->setup_desc;
		struct usbd_dma_data_desc *d =
			(struct usbd_dma_data_desc *)ep_out->data_desc;

		ARG_UNUSED(s);
		ARG_UNUSED(d);
		SYS_LOG_DBG("\tbufsize  = 0x%08x", ep_out->size >> 16);
		SYS_LOG_DBG("\tmaxpkt   = 0x%08x", ep_out->size & 0xFFFF);
		SYS_LOG_DBG("\tsetupbf  = 0x%08x", ep_out->setup_desc);
		SYS_LOG_DBG("\t\t0x%08x : 0x%08x : 0x%08x",
				s->status, s->data[0], s->data[1]);
		SYS_LOG_DBG("\tdesc     = 0x%08x", ep_out->data_desc);
		SYS_LOG_DBG("\t\t0x%08x : 0x%08x : 0x%08x",
				d->status, (u32_t)d->buffer, (u32_t)d->next);
		if (usbd->epintmask & BIT(OUT_EPC_IDX(idx))) {
			SYS_LOG_DBG("\tinterrupt masked");
		} else {
			SYS_LOG_DBG("\tinterrupt enabled");
		}
		SYS_LOG_DBG("\tRx Len   = %d", epc_entry->epo.rx_pkt_len);
		SYS_LOG_DBG("\tData Rd  = %d", epc_entry->epo.data_read);
	}
}

/* Clear NAK */
static void usbd_clear_nak(u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_OUT(ep) ? idx : OUT_EPC_IDX(idx);

	if (USBD_EP_DIR_IN(ep))
		usbd->ep_in[idx].control |= USBD_EPCTRL_CLEAR_NAK;
	else
		usbd->ep_out[idx].control |= USBD_EPCTRL_CLEAR_NAK;

	/* If RX fifo is not empty, set delayed clear nak flag */
	if (!(usbd->devstatus & USBD_STS_RXFIFO_EMPTY))
		usbd_data.delayed_cnak |= BIT(epc_idx);
}

/* Set NAK */
static void usbd_set_nak(u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	if (USBD_EP_DIR_IN(ep))
		usbd->ep_in[idx].control |= USBD_EPCTRL_SET_NAK;
	else
		usbd->ep_out[idx].control |= USBD_EPCTRL_SET_NAK;

	usbd_data.delayed_cnak &= ~BIT(epc_idx);
}

/* Enable Receive DMA */
static void usbd_rx_dma_enable(void)
{
	int i;
	u32_t key;

	key = irq_lock();

	if (usbd->devctrl & USBD_CTRL_RXDMA_EN)
		goto done;

	/* Check all OUT EPs are host ready before enabling rx dma */
	for (i = 0; i < USBD_NUM_OUT_ENDPOINTS; i++) {
		struct usbd_ep_context *epc_entry = &epc[OUT_EPC_IDX(i)];

		if (!is_ctrl(epc_entry)) {
			if ((epc_entry->desc.status & USBD_DMADESC_BUFSTAT_MASK)
			    != USBD_DMADESC_BUFSTAT_HOST_READY) {
				/* An OUT EP isn't active, don't enable RXDMA */
				goto done;
			} else {
				usbd_clear_nak((i | USB_EP_DIR_OUT));
			}
		}
	}
	usbd->devctrl |= USBD_CTRL_RXDMA_EN;
done:
	irq_unlock(key);
}

/*
 * @brief Prepare the DMA info for IN transaction
 *
 * @param ep Endpoint address
 * @param addr Address of the data to send
 * @param len Bytes to send
 * @param dma_all_bytes Flag to indicate of all bytes need to be sent or
 *			if the api can transmit part of the data
 * @return None
 */
static void prepare_tx_dma(
		const u8_t ep, const u8_t *addr,u32_t len, bool dma_all_bytes)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	struct usbd_ep_context *epc_entry = &epc[epc_idx];
	u32_t mps = epc_entry->cfg.ep_mps;

	epc_entry->epi.buffer = (u8_t *)addr;
	if (dma_all_bytes) {
		/* Even if it is a multiple of mps, a zero length
		 * packet needs to be sent
		 */
		epc_entry->epi.num_in_dmas = (len / mps) + 1;
		/* If 0, send a zero length packet */
		epc_entry->epi.last_dma_size = len % mps;
	} else {
		epc_entry->epi.num_in_dmas = 1;
		epc_entry->epi.last_dma_size = min(mps, len);
	}
}

/*
 * @brief Configures the next TX DMA, but does not start it
 *
 * @param ep Endpoint address
 * @return Returns the number of bytes configured to be sent
 */
static u32_t config_tx_dma(const u8_t ep)
{
	u32_t bytes_to_send, key;
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	struct usbd_ep_context *epc_entry = &epc[epc_idx];

	key = irq_lock();

	/* All packet except the last will be MAX size */
	bytes_to_send = epc_entry->epi.num_in_dmas == 1 ?
			epc_entry->epi.last_dma_size :
			epc_entry->cfg.ep_mps;

	/* Flush data */
#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_dcache_by_addr((u32_t)epc_entry->epi.buffer, bytes_to_send);
#endif

	/* Configure buffer and size for tx dma */
	memcpy(UNCACHED_ADDR(usbd_data.tx_buf), epc_entry->epi.buffer,
	       bytes_to_send);

	/* Setup descriptor */
	epc_entry->desc.buffer = usbd_data.tx_buf;

#define CONFIG_DELAYED_RESP_TO_IN_TOKEN
#ifdef CONFIG_DELAYED_RESP_TO_IN_TOKEN
	epc_entry->epi.state = USBD_EPS_PENDING; /* state = pending */
	/* Set desc status */
	epc_entry->desc.status = bytes_to_send | USBD_DMADESC_LAST |
				 USBD_DMADESC_BUFSTAT_HOST_BUSY;

	/* Clear any pending buf not avail interrupts */
	usbd->ep_in[idx].status = USBD_EPSTS_BUFNOTAVAIL;
#else
	epc_entry->epi.state = USBD_EPS_ACTIVE; /* state = active */
	/* Set desc status */
	epc_entry->desc.status = bytes_to_send | USBD_DMADESC_LAST |
				 USBD_DMADESC_BUFSTAT_HOST_READY;

	/* Set Poll Demand bit  */
	usbd->ep_in[idx].control |= USBD_EPCTRL_POLL_DEMAND;

	/* Clear any pending buf not avail interrupts */
	usbd->ep_in[idx].status = USBD_EPSTS_BUFNOTAVAIL;
#endif

	/* Update epi info for next DMA */
	epc_entry->epi.num_in_dmas--;
	epc_entry->epi.buffer += bytes_to_send;

	/* Clear NAK */
	usbd_clear_nak(ep);

	irq_unlock(key);

	return bytes_to_send;
}

/*
 * @brief Kick start the TX DMA
 *
 * @param ep Endpoint address
 * @return None
 */
static void start_tx_dma(const u8_t ep)
{
	u32_t status;
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	struct usbd_ep_context *epc_entry = &epc[epc_idx];

	/* Update state */
	epc_entry->epi.state = USBD_EPS_ACTIVE;

	/* Set host status */
	status = epc_entry->desc.status;
	status &= ~USBD_DMADESC_BUFSTAT_MASK;
	status |= USBD_DMADESC_BUFSTAT_HOST_READY | USBD_DMADESC_LAST;
	epc_entry->desc.status = status;

	/* FIXME: There appears to be a need for a delay between desc update
	 * and poll demand set. Adding a data memory barrier appears to do it
	 */
	__asm__ ("dmb");

	/* Set POLL_DEMAND */
	usbd->ep_in[idx].control |= USBD_EPCTRL_POLL_DEMAND;

	/* Clear any pending buf not avail interrupts */
	usbd->ep_in[idx].status = USBD_EPSTS_BUFNOTAVAIL;

	return;
}

/**
 * @brief  Process OUT Endpoint interruppt for a given end point
 * @details This function is called either from the usbd_isr() or from the
 *	    epout_ist_fn() to handle the OUT EP interrupt of type 'DATA_DESC'.
 *	    When and OUT token is received on the bus, this api calls the
 *	    ep callback function registered for this EP by the user. Once the
 *	    callback returns, the descriptor is marked host ready to receive
 *	    further packets. When this api is called, the expectation is that
 *	    the DMA engine is in the process of writing the rx fifo data to
 *	    system memory or had just finished doing that. The status quadlet
 *	    is expected to reflect this, if not a warning message is logged.
 */
static void process_epout_interrupt(u8_t ep_num)
{
	u32_t status, timeout;
	struct usbd_ep_context *epc_entry = &epc[OUT_EPC_IDX(ep_num)];

	/* If the EP is not enabled, just set host busy and clear the nak */
	if (!epc_entry->ep_enabled) {
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_BUSY |
					 USBD_DMADESC_LAST;
		usbd_clear_nak(USB_EP_DIR_OUT | ep_num);
		return;
	}

	/* Poll on status */
	timeout = USBD_DMA_TIMEOUT;
	do {
		status = epc_entry->desc.status;
		status &= USBD_DMADESC_BUFSTAT_MASK;
	} while ((status == USBD_DMADESC_BUFSTAT_DMA_BUSY) && --timeout);

	/* Set host ready and return if timed out waiting for DMA */
	if (timeout == 0) {
		usbd->ep_out[ep_num].status = USBD_EPSTS_OUT_DATA;
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_READY |
					 USBD_DMADESC_LAST;
		SYS_LOG_ERR("!!!! Timed out waiting for DMA\n");
		return;
	}

	if (status != USBD_DMADESC_BUFSTAT_DMA_DONE) {
		SYS_LOG_ERR("Received packet on OUT EP but status != DMA DONE");
		usbd->ep_out[ep_num].status = USBD_EPSTS_OUT_DATA;
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_READY |
					 USBD_DMADESC_LAST;
		return;
	}

	/* Set rx packet length */
	status = epc_entry->desc.status;
	epc_entry->epo.rx_pkt_len = status & USBD_DMADESC_RX_BYTES;
	epc_entry->epo.data_read = 0;

	SYS_LOG_DBG("Received %d byte OUT packet on EP%d\n",
		    epc_entry->epo.rx_pkt_len, ep_num);

	/* Set Host Busy */
	epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_BUSY |
				 USBD_DMADESC_LAST;

	/* Call ep status callback */
	if (epc_entry->cb) {
		epc_entry->cb((USB_EP_DIR_OUT | ep_num), USB_DC_EP_DATA_OUT);
		if (epc_entry->epo.data_read != epc_entry->epo.rx_pkt_len)
			SYS_LOG_WRN("EP callback did not read all rx data!");
	}

	epc_entry->epo.data_read = 0;
	epc_entry->epo.rx_pkt_len = 0;

	/* Set Host Ready */
	epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_READY |
				 USBD_DMADESC_LAST;

	/* Clear the OUT status */
	usbd->ep_out[ep_num].status = USBD_EPSTS_OUT_DATA;
}

/**
 * @brief Process all EP out 'DATA' interrupts
 * @details This function calls process_epout_interrupt() for all endpoints that
 *	    have received an OUT token.
 */
static void process_epout_interrupts(u32_t out_int_mask)
{
	int i;

	/* Loop through all end points */
	for (i = 0; i < USBD_NUM_OUT_ENDPOINTS; i++) {
		if (!(out_int_mask & BIT(i)))
			continue;

		/* Handle OUT DATA_DESC interrupts only, error interrupts and
		 * setup desc interrupts are handled in usbd_isr()
		 */
		if ((usbd->ep_out[i].status & USBD_EPSTS_OUT_MASK) ==
		    USBD_EPSTS_OUT_DATA) {
			/* Clear interrupt bit */
			usbd->epint = BIT(i) << USBD_NUM_IN_ENDPOINTS;
			process_epout_interrupt(i);
		}
	}

	/* Enable rx_dma after all out ep interrupts are serviced */
	usbd_rx_dma_enable();
}

/*
 * @brief Cancel any end point transaction and restore end point
 */
void usbd_cancel(u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	struct usbd_ep_context *epc_entry = USBD_EP_DIR_IN(ep) ?
					    &epc[idx] : &epc[OUT_EPC_IDX(idx)];

	if (USBD_EP_DIR_IN(ep)) {
		usbd_set_nak(ep); /* Set NAK */
		epc_entry->epi.state = USBD_EPS_IDLE; /* Reset state */

		/* Flush the TX FIFO, take back control of the endpoint */
		usbd->ep_in[idx].control |= USBD_EPCTRL_FLUSHTXFIFO;
		usbd->ep_in[idx].control &= ~USBD_EPCTRL_POLL_DEMAND;
		usbd->ep_in[idx].control &= ~USBD_EPCTRL_FLUSHTXFIFO;

		/* Set host busy */
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_BUSY |
					 USBD_DMADESC_LAST;
	} else {
		if (is_ctrl(epc_entry))
			USBD_STATUS_CB(usbd_data, USB_DC_ERROR);

		usbd_clear_nak(ep); /* Clear NAK */

		/* Set host busy */
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_READY |
					 USBD_DMADESC_LAST;
	}
}

/* Get AHB error string for a given endpoint index
 * Returns "None", "Buffer Error", "Destination Error" or "Reserved"
 */
const char *get_rxtx_err_string(u32_t epc_idx)
{
	switch (epc[epc_idx].desc.status & USBD_DMADESC_RXTXSTAT_MASK) {
	default:
	case USBD_DMADESC_RXTXSTAT_SUCCESS:
		return "None";
	case USBD_DMADESC_RXTXSTAT_DESERR:
		return "Destination Error";
	case USBD_DMADESC_RXTXSTAT_RESERVED:
		return "Reserved";
	case USBD_DMADESC_RXTXSTAT_BUFERR:
		return "Buffer Error";
	}
}

/*
 * @brief Handle Endpoint errors
 * @details This function handle endpoint errors. If any error are detected, the
 *	    endpoint sw state is reset and the interrupt bit is cleared to avoid
 *	    any further processing of IN or OUT interrupts for the end points.
 */
u32_t handle_ep_errors(u32_t epint)
{
	int i;
	u32_t status;
	bool reset_ep;

	for (i = 0; i < TOTAL_ENDPOINTS; i++) {
		u8_t idx = i & (USBD_NUM_IN_ENDPOINTS - 1);
		u8_t ep = EPC_IDX_TO_EP(i);

		if (!(BIT(i) & epint))
			continue;

		reset_ep = false;
		status = USBD_EP_DIR_IN(ep) ?
			 usbd->ep_in[idx].status : usbd->ep_out[idx].status;

		/* Handle AHB error */
		if (status & USBD_EPSTS_AHB_ERR) {
			usbd_dump_endpoint(ep);
			SYS_LOG_ERR("%s AHB error: EP%d [%s]",
				USBD_EP_DIR_IN(ep) ? "IN" : "OUT",
				idx, get_rxtx_err_string(i));

			if (USBD_EP_DIR_IN(ep))
				usbd->ep_in[idx].status = USBD_EPSTS_AHB_ERR;
			else
				usbd->ep_out[idx].status = USBD_EPSTS_AHB_ERR;

			reset_ep = true;
		}

		/* Handle Buffer not available error */
		if (status & USBD_EPSTS_BUFNOTAVAIL) {
			usbd_dump_endpoint(ep);
			SYS_LOG_ERR("%s buf not avail error: EP%d [%s]\n",
				USBD_EP_DIR_IN(ep) ? "IN" : "OUT",
				idx, get_rxtx_err_string(i));

			if (USBD_EP_DIR_IN(ep))
				usbd->ep_in[idx].status =
					USBD_EPSTS_BUFNOTAVAIL;
			else
				usbd->ep_out[idx].status =
					USBD_EPSTS_BUFNOTAVAIL;

			reset_ep = true;
		}

		/* Handle Tx DMA error on IN EP */
		if (USBD_EP_DIR_IN(ep) && (status & USBD_EPSTS_TX_DMA_DONE)) {
			if (epc[i].desc.status & USBD_DMADESC_RXTXSTAT_MASK) {
				SYS_LOG_ERR("IN TX DMA Error: EP%d\n", idx);
				usbd->ep_in[idx].status = USBD_EPSTS_TX_DMA_DONE;
				usbd->ep_in[idx].status = USBD_EPSTS_IN;
				reset_ep = true;
			}
		}

		/* Handle Rx DMA error on OUT EP */
		if (USBD_EP_DIR_OUT(ep) &&
		    ((epc[i].desc.status & USBD_DMADESC_BUFSTAT_MASK) ==
		     USBD_DMADESC_BUFSTAT_DMA_DONE)) {
			if (epc[i].desc.status & USBD_DMADESC_RXTXSTAT_MASK) {
				SYS_LOG_ERR("OUT RX DMA Error: EP%d\n", idx);
				usbd->ep_out[idx].status &= USBD_EPSTS_OUT_MASK;
				reset_ep = true;
			}
		}

		if (reset_ep) {
			usbd_cancel(ep);
			epint &= ~BIT(i);
			usbd->epint = BIT(i);
		}
	}

	return epint;
}

/* Get max packet size for a end point type */
static u32_t get_max_packet_size(u8_t ep_type)
{
	switch (ep_type) {
	default:
	case USB_DC_EP_CONTROL:
		return USBD_CNTL_MAX_PKT_SIZE;
	case USB_DC_EP_ISOCHRONOUS:
		return USBD_ISOC_MAX_PKT_SIZE;
	case USB_DC_EP_BULK:
		return USBD_BULK_MAX_PKT_SIZE;
	case USB_DC_EP_INTERRUPT:
		return USBD_INTR_MAX_PKT_SIZE;
	}
}

/*
 * @brief Save SETUP payload data
 */
void save_setup_payload(struct usbd_ep_context *epc_entry,
			u32_t word1,
			u32_t word2)
{
	/* SETUP packet payload is stored at the end out the OUT EP buffer */
	u32_t *buffer = (u32_t *)(epc_entry->epo.buffer +
				  get_max_packet_size(USB_DC_EP_CONTROL));

	buffer[0] = word1;
	buffer[1] = word2;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	clean_dcache_by_addr((u32_t)buffer, 8);
#endif
}

/*
 * @brief Handle set config interrupt
 * @details
 * Problem:
 *	The reception of a set config packet over the USB bus, from the USB
 *	host does not result in an EP OUT interrupt (SETUP) on the control
 *	endpoint EP0. Instead we receive a SET_CONFIG interrupt from the
 *	USB device controller. It is still not clear where the wValue field
 *	(that holds the configuration index) from this SETUP packet can be
 *	retrieved. The legacy USBD drivers assume the config index as 1
 *	always. Perhaps the USBD hardware only support one configuration
 *	index. The device controller driver requires the current config
 *	index, interface number and alternate setting number to program the
 *	UDC registers for the endpoints that belong to the current
 *	configration.
 * Solution:
 *	This functions emulates the reception of a SET_CONFIG SETUP packet on
 *	control EP0 and hardcodes the config index to 1.
 *
 */
void handle_set_config(void)
{
	struct usbd_ep_context *epc_entry;

	SYS_LOG_DBG("Set Config received");

	epc_entry = &epc[OUT_EPC_IDX(0)];
	epc_entry->epo.rx_pkt_len = 8;
	epc_entry->epo.data_read = 0;

	/* Get config index from the wValue field in the setup packet.
	 * Using the hardcoded current config index, since there appears to be
	 * no way to retrieve the config index received in the SETUP packet
	 */
	usbd_data.cur_config = USBD_DEFAULT_CONFIG_INDEX;

	/* Set the setup payload for a SET_CONFIG (0x09) packet */
	save_setup_payload(epc_entry, 0x0900 | (usbd_data.cur_config << 16), 0);
	epc_entry->epo.in_setup_callback = true;
	epc_entry->cb((0 | USB_EP_DIR_OUT), USB_DC_EP_SETUP);
	epc_entry->epo.in_setup_callback = false;
	epc_entry->epo.rx_pkt_len = 0;

	/* Set the CSR_DONE bit, as the EPs belonging to the configuration
	 * are expected to be configured and enabled in the above EP callback
	 */
	usbd->devctrl |= USBD_CTRL_UDCCSR_DONE;
}

/*
 * @brief Handle set interface interrupt
 * @details
 * Problem:
 *	The reception of a set interface packet over the USB bus, from the USB
 *	host does not result in an EP OUT interrupt (SETUP) on the control
 *	endpoint EP0. Instead we receive a SET_INTERFACE interrupt from the
 *	USB device controller. It is still not clear where the wValue/wIndex
 *	fields (which holds the configuration index) from this SETUP packet can
 *	be retrieved. The legacy USBD drivers assume the intf num/alt setting as
 *	0 always. Perhaps the USBD hardware only support one interface per
 *	configuration and only one alternate setting per interface. The device
 *	controller driver requires the current config index, interface number
 *	and alternate setting number to program the UDC registers for the
 *	endpoints that belong to the current configration.
 * Solution:
 *	This functions emulates the reception of a SET_INTERFACE SETUP packet on
 *	control EP0 and hardcodes the interface number and alternate setting to
 *	0.
 *
 */
void handle_set_interface(void)
{
	struct usbd_ep_context *epc_entry;

	SYS_LOG_DBG("Set Interface received");

	epc_entry = &epc[OUT_EPC_IDX(0)];
	epc_entry->epo.rx_pkt_len = 8;
	epc_entry->epo.data_read = 0;

	/* Set the setup payload for a SET_INTERFACE (0x0B) packet */
	save_setup_payload(epc_entry,
			   0x0B00 | (USBD_DEFAULT_ALTERNATE_SETTING << 16),
			   USBD_DEFAULT_INTERFACE_NUM);
	epc_entry->epo.in_setup_callback = true;
	epc_entry->cb((0 | USB_EP_DIR_OUT), USB_DC_EP_SETUP);
	epc_entry->epo.in_setup_callback = false;
	epc_entry->epo.rx_pkt_len = 0;

	/* Set the CSR_DONE bit, as the EPs belonging to the configuration
	 * are expected to be configured and enabled in the above EP callback
	 */
	usbd->devctrl |= USBD_CTRL_UDCCSR_DONE;
}

/*
 * @brief Handle device error interrupts
 * @detials Handles the listed device level interrupts
 *	- Reset
 *	- LPM token
 *	- Idle
 *	- Suspend
 *	- Set config
 *	- Enum speed done
 *	- Set interface
 *
 * @return true if SET_CONFIG or SET_INTERFACE interrupt was handled, else false
 */
bool handle_device_interrupts(u32_t devint)
{
	bool ret = false;

	if (devint & USBD_INT_RESET) {
		usbd->devint = USBD_INT_RESET;
		USBD_STATUS_CB(usbd_data, USB_DC_RESET);
		SYS_LOG_DBG("Reset received\n");
	}

	if (devint & USBD_INT_SET_CONFIG) {
		/* Handle set config */
		handle_set_config();
		/* Clear interrupt */
		usbd->devint = USBD_INT_SET_CONFIG;
		ret = true;
	}

	if (devint & USBD_INT_SET_INTERFACE) {
		/* Handle set config */
		handle_set_interface();
		/* Clear interrupt */
		usbd->devint = USBD_INT_SET_INTERFACE;
		ret = true;
	}

	if (devint & USBD_INT_ENUM_SPEED) {
		usbd->devint = USBD_INT_ENUM_SPEED;
		USBD_STATUS_CB(usbd_data, USB_DC_CONNECTED);
		SYS_LOG_DBG("Speed Enumeration completed\n");
	}

	if (devint & USBD_INT_SUSPEND) {
		usbd->devint = USBD_INT_SUSPEND;
		USBD_STATUS_CB(usbd_data, USB_DC_SUSPEND);
	}

	if (devint & USBD_INT_RMTWKP) {
		usbd->devint = USBD_INT_RMTWKP;
		USBD_STATUS_CB(usbd_data, USB_DC_RESUME);
		SYS_LOG_DBG("Remote Wakeup from Host\n");
	}

	if (devint & USBD_INT_LPM_TKN)
		usbd->devint = USBD_INT_LPM_TKN;

	if (devint & USBD_INT_IDLE)
		usbd->devint = USBD_INT_IDLE;

	return ret;
}

/**
 * @brief USB device interrupt service handler
 * @details The USBD Interrupt service handler performs the following functions
 *	    in the listed order
 *
 * 1. Handle all Device interrupts (DEVSTATUS register)
 * 2. For each endpoint handle the error interrupts
 * 3. For all interrupted OUT endpoints, handle the reception of SETUP token
 * 4. For all interrupted IN endpoints, setup tx data (if available) OR set NAK
 * 5. Call process_epout_interrupts()
 */
static void usbd_dev_ep_isr(u32_t epint, u32_t devint);
static void usbd_isr(void *arg)
{
	u32_t epint, devint;

	/* Loop through ISR till interrupts are all cleared */
	epint = usbd->epint;
	devint = usbd->devint;
	do {
		usbd_dev_ep_isr(epint, devint);
		epint = usbd->epint;
		devint = usbd->devint;
	} while (epint || devint);
}

static void usbd_dev_ep_isr(u32_t epint, u32_t devint)
{
	int i;
	bool set_csr_done;
	u32_t out_int_mask, in_int_mask;

	/* Handle delayed CNAK */
	if (usbd_data.delayed_cnak &&
	    (usbd->devstatus & USBD_STS_RXFIFO_EMPTY)) {
		for (i = 0; i < TOTAL_ENDPOINTS; i++) {
			if (usbd_data.delayed_cnak & BIT(i)) {
				u8_t ep;

				ep = (i < USBD_NUM_IN_ENDPOINTS) ?
				     (i | USB_EP_DIR_IN) : (i | USB_EP_DIR_OUT);
				usbd_data.delayed_cnak &= ~(BIT(i));
				usbd_clear_nak(ep);
			}
		}
	}

	/*
	 * 1: Handle device interrupts
	 */
	if (handle_device_interrupts(devint)) {
		/* Set the CSR_DONE if we received a SET_CONFIG or
		 * SET_INTERFACE request
		 */
		set_csr_done = true;
	}

	/*
	 * 2: Handle Endpoint errors
	 *	AHB
	 *	BUFF_NOT_AVAIL
	 *	Status quadlet TX/RX status errors: DESERR, BUFERR
	 */
	epint = handle_ep_errors(epint);
	in_int_mask = epint & (BIT(USBD_NUM_IN_ENDPOINTS) - 1);
	out_int_mask = epint >> USBD_NUM_IN_ENDPOINTS;

	/*
	 * 3: Handle SETUP token
	 *  - If Setup token received
	 *	- Set host busy, set rx_pkt_len = 8 and data_read = 0
	 *	- call SETUP ep callback
	 *	- After OUT ep callback returns
	 *	    - clear int
	 *	    - set host ready
	 *	    - clear rx_pkt_len, data_read
	 *	- clear nak
	 *	- clear interrupt
	 *  - Else set nak if not setup packet
	 */
	for (i = 0; i < USBD_NUM_OUT_ENDPOINTS; i++) {
		u32_t status;
		u8_t ep = i | USB_EP_DIR_OUT;
		struct usbd_ep_context *epc_entry = &epc[OUT_EPC_IDX(i)];

		/* Skip EPs whose int bit isn't set */
		if (!(out_int_mask & BIT(i)))
			continue;

		/* Skip OUT packets */
		if ((usbd->ep_out[i].status & USBD_EPSTS_OUT_MASK) ==
			USBD_EPSTS_OUT_DATA) {
			/* Set NAK, let process_epout_interrupts() handle the
			 * OUT data interrupts.
			 */
			usbd_set_nak(ep);
			continue;
		}

		/* Neither a SETUP nor an OUT packet */
		if ((usbd->ep_out[i].status & USBD_EPSTS_OUT_MASK) !=
			USBD_EPSTS_OUT_SETUP) {
			/* Set host ready, clear interrupt, clear nak */
			epc_entry->epo.setup->status =
				USBD_DMADESC_BUFSTAT_HOST_READY;
			usbd->epint = BIT(USBD_NUM_IN_ENDPOINTS + i);
			usbd_clear_nak(ep);
			out_int_mask &= ~BIT(i);
			continue;
		}

		/* Check DMA status */
		status = epc_entry->epo.setup->status;
		status &= USBD_DMADESC_BUFSTAT_MASK;
		if (status != USBD_DMADESC_BUFSTAT_DMA_DONE) {
			SYS_LOG_WRN("Setup interrupt received, but dma status "
				    "is not DMA_DONE : %x", status);
			continue;
		}

		/* Copy the SETUP packet data before it gets overwritten by
		 * another SETUP packet
		 */
		save_setup_payload(epc_entry,
				   epc_entry->epo.setup->data[0],
				   epc_entry->epo.setup->data[1]);

		/* Clear interrupt */
		usbd->epint = BIT(USBD_NUM_IN_ENDPOINTS + i);

		/* Clear the OUT status */
		usbd->ep_out[i].status = USBD_EPSTS_OUT_SETUP;

		/* Set host busy */
		epc_entry->epo.setup->status = USBD_DMADESC_BUFSTAT_HOST_BUSY;

		/* Set epo params */
		epc_entry->epo.rx_pkt_len = 8;
		epc_entry->epo.data_read = 0;

		/* Call EP callback */
		if (epc_entry->cb) {
			epc_entry->epo.in_setup_callback = true;
			epc_entry->cb(ep, USB_DC_EP_SETUP);
			epc_entry->epo.in_setup_callback = false;
			if (epc_entry->epo.rx_pkt_len !=
			    epc_entry->epo.data_read) {
				SYS_LOG_WRN("Setup callback: User did not read "
					    "the complete packet");
			}
		}

		/* Set the CSR_DONE: After the EP setup callback in response to
		 * a SET_CONFIG or SET_INTERFACE is complete. At this point we
		 * expect all the UDC register config to be completed by calls
		 * to ep_enable() from the ep callback function.
		 */
		if (set_csr_done)
			usbd->devctrl |= USBD_CTRL_UDCCSR_DONE;

		/* Reset epo params */
		epc_entry->epo.rx_pkt_len = 0;
		epc_entry->epo.data_read = 0;

		/* Set host ready */
		epc_entry->epo.setup->status = USBD_DMADESC_BUFSTAT_HOST_READY;

		/* Clear NAK */
		usbd_clear_nak(ep);

		/* Clear out_int_mask: process_epout_interrupts() will handle
		 * only DATA packets.
		 */
		out_int_mask &= ~BIT(i);
	}

	/*
	 * 4: Handle IN token interrupts
	 *    - If TX_DMA_DONE interrupt, verify status is active, call the
	 *      endpoint callback and set state = idle. Then set NAK.
	 *    - If a usb_ep_write() has already been called for this endpoint
	 *      (state pending) configure the tx dma (set poll demand and host
	 *      ready = 1, state = active)
	 *    - Else Set NAK, the IN token will be re-sent by host
	 *    - Clear interrupt
	 */
	for (i = 0; i < USBD_NUM_IN_ENDPOINTS; i++) {
		u32_t ep_status;
		u8_t ep = i | USB_EP_DIR_IN;
		struct usbd_ep_context *epc_entry = &epc[i];

		if (!(in_int_mask & BIT(i)))
			continue;

		/* TX_DMA_DONE */
		ep_status = usbd->ep_in[i].status;
		if (ep_status & USBD_EPSTS_TX_DMA_DONE) {
			usbd->ep_in[i].status = USBD_EPSTS_TX_DMA_DONE;

			SYS_LOG_DBG("TX DMA complete: %d bytes sent\n",
				epc_entry->desc.status & USBD_DMADESC_TX_BYTES);
			if (epc_entry->epi.state == USBD_EPS_ACTIVE) {
				if (epc_entry->epi.num_in_dmas) {
					/* If more DMAs left, configure next */
					config_tx_dma(ep);
				} else {
					/* IN epc [0..15] */
					epc_entry->epi.state = USBD_EPS_IDLE;
					epc_entry->cb(ep, USB_DC_EP_DATA_IN);
				}
			}
		}

		/* Clear other TX status bits */
		if (ep_status & USBD_EPSTS_TX_FIFO_EMPTY)
			usbd->ep_in[i].status = USBD_EPSTS_TX_FIFO_EMPTY;

		if (ep_status & USBD_EPSTS_TX_DONE_FIFO_EMPTY)
			usbd->ep_in[i].status = USBD_EPSTS_TX_DONE_FIFO_EMPTY;

		/* IN token */
		if (ep_status & USBD_EPSTS_IN) {
			if (epc_entry->epi.state == USBD_EPS_PENDING)
				start_tx_dma(ep);
			usbd->ep_in[i].status = USBD_EPSTS_IN;
		}

		/* Clear interrupt */
		usbd->epint = BIT(i);
	}

	/* 5: Handle EP OUT DATA token interrupts */
#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
	usbd_data.ept.msgs[++usbd_data.epf.index].out_int_mask = out_int_mask;
	k_fifo_put(fifo, &usbd_data.ept.msgs[usbd_data.epf.index]);
#else
	process_epout_interrupts(usbd->epint >> USBD_NUM_IN_ENDPOINTS);
#endif
}

/**
 * @brief USB device controller initialization
 */
static int usb_dc_bcm5820x_init(struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	/* Initialize the usbd device data structures */
	usbd_data.attached = false;
	usbd_data.cb = NULL;
	usbd_data.udc_reg_no = 0;
	usbd_data.cur_config = 0;

	/* Get uncached addresses for setup descriptor and end point context */
	usbd_data.setup = UNCACHED_ADDR(&setup_desc);
	epc = UNCACHED_ADDR(&ep_context[0]);

	/* Clear the setup descriptor and end point information */
	memset(usbd_data.setup, 0, sizeof(setup_desc));
	memset(epc, 0, sizeof(ep_context));

#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
	/* Create the thread for servicing endpoint interrupts */
	if (k_thread_create(&usbd_data.ept.thread, usbd_data.ept.stack,
			    EPOUT_IST_STACK_SIZE, epout_ist_fn, NULL,
			    NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT) == NULL) {
		SYS_LOG_ERR("Error creating EPOUT interrupt service thread!");
		return -EPERM;
	}

	/* Initialize the epout ist fifo */
	k_fifo_init(&usbd_data.ept.fifo);
#endif

	/* Initialize the USBD Phy */
	ret = usbd_phy_init();
	if (ret) {
		SYS_LOG_ERR("Error initializing USB PHY: %d", ret);
		return ret;
	}

	ret = usbd_hw_init();
	if (ret) {
		SYS_LOG_ERR("Error initializing USB Hardware: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief attach USB for device connection
 *
 * Function to attach USB for device connection. Upon success, the USB PLL
 * is enabled, and the USB device is now capable of transmitting and receiving
 * on the USB bus and of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_attach(void)
{
	int i, ret;

	/* Enable PLL */
	ret = usbd_pll_enable();
	if (ret) {
		SYS_LOG_ERR("Error enabling USBD PLL: %d", ret);
		return ret;
	}

	/* Reset usbd core/phy */
	ret = usbd_hw_reset();
	if (ret) {
		SYS_LOG_ERR("Error resetting USBD core/phy: %d", ret);
		return ret;
	}

	/* Set nak for all end points */
	for (i = 0; i < USBD_NUM_IN_ENDPOINTS; i++)
		usbd->ep_in[i].control |= USBD_EPCTRL_SET_NAK;
	for (i = 0; i < USBD_NUM_OUT_ENDPOINTS; i++)
		usbd->ep_out[i].control |= USBD_EPCTRL_SET_NAK;

	/* Disable soft disconnect */
	usbd->devctrl &= ~USBD_CTRL_SOFT_DISC;

	/* Install ISR */
	IRQ_CONNECT(USBD_IRQ, 0, usbd_isr, NULL, 0);

	/* Clear all pending endpoint interrupts */
	usbd->epint = 0xFFFFFFFF;

	/* Enable USBD interrupt */
	irq_enable(USBD_IRQ);

	usbd_data.attached = true;
	return 0;
}

/**
 * @brief detach the USB device
 *
 * Function to detach the USB device. Upon success, the USB hardware PLL
 * is powered down and USB communication is disabled.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_detach(void)
{
	int ret, i;

	ret = usbd_pll_disable();
	if (ret) {
		SYS_LOG_ERR("Error disabling USBD PLL: %d", ret);
		return ret;
	}

	/* Disable USBD interrupt */
	irq_disable(USBD_IRQ);

	/* Enable soft disconnect */
	usbd->devctrl |= USBD_CTRL_SOFT_DISC;

	usbd_data.attached = false;

	/* Reset UDC EP regs */
	for (i = 0; i < USBD_NUM_UDC_EP_REGS; i++)
		usbd->udc_ep[i] = 0;

	return 0;
}

/**
 * @brief reset the USB device
 *
 * This function returns the USB device and firmware back to it's initial state.
 * N.B. the USB PLL is handled by the usb_detach function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_reset(void)
{
	int i, ret;

	ret = usbd_hw_reset();
	if (ret) {
		SYS_LOG_ERR("Error resetting USBD core/phy: %d", ret);
		return ret;
	}

	/* Initialize the usbd device data structures */
	usbd_data.attached = false;
	usbd_data.cb = NULL;
	usbd_data.udc_reg_no = 0;
	usbd_data.cur_config = 0;

#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
	/* Empty EPOUT IST fifo */
	while (!k_fifo_is_empty(&usbd_data.ept.fifo))
		k_fifo_get(&usbd_data.ept.fifo, K_NO_WAIT);
#endif

	/* Clear the setup descriptor and end point information */
	memset(usbd_data.setup, 0, sizeof(setup_desc));

	/* Release any OUT EP buffer memory before clearing the EP context */
	for (i = USBD_NUM_IN_ENDPOINTS; i < TOTAL_ENDPOINTS; i++) {
		if (is_out(&epc[i]) && epc[i].epo.buffer)
			cache_line_aligned_free(epc[i].epo.buffer);
	}
	memset(epc, 0, sizeof(ep_context));

	/* Reset UDC EP regs */
	for (i = 0; i < USBD_NUM_UDC_EP_REGS; i++)
		usbd->udc_ep[i] = 0;

	return 0;
}

/**
 * @brief set USB device address
 *
 * @param[in] addr device address
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_set_address(const u8_t addr)
{
	ARG_UNUSED(addr);

	/* The USBD controller on BCM 5820X SoCs does not provide an option to
	 * configure the slave address to which the Device should respond after
	 * a SET_ADDR command from the host. So the USBD hardware controller
	 * is expected respond to the host packets that are addressed to the
	 * device following the SET_ADDR transaction.
	 */

	return 0;
}

/**
 * @brief set USB device controller status callback
 *
 * Function to set USB device controller status callback. The registered
 * callback is used to report changes in the status of the device controller.
 *
 * @param[in] cb callback function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	usbd_data.cb = cb;

	return 0;
}

/**
 * @brief configure endpoint
 *
 * Function to configure an endpoint. usb_dc_ep_cfg_data structure provides
 * the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t epc_idx, idx;
	struct usbd_ep_context *epc_entry;

	if (!usbd_data.attached)
		return -EINVAL;

	SYS_LOG_DBG("Configuring EP 0x%x", cfg->ep_addr);

	/* Index ranges from 0 - 15 and is used to address registers that
	 * specific to In and OUT EPs
	 */
	idx = USBD_EP_IDX(cfg->ep_addr);

	/* epc_idx ranges from 0 - 31 used to address the ep_context array,
	 * which holds the Endpoint software state information. 0 - 15 indices
	 * are for IN endpoints and 16 - 31 indices are for OUT endpoints.
	 */
	epc_idx = USBD_EP_DIR_IN(cfg->ep_addr) ? idx : OUT_EPC_IDX(idx);

	epc_entry = &epc[epc_idx];

	epc_entry->cfg = *cfg;
	epc_entry->epi.state = USBD_EPS_IDLE;
	epc_entry->epo.rx_pkt_len = 0;
	epc_entry->epo.data_read = 0;
	epc_entry->epo.buffer = NULL;

	/* Configure IN End point */
	if (is_in(epc_entry)) {
		volatile struct usbd_ep_in_s *ep_in = &usbd->ep_in[idx];

		/* Set EP type */
		ep_in->control = cfg->ep_type << USBD_EPCTRL_TYPE_SHIFT;
		/* Set max packet size */
		ep_in->max_pkt_size = cfg->ep_mps;
		/* Set buffer size */
		ep_in->buff_size = cfg->ep_mps >> 2; /* In 32-bit words */
		/* Flush Tx Fifo */
		ep_in->control |= USBD_EPCTRL_FLUSHTXFIFO;
		ep_in->control &= ~USBD_EPCTRL_FLUSHTXFIFO;

		/* Nothing to send yet, so set NAK */
		ep_in->control |= USBD_EPCTRL_SET_NAK;

		/* Set the data descriptor status and buffer pointers */
		epc_entry->desc.buffer = NULL;
		epc_entry->desc.next = NULL;
		epc_entry->desc.status = USBD_DMADESC_BUFSTAT_HOST_BUSY |
					   USBD_DMADESC_LAST;
	} else { /* Configure OUT End point */
		u32_t size;
		volatile struct usbd_ep_out_s *ep_out = &usbd->ep_out[idx];

		/* Set EP type */
		ep_out->control = cfg->ep_type << USBD_EPCTRL_TYPE_SHIFT;

		/* Set max packet size */
		ep_out->size = cfg->ep_mps;

		/* FIXME: The driver assumes, there is only one control OUT
		 * endpoint (EP0, the default one) and uses usbd_data.setup
		 * for the setup descriptor. The api returns an error if the
		 * user were to configure a non-EP0 control OUT endpoint.
		 */
		if (is_ctrl(epc_entry)) {
			if (idx != 0)
				return -ENOTSUP;

			/* Configure Setup descriptor for control out end points
			 * Use Uncached setup descriptor address
			 */
			ep_out->setup_desc = (u32_t)&setup_desc;

			/* Mark the setup desc as ready to receive a packet */
			usbd_data.setup->status &= ~USBD_DMADESC_BUFSTAT_MASK;
			usbd_data.setup->status |=
					USBD_DMADESC_BUFSTAT_HOST_READY;

			/* Update the epc structure too */
			epc_entry->epo.setup = usbd_data.setup;

			/* 8 additional bytes for the SETUP packet data */
			size = 8;
		} else {
			size = 0;
		}

		/* Allocate buffer for the non-control OUT EP */
		size += get_max_packet_size(cfg->ep_type);
		if (epc_entry->epo.buffer == NULL)
			epc_entry->epo.buffer =
				cache_line_aligned_alloc(size);

		if (epc_entry->epo.buffer == NULL)
			return -ENOMEM;

		/* Set the data descriptor status and buffer pointers */
		epc_entry->desc.buffer = epc_entry->epo.buffer;
		epc_entry->desc.next = NULL;
		epc_entry->desc.status = USBD_DMADESC_LAST |
					 USBD_DMADESC_BUFSTAT_HOST_READY;

		/* Set the OUT EP data descriptor */
		ep_out->data_desc = (u32_t)&ep_context[epc_idx].desc;
	}

	/* Clear delayed NAK */
	usbd_data.delayed_cnak &= ~BIT(epc_idx);

	/* Configure UDC20 register (For all except control IN ep) */
	if (!is_ctrlin(epc_entry)) {
		u32_t regval;

		regval = idx | (cfg->ep_mps << USBD_UDCEP_MAX_PKTSIZE_SHIFT);
		regval |= is_out(epc_entry) ? USBD_UDCEP_DIR_OUT
					   : USBD_UDCEP_DIR_IN;
		regval |= cfg->ep_type << USBD_UDCEP_TYPE_SHIFT;

		regval |= usbd_data.cur_config << USBD_UDCEP_CONFIG_SHIFT;

		usbd->udc_ep[usbd_data.udc_reg_no] = regval;
		usbd_data.udc_reg_no++;
	}

	/* Set configured flag */
	epc_entry->configured = true;
	return 0;
}

/**
 * @brief set stall condition for the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_stall(const u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);

	if (!usbd_data.attached) {
		return -EINVAL;
	}

	if (!is_configured(ep)) {
		return -EINVAL;
	}

	if (!idx) {
		/* Not possible to set stall for EP0? */
		return -EINVAL;
	}

	if (USBD_EP_DIR_OUT(ep))
		usbd->ep_out[idx].control |= USBD_EPCTRL_STALL;
	else
		usbd->ep_in[idx].control |= USBD_EPCTRL_STALL;

	/* FIXME: Confirm if below requirements are valid
	 *
	 * 1. The application must check for RxFIFO emptiness before
	 *    setting the IN and OUT STALL bit.
	 *
	 * 2. Code from MPOS driver
	 *	Synopsys AHB p 210 chapter 13.4  clear OUT NAK bit
	 *	usbd_clear_nak(ep);
	 *	usbd_rx_dma_enable();
	 */
	return 0;
}

/**
 * @brief clear stall condition for the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_clear_stall(const u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);

	if (!usbd_data.attached) {
		return -EINVAL;
	}

	if (!is_configured(ep)) {
		return -EINVAL;
	}

	if (!idx) {
		/* Not possible to clear stall for EP0 */
		return -EINVAL;
	}

	if (USBD_EP_DIR_OUT(ep))
		usbd->ep_out[idx].control &= ~USBD_EPCTRL_STALL;
	else
		usbd->ep_in[idx].control &= ~USBD_EPCTRL_STALL;

	return 0;
}

/**
 * @brief check if selected endpoint is stalled
 *
 * @param[in]  ep       Endpoint address corresponding to the one
 *                      listed in the device configuration table
 * @param[out] stalled  Endpoint stall status
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_is_stalled(const u8_t ep, u8_t *const stalled)
{
	u8_t idx = USBD_EP_IDX(ep);

	if (!usbd_data.attached) {
		return -EINVAL;
	}

	if (!is_configured(ep)) {
		return -EINVAL;
	}

	if (stalled == NULL) {
		return -EINVAL;
	}

	if (USBD_EP_DIR_OUT(ep))
		*stalled = usbd->ep_out[idx].control & USBD_EPCTRL_STALL;
	else
		*stalled = usbd->ep_in[idx].control & USBD_EPCTRL_STALL;

	return 0;
}

/**
 * @brief halt the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_halt(const u8_t ep)
{
	/* USBD endpoint control registers don't provide a endpoint disable
	 * bit. Setting the stall bit should achieve what is needed of the api
	 */
	return usb_dc_ep_set_stall(ep);
}

/**
 * @brief enable the selected endpoint
 *
 * Function to enable the selected endpoint. Upon success interrupts are
 * enabled for the corresponding endpoint and the endpoint is ready for
 * transmitting/receiving data.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_enable(const u8_t ep)
{
	u32_t status;
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	SYS_LOG_DBG("Enabling EP 0x%x", ep);
	/* Enable EP int mask */
	usbd->epintmask &= ~BIT(epc_idx);

	if (USBD_EP_DIR_OUT(ep)) {
		status = USBD_DMADESC_BUFSTAT_HOST_READY;

		/* Set HOST READY in the SETUP descriptor if it is a control EP
		 */
		if (is_ctrl(&epc[epc_idx])) /* Control EP */
			epc[epc_idx].epo.setup->status = status;

		/* Set both HOST READY and LAST bits for in OUT descriptor */
		epc[epc_idx].desc.status = status |= USBD_DMADESC_LAST;

		/* Clear NAK */
		usbd_clear_nak(ep);
	} else {
		/* FIXME: Is this really needed? */
		/* Set NAK for IN EPs to begin with */
		usbd_set_nak(ep);
	}

	/* Set EP enabled flag */
	epc[epc_idx].ep_enabled = true;

	if (is_ctrlout(&epc[epc_idx])) {
		/* CSR programming can be assumed to be complete once the
		 * host is ready to enable EPOUT0
		 */
		usbd->devctrl |= USBD_CTRL_UDCCSR_DONE;

		/* Set RXDMA_EN only after ctrl out ep is configured */
		usbd_rx_dma_enable();
	}
	return 0;
}

/**
 * @brief disable the selected endpoint
 *
 * Function to disable the selected endpoint. Upon success interrupts are
 * disabled for the corresponding endpoint and the endpoint is no longer able
 * for transmitting/receiving data.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_disable(const u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	/* Disable EP interrupts */
	usbd->epintmask |= BIT(epc_idx);

	/* Set NAK for EP */
	if (USBD_EP_DIR_OUT(ep))
		usbd->ep_out[idx].control |= USBD_EPCTRL_SET_NAK;
	else
		usbd->ep_in[idx].control |= USBD_EPCTRL_SET_NAK;

	epc[epc_idx].ep_enabled = false;
	return 0;
}

/**
 * @brief flush the selected endpoint
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_flush(const u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u32_t timeout;

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	if (USBD_EP_DIR_OUT(ep)) {
#ifdef USBD_SUPPORTS_MULTI_RX_FIFO
		/* Set NAK first */
		usbd->ep_out[idx].control |= USBD_EPCTRL_SET_NAK;
		/* Set MRX_FIFO and poll for completion */
		usbd->ep_out[idx].control |= USBD_EPCTRL_MRX_FLUSH;
		timeout = USBD_FIFO_FLUSH_TIMEOUT;
		do {
			if (usbd->ep_out[idx].status & USBD_EPSTS_RXFIFO_EMPTY)
				break;
			k_busy_wait(1);
		} while (--timeout);

		/* Clear flush bit and NAK */
		usbd->ep_out[idx].control &= ~USBD_EPCTRL_MRX_FLUSH;
		usbd->ep_out[idx].control |= USBD_EPCTRL_CLEAR_NAK;

		if (timeout == 0)
			return -ETIMEDOUT;
#else
		/* Single RX FIFO implementation does not provide a way to flush
		 * individual OUT endpoints
		 */
		return -EINVAL;
#endif
	} else {
		/* Set Flush TX fifo and poll for completion */
		usbd->ep_in[idx].control |= USBD_EPCTRL_FLUSHTXFIFO;
		timeout = USBD_FIFO_FLUSH_TIMEOUT;
		do {
			if (usbd->ep_in[idx].status & USBD_EPSTS_TX_DMA_DONE)
				break;
			k_busy_wait(1);
		} while (--timeout);

		if (timeout == 0)
			return -ETIMEDOUT;
	}

	return 0;
}

/**
 * @brief set callback function for the specified endpoint
 *
 * Function to set callback function for notification of data received and
 * available to application or transmit done on the selected endpoint,
 * NULL if callback not required by application code.
 *
 * @param[in] ep Endpoint address corresponding to the one
 *               listed in the device configuration table
 * @param[in] cb callback function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_callback(const u8_t ep, const usb_dc_ep_callback cb)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	if (!usbd_data.attached)
		return -EINVAL;

	/* Set the endpoint callback */
	epc[epc_idx].cb = cb;

	return 0;
}

/**
 * @brief read data from the specified endpoint
 *
 * This function is called by the Endpoint handler function, after an OUT
 * interrupt has been received for that EP. The application must only call this
 * function through the supplied usb_ep_callback function. This function clears
 * the ENDPOINT NAK, if all data in the endpoint FIFO has been read,
 * so as to accept more data from host.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 * @param[in]  data         pointer to data buffer to write to
 * @param[in]  max_data_len max length of data to read
 * @param[out] read_bytes   Number of bytes read. If data is NULL and
 *                          max_data_len is 0 the number of bytes
 *                          available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read(const u8_t ep, u8_t *const data,
		const u32_t max_data_len, u32_t *const read_bytes)
{
	if (usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes) != 0) {
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	if (usb_dc_ep_read_continue(ep) != 0) {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief read data from the specified endpoint
 *
 * This is similar to usb_dc_ep_read, the difference being that, it doesn't
 * clear the endpoint NAKs so that the consumer is not bogged down by further
 * upcalls till he is done with the processing of the data. The caller should
 * reactivate ep by invoking usb_dc_ep_read_continue() do so.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 * @param[in]  data         pointer to data buffer to write to
 * @param[in]  max_data_len max length of data to read
 * @param[out] read_bytes   Number of bytes read. If data is NULL and
 *                          max_data_len is 0 the number of bytes
 *                          available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note If CONFIG_SERVICE_EPOUT_INT_IN_THREAD is set, this api will be
 *	 called from an interrupt context, if since the endpoint callback will
 *	 be called from the usbd_isr and the read() is expected to be called
 *	 from within the callback. This should fine since data from the OUT
 *	 packet would already be available in the malloc'ed buffer for the
 *	 endpoint.
 */
int usb_dc_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *read_bytes)
{
	u8_t *src_buff;
	u8_t idx = USBD_EP_IDX(ep);
	u32_t bytes_left, bytes_read;
	struct usbd_ep_context *epc_entry;

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	/* Check ep is OUT */
	if (!(USBD_EP_DIR_OUT(ep))) {
		SYS_LOG_ERR("Read on OUT endpoint!");
		return -EINVAL;
	}

	if (!data && max_data_len) {
		SYS_LOG_ERR("Invalid buffer");
		return -EINVAL;
	}

	epc_entry = &epc[OUT_EPC_IDX(idx)];

	/* Check if ep enabled */
	if (!epc_entry->ep_enabled) {
		SYS_LOG_ERR("Endpoint not enabled!");
		return -EINVAL;
	}

	bytes_left = epc_entry->epo.rx_pkt_len - epc_entry->epo.data_read;

	if (!data && !max_data_len) {
		/* If buffer and max data length are zero return
		 * the packet size
		 */
		if (read_bytes)
			*read_bytes = bytes_left;

		return 0;
	}

	/* Determine bytes to read */
	if (bytes_left > max_data_len) {
		if (max_data_len)
			SYS_LOG_WRN("Rx buffer too small (%d < %d)!",
				     max_data_len, bytes_left);
		bytes_read = max_data_len;
	} else {
		bytes_read = bytes_left;
	}

	SYS_LOG_DBG("Read EP%d, req %d, read %d bytes",
		    ep, max_data_len, bytes_read);

	if (is_ctrlout(epc_entry) && epc_entry->epo.in_setup_callback) {
		/* Setup payload is stored at the end of the OUT EP buffer */
		src_buff = epc_entry->epo.buffer +
			   get_max_packet_size(USB_DC_EP_CONTROL) +
			   epc_entry->epo.data_read;
	} else {
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Invalidate only on the first read */
		if (epc_entry->epo.data_read == 0)
			invalidate_dcache_by_addr((u32_t)epc_entry->epo.buffer,
						  epc_entry->epo.rx_pkt_len);
#endif
		src_buff = epc_entry->epo.buffer + epc_entry->epo.data_read;
	}

	if (max_data_len) {
		SYS_LOG_DBG("%s Data: 0x%02x 0x%02x 0x%02x 0x%02x "
			    "0x%02x 0x%02x 0x%02x 0x%02x\n",
			    epc_entry->epo.in_setup_callback ? "SETUP" : "OUT",
			    src_buff[0], src_buff[1], src_buff[2], src_buff[3],
			    src_buff[4], src_buff[5], src_buff[6], src_buff[7]);
	}

	/* Copy the data to user buffer */
	memcpy(data, src_buff, bytes_read);

	if (read_bytes)
		*read_bytes = bytes_read;

	epc_entry->epo.data_read += bytes_read;

	return 0;
}

/**
 * @brief Continue reading data from the endpoint
 *
 * Clear the endpoint NAK and enable the endpoint to accept more data
 * from the host. Usually called after usb_dc_ep_read_wait() when the consumer
 * is fine to accept more data. Thus these calls together acts as flow control
 * mechanism.
 *
 * @param[in]  ep           Endpoint address corresponding to the one
 *                          listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read_continue(u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	/* Check ep is OUT */
	if (!(USBD_EP_DIR_OUT(ep))) {
		SYS_LOG_ERR("Read on OUT endpoint!");
		return -EINVAL;
	}

	/* Clear NAK if host has read all the received data */
	if (epc[epc_idx].epo.data_read == epc[epc_idx].epo.rx_pkt_len) {
		/* Clear NAK */
		usbd->ep_out[idx].control |= USBD_EPCTRL_CLEAR_NAK;
	}

	return 0;
}

/**
 * @brief write data to the specified endpoint
 *
 * This function is called to write data to the specified endpoint. The supplied
 * usb_ep_callback function will be called when data is transmitted out.
 *
 * @param[in]  ep        Endpoint address corresponding to the one
 *                       listed in the device configuration table
 * @param[in]  data      pointer to data to write
 * @param[in]  data_len  length of data requested to write. This may
 *                       be zero for a zero length status packet.
 * @param[out] ret_bytes bytes scheduled for transmission. This value
 *                       may be NULL if the application expects all
 *                       bytes to be written
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_write(const u8_t ep, const u8_t *const data,
		const u32_t data_len, u32_t * const ret_bytes)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);

	u32_t bytes_to_send;
	struct usbd_ep_context *epc_entry;

	epc_entry = &epc[epc_idx];

	if (!usbd_data.attached)
		return -EINVAL;

	if (!is_configured(ep))
		return -EINVAL;

	/* Check if IN ep */
	if (!(USBD_EP_DIR_IN(ep)))
		return -EINVAL;

	/* Check if ep enabled */
	if (!(epc_entry->ep_enabled))
		return -EINVAL;

	/* If a previous write is still pending or is in progress,
	 * return a -EBUSY.
	 */
	if (epc_entry->epi.state == USBD_EPS_ACTIVE)
		return -EBUSY;

	if (epc_entry->epi.state == USBD_EPS_PENDING)
		usbd_cancel(ep);

	/* Prepare the tx DMA */
	prepare_tx_dma(ep, data, data_len, ret_bytes == NULL);

	/* Configure the first Tx DMA */
	bytes_to_send = config_tx_dma(ep);

	SYS_LOG_DBG("Write EP%d, req %d, writing %d bytes", ep, data_len,
		    bytes_to_send);

	if (ret_bytes)
		*ret_bytes = bytes_to_send;

	return 0;
}

#ifdef CONFIG_SERVICE_EPOUT_INT_IN_THREAD
static void epout_ist_fn(void *p1, void *p2, void *p3)
{
	struct epout_ist_fifo_entry *msg;

	/* Wait on EPOUT IST fifo and call handle_epout_interrupt() */
	while (1) {
		msg = k_fifo_get(&usbd_data.ept.fifo, K_FOREVER);

		process_epout_interrupts(msg->out_int_mask);
	}
}
#endif

int usb_dc_ep_mps(const u8_t ep)
{
	u8_t idx = USBD_EP_IDX(ep);
	u8_t epc_idx = USBD_EP_DIR_IN(ep) ? idx : OUT_EPC_IDX(idx);
	struct usbd_ep_context *epc_entry = &epc[epc_idx];

	return epc_entry->cfg.ep_mps;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data * const cfg)
{
	u8_t idx = USBD_EP_IDX(cfg->ep_addr);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && idx) {
		SYS_LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (idx >= USBD_NUM_IN_ENDPOINTS) {
		SYS_LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	return 0;
}

SYS_INIT(usb_dc_bcm5820x_init, PRE_KERNEL_2, 99);
