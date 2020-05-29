/******************************************************************************
 *  Copyright (C) 2019 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/*
 * @file gcm_alt.h
 *
 * @brief gcm alternate header file
 */

#ifndef MBEDTLS_GCM_ALT_H
#define MBEDTLS_GCM_ALT_H

#include "mbedtls/gcm.h"

#ifdef MBEDTLS_GCM_ALT

#ifdef __cplusplus
extern "C" {
#endif

#define GCM_128_KEY_LEN_BYTES			16
#define GCM_192_KEY_LEN_BYTES			24
#define GCM_256_KEY_LEN_BYTES			32

/**
 * \brief          GCM context structure
 */
typedef struct {
	unsigned int key_size; /* Key size: AES_KEY_SIZE_128/192/256 */
	unsigned char keys[GCM_256_KEY_LEN_BYTES]; /* Cipher key */
}
mbedtls_gcm_context;

/**
 * \brief           Initialize GCM context (just makes references valid)
 *                  Makes the context ready for mbedtls_gcm_setkey() or
 *                  mbedtls_gcm_free().
 *
 * \param ctx       GCM context to initialize
 */
void mbedtls_gcm_init(mbedtls_gcm_context *ctx);

/**
 * \brief           GCM initialization (encryption)
 *
 * \param ctx       GCM context to be initialized
 * \param cipher    cipher to use (a 128-bit block cipher)
 * \param key       encryption key
 * \param keybits   must be 128, 192 or 256
 *
 * \return          0 if successful, or a cipher specific error code
 */
int mbedtls_gcm_setkey(mbedtls_gcm_context *ctx,
		       mbedtls_cipher_id_t cipher,
		       const unsigned char *key,
		       unsigned int keybits);

/**
 * \brief           GCM buffer encryption/decryption using a block cipher
 *
 * \note On encryption, the output buffer can be the same as the input buffer.
 *       On decryption, the output buffer cannot be the same as input buffer.
 *       If buffers overlap, the output buffer must trail at least 8 bytes
 *       behind the input buffer.
 *
 * \param ctx       GCM context
 * \param mode      MBEDTLS_GCM_ENCRYPT or MBEDTLS_GCM_DECRYPT
 * \param length    length of the input data
 * \param iv        initialization vector
 * \param iv_len    length of IV
 * \param add       additional data
 * \param add_len   length of additional data
 * \param input     buffer holding the input data
 * \param output    buffer for holding the output data
 * \param tag_len   length of the tag to generate
 * \param tag       buffer for holding the tag
 *
 * \return         0 if successful
 */
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
			      int mode,
			      size_t length,
			      const unsigned char *iv,
			      size_t iv_len,
			      const unsigned char *add,
			      size_t add_len,
			      const unsigned char *input,
			      unsigned char *output,
			      size_t tag_len,
			      unsigned char *tag);

/**
 * \brief           GCM buffer authenticated decryption using a block cipher
 *
 * \note On decryption, the output buffer cannot be the same as input buffer.
 *       If buffers overlap, the output buffer must trail at least 8 bytes
 *       behind the input buffer.
 *
 * \param ctx       GCM context
 * \param length    length of the input data
 * \param iv        initialization vector
 * \param iv_len    length of IV
 * \param add       additional data
 * \param add_len   length of additional data
 * \param tag       buffer holding the tag
 * \param tag_len   length of the tag
 * \param input     buffer holding the input data
 * \param output    buffer for holding the output data
 *
 * \return         0 if successful and authenticated,
 *                 MBEDTLS_ERR_GCM_AUTH_FAILED if tag does not match
 */
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context *ctx,
			     size_t length,
			     const unsigned char *iv,
			     size_t iv_len,
			     const unsigned char *add,
			     size_t add_len,
			     const unsigned char *tag,
			     size_t tag_len,
			     const unsigned char *input,
			     unsigned char *output);

/**
 * \brief           Generic GCM stream start function
 *
 * \param ctx       GCM context
 * \param mode      MBEDTLS_GCM_ENCRYPT or MBEDTLS_GCM_DECRYPT
 * \param iv        initialization vector
 * \param iv_len    length of IV
 * \param add       additional data (or NULL if length is 0)
 * \param add_len   length of additional data
 *
 * \return         0 if successful
 */
int mbedtls_gcm_starts(mbedtls_gcm_context *ctx,
		       int mode,
		       const unsigned char *iv,
		       size_t iv_len,
		       const unsigned char *add,
		       size_t add_len);

/**
 * \brief           Generic GCM update function. Encrypts/decrypts using the
 *                  given GCM context. Expects input to be a multiple of 16
 *                  bytes! Only the last call before mbedtls_gcm_finish() can be
 *                  less than 16 bytes!
 *
 * \note On decryption, the output buffer cannot be the same as input buffer.
 *       If buffers overlap, the output buffer must trail at least 8 bytes
 *       behind the input buffer.
 *
 * \param ctx       GCM context
 * \param length    length of the input data
 * \param input     buffer holding the input data
 * \param output    buffer for holding the output data
 *
 * \return         0 if successful or MBEDTLS_ERR_GCM_BAD_INPUT
 */
int mbedtls_gcm_update(mbedtls_gcm_context *ctx,
		       size_t length,
		       const unsigned char *input,
		       unsigned char *output);

/**
 * \brief           Generic GCM finalisation function. Wraps up the GCM stream
 *                  and generates the tag. The tag can have a maximum length of
 *                  16 bytes.
 *
 * \param ctx       GCM context
 * \param tag       buffer for holding the tag
 * \param tag_len   length of the tag to generate (must be at least 4)
 *
 * \return          0 if successful or MBEDTLS_ERR_GCM_BAD_INPUT
 */
int mbedtls_gcm_finish(mbedtls_gcm_context *ctx,
		       unsigned char *tag,
		       size_t tag_len);

/**
 * \brief           Free a GCM context and underlying cipher sub-context
 *
 * \param ctx       GCM context to free
 */
void mbedtls_gcm_free(mbedtls_gcm_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_GCM_ALT */

#endif /* gcm_alt.h */
