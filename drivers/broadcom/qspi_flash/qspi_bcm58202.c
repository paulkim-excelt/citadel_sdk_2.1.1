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

/* @file qspi_bcm58202.c
 *
 * QSPI flash driver
 *
 * This driver provides apis to access the serial
 * flash device
 *
 */

/** @defgroup bcm58202_qspi_flash
 * Connectivity SDK: QSPI Flash operations.
 *  @{
 */

#include <board.h>
#include <qspi_flash.h>
#include <arch/cpu.h>
#include <kernel.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/util.h>
#include <string.h>
#include <genpll.h>
#include <cortex_a/cache.h>

#ifdef CONFIG_QSPI_DMA_SUPPORT
#include <broadcom/dma.h>
#endif

#ifdef CONFIG_MEM_LAYOUT_AVAILABLE
#include <arch/arm/soc/bcm58202/layout.h>
#endif

/* Since QSPI driver uses memory mapped regions to read/write the QSPI flash,
 * depending on the read/write address/size and the SMAU window configuration,
 * the QSPI access could straddle across the SMAU window boundary. To handle
 * this case the QSPI access would need to be split into 2 transactions. This
 * define configures the MINIMUM window size that will be set for an SMAU window
 */
#ifndef CONFIG_MIN_SMAU_WINDOW_SIZE
#define CONFIG_MIN_SMAU_WINDOW_SIZE	MB(16)
#else
/* Error if SMAU window size is not a power of 2 */
#if (CONFIG_MIN_SMAU_WINDOW_SIZE) & ((CONFIG_MIN_SMAU_WINDOW_SIZE) - 1)
#error CONFIG_MIN_SMAU_WINDOW_SIZE is not a power of 2
#endif
#endif

#include "qspi_bcm58202_regs.h"
#include "qspi_bcm58202_interface.h"
#include "qspi_bcm58202.h"

