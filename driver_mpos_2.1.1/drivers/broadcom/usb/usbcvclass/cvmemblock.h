/*
 *  Copyright 2004
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  Irvine CA 92619-7013
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************/
#ifndef CV_MEMBLOCK_H
#define CV_MEMBLOCK_H

#include "cvclass.h"

typedef enum {
	cvclass_state_header,		/* Header received */
	cvclass_state_data,		/* Data being received */
	cvclass_state_queued,		/* Command queued */
	cvclass_state_command_start,	/* Command execution started */
	cvclass_state_command_end,	/* Command execution ended */
} cvclass_state_t;

typedef struct CVCLASS_MEM_BLOCK
{
	tCVINTERFACE   CVClassInterface;
	uint32_t dwDataLen;
	uint32_t dwlength;
	uint16_t wRcvLength;
	uint8_t  bnotified;
	uint8_t  bRcvStatus;
	uint8_t  bcmdRcvd;			/* Flag to indicate whether requested BulkOut cmd is recvd from host */
	uint8_t  bcvCmdInProgress;	/* Flag to indicate cv command in progress */
	void	*cmd;			/* current command */
	cvclass_state_t	state;		/* current state */
	uint8_t command_complete_called;
} tCVCLASS_MEM_BLOCK;

#endif
