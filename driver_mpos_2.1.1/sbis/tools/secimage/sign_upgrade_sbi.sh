#!/bin/bash
secapp=./secimage

if [ "$1" == "" ]
then
	echo "Usage: $0 <in_file>"
	exit 1
fi

IN_FILE=$1
OUT_FILE=`echo $IN_FILE | sed -E 's/(^.+)\.bin/\1.sign.bin/'`

# Parameters for 58201 A0 Citadel POS parts
PRODUCT_ID=0x1202
CUSTOMER_ID=0x60000003
SBI_REVISION_ID=0x0
CUSTOMER_REVISION_ID=0x0
HMAC_SHA256_KEY_FILE=khmacsha256
AUTH_CFG_FILE=unauth.cfg
SBI_CONFIG_HMACSHA=0x40

./${secapp} sbi -out        $OUT_FILE              \
	-config             $AUTH_CFG_FILE         \
	-hmac               $HMAC_SHA256_KEY_FILE  \
	-bl                 $IN_FILE               \
	-SBIConfiguration   $SBI_CONFIG_HMACSHA    \
	-SBIRevisionID      $SBI_REVISION_ID       \
	-CustomerID         $CUSTOMER_ID           \
	-ProductID          $PRODUCT_ID            \
	-CustomerRevisionID $CUSTOMER_REVISION_ID  \
	-SBIUsage           0x0
