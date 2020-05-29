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
#include <broadcom/clock_control.h>
#include <device.h>
#include <toolchain.h>
#include <irq.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <clock_a7.h>
#include "sotp.h"

#define A7_CRM_IRQ_EN				0x0004
#define A7_CRM_IRQ_EN_PLL0_IRQ_EN		4

#define A7_CRM_PLL_PWR_ON			0x0070
#define A7_CRM_PLL_PWR_ON_PLL0_POST_RESETB	5
#define A7_CRM_PLL_PWR_ON_PLL0_RESETB		4
#define A7_CRM_PLL_PWR_ON_PLL0_ISO_PLLOUT	3
#define A7_CRM_PLL_PWR_ON_PLL0_ISO_PLL		2
#define A7_CRM_PLL_PWR_ON_PLL0_PWRON_LDO	1
#define A7_CRM_PLL_PWR_ON_PLL0_PWRON_PLL	0

#define A7_CRM_PLL_CMD				0x0080
#define A7_CRM_PLL_CMD_UPDATE_PLL0_FREQ_POST	1
#define A7_CRM_PLL_CMD_UPDATE_PLL0_FREQ_VCO	0

#define A7_CRM_PLL_STATUS			0x0084
#define A7_CRM_PLL_CTRL_STATUS_0		0x0090
#define A7_CRM_PLL_CTRL_STATUS_1		0x0094
#define A7_CRM_PLL_PLL0_LOCK			9

#define A7_CRM_PLL_IRQ_EN			0x0088
#define A7_CRM_PLL_IRQ_EN_PLL0_LOCK_IRQ		9
#define A7_CRM_PLL_IRQ_EN_PLL0_LOCK_LOST_IRQ	8

#define A7_CRM_PLL_IRQ_STATUS			0x008c
#define A7_CRM_PLL_IRQ_PLL0_LOCK		9
#define A7_CRM_PLL_IRQ_PLL0_LOCK_LOST		8

#define A7_CRM_PLL0_CHAN_CTRL			0x00a0
#define A7_CRM_PLL0_CHAN_CTRL_PLL0_CHAN0_EN_SEL 0

#define A7_CRM_PLL_CHAN_BYPASS			0x00ac
#define A7_CRM_PLL_CHAN_BYPASS_CHAN_0		0
#define A7_CRM_PLL_CHAN_BYPASS_CHAN_4		4

#define A7_CRM_PLL0_CTRL1			0x0100

#define A7_CRM_PLL0_CTRL2			0x0104
#define A7_CRM_PLL0_NDIV_FRAC_MODE_SEL		15
#define A7_CRM_PLL0_NDIV_FRAC_WIDTH		20

#define A7_CRM_PLL0_CTRL3			0x0108
#define A7_CRM_PLL0_CTRL3_PLL0_PDIV		12

#define A7_CRM_PLL0_CTRL4			0x010c
#define A7_CRM_PLL0_CTRL4_PLL0_FREFEFF_INFO	10
#define A7_CRM_PLL0_CTRL4_PLL0_KA		7
#define A7_CRM_PLL0_CTRL4_PLL0_KI		4
#define A7_CRM_PLL0_CTRL4_PLL0_KP		0

#define A7_CRM_PLL0_CFG0_CTRL			0x0120
#define A7_CRM_PLL0_CFG1_CTRL			0x0124
#define A7_CRM_PLL0_CFG2_CTRL			0x0128
#define A7_CRM_PLL0_CFG3_CTRL			0x012c

#define A7_CRM_PLL0_CHAN0_DESCRIPTION		0x0140
#define A7_CRM_PLL0_CHAN0_DESC_PLL0_CHAN0_MDIV	0

#define A7_CRM_SUBSYSTEM_CFG_2			0x0c88

#define MAX_MDIV_VAL				256
#define MAX_NDIV_INT_VAL			1024
#define MIN_NDIV_INT_VAL			16
#define MAX_PDIV_VAL				8
#define MAX_FVCO_CLOCK_MHZ			2400
#define MIN_FVCO_CLOCK_MHZ			600
#define MIN_FVCO_CLOCK_KHZ			600000
#define MAX_FVCO_CLOCK_KHZ			2400000
#define REF_CLK_FREQ_MHZ	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / MHZ(1))
#define REF_CLK_FREQ_KHZ 	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / KHZ(1))
/* Max VCO Values from OTP identifying different Part Types */
#define PART_TYPE_350MHZ 	3
#define PART_TYPE_500MHZ 	2
#define PART_TYPE_1GHZ 	1
/* Parameter Limits */
/* Fixme: Per Hardware Manual mdiv can take values between 1 - 256
 * or 1 - 512 depending on whether 8 or 9 bits used. But how to know
 * how many bits are used in the first place?
 */
