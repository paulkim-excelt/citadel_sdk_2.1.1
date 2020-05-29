chmod +w chain.out
secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x003E0000 0 -key rsa_sbi1.pem 0x003E0000 0 -key rsa_sbi2.pem 0x003E0000 0 -key rsa_sbi3.pem 0x003E0000 0

# rsa sign with chain of trust file
# NOTE: must use the last -key in the chain of trust file to sign SBI
secimage sbi -out fabi.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi3.pem -bl recovery_f.bin -depth 3

#### final:
cat add0.bin fabi.bin > recovery_image

rm -Rf fabi.bin


## Create the Recovery image.

secimage sbi -out fabi.bin -dl kaes256 -iv ivaes -hmac khmacsha256 -bblkey BBLkey.bin -bl primary_f.bin -recovery recovery_image

cat add0.bin fabi.bin > eng_final_recovery_hmac_aes_bbl_primary.bin


secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x003E0000 0 -key rsa_sbi1.pem 0x003E0000 0 -BBLStatus 1 -key rsa_sbi2.pem 0x003E0000 0 -BBLStatus 1 -key rsa_sbi3.pem 0x003E0000 0 -BBLStatus 1

secimage sbi -out fabi.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi3.pem -bl primary_f.bin -depth 3 -recovery recovery_image

cat add0.bin fabi.bin > eng_final_primary_bbl_1_recovery.bin


secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x003E0000 0 -key rsa_sbi1.pem 0x003E0000 0 -BBLStatus 2 -key rsa_sbi2.pem 0x003E0000 0 -BBLStatus 2 -key rsa_sbi3.pem 0x003E0000 0 -BBLStatus 2

secimage sbi -out fabi.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi3.pem -bl primary_f.bin -depth 3 -recovery recovery_image

cat add0.bin fabi.bin > eng_final_primary_bbl_2_recovery.bin

secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x003E0000 0 -key rsa_sbi1.pem 0x003E0000 0 -BBLStatus 3 -key rsa_sbi2.pem 0x003E0000 0 -BBLStatus 3 -key rsa_sbi3.pem 0x003E0000 0 -BBLStatus 3

secimage sbi -out fabi.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi3.pem -bl primary_f.bin -depth 3 -recovery recovery_image

cat add0.bin fabi.bin > eng_final_primary_bbl_3_recovery.bin


secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x003E0000 0 -key rsa_sbi1.pem 0x003E0000 0 -BBLStatus 4 -key rsa_sbi2.pem 0x003E0000 0 -BBLStatus 4 -key rsa_sbi3.pem 0x003E0000 0 -BBLStatus 4

secimage sbi -out fabi.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi3.pem -bl primary_f.bin -depth 3 -recovery recovery_image

cat add0.bin fabi.bin > eng_final_primary_bbl_4_recovery.bin


rm -Rf fabi.bin
rm -Rf recovery_image



