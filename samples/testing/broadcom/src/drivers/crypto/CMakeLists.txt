zephyr_library_sources(test_crypto.c)
zephyr_library_sources(pci_eval_test.c)

zephyr_library_sources_ifdef(CONFIG_MBEDTLS test_mbedtls.c)

