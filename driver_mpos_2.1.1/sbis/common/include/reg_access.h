/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

#ifndef __REG_ACCESS_H__
#define __REG_ACCESS_H__

#include <zephyr/types.h>

extern uint32_t cpu_rd_single(uint32_t addr, int size);
extern void cpu_wr_single(uint32_t addr, uint32_t data, int size);

#define BIT64       0x3
#define BIT32       0x2
#define BIT16       0x1
#define BIT8        0x0

#define AHB_BIT8    0x0        //3'b000
#define AHB_BIT16   0x1        //3'b001
#define AHB_BIT32   0x2        //3'b010
#define AHB_BIT64   0x3        //3'b011

#define WrReg(A,D) *((volatile uint32_t*)A) = D
#define RdReg(A)   *((volatile uint32_t*)A)

#define RD8(addr) CPU_READ_SINGLE(addr, 1)
#define RD16(addr) CPU_READ_SINGLE(addr, 2)
#define RD32(addr) CPU_READ_SINGLE(addr, 4)
#define RD64(addr) CPU_READ_SINGLE(addr, 8)

#define WR8(addr, data)  CPU_WRITE_SINGLE(addr, data, 1)
#define WR16(addr, data) CPU_WRITE_SINGLE(addr, data, 2)
#define WR32(addr, data) CPU_WRITE_SINGLE(addr, data, 4)
#define WR64(addr, data) CPU_WRITE_SINGLE(addr, data, 8)

#define rd(addr) CPU_READ_SINGLE(addr, 4)
#define wr(addr, data) CPU_WRITE_SINGLE(addr, data, 4)
#define RD(addr) CPU_READ_SINGLE(addr, 4)
#define WR(addr, data) CPU_WRITE_SINGLE(addr, data, 4)

#define CPU_READ_SINGLE(addr,size)  ((size == 1)  ?  *((volatile uint8_t *) addr) :\
                                    ((size == 2)  ?  *((volatile uint16_t *)addr) :\
                                    ((size == 8)  ?  *((volatile uint64_t *)addr) :\
                                                     *((volatile uint32_t *)addr))))

#define CPU_WRITE_SINGLE(addr,data,size)  if(size == 1) *((volatile uint8_t *)addr) = data ;\
                                          else if(size == 2) *((volatile uint16_t *)addr) = data ; \
                                          else if(size == 8) *((volatile uint64_t *)addr) = data ; \
                                          else *((volatile uint32_t *)addr) = data
#define reg32setbit(addr , num)    (reg32_write(addr, reg32_read(addr) | (1<< num)))
#define reg32clrbit(addr , num)    (reg32_write(addr, reg32_read(addr) & (0xffffffff ^ (1<< num))))

#define CPU_READ_8(addr) CPU_READ_SINGLE(addr, 1)
#define CPU_READ_16(addr) CPU_READ_SINGLE(addr, 2)
#define CPU_READ_32(addr) CPU_READ_SINGLE(addr, 4)
#define CPU_READ_64(addr) CPU_READ_SINGLE(addr, 8)

#define CPU_WRITE_8(addr, data)  CPU_WRITE_SINGLE(addr, data, 1)
#define CPU_WRITE_16(addr, data) CPU_WRITE_SINGLE(addr, data, 2)
#define CPU_WRITE_32(addr, data) CPU_WRITE_SINGLE(addr, data, 4)
#define CPU_WRITE_64(addr, data) CPU_WRITE_SINGLE(addr, data, 8)

#define CPU_RMW_OR_32(addr,data)  (*((volatile uint32_t *)addr) |= data)
#define CPU_RMW_AND_32(addr,data) (*((volatile uint32_t *)addr) &= data)
#define CPU_RMW_OR_8(addr,data)  (*((volatile uint8_t *)addr) |= data)


#define CPU_WRITE_BURST(dest_addr, src_addr, byte_length) memcpy((void *)dest_addr, (const void *)src_addr, byte_length)

#define CPU_BZERO(dest_addr, byte_length) memset((void *)dest_addr, 0, byte_length)

#define CPU_RMW_INC_SINGLE(addr,data,size) *((unsigned long *)addr) += data

#define AHB_WRITE_VERIFY(addr, data) AHB_WRITE_VERIFY_mb(addr, data);

#define GET_IPROC_REG64_FIELD(field,regvalue) (uint64_t)((((uint64_t)(regvalue) >> ((uint64_t)(field) & 63)) & ((1 << ((uint64_t)(field) >> 6)) - 1)))

#define GET_IPROC_REG32_FIELD(field,regvalue) (uint32_t)((((uint32_t)(regvalue) >> ((uint32_t)(field) & 31)) & ((1 << ((uint32_t)(field) >> 5)) - 1)))

