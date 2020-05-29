/******************************************************************************
 *	Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *	and/or its subsidiaries.
 *
 *	This program is the proprietary software of Broadcom and/or its licensors,
 *	and may only be used, duplicated, modified or distributed pursuant to the
 *	terms and conditions of a separate, written license agreement executed
 *	between you and Broadcom (an "Authorized License").  Except as set forth in
 *	an Authorized License, Broadcom grants no license (express or implied),
 *	right to use, or waiver of any kind with respect to the Software, and
 *	Broadcom expressly reserves all rights in and to the Software and all
 *	intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *	THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *	IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *	Except as expressly set forth in the Authorized License,
 *
 *	1.	   This program, including its structure, sequence and organization,
 *	constitutes the valuable trade secrets of Broadcom, and you shall use all
 *	reasonable efforts to protect the confidentiality thereof, and to use this
 *	information only in connection with your use of Broadcom integrated circuit
 *	products.
 *
 *	2.	   TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *	"AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *	OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *	RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *	IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *	PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *	ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *	ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *	3.	   TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *	ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *	INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *	RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *	HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *	EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *	WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *	FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/* @file usbregisters.h
 *
 * This file contains the USBD hw block register mappings
 *
 *
 */

#ifndef __USB_REGISTERS_H
#define __USB_REGISTERS_H

#include "phx.h"
#include"usbdesc.h"
#include "socregs.h"

/*********************** USBD Block Base address ************************/
#define MMAP_USB_DEVICE_BASE       ENDPNT_IN_CTRL_0

/*********************** 5890 Hardware related **************************/
#define PHX_USB_ENDP_NE     9  /* NUMBER OF ENDP IN UDC */
#define PHX_USB_CONFIG_NC   1  /* NUMBER OF CONFIGURATION IN UDC */
#define PHX_USB_STRING_NS   0  /* NUMBER OF STRING IN UDC */
#define PHX_USB_ENDP_MAXSIZE 64 /* This value given by roger, in bytes. also refer to usb_device_test_1.c */
// #define PHX_USB_ENDP_MAXSIZE 0x200
#define PHX_USB_ENDP_MAXNUM  4  /* the max number of endp: endp0, endp1(in/out), etc. */

/* fifo size, in words, each entry is 36 bits (4 word plus 4 bits of byte enable, 1 bit for each byte) */

#define PHX_USB_RXFIFO_DEPTH_EACH_ENDP   16
#define PHX_USB_RXFIFO_DEPTH_TOTAL       80 /* in words, 5 OUT ENDP */
#define PHX_USB_TXFIFO_DEPTH_ENDP0       16
#define PHX_USB_TXFIFO_DEPTH_OTHER_ENDP  32 /* 4 such ENDP */
#define PHX_USB_TXFIFO_DEPTH_TOTAL      144 /* 5 IN ENDP IN TOTAL */

/************************ Synopsys CSR REGISTER OFFSETS **************************/
#define USB_UDCAHB_REG_OFFSET 0
#define USB_UDC_REG_OFFSET    0x500
#define USB_RXFIFO_OFFSET     0x800
#define USB_TXFIFO_OFFSET     (USB_RXFIFO_OFFSET + PHX_USB_RXFIFO_DEPTH_TOTAL)
/************************ Synopsys UDC-AHB CSR REGISTERS **************************/

#define USB_UDCAHB_REG_BASE           (volatile uint8_t *)(MMAP_USB_DEVICE_BASE + USB_UDCAHB_REG_OFFSET)

