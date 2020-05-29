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

#include <stdbool.h>
#include <errno.h>
#include <device.h>
#include <pinmux.h>
#include <toolchain.h>
#include <board.h>
#include <arch/cpu.h>
#include <logging/sys_log.h>

#define CITADEL_PINMUX_IOF	4
#define CITADEL_PINMUX_IOF_MASK	0x7
#define CITADEL_PINMUX_PINS	69

/* Citadel MFIO block has pins numbered from 0 - 68, and then 71.
 * No pin 69 and 70. In addition the address for control register 71
 * cannot be computed as base + 4*n, as for other register. Hence programming
 * this pin needs special handling
 */
#define CITADEL_PINMUX_PIN_71	71

struct iomux_citadel_data {
	bool pinset[CITADEL_PINMUX_PIN_71 + 1];
};

struct iomux_citadel_config {
	u32_t base;
	u32_t base71;
};

static int iomux_citadel_get(struct device *dev, u32_t pin, u32_t *func);

static int iomux_citadel_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct iomux_citadel_config *pcc = dev->config->config_info;
	struct iomux_citadel_data *icd = dev->driver_data;
	u32_t val, addr;

	if ((func & CITADEL_PINMUX_IOF_MASK) > CITADEL_PINMUX_IOF)
		return -EINVAL;

	if ((pin >= CITADEL_PINMUX_PINS) && (pin != CITADEL_PINMUX_PIN_71))
		return -EINVAL;

	if (!icd->pinset[pin]) {
		icd->pinset[pin] = true;
	} else {
#ifdef CONFIG_ALLOW_IOMUX_OVERRIDE
		{
			u32_t current_func;

			iomux_citadel_get(dev, pin, &current_func);
			SYS_LOG_WRN("Overriding pin %d function: %x -> %x",
				    pin, current_func, func);
		}
#else
		SYS_LOG_ERR("trying to use pin %d, but already used", pin);
		return -EINVAL;
#endif
	}

	if (pin == CITADEL_PINMUX_PIN_71)
		addr = pcc->base71;
	else
		addr = pcc->base + (pin * 4);

	val = func;

	SYS_LOG_DBG("%s: writing to %x", __func__, addr);
	sys_write32(val, addr);

	return 0;
}

static int iomux_citadel_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct iomux_citadel_config *pcc = dev->config->config_info;
	u32_t addr;

	if (func == NULL)
		return -EINVAL;

	if ((pin >= CITADEL_PINMUX_PINS) && (pin != CITADEL_PINMUX_PIN_71))
		return -EINVAL;

	if (pin == CITADEL_PINMUX_PIN_71)
		addr = pcc->base71;
	else
		addr = pcc->base + (pin * 4);

	*func = sys_read32(addr);

	return 0;
}

static int iomux_citadel_pullup(struct device *dev, u32_t pin, u8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENOTSUP;
}

static int iomux_citadel_input(struct device *dev, u32_t pin, u8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENOTSUP;
}

static int iomux_citadel_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct pinmux_driver_api iomux_citadel_driver_api = {
	.set = iomux_citadel_set,
	.get = iomux_citadel_get,
	.pullup = iomux_citadel_pullup,
	.input = iomux_citadel_input,
};

static const struct iomux_citadel_config iomux_citadel_0_config = {
	.base = CITADEL_PINMUX_0_BASE_ADDR,
	.base71 = CITADEL_PINMUX_71_BASE_ADDR,
};

static struct iomux_citadel_data iomux_citadel_0_data;

DEVICE_AND_API_INIT(iomux_citadel_0, CONFIG_PINMUX_CITADEL_0_NAME,
		    &iomux_citadel_init, &iomux_citadel_0_data,
		    &iomux_citadel_0_config, PRE_KERNEL_1,
		    CONFIG_IOMUX_INIT_PRIORITY, &iomux_citadel_driver_api);
