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
 * Broadcom Corporation Credential Vault API
 */
/*
 * cvmain.h:  PHX2 CV main header file
 */
/*
 * Revision History:
 *
 * 01/08/07 DPA Created.
*/

#ifndef _CVMAIN_H_
#define _CVMAIN_H_ 1

#include "cvapi.h"
#include "cvinternal.h"
#include <platform.h>

typedef struct _rpb_struct
{
   uint32_t magic;              /* magic should be set to 0x45F2D99A */
   uint32_t length;     /* Length of RPB structure */
   uint32_t sbiAddress; /* SBI Address */
   uint16_t     devSecurityConfig;
   uint16_t SBIRevisionID;
   uint32_t CustomerID;
   uint16_t     ProductID;                                              /* Product ID */
   uint16_t     CustomerRevisionID;                             /* Customer Revision ID */
   uint32_t ErrorStatus;
   uint32_t SBLState;   /* Bootstrap State */
   uint32_t errCnt;             /* error counter */
   uint32_t     failedAuthAttempts;
   uint32_t failedRecoveryAttempts;
   uint32_t ExceptionState; /* Exception State */
   uint32_t     TOCArrayPtr;
   uint32_t TOCEntryInfo;
   uint32_t encDeviceType; /* Encoded Device Type */
   uint32_t authType; /* Authentication Type */
   uint32_t bootSrc; /* Boot Source */
   uint32_t powerStatus; /* Power Status */
} RPB_STRUCT;

#endif				       /* end _CVMAIN_H_ */
