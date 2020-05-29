
This distribution contains the libraries for the Linux CV load utility

*****************************************************************
The procedure to build the binaries and library files
*****************************************************************
This utility is tested on Ubuntu 18.04

Note: Please install usb dev package "sudo apt-get install libusb-1.0-0-dev"

STEP-1: Go to the sbis/tools/linux_utils/load_sbi directory, which will have
        a Makefile

STEP-2: Build using the make command as below
	make clean all

STEP-3: The binary file CVFWUpgrade will be generated in "bin" directory and
	the library files will be generated in "lib" directory.

STEP-4: Copy the .so file and .a file in load_sbi/lib into /usr/lib directory
	cd load_sbi/lib
	sudo cp * /usr/lib/.

STEP-5: Copy the "CVFWUpgrade", "upgrade sbi image" and "SBI Image" files in to
        a folder.

STEP-5: run  ./CVFWUpgrade with "upgrade sbi image" file and "SBI Image"
        Ex: sudo ./CVFWUpgrade upgrade_sbi.sign.toc.bin Image.bin


*********************************************************************
The procedure to build the "upgrade sbi image" image
*************************************************************
STEP-1: Go to sbis/upgrade_sbi

STEP-2: Compile the upgrade_sbi source as mentioned below
        export UNASSIGNED_BUILD=1
        export CROSS_COMPILE=<tool chain path>/gcc-arm-none-eabi-6-2017-q1-update/bin/arm-none-eabi-
        make clean
        make
