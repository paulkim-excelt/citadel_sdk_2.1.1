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

/* @file crypto_pka.c
 *
 * pka algorithms implementation
 * This file contains the pka algorithms like RSA,DSA,ECC and diffie hellman.
 * The pka uses an internal library which interacts with the pka hardware
 * accelerator to calculate complex mathematics involved in the pka algos
 *
 */

/*
 * TODO or FIXME
 *
 * 1. ENTER_CRITICAL(); enable when available
 * 2. if (arg != NULL){ FIXME has to be solved, this is required for warning
 *    removal
 * 3. printf has to be system LOGS for debug messages
 * */
/* ***************************************************************************
 * Includes
 * ****************************************************************************/
#include <string.h>
#include <zephyr/types.h>
#include <logging/sys_log.h>
#include <crypto/crypto_symmetric.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <crypto/crypto_rng.h>
#include <crypto/crypto_smau.h>
#include <pka/q_lip.h>
#include <pka/q_lip_aux.h>
#include <pka/q_pka_hw.h>
#include <pka/q_elliptic.h>
#include <pka/q_rsa.h>
#include <pka/q_rsa_4k.h>
#include <pka/q_dsa.h>
#include <pka/q_dh.h>
#include <pka/q_lip_utils.h>
#include <pka/q_context.h>
#include <broadcom/dma.h>

#define PKA_RSA_QLIP_MAX   2048

/* **************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
/**
 * Definition
 */
#define ROUNDUP_TO_32_BIT(n) ((n+31)&(~31))

/* **************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* **************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/
static const u8_t sha1DerPkcs1Array[SHA1_DER_PREAMBLE_LENGTH] = {
		0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
        0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14};
static const u8_t sha256DerPkcs1Array[SHA256_DER_PREAMBLE_LENGTH] = {
		0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
        0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
/* **************************************************************************
 * Private Functions Prototypes
 * ***************************************************************************/

/**
 *
 */
static BCM_SCAPI_STATUS find_mod_size(u32_t inLen, u32_t * outLen);
/**
 *
 */
static u32_t bcm_rsa_find_num_bits(u8_t * sum, u32_t sumLen);
/**
 *
 */
static BCM_SCAPI_STATUS bcm_primerng(
               crypto_lib_handle *pHandle,
               u32_t              length,
               u32_t             *result_bits,
               u8_t               *result,

               /* Passing globals */
               u32_t             *OneBits,
               u32_t             *One,
               u32_t             *HugeModulusBits,
               u32_t             *HugeModulus,
               q_lip_ctx_t       *q_lip_ctx
           );
/**
 *
 */
static BCM_SCAPI_STATUS bcm_rsa_get_random(u32_t *bits, u8_t *array);
#define USE_RAW_RANDOM_DATA_FOR_PRIME_RNG
#ifdef USE_RAW_RANDOM_DATA_FOR_PRIME_RNG
static BCM_SCAPI_STATUS bcm_rsa_get_random_raw(u32_t *bits, u8_t *array);
#else
/**
 *
 */
static BCM_SCAPI_STATUS bcm_rsa_get_random_blocks(u32_t m, u32_t lastBlockSize,
                                    u8_t * rng);
#endif
/**
 *
 */
static BCM_SCAPI_STATUS bcm_test_prime_rm(
         crypto_lib_handle *pHandle,
         int                  num_witnesses,
         u32_t             *p_bits,
         u32_t             *p,

         /* Passing globals */
         u32_t             *OneBits,
         u32_t             *One,
         u32_t             *HugeModulusBits,
         u32_t             *HugeModulus,
         q_lip_ctx_t       *q_lip_ctx
     );

/**
 *
 */
static BCM_SCAPI_STATUS bcm_test_prime_sp(crypto_lib_handle *pHandle,
                                   u32_t p_bits,
                                   u32_t *p);

/**
 *
 */
static BCM_SCAPI_STATUS bcm_math_ppsel (
            crypto_lib_handle *pHandle,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u32_t iter,
                    scapi_callback callback, void *arg);

/**
 *
 */
static BCM_SCAPI_STATUS
bcm_dsa_genk(crypto_lib_handle *pHandle,
             u8_t  *q,        u32_t q_bitlen,
             u8_t  *k,
             u32_t *k_bytelen,
             u8_t  *k_inv,
             u32_t *k_inv_bytelen);

static BCM_SCAPI_STATUS emsa_pkcs1_v15_encode_hash(crypto_lib_handle *pHandle,
		u32_t hLen, u8_t *h, u32_t emLen,
		u8_t *em, BCM_HASH_ALG hashAlg);

static BCM_SCAPI_STATUS emsa_pkcs1_v15_encode(crypto_lib_handle *pHandle,
		u32_t mLen, u8_t *M, u32_t emLen,
		u8_t *em, BCM_HASH_ALG hashAlg);

/* ***************************************************************************
 * Public Functions
 * ***************************************************************************/
/*---------------------------------------------------------------
 * Name    : crypto_pka_math_accelerate
 * Purpose : Perform a modular math operation
 * Input   : pointers to the data parameters
 *           For ModExp, paramB is the exponent
 * Output  : result: will have length of find_mod_size(modulus length)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *           There are some HW limitation of the MODREM
 *           (something like modulus MSB must be 1),
 *           Caller can call rsa crt function instead.
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS  crypto_pka_math_accelerate (
                    crypto_lib_handle *pHandle,
                    u32_t         cmd,
                    u8_t *modN,   u32_t  modN_bitlen,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u8_t *paramB, u32_t  paramB_bitlen,
                    u8_t *result, u32_t  *result_bytelen,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t modN_len_adj, paramB_len_adj;
    //    u32_t exp_bitlen = 0;
    q_lint_t qlip_paramA,qlip_result;         /* q_lint pointer to parameters*/
    q_lint_t qlip_modN,qlip_paramB;
    LOCAL_QLIP_CONTEXT_DEFINITION;

#ifdef CRYPTO_DEBUG
    printf("In crypto_pka_math_accelerate\n");
