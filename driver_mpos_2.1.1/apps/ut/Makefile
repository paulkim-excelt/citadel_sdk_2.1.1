obj-y += ztest/
obj-y += tests/
obj-y += app_main.o

export CPU_TEST=y

CFLAGS += -DCONFIG_ZTEST -I$(MPOS_BASE)/apps/ut/ztest/include

include $(MPOS_BASE)/Makefile.inc
