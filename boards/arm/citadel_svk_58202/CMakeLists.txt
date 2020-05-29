# Include and board specific sources, here
# For example, pin mux settings etc.

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/include/broadcom/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/boards/arm/${BOARD})

zephyr_library_sources(pinmux.c)