#endif


    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

    status = find_mod_size(BITLEN2BYTELEN(modN_bitlen), &modN_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    /* Now set up all the parameters to send to the qlip */

    /* Find parameter size */
    if (cmd == BCM_SCAPI_MATH_MODEXP){
        /* exponent round up to 32bits */
        paramB_len_adj = (ROUNDUP_TO_32_BIT(paramB_bitlen) >> 3);
    }
    else
    {
      paramB_len_adj = modN_len_adj; /* otherwise round up to exponent size */
    }


    /* Set n */
    if (cmd != BCM_SCAPI_MATH_MUL) {
        set_param_bn(&qlip_modN,modN, modN_bitlen, modN_len_adj);
    }

    /* Set A */
    set_param_bn(&qlip_paramA,paramA, paramA_bitlen, modN_len_adj);

    /* Set B. If it's REM operation, don't send */
    if ((cmd != BCM_SCAPI_MATH_MODREM) && ((cmd != BCM_SCAPI_MATH_MODINV)))
      {
    set_param_bn(&qlip_paramB,paramB, paramB_bitlen, paramB_len_adj);
      }

    /* Set Result */
    set_param_bn(&qlip_result,result, modN_bitlen, modN_len_adj);

#ifdef CRYPTO_DEBUG
    if (cmd != BCM_SCAPI_MATH_MUL) {
        printf("modN = 0x%x,size = %d , alloc = %d, neg = %d\n",
              *(qlip_modN.limb), qlip_modN.size,
              qlip_modN.alloc, qlip_modN.neg);
    }
    printf("modA = 0x%x,size = %d , alloc = %d, neg = %d\n",
            *(qlip_paramA.limb), qlip_paramA.size,
            qlip_paramA.alloc, qlip_paramA.neg);
    printf("modB = 0x%x,size = %d , alloc = %d, neg = %d\n",
            *(qlip_paramB.limb), qlip_paramB.size,
            qlip_paramB.alloc, qlip_paramB.neg);
#endif

    pHandle -> busy        = TRUE;
    switch (cmd) {
    case BCM_SCAPI_MATH_MODADD:
      status =  q_modadd (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODSUB:
      status =  q_modsub (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODMUL:
      status =  q_modmul (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODREM:
      status = q_mod(QLIP_CONTEXT, &qlip_result, &qlip_paramA, &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODEXP:
      //exp_bitlen = paramB_bitlen;
      if((modN_bitlen > PKA_RSA_QLIP_MAX) ||
         (paramB_bitlen > PKA_RSA_QLIP_MAX) ||
         (paramA_bitlen > PKA_RSA_QLIP_MAX))
      {
        status =  q_modexp_sw (QLIP_CONTEXT, &qlip_result,
                               &qlip_paramA, &qlip_paramB, &qlip_modN);
      }
      else
      {
#ifdef CRYPTO_DEBUG
    	  printf("Calling q_modexp\n");
#endif

         status =  q_modexp(QLIP_CONTEXT, &qlip_result,
                            &qlip_paramA, &qlip_paramB, &qlip_modN);
    }
      break;
    case BCM_SCAPI_MATH_MODINV:
      if (modN_bitlen > PKA_RSA_QLIP_MAX)
        status =  q_modinv_sw (QLIP_CONTEXT, &qlip_result,
                            &qlip_paramA,  &qlip_modN);
      else
        status =  q_modinv (QLIP_CONTEXT, &qlip_result,
                            &qlip_paramA,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MUL:
      status = q_mul(QLIP_CONTEXT, &qlip_result, &qlip_paramA, &qlip_paramB);
      break;
    default:
        return BCM_SCAPI_STATUS_CRYPTO_MATH_UNSUPPORTED;
    }

#ifdef CRYPTO_DEBUG
    printf("result = 0x%x,size = %d , alloc = %d, neg = %d\n",
            *(qlip_result.limb), qlip_result.size,
            qlip_result.alloc, qlip_result.neg);
#endif
      /* Command is completed. */
      pHandle -> busy        = FALSE;
      *result_bytelen = modN_len_adj;

    if (callback == NULL)
    {
    }
    else
    {
      pHandle -> result      = result;
      pHandle -> result_len  = modN_len_adj;
      pHandle -> presult_len = result_bytelen;
      pHandle -> callback    = callback;
      pHandle -> arg         = arg;
      (callback) (status, pHandle, pHandle->arg);
        /* assign context for use in check_completion */
    }

#ifdef CRYPTO_DEBUG
    printf("Exit crypto_pka_math_accelerate\n");
#endif
  return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_mont_math_accelerate
 * Purpose : Perform a Montgomery modular math operation
 * Input   : pointers to the data parameters
 *           For ModExp, paramB is the exponent
 * Output  : result: will have length of find_mod_size(modulus length)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *           There are some HW limitation of the MODREM
 *           (something like modulus MSB must be 1),
 *           Caller can call rsa crt function instead.
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_mont_math_accelerate (
                    crypto_lib_handle *pHandle,
                    u32_t         cmd,
                    u8_t *modN,   u32_t  modN_bitlen,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u8_t *paramB, u32_t  paramB_bitlen,
                    u8_t *result, u32_t  *result_bytelen,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status =BCM_SCAPI_STATUS_OK;

    u32_t modN_len_adj, paramB_len_adj;
    //    u32_t exp_bitlen = 0;
    q_lint_t qlip_paramA,qlip_result;         /* q_lint pointer to parameters */
    q_lint_t qlip_modN,qlip_paramB;
    u8_t *MontPtr;
    /* WARNING REMOVAL */
    //u32_t KeySize;
    q_mont_t mont;
    qlip_modN.limb = 0;
    qlip_modN.size = 0;

    LOCAL_QLIP_CONTEXT_DEFINITION;
    status = q_init( QLIP_CONTEXT, &mont.n, 0x80 );
    if( status ==  BCM_SCAPI_STATUS_OK )
    {
        status = q_init( QLIP_CONTEXT, &mont.np, 0x81 );
    }
    if( status ==  BCM_SCAPI_STATUS_OK )
    {
        status = q_init( QLIP_CONTEXT, &mont.rr, 0x81 );
    }

    MontPtr = modN;

    // n
    mont.n.size = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    memcpy( mont.n.limb, MontPtr, mont.n.size*4);
    MontPtr += mont.n.size*4;
    mont.n.alloc = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    mont.n.neg = *(u32_t *)MontPtr;
    MontPtr += sizeof(int);
    mont.br = (mont.n.size << 5) ;

    // np
    mont.np.size = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    memcpy( mont.np.limb, MontPtr, mont.np.size*4);
    MontPtr += mont.np.size*4;
    mont.np.alloc = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    mont.np.neg = *(u32_t *)MontPtr;
    MontPtr += sizeof(int);

    // rr
    mont.rr.size = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    memcpy( mont.rr.limb, MontPtr, mont.rr.size*4);
    MontPtr += mont.rr.size*4;
    mont.rr.alloc = (*(u32_t *)MontPtr);
    MontPtr += sizeof(int);
    mont.rr.neg = *(u32_t *)MontPtr;

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

    status = find_mod_size(BITLEN2BYTELEN(modN_bitlen), &modN_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    /* Now set up all the parameters to send to the qlip */

    /* Find parameter size */
    if (cmd == BCM_SCAPI_MATH_MODEXP){
        /* exponent round up to 32bits */
      paramB_len_adj = (ROUNDUP_TO_32_BIT(paramB_bitlen) >> 3);
    }
    else
    {
      paramB_len_adj = modN_len_adj; /* otherwise round up to exponent size */
    }

    /* Set A */
    set_param_bn(&qlip_paramA,paramA, paramA_bitlen, modN_len_adj);

    /* Set B. If it's REM operation, don't send */
    if ((cmd != BCM_SCAPI_MATH_MODREM) && ((cmd != BCM_SCAPI_MATH_MODINV)))
    {
        set_param_bn(&qlip_paramB,paramB, paramB_bitlen, paramB_len_adj);
    }

    /* Set Result */
    set_param_bn(&qlip_result,result, modN_bitlen, modN_len_adj);

    pHandle -> busy        = TRUE;
    switch (cmd) {
    case BCM_SCAPI_MATH_MODADD:
      status =  q_modadd (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODSUB:
      status =  q_modsub (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODMUL:
      status =  q_modmul (QLIP_CONTEXT, &qlip_result,
                          &qlip_paramA,  &qlip_paramB,  &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODREM:
      status = q_mod(QLIP_CONTEXT, &qlip_result, &qlip_paramA, &qlip_modN);
      break;
    case BCM_SCAPI_MATH_MODEXP:
        //exp_bitlen = paramB_bitlen;
        if(modN_bitlen > 2048 || paramB_bitlen > 2048 || paramA_bitlen > 2048)
        {
            status =  q_modexp_mont_sw (QLIP_CONTEXT, &qlip_result,
                                        &qlip_paramA, &qlip_paramB, &mont);
        }
        else
        {
            status =  q_modexp(QLIP_CONTEXT, &qlip_result,
                               &qlip_paramA, &qlip_paramB, &qlip_modN);
        }
      break;
    case BCM_SCAPI_MATH_MODINV:
      status =  q_modinv (QLIP_CONTEXT, &qlip_result, &qlip_paramA, &qlip_modN);
      break;
    default:
        return BCM_SCAPI_STATUS_CRYPTO_MATH_UNSUPPORTED;
    }

    /* Command is completed. */
    pHandle -> busy        = FALSE;
    *result_bytelen = modN_len_adj;

    if (callback == NULL)
    {
    }
    else
    {
        pHandle -> result      = result;
        pHandle -> result_len  = modN_len_adj;
        pHandle -> presult_len = result_bytelen;
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        (callback) (status, pHandle, pHandle->arg);
        /* assign context for use in check_completion */
    }

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_rsa_mod_exp
 * Purpose : Perform an RSA mod exponentiation
 * Input   : pointers to the data parameters and crypto context
             x: the plain text message
             m: the modulus
             e: the exponent
 * Output  : y: = x**e mod m, y length is find_mod_size(m_bitlen)
 * Return  : Appropriate status
 * Remark  :
             If the callback parameter is NULL, the operation is synchronous
             The modulus will be round up according to a table
             The exponent will be round up to the nearest 32 bits
             The exponent and modulus parameter sizes may not be the same
             x_len must <= m_len (right?)
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_rsa_mod_exp (
                     crypto_lib_handle *pHandle,
                     u8_t *x, u32_t x_bitlen,
                     u8_t *m, u32_t m_bitlen,
                     u8_t *e, u32_t e_bitlen,
                     u8_t *y, u32_t *y_bytelen,
                     scapi_callback callback, void *arg
                     )
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t m_len_adj, e_len_adj;
    q_lint_t c;
    q_rsa_key_t rsa;
    q_lint_t qlip_m;
    LOCAL_QLIP_CONTEXT_DEFINITION;
	u8_t *x_t = NULL, *m_t = NULL, *e_t = NULL;
	u32_t x_len, m_len, e_len;
	u32_t x_len_round, m_len_round, e_len_round;
	q_lint_t q_t;

    if (!(pHandle && x && m && e && y)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

	if ((x_bitlen % 8) || (m_bitlen % 8) || (e_bitlen % 8))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	x_len = BITLEN2BYTELEN(x_bitlen);
	if (x_len % 4) {
		x_len_round = (x_len + 4) / 4; /* In words */
		x_t = (u8_t *)q_malloc(QLIP_CONTEXT, x_len_round);
		if(!x_t)
			return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;

		memcpy(x_t, x, x_len);
		/* Fill zero in last bytes */
		memset(x_t + x_len, 0, (x_len_round * 4 - x_len));
		x = x_t;
	}

	m_len = BITLEN2BYTELEN(m_bitlen);
	if (m_len % 4) {
		m_len_round = (m_len + 4) / 4; /* In words */
		m_t = (u8_t *)q_malloc(QLIP_CONTEXT, m_len_round);
		if(!m_t) {
			status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
			goto exit;
		}
		memcpy(m_t, m, m_len);
		/* Fill zero in last bytes */
		memset(m_t + m_len, 0, (m_len_round * 4 - m_len));
		m = m_t;
	}

	e_len = BITLEN2BYTELEN(e_bitlen);
	if (e_len % 4) {
		e_len_round = (e_len + 4) / 4; /* In words */
		e_t = (u8_t *)q_malloc(QLIP_CONTEXT, e_len_round);
		if(!e_t) {
			status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
			goto exit;
		}
		memcpy(e_t, e, e_len);
		/* Fill zero in last bytes */
		memset(e_t + e_len, 0, (e_len_round * 4 - e_len));
		e = e_t;
	}

    /* adjust modulus length */
    status = find_mod_size(BITLEN2BYTELEN(m_bitlen), &m_len_adj);
	if (status != BCM_SCAPI_STATUS_OK) {
		status = BCM_SCAPI_STATUS_PARAMETER_INVALID;
		goto exit;
	}

    /*
     * Mapping: y==c,rsa.n==m,rsa.e==e,qlip_m=x
     */

    /* adjust exponent length */
    e_len_adj = (ROUNDUP_TO_32_BIT(e_bitlen) >> 3);
    /* Set m */
    set_param_bn(&rsa.n,m, m_bitlen, m_len_adj);
    /* Set c, which need to be padded to m_len_adj */
    set_param_bn(&c,y, m_bitlen, m_len_adj); /* SOR ??? */
    /* Set e */
    set_param_bn(&rsa.e,e, e_bitlen, e_len_adj);
    /* Set x, which need to be padded to m_len_adj */
    set_param_bn(&qlip_m,x, x_bitlen, m_len_adj);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */
    /* Call the qlip function. */
    status = q_rsa_enc (QLIP_CONTEXT,
            &c,
            &rsa,
            &qlip_m);

    pHandle -> busy       = FALSE;
    /* Command is completed */

    *y_bytelen = m_len_adj;  /* Do we need to do this? */

    if (callback == NULL)
    {

    }
    else
    {
        /* assign context for use in check_completion */
      pHandle -> result     = y;
      pHandle -> result_len = m_len_adj;
      pHandle -> presult_len = y_bytelen;
      pHandle -> callback   = callback;
      pHandle -> arg        = arg;
      /* Since in sync mode call back now */
      (callback) (status, pHandle, pHandle->arg);
    }

exit:
	if (x_t) {
		q_t.limb = (q_limb_t *)x_t;
		q_t.alloc = x_len_round;
		memset(x_t, 0, x_len_round * 4);
		q_free(QLIP_CONTEXT, &q_t);
	}

	if (m_t) {
		q_t.limb = (q_limb_t *)m_t;
		q_t.alloc = m_len_round;
		memset(m_t, 0, m_len_round * 4);
		q_free(QLIP_CONTEXT, &q_t);
	}

	if (e_t) {
		q_t.limb = (q_limb_t *)e_t;
		q_t.alloc = e_len_round;
		memset(e_t, 0, e_len_round * 4);
		q_free(QLIP_CONTEXT, &q_t);
	}

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_rsa_mod_exp_crt
 * Purpose : Perfrom an RSA Chinese Remainder Theorem operation
 * Input   : pointers to the data parameters and crypto context
             x: the encrypted message
             p
             q
             edp, qinv: actual bit length must <= p bit length, and
             padded to p bit length
             edq: actual bit length must <= q bit length, and padded
             to q bit length
 * Output  : y, length is find_mod_size(q_bitlen*2)
 * Return  : Appropriate status
 * Remark  :
             Hardware uses pinv, so we internally swap p and q, dp and dq
             and the upper layer app is always expected to pass use qinv.
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_rsa_mod_exp_crt (
                    crypto_lib_handle *pHandle,
                    u8_t *x,    u32_t x_bitlen,
                    u8_t *edq,
                    u8_t *q,    u32_t q_bitlen,
                    u8_t *edp,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *qinv,
                    u8_t *y,    u32_t *y_bytelen,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t q_len_adj;
    q_lint_t m;
    q_rsa_crt_key_t rsa;
    q_lint_t c;
    LOCAL_QLIP_CONTEXT_DEFINITION;

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

    if (!(pHandle && x && edq && q && edp && p && qinv && y)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    /* Find parameter size according to the modulus length
     * (twice the size of p or q) */
    status = find_mod_size(BITLEN2BYTELEN(q_bitlen)*2, &q_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }
    q_len_adj >>= 1;

    /*
    * mapping: rsa.p==q,rsa.q==p,rsa.dp==dq,rsa.dq==dp,rsa.qinv==qinv,m==y,c==x
    * Note: swapping q and p.
    */

    /* Set p, pad to same adj length as q. */
    set_param_bn(&rsa.p, p, p_bitlen, q_len_adj);
    /* Set q */
    set_param_bn(&rsa.q, q, q_bitlen, q_len_adj);
    /* Set dp, caller must have pad it to p_bitlen */
    set_param_bn(&rsa.dp, edp, p_bitlen, q_len_adj);
    /* Set dq, caller must have pad it to q_bitlen */
    set_param_bn(&rsa.dq, edq, q_bitlen, q_len_adj);
    /* Set qinv, caller must have pad it to p_bitlen */
    set_param_bn(&rsa.qinv,qinv, p_bitlen, q_len_adj);

    /* Set x, modulus size, not half */
    set_param_bn(&m,y, q_bitlen, q_len_adj<<1); /* SOR? */
    set_param_bn(&c,x, x_bitlen, q_len_adj<<1);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */

    /* Call the qlip function. */
    status = q_rsa_crt_4k (QLIP_CONTEXT,
                            &m,
                            &rsa,
                            &c);

    pHandle -> busy        = FALSE; /* Prevent reentrancy */
    /* Command is completed. */
    *y_bytelen = q_len_adj<<1; /* Do we need to do this */

    if (callback == NULL)
    {

    }
    else
    {
        /* assign context for use in check_completion */
        pHandle -> result      = y;
        pHandle -> result_len  = q_len_adj<<1;
        pHandle -> presult_len = y_bytelen;
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        /* Since in sync mode call back now */
        (callback) (status, pHandle, pHandle->arg);
    }

    return status;
}

/*-----------------------------------------------------------------------------
 * Name:  crypto_pka_rsa_key_generate
 * Purpose : generate RSA public and private keys, PKCS#1 v2.1
 * Inputs: pointers to the data parameters
 *         modulus_bits  - input, requested modulus size
 *         e_bits    - Length pointer to public key exponent (e_bits)
 *         e         - Byte pointer (e), manual input or generated output
 * In/Out:
 *         p_bits    - Length pointer to private key prime (p_bits)
 *         p         - Byte pointer (p), manual input or generated output
 *         q_bits    - Length pointer to private key prime (q_bit)
 *         q         - Byte pointer (q), manual input or generated output
 *
 *      Action: If any *p_bits, *q_bits == NULL for In/Outs then
 *          generated output has been requested. If these are
 *          not NULL then present values of p or q are used
 *          upon function entry. The user is responsible for
 *          establishing valid primes.
 *
 *
 *  Outputs:
 *      n_bits    - Length pointer to public key modulus (n_bits)
 *      n         - Byte pointer (n), generated output
 *      d_bits    - Length pointer to private key exponent (d_bits)
 *      d         - Byte pointer (d), generated output
 *      dp_bits   - Length pointer to private prime coefficient (dp_bits)
 *      dp        - Byte pointer (dp), generated output
 *      dq_bits   - Length pointer to private prime coefficient (dq_bits)
 *      dq        - Byte pointer (dq), generated output
 *      qinv_bits - Length pointer to private prime inverse (qinv_bits)
 *      qinv      - Byte pointer (qinv), generated output
 *
 *
 *  Description: This function takes as its inputs a public
 *      exponent and the lengths of the private values p and q.
 *      From these, this function computes all of the public and
 *      private keys necessary for RSA Public and Private CRT
 *      operations.
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_rsa_key_generate (
crypto_lib_handle *pHandle,
u32_t    modulus_bits,            /* input, requested modulus size */
u32_t *  e_bits,    u8_t *  e,    /* input */
u32_t *  p_bits,    u8_t *  p,    /* manual input or generated output */
u32_t *  q_bits,    u8_t *  q,    /* manual input or generated output */
u32_t *  n_bits,    u8_t *  n,    /* generated output */
u32_t *  d_bits,    u8_t *  d,    /* generated output */
u32_t *  dp_bits,   u8_t *  dp,   /* generated output */
u32_t *  dq_bits,   u8_t *  dq,   /* generated output */
u32_t *  qinv_bits, u8_t *  qinv  /* generated output */
)
{
    /* Can't use globals directly, must these directly */
    int i;
    u32_t *  One             = NULL;
    u32_t *  HugeModulus     = NULL;
    u32_t    OneBits         = 0;
    u32_t    HugeModulusBits = 0;

    BCM_SCAPI_STATUS  status          = BCM_SCAPI_STATUS_OK;
    BCM_SCAPI_STATUS  estatus         = BCM_SCAPI_STATUS_OK;
    BCM_SCAPI_STATUS  rpstatus        = BCM_SCAPI_STATUS_OK;

    /* Locals within rsakeygen */
    u8_t   p1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};
    u8_t   q1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};
    u8_t   p1q1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};
    u32_t    p1bits          = 0;
    u32_t    q1bits          = 0;
    u32_t    p1q1bits        = 0;
    u32_t    tmpe[64];
    u32_t    tmpzero[64];
    u32_t    tmpone[64];

    int count, manual_p, manual_q;

    int manual_exp_retries =  RSA_N_GEN_ATTEMPTS;

    q_lint_t  q_tmp;

    u32_t *thash_in;
    u32_t *tsig = NULL;
    u32_t *thash_out = NULL;
    u32_t siglen, hash_size;

    /* A local qlip heap of 4KB is sufficient for RSA key generation */
    q_lip_ctx_t qlip_ctx;
    u32_t qlip_heap[1024];
    q_ctx_init(&qlip_ctx, &qlip_heap[0],
               sizeof(qlip_heap)/sizeof(u32_t), /* In words */
               (q_yield)(OS_YIELD_FUNCTION));

    tsig = cache_line_aligned_alloc(RSA_MAX_MOD_BYTES);
    if (tsig == NULL) {
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }
    thash_out = cache_line_aligned_alloc(RSA_MAX_MOD_BYTES);
    if (thash_out == NULL) {
        cache_line_aligned_free(tsig);
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    /*
    * Restriction: requested modulus size must not exceed RSA_MAX_MODULUS_SIZE
    */
    memset(tmpe, 0x00, 256);
    memcpy(tmpe, e, 4);
    memset(tmpzero, 0x00, 256);
    memset(tmpone, 0x00, 256);

    if(modulus_bits > RSA_MAX_MODULUS_SIZE){
        return BCM_SCAPI_STATUS_CRYPTO_KEY_SIZE_MISMATCH;
    }

    /* Begin constants initialization */

    /* Initialize One */
    One = (u32_t *)q_malloc(QLIP_CONTEXT, (RSA_MAX_MATH_BYTES+4)/4);
    if(One == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(One, 0x00, RSA_MAX_MATH_BYTES+4);

    One[0] = 0x00000001;
    OneBits = 1;

    /* Initialize HugeModulus */
    HugeModulus = (u32_t *)q_malloc(QLIP_CONTEXT, RSA_MAX_MOD_BYTES/4);
    if(HugeModulus == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(HugeModulus, 0xff, RSA_MAX_MOD_BYTES);

    HugeModulusBits = modulus_bits; //RSA_MAX_MOD_BYTES << 3;

    /* Generate random primes p and q, each half the size of requested
    modulus n size.
    Note: p and q can be manually provided for test purposes(*x_bits != 0)*/

    if (*p_bits == 0){
        manual_p = 0;
    }
    else{
        manual_p = 1;
    }

    if (*q_bits == 0){
        manual_q = 0;
    }
    else{
        manual_q = 1;
    }

    manual_exp_retries=  RSA_N_GEN_ATTEMPTS;

    retry_primegen:

    count = RSA_N_GEN_ATTEMPTS;
    estatus = BCM_SCAPI_STATUS_OK;
    do
    {
        rpstatus = BCM_SCAPI_STATUS_OK;
        if (!manual_p){
            rpstatus = bcm_primerng(pHandle,
            (modulus_bits + 1)>>1,  /* divide by 2 */
            p_bits,           p,
            &OneBits,         One,
            &HugeModulusBits, HugeModulus,
            QLIP_CONTEXT);
            if (rpstatus){
                goto cleanup_and_exit;
            }
        }

        if (!manual_q) {
            rpstatus = bcm_primerng(pHandle,
            (modulus_bits + 1)>>1,  /* divide by 2 */
            q_bits,          q,
            &OneBits,         One,
            &HugeModulusBits, HugeModulus,
            QLIP_CONTEXT);
            if (rpstatus){
                goto cleanup_and_exit;
            }
        }

        /* Compute n = p * q */
        rpstatus = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
        BCM_SCAPI_MATH_MUL,
        (u8_t *)NULL, modulus_bits,
        (u8_t *)p, *p_bits,
        (u8_t *)q, *q_bits,
        (u8_t *)n, n_bits,
        NULL, NULL);

        if(rpstatus){
            goto cleanup_and_exit;
        }
        //*n_bits = BYTELEN2BITLEN(*n_bits);
        *n_bits = bcm_rsa_find_num_bits((u8_t *)n, *n_bits);
    } while ((*n_bits > modulus_bits) && --count);

    if (*n_bits > modulus_bits) {
    status |= BCM_SCAPI_STATUS_CRYPTO_ERROR;
    }

    /******************************************
    * Compute d = e ** -1 mod ((p - 1)(q - 1))
    ******************************************/
    rpstatus = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODSUB,
    (u8_t *)HugeModulus, HugeModulusBits,
    (u8_t *)p, *p_bits,
    (u8_t *)One, OneBits,
    (u8_t *)p1, &p1bits,
    NULL, NULL);

    if(rpstatus){
        goto cleanup_and_exit;
    }
    //p1bits = BYTELEN2BITLEN(p1bits);
    p1bits = bcm_rsa_find_num_bits((u8_t *)p1, p1bits);

    rpstatus = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODSUB,
    (u8_t *)HugeModulus, HugeModulusBits,
    (u8_t *)q, *q_bits,
    (u8_t *)One, OneBits,
    (u8_t *)q1, &q1bits,
    NULL, NULL);

    if(rpstatus){
        goto cleanup_and_exit;
    }
    //q1bits = BYTELEN2BITLEN(q1bits);
    q1bits = bcm_rsa_find_num_bits((u8_t *)q1, q1bits);

    rpstatus = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MUL,
    (u8_t *)NULL, modulus_bits,
    (u8_t *)p1, p1bits,
    (u8_t *)q1, q1bits,
    (u8_t *)p1q1, &p1q1bits,
    NULL, NULL);

    if(rpstatus){
        goto cleanup_and_exit;
    }
    //p1q1bits = BYTELEN2BITLEN(p1q1bits);
    p1q1bits = bcm_rsa_find_num_bits((u8_t *)p1q1, p1q1bits);

    /* At this point, we have the primes p and q, p1 = p-1, q1 = q-1,
     * and p1q1 = p1*q1        */
    /* e is supposed to be randomly chosen and relatively prime to p1q1,
     * i.e. gcd(e,p1q1) = 1 */

    /****************************************************
    * Note: e may only be manually provided (*e_bits != 0)
    ****************************************************/
    /* Manually provided public exponent e */
    estatus = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODINV,
    (u8_t *)p1q1, p1q1bits,
    (u8_t *)tmpe, p1q1bits,
    (u8_t *)NULL, 0,
    (u8_t *)d, d_bits,
    NULL, NULL);

    //*d_bits = BYTELEN2BITLEN(*d_bits);
    *d_bits = bcm_rsa_find_num_bits((u8_t *)d, *d_bits);

    if(estatus) {
        if (manual_exp_retries ==0) {
            goto cleanup_and_exit;
        }
        else {
            manual_exp_retries--;
            goto retry_primegen;
        }
    }

    /****************************
    * Compute dp = d mod (p - 1)
    ****************************/
    memcpy(tmpone, One, BITLEN2BYTELEN(OneBits));
    status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODREM,
    (u8_t *)p1, p1bits,
    (u8_t *)d, *d_bits,
    (u8_t *)NULL, 0,
    (u8_t *)dp, dp_bits,
    NULL, NULL);

    if(status != BCM_SCAPI_STATUS_OK){
        goto cleanup_and_exit;
    }

    //*dp_bits = BYTELEN2BITLEN(*dp_bits);
    *dp_bits = bcm_rsa_find_num_bits((u8_t *)dp, *dp_bits);

    /****************************
    * Compute dq = d mod (q - 1)
    ***************************/
    status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODREM,
    (u8_t *)q1, q1bits,
    (u8_t *)d, *d_bits,
    (u8_t *)NULL, 0,
    (u8_t *)dq, dq_bits,
    NULL, NULL);

    if(status != BCM_SCAPI_STATUS_OK){
        goto cleanup_and_exit;
    }

    //*dq_bits = BYTELEN2BITLEN(*dq_bits);
    *dq_bits = bcm_rsa_find_num_bits((u8_t *)dq, *dq_bits);

    /*
     * Compute qinv = q ^ -1 mod p
     */
    if (*q_bits < *p_bits) {
        /* modinv needs 256 bit base */
        memset(HugeModulus, 0, BITLEN2BYTELEN(*p_bits));
        memcpy(HugeModulus, q, BITLEN2BYTELEN(*q_bits));
        status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
                                 BCM_SCAPI_MATH_MODINV,
                                 (uint8_t *)p, *p_bits,
                                 (uint8_t *)HugeModulus, *p_bits,
                                 (uint8_t *)NULL, 0,
                                 (uint8_t *)qinv, qinv_bits,
                                 NULL, NULL);
    } else {
        status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
                                 BCM_SCAPI_MATH_MODINV,
                                 (uint8_t *)p, *p_bits,
                                 (uint8_t *)q, *q_bits,
                                 (uint8_t *)NULL, 0,
                                 (uint8_t *)qinv, qinv_bits,
                                 NULL, NULL);
    }

    if(status != BCM_SCAPI_STATUS_OK){
        goto cleanup_and_exit;
    }

    //*qinv_bits = BYTELEN2BITLEN(*qinv_bits);
    *qinv_bits = bcm_rsa_find_num_bits((u8_t *)qinv, *qinv_bits);

    /* Re-use the HugeModulus buffer for hash input and init it to some pattern
     */
    thash_in = HugeModulus;
    hash_size = modulus_bits / 8;
    for (i = 0; i < hash_size/4; i++)
        thash_in[i] = 0x38de4f25; /* Random pattern */

    /* FIPS requirement - pairwise consistency test */
    /* Sign test hash using crt method as it is faster */
    status = crypto_pka_rsa_mod_exp_crt(
                  pHandle,
                  (u8_t *)thash_in, hash_size*8,
                  dq, q, *q_bits,
                  dp, p, *p_bits,
                  qinv,
                  (u8_t *)tsig, &siglen,
                  NULL, NULL);

    if(status != BCM_SCAPI_STATUS_OK){
        goto cleanup_and_exit;
    }

    if (modulus_bits > PKA_RSA_QLIP_MAX) {
        /* FIXME: Skipping Verify for 4K keys since it doesn't seem to
         * be working
         */
        goto cleanup_and_exit;
    }

    /* Verify signature of test hash */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
    (u8_t *)n, *n_bits,
    (u8_t *)tsig, *n_bits,
    (u8_t *)e, *e_bits,
    (u8_t *)thash_out, /* output */ &siglen,
    NULL, 0);

    if(status != BCM_SCAPI_STATUS_OK){
        goto cleanup_and_exit;
    }

    if(memcmp(thash_in, thash_out, hash_size)){
        status = BCM_SCAPI_STATUS_CRYPTO_ERROR;
    }

cleanup_and_exit:

    /* Done with retry_primegen and this is also
     * the early exit point for status,
    estatus and rpstatus. Record the total status here */
    status = (status/* & ~NO_INVERSE_EXISTS*/) |
             (estatus /* & ~NO_INVERSE_EXISTS */) | rpstatus;

    /* clean up of p1, q1, p1q1 */
    memset(p1, 0x00, RSA_MAX_PUBLIC_KEY_BYTES);
    p1bits = 0;

    memset(q1, 0x00, RSA_MAX_PUBLIC_KEY_BYTES);
    q1bits = 0;

    memset(p1q1, 0x00, RSA_MAX_PUBLIC_KEY_BYTES);
    p1q1bits = 0;

    memset(HugeModulus, 0, RSA_MAX_MOD_BYTES);
    set_param_bn(&q_tmp, (u8_t *)HugeModulus,
                 BYTELEN2BITLEN(RSA_MAX_MOD_BYTES),0);
    q_free(QLIP_CONTEXT, &q_tmp);
    HugeModulus = NULL;

    memset(One, 0, RSA_MAX_MATH_BYTES+4);
    set_param_bn(&q_tmp, (u8_t *)One,
                 BYTELEN2BITLEN((RSA_MAX_MATH_BYTES+4)),0);
    q_free(QLIP_CONTEXT, &q_tmp);
    One = NULL;

    if (thash_out != NULL)
        cache_line_aligned_free(thash_out);

    if (tsig != NULL)
        cache_line_aligned_free(tsig);

    return(status);
}

/**
 * This API
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_verify (
                    crypto_lib_handle *pHandle,
                    u32_t nLen, u8_t *n, /* modulus */
                    u32_t eLen, u8_t *e, /* exponent */
                    u32_t MLen, u8_t *M, /* message */
                    u32_t SLen, u8_t *S,  /* signature */
                    BCM_HASH_ALG hashAlg
                    )
{
    u8_t EM[2048]; /* 512 bytes, supports upto 4096 bit keys */
    u8_t EM2[2048]; /* this is the EM' */
    u32_t modexp_len;
    BCM_SCAPI_STATUS status;
#ifdef  CRYPTO_DEBUG
    u32_t i;
    printf("Enter bcm_rsassa_pkcs1_v15_verify\n");
#endif
    /* Step1: length checking */
    /* Remove this check to verify Montgomery pre-computation
    if (SLen != nLen)
    return BCM_SCAPI_STATUS_RSA_PKCS1_LENGTH_NOT_MATCH; */

    /* Step2: RSA verification */
    /* Step2a: OS2IP(s) */
    /* We can skip this step since it's just big endian word */

    /* Step2b: RSAVP1(n, e, s), e.g. output = s^e mod n */

    /* need to change to PKE hardware format */

    if( nLen > 512 )
    {
#ifdef  CRYPTO_DEBUG
    printf("nLen > 512\n");
#endif
        status = crypto_pka_mont_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
        (u8_t *)n, BYTELEN2BITLEN(nLen), /* modulus */
        (u8_t *)S, BYTELEN2BITLEN(SLen), /* message, e.g. signature */
        (u8_t *)e, BYTELEN2BITLEN(eLen), /* exponent */
        (u8_t *)EM, /* output */ &modexp_len,
        NULL, 0);
    }
    else
    {
#ifdef  CRYPTO_DEBUG
    printf("nLen <= 512\n");
#endif

        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
        (u8_t *)n, BYTELEN2BITLEN(nLen), /* modulus */
        (u8_t *)S, BYTELEN2BITLEN(SLen), /* message, e.g. signature */
        (u8_t *)e, BYTELEN2BITLEN(eLen), /* exponent */
        (u8_t *)EM, /* output */ &modexp_len,
        NULL, 0);
    }

    if (status != BCM_SCAPI_STATUS_OK){
        return status;
    }

    /* Step2c: I2OSP(output) --- modexpOutput is the EM */
