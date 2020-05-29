ZEPHYRINCLUDE += -I$(ZEPHYR_BASE)/include/broadcom/drivers/
ZEPHYRINCLUDE += -I$(ZEPHYR_BASE)/include/drivers
ZEPHYRINCLUDE += -I$(ZEPHYR_BASE)/samples/testing/broadcom/src

ifneq ($(MPOS_BASE),)
CFLAGS += -I$(MPOS_BASE)/apps/ut/tests -I$(MPOS_BASE)/tests
ifeq ($(CONFIG_SPI_LEGACY_API), 1)
CFLAGS += -DCONFIG_SPI_LEGACY_API
endif
endif

obj-y += drivers/
obj-y += cpu/
obj-y += fp_spi/
obj-$(CONFIG_ZBAR) += zbar/
obj-y += test.o

-include $(ZEPHYR_BASE)/tests/Makefile.test

-include $(MPOS_BASE)/Makefile.inc
