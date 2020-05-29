ifeq ($(CONFIG_ARM_CORE_MMU), y)
OBJS += arm_core_mmu.o
endif
OBJS += arm_core_cache.o
OBJS += exc_manage.o
OBJS += exc_handler.o
OBJS += fault.o

CFLAGS += -DCONFIG_FAULT_DUMP=2

include $(MPOS_BASE)/Makefile.inc
