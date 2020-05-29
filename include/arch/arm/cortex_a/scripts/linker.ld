/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-A7 platform.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>
#include <generated_dts_board.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
  #define ROMABLE_REGION FLASH
  #define RAMABLE_REGION SRAM
#else
  #define ROMABLE_REGION SRAM
  #define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
  #define _DATA_IN_ROM __data_rom_start
#else
  #define _DATA_IN_ROM
#endif

/* Default for platforms not defining it */
#ifndef CONFIG_EXTRA_ROM_CODE_OFFSET
#define CONFIG_EXTRA_ROM_CODE_OFFSET 0
#endif
#ifndef CONFIG_FLASH_LOAD_OFFSET
#define CONFIG_FLASH_LOAD_OFFSET 0
#endif

#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET + CONFIG_EXTRA_ROM_CODE_OFFSET)
#if CONFIG_FLASH_LOAD_SIZE > 0
  #define ROM_SIZE CONFIG_FLASH_LOAD_SIZE
#else
  #define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET)
#endif

/* This config flag specifies the RAM reserved for use by the boot loader
 * when the system supports deep sleep enter/exit. This memory is expected
 * to be used by the boot loader and will not be preserved after a deep sleep
 * enter/exit sequence.
 */
#if !defined(CONFIG_RAM_RESERVED_FOR_POWER_MGMT)
#define CONFIG_RAM_RESERVED_FOR_POWER_MGMT	0
#endif

#if defined(CONFIG_XIP)
	#if defined(CONFIG_IS_BOOTLOADER)
		#define RAM_SIZE (CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
		#define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
			(CONFIG_SRAM_SIZE * 1K - RAM_SIZE))
	#else
		#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - \
				  CONFIG_RAM_RESERVED_FOR_POWER_MGMT * 1K)
		#define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
				  CONFIG_RAM_RESERVED_FOR_POWER_MGMT * 1K)
	#endif
#else
	#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
	#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif

