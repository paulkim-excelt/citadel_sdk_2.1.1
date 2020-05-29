/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
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

/*
 *  Broadcom Common Firmware Environment (CFE)
 *
 *  Queue Management routines	File: lib_queue.c
 *
 *  Routines to manage doubly-linked queues.
 *
 *  Author:  Mitch Lichtenberg
 *
 */

#include "cfeglue.h"
#include "lib_queue.h"

/*
 *  Q_ENQUEUE(qb,item)
 *
 *  Add item to a queue
 *
 *  Input Parameters:
 *	qb - queue block
 *	item - item to add
 *
 *  Return Value:
 *	Nothing.
 */

void q_enqueue(queue_t *qb, queue_t *item)
{
	qb->q_prev->q_next = item;
	item->q_next = qb;
	item->q_prev = qb->q_prev;
	qb->q_prev = item;
}


/*
 *  Q_DEQUEUE(element)
 *
 *  Remove an element from the queue
 *
 *  Input Parameters:
 *	element - element to remove
 *
 *  Return Value:
 *	Nothing.
 */

void q_dequeue(queue_t *item)
{
	item->q_prev->q_next = item->q_next;
	item->q_next->q_prev = item->q_prev;
}

/*
 *  Q_DEQNEXT(qb)
 *
 *  Dequeue next element from the specified queue
 *
 *  Input Parameters:
 *	qb - queue block
 *
 *  Return Value:
 *	next element, or NULL
 */

queue_t *q_deqnext(queue_t *qb)
{
	if (qb->q_next == qb)
		return NULL;

	qb = qb->q_next;

	qb->q_prev->q_next = qb->q_next;
	qb->q_next->q_prev = qb->q_prev;

	return qb;
}

/*
 *  Q_MAP(qb)
 *
 *  "Map" a queue, calling the specified function for each
 *  element in the queue
 *
 *  If the function returns nonzero, q_map will terminate.
 *
 *  Input Parameters:
 *	qb - queue block
 *	fn - function pointer
 *	a,b - parameters for the function
 *
 *  Return Value:
 *	return value from function, or zero if entire queue
 *	was mapped.
 */

int q_map(queue_t *qb, int (*func)(queue_t *, unsigned int, unsigned int),
	  unsigned int a, unsigned int b)
{
	queue_t *qe;
	queue_t *nextq;
	int res;

	qe = qb;

	qe = qb->q_next;

	while (qe != qb) {
		nextq = qe->q_next;
		res = (*func)(qe, a, b);
		if (res)
			return res;
		qe = nextq;
	}

	return 0;
}
/*
 *  Q_COUNT(qb)
 *
 *  Counts the elements on a queue (not interlocked)
 *
 *  Input Parameters:
 *	qb - queue block
 *
 *  Return Value:
 *	number of elements
 */
int q_count(queue_t *qb)
{
	queue_t *qe;
	int res = 0;

	qe = qb;

	while (qe->q_next != qb) {
		qe = qe->q_next;
		res++;
	}

	return res;
}

/*
 *  Q_FIND(qb,item)
 *
 *  Determines if a particular element is on a queue.
 *
 *  Input Parameters:
 *	qb - queue block
 *	item - queue element
 *
 *  Return Value:
 *	0 - not on queue
 *	>0 - position on queue
 */

int q_find(queue_t *qb, queue_t *item)
{
	queue_t *q;
	int res = 1;

	q = qb->q_next;

	while (q != item) {
		if (q == qb)
			return 0;
		q = q->q_next;
		res++;
	}

	return res;
}

/* TODO: Remove the below routines for Citadel. No callers
 * Currently putting them under #ifdef
 */
#ifdef USE_KMALLOC
void *KMALLOC(unsigned int size, unsigned int align)
{
	void *buf = NULL;

	if (align)
		buf =  bcm_malloc((size + align - 1) / align);
	else
		buf =  bcm_malloc(size);

	return buf;
}

void *KMALLOC_UNALIGNED(unsigned int size)
{
	return KMALLOC(size, 0);
}

void KFREE(void *ptr)
{
	bcm_free(ptr);
}
#endif /* USE_KMALLOC */
