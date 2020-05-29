/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/* @file crypto_pka_util.c
 *
 * Utility Api's for the crypto pka
 *
 *
 *
 */

/*
 * TODO
 *
 *
 * */
/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <crypto/crypto.h>
#include <pka/q_lip.h>
#include <pka/q_lip_aux.h>

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
/**
 * Definition
 */

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/


/* ***************************************************************************
 * Global and  Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/
/** Zeros a block of memory
 * @param[in,out] addr the block to be zeroized
 * @param[in] len the length (in bytes) of the block
 */
void secure_memclr(void* addr, unsigned len)
{
   int i;
   u8_t *a = (u8_t*)addr;
   for (i = 0; i < len; ++i) {
      a[i] = 0;
   }
}

/** Standard memcpy() function
 * @param[out] to the address to copy to
 * @param[in] from the address to copy from
 * @param[in] len the length in bytes to copy
 */
void secure_memcpy(u8_t* to, const u8_t* from, unsigned len)
{
   int i;
   for (i = 0; i < len; ++i) {
      to[i] = from[i];
   }
}

/** Standard memmove() function
 * @param[out] to the address to copy to
 * @param[in] from the address to copy from
 * @param[in] len the length in bytes to copy
 */
void secure_memmove(u8_t* to, const u8_t* from, unsigned len)
{
   int i;
   if (to <= from) {
      for (i = 0; i < len; ++i) {
         to[i] = from[i];
      }
   } else {
      for (i = len-1; i >= 0; --i) {
         to[i] = from[i];
      }
   }
}

/** Compare two blocks of memory, taking constant time to do so (to prevent timing attacks)
 * @param[in] a the first memory address to compare
 * @param[in] b the second memory address to compare
 * @param[in] len the length (in bytes) to check
 * @return returns non zero if the two arrays are equal
 */
int secure_memeql(const u8_t* a, const u8_t* b, unsigned len)
{
   int i;
   u8_t diff = 0;

   for (i = 0; i < len; ++i) {
      // xoring the two bytes = zero if they're the same.  Or-ing with "diff" will set bits if
      // the xor was non-zero:
      diff |= (a[i] ^ b[i]);

      if(diff)
      {
          return 0;
      }
   }

   return (~diff);
}

/** Copies bytes, while reversing the order.  \a to and \a from should not overlap, unless they are identical.
 * @param[out] to the address to copy to
 * @param[in] from the address to copy from
 * @param[in] len the length in bytes to copy
 */
void secure_memrev(u8_t* to, const u8_t* from, unsigned len)
{
   int i;
   for (i = 0; i < (len+1)/2; ++i) {
      u8_t tmp = from[i];  // in case we're doing this in-place, swap using tmp
      to[i] = from[len-1-i];
      to[len-1-i] = tmp;
   }
}

/* swap the word sequence, e.g. first word becomes last word */
/* pIn and pOut can be the same     */
/* nLen: in bytes */
void bcm_swap_words(unsigned int *pInt, unsigned int nLen)
{
    unsigned int i, tmp, *pIntEnd;
    unsigned char tmp1[512];
    unsigned char *tmp3 = (unsigned char *)pInt;

    if (nLen > 512) return;

    memcpy(tmp1, tmp3, nLen);
    pInt = (unsigned int *)tmp1;
    pIntEnd = pInt + (nLen>>2) - 1;
    for (i=0; pInt < pIntEnd; i++, pInt++, pIntEnd--)
    {
        tmp = *pInt;
        *pInt = *pIntEnd;
        *pIntEnd = tmp;
    }
    memcpy(tmp3, tmp1, nLen);
}

/* swap the endianness of the words */
/* pIn and pOut can be the same     */
/* fByte: swap by byte or 16bit word */
void bcm_swap_endian(unsigned int *pIn, unsigned int nLen,
                            unsigned int *pOut, unsigned int fByte)
{
    unsigned int i, tmp;
    unsigned char tmp1[512];
    unsigned char tmp2[512];
    unsigned int lnLen = nLen;
    unsigned char *tmp3 = (unsigned char *)pIn;
    unsigned char *tmp4 = (unsigned char *)pOut;
    unsigned int *tmp11 = (unsigned int *)tmp1;
    unsigned int *tmp21 = (unsigned int *) tmp2;

    if (nLen & 0x3) return; /* return if not in words */

    if (nLen > 512) return; /* local buffer not big enough */

    memcpy(tmp1, tmp3, nLen);

    nLen >>= 2;
    for (i=0; i<nLen; i++)
    {
        /* this is to deal with unaligned accesses */
        tmp = tmp11[i];
        if (fByte)
        tmp = ((tmp & 0x0FF) << 24) | ((tmp & 0x0FF00) << 8) |
              ((tmp & 0x0FF0000) >> 8) | ((tmp & 0xFF000000) >> 24);
        else
        tmp = ((tmp & 0x0FFFF) << 16) | ((tmp & 0xFFFF0000) >> 16);
        /* this is to deal with unaligned accesses */
        tmp21[i] = tmp;
    }

    memcpy(tmp4, tmp2, lnLen);
}

