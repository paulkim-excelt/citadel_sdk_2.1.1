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

/* @file unicam_bcm5820x_csi.c
 *
 * Unicam (Universal camera inerface) driver for the serial (CSI2) interface
 *
 * This driver provides apis, to configure the serial camera connected
 * to the unicam controller and capture images from it.
 *
 */

#include <board.h>
#include <unicam.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <broadcom/dma.h>
#include "unicam_bcm5820x_csi.h"
#include "unicam_bcm5820x_regs.h"

#ifdef CONFIG_LI_V024M_CAMERA
#include "li_v024m.h"
#else
#warning Unicam driver enabled, but no supported camera module selected
#endif

/* Unicam Interrupt number */
#define UNICAM_INTERRUPT_NUM	SPI_IRQ(CHIP_INTR__IOSYS_UNICAM_INTERRUPT)

/* Unicam data buffer size */
#define DATA_BUFF_SIZE		KB(8)

/* I2C interfaces available */
#define I2C0	0
#define I2C1	1
#define I2C2	2

/* Default to SVK */
#ifndef CONFIG_CAM_I2C_PORT
#define CONFIG_CAM_I2C_PORT I2C1
#endif

/* Timeout while waiting for an frame */
#define FRAME_WAIT_COUNT	500

#define DATA_INT_STATUS_BIT	24
#define WIPE_PING_PONG_BUFFER_AFTER_IMAGE_READ

/* Return pointer aligned to next 'ALIGN'-byte boundary */
#define ALIGN_PTR(PTR, ALIGN) (void *)(((u32_t)PTR + ALIGN - 1) & ~(ALIGN - 1))

/* Frame end interrupts don't appear to be working reliably with the unicam
 * controller. This interrupt is essential especially in double buffering mode
 * to sync to the first segment in the frame. This define enables the code to
 * configure packet capture module for capturing frame end packets.
 */
#define USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
#ifdef USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
#define CONFIG_CAPTURE_AND_LOG_PACKETS
#endif

/* For debugging: Enable this to check if frame start and
 * image packets are being captured by the unicam controller
 */
#define CONFIG_CAPTURE_AND_LOG_PACKETS
#ifdef CONFIG_CAPTURE_AND_LOG_PACKETS
struct packet_capture_info {
	u8_t virtual_chan;	/* Virtual channel Identifier (0 - 3) */
	u8_t data_type;		/* Data type in the packet header (0 - 63)
				 * See MIPI CSI2 spec for list of data types
				 */
};

#ifndef USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
static struct packet_capture_info packet_cap[] = {
	{0x0, MIPI_DATA_TYPE_FRAME_START},
	{0x0, MIPI_DATA_TYPE_RAW8}
};
#endif /* USE_PACKET_CAPTURE_TO_DETECT_FRAME_END */
#endif /* CONFIG_CAPTURE_AND_LOG_PACKETS */

/* Bits per pixel info */
struct bpp_info {
	u8_t pixel_format;
	u8_t bpp;
};

/* Image buffer read task params */
#define TASK_STACK_SIZE	2048
static K_THREAD_STACK_DEFINE(image_buffer_read_task_stack, TASK_STACK_SIZE);
static struct k_thread img_buff_read_task;

/* Messages sent from ISR to image buffer read task */
#define BUFFER_0_READY		0x10
#define BUFFER_1_READY		0x11
#define FRAME_START_INTERRUPT	0x21
#define FRAME_END_INTERRUPT	0x22
#define STREAMING_STOPPED	0x31
#define PACKET_CAPTURED_0	0x41
#define PACKET_CAPTURED_1	0x42

/* Alignment lengths */
#define AXI_BURST_LENGTH	16
#define STRIDE_ALIGN		256
#define CACHE_LINE_SIZE		64

/* IDSF Parameter range (16 bit) */
#define IDSF_PARAM_RANGE	65536

/* Fifo size */
#define ISR_FIFO_SIZE		256

/* ISR message FIFO entry */
struct isr_fifo_entry {
	void *link_in_fifo; /* Used by Zephyr kernel */
	u32_t msg;
};

/* Fifo elements */
struct isr_fifo_entry msgs[ISR_FIFO_SIZE];
u8_t fifo_index;

/*
 * @brief Unicam device private data
 *
 * state - Driver state (Initialized/Configured/Streaming)
 * img_buff0 - Pointer to image buffer 0
 * img_buff1 - Pointer to image buffer 1
 * data_buff0 - Pointer to data buffer 0
 * data_buff1 - Pointer to data buffer 1
 * alloc_ptr - A single allocation is made for all the image and data buffers
 *	       alloc_ptr holds this pointer.
 * alloc_size - A single allocation is made for all the image and data buffers
 *		alloc_size hold this total size
 * read_buff - Pointer to image read buffer (used only in double buffering mode)
 * read_buff_image_valid - True, if the image has not yet been read by get_frame
 *			   False, if it has been read by get_frame() already
 * read_buff_seg_num - Segment id of the segment copied to read buff
 * i2c_interface - 0/1/2, index of the i2c controller. 5820X has 3 i2c
 *		   controllers
 * i2c_info - I2C device info needed by camera module specific code
 * segment_id - Holds the index of the next segment that will be written out
 *		by the unicam output engine. This is valid only when double
 *		buffering is enabled
 * isr_fifo - FIFO used byt the unicam ISR to communicate frame reception to the
 *	      image read task
 * read_buff_lock - Lock used over the read buffer between the writing thread
 *		    and the get_next_frame() api to ensure data synchronization
 * get_frame_info - Pointer to image_buffer passed by to get_frame() call. This
 *		    field is used to communicate the image buffer to the read
 *		    task, so that it can be updated with the image data by the
 *		    read buffer thread.
 * num_active_data_lanes - Number of MIPI data lanes in use
 */
struct unicam_data {
	u8_t	state;

	u8_t	*img_buff0;
	u8_t	*img_buff1;

	u8_t	*data_buff0;
	u8_t	*data_buff1;

	u8_t	*read_buff;
	u8_t	read_buff_image_valid;
	u8_t	read_buff_seg_num;

	u8_t	*alloc_ptr;
	u32_t	alloc_size;

	u8_t	i2c_interface;
	struct i2c_dev_info i2c_info;

	u8_t	segment_id;
	struct k_fifo isr_fifo;

	struct k_sem read_buff_lock;
	struct image_buffer *get_frame_info;

	u8_t	num_active_data_lanes;
};

/* Bits per pixel as packed on the csi-2 bus */
static const struct bpp_info bpp[] = {
	{UNICAM_PIXEL_FORMAT_RAW6,	 6},
	{UNICAM_PIXEL_FORMAT_RAW7,	 7},
	{UNICAM_PIXEL_FORMAT_RAW8,	 8},
	{UNICAM_PIXEL_FORMAT_RAW10,	 10},
	{UNICAM_PIXEL_FORMAT_RAW12,	 12},
	{UNICAM_PIXEL_FORMAT_RAW14,	 14},
	{UNICAM_PIXEL_FORMAT_RGB444,	 16},
	{UNICAM_PIXEL_FORMAT_RGB555,	 16},
	{UNICAM_PIXEL_FORMAT_RGB565,	 16},
	{UNICAM_PIXEL_FORMAT_RGB666,	 18},
	{UNICAM_PIXEL_FORMAT_RGB888,	 24},
	{UNICAM_PIXEL_FORMAT_YUV_420_8,	 8},
	{UNICAM_PIXEL_FORMAT_YUV_420_10, 10},
	{UNICAM_PIXEL_FORMAT_YUV_422_8,	 8},
	{UNICAM_PIXEL_FORMAT_YUV_422_10, 10}
};

/*
 * @brief Return bits per pixel required for a give pixel format
 */
static u8_t get_bpp(u8_t pixel_format)
{
	u32_t i;

	for (i = 0; i < ARRAY_SIZE(bpp); i++)
		if (pixel_format == bpp[i].pixel_format)
			return bpp[i].bpp;

	/* Return 8 (1 byte) for an invalid format */
	return 8;
}

/*
 * @brief Reset Analogue PHY
 */
static void reset_analog_phy(void)
{
	u32_t val;

	/* Analogue reset PHY */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, AR, 1);
	sys_write32(val, CAMANA_ADDR);
	/* Sleep for 100 ms for Analog phy to reset */
	k_sleep(100);
	SET_FIELD(val, CAMANA, AR, 0);
	sys_write32(val, CAMANA_ADDR);
}

/*
 * @brief Enable Analog Power
 */
