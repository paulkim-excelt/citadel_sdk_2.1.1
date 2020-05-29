# Install script for directory: /home/paulkim/work/sdk/citadel/samples/testing/broadcom/src/drivers

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
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/clock_control/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/adc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/bbl/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/crypto/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/pka/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/dma/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/dmu/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/flextimer/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/gpio/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/i2c/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/pm/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/pwm/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/qspi_flash/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/rtc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/sc/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/sdio/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/smau/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/sotp/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/spi/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/timer/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/unicam/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/lcd/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/wdt/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/usbd/cmake_install.cmake")
  include("/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/src/drivers/usbh/cmake_install.cmake")

endif()

