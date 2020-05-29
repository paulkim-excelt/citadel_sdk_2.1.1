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
*   q_lip_utils.c
*
* \brief
*   This file contains 
*     - the print routine for debug
*     - template abort routine for error handling and debug
*     - QLIP context initialization
*     - default yield function
*     - data import function
*     - data export function
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

#include <stdlib.h>
#include <stdio.h>
#include "q_lip.h"
#include "q_lip_aux.h"
#include "q_lip_utils.h"

#ifdef QLIP_USE_PKA_HW
  #include "q_pka_hw.h"
#endif

/**********************************************************************
 * q_ctx_init()
 *
 * QLIP context initialization routine
 *
 * Description: This routine resets hardware if applicable, and then
 * initializes the QLIP context data structure.  The context data
 * structure contains the following information: context memory base
 * pointer and size, yield function pointer, and current unused memory
 * pointer.
 *
 **********************************************************************/

q_status_t q_ctx_init (q_lip_ctx_t *ctx,         /* QLIP context pointer */
                       uint32_t *ctxDataMemPtr,  /* QLIP data memory pointer */
                       uint32_t ctxDataMemSize,  /* QLIP data memory size (in words) */
                       q_status_t (*q_yield)())  /* QLIP yield function pointer */
{
  int i;

  /* Initialize QLIP context memory upper bound */
  ctx->CurMemLmt  = (uint32_t) ctxDataMemSize;

  /* Initialize pointer to the start of the context data memory */
  ctx->CtxMem     = (uint32_t*) ctxDataMemPtr;
  ctx->CurMemPtr  = 0;

  /* Initialize funtion pointer for yeild function */
  if (q_yield == NULL) {
    ctx->q_yield  = q_yield_default;
  } else {
    ctx->q_yield  = q_yield;
  }

  /* Check context memory size */
  if (ctxDataMemSize < MIN_CTX_SIZE) {
    ctx->status = Q_ERR_CTX_MEM_SIZE_LOW;
  } else {
    ctx->status = Q_SUCCESS;
  }

/* add this one to disable SPA/DPA by default */
#ifdef QLIP_USE_COUNTER_SPADPA
  ctx->ESCAPE = 0;

  for(i=0; i<4; i++) {
    ctx->lfsr.l[i] = 0x5a5a5a5a;
  }
  ctx->lfsr.l[4] = 0x1;
#endif

/* reset hardware */
#ifdef QLIP_USE_PKA_HW
  q_pka_hw_rst();
#endif

  return (ctx->status);
}

/**********************************************************************
 * q_yield_default()
 *
 * QLIP default yield function.
 *
 * Description: This function is called when QLIP functions waits for
 * the hardware to complete a sequence of operations.  This default
 * yield function is only used when the application does not provide
 * its own yield function.
 **********************************************************************/

q_status_t q_yield_default ()
{
  /*
    This function is called when the QLIP is waiting for HW to finish
    a task. As part of the QLIP, this function simply returns 0.
  */

  return (Q_SUCCESS);
}

/**********************************************************************
 * q_import ()
 *
 * Description: This function imports data from an array into a q_lint
 * structure.  Both word order (BIG_NUM or normal) and endianness can
 * be specified.
 **********************************************************************/