static void enable_analog_phy_power(void)
{
	u32_t val;

	/* Enable Analog and Band Gap Power */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, APD, 0);
	SET_FIELD(val, CAMANA, BPD, 0);
	sys_write32(val, CAMANA_ADDR);
}

/*
 * @brief Configure pixel correction parameters
 */
static void config_pixel_correction_params(struct unicam_config *config)
{
	u32_t linelen, val;
	struct windowing_params *win = config->win_params;
	struct pixel_correction_params *pc = config->pc_params;

	if (config->pc_params) {
		linelen = win ? (win->end_pixel - win->end_pixel + 1) :
				config->capture_width;

		/* Set pixel correction params */
		val = 0x0;
		SET_FIELD(val, CAMICC, ICLL, linelen);
		SET_FIELD(val, CAMICC, ICLT, pc->iclt);
		SET_FIELD(val, CAMICC, ICST, pc->icst);
		SET_FIELD(val, CAMICC, ICFH, pc->icfh);
		SET_FIELD(val, CAMICC, ICFL, pc->icfl);
		sys_write32(val, CAMICC_ADDR);

		/* Enable Pixel correction */
		val = sys_read32(CAMIPIPE_ADDR);
		SET_FIELD(val, CAMIPIPE, ICM, 1);
		sys_write32(val, CAMIPIPE_ADDR);
	} else {
		/* Disable Pixel correction */
		val = sys_read32(CAMIPIPE_ADDR);
		SET_FIELD(val, CAMIPIPE, ICM, 0);
		sys_write32(val, CAMIPIPE_ADDR);
	}
}

/*
 * @brief Configure image down sizing parameters
 */
static void config_downsizing_params(struct unicam_config *config)
{
	u32_t linelen, val, i, j;
	struct down_size_params *ds = config->ds_params;
	struct windowing_params *win = config->win_params;

	if (config->ds_params) {
		/* Program CAMIDC */
		val = 0x0;
		SET_FIELD(val, CAMIDC, IDSF, ds->idsf);
		linelen = win ? (win->end_pixel - win->end_pixel + 1) :
				config->capture_width;
		SET_FIELD(val, CAMIDC, IDLL, linelen);
		sys_write32(val, CAMIDC_ADDR);

		/* Program CAMIDPO */
		val = 0x0;
		SET_FIELD(val, CAMIDPO, IDSO, ds->start_offset);
		SET_FIELD(val, CAMIDPO, IDOPO, ds->odd_pixel_offset);
		sys_write32(val, CAMIDPO_ADDR);

		/* Program the co-efficients */
		for (i = 0; i < ds->num_phases; i++) {
			/* We write 2 co-efficients at once */
			for (j = 0; j < ds->num_coefficients; j += 2) {
				u32_t offset;

				/* 2 bytes per co-efficient */
				offset = (i*ds->num_coefficients + j)*2;
				val = 0x0;
				SET_FIELD(val, CAMIDCD, IDCDA,
				  ds->coefficients[i*ds->num_coefficients+j]);
				SET_FIELD(val, CAMIDCD, IDCDB,
				  ds->coefficients[i*ds->num_coefficients+j+1]);
				sys_write32(offset, CAMIDCA_ADDR);
				sys_write32(val, CAMIDCD_ADDR);
			}
		}

		/* Enable Down Sizing */
		val = sys_read32(CAMIPIPE_ADDR);
		SET_FIELD(val, CAMIPIPE, IDM, 1);
		sys_write32(val, CAMIPIPE_ADDR);
	} else {
		/* Enable Down Sizing */
		val = sys_read32(CAMIPIPE_ADDR);
		SET_FIELD(val, CAMIPIPE, IDM, 0);
		sys_write32(val, CAMIPIPE_ADDR);
	}
}

/*
 * @brief Configure Windowing parameters
 */
static void config_windowing_params(struct unicam_config *config)
{
	u32_t val;
	struct windowing_params *win = config->win_params;

	if (config->win_params) {
		val = 0x0;
		/* Pixels processed in pairs - align start/end pixels */
		SET_FIELD(val, CAMIHWIN, HWSP, win->start_pixel & ~0x1);
		SET_FIELD(val, CAMIHWIN, HWSP, win->end_pixel | 0x1);
		sys_write32(val, CAMIHWIN_ADDR);

		val = 0x0;
		SET_FIELD(val, CAMIVWIN, VWSL, win->start_line);
		SET_FIELD(val, CAMIVWIN, VWEL, win->end_line);
		sys_write32(val, CAMIVWIN_ADDR);
	} else {
		/* Disable Windowing */
		sys_write32(0x0, CAMIHWIN_ADDR);
		sys_write32(0x0, CAMIVWIN_ADDR);
	}
}

/*
 * @brief Get image height
 * @details Retrieve the image height as output by the unicam pipeline. This
 *	    will be smaller than or equal to the capture height
 */
static u32_t get_image_height(const struct unicam_config *config)
{
	u32_t img_height = config->capture_height;

	if (config->win_params)
		img_height = config->win_params->end_line -
			     config->win_params->start_line + 1;

	if (config->ds_params)
		img_height = (img_height * IDSF_PARAM_RANGE) /
			     (IDSF_PARAM_RANGE + config->ds_params->idsf);

	return img_height;
}

/*
 * @brief Get image width
 * @details Retrieve the image width as output by the unicam pipeline. This
 *	    will be smaller than or equal to the capture width
 */
static u32_t get_image_width(struct unicam_config *config)
{
	u32_t img_width = config->capture_width;

	if (config->win_params)
		img_width = config->win_params->end_pixel -
			     config->win_params->start_pixel + 1;

	if (config->ds_params)
		img_width = (img_width * IDSF_PARAM_RANGE) /
			    (IDSF_PARAM_RANGE + config->ds_params->idsf);

	return img_width;
}

#ifdef CONFIG_USE_STATIC_MEMORY_FOR_BUFFER
/* When using statically allocated memory, we assume a max of 640x80 size
 * segment buffer.
 */
#define MAX_WIDTH		640
#define MAX_HEIGHT		40

#define CAM_LINE_SIZE		((MAX_WIDTH + STRIDE_ALIGN - 1) &	\
				 ~(STRIDE_ALIGN - 1))
#define CAM_IMAGE_BUFF_SIZE	(CAM_LINE_SIZE*MAX_HEIGHT + STRIDE_ALIGN)
#define CAM_DATA_BUFF_SIZE	(DATA_BUFF_SIZE + STRIDE_ALIGN)
#define CAM_BUFF_SIZE		((2*CAM_IMAGE_BUFF_SIZE) + /* Image buffer */ \
				(2*CAM_DATA_BUFF_SIZE) +   /* Data buffer */  \
				CAM_IMAGE_BUFF_SIZE +	   /* Read buffer */  \
				STRIDE_ALIGN)

static u8_t __attribute__((__aligned__(STRIDE_ALIGN))) cam_buff[CAM_BUFF_SIZE];
#endif

/*
 * @brief Configure image and data buffers
 * @details This function allocates the memory required for image and data
 *	    buffers. It takes into account the windowing and downsizing params
 *	    to compute the buffers size. It also takes into account the
 *	    double_buff_en flag and enables/disables the hw bit accordingly.
 *	    If the memory allocation succeeds then the previously allocated
 *	    memory is freed.
 *
 * @return 0 on success and -errno on failure
 */
static int configure_image_data_buffers(
		struct unicam_config *config,
		struct unicam_data *dd)
{
	u32_t alloc_size;
	u32_t img_size, stride, dt_size, val;
	u32_t segment_height, img_height, img_width;

	/* Calculate image height and width (After it passes through the
	 * Image processing pipeline
	 */
	img_height = get_image_height(config);
	img_width = get_image_width(config);

	/* Calculate image width */
	stride = img_width*get_bpp(config->pixel_format)/8;

	/* Align image buffer size */
	stride = (stride + STRIDE_ALIGN - 1) & ~(STRIDE_ALIGN - 1);

	/* From unicam peripheral spec */
	dt_size = DATA_BUFF_SIZE;

	/* Align data buffer size */
	dt_size = (dt_size + AXI_BURST_LENGTH - 1) & ~(AXI_BURST_LENGTH - 1);

	/*  - Allow 'AXI_BURST_LENGTH' bytes between buffers,
	 *    because both start and end addresses both need to be
	 *    aligned even though they are inclusive.
	 */
	dt_size += AXI_BURST_LENGTH;

#ifndef CONFIG_USE_STATIC_MEMORY_FOR_BUFFER
	/* Free previously allocated image and data buffers only after the
	 * Allocation for the current configuration succeeds
	 */
	if (dd->alloc_ptr)
		cache_line_aligned_free(dd->alloc_ptr);
#endif

