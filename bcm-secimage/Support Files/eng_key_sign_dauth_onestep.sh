../secimage chain -out chain.out -rsa -key rsa_eng_key.pem 0x60000007 0x8000 0x01 -key rsa_sbi1.pem 0x60000007 0x8000 0x01 

../secimage sbi -out onestep_dauth.bin -config unauth.cfg -dauth -public rsa_eng_key_pub.pem -chain chain.out -rsa rsa_sbi1.pem -bl simple_sbi.bin -depth 1 -SBIConfiguration 0x10 -SBIRevisionID 0x10 -CustomerID 0x60000007 -ProductID 0x8000 -CustomerRevisionID 0x01 -BroadcomRevisionID 0x0001 -SBIUsage 0x0
