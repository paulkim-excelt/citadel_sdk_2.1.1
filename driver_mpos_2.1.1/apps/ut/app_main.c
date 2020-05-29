/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Sample application that runs all automated unit tests
 */

#include <string.h>
#include <kernel.h>
#include <init.h>
#include <ztest.h>
#include <shell/shell.h>
#include <logging/sys_log.h>

#define TEST_STACK_SIZE 30000
extern const struct shell_cmd test_commands[];

void run_tests(void *p1, void *p2, void *p3)
{
	const struct shell_cmd *tc = &test_commands[0];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	TC_PRINT("Running automated unit tests\n");
	TC_PRINT("============================\n");
	if (strcmp(tc->cmd_name, "AUTOMATED_TESTS") == 0) {
		tc++;
		/* Call each automated test command in a loop */
		while (strcmp(tc->cmd_name, "all")) {
			TC_PRINT("\nRunning \"%s\"\n", tc->cmd_name);
			TC_PRINT("==================\n");
			k_sleep(1000);

			tc->cb(1, (char **)&tc->cmd_name);
			tc++;
#ifndef KERNEL
			/* FIXME: Remove this when all drivers support
			 * no_os mode
			 */
			//break;
#endif
			k_sleep(1000);
		}
	}

	TC_PRINT("Completed running all tests!\n");

	/* Spin forever after running tests */
	while (1) {
		k_sleep(1);
	}
}

#ifdef KERNEL
static K_THREAD_STACK_DEFINE(ut_task_stack, TEST_STACK_SIZE);
int app_main(struct device *arg)
{
	k_tid_t id;
	static struct k_thread ut_task;

	ARG_UNUSED(arg);

	id = k_thread_create(&ut_task, ut_task_stack, TEST_STACK_SIZE, run_tests,
			     NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	if (id == NULL) {
		SYS_LOG_ERR("Error Creating thread to run unit tests!");
		return -1;
	}

	return 0;
}

SYS_INIT(app_main, POST_KERNEL, 99);
#else
static u8_t __aligned(16) test_stack[TEST_STACK_SIZE];
void app_main(void)
{
	u8_t *stack_ptr = &test_stack[TEST_STACK_SIZE];

	__asm__ volatile ("mov sp, %[stack_ptr]"
			  : : [stack_ptr] "r" (stack_ptr));
	run_tests(NULL, NULL, NULL);

	while(1);
}
#endif
