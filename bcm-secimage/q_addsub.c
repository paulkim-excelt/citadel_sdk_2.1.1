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
*   q_addsub.c
*
* \brief
*   This file contains routines for signed and unsigned add, subtract,
* modulo add and modulo subtract.
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

#ifdef QLIP_USE_PKE_HW
#include "q_pke_hw.h"
#endif

#ifdef QLIP_USE_PKA_HW
#include "q_pka_hw.h"
#endif

/**********************************************************************
 * q_asm_addc ()
 *
 * Description: inline primitive functions
 *
 **********************************************************************/
__inline uint32_t q_asm_addc(uint32_t al, uint32_t bl, uint32_t *carry)
{
#ifdef TARGET_ARM1136
  uint32_t ret;
	__asm {
		MOV   R1, #0;
		ADDS  R0, al, bl;
		ORRCS R1, R1, #1;
		ADDS  ret, R0, (*carry);
		ORRCS R1, R1, #1;
		MOV   (*carry), R1;
	}
	return(ret);
#else
  uint32_t ret;
  ret = al + bl + *carry;
  *carry = (al > (UINT_MASK - bl)) || ((al == (UINT_MASK - bl)) && *carry);
  return(ret);
#endif
}

/**********************************************************************
 * q_uadd ()
 *
 * Description: unsigned z = a + b
 *
 * Notes: without dynamic allocation, the sum z must always be
 * allocated to one limb larger than the max_size(a, b)
 **********************************************************************/

q_status_t q_uadd (q_lint_t *z,       /* q_lint pointer to result z */
                   q_lint_t *a,       /* q_lint pointer to source a */
                   q_lint_t *b)    /* q_lint pointer to source b */
{
  q_status_t status = Q_SUCCESS;
  int i;
  uint32_t carry;
  q_limb_ptr_t zp;
  q_limb_ptr_t ap, bp;
  uint32_t size_s, size_l;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  /* need normalization here ? */
  size_s = (a->size > b->size) ? b->size : a->size;
  size_l = (a->size > b->size) ? a->size : b->size;

  if (z->alloc < size_l + 1) {
#ifdef QLIP_DEBUG
    q_abort(__FILE__, __LINE__, "QLIP: q_uadd insufficient memory for the result!");
#endif

    status = Q_ERR_DST_OVERFLOW;
    goto Q_UADD_EXIT;
  }

  ap = (a->size > b->size) ? a->limb : b->limb;
  bp = (a->size > b->size) ? b->limb : a->limb;
  zp = z->limb;
  carry = 0;
  for (i=0; i<size_s; i++) {
    *zp = q_asm_addc(*ap, *bp, &carry);
    zp++; ap++; bp++;
  }

  /* skip optimization of direct copy when carry = 0 */
  for (i=size_s; i<size_l; i++) {
    *zp = q_asm_addc(*ap, 0, &carry);
    zp++; ap++; bp++;
  }

  *zp = carry;
  z->size = carry ? size_l + 1 : size_l;
  z->neg = a->neg;

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_UADD_EXIT:
  return (status);
}

/**********************************************************************
 * q_add ()
 *
 * Description: signed z = a+b
 **********************************************************************/

q_status_t q_add (q_lint_t *z,        /* q_lint pointer to result z */
                  q_lint_t *a,        /* q_lint pointer to source a */
                  q_lint_t *b)     /* q_lint pointer to source b */
{
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  /*  a +  b  a+b
   *  a + -b  a-b
   * -a +  b  b-a
   * -a + -b  -(a+b)
   */

  if (a->neg ^ b->neg) {
    if (q_ucmp(a, b) > 0) {
      status = q_usub(z, a, b);
      z->neg = a->neg;
    } else {
      status = q_usub(z, b, a);
      z->neg = b->neg;
    }
    goto Q_ADD_EXIT;
  }

  status = q_uadd(z, a, b);
  z->neg = a->neg;

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_ADD_EXIT:
  return (status);
}

/**********************************************************************
 * q_usub ()
 *
 * Description: unsigned z = a - b
 *
 * Note: without dynamic allocation, the sum z must always be
 * allocated to the max_size(a, b)
 **********************************************************************/

