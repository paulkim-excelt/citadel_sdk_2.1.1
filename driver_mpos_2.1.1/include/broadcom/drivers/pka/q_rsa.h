/*******************************************************************
 *
 * Copyright 2006
 * Broadcom Corporation
 * 16215 Alton Parkway
 * PO Box 57013
 * Irvine CA 92619-7013
 *
 * Broadcom provides the current code only to licensees who
 * may have the non-exclusive right to use or modify this
 * code according to the license.
 * This program may be not sold, resold or disseminated in
 * any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************/
/*!
******************************************************************
* \file
*   q_rsa.h
*
* \brief
*   Header file for RSA functions
*   RSA encryption can be implemented using modexp function. For
*   embedded system with limited ROM size, RSA encryption function is
*   optional.
*
*   ppsel function is included here because it is used for RSA key generation.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 12/15/2007
*
* \version
*   0.1
*
******************************************************************
*/
#ifndef _Q_RSA_H_
#define _Q_RSA_H_

typedef struct q_rsa_key {
  q_lint_t n;
  q_lint_t e;
} q_rsa_key_t; //!< data structure of the RSA public key

typedef struct q_rsa_crt_key {
  q_lint_t p;
  q_lint_t q;
  q_lint_t dp;
  q_lint_t dq;
  q_lint_t qinv;
} q_rsa_crt_key_t; //!< data structure of the RSA CRT private key

q_status_t q_rsa_enc (q_lip_ctx_t *ctx, q_lint_t *c, q_rsa_key_t *rsa, q_lint_t *m);
/*!< RSA encryption function
    \param ctx QLIP context pointer
    \param c cipher text
    \param rsa RSA public key structure
    \param m clear text
*/

q_status_t q_rsa_crt (q_lip_ctx_t *ctx, q_lint_t *m, q_rsa_crt_key_t *rsa, q_lint_t *c);
/*!< RSA CRT decryption function
    \param ctx QLIP context pointer
    \param m clear text
    \param rsa RSA CRT private key structure
    \param c cipher text
*/

q_status_t q_ppsel (q_lip_ctx_t *ctx, q_lint_t *seed, u32_t step_size, u32_t num_retries);
/*!< prime number pre-selection function to return a qualified prime number candidate for Rabin-Miller
    \param ctx QLIP context pointer
    \param seed the prime number seed to be selected
    \param step_size increment after each trial
    \param num_retries number of retries
*/

#endif /*_Q_RSA_H_*/
