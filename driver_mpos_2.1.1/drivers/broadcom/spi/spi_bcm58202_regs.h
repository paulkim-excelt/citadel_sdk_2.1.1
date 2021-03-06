/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/* @file spi_bcm58202_regs.h
 *
 * SPI register defintions for bcm58202
 *
 * This header defines the register addresses and field positions
 * and masks for the SPI controller registers on bcm58202
 *
 */

#ifndef	__SPI_BCM58202_REGS_H__
#define	__SPI_BCM58202_REGS_H__

#define SPI_SSPCR0_BASE 0x000
#define SPI_SSPCR1_BASE 0x004
#define SPI_SSPDR_BASE 0x008
#define SPI_SSPSR_BASE 0x00C
#define SPI_SSPCPSR_BASE 0x010
#define SPI_SSPI1MSC_BASE 0x014
#define SPI_SSPRIS_BASE 0x018
#define SPI_SSPMIS_BASE 0x01C
#define SPI_SSPI1CR_BASE 0x020
#define SPI_SSPDMACR_BASE 0x024
#define SPI_SSPTCR_BASE 0x080
#define SPI_SSPI1TIP_BASE 0x084
#define SPI_SSPI1TOP_BASE 0x088
#define SPI_SSPTDR_BASE 0x08C
#define SPI_SSPPERIPHID0_BASE 0xFE0
#define SPI_SSPPERIPHID1_BASE 0xFE4
#define SPI_SSPPERIPHID2_BASE 0xFE8
#define SPI_SSPPERIPHID3_BASE 0xFEC
#define SPI_SSPPCELLID0_BASE 0xFF0
#define SPI_SSPPCELLID1_BASE 0xFF4
#define SPI_SSPPCELLID2_BASE 0xFF8
#define SPI_SSPPCELLID3_BASE 0xFFC

