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

#ifndef __IPROC_SOTP_H__
#define __IPROC_SOTP_H__

/* sizes in bytes */
#define OTP_SIZE_SCFG            4  /* 4 bytes of SCFG bits   */
#define OTP_SIZE_DCFG            1  /* 1 bytes of DCFG bits */
#define OTP_SIZE_KAES            32 /* 32 bytes of AES key  */
#define OTP_SIZE_KHMAC_SHA256    32 /* 32 bytes of HMAC-SHA256 key */
#define OTP_SIZE_DAUTH            32 /* 32 bytes of DAUTH key   */
#define OTP_SIZE_KDI_PRIV        32 /* 32 bytes of ECDSA P-256 private key */
#define OTP_SIZE_KBIND            32 /* 32 bytes of AES-256 binding key */

#define OTP_SIZE_ALL_KEYS (4+1+2+2+2+2+4+2+2+OTP_SIZE_KAES + \
			   OTP_SIZE_KHMAC_SHA256 + \
			   OTP_SIZE_DAUTH + \
			   OTP_SIZE_KDI_PRIV + \
			   OTP_SIZE_KBIND) /* 140 bytes */

#define OTP_SCFG_MANFOTPPRGCOMPLETE_MASK    0x0003
#define OTP_SCFG_CIDPRGCOMPLETE_MASK        0x000C
#define OTP_SCFG_NONABDEVICE_MASK            0x0030
#define OTP_SCFG_ABDEVICE_MASK                0x00C0
#define OTP_SCFG_DEVDEVICE_MASK                0x0300
#define OTP_SCFG_VOLTAGEMONITORENABLED_MASK 0x0C00

#define OTP_DCFG_AUTHENTICATE_INPLACE        0x0001
#define OTP_DCFG_AUTHTYPE_RSA                0x0010
#define OTP_DCFG_AUTHTYPE_ECDSA                0x0020
#define OTP_DCFG_AUTHTYPE_SYMMETRIC            0x0040
#define OTP_DCFG_ENCR_AES128                0x0100
#define OTP_DCFG_ENCR_AES256                0x0200
#define OTP_DCFG_DECRYPT_BEFORE_AUTH        0x0400

#define OTP_DCFG_SBLCONFIG_KBASE_ECDSA        (0x1 << 29) // 0x20000000
#define OTP_DCFG_SBLCONFIG_USEKEK            (0x1 << 30) // 0x40000000
#define OTP_DCFG_SBLCONFIG_NSFORRECOVERY    (0x1 << 31) // 0x80000000

#define NONAB_BOOT              0x06CC66CC
#define AB_PENDING              0x05AA55AA
#define AB_PRODUCTION           0x65AA55AA
#define AB_DEVELOPMENT          0x95AA55AA
#define AB_FASTAUTH             0x09CC66CC
#define NONAB_BOOT              0x06CC66CC
#define UNASSIGNED_BOOT         0x90CC66CC
#define UNPROGRAMMED_BOOT       0x60CC66CC

#define SDD_OFF_L 12
#define SEC_CONFIG_DEV_NAB_OFF_L 32
#define SEC_CONFIG_DEV_UA_OFF_L 15
#define SEC_CONFIG_DEV_AB_OFF_L 0
#define SEC_CONFIG_DEV_AB_ROW12_OFF_L 40
#define SEC_CONFIG_MASK 0x1FF
#define JTAG_DIS_FOR_A7 (1 << 3)

#define SOTP_SECTION3_SERAS_EN_OFFSET  14
#define SOTP_SECTION3_EDONE_OFFSET  16
#define SOTP_SECTION3_ROMLOCK_EN_OFFSET  8
#define SOTP_SECTION4_CLASSID_OFFSET 9
#define SMAU_INIT_CAM 0x50020000
#define SOTP_SECTION4_UART_EN_OFFSET 22
#define SOTP_SECTION4_CACHE_EN_OFFSET 28
#define SECTION3_ROW14_CPUCLK_OFFSET 24
#define SECTION3_ROW14_CPUCLK_MASK 0xF
/*
 * Inverted versions of Encoded Device Type values.
 */
#define INV_AB_PRODUCTION (~AB_PRODUCTION)
#define INV_AB_PENDING (~AB_PENDING)
#define INV_AB_DEVELOPMENT (~AB_DEVELOPMENT)
#define INV_AB_FASTAUTH (~AB_FASTAUTH)
#define INV_NONAB_BOOT (~NONAB_BOOT)
#define INV_UNASSIGNED_BOOT (~UNASSIGNED_BOOT)

#endif /* __IPROC_SOTP_H__ */
