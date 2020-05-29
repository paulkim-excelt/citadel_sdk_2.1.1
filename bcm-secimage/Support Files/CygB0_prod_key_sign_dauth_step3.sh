# append the IT provided signature to the boot image blob
cat production.fsbi.boot_image.bin sign_me.rsa.hash.cygnus.b0.productid.0xabcd.file_4846_CYGNUS-B0-CHAIN1.sign > PRODUCTION_SBI_DAUTH_CygnusB0.bin
