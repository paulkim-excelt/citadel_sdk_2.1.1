/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

#include <platform.h>
#include <post_log.h>
#include <qspi_flash.h>
#include <reg_access.h>
#include <usbd_if.h>
#include <utils.h>
#include <protocol/protocol.h>
#include <cortex_a/barrier.h>
#include <sotp.h>
#include <sbi.h>
#include <string.h>

#define USE_TAG_PROTECT

/*#define CV_CMD_STATE  0
#define CV_DAT_STATE  1
#define CV_EXE_STATE  2

#define cvclass_mem_block ((tCVCLASS_MEM_BLOCK *)IPROC_BTROM_GET_USB_cvclass_mem_block())
#define usb_device_ctx ((usb_device_ctx_t *)IPROC_BTROM_GET_USB_usb_device_ctx_t())
#define epInterfaceBuffer ((uint8_t *)IPROC_BTROM_GET_USB_epInterfaceBuffer())
#define pcvclass_mem_block   ((tCVCLASS_MEM_BLOCK *)cvclass_mem_block)

#define CVClassInterface (pcvclass_mem_block->CVClassInterface)
#define wRcvLength    (pcvclass_mem_block->wRcvLength)
#define bRcvStatus (pcvclass_mem_block->bRcvStatus)
#define dwDataLen (pcvclass_mem_block->dwDataLen)
#define dwlength (pcvclass_mem_block->dwlength)
#define bcmdRcvd (pcvclass_mem_block->bcmdRcvd)
#define bcvCmdInProgress (pcvclass_mem_block->bcvCmdInProgress)
#define wflags    (pcvclass_mem_block->wflags)
*/

#define FSBI_VERSION_STRING "upgrade_sbi"  " (" SBI_DATE " - " SBI_TIME ")"
#define FLASH_PAGE_SIZE 256

#ifdef USE_TAG_PROTECT
uint32_t save_tag = 0;
uint32_t save_offset = 0;
uint8_t page_buffer[FLASH_PAGE_SIZE] = {0};
#endif

int single_step_convert();

static void reset_iproc(void)
{
    sys_write32(0x0, IPROC_CRMU_MAIL_BOX0);
    sys_write32(0xFFFFFFFF, IPROC_CRMU_MAIL_BOX1);
}

int32_t image_handler(uint8_t *data, uint32_t data_len, uint32_t address)
{
	struct device *qspi_dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);
#ifdef USE_TAG_PROTECT
	uint32_t *ptr = NULL;
	uint32_t i = 0;
	if (address == 0) {
		for (i = 0; i < FLASH_PAGE_SIZE; i += 4) {
			ptr = (uint32_t *)(data + i);
			if ((*ptr == TOC_TAG) || (*ptr == SBI_TAG)) {
				save_tag = *ptr;
				save_offset = i;
				post_log("Detect tag 0x%08x at address 0x%08x, replace this tag\n",
					 save_tag, save_offset);
				memcpy(page_buffer, data, FLASH_PAGE_SIZE);
				memset(data, 0xff, FLASH_PAGE_SIZE);
				break;
			}
		}
	}
#endif
	qspi_flash_erase(qspi_dev, address, data_len);
	post_log("Erase flash at offset 0x%x,length is %d bytes\n", address, data_len); 
	qspi_flash_write(qspi_dev, address, data_len, data);
	post_log("Write %d bytes at offset 0x%x \n", data_len, address);
	return 0;
}

int sbi_main(void)
{
	struct device *qspi_dev = device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);
#ifdef LYNX_CONVERT
	sotp_bcm58202_status_t status = IPROC_OTP_VALID;
	struct sotp_bcm58202_dev_config config;
#endif

#ifdef SBI_DEBUG
	post_log("%s\n", FSBI_VERSION_STRING);
	post_log("compiled by %s\n", LINUX_COMPILE_BY);
	post_log("compile host %s\n",LINUX_COMPILE_HOST);
	post_log("compiler %s\n",LINUX_COMPILER);
	post_log("In sbi_main()\n");
#endif

	/* usb device phy related init	 */
	post_log("In iproc_usb_device_early_init()\n");
	iproc_usb_device_early_init();

	/* Copy data from usb interface to internal memory  */
	post_log("In iproc_usb_device_init)\n");
	iproc_usb_device_init();

	post_log("[%s():%d],begin to receive data\n", __func__, __LINE__);
	protocol_register_download_callback(image_handler);
	protocol_start();

#ifdef USE_TAG_PROTECT
	if ((save_tag == TOC_TAG) || (save_tag == SBI_TAG)) {
		post_log("Restore tag 0x%08x at address 0x%08x\n", save_tag, save_offset);
		qspi_flash_write(qspi_dev, 0, FLASH_PAGE_SIZE, page_buffer);
	}
#endif

#ifdef LYNX_CONVERT
	if (IPROC_OTP_VALID == isunassigned()) {
		status = readDevCfg(&config);
		if (status != IPROC_OTP_VALID) {
			post_log("Read OTP failed!!!\n");
		} else {
			post_log("BRCMRevisionID = 0x%x, ProductID = 0x%x\n",
				 config.BRCMRevisionID, config.ProductID);
			post_log("Convert from unassigned to ABpending\n");
			setConfigABDevPend();
			/* setConfigABPend(); */
			post_log("Perform soft reset\n");
			reset_iproc();
			post_log("Please reset system\n");
		}
	}
#endif

	while(1);
	return 0;
}