q_status_t q_usub (q_lint_t *z,     /* q_lint pointer to result z */
                   q_lint_t *a,     /* q_lint pointer to source a */
                   q_lint_t *b)    /* q_lint pointer to source b */
{
  q_status_t status = Q_SUCCESS;
  int i;
  uint32_t carry;
  q_limb_ptr_t zp;
  q_limb_ptr_t ap, bp;
  q_limb_t tmp;
  uint32_t size_a, size_b;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  size_a = a->size;
  size_b = b->size;

  if (z->alloc < size_a || size_a < size_b) {
#ifdef QLIP_DEBUG
    q_abort(__FILE__, __LINE__, "QLIP: q_usub result incorrect size!");
#endif

    status = Q_ERR_DST_OVERFLOW;
    goto Q_USUB_EXIT;
  }

  ap = a->limb;
  bp = b->limb;
  zp = z->limb;

  carry = 1;

  for (i=0; i<size_b; i++) {
    tmp = *bp;
    tmp = ~(tmp);
    *zp = q_asm_addc(*ap, tmp, &carry);
    zp++; ap++; bp++;
  }

  for (i=size_b; i<size_a; i++) {
    *zp = q_asm_addc(*ap, UINT_MASK, &carry);
    zp++; ap++;
  }

  z->size = size_a;
  z->neg = a->neg;
  Q_NORMALIZE(z->limb, z->size);

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_USUB_EXIT:
  return (status);
}

/**********************************************************************
 * q_sub ()
 *
 * Description: signed z = a + b
 **********************************************************************/

q_status_t q_sub (q_lint_t *z,        /* q_lint pointer to result z */
                  q_lint_t *a,        /* q_lint pointer to source a */
                  q_lint_t *b)     /* q_lint pointer to source b */
{
  q_status_t status = Q_SUCCESS;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  /*  a -  b  a-b
   *  a - -b  a+b
   * -a -  b  -a-b
   * -a - -b  -a+b
   */

  /* -a-b or a+b */
  if (a->neg ^ b->neg) {
    status = q_uadd(z, a, b);
    z->neg = a->neg;
    goto Q_SUB_EXIT;
  }

  /* -a+b or a-b */
  if (q_ucmp(a, b) > 0) {
    status = q_usub(z, a, b);
    z->neg = a->neg;
  } else if (q_ucmp(a, b) < 0) {
    status = q_usub(z, b, a);
    z->neg = !a->neg;
  } else {
    q_SetZero(z);
  }

 Q_SUB_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (status);
}

/**********************************************************************
 * q_ucmp ()
 *
 * Description: unsigned comparison
 **********************************************************************/

int q_ucmp (q_lint_t *a,   /* q_lint pointer to source a */
            q_lint_t *b)   /* q_lint pointer to source b */
{
  int result = 0;
  int i;
  q_limb_ptr_t ap, bp;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  result = a->size - b->size;
  if (result != 0) goto Q_UCMP_EXIT;

  ap = a->limb;
  bp = b->limb;

  for (i=a->size-1; i>=0; i--) {
    if (ap[i] != bp[i]) {
      result = (ap[i] > bp[i]) ? 1 : -1;
      goto Q_UCMP_EXIT;
    }
  }

 Q_UCMP_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (result);
}

/**********************************************************************
 * q_cmp ()
 *
 * Description: signed comparison
 **********************************************************************/

int q_cmp (q_lint_t *a,    /* q_lint pointer to source a */
		   q_lint_t *b)    /* q_lint pointer to source b */
{
  int result = 0;
  int i;
  q_limb_ptr_t ap, bp;
  int res;

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  if (a->neg ^ b->neg) {
    if (a->neg) {
      result = -1;
      goto Q_CMP_EXIT;
    } else {
      result = 1;
      goto Q_CMP_EXIT;
    }
  }

  res = a->size - b->size;
  if (res != 0) {
    result =  res ^ a->neg;
    goto Q_CMP_EXIT;
  }

  ap = a->limb;
  bp = b->limb;
  for (i=a->size-1; i>=0; i--) {
    if (ap[i] != bp[i]) {
      if (a->neg) {
        result = (ap[i] > bp[i]) ? -1 : 1;
        goto Q_CMP_EXIT;
      } else {
        result = (ap[i] > bp[i]) ? 1 : -1;
        goto Q_CMP_EXIT;
      }
    }
  }

 Q_CMP_EXIT:

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

  return (result);
}

