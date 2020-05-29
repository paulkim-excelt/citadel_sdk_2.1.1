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

/*
 *  Macros to muck with bitfields
 */

#ifndef _OHCI_H_
#define _OHCI_H_

#ifdef CONFIG_DATA_CACHE_SUPPORT
#define ALIGNMENT 64
#else
#define ALIGNMENT 32
#endif

#define USE_STATIC_MEM_ALLOC 1

#define REG8_RSVD(start, end)	\
	u8_t rsvd_##start[(end - start) / sizeof(u8_t)]
#define REG16_RSVD(start, end)	\
	u16_t rsvd_##start[(end - start) / sizeof(u16_t)]
#define REG32_RSVD(start, end)  \
	u32_t rsvd_##start[(end - start) / sizeof(u32_t)]

#define _OHCI_MAKE32(x) ((u32_t)(x))

/*
 * Make a mask for 1 bit at position 'n'
 */

#define _OHCI_MAKEMASK1(n) (_OHCI_MAKE32(1) << _OHCI_MAKE32(n))

/*
 * Make a mask for 'v' bits at position 'n'
 */

#define _OHCI_MAKEMASK(v, n) \
	(_OHCI_MAKE32((_OHCI_MAKE32(1)<<(v))-1) << _OHCI_MAKE32(n))

/*
 * Make a value at 'v' at bit position 'n'
 */

#define _OHCI_MAKEVALUE(v, n) (_OHCI_MAKE32(v) << _OHCI_MAKE32(n))
#define _OHCI_GETVALUE(v, n, m) \
	((_OHCI_MAKE32(v) & _OHCI_MAKE32(m)) >> _OHCI_MAKE32(n))

/*
 *  Endpoint Descriptor
 */

#define OHCI_ED_ALIGN	32

typedef struct ohci_ed_s {
	volatile u32_t ed_control;
	volatile u32_t ed_tailp;
	volatile u32_t ed_headp;
	volatile u32_t ed_next_ed;
} __attribute__((aligned(ALIGNMENT))) ohci_ed_t;

#define S_OHCI_ED_FA		0
#define M_OHCI_ED_FA		_OHCI_MAKEMASK(7, S_OHCI_ED_FA)
#define V_OHCI_ED_FA(x)		_OHCI_MAKEVALUE(x, S_OHCI_ED_FA)
#define G_OHCI_ED_FA(x) \
	_OHCI_GETVALUE(x, S_OHCI_ED_FA, M_OHCI_ED_FA)

#define S_OHCI_ED_EN		7
#define M_OHCI_ED_EN		_OHCI_MAKEMASK(4, S_OHCI_ED_EN)
#define V_OHCI_ED_EN(x)		_OHCI_MAKEVALUE(x, S_OHCI_ED_EN)
#define G_OHCI_ED_EN(x)	\
	_OHCI_GETVALUE(x, S_OHCI_ED_EN, M_OHCI_ED_EN)

#define S_OHCI_ED_DIR		11
#define M_OHCI_ED_DIR		_OHCI_MAKEMASK(2, S_OHCI_ED_DIR)
#define V_OHCI_ED_DIR(x)	_OHCI_MAKEVALUE(x, S_OHCI_ED_DIR)
#define G_OHCI_ED_DIR(x) \
	_OHCI_GETVALUE(x, S_OHCI_ED_DIR, M_OHCI_ED_DIR)

#define K_OHCI_ED_DIR_FROMTD	0
#define K_OHCI_ED_DIR_OUT	1
#define K_OHCI_ED_DIR_IN	2

#define M_OHCI_ED_LOWSPEED	_OHCI_MAKEMASK1(13)
#define M_OHCI_ED_SKIP		_OHCI_MAKEMASK1(14)
#define M_OHCI_ED_ISOCFMT	_OHCI_MAKEMASK1(15)

#define S_OHCI_ED_MPS		16
#define M_OHCI_ED_MPS		_OHCI_MAKEMASK(11, S_OHCI_ED_MPS)
#define V_OHCI_ED_MPS(x)	_OHCI_MAKEVALUE(x, S_OHCI_ED_MPS)
#define G_OHCI_ED_MPS(x) \
	_OHCI_GETVALUE(x, S_OHCI_ED_MPS, M_OHCI_ED_MPS)

