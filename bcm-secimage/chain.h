/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: chain.h
 */

#ifndef _CHAIN_H_
#define _CHAIN_H_

#define RSA_SIZE_SIGNATURE_DEFAULT	512
#define RSA_SIZE_MODULUS_DEFAULT	RSA_SIZE_SIGNATURE_DEFAULT


#include "secimage.h"
#include "crypto.h"

typedef struct DSAsigChain
{
	DSA *dsa_key;
	uint8_t signature[DSA_SIGNATURE_SIZE];
	struct DSAsigChain *next;
} DSA_CHAIN;

int CreateDSAChainTrust(int ac, char **av);
int WriteDSAChainFile(DSA_CHAIN *head, char *filename);
int FreeDSAChain(DSA_CHAIN *head);
int CreateDSAChainSignatures(DSA_CHAIN *head, int headpub);

//typedef struct _chain_block{
//	RSA *rsa_key;
//        uint32_t  size; /* Bit size of the next parameter, nextKey, i.e public key modulus */
//        uint32_t  signature[RSA_SIZE_SIGNATURE_DEFAULT>>2]; /* signature of the next 3 structure members */
//        //uint32_t  nextKey[RSA_SIZE_MODULUS_DEFAULT>>2];  /* modulus for RSA signature verification */
//        uint32_t  RevisionInfo; /* [23:16] ROM revision number; [7:0] Customer revision */
//        uint32_t  CustomerID;   /* this will show if it's broadcom key */
//        uint32_t  BBLStatus;
//	uint8_t *MontCtx;
//	uint32_t MontCtxSize;
//	struct _chain_block *next;
//} CHAIN_BLOCK;

typedef struct chain_block
{
	IPROC_SBI_PUB_KEYTYPE	KeyType;						/* type of the key in the entry */
	uint16_t				ChainBlocklength;				/* size of the chain block */
	RSA*					rsa_key;
	uint32_t				size;							/* Bit size of the next parameter, nextKey, i.e. public key modulus */
	uint8_t*				MontCtx;
	uint32_t				MontCtxSize;
	uint32_t				CustomerID;						/* Customer ID */
	uint16_t				COTBindingID;					/* Chain of Trust Binding ID */
	uint16_t				CustomerRevisionID;				/* Customer Revision ID */
	uint32_t				BBLStatus;						
	uint16_t				SignatureVerificationConfig;
	uint16_t				SignatureLen;
	uint32_t				signature[RSA_4096_KEY_BYTES];	/* maximum signature size buffer */
	struct chain_block*		next;
} CHAIN_BLOCK;

typedef struct chain_block_ec
{
	IPROC_SBI_PUB_KEYTYPE	KeyType;						/* type of the key in the entry */
	uint16_t				ChainBlocklength;				/* size of the chain block */
	EC_KEY*					ec_key;
	uint32_t				size;							/* Bit size of the next parameter, nextKey, i.e. public key modulus */
	uint32_t				CustomerID;						/* Customer ID */
	uint16_t				COTBindingID;					/* Chain of Trust Binding ID */
	uint16_t				CustomerRevisionID;				/* Customer Revision ID */
	uint32_t				BBLStatus;						
	uint16_t				SignatureVerificationConfig;
	uint16_t				SignatureLen;
	uint32_t				signature[EC_P512_KEY_BYTES];	/* maximum signature size buffer */
	struct chain_block_ec*		next;
} CHAIN_BLOCK_EC;

int CreateRSAChainTrust(int ac, char **av, int verbose);
int CreateRSAChainHash(int ac, char **av, int verbose);
int WriteRSAChainFile(CHAIN_BLOCK *head, char *filename, int verbose);
int FreeRSAChain(CHAIN_BLOCK *head);
int CreateRSAChainSignatures(CHAIN_BLOCK *head, int headpub, int use_sha256, int verbose);

int CreateECChainTrust(int ac, char **av, int verbose);
int CreateECChainHash(int ac, char **av, int verbose);
int ChainAddMontCont( RSA *RsaKey, uint8_t *Output, uint32_t *MontContLength, int verbose);
int CreateECDSAChainSignatures(CHAIN_BLOCK_EC *head, int headpub, int use_sha256, int verbose);
int WriteECChainFile(CHAIN_BLOCK_EC *head, char *filename, int verbose);
int FreeECChain(CHAIN_BLOCK_EC *head);

#endif /* _CHAIN_H_ */
