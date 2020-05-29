obj-y += sotp.o

# FIXME, this is temporary hack should be checked with tests
ifneq ($(MPOS_BASE),)
CFLAGS += -Wno-sign-compare
else
ccflags-y += -Wno-sign-compare
endif

-include $(MPOS_BASE)/Makefile.inc
