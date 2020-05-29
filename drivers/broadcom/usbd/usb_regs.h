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
 * @file usb_regs.h
 * @brief Citadel USB Register offsets and bit fields
 */

#ifndef __USB_REGS_H__
#define __USB_REGS_H__

#define USBD_BASE	BRCM_CIT_USBD_47000000_BASE_ADDRESS
#define USBD_IRQ	SPI_IRQ(BRCM_CIT_USBD_47000000_IRQ_0)

#define USBD_NUM_IN_ENDPOINTS	16
#define USBD_NUM_OUT_ENDPOINTS	16
#define USBD_NUM_UDC_EP_REGS	32

struct usbd_ep_in_s {
	volatile u32_t	control;
	volatile u32_t	status;
	volatile u32_t	buff_size;
	volatile u32_t	max_pkt_size;
	volatile u32_t	reserved1;
	volatile u32_t	desc;
	volatile u32_t	reserved2[2];
};

struct usbd_ep_out_s {
	volatile u32_t	control;
	volatile u32_t	status;
	volatile u32_t	frame_num;
	volatile u32_t	size;
	volatile u32_t	setup_desc;
	volatile u32_t	data_desc;
	volatile u32_t	reserved[2];
};

struct usbd_reg_map_s {
	volatile struct usbd_ep_in_s	ep_in[USBD_NUM_IN_ENDPOINTS];
	volatile struct usbd_ep_out_s	ep_out[USBD_NUM_OUT_ENDPOINTS];
	volatile u32_t	devcfg;
	volatile u32_t	devctrl;
	volatile u32_t	devstatus;
	volatile u32_t	devint;
	volatile u32_t	devintmask;
	volatile u32_t	epint;
	volatile u32_t	epintmask;
	volatile u32_t	reserved1;
	volatile u32_t	rel_num;
	volatile u32_t	lpm_ctrl_stat;
	volatile u32_t	reserved[55];
	volatile u32_t	udc_ep[USBD_NUM_UDC_EP_REGS];
};

/* USBD register map pointer
 * USBD regisers can be accessed as ...
 *    usbd->ep_in[8]
 *    usbd->epint
 *    usbd->udc_ep[3]
 */
#define usbd	((volatile struct usbd_reg_map_s *)USBD_BASE)

/* Register bit fields */
/* DEVICE CONFIGURATION REGISTER: usbd->devcfg */
/* SPD */
#define USBD_CFG_SPD_HS		0x00000000 /* PHY clock is 30/60 MHz */
#define USBD_CFG_SPD_FS		0x00000001 /* PHY clock is 30/60 MHz */
#define USBD_CFG_SPD_LS		0x00000002 /* PHY clock is 6 MHz */
#define USBD_CFG_SPD_FS48	0x00000003 /* PHY clock is 48 MHz */
/* RWKP */
#define USBD_CFG_REMOTE_WKUP	0x00000004 /* REMOTE WAKEUP */
/* SP */
#define USBD_CFG_SELF_POWER	0x00000008
/* SS */
#define USBD_CFG_SUPPORT_SYNC	0x00000010 /* SUPPORT SYNC FRAME */
/* PI */
#define USBD_CFG_PHY_8BIT	0x00000020 /* UTMI PHY SUPPORT 8 BIT I/F  */
/* DIR */
#define USBD_CFG_BIDIRECTIONAL	0x00000040 /* UTMI DATA BUS Bidirectional */
/* STATUS|STATUS_1 : handshake response option during control cmd's status-out.
 * p99 table5-16 bits 9-15 reserved for udc11-ahb subsytem
 */
#define USBD_CFG_HANDSHAKE_RES	0x00000180
/* STATUS */
#define USBD_CFG_HALT_RES_ACK	0x00000000 /* RESPONSE WITH ACK WHEN HOST */
#define USBD_CFG_HALT_RES_STALL	0x00010000 /* ISSUED CLEAR_FEATURE */
/* CSR_PRG */
#define USBD_CFG_UDCCSR_PROGRAM 0x00020000 /* UDC CSR DYNAMIC PROGRAMMING */
/* UDC supports set descriptor, it will be passed up to application
 * Return nak if not set
 */
#define USBD_CFG_SET_DESC	0x00040000
/* DDR - Double data rate */
#define USBD_CFG_DDR		0x00080000 /* RESERVED FOR UDC11-AHB SUBSYSTEM*/
/* LPM_AUTO */
#define USBD_CFG_LPM_AUTO	0x00100000 /* LPM mode auto response */
/* LPM_EN */
#define USBD_CFG_LPM_EN		0x00200000 /* LPM mode enable */

