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

/* @file li_v024m.c
 *
 * Driver for Leopard Imaging's V024M camera module
 *
 * This driver provides apis to configure Leopard Imaging's V024M camera module.
 * The camera module contains a Aptina's (now OnSemi) 1/3" Wide-VGA MT9V024
 * CMOS image sensor. The sensor supports a parallel output which is converted
 * to a MIPI CSI 2 compliant serial output using a bridge IC (Toshiba's
 * TC358746AXBG). The MIPI data/clock lanes from the bridge IC is connected
 * to the unicam controllers CSI pins.
 *
 */

#include <board.h>
#include <unicam.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <broadcom/i2c.h>
#include "unicam_bcm5820x_csi.h"
#include "unicam_bcm5820x_regs.h"
#include "li_v024m.h"

#ifdef CONFIG_LI_V024M_CAMERA

#define MAX_CAPTURE_WIDTH	752
#define MAX_CAPTURE_HEIGHT	480
#define MAX_FRAMES_PER_SEC	60

/* Maximum blanking value configurable */
#define MAX_BLANKING		0x3FF

/* FIXME: Without the additional horizontal blanking
 * the capture image appears to have artifacts at the
 * the point where white turns to black and vice versa
 */
#define EXTRA_HORIZ_BLANKING	817

/* For debugging only */
/* #define CONFIG_COLORBAR_ENABLE */

/* Toshiba Bridge register info */
struct tc358746_reg_struct {
	u8_t	size;
	u16_t	addr;
	u32_t	val;
};

/* On Semi sensor register info */
struct mt9v024_reg_struct {
	u8_t	addr;
	u16_t	val;
};

