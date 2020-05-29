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

/* @file crypto_pka_selftest.h
 *
 *
 * This file contains the
 *
 */

#ifndef  CRYPTO_PKA_SELFTEST_H
#define  CRYPTO_PKA_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <zephyr/types.h>
#include <crypto/crypto.h>
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

BCM_SCAPI_STATUS crypto_pka_selftest_modexp(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_modinv(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_rsa_keygen(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_rsa_enc(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_rsa_dec(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_pkcs1_rsassa_v15_sig(
		crypto_lib_handle *handle);
BCM_SCAPI_STATUS crypto_pka_selftest_pkcs1_rsassa_v15_verify(
		crypto_lib_handle *pHandle);

BCM_SCAPI_STATUS crypto_pka_selftest_dsa_sign(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_dsa_verify(crypto_lib_handle *pHandle);

BCM_SCAPI_STATUS crypto_pka_selftest_dsa2048_sign_verify(
                                                     crypto_lib_handle *handle);
BCM_SCAPI_STATUS crypto_pka_selftest_dh(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_selftest_ecdh(crypto_lib_handle *pHandle);

BCM_SCAPI_STATUS crypto_pka_fips_selftest_ecdh(crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_fips_test_ecdh_key(crypto_lib_handle *pHandle,
		u8_t* A_priv, u8_t* A_pubx,
		u8_t* A_puby);
BCM_SCAPI_STATUS crypto_pka_selftest_ecdsa_sign_verify(
                                                    crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_cust_selftest_ecdsa_p256_sign_verify(
                                        crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_cust_selftest_ecp_diffie_hellman_shared_secret(
                                                crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_cust_selftest_ecdsa_p256_key_generate(
                                    crypto_lib_handle *pHandle);
BCM_SCAPI_STATUS crypto_pka_cust_selftest_ec_point_verify(
                                            crypto_lib_handle *pHandle);
#endif /* CRYPTO_PKA_SELFTEST_H */
