# Common make configuration flags
include ../make.cfg

# Target name
TARGET = sample_aai

# Defaults
target ?= ram

# Memory layout for RAM and FLash images
# For RAM images whose binary size < 256 KB
#	The 1 MB of SRAM (0x24000000 - 0x240FFFFFF) is split into 3 parts
#	1. Top 128 KB is reserved for the Stage 2 Boot loader
#	2. The next 256 KB for the loadable sections (code/rodata)
#	3. Bottom 640 KB for
#	   - The bss/heap/stack occupies the region 0x2404D400 - 0x240FFFFF
#
# For Flash image whose binary size > 256 KB
# 	1. The entire flash from offset 0x20000 (top 128 K reserved for S2BL) is 
#	   available for text and rodata (0x63000000 - 0x637FFFFF)
#	2. The complete 1MB of SRAM is available for data/heap/stack/bss for flash
#	   images

# Top 128 KB is reserved for S2BL (Stage 2 boot loader)
CFLAGS += -DS2BL_IMAGE_SIZE=DEF_AAI_IMAGE_OFFSET
CFLAGS += -DDEFAULT_FLASH_SIZE=0x800000 # 8 MB

ifeq ($(target), ram)
CFLAGS += -DRO_MEM_START=SRAM_START+$(DEF_S2BL_SIZE)+SBI_HEADER_SIZE_RSA2K
CFLAGS += -DRO_MEM_LENGTH=0x40000 # 256 K
CFLAGS += -DRW_MEM_START=RO_MEM_START+RO_MEM_LENGTH
CFLAGS += -DRW_MEM_LENGTH=SRAM_SIZE+SRAM_START-\(RW_MEM_START\)
else
CFLAGS += -DRO_MEM_START=FLASH_START+S2BL_IMAGE_SIZE+SBI_HEADER_SIZE_RSA2K
CFLAGS += -DRO_MEM_LENGTH=DEFAULT_FLASH_SIZE+FLASH_START-\(RO_MEM_START\)
CFLAGS += -DRW_MEM_START=SRAM_START
CFLAGS += -DRW_MEM_LENGTH=SRAM_SIZE
endif

# Linker flags
LDFLAGS += -Bstatic
LDSFILE = $(CWD)/$(TARGET).lds
LDFILE = $(BUILDDIR)/$(TARGET).ld

# Required only for SBI Debug build
CFLAGS += -DSBI_DEBUG -DCONFIG_DEBUG

# App sources
APP_SRCS = $(CWD)/sample_aai.c

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
	$(Q)./sign_$(TARGET).sh

$(LDFILE): $(LDSFILE)
	$(Q)$(CC) $(CFLAGS) -E -P -o $@ -x c-header $<

clean: 
	$(Q)echo [Cleaning ... ]
	$(Q)-rm -rf $(BUILDDIR)
	$(Q)-rm -f $(TARGET).bin *.bin *.xip *.noxip

help:
	$(Q)echo "make                # for small image (<=256K) that runs in"
	$(Q)echo "                    # scratch ram, same as make target=ram"
	$(Q)echo "make target=flash   # for big image (>256K) that runs in flash"

include $(MPOS_BASE)/sbis/driver.cfg
include $(MPOS_BASE)/Makefile.inc
