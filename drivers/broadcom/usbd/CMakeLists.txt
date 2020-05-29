# These usbd driver files are for Zephyr only
# Zephyr provides the USB device core driver and the
# various USB class drivers (CDC-ACM, Mass Storage, DFU etc ...)
# The mpos usbd driver has all these drivers jumbled into one (usb/usbd)
# In order to re-use code between Zephyr and mpos, the mpos usbd
# driver needs to be re-structured and ridden of all control vault
# specific changes. To avoid this the device controller apis
# are re-written for Zephyr in this driver

zephyr_library_sources(usb_dc_bcm5820x.c)
