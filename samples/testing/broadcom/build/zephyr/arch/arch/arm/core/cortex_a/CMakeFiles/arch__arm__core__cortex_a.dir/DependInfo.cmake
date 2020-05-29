# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  "C"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/cpu_idle.S" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/cpu_idle.S.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/exc_handlers.S" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/exc_handlers.S.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/reset.S" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/reset.S.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/swap_helper.S" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/swap_helper.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "KERNEL"
  "PATH_TO_KEYS=\"/home/paulkim/work/sdk/citadel/samples/testing/broadcom/src/drivers/pka/\""
  "PATH_TO_M0_IMAGE_BIN=\"/home/paulkim/work/sdk/citadel/drivers/broadcom/pm/crmu/build/M0/\""
  "USBD_CB_HAS_PARAM"
  "_FORTIFY_SOURCE=2"
  "__RSTR_STACK_START__=_image_ram_start"
  "__ZEPHYR_SUPERVISOR__"
  "__ZEPHYR__=1"
  "_sys_clock_driver_init=z_clock_driver_init"
  "_timer_cycle_get_32=z_timer_cycle_get_32"
  "max=MAX"
  "min=MIN"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "../../../../include/drivers"
  "../../../../arch/arm/soc/brcm_bcm5820x"
  "../../../../kernel/include"
  "../../../../include"
  "../../../../arch/arm/include"
  "zephyr/include/generated"
  "../../../../soc/arm/brcm_bcm5820x"
  "../../../../lib/libc/minimal/include"
  "../../../../ext/lib/fnmatch/."
  "../../../../subsys/testsuite/include"
  "../../../../subsys/testsuite/ztest/include"
  "/home/paulkim/work/toolchains/gcc-arm-none-eabi-6-2017-q1-update/bin/../lib/gcc/arm-none-eabi/6.3.1/include"
  "/home/paulkim/work/toolchains/gcc-arm-none-eabi-6-2017-q1-update/bin/../lib/gcc/arm-none-eabi/6.3.1/include-fixed"
  )
set(CMAKE_DEPENDS_CHECK_C
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/arm_core_cache.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/arm_core_cache.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/arm_core_mmu.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/arm_core_mmu.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/exc_manage.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/exc_manage.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/fatal.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/fatal.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/fault.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/fault.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/gic400.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/gic400.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/irq_manage.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/irq_manage.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/prep_c.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/prep_c.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/stack.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/stack.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/swap.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/swap.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/sys_fatal_error_handler.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/sys_fatal_error_handler.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/thread.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/thread.c.obj"
  "/home/paulkim/work/sdk/citadel/arch/arm/core/cortex_a/thread_abort.c" "/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/arch/arch/arm/core/cortex_a/CMakeFiles/arch__arm__core__cortex_a.dir/thread_abort.c.obj"
  )
set(CMAKE_C_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_C
  "KERNEL"
  "PATH_TO_KEYS=\"/home/paulkim/work/sdk/citadel/samples/testing/broadcom/src/drivers/pka/\""
  "PATH_TO_M0_IMAGE_BIN=\"/home/paulkim/work/sdk/citadel/drivers/broadcom/pm/crmu/build/M0/\""
  "USBD_CB_HAS_PARAM"
  "_FORTIFY_SOURCE=2"
  "__RSTR_STACK_START__=_image_ram_start"
  "__ZEPHYR_SUPERVISOR__"
  "__ZEPHYR__=1"
  "_sys_clock_driver_init=z_clock_driver_init"
  "_timer_cycle_get_32=z_timer_cycle_get_32"
  "max=MAX"
  "min=MIN"
  )

# The include file search paths:
set(CMAKE_C_TARGET_INCLUDE_PATH
  "../../../../include/drivers"
  "../../../../arch/arm/soc/brcm_bcm5820x"
  "../../../../kernel/include"
  "../../../../include"
  "../../../../arch/arm/include"
  "zephyr/include/generated"
  "../../../../soc/arm/brcm_bcm5820x"
  "../../../../lib/libc/minimal/include"
  "../../../../ext/lib/fnmatch/."
  "../../../../subsys/testsuite/include"
  "../../../../subsys/testsuite/ztest/include"
  "/home/paulkim/work/toolchains/gcc-arm-none-eabi-6-2017-q1-update/bin/../lib/gcc/arm-none-eabi/6.3.1/include"
  "/home/paulkim/work/toolchains/gcc-arm-none-eabi-6-2017-q1-update/bin/../lib/gcc/arm-none-eabi/6.3.1/include-fixed"
  )

# Targets to which this target links.
set(CMAKE_TARGET_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
