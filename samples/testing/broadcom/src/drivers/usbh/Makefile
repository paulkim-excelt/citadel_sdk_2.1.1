ifeq ($(MPOS_BASE),)
obj-$(CONFIG_BRCM_DRIVER_USBH) += test_usbh_zephyr.o
else
CFLAGS += -I$(MPOS_BASE)/drivers/broadcom/usb/usbhost
obj-$(CONFIG_BRCM_DRIVER_USB) += test_usbh.o
endif

-include $(ZEPHYR_BASE)/tests/Makefile.test
-include $(MPOS_BASE)/Makefile.inc
