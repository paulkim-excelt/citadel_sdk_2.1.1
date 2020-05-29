#
# Copyright Broadcom Inc. (2018)
#

# Common make configuration flags
include ../make.cfg

# Target name
TARGET = stage2_bl

# Top 128 KB is reserved for S2BL (Stage 2 boot loader)
# The flash address for the XIP image to be validated and executed
XIP_IMAGE_START_ADDR = \(FLASH_START+DEF_AAI_IMAGE_OFFSET\)

# Stage 2 Boot loader will fit into the region between 0x24000000 & 0x24020000
# The top IPROC stack region upto 0x2400CC00 will be used as data memory for
# no-load sections (bss/heap/stack). And the region from the default SBI start
# (0x2400D360 for RSA2K signed image) to 0x24020000 will be used for code/data/
# rodata/fastcode (loadable sections). This will allow the target image, if
# linked to ram (target=ram), to avail the 896 KB of SRAM starting from
# 0x24000000. Note that the sample AAI that will be built with target=ram should
# now link to the address 0x24020000 + SBI_HEADER_SIZE.
S2BL_SIZE = $(DEF_S2BL_SIZE)

# Defaults
target ?= ram

# Use the section from end of SBI header upto 128 KB from the start of SRAM
# for code/data (loadable sections)
CFLAGS += -DRO_MEM_START=DEF_SBI_BIN_START_RSA2K
CFLAGS += -DRO_MEM_LENGTH=SRAM_START+$(S2BL_SIZE)-\(RO_MEM_START\)
# Use the IPROC stack start region (0x24000000 - 0x2400CC00) for bss/heap/stack
CFLAGS += -DRW_MEM_START=SRAM_START
CFLAGS += -DRW_MEM_LENGTH=LE_IMAGE_START-SRAM_START

CFLAGS += -DIPROC_SBI_FLASH_ADDRESS=$(XIP_IMAGE_START_ADDR)
CFLAGS += -DIPROC_SBI_LOAD_ADDRESS=\(SRAM_START+$(S2BL_SIZE)\)

CFLAGS += -fno-tree-scev-cprop -ffunction-sections -fdata-sections

ifeq ($(target), flash)
CFLAGS += -DV_QFLASH -DALIGN_16B
endif

# List of driver dependencies
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/crypto
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/pka
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/sotp
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/smau
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dma
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dmu

# Include driver dependencies for the build
SUBDIRS += $(BRCM_DRIVER_DEPS)

# Linker flags
LDFLAGS += -L$(COMMON)/lib --gc-sections
LDSFILE = $(CWD)/$(TARGET).lds
LDFILE = $(BUILDDIR)/$(TARGET).ld

# Required only for SBI Debug build
CFLAGS += -DSBI_DEBUG -DCONFIG_DEBUG

# USBD include path 
CFLAGS += -I$(MPOS_BASE)/sbis/usbd/include -I$(MPOS_BASE)/sbis/usbd

# App sources
APP_SRCS = $(CWD)/stage2_bl.c $(CWD)/stage2_bl_dev_keys.c

# App objects
APP_OBJS = $(subst $(MPOS_BASE),$(BUILDDIR),$(APP_SRCS:.c=.o))

.PHONY: help

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): subdirs $(LDFILE) $(APP_OBJS)
	$(Q)echo "[Linking ... $(notdir $@)]"
	$(Q)$(LD) -o $@ $(APP_OBJS) `find $(subst $(MPOS_BASE),$(BUILDDIR),$(SUBDIRS)) -type f -name '*.o'` \
        `find $(subst $(MPOS_BASE),$(BUILDDIR),$(SUBDIRS)) -type f -name '*.a'` $(LDFLAGS)
	$(Q)$(OBJDUMP) -d $@ > $@.lst
	$(Q)$(OBJCOPY) -O binary $@ $@.bin
	$(Q)cp -f $@.bin $(CWD)
	$(Q)./sign_stage2_bl.sh

$(LDFILE): $(LDSFILE)
	$(Q)$(CC) $(CFLAGS) -E -P -o $@ -x c-header $<

clean:
	$(Q)echo [Cleaning ... ]
	$(Q)-rm -rf $(BUILDDIR)
	$(Q)-rm -f $(TARGET).bin
	$(Q)-rm -f *.bin

help:
	@echo "make                # To verify AAI in scratch ram"
	@echo "make target=flash   # To verify AAI in flash"

include $(MPOS_BASE)/sbis/driver.cfg
include $(MPOS_BASE)/Makefile.inc