q_status_t q_import (q_lip_ctx_t *ctx,   /* QLIP context pointer */
                     q_lint_t *z,        /* destination q_lint pointer */
                     q_size_t size,      /* data size (in word) */
                     int order,          /* import order: 1 = BIG_NUM, -1 = normal */
                     int endian,         /* endianness: 0 = native, 1 = big, -1 = little */
                     const void *data)   /* source data pointer */
{
  q_limb_ptr_t zp;

  z->alloc = size;
  z->limb = (q_limb_ptr_t) q_malloc(ctx, size);
  if (!z->limb) {
    #ifdef QLIP_DEBUG
    q_abort(__FILE__, __LINE__, "QLIP: Insufficient memory or zero memory allocation!");
    #endif

    ctx->status = Q_ERR_CTX_OVERFLOW;
    goto Q_IMPORT_EXIT;
  }

  z->size = size;
  z->neg = 0;

  if (endian == 0) {
    endian = HOST_ENDIAN;
  }

  zp = z->limb;

  if ((order == 1) && (endian == HOST_ENDIAN)) {
    QLIP_COPY(zp, (q_limb_ptr_t)data, size);
  }

  if ((order == 1) && (endian == - HOST_ENDIAN)) {
    QLIP_COPY_BSWAP(zp, (q_limb_ptr_t)data, size);
  }

  if ((order == -1) && (endian == HOST_ENDIAN)) {
    QLIP_RCOPY(zp, (q_limb_ptr_t)data, size);
  }

  if ((order == -1) && (endian == - HOST_ENDIAN)) {
    QLIP_RCOPY_BSWAP(zp, (q_limb_ptr_t)data, size);
  }

  Q_NORMALIZE (zp, (z->size));

 Q_IMPORT_EXIT:
  return (ctx->status);
}

/**********************************************************************
 * q_export ()
 *
 * Description: This function exports data from a q_lint structure to
 * an array.  Both word order (BIG_NUM or normal) and endianness can
 * be specified.
 **********************************************************************/

q_status_t q_export (void *data,          /* destination data array pointer */
                     int *size,           /* variable pointer of number of words exported */
                     int order,           /* export order: 1 = BIG_NUM, -1 = normal */
                     int endian,          /* endianness: 0 = native, 1 = big, -1 = little */
                     q_lint_t *a)         /* pointer to source q_lint */
{
  q_limb_ptr_t ap;

  ap = a->limb;
  *(size) = a->neg ? a->size * -1 : a->size;

  if (endian == 0) {
    endian = HOST_ENDIAN;
  }

  if ((order == 1) && (endian == HOST_ENDIAN)) {
    QLIP_COPY ((q_limb_ptr_t)data, ap, (a->size));
  }

  if ((order == 1) && (endian == - HOST_ENDIAN)) {
    QLIP_COPY_BSWAP ((q_limb_ptr_t)data, ap, (a->size));
  }

  if ((order == -1) && (endian == HOST_ENDIAN)) {
    QLIP_RCOPY ((q_limb_ptr_t)data, ap, (a->size));
  }

  if ((order == -1) && (endian == - HOST_ENDIAN)) {
    QLIP_RCOPY_BSWAP ((q_limb_ptr_t)data, ap, (a->size));
  }

  return (Q_SUCCESS);
}

#ifdef QLIP_DEBUG
/**********************************************************************
 * q_print ()
 *
 * Description: print utility for debug purpose.
 * Embedded system can use this function to redirect debug data to 
 * special debug console.
 **********************************************************************/
void q_print(const char* name,
             q_lint_t *z)
{
  q_limb_ptr_t zp;
  int i;

  zp = z->limb;
  if(z->neg) {
    printf("%s = -0x", name);
  } else {
    printf("%s = 0x", name);
  }

  if(!z->size) {
    printf("0");
  }

  /* print in normal format */
  for(i=z->size-1; i>=0; i--) {
	printf("%08X", zp[i]);
  }
  printf("\n");
}

/**********************************************************************
 * q_abort ()
 *
 * Description: Default abort handler.
 **********************************************************************/
void q_abort(const char *filename, const int line, const char *str) 
{
	fprintf(stderr, "ERROR at line %d in file %s.\n\t-->%s\n", line, filename, str);
}
#endif

#ifdef QLIP_USE_COUNTER_SPADPA
/**********************************************************************
 * q_cfg_counter_spadpa ()
 *
 * Description: This function enables/disables SPA/DPA counter measure.
 * When PKA hardware is used, the configuration is written into PKA Control
 * and Status Register ESCAPE field bit range [18:16].
 * The ESCAPE field is encoded as the following:
 * 
 *   bit [16]    - (ESCAPE & 0x1): enabled modexp protection
 *   bit [18:17] - (ESCAPE & 0x6): enabled random pipeline stall
 *                 0x1: high stall, 0x2: medium stall; 0x3: low stall
 * This function also seeds the LFSR and install the get_random function.
 **********************************************************************/
