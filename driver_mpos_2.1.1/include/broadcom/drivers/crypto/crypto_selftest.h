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
 * This file contains the
 *
 */

#ifndef  CRYPTO_SELFTEST_H
#define  CRYPTO_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include "crypto.h"
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
BCM_SCAPI_STATUS crypto_rng_selftest(crypto_lib_handle *pHandle,
				     fips_rng_context *rngctx);
BCM_SCAPI_STATUS crypto_selftest_sha1(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_sha256(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_hmac_sha256(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_sha3_224(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_sha3_256(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_sha3_384(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_sha3_512(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_aes_cbc(crypto_lib_handle * pHandle);
BCM_SCAPI_STATUS crypto_selftest_aes_ctr(crypto_lib_handle * pHandle);
BCM_SCAPI_STATUS crypto_selftest_aes_128_ccm(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_aes_cbc_sha256_hmac(
		crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_des(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_selftest_3des(crypto_lib_handle *pHandle);
#endif /* CRYPTO_SELFTEST_H */
