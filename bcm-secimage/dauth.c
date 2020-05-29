/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: sbi.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "secimage.h"
#include "crypto.h"
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

unsigned int DauthArray[2048];

#define QLIP_MONT_CTX_SIZE 1576

#define MAX_KEY_SIZE	4096

/*----------------------------------------------------------------------
 * Name    : SecureBootImageHandler
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int DauthGenHandler(int ac, char **av)
{
	if (ac > 0) {
		if (!strcmp(*av, "-out"))
		  return GenDAuthHash (--ac, ++av);
	}
	DauthUsage();
	return STATUS_OK;
}

// TODO: Verify this function later
/*----------------------------------------------------------------------
 * Name    : GenDAuthHash
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int GenDAuthHash(int ac, char **av)
{
	char *outfile, *dauthkey=NULL, *arg;
	int status = STATUS_OK, verbose = 0, dauth_pubin=0,mont_ctx = 0;
	uint8_t digest[SHA256_HASH_SIZE];
	int RetVal = -1 ;
	RSA *rsa_key;
	int KeySize;
	uint32_t DauthStart = 0, totalLen = 0;
	int length = MAX_KEY_SIZE ;
	uint32_t * pDauthArray = DauthArray;

	outfile = *av;
	--ac; ++av;
	if (ac <= 0)
		return DauthUsage();
	printf("ac = %d\n", ac);
	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-v")) {
			verbose = 1;
		}
        if (!strcmp(arg, "-key")) {
			--ac; ++av;

			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-mont")) {
				if(verbose)
					printf(" Enabling Montogomery context ");
				mont_ctx = 1;
				--ac; ++av;
			}

			if (*av == NULL) {
			  printf("\nMissing parameter for -key\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-public")) {
				dauth_pubin = 1;
				--ac; ++av;
			}

			if (*av == NULL || **av == '-') {
			  printf("\nMissing key file name for -key\n");
			  return -1;
			}

			dauthkey = *av ;
		}
		--ac; ++av;
	}

	if( dauthkey ) {
	  if (verbose) {
		printf("\r\n DAUTH Key file name is %s ", dauthkey ) ;
		printf("\r\n The value of mont_ctx %d", mont_ctx );
	  }

		if(dauth_pubin) {
		  rsa_key = GetRSAPublicKeyFromFile(dauthkey, verbose);
		  if (verbose)
			printf("\r\n Trying to open Public Key file %s ", dauthkey );
		} 
		else {
			printf("DAuth key must be a public key\n");
			return(SBERROR_RSA_KEY_ERROR);
		}

		if (!rsa_key) {
			printf("Error getting RSA key from %s\n", dauthkey);
			return(SBERROR_RSA_KEY_ERROR);
		}
		else
		{
			KeySize =  RSA_size( rsa_key ) ;
			if (verbose)
			  printf("\r\n THE RSA KEY SIZE is %d \n", KeySize ) ;
            /* printf("\r\n .... Modulus m: %d bytes\n%s\n\n",
                        BN_num_bytes(rsa_key->n), BN_bn2hex(rsa_key->n)); */
		}
		DauthStart = 0;

		if( KeySize == 512 )
		{
			*pDauthArray = QLIP_MONT_CTX_SIZE;
			pDauthArray++;
			totalLen += 4 ;
			RetVal = AddMontCont( (uint32_t *)((uint32_t)DauthArray + 4), rsa_key, 1 ) ;

			if (verbose)
			  printf("\r\n SBI DAUTH : RetVal is %d \n", RetVal );

			pDauthArray += (RetVal/4);
			totalLen += RetVal ;
			pDauthArray++;
			totalLen += 4 ;
		}
		else
		{
			*pDauthArray = 0;
			pDauthArray++;
			totalLen += 4 ;
		}
	
		length = BN_num_bytes(rsa_key->n);

		if (verbose)
		  printf("RSA key length = %d\n", length);

 		*pDauthArray  = length * 8;
		//return(status);
		pDauthArray++;
		totalLen += 4;
	
		BN_bn2bin(rsa_key->n, (uint8_t *)pDauthArray);

		if (verbose)
		  DataDump("DAUTH public key", (uint8_t *)pDauthArray, length);

		pDauthArray = DauthArray;
		if( KeySize == 512 )
		{
			pDauthArray++;
			RetVal = Sha256Hash((uint8_t *)pDauthArray, (RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t)) , digest, verbose);	

			if (verbose) {
			  printf("\r\n .. The value of length is %d " , length );
			  DataDump(" MONT CTX BEFORE HASHING ", (uint8_t *)pDauthArray, (RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t)));
			}
		}
		else
		{
		  RetVal = Sha256Hash((uint8_t *)pDauthArray, length + 4 + 4, digest, verbose);
		}

		if (RetVal) 
		{
				printf ("SBIAddDauthKey: SHA256 hash error\n");
				return RetVal;
		}

		if (verbose)
		  DataDump("DAUTH public key hash", digest, SHA256_HASH_SIZE);

		RetVal = DataWrite(outfile, digest, SHA256_HASH_SIZE);
		if (RetVal) 
		{
			printf ("SBIAddDauthKey: Hash write error\n");
			return RetVal;
		}
	} 
	else {
		printf (" No DAUTH key\n");
		DauthUsage();
	}
	return status;
}


