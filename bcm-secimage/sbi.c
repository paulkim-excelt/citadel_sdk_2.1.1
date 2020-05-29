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

unsigned int SBIArray[MAX_BOOTLOADER_SIZE / sizeof(int)];

#define QLIP_MONT_CTX_SIZE 1576
#define PAGE_SIZE_4K  0x1000

/*---------------------------------------------------------------
 * Name    : YieldHandler
 * Purpose : Intermediate call for system yield Function
 * Input   : pointers to the data parameters
 * Return  : 0
 */
int YieldHandler(void)
{
  return(STATUS_OK); /* Eventually will be the OS yield function. */
}


/*
 *
 *  Case 1. Boot No Image  For reproting public key
 *
 *  Prereq: None
 *
 *  Header must have location for Kcr-pub reporting and error code reporting
 *  (what about Kaes and Kauth)
 *
 *                          ___________________________________
 *                         |                                   |
 *                         |          Image Header             |
 *                         |___________________________________|
 *
 *
 *
 *  Case 2. Boot Kaes Encrypted Kauth Authenticated Image (Local Keys)
 *
 *  Prereq: Kaes Kauth IV Boot-Image
 *
 *  Do: encr BI with Kaes
 *      build SBI
 *      calculate HMA-SHA1(Kauth, SBI)
 *
 *  Complete SBI must have Header, IV, encrypted bootimage, HMAC-hash
 *
 *                          ___________________________________
 *                         |                                   |
 *                         |          Image Header             |
 *                         |___________________________________|
 *                         |                                   |
 *                         |           Encryption IV           |
 *                         |___________________________________|
 *                         |                                   |
 *                         |                                   |
 *                         |           Secure Image            |
 *                         |    Encrypted with local key Kaes  |
 *                         |                                   |
 *                         |___________________________________|
 *                         |                                   |
 *                         |    HMAC-SHA over all of above     |
 *                         | Authenticated with local key Kauth|
 *                         |___________________________________|
 *
 *
 *  Case 3. Authenticaed signed Cleartext Boot Image
 *
 *  Prereq: Boot-Image  Kdi-pub
 *
 *  Do: encr BI with Kaes
 *      build SBI
 *      calculate HMA-SHA1( Krom, SBI)
 *      calculate DSA sign of the previous HMAC with kdi-pub
 *
 *  Complete SBI must have Header, bootimage, DSA signature
 *
 *                          ___________________________________
 *                         |                                   |
 *                         |          Image Header             |
 *                         |___________________________________|
 *                         |                                   |
 *                         |                                   |
 *                         |     Cleartext Boot Image          |
 *                         |                                   |
 *                         |                                   |
 *                         |___________________________________|
 *                         |                                   |
 *                         |     <HMAC-SHA1(Krom,All)>Kdi-pub  |
 *                         |___________________________________|
 *
 *
 *  Case 4. Boot DLIES secured Signed Image
 *
 *  Prereq: Boot-Image  Kdi-pub DLIES parameters  Kdc-pub
 *
 *  Do: Generate fresh DH key pair
 *      Generate fresh DH parameters
 *      Generate DH shared secret using Kdc-pub
 *      calculate HMAC-SHA(Header, DHparam) with Krom as key
 *      calculate DSA sign of the previous HMAC with kdi-pub
 *      Generate k1,k2,k3
 *      encr BI with DLIES k1
 *      auth BI with DLIES k2
 *      build SBI
 *      calculate HMA-SHA1( Krom, SBI)
 *
 *  Complete SBI must have Header, bootimage, DSA signature
 *
 *                          ___________________________________
 *                         |                                   |
 *                         |          Image Header             |
 *                         |___________________________________|
 *                         |                                   |
 *                         |     DLIES Encryption Parameter    |
 *                         |___________________________________|
 *                         |                                   |
 *                         |   DLIES Authentication Parameter  |
 *                         |___________________________________|
 *                         |                                   |
 *                         |   <HMAC-SHA1(Krom,Above)>Kdi-pub  |
 *                         |___________________________________|
 *                         |                                   |
 *                         |                                   |
 *                         |           Secure Image            |
 *                         |    Encrypted with DLIES key k1    |
 *                         |  Authencticated with DLIES key k2 |
 *                         |                                   |
 *                         |___________________________________|
 *                         |                                   |
 *                         |   HMAC-SHA1(DLIESk2,Boot)         |
 *                         |___________________________________|
 *
 *
 *
 */
#define LOCAL_AKEY_SIZE			20
#define LOCAL_IV_SIZE			16

/* a hack to get encrypted image working */
uint32_t extra_header=0;

/*Global variable which states whether DAuth is selected or not */
uint32_t DAuth = 0;

#define MAX_KEY_SIZE	4096

