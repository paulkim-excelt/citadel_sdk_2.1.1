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
*   q_mod.c
*
* \brief
*   This file contains modulo remainder routines.
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

#ifdef QLIP_USE_PKA_HW
#include "q_pka_hw.h"
#endif

#ifdef QLIP_USE_GMP
#include "gmp.h"
#endif

/***********************************************************************
 * q_mod_2pn ()
 *
 * Description: z = a mod n, where n = 2^k
 **********************************************************************/

q_status_t q_mod_2pn (q_lint_t *z,
                      q_lint_t *a,
                      uint32_t bits)
{
  q_status_t status = Q_SUCCESS;
  int i;
  int words;
  q_limb_ptr_t zp, ap;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  words = (bits >> BITS_FOR_MACHINE_WD);

  if (a->size < words) {
    status = q_copy (z, a);
    goto Q_MOD_2PN_EXIT;
  }

  if (z->alloc < words + 1) {
#ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: mod_2pn insufficient memory for the result!");
#endif
    status = Q_ERR_DST_OVERFLOW;
    goto Q_MOD_2PN_EXIT;
  }

  z->size = words;
  zp = z->limb;
  ap = a->limb;

  /* the operation should still work when z == a */
  for (i=0; i< words; i++) {
    zp[i] = ap[i];
  }

  words = bits - (words << BITS_FOR_MACHINE_WD);

  if (words) {
    zp[i] = ap[i] & (UINT_MASK >> (MACHINE_WD-words));
    z->size++;
  }

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_MOD_2PN_EXIT:
  return (status);
}

#ifndef REDUCED_RELEASE_CODE_SIZE
/***********************************************************************
 * q_mod_partial ()
 * z = a mod n
 *
 * Description: This function is valid only when the MSB of a and n
 * are the same.
 **********************************************************************/

q_status_t q_mod_partial (q_lint_t *z,
                          q_lint_t *a,
                          q_lint_t *n)
{
  q_status_t status = Q_SUCCESS;
  int res;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  if (a->size != n->size) {
#ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: Simple mod size mismatch!");
#endif
    status = Q_ERR_DST_OVERFLOW;
    goto Q_MOD_PARTIAL_EXIT;
  }

  res = q_ucmp (a, n);

  /* if a > n, perform substraction */
  if (res) {
    status = q_usub (z, a, n);
  } else {
    status = q_copy (z, a);
  }

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_MOD_PARTIAL_EXIT:
  return (status);
}
#endif //REDUCED_RELEASE_CODE_SIZE

/***********************************************************************
 * q_mod ()
 *
 * Description:  z = a mod n
 **********************************************************************/

q_status_t q_mod (q_lip_ctx_t *ctx,
                  q_lint_t *z,
                  q_lint_t *a,
                  q_lint_t *n)
{
#ifdef QLIP_USE_GMP
  mpz_t ma, mn, mz;
#endif

#ifdef QLIP_USE_PKA_HW
  volatile uint32_t pka_status;
  opcode_t sequence[4];
  uint32_t LIR_n;
  uint32_t LIR_a;
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  /* ZQI NOTE: we can not can the legacy PKE q_modrem_hw function directly
     unless we know that the size of a is no bigger than the size of n. Since
     this function is suppose to be more generic, the hardware restriction is
     not always met. Therefore we don't put the code here to invoke the hardware call. */

#ifdef QLIP_USE_GMP
  mpz_init (ma);
  mpz_init (mn);
  mpz_init (mz);

  mpz_import (ma, a->size, -1, 4, 0, 0, a->limb);
  mpz_import (mn, n->size, -1, 4, 0, 0, n->limb);

  mpz_mod (mz, ma, mn);
  mpz_export (z->limb, & (z->size), -1, 4, 0, 0, mz);

  mpz_clear (ma);
  mpz_clear (mn);
  mpz_clear (mz);
#else /* QLIP_USE_GMP */
#ifdef QLIP_USE_PKA_HW
  /*
   * Limitation: n is limited to 2K bits
   *             a is limited to 4K bits
   */
  LIR_n = PKA_LIR_H;
  LIR_a = PKA_LIR_J;

  /* J[0] = a */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_a, 0), a->size);
  sequence[0].ptr = a->limb;

  /* H[2] = n */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 2), n->size);
  sequence[1].ptr = n->limb;

  /* H[0] = J[0] mod H[2] */
  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MODREM, PKA_LIR (LIR_n, 0), PKA_LIR (LIR_a, 0));
  sequence[2].op2 = PACK_OP2 (PKA_NULL, PKA_LIR (LIR_n, 2));
  sequence[2].ptr = NULL;

  /* unload H[0] */
  sequence[3].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_n, 0), n->size);
  sequence[3].ptr = NULL;

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
    if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MOD_EXIT;
  }
  q_pka_hw_write_sequence (4, sequence);

  /* read result back */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_MOD_EXIT;
    } else {
      if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MOD_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);
  q_pka_hw_read_LIR (n->size, z->limb);
  z->size = n->size;

#else /* QLIP_USE_PKA_HW */
  /* call Tao's function */
  int a_neg;
  a_neg = a->neg;
  a->neg = 0;

  q_fdivrem (ctx, a, n, NULL, z);
  a->neg = a_neg;

#endif /* QLIP_USE_PKA_HW */
#endif /* QLIP_USE_GMP */

 Q_MOD_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}

/***********************************************************************
 * q_mod_sw ()
 *
 * Description:  z = a mod n
 * 
 * create a software-only version for 4K RSA
 **********************************************************************/

q_status_t q_mod_sw (q_lip_ctx_t *ctx,
                  q_lint_t *z,
                  q_lint_t *a,
                  q_lint_t *n)
{
  int a_neg;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  a_neg = a->neg;
  a->neg = 0;

  q_fdivrem (ctx, a, n, NULL, z);
  a->neg = a_neg;

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}