	/* Allocate image and data buffers */
	if (config->double_buff_en) {
		segment_height = config->segment_height;
		if (segment_height == 0)
			segment_height = img_height;

		/* Calculate image size
		 *  - Allow 'AXI_BURST_LENGTH' bytes between buffers,
		 *    because both start and end addresses both need to be
		 *    aligned even though they are inclusive.
		 */
		img_size = stride * segment_height + AXI_BURST_LENGTH;

		/* Calculate allocation size
		 *  - 2 image buffers
		 *  - 2 data buffers
		 *  - 1 read image buffer (same size as image buffer)
		 *  - STRIDE_ALIGN to accommodate aligning the allocated
		 *    pointer to the next STRIDE_ALIGN boundary
		 */
		alloc_size = 2*(img_size + dt_size) + img_size + STRIDE_ALIGN;

#ifdef CONFIG_USE_STATIC_MEMORY_FOR_BUFFER
		if (alloc_size > CAM_BUFF_SIZE)
			return -ENOMEM;
		dd->alloc_ptr = cam_buff;
#else
		dd->alloc_ptr = cache_line_aligned_alloc(alloc_size);
		if (dd->alloc_ptr == NULL)
			return -ENOMEM;
#endif

		/* Update allocation size */
		dd->alloc_size = alloc_size;

		/* Image buffer has to start at a stride align boundary */
		dd->img_buff0 = ALIGN_PTR(dd->alloc_ptr, STRIDE_ALIGN);

		/* Partition allocated memory */
		dd->img_buff1 = dd->img_buff0 + img_size;
		dd->data_buff0 = dd->img_buff1 + img_size;
		dd->data_buff1 = dd->data_buff0 + dt_size;
		dd->read_buff = dd->data_buff1 + dt_size;
		dd->read_buff_image_valid = false;

		/* Program image and data buffer start/end addresses */
		sys_write32((u32_t)dd->img_buff0, CAMIBSA0_ADDR);
		sys_write32((u32_t)dd->img_buff0 + img_size - 1, CAMIBEA0_ADDR);
		sys_write32((u32_t)dd->img_buff1, CAMIBSA1_ADDR);
		sys_write32((u32_t)dd->img_buff1 + img_size - 1, CAMIBEA1_ADDR);
		sys_write32((u32_t)dd->data_buff0, CAMDBSA0_ADDR);
		sys_write32((u32_t)dd->data_buff0 + dt_size - 1, CAMDBEA0_ADDR);
		sys_write32((u32_t)dd->data_buff1, CAMDBSA1_ADDR);
		sys_write32((u32_t)dd->data_buff1 + dt_size - 1, CAMDBEA1_ADDR);

		/* Set double buffer enable bit */
		val = sys_read32(CAMDBCTL_ADDR);
		SET_FIELD(val, CAMDBCTL, DB_EN, 1);
		sys_write32(val, CAMDBCTL_ADDR);
	} else {
		img_size = stride*img_height + AXI_BURST_LENGTH;

		alloc_size = img_size + dt_size + STRIDE_ALIGN;

#ifdef CONFIG_USE_STATIC_MEMORY_FOR_BUFFER
		if (alloc_size > CAM_BUFF_SIZE)
			return -ENOMEM;
		dd->alloc_ptr = cam_buff;
#else
		dd->alloc_ptr = cache_line_aligned_alloc(alloc_size);
		if (dd->alloc_ptr == NULL)
			return -ENOMEM;
#endif
		/* Update allocation size */
		dd->alloc_size = alloc_size;

		/* Image buffer has to start at a stride align boundary */
		dd->img_buff0 = ALIGN_PTR(dd->alloc_ptr, STRIDE_ALIGN);

		/* Set data buffer address */
		dd->data_buff0 = dd->img_buff0 + img_size;

		/* Program image and data buffer start/end addresses */
		sys_write32((u32_t)dd->img_buff0, CAMIBSA0_ADDR);
		sys_write32((u32_t)dd->img_buff0 + img_size - 1, CAMIBEA0_ADDR);
		sys_write32((u32_t)dd->data_buff0, CAMDBSA0_ADDR);
		sys_write32((u32_t)dd->data_buff0 + dt_size - 1, CAMDBEA0_ADDR);

		/* Disable double buffer enable bit */
		val = sys_read32(CAMDBCTL_ADDR);
		SET_FIELD(val, CAMDBCTL, DB_EN, 0);
		sys_write32(val, CAMDBCTL_ADDR);
	}

	return 0;
}

/*
 * @brief Configure packet capture
 */
void configure_packet_capture(void)
{
	u32_t val;

#ifdef CONFIG_CAPTURE_AND_LOG_PACKETS
	val = 0x0;
	SET_FIELD(val, CAMCMP0, PCEN, 1);
	SET_FIELD(val, CAMCMP0, GIN, 1);
	SET_FIELD(val, CAMCMP0, CPHN, 1);

#ifdef USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
	/* Configure only packet capture0 (frame end) */
	SET_FIELD(val, CAMCMP0, PCVCN, 0x0);
	SET_FIELD(val, CAMCMP0, PCDTN, MIPI_DATA_TYPE_FRAME_END);
	sys_write32(val, CAMCMP0_ADDR);
#else
	/* Configure both packet captures (As configured in packet_cap[]) */
	SET_FIELD(val, CAMCMP0, PCVCN, packet_cap[0].virtual_chan);
	SET_FIELD(val, CAMCMP0, PCDTN, packet_cap[0].data_type);
	sys_write32(val, CAMCMP0_ADDR);
	SET_FIELD(val, CAMCMP0, PCVCN, packet_cap[1].virtual_chan);
	SET_FIELD(val, CAMCMP0, PCDTN, packet_cap[1].data_type);
	sys_write32(val, CAMCMP1_ADDR);
#endif /* USE_PACKET_CAPTURE_TO_DETECT_FRAME_END */
#else
	sys_write32(0x0, CAMCMP0_ADDR);
	sys_write32(0x0, CAMCMP1_ADDR);
#endif /* CONFIG_CAPTURE_AND_LOG_PACKETS */
}

/*
 * @brief Initialize unicam registers
 */
