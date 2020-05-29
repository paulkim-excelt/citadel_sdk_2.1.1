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

#include "cvapi.h"
#include "cvinternal.h"
#include "usbdevice.h"
#include "nfpclass.h"
#include "nfpmemblock.h"
#include "volatile_mem.h"
#include "console.h"
#include "ushx_api.h"
#include "phx_osl.h"
#include "nci.h"
#include "rfid_hw_api.h"
#include "bcm_types.h"
#include "sched_cmd.h"

extern tNFPCLASS_MEM_BLOCK nfpclass_mem_block;

#define NFPClassInterface	(pnfpclass_mem_block->NFPClassInterface)
#define wRcvLength    		(pnfpclass_mem_block->wRcvLength)
#define bcmdRcvd 			(pnfpclass_mem_block->bcmdRcvd)
#define bCurrentPktProcessing	(pnfpclass_mem_block->bCurrentPktProcessing)

#define RF_DISCOVER_COMMAND_LENGTH 21
int usbnfp_nci_send_data(uint8_t *sendBuf, uint16_t slen);
void nci_msg_parsing(uint8_t *sendBuf, uint16_t slen);


// Prepares RX Endpoint to receive a new BulkOut message and terminate CV command processing
cv_status usbNFPCommandComplete(uint8_t *pBuffer)
{
	//bcvCmdInProgress = FALSE;
	//bcmdRcvd = FALSE;
	//pnfpclass_mem_block->command_complete_called = 1;
	recvEndpointData(NFPClassInterface.BulkOutEp, pBuffer,
			NFPClassInterface.BulkOutEp->maxPktSize);
	return CV_SUCCESS;
}

bool usbNFPBulkInComplete(int ticks)
{
	bool status = 0;
	uint32_t startTime;

	printf("nfpBIC\n");

	get_ms_ticks(&startTime);
	do {
		status = usbTransferComplete(NFPClassInterface.BulkInEp, (ticks/1000));
		if (!status) {
			//printf("yielding ... %u \n", cvGetDeltaTime(startTime) );
			PROCESS_YIELD_CMDS();
		} else {
			break;
		}
	} while ((cvGetDeltaTime(startTime) < ticks) && canUsbWait());

	if (!status) {
		printf("nfpusbd: CvBulkIn timeout (%d)\n", ticks);
		usbd_dump_endpoint(NFPClassInterface.BulkInEp);
	}

	return status;
}

bool usbNFPIntrInComplete(int ticks)
{
	bool status = 0;
	uint32_t startTime;

	//printf("nfpIIC\n");

	get_ms_ticks(&startTime);
	do {
		status = usbTransferComplete(NFPClassInterface.IntrInEp, (ticks/1000));
		if (!status) {
			//printf("yielding... %u \n", cvGetDeltaTime(startTime) );
			PROCESS_YIELD_CMDS();
		} else {
			break;
		}
	} while ((cvGetDeltaTime(startTime) < ticks) && canUsbWait());

	if (!status) {
		printf("nfpusb: IntrIn timeout (%d)\n", ticks);
		usbd_dump_endpoint(NFPClassInterface.IntrInEp);
	}

	return status;
}

// data from device to host send is done, nonblock function, bulk out
bool usbNFPPollCmdSent(void)
{
	return is_avail(NFPClassInterface.BulkOutEp);
}

bool usbNFPPollIntrSent(void)
{
	return is_avail(NFPClassInterface.IntrInEp);
}

// send interrupt in data to host
cv_status usbNFPInterruptIn(cv_usb_interrupt *interrupt)
{
	sendEndpointData(NFPClassInterface.IntrInEp, (uint8_t *)interrupt, sizeof(cv_usb_interrupt));
	return CV_SUCCESS;
}

