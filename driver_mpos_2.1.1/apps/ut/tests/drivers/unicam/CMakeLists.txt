zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/zbar-0.10/include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/broadcom)

zephyr_library_sources(test_unicam.c)
