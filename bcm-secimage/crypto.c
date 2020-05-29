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
 *  Uses openssl library
 */

#include <stdio.h>
#include <string.h>
#include "secimage.h"
#include "crypto.h"
#include "sbi.h"

/* default HMAC SHA256 key, this is the same as what's in bootrom header */
unsigned char default_hmac_fixed[SHA256_HASH_SIZE]={
    0xd1, 0xef, 0xbc, 0xba, 0xd7, 0x98, 0xd8, 0x71,
    0x00, 0x3d, 0xee, 0x3b, 0xf7, 0xb8, 0x46, 0x1c,
    0x53, 0xa8, 0xb9, 0xc5, 0xb6, 0xdc, 0x57, 0xdc,
    0x12, 0x80, 0x63, 0x1d, 0xae, 0xa3, 0xe0, 0x03
};

#define BUFF_SIZE		10000

/*----------------------------------------------------------------------
 * Name    : LoadDefaultCrypto
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
static int LoadDefaultCrypto(void)
{
	return(0);
}

/*----------------------------------------------------------------------
 * Name    : InitCrypto
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int InitCrypto(void)
{
	OpenSSL_add_all_digests();
	return LoadDefaultCrypto();
}

/*----------------------------------------------------------------------
 * Name    : ShutdownCrypto
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int ShutdownCrypto(void)
{
	return(0);
}

/*----------------------------------------------------------------------
 * Name    : SBILocalCrypto
 * Purpose : SBI local crypto operations
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBILocalCrypto(uint32_t enc, uint8_t *data, uint32_t dataLen, uint8_t *ekey, uint32_t AESKeySize, uint8_t *iv, int verbose)
{
	EVP_CIPHER_CTX ctx;
	const EVP_CIPHER *evp_cipher=NULL;
	int status = STATUS_OK, len;

	if(iv == NULL)
	{
		if(AESKeySize == KEYSIZE_AES256)
			evp_cipher = EVP_aes_256_ecb();
		else if (AESKeySize == KEYSIZE_AES128)
			evp_cipher = EVP_aes_128_ecb();
	}
	else
	{
		if(AESKeySize == KEYSIZE_AES256)
			evp_cipher = EVP_aes_256_cbc();
		else if (AESKeySize == KEYSIZE_AES128)
			evp_cipher = EVP_aes_128_cbc();
	}

	if (verbose) {
	  printf ("\nSBI local encrypting/decrypt %d bytes\n", dataLen);
	  DataDump("SBI local encrypt/decrypt data", data, dataLen);
	}

	EVP_CIPHER_CTX_init(&ctx);

	status = EVP_CipherInit_ex(&ctx, evp_cipher, NULL, ekey, iv, enc);
	if (status != 1)
		printf("EVP_CipherInit_ex error\n");
	/* disable padding function */
	EVP_CIPHER_CTX_set_padding(&ctx, 0);
	status = EVP_CipherUpdate(&ctx, data, &len, data, dataLen);
	if (status != 1)
		printf("EVP_CipherUpdate error\n");
	status = EVP_CipherFinal_ex(&ctx, data, &len);
	if (status != 1)
		printf("EVP_CipherFinal_ex error\n");

	if (verbose)
	  DataDump("SBI encrypted/decrypted data", data, dataLen);

	EVP_CIPHER_CTX_cleanup(&ctx);

	return status;
}

/*----------------------------------------------------------------------
 * Name    : Sha1Hash
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int Sha1Hash (uint8_t *data, uint32_t len, uint8_t *hash)
{
	EVP_MD_CTX ctx;

#ifdef DEBUG_DATA
	DataDump("SHA1 Data", data, len);
#endif
	DataDump("SHA1 Data", data, len); //fye

	EVP_MD_CTX_init(&ctx);
	EVP_DigestInit_ex(&ctx, EVP_sha1(), NULL);
	EVP_DigestUpdate(&ctx, data, len);
	EVP_DigestFinal_ex(&ctx, hash, NULL);

#ifdef DEBUG_SHA256
	DataDump("SHA hash", hash, SHA1_HASH_SIZE);
#endif
	DataDump("SHA hash", hash, SHA1_HASH_SIZE); //fye

	EVP_MD_CTX_cleanup(&ctx);
	return 0;
}


/*----------------------------------------------------------------------
 * Name    : Sha256Hash
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int Sha256Hash (uint8_t *data, uint32_t len, uint8_t *hash, int verbose)
{
	EVP_MD_CTX ctx;

	if (verbose)
	  DataDump("SHA256 Data", data, len);

	EVP_MD_CTX_init(&ctx);
	EVP_DigestInit_ex(&ctx, EVP_sha256(), NULL);
	EVP_DigestUpdate(&ctx, data, len);
	EVP_DigestFinal_ex(&ctx, hash, NULL);

	if (verbose)
	  DataDump("SHA hash", hash, SHA256_HASH_SIZE);

	EVP_MD_CTX_cleanup(&ctx);
	return 0;
}

/*----------------------------------------------------------------------
 * Name    : HmacSha256Hash
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int HmacSha256Hash (uint8_t *data, uint32_t len, uint8_t *hash, uint8_t *key, int verbose)
{
	HMAC_CTX hctx;

	if (verbose)
	  DataDump("HMAC Data ", data, len); //fye

	HMAC_CTX_init (&hctx);
	HMAC_Init_ex (&hctx, key, SHA256_HASH_SIZE, EVP_sha256(), NULL);

	HMAC_Init_ex (&hctx, NULL, 0, NULL, NULL); /* FIXME: why we need this? NULL means to use whatever there is? if removed, result is different */
	HMAC_Update (&hctx, data, len);
	HMAC_Final (&hctx, hash, NULL);

	if (verbose)
	  DataDump("HMAC SHA256 hash", hash, SHA256_HASH_SIZE);

	HMAC_CTX_cleanup (&hctx);
	return 0;
}

