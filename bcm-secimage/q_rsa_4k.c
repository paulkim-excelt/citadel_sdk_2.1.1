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
*   q_rsa_4k.c
*
* \brief
*   This file contains the RSA encryption and CRT decryption routines\n
*   for 3K and 4K key sizes.
*   
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 4/29/2008
*
* \version
*   0.1
*
******************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include "q_lip.h"
#include "q_lip_aux.h"
#include "q_rsa.h"
#include "q_rsa_4k.h"

#ifdef QLIP_DEBUG
#include "q_lip_utils.h"
#endif

#ifdef QLIP_USE_PKA_HW
#include "q_pka_hw.h"
#endif

/**********************************************************************
 * q_64b_mul ()
 * q_64b_add ()
 *
 * Description: inline primitive functions
 *
 **********************************************************************/

#define lo64(r) (((unsigned*) &r)[0])
#define hi64(r) (((unsigned*) &r)[1])
__inline static uint64_t q_64b_mul(uint64_t r, uint32_t a, uint32_t b)
{
#ifdef TARGET_ARM1136
  __asm {
    UMLAL lo64(r), hi64(r), a, b
  }
#else
  r += (uint64_t) a * (uint64_t) b;
#endif
  return r;
}

__inline static uint64_t q_64b_add(uint64_t a, uint64_t b)
{
#ifdef TARGET_ARM1136
  __asm {
    ADDS lo64(a), lo64(a), lo64(b);
    ADC hi64(a), hi64(a), hi64(b)
  }
#else
  a = a + b;
#endif
  return a;
}

/**********************************************************************
 * q_modexp_sw ()
 * z = a ^ e mod n
 *
 * Description: Modulo exponentiation.
 * Assumptions:
 *  1. all numbers are bignum
 *  2. no optimization based on bit length
 *  3. size (e) <= size (n) = size (a)
 **********************************************************************/

q_status_t q_modexp_sw (q_lip_ctx_t *ctx,
                     q_lint_t *z,
                     q_lint_t *a,
                     q_lint_t *e,
                     q_lint_t *n)
{
  q_status_t status = Q_SUCCESS;
  q_mont_t mont;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  status = q_init (ctx, &mont.n, n->alloc);
  status += q_init (ctx, &mont.np, (n->alloc+1));
  status += q_init (ctx, &mont.rr, (n->alloc+1));
  if (status != Q_SUCCESS) goto Q_MODEXP_SW_EXIT;

/* Step 1: Montgomery context initialization */
  q_mont_init_sw (ctx, &mont, n);

/* Step 2: Modexp using Montgomery method*/
  q_modexp_mont_sw (ctx, z, a, e, &mont);

  q_free (ctx, &mont.rr);
  q_free (ctx, &mont.np);
  q_free (ctx, &mont.n);

 Q_MODEXP_SW_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}

/**********************************************************************
 * q_mont_init_sw ()
 *
 * Description: Montgomery parameter initialization. This function
 * computes the following elements from the modulus n:
 *  np, rr, where
 *  r = 1 << (word_size (n) * 32),
 *  rr = r ^ 2 mod n,
 *  r * r_inv - n * np = 1.
 **********************************************************************/