#define M_OHCI_ED_PTRMASK	0xFFFFFFF0
#define M_OHCI_ED_HALT		_OHCI_MAKEMASK1(0)
#define M_OHCI_ED_TOGGLECARRY	_OHCI_MAKEMASK1(1)

/*
 *  Transfer Descriptor (interrupt, bulk)
 */

#define OHCI_TD_ALIGN	32
#define M_OHCI_TD_SHORTOK	_OHCI_MAKEMASK1(18)

#define S_OHCI_TD_PID		19
#define M_OHCI_TD_PID		_OHCI_MAKEMASK(2, S_OHCI_TD_PID)
#define V_OHCI_TD_PID(x)	_OHCI_MAKEVALUE(x, S_OHCI_TD_PID)
#define G_OHCI_TD_PID(x) \
	_OHCI_GETVALUE(x, S_OHCI_TD_PID, M_OHCI_TD_PID)

#define K_OHCI_TD_SETUP		0
#define K_OHCI_TD_OUT		1
#define K_OHCI_TD_IN		2
#define K_OHCI_TD_RESERVED	3

#define V_OHCI_TD_SETUP		V_OHCI_TD_PID(K_OHCI_TD_SETUP)
#define V_OHCI_TD_OUT		V_OHCI_TD_PID(K_OHCI_TD_OUT)
#define V_OHCI_TD_IN		V_OHCI_TD_PID(K_OHCI_TD_IN)
#define V_OHCI_TD_RESERVED	V_OHCI_TD_PID(K_OHCI_TD_RESERVED)

#define S_OHCI_TD_DI		21
#define M_OHCI_TD_DI		_OHCI_MAKEMASK(3, S_OHCI_TD_DI)
#define V_OHCI_TD_DI(x)		_OHCI_MAKEVALUE(x, S_OHCI_TD_DI)
#define G_OHCI_TD_DI(x)	\
	_OHCI_GETVALUE(x, S_OHCI_TD_DI, M_OHCI_TD_DI)

#define K_OHCI_TD_NOINTR	7
#define V_OHCI_TD_NOINTR	V_OHCI_TD_DI(K_OHCI_TD_NOINTR)

#define S_OHCI_TD_DT		24
#define M_OHCI_TD_DT		_OHCI_MAKEMASK(2, S_OHCI_TD_DT)
#define V_OHCI_TD_DT(x)		_OHCI_MAKEVALUE(x, S_OHCI_TD_DT)
#define G_OHCI_TD_DT(x)		_OHCI_GETVALUE(x, S_OHCI_TD_DT, M_OHCI_TD_DT)

#define K_OHCI_TD_DT_DATA0	2
#define K_OHCI_TD_DT_DATA1	3
#define K_OHCI_TD_DT_TCARRY	0

#define S_OHCI_TD_EC		26
#define M_OHCI_TD_EC		_OHCI_MAKEMASK(2, S_OHCI_TD_EC)
#define V_OHCI_TD_EC(x)		_OHCI_MAKEVALUE(x, S_OHCI_TD_EC)
#define G_OHCI_TD_EC(x)		_OHCI_GETVALUE(x, S_OHCI_TD_EC, M_OHCI_TD_EC)

#define S_OHCI_TD_CC		28
#define M_OHCI_TD_CC		_OHCI_MAKEMASK(4, S_OHCI_TD_CC)
#define V_OHCI_TD_CC(x)		_OHCI_MAKEVALUE(x, S_OHCI_TD_CC)
#define G_OHCI_TD_CC(x)		_OHCI_GETVALUE(x, S_OHCI_TD_CC, M_OHCI_TD_CC)

#define K_OHCI_CC_NOERROR		0
#define K_OHCI_CC_CRC			1
#define K_OHCI_CC_BITSTUFFING		2
#define K_OHCI_CC_DATATOGGLEMISMATCH	3
#define K_OHCI_CC_STALL			4
#define K_OHCI_CC_DEVICENOTRESPONDING	5
#define K_OHCI_CC_PIDCHECKFAILURE	6
#define K_OHCI_CC_UNEXPECTEDPID		7
#define K_OHCI_CC_DATAOVERRUN		8
#define K_OHCI_CC_DATAUNDERRUN		9
#define K_OHCI_CC_BUFFEROVERRUN		12
#define K_OHCI_CC_BUFFERUNDERRUN	13
#define K_OHCI_CC_NOTACCESSED		15

