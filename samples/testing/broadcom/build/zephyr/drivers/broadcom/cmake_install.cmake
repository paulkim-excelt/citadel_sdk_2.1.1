# Install script for directory: /home/paulkim/work/sdk/citadel/drivers/broadcom

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/clock_control/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/pinmux/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/timer/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/adc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/bbl/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/spru/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/crypto/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/pka/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/dma/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/dmu/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/flextimer/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/gpio/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/i2c/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/msr/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/lcd/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/pm/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/pwm/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/qspi_flash/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/rtc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/sc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/sdio/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/smau/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/sotp/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/spi/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/unicam/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/wdt/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/usbd/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/drivers/broadcom/usbh/cmake_install.cmake")

endif()

