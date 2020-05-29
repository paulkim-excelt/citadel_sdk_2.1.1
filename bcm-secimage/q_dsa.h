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
*   q_dsa.h
*
* \brief
*   header file for DSA functions
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

#ifndef _Q_DSA_H_
#define _Q_DSA_H_

typedef struct q_dsa_param { 
  q_lint_t p;
  q_lint_t q;
  q_lint_t g;
} q_dsa_param_t; //!< data structure of the DSA parameter

q_status_t q_dsa_sign (q_lip_ctx_t *ctx, q_signature_t *rs, q_dsa_param_t *dsa, q_lint_t *d, q_lint_t *h, q_lint_t *k);
/*!< DSA digital signature signing function
    \param ctx QLIP context pointer
    \param rs generated DSA signature
    \param dsa DSA parameters
    \param d signing private key
    \param h hashed message to be signed
    \param k random
*/

q_status_t q_dsa_verify (q_lip_ctx_t *ctx, q_lint_t *v, q_dsa_param_t *dsa, q_lint_t *y, q_lint_t *h, q_signature_t *rs);
/*!< DSA digital signature verification function
    \param ctx QLIP context pointer
    \param v computed signature verification value to be compared with rs->s
    \param dsa DSA parameters
    \param y DSA public key
    \param h hashed message to be signed
    \param rs generated DSA signature
*/

#endif /*_Q_DSA_H_*/
