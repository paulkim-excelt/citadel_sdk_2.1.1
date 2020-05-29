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

/* @file li_v024m.h
 *
 * Private driver header for Leopard Imaging's V024M camera module
 *
 * Private APIs required by the unicam driver for configuring the LI V024M
 * module are declared here
 */

#ifndef __LI_V024M_H__
#define __LI_V024M_H__

/* I2C 7-bit slave address excluding the rd/wr bit */
#define V024M_SENSOR_ADDR	(0x90 >> 1)	/* Sensor address */
#define MIPI_BRIDGE_ADDR	(0x1C >> 1)	/* Toshiba Parallel to MIPI
						 * bridge
						 */
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
int li_v024m_init(struct i2c_dev_info *i2c);

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
int li_v024m_configure(
	const struct unicam_config *uc,
	struct i2c_dev_info *i2c);

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
	struct i2c_dev_info *i2c);

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
	struct i2c_dev_info *i2c);

/*
 * @brief Get max image width
 * @details Return maximum image width supported by the sensor
 *
 * @return Maximum image width supported
 */
u16_t li_v024m_max_width(void);

/*
 * @brief Get max image height
 * @details Return maximum image height supported by the sensor
 *
 * @return Maximum image height supported
 */
u16_t li_v024m_max_height(void);

/*
 * @brief Get max frame rate
 * @details Return maximum frame rate supported by the sensor
 *
 * @return Maximum frame rate supported
 */
u16_t li_v024m_max_fps(void);

#endif /* __LI_V024M_H__ */
