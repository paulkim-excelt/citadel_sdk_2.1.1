ifneq ($(MPOS_BASE),)
CFLAGS += -I$(MPOS_BASE)/subsys/zbar-0.10/include/ -I$(MPOS_BASE)/drivers/broadcom
else
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/include -I$(ZEPHYR_BASE)/drivers/broadcom
endif

obj-y += test_unicam.o

-include $(ZEPHYR_BASE)/tests/Makefile.test
-include $(MPOS_BASE)/Makefile.inc
