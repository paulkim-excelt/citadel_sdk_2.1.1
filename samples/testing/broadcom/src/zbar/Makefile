ifneq ($(MPOS_BASE),)
CFLAGS += -I$(MPOS_BASE)/subsys/zbar-0.10/include/
else
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/include/
endif

obj-$(CONFIG_ZBAR_STATIC_TEST) += test_zbar.o

-include $(ZEPHYR_BASE)/tests/Makefile.test
-include $(MPOS_BASE)/Makefile.inc
