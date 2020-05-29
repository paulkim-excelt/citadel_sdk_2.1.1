#
# Copyright Broadcom Inc. (2018)
#

# Common make configuration flags
include ../make.cfg

# Target name
TARGET = upgrade_sbi

# Memory layout for image
# The 1 MB of SRAM (0x24000000 - 0x240FFFFFF) is split into 2 parts
# 1. Top 309 KB (51K RAM start offset (0xCC00), 2 KB for headers (SBI & ToC), 
#                256K for code/data (Loadable sections) )
#    The SRAM image start address for LAE images is 0x2400CC00 (ram image)
#    - The ToC occupies the region 0x2400CC00 - 0x2400D000
#    - The SBI header occupies the region 0x2400D000 - 0x2400D360
#    - The SBI binary occupies the region 0x2400D360 - 0x2404D400
# 2. Bottom 715 KB for data
#    - The bss/heap/stack occupies the region 0x2404D400 - 0x240FFFFF
CFLAGS += -DRO_MEM_START=DEF_SBI_BIN_START_HMAC
CFLAGS += -DRO_MEM_LENGTH=0x40000 # 256 K
CFLAGS += -DRW_MEM_START=RO_MEM_START+RO_MEM_LENGTH
CFLAGS += -DRW_MEM_LENGTH=SRAM_SIZE+SRAM_START-\(RW_MEM_START\)

# List of driver dependencies
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/qspi_flash
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dma
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dmu

# Include driver dependencies for the build
SUBDIRS += $(MPOS_BASE)/sbis/usbd
SUBDIRS += $(BRCM_DRIVER_DEPS)

# Linker flags
LDFLAGS += -L$(COMMON)/lib -Bstatic
LDSFILE = $(CWD)/$(TARGET).lds
LDFILE = $(BUILDDIR)/$(TARGET).ld

# Required only for SBI Debug build
CFLAGS += -DSBI_DEBUG -DCONFIG_DEBUG

# USBD include path 
CFLAGS += -I$(MPOS_BASE)/sbis/usbd/include -I$(MPOS_BASE)/sbis/usbd

# App sources
APP_SRCS = $(CWD)/upgrade_sbi.c

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
	$(Q)./sign_upgrade_sbi.sh

$(LDFILE): $(LDSFILE)
	$(Q)$(CC) $(CFLAGS) -E -P -o $@ -x c-header $<

clean:
	$(Q)echo [Cleaning ... ]
	$(Q)-rm -rf $(BUILDDIR)
	$(Q)-rm -f $(TARGET).bin
	$(Q)-rm -f *.bin

include $(MPOS_BASE)/sbis/driver.cfg
include $(MPOS_BASE)/Makefile.inc
