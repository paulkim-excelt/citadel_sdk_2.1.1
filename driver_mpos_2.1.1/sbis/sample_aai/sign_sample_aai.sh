#!/bin/bash

SEC_DIR=../tools/secimage

TARGET=sample_aai
cp -f $TARGET.bin  ${SEC_DIR}/$TARGET.bin
cd ${SEC_DIR}

echo Signing image with RSA 2K
./sign_image_rsa_a0_ab.sh $TARGET.bin

cd -
cp -f ${SEC_DIR}/$TARGET.sign.bin ./

