# append the IT provided signature to the boot image blob
cat production.fsbi.boot_image.bin sign_me.rsa.hash.productid.0x8000.hash.file_4452_NORTHSTAR_PLUS-B0-CHAIN1.sign > PRODUCTION_SBI_DAUTH.bin
