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

#ifndef _REG_ACCESS_H_
#define _REG_ACCESS_H_

/* This header is brought from DV. This header contains legacy
 * macros, definitions & types which are necessary to build the
 * ihost_config function.
 *
 * !!! Warning !!!
 *
 * Bare minimum clean-up has been done. But, in interest of time
 * and to minimize risk of breaking code, this file should
 * be left as it is until refactoring it takes higher priority
 * than other objectives.
 *
 * In case of issues arising out of this file, compare with the
 * version from DV.
 */
#include <string.h>
#include "mbox_defines.h"
#include "zephyr/types.h"

#define BIT64    0x8
#define BIT32    0x4
#define BIT16    0x2
#define BIT8     0x1

#ifndef timeout
#define timeout(cnt) { s32_t time_cnt=0; while(time_cnt < (cnt*4)) { time_cnt++; } }
#endif

#define CPU_READ_BURST(addr, data, size) memcpy((void *)data, (void *)addr, size);

#define CPU_WRITE_SINGLE(addr,data,size)  if(size == BIT8) *((volatile u8_t *)(addr)) = (u8_t) (data) ;\
                                       else if(size == BIT16) *((volatile u16_t *)(addr)) = (u16_t) (data) ; \
                                       else if(size == BIT64) *((volatile u64_t *)(addr)) = (u64_t) (data) ; \
                                       else *((volatile u32_t *)(addr)) = (u32_t) (data)

#define CPU_READ_SINGLE(addr,data,size) (data = ((size == BIT8) ?  *((volatile u8_t *)(addr)) :\
                                      ((size == BIT16) ? *((volatile u16_t *)(addr)) :\
				      ((size == BIT64) ? *((volatile u64_t *)(addr)) :\
                                      *((volatile u32_t *)(addr))))))

#define print_log(...) { \
                        volatile char *str_to_print; \
                        volatile char *print_busy; \
			            str_to_print = (volatile char *)(MBOX_STR_TO_PRINT); \
			            print_busy   = (volatile char *)(MBOX_PRINT_BUSY); \
			            while(*print_busy); \
			            sprintf((char*)(MBOX_STRBUFF),__VA_ARGS__); \
			            *print_busy = 1; \
			            *str_to_print = 1; \
			            } 

#define CPU_WRITE_BURST(dest_addr, src_addr, byte_length) memcpy((void *)dest_addr, (const void *)src_addr, byte_length)

#define CPU_BZERO(dest_addr, byte_length) memset((void *)dest_addr, 0, byte_length)
#define WrReg(reg, D) *((volatile u32_t*)(reg)) = D
#define RdReg(reg)   *((volatile u32_t*)(reg))
/* register/field access macros
 * GetField(reg, field, value): extracts a register/field from a value
 *
 * RdRegField(base, reg, field): 'base' is a base/root address and 'reg' is a register macro. reads the register at the address and extracts the field
 *
 * FieldMask(reg, ...): takes a variable number of arguments representing different fields of register 'reg'. generates a mask
 *                      where the bits corresponding to the indicated fields are '1'
 *
 * ModField(reg, orig, ...): takes the 'orig' value of a register and a variable number of field/value pairs. Each field is 
 *                           modified to the specified value
 *
 * SetField(reg, ...): takes a variable number of field/value pairs and constructs a 32b value with the specified fields set to
 *                     the associated values (other bits are 0)
 *
 * RMWRegField(base, reg, ...): takes a variable number of field/value pairs, reads the specified register, modifies the indicated 
 *                              fields, and writes the new value back to the register
 *
 * WrRegField(base, reg, ...): takes a variable number of field/value pairs, constructs a 32b value with the specified fiedls set to
 *                             to the asscoaited values, and write the register
 *
 * RdRegFieldEqual(base, reg, ...): takes a register and a variable number of field/value pairs. returns 1 if all specified fields
 *                                  of the register match the associated value, 0 otherwise
 *
 * NARGS counts the number of arguments in a variadic macro
 */
#define _NARGS(_64, _63, _62, _61, _60, _59, _58, _57, _56, _55, _54, _53, _52, _51, _50, _49, \
               _48, _47, _46, _45, _44, _43, _42, _41, _40, _39, _38, _37, _36, _35, _34, _33, \
               _32, _31, _30, _29, _28, _27, _26, _25, _24, _23, _22, _21, _20, _19, _18, _17, \
               _16, _15, _14, _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0, ...) _0

#define NARGS(...) _NARGS(__VA_ARGS__,                                  \
                          64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, \
                          48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, \
                          32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, \
                          16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

/* GET_VA_MACRO takes a macro name and a variable number of args
 * it counts the args and expands to <macro_name>N
 * where N is the number of args
 */
#define GET_VA_MACRO_N(N, macro_name) macro_name##N
#define _GET_VA_MACRO_N(N, macro_name) GET_VA_MACRO_N(N, macro_name)
#define GET_VA_MACRO(macro_name, ...) _GET_VA_MACRO_N(NARGS(__VA_ARGS__), macro_name)


#define FieldMask1(prefix, field) (((u32_t)0xffffffff)<<(prefix##field##_R)&(((u32_t)0xffffffff)>>(31-prefix##field##_L)))
#define FieldMask2(prefix, field, ...) (FieldMask1(prefix, field)|FieldMask1(prefix, __VA_ARGS__))
#define FieldMask3(prefix, field, ...) (FieldMask1(prefix, field)|FieldMask2(prefix, __VA_ARGS__))
/* if you need to handle more fields, just add the macros here */
#define _FieldMask(prefix, ...) GET_VA_MACRO(FieldMask, __VA_ARGS__)(prefix, __VA_ARGS__)
#define FieldMask(reg, ...) _FieldMask(reg##__, __VA_ARGS__)

