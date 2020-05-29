#!/usr/bin/env python
import sys
import string
import base64
import argparse
import re
import binascii

rsa_private_key = "BEGIN RSA PRIVATE KEY"
rsa_public_key  = "BEGIN RSA PUBLIC KEY"
private_key     = "BEGIN PRIVATE KEY"
public_key      = "BEGIN PUBLIC KEY"

def print_array(data):
    data = data[1:]
    if (len(data) == 256):
        print "int encryption = ENCRYPT_RSA2048;";
        print "uint8_t RSA2KpubKeyModulus[256] = {"
        for i in range(0, len(data)):
            if (i % 8 == 0):
                print "    ",
            print "0x%02x, "%ord(data[i]),
            if (i % 8 == 7):
                print ""
        print "};"
        print "uint8_t RSA4KpubKeyModulus[512] = {0};"
        print "uint8_t ECDSApubKey[64] = {0};"
    elif (len(data) == 512):
        print "int encryption = ENCRYPT_RSA4096;";
        print "uint8_t RSA2KpubKeyModulus[256] = {0};"
        print "uint8_t RSA4KpubKeyModulus[512] = {"
        for i in range(0, len(data)):
            if (i % 8 == 0):
                print "    ",
            print "0x%02x, "%ord(data[i]),
            if (i % 8 == 7):
                print ""
        print "};"
        print "uint8_t ECDSApubKey[64] = {0};"
    else:
        print "// Key length %d is not supported!"%len(data);

def get_length(data):
    head = data[0:4]
    if (ord(head[1]) == 0x81):
        length = ord(head[2])
        data = data[3:]
    elif (ord(head[1]) == 0x82):
        length = ord(head[2])*256+ord(head[3])
        data = data[4:]
    else:
        length = ord(head[1])
        data = data[2:]
    return (data, length)

def get_data(data, length):
    return (data[length:], data[0:length])

def parse_rsa_private(pem_data):
    # sequence
    (pem_data, length) = get_length(pem_data)
    # version
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # modulus
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    print_array(output)
    # publicExponent
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # privateExponent
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # prime1
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # prime2
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # exponent1
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # exponent2
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # coefficient
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    return

def parse_rsa_public(pem_data):
    # sequence
    (pem_data, length) = get_length(pem_data)
    # modulus
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    print_array(output)
    # exponent
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    return

def parse_public(pem_data):
    # sequence
    (pem_data, length) = get_length(pem_data)
    # sequence algorithm
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # BIT STRING
    (pem_data, length) = get_length(pem_data)
    pem_data = pem_data[1:]
    parse_rsa_public(pem_data)
    return

def parse_private(pem_data):
    # sequence
    (pem_data, length) = get_length(pem_data)
    # version
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length) 
    # sequence algorithm
    (pem_data, length) = get_length(pem_data)
    (pem_data, output) = get_data(pem_data, length)
    # BIT STRING
    (pem_data, length) = get_length(pem_data)
    pem_data = pem_data[1:]
    parse_rsa_private(pem_data)   
    return

def parse_pem(name):
    try:
        file = open(name, "r")
    except:
        print "Cant't open " + name
        sys.exit(1)
    is_rsa_public = 0
    is_rsa_private = 0
    is_public = 0
    is_private = 0
    pem_str = file.read()
    file.close
    is_rsa_public = string.find(pem_str, rsa_public_key)
    is_rsa_private = string.find(pem_str, rsa_private_key)
    is_public = string.find(pem_str, public_key)
    is_private = string.find(pem_str, private_key)
    if (is_rsa_public > 0):
        print "// PEM file is RSA public key\n"
    elif (is_rsa_private > 0):
        print "// PEM file is RSA private key\n"
    elif (is_public > 0):
        print "// PEM file is public key\n"
    elif (is_private > 0):
        print "// PEM file is private key\n"
    else:
        print "// PEM file is invalid"
        print "%s\n"%pem_str
        return;
    pem_str = re.sub("-----[\s\w]+-----", "", pem_str)
    pem_str = string.strip(pem_str, "\r\n")
    pem_data = base64.b64decode(pem_str)
    if (is_rsa_private > 0):
        parse_rsa_private(pem_data)
    elif (is_rsa_public > 0):
        parse_rsa_public(pem_data)
    elif (is_public > 0):
        parse_public(pem_data)
    elif (is_private > 0):
        parse_private(pem_data)
    else:
        print binascii.b2a_hex(pem_data)
    return;

def str2int(in_str):
    sub_str = in_str[0:2]
    if (sub_str == "0x"):
        return int(in_str, 16)
    else:
        return int(in_str, 10)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", nargs=1, type=str, help="PEM file", required=True)
    parser.add_argument("-ci", nargs=1, type=str2int, help="CustomerID", required=True)
    parser.add_argument("-csc", nargs=1, type=str2int, help="CustomerSBLConfiguration", required=True)
    parser.add_argument("-cri", nargs=1, type=str2int, help="CustomerRevisionID", required=True)
    parser.add_argument("-sri", nargs=1, type=str2int, help="SBIRevisionID", required=True)
    args = parser.parse_args()
    key_name = args.f[0]  
    customer_id = args.ci[0]
    customer_sbl_configuration = args.csc[0]
    customer_revision_id = args.cri[0]
    sbi_revision_id = args.sri[0]
    print "#include \"customization_key.h\"\n"
    parse_pem(key_name)
    print "uint32_t CustomerID = 0x%x;"%customer_id
    print "uint32_t CustomerSBLConfiguration = 0x%x;"%customer_sbl_configuration
    print "uint16_t CustomerRevisionID = 0x%x;"%customer_revision_id
    print "uint16_t SBIRevisionID = 0x%x;"%sbi_revision_id

