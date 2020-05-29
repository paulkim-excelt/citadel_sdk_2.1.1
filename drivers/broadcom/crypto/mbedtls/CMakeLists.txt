if (${CONFIG_MBEDTLS_BUILTIN})
zephyr_library_include_directories(
	${ZEPHYR_BASE}/ext/lib/crypto/mbedtls/include
	${ZEPHYR_BASE}/ext/lib/crypto/mbedtls/configs
	${ZEPHYR_BASE}/include/broadcom/drivers/crypto/mbedtls
	)
endif()

zephyr_compile_options_ifdef(CONFIG_MBEDTLS_BUILTIN -DMBEDTLS_CONFIG_FILE=\"${CONFIG_MBEDTLS_CFG_FILE}\")

zephyr_library_sources(
	aes_alt.c
	des_alt.c
	sha256_alt.c
	sha1_alt.c
	gcm_alt.c
	rsa_alt.c
	ecdh_alt.c
	ecdsa_alt.c
	)
