/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void app_main(void);

#pragma weak app_main = default_app_main

#include <misc/printk.h>
#include <irq.h>
#include <board.h>

int main(void)
{
	printk("No OS version of SDK build!\n");

	/* Enable interrupts */
#ifdef CONFIG_ENABLE_IRQ_FOR_NO_OS
	irq_unlock(0);
#endif /* CONFIG_ENABLE_IRQ_FOR_NO_OS */

	app_main();

	while (1);

	return 0;
}

void default_app_main(void)
{
	printk("No application was built into this image!\n");
}