/* DEVICE CONTROL REGISTER: usbd->devctrl */
/* RES */
#define USBD_CTRL_RESUME	0x00000001 /* RESUME SIGNALING ON USB */
/* RDE */
#define USBD_CTRL_RXDMA_EN	0x00000004 /* RX DMA ENABLE */
/* TDE */
#define USBD_CTRL_TXDMA_EN	0x00000008 /* TX DMA ENABLE */
/* DU */
#define USBD_CTRL_DESC_UPDATE	0x00000010 /* DMA UPDATES DESCRIPTOR  */
/* BE */
#define USBD_CTRL_BIG_ENDIAN	0x00000020
#define USBD_CTRL_LITTLE_ENDIAN	0x00000000
/* BF */
#define USBD_CTRL_BUFF_FILL	0x00000040 /* DMA BUFFER FILL MODE */
/* THE */
#define USBD_CTRL_THRESHOLD_EN	0x00000080 /* DMA THRESHOLD ENABLE */
/* BREN */
#define USBD_CTRL_BURST_EN	0x00000100 /* DMA BURST ENABLE */
/* MODE */
#define USBD_CTRL_MODE_DMA	0x00000200
#define USBD_CTRL_MODE_SLAVE	0x00000000
/* SD */
#define USBD_CTRL_SOFT_DISC	0x00000400 /* SOFT DISCONNECT, UDC20 */
/* SCALE */
#define USBD_CTRL_SCALE_DOWN	0x00000800 /* FOR SIMULATION USE */
/* DEVNAK */
#define USBD_CTRL_DEVICE_NAK	0x00001000 /* RETURNS NAK TO ALL OUT ENDP */
/* CSR_DONE */
#define USBD_CTRL_UDCCSR_DONE	0x00002000 /* FINISHED ALL UDCCSR PROGRAMMING */
/* SRX_FLUSH */
#define USBD_CTRL_SRX_FLUSH	0x00004000
/* BRLEN */
#define USBD_CTRL_BURSTLEN_MASK	0x00FF0000 /* DMA BURST LENGTH, IN WORDS  */
/* THLEN */
#define USBD_CTRL_THRSHLEN_MASK	0xFF000000 /* DMA THRESHOLD LENGTH, IN WORDS*/

/* DEVICE STATUS REGISTER: usbd->devstatus */
/* CFG: THE CONFIGURATION SET_CONFIG CMD */
#define USBD_STS_CONFIG_MASK	0x0000000F
/* INTF: THE INTERFACE SET BY THE SET_INTERFACE CMD */
#define USBD_STS_INTERFACE_MASK	0x000000F0
/* ALT: THE ALTERNATE SETTING, TO WHICH THE INTERFACE IS SWITCHED */
#define USBD_STS_ALTERNATE_MASK	0x00000F00
/* SUSP */
#define USBD_STS_SUSPENDED	0x00001000
/* ENUM_SPD */
#define USBD_STS_ENUM_SPEED	0x00006000 /* ENUMERATED SPEED */
/* RXFIFO_EMPTY */
#define USBD_STS_RXFIFO_EMPTY	0x00008000
/* PHY_ERROR */
#define USBD_STS_PHY_ERR	0x00010000 /* RESERVED FOR UDC11-AHB */
/* TS: MASK FOR FRAME NUM (OF RECEIVED SOF) */
#define USBD_STS_FRAMENUMBER	0xFFFC0000

/* DEVICE INTERRUPT REGISTER: usbd->devint */
/* SC */
#define USBD_INT_SET_CONFIG	0x00000001 /* RECEIVED SET_CONFIG CMD */
/* SI */
#define USBD_INT_SET_INTERFACE	0x00000002 /* RECEIVED SET_INTERFACE CMD */
/* ES */
#define USBD_INT_IDLE		0x00000004 /* IDLE STATE DETECTED */
/* UR */
#define USBD_INT_RESET		0x00000008 /* RESET DETECTED */
/* US */
#define USBD_INT_SUSPEND	0x00000010 /* SUSPEND DETECTED */
/* SOF */
#define USBD_INT_SOF		0x00000020 /* SOF TOKEN DETECTED */
/* ENUM */
#define USBD_INT_ENUM_SPEED	0x00000040 /* ENUM SPEED COMPLETE */
/* RMTWKP */
#define USBD_INT_RMTWKP		0x00000080 /* REMOTE WAKEUP */
/* LPM_TKN */
#define USBD_INT_LPM_TKN	0x00000100 /* LPM RECEIVED */
/* SLEEP */
#define USBD_INT_SLEEP		0x00000200 /* ENTER LPM SLEEP */
/* EARLY_SLEEP */
#define USBD_INT_EARLY_SLEEP	0x00000400 /* ENTER EARLY LPM SLEEP */
/* R */
#define USBD_INT_RESERVED	0xFFFFFF80

