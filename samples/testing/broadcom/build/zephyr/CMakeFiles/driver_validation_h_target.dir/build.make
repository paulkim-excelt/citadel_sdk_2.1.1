# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/cmake/bin/cmake

# The command to remove a file.
RM = /opt/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/paulkim/work/sdk/citadel/samples/testing/broadcom

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build

# Utility rule file for driver_validation_h_target.

# Include the progress variables for this target.
include zephyr/CMakeFiles/driver_validation_h_target.dir/progress.make

zephyr/CMakeFiles/driver_validation_h_target: zephyr/include/generated/driver-validation.h


zephyr/include/generated/driver-validation.h:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating include/generated/driver-validation.h"
	cd /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr && /usr/bin/python3 /home/paulkim/work/sdk/citadel/scripts/gen_kobject_list.py --validation-output /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/include/generated/driver-validation.h 

driver_validation_h_target: zephyr/CMakeFiles/driver_validation_h_target
driver_validation_h_target: zephyr/include/generated/driver-validation.h
driver_validation_h_target: zephyr/CMakeFiles/driver_validation_h_target.dir/build.make

.PHONY : driver_validation_h_target

# Rule to build all files generated by this target.
zephyr/CMakeFiles/driver_validation_h_target.dir/build: driver_validation_h_target

.PHONY : zephyr/CMakeFiles/driver_validation_h_target.dir/build

zephyr/CMakeFiles/driver_validation_h_target.dir/clean:
	cd /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr && $(CMAKE_COMMAND) -P CMakeFiles/driver_validation_h_target.dir/cmake_clean.cmake
.PHONY : zephyr/CMakeFiles/driver_validation_h_target.dir/clean

zephyr/CMakeFiles/driver_validation_h_target.dir/depend:
	cd /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/paulkim/work/sdk/citadel/samples/testing/broadcom /home/paulkim/work/sdk/citadel /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr /home/paulkim/work/sdk/citadel/samples/testing/broadcom/build/zephyr/CMakeFiles/driver_validation_h_target.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : zephyr/CMakeFiles/driver_validation_h_target.dir/depend
