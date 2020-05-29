cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(broadcom)

add_subdirectory(src)

get_property(EXTRA_TEST_LIBS_PROP GLOBAL PROPERTY EXTRA_TEST_LIBS)

# Include any extra test libraries here
target_link_libraries(app PRIVATE -Wl,--whole-archive ${EXTRA_TEST_LIBS_PROP} -Wl,--no-whole-archive)

target_sources(app PRIVATE ${app_sources})
