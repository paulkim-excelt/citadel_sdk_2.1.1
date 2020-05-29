/**
 * @file
 *
 * @brief Public APIs for the DMA drivers.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <kernel.h>
#include <device.h>
#include <cortex_a/cache.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DMA Interface
 * @defgroup DMA_interface DMA Interface
 * @ingroup io_interfaces
 * @{
 */

enum dma_channel_direction {
	MEMORY_TO_MEMORY = 0x0,
	MEMORY_TO_PERIPHERAL,
	PERIPHERAL_TO_MEMORY,
	PERIPHERAL_TO_PERIPHERAL,
	/* Dest peripheral as controller */
	PERIPHERAL_TO_PERIPHERAL_DEST_CTRL,
	/* Peripheral as controller */
	MEMORY_TO_PERIPHERAL_PER_CTRL,
	/* Peripheral as controller */
	PERIPHERAL_TO_MEMORY_PER_CTRL,
	/* Source peripheral as controller */
	PERIPHERAL_TO_PERIPHERAL_SRC_CTRL
};

enum dma_channel_address_step {
	ADDRESS_INCREMENT = 0x00,
	ADDRESS_DECREMENT = 0x01,
	ADDRESS_NO_CHANGE = 0x02
};

enum dma_channel_type {
	NORMAL_CHANNEL,
	/* Allocated permanently till release */
	ALLOCATED_CHANNEL
};

#ifndef CONFIG_DATA_CACHE_SUPPORT
#define DCACHE_LINE_SIZE	64
#endif

/* Helper macro to align to cache lines size */
#define CACHE_LINE_ALIGN(size)	(((u32_t)size + 0x3F) & ~0x3F)

/**
 * @brief Allocate dcahe line size aligned memory from heap.
 *
 * This routine provides a malloced pointer aligned to the dcache lines size
 * This routine should be used for allocating DMA destination buffers. This
 * alignment restriction is required to clean and invalidate buffers that were
 * written to by the DMA controller.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the aligned memory allocated if successful; otherwise NULL
 */
static inline void *cache_line_aligned_alloc(size_t size)
{
	size_t s;
	u32_t alloc_ptr, ret_ptr;

	/* Round up size to next multiple of 'align' */
	s = ((size + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1))
	    + sizeof(u32_t)		/* Memory for the allocated pointer */
	    + DCACHE_LINE_SIZE - 1; 	/* Extra memory to have the return
	    				 * pointer aligned
	    				 */

	alloc_ptr = (u32_t)k_malloc(s);
	if (alloc_ptr == 0x0)
		return NULL;

	/* Round up allocated pointer to next aligned address */
	ret_ptr = (alloc_ptr + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1);

	/* Check if we have space for the allocated pointer
	 * If not, set the return pointer to the next aligned address.
	 */
	if ((ret_ptr - alloc_ptr) < sizeof(u32_t))
		ret_ptr += DCACHE_LINE_SIZE;

	*(u32_t *)(ret_ptr - 4) = alloc_ptr;

	return (void *)ret_ptr;
}

/**
 * @brief Free memory allocated with cache_line_aligned_alloc.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated using cache_line_aligned_alloc.
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
static inline void cache_line_aligned_free(void *ptr)
{
	if (ptr)
		k_free((void *)(*(u32_t *)((u32_t)ptr - 4)));
}

/**
 * @brief DMA block configuration structure.
 *
 * source_address is block starting address at source
 * source_gather_interval is the address adjustment at gather boundary
 * dest_address is block starting address at destination
 * dest_scatter_interval is the address adjustment at scatter boundary
 * dest_scatter_count is the continuous transfer count between scatter
 *                    boundaries
 * source_gather_count is the continuous transfer count between gather
 *                     boundaries
 * block_size is the number of bytes to be transferred for this block.
 *
 * config is a bit field with the following parts:
 *     source_gather_en   [ 0 ]       - 0-disable, 1-enable
 *     dest_scatter_en    [ 1 ]       - 0-disable, 1-enable
 *     source_addr_adj    [ 2 : 3 ]   - 00-increment, 01-decrement,
 *                                      10-no change
 *     dest_addr_adj      [ 4 : 5 ]   - 00-increment, 01-decrement,
 *                                      10-no change
 *     source_reload_en   [ 6 ]       - reload source address at the end of
 *                                      block transfer
 *                                      0-disable, 1-enable
 *     dest_reload_en     [ 7 ]       - reload destination address at the end
 *                                      of block transfer
 *                                      0-disable, 1-enable
 *     fifo_mode_control  [ 8 : 11 ]  - How full  of the fifo before transfer
 *                                      start. HW specific.
 *     flow_control_mode  [ 12 ]      - 0-source request served upon data
 *                                        availability
 *                                      1-source request postponed until
 *                                        destination request happens
 *     reserved           [ 13 : 15 ]
 */
