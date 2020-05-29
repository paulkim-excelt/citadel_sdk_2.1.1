/*
 * Copyright (c) 2018, Broadcom Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Stub for shell infrastructure in Zephyr
 */

typedef int (*shell_cmd_function_t)(int argc, char *argv[]);

struct shell_cmd {
       const char *cmd_name;
       shell_cmd_function_t cb;
       const char *help;
};

#define SHELL_REGISTER(A, B)
