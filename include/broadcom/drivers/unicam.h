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

/* @file unicam.h
 *
 * Unicam camera controller Driver api
 *
 * This driver provides apis to configure and capture images through the unicam
 * controller.
 *
 */

#ifndef __UNICAM_H__
#define __UNICAM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** All platform specific headers are included here */
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <device.h>

/* Pixel formats - These are set to match the MIPI Data Type codes */
#define UNICAM_PIXEL_FORMAT_RAW6	0x28
#define UNICAM_PIXEL_FORMAT_RAW7	0x29
#define UNICAM_PIXEL_FORMAT_RAW8	0x2A
#define UNICAM_PIXEL_FORMAT_RAW10	0x2B
#define UNICAM_PIXEL_FORMAT_RAW12	0x2C
#define UNICAM_PIXEL_FORMAT_RAW14	0x2D

#define UNICAM_PIXEL_FORMAT_RGB444	0x20
#define UNICAM_PIXEL_FORMAT_RGB555	0x21
#define UNICAM_PIXEL_FORMAT_RGB565	0x22
#define UNICAM_PIXEL_FORMAT_RGB666	0x23
#define UNICAM_PIXEL_FORMAT_RGB888	0x24

#define UNICAM_PIXEL_FORMAT_YUV_420_8	0x18	/* 8-bit format */
#define UNICAM_PIXEL_FORMAT_YUV_420_10	0x19	/* 10-bit format */
#define UNICAM_PIXEL_FORMAT_YUV_422_8	0x1E	/* 8-bit format */
#define UNICAM_PIXEL_FORMAT_YUV_422_10	0x1F	/* 10-bit format */

/* Frame Types
 * In an interfaced frame capture mode, the frame types will either be
 * EVEN_0, ODD_1, EVEN_0, ODD_1  Or
 * ODD_0, EVEN_1, ODD_0, EVEN_1
 * This can be used to detect which even and odd frames belong to the same image
 */
#define UNICAM_FT_PROGRESSIVE		0x1	/* Progressive frame */
/* Interfaced frame with even field: even frame precedes odd frame for the image
 */
#define UNICAM_FT_INTERLACED_EVEN_0	0x2
/* Interfaced frame with even field: even frame follows odd frame for the image
 */
#define UNICAM_FT_INTERLACED_EVEN_1	0x3
/* Interfaced frame with odd field: odd frame precedes even frame for the image
 */
#define UNICAM_FT_INTERLACED_ODD_0	0x4
/* Interfaced frame with odd field: odd frame follows even frame for the image
 */
#define UNICAM_FT_INTERLACED_ODD_1	0x5

/* Unicam bus interface to camera */
#define UNICAM_BUS_TYPE_PARALLEL	0x1
#define UNICAM_BUS_TYPE_SERIAL		0x2

/* Unicam controller serial operation modes */
#define UNICAM_MODE_CSI2		0x1
#define UNICAM_MODE_CCP2		0x2

/* Unicam image capture mode */
#define UNICAM_CAPTURE_MODE_SNAPSHOT	0x1
#define UNICAM_CAPTURE_MODE_STREAMING	0x2

/*
 * @brief Pixel correction parameters
 * @details Parameters that control the pixel correction
 *
 * iclt - Image correction largest trusted (U3 format)
 * icst - Image correction smallest trusted (U3 format)
 * icfh - Image correction factor high (U2.3 format)
 * icfl - Image correction factor low (U2.3 format)
 *
 * Refer Unicam Peripheral Specication for details on how these parameters are
 * applied to perform pixel correction.
 */
struct pixel_correction_params {
	u8_t iclt;
	u8_t icst;
	u8_t icfh;
	u8_t icfl;
};

/*
 * @brief Down sizing parameters
 * @details Parameters to specify the downsizing of captured image
 *
 * idsf - Down scale factor. The down scaling ratio is determined by the eqn.
 *	  1.0 + idsf/65536. So to downscale by a factor of 1.5x, set idsf to
 *	  32678. Range for idsf is from 0 to 65535, which translates to a down
 *	  sizing range of 1.0x to 2.0x
 * start_offset - Index of the first pixel in the line at which to apply the
 *		  horizontal downsize
 * odd_pixel_offset - The sample position of the odd pixel relative to the
 *		      preceding even pixel such that the spatial location is
 *		      displaced to the right by 1.0+IDOPO/32768. (S16 format)
 * num_phases - Number of phases
 * num_coefficients - Number of coefficients per phase
 * coefficients - Pointer to the coefficient array of size (S13.0)
 *		  [num_phases] x [num_coefficients]
 *
 * Refer Unicam Peripheral Specification document for examples on how these
 * parameters are applied to achieve downsizing.
 */
struct down_size_params {
	u16_t idsf;
	u16_t start_offset;
	u16_t odd_pixel_offset;
	u8_t num_phases;
	u8_t num_coefficients;
	u16_t *coefficients;
};

