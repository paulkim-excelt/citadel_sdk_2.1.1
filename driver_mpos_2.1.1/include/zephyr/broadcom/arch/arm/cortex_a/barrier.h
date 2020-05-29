/*
 * Copyright (c) 2017 Broadcom Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BARRIER_H
#define __BARRIER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
/* nothing */
#else

#ifdef CONFIG_CPU_CORTEX_A

/* Memory and Synchronization barriers */
#define data_mem_barrier()	({__asm__ volatile("dmb\n"); })
#define data_sync_barrier()	({__asm__ volatile("dsb\n"); })
#define instr_sync_barrier()	({__asm__ volatile("isb\n"); })

#else
#error Unsupported ARM architecture
#endif /* CONFIG_CPU_CORTEX_A */

#endif

#ifdef __cplusplus
}
#endif

#endif
