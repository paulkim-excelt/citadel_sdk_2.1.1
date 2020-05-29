SUBDIRS += $(CONFIG_APP)

# Enable unit test app, if not app is enabled in the build
ifeq ($(CONFIG_APP), )
SUBDIRS += ut
endif

include $(MPOS_BASE)/Makefile.inc