/*----------------------------------------------------------------------
 * Name    : SecureBootImageHandler
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SecureBootImageHandler(int ac, char **av)
{
	if (ac > 0) {
		if (!strcmp(*av, "-out"))
			return CreateSecureBootImage (--ac, ++av);
		if (!strcmp(*av, "-in")) {
			return VerifySBI (*++av);
		}
	}
	SBIUsage();
	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : Alignment setting
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int alignmentSetting(int length, int alignByte)
{
   int val;
	val = (length/alignByte)+1;
	val = val * alignByte;
	return val;
}
/*----------------------------------------------------------------------
 * Name    : CreateSecureBootImage
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateSecureBootImage(int ac, char **av)
{
	SBI_HEADER *sbi;
	UNAUTHENT_SBI_HEADER *unh;
	AUTHENTICATED_SBI_HEADER *auh;

	char *outfile, *configfile, *arg, *privkey = NULL, *pstkey = NULL, *local = NULL, *bl = NULL, *iv = NULL;
	char *chain=NULL, *dauthkey=NULL, *bblkey = NULL, *recimage = NULL, *ecdsPtr = NULL;
	int status = STATUS_OK, verbose = 0, rsanosign = 0, ecdsanosign = 0, sigflag = 0, dauth_pubin=0, recovery = 0, mont_ctx = 0, alignByte = 0, sbiLoadAddress = 0, pstflag = 0, i = 0;
	uint8_t big_endian =  0;
	uint32_t sbiLen, authLen;
	uint32_t use_sha256 = 1;
	uint32_t use_dba = 0;
	int aesEncryption = 0;
	uint32_t BroadcomRevisionID=0;
	uint32_t AESKeySize = 0;
	uint32_t SBIConfiguration=0;
    uint32_t IVC,sigLen, finalSigLength;
    char *previousOption = NULL;
	 uint8_t *sigOffset = NULL;

	outfile = *av;
	--ac; ++av;
	if (ac <= 0) {
		return SBIUsage();
    }

	sbi = (SBI_HEADER *)SBIArray;
	if (sbi == NULL) {
		puts ("Memory allocation error");
		return SBERROR_MALLOC;
	}
	unh = &(sbi->unh);
	auh = &(sbi->sbi);
	InitializeSBI(sbi);

	while (ac) {
		arg = *av;
		if (!strcmp(arg, "-v")) {
		  previousOption = arg;
			verbose = 1;
		}
                else if (!strcmp(arg, "-BE")) {
		  previousOption = arg;
                        big_endian = 1;
                        if (verbose)
                                printf ("Boot Loader Payload is Big Endian\n");
                }
		else if (!strcmp(arg, "-bl")) {
		  previousOption = arg;
			--ac, ++av;
			bl = *av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing boot image file name for -bl\n");
			  return -1;
			}

			if (verbose)
				printf ("Boot Image file: %s\n", *av);
		}
		else if (!strcmp(arg, "-iv")) {
		  previousOption = arg;
			--ac, ++av;
			iv = *av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing AES IV file name for -iv\n");
			  return -1;
			}

			if (verbose)
				printf ("AES IV file: %s\n", *av);
		}
		else if (!strcmp(arg, "-dl")) {
		  previousOption = arg;
			--ac, ++av;
			local = *av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing SBI local key file name for -dl\n");
			  return -1;
			}

			aesEncryption = 1;
			if (verbose)
				printf ("SBI local key file: %s\n", *av);
		}
		else if (!strcmp(arg, "-rsa")) {
		  previousOption = arg;
			--ac, ++av;
			privkey = *av;

			if (privkey == NULL) {
			  printf("\nMissing RSA Private key file name for -rsa\n");
			  return -1;
			}

			sigflag = RSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;
			if (verbose)
				printf ("RSA Private Key file: %s\n", privkey);
			rsaSignaturePrework((uint8_t *)auh, privkey, verbose);

		} else if (!strcmp(arg, "-sbialign")) {
			previousOption = arg;
			--ac, ++av;

			if (*av == NULL || *av == '-') {
			  printf("\nMissing SBI byte boundary alignment parameter for -sbialign\n");
			  return -1;
			}

			sscanf (*av, "%x", &alignByte);
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			   printf("\nMissing SBI load adress parameter for -sbialign\n");
			  return -1;
			}


			sscanf (*av, "%x", &sbiLoadAddress);

			if (verbose)
				printf ("Align SBI start to 0x%x byte boundary for SBI load address:0x%08x\n", alignByte, sbiLoadAddress);

		}
		else if (!strcmp(arg, "-rsanosign")) {
		  previousOption = arg;
			rsanosign = 1;
			sigflag = RSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;

			--ac, ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing length parameter for -rasnosign\n");
			  return -1;
			}

			sscanf (*av, "%d", &sigLen);

			--ac, ++av;
			if (*av == NULL || **av == '-') {
			  printf("\nMissing IVC parameter for -rsanosign\n");
			  return -1;
			}

			sscanf (*av, "%x", &IVC);

			if (verbose)
				printf ("rsanosign adding %d bytes to header length IVC=%04X\n", sigLen,IVC);
		}
		else if (!strcmp(arg, "-ecdsa")) {
		  previousOption = arg;
			--ac, ++av;
			privkey = *av;

			if (privkey == NULL) {
			  printf("\nMissing ECDSA Private key file name for -ecdsa\n");
			  return -1;
			}

			sigflag = ECDSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;
			if (verbose)
				printf ("ECDSA Private Key file: %s\n", privkey);
			 ecdsPtr = ecdsSignaturePrework( (uint8_t *)auh, privkey, verbose);
		}
		else if (!strcmp(arg, "-ecdsanosign")) {
		  previousOption = arg;
			ecdsanosign = 1;
			sigflag = ECDSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;
		}
		else if (!strcmp(arg, "-chain")) {
		  previousOption = arg;
			--ac, ++av;
			chain = *av;
			if (chain == NULL) {
			  printf("\nMissing chain file name for -chain\n");
			  return -1;
			}
		}
		else if (!strcmp(arg, "-ecdsa")) {
		  previousOption = arg;
			--ac, ++av;
			privkey = *av;

			if (privkey == NULL) {
			  printf("\nMissing ECDSA Private key file name for -ecdsa\n");
			  return -1;
			}

			sigflag = ECDSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;
			if (verbose)
				printf ("ECDSA Private Key file: %s\n", privkey);
		}
		else if (!strcmp(arg, "-ecdsanosign")) {
		  previousOption = arg;
			ecdsanosign = 1;
			sigflag = ECDSA_FLAG;
			if (!auh->nCOTEntries)
				auh->nCOTEntries = 0;
		}
		else if (!strcmp(arg, "-chain")) {
		  previousOption = arg;
			--ac, ++av;
			chain = *av;

			if (chain == NULL) {
			  printf("\nMissing argument for -chain\n");
			  return -1;
			}

			if (verbose)
				printf ("Adding RSA flag but no signature\n");
		}
		else if (!strcmp(arg, "-depth")) {
			uint32_t depth=0;
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing depth argument for -depth\n");
			  return -1;
			}

			sscanf (*av, "%x", &depth );
			auh->nCOTEntries = depth;
			if (verbose)
				printf ("Chain of trust has %d public keys\n", depth);
		}
		else if (!strcmp(arg, "-SBIConfiguration"))
		{
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -SBIConfiguration\n");
			  return -1;
			}

			sscanf (*av, "%x", &SBIConfiguration );
			auh->SBIConfiguration = SBIConfiguration;
			if (verbose)
				printf ("SBIConfiguration = %08x\n", SBIConfiguration);
		}
		else if (!strcmp(arg, "-SBIRevisionID"))
		{
			uint32_t SBIRevisionID=0;
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -SBIRevisionID\n");
			  return -1;
			}

			sscanf (*av, "%x", &SBIRevisionID );
			auh->SBIRevisionID = SBIRevisionID;
			if (verbose)
				printf ("SBIRevisionID = %08x\n", SBIRevisionID);
		}
		else if (!strcmp(arg, "-CustomerID"))
		{
			uint32_t CustomerID=0;
		  previousOption = arg;
			--ac, ++av;
			sscanf (*av, "%x", &CustomerID );
			auh->CustomerID = CustomerID;
			if (verbose)
				printf ("CustomerID = %08x\n", CustomerID);
		}
		else if (!strcmp(arg, "-ProductID"))
		{
			uint32_t ProductID=0;
		  previousOption = arg;
			--ac, ++av;
			sscanf (*av, "%x", &ProductID );
			auh->ProductID = ProductID;
			if (verbose)
				printf ("ProductID = %08x\n", ProductID);
		}
		else if (!strcmp(arg, "-CustomerRevisionID"))
		{
			uint32_t CustomerRevisionID=0;
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -CustomerRevisionID\n");
			  return -1;
			}

			sscanf (*av, "%x", &CustomerRevisionID );
			auh->CustomerRevisionID = CustomerRevisionID;
			if (verbose)
				printf ("CustomerRevisionID = %08x\n", CustomerRevisionID);
		}
		else if (!strcmp(arg, "-SBIUsage"))
		{
			uint32_t SBIUsage=0;
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -SBIUsage\n");
			  return -1;
			}

			sscanf (*av, "%x", &SBIUsage );
			auh->SBIUsage = SBIUsage;
			if (verbose)
				printf ("SBIUsage = %08x\n", SBIUsage);
		}
		else if (!strcmp(arg, "-BroadcomRevisionID"))
		{
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-' || **av == '-') {
			  printf("\nMissing parameter for -BroadcomRevisionID\n");
			  return -1;
			}

			sscanf (*av, "%x", &BroadcomRevisionID );
			if (verbose)
				printf ("BroadcomRevisionID = %08x\n", BroadcomRevisionID);
		}
		else if (!strcmp(arg, "-AESKeySize"))
		{
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing size for -AESKeySize\n");
			  return -1;
			}

			sscanf (*av, "%d", &AESKeySize );
			if (verbose)
				printf ("AESKeySize = %08x\n", AESKeySize);
		}
		else if (!strcmp(arg, "-hmac")) {
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing sizefile name for -hmac\n");
			  return -1;
			}

			privkey = *av;
			sigflag = HMAC_FLAG;
			if (verbose)
				printf ("HMAC Private Key file: %s\n", privkey);
			auh->AuthLength = SHA256_HASH_SIZE;
			auh->ImageVerificationConfig = IPROC_SBI_VERIFICATION_CONFIG_SIGNATURE_OR_HMAC_HASHALG_SHA256;
		}
		else if (!strcmp(arg, "-pst")) {
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing sizefile name for -pst\n");
			  return -1;
			}

			pstkey = *av;
			pstflag = PST_FLAG;
			alignByte = PAGE_SIZE_4K;
			if (verbose)
				printf ("HMAC Private Key file: %s\n", pstkey);
		}
		else if (!strcmp(arg, "-extraheader")) {
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -extraheader\n");
			  return -1;
			}

			sscanf (*av, "%d", &extra_header );
			if (verbose)
				printf ("Extra header %d\n", extra_header);
		}
		else if (!strcmp(arg, "-dauth")) {
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av != '-') {
			  printf("\nMissing parameter for -dauth\n");
			  return -1;
			}

                        arg = *av;
                        if (!strcmp(arg, "-mont")) {
				if(verbose)
					printf(" Enabling Montogomery context ");

                                mont_ctx = 1;
                                --ac, ++av;
                        }

			if (*av == NULL || **av != '-') {
			  printf("\nMissing parameter for -dauth\n");
			  return -1;
			}

			arg = *av;
			if (!strcmp(arg, "-public")) {
				dauth_pubin = 1;
				--ac, ++av;
			}

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -dauth\n");
			  return -1;
			}

			dauthkey = *av ;
			use_sha256 = 1;
			if(verbose)
				printf("Adding DAUTH  public key %s\n", dauthkey ) ;
		}
		else if(!strcmp(arg, "-config")){
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing filename for -config\n");
			  return -1;
			}

			configfile = *av;
			if(verbose)
				printf("Config file is %s ", configfile ) ;
		}
		else if(!strcmp(arg, "-bblkey")){
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -bblkey\n");
			  return -1;
			}

			bblkey = *av;
		}
		else if (!strcmp(arg, "-recovery")) {
		  previousOption = arg;
			--ac, ++av;

			if (*av == NULL || **av == '-') {
			  printf("\nMissing parameter for -recovery\n");
			  return -1;
			}

			recimage = *av;
			recovery = 1;
			if (verbose)
				printf ("Adding RSA flag but no signature\n");
		}
		else {
		  printf("\nUnexpected input:\"%s\" ", arg);
		  if (previousOption != NULL)
		    printf("for option:%s", previousOption);
		  printf("\n\n");

		  return SBIUsage();
		}
		--ac, ++av;
	}

	if (status == STATUS_OK) {
		if (extra_header && !iv) {
			printf("An IV file must come together with extra header\n");
			return -1;
		}

		if ( ((!iv && local) || (iv && !local)) && !extra_header)
		{
			printf("IV file must come together with AES key file\n");
			return -1;
		}

		if (configfile != NULL) {
		  if (FillUnauthFromConfigFile( sbi, verbose, configfile) != STATUS_OK) {
		    return -1;
		  }
		}


		if((SBIConfiguration & IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH) == IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH)
			use_dba = 1;

		// TODO: Calculate Length and CRC at the end
		//unh->crc = calc_crc32(0xFFFFFFFF, (uint8_t *)unh, sizeof( UNAUTHENT_SBI_HEADER) - sizeof(uint32_t)) ;
		//printf("\r\n crc of Unh is 0x%8X", unh->crc);
		if (dauthkey) {
			if (verbose)
				printf("\n Adding DAUTH key to SBI ");

			if (sigflag == RSA_FLAG)
				status = SBIAddDauthKey(sbi,
							verbose,
							dauthkey,
							dauth_pubin,
							BroadcomRevisionID);
			else if (sigflag == ECDSA_FLAG)
				status = SBIAddECDauthKey(sbi,
							  dauthkey,
							  dauth_pubin,
							  BroadcomRevisionID,
							  verbose);

			if (status != STATUS_OK)
				return -1;
		}

		/* Add the RSA/DSA Public Key */
		if (rsanosign) {
			if (chain) {
			/* Adding the public key chain of trust */
			  status = SBIAddPubkeyChain(sbi, verbose, chain);
			}
		}
		else if (chain) {
			/* Adding the public key chain of trust */
		  status = SBIAddPubkeyChain(sbi, verbose, chain);
		}
		else {
			/* Adding the public key from private key */
			/* Kcr_pub does not go inside SBI anymore, it's moved to ABI
			if (auh->HeaderFlags & SBI_AUTH_DSA_SIGNATURE)
				status = SBIAddDSAPubkey(sbi, privkey, 1);
			if (auh->HeaderFlags & SBI_AUTH_RSA_SIGNATURE)
				status = SBIAddRSAPubkey(sbi, privkey, 1);
			if (status)
				return status;
			*/
		}

		if (status != STATUS_OK)
			return -1;

		/* IV need to be added ahead of bl */
		status = SBIAddBootLoaderPayload(sbi,
						 verbose,
						 bl,
						 iv,
						 big_endian,
						 alignByte,
						 sbiLoadAddress);
		if (status)
			return -1;

		if (aesEncryption && (use_dba == 0))
		  status = CreateLocalSecureBootImage(sbi, local, bblkey, AESKeySize, verbose);

		if (status < 0)
			return status;

		/* special case of Including iv for RSA also */
		if (extra_header && iv) {
			auh->BootLoaderCodeOffset += extra_header;
		}

		sbiLen = auh->SBITotalLength;

		/* Pre-requisites for signature generation */
		if(alignByte == PAGE_SIZE_4K && pstflag == PST_FLAG)
			auh->AuthenticationOffset = sbiLen + PAGE_SIZE_4K;
		else {
			auh->AuthenticationOffset = sbiLen;
		}
		/* fix up headers in nosign case */
		if (rsanosign) {
			auh->AuthLength = sigLen;
			auh->ImageVerificationConfig = IVC;
		}

		if (rsanosign) {
			/* account for sig to be appended later */
			unh->Length = auh->AuthenticationOffset + sigLen;
		} else {
			unh->Length = auh->AuthenticationOffset;
		}

		unh->Length = auh->AuthenticationOffset + auh->AuthLength;
		/*
		 * Below line should be at the end of unh modifications &
		 * before PST signature generation
		 */
		unh->crc = calc_crc32(0xFFFFFFFF,
				      (uint8_t *)unh,
				      sizeof(UNAUTHENT_SBI_HEADER) -
				      sizeof(uint32_t));

		/* Reserve space for PST signature for each 4K Page.*/
		if (alignByte == PAGE_SIZE_4K && pstflag == PST_FLAG) {
			uint32_t i = 0;

			for (i = 0;
			     i <= auh->SBITotalLength;
			     i = i + alignByte) {
				if (verbose)
					printf("HMAC signing %d bytes\n",
						auh->SBITotalLength);
				/*
				 * last param is signature offset where it
				 * should be written *
				 */
				status = AppendHMACSignature(((uint8_t *)unh+i),
							     alignByte,
							     pstkey,
							     bblkey,
							     verbose,
							     (sbiLen-i));
				if (status < 0 ) {
					return -1;
				} else if (status > 0) {
					sbiLen += status;
					status = 0;
				}
			}
		}

		/*Alignment Setting */
		if (alignByte != 0) {
			if((sbiLen % alignByte) != 0)
				sbiLen = alignmentSetting(sbiLen, alignByte);

		}
		/* End of HMAC */