/* DEVICE ENDPOINT INTERRUPT REGISTER: usbd->epint */
/* Bit Map of each Endpoint in/out interrupt (mask) register */
#define USBD_EPINT_IN(N)	BIT(N)
#define USBD_EPINT_OUT(N)	BIT(USBD_NUM_IN_ENDPOINTS + N)
#define USBD_EPINT_INOUT(N)	(USBD_EPINT_IN(N) | USBD_EPINT_OUT(N))

/* ENDPOINT CONTROL REGISTER: usbd->ep_xx[i].control (where xx = in / out) */
/* S: NEED TO CHECK RXFIFO EMPTINESS BEFORE SET THIS */
#define USBD_EPCTRL_STALL	0x00000001
/* F */
#define USBD_EPCTRL_FLUSHTXFIFO	0x00000002
/* SN: FOR OUT ENDP, SUBSYSTEM DOES NOT CHECK CORRECTNESS */
#define USBD_EPCTRL_SNOOP	0x00000004
/* P */
#define USBD_EPCTRL_POLL_DEMAND	0x00000008 /* FOR IN ENDP */
/* ET: End point types */
#define USBD_EPCTRL_TYPE_SHIFT	4
#define USBD_EPCTRL_TYPE_MASK	0x30
#define USBD_EPCTRL_TYPE_CTRL	0x00000000
#define USBD_EPCTRL_TYPE_ISOC	0x00000010
#define USBD_EPCTRL_TYPE_BULK	0x00000020
#define USBD_EPCTRL_TYPE_INTR	0x00000030
/* NAK */
#define USBD_EPCTRL_NAK		0x00000040 /* ENDP RESPONDS NAK */
/* SNAK */
#define USBD_EPCTRL_SET_NAK	0x00000080 /* SET THE NAK BIT */
/* CNAK */
#define USBD_EPCTRL_CLEAR_NAK	0x00000100 /* CLEAR THE NAK BIT */
/* PRDY */
#define USBD_EPCTRL_RX_READY	0x00000200 /* FOR DMA */
/* MRX_FLUSH */
#define USBD_EPCTRL_MRX_FLUSH	0x00001000 /* Rx fifo flush */

/* ENDPOINT STATUS REGISTER: usbd->ep_xx[i].status (where xx = in / out) */
/* OUT */
#define USBD_EPSTS_OUT_MASK	0x00000030 /* MASK FOR OUT PACKET TYPE */
#define USBD_EPSTS_OUT_DATA	0x00000010 /* OUT RECEIVED DATA */
#define USBD_EPSTS_OUT_SETUP	0x00000020 /* OUT RECEIVED SETUP (8 BYTES) */
/* IN */
#define USBD_EPSTS_IN		0x00000040 /* IN TOKEN RECEIVED */
/* BNA */
#define USBD_EPSTS_BUFNOTAVAIL	0x00000080 /* DMA BUFFER NOT AVAILABLE */
/* MRXFIFO_EMPTY */
#define USBD_EPSTS_RXFIFO_EMPTY	0x00000100 /* Rx Fifo empty */
/* HE */
#define USBD_EPSTS_AHB_ERR	0x00000200
/* TDC */
#define USBD_EPSTS_TX_DMA_DONE	0x00000400 /* TX DMA COMPLETION */
/* RX_PKT_SIZE */
#define USBD_EPSTS_RXPKT_SIZE_MASK	0x007FF800
#define USBD_EPSTS_RXPKT_SIZE_SHIFT	11
/* XFERDONE_TXEMPTY */
#define USBD_EPSTS_TX_FIFO_EMPTY	0x08000000
/* TXEMPTY */
#define USBD_EPSTS_TX_DONE_FIFO_EMPTY	0x01000000

/* UDC20 ENDPOINT REGISTER: usbd->udc_ep[i] */
#define USBD_UDCEP_NUMBER_MASK		0x0000000F
#define USBD_UDCEP_DIR_IN		0x00000010
#define USBD_UDCEP_DIR_OUT		0x00000000
#define USBD_UDCEP_TYPE_SHIFT		5
#define USBD_UDCEP_TYPE_CONTROL		0x00000000
#define USBD_UDCEP_TYPE_ISOCHRONOUS	0x00000020
#define USBD_UDCEP_TYPE_BULK		0x00000040
#define USBD_UDCEP_TYPE_INTERRUPT	0x00000060
/* [10:7], CONFIGURATION THIS ENDP BELONGS */
#define USBD_UDCEP_CONFIG_MASK		0x00000780
#define USBD_UDCEP_CONFIG_SHIFT		7
/* [14:11], INTERFACE THIS ENDP BELONGS */
#define USBD_UDCEP_INTERFACE_MASK	0x00007800
#define USBD_UDCEP_INTERFACE_SHIFT	11
/* [18:15], ALTERNATE SETTING THIS ENDP BELONGS */
#define USBD_UDCEP_ALTERNATE_MASK	0x00078000
#define USBD_UDCEP_ALTERNATE_SHIFT	15
/* [31:29], MAX PACKET SIZE */
#define USBD_UDCEP_MAX_PKTSIZE_MASK	0x1FF80000
#define USBD_UDCEP_MAX_PKTSIZE_SHIFT	19