/** Mask for the specified field, given register name and field name */
#define MASK(REG, FIELD)	\
	(((0x1 << REG##__##FIELD##_WIDTH) - 1) << REG##__##FIELD##_R)
/** Position of the specified register given register name and field name */
#define POS(REG, FIELD)	 	(REG##__##FIELD##_R)


/** Get Field Macro - From a u32_t */
#define GET_FIELD(VALUE, REG, FIELD)	   \
	((VALUE & MASK(REG, FIELD)) >> POS(REG, FIELD))

/** Set Field Macro - To a u32_t */
#define SET_FIELD(VALUE, REG, FIELD, VAL)	\
	((VALUE) =				\
		((VALUE) & ~MASK(REG, FIELD) |	\
		((VAL) << POS(REG, FIELD) & MASK(REG, FIELD))))

/** Get Field Macro - From a register */
#define GET_REG_FIELD(REG, FIELD)		\
	((*(volatile u32_t *)REG##_ADDR & MASK(REG, FIELD)) >> POS(REG, FIELD))

/** Set Field Macro - To a register */
#define SET_REG_FIELD(REG, FIELD, VAL)		\
	(*(volatile u32_t *)REG##_ADDR =		\
		(*(volatile u32_t *)REG##_ADDR & ~MASK(REG, FIELD)) |	\
		(((VAL) << POS(REG, FIELD)) & MASK(REG, FIELD)))

/*
 * Reason for locking interrupts in QSPI apis
 *
 * The QSPI controller has 3 components. BSPI, MSPI and WSPI. BSPI module makes
 * XIP possible. When the QSPI driver apis are called, the apis manipulate the
 * QSPI registers in one way or another. At this point BSPI is locked out of
 * accessing the flash, so that the MSPI/WSPI interfaces can be used to read
 * flash status registers, set write enable bits, program/erase etc ... Enabling
 * interrupts or making any system calls from the driver can cause code
 * execution from flash, due to a context switch or if the interrupt handlers
 * are in flash etc ... This will result in a CPU hang, because BSPI will not
 * respond and no further instructions will be fetched from the flash. The only
 * way around this is to ..
 *
 * 1. Disable all interrupts
 * 2. Not make system calls that could invoke the scheduler
 * 3. Not call any functions located in flash
 * 4. Relocate the QSPI driver code to run from RAM at bootup
 */

/** Default flash configuration values */
#define CONFIG_PRI_FLASH_CLOCK_FREQUENCY	25000000

/* 25 system clocks @ 200MHz - 125 ns */
#define QSPI_FLASH_DEFAULT_CHIPSEL_PUSLE_WIDTH	25
/*  4 system clocks @ 200MHz - 30 ns */
#define QSPI_FLASH_DEFAULT_PCS_SCK_DELAY	4

/** Dma Done timeout in micro seconds */
#define DMA_DONE_TIMEOUT	1000	/* 1 ms */

/** QSPI device manufacturer IDs for supported flashes */
#define QSPI_FLASH_SPANSION_MFG_ID		0x01
#define QSPI_FLASH_MACRONIX_MFG_ID		0xC2
#define QSPI_FLASH_MICRON_MFG_ID		0x20
#define QSPI_FLASH_GIGA_DEVICE_MFG_ID		0xC8
#define QSPI_FLASH_XMC_MFG_ID			0x20 /* Same MFG ID as Micron*/
#define QSPI_FLASH_WINBOND_MFG_ID		0xEF

/* Use device id to differentiate between micron and xmc devices as they share
 * the same manufacturer id
 */
#define MICRON_DEVICE_ID	0xBA
#define XMC_DEVICE_ID		0x70

/* DMA burst size to generate a single AXI transaction for writes */
#define DMA_BURST_SIZE_FOR_WSPI			0x40

/* BSPI mapped memory location
 * 32M @ 1584M for CS0
 * 32M @ 1616M for CS1
 */
#define BSPI_MEMORY_MAPPED_REGION_START(ss)	\
	((GET_REG_FIELD(BSPI_REGISTERS_BSPI_CS_OVERRIDE, BSPI_DIRECT_SEL) == 0)\
	 ? ((ss) == 0 ? BCM58202_BSPI_CS0_START : BCM58202_BSPI_CS1_START)\
	 : WSPI_MEMORY_MAPPED_REGION_START(ss))

/* WSPI mapped memory location
 * 32M @ 1648M for CS0
 * 32M @ 1680M for CS1
 */
#define WSPI_MEMORY_MAPPED_REGION_START(ss)	\
	((ss) == 0 ? BCM58202_WSPI_CS0_START : BCM58202_WSPI_CS1_START)

/** Default WSPI polling delays */
#define WSPI_POLL_DELAY				39	/* = 25 us */
/* Computation:
 *  1 count delay = 16 sys clk periods
 *  16/25 MHz = 640 nano seconds
 *  So 25 us = 25000/640 = 39
 */

#define WSPI_POLL_TIMEOUT_COUNT			800	/* Wait for a max of 20
							 * milli seconds.
							 * Typical Page program
							 * time 3 - 5 ms
							 */

/* QSPI controller bitfields */
#define CDRAM_MSPI_CDRAM00_PCS_MSK	0x3
#define CDRAM_MSPI_CDRAM00_DT_MSK	0x20
#define CDRAM_MSPI_CDRAM00_DSCK_MSK	0x10
#define CDRAM_MSPI_CDRAM00_BITSE_MSK	0x40
#define CDRAM_MSPI_CDRAM00_CONT_MSK	0x80
#define CDRAM_MSPI_CDRAM00_MODE_MSK	0x30

/* Helper macros */
#define SIZE_16MB		0x01000000
#define BYTE_4_NON_ZERO(A)	(((A) >> 24) != 0)

/* Max number of bytes per transaction is for Page Program
 * command - 1 byte
 * address - 4 bytes
 * data - 256 bytes (One page size)
 */
#define FLASH_PAGE_SIZE	256
#define MAX_BYTES_PER_MSPI_TRANSACTION	(1 + 4 + FLASH_PAGE_SIZE)

/* Bit mask for n bits */
#define GET_MASK(n)		((0x1UL << (n)) - 1)

/** Private functions */
/**
 * @brief       Select Slave
 * @details     This function configures the BSPI, MSPI and WSPI controllers
 *              to access the specified slave CS0/CS1
 *
 * @param[in]   ss - slave select
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int qspi_select_slave(struct qspi_driver_data *dd);

/**
 * @brief       Enumerates flash device
 * @details     This function enumerates the flash device and
 *              returns the flash device type
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[out]  flash_type - Enumerated flash type
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int qspi_flash_enumerate(
		struct qspi_driver_data *dd,
		qspi_serial_flash_type_e *flash_type);

/**
 * @brief       Erases a flash chunk
 * @details     This function erases a flash chunk
 *              that does not cross the 16MB boundary
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to erase data from
 * @param[in]   len - Data size to erase
 * @param[in]   erase_command - SPI command for erasing
 * @param[in]   erase_size - The erase size for the command given
 * @param[in]   four_byte_enable - true if address >= 16M
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int qspi_flash_erase_chunk(
		struct qspi_driver_data *dd,
		u32_t address,
		u32_t len,
		u32_t erase_size,
		bool four_byte_enable);

#ifndef CONFIG_USE_BSPI_FOR_READ
/**
 * @brief       Read data through MSPI
 * @details     This function uses the MSPI controller to read
 *              data one byte at a time
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to read data from
 * @param[in]   len - Data size to read
 * @param[in]   data - buffer to read data into
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int mspi_read_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data);
#endif

#ifdef CONFIG_USE_BSPI_FOR_READ
/**
 * @brief       Read data through BSPI
 * @details     This function reads data using BSPI interface
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to read data from
 * @param[in]   len - Data size to read
 * @param[in]   data - buffer to read data into
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int bspi_read_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data);
#endif /* CONFIG_USE_BSPI_FOR_READ */

/**
 * @brief       Write data through MSPI
 * @details     This function uses the MSPI controller to write
 *              data one byte at a time
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to write data to
 * @param[in]   len - Data size to write
 * @param[in]   data - buffer to write data from
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int mspi_write_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data);

#ifdef CONFIG_USE_WSPI_FOR_WRITE
/**
 * @brief       Write data through WSPI
 * @details     This function write data using WSPI interface
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to write data to
 * @param[in]   len - Data size to write
 * @param[in]   date - buffer to write data from
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int wspi_write_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data);
#endif /* CONFIG_USE_WSPI_FOR_WRITE */

/**
 * @brief       Get erase command
 * @details     This function retrieves the most optimal flash device
 *              specific erase command for the given address and size
 *
 * @param[in]   dd - Pointer to the qspi driver data for this driver instance
 * @param[in]   address - Address to erase
 * @param[out]  command - SPI erase command as determined by the function
 * @param[inout]size - Caller specifies the erase size and the function
 *                     returns the erase size for the erase command returned
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int qspi_get_erase_command(
		struct qspi_driver_data *dd,
		u32_t address,
		u8_t *command,
		u32_t *size);

/**
 * @brief       Return number of memory chunks
 * @details     This function determines the number of memory chunks in the
 *              specified region, where each chunk does not cross the 16 M
 *              boundary. This info is useful to determine if we need to enter
 *              4 byte addressing mode on the flash device
 *
 * @param[in]   address - Starting address of memory region
 * @param[in]   len - Size to the memory region
 * @param[out]  addresses - Starting address of all memory chunks
 * @param[out]  sizes - Size of all memory chunks
 *
 * @return      number of memory chunks - either 1 or 2
 */
static int qspi_get_mem_chunks(
		u32_t address,
		u32_t len,
		u32_t *addresses,
		u32_t *sizes);
/**
 * @brief       Flush the BSPI pre-fetch buffers
 * @details     This function flushes the BSPI pre-fetch buffers 0 and 1
 */
static void bspi_flush_prefetch_buffers(void);

#ifdef CONFIG_ENABLE_CACHE_INVAL_ON_WRITE
/**
 * @brief       Invalidate the memory mapped cache along the read path
 * @details     This function will invalidate all caches along the read path
 *		from flash to CPU to ensure cache coherency. This is required,
 *		because the memory map for read and write are not the same.
 *		There are 2 caches along the read path- SMAU and D-Cache.
 */
static void qspi_invalidate_memory_mapped_caches(
		struct qspi_driver_data *dd, u32_t addr, u32_t len);
#endif

/* RAM start and end addresses to check QSPI always executes out of RAM */
extern u32_t __RAM_START;
extern u32_t __RAM_END;

static struct qspi_driver_data *primary_driver_data = NULL;

#ifdef CONFIG_QSPI_DMA_SUPPORT
/* Dma Channel ID - Shared between all QSPI driver objects
 * Since this will be used only when interrupts are locked
 * and that will ensure its atomic usage by the qspi driver
 * object using the dma channel
 */
#define DMA_CHANNEL_INVALID	(~0UL)
static u32_t dma_chan_id = DMA_CHANNEL_INVALID;
static struct device *dma_dev = NULL;
#endif

/* The qspi source clock is saved at init, so that the clock driver api
 * clk_get_ahb(), which is located in flash is not called during flash
 * controller configuration.
 */
static u32_t qspi_src_clk;

/* @brief QSPI driver data */
struct qspi_driver_data {
	/* Flash configuration - This is initialized to the device->config
	 * at device init and is updated later with calls to
	 * qspi_flash_configure
	 */
	struct qspi_flash_config flash_config;

	/* CDRAM masks */
	u32_t cdram_mask;

	/* Flash device specific command and status bitmask information */
	qspi_flash_command_info_s command_info;

	/* Flash device interfaces */
	qspi_flash_device_interface_s *interface;

	/* Flash sizes in bytes */
	u32_t flash_size;

	/* Flash device information */
	struct qspi_flash_info info;
};

static int interface_not_available(qspi_flash_device_interface_s **interface);

/* Flash specific get interface function pointers */
#pragma weak qspi_flash_spansion_get_interface = interface_not_available
#pragma weak qspi_flash_macronix_get_interface = interface_not_available
#pragma weak qspi_flash_micron_get_interface = interface_not_available
#pragma weak qspi_flash_giga_device_get_interface = interface_not_available
#pragma weak qspi_flash_xmc_get_interface = interface_not_available
#pragma weak qspi_flash_winbond_get_interface = interface_not_available


static qspi_flash_get_interface flash_get_interface[FLASH_MAXIMUM] = {
	qspi_flash_spansion_get_interface,
	qspi_flash_macronix_get_interface,
	qspi_flash_micron_get_interface,
	qspi_flash_giga_device_get_interface,
	qspi_flash_xmc_get_interface,
	qspi_flash_winbond_get_interface,
};

static u32_t qspi_get_min_erase_sector_size(struct qspi_driver_data *dd);

const char *flash_device_names[FLASH_MAXIMUM] = {
	"Spansion",
	"Macronix",
	"Micron",
	"Giga Device",
	"XMC",
	"Winbond",
};

/**
 * @brief       QSPI Driver initialization
 * @details     This api initializes the QSPI flash driver. It enumerates the
 *              flash devices selected by PCS0 or PCS1 (Peripheral Chip Select)
 *              and populates device specific configuration structures needed to
 *              transact with the flash devices through various SPI modules.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on Success
 * @return      errno on Failure
 */
static int bcm58202_qspi_flash_init(struct device *dev)
{
	int ret = 0;
	qspi_serial_flash_type_e flash_type;
	struct qspi_driver_data *dd;
	struct qspi_flash_config *dev_config;
	u32_t key = 0;

	if (dev->driver_data) {
		SYS_LOG_WRN("QSPI driver already initialized");
		return -EPERM;
	}

	ret = clk_get_ahb(&qspi_src_clk);
	if (ret) {
		SYS_LOG_WRN("QSPI source clock rate cannot be determined\n");
		SYS_LOG_WRN("Assuming default 200 MHz\n");
		qspi_src_clk = MHZ(200);
	}

	dd = (struct qspi_driver_data *)k_malloc(
					sizeof(struct qspi_driver_data));
	if (dd == NULL) {
		SYS_LOG_ERR("Error allocating driver data for QSPI driver");
		return -ENOMEM;
	}

	/* Lock interrupts
	 * See comment under header "Reason for locking interrupts in QSPI apis"
	 * at the top of the file, for detailed explanation.
	 */
	key = irq_lock();

	/* Reset driver data */
	memset(dd, 0, sizeof(*dd));

	dev_config = (struct qspi_flash_config *)dev->config->config_info;

	/* Initialize default flash configuration
	 * We make a copy here, because the config_info in dev->config
	 * resides on read only memory (in this case the flash). And
	 * we shouldn't be trying to access the flash when we are executing
	 * flash driver apis, as this will cause a clash between BSPI (which
	 * takes over the qspi contoller when flash is read through memory map)
	 * and MSPI (which is used to transact with the flash in the qspi apis).
	 * The result will be an unresponsive BSPI resulting in CPU hang
	 */
	dd->flash_config = *dev_config;

	/* Enumerate the flash
	 *  - get interface for the flash
	 *  - get command info for the flash
	 */
	ret = qspi_flash_enumerate(dd, &flash_type);
	if (ret || (flash_type >= FLASH_MAXIMUM)) {
		/* We expect only the primary flash to be connected */
		if (dev_config->slave_num == 0)
			SYS_LOG_ERR("Error enumerating flash");
		goto cleanup;
	}

	ret = flash_get_interface[flash_type](&dd->interface);
	if (ret || (dd->interface == NULL)) {
		SYS_LOG_ERR("Flash Type not supported : [Slave # = %d]",
						dev_config->slave_num);
		ret = -EOPNOTSUPP;
		goto cleanup;
	}

	ret = dd->interface->get_command_info(&dd->command_info);
	if (ret) {
		SYS_LOG_ERR("Error getting command info for flash");
		goto cleanup;
	}
	/* Get the device info */
	dd->info.min_erase_sector_size = qspi_get_min_erase_sector_size(dd);
	ret = dd->interface->get_size(dd, &dd->info.flash_size);
	if (ret)
		goto cleanup;

	dd->flash_size = dd->info.flash_size;

	SYS_LOG_INF("Detected flash : %s [%d KB] @CS%d",
		    flash_device_names[flash_type], (dd->flash_size >> 10),
		    dd->flash_config.slave_num);

	if (dd->flash_config.slave_num == 0) {
		ret = qspi_select_slave(dd);
		if (ret) {
			SYS_LOG_ERR("Could not set current slave select to: %d",
				    dd->flash_config.slave_num);
			goto cleanup;
		}
	}

	dev->driver_data = dd;

	if (dd->flash_config.slave_num == 0)
		primary_driver_data = dd;

	/* Unlock interrupts */
	irq_unlock(key);

	/* Allocate dma channel - This can be done after unlocking interrupts */
#ifdef CONFIG_QSPI_DMA_SUPPORT
	/* Check to make sure a previous qspi driver instantiation did
	 * not already allocate a DMA channel
	 */
	if (dma_chan_id == DMA_CHANNEL_INVALID) {
		dma_dev = device_get_binding(CONFIG_DMA_PL080_NAME);
		if (dma_dev != NULL) {
			ret = dma_request(dma_dev,
					  &dma_chan_id,
					  ALLOCATED_CHANNEL);
			/* Allocation failed, set dma channel id to invalid */
			if (ret)
				dma_chan_id = DMA_CHANNEL_INVALID;
		}
	}
	if (dma_chan_id == DMA_CHANNEL_INVALID)
		SYS_LOG_WRN("Warning: DMA channel allocation failed. QSPI "
			    "driver will still function, but without DMA");
#endif

	return 0;

cleanup:
	/* Unlock interrupts */
	irq_unlock(key);

	/* Free malloc'ed memory */
	k_free(dd);

	return ret;
}

/**
 * @brief       Set flash device config
 * @details     This api sets the serial flash configuration for the
 *              specified flash device.
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   config - Pointer to flash device configuration
 *
 * @return      0 on success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_set_config(
		struct device *dev,
		struct qspi_flash_config *config)
{
	int ret = 0;
	u32_t key = 0;
	struct qspi_driver_data *dd;

	/* Check for driver initialization */
	if (dev->driver_data == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	/* Check config parameter */
	if (config == NULL)
		return -EINVAL;

	/* Lock interrupts
	 * See comment under header "Reason for locking interrupts in QSPI apis"
	 * at the top of the file, for detailed explanation.
	 */
	key = irq_lock();

	dd = (struct qspi_driver_data *)dev->driver_data;
	dd->flash_config = *config;

	/* If the configuration is being changed for the active (primary) slave,
	 * re-configure the QSPI controller
	 */
	if (config->slave_num == 0) {
		ret = qspi_select_slave(dev->driver_data);
		if (ret) {
			SYS_LOG_ERR("Could not update config for flash : #%d",
				     config->slave_num);
			goto done;
		}
	}

done:
	/* Unlock interrupts */
	irq_unlock(key);

	return ret;
}

/**
 * @brief       Get flash device Info
 * @details     This api gets the serial flash device info
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[out]  info - Pointer to flash device info to be populated
 *
 * @return      0 on success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_get_dev_info(
		struct device *dev,
		struct qspi_flash_info *info)
{
	struct qspi_driver_data *dd;

	/* Check for driver initialization */
	if (dev->driver_data == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	/* Param Check */
	if (info == NULL)
		return -EINVAL;

	dd = (struct qspi_driver_data *)dev->driver_data;
	*info = dd->info;

	return 0;
}

/**
 * @brief       Read the flash memory
 * @details     This api reads data from the flash device at specified offset
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - flash offset to read the data from
 * @param[in]   data_len - number of bytes to read
 * @param[out]  data - buffer to read the data to
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_read(
		struct device *dev,
		u32_t address,
		u32_t data_len,
		u8_t *data)
{
	int ret = 0;
	u32_t key = 0;
	struct qspi_driver_data *dd;

	dd = (struct qspi_driver_data *)dev->driver_data;
	/* Check for driver initialization */
	if (dd == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	if ((data_len > SIZE_16MB) /* Api supports max 16 M size for write */
		|| (data_len == 0)
		|| (data == NULL)) {
		return -EINVAL;
	}

	/* Check if interface is available */
	if (dd->interface == NULL)
		return -EOPNOTSUPP;

	/* Destination buffer cannot be in flash */
	if (((u32_t)data >= BCM58202_BSPI_CS0_START) &&
	    ((u32_t)data < (BCM58202_WSPI_CS1_START + BCM58202_WSPI_CS1_SIZE)))
		return -EINVAL;

	if (dd->flash_config.slave_num != 0) {
		/* Lock interrupts
		 * See comment under header "Reason for locking interrupts in
		 * QSPI apis" at the top of the file, for detailed explanation.
		 */
		key = irq_lock();

		/* Switch slave select to requested slave */
		ret = qspi_select_slave(dd);
		if (ret) {
			SYS_LOG_ERR("Could not switch to flash : #%d",
				     dd->flash_config.slave_num);
			goto done;
		}
	}

	/* Check if the read region is within the flash address range
	 * Called after qspi_select_slave(), because flash_size is set
	 * after qspi_select_slave()
	 */
	if ((address + data_len) > dd->flash_size) {
		ret = -EINVAL;
		goto done;
	}

#ifdef CONFIG_USE_BSPI_FOR_READ
		ret = bspi_read_data(dd, address, data_len, data);
#else
		ret = mspi_read_data(dd, address, data_len, data);
#endif

done:
	if (dd->flash_config.slave_num != 0) {
		/* Configure QSPI controller for flash 0, as the image
		 * we are exeucting out of resides in the primary flash
		 */
		qspi_select_slave(primary_driver_data);
		/* Unlock interrupts */
		irq_unlock(key);
	}

	return ret;
}

/**
 * @brief       Erase serial flash
 * @details     This api erases the flash sectors specified by the address
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - Address of the sector that needs to be erased. Note
 *                        that address should be aligned to the sector boundary
 * @param[in]   len - Number of bytes to be erased. Note length needs to be a
 *                    multiple of the flash sector size
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_erase(
		struct device *dev,
		u32_t address,
		u32_t len)
{
	int ret = 0;
	u32_t key = 0;
	u8_t command;
	u32_t erase_size;
	u32_t sizes[2];
	u32_t addresses[2];
	u32_t num_chunks, i;
	struct qspi_driver_data *dd;

	dd = (struct qspi_driver_data *)dev->driver_data;
	/* Check for driver initialization */
	if (dd == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	if ((len > SIZE_16MB) /* Api supports max 16 M size for erase */
		|| (len == 0)) {
		return -EINVAL;
	}

	/* Check if interface is available */
	if (dd->interface == NULL)
		return -EOPNOTSUPP;

	/* Check if the erase region is within the flash address range
	 * Called after qspi_select_slave(), because flash_size is set
	 * after qspi_select_slave()
	 */
	if ((address + len) > dd->flash_size)
		return -EINVAL;

	/* Lock interrupts
	 * See comment under header "Reason for locking interrupts in QSPI apis"
	 * at the top of the file, for detailed explanation.
	 */
	key = irq_lock();

	if (dd->flash_config.slave_num != 0) {
		/* Switch slave select to requested slave */
		ret = qspi_select_slave(dd);
		if (ret) {
			SYS_LOG_ERR("Could not switch to flash : #%d",
				     dd->flash_config.slave_num);
			goto done;
		}
	}

	/* Check if it is possible to erase the length provided */
	erase_size = len;
	ret = qspi_get_erase_command(dd, address, &command, &erase_size);
	if (ret) {
		SYS_LOG_ERR("Flash device [#%d] does not support erase commands"
			    " for size %d", dd->flash_config.slave_num, len);
		goto done;
	}

	/* Check if the range crosses 16M boundary */
	num_chunks = qspi_get_mem_chunks(address, len, addresses, sizes);

	for (i = 0; i < num_chunks; i++) {
		bool four_byte_enable = BYTE_4_NON_ZERO(addresses[i]);

		ret = qspi_flash_erase_chunk(dd, addresses[i], sizes[i],
					     erase_size, four_byte_enable);
		if (ret)
			break;
	}

done:
	if (dd->flash_config.slave_num != 0) {
		/* Configure QSPI controller for flash 0, as the image
		 * we are exeucting out of resides in the primary flash
		 */
		qspi_select_slave(primary_driver_data);
	}

#ifdef CONFIG_ENABLE_CACHE_INVAL_ON_WRITE
	/* Invalidate caches */
	qspi_invalidate_memory_mapped_caches(dd, address, len);
#endif

	/* Unlock interrupts */
	irq_unlock(key);

	return ret;
}

/**
 * @brief       Write to the flash memory
 * @details     This api writes data to the flash device at the specified offset
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 * @param[in]   address - flash offset to write the data to
 * @param[in]   data_len - number of bytes to write
 * @param[in]   data - Data to be written to flash
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_write(
		struct device *dev,
		u32_t address,
		u32_t data_len,
		u8_t *data)
{
	int ret = 0;
	u32_t key = 0;
	struct qspi_driver_data *dd;

	dd = (struct qspi_driver_data *)dev->driver_data;

	/* Check for driver initialization */
	if (dd == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	if ((data_len > SIZE_16MB) /* Api supports max 16 M size for write */
		|| (data_len == 0)
		|| (data == NULL)) {
		return -EINVAL;
	}

	/* Check if interface is available */
	if (dd->interface == NULL)
		return -EOPNOTSUPP;

	/* Source buffer cannot be in flash */
	if (((u32_t)data >= BCM58202_BSPI_CS0_START) &&
	    ((u32_t)data < (BCM58202_WSPI_CS1_START + BCM58202_WSPI_CS1_SIZE)))
		return -EINVAL;

	/* Check if the write region is within the flash address range
	 * Called after qspi_select_slave(), because flash_size is set
	 * after qspi_select_slave()
	 */
	if ((address + data_len) > dd->flash_size)
		return -EINVAL;

	/* Lock interrupts
	 * See comment under header "Reason for locking interrupts in QSPI apis"
	 * at the top of the file, for detailed explanation.
	 */
	key = irq_lock();

	if (dd->flash_config.slave_num != 0) {
		/* Switch slave select if needed */
		ret = qspi_select_slave(dd);
		if (ret) {
			SYS_LOG_ERR("Could not switch to flash  : #%d",
				     dd->flash_config.slave_num);
			goto done;
		}
	}

#ifdef CONFIG_USE_WSPI_FOR_WRITE
	/* Use MSPI for single byte write
	 * or if page program is not supported by the flash
	 * or if we are not allowed to use WSPI
	 */
	if (dd->command_info.page_program == false) {
		/* Use MSPI */
		ret = mspi_write_data(dd, address, data_len, data);
	} else {
		/* Use WSPI with DMA */
		ret = wspi_write_data(dd, address, data_len, data);
		/* Flush BSPI buffers after write to flash */
		bspi_flush_prefetch_buffers();
	}
#else
	ret = mspi_write_data(dd, address, data_len, data);
#endif

done:
	if (dd->flash_config.slave_num != 0) {
		/* Configure QSPI controller for flash 0, as the image
		 * we are exeucting out of resides in the primary flash
		 */
		qspi_select_slave(primary_driver_data);
	}

#ifdef CONFIG_ENABLE_CACHE_INVAL_ON_WRITE
	/* Invalidate caches */
	qspi_invalidate_memory_mapped_caches(dd, address, data_len);
#endif

	/* Unlock interrupts */
	irq_unlock(key);

	return ret;
}

/**
 * @brief       Erase entire serial flash
 * @details     This api erases the complete serial flash
 *
 * @param[in]   dev - Pointer to the device structure for the driver instance
 *
 * @return      0 on Success
 * @return      errno on failure
 */
static int bcm58202_qspi_flash_chip_erase(struct device *dev)
{
	int ret = 0;
	u32_t key = 0;
	u8_t command;
	u8_t status;
	struct qspi_driver_data *dd;
	qspi_flash_command_info_s *p_cmd;

	dd = (struct qspi_driver_data *)dev->driver_data;
	/* Check for driver initialization */
	if (dd == NULL) {
		SYS_LOG_WRN("QSPI driver not initialized");
		return -EPERM;
	}

	/* Lock interrupts
	 * See comment under header "Reason for locking interrupts in QSPI apis"
	 * at the top of the file, for detailed explanation.
	 */
	key = irq_lock();

	if (dd->flash_config.slave_num != 0) {
		/* Switch slave select if needed */
		ret = qspi_select_slave(dd);
		if (ret) {
			SYS_LOG_ERR("Could not switch to flash  : #%d",
				     dd->flash_config.slave_num);
			goto done;
		}
	}

	p_cmd = &dd->command_info;

	/* First Send write enable latch command */
	command = p_cmd->write_enable;
	ret = mspi_transfer(dd, &command, 1, NULL, 0);
	if (ret)
		goto done;

	/* Send Chip erase command */
	command = p_cmd->chip_erase;
	ret = mspi_transfer(dd, &command, 1, NULL, 0);
	if (ret)
		goto done;

	/* Wait for busy bit to be cleared */
	do {
		command = p_cmd->read_status;
		ret = mspi_transfer(dd, &command, 1, &status, 1);
		if (ret)
			break;

		/* Chip erase typically takes 10s of seconds */
	} while (status & (0x1 << p_cmd->busy_bit_pos));

	/* Send write disable command */
	command = p_cmd->write_disable;
	mspi_transfer(dd, &command, 1, NULL, 0);

done:
	if (dd->flash_config.slave_num != 0) {
		/* Configure QSPI controller for flash 0, as the image
		 * we are exeucting out of resides in the primary flash
		 */
		qspi_select_slave(primary_driver_data);
	}

#ifdef CONFIG_ENABLE_CACHE_INVAL_ON_WRITE
	/* Invalidate caches */
	qspi_invalidate_memory_mapped_caches(dd, 0, dd->info.flash_size);
#endif

	/* Unlock interrupts */
	irq_unlock(key);

	return ret;
}

/** Private functions */
static int interface_not_available(qspi_flash_device_interface_s **interface)
{
	*interface = NULL;

	return -EOPNOTSUPP;
}

#ifdef CONFIG_USE_BSPI_FOR_READ
static u8_t *smau_get_virtual_from_physical(u32_t phys_addr)
{
	u32_t win, base, size;

	/* Check which window the address belongs to */
	for (win = 0; win < 4; win++) {
		base = sys_read32(SMU_WIN_BASE_0 + win*4);
		/* Size is programed as the 2's complement */
		size = ~(sys_read32(SMU_WIN_SIZE_0 + win*4)) + 1;

		if ((phys_addr >= base)
			&& (phys_addr < (base + size))) {
			return (u8_t *)(BCM58202_BSPI_CS0_START + win*SIZE_16MB
					+ (phys_addr - base));
		}
	}

	/* Return the physical address if no windows were configured */
	return (u8_t *)phys_addr;
}
#endif

#ifdef CONFIG_ENABLE_CACHE_INVAL_ON_WRITE
static void qspi_invalidate_memory_mapped_caches(
		struct qspi_driver_data *dd, u32_t addr, u32_t len)
{
#ifdef CONFIG_DATA_CACHE_SUPPORT
	u32_t base, num_addr_bits, addresses[2], sizes[2], num_chunks, i;
#else
	ARG_UNUSED(dd);
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
#endif

	/* Invalidate SMAU cache if SMAU path is enabled for read */
	if (!GET_REG_FIELD(BSPI_REGISTERS_BSPI_CS_OVERRIDE, BSPI_DIRECT_SEL)) {
		SET_REG_FIELD(SMU_CONTROL, CACHE_FLUSH, 1);
		/* Wait for the cache flush bit to clear itself */
		while (GET_REG_FIELD(SMU_CONTROL, CACHE_FLUSH) != 0)
			;
	}

#if defined CONFIG_DATA_CACHE_SUPPORT && defined CONFIG_USE_BSPI_FOR_READ

	/* Invalidating data cache */
	base = (dd->flash_config.slave_num) ?
		BCM58202_BSPI_CS1_START :
		BCM58202_BSPI_CS0_START;

	/* Get num_addr_bits for the flash device in 4 byte addressing mode */
	num_addr_bits = dd->command_info.num_addr_bits;

	num_chunks = qspi_get_mem_chunks(addr, len, addresses, sizes);
	for (i = 0; i < num_chunks; i++) {
		/* Some flashes always use 24 bits even for addressing upper
		 * 16 MB with an internal 4th byte register to complement the 24
		 * bits on the SPI line. For these flash devices the address
		 * with the 24 bit offset should be invalidated
		 */
		if (num_addr_bits == 24)
			addresses[i] &= SIZE_16MB - 1;

		invalidate_dcache_by_addr(
		    (u32_t)smau_get_virtual_from_physical(base + addresses[i]),
		    sizes[i]);
	}
#endif /* CONFIG_DATA_CACHE_SUPPORT && CONFIG_USE_BSPI_FOR_READ */
}
#endif /* CONFIG_ENABLE_CACHE_INVAL_ON_WRITE */

static int qspi_get_erase_command_for_size(struct qspi_driver_data *dd,
					   u32_t erase_size,
					   u8_t *erase_command)
{
	int ret;
	u8_t count, i;
	qspi_erase_command_info_s *erase_cmds;

	if (dd == NULL)
		return -EINVAL;

	ret = dd->interface->get_erase_commands(&erase_cmds, &count);
	if (ret)
		return ret;

	for (i = 0; i < count; i++) {
		if (erase_cmds[i].erase_size == erase_size) {
			*erase_command = erase_cmds[i].erase_command;
			return 0;
		}
	}

	return -EOPNOTSUPP;
}

static int qspi_flash_erase_chunk(struct qspi_driver_data *dd,
				  u32_t address,
				  u32_t len,
				  u32_t erase_size,
				  bool four_byte_enable)
{
	int ret = 0;
	int num_bytes;
	u8_t i;
	u8_t status;
	u8_t command[5];	/* 1 byte for command and 4 bytes for address */
	u32_t num_erases;
	u8_t erase_command;

	qspi_flash_command_info_s *p_cmd = &dd->command_info;

	/* Enter 4 byte mode */
	if (four_byte_enable) {
		dd->interface->enable_4byte_addr(
				    dd,
				    QSPI_DATA_LANE(dd->flash_config.op_mode),
				    address);
	}

	/* Compute number of erases needed */
	num_erases = len / erase_size;
	ret = qspi_get_erase_command_for_size(dd, erase_size, &erase_command);
	if (ret)
		goto done;

	for (i = 0; i < num_erases; i++) {
		u32_t erase_addr;

		/* First Send write enable latch command
		 * Need WEL for every transaction because some flash devices
		 * reset WEL latch after the sector erase is complete
		 */
		command[0] = p_cmd->write_enable;
		ret = mspi_transfer(dd, &command[0], 1, NULL, 0);
		if (ret)
			break;

		/* Erase start address */
		erase_addr = address + (i * erase_size);

		/* Send erase command */
		num_bytes = 0;
		command[num_bytes++] = erase_command;
		if (four_byte_enable && (p_cmd->num_addr_bits == 32)) {
			command[num_bytes++] = (u8_t)(erase_addr >> 24);
		}
		command[num_bytes++] = (u8_t)(erase_addr >> 16);
		command[num_bytes++] = (u8_t)(erase_addr >> 8);
		command[num_bytes++] = (u8_t)(erase_addr);

		ret = mspi_transfer(dd, &command[0], num_bytes, NULL, 0);
		if (ret)
			break;
		/* Wait for busy bit to be cleared */
		do {
			command[0] = p_cmd->read_status;
			ret = mspi_transfer(dd, &command[0], 1, &status, 1);
			if (ret)
				break;
		} while (status & (0x1 << p_cmd->busy_bit_pos));

		if (ret)
			break;
	}

	/* Send write disable command */
	command[0] = p_cmd->write_disable;
	mspi_transfer(dd, &command[0], 1, NULL, 0);

done:
	/* Exit 4 byte mode */
	if (four_byte_enable)
		dd->interface->disable_4byte_addr(dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode));

	return ret;
}

static void bspi_flush_prefetch_buffers(void)
{
	/* Flush BSPI pre-fetch buffers - Needs a rising edge on the register */
	SET_REG_FIELD(BSPI_REGISTERS_B0_CTRL, B0_FLUSH, 0);
	SET_REG_FIELD(BSPI_REGISTERS_B0_CTRL, B0_FLUSH, 1);

	SET_REG_FIELD(BSPI_REGISTERS_B1_CTRL, B1_FLUSH, 0);
	SET_REG_FIELD(BSPI_REGISTERS_B1_CTRL, B1_FLUSH, 1);
}

static void mspi_acquire_bus(void)
{
	/* If BSPI is busy, wait for BSPI to complete the transaction */
	if (GET_REG_FIELD(BSPI_REGISTERS_MAST_N_BOOT_CTRL, MAST_N_BOOT)
	    == 0x0) {
		/* Poll on bspi busy status bit */
		while (GET_REG_FIELD(BSPI_REGISTERS_BUSY_STATUS, BUSY) == 0x1)
			;
	}

	/* Claim the bus by setting mast_n_boot bit */
	SET_REG_FIELD(BSPI_REGISTERS_MAST_N_BOOT_CTRL, MAST_N_BOOT, 0x1);
}

static void mspi_release_bus(void)
{
	/* Flush pre-fetch buffers */
	bspi_flush_prefetch_buffers();

	/* Release the bus by clearing mast_n_boot bit */
	SET_REG_FIELD(BSPI_REGISTERS_MAST_N_BOOT_CTRL, MAST_N_BOOT, 0x0);
}

static u32_t brcm58202_qspi_flash_div(u32_t num, u32_t den)
{
	u32_t quo = 0;

	if (den == 0) {
		quo = 0xFFFFFFFF;
	} else {
		while (num >= den) {
			num -= den;
			quo++;
		}
	}

	return quo;
}

static void mspi_common_init(struct qspi_driver_data *dd)
{
	u32_t spbr;
	u8_t ss = dd->flash_config.slave_num;
	struct qspi_flash_config *fc = &dd->flash_config;

	/** MSPI initialization **/
	/* Set Chip select PCS[1:0]
	 * - The PCS bits are active low and need to be flipped
	 */
	dd->cdram_mask = (0x1 << ss) ^ CDRAM_MSPI_CDRAM00_PCS_MSK;

	/* Baud rate */
	spbr = brcm58202_qspi_flash_div(qspi_src_clk, (2 * fc->frequency));
	spbr = max(spbr, 8);    /* Min Divisor is 8 */
	spbr = min(spbr, 256);  /* Max Divisor is 256 */
	spbr &= 0xFF; /* 256 maps to 0 */
	SET_REG_FIELD(MSPI_SPCR0_LSB, SPBR, spbr);

	/* Clock phase and polarity */
	SET_REG_FIELD(MSPI_SPCR0_MSB, CPHA, QSPI_CLOCK_PHASE(fc->op_mode));
	SET_REG_FIELD(MSPI_SPCR0_MSB, CPOL, QSPI_CLOCK_POL(fc->op_mode));

	/* 8 bits per transfer */
	SET_REG_FIELD(MSPI_SPCR0_MSB, BITS, 8);
	/* No start transaction delay */
	SET_REG_FIELD(MSPI_SPCR0_MSB, STARTTRANSDELAY, 0);
	/* SPI master mode */
	SET_REG_FIELD(MSPI_SPCR0_MSB, MSTR, 1);

	/* Delays */
	SET_REG_FIELD(MSPI_SPCR1_LSB, DTL,
		      QSPI_FLASH_DEFAULT_CHIPSEL_PUSLE_WIDTH);
	/* Set the CDRAM DT mask */
	dd->cdram_mask |= CDRAM_MSPI_CDRAM00_DT_MSK;

	SET_REG_FIELD(MSPI_SPCR1_MSB, RDSCLK, QSPI_FLASH_DEFAULT_PCS_SCK_DELAY);
	/* Set the CDRAM DSCK mask */
	dd->cdram_mask |= CDRAM_MSPI_CDRAM00_DSCK_MSK;

	/* Set other CDRAM fields
	 * always 8 bits per transfer
	 */
	dd->cdram_mask &= ~CDRAM_MSPI_CDRAM00_BITSE_MSK;

	/* The device specific driver will initialize the mode based on the
	 * config.op_mode value and the device's capability to support Dual/Quad
	 * lane modes. So here we default to single lane mode
	 */
	dd->cdram_mask &= ~CDRAM_MSPI_CDRAM00_MODE_MSK;

	/* Clear status bits */
	SET_REG_FIELD(MSPI_MSPI_STATUS, SPIF, 0);
	SET_REG_FIELD(MSPI_MSPI_STATUS, HALTA, 0);

	/* If spe bit is set, mspi transaction is in progress
	 * - Need to use the halt bit for a clean shut down
	 */
	if (GET_REG_FIELD(MSPI_SPCR2, SPE) != 0) {
		/* Reset MSPI by setting HALT bit */
		SET_REG_FIELD(MSPI_SPCR2, HALT, 1);

		/* Wait for HALTA */
		while (GET_REG_FIELD(MSPI_MSPI_STATUS, HALTA) != 1)
			;

		/* Clear the spe bit */
		SET_REG_FIELD(MSPI_SPCR2, SPE, 0);
	}

	/* Clear status bits again */
	SET_REG_FIELD(MSPI_MSPI_STATUS, SPIF, 0);
	SET_REG_FIELD(MSPI_MSPI_STATUS, HALTA, 0);
}

static void wspi_common_init(struct qspi_driver_data *dd)
{
	u32_t spbr;
	struct qspi_flash_config *fc = &dd->flash_config;

	/* Baud rate */
	spbr = brcm58202_qspi_flash_div(qspi_src_clk, (2 * fc->frequency));
	spbr = max(spbr, 4);    /* Min Divisor is 4 */
	spbr = min(spbr, 256);  /* Max Divisor is 256 */
	spbr &= 0xFF; /* 256 maps to 0 */

	SET_REG_FIELD(WSPI_DSCLK_CSPW_SPBR, SPBR, spbr);

	/* Delays */
	SET_REG_FIELD(WSPI_DSCLK_CSPW_SPBR, CSPW,
		      QSPI_FLASH_DEFAULT_CHIPSEL_PUSLE_WIDTH);
	SET_REG_FIELD(WSPI_DSCLK_CSPW_SPBR, DSCLK,
		      QSPI_FLASH_DEFAULT_PCS_SCK_DELAY);

	/* Clock phase and polarity */
	SET_REG_FIELD(WSPI_CPOL_CPHA, CPHA, QSPI_CLOCK_PHASE(fc->op_mode));
	SET_REG_FIELD(WSPI_CPOL_CPHA, CPOL, QSPI_CLOCK_POL(fc->op_mode));

	/* Configure polling timeout */
	SET_REG_FIELD(WSPI_POLL_TIMEOUT, NUMBER_OF_POLLING,
		      WSPI_POLL_TIMEOUT_COUNT);
	SET_REG_FIELD(WSPI_POLL_TIMEOUT, DELAY_BETWEEN_POLL, WSPI_POLL_DELAY);

	/* Clear halt acknowledge bit */
	SET_REG_FIELD(WSPI_HALTA, HALTA, 0);

	/* Check if WSPI is busy */
	/* This condition needs to become a GET_FIELD after hardware .regs
	 * file is updated with bit fields for the WSPI_STATUS register
	 */
	if ((sys_read32(WSPI_STATUS_ADDR) & 0x1) == 0) { /* Idle bit is bit0 */
		/* Reset WSPI by setting HALT and WSPIF bit*/
		sys_write32(MASK(WSPI_HALT_WSPIF, HALT) |
			    MASK(WSPI_HALT_WSPIF, WSPIF),
			    WSPI_HALT_WSPIF_ADDR);
		/* Wait for halt acknowledge */
		while (GET_REG_FIELD(WSPI_HALTA, HALTA) != 1)
			;
		SET_REG_FIELD(WSPI_HALTA, HALTA, 0);
	}

	/* Clear the halt and wspif bits */
	SET_REG_FIELD(WSPI_HALT_WSPIF, HALT, 0);
	SET_REG_FIELD(WSPI_HALT_WSPIF, WSPIF, 0);
}

#define mspi_tx_bytes(DD, TXBUF, TXLEN, END)    \
	mspi_exchange_bytes(DD, TXBUF, NULL, TXLEN, END)

#define mspi_rx_bytes(DD, RXBUF, RXLEN, END)    \
	mspi_exchange_bytes(DD, NULL, RXBUF, RXLEN, END)

static int mspi_exchange_bytes(
		struct qspi_driver_data *dd,
		u8_t *tx,
		u8_t *rx,
		u32_t length,
		bool end)
{
	int i;
	u8_t chunk;
	u32_t cdram;
	u32_t remaining;

	if (length > MAX_BYTES_PER_MSPI_TRANSACTION)
		return -EINVAL;

	/* Set PCS
	 * Disable DT/DSCK
	 * 8 bits per serial transfer
	 * Single lane mode
	 * Continue to assert slave select line */
	cdram = dd->cdram_mask | CDRAM_MSPI_CDRAM00_CONT_MSK;

	remaining = length;
	while (remaining) {
		/* Max of 16 cdram operations allowed at once */
		chunk = min(16, remaining);

		/* Setup cdram and txram registers */
		for (i = 0; i < chunk; i++) {
			u32_t txram_addr = MSPI_TXRAM00_ADDR +
					   i*(MSPI_TXRAM02_ADDR -
					      MSPI_TXRAM00_ADDR);
			u32_t cdram_addr = MSPI_CDRAM00_ADDR +
					   i*(MSPI_CDRAM01_ADDR -
					      MSPI_CDRAM00_ADDR);
			sys_write32(tx ? tx[i] : 0xFF, txram_addr);
			sys_write32(cdram, cdram_addr);
		}

		/* If this is the last chunk then clear
		 * the cont bit for the last write
		 */
		if (end && (remaining == chunk)) {
			u32_t cdram_addr = MSPI_CDRAM00_ADDR +
					   (chunk-1)*(MSPI_CDRAM01_ADDR -
						      MSPI_CDRAM00_ADDR);
			sys_write32((cdram & (~CDRAM_MSPI_CDRAM00_CONT_MSK)),
				    cdram_addr);
		}

		/* Clear the status */
		sys_write32(0, MSPI_MSPI_STATUS_ADDR);

		/* Setup for Transfer */
		sys_write32(0, MSPI_NEWQP_ADDR);
		sys_write32(chunk - 1, MSPI_ENDQP_ADDR);

		/* Start the transfer - Set spe | cont */
		sys_write32(MASK(MSPI_SPCR2, CONT_AFTER_CMD) |
			    MASK(MSPI_SPCR2, SPE),
			    MSPI_SPCR2_ADDR);

		/* Wait for SPIF - Need to poll, because mspi_SPCR2.spifie
		 * (SPI finished interrupt enable), according to the .regs
		 * description, is not used in RTL anymore. So ignore this field
		 */
		while (GET_REG_FIELD(MSPI_MSPI_STATUS, SPIF) == 0)
			;

		/* Read bytes out of rxram */
		if (rx) {
			for (i = 0; i < chunk; i++) {
				u32_t rxram_addr = MSPI_RXRAM01_ADDR +
						   i*(MSPI_RXRAM02_ADDR -
						      MSPI_RXRAM00_ADDR);
				rx[i] = sys_read32(rxram_addr);
			}
		}

		/* Update remaining bytes and tx,rx pointers */
		remaining -= chunk;
		if (tx)
			tx += chunk;
		if (rx)
			rx += chunk;
	}

	return 0;
}

static int qspi_flash_enumerate(struct qspi_driver_data *dd,
	qspi_serial_flash_type_e *flash_type)
{
	int ret;
	u8_t command;
	u8_t response[3];

	/* Initialize mspi (with default config), as this is needed for
	 * enumeration. Only perform common mspi init here, device specific
	 * initialization is done after enumeration
	 */
	mspi_common_init(dd);

	/* Enumerate by reading the MFG ID using the JEDEC command
	 * Most flashes support this. If there is a flash device
	 * that does not support this command, then the enumeration
	 * procedure needs to be updated accordingly
	 */
	command = QSPI_FLASH_JEDEC_READ_MFG_ID_COMMAND;
	ret = mspi_transfer(dd, &command, sizeof(command),
			    response, sizeof(response));
	if (ret)
		return ret;

	switch (response[0]) {
	case QSPI_FLASH_SPANSION_MFG_ID:
		*flash_type = FLASH_SPANSION;
		break;
	case QSPI_FLASH_MACRONIX_MFG_ID:
		*flash_type = FLASH_MACRONIX;
		break;
	case QSPI_FLASH_MICRON_MFG_ID:
		switch (response[1]) {
		case MICRON_DEVICE_ID:
			*flash_type = FLASH_MICRON;
			break;
		case XMC_DEVICE_ID:
			*flash_type = FLASH_XMC;
			break;
		default:
			return -EOPNOTSUPP;
		}
		break;
	case QSPI_FLASH_GIGA_DEVICE_MFG_ID:
		*flash_type = FLASH_GIGA_DEVICE;
		break;
	case QSPI_FLASH_WINBOND_MFG_ID:
		*flash_type = FLASH_WINBOND;
		break;
	default:
		return -EOPNOTSUPP;
	}

	dd->info.name = (char *)flash_device_names[*flash_type];

	return 0;
}

static int qspi_select_slave(struct qspi_driver_data *dd)
{
	int ret;

	/* Do not configure Quad lane mode if it is not enabled */
#ifndef CONFIG_QSPI_QUAD_IO_ENABLED
	if (QSPI_DATA_LANE(dd->flash_config.op_mode) == QSPI_MODE_QUAD)
		return -EOPNOTSUPP;
#endif

	/**************************************************************/
	/** BSPI Initialization (Common, device-independent settings)**/
	/**************************************************************/
	/* Set 4th address byte to 0 */
	SET_REG_FIELD(BSPI_REGISTERS_BSPI_FLASH_UPPER_ADDR_BYTE,
				  BSPI_FLASH_UPPER_ADDR, 0);

	/* Flush pre-fetch buffers */
	bspi_flush_prefetch_buffers();

	/* RAF specific initialization? */
	SET_REG_FIELD(RAF_CTRL, CLEAR, 1);

	/* MSPI initialization - only device independent settings */
	mspi_common_init(dd);

	/* WSPI initialization - only device independent settings */
	wspi_common_init(dd);

	/* Recommended setting for accessing secondary flash, even if
	 * offset within the flash is under 16MB
	 */
	if (dd->flash_config.slave_num) {
		SET_REG_FIELD(BSPI_REGISTERS_STRAP_OVERRIDE_CTRL,
			      ADDR_4BYTE_N_3BYTE, 1);
		SET_REG_FIELD(BSPI_REGISTERS_STRAP_OVERRIDE_CTRL, OVERRIDE, 1);
	}

	/* Now do the device specific initialization */
	if (dd->interface != NULL) {
		ret = dd->interface->init(dd, &dd->flash_config);
		if (ret)
			return ret;
	} else {
		SYS_LOG_ERR("Flash device interface unavailable for flash: #%d",
			    dd->flash_config.slave_num);
		return -EOPNOTSUPP;
	}

	return 0;
}

#ifdef CONFIG_QSPI_DMA_SUPPORT
#define MAX_DMA_SIZE_SUPPORTED		(4095)
/* Max DMA size that is aligned to page size  (256) */
#define MAX_DMA_SIZE_PAGE_ALIGNED	(MAX_DMA_SIZE_SUPPORTED & ~0xFF)
static int dma_transfer_data(u8_t *dst_addr, u8_t *src_addr, u32_t dma_size)
{
	int ret;
	u32_t remaining, size;

	struct dma_config dma_cfg;
	struct dma_block_config dma_block_cfg0;

	/* Configure DMA block */
	dma_block_cfg0.source_address = (u32_t)src_addr;
	dma_block_cfg0.dest_address = (u32_t)dst_addr;
	dma_block_cfg0.next_block = NULL;

	/* Setup the DMA config */
	dma_cfg.head_block = &dma_block_cfg0;

	remaining = dma_size;
	do {
		/* Compute DMA size */
		size = (remaining >= MAX_DMA_SIZE_PAGE_ALIGNED) ?
			MAX_DMA_SIZE_PAGE_ALIGNED : remaining;
		dma_block_cfg0.block_size = size;

		/* Kick off the DMA */
		ret = dma_memcpy(dma_dev, dma_chan_id, &dma_cfg);
		if (ret)
			break;

		/* Update remaining bytes */
		remaining -= size;

		/* Update source and dest addresses */
		dma_block_cfg0.source_address += size;
		dma_block_cfg0.dest_address += size;
	} while (remaining);

	return ret;
}
#endif /* CONFIG_QSPI_DMA_SUPPORT */

int flash_memcpy(u8_t *dst_addr, u8_t *src_addr, u32_t size)
{
	u32_t i;

#ifdef CONFIG_QSPI_DMA_SUPPORT
	int ret;

	if (dma_chan_id != DMA_CHANNEL_INVALID) {
		ret = dma_transfer_data(dst_addr, src_addr, size);
		return ret;
	}

	/* In case DMA channel is invalid, fall through to doing a memcpy */
#endif
	for (i = 0; i < size; i++)
		dst_addr[i] = src_addr[i];

	return 0;
}

#ifdef CONFIG_USE_WSPI_FOR_WRITE
#ifdef CONFIG_USE_CPU_MEMCPY_FOR_WRITE
/*
 * The DMA controller PL080 has a fifo of size 4. This, along with the src/dst
 * alignment, limits the size of the AXI busrt that can be generated on the bus.
 * For example if the src is aligned to a byte, then a max of 4-byte AXI burst
 * will be generated. This becomes very inefficient for writing to the QSPI
 * flash. The reason for this is that the WSPI module which translates the
 * memory mapped writes to flash, by converting AXI writes to SPI transactions,
 * works on an AXI transaction boundary. And for the afore mentioned example,
 * this will results in the following SPI transactions for every 4 bytes. This
 * constitutes one write transaction on the SPI bus.
 *
 * - Write enable Latch (WEL)
 * - Read Status and check for WEL = 1
 * - Write or Page program command with 24/32 bit address for n (4) bytes
 * - Read Status and repeat this step until WIP = 0
 *
 * So if the transfer size were to be 10 KB, there would be approx. 2500 SPI
 * write transacactions for the write to complete. The AXI busrt size can be a
 * max of 64 bytes. Thus by increasing the AXI burst size, the number of SPI
 * write transaction for a given size becomes smaller. And the overhead for
 * setting the WEL bit and sending the address bits as part of the write command
 * is reduced.
 *
 * This routine attempts to generate upto 40 byte AXI bursts to make the write
 * more efficient, by making use of the ldmia/stmia instructions. But note that
 * these instructions only work on 4 byte aligned addresses/size.Since there is
 * no restriction on the alignments for source, destination and size, this
 * function will have to address all combinations of these alignments.
 *
 * Implementation:
 *	This routine works by splitting the write into upto 3 transactions
 *
 *	1. If the dest address is not aligned to a 4 byte boundary, then this
 *	   step will transfer n (0-3) bytes to the destination, so that the next
 *	   destination location to write to is 4 byte aligned.
 *
 *	2. This step writes the remaining data to the flash in multiples of 4.
 *
 *	3. This step writes the left over bytes (if any) from step 2. This step
 *	   will have upto maximum of 3 bytes to write to the flash.
 *
 * If the addresses and the size are 4 byte aligned, then steps 1 and 3 don't
 * do anything and step 2 takes care of the complete transfer
 */
static int cpu_memcpy(u8_t *dst_addr, u8_t *src_addr, u32_t dma_size)
{
	u8_t *d, *s;
	u32_t remaining, size, i;

	remaining = dma_size;
	d = dst_addr;
	s = src_addr;

	/* Step 1 */
	if ((u32_t)d & 0x3) {
		size = 4 - ((u32_t)d & 0x3);
		for (i = 0; i < size; i++)
			*d++ = *s++;
		remaining -= size;
	}

	/* Step 2: At this point d and size are 4 byte aligned */
	size = remaining & ~0x3;
	if (size)  {
		qspi_memcpy_src_unaligned(d, s, size);
		d += size;
		s += size;
		remaining -= size;
	}

	/* Step 3 */
	if (remaining) {
		for (i = 0; i < remaining; i++)
			*d++ = *s++;
	}

	return 0;
}
#endif /* CONFIG_USE_CPU_MEMCPY_FOR_WRITE */

static int wspi_write_data_chunk(
		struct qspi_driver_data *dd,
		u32_t address,
		u32_t size,
		u8_t *data,
		bool four_byte_enable)
{
	int ret = 0;
	u8_t command;
	u8_t *src, *dst;
	u32_t memmap_addr, num_addr_bits;
#ifndef CONFIG_USE_CPU_MEMCPY_FOR_WRITE
	u32_t dma_size, remaining;
#endif
	qspi_flash_command_info_s *p_cmd = &dd->command_info;
	struct qspi_flash_config *fc = &dd->flash_config;

	/* WSPI module generates a transaction on the bus for
	 * every AXI transaction. The max data size for an AXI transaction
	 * is 16 DWORDs or 64 bytes. To achieve max WSPI efficiency, the
	 * write api needs to generate bursts of 64 byte writes to the
	 * memory mapped region. This is done using the dma driver apis
	 */

	/* Enter 4 byte mode */
	if (four_byte_enable) {
		dd->interface->enable_4byte_addr(
				dd,
				QSPI_DATA_LANE(fc->op_mode),
				address);
		num_addr_bits = dd->command_info.num_addr_bits;
	} else {
		num_addr_bits = 24;
	}

	/* Truncate address bits based on the flash device capacity */
	memmap_addr = (address & GET_MASK(num_addr_bits))
		      + WSPI_MEMORY_MAPPED_REGION_START(fc->slave_num);

	/* Write enable is called by the WSPI controller */

	/* Carry out one DMA per 64 bytes */
	src = data;
	dst = (u8_t *)memmap_addr;

#ifdef CONFIG_USE_CPU_MEMCPY_FOR_WRITE
	ret = cpu_memcpy(dst, src, size);
#else
	remaining = size;

	/* Determine DMA size for first chunk */
	/* Not 64 byte aligned - to handle first pp case */
	if (memmap_addr & (DMA_BURST_SIZE_FOR_WSPI - 1))
		dma_size = DMA_BURST_SIZE_FOR_WSPI
			   - (memmap_addr & (DMA_BURST_SIZE_FOR_WSPI - 1));
	else
		dma_size = DMA_BURST_SIZE_FOR_WSPI;
	dma_size = min(dma_size, remaining);

	while (remaining) {
		/* Clear WSPI done status bits - Writing a 1 clears the bits */
		SET_REG_FIELD(WSPI_INTERRUPT_WSPI_DONE, WSPI_DONE, 1);
		SET_REG_FIELD(WSPI_INTERRUPT_WSPI_DONE, WSPI_ERROR, 1);

		/* DMA write to WSPI mapped memory */
		ret = flash_memcpy(dst, src, dma_size);
		if (ret)
			break;

		/* Wait for WSPI status bit/interrupt
		 * Need to poll instead of using a delay, interrupts are
		 * disabled while executing the qspi apis and kernel sys delay
		 * apis cannot be invoked as they would re-enable the interrupts
		 */
		while (GET_REG_FIELD(WSPI_INTERRUPT_WSPI_DONE, WSPI_DONE) == 0)
			;

		/* Check if error status is set */
		if (GET_REG_FIELD(WSPI_INTERRUPT_WSPI_DONE, WSPI_ERROR) != 0) {
			ret = -EINPROGRESS;
			break;
		}

		/* Update pointers and remaining bytes */
		dst += dma_size;
		src += dma_size;
		remaining -= dma_size;

		/* Next DMA size */
		dma_size = min(DMA_BURST_SIZE_FOR_WSPI, remaining);
	}
#endif /* CONFIG_USE_CPU_MEMCPY_FOR_WRITE */

	/* Write Disable */
	command = p_cmd->write_disable;
	mspi_transfer(dd, &command, 1, NULL, 0);

	/* Exit 4 byte mode */
	if (four_byte_enable)
		dd->interface->disable_4byte_addr(dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode));

	return ret;
}
#endif /* CONFIG_USE_WSPI_FOR_WRITE */

/* This function returns the number of (CONFIG_MIN_SMAU_WINDOW_SIZE) aligned
 * memory chunks the given region spans It also returns the address and size for
 * each of the chunks. Assumes len <= CONFIG_MIN_SMAU_WINDOW_SIZE
 */
static int qspi_get_mem_chunks(
		u32_t address,
		u32_t len,
		u32_t *addresses,
		u32_t *sizes)
{
	u32_t num_chunks;
	u32_t addr_mask;

	/* 2's complement of the size gives the address mask
	 * Ex: Address mask for 16 MB 0x1000000 is (0xFEFFFFFF + 1) = 0xFF000000
	 */
	addr_mask = ~CONFIG_MIN_SMAU_WINDOW_SIZE + 1;

	/* Compare MSByte of the start and end addresses */
	if ((address & addr_mask) != ((address + len - 1) & addr_mask)) {
		num_chunks = 2;
		addresses[0] = address;
		sizes[0] = CONFIG_MIN_SMAU_WINDOW_SIZE -
			   (address & (CONFIG_MIN_SMAU_WINDOW_SIZE-1));
		addresses[1] = address + sizes[0];
		sizes[1] = len - sizes[0];
	} else {
		num_chunks = 1;
		addresses[0] = address;
		sizes[0] = len;
	}

	return num_chunks;
}

#ifdef CONFIG_USE_WSPI_FOR_WRITE
static int wspi_write_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data)
{
	int ret = 0;
	int num_chunks, i;

	u32_t sizes[2];
	u32_t addresses[2];

	/* Check if address crosses 16M boundary */
	num_chunks = qspi_get_mem_chunks(addr, len, addresses, sizes);

	for (i = 0; i < num_chunks; i++) {
		bool four_byte_enable = (addresses[i] >> 24 != 0);

		ret = wspi_write_data_chunk(dd, addresses[i], sizes[i],
					    data, four_byte_enable);
		if (ret)
			break;
		data += sizes[i];
	}

	return ret;
}
#endif /* CONFIG_USE_WSPI_FOR_WRITE */

#ifndef CONFIG_USE_BSPI_FOR_READ
static int mspi_read_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data)
{
	int ret = 0;
	u32_t i, key = 0;

	bool four_byte_mode = false;
	u8_t command[5];	/* 1 byte for command and 4 bytes for address */
	u8_t data_byte, num_bytes;
	qspi_flash_command_info_s *p_cmd = &dd->command_info;

	/* Check if we need to enter 4 byte mode */
	if ((addr + len) > SIZE_16MB)
		four_byte_mode = true;

	/* Enter 4 byte mode */
	if (four_byte_mode) {
		key = irq_lock();
		dd->interface->enable_4byte_addr(
				dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode),
				addr);
	}

	for (i = 0; i < len; i++) {
		/* Prepare command buffer */
		num_bytes = 0;
		command[num_bytes++] = p_cmd->read;
		if (four_byte_mode && (p_cmd->num_addr_bits == 32))
			command[num_bytes++] = (u8_t)((addr+i) >> 24);
		command[num_bytes++] = (u8_t)((addr+i) >> 16);
		command[num_bytes++] = (u8_t)((addr+i) >> 8);
		command[num_bytes++] = (u8_t)(addr+i);

		ret = mspi_transfer(dd, command, num_bytes, &data_byte, 1);
		if (ret)
			break;
		/* Read data byte into buffer */
		data[i] = data_byte;
	}

	/* Exit 4 byte mode */
	if (four_byte_mode) {
		dd->interface->disable_4byte_addr(dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode));
		irq_unlock(key);
	}

	return ret;
}
#endif /* !CONFIG_USE_BSPI_FOR_READ */

