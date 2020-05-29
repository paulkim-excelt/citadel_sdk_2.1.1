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

/* @file unicam_bcm5820x_csi.h
 *
 * Unicam CSI driver private header
 *
 * This header defines structures and private apis for use in camera module
 * specific drivers.
 *
 */

#ifndef	__UNICAM_BCM5820X_CSI_H__
#define	__UNICAM_BCM5820X_CSI_H__

/* MIPI CSI2 data type codes */
/* Synchronization Short Packet Data Type Codes */
#define MIPI_DATA_TYPE_FRAME_START		0x0
#define MIPI_DATA_TYPE_FRAME_END		0x1
#define MIPI_DATA_TYPE_LINE_START		0x2
#define MIPI_DATA_TYPE_LINE_END			0x3
/* Generic 8-bit Long Packet Data Type Codes */
#define MIPI_DATA_TYPE_NULL			0x10
#define MIPI_DATA_TYPE_BLANKING_DATA		0x11
#define MIPI_DATA_TYPE_NON_IMAGE_DATA		0x12
/* YUV Image Data Type Codes */
#define MIPI_DATA_TYPE_YUV_420_8BIT		0x18
#define MIPI_DATA_TYPE_YUV_420_10BIT		0x19
#define MIPI_DATA_TYPE_YUV_420_8BIT_LEGACY	0x1A
#define MIPI_DATA_TYPE_YUV_420_8BIT_CSPS	0x1C /* CSPS: Chroma Shifted */
#define MIPI_DATA_TYPE_YUV_420_10BIT_CSPS	0x1D /*       Pixel Sampling */
#define MIPI_DATA_TYPE_YUV_422_8BIT		0x1E
#define MIPI_DATA_TYPE_YUV_422_10BIT		0x1F
/* RGB Image Data Type Codes */
#define MIPI_DATA_TYPE_RGB_444			0x20
#define MIPI_DATA_TYPE_RGB_555			0x21
#define MIPI_DATA_TYPE_RGB_565			0x22
#define MIPI_DATA_TYPE_RGB_666			0x23
#define MIPI_DATA_TYPE_RGB_888			0x24
/* RAW Image Data Type Codes */
#define MIPI_DATA_TYPE_RAW6			0x28
#define MIPI_DATA_TYPE_RAW7			0x29
#define MIPI_DATA_TYPE_RAW8			0x2A
#define MIPI_DATA_TYPE_RAW10			0x2B
#define MIPI_DATA_TYPE_RAW12			0x2C
#define MIPI_DATA_TYPE_RAW14			0x2D
/* User defined 8-bit Data Type Codes */
#define MIPI_DATA_TYPE_USER_DEFINED_1		0x30
#define MIPI_DATA_TYPE_USER_DEFINED_2		0x31
#define MIPI_DATA_TYPE_USER_DEFINED_3		0x32
#define MIPI_DATA_TYPE_USER_DEFINED_4		0x33
#define MIPI_DATA_TYPE_USER_DEFINED_5		0x34
#define MIPI_DATA_TYPE_USER_DEFINED_6		0x35
#define MIPI_DATA_TYPE_USER_DEFINED_7		0x36
#define MIPI_DATA_TYPE_USER_DEFINED_8		0x37

/* Maximum number of I2C slaves on the camera module */
#define MAX_I2C_SLAVES	2

/* Unicam driver states */
#define UNICAM_DRV_STATE_UNINITIALIZED	0
#define UNICAM_DRV_STATE_INITIALIZED	1
#define UNICAM_DRV_STATE_CONFIGURED	2
#define UNICAM_DRV_STATE_STREAMING	3

/*
 * @brief I2C device info
 *
 * i2c_dev - The I2C device driver handle to communicate with the camera
 *	     sensor/module
 * i2c_addrs - I2C slave addresses used by the camera sensor/module. This array
 *	       is populated and used by the camera module specific driver
 */
struct i2c_dev_info {
	struct device *i2c_dev;
	u8_t	i2c_addrs[MAX_I2C_SLAVES];
};

/* Clock and Data lane timer values */
struct unicam_lane_timers {
	u8_t	clt1;
	u8_t	clt2;
	u8_t	dlt1;
	u8_t	dlt2;
	u8_t	dlt3;
};

/* API to set data and clock lane timer values for unicam controller */
void unicam_set_lane_timer_values(struct unicam_lane_timers *timers);

#endif /* __UNICAM_BCM5820X_CSI_H__ */