#define K_OHCI_CC_CANCELLED		0xFF

#define OHCI_TD_MAX_DATA		8192
/*
 *  Transfer Descriptor (isochronous)
 */

#define S_OHCI_ITD_SF		0
#define M_OHCI_ITD_SF		_OHCI_MAKEMASK(16, S_OHCI_ITD_SF)
#define V_OHCI_ITD_SF(x)	_OHCI_MAKEVALUE(x, S_OHCI_ITD_SF)
#define G_OHCI_ITD_SF(x) \
	_OHCI_GETVALUE(x, S_OHCI_ITD_SF, M_OHCI_ITD_SF)

#define S_OHCI_ITD_FC		24
#define M_OHCI_ITD_FC		_OHCI_MAKEMASK(3, S_OHCI_ITD_FC)
#define V_OHCI_ITD_FC(x)	_OHCI_MAKEVALUE(x, S_OHCI_ITD_FC)
#define G_OHCI_ITD_FC(x) \
	_OHCI_GETVALUE(x, S_OHCI_ITD_FC, M_OHCI_ITD_FC)

typedef struct ohci_td_s {
	volatile u32_t td_control;
	volatile u32_t td_cbp;
	volatile u32_t td_next_td;

	volatile u32_t td_be;
	volatile u32_t type;	/*  1:TD, 2:ITD  */
	volatile u32_t usbEndpoint;
} __attribute__((aligned(ALIGNMENT)))ohci_td_t; //make each object 64 byte sized


/*
 *  Host Controller Communications Area (HCCA)
 */

#define OHCI_INTTABLE_SIZE	32

#define OHCI_HCCA_ALIGN		256 /*Align on 256-byte boundary */

typedef struct ohci_hcca_s {
	u32_t hcca_inttable[OHCI_INTTABLE_SIZE];
	u32_t hcca_framenum;	/*note: actually two 16-bit fields */
	u32_t hcca_donehead;
	u32_t hcca_reserved[29];/*round to 256 bytes */
	u32_t hcca_pad;
} ohci_hcca_t;

/*
 *  Registers
 */
#define USB_OHCI_BASE (u32_t)0x46000800

#define _OHCI_REGIDX(x)	((x)*4)
#define R_OHCI_REVISION		(0x0)
#define R_OHCI_CONTROL		(0x4)
#define R_OHCI_CMDSTATUS	(0x8)
#define R_OHCI_INTSTATUS	(0xc)
#define R_OHCI_INTENABLE	(0x10)
#define R_OHCI_INTDISABLE	(0x14)
#define R_OHCI_HCCA		(0x18)
#define R_OHCI_PERIODCURRENTED	(0x1c)
#define R_OHCI_CONTROLHEADED	(0x20)
#define R_OHCI_CONTROLCURRENTED (0x24)
#define R_OHCI_BULKHEADED	(0x28)
#define R_OHCI_BULKCURRENTED	(0x2c)
#define R_OHCI_DONEHEAD		(0x30)
#define R_OHCI_FMINTERVAL	(0x34)
#define R_OHCI_FMREMAINING	(0x38)
#define R_OHCI_FMNUMBER		(0x3c)
#define R_OHCI_PERIODICSTART	(0x40)
#define R_OHCI_LSTHRESHOLD	(0x44)
#define R_OHCI_RHDSCRA		(0x48)
#define R_OHCI_RHDSCRB		(0x4c)
#define R_OHCI_RHSTATUS		(0x50)
#define R_OHCI_RHPORTSTATUS(x)	(0x50+ (x*4)) /* 1-based! */

/*
 *R_OHCI_REVISION
 */

#define S_OHCI_REV_REV		0
#define M_OHCI_REV_REV		_OHCI_MAKEMASK(8, S_OHCI_REV_REV)
#define V_OHCI_REV_REV(x)	_OHCI_MAKEVALUE(x, S_OHCI_REV_REV)
#define G_OHCI_REV_REV(x) \
	_OHCI_GETVALUE(x, S_OHCI_REV_REV, M_OHCI_REV_REV)
#define K_OHCI_REV_11		0x10

/*
 *R_OHCI_CONTROL
 */