q_status_t q_cfg_counter_spadpa (q_lip_ctx_t *ctx,   /* QLIP context pointer */
                                 uint32_t ESCAPE,    /* flags to configure SPA/DPA counter measure settings */
                                 uint32_t seed,      /* seed valud to LFSR */
                                 void (*q_get_random)(LFSR_S *lfsr, int32_t length, uint32_t *rnd)) /* get_random function pointer */
{
#ifdef QLIP_USE_PKA_HW
  uint32_t pka_status;
#endif

  ctx->ESCAPE = ESCAPE;

  if(seed != 0) {
    q_seed_lfsr(&ctx->lfsr, seed);
  }

  q_shift_lfsr(&ctx->lfsr, 253);

  if (q_get_random == NULL) {
    ctx->q_get_random  = q_get_random_default;
  } else {
    ctx->q_get_random  = q_get_random;
  }

#ifdef QLIP_USE_PKA_HW
  pka_status = q_pka_hw_rd_status ();
  pka_status |= ((ESCAPE & PKA_CTL_ESCAPE_MASK) << PKA_CTL_ESCAPE_LOC);
  q_pka_hw_wr_status (pka_status);
#endif
  
  return (Q_SUCCESS);
}

/**********************************************************************
 * q_seed_lfsr ()
 *
 * Description: This function seeds the LFSR.
 **********************************************************************/
void q_seed_lfsr(LFSR_S *lfsr, uint32_t seed) 
{
/*seed the MSW */
  if(lfsr->l[1] != seed) lfsr->l[1] ^= seed;
  if(lfsr->l[1] != seed) lfsr->l[3] ^= seed;
}

/**********************************************************************
 * q_shift_lfsr ()
 *
 * Description: This function shifts the LFSR by count.
 **********************************************************************/
void q_shift_lfsr(LFSR_S *lfsr, int32_t count)
{
  int i;
  uint32_t t;

#ifdef QLIP_DEBUG
  printf("......SHIFT LFSR BY: %d......\n", count);
#endif

//a 64-bit LFSR and a 65-bit LFSR
//polynomial
// lfsr0: x(64) + x(4) + x(3) + x + 1
// lfsr1: x(65) + x(18)+ 1

  for(i=0; i<count; i++) {
    t = ((lfsr->l[1] & 0x80000000) >> 31) ^ 
        ((lfsr->l[1] & 0x20000000) >> 29) ^ 
        ((lfsr->l[1] & 0x10000000) >> 28) ^ 
        (lfsr->l[0] & 0x1);
    lfsr->l[0] = (lfsr->l[0] >> 1) | ((lfsr->l[1] & 0x1) << 31);
    lfsr->l[1] = (lfsr->l[1] >> 1) | (t << 31);
  
    t = ((lfsr->l[3] & 0x00008000) >> 15) ^ (lfsr->l[2] & 0x1);
    lfsr->l[2] = (lfsr->l[2] >> 1) | ((lfsr->l[3] & 0x1) << 31);
    lfsr->l[3] = (lfsr->l[3] >> 1) | ((lfsr->l[4] & 0x1) << 31);
    lfsr->l[4] = t;
  }
}

/**********************************************************************
 * q_get_random_default ()
 *
 * Description: This function is the default function to generate pseudo
 *              random numbers from the LFSR.
 **********************************************************************/
void q_get_random_default(LFSR_S *lfsr, int32_t length, uint32_t *rnd)
{
  int i;

  for(i=0; i<length; i++) {
/* take random data from LFSR */
    rnd[i] = lfsr->l[0+i%4];

/*shift LFSR */
    if((i%4) == 0) {
      q_shift_lfsr(lfsr, 7);
    }
  }
}

#endif
