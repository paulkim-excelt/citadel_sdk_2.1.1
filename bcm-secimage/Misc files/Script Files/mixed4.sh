chmod +w chain.out
secimage chain -out chain.out -rsa -sha256 -key rsa_dauth4096_key.pem 0x001E0000 0 -key rsa_sbi1.pem 0x001E0000 0 -key rsa_sbi2.pem 0x001E0000 0 -key rsa_sbi3.pem 0x001E0000 0

# rsa sign with chain of trust file
# NOTE: must use the last -key in the chain of trust file to sign SBI
secimage sbi -out fabi.bin -config unauth.cfg -dauth -public rsa_dauth4096_key_pub.pem -chain chain.out -rsa rsa_sbi3.pem -bl f.bin -depth 3

#### final:
cat add0.bin fabi.bin > final.bin