#ifdef CONFIG_USE_BSPI_FOR_READ
static int bspi_read_data_chunk(
		struct qspi_driver_data *dd,
		u32_t address,
		u32_t size,
		u8_t *data,
		bool four_byte_enable)
{
	int ret = 0;
	u8_t *memmap_addr;
	u32_t data_len, key = 0, num_addr_bits, phys_addr;
	struct qspi_flash_config *fc = &dd->flash_config;

	/* Enter 4 byte mode */
	if (four_byte_enable) {
		key = irq_lock();
		dd->interface->enable_4byte_addr(
				dd,
				QSPI_DATA_LANE(fc->op_mode),
				address);
		num_addr_bits = dd->command_info.num_addr_bits;
	} else {
		num_addr_bits = 24;
	}

	phys_addr = ((address & GET_MASK(num_addr_bits))
			+ BSPI_MEMORY_MAPPED_REGION_START(fc->slave_num));

	/* Get the SMAU virtual address if SMAU path is enabled */
	if (GET_REG_FIELD(BSPI_REGISTERS_BSPI_CS_OVERRIDE, BSPI_DIRECT_SEL))
		memmap_addr = (u8_t *)phys_addr;
	else
		memmap_addr = smau_get_virtual_from_physical(phys_addr);

	/* Flush pre-fetch buffers */
	bspi_flush_prefetch_buffers();

	data_len = size;

	/* Read data from memory mapped region */
#ifdef CONFIG_USE_RAF_TO_READ_DATA
	/* Clear the fifo */
	SET_REG_FIELD(RAF_CTRL, CLEAR, 1);
	/* Wait for hw to recognize the clear command */
	while (GET_REG_FIELD(RAF_CTRL, CLEAR) == 0x1)
		;

	/* Configure the start address and size */
	sys_write32((u32_t)memmap_addr, RAF_START_ADDR);
	sys_write32(size, RAF_NUM_WORDS_ADDR);

	/* Kick start the reads */
	SET_REG_FIELD(RAF_CTRL, START, 1);

	data_len = size;
	while (data_len) {
		if (GET_REG_FIELD(RAF_STATUS, FIFO_EMPTY) != 0) {
			u32_t i;

			/* Fifo has some data, so read the first word */
			u32_t word = sys_read32(RAF_READ_DATA_ADDR);

			/* Copy word to data buffer */
			for (i = 0; i < min(4, data_len); i++)
				*data++ = GET_BYTE(word, i);

			/* Update remaining bytes */
			data_len -= min(4, data_len);
		} else if (GET_REG_FIELD(RAF_STATUS, SESSION_BUSY) == 0) {
			/* Read complete */
			if (data_len)
				ret = -EIO;
			break;
		} else {
			/* Wait till more data is read into the FIFO */
			continue;
		}
	}
#else
	flash_memcpy(data, memmap_addr, data_len);
#endif

	/* Wait for BSPI to clear busy status flag */
	while (GET_REG_FIELD(BSPI_REGISTERS_BUSY_STATUS, BUSY) != 0)
		;

	/* Exit 4 byte mode */
	if (four_byte_enable) {
		dd->interface->disable_4byte_addr(dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode));
		irq_unlock(key);
	}

	return ret;
}