struct dma_block_config {
	u32_t source_address;
	u32_t source_gather_interval;
	u32_t dest_address;
	u32_t dest_scatter_interval;
	u16_t dest_scatter_count;
	u16_t source_gather_count;
	u32_t block_size;
	struct dma_block_config *next_block;
	u16_t  source_gather_en :  1;
	u16_t  dest_scatter_en :   1;
	u16_t  source_addr_adj :   2;
	u16_t  dest_addr_adj :     2;
	u16_t  source_reload_en :  1;
	u16_t  dest_reload_en :    1;
	u16_t  fifo_mode_control : 4;
	u16_t  flow_control_mode : 1;
	u16_t  reserved :          3;
};

/**
 * @brief DMA configuration structure.
 *
 *     dma_slot             [ 0 : 5 ]   - which peripheral and direction
 *                                        (HW specific)
 *     channel_direction    [ 6 : 8 ]   - 000-memory to memory, 001-memory to
 *                                        peripheral, 010-peripheral to memory,
 *                                        ...
 *     complete_callback_en [ 9 ]       - 0-callback invoked at completion only
 *                                        1-callback invoked at completion of
 *                                          each block
 *     error_callback_en    [ 10 ]      - 0-error callback enabled
 *                                        1-error callback disabled
 *     source_handshake     [ 11 ]      - 0-HW, 1-SW
 *     dest_handshake       [ 12 ]      - 0-HW, 1-SW
 *     channel_priority     [ 13 : 16 ] - DMA channel priority
 *     source_chaining_en   [ 17 ]      - enable/disable source block chaining
 *                                        0-disable, 1-enable
 *     dest_chaining_en     [ 18 ]      - enable/disable destination block
 *                                        chaining.
 *                                        0-disable, 1-enable
 *     reserved             [ 19 : 31 ]
 *
 *     source_data_size    [ 0 : 15 ]   - width of source data (in bytes)
 *     dest_data_size      [ 16 : 31 ]  - width of dest data (in bytes)
 *     source_burst_length [ 0 : 15 ]   - number of source data units
 *     dest_burst_length   [ 16 : 31 ]  - number of destination data units
 *
 *     block_count  is the number of blocks used for block chaining, this
 *     depends on availability of the DMA controller.
 *
 * dma_callback is the callback function pointer. If enabled, callback function
 *              will be invoked at transfer completion or when error happens
 *              (error_code: zero-transfer success, non zero-error happens).
 */
struct dma_config {
	u32_t  dma_slot :             6;
	u32_t  channel_direction :    3;
	u32_t  complete_callback_en : 1;
	u32_t  error_callback_en :    1;
	u32_t  source_handshake :     1;
	u32_t  dest_handshake :       1;
	u32_t  channel_priority :     4;
	u32_t  source_chaining_en :   1;
	u32_t  dest_chaining_en :     1;
	u32_t  reserved :            13;
	u32_t  source_data_size :    16;
	u32_t  dest_data_size :      16;
	u32_t  source_burst_length : 16;
	u32_t  dest_burst_length :   16;
	u32_t  source_peripheral :   16;
	u32_t  dest_peripheral :     16;
	u32_t  endian_mode : 4;
	u32_t block_count;
	struct dma_block_config *head_block;
	void (*dma_callback)(struct device *dev, u32_t channel,
			     int error_code);
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

typedef int (*dma_api_config)(struct device *dev, u32_t channel,
			      struct dma_config *config);

typedef int (*dma_api_start)(struct device *dev, u32_t channel);

typedef int (*dma_api_stop)(struct device *dev, u32_t channel);

typedef int (*dma_api_status)(struct device *dev, u32_t channel, u32_t *status);

typedef int (*dma_api_channel_request)(struct device *dev, u32_t *channel,
				       u8_t type);

typedef int (*dma_api_channel_release)(struct device *dev, u32_t channel);

struct dma_driver_api {
	dma_api_config config;
	dma_api_start start;
	dma_api_stop stop;
	dma_api_status status;
	dma_api_channel_request request;
	dma_api_channel_release release;
};
/**
 * @endcond
 */

/**
 * @brief Configure individual channel for DMA transfer.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 * @param config  Data structure containing the intended configuration for the
 *                selected channel
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_config(struct device *dev, u32_t channel,
			     struct dma_config *config)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->config(dev, channel, config);
}

/**
 * @brief Enables DMA channel and starts the transfer, the channel must be
 *        configured beforehand.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer will
 *                be processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_start(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->start(dev, channel);
}

/**
 * @brief Stops the DMA transfer and disables the channel.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 *                being processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_stop(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->stop(dev, channel);
}

/**
 * @brief Gets the DMA channel status.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 *                being processed
 * @param status  Status of the channel 1: active 0: idle
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_status(struct device *dev, u32_t channel, u32_t *status)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->status(dev, channel, status);
}

/**
 * @brief Gets a DMA channel.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Pointer to the numeric identification of the channel to be
 *                used for the request
 * @param type    Normal or allocated channel
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_request(struct device *dev, u32_t *channel,
			      u8_t type)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->request(dev, channel, type);
}

/**
 * @brief Release the DMA channel.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to be released
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_release(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->release(dev, channel);
}

/**
 * @brief Copy memory using DMA without OS calls
 *
 * @param dev Device struct
 * @param ch Channel number
 * @param cfg Dma config struct
 *
 * @return 0 for success, error otherwise
 */
s32_t dma_memcpy(struct device *dev, u32_t ch, struct dma_config *cfg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _DMA_H_ */
