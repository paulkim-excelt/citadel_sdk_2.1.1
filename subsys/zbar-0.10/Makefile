#Should we use toolchain provided header files rather than the minimal libc?
#ZEPHYRINCLUDE := $(filter-out -I$(srctree)/lib/libc/minimal/include,$(ZEPHYRINCLUDE))
#ccflags-y += -I${GCCARMEMB_TOOLCHAIN_PATH}/arm-none-eabi/include

ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/iconv_simple
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/include
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/zbar
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/zbar/decoder
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/zbar/processor
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/zbar/qrcode
ccflags-y += -I$(ZEPHYR_BASE)/subsys/zbar-0.10/portable
ccflags-y += -include $(ZEPHYR_BASE)/subsys/zbar-0.10/portable/port.h

obj-$(CONFIG_ZBAR) += zbar_img.o
obj-$(CONFIG_ZBAR) += iconv_simple/iconv.o
obj-$(CONFIG_ZBAR) += iconv_simple/iconv_close.o
obj-$(CONFIG_ZBAR) += iconv_simple/iconv_open.o
obj-$(CONFIG_ZBAR) += zbar/error.o
obj-$(CONFIG_ZBAR) += zbar/image.o
obj-$(CONFIG_ZBAR) += zbar/decoder.o
obj-$(CONFIG_ZBAR) += zbar/img_scanner.o
obj-$(CONFIG_ZBAR) += zbar/refcnt.o
obj-$(CONFIG_ZBAR) += zbar/config.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/qrdectxt.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/rs.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/qrdec.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/isaac.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/util.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/bch15_5.o
obj-$(CONFIG_ZBAR) += zbar/qrcode/binarize.o
obj-$(CONFIG_ZBAR) += zbar/convert.o
obj-$(CONFIG_ZBAR) += zbar/decoder/qr_finder.o
obj-$(CONFIG_ZBAR) += zbar/processor/posix.o
obj-$(CONFIG_ZBAR) += zbar/processor/null.o
obj-$(CONFIG_ZBAR) += zbar/processor/lock.o
obj-$(CONFIG_ZBAR) += zbar/symbol.o
obj-$(CONFIG_ZBAR) += zbar/scanner.o
obj-$(CONFIG_ZBAR) += zbar/processor.o
obj-$(CONFIG_ZBAR) += portable/port.o
obj-$(CONFIG_ZBAR) += portable/qsort.o

#global -Werror is too strict for zbar, disable it or disable some implied options
#ccflags-y := $(filter-out -Werror,$(CFLAGS))
ccflags-y += -Wno-unused-parameter 
ccflags-y += -Wno-unused-value
ccflags-y += -Wno-unused-variable 
ccflags-y += -Wno-missing-field-initializers
ccflags-y += -Wno-parentheses
ccflags-y += -Wno-sign-compare
ccflags-y += -Wno-type-limits
ccflags-y += -Wno-logical-not-parentheses
