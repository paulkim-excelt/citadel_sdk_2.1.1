################################################
#
# Copyright Broadcom corporation (2017)
#
# Makefile for cv module
# Files different for SBI and AAI
# Additional Internal differences based on CFLAGS
################################################

ifeq ($(USB_TARGET), SBI)
###############################################
obj-y += cvmanager.o
obj-y += cvutils.o
obj-y += cvversion.o
###############################################
endif

ifeq ($(USB_TARGET), AAI)
###############################################
obj-y += cvversion.o
obj-y += cvauth.o
obj-y += cvcryptohandler.o
obj-y += cvhandler.o
obj-y += cvinit.o
obj-y += cvmanager.o
obj-y += cvobjstore.o
obj-y += cvutilhandler.o
obj-y += cvutils.o
obj-y += cvobjhandler.o
obj-y += extintf.o
obj-y += cvfphandler.o
obj-y += cvdevhandler.o
obj-y += cvpbahandler.o
obj-y += cvfpstore.o
obj-y += easyzlib.o
###############################################
endif

-include $(MPOS_BASE)/Makefile.inc
