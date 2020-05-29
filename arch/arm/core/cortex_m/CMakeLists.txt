# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.S
  exc_exit.S
  fault.c
  fault_s.S
  fatal.c
  irq_init.c
  irq_manage.c
  vector_table.S
  reset.S
  nmi_on_reset.S
  prep_c.c
  scb.c
  nmi.c
  exc_manage.c
  swap.c
  swap_helper.S
  sys_fatal_error_handler.c
  thread.c
  thread_abort.c
  )

zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0 irq_relay.S)
zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
