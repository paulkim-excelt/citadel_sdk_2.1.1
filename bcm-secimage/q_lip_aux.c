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
*   q_lip_aux.c
*
* \brief
*   This file contains data initialization, checking, context memory allocation
* and de-allocation functions.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 7/5/2006\n
*   Updated: 12/15/2007
*
* \version
*   0.2
*
******************************************************************
*/

#include <stdio.h>
#include "q_lip.h"
#include "q_lip_aux.h"

#ifdef QLIP_DEBUG
#include "q_lip_utils.h"
#endif

/**********************************************************************
 * q_init ()
 *
 * Description: This function initializes a q_lint structure. It
 * allocates the data memory in the QLIP context, clears the allocated
 * memory, and initializes the parameters.
 **********************************************************************/

q_status_t q_init (q_lip_ctx_t *ctx,    /* QLIP context pointer */
                   q_lint_t *z,      /* pointer to q_lint to be initialized */
                   q_size_t size)       /* maximum data word size */
{
  int i;

  z->neg   = 0;
  z->size  = 0;
  z->alloc = size;
  z->limb  = (q_limb_ptr_t) q_malloc (ctx, size);

  if (z->limb == NULL) {
    #ifdef QLIP_DEBUG
    q_abort(__FILE__, __LINE__, "QLIP: Insufficient memory or zero memory allocation!");
    #endif

    ctx->status = Q_ERR_CTX_OVERFLOW;
    goto Q_INIT_EXIT;
  }

  for (i=0; i<size; i++) {
    z->limb[i] = 0L;
  }

 Q_INIT_EXIT:
  return (ctx->status);
}

/**********************************************************************
 * q_copy ()
 *
 * Description: This function copies the data content from a source
 * q_lint structure to the destination q_lint structure.  The
 * destination q_lint structure does not have to have the same memory
 * capacity, but is expected to have enough memory space to hold the
 * data in the source q_lint structure.
 **********************************************************************/

q_status_t q_copy (q_lint_t *z,      /* destination q_lint pointer */
                   q_lint_t *a)   /* source q_lint pointer */
{
  q_status_t status = Q_SUCCESS;
  int i;
  q_limb_ptr_t ap, zp;

  if (z->alloc < a->size) {
    #ifdef QLIP_DEBUG
    q_abort (__FILE__, __LINE__, "QLIP: Copy target doesn't have sufficient memory!");
    #endif

    status = Q_ERR_DST_OVERFLOW;
    goto Q_COPY_EXIT;
  }

  ap = a->limb;
  zp = z->limb;

  for (i=0; i<a->size; i++) {
    zp[i] = ap[i];
  }
  z->size = a->size;
  z->neg = a->neg;

 Q_COPY_EXIT:
  return (status);
}

/**********************************************************************
 * q_malloc ()
 *
 * Description: This function allocates context memory for a long integer
 * data.
 **********************************************************************/
uint32_t *q_malloc (q_lip_ctx_t *ctx,   /* QLIP context pointer */
                    q_size_t size)      /* memory size in words */
{
  uint32_t *ret;

  if((ctx->CurMemPtr + size) > ctx->CurMemLmt) {
    ret = 0;
  } else if(!size) {
    ret = 0;
  } else {
    ret = (ctx->CtxMem + ctx->CurMemPtr);
    ctx->CurMemPtr += size;
#ifdef QLIP_DEBUG
    printf("%s, %d, Memory Allocation: Pointer is set to %d!\n", __FILE__, __LINE__, ctx->CurMemPtr);
#endif
  }

  return(ret);
}

/**********************************************************************
 * q_free ()
 *
 * Description: This function de-allocates context memory for a long integer
 * data.
 **********************************************************************/
q_status_t q_free (q_lip_ctx_t *ctx,   /* QLIP context pointer */
                   q_lint_t *z)     /* pointer to the q_lint to be freed */
{
  if(z->limb + z->alloc != ctx->CtxMem + ctx->CurMemPtr) {
#ifdef QLIP_DEBUG
    printf("%s, %d, Memory De-allocation: Out-of-order de-allocation error!\n", __FILE__, __LINE__);
#endif
    ctx->status = Q_ERR_MEM_DEALLOC;
    goto Q_FREE_EXIT;
  } else {
    ctx->CurMemPtr -= z->alloc;
#ifdef QLIP_DEBUG
    printf("%s, %d, Memory De-allocation: Pointer is set to %d!\n", __FILE__, __LINE__, ctx->CurMemPtr);
#endif
  }
 Q_FREE_EXIT:
  return (ctx->status);
}

/* common functions */
/*inline*/ int q_IsZero(q_lint_t *a) {
	int i;
	q_limb_ptr_t ap;
	ap = a->limb;

	if(!a->size) { return(1); }

	for(i=0; i < (a->size); i++) {
		if(ap[i]) {
			return(0);
		} 	
	}	
	return(1);
}

/*inline*/ int q_IsOne(q_lint_t *a) {
	q_limb_ptr_t ap;
	ap = a->limb;

	if(a->size > 1) { return(0); }
	if(a->size == 0) { return(0); }

	return(a->limb[0] == 1L);
}

/*inline*/ void q_SetZero(q_lint_t *a) {
	int i;
	q_limb_ptr_t ap;
	ap = a->limb;
	a->size = 0;
	a->neg = 0;

	for(i=0; i < (a->alloc); i++) {
		ap[i] = 0;
	}	
}

/*defined for bignum*/
/*inline*/ void q_SetOne(q_lint_t *a) {
	int i;
	q_limb_ptr_t ap;
	ap = a->limb;
	a->size = 1;
	a->neg = 0;

	for(i=1; i < (a->alloc); i++) {
		ap[i] = 0;
	}	
	*ap = 1;
}

/*defined for bignum*/
/*inline*/ int q_IsOdd(q_lint_t *a) {
	q_limb_ptr_t ap;
	ap = a->limb;
	
	return((*ap) & 1);
}

/* leading one detection */
/* return value range is [0, MACHINE_WD-1] */
/*inline*/ int q_LeadingOne(uint32_t r) {
/* do a binary search to find the leading 1 */
  uint32_t last, mlast, bmask;
	int k, br;

  br = 0;
  k = MACHINE_WD/2;
  bmask = -1L << k;
	last = r;

  while(k) {
    mlast = last & bmask;
    if(mlast) {
      br += k;
      last = mlast;
    }
    k = k >> 1;
    bmask ^= (bmask >> k);
  }

	return(br);
}

