#
# Copyright Broadcom corporation (2017)
#

#########################
## Toolchain variables ##
#########################
# Suppress target recipe prints by default
Q ?= @
# Default compiler
CROSS_COMPILE ?= /projects/ccxsw/toolchains/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
MKDIR=mkdir
CP=cp
CD=cd

########################
## Build output paths ##
########################
MPOS_BASE = $(CURDIR)
BUILDDIR = $(MPOS_BASE)/build
CRMUDIR = $(MPOS_BASE)/drivers/broadcom/pm/crmu

############################
## Configuration includes ##
############################
# Build configuration definition
include $(MPOS_BASE)/config/make.config
# Board specific configuration
-include $(MPOS_BASE)/boards/$(BOARD)/.config
# Driver specific configuration
include $(MPOS_BASE)/config/drivers.config

# Target name
ifneq ($(CONFIG_SOC_UNPROGRAMMED), n)
	PART = up
else
	PART = ua
endif
ifneq ($(CONFIG_XIP), n)
	EXEC_TYPE = xip
else
	EXEC_TYPE = le
endif

TARGET = $(BOARD)_$(PART)_$(EXEC_TYPE)

# Enable boot banner
CONFIG_BOOT_BANNER ?= y

###############
## Functions ##
###############
# Function to export variable
define export_var
export $(1)
endef
# Function to add a define to CFLAGS
define add_def_flag
ifneq ($(2), n)
	CFLAGS += -D$(1)=$(2)
endif
endef

####################
## Compiler flags ##
####################
# Arch specific compile flags
CFLAGS += -DCONFIG_ARM -DCONFIG_ARMV7 -DCONFIG_CPU_CORTEX_A
ARCH_FLAGS += -march=armv7-a -mtune=cortex-a7 -mthumb
ifeq ($(CONFIG_FPU_ENABLED), y)
ARCH_FLAGS += -mfpu=neon-vfpv4 -mfloat-abi=hard
endif
# Add -D option to CFLAGS for all CONFIG_XXX varaibles defined
$(foreach V,$(sort $(.VARIABLES)), $(if $(findstring CONFIG_,$(V)), $(if $($(V)), $(eval $(call add_def_flag,$(V),$($(V)))))))

## Header include paths ##
#
# NOTE: include/broadcom is intentionally not included into the compile flags
# This should not be included in CFLAGS and any header files directly under
# include/broadcom/*.h should be included in sources as #include <broadcom/xxx.h>
# For example #include <broadcom/dma.h>
# The reason for this is the header files under here (include/broadcom/*.h) are
# zephyr modified header files. Which means the zephyr repo will have 2 versions
# of these headers (unmodified and brcm-modified). So to make sure the drivers include
# the right version (brcm-modified), the headers will need to be included as <broadcom/xxx.h>
CFLAGS += -I$(MPOS_BASE)/include
CFLAGS += -I$(MPOS_BASE)/include/osal/
CFLAGS += -I$(MPOS_BASE)/include/zephyr/
CFLAGS += -I$(MPOS_BASE)/include/zephyr/broadcom
CFLAGS += -I$(MPOS_BASE)/include/zephyr/broadcom/arch/arm
CFLAGS += -I$(MPOS_BASE)/include/broadcom/drivers
CFLAGS += -I$(MPOS_BASE)/boards/$(BOARD)
CFLAGS += -include $(MPOS_BASE)/boards/$(BOARD)/board.h -include $(MPOS_BASE)/arch/arm/soc/bcm_citadel/bcm5820x/soc.h

# Add OS specific include paths
CFLAGS += $(patsubst %, -I$(MPOS_BASE)/os/$(CONFIG_RTOS)/%, $(RTOS_INCLUDE_REL_PATH))

## Additional Compiler flags ##
ifneq ($(CONFIG_RTOS), no_os)
CFLAGS += -DKERNEL
endif
## Debug flags
ifeq ($(DEBUG), 1)
CFLAGS += -Wall -Wextra -g -Og $(ARCH_FLAGS) -DCONFIG_DEBUG
CFLAGS += -DCONFIG_SYS_LOG_DEFAULT_LEVEL=SYS_LOG_LEVEL_DEBUG
else
CFLAGS += -Wall -Wextra -g -Werror -Os $(ARCH_FLAGS)
CFLAGS += -DCONFIG_SYS_LOG_DEFAULT_LEVEL=SYS_LOG_LEVEL_WARNING
endif

