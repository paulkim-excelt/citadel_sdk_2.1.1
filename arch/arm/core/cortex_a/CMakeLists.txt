zephyr_library()

zephyr_library_sources(
  fault.c
  fatal.c
  irq_manage.c
  thread.c
  cpu_idle.S
  swap.c
  swap_helper.S
  thread_abort.c
  exc_manage.c
  prep_c.c
  reset.S
  stack.c
  exc_handlers.S
  sys_fatal_error_handler.c
  arm_core_cache.c
  )

zephyr_library_sources_ifdef(CONFIG_ARM_GIC_400 gic400.c)
zephyr_library_sources_ifdef(CONFIG_ARM_CORE_MMU arm_core_mmu.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/include/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/arch/arm/soc/${SOC_PATH})
zephyr_library_include_directories(${ZEPHYR_BASE}/kernel/include)
zephyr_library_include_directories(${ZEPHYR_BASE}/include)
