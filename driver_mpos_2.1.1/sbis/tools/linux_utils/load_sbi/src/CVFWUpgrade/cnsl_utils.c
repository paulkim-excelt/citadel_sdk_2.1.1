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
/**********************************************************
    includes
**********************************************************/
#include "cvapi.h"      //For defines such as uint32_t etc.
#include "cnsl_utils.h"
#include "ushupgrade.h"
#include <termios.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif


/**********************************************************
    defines
**********************************************************/

#define NumPrevCmds				8
#define CmdLen					128
#define CMD_STR_FULL()          ((lastCmdIdx==firstCmdIdx)?TRUE:FALSE)
#define CMD_STR_NOT_EMPTY()     ((lastCmdIdx!=firstCmdIdx)?TRUE:FALSE)
#define CMD_STR_IDX_INC(_i)     ((_i+1)%NumPrevCmds)
#define CMD_STR_IDX_DEC(_i)     ((_i)?(_i-1):(NumPrevCmds-1))
#define LogStrLen				1024


/**********************************************************
    vars
**********************************************************/

uint32_t            cnsl_info_level=CNSL_DFLT;
int                 fillOneScreen = 0;
int                 outputcnt = 0;
FILE               *pInputSrcFn=NULL;
cnsl_input_src_t    gInputSrc=CNSL_INPUT_SRC_KEYBOARD;
int                 lastCmdIdx=0, firstCmdIdx=0;
char                prevCmds[NumPrevCmds][CmdLen];
char                logfilename[32];
FILE               *logFp=NULL;
char                logStr[LogStrLen];
char                gCmdLine[] = "CVFWUpgrade";


/**********************************************************
    routine prototypes
**********************************************************/

char *date_time_stamp(void);


/**********************************************************
    routine definitions
**********************************************************/

uint32_t cnslGetCnslLevel( void )
{
    return cnsl_info_level;
}


void cnslSetCnslLevel( uint32_t cnslLevel )
{
    cnsl_info_level = cnslLevel;
}


uint32_t cnslCnslLevel( uint32_t cnslLevel )
{
    return ( cnsl_info_level & cnslLevel );
}

int cnslOpenLogfile( char *filename )
{
    if (logFp) {
        cnslInfo( CNSL_ERR, "logfile already open %s\n" );
        return 1;
    }

    if ((logFp = fopen(filename, "a+")) == NULL) {
        cnslInfo( CNSL_ERR, "Could not open logfile %s\n", filename );
        return 1;
    }
    fseek( logFp, 0, SEEK_END );

	if (ftell(logFp))
        fprintf(logFp, "\n");

	fprintf(logFp, "%s: Command - %s\n", date_time_stamp(), gCmdLine);
    fflush(logFp);
    return 0;
}


void cnslCloseLogfile(void)
{
    // close logfile
    if (logFp) {
        fclose(logFp);
        logFp=NULL;
    }
}


void cnslInfo( uint32_t level, char *fmt, ... )
{
    va_list ap;

    if ( level & cnsl_info_level) {
        va_start(ap, fmt);
        memset(logStr, 0, LogStrLen);
        vsnprintf( logStr, LogStrLen, fmt, ap);
        printf("%s", logStr);
        fflush(stdout);
        if (logFp) {
            fprintf(logFp, "%s: %s", date_time_stamp(), logStr);
            fflush(logFp);
        }
        va_end(ap);

        if ( strchr( fmt, '\n' ) != NULL ) {
            // only count CRs
            if ( fillOneScreen==1 ) {
                outputcnt++;
                if ( outputcnt >= 20 ) {
                    printf( "Hit any key to continue\n" );
                    cnslGetCh();
                    outputcnt = 0;
                }
            }
        }
    }
}


void cnslPrintf( char *fmt, ... )
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    if (logFp) {
        vfprintf(logFp, fmt, ap);
        fflush(logFp);
    }
    va_end(ap);
    fflush(stdout);

    if ( strchr( fmt, '\n' ) != NULL ) {
        // only count CRs
        if ( fillOneScreen==1 ) {
            outputcnt++;
            if ( outputcnt >= 20 ) {
                printf( "Hit any key to continue\n" );
                cnslGetCh();
                outputcnt = 0;
            }
        }
    }
}


uint8_t cnslGetCh( void )
{
    return getchar();
}


void cnslDelChar( void )
{
    putchar(0x08);
    putchar(0x20);
    putchar(0x08);
}


void cnslDelLine( int num )
{
    int idx;
    for (idx=0; idx<num; idx++ )
        cnslDelChar();
}


int cnslPause( void )
{
    cnslPrintf("Hit any key to continue...\n");
    return cnslGetCh();
}


