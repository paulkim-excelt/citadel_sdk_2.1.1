/*
 * $Copyright Broadcom Corporation$
 *
 */
#ifndef __USB_REGISTERS_H
#define __USB_REGISTERS_H

#include <platform.h>
#include "usbdesc.h"

/*********************** 58300 Hardware related **************************/


#define PHX_USB_ENDP_NE     	9  /* NUMBER OF ENDP IN UDC */
#define PHX_USB_CONFIG_NC   	1  /* NUMBER OF CONFIGURATION IN UDC */
#define PHX_USB_STRING_NS   	0  /* NUMBER OF STRING IN UDC */
#define PHX_USB_ENDP_MAXSIZE 	64 /* This value given by roger, in bytes. also refer to usb_device_test_1.c */
#define PHX_USB_ENDP_MAXNUM  	4  /* the max number of endp: endp0, endp1(in/out), etc. */

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

#define USB_DEVICE_BASE				0x47000000
#define USB_UDCAHB_REG_BASE           		(volatile uint8_t *)(USB_DEVICE_BASE)

/* registers for every endp, synopsys defined up to n=15, 5890 has n=4 */
#define USB_UDCAHB_IN_ENDP_CONTROL(n)      	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x00))
#define USB_UDCAHB_IN_ENDP_STATUS(n)       	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x04))
#define USB_UDCAHB_IN_ENDP_BUFSIZE(n)      	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x08))
#define USB_UDCAHB_IN_ENDP_MAXPKTSIZE(n)   	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x0C))
#define USB_UDCAHB_IN_ENDP_SETUPBUF(n)    	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x10))
#define USB_UDCAHB_IN_ENDP_DESCRIPTOR(n)   	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x14))
#define USB_UDCAHB_IN_ENDP_WRCONFIRM(n)    	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x1C))

#define USB_UDCAHB_OUT_ENDP_CONTROL(n)     	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x200))
#define USB_UDCAHB_OUT_ENDP_STATUS(n)      	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x204))
#define USB_UDCAHB_OUT_ENDP_FRAMENUMBER(n) 	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x208))
#define USB_UDCAHB_OUT_ENDP_MAXPKTSIZE(n)  	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x20C))
#define USB_UDCAHB_OUT_ENDP_SETUPBUF(n)    	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x210))
#define USB_UDCAHB_OUT_ENDP_DESCRIPTOR(n)  	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x214))
#define USB_UDCAHB_OUT_ENDP_RDCONFIRM(n)   	((volatile uint32_t *)(USB_DEVICE_BASE + n*0x20 + 0x21C))

/* global registers */
#define IPROC_USB2D_DEVCONFIG				((volatile uint32_t *)(USB_DEVICE_BASE + 0x400))
#define IPROC_USB2D_DEVCTRL					((volatile uint32_t *)(USB_DEVICE_BASE + 0x404))
#define IPROC_USB2D_DEVSTATUS				((volatile uint32_t *)(USB_DEVICE_BASE + 0x408))
#define IPROC_USB2D_DEVINTR					((volatile uint32_t *)(USB_DEVICE_BASE + 0x40C))
#define IPROC_USB2D_DEVINTRMASK				((volatile uint32_t *)(USB_DEVICE_BASE + 0x410))
#define IPROC_USB2D_ENDPNTINTR				((volatile uint32_t *)(USB_DEVICE_BASE + 0x414))
#define IPROC_USB2D_ENDPNTINTRMASK			((volatile uint32_t *)(USB_DEVICE_BASE + 0x418))
#define IPROC_USB2D_TESTMODE				((volatile uint32_t *)(USB_DEVICE_BASE + 0x41C))
#define IPROC_USB2D_RELEASE_NUMBER			((volatile uint32_t *)(USB_DEVICE_BASE + 0x420))
#define IPROC_USB2D_LPMCTRLSTATUS			((volatile uint32_t *)(USB_DEVICE_BASE + 0x424))

#define USB_UDCAHB_DEVICE_CONFIG           	((volatile uint32_t *)(USB_DEVICE_BASE + 0x400))
#define USB_UDCAHB_DEVICE_CONTROL          	((volatile uint32_t *)(USB_DEVICE_BASE + 0x404))
#define USB_UDCAHB_DEVICE_STATUS           	((volatile uint32_t *)(USB_DEVICE_BASE + 0x408))
#define USB_UDCAHB_DEVICE_INT              	((volatile uint32_t *)(USB_DEVICE_BASE + 0x40C))
#define USB_UDCAHB_DEVICE_INTMASK          	((volatile uint32_t *)(USB_DEVICE_BASE + 0x410))
#define USB_UDCAHB_ENDP_INT                	((volatile uint32_t *)(USB_DEVICE_BASE + 0x414))
#define USB_UDCAHB_ENDP_INTMASK            	((volatile uint32_t *)(USB_DEVICE_BASE + 0x418))

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
//R
#define USB_DEVICE_INT_RESERVED          0xFFFFFF80


/* device endp interrupt register */