q_status_t q_mont_init_sw (q_lip_ctx_t *ctx,
                        q_mont_t *mont,
                        q_lint_t *n)
{
  q_status_t status = Q_SUCCESS;
  int i;
  q_limb_ptr_t np;
  q_size_t ns;
  int br;
  q_lint_t c1;
  q_lint_t rr;
  uint32_t n0 ,np0;
  uint64_t z;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  np = n->limb;
  ns = n->size;
  if ((ctx->status = q_copy (&mont->n, n)) != Q_SUCCESS) goto Q_MONT_INIT_SW_EXIT;

  br = (n->size) << BITS_FOR_MACHINE_WD;
  mont->br = br;

  status += q_init (ctx, &c1, 1);
  status += q_init (ctx, &rr, (n->alloc*2+1));
  if (status != Q_SUCCESS) goto Q_MONT_INIT_SW_EXIT;
  q_SetOne (&c1);

#ifdef QLIP_MONT_WORD
  /* compute n0' = - n0 ^(-1) mod b - Dusse and Kaliski */
  z = (1L << 32);
  n0 = z - n->limb[0];
  np0 = 1;
  for(i=1; i<32; i++) {
     z = n0 * np0;
     if((z % (1 << (i+1))) >= (1 << i)) np0 += (1 << i);
  }

  /* mont->np */
  mont->np.size = 1;
  mont->np.limb[0] = np0;

#else
  /* R = 2^br */
  /* determine the size of R */
  /*
    PKA HW requires R be constructed as follows:
    R = 1 << (32*(n->size)),
    where n->size is number of 32-bit words of n.
  */

  rr.size = 1 + n->size;
  rr.limb[n->size] = 1L;

  /* R = R mod N */
  if (q_mod_sw (ctx, &rr, &rr, n)) goto Q_MONT_INIT_SW_EXIT;

  /* Ri = R^-1 mod N*/
/* the corresponding PKA call takes the first iteration of q_euclid outside of the main loop,
therefore the corresponding section of q_euclid need to be able to accept non-default last_x and x 
inputs. The firmware-only call doesn't use this method. */
//  if (q_modinv_sw(ctx, &mont->rr, &rr, n)) goto Q_MONT_INIT_SW_EXIT; // Changed - Rayees
  if (q_modinv(ctx, &mont->rr, &rr, n)) goto Q_MONT_INIT_SW_EXIT;

/* free and re-allocate */
/*
	q_free(ctx, &rr);
  status = q_init (ctx, &rr, (n->alloc*2+1));
  if (status != Q_SUCCESS) goto Q_MONT_INIT_SW_EXIT;
*/

  /* R * Ri - 1 */
  q_mul_2pn (&rr, &mont->rr, br);
  q_usub (&rr, &rr, &c1);

  /* Np = (R * Ri - 1) / N */
  if (q_div (ctx, &mont->np, &rr, n)) goto Q_MONT_INIT_SW_EXIT;

  /* need to make sure that np is zero-extended to the size of n */
  if (mont->np.size < mont->n.size) {
    for (i=mont->np.size; i<mont->n.size; i++) {
      mont->np.limb[i] = 0;
    }
    mont->np.size = mont->n.size;
  }
#endif

  /* mont->rr = R ^ 2 mod N */
  q_mul_2pn (&rr, &c1, mont->br*2);
  if (q_mod_sw (ctx, &mont->rr, &rr, n)) goto Q_MONT_INIT_SW_EXIT;

  /* need to make sure that rr is zero-extended to the size of n */
  if (mont->rr.size < mont->n.size) {
    for (i=mont->rr.size; i<mont->n.size; i++) {
      mont->rr.limb[i] = 0;
    }
    mont->rr.size = mont->n.size;
  }

  q_free (ctx, &rr);
  q_free (ctx, &c1);

Q_MONT_INIT_SW_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}

/**********************************************************************
 * q_mont_mul_sw ()
 *
 * Description: Montgomery Multiplication
 * Assumptions: a and b are already residue numbers
 **********************************************************************/

q_status_t q_mont_mul_sw (q_lip_ctx_t *ctx,
                       q_lint_t *z,
                       q_lint_t *a,
                       q_lint_t *b,
                       q_mont_t *mont)
{
  q_status_t status = Q_SUCCESS;
  q_lint_t t, m, u;
  q_size_t st;
  int i, j;
  uint64_t cs;
  uint32_t ta, tb, tm, carry;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_MONT_WORD
  status = q_init (ctx, &t, b->alloc+2);
  if (status != Q_SUCCESS) goto Q_MONT_MUL_SW_EXIT;

  /* optimized MonPro routine based on CIOS - Cetin Kaya Koc */
  for(i =0; i < mont->n.size; i++) {
     ta = (i >= a->size) ? 0 : a->limb[i];
     carry = 0;
     for(j=0; j < mont->n.size; j++) {
        tb = (j >= b->size) ? 0 : b->limb[j];
        cs = t.limb[j];
        cs = q_64b_mul ((uint64_t)t.limb[j], ta, tb);
        cs = q_64b_add (cs, (uint64_t)carry);
        t.limb[j] = (uint32_t) cs;
        carry = (uint32_t) (cs >> 32);
     }
     cs = q_64b_add ((uint64_t)t.limb[j], (uint64_t)carry);
     t.limb[j] = (uint32_t) cs;
     carry = (uint32_t) (cs >> 32);
     t.limb[j+1] = carry;

     carry = 0;
     tm = t.limb[0] * mont->np.limb[0];
     cs = q_64b_mul((uint64_t)t.limb[0], tm, mont->n.limb[0]);
     carry = (uint32_t) (cs >> 32);
     for(j=1; j<mont->n.size; j++) {
        cs = q_64b_mul ((uint64_t)t.limb[j], tm, mont->n.limb[j]);
        cs = q_64b_add (cs, (uint64_t)carry);
        t.limb[j-1] = (uint32_t) cs;
        carry = (uint32_t) (cs >> 32);
     }
     cs = q_64b_add ((uint64_t)t.limb[j], (uint64_t)carry);
     t.limb[j-1] = (uint32_t) cs;
     carry = (uint32_t) (cs >> 32);
     t.limb[j] = t.limb[j+1] + carry;
  }
  t.size = mont->n.size+1;
  Q_NORMALIZE (t.limb, t.size);
#else
  /* allocate temporary memory */
  st = a->alloc + b->alloc + mont->np.alloc;
  status = q_init (ctx, &t, st);
  status += q_init (ctx, &m, st);
  status += q_init (ctx, &u, st);
  if (status != Q_SUCCESS) goto Q_MONT_MUL_SW_EXIT;

  if (q_IsOne (a))  q_copy (&t, b);
  else if (q_IsOne (b)) q_copy (&t, a);
  else if (q_mul_sw (ctx, &t, a, b)) goto Q_MONT_MUL_SW_EXIT;

  q_mod_2pn (&u, &t, (uint32_t)mont->br);

  q_mul_sw (ctx, &m, &u, &mont->np);
  q_mod_2pn (&u, &m, (uint32_t)mont->br);

  q_mul_sw (ctx, &m, &u, &mont->n);
  q_uadd (&t, &t, &m);

  q_free (ctx, &u);
  q_free (ctx, &m);

  q_div_2pn (&t, &t, (uint32_t)mont->br);
  Q_NORMALIZE (t.limb, t.size);
#endif

  if ((z->alloc < t.size) && t.limb[t.size-1] != 1L) {
#ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: q_mont_mul insufficient memory for the result!");
#endif
    ctx->status = Q_ERR_DST_OVERFLOW;
    goto Q_MONT_MUL_SW_EXIT;
  }

  if (q_ucmp (&t, &mont->n) > 0) {
    q_usub (&t, &t, &mont->n);
  }

  if (q_copy (z, &t)) goto Q_MONT_MUL_SW_EXIT;
  q_free (ctx, &t);

 Q_MONT_MUL_SW_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}