/*----------------------------------------------------------------------
 * Name    : base64_decode
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
long base64_decode(char *in, long inlen, char *out)
{
	BIO *bio=NULL, *b64=NULL;
	//char buf[inlen*2];
	char buf[4096];
	long outlen;

	bio = BIO_new_mem_buf(in, inlen);
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_push(b64, bio);

	outlen = BIO_read(bio, buf, inlen * 2);
	if (outlen <= 0) {
		printf ("Error reading bio for b64 decode\n");
		outlen = 0;
	}
	if (outlen > MAX_DER_SIGNATURE_SIZE)
		outlen = 0;
	else
		memcpy(out, buf, outlen);

	if (bio)
		BIO_free_all(bio);
	return outlen;
}

/*----------------------------------------------------------------------
 * Name    : base64_encode
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
uint8_t *base64_encode(char *in, long inlen)
{
	BIO *bio=NULL, *b64=NULL;
	char *ptr;
	long plen = -1;
	uint8_t *res = NULL;

	bio = BIO_new(BIO_s_mem());
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_push(b64, bio);

	BIO_write(bio, in, inlen);
	BIO_flush(bio);

	plen = BIO_get_mem_data(bio, &ptr);
	res = malloc(plen+1);
	if (res) {
		memcpy (res, ptr, plen);
		res[plen] = '\0';
	}

	if (bio)
		BIO_free_all(bio);
	return res;
}

/*----------------------------------------------------------------------
 * Name    : Shared2Bignum
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int Shared2Bignum(uint8_t *buf, int len)
{
	BIGNUM *shared = NULL;

	shared = BN_bin2bn (buf, len, NULL);
	if (shared == NULL) {
		printf ("Error reading shared secret\n");
		return SBERROR_SHARED_SECRET_FORMAT;
	}

	if (shared)
		BN_free(shared);

	return 0;
}

/* this function does sha256 then sha1 */
int GenerateDataHash1(uint8_t *data, uint32_t len, uint8_t *digest, int verbose)
{
	uint8_t hdigest[SHA256_HASH_SIZE];
	int status = 0;

#ifdef SUN_OS
	RAND_seed((const void *)hdigest, 128);
#endif
	status = HmacSha256Hash (data, len, hdigest, default_hmac_fixed, verbose);
	if (status) {
		printf ("HMAC-SHA256 hash error\n");
		return status;
	}

	if (verbose)
	  DataDump ("HMAC-SHA256 hash", hdigest, SHA256_HASH_SIZE);

	status = Sha1Hash (hdigest, SHA256_HASH_SIZE /* plain size */, digest);
	if (status) {
		printf ("SHA1 hash error\n");
		return status;
	}
	return status;
}

/* this function does sha256 then sha256 */
int GenerateDataHash(uint8_t *data, uint32_t len, uint8_t *digest, int verbose)
{
	int status = 0;

	if (verbose)
	  DataDump ("Data to hash", data, len);


#ifdef SUN_OS
	RAND_seed((const void *)hdigest, 128);
#endif

	status = Sha256Hash (data, len, digest, verbose);
	if (status) {
		printf ("SHA256 hash error\n");
		return status;
	}
	return status;
}



