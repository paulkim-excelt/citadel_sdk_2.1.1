chmod +w chain.out
#./secimage chain -out chain.out -rsa -sha256 -key -mont rsa_dauth4096_key.pem 0x00300000 0 -key -mont rsa_sbi1_4096.pem 0x00300000 0 -key -mont rsa_sbi2_4096.pem 0x00300000 0 -key -mont rsa_sbi3_4096.pem 0x00300000 0
./secimage chain -out chain.out -rsa -sha256 -key -mont rsa_dauth4096_key.pem 0x00300000 0 -BBLStatus $2 -key -mont rsa_sbi3_4096.pem 0x00300000 0 -BBLStatus $2

# rsa sign with chain of trust file
# NOTE: must use the last -key in the chain of trust file to sign SBI
./secimage sbi -out fabi.bin -config unauth.cfg -dauth -mont -public rsa_dauth4096_key_pub.pem -chain chain.out -rsa rsa_sbi3_4096.pem -bl $1 -depth 1 

mv Dauth_hash.out Dauth_hash_production_4k.out
#### final:
cat add0.bin fabi.bin > $1.signed

rm -Rf fabi.bin

