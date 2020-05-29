../secimage chain -out chain.nosign.bin -rsanosign -key -public rsa_sbi1_pub.pem 0x60000007 0x8000 0x01

openssl dgst -sha256 -sign rsa_eng_key.pem -out nosign.sig.bin chain.nosign.bin

../secimage chain -out chain.bin -sig nosign.sig.bin -rsa -key -public rsa_sbi1_pub.pem 0x60000007 0x8000 0x01

../secimage sbi -out fsbi.boot_image.bin -rsanosign 256 0x0011 -dauth -public rsa_eng_key_pub.pem -chain chain.bin -config unauth.cfg -bl simple_sbi.bin -depth 1 -SBIConfiguration 0x10 -SBIRevisionID 0x10 -CustomerID 0x60000007 -ProductID 0x8000 -CustomerRevisionID 0x01 -BroadcomRevisionID 0x0001 -SBIUsage 0x0

openssl dgst -sha256 -sign rsa_sbi1.pem -out fsbi.boot_image.bin.sig sign_me.rsa.hash 

cat fsbi.boot_image.bin fsbi.boot_image.bin.sig > SBI_DAUTH.bin
