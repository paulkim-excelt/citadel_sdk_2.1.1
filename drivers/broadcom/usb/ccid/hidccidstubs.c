#ifdef PHX_REQ_CTLESS
#inclue"phx_glbs.h"
#include "phx.h"
#include "usbdesc.h"
#include "usbdevice.h"

#include "ushx_api.h"
#include "rfid/rfid_hw_api.h"

#include "HID/GeneralDefs.h"

/* system context */
#include "HID/CardContext.h"
#include "HID/applHidContext.h"
#include "HID/brfd_api.h"

#include "HID/byteorder.h"
#include "HID/hidccidstubs.h"
#include "HID/ccidctless.h"


#ifdef HID_STUB
struct volatileStruct theVolatileStruct;
struct volatileStruct * VOLATILE_MEM_PTR = &theVolatileStruct;
struct HIDCCIDInterface hidCCIDInterface;

static void *hidSmallHeap;
static void *hidBigHeap;
void *hid_glb_ptr;

void HIDRF_setupHeaps(void *small, const size_t smallSize, void *large, const size_t largeSize)
{
	hidSmallHeap = small;
	hidBigHeap = large;
	/* Initialise the heap */
	ushxInitHeap(smallSize, small);
	ushxInitHeap(largeSize, large);
}

void *rfid_get_small_heap(void)
{
	return hidSmallHeap;
}

void *rfid_get_big_heap(void) 
{
	return hidBigHeap;
}
void HIDRF_recvEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
	DEBUG_PRINT("recvEndpointData\n");
}

void HIDRF_sendEndpointData(EPSTATUS *ep, uint8_t *pData, uint32_t len)
{
	DEBUG_PRINT("sendEndpointData\n");
}

#endif
