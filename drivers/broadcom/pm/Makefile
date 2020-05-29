obj-y += smbus.o
obj-y += save_restore.o
obj-y += mproc_pm.o

ifneq ($(MPOS_BASE),)
CFLAGS += -I$(MPOS_BASE)/drivers/broadcom/pm/crmu -I$(MPOS_BASE)/build/M0
else
CRMU_SRC_PATH=$(ZEPHYR_BASE)/drivers/broadcom/pm/crmu
ccflags-y += -I$(CRMU_SRC_PATH) -I$(CRMU_SRC_PATH)/build/M0

# Target/recipe to Build M0 code
drivers/broadcom/pm/mproc_pm.o: $(CRMU_SRC_PATH)/build/M0/M0_image.bin
$(CRMU_SRC_PATH)/build/M0/M0_image.bin:
	$(Q)echo "  MAKE    M0"
	$(Q)-mkdir -p $(CRMU_SRC_PATH)/build/M0
	$(Q)echo `make -C $(CRMU_SRC_PATH) M0 2>&1` > $(CRMU_SRC_PATH)/build/M0/output.log
	$(Q)cp -f $(CRMU_SRC_PATH)/build/M0/M0_image.bin $(O)
	$(Q)-rm -rf $(CRMU_SRC_PATH)/build
	$(Q)-rm -f $(O)/drivers/broadcom/pm/remove_m0_bins

# Delete M0_image.bin to make sure it's rebuilt each time
drivers/broadcom/pm/built-in.o: $(O)/drivers/broadcom/pm/remove_m0_bins
$(O)/drivers/broadcom/pm/remove_m0_bins:
	$(Q)-rm -f $(O)/M0_image.bin
	$(Q)touch $@
endif

-include $(MPOS_BASE)/Makefile.inc
