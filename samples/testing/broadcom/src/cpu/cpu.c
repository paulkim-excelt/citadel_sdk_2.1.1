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
#include <board.h>
#include <device.h>
#include <ztest.h>
#include <test.h>
#include <logging/sys_log.h>
#include <cortex_a/cp15.h>
#include <broadcom/dma.h>
#include <cortex_a/cache.h>

static void test_caches(void)
{
	unsigned int val;

	/* Check if the i and d caches are enabled */
	val = read_sctlr();
	if (val & DATA_CACHE_ENABLE_BIT)
		TC_PRINT("Data and Unified Caches Enabled\n");
	else
		TC_PRINT("Data and Unified Caches Disabled\n");

	if (val & INSTR_CACHE_ENABLE_BIT)
		TC_PRINT("Instruction Cache Enabled\n");
	else
		TC_PRINT("Instruction Cache Disabled\n");

	/* If not, then error out */
#ifdef CONFIG_DATA_CACHE_SUPPORT
	zassert((val & DATA_CACHE_ENABLE_BIT), "Data and Unified Caches Disabled",
		"Data and Unified Caches Disabled");
#endif
	zassert((val & INSTR_CACHE_ENABLE_BIT), "Instruction Cache Disabled",
		"Instruction Cache Disabled");
}

/* Secure configuration register */
static void test_neon(void)
{
	unsigned int val;

	/* Check if VFP and NEON are enabled */
	val = read_cpacr();
	if (val & ASEDIS_DISABLE_BIT)
		TC_PRINT("Advanced SIMD extension Disabled\n");
	else
		TC_PRINT("Advanced SIMD extension enabled\n");
	if (val & D32DIS_DISABLE_BIT)
		TC_PRINT("Use of D16-D31 Disabled\n");
	else
		TC_PRINT("Use of D16-D31 enabled\n");
	if (val & CP11_PRIV_ACCESS)
		TC_PRINT("CP11 Privileged mode access enabled\n");
	else
		TC_PRINT("CP11 Privileged mode access disabled\n");
	if (val & CP11_USR_ACCESS)
		TC_PRINT("CP11 User mode access enabled\n");
	else
		TC_PRINT("CP11 User mode access disabled\n");
	if (val & CP10_PRIV_ACCESS)
		TC_PRINT("CP10 Privileged mode access enabled\n");
	else
		TC_PRINT("CP10 Privileged mode access disabled\n");
	if (val & CP10_USR_ACCESS)
		TC_PRINT("CP10 User mode access enabled\n");
	else
		TC_PRINT("CP10 User mode access disabled\n");

	zassert(!(val & ASEDIS_DISABLE_BIT), "Advanced SIMD extension Disabled",
		"Advanced SIMD extension Disabled");
}

static void test_sleepwait(void)
{
	u32_t time_before, time_after;
	int i;

	for (i = 1; i <= 10; i++) {
		time_before = _timer_cycle_get_32();
		k_busy_wait(i);
		time_after = _timer_cycle_get_32();

		TC_PRINT("%dus delay, difference is %d\n",
			 i, time_after - time_before);
	}

	for (i = 1; i <= 10; i++) {
		time_before = _timer_cycle_get_32();
		k_sleep(i);
		time_after = _timer_cycle_get_32();

		TC_PRINT("%dms sleep, difference is %d\n",
			 i, time_after - time_before);
	}
}

static void all_cpu_tests(void)
{
	test_sleepwait();
	test_caches();
	test_neon();
}

/* Buffer used for DMA should be a multiple of cache
 * line and should be aligned to the cache line size
 */
#define BUFF_SIZE	16
static void dcache_test(void)
{
#ifdef CONFIG_DATA_CACHE_SUPPORT
#ifdef CONFIG_BRCM_DRIVER_DMA
	u32_t chan, i;
	u32_t  __attribute__((aligned(64))) a[BUFF_SIZE];
	u32_t  __attribute__((aligned(64))) b[BUFF_SIZE];
	struct dma_config dma_cfg;
	struct dma_block_config dma_block_cfg0;
	struct device *dma_dev = device_get_binding(CONFIG_DMA_PL080_NAME);

	dma_request(dma_dev, &chan, ALLOCATED_CHANNEL);

	/* Initialize a[] and b[] to different values */
	for (i = 0; i < BUFF_SIZE; i++) {
		a[i] = 0x20 + i;
		b[i] = 0x12;
	}

	/* Flush a[] and b[] values from cache to SRAM */
	clean_dcache_by_addr((u32_t)&a[0], sizeof(a));
	clean_dcache_by_addr((u32_t)&b[0], sizeof(b));

	/* Configure DMA block */
	dma_block_cfg0.source_address = (u32_t)&a[0];
	dma_block_cfg0.dest_address = (u32_t)&b[0];
	dma_block_cfg0.block_size = BUFF_SIZE*sizeof(u32_t);
	dma_block_cfg0.next_block = NULL;

	/* Setup the DMA config */
	dma_cfg.head_block = &dma_block_cfg0;

	/* Copy a -> b via dma */
	dma_memcpy(dma_dev, chan, &dma_cfg);

	/* Confirm b[] == a[] after cache is invalidated (done by DMA api) */
	for (i = 0; i < BUFF_SIZE; i++)
		zassert_true(b[i] == a[i], "Expected b[i] == a[i]");

	dma_release(dma_dev, chan);
#endif
#else
	TC_PRINT("Enable CONFIG_DATA_CACHE_SUPPORT to run this test!\n");
#endif
}

SHELL_TEST_DECLARE(test_cpu)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ztest_test_suite(cpu_tests,
			 ztest_unit_test(all_cpu_tests),
			 ztest_unit_test(dcache_test));

	ztest_run_test_suite(cpu_tests);

	return 0;
}