/*----------------------------------------------------------------------
 * Name    : GetRSAPrivateKeyFromFile
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
RSA * GetRSAPrivateKeyFromFile(char *filename, int verbose)
{
	RSA *rsa_key = NULL;
	BIO *in=NULL;

	in=BIO_new(BIO_s_file());
	if (in == NULL) {
		printf("Error BIO file \n");
		return NULL;
	}

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening RSA key file %s\n", filename);
		return NULL;
	}

	rsa_key=PEM_read_bio_RSAPrivateKey(in, NULL, NULL, NULL);
	if (rsa_key == NULL) {
		printf("Unable to load RSA Key\n");
		return NULL;
	}

	if (verbose) {
	  printf("\r\n DUMPING RSA PRIVATE KEY at the beginning " );
	  PrintRSAKey (rsa_key, DUMP_ALL);
	}
	BIO_free(in);

	return rsa_key;
}


/*----------------------------------------------------------------------
 * Name    : GetECPrivateKeyFromFile
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
EC_KEY * GetECPrivateKeyFromFile(char *filename, int verbose)
{
	EC_KEY *ec_key;
	BIO *in=NULL;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening EC key file %s\n", filename);
		return NULL;
	}

	ec_key=PEM_read_bio_ECPrivateKey(in, NULL, NULL, NULL);
	if (ec_key == NULL) {
		printf("Unable to load EC Key\n");
		return NULL;
	}

	if (verbose) {
	  printf("\r\n DUMPING EC KEY\n" );
	  PrintECKey (ec_key);
	}

	return ec_key;
}


/*----------------------------------------------------------------------
 * Name    : GetRSAPublicKeyFromFile
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
RSA * GetRSAPublicKeyFromFile(char *filename, int verbose)
{
	RSA *rsa_key;
	BIO *in=NULL;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening RSA key file %s\n", filename);
		return NULL;
	}

	rsa_key=PEM_read_bio_RSA_PUBKEY(in, NULL, NULL, NULL);
	if (rsa_key == NULL) {
		printf("Unable to load EC Key\n");
		return NULL;
	}

	if (verbose) {
	  printf("\r\n DUMPING RSA KEY at the beginning " );
	  PrintRSAKey (rsa_key, DUMP_PUBLIC);
	}

	return rsa_key;
}

/*----------------------------------------------------------------------
 * Name    : GetECPublicKeyFromFile
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
EC_KEY * GetECPublicKeyFromFile(char *filename, int verbose)
{
	EC_KEY *ec_key;
	BIO *in=NULL;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening RSA key file %s\n", filename);
		return NULL;
	}

	ec_key=PEM_read_bio_EC_PUBKEY(in, NULL, NULL, NULL);
	if (ec_key == NULL) {
		printf("Unable to load RSA Key\n");
		return NULL;
	}

	if (verbose) {
	  printf("\r\n DUMPING EC KEY\n" );
	  PrintECKey (ec_key);
	}

	return ec_key;
}

/*----------------------------------------------------------------------
 * Name    : PrintRSAKey
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
void PrintRSAKey (RSA *rsa_key, unsigned int flag)
{
	if (flag & DUMP_PARAMETERS) {
		printf("RSA Parameters\n");
		printf("Exponent e: %d bytes\n%s\n\n",
			BN_num_bytes(rsa_key->e), BN_bn2hex(rsa_key->e));
	}
	if (flag & DUMP_PRIVATE) {
		printf("Private d: %d bytes\n%s\n\n",
			BN_num_bytes(rsa_key->d), BN_bn2hex(rsa_key->d));
	}
	if (flag & DUMP_PUBLIC) {
		printf("Modulus m: %d bytes\n%s\n\n",
			BN_num_bytes(rsa_key->n), BN_bn2hex(rsa_key->n));
	}
}

/*----------------------------------------------------------------------
 * Name    : PrintECKey
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
void PrintECKey (EC_KEY* ec_key)
{
	BIO *mem = BIO_new(BIO_s_mem());
	BUF_MEM *bptr;
    long datalen = -1;
    char *data = NULL;

	EC_KEY_print(mem, ec_key, 0);
	
    datalen = BIO_get_mem_data(mem, &bptr);
    data = (char*)malloc(datalen+1);
    memcpy (data, bptr, datalen);
    data[datalen] = '\0';
    printf("%s", data);
	free(data);
	BIO_set_close(mem, BIO_NOCLOSE); /* So BIO_free() leaves BUF_MEM alone */
	BIO_free(mem);

}

char pkcs_sha1[SHA1_DER_PREAMBLE_LENGTH] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
	0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
};

char pkcs_sha256[SHA256_DER_PREAMBLE_LENGTH] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
    0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};


#define PAD_OFFSET		(PKCS1_PAD_SIZE-SHA1_HASH_SIZE)

/*----------------------------------------------------------------------
 * Name    : GeneratePKCS1Padding
 * Purpose : Generate  PKCS1 SHA256 padding
 * Input   :
 * Output  :
 *---------------------------------------------------------------------*/
