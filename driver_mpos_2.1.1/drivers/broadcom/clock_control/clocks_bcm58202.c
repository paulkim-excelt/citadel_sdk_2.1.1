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
#include <atomic.h>
#include <board.h>
#include <broadcom/clock_control.h>
#include <device.h>
#include <errno.h>
#include <irq.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <toolchain.h>

#define CRMU_PLL_AON_CTRL			0x0000
#define CRMU_PLL_AON_CTRL__GENPLL_PWRDN		1
#define CRMU_PLL_AON_CTRL__GENPLL_ISO_IN	0

#define CRMU_GENPLL_CONTROL0			0x0000
#define CRMU_GENPLL_CONTROL1_OFFSET		0x0004
#define CRMU_GENPLL_CONTROL1_LOAD_EN_SHIFT	6
#define CRMU_GENPLL_CONTROL2_OFFSET		0x0008
#define CRMU_GENPLL_CONTROL2_DATAMASK		0x00ffffff
#define CRMU_GENPLL_CONTROL3_OFFSET		0x000c
#define CRMU_GENPLL_CONTROL3_PDIV_SHIFT		24
#define CRMU_GENPLL_CONTROL3_PDIV_MASK		0xf
#define CRMU_GENPLL_CONTROL3_NDIV_INT_MASK	0x1ff
#define CRMU_GENPLL_CONTROL4_OFFSET		0x0010
#define CRMU_GENPLL_CONTROLX_MDIV_SHIFT		16
#define CRMU_GENPLL_CONTROLX_MDIV_MASK		0xff

#define NUM_CLK_CHAN				6

#define REF_CLK_FREQ_MHZ	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MHZ(1))

struct clk {
	u8_t enable_offset;
	u8_t enable_shift;
	u8_t hold_shift;

	atomic_val_t refcount;
};

struct clk clk_en[NUM_CLK_CHAN] = {
	{0x4, 6, 0, 0},  /* Channel 1 - Unicam LP Clock */
	{0x4, 7, 1, 0},  /* Channel 2 - AXI/AHB and APB (divided by 2) */
	{0x4, 8, 2, 0},  /* Channel 3 - WDT, Timer, UART Baud Clock */
	{0x4, 9, 3, 0},  /* Channel 4 - QSPI interface clock */
	{0x4, 10, 4, 0}, /* Channel 5 - Smartcard clock */
	{0x4, 11, 5, 0}, /* Channel 6 - SMC clock */
};

struct genpll_clk_cfg {
	mem_addr_t genpll_base;
	mem_addr_t aon_base;
};

static void genpll_clk_enable(struct device *dev, u32_t channel)
{
	const struct genpll_clk_cfg *cfg = dev->config->config_info;
	mem_addr_t genpll_base = cfg->genpll_base;
	const struct clk en = clk_en[channel];

	/* channel enable is active low */
	sys_clear_bit(genpll_base + CRMU_GENPLL_CONTROL0 + en.enable_offset,
		      1 << en.enable_shift);

	/* also make sure channel is not held */
	sys_clear_bit(genpll_base + CRMU_GENPLL_CONTROL0 + en.enable_offset,
		      1 << en.hold_shift);
}

static void genpll_clk_disable(struct device *dev, u32_t channel)
{
	const struct genpll_clk_cfg *cfg = dev->config->config_info;
	mem_addr_t genpll_base = cfg->genpll_base;
	mem_addr_t aon_base = cfg->aon_base;
	const struct clk en = clk_en[channel];

	sys_set_bit(genpll_base + CRMU_GENPLL_CONTROL0 + en.enable_offset,
		    1 << en.enable_shift);

	/* latch input value so core power can be shut down */
	sys_set_bit(aon_base + CRMU_PLL_AON_CTRL,
		    1 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN);

	/* power down the core */
	sys_clear_bit(aon_base + CRMU_PLL_AON_CTRL,
		      1 << CRMU_PLL_AON_CTRL__GENPLL_PWRDN |
		      1 << CRMU_PLL_AON_CTRL__GENPLL_ISO_IN);
}


