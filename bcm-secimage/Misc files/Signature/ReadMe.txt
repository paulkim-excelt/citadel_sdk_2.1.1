SecureSign Request (1073):
------------------------------------
HD0000003928776 

Input Files:
------------
- BCM5892-D0-01-CHAIN1.pem.hash (MD5:  42bf65c53586ee46f79b6f45a7ba0825)


Output Delivered:
-----------------
- ReadMe.txt
- BCM5892-D0-01-CHAIN1.pem.hash
- BCM5892-D0-01-CHAIN1.pem.hash.BCM5892-D0-01-ROT.sign.1337713129
- BCM5892-D0-01-ROT.cert
- BCM5892-D0-01-ROT.pem


Config:
-------
- Key Info:  BCM5892-D0-01-ROT - BCM5892-D0-01-ROT (RSA 2048 bit, SHA1)

- Sign Info: SHA256

Reason / Comment
----------------
Signature on hash of D0 chain1 key, for simple SBI with CID=0xFF and CREV=1, required for DV testing of D0 SBL.

Hash generated using 
/projects/BCM5880_EM1/work/svanti/sw/bcm5892/fw/tools/secimage/sbi_production_D0_CID_0xFF_CREV_1/step1.sh