/* Synopsys UDC DMA Data Structures */
struct usbd_dma_setup_desc {
	volatile u32_t status;	/* Status Quadlet */
	u32_t reserved;	/* Reserved */
	volatile u32_t data[2];	/* First 8 bytes of the SETUP data */
};

struct usbd_dma_data_desc {
	volatile u32_t status;	/* Status Quadlet */
	u32_t reserved;	/* Reserved */
	u8_t* volatile buffer;	/* Buffer pointer */
	/* Pointer to next DMA descriptor */
	struct usbd_dma_data_desc* volatile next;
};

#define USBD_DMADESC_BUFSTAT_MASK	0xC0000000
#define USBD_DMADESC_BUFSTAT_HOST_READY	0x00000000
#define USBD_DMADESC_BUFSTAT_DMA_BUSY	0x40000000
#define USBD_DMADESC_BUFSTAT_DMA_DONE	0x80000000
#define USBD_DMADESC_BUFSTAT_HOST_BUSY	0xC0000000
#define USBD_DMADESC_RXTXSTAT_MASK	0x30000000
#define USBD_DMADESC_RXTXSTAT_SUCCESS	0x00000000
#define USBD_DMADESC_RXTXSTAT_DESERR	0x10000000
#define USBD_DMADESC_RXTXSTAT_RESERVED	0x20000000
#define USBD_DMADESC_RXTXSTAT_BUFERR	0x30000000
/* SPECIAL FOR SETUP STRUCTURE */
/* [27:24] CONFIGURATION NUMBER */
#define USBD_DMADESC_CONFIG_MASK	0x0F000000
/* [23:20] INTERFACE     NUMBER */
#define USBD_DMADESC_INTERFACE_MASK	0x00F00000
/* [19:16] ALTERNATE SETTING NUMBER */
#define USBD_DMADESC_ALTERNATE_MASK	0x000F0000
/* FOR IN/OUT DMA DESCRIPTORS */
/* [27] THE LAST IN THE CHAIN */
#define USBD_DMADESC_LAST		0x08000000
/* [26:16] OUT: CURRENT ISO-OUT THAT RX, IN: IN WHICH CURRENT PKT MUST TX */
#define USBD_DMADESC_FRAME_NUMBER	0x07FF0000
/* [15:0] OUT ONLY */
#define USBD_DMADESC_RX_BYTES		0x0000FFFF
/* [15:14] IN-ISOCHRONOUS ONLY */
#define USBD_DMADESC_PID		0x0000C000
/* [13:0] IN ONLY 16K */
#define USBD_DMADESC_TX_BYTES		0x00003FFF

/* USBD PHY control register fields */
#define CDRU_USBPHY_D_CTRL2__ldo_pwrdwnb	(0)
#define CDRU_USBPHY_D_CTRL2__afeldocntlen	(1)
#define CRMU_USBPHY_D_CTRL__phy_iso		(20)
#define CDRU_USBPHY_D_CTRL2__iddq		(10)
#define CDRU_USBPHY_D_CTRL1__resetb		(2)
#define CDRU_USBPHY_D_CTRL1__pll_suspend_en	(27)
#define CDRU_USBPHY_D_CTRL1__ka_R		(16)
#define CDRU_USBPHY_D_CTRL1__ki_R		(19)
#define CDRU_USBPHY_D_CTRL1__kp_R		(22)
#define CDRU_USBPHY_D_CTRL1__ndiv_int_R		(6)
#define CDRU_USBPHY_D_CTRL1__pdiv_R		(3)
#define CDRU_USBPHY_D_CTRL1__pll_resetb		(26)
#define CDRU_USBPHY_D_P1CTRL__soft_resetb	(1)
#define CDRU_USBPHY_D_STATUS__pll_lock		(0)
#define CDRU_USBPHY_D_P1CTRL__afe_non_driving	(0)
#define CDRU_SW_RESET_CTRL__usbd_sw_reset_n	(24)

#endif /* __USB_REGS_H__ */