#define S_OHCI_CONTROL_CBSR	0
#define M_OHCI_CONTROL_CBSR	_OHCI_MAKEMASK(2, S_OHCI_CONTROL_CBSR)
#define V_OHCI_CONTROL_CBSR(x)	_OHCI_MAKEVALUE(x, S_OHCI_CONTROL_CBSR)
#define G_OHCI_CONTROL_CBSR(x)	\
	_OHCI_GETVALUE(x, S_OHCI_CONTROL_CBSR, M_OHCI_CONTROL_CBSR)

#define K_OHCI_CBSR_11		0
#define K_OHCI_CBSR_21		1
#define K_OHCI_CBSR_31		2
#define K_OHCI_CBSR_41		3

#define M_OHCI_CONTROL_PLE	_OHCI_MAKEMASK1(2)
#define M_OHCI_CONTROL_IE	_OHCI_MAKEMASK1(3)
#define M_OHCI_CONTROL_CLE	_OHCI_MAKEMASK1(4)
#define M_OHCI_CONTROL_BLE	_OHCI_MAKEMASK1(5)

#define S_OHCI_CONTROL_HCFS	6
#define M_OHCI_CONTROL_HCFS	_OHCI_MAKEMASK(2, S_OHCI_CONTROL_HCFS)
#define V_OHCI_CONTROL_HCFS(x)	_OHCI_MAKEVALUE(x, S_OHCI_CONTROL_HCFS)
#define G_OHCI_CONTROL_HCFS(x)	\
	_OHCI_GETVALUE(x, S_OHCI_CONTROL_HCFS, M_OHCI_CONTROL_HCFS)

#define K_OHCI_HCFS_RESET	0
#define K_OHCI_HCFS_RESUME	1
#define K_OHCI_HCFS_OPERATIONAL	2
#define K_OHCI_HCFS_SUSPEND	3

#define M_OHCI_CONTROL_IR	_OHCI_MAKEMASK1(8)
#define M_OHCI_CONTROL_RWC	_OHCI_MAKEMASK1(9)
#define M_OHCI_CONTROL_RWE	_OHCI_MAKEMASK1(10)

/*
 *R_OHCI_CMDSTATUS
 */

#define M_OHCI_CMDSTATUS_HCR	_OHCI_MAKEMASK1(0)
#define M_OHCI_CMDSTATUS_CLF	_OHCI_MAKEMASK1(1)
#define M_OHCI_CMDSTATUS_BLF	_OHCI_MAKEMASK1(2)
#define M_OHCI_CMDSTATUS_OCR	_OHCI_MAKEMASK1(3)

#define S_OHCI_CMDSTATUS_SOC	16
#define M_OHCI_CMDSTATUS_SOC	_OHCI_MAKEMASK(2, S_OHCI_CMDSTATUS_SOC)
#define V_OHCI_CMDSTATUS_SOC(x)	_OHCI_MAKEVALUE(x, S_OHCI_CMDSTATUS_SOC)
#define G_OHCI_CMDSTATUS_SOC(x)	\
	_OHCI_GETVALUE(x, S_OHCI_CMDSTATUS_SOC, M_OHCI_CMDSTATUS_SOC)

/*
 *R_OHCI_INTSTATUS, R_OHCI_INTENABLE, R_OHCI_INTDISABLE
 */

#define M_OHCI_INT_SO		_OHCI_MAKEMASK1(0)
#define M_OHCI_INT_WDH		_OHCI_MAKEMASK1(1)
#define M_OHCI_INT_SF		_OHCI_MAKEMASK1(2)
#define M_OHCI_INT_RD		_OHCI_MAKEMASK1(3)
#define M_OHCI_INT_UE		_OHCI_MAKEMASK1(4)
#define M_OHCI_INT_FNO		_OHCI_MAKEMASK1(5)
#define M_OHCI_INT_RHSC		_OHCI_MAKEMASK1(6)
#define M_OHCI_INT_OC		_OHCI_MAKEMASK1(30)
#define M_OHCI_INT_MIE		_OHCI_MAKEMASK1(31)

#define M_OHCI_INT_ALL	(M_OHCI_INT_SO | M_OHCI_INT_WDH | M_OHCI_INT_SF | \
			M_OHCI_INT_RD | M_OHCI_INT_UE | M_OHCI_INT_FNO | \
			M_OHCI_INT_RHSC | M_OHCI_INT_OC | M_OHCI_INT_MIE)

