/*
 * $Copyright Broadcom Corporation$
 *
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <board.h>
#include "regs.h"

#define reg_read(a)                           (*(volatile unsigned int *)(a))
#define reg_read_or(a, v)                     ((*(volatile unsigned int *)(a))|(v))
#define reg_read_and(a, v)                    ((*(volatile unsigned int *)(a))&(v))

#define reg_write(a, v)                       do { *(volatile unsigned int *)(a) = (v); } while(0)
#define reg_write_or(a, v)                    do { *(volatile unsigned int *)(a) |= (v); } while(0)
#define reg_write_and(a, v)                   do { *(volatile unsigned int *)(a) &= (v); } while(0)

#define reg_bit_read(a, n)                    (((*(volatile unsigned int *)(a))&(1<<(n)))>>(n))
#define reg_bit_clr(a, n)                     do { *(volatile unsigned int *)(a) &= ~(1<<(n)); } while(0)
#define reg_bit_set(a, n)                     do { *(volatile unsigned int *)(a) |= (1<<(n)); } while(0)

#define reg_bits_read(a, off, nbits)          (((*(volatile unsigned int *)(a))&(((1<<(nbits))-1)<<(off)))>>(off))
#define reg_bits_clr(a, off, nbits)           do { *(volatile unsigned int *)(a) &= ~(((1<<(nbits))-1)<<(off)); } while(0)
#define reg_bits_set(a, off, nbits, v)        do { reg_bits_clr(a,off,nbits); *(volatile unsigned int *)(a) |= (((1<<(nbits))-1)&(v))<<(off); } while(0)

#define sint8_t int8_t
#define sint16_t int16_t
#define sint32_t int32_t
#define sint64_t int64_t

#define BOOL	bool
#define TRUE	true
#define FALSE	false

typedef int iproc_status_t;

#endif