static int bspi_read_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data)
{
	int ret = 0;
	int num_chunks, i;

	u32_t sizes[2];
	u32_t addresses[2];

	/* Check if address crosses 16M boundary */
	num_chunks = qspi_get_mem_chunks(addr, len, addresses, sizes);

	for (i = 0; i < num_chunks; i++) {
		bool four_byte_enable = (addresses[i] >> 24 != 0);

		ret = bspi_read_data_chunk(dd, addresses[i], sizes[i],
					   data, four_byte_enable);
		if (ret)
			break;

		data += sizes[i];
	}

	return ret;
}
#endif /* CONFIG_USE_BSPI_FOR_READ */

static u8_t command[MAX_BYTES_PER_MSPI_TRANSACTION];

static int mspi_write_data(
		struct qspi_driver_data *dd,
		u32_t addr,
		u32_t len,
		u8_t *data)
{
	int ret = 0;
	u32_t i, key = 0, write_size, remaining, offset, num_bytes;

	u8_t response;
	bool four_byte_mode = false;
	qspi_flash_command_info_s *p_cmd = &dd->command_info;

	/* Check if we need to enter 4 byte mode */
	if ((addr + len) > SIZE_16MB)
		four_byte_mode = true;

	/* Enter 4 byte mode */
	if (four_byte_mode) {
		key = irq_lock();
		dd->interface->enable_4byte_addr(
				dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode),
				addr);
	}

	/* Write bytes upto the next 256 byte aligned address */
	remaining = len;
	write_size = FLASH_PAGE_SIZE - (addr & (FLASH_PAGE_SIZE - 1));
	write_size = min(remaining, write_size);
	offset = 0;
	do {
		/* Write enable
		 * Need WEL for every transaction because some flash
		 * devices reset WEL latch after the write is complete
		 */
		command[0] = p_cmd->write_enable;
		ret = mspi_transfer(dd, command, 1, NULL, 0);
		if (ret)
			break;

		/* Prepare command buffer */
		num_bytes = 0;
		command[num_bytes++] = p_cmd->write;
		if (four_byte_mode && (p_cmd->num_addr_bits == 32))
			command[num_bytes++] = (u8_t)((addr+offset) >> 24);
		command[num_bytes++] = (u8_t)((addr+offset) >> 16);
		command[num_bytes++] = (u8_t)((addr+offset) >> 8);
		command[num_bytes++] = (u8_t)(addr+offset);

		/* Write data byte into buffer */
		for (i = 0; i < write_size; i++)
			command[num_bytes++] = data[offset+i];

		ret = mspi_transfer(dd, command, num_bytes, NULL, 0);
		if (ret)
			break;

		/* Wait for WIP bit to clear */
		do {
			command[0] = p_cmd->read_status;
			ret = mspi_transfer(dd, command, 1, &response, 1);
			if (ret)
				break;
		} while (response & BIT(p_cmd->busy_bit_pos));

		if (ret)
			break;

		remaining -= write_size;
		offset +=  write_size;
		write_size = min(remaining, FLASH_PAGE_SIZE);
	} while (remaining);

	/* Write Disable */
	command[0] = p_cmd->write_disable;
	mspi_transfer(dd, command, 1, NULL, 0);

	/* Exit 4 byte mode */
	if (four_byte_mode) {
		dd->interface->disable_4byte_addr(dd,
				QSPI_DATA_LANE(dd->flash_config.op_mode));
		irq_unlock(key);
	}

	return ret;
}