static void init_unicam_regs(void)
{
	u32_t val, i;

	/* Initialize UNICAM_REG register */
	val = 0;
	SET_FIELD(val, UNICAM_REG, CAM_FS2X_EN_CLK, 1);
	SET_FIELD(val, UNICAM_REG, CAM_LDO_CNTL_EN, 1);
	sys_write32(val, UNICAM_REG_ADDR);
	k_busy_wait(3);

	SET_FIELD(val, UNICAM_REG, CAM_HP_EN, 1);
	SET_FIELD(val, UNICAM_REG, CAM_LDORSTB_1P8, 1);
	SET_FIELD(val, UNICAM_REG, CAM_LP_EN, 1);
	sys_write32(val, UNICAM_REG_ADDR);
	k_busy_wait(100);

	/* Initialize the unicam controller registers */
	/* CAMCTL */
	val = 0x0;
	SET_FIELD(val, CAMCTL, MEN, 1);		/* Memories enabled */
	SET_FIELD(val, CAMCTL, OET, 32);	/* Set output engine timeout */
	SET_FIELD(val, CAMCTL, PFT, 5);		/* PFT - Set to default (5) */
	SET_FIELD(val, CAMCTL, CPM, 0);		/* Select CSI2 mode */
	SET_FIELD(val, CAMCTL, CPE, 0);		/* Leave peripheral disabled */
	sys_write32(val, CAMCTL_ADDR);

	/* CAMANA */
	val = 0x0;
	SET_FIELD(val, CAMANA, PTATADJ, 7);	/* Band gap bias control */
	SET_FIELD(val, CAMANA, CTATADJ, 7);	/* Band gap bias control */
	SET_FIELD(val, CAMANA, DDL, 1);		/* Disable data lanes */
	SET_FIELD(val, CAMANA, LDO_PU, 1);	/* Power up LDO */
	SET_FIELD(val, CAMANA, APD, 1);		/* Analogue power down */
	SET_FIELD(val, CAMANA, BPD, 1);		/* Band-gap power down */
	sys_write32(val, CAMANA_ADDR);

	/* CAMPRI */
	val = 0x0;
	SET_FIELD(val, CAMPRI, BL, 0);		/* Set AXI burst length */
	SET_FIELD(val, CAMPRI, BS, 2);		/* Set AXI burst spacing */
	SET_FIELD(val, CAMPRI, PE, 0);		/* Disable panic */
	sys_write32(val, CAMPRI_ADDR);

	/* CAMCLK */
	val = 0x0;
	SET_FIELD(val, CAMCLK, CLE, 0);		/* Disable clock lane */
	SET_FIELD(val, CAMCLK, CLPD, 1);	/* Power down clock */
	SET_FIELD(val, CAMCLK, CLLPE, 0);	/* Disable low power */
	SET_FIELD(val, CAMCLK, CLHSE, 0);	/* HS mode - automatic */
	SET_FIELD(val, CAMCLK, CLTRE, 0);	/* Termination resistor enable
						 * - automatic
						 */
	sys_write32(val, CAMCLK_ADDR);

	/* CAMDATn */
	val = 0x0;
	SET_FIELD(val, CAMDAT0, DLEN, 0);	/* Disable data lane */
	SET_FIELD(val, CAMDAT0, DLPDN, 0);	/* Power Down data lane */
	SET_FIELD(val, CAMDAT0, DLLPEN, 0);	/* Disable low power */
	SET_FIELD(val, CAMDAT0, DLHSEN, 0);	/* HS mode - automatic */
	SET_FIELD(val, CAMDAT0, DLTREN, 0);	/* Termination resistor enable
						 * - automatic
						 */
	SET_FIELD(val, CAMDAT0, DLSMN, 1);	/* No bit errors allowed */
	for (i = 0; i < 4; i++)			/* For all 4 data lanes */
		sys_write32(val, CAMDAT0_ADDR + i*4);

	/* CAMICTL */
	val = 0x0;
	SET_FIELD(val, CAMICTL, IBOB, 1);
	sys_write32(val, CAMICTL_ADDR);

	/* Disable all image processing pipeline features
	 * CAMIPIPE, CAMIHWIN, CAMIVWIN
	 */
	sys_write32(0, CAMIPIPE_ADDR);
	sys_write32(0, CAMIHWIN_ADDR);
	sys_write32(0, CAMIVWIN_ADDR);

	/* CAMDCS */
	val = 0x0;
	SET_FIELD(val, CAMDCS, DBOB, 1);
	sys_write32(val, CAMDCS_ADDR);

	/* CAMFIX0 */
	val = 0x0;
	SET_FIELD(val, CAMFIX0, CPI_SELECT, 0);
	sys_write32(val, CAMFIX0_ADDR);
}

/*
 * @brief Soft reset unicam controller for debug/error recovery purposes
 */
void unicam_soft_reset(void)
{
	u32_t val;

	/* Disable data lines */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, DDL, 1);
	sys_write32(val, CAMANA_ADDR);

	/* Shutdown output engine */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, SOE, 1);
	sys_write32(val, CAMCTL_ADDR);
	/* Current transaction complete, when OES = 1 */
	while (GET_FIELD(sys_read32(CAMSTA_ADDR), CAMSTA, OES) != 0x1)
		;

	/* Assert soft reset */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, CPR, 1);
	sys_write32(val, CAMCTL_ADDR);
	k_sleep(100);

	/* Clear soft reset */
	SET_FIELD(val, CAMCTL, CPR, 0);
	sys_write32(val, CAMCTL_ADDR);

	/* Enable AXI transfers, SOE = 0 */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, SOE, 0);
	sys_write32(val, CAMCTL_ADDR);

	/* Enable data lanes, DDL = 0 */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, DDL, 0);
	sys_write32(val, CAMANA_ADDR);
}

/*
 * @brief Fifo add element
 * @details Called from unicam ISR to add an element into fifo
 */
static void add_msg_to_fifo(struct k_fifo *fifo, u8_t msg)
{
	msgs[++fifo_index].msg = msg;
	k_fifo_put(fifo, &msgs[fifo_index]);
}

/*
 * @brief Unicam controller ISR handler
 * @details This unicam controller ISR checks the CSI2 interrupts status bits
 *	    and clears the pending interrupt bits. Any actions that are required
 *	    to be taken for a pending interrupt should be taken here. For
 *	    instance, the buffer ready interrupt, when enabled, will be cleared
 *	    by the ISR for subsequent image captures into the image buffer.
 *	    If any blocking operations or system calls need to be made based on
 *	    then interrupt status, the ISR will use event flags or a similar
 *	    mechanism to notify a low priority thread to act on the interrupt
 *	    This low priority thread shall clear the event after performing the
 *	    appropriate action. Currently this thread is not required and is not
 *	    implemented in this driver.
 */
static void unicam_isr(void *arg)
{
	u32_t status, istatus;
	struct device *dev = (struct device *)arg;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;

	status = sys_read32(CAMSTA_ADDR);
	/* Check for overall interrupt pending bit first */
	if (GET_FIELD(status, CAMSTA, IS) == 0)
		return;

	/* Check buffer 0 and 1 ready interrupts */
	if (GET_FIELD(status, CAMSTA, BUF0_RDY)) {
		if (dd->state == UNICAM_DRV_STATE_STREAMING)
			add_msg_to_fifo(&dd->isr_fifo, BUFFER_0_READY);

		sys_write32(BIT(CAMSTA__BUF0_RDY), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, BUF1_RDY)) {
		if (dd->state == UNICAM_DRV_STATE_STREAMING)
			add_msg_to_fifo(&dd->isr_fifo, BUFFER_1_READY);

		sys_write32(BIT(CAMSTA__BUF1_RDY), CAMSTA_ADDR);
	}

	/* Check for packet capture interrupts */
	if (GET_FIELD(status, CAMSTA, PI0)) {
		u32_t cap = sys_read32(CAMCAP0_ADDR), msg;

#ifdef USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
		msg = FRAME_END_INTERRUPT;
#else
		msg = PACKET_CAPTURED_0;
#endif
		if (GET_FIELD(cap, CAMCAP0, CPHV))
			add_msg_to_fifo(&dd->isr_fifo, msg);
		sys_write32(BIT(CAMSTA__PI0), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, PI1)) {
		u32_t cap = sys_read32(CAMCAP1_ADDR);

		if (GET_FIELD(cap, CAMCAP1, CPHV))
			add_msg_to_fifo(&dd->isr_fifo, PACKET_CAPTURED_1);

		sys_write32(BIT(CAMSTA__PI1), CAMSTA_ADDR);
	}

	/* Data interrupt status is duplicated in this register (original status
	 * is available in CAMDCS) and needs to be cleared
	 */
	if (status & BIT(DATA_INT_STATUS_BIT))
		sys_write32(BIT(DATA_INT_STATUS_BIT), CAMSTA_ADDR);

	/* Check and clear image status interrupts */
	istatus = sys_read32(CAMISTA_ADDR);

	/* Line count interrupt */
	if (GET_FIELD(istatus, CAMISTA, LCI))
		sys_write32(BIT(CAMISTA__LCI), CAMISTA_ADDR);

	/* Frame start interrupt */
	if (GET_FIELD(istatus, CAMISTA, FSI)) {
		add_msg_to_fifo(&dd->isr_fifo, FRAME_START_INTERRUPT);
		sys_write32(BIT(CAMISTA__FSI), CAMISTA_ADDR);
	}

	/* Frame end interrupt */
	if (GET_FIELD(istatus, CAMISTA, FEI)) {
#ifndef USE_PACKET_CAPTURE_TO_DETECT_FRAME_END
		add_msg_to_fifo(&dd->isr_fifo, FRAME_END_INTERRUPT);
#endif
		sys_write32(BIT(CAMISTA__FEI), CAMISTA_ADDR);
	}

	status = sys_read32(CAMDCS_ADDR);
	if (GET_FIELD(status, CAMDCS, DI)) {
		/* Don't set LDP while clearing Data interrupt bit */
		SET_FIELD(status, CAMDCS, LDP, 0);
		/* Write 1 to clear data interrupt bit */
		SET_FIELD(status, CAMDCS, DI, 1);
		sys_write32(status, CAMDCS_ADDR);
	}
}

/*
 * @brief Dump Unicam registers (Used for debugging)
 */
