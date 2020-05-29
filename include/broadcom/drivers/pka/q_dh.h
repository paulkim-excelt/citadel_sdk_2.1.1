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
*   q_dh.h
*
* \brief
*   header file for Diffie-Hellman related functions.
*
*   Diffie-Hellman functions can be implemented using modexp function.
*   For embedded system with limited ROM space, Diffie-Hellman functions
*   are optional.
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

#ifndef _Q_DH_H_
#define _Q_DH_H_

typedef struct q_dh_param { 
  q_lint_t p;
  q_lint_t g;
} q_dh_param_t; //!< data structure of the Diffie-Hellman parameter

q_status_t q_dh_pk (q_lip_ctx_t *ctx, q_lint_t *xpub, q_dh_param_t *dh, q_lint_t *x);
/*!< Diffie-Hellman public value (public key) generation
    \param ctx QLIP context pointer
    \param xpub generated DH public value
    \param dh DH parameters
    \param x DH private key
*/

q_status_t q_dh_ss (q_lip_ctx_t *ctx, q_lint_t *ss, q_dh_param_t *dh, q_lint_t *xpub, q_lint_t *y);
/*!< Diffie-Hellman shared secret generation
    \param ctx QLIP context pointer
    \param ss DH shared secret
    \param dh DH parameters
    \param xpub DH public value of x
    \param y DH private key of y
*/

#endif /*_Q_DH_H_*/
