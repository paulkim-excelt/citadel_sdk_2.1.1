/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PORT_H__
#define __PORT_H__

#ifndef _ASMLANGUAGE
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned long int strtoul(const char *str, char **endptr, int base);
long int strtol(const char *str, char **endptr, int base);
int atoi(const char *s);
int abs(int num);
void qsort(void *base, size_t numelem, size_t size,
	int (*cmp)(const void *e1, const void *e2));
char *strdup(const char *s);
void *k_calloc(unsigned int count, unsigned int size);
void *k_realloc(void *old_ptr, unsigned int new_size);

#ifdef __cplusplus
}
#endif
#endif /* !_ASMLANGUAGE */
#endif  /* __PORT_H__ */
