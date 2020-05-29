/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: chain.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chain.h"
#include "sbi.h"


#include "q_lip.h"
#include "q_lip_aux.h"
#include "q_pka_hw.h"
#include "q_elliptic.h"
#include "q_rsa.h"
#include "q_rsa_4k.h"
#include "q_dsa.h"
#include "q_dh.h"
#include "q_lip_utils.h"


int my_BN_MONT_CTX_set(BN_MONT_CTX *mont, const BIGNUM *mod, BN_CTX *ctx);

/*----------------------------------------------------------------------
 * Name    : ChainTrustUsage
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int ChainTrustHandler(int ac, char **av)
{
	if (ac > 0) {
		if (!strcmp(*av, "-out"))
		  return CreateChainTrust (ac, av);
		if (!strcmp(*av, "-in"))
			return VerifyChainTrust (ac, av);
	}
	ChainTrustUsage();
	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : ChainTrustUsage
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateChainTrust(int ac, char **av)
{
	char *arg, **av0;
	int ecdsa= 0, rsa=0, rsanosign=0, ecdsanosign=0, ac0, verbose = 0;

	if (ac <= 0)
		return ChainTrustUsage();

	ac0 = ac;
	av0 = av;
	/* skip the output file for now */
	ac--; av++;
	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-ecdsa"))
			ecdsa = 1;
		if (!strcmp(arg, "-rsa"))
			rsa = 1;
		if (!strcmp(arg, "-rsanosign"))
			rsanosign = 1;
		if (!strcmp(arg, "-ecdsanosign"))
			ecdsanosign = 1;
		if (!strcmp(arg, "-v"))
			verbose = 1;
		ac--; av++;
	}

	if (rsa)
	  return CreateRSAChainTrust(ac0, av0, verbose);
	else if(ecdsa)
	  return CreateECChainTrust(ac0, av0, verbose);

	if(rsanosign)
	  return CreateRSAChainHash(ac0, av0, verbose);
	else if(ecdsanosign)
	  return CreateECChainHash(ac0, av0, verbose);

	printf ("Must specify ECDSA or RSA signatures\n");
	return ChainTrustUsage();
}


/*----------------------------------------------------------------------
 * Name    : CreateRSAChainTrust
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateRSAChainTrust(int ac, char **av, int verbose)
{
	CHAIN_BLOCK *head = NULL, *node, *newnode;
	char *outfile = NULL, *arg;
	int status = STATUS_OK, BBLStatus= SBI_BBL_STATUS_DONT_CARE ;
	int headpub = 0, tmp = 0, pubin =0, use_sha256 = 1, use_ecdsa = 0, RetVal, mont_ctx = 0;
	uint32_t length;
	uint32_t MontCtxLength;
	

	newnode = node = head;
	if (ac <= 0)
		return ChainTrustUsage();

	while (ac) {
		mont_ctx = 0;
		arg = *av;
		if (!strcmp(arg, "-out")) {
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing file name for -out\n");
			  return -1;
			}

			outfile = *av;
			if (verbose)
			  printf ("RSA chain output file %s\n", outfile);
		}
		else if (!strcmp(arg, "-ecdsa")) {
			use_ecdsa = 1;
		}
		else if (!strcmp(arg, "-sha256")) {
			use_sha256 = 1;
		}
		else if (!strcmp(arg, "-sig")) {
			--ac, ++av;

			newnode = (CHAIN_BLOCK *)calloc (sizeof(CHAIN_BLOCK), 1);
			if (!newnode) {
				printf("Error allocating RSA chain link\n");
				break;
			}

			status = ReadRSASigFile(*av, (uint8_t *)newnode->signature, &length);
			if (status)
				break;
			newnode->size = length * 8;
			newnode->SignatureLen = length;
			/* Indicate that the head is a real public key so
			 * the signature is provided instead of calculated */
			headpub = 1;
			if (head == NULL)
				head = node = newnode;
			else {
				newnode->next = head;
				head = newnode;
				newnode = NULL;
			}
		}
		else if (!strcmp(arg, "-key")) {
			--ac, ++av;

			newnode = (CHAIN_BLOCK *)calloc (sizeof(CHAIN_BLOCK), 1);
			if (!newnode) {
				printf("Error allocating RSA chain link\n");
				break;
			}

			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-public")) {
				pubin = 1;
				--ac, ++av;
			}

			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-mont")) {
				mont_ctx = 1;
				--ac, ++av;
			}
            
			if (*av == NULL || **av == '-') {
			  printf("\nMissing key file name for -key\n");
			  return -1;
			}

			if(pubin) {
			  newnode->rsa_key = GetRSAPublicKeyFromFile(*av, verbose);
			} 
			else {
			  newnode->rsa_key = GetRSAPrivateKeyFromFile(*av, verbose);
			}

			if (!newnode->rsa_key) {
				printf("Error getting RSA key from %s\n", *av);
				return -1;
			}
			else
			{
			  if (verbose)
				printf("\r\n Adding key %s to the COT ", *av );
			}

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing customer ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->CustomerID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing product ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->COTBindingID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing revision ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->CustomerRevisionID = tmp;

			if((use_ecdsa == 0) && (use_sha256 == 1))
				newnode->SignatureVerificationConfig = IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5 | IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256; // 0x0011

			if (verbose)
			  printf("CustomerID = 0x%x, COTBindingID = 0x%x, CustomerRevisionID = 0x%x\n", newnode->CustomerID, newnode->COTBindingID, newnode->CustomerRevisionID);

			if(ac-1) {
			   --ac; ++av;

			   arg = *av;
			   if( !strcmp(arg,"-BBLStatus"))
			   {
				   --ac; ++av;
                   sscanf(*av, "%d", &tmp ) ;
      			   printf("BBLStatus = %d\n",tmp);
                   switch (tmp) {
                      case 0:
                      case 1:
                      case 2:
                      case 3:
                      case 4:
                         {
                            BBLStatus = tmp;
                            break;
                         }
                  }
			      newnode->BBLStatus = BBLStatus;
			   }
			   else {
				  ++ac; --av;
			   }
			}
			newnode->size = RSA_size( newnode->rsa_key )*8 ;

			if(newnode->size == 2048)
			{
				newnode->KeyType = IPROC_SBI_RSA2048;
				newnode->ChainBlocklength = sizeof(uint32_t)*4 + RSA_2048_KEY_BYTES*2;
			}
			else if(newnode->size == 4096)
			{
				newnode->KeyType = IPROC_SBI_RSA4096;
				newnode->ChainBlocklength = sizeof(uint32_t)*4 + RSA_4096_KEY_BYTES*2 + RSA_4096_MONT_CTX_BYTES;
			}
			
			if(use_ecdsa == 0)
				newnode->SignatureLen = RSA_size(newnode->rsa_key);

			newnode->MontCtxSize = 0;
			if( newnode->size == 4096 )
			{
			  if (verbose)
			    printf("\r\n CHAIN_DEBUG ..Adding Mont Ctx ..\r\n" );

				newnode->MontCtx = (uint8_t *)calloc( 4096 , 1) ;
				if( newnode->MontCtx == NULL )
				{
					printf("\r\n ERROR : CALLOC Failed \r\n" );
					exit(0);
				}
				RetVal = ChainAddMontCont(newnode->rsa_key, newnode->MontCtx, &MontCtxLength, verbose ) ;

				if (verbose)
				  printf("\r\n CHAIN.OUT RetVal is %d ", RetVal);

				if ( RetVal == STATUS_OK )
				{
					newnode->MontCtxSize = MontCtxLength ;	
					if (verbose)
					  DataDump("\r\nMONT CTX DUMP .... \r\n", newnode->MontCtx,  MontCtxLength) ;
				}
				else
				{
					newnode->MontCtxSize =  0 ;
					printf("\r\n ERROR ........ ChainAddMontCont() returned %d ", RetVal );
					exit(0);
				}
			}

			if (verbose)
			  printf("\r\n While Building COT newnode->size is %d " , newnode->size );

			if (head == NULL)
				head = node = newnode;
			else {
				node->next = newnode;
				node = node->next;
				newnode = NULL;
			}
		}
		--ac, ++av;
	}
	if (!status)
	  status = CreateRSAChainSignatures(head, headpub,use_sha256, verbose);
	if (!status)
	  status = WriteRSAChainFile(head, outfile, verbose);
	if (!status)
		status = FreeRSAChain(head);
	return status;
}