#define MDIV_MAX		512
/* Fixme: Per Hardware Manual pdiv can take values between 1 - 15
 * for some reason this is limited to 1, i.e. as good as having
 * no pre-divisor at all.
 */
#define PDIV_MAX		1
#define NDIV_INT_MAX_0		1024
#define NDIV_INT_MAX_1		77
#define NDIV_INT_MAX_2		39
#define NDIV_INT_MAX_3		27

struct citadel_a7_cfg_data {
	mem_addr_t base;
};

static int citadel_a7_clock_control_on(struct device *dev,
				       clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int citadel_a7_clock_control_off(struct device *dev,
					clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static s32_t a7pll_determine_rate(struct device *dev)
{
	const struct citadel_a7_cfg_data *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;
	u32_t pdiv, ndiv_int, mdiv;
	u32_t eff_pdiv, eff_ndiv_int, eff_mdiv;
	u32_t ndiv_frac_mode_sel, ndiv_frac_msb, ndiv_frac_lsb;
	u64_t fvco;

	/* Read out the effective pdiv, ndiv_int & mdiv values from status registers */
	eff_pdiv = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_0);
	eff_pdiv = (eff_pdiv >> A7_CRM_PLL0_CTRL3_PLL0_PDIV) & 0xf;
	eff_ndiv_int = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_0) & 0x3ff;
	eff_mdiv = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_1) & 0x1ff;

	pdiv = sys_read32(base + A7_CRM_PLL0_CTRL3);
	pdiv = (pdiv >> A7_CRM_PLL0_CTRL3_PLL0_PDIV) & 0xf;
	if (pdiv == 0) {
		SYS_LOG_ERR("pdiv is 0, Illegal State in A7_CRM_PLL0_CTRL3!!\n");
		return -ENOTSUP;
	} else if (pdiv != eff_pdiv) {
		SYS_LOG_ERR("pdiv:%d, but effective pdiv:%d!\n", pdiv, eff_pdiv);
	}
	ndiv_int = sys_read32(base + A7_CRM_PLL0_CTRL3) & 0x3ff;
	if (ndiv_int != eff_ndiv_int) {
		SYS_LOG_ERR("ndiv_int:%d, but effective ndiv_int:%d!\n", ndiv_int, eff_ndiv_int);
	}
	ndiv_frac_msb = sys_read32(base + A7_CRM_PLL0_CTRL2);
	ndiv_frac_mode_sel = (ndiv_frac_msb & (1 << A7_CRM_PLL0_NDIV_FRAC_MODE_SEL));
	ndiv_frac_mode_sel = (ndiv_frac_mode_sel >> A7_CRM_PLL0_NDIV_FRAC_MODE_SEL);
	ndiv_frac_msb = (ndiv_frac_msb & ~(1 << A7_CRM_PLL0_NDIV_FRAC_MODE_SEL));
	ndiv_frac_lsb = sys_read32(base + A7_CRM_PLL0_CTRL1);

	fvco = (u64_t)(ndiv_int * REF_CLK_FREQ_MHZ);
	if (ndiv_frac_mode_sel) {
		fvco += (((ndiv_frac_msb << 10) + ndiv_frac_lsb) * REF_CLK_FREQ_MHZ) >> 20;
		SYS_LOG_DBG("(1 / %d) * (%d + ((%d << 10 + %d) >> 20)) * REF_CLK_FREQ_MHZ = ",
		    pdiv, ndiv_int, ndiv_frac_msb, ndiv_frac_lsb);
	} else {
		fvco += ((u64_t)ndiv_frac_msb * REF_CLK_FREQ_MHZ) / ndiv_frac_lsb;
		SYS_LOG_DBG("(1 / %d) * (%d + (%d / %d)) * REF_CLK_FREQ_MHZ = ",
			pdiv, ndiv_int, ndiv_frac_msb, ndiv_frac_lsb);
	}
	fvco = fvco / pdiv;
	SYS_LOG_DBG("%lld", fvco);

	mdiv = sys_read32(base + A7_CRM_PLL0_CHAN0_DESCRIPTION) & 0x1ff;
	if (mdiv != eff_mdiv) {
		SYS_LOG_ERR("mdiv:%d, but effective mdiv:%d!\n", mdiv, eff_mdiv);
	}
	/* Fixme: How to know if 8 bits or 9 bits are used? */
	if (mdiv == 0)
		mdiv = MDIV_MAX;
	SYS_LOG_DBG("CPU freq is %d (%lld / %d)", (u32_t)(fvco / mdiv), fvco, mdiv);

	return MHZ(fvco) / mdiv;
}

