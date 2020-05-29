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

#ifndef __CNSL_UTILS_H__
#define __CNSL_UTILS_H__

/**********************************************************
    includes
**********************************************************/
#include <stdbool.h>

/**********************************************************
    defines
**********************************************************/

/* Bits 27-31, are unmaskable and use these bits
 * to show user data/errors on the stdout.
 */
#define CNSL_DATA       0x0001
#define CNSL_ERR        0x0002
#define CNSL_WARN       0x0004
#define CNSL_DATAV      0x0008      // data verbose
#define CNSL_DBG        0x0010
#define CNSL_DBGREG     0x0020
#define CNSL_DBGMEM     0x0040
#define CNSL_DBGUSB     0x0080
#define CNSL_DFLT       CNSL_DATA|CNSL_ERR|CNSL_WARN
#define CNSL_ALL        CNSL_DATA|CNSL_ERR|CNSL_WARN|CNSL_DBG|CNSL_DBGREG|CNSL_DBGMEM

typedef enum _cnsl_input_src {
    CNSL_INPUT_SRC_KEYBOARD=0,
    CNSL_INPUT_SRC_FILE,
} cnsl_input_src_t;


/**********************************************************
    routine prototypes
**********************************************************/

void cnslSleepWTick(int sleepSeconds);
void cnslSleepMS(int milliseconds); // cross-platform sleep function
uint32_t cnslGetCnslLevel( void );
void cnslSetCnslLevel( uint32_t cnslLevel );
uint32_t cnslCnslLevel( uint32_t cnslLevel );
int cnslOpenLogfile( char *filename );
void cnslCloseLogfile(void);
void cnslInfo( uint32_t level, char *fmt, ... );
void cnslPrintf( char *fmt, ... );
uint8_t cnslGetCh( void );
int cnslPause( void );
char *cnslGetStr( char *pOutStr, int strBufSize );
uint8_t cnslKeyHit( void );
void cnslFlush( void );
void cnslFillOneScreenOn( void );
void cnslFillOneScreenOff( void );
cnsl_input_src_t cnslRetInputSrc( void );
int cnslSelectInputSrc( cnsl_input_src_t src, void* filename );
void cnslMemDisplay32( void *ptr, uint32_t len );
void cnslMemDisplay8( void *ptr, uint32_t len );
bool cnslToLower(char *str);

#endif // __CNSL_UTILS_H__