/**********************************************************************
 * q_modadd ()
 *
 * Description: Modolo addition z = (a + b) % n
 *
 * Assumptions:
 *    (1) 0 <= a < n, 0 <= b < n
 *    (2) a, b < 2 ^ 4096
 **********************************************************************/

q_status_t q_modadd (q_lip_ctx_t *ctx, /* QLIP context pointer */
                     q_lint_t *z,      /* q_lint pointer to result z */
                     q_lint_t *a,      /* q_lint pointer to source a */
                     q_lint_t *b,      /* q_lint pointer to source b */
                     q_lint_t *n)      /* q_lint pointer to modulus n */
{
#ifdef QLIP_USE_PKE_HW
  q_lint_t ta, tb, tn;
#endif

#ifdef QLIP_USE_PKA_HW
  volatile uint32_t pka_status;
  uint32_t LIR_n;
  opcode_t sequence[5];
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_USE_PKE_HW
  q_init(ctx, &ta, n->alloc);
  q_init(ctx, &tb, n->alloc);
  q_init(ctx, &tn, n->alloc);
  q_copy(&ta, a);
  q_copy(&tb, b);
  q_copy(&tn, n);
  q_modadd_hw(z->limb, n->size*MACHINE_WD, tn.limb, ta.limb, tb.limb);
  z->size = n->size;
  Q_NORMALIZE(z->limb, z->size);
  q_free(ctx, &tn);
  q_free(ctx, &tb);
  q_free(ctx, &ta);
#else /* QLIP_USE_PKE_HW */
#ifdef QLIP_USE_PKA_HW
  if (z->alloc < n->size) {
    ctx->status = Q_ERR_DST_OVERFLOW;
    goto Q_MODADD_EXIT;
  }

  z->size = n->size;
  LIR_n = PKA_LIR_J;

  /* load modulus n to x[0] */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 0), n->size);
  sequence[0].ptr = n->limb;

  /* load a to x[1] */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 1), a->size);
  sequence[1].ptr = a->limb;

  /* load b to x[2] */
  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 2), b->size);
  sequence[2].ptr = b->limb;

  /* x[1] = x[1] + x[2] mod x[0] */
  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MODADD, PKA_LIR (LIR_n, 1), PKA_LIR (LIR_n, 1));
  sequence[3].op2 = PACK_OP2 (PKA_LIR (LIR_n, 2), PKA_LIR (LIR_n, 0));
  sequence[3].ptr = NULL;

  /* read back result x[1] */
  sequence[4].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_n, 1), n->size);
  sequence[4].ptr = NULL;

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
    if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MODADD_EXIT;
  }
  q_pka_hw_write_sequence (5, sequence);

  /* read result back */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {

    /*
      While we wait for the the first unload opcode, we want to
      monitor the CMD_ERR bit in the status register, as the math
      opcodes before the first unload opcode may trigger PKA HW
      related error. We do not need to monitor the CMD_ERR for the
      subsequent unload opcode.
    */

    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_MODADD_EXIT;
    } else {
      if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MODADD_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);
  q_pka_hw_read_LIR (n->size, z->limb);

#else /* QLIP_USE_PKA_HW */

  if (ctx->status = q_add(z, a, b)) goto Q_MODADD_EXIT;
  q_mod(ctx, z, z, n);

#endif  /* QLIP_USE_PKA_HW */
#endif  /* QLIP_USE_PKE_HW */

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_MODADD_EXIT:
  return (ctx->status);
}

/**********************************************************************
 * q_modsub ()
 *
 * Description: Modulo subtraction z = (a - b) % n
 *
 * Assumptions:
 *  (1) 0 <= a < n, 0 <= b < n
 *  (2) a, b < 2 ^ 4096
 **********************************************************************/