static int bcm58202_clock_control_on(struct device *dev,
				     clock_control_subsys_t sys)
{
	u32_t chan = (u32_t) sys;

	if (chan < 1 || chan > 6)
		return -ENODEV;

	/* Make Channel zero based for use in the array */
	chan -= 1;

	if (atomic_inc(&clk_en[chan].refcount) != 0)
		return 0;

	genpll_clk_enable(dev, chan);

	return 0;
}

static int bcm58202_clock_control_off(struct device *dev,
				      clock_control_subsys_t sys)
{
	u32_t chan = (u32_t) sys;

	if (chan < 1 || chan > 6)
		return -ENODEV;

	/* Make Channel zero based for use in the array */
	chan -= 1;

	if (atomic_dec(&clk_en[chan].refcount) > 1)
		return 0;

	genpll_clk_disable(dev, chan);

	return 0;
}

static int genpll_determine_rate(struct device *dev, u32_t chan)
{
	const struct genpll_clk_cfg *cfg = dev->config->config_info;
	mem_addr_t genpll_base = cfg->genpll_base;
	u32_t pdiv, ndiv_int, ndiv_frac, fvco, mdiv, offset;

	pdiv = sys_read32(genpll_base + CRMU_GENPLL_CONTROL3_OFFSET);
	pdiv >>= CRMU_GENPLL_CONTROL3_PDIV_SHIFT;
	pdiv &= CRMU_GENPLL_CONTROL3_PDIV_MASK;
	ndiv_int = sys_read32(genpll_base + CRMU_GENPLL_CONTROL3_OFFSET);
	ndiv_int &= CRMU_GENPLL_CONTROL3_NDIV_INT_MASK;
	ndiv_frac = sys_read32(genpll_base + CRMU_GENPLL_CONTROL2_OFFSET);
	ndiv_frac &= CRMU_GENPLL_CONTROL2_DATAMASK;

	fvco = (ndiv_int + ndiv_frac / MHZ(1)) * REF_CLK_FREQ_MHZ / pdiv;
	SYS_LOG_DBG("(1 / %d) * (%d + %d / MHZ(1)) * REF_CLK_FREQ_MHZ = %d",
		    pdiv, ndiv_int, ndiv_frac, fvco);

	/* Logic to determine which control register to use.  There are 2 per
	 * CTRL register, starting at CTRL4 for channel 1.
	 */
	offset = genpll_base + CRMU_GENPLL_CONTROL4_OFFSET + ((chan / 2) * 0x4);
	mdiv = sys_read32(offset);
	if (chan % 2 != 0)
		mdiv >>= CRMU_GENPLL_CONTROLX_MDIV_SHIFT;
	mdiv &= CRMU_GENPLL_CONTROLX_MDIV_MASK;

	SYS_LOG_DBG("Chan %d output freq is %d (%d / %d)",
		    chan + 1, fvco / mdiv, fvco, mdiv);

	return MHZ(fvco) / mdiv;
}

static int bcm58202_clock_control_get_subsys_rate(struct device *dev,
						  clock_control_subsys_t sys,
						  u32_t *rate)
{
	u32_t chan = (u32_t) sys;

	ARG_UNUSED(dev);

	SYS_LOG_DBG("Attempting to query clock speed of channel %d", chan);

	if (chan < 1 || chan > 6)
		return -ENODEV;

	/* Make Channel zero based for use in the array */
	chan -= 1;

	*rate = genpll_determine_rate(dev, chan);

	return 0;
}