// Bit Map of each Endpoint in/out interrupt (mask) register
#define USB_DEVICE_ENDP_INT_IN(n)        (1<<(n))
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
#define IPROC_USB2D_UDC20_REG_OFFSET		0x500
#define IPROC_USB2D_UDC20_REG_BASE			(volatile uint32_t *)(USB_DEVICE_BASE + IPROC_USB2D_UDC20_REG_OFFSET)
#define IPROC_USB2D_UDC20_ENDPOINT_BASE		(volatile uint32_t *)(IPROC_USB2D_UDC20_REG_BASE + 0x04)
#define IPROC_USB2D_UDC20_CONFIG_BASE		(volatile uint32_t *)(IPROC_USB2D_UDC20_REG_BASE + 0x04 * PHX_USB_ENDP_NE)
#define IPROC_USB2D_UDC20_STRING_BASE		(volatile uint32_t *)(IPROC_USB2D_UDC20_CONFIG_BASE + 0x4 * PHX_USB_CONFIG_NC)
#define IPROC_USB2D_UDC20_DEVICE_DESCRIPTOR (volatile uint32_t *)(IPROC_USB2D_UDC20_REG_BASE + 0x00)
#define IPROC_USB2D_UDC20_ENDP_NE(n)		(volatile uint32_t *)(IPROC_USB2D_UDC20_ENDPOINT_BASE + n * 0x04)
#define IPROC_USB2D_UDC20_CONFIG_NC(n)		(volatile uint32_t *)(IPROC_USB2D_UDC20_CONFIG_BASE + n * 0x04)
#define IPROC_USB2D_UDC20_STRING_NS(n)		(volatile uint32_t *)(IPROC_USB2D_UDC20_STRING_BASE + n *0x04)


#define USB_UDC_REG_BASE              	(volatile uint8_t *)(USB_DEVICE_BASE + USB_UDC_REG_OFFSET)
#define USB_UDC_ENDP_BASE             	(volatile uint8_t *)(USB_UDC_REG_BASE    + 0x04)
#define USB_UDC_CONFIG_BASE           	(volatile uint8_t *)(USB_UDC_ENDP_BASE   + 0x04 * PHX_USB_ENDP_NE)
#define USB_UDC_STRING_BASE           	(volatile uint8_t *)(USB_UDC_CONFIG_BASE + 0x04 * PHX_USB_CONFIG_NC)

#define USB_UDC_DEVICE_DESCRIPTOR     	((volatile uint32_t *)(USB_UDC_REG_BASE    + 0x00))
#define USB_UDC_ENDP_NE(n)            	((volatile uint32_t *)(USB_UDC_ENDP_BASE   + (n)* 0x04)) /* n=0 to 8 */
#define USB_UDC_CONFIG_NC(n)          	((volatile uint32_t *)(USB_UDC_CONFIG_BASE + (n)* 0x04))
#define USB_UDC_STRING_NS(n)          	((volatile uint32_t *)(USB_UDC_STRING_BASE + (n)* 0x04))


/* endpoint NE register */

#define USB_UDC_ENDP_NUMBER_MASK       	0x0000000F
#define USB_UDC_ENDP_DIR_IN            	0x00000010
#define USB_UDC_ENDP_DIR_OUT           	0x00000000
#define USB_UDC_ENDP_TYPE_CONTROL      	0x00000000
#define USB_UDC_ENDP_TYPE_ISOCHRONOUS  	0x00000020
#define USB_UDC_ENDP_TYPE_BULK         	0x00000040
#define USB_UDC_ENDP_TYPE_INTERRUPT    	0x00000060
#define USB_UDC_ENDP_CONFIG_MASK       	0x00000780 /* [10:7], CONFIGURATION THIS ENDP BELONGS */
#define USB_UDC_ENDP_CONFIG_SHIFT      	7
#define USB_UDC_ENDP_INTERFACE_MASK   	0x00007800 /* [14:11], INTERFACE THIS ENDP BELONGS */
#define USB_UDC_ENDP_INTERFACE_SHIFT   	11
#define USB_UDC_ENDP_ALTERNATE_MASK    	0x00078000 /* [18:15], ALTERNATE SETTING THIS ENDP BELONGS */
#define USB_UDC_ENDP_ALTERNATE_SHIFT   	15
#define USB_UDC_ENDP_MAX_PKTSIZE_MASK  	0x1FF80000 /* [31:29], MAX PACKET SIZE */
#define USB_UDC_ENDP_MAX_PKTSIZE_SHIFT 	19


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
#define USB_DMA_DESC_TX_BYTES              0x00003FFF /* [13:0] IN ONLY */


#define USB_DMA_DESC_SIZE             		(sizeof(usb_dma_desc))
#define USB_DMA_SETUP_SIZE            		(sizeof(usb_dma_setup))
#define USB_DMA_DESC_SET_READY(pDesc) 		((pDesc)->dwStatus = (((pDesc)->dwStatus & ~USB_DMA_DESC_BUFSTAT_MASK) | USB_DMA_DESC_BUFSTAT_HOST_READY))


/************************   5890 USB DMA Structures  **************************/