/* Called by NFC scheduler task */
bool usbNFPSendCmdResponse(void)
{
	int i;
	bool status = 0;
	uint16_t tmp_len;
	uint8_t  *tmp_buffer;

//	printf("%s:pnfpclass_mem_block->pktBeingProcessed=%d,len =%d \n",__FUNCTION__,pnfpclass_mem_block->pktBeingProcessed,
//		pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktBeingProcessed]); 

	ENTER_CRITICAL();
	if(pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktBeingProcessed]==0)
	{
		EXIT_CRITICAL();
		printf("%s: No data to be processed \n", __FUNCTION__);
		return status;
	}
	EXIT_CRITICAL();

	tmp_len = pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktBeingProcessed];
	tmp_buffer = &(pnfpclass_mem_block->nfpRxbuffer[pnfpclass_mem_block->pktBeingProcessed][0]);

    if (tmp_len > NFP_RX_TX_BUFFER_SIZE)
    {
        printf("%s: Packet length of %d exceeds buffer size \n", __FUNCTION__, tmp_len);
        tmp_len = NFP_RX_TX_BUFFER_SIZE;
    }

	/* Debug logs */
	if(is_nfc_pkt_log_enabled())
	{
        printf("%s: to NFCC len=%d, hdr:", __FUNCTION__, tmp_len);
        for (i=0; i < tmp_len && i < 5; i++)
            printf(" %02x", tmp_buffer[i]);
        printf("\n(BCMIX): ");
        for (i = 5; i < tmp_len; i++)
            printf("%02x", tmp_buffer[i]);
        printf("\n");
	}

	/* If RF_DISCOVER command is sent resume RFID handling */
	if (tmp_len > 7 && tmp_buffer[5] == 0x21 && tmp_buffer[6] == 0x03)
	{
        if (pnci_mem_block->bSuspendedRFIDAndNeedToResume)
        {
            printf("rfid_resume\n");
            rfid_handle_resume();
            pnci_mem_block->bSuspendedRFIDAndNeedToResume = 0;
        }
        else
        {
            printf("RF_DISCOVER but not suspended\n");
        }

        /* Save the RF Discover parameters for later use by nci_discover() function */
        if (tmp_buffer[7] <= MAX_RF_DISCOVER_PARAM_SIZE)
        {
            pnci_mem_block->nRfDiscoverParamSize = tmp_buffer[7];
            memcpy(&pnci_mem_block->aRfDiscoverParam[0], &tmp_buffer[8], pnci_mem_block->nRfDiscoverParamSize);
            printf("Saving %d bytes of RF_DISCOVER params\n", pnci_mem_block->nRfDiscoverParamSize);
        }
	}

    // Send command to 20793 and queue response to usb host
    usbnfp_nci_send_data(tmp_buffer, tmp_len);

	ENTER_CRITICAL();
	pnfpclass_mem_block->nfpRxbufferLength[pnfpclass_mem_block->pktBeingProcessed]=0;
	//memset(&(pnfpclass_mem_block->nfpRxbuffer[pnfpclass_mem_block->pktBeingProcessed][0]),0,NFP_RX_TX_BUFFER_SIZE);
	EXIT_CRITICAL();

	pnfpclass_mem_block->pktBeingProcessed = (pnfpclass_mem_block->pktBeingProcessed+1)%NFP_RX_QUEUE_LEN;

	/* Turn SPI interrupts on */
	nci_enable_isr();

	return status;
}

bool usbNFPBulkSend(uint8_t *pBuffer, uint32_t len, int ticks)
{
	bool result=0;

	/* Debug logs */
	if(is_nfc_pkt_log_enabled())
	{
		printf("usbNFPBulkSend: %d bytes\n", len);
	}
#if 0
	if (pnfpclass_mem_block->bUsbIntEnabled)
	{
		sendEndpointData(NFPClassInterface.IntrInEp, (uint8_t *)&len, sizeof(uint32_t));
		result = usbNFPIntrInComplete(ticks);
		if (!result) {
			usbd_cancel(NFPClassInterface.IntrInEp);
			printf("usbNFPSend: IntrIn timeout\n");
			return 0;
		}
	}
#endif
	result = usbTransferComplete(NFPClassInterface.BulkInEp, ticks);
    if (!result)
        printf("usbNFPSend: USB endpoint not available after %d ticks\n", ticks);

	if(pnfpclass_mem_block->nfpTxBuffer != pBuffer)
	   	memcpy(pnfpclass_mem_block->nfpTxBuffer, pBuffer, len);
	

	sendEndpointData(NFPClassInterface.BulkInEp, pnfpclass_mem_block->nfpTxBuffer, len);
	/* allow this transfer to complete */
	result = usbTransferComplete(NFPClassInterface.BulkInEp, ticks);
	if(!result)
	{
		printf("usbNFPBulkSend: doing resume again \n");
		usbdResume();
		result = usbTransferComplete(NFPClassInterface.BulkInEp, ticks);
		if(!result)
		{
		   printf("usbNFPBulkSend: BulkIn timeout (%d)\n", ticks);
		   usbd_cancel(NFPClassInterface.BulkInEp);
		}
	}
	return result;
}