#define GET_IPROC_REG16_FIELD(field,regvalue) (uint16_t)((((uint16_t)(regvalue) >> ((uint16_t)(field) & 15)) & ((1 << ((uint16_t)(field) >> 4)) - 1)))

#define GET_IPROC_REG8_FIELD(field,regvalue) (uint8_t)((((uint8_t)(regvalue) >> ((uint8_t)(field) & 7)) & ((1 << ((uint8_t)(field) >> 3)) - 1)))

#define GET_IPROC_REG_FIELD(field,regvalue,size) ((size == AHB_BIT32) ? GET_IPROC_REG32_FIELD(field,regvalue) :\
                              ((size == AHB_BIT64) ? GET_IPROC_REG64_FIELD(field,regvalue) :\
                              ((size == AHB_BIT8) ? GET_IPROC_REG8_FIELD(field,regvalue) :\
                              GET_IPROC_REG16_FIELD(field,regvalue))))

#define SET_IPROC_REG64_FIELD(addr,field,fieldvalue) \
                                       regvalue = CPU_READ_SINGLE(addr,AHB_BIT64); \
                                       width = (uint64_t)((field) >> 6); \
                                       width = (width == 0 ? 1 : width); \
                                       start_offset = (uint64_t)((field) & 63); \
                                       mask = (uint64_t)(~(((1 << width) - 1) << start_offset)); \
                                       regvalue = (uint64_t)(regvalue & mask); \
                                       fieldvalue = (uint64_t)((fieldvalue << start_offset) & mask); \
                                       regvalue = (uint64_t)(regvalue | fieldvalue); \
                                       CPU_WRITE_SINGLE(addr,regvalue,AHB_BIT64)

#define SET_IPROC_REG32_FIELD(addr,field,fieldvalue) \
                                       regvalue = CPU_READ_SINGLE(addr,AHB_BIT32); \
                                       width = (uint32_t)((field) >> 5); \
                                       width = (width == 0 ? 1 : width); \
                                       start_offset = (uint32_t)((field) & 31); \
                                       mask = (uint32_t)(~(((1 << width) - 1) << start_offset)); \
                                       regvalue = (uint32_t)(regvalue & mask); \
                                       fieldvalue = (uint32_t)((fieldvalue << start_offset) & mask); \
                                       regvalue = (uint32_t)(regvalue | fieldvalue); \
                                       CPU_WRITE_SINGLE(addr,regvalue,AHB_BIT32)

#define SET_IPROC_REG16_FIELD(addr,field,fieldvalue) \
                                       regvalue = CPU_READ_SINGLE(addr,AHB_BIT16); \
                                       width = (uint16_t)((field) >> 4); \
                                       width = (width == 0 ? 1 : width); \
                                       start_offset = (uint16_t)((field) & 15); \
                                       mask = (uint16_t)(~(((1 << width) - 1) << start_offset)); \
                                       regvalue = (uint16_t)(regvalue & mask); \
                                       fieldvalue = (uint16_t)((fieldvalue << start_offset) & mask); \
                                       regvalue = (uint16_t)(regvalue | fieldvalue); \
                                       CPU_WRITE_SINGLE(addr,regvalue,AHB_BIT16)

#define SET_IPROC_REG8_FIELD(addr,field,fieldvalue) \
                                       regvalue = CPU_READ_SINGLE(addr,AHB_BIT8); \
                                       width = (uint8_t)((field) >> 3); \
                                       width = (width == 0 ? 1 : width); \
                                       start_offset = (uint8_t)((field) & 7); \
                                       mask = (uint8_t)(~(((1 << width) - 1) << start_offset)); \
                                       regvalue = (uint8_t)(regvalue & mask); \
                                       fieldvalue = (uint8_t)((fieldvalue << start_offset) & mask); \
                                       regvalue = (uint8_t)(regvalue | fieldvalue); \
                                       CPU_WRITE_SINGLE(addr,regvalue,AHB_BIT8)

#define SET_IPROC_REG_FIELD(addr,field,fieldvalue,size) ((size == AHB_BIT32) ? SET_IPROC_REG32_FIELD(addr,field,fieldvalue) :\
                              ((size == AHB_BIT64) ? SET_IPROC_REG64_FIELD(addr,field,fieldvalue) :\
                              ((size == AHB_BIT8) ? SET_IPROC_REG8_FIELD(addr,field,fieldvalue) :\
                              SET_IPROC_REG16_FIELD(addr,field,fieldvalue))))

#endif /* __REG_ACCESS_H__ */
 
 