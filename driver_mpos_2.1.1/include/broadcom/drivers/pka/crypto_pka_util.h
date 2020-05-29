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

/* @file crypto_pka_util.c
 *
 * Utility Api's for the crypto pka
 *
 *
 *
 */

#ifndef CRYPTO_PKA_UTIL_H_
#define CRYPTO_PKA_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include <pka/q_lip.h>
#include <pka/q_lip_aux.h>
/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/
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
 *
 */
void secure_memclr(void* addr, unsigned len);

/**
 *
 */
void secure_memcpy(u8_t* to, const u8_t* from, unsigned len);

/**
 *
 */
void secure_memmove(u8_t* to, const u8_t* from, unsigned len);

/**
 *
 */
int secure_memeql(const u8_t* a, const u8_t* b, unsigned len);

/**
 *
 */
void secure_memrev(u8_t* to, const u8_t* from, unsigned len);

/**
 *
 */
void bcm_swap_words(unsigned int *pInt, unsigned int nLen);

/**
 *
 */
void bcm_swap_endian(unsigned int *pIn, unsigned int nLen,
                            unsigned int *pOut, unsigned int fByte);

/**
 *
 */
void bcm_int2bignum(unsigned int *pInt, unsigned int nLen);

/**
 *
 */
void set_param_bn(q_lint_t *qparam, u8_t * param,
                         u32_t param_len, u32_t pad_len);

unsigned
bcm_soft_add_byte2byte_plusX(u8_t *A, u32_t lenA, u8_t *B, u32_t lenB, u8_t x);

void fips_error();

#endif /* CRYPTO_PKA_UTIL_H_ */
