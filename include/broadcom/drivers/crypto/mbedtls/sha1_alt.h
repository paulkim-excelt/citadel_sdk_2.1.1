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
 * @file sha1_alt.h
 *
 * @brief sha1 alternate header file
 */

#ifndef MBEDTLS_SHA1_ALT_H
#define MBEDTLS_SHA1_ALT_H

#include "mbedtls/sha1.h"

#if defined(MBEDTLS_SHA1_ALT)

#define BLOCK_SIZE (64)

#ifdef __cplusplus
extern "C" {
#endif

struct mbedtls_sha1_context_s;

/**
 * \brief          ShA-1 context structure
 */
typedef struct mbedtls_sha1_context_s {
	unsigned char digest[BLOCK_SIZE]__attribute__ ((aligned(64)));
	unsigned char last_block[BLOCK_SIZE];
	unsigned char init;
	unsigned char end;
	unsigned char last_block_present;
	unsigned int total_len;
}
mbedtls_sha1_context __attribute__ ((aligned(64)));

#ifndef MBEDTLS_VERSION_2_16
/**
 * \brief          Initialize ShA-1 context
 *
 * \param ctx      ShA-1 context to be initialized
 */
void mbedtls_sha1_init(mbedtls_sha1_context *ctx);

/**
 * \brief          Clear ShA-1 context
 *
 * \param ctx      ShA-1 context to be cleared
 */
void mbedtls_sha1_free(mbedtls_sha1_context *ctx);

/**
 * \brief          Clone (the state of) a ShA-1 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 */
void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
			  const mbedtls_sha1_context *src);

/**
 * \brief          ShA-1 context setup
 *
 * \param ctx      context to be initialized
 *
 * \returns        error code
 */
int mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx);

/**
 * \brief          ShA-1 process buffer
 *
 * \param ctx      ShA-1 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 *
 * \returns        error code
 */
int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx,
			      const unsigned char *input, size_t ilen);

/**
 * \brief          ShA-1 final digest
 *
 * \param ctx      ShA-1 context
 * \param output   ShA-1 checksum result
 *
 * \returns        error code
 */
int mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx,
			      unsigned char output[32]);

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED      __attribute__((deprecated))
#else
#define MBEDTLS_DEPRECATED
#endif
/**
 * \brief          This function starts a ShA-1 checksum calculation.
 *
 * \deprecated     Superseded by mbedtls_sha1_starts_ret() in 2.7.0.
 *
 * \param ctx      The SHA-1 context to initialize.
 */
int mbedtls_sha1_starts(mbedtls_sha1_context *ctx);

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-1 checksum calculation.
 *
 * \deprecated     Superseded by mbedtls_sha1_update_ret() in 2.7.0.
 *
 * \param ctx      The SHA-1 context to initialize.
 * \param input    The buffer holding the data.
 * \param ilen     The length of the input data.
 */
int mbedtls_sha1_update(mbedtls_sha1_context *ctx,
			  const unsigned char *input, size_t ilen);

/**
 * \brief          This function finishes the SHA-1 operation, and writes
 *                 the result to the output buffer.
 *
 * \deprecated     Superseded by mbedtls_sha1_finish_ret() in 2.7.0.
 *
 * \param ctx      The SHA-1 context.
 * \param output   The SHA-1 checksum result.
 */
int mbedtls_sha1_finish(mbedtls_sha1_context *ctx,
			   unsigned char output[32]);

/**
 * \brief          This function processes a single data block within
 *                 the ongoing ShA-1 computation. This function is for
 *                 internal use only.
 *
 * \deprecated     Superseded by mbedtls_internal_sha1_process() in 2.7.0.
 *
 * \param ctx      The ShA-1 context.
 * \param data     The buffer holding one block of data.
 */
int mbedtls_sha1_process(mbedtls_sha1_context *ctx,
			   const unsigned char data[64]);
#endif /* !MBEDTLS_VERSION_2_16 */

#undef MBEDTLS_DEPRECATED
#endif /* !MBEDTLS_DEPRECATED_REMOVED */

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA1_ALT */

#endif /* sha1_alt.h */
