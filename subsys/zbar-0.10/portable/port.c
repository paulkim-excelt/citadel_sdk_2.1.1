/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <assert.h>
#include <misc/printk.h>
#include <kernel.h>
#include <string.h>

void __assert_func (
	const char* file,
	int line,
	const char* func,
	const char* expr)
{
	printk("ASSERT: [%s:%d] assertion \"%s\" failed\n", file, line, expr);
	if (func)
		printk("FUNCITON: %s\n", func);
}

void *k_realloc(void *old_ptr, unsigned int new_size)
{
	void *new_ptr=NULL;

	if(!new_size)
	{
		k_free(old_ptr);
		return NULL;
	}

	new_ptr = k_malloc(new_size);
	if(!new_ptr)
		return NULL;

	if(old_ptr)
	{
		memcpy(new_ptr, old_ptr, new_size);
		k_free(old_ptr);
	}

	return new_ptr;
}
