zephyr_library()

zephyr_library_include_directories(
	${ZEPHYR_BASE}/subsys/zbar-0.10/iconv_simple
	${ZEPHYR_BASE}/subsys/zbar-0.10/include
	${ZEPHYR_BASE}/subsys/zbar-0.10/zbar
	${ZEPHYR_BASE}/subsys/zbar-0.10/zbar/decoder
	${ZEPHYR_BASE}/subsys/zbar-0.10/zbar/processor
	${ZEPHYR_BASE}/subsys/zbar-0.10/zbar/qrcode
	${ZEPHYR_BASE}/subsys/zbar-0.10/portable
	)
zephyr_compile_options(-include ${ZEPHYR_BASE}/subsys/zbar-0.10/portable/port.h)

zephyr_library_sources_ifdef(CONFIG_ZBAR zbar_img.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR iconv_simple/iconv.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR iconv_simple/iconv_close.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR iconv_simple/iconv_open.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/error.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/image.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/decoder.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/img_scanner.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/refcnt.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/config.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/qrdectxt.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/rs.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/qrdec.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/isaac.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/util.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/bch15_5.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/qrcode/binarize.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/convert.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/decoder/qr_finder.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/processor/posix.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/processor/null.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/processor/lock.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/symbol.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/scanner.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR zbar/processor.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR portable/port.c)
zephyr_library_sources_ifdef(CONFIG_ZBAR portable/qsort.c)

#global -Werror is too strict for zbar, disable it or disable some implied options
zephyr_compile_options(
	-Wno-unused-parameter
	-Wno-unused-value
	-Wno-unused-variable
	-Wno-missing-field-initializers
	-Wno-parentheses
	-Wno-sign-compare
	-Wno-type-limits
	-Wno-logical-not-parentheses
	-Wno-pointer-arith
	)
