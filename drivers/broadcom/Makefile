ZEPHYRINCLUDE += -I$(ZEPHYR_BASE)/include/broadcom/drivers/
# To allow drivers to access other private driver apis
ZEPHYRINCLUDE += -I$(ZEPHYR_BASE)/drivers/broadcom/
ifneq ($(MPOS_BASE),)
CFLAGS +=  -I$(MPOS_BASE)/drivers/broadcom/
endif

obj-y += clock_control/
obj-y += pinmux/
obj-y += timer/

obj-$(CONFIG_BRCM_DRIVER_ADC) += adc/
obj-$(CONFIG_BRCM_DRIVER_BBL) += bbl/ spru/
obj-$(CONFIG_BRCM_DRIVER_CRYPTO) += crypto/ pka/
obj-$(CONFIG_BRCM_DRIVER_DMA) += dma/
obj-$(CONFIG_BRCM_DRIVER_DMU) += dmu/
obj-$(CONFIG_BRCM_DRIVER_FT) += flextimer/
obj-$(CONFIG_BRCM_DRIVER_GPIO) += gpio/
obj-$(CONFIG_BRCM_DRIVER_I2C) += i2c/
obj-$(CONFIG_BRCM_DRIVER_KEYPAD) += keypad/
obj-$(CONFIG_BRCM_DRIVER_ADC) += msr/
obj-$(CONFIG_BRCM_DRIVER_LCD) += lcd/
obj-$(CONFIG_BRCM_DRIVER_PM) += pm/
obj-$(CONFIG_BRCM_DRIVER_PRINTER) += printer/
obj-$(CONFIG_BRCM_DRIVER_PWM) += pwm/
obj-$(CONFIG_BRCM_DRIVER_QSPI) += qspi_flash/
obj-$(CONFIG_BRCM_DRIVER_RTC) += rtc/ spru/
obj-$(CONFIG_BRCM_DRIVER_SCC) += sc/
obj-$(CONFIG_BRCM_DRIVER_SDIO) += sdio/
obj-$(CONFIG_BRCM_DRIVER_SMAU) += smau/
obj-$(CONFIG_BRCM_DRIVER_SMC) += smc/
obj-$(CONFIG_BRCM_DRIVER_SOTP) += sotp/
obj-$(CONFIG_BRCM_DRIVER_SPI) += spi/
obj-$(CONFIG_BRCM_DRIVER_SPL) += spl/
obj-$(CONFIG_BRCM_DRIVER_UNICAM) += unicam/
obj-$(CONFIG_BRCM_DRIVER_USB) += usb/
obj-$(CONFIG_BRCM_DRIVER_WDT) += wdt/

ifeq ($(MPOS_BASE),)
# Zephyr USB device controller driver resides
# at drivers/broadcom/usbd/
obj-$(CONFIG_BRCM_DRIVER_USBD) += usbd/
obj-$(CONFIG_BRCM_DRIVER_USBH) += usbh/
else
obj-$(CONFIG_BRCM_DRIVER_USBD) += usb/usbd/
endif

-include $(MPOS_BASE)/Makefile.inc

# For Zephyr only
drivers/broadcom/built-in.o: drivers/broadcom/qspi_flash/rename drivers/broadcom/smau/rename
drivers/broadcom/qspi_flash/rename: drivers/broadcom/qspi_flash/built-in.o
	@$(OBJCOPY) --prefix-sections=fc $^
	@$(OBJCOPY) --rename fc.devconfig.init=.devconfig.init \
                    --rename fc.init_PRE_KERNEL_25=.init_PRE_KERNEL_25 $^
	@touch $@
drivers/broadcom/smau/rename: drivers/broadcom/smau/built-in.o
	@$(OBJCOPY) --prefix-sections=fc $^
	@$(OBJCOPY) --rename fc.devconfig.init=.devconfig.init \
                    --rename fc.init_PRE_KERNEL_24=.init_PRE_KERNEL_24 $^
	@touch $@