/* Sensor configuration registers - Provided by Leopard imaging */
static struct mt9v024_reg_struct sensor_regs[] = {
	/* Set this to 0x1 for WVGA */
	{0x01, 0x0038},	/* COL_WINDOW_START_CONTEXTA_REG */
	{0x02, 0x0004},	/* ROW_WINDOW_START_CONTEXTA_REG */
	{0x03, 0x01E0},	/* ROW_WINDOW_SIZE_CONTEXTA_REG */
	/* Set this to 0x2F0 for WVGA */
	{0x04, 0x0280},	/* COL_WINDOW_SIZE_CONTEXTA_REG */
	/* Set this to 0x5D for WVGA */
	{0x05, 0x005E},	/* HORZ_BLANK_CONTEXTA_REG */
	/* Set this to 0x34 for WVGA */
	{0x06, 0x0039},	/* VERT_BLANK_CONTEXTA_REG (0x0041) tweak to 59.995 */
	{0x07, 0x0188},	/* CONTROL_MODE_REG */
	{0x08, 0x0190},	/* COARSE_SHUTTER_WIDTH_1_CONTEXTA */
	{0x09, 0x01BD},	/* COARSE_SHUTTER_WIDTH_2_CONTEXTA */
	{0x0A, 0x0164},	/* SHUTTER_WIDTH_CONTROL_CONTEXTA */
	/* Set this to 0x20 for WVGA */
	{0x0B, 0x01C2},	/* COARSE_SHUTTER_WIDTH_TOTAL_CONTEXTA */
	{0x0C, 0x0000},	/* RESET_REG */
	{0x0D, 0x0300},	/* READ_MODE_REG */
	{0x0E, 0x0000},	/* READ_MODE2_REG */
	{0x0F, 0x0100},	/* PIXEL_OPERATION_MODE */
	{0x10, 0x0040},	/* RESERVED_CORE_10 */
	{0x11, 0x8042},	/* RESERVED_CORE_11 */
	{0x12, 0x0022},	/* RESERVED_CORE_12 */
	{0x13, 0x2D2E},	/* RESERVED_CORE_13 */
	{0x14, 0x0E02},	/* RESERVED_CORE_14 */
	{0x15, 0x0E32},	/* RESERVED_CORE_15 */
	{0x16, 0x2802},	/* RESERVED_CORE_16 */
	{0x17, 0x3E38},	/* RESERVED_CORE_17 */
	{0x18, 0x3E38},	/* RESERVED_CORE_18 */
	{0x19, 0x2802},	/* RESERVED_CORE_19 */
	{0x1A, 0x0428},	/* RESERVED_CORE_1A */
	{0x1B, 0x0000},	/* LED_OUT_CONTROL */
	{0x1C, 0x0302},	/* DATA_COMPRESSION */
	{0x1D, 0x0040},	/* RESERVED_CORE_1D */
	{0x1E, 0x0000},	/* RESERVED_CORE_1E */
	{0x1F, 0x0000},	/* RESERVED_CORE_1F */
	{0x20, 0x03C7},	/* RESERVED_CORE_20 */
	{0x21, 0x0020},	/* RESERVED_CORE_21 */
	{0x22, 0x0020},	/* RESERVED_CORE_22 */
	{0x23, 0x0010},	/* RESERVED_CORE_23 */
	{0x24, 0x001B},	/* RESERVED_CORE_24 */
	{0x25, 0x001A},	/* RESERVED_CORE_25 */
	{0x26, 0x0004},	/* RESERVED_CORE_26 */
	{0x27, 0x000C},	/* RESERVED_CORE_27 */
	{0x28, 0x0010},	/* RESERVED_CORE_28 */
	{0x29, 0x0010},	/* RESERVED_CORE_29 */
	{0x2A, 0x0020},	/* RESERVED_CORE_2A */
	{0x2B, 0x0003},	/* RESERVED_CORE_2B */
	{0x2C, 0x0004},	/* VREF_ADC_CONTROL */
	{0x2D, 0x0004},	/* RESERVED_CORE_2D */
	{0x2E, 0x0007},	/* RESERVED_CORE_2E */
	{0x2F, 0x0003},	/* RESERVED_CORE_2F */
	{0x30, 0x0003},	/* RESERVED_CORE_30 */
	{0x31, 0x001F},	/* V1_CONTROL_CONTEXTA */
	{0x32, 0x001A},	/* V2_CONTROL_CONTEXTA */
	{0x33, 0x0012},	/* V3_CONTROL_CONTEXTA */
	{0x34, 0x0003},	/* V4_CONTROL_CONTEXTA */
	{0x35, 0x0020},	/* GLOBAL_GAIN_CONTEXTA_REG */
	{0x36, 0x0010},	/* GLOBAL_GAIN_CONTEXTB_REG */
	{0x37, 0x0000},	/* RESERVED_CORE_37 */
	{0x38, 0x0000},	/* RESERVED_CORE_38 */
	{0x39, 0x0025},	/* V1_CONTROL_CONTEXTB */
	{0x3A, 0x0020},	/* V2_CONTROL_CONTEXTB */
	{0x3B, 0x0003},	/* V3_CONTROL_CONTEXTB */
	{0x3C, 0x0003},	/* V4_CONTROL_CONTEXTB */
	{0x46, 0x231D},	/* DARK_AVG_THRESHOLDS */
	{0x47, 0x0080},	/* CALIB_CONTROL_REG */
	{0x4C, 0x0002},	/* STEP_SIZE_AVG_MODE */
	{0x70, 0x0000},	/* ROW_NOISE_CONTROL */
	{0x71, 0x002A},	/* NOISE_CONSTANT */
	{0x72, 0x0000},	/* PIXCLK_CONTROL */
	{0x7F, 0x0000},	/* TEST_DATA */
	{0x80, 0x04F4},	/* TILE_X0_Y0 */
	{0x81, 0x04F4},	/* TILE_X1_Y0 */
	{0x82, 0x04F4},	/* TILE_X2_Y0 */
	{0x83, 0x04F4},	/* TILE_X3_Y0 */
	{0x84, 0x04F4},	/* TILE_X4_Y0 */
	{0x85, 0x04F4},	/* TILE_X0_Y1 */
	{0x86, 0x04F4},	/* TILE_X1_Y1 */
	{0x87, 0x04F4},	/* TILE_X2_Y1 */
	{0x88, 0x04F4},	/* TILE_X3_Y1 */
	{0x89, 0x04F4},	/* TILE_X4_Y1 */
	{0x8A, 0x04F4},	/* TILE_X0_Y2 */
	{0x8B, 0x04F4},	/* TILE_X1_Y2 */
	{0x8C, 0x04F4},	/* TILE_X2_Y2 */
	{0x8D, 0x04F4},	/* TILE_X3_Y2 */
	{0x8E, 0x04F4},	/* TILE_X4_Y2 */
	{0x8F, 0x04F4},	/* TILE_X0_Y3 */
	{0x90, 0x04F4},	/* TILE_X1_Y3 */
	{0x91, 0x04F4},	/* TILE_X2_Y3 */
	{0x92, 0x04F4},	/* TILE_X3_Y3 */
	{0x93, 0x04F4},	/* TILE_X4_Y3 */
	{0x94, 0x04F4},	/* TILE_X0_Y4 */
	{0x95, 0x04F4},	/* TILE_X1_Y4 */
	{0x96, 0x04F4},	/* TILE_X2_Y4 */
	{0x97, 0x04F4},	/* TILE_X3_Y4 */
	{0x98, 0x04F4},	/* TILE_X4_Y4 */
	{0x99, 0x0000},	/* X0_SLASH5 */
	{0x9A, 0x0096},	/* X1_SLASH5 */
	{0x9B, 0x012C},	/* X2_SLASH5 */
	{0x9C, 0x01C2},	/* X3_SLASH5 */
	{0x9D, 0x0258},	/* X4_SLASH5 */
	{0x9E, 0x02F0},	/* X5_SLASH5 */
	{0x9F, 0x0000},	/* Y0_SLASH5 */
	{0xA0, 0x0060},	/* Y1_SLASH5 */
	{0xA1, 0x00C0},	/* Y2_SLASH5 */
	{0xA2, 0x0120},	/* Y3_SLASH5 */
	{0xA3, 0x0180},	/* Y4_SLASH5 */
	{0xA4, 0x01E0},	/* Y5_SLASH5 */
	{0xA5, 0x003A},	/* DESIRED_BIN */
	{0xA6, 0x0002},	/* EXP_SKIP_FRM_H */
	{0xA8, 0x0000},	/* EXP_LPF */
	{0xA9, 0x0002},	/* GAIN_SKIP_FRM */
	{0xAA, 0x0002},	/* GAIN_LPF_H */
	{0xAB, 0x0040},	/* MAX_GAIN */
	{0xAC, 0x0001},	/* MIN_COARSE_EXPOSURE */
	{0xAD, 0x01E0},	/* MAX_COARSE_EXPOSURE */
	{0xAE, 0x0014},	/* BIN_DIFF_THRESHOLD */
	{0xAF, 0x0033},	/* AUTO_BLOCK_CONTROL,
			 * Enable AE and AG (both context A and context B)
			 */
	{0xB0, 0xABE0},	/* PIXEL_COUNT */
	{0xB1, 0x0002},	/* LVDS_MASTER_CONTROL */
	{0xB2, 0x0010},	/* LVDS_SHFT_CLK_CONTROL */
	{0xB3, 0x0010},	/* LVDS_DATA_CONTROL */
	{0xB4, 0x0000},	/* LVDS_DATA_STREAM_LATENCY */
	{0xB5, 0x0000},	/* LVDS_INTERNAL_SYNC */
	{0xB6, 0x0000},	/* LVDS_USE_10BIT_PIXELS */
	{0xB7, 0x0000},	/* STEREO_ERROR_CONTROL */
	{0xBF, 0x0016},	/* INTERLACE_FIELD_VBLANK */
	{0xC0, 0x000A},	/* IMAGE_CAPTURE_NUM */
	{0xC2, 0x18D0},	/* ANALOG_CONTROLS */
	{0xC3, 0x007F},	/* RESERVED_CORE_C3 */
	{0xC4, 0x007F},	/* RESERVED_CORE_C4 */
	{0xC5, 0x007F},	/* RESERVED_CORE_C5 */
	{0xC6, 0x0000},	/* NTSC_FV_CONTROL */
	{0xC7, 0x4416},	/* NTSC_HBLANK */
	{0xC8, 0x4421},	/* NTSC_VBLANK */
	{0xC9, 0x0002},	/* COL_WINDOW_START_CONTEXTB_REG */
	{0xCA, 0x0004},	/* ROW_WINDOW_START_CONTEXTB_REG */
	{0xCB, 0x01E0},	/* ROW_WINDOW_SIZE_CONTEXTB_REG */
	{0xCC, 0x02EE},	/* COL_WINDOW_SIZE_CONTEXTB_REG */
	{0xCD, 0x0100},	/* HORZ_BLANK_CONTEXTB_REG */
	{0xCE, 0x0100},	/* VERT_BLANK_CONTEXTB_REG */
	{0xCF, 0x0190},	/* COARSE_SHUTTER_WIDTH_1_CONTEXTB */
	{0xD0, 0x01BD},	/* COARSE_SHUTTER_WIDTH_2_CONTEXTB */
	{0xD1, 0x0064},	/* SHUTTER_WIDTH_CONTROL_CONTEXTB */
	{0xD2, 0x01C2},	/* COARSE_SHUTTER_WIDTH_TOTAL_CONTEXTB */
	{0xD3, 0x0000},	/* FINE_SHUTTER_WIDTH_1_CONTEXTA */
	{0xD4, 0x0000},	/* FINE_SHUTTER_WIDTH_2_CONTEXTA */
	{0xD5, 0x0000},	/* FINE_SHUTTER_WIDTH_TOTAL_CONTEXTA */
	{0xD6, 0x0000},	/* FINE_SHUTTER_WIDTH_1_CONTEXTB */
	{0xD7, 0x0000},	/* FINE_SHUTTER_WIDTH_2_CONTEXTB */
	{0xD8, 0x0000},	/* FINE_SHUTTER_WIDTH_TOTAL_CONTEXTB */
	{0xD9, 0x0000},	/* MONITOR_MODE_CONTROL */
	{0xC2, 0x18D0},	/* ANALOG_CONTROLS */
	{0x07, 0x0388}, /* CONTROL_MODE_REG: 0x388 master mode, 0x398 snapshot*/
};

