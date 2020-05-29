# take the IT generated signature and have secimage produce a sign_me to submit again to IT
../secimage chain -out chain.bin -sig chain.nosign.bin.cygnus.b0.productid.0xabcd.bin.file_4836_CYGNUS-B0-ROT.sign -rsa -key -public CYGNUS-B0-CHAIN1.pem 0x60000007 0xABCD 0x01

../secimage sbi -out production.fsbi.boot_image.bin -rsanosign 256 0x0011 -dauth -public CYGNUS-B0-ROT.pem -chain chain.bin -config unauth.cfg -bl simple_sbi.bin -depth 1 -SBIConfiguration 0x10 -SBIRevisionID 0x10 -CustomerID 0x60000007 -ProductID 0xABCD -CustomerRevisionID 0x01 -BroadcomRevisionID 0x0001 -SBIUsage 0x0

echo 'Submit sign_me.rsa.hash to IT SecureSign'
