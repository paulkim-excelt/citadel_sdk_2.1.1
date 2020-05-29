#!/bin/bash 

if [ "$1" == "" ]
then
	echo "Usage: $0 <in_file>"
	exit 1
fi

# Get input and output file names (image.bin -> image.sign.bin)
IN_FILE=$1
OUT_FILE=`echo $IN_FILE | sed -E 's/(^.+)\.bin/\1.sign.bin/'`

# Parameters for 58201 A0 Citadel POS parts
PRODUCT_ID=0x1202
BROADCOM_REVISION_ID=0x2A0
CUSTOMER_ID=0x60000003
SBI_REVISION_ID=0x0
CUSTOMER_REVISION_ID=0x0
DEV_AUTH_PRIVATE_KEY=rsa_dauth4096_key.pem
DEV_AUTH_PUBLIC_KEY=rsa_dauth4096_key_pub.pem
CHAIN_OF_TRUST_1_KEY=rsa_sbi1_4096.pem
AUTH_CFG_FILE=unauth.cfg
SBI_CONFIG_RSA=0x10

secapp=./secimage

${secapp} chain -out chain.out \
	-rsa -key $DEV_AUTH_PRIVATE_KEY $CUSTOMER_ID $PRODUCT_ID $CUSTOMER_REVISION_ID \
	     -key $CHAIN_OF_TRUST_1_KEY $CUSTOMER_ID $PRODUCT_ID $CUSTOMER_REVISION_ID \
	     -v

${secapp} sbi -out        $OUT_FILE              \
	-config             $AUTH_CFG_FILE         \
	-sbialign           0x10 0x60000000        \
	-dauth -public      $DEV_AUTH_PUBLIC_KEY   \
	-chain              chain.out              \
	-rsa                $CHAIN_OF_TRUST_1_KEY  \
	-bl                 $IN_FILE               \
	-depth              1                      \
	-SBIConfiguration   $SBI_CONFIG_RSA        \
	-SBIRevisionID      $SBI_REVISION_ID       \
	-CustomerID         $CUSTOMER_ID           \
	-ProductID          $PRODUCT_ID            \
	-CustomerRevisionID $CUSTOMER_REVISION_ID  \
	-BroadcomRevisionID $BROADCOM_REVISION_ID  \
	-SBIUsage           0x0                    \
	-v


