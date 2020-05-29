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
 * @file test_adc.c
 * @brief  ADC & MSR driver test
 *
 * @detail
 * Testing the ADC & MSR Requires User Interaction because
 * it involves swiping a Magnetic Strip along the swiping slot.
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <adc/adc.h>
#include <msr_swipe.h>
#include <broadcom/dma.h>
#include <broadcom/gpio.h>
#include <errno.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/types.h>
#include <test.h>

#ifdef TRACE_ADC_DATA
/* FIXME:
 * This buffer need to be provided by the user. But where will
 * the user obtain this, we provide here a sufficiently large
 * buffer. But 3 x 36KB is too much memory. So try to reduce
 * the size. Also provide provision where the user can provide
 * the buffer and the code can work off using a pointer to this
 * buffer.
 *
 * !!! Have to enlarge this ADC_BSIZE size if want to capture
 * the whole ADC samples... 
 * If no enough memory in the system, try to disable BT and NFC
 * features in Makefile to release more buffer.
 */
#define ADC_BSIZE (18*1024)
#else
#define ADC_BSIZE (4*1024)
#endif

static u16_t *testbuf1;

#ifdef CONFIG_ADCMSR
static u16_t *testbuf2;
static u16_t *testbuf3;
#endif

static s32_t intno = 0;
static s32_t intsize = 0;

s32_t adc_test_main_usage(char *cmd)
{
	TC_PRINT("Usage: %s [adc_no] [ch_no]\n", cmd);
	TC_PRINT("    Test ADC channel ch_no with ADC adc_no\n");
	return 0;
}

/* one Snapshot */
s32_t adc_test_snapshot(struct device *adc, adc_no adcno, adc_ch chno)
{
	u16_t v;
	const struct adc_driver_api *api;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
		return TC_FAIL;
	}
	api = adc->driver_api;

	TC_PRINT("Snapshot mode on ADC %d - Channel %d:\n", adcno, chno);

	api->get_snapshot(adc, adcno, chno, &v);

	TC_PRINT("Snapshot value = %x\n", v);

	return 0;
}

/* Polling mode definit times */
s32_t adc_test_tdm_polling_definit(struct device *adc, adc_no adcno, adc_ch chno)
{
	s32_t i;
	s32_t size = 0;
	s32_t rounds = 48;
	const struct adc_driver_api *api;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
		return TC_FAIL;
	}
	api = adc->driver_api;

	TC_PRINT("Polling %d rounds...\n", rounds);
	memset(testbuf1, 0, ADC_BSIZE * sizeof(u16_t));

	api->enable_nrounds(adc, adcno, chno, rounds);
	k_sleep(200);

	size = api->get_data(adc, adcno, chno, testbuf1, rounds);
	api->disable_tdm(adc, adcno, chno);

	TC_PRINT("ADC TDM definite %d rounds: got %d\n", rounds, size);
	for (i = 0; i < size; i++)
		TC_PRINT("%04x%c", testbuf1[i], (i + 1) % 20 ? ' ':'\n');
	TC_PRINT("\n");
	return 0;
}

/* TDM polling mode infinite times */
s32_t adc_test_tdm_polling_infinit(struct device *adc, adc_no adcno, adc_ch chno)
{
	s32_t i;
	s32_t size = 0;
	const struct adc_driver_api *api;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
		return TC_FAIL;
	}
	api = adc->driver_api;

	TC_PRINT("Polling tdm infinite without interupt:\n");
	memset(testbuf1, 0, ADC_BSIZE * sizeof(u16_t));

	api->enable_tdm(adc, adcno, chno, NULL);
	while (size < 100) {
		size += api->get_data(adc, adcno, chno, testbuf1 + size, (ADC_BSIZE * sizeof(u16_t) / 2) - size);
	}
	api->disable_tdm(adc, adcno, chno);

	TC_PRINT("Got %d data: \n", size);
	for (i = 0; i < size; i++)
		TC_PRINT("%04x%c", testbuf1[i], (i + 1) % 20 ? ' ':'\n');
	TC_PRINT("\n");

	return 0;
}