/* registers for every endp, synopsys defined up to n=15, 5890 has n=4 */
#define USB_UDCAHB_IN_ENDP_CONTROL(n)      ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x00))
#define USB_UDCAHB_IN_ENDP_STATUS(n)       ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x04))
#define USB_UDCAHB_IN_ENDP_BUFSIZE(n)      ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x08))
#define USB_UDCAHB_IN_ENDP_MAXPKTSIZE(n)   ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x0C))
#define USB_UDCAHB_IN_ENDP_DESCRIPTOR(n)   ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x14))
#define USB_UDCAHB_IN_ENDP_WRCONFIRM(n)    ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x1C))
#define USB_UDCAHB_OUT_ENDP_CONTROL(n)     ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x200))
#define USB_UDCAHB_OUT_ENDP_STATUS(n)      ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x204))
#define USB_UDCAHB_OUT_ENDP_FRAMENUMBER(n) ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x208))
#define USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(n)  ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x20C))
#define USB_UDCAHB_OUT_ENDP_SETUPBUF(n)    ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x210))
#define USB_UDCAHB_OUT_ENDP_DESCRIPTOR(n)  ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x214))
#define USB_UDCAHB_OUT_ENDP_RDCONFIRM(n)   ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + n*0x20 + 0x21C))

/* global registers */
#define USB_UDCAHB_DEVICE_CONFIG           ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x400))
#define USB_UDCAHB_DEVICE_CONTROL          ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x404))
#define USB_UDCAHB_DEVICE_STATUS           ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x408))
#define USB_UDCAHB_DEVICE_INT              ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x40C))
#define USB_UDCAHB_DEVICE_INTMASK          ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x410))
#define USB_UDCAHB_ENDP_INT                ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x414))
#define USB_UDCAHB_ENDP_INTMASK            ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x418))
#define USB_UDCAHB_TESTMODE                ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x41c))
#define USB_UDCAHB_RELEASE_NUMBER          ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x420))
#define USB_UDCAHB_LPM_CTRL_STATUS         ((volatile uint32_t *)(USB_UDCAHB_REG_BASE + 0x424))

/* device configuration register */
//SPD
#define USB_DEVICE_CONFIG_SPD_HS         0x00000000 /* PHY clock is 30 or 60 Mhz */
#define USB_DEVICE_CONFIG_SPD_FS         0x00000001 /* PHY clock is 30 or 60 Mhz */
#define USB_DEVICE_CONFIG_SPD_LS         0x00000002 /* PHY clock is 6 Mhz */
#define USB_DEVICE_CONFIG_SPD_FS48       0x00000003 /* PHY clock is 48 Mhz */
// RWKP
#define USB_DEVICE_CONFIG_REMOTE_WKUP    0x00000004 /* REMOTE WAKEUP */
// SP
#define USB_DEVICE_CONFIG_SELF_POWER     0x00000008
//SS
#define USB_DEVICE_CONFIG_SUPPORT_SYNC   0x00000010 /* SUPPORT SYNC FRAME */
//PI
#define USB_DEVICE_CONFIG_PHY_8BIT       0x00000020 /* UTMI PHY SUPPORT 8 BIT INTERFACE */
//DIR
#define USB_DEVICE_CONFIG_BIDIRECTIONAL  0x00000040 /* UTMI DATA BUS SUPPORT BIDIRECTIONAL */
//??
#define USB_DEVICE_CONFIG_HANDSHAKE_RES  0x00000180 /* HDSHAKE RESPONSE OPTION DURING CONTROL CMD'S STATUS-OUT. P99 TABLE5-16 */
                                                    /* BITS 9-15 RESERVED FOR UDC11-AHB SUBSYTEM */
//STATUS
#define USB_DEVICE_CONFIG_HALT_RES_ACK   0x00000000     /* RESPONSE WITH ACK WHEN HOST ISSUED CLEAR_FEATURE */
#define USB_DEVICE_CONFIG_HALT_RES_STALL 0x00010000   
// Unknown to HT
#define USB_DEVICE_CONFIG_UDCCSR_PROGRAM 0x00020000 /* UDC CSR DYNAMIC PROGRAMMING SUPPORTED */
#define USB_DEVICE_CONFIG_SET_DESC       0x00040000 /* UDC SUPPORTS SET DESCRIPTOR, IT WILL BE PASSED UP TO APPLICATION. RETURN NAK IF NOT SET */
#define USB_DEVICE_CONFIG_DDR            0x00080000 /* RESERVED FOR UDC11-AHB SUBSYSTEM */
#define USB_DEVICE_CONFIG_LPM_AUTO       0x00100000 /* LPM mode auto response */
#define USB_DEVICE_CONFIG_LPM_EN         0x00200000 /* LPM mode enable */

