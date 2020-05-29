/*
 * $Copyright Broadcom Corporation$
 *
 */
#include <post_log.h>
#include <usbdevice.h>
#include <usbd_if.h>
#include <platform.h>
#include "protocol.h"
#include <string.h>

#ifdef SBI_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

#define BITS_PER_LONG 		32

#define BUF_SIZE		IPROC_USB_BUF_SIZE
#define RCV_BUFFER_SIZE		(128 * 1024)/* 128K */
#define RCV_BUFFER		IPROC_BTROM_GET_USB_SBI_IMAGE()
#define PER_ONCE_READ_MAX_LEN	RCV_BUFFER_SIZE

/*
 *  command structure
 */
struct usbd_request {
	uint32_t addr;	/* NAND: offset address, NOR: physical address */
	uint32_t param; 	/* flash size or fuse value */
	uint32_t param1; /* reversed */
	cmd_t cmd;	/* flash or fuse command */
};

/*
 * response structure
 */
struct usbd_response {
	uint16_t ack;	/* response ack value */
	uint16_t csum;	/* data checksum */
	uint32_t len;	/* data len */
};

static int32_t protocol_cmd_receive(struct usbd_request*);
static int32_t protocol_cmd_parser(struct usbd_request*);
static void protocol_response_send(uint16_t ack, uint16_t csum, uint32_t len);
static void protocol_response_data_create(uint8_t *buffer,uint16_t ack,
					  uint16_t csum, uint32_t len);

static protocol_download_callback protocol_download_handler = NULL;

void protocol_register_download_callback(protocol_download_callback handler)
{
    protocol_download_handler = handler;
    return;
}

static void dump_buffer(uint8_t *buffer, uint32_t len)
{
	uint32_t i=0;

	for(i=0;i<len;i++){
		DEBUG_LOG("0x%x ",buffer[i]);
		if (i%8 == 7) DEBUG_LOG("\n");
	}
	DEBUG_LOG("\n");
}


/*
 * Function to receive command from channel,
 * fill the command request structure.
 *
 * @req  command request returned to caller
 * 
 * @return 0 successful, less than 0 error
 */
static int32_t protocol_cmd_receive(struct usbd_request *req)
{
/*cmd needn't save */
	uint8_t buffer[USBD_COMMAND_LEN];
	uint8_t response[USBD_RESPONSE_LEN];
	cv_f_sbi_hdr_t *hdr = (cv_f_sbi_hdr_t *)buffer;
	uint32_t ret = 0;
	uint32_t readLen = 0;
	uint32_t actualReadLen = 0;

	/* read from USB  */
	iproc_usb_device_bulk_read(buffer, sizeof(buffer), &actualReadLen);
	/* dump_buffer(buffer, actualReadLen); */

	/* check magic number */
	if (hdr->chip_spec_hdr.cmd_magic != USBD_COMMAND_MAGIC) {
		DEBUG_LOG("%s:%d invaild command header magic\n",
			     __func__, __LINE__);
		protocol_response_data_create(response, 1, 1, readLen);
		iproc_usb_device_interrupt_write(response, sizeof(response));
		return -2;
	}

	/* fill the buffer data into command struct */
	req->cmd = hdr->chip_spec_hdr.cmd;
	req->addr = hdr->chip_spec_hdr.data[0];
	req->param = hdr->chip_spec_hdr.data[1];
	req->param1 = hdr->chip_spec_hdr.data[2];

	DEBUG_LOG("%s:%d cmd[0x%x] offset[0x%x] size[0x%x] total[0x%x]\n",
		  __func__, __LINE__, req->cmd, req->addr, req->param,
		  req->param1);

	return ret;
}

/*
 * Function to receive data from channel,
 *
 * @addr offset
 * @req  command request
 * 
 * @return read bytes if successful, less than 0 error
 */
static int32_t protocol_channel_recv(uint32_t addr, uint32_t len)
{
	uint8_t response[USBD_RESPONSE_LEN];
	uint32_t readLen = 0, i = 0;
	uint32_t actualReadLen = 0;
    uint8_t *pBuffer = (uint8_t *)RCV_BUFFER;
	/* uint16_t cs = 0; */

	DEBUG_LOG("[%s():%d],len[0x%x]\n", __func__, __LINE__, len);

	memset(pBuffer, 0x00,BUF_SIZE);
	while (readLen < len)
	{
		actualReadLen = 0;
		DEBUG_LOG("[%s():%d] pBuffer = 0x%x\n",
			  __func__, __LINE__, pBuffer);
		iproc_usb_device_bulk_read(pBuffer, BUF_SIZE, &actualReadLen);
		/*for (i = 0; i < actualReadLen; i++) cs ^= gBuffer[i]; */
		readLen += actualReadLen;
        	pBuffer += actualReadLen;
	}

	/* protocol_response_data_create(response, 2, cs, readLen); */
	/* proc_usb_device_interrupt_write(response, sizeof(response)); */

	DEBUG_LOG("[%s():%d],read[0x%x]\n", __func__, __LINE__, readLen);
	return readLen;
}

