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

/* @file crypto_pka_ushx.c
 *
 * Customized API's
 *
 *
 *
 */
/**
 * TODO
 * 1.
 */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <zephyr/types.h>
#include <crypto/crypto_symmetric.h>
#include <crypto/crypto.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <pka/q_lip.h>
#include <pka/q_lip_aux.h>
#include <pka/q_context.h>
#include <pka/q_pka_hw.h>
#include <pka/q_lip_utils.h>

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/

/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ecp_ecdsa_key_generate
 * Purpose : Generate an Elliptic curve DSA Key Pair 
 * Input   : 
 *      	pHandle : Pointer to the Crypto handle
 *      	privateKey : Pointer to the privateKey  It must be 32 Bytes in size.
 *		publicKey : Pointer to the publicKey  It must be 64 Bytes in size.
 * Return  : On success(= 0) returns the Public Key
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_key_generate(
		crypto_lib_handle *pHandle,u8_t* privateKey, u8_t* publicKey)
{
    /* The prime modulus */
    u32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve; In FIPS 186-3,
     * the selection a = p-3 for the coefficient of x was made for
     * reasons of efficiency
    taken from section D.1.2.3 Page 89 for Curve P-256 */
    u32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve */
    u32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
    {
        0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };
    /* Order n */
    u32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
    {
        0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
        0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
    };
    /* The base point x coordinate Gx */
    u32_t ECC_P256DomainParams_Gx [ECC_SIZE_P256>>2] =
    {
        0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
        0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2
    };
    /* The base point y coordinate Gy */
    u32_t ECC_P256DomainParams_Gy [ECC_SIZE_P256>>2] =
    {
        0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
        0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2
    };

    return crypto_pka_ecp_ecdsa_key_generate(pHandle,
                        0, /* Curve Type = 0 (Prime field) */
                        (u8_t*)ECC_P256DomainParams_Prime,
                        (ECC_SIZE_P256)*8, /* P Bit len */
                        (u8_t*)ECC_P256DomainParams_a,
                        (u8_t*)ECC_P256DomainParams_b,
                        (u8_t*)ECC_P256DomainParams_n,
                        (u8_t*)ECC_P256DomainParams_Gx,
                        (u8_t*)ECC_P256DomainParams_Gy,
                        (u8_t*)privateKey, /* private key , random integer */
                        0, /* set this to 0 to generate random */
                        publicKey,
                        (u8_t *)(publicKey + ECC_SIZE_P256),
                        NULL, NULL);
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ecp_ecdsa_key_generate_publickeyonly
 * Purpose : Generate an Elliptic curve DSA Key Pair. This is redundant
 * 			from previous API,
 *           could not change the above to accomodate this change Since
 *           it is used by vendors.
 * Input   : 
 *      	pHandle : Pointer to the Crypto handle
 *      	privateKey : Pointer to the privateKey  It must be 32 Bytes in
 *      	size and must have valid value.
 *		publicKey : Pointer to the publicKey  It must be 64 Bytes in size.
 * Return  : On success(= 0) returns the Public Key
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ecp_ecdsa_key_generate_publickeyonly(
		crypto_lib_handle *pHandle,u8_t* privateKey, u8_t* publicKey)
{
    /* The prime modulus */
    u32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve; In FIPS 186-3,
     * the selection a = p-3 for the coefficient of x was made for
     * reasons of efficiency
    taken from section D.1.2.3 Page 89 for Curve P-256 */
    u32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve */
    u32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
    {
        0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };
    /* Order n */
    u32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
    {
        0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
        0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
    };
    /* The base point x coordinate Gx */
    u32_t ECC_P256DomainParams_Gx [ECC_SIZE_P256>>2] =
    {
        0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
        0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2
    };
    /* The base point y coordinate Gy */
    u32_t ECC_P256DomainParams_Gy [ECC_SIZE_P256>>2] =
    {
        0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
        0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2
    };

    return crypto_pka_ecp_ecdsa_key_generate(pHandle,
                                0, /* Curve Type = 0 (Prime field) */
                                (u8_t*)ECC_P256DomainParams_Prime,
                                (ECC_SIZE_P256)*8, /* P Bit len */
                                (u8_t*)ECC_P256DomainParams_a,
                                (u8_t*)ECC_P256DomainParams_b,
                                (u8_t*)ECC_P256DomainParams_n,
                                (u8_t*)ECC_P256DomainParams_Gx,
                                (u8_t*)ECC_P256DomainParams_Gy,
                                privateKey, /* private key , random integer */
                                1, /* set this to 0 to generate random */
                                publicKey,
                                (u8_t *)(publicKey + ECC_SIZE_P256),
                                NULL, NULL);
}


/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ecp_ecdsa_sign
 * Purpose : Perform an Elliptic curve DSA signature with SHA-256 on
 * 				NIST P-256 curve Signature
 * Input   : pointers to the data parameters
 *			pHandle : Pointer to the Crypto handle
 *			privKey  : Pointer to the Private Key
 * 			messageLength: Length of the message to be signed
 * 			message : Pointer to the message
 *			Signature : Pointer to the signature for the message.
 *			It must be 64 Bytes in size.
 * Return  : On success(= 0) returns the Signature
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_sign(crypto_lib_handle *pHandle,
		u8_t* privKey, u32_t messageLength, u8_t* message, u8_t* signature)
{

    u8_t inputSha[SHA256_HASH_SIZE];
    BCM_SCAPI_STATUS status;

    /* The prime modulus */
    u32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve; In FIPS 186-3, the selection
     * a = p-3 for the coefficient of x was made for reasons of efficiency
    taken from section D.1.2.3 Page 89 for Curve P-256 */
    u32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve */
    u32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
    {
        0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };
    /* Order n */
    u32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
    {
        0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
        0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
    };
    /* The base point x coordinate Gx */
    u32_t ECC_P256DomainParams_Gx [ECC_SIZE_P256>>2] =
    {
        0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
        0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2
    };
    /* The base point y coordinate Gy */
    u32_t ECC_P256DomainParams_Gy [ECC_SIZE_P256>>2] =
    {
        0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
        0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2
    };
    /* Random integer */
    u32_t k[8]   = {0x34ee45d0, 0x1965c7b1, 0xfa47055b, 0x5b12ae37,
    				0xecaed496, 0x4cef3f71, 0x85643433, 0x580ec00d};

    status = crypto_symmetric_hmac_sha256(pHandle, message, messageLength,
    		NULL, 0, inputSha, TRUE);
    if (status != BCM_SCAPI_STATUS_OK){
        return status;
    }

    bcm_int2bignum((u32_t*)inputSha, SHA256_HASH_SIZE);
    /* Curve Type = 0 (Prime field) */
    return crypto_pka_ecp_ecdsa_sign (pHandle, inputSha, SHA256_HASH_SIZE, 0 ,
                    (u8_t *)ECC_P256DomainParams_Prime,
                    (ECC_SIZE_P256)*8,
                    (u8_t *)ECC_P256DomainParams_a,
                    (u8_t *)ECC_P256DomainParams_b,
                    (u8_t *)ECC_P256DomainParams_n,
                    (u8_t *)ECC_P256DomainParams_Gx,
                    (u8_t *)ECC_P256DomainParams_Gy,
                    (u8_t *)k, /* Set this to NULL to generate random number */
                    0, /* set this to 0 to generate random */
                    privKey,
                    signature,
                    (u8_t *)(signature + ECC_SIZE_P256),
                    NULL, NULL);
}


/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ecp_ecdsa_verify
 * Purpose : Verify an Elliptic curve DSA signature with SHA-256 on
 * 			NIST P-256 curve
 * Input   : pointers to the data parameters
 *			pHandle : Pointer to the Crypto handle
 *			pubKey  : Pointer to the public Key
 * 			messageLength: Length of the message to be verified
 * 			message : Pointer to the message
 *			Signature : Pointer to the signature for the message
 * Return  : If Message Hash == r then verify success( = 0)
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ecdsa_p256_verify(crypto_lib_handle *pHandle,
			u8_t* pubKey, u32_t messageLength, u8_t* message, u8_t* Signature)
{
    u8_t outputSha[SHA256_HASH_SIZE];

    BCM_SCAPI_STATUS status;

    u32_t QZ[ECC_SIZE_P256>>2] =
    {
        0x00000001, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    /* The prime modulus */
    u32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve; In FIPS 186-3,
     * the selection a = p-3 for the coefficient of x was made for
     * reasons of efficiency
    taken from section D.1.2.3 Page 89 for Curve P-256 */
    u32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve */
    u32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
    {
        0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };
    /* Order n */
    u32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
    {
        0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
        0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
    };
    /* The base point x coordinate Gx */
    u32_t ECC_P256DomainParams_Gx [ECC_SIZE_P256>>2] =
    {
        0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
        0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2
    };
    /* The base point y coordinate Gy */
    u32_t ECC_P256DomainParams_Gy [ECC_SIZE_P256>>2] =
    {
        0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
        0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2
    };

    status = crypto_symmetric_hmac_sha256(pHandle, message, messageLength,
    									  NULL, 0, outputSha, TRUE);
    if (status != BCM_SCAPI_STATUS_OK){
        return status;
    }

    bcm_int2bignum((u32_t*)outputSha, SHA256_HASH_SIZE);
    /* Curve Type = 0 (Prime field) */
    return crypto_pka_ecp_ecdsa_verify (pHandle, outputSha, SHA256_HASH_SIZE,0,
                                (u8_t *)ECC_P256DomainParams_Prime,
                                (ECC_SIZE_P256)*8,
                                (u8_t *)ECC_P256DomainParams_a,
                                (u8_t *)ECC_P256DomainParams_b,
                                (u8_t *)ECC_P256DomainParams_n,
                                (u8_t *)ECC_P256DomainParams_Gx,
                                (u8_t *)ECC_P256DomainParams_Gy,
                                pubKey, // pubKey_x
                                (u8_t *)(pubKey + ECC_SIZE_P256), // pubKey_y
                                (u8_t *)QZ,
                                Signature,
                                (u8_t *)(Signature + ECC_SIZE_P256),
                                NULL, NULL);
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ecp_diffie_hellman_shared_secret
 * Purpose : Generate an Elliptic Curve Diffie-Hellman shared secret with
 * 			P-256 curve
 * Input   : pointers to the data parameters
 *			pHandle : Pointer to the Crypto handle
 *			privKey  : Pointer to the Private Key
 *			pubKey  : Pointer to the Public Key
 *			sharedSecret : Pointer to the shared Secret. This must be 96 bytes
 *			in length.
 * Return  : if success (=0) returns the shared Secret
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ecp_diffie_hellman_shared_secret(
		crypto_lib_handle *pHandle, u8_t* privKey,u8_t* pubKey,
		u8_t* sharedSecret)
{
    /* The prime modulus */
    u32_t ECC_P256DomainParams_Prime[ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve; In FIPS 186-3,
     * the selection a = p-3 for the coefficient of x was made for
     * reasons of efficiency
    taken from section D.1.2.3 Page 89 for Curve P-256 */
    u32_t ECC_P256DomainParams_a [ECC_SIZE_P256>>2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    /* Co-efficient defining the elliptic curve */
    u32_t ECC_P256DomainParams_b [ECC_SIZE_P256>>2] =
    {
        0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };
    /* Order n */
    u32_t ECC_P256DomainParams_n [ECC_SIZE_P256>>2] =
    {
        0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
        0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF
    };
    /* Curve Type = 0 (Prime field) */
    return crypto_pka_ecp_diffie_hellman_shared_secret (pHandle, 0 ,
                                (u8_t *)ECC_P256DomainParams_Prime,
                                (ECC_SIZE_P256)*8,
                                (u8_t *)ECC_P256DomainParams_a,
                                (u8_t *)ECC_P256DomainParams_b,
                                (u8_t *)ECC_P256DomainParams_n,
                                (u8_t *)privKey,
                                (u8_t *)pubKey, // pubKey_x
                                (u8_t *)(pubKey + ECC_SIZE_P256), // pubKey_y
                                (u8_t *)sharedSecret,
                                (u8_t *)(sharedSecret + ECC_SIZE_P256),
                                NULL, NULL);
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_cust_ec_point_verify
 * Purpose : Verify an Elliptic curve public key (an EC point on P-256)
 * Input   : pointers to the data parameters
 * pHandle : Pointer to the Crypto handle
 * pubKey  : Pointer to the public Key (64 bytes raw uncompressed format)
 * Return  : If public key is valid, then success( = 0)
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_cust_ec_point_verify(crypto_lib_handle *pHandle,
												u8_t *pubKey)
{

    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    int i;

    if(!pHandle)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    u32_t pbytes[ECC_SIZE_P256 >> 2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    u32_t abytes[ECC_SIZE_P256 >> 2] =
    {
        0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
        0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
    };
    u32_t bbytes[ECC_SIZE_P256 >> 2] =
    {
            0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
        0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
    };

    u8_t xbytes[ECC_SIZE_P256];
    u8_t ybytes[ECC_SIZE_P256];

    /* our point to test against the curve */
    q_lint_t    x;
    q_lint_t    y;
    q_lint_t    a;
    q_lint_t    b;
    q_lint_t    p;
    q_status_t q_status=Q_SUCCESS;

    /* initialize hardware first */
    q_pka_hw_rst();

    LOCAL_QLIP_CONTEXT_DEFINITION;

    /* qlip likes lengths in bits */
    u32_t    bitlen = ECC_SIZE_P256 * 8;

    /* extract curve point x and y from the public key */
    memcpy(xbytes, pubKey, sizeof(xbytes));
    memcpy(ybytes, &pubKey[ECC_SIZE_P256], sizeof(ybytes));

    /* Set the variables, since they are already formatted as bignum,
     * set_param_bn will make them positive #s */
    set_param_bn(&x, xbytes, bitlen, 0);
    set_param_bn(&y, ybytes, bitlen, 0);
    set_param_bn(&a, (u8_t*)abytes, bitlen, 0);
    set_param_bn(&b, (u8_t*)bbytes, bitlen, 0);
    set_param_bn(&p, (u8_t*)pbytes, bitlen, 0);

    /* Check 1. Check that x and y are not negative (see above) */
    /* Check 2. Now compare x < p */
    if (q_cmp(&x, &p) != -1)
    {
        return (BCM_SCAPI_STATUS_PARAMETER_INVALID);
    }
    /* Check 3. Now compare y < p */
    if (q_cmp(&y, &p) != -1)
    {
        return (BCM_SCAPI_STATUS_PARAMETER_INVALID);
    }

    /* Check 4: The curve is y^2 = x^3 + ax + b */
    /* So, we calculate
    t1 = Qx*Qx mod ec.p
    t2 = t1 + ec.a mod ec.p
    t1 = t2*Qx mod ec.p
    t2 = t1 + ec.b mod ec.p
    t1 = Qy*Qy mod ec.p
    t3 = t2-t1 mod ec.p should be 0
    */
    /*declared t2 and t6 and initialized along with t1 to fix coverity error*/
    q_lint_t t1;
    q_lint_t t2;
    q_lint_t t6;
    memset(&t1, 0, sizeof(t1));
    memset(&t2, 0, sizeof(t2));
    memset(&t6, 0, sizeof(t6));
    q_status = q_init(QLIP_CONTEXT, &t1, 2 * x.size);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }
    q_status = q_modsqr(QLIP_CONTEXT, &t1, &x, &p);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }

    q_status = q_init(QLIP_CONTEXT, &t2, 2 * x.size);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }
    q_status = q_modadd(QLIP_CONTEXT, &t2, &t1, &a, &p);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }

    q_status = q_modmul(QLIP_CONTEXT, &t1, &t2, &x, &p);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }

    q_status = q_modadd(QLIP_CONTEXT, &t2, &t1, &b, &p);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }

    q_status = q_modsqr(QLIP_CONTEXT, &t1, &y, &p);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }

    q_status = q_init(QLIP_CONTEXT, &t6, 3 * x.size);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }
    /* q_sub will return a zero length result if t4 and t5 are
    equal.  q_mod will not accept that so we test it separately
    */
    q_status = q_sub(&t6, &t2, &t1);
    if (q_status != Q_SUCCESS)
    {
        goto cleanup_exit;
    }
    if (t6.size == 0)
    {
        status = BCM_SCAPI_STATUS_OK;
        goto cleanup_exit;
    }

    q_copy(&t1, &t6);
    q_mod(QLIP_CONTEXT, &t6, &t1, &p);

    if (t6.size == 0)
    {
        status = BCM_SCAPI_STATUS_OK;
        goto cleanup_exit;
    }

    status = BCM_SCAPI_STATUS_OK;
    for (i = 0; i < t6.size; i++)
    {
        if (t6.limb[i] != 0)
        {
            status = BCM_SCAPI_STATUS_SELFTEST_MATH_FAIL;
            break;
        }
    }

    cleanup_exit:
    if(q_status != Q_SUCCESS){
        status = BCM_SCAPI_STATUS_CRYPTO_ERROR;
    }

    if (t6.alloc) q_free(QLIP_CONTEXT, &t6);
    if (t2.alloc) q_free(QLIP_CONTEXT, &t2);
    if (t1.alloc) q_free(QLIP_CONTEXT, &t1);

    return status;
}


