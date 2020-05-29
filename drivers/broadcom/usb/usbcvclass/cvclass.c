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

#include "cvclass.h"
#include "usbdevice.h"
#include "usbregisters.h"
#include "cvmemblock.h"
#ifdef USH_BOOTROM  /*AAI */
#include "volatile_mem.h"
#endif
#include "cvapi.h"
#include "cvinternal.h"
#ifdef USH_BOOTROM  /*AAI */
#include "sched_cmd.h"
#include "phx_upgrade.h"
#include "cvmain.h"
#endif
#include "console.h"

#ifdef USH_BOOTROM /*AAI */
#include <toolchain/gcc.h>
#endif

#include "stub.h"


#define CV_CMD_STATE  0
#define CV_DAT_STATE  1
#define CV_EXE_STATE  2

#ifdef USH_BOOTROM  /*AAI */
//#define pcvclass_mem_block ((tCVCLASS_MEM_BLOCK *)VOLATILE_MEM_PTR->cvclass_mem_block)
#else
extern tCVCLASS_MEM_BLOCK cvclass_mem_block;
#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)&cvclass_mem_block)
#endif

#define CVClassInterface	(pcvclass_mem_block->CVClassInterface)
#define wRcvLength		(pcvclass_mem_block->wRcvLength)
#define bRcvStatus		(pcvclass_mem_block->bRcvStatus)
#define dwDataLen		(pcvclass_mem_block->dwDataLen)
#define dwlength		(pcvclass_mem_block->dwlength)
#define bcmdRcvd		(pcvclass_mem_block->bcmdRcvd)
#define bcvCmdInProgress	(pcvclass_mem_block->bcvCmdInProgress)
extern int cvIsCmdValid(void *buf, uint32_t len, uint32_t complete);

void
cvclass_dump_cmd(uint8_t *buf, int len)
{
	int i;
	ARG_UNUSED(buf);

	printf("CV: cmd dump (%d xferred)", len);
	for (i = 0; i < len; ++i)
		printf("%c%02x", (i%16) ? ' ' : '\n', buf[i]);
	printf("\n");
}

void CvClassCallBackHandler(EPSTATUS *ep)
{
	uint32_t rx_len;
	unsigned int xTaskWoken = FALSE;

#ifndef USH_BOOTROM  /* SBI */
	ARG_UNUSED(xTaskWoken);
#endif

	printf("CvClassCallBackHandler bRcvStatus:%d\n",bRcvStatus);

	switch (bRcvStatus) {
	case CV_CMD_STATE:
		/* first part of command received, flush any pending IN transfers */
		usbd_cancel(CVClassInterface.BulkInEp);
		usbd_cancel(CVClassInterface.IntrInEp);

		pcvclass_mem_block->cmd = ep->dma.pBuffer;
		rx_len = (((uint8_t *) ep->dma.pXfr)) - ((uint8_t *) pcvclass_mem_block->cmd);
		dwDataLen = ((cv_encap_command *)ep->dma.pBuffer)->transportLen;
		if (!cvIsCmdValid(ep->dma.pBuffer, rx_len, 0)) {
			/* signal error and prep for next command */
			printf("CV: bad command detected at header\n");
			cvclass_dump_cmd((uint8_t *) ep->dma.pBuffer, rx_len); 
			recvEndpointData(ep, (uint8_t*)CV_SECURE_IO_COMMAND_BUF, ep->maxPktSize);
			break;
		}

		if ((dwDataLen == 0) || (ep->dma.transferred < ep->maxPktSize)) {
			bRcvStatus = CV_EXE_STATE;
			break;
		}
		bRcvStatus = CV_DAT_STATE;
		dwlength = dwDataLen - ep->maxPktSize;
		if (dwlength > 0) {
			pcvclass_mem_block->state = cvclass_state_header;
			recvEndpointData(ep, ep->dma.pBuffer + ep->maxPktSize, dwlength);
			return;
		}
		/* fall through */
	case CV_DAT_STATE:
		bRcvStatus = CV_EXE_STATE;
		pcvclass_mem_block->state = cvclass_state_data;
		break;
	default:;
	}
	if (bRcvStatus == CV_EXE_STATE)	{
		cv_encap_command *cmd = (cv_encap_command *)pcvclass_mem_block->cmd;
		/*
		 * Schedule CV_CMD to queue only if transport type is CV_TRANS_TYPE_ENCAPSULATED, 
		 * other transport types are handled internally by CV
		 */
		
		/* Basic CV sanity test. Did we get the data the command claims? */
		rx_len = (((uint8_t *) ep->dma.pXfr)) - ((uint8_t *) pcvclass_mem_block->cmd);
		if (cmd->transportType == CV_TRANS_TYPE_ENCAPSULATED && cmd->commandID != CV_CALLBACK_STATUS) {
			if (!cvIsCmdValid(cmd, rx_len, 1)) {
				printf("CV: bad command detected at body\n");
				cvclass_dump_cmd((uint8_t *) cmd, rx_len);
				recvEndpointData(ep, (uint8_t*)CV_SECURE_IO_COMMAND_BUF, ep->maxPktSize);
			}
			else {
#ifdef CV_TRACE
				printf("cvCmd: 0x%x %d bytes @ %p\n", cmd->commandID, rx_len, cmd);
#endif
#ifndef USH_BOOTROM  /* SBI */
				cvManager();
#else
				xTaskWoken = queue_cmd(SCHED_CV_CMD, QUEUE_FROM_ISR, xTaskWoken);
				pcvclass_mem_block->state = cvclass_state_queued;

				/* If a task was woken by either a character being received or a character
				being transmitted then we may need to switch to another task. */
				SCHEDULE_FROM_ISR(xTaskWoken);
#endif
				bcvCmdInProgress = TRUE;
				// No need to block the CV endpoint till the completion, there is already a lock in the CV host driver to block simultaneous requests
				//recvEndpointData(ep, CV_SECURE_IO_COMMAND_BUF, ep->maxPktSize);

			}
		}
		else {	/* Prepare RX endpoint when CV is not processing a command */
#ifdef CV_TRACE
			printf("cvData 0x%x %d bytes @ %p\n", cmd->commandID, rx_len, cmd);
#endif
			if (!bcvCmdInProgress)
				recvEndpointData(ep, (uint8_t*)CV_SECURE_IO_COMMAND_BUF, ep->maxPktSize);
		}
		bRcvStatus = CV_CMD_STATE;
		bcmdRcvd = TRUE;
	}	
}

