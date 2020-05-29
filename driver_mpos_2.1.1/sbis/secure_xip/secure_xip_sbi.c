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
#include <smau.h>
#include <cortex_a/barrier.h>
#include "secure_xip.h"

/**
 * QSPI flash sizes
 */
typedef enum qspi_flash_size{
    QSPI_FLASH_SIZE_4MB  = (4*1024*1024),
    QSPI_FLASH_SIZE_8MB  = (8*1024*1024),
    QSPI_FLASH_SIZE_16MB = (16*1024*1024),
    QSPI_FLASH_SIZE_32MB = (32*1024*1024)
}qspi_flash_size_e;

int32_t image_handler(uint8_t *data, uint32_t data_len, uint32_t address)
{
    int32_t chunks = 0;
    uint32_t *ptr = (uint32_t *)data;
    if (0 == address) {
        // Workaround for flash address
        ptr[4] = IPROC_SBI_FLASH_ADDRESS & 0x00ffffff;
        chunks = xip_config(ptr, data_len);
        if (chunks < 0) {
            post_log("\nxip_config failed: %d \n", chunks);
            while(1);
        }
    }
    post_log("Program %d bytes at offset 0x%x \n", data_len, address);
    xip_prg_chunk(ptr);
    return 0;
}

void resetIPROC(void)
{
    // You can use the M0 mailbox to implement the Soft Reset you want: write a value 0xFFFF_FFFF to
    // IPROC_CRMU_MAIL_BOX1_IPROC2MCU_MSG_1 for a Soft Reset.
    sys_write32(0x0, IPROC_CRMU_MAIL_BOX0);
    sys_write32(0xFFFFFFFF, IPROC_CRMU_MAIL_BOX1);
}

int sbi_main(void)
{
	int i = 0;
	int chunks = 0;
	int32_t iaddr = 0;
	char k;

	post_log("\n\n**** XIP SBI %s: %s ****\n\n", __DATE__, __TIME__);
	post_log("Press 'd' to start image download from USB.\n\n");
	post_log("Press 'b' to boot from flash.\n\n");

	while (1) {
        k = getch();
        if (k == 'd' || k == 'b')
            break;
	}

	if (k == 'b') {
		post_log("** boot from flash **\n");
		iaddr = xip_check_image(IPROC_SBI_FLASH_ADDRESS);
        if (iaddr > 0) {
            post_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            post_log("!!! AAI image is valid, jumping to %x !!!\n", iaddr);
            post_log("!!! If Secure XIP works then you should !!!!!!!\n");
            post_log("!!! See the \"Citadel Test AAI\" boot up !!!!!!!!\n");
            post_log("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
            k_sleep(3000);
            /* Configure SMAU just before branching */
            xip_configure_smau(IPROC_SBI_FLASH_ADDRESS);
			((void (*)(void))iaddr)();
		} else {
			post_log("AAI image is invalid, program image from USB\n");
		}
	} else {
		post_log("** Key pressed, program image from USB **\n");
	}

    /* usb device phy related init   */
    post_log("In iproc_usb_device_early_init()\n");
    iproc_usb_device_early_init();

    /* Copy data from usb interface to internal memory  */
    post_log("In iproc_usb_device_init)\n");
    iproc_usb_device_init();

    usbd_delay_ms(10);

    post_log("[%s():%d],begin to receive data\n",__func__,__LINE__);
    protocol_register_download_callback(image_handler);
    protocol_start();

	post_log("\n -- Program done, reset system -- \n");
	resetIPROC();
	/* this function couldn't return */
	while(1);
	return 0;
}