/*---------------------------------------------------------------
 * Name    : usbnfp_nci_send_data
 * Purpose : Sends nci data to 2079X
 * Input   : sendBuf:  send buffer
 *           slen:	   length of send buffer
 * Return  : various RFID_STATUS
 * Note    :
 *--------------------------------------------------------------*/
int usbnfp_nci_send_data(uint8_t *sendBuf, uint16_t slen)
{
    uint32_t timeout;
    uint32_t index;
    uint8_t  octet;
    uint16_t i;
    uint8_t *rsp = pnfpclass_mem_block->nfpTxBuffer;
	rfid_nci_hw* pHW = (rfid_nci_hw *)VOLATILE_MEM_PTR->rfid_hw_nci_ptr;

    /* Debug logs */
    if(is_nfc_pkt_log_enabled())
    {
        nci_msg_parsing(sendBuf,slen);
    }

    /* Send deactivation to HID stack if we receive a deactivate cmd from NFC host driver for T1/T2/T3 cards */
	if(pHW->bnotifiedNfcActivation == TRUE && parse_rf_deactivate_cmd_msg(sendBuf) == TRUE)    
	{        
		queue_cmd(SCHED_HID_CARD_DEACTIV_NOTIFY, QUEUE_FROM_TASK, NULL);        
		pHW->bnotifiedNfcActivation = FALSE;    
	}


    timeout = 0;
    nci_spi_cs_low();
    while ((timeout++ < NFC_SPI_INT_TIMEOUT/3) && (nci_spi_int_state() != 0)) {}
	if (timeout >= NFC_SPI_INT_TIMEOUT/3) {
        printf("%s: failed before write\n",__func__);
		nci_spi_cs_high();
        return 1;			/* Temporary */
    }
	/* Send buffer */
    for (i=0;i<slen;i++) {
        nci_spi_byte_operation(sendBuf[i], &octet);
    }

    timeout = 0;
    while ((timeout++ < NFC_SPI_INT_TIMEOUT) && (nci_spi_int_state() == 0)) {}
	if (timeout >= NFC_SPI_INT_TIMEOUT) {
        printf("%s: failed after write\n",__func__);
		nci_spi_cs_high();
        return 1;
    }
	nci_spi_cs_high();
	
    return 0;
}

/*---------------------------------------------------------------
 * Name    : usbnfp_nci_receive_data
 * Purpose : Receives nci data from 2079X
 * Input   : None
 * Return  : various RFID_STATUS
 * Note    :
 *--------------------------------------------------------------*/
int usbnfp_nci_receive_data()
{
    uint32_t timeout;
	uint32_t index;
    uint8_t  octet;
	uint8_t *rsp = pnfpclass_mem_block->nfpTxBuffer;
	uint16_t *rlen;
    uint16_t i;	
	
	/* Resume usb device if selectively suspended */
	usbdResume();
	
	/* Check for spi_int active */
	timeout = 0;	
    while ((timeout++ < NFC_SPI_INT_TIMEOUT) && (nci_spi_int_state() != 0)) {}
	if (timeout >= NFC_SPI_INT_TIMEOUT) {
        printf("%s: failed before read\n",__func__);
        return 1;
    }

	/* Now read */
	nci_spi_cs_low();

    index = 0;
    // Send a direct read, then input all the bytes until spi_int goes high
    nci_spi_byte_operation(DIRECT_READ_COMMAND, &rsp[index++]);

    while ((nci_spi_int_state() == 0) && (index < NFP_MAX_PACKET_SIZE)) {
        nci_spi_byte_operation(0x00, &rsp[index++]);
    }

    nci_spi_cs_high();

    /* Debug logs */
    if(is_nfc_pkt_log_enabled())
    {
        printf("%s:NCI Response Length: %d\n",__FUNCTION__,index);
        printf("NCI Response:");
        for (i=0; i < index; i++)
        {
            printf(" %02x", rsp[i]);
        }
        printf("...\n");	
    }
	
	usbNFPBulkSend(pnfpclass_mem_block->nfpTxBuffer, index, NFP_REQUEST_TIMEOUT);

	return 0;
}