/*  one buffer for control and one for bulk */
#define USB_CONTROL_BUFNUM			0
#define USB_MASS_STORAGE_BUFNUM		1
#define USB_BUFNUM_IN				3
#define USB_BUFNUM_OUT				2
#define USB_MAX_DMA_BUF_LEN 		PHX_USB_ENDP_MAXSIZE
#define USB_DMA_DESC_TYPE_IN    	0x0001
#define USB_DMA_DESC_TYPE_OUT   	0x0002
#define USB_DMA_DESC_TYPE_SETUP 	0x0003

#define MAX_READ_SIZE 				PHX_USB_ENDP_MAXSIZE
#define NUM_STRING_DESCRIPTORS		4

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


#define CHECK_TIMEOUT() 				(!*((volatile uint32_t *)0x10506004))

#define CHECK_HANDLE_TIMEOUT(error) 	EXTRA_LOOP_HANDLER();	\
if(CHECK_TIMEOUT())  { \
    USB_CTX_GET(usb_glb_error) = error;	\
    return error; /* fye: remove this line to disable timeout */	\
}


/* 0x150000 is almost a minute when ref clk is 24 */

#define TEST_CLK						24
#define REF_CLK_MULTIPLE				12

//#define SIXTY_SECONDS	((0x150000/TEST_CLK) * ((USB_CTX_GET(ref_clk_setting)+1)*REF_CLK_MULTIPLE))
#define MAX_WRITE_INT_TIMEOUT			(SIXTY_SECONDS/4)
#define MAX_DMA_DONE_TIMEOUT			(SIXTY_SECONDS/4)
#define MAX_READ_INT_TIMEOUT			(SIXTY_SECONDS/4)
#define MAX_CONTROL_WAIT_TIMEOUT 	 	 SIXTY_SECONDS
#define WAIT_AFTER_GPIO_PULLDOWN    	(SIXTY_SECONDS/60)
#define WAIT_AFTER_GPIO_PULLUP      	((0x1000/TEST_CLK) * ((1+1)*REF_CLK_MULTIPLE))

#define usbDevHw_REG_MULTI_RX_FIFO                          0

/* ---- Endpoint FIFO register definitions ------------------------ */

/*
 * The endpoint type field in the FIFO control register has the same enumeration
 * as the USB protocol. Not going to define it here.
 */
#define usbDevHw_REG_ENDPT_FIFO_CTRL_OUT_FLUSH_ENABLE       (1 << 12)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_OUT_CLOSE_DESC         (1 << 11)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_IN_SEND_NULL           (1 << 10)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_OUT_DMA_ENABLE         (1 << 9)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_NAK_CLEAR              (1 << 8)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_NAK_SET                (1 << 7)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_NAK_IN_PROGRESS        (1 << 6)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_TYPE_SHIFT             4
#define usbDevHw_REG_ENDPT_FIFO_CTRL_TYPE_MASK              (3 << usbDevHw_REG_ENDPT_FIFO_CTRL_TYPE_SHIFT)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_IN_DMA_ENABLE          (1 << 3)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_SNOOP_ENABLE           (1 << 2)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_IN_FLUSH_ENABLE        (1 << 1)
#define usbDevHw_REG_ENDPT_FIFO_CTRL_STALL_ENABLE           (1 << 0)


#define usbDevHw_REG_ENDPT_FIFO_STATUS_CLOSE_DESC_CLEAR     (1 << 28)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_IN_XFER_DONE         (1 << 27)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_STALL_SET_RX         (1 << 26)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_STALL_CLEAR_RX       (1 << 25)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_IN_FIFO_EMPTY        (1 << 24)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_IN_DMA_DONE          (1 << 10)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_AHB_BUS_ERROR        (1 << 9)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_OUT_FIFO_EMPTY       (1 << 8)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_DMA_BUF_NOT_AVAIL    (1 << 7)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_IN_TOKEN_RX          (1 << 6)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_OUT_DMA_SETUP_DONE   (1 << 5)
#define usbDevHw_REG_ENDPT_FIFO_STATUS_OUT_DMA_DATA_DONE    (1 << 4)


#define usbDevHw_REG_ENDPT_FIFO_SIZE1_OUT_ISOC_PID_SHIFT    16
#define usbDevHw_REG_ENDPT_FIFO_SIZE1_OUT_ISOC_PID_MASK     (3 << usbDevHw_REG_ENDPT_FIFO_SIZE1_OUT_ISOC_PID_SHIFT)
#define usbDevHw_REG_ENDPT_FIFO_SIZE1_IN_DEPTH_SHIFT        0
#define usbDevHw_REG_ENDPT_FIFO_SIZE1_IN_DEPTH_MASK         (0xffff << usbDevHw_REG_ENDPT_FIFO_SIZE1_IN_DEPTH_SHIFT)
#define usbDevHw_REG_ENDPT_FIFO_SIZE1_OUT_FRAME_NUM_SHIFT   usbDevHw_REG_ENDPT_FIFO_SIZE1_IN_DEPTH_SHIFT
#define usbDevHw_REG_ENDPT_FIFO_SIZE1_OUT_FRAME_NUM_MASK    usbDevHw_REG_ENDPT_FIFO_SIZE1_IN_DEPTH_MASK


