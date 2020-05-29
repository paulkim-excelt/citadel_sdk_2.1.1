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

/* @file crypto_sw_utils.h
 *
 * crypto utils header file
 *
 * This header declares the common driver functions for crypto
 */

#ifndef __CRYPTO_SW_UTILS_H__
#define __CRYPTO_SW_UTILS_H__

#define CRYPTO_DATA_SIZE		(131072)
#define AES128_KEY_SIZE			(16)
#define AES256_KEY_SIZE			(32)
#define CRYPTO_DES_KEY_SIZE		(8)
#define CRYPTO_DES_BLK_SIZE		(8)
#define CRYPTO_3DES_KEY_SIZE		(24)
#define CRYPTO_3DES_BLK_SIZE		(8)
#define SHA1_BLK_SIZE			(64)
#define SHA256_BLK_SIZE			(64)
#define DPA_KEY_SIZE			(6)

#define MASK_G_POLY			(0x163)
#define MASK_G				(0x63)

#define AES_G_POLY			(0x11b)
#define AES_G				(0x1b)

#ifdef CRYPTO_DEBUG
#define crypto_debug(format, args...)		\
do {						\
	printk((format), ##args)		\
} while (0)
#else
#define crypto_debug(format, args...)
#endif

#define CRYPTO_RESULT(result, str)					      \
do {									      \
	if ((result) != 0) {						      \
		crypto_debug("%s(%d): %s\n", __func__, __LINE__, (str));      \
		return result;						      \
	}								      \
} while (0)

#define crypto_assert(value)						      \
do {									      \
	if (0 == (value)) {						      \
		crypto_debug("%s(%d): assert failed!\n", __func__, __LINE__); \
		return -1;						      \
	}                                                                     \
} while (0)

#define __brcm_xor_shuffle(res, i0, i1, rnd) {				\
	res = ((~((rnd)&0x01)) & ((i0)^(i1))) | ((i1) ^ (i0));		\
}

#define __brcm_xor_3(res, i0, i1, i2, rnd) {				\
	__brcm_xor_shuffle((res), (i0), (i1), (rnd));			\
	__brcm_xor_shuffle((res), (res), (i2), (((rnd) >> 8) & 0xFF));	\
}

#define __brcm_delay(n, u) {				\
	volatile u8_t cs, ce;				\
							\
	cs = (u8_t)(u);					\
	ce = (u8_t)(u) + (u8_t)((n) & 0x03);		\
	do {						\
		if (++cs >= ce)				\
			break;				\
	} while (1);					\
}

#define __brcm_rnd_delay() {				\
	u32_t random;					\
	volatile u8_t cs, ce;				\
							\
	bcm_rng_readwords(&random, 1);			\
	cs = (u8_t)(random);				\
	ce = (u8_t)(random) + (u8_t)((random>>8) & 0x03);	\
	do {						\
		if (++cs >= ce)				\
			break;				\
	} while (1);					\
}

#define NUM_CRYPTO_CTX       3  /* PKE/MATH, BULK, RNG */
#define PHX_CRYPTO_LIB_HANDLE_SIZE (NUM_CRYPTO_CTX*sizeof(crypto_lib_handle))

#define _R0  _rnd_i0
#define _R1  _rnd_i1
#define _R2  _rnd_i2

s32_t crypto_swap_end(u32_t *ptr);

BCM_SCAPI_STATUS bcm_rng_readwords(u32_t *word, s32_t word_num);

BCM_SCAPI_STATUS aes_dpa_enable(void);

void __xor_buf(u8_t *in0, u8_t *in1, u8_t *out, u32_t len);

extern int _rnd_i0, _rnd_i1, _rnd_i2;

#endif
