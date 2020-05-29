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
*   q_lip.h
*
* \brief
*   QLIP main header file for basic long integer structures, functions
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

#ifndef _QLIP_H_
#define _QLIP_H_

#define MACHINE_WD            32
#define BITS_FOR_MACHINE_WD   5
#define UINT_MASK             0xffffffffL
#define EEA_INV               1
#define MIN_CTX_SIZE          512 

/* static code configuration */
/* use Paul Barrett's method for modulo reduction */
#define QLIP_MOD_USE_PB

/* methods of exponent scan */
/*#define QLIP_MODEXP_RL_BIN */
#define QLIP_MODEXP_LR_BIN

/* QLIP status */
#define Q_SUCCESS                            0
#define Q_ERR_CTX_MEM_SIZE_LOW              -1
#define Q_ERR_CTX_ERR                       -2
#define Q_ERR_CTX_OVERFLOW                  -3
#define Q_ERR_MEM_DEALLOC                   -4
#define Q_ERR_DST_OVERFLOW                  -5
#define Q_ERR_DIV_BY_0                      -6
#define Q_ERR_NO_MODINV                     -7

#define Q_ERR_EC_PT_NOT_AFFINE              -10
#define Q_ERR_PKA_HW_ERR                    -100
#define Q_ERR_PPSEL_FAILED                  -200

#define Q_ERR_GENERIC                       -1000

/* Type definition */
#if SUN_OS 
  #ifndef int32_t
  typedef int int32_t;
  #endif
  
  #ifndef uint64_t
  typedef unsigned long long int uint64_t;
  #endif
  
  #ifndef uint32_t
  typedef unsigned int        uint32_t;
  #endif
  
  #ifndef uint16_t
  typedef unsigned short int  uint16_t;
  #endif
  
  #ifndef uint8_t
  typedef unsigned char       uint8_t;
  #endif
#else
  #include <stdint.h>
#endif

typedef uint32_t            q_limb_t;
typedef q_limb_t*           q_limb_ptr_t;
typedef uint32_t            q_size_t;

#ifndef q_status_t
  typedef int32_t             q_status_t;
#endif

/* the long integer */
typedef struct q_lint { 
  q_limb_t *limb;     //!< pointer to long integer value
  int size;           //!< the size of the long integer in words
  int alloc;          //!< the memory allocated for the long integer
  int neg;            //!< sign flag of the long integer
} q_lint_t; //!< data structure of the long integer

typedef struct q_mont { 
  q_lint_t n;
  q_lint_t np;
  q_lint_t rr;
  int br;
} q_mont_t; //!< data structure of the Montgomery fields

#ifdef QLIP_USE_COUNTER_SPADPA
typedef struct LFSR {
  uint32_t l[5];
} LFSR_S;
#endif

typedef struct q_lip_ctx { 
  q_status_t status;         //!< QLIP status
  q_status_t (*q_yield)();   //!< QLIP yield function pointer
  uint32_t CurMemLmt;        //!< QLIP context data memory size in words
  uint32_t CurMemPtr;        //!< Pointer that points to the next unused QLIP context address
  uint32_t *CtxMem;          //!< Pointer to the start of QLIP context data memory
#ifdef QLIP_USE_COUNTER_SPADPA
  uint32_t ESCAPE;           //!< Flag to configure SPA/DPA counter measure 
  LFSR_S     lfsr;
  void (*q_get_random)(LFSR_S *lfsr, int32_t length, uint32_t *rnd);
#endif
} q_lip_ctx_t; //!< data structure of the QLIP context

typedef struct q_signature {
  q_lint_t r;
  q_lint_t s;
} q_signature_t; //!< data structure of the DSA or EC-DSA signature

/* Macros */
#ifndef HOST_ENDIAN
static const q_limb_t  endian_test = (1L << 25) - 1;
#define HOST_ENDIAN    (* (signed char *) &endian_test)
#endif

#ifndef BSWAP
#define BSWAP(value) ((value & 0xff000000) >> 24 | (value & 0x00ff0000) >> 8 | \
                     (value & 0x0000ff00) << 8 | (value & 0x000000ff) << 24)
#endif

#ifndef Q_NORMALIZE
#define Q_NORMALIZE(DST, LIMBS)                   \
do {                                              \
  while (LIMBS > 0) {                             \
    if ((DST)[(LIMBS) - 1] != 0)                  \
      break;                                      \
    LIMBS--;                                      \
  }                                               \
} while (0)
#endif

/* function declarations */
extern __inline uint32_t q_asm_addc(uint32_t al, uint32_t bl, uint32_t *carry);
/*!< 32-bit addition with carry, may be mapped to a single instruction
    \param al source operand
    \param bl source operand
    \param carry carry input
*/

#if 0
// these are defined as static where needed for some reason
__inline uint64_t q_64b_mul(uint64_t r, uint32_t a, uint32_t b); 
/*!< 32x32 multiplication, may be mapped to a single instruction
    \param r accmulating input
    \param a source operand
    \param b source operand
*/

__inline uint64_t q_64b_add(uint64_t a, uint64_t b); 
/*!< 64-bit addition, may be mapped to a single ins.
    \param a source operand
    \param b source operand
*/
#endif