#ifdef  CRYPTO_DEBUG
    printf("Step 2 : bcm_swap_words\n");
#endif
    bcm_int2bignum((u32_t *)EM, SLen); //bcm_swap_words
#ifdef  CRYPTO_DEBUG
    for(i=0; i< SLen; i+=4)
    {
        printf("EM : 0x%x 0x%x 0x%x 0x%x \n", EM[i], EM[i+1], EM[i+2], EM[i+3]);
    }
#endif

    /* Step3: EMSA-PKCS1-v1_5 encoding */
#ifdef  CRYPTO_DEBUG
    printf("Step 3: EMSA-PKCS1-v1_5 encoding\n");
#endif
    status = emsa_pkcs1_v15_encode(pHandle, MLen, M,
                                              SLen, EM2, hashAlg);
    if (status != BCM_SCAPI_STATUS_OK){
        return status;
    }

    /* Step4: compare */
#ifdef  CRYPTO_DEBUG
    printf("Step 4: compare\n");
    printf("SLen = %d\n", SLen);
#endif

    if (!secure_memeql(EM, EM2, SLen)) {
#ifdef  CRYPTO_DEBUG
    printf("Return BCM_SCAPI_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL\n");
#endif
        return BCM_SCAPI_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL;
    }
#ifdef  CRYPTO_DEBUG
else
    printf("Return BCM_SCAPI_STATUS_OK\n");
#endif
#ifdef  CRYPTO_DEBUG
    printf("Exit bcm_rsassa_pkcs1_v15_verify\n");
#endif
    return BCM_SCAPI_STATUS_OK;

}

/* generate DSA public and private keys */
/* private key will always be 20 bytes  */
BCM_SCAPI_STATUS crypto_pka_dsa_key_generate(crypto_lib_handle *pHandle,
				u32_t l, u32_t n,
				u8_t *p, u8_t *q, u8_t *g,
				u8_t *privkey, u8_t *pubkey, u32_t bPrivValid)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_CRYPTO_ERROR;
	u32_t pubkey_len;

	ARG_UNUSED(q);

	/* First generate the private key if passed in is not valid */
	if (!bPrivValid) {
		status = crypto_rng_generate(pHandle,
				NULL, 0,
				privkey, WORDLEN2BYTELEN(5),
				NULL, 0);
		if (status != BCM_SCAPI_STATUS_OK)
			return status;
	}

	/* Don't need to check that the private key is less than q ---
	 * we did mod q in generation of rng
	 */

	/* Generate the public key: g^x mod p, x is the privkey */
	status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
					(u8_t *)p, l,
					(u8_t *)g, n,
					privkey, BYTELEN2BITLEN(DSA_SIZE_X),
					pubkey, &pubkey_len, NULL, NULL);

	return status;
}