/* device control register */

//RES
#define USB_DEVICE_CONTROL_RESUME        0x00000001 /* RESUME SIGNALING ON USB */
//RDE
#define USB_DEVICE_CONTROL_RXDMA_EN      0x00000004 /* RX DAM ENABLE           */
//TDE
#define USB_DEVICE_CONTROL_TXDMA_EN      0x00000008 /* TX DMA ENABLE           */
//DU
#define USB_DEVICE_CONTROL_DESC_UPDATE   0x00000010 /* DMA UPDATES DESCRIPTOR  */
//BE
#define USB_DEVICE_CONTROL_BIGENDIAN     0x00000020
#define USB_DEVICE_CONTROL_LITTLEENDIAN  0x00000000

//BF 
#define USB_DEVICE_CONTROL_BUFF_FILL     0x00000040 /* DMA BUFFER FILL MODE    */
//THE
#define USB_DEVICE_CONTROL_THRESHOLD_EN  0x00000080 /* DMA THRESHOLD ENABLE    */
//BREN
#define USB_DEVICE_CONTROL_BURST_EN      0x00000100 /* DMA BURST ENABLE        */
//MODE
#define USB_DEVICE_CONTROL_MODE_DMA      0x00000200
#define USB_DEVICE_CONTROL_MODE_SLAVE    0x00000000

//SD
#define USB_DEVICE_CONTROL_SOFT_DISC     0x00000400 /* SOFT DISCONNECT, UDC20  */

//Unknown to HT
#define USB_DEVICE_CONTROL_SCALE_DOWN    0x00000800 /* FOR SIMULATION USE      */
#define USB_DEVICE_CONTROL_DEVICE_NAK    0x00001000 /* RETURNS NAK TO ALL OUT ENDP */
#define USB_DEVICE_CONTROL_UDCCSR_DONE   0x00002000 /* FINISHED ALL UDCCSR PROGRAMMING */

//BRLEN
#define USB_DEVICE_CONTROL_BURSTLEN_MASK 0x00FF0000 /* DMA BURST LENGTH, IN WORDS  */
//THLEN
#define USB_DEVICE_CONTROL_THRSHLEN_MASK 0xFF000000 /* DMA THRESHOLD LENGTH, IN WORDS*/



/* device status register */

//CFG
#define USB_DEVICE_STATUS_CONFIG_MASK    0x0000000F /* THE CONFIGURATION SET BY THE SET_CONFIG CMD */
//INTF
#define USB_DEVICE_STATUS_INTERFACE_MASK 0x000000F0 /* THE INTERFACE SET BY THE SET_INTERFACE CMD */
//ALT
#define USB_DEVICE_STATUS_ALTERNATE_MASK 0x00000F00 /* THE ALTERNATE SETTING, TO WHICH THE INTERFACE IS SWITHED */
//SUSP
#define USB_DEVICE_STATUS_SUSPENDED      0x00001000
//ENUM SPD
#define USB_DEVICE_STATUS_ENUM_SPEED     0x00006000 /* ENUMERATED SPEED */
//RXFIFO EMPTY
#define USB_DEVICE_STATUS_RXFIFO_EMPTY   0x00008000
//PHY ERROR
#define USB_DEVICE_STATUS_PHY_ERR        0x00010000 /* RESERVED FOR UDC11-AHB */
//TS
#define USB_DEVICE_STATUS_FRAMENUMBER    0xFFFC0000 /* MASK FOR FRAME NUMBER OF THE RECEIVED SOF */



/* device interrupt register */

