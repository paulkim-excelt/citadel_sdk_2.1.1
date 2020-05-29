obj-y += test_pka.o
obj-y += test_openssl_pka.o

# SHA2-512 hash implementation
obj-y += sha512.o

ifneq ($(MPOS_BASE),)
CFLAGS += -ffunction-sections -I$(MPOS_BASE)/tests/drivers/pka
CFLAGS += -I$(MPOS_BASE)/apps/ut/tests/drivers/pka
endif

-include $(ZEPHYR_BASE)/tests/Makefile.test
-include $(MPOS_BASE)/Makefile.inc