static int citadel_a7_clock_control_get_subsys_rate(struct device *dev,
						    clock_control_subsys_t sys,
						    u32_t *rate)
{
	ARG_UNUSED(sys);

	if (!dev)
		return -EINVAL;

	SYS_LOG_DBG("Attempting to query clock speed of CPU");

	*rate = a7pll_determine_rate(dev);

	return 0;
}

/* Desired resolution is in MHz */
static void calculate_parameters_old(const u32_t rate_mhz,
        u32_t *mdiv, u32_t *pdiv, u32_t *ndivint, u32_t *ndivfrac)
{
	u64_t fvco;
	u32_t new_rate;

	/* Existing / Old Method
	 * Determine the values to be programmed
	 * All internal calculations are in MHz
	 */
	new_rate = rate_mhz;
	/* To get the requested rate within the bounds of the min/max Fvco, we
	 * must increase the rate requested and increase the mdiv divisor to
	 * get as close as possible.
	 */
	*mdiv = MAX_FVCO_CLOCK_MHZ / new_rate;
	if (*mdiv > MAX_MDIV_VAL)
		*mdiv = MAX_MDIV_VAL;
	new_rate *= *mdiv;
	if (new_rate < MIN_FVCO_CLOCK_MHZ)
		new_rate = MIN_FVCO_CLOCK_MHZ;
	/* Remove the reference clock from the rate to have it only contain the
	 * pdiv and ndiv parts
	 */
	new_rate /= REF_CLK_FREQ_MHZ;
	/* Use pdiv to fit the rate into the ndiv_int min/max */
	*pdiv = MIN_NDIV_INT_VAL / new_rate;
	if (!(*pdiv))
		*pdiv = 1;
	*ndivint = new_rate * (*pdiv);
	if ((*mdiv) == MAX_MDIV_VAL)
		(*ndivint)++;
	*ndivfrac = 0;
	fvco = (*ndivint) * REF_CLK_FREQ_MHZ / (*pdiv);

	if ((*mdiv) > MAX_MDIV_VAL)
		SYS_LOG_ERR("mdiv of %d\n", *mdiv);
	if ((*ndivint) > MAX_NDIV_INT_VAL || (*ndivint) < MIN_NDIV_INT_VAL)
		SYS_LOG_ERR("ndiv_int of %d\n", (*ndivint));
	if (fvco > MAX_FVCO_CLOCK_MHZ || fvco < MIN_FVCO_CLOCK_MHZ)
		SYS_LOG_ERR("Fvco of %lld\n", fvco);
}