/*
 * @brief Image windowing parameters
 * @details Parameters that specify how the capture image should be cropped
 *
 * start_pixel - First valid pixel in the line for horizontal windowing
 * end_pixel - Last valid pixel in the line for horizontal windowing
 *	       Horizontal windowing is done in pixel pairs.
 *	       So end_pixel and start_pixel should be a multiple of 2
 * start_line - First valid line in the frame for vertical windowing
 * end_line - Last valid line in the frame for vertial windowing
 */
struct windowing_params {
	u16_t start_pixel;
	u16_t end_pixel;
	u16_t start_line;
	u16_t end_line;
};

/*
 * @brief Unicam controller configuration parameters
 * @details This structure provides the configuration specific to image capture
 *	    and image processing (if any is required).
 *
 * bus_type - Serial or Parallel interface to camera module (CSI/CCP or CPI)
 * serial_mode -  Mode of operation for the unicam controller when bus_type is
 *		  set to Serial (CSI/CCP)
 * capture_width - Width of the Image to be captured by the camera sensor
 *		   Note that this may not be the final image size received with
 *		   unicam_get_frame(). The actual size received will factor
 *		   in any down sizing or windowing requirements
 * capture_height - Height of the Image to be captured by the camera sensor
 *		    Note that this may not be the final image size received with
 *		    unicam_get_frame(). The actual size received will factor
 *		    in any down sizing or windowing requirements
 * pixel_format - Image pixel format. In case where the sensor does not support
 *		  the specified format, the unicam controller's pipiline may be
 *		  convert to the requested pixel format (Ex: RAW10 - RAW6)
 * frame_format - Progressive or Interlaced
 * capture_mode - Snapshot mode or Streaming mode
 * double_buff_en - Enable double buffering for image and data packets. This
 *		    will double the memory requirements. Disabling this will
 *		    result in unicam_get_frame() blocking for up to one frame
 *		    capture time.
 * segment_height - The unicam traffic generator block allows the image to be
 *		    captured in segments to avoid allocating a full image size
 *		    buffer for capturing one frame. This parameter determines
 *		    the height of the segment. The segment width is the same as
 *		    the image width. The image height specified should be a
 *		    multiple of the segment height. For ex: To capture a VGA
 *		    image (640x480) the segment size can be set to 80. This will
 *		    capture one image as 6 segments. This parameter is used
 *		    only when double buffering is enabled.
 * pc_params - Pointer to pixel correction parameters. Set to NULL to disable
 *	       Pixel correction
 * ds_params - Pointer to downsizing parameters. Set to NULL to disable
 *	       Image down sizing
 * win_params - Pointer to windowing (cropping) parameters. Set to NULL to
 *		disable image windowing.
 * csi2_params - Parameters specific to CSI2 mode of operation with serial bus
 *		 interface
 * minimize_data_rates - Set to true to configure max data lanes possible, so
 *			 that the data rate on each lane is minimized.
 * ccp2_params - Parameters specific to CCP2 mode of operation with serial bus
 *		 interface
 * cpi_params - Parameters specific to parallel bus interface
 * sensor_params - Pointer to sensor specific parameters. This can be used to
 *		   pass in any sensor specific parameters that the sensor
 *		   specific driver understands and programs the sensor.
 */
struct unicam_config {
	u8_t	bus_type;
	u8_t	serial_mode;
	u16_t	capture_width;
	u16_t	capture_height;
	u16_t	capture_rate;
	u8_t	pixel_format;
	u8_t	frame_format;
	u8_t	capture_mode;
	bool	double_buff_en;
	u16_t	segment_height;

	struct pixel_correction_params *pc_params;
	struct down_size_params *ds_params;
	struct windowing_params *win_params;

	/* Protocol specific params */
	union {
		struct csi2_params {
			bool minimize_data_rates;
		} csi2_params;
		struct ccp2_params {
			/* Add CCP specific params here */
		} ccp2_params;
		struct cpi_params {
			/* Add CPI specific params here */
		} cpi_params;
	};

	void *sensor_params;
};

/*
 * @brief Image buffer information
 * @details Information about a capture image is stored in this structure
 *
 * buffer - Pointer to the allocated memory for holding the image content
 * length - Length of the allocated buffer
 * width  - width of the image contained in the image buffer (in pixels)
 * height - height of the image contained in the image buffer (in pixels)
 *	    width and height are set to 0 to indicate no valid image in buffer
 * line_stride - Length of one line of image data in bytes. The next line data
 *		 begins at an offset of this many bytes from the current line
 *		 start
 * pixel_format - enumeration to indicate the pixel format of the captured image
 *		  See UNICAM_PIXEL_FORMAT_XXX defines
 * frame_type - Progressive or Interlaced (even field or odd field)
 *		See UNICAM_FT_XXX defines
 * segment_num - The segment index in the image (0 to N-1, when N is
 *		 image_height/segment_height)
 */
