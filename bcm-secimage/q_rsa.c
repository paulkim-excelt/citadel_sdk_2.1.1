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
*   q_rsa.c
*
* \brief
*   This file contains the RSA encryption and CRT decryption routines.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 7/31/2006\n
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
#include "q_rsa.h"

#ifdef QLIP_DEBUG
#include "q_lip_utils.h"
#endif

#ifdef QLIP_USE_PKE_HW
#include "q_pke_hw.h"
#endif

#ifdef QLIP_USE_PKA_HW
#include "q_pka_hw.h"
#endif

#ifdef QLIP_USE_GMP
#include "gmp.h"
#endif

/**********************************************************************
 * q_rsa_enc ()
 *
 * Description: RSA encryption.
 *     c = m ^ e mod n
 **********************************************************************/

#ifndef REDUCED_RELEASE_CODE_SIZE
q_status_t q_rsa_enc (q_lip_ctx_t *ctx,
                      q_lint_t *c,
                      q_rsa_key_t *rsa,
                      q_lint_t *m)
{
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_USE_PKE_HW
  q_size_t blen_n, blen_e;
  blen_n = rsa->n.size * MACHINE_WD;
  blen_e = rsa->e.size * MACHINE_WD;

  c->size = rsa->n.size;
  q_rsa_enc_hw (c->limb, blen_n, blen_e, rsa->n.limb, rsa->e.limb, m->limb);
#else

#ifdef QLIP_USE_PKA_HW

  /* Note: PKA support modulus size up to 2Kb */
	int i;
  q_mont_t mont;
  volatile uint32_t pka_status;
  uint32_t LIR_n;
  opcode_t sequence[10];
	for(i=0; i<10; i++) {
		sequence[i].ptr = NULL;
	}

  /* initialize hardware first */
  q_pka_hw_rst ();

  /*need to initialize Montgomery context in FW*/
  status = q_init (ctx, &mont.n, rsa->n.alloc);
  status += q_init (ctx, &mont.np, (rsa->n.alloc+1));
  status += q_init (ctx, &mont.rr,  (rsa->n.alloc+1));
  if (status != Q_SUCCESS) goto Q_RSA_ENC_EXIT;

  if (q_mont_init (ctx, &mont, &rsa->n)) goto Q_RSA_ENC_EXIT;

  /* create command sequence */
  LIR_n = q_pka_sel_LIR (rsa->n.size);

  /* x[0] = m (message) */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 0), m->size);
  sequence[0].ptr = m->limb;

  /* x[1] = e */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 1), rsa->e.size);
  sequence[1].ptr = rsa->e.limb;

  /* x[2] = n.n */
  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 2), mont.n.size);
  sequence[2].ptr = mont.n.limb;

  /* x[3] = n.np */
  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 3), mont.np.size);
  sequence[3].ptr = mont.np.limb;

  /* x[4] = n.rr */
  sequence[4].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 4), mont.rr.size);
  sequence[4].ptr = mont.rr.limb;

  /* x[0] = x[0] * x[4] mod x[2] (convert to residue) */
  sequence[5].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_n, 0), PKA_LIR (LIR_n, 0));
  sequence[5].op2 = PACK_OP2 (PKA_LIR (LIR_n, 4), PKA_LIR (LIR_n, 2));

  /* x[4] = x[0] ^ x[1] mod x[2] (c = m^e mod n) */
  sequence[6].op1 = PACK_OP1 (0, PKA_OP_MODEXP, PKA_LIR (LIR_n, 4), PKA_LIR (LIR_n, 0));
  sequence[6].op2 = PACK_OP2 (PKA_LIR (LIR_n, 1), PKA_LIR (LIR_n, 2));

  /* convert back */
  /* x[0] = 1 */
  sequence[7].op1 = PACK_OP1 (0, PKA_OP_SLIR, PKA_LIR (LIR_n, 0), PKA_NULL);
  sequence[7].op2 = PACK_OP2 (PKA_NULL, 1);

  /* x[0] = x[0] * x[4] mod x[2] */
  sequence[8].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_n, 0), PKA_LIR (LIR_n, 0));
  sequence[8].op2 = PACK_OP2 (PKA_LIR (LIR_n, 4), PKA_LIR (LIR_n, 2));

  /* unload result x[0] */
  sequence[9].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_n, 0), rsa->n.size);

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
    if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_RSA_ENC_EXIT;
  }
  q_pka_hw_write_sequence (10, sequence);

  q_free (ctx, &mont.rr);
  q_free (ctx, &mont.np);
  q_free (ctx, &mont.n);

  c->size = rsa->n.size;

  /* read result back */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_ENC_EXIT;
    } else {
      if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_RSA_ENC_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);
  q_pka_hw_read_LIR (c->size, c->limb);