/* Notes from hardware:
 * From crm_aon_cfg_postproc.v
 *------------------------------
 * case (OTP_MAX_VCO[1:0])
 *    2'd1: if ((in_pll_ndiv_int >= NDIV_INT_MAX_1) || (in_pll_ndiv_int == 10'd0)) begin
 *             out_pll_ndiv_int           = NDIV_INT_MAX_1;
 *             out_pll_ndiv_frac_mode_sel = 1'b0;
 *             out_pll_ndiv_frac          = 20'h0;
 *          end
 *    2'd2: if ((in_pll_ndiv_int >= NDIV_INT_MAX_2) || (in_pll_ndiv_int == 10'd0)) begin
 *             out_pll_ndiv_int           = NDIV_INT_MAX_2;
 *             out_pll_ndiv_frac_mode_sel = 1'b0;
 *             out_pll_ndiv_frac          = 20'h0;
 *          end
 *    2'd3: if ((in_pll_ndiv_int >= NDIV_INT_MAX_3) || (in_pll_ndiv_int == 10'd0)) begin
 *             out_pll_ndiv_int           = NDIV_INT_MAX_3;
 *             out_pll_ndiv_frac_mode_sel = 1'b0;
 *             out_pll_ndiv_frac          = 20'h0;
 *          end
 * endcase
 *
 * Also mdiv and pdiv could be set to a higher numbers.  High mdiv and pdiv
 * yield lower output frequency are not likely used.
 *
 * assign out_pll_ch0_mdiv = ((OTP_MAX_VCO != 2'd0) &&
 *                          ((in_pll_ch0_mdiv != 9'd0) &&
 *                           // ((in_pll_ch0_mdiv * in_pll_pdiv) < (MDIV_MIN * PDIV_MIN)) &&
 *                           (in_pll_ch0_mdiv < MDIV_MIN))) ? MDIV_MIN : in_pll_ch0_mdiv;
 *
 * assign out_pll_pdiv = ((OTP_MAX_VCO != 2'd0) &&
 *                      // ((in_pll_ch0_mdiv * in_pll_pdiv) < (MDIV_MIN * PDIV_MIN)) &&
 *                      (in_pll_pdiv < PDIV_MIN)) ? PDIV_MIN : in_pll_pdiv;
 *
 * MDIV_MIN is 2 for OTP value of 2'b01 & PDIV_MIN is 1 for any OTP value.
 *
 * Algorithm Used:
 * Search for the best tuple <mdiv, pdiv, ndivint, ndivfrac> that
 * minimizes the difference (error) between expected frequency output
 * and desired frequency output. (resolution is in MHz).
 *
 */
static void calculate_parameters_new(const u32_t rate_mhz,
        u32_t *mdiv, u32_t *pdiv, u32_t *ndivint, u32_t *ndivfrac)
{
	/* range of mdiv */
	u32_t mdiv_min, mdiv_max;
	/* Iterators & temporary variables */
	u32_t m, p, md, pd, nint;
	u64_t fvco, nfrac, factual, error, minerror = -1;
	u32_t part_type = 0;

#ifdef CONFIG_BRCM_DRIVER_SOTP
	part_type = read_crmu_otp_max_vco();
#endif
	/* SYS_LOG_ERR("Part Type: %d", part_type); */
	mdiv_max = MAX_FVCO_CLOCK_MHZ / rate_mhz;
	mdiv_min = MIN_FVCO_CLOCK_MHZ / rate_mhz;
	mdiv_min = ((mdiv_min < 1) || (mdiv_min > MDIV_MAX)) ? 1 : mdiv_min;
	mdiv_max = (mdiv_max < 1) ? 1 : mdiv_max;
	mdiv_max = (mdiv_max > MDIV_MAX) ? MDIV_MAX : mdiv_max;

	/* Iterate over possible values of mdiv & pdiv */
	for (m = mdiv_min; m <= mdiv_max; m++) {
		fvco = rate_mhz * m;
		fvco = (fvco < MIN_FVCO_CLOCK_MHZ) ? MIN_FVCO_CLOCK_MHZ : fvco;
		fvco = (fvco > MAX_FVCO_CLOCK_MHZ) ? MAX_FVCO_CLOCK_MHZ : fvco;
		for (p = 1; p <= PDIV_MAX; p++) {
			nint = (fvco * p) / REF_CLK_FREQ_MHZ;
			nfrac = (fvco * p) % REF_CLK_FREQ_MHZ;
			/* This option is always more accurate */
			nfrac = (nfrac << 20) / REF_CLK_FREQ_MHZ;
			pd = p;
			md = m;
			switch (part_type) {
			case PART_TYPE_500MHZ:
				pd = 1;
				if (md < 2)
					md = 2;
				if (nint >= NDIV_INT_MAX_2 || nint == 0) {
					nint = NDIV_INT_MAX_2;
					nfrac = 0;
				} else if (nint < NDIV_INT_MAX_3) {
					nint = NDIV_INT_MAX_3;
				}
				break;
			case PART_TYPE_1GHZ:
				if (nint >= NDIV_INT_MAX_1 || nint == 0) {
					nint = NDIV_INT_MAX_1;
					nfrac = 0;
				}
				break;
			default:
				if (nint >= NDIV_INT_MAX_0 || nint == 0) {
					nint = NDIV_INT_MAX_0;
				}
				break;
			}
			/* Compute factual using the above parameters */
			factual = (nint + (nfrac >> 20)) * REF_CLK_FREQ_MHZ / (pd * md);
			error = (factual > rate_mhz) ? (factual - rate_mhz) : (rate_mhz - factual);
			if (error < minerror) {
				/* Update results */
				*mdiv = (md == MDIV_MAX) ? 0 : md;
				*pdiv = pd;
				*ndivint = (nint == NDIV_INT_MAX_0) ? 0 : nint;
				*ndivfrac = nfrac;
				minerror = error;
			}
		}
	}
}

