/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/*
 * @file test_ft.c
 * @brief  FT & MSR driver test
 *
 * @detail
 * Testing the FT & MSR Requires User Interaction because
 * it involves swiping a Magnetic Strip along the swiping slot.
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <broadcom/dma.h>
#include <broadcom/gpio.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>
#include <flextimer/flextimer.h>

#define CONFIG_FT_MSR

#define BUFFER_SIZE (1024*2)

static s32_t size[3];
static s32_t sizeo[3];
static u8_t output[128];

s32_t ft_test_usage(char *cmd)
{
	TC_PRINT("Usage: %s\n", cmd);
	TC_PRINT("Flextimer MSR test, swipe card to complete\n");
	return 0;
}

s32_t ft_test(struct device *ft)
{
	volatile s32_t count = 7500;
	u16_t *p;
	s32_t ret = 0;
	u16_t *testbuf[3];

	size[0] = size[1] = size[2] = 0;
	sizeo[0] = sizeo[1] = sizeo[2] = 0;

	/* Note: In case of Pin Mux Programming Failure; build with
	 * CONFIG_ALLOW_IOMUX_OVERRIDE=y
	 * and invoke disable FT , which will restore the pins to their
	 * original configuration.
	 */
	ret = ft_enable(ft, 1);
	if(ret) {
		TC_PRINT("Could not configure MFIO Pins for FT in IOMUX mode 1.\n");
		TC_PRINT("Trying IOMUX mode 2 for Flex Timer.\n");
		ft_set_iomux_mode(ft, FT_IOMUX_MODE2);
		ret = ft_enable(ft, 1);
		if(ret) {
			TC_PRINT("Could not configure MFIO Pins for FT in IOMUX mode 2 either.\n");
			TC_PRINT("Build with CONFIG_ALLOW_IOMUX_OVERRIDE=y set in defconfig file & re-run test.\n");
			TC_PRINT("Exiting Test.\n");
			zassert_false(ret, "Could not program MFIO.");
		}
	}

	/* Acquire Dynamic memory for Buffer */
	testbuf[0] = (u16_t *)k_malloc(BUFFER_SIZE);
	testbuf[1] = (u16_t *)k_malloc(BUFFER_SIZE);
	testbuf[2] = (u16_t *)k_malloc(BUFFER_SIZE);
	zassert_true((testbuf[0] && testbuf[1] && testbuf[2]), "malloc failed");

	TC_PRINT("Configured MFIO Pins for Flex Timer.\n");
	TC_PRINT(">>>> enabled FT. <<<<\n");
	TC_PRINT(">>>> Swipe the card (within 1 second) <<<<\n");

	ft_wait_for_sos(ft);
	TC_PRINT("-----> Detected Start of Swipe <-----\n");

	while (count--) {
		size[0] += ft_get_data(ft, 0, (uint16_t *)testbuf[0] + size[0], 64);
		size[1] += ft_get_data(ft, 1, (uint16_t *)testbuf[1] + size[1], 64);
		size[2] += ft_get_data(ft, 2, (uint16_t *)testbuf[2] + size[2], 64);
		k_busy_wait(200);

		if (size[0] < 10 && size[1] < 10 && size[2] < 10)
			continue;

		if (count % 5 == 0) {
			if (size[0] == sizeo[0] && size[1] == sizeo[1] && size[2] == sizeo[2])
				break;
			sizeo[0] = size[0];
			sizeo[1] = size[1];
			sizeo[2] = size[2];
		}
	}

	if (size[0] > 10) {
		TC_PRINT("\n------FT channel 0: size %d------\n", size[0]);
		p = (uint16_t *)testbuf[0];
		ret = ft_decode(p, &size[0], 0, output, sizeof(output));
		if (ret)
			TC_PRINT("decode error !! %d\n", ret);
		else
			TC_PRINT("LRC check OK: %d\n", size[0]);
		if (size[0])
			TC_PRINT("%s\n\n", output);
	}
	if (size[1] > 10) {
		TC_PRINT("\n------FT channel 1: size %d------\n", size[1]);
		p = (uint16_t *)testbuf[1];
		ret = ft_decode(p, &size[1], 1, output, sizeof(output));
		if (ret)
			TC_PRINT("decode error !! %d\n", ret);
		else
			TC_PRINT("LRC check OK: %d\n", size[1]);
		if (size[1])
			TC_PRINT("%s\n", output);
	}
	if (size[2] > 10) {
		TC_PRINT("\n------FT channel 2: size %d------\n", size[2]);
		p = (uint16_t *)testbuf[2] + 4;
		ret = ft_decode(p, &size[2], 2, output, sizeof(output));
		if (ret)
			TC_PRINT("decode error !! %d\n", ret);
		else
			TC_PRINT("LRC check OK: %d\n", size[2]);
		if (size[2])
			TC_PRINT("%s\n", output);
	}

	/* Release Dynamic memory for Buffer */
	k_free(testbuf[0]);
	k_free(testbuf[1]);
	k_free(testbuf[2]);

	/* Restore the IOMUX setting */
	ft_disable(ft);

	return 0;
}

/************************************************
 ***************  FT MSRR Test *****************
 ************************************************
 */
#ifdef CONFIG_FT_MSR
SHELL_TEST_DECLARE(test_ft_msrr)
{
	struct device *ft;
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ft = device_get_binding(CONFIG_FT_NAME);
	zassert_not_null(ft, "Cannot get FT device\n");

	ft_test(ft);

	return 0;
}
#endif
