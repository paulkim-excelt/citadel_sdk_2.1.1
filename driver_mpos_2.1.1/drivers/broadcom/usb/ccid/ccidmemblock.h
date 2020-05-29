#ifndef CCID_MEMBLOCK_H
#define CCID_MEMBLOCK_H


#include "rfid/rfid_hw_api.h"
#include "HID/GeneralDefs.h"
#include "HID/applHidContext.h"
#include "HID/CardContext.h"
#include "HID/brfd_api.h" 

#include "ccidclass.h"
#include "bscd.h"

#define ATR_MAX_PROTOCOLS 16 
#define ATR_MAX_IB        4
#define CCID_ATR_DEFAULT_TA1 0x11
#define CCID_ATR_DEFAULT_TC1 0x00
#define CCID_ATR_DEFAULT_TC2 0x0A
#define CCID_ATR_DEFAULT_TA3 0x20
#define CCID_ATR_DEFAULT_TB3 0x4D

#define ATR_SET_INTERFACE_BYTE(td_count,ib_n,ib_value) {                  \
                    ccid_atr.ib[td_count][ib_n].present = 1;              \
                    ccid_atr.ib[td_count][ib_n].value = ib_value;         \
                }

#define ATR_INTERFACE_BYTE_TA 0
#define ATR_INTERFACE_BYTE_TB 1
#define ATR_INTERFACE_BYTE_TC 2
#define ATR_INTERFACE_BYTE_TD 3

typedef struct
{
  uint8_t TS;
  uint8_t T0;
  struct
  {
    uint8_t value;
    uint8_t present;
  }
  ib[ATR_MAX_PROTOCOLS][ATR_MAX_IB];
}
ATR_t;

typedef struct CCID_MEM_BLOCK
{
	tCCIDINTERFACE CCIDInterface;
	g_smartcard gSmartCard;
	BRFD_gSmartCardStruct gRFIDSmartCard;
    ATR_t                 ccid_atr;

	uint8_t bActiveStatus;
	uint8_t bClockStatus;
	uint8_t bSlotChange;
	uint8_t bRcvStatus;
	uint16_t wRcvLength ;
	uint8_t  bCCIDMode  ;   // CCID T= CL or T= T0,T1 
	uint8_t  bThisSlot;
	uint8_t  benumerated;
	uint16_t bXfrStatus;
	
	uint8_t __attribute__ ((aligned (4)))CCIDCmdMsg[64];
	uint8_t __attribute__ ((aligned (4)))CCIDNotifyBufferIn[8];
	uint8_t __attribute__ ((aligned (4)))CCIDMsgBufferOut[2*CCID_MAX_BUF_LENGTH];
	uint8_t __attribute__ ((aligned (4)))CCIDMsgBufferIn[2*CCID_MAX_BUF_LENGTH];
	uint8_t __attribute__ ((aligned (4)))ClockFrequency[4]; 

}tCCID_MEM_BLOCK;

#ifdef PHX_REQ_CT
extern tCCID_MEM_BLOCK ccid_mem_block;
#endif 

#ifdef PHX_REQ_CTLESS
extern tCCID_MEM_BLOCK ccid_mem_block_ctless;
#endif

#endif

