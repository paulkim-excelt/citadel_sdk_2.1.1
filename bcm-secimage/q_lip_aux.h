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
*   q_lip_aux.h
*
* \brief
*   header file for data initialization, checking and memory management functions 
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 12/15/2006\n
*   Updated: 10/24/2007
*
* \version
*   0.2
*
******************************************************************
*/

#ifndef _Q_LIP_AUX_H_
#define _Q_LIP_AUX_H_

q_status_t q_init (q_lip_ctx_t *ctx, q_lint_t *z, q_size_t size);
/*!< initialize a long integer and allocate storage in the QLIP context
    \param ctx QLIP context pointer 
    \param z long integer to be initialized
    \param size the memory size (in 32-bit words) to be allocated for the long integer

    \note  long integer must be initialized before it is used. Initialization allocates
    memory storage in the context.
*/

q_status_t q_copy (q_lint_t *z, q_lint_t *a);
/*!< copy a long integer
    \param z result long integer
    \param a source long integer
*/

uint32_t *q_malloc (q_lip_ctx_t *ctx, q_size_t size);
/*!< allocate memory in QLIP context
    \param ctx QLIP context pointer
    \param size the size of the memory to be allocated (in 32-bit words)
*/

q_status_t q_free (q_lip_ctx_t *ctx, q_lint_t *z);
/*!< free memory allocated from the long integer from QLIP context
    \param ctx QLIP context pointer
    \param z the long integer
*/

/*inline*/ int q_IsZero(q_lint_t *a); 
/*!< check if the long integer is zero
    \param a the long integer
*/

/*inline*/ int q_IsOne(q_lint_t *a);  
/*!< check if the long integer is one
    \param a the long integer
*/

/*inline*/ void q_SetZero(q_lint_t *a);
/*!< set the long integer to zero
    \param a the long integer
*/

/*inline*/ void q_SetOne(q_lint_t *a); 
/*!< set the long integer to one
    \param a the long integer
*/

/*inline*/ int q_IsOdd(q_lint_t *a);  
/*!< check if the long integer is odd
    \param a the long integer
*/

/*inline*/ int q_LeadingOne(uint32_t r); 
/*!<find the leading '1' of a 32-bit integer, may be mapped to a single instruction
    \param r the 32-b integer
*/

#endif /*_Q_LIP_AUX_H_*/