void unicam_dump_regs(void)
{
	SYS_LOG_ERR("CAMCTL   = 0x%08x", sys_read32(CAMCTL_ADDR));
	SYS_LOG_ERR("CAMANA   = 0x%08x", sys_read32(CAMANA_ADDR));
	SYS_LOG_ERR("CAMCLT   = 0x%08x", sys_read32(CAMCLT_ADDR));
	SYS_LOG_ERR("CAMDLT   = 0x%08x", sys_read32(CAMDLT_ADDR));
	SYS_LOG_ERR("CAMCLK   = 0x%08x", sys_read32(CAMCLK_ADDR));
	SYS_LOG_ERR("CAMDAT0  = 0x%08x", sys_read32(CAMDAT0_ADDR));
	SYS_LOG_ERR("CAMDAT1  = 0x%08x", sys_read32(CAMDAT1_ADDR));
	SYS_LOG_ERR("CAMDAT2  = 0x%08x", sys_read32(CAMDAT2_ADDR));
	SYS_LOG_ERR("CAMDAT3  = 0x%08x", sys_read32(CAMDAT3_ADDR));
	SYS_LOG_ERR("CAMICTL  = 0x%08x", sys_read32(CAMICTL_ADDR));
	SYS_LOG_ERR("CAMIDI0  = 0x%08x", sys_read32(CAMIDI0_ADDR));
	SYS_LOG_ERR("CAMIDI1  = 0x%08x", sys_read32(CAMIDI1_ADDR));
	SYS_LOG_ERR("CAMIBSA0 = 0x%08x", sys_read32(CAMIBSA0_ADDR));
	SYS_LOG_ERR("CAMIBEA0 = 0x%08x", sys_read32(CAMIBEA0_ADDR));
	SYS_LOG_ERR("CAMIBSA1 = 0x%08x", sys_read32(CAMIBSA1_ADDR));
	SYS_LOG_ERR("CAMIBEA1 = 0x%08x", sys_read32(CAMIBEA1_ADDR));
	SYS_LOG_ERR("CAMIBLS  = 0x%08x", sys_read32(CAMIBLS_ADDR));
	SYS_LOG_ERR("CAMIPIPE = 0x%08x", sys_read32(CAMIPIPE_ADDR));
	SYS_LOG_ERR("CAMDBSA0 = 0x%08x", sys_read32(CAMDBSA0_ADDR));
	SYS_LOG_ERR("CAMDBEA0 = 0x%08x", sys_read32(CAMDBEA0_ADDR));
	SYS_LOG_ERR("CAMDBSA1 = 0x%08x", sys_read32(CAMDBSA1_ADDR));
	SYS_LOG_ERR("CAMDBEA1 = 0x%08x", sys_read32(CAMDBEA1_ADDR));
	SYS_LOG_ERR("CAMDCS   = 0x%08x", sys_read32(CAMDCS_ADDR));
	SYS_LOG_ERR("CAMICTL  = 0x%08x", sys_read32(CAMICTL_ADDR));
	SYS_LOG_ERR("CAMFIX0  = 0x%08x", sys_read32(CAMFIX0_ADDR));
	SYS_LOG_ERR("CAMSTA   = 0x%08x", sys_read32(CAMSTA_ADDR));
	SYS_LOG_ERR("CAMISTA  = 0x%08x", sys_read32(CAMISTA_ADDR));
	SYS_LOG_ERR("CAMTGBSZ = 0x%08x", sys_read32(CAMTGBSZ_ADDR));
}

/*
 * @brief Log the unicam controller CSI2 error status
 * @details The unicam controller provides a number of error status bits that
 *	    can be useful for debugging issues with image capture. This function
 *	    logs all the CSI2 error status bits from various registers and
 *	    clears the errors, once they are logged. This function is meant for
 *	    debug purposes.
 */
void unicam_log_error_status(void)
{
	u32_t status, i;

	status = sys_read32(CAMSTA_ADDR);

	/* Panic status */
	if (GET_FIELD(status, CAMSTA, PS)) {
		SYS_LOG_ERR("Panic status set!");
		sys_write32(BIT(CAMSTA__PS), CAMSTA_ADDR);
	}

	/* Data integrity error bits */
	if (GET_FIELD(status, CAMSTA, DL)) {
		SYS_LOG_ERR("Data lost status is set!");
		sys_write32(BIT(CAMSTA__DL), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, BFO)) {
		SYS_LOG_ERR("Burst FIFO overflow detected!");
		sys_write32(BIT(CAMSTA__BFO), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, OFO)) {
		SYS_LOG_ERR("Output FIFO overflow detected!");
		sys_write32(BIT(CAMSTA__OFO), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, IFO)) {
		SYS_LOG_ERR("Input FIFO overflow detected!");
		sys_write32(BIT(CAMSTA__IFO), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, CRCE)) {
		SYS_LOG_ERR("CRC error detected!");
		sys_write32(BIT(CAMSTA__CRCE), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, SSC)) {
		SYS_LOG_ERR("Bit alignment mismatch b/w start & end of line!");
		sys_write32(BIT(CAMSTA__SSC), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, PLE)) {
		SYS_LOG_ERR("Packet length error detected!");
		sys_write32(BIT(CAMSTA__PLE), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, HOE)) {
		SYS_LOG_ERR("Multiple bit errors detected in packet!");
		sys_write32(BIT(CAMSTA__HOE), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, PBE)) {
		SYS_LOG_ERR("Parity bit error detected!");
		sys_write32(BIT(CAMSTA__PBE), CAMSTA_ADDR);
	}

	if (GET_FIELD(status, CAMSTA, SBE)) {
		SYS_LOG_ERR("Single bit error detected!");
		sys_write32(BIT(CAMSTA__SBE), CAMSTA_ADDR);
	}

	/* Clock lane errors */
	status = sys_read32(CAMCLK_ADDR);
	if (GET_FIELD(status, CAMCLK, CLSTE)) {
		SYS_LOG_ERR("Clock Low power state transition error: %d",
			    GET_FIELD(status, CAMCLK, CLSTE));
		/* Write back read value, as this register contains other
		 * configuration bits that shouldn't be cleared
		 */
		sys_write32(status, CAMCLK_ADDR);
	}

	/* Data lane errors */
	for (i = 0; i < 4; i++) {
		status = sys_read32(CAMDAT0_ADDR + i*4);
		if (GET_FIELD(status, CAMDAT0, DLSTEN)) {
			SYS_LOG_ERR("Data[%d] Low power state trans error: %d",
				    i, GET_FIELD(status, CAMDAT0, DLSN));
			/* Write back read value, as this register contains
			 * configuration bits that shouldn't be cleared
			 */
			sys_write32(status, CAMCLK_ADDR);
		}
		if (GET_FIELD(status, CAMDAT0, DLSEN)) {
			SYS_LOG_ERR("Data[%d] Lane sync error detected!", i);
			sys_write32(status, CAMCLK_ADDR);
		}
		if (GET_FIELD(status, CAMDAT0, DLFON)) {
			SYS_LOG_ERR("Data[%d] Lane Fifo Overflow detected!", i);
			sys_write32(status, CAMCLK_ADDR);
		}
	}
}

/*
 * @brief Log the unicam controller status
 * @details This function logs all the CSI2 debug status bits from various and
 *	    registers. This function is meant for debug purposes.
 */
void unicam_log_status(void)
{
	u32_t reg;

	ARG_UNUSED(reg);

	reg = sys_read32(CAMIHSTA_ADDR);
	SYS_LOG_DBG("Pixels per line: %d\n", GET_FIELD(reg, CAMIHSTA, PPL));

	reg = sys_read32(CAMIVSTA_ADDR);
	SYS_LOG_DBG("Lines per frame: %d\n", GET_FIELD(reg, CAMIVSTA, LPF)+1);

	/* Fifo level registers */
	reg = sys_read32(CAMDBG0_ADDR);
	SYS_LOG_DBG("Lane 0 Fifo Peak Level : %d\n", reg & 0x1F);
	SYS_LOG_DBG("Lane 1 Fifo Peak Level : %d\n", (reg >> 5) & 0x1F);
	SYS_LOG_DBG("Lane 2 Fifo Peak Level : %d\n", (reg >> 10) & 0x1F);
	SYS_LOG_DBG("Lane 3 Fifo Peak Level : %d\n", (reg >> 15) & 0x1F);

	reg = sys_read32(CAMDBG1_ADDR);
	SYS_LOG_DBG("Output burst fifo peak level: %d\n", reg & 0xFFFF);
	SYS_LOG_DBG("Output data fifo peak level : %d\n", reg >> 16);

	reg = sys_read32(CAMDBG2_ADDR);
	SYS_LOG_DBG("Output burst fifo curr level: %d\n", reg & 0xFFFF);
	SYS_LOG_DBG("Output data fifo curr level : %d\n", reg >> 16);
}