int EC_Size(EC_KEY* ek)
{
	size_t	buf_len=0;
	BIGNUM  *pub_key=NULL;
	BN_CTX  *ctx=NULL;
	const EC_GROUP *group;
	const EC_POINT *public_key;

	public_key = EC_KEY_get0_public_key(ek);
	group = EC_KEY_get0_group(ek);
	ctx = BN_CTX_new();
	pub_key = EC_POINT_point2bn(group, public_key,	EC_KEY_get_conv_form(ek), NULL, ctx);
	if(!pub_key)
		return 0;
	buf_len = (size_t)BN_num_bytes(pub_key);

	if (pub_key) 
		BN_free(pub_key);
	if (ctx)
		BN_CTX_free(ctx);

	return buf_len-1; // to remove the bit defining how the point representation
}

int EC_KEY2bin(EC_KEY* ek, unsigned char *to)
{
	size_t	buf_len=0;
	BIGNUM  *pub_key=NULL;
	BN_CTX  *ctx=NULL;
	const EC_GROUP *group;
	const EC_POINT *public_key;
    unsigned char *data = NULL;

	public_key = EC_KEY_get0_public_key(ek);
	group = EC_KEY_get0_group(ek);
	ctx = BN_CTX_new();
	pub_key = EC_POINT_point2bn(group, public_key,	EC_KEY_get_conv_form(ek), NULL, ctx);
	if(!pub_key)
		return 0;
	buf_len = (size_t)BN_num_bytes(pub_key);
    data = (unsigned char*)malloc(buf_len);
	BN_bn2bin(pub_key, data);
	memcpy(to, data+1, buf_len-1);
	
	if (pub_key) 
		BN_free(pub_key);
	if (ctx)
		BN_CTX_free(ctx);

	return buf_len-1; // to remove the bit defining how the point representation
}

/*----------------------------------------------------------------------
 * Name    : CreateECChainTrust
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateECChainTrust(int ac, char **av, int verbose)
{
	CHAIN_BLOCK_EC *head = NULL, *node, *newnode;
	char *outfile = NULL, *arg;
	int status = STATUS_OK, BBLStatus= SBI_BBL_STATUS_DONT_CARE ;
	int headpub = 0, tmp = 0, pubin =0, use_sha256 = 1, use_ecdsa = 0;
	uint32_t length;

	newnode = node = head;
	if (ac <= 0)
		return ChainTrustUsage();

	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-out")) {
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing file name for -out\n");
			  return -1;
			}

			outfile = *av;
			if (verbose)
			  printf ("EC chain output file %s\n", outfile);
		}
		else if (!strcmp(arg, "-sha256")) {
			use_sha256 = 1;
		}
		else if (!strcmp(arg, "-sig")) {
			--ac, ++av;

			newnode = (CHAIN_BLOCK_EC *)calloc (sizeof(CHAIN_BLOCK_EC), 1);
			if (!newnode) {
				printf("Error allocating EC chain link\n");
				break;
			}

			status = ReadRSASigFile(*av, (uint8_t *)newnode->signature, &length);
			if (status)
				break;
			newnode->size = length * 8;
			/* Indicate that the head is a real public key so
			 * the signature is provided instead of calculated */
			headpub = 1;
			if (head == NULL)
				head = node = newnode;
			else {
				newnode->next = head;
				head = newnode;
				newnode = NULL;
			}
		}
		else if (!strcmp(arg, "-key")) {
			--ac, ++av;

			newnode = (CHAIN_BLOCK_EC *)calloc (sizeof(CHAIN_BLOCK_EC), 1);
			if (!newnode) {
				printf("Error allocating RSA chain link\n");
				break;
			}

			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-public")) {
				pubin = 1;
				--ac, ++av;
			}
         
			if (*av == NULL || **av == '-') {
			  printf("\nMissing key file name for -key\n");
			  return -1;
			}

			if(pubin) {
			  newnode->ec_key = GetECPublicKeyFromFile(*av, verbose);
			} 
			else {
			  newnode->ec_key = GetECPrivateKeyFromFile(*av, verbose);
			}

			if (!newnode->ec_key) {
				printf("Error getting EC key from %s\n", *av);
				break;
			}
			else
			{
			  if (verbose)
				printf("\r\n Adding key %s to the COT ", *av );
			}

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing customer ID -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->CustomerID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing product ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->COTBindingID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing revision ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			newnode->CustomerRevisionID = tmp;

			if(use_sha256 == 1)
				newnode->SignatureVerificationConfig = IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256; // 0x0010

			if (verbose)
			  printf("CustomerID = 0x%x, COTBindingID = 0x%x, CustomerRevisionID = 0x%x\n", newnode->CustomerID, newnode->COTBindingID, newnode->CustomerRevisionID);

			if(ac-1) {
			   --ac; ++av;

			   arg = *av;
			   if( !strcmp(arg,"-BBLStatus"))
			   {
				   --ac; ++av;
                   sscanf(*av, "%d", &tmp ) ;
      			   printf("BBLStatus = %d\n",tmp);
                   switch (tmp) {
                      case 0:
                      case 1:
                      case 2:
                      case 3:
                      case 4:
                         {
                            BBLStatus = tmp;
                            break;
                         }
                  }
			      newnode->BBLStatus = BBLStatus;
			   }
			   else {
				  ++ac; --av;
			   }
			}
			newnode->size = EC_Size(newnode->ec_key);

			if(newnode->size == EC_P256_KEY_BYTES)
			{
				newnode->KeyType = IPROC_SBI_EC_P256;
				newnode->ChainBlocklength = sizeof(uint32_t)*4 + EC_P256_KEY_BYTES*2;
			}
			
			newnode->SignatureLen = EC_Size(newnode->ec_key);

			if (verbose)
			  printf("\r\n While Building COT newnode->size is %d " , newnode->size );

			if (head == NULL)
				head = node = newnode;
			else {
				node->next = newnode;
				node = node->next;
				newnode = NULL;
			}
		}
		--ac, ++av;
	}
	if (!status)
	  status = CreateECDSAChainSignatures(head, headpub,use_sha256, verbose);
	if (!status)
	  status = WriteECChainFile(head, outfile, verbose);
	if (!status)
		status = FreeECChain(head);
	return status;
}

