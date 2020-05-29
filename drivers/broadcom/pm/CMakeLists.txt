set(CRMU_SRC_PATH ${ZEPHYR_BASE}/drivers/broadcom/pm/crmu)

zephyr_library_include_directories(
	${CRMU_SRC_PATH}
	${CRMU_SRC_PATH}/build/M0
	)

zephyr_library_sources(smbus.c)
zephyr_library_sources(save_restore.c)
zephyr_library_sources(mproc_pm.c)

zephyr_compile_definitions(-DPATH_TO_M0_IMAGE_BIN="${CRMU_SRC_PATH}/build/M0/")
zephyr_compile_definitions(-D__RSTR_STACK_START__=_image_ram_start)

# Custom commands to build M0 code unconditionally and re-compile mproc_pm.c each time
add_custom_target(m0_build ALL
	COMMAND rm -rf ${CRMU_SRC_PATH}/build &&
			ZEPHYRINCLUDE=\"-I${ZEPHYR_BASE}/include
							-I${CMAKE_BINARY_DIR}/zephyr/include/generated
							-I${ZEPHYR_BASE}/include/broadcom/drivers
							-I${ZEPHYR_BASE}/arch/arm/include
							-I${ZEPHYR_BASE}/subsys/testsuite/ztest/include
							-DCONFIG_ARMV6_M_ARMV8_M_BASELINE
							-DDEFINE_LOCAL_STRUCT_DEVICE
							\"
			make -C ${CRMU_SRC_PATH} M0 &&
			touch ${CRMU_SRC_PATH}/../mproc_pm.c
	)

add_dependencies(zephyr m0_build)

# M0 build clean up
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
	COMMAND rm -rf ${CRMU_SRC_PATH}/build/ && touch ${CRMU_SRC_PATH}/../mproc_pm.c
    )

