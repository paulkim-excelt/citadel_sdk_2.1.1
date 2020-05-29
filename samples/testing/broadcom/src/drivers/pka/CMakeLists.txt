zephyr_library_sources(test_pka.c)
zephyr_library_sources(test_openssl_pka.c)

zephyr_compile_definitions(-DPATH_TO_KEYS="${ZEPHYR_BASE}/samples/testing/broadcom/src/drivers/pka/")

# SHA2-512 hash implementation
zephyr_library_sources(sha512.c)