/*} Bridge register settings - Provided by Leopard imaging */
static struct tc358746_reg_struct bridge_regs[] = {
	{2, 0x0004, 0x0004},
	{2, 0x0002, 0x0001},
	{2, 0x0002, 0x0000},
	{2, 0x0016, 0x504D},
	{2, 0x0018, 0x0213},
	{2, 0x0006, 0x0064},	 /* Set to 0x78 for WVGA */
	{2, 0x0008, 0x0010},	 /* RAW10 */
	{2, 0x0022, 0x0320},	 /* Set to 0x3AC for WVGA */
	{4, 0x0140, 0x00000000}, /* Enable clock lane */
	{4, 0x0144, 0x00000000}, /* Enable data lane 0 */
	{4, 0x0148, 0x00000001}, /* Disable data lane 1 */
	{4, 0x014C, 0x00000001}, /* Disable data lane 2 */
	{4, 0x0150, 0x00000001}, /* Disable data lane 3 */
	{4, 0x0210, 0x00001200},
	{4, 0x0214, 0x00000002}, /* Modified from 0x1 as it was violating
				  * LPTimeCount (LP Tx Count) parameter
				  */
	{4, 0x0218, 0x00000E02}, /* Modified from 0x701 as it was violating
				  * the timing spec for TCLK/TPREPARE
				  */
	{4, 0x021C, 0x00000000},
	{4, 0x0220, 0x00000002},
	{4, 0x0224, 0x00004988},
	{4, 0x0228, 0x00000007}, /* Should be set to > 48/8 for unicam */
	{4, 0x022C, 0x00000001}, /* Modified from 0x0 as it was violating
				  * THS_TRAILCNT parameter
				  */
	{4, 0x0234, 0x00000003}, /* 0x7 to enable regulator for data lane 1 */
	{4, 0x0238, 0x00000000},
	{4, 0x0204, 0x00000001}, /* TX PPI starts */
	{4, 0x0518, 0x00000001},
	{4, 0x0500, 0xA30080A1}, /* 0xA30080A3 to enable data lane 1 */
	{2, 0x0004, 0x0044},     /* 0x45 to activate data lane 1 */
};