//SC
#define USB_DEVICE_INT_SET_CONFIG        0x00000001 /* RECEIVED SET_CONFIG CMD    */
//SI
#define USB_DEVICE_INT_SET_INTERFACE     0x00000002 /* RECEIVED SET_INTERFACE CMD */
//ES
#define USB_DEVICE_INT_IDLE              0x00000004 /* IDLE STATE DETECTED        */
//UR
#define USB_DEVICE_INT_RESET             0x00000008 /* RESET DETECTED             */
//US
#define USB_DEVICE_INT_SUSPEND           0x00000010 /* SUSPEND DETECTED           */
//SOF
#define USB_DEVICE_INT_SOF               0x00000020 /* SOF TOKEN DETECTED         */
//ENUM
#define USB_DEVICE_INT_ENUM_SPEED        0x00000040 /* ENUM SPEED COMPLETE        */
//RMTWKP
#define USB_DEVICE_INT_RMTWKP            0x00000080 /* REMOTE WAKEUP              */
//LPM_TKN
#define USB_DEVICE_INT_LPM_TKN           0x00000100 /* LPM RECEIVED               */
//SLEEP
#define USB_DEVICE_INT_SLEEP             0x00000200 /* ENTER LPM SLEEP            */
//EARLY_SLEEP
#define USB_DEVICE_INT_EARLY_SLEEP       0x00000400 /* ENTER EARLY LPM SLEEP            */
//R
#define USB_DEVICE_INT_RESERVED          0xFFFFFF80


/* device endp interrupt register */

// Bit Map of each Endpoint in/out interrupt (mask) register
#define USB_DEVICE_ENDP_INT_IN(n)          (1<<(n))
#define USB_DEVICE_ENDP_INT_OUT(n)       (1<<(16+n))
#define USB_DEVICE_ENDP_INT_INOUT(n)     ((1<<n)|(1<<(16+n))) 

/* Device test mode register */
#define USB_DEVICE_TSTMODE		1

/* endpoint specific register --- control register */
//S
#define USB_ENDP_CONTROL_STALL            0x00000001 /* NEED TO CHECK RXFIFO EMPTINESS BEFORE SET THIS */
//F
#define USB_ENDP_CONTROL_FLUSH_TXFIFO     0x00000002
//SN
#define USB_ENDP_CONTROL_SNOOP            0x00000004 /* FOR OUT ENDP, SUBSYSTEM DOES NOT CHECK CORRECTNESS */
//P
#define USB_ENDP_CONTROL_POLL_DEMAND      0x00000008 /* FOR IN ENDP */
//ET
#define USB_ENDP_CONTROL_TYPE_CONTROL     0x00000000
#define USB_ENDP_CONTROL_TYPE_ISOCHRONOUS 0x00000010
#define USB_ENDP_CONTROL_TYPE_BULK        0x00000020
#define USB_ENDP_CONTROL_TYPE_INTERRUPT   0x00000030
//NAK
#define USB_ENDP_CONTROL_NAK              0x00000040 /* ENDP RESPONDS NAK */
//SNAK
#define USB_ENDP_CONTROL_SET_NAK          0x00000080 /* SET THE NAK BIT */
//CNAK
#define USB_ENDP_CONTROL_CLEAR_NAK        0x00000100 /* CLEAR THE NAK BIT */
//PRDY
#define USB_ENDP_CONTROL_RX_READY         0x00000200 /* FOR DMA */



/* endpoint specific register --- status register */
//OUT
#define USB_ENDP_STATUS_OUT_MASK          0x00000030 /* MASK FOR OUT PACKET TYPE */
#define USB_ENDP_STATUS_OUT_DATA          0x00000010 /* OUT RECEIVED DATA */
#define USB_ENDP_STATUS_OUT_SETUP         0x00000020 /* OUT RECEIVED SETUP (8 BYTES) */
//IN
#define USB_ENDP_STATUS_IN                0x00000040 /* IN TOKEN RECEIVED */
//BNA
#define USB_ENDP_STATUS_BUF_NOTAVAIL      0x00000080 /* DMA BUFFER NOT AVAILABLE */
//HE
#define USB_ENDP_STATUS_AHB_ERR           0x00000200
//TDC
#define USB_ENDP_STATUS_TX_DMA_DONE       0x00000400 /* TX DMA COMPLETION */
//RX PKT SIZE
#define USB_ENDP_STATUS_RXPKT_SIZE_MASK   0x007FF800
#define USB_ENDP_STATUS_RXPKT_SIZE_SHIFT  11