static int bcm5820x_unicam_csi_init(struct device *dev);

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
static int bcm5820x_unicam_csi_configure(
			struct device *dev,
			struct unicam_config *config)
{
	int ret;
	u32_t val, i, stride;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;

	/* Check driver state */
	if (dd->state != UNICAM_DRV_STATE_INITIALIZED &&
		dd->state != UNICAM_DRV_STATE_CONFIGURED) {
		if (dd->state == UNICAM_DRV_STATE_STREAMING)
			SYS_LOG_WRN("Can't reconfigure unicam while streaming");
		else
			SYS_LOG_WRN("Unicam driver not initialized!");
		return -EPERM;
	}

	/* Check params */
	/* Only CSI mode is supported in this driver */
	if ((config->bus_type != UNICAM_BUS_TYPE_SERIAL) ||
	    (config->serial_mode != UNICAM_MODE_CSI2))
		return -EINVAL;

#ifdef CONFIG_LI_V024M_CAMERA
	/* Check frame size and fps limits */
	if (config->capture_width > li_v024m_max_width())
		return -EINVAL;
	if (config->capture_height > li_v024m_max_height())
		return -EINVAL;
	if (config->capture_rate > li_v024m_max_fps())
		return -EINVAL;
#endif

	if (config->capture_mode != UNICAM_CAPTURE_MODE_STREAMING) {
		SYS_LOG_WRN("Only Streaming mode is supported currently!");
		return -EOPNOTSUPP;
	}

	if (config->frame_format != UNICAM_FT_PROGRESSIVE) {
		SYS_LOG_WRN("Only progressive frame type supported currently!");
		return -EOPNOTSUPP;
	}

	if (config->double_buff_en && config->segment_height) {
		/* Verify image height is a multiple of segment height */
		u32_t num_segs, img_height;

		img_height = get_image_height(config);
		num_segs = (img_height + config->segment_height - 1) /
			    config->segment_height;
		if (img_height != (num_segs*config->segment_height))
			return -EINVAL;
	}

#ifdef CONFIG_LI_V024M_CAMERA
	ret = li_v024m_configure(config, &dd->i2c_info);
	if (ret)
		return ret;
#endif

	/* Setup CAMANA */
	reset_analog_phy();
	enable_analog_phy_power();

	/* Set ID registers to capture image data through image buffer
	 * Set one ID for each possible value of VC (0 - 3)
	 */
	val = 0;
	for (i = 0; i < 4; i++)
		val |= ((i << 0x6) | config->pixel_format) << i*8;
	sys_write32(val, CAMIDI0_ADDR);

	/* If the camera module does not support a certain format that
	 * the unicam pipiline supports, then we could program the CAMIPIPE
	 * params (DEBL, DEM, PPM, DDM, PUM) to convert the pixel format to
	 * the user requested format. Currently this is not supported in this
	 * driver.
	 */

	/* Program Pixel correction registers */
	config_pixel_correction_params(config);

	/* Program downsizing registers */
	config_downsizing_params(config);

	/* Program windowing registers */
	config_windowing_params(config);

	/* Setup image and data buffers */
	ret = configure_image_data_buffers(config, dd);
	if (ret)
		return ret;

	/* Configure line stride */
	stride = get_image_width(config)*get_bpp(config->pixel_format)/8;
	stride = (stride + STRIDE_ALIGN - 1) & ~(STRIDE_ALIGN - 1);
	sys_write32(stride, CAMIBLS_ADDR);

	/* Setup DCS register */
	val = sys_read32(CAMDCS_ADDR);
	SET_FIELD(val, CAMDCS, EDL, 2);	/* Embedded data lines = 2 */
	SET_FIELD(val, CAMDCS, DIM, 0);	/* Data interrupt mode - Frame end */
	SET_FIELD(val, CAMDCS, DIE, 1);	/* Data interrupt enabled */
	sys_write32(val, CAMDCS_ADDR);

	/* Update config structure and driver state */
	*(struct unicam_config *)dev->config->config_info = *config;
	dd->state = UNICAM_DRV_STATE_CONFIGURED;

	return 0;
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
static int bcm5820x_unicam_get_config(
			struct device *dev,
			struct unicam_config *config)
{
	if (config == NULL)
		return -EINVAL;

	*config = *(struct unicam_config *)dev->config->config_info;
	return 0;
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
static int bcm5820x_unicam_csi_start_stream(struct device *dev)
{
	int ret;
	u32_t val, i;
	struct unicam_config *config;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;

	config = (struct unicam_config *)dev->config->config_info;

	if (dd->state != UNICAM_DRV_STATE_CONFIGURED)
		return -EPERM;

	/* DDL clear in CAMANA */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, DDL, 0);
	sys_write32(val, CAMANA_ADDR);

	/* Enable clock lane */
	val = sys_read32(CAMCLK_ADDR);
	SET_FIELD(val, CAMCLK, CLLPE, 1);	/* Low power enabled */
	SET_FIELD(val, CAMCLK, CLPD, 0);	/* clock lane power up */
	SET_FIELD(val, CAMCLK, CLE, 1);		/* Enable clock lane */
	sys_write32(val, CAMCLK_ADDR);

	/* Enable data lanes */
	for (i = 0; i < dd->num_active_data_lanes; i++) {
		val = sys_read32(CAMDAT0_ADDR + i*4);
		SET_FIELD(val, CAMDAT0, DLLPEN, 1);   /* Low power enabled */
		SET_FIELD(val, CAMDAT0, DLPDN, 0);    /* Data lane powered up */
		SET_FIELD(val, CAMDAT0, DLEN, 1);     /* Enable Data lane */
		sys_write32(val, CAMDAT0_ADDR + i*4);
	}

	/* Enable FE/FS interrupts */
	val = sys_read32(CAMICTL_ADDR);
	SET_FIELD(val, CAMICTL, LIP, 0);	/* Clear write only fields */
	SET_FIELD(val, CAMICTL, TFC, 0);	/* Clear write only fields */
	SET_FIELD(val, CAMICTL, FSIE, 0);	/* Enable frame start int */
	SET_FIELD(val, CAMICTL, FEIE, 1);	/* Enable frame end int */
	sys_write32(val, CAMICTL_ADDR);

	/* Enable Buffer Ready interrupts */
	val = sys_read32(CAMDBCTL_ADDR);
	SET_FIELD(val, CAMDBCTL, BUF0_IE, 1);
	if (config->double_buff_en)
		SET_FIELD(val, CAMDBCTL, BUF1_IE, 1);
	sys_write32(val, CAMDBCTL_ADDR);

	/* Configure packet capture */
	configure_packet_capture();

	/* Set bit 9 and 6 in CAMFIX0 - Per DV guideline */
	val = sys_read32(CAMFIX0_ADDR) | BIT(9) | BIT(6);
	sys_write32(val, CAMFIX0_ADDR);

	/* Set Traffic generator's Data Burst max lines */
	val = sys_read32(CAMTGBSZ_ADDR);
	SET_FIELD(val, CAMTGBSZ, DBMAXLINES, config->segment_height - 1);
	sys_write32(val, CAMTGBSZ_ADDR);

	/* Enable master peripheral for unicam */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, CPE, 1);
	sys_write32(val, CAMCTL_ADDR);

	/* Set LIP and DIP to image and data buffer update pointers */
	val = sys_read32(CAMICTL_ADDR);
	SET_FIELD(val, CAMICTL, TFC, 0);	/* Clear write only fields */
	SET_FIELD(val, CAMICTL, LIP, 1);
	sys_write32(val, CAMICTL_ADDR);

	val = sys_read32(CAMDCS_ADDR);
	SET_FIELD(val, CAMDCS, LDP, 1);
	sys_write32(val, CAMDCS_ADDR);

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* invalidate image/data buffers */
	invalidate_dcache_by_addr((u32_t)dd->img_buff0, dd->alloc_size);
#endif

	/* Start streaming from sensor */
	ret = li_v024m_start_stream(config, &dd->i2c_info);
	if (ret == 0)
		dd->state = UNICAM_DRV_STATE_STREAMING;

