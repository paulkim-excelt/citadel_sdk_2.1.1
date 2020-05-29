SYS_STACK_SIZE = DEFINED(SYS_STACK_SIZE) ? SYS_STACK_SIZE : 0x800;
EXP_STACK_SIZE = DEFINED(EXP_STACK_SIZE) ? EXP_STACK_SIZE : 0x400;

#define DEVICE_INIT_LEVEL(level)                                \
                __device_##level##_start = .;                   \
                KEEP(*(SORT(.init_##level[0-9])));              \
                KEEP(*(SORT(.init_##level[1-9][0-9])));         \
                __device_##level##_end = .;                     \

ENTRY(_start)

MEMORY {
	RAM  (rwx) : org = 0x24010000, len = 0x000F0000 /* FIXME: Bottom 64KB for SBL - Explore Optimization */
}

SECTIONS {
	/* Put everything in RAM */
	.text :
	{
		*startup.o(.text.startup .text.startup.*)
		*(.text.startup .text.startup.*)
		*(.text .text*) /*code */
		*(text.fastcode)
		*(.rodata) *(.rodata*)   /* readonly data */
		*(.glue_7) *(.glue_7t)   /* ARM/thumb glue */
		*(.eh_frame)

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;        /* end of code */
	} > RAM

	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > RAM

	.devconfig :
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} > RAM

	.rodata :
	{
		*(.rodata .rodata*)
	} > RAM

	/* No need to initialize for a load and execute image */
	__fastcode_load__ = .;
	__fastcode_start__ = .;
	__fastcode_end__ = .;

	/* No need to initialize for a load and execute image */
	_sidata = .;
	_sdata = .;		/* overall data start */
	__data_start__ = .;
	__data_end__ = .;
	_edata = .;

	.data :
	{
		. = ALIGN(4);
		*(.data .data.*)
	} > RAM
	PROVIDE(edata = .);

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

	.bss (NOLOAD):
	{
		. = ALIGN(4);
		_sbss = .;		/* bss start. Labels used in start-up code to zero the .bss section */
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
	FIQ_MODE_STACK_SIZE = EXP_STACK_SIZE;	/* Fast interrupt mode */
	IRQ_MODE_STACK_SIZE = EXP_STACK_SIZE;	/* Regular interrupt mode */
	ABT_MODE_STACK_SIZE = EXP_STACK_SIZE;	/* Abort mode */
	UND_MODE_STACK_SIZE = EXP_STACK_SIZE;	/* Undefined instruction mode */
	USR_MODE_STACK_SIZE = SYS_STACK_SIZE;	/* User/system mode */
	RSTR_STACK_SIZE = 0x4000;		/* Low power mode restore */

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

	ASSERT(__USR_STACK_START__ <= __RAM_END, "**** ITCM OVERFLOW DETECTED ****")
}
