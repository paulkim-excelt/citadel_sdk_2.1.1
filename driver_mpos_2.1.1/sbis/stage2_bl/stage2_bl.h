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

#ifndef __STAGE2_BL_H__
#define __STAGE2_BL_H__

#include <reg_access.h>
#include <sotp.h>

#define IPROC_TRUE  0x55 //01010101b
#define IPROC_FALSE 0x66 //01100110b

#define POWER_STATUS_POR	0x1
#define POWER_STATUS_PIN_RESET	0x2
#define POWER_STATUS_SOFT_RESET	0x3
#define POWER_STATUS_SLEEP	0x4
#define POWER_STATUS_DEEPSLEEP	0x5

#define RSA_2048_KEY_BYTES	256
#define RSA_4096_KEY_BYTES	512
#define RSA_4096_MONT_CTX_BYTES 1576

#define SHA256_KEY_SIZE         	32

typedef uint16_t IPROC_PUB_KEYTYPE;

typedef struct _device_keys{
	uint32_t  Khmac_fixed[SHA256_KEY_SIZE>>2];
	uint32_t  Kbase_Rsa_pubE;				/* small enough to save it as one word */
} DEVICE_KEYS;

extern const DEVICE_KEYS device_keys;

/***** BOOTROM State definition *****/
#define IPROC_BOOTSTATE_INIT			0x0000 /* Initial state */
#define IPROC_BOOTSTATE_CPUINIT			0x0100 /* CPU and Stack Initialization done */
#define IPROC_BOOTSTATE_INIT_DONE		0x0101 /* Initialization done */

#define IPROC_BOOTSTATE_CFGINTF			0x0200
#define IPROC_BOOTSTATE_CFGINTF_POWER		0x0201 /* Determine Boot Source, Turn On Boot Interface Power Done */
#define IPROC_BOOTSTATE_CFGINTF_CUSTID		0x0202 /* Read Customer ID from SOTP done */
#define IPROC_BOOTSTATE_CFGINTF_CONFIG		0x0203 /* Boot Interface Config from Pin done */
#define IPROC_BOOTSTATE_CFGINTF_SBI_UNAUTH_HD	0x0204 /* Read in Unauthenticated SBI Header Done */
#define IPROC_BOOTSTATE_CFGINTF_PLL_CONFIG	0x0205 /* PLL Configuration Done */

#define IPROC_BOOTSTATE_FIPS			0x0300 /* FIPS self tests started */
#define IPROC_BOOTSTATE_FIPS_HMAC256		0x0301 /* FIPS HMAC-SHA256 Self Test Done */
#define IPROC_BOOTSTATE_FIPS_AES		0x0302 /* FIPS AES Self Test Done */
#define IPROC_BOOTSTATE_FIPS_RSA		0x0303 /* FIPS RSA Signature Verification Self Test Done */
#define IPROC_BOOTSTATE_FIPS_ECDSA		0x0304 /* FIPS ECDSA Signature Verification Self Test Done */

#define IPROC_BOOTSTATE_SBILD			0x0400 /* SBI image load stage starts */
#define IPROC_BOOTSTATE_SOTP_TOC_LIST		0x0401 /* Valid TOC list found */
#define IPROC_BOOTSTATE_SOTP_TOC_ENTRY		0x0402 /* Valid TOC entry found */
#define IPROC_BOOTSTATE_SBILD_HEADER		0x0403 /* SBI header read in done */
#define IPROC_BOOTSTATE_SBILD_HEADERCK		0x0404 /* SBI header check done */
#define IPROC_BOOTSTATE_SBILD_IMAGE		0x0405 /* SBI image load done */
#define IPROC_BOOTSTATE_SBILD_AUTH		0x0406 /* SBI Auth data load done */
#define IPROC_BOOTSTATE_SBILD_FASTAUTH		0x0407 /* SBI image load stage starts for Fast Authentication Mode */
#define IPROC_BOOTSTATE_SBILD_FASTAUTH_HEADER	0x0408 /* SBI header read in done for Fast Authentication Mode */
#define IPROC_BOOTSTATE_SBILD_FASTAUTH_IMAGE	0x0409 /* SBI image load done for Fast Authentication Mode */
#define IPROC_BOOTSTATE_SBILD_NONAB		0x0410 /* SBI image load stage starts for non AB Mode */
#define IPROC_BOOTSTATE_SBILD_NONAB_IMAGE	0x0411 /* SBI image load done for non AB Mode */

#define IPROC_BOOTSTATE_BBL_KEK_READ		0x0500 /* BBL KEK Read */
#define IPROC_BOOTSTATE_CHECK_BBL_INTERFACE_ACTIVE	0x0501 /* Check if BBL Interface is Active */