#define _GetField(prefix, field, value) ((_FieldMask(prefix, field)&value)>>(prefix##field##_R))
#define GetField(reg, field, value) _GetField(reg##__, field, value)

#define RdRegField(base, reg, field) _GetField(reg##__, field, RdReg(base+reg))

#define ModField2(prefix, orig, field, value) ((orig&(~_FieldMask(prefix, field)))|(_FieldMask(prefix, field)&(((u32_t)value)<<(prefix##field##_R))))
#define ModField4(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField6(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField8(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField10(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField12(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField14(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField16(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField18(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField20(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField22(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField24(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField26(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField28(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField30(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
#define ModField32(prefix, orig, field, value, ...) (ModField2(prefix, GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__),  field, value))
/* if you need to handle more field/value pairs
 * (up to 32 total), just add the macros here
 */
#define _ModField(prefix, orig, ...) GET_VA_MACRO(ModField, __VA_ARGS__)(prefix, orig, __VA_ARGS__)
#define ModField(reg, orig, ...) _ModField(reg##__, orig, __VA_ARGS__)

#define _SetField(prefix, ...) _ModField(prefix, 0, __VA_ARGS__)
#define SetField(reg, ...) _SetField(reg##__, __VA_ARGS__)

#define RMWRegField(base, reg, ...) WrReg(base+reg, _ModField(reg##__, RdReg(base+reg), __VA_ARGS__))
#define WrRegField(base, reg, ...) WrReg(base+reg, _SetField(reg##__, __VA_ARGS__))

/* FieldMaskSkipVal() takes field/value pairs, but just creates
 * the aggregate mask for all of the field, discarding the values
 * this way, the RdRegFieldEqual() macro can take field/value pairs.
 */
#define FieldMaskSkipVal2(prefix, field, value) FieldMask1(prefix, field)
#define FieldMaskSkipVal4(prefix, field, value, ...) (FieldMaskSkipVal2(prefix, field, value) | GET_VA_MACRO(FieldMaskSkipVal, __VA_ARGS__)(prefix, __VA_ARGS__))
#define FieldMaskSkipVal6(prefix, field, value, ...) (FieldMaskSkipVal2(prefix, field, value) | GET_VA_MACRO(FieldMaskSkipVal, __VA_ARGS__)(prefix, __VA_ARGS__))
#define FieldMaskSkipVal8(prefix, field, value, ...) (FieldMaskSkipVal2(prefix, field, value) | GET_VA_MACRO(FieldMaskSkipVal, __VA_ARGS__)(prefix, __VA_ARGS__))
#define FieldMaskSkipVal10(prefix, field, value, ...) (FieldMaskSkipVal2(prefix, field, value) | GET_VA_MACRO(FieldMaskSkipVal, __VA_ARGS__)(prefix, __VA_ARGS__))

/* if you need to handle more fields, just add the macros here */
#define _FieldMaskSkipVal(prefix, ...) GET_VA_MACRO(FieldMaskSkipVal, __VA_ARGS__)(prefix, __VA_ARGS__)
#define FieldMaskSkipVal(reg, ...) _FieldMaskSkipVal(reg##__, __VA_ARGS__)

#define RdRegFieldEqual(base, reg, ...) ((RdReg(base+reg)&_FieldMaskSkipVal(reg##__, __VA_ARGS__)) == (_SetField(reg##__, __VA_ARGS__)))

#define CAPTURE_EXCEPTION(addr , jump_addr) (reg32_write(&addr,jump_addr))


#define cpu_wr_single(addr,data,size)  if(size == BIT8) *((volatile u8_t *)(addr)) = (u8_t)(data);\
                                       else if(size == BIT16) *((volatile u16_t *)(addr)) = (u16_t)(data); \
                                       else if(size == BIT64) *((volatile u64_t *)(addr)) = (u64_t)(data); \
                                       else *((volatile u32_t *)(addr)) = (u32_t)(data)

#define cpu_rd_single(addr,size)   *((volatile u32_t*)addr)

#define timer_wait(timeout) do {                \
    WrRegField(0, CRMU_TIM_TIMER1Control,       \
               TimerEn, 0,                      \
               TimerSize, 1);                   \
    WrReg(CRMU_TIM_TIMER1Load, timeout);        \
    WrReg(CRMU_TIM_TIMER1IntClr, 0x1);          \
    WrRegField(0, CRMU_TIM_TIMER1Control,       \
               TimerEn, 1,                      \
               TimerSize, 1);                   \
    while(!RdReg(CRMU_TIM_TIMER1RIS));          \
  } while (0)

#define reg32_write(addr,data) *(( u32_t *)(addr)) = (u32_t)(data)
#define reg32_read(addr)       *((u32_t *)(addr)) 
#define reg32setbit(addr, num) (reg32_write(addr, reg32_read(addr) | (1<< num)))
#define reg32clrbit(addr, num) (reg32_write(addr, reg32_read(addr) & (0xffffffff ^ (1<< num))))

#define timer_wait_ns(timeout) timer_wait(ns2cycles(timeout))

#endif /* _REG_ACCESS_H_ */