/************************   Synopsys UDC CSR REGISTERS  **************************/

#define USB_UDC_REG_BASE              (volatile uint8_t *)(MMAP_USB_DEVICE_BASE + USB_UDC_REG_OFFSET)
#define USB_UDC_ENDP_BASE             (volatile uint8_t *)(USB_UDC_REG_BASE    + 0x04)
#define USB_UDC_CONFIG_BASE           (volatile uint8_t *)(USB_UDC_ENDP_BASE   + 0x04 * PHX_USB_ENDP_NE)
#define USB_UDC_STRING_BASE           (volatile uint8_t *)(USB_UDC_CONFIG_BASE + 0x04 * PHX_USB_CONFIG_NC)


#define USB_UDC_DEVICE_DESCRIPTOR     ((volatile uint32_t *)(USB_UDC_REG_BASE    + 0x00))
#define USB_UDC_ENDP_NE(n)            ((volatile uint32_t *)(USB_UDC_ENDP_BASE   + n* 0x04)) /* n=0 to 8 */
#define USB_UDC_CONFIG_NC(n)          ((volatile uint32_t *)(USB_UDC_CONFIG_BASE + n* 0x04))
#define USB_UDC_STRING_NS(n)          ((volatile uint32_t *)(USB_UDC_STRING_BASE + n* 0x04))


/* endpoint NE register */

#define USB_UDC_ENDP_NUMBER_MASK       0x0000000F
#define USB_UDC_ENDP_DIR_IN            0x00000010
#define USB_UDC_ENDP_DIR_OUT           0x00000000
#define USB_UDC_ENDP_TYPE_CONTROL      0x00000000
#define USB_UDC_ENDP_TYPE_ISOCHRONOUS  0x00000020
#define USB_UDC_ENDP_TYPE_BULK         0x00000040
#define USB_UDC_ENDP_TYPE_INTERRUPT    0x00000060
#define USB_UDC_ENDP_CONFIG_MASK       0x00000780 /* [10:7], CONFIGURATION THIS ENDP BELONGS */
#define USB_UDC_ENDP_CONFIG_SHIFT      7
#define USB_UDC_ENDP_INTERFACE_MASK    0x00007800 /* [14:11], INTERFACE THIS ENDP BELONGS */
#define USB_UDC_ENDP_INTERFACE_SHIFT   11
#define USB_UDC_ENDP_ALTERNATE_MASK    0x00078000 /* [18:15], ALTERNATE SETTING THIS ENDP BELONGS */
#define USB_UDC_ENDP_ALTERNATE_SHIFT   15
#define USB_UDC_ENDP_MAX_PKTSIZE_MASK  0x1FF80000 /* [31:29], MAX PACKET SIZE */
#define USB_UDC_ENDP_MAX_PKTSIZE_SHIFT 19





/************************   Synopsys UDC DMA Data Structures  **************************/


#define USB_DMA_DESC_BUFSTAT_MASK          0xC0000000
#define USB_DMA_DESC_BUFSTAT_HOST_READY    0x00000000
#define USB_DMA_DESC_BUFSTAT_DMA_BUSY      0x60000000
#define USB_DMA_DESC_BUFSTAT_DMA_DONE      0x80000000
#define USB_DMA_DESC_BUFSTAT_HOST_BUSY     0xC0000000
#define USB_DMA_DESC_RXTXSTAT_MASK         0x30000000
#define USB_DMA_DESC_RXTXSTAT_SUCCESS      0x00000000
#define USB_DMA_DESC_RXTXSTAT_DESERR       0x10000000
#define USB_DMA_DESC_RXTXSTAT_RESERVED     0x20000000
#define USB_DMA_DESC_RXTXSTAT_BUFERR       0x30000000
/* SPECIAL FOR SETUP STRUCTURE */
#define USB_DMA_DESC_CONFIG_MASK           0x0F000000 /* [27:24] CONFIGURATION NUMBER */
#define USB_DMA_DESC_INTERFACE_MASK        0x00F00000 /* [23:20] INTERFACE     NUMBER */
#define USB_DMA_DESC_ALTERNATE_MASK        0x000F0000 /* [19:16] ALTERNATE SETTING NUMBER */
/* FOR IN/OUT DMA DESCRIPTORS */
#define USB_DMA_DESC_LAST                  0x08000000 /* [27] THE LAST IN THE CHAIN */
#define USB_DMA_DESC_FRAME_NUMBER          0x07FF0000 /* [26:16] OUT: CURRENT ISO-OUT THAT RX, IN: IN WHICH CURRENT PKT MUST TX */
#define USB_DMA_DESC_RX_BYTES              0x0000FFFF /* [15:0] OUT ONLY */
#define USB_DMA_DESC_PID                   0x0000C000 /* [15:14] IN-ISOCHRONOUS ONLY */
#define USB_DMA_DESC_TX_BYTES              0x00003FFF /* [13:0] IN ONLY 16K*/ 