#else
#ifdef QLIP_USE_GMP
  mpz_t mc, mn, me, mm;
  mpz_init (mc);
  mpz_init (mn);
  mpz_init (me);
  mpz_init (mm);

  mpz_import (mn, rsa->n.size, -1, 4, 0, 0, rsa->n.limb);
  mpz_import (me, rsa->e.size, -1, 4, 0, 0, rsa->e.limb);
  mpz_import (mm, m->size, -1, 4, 0, 0, m->limb);

  mpz_powm (mc, mm, me, mn);
  mpz_export (c->limb, & (c->size), -1, 4, 0, 0, mc);
  mpz_clear (mc);
  mpz_clear (mn);
  mpz_clear (me);
  mpz_clear (mm);
#else
#ifdef QLIP_MOD_USE_MONT
  q_mont_t mont;
  q_lint_t mm;
  q_init (ctx, &mm, m->alloc);
  q_init (ctx, &mont.n, rsa->n.alloc);
  q_init (ctx, &mont.np, (rsa->n.alloc+1));
  q_init (ctx, &mont.rr,  (rsa->n.alloc+1));

  q_mont_init (ctx, &mont, &rsa->n);
  q_mont_mul (ctx, &mm, &mont.rr, m, &mont); /* convert to residue */
  q_modexp_mont (ctx, c, &mm, &rsa->e, &mont);
  q_SetOne (&mm);              /* convert back */
  q_mont_mul (ctx, c, c, &mm, &mont);

  q_free (ctx, &mont.rr);
  q_free (ctx, &mont.np);
  q_free (ctx, &mont.n);
  q_free (ctx, &mm);
#else
  q_modexp (ctx, c, m, &rsa->e, &rsa->n);

#endif /* QLIP_MOD_USE_MONT */
#endif /* QLIP_USE_GMP */
#endif /* QLIP_USE_PKA_HW */
#endif /* QLIP_USE_PKE_HW */

 Q_RSA_ENC_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}
#endif //REDUCED_RELEASE_CODE_SIZE

/**********************************************************************
 * q_rsa_crt ()
 *
 * Description: RSA CRT decryption.
 **********************************************************************/