#define usbDevHw_REG_ENDPT_FIFO_SIZE2_OUT_DEPTH_SHIFT       16
#define usbDevHw_REG_ENDPT_FIFO_SIZE2_OUT_DEPTH_MASK        (0xffff << usbDevHw_REG_ENDPT_FIFO_SIZE2_OUT_DEPTH_SHIFT)
#define usbDevHw_REG_ENDPT_FIFO_SIZE2_PKT_MAX_SHIFT         0
#define usbDevHw_REG_ENDPT_FIFO_SIZE2_PKT_MAX_MASK          (0xffff << usbDevHw_REG_ENDPT_FIFO_SIZE2_PKT_MAX_SHIFT)


/* ---- Endpoint Configuration register definitions --------------- */

/*
 * The endpoint type field in the config register has the same enumeration
 * as the USB protocol. Not going to define it here.
 */
#define usbDevHw_REG_ENDPT_CFG_PKT_MAX_SHIFT                19
#define usbDevHw_REG_ENDPT_CFG_PKT_MAX_MASK                 (0x7ff << usbDevHw_REG_ENDPT_CFG_PKT_MAX_SHIFT)
#define usbDevHw_REG_ENDPT_CFG_ALT_NUM_SHIFT                15
#define usbDevHw_REG_ENDPT_CFG_ALT_NUM_MASK                 (0xf << usbDevHw_REG_ENDPT_CFG_ALT_NUM_SHIFT)
#define usbDevHw_REG_ENDPT_CFG_INTF_NUM_SHIFT               11
#define usbDevHw_REG_ENDPT_CFG_INTF_NUM_MASK                (0xf << usbDevHw_REG_ENDPT_CFG_INTF_NUM_SHIFT)
#define usbDevHw_REG_ENDPT_CFG_CFG_NUM_SHIFT                7
#define usbDevHw_REG_ENDPT_CFG_CFG_NUM_MASK                 (0xf << usbDevHw_REG_ENDPT_CFG_CFG_NUM_SHIFT)
#define usbDevHw_REG_ENDPT_CFG_TYPE_SHIFT                   5
#define usbDevHw_REG_ENDPT_CFG_TYPE_MASK                    (0x3 << usbDevHw_REG_ENDPT_CFG_TYPE_SHIFT)
#define usbDevHw_REG_ENDPT_CFG_DIRN_IN                      (1 << 4)
#define usbDevHw_REG_ENDPT_CFG_DIRN_OUT                     0
#define usbDevHw_REG_ENDPT_CFG_FIFO_NUM_SHIFT               0
#define usbDevHw_REG_ENDPT_CFG_FIFO_NUM_MASK                (0xf << usbDevHw_REG_ENDPT_CFG_FIFO_NUM_SHIFT)


/* ---- Endpoint Interrupt register definitions ------------------ */

#define usbDevHw_REG_ENDPT_INTR_OUT_SHIFT                   16
#define usbDevHw_REG_ENDPT_INTR_OUT_MASK                    (0xf << usbDevHw_REG_ENDPT_INTR_OUT_SHIFT)
#define usbDevHw_REG_ENDPT_INTR_IN_SHIFT                    0
#define usbDevHw_REG_ENDPT_INTR_IN_MASK                     (0xf << usbDevHw_REG_ENDPT_INTR_IN_SHIFT)

/* Device Controller register definitions */
#define usbDevHw_REG_DEV_CFG_ULPI_DDR_ENABLE                (1 << 19)
#define usbDevHw_REG_DEV_CFG_SET_DESCRIPTOR_ENABLE          (1 << 18)
#define usbDevHw_REG_DEV_CFG_CSR_PROGRAM_ENABLE             (1 << 17)
#define usbDevHw_REG_DEV_CFG_HALT_STALL_ENABLE              (1 << 16)
#define usbDevHw_REG_DEV_CFG_HS_TIMEOUT_CALIB_SHIFT         13
#define usbDevHw_REG_DEV_CFG_HS_TIMEOUT_CALIB_MASK          (7 << usbDevHw_REG_DEV_CFG_HS_TIMEOUT_CALIB_SHIFT)
#define usbDevHw_REG_DEV_CFG_FS_TIMEOUT_CALIB_SHIFT         10
#define usbDevHw_REG_DEV_CFG_FS_TIMEOUT_CALIB_MASK          (7 << usbDevHw_REG_DEV_CFG_FS_TIMEOUT_CALIB_SHIFT)
#define usbDevHw_REG_DEV_CFG_STATUS_1_ENABLE                (1 << 8)
#define usbDevHw_REG_DEV_CFG_STATUS_ENABLE                  (1 << 7)
#define usbDevHw_REG_DEV_CFG_UTMI_BI_DIRN_ENABLE            (1 << 6)
#define usbDevHw_REG_DEV_CFG_UTMI_8BIT_ENABLE               (1 << 5)
#define usbDevHw_REG_DEV_CFG_SYNC_FRAME_ENABLE              (1 << 4)
#define usbDevHw_REG_DEV_CFG_SELF_PWR_ENABLE                (1 << 3)
#define usbDevHw_REG_DEV_CFG_REMOTE_WAKEUP_ENABLE           (1 << 2)
#define usbDevHw_REG_DEV_CFG_SPD_SHIFT                      0
#define usbDevHw_REG_DEV_CFG_SPD_MASK                       (3 << usbDevHw_REG_DEV_CFG_SPD_SHIFT)
#define usbDevHw_REG_DEV_CFG_SPD_HS                         (0 << usbDevHw_REG_DEV_CFG_SPD_SHIFT)
#define usbDevHw_REG_DEV_CFG_SPD_FS                         (1 << usbDevHw_REG_DEV_CFG_SPD_SHIFT)
#define usbDevHw_REG_DEV_CFG_SPD_LS                         (2 << usbDevHw_REG_DEV_CFG_SPD_SHIFT)
#define usbDevHw_REG_DEV_CFG_SPD_FS_48MHZ                   (3 << usbDevHw_REG_DEV_CFG_SPD_SHIFT)


