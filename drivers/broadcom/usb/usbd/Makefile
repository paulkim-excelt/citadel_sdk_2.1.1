################################################
#
# Copyright Broadcom corporation (2017)
#
# Makefile for USBD base driver
#
# Files common across SBI and AAI
# Internal differences based on CFLAGS
###############################################
	#Comment to build for OS INDEPENDENT MODE
	CFLAGS += -DUSH_BOOTROM

	#Include
	CFLAGS += -I./include/

	#Objects
	# USBD
	obj-y += usbinit.o
	obj-y += usbdevice.o
	obj-y += usb.o
	obj-y += stub.o
	obj-y += usbd_template.o

	#USBCVCLASS DUMMY
	obj-y += usbcvclass/cvclass.o
	obj-y += usbcvclass/cvglobalvar.o
###############################################

-include $(MPOS_BASE)/Makefile.inc
