/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: crypto.c
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ec.h>


/* DSA flag options */
#define DUMP_PUBLIC		0x00000001
#define DUMP_PRIVATE	0x00000002
#define DUMP_PARAMETERS	0x00000004
#define DUMP_ALL		0x00000007


/* Function Prototypes */
DSA *GetDefaultDSAParameters(void);
DH  *GetDefaultDHParameters(void);
BIGNUM *GetPeerDHPublicKey(char *filename);
long base64_decode(char *in, long inlen, char *out);
int	DsaSign(uint8_t *data, uint32_t len, uint8_t *sig, DSA *dsa_key);
DSA *GetDSAPrivateKeyFromFile(char *filename);
DSA *GetDSAPublicKeyFromFile(char *filename);
void PrintDSAKey (DSA *dsa_key, unsigned int flag);
int     RsaSign(uint8_t *data, uint32_t len, uint8_t *sig, RSA *rsa_key, uint32_t dauth, int verbose);
RSA *GetRSAPrivateKeyFromFile(char *filename, int verbose);
RSA *GetRSAPublicKeyFromFile(char *filename, int verbose);
void PrintRSAKey (RSA *rsa_key, unsigned int flag);
void PrintDHKey (DH *dh_key, unsigned int flag);
void PrintDSASig (DSA_SIG *dsa_sig, unsigned int flag);
int PutDHParameterToFile(DH *dh_key, char *filename);
int CheckDHParam(char *filename);
int Sha256Hash (uint8_t *data, uint32_t len, uint8_t *hash, int verbose);
EC_KEY *GetECPrivateKeyFromFile(char *filename, int verbose);
EC_KEY *GetECPublicKeyFromFile(char *filename, int verbose);
void PrintECKey (EC_KEY* ec_key);
int	ECDSASign(uint8_t *data, uint32_t len, uint8_t *sig, EC_KEY *ec_key, int verbose);
int EC_Size(EC_KEY* ek);
int EC_KEY2bin(EC_KEY* ek, unsigned char *to);
int AppendECDSASignature(uint8_t *data, uint32_t length, char *filename, int verbose, char *key);
#endif /* _CRYPTO_H_ */