/*
 * Function to parser the command from host
 * Parser the command, and then call the corresponding functions.
 *
 * @req  command request
 */
static int32_t protocol_cmd_parser(struct usbd_request *req)
{
	uint16_t ack = 0, checksum = 0;
	int32_t ret = 0;
	uint32_t length = 0;
	uint8_t fuse_val = 0;
	uint32_t size, len;
	uint8_t file_format = 0;
	int no_response = !!((req->param1 >> 24) & 0xff);

	no_response = 1;

	switch (req->cmd)
	{
		case CMD_DOWNLOAD:
			DEBUG_LOG("[%s():%d],command=CMD_DOWNLOAD\n", __func__, __LINE__);
			/* receive data from channel */
			ret = protocol_channel_recv(req->addr, req->param);
			DEBUG_LOG("%s:%d rcvd[0x%x] offset[0x%x] size[0x%x] total[0x%x]\n",
				    __func__, __LINE__, ret, req->addr, req->param, req->param1);
			DEBUG_LOG(".");
			if (ret < 0) {
				DEBUG_LOG("[%s():%d],Invalid read %d\n",__func__,__LINE__,ret);
				protocol_response_send(ERROR_COMMAND, 0, 0);
                		return -1;
			} else if (req->param1 == (ret + req->addr)) {
				/* we are done, return 0 */
				DEBUG_LOG("\n[%s():%d], final ret %d\n", __func__, __LINE__, ret);
				if (protocol_download_handler != NULL) {
					protocol_download_handler((uint8_t *)RCV_BUFFER, ret, req->addr);
				}
				protocol_response_send(ack, 0xffff, 0xffffffff);
				return 0;
			} else {
				DEBUG_LOG("\n[%s():%d], ret %d\n", __func__, __LINE__, ret);
				if (protocol_download_handler != NULL) {
				    protocol_download_handler((uint8_t *)RCV_BUFFER, ret, req->addr);
				}                             
			      	protocol_response_send(ack, 0xffff, 0xffffffff);
			    	/* not done yet */
				    return -1;
			}
			break;
		default:
		{
			DEBUG_LOG("[%s():%d],Invalid command id\n",__func__,__LINE__);
			protocol_response_send(ERROR_COMMAND, 0, 0);
			return -1;
		}
	}

	protocol_response_send(ack, 0xffff, 0xffffffff);

	/* not done yet */
	return -1;
}

/*
 * Function to send response data to host
 * through channel.
 *
 * response protocol:
 *  ----------------------------------------
 * | ack[15:0] | len[15:0] | checksum[15:0] |
 *  ----------------------------------------
 *
 * @ack  response ack value
 * @len  response len value
 * @csum  response checksum value
 *
 */
static void protocol_response_data_create(uint8_t *buffer,uint16_t ack, uint16_t csum, uint32_t len)
{
	cv_response_t *response = (cv_response_t *)buffer;

	response->ack = ack;
	response->csum = csum;
	response->len = len;

	DEBUG_LOG("[%s():%d],protocol_response_send_data= ", __func__, __LINE__);
	/* dump_buffer(buffer, USBD_RESPONSE_LEN); */

	return;
}

/*
 * Function to send response data to host
 * through channel.
 *
 * response protocol:
 *  ----------------------------------------
 * | ack[15:0] | len[15:0] | checksum[15:0] |
 *  ----------------------------------------
 *
 * @ack  response ack value
 * @len  response len value
 * @csum  response checksum value
 *
 */
static void protocol_response_send(uint16_t ack, uint16_t csum, uint32_t len)
{
	uint8_t buffer[USBD_RESPONSE_LEN];

	protocol_response_data_create(buffer,ack,csum,len);

	/* protocol_channel_send((uint8_t *)buffer, USBD_RESPONSE_LEN); */
	{
		uint8_t intInBuf[8] = {0x0,0x0,0x0,0x0,0x40/*0x2c*/,0x0,0x0,0x0};
		protocol_response_data_create(intInBuf, ack, csum, len);
		iproc_usb_device_interrupt_write(intInBuf, sizeof(intInBuf));
	}
	iproc_usb_device_bulk_write((uint8_t *)buffer, USBD_RESPONSE_LEN);

	return;
}

void protocol_start()
{
	struct usbd_request cmd;
	int32_t err, ret;

	memset(&cmd, 0, sizeof(cmd));
	while (1) {
		/* wait for command and do receive */
		err = protocol_cmd_receive(&cmd);
		if (err == 0) {
			/* parser the recv command */
			ret = protocol_cmd_parser(&cmd);
	 		DEBUG_LOG("[%s():%d], ret %d\n", __func__, __LINE__, ret);
			if (0 == ret)
                		break;
		} else {
			/* invalid command */
			protocol_response_send(ERROR_COMMAND, 0, 0);
		}
		usbdevice_isr();
		usbd_idle();
	}

	return;
}