q_status_t q_uadd (q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< unsigned long integer add z = a + b
    \param z result
    \param a source
    \param b source
*/

q_status_t q_add (q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< signed long integer add z = a + b
    \param z result
    \param a source
    \param b source
*/

q_status_t q_usub (q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< unsigned long integer subtract z = a - b
    \param z result
    \param a source
    \param b source
*/

q_status_t q_sub (q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< signed long integer subtract z = a - b
    \param z result
    \param a source
    \param b source
*/

q_status_t q_ucmp (q_lint_t *a, q_lint_t *b);
/*!< unsigned long integer comparison (a>b) return 1; (a<b) return -1; (a==b) return 0;
    \param a source
    \param b source
*/

q_status_t q_cmp (q_lint_t *a, q_lint_t *b);
/*!< signed long integer comparison (a>b) return 1; (a<b) return -1; (a==b) return 0;
    \param a source
    \param b source
*/

q_status_t q_modadd (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b, q_lint_t *n);
/*!< long integer modulo add z = (a + b) mod n
    \param ctx QLIP context pointer 
    \param z result
    \param a source
    \param b source
    \param n modulus
*/

q_status_t q_modsub (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b, q_lint_t *n);
/*!< long integer modulo subtract z = (a - b) mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
    \param n modulus
*/

q_status_t q_mod (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo remainder z = a mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_mod_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo remainder z = a mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_mod_partial (q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo remainder z = a mod n when the size of a is no bigger than the
     size of n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_mod_2pn (q_lint_t *z, q_lint_t *a, uint32_t bits);
/*!< long integer modulo remainder z = a mod n when n is power-of-2
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param bits the number of bits of the remainder
*/

q_status_t q_mul (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< long integer multiply z = a * b
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
*/

q_status_t q_mul_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< long integer multiply z = a * b
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
*/

q_status_t q_sqr (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a);
/*!< long integer square z = a^2
    \param ctx QLIP context pointer
    \param z result
    \param a source
*/

q_status_t q_modmul (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b, q_lint_t *n);
/*!< long integer modulo multiply z = a * b mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
    \param n modulus
*/

q_status_t q_modsqr (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo square z = a^2 mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_modmul_pb (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *r, q_lint_t *a, q_lint_t *b, q_lint_t *n);
/*!< long integer modulo multiply z = a * b mod n using Barrett's method
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
    \param n modulus
*/

q_status_t q_modsqr_pb (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *r, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo square z = a^2 mod n using Barret's method
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_mul_2pn (q_lint_t *z, q_lint_t *a, uint32_t bits);
/*!< long integer multiply when the second operand is a power-of-2, effectively a left shift function
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param bits second operand in number of bits
*/

void get_digit (int *opa, int index, int *value);
void set_digit (int *opa, int index, int value);
q_status_t q_fdivrem (q_lip_ctx_t *ctx, q_lint_t *opa, q_lint_t *opn, q_lint_t *quotient, q_lint_t *remainder);
/*!< long integer divide and remainder routine opa = opn * quotient + remainder
    \param ctx QLIP context pointer
    \param opa dividend
    \param opn divider
    \param quotient
    \param remainder
*/

q_status_t q_div (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< long integer divide z = a / b
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
*/

q_status_t q_div_2pn (q_lint_t *z, q_lint_t *a, uint32_t bits);
/*!< long integer divide when the second operand is power-of-2, effective a right shift function
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param bits second operand in number of bits
*/

q_status_t q_mod_div2 (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo divide by 2, z = a/2 mod n
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_modexp (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *e, q_lint_t *n);
/*!< long integer modulo exponentiation , z = a^e mod n
    \param ctx QLIP context pointer
    \param z result
    \param a base
    \param e exponent
    \param n modulus
*/

q_status_t q_mont_init (q_lip_ctx_t *ctx, q_mont_t *mont, q_lint_t *n);
/*!< Montgomery field initialization function
    \param ctx QLIP context pointer
    \param n modulus
    \param mont Montgomery fields
*/

q_status_t q_mont_mul (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b, q_mont_t *mont);
/*!< long integer modulo mulitply, z = a * b mod n using Montgomery's method
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
    \param mont Montgomery fields
*/

q_status_t q_modexp_mont (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *e, q_mont_t *mont);
/*!< long integer modulo exponentiation , z = a^e mod n using Montgomery's method
    \param ctx QLIP context pointer
    \param z result
    \param a base
    \param e exponent
    \param mont Montgomery fields
*/

q_status_t q_gcd (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *b);
/*!< greatest common divisor function
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param b source
*/

q_status_t q_euclid (q_lip_ctx_t *ctx, q_lint_t *x, q_lint_t *lx, q_lint_t *a, q_lint_t *b);
/*!< Extended Euclid Function d = a * x + b * y return x
    \param ctx QLIP context pointer
    \param x result
    \param lx result
    \param a source
    \param b source
*/

q_status_t q_euclid_sw (q_lip_ctx_t *ctx, q_lint_t *x, q_lint_t *lx, q_lint_t *a, q_lint_t *b);
/*!< Extended Euclid Function d = a * x + b * y return x
    \param ctx QLIP context pointer
    \param x result
    \param lx result
    \param a source
    \param b source
*/

q_status_t q_modinv (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo inverse, z = a ^ (-1) mod n using Extended Euclid Algorithm
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_modinv_sw (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *n);
/*!< long integer modulo inverse, z = a ^ (-1) mod n using Extended Euclid Algorithm
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param n modulus
*/

q_status_t q_modpinv (q_lip_ctx_t *ctx, q_lint_t *z, q_lint_t *a, q_lint_t *p);
/*!< long integer modulo inverse, z = a ^ (-1) mod p when modulus is a prime number 
    \note using this separate function for prime modulus to avoid loop in EEA
    \param ctx QLIP context pointer
    \param z result
    \param a source
    \param p prime modulus
*/

#endif /*_QLIP_H_*/
