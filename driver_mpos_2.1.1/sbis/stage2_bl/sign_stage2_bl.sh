#!/bin/bash

SEC_DIR=../tools/secimage/
TOOLS_PATH=../tools/

TARGET=stage2_bl
cp -f $TARGET.bin  ${SEC_DIR}/

cd ${SEC_DIR}
echo Signing stage2_blwith RSA 2K
./sign_image_rsa_a0_ab.sh $TARGET.bin
cd -

# tocgen script assumes the input file name is sbi.bin
mv ${SEC_DIR}/${TARGET}.sign.bin sbi.bin

# Add Toc Header
${TOOLS_PATH}/scripts/tocgen_v4.py -o ${TARGET}.sign.toc.bin -e 0xFFFB

rm -f ${SEC_DIR}/$TARGET.bin
rm -f sbi.bin