/* change a block of ints to BIGNUM format, will replace in place */
/* e.g. : change endianess inside the int, and the endianness among the ints */
void bcm_int2bignum(unsigned int *pInt, unsigned int nLen)
{
    if (nLen & 0x3) return; /* return if not in words */

    bcm_swap_endian(pInt, nLen, pInt, TRUE);
    bcm_swap_words(pInt, nLen);
}

////////////////////////////////////////////////////////////////////
// Function: set_param_bn
// This function formats a parameter for processing by the qlip library
// The input data is already in big number format.
//
// Inputs:
// qparm     = Pointer to qlib_t structure
// param     = u8_t pointer to input parameter
// param_len = length of param in bits
// pad_len   = length of padded param in bytes
////////////////////////////////////////////////////////////////////
void set_param_bn(q_lint_t *qparam, u8_t * param,
                         u32_t param_len, u32_t pad_len)
{
	ARG_UNUSED(pad_len);
    qparam->limb = (q_limb_t *)param;

    /* WARNING removal TODO please fix the correct one */

    /* this is a temporary fix. find a permanent fix later */
    /* qparam->size = pad_len/4; */
    qparam->size = BITLEN2BYTELEN(param_len);
    if (qparam->size % 4)
    qparam->size = (qparam->size/4)+1;
    else
    qparam->size = (qparam->size/4);
    qparam->alloc=qparam->size;
    qparam->neg=0;
}

/*---------------------------------------------------------------
 * Name    : bcm_soft_add_byte2byte_plusX
 * Purpose : Performs A = A + B + X with big-endian bignums
 *           if B is zero length then performs A = A + X
 * Input   : A: first number to add (and also output)
 *           lenA: length of A in bytes
 *           B: second number to add
 *           lenB: length of B in bytes
 *           X: byte that is added to sum (0-255)
 * Output  : A
 * Return  : carry
 *--------------------------------------------------------------*/
unsigned
bcm_soft_add_byte2byte_plusX(u8_t *A, u32_t lenA, u8_t *B, u32_t lenB, u8_t x)
{
    u8_t tmpA = 0;
    u8_t tmpB = 0;
    u8_t *pA = A + (lenA - 1);
    u8_t *pB = B + (lenB - 1);

    u8_t carry = x;
    u32_t tmp;

    while(lenA || lenB) {
        if (lenA)
            tmpA = *pA;
        else
            tmpA = 0;

        if (lenB)
            tmpB = *pB;
        else
            tmpB = 0;

        // Update the output
        tmp = tmpA + tmpB + carry;
        *pA = tmp;
        carry = (tmp >> 8);

        if (lenA) {
            lenA--;
            pA--;
        }
        if (lenB) {
            lenB--;
            pB--;
        }
    }

    return carry;
}

void fips_error() {

    printk(" ========== FIPS ERROR ========== \n");

    // delay about a millisecond
    u32_t i;
    for (i = 0; i < 15000; ++i) {
	    __asm volatile("nop");
    }

/*  FIXME PRASAD, this functionality exists only in FIPS
#ifdef FIPS_DEBUG
    while (1);
#endif
    {
        uint32_t reg_val = AHB_READ_SINGLE(CRMU_SOFT_RESET_CTRL,AHB_BIT32);
        reg_val &= ~(CRMU_SOFT_RESET_CTRL__SOFT_SYS_RST_L);
        AHB_WRITE_SINGLE(CRMU_SOFT_RESET_CTRL,reg_val,AHB_BIT32);
    }

    printf("ERROR: FAILED to reset\n");
*/

    // That should have caused a reset.  In the event of hardware malfunction that prevents a reset,
    // don't return to the caller, just hang
    while(1);
}
