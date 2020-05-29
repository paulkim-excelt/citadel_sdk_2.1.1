# hmac sha256 sign
secimage sbi -out fabi.bin -hmac khmacsha256 -bl f.bin

cat add0.bin fabi.bin > final_symmetric_hmac.bin

rm -Rf fabi.bin

