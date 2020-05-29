#!/bin/bash
#./secimage chain -out chain.out -rsa -key domec.dauth.rsa_privkey.pem 0x0 0x305 0x500 -key rsa_sbi1.pem 0x0 0x305 0x500
./secimage sbi -out fabi_p_sbi_usb.bin -config lynx_unauth.cfg  -bl p_sbi.bin 





