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

/* @file crypto_rng.c
 *
 * This file contains the random number generator API's
 *
 *
 */

/*
 * TODO
 *
 * */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <misc/printk.h>
#include <zephyr/types.h>
#include <crypto/crypto_rng.h>
#include <crypto/crypto.h>
#include <pka/crypto_pka.h>
#include <pka/crypto_pka_util.h>
#include <crypto/crypto_symmetric.h>

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
static void crypto_rng_readout(u8_t *result, u32_t num_words);
static BCM_SCAPI_STATUS bcm_rng_fips_hash_step(crypto_lib_handle *pHandle,
                   u8_t *input, u32_t input_len,
                   u8_t *output, u32_t output_len);
/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/

/*---------------------------------------------------------------
 * Name    : crypto_rng_init
 * Purpose : init the fips rng
 * 	     The algorithm is taken from the NIST SP 800-90A document Revision 1
 * 	     The instantiation psuedocode is present in appendix B.1.1
 * Input   : pHandle:               crypto handle
 *           rngctx:                user allocated memory, will be used as rng context.
 *           prediction resistance: reseed every request
 *           entropy_input:         32 bytes of entropy if testing
 *           entropy_length:        Entropy length
 *           nonce:                 16 bytes of nonce if desired/testing
 *           nonce_length:          Nonce length
 *           personal_str:          optional personalization string (0-64 bytes)
 *           personal_str_len:      len of personal_str in bytes
 * Return  : Appropriate status
 * Remark  :
 *           Security Strength:      256 bits (32 bytes)
 *           Entropy Input:          256 bits (32 bytes)
 *           Nonce:                  128 bits (16 bytes)
 *           Personalization String: 0-256 bits (0-32 bytes)
 *           Additional Input        0-256 bits (0-32 bytes)
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_rng_init(crypto_lib_handle *pHandle,
                   fips_rng_context *rngctx,
                   u8_t prediction_resist,
                   u32_t *entropy_input, u8_t entropy_length,
                   u32_t *nonce_input, u8_t nonce_length,
                   u8_t *personal_str, u8_t personal_str_len)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;

    u8_t hash_input[RNG_FIPS_TEMP_SIZE];
    u8_t *hash_ptr = (u8_t*)hash_input;
    u8_t hash_input_len;

    u32_t entropy[RNG_FIPS_ENTROPY_SIZE/4];
    u32_t nonce[RNG_FIPS_NONCE_SIZE/4];

    u8_t v[RNG_FIPS_SEEDLEN];
    u8_t c[RNG_FIPS_SEEDLEN];

    if (!pHandle || !rngctx)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (entropy_length > RNG_FIPS_ENTROPY_SIZE)
	    return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (nonce_length > RNG_FIPS_NONCE_SIZE)
	    return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (personal_str_len > RNG_FIPS_MAX_PERSONAL_STRING)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    secure_memclr((void *)rngctx, sizeof(fips_rng_context));

    /* Store the RNG context */
    pHandle->rngctx = rngctx;

    /* enable the rng core */
    *RNG_CTL_REG |= RNG_CTL_EN;

    /* generate entropy if not testing */
    if (entropy_input) {
        /* for testing */
        secure_memcpy((u8_t*)entropy, (u8_t*)entropy_input, RNG_FIPS_ENTROPY_SIZE);
    } else {
        status = crypto_rng_raw_generate(pHandle, (u8_t*)entropy, RNG_FIPS_ENTROPY_SIZE/4, NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_INIT_RETURN;
    }

    /* generate the nonce if not testing */
    if (nonce_input) {
        secure_memcpy((u8_t*)nonce, (u8_t*)nonce_input, RNG_FIPS_NONCE_SIZE);
    } else {
        status = crypto_rng_raw_generate(pHandle, (u8_t*)nonce, RNG_FIPS_NONCE_SIZE/4, NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_INIT_RETURN;
    }

    /* Uncomment this to match NIST pseudocode */
    /*  nonce = nonce + 1 */
    /* status = bcm_soft_add_byte2byte_plusX((u8_t*)nonce, RNG_FIPS_NONCE_SIZE, NULL, 0, 1); */
    /* if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_INIT_RETURN; */

    /* 7. seed_material = entropy_input || instantiation_nonce ||
     * 			  personalization_string.
     */
    secure_memcpy(hash_ptr, (u8_t*)entropy, RNG_FIPS_ENTROPY_SIZE);
    hash_input_len = RNG_FIPS_ENTROPY_SIZE;
    secure_memcpy(hash_ptr + hash_input_len, (u8_t*)nonce, RNG_FIPS_NONCE_SIZE);
    hash_input_len += RNG_FIPS_NONCE_SIZE;
    if (personal_str) {
        secure_memcpy(hash_ptr + hash_input_len, personal_str, personal_str_len);
        hash_input_len += personal_str_len;
    }

    /* 8. V = Hash_df(entropy || nonce || per_string, seedlen) */
    status = bcm_rng_fips_hash_step(pHandle, hash_input, hash_input_len, v, RNG_FIPS_SEEDLEN);
    if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_INIT_RETURN;

    /* 10. C = Hash_df((0x00 || V), seedlen) */
    hash_ptr[0] = 0x00;
    hash_input_len = 1;
    secure_memcpy(hash_ptr + hash_input_len, v, RNG_FIPS_SEEDLEN);
    hash_input_len += RNG_FIPS_SEEDLEN;

    status = bcm_rng_fips_hash_step(pHandle, hash_input, hash_input_len, c, RNG_FIPS_SEEDLEN);
    if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_INIT_RETURN;

    /* 14. Save the internal state. */
    secure_memcpy(rngctx->v, v, RNG_FIPS_SEEDLEN);
    secure_memcpy(rngctx->c, c, RNG_FIPS_SEEDLEN);
    rngctx->reseed_counter = 1;
    rngctx->prediction_resist = prediction_resist;

RNG_HASH_INIT_RETURN:
    secure_memclr(hash_input, RNG_FIPS_TEMP_SIZE);
    secure_memclr(entropy, RNG_FIPS_ENTROPY_SIZE);
    secure_memclr(nonce, RNG_FIPS_NONCE_SIZE);
    secure_memclr(v, RNG_FIPS_SEEDLEN);
    secure_memclr(c, RNG_FIPS_SEEDLEN);

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_rng_reseed
 * Purpose : implement SP800-90A reseed
 * The algorithm is taken from the NIST SP 800-90A document Revision 1
 * B.1.2 Reseeding a Hash_DRBG Instantiation
 * Input   : pHandle:              crypto handle
 *           entropy_input:        32 bytes of entropy if testing
 *           entropy_length:        Entropy length
 *           additional_input:     optional additional input (0-64 bytes)
 *           additional_input_len: len of additional_input in bytes
 * Return  : Appropriate status
 * Remark  : SHA-256 hash
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_rng_reseed(crypto_lib_handle *pHandle,
                u32_t *entropy_input, u8_t entropy_length,
                u8_t *additional_input, u8_t additional_input_len)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    fips_rng_context *rngctx;

    u32_t entropy[RNG_FIPS_ENTROPY_SIZE/4];

    u8_t v[RNG_FIPS_SEEDLEN];
    u8_t c[RNG_FIPS_SEEDLEN];

    u8_t hash_input[RNG_FIPS_TEMP_SIZE];
    u8_t *hash_ptr = (u8_t*)hash_input;
    u8_t hash_input_len;

    if (!pHandle->rngctx)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (entropy_length > RNG_FIPS_ENTROPY_SIZE)
	    return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (additional_input_len > RNG_FIPS_MAX_ADDITIONAL_INPUT)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    rngctx = pHandle->rngctx;

    // generate entropy if not testing
    if (entropy_input) {
        // for testing
        secure_memcpy((u8_t*)entropy, (u8_t*)entropy_input, RNG_FIPS_ENTROPY_SIZE);
    } else {
        status = crypto_rng_raw_generate(pHandle, (u8_t*)entropy, RNG_FIPS_ENTROPY_SIZE/4, NULL, NULL);
        if (status != BCM_SCAPI_STATUS_OK)
        	goto RNG_HASH_RESEED_RETURN;
    }

    // V = Hash_df(0x01 || V || entropy || additional_input, seedlen)
    hash_ptr[0] = 0x01;
    hash_input_len = 1;
    secure_memcpy(hash_ptr + hash_input_len, rngctx->v, RNG_FIPS_SEEDLEN);
    hash_input_len += RNG_FIPS_SEEDLEN;
    secure_memcpy(hash_ptr + hash_input_len, (u8_t*)entropy, RNG_FIPS_ENTROPY_SIZE);
    hash_input_len += RNG_FIPS_ENTROPY_SIZE;
    if (additional_input) {
        secure_memcpy(hash_ptr + hash_input_len, additional_input, additional_input_len);
        hash_input_len += additional_input_len;
    }

    status = bcm_rng_fips_hash_step(pHandle, hash_input, hash_input_len, v, RNG_FIPS_SEEDLEN);
    if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_RESEED_RETURN;

    // C = Hash_df((0x00 || V), seedlen)
    hash_ptr[0] = 0x00;
    hash_input_len = 1;
    secure_memcpy(hash_ptr + hash_input_len, v, RNG_FIPS_SEEDLEN);
    hash_input_len += RNG_FIPS_SEEDLEN;

    status = bcm_rng_fips_hash_step(pHandle, hash_input, hash_input_len, c, RNG_FIPS_SEEDLEN);
    if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_RESEED_RETURN;

    secure_memcpy(rngctx->v, v, RNG_FIPS_SEEDLEN);
    secure_memcpy(rngctx->c, c, RNG_FIPS_SEEDLEN);
    rngctx->reseed_counter = 1;

RNG_HASH_RESEED_RETURN:
    secure_memclr(hash_input, RNG_FIPS_TEMP_SIZE);
    secure_memclr(entropy, RNG_FIPS_ENTROPY_SIZE);
    secure_memclr(v, RNG_FIPS_SEEDLEN);
    secure_memclr(c, RNG_FIPS_SEEDLEN);

    return status;
}


/*---------------------------------------------------------------
 * Name    : crypto_rng_generate
 * Purpose : implement SP800-90A generate
 * The algorithm is taken from the NIST SP 800-90A document Revision 1
 * B.1.3 Generating Pseudorandom Bits Using Hash_DRBG
 * Input   : pHandle:              crypto handle
 *           entropy_input:        32 bytes of entropy if testing
 *           entropy_length:        Entropy length
 *           output:               output buffer
 *           output_len:           requested length of RNG in bytes
 *           additional_input:     optional additional input (0-64 bytes)
 *           additional_input_len: len of additional_input in bytes
 * Return  : Appropriate status
 * Remark  : SHA-256 hash
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_rng_generate(crypto_lib_handle *pHandle,
                     u32_t *entropy_input, u8_t entropy_length,
                     u8_t *output, u32_t output_len,
                     u8_t *additional_input, u8_t additional_input_len)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;
    fips_rng_context *rngctx;

#ifdef CONFIG_DATA_CACHE_SUPPORT
    /* Make sure that the size of the buffers are atleast 64 bytes
     * and are of 64 bytes aligned SHA256_HASH_SIZE = 32,
     * SHA256_HASH_SIZE*2 = 64 makes sure that the size is multiple of 64 */
    u32_t w[SHA256_HASH_SIZE/2]__attribute__((aligned(64)));
    u32_t h[SHA256_HASH_SIZE/2]__attribute__((aligned(64)));
#else
    u32_t w[SHA256_HASH_SIZE/4];
    u32_t h[SHA256_HASH_SIZE/4];
#endif
    u8_t  v[RNG_FIPS_SEEDLEN];
    u32_t data[RNG_FIPS_SEEDLEN/4 + 1];          // isn't even, so add one for the overage

    u32_t hash_input[RNG_FIPS_TEMP_SIZE/4 + 1];  // isn't even, so add one for the overage
    u8_t *hash_ptr = (u8_t*)hash_input;
    u8_t  hash_input_len;

    u8_t chunksize;

    if (!pHandle->rngctx)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (entropy_length > RNG_FIPS_ENTROPY_SIZE)
	    return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (additional_input_len > RNG_FIPS_MAX_ADDITIONAL_INPUT)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;
    if (output_len > RNG_FIPS_MAX_REQUEST)
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    rngctx = pHandle->rngctx;

    // handle reseed if necessary (counter or prediction_resistance)
    if ( (rngctx->prediction_resist) || (rngctx->reseed_counter > RNG_FIPS_MAX_RESEED_COUNT) ) {
        status = crypto_rng_reseed(pHandle, entropy_input, RNG_FIPS_ENTROPY_SIZE,
        			additional_input, additional_input_len);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_GENERATE_RETURN;

        additional_input = NULL;
        additional_input_len = 0;
    }

    // load v into local
    secure_memcpy(v, rngctx->v, RNG_FIPS_SEEDLEN);

    // handle additional input if present
    if (additional_input) {
        // w = Hash(0x02 || V || additional_input)
        hash_ptr[0] = 0x02;
        hash_input_len = 1;
        secure_memcpy(hash_ptr + hash_input_len, v, RNG_FIPS_SEEDLEN);
        hash_input_len += RNG_FIPS_SEEDLEN;
        secure_memcpy(hash_ptr + hash_input_len, additional_input, additional_input_len);
        hash_input_len += additional_input_len;

        status = crypto_symmetric_hmac_sha256(pHandle, (u8_t*) hash_input, hash_input_len, NULL, 0, (u8_t*)w, 0);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_GENERATE_RETURN;

        // V = (V + w)mod 2^440
        bcm_soft_add_byte2byte_plusX(v, RNG_FIPS_SEEDLEN, (u8_t*)w, SHA256_HASH_SIZE, 0);
    }

    // data = V
    secure_memcpy((u8_t*)data, v, RNG_FIPS_SEEDLEN);

    // for i = 1 to (requested_size / HASH_SIZE)
    while (output_len) {
        // w = Hash(data)
        status = crypto_symmetric_hmac_sha256(pHandle, (u8_t*)data, RNG_FIPS_SEEDLEN, NULL, 0, (u8_t*)w, 0);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_GENERATE_RETURN;

        // continuous test DRBG vs last round
        if (secure_memeql((u8_t*)w, rngctx->saved_drbg, RNG_FIPS_ENTROPY_SIZE))
            fips_error();
        secure_memcpy(rngctx->saved_drbg, (u8_t*)w, RNG_FIPS_ENTROPY_SIZE);

        // W = W || w
        // output = leftmost(W, requested_size)
        chunksize = (output_len > SHA256_HASH_SIZE) ? SHA256_HASH_SIZE : output_len;

        // place DRBG round in output buffer
        secure_memcpy(output, (u8_t*)w, chunksize);
        output += chunksize;
        output_len -= chunksize;

        // data = (data + 1) mod 2^440
        bcm_soft_add_byte2byte_plusX((u8_t*)data, RNG_FIPS_SEEDLEN, NULL, 0, 1);
    }

    // H = Hash(0x03 || V)
    hash_ptr[0] = 0x03;
    hash_input_len = 1;
    secure_memcpy(hash_ptr + hash_input_len, v, RNG_FIPS_SEEDLEN);
    hash_input_len += RNG_FIPS_SEEDLEN;

    status = crypto_symmetric_hmac_sha256(pHandle, (u8_t*)hash_input, hash_input_len, NULL, 0, (u8_t*)h, 0);
    if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_GENERATE_RETURN;

    // V = (V + H + C + reseed_counter) mode 2^440
    // done in two steps V = V + h + counter, then V = V + C
    bcm_soft_add_byte2byte_plusX(v, RNG_FIPS_SEEDLEN, (u8_t*)h, SHA256_HASH_SIZE, rngctx->reseed_counter);
    bcm_soft_add_byte2byte_plusX(v, RNG_FIPS_SEEDLEN, rngctx->c, RNG_FIPS_SEEDLEN, 0);

    // reseed_counter += 1
    rngctx->reseed_counter++;

    secure_memcpy(rngctx->v, v, RNG_FIPS_SEEDLEN);

RNG_HASH_GENERATE_RETURN:
#ifdef CONFIG_DATA_CACHE_SUPPORT
    secure_memclr(w, SHA256_HASH_SIZE/2);
    secure_memclr(h, SHA256_HASH_SIZE/2);
#else
    secure_memclr(w, SHA256_HASH_SIZE/4);
    secure_memclr(h, SHA256_HASH_SIZE/4);
#endif
    secure_memclr(hash_input, sizeof(hash_input));
    secure_memclr(v, RNG_FIPS_SEEDLEN);
    secure_memclr(data, sizeof(data));

    return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_rng_raw_generate
 * Purpose : Generate a block of random bytes.
 *           This is not a user API, user should always call the fips version
 * Input   :
 *           result: pointer to the final output memory
 *           num_words: the number of RNG words that the caller needs
 * Return  : Appropriate status
 * Remark  : If the callback parameter is NULL, the operation is synchronous
 *           Difference with PKE functions: no output length pointer
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_rng_raw_generate (
                    crypto_lib_handle *pHandle,
                    u8_t  *result, u32_t num_words,
                    scapi_callback callback, void *arg)
{
	volatile u32_t nAvailbleWords;
	crypto_lib_handle *old_pHandle = pHandle;

	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;

	pHandle = &(pHandle[CRYPTO_INDEX_RNG]);
	CHECK_DEVICE_BUSY(CRYPTO_INDEX_RNG);

	if (num_words > RNG_MAX_NUM_WORDS)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	/* Get random numbers. */
	nAvailbleWords = RNG_GET_AVAILABLE_WORDS();

	if (callback == NULL) {
		/* For blocking function, keep waiting */
		while (nAvailbleWords < num_words) {
			nAvailbleWords = RNG_GET_AVAILABLE_WORDS();
		}

		crypto_rng_readout(result, num_words);

		/* continuous test raw rng for last generated output */
		if (secure_memeql((u8_t*)result, old_pHandle->rngctx->saved_raw,
				num_words*4))
			fips_error();
		secure_memcpy(old_pHandle->rngctx->saved_raw, (u8_t*)result,
				num_words*4);
	} else {
	if (nAvailbleWords >= num_words) {
			/* we have the numbers already, read it out */
			crypto_rng_readout(result, num_words);
			/* call the callback */
			callback (status, pHandle, arg);
		} else {
			/* assign context structure */
			pHandle -> busy       = TRUE;
			pHandle -> result     = result;
			pHandle -> result_len = num_words;
			pHandle -> callback   = callback;
			pHandle -> arg        = arg;
		}
	}

	return status;
}

/* ***************************************************************************
 * Private Functions
 * ****************************************************************************/

static void crypto_rng_readout(u8_t *result, u32_t num_words)
{
    u32_t i;
    u32_t rngTemp;
    for (i = 0; i < num_words; i++)
    {
	    rngTemp = *RNG_DATA_REG;
	    /* Write random data */
	    secure_memcpy(result, (u8_t *)&rngTemp, 4);
	    result += 4;
    }
}

/*---------------------------------------------------------------
 * Name    : bcm_rng_fips_hash_step
 * Purpose : implement SP800-90A Hash_df
 * Input   : pHandle:               crypto handle
 *           input:                 input buffer
 *           input_len:             length of input data (bytes)
 *           output:                output buffer (max of 255 * 32 bytes)
 *           output_len:            requested length of hash data (bytes)
 * Return  : Appropriate status
 * Remark  :
 *--------------------------------------------------------------*/
static BCM_SCAPI_STATUS bcm_rng_fips_hash_step(crypto_lib_handle *pHandle,
                   u8_t *input, u32_t input_len,
                   u8_t *output, u32_t output_len)
{
    BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_OK;

    u32_t hash_input[RNG_FIPS_TEMP_SIZE/4 + 1];  // isn't even, so add one for the overage
    u8_t *hash_ptr = (u8_t*)hash_input;
    u8_t hash_input_len;

#ifdef CONFIG_DATA_CACHE_SUPPORT
    u32_t hash[SHA256_HASH_SIZE/2]__attribute__((aligned(64)));
#else
    u32_t hash[SHA256_HASH_SIZE/4];
#endif

    u32_t bits_to_return = output_len * 8;
    u8_t chunksize;

    if (output_len > (SHA256_HASH_SIZE * 255))
        return BCM_SCAPI_STATUS_PARAMETER_INVALID;

    // bits_to_return is used BE
    bcm_swap_endian(&bits_to_return, 4, &bits_to_return, 1);

    // setup the input: Hash (counter || no_of_bits_to_return || input_string)
    hash_ptr[0] = 0x01;
    hash_input_len = 1;
    secure_memcpy(hash_ptr + hash_input_len, (u8_t*)&bits_to_return, 4);
    hash_input_len += 4;
    secure_memcpy(hash_ptr + hash_input_len, input, input_len);
    hash_input_len += input_len;

    while (output_len) {
        // no_of_bits_to_return is used as a 32-bit string.
        // temp = temp || Hash (counter || no_of_bits_to_return || input_string)
        status = crypto_symmetric_hmac_sha256(pHandle, (u8_t*) hash_input, hash_input_len, NULL, 0, (u8_t*)hash, 0);
        if (status != BCM_SCAPI_STATUS_OK) goto RNG_HASH_STEP_RETURN;

        // requested_bits = leftmost (temp, no_of_bits_to_return). Return (SUCCESS, requested_bits).
        chunksize = (output_len > SHA256_HASH_SIZE) ? SHA256_HASH_SIZE : output_len;
        secure_memcpy((u8_t*)output, (u8_t*)hash, chunksize);
        output += chunksize;
        output_len -= chunksize;

        // counter = counter + 1
        hash_ptr[0]++;
    }

RNG_HASH_STEP_RETURN:
    secure_memclr(hash_input, sizeof(hash_input));
#ifdef CONFIG_DATA_CACHE_SUPPORT
    secure_memclr(hash, SHA256_HASH_SIZE/2);
#else
    secure_memclr(hash, SHA256_HASH_SIZE/4);
#endif

    if (status != BCM_SCAPI_STATUS_OK)
        secure_memclr(output, output_len);

    return status;
}