	return ret;
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
static int bcm5820x_unicam_csi_stop_stream(struct device *dev)
{
	int ret;
	u32_t val, i;
	struct unicam_config *config;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;

	config = (struct unicam_config *)dev->config->config_info;

	if (dd->state != UNICAM_DRV_STATE_STREAMING)
		return -EPERM;

	/* Disable FE/FS interrupts */
	val = sys_read32(CAMICTL_ADDR);
	SET_FIELD(val, CAMICTL, LIP, 0);	/* Clear write only fields */
	SET_FIELD(val, CAMICTL, TFC, 0);	/* Clear write only fields */
	SET_FIELD(val, CAMICTL, FSIE, 0);	/* Disable frame start int */
	SET_FIELD(val, CAMICTL, FEIE, 0);	/* Disable frame end int */
	sys_write32(val, CAMICTL_ADDR);

	/* Disable Buffer Ready interrupts */
	val = sys_read32(CAMDBCTL_ADDR);
	SET_FIELD(val, CAMDBCTL, BUF0_IE, 0);
	SET_FIELD(val, CAMDBCTL, BUF1_IE, 0);
	sys_write32(val, CAMDBCTL_ADDR);

	/* Disable data lanes CAMANA.DDL = 1 */
	val = sys_read32(CAMANA_ADDR);
	SET_FIELD(val, CAMANA, DDL, 1);
	sys_write32(val, CAMANA_ADDR);

	/* Shutdown output engine */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, SOE, 1);
	sys_write32(val, CAMCTL_ADDR);

	/* Poll on OES bit to check if shutdown is complete */
	while (GET_FIELD(sys_read32(CAMSTA_ADDR), CAMSTA, OES) == 0)
		;
	SET_FIELD(val, CAMCTL, SOE, 0);
	sys_write32(val, CAMCTL_ADDR);

	/* Reset unicam peripheral */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, CPR, 1);
	sys_write32(val, CAMCTL_ADDR);
	k_sleep(100);
	/* Clear reset */
	SET_FIELD(val, CAMCTL, CPR, 0);
	sys_write32(val, CAMCTL_ADDR);

	/* Disable data lanes */
	for (i = 0; i < dd->num_active_data_lanes; i++) {
		val = sys_read32(CAMDAT0_ADDR + i*4);
		SET_FIELD(val, CAMDAT0, DLLPEN, 0);   /* Low power disabled */
		SET_FIELD(val, CAMDAT0, DLPDN, 1);    /* Power down data lane */
		SET_FIELD(val, CAMDAT0, DLEN, 0);     /* Disable data lane */
		sys_write32(val, CAMDAT0_ADDR + i*4);
	}

	/* Disable clock lane */
	val = sys_read32(CAMCLK_ADDR);
	SET_FIELD(val, CAMCLK, CLLPE, 0);	/* Low power disabled */
	SET_FIELD(val, CAMCLK, CLPD, 1);	/* Clock lane powered down */
	SET_FIELD(val, CAMCLK, CLE, 0);		/* Disable clock lane */
	sys_write32(val, CAMCLK_ADDR);

	/* Clear Traffic generator's Data Burst max lines */
	val = sys_read32(CAMTGBSZ_ADDR);
	SET_FIELD(val, CAMTGBSZ, DBMAXLINES, 0);
	sys_write32(val, CAMTGBSZ_ADDR);

	/* Disable peripheral */
	val = sys_read32(CAMCTL_ADDR);
	SET_FIELD(val, CAMCTL, CPE, 0);
	sys_write32(val, CAMCTL_ADDR);

	dd->state = UNICAM_DRV_STATE_CONFIGURED;

	/* Shutdown sensor/module */
	ret = li_v024m_stop_stream(config, &dd->i2c_info);
	if (ret)
		return ret;

	/* Send STREAMING_STOPPED message to task */
	add_msg_to_fifo(&dd->isr_fifo, STREAMING_STOPPED);

	/* Yield to image buffer read thread */
	k_sleep(10);

	return 0;
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
static int bcm5820x_unicam_csi_get_frame(
			struct device *dev,
			struct image_buffer *img_buf)
{
	u32_t sz, wait_count;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;
	struct unicam_config *config;

	config = (struct unicam_config *)dev->config->config_info;

	if ((img_buf == NULL) || (img_buf->buffer == NULL))
		return -EINVAL;

	if (dd->state != UNICAM_DRV_STATE_STREAMING)
		return -EPERM;

	if (config->double_buff_en) {
		sz = sys_read32(CAMIBEA0_ADDR) - sys_read32(CAMIBSA0_ADDR) + 1;

		/* Take the lock before copying the image data */
		if (k_sem_take(&dd->read_buff_lock, FRAME_WAIT_COUNT) == 0) {
			/* Return image if available */
			if (dd->read_buff_image_valid) {
				/* Copy image data */
				memcpy(img_buf->buffer, dd->read_buff, sz);
				/* Update segment id */
				img_buf->segment_num = dd->read_buff_seg_num;
				/* Mark buffer as invalid */
				dd->read_buff_image_valid = false;
			} else {
				k_sem_give(&dd->read_buff_lock);
				return -EAGAIN;
			}

			k_sem_give(&dd->read_buff_lock);
		} else {
			SYS_LOG_ERR("Frame not available\n");
			return -EIO;
		}
	} else {
		wait_count = FRAME_WAIT_COUNT;

		dd->get_frame_info = img_buf;
		while (dd->get_frame_info && --wait_count)
			k_sleep(1);

		dd->get_frame_info = NULL;

		if (wait_count == 0) {
			SYS_LOG_ERR("Frame not available\n");
			return -EAGAIN;
		}

		/* Set segment num to 0 for single buffer mode */
		img_buf->segment_num = 0;
	}

	/* Populate other image information */
	img_buf->pixel_format = config->pixel_format;
	img_buf->frame_type = config->frame_format;
	if (config->double_buff_en)
		img_buf->height = config->segment_height;
	else
		img_buf->height = get_image_height(config);

	img_buf->width = get_image_width(config);

	/* Align to AXI burst size */
	img_buf->line_stride = (img_buf->width*get_bpp(config->pixel_format)/8 +
				 STRIDE_ALIGN - 1) &
				~(STRIDE_ALIGN - 1);

	return 0;
}

/*
 * @brief Helper function to copy image from buf0/1 to read buffer
 */
static void copy_image_to_read_buff(
		struct unicam_data *dd,
		u32_t addr,
		u32_t size,
		u8_t num_segs)
{
#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Invalidate buffer if it is cached */
	invalidate_dcache_by_addr(addr, size);
#endif

	/* Lock the buffer and copy image data */
	k_sem_take(&dd->read_buff_lock, K_FOREVER);
	memcpy(dd->read_buff, (u8_t *)addr, size);
	dd->read_buff_seg_num = dd->segment_id;
	dd->read_buff_image_valid = true;
	dd->segment_id++;
	dd->segment_id %= num_segs;

#ifdef WIPE_PING_PONG_BUFFER_AFTER_IMAGE_READ
	{
		u32_t i, j;

		/* Initialize buffer to a checker pattern 16x16 pixel size */
		for (i = 0; i < size/16; i++)
			for (j = 0; j < 16; j++)
				*((u8_t *)addr + i*16 + j) = i % 2 ? 0xFF : 0x0;
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Invalidate buffer if it is cached */
		clean_dcache_by_addr(addr, size);
#endif
	}

#endif /* WIPE_PING_PONG_BUFFER_AFTER_IMAGE_READ */

	k_sem_give(&dd->read_buff_lock);
}

/*
 * @brief Image read buffer thread entry function
 * @details This thread is used to evacuate the image buffers.
 *	    The unicam ISR sends a message to this task indicating a buffer is
 *	    available. The action taken by the task to this message depends on
 *	    the buffering mode. The onus of clearing the buffer ready interrupt
 *	    and free the buffer for subsequent image capture, is on this task.
 *	    In double buffer mode:
 *		Once the task receives this message, it copies the captured
 *		segment to a read buffer and the buffer ready interrupt bit is
 *		cleared. The segment in this read buffer is returned when
 *		get_frame() api is called. The segment id is stored along with
 *		the image data and incremented.
 *	    In Single buffer mode:
 *		If there is no pending get_frame(), then the buffer is
 *		immediately released for use by the unicam output engine. If a
 *		get_frame() api call was recevied prior to this message, then
 *		the image data is populated into the buffer passed by get_frame
 *		and following that the buffer ready interrupt is cleared. This
 *		will result in the get_frame() api blocking for up a time equal
 *		to the frame period. But this is the side effect of using single
 *		buffer for image capture.
 *
 */
