# hmac sha256 sign with aes encryption
secimage sbi -out fabi.bin -dl kaes256 -iv ivaes -hmac khmacsha256 -bblkey BBLkey.bin -bl f.bin

cat add0.bin fabi.bin > final_symmetric_hmac_aes_bbl.bin

rm -Rf fabi.bin

