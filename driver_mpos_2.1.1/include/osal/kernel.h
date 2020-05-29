/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef _kernel__h_
#define _kernel__h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <sys_clock.h>
#include <arch/cpu.h>

#define __syscall
#define __packed		__attribute__((__packed__))
#define __aligned(x)		__attribute__((__aligned__(x)))
#define K_PRIO_COOP(x)		x

/**
 * @defgroup heap_apis Heap Memory Pool APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Allocate memory from heap.
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_malloc(size_t size);

/**
 * @brief Free memory allocated from heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
extern void k_free(void *ptr);

#ifdef CONFIG_ZBAR
/**
 * @brief Allocate memory from heap, fill zero on success
 *
 * This routine provides traditional calloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param count Amount of memory blocks requested.
 * @param size Amount of one block memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_calloc(unsigned int count, unsigned int size);

/**
 * @brief Free current memory, and re-allocate memory from heap.
 *
 * This routine provides traditional realloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param old_ptr Memory to be freed.
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_realloc(void *old_ptr, unsigned int size);
#endif

/**
 * @} end defgroup heap_apis
 */


/**
 * @defgroup fifo_apis Fifo APIs
 * @ingroup kernel_apis
 * @{
 */
struct k_fifo {
	void *_q;
};

/**
 * @brief Initialize a fifo.
 *
 * This routine initializes a fifo object, prior to its first use.
 *
 * @param fifo Address of the fifo.
 *
 * @return N/A
 */
extern void k_fifo_init(struct k_fifo *fifo);

/**
 * @brief Cancel waiting on a fifo.
 *
 * This routine causes first thread pending on @a fifo, if any, to
 * return from k_fifo_get() call with NULL value (as if timeout
 * expired).
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 *
 * @return N/A
 */
extern void k_fifo_cancel_wait(struct k_fifo *fifo);

/**
 * @brief Add an element to a fifo.
 *
 * This routine adds a data item to @a fifo. A fifo data item must be
 * aligned on a 4-byte boundary, and the first 32 bits of the item are
 * reserved for the kernel's use.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 * @param data Address of the data item.
 *
 * @return N/A
 */
extern void k_fifo_put(struct k_fifo *fifo, void *data);

/**
 * @brief Get an element from a fifo.
 *
 * This routine removes a data item from @a fifo in a "first in, first out"
 * manner. The first 32 bits of the data item are reserved for the kernel's use.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param fifo Address of the fifo.
 * @param timeout Waiting period to obtain a data item (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
extern void *k_fifo_get(struct k_fifo *fifo, s32_t timeout);

/**
 * @brief Query a fifo to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads is also trying to read from the fifo.
 *
 * @note Can be called by ISRs.
 *
 * @param fifo Address of the fifo.
 *
 * @return Non-zero if the fifo is empty.
 * @return 0 if data is available.
 */
extern int k_fifo_is_empty(struct k_fifo *fifo);

/**
 * @brief Peek element at the head of fifo.
 *
 * Return element from the head of fifo without removing it. A usecase
 * for this is if elements of the fifo are themselves containers. Then
 * on each iteration of processing, a head container will be peeked,
 * and some data processed out of it, and only if the container is empty,
 * it will be completely remove from the fifo.
 *
 * @param fifo Address of the fifo.
 *
 * @return Head element, or NULL if the fifo is empty.
 */
extern void *k_fifo_peek_head(struct k_fifo *fifo);

/**
 * @} end defgroup fifo_apis
 */

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT 0

/**
 * @brief Generate infinite timeout delay.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define K_FOREVER (-1)

/**
 * @typedef k_thread_entry_t
 * @brief Thread entry point function type.
 *
 * A thread's entry point function is invoked when the thread starts executing.
 * Up to 3 argument values can be passed to the function.
 *
 * The thread terminates execution permanently if the entry point function
 * returns. The thread is responsible for releasing any shared resources
 * it may own (such as mutexes and dynamically allocated memory), prior to
 * returning.
 *
 * @param p1 First argument.
 * @param p2 Second argument.
 * @param p3 Third argument.
 *
 * @return N/A
 */
typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);

/**
 * @defgroup thread_apis Thread APIs
 * @ingroup kernel_apis
 * @{
 */