#ifdef BYTE_ORDER_BIG_ENDIAN
		{
		int i;
		for (i = 0; i < sizeof(SBI_HEADER); i += 4)
			swap_word((void *)sbi + i);
		}
#endif
		authLen = sbiLen - sizeof (UNAUTHENT_SBI_HEADER);
		if ((alignByte) && (sbi->sbi.AuthenticationOffset != sbiLen)) {
			/*
			 * AuthenticationOffset has not taken into account the
			 * padding so we need to update parameter again.
			 */
			sbi->sbi.AuthenticationOffset = sbiLen;
		}

      /* Pre-requisites for signature generation */
			if(alignByte == PAGE_SIZE_4K && pstflag == PST_FLAG)
			{
				sigOffset =  ( ((uint8_t *)unh) + auh->AuthenticationOffset - PAGE_SIZE_4K);
				finalSigLength = PAGE_SIZE_4K;
			}
			else
			{
				sigOffset = (uint8_t *)auh;
				finalSigLength = authLen;
			}

		/* Now do the public key signatures */
		if ((sigflag == RSA_FLAG) && !rsanosign) {
		  if (verbose)
		    printf ("RSA signing %d bytes\n", authLen);
			status = AppendRSASignature(sigOffset, finalSigLength, privkey, use_sha256, verbose);
			if (status < 0 ) {
			  return -1;
			} else if (status > 0) {
				sbiLen += status;
				status = 0;
			}
		}
		if ((sigflag == RSA_FLAG) && rsanosign) {
			printf ("\nWriting SBI hash to the file sign_me.rsa\n");
 			OutputHashFile(sigOffset, finalSigLength, "sign_me.rsa", 0, verbose);
		}
		if ((sigflag == ECDSA_FLAG) && !ecdsanosign) {
		  if (verbose)
			printf ("ECDSA signing %d bytes\n", authLen);
			status = AppendECDSASignature((uint8_t *)sigOffset, finalSigLength, privkey, verbose, ecdsPtr);
			if (status < 0 ) {
			  return -1;
			} else if (status > 0) {
				sbiLen += status;
				status = 0;
			}
		}
		if ((sigflag == ECDSA_FLAG) && ecdsanosign) {
			printf ("\nWriting SBI hash to the file sign_me.ecdsa\n");
 			OutputHashFile(sigOffset, finalSigLength, "sign_me.ecdsa", 0, verbose);
		}

		if ((sigflag == HMAC_FLAG)) {
		  if (verbose)
			printf ("HMAC signing %d bytes\n", authLen);
			/* last parameter is destinationOffset, it will be same as 2nd param in few cases */
			status = AppendHMACSignature(sigOffset, finalSigLength, privkey, bblkey, verbose, finalSigLength);
			if (status < 0 ) {
			  return -1;
			} else if (status > 0) {
				sbiLen += status;
				status = 0;
			}
		}

		if (aesEncryption && (use_dba == 1))
		{
		  status = CreateLocalSecureBootImage(sbi, local, bblkey, AESKeySize, verbose);
			status = (status == 1) ? 0 : -1;
		}


		if ((!status) && recimage) {
			/* Adding the recovery image at the end of SBI */
			status = SBIAddRecoveryImage(sbi, recimage, sbiLen);
			printf("\r\n SBIAddRecoveryImage Returned %d ... \r\n" , status );
			if( status > 0 )
			{
				sbiLen += status;
				status =  0;
			}
		}

		if (!status) {
			SBI_HEADER header;
			memcpy((uint8_t *)&header, (uint8_t *)sbi, sizeof(SBI_HEADER));
			printf ("\nGenerating Secure Boot Image file %s: %d bytes\n\n",
							outfile, sbiLen);
			status = DataWrite(outfile, (void *)sbi, sbiLen);
		}
	}
		PrintSBI(sbi);
	if (status < 0)
		printf ("SBI Generation error %d\n", status);
	return status;
}

