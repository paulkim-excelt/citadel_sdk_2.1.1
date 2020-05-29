SUBDIRS += common

# Select the OS to build if CONFIG_RTOS is defined
# Build the no os version if not
ifneq ($(CONFIG_RTOS),)
SUBDIRS += $(CONFIG_RTOS)
else
SUBDIRS += no_os
endif

include $(MPOS_BASE)/Makefile.inc