q_status_t q_rsa_crt (q_lip_ctx_t *ctx,
                      q_lint_t *m,
                      q_rsa_crt_key_t *rsa,
                      q_lint_t *c)
{
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_USE_PKE_HW
  q_size_t blen_p, blen_q, blen_c;
#endif

#ifdef QLIP_USE_PKA_HW
	int i;
  q_mont_t mont1, mont2;
  volatile uint32_t pka_status;
  uint32_t LIR_p, LIR_c;
  opcode_t sequence[32];
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_USE_PKE_HW
  blen_p = rsa->p.size * MACHINE_WD;
  blen_q = rsa->q.size * MACHINE_WD;
  blen_c = c->size * MACHINE_WD;

  m->size = c->size;
  q_rsa_crt_hw (m->limb, blen_q, blen_p, rsa->q.limb, rsa->p.limb, rsa->dq.limb,
                rsa->dp.limb, rsa->qinv.limb, blen_c, c->limb);

#else
#ifdef QLIP_USE_PKA_HW
	for(i=0; i<32; i++) {
		sequence[i].ptr = NULL;
	}

  /* initialize hardware first */
  q_pka_hw_rst ();

  /*need to initialize Montgomery context in FW*/
  status = q_init (ctx, &mont1.n, rsa->p.alloc);
  status += q_init (ctx, &mont1.np, (rsa->p.alloc+1));
  status += q_init (ctx, &mont1.rr,  (rsa->p.alloc+1));

  status += q_init (ctx, &mont2.n, rsa->q.alloc);
  status += q_init (ctx, &mont2.np, (rsa->q.alloc+1));
  status += q_init (ctx, &mont2.rr,  (rsa->q.alloc+1));

  if (status != Q_SUCCESS) goto Q_RSA_CRT_EXIT;

  if (q_mont_init (ctx, &mont1, &rsa->p)) goto Q_RSA_CRT_EXIT;
  q_pka_hw_rst ();
  if (q_mont_init (ctx, &mont2, &rsa->q)) goto Q_RSA_CRT_EXIT;
  q_pka_hw_rst ();

  /* create command sequence */
  LIR_p = q_pka_sel_LIR ((rsa->p.size >= rsa->q.size) ? rsa->p.size : rsa->q.size);
  LIR_c = q_pka_sel_LIR (c->size);

  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_c, 0), c->size);
  sequence[0].ptr = c->limb;

  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont1.n.size);
  sequence[1].ptr = mont1.n.limb;

  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 3), mont1.np.size);
  sequence[2].ptr = mont1.np.limb;

  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 4), mont1.rr.size);
  sequence[3].ptr = mont1.rr.limb;

  sequence[4].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 5), mont2.n.size);
  sequence[4].ptr = mont2.n.limb;

  sequence[5].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 6), mont2.np.size);
  sequence[5].ptr = mont2.np.limb;

  sequence[6].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 7), mont2.rr.size);
  sequence[6].ptr = mont2.rr.limb;

  sequence[7].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 8), rsa->dp.size);
  sequence[7].ptr = rsa->dp.limb;

  sequence[8].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 9), rsa->dq.size);
  sequence[8].ptr = rsa->dq.limb;

  sequence[9].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 10), rsa->qinv.size);
  sequence[9].ptr = rsa->qinv.limb;

  sequence[10].op1 = PACK_OP1 (0, PKA_OP_SLIR, PKA_LIR (LIR_p, 13), PKA_NULL);
  sequence[10].op2 = PACK_OP2 (PKA_NULL, 1);

  sequence[11].op1 = PACK_OP1 (0, PKA_OP_MODREM, PKA_LIR (LIR_p, 11), PKA_LIR (LIR_c, 0)); /* c mod p */
  sequence[11].op2 = PACK_OP2 (PKA_NULL, PKA_LIR (LIR_p, 2));

  sequence[12].op1 = PACK_OP1 (0, PKA_OP_MODREM, PKA_LIR (LIR_p, 12), PKA_LIR (LIR_c, 0)); /* c mod q */
  sequence[12].op2 = PACK_OP2 (PKA_NULL, PKA_LIR (LIR_p, 5));

  /* convert to residue */
  sequence[13].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 11));
  sequence[13].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  sequence[14].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 12));
  sequence[14].op2 = PACK_OP2 (PKA_LIR (LIR_p, 7), PKA_LIR (LIR_p, 5));

  /* dummy */
  sequence[15].op1 = PACK_OP1 (0, PKA_OP_LMUL2N, PKA_LIR (LIR_p, 11), PKA_LIR (LIR_p, 13));
  sequence[15].op2 = PACK_OP2 (PKA_NULL, (rsa->q.size-1)*32);

  /* m2 = (c mod q)^dq mod q */
  sequence[16].op1 = PACK_OP1 (0, PKA_OP_MODEXP, PKA_LIR (LIR_p, 12), PKA_LIR (LIR_p, 1)); 
  sequence[16].op2 = PACK_OP2 (PKA_LIR (LIR_p, 9), PKA_LIR (LIR_p, 5));

  /* convert m2 from q-residue to p-residue */
  sequence[17].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 12));
  sequence[17].op2 = PACK_OP2 (PKA_LIR (LIR_p, 13), PKA_LIR (LIR_p, 5));

  /* convert qinv */
  sequence[18].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 9), PKA_LIR (LIR_p, 10));
  sequence[18].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  /* dummy */
  sequence[19].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 10), PKA_LIR (LIR_p, 11));
  sequence[19].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  sequence[20].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 10), PKA_LIR (LIR_p, 11));
  sequence[20].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  sequence[21].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 10), PKA_LIR (LIR_p, 11));
  sequence[21].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  /* continue in p-residue domain */
  sequence[22].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 12), PKA_LIR (LIR_p, 1));
  sequence[22].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  /* m1 = (c mod p)^dp mod p */
  sequence[23].op1 = PACK_OP1 (0, PKA_OP_MODEXP, PKA_LIR (LIR_p, 11), PKA_LIR (LIR_p, 0)); 
  sequence[23].op2 = PACK_OP2 (PKA_LIR (LIR_p, 8), PKA_LIR (LIR_p, 2));

  /* split multiply */
  sequence[24].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 8), PKA_LIR (LIR_p, 11)); /* m1*qinv mod p */
  sequence[24].op2 = PACK_OP2 (PKA_LIR (LIR_p, 9), PKA_LIR (LIR_p, 2));

  sequence[25].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 10), PKA_LIR (LIR_p, 12)); /* m2*qinv mod p */
  sequence[25].op2 = PACK_OP2 (PKA_LIR (LIR_p, 9), PKA_LIR (LIR_p, 2));

  /* split convert back */
  sequence[26].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 11), PKA_LIR (LIR_p, 8));
  sequence[26].op2 = PACK_OP2 (PKA_LIR (LIR_p, 13), PKA_LIR (LIR_p, 2));

  sequence[27].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 12), PKA_LIR (LIR_p, 10));
  sequence[27].op2 = PACK_OP2 (PKA_LIR (LIR_p, 13), PKA_LIR (LIR_p, 2));

  /* subtraction */
  sequence[28].op1 = PACK_OP1 (0, PKA_OP_MODSUB, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 11)); /* (m1-m2)*qinv mod p */
  sequence[28].op2 = PACK_OP2 (PKA_LIR (LIR_p, 12), PKA_LIR (LIR_p, 2));

  /* long multiply */
  sequence[29].op1 = PACK_OP1 (0, PKA_OP_LMUL, PKA_LIR (LIR_c, 4), PKA_LIR (LIR_p, 0));
  sequence[29].op2 = PACK_OP2 (PKA_LIR (LIR_p, 5), PKA_NULL);

  /* addition */
  sequence[30].op1 = PACK_OP1 (0, PKA_OP_LADD, PKA_LIR (LIR_c, 0), PKA_LIR (LIR_c, 4));
  sequence[30].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_NULL);

  sequence[31].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_c, 0), c->size);

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }

  q_pka_hw_write_sequence (32, sequence);

  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_EXIT;
    } else {
      if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_RSA_CRT_EXIT;
    }
  }

  q_free (ctx, &mont2.rr);
  q_free (ctx, &mont2.np);
  q_free (ctx, &mont2.n);

  q_free (ctx, &mont1.rr);
  q_free (ctx, &mont1.np);
  q_free (ctx, &mont1.n);

  m->size = c->size;

  /* read result back */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_EXIT;
    } 
  }
  q_pka_hw_wr_status (pka_status);
  q_pka_hw_read_LIR (m->size, m->limb);

