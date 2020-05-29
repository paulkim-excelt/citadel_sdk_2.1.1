../secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x0 0xABCD 0x01 -key rsa_sbi1.pem 0x0 0xABCD 0x01 

../secimage sbi -out onestep.bin -config unauth.cfg -chain chain.out -rsa rsa_sbi1.pem -bl simple_sbi.bin -depth 1 -SBIConfiguration 0x10 -SBIRevisionID 0x10 -CustomerID 0x0 -ProductID 0xABCD -CustomerRevisionID 0x01 -SBIUsage 0x0
