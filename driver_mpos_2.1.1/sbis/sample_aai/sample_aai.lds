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

SYS_STACK_SIZE = DEFINED(SYS_STACK_SIZE) ? SYS_STACK_SIZE : 0x800;
EXP_STACK_SIZE = DEFINED(EXP_STACK_SIZE) ? EXP_STACK_SIZE : 0x400;

#define DEVICE_INIT_LEVEL(level)                                \
                __device_##level##_start = .;                   \
                KEEP(*(SORT(.init_##level[0-9])));              \
                KEEP(*(SORT(.init_##level[1-9][0-9])));         \
                __device_##level##_end = .;                     \

ENTRY(_start)

MEMORY {
  FLASH (rx) : org = RO_MEM_START, len = RO_MEM_LENGTH
  RAM  (rwx) : org = RW_MEM_START, len = RW_MEM_LENGTH
}

SECTIONS {
  /* Put vectors, program code and readonly data in FLASH */
  .text :
  {
    *startup.o(.text.startup .text.startup.*)
    *(.text.startup .text.startup.*)
    EXCLUDE_FILE (*qspi_bcm58202*.o *smau.o *test_smau.o) *(.text .text*) /*code */
    *(.rodata) *(.rodata*)   /* readonly data */
    *(.glue_7) *(.glue_7t)   /* ARM/thumb glue */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* end of code */
  } > FLASH

  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > FLASH

  .devconfig :
  {
    __devconfig_start = .;
    *(".devconfig.*")
    KEEP(*(SORT_BY_NAME(".devconfig*")))
    __devconfig_end = .;
  } > FLASH

  .rodata :
  {
    EXCLUDE_FILE (*qspi_bcm58202*.o *smau.o *test_smau.o) *(.rodata .rodata*)
  } > FLASH

  _etext = .;
  PROVIDE(etext = .);
  .fastcode : AT(_etext)
  {
    __fastcode_load__ = LOADADDR(.fastcode);
    __fastcode_start__ = .;
    . = ALIGN(4);
    *(text.fastcode)
    *qspi_bcm58202*.o(.text*)
    *qspi_bcm58202*.o(.rodata*)
    *smau.o(.text*)
    *smau.o(.rodata*)
    *test_smau.o(.text*)
    *test_smau.o(.rodata*)
    . = ALIGN(4);
    __fastcode_end__ = .;
  } > RAM

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  .data : AT( _etext + SIZEOF(.fastcode))
  {
    _sdata = .;   /* overall data start */
    __data_start__ = .;
    . = ALIGN(4);
    *(.data .data.*)
    __data_end__ = .;
  } > RAM
  PROVIDE(edata = .);
  _edata = .;

  initlevel :
  {
    __device_init_start = .;
    DEVICE_INIT_LEVEL(PRE_KERNEL_1)
    DEVICE_INIT_LEVEL(PRE_KERNEL_2)
    DEVICE_INIT_LEVEL(POST_KERNEL)
    DEVICE_INIT_LEVEL(APPLICATION)
    __device_init_end = .;
  } > RAM
  _device_init_load = LOADADDR(initlevel);
  /* This should point to the end of the last load section (not marked as NOLOAD)*/
  END_LOADABLE_SECTION = LOADADDR(initlevel) + SIZEOF(initlevel);

  .bss (NOLOAD):
  {
    . = ALIGN(4);
    _sbss = .;    /* bss start. Labels used in start-up code to zero the .bss section */
    __bss_start__ = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM
  PROVIDE(end = .);
  _end = .;

  __RAM_START = ORIGIN(RAM);
  __RAM_END = __RAM_START + LENGTH(RAM);

  /* Carve out the heap in the memory between bss end and stack start
   * +---------+
   * +   ...   +
   * +         +
   * +   BSS   +
   * +---------+
   * +   HEAP  +
   * +---------+
   * +   STACK +
   * +---------+
   */

  /* Align heap to 1K boundary */
  __heap_start__ = (__bss_end__ + 0x3FF) & ~(0x3FF);

  /* Define stack sizes for each CPU mode and compute the total */
  /* This is the mode in which we enter SBI from SBL */
  SVC_MODE_STACK_SIZE = SYS_STACK_SIZE;
  FIQ_MODE_STACK_SIZE = EXP_STACK_SIZE; /* Fast interrupt mode */
  IRQ_MODE_STACK_SIZE = EXP_STACK_SIZE; /* Regular interrupt mode */
  ABT_MODE_STACK_SIZE = EXP_STACK_SIZE; /* Abort mode */
  UND_MODE_STACK_SIZE = EXP_STACK_SIZE; /* Undefined instruction mode */
  USR_MODE_STACK_SIZE = SYS_STACK_SIZE; /* User/system mode */
  RSTR_STACK_SIZE = 0x4000;   /* Low power mode restore */

  TOTAL_STACK_SIZE = SVC_MODE_STACK_SIZE + FIQ_MODE_STACK_SIZE +
         IRQ_MODE_STACK_SIZE + ABT_MODE_STACK_SIZE +
         UND_MODE_STACK_SIZE + USR_MODE_STACK_SIZE;

  __heap_size__ = (__RAM_END - __heap_start__ - TOTAL_STACK_SIZE) ;
  __heap_end__ = (__heap_start__ + __heap_size__) ;

  __SVC_STACK_START__ = __heap_end__ + SVC_MODE_STACK_SIZE;
  __FIQ_STACK_START__ = __SVC_STACK_START__ + FIQ_MODE_STACK_SIZE;
  __IRQ_STACK_START__ = __FIQ_STACK_START__ + IRQ_MODE_STACK_SIZE;
  __ABT_STACK_START__ = __IRQ_STACK_START__ + ABT_MODE_STACK_SIZE;
  __UND_STACK_START__ = __ABT_STACK_START__ + UND_MODE_STACK_SIZE;
  __USR_STACK_START__ = __UND_STACK_START__ + USR_MODE_STACK_SIZE;

  /* Low power save/restore logic can re-use the SBL RAM space */
  __RSTR_STACK_START__ = __RAM_START;

  ASSERT(__USR_STACK_START__ <= __RAM_END, "**** SRAM OVERFLOW DETECTED ****")
}