#else
#ifdef QLIP_USE_GMP
  q_size_t size;

  mpz_t mp, mq, mdp, mdq, mqinv;
  mpz_t mc, mm;
  mpz_t mt1, mt2;

  mpz_init (mp);
  mpz_init (mq);
  mpz_init (mdp);
  mpz_init (mdq);
  mpz_init (mqinv);
  mpz_init (mc);
  mpz_init (mm);
  mpz_init (mt1);
  mpz_init (mt2);

  mpz_import (mp, rsa->p.size, -1, 4, 0, 0, rsa->p.limb);
  mpz_import (mq, rsa->q.size, -1, 4, 0, 0, rsa->q.limb);
  mpz_import (mdp, rsa->dp.size, -1, 4, 0, 0, rsa->dp.limb);
  mpz_import (mdq, rsa->dq.size, -1, 4, 0, 0, rsa->dq.limb);
  mpz_import (mqinv, rsa->qinv.size, -1, 4, 0, 0, rsa->qinv.limb);
  mpz_import (mc, c->size, -1, 4, 0, 0, c->limb);

  mpz_mod (mt1, mc, mp);
  mpz_mod (mt2, mc, mq);

  mpz_powm (mt1, mt1, mdp, mp);
  mpz_powm (mt2, mt2, mdq, mq);

  mpz_sub (mm, mt1, mt2);
  mpz_mul (mm, mm, mqinv);
  mpz_mod (mm, mm, mp);

  mpz_mul (mm, mm, mq);
  mpz_add (mm, mm, mt2);

  mpz_export (m->limb, &m->size, -1, 4, 0, 0, mm);

  mpz_clear (mp);
  mpz_clear (mq);
  mpz_clear (mdp);
  mpz_clear (mdq);
  mpz_clear (mqinv);
  mpz_clear (mc);
  mpz_clear (mm);
  mpz_clear (mt1);
  mpz_clear (mt2);