/*----------------------------------------------------------------------------
 * Name    : crypto_pka_dsa_sign
 * Purpose : Perform a DSA signature
 * Input   : pointers to the data parameters
 *           hash: SHA1 hash of the message, length is 20 bytes
 *           p: 512 bit to 1024 bit prime, length must be exact in 64 bit
 *           	increments
 *           q: 160 bit prime
 *           g: = h**(p-1)/q mod p, where h is a generator. g length is same
 *           	as p length
 *           random: random number, length is 20 bytes,
                                 generated internally if specified NULL
 *           x: private key, length is 20 bytes
 * Output  : rs: the signature, first part is r, followed by s, each is q length
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *----------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_dsa_sign (
                  crypto_lib_handle *pHandle,
                  u8_t *hash,
                  u8_t *random,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *q,
                  u8_t *g,
                  u8_t *x,
                  u8_t *rs,     u32_t *rs_bytelen,
                  scapi_callback   callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t   p_len_adj;
    q_dsa_param_t dsa;
    q_signature_t qlip_rs;   /* q_signature pointer to rs */
    q_lint_t d;         /* q_lint pointer to d */
    q_lint_t h;         /* q_lint poitner to h */
    q_lint_t k;         /* q_lint pointer to k */

    u8_t  randk[DSA_SIZE_RANDOM];
    LOCAL_QLIP_CONTEXT_DEFINITION;


    if (!(pHandle && hash && p && q && g && x && rs)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)

    /* Find parameter size according to the prime length */
    status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    if(NULL == random) {
	status = crypto_rng_generate(pHandle, NULL, 0, randk, DSA_SIZE_RANDOM,
					NULL, 0);

        if (status != BCM_SCAPI_STATUS_OK){
            return status;
        }
        /* Convert the random number from word format to BIGNUM format */
        bcm_swap_words((u32_t *)randk, DSA_SIZE_RANDOM);
        random = randk;
    }

    /*
    * Mapping: p==p,q==q,g==g,h==hash,d==x,k==random,rs==rs
    */

    /* Set q, 20 bytes */
    set_param_bn(&dsa.q, q,BYTELEN2BITLEN(DSA_SIZE_Q), DSA_SIZE_Q);
    /* Set p */
    set_param_bn(&dsa.p,p, p_bitlen, p_len_adj);
    /* Set g */
    set_param_bn(&dsa.g, g,p_bitlen, p_len_adj);  /* SOR p_bitlen ??*/
    /* Set private key, 20 bytes */
    set_param_bn(&d,x, BYTELEN2BITLEN(DSA_SIZE_X), DSA_SIZE_X);
    /* Set hash, 20 bytes */
    set_param_bn(&h,hash, BYTELEN2BITLEN(DSA_SIZE_HASH), DSA_SIZE_HASH);
    /* Set random number */
    set_param_bn(&k,random, BYTELEN2BITLEN(DSA_SIZE_RANDOM), DSA_SIZE_RANDOM);

    /* Set rs, we need to break into two fragments. */
    set_param_bn(&qlip_rs.r,rs,BYTELEN2BITLEN(DSA_SIZE_R), DSA_SIZE_R);
    set_param_bn(&qlip_rs.s,&rs[DSA_SIZE_R], BYTELEN2BITLEN(DSA_SIZE_S),
                 DSA_SIZE_S);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */
    /* Call the qlip function. */
    status = q_dsa_sign (QLIP_CONTEXT,    /* QLIP context pointer */
                        &qlip_rs,   /* q_signature pointer to rs */
                        &dsa,                /* q_dsa_param pointer to dsa */
                        &d,         /* q_lint pointer to d */
                        &h,         /* q_lint poitner to h */
                        &k) ;        /* q_lint pointer to k */

    pHandle -> busy        = FALSE;
    /* Command is completed */
    *rs_bytelen = DSA_SIZE_SIGNATURE; /* Is this needed */
    if (callback == NULL)
    {

    }
    else
    {
        /* assign context for use in check_completion */
        pHandle -> result      = rs;
        pHandle -> result_len  = DSA_SIZE_SIGNATURE;
        pHandle -> presult_len = rs_bytelen;
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        /* Since in sync mode call back now */
        (callback) (status, pHandle, pHandle->arg);
    }

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_dsa_verify
 * Purpose : Verify a DSA signature (private key: x, public key: p, q, g, y.
 * Input   : pointers to the data parameters
 *           p: 512 bit to 1024 bit prime, in 64 bits increment
 *           q: 160 bit prime
 *           g: = h**(p-1)/q mod p, where h is a generator, g length is
 *           same as p length
 *           y: = g**x mod p, length is same as p length
 *           r,s: signature
 *           For parameters q, r, s, they don't have a length field,
 *           since it's fixed 20 bytes long.
 * Return  : v. If v == r then verify success
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_dsa_verify (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *q,
                    u8_t *g,
                    u8_t *y,
                    u8_t *r,    u8_t  *s,
                    u8_t *v,    u32_t *v_bytelen,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t   p_len_adj;
    q_lint_t qlip_v;         /* q_lint pointer to v */
    q_dsa_param_t dsa;  /* q_dsa_param pointer to dsa */
    q_lint_t qlip_y;         /* q_lint pointer to y */
    q_lint_t h;         /* q_lint pointer to h */
    q_signature_t qlip_rs;   /* q_signature pointer to rs */
    LOCAL_QLIP_CONTEXT_DEFINITION;

    if (!(pHandle && p && q && g && y && r && s && v)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)


    /* Find parameter size according to the prime length */
    status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    /*
    * Mapping,p==p,q==q,g==g,r==r,s==s,h==hash,v==v
    */

    /* Set q */
    set_param_bn(&dsa.q,q, BYTELEN2BITLEN(DSA_SIZE_Q), DSA_SIZE_Q);
    /* Set p */
    set_param_bn(&dsa.p,p, p_bitlen, p_len_adj);
    /* Set g */
    set_param_bn(&dsa.g, g, p_bitlen, p_len_adj);

    /* Set public key */
    set_param_bn(&qlip_y,y, p_bitlen, p_len_adj);
    set_param_bn(&qlip_v,v, p_bitlen, p_len_adj);
    /* Set hash */
    set_param_bn(&h,hash, BYTELEN2BITLEN(DSA_SIZE_HASH), DSA_SIZE_HASH);
    /* Set signature */
    set_param_bn(&qlip_rs.r,r, BYTELEN2BITLEN(DSA_SIZE_R), DSA_SIZE_R);
    set_param_bn(&qlip_rs.s,s, BYTELEN2BITLEN(DSA_SIZE_S), DSA_SIZE_S);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */
    /* Call the qlip function. */
    status = q_dsa_verify (QLIP_CONTEXT,    /* QLIP context pointer */
                            &qlip_v,         /* q_lint pointer to v */
                            &dsa,  /* q_dsa_param pointer to dsa */
                            &qlip_y,         /* q_lint pointer to y */
                            &h,         /* q_lint pointer to h */
                            &qlip_rs);   /* q_signature pointer to rs */

    pHandle -> busy        = FALSE;
    /* Command is completed. */
    *v_bytelen = DSA_SIZE_V; /* Do we need to do this */

    if (callback == NULL)
    {

    }
    else
    {
        /* assign context for use in check_completion */
        pHandle -> result      = v;
        pHandle -> result_len  = DSA_SIZE_V;
        pHandle -> presult_len = v_bytelen;
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        /* Since in sync mode call back now */
        (callback) (status, pHandle, pHandle->arg);
    }

    return status;
}


/*-------------------------------------------------------------------------------
 * Name    : crypto_pka_dsa_sign_2048
 * Purpose : Perform a DSA signature
 * Input   : pointers to the data parameters
 *           hash: SHA1 hash of the message, length is 20 bytes
 *           p: 2048 bit prime
 *           q: 160-256 bit prime
 *           g: = h**(p-1)/q mod p, where h is a generator. g length is same as p length
 *           random: random number, length is 20 bytes,
 *                   generated internally if specified NULL
 *           x: private key, length is 20 bytes
 * Output  : rs: the signature, first part is r, followed by s, each is q length
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *------------------------------------------------------------------------------*/
BCM_SCAPI_STATUS
crypto_pka_dsa_sign_2048(crypto_lib_handle *pHandle,
             u8_t *hash,
             u8_t *random,
             u8_t *p,      u32_t p_bitlen,
             u8_t *q,
             u8_t *g,
             u8_t *x,
             u8_t *rs,     u32_t *rs_bytelen,
             scapi_callback   callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u8_t  k[DSA_SIZE_RANDOM_2048];
    u8_t  k_inv[DSA_SIZE_RANDOM_2048];
    u8_t  tmp[DSA_SIZE_P_DEFAULT_2048];
    u8_t  tmp2[DSA_SIZE_P_DEFAULT_2048];
    u32_t q_bitlen = DSA_SIZE_Q_2048 * 8;
    u32_t g_bitlen = p_bitlen;
    u32_t x_bitlen = DSA_SIZE_X_2048 * 8;
    u32_t k_bytelen;
    u32_t k_inv_bytelen;
    u32_t tmp_bytelen;
    u32_t tmp2_bytelen;
    u8_t zero[DSA_SIZE_R_2048] = {0};
    u8_t r[DSA_SIZE_R_2048], s[DSA_SIZE_S_2048];
    u32_t r_bytelen, s_bytelen;
    u8_t failed;

    if (!(pHandle && hash && p && q && g && x && rs))
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (p_bitlen != DSA_SIZE_P_DEFAULT_2048 * 8)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (callback != NULL)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (arg != NULL)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    // in case we exit early, signature size is preset to zero
    *rs_bytelen = 0;

    secure_memrev(q, q, q_bitlen / 8);
    secure_memrev(p, p, p_bitlen / 8);
    secure_memrev(g, g, g_bitlen / 8);
    secure_memrev(x, x, x_bitlen / 8);
    secure_memrev(hash, hash, q_bitlen / 8);

    do {
        if (NULL == random) {
            memset(k, 0x0, sizeof(k));

            status = bcm_dsa_genk(pHandle, q, q_bitlen, k, &k_bytelen,
            						k_inv, &k_inv_bytelen);
            if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;
        } else {
            k_bytelen = q_bitlen/8;
            secure_memrev(k, random, k_bytelen);

            status = crypto_pka_math_accelerate(pHandle,
                                         BCM_SCAPI_MATH_MODINV,
                                         q, q_bitlen,
                                         k, q_bitlen,
                                         NULL, 0,
                                         k_inv, &k_inv_bytelen,
                                         NULL, 0);
            if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

            random = NULL; /* force regeneration if k leads to invalid r or s */
        }
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        /* r = (g^k mod p) mod q, so...*/
        /* tmp = (g^k mod p) */
        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
                                     p, p_bitlen,
                                     g, p_bitlen,
                                     k, k_bytelen * 8,
                                     tmp, &tmp_bytelen,
                                     NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        /* r = (tmp) mod q */
        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODREM,
                                     q, q_bitlen,
                                     tmp, tmp_bytelen * 8,
                                     NULL, 0,
                                     r, &r_bytelen,
                                     NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        /* s = k^-1 (hash + x * r) mod q, so... */

        /* tmp = (x * r) mod q*/
        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODMUL,
                                     q, q_bitlen,
                                     x, x_bitlen,
                                     r, r_bytelen * 8,
                                     tmp, &tmp_bytelen,
                                     NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        /* tmp2 = (hash + tmp) mod q */
        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODADD,
                                     q, q_bitlen,
                                     hash, q_bitlen,
                                     tmp, tmp_bytelen * 8,
                                     tmp2, &tmp2_bytelen,
                                     NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        /* s = (k_inv * tmp2) mod q */
        status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODMUL,
                                     q, q_bitlen,
                                     k_inv, k_inv_bytelen * 8,
                                     tmp2, tmp2_bytelen * 8,
                                     s, &s_bytelen,
                                     NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto DSA_SIGN_RETURN;

        // R should not equal null
        failed = (!secure_memeql(r, zero, r_bytelen) == 0);
        // S should not equal null
        failed |= (!secure_memeql(s, zero, s_bytelen) == 0);
        // R should be the right length
        failed |= (r_bytelen != q_bitlen/8);
        // S should be the right length
        failed |= (s_bytelen != q_bitlen/8);
    } while (failed);


    secure_memrev(rs, r, r_bytelen);
    secure_memrev(rs + r_bytelen, s, s_bytelen);

    *rs_bytelen = DSA_SIZE_SIGNATURE_2048;

DSA_SIGN_RETURN:
    secure_memrev(hash, hash, q_bitlen / 8);
    secure_memrev(p, p, p_bitlen / 8);
    secure_memrev(q, q, q_bitlen / 8);
    secure_memrev(g, g, g_bitlen / 8);
    secure_memrev(x, x, x_bitlen / 8);

    secure_memclr(k, DSA_SIZE_RANDOM_2048);
    secure_memclr(k_inv, DSA_SIZE_RANDOM_2048);
    secure_memclr(tmp, DSA_SIZE_P_DEFAULT_2048);
    secure_memclr(tmp2, DSA_SIZE_P_DEFAULT_2048);
    secure_memclr(r, DSA_SIZE_R_2048);
    secure_memclr(s, DSA_SIZE_S_2048);

    return status;
}


/*---------------------------------------------------------------
 * Name    : crypto_pka_dsa_verify_2048
 * Purpose : Verify a DSA signature (private key: x, public key: p, q, g, y.
 * Input   : pointers to the data parameters
 *           p: 2048 bit
 *           q: 160 bit prime
 *           g: = h**(p-1)/q mod p, where h is a generator, g length is same as p length
 *           y: = g**x mod p, length is same as p length
 *           r,s: signature
 *           For parameters q, r, s, they don't have a length field, since it's fixed 20 bytes long.
 * Return  : v. If v == r then verify success
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_dsa_verify_2048 (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u8_t *p,    u32_t p_bitlen,
                    u8_t *q,
                    u8_t *g,
                    u8_t *y,
                    u8_t *r,    u8_t  *s,
                    u8_t *v,    u32_t *v_bytelen,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t s_bytelen = DSA_SIZE_S_2048, r_bytelen = DSA_SIZE_R_2048;
    u32_t q_bitlen = DSA_SIZE_Q_2048 * 8;
    u32_t g_bitlen = p_bitlen, y_bitlen = p_bitlen;

    u8_t w[SHA256_HASH_SIZE], u1[SHA256_HASH_SIZE], u2[SHA256_HASH_SIZE];
    u32_t w_bytelen, u1_bytelen, u2_bytelen;
    u8_t tmp1[DSA_SIZE_P_DEFAULT_2048], tmp2[DSA_SIZE_P_DEFAULT_2048], tmp3[DSA_SIZE_P_DEFAULT_2048];
    u32_t tmp1_bytelen, tmp2_bytelen, tmp3_bytelen;

    // directly return since we have not reversed inputs
    if (callback != NULL)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (!(pHandle && p && q && g && y && r && s && v))
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    if (p_bitlen != DSA_SIZE_P_DEFAULT_2048 * 8)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    /* reject if r >= q or s >= q */
    if (memcmp(r, q, r_bytelen) >= 0 || memcmp(s, q, s_bytelen) >= 0)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (arg != NULL)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    secure_memrev(hash, hash, SHA256_HASH_SIZE);
    secure_memrev(p, p, p_bitlen / 8);
    secure_memrev(q, q, q_bitlen / 8);
    secure_memrev(g, g, g_bitlen / 8);
    secure_memrev(y, y, y_bitlen / 8);
    secure_memrev(r, r, r_bytelen);
    secure_memrev(s, s, s_bytelen);

    /* w = s^-1 mod q*/
    status = crypto_pka_math_accelerate(pHandle,
                                 BCM_SCAPI_MATH_MODINV,
                                 q, q_bitlen,
                                 s, s_bytelen*8,
                                 NULL, 0,
                                 w, &w_bytelen,
                                 NULL, 0);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /* u1 = (hash * w) mod q */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODMUL,
                                 q, q_bitlen,
                                 hash, SHA256_HASH_SIZE * 8,
                                 w, w_bytelen * 8,
                                 u1, &u1_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /* u2 = (r * w) mod q */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODMUL,
                                 q, q_bitlen,
                                 r, r_bytelen * 8,
                                 w, w_bytelen * 8,
                                 u2, &u2_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /* v = ((g^u1 * y^u2) mod p) mod q ... */
    /*   tmp1 = g^u1 mod p */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
                                 p, p_bitlen,
                                 g, g_bitlen,
                                 u1, u1_bytelen * 8,
                                 tmp1, &tmp1_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /*   tmp2 = y^u2 mod p */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
                                 p, p_bitlen,
                                 y, y_bitlen,
                                 u2, u2_bytelen * 8,
                                 tmp2, &tmp2_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /*   tmp3 = tmp1 * tmp2 mod p */
    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODMUL,
                                 p, p_bitlen,
                                 tmp1, tmp1_bytelen * 8,
                                 tmp2, tmp2_bytelen * 8,
                                 tmp3, &tmp3_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /*   v = (tmp3) mod q */

    status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODREM,
                                 q, q_bitlen,
                                 tmp3, tmp3_bytelen * 8,
                                 NULL, 0,
                                 v, v_bytelen,
                                 NULL, NULL);
    if (status != BCM_SCAPI_STATUS_OK) goto DSA_VERIFY_RETURN;

    /* sig is valid if v == r, determined by the caller */

DSA_VERIFY_RETURN:
    secure_memrev(hash, hash, SHA256_HASH_SIZE);
    secure_memrev(p, p, p_bitlen / 8);
    secure_memrev(q, q, q_bitlen / 8);
    secure_memrev(g, g, g_bitlen / 8);
    secure_memrev(y, y, y_bitlen / 8);
    secure_memrev(r, r, r_bytelen);
    secure_memrev(s, s, s_bytelen);
    secure_memrev(v, v, *v_bytelen);

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_diffie_hellman_generate
 * Purpose : Generate a Diffie-Hellman key pair
 * Input   : pointers to the data parameters
 *           x: the secret value, has actual bit length (IN/OUT)
 *           xvalid: 1 - manual input of private key pointed by x,
 *                   0 - generate private key internally.
 *           g: generator, has actual bit length
 *           m: modulus, has actual bit length
 * Output  : y: the public value. y = g**x mod m, y length is find_mod_size(m_bitlen)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
             g and x are rounded up to 32 bits
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_diffie_hellman_generate (
                    crypto_lib_handle *pHandle,
                    u8_t *x,      u32_t x_bitlen,
                    u32_t xvalid,
                    u8_t *y,      u32_t *y_bytelen,
                    u8_t *g,      u32_t g_bitlen,
                    u8_t *m,      u32_t m_bitlen,
                    scapi_callback callback, void *arg
                    )
{
	BCM_SCAPI_STATUS status;
	q_dh_param_t dh_params; /* DH Params to qlip funcion parameters */
	q_lint_t xpub;       /* pointer to public value q_lint */
	q_lint_t qlip_x;         /* pointer to private secret value q_lint */
	u32_t m_len_adj, x_len_adj; /* all in bytes */
	u32_t *tmp;
	s32_t it;
	crypto_lib_handle *old_pHandle = pHandle;

	LOCAL_QLIP_CONTEXT_DEFINITION;

	if (!(pHandle && x && y && g && m)){
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
	CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

	/* adjust lengths */
	status = find_mod_size(BITLEN2BYTELEN(m_bitlen), &m_len_adj);
	if (status != BCM_SCAPI_STATUS_OK){
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	x_len_adj = (ROUNDUP_TO_32_BIT(x_bitlen) >> 3);
	if (!xvalid) {
		/* value in x is not valid, generate it internally */
		it = (x_len_adj + DSA_SIZE_RANDOM -1)/DSA_SIZE_RANDOM;
		tmp = q_malloc(QLIP_CONTEXT, it * DSA_SIZE_RANDOM >> 2);

		if(!tmp) {
			return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		}

		status = crypto_rng_generate(old_pHandle, NULL, 0,
					    (u8_t*)tmp, it * DSA_SIZE_RANDOM,
					    NULL,0);
		if (status != BCM_SCAPI_STATUS_OK) {
			return status;
		}

		memcpy(x,tmp, x_len_adj);

		set_param_bn(&qlip_x,(u8_t *)tmp,
			     BYTELEN2BITLEN(it * DSA_SIZE_RANDOM),0);
		q_free(QLIP_CONTEXT, &qlip_x);
	}

	/*
	* Mapping: y==xpub,g==g,m==p,x==x
	*/

	/* Set m, pad to nearest modulus length */
	set_param_bn(&dh_params.p,m, m_bitlen, m_len_adj);
	/* Set g, need to pad to modulus length */
	set_param_bn(&dh_params.g, g, g_bitlen, m_len_adj);
	/* Set y, need to pad to modulus length */
	set_param_bn(&xpub, y, m_bitlen, m_len_adj);
	/* Set x, pad to 32 bits */
	set_param_bn(&qlip_x,x, x_bitlen, x_len_adj);

	pHandle -> busy        = TRUE; /* Prevent reentrancy */
	/* Call the q_lip function.*/
	status = q_dh_pk (QLIP_CONTEXT, /* QLIP context pointer */
	&xpub,      /* pointer to public value q_lint */
	&dh_params, /* pointer to DH parameter structure */
	&qlip_x);   /* pointer to private secret value q_lint */

	pHandle->busy = FALSE;

	/* Command is completed at this stage. Result should be copied back.  */
	*y_bytelen = m_len_adj; /* Not Sure this copy is needed. */


	if (callback == NULL)
	{
		/* Set the result len and copyback the data */
	}
	else
	{
		/* assign context for use in check_completion */
		pHandle -> result      = y;
		pHandle -> result_len  = m_len_adj;
		pHandle -> presult_len = y_bytelen;
		pHandle -> callback    = callback;
		pHandle -> arg         = arg;
		/* Since in sync mode call back now */
		(callback) (status, pHandle, pHandle->arg);
	}

	return status;
}

/*---------------------------------------------------------------------------
 * Name    : crypto_pka_diffie_hellman_shared_secret
 * Purpose : Generate a Diffie-Hellman shared secret
 * Input   : pointers to the data parameters
             x: the secret value, has actual bit length
             y: the other end public value, has actual bit length
             m: modulus, has actual bit length
 * Output  : k: the shared secret. k = y**x mod m, k length is find_mod_size(m_bitlen)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_diffie_hellman_shared_secret (
                     crypto_lib_handle *pHandle,
                     u8_t *x, u32_t x_bitlen,
                     u8_t *y, u32_t y_bitlen,
                     u8_t *m, u32_t m_bitlen,
                     u8_t *k, u32_t *k_bytelen,
                     scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t m_len_adj, x_len_adj;
    q_dh_param_t dh;        /* QLI{P  DH parameter strcture */
    q_lint_t ss;           /* q_lint pointer to DH shared secret_lint */
    q_lint_t xpub;         /* q_lint pointer to DH public value_lint */
    q_lint_t qlip_y;            /* q_lint pointer to DH private secret */
    LOCAL_QLIP_CONTEXT_DEFINITION;


    if (!(pHandle && x && y && k && m)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

    /* adjust lengths */
    status = find_mod_size(BITLEN2BYTELEN(m_bitlen), &m_len_adj);
    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    x_len_adj = ROUNDUP_TO_32_BIT(x_bitlen) >> 3;

    /*
    * Mapping: p==m,x==qlip_y,k==ss,xpub==y
    *
    */
    /* Set m */
    set_param_bn(&dh.p, m, m_bitlen, m_len_adj);
    /* Set y, round up to modulus size */
    set_param_bn(&qlip_y,x, x_bitlen, x_len_adj);
    /* Set x, round up to 32 bits */
    set_param_bn(&ss,k, m_bitlen, m_len_adj);
    /* Set x, round up to 32 bits */
    set_param_bn(&xpub,y, y_bitlen, m_len_adj);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */

    /* Call the qlip function. */
    status = q_dh_ss (QLIP_CONTEXT,/* QLIP context pointer */
                        &ss,       /* q_lint pointer to DH shared secret_lint */
                        &dh,       /* pointer to DH parameter strcture */
                        &xpub,     /* q_lint pointer to DH public value_lint */
                        &qlip_y);  /* q_lint pointer to DH private secret */

    pHandle -> busy        = FALSE;
    /* Function is complete here. */
    *k_bytelen = m_len_adj;  /* Data should be already copied back */

    if (callback == NULL)
    {

    }
    else
    {
        /* assign context for use in check_completion */
        pHandle -> result      = k;
        pHandle -> result_len  = m_len_adj;
        pHandle -> presult_len = k_bytelen;
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        /* Since in sync mode call back now */
        (callback) (status, pHandle, pHandle->arg);
    }

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_ecp_diffie_hellman_generate
 * Purpose : Generate an Elliptic Curve Diffie-Hellman key pair
 * Input   : pointers to the data parameters
 *                       type: Curve type, 0 - Prime field, 1 - binary field
 *                       p: prime p
 *                       p_bitlen: size of prime p in bits
 *                       a: Parameter a
 *                       b: Parameter b
 *                       n: The order of the Base point G (represented by "r"
 *                          in sample curves defined in FIPS 186-2 document)
 *                       Gx: x coordinate of Base point G
 *                       Gy: y coordinate of Base point G
 *                       d: private key, random integer from [1, n-1]
 *                       dvalid: 1 - manual input of private key pointed by d,
 *                               0 - generate private key internally.
 *                       Qx: x coordinate of public key Q
 *                       Qy: y coordinate of public key Q
 * Output  : Qx and Qy, x and y coordinates of point Q, public key (Q = dG)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_ecp_diffie_hellman_generate (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *d,
                  u32_t dvalid,
                  u8_t *Qx,
                  u8_t *Qy,
                  scapi_callback callback, void *arg
                  )
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	crypto_lib_handle *old_pHandle = pHandle;
	u32_t   p_len_adj;
	q_point_t  ptG;     /* q_point pointer to base point G */
	q_point_t  ptQ;     /* q_point pointer to base point Q */
	q_curve_t  ec;      /* q_point pointer to curve parameters */
	q_lint_t   qd;      /* q_lint pointer to d */
	u8_t *Gz, *Qz;
	int i;
	u32_t *rand = NULL;
	u8_t *tmp;
	s32_t len, it = 0;

	LOCAL_QLIP_CONTEXT_DEFINITION;

	ARG_UNUSED(arg);
	if (!(pHandle && p && a && b && n && Gx && Gy && Qx && Qy))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
	CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)

	/* Find parameter size according to the prime length */
	status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);
	if (status != BCM_SCAPI_STATUS_OK)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	if (!dvalid) {
		/* value in d is not valid, generate it internally */
		len = (p_bitlen + 7) >> 3;
		it = (len + DSA_SIZE_RANDOM -1)/DSA_SIZE_RANDOM;
		rand = q_malloc(QLIP_CONTEXT, it * DSA_SIZE_RANDOM >> 2);
		if(!rand)
			return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;

		status = crypto_rng_generate(old_pHandle, NULL, 0,
					(u8_t*)rand, DSA_SIZE_RANDOM * it,
					NULL, 0);
		if (status != BCM_SCAPI_STATUS_OK)
			goto DH_GENERATE_POST_RAND;

		/* Mask extra bits if any so that the actual random number is of
		p_bitlen */
		tmp = (u8_t *)rand;
		if((it * DSA_SIZE_RANDOM * 8) - p_bitlen) {
			memset(tmp+len, 0, (it * DSA_SIZE_RANDOM) - len);
			tmp[len-1] &= (1 << (8 - ((len * 8) - p_bitlen))) - 1;
		}
		status = crypto_pka_math_accelerate(pHandle,
						BCM_SCAPI_MATH_MODREM,
						n, p_bitlen,
						(u8_t *)rand, p_bitlen,
						NULL, 0,
						d, (u32_t *) &len,
						NULL, NULL);
		if (status != BCM_SCAPI_STATUS_OK)
			goto DH_GENERATE_POST_RAND;
	}

	/*
	* Mapping: one-to-one
	*/

	/* Set curve parameters */
	ec.type = type;
	set_param_bn(&ec.n, n,p_bitlen, 0);

	Gz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Gz) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto DH_GENERATE_POST_RAND;
	}

	Qz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Qz) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto DH_GENERATE_POST_G;
	}

	for (i = 0; i < (ec.n.alloc * 4); i++) {
		Gz[i] = 0;
		Qx[i] = 0;
		Qy[i] = 0;
		Qz[i] = 0;
	}

	((u32_t *)Gz)[0] = 0x00000001;

	set_param_bn(&ec.a, a,p_bitlen, 0);
	set_param_bn(&ec.b, b,p_bitlen, 0);
	set_param_bn(&ec.p, p,p_bitlen, 0);

	/* Set Base pointer G parameters */
	set_param_bn(&ptG.X,Gx, p_bitlen, 0);
	set_param_bn(&ptG.Y,Gy, p_bitlen, 0);
	set_param_bn(&ptG.Z,Gz, p_bitlen, 0);

	/* Set private key */
	set_param_bn(&qd,d, p_bitlen, 0);

	/* size = ec.n.alloc; */

	set_param_bn(&ptQ.X,Qx, p_bitlen, 0);
	set_param_bn(&ptQ.Y,Qy, p_bitlen, 0);
	set_param_bn(&ptQ.Z,Qz, p_bitlen, 0);

	pHandle -> busy        = TRUE; /* Prevent reentrancy */
	/* Call the q_lip function.*/
	status = q_ecp_pt_mul (QLIP_CONTEXT,
				&ptQ,   /* pointer to result point Q */
				&ptG,   /* pointer to Base point G */
				&qd,    /* pointer to private key d */
				&ec);  /* curve pointer */

	pHandle->busy = FALSE;

	/* de-allocate memory chunks allocated by q_malloc(),
	in the reverse order in which they are allocated */

	q_free(QLIP_CONTEXT, &ptQ.Z);
