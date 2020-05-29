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
*   q_fdivrem.c
*
* \brief
*   This file contains the optimized division and remainder routine.
*
* \author
*   Tao Long (x2-8034)   taolong@broadcom.com
*
* \date
*   Originated: 8/16/2007\n
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

/***********************************************************************
 * This function implements both long division and modulo reduction.
 * Inputs:
 *    opa -- dividend
 *    opn -- divisor
 * Output:
 *    quotient -- floor (opa / opn)
 *    remainder -- opa % opn
 * Description:
 *  This function implements Alg 4.3.1D in Knuth's Art of Computer
 * Programming Vol 2 (3rd Ed). The digit size is 16 bits.
 **********************************************************************/
void get_digit (int *opa,  /* long integer LSD pointer */
                int index,  /* 16-bit digit index (LSD = 0) */
                int *value) /* 16-bit digit value */
{
  unsigned int word;

  word = *(opa + (index >> 1));
  *value = (index & 0x1) ? (word >> 16) : (word & 0xffff);
  if (index < 0) *value = 0;
}

void set_digit (int *opa,  // long integer LSD pointer
                int index,  // 16-bit digit index (LSD = 0)
                int value)  // 16-bit digit value
{
  int word;

  value &= 0xffff;
  word = *(opa + (index >> 1));
  word &= (index & 0x1) ? 0xffff : 0xffff0000;
  word |= (index & 0x1) ? (value<<16) : value;
  *(opa + (index >> 1)) = word;
}

q_status_t q_fdivrem (q_lip_ctx_t *ctx,
                      q_lint_t *opa,
                      q_lint_t *opn,
                      q_lint_t *quotient,
                      q_lint_t *remainder)
{
  q_status_t status = Q_SUCCESS;

  int dcnt;
  int opn_msd0, opn_msd1;
  int opa_msd0, opa_msd1, opa_msd2;
  int opn_norm_factor;
  int mod_size;
  int q_hat, r_hat;
  int i;

  q_lint_t tmp_lint1;
  q_lint_t tmp_lint2;
  q_lint_t norm_opa;
  q_lint_t norm_opn;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  status = q_init  (ctx, &tmp_lint1, 2 + opa->alloc);
  status += q_init (ctx, &tmp_lint2, 2 + opa->alloc);
  status += q_init (ctx, &norm_opa, 2 + opa->alloc);
  status += q_init (ctx, &norm_opn, 2 + opn->alloc);

  if (status != Q_SUCCESS) goto Q_FDIVREM_EXIT;

  /* D1: Normalize */
  dcnt = (opn->size << 1) - 1;
  for (; dcnt>=0; dcnt--) {
    get_digit ((int *)opn->limb, dcnt, &opn_msd0);
    if (opn_msd0 == 0) continue;
    opn_norm_factor = opn_msd0;
    for (i=0; ! (opn_norm_factor & 0x8000); i++) {
      opn_norm_factor <<= 1;
    }
    opn_norm_factor = i;
    mod_size = dcnt;
    break;
  }

  tmp_lint1.limb[0] = (1<<opn_norm_factor);
  tmp_lint1.size = 1;
  q_mul_sw (ctx, &norm_opa, opa, &tmp_lint1);
  q_mul_sw (ctx, &norm_opn, opn, &tmp_lint1);
  get_digit ((int *)norm_opn.limb, mod_size, &opn_msd0);
  get_digit ((int *)norm_opn.limb, mod_size-1, &opn_msd1);
  if (remainder != NULL) remainder->size = opn->size;

  /* D2: Initialize dcnt */
  dcnt = (norm_opa.size << 1) - 1;
  if (dcnt <= mod_size) dcnt = mod_size + 1;
  for (; dcnt>mod_size+1; dcnt--) {
    get_digit ((int *)opa->limb, dcnt, &opa_msd0);
    if (opa_msd0 == 0) continue;
    break;
  }
  dcnt++;
  if (quotient != NULL) {
    quotient->size = ((dcnt - mod_size + 1) >> 1);
    for (i=0; i<quotient->alloc; i++) {
      *(quotient->limb + i) = 0;
    }
  }
  if (remainder != NULL) {
    for (i=0; i<remainder->alloc; i++) {
      *(remainder->limb+i) = 0;
    }
    remainder->size = opn->size;
  }

  /* D7: Loop on dcnt */
  for (; dcnt>mod_size; dcnt--) {

    /* D3: Calculate estimated quotient q_hat */
    get_digit ((int *)norm_opa.limb, dcnt, &opa_msd0);
    get_digit ((int *)norm_opa.limb, dcnt-1, &opa_msd1);
    get_digit ((int *)norm_opa.limb, dcnt-2, &opa_msd2);

    q_hat = ((unsigned) ((opa_msd0<<16) + opa_msd1)) / opn_msd0;
    r_hat = ((unsigned) ((opa_msd0<<16) + opa_msd1)) - q_hat * opn_msd0;
    for (; r_hat < (1<<16); ) {
      if ((q_hat == (1<<16)) ||
           ((unsigned) (q_hat * opn_msd1) > (unsigned) ((r_hat<<16) + opa_msd2))) {
        q_hat--;
        r_hat += opn_msd0;
      } else {
        break;
      }
    }

    /* D4: Multiply and subtract */
    tmp_lint1.limb[0] = q_hat;
    tmp_lint1.size = 1;
    q_mul_sw (ctx, &tmp_lint2, &norm_opn, &tmp_lint1);
    q_mul_2pn (&tmp_lint1, &tmp_lint2, (dcnt - mod_size - 1)<<4);
    q_sub (&norm_opa, &norm_opa, &tmp_lint1);

    /* D5: Test remainder */
    if (norm_opa.neg) {

      /* D6: Add back */
      q_hat--;
      q_mul_2pn (&tmp_lint2, &norm_opn, (dcnt - mod_size - 1)<<4);
      q_add (&norm_opa, &norm_opa, &tmp_lint2);
    }
    if (quotient != NULL)
      set_digit ((int *)quotient->limb, dcnt-mod_size-1, q_hat);
  }

  /* D8: Unnormalize */
  if (remainder != NULL)
    q_div_2pn (remainder, &norm_opa, opn_norm_factor);

  q_free (ctx, &norm_opn);
  q_free (ctx, &norm_opa);
  q_free (ctx, &tmp_lint2);
  q_free (ctx, &tmp_lint1);

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_FDIVREM_EXIT:
  return (ctx->status);
}