/*
 *R_OHCI_FMINTERVAL
 */

#define S_OHCI_FMINTERVAL_FI		0
#define M_OHCI_FMINTERVAL_FI		_OHCI_MAKEMASK(14, S_OHCI_FMINTERVAL_FI)
#define V_OHCI_FMINTERVAL_FI(x)		_OHCI_MAKEVALUE(x, S_OHCI_FMINTERVAL_FI)
#define G_OHCI_FMINTERVAL_FI(x)	\
	_OHCI_GETVALUE(x, S_OHCI_FMINTERVAL_FI, M_OHCI_FMINTERVAL_FI)

#define S_OHCI_FMINTERVAL_FSMPS		16
#define M_OHCI_FMINTERVAL_FSMPS	 \
	_OHCI_MAKEMASK(15, S_OHCI_FMINTERVAL_FSMPS)
#define V_OHCI_FMINTERVAL_FSMPS(x) \
	_OHCI_MAKEVALUE(x, S_OHCI_FMINTERVAL_FSMPS)
#define G_OHCI_FMINTERVAL_FSMPS(x) \
	_OHCI_GETVALUE(x, S_OHCI_FMINTERVAL_FSMPS, M_OHCI_FMINTERVAL_FSMPS)

#define OHCI_CALC_FSMPS(x) ((((x)-210)*6/7))

#define M_OHCI_FMINTERVAL_FIT		_OHCI_MAKEMASK1(31)

/*
 *R_OHCI_FMREMAINING
 */

#define S_OHCI_FMREMAINING_FR		0
#define M_OHCI_FMREMAINING_FR \
	_OHCI_MAKEMASK(14, S_OHCI_FMREMAINING_FR)
#define V_OHCI_FMREMAINING_FR(x) \
	_OHCI_MAKEVALUE(x, S_OHCI_FMREMAINING_FR)
#define G_OHCI_FMREMAINING_FR(x) \
	_OHCI_GETVALUE(x, S_OHCI_FMREMAINING_FR, M_OHCI_FMREMAINING_FR)

#define M_OHCI_FMREMAINING_FRT		_OHCI_MAKEMASK1(31)

/*
 *R_OHCI_RHDSCRA
 */

#define S_OHCI_RHDSCRA_NDP	0
#define M_OHCI_RHDSCRA_NDP	_OHCI_MAKEMASK(8, S_OHCI_RHDSCRA_NDP)
#define V_OHCI_RHDSCRA_NDP(x)	_OHCI_MAKEVALUE(x, S_OHCI_RHDSCRA_NDP)
#define G_OHCI_RHDSCRA_NDP(x) \
	_OHCI_GETVALUE(x, S_OHCI_RHDSCRA_NDP, M_OHCI_RHDSCRA_NDP)

#define M_OHCI_RHDSCRA_PSM	_OHCI_MAKEMASK1(8)
#define M_OHCI_RHDSCRA_NPS	_OHCI_MAKEMASK1(9)
#define M_OHCI_RHDSCRA_DT	_OHCI_MAKEMASK1(10)
#define M_OHCI_RHDSCRA_OCPM	_OHCI_MAKEMASK1(11)
#define M_OHCI_RHDSCRA_NOCP	_OHCI_MAKEMASK1(12)

#define S_OHCI_RHDSCRA_POTPGT	 24
#define M_OHCI_RHDSCRA_POTPGT	 _OHCI_MAKEMASK(8, S_OHCI_RHDSCRA_POTPGT)
#define V_OHCI_RHDSCRA_POTPGT(x) _OHCI_MAKEVALUE(x, S_OHCI_RHDSCRA_POTPGT)
#define G_OHCI_RHDSCRA_POTPGT(x) \
	 _OHCI_GETVALUE(x, S_OHCI_RHDSCRA_POTPGT, M_OHCI_RHDSCRA_POTPGT)

/*
 *R_OHCI_RHDSCRB
 */

#define S_OHCI_RHDSCRB_DR	0
#define M_OHCI_RHDSCRB_DR	_OHCI_MAKEMASK(16, S_OHCI_RHDSCRB_DR)
#define V_OHCI_RHDSCRB_DR(x)	_OHCI_MAKEVALUE(x, S_OHCI_RHDSCRB_DR)
#define G_OHCI_RHDSCRB_DR(x) \
	_OHCI_GETVALUE(x, S_OHCI_RHDSCRB_DR, M_OHCI_RHDSCRB_DR)