DH_GENERATE_POST_G:
	q_free(QLIP_CONTEXT, &ptG.Z);
DH_GENERATE_POST_RAND:
	if (!dvalid) {
		set_param_bn(&ptG.Z,(u8_t *)rand,
			     BYTELEN2BITLEN(it * DSA_SIZE_RANDOM), 0);
		q_free(QLIP_CONTEXT, &ptG.Z);
	}

	return status;
}

/*---------------------------------------------------------------------------
 * Name    : crypto_pka_ecp_diffie_hellman_shared_secret
 * Purpose : Generate an Elliptic Curve Diffie-Hellman shared secret
 * Input   : pointers to the data parameters
 *                       type: Curve type, 0 - Prime field, 1 - binary field
 *                       p: prime p
 *                       p_bitlen: size of prime p in bits
 *                       a: Parameter a
 *                       b: Parameter b
 *                       n: The order of the Base point G (represented by "r"
 *                          in sample curves defined in FIPS 186-2 document)
 *           d: private key, random integer from [1, n-1]
 *                       Qx: x coordinate of other end public key Q
 *                       Qy: y coordinate of other end public key Q
 * Output  : Kx: the shared secret. (Kx, Ky, Kz) = dQ
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_ecp_diffie_hellman_shared_secret (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *d,
                  u8_t *Qx,
                  u8_t *Qy,
                  u8_t *Kx,
                  u8_t *Ky,
                  scapi_callback callback, void *arg
                  )
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    u32_t   p_len_adj;
    q_point_t  ptK;     /* q_point pointer to point K */
    q_point_t  ptQ;     /* q_point pointer to point Q */
    q_curve_t  ec;      /* q_point pointer to curve parameters */
    q_lint_t   qd;         /* q_lint pointer to d */
    u8_t *Qz, *Kz;
    int i;
    LOCAL_QLIP_CONTEXT_DEFINITION;

    if (!(pHandle && p && a && b && n && Qx && Qy && d && Kx && Ky)){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    if (arg != NULL){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)

    /* Find parameter size according to the prime length */
    status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);

    if (status != BCM_SCAPI_STATUS_OK){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    /*
    * Mapping: one-to-one
    */

    /* Set curve parameters */
    ec.type = type;
    set_param_bn(&ec.n, n,p_bitlen, 0);

    Qz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
    Kz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);

    if(!(Qz && Kz)){
        return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
    }

    for (i = 0; i < (ec.n.alloc * 4); i++) {
        Qz[i] = 0;
        Kx[i] = 0;
        Ky[i] = 0;
        Kz[i] = 0;
    }

    ((u32_t*)Qz)[0] = 0x00000001;

    set_param_bn(&ec.a, a,p_bitlen, 0);
    set_param_bn(&ec.b, b,p_bitlen, 0);
    set_param_bn(&ec.p, p,p_bitlen, 0);

    /* Set point Q parameters */
    set_param_bn(&ptQ.X,Qx, p_bitlen, 0);
    set_param_bn(&ptQ.Y,Qy, p_bitlen, 0);
    set_param_bn(&ptQ.Z,Qz, p_bitlen, 0);

    /* Set private key */
    set_param_bn(&qd,d, p_bitlen, 0);

    /* Variable not being used after assigned hence removing */
    //size = ec.n.alloc;

    set_param_bn(&ptK.X,Kx, p_bitlen, 0);
    set_param_bn(&ptK.Y,Ky, p_bitlen, 0);
    set_param_bn(&ptK.Z,Kz, p_bitlen, 0);

    pHandle -> busy        = TRUE; /* Prevent reentrancy */
    /* Call the q_lip function.*/
    status = q_ecp_pt_mul (QLIP_CONTEXT,
                            &ptK,   /* pointer to result point Q */
                            &ptQ,   /* pointer to Base point G */
                            &qd,    /* pointer to private key d */
                            &ec);  /* curve pointer */

    pHandle->busy = FALSE;

    /* de-allocate memory chunks allocated by q_malloc(),
    in the reverse order in which they are allocated */
    q_free(QLIP_CONTEXT, &ptK.Z);
    q_free(QLIP_CONTEXT, &ptQ.Z);

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_ecp_ecdsa_key_generate
 * Purpose : Generate an Elliptic Curve DSA key pair
 * Input   : pointers to the data parameters
 *                       type: Curve type, 0 - Prime field, 1 - binary field
 *                       p: prime p
 *                       p_bitlen: size of prime p in bits
 *                       a: Parameter a
 *                       b: Parameter b
 *                       n: The order of the Base point G (represented by "r"
 *                          in sample curves defined in FIPS 186-2 document)
 *                       Gx: x coordinate of Base point G
 *                       Gy: y coordinate of Base point G
 *                       d: private key, random integer from [1, n-1]
 *                       dvalid: 1 - manual input of private key pointed by d,
 *                               0 - generate private key internally.
 *                       Qx: x coordinate of public key Q
 *                       Qy: y coordinate of public key Q
 * Output  : Qx and Qy, x and y coordinates of point Q, public key (Q = dG)
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_key_generate (
                  crypto_lib_handle *pHandle,
                  u8_t type,
                  u8_t *p,      u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *d,
                  u32_t dvalid,
                  u8_t *Qx,
                  u8_t *Qy,
                  scapi_callback callback, void *arg)
{
    return(crypto_pka_ecp_diffie_hellman_generate(pHandle, type, p, p_bitlen,
                                                a, b,
                                                n, Gx, Gy, d, dvalid, Qx, Qy,
                                                callback, arg));
}

/*-------------------------------------------------------------------------------
 * Name    : crypto_pka_ecp_ecdsa_sign
 * Purpose : Perform an elliptic curve DSA signature
 * Input   : pointers to the data parameters
 *           hash: SHA1 hash of the message, length is 20 bytes
 *                       type: Curve type, 0 - Prime field, 1 - binary field
 *                       p: prime p
 *                       p_bitlen: size of prime p in bits
 *                       a: Parameter a
 *                       b: Parameter b
 *                       n: The order of the Base point G (represented by "r"
 *                          in sample curves defined in FIPS 186-2 document)
 *                       Gx: x coordinate of Base point G
 *                       Gy: y coordinate of Base point G
 *           k: random integer from [1, n-1]
 *           d: private key
 * Output  : rs: the signature, first part is r, followed by s, each is q length
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *------------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_sign (
                  crypto_lib_handle *pHandle,
                  u8_t *hash,
                  u32_t hashLength,
                  u8_t type,
                  u8_t *p,
                  u32_t p_bitlen,
                  u8_t *a,
                  u8_t *b,
                  u8_t *n,
                  u8_t *Gx,
                  u8_t *Gy,
                  u8_t *k,
                  u32_t kvalid,
                  u8_t *d,
                  u8_t *r,
                  u8_t *s,
                  scapi_callback   callback, void *arg)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	crypto_lib_handle *old_pHandle = pHandle;
	u32_t   p_len_adj;
	q_point_t  ptG;     /* q_point pointer to base point G */
	q_curve_t  ec;      /* q_point pointer to curve parameters */
	q_signature_t qlip_rs;   /* q_signature pointer to rs */
	q_lint_t qd;         /* q_lint pointer to d */
	q_lint_t qlip_hash;         /* q_lint poitner to hash */
	q_lint_t qk;         /* q_lint pointer to k */
	u8_t *Gz;
	int i;
	u32_t *rand = NULL;
	u8_t *tmp;
	s32_t len, it = 0;
	/* Workaround for qlip input parameter corruption */
	u8_t *Gx_c;
	/* End Workaround */

	LOCAL_QLIP_CONTEXT_DEFINITION;

	ARG_UNUSED(arg);
	if (!(pHandle && hash && p && a && b && n &&
	      Gx && Gy && k && d && r && s))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
	CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)

	/* Find parameter size according to the prime length */
	status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);
	if (status != BCM_SCAPI_STATUS_OK)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	if (!kvalid) {
		/* value in d is not valid, generate it internally */
		len = (p_bitlen + 7) >> 3;
		it = (len + DSA_SIZE_RANDOM -1)/DSA_SIZE_RANDOM;
		rand = q_malloc(QLIP_CONTEXT, it * DSA_SIZE_RANDOM >> 2);
		if(!rand)
			return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;

		status = crypto_rng_generate(old_pHandle, NULL, 0, (u8_t*)rand,
					     DSA_SIZE_RANDOM * it, NULL, 0);
		if (status != BCM_SCAPI_STATUS_OK)
			goto ECDSA_SIGN_POST_RAND;

		/* Mask extra bits if any so that the actual random number is of
		p_bitlen */
		tmp = (u8_t *)rand;
		if ((it * DSA_SIZE_RANDOM * 8) - p_bitlen) {
			memset(tmp+len, 0, (it * DSA_SIZE_RANDOM) - len);
			tmp[len-1] &= (1 << (8 - ((len * 8) - p_bitlen))) - 1;
		}

		status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODREM,
					n, p_bitlen,
					(u8_t *)rand, p_bitlen,
					NULL, 0,
					k, (u32_t *) &len,
					NULL, NULL);
		if (status != BCM_SCAPI_STATUS_OK)
			goto ECDSA_SIGN_POST_RAND;
	}

	/*
	* Mapping: one-to-one
	*/

	/* Set curve parameters */
	ec.type = type;
	set_param_bn(&ec.n, n,p_bitlen, 0);

	Gz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if(!Gz) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_SIGN_POST_RAND;
	}

	for (i = 0; i < (ec.n.alloc * 4); i++) {
		Gz[i] = 0;
	}

	((u32_t*)Gz)[0] = 0x00000001;

	/* Workaround for qlip input parameter corruption */
	Gx_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if(!Gx_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_SIGN_POST_GZ;
	}

	memcpy(Gx_c, Gx, ec.n.alloc * 4);

	set_param_bn(&ec.a, a,p_bitlen, 0);
	set_param_bn(&ec.b, b,p_bitlen, 0);
	set_param_bn(&ec.p, p,p_bitlen, 0);

	/* Set Base pointer G parameters */
	set_param_bn(&ptG.X,Gx_c, p_bitlen, 0);
	set_param_bn(&ptG.Y,Gy, p_bitlen, 0);
	set_param_bn(&ptG.Z,Gz, p_bitlen, 0);
	/* End workaround */

	/* Set private key */
	set_param_bn(&qd,d, p_bitlen, 0);
	/* Set hash, 20 bytes */
	set_param_bn(&qlip_hash,hash, BYTELEN2BITLEN(hashLength), hashLength);
	/* Set random number k */
	set_param_bn(&qk,k, p_bitlen, 0);

	/* Set rs, we need to break into two fragments. */
	set_param_bn(&qlip_rs.r,r,p_bitlen, 0);
	set_param_bn(&qlip_rs.s,s,p_bitlen, 0);

	pHandle->busy = TRUE; /* Prevent reentrancy */

	/* Call the qlip function. */
	status = q_ecp_ecdsa_sign(QLIP_CONTEXT,    /* QLIP context pointer */
				&qlip_rs,    /* q_signature pointer to rs */
				&ptG,        /* q_point poitner to G */
				&ec,         /* q_curve pointer to curve */
				&qd,         /* q_lint pointer to d */
				&qlip_hash,  /* q_lint pointer to hash */
				&qk);        /* q_lint pointer to k */

	pHandle->busy = FALSE;

	/* de-allocate memory chunks allocated by q_malloc(),
	in the reverse order in which they are allocated */
	q_free(QLIP_CONTEXT, &ptG.X);

	ECDSA_SIGN_POST_GZ:
	q_free(QLIP_CONTEXT, &ptG.Z);

	ECDSA_SIGN_POST_RAND:
	if (!kvalid) {
		set_param_bn(&ptG.Z,(u8_t *)rand,
			     BYTELEN2BITLEN(it * DSA_SIZE_RANDOM), 0);
		q_free(QLIP_CONTEXT, &ptG.Z);
	}

	return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_pka_ecp_ecdsa_verify
 * Purpose : Verify an Elliptic curve DSA signature
 * Input   : pointers to the data parameters
 *           hash: SHA1 hash of the message, length is hashLength bytes
 *                       type: Curve type, 0 - Prime field, 1 - binary field
 *                       p: prime p
 *                       p_bitlen: size of prime p in bits
 *                       a: Parameter a
 *                       b: Parameter b
 *                       n: The order of the Base point G (represented by "r"
 *                          in sample curves defined in FIPS 186-2 document)
 *                       Gx: x coordinate of Base point G
 *                       Gy: y coordinate of Base point G
 *                       Qx: x coordinate of Base point Q
 *                       Qy: y coordinate of Base point Q
 *                       Qz: y coordinate of Base point Q
 *           r: first part of signature
 *           s: second part of signature
 * Return  : v. If v == r then verify success
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_ecp_ecdsa_verify (
                    crypto_lib_handle *pHandle,
                    u8_t *hash,
                    u32_t hashLength,
                    u8_t type,
                    u8_t *p,
                    u32_t p_bitlen,
                    u8_t *a,
                    u8_t *b,
                    u8_t *n,
                    u8_t *Gx,
                    u8_t *Gy,
                    u8_t *Qx,
                    u8_t *Qy,
                    u8_t *Qz,
                    u8_t *r,
                    u8_t  *s,
                    /*u8_t *v,*/
                    scapi_callback callback, void *arg)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	u32_t   p_len_adj;
	q_point_t  ptG;     /* q_point pointer to base point G */
	q_point_t  ptQ;     /* q_point pointer to base point Q or Public key */
	q_curve_t  ec;      /* q_point pointer to curve parameters */

	q_lint_t qlip_v;         /* q_lint pointer to v */
	q_lint_t qlip_hash;         /* q_lint pointer to h */
	q_signature_t qlip_rs;   /* q_signature pointer to rs */
	u8_t *Gz;
	int i;
	/* Workaround for qlip input parameter corruption */
	u8_t *Gx_c, *Gy_c, *a_c, *Qx_c, *Qy_c, *Qz_c;
	/* End Workaround */
	//u32_t QV[(p_bitlen/8)];
	u32_t QV[(256/8)];
	u8_t *v = (u8_t *)QV;

	LOCAL_QLIP_CONTEXT_DEFINITION;

	ARG_UNUSED(arg);
	if (!(pHandle && p && a && b && n && Gx && Gy
			&& Qx && Qy && r && s && v))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
	CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH)

	/* Find parameter size according to the prime length */
	status = find_mod_size(BITLEN2BYTELEN(p_bitlen), &p_len_adj);
	if (status != BCM_SCAPI_STATUS_OK)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	/*
	* Mapping,r==r,s==s,h==hash,v==v
	*/

	/* Set curve parameters */
	ec.type = type;
	set_param_bn(&ec.n, n,p_bitlen, 0);

	Gz = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if(!Gz)
		return BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
	for (i = 0; i < (ec.n.alloc * 4); i++) {
		Gz[i] = 0;
	}

	((u32_t *)Gz)[0] = 0x00000001;

	/* Workaround for qlip input parameter corruption */
	Gx_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Gx_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_GZ;
	}
	Gy_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Gy_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_GX;
	}
	a_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!a_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_GY;
	}
	Qx_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Qx_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_AC;
	}
	Qy_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Gy_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_QX;
	}
	Qz_c = (u8_t *)q_malloc(QLIP_CONTEXT,ec.n.alloc);
	if (!Qz_c) {
		status = BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW;
		goto ECDSA_VERIFY_POST_QY;
	}

	memcpy(Gx_c, Gx, ec.n.alloc * 4);
	memcpy(Gy_c, Gy, ec.n.alloc * 4);
	memcpy(a_c, a, ec.n.alloc * 4);
	memcpy(Qx_c, Qx, ec.n.alloc * 4);
	memcpy(Qy_c, Qy, ec.n.alloc * 4);
	if (Qz != NULL) {
		secure_memcpy(Qz_c, Qz, ec.n.alloc * 4);
	} else {
		/* Qz is NULL i.e Q is affine */
		for (i = 0; i < (ec.n.alloc * 4); i++) {
			Qz_c[i] = 0;
		}
		((u32_t *)Qz_c)[0] = 0x00000001;
	}

	set_param_bn(&ec.a, a_c,p_bitlen, 0);
	set_param_bn(&ec.b, b,p_bitlen, 0);
	set_param_bn(&ec.p, p,p_bitlen, 0);

	/* Set Base pointer G parameters */
	set_param_bn(&ptG.X,Gx_c, p_bitlen, 0);
	set_param_bn(&ptG.Y,Gy_c, p_bitlen, 0);
	set_param_bn(&ptG.Z,Gz, p_bitlen, 0);

	/* Set public key Q parameters */
	set_param_bn(&ptQ.X,Qx_c, p_bitlen, 0);
	set_param_bn(&ptQ.Y,Qy_c, p_bitlen, 0);
	set_param_bn(&ptQ.Z,Qz_c, p_bitlen, 0);
	/* End Workaround */

	/* Set hash */
	/* set_param_bn(&qlip_hash,hash,
	 * BYTELEN2BITLEN(DSA_SIZE_HASH), DSA_SIZE_HASH);
	 */
	set_param_bn(&qlip_hash,hash, BYTELEN2BITLEN(hashLength), hashLength);
	/* Set signature */
	set_param_bn(&qlip_rs.r,r, p_bitlen, 0);
	set_param_bn(&qlip_rs.s,s, p_bitlen, 0);
	set_param_bn(&qlip_v,v, p_bitlen, 0);

	pHandle -> busy        = TRUE; /* Prevent reentrancy */

	/* Call the qlip function. */
	status = q_ecp_ecdsa_verify (QLIP_CONTEXT,  /* QLIP context pointer */
				&qlip_v,         /* q_lint pointer to v */
				&ptG,            /* q_lint pointer to G */
				&ec,             /* q_curve pointer to curve */
				&ptQ,            /* q_point pointer to Q */
				&qlip_hash,      /* q_lint pointer to hash */
				&qlip_rs);

	pHandle -> busy        = FALSE;

	secure_memclr(Gx_c, ec.n.alloc);
	secure_memclr(Gy_c, ec.n.alloc);
	secure_memclr(a_c,  ec.n.alloc);
	secure_memclr(Qx_c, ec.n.alloc);
	secure_memclr(Qy_c, ec.n.alloc);
	secure_memclr(Qz_c, ec.n.alloc);
	secure_memclr(Gz,   ec.n.alloc);

	/* de-allocate memory chunks allocated by q_malloc(),
	in the reverse order in which they are allocated */
	q_free(QLIP_CONTEXT, &ptQ.Z);
