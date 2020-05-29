/*
 * $Copyright Broadcom Corporation$
 *
 */
#include "usbdevice_internal.h"
#include "usbregisters.h"
#include "usbcvclass/cvclass.h"
#include "usbbootinterface.h"
#include "usbd_if.h"
#include "usbdevice.h"
//#include <cv/cvapi.h>
//#include <cv/cvinternal.h>
#include <string.h>
#include <kernel.h>

/* USB memory */
u8_t usb_mem[IPROC_USB_MEM_SIZE];

#ifdef SBL_DEBUG
#define DEBUG_LOG post_log
#else
#define DEBUG_LOG(x,...)
#endif

#define MIN(x,y) ((x)<(y)?(x):(y))

#define cvclass_mem_block ((tCVCLASS_MEM_BLOCK *)IPROC_BTROM_GET_USB_cvclass_mem_block())
#define usb_device_ctx ((usb_device_ctx_t *)IPROC_BTROM_GET_USB_usb_device_ctx_t())
#define usb_mem_block ((tUSB_MEM_BLOCK *)IPROC_BTROM_GET_USB_usb_mem_block())

#define epInterfaceSendBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceSendBuffer())
#define epInterfaceBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceBuffer())

#define sbi_image ((uint8_t *)IPROC_BTROM_GET_USB_SBI_IMAGE())

#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)cvclass_mem_block)

#define dwlength (pcvclass_mem_block->dwlength)
#define wflags    (pcvclass_mem_block->wflags)
#define CVClassInterface (pcvclass_mem_block->CVClassInterface)

#define __raw_writel(regval, reg)	*(volatile unsigned int *)(reg) = (regval)
#define __raw_readl(reg)			*(volatile unsigned int *)(reg)

#define USBD_DISABLE_ISO_DELAY_US			(100)
#define USBD_DISABLE_PHY_RST_DELAY_US		(150)
#define USBD_DISABLE_NON_DRV_DELAY_US		(250)
#define USBD_INIT_DONE_DELAY_MS				(1)

void usbd_delay_us(int us)
{
	k_busy_wait(us);
}

void usbd_delay_ms(int ms)
{
	usbd_delay_us(1000*ms);
}

#define CDRU_USBPHY_D_P1CTRL__soft_resetb (1)
#define CDRU_USBPHY_D_P1CTRL__afe_non_driving (0)
static void lynx_usb_shutdown(void)
{

	unsigned int reg_data;

	/* USB Reset */
	reg_data = reg32_read(CDRU_SW_RESET_CTRL);
	reg_data &= ~((1 << CDRU_SW_RESET_CTRL__usbd_sw_reset_n));
	reg32_write(CDRU_SW_RESET_CTRL, reg_data);

	// Apply PLL reset
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1);
	reg_data &= ~(1 << CDRU_USBPHY_D_CTRL1__pll_resetb);
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	// Apply PHY reset
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1);
	reg_data &= ~(1 << CDRU_USBPHY_D_CTRL1__resetb);
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

}