#define usbDevHw_REG_DEV_CTRL_DMA_OUT_THRESHOLD_LEN_SHIFT   24
#define usbDevHw_REG_DEV_CTRL_DMA_OUT_THRESHOLD_LEN_MASK    (0xff << usbDevHw_REG_DEV_CTRL_DMA_OUT_THRESHOLD_LEN_SHIFT)
#define usbDevHw_REG_DEV_CTRL_DMA_BURST_LEN_SHIFT           16
#define usbDevHw_REG_DEV_CTRL_DMA_BURST_LEN_MASK            (0xff << usbDevHw_REG_DEV_CTRL_DMA_BURST_LEN_SHIFT)
#define usbDevHw_REG_DEV_CTRL_OUT_FIFO_FLUSH_ENABLE         (1 << 14)
#define usbDevHw_REG_DEV_CTRL_CSR_DONE                      (1 << 13)
#define usbDevHw_REG_DEV_CTRL_OUT_NAK_ALL_ENABLE            (1 << 12)
#define usbDevHw_REG_DEV_CTRL_DISCONNECT_ENABLE             (1 << 10)
#define usbDevHw_REG_DEV_CTRL_DMA_MODE_ENABLE               (1 << 9)
#define usbDevHw_REG_DEV_CTRL_DMA_BURST_ENABLE              (1 << 8)
#define usbDevHw_REG_DEV_CTRL_DMA_OUT_THRESHOLD_ENABLE      (1 << 7)
#define usbDevHw_REG_DEV_CTRL_DMA_BUFF_FILL_MODE_ENABLE     (1 << 6)
#define usbDevHw_REG_DEV_CTRL_ENDIAN_BIG_ENABLE             (1 << 5)
#define usbDevHw_REG_DEV_CTRL_DMA_DESC_UPDATE_ENABLE        (1 << 4)
#define usbDevHw_REG_DEV_CTRL_DMA_IN_ENABLE                 (1 << 3)  /*TX DMA Enable */
#define usbDevHw_REG_DEV_CTRL_DMA_OUT_ENABLE                (1 << 2)  /*RX DMA Enable */
#define usbDevHw_REG_DEV_CTRL_RESUME_SIGNAL_ENABLE          (1 << 0)
#define usbDevHw_REG_DEV_CTRL_LE_ENABLE                      0           /*^BCM5892 */


#define usbDevHw_REG_DEV_STATUS_SOF_FRAME_NUM_SHIFT         18
#define usbDevHw_REG_DEV_STATUS_SOF_FRAME_NUM_MASK          (0x3ffff << usbDevHw_REG_DEV_STATUS_SOF_FRAME_NUM_SHIFT)
#define usbDevHw_REG_DEV_STATUS_REMOTE_WAKEUP_ALLOWED       (1 << 17)
#define usbDevHw_REG_DEV_STATUS_PHY_ERROR                   (1 << 16)
#define usbDevHw_REG_DEV_STATUS_OUT_FIFO_EMPTY              (1 << 15)
#define usbDevHw_REG_DEV_STATUS_SPD_SHIFT                   13
#define usbDevHw_REG_DEV_STATUS_SPD_MASK                    (3 << usbDevHw_REG_DEV_STATUS_SPD_SHIFT)
#define usbDevHw_REG_DEV_STATUS_SPD_HS                      (0 << usbDevHw_REG_DEV_STATUS_SPD_SHIFT)
#define usbDevHw_REG_DEV_STATUS_SPD_FS                      (1 << usbDevHw_REG_DEV_STATUS_SPD_SHIFT)
#define usbDevHw_REG_DEV_STATUS_SPD_LS                      (2 << usbDevHw_REG_DEV_STATUS_SPD_SHIFT)
#define usbDevHw_REG_DEV_STATUS_SPD_FS_48MHZ                (3 << usbDevHw_REG_DEV_STATUS_SPD_SHIFT)
#define usbDevHw_REG_DEV_STATUS_BUS_SUSPENDED               (1 << 12)
#define usbDevHw_REG_DEV_STATUS_ALT_NUM_SHIFT               8
#define usbDevHw_REG_DEV_STATUS_ALT_NUM_MASK                (0xf << usbDevHw_REG_DEV_STATUS_ALT_NUM_SHIFT)
#define usbDevHw_REG_DEV_STATUS_INTF_NUM_SHIFT              4
#define usbDevHw_REG_DEV_STATUS_INTF_NUM_MASK               (0xf << usbDevHw_REG_DEV_STATUS_INTF_NUM_SHIFT)
#define usbDevHw_REG_DEV_STATUS_CFG_NUM_SHIFT               0
#define usbDevHw_REG_DEV_STATUS_CFG_NUM_MASK                (0xf << usbDevHw_REG_DEV_STATUS_CFG_NUM_SHIFT)


