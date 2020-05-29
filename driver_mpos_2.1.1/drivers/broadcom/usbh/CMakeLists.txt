add_subdirectory(host)
add_subdirectory(class)

zephyr_library_sources(usb_host.c usb_host_bcm5820x.c)
