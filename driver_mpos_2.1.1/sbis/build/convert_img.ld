SYS_STACK_SIZE = DEFINED(SYS_STACK_SIZE) ? SYS_STACK_SIZE : 0x800;
EXP_STACK_SIZE = DEFINED(EXP_STACK_SIZE) ? EXP_STACK_SIZE : 0x400;
ENTRY(_start)
MEMORY {
  FLASH (rx) : org = 0x63000000 +0x400 +0x360, len = 0x800000 +0x63000000 -(0x63000000 +0x400 +0x360)
  RAM (rwx) : org = 0x24000000, len = 0x100000
}
SECTIONS {
  .text :
  {
    *startup.o(.text.startup .text.startup.*)
    *(.text.startup .text.startup.*)
    EXCLUDE_FILE (*qspi_bcm58202*.o *smau.o *test_smau.o) *(.text .text*)
    *(.rodata) *(.rodata*)
    *(.glue_7) *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
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
  _sidata = LOADADDR(.data);
  .data : AT( _etext + SIZEOF(.fastcode))
  {
    _sdata = .;
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
    __device_PRE_KERNEL_1_start = .; KEEP(*(SORT(.init_PRE_KERNEL_1[0-9]))); KEEP(*(SORT(.init_PRE_KERNEL_1[1-9][0-9]))); __device_PRE_KERNEL_1_end = .;
    __device_PRE_KERNEL_2_start = .; KEEP(*(SORT(.init_PRE_KERNEL_2[0-9]))); KEEP(*(SORT(.init_PRE_KERNEL_2[1-9][0-9]))); __device_PRE_KERNEL_2_end = .;
    __device_POST_KERNEL_start = .; KEEP(*(SORT(.init_POST_KERNEL[0-9]))); KEEP(*(SORT(.init_POST_KERNEL[1-9][0-9]))); __device_POST_KERNEL_end = .;
    __device_APPLICATION_start = .; KEEP(*(SORT(.init_APPLICATION[0-9]))); KEEP(*(SORT(.init_APPLICATION[1-9][0-9]))); __device_APPLICATION_end = .;
    __device_init_end = .;
  } > RAM
  _device_init_load = LOADADDR(initlevel);
  END_LOADABLE_SECTION = LOADADDR(initlevel) + SIZEOF(initlevel);
  .bss (NOLOAD):
  {
    . = ALIGN(4);
    _sbss = .;
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
  __heap_start__ = (__bss_end__ + 0x3FF) & ~(0x3FF);
  SVC_MODE_STACK_SIZE = SYS_STACK_SIZE;
  FIQ_MODE_STACK_SIZE = EXP_STACK_SIZE;
  IRQ_MODE_STACK_SIZE = EXP_STACK_SIZE;
  ABT_MODE_STACK_SIZE = EXP_STACK_SIZE;
  UND_MODE_STACK_SIZE = EXP_STACK_SIZE;
  USR_MODE_STACK_SIZE = SYS_STACK_SIZE;
  RSTR_STACK_SIZE = 0x4000;
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
  __RSTR_STACK_START__ = __RAM_START;
  ASSERT(__USR_STACK_START__ <= __RAM_END, "**** SRAM OVERFLOW DETECTED ****")
}