#define usbDevHw_REG_DEV_INTR_REMOTE_WAKEUP_DELTA           (1 << 7) /*Remote Wakeup Delta*/
#define usbDevHw_REG_DEV_INTR_SPD_ENUM_DONE                 (1 << 6) /*ENUM Speed Completed*/
#define usbDevHw_REG_DEV_INTR_SOF_RX                        (1 << 5) /*SOF Token Detected */
#define usbDevHw_REG_DEV_INTR_BUS_SUSPEND                   (1 << 4) /*SUSPEND State Detected*/
#define usbDevHw_REG_DEV_INTR_BUS_RESET                     (1 << 3) /*RESET State Detected */
#define usbDevHw_REG_DEV_INTR_BUS_IDLE                      (1 << 2) /*IDLE State Detected*/
#define usbDevHw_REG_DEV_INTR_SET_INTF_RX                   (1 << 1) /*Received SET_INTERFACE CMD*/
#define usbDevHw_REG_DEV_INTR_SET_CFG_RX                    (1 << 0) /*Received SET_CONFIG CMD*/


/* ---- DMA Descriptor definitions ------------------------------- */

#define usbDevHw_REG_DMA_STATUS_BUF_SHIFT                   30
#define usbDevHw_REG_DMA_STATUS_BUF_HOST_READY              (0 << usbDevHw_REG_DMA_STATUS_BUF_SHIFT)
#define usbDevHw_REG_DMA_STATUS_BUF_DMA_BUSY                (1 << usbDevHw_REG_DMA_STATUS_BUF_SHIFT)
#define usbDevHw_REG_DMA_STATUS_BUF_DMA_DONE                (2 << usbDevHw_REG_DMA_STATUS_BUF_SHIFT)
#define usbDevHw_REG_DMA_STATUS_BUF_HOST_BUSY               (3 << usbDevHw_REG_DMA_STATUS_BUF_SHIFT)
#define usbDevHw_REG_DMA_STATUS_BUF_MASK                    (3 << usbDevHw_REG_DMA_STATUS_BUF_SHIFT)

#define usbDevHw_REG_DMA_STATUS_RX_SHIFT                    28
#define usbDevHw_REG_DMA_STATUS_RX_SUCCESS                  (0 << usbDevHw_REG_DMA_STATUS_RX_SHIFT)
#define usbDevHw_REG_DMA_STATUS_RX_ERR_DESC                 (1 << usbDevHw_REG_DMA_STATUS_RX_SHIFT)
#define usbDevHw_REG_DMA_STATUS_RX_ERR_BUF                  (3 << usbDevHw_REG_DMA_STATUS_RX_SHIFT)
#define usbDevHw_REG_DMA_STATUS_RX_MASK                     (3 << usbDevHw_REG_DMA_STATUS_RX_SHIFT)

#define usbDevHw_REG_DMA_STATUS_CFG_NUM_SHIFT               24
#define usbDevHw_REG_DMA_STATUS_CFG_NUM_MASK                (0xf << usbDevHw_REG_DMA_STATUS_CFG_NUM_SHIFT)

#define usbDevHw_REG_DMA_STATUS_INTF_NUM_SHIFT              20
#define usbDevHw_REG_DMA_STATUS_INTF_NUM_MASK               (0xf << usbDevHw_REG_DMA_STATUS_INTF_NUM_SHIFT)

#define usbDevHw_REG_DMA_STATUS_ALT_NUM_SHIFT               16
#define usbDevHw_REG_DMA_STATUS_ALT_NUM_MASK                (0xf << usbDevHw_REG_DMA_STATUS_ALT_NUM_SHIFT)

#define usbDevHw_REG_DMA_STATUS_LAST_DESC                   (1 << 27)

#define usbDevHw_REG_DMA_STATUS_FRAME_NUM_SHIFT             16
#define usbDevHw_REG_DMA_STATUS_FRAME_NUM_MASK              (0x7ff << usbDevHw_REG_DMA_STATUS_FRAME_NUM_SHIFT)

#define usbDevHw_REG_DMA_STATUS_BYTE_CNT_SHIFT              0

#define usbDevHw_REG_DMA_STATUS_ISO_PID_SHIFT               14
#define usbDevHw_REG_DMA_STATUS_ISO_PID_MASK                (0x3 << usbDevHw_REG_DMA_STATUS_ISO_PID_SHIFT)

#define usbDevHw_REG_DMA_STATUS_ISO_BYTE_CNT_SHIFT          usbDevHw_REG_DMA_STATUS_BYTE_CNT_SHIFT
#define usbDevHw_REG_DMA_STATUS_ISO_BYTE_CNT_MASK           (0x3fff << usbDevHw_REG_DMA_STATUS_ISO_BYTE_CNT_SHIFT)