/* Function to return the minimum erase sector size that the
 * flash device supports
 */
static u32_t qspi_get_min_erase_sector_size(struct qspi_driver_data *dd)
{
	int ret;
	u32_t i;
	u8_t count;
	u32_t min_erase_size = 0xFFFFFFFF;
	qspi_erase_command_info_s *erase_cmds;

	if (dd == NULL)
		return -EINVAL;

	ret = dd->interface->get_erase_commands(&erase_cmds, &count);
	if (ret)
		return ret;

	for (i = 0; i < count; i++) {
		if (erase_cmds[i].erase_size < min_erase_size)
			min_erase_size = erase_cmds[i].erase_size;
	}

	return min_erase_size;
}

/* Function to return the command to erase the specified size
 * For example if the size specified is 12 KB and the flash device supports
 * erasing 4 KB, 8 KB and 16 KB sectors, then this function returns the
 * command for erasing a 4KB sector and sets size = 4*1024. Note that size is
 * an inout parameter for this function
 */
static int qspi_get_erase_command(
		struct qspi_driver_data *dd,
		u32_t address,
		u8_t *command,
		u32_t *size)
{
	int max_idx, min_idx;
	u8_t count;
	int ret;
	qspi_erase_command_info_s *erase_cmds;

	if ((command == NULL) || (size == NULL))
		return -EINVAL;

	ret = dd->interface->get_erase_commands(&erase_cmds, &count);
	if (ret)
		return ret;

	/* Get the max erase size that aligns with address */
	for (max_idx = 0; max_idx < count; max_idx++) {
		if ((address % erase_cmds[max_idx].erase_size) != 0)
			break;
	}
	if (max_idx == 0) /* No erase sizes align with address */
		return -EINVAL;
	/* max_idx now has the index to the erase size that can be used
	 * for the address given
	 */
	max_idx--;

	/* Get the max erase size that aligns with size */
	for (min_idx = 0; min_idx < count; min_idx++) {
		if ((*size % erase_cmds[min_idx].erase_size) != 0)
			break;
	}
	if (min_idx == 0) /* No erase sizes align with size */
		return -EINVAL;
	/* min_idx now has the index to the erase size that can be used
	 * for the size given
	 */
	min_idx--;

	/* Here we assume that the erase sizes are such that every
	 * erase size is an exact multiple of the previous erase size */
	max_idx = min(max_idx, min_idx);

	*command = erase_cmds[max_idx].erase_command;
	*size = erase_cmds[max_idx].erase_size;

	return 0;
}

