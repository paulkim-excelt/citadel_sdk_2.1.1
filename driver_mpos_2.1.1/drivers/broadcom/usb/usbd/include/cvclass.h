
#ifndef  _CVCLASS_H
#define  _CVCLASS_H

#include"usbdesc.h"
#include"usbdevice.h"

#define CV_BULKIN 1
#define CV_BULKOUT 2
#define CV_INTERRUPTIN 3

// Cv Bulk Out Command Message

// Cv Bulk In Response Message

// Cv Interrupt In Messages


// Cv Message OFFSET


#pragma pack(1)
typedef struct 
{
          uint8_t 	bLength;
          uint8_t 	bDescriptorType;
          uint16_t	bcdCCID;
          uint8_t 	bMaxSlotIndex;
          uint8_t 	bVoltageSupport;
          uint32_t 	dwProtocols;
          uint32_t 	dwDefaultClock;
          uint32_t 	dwMaxiumClock;
          uint8_t 	bNumClockSupported;
          uint32_t 	dwDataRate;
          uint32_t 	dwMaxDataRate;
          uint8_t  	bNumDataRatesSupported;
          uint32_t 	dwMaxIFSD;
          uint32_t 	dwSynchProtocols;
          uint32_t 	dwMechanical;
          uint32_t 	dwFeatures;
          uint32_t 	dwMaxCCIDMessageLength;
          uint8_t  	bClassGetResponse;
          uint8_t  	bClassEnvelope;
          uint16_t  wLcdLayout;
          uint8_t  	bPINSupport;
          uint8_t  	bMaxCCIDBusySlots;
          
}tCVCLASS_DESCRIPTOR;

#pragma pack()


typedef struct  CVClassDeviceConfigurationDescriptorCollection
{
	 tCONFIGURATION_DESCRIPTOR  ConfigurationDescriptor ;
	 tINTERFACE_DESCRIPTOR InterfaceDescriptor ;
 	 tCVCLASS_DESCRIPTOR   CVClassDescriptor;
	 tENDPOINT_DESCRIPTOR  BulkInEndPDescriptor ;
	 tENDPOINT_DESCRIPTOR  BulkOutEndPDescriptor;
	 tENDPOINT_DESCRIPTOR  IntrInEndPDescriptor;
}tCVClASSDDEVICE_DESC_COLLECTION;

typedef struct 
{
	void (*pUserAppInitial)(void);
	void (*pCallback)(void);
	EPSTATUS *BulkInEp;
	EPSTATUS *BulkOutEp;
	EPSTATUS *IntrInEp;
}tCVINTERFACE;


#if 0 
typedef struct 
{
	uint8_t bCmd;
	void (*pAction)(void);
}tCMDACTION;
#endif


void CvClassCallBackHandler(EPSTATUS *ep);
void cv_usbd_reset(void);
void InitialCvClass(uint8_t *pbuffer, uint16_t wlen);
void write( uint8_t EpNum, uint8_t *pbuff, uint16_t wlen);

#endif