char *cnslGetStr( char *pOutStr, int strBufSize )
{
    uint8_t         key;
    uint8_t         charIdx=0;
    char            cmdStr[CmdLen];
    int             prevIdx=lastCmdIdx;

    if ( gInputSrc == CNSL_INPUT_SRC_KEYBOARD ) {
        // keyboard input
        memset( cmdStr, 0, CmdLen );
        while ( 1 ) {
            key = cnslGetCh();
            if ( key == 0xd ) {
                // CR do something
                cnslPrintf( "\n" );
                if ( !charIdx )
                    return NULL;

                if ( strcmp( prevCmds[CMD_STR_IDX_DEC(lastCmdIdx)], cmdStr ) != 0 ) {
                    memcpy( prevCmds[lastCmdIdx], cmdStr, CmdLen );
                    lastCmdIdx = CMD_STR_IDX_INC(lastCmdIdx);
                    if ( CMD_STR_FULL() )
                        firstCmdIdx = CMD_STR_IDX_INC(firstCmdIdx);
                }

                strcpy( pOutStr, cmdStr );
                return pOutStr;
            }
            if ( key == 0x0 ) {
                // special char get other char
                key = cnslGetCh();
                if ( key == 0x48 ) {
                    // up arrow
                    if ( CMD_STR_NOT_EMPTY() ) {
                        // there are previous commands
                        if ( prevIdx != firstCmdIdx ) {
                            // not first cmd point to previous cmd
                            prevIdx = CMD_STR_IDX_DEC(prevIdx);
                        }

                        memset( cmdStr, 0, CmdLen );
                        strcpy( cmdStr, prevCmds[prevIdx] );
                        cnslDelLine( charIdx );
                        cnslPrintf( "%s", cmdStr );
                        charIdx = (uint8_t)strlen( cmdStr );
                    }
                }
                else if ( key == 0x50 ) {
                    // down arrow
                    if ( CMD_STR_NOT_EMPTY() ) {
                        // there are previous commands
                        if ( prevIdx != lastCmdIdx ) {
                            // not first cmd point to previous cmd
                            prevIdx = CMD_STR_IDX_INC(prevIdx);
                        }
                        memset( cmdStr, 0, CmdLen );
                        if ( prevIdx != lastCmdIdx ) {
                            strcpy( cmdStr, prevCmds[prevIdx] );   
                        }

                        cnslDelLine( charIdx );
                        cnslPrintf( "%s", cmdStr );
                        charIdx = (uint8_t)strlen( cmdStr );
                    }
                }
                else if ( key == 0x4b ) {
                    // back arrow
                    if ( charIdx > 0 ) {
                        cnslDelChar();
                        charIdx--;
                        cmdStr[charIdx] = 0;
                    }
                }
            }
            else if ( key == 0x8 ) {
                // backspace
                if ( charIdx > 0 ) {
                    cnslDelChar();
                    charIdx--;
                    cmdStr[charIdx] = 0;
                }
            }
            else if ( (charIdx<(CmdLen-1)) && (key>=0x20) && (key<=0x7f) ) {
                // if string not full - output char
                cnslPrintf("%c", key);
                cmdStr[charIdx++] = key;
            }
        }
    }

    return NULL;
}

void cnslFlush( void )
{
    fflush( stdout);
}

void cnslFillOneScreenOn( void )
{
    fillOneScreen = 1;
    outputcnt = 0;
}

void cnslFillOneScreenOff( void )
{
    fillOneScreen = 0;
}


void cnslMemDisplay32( void *ptr, uint32_t len )
{
    uint32_t *pdword=(uint32_t*)ptr, idx, data;

    for (idx=0; idx<len; idx++) {
       if ( (idx%4) == 0 )
           cnslInfo( CNSL_DATA, "0x%x ", pdword );
       data = *pdword++; 
       cnslInfo( CNSL_DATA, " 0x%08x", data );
       if ( (idx%4) == 3 )
           cnslInfo( CNSL_DATA, "\n" );
    }
    if ( (idx%4) != 0 )
       cnslInfo( CNSL_DATA, "\n" );
}


void cnslMemDisplay8( void *ptr, uint32_t len )
{
    uint8_t     *pdword=(uint8_t*)ptr, data;
    uint32_t    idx;

    for (idx=0; idx<len; idx++) {
       if ( (idx%8) == 0 )
           cnslInfo( CNSL_DATA, "0x%x ", pdword );
       data = *pdword++; 
       cnslInfo( CNSL_DATA, " %02x", data );
       if ( (idx%8) == 7 )
           cnslInfo( CNSL_DATA, "\n" );
    }
    if ( (idx%8) != 0 )
       cnslInfo( CNSL_DATA, "\n" );
}

/* 
* Function:
*       data_time_stamp
* Purpose:
*       This routine returns a pointer to the date and time stamp string
* Parameters:
*       n/a
* ReturnValue:
*       char*       - pointer to the date and time stamp string.
*/
char *date_time_stamp(void)
{
    static char str[80];
    time_t lt;
    struct tm *pCurTime;

    lt = time(NULL);

    pCurTime = localtime(&lt);

    strftime(str, 80, "%A %B %d, %Y %X", pCurTime);
    return str;
}

/* 
* Function:
*       cnsToLower
* Purpose:
*       Converts string to lower case string
* Parameters:
*       str - input and output
* ReturnValue:
*       bool - true if successful, false is not
*/
bool cnslToLower(char *str)
{
    int i;

    if(!str) 
    {
        cnslInfo(CNSL_ERR, "cnsToLower() called with null string pointer\n");
        return false;
    }

    for (i = 0; str[i]; i++)
    {
        str[i] = tolower((unsigned char)str[i]);
    }

    return true;
}

/* 
* Function:
*       cnslSleepMS
* Purpose:
*       Sleep in unit of milliseconds
* Parameters:
*       milliseconds - number of milliseconds to sleep
* ReturnValue:
*       None
*/
void cnslSleepMS(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}

/* 
* Function:
*       cnslSleepWTick
* Purpose:
*       Sleep in unit of seconds and print "." during every 1 sec
* Parameters:
*       sleepSeconds - number of seconds to sleep
* ReturnValue:
*       None
*/
void cnslSleepWTick(int sleepSeconds)
{
    int i;

    cnslPrintf("\n");
    for(i=0; i<sleepSeconds; i++)
    {
        cnslPrintf(".");
        cnslSleepMS(1000);
    }
    cnslPrintf("\n");

    return;
}
