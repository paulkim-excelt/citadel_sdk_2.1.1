/*******************************************************************
 *
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

#ifndef  _CCID_CLASS_H
#define  _CCID_CLASS_H

#include "type.h"
#include "usbdesc.h"
#include "usbdevice.h"

#define CCID_MAX_BUF_LENGTH	0x210
#define CCID_INTR_BUF_SIZE	16

#define CCID_ABORT				0x01
#define CCID_CLOCK_FREQUENCIES			0x02
#define CCID_DATA_RATES				0x03

// CCID Bulk Out Command Message
#define PC_to_RDR_SetParameters			0x61
#define PC_to_RDR_IccPowerOn			0x62
#define PC_to_RDR_IccPowerOff			0x63
#define PC_to_RDR_GetSlotStatus			0x65
#define PC_to_RDR_Secure			0x69
#define PC_to_RDR_T0APDU			0x6A
#define PC_to_RDR_Escape			0x6B
#define PC_to_RDR_GetParameters			0x6C
#define PC_to_RDR_ResetParameters		0x6D
#define PC_to_RDR_IccClock			0x6E
#define PC_to_RDR_XfrBlock			0x6F
#define PC_to_RDR_Mechanical			0x71
#define PC_to_RDR_Abort				0x72
#define PC_to_RDR_SetDataRateAndClockFrequency	0x73

// CCID Bulk In Response Message
#define RDR_to_PC_DataBlock			0x80
#define RDR_to_PC_SlotStatus			0x81
#define RDR_to_PC_Parameters			0x82
#define RDR_to_PC_Escape			0x83
#define RDR_to_PC_DataRateAndClockFrequency	0x84

// CCID Interrupt In Messages
#define RDR_to_PC_NotifySlotChange		0x50
#define RDR_to_PC_HardwareError			0x51


// CCID Message OFFSET
#define COMMAND_OFFSET		0x00
#define EXTRA_BYTES_OFFSET	0x01
#define SLOT_OFFSET		0x05
#define SEQUENCE_OFFSET		0x06
#define STATUS_OFFSET		0x07
#define PROTOCOL_OFFSET		0x07
#define BWI_OFFSET			0x07 /* Block wait index in PC2RD XfrBlock */
#define CCID_ERROR_OFFSET	0x08
#define CLOCK_STATUS_OFFSET	0x09
#define SETP_FD_OFFSET      0x0A /* for RC2RD Parameter command, F/D timing value */
#define SETP_GUARD_OFFSET   0x0C /* for RC2RD Parameter command, guard time value */
#define SETP_T1WAIT_OFFSET  0x0D /* for RC2RD Parameter command, T1 wait time value */
#define SETP_T1IFSC_OFFSET  0x0D /* for RC2RD Parameter command, T1 ifsc value */
#define RES_PROTOCOL_OFFSET	0x09 /* for RD2PC Parameter response, protocol number */
#define MESSAGE_HEADER		0x0A
#define RES_TCCKST_OFFSET   0x0B /* for RD2PC Parameter response, conversion */
#define RES_CLKSTOP_OFFSET  0x0E /* for RD2PC Parameter response, ICC Clock Stop Support */
#define RES_BIFSC_OFFSET    0x0F /* for RD2PC Parameter response, IFSC */
#define RES_NAD_OFFSET      0x10 /* for RD2PC Parameter response, Nad value used by CCID */
 
// CCID Notification Message ( Intr EP In )
//#define NOTIFY_MSG_LENGTH   0x02


// CCID SetParameter
#define CCID_GET_F(n)       ((n >> 4) & 0x0f)
#define CCID_GET_D(n)       (n & 0x0f)

// Notification bit

// the following two n is the slot number
#define SLOT_CHANGED_STATUS(n)	(1<<(n))
#define CURRENT_STATE(n)	(1<<(n+1))



// CMD STATUS
// bits [6-7] in the slot status field (e.g. slot status register)
// Refer to CCID spec Section 6.2.6 (P55)
#define CMDSTATUS_OK       (0x00 << 6)
#define CMDSTATUS_FAIL     (0x01 << 6)
#define CMDSTATUS_TIMEEXT  (0x02 << 6)

