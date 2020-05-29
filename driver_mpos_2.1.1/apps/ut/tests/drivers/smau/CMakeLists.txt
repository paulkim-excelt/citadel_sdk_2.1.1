set (TEST_SMAU_SOURCES test_smau.c)

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/include/broadcom/drivers)
zephyr_library_include_directories(${ZEPHYR_BASE}/boards/arm/${BOARD})
zephyr_library_include_directories(${ZEPHYR_BASE}/samples/testing/broadcom/src)

zephyr_library_sources(${TEST_SMAU_SOURCES})

# Add .ramfunc prefix to all sections in the smau test object files
# as nothing residing on the flash can be accessed from within the driver
#
set (OBJ_FILES ${TEST_SMAU_SOURCES})
list(TRANSFORM OBJ_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${ZEPHYR_CURRENT_LIBRARY}.dir/)
list(TRANSFORM OBJ_FILES APPEND .obj)

add_custom_command(TARGET ${ZEPHYR_CURRENT_LIBRARY}
    PRE_BUILD
    COMMAND for f in ${OBJ_FILES} \; do  if [ \"`${CMAKE_OBJDUMP} -h $$f | grep ramfunc\.text | wc -l`\" = \"0\" ] \; then ${CMAKE_OBJCOPY} --prefix-sections=.ramfunc $$f \; fi \; done
)

# Creating a zephyr_library here (so that pre build steps could be added)
# does not include the generated library in the linking list for the zephyr
# target. To overcome this we explicitly include it in the app's CMakeLists.txt
# using the EXTRA_TEST_LIBS property which is set here
add_dependencies(${ZEPHYR_CURRENT_LIBRARY} offsets)
set_property(GLOBAL APPEND PROPERTY EXTRA_TEST_LIBS ${ZEPHYR_CURRENT_LIBRARY})