int compare_parameters(struct device *dev, const u32_t rate_mhz, struct clock_params *p)
{
	ARG_UNUSED(dev);

	/* New Method with Optimizations for better accuracy */
	calculate_parameters_new(rate_mhz, &p->mdiv[0], &p->pdiv[0],
							 &p->ndivint[0], &p->ndivfrac[0]);
	p->fvco[0] = ((p->ndivint[0] + (p->ndivfrac[0] >> 20)) * REF_CLK_FREQ_MHZ) / p->pdiv[0];
	p->resrate[0] = (p->fvco[0] / p->mdiv[0]);
	p->error[0] = (rate_mhz > p->resrate[0]) ? (rate_mhz - p->resrate[0]) : (p->resrate[0] - rate_mhz);

	/* Old Method */
	calculate_parameters_old(rate_mhz, &p->mdiv[1], &p->pdiv[1],
							 &p->ndivint[1], &p->ndivfrac[1]);
	p->fvco[1] = ((p->ndivint[1] + (p->ndivfrac[1] >> 20)) * REF_CLK_FREQ_MHZ) / p->pdiv[1];
	p->resrate[1] = (p->fvco[1] / p->mdiv[1]);
	p->error[1] = (rate_mhz > p->resrate[1]) ? (rate_mhz - p->resrate[1]) : (p->resrate[1] - rate_mhz);

	return 0;
}

int get_effective_parameters(struct device *dev,
							  struct clock_params *p, u32_t method)
{
	const struct citadel_a7_cfg_data *cfg = dev->config->config_info;
	mem_addr_t base = cfg->base;

	/* Read out the effective pdiv, ndiv_int & mdiv values from status registers */
	p->effpdiv = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_0);
	p->effpdiv = (p->effpdiv >> A7_CRM_PLL0_CTRL3_PLL0_PDIV) & 0xf;
	p->effndivint = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_0) & 0x3ff;
	p->effmdiv = sys_read32(base + A7_CRM_PLL_CTRL_STATUS_1) & 0x1ff;
	p->efffvco = (p->effndivint + (p->ndivfrac[method] >> 20)) * REF_CLK_FREQ_MHZ;
	p->efffvco = p->efffvco / p->effpdiv;
	p->effresrate = (p->efffvco / p->effmdiv);

	return 0;
}