#define S_OHCI_RHDSCRB_PPCM	16
#define M_OHCI_RHDSCRB_PPCM	_OHCI_MAKEMASK(16, S_OHCI_RHDSCRB_PPCM)
#define V_OHCI_RHDSCRB_PPCM(x)	_OHCI_MAKEVALUE(x, S_OHCI_RHDSCRB_PPCM)
#define G_OHCI_RHDSCRB_PPCM(x) \
	_OHCI_GETVALUE(x, S_OHCI_RHDSCRB_PPCM, M_OHCI_RHDSCRB_PPCM)

/*
 *R_OHCI_RHSTATUS
 */

#define M_OHCI_RHSTATUS_LPS	_OHCI_MAKEMASK1(0)
#define M_OHCI_RHSTATUS_OCI	_OHCI_MAKEMASK1(1)
#define M_OHCI_RHSTATUS_DRWE	_OHCI_MAKEMASK1(15)
#define M_OHCI_RHSTATUS_LPSC    _OHCI_MAKEMASK1(16)
#define M_OHCI_RHSTATUS_OCIC	_OHCI_MAKEMASK1(17)
#define M_OHCI_RHSTATUS_CRWE	_OHCI_MAKEMASK1(31)

/*
 *R_OHCI_RHPORTSTATUS
 */

#define M_OHCI_RHPORTSTAT_CCS	_OHCI_MAKEMASK1(0)/* CurrentConnectStatus */
#define M_OHCI_RHPORTSTAT_PES	_OHCI_MAKEMASK1(1)/* PortEnableStatus */
#define M_OHCI_RHPORTSTAT_PSS	_OHCI_MAKEMASK1(2)/* PortSuspendStatus */
#define M_OHCI_RHPORTSTAT_POCI	_OHCI_MAKEMASK1(3)/* PortOverCurrentIndicator */
#define M_OHCI_RHPORTSTAT_PRS	_OHCI_MAKEMASK1(4)/* PortResetStatus */
#define M_OHCI_RHPORTSTAT_PPS	_OHCI_MAKEMASK1(8)/* PortPowerStatus */
#define M_OHCI_RHPORTSTAT_LSDA	_OHCI_MAKEMASK1(9)/* LowSpeedDeviceAttached */
#define M_OHCI_RHPORTSTAT_CSC	_OHCI_MAKEMASK1(16)/* ConnectStatusChange */
#define M_OHCI_RHPORTSTAT_PESC	_OHCI_MAKEMASK1(17)/* PortEnableStatusChange */
#define M_OHCI_RHPORTSTAT_PSSC	_OHCI_MAKEMASK1(18)/* PortSuspendStatusChange */
#define M_OHCI_RHPORTSTAT_OCIC	_OHCI_MAKEMASK1(19)/* PortOCIndicatorChange */
#define M_OHCI_RHPORTSTAT_PRSC	_OHCI_MAKEMASK1(20)/* PortResetStatusChange */

#define M_OHCI_RHPORTSTAT_ALLC (M_OHCI_RHPORTSTAT_CSC | \
				M_OHCI_RHPORTSTAT_PSSC | \
				M_OHCI_RHPORTSTAT_OCIC | \
				M_OHCI_RHPORTSTAT_PRSC)

typedef struct ohci_reg_s {
	u32_t revision;
	u32_t control;
	u32_t cmdstatus;
	u32_t intstatus;
	u32_t intenable;
	u32_t intdisable;
	u32_t hcca;
	u32_t periodcurrented;
	u32_t controlheaded;
	u32_t controlcurrented;
	u32_t bulkheaded;
	u32_t bulkcurrented;
	u32_t donehead;
	u32_t fminterval;
	u32_t fmremaining;
	u32_t fmnumber;
	u32_t periodicstart;
	u32_t lsthreshold;
	u32_t rhdscra;
	u32_t rhdscrb;
	u32_t rhstatus;
	u32_t rhport00;
	u32_t rhport01;
	u32_t rhport02;
	 REG8_RSVD(0x860, 0x1200);
	u32_t phyctrlp0;
	 REG8_RSVD(0x1204, 0x1300);
	u32_t framelenadj;
	u32_t shimctrl;
	u32_t shimslavestatus;
	u32_t shimmasterstatus;
	 REG8_RSVD(0x1310, 0x1314);
	u32_t ehcistatus;
	 REG8_RSVD(0x1318, 0x1510);
	u32_t utmip0ctl;
	u32_t tpctl;
	 REG8_RSVD(0x1518, 0x1538);
	u32_t tpoutp0;
	 REG8_RSVD(0x153c, 0x1604);
	u32_t ohcipwrsts;
	u32_t ehcixfer;
	u32_t ehcilpsmcstate;
	 REG8_RSVD(0x1610, 0x1700);
	u32_t ohciehcistrap;
	u32_t ohciccssts;
	 REG8_RSVD(0x1708, 0x170c);
	u32_t intrcontrol;
	 REG8_RSVD(0x1710, 0x1800);
	u32_t ehciincrxstrap;
} ohci_reg_t;