/* Common functions used by device specific flash driver modules */
void bspi_configure_lane_mode(qspi_flash_lane_mode_e lane_mode)
{
	/* Enable flex mode */
	SET_REG_FIELD(BSPI_REGISTERS_FLEX_MODE_ENABLE,
		      BSPI_FLEX_MODE_ENABLE, 1);

	switch (lane_mode) {
	default:
	case QSPI_LANE_MODE_1_1_1:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		break;
	case QSPI_LANE_MODE_1_1_2:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_DUAL);
		break;
	case QSPI_LANE_MODE_1_2_2:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_DUAL);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_DUAL);
		break;
	case QSPI_LANE_MODE_2_2_2:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_DUAL);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_DUAL);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_DUAL);
		break;
	case QSPI_LANE_MODE_1_1_4:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_QUAD);
		break;
	case QSPI_LANE_MODE_1_4_4:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_SINGLE);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_QUAD);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_QUAD);
		break;
	case QSPI_LANE_MODE_4_4_4:
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, CMD_BPC_SELECT,
			      QSPI_MODE_QUAD);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, ADDR_BPC_SELECT,
			      QSPI_MODE_QUAD);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, DATA_BPC_SELECT,
			      QSPI_MODE_QUAD);
		break;
	}
}

void bspi_set_dummy_cycles(u8_t dummy_cycles)
{
	/* 3rd arg is the bit field name and 4th is the parameter to function */
	SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_PHASE, DUMMY_CYCLES,
		      dummy_cycles);
}