/*----------------------------------------------------------------------
 * Name    : InitializeSBI
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int InitializeSBI(SBI_HEADER *header)
{
	AUTHENTICATED_SBI_HEADER *auh = &(header->sbi);

	memset ((uint8_t *)header, 0, sizeof(SBI_HEADER));
	auh->SBIConfiguration = 0;
	auh->SBITotalLength = sizeof(SBI_HEADER);
	auh->COTOffset = 0;
	auh->MagicNumber = SBI_MAGIC_NUMBER;

	return STATUS_OK;
}


/*----------------------------------------------------------------------
 * Name    : PrintSBI
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int PrintSBI(SBI_HEADER *header)
{
  AUTHENTICATED_SBI_HEADER *auh = (AUTHENTICATED_SBI_HEADER *)&(header->sbi);

  if (auh->MagicNumber != SBI_MAGIC_NUMBER) {
    printf ("Bad SBI magic number %08x %08x\n", auh->MagicNumber , SBI_MAGIC_NUMBER);
    DataDump("SBI Header", header, sizeof(SBI_HEADER));
    return SBIERROR_IMAGE_HEADER;
  }

  printf("\nAuth SBI Configuration               0x%08x ", auh->SBIConfiguration);

  if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_SYMMETRIC) == IPROC_SBI_CONFIG_SYMMETRIC)
    printf("IPROC_SBI_CONFIG_SYMMETRIC ");
  if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE) == IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE)
    printf("IPROC_SBI_CONFIG_AUTHENTICATE_INPLACE ");

  if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_AES_256_ENCRYPTION) == IPROC_SBI_CONFIG_AES_256_ENCRYPTION)
    printf("IPROC_SBI_CONFIG_AES_256_ENCRYPTION ");
  else if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS)
    printf("IPROC_SBI_CONFIG_AES_128_ENCRYPTION_FIRST128BITS ");
  else if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS) == IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS)
    printf("IPROC_SBI_CONFIG_AES_128_ENCRYPTION_SECOND128BITS ");

  if ((auh->SBIConfiguration & IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH) == IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH)
    printf("IPROC_SBI_CONFIG_DECRYPT_BEFORE_AUTH ");
  printf("\n");

  printf("COT Offset                           %6d (0x%x) COT Length: 0x%x \n", auh->COTOffset, auh->COTOffset, auh->COTLength);
  printf("Number of COT Entries (Chain Depth)  %6d (0x%x)\n", auh->nCOTEntries, auh->nCOTEntries);
  printf("Boot Loader Code Offset              %6d (0x%x)\n", auh->BootLoaderCodeOffset, auh->BootLoaderCodeOffset);
  printf("Boot Loader Code Length              %6d (0x%x)\n", auh->BootLoaderCodeLength, auh->BootLoaderCodeLength);
  printf("Authentication Offset                %6d (0x%x) AuthLenth: 0x%x \n", auh->AuthenticationOffset, auh->AuthenticationOffset, auh->AuthLength);
  return STATUS_OK;
}


/*----------------------------------------------------------------------
 * Name    : VerifySBI
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int VerifySBI(char *filename)
{

	return 0;
}

/*----------------------------------------------------------------------
 * Name    : SBIAddBootLoaderPayload
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBIAddBootLoaderPayload(SBI_HEADER *h, int verbose, char *filename, char *ivfilename, uint8_t big_endian, int alignByte, uint32_t sbiLoadAddress)
{
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint32_t totalLen;
	int length = 0, i, swap_length=0;
	int padlen = 0;
	int status = STATUS_OK;
	uint32_t data, *ptr;

	struct stat file_stat;

	if (ivfilename)
	{
		length = LOCAL_IV_SIZE;
		totalLen = auh->SBITotalLength;
		status = DataRead(ivfilename, (uint8_t *)h + totalLen, &length);
		if (status || length != LOCAL_IV_SIZE)
		{
			printf("iv file read error, or not the correct size\n");
			return -1;
		}
		auh->SBITotalLength += length;
	}

	length = MAX_BOOTLOADER_SIZE;
	status = stat(filename, &file_stat);

	if (status != 0) {
	  printf("\nFailed to open file:%s\n", filename);
	  return -1;
	}

	if (verbose)
	  printf("Filesize %s: %d bytes\n",filename, (int)file_stat.st_size);

	if (file_stat.st_size > MAX_BOOTLOADER_SIZE) {
	  printf("\nFilesize %d greater than MAX %d\n", (int)file_stat.st_size, MAX_BOOTLOADER_SIZE);
	  return -1;
	}

	totalLen = auh->SBITotalLength;

	/*
	 * If required the start of the SBI can be padded to ensure instruction alignment
	 */
	if (alignByte > 0) {
		int offPad = 0;
		uint32_t address = sbiLoadAddress + totalLen;

		offPad = ((address % alignByte) == 0 ? 0:(alignByte - (address % alignByte)));

		if (offPad > 0) {
			totalLen += offPad;
			if (verbose)
				printf("Aligning SBI entry offset to 0x%0x byte boundary by padding 0x%x bytes. Offset: 0x%x\n", alignByte, offPad, totalLen);
			auh->SBITotalLength += offPad;
		}
	}

	status = DataRead(filename, (uint8_t *)h + totalLen, &length);
	if (status != STATUS_OK)
		return -1;

	if (verbose)
		printf("\r\n Adding file %s ... \r\n", filename );

	if (!status) {
		if( big_endian )
		{
			if( length & 0x4 )
			{
				swap_length = length - (length&0x4);
			}
			else
			{
				swap_length = length;
			}
			for( i=0; i< swap_length; i+= 4 )
			{
				ptr = (uint32_t*)((uint8_t *)h + totalLen + i);
				data = *ptr;
				data = (data >> 24) | ((data >> 8) & 0xFF00) | ((data << 8) & 0xFF0000) | (data << 24);
				*ptr = data;
			}
		}
      if (alignByte != PAGE_SIZE_4K) /* Regular methods */
	{
	  if (length & 15)
	    {
	      padlen = 16 - (length & 15);
	      memset ((uint8_t *) h + totalLen + length, 0, padlen);
	      if (verbose)
		printf
		  ("Boot Image length %d, will pad %d bytes to make 16s multiple.\n",
		   length, padlen);
	      length += padlen;
	    }
	}
      else if (alignByte == PAGE_SIZE_4K)
	{
      /*Alignment Setting */
	  if ((length % alignByte) != 0)
	    {
	       length = alignmentSetting (length, alignByte);
	    }
	}

      auh->BootLoaderCodeOffset = auh->SBITotalLength;
		auh->BootLoaderCodeLength = length;
		auh->SBITotalLength += length;

#ifdef DEBUG_DATA
		ShortDump ("BootImage payload", (uint8_t *)h + totalLen, length);
#endif
	} else
		printf ("Error reading BootImage Payload from %s\n", filename);

	return status;
}

