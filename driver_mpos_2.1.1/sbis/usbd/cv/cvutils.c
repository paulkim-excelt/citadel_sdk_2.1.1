/*
 * $Copyright Broadcom Corporation$
 *
 */
/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/
/* 
 * Broadcom Corporation Credential Vault
 */
/*
 * cvmanager.c:  Credential Vault Utilities
 */
/*
 * Revision History:
 *
 * 01/07/07 DPA Created.
*/
#include "cvmain.h"
#include <platform.h>

void
cvGetVersion(cv_version *version)
{
	/* this routine gets the version of the CV firmware and is updated with a firmware upgrade */
	version->versionMajor = CV_MAJOR_VERSION;
	version->versionMinor = CV_MINOR_VERSION;
}