void bspi_set_mode_byte(u8_t *mode, u8_t qspi_mode)
{
	if (mode != NULL) {
		/* Mode bits per cycle is determined by qspi_mode */
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_CYCLE, MODE_BPC_SELECT,
			      qspi_mode);
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_PHASE, MODE_BPP, 1);
		SET_REG_FIELD(BSPI_REGISTERS_CMD_AND_MODE_BYTE, BSPI_MODE_BYTE,
			      *mode);
	} else {
		/* No mode byte needed, just set bpp = 0 */
		SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_PHASE, MODE_BPP, 0);
	}
}

void bspi_set_command_byte(u8_t command)
{
	SET_REG_FIELD(BSPI_REGISTERS_CMD_AND_MODE_BYTE, BSPI_CMD_BYTE, command);
}

void bspi_set_4_byte_addr_mode(bool enable, u8_t byte_4)
{
	SET_REG_FIELD(BSPI_REGISTERS_BITS_PER_PHASE, ADDR_BPP_SELECT,
		      enable ? 1 : 0);
	if (enable) {
		SET_REG_FIELD(BSPI_REGISTERS_BSPI_FLASH_UPPER_ADDR_BYTE,
					  BSPI_FLASH_UPPER_ADDR, byte_4);
	}
}

int mspi_transfer(
	struct qspi_driver_data *dd,
	u8_t *tx,
	u32_t tx_len,
	u8_t *rx,
	u32_t rx_len)
{
	int ret;

	/* Check params */
	if ((tx == NULL) || (tx_len == 0))
		return -EINVAL;

	/* Acquire the bus for MSPI transfer */
	mspi_acquire_bus();

	/* Set the write lock bit */
	SET_REG_FIELD(MSPI_WRITE_LOCK, WRITELOCK, 1);

	/* Send command and optionally address + data */
	ret = mspi_tx_bytes(dd, tx, tx_len, rx_len == 0);
	if (ret)
		goto done;

	if ((rx != NULL) && (rx_len != 0)) {
		/* Read data */
		ret = mspi_rx_bytes(dd, rx, rx_len, true);
		if (ret)
			goto done;
	}

done:
	/* Clear the write lock bit */
	SET_REG_FIELD(MSPI_WRITE_LOCK, WRITELOCK, 0);

	/* Release the bus to BSPI */
	mspi_release_bus();
	return ret;
}