struct k_thread {
	void *_t;
};

typedef struct k_thread *k_tid_t;

/* Using typedef deliberately here, this is quite intended to be an opaque
 * type. K_THREAD_STACK_BUFFER() should be used to access the data within.
 *
 * The purpose of this data type is to clearly distinguish between the
 * declared symbol for a stack (of type k_thread_stack_t) and the underlying
 * buffer which composes the stack data actually used by the underlying
 * thread; they cannot be used interchangably as some arches precede the
 * stack buffer region with guard areas that trigger a MPU or MMU fault
 * if written to.
 *
 * APIs that want to work with the buffer inside should continue to use
 * char *.
 *
 * Stacks should always be created with K_THREAD_STACK_DEFINE().
 */
struct __packed _k_thread_stack_element {
	char data;
};
typedef struct _k_thread_stack_element *k_thread_stack_t;

/**
 * @brief Declare a toplevel thread stack memory region
 *
 * This declares a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN and put in
 * 'noinit' section so that it isn't zeroed at boot
 *
 * The declared symbol will always be a k_thread_stack_t which can be passed to
 * k_thread_create, but should otherwise not be manipulated. If the buffer
 * inside needs to be examined, use K_THREAD_STACK_BUFFER().
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of k_thread_create(), it may not be the same as the
 * 'size' parameter. Use K_THREAD_STACK_SIZEOF() instead.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 */
#define K_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) sym[size]

/**
 * @brief Create a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * Historically, users often would use the beginning of the stack memory region
 * to store the struct k_thread data, although corruption will occur if the
 * stack overflows this region and stack protection features may not detect this
 * situation.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay (in milliseconds), or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 */
extern k_tid_t k_thread_create(struct k_thread *new_thread,
			       k_thread_stack_t stack,
			       size_t stack_size,
			       k_thread_entry_t entry,
			       void *p1, void *p2, void *p3,
			       int prio, u32_t options, s32_t delay);


/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration
 * milliseconds.
 *
 * @param duration Number of milliseconds to sleep.
 *
 * @return N/A
 */
__syscall void k_sleep(s32_t duration);

/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @return N/A
 */
__syscall void k_busy_wait(u32_t usec_to_wait);

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 *
 * @return N/A
 */
__syscall void k_yield(void);

/**
 * @brief Wake up a sleeping thread.
 *
 * This routine prematurely wakes up @a thread from sleeping.
 *
 * If @a thread is not currently sleeping, the routine has no effect.
 *
 * @param thread ID of thread to wake.
 *
 * @return N/A
 */
__syscall void k_wakeup(k_tid_t thread);

/**
 * @brief Get thread ID of the current thread.
 *
 * @return ID of current thread.
 */
__syscall k_tid_t k_current_get(void);

/**
 * @brief Abort a thread.
 *
 * This routine permanently stops execution of @a thread. The thread is taken
 * off all kernel queues it is part of (i.e. the ready queue, the timeout
 * queue, or a kernel object wait queue). However, any kernel resources the
 * thread might currently own (such as mutexes or memory blocks) are not
 * released. It is the responsibility of the caller of this routine to ensure
 * all necessary cleanup is performed.
 *
 * @param thread ID of thread to abort.
 *
 * @return N/A
 */
__syscall void k_thread_abort(k_tid_t thread);


/**
 * @brief Start an inactive thread
 *
 * If a thread was created with K_FOREVER in the delay parameter, it will
 * not be added to the scheduling queue until this function is called
 * on it.
 *
 * @param thread thread to start
 */
__syscall void k_thread_start(k_tid_t thread);

/**
 * @brief Get a thread's priority.
 *
 * This routine gets the priority of @a thread.
 *
 * @param thread ID of thread whose priority is needed.
 *
 * @return Priority of @a thread.
 */
__syscall int k_thread_priority_get(k_tid_t thread);