#define USB_DMA_DESC_SIZE             (sizeof(usb_dma_desc))
#define USB_DMA_SETUP_SIZE            (sizeof(usb_dma_setup))
#define USB_DMA_DESC_SET_READY(pDesc) 	((pDesc)->dwStatus = (((pDesc)->dwStatus & ~USB_DMA_DESC_BUFSTAT_MASK) | USB_DMA_DESC_BUFSTAT_HOST_READY))


/************************   5890 USB DMA Structures  **************************/


/*  one buffer for control and one for bulk */
#define USB_CONTROL_BUFNUM	0
#define USB_MASS_STORAGE_BUFNUM	1
#define USB_BUFNUM_IN	3
#define USB_BUFNUM_OUT	2
#define USB_MAX_DMA_BUF_LEN 	PHX_USB_ENDP_MAXSIZE
#define USB_DMA_DESC_TYPE_IN    0x0001
#define USB_DMA_DESC_TYPE_OUT   0x0002
#define USB_DMA_DESC_TYPE_SETUP 0x0003

#define MAX_READ_SIZE PHX_USB_ENDP_MAXSIZE
#define NUM_STRING_DESCRIPTORS	4



#ifdef DONT_RESTART_TIMER
#define RESTART_TIMER(val)  if(USB_CTX_GET(usb_use_master_timeout) == 0) {	\
	restart_timer(val);	\
	}
#define RESTART_TIMER_DATA(val)	if(USB_CTX_GET(usb_use_master_timeout) == 0) {	\
	restart_timer(val); \
}
#else
#define RESTART_TIMER(val)	restart_timer(val)
#define RESTART_TIMER_DATA(val)	if(USB_CTX_GET(usb_control_between_data) == 0) {	\
	restart_timer(val); \
}

#endif


#define CHECK_TIMEOUT() 	(!*((volatile uint32_t *)0x10506004))

#define CHECK_HANDLE_TIMEOUT(error) 	EXTRA_LOOP_HANDLER();	\
if(CHECK_TIMEOUT())  { \
    USB_CTX_GET(usb_glb_error) = error;	\
    return error; /* fye: remove this line to disable timeout */	\
}


/* 0x150000 is almost a minute when ref clk is 24 */

#define TEST_CLK		24
#define REF_CLK_MULTIPLE	12

//#define SIXTY_SECONDS	((0x150000/TEST_CLK) * ((USB_CTX_GET(ref_clk_setting)+1)*REF_CLK_MULTIPLE))
#define MAX_WRITE_INT_TIMEOUT		(SIXTY_SECONDS/4)
#define MAX_DMA_DONE_TIMEOUT		(SIXTY_SECONDS/4)
#define MAX_READ_INT_TIMEOUT		(SIXTY_SECONDS/4)
#define MAX_CONTROL_WAIT_TIMEOUT 	SIXTY_SECONDS
#define WAIT_AFTER_GPIO_PULLDOWN   (SIXTY_SECONDS/60)
#define WAIT_AFTER_GPIO_PULLUP      ((0x1000/TEST_CLK) * ((1+1)*REF_CLK_MULTIPLE))

#endif // __USB_UDC_H__