#define IPROC_BOOTSTATE_SOTP_START		0x0600 /* SOTP access started */
#define IPROC_BOOTSTATE_SOTP_DEVCFG_READ	0x0601 /* Read Device Config successfully */
#define IPROC_BOOTSTATE_SOTP_CUSTID_READ	0x0602 /* Read Customer ID successfully */
#define IPROC_BOOTSTATE_SOTP_CUSTCFG_READ	0x0603 /* Read Customer Config successfully */
#define IPROC_BOOTSTATE_SOTP_KHMAC_READ		0x0604 /* Read KHMAC */
#define IPROC_BOOTSTATE_SOTP_KAES_READ		0x0605 /* Read KAES */
#define IPROC_BOOTSTATE_SOTP_DAUTH_READ		0x0606 /* Read DAUTH */
#define IPROC_BOOTSTATE_SOTP_FINISH_READ	0x0607 /* Finished Reading SOTP */
#define IPROC_BOOTSTATE_SOTP_SBLCONFIG_READ	0x0608 /* Read SBLConfiguration successfully */

#define IPROC_BOOTSTATE_SBIVF_HMAC_SHA256_START		0x0700 /* SBI symmetric verification stage starts */
#define IPROC_BOOTSTATE_SBIVF_HASH_AES			0x0701 /* SBI image hash generate done and optional AES decryption done */
#define IPROC_BOOTSTATE_SBIVF_HMAC_SHA256_SUCCESS	0x0702 /* SBI symmetric verification done */
#define IPROC_BOOTSTATE_SBIVF_ASYM_START		0x0703 /* SBI image Asymmetric verification stage starts */
#define IPROC_BOOTSTATE_SBIVF_AUTH_CHAIN1		0x0704 /* SBI first chain of verification done (including customer ID etc.) */
#define IPROC_BOOTSTATE_SBIVF_AUTH_CHAIN2		0x0705 /* SBI second chain of verification done (including customer ID etc.) */
#define IPROC_BOOTSTATE_SBIVF_AUTH_CHAIN3		0x0706 /* SBI third chain of verification done (including customer ID etc.) */
#define IPROC_BOOTSTATE_SBIVF_AUTH_CHAIN		0x0707 /* SBI chain of verification done (including customer ID etc.) */
#define IPROC_BOOTSTATE_SBIVF_ASYM_SUCCESS		0x0708 /* SBI image Asymmetric verification stage done */

#define IPROC_BOOTSTATE_JUMP2SBI		0x0800 /* Bootloader is done, will jump to SBI now */
#define IPROC_BOOTSTATE_NONAB_JUMP2SBI		0x0801 /* Non AB boot - jump to SBI */
#define IPROC_BOOTSTATE_FASTAUTH_JUMP2SBI	0x0802 /* Fast Authentication - jump to SBI */

#define IPROC_BOOTSTATE_FASTXIP_CHECK_BBL_INTERFACE_ACTIVE	0x0900 /* Check if BBL Interface is Active for Fast XIP */
#define IPROC_BOOTSTATE_FASTXIP_BBL_READ_CONFIG		0x0901 /* Start read of SMAU Config from BBL */
#define IPROC_BOOTSTATE_FASTXIP_BBL_JUMP2SBI		0x0902 /* Fast XIP (BBL) - jump to SBI */
#define IPROC_BOOTSTATE_FASTXIP_AON_READ_CONFIG		0x0903 /* Start read of SMAU Config from CRMU */
#define IPROC_BOOTSTATE_FASTXIP_AON_JUMP2SBI		0x0904 /* Fast XIP (CRMU) - jump to SBI */
#define IPROC_BOOTSTATE_FASTXIP_CHECK_COMPLETED		0x0905 /* Fast XIP check completed - couldnt find SMAU Config */

#define IPROC_AUTH_TYPE_HMAC_SHA256		0x00
#define IPROC_AUTH_TYPE_RSA			0x01
#define IPROC_AUTH_TYPE_ECDSA			0x02
#define IPROC_MAX_AUTH_SIZE			512 /* RSA is the biggest size, 512 x 8 = 4096 bits */
#define IPROC_RSA_2048_KEY_BYTES		256
#define IPROC_RSA_4096_KEY_BYTES		512
#define IPROC_RSA_4096_MONT_CTX_BYTES		1576
#define IPROC_AES_128_KEY_BYTES			(128/8)
#define IPROC_AES_256_KEY_BYTES			(256/8)
#define IPROC_EC_P256_KEY_BYTES			64
#define IPROC_HMAC_SHA256_KEY_BYTES		32
#define IPROC_KEY_ALG_RSA			0x0001
#define IPROC_KEY_ALG_EC			0x0002
#define BBL_KEK_LENGTH_BYTES			(BBL_KEK_LENGTH/8)

#define IPROC_DEV_CONFIG_RSA			(0x1 << 4)	/* 0x0010 */
#define IPROC_DEV_CONFIG_ECDSA			(0x1 << 5)	/* 0x0020 */

#define CID_MFGDEBUG				(CID_TYPE_PROD | 0x000FFFFE)

#endif	/* __STAGE2_BL_H__ */