void cv_eventCallback(EPSTATUS *ep, usbd_event_t event)
{
	ARG_UNUSED(ep);
	ARG_UNUSED(event);
	printf("CV ep%d-%s event %d\n", ep->number, is_in(ep) ? "in" : "out", event);
}

void InitialCvClass(uint8_t *pbuffer, uint16_t wlen)
{
	bcvCmdInProgress = FALSE;
	/* Set initial state to CMD_STATE */
	bRcvStatus = CV_CMD_STATE;
	pcvclass_mem_block->state = cvclass_state_command_end;
	recvEndpointData(CVClassInterface.BulkOutEp, pbuffer, wlen);
}

/*
 * Called from the main task thread by USBD after a reset has been detected
 * and the HW & endpoint data structures have been reinitialized.
 * Reset any machnine state necessary, and initialize the endpoints as needed.
 */
void cv_usbd_reset(void)
{
	printf("cv_usbd_reset \n");

	memset(&CVClassInterface, 0, sizeof(CVClassInterface));
	
	/* acquire endpoint handles */
	CVClassInterface.BulkInEp =  usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | IN_DIR);
	CVClassInterface.BulkOutEp = usbd_find_endpoint(USBD_CV_INTERFACE, BULK_TYPE | OUT_DIR);
	CVClassInterface.IntrInEp =  usbd_find_endpoint(USBD_CV_INTERFACE, INTR_TYPE | IN_DIR);
	CVClassInterface.BulkOutEp->appCallback = CvClassCallBackHandler;
	CVClassInterface.BulkOutEp->eventCallback = cv_eventCallback;

	/* initialize the cv class after setting up the bulk endpoints */
	InitialCvClass((uint8_t*)CV_SECURE_IO_COMMAND_BUF, CVClassInterface.BulkOutEp->maxPktSize);


}

void
cvclass_command_start()
{
	pcvclass_mem_block->state = cvclass_state_command_start;
	pcvclass_mem_block->command_complete_called = 0;
}

void
cvclass_command_end()
{
	if (!pcvclass_mem_block->command_complete_called)
		printf("CV class ended w/o calling command complete\n");
	pcvclass_mem_block->state = cvclass_state_command_end;
}