/* Read v024m sensor register */
static int v024m_read(struct device *i2c_dev, u8_t reg_addr, u16_t *val)
{
	int ret;
	u8_t value[2];


	ret = i2c_burst_read(i2c_dev, V024M_SENSOR_ADDR,
			     reg_addr, value, sizeof(value));

	if (ret == 0)
		*val = ((u16_t)(value[0]) << 8) | value[1];

	return ret;
}

/* Write v024m sensor register */
int v024m_write(struct device *i2c_dev, u8_t reg_addr, u16_t val)
{
	struct i2c_msg msg;
	u8_t buff[3];

	buff[0] = reg_addr;
	buff[1] = (u8_t)(val >> 8);
	buff[2] = (u8_t)(val & 0xFF);
	msg.buf = buff;
	msg.len = sizeof(buff);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, V024M_SENSOR_ADDR);
}

/* Toshiba TC358746 IC, 16 bit register read */
static int tc358746_read_16(struct device *i2c_dev, u16_t reg_addr, u16_t *val)
{
	int ret;
	u8_t addr[2], value[2];
	struct i2c_msg msgs[2];

	addr[0] = reg_addr >> 8;
	addr[1] = reg_addr & 0xFF;

	msgs[0].buf = addr;
	msgs[0].len = sizeof(addr);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	msgs[1].buf = value;
	msgs[1].len = sizeof(value);
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msgs, 2, MIPI_BRIDGE_ADDR);

	if (ret == 0)
		*val = ((u16_t)(value[0]) << 8) | value[1];

	return ret;
}