ECDSA_VERIFY_POST_QY:
	q_free(QLIP_CONTEXT, &ptQ.Y);
ECDSA_VERIFY_POST_QX:
	q_free(QLIP_CONTEXT, &ptQ.X);
ECDSA_VERIFY_POST_AC:
	q_free(QLIP_CONTEXT, &ec.a);
ECDSA_VERIFY_POST_GY:
	q_free(QLIP_CONTEXT, &ptG.Y);
ECDSA_VERIFY_POST_GX:
	q_free(QLIP_CONTEXT, &ptG.X);
ECDSA_VERIFY_POST_GZ:
	q_free(QLIP_CONTEXT, &ptG.Z);

	if(status != BCM_SCAPI_STATUS_OK)
		return status;
	if (!secure_memeql(v, r, (p_bitlen/8)))
		return(BCM_SCAPI_STATUS_ECDSA_VERIFY_FAIL);

	return BCM_SCAPI_STATUS_OK;
}

/*-----------------------------------------------
 *  Name:  crypto_pka_emsa_pkcs1_v15_encode_hash
 *  Inputs:  preamble    - DER preamble with OID of the hash type
 *           preambleLen - length of preamble
 *           hash        - Hash of message
 *           hashLen     - length of hash
 *  Outputs:  emLen - Length of encoded message (em) in bytes
 *            em    - Byte pointer to encoded message (em) u8_t aligned
 *  Description: Encoding Method for Signatures with Appendix.
 *    ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.pdf
 *    http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
 -----------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_emsa_pkcs1_v15_encode_hash(
                    const u8_t *preamble, unsigned preambleLen,
                    const u8_t *hash, unsigned hashLen,
                    u32_t emLen, u8_t *em
                    )
{
	u32_t i;
	u32_t psLen;                    /* pad string length (PS) */
	u16_t tLen  = preambleLen + hashLen;
	u8_t *bp = em;  /* Setup pointer buffers */

	/* 1. Generate Hash.  But... our caller already did that for us */

	/* 2. Generate T: Encode algorithm ID etc.into DER --- refer to step5 */

	/* 3. length check */
	if (emLen < tLen + 11)
		return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* 4. Generate PS, which is a number of 0xFF --- refer to step 5 */

	/* 5. Generate em = 0x00 || 0x01 || PS || 0x00 || T */
	*bp = 0x00;
	bp++;
	*bp = 0x01;
	bp++;

	for (i = 0, psLen = 0; i < ((int)emLen - (int)tLen - 3); i++) {
		*bp = EMSA_PKCS1_V15_PS_BYTE;
		bp++;
		psLen++;
	}

	if (psLen < EMSA_PKCS1_V15_PS_LENGTH)
		return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* Post the last flag u8_t */
	*bp = 0x00; bp++;

	/* Post T(h) data into em(T) buffer */
	secure_memcpy(bp, preamble, preambleLen);
	bp += preambleLen;

	secure_memcpy(bp, hash, hashLen);

	return BCM_SCAPI_STATUS_OK;
}

/*-----------------------------------------------------------------------------
 * Name:  crypto_pka_fips_rsa_key_regenerate
 * Purpose : regenerate RSA private key exponent and public modulus from p and q
 * Inputs: pointers to the data parameters
 *         modulus_bits  - input, requested modulus size
 *         e             - Byte pointer (e), manual input
 *         p             - Byte pointer (p), manual input
 *         q             - Byte pointer (q), manual input
 *
 *  Outputs:
 *    n         - Byte pointer (n, public modulus), generated output
 *    d         - Byte pointer (d, private exponent), generated output
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_pka_fips_rsa_key_regenerate (
				      crypto_lib_handle *pHandle,
				      u32_t    modulus_bits,
				      u8_t *  p,
				      u8_t *  q,
				      u8_t *  n,
				      u8_t *  d
					)
{
	/* Can't use globals directly, must these directly */

	u8_t *  One             = NULL;
	u8_t *  HugeModulus     = NULL;
	u32_t   OneBits         = 0;
	u32_t   HugeModulusBits = 0;
	BCM_SCAPI_STATUS  status          = BCM_SCAPI_STATUS_OK;
	u8_t e[4] = {0x01, 0x00, 0x01, 0x00};

	/* Locals within rsakeygen */
	u8_t   p1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};
	u8_t   q1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};
	u8_t   p1q1[RSA_MAX_PUBLIC_KEY_BYTES] = {0};

	u32_t    n_bits;
	u32_t    p_bits = modulus_bits/2;
	u32_t    q_bits = modulus_bits/2;
	u32_t    d_bits;
	u32_t    p1bits          = 0;
	u32_t    q1bits          = 0;
	u32_t    p1q1bits        = 0;
	u8_t     tmpe[64*4];

	q_lint_t  q_tmp;

	int i;

	LOCAL_QLIP_CONTEXT_DEFINITION;

	/*
	* Restriction: requested modulus size must not exceed
	* RSA_MAX_MODULUS_SIZE...
	*/
	secure_memclr(tmpe, 256);
	secure_memcpy(tmpe, e, 4);

	if(modulus_bits > RSA_MAX_MODULUS_SIZE)
		return BCM_SCAPI_STATUS_CRYPTO_KEY_SIZE_MISMATCH;

	/* Begin constants initialization */

	/* Initialize One */
	One = (u8_t*)q_malloc(QLIP_CONTEXT, (RSA_MAX_MATH_BYTES+4)/4);
	if(One == NULL)
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;

	secure_memclr(One, RSA_MAX_MATH_BYTES+4);

	One[0] = 0x01;
	OneBits = 1;

	/* Initialize HugeModulus */
	HugeModulus = (u8_t*)q_malloc(QLIP_CONTEXT, RSA_MAX_MOD_BYTES/4);
	if(HugeModulus == NULL)
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;

	for (i=0; i < RSA_MAX_MOD_BYTES; ++i) {
		HugeModulus[i] = 0xff;
	}

	HugeModulusBits = modulus_bits; /* RSA_MAX_MOD_BYTES << 3; */

	/* Compute n = p * q */
	status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODMUL,
					HugeModulus, HugeModulusBits,
					p, p_bits,
					q, q_bits,
					n, &n_bits, NULL, 0);

	n_bits = bcm_rsa_find_num_bits(n, n_bits);

	if (n_bits > modulus_bits) {
		status |= BCM_SCAPI_STATUS_CRYPTO_ERROR;
	}

	if (status) {
		goto cleanup_and_exit;
	}

	/******************************************
	* Compute d = e ** -1 mod ((p - 1)(q - 1))
	******************************************/
	status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODSUB,
					HugeModulus, HugeModulusBits,
					p, p_bits,
					One, OneBits,
					p1, &p1bits, NULL, 0);

	if(status) {
		goto cleanup_and_exit;
	}

	p1bits = bcm_rsa_find_num_bits(p1, p1bits);

	status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODSUB,
					HugeModulus, HugeModulusBits,
					q, q_bits,
					One, OneBits,
					q1, &q1bits, NULL, 0);

	if(status) {
		goto cleanup_and_exit;
	}

	q1bits = bcm_rsa_find_num_bits(q1, q1bits);

	status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODMUL,
					HugeModulus, HugeModulusBits,
					p1, p1bits,
					q1, q1bits,
					p1q1, &p1q1bits, NULL, 0);

	if(status)
		goto cleanup_and_exit;

	p1q1bits = bcm_rsa_find_num_bits(p1q1, p1q1bits);

	/* Manually provided public exponent e */
	status = crypto_pka_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODINV,
					p1q1, p1q1bits,
					tmpe, p1q1bits,
					NULL, 0,
					d, &d_bits, NULL, 0);

	d_bits = bcm_rsa_find_num_bits(d, d_bits);

cleanup_and_exit:

	/* clean up of p1, q1, p1q1 */
	secure_memclr(p1, RSA_MAX_PUBLIC_KEY_BYTES);
	p1bits = 0;

	secure_memclr(q1, RSA_MAX_PUBLIC_KEY_BYTES);
	q1bits = 0;

	secure_memclr(p1q1, RSA_MAX_PUBLIC_KEY_BYTES);
	p1q1bits = 0;

	if (HugeModulus) {
		secure_memclr(HugeModulus, RSA_MAX_MOD_BYTES);
		set_param_bn(&q_tmp, HugeModulus,
				BYTELEN2BITLEN(RSA_MAX_MOD_BYTES),0);
		q_free(QLIP_CONTEXT, &q_tmp);
		HugeModulus = NULL;
	}

	if (One) {
		secure_memclr(One, RSA_MAX_MATH_BYTES+4);
		set_param_bn(&q_tmp, One,
				BYTELEN2BITLEN((RSA_MAX_MATH_BYTES+4)),0);
		q_free(QLIP_CONTEXT, &q_tmp);
		One = NULL;
	}

	return status;
}

/**
 * This API
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_sign_hash(
                    crypto_lib_handle *pHandle, u32_t pqLen,
                    u8_t *pq,
                    u32_t hashLen, u8_t * hash,
                    u32_t *sigLen, u8_t *sig
                    )
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	u8_t n[256], d[256];
	u8_t EM[256];

	u8_t *p = pq;
	u8_t *q = pq + (pqLen/2);
	const u8_t *preamble = NULL;
	unsigned preambleLen = 0;

	*sigLen = 0;

	if (pqLen != 256) {
		status = BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (status == BCM_SCAPI_STATUS_OK) {
		status = crypto_pka_fips_rsa_key_regenerate(pHandle,
							pqLen*8, p, q, n, d);
	}

	if (status == BCM_SCAPI_STATUS_OK) {
		switch(hashLen) {
			case SHA1_HASH_SIZE:
				preamble = sha1DerPkcs1Array;
				preambleLen = SHA1_DER_PREAMBLE_LENGTH;
			break;
			case SHA256_HASH_SIZE:
				preamble = sha256DerPkcs1Array;
				preambleLen = SHA256_DER_PREAMBLE_LENGTH;
			break;
			default:
				status = BCM_SCAPI_STATUS_PARAMETER_INVALID;
		}
	}

	if (status == BCM_SCAPI_STATUS_OK) {
		status = crypto_pka_emsa_pkcs1_v15_encode_hash(
						preamble, preambleLen,
						hash, hashLen, pqLen, EM);
	}

	if (status == BCM_SCAPI_STATUS_OK) {
		secure_memrev(EM, EM, pqLen);

		status = crypto_pka_rsa_mod_exp(pHandle, EM,
					pqLen*8, n, pqLen*8, d,
					pqLen*8, sig, sigLen, NULL, 0);

		if (*sigLen != pqLen) {
			status = BCM_SCAPI_STATUS_RSA_PKCS1_LENGTH_NOT_MATCH;
		}
	}

	secure_memclr(EM, sizeof(EM));
	secure_memclr(n, sizeof(n));
	secure_memclr(d, sizeof(d));
	return status;
}

/**
 * This API computes the PKCS1_V15 encoding for the given Hash string
 * @param[in]
 * @param[out]
 * @return Status of the Operation
 * @retval
 */
