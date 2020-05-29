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

/* @file crypto_rng.h
 *
 *
 * This file contains
 *
 */

/**
 * TODO
 * 1. Put these into the crypto_pka.h header file, RSA_SIZE_MODULUS_DEFAULT
 */
#ifndef  CRYPTORNG_H
#define  CRYPTORNG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include <device.h>
#include <board.h>
#include "crypto.h"

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/

/* RNG defines start */
/* Registers */
#define RNG_CTL_REG             (volatile uint32_t *)(RNG_BASE_ADDR + 0x0)
/* Status register */
#define RNG_STAT_REG            (volatile uint32_t *)(RNG_BASE_ADDR + 0x4)
/* Data register */
#define RNG_DATA_REG            (volatile uint32_t *)(RNG_BASE_ADDR + 0x8)
/* FIFO threshold register */
#define RNG_THRESHOLD_REG       (volatile uint32_t *)(RNG_BASE_ADDR + 0xC)
/* Interrupt Control register */
#define RNG_INT_REG             (volatile uint32_t *)(RNG_BASE_ADDR + 0x10)

/* Control Register fields */
#define RNG_CTL_EN        0x0001

/* Status register fields */
/* bit 0 to 19, default value 0x40000 */
#define RNG_STAT_WARM_MSK            0x0FFFFF
#define RNG_STAT_VALIDWORDS_SHIFT    24
#define RNG_STAT_VALIDWORDS_MASK     0x0FF

#define RNG_GET_AVAILABLE_WORDS() \
         ((*RNG_STAT_REG >> RNG_STAT_VALIDWORDS_SHIFT) & \
		  RNG_STAT_VALIDWORDS_MASK)
/* the maximum number of RNG words availble from RNG core */
#define RNG_MAX_NUM_WORDS 16

/* Sizes are in bytes */
#define RNG_FIPS_NONCE_SIZE                  16  /* 128 bits of nonce */
#define RNG_FIPS_MAX_PERSONAL_STRING         32  /* max number of
					personalization string bytes */
#define RNG_FIPS_MAX_ADDITIONAL_INPUT        32  /* max number of
						additional info bytes */
/* we hash different combinations of the three above + 1 byte */
#define RNG_FIPS_TEMP_SIZE                   (55*3 + 1)

#define RNG_FIPS_SECURITY_STRENGTH           256

#define RNG_FIPS_MAX_RESEED_COUNT            100
#define RNG_FIPS_MAX_REQUEST                 4096

#define CRYPTO_INDEX_RNG     2

/* put these into the crypto_pka.h header file */
#define RSA_SIZE_MODULUS_DEFAULT     256 /* default RSA modulus size */
#define RSA_SIZE_SIGNATURE_DEFAULT   RSA_SIZE_MODULUS_DEFAULT
#define ECC_SIZE_P256                32

/* For DSA-X random number generation */
#define SHA_INIT_STATE0          0x67452301
#define SHA_INIT_STATE1          0xEFCDAB89
#define SHA_INIT_STATE2          0x98BADCFE
#define SHA_INIT_STATE3          0x10325476
#define SHA_INIT_STATE4          0xC3D2E1F0

/* RNG types */
/* FIPS 186-2 original standard */
#define BCM_SCAPI_RNG_TYPE_DSA_X                        0x0001
#define BCM_SCAPI_RNG_TYPE_DSA_K                        0x0002
/* FIPS 186-2 change notice dated 10/05/2001 */
#define BCM_SCAPI_RNG_TYPE_DSA_X_CHG                    0x0004
#define BCM_SCAPI_RNG_TYPE_DSA_K_CHG                    0x0008
/* General Purpose RNG, "mod q" is omitted */
#define BCM_SCAPI_RNG_TYPE_OTHER                        0x0010
#define BCM_SCAPI_RNG_TYPE_SKIP_CONTINUOUS_CHK          0x0020
#define BCM_SCAPI_RNG_TYPE_MASK_ORG         	(BCM_SCAPI_RNG_TYPE_DSA_X |\
						BCM_SCAPI_RNG_TYPE_DSA_K)
#define BCM_SCAPI_RNG_TYPE_MASK_CHG             (BCM_SCAPI_RNG_TYPE_DSA_X_CHG |\
						BCM_SCAPI_RNG_TYPE_DSA_K_CHG)
#define BCM_SCAPI_RNG_TYPE_MASK_K               (BCM_SCAPI_RNG_TYPE_DSA_K |\
						BCM_SCAPI_RNG_TYPE_DSA_K_CHG)

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/
/**
 * This API intializes the fips rng
 * @param[in] pHandle:   crypto handle
 * @param[in] rngctx:    user allocated memory, will be used as rng context.
 * @param[in] prediction resistance: reseed every request
 * @param[in] entropy_input:         32 bytes of entropy if testing
 * @param[in] entropy_length:        Entropy length
 * @param[in] nonce:                 16 bytes of nonce if desired/testing
 * @param[in] nonce_length:          Nonce length
 * @param[in] personal_str:   optional personalization string (0-64 bytes)
 * @param[in] personal_str_len:      len of personal_str in bytes
 * @return Appropriate status
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_rng_init(crypto_lib_handle *pHandle,
                   fips_rng_context *rngctx,
                   u8_t prediction_resist,
                   u32_t *entropy_input, u8_t entropy_length,
                   u32_t *nonce_input, u8_t nonce_length,
                   u8_t *personal_str, u8_t personal_str_len);

/**
 * This API implements SP800-90A reseed
 *
 * @param[in] pHandle:              crypto handle
 * @param[in] entropy_input:        32 bytes of entropy if testing
 * @param[in] entropy_length: Entropy length
 * @param[in] additional_input:     optional additional input (0-64 bytes)
 * @param[in] additional_input_len: len of additional_input in bytes*
 * @return Appropriate status
 * @retval #BCM_SCAPI_STATUS
 */
 BCM_SCAPI_STATUS crypto_rng_reseed(crypto_lib_handle *pHandle,
                 u32_t *entropy_input, u8_t entropy_length,
                 u8_t *additional_input, u8_t additional_input_len);

/**
 * This API implement SP800-90A generate
 * @param[in] pHandle: crypto handle
 * @param[in] entropy_input:32 bytes of entropy if testing
 * @param[in] entropy_length: Entropy length
 * @param[inout] output: output buffer
 * @param[in] output_len: requested length of RNG in bytes
 * @param[in] additional_input: optional additional input (0-64 bytes)
 * @param[in] additional_input_len: len of additional_input in bytes
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_rng_generate(crypto_lib_handle *pHandle,
                     u32_t *entropy_input, u8_t entropy_length,
                     u8_t *output, u32_t output_len,
                     u8_t *additional_input, u8_t additional_input_len);

/**
 * This API Generate a block of random bytes.
 * This is not a user API, user should always call the fips version
 * @param[in] pHandle: crypto handle
 * @param[in] result: pointer to the final output memory
 * @param[in] num_words: the number of RNG words that the caller needs
 * @param[in] callback : Callback function pointer
 * @param[in] arg : Void argument for utility purposes
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_rng_raw_generate (
                    crypto_lib_handle *pHandle,
                    u8_t  *result, u32_t num_words,
                    scapi_callback callback, void *arg);

#endif /* CRYPTORNG_H */
