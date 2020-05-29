#-include $(MPOS_BASE)/ext/lib/crypto/mbedtls/Makefile.include
subdir-ccflags-$(CONFIG_MBEDTLS_BUILTIN) += -I$(srctree)/ext/lib/crypto/mbedtls/include

subdir-ccflags-$(CONFIG_MBEDTLS_BUILTIN) += -I$(srctree)/ext/lib/crypto/mbedtls/configs

subdir-ccflags-$(CONFIG_CRYPTO_BRCM_MBEDTLS) += -I$(srctree)/include/broadcom/drivers/crypto/mbedtls

subdir-ccflags-$(CONFIG_MBEDTLS_BUILTIN) += -DMBEDTLS_CONFIG_FILE='$(CONFIG_MBEDTLS_CFG_FILE)'
obj-y += aes_alt.o
obj-y += des_alt.o
obj-y += sha256_alt.o
obj-y += sha1_alt.o
obj-y += gcm_alt.o
obj-y += rsa_alt.o
obj-y += ecdh_alt.o
obj-y += ecdsa_alt.o

-include $(MPOS_BASE)/Makefile.inc