BCM_SCAPI_STATUS crypto_pka_rsassa_pkcs1_v15_verify_hash(
                    crypto_lib_handle *pHandle,
                    u32_t nLen, u8_t *n,
                    u32_t eLen, u8_t *e,
                    u32_t HLen, u8_t *H,
                    u32_t SLen, u8_t *S,
                    BCM_HASH_ALG hashAlg
                    )
{
	u8_t EM[2048]; /* 512 bytes, supports upto 4096 bit keys */
	u8_t EM2[2048]; /* this is the EM' */
	u32_t modexp_len;
	BCM_SCAPI_STATUS status;

	if ( nLen > 512 ) {
		status = crypto_pka_mont_math_accelerate(pHandle,
					BCM_SCAPI_MATH_MODEXP,
					/* modulus */
					(u8_t *)n, BYTELEN2BITLEN(nLen),
					/* message, e.g. signature */
					(u8_t *)S, BYTELEN2BITLEN(SLen),
					/* exponent */
					(u8_t *)e, BYTELEN2BITLEN(eLen),
					(u8_t *)EM, /* output */ &modexp_len,
					NULL, 0);
	} else	{
		status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODEXP,
					/* modulus */
					(u8_t *)n, BYTELEN2BITLEN(nLen),
					/* message, e.g. signature */
					(u8_t *)S, BYTELEN2BITLEN(SLen),
					/* exponent */
					(u8_t *)e, BYTELEN2BITLEN(eLen),
					(u8_t *)EM, /* output */ &modexp_len,
					NULL, 0);
	}
	if (status != BCM_SCAPI_STATUS_OK)
		return status;

	/* Step2c: I2OSP(output) --- modexpOutput is the EM */
	bcm_int2bignum((u32_t *)EM, SLen); //bcm_swap_words

	/* Step3: EMSA-PKCS1-v1_5 encoding */
	status = emsa_pkcs1_v15_encode_hash(pHandle, HLen, H, SLen,
						EM2, hashAlg);
	if (status != BCM_SCAPI_STATUS_OK)
		return status;

	/* Step4: compare */
	if (!secure_memeql(EM, EM2, SLen)) {
		return BCM_SCAPI_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL;
	}
#ifdef  CRYPTO_DEBUG
	printf("Exit %s\n",__func__);
#endif
	return BCM_SCAPI_STATUS_OK;

}

/*---------------------------------------------------------------
 * Name    : YieldHandler
 * Purpose : Intermediate call for system yield Function
 * Input   : pointers to the data parameters
 * Return  : 0
 */
BCM_SCAPI_STATUS YieldHandler(void)
{
    return(BCM_SCAPI_STATUS_OK); /* Eventually will be the OS yield function. */
}

/* ****************************************************************************
 * Private Functions
 * ***************************************************************************/

////////////////////////////////////////////////////////////////////
// Function: find_mod_size
// Find the parameter size for a modulus operation
//
// Inputs:
// nLen = length of field, in bytes
// paramLen = length of parameter
////////////////////////////////////////////////////////////////////
static BCM_SCAPI_STATUS find_mod_size(u32_t inLen, u32_t * outLen)
{
    /* No need to find parameter size according to the prime length for 5880 */
    *outLen = inLen;

    return BCM_SCAPI_STATUS_OK;
}

/*-----------------------------------------------------------------------------
 * Name:  bcm_rsa_find_num_bits
 * Purpose : Find the MSB in a big number
 * Inputs: pointers to the data parameters
 *        sum : pointer to the big number
 *        sumLen: length of "sum" in bytes
 * Outputs:
 *        None
 *
 * Returns:
 *        MSB or number of valid bits in big number
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/

static u32_t bcm_rsa_find_num_bits(u8_t * sum, u32_t sumLen){
    u32_t sumBits = 0;
    int j,k;

    for(j=sumLen - 1; j >= 0; j--) {
        if(sum[j] != 0) {
            sumBits = sumLen * 8;
            for(k=7; k>=0; k--) {
                if (sum[j] && !(sum[j] & (0x1 << k))) {
                    sumBits -= 1;
                } else {
                    return sumBits;
                }
            }
            return sumBits;
        } else {
            sumLen--;
        }
    }
    return sumBits;
}

/*-----------------------------------------------------------------------------
 * Name:  bcm_primerng
 * Purpose : Generates a prime number in result buffer of size result_bits
 *
 * Inputs: pointers to the data parameters
 *         length          - Request length in bits of random prime
 *         OneBits         - Length pointer to (One) constant
 *         One             - Byte pointer to (One) constant
 *         HugeModulusBits - Length pointer to (HugeModulus) constant
 *         HugeModulus     - Byte pointer to (HugeModulus) constant
 *
 * Outputs:
 *         result          - Byte pointer to a random prime number
 *         result_bits     - Length pointer to a random prime number
 *
 *  Returns: BCM_SCAPI_STATUS_OK or error code
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/
static BCM_SCAPI_STATUS bcm_primerng(
               crypto_lib_handle *pHandle,
               u32_t              length,
               u32_t             *result_bits,
               u8_t               *result,

               /* Passing globals */
               u32_t             *OneBits,
               u32_t             *One,
               u32_t             *HugeModulusBits,
               u32_t             *HugeModulus,
               q_lip_ctx_t       *q_lip_ctx
           )
{
    BCM_SCAPI_STATUS status         = BCM_SCAPI_STATUS_OK;
    BCM_SCAPI_STATUS pstatus        = BCM_SCAPI_STATUS_OK;
    u32_t *prn             = NULL;
    u32_t  prnbits         = length; /* User-requested number of random bits */
    u32_t prnBytes         = BITLEN2BYTELEN(prnbits);
    /* Index to last LongKey element */
    int     ms_uint32         = (ROUNDUP_TO_32_BIT(length)/32) - 1;
    q_lint_t  q_tmp;
    //int i;

    /* Allocate memory for maximum sized RN */
    prn = q_malloc(q_lip_ctx, RSA_MAX_RNG_BYTES/4);
    if(prn == NULL){
        return(BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY);
    }

    memset(prn,0,RSA_MAX_RNG_BYTES);

    /* The RNG returns a RN no bigger than that requested.     */
    /* However, the returned size may be smaller than that requested */
    /* if the significant bit(s) = 0.                                */
    status = bcm_rsa_get_random(&prnbits, (u8_t *)prn);
    if(status != BCM_SCAPI_STATUS_OK){
        return(status);
    }

    bcm_swap_endian(prn, prnBytes, prn, TRUE);
    bcm_swap_words(prn, prnBytes);

    /* Guarantee RN is exact requested width and is odd */
    /* 2 msb of each p/q prime must be set to guarantee full modulus width */

    prn[ms_uint32] |= (0x00000001 << ((length-1) & 31));
    if ((length - 1) & 31)
      prn[ms_uint32] |=  0x1 << ((length-2) & 31);
    else
      prn[ms_uint32 - 1] |= 0x1 << ((length-2) & 31);

    prn[0] |= 0x00000001;

    prnbits = length;

    while(pstatus == 0)
    {
        if((bcm_test_prime_sp(pHandle, prnbits, prn) == BCM_SCAPI_STATUS_OK) &&
        ((pstatus=bcm_test_prime_rm(pHandle, RSA_NUM_WITNESS_TESTS, &prnbits,
                                    prn, OneBits, One,
                                    HugeModulusBits, HugeModulus,
                                    q_lip_ctx))
                == BCM_SCAPI_STATUS_CRYPTO_RN_PRIME))
        {
            /* Got prime RN or error condition, break out of while loop */
            break;
        }
        else
        {
            /* max_rn_increments attempts to adjust RN failed;
             * just get another RN */
            if((pstatus = bcm_rsa_get_random(&prnbits, (u8_t *)prn)) !=
                                             BCM_SCAPI_STATUS_OK)
            {
                return(pstatus);
            }

            bcm_swap_endian(prn, prnBytes, prn, TRUE);
            bcm_swap_words(prn, prnBytes);
            /* Guarantee RN is exact requested width and is odd */
            prn[ms_uint32] |= (0x00000001 << ((length-1) & 31));
            if ((length - 1) & 31)
              prn[ms_uint32] |=  0x1 << ((length-2) & 31);
            else
              prn[ms_uint32 - 1] |= 0x1 << ((length-2) & 31);
            prn[0] |= 0x00000001;
            prnbits = length;

            //i = 0; /* Work on this new RN another 999 times (max) */
        } /* RN adjust / new RN */
    } /* Until bcm_rsa_get_random failure or until prime RN found */

    memcpy(result, (u8_t *)prn, BITLEN2BYTELEN(prnbits));
    *result_bits = prnbits;

    if(prn != NULL) {
        memset(prn,0,RSA_MAX_RNG_BYTES);
        set_param_bn(&q_tmp, (u8_t *)prn, BYTELEN2BITLEN(RSA_MAX_RNG_BYTES),0);
        q_free(q_lip_ctx, &q_tmp);

        prn = NULL;
        prnbits = 0;
    }

    return status;
}

#ifdef USE_RAW_RANDOM_DATA_FOR_PRIME_RNG
/*-----------------------------------------------------------------------------
 * Name:  bcm_rsa_get_random_raw
 * Purpose : generates raw random number for prime number generation and check
 * Inputs: pointers to the data parameters
 *        bits : pointer to the size of random number required, in bits
 *
 * Outputs:
 *        array : pointer to the random number generated, of length "bits"
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/

static BCM_SCAPI_STATUS bcm_rsa_get_random_raw(u32_t *bits, u8_t *array)
{
    u8_t result[RNG_MAX_NUM_WORDS*sizeof(u32_t)];
    u32_t remaining, bytes, offset;
    BCM_SCAPI_STATUS rv;
    fips_rng_context rngctx;
    u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};

    crypto_init((crypto_lib_handle *)&cryptoHandle);

    rv = crypto_rng_init((crypto_lib_handle *)&cryptoHandle,
                         &rngctx, 1, NULL, 0, NULL, 0, NULL, 0);
    if (BCM_SCAPI_STATUS_OK != rv) {
        SYS_LOG_ERR("Couldn't initialise rng\n");
        return rv;
    }

    offset = 0;
    remaining = (*bits + 7) / 8;
    while (remaining) {
      crypto_rng_raw_generate((crypto_lib_handle *)&cryptoHandle, result,
                              RNG_MAX_NUM_WORDS, NULL, NULL);
      bytes = remaining > sizeof(result) ? sizeof(result) : remaining;
      memcpy(array + offset, result, bytes);
      remaining -= bytes;
      offset += bytes;
    }

    return BCM_SCAPI_STATUS_OK;
}
#endif

/*-----------------------------------------------------------------------------
 * Name:  bcm_rsa_get_random
 * Purpose : generates random number for RSA key generate function
 * Inputs: pointers to the data parameters
 *        bits : pointer to the size of random number required, in bits
 *
 * Outputs:
 *        array : pointer to the random number generated, of length "bits"
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/

static BCM_SCAPI_STATUS bcm_rsa_get_random(u32_t *bits, u8_t *array)
{
#ifdef USE_RAW_RANDOM_DATA_FOR_PRIME_RNG
    return bcm_rsa_get_random_raw(bits, array);
#else
    u32_t k;
    u32_t m, lastBlockSize;
    BCM_SCAPI_STATUS status;

    k = BITLEN2BYTELEN(*bits);
    if (! k){
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    }

    m = 1;
    lastBlockSize = k;

    while (lastBlockSize > RSA_NONCE_SIZE){
        lastBlockSize -= RSA_NONCE_SIZE;
        m+=1;
    }

    //ENTER_CRITICAL();
    status = bcm_rsa_get_random_blocks(m, lastBlockSize, array);
    //EXIT_CRITICAL();

    return status;
#endif
}

/*-----------------------------------------------------------------------------
 * Name:  bcm_rsa_get_random_blocks
 * Purpose : generates random number for RSA key generate function, in blocks,
 *           FIPS 186-2 3.1/3.2
 * Inputs: pointers to the data parameters
 *         m = # of 160b blocks to generate
 *         lastBlockSize = size in bytes of last block
 *
 * Outputs:
 *        rng = u8_t pointer to 160b random
 *
 *  Returns:
 *          BCM_STAUTS_OK or error code
 *
 * Remarks: All input and output parameters in BIGNUM format
 *---------------------------------------------------------------------------*/
#ifndef USE_RAW_RANDOM_DATA_FOR_PRIME_RNG
static BCM_SCAPI_STATUS bcm_rsa_get_random_blocks(u32_t m, u32_t lastBlockSize,
                                     u8_t * rng)
{

    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;

    /* Temporary variables */
    u32_t i;
    u8_t inst_EntropyInput[32] =    { 0x29,0xe1,0xa4,0x27, 0x09,0xb7,0xe8,0x4d,
		    	    	      0xbe,0x50,0x78,0x8f, 0xba,0xd8,0xcb,0x60,
                                      0x9c,0x12,0x7e,0xec, 0x32,0x62,0x63,0x6a,
                                      0x51,0x3f,0xd9,0x05, 0x9f,0xb8,0xba,0xe4};

    u8_t inst_Nonce[16] =           { 0xf3,0xa5,0x21,0xf2,
		    	              0x8d,0xff,0xbd,0x97,
		    	              0x57,0x4c,0x40,0x5b,
		    	              0x69,0xb6,0x36,0xad};

    u8_t inst_PersonalStr[32] =     { 0xc9,0x9a,0x11,0x47, 0xd8,0xdb,0x40,0x1f,
		    	    	      0x4f,0xcf,0x76,0x38, 0x67,0x29,0x67,0x58,
                                      0xe2,0x64,0x04,0xa3, 0x0a,0x4a,0x9f,0xa4,
                                      0x96,0xa7,0x17,0xf2, 0x1f,0x5d,0x74,0x9b};
    u32_t *tmp;

    fips_rng_context  rngctx;
    u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};

    crypto_init((crypto_lib_handle *)&cryptoHandle);

    /* 3rd parameter, rng selftest included */
    status = crypto_rng_init((crypto_lib_handle *)&cryptoHandle, &rngctx, 1,
			(u32_t*)inst_EntropyInput, sizeof(inst_EntropyInput),
			(u32_t*)inst_Nonce, sizeof(inst_Nonce),
			inst_PersonalStr, sizeof(inst_PersonalStr));
    if (status != BCM_SCAPI_STATUS_OK)
       return status;

    status = crypto_rng_generate((crypto_lib_handle *)&cryptoHandle, NULL, 0,
		                 rng, WORDLEN2BYTELEN(5) * (m - 1) + lastBlockSize,
		                 NULL, 0);

    bcm_swap_words((unsigned int *)(rng), 20);
    if (status != BCM_SCAPI_STATUS_OK)
      return status;

    tmp = (u32_t *)rng;
    for (i = 0; i < m; i++)
    {
        if (i < m-1)
        {
            bcm_swap_endian(tmp, RSA_NONCE_SIZE, tmp, TRUE);
            bcm_swap_words(tmp, RSA_NONCE_SIZE);

            tmp += (RSA_NONCE_SIZE/sizeof(u32_t));
        }
        else
        {
            bcm_swap_endian(tmp, lastBlockSize, tmp, TRUE);
            bcm_swap_words(tmp, lastBlockSize);
        }
    }

    return status;
}
#endif

/*-----------------------------------------------------------------------------
* Name:  bcm_test_prime_rm
* Purpose : To test whether a given number is Prime,
*             Rabin-Miller primality test
*
* Inputs: pointers to the data parameters
*         p      - Byte pointer to (p)
*         p_bits - Length pointer to p_bits, size of p
*
* Outputs:
*        None
*
*  Returns: BCM_SCAPI_STATUS_CRYPTO_RN_PRIME   =>  p is probably prime
*           BCM_SCAPI_STATUS_CRYPTO_RN_NOT_PRIME   =>  p is not prime
*           All other error codes  =>  error occurred
*
* Remarks: All input and output parameters in BIGNUM format
*---------------------------------------------------------------------------*/

