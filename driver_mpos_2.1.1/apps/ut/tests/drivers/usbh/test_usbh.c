/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
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

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/dma.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

#include "cfeglue.h"
#include "usbhost_mem.h"
#include "ohci.h"
#include "ehci.h"

int test_num;
enum test {
	TEST_OHCI_REG_DUMP = 1,
	TEST_EHCI_REG_DUMP,
	TEST_OHCI_SUSPEND,
	TEST_OHCI_RESUME,
	TEST_EHCI_SUSPEND,
	TEST_EHCI_RESUME,
	TEST_OHCI_MAIN,
	TEST_EHCI_MAIN,
};

enum usbh {
	OHCI = 1,
	EHCI,
};

extern void usbhost_init(void);
extern void usbh_phyInit(void);

static int usbhost_test_main(int param)
{
	if (param == OHCI) {
		TC_PRINT("OHCI Start...\n");
		usbHostOhci = 1;
	}
	if (param == EHCI) {
		TC_PRINT("EHCI Start...\n");
		usbHostOhci = 0;
#ifdef SMC_ENABLED
		smc_start();
#endif
	}
	usbh_phyInit();
	usbhost_init();

	return 0;
}

static int usbh_ohci_port_suspend(void)
{
	ohci_reg->rhport00 |= M_OHCI_RHPORTSTAT_PSS;

	TC_PRINT("ohci_reg->rhport00=%08x\n", ohci_reg->rhport00);
	return 0;
}

static int usbh_ohci_port_resume(void)
{
	u32_t reg;

	reg = ohci_reg->rhport00;
	reg = M_OHCI_RHPORTSTAT_PRS | M_OHCI_RHPORTSTAT_PES;
	reg &= ~M_OHCI_RHPORTSTAT_PSS;
	ohci_reg->rhport00 = reg;

	/* wait for port resume successfully */
	usb_delay_ms(20);

	/* check resume whether complete */
	TC_PRINT("ohci_reg->rhport00=%08x\n", ohci_reg->rhport00);

	return 0;
}

static int usbh_ehci_suspend(void)
{
	u32_t portsc;

	portsc = EHCI_READ_CSR(USB_EHCI_BASE, R_EHCI_PORTSC);
	/*  whether port is enabled */
	if (!(portsc & M_EHCI_PORT_ENABLE_DISABLE)) {
		TC_PRINT("EHCI port is not enabled portsc=%08x\n", portsc);
		return -1;
	}

	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_PORTSC, V_EHCI_SUSPEND(1));
	k_sleep(2);

	/* need a delay ? and check whether the write is successful. */
	portsc = EHCI_READ_CSR(USB_EHCI_BASE, R_EHCI_PORTSC);
	TC_PRINT("ECHI portsc=%08x\n", portsc);

	return 0;
}

static int usbh_ehci_resume(void)
{
	u32_t portsc;

	portsc = EHCI_READ_CSR(USB_EHCI_BASE, R_EHCI_PORTSC);
	/* whether port is in suspended */
	if (!(portsc & M_EHCI_SUSPEND)) {
		TC_PRINT("EHCI port is not suspended portsc=%08x\n", portsc);
		return -1;
	}

	EHCI_SET_BITS(USB_EHCI_BASE, R_EHCI_PORTSC,
		      V_EHCI_FORCE_PORT_RESUME(1));
	/* according to USB2.0 SPEC */
	k_sleep(20);

	EHCI_UNSET_BITS(USB_EHCI_BASE, R_EHCI_PORTSC,
			V_EHCI_FORCE_PORT_RESUME(1));
	k_sleep(2);

	portsc = EHCI_READ_CSR(USB_EHCI_BASE, R_EHCI_PORTSC);
	TC_PRINT("EHCI portsc=%08x\n", portsc);

	return 0;
}

static int usbh_test(int param)
{
	TC_PRINT("USBH testing...\n");

	switch (param) {
	case TEST_OHCI_REG_DUMP:
		ohci_reg_dump(ohci_reg);
		break;

	case TEST_EHCI_REG_DUMP:
		ehci_reg_dump(ehci_reg);
		break;
	case TEST_OHCI_SUSPEND:
		usbh_ohci_port_suspend();
		break;

	case TEST_OHCI_RESUME:
		usbh_ohci_port_resume();
		break;

	case TEST_EHCI_SUSPEND:
		usbh_ehci_suspend();
		break;

	case TEST_EHCI_RESUME:
		usbh_ehci_resume();
		break;

	case TEST_OHCI_MAIN:
		usbhost_test_main(OHCI);
		break;

	case TEST_EHCI_MAIN:
		usbhost_test_main(EHCI);
		break;

	default:
		break;
	}

	TC_PRINT("USBH testing end\n");
	return 0;
}

static int usbh_test_usage(void)
{
	TC_PRINT("usage:\n");
	TC_PRINT("\tusbh <test_num>\n");
	TC_PRINT("\t\t1: ohci reg dump\n"
	      "\t\t2: ehci reg dump\n"
	      "\t\t3: ohci port suspend testing\n"
	      "\t\t4: ohci port resume testing\n"
	      "\t\t5: ehci suspend testing\n"
	      "\t\t6: ehci resume testing\n"
	      "\t\t7: ohci_test_main\n"
	      "\t\t8: ehci_test_main\n");
	return 0;
}

void usbhost_test(void)
{
	TC_PRINT("test_num: %d\n", test_num);
	if ((test_num > 0) && (test_num < 9))
		zassert_true((usbh_test(test_num) == TC_PASS), NULL);
}

SHELL_TEST_DECLARE(test_usbh)
{

	if (argc == 1) {
		usbh_test_usage();
		return 0;
	}

	if (argc == 2)
		test_num = atoi(argv[1]);

	ztest_test_suite(usbh_driver_tests, ztest_unit_test(usbhost_test));
	ztest_run_test_suite(usbh_driver_tests);

	return 0;
}
