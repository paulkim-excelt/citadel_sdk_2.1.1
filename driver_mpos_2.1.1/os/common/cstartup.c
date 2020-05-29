/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The transfer of control from the reset handler to main() happens through the
 * start_scheduler() function defined here. The global data, bss and fast code
 * sections are initialized in this function. So no global/static variables
 * should be used here. In addition, all the device driver initialization
 * functions arecalled, before transferring the control to main(). main()
 * function is expected to invoke the scheduler and enable interrupts.
 */

#include <string.h>
#include <toolchain/gcc.h>
#include <irq.h>
#include <cortex_a/cp15.h>

extern int __bss_start__;
extern int __bss_end__;

extern int __device_init_start;
extern int __device_init_end;
extern int _device_init_load;

extern int __fastcode_load__;
extern int __fastcode_start__;
extern int __fastcode_end__;

extern void main(void);
extern void osal_device_init(void);

#ifdef CONFIG_FPU_ENABLED
#define FPU_ENABLE_MASK		0x40000000
#define FPU_ACCESS_MASK		((0x3 << 20) |	/* Cp10 full access */ \
				 (0x3 << 22))	/* Cp11 full access */
#define FPU_NS_ACCESS_MASK	((0x1 << 10) |	/* Cp10 non-secure access */ \
				 (0x1 << 11))	/* Cp11 non-secure access */

static void enable_fpu(void)
{
	u32_t reg, fp_exc;

	/* Enable full Access to cp10 and cp11 registers */
	reg = read_cpacr();
	reg |= FPU_ACCESS_MASK;
	write_cpacr(reg);

	/* Enable non-secure access to cp10 and cp11 registers */
	reg = read_nsacr();
	reg |= FPU_NS_ACCESS_MASK;
	write_nsacr(reg);

	/* Set FPSCR.EN to enable VFP */
	__asm__ volatile ("vmrs %[fp_exc], fpexc\t\n"
			  "orr %[fp_exc], %[fp_en_mask]\t\n"
			  "vmsr fpexc, %[fp_exc]"
			  : [fp_exc] "=r" (fp_exc)
			  : [fp_en_mask] "i" (FPU_ENABLE_MASK)
			  );
}
#endif /* CONFIG_FPU_ENABLED */

static void init_hardware(void)
{
	/* Disable interrupts while initializing devices and starting up OS.
	 * Interrupts are automatically enabled when the scheduler is started.
	 */
	irq_lock();

	/* Enable FPU if CONFIG_FPU_ENABLED is set */
#ifdef CONFIG_FPU_ENABLED
	enable_fpu();
#endif

	/* Call the OSAL to do the Zephyr-like device discovery */
	osal_device_init();
}

static void data_mem_init(void)
{
	/*__bss_start__ and __bss_end__ are defined in the linker script */
	int *bss = &__bss_start__;
	int *bss_end = &__bss_end__;

	/*  Clear the BSS section  */
	while (bss < bss_end)
		*bss++ = 0;

	/* Copy the driver objects from flash to RAM */
	memcpy(&__device_init_start, &_device_init_load,
		((unsigned long)&__device_init_end -
		 (unsigned long)&__device_init_start));

	/* Relocate fast code */
	memcpy(&__fastcode_start__, &__fastcode_load__,
		((unsigned long)&__fastcode_end__ -
		 (unsigned long)&__fastcode_start__));
}

void start_scheduler(void)
{
	/* Initialize data memory */
	data_mem_init();

	/* Initialize hardware before calling main() */
	init_hardware();

	/* main function is only expected to start the RTOS scheduler */
	main();

	/* Should never return from main */
	while (1) {

	}
}