/**********************************************************************
 * q_modexp_mont_sw ()
 *
 * Description: Montgomery modulo exponentiation
 * Assumptions: a is not a residue number. A pre and a post conversion step
 * is needed.
 **********************************************************************/

q_status_t q_modexp_mont_sw (q_lip_ctx_t *ctx,
                          q_lint_t *z,
                          q_lint_t *a,
                          q_lint_t *e,
                          q_mont_t *mont)
{
  int i, j;
  q_size_t es;
  q_limb_ptr_t zp, ep;
  q_lint_t mm;
  q_lint_t mx;
  int bitset; /* points to the current machine word */
  int msb_e;
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  status = q_init (ctx, &mm, a->alloc);
  status += q_init (ctx, &mx, a->alloc);
  if (status != Q_SUCCESS) goto Q_MODEXP_MONT_SW_EXIT;

  q_mont_mul_sw (ctx, &mm, &mont->rr, a, mont); /* convert input a to residue */

  if (q_IsZero (e)) {
    q_SetOne (z);
    goto Q_MODEXP_MONT_SW_EXIT;
  }

  if (!q_IsOdd (&mont->n)) {
#ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: modulus must be an odd number to perform Montgomery modexp!");
#endif
    ctx->status = Q_ERR_GENERIC;
    goto Q_MODEXP_MONT_SW_EXIT;
  }

  zp = z->limb;
  ep = e->limb;
  es = e->size;

  q_SetOne (&mx);
  if (q_mont_mul_sw (ctx, &mx, &mx, &mont->rr, mont)) goto Q_MODEXP_MONT_SW_EXIT;

  /* LR binary method */
  if(es == 1) {
    msb_e = q_LeadingOne(ep[0]);
    for(j=msb_e; j>=0; j--) {
        if (q_mont_mul_sw (ctx, &mx, &mx, &mx, mont)) goto Q_MODEXP_MONT_SW_EXIT;
        bitset = ep[0] & (1 << j);
        if (bitset) {
          if (q_mont_mul_sw (ctx, &mx, &mx, &mm, mont)) goto Q_MODEXP_MONT_SW_EXIT;
        }
    }
  } else {
    for (i=es-1; i >= 0; i--) {
      for (j=MACHINE_WD-1; j >= 0; j--) {
        if (q_mont_mul_sw (ctx, &mx, &mx, &mx, mont)) goto Q_MODEXP_MONT_SW_EXIT;
        bitset = ep[i] & (1 << j);
        if (bitset) {
          if (q_mont_mul_sw (ctx, &mx, &mx, &mm, mont)) goto Q_MODEXP_MONT_SW_EXIT;
        }
      }  
    } 
  }

  if (q_copy (z, &mx)) goto Q_MODEXP_MONT_SW_EXIT;
  q_free (ctx, &mx);
  Q_NORMALIZE (zp, z->size);

  q_SetOne (&mm);              /* convert result back */
  q_mont_mul_sw (ctx, z, z, &mm, mont);
  q_free (ctx, &mm);

Q_MODEXP_MONT_SW_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}

