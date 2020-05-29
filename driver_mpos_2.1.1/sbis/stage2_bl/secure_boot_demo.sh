#! /bin/bash

# Check that Secure boot demo is not being built for unassigned parts
if [ "$UNASSIGNED_BUILD" == "1" ]
then
	echo "Error: Secure boot demo not valid for Unassigned builds!!!"
	exit 1
fi

# Build the sample AAI application
CWD=$PWD
cd ../sample_aai
make clean && make target=flash
cp -f sample_aai.sign.bin $CWD/sample_aai.signed

# Build Stage 2 Boot loader
cd -
make clean && make target=flash

# Get AAI image offset and pad the S2BL image appropriately
# to have the AAI located at the offset for which it is built
AAI_IMAGE_OFFSET=`grep DEF_AAI_IMAGE_OFFSET ../make.cfg  | head -1 | sed -E 's/^.+DEF_AAI_IMAGE_OFFSET=//'`
((offset=$AAI_IMAGE_OFFSET))

# Merge the 2 into a single binary so as
# to have the Stage 2BL occupy the first 'offset' bytes
dd if=/dev/zero of=ff.bin bs=$offset count=1
# Append 'offset' bytes of 0xff to the S2 BL binary
cat stage2_bl.sign.toc.bin ff.bin > s2bl_padded.bin
# Truncate the s2bl binary to 'offset' bytes
dd bs=$offset count=1 if=s2bl_padded.bin of=s2bl_padded_to_aai_size.bin
# Merge the trauncated S2BL and the sample AAI binary
cat s2bl_padded_to_aai_size.bin sample_aai.signed > secure_boot_demo.bin
# Remove temporary files
rm sample_aai.signed s2bl_padded_to_aai_size.bin s2bl_padded.bin ff.bin