#define usbDevHw_REG_DMA_STATUS_NON_ISO_BYTE_CNT_SHIFT      usbDevHw_REG_DMA_STATUS_BYTE_CNT_SHIFT
#define usbDevHw_REG_DMA_STATUS_NON_ISO_BYTE_CNT_MASK       (0xffff << usbDevHw_REG_DMA_STATUS_NON_ISO_BYTE_CNT_SHIFT)

/* ---- Block level strap option register definitions ----------- */

#define usbDevHw_REG_STRAP_SNAK_ENH                         (1 << 0)
#define usbDevHw_REG_STRAP_STALL_SET_ENH                    (1 << 2)
#define usbDevHw_REG_STRAP_CNAK_CLR_ENH                     (1 << 3)

/* ---- Endpoint Configuration register definitions --------------- */

/*
 * usbDevHw_REG_ENDPT_CNT is just for register layout purposes. There may not be this
 * many endpoints actually enabled (activated, configured) in the RTL.
 */
#define usbDevHw_REG_ENDPT_CNT      10

typedef struct
{
    uint32_t ctrl;
    uint32_t status;
    uint32_t size1;
    uint32_t size2; /* Buf Size OUT/Max PKT SIZE */
    uint32_t setupBufAddr;
    uint32_t dataDescAddr;
}
usbDevHw_REG_ENDPT_FIFO_t;

/* maps to page # 97 of UDC data sheet */

typedef struct
{
    usbDevHw_REG_ENDPT_FIFO_t eptFifoIn[usbDevHw_REG_ENDPT_CNT];
    usbDevHw_REG_ENDPT_FIFO_t eptFifoOut[usbDevHw_REG_ENDPT_CNT];

    uint32_t devCfg;
    uint32_t devCtrl;
    uint32_t devStatus;
    uint32_t devIntrStatus;
    uint32_t devIntrMask;
    uint32_t eptIntrStatus;
    uint32_t eptIntrMask;
    uint32_t releaseNum ;  
    uint32_t LpmCtrlStatus;  

    uint32_t eptCfg[usbDevHw_REG_ENDPT_CNT];
	uint32_t RxFIFO[256];
	uint32_t TxFIFO[256];
    uint32_t strap;
}
usbDevHw_REG_t;

/* structure required for initial pull up pull down to generate d+ d- potential difference */


typedef struct
{
uint32_t GPIO1_enable;
uint32_t GPIO1_data;
}
usbDevPull_REG_t;


/* ---- Public Constants and Types --------------------------------------- */

#define usbDevHw_DEVICE_IRQ_REMOTEWAKEUP_DELTA	  	usbDevHw_REG_DEV_INTR_REMOTE_WAKEUP_DELTA
#define usbDevHw_DEVICE_IRQ_SPEED_ENUM_DONE		 	usbDevHw_REG_DEV_INTR_SPD_ENUM_DONE
#define usbDevHw_DEVICE_IRQ_SOF_DETECTED			usbDevHw_REG_DEV_INTR_SOF_RX
#define usbDevHw_DEVICE_IRQ_BUS_SUSPEND			 	usbDevHw_REG_DEV_INTR_BUS_SUSPEND
#define usbDevHw_DEVICE_IRQ_BUS_RESET			   	usbDevHw_REG_DEV_INTR_BUS_RESET
#define usbDevHw_DEVICE_IRQ_BUS_IDLE				usbDevHw_REG_DEV_INTR_BUS_IDLE
#define usbDevHw_DEVICE_IRQ_SET_INTF				usbDevHw_REG_DEV_INTR_SET_INTF_RX
#define usbDevHw_DEVICE_IRQ_SET_CFG				 	usbDevHw_REG_DEV_INTR_SET_CFG_RX

#define usbDevHw_ENDPT_STATUS_DMA_ERROR			 	usbDevHw_REG_ENDPT_FIFO_STATUS_AHB_BUS_ERROR
#define usbDevHw_ENDPT_STATUS_DMA_BUF_NOT_AVAIL		usbDevHw_REG_ENDPT_FIFO_STATUS_DMA_BUF_NOT_AVAIL
#define usbDevHw_ENDPT_STATUS_IN_TOKEN_RX		   	usbDevHw_REG_ENDPT_FIFO_STATUS_IN_TOKEN_RX
#define usbDevHw_ENDPT_STATUS_IN_DMA_DONE		   	usbDevHw_REG_ENDPT_FIFO_STATUS_IN_DMA_DONE
#define	usbDevHw_ENDPT_STATUS_IN_FIFO_EMPTY        	usbDevHw_REG_ENDPT_FIFO_STATUS_IN_FIFO_EMPTY
#define usbDevHw_ENDPT_STATUS_IN_XFER_DONE		  	usbDevHw_REG_ENDPT_FIFO_STATUS_IN_XFER_DONE
#define usbDevHw_ENDPT_STATUS_OUT_DMA_DATA_DONE	 	usbDevHw_REG_ENDPT_FIFO_STATUS_OUT_DMA_DATA_DONE
#define usbDevHw_ENDPT_STATUS_OUT_DMA_SETUP_DONE	usbDevHw_REG_ENDPT_FIFO_STATUS_OUT_DMA_SETUP_DONE