static void lynx_usb_device_hw_init(int cnt)
{
	// NOTE: We missed the following bit defines in Lynx' socreg.h
#define CDRU_USBPHY_D_P1CTRL__soft_resetb (1)
#define CDRU_USBPHY_D_P1CTRL__afe_non_driving (0)
	volatile unsigned int reg_data;
	volatile uint32_t dctrl;
	uint32_t lockCount = 0;


	/* FIXME: Required? */
	/*reg32_write(SYS_CLK_SOURCE_SEL_CTRL, 0x124);
	lynx_usb_shutdown();
	usbd_delay_ms(5);*/


	// Enable USB
	dctrl = reg32_read(CDRU_SW_RESET_CTRL);
	dctrl |= (1 << CDRU_SW_RESET_CTRL__usbd_sw_reset_n);
	reg32_write(CDRU_SW_RESET_CTRL, dctrl);

	if (cnt == 1) { /* Do only the first time this is called */
		// Disable LDO
		reg_data = reg32_read(CDRU_USBPHY_D_CTRL2);
		reg_data |= (1 << CDRU_USBPHY_D_CTRL2__ldo_pwrdwnb);
		reg_data |= (1 << CDRU_USBPHY_D_CTRL2__afeldocntlen);
		reg32_write(CDRU_USBPHY_D_CTRL2, reg_data);

		// Disable Isolation
		usbd_delay_us(USBD_DISABLE_ISO_DELAY_US);
		reg_data = reg32_read(CRMU_USBPHY_D_CTRL);
		reg_data &= ~(1 << CRMU_USBPHY_D_CTRL__phy_iso);
		reg32_write(CRMU_USBPHY_D_CTRL, reg_data);
	}


	// Enable IDDQ current
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL2);
	reg_data &= ~(1 << CDRU_USBPHY_D_CTRL2__iddq);
	reg32_write(CDRU_USBPHY_D_CTRL2, reg_data);

	// Disable PHY reset
	usbd_delay_us(USBD_DISABLE_PHY_RST_DELAY_US);
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1);
	reg_data |= (1 << CDRU_USBPHY_D_CTRL1__resetb);
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	usbd_delay_us(USBD_DISABLE_PHY_RST_DELAY_US);
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1) & 0xfc00ffff;
	reg_data |= (3 << 16); // ka
	reg_data |= (3 << 19); // ki
	reg_data |= (0xa << 22); // kp
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	// Suspend enable
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1);
	reg_data |= (1 << CDRU_USBPHY_D_CTRL1__pll_suspend_en);
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	/* UTMI CLK for 60 Mhz */
	usbd_delay_us(150);
	reg_data = reg32_read(CRMU_USBPHY_D_CTRL) & 0xfff00000;
	reg_data |= 0xec4ec; //'d967916
	reg32_write(CRMU_USBPHY_D_CTRL, reg_data);
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1) & 0xfc000007;
	reg_data |= (0x03 << CDRU_USBPHY_D_CTRL1__ka_R); // ka 'd3
	reg_data |= (0x03 << CDRU_USBPHY_D_CTRL1__ki_R); // ki 'd3
	reg_data |= (0x0a << CDRU_USBPHY_D_CTRL1__kp_R); // kp 'hA
	reg_data |= (0x24 << CDRU_USBPHY_D_CTRL1__ndiv_int_R ); //ndiv 'd36
	reg_data |= (0x01 << CDRU_USBPHY_D_CTRL1__pdiv_R ); //pdiv 'd1
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	//Disable PLL reset
	reg_data = reg32_read(CDRU_USBPHY_D_CTRL1);
	reg_data |= (1 << CDRU_USBPHY_D_CTRL1__pll_resetb);
	reg32_write(CDRU_USBPHY_D_CTRL1, reg_data);

	// Disable software reset
	reg_data = reg32_read(CDRU_USBPHY_D_P1CTRL);
	reg_data |= (1 << CDRU_USBPHY_D_P1CTRL__soft_resetb);
	reg32_write(CDRU_USBPHY_D_P1CTRL, reg_data);

#ifndef CONFIG_LYNX_EMULATION
#define PLL_LOCK_COUNT 5
	/*
	 * Wait for stable lock. It is possible for the PLL to jump in and out of lock.
	 * Attempting to access the UDC20 block registers without PLL lock can result in
	 * HARD_FAULT exceptions.
	 */
	while (1) {
		reg_data = reg32_read(CDRU_USBPHY_D_STATUS);
		if ((reg_data & (1 << CDRU_USBPHY_D_STATUS__pll_lock)) == 0) {
			lockCount = 0;
		} else {
			if (lockCount >= PLL_LOCK_COUNT)
				break;
			lockCount++;
		}

		usbd_delay_ms(1 + (2 * lockCount));
	}

	// Disable non-driving
	usbd_delay_us(USBD_DISABLE_NON_DRV_DELAY_US);
	reg_data = reg32_read(CDRU_USBPHY_D_P1CTRL);
	reg_data &= ~(1 << CDRU_USBPHY_D_P1CTRL__afe_non_driving);
	reg32_write(CDRU_USBPHY_D_P1CTRL, reg_data);
#endif // CONFIG_LYNX_EMULATION


	// Flush Device RxFIFO
	dctrl = *USB_UDCAHB_DEVICE_CONTROL;
	dctrl |= (1 << DEVCTRL__DEVNAK);
	*USB_UDCAHB_DEVICE_CONTROL = dctrl;

	dctrl = *USB_UDCAHB_DEVICE_CONTROL;
	dctrl |= (1 << DEVCTRL__SRX_FLUSH);
	*USB_UDCAHB_DEVICE_CONTROL = dctrl;

	dctrl = *USB_UDCAHB_DEVICE_CONTROL;
	dctrl &= ~(1 << DEVCTRL__DEVNAK);
	*USB_UDCAHB_DEVICE_CONTROL = dctrl;

	usbd_delay_ms(1);
}

void iproc_usb_device_early_init(void)
{
	lynx_usb_device_hw_init(1);
}


