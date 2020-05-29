zephyr_library_sources(
	crypto_rng.c
	crypto_selftest.c
	crypto_smau.c
	crypto_symmetric.c
	gf8_func.c
	)

add_subdirectory_ifdef(CONFIG_CRYPTO_BRCM_MBEDTLS mbedtls)