s32_t apply_parameters(struct device *dev,
			  u32_t mdiv, u32_t pdiv, u32_t ndiv_int, u32_t ndiv_frac)
{
	const struct citadel_a7_cfg_data *cfg;
	mem_addr_t base;
	u32_t val, current_rate;
	u64_t fvco;

	if (!dev)
		return -EINVAL;

	cfg = dev->config->config_info;
	base = cfg->base;

	/* Check to see if the rate that would be programmed would change the
	 * current rate.  If not, don't make the change and exit without error
	 */
	fvco = ((ndiv_int + (ndiv_frac >> 20)) * REF_CLK_FREQ_MHZ) / pdiv;
	current_rate = a7pll_determine_rate(dev) / MHZ(1);
	if (current_rate == (u32_t)(fvco / mdiv)) {
		SYS_LOG_DBG("Keeping CPU freq of %lld (%lld / %d)", (fvco / mdiv), fvco, mdiv);
		return 0;
	} else
		SYS_LOG_DBG("CPU freq will be %lld (%lld / %d)", (fvco / mdiv), fvco, mdiv);

	/* Enable bypass PLL */
	val = sys_read32(base + A7_CRM_PLL_CHAN_BYPASS);
	val |= 1 << A7_CRM_PLL_CHAN_BYPASS_CHAN_0;
	val |= 1 << A7_CRM_PLL_CHAN_BYPASS_CHAN_4;
	sys_write32(val, base + A7_CRM_PLL_CHAN_BYPASS);

	/* Assert both PLL Reset & PLL Post Reset */
	val = sys_read32(base + A7_CRM_PLL_PWR_ON);
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_RESETB);
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_POST_RESETB);
	sys_write32(val, base + A7_CRM_PLL_PWR_ON);

	val = sys_read32(base + A7_CRM_PLL_IRQ_STATUS);
	val |= 1 << A7_CRM_PLL_IRQ_PLL0_LOCK_LOST;
	val |= 1 << A7_CRM_PLL_IRQ_PLL0_LOCK;
	sys_write32(val, base + A7_CRM_PLL_IRQ_STATUS);

	val = sys_read32(base + A7_CRM_IRQ_EN);
	val |= 1 << A7_CRM_IRQ_EN_PLL0_IRQ_EN;
	sys_write32(val, base + A7_CRM_IRQ_EN);

	val = sys_read32(base + A7_CRM_PLL_IRQ_EN);
	val |= 1 << A7_CRM_PLL_IRQ_EN_PLL0_LOCK_LOST_IRQ;
	val |= 1 << A7_CRM_PLL_IRQ_EN_PLL0_LOCK_IRQ;
	sys_write32(val, base + A7_CRM_PLL_IRQ_EN);

	/* Apply ndiv_int & mdiv to obtain the desired core freq */
	sys_write32((u32_t) (ndiv_frac & 0x3ff), base + A7_CRM_PLL0_CTRL1);
	sys_write32((u32_t) ((ndiv_frac >> 10) & 0x3ff), base + A7_CRM_PLL0_CTRL2);
	sys_write32((ndiv_int & 0x3ff) | ((pdiv & 0xf) << A7_CRM_PLL0_CTRL3_PLL0_PDIV),
		    base + A7_CRM_PLL0_CTRL3);

	/* FIXME - Values taken from SBL code.  Unsure of their meaning, but
	 * lack documentation to determine it.
	 *
	 * PLL0_FREFEFF_INFO: (16FF only) Effective reference frequency
	 * (Fref/PDIV) information supplied by the user.
	 * 6'd0: PLL will pick up user supplied ka, ki, kp, pwm_rate
	 * and vco_fb_div2
	 * Others: PLL automatically selects ka, ki, kp, pwm_rate and
	 * vco_fb_div2
	 * 6'd10: 10 MHz;
	 * 6'd11: (10, 11] MHz; ... ;
	 * 6'd60: (59,60] MHz
	 * 6'd61~6'd63: not used
	 * When i_32k_refclock_sel=1,
	 * 6'b000001: automatic selection mode
	 * 6'b000000: manual selection mode
	 * NOTE: default value is 26 for Shamu
	 */
	val = 0xa << A7_CRM_PLL0_CTRL4_PLL0_KP;
	val |= 3 << A7_CRM_PLL0_CTRL4_PLL0_KI;
	val |= 3 << A7_CRM_PLL0_CTRL4_PLL0_KA;
	val |= 26 << A7_CRM_PLL0_CTRL4_PLL0_FREFEFF_INFO;
	sys_write32(val, base + A7_CRM_PLL0_CTRL4);

	sys_write32(0, base + A7_CRM_PLL0_CFG0_CTRL);
	sys_write32(0, base + A7_CRM_PLL0_CFG1_CTRL);
	sys_write32(0, base + A7_CRM_PLL0_CFG2_CTRL);
	sys_write32(0, base + A7_CRM_PLL0_CFG3_CTRL);

	val = sys_read32(base + A7_CRM_PLL0_CHAN_CTRL);
	val |= 3 << A7_CRM_PLL0_CHAN_CTRL_PLL0_CHAN0_EN_SEL;
	sys_write32(val, base + A7_CRM_PLL0_CHAN_CTRL);

	val = sys_read32(base + A7_CRM_PLL0_CHAN0_DESCRIPTION);
	val &= ~0x1ff;
	val |= (mdiv & 0x1ff) << A7_CRM_PLL0_CHAN0_DESC_PLL0_CHAN0_MDIV;
	sys_write32(val, base + A7_CRM_PLL0_CHAN0_DESCRIPTION);

	/* Update PLL */
	val = sys_read32(base + A7_CRM_PLL_CMD);
	val |= 1 << A7_CRM_PLL_CMD_UPDATE_PLL0_FREQ_VCO;
	val |= 1 << A7_CRM_PLL_CMD_UPDATE_PLL0_FREQ_POST;
	sys_write32(val, base + A7_CRM_PLL_CMD);

	/* De-assert PLL Reset */
	val = sys_read32(base + A7_CRM_PLL_PWR_ON);
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_PWRON_PLL;
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_PWRON_LDO;
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_ISO_PLL);
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_ISO_PLLOUT);
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_RESETB;
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_POST_RESETB);
	sys_write32(val, base + A7_CRM_PLL_PWR_ON);

	/* Wait for PLL lock */
	do {
	} while (!sys_test_bit(base + A7_CRM_PLL_STATUS, A7_CRM_PLL_PLL0_LOCK));

	/* De-assert PLL Post Reset */
	sys_write32(1, base + A7_CRM_SUBSYSTEM_CFG_2);

	/* PLL reset - post-div channel reset */
	val = sys_read32(base + A7_CRM_PLL_PWR_ON);
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_PWRON_PLL;
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_PWRON_LDO;
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_ISO_PLL);
	val &= ~(1 << A7_CRM_PLL_PWR_ON_PLL0_ISO_PLLOUT);
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_RESETB;
	val |= 1 << A7_CRM_PLL_PWR_ON_PLL0_POST_RESETB;
	sys_write32(val, base + A7_CRM_PLL_PWR_ON);

	/* Disable bypass PLL */
	val = sys_read32(base + A7_CRM_PLL_CHAN_BYPASS);
	val &= ~(1 << A7_CRM_PLL_CHAN_BYPASS_CHAN_0);
	val &= ~(1 << A7_CRM_PLL_CHAN_BYPASS_CHAN_4);
	sys_write32(val, base + A7_CRM_PLL_CHAN_BYPASS);

	/* Data sync barrier to ensure the writes are finished */
	__asm__ volatile("dsb");

	return 0;
}

