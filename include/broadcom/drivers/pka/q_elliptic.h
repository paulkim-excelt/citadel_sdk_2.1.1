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
*   q_elliptic.h
*
* \brief
*   header file for basic prime field elliptic curve data structures and functions
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 12/15/2006
*   Updated: 10/24/2007
*
* \version
*   0.2
*
******************************************************************
*/

#ifndef _Q_ELLIPTIC_H_
#define _Q_ELLIPTIC_H_

#define QLIP_PKA_ECP_DBL_SEQ_LEN 19
#define QLIP_PKA_ECP_ADD_SEQ_LEN 25
#define QLIP_PKA_ECP_SUB_SEQ_LEN 27

typedef struct q_point {
	q_lint_t X;
	q_lint_t Y;
	q_lint_t Z; //!< Z=1 if Affine
} q_point_t;
//!< data structure of a point on the elliptic curve

typedef struct q_curve {
	u32_t type; //!< 0: prime field, 1: binary field
	q_lint_t n;    //!< the order or the curve
	q_lint_t a;    //!< parameter a
	q_lint_t b;    //!< parameter b
	q_lint_t p;    //!< prime p
	q_mont_t *mn;  //!< Montgomery context for curve order n
	q_mont_t *mp;  //!< Montgomery context for prime p
} q_curve_t;
//!< data structure of an elliptic curve

/* point operation functions */
q_status_t q_pt_copy (q_point_t *r, q_point_t *p); //!< copy a point to another point

int q_pt_IsAffine (q_point_t *p); //!< determine if the coordinates of a point is in Affine format

q_status_t q_ecp_prj_2_affine (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p, q_curve_t *curve);
/*!< Jacobian coordinates to Affine coordinates conversion.
    \param ctx The QLIP context
    \param r the converted Affine point
    \param p the Jacobian point
    \param curve the Elliptic curve data structure
*/

q_status_t q_ecp_pt_mul (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p, q_lint_t *k, q_curve_t *curve);
/*!< Affine prime field point multiplication, r(x,y) = p(x,y) * k
    \param ctx The QLIP context
    \param r the result point
    \param p the source point
    \param k the multiplier value
    \param curve the Elliptic curve data structure
*/

q_status_t q_ecp_pt_mul_prj (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p, q_point_t *p_minus, q_lint_t *k, q_curve_t *curve, q_lint_t *tmp);
/*!< Jacobian prime field point multiplication with precomputed negative and pre-allocated temporary storage for performance.
    \note this function is intended to be called multiple times by a higher level function such as ECDSA functions.
    \param ctx The QLIP context
    \param r the result point
    \param p the source point
    \param p the negative source point
    \param k the multiplier value
    \param curve the Elliptic curve data structure
    \param tmp temporary storage allocated at higher level
*/

q_status_t q_ecp_pt_dbl (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p, q_curve_t *curve);
/*!< Affine prime field point doubling, r(x,y) = p(x,y) + p(x,y).
		\note this is a wrapper function of q_ecp_pt_dbl_prj
    \param ctx The QLIP context
    \param r the result point
    \param p the source point
    \param curve the Elliptic curve data structure
*/

q_status_t q_ecp_pt_add (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p0, q_point_t *p1, q_curve_t *curve);
/*!< Affine prime field point addition, r(x,y) = p0(x,y) + p1(x,y).
    \note this is a wrapper function of q_ecp_pt_add_prj
    \param ctx The QLIP context
    \param r the result point
    \param p0 the source point p0
    \param p1 the source point p1
    \param curve the Elliptic curve data structure
*/

q_status_t q_ecp_pt_sub (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p0, q_point_t *p1, q_curve_t *curve);
/*!< Affine prime field point subtraction, r(x,y) = p0(x,y) - p1(x,y).
    \param ctx The QLIP context
    \param r the result point
    \param p0 the source point p0
    \param p1 the source point p1
    \param curve the Elliptic curve data structure 
*/

q_status_t q_ecp_pt_dbl_prj (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p, q_curve_t *curve, q_lint_t *tmp);
/*!< Jacobian prime field point doubling, r(x,y) = p(x,y) + p(x,y).
    \note this function is intended to called multiple times by a higher level function such as q_ecp_pt_mul_prj 
    \param ctx The QLIP context
    \param r the result point
    \param p the source point
    \param curve the Elliptic curve data structure
    \param tmp temporary storage allocated at higher level
*/


q_status_t q_ecp_pt_add_prj (q_lip_ctx_t *ctx, q_point_t *r, q_point_t *p0, q_point_t *p1, q_curve_t *curve, q_lint_t *tmp);
/*!< Jacobian prime field point addition, r(x,y) = p0(x,y) + p1(x,y).
    \note this function is intended to called multiple times by a higher level function such as q_ecp_pt_mul_prj 
    \param ctx The QLIP context
    \param r the result point
    \param p0 the source point p0
    \param p1 the source point p1
    \param curve the Elliptic curve data structure
    \param tmp temporary storage allocated at higher level
*/

q_status_t q_ecp_ecdsa_sign (q_lip_ctx_t *ctx, q_signature_t *rs, q_point_t *G, q_curve_t *curve, 
                             q_lint_t *d, q_lint_t *h, q_lint_t *k);
/*!< EC-DSA signature singing function
    \param ctx The QLIP context
    \param rs the computed signature
    \param G the base point
    \param curve the Elliptic curve data structure
    \param d the private key for signing
    \param h the hash result of the message to be signed
    \param k the random nonce
*/

q_status_t q_ecp_ecdsa_verify (q_lip_ctx_t *ctx, q_lint_t *v, q_point_t *G, q_curve_t *curve, 
                             q_point_t *Q, q_lint_t *h, q_signature_t *rs);
/*!< EC-DSA signature verification function
    \note  higher level function still has to compare v == s to verify the signature is indeed correct.
    \param ctx The QLIP context
    \param v the computed signature verification value
    \param G the base point
    \param curve the Elliptic curve data structure
    \param Q the public key for signature verification
    \param h the hash result of the message to be signed
    \param rs the signature to be verified
*/

#ifdef QLIP_USE_PKA_HW
q_status_t q_ecp_prj_pka_init (q_lip_ctx_t *ctx, q_curve_t *curve, q_point_t *pt0, q_point_t *pt1);
/*!< Optimized PKA hardware initialization and data loading function
    \note this function is only used with PKA hardware. It represents a hardware micro-code sequence invoked by higher level functions.
*/

void q_ecp_pt_dbl_prj_pka (opcode_t *sequence, u32_t LIR_p);
/*!< Optimized PKA hardware point doubling function
    \note this function is only used with PKA hardware. It represents a hardware micro-code sequence invoked by higher level functions.
*/

void q_ecp_pt_add_prj_pka (opcode_t *sequence, u32_t LIR_p);
/*!< Optimized PKA hardware point addition function
    \note this function is only used with PKA hardware. It represents a hardware micro-code sequence invoked by higher level functions.
*/

q_status_t q_ecp_prj_pka_get_result (q_lip_ctx_t *ctx, q_point_t *pt);
/*!< Optimized PKA hardware result unloading function
    \note this function is only used with PKA hardware. It represents a hardware micro-code sequence invoked by higher level functions.
*/
#endif

#endif
