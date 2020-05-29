chmod +w chain.out
./secimage chain -out chain.out -rsa -sha256 -key -mont rsa_dauth4096_key.pem 0x003E0000 0 -key -mont rsa_sbi1_4096.pem 0x003E0000 0 -key -mont rsa_sbi2_4096.pem 0x003E0000 0 -key -mont rsa_sbi3_4096.pem 0x003E0000 0

# rsa sign with chain of trust file
# NOTE: must use the last -key in the chain of trust file to sign SBI
./secimage sbi -out fabi.bin -config unauth.cfg -dauth -mont -public rsa_dauth4096_key_pub.pem -chain chain.out -rsa rsa_sbi3_4096.pem -bl f.bin -depth 3

mv Dauth_hash.out Dauth_hash_eng_4k.out

#### final:
cat add0.bin fabi.bin > final_c0_eng_dauth_4k.bin