/* Toshiba TC358746 IC, 32 bit register read */
int tc358746_read_32(struct device *i2c_dev, u16_t reg_addr, u32_t *val)
{
	int ret;
	u8_t addr[2], value[4];
	struct i2c_msg msgs[2];

	addr[0] = reg_addr >> 8;
	addr[1] = reg_addr & 0xFF;

	msgs[0].buf = addr;
	msgs[0].len = sizeof(addr);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	msgs[1].buf = value;
	msgs[1].len = sizeof(value);
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msgs, 2, MIPI_BRIDGE_ADDR);

	/* For 32 bit read, the data is read in the order
	 * DATA[15:8], DATA[7:0], DATA[31:24], DATA[23:16]
	 */
	if (ret == 0)
		*val = ((u32_t)(value[2]) << 24) | ((u32_t)(value[3]) << 16) |
			((u32_t)(value[0]) << 8) | (u32_t)value[1];

	return ret;
}

/* Toshiba TC358746 IC, 16 bit register write */
int tc358746_write_16(struct device *i2c_dev, u16_t reg_addr, u16_t val)
{
	struct i2c_msg msg;
	u8_t buff[4];

	buff[0] = (u8_t)(reg_addr >> 8);
	buff[1] = (u8_t)(reg_addr & 0xFF);
	buff[2] = (u8_t)(val >> 8);
	buff[3] = (u8_t)(val & 0xFF);
	msg.buf = buff;
	msg.len = sizeof(buff);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, MIPI_BRIDGE_ADDR);
}

/* Toshiba TC358746 IC, 32 bit register write */
int tc358746_write_32(struct device *i2c_dev, u16_t reg_addr, u32_t val)
{
	struct i2c_msg msg;
	u8_t buff[6];

	buff[0] = (u8_t)(reg_addr >> 8);
	buff[1] = (u8_t)(reg_addr & 0xFF);
	/* For 32 bit write, the data is written in the order
	 * DATA[15:8], DATA[7:0], DATA[31:24], DATA[23:16]
	 */
	buff[2] = (u8_t)(val >> 8);
	buff[3] = (u8_t)(val & 0xFF);
	buff[4] = (u8_t)(val >> 24);
	buff[5] = (u8_t)(val >> 16);
	msg.buf = buff;
	msg.len = sizeof(buff);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, MIPI_BRIDGE_ADDR);
}

/* Update sensor_regs with given value for a given address */
static void set_sensor_reg(u8_t addr, u16_t value)
{
	u32_t i;

	for (i = 0; i < ARRAY_SIZE(sensor_regs); i++) {
		if (sensor_regs[i].addr == addr) {
			sensor_regs[i].val = value;
			break;
		}
	}
}

/* Update bridge_regs with given value for a given address */
static void set_bridge_reg(u16_t addr, u32_t value)
{
	u32_t i;

	for (i = 0; i < ARRAY_SIZE(bridge_regs); i++) {
		if (bridge_regs[i].addr == addr) {
			bridge_regs[i].val = value;
			break;
		}
	}
}

/*
 * @brief Initialize the LI V024M camera module
 * @details This function checks that the camera module is accessible through
 *	    it's I2C slave addresses (Toshiba's bridge IC and Aptina's V024M
 *	    sensor). In addition it also configures the lane timer values for
 *	    clock and data lanes in the Unicam controller. These values are
 *	    module specific and needs to be programmed here.
 *
 * @param i2c - I2C device information
 *
 * @return 0 on Success, -errno otherwise
 */
