# generate unsigned COT to submit to IT signing process
echo '../secimage chain -out chain.nosign.bin -rsanosign -public CYGNUS-B0-CHAIN1.pem 0x60000007 0xABCD 0x01'
../secimage chain -out chain.nosign.bin -rsanosign -key -public CYGNUS-B0-CHAIN1.pem 0x60000007 0xABCD 0x01
echo 'Submit chain.nosign.bin to SecureSign process'