#define SPI_SSPCR0__SCR_L 15
#define SPI_SSPCR0__SCR_R 8
#define SPI_SSPCR0__SCR_WIDTH 8
#define SPI_SSPCR0__SCR_RESETVALUE 0x00
#define SPI_SSPCR0__SPH_L 7
#define SPI_SSPCR0__SPH_R 7
#define SPI_SSPCR0__SPH 7
#define SPI_SSPCR0__SPH_WIDTH 1
#define SPI_SSPCR0__SPH_RESETVALUE 0x0
#define SPI_SSPCR0__SPO_L 6
#define SPI_SSPCR0__SPO_R 6
#define SPI_SSPCR0__SPO 6
#define SPI_SSPCR0__SPO_WIDTH 1
#define SPI_SSPCR0__SPO_RESETVALUE 0x0
#define SPI_SSPCR0__FRF_L 5
#define SPI_SSPCR0__FRF_R 4
#define SPI_SSPCR0__FRF_WIDTH 2
#define SPI_SSPCR0__FRF_RESETVALUE 0x0
#define SPI_SSPCR0__DSS_L 3
#define SPI_SSPCR0__DSS_R 0
#define SPI_SSPCR0__DSS_WIDTH 4
#define SPI_SSPCR0__DSS_RESETVALUE 0x0
#define SPI_SSPCR0__RESERVED_L 31
#define SPI_SSPCR0__RESERVED_R 16
#define SPI_SSPCR0_WIDTH 16
#define SPI_SSPCR0__WIDTH 16
#define SPI_SSPCR0_ALL_L 15
#define SPI_SSPCR0_ALL_R 0
#define SPI_SSPCR0__ALL_L 15
#define SPI_SSPCR0__ALL_R 0
#define SPI_SSPCR0_DATAMASK 0x0000FFFF
#define SPI_SSPCR0_RESETVALUE 0x0
#define SPI_SSPCR1__SOD_L 3
#define SPI_SSPCR1__SOD_R 3
#define SPI_SSPCR1__SOD 3
#define SPI_SSPCR1__SOD_WIDTH 1
#define SPI_SSPCR1__SOD_RESETVALUE 0x0
#define SPI_SSPCR1__MS_L 2
#define SPI_SSPCR1__MS_R 2
#define SPI_SSPCR1__MS 2
#define SPI_SSPCR1__MS_WIDTH 1
#define SPI_SSPCR1__MS_RESETVALUE 0x0
#define SPI_SSPCR1__SSE_L 1
#define SPI_SSPCR1__SSE_R 1
#define SPI_SSPCR1__SSE 1
#define SPI_SSPCR1__SSE_WIDTH 1
#define SPI_SSPCR1__SSE_RESETVALUE 0x0
#define SPI_SSPCR1__LBM_L 0
#define SPI_SSPCR1__LBM_R 0
#define SPI_SSPCR1__LBM 0
#define SPI_SSPCR1__LBM_WIDTH 1
#define SPI_SSPCR1__LBM_RESETVALUE 0x0
#define SPI_SSPCR1__RESERVED_L 31
#define SPI_SSPCR1__RESERVED_R 4
#define SPI_SSPCR1_WIDTH 4
#define SPI_SSPCR1__WIDTH 4
#define SPI_SSPCR1_ALL_L 3
#define SPI_SSPCR1_ALL_R 0
#define SPI_SSPCR1__ALL_L 3
#define SPI_SSPCR1__ALL_R 0
#define SPI_SSPCR1_DATAMASK 0x0000000F
#define SPI_SSPCR1_RESETVALUE 0x0
#define SPI_SSPDR__DATA_L 15
#define SPI_SSPDR__DATA_R 0
#define SPI_SSPDR__DATA_WIDTH 16
#define SPI_SSPDR__DATA_RESETVALUE 0x0000
#define SPI_SSPDR__RESERVED_L 31
#define SPI_SSPDR__RESERVED_R 16
#define SPI_SSPDR_WIDTH 16
#define SPI_SSPDR__WIDTH 16
#define SPI_SSPDR_ALL_L 15
#define SPI_SSPDR_ALL_R 0
#define SPI_SSPDR__ALL_L 15
#define SPI_SSPDR__ALL_R 0
#define SPI_SSPDR_DATAMASK 0x0000FFFF
#define SPI_SSPDR_RESETVALUE 0x0
#define SPI_SSPSR__BSY_L 4
#define SPI_SSPSR__BSY_R 4
#define SPI_SSPSR__BSY 4
#define SPI_SSPSR__BSY_WIDTH 1
#define SPI_SSPSR__BSY_RESETVALUE 0x0
#define SPI_SSPSR__RFF_L 3
#define SPI_SSPSR__RFF_R 3
#define SPI_SSPSR__RFF 3
#define SPI_SSPSR__RFF_WIDTH 1
#define SPI_SSPSR__RFF_RESETVALUE 0x0
#define SPI_SSPSR__RNE_L 2
#define SPI_SSPSR__RNE_R 2
#define SPI_SSPSR__RNE 2
#define SPI_SSPSR__RNE_WIDTH 1
#define SPI_SSPSR__RNE_RESETVALUE 0x0
#define SPI_SSPSR__TNF_L 1
#define SPI_SSPSR__TNF_R 1
#define SPI_SSPSR__TNF 1
#define SPI_SSPSR__TNF_WIDTH 1
#define SPI_SSPSR__TNF_RESETVALUE 0x1
#define SPI_SSPSR__TFE_L 0
#define SPI_SSPSR__TFE_R 0
#define SPI_SSPSR__TFE 0
#define SPI_SSPSR__TFE_WIDTH 1
#define SPI_SSPSR__TFE_RESETVALUE 0x1
#define SPI_SSPSR__RESERVED_L 31
#define SPI_SSPSR__RESERVED_R 5
#define SPI_SSPSR_WIDTH 5
#define SPI_SSPSR__WIDTH 5
#define SPI_SSPSR_ALL_L 4
#define SPI_SSPSR_ALL_R 0
#define SPI_SSPSR__ALL_L 4
#define SPI_SSPSR__ALL_R 0
#define SPI_SSPSR_DATAMASK 0x0000001F
#define SPI_SSPSR_RESETVALUE 0x3
#define SPI_SSPCPSR__CPSDVSR_L 7
#define SPI_SSPCPSR__CPSDVSR_R 0
#define SPI_SSPCPSR__CPSDVSR_WIDTH 8
#define SPI_SSPCPSR__CPSDVSR_RESETVALUE 0x00
#define SPI_SSPCPSR__RESERVED_L 31
#define SPI_SSPCPSR__RESERVED_R 8
#define SPI_SSPCPSR_WIDTH 8
#define SPI_SSPCPSR__WIDTH 8
#define SPI_SSPCPSR_ALL_L 7
#define SPI_SSPCPSR_ALL_R 0
#define SPI_SSPCPSR__ALL_L 7
#define SPI_SSPCPSR__ALL_R 0
#define SPI_SSPCPSR_DATAMASK 0x000000FF
#define SPI_SSPCPSR_RESETVALUE 0x0
#define SPI_SSPI1MSC__TXIM_L 3
#define SPI_SSPI1MSC__TXIM_R 3
#define SPI_SSPI1MSC__TXIM 3
#define SPI_SSPI1MSC__TXIM_WIDTH 1
#define SPI_SSPI1MSC__TXIM_RESETVALUE 0x0
#define SPI_SSPI1MSC__RXIM_L 2
#define SPI_SSPI1MSC__RXIM_R 2
#define SPI_SSPI1MSC__RXIM 2
#define SPI_SSPI1MSC__RXIM_WIDTH 1
#define SPI_SSPI1MSC__RXIM_RESETVALUE 0x0
#define SPI_SSPI1MSC__RTIM_L 1
#define SPI_SSPI1MSC__RTIM_R 1
#define SPI_SSPI1MSC__RTIM 1
#define SPI_SSPI1MSC__RTIM_WIDTH 1
#define SPI_SSPI1MSC__RTIM_RESETVALUE 0x0
#define SPI_SSPI1MSC__RORIM_L 0
#define SPI_SSPI1MSC__RORIM_R 0
#define SPI_SSPI1MSC__RORIM 0
#define SPI_SSPI1MSC__RORIM_WIDTH 1
#define SPI_SSPI1MSC__RORIM_RESETVALUE 0x0
#define SPI_SSPI1MSC__RESERVED_L 31
#define SPI_SSPI1MSC__RESERVED_R 4
#define SPI_SSPI1MSC_WIDTH 4
#define SPI_SSPI1MSC__WIDTH 4
#define SPI_SSPI1MSC_ALL_L 3
#define SPI_SSPI1MSC_ALL_R 0
#define SPI_SSPI1MSC__ALL_L 3
#define SPI_SSPI1MSC__ALL_R 0
#define SPI_SSPI1MSC_DATAMASK 0x0000000F
#define SPI_SSPI1MSC_RESETVALUE 0x0
#define SPI_SSPRIS__TXRIS_L 3
#define SPI_SSPRIS__TXRIS_R 3
#define SPI_SSPRIS__TXRIS 3
#define SPI_SSPRIS__TXRIS_WIDTH 1
#define SPI_SSPRIS__TXRIS_RESETVALUE 0x1
#define SPI_SSPRIS__RXRIS_L 2
#define SPI_SSPRIS__RXRIS_R 2
#define SPI_SSPRIS__RXRIS 2
#define SPI_SSPRIS__RXRIS_WIDTH 1
#define SPI_SSPRIS__RXRIS_RESETVALUE 0x0
#define SPI_SSPRIS__RTRIS_L 1
#define SPI_SSPRIS__RTRIS_R 1
#define SPI_SSPRIS__RTRIS 1
#define SPI_SSPRIS__RTRIS_WIDTH 1
#define SPI_SSPRIS__RTRIS_RESETVALUE 0x0
#define SPI_SSPRIS__RORRIS_L 0
#define SPI_SSPRIS__RORRIS_R 0
#define SPI_SSPRIS__RORRIS 0
#define SPI_SSPRIS__RORRIS_WIDTH 1
#define SPI_SSPRIS__RORRIS_RESETVALUE 0x0
#define SPI_SSPRIS__RESERVED_L 31
#define SPI_SSPRIS__RESERVED_R 4
#define SPI_SSPRIS_WIDTH 4
#define SPI_SSPRIS__WIDTH 4
#define SPI_SSPRIS_ALL_L 3
#define SPI_SSPRIS_ALL_R 0
#define SPI_SSPRIS__ALL_L 3
#define SPI_SSPRIS__ALL_R 0
#define SPI_SSPRIS_DATAMASK 0x0000000F
#define SPI_SSPRIS_RESETVALUE 0x8
#define SPI_SSPMIS__TXMIS_L 3
#define SPI_SSPMIS__TXMIS_R 3
#define SPI_SSPMIS__TXMIS 3
#define SPI_SSPMIS__TXMIS_WIDTH 1
#define SPI_SSPMIS__TXMIS_RESETVALUE 0x0
#define SPI_SSPMIS__RXMIS_L 2
#define SPI_SSPMIS__RXMIS_R 2
#define SPI_SSPMIS__RXMIS 2
#define SPI_SSPMIS__RXMIS_WIDTH 1
#define SPI_SSPMIS__RXMIS_RESETVALUE 0x0
#define SPI_SSPMIS__RTMIS_L 1
#define SPI_SSPMIS__RTMIS_R 1
#define SPI_SSPMIS__RTMIS 1
#define SPI_SSPMIS__RTMIS_WIDTH 1
#define SPI_SSPMIS__RTMIS_RESETVALUE 0x0
#define SPI_SSPMIS__RORMIS_L 0
#define SPI_SSPMIS__RORMIS_R 0
#define SPI_SSPMIS__RORMIS 0
#define SPI_SSPMIS__RORMIS_WIDTH 1
#define SPI_SSPMIS__RORMIS_RESETVALUE 0x0
#define SPI_SSPMIS__RESERVED_L 31
#define SPI_SSPMIS__RESERVED_R 4
#define SPI_SSPMIS_WIDTH 4
#define SPI_SSPMIS__WIDTH 4
#define SPI_SSPMIS_ALL_L 3
#define SPI_SSPMIS_ALL_R 0
#define SPI_SSPMIS__ALL_L 3
#define SPI_SSPMIS__ALL_R 0
#define SPI_SSPMIS_DATAMASK 0x0000000F
#define SPI_SSPMIS_RESETVALUE 0x0
#define SPI_SSPI1CR__RTIC_L 1
#define SPI_SSPI1CR__RTIC_R 1
#define SPI_SSPI1CR__RTIC 1
#define SPI_SSPI1CR__RTIC_WIDTH 1
#define SPI_SSPI1CR__RTIC_RESETVALUE 0x0
#define SPI_SSPI1CR__RORIC_L 0
#define SPI_SSPI1CR__RORIC_R 0
#define SPI_SSPI1CR__RORIC 0
#define SPI_SSPI1CR__RORIC_WIDTH 1
#define SPI_SSPI1CR__RORIC_RESETVALUE 0x0
#define SPI_SSPI1CR__RESERVED_L 31
#define SPI_SSPI1CR__RESERVED_R 2
#define SPI_SSPI1CR_WIDTH 2
#define SPI_SSPI1CR__WIDTH 2
#define SPI_SSPI1CR_ALL_L 1
#define SPI_SSPI1CR_ALL_R 0
#define SPI_SSPI1CR__ALL_L 1
#define SPI_SSPI1CR__ALL_R 0
#define SPI_SSPI1CR_DATAMASK 0x00000003
#define SPI_SSPI1CR_RESETVALUE 0x0
#define SPI_SSPDMACR__TXDMAE_L 1
#define SPI_SSPDMACR__TXDMAE_R 1
#define SPI_SSPDMACR__TXDMAE 1
#define SPI_SSPDMACR__TXDMAE_WIDTH 1
#define SPI_SSPDMACR__TXDMAE_RESETVALUE 0x0
#define SPI_SSPDMACR__RXDMAE_L 0
#define SPI_SSPDMACR__RXDMAE_R 0
#define SPI_SSPDMACR__RXDMAE 0
#define SPI_SSPDMACR__RXDMAE_WIDTH 1
#define SPI_SSPDMACR__RXDMAE_RESETVALUE 0x0
#define SPI_SSPDMACR__RESERVED_L 31
#define SPI_SSPDMACR__RESERVED_R 2
#define SPI_SSPDMACR_WIDTH 2
#define SPI_SSPDMACR__WIDTH 2
#define SPI_SSPDMACR_ALL_L 1
#define SPI_SSPDMACR_ALL_R 0
#define SPI_SSPDMACR__ALL_L 1
#define SPI_SSPDMACR__ALL_R 0
#define SPI_SSPDMACR_DATAMASK 0x00000003
#define SPI_SSPDMACR_RESETVALUE 0x0
#define SPI_SSPTCR__TESTFIFO_L 1
#define SPI_SSPTCR__TESTFIFO_R 1
#define SPI_SSPTCR__TESTFIFO 1
#define SPI_SSPTCR__TESTFIFO_WIDTH 1
#define SPI_SSPTCR__TESTFIFO_RESETVALUE 0x0
#define SPI_SSPTCR__ITEN_L 0
#define SPI_SSPTCR__ITEN_R 0
#define SPI_SSPTCR__ITEN 0
#define SPI_SSPTCR__ITEN_WIDTH 1
#define SPI_SSPTCR__ITEN_RESETVALUE 0x0
#define SPI_SSPTCR__RESERVED_L 31
#define SPI_SSPTCR__RESERVED_R 2
#define SPI_SSPTCR_WIDTH 2
#define SPI_SSPTCR__WIDTH 2
#define SPI_SSPTCR_ALL_L 1
#define SPI_SSPTCR_ALL_R 0
#define SPI_SSPTCR__ALL_L 1
#define SPI_SSPTCR__ALL_R 0
#define SPI_SSPTCR_DATAMASK 0x00000003
#define SPI_SSPTCR_RESETVALUE 0x0
#define SPI_SSPI1TIP__SSPRXDMACLR_L 3
#define SPI_SSPI1TIP__SSPRXDMACLR_R 3
#define SPI_SSPI1TIP__SSPRXDMACLR 3
#define SPI_SSPI1TIP__SSPRXDMACLR_WIDTH 1
#define SPI_SSPI1TIP__SSPRXDMACLR_RESETVALUE 0x0
#define SPI_SSPI1TIP__SSPCLKIN_L 2
#define SPI_SSPI1TIP__SSPCLKIN_R 2
#define SPI_SSPI1TIP__SSPCLKIN 2
#define SPI_SSPI1TIP__SSPCLKIN_WIDTH 1
#define SPI_SSPI1TIP__SSPCLKIN_RESETVALUE 0x0
#define SPI_SSPI1TIP__SSPFSSIN_L 1
#define SPI_SSPI1TIP__SSPFSSIN_R 1
#define SPI_SSPI1TIP__SSPFSSIN 1
#define SPI_SSPI1TIP__SSPFSSIN_WIDTH 1
#define SPI_SSPI1TIP__SSPFSSIN_RESETVALUE 0x0
#define SPI_SSPI1TIP__SSPRXD_L 0
#define SPI_SSPI1TIP__SSPRXD_R 0
#define SPI_SSPI1TIP__SSPRXD 0
#define SPI_SSPI1TIP__SSPRXD_WIDTH 1
#define SPI_SSPI1TIP__SSPRXD_RESETVALUE 0x0
#define SPI_SSPI1TIP__RESERVED_L 31
#define SPI_SSPI1TIP__RESERVED_R 4
#define SPI_SSPI1TIP_WIDTH 4
#define SPI_SSPI1TIP__WIDTH 4
#define SPI_SSPI1TIP_ALL_L 3
#define SPI_SSPI1TIP_ALL_R 0
#define SPI_SSPI1TIP__ALL_L 3
#define SPI_SSPI1TIP__ALL_R 0
#define SPI_SSPI1TIP_DATAMASK 0x0000000F
#define SPI_SSPI1TIP_RESETVALUE 0x0
#define SPI_SSPI1TOP__NSSPCTLOE_L 3
#define SPI_SSPI1TOP__NSSPCTLOE_R 3
#define SPI_SSPI1TOP__NSSPCTLOE 3
#define SPI_SSPI1TOP__NSSPCTLOE_WIDTH 1
#define SPI_SSPI1TOP__NSSPCTLOE_RESETVALUE 0x0
#define SPI_SSPI1TOP__SSPCLKOUT_L 2
#define SPI_SSPI1TOP__SSPCLKOUT_R 2
#define SPI_SSPI1TOP__SSPCLKOUT 2
#define SPI_SSPI1TOP__SSPCLKOUT_WIDTH 1
#define SPI_SSPI1TOP__SSPCLKOUT_RESETVALUE 0x0
#define SPI_SSPI1TOP__SSPFSSOUT_L 1
#define SPI_SSPI1TOP__SSPFSSOUT_R 1
#define SPI_SSPI1TOP__SSPFSSOUT 1
#define SPI_SSPI1TOP__SSPFSSOUT_WIDTH 1
#define SPI_SSPI1TOP__SSPFSSOUT_RESETVALUE 0x0
#define SPI_SSPI1TOP__SSPTXD_L 0
#define SPI_SSPI1TOP__SSPTXD_R 0
#define SPI_SSPI1TOP__SSPTXD 0
#define SPI_SSPI1TOP__SSPTXD_WIDTH 1
#define SPI_SSPI1TOP__SSPTXD_RESETVALUE 0x0
#define SPI_SSPI1TOP__RESERVED_L 31
#define SPI_SSPI1TOP__RESERVED_R 4
#define SPI_SSPI1TOP_WIDTH 4
#define SPI_SSPI1TOP__WIDTH 4
#define SPI_SSPI1TOP_ALL_L 3
#define SPI_SSPI1TOP_ALL_R 0
#define SPI_SSPI1TOP__ALL_L 3
#define SPI_SSPI1TOP__ALL_R 0
#define SPI_SSPI1TOP_DATAMASK 0x0000000F
#define SPI_SSPI1TOP_RESETVALUE 0x0
#define SPI_SSPTDR__DATA_L 15
#define SPI_SSPTDR__DATA_R 0
#define SPI_SSPTDR__DATA_WIDTH 16
#define SPI_SSPTDR__DATA_RESETVALUE 0x0000
#define SPI_SSPTDR__RESERVED_L 31
#define SPI_SSPTDR__RESERVED_R 16
#define SPI_SSPTDR_WIDTH 16
#define SPI_SSPTDR__WIDTH 16
#define SPI_SSPTDR_ALL_L 15
#define SPI_SSPTDR_ALL_R 0
#define SPI_SSPTDR__ALL_L 15
#define SPI_SSPTDR__ALL_R 0
#define SPI_SSPTDR_DATAMASK 0x0000FFFF
#define SPI_SSPTDR_RESETVALUE 0x0
#define SPI_SSPPERIPHID0__PARTNUMBER0_L 7
#define SPI_SSPPERIPHID0__PARTNUMBER0_R 0
#define SPI_SSPPERIPHID0__PARTNUMBER0_WIDTH 8
#define SPI_SSPPERIPHID0__PARTNUMBER0_RESETVALUE 0x22
#define SPI_SSPPERIPHID0__RESERVED_L 31
#define SPI_SSPPERIPHID0__RESERVED_R 8
#define SPI_SSPPERIPHID0_WIDTH 8
#define SPI_SSPPERIPHID0__WIDTH 8
#define SPI_SSPPERIPHID0_ALL_L 7
#define SPI_SSPPERIPHID0_ALL_R 0
#define SPI_SSPPERIPHID0__ALL_L 7
#define SPI_SSPPERIPHID0__ALL_R 0
#define SPI_SSPPERIPHID0_DATAMASK 0x000000FF
#define SPI_SSPPERIPHID0_RESETVALUE 0x22
#define SPI_SSPPERIPHID1__DESIGNER0_L 7
#define SPI_SSPPERIPHID1__DESIGNER0_R 4
#define SPI_SSPPERIPHID1__DESIGNER0_WIDTH 4
#define SPI_SSPPERIPHID1__DESIGNER0_RESETVALUE 0x1
#define SPI_SSPPERIPHID1__PARTNUMBER1_L 3
#define SPI_SSPPERIPHID1__PARTNUMBER1_R 0
#define SPI_SSPPERIPHID1__PARTNUMBER1_WIDTH 4
#define SPI_SSPPERIPHID1__PARTNUMBER1_RESETVALUE 0x0
#define SPI_SSPPERIPHID1__RESERVED_L 31
#define SPI_SSPPERIPHID1__RESERVED_R 8
#define SPI_SSPPERIPHID1_WIDTH 8
#define SPI_SSPPERIPHID1__WIDTH 8
#define SPI_SSPPERIPHID1_ALL_L 7
#define SPI_SSPPERIPHID1_ALL_R 0
#define SPI_SSPPERIPHID1__ALL_L 7
#define SPI_SSPPERIPHID1__ALL_R 0
#define SPI_SSPPERIPHID1_DATAMASK 0x000000FF
#define SPI_SSPPERIPHID1_RESETVALUE 0x10
#define SPI_SSPPERIPHID2__REVISION_L 7
#define SPI_SSPPERIPHID2__REVISION_R 4
#define SPI_SSPPERIPHID2__REVISION_WIDTH 4
#define SPI_SSPPERIPHID2__REVISION_RESETVALUE 0x2
#define SPI_SSPPERIPHID2__DESIGNER1_L 3
#define SPI_SSPPERIPHID2__DESIGNER1_R 0
#define SPI_SSPPERIPHID2__DESIGNER1_WIDTH 4
#define SPI_SSPPERIPHID2__DESIGNER1_RESETVALUE 0x4
#define SPI_SSPPERIPHID2__RESERVED_L 31
#define SPI_SSPPERIPHID2__RESERVED_R 8
#define SPI_SSPPERIPHID2_WIDTH 8
#define SPI_SSPPERIPHID2__WIDTH 8
#define SPI_SSPPERIPHID2_ALL_L 7
#define SPI_SSPPERIPHID2_ALL_R 0
#define SPI_SSPPERIPHID2__ALL_L 7
#define SPI_SSPPERIPHID2__ALL_R 0
#define SPI_SSPPERIPHID2_DATAMASK 0x000000FF
#define SPI_SSPPERIPHID2_RESETVALUE 0x24
#define SPI_SSPPERIPHID3__CONFIGURATION_L 7
#define SPI_SSPPERIPHID3__CONFIGURATION_R 0
#define SPI_SSPPERIPHID3__CONFIGURATION_WIDTH 8
#define SPI_SSPPERIPHID3__CONFIGURATION_RESETVALUE 0x00
#define SPI_SSPPERIPHID3__RESERVED_L 31
#define SPI_SSPPERIPHID3__RESERVED_R 8
#define SPI_SSPPERIPHID3_WIDTH 8
#define SPI_SSPPERIPHID3__WIDTH 8
#define SPI_SSPPERIPHID3_ALL_L 7
#define SPI_SSPPERIPHID3_ALL_R 0
#define SPI_SSPPERIPHID3__ALL_L 7
#define SPI_SSPPERIPHID3__ALL_R 0
#define SPI_SSPPERIPHID3_DATAMASK 0x000000FF
#define SPI_SSPPERIPHID3_RESETVALUE 0x0
#define SPI_SSPPCELLID0__SSPPCELLID0_L 7
#define SPI_SSPPCELLID0__SSPPCELLID0_R 0
#define SPI_SSPPCELLID0__SSPPCELLID0_WIDTH 8
#define SPI_SSPPCELLID0__SSPPCELLID0_RESETVALUE 0x0D
#define SPI_SSPPCELLID0__RESERVED_L 31
#define SPI_SSPPCELLID0__RESERVED_R 8
#define SPI_SSPPCELLID0_WIDTH 8
#define SPI_SSPPCELLID0__WIDTH 8
#define SPI_SSPPCELLID0_ALL_L 7
#define SPI_SSPPCELLID0_ALL_R 0
#define SPI_SSPPCELLID0__ALL_L 7
#define SPI_SSPPCELLID0__ALL_R 0
#define SPI_SSPPCELLID0_DATAMASK 0x000000FF
#define SPI_SSPPCELLID0_RESETVALUE 0xD
#define SPI_SSPPCELLID1__SSPPCELLID1_L 7
#define SPI_SSPPCELLID1__SSPPCELLID1_R 0
#define SPI_SSPPCELLID1__SSPPCELLID1_WIDTH 8
#define SPI_SSPPCELLID1__SSPPCELLID1_RESETVALUE 0xF0
#define SPI_SSPPCELLID1__RESERVED_L 31
#define SPI_SSPPCELLID1__RESERVED_R 8
#define SPI_SSPPCELLID1_WIDTH 8
#define SPI_SSPPCELLID1__WIDTH 8
#define SPI_SSPPCELLID1_ALL_L 7
#define SPI_SSPPCELLID1_ALL_R 0
#define SPI_SSPPCELLID1__ALL_L 7
#define SPI_SSPPCELLID1__ALL_R 0
#define SPI_SSPPCELLID1_DATAMASK 0x000000FF
#define SPI_SSPPCELLID1_RESETVALUE 0xF0
#define SPI_SSPPCELLID2__SSPPCELLID2_L 7
#define SPI_SSPPCELLID2__SSPPCELLID2_R 0
#define SPI_SSPPCELLID2__SSPPCELLID2_WIDTH 8
#define SPI_SSPPCELLID2__SSPPCELLID2_RESETVALUE 0x05
#define SPI_SSPPCELLID2__RESERVED_L 31
#define SPI_SSPPCELLID2__RESERVED_R 8
#define SPI_SSPPCELLID2_WIDTH 8
#define SPI_SSPPCELLID2__WIDTH 8
#define SPI_SSPPCELLID2_ALL_L 7
#define SPI_SSPPCELLID2_ALL_R 0
#define SPI_SSPPCELLID2__ALL_L 7
#define SPI_SSPPCELLID2__ALL_R 0
#define SPI_SSPPCELLID2_DATAMASK 0x000000FF
#define SPI_SSPPCELLID2_RESETVALUE 0x5
#define SPI_SSPPCELLID3__SSPPCELLID3_L 7
#define SPI_SSPPCELLID3__SSPPCELLID3_R 0
#define SPI_SSPPCELLID3__SSPPCELLID3_WIDTH 8
#define SPI_SSPPCELLID3__SSPPCELLID3_RESETVALUE 0xB1
#define SPI_SSPPCELLID3__RESERVED_L 31
#define SPI_SSPPCELLID3__RESERVED_R 8
#define SPI_SSPPCELLID3_WIDTH 8
#define SPI_SSPPCELLID3__WIDTH 8
#define SPI_SSPPCELLID3_ALL_L 7
#define SPI_SSPPCELLID3_ALL_R 0
#define SPI_SSPPCELLID3__ALL_L 7
#define SPI_SSPPCELLID3__ALL_R 0
#define SPI_SSPPCELLID3_DATAMASK 0x000000FF
#define SPI_SSPPCELLID3_RESETVALUE 0xB1