int GeneratePKCS1Padding(uint8_t *p, uint32_t emlen, uint32_t use_sha256, int verbose)
{
	uint32_t tlen, pslen;
	uint8_t *bp = p;
	int i;
	if(use_sha256) {
	   tlen  = SHA256_DER_PREAMBLE_LENGTH + SHA256_HASH_SIZE;
	}
	else {
		/* defaults to SHA1 */
	   tlen  = SHA1_DER_PREAMBLE_LENGTH + SHA1_HASH_SIZE;
	}

	if (emlen < tlen + 11)
		return SBERROR_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

	/* 5. Generate p = 0x00 || 0x01 || PS || 0x00 || T */
	*bp = 0x00; bp++;
	*bp = 0x01; bp++;

	for (i=0, pslen=0; i<((int)emlen - (int)tlen - 3); i++)
	{
		*bp = 0xFF;
		bp++;
		pslen++;
	}
    if (pslen < EMSA_PKCS1_V15_PS_LENGTH)
		return SBERROR_RSA_PKCS1_ENCODED_MSG_TOO_SHORT;

    /* Post the last flag uint8_t */
    *bp = 0x00; bp++;

    /* Post T(h) data into em(T) buffer */
	if(use_sha256) {
       memcpy(bp, pkcs_sha256, SHA256_DER_PREAMBLE_LENGTH);
       bp += SHA256_DER_PREAMBLE_LENGTH;
	}
	else {
       memcpy(bp, pkcs_sha1, SHA1_DER_PREAMBLE_LENGTH);
       bp += SHA1_DER_PREAMBLE_LENGTH;
	}

	if (verbose)
	  DataDump ("PKCS1 Padding", p, emlen - (use_sha256 ? SHA256_HASH_SIZE : SHA1_HASH_SIZE));

	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : RsaSign
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int	RsaSign(uint8_t *data, uint32_t len, uint8_t *sig, RSA *rsa_key, uint32_t use_sha256, int verbose)
{
	int siglen;
	uint8_t buf[MAX_DER_SIGNATURE_SIZE * 2];
	uint8_t digest[SHA256_HASH_SIZE]; /* this is not sha256 size */
	uint8_t pkcs1Pad[MAX_DER_SIGNATURE_SIZE * 2];
	int status;

	// Always set use_sha256 to true
	use_sha256 = 1;

	if (rsa_key == NULL)
		return SBERROR_DSA_KEY_ERROR;

	if(use_sha256)
	{
	  status = GenerateDataHash(data, len, digest, verbose);
	}
	else
	{
	  status = GenerateDataHash1(data, len, digest, verbose);
	}
	if (status)
		return status;


	if (RSA_size(rsa_key) > (MAX_DER_SIGNATURE_SIZE * 2)) {
		status = SBERROR_DER_SIGNATURE_SIZE;
		printf ("Buffer size is too small for RSA signatures\n");
		return status;
	}

	status = GeneratePKCS1Padding(pkcs1Pad, RSA_size(rsa_key), use_sha256, verbose);
	if (status)
		return status;
	if(use_sha256) {
		memcpy (pkcs1Pad+RSA_size(rsa_key)-SHA256_HASH_SIZE, digest, SHA256_HASH_SIZE);
	}
	else {
		memcpy (pkcs1Pad+RSA_size(rsa_key)-SHA1_HASH_SIZE, digest, SHA1_HASH_SIZE);
	}

	if (verbose)
	  DataDump ("Data to sign", pkcs1Pad, RSA_size(rsa_key)); //fye

	status = RSA_private_encrypt(RSA_size(rsa_key), pkcs1Pad, buf,
					rsa_key, RSA_NO_PADDING);
	if (status == 0) {
		printf ("RSA sign error\n");
		return SBERROR_RSA_SIGN;
	}
	siglen = RSA_size(rsa_key);

	if (verbose)
	  DataDump ("RSA Signature", buf, siglen);

	memcpy(sig, buf, siglen);
	return siglen;
}

/*----------------------------------------------------------------------
 * Name    : ECDSASign
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int	ECDSASign(uint8_t *data, uint32_t len, uint8_t *sig, EC_KEY *ec_key, int verbose)
{
	unsigned int siglen;
	uint8_t digest[SHA256_HASH_SIZE]; /* this is not sha256 size */
	int status;
	ECDSA_SIG *signature; 

	if (ec_key == NULL)
		return SBERROR_EC_KEY_ERROR;

	status = GenerateDataHash(data, len, digest, verbose);
	if (status)
		return status;

	signature = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, ec_key);
	if(signature == NULL)
	{ 
		printf("ERROR ECDSA_sign\n"); 
		return SBERROR_FAIL_ECDSA_SIGN; 
	} 
	
	if (verbose)
	  printf("ECDSA Signature success \n");
	BN_bn2bin(signature->r, sig);

	if (verbose)
	  DataDump("ECDSA sig r:", sig, BN_num_bytes(signature->r));

	siglen = BN_num_bytes(signature->r);	
	BN_bn2bin(signature->s, sig+siglen);

	if (verbose)
	  DataDump("ECDSA sig s:", sig+siglen, BN_num_bytes(signature->s));

	siglen += BN_num_bytes(signature->s);

	if (verbose) {
	  DataDump ("ECDSA Signature", sig, siglen);
	  printf("ECDSA Signature length is %d \n",siglen); 
	}

	return siglen;
}

/*----------------------------------------------------------------------
 * Name    : AppendRSASignature
 * Purpose : Caluculates and appends RSA signature at the end of the data
 * Input   : data: over which the signature is calculated
 * 			 legnth: length of the data
 * 			 filename: to read the RSA private key
 * Output  : RSA signature appended at the end of data
 * 			 Returns the size of the signature if successfull
 *---------------------------------------------------------------------*/

