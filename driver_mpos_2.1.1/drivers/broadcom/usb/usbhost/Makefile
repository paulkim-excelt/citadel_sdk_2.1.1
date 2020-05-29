# Includes
CFLAGS += -I../usb/usbd/
CFLAGS += -I../usb/usbd/include/
CFLAGS += -I../usb/cv/
CFLAGS += -I../usb/usbcvclass/
CFLAGS += -I../usb/ccid/
CFLAGS += -I../usb/usbnfpclass/

#remove later
CFLAGS += -I../stubs/
CFLAGS += -I$(MPOS_BASE)/OpenRTOS/include/

obj-y += cfeglue.o
obj-y += ehci.o
obj-y += lib_queue.o
obj-y += ohci.o
obj-y += usbd.o
obj-y += usbdebug.o
obj-y += usbdevs.o
obj-y += usbhub.o
obj-y += usbmain.o

-include $(MPOS_BASE)/Makefile.inc