#define REG_WR(reg, val)	((reg) = (val))
#define REG_MOD_OR(reg, val)	((reg) |= (val))
#define REG_MOD_AND(reg, val)	((reg) &= (val))
#define REG_MOD_MASK(reg, mask, val)	((reg) = (((reg) & (mask)) | (val)))

/*
 *  OHCI Structures
 */

#define beginningof(ptr, type, field) ((type *)(((int) (ptr)) - \
					((int) ((type *) 0)->field)))

/* DO-NOT CHANGE ITS DEPTH OF INTR TREE */
#define OHCI_INTTREE_SIZE	32	/*63 */

/* Increase as per requirements */
#define OHCI_EDPOOL_SIZE	64	/*128 */

/* Increase as per requirements */
#define OHCI_TDPOOL_SIZE	32	/* 32 */

typedef struct ohci_endpoint_s {
	struct ohci_endpoint_s *ep_next;
	u32_t ep_phys;
	u32_t ep_flags;
	u32_t ep_mps;
	u32_t ep_num;
} ohci_endpoint_t;

typedef struct ohci_transfer_s {
	void *t_ref;
	u32_t t_length;
	struct ohci_transfer_s *t_next;
} __attribute__((aligned(ALIGNMENT)))ohci_transfer_t; //make each object 64 byte sized

#define q_init(q) (q)->q_prev = (q), (q)->q_next = (q)
#define q_isempty(q) ((q)->q_next == (q))
#define q_getfirst(q) ((q)->q_next)
#define q_getlast(q) ((q)->q_prev)

typedef struct queue_s {
	struct queue_s *q_next;
	struct queue_s *q_prev;
} queue_t;

typedef struct ohci_softc_s {
	ohci_endpoint_t *ohci_edtable[OHCI_INTTREE_SIZE];
	ohci_endpoint_t *ohci_inttable[OHCI_INTTABLE_SIZE];
	ohci_endpoint_t *ohci_isoc_list;
	ohci_endpoint_t *ohci_ctl_list;
	ohci_endpoint_t *ohci_bulk_list;
	ohci_hcca_t *ohci_hcca;
	ohci_endpoint_t *ohci_endpoint_pool;
	ohci_transfer_t *ohci_transfer_pool;
	ohci_ed_t *ohci_hwedpool;
	ohci_td_t *ohci_hwtdpool;
	ohci_endpoint_t *ohci_endpoint_freelist;
	ohci_transfer_t *ohci_transfer_freelist;

	volatile u32_t *ohci_regs;

	int ohci_ndp;
	long ohci_addr;
	u32_t ohci_intdisable;

	int ohci_rh_newaddr;	/*Address to be set on next status update */
	int ohci_rh_addr;	/*address of root hub */
	int ohci_rh_conf;	/*current configuration # */
	u8_t ohci_rh_buf[128];	/*buffer to hold hub responses */
	u8_t *ohci_rh_ptr;	/*pointer into buffer */
	int ohci_rh_len;	/*remaining bytes to transfer */
	queue_t ohci_rh_intrq;	/*Interrupt request queue */
	struct usbbus *ohci_bus;	/*owning usbbus structure */

} ohci_softc_t;

#define OHCI_RESET_DELAY	10

extern const struct usb_hcd ohci_driver;
extern volatile ohci_reg_t *ohci_reg;

void ohci_reg_dump(volatile ohci_reg_t *ohci_reg);
extern volatile ohci_reg_t *ohci_reg;

#endif