int AppendRSASignature(uint8_t *data, uint32_t length, char *filename, uint32_t use_sha256, int verbose)
{
	RSA *rsa_key = NULL;
	uint8_t hdigest[SHA256_HASH_SIZE];
	int status = STATUS_OK;
	rsa_key = GetRSAPrivateKeyFromFile(filename, verbose);

	status = HmacSha256Hash (data, length, hdigest, default_hmac_fixed, verbose);

	if (verbose)
	  DataDump("\r\n The HMAC SHA256 of the image is ", hdigest, SHA256_HASH_SIZE);

	// TODO: Do HMAC_SHA256 of the SBI_Image
	status = RsaSign(hdigest, SHA256_HASH_SIZE, data + length, rsa_key, use_sha256, verbose);

	if (verbose)
	  DataDump("\r\n The final image signature is ", ( data + length), 0x200 );

	RSA_free(rsa_key);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : rsaSignaturePrework
 * Purpose : Caluculates the RSA KEY length and other pre requisites
 * Input   : key file
 * Output  : RSA size
 *---------------------------------------------------------------------*/
void rsaSignaturePrework(uint8_t *data, char *filename, int verbose)
{
	RSA *rsa_key = NULL;
	int size;
	rsa_key = GetRSAPrivateKeyFromFile(filename, verbose);


	size = RSA_size( rsa_key );
	((AUTHENTICATED_SBI_HEADER *)data)->AuthLength = size;
	((AUTHENTICATED_SBI_HEADER *)data)->ImageVerificationConfig = (IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5 | IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256);

	if (verbose)
	  printf("ImageVerificationConfig %08x\n", ((AUTHENTICATED_SBI_HEADER *)data)->ImageVerificationConfig); 
} 

/*----------------------------------------------------------------------
 * Name    : AppendECDSASignature
 * Purpose : Caluculates and appends ECDSA signature at the end of the data
 * Input   : data: over which the signature is calculated
 * 			 legnth: length of the data
 * 			 filename: to read the ECDSA private key
 * Output  : ECDSA signature appended at the end of data
 * 			 Returns the size of the signature if successfull
 *---------------------------------------------------------------------*/
int AppendECDSASignature(uint8_t *data, uint32_t length, char *filename, int verbose, char *ecdsPtr)
{
	EC_KEY *ec_key = (EC_KEY *)ecdsPtr;
	int status = STATUS_OK;
	uint8_t hdigest[SHA256_HASH_SIZE];


	status = HmacSha256Hash (data, length, hdigest, default_hmac_fixed, verbose);

	if (verbose)
	  DataDump("\r\n The HMAC SHA256 of the image is ", hdigest, SHA256_HASH_SIZE);

	// TODO: Do HMAC_SHA256 of the SBI_Image
	status = ECDSASign(hdigest, SHA256_HASH_SIZE, data + length, ec_key, verbose);

	if (verbose)
	  DataDump("\r\n The final image signature is ", ( data + length), 0x40 );

	EC_KEY_free(ec_key);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : ecdsSignaturePrework
 * Purpose : Caluculates the ECDS KEY length and other pre requisites
 * Input   : key file
 * Output  : ECDS size, ECDS key
 *---------------------------------------------------------------------*/
char * ecdsSignaturePrework(uint8_t *data, char *filename, int verbose)
{
	EC_KEY *ec_key = NULL;
	BIO *in=NULL;
	int  size;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening EC key file %s\n", filename);
		return SBERROR_DATA_FILE;
	}

	ec_key=PEM_read_bio_ECPrivateKey(in, NULL, NULL, NULL);
	if (ec_key == NULL) {
		printf("Unable to load EC Key\n");
		return SBERROR_EC_KEY_ERROR;
	}

	if (verbose) {
	  printf("\r\n ...............     Key used to  append final signature ........... \r\n");
	  PrintECKey (ec_key);
	}

	size = EC_Size( ec_key );
	((AUTHENTICATED_SBI_HEADER *)data)->AuthLength = size;
	((AUTHENTICATED_SBI_HEADER *)data)->ImageVerificationConfig = IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256;

	if (verbose)
	  printf("ImageVerificationConfig %08x\n", ((AUTHENTICATED_SBI_HEADER *)data)->ImageVerificationConfig); 
	BIO_free(in);
	return (char *)ec_key;
} 


/*----------------------------------------------------------------------
 * Name    : AppendHMACSignature
 * Purpose : Caluculates and appends HMAC signature at the end of the data
 * Input   : data: over which the signature is calculated
 * 			 legnth: length of the data
 * 			 filename: to read the HMAC key
 * Output  : HMAC appended at the end of data
 * 			 Returns the size of the hash if successfull
 *---------------------------------------------------------------------*/
int AppendHMACSignature(uint8_t *data, uint32_t length, char *filename, char *bblkeyfile, int verbose, int destOffset)
{
	uint8_t  hmackey[SHA256_HASH_SIZE];
	uint8_t  bblkey[KEYSIZE_AES128];
	uint32_t len, bbllen;
	uint32_t status;
	uint8_t *digest = data + destOffset;

	len = ReadBinaryFile(filename, hmackey, SHA256_HASH_SIZE);
	if (len != SHA256_HASH_SIZE) {
		printf("Error reading hmac key file \n");
		return -1;
	}

	if(bblkeyfile) {
		bbllen = KEYSIZE_AES128;
		status = DataRead(bblkeyfile, bblkey, &bbllen);
		if (!status) {
			/* Decrypting the OTP key using BBL key */
		  status = SBILocalCrypto(0, hmackey, SHA256_HASH_SIZE, bblkey, bbllen, NULL, verbose);
			status = (status == 1) ? 0 : 1;
		}

		if (status) {
			printf ("HMAC key decryption error\n");
			return -1;
		}

	}

	if (verbose)
	  printf("ImageVerificationConfig %08x\n", ((AUTHENTICATED_SBI_HEADER *)data)->ImageVerificationConfig); 

	status = HmacSha256Hash (data, length, digest, hmackey, verbose);

	if (status) {
		printf ("HMAC-SHA256 hash error\n");
		return -1;
	}

	return SHA256_HASH_SIZE;
}


/*----------------------------------------------------------------------
 * Name    : OutputHashFile
 * Purpose :
 * Input   :
 * Output  : Writes the HMAC SHA256 hash to the output file
 * 			 Returns the size of the signature if successfull
 *---------------------------------------------------------------------*/
int OutputHashFile(uint8_t *data, uint32_t len, char *filename, int flag, int verbose)
{
	uint8_t digest[SHA256_HASH_SIZE];
	int status, namelen, outputlen=0;
	char hashfile[256];
	uint8_t* output=NULL;

#ifdef SUN_OS
	RAND_seed((const void *)digest, 128);
#endif
	status = HmacSha256Hash (data, len, digest, default_hmac_fixed, verbose);
	if (status) {
		printf ("HMAC-SHA256 hash error\n");
		return status;
	}
	output = digest;
	outputlen= SHA256_HASH_SIZE;

	if (verbose)
	  DataDump ("Data to sign", output, outputlen);

	namelen = strlen(filename);
	if (namelen > MAX_FILE_NAME - 6)
		return SBERROR_FILENAME_LEN;
	strcpy(hashfile, filename);
	strcat(hashfile+namelen, ".hash");

	if (verbose)
	  printf ("Generating ABI hash file %s\n", hashfile);

	status = DataWrite(hashfile, (char *)output, outputlen);
	if (status)
		return status;
	return 0;
}

/*----------------------------------------------------------------------
 * Name    : ReadRSASigFile
 * Purpose :
 * Input   :
 * Output  :
 *---------------------------------------------------------------------*/
int ReadRSASigFile(char *filename, uint8_t *sig, uint32_t *length)
{
	*length = RSA_SIGNATURE_SIZE * 2;

	return DataRead(filename, sig, length);
}


/*----------------------------------------------------------------------
 * Name    : VerifyRSASignature
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int	VerifyRSASignature(uint8_t *data, uint32_t len, uint8_t *sig, char *filename, int verbose)
{
#if 0
	RSA *rsa_key = NULL;
	uint8_t digest[SHA256_HASH_SIZE];
	uint8_t buf[1024];
	BIO *in=NULL;
	uint8_t pkcs1Pad[PKCS1_PAD_SIZE];
	int status = STATUS_OK, siglen, msglen;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening RSA key file %s\n", filename);
		return SBERROR_DATA_FILE;
	}
	/* Read the file with the RSA public key */
	rsa_key=PEM_read_bio_RSA_PUBKEY(in, NULL, NULL, NULL);
	if (rsa_key == NULL) {
		printf("Unable to load RSA public key\n");
		return SBERROR_RSA_KEY_ERROR;
	}

	if (verbose)
	  PrintRSAKey (rsa_key, DUMP_PUBLIC);

	status = GenerateDataHash(data, len, digest);
	if (status)
		return status;
	GeneratePKCS1Padding(pkcs1Pad);
	memcpy (pkcs1Pad+PAD_OFFSET, digest, SHA256_HASH_SIZE);

	if (verbose)
	  DataDump ("Data to sign", pkcs1Pad, PKCS1_PAD_SIZE);

	siglen = RSA_size(rsa_key);

	if (verbose)
	  DataDump("RSA Signature", sig, siglen);

	/* Check the signature */
	msglen = RSA_public_decrypt(siglen, sig, buf, rsa_key, RSA_NO_PADDING);

	if (msglen != PKCS1_PAD_SIZE)
		printf ("\nRSA_public_decrypt message length error %d\n", msglen);
	else if (memcmp(buf, pkcs1Pad, PKCS1_PAD_SIZE)) {
		puts("\nRSA signature mismatch!!");
		puts("This will be reported to the proper authorities");
		
		if (verbose) {
		  DataDump ("Calculated padded data", pkcs1Pad, PKCS1_PAD_SIZE);
		  DataDump ("Decrypted padded data", buf, PKCS1_PAD_SIZE);
		}
	} else {
	  if (verbose)
		printf("\nRSA signature is verified using public key %s!!\n", filename);
	}

	RSA_free(rsa_key);
	BIO_free(in);
	return status;
#endif
	return 0;
}