//int getChainLength(CHAIN_BLOCK *head)
//{
//  CHAIN_BLOCK* cur = head;
//  int size = 0;
//
//  while (cur != NULL)
//  {
//    ++size;
//    cur = cur->next;
//  }
//
//  return size;
//}

/*----------------------------------------------------------------------
 * Name    : WriteRSAChainFile
 * Purpose : Write the file of RSA chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int WriteRSAChainFile(CHAIN_BLOCK *head, char *filename, int verbose)
{
	int status = STATUS_OK, size, depth = 0;
	CHAIN_BLOCK *node = head;
	RSA *rsa_key;
	uint8_t pubkey[RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t)];
	uint16_t pubkeyLen = 0;
	int keysize = 0;
	FILE *file;

	//uint16_t COTIdentifier = 0xcc; /* TODO: To change later */
	//uint16_t nCOTEntries = getChainLength(head) - 1; /* taking away the ROT entry from the length */

	if (verbose)
	  printf ("Creating RSA chain of trust to file: %s\n", filename);

	file = fopen (filename, "wb");
	if (file == NULL) 
	{
		printf ("Unable to open RSA chain file\n");
		return SBERROR_DATA_FILE;
	}

	//size = fwrite(&COTIdentifier, 1, sizeof(uint16_t), file); /* First write the COT identifier */
	//if( size != sizeof(uint16_t) )
	//{
	//	printf ("Unable to write to RSA chain: COTIdentifier\n");
	//	return SBERROR_DATA_READ;
	//}
	//else
	//{
	//	printf("\r\n CHAIN.OUT : Writing COTIdentifier. The COT IDENTIFIER is %d  \r\n", COTIdentifier);
	//}

	//size = fwrite(&nCOTEntries, 1, sizeof(uint16_t), file);
	//if( size != sizeof(uint16_t) )
	//{
	//	printf ("Unable to write to RSA chain: nCOTEntries\n");
	//	return SBERROR_DATA_READ;
	//}
	//else
	//{
	//	printf("\r\n CHAIN.OUT : Writing nCOTEntries. NUMBER OF COT ENTRIES is %d  \r\n", nCOTEntries);
	//}

	//pubkeyLen = BN_num_bytes(node->rsa_key->n) + 2*4;
	while (node && node->next)
	{
		size = fwrite(&node->next->KeyType, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: KeyType(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing KeyType(%d). KEY TYPE is 0x%08x \r\n", depth, node->next->KeyType);
		}

		size = fwrite(&node->next->ChainBlocklength, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: ChainBlocklength(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing ChainBlocklength(%d). CHAIN BLOCK LENGTH is %d \r\n", depth, node->next->ChainBlocklength);
		}

		rsa_key = node->next->rsa_key;
		keysize = BN_num_bytes(node->next->rsa_key->n);

		if( (keysize != RSA_2048_KEY_BYTES) && (keysize != RSA_4096_KEY_BYTES))
		{
			printf("Unsupported RSA KeySize = %d", keysize);
			return SBERROR_DATA_INPUT;
		}

		if (verbose)
		  printf("\r\n CHAIN.OUT : Writing public key --------------\r\n" ) ;

		BN_bn2bin(rsa_key->n, pubkey);
			
		size = fwrite (pubkey, 1, keysize, file);
		if (size != keysize) 
		{
			printf ("Unable to write public key (%d) to RSA chain file\n", depth);
			return SBERROR_DATA_READ;
		}

		if(keysize == RSA_4096_KEY_BYTES)
		{
			// Write the Montgomery Context following the key
			size = fwrite(node->next->MontCtx, 1, node->next->MontCtxSize, file);
			if( size != node->next->MontCtxSize )
			{
				printf ("Unable to write to RSA chain: MontCtx (%d)\n", depth);
				return SBERROR_DATA_READ;
			}
			else
			{
			  if (verbose)
				printf("\r\n CHAIN.OUT : Writing MontCtx (%d): 0x%08x bytes \r\n", depth, node->next->MontCtxSize);
			}
		}

		size = fwrite(&node->next->CustomerID, 1, sizeof(uint32_t), file);
		if( size != sizeof(uint32_t) )
		{
			printf ("Unable to write to RSA chain: CustomerID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing CustomerID(%d). CUSTOMER ID is 0x%08x \r\n", depth, node->next->CustomerID);
		}

		size = fwrite(&node->next->COTBindingID, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: COTBindingID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing COTBindingID(%d). COT BINDING ID is 0x%04x \r\n", depth, node->next->COTBindingID);
		}

		size = fwrite(&node->next->CustomerRevisionID, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: CustomerRevisionID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing CustomerRevisionID(%d). CUSTOMER REVISION ID is 0x%04x \r\n", depth, node->next->CustomerRevisionID);
		}

		size = fwrite(&node->next->SignatureVerificationConfig, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: SignatureVerificationConfig(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing SignatureVerificationConfig(%d). Signature Verification Config is 0x%04x \r\n", depth, node->next->SignatureVerificationConfig);
		}

		size = fwrite(&node->next->SignatureLen, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to RSA chain: SignatureLen(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing SignatureLen(%d). Signature Len is 0x%04x \r\n", depth, node->next->SignatureLen);
		}

		size = fwrite (node->signature, 1,node->SignatureLen, file);

		if (verbose) {
		  printf("\r\nSIZES Signature size written is %d\n",size);
		  printf("\r\n CHAIN.OUT : Written signature of %d Bytes \r\n", size ) ;
		}

		if (size != node->SignatureLen) 
		{
			printf ("Unable to write to RSA chain file: Signature (%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		depth++;
		node = node->next;
	}
	
	fclose(file);
	printf("\nWritten a chain of %d RSA public keys, %d signatures to file %s\n\n",
					depth+1, depth, filename);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : WriteECChainFile
 * Purpose : Write the file of EC chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int WriteECChainFile(CHAIN_BLOCK_EC *head, char *filename, int verbose)
{
	int status = STATUS_OK, size, depth = 0;
	CHAIN_BLOCK_EC *node = head;
	EC_KEY *ec_key;
	uint8_t pubkey[EC_P256_KEY_BYTES + 3*sizeof(uint32_t)];
	uint16_t pubkeyLen = 0;
	int keysize = 0;
	FILE *file;

	//uint16_t COTIdentifier = 0xcc; /* TODO: To change later */
	//uint16_t nCOTEntries = getChainLength(head) - 1; /* taking away the ROT entry from the length */

	if (verbose)
	  printf ("Creating EC chain of trust to file: %s\n", filename);

	file = fopen (filename, "wb");
	if (file == NULL) 
	{
		printf ("Unable to open EC chain file\n");
		return SBERROR_DATA_FILE;
	}

	//size = fwrite(&COTIdentifier, 1, sizeof(uint16_t), file); /* First write the COT identifier */
	//if( size != sizeof(uint16_t) )
	//{
	//	printf ("Unable to write to RSA chain: COTIdentifier\n");
	//	return SBERROR_DATA_READ;
	//}
	//else
	//{
	//	printf("\r\n CHAIN.OUT : Writing COTIdentifier. The COT IDENTIFIER is %d  \r\n", COTIdentifier);
	//}

	//size = fwrite(&nCOTEntries, 1, sizeof(uint16_t), file);
	//if( size != sizeof(uint16_t) )
	//{
	//	printf ("Unable to write to RSA chain: nCOTEntries\n");
	//	return SBERROR_DATA_READ;
	//}
	//else
	//{
	//	printf("\r\n CHAIN.OUT : Writing nCOTEntries. NUMBER OF COT ENTRIES is %d  \r\n", nCOTEntries);
	//}

	//pubkeyLen = BN_num_bytes(node->rsa_key->n) + 2*4;
	while (node && node->next)
	{
		size = fwrite(&node->next->KeyType, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: KeyType(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing KeyType(%d). KEY TYPE is 0x%08x \r\n", depth, node->next->KeyType);
		}

		size = fwrite(&node->next->ChainBlocklength, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: ChainBlocklength(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing ChainBlocklength(%d). CHAIN BLOCK LENGTH is %d \r\n", depth, node->next->ChainBlocklength);
		}

		ec_key = node->next->ec_key;
		keysize = EC_Size(node->next->ec_key);

		if( (keysize != EC_P256_KEY_BYTES))
		{
			printf("Unsupported EC KeySize = %d", keysize);
			return SBERROR_DATA_INPUT;
		}

		if (verbose)
		  printf("\r\n CHAIN.OUT : Writing public key --------------\r\n" ) ;

		EC_KEY2bin(ec_key, pubkey);
			
		size = fwrite (pubkey, 1, keysize, file);
		if (size != keysize) 
		{
			printf ("Unable to write public key (%d) to EC chain file\n", depth);
			return SBERROR_DATA_READ;
		}

		size = fwrite(&node->next->CustomerID, 1, sizeof(uint32_t), file);
		if( size != sizeof(uint32_t) )
		{
			printf ("Unable to write to EC chain: CustomerID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing CustomerID(%d). CUSTOMER ID is 0x%08x \r\n", depth, node->next->CustomerID);
		}

		size = fwrite(&node->next->COTBindingID, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: COTBindingID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing COTBindingID(%d). COT BINDING ID is 0x%04x \r\n", depth, node->next->COTBindingID);
		}

		size = fwrite(&node->next->CustomerRevisionID, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: CustomerRevisionID(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing CustomerRevisionID(%d). CUSTOMER REVISION ID is 0x%04x \r\n", depth, node->next->CustomerRevisionID);
		}

		size = fwrite(&node->next->SignatureVerificationConfig, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: SignatureVerificationConfig(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing SignatureVerificationConfig(%d). Signature Verification Config is 0x%04x \r\n", depth, node->next->SignatureVerificationConfig);
		}

		size = fwrite(&node->next->SignatureLen, 1, sizeof(uint16_t), file);
		if( size != sizeof(uint16_t) )
		{
			printf ("Unable to write to EC chain: SignatureLen(%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		else
		{
		  if (verbose)
			printf("\r\n CHAIN.OUT : Writing SignatureLen(%d). Signature Len is 0x%04x \r\n", depth, node->next->SignatureLen);
		}

		size = fwrite (node->signature, 1,node->SignatureLen, file);

		if (verbose) {
		  printf("\r\nSIZES Signature size written is %d\n",size);
		  printf("\r\n CHAIN.OUT : Written signature of %d Bytes \r\n", size ) ;
		}

		if (size != node->SignatureLen) 
		{
			printf ("Unable to write to EC chain file: Signature (%d)\n", depth);
			return SBERROR_DATA_READ;
		}
		depth++;
		node = node->next;
	}
	
	fclose(file);
	printf("\nWritten a chain of %d EC public keys, %d signatures to file %s\n",
					depth+1, depth, filename);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : FreeRSAChain
 * Purpose : Write the file of RSA chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int FreeRSAChain(CHAIN_BLOCK *head)
{
	int status = STATUS_OK;
	CHAIN_BLOCK *node=head, *temp;

	while(node->next) {
		temp = node->next;
		RSA_free(node->rsa_key);
		free(node);
		node = temp;
	}

	return status;
}

/*----------------------------------------------------------------------
 * Name    : FreeECChain
 * Purpose : Write the file of RSA chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int FreeECChain(CHAIN_BLOCK_EC *head)
{
	int status = STATUS_OK;
	CHAIN_BLOCK_EC *node=head, *temp;

	while(node->next) {
		temp = node->next;
		EC_KEY_free(node->ec_key);
		free(node);
		node = temp;
	}

	return status;
}

/*----------------------------------------------------------------------
 * Name    : CreateRSAChainSignatures
 * Purpose : Creates the RSA signatures in the chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateRSAChainSignatures(CHAIN_BLOCK *head, int headpub, int use_sha256, int verbose)
{
	int status = STATUS_OK;
	CHAIN_BLOCK *node = head;
	RSA *rsa_key, *rsa_sign_key;
	uint8_t pubkey[4+512+1576+2*4+512];
	uint16_t pubkeyLen=0, pub_index=0, i=0;
	uint32_t keysize =0;

	if(headpub) {
		/* Skip this node as the signature was externally fed */
        node = head->next;
	}

	while (node->next) {
			pubkeyLen = 0;
			*(uint32_t *)&pubkey[0] = node->next->KeyType;
			pubkeyLen += 2;
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->ChainBlocklength;
			pubkeyLen += 2;
			keysize = BN_num_bytes(node->next->rsa_key->n);

			if (verbose)
			  printf("\r\n SPECIAL_DEBUG keysize is ....  %d ... " , keysize); 

			pub_index = pubkeyLen;
			rsa_key = node->next->rsa_key;
			BN_bn2bin(rsa_key->n, &pubkey[pub_index]);

			if (verbose)
			  printf("\r\n SPECIAL_DEBUG pub_index is ....  %d ... " , pub_index);

			pubkeyLen += keysize;

			if( node->next->MontCtxSize > 0 )
			{
				memcpy( &pubkey[pubkeyLen], node->next->MontCtx, (node->next->MontCtxSize) );
				pubkeyLen += (node->next->MontCtxSize);
				if (verbose)
				  printf("\r\n SPECIAL_DEBUG ... pubkeyLen is %d ", pubkeyLen); 
			}

			/* the key being signed is the next key, so RevisionInfo and CustomerID should also come from next key */
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->CustomerID;
			pubkeyLen += sizeof(uint32_t);
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->COTBindingID;
			pubkeyLen += sizeof(uint16_t);
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->CustomerRevisionID;
			pubkeyLen += sizeof(uint16_t);

			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->SignatureVerificationConfig;
			pubkeyLen += sizeof(uint16_t);

			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->SignatureLen;
			pubkeyLen += sizeof(uint16_t);


			// *(uint32_t *)(&pubkey[pubkeyLen]) = node->next->BBLStatus;

			if (verbose)
			  printf("\r\n While CreateRSAChainSignatures pubkeyLen is %d ", pubkeyLen );
			rsa_sign_key = node->rsa_key;
			
			/* if the head is a public key then signature is read not calculated */
			if (node != head || !headpub)
			{
			  node->SignatureLen = RsaSign(pubkey, pubkeyLen, (uint8_t *)node->signature, rsa_sign_key, use_sha256, verbose);
			}

		node = node->next;
	}


	return status;
}

/*----------------------------------------------------------------------
 * Name    : CreateECDSAChainSignatures
 * Purpose : Creates the ECDSA signatures in the chain of trust
 * Input   : Chain of private keys
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateECDSAChainSignatures(CHAIN_BLOCK_EC *head, int headpub, int use_sha256, int verbose)
{
	int status = STATUS_OK;
	CHAIN_BLOCK_EC *node = head;
	EC_KEY *ec_key, *ecdsa_sign_key;
	uint8_t pubkey[4+64+2*4+64];
	uint16_t pubkeyLen=0, pub_index=0, i=0;
	uint32_t keysize =0;

	if(headpub) {
		/* Skip this node as the signature was externally fed */
        node = head->next;
	}

	while (node->next) {
			pubkeyLen = 0;
			*(uint32_t *)&pubkey[0] = node->next->KeyType;
			pubkeyLen += 2;
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->ChainBlocklength;
			pubkeyLen += 2;
			keysize = EC_Size(node->next->ec_key);

			if (verbose)
			  printf("\r\n SPECIAL_DEBUG keysize is ....  %d ... " , keysize); 

			pub_index = pubkeyLen;
			ec_key = node->next->ec_key;
			EC_KEY2bin(ec_key, &pubkey[pub_index]);

			if (verbose)
			  printf("\r\n SPECIAL_DEBUG pub_index is ....  %d ... " , pub_index);

			pubkeyLen += keysize;


			/* the key being signed is the next key, so RevisionInfo and CustomerID should also come from next key */
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->CustomerID;
			pubkeyLen += sizeof(uint32_t);
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->COTBindingID;
			pubkeyLen += sizeof(uint16_t);
			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->CustomerRevisionID;
			pubkeyLen += sizeof(uint16_t);

			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->SignatureVerificationConfig;
			pubkeyLen += sizeof(uint16_t);

			*(uint32_t *)(&pubkey[pubkeyLen]) = node->next->SignatureLen;
			pubkeyLen += sizeof(uint16_t);


			// *(uint32_t *)(&pubkey[pubkeyLen]) = node->next->BBLStatus;

			if (verbose)
			  printf("\r\n While CreateRSAChainSignatures pubkeyLen is %d ", pubkeyLen );

			ecdsa_sign_key = node->ec_key;
			
			/* if the head is a public key then signature is read not calculated */
			if (node != head || !headpub)
			{
			  node->SignatureLen = ECDSASign(pubkey, pubkeyLen, (uint8_t *)node->signature, ecdsa_sign_key, verbose);
			}

		node = node->next;
	}


	return status;
}


/*----------------------------------------------------------------------
 * Name    : CreateRSAChainHash
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateRSAChainHash(int ac, char **av, int verbose)
{
	char *outfile = NULL, *arg;
	int status = STATUS_OK;
	int tmp = 0;
	RSA *rsa_key;
	uint8_t pubkey[4+4+256+3*4];
	uint16_t pubkeyLen;
	IPROC_SBI_PUB_KEYTYPE	KeyType;						/* type of the key in the entry */
	uint16_t				COTEntryLength;					/* size of the COT Entry */
	uint16_t				ProductID;						/* Product ID */
	uint16_t				CustomerRevisionID;				/* Customer Revision ID */
	uint32_t				CustomerID;						/* Customer ID */
	uint16_t				SignatureVerificationConfig;
	uint16_t				SignatureLen;
	int pubin = 0;
	int pub_index = 0;
	uint8_t digest[SHA256_HASH_SIZE];
	int namelen;
	char hashfile[256];
	
	uint32_t BBLStatus = SBI_BBL_STATUS_DONT_CARE ;
	if (ac <= 0)
		return ChainTrustUsage();

	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-out")) {
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing file name for -out\n");
			  return -1;
			}

			outfile = *av;
			if (verbose)
			  printf ("RSA chain Hash output file %s.hash\n", outfile);
		}
		else if (!strcmp(arg, "-key")) {
			--ac, ++av;
			
			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}
			
			arg = *av;
			if (!strcmp(arg, "-public")) {
				pubin = 1;
				--ac, ++av;
			}
            
			if (*av == NULL || **av == '-') {
			  printf("\nMissing key file name for -key\n");
			  return -1;
			}

			if(pubin) {
			  rsa_key = GetRSAPublicKeyFromFile(*av, verbose);
			} 
			else {
			  rsa_key = GetRSAPrivateKeyFromFile(*av, verbose);
			}
			if (!rsa_key) {
				printf("Error getting RSA key from %s\n", *av);
				return -1;
			}

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing customer ID value for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			CustomerID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing product ID value for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			ProductID = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing revision ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			CustomerRevisionID = tmp;

			if (verbose)
			  printf("CustomerID = 0x%x, ProductID = 0x%x, CustomerRevisionID = 0x%x\n", CustomerID, ProductID, CustomerRevisionID);

			if(ac - 1) {
			   --ac; ++av;
			   arg = *av;
			   if(!strcmp(arg, "-BBLStatus")){
			      --ac; ++av;
				  sscanf(*av, "%d", &tmp ) ;
				  printf("BBLStatus = %d\n", tmp);
				  switch (tmp)
				  {

				      case 0:
					  case 1:
					  case 2:
					  case 3:
					  case 4:
					  {
					  	 BBLStatus = tmp;
						 break;
					  }
				  }
			   }
			   else {
				  ++ac; --av;
			   }
			}
		}
		--ac, ++av;
	}

	pubkeyLen =  RSA_size(rsa_key);

	if(pubkeyLen == 256)
	{
		KeyType = IPROC_SBI_RSA2048;
		COTEntryLength = sizeof(uint32_t)*4 + 2*RSA_2048_KEY_BYTES;
	}
	else
	{
		// This option is only supported for RSA 2048 keys
		printf("\r\n ERROR ........ CreateRSAChainHash() invalid keySize: %d\n", pubkeyLen);
		exit(0);
	}

	SignatureVerificationConfig = IPROC_SBI_VERIFICATION_CONFIG_RSASSA_PKCS1_v1_5 | IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256; // 0x0011
	SignatureLen = RSA_size(rsa_key);

	pub_index = 0;
	*(uint32_t *)&pubkey[0] = KeyType;
	pub_index += 2;
	*(uint32_t *)(&pubkey[pub_index]) = COTEntryLength;
	pub_index += 2;
	BN_bn2bin(rsa_key->n, &pubkey[pub_index]);
	pub_index += pubkeyLen;
	*(uint32_t *)(&pubkey[pub_index]) = CustomerID;
	pub_index += sizeof(uint32_t);
	*(uint32_t *)(&pubkey[pub_index]) = ProductID;
	pub_index += sizeof(uint16_t);
	*(uint32_t *)(&pubkey[pub_index]) = CustomerRevisionID;
	pub_index += sizeof(uint16_t);
	*(uint32_t *)(&pubkey[pub_index]) = SignatureVerificationConfig;
	pub_index += sizeof(uint16_t);
	*(uint32_t *)(&pubkey[pub_index]) = SignatureLen;
	pub_index += sizeof(uint16_t);

	  printf ("\nWriting COTEntry to the file %s\n", outfile);
	if (verbose) {
	  DataDump ("COTEntry", pubkey, (COTEntryLength-RSA_2048_KEY_BYTES));
	}

	status = DataWrite(outfile, (char *)pubkey, (COTEntryLength-RSA_2048_KEY_BYTES));
	if (status)
		return status;
	return 0;
}


/*----------------------------------------------------------------------
 * Name    : CreateECChainHash
 * Purpose : Specifies the usage of ABI paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateECChainHash(int ac, char **av, int verbose)
{
	char *outfile = NULL, *arg;
	int status = STATUS_OK;
	int tmp = 0;
	RSA *rsa_key;
	uint32_t RevisionInfo;
	uint32_t CustomerID;
	uint8_t pubkey[4+4+256+3*4];
	uint16_t pubkeyLen;
	int pubin = 0;

	uint32_t BBLStatus = SBI_BBL_STATUS_DONT_CARE ;
	if (ac <= 0)
		return ChainTrustUsage();

	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-out")) {
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing file name for -out\n");
			  return -1;
			}
			
			outfile = *av;
			if (verbose)
			  printf ("RSA chain Hash output file %s.hash\n", outfile);
		}
		else if (!strcmp(arg, "-key")) {
			--ac, ++av;
			
			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}
			arg = *av;
			if (!strcmp(arg, "-public")) {
				pubin = 1;
				--ac, ++av;
			}
            
			if (*av == NULL || **av == '-') {
			  printf("\nMissing key file name for -key\n");
			  return -1;
			}

			if(pubin) {
			  rsa_key = GetRSAPublicKeyFromFile(*av, verbose);
			} 
			else {
			  rsa_key = GetRSAPrivateKeyFromFile(*av, verbose);
			}
			if (!rsa_key) {
				printf("Error getting RSA key form %s\n", *av);
			       return -1;
			}

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing revision ID for -key\n");
			  return -1;
			}	
			sscanf(*av, "%x", &tmp);
			RevisionInfo = tmp;

			--ac; ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing key customer ID for -key\n");
			  return -1;
			}
			sscanf(*av, "%x", &tmp);
			CustomerID = tmp;

			if (verbose)
			  printf("RevisionInfo = 0x%x, CustomerID = 0x%x\n", RevisionInfo, CustomerID);

			if(ac - 1) {
			   --ac; ++av;
			   arg = *av;
			   if(!strcmp(arg, "-BBLStatus")){
			      --ac; ++av;
				  sscanf(*av, "%d", &tmp ) ;
				  printf("BBLStatus = %d\n", tmp);
				  switch (tmp)
				  {

				      case 0:
					  case 1:
					  case 2:
					  case 3:
					  case 4:
					  {
					  	 BBLStatus = tmp;
						 break;
					  }
				  }
			   }
			   else {
				  ++ac; --av;
			   }
			}
		}
		--ac, ++av;
	}

	pubkeyLen = BN_num_bytes(rsa_key->n);
	pubkeyLen += 8+3*4;
	BN_bn2bin(rsa_key->n, &pubkey[8]);

	*(uint32_t *)(&pubkey[0])   = 0x0;
	*(uint32_t *)(&pubkey[4])   = 0x800;
	*(uint32_t *)(&pubkey[8+256])   = RevisionInfo;
	*(uint32_t *)(&pubkey[8+256+4]) = CustomerID;
	*(uint32_t *)(&pubkey[8+256+8]) = BBLStatus;
	
	printf ("\nWriting hash to the file %s\n", outfile);

 	OutputHashFile(pubkey, pubkeyLen, outfile, 0, verbose);

	return status;
}


/*----------------------------------------------------------------------
 * Name    : PrintRSAChainTrust
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int PrintRSAChainTrust(CHAIN_BLOCK *node)
{
	while (node) {
		PrintRSAKey (node->rsa_key, DUMP_PUBLIC);
		if (node->signature[0])
			DataDump ("RSA signature", node->signature, RSA_SIGNATURE_SIZE);
		node = node->next;
	}
	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : VerifyChainTrust
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int VerifyChainTrust(int ac, char **av)
{
	char *filename;

	filename = *av;
	--ac; ++av;
	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : ChainTrustUsage
 * Purpose : Specifies the usage of chain of trust paramaters
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int ChainTrustUsage(void)
{
	printf ("\nTo create a chain of trust:\n");
	printf ("secimage: chain -out outfile [-dsa] [-rsa] -key privkey [-key privkey ..]  -BBLStatus <0/1/2/3/4>\n");
	printf ("\nTo verify a chain of trust:\n");
	printf ("secimage: chain -in file [-v] [-dsa] [-rsa]\n\n");
	printf ("\nTo generate Hash of first key in chain of trust, for production key signing:\n");
	printf ("secimage: chain -out outfile [-rsanosign] [-key [-public] privkey/publickey]\n\n");
	return STATUS_OK;
}

int ChainAddMontCont( RSA *RsaKey, uint8_t *Output, uint32_t *MontContLength, int verbose )
{
	int RetVal = STATUS_OK;
	int Index = 0;
	uint32_t MontPreComputeLength;
	int status;

	q_lint_t n;
    q_mont_t mont;


    LOCAL_QLIP_CONTEXT_DEFINITION;



	if( RsaKey == NULL )
	{
		return -1;
	}
	
	if( Output == NULL )
	{
		return -1;
	}

	if( MontContLength == NULL )
	{
		return -1;
	}


        status  = q_init (&qlip_ctx, &n, (RsaKey->n->top)*2);
        status += q_init (&qlip_ctx, &mont.n, n.alloc);
        status += q_init (&qlip_ctx, &mont.np, (n.alloc+1));
        status += q_init (&qlip_ctx, &mont.rr, (n.alloc+1));
        if (status != STATUS_OK )
        {
                printf("\r\n q_init failed ......\r\n ");
                return( -1 );
        }
	
	if (verbose)
	  printf("\r\n RsaKey->n->top is %d \r\n", RsaKey->n->top );

        n.size = (RsaKey->n->top);

	if (verbose)
	  printf("\r\n The Value of n.size is %d ", n.size );

        n.alloc = (RsaKey->n->dmax);

	if (verbose)
	  printf("\r\n The Value of n.alloc is %d ", n.alloc );

        n.neg = RsaKey->n->neg;

	if (verbose)
	  printf("\r\n The Value of n.neg is %d ", n.neg);

        memcpy( n.limb, RsaKey->n->d, (RsaKey->n->top)*4 ) ;

	if (verbose)
	  DataDump("\r\n The value of n.limb is ..........", n.limb, (RsaKey->n->top)*4 );




        status = q_mont_init_sw (&qlip_ctx, &mont, &n);

        if( status != STATUS_OK )
        {
                printf("\r\n q_mont_init_sw failed ...... with ERROR CODE : %d \r\n ", status);
                return( -1 );

        }

	if (verbose)
	  printf("\r\n After q_mont_init_sw()  mont.n.size is %d,  mont.np.size is %d  , mont.rr.size is %d ", mont.n.size, mont.np.size, mont.rr.size);

        MontPreComputeLength =  sizeof(mont.n.size) + sizeof(mont.n.alloc) + sizeof(mont.n.neg) + mont.n.size*4 + \
                                sizeof(mont.np.size) + sizeof(mont.np.alloc) + sizeof(mont.np.neg) + mont.np.size*4 + \
                                sizeof(mont.rr.size) + sizeof(mont.rr.alloc) + sizeof(mont.rr.neg) + mont.rr.size*4 + \
                                sizeof(mont.br);

	if (verbose)
	  printf("\r\n The MontPreComputeLength value is %d ", MontPreComputeLength );

    /*    *(uint32_t *)((uint32_t)Output + Index) = MontPreComputeLength * 8;

        Index+= 4;*/

        //n
        *(uint32_t *)((uint32_t)Output + Index) = mont.n.size;
        Index += sizeof(mont.n.size);
        memcpy(  (uint8_t *)Output + Index, mont.n.limb, mont.n.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at n is ......... ", (uint8_t *)Output + Index, mont.n.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	Index += mont.n.size*4 ;
        *(uint32_t *)((uint32_t)Output + Index) = mont.n.alloc;
        Index += sizeof(mont.n.alloc);
        *(uint32_t *)((uint32_t)Output + Index) = mont.n.neg;
        Index += sizeof(mont.n.neg);

        //np
        *(uint32_t *)((uint32_t)Output + Index) = mont.np.size;
        Index += sizeof(mont.np.size);
        memcpy(  (uint8_t *)Output + Index, mont.np.limb, mont.np.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at np is ......... ", (uint8_t *)Output + Index, mont.np.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	Index += mont.np.size*4 ;
        *(uint32_t *)((uint32_t)Output + Index) = mont.np.alloc;
        Index += sizeof(mont.np.alloc);
        *(uint32_t *)((uint32_t)Output + Index) = mont.np.neg;
        Index += sizeof(mont.np.neg);



        //rr
        *(uint32_t *)((uint32_t)Output + Index) = mont.rr.size;
        Index += sizeof(mont.rr.size);
        memcpy(  (uint8_t *)Output + Index, mont.rr.limb, mont.rr.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at rr is ......... ", (uint8_t *)Output + Index, mont.rr.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	Index += mont.rr.size*4 ;
        *(uint32_t *)((uint32_t)Output + Index) = mont.rr.alloc;
        Index += sizeof(mont.rr.alloc);
        *(uint32_t *)((uint32_t)Output + Index) = mont.rr.neg;
        Index += sizeof(mont.rr.neg);

        *MontContLength = MontPreComputeLength;



        /*BN_MONT_CTX *mont;
        BN_CTX *ctx;*/

	

	
        /*mont = BN_MONT_CTX_new();
        memset( mont,0, sizeof(BN_MONT_CTX));
        ctx = BN_CTX_new();
        //BN_MONT_CTX_init( mont );
        BN_CTX_init( ctx );*/

	



/*	my_BN_MONT_CTX_set( mont, RsaKey->n, ctx );

        MontPreComputeLength = sizeof( mont->ri ) + \
                                (mont->RR.top)*8  + sizeof(mont->RR.top) + sizeof(mont->RR.dmax) + sizeof(mont->RR.neg) + sizeof(mont->RR.flags) + \
                                (mont->N.top)*8  + sizeof(mont->N.top) + sizeof(mont->N.dmax) + sizeof(mont->N.neg) + sizeof(mont->N.flags) + \
                                (mont->Ni.top)*8  + sizeof(mont->Ni.top) + sizeof(mont->Ni.dmax) + sizeof(mont->Ni.neg) + sizeof(mont->Ni.flags) + \
                                sizeof(mont->n0) + sizeof(mont->flags) ;

	printf("\r\n The MontPreComputeLength value is %d ", MontPreComputeLength );

        *MontContLength = MontPreComputeLength*8 ;


        *(uint32_t *)((uint32_t)Output + Index) = mont->ri;
	printf("\r\n CHAIN_MONT: The value of mont->ri s %d ", *(uint32_t *)((uint32_t)Output + Index));
        Index += sizeof(mont->ri);

        //RR
        *(uint32_t *)((uint32_t)Output + Index) = mont->RR.top;
	printf("\r\n CHAIN_MONT: The value of mont->RR.top is %d ",  *(uint32_t *)((uint32_t)Output + Index)  );
        Index += sizeof(mont->RR.top);
        BN_bn2bin( &(mont->RR), (uint8_t *)Output + Index );
	printf("\r\n BEFORE   : Index is %d ", Index );
        Index += BN_num_bytes(&(mont->RR));
	printf("\r\n AFTER    : Index is %d ", Index );
        *(uint32_t *)((uint32_t)Output + Index) = mont->RR.dmax;
	printf("\r\n CHAIN_MONT: The value of mont->RR.dmaxis %d ",*(uint32_t *)((uint32_t)Output + Index));
        Index += sizeof(mont->RR.dmax);
        *(uint32_t *)((uint32_t)Output + Index) = mont->RR.neg;
        Index += sizeof(mont->RR.neg);
        *(uint32_t *)((uint32_t)Output + Index) = mont->RR.flags;
        Index += sizeof(mont->RR.flags);

        //N
        *(uint32_t *)((uint32_t)Output + Index) = mont->N.top;
	printf("\r\n mont->N.top is %d ", mont->N.top );
        Index += sizeof(mont->N.top);
        BN_bn2bin( &(mont->N), (uint8_t *)Output + Index );
        Index += BN_num_bytes(&(mont->N));
        *(uint32_t *)((uint32_t)Output + Index) = mont->N.dmax;
        Index += sizeof(mont->N.dmax);
        *(uint32_t *)((uint32_t)Output + Index) = mont->N.neg;
        Index += sizeof(mont->N.neg);
        *(uint32_t *)((uint32_t)Output + Index) = mont->N.flags;
        Index += sizeof(mont->N.flags);


        //Ni
        *(uint32_t *)((uint32_t)Output + Index) = mont->Ni.top;
	printf("\r\n mont->Ni.top is %d ", mont->Ni.top );
        Index += sizeof(mont->Ni.top);
        BN_bn2bin( &(mont->Ni), (uint8_t *)Output + Index );
        Index += BN_num_bytes(&(mont->Ni));
        *(uint32_t *)((uint32_t)Output + Index) = mont->Ni.dmax;
        Index += sizeof(mont->Ni.dmax);
        *(uint32_t *)((uint32_t)Output + Index) = mont->Ni.neg;
        Index += sizeof(mont->Ni.neg);
        *(uint32_t *)((uint32_t)Output + Index) = mont->Ni.flags;
        Index += sizeof(mont->Ni.flags);

        *(uint32_t *)((uint32_t)Output + Index) = mont->n0;
        Index += sizeof(mont->n0);
        *(uint32_t *)((uint32_t)Output + Index) = mont->flags;
        Index += sizeof(mont->flags);*/


	q_free( &qlip_ctx, &mont.rr);
	q_free( &qlip_ctx, &mont.np);
	q_free( &qlip_ctx, &mont.n);
	q_free( &qlip_ctx, &n);


	return( RetVal ) ;
}

int my_BN_MONT_CTX_set(BN_MONT_CTX *mont, const BIGNUM *mod, BN_CTX *ctx)
        {
        int ret = 0;
        BIGNUM *Ri,*R;

        BN_CTX_start(ctx);
	printf("\r\n IN FUNCTION my_BN_MONT_CTX_set " ) ;
        if((Ri = BN_CTX_get(ctx)) == NULL) goto err;
        R= &(mont->RR);                                 /* grab RR as a temp */
        if (!BN_copy(&(mont->N),mod)) goto err;         /* Set N */
        mont->N.neg = 0;
                { /* bignum version */

                mont->ri=BN_num_bits(&mont->N);
                BN_zero(R);
                if (!BN_set_bit(R,mont->ri)) goto err;  /* R = 2^ri */
                                                        /* Ri = R^-1 mod N*/
                if ((BN_mod_inverse(Ri,R,&mont->N,ctx)) == NULL)
                        goto err;
                if (!BN_lshift(Ri,Ri,mont->ri)) goto err; /* R*Ri */
                if (!BN_sub_word(Ri,1)) goto err;
                                                        /* Ni = (R*Ri-1) / N */
                if (!BN_div(&(mont->Ni),NULL,Ri,&mont->N,ctx)) goto err;
                }
        /* setup RR for conversions */
        BN_zero(&(mont->RR));
        if (!BN_set_bit(&(mont->RR),mont->ri*2))
	{
		printf("\r\n ERROR: BN_set_bit failed ");
		 goto err;
	}
        if (!BN_mod(&(mont->RR),&(mont->RR),&(mont->N),ctx))
	{
		printf("\r\n ERROR: BN_mod failed ");
		 goto err;
	}

        ret = 1;
err:
        BN_CTX_end(ctx);
        return ret;
        }