/* SPI Status flags */
#define TX_NOT_FULL(DD)		\
	GET_FIELD(sys_read32(DD->base + SPI_SSPSR_BASE), SPI_SSPSR, TNF)

#define TX_FIFO_EMPTY(DD)	\
	GET_FIELD(sys_read32(DD->base + SPI_SSPSR_BASE), SPI_SSPSR, TFE)

#define RX_NOT_EMPTY(DD)	\
	GET_FIELD(sys_read32(DD->base + SPI_SSPSR_BASE), SPI_SSPSR, RNE)

#define RX_FIFO_FULL(DD)	\
	GET_FIELD(sys_read32(DD->base + SPI_SSPSR_BASE), SPI_SSPSR, RFF)

#define SPI_BUS_BUSY(DD)	\
	GET_FIELD(sys_read32(DD->base + SPI_SSPSR_BASE), SPI_SSPSR, BSY)

/* FIXME: These macros should be defined in a common header file */
/** Mask for the specified field, given register name and field name */
#define MASK(REG, FIELD)	\
	(((0x1 << REG##__##FIELD##_WIDTH) - 1) << REG##__##FIELD##_R)
/** Position of the specified register given register name and field name */
#define POS(REG, FIELD)		(REG##__##FIELD##_R)

/** Get Field Macro - From a u32_t */
#define GET_FIELD(VALUE, REG, FIELD)	   \
	((VALUE & MASK(REG, FIELD)) >> POS(REG, FIELD))

/** Set Field Macro - To a u32_t */
#define SET_FIELD(VALUE, REG, FIELD, VAL)	\
	((VALUE) =				\
		(((VALUE) & ~MASK(REG, FIELD))|	\
		((VAL) << POS(REG, FIELD) & MASK(REG, FIELD))))


/* FIXME: These should be defined in the GPIO header file
 */
#define PAD_CONTROL_HYS_EN_MASK			0x400
#define PAD_CONTROL_HYS_EN_SHIFT		10

#define PAD_CONTROL_DRV_STRENGTH_MASK		0x380
#define PAD_CONTROL_DRV_STRENGTH_SHIFT		7

#define PAD_CONTROL_PULL_DOWN_MASK		0x40
#define PAD_CONTROL_PULL_DOWN_SHIFT		6

#define PAD_CONTROL_PULL_UP_MASK		0x20
#define PAD_CONTROL_PULL_UP_SHIFT		5

#define PAD_CONTROL_SLEW_RATE_MASK		0x10
#define PAD_CONTROL_SLEW_RATE_SHIFT		4

#define PAD_CONTROL_INPUT_DIS_MASK		0x8
#define PAD_CONTROL_INPUT_DIS_SHIFT		3

#define PAD_CONTROL_FUNCTION_SEL_MASK		0x7
#define PAD_CONTROL_FUNCTION_SEL_SHIFT		0

#ifdef CONFIG_SPI_LEGACY_API
/* The legacy SPI driver does not have the vendor specific
 * config element in spi_config structure, so we use the
 * upper 16 bits of spi_config.config to pass the vendor
 * specific configuration information. Currenty this is used to pass
 * the pad control configuration for SPI pins (hysteresis, pull up/down,
 * drive strength, slew rate, etc ...)
 */
#define VENDOR_INFO_BIT_OFFSET	16
#define PAD_HYS_EN(A)				\
	(((A << PAD_CONTROL_HYS_EN_SHIFT)	\
		& PAD_CONTROL_HYS_EN_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_DRIVE_STRENGTH(A)			\
	(((A << PAD_CONTROL_DRV_STRENGTH_SHIFT)	\
		& PAD_CONTROL_DRV_STRENGTH_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_PULL_DOWN(A)			\
	(((A << PAD_CONTROL_PULL_DOWN_SHIFT)	\
		& PAD_CONTROL_PULL_DOWN_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_PULL_UP(A)				\
	(((A << PAD_CONTROL_PULL_UP_SHIFT)	\
		& PAD_CONTROL_PULL_UP_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_SLEW_RATE(A)			\
	(((A << PAD_CONTROL_SLEW_RATE_SHIFT)	\
		& PAD_CONTROL_SLEW_RATE_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_INPUT_DISABLE(A)			\
	(((A << PAD_CONTROL_INPUT_DIS_SHIFT)	\
		& PAD_CONTROL_INPUT_DIS_MASK) << VENDOR_INFO_BIT_OFFSET)

#define PAD_FUNCTION_SEL(A)			\
	(((A << PAD_CONTROL_FUNCTION_SEL_SHIFT)	\
		& PAD_CONTROL_FUNCTION_SEL_MASK) << VENDOR_INFO_BIT_OFFSET)
#else
/*
 * The pad control settings that need to be configured for
 * the SPI pins will be passed in the vendor field of spi_config
 * structure.
 */
#define PAD_HYS_EN(A)		\
	((A << PAD_CONTROL_HYS_EN_SHIFT) & PAD_CONTROL_HYS_EN_MASK)
#define PAD_DRIVE_STRENGTH(A)	\
	((A << PAD_CONTROL_DRV_STRENGTH_SHIFT) & PAD_CONTROL_DRV_STRENGTH_MASK)
#define PAD_PULL_DOWN(A)	\
	((A << PAD_CONTROL_PULL_DOWN_SHIFT) & PAD_CONTROL_PULL_DOWN_MASK)
#define PAD_PULL_UP(A)		\
	((A << PAD_CONTROL_PULL_UP_SHIFT) & PAD_CONTROL_PULL_UP_MASK)
#define PAD_SLEW_RATE(A)	\
	((A << PAD_CONTROL_SLEW_RATE_SHIFT) & PAD_CONTROL_SLEW_RATE_MASK)
#define PAD_INPUT_DISABLE(A)	\
	((A << PAD_CONTROL_INPUT_DIS_SHIFT) & PAD_CONTROL_INPUT_DIS_MASK)
#define PAD_FUNCTION_SEL(A)	\
	((A << PAD_CONTROL_FUNCTION_SEL_SHIFT) & PAD_CONTROL_FUNCTION_SEL_MASK)
#endif /* CONFIG_SPI_LEGACY_API */

#define NUM_GPIO_PER_REG	32
#define GPIO_PIN_COUNT_MASK	(NUM_GPIO_PER_REG - 1)

/* DMA peripheral IDs:
 * Fixme: This should come from the soc header file or DMA driver header
 */
#define DMAC_PERIPHERAL_SPI4_RX     1
#define DMAC_PERIPHERAL_SPI4_TX     2
#define DMAC_PERIPHERAL_SPI5_RX     3
#define DMAC_PERIPHERAL_SPI5_TX     4
#define DMAC_PERIPHERAL_SPI3_RX     16
#define DMAC_PERIPHERAL_SPI3_TX     17
#define DMAC_PERIPHERAL_SPI2_RX     18
#define DMAC_PERIPHERAL_SPI2_TX     19
#define DMAC_PERIPHERAL_SPI1_RX     20
#define DMAC_PERIPHERAL_SPI1_TX     21

#endif	/* __SPI_BCM58202_REGS_H__ */
