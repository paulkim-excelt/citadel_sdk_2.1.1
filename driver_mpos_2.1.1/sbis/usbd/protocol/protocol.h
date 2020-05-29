#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "cv/cvmain.h"
#include "usbdevice.h"

typedef struct PACKED chip_spec_hdr {
	uint16_t cmd_magic;
	uint16_t cmd;
	uint32_t data[15];
} chip_spec_hdr_t;

typedef struct PACKED cv_f_sbi_hdr
{
   cv_encap_command  cv_cmd_hdr;
	/* on windows this size is uint16_t due to compiler differences */
	uint32_t padding;
   chip_spec_hdr_t   chip_spec_hdr;
   uint8_t           trail[USBD_BULK_MAX_PKT_SIZE-sizeof(cv_encap_command)-sizeof(uint32_t)-sizeof(chip_spec_hdr_t)];
} cv_f_sbi_hdr_t;

typedef struct PACKED cv_response
{
	uint16_t ack;
	uint16_t csum;
	uint32_t len;
} cv_response_t;

#define USBD_COMMAND_MAGIC	0x0606
#define USBD_COMMAND_LEN	(sizeof(cv_f_sbi_hdr_t))
#define USBD_RESPONSE_LEN	(sizeof(cv_response_t))
#define ERROR_COMMAND		0xffff
#define INVALID_PARAM		0xfffe

/* command id  */
#define CMD_COMMON	(0x01)
#define CMD_EXTEND	(0x02)

typedef uint16_t cmd_t;
enum cmd_t_e
{
	CMD_DOWNLOAD		= ((CMD_COMMON << 8) | 0x01)
};

typedef int32_t (*protocol_download_callback)(uint8_t *data,
					      uint32_t data_len, uint32_t address);
void protocol_register_download_callback(protocol_download_callback handler);
void protocol_start(void);

#endif /* __PROTOCOL_H__ */