static void img_buff_read_task_fn(void *p1, void *p2, void *p3)
{
	u8_t ns;
	u32_t msg, addr, size;
	struct isr_fifo_entry *entry;
	struct device *dev = (struct device *)p1;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;
	const struct unicam_config *cfg = dev->config->config_info;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Compute number of segments per frame */
	ns = get_image_height(cfg) / cfg->segment_height;

	do {
		/* Wait until we start streaming */
		if (dd->state != UNICAM_DRV_STATE_STREAMING) {
			while (dd->state != UNICAM_DRV_STATE_STREAMING)
				k_sleep(1);

			/* Reset segment id */
			dd->segment_id = 0;
			/* Re-compute number of segments per frame */
			ns = get_image_height(cfg) / cfg->segment_height;
			/* Mark read buffer as invalid */
			dd->read_buff_image_valid = false;
		}

		entry = k_fifo_get(&dd->isr_fifo, K_FOREVER);
		if (entry)
			msg = entry->msg;
		else
			continue;

#ifdef CONFIG_CAPTURE_AND_LOG_PACKETS
		switch (msg) {
		u32_t val;
		case PACKET_CAPTURED_0:
			val = sys_read32(CAMCAP0_ADDR);

			/* Clear the valid bit */
			sys_write32(BIT(31), CAMCAP0_ADDR);

			SYS_LOG_DBG("[CAP0] VC = %d : DT = 0x%2x : Size = %d\n",
				    GET_FIELD(val, CAMCAP0, CVCN),
				    GET_FIELD(val, CAMCAP0, CDTN),
				    GET_FIELD(val, CAMCAP0, CWCN));

			val = sys_read32(CAMSTA_ADDR);
			if (GET_FIELD(val, CAMSTA, IS))
				sys_write32(val, CAMSTA_ADDR);
			break;
		case PACKET_CAPTURED_1:
			val = sys_read32(CAMCAP1_ADDR);

			/* Clear the valid bit */
			sys_write32(BIT(31), CAMCAP1_ADDR);

			SYS_LOG_DBG("[CAP1] VC = %d : DT = 0x%2x : Size = %d\n",
				    GET_FIELD(val, CAMCAP1, CVCN),
				    GET_FIELD(val, CAMCAP1, CDTN),
				    GET_FIELD(val, CAMCAP1, CWCN));

			val = sys_read32(CAMSTA_ADDR);
			if (GET_FIELD(val, CAMSTA, IS))
				sys_write32(val, CAMSTA_ADDR);
			break;
		default:
			break;
		}
#endif
		if (cfg->double_buff_en) {
			switch (msg) {
			case BUFFER_0_READY:
				addr = sys_read32(CAMIBSA0_ADDR);
				size = sys_read32(CAMIBEA0_ADDR) - addr + 1;
				copy_image_to_read_buff(dd, addr, size, ns);
				break;
			case BUFFER_1_READY:
				addr = sys_read32(CAMIBSA1_ADDR);
				size = sys_read32(CAMIBEA1_ADDR) - addr + 1;
				copy_image_to_read_buff(dd, addr, size, ns);
				break;
			case FRAME_END_INTERRUPT:
				/* Reset segment id on frame end */
				dd->segment_id = 0;
				break;
			case STREAMING_STOPPED:
			default:
				break;
			}
		} else {
			switch (msg) {
			case FRAME_END_INTERRUPT:
				/* Update image buffer memory, if get_frame is
				 * waiting
				 */
				if (dd->get_frame_info) {
					addr = sys_read32(CAMIBSA0_ADDR);
					size = sys_read32(CAMIBEA0_ADDR)-addr+1;
#ifdef CONFIG_DATA_CACHE_SUPPORT
					/* Invalidate buffer if it is cached */
					invalidate_dcache_by_addr(addr, size);
#endif
					memcpy(dd->get_frame_info->buffer,
					       dd->img_buff0, size);
					/* Clear get_frame_info */
					dd->get_frame_info = NULL;
				}
				/* Clear the buffer ready bit to release the
				 * buffer to unicam output engine
				 */
				sys_write32(BIT(CAMSTA__BUF0_RDY), CAMSTA_ADDR);
				break;
			default:
				break;
			}
		}
	} while (1); /* We don't expect to exit from this thread */
}

/* API to set data and clock lane timer values for unicam controller */
void unicam_set_lane_timer_values(struct unicam_lane_timers *timers)
{
	u32_t val;

	val = 0x0;
	SET_FIELD(val, CAMCLT, CLT1, timers->clt1);
	SET_FIELD(val, CAMCLT, CLT2, timers->clt2);
	sys_write32(val, CAMCLT_ADDR);

	val = 0x0;
	SET_FIELD(val, CAMDLT, DLT1, timers->dlt1);
	SET_FIELD(val, CAMDLT, DLT2, timers->dlt2);
	SET_FIELD(val, CAMDLT, DLT3, timers->dlt3);
	sys_write32(val, CAMDLT_ADDR);
}

static struct unicam_driver_api bcm5820x_unicam_api = {
	.configure	= bcm5820x_unicam_csi_configure,
	.get_config	= bcm5820x_unicam_get_config,
	.start_stream	= bcm5820x_unicam_csi_start_stream,
	.stop_stream	= bcm5820x_unicam_csi_stop_stream,
	.get_frame	= bcm5820x_unicam_csi_get_frame
};

static struct unicam_config unicam_dev_cfg;

static struct unicam_data unicam_dev_data = {
	.num_active_data_lanes = 1,	/* Only one lane is connected on SVK */
	.state = UNICAM_DRV_STATE_UNINITIALIZED,
	.i2c_interface = CONFIG_CAM_I2C_PORT
};

DEVICE_AND_API_INIT(unicam, CONFIG_UNICAM_DEV_NAME,
		    bcm5820x_unicam_csi_init, &unicam_dev_data,
		    &unicam_dev_cfg, POST_KERNEL,
		    CONFIG_UNICAM_DRIVER_INIT_PRIORITY, &bcm5820x_unicam_api);

static int bcm5820x_unicam_csi_init(struct device *dev)
{
	int ret;
	u32_t val;
	k_tid_t id;
	struct unicam_data *dd = (struct unicam_data *)dev->driver_data;

	if (dd->state != UNICAM_DRV_STATE_UNINITIALIZED) {
		SYS_LOG_WRN("Unicam driver already initialized");
		return -EPERM;
	}

	/* Configure the pinmux for I2C and get the driver handle */
	val = sys_read32(CRMU_IOMUX_CONTROL_ADDR);
	switch (dd->i2c_interface) {
	default:
	case I2C0: /* I2C0 pins are dedicated, not muxed with other functions */
		dd->i2c_info.i2c_dev = device_get_binding(CONFIG_I2C0_NAME);
		break;
	case I2C1:
		dd->i2c_info.i2c_dev = device_get_binding(CONFIG_I2C1_NAME);
		SET_FIELD(val, CRMU_IOMUX_CONTROL, CRMU_ENABLE_SMBUS1, 1);
		break;
	case I2C2:
		dd->i2c_info.i2c_dev = device_get_binding(CONFIG_I2C2_NAME);
		SET_FIELD(val, CRMU_IOMUX_CONTROL, CRMU_ENABLE_SMBUS2, 1);
		break;
	}

	if (dd->i2c_info.i2c_dev == NULL)
		return -EIO;

	sys_write32(val, CRMU_IOMUX_CONTROL_ADDR);

	/* Initialize unicam registers */
	init_unicam_regs();

	/* Initialize the camera module */
#ifdef CONFIG_LI_V024M_CAMERA
	ret = li_v024m_init(&dd->i2c_info);
	if (ret) {
		SYS_LOG_ERR("Error initializing LI V024M module: %d\n", ret);
		return ret;
	}
#endif

	/* Create image buffer read task */
	id = k_thread_create(&img_buff_read_task, image_buffer_read_task_stack,
			     TASK_STACK_SIZE, img_buff_read_task_fn, dev,
			     NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	if (id == NULL)
		return -EPERM;

	/* Create the message fifo between ISR and image buffer read task */
	k_fifo_init(&dd->isr_fifo);

	/* Initialize the read buffer lock */
	k_sem_init(&dd->read_buff_lock, 1, 1);

	/* Enable interrupt and install handler */
	IRQ_CONNECT(UNICAM_INTERRUPT_NUM,
		    0, unicam_isr, DEVICE_GET(unicam), 0);
	irq_enable(UNICAM_INTERRUPT_NUM);

	fifo_index = 0;
	dd->alloc_ptr = NULL;

	dd->state = UNICAM_DRV_STATE_INITIALIZED;

	return 0;
}
