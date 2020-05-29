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
*   q_rsa_4k.h
*
* \brief
*   Header file for RSA functions with 3k or 4k modulus sizes.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 5/1/2008
*
* \version
*   0.1
*
******************************************************************
*/
#ifndef _Q_RSA_4K_H_
#define _Q_RSA_4K_H_

#include "q_rsa.h"

q_status_t q_modexp_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *e, q_lint_t *n);
/*!< long integer modulo exponentiation , z = a^e mod n
    \param ctx QLIP context pointer
    \param z result
    \param a base
    \param e exponent
    \param n modulus
*/

q_status_t q_mont_init_sw (q_lip_ctx_t *ctx, q_mont_t *mont, q_lint_t *n);
/*!< Montgomery field initialization function
    \param ctx QLIP context pointer
    \param n modulus
    \param mont Montgomery fields
*/

q_status_t q_mont_mul_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b, q_mont_t *mont);
/*!< long integer modulo mulitply, z = a * b mod n using Montgomery's method
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
    \param mont Montgomery fields
*/

q_status_t q_modexp_mont_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *e, q_mont_t *mont);
/*!< long integer modulo exponentiation , z = a^e mod n using Montgomery's method
    \param ctx QLIP context pointer
    \param z result
    \param a base
    \param e exponent
    \param mont Montgomery fields
*/

q_status_t q_gcd_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< greatest common divisor function
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
*/

q_status_t q_euclid_sw (q_lip_ctx_t *ctx, q_lint_t *x, q_lint_t *lx, q_lint_t *a, q_lint_t *b);
/*!< Extended Euclid Function d = a * x + b * y return x
    \param ctx QLIP context pointer
    \param x result
    \param lx result
    \param a source
    \param b source
*/

q_status_t q_rsa_crt_4k (q_lip_ctx_t *ctx, q_lint_t *m, q_rsa_crt_key_t *rsa, q_lint_t *c);
/*!< RSA CRT decryption function
    \param ctx QLIP context pointer
    \param m clear text
    \param rsa RSA CRT private key structure
    \param c cipher text
*/

#endif /*_Q_RSA_4K_H_*/