void adc_test_cb(struct device *adc, adc_no adcno, adc_ch chno, u32_t count)
{
	s32_t size;
	const struct adc_driver_api *api;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
	}
	else {
		api = adc->driver_api;
		size = api->get_data(adc, adcno, chno, testbuf1, count);
		intsize += size;
		intno ++;
	}
}

/* TDM interrupt mode */
s32_t adc_test_tdm_intr(struct device *adc, adc_no adcno, adc_ch chno)
{
	s32_t i;
	const struct adc_driver_api *api;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
		return TC_FAIL;
	}
	api = adc->driver_api;

	TC_PRINT("TDM with interrupt:\n");
	memset(testbuf1, 0, ADC_BSIZE * sizeof(u16_t));

	api->enable_tdm(adc, adcno, chno, (adc_irq_callback_t)adc_test_cb);
	intno = 0;
	intsize = 0;
	k_sleep(200);

	api->disable_tdm(adc, adcno, chno);
	TC_PRINT("ADC intr mode: total int %d total data %d \n", intno, intsize);

	for (i = 0; i < 32; i++)
		TC_PRINT("%04x%c", testbuf1[i], (i + 1) % 20 ? ' ':'\n');
	TC_PRINT("\n");

	return 0;
}

/* Main ADC Test Invocation
 *
 */
static s32_t adc_test(struct device *adc, adc_no adcno, adc_ch chno)
{
	const struct adc_driver_api *api;

	adcno %= adc_max;
	chno %= adc_chmax;

	if (!adc) {
		TC_ERROR("Invalid ADC controller\n");
		return TC_FAIL;
	}
	api = adc->driver_api;
	TC_PRINT("\n.....Testing ADC %d - Channel %d:\n", adcno, chno);

	/* Power Down ADC devices */
	api->powerdown(adc);

	/* Initialize ADC devices */
	api->init_all(adc);

	/* ADC & Channel Specific Tests */
	/* Test ADC in Snapshot Mode */
	adc_test_snapshot(adc, adcno, chno);
	/* Test ADC in Finite Polling Mode */
	adc_test_tdm_polling_definit(adc, adcno, chno);
	/* Test ADC in Infinite Polling Mode */
	adc_test_tdm_polling_infinit(adc, adcno, chno);
	/* Test ADC in TDM Interrupt Mode */
	adc_test_tdm_intr(adc, adcno, chno);

	TC_PRINT("ADC %d - Channel %d Test passed\n", adcno, chno);
	return TC_PASS;
}

static void adc_test_battery(void)
{
	struct device *adc;

	adc = device_get_binding(CONFIG_ADC_NAME);
	zassert_not_null(adc, "Cannot get ADC device\n");

	/* Allocate memory from heap for the test buffer */
	testbuf1 = (u16_t *)k_malloc(ADC_BSIZE * sizeof(u16_t));
	zassert_not_null(testbuf1, "Cannot allocate memory for buffer.\n");

	/* ADC0 Ch 0 Test */
	adc_test(adc, 0, 0);
	/* ADC0 Ch 1 Test */
	adc_test(adc, 0, 1);
	/* ADC1 Ch 0 Test */
	adc_test(adc, 1, 0);
	/* ADC1 Ch 1 Test */
	adc_test(adc, 1, 1);
	/* ADC2 Ch 0 Test */
	adc_test(adc, 2, 0);
	/* ADC2 Ch 1 Test */
	adc_test(adc, 2, 1);

	/* Free Allocated Buffer */
	k_free(testbuf1);
}

SHELL_TEST_DECLARE(test_adc)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(adc_driver_tests,
			 ztest_unit_test(adc_test_battery));

	ztest_run_test_suite(adc_driver_tests);

	return 0;
}

/************************************************
 ***************  ADC MSRR Test *****************
 ************************************************
 */