/*----------------------------------------------------------------------
 * Name    : SBIAddRSAPubkey
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBIAddRSAPubkey(SBI_HEADER *h, char *filename, int priv, int verbose)
{
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint32_t totalLen = auh->SBITotalLength;
	uint16_t length = MAX_AUTH_PARAM_SIZE;
	int status = STATUS_OK;

	status = WriteRSAPubkey(filename, (uint8_t *)h + totalLen, &length, priv, verbose);
	if (!status) {
		printf ("\r\n Added RSA public key %d bytes... \n", length);
		auh->COTOffset = totalLen;
		printf("\r\n auh->COTOffset is %d ", auh->COTOffset);
		auh->SBITotalLength += length;
	}
	return status;
}

/*----------------------------------------------------------------------
 * Name    : SBIAddPubkeyChain
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBIAddPubkeyChain(SBI_HEADER *h, int verbose, char *filename)
{
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint32_t totalLen = auh->SBITotalLength;
	int length = 64000; /* Allow for up to 4 RSA sigs */
	int status = STATUS_OK;
	status = DataRead(filename, (uint8_t *)h + totalLen, &length);
	if ((status != STATUS_OK) || (length & 0x03))
	  return -1;

	if (!status) {
	  if (verbose)
	    printf ("Added public key chain file : %s  Length %d bytes\n", filename, length);
		if(!auh->COTOffset)
		   auh->COTOffset = totalLen;
		auh->COTLength = length;
		auh->SBITotalLength += length;
	}
	return status;
}


