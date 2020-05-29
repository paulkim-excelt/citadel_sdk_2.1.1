zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/zbar-0.10/include)

zephyr_library_sources_ifdef(CONFIG_ZBAR_STATIC_TEST test_zbar.c)