int	TestDsaSign(char *keyfile, char* sigfile, char *datafile)
{
	uint8_t der_sig[MAX_DER_SIGNATURE_SIZE];
	uint8_t *q = der_sig;
	DSA *dsa_key = NULL;
	DSA_SIG *dsig = NULL;
	BIO *pubin=NULL;
	int status = STATUS_OK, siglen=BUFF_SIZE, datalen=BUFF_SIZE;
	uint8_t buf[BUFF_SIZE];
	uint8_t sig[40];
	uint8_t digest[SHA256_HASH_SIZE];
	const uint8_t *p = buf;

	pubin=BIO_new(BIO_s_file());
	if (pubin == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(pubin,keyfile) <= 0) {
		printf("Error opening DSA key file %s\n", keyfile);
		return SBERROR_DATA_FILE;
	}

	/* Read the file with the DSA public key */
	dsa_key=PEM_read_bio_DSA_PUBKEY(pubin, NULL, NULL, NULL);
	if (dsa_key == NULL) {
		printf("Unable to load DSA public key\n");
		return SBERROR_DSA_KEY_ERROR;
	}
#ifdef DEBUG_DSA
	PrintDSAKey (dsa_key, DUMP_PUBLIC|DUMP_PARAMETERS);
#endif

	/* Read the file with the DSA signature*/
	DataRead(sigfile, buf, &siglen);

#ifdef DEBUG_DER_DSA
	DataDump ("DSA DER Signature", buf, siglen);
#endif
	if (!d2i_DSA_SIG (&dsig, &p, (long) siglen))
			printf ("d2i_DSA_SIG error\n");

	/* Put the raw signature in DSA_SIG format */
	BN_bn2bin(dsig->r, sig);
#ifdef DEBUG_SIGNATURE
	DataDump("DSA sig r:", sig, BN_num_bytes(dsig->r));
#endif
	BN_bn2bin(dsig->s, sig+20);
#ifdef DEBUG_SIGNATURE
	DataDump("DSA sig s:", sig+20, BN_num_bytes(dsig->s));
#endif
#if 1
	status = DataWrite("sig.out", (char *)sig, 40);
#endif
	status = DataRead(datafile, buf, &datalen);
	if (status)
		return status;
	printf("Read %d bytes from data file %s\n", datalen, datafile);

	status = Sha256Hash (buf, datalen, digest, 1);
	if (status) {
		printf ("SHA256 hash error\n");
		return status;
	}
#ifdef DEBUG_SIGNATURE
	DataDump("Data hash", buf, SHA256_HASH_SIZE);
#endif

	siglen = i2d_DSA_SIG (dsig, &q);
	printf("DSA signature length %d\n", siglen);
	status = DSA_verify(EVP_PKEY_DSA, digest, SHA256_HASH_SIZE,
								der_sig, siglen, dsa_key);
	switch (status) {
	case 0:
		puts ("\nDSA signature failure!!");
		break;
	case 1:
		printf("\nDSA signature is verified using public key %s!!\n", keyfile);
		break;
	default:
		printf ("DSA_verify error %d %lx\n", status, ERR_get_error());
	}

	DSA_SIG_free(dsig);
	DSA_free(dsa_key);
	BIO_free(pubin);

	return status;
}
int	TestRsaSign(char *keyfile, char* sigfile, char *datafile)
{
#if 0
	RSA *rsa_key = NULL;
	BIO *pubin=NULL;
	int status = STATUS_OK, siglen=BUFF_SIZE, datalen=BUFF_SIZE, msglen;
	uint8_t pkcs1Pad[PKCS1_PAD_SIZE];
	uint8_t buf[1000];
	uint8_t sig[256];
	uint8_t data[1000];

	pubin=BIO_new(BIO_s_file());
	if (pubin == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(pubin,keyfile) <= 0) {
		printf("Error opening RSA key file %s\n", keyfile);
		return SBERROR_DATA_FILE;
	}

	/* Read the file with the RSA public key */
	rsa_key=PEM_read_bio_RSA_PUBKEY(pubin, NULL, NULL, NULL);
	if (rsa_key == NULL) {
		printf("Unable to load RSA public key\n");
		return SBERROR_RSA_KEY_ERROR;
	}
#ifdef DEBUG_RSA
	PrintRSAKey (rsa_key, DUMP_PUBLIC);
#endif

	/* Read the file with the RSA signature*/
	DataRead(sigfile, sig, &siglen);
	if (siglen != RSA_size(rsa_key)) {
		printf ("RSA signature length error read %d, expected %d\n",
						siglen, RSA_size(rsa_key));
		return -1;
	}
#ifdef DEBUG_RSA
	DataDump ("RSA Signature", sig, siglen);
#endif

	/* Read the original data */
	status = DataRead(datafile, data, &datalen);
	if (status)
		return status;
	printf("Read %d bytes from data file %s\n", datalen, datafile);

	GeneratePKCS1Padding(pkcs1Pad);
	status = Sha256Hash (data, datalen, pkcs1Pad+PAD_OFFSET);
	if (status) {
		printf ("SHA256 hash error\n");
		return status;
	}
#ifdef DEBUG_SIGNATURE
	DataDump ("RSA data to sign", pkcs1Pad, PKCS1_PAD_SIZE);
#endif

	/* Check the signature */
	msglen = RSA_public_decrypt(siglen, sig, buf, rsa_key, RSA_NO_PADDING);

	if (msglen <= 0) {
		printf ("RSA_public_decrypt returned %d\n", msglen);
		return -1;
	}

	if (memcmp(buf, pkcs1Pad, msglen)) {
		puts("\nRSA signature mismatch!!");
		puts("This will be reported to the proper authorities");
#ifdef DEBUG_SIGNATURE
		DataDump ("Calculated padded data", pkcs1Pad, PKCS1_PAD_SIZE);
		DataDump ("Decrypted padded data", buf, msglen);
#endif
	}
	else
		printf("\nRSA signature is verified using public key %s!!\n", keyfile);

	RSA_free(rsa_key);
	BIO_free(pubin);
	return status;
#endif
	return 0;
}