/*----------------------------------------------------------------------
 * Name    : CreateLocalSecureBootImage
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int CreateLocalSecureBootImage(SBI_HEADER *h, char *aesfile, char *bblkeyfile, uint32_t AESKeySize, int verbose)
{
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint8_t ekey[KEYSIZE_AES256];
	uint8_t bblkey[KEYSIZE_AES128];
	uint8_t bblkeyhash[SHA256_HASH_SIZE];
	uint8_t *iv = NULL;
	uint8_t *data = (uint8_t *)h;
	// uint8_t *edata;
	int status = STATUS_OK, length, bbllen;
    // uint16_t edataLen;

	length = AESKeySize;

	status = DataRead(aesfile, ekey, &length);
	if(status)
		return(status);

	if(bblkeyfile) {
		bbllen = KEYSIZE_AES128;
		status = DataRead(bblkeyfile, bblkey, &bbllen);
		if (status != STATUS_OK)
		  return -1;

		if (!status) {
			/* Decrypting the OTP key using BBL key */

		  if (verbose)
		    printf("Decrypting the OTP key using BBL key ...");

			status = SBILocalCrypto(0, ekey, length, bblkey, bbllen, NULL, verbose);
			status = (status == 1) ? 0 : -1;
		}
		/******* Write BBL Hash to a file **********/
		Sha256Hash( bblkey, length, bblkeyhash, verbose);
		DataWrite("bblhash.out", (char *)bblkeyhash, SHA256_HASH_SIZE);
	}

	if (!status) {
		iv = data + auh->BootLoaderCodeOffset - LOCAL_IV_SIZE; /* IV in sbi already */
		//edata = iv + LOCAL_IV_SIZE;
		//edataLen = auh->BootLoaderCodeLength - LOCAL_IV_SIZE;

		if (verbose)
		  printf("Encrypting the Data using OTP key...");

		status = SBILocalCrypto(1, data + auh->BootLoaderCodeOffset, auh->BootLoaderCodeLength, ekey, AESKeySize, iv, verbose);
	}
	return status;
}