#ifdef CONFIG_ADC_MSR
s32_t adc_test_msrr(struct device *msr, struct device *adc)
{
	TC_PRINT("\n>>>> Please swipe the card in 15s <<<<\n");

#ifdef CONFIG_BOARD_SERP_CP
	msr_swipe_configure_adc_info(msr, 2, 0, 1, 1, 1, 0);
#else
	msr_swipe_configure_adc_info(msr, 0, 0, 1, 0, 2, 0);
#endif

	msr_swipe_init_with_adc(msr, adc);

	msr_swipe_gather_data(msr, adc);

	return 0;
}

static void adc_test_msrr_battery(void)
{
	struct device *msr;
	struct device *adc;

	msr = device_get_binding(CONFIG_MSR_NAME);
	zassert_not_null(msr, "Cannot get MSR SWIPE device\n");

	adc = device_get_binding(CONFIG_ADC_NAME);
	zassert_not_null(adc, "Cannot get ADC device\n");

	/* ADC MSRR Test */
	zassert_true((adc_test_msrr(msr, adc) == TC_PASS), NULL);
}

SHELL_TEST_DECLARE(test_adc_msrr)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(adc_msrr_driver_tests,
			 ztest_unit_test(adc_test_msrr_battery));

	ztest_run_test_suite(adc_msrr_driver_tests);

	return 0;
}
#endif

#ifdef ADC_DMA_TEST
static struct adc_dma adcdma[3];
static void adc_dma_cb(int ch)
{
	adcdma[ch].intcnt++;
}

int adc_dma_test_usage(char *cmd)
{
	TC_PRINT("Usage: %s\n", cmd);
	TC_PRINT("    ADC dma test, using ADC0 ch0, ADC1 ch0, ADC2 ch0\n");
	return 0;
}

int adc_dma_test(void)
{
	int i, j;

	memset(adcdma[0].RxDMABuf, 0, sizeof(adcdma[0].RxDMABuf));
	adcdma[0].cb = adc_dma_cb;
	adcdma[0].dmach = DMA_ADC_CH0;
	adcdma[0].nburst = 4;
	adcdma[0].intcnt = 0;

	memset(adcdma[1].RxDMABuf, 0, sizeof(adcdma[1].RxDMABuf));
	adcdma[1].cb = adc_dma_cb;
	adcdma[1].dmach = DMA_ADC_CH1;
	adcdma[1].nburst = 4;
	adcdma[1].intcnt = 0;

	memset(adcdma[2].RxDMABuf, 0, sizeof(adcdma[2].RxDMABuf));
	adcdma[2].cb = adc_dma_cb;
	adcdma[2].dmach = DMA_ADC_CH2;
	adcdma[2].nburst = 4;
	adcdma[2].intcnt = 0;

	adc_init(0);
	adc_init(1);
	adc_init(2);

	adc_dma_enable(0, 0, &adcdma[0]);
	adc_dma_enable(1, 0, &adcdma[1]);
	adc_dma_enable(2, 0, &adcdma[2]);

	bcm_task_delay(1000);

	idc_dcache_inv_addr(adcdma[0].RxDMABuf, sizeof(adcdma[0].RxDMABuf));
	idc_dcache_inv_addr(adcdma[1].RxDMABuf, sizeof(adcdma[1].RxDMABuf));
	idc_dcache_inv_addr(adcdma[2].RxDMABuf, sizeof(adcdma[2].RxDMABuf));

	for (j = 0; j < 3; j++) {
		TC_PRINT("\n CH %d --> dma cb : %d %d\n", j, adcdma[j].intcnt, adcdma[j].nburst);
		for (i = 0; i < sizeof(adcdma[j].RxDMABuf) / 2; i++)
			TC_PRINT("%04x%c", adcdma[j].RxDMABuf[i], (i + 1) % 20 ? ' ':'\n');
		adc_dma_disable(j, 0, &adcdma[i]);
	}

	if (!adcdma[0].intcnt || !adcdma[1].intcnt || !adcdma[1].intcnt)
		return -1;

	return 0;
}
#endif
