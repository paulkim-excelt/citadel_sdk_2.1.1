#
# Copyright Broadcom Inc. (2018)
#

# Common make configuration flags
include ../make.cfg

# Target name
TARGET = convert_img

CFLAGS += -DDEFAULT_FLASH_SIZE=0x800000 # 8MB default flash size

# The secure_xip image sits between the start of the flash and AAI image offset
CFLAGS += -DRO_MEM_START=FLASH_START+DEF_TOC_SIZE+SBI_HEADER_SIZE_RSA2K
CFLAGS += -DRO_MEM_LENGTH=DEFAULT_FLASH_SIZE+FLASH_START-\(RO_MEM_START\)
CFLAGS += -DRW_MEM_START=SRAM_START
CFLAGS += -DRW_MEM_LENGTH=SRAM_SIZE

# List of driver dependencies
BRCM_DRIVER_DEPS = $(MPOS_BASE)/drivers/broadcom/sotp
ifeq ($(convert), 1)
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/smau
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/crypto
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/pka
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dma
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dmu
endif

# Include driver dependencies for the build
SUBDIRS += $(BRCM_DRIVER_DEPS)

# Linker flags
LDFLAGS += -L$(COMMON)/lib
LDSFILE = $(CWD)/$(TARGET).lds
LDFILE = $(BUILDDIR)/$(TARGET).ld

# Compiler flags
ifeq ($(convert), 1)
CFLAGS+=-DMPOS_CONVERT
# To include Customization apis
export CONFIG_COMMON_CUSTOM_API=y
# Define the HMAC SHA key file for programming HMAC SHA key
CFLAGS += -DKHMAC_SHA_BIN_FILE='"$(TOOLS_PATH)/secimage/khmacsha256"'
endif

# Required only for SBI Debug build
CFLAGS += -DSBI_DEBUG -DCONFIG_DEBUG

# App sources
KEY_FILE = $(CWD)/customization_key.c
APP_SRCS = $(CWD)/convert_img.c $(KEY_FILE)

# App objects
APP_OBJS = $(subst $(MPOS_BASE),$(BUILDDIR),$(APP_SRCS:.c=.o))

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): subdirs $(LDFILE) $(APP_OBJS)
	$(Q)echo "[Linking ... $(notdir $@)]"
	$(Q)$(LD) -o $@ $(APP_OBJS) `find $(subst $(MPOS_BASE),$(BUILDDIR),$(SUBDIRS)) -type f -name '*.o'` \
        `find $(subst $(MPOS_BASE),$(BUILDDIR),$(SUBDIRS)) -type f -name '*.a'` $(LDFLAGS)
	$(Q)$(OBJDUMP) -d $@ > $@.lst
	$(Q)$(OBJCOPY) -O binary $@ $@.bin
	$(Q)cp -f $@.bin $(CWD)/sbi.bin
	$(Q)$(TOOLS_PATH)/scripts/tocgen_sim.py -o $(TARGET).toc.bin

$(LDFILE): $(LDSFILE)
	$(Q)$(CC) $(CFLAGS) -E -P -o $@ -x c-header $<

$(KEY_FILE):
	$(Q)echo [Generating ... $(notdir $@)]
	$(Q)python $(TOOLS_PATH)/scripts/generatekey.py -f \
               $(TOOLS_PATH)/secimage/LYNX-BRCM-CID-0x60000003-ROT.priv.pem \
               -ci 0x60000003 -csc 0x44 -cri 0 -sri 0 > $@

clean:
	$(Q)echo [Cleaning ... ]
	$(Q)-rm -rf $(BUILDDIR)
	$(Q)-rm -f $(KEY_FILE)
	$(Q)-rm -f $(notdir $(TARGET).bin) *.bin

include $(MPOS_BASE)/sbis/driver.cfg
include $(MPOS_BASE)/Makefile.inc