static BCM_SCAPI_STATUS bcm_test_prime_rm(
         crypto_lib_handle *pHandle,
         int                  num_witnesses,
         u32_t             *p_bits,
         u32_t             *p,

         /* Passing globals */
         u32_t             *OneBits,
         u32_t             *One,
         u32_t             *HugeModulusBits,
         u32_t             *HugeModulus,
         q_lip_ctx_t       *q_lip_ctx
     )
{
    int i;
    u32_t b, j;
    q_lint_t  q_tmp;
    q_lint_t  q_m;

    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;

    u32_t *a            = NULL;
    u32_t *m            = NULL;
    u32_t *p1           = NULL;
    u32_t *z            = NULL;
    //u32_t *tmp          = NULL;

    u32_t  abits        = 0;
    u32_t  mbits        = 0;
    u32_t  p1bits       = 0;
    u32_t  zbits        = 0;

    a = (u32_t *) q_malloc(q_lip_ctx, RSA_MAX_RNG_BYTES/4);
    if(a == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(a,0,RSA_MAX_RNG_BYTES);
    abits = *p_bits;

    m = (u32_t *) q_malloc(q_lip_ctx, RSA_MAX_RNG_BYTES/4);
    if(m == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(m,0,RSA_MAX_RNG_BYTES);
    mbits = *p_bits;

    p1 = (u32_t *) q_malloc(q_lip_ctx, (RSA_MAX_MATH_BYTES + 4)/4);
    if(p1 == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(p1,0,RSA_MAX_MATH_BYTES + 4);
    p1bits = *p_bits;


    z = (u32_t *) q_malloc(q_lip_ctx, (RSA_MAX_MATH_BYTES + 4)/4);
    if(z == NULL){
        return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
    }

    memset(z,0,RSA_MAX_MATH_BYTES + 4);
    zbits = *p_bits;

    /* Compute b such that 2 ** b is the largest power of 2 that divides p-1 */
    /* Compute m such that p = 1 + 2 ** b * m */
    status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
    BCM_SCAPI_MATH_MODSUB,
    (u8_t *)HugeModulus, *HugeModulusBits,
    (u8_t *)p, *p_bits,
    (u8_t *)One, *OneBits, (u8_t *)m, &mbits,
    NULL, NULL);

    if(status != BCM_SCAPI_STATUS_OK){
        return(status);
    }
    //mbits = BYTELEN2BITLEN(mbits);
    mbits = bcm_rsa_find_num_bits((u8_t *) m, mbits);

    /* Copy m=(p-1) to p1 */
    for (i=0; i < (BITLEN2BYTELEN(*p_bits)/4); i++){
        p1[i] = m[i];
    }

    p1bits = mbits;

    /* Divide m by 2 until m is odd; b = # of divisions required */
    b = 0;
    set_param_bn(&q_m, (u8_t *)m,mbits, 0);
    /* Srini : m to be converted to BIGNUM format??? */
    do {
        q_div_2pn (&q_m, &q_m, 1); /* Right shift m by one bit */
        b++;
    } while ((m[0] & 0x00000001) == 0);

    /* At this point, m * 2**b = p-1 */
    /* From table C3 in
     * https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf
     */
    num_witnesses = (*p_bits > 1024) ? 3 : ((*p_bits > 512) ? 4 : 5);
    for(i = 0; (i < num_witnesses) && (status == 0); i++)
    {
        /* Choose a random witness "a", 1 < a < p  */
        /* "a" must be < p */

        /* WARNING: Must zero out answer, as accelerate writes
         * only significant bits */
        memset(a,0,RSA_MAX_RNG_BYTES);

        abits = *p_bits - 1;
        status = bcm_rsa_get_random(&abits, (u8_t *)a);
        if(status != BCM_SCAPI_STATUS_OK){
            return(status);
        }

        /* WARNING: Must zero out answer, as accelerate writes
         *  only significant bits */
        memset(z,0,RSA_MAX_MATH_BYTES + 4);

        status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
        BCM_SCAPI_MATH_MODEXP,
        (u8_t *)p, *p_bits,
        (u8_t *)a, abits,
        (u8_t *)m, mbits, (u8_t *)z, &zbits,
        NULL, NULL);

        if(status != BCM_SCAPI_STATUS_OK) return(status);

        //zbits = BYTELEN2BITLEN(zbits);
        zbits = bcm_rsa_find_num_bits((u8_t *) z, zbits);

        set_param_bn(&q_m, (u8_t *)z, zbits, 0);
        set_param_bn(&q_tmp, (u8_t *)One, *OneBits, 0);

        if (q_ucmp (&q_m, &q_tmp) == 0) {
            continue;  /* p passes (a is a non-witness) and may be prime.
            Next witness. */
        }

        set_param_bn(&q_m, (u8_t *)z, zbits, 0);
        set_param_bn(&q_tmp, (u8_t *)p1, p1bits, 0);

        if (q_ucmp (&q_m, &q_tmp) == 0) {
            /* z == -1 (mod p), 'p' is probably prime. Next Witness */
            continue;
        }

        j = b;
        int next_witness = 0;
        while (--j)
        {
            /* WARNING: Must zero out answer, as accelerate writes
             * only significant bits */
            /* op: z = z ** 2 mod p (c = a ** b mod m) */
            status = crypto_pka_math_accelerate((crypto_lib_handle *)pHandle,
            BCM_SCAPI_MATH_MODMUL,
            (u8_t *)p, *p_bits,
            (u8_t *)z, zbits,
            (u8_t *)z, zbits,
            (u8_t *)z, &zbits,
            NULL, NULL);

            if(status != BCM_SCAPI_STATUS_OK) return(status);
            //zbits = BYTELEN2BITLEN(zbits);
            zbits = bcm_rsa_find_num_bits((u8_t *) z, zbits);

            set_param_bn(&q_m, (u8_t *)z, zbits, 0);
            set_param_bn(&q_tmp, (u8_t *)One, *OneBits, 0);

            if (q_ucmp (&q_m, &q_tmp) == 0) {
                /* 'p' is composite, otherwise a previous 'a' would
                 * have been == -1 (mod 'p') */
                status = BCM_SCAPI_STATUS_CRYPTO_RN_NOT_PRIME;
                goto finish_cleanup;
            }

            set_param_bn(&q_tmp, (u8_t *)p1, p1bits, 0);

            if (q_ucmp (&q_m, &q_tmp) == 0) {
                /* z == -1 (mod p), 'p' is probably prime. Next Witness */
                next_witness = 1;
                break;
            }
        }

        if (next_witness) {
            continue;
        }
        /*
         * If we get here, 'z' is the (p-1)/2-th power of the original 'z', and
         * it is neither -1 nor +1 -- so 'p' cannot be prime
         */
        status = BCM_SCAPI_STATUS_CRYPTO_RN_NOT_PRIME;
        goto finish_cleanup;
    } /* For each of num_witnesses witness tests */

    /* No witnesses found to show 'p' as composite */
    status = BCM_SCAPI_STATUS_CRYPTO_RN_PRIME;

finish_cleanup:

    q_tmp.limb = z;
    q_tmp.alloc = (RSA_MAX_MATH_BYTES + 4) / 4;
    memset(z,0,(RSA_MAX_MATH_BYTES + 4));
    q_free(q_lip_ctx, &q_tmp);
    z = NULL;
    zbits = 0;

    q_tmp.limb = p1;
    q_tmp.alloc = (RSA_MAX_MATH_BYTES + 4) / 4;
    memset(p1,0,(RSA_MAX_MATH_BYTES + 4));
    q_free(q_lip_ctx, &q_tmp);
    p1 = NULL;
    p1bits = 0;

    q_tmp.limb = m;
    q_tmp.alloc = RSA_MAX_RNG_BYTES / 4;
    memset(m,0,RSA_MAX_RNG_BYTES);
    q_free(q_lip_ctx, &q_tmp);
    m = NULL;
    mbits = 0;

    q_tmp.limb = a;
    q_tmp.alloc = RSA_MAX_RNG_BYTES / 4;
    memset(a,0,RSA_MAX_RNG_BYTES);
    q_free(q_lip_ctx, &q_tmp);
    a = NULL;
    abits = 0;

    return(status);
}


/*-----------------------------------------------------------------------------
* Name:  bcm_test_prime_sp
* Purpose : To test whether a given prime number is divisible by small primes
*
* Inputs: pointers to the data parameters
*         p      - Byte pointer to (p)
*         p_bits - Length pointer to p_bits, size of p
*
* Outputs:
*        None
*
*  Returns: BCM_SCAPI_STATUS_OK or error code
*
* Remarks: All input and output parameters in BIGNUM format
*---------------------------------------------------------------------------*/
static BCM_SCAPI_STATUS bcm_test_prime_sp(crypto_lib_handle *pHandle,
                                   u32_t p_bits,
                                   u32_t *p)
{
    return bcm_math_ppsel((crypto_lib_handle *)pHandle, (u8_t *)p,
                          p_bits, RSA_RN_INCREMENT * p_bits,0,0);
}

/*---------------------------------------------------------------------------
 * Name    : bcm_math_ppsel
 * Purpose : given a random number determine if it is
 *          divisible by small prime numbers
 * Input   : pointers to the data parameters
 * Output  : result: success if it is a prime number
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *           There are some HW limitation of the MODREM
 *           (something like modulus MSB must be 1),
 *           Caller can call rsa crt function instead.
 *---------------------------------------------------------------------------*/
static BCM_SCAPI_STATUS bcm_math_ppsel (
                    crypto_lib_handle *pHandle,
                    u8_t *paramA, u32_t  paramA_bitlen,
                    u32_t iter,
                    scapi_callback callback, void *arg)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    q_lint_t qlip_paramA;         /* q_lint pointer to parameters */
    LOCAL_QLIP_CONTEXT_DEFINITION;

    pHandle = &(pHandle[CRYPTO_INDEX_PKEMATH]);
    CHECK_DEVICE_BUSY(CRYPTO_INDEX_PKEMATH);

    /* Set A */
    set_param_bn(&qlip_paramA,paramA, paramA_bitlen, 0);

    status =  q_ppsel (QLIP_CONTEXT, &qlip_paramA, 2, iter);
    if(status!=BCM_SCAPI_STATUS_OK){
        return status;
    }
    /* Command is completed. */
    pHandle -> busy        = FALSE;

    if (callback == NULL)
    {
    }
    else
    {
        pHandle -> result      = paramA;
        pHandle -> result_len  = BITLEN2BYTELEN(paramA_bitlen);
        pHandle -> presult_len = &(pHandle -> result_len);
        pHandle -> callback    = callback;
        pHandle -> arg         = arg;
        (callback) (status, pHandle, pHandle->arg);
        /* assign context for use in check_completion */
    }

    return status;
}

/*------------------------------------------------------------------------------
 * Name    : bcm_dsa_genk
 * Purpose : Generate a valid k/k_inv
 * Input   : pointers to the data parameters
 *           q: 256bit prime
 *           k: 256bit number
 *           k_inv: 256 bit number
 * Output  : k and k_inv
 * Return  : Appropriate status
 * Remark  :
 *----------------------------------------------------------------------------*/
static BCM_SCAPI_STATUS bcm_dsa_genk(crypto_lib_handle *pHandle,
				     u8_t  *q,        u32_t q_bitlen,
				     u8_t  *k,
				     u32_t *k_bytelen,
				     u8_t  *k_inv,
				     u32_t *k_inv_bytelen)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	u8_t  k_plus2[DSA_SIZE_RANDOM_2048];

	/* verify correct bitlen */
	if (q_bitlen != 256)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	*k_bytelen = q_bitlen / 8;

	/* Note k is BE in the loop and thru the k = c + 1, then
	 * switched to the normal LE format */
	do {
		/* obtain N bits of RNG */
		status = crypto_rng_generate(pHandle, NULL, 0,
					     k, *k_bytelen,
					     NULL, 0);
		if (status != BCM_SCAPI_STATUS_OK)
			return status;

		memset(k_plus2, 0x0, *k_bytelen);
		bcm_soft_add_byte2byte_plusX(k_plus2, DSA_SIZE_RANDOM_2048, k, DSA_SIZE_RANDOM_2048, 2);

		/* loop while (c > q - 2) i.e. c + 2 > q */
	}while (memcmp(k_plus2, q, DSA_SIZE_RANDOM_2048) > 0);

	/* k = c + 1 */
	bcm_soft_add_byte2byte_plusX(k, DSA_SIZE_RANDOM_2048, NULL, 0, 1);
	secure_memrev(k, k, q_bitlen / 8);

	/* generate k_inv */
	status = crypto_pka_math_accelerate(pHandle, BCM_SCAPI_MATH_MODINV,
					q, q_bitlen,
					k, q_bitlen,
					NULL, 0,
					k_inv, k_inv_bytelen,
					NULL, 0);

	return status;
}

/*-----------------------------------------------
 *  Name:  emsa_pkcs1_v15_encode_hash
 *  Inputs: signType  - Bool type of sign encoding
 *      hLen      - Length of hash to encode in bytes
 *      h         - Byte pointer to hash
 *  Outputs:    emLen - Length of encoded message (em) in bytes
 *      em    - Byte pointer to encoded message (em) u8_t aligned
 *  Description: Encoding Method for Signatures with Appendix.
 *      ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.pdf
 *      http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
 *      or see RFC#3447
-----------------------------------------------------*/
static BCM_SCAPI_STATUS emsa_pkcs1_v15_encode_hash(crypto_lib_handle *pHandle,
		u32_t hLen, u8_t *h, u32_t emLen,
		u8_t *em, BCM_HASH_ALG hashAlg)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	u32_t i;
	u32_t psLen;                    /* pad string length (PS) */
	uint16_t tLen  = SHA256_DER_PREAMBLE_LENGTH + SHA256_HASH_SIZE;
	u8_t *bp = em;  /* Setup pointer buffers */

	ARG_UNUSED(pHandle);
	ARG_UNUSED(hLen);

	/* 3. length check */
	if (emLen < tLen + 11)
	return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* 4. Generate PS, which is a number of 0xFF --- refer to step 5 */

	/* 5. Generate em = 0x00 || 0x01 || PS || 0x00 || T */
	*bp = 0x00; bp++;
	*bp = 0x01; bp++;


	for (i = 0, psLen = 0; i < ((int)emLen - (int)tLen - 3); i++) {
		*bp = EMSA_PKCS1_V15_PS_BYTE;
		bp++;
		psLen++;
	}

	if (psLen < EMSA_PKCS1_V15_PS_LENGTH)
		return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* Post the last flag u8_t */
	*bp = 0x00; bp++;

	/* Post T(h) data into em(T) buffer */

	switch (hashAlg)
	{
		case BCM_HASH_ALG_SHA1:
		{
			for (i = 0; i < SHA1_DER_PREAMBLE_LENGTH; i++) {
				*bp = sha1DerPkcs1Array[i];
				bp++;
			}
			for (i = 0; i < SHA1_HASH_SIZE; i++) {
				*bp = h[i];
				bp++;
			}
		}
		break;

		case BCM_HASH_ALG_SHA2_256:
		{
			for (i = 0; i < SHA256_DER_PREAMBLE_LENGTH; i++) {
				*bp = sha256DerPkcs1Array[i];
				bp++;
			}
			for (i = 0; i < SHA256_HASH_SIZE; i++) {
				*bp = h[i];
				bp++;
			}
		}
		break;

		default:
			return BCM_SCAPI_STATUS_RSA_INVALID_HASHALG;
	}
	return status;
}

/*-----------------------------------------------
 *  Name:  emsa_pkcs1_v15_encode
 *  Inputs: signType  - Bool type of sign encoding
 *      mLen      - Length of message to encode (m) in bytes
 *      M         - Byte pointer to message (n) u8_t aligned, in little endian
 *  Outputs:    emLen - Length of encoded message (em) in bytes
 *      em    - Byte pointer to encoded message (em) u8_t aligned
 *  Description: Encoding Method for Signatures with Appendix.
 *      ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.pdf
 *      http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
 *      or see RFC#3447
-----------------------------------------------------*/
static BCM_SCAPI_STATUS emsa_pkcs1_v15_encode(crypto_lib_handle *pHandle,
				u32_t mLen, u8_t *M, u32_t emLen,
				u8_t *em, BCM_HASH_ALG hashAlg)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
	/* The buffer size should be 64 byte and in chunks of 64 bytes due to
	 * the data cache lines */
	u8_t outputSha[SHA256_HASH_SIZE*2] __attribute__((aligned(64)));
	u32_t i;
	u32_t psLen;                    /* pad string length (PS) */
	uint16_t tLen  = SHA256_DER_PREAMBLE_LENGTH + SHA256_HASH_SIZE;
	u8_t *bp = em;  /* Setup pointer buffers */

	/* 1. Generate Hash */
	/* M in little endian, so we swap */

	switch (hashAlg)
	{
		case BCM_HASH_ALG_SHA1:
		{
			tLen  = SHA1_DER_PREAMBLE_LENGTH + SHA1_HASH_SIZE;
			status = crypto_symmetric_hmac_sha1(pHandle, M,
							mLen, NULL, 0,
							outputSha, TRUE);
		}
		break;

		case BCM_HASH_ALG_SHA2_256:
		{
			tLen  = SHA256_DER_PREAMBLE_LENGTH + SHA256_HASH_SIZE;
			status = crypto_symmetric_hmac_sha256(pHandle, M,
							mLen, NULL, 0,
							outputSha, TRUE);
		}
		break;

		default:
			return BCM_SCAPI_STATUS_RSA_INVALID_HASHALG;
	}


	if (status != BCM_SCAPI_STATUS_OK) return status;

	/* 2. Generate T: Encode algorithm ID etc. into DER ---
	 * refer to step 5
	 */

	/* 3. length check */
	if (emLen < tLen + 11)
		return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* 4. Generate PS, which is a number of 0xFF --- refer to step 5 */

	/* 5. Generate em = 0x00 || 0x01 || PS || 0x00 || T */
	*bp = 0x00; bp++;
	*bp = 0x01; bp++;


	for (i = 0, psLen = 0; i < ((int)emLen - (int)tLen - 3); i++) {
		*bp = EMSA_PKCS1_V15_PS_BYTE;
		bp++;
		psLen++;
	}

	if (psLen < EMSA_PKCS1_V15_PS_LENGTH)
		return BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* Post the last flag u8_t */
	*bp = 0x00; bp++;

	/* Post T(h) data into em(T) buffer */

	switch (hashAlg)
	{
		case BCM_HASH_ALG_SHA1:
		{
			for (i = 0; i < SHA1_DER_PREAMBLE_LENGTH; i++) {
				*bp = sha1DerPkcs1Array[i];
				bp++;
			}

			for (i = 0; i < SHA1_HASH_SIZE; i++) {
				*bp = outputSha[i];
				bp++;
			}
		}
		break;

		case BCM_HASH_ALG_SHA2_256:
		{
			for (i = 0; i < SHA256_DER_PREAMBLE_LENGTH; i++) {
				*bp = sha256DerPkcs1Array[i];
				bp++;
			}
			for (i = 0; i < SHA256_HASH_SIZE; i++) {
				*bp = outputSha[i];
				bp++;
			}
		}
		break;

		default:
			return BCM_SCAPI_STATUS_RSA_INVALID_HASHALG;
	}
	return status;
}
