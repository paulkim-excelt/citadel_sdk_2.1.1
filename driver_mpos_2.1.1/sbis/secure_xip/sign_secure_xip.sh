#!/bin/bash

SEC_DIR=../tools/secimage
TOOLS_PATH=../tools/

TARGET=secure_xip

if [ "$UNASSIGNED_BUILD" == "1" ]
then
	# For Unassigned builds, just prepend the binary with
	# zeroes equivalent to the size of the SBI header (0x360 for RSA2K)
	# Note that these zeros will act as NOP and ensure that the
	# target image is loaded at the correct linked address
	dd if=/dev/zero of=sbi_zero.bin bs=864 count=1
	cat sbi_zero.bin $TARGET.bin > sbi.bin
	rm sbi_zero.bin

	EXEC_MODE=0xFFFE # XIP
else
	# Copy the file to be signed to secdir
	cp -f $TARGET.bin ${SEC_DIR}/.

	# Sign the target binary only for AB builds
	cd ${SEC_DIR}
	./sign_image_rsa_a0_ab.sh $TARGET.bin
	cd -

	# tocgen script assumes the input file name is sbi.bin
	mv ${SEC_DIR}/${TARGET}.sign.bin sbi.bin
	rm ${SEC_DIR}/$TARGET.bin

	EXEC_MODE=0xFFFB # Load, Authenticate and Execute
fi

# Add Toc Header
${TOOLS_PATH}/scripts/tocgen_v4.py -o ${TARGET}.sign.toc.bin -e $EXEC_MODE

# Remove Temporary files
rm sbi.bin

echo DONE
