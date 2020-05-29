set (SMAU_SOURCES
	smau.c
	)

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/include/broadcom/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/boards/arm/${BOARD})
zephyr_library_sources(${SMAU_SOURCES})

# Add .ramfunc prefix to all sections in the smau driver object files
# as nothing residing on the flash can be accessed from within the driver
#
set (OBJ_FILES ${SMAU_SOURCES})
list(TRANSFORM OBJ_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${ZEPHYR_CURRENT_LIBRARY}.dir/)
list(TRANSFORM OBJ_FILES APPEND .obj)

add_custom_command(TARGET ${ZEPHYR_CURRENT_LIBRARY}
    PRE_BUILD
    COMMAND for f in ${OBJ_FILES} \; do  if [ \"`${CMAKE_OBJDUMP} -h $$f | grep ramfunc\.text | wc -l`\" = \"0\" ] \; then ${CMAKE_OBJCOPY} --prefix-sections=.ramfunc $$f \; fi \; done
	COMMAND for f in ${OBJ_FILES} \; do ${CMAKE_OBJCOPY} --rename .ramfunc.devconfig.init=.devconfig.init --rename .ramfunc.init_PRE_KERNEL_24=.init_PRE_KERNEL_24 $$f \; done
    DEPENDS ${OBJ_FILES}
)
