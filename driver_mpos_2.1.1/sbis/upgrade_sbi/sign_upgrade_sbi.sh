#!/bin/bash

SEC_DIR=../tools/secimage
TOOLS_PATH=../tools/

TARGET=upgrade_sbi

if [ "$UNASSIGNED_BUILD" == "1" ]
then
	# For Unassigned builds, just prepend the binary with
	# zeroes equivalent to the size of the SBI header (0x40 for HMACSHA)
	# Note that these zeros will act as NOP and ensure that the
	# target image is loaded at the correct linked address
	dd if=/dev/zero of=sbi_zero.bin bs=64 count=1
	cat sbi_zero.bin $TARGET.bin > sbi.bin
	rm sbi_zero.bin

	EXEC_MODE=0xFFFC # Load and Execute (No auth)
else
	# Copy the file to be signed to secdir
	cp $TARGET.bin  ${SEC_DIR}/$TARGET.bin

	# Sign the target binary only for AB builds
	cd ${SEC_DIR}
	./sign_$TARGET.sh $TARGET.bin
	cd -

	# tocgen script assumes the input file name is sbi.bin
	mv ${SEC_DIR}/${TARGET}.sign.bin sbi.bin
	rm ${SEC_DIR}/$TARGET.bin

	EXEC_MODE=0xFFFB # Load, Authenticate and Execute
fi

# Add TOC header
${TOOLS_PATH}/scripts/tocgen_v4.py -o ${TARGET}.sign.toc.bin -e $EXEC_MODE

# Remove Temporary files
rm sbi.bin

echo DONE