/*----------------------------------------------------------------------
 * Name    : DauthUsage
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int DauthUsage(void)
{
	printf ("\nTo create Dauth hash:\n");
	printf ("secimage: dauth -out hashoutputfile -key <-mont> [-public rsa_dauth_public key]");
	printf ("\n -mont: creates the montgomery context, a must for RSA 4096 bit keys, optional for 2048 bit keys\n\n");
	return STATUS_OK;
}

int AddMontCont( uint32_t *h, RSA *RsaKey, int Dauth )
{
	int RetVal = -1 ;
	int length;
	uint32_t MontPreComputeLength;
    uint32_t totalLen = 0;
    uint32_t DauthStart;
	int status = STATUS_OK;

	q_lint_t n;
	q_mont_t mont;

	LOCAL_QLIP_CONTEXT_DEFINITION;

	status  = q_init (&qlip_ctx, &n, (RsaKey->n->top)*2); 
	status += q_init (&qlip_ctx, &mont.n, n.alloc);
	status += q_init (&qlip_ctx, &mont.np, (n.alloc+1));
	status += q_init (&qlip_ctx, &mont.rr, (n.alloc+1));
	if (status != STATUS_OK )
	{
		printf("\r\n ERROR:     q_init failed ......\r\n ");
		return( -1 );
	}

	n.size = (RsaKey->n->top)*2;	
	printf("\r\n The Value of n.size is %d ", n.size );
	n.alloc = (RsaKey->n->dmax)*2;
	printf("\r\n The Value of n.alloc is %d ", n.alloc );
	n.neg = RsaKey->n->neg;
	printf("\r\n The Value of n.neg is %d ", n.neg);

	memcpy( n.limb, RsaKey->n->d, (RsaKey->n->top)*8) ;

	DataDump("\r\n The value of n.limb is ..........", RsaKey->n->d, (RsaKey->n->top)*8 );

	status = q_mont_init_sw (&qlip_ctx, &mont, &n);

	if( status != STATUS_OK )
	{
		printf("\r\n ERROR:    q_mont_init_sw failed ......\r\n ");
		return( -1 );

	}

	MontPreComputeLength =  sizeof(mont.n.size) + sizeof(mont.n.alloc) + sizeof(mont.n.neg) + mont.n.size*4 + \
				sizeof(mont.np.size) + sizeof(mont.np.alloc) + sizeof(mont.np.neg) + mont.np.size*4 + \
				sizeof(mont.rr.size) + sizeof(mont.rr.alloc) + sizeof(mont.rr.neg) + mont.rr.size*4 + \
				sizeof(mont.br);

	printf("\r\n The MontPreComputeLength value is %d ", MontPreComputeLength );

        *(uint32_t *)((uint32_t)h) = MontPreComputeLength * 8;
        length = MontPreComputeLength;

	totalLen += 4;
	DauthStart = totalLen;

        //n
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.size;
        totalLen += sizeof(mont.n.size);
        memcpy(  (uint8_t *)h + totalLen, mont.n.limb, mont.n.size*4);
        printf("\r\n--------------------------------------------") ;
        DataDump("\r\n The Value at n is ......... ", (uint8_t *)h + totalLen, mont.n.size*4 );
        printf("\r\n--------------------------------------------") ;
        totalLen += mont.n.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.alloc;
        totalLen += sizeof(mont.n.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.neg;
        totalLen += sizeof(mont.n.neg);

        //np
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.size;
        totalLen += sizeof(mont.np.size);
        memcpy(  (uint8_t *)h + totalLen, mont.np.limb, mont.np.size*4);
        printf("\r\n--------------------------------------------") ;
        DataDump("\r\n The Value at np is ......... ", (uint8_t *)h + totalLen, mont.np.size*4 );
        printf("\r\n--------------------------------------------") ;
        totalLen += mont.np.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.alloc;
        totalLen += sizeof(mont.np.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.neg;
        totalLen += sizeof(mont.np.neg);



	//rr
	*(uint32_t *)((uint32_t)h + totalLen) = mont.rr.size;
        totalLen += sizeof(mont.rr.size);
	memcpy(  (uint8_t *)h + totalLen, mont.rr.limb, mont.rr.size*4);
        printf("\r\n--------------------------------------------") ;
        DataDump("\r\n The Value at rr is ......... ", (uint8_t *)h + totalLen, mont.rr.size*4 );
        printf("\r\n--------------------------------------------") ;
	totalLen += mont.rr.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.rr.alloc;
        totalLen += sizeof(mont.rr.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.rr.neg;
        totalLen += sizeof(mont.rr.neg);

		return( length );
}