static int genpll_set_rate(struct device *dev, u32_t chan, u32_t rate)
{
	const struct genpll_clk_cfg *cfg = dev->config->config_info;
	mem_addr_t genpll_base = cfg->genpll_base;
	u32_t pdiv, ndiv_int, ndiv_frac, fvco, mdiv, offset, val;

	/* Read what is currently there */
	pdiv = sys_read32(genpll_base + CRMU_GENPLL_CONTROL3_OFFSET);
	pdiv >>= CRMU_GENPLL_CONTROL3_PDIV_SHIFT;
	pdiv &= CRMU_GENPLL_CONTROL3_PDIV_MASK;
	ndiv_int = sys_read32(genpll_base + CRMU_GENPLL_CONTROL3_OFFSET);
	ndiv_int &= CRMU_GENPLL_CONTROL3_NDIV_INT_MASK;
	ndiv_frac = sys_read32(genpll_base + CRMU_GENPLL_CONTROL2_OFFSET);
	ndiv_frac &= CRMU_GENPLL_CONTROL2_DATAMASK;

	fvco = (ndiv_int + ndiv_frac / MHZ(1)) * REF_CLK_FREQ_MHZ / pdiv;

	/* The best you can do is change the mdiv for the channel to get as
	 * close as possible to the rate requested.  So, find the mdiv value
	 * that gets us closest to that value, and change it if different than
	 * the current value.
	 */
	for (mdiv = 1; mdiv < 256; mdiv++) {
		if (MHZ(fvco) / mdiv < rate)
			break;
	}

	/* Now read the mdiv register and program if necessary */
	offset = genpll_base + CRMU_GENPLL_CONTROL4_OFFSET + ((chan / 2) * 0x4);
	val = sys_read32(offset);
	if (chan % 2 != 0)
		val >>= CRMU_GENPLL_CONTROLX_MDIV_SHIFT;
	val &= CRMU_GENPLL_CONTROLX_MDIV_MASK;

	SYS_LOG_DBG("Chan %d output freq is %d (%d / %d)",
		    chan + 1, MHZ(fvco) / val, fvco, val);
	SYS_LOG_DBG("Want to use mdiv of %d", mdiv);
	if (val != mdiv) {
		val = sys_read32(offset);
		val &= ~(CRMU_GENPLL_CONTROLX_MDIV_MASK <<
			 (CRMU_GENPLL_CONTROLX_MDIV_SHIFT * (chan % 2)));
		val |= mdiv << (CRMU_GENPLL_CONTROLX_MDIV_SHIFT * (chan % 2));
		sys_write32(val, offset);

		/* Now enable the new mdiv */
		sys_clear_bit(genpll_base + CRMU_GENPLL_CONTROL1_OFFSET,
			    chan + CRMU_GENPLL_CONTROL1_LOAD_EN_SHIFT);
		sys_set_bit(genpll_base + CRMU_GENPLL_CONTROL1_OFFSET,
			    chan + CRMU_GENPLL_CONTROL1_LOAD_EN_SHIFT);
		sys_clear_bit(genpll_base + CRMU_GENPLL_CONTROL1_OFFSET,
			    chan + CRMU_GENPLL_CONTROL1_LOAD_EN_SHIFT);
	}

	return 0;
}

static int bcm58202_clock_control_set_subsys_rate(struct device *dev,
						  clock_control_subsys_t sys,
						  u32_t rate)
{
	u32_t chan = (u32_t) sys;

	SYS_LOG_DBG("Attempting to set clock speed of channel %d to %d",
		    chan, rate);

	if (chan < 1 || chan > 6)
		return -ENODEV;

	/* Make Channel zero based for use in the array */
	chan -= 1;

	return genpll_set_rate(dev, chan, rate);
}

static struct genpll_clk_cfg genpll_clk_cfg_data = {
	.genpll_base = BRCM_CIT_GENPLL_3001D000_BASE_ADDRESS_0,
	.aon_base = BRCM_CIT_AONPLL_3001C020_BASE_ADDRESS_0,
};

static struct clock_control_driver_api bcm58202_clock_control_api = {
	.on = bcm58202_clock_control_on,
	.off = bcm58202_clock_control_off,
	.get_rate = bcm58202_clock_control_get_subsys_rate,
	.set_rate = bcm58202_clock_control_set_subsys_rate,
};

static int bcm58202_clock_control_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return  0;
}

DEVICE_AND_API_INIT(bcm58202_clocks, CLOCK_CONTROL_NAME,
		    bcm58202_clock_control_init, NULL, &genpll_clk_cfg_data,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &bcm58202_clock_control_api);
