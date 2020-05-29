zephyr_library_sources(sotp.c)

# FIXME, this is temporary hack should be checked with tests
zephyr_compile_options(-Wno-sign-compare)