/**********************************************************************
 * q_rsa_crt_4k ()
 *
 * Description: RSA CRT decryption.
 **********************************************************************/

q_status_t q_rsa_crt_4k (q_lip_ctx_t *ctx,
                      q_lint_t *m,
                      q_rsa_crt_key_t *rsa,
                      q_lint_t *c)
{
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_USE_PKA_HW
	int i;
	q_lint_t m2;
  q_mont_t mont1, mont2;
  volatile uint32_t pka_status;
  uint32_t LIR_p, LIR_c;
  opcode_t sequence[12];
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_USE_PKA_HW
  /* initialize hardware first */
  q_pka_hw_rst ();

  /*need to initialize Montgomery context in FW*/
  status = q_init (ctx, &mont1.n, rsa->p.alloc);
  status += q_init (ctx, &mont1.np, (rsa->p.alloc+1));
  status += q_init (ctx, &mont1.rr,  (rsa->p.alloc+1));

  status += q_init (ctx, &mont2.n, rsa->q.alloc);
  status += q_init (ctx, &mont2.np, (rsa->q.alloc+1));
  status += q_init (ctx, &mont2.rr,  (rsa->q.alloc+1));
	status += q_init (ctx, &m2, rsa->p.alloc);

  if (status != Q_SUCCESS) goto Q_RSA_CRT_4K_EXIT;

  if (q_mont_init(ctx, &mont1, &rsa->p)) goto Q_RSA_CRT_4K_EXIT;
  q_pka_hw_rst ();

  if (q_mont_init(ctx, &mont2, &rsa->q)) goto Q_RSA_CRT_4K_EXIT;
  q_pka_hw_rst ();

	m2.size = rsa->p.size;

  /* create command sequence */
  LIR_p = q_pka_sel_LIR ((rsa->p.size >= rsa->q.size) ? rsa->p.size : rsa->q.size);
  LIR_c = q_pka_sel_LIR (c->size);

	/* compute m2 */
	for(i=0; i<3; i++) {
		sequence[i].ptr = NULL;
	}

  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_c, 0), c->size);
  sequence[0].ptr = c->limb;

  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont2.n.size);
  sequence[1].ptr = mont2.n.limb;

  /* c mod q */
  sequence[2].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MODREM, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_c, 0)); 
  sequence[2].op2 = PACK_OP2 (PKA_NULL, PKA_LIR (LIR_p, 2));

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (3, sequence);

	/* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    } 
  }
  q_pka_hw_wr_status (pka_status);

	for(i=0; i<9; i++) {
		sequence[i].ptr = NULL;
	}

	/* load exponent */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 1), rsa->dq.size);
  sequence[0].ptr = rsa->dq.limb;

	/* load mont */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont2.n.size);
  sequence[1].ptr = mont2.n.limb;

  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 3), mont2.np.size);
  sequence[2].ptr = mont2.np.limb;

  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 4), mont2.rr.size);
  sequence[3].ptr = mont2.rr.limb;

  /* convert to residue */
  sequence[4].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0));
  sequence[4].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

	/* m2 = (c mod q)^dq mod q */
  sequence[5].op1 = PACK_OP1 (0, PKA_OP_MODEXP, PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 0)); 
  sequence[5].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 2));

 /* convert m2 from q-residue to normal */
  sequence[6].op1 = PACK_OP1 (0, PKA_OP_SLIR, PKA_LIR (LIR_p, 0), PKA_NULL);
  sequence[6].op2 = PACK_OP2 (PKA_NULL, 1);

  sequence[7].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0));
  sequence[7].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

  sequence[8].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_p, 0), m2.size);

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (9, sequence);

	/* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    } 
  }
  q_pka_hw_wr_status (pka_status);

  /* read m2 back */
  q_pka_hw_read_LIR (m2.size, m2.limb);

	/* compute m1 */
	for(i=0; i<3; i++) {
		sequence[i].ptr = NULL;
	}

  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_c, 0), c->size);
  sequence[0].ptr = c->limb;

  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont1.n.size);
  sequence[1].ptr = mont1.n.limb;

  /* c mod p */
  sequence[2].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MODREM, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_c, 0)); 
  sequence[2].op2 = PACK_OP2 (PKA_NULL, PKA_LIR (LIR_p, 2));

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (3, sequence);

	/* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    } 
  }
  q_pka_hw_wr_status (pka_status);

	for(i=0; i<9; i++) {
		sequence[i].ptr = NULL;
	}

	/* load exponent */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 1), rsa->dp.size);
  sequence[0].ptr = rsa->dp.limb;

	/* load mont */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont1.n.size);
  sequence[1].ptr = mont1.n.limb;

  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 3), mont1.np.size);
  sequence[2].ptr = mont1.np.limb;

  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 4), mont1.rr.size);
  sequence[3].ptr = mont1.rr.limb;

  /* convert to residue */
  sequence[4].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0));
  sequence[4].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

	/* m1 = (c mod p)^dp mod p */
  sequence[5].op1 = PACK_OP1 (0, PKA_OP_MODEXP, PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 0)); 
  sequence[5].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 2));
  
  sequence[6].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MOVE, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 4));
  sequence[6].op2 = 0;

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (7, sequence);

	/* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    } 
  }
  q_pka_hw_wr_status (pka_status);

  /* continue in p-residue domain */
	for(i=0; i<12; i++) {
		sequence[i].ptr = NULL;
	}

	/* load m2 back in */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 1), m2.size);
  sequence[0].ptr = m2.limb;

  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), mont1.n.size);
  sequence[1].ptr = mont1.n.limb;

  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 3), mont1.np.size);
  sequence[2].ptr = mont1.np.limb;

  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 4), mont1.rr.size);
  sequence[3].ptr = mont1.rr.limb;

  sequence[4].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 5), rsa->qinv.size);
  sequence[4].ptr = rsa->qinv.limb;

  /* convert m2 from normal to p-residue */
  sequence[5].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 1));
  sequence[5].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

	/* (m1-m2) mod p */
  sequence[6].op1 = PACK_OP1 (0, PKA_OP_MODSUB, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0)); 
  sequence[6].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 2));

  /* convert qinv */
  sequence[7].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 5), PKA_LIR (LIR_p, 5));
  sequence[7].op2 = PACK_OP2 (PKA_LIR (LIR_p, 4), PKA_LIR (LIR_p, 2));

	/* (m1-m2)*qinv mod p */
  sequence[8].op1 = PACK_OP1 (0, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0)); 
  sequence[8].op2 = PACK_OP2 (PKA_LIR (LIR_p, 5), PKA_LIR (LIR_p, 2));

  /* convert back */
  sequence[9].op1 = PACK_OP1 (0, PKA_OP_SLIR, PKA_LIR (LIR_p, 1), PKA_NULL);
  sequence[9].op2 = PACK_OP2 (PKA_NULL, 1);

  sequence[10].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MODMUL, PKA_LIR (LIR_p, 0), PKA_LIR (LIR_p, 0));
  sequence[10].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_LIR (LIR_p, 2));

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (11, sequence);

	/* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);

  for(i=0; i<5; i++) {
    sequence[i].ptr = NULL;
  }

  /* load m2 back in */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 1), rsa->q.size);
  sequence[0].ptr = rsa->q.limb;

  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_p, 2), m2.size);
  sequence[1].ptr = m2.limb;

  /* long multiply ((m1-m2)*qinv mod p)*q */
  sequence[2].op1 = PACK_OP1 (0, PKA_OP_LMUL, PKA_LIR (LIR_c, 0), PKA_LIR (LIR_p, 0));
  sequence[2].op2 = PACK_OP2 (PKA_LIR (LIR_p, 1), PKA_NULL);

  /* addition */
  sequence[3].op1 = PACK_OP1 (0, PKA_OP_LADD, PKA_LIR (LIR_c, 0), PKA_LIR (LIR_c, 0));
  sequence[3].op2 = PACK_OP2 (PKA_LIR (LIR_p, 2), PKA_NULL);

  sequence[4].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_c, 0), c->size);

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
  }
  q_pka_hw_write_sequence (5, sequence);

  /* wait for completion */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {
    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_RSA_CRT_4K_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);

  /* read result back */
  m->size = c->size;
  q_pka_hw_read_LIR (m->size, m->limb);

	q_free (ctx, &m2);

  q_free (ctx, &mont2.rr);
  q_free (ctx, &mont2.np);
  q_free (ctx, &mont2.n);

  q_free (ctx, &mont1.rr);
  q_free (ctx, &mont1.np);
  q_free (ctx, &mont1.n);
#else
  q_rsa_crt(ctx, m, rsa, c);
#endif

 Q_RSA_CRT_4K_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s ()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (ctx->status);
}
