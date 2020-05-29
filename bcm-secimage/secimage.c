/******************************************************************************
 *  Copyright 2005
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *****************************************************************************/
/*
 *  Broadcom Corporation 5890 Boot Image Creation
 *  File: secimage.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "secimage.h"

/*----------------------------------------------------------------------
 * Name    : Usage
 * Purpose : Specifies the first layer of usage of this module
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int Usage(void)
{
	puts ("secimage:  sbi|chain | dauth [parameters]");
	return STATUS_OK;
}

/*----------------------------------------------------------------------
 * Name    : main
 * Purpose : Calls the main processing engines
 * Input   : Specified is Usage()
 * Output  : none
 *---------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	if (argc >= 2) {
		argc--; argv++;
		if (!strcmp (*argv, "sbi"))
			return SecureBootImageHandler(--argc, ++argv);

		if (!strcmp (*argv, "chain"))
			return ChainTrustHandler(--argc, ++argv);

		if (!strcmp (*argv, "dauth"))
			return DauthGenHandler(--argc, ++argv);

		if (!strcmp (*argv, "test"))
			return InitCrypto();

	}
	Usage();

	return STATUS_OK;
}
