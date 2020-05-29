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
*   q_lip_utils.h
*
* \brief
*   header file for utility functions. Utility functions are for QLIP
* debugging or for data convertion to QLIP input format or from QLIP output
* format.
*
* \author
*   Charles Qi (x2-8439)
*
* \date
*   Originated: 12/15/2007
*
* \version
*   0.1
*
******************************************************************
*/

#ifndef _Q_LIP_UTILS_H_
#define _Q_LIP_UTILS_H_

#if ! defined(QLIP_COPY)
#define QLIP_COPY(dst, src, n)                    \
do {                                              \
  if ((n) != 0) {                                 \
    q_size_t __n = (n);                           \
    q_limb_ptr_t __dst = (dst);                   \
    q_limb_ptr_t __src = (src);                   \
    q_limb_t __x;                                 \
    if (__n != 0) {                               \
        do {                                      \
          __x = *__src++;                         \
          *__dst++ = __x;                         \
        }                                         \
        while (--__n);                            \
    }                                             \
  }                                               \
} while (0)
#endif

#if ! defined(QLIP_COPY_BSWAP)
#define QLIP_COPY_BSWAP(dst, src, n)              \
do {                                              \
  if ((n) != 0) {                                 \
    q_size_t __n = (n);                           \
    q_limb_ptr_t __dst = (dst);                   \
    q_limb_ptr_t __src = (src);                   \
    q_limb_t __x;                                 \
    if (__n != 0) {                               \
      do {                                        \
        __x = *__src++;                           \
        __x = BSWAP(__x);                         \
        *__dst++ = __x;                           \
      } while (--__n);                            \
    }                                             \
  }                                               \
} while (0)
#endif

#define QLIP_RCOPY(dst, src, size)                \
do {                                              \
  q_limb_ptr_t     __dst = (dst);                 \
  q_size_t    __size = (size);                    \
  q_limb_ptr_t  __src = (src) + __size - 1;       \
  q_size_t    __i;                                \
  for (__i = 0; __i < __size; __i++) {            \
    *__dst = *__src;                              \
    __dst++;                                      \
    __src--;                                      \
  }                                               \
} while (0)

#define QLIP_RCOPY_BSWAP(dst, src, size)          \
do {                                              \
  q_limb_ptr_t     __dst = (dst);                 \
  q_size_t    __size = (size);                    \
  q_limb_ptr_t  __src = (src) + __size - 1;       \
  q_size_t    __i;                                \
  for (__i = 0; __i < __size; __i++) {            \
    *__dst = *__src;                              \
    *__dst = BSWAP(*__dst);                       \
    __dst++;                                      \
    __src--;                                      \
  }                                               \
} while (0)

/* auxilary function declarations */
q_status_t q_ctx_init (q_lip_ctx_t *ctx,             /* QLIP context pointer */
                       uint32_t *ctxDataMemPtr,      /* QLIP data memory pointer */
                       uint32_t ctxDataMemSize,      /* QLIP data memory size (in words) */
                       q_status_t (*q_yield)());     /* QLIP yield function pointer */
/*!< QLIP context initialization
    \param ctx QLIP context pointer provided by higher level software
    \param ctxDataMemPtr QLIP context data memory pointer
    \param ctxDataMemSize QLIP context data memory size (in words)
    \param q_yield function pointer to a yield function provided by higher level software

    \note  QLIP relies on a piece of scratch memory being provided by the calling function.
*/

q_status_t q_yield_default ();                       /* Default yield function */
/*!< The default yield function
    \note  
		  QLIP is blocking. The yield function provides some flexibility to instrument non-blocking
      behavior by higher level software. When QLIP function enters hardware polling loop, the 
      yield function can be called.
*/

q_status_t q_import (q_lip_ctx_t *ctx, q_lint_t *z, q_size_t size, int order,
              int endian, const void *data);
/*!< initialize a long integer and import long integer value from a data array
    \param ctx QLIP context pointer
    \param z long integer to be initialized
    \param size the memory size (in 32-bit words) to be allocated for the long integer
	  \param order the long integer can be initialized in NORMAL (1) or BIGNUM order (-1)
    \param endian the Endianness to be applied
		\param pointer to the data array

    \note  long integer must be initialized before it is used. Initialization allocates
    memory storage in the context.
*/

q_status_t q_export (void *data, int *size, int order, int endian, q_lint_t *a);
/*!< export the value of a long integer to a data array
    \param pointer to the data array
    \param size the size of the data
    \param order the long integer can be initialized in NORMAL (1) or BIGNUM order (-1)
    \param endian the Endianness to be applied
    \param a long integer exported
*/

#ifdef QLIP_DEBUG
void q_print (const char* name, q_lint_t *z);
/*!< print the value of a long integer for debug
    \param name name string to be printed
    \param z long integer 
*/

void q_abort (const char *filename, const int line, const char *str);
/*!< abort routine for error handling and debugging
    \param filename the name of the file that causes the error
    \param line the line number where the error occured
    \param str the error message to be printed
*/
#endif

#ifdef QLIP_USE_COUNTER_SPADPA
q_status_t q_cfg_counter_spadpa (q_lip_ctx_t *ctx,   /* QLIP context pointer */
                                 uint32_t ESCAPE,    /* flags to configure SPA/DPA counter measure settings */
                                 uint32_t seed,      /* seed valud to LFSR */
                                 void (*q_get_random)(LFSR_S *lfsr, int32_t length, uint32_t *rnd)); /* get_random function pointer */
               
/*!< configure SPA/DPA counter measure
    \param ctx QLIP context pointer provided by higher level software
    \param ESCAPE flags to configure SPA/DPA counter measure settings
    \param seed seed value to LFSR
    \param q_get_random function pointer to a random number generator function provided by higher level software
*/

void q_seed_lfsr(LFSR_S *lfsr, uint32_t seed);
/*!< function to facilitate higher level software to seed the LFSR
    \param lfsr pointer to the lfsr that can be used to extract the random data
    \param seed seed value to LFSR
*/

void q_shift_lfsr(LFSR_S *lfsr, int32_t count);
/*!< function to shift LFSR
    \param lfsr pointer to the lfsr that can be used to extract the random data
    \param count number of steps to shift
*/

void q_get_random_default(LFSR_S *lfsr, int32_t length, uint32_t *rnd);
/*!< default function to generate random value from LFSR
    \param lfsr pointer to the lfsr that can be used to extract the random data
    \param length the word length of the random to be generated 
    \param rnd the location of the memory buffer to store the generated random
*/

#endif

#endif /*_Q_LIP_UTILS_H_*/
