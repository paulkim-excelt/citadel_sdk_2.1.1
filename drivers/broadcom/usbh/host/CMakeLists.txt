zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/broadcom/usbh)

zephyr_library_sources(ohci.c)
zephyr_library_sources(ehci.c)