int li_v024m_init(struct i2c_dev_info *i2c)
{
	int ret;
	u16_t id;
	struct unicam_lane_timers timers;

	/* Verify the I2C slaves are accessible */

	/* Sensor */
	i2c->i2c_addrs[0] = V024M_SENSOR_ADDR;

	/* Read R0x6B and verify it has 0x4 (Monochrome CFA sensor)
	 * This doesn't seem to be working (We read 0x709F instead)
	 * So read the chip ID from reg 0x0 and expect 0x1324
	 */
	ret = v024m_read(i2c->i2c_dev, 0x0, &id);
	if (ret || (id != 0x1324)) {
		return -EIO;
	}

	SYS_LOG_DBG("Detected V024M camera sensor!");
	SYS_LOG_DBG("Chip ID = 0x%04x", id);

	/* Bridge IC */
	i2c->i2c_addrs[1] = MIPI_BRIDGE_ADDR;

	/* Read chip ID and confirm it reads 0x44xx */
	ret = tc358746_read_16(i2c->i2c_dev, 0x0000, &id);
	if (ret || ((id & 0xFF00) != 0x4400))
		return -EIO;

	SYS_LOG_DBG("Detected Toshiba bridge IC on Camera module!");
	SYS_LOG_DBG("Chip ID: 0x%02x Rev ID: 0x%02x", id >> 8, id & 0xFF);

	/* Program the clock and data lane timer values */
	/* FIXME: These values are in LP clock cycles, whereas the Toshiba
	 * bridge IC values are in HS clock cycles, need to confirm that
	 * they match the TCLK_PREPARE, TCLK_ZERO and THS_PREPARE and THS_ZERO
	 * on the Toshiba bridge
	 */
	timers.clt1 = 4;
	timers.clt2 = 5;
	timers.dlt1 = 4;
	timers.dlt2 = 5;
	timers.dlt3 = 0;
	unicam_set_lane_timer_values(&timers);

	return 0;
}

/*
 * @brief Configure the LI V024M module
 * @details This api configures the LI V024M module with the requested image
 *	    capture settings. This includes configuring the MT9V024M sensor
 *	    as well as the Toshiba TC358746AXBG bridge.
 *
 * @param uc - Unicam configuration
 * @param i2c - I2C device information
 *
 * @return 0 on Success, -errno otherwise
 */