/**
 * @brief Set a thread's priority.
 *
 * This routine immediately changes the priority of @a thread.
 *
 * Rescheduling can occur immediately depending on the priority @a thread is
 * set to:
 *
 * - If its priority is raised above the priority of the caller of this
 * function, and the caller is preemptible, @a thread will be scheduled in.
 *
 * - If the caller operates on itself, it lowers its priority below that of
 * other threads in the system, and the caller is preemptible, the thread of
 * highest priority will be scheduled in.
 *
 * Priority can be assigned in the range of -CONFIG_NUM_COOP_PRIORITIES to
 * CONFIG_NUM_PREEMPT_PRIORITIES-1, where -CONFIG_NUM_COOP_PRIORITIES is the
 * highest priority.
 *
 * @param thread ID of thread whose priority is to be set.
 * @param prio New priority.
 *
 * @warning Changing the priority of a thread currently involved in mutex
 * priority inheritance may result in undefined behavior.
 *
 * @return N/A
 */
__syscall void k_thread_priority_set(k_tid_t thread, int prio);

/**
 * @brief Suspend a thread.
 *
 * This routine prevents the kernel scheduler from making @a thread the
 * current thread. All other internal operations on @a thread are still
 * performed; for example, any timeout it is waiting on keeps ticking,
 * kernel objects it is waiting on are still handed to it, etc.
 *
 * If @a thread is already suspended, the routine has no effect.
 *
 * @param thread ID of thread to suspend.
 *
 * @return N/A
 */
__syscall void k_thread_suspend(k_tid_t thread);

/**
 * @brief Resume a suspended thread.
 *
 * This routine allows the kernel scheduler to make @a thread the current
 * thread, when it is next eligible for that role.
 *
 * If @a thread is not currently suspended, the routine has no effect.
 *
 * @param thread ID of thread to resume.
 *
 * @return N/A
 */
__syscall void k_thread_resume(k_tid_t thread);

/**
 * @} end defgroup thread_apis
 */

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32-bits of the elapsed time since the system
 * booted, in milliseconds.
 *
 * This routine can be more efficient than k_uptime_get(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold a system uptime time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @return Current uptime.
 */
extern u32_t k_uptime_get_32(void);

/**
 * @brief Read the hardware clock.
 *
 * This routine returns the current time, as measured by the system's hardware
 * clock.
 *
 * @return Current hardware clock up-counter (in cycles).
 */
#define k_cycle_get_32()	_arch_k_cycle_get_32()

/**
 * @} end addtogroup clock_apis
 */

/**
 * @defgroup semaphore_apis Semaphore APIs
 * @ingroup kernel_apis
 * @{
 */

struct k_sem {
	void *_s;
};

/**
 * @brief Initialize a semaphore.
 *
 * This routine initializes a semaphore object, prior to its first use.
 *
 * @param sem Address of the semaphore.
 * @param initial_count Initial semaphore count.
 * @param limit Maximum permitted semaphore count.
 *
 * @return N/A
 */
extern void k_sem_init(struct k_sem *sem, unsigned int initial_count,
			unsigned int limit);

/**
 * @brief Take a semaphore.
 *
 * This routine takes @a sem.
 *
 * @note Can be called by ISRs, but @a timeout must be set to K_NO_WAIT.
 *
 * @param sem Address of the semaphore.
 * @param timeout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @note When porting code from the nanokernel legacy API to the new API, be
 * careful with the return value of this function. The return value is the
 * reverse of the one of nano_sem_take family of APIs: 0 means success, and
 * non-zero means failure, while the nano_sem_take family returns 1 for success
 * and 0 for failure.
 *
 * @retval 0 Semaphore taken.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
extern int k_sem_take(struct k_sem *sem, s32_t timeout);

/**
 * @brief Give a semaphore.
 *
 * This routine gives @a sem, unless the semaphore is already at its maximum
 * permitted count.
 *
 * @note Can be called by ISRs.
 *
 * @param sem Address of the semaphore.
 *
 * @return N/A
 */
extern void k_sem_give(struct k_sem *sem);

/**
 * @brief Get a semaphore's count.
 *
 * This routine returns the current count of @a sem.
 *
 * @param sem Address of the semaphore.
 *
 * @return Current semaphore count.
 */
unsigned int k_sem_count_get(struct k_sem *sem);

/**
 * @} end defgroup semaphore_apis
 */

/**
 * @brief Check if we are in interrupt mode
 *
 * This routine returns true if we are executing an ISR
 *
 * @return true if executing in ISR mode, false otherwise
 */
bool k_is_in_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel__h_ */
