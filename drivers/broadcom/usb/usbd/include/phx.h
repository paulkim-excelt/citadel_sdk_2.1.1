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

/* @file phx.h
 *
 * FIXME remove later.
 *
 *
 */

#ifndef __PHX_H
#define __PHX_H

/**********************************************************************
 *  Status Codes
 **********************************************************************/
typedef enum phx_status_code {
    PHX_STATUS_OK                           =  0,
    PHX_STATUS_OK_NEED_DMA                  =  1,

    PHX_EXCEPTION_NMI                       = 10, /* exceptions in secure mode */
    PHX_EXCEPTION_HARD_FAULT,
    PHX_EXCEPTION_MEM,
    PHX_EXCEPTION_BUS_FAULT,
    PHX_EXCEPTION_USAGE,
    PHX_EXCEPTION_RESERVED,
    PHX_EXCEPTION_SVC,
    PHX_EXCEPTION_DEBUG,
    PHX_EXCEPTION_PENDSV,
    PHX_EXCEPTION_SYSTICK,
    PHX_EXCEPTION_IRQ,
    PHX_EXCEPTION_DEFAULT,                        /* this is not a separate exception, this is the default */

    PHX_STATUS_MBIST_FAIL_PKE               = 30,
    PHX_STATUS_MBIST_FAIL_IDTCM,
    PHX_STATUS_MBIST_FAIL_SCRATCH,
    PHX_STATUS_MBIST_FAIL,                        /* general case, includes any above */

    PHX_EXCEPTION_OPEN_UNDEFINED            = 40, /* exception in open mode */
    PHX_EXCEPTION_OPEN_SWI,
    PHX_EXCEPTION_OPEN_PRE_ABT,
    PHX_EXCEPTION_OPEN_DATA_ABT,
    PHX_EXCEPTION_OPEN_RESEVED,
    PHX_EXCEPTION_OPEN_IRQ,
    PHX_EXCEPTION_OPEN_FIQ,
    PHX_EXCEPTION_OPEN_NMI,
    PHX_EXCEPTION_OPEN_HARD_FAULT,
    PHX_EXCEPTION_OPEN_DEFAULT,

    PHX_STATUS_NO_DEVICE                    = 50,
    PHX_STATUS_DEVICE_BUSY,
    PHX_STATUS_DEVICE_FAILED,
    PHX_STATUS_NO_MEMORY,
    PHX_STATUS_NO_RESOURCES,
    PHX_STATUS_NO_BUFFERS,
    PHX_STATUS_PARAMETER_INVALID,
    PHX_STATUS_TIMED_OUT,
    PHX_STATUS_UNAUTHORIZED,
    PHX_STATUS_INVALID_DIVP,                       /* new for 5880 */

    PHX_STATUS_BSC_DEVICE_NO_ACK            = 80,
    PHX_STATUS_BSC_DEVICE_INCOMPLETE,
    PHX_STATUS_BSC_TIMEOUT,
    PHX_STATUS_BBL_BUSY                     = 90,
    PHX_STATUS_CRYPTO_ERROR                 = 100,

    PHX_STATUS_CRYPTO_NEED2CALLS,
    PHX_STATUS_CRYPTO_PLAINTEXT_TOOLONG,
    PHX_STATUS_CRYPTO_AUTH_FAILED,
    PHX_STATUS_CRYPTO_ENCR_UNSUPPORTED,
    PHX_STATUS_CRYPTO_AUTH_UNSUPPORTED,
    PHX_STATUS_CRYPTO_MATH_UNSUPPORTED,
    PHX_STATUS_CRYPTO_WRONG_STEP,
    PHX_STATUS_CRYPTO_KEY_SIZE_MISMATCH,
    PHX_STATUS_CRYPTO_AES_LEN_NOTALIGN,
    PHX_STATUS_CRYPTO_AES_OFFSET_NOTALIGN,
    PHX_STATUS_CRYPTO_AUTH_OFFSET_NOTALIGN,
    PHX_STATUS_CRYPTO_DEVICE_NO_OPERATION,
    PHX_STATUS_CRYPTO_NOT_DONE,
    PHX_STATUS_CRYPTO_RNG_NO_ENOUGH_BITS,
    PHX_STATUS_CRYPTO_RNG_RAW_COMPARE_FAIL,       /* generate rng is same as what saved last time */
    PHX_STATUS_CRYPTO_RNG_SHA_COMPARE_FAIL,       /* rng after hash is same as what saved last time */
    PHX_STATUS_CRYPTO_TIMEOUT,
    PHX_STATUS_CRYPTO_AUTH_LENGTH_NOTALIGN,       /* for sha init/update */
    PHX_STATUS_CRYPTO_RN_PRIME,
    PHX_STATUS_CRYPTO_RN_NOT_PRIME,
    PHX_STATUS_CRYPTO_QLIP_CTX_OVERFLOW,

    PHX_STATUS_RSA_PKCS1_LENGTH_NOT_MATCH   = 150, /* PKCS1 related error codes */
    PHX_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL,
    PHX_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT,

    PHX_STATUS_SELFTEST_FAIL                = 200,
    PHX_STATUS_SELFTEST_3DES_FAIL,
    PHX_STATUS_SELFTEST_DES_FAIL,
    PHX_STATUS_SELFTEST_AES_FAIL,
    PHX_STATUS_SELFTEST_SHA1_FAIL,
    PHX_STATUS_SELFTEST_DH_FAIL,
    PHX_STATUS_SELFTEST_RSA_FAIL,
    PHX_STATUS_SELFTEST_DSA_FAIL,
    PHX_STATUS_SELFTEST_DSA_SIGN_FAIL,
    PHX_STATUS_SELFTEST_DSA_VERIFY_FAIL,
    PHX_STATUS_SELFTEST_DSA_PAIRWISE_FAIL,
    PHX_STATUS_SELFTEST_ECDSA_SIGN_FAIL,
    PHX_STATUS_SELFTEST_ECDSA_VERIFY_FAIL,
    PHX_STATUS_SELFTEST_ECDSA_PAIRWISE_FAIL,
    PHX_STATUS_SELFTEST_ECDSA_PAIRWISE_INTRN_FAIL,
    PHX_STATUS_SELFTEST_MATH_FAIL,
    PHX_STATUS_SELFTEST_RNG_FAIL,
    PHX_STATUS_SELFTEST_MODEXP_FAIL,
    PHX_STATUS_SELFTEST_MODINV_FAIL,
    PHX_STATUS_SELFTEST_SHA256_FAIL,

    PHX_STATUS_DMA_NO_FREE_CHANNEL          = 250,
    PHX_STATUS_DMA_INVALID_CHANNEL,
    PHX_STATUS_DMA_BUFLIST_FILLED,
    PHX_STATUS_DMA_ERROR,
    PHX_STATUS_DMA_CHANNEL_ACTIVE,       // channel halt, but still has data in fifo
    PHX_STATUS_DMA_CHANNEL_BUSY,
    PHX_STATUS_DMA_CHANNEL_FREE,
    PHX_STATUS_DMA_NOT_DONE,

    PHX_STATUS_SMAU_SCRATCH_BUSY            = 300,
    PHX_STATUS_SMAU_SCRATCH_NOT_DONE,
    PHX_STATUS_SMAU_DMA_NOT_DONE,
    PHX_STATUS_SMAU_DMA_ERROR,

    PHX_STATUS_SPI_FLASH_PROTECTED          = 350,
    PHX_STATUS_SPI_WRITE_NOTSUPPORT,
    PHX_STATUS_SPI_READ_FIFO_EMPTY,
    PHX_STATUS_SPI_WRITE_FIFO_FULL,
    PHX_STATUS_SPI_DATA_STORE_NULL,
    PHX_STATUS_SPI_WRITE_VERIFY_FAILED,
    PHX_STATUS_SPI_NOT_SUPPORTED_FUNCTION,
    PHX_STATUS_SPI_NOT_PROTECTED,
    PHX_STATUS_SPI_UNKNOWN_FLASH,

    PHX_STATUS_USB_DEVICE_NOTSUPPORT        = 360, /* SSMC ADDR PINs set to host mode, so device mode not support */
    PHX_STATUS_USB_DMA_CHAIN_INVALID,
    PHX_STATUS_USB_DMA_TX_DESC_ERR,
    PHX_STATUS_USB_DMA_TX_BUFF_ERR,
    PHX_STATUS_USB_DMA_TX_UNKNOWN_STATUS,
    PHX_STATUS_USB_DMA_RX_NOTDONE,
    PHX_STATUS_USB_SUSPENDED,
    PHX_STATUS_USB_REQUEST_UNKNOWN,
    PHX_STATUS_USB_REQUEST_WRONG_STRING,
    PHX_STATUS_USB_TIMEDOUT,
    PHX_STATUS_USB_CONTRLINIT_FAIL_CONFIG,
    PHX_STATUS_USB_READ_INT_TIMEDOUT,
    PHX_STATUS_USB_WRITE_INT_TIMEDOUT,
    PHX_STATUS_USB_DMA_DONE_TIMEDOUT,
    PHX_STATUS_USB_CONTROL_WAIT_TIMEDOUT,
    PHX_STATUS_USB_READ_INSUFFICIENT_DATA,

    PHX_STATUS_USB_OFFSET_GT_READ_OFFSET,

    PHX_STATUS_LOTP_UNPROG                  = 400,
    PHX_STATUS_LOTP_PROG_OVERFLOW,		   /* program row overflow */
    PHX_STATUS_LOTP_FAIL_PROG_FAILED,		   /* min 2 fail bits program failed */
    PHX_STATUS_LOTP_RD_OVERFLOW,		   /* out of bounds overflow, while reading mfg keys array */
    PHX_STATUS_LOTP_RD_CRC_FAIL,		   /* reading mfg keys array yields crc error. */
    PHX_STATUS_LOTP_RD_SCFG_NOT_MATCHED,	   /* SCFG bits doesn't match with those of DCFG status register. */
    PHX_STATUS_LOTP_ILENGTH_SMALL,		   /* Length passed in is smaller than the one programmed */
    PHX_STATUS_OEM_DEPROG_ERR,                     /* OEM programming 1->0 */
    PHX_STATUS_OEM_PROG_ERR,                       /* OEM programming 0->1 failed */
    PHX_STATUS_CUSTID_MISMATCH,
    PHX_STATUS_CUSTREV_MISMATCH,
    PHX_STATUS_SVID_MISMATCH,
    PHX_STATUS_OTP2T_BUSY                   = 430, /* hardware is reading now */
    PHX_STATUS_OTP2T_READ_EMPTY,
    PHX_STATUS_OTP2T_PROG_FAIL,

    PHX_STATUS_BTSTRAP_HASH_NOTMATCH        = 450, /* BOOTSTRAP */
    PHX_STATUS_BTSTRAP_UNKNOWN_BTINTERFACE,
    PHX_STATUS_BTSTRAP_SYM_VERIFY_NOTALLOW,
    PHX_STATUS_BTSTRAP_UNKNOWN_AUTH_METHOD,
    PHX_STATUS_BTSTRAP_BTSRC_BOUNDARY_CHECK_FAIL,

    PHX_STATUS_SBI_MAGIC_NOTMATCH_UNAUTH    = 500, /* unauthenticated SBI header magic does not match */
    PHX_STATUS_SBI_MAGIC_NOTMATCH_AUTH,            /* authenticated SBI header magic does not match */
    PHX_STATUS_SBI_NO_IMAGE,
    PHX_STATUS_SBI_SIZE_TOO_BIG,
    PHX_STATUS_SBI_HASH_NOTMATCH,   /* for symm verification */
    PHX_STATUS_SBI_DSASIG_NOTMATCH, /* for asym verification NO DSA for 5880 */
    PHX_STATUS_SBI_NO_RELEASEINFO,       /* Release info payload is not there */

    PHX_STATUS_SBI_OFFSET_NOTALIGN,      /* SBI header offsets not aligned (auth payload offset and sbi code offset */
    PHX_STATUS_SBI_STARTVEC_NOTALIGN,    /* the starting vector is not aligned */

    PHX_STATUS_SBI_CHAIN_OF_TRUST_FAIL,
    PHX_STATUS_SBI_CHAIN_BTROM_REV_NOTMATCH,
    PHX_STATUS_SBI_CHAIN_CUST_ID_NOTMATCH,
    PHX_STATUS_SBI_CHAIN_CUST_REV_NOTMATCH,
    PHX_STATUS_SBI_CHAIN_CUST_REV_NOTMATCH_BRCM,

    PHX_STATUS_SBI_NO_CHAIN,


    PHX_STATUS_SC_NO_ATR                    = 700, /* Either no smart card, or synchronous card which we don't support */
    PHX_STATUS_SC_ATR_TCK_FAIL,         /* check sum of ATR failed */
    PHX_STATUS_SC_ATR_TOO_MANY_SETS,    /* too much sets of data in the ATR response */
    PHX_STATUS_SC_RX_TIMEOUT,           /* wait for Rx byte, timed out */
    PHX_STATUS_SC_TX_TIMEOUT,           /* wait for Tx byte, timed out (normally does not happen) */
    PHX_STATUS_SC_T0_BUF_TOO_SHORT,     /* send out buf too short */
    PHX_STATUS_SC_T0_BUF_WRONG_LEN,     /* send out buf contains a wrong length */
    PHX_STATUS_SC_T0_WRONG_INS,
    PHX_STATUS_SC_PTS_FAIL,
    PHX_STATUS_SC_CARD_NOT_SUPPORTED,

    PHX_STATUS_SMC_AHBMODE_INVALID          = 750,
    PHX_STATUS_SMC_AMD_FLASH_PRM_ERROR,

    /* 7000 - 8000  -- reserved for os api's */

    /* 8000 - 9000  -- reserved for ushx api's */
    PHX_INVALID_ARG = 8000,
    PHX_DEVICE_FAILED ,
    PHX_FLASH_ACCESS_FAILURE ,
} PHX_STATUS;
#endif