int citadel_a7_clock_control_set_subsys_rate(struct device *dev,
						    clock_control_subsys_t sys,
						    const u32_t rate)
{
	u32_t pdiv, mdiv, ndiv_int, ndiv_frac, new_rate;

	ARG_UNUSED(sys);

	if (!dev)
		return -EINVAL;

	SYS_LOG_DBG("Attempting to set clock speed of CPU to %d", rate);

	/* Determine the values to be programmed */
	/* All internal calculations are in MHz */
	new_rate = rate / MHZ(1);
	calculate_parameters_new(new_rate, &mdiv, &pdiv, &ndiv_int, &ndiv_frac);

	SYS_LOG_DBG("fvco(mhz) = (1 / %d) * (%d + %d / 1MHZ) * REF_CLK_FREQ_MHZ = %lld",
		    pdiv, ndiv_int, ndiv_frac, (ndiv_int * REF_CLK_FREQ_MHZ / pdiv));

	apply_parameters(dev, mdiv, pdiv, ndiv_int, ndiv_frac);

	return 0;
}

static struct citadel_a7_cfg_data citadel_a7_cfg = {
	.base = BRCM_CIT_ARMPLL_56010000_BASE_ADDRESS_0,
};

static struct clock_control_driver_api citadel_a7_clock_control_api = {
	.on = citadel_a7_clock_control_on,
	.off = citadel_a7_clock_control_off,
	.get_rate = citadel_a7_clock_control_get_subsys_rate,
	.set_rate = citadel_a7_clock_control_set_subsys_rate,
	.compare = compare_parameters,
	.get_effective_params = get_effective_parameters,
	.apply = apply_parameters,
};

static int citadel_a7_clock_control_init(struct device *dev)
{
	if (!dev)
		return -EINVAL;

	/* Per Arch's Recommendation, default A7 CPU frequency is left
	 * at 208MHz. Applications needing higher frequency are expected
	 * to scale the frequency to 1GHz (or desired frequency) before
	 * running their application. e.g. Finger Print scanner might
	 * bump the frequency to 1GHz before starting scan.
	 */
	clk_a7_set(MHZ(208));

	SYS_LOG_INF("CPU running at %dMHz", a7pll_determine_rate(dev) / MHZ(1));

	return  0;
}

DEVICE_AND_API_INIT(citadel_a7_clocks, A7_CLOCK_CONTROL_NAME,
		    citadel_a7_clock_control_init, NULL, &citadel_a7_cfg,
		    PRE_KERNEL_1, CONFIG_A7_CLOCK_CONTROL_INIT_PRIORITY,
		    &citadel_a7_clock_control_api);
