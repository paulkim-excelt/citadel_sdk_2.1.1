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
#ifndef NFP_MEMBLOCK_H
#define NFP_MEMBLOCK_H

#include "nfpclass.h"

#define NFP_RX_TX_BUFFER_SIZE  264  // Should be more than CV_USB_MAXPACKETSIZE 
#define NFP_RX_QUEUE_LEN		3	

typedef enum {
	nfpclass_state_header,			/* Header received */
	nfpclass_state_data,			/* Data being received */
	nfpclass_state_queued,			/* Command queued */
	nfpclass_state_command_start,	/* Command execution started */
	nfpclass_state_command_end,		/* Command execution ended */
} nfpclass_state_t;

typedef struct NFPCLASS_MEM_BLOCK
{
	tNFPINTERFACE   NFPClassInterface;
	uint8_t  bPktInProgress;				/* Flag to indicate NFP packet being assembled */
	uint32_t dwTotalLen;
	uint32_t bytesReceived;
	uint32_t rxIndex;
	uint8_t  bNCIResponseAvailable;
	uint32_t nciResponseLength;
	uint8_t	__attribute__ ((aligned (4))) nfpbuffer[NFP_RX_TX_BUFFER_SIZE];
	uint8_t	__attribute__ ((aligned (4))) nfpTxBuffer[NFP_RX_TX_BUFFER_SIZE];
	uint8_t	__attribute__ ((aligned (4))) nfpRxbuffer[NFP_RX_QUEUE_LEN][NFP_RX_TX_BUFFER_SIZE];
	uint16_t nfpRxbufferLength[NFP_RX_QUEUE_LEN];
	uint8_t  pktBeingProcessed;
	uint8_t  pktQueueIndex;
	uint8_t	 bUsbIntEnabled;  /* Enable once Interrupt mode is enabled */
} tNFPCLASS_MEM_BLOCK;

extern tNFPCLASS_MEM_BLOCK nfpclass_mem_block;

bool usbNFPBulkSend(uint8_t *pBuffer, uint32_t len, int ticks);
bool usbNFPSendCmdResponse(void);


#endif