MEMORY
    {
    FLASH                 (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
#ifdef DT_CCM_BASE_ADDRESS
    CCM                   (rw) : ORIGIN = DT_CCM_BASE_ADDRESS, LENGTH = DT_CCM_SIZE * 1K
#endif
    SRAM                  (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
    }

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <linker/rel-sections.ld>

    /*
     * .plt and .iplt are here according to 'arm-zephyr-elf-ld --verbose',
     * before text section.
     */
    /DISCARD/ :
	{
	*(.plt)
	}

    /DISCARD/ :
	{
	*(.iplt)
	}

    GROUP_START(ROMABLE_REGION)

	_image_rom_start = ROM_ADDR;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	_image_text_start = .;
	. = CONFIG_TEXT_SECTION_OFFSET;
	*(.text._startup .text._startup.*)
	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * after .gnu.linkonce.t.*
     */
    *(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

	_image_text_end = .;

	. = ALIGN(4);
	_vector_start = .;
	KEEP(*(.exc_vector_table))
	KEEP(*(".exc_vector_table.*"))

	KEEP(*(IRQ_VECTOR_TABLE))

	KEEP(*(.openocd_dbg))
	KEEP(*(".openocd_dbg.*"))

	_vector_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION

#include <linker_relocate.ld>

#endif /* CONFIG_CODE_DATA_RELOCATION */

#if defined (CONFIG_CPLUSPLUS)
	SECTION_PROLOGUE(.ARM.extab,,)
	{
	/*
	 * .ARM.extab section containing exception unwinding information.
	 */
	*(.ARM.extab* .gnu.linkonce.armextab.*)
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_PROLOGUE(.ARM.exidx,,)
	{
	/*
	 * This section, related to stack and exception unwinding, is placed
	 * explicitly to prevent it from being shared between multiple regions.
	 * It  must be defined for gcc to support 64-bit math and avoid
	 * section overlap.
	 */
	__exidx_start = .;
#if defined (__GCC_LINKER_CMD__)
	*(.ARM.exidx* gnu.linkonce.armexidx.*)
#endif
	__exidx_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rodata_start = .;

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)

#ifdef CONFIG_SOC_RODATA_LD
#include <soc-rodata.ld>
#endif

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif

#include <linker/priv_stacks-rom.ld>
#include <linker/kobject-rom.ld>

	/*
	 * For XIP images, in order to avoid the situation when __data_rom_start
	 * is 32-bit aligned, but the actual data is placed right after rodata
	 * section, which may not end exactly at 32-bit border, pad rodata
	 * section, so __data_rom_start points at data and it is 32-bit aligned.
	 *
	 * On non-XIP images this may enlarge image size up to 3 bytes. This
	 * generally is not an issue, since modern ROM and FLASH memory is
	 * usually 4k aligned.
	 */
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rom_end = .;

    GROUP_END(ROMABLE_REGION)

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    /DISCARD/ : {
	*(.got.plt)
	*(.igot.plt)
	*(.got)
	*(.igot)
	}

    GROUP_START(RAMABLE_REGION)

	. = RAM_ADDR;
	. = ALIGN(4);
	_image_ram_start = .;

	/* Section for aligned data that needs no initialization
	 * Place this section at the beginning of data ram to avoid
	 * large sized fills that waste memory.
	 */
	SECTION_DATA_PROLOGUE(_aligned_noinit_data, (NOLOAD),)
	{
		*(.data.aligned.noinit.16k.*);
		*(.data.aligned.noinit.8k.*);
		*(.data.aligned.noinit.4k.*);
		*(.data.aligned.noinit.2k.*);
		*(.data.aligned.noinit.1k.*);
	}

#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
    SECTION_DATA_PROLOGUE(.ramfunc,,)
	{
		_ramfunc_ram_start = .;
		__f_start = .;
		*(.ramfunc)
		*(".ramfunc.text*")
		*(".ramfunctext*")
		*(fctext.*)
		*(text.fastcode*)
		_ramfunc_ram_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	_ramfunc_ram_size = _ramfunc_ram_end - _ramfunc_ram_start;
	_ramfunc_rom_start = LOADADDR(.ramfunc);
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
	__bss_start = .;
	__kernel_ram_start = .;

	*(.bss)
	*(".bss.*")
	*(COMMON)
	*(".kernel_bss.*")
	*(.ramfunc.bss*)
	*(.ramfunc.COMMON*)
        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
	__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
        {
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
        *(.noinit)
        *(".noinit.*")
	*(".kernel_noinit.*")


        } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	__data_ram_start = .;
	*(.data)
	*(".data.*")
	*(".kernel.*")
	*(.ramfunc.data*)
	*(.ramfunc.rodata*)


	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    __data_rom_start = LOADADDR(_DATA_SECTION_NAME);

#include <linker/common-ram.ld>
#include <linker/priv_stacks.ld>
#include <linker/kobject.ld>

#include <linker/priv_stacks-noinit.ld>

    __data_ram_end = .;


    /* Define linker symbols */

    _image_ram_end = .;
    _end = .; /* end of image */

    __kernel_ram_end = RAM_ADDR + RAM_SIZE;
    __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

    GROUP_END(RAMABLE_REGION)

#ifdef DT_CCM_BASE_ADDRESS

    GROUP_START(CCM)

	SECTION_PROLOGUE(_CCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_start = .;
		__ccm_bss_start = .;
		*(.ccm_bss)
		*(".ccm_bss.*")
	} GROUP_LINK_IN(CCM)

	__ccm_bss_end = .;

	SECTION_PROLOGUE(_CCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_noinit_start = .;
		*(.ccm_noinit)
		*(".ccm_noinit.*")
	} GROUP_LINK_IN(CCM)

	__ccm_noinit_end = .;

	SECTION_PROLOGUE(_CCM_DATA_SECTION_NAME,,SUBALIGN(4))
	{
		__ccm_data_start = .;
		*(.ccm_data)
		*(".ccm_data.*")
	} GROUP_LINK_IN(CCM AT> ROMABLE_REGION)

	__ccm_data_end = .;
	__ccm_end = .;

	__ccm_data_rom_start = LOADADDR(_CCM_DATA_SECTION_NAME);

    GROUP_END(CCM)

#endif /* DT_CCM_BASE_ADDRESS */

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

#include <linker/debug-sections.ld>

    SECTION_PROLOGUE(.ARM.attributes, 0,)
	{
	KEEP(*(.ARM.attributes))
	KEEP(*(.gnu.attributes))
	}

    /DISCARD/ :
	{
		*(.ramfunc.debug*)
		*(.ramfunc.comment*)
		*(.ramfunc.ARM.*)
	}

/* Must be last in romable region */
SECTION_PROLOGUE(.last_section,(NOLOAD),)
{
} GROUP_LINK_IN(ROMABLE_REGION)

/* To provide the image size as a const expression,
 * calculate this value here. */
_flash_used = LOADADDR(.last_section) - _image_rom_start;

    }