##################
## Linker flags ##
##################
LIBGCC_D = $(shell dirname `${CC} $(ARCH_FLAGS) -print-libgcc-file-name`)
LIBC_D = $(shell dirname `${CC} $(ARCH_FLAGS) -print-file-name=libc.a`)
LDFLAGS += -L$(LIBC_D) -lc -L$(LIBGCC_D) -lgcc -lnosys
LDFLAGS += --entry=_start -T $(BUILDDIR)/$(TARGET).ld -Map=$(BUILDDIR)/$(TARGET).map
LDFILE = $(MPOS_BASE)/config/linker_$(EXEC_TYPE).lds

## Export variables for sub makes
$(foreach V,$(sort $(.VARIABLES)), $(if $(findstring CONFIG_,$(V)), $(eval $(call export_var,$(V)))))
export CC CFLAGS LD MPOS_BASE BUILDDIR Q MAKEOPTS AS AR CP CD MKDIR OBJCOPY OBJDUMP

## Top level directories to build
SUBDIRS = apps arch boards drivers os osal

#########################
## Targets and Recipes ##
#########################
all: $(TARGET)

$(TARGET): GENERATE_LD M0_BUILD subdirs
	$(Q)echo [Linking ... $@]
	$(Q)$(LD) -o $(BUILDDIR)/$@ $(shell find $(BUILDDIR) -name *.o -not -path '$(BUILDDIR)/M0/*') \
	        $(shell find $(BUILDDIR) -not -path '$(BUILDDIR)/M0/*' | grep \\.a$) $(LDFLAGS)
	$(Q)echo [Generating ... $@.lst]
	$(Q)$(OBJDUMP) -d --source --syms $(BUILDDIR)/$@ > $(BUILDDIR)/$@.lst
	$(Q)echo [Creating ... $@.bin]
	$(Q)$(OBJCOPY) -O binary -S -R .ARM* -R .comment -R .debug* $(BUILDDIR)/$@  $(BUILDDIR)/$@.bin
	$(Q)$(CP) $(BUILDDIR)/$@.bin .

$(BUILDDIR)/lib$(TARGET).a: M0_BUILD subdirs
	$(Q)-rm -f $@

library: $(BUILDDIR)/lib$(TARGET).a
	$(Q)echo [Archive ... $(notdir $<)]
	$(Q)$(AR) rcs $< $(shell find $(BUILDDIR) -name *.o -not -path '$(BUILDDIR)/M0/*' -not -path '$(BUILDDIR)/os/no_os/*' -not -path '$(BUILDDIR)/osal/no_os/*' ! -name main.o ! -name app_main.o) \
	        $(shell find $(BUILDDIR) -not -path '$(BUILDDIR)/M0/*' | grep \\.a$)
	$(Q)echo [Generating ... $(notdir $(<:.a=.lst))]
	$(Q)$(OBJDUMP) -d --source --syms $< > $(<:.a=.lst)
	$(Q)$(CP) $< .

GENERATE_LD:
	$(Q)$(MKDIR) -p $(BUILDDIR)
	$(Q)echo [Generating ... $(TARGET).ld]
	$(Q)$(CC) $(CFLAGS) $(DEFINES) -E -P -o $(BUILDDIR)/$(TARGET).ld -x c-header $(LDFILE)

M0_BUILD:
	$(Q)$(MKDIR) -p $(BUILDDIR)
	$(Q)echo [Buidling and Copying CRMU Handler binaries ...]
	$(Q)$(CD) $(CRMUDIR) && make M0 > $(BUILDDIR)/m0_build_log.txt 2>&1
	$(Q)echo [M0 Build complete !!!]

clean:
	$(Q)echo [Cleaning]
	$(Q)-rm -rf *.bin *.a
	$(Q)-rm -rf $(BUILDDIR)

include $(MPOS_BASE)/Makefile.inc
