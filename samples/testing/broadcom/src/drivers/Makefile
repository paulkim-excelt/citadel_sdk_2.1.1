obj-y += clock_control/
obj-$(CONFIG_BRCM_DRIVER_ADC) += adc/
obj-$(CONFIG_BRCM_DRIVER_BBL) += bbl/
obj-$(CONFIG_BRCM_DRIVER_CRYPTO) += crypto/ pka/
obj-$(CONFIG_BRCM_DRIVER_DMA) += dma/
obj-$(CONFIG_BRCM_DRIVER_DMU) += dmu/
obj-$(CONFIG_BRCM_DRIVER_FT) += flextimer/
obj-$(CONFIG_BRCM_DRIVER_GPIO) += gpio/
obj-$(CONFIG_BRCM_DRIVER_KEYPAD) += keypad/
obj-$(CONFIG_BRCM_DRIVER_I2C) += i2c/
obj-$(CONFIG_BRCM_DRIVER_PM) += pm/
obj-$(CONFIG_BRCM_DRIVER_PRINTER) += printer/
obj-$(CONFIG_BRCM_DRIVER_PWM) += pwm/
obj-$(CONFIG_BRCM_DRIVER_QSPI) += qspi_flash/
obj-$(CONFIG_BRCM_DRIVER_RTC) += rtc/
obj-$(CONFIG_BRCM_DRIVER_SCC) += sc/
obj-$(CONFIG_BRCM_DRIVER_SDIO) += sdio/
obj-$(CONFIG_BRCM_DRIVER_SMAU) += smau/
obj-$(CONFIG_BRCM_DRIVER_SMC) += smc/
obj-$(CONFIG_BRCM_DRIVER_SOTP) += sotp/
obj-$(CONFIG_BRCM_DRIVER_SPI) += spi/
obj-$(CONFIG_BRCM_DRIVER_SPL) += spl/
obj-$(CONFIG_BRCM_DRIVER_TIMERS) += timer/
obj-$(CONFIG_BRCM_DRIVER_UNICAM) += unicam/
obj-$(CONFIG_BRCM_DRIVER_USB) += usbh/
obj-$(CONFIG_BRCM_DRIVER_LCD) += lcd/
obj-$(CONFIG_BRCM_DRIVER_WDT) += wdt/

ifeq ($(MPOS_BASE),)
# Test for Zephyr USB device controller driver
obj-$(CONFIG_BRCM_DRIVER_USBD) += usbd/
obj-$(CONFIG_BRCM_DRIVER_USBH) += usbh/
endif

-include $(MPOS_BASE)/Makefile.inc

# For Zephyr only
src/drivers/built-in.o: src/drivers/smau/rename
src/drivers/smau/rename: src/drivers/smau/built-in.o
	@$(OBJCOPY) --prefix-sections=fc $^
	@touch $@