int li_v024m_configure(const struct unicam_config *uc, struct i2c_dev_info *i2c)
{
	u8_t bpp;
	int vb_pcs;
	u16_t sensor_val, fifo_thresh;
	u32_t i, h_blanking, v_blanking;

	ARG_UNUSED(i2c);

#ifdef CONFIG_COLORBAR_ENABLE
	/* When color bar is enabled, we only support 640x480 RAW 8 format */
	if (uc->capture_width != 640)
		return -EINVAL;
	if (uc->capture_height != 480)
		return -EINVAL;
	if (uc->pixel_format != UNICAM_PIXEL_FORMAT_RAW8)
		return -EINVAL;
#endif

	/* Check capture params */
	if ((uc->capture_width == 0) ||
	    (uc->capture_width > MAX_CAPTURE_WIDTH))
		return -EINVAL;

	if ((uc->capture_height == 0) ||
	    (uc->capture_height > MAX_CAPTURE_HEIGHT))
		return -EINVAL;

	if ((uc->capture_rate == 0) ||
	    (uc->capture_rate > MAX_FRAMES_PER_SEC))
		return -EINVAL;

	/* Check params for supported values */
	if (uc->capture_mode != UNICAM_CAPTURE_MODE_STREAMING)
		return -EOPNOTSUPP;

	if (uc->frame_format != UNICAM_FT_PROGRESSIVE)
		return -EOPNOTSUPP;

	if (uc->pixel_format == UNICAM_PIXEL_FORMAT_RAW10)
		bpp = 10;
	else if (uc->pixel_format == UNICAM_PIXEL_FORMAT_RAW8)
		bpp = 8;
	else
		return -EOPNOTSUPP;


	/* The sensor + bridge only supports RAW10 format. To get RAW8 format
	 * we will set the digital gain in the sensor to 0.25 and set the
	 * parallel data format in the bridge IC to RAW8. Although the parallel
	 * interface between the bridge and the sensor is 10 bit, setting the
	 * digital gain to 0.25 will zero out the upper 2 bits [9:8]. This
	 * should appear as an 8-bit connection to the bridge.
	 */
	/* Set the digital gain for all tiles */
	sensor_val = (uc->pixel_format == UNICAM_PIXEL_FORMAT_RAW10) ?
		      0x04F4 :	/* Gain = 1.0 */
		      0x01F1;	/* Gain = 0.25 */
	for (i = 0x80; i <= 0x98; i++) /* Digital gain regs 0x80 - 0x98 */
		set_sensor_reg(i, sensor_val);

	/* Set Peripheral Data Format for bridge to RAW10/RAW8
	 * Reg addr: 0x8, Bits[7:4], 0x0 - RAW8, 0x1 - RAW10
	 */
	for (i = 0; i < ARRAY_SIZE(bridge_regs); i++) {
		if (bridge_regs[i].addr == 0x0008) {
			bridge_regs[i].val &= ~(0xF0); /* RAW8 */
			if (uc->pixel_format == UNICAM_PIXEL_FORMAT_RAW10)
				bridge_regs[i].val |= 0x1 << 4; /* RAW10 */
			break;
		}
	}

	/* Set fifo level for bridge (0x0006) */
	fifo_thresh = uc->capture_width*bpp/64;
	fifo_thresh = (fifo_thresh + 0x3) & ~0x3; /* Round up to align to 4 */
	set_bridge_reg(0x0006, fifo_thresh);

	/* Set word count for the bride (0x0022) (bytes per line) */
	set_bridge_reg(0x0022, uc->capture_width*bpp/8);

	/* Configure capture size and capture rates */
	/* Set row start */
	sensor_val = 4 + ((MAX_CAPTURE_HEIGHT - uc->capture_height) / 2);
	set_sensor_reg(0x2, sensor_val);
	set_sensor_reg(0xCA, sensor_val);
	/* Set row size */
	set_sensor_reg(0x3, uc->capture_height);
	set_sensor_reg(0xCB, uc->capture_height);

	/* Set column start */
	sensor_val = (MAX_CAPTURE_WIDTH - uc->capture_width) / 2;
	sensor_val = (sensor_val) ? sensor_val : 0x1; /* Set to 1 for full res*/
	set_sensor_reg(0x1, sensor_val);
	set_sensor_reg(0xC9, sensor_val);
	/* Set column size */
	set_sensor_reg(0x4, uc->capture_width);
	set_sensor_reg(0xCC, uc->capture_width);

	/* Set max shutter width since AEC is enabled by default */
	set_sensor_reg(0xAD, uc->capture_height);

	/* Set the horizontal and vertical blanking taking into account
	 * the capture width, height and rate
	 * Horizontal blanking (hb) = 94 pixel clocks for full res (w = 752)
	 *	=> for a capture width w, hb = 94 + 752 - w
	 */
	h_blanking = 94 + MAX_CAPTURE_WIDTH - uc->capture_width +
		   EXTRA_HORIZ_BLANKING;
	h_blanking = min(h_blanking, MAX_BLANKING);
	set_sensor_reg(0x05, h_blanking);
	set_sensor_reg(0x0CD, h_blanking);

	/* Vertical blanking calculations: (all time units in pixel clocks)
	 *	1 pixel clock (pc) = 1/26.66Mhz = (100/2666) us
	 *	X = Extra horizontal blanking
	 *	row_time = (MAX_CAPTURE_WIDTH + 94 + X) pcs (846 + X pcs)
	 *	frame_valid_time = num_rows*row_time = h*(X + 846) pcs
	 *	frame_time = frame_valid_time + vertical_blanking
	 *	frame_time = 1000000/capture_rate (us)
	 *		   = (1000000/capture_rate)/(100/2666) pcs
	 *		   = 26660000/capture_rate pcs
	 * So vertical_blanking (pcs)
	 *		   = 26660000/capture_rate - num_rows*(X + 846)
	 */
	vb_pcs = (26660000/uc->capture_rate) -
		 (uc->capture_height*(uc->capture_width + h_blanking));
	vb_pcs = max(vb_pcs, 4);

	/* VB Register value (vb_val) is computed as:
	 *	vb_pcs = vb_val*row_time + 4
	 *	vb_val = (vb_pcs - 4)/(X + 846)
	 */
	v_blanking = (vb_pcs - 4) / (uc->capture_width + h_blanking);
	v_blanking = min(v_blanking, MAX_BLANKING);
	set_sensor_reg(0x06, v_blanking);
	set_sensor_reg(0x0CE, v_blanking);

	return 0;
}

/*
 * @brief Start video streaming from LI V024M module
 * @details This api starts the image data streaming on the MIPI data lines
 *
 * @param uc - Unicam configuration
 * @param i2c - I2C device information
 *
 * @return 0 on Success, -errno otherwise
 */
int li_v024m_start_stream(
	const struct unicam_config *uc,
	struct i2c_dev_info *i2c)
{
	int ret;
	u32_t i;

	ARG_UNUSED(uc);