#else
#ifdef QLIP_MOD_USE_MONT
  q_mont_t mont1, mont2;
  q_lint_t m1, m2, m3;
  q_init (ctx, &m1, m->alloc+1);
  q_init (ctx, &m2, rsa->p.alloc);
  q_init (ctx, &m3, rsa->q.alloc);

  q_init (ctx, &mont1.n, rsa->p.alloc);
  q_init (ctx, &mont1.np, (rsa->p.alloc+1));
  q_init (ctx, &mont1.rr,  (rsa->p.alloc+1));

  q_init (ctx, &mont2.n, rsa->q.alloc);
  q_init (ctx, &mont2.np, (rsa->q.alloc+1));
  q_init (ctx, &mont2.rr,  (rsa->q.alloc+1));

  q_mont_init (ctx, &mont1, &rsa->p);
  q_mont_init (ctx, &mont2, &rsa->q);

  q_mod (ctx, &m2, c, &rsa->p);
  q_mod (ctx, &m3, c, &rsa->q);

  /* convert to residue domain */
  q_mont_mul (ctx, &m2, &mont1.rr, &m2, &mont1);
  q_mont_mul (ctx, &m3, &mont2.rr, &m3, &mont2);

  q_modexp_mont (ctx, &m1, &m2, &rsa->dp, &mont1);
  q_modexp_mont (ctx, &m2, &m3, &rsa->dq, &mont2);

  /* convert m2 from q-residue to p-residue */
  q_SetOne (&m3);
  q_mont_mul (ctx, &m2, &m2, &m3, &mont2);
  q_mont_mul (ctx, &m3, &m2, &mont1.rr, &mont1);

  q_modsub (ctx, &m1, &m1, &m3, &rsa->p);
  q_mont_mul (ctx, &m3, &mont1.rr, &rsa->qinv, &mont1);
  q_mont_mul (ctx, &m1, &m1, &m3, &mont1);

  q_SetOne (&m3);
  q_mont_mul (ctx, &m1, &m1, &m3, &mont1);

  q_mul (ctx, &m1, &m1, &rsa->q);
  q_uadd (&m1, &m1, &m2);
  q_copy (m, &m1);

  q_free (ctx, &mont2.rr);
  q_free (ctx, &mont2.np);
  q_free (ctx, &mont2.n);

  q_free (ctx, &mont1.rr);
  q_free (ctx, &mont1.np);
  q_free (ctx, &mont1.n);

  q_free (ctx, &m3);
  q_free (ctx, &m2);
  q_free (ctx, &m1);

#else
  q_lint_t m1, m2, m3;
  q_init (ctx, &m1, m->alloc+1);
  q_init (ctx, &m2, rsa->p.alloc);
  q_init (ctx, &m3, rsa->p.alloc);

  q_mod (ctx, &m2, c, &rsa->p);
  q_mod (ctx, &m3, c, &rsa->q);

  q_modexp (ctx, &m1, &m2, &rsa->dp, &rsa->p);
  q_modexp (ctx, &m2, &m3, &rsa->dq, &rsa->q);

  q_modsub (ctx, &m1, &m1, &m2, &rsa->p);

  q_modmul (ctx, &m1, &m1, &rsa->qinv, &rsa->p);
  q_mul (ctx, &m1, &m1, &rsa->q);
  q_uadd (&m1, &m1, &m2);
  q_copy (m, &m1);

  q_free (ctx, &m3);
  q_free (ctx, &m2);
  q_free (ctx, &m1);

#endif /*QLIP_MOD_USE_MONT*/
#endif /*QLIP_USE_GMP*/
#endif /*QLIP_USE_PKA_HW*/
#endif /*QLIP_USE_PKE_HW*/

 Q_RSA_CRT_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}