/* Common notification routine shared between RFID and NFC */
int nci_notification_common(uint8_t *rsp, uint16_t *rlen)
{
    uint32_t timeout;
    uint32_t index;
//    uint8_t  octet;

    timeout = 0;
    while ((timeout++ < NFC_SPI_INT_TIMEOUT) && (nci_spi_int_state() != 0)) {}
    nci_spi_cs_low();

    index = 0;
    /*nci_spi_byte_operation(DIRECT_READ_COMMAND, &octet);
    nci_spi_byte_operation(0x00, &octet);*/  // Direct Read
	
    // Send a direct read, then input all the bytes until spi_int goes high
    nci_spi_byte_operation(DIRECT_READ_COMMAND, &rsp[index++]);
	
    while ((nci_spi_int_state() == 0) && (index < *rlen)) {
        nci_spi_byte_operation(0x00, &rsp[index++]);
    }

    nci_spi_cs_high();

    *rlen = index;

    if (timeout >= NFC_SPI_INT_TIMEOUT) {
        // printf("%s: fail\n",__func__);
        return 1;
    }
    return 0;
}





/* Return true if the msg is NCI CMD message */
int nci_msg_is_cmd_msg(uint8_t *sendBuf)
{
	if(((sendBuf[NCI_BUFF_OFFSET] & NCI_MT_MASK) >> NCI_MT_SHIFT)  == NCI_MT_CMD ) 
	{
	  return TRUE;	
	}
	return FALSE;
}

int nci_msg_is_frag_msg(uint8_t *sendBuf)
{
	if(((sendBuf[NCI_BUFF_OFFSET] & NCI_PBF_MASK) >> NCI_PBF_SHIFT) == NCI_PBF_ST_CONT ) 
	{
	  return TRUE;	
	}
	return FALSE;
}


void nci_msg_parsing(uint8_t *sendBuf, uint16_t slen)
{
	uint8_t frag=0;
	uint8_t display_str[][16] = {"UNKNOWN","CORE_RESET","CORE_INIT","CORE_SET_CONFIG","CORE_GET_CONFIG",
								  "RF_DISCOVER_MAP","RF_DISCOVER","RF_INTF_ACTIV","RF_DEACTIV",
								"BLD_INFO","XTAL_CMD","GET_PATCH","PATCH_DOWNLD","EEPROM_RW","RF_T3T_POLL"};

	if(((sendBuf[NCI_BUFF_OFFSET] & NCI_PBF_MASK) >> NCI_PBF_SHIFT) == NCI_PBF_ST_CONT) 
	{
		frag =1;
	}
	/* First 3 high bytes in the packet determine the Packet type */
	if(((sendBuf[NCI_BUFF_OFFSET] & NCI_MT_MASK) >> NCI_MT_SHIFT)  == NCI_MT_DATA) 
	{
		printf("Recv Data Msg of Length %d bytes (frag= %x) from Host Driver \n",slen,frag);
	}
	else if (((sendBuf[NCI_BUFF_OFFSET] & NCI_MT_MASK) >> NCI_MT_SHIFT)  == NCI_MT_CMD) 
	{
		uint8_t gid = (sendBuf[NCI_BUFF_OFFSET] & NCI_GID_MASK) >> NCI_GID_SHIFT;
		uint8_t oid = (sendBuf[NCI_BUFF_OFFSET+1] & NCI_OID_MASK) >> NCI_OID_SHIFT;
		uint32_t idx=0; 
		if(gid == 0 && oid == 0) 
			idx = 1;
		if(gid == 0 && oid == 1) 
			idx = 2;
		if(gid == 0 && oid == 2) 
			idx = 3;
		if(gid == 0 && oid == 3) 
			idx = 4;
		if(gid == 1 && oid == 0) 
			idx = 5;
		if(gid == 1 && oid == 3) 
			idx = 6;
		if(gid == 1 && oid == 5) 
			idx = 7;
		if(gid == 1 && oid == 6) 
			idx = 8;
		if(gid == 0xF && oid == 0x4) 
			idx = 9;
		if(gid == 0xF && oid == 0x1D) 
			idx = 10;
		if(gid == 0xF && oid == 0x2D) 
			idx = 11;
		if(gid == 0xF && oid == 0x2E) 
			idx = 12;
		if(gid == 0xF && oid == 0x29) 
			idx = 13;
		if(gid == 1 && oid == 8) 
			idx = 14;
		printf("Recv CMD %s Msg of Length %d bytes (frag= %x) from Host Driver (gid=%x and oid= %x) \n",display_str[idx],slen,frag,gid,oid);
	}
}