/*----------------------------------------------------------------------
 * Name    : WriteRSAPubkey
 * Purpose :
 * Input   :
 * Output  : Write the value of the DSA public key
 * Note    : privflag indicates that the input is a private key file
 *---------------------------------------------------------------------*/
int WriteRSAPubkey(char *filename, uint8_t *dest, uint16_t *len, int privflag, int verbose)
{
	uint8_t buffer[BUFF_SIZE];
	uint8_t digest[BUFF_SIZE];
	int status = STATUS_OK;
	RSA *rsa_key = NULL;
	BIO *in=NULL;

	in=BIO_new(BIO_s_file());
	if (in == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(in,filename) <= 0) {
		printf("Error opening RSA public key file %s\n", filename);
		return SBERROR_DATA_FILE;
	}

	if (privflag)
		/* Read the file with the RSA private key */
		rsa_key=PEM_read_bio_RSAPrivateKey(in, NULL, NULL, NULL);
	else
		/* Read the file with the RSA public key */
		rsa_key=PEM_read_bio_RSA_PUBKEY(in, NULL, NULL, NULL);

	if (rsa_key == NULL) {
		printf("Unable to load RSA public key\n");
		return SBERROR_RSA_KEY_ERROR;
	}
	*len = BN_num_bytes(rsa_key->n);
	BN_bn2bin(rsa_key->n, buffer);
	memcpy(dest, buffer, *len);

	if (verbose)
	  DataDump("RSA PubKey:", buffer, *len);

	/* Calculate Dauth */
	HmacSha256Hash (buffer, *len, digest, default_hmac_fixed, verbose);
	status = DataWrite("dauth.out", (char*)digest, SHA256_HASH_SIZE);

	RSA_free(rsa_key);
	BIO_free(in);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : WriteDauth
 * Purpose :
 * Input   :
 * Output  : Write the value of the dauth hash
 *---------------------------------------------------------------------*/
int WriteDauth(char *filename, int verbose)
{
	uint8_t hash[SHA256_HASH_SIZE];
	uint8_t buffer[BUFF_SIZE];
	int status = STATUS_OK, len;
	DSA *dsa_key = NULL;
	BIO *pubin=NULL;

	pubin=BIO_new(BIO_s_file());
	if (pubin == NULL)
		printf("Error BIO file \n");

	if (BIO_read_filename(pubin,filename) <= 0) {
		printf("Error opening DSA key file %s\n", filename);
		return SBERROR_DATA_FILE;
	}

	/* Read the file with the DSA public key */
	dsa_key=PEM_read_bio_DSA_PUBKEY(pubin, NULL, NULL, NULL);
	if (dsa_key == NULL) {
		printf("Unable to load DSA public key\n");
		return SBERROR_DSA_KEY_ERROR;
	}
	len = BN_num_bytes(dsa_key->pub_key);
	BN_bn2bin(dsa_key->pub_key, buffer);

	if (verbose)
	  DataDump("DSA PubKey:", buffer, len);

	/* Calculate Dauth */
	HmacSha256Hash (buffer, len, hash, default_hmac_fixed, verbose);

	status = DataWrite("dauth.out", (char*)hash, SHA256_HASH_SIZE);
	return status;
}

