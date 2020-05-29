#
# Copyright Broadcom Inc. (2018)
#

# Common make configuration flags
include ../make.cfg

# Target name
TARGET = secure_xip

# The flash address for the XIP image to be validated and executed
# In this demo we use the Sample AAI as the XIP image which is built
# and prepended the SCD header of size 0x100 bytes. The XIP image start
# address includes the SCD header and resides in flash @ 0x63030000. The
# sample aai image is therefore linked at 0x63030100 to accommodate for
# the SCD header (see sample_aai/Makefile.sxip)
XIP_IMAGE_START_ADDR = \(FLASH_START+DEF_AAI_IMAGE_OFFSET\)

# Defaults
target ?= dev
algo ?= aes_hmac

# For unassigned builds the secure xip sbi will be built as an XIP image
# And for AB builds (default), it will be built as an LAE image (as this is a
# requirement for AB parts)
ifeq ($(UNASSIGNED_BUILD),1)
# The secure_xip image sits between the start of the flash and AAI image offset
CFLAGS += -DRO_MEM_START=FLASH_START+DEF_TOC_SIZE+SBI_HEADER_SIZE_RSA2K
CFLAGS += -DRO_MEM_LENGTH=DEF_AAI_IMAGE_OFFSET+FLASH_START-\(RO_MEM_START\)
CFLAGS += -DRW_MEM_START=SRAM_START
CFLAGS += -DRW_MEM_LENGTH=SRAM_SIZE
else
# Top 256 KB for code/rodata/fastcode/data (loadable sections)
SECURE_XIP_SBI_SIZE=0x40000
CFLAGS += -DRO_MEM_START=DEF_SBI_BIN_START_RSA2K
CFLAGS += -DRO_MEM_LENGTH=SRAM_START+$(SECURE_XIP_SBI_SIZE)-\(RO_MEM_START\)
CFLAGS += -DRW_MEM_START=RO_MEM_START+RO_MEM_LENGTH
CFLAGS += -DRW_MEM_LENGTH=SRAM_START+SRAM_SIZE-\(RW_MEM_START\)
endif

CFLAGS += -DIPROC_SBI_FLASH_ADDRESS=$(XIP_IMAGE_START_ADDR)

# To include Crypto Util apis
export CONFIG_COMMON_CRYPTO_UTIL=y
# To include Customization apis
export CONFIG_COMMON_CUSTOM_API=y

# List of driver dependencies
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/sotp
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/smau
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/crypto
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/qspi_flash
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dma
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/dmu
BRCM_DRIVER_DEPS += $(MPOS_BASE)/drivers/broadcom/pka

# Include driver dependencies for the build
SUBDIRS += $(MPOS_BASE)/sbis/usbd
SUBDIRS += $(BRCM_DRIVER_DEPS)

# Linker flags
LDFLAGS += -L$(COMMON)/lib -Bstatic
LDSFILE = $(CWD)/$(TARGET).lds
LDFILE = $(BUILDDIR)/$(TARGET).ld

# Required only for SBI Debug build
CFLAGS += -DSBI_DEBUG -DCONFIG_DEBUG

# Additional compiler flags
ifeq ($(algo), aes_hmac)
CFLAGS += -DALGO_AES_HMAC
endif

ifeq ($(algo), hmac)
CFLAGS += -DALGO_HMAC
endif

# USBD include path 
CFLAGS += -I$(MPOS_BASE)/sbis/usbd/include -I$(MPOS_BASE)/sbis/usbd

# App sources
APP_SRCS = $(CWD)/secure_xip.c $(CWD)/secure_xip_sbi.c

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
	$(Q)./sign_secure_xip.sh $(target)

$(LDFILE): $(LDSFILE)
	$(Q)$(CC) $(CFLAGS) -E -P -o $@ -x c-header $<

clean:
	$(Q)echo [Cleaning ... ]
	$(Q)-rm -rf $(BUILDDIR)
	$(Q)-rm -f $(TARGET).sign.bin $(TARGET).bin *.bin

help:
	$(Q)echo "make             # make and sign for dev"
	$(Q)echo "make target=qa   # make and sign for qa"  

include $(MPOS_BASE)/sbis/driver.cfg
include $(MPOS_BASE)/Makefile.inc
