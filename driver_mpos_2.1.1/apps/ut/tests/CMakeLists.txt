zephyr_library_sources(test.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/include/broadcom/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/include/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/boards/arm/${BOARD})
zephyr_library_include_directories(${ZEPHYR_BASE}/samples/testing/broadcom/src)

add_subdirectory(drivers)
add_subdirectory(cpu)
add_subdirectory(fp_spi)
add_subdirectory_ifdef(CONFIG_ZBAR zbar)

#Zephyr v1.9 to v1.14 function name changes
zephyr_compile_definitions(-D_timer_cycle_get_32=z_timer_cycle_get_32)
zephyr_compile_definitions(-D_sys_clock_driver_init=z_clock_driver_init)
zephyr_compile_definitions(-Dmin=MIN -Dmax=MAX)
zephyr_compile_definitions(-DUSBD_CB_HAS_PARAM)