	/* Program the MT9V024M Sensor registers first */
	for (i = 0; i < ARRAY_SIZE(sensor_regs); i++) {
		ret = v024m_write(i2c->i2c_dev,
				  sensor_regs[i].addr,
				  sensor_regs[i].val);
		if (ret)
			return ret;
	}

	/* Program the Bridge registers */
	for (i = 0; i < ARRAY_SIZE(bridge_regs); i++) {
		if (bridge_regs[i].size == 2) {
			ret = tc358746_write_16(i2c->i2c_dev,
						bridge_regs[i].addr,
						(u16_t)bridge_regs[i].val);
		} else {
			ret = tc358746_write_32(i2c->i2c_dev,
						bridge_regs[i].addr,
						bridge_regs[i].val);
		}
		if (ret)
			return ret;
#ifdef CONFIG_COLORBAR_ENABLE
		/* Don't enable the parallel port (last command 0x0004 <- 0x44*/
		if (i == ARRAY_SIZE(bridge_regs) - 2)
			break;
#endif
	}

#ifdef CONFIG_COLORBAR_ENABLE
	{
		u16_t val;

		tc358746_read_16(i2c->i2c_dev, 0x0008, &val);
		val |= 0x1;
		tc358746_write_16(i2c->i2c_dev, 0x0008, val);

		/* Raw 8 format will be set in the CSI packet header */
		tc358746_write_16(i2c->i2c_dev, 0x0050, 0x002A);

		/* tc358746_write_16(i2c->i2c_dev, 0x0022, 0x0384); */
		tc358746_write_16(i2c->i2c_dev, 0x00E0, 0x8000);
		tc358746_write_16(i2c->i2c_dev, 0x00E2, 0x0280);
		tc358746_write_16(i2c->i2c_dev, 0x00E4, 0x0285);

		k_sleep(5);
		k_sleep(40);
		k_sleep(40);
		for (i = 0; i < 50*16; i++) {
			tc358746_write_16(i2c->i2c_dev, 0x00e8, 0x1111*(i/50));
		}
		tc358746_write_16(i2c->i2c_dev, 0x00E0, 0xC1E0);
	}
#endif

	return 0;
}

/*
 * @brief Stop video streaming from LI V024M module
 * @details This api stops the image data streaming on the MIPI data lines
 *	    It also puts the camera module into a standby state so as to draw
 *	    minimal power
 *
 * @param uc - Unicam configuration
 * @param i2c - I2C device information
 *
 * @return 0 on Success, -errno otherwise
 */
int li_v024m_stop_stream(
	const struct unicam_config *uc,
	struct i2c_dev_info *i2c)
{
	int ret;
	u16_t regval;

	ARG_UNUSED(uc);

	/* The V024M sensor can be put into standby by asserting the
	 * standby signal. But the standby pin on the sensor is not connected
	 * to any of the GPIOs on the BCM58201 SVK board. The pin is however
	 * routed to a header on the board and would require an external
	 * connection to a GPIO to connect to the Sensor's standby pin
	 *
	 * Here we choose to just to configure the sensor in snapshot mode
	 * instead. This should keep the data and clock lines on the MIPI
	 * interface from toggling and image capture to sieze. Although this
	 * is not the same as standby, this should reduce the camera module's
	 * power consumption as the MIPI lanes would be in low power mode
	 *
	 * For the Toshiba bridge IC, there doesn't seem to be a standby mode
	 * and the chip needs to be reset for it to sieze the bridging function
	 */

	/* Set R0x07[4] = 1 to switch to snapshot mode */
	ret = v024m_read(i2c->i2c_dev, 0x07, &regval);
	if (ret)
		return ret;

	regval |= BIT(4);
	ret = v024m_write(i2c->i2c_dev, 0x07, regval);

	return ret;
}

/*
 * @brief Get max image width
 * @details Return maximum image width supported by the sensor
 *
 * @return Maximum image width supported
 */
u16_t li_v024m_max_width(void)
{
	return MAX_CAPTURE_WIDTH;
}

/*
 * @brief Get max image height
 * @details Return maximum image height supported by the sensor
 *
 * @return Maximum image height supported
 */
u16_t li_v024m_max_height(void)
{
	return MAX_CAPTURE_HEIGHT;
}

/*
 * @brief Get max frame rate
 * @details Return maximum frame rate supported by the sensor
 *
 * @return Maximum frame rate supported
 */
u16_t li_v024m_max_fps(void)
{
	return MAX_FRAMES_PER_SEC;
}


#endif /* CONFIG_LI_V024M_CAMERA */