void wspi_set_wren_bit_info(u8_t bit_pos, bool invert_polarity)
{
	SET_REG_FIELD(WSPI_CONFIG, WRITE_ENABLE_SEL, bit_pos);
	SET_REG_FIELD(WSPI_CONFIG, WREN_POL, invert_polarity ? 1 : 0);
}

void wspi_set_wip_bit_info(u8_t bit_pos, bool invert_polarity)
{
	SET_REG_FIELD(WSPI_CONFIG, WRITE_IN_PRO_SEL, bit_pos);
	SET_REG_FIELD(WSPI_CONFIG, WIP_POL, invert_polarity ? 1 : 0);
}

void wspi_configure_lane_mode(qspi_flash_lane_mode_e lane_mode)
{
	switch (lane_mode) {
	case QSPI_LANE_MODE_1_1_1:
		SET_REG_FIELD(WSPI_CONFIG, QUAD, 0);
		SET_REG_FIELD(WSPI_CONFIG, DUAL, 0);
		SET_REG_FIELD(WSPI_CONFIG, AME, 0);
		break;
	case QSPI_LANE_MODE_1_1_2:
		SET_REG_FIELD(WSPI_CONFIG, QUAD, 0);
		SET_REG_FIELD(WSPI_CONFIG, DUAL, 1);
		SET_REG_FIELD(WSPI_CONFIG, AME, 0);
		break;
	case QSPI_LANE_MODE_1_2_2:
		SET_REG_FIELD(WSPI_CONFIG, QUAD, 0);
		SET_REG_FIELD(WSPI_CONFIG, DUAL, 1);
		SET_REG_FIELD(WSPI_CONFIG, AME, 1);
		break;
	case QSPI_LANE_MODE_1_1_4:
		SET_REG_FIELD(WSPI_CONFIG, QUAD, 1);
		SET_REG_FIELD(WSPI_CONFIG, DUAL, 0);
		SET_REG_FIELD(WSPI_CONFIG, AME, 0);
		break;
	case QSPI_LANE_MODE_1_4_4:
		SET_REG_FIELD(WSPI_CONFIG, QUAD, 1);
		SET_REG_FIELD(WSPI_CONFIG, DUAL, 0);
		SET_REG_FIELD(WSPI_CONFIG, AME, 1);
		break;
	default:
	case QSPI_LANE_MODE_2_2_2:
	case QSPI_LANE_MODE_4_4_4:
		SYS_LOG_WRN("Mode %d not supported in QSPI", lane_mode);
		break;
	}
}

void wspi_set_command(wspi_command_e type, u8_t command)
{
	switch (type) {
	case WSPI_COMMAND_WREN:
		SET_REG_FIELD(WSPI_COMMAND0, WREN, command);
		break;
	case WSPI_COMMAND_RDSR:
		SET_REG_FIELD(WSPI_COMMAND0, RDSR, command);
		break;
	case WSPI_COMMAND_PP:
		SET_REG_FIELD(WSPI_COMMAND0, PP, command);
		break;
	case WSPI_COMMAND_4PP:
		SET_REG_FIELD(WSPI_COMMAND0, X4PP, command);
		break;
	case WSPI_COMMAND_2PP:
		SET_REG_FIELD(WSPI_COMMAND1, X2PP, command);
		break;
	case WSPI_COMMAND_ENTER_4B:
		SET_REG_FIELD(WSPI_COMMAND1, EN4B, command);
		break;
	case WSPI_COMMAND_EXIT_4B:
		SET_REG_FIELD(WSPI_COMMAND1, EX4B, command);
		break;
	}
}

void wspi_set_4_byte_addr_mode(bool enable)
{
	SET_REG_FIELD(WSPI_CONFIG, B4AE, enable ? 1 : 0);
}

void wspi_set_use_4th_byte_from_register(u8_t *byte_4)
{
	if (byte_4 != NULL) {
		SET_REG_FIELD(WSPI_CONFIG, B4RE, 1);
		SET_REG_FIELD(WSPI_FOURTH_ADDR_BYTE, FOURTH_ADDR_BYTE, *byte_4);
	} else {
		SET_REG_FIELD(WSPI_CONFIG, B4RE, 0);
	}
}

int qspi_update_command_info(struct qspi_driver_data *dd,
			     qspi_flash_command_info_s *commands)
{
	if ((dd == NULL) || (commands == NULL))
		return -EINVAL;

	dd->command_info = *commands;

	return 0;
}

/* Flash configurtion for primary flash */
static struct qspi_flash_config primary_flash_cfg = {
	.frequency = CONFIG_PRI_FLASH_CLOCK_FREQUENCY,
	.op_mode = 0x3, /* Clock POL = 1, Clock PHASE = 1, Data lane = Single */
	.slave_num = 0  /* Primary flash */
};

/* Flash api */
static struct qspi_flash_driver_api bcm58202_qspi_flash_api = {
	.configure = bcm58202_qspi_flash_set_config,
	.get_devinfo = bcm58202_qspi_flash_get_dev_info,
	.read = bcm58202_qspi_flash_read,
	.erase = bcm58202_qspi_flash_erase,
	.write = bcm58202_qspi_flash_write,
	.chip_erase = bcm58202_qspi_flash_chip_erase
};

/* bcm58202 QSPI flash controller can support 2 slaves
 * CS0 selects the primary flash and CS1 selects the secondary
 * flash. The below initialization configures the primary flash
 * An additional entry with desired settings should be added
 * to initialize the qspi driver for the secondary flash
 */
DEVICE_AND_API_INIT(qspi_pri, CONFIG_PRIMARY_FLASH_DEV_NAME,
		    bcm58202_qspi_flash_init, NULL,
		    &primary_flash_cfg, PRE_KERNEL_2,
		    CONFIG_QSPI_DRIVER_INIT_PRIORITY, &bcm58202_qspi_flash_api);

#ifdef CONFIG_SECONDARY_FLASH
/* Flash configurtion for secondary flash */
static struct qspi_flash_config secondary_flash_cfg = {
	.frequency = CONFIG_SEC_FLASH_CLOCK_FREQUENCY,
	.op_mode = 0x0, /* Clock POL = 0, Clock PHASE = 0, Data lane = Single */
	.slave_num = 1  /* Secondary flash */
};

/* Note that the priority of the driver for secondary flash should be lower
 * (higher in number) than that of the driver for pin mux. This will ensure
 * the GPIO pins are setup as QSPI pins before the qspi driver tried to
 * enumerate the secondary flash.
 */
DEVICE_AND_API_INIT(qspi_sec, CONFIG_SECONDARY_FLASH_DEV_NAME,
		    bcm58202_qspi_flash_init, NULL,
		    &secondary_flash_cfg, PRE_KERNEL_2,
		    CONFIG_QSPI_DRIVER_INIT_PRIORITY, &bcm58202_qspi_flash_api);
#endif
