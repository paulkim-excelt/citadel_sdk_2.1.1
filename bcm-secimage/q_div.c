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
*   q_div.c
*
* \brief
*   This file contains a long division routine, an optimized divide by
* power-of-2 routine and a modulo divide-by-2 routine.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 7/5/2006\n
*   Updated: 10/24/2007
*
* \version
*   0.2
*
******************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include "q_lip.h"
#include "q_lip_aux.h"

#ifdef QLIP_DEBUG
#include "q_lip_utils.h"
#endif

#ifdef QLIP_USE_GMP
#include "gmp.h"
#endif

/**********************************************************************
 * q_div ()
 * z = floor (a / b)
 *
 * Description: This function returns the quotient.
 **********************************************************************/

q_status_t q_div (q_lip_ctx_t *ctx,    /* QLIP context pointer */
                  q_lint_t *z,      /* q_lint pointer to result z */
                  q_lint_t *a,   /* q_lint pointer to source a */
                  q_lint_t *b)   /* q_lint pointer to source b */
{
  int i;

#ifdef QLIP_USE_GMP
  mpz_t mz, ma, mb;
#else
  int a_neg, b_neg;
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  if (q_IsZero (b)) {
#ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: q_div divide by zero!");
#endif
    ctx->status = Q_ERR_DIV_BY_0;
    goto Q_DIV_EXIT;
  }

  if (!a->size || a->size < b->size) {
    q_SetZero (z);
    goto Q_DIV_EXIT;
  }

  if (a->size == b->size) {
    //to add leading 1 detection
    i=0;
  }

#ifdef QLIP_USE_GMP
  mpz_init (mz);
  mpz_init (ma);
  mpz_init (mb);

  mpz_import (mb, b->size, -1, 4, 0, 0, b->limb);
  mpz_import (ma, a->size, -1, 4, 0, 0, a->limb);
  mpz_div (mz, ma, mb);

  mpz_export (z->limb, & (z->size), -1, 4, 0, 0, mz);

  mpz_clear (mz);
  mpz_clear (ma);
  mpz_clear (mb);
#else
  /* call Tao's function */
  a_neg = a->neg;
  b_neg = b->neg;

  q_fdivrem (ctx, a, b, z, NULL);

  z->neg = a_neg ^ b_neg;

  Q_NORMALIZE (z->limb, z->size);

#endif

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_DIV_EXIT:
  return (ctx->status);
}

/**********************************************************************
 * q_div_2pn ()
 * z = (a / b) where b is 2^n
 *
 * Description: This function is a bit shift operation.
 **********************************************************************/

q_status_t q_div_2pn (q_lint_t *z,
                      q_lint_t *a,
                      uint32_t bits)
{
  int i;
  int shift;
  q_limb_ptr_t zp, ap;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  shift = (bits >> BITS_FOR_MACHINE_WD);

  if (shift >= a->size) {
    q_SetZero (z);
    goto Q_DIV_2PN_EXIT;
  }

  zp = z->limb;
  ap = a->limb;

  for (i=0; i < (a->size - shift); i++) {
    zp[i] = ap[i+shift];
  }
  z->size = a->size - shift;

  shift = bits - (shift << BITS_FOR_MACHINE_WD);
  if (shift) {
    for (i=0; i < (z->size-1); i++) {
      zp[i] = (zp[i] >> shift) | ( (zp[i+1] & (UINT_MASK >> (MACHINE_WD-shift))) << (MACHINE_WD-shift));
    }
    zp[i] = (zp[i] >> shift);
  }

  Q_NORMALIZE (z->limb, z->size);

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_DIV_2PN_EXIT:
  return (Q_SUCCESS);
}

/**********************************************************************
 * q_mod_div2 ()
 * z = a / 2 mod n
 *
 * Description: This function requires the modulus n be an odd
 * number. The algorithm is as follows.
 *
 * If (a == even) z = a / 2;
 * Else z = (a + n) / 2;
 * Return z;
 **********************************************************************/

q_status_t q_mod_div2 (q_lip_ctx_t *ctx,
                       q_lint_t *z,
                       q_lint_t *a,
                       q_lint_t *n)
{
  q_lint_t tmp;
  q_size_t size;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */
  size = a->size+1;
  if (q_init (ctx, &tmp, size)) goto Q_MOD_DIV2_EXIT;

  q_copy (&tmp, a);
  if (a->limb[0] & 0x00000001) q_uadd (&tmp, &tmp, n);
  q_div_2pn (&tmp, &tmp, 1);

  if ((ctx->status = q_copy (z, &tmp)) != Q_SUCCESS) goto Q_MOD_DIV2_EXIT;

  q_free (ctx, &tmp);

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_MOD_DIV2_EXIT:
  return (ctx->status);
}
