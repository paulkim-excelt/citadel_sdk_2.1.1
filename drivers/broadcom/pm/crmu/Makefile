###### TOOLCHAIN FOR M0  ######
# M0 toolchain options: GCC, ARMCC

CROSS_COMPILE ?= /projects/ccxsw/toolchains/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-
M0TOOL    ?= GCC
BUILDDIR ?= $(shell pwd)/build

ifneq ($(MPOS_BASE),)
 GCCFLAGS += -I$(MPOS_BASE)/include/broadcom/
 GCCFLAGS += -I$(MPOS_BASE)/include/osal/
 GCCFLAGS += -I$(MPOS_BASE)/include/broadcom/drivers
 GCCFLAGS += -I$(MPOS_BASE)/include/osal/kernel
 GCCFLAGS += -I$(MPOS_BASE)/include/zephyr/
 GCCFLAGS += -I$(MPOS_BASE)/include/zephyr/broadcom
 GCCFLAGS += -I$(MPOS_BASE)/include/zephyr/broadcom/arch/arm
else
 GCCFLAGS += $(ZEPHYRINCLUDE) -DCONFIG_ARMV7_M -DCONFIG_CPU_CORTEX_M0
 # Hacks to build with Zephyr
 GCCFLAGS += -DCONFIG_NUM_IRQ_PRIO_BITS=4
endif
GCCFLAGS += -DCONFIG_ARM -DCONFIG_ARMV7 -DCONFIG_CPU_CORTEX_M

#GCCFLAGS += -DCITADEL_CS_APP

ifeq ($(PHX_DEEPSLEEP_SIMULATION), 1)
GCCFLAGS += -DPHX_DEEPSLEEP_SIMULATION
endif

ifeq ($(PM_TEST), 1)
    GCCFLAGS += -DPM_UNIT_TEST
endif

ifeq ($(M0TOOL), ARMCC)
M0_CC     = armcc -DM0TOOL_ARMCC --cpu=Cortex-M1 --thumb --littleend -g -c $(GCCFLAGS)
M0_LD     = armlink
M0_OBJCPY = $(CROSS_COMPILE)objcopy -j ER_RO
else
M0_CC     = $(CROSS_COMPILE)gcc -DM0TOOL_GCC -pie -O1 -mcpu=cortex-m0 -mthumb -mlittle-endian -g $(GCCFLAGS)
M0_LD     = $(CROSS_COMPILE)ld
M0_OBJCPY = $(CROSS_COMPILE)objcopy -j .text
endif

OBJDUMP=$(CROSS_COMPILE)objdump

# Building M0 also needs CROSS_COMPILE set
CC_check:
ifeq ($(M0TOOL), ARMCC)
ifndef CROSS_COMPILE
	@echo "CROSS_COMPILE of LDK toolchain undefined!"
	@exit 1
endif
endif

M0_BUILDDIR=$(BUILDDIR)/M0
TARGET_NAME=M0_image
TARGET=$(M0_BUILDDIR)/$(TARGET_NAME).bin
sources=$(wildcard *.c)
DEPS=M0.h regaccess.h ihost_config.h M0_mproc_common.h mbox_defines.h
OBJS=$(addprefix $(M0_BUILDDIR)/, $(patsubst %.c, %.o, $(sources)))

$(OBJS) : $(M0_BUILDDIR)/%.o: %.c $(DEPS) Makefile
	mkdir -p $(dir $@)
	$(M0_CC) -c $< -o $@

M0: M0_clean $(TARGET)
$(TARGET): $(OBJS) M0_scripts.lds
	rm -rf ../mproc_pm.o
	$(M0_LD) -o $(M0_BUILDDIR)/$(TARGET_NAME).out --script=M0_scripts.lds $(OBJS)
	$(OBJDUMP) -d --source --syms $(M0_BUILDDIR)/$(TARGET_NAME).out > $(M0_BUILDDIR)/$(TARGET_NAME).lst
	$(M0_OBJCPY) -O binary -S -R .ARM* -R .comment -R .debug* $(M0_BUILDDIR)/$(TARGET_NAME).out $(TARGET)

M0_clean:
	rm -rf M0_*.o M0_*.out M0_*.obj M0_*.bin M0_*.dump
	rm -rf $(BUILDDIR)/M0
clean:
	rm -f *.o  *.out *.asm
	rm -rf $(BUILDDIR)/M0
	chmod -x *
