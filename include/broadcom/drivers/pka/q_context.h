/*******************************************************************
 *
 * Copyright 2015
 * Broadcom Corporation
 * 16215 Alton Parkway
 * PO Box 57013
 * Irvine CA 92619-7013
 *
 * Broadcom provides the current code only to licensees who
 * may have the non-exclusive right to use or modify this
 * code according to the license.
 * This program may be not sold, resold or disseminated in
 * any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************/

extern BCM_SCAPI_STATUS YieldHandler(void); /* scapi_pka.c */
#define OS_YIELD_FUNCTION YieldHandler

/*Removed below macro to fix coverity issue */


#ifdef USE_STATIC_QLIP_HEAP
#define BASIC_TYPES_DEFINED
#include "volatile_mem.h"
#define QLIP_CONTEXT (q_lip_ctx_t *)VOLATILE_MEM_PTR->qlip_ctx_ptr
#define LOCAL_QLIP_CONTEXT_DEFINITION 	{}

	/* q_ctx_init is called once at boot */
#else
/* local_xxx defines the local qlip context on the stack */
#define QLIP_HEAP_SIZE 4096 /* Heapsize is in Words */
#define LOCAL_QLIP_CONTEXT_DEFINITION \
q_lip_ctx_t qlip_ctx; \
u32_t qlip_heap[QLIP_HEAP_SIZE]; \
q_ctx_init(&qlip_ctx,&qlip_heap[0],QLIP_HEAP_SIZE,(q_yield)(OS_YIELD_FUNCTION));
#define QLIP_CONTEXT &qlip_ctx 
#endif