q_status_t q_modsub (q_lip_ctx_t *ctx, /* QLIP context pointer */
                     q_lint_t *z,      /* q_lint pointer to result z */
                     q_lint_t *a,      /* q_lint pointer to source a */
                     q_lint_t *b,      /* q_lint pointer to source b */
                     q_lint_t *n)      /* q_lint pointer to modulus n */
{
#ifdef QLIP_USE_PKE_HW
  q_lint_t ta, tb, tn;
#endif

#ifdef QLIP_USE_PKA_HW
  volatile uint32_t pka_status;
  uint32_t LIR_n;
  opcode_t sequence[5];
#endif

#ifdef QLIP_DEBUG
  printf ("Enter function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

#ifdef QLIP_USE_PKE_HW
  q_init(ctx, &ta, n->alloc);
  q_init(ctx, &tb, n->alloc);
  q_init(ctx, &tn, n->alloc);
  q_copy(&ta, a);
  q_copy(&tb, b);
  q_copy(&tn, n);
  q_modsub_hw(z->limb, n->size*MACHINE_WD, tn.limb, ta.limb, tb.limb);
  z->size = n->size;
  Q_NORMALIZE(z->limb, z->size);
  q_free(ctx, &tn);
  q_free(ctx, &tb);
  q_free(ctx, &ta);
#else /* QLIP_USE_PKE_HW */
#ifdef QLIP_USE_PKA_HW
  if (z->alloc < n->size) {
    ctx->status = Q_ERR_DST_OVERFLOW;
    goto Q_MODSUB_EXIT;
  }

  z->size = n->size;
  LIR_n = PKA_LIR_J;

  /* load modulus n to x[0] */
  sequence[0].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 0), n->size);
  sequence[0].ptr = n->limb;

  /* load a to x[1] */
  sequence[1].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 1), a->size);
  sequence[1].ptr = a->limb;

  /* load b to x[2] */
  sequence[2].op1 = PACK_OP1 (0, PKA_OP_MTLIRI, PKA_LIR (LIR_n, 2), b->size);
  sequence[2].ptr = b->limb;

  /* x[1] = x[1] - x[2] mod x[0] */
  sequence[3].op1 = PACK_OP1 (0, PKA_OP_MODSUB, PKA_LIR (LIR_n, 1), PKA_LIR (LIR_n, 1));
  sequence[3].op2 = PACK_OP2 (PKA_LIR (LIR_n, 2), PKA_LIR (LIR_n, 0));
  sequence[3].ptr = NULL;

  /* read back result x[1] */
  sequence[4].op1 = PACK_OP1 (PKA_EOS, PKA_OP_MFLIRI, PKA_LIR (LIR_n, 1), n->size);
  sequence[4].ptr = NULL;

  /* send sequnence */
  while (q_pka_hw_rd_status () & PKA_STAT_BUSY) {
    if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MODSUB_EXIT;
  }
  q_pka_hw_write_sequence (5, sequence);

  /* read result back */
  while (! ((pka_status = q_pka_hw_rd_status ()) & PKA_STAT_DONE)) {

    /*
      While we wait for the the first unload opcode, we want to
      monitor the CMD_ERR bit in the status register, as the math
      opcodes before the first unload opcode may trigger PKA HW
      related error. We do not need to monitor the CMD_ERR for the
      subsequent unload opcode.
    */

    if (pka_status & PKA_STAT_ERROR) {
      ctx->status = Q_ERR_PKA_HW_ERR;
      goto Q_MODSUB_EXIT;
    } else {
      if ((ctx->status = ctx->q_yield ()) != Q_SUCCESS) goto Q_MODSUB_EXIT;
    }
  }
  q_pka_hw_wr_status (pka_status);
  q_pka_hw_read_LIR (n->size, z->limb);

#else /* QLIP_USE_PKA_HW */
  if (ctx->status = q_sub(z, a, b)) goto Q_MODSUB_EXIT;
  if (q_mod(ctx, z, z, n)) goto Q_MODSUB_EXIT;
  if (z->neg) ctx->status = q_add(z, z, n);
#endif /* QLIP_USE_PKA_HW */
#endif /* QLIP_USE_PKE_HW */

#ifdef QLIP_DEBUG
  printf ("Exit function %s()\n", __FUNCTION__);
#endif /* QLIP_DEBUG */

 Q_MODSUB_EXIT:
  return (ctx->status);
}