/*----------------------------------------------------------------------
 * Name    : SBIUsage
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBIUsage(void)
{
	printf ("\nTo create a Secure Boot Image:\n");
	printf ("secimage: sbi -out sbifile [-v] [-rsa priv_pem_key] [-hmac hmac_binary_key] <-config configfile>");
	printf ("\n\t\t[-dl aes_key] [-bl blpayload]");
	printf ("\n\t\t[-rsanosign siglen imagevercfg] [-chain chain]\n");
	printf ("\n\t\t[-extraheader <bytes> (extra header + iv used to embed iv with rsa mode of operation to be useful for re-encryption with iv] [-iv <ivfilename>]\n");
//	printf ("\nTo verify an Authenticated Boot Image:\n");
//	printf ("secimage: sbi -in sbifile [-v] [-pubkey keyfile] [-payloads]\n\n");
	return STATUS_OK;
}

int SBIAddMontCont( SBI_HEADER *h, RSA *RsaKey, int offset, int verbose )
{
	uint32_t MontPreComputeLength;
    uint32_t totalLen = offset;

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

	n.size = (RsaKey->n->top);

	if (verbose)
	  printf("\r\n The Value of n.size is %d ", n.size );

	n.alloc = (RsaKey->n->dmax);

	if (verbose)
	  printf("\r\n The Value of n.alloc is %d ", n.alloc );

	n.neg = RsaKey->n->neg;

	if (verbose)
	  printf("\r\n The Value of n.neg is %d ", n.neg);

	memcpy( n.limb, RsaKey->n->d, (RsaKey->n->top)*4) ;

	if (verbose)
	  DataDump("\r\n The value of n.limb is ..........", RsaKey->n->d, (RsaKey->n->top)*4 );

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

	if (verbose)
	  printf("\r\n The MontPreComputeLength value is %d ", MontPreComputeLength );

        //n
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.size;
        totalLen += sizeof(mont.n.size);
        memcpy(  (uint8_t *)h + totalLen, mont.n.limb, mont.n.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at n is ......... ", (uint8_t *)h + totalLen, mont.n.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	totalLen += mont.n.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.alloc;
        totalLen += sizeof(mont.n.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.n.neg;
        totalLen += sizeof(mont.n.neg);

        //np
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.size;
        totalLen += sizeof(mont.np.size);
        memcpy(  (uint8_t *)h + totalLen, mont.np.limb, mont.np.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at np is ......... ", (uint8_t *)h + totalLen, mont.np.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	totalLen += mont.np.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.alloc;
        totalLen += sizeof(mont.np.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.np.neg;
        totalLen += sizeof(mont.np.neg);



	//rr
	*(uint32_t *)((uint32_t)h + totalLen) = mont.rr.size;
        totalLen += sizeof(mont.rr.size);
	memcpy(  (uint8_t *)h + totalLen, mont.rr.limb, mont.rr.size*4);

	if (verbose) {
	  printf("\r\n--------------------------------------------") ;
	  DataDump("\r\n The Value at rr is ......... ", (uint8_t *)h + totalLen, mont.rr.size*4 );
	  printf("\r\n--------------------------------------------") ;
	}

	totalLen += mont.rr.size*4;
        *(uint32_t *)((uint32_t)h + totalLen) = mont.rr.alloc;
        totalLen += sizeof(mont.rr.alloc);
        *(uint32_t *)((uint32_t)h + totalLen) = mont.rr.neg;
        totalLen += sizeof(mont.rr.neg);

	return MontPreComputeLength;
}


/*----------------------------------------------------------------------
 * Name    : SBIAddDauthKey
 * Purpose : Adds the optional Dauth key to the SBI
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/


int  SBIAddDauthKey(SBI_HEADER *h, int verbose, char *DauthKey, int dauth_pubin, uint32_t BroadcomRevisionID)
{
	int RetVal = -1  ;
	int length = MAX_KEY_SIZE ;
	int KeySize;
	uint32_t DauthStart = 0;
	uint8_t digest[SHA256_HASH_SIZE];
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint32_t totalLen = auh->SBITotalLength;
	RSA *rsa_key;

	IPROC_SBI_PUB_KEYTYPE KeyType;

	if (verbose)
	  printf("\r\n DAUTH Key file name is %s ", DauthKey ) ;

	if(dauth_pubin) {
	  rsa_key = GetRSAPublicKeyFromFile(DauthKey, verbose);
	   if (verbose)
		printf("\r\n Trying to open Public Key file %s ", DauthKey );
	}
	else {
	  rsa_key = GetRSAPrivateKeyFromFile(DauthKey, verbose);
	   if (verbose)
		printf("\r\n Trying to open Private Key file %s ", DauthKey );
	}

	if (!rsa_key) {
		printf("Error getting RSA key from %s\n", DauthKey);
		return(SBERROR_RSA_KEY_ERROR);
	}
	else
	{
		KeySize =  RSA_size( rsa_key ) ;
		if (verbose) {
		  printf("\r\n THE RSA KEY SIZE is %d ", KeySize ) ;
		  printf("\r\n .... Modulus m: %d bytes\n%s\n\n",
			 BN_num_bytes(rsa_key->n), BN_bn2hex(rsa_key->n));
		}
	}

	DauthStart = totalLen;

	if( KeySize == 512 )
	{
		KeyType = IPROC_SBI_RSA4096;
	}
	else if (KeySize == 256)
	{
		KeyType = IPROC_SBI_RSA2048;
	}
	else
	{
		printf ("SBIAddDauthKey: Invalid KeySize\n");
		return SBERROR_INVALID_KEYSIZE;
	}
	*(uint16_t *)( (uint32_t)h + totalLen ) = (uint16_t)KeyType;
	totalLen += sizeof(uint16_t);

	length = BN_num_bytes(rsa_key->n);
	if(length == 512)
	{
		*(uint16_t *)((uint32_t)h + totalLen) = (uint16_t)(RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + (3*sizeof(uint32_t)));
	}
	else
	{
		*(uint16_t *)((uint32_t)h + totalLen) = (uint16_t)(length + (3*sizeof(uint32_t)));
	}
	totalLen += sizeof(uint16_t);
	BN_bn2bin(rsa_key->n, (uint8_t *)h + totalLen);
	if (verbose)
	  DataDump("DAUTH public key", (uint8_t *)h + totalLen, length);
	totalLen += length;
	if( KeySize == 512 )
	{
	  RetVal = SBIAddMontCont( h, rsa_key, totalLen, verbose ) ;
		if (verbose)
		  printf("\r\n SBI DAUTH : RetVal is %d ", RetVal );
		if(RetVal != RSA_4096_MONT_CTX_BYTES)
		{
		  if (verbose)
			printf ("SBIAddDauthKey: Invalid MontCtx size\n");
		  return SBERROR_INVALID_MONTCTXSIZE;
		}
		totalLen += RetVal ;
	}
	*(uint32_t *)((uint32_t)h + totalLen) = h->sbi.CustomerID;
	totalLen += sizeof(uint32_t);
	*(uint16_t *)((uint32_t)h + totalLen) = h->sbi.ProductID;
	totalLen += sizeof(uint16_t);
	*(uint16_t *)((uint32_t)h + totalLen) = BroadcomRevisionID;
	totalLen += sizeof(uint16_t);

	if( KeySize == 512 )
	{
	  RetVal = Sha256Hash((uint8_t *)h + DauthStart, (RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t)) , digest, verbose);
		if (verbose)
		  DataDump("MONT CTX BEFORE HASHING ", (uint8_t *)h + DauthStart, (RSA_4096_KEY_BYTES + RSA_4096_MONT_CTX_BYTES + 3*sizeof(uint32_t)));
	}
	else
	{
	  RetVal = Sha256Hash((uint8_t *)h + DauthStart, length + 3*sizeof(uint32_t), digest, verbose);
	}


	if (RetVal)
	{
		printf ("SBIAddDauthKey: SHA256 hash error\n");
		return RetVal;
	}

        if (verbose)
	  DataDump("DAUTH public key hash", digest, SHA256_HASH_SIZE);

	RetVal = DataWrite("Dauth_hash.out", (char *)digest, SHA256_HASH_SIZE);
	if (RetVal)
	{
		printf ("SBIAddDauthKey: Hash write error\n");
		return RetVal;
	}

	auh->SBITotalLength = totalLen;
	return(RetVal);
}

/*----------------------------------------------------------------------
 * Name    : SBIAddECDauthKey
 * Purpose : Adds the optional Dauth key to the SBI
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/


int  SBIAddECDauthKey(SBI_HEADER *h, char *DauthKey, int dauth_pubin, uint32_t BroadcomRevisionID, int verbose)
{
	int RetVal = -1  ;
	int length = MAX_KEY_SIZE ;
	int KeySize;
	uint32_t DauthStart = 0;
	uint8_t digest[SHA256_HASH_SIZE];
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	uint32_t totalLen = auh->SBITotalLength;
	EC_KEY *ec_key;

	IPROC_SBI_PUB_KEYTYPE KeyType;

	if (verbose)
	  printf("\r\n DAUTH Key file name is %s ", DauthKey ) ;

	if(dauth_pubin) {
	  ec_key = GetECPublicKeyFromFile(DauthKey, verbose);
	  if (verbose)
		printf("\r\n Trying to open Public Key file %s ", DauthKey );
	}
	else {
	  ec_key = GetECPrivateKeyFromFile(DauthKey, verbose);
	  if (verbose)
		printf("\r\n Trying to open Private Key file %s ", DauthKey );
	}

	if (!ec_key) {
		printf("Error getting EC key from %s\n", DauthKey);
		return(SBERROR_EC_KEY_ERROR);
	}
	else
	{
		KeySize =  EC_Size(ec_key) ;
		if (verbose)
		  printf("\r\n THE EC KEY SIZE is %d ", KeySize) ;
		PrintECKey(ec_key);
	}

	DauthStart = totalLen;

	if( KeySize == EC_P256_KEY_BYTES )
	{
		KeyType = IPROC_SBI_EC_P256;
	}
	else
	{
		printf ("SBIAddECDauthKey: Invalid KeySize\n");
		return SBERROR_INVALID_KEYSIZE;
	}
	*(uint16_t *)( (uint32_t)h + totalLen ) = (uint16_t)KeyType;
	totalLen += sizeof(uint16_t);

	length = EC_Size(ec_key);
	*(uint16_t *)((uint32_t)h + totalLen) = (uint16_t)(length + (3*sizeof(uint32_t)));
	totalLen += sizeof(uint16_t);

	EC_KEY2bin(ec_key, (uint8_t *)h + totalLen);

	if (verbose)
	  DataDump("EC DAUTH public key", (uint8_t *)h + totalLen, length);

	totalLen += length;

	*(uint32_t *)((uint32_t)h + totalLen) = h->sbi.CustomerID;
	totalLen += sizeof(uint32_t);
	*(uint16_t *)((uint32_t)h + totalLen) = h->sbi.ProductID;
	totalLen += sizeof(uint16_t);
	*(uint16_t *)((uint32_t)h + totalLen) = BroadcomRevisionID;
	totalLen += sizeof(uint16_t);

	RetVal = Sha256Hash((uint8_t *)h + DauthStart, length + 3*sizeof(uint32_t), digest, verbose);

	if (RetVal)
	{
		printf ("SBIAddECDauthKey: SHA256 hash error\n");
		return RetVal;
	}


	if (verbose)
	  DataDump("EC DAUTH public key hash", digest, SHA256_HASH_SIZE);

	RetVal = DataWrite("ECDauth_hash.out", (char*)digest, SHA256_HASH_SIZE);
	if (RetVal)
	{
		printf ("SBIAddECDauthKey: Hash write error\n");
		return RetVal;
	}

	auh->SBITotalLength = totalLen;
	return(RetVal);
}


/*----------------------------------------------------------------------
 * Name    : SBIAddRecoveryImage
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int SBIAddRecoveryImage(SBI_HEADER *h, char *filename, int recovery_offset)
{
	AUTHENTICATED_SBI_HEADER *auh = &(h->sbi);
	UNAUTHENT_SBI_HEADER *unh = &(h->unh);
	int length = MAX_SBI_SIZE + 8;
	int status = STATUS_OK;
	status = DataRead(filename, (uint8_t *)h + recovery_offset, &length);
	if (status != STATUS_OK)
	  return -1;

	printf("\r\n In SBIAddRecoveryImage() auh->SBITotalLength is %d ... \r\n", auh->SBITotalLength );
	if (!status) {
		printf ("Added Recovery Image %d bytes\n", length);
//		unh->RecAddress = 8 + recovery_offset;
        unh->crc = calc_crc32(0xFFFFFFFF, (uint8_t *)unh,
			          sizeof(UNAUTHENT_SBI_HEADER) - sizeof(uint32_t));
		return length;
	}
	return status;
}