// ICC STATUS
// bits [0-1] in the slot status field (e.g. slot status register)
// Refer to CCID spec Section 6.2.6 (P55)
#define ICCSTATUS_ACTIVE         0  // present, powered, not in reset
#define ICCSTATUS_INACTIVE       1  // present, but not in active mode
#define ICCSTATUS_ABSENT         2  // no card

// Slot Error Register when bmCommandStatus = 1
// Refer to CCID spec Section 6.2.6 (P54)
#define SLOTERR_CMD_ABORTED					0xFF
#define SLOTERR_ICC_MUTE					0xFE
#define SLOTERR_XFR_PARITY_ERROR			0xFD
#define SLOTERR_XFER_OVERRUN				0xFC
#define SLOTERR_HW_ERROR					0xFB
#define SLOTERR_BAD_ATR_TS					0xF8
#define SLOTERR_BAD_ATR_TCK					0xF7
#define SLOTERR_ICC_PROTOCOL_NOT_SUPPORTED	0xF6
#define SLOTERR_ICC_CLASS_NOT_SUPPORTED		0xF5
#define SLOTERR_PROCEDURE_BYTE_CONFLICT		0xF4
#define SLOTERR_DEACTIVATED_PROTOCOL		0xF3
#define SLOTERR_BUSY_WITH_AUTO_SEQUENCE		0xF2
#define SLOTERR_PIN_TIMEOUT					0xF0
#define SLOTERR_PIN_CANCELLED				0xEF
#define SLOTERR_CMD_SLOT_BUSY				0xE0
#define SLOTERR_CMD_NOT_SUPPORTED			0x00
#define SLOTERR_NONE                        0x00 /* Note this is not in the table. ErrCode is only valid when cmdStatus != 0 */


#pragma pack(1)
typedef struct
{
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	bcdCCID;
	uint8_t		bMaxSlotIndex;
	uint8_t		bVoltageSupport;
	uint32_t	dwProtocols;
	uint32_t	dwDefaultClock;
	uint32_t	dwMaxiumClock;
	uint8_t		bNumClockSupported;
	uint32_t	dwDataRate;
	uint32_t	dwMaxDataRate;
	uint8_t		bNumDataRatesSupported;
	uint32_t	dwMaxIFSD;
	uint32_t	dwSynchProtocols;
	uint32_t	dwMechanical;
	uint32_t	dwFeatures;
	uint32_t	dwMaxCCIDMessageLength;
	uint8_t		bClassGetResponse;
	uint8_t		bClassEnvelope;
	uint16_t	wLcdLayout;
	uint8_t		bPINSupport;
	uint8_t		bMaxCCIDBusySlots;
} tCCIDCLASS_DESCRIPTOR;

#pragma pack()

typedef struct CCIDDeviceConfigurationDescriptorCollection
{
	tCONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
	tINTERFACE_DESCRIPTOR InterfaceDescriptor;
 	tCCIDCLASS_DESCRIPTOR CCIDClassDescriptor;
	tENDPOINT_DESCRIPTOR  BulkInEndPDescriptor;
	tENDPOINT_DESCRIPTOR  BulkOutEndPDescriptor;
	tENDPOINT_DESCRIPTOR  IntrInEndPDescriptor;
} tCCIDDDEVICE_DESC_COLLECTION;

typedef struct
{
	void (*pUserAppInitial)(void);
	void (*pUserAppCallback)(void);
	EPSTATUS *BulkInEp;
	EPSTATUS *BulkOutEp;
	EPSTATUS *IntrInEp;
} tCCIDINTERFACE;

void NotifySlotChange(uint8_t bSlotNumber,uint8_t bstatus);
void CCIDCallBackHandler(EPSTATUS *ep);
void InitialCCIDDevice(void);
uint8_t parseCtATR(uint8_t *atrBuf, uint8_t atrLen, uint8_t *cardClass);

void ClassAbort(void);
void ClassGetClockfrequency(void);
void ClassGetDataRate(void);

#endif