void iproc_usb_device_init(void)
{

#if IPROC_BOOTSTRAP_DEBUG
	icache_disable();
	invalidate_icache_all();
	dcache_disable();
	v7_outer_cache_disable();
	invalidate_dcache_all();
#endif

	DEBUG_LOG("[%s():%d],begin\n",__func__,__LINE__);
/*Global parameter initialize*/
	memset((void *)(cvclass_mem_block), 0, sizeof(tCVCLASS_MEM_BLOCK));
	memset((void *)(usb_mem_block), 0, sizeof(tUSB_MEM_BLOCK));
	memset((void *)epInterfaceSendBuffer, 0, IPROC_BTROM_GET_USB_epInterfaceSendBuffer_size);
	memset((void *)epInterfaceBuffer, 0, IPROC_BTROM_GET_USB_epInterfaceBuffer_size);
	memset((void *)USB_START_ADDRESS, 0, IPROC_USB_MEM_SIZE);
//	post_log("usbDeviceInit()\n");	
	/* using usb cv */
	usbDeviceInit();	
	usbDeviceStart();
#if IPROC_BOOTSTRAP_DEBUG
	usbd_dump();
#endif
	//usbd_delay_ms(10);
	//DEBUG_LOG("Download @ addr 0x%x\n",UBOOT_START_ADDRESS  + offset * SIZE);
	//iproc_usb_device_read(0, UBOOT_START_ADDRESS  + offset * SIZE, IMAGE_SIZE);
	//DEBUG_LOG("\nDone\n");
	DEBUG_LOG("[%s():%d],USB_START_ADDRESS=0x%x\n",__func__,__LINE__,USB_START_ADDRESS);
	DEBUG_LOG("[%s():%d],end\n",__func__,__LINE__);
}

int iproc_usb_device_bulk_read(uint8_t *data, uint32_t readLen, uint32_t *actualReadLen)
{
	iproc_status_t status = 0;
	uint8_t *dst_ptr = data;
	uint32_t toReadLen = readLen, actualLen = 0;

	DEBUG_LOG("[%s():%d], want to read %d\n",__func__,__LINE__,toReadLen);
	*actualReadLen = 0;

	while (readLen > 0)
	{
		while (usb_device_ctx->total_rem_len == 0)
		{
			usbdevice_isr();
			usbd_idle();
		}

		actualLen = MIN(readLen, usb_device_ctx->total_rem_len);
		memcpy((void*)dst_ptr, (void*)usb_device_ctx->cur_read_offset, actualLen);
		usb_device_ctx->total_rem_len -= actualLen;
		usb_device_ctx->cur_read_offset += actualLen;
		*actualReadLen += actualLen;
		dst_ptr += actualLen;
		readLen -= actualLen;

		usbCvBulkOutSetUp(CVClassInterface.BulkOutEp->maxPktSize);
		//DEBUG_LOG("[%s():%d], maxPktSize 0x%x\n",__func__,__LINE__,CVClassInterface.BulkOutEp->maxPktSize);
		//DEBUG_LOG("[%s():%d], read 0x%x rcved 0x%x of 0x%x\n",__func__,__LINE__,actualLen,*actualReadLen,toReadLen);
	}
	DEBUG_LOG("[%s():%d], bulk read 0x%x\n",__func__,__LINE__,*actualReadLen);
	return status;
}

int iproc_usb_device_bulk_write( uint8_t *data, uint32_t writeLen)
{
	uint32_t  left = writeLen;
	uint32_t  hasWritedLen = 0;
	uint32_t  sendLen=0;

	//DEBUG_LOG("[%s():%d], want to write %d\n",__func__,__LINE__,writeLen);

	while(left>0){
		if(left<CVClassInterface.BulkOutEp->maxPktSize){
			sendLen = left;
		}else{
			sendLen = CVClassInterface.BulkOutEp->maxPktSize;
		}
		usbCvBulkIn(data + hasWritedLen, sendLen);
		hasWritedLen += sendLen;
		left -=sendLen;
	}

	return 0;
}

int iproc_usb_device_interrupt_write( uint8_t *data, uint32_t writeLen)
{
	uint32_t  left = writeLen;
	uint32_t  hasWritedLen = 0;
	uint32_t  sendLen=0;

	//DEBUG_LOG("[%s():%d], want to write %d\n",__func__,__LINE__,writeLen);

	while(left>0){
		//DEBUG_LOG("[%s():%d],continue,left=%d\n",__func__,__LINE__,left);
		if(left<CVClassInterface.BulkOutEp->maxPktSize){
			sendLen = left;
		}else{
			sendLen = CVClassInterface.BulkOutEp->maxPktSize;
		}
		usbCvInterruptIn(data + hasWritedLen, sendLen);
		hasWritedLen += sendLen;
		left -=sendLen;
	}

	//DEBUG_LOG("[%s():%d],end\n",__func__,__LINE__);
	return 0;
}

extern void UsbShutdown(void);
void iproc_usb_device_shutdown(void)
{
	UsbShutdown();
}

extern bool UsbResume(uint8_t ucState);
void iproc_usb_device_resume(void)
{
	UsbResume(0);
}

