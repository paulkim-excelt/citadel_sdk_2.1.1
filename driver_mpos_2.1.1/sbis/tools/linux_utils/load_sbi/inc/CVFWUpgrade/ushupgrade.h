/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/
#ifndef __WPE_USH_DIAG_H__
#define __WPE_USH_DIAG_H__

/**********************************************************
    defines
**********************************************************/

#define MaxFileNamelen  255
#define MaxCvPasswordLen    32

#define MaxCmdLineLen	256

#define VAL_CID1		1
#define VAL_CID7		7
#define VAL_CIDFE		0xFE
     
#define OP_CVVERSION	0x00000001
#define OP_CID1			0x00000002
#define OP_CID7			0x00000004
#define OP_CIDFE		0x00000008

enum ush_return_code_e {
	USH_RC_SUCCESS,				// 0x00
	USH_RC_SUCCESS_RESET,		// 0x01	Successful, but must reset
	USH_RC_CANCELED,			// 0x02 Canceled
	USH_RC_INVALID_PARAMETER,	// 0x03	Invalid parameter
	USH_RC_MISSING_PARAMETER,	// 0x04	Missing parameter
	USH_RC_INIT_CHK_ERR,		// 0x05	Failed Antihammering pre check
	USH_RC_FILE_ERR,			// 0x06	Missing file
	USH_RC_ERROR,				// 0x07	Generic error
	USH_RC_NOUSH_ERR,			// 0x08	Did not detect USH
	USH_RC_AUTH_ERR,			// 0x09	Authentication error
	USH_RC_TPM_ERR,				// 0x0a	TPM Error
	USH_RC_AH_ERR,				// 0x0b	Anti-hammering error
	USH_RC_TASK_ERR,			// 0x0c	Could not kill necessary tasks
	USH_RC_DLL_ERR,				// 0x0d	DLL mismatch
	USH_RC_LOADSBI_ERR,			// 0x0e	Could not load SBI (running from bootstrap)
	USH_RC_CHIP_ID_ERR,			// 0x0f	Unknown chip ID
	USH_RC_PBA_ERR,				// 0x10	Failed to load PBA
	USH_RC_CLR_SCD_ERR,			// 0x11	Failed to clear SCD
	USH_RC_RESET_ERR,			// 0x12	Reset returned error
	USH_RC_NUM_ERR,				// 0x13	USH failed to numerate
	USH_RC_BCM_ERR,				// 0x14	Failed to update BCM
	USH_RC_MISSING_DLL,			// 0x15	Missing TPM DLL
	USH_RC_MISSING_DRV,			// 0x16	Missing TPM Driver
	USH_RC_RFID_PARAM_ERR,		// 0x17	Failed to update RFID parameters
	USH_RC_SYSID_DLL_ERR,		// 0x18 Failed to load SYSID DLL
	USH_RC_TPM_OWNERSHIP_ERR,	// 0x19 No TPM ownership
	USH_RC_FP_PARAM_ERR,		// 0x1a	Failed to update FP parameters
};

enum ush_status_e {
        USH_SUCCESS,                // 0x00
        USH_ERROR,                  // 0x01
        USH_INVALID_PARAMETER,      // 0x02
        USH_INVALID_FILE,           // 0x03
        USH_MISSING_PARAMETER,      // 0x04
        USH_ERROR_USB,              // 0x05
        USH_ERROR_CV,               // 0x06
        USH_TEST_FAILED,            // 0x07
        USH_NO_FIRMWARE,            // 0x08
        USH_ERROR_TPM,              // 0x09
        USH_AUTH_ERR,               // 0x0a
        USH_TPM_ERR,                // 0x0b
        USH_C0_CID1,                // 0x0c
        USH_C0_CID,                 // 0x0d
        USH_AH_ERR,                 // 0x0e
        USH_FW_ERR                  // 0x0f
};

/**********************************************************
    structs
**********************************************************/

typedef struct mainParams_s
{
    uint32_t    op_mode;           // operational mode
    char        cvAdminPassword[MaxCvPasswordLen+1];
} mainParams_t;

/**********************************************************
    vars
**********************************************************/

extern int retStatus;
extern char gCmdLine[];

#endif //__WPE_USH_DIAG_H__