struct image_buffer {
	void	*buffer;
	u32_t	length;
	u16_t	width;
	u16_t	height;
	u16_t	line_stride;
	u8_t	pixel_format;
	u8_t	frame_type;
	u16_t	segment_num;
};

/**
 * @typedef unicam_api_configure
 * @brief Callback API for configuring the Unicam driver
 *
 * See unicam_configure() for argument descriptions
 */
typedef int (*unicam_api_configure)(
		struct device *dev,
		struct unicam_config *config);

/**
 * @typedef unicam_api_get_config
 * @brief Callback API for retrieving the Unicam driver configuration
 *
 * See unicam_get_config() for argument descriptions
 */
typedef int (*unicam_api_get_config)(
		struct device *dev,
		struct unicam_config *config);

/**
 * @typedef unicam_api_start_stream
 * @brief Callback API for Starting the video stream from the camera sensor
 *	  through the unicam interface. Calling this api will start the
 *	  reception of image frames from the camera sensor on the CSI/CCP/CPI
 *	  bus. This api can be called only after successful configuration.
 *
 * See unicam_start_stream() for argument descriptions
 */
typedef int (*unicam_api_start_stream)(struct device *dev);

/**
 * @typedef unicam_api_stop_stream
 * @brief Callback API for Stopping the video stream from the camera sensor.
 *	  This api should be called only after a successful start stream call.
 *	  It configures the camera sensor and the unicam controller to disable
 *	  the capture and reception of image packets. If there are any blocks
 *	  that can be powered down (in the camera module or unicam controller)
 *	  they should be powered down on this call to save power.
 *
 * See unicam_stop_stream() for argument descriptions
 */
typedef int (*unicam_api_stop_stream)(struct device *dev);

/**
 * @typedef unicam_api_get_frame
 * @brief Callback API for get the captured frame from the camera sensor.
 *
 * See unicam_get_frame() for argument descriptions
 */
typedef int (*unicam_api_get_frame)(
		struct device *dev,
		struct image_buffer *img_buf);

/**
 * @brief Unicam driver API
 */
struct unicam_driver_api {
	unicam_api_configure configure;
	unicam_api_get_config get_config;
	unicam_api_start_stream start_stream;
	unicam_api_stop_stream stop_stream;
	unicam_api_get_frame get_frame;
};

/**
 * @brief       Configure the unicam controller
 * @details     This api configures the unicam controller and the attached
 *		sensor as per the specified configuration params.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to unicam controller conifguration.
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int unicam_configure(
			struct device *dev,
			struct unicam_config *config)
{
	struct unicam_driver_api *api;

	api = (struct unicam_driver_api *)dev->driver_api;
	return api->configure(dev, config);
}

/**
 * @brief       Retrieve the unicam controller configuration
 * @details     This api returns the unicam controller configuration.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to unicam controller conifguration.
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int unicam_get_config(
			struct device *dev,
			struct unicam_config *config)
{
	struct unicam_driver_api *api;

	api = (struct unicam_driver_api *)dev->driver_api;
	return api->get_config(dev, config);
}

/**
 * @brief       Start the video stream
 * @details     This api configures the unicam controller and the attached
 *		sensor to start streaming the images over the CSI/CCP/CPI
 *		lines. If the controller has been configured in snapshot mode,
 *		then enables all the modules and powers up any blocks required
 *		be ready to receive an image from the sensor. For example, for
 *		a CSI2 sensor, this api will enable the data and clock lanes.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int unicam_start_stream(struct device *dev)
{
	struct unicam_driver_api *api;

	api = (struct unicam_driver_api *)dev->driver_api;
	return api->start_stream(dev);
}

/**
 * @brief       Stop the video stream
 * @details     This api should be called only after a successful start stream
 *		call. It configures the camera sensor and the unicam controller
 *		to disable the capture and reception of image packets. If there
 *		are any blocks that can be powered down (in the camera module
 *		or unicam controller) they will be powered down on this call
 *		to save power.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int unicam_stop_stream(struct device *dev)
{
	struct unicam_driver_api *api;

	api = (struct unicam_driver_api *)dev->driver_api;
	return api->stop_stream(dev);
}

/**
 * @brief       Get the captured image from the sensor
 * @details     This api retrieves the most recently captured image from the
 *		camera sensor. In case of a snapshot mode, this api triggers
 *		the signals required to capture the image (exposure, readout)
 *		and waits for the capture to complete before returning the
 *		captured frame.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   img_buf - Pointer to image buffer with metadata about the image
 *
 * @return      0 on success
 * @return      errno on failure
 */
static inline int unicam_get_frame(
			struct device *dev,
			struct image_buffer *img_buf)
{
	struct unicam_driver_api *api;

	api = (struct unicam_driver_api *)dev->driver_api;
	return api->get_frame(dev, img_buf);
}


#ifdef __cplusplus
}
#endif

#endif /* __UNICAM_H__ */
