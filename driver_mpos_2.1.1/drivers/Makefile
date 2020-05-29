# Broadcom drivers
SUBDIRS += broadcom

# Drivers borrowed/modified from Zephyr
SUBDIRS-$(CONFIG_BRCM_DRIVER_UART_NS16550) += console serial
SUBDIRS += interrupt_controller
SUBDIRS += timer

include $(MPOS_BASE)/Makefile.inc