/* Number of endpoints actually configured (supported) in RTL. This may be less than usbDevHw_REG_ENDPT_CNT. */
#define usbDevHw_ENDPT_CFG_CNT					  10

//#define usbDevHw_REG_P				((volatile usbDevHw_REG_t *)(USB_DEVICE_BASE))
#define START_USB1_P			  	((volatile usbDevHw_REG_t *)(USB_DEVICE_BASE))
#if 0
#define usbDevPull_REG_P		  	((volatile usbDevPull_REG_t *)(MM_IO_BASE_GIO1_R_GRPF1_OUT_ENABLE_MEMADDR))
#endif
/* ---- Public Variable Externs ------------------------------------------ */
/* ---- Public Function Prototypes --------------------------------------- */

/* ---- Inline Function Definitions -------------------------------------- */

#ifdef usbDevHw_DEBUG_REG

#define REG_WR(reg, val)			 	reg32_write(&reg, val)
#define REG_MOD_OR(reg, val)		  	reg32_modify_or(&reg, val)
#define REG_MOD_AND(reg, val)		 	reg32_modify_and(&reg, val)
#define REG_MOD_MASK(reg, mask, val)  	reg32_modify_mask(&reg, mask, val)

#else

#define REG_WR(reg, val)			  	*(uint32_t*)reg = val
#define REG_MOD_OR(reg, val)		  	*(uint32_t*)reg = *(uint32_t*)reg | val
#define REG_MOD_AND(reg, val)		 	*(uint32_t*)reg = *(uint32_t*)reg & val
#define REG_MOD_MASK(reg, mask, val)  	*(uint32_t*)reg = (*(uint32_t*)reg & mask) | val

#endif

#define usbDevHw_DEVICE_IRQ_ALL            (usbDevHw_DEVICE_IRQ_REMOTEWAKEUP_DELTA | \
                                            usbDevHw_DEVICE_IRQ_SPEED_ENUM_DONE    | \
                                            usbDevHw_DEVICE_IRQ_SOF_DETECTED       | \
                                            usbDevHw_DEVICE_IRQ_BUS_SUSPEND        | \
                                            usbDevHw_DEVICE_IRQ_BUS_RESET          | \
                                            usbDevHw_DEVICE_IRQ_BUS_IDLE           | \
                                            usbDevHw_DEVICE_IRQ_SET_INTF           | \
                                            usbDevHw_DEVICE_IRQ_SET_CFG            )

#define usbDevHw_DEVICE_SPEED_UNKNOWN       0
#define usbDevHw_DEVICE_SPEED_LOW           1
#define usbDevHw_DEVICE_SPEED_FULL          2
#define usbDevHw_DEVICE_SPEED_HIGH          3

/*
 * usbDevHw_ENDPT_STATUS_DMA_ERROR
 * usbDevHw_ENDPT_STATUS_DMA_BUF_NOT_AVAIL
 * usbDevHw_ENDPT_STATUS_IN_TOKEN_RX
 * usbDevHw_ENDPT_STATUS_IN_DMA_DONE
 * usbDevHw_ENDPT_STATUS_IN_XFER_DONE
 * usbDevHw_ENDPT_STATUS_OUT_DMA_DATA_DONE
 * usbDevHw_ENDPT_STATUS_OUT_DMA_SETUP_DONE
 */
#define usbDevHw_ENDPT_STATUS_ALL          (usbDevHw_ENDPT_STATUS_DMA_ERROR         | \
                                            usbDevHw_ENDPT_STATUS_DMA_BUF_NOT_AVAIL | \
                                            usbDevHw_ENDPT_STATUS_IN_TOKEN_RX       | \
                                            usbDevHw_ENDPT_STATUS_IN_DMA_DONE       | \
                                            usbDevHw_ENDPT_STATUS_IN_XFER_DONE      | \
                                            usbDevHw_ENDPT_STATUS_OUT_DMA_DATA_DONE | \
                                            usbDevHw_ENDPT_STATUS_OUT_DMA_SETUP_DONE )

/*
 * The ENDPT_DIRN enumeration is the same as the USB protocol endpoint descriptors'
 * bEndpointAddress field and control requests bRequestType. This is intentional.
 */
#define usbDevHw_ENDPT_DIRN_IN              0x80
#define usbDevHw_ENDPT_DIRN_OUT             0x00
#define usbDevHw_ENDPT_DIRN_MASK            0x80

#define usbDevHw_ENDPT_TYPE_CTRL            0
#define usbDevHw_ENDPT_TYPE_ISOC            1
#define usbDevHw_ENDPT_TYPE_BULK            2
#define usbDevHw_ENDPT_TYPE_INTR            3
#define usbDevHw_ENDPT_TYPE_MASK            0x03

#ifndef reg32_read
#define reg32_read(addr) (*(volatile unsigned int *)(addr))
#define reg32_write(addr, l) ((*(volatile unsigned int *) (addr)) = (l))
#endif

#endif // __USB_UDC_H__

