# generate unsigned COT to submit to IT signing process
echo '../secimage chain -out chain.nosign.bin -rsanosign -public NORTHSTAR_PLUS-B0-CHAIN1.pem 0x0 0x8000 0x01'
../secimage chain -out chain.nosign.bin -rsanosign -key -public NORTHSTAR_PLUS-B0-CHAIN1.pem 0x0 0x8000 0x01
echo 'Submit chain.nosign.bin to SecureSign process'
