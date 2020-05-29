/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term “Broadcom”
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

/* @file disk_access_sdcard.c
 *
 * @brief Disk Access api for SD card
 *
 * This driver provides disk access apis for SD cards that are needed for a
 * file system implementation
 *
 */

#include <string.h>
#include <init.h>
#include <zephyr/types.h>
#include <disk_access.h>
#include <errno.h>
#include <sdio.h>
#include "sd_mem_card_test.h"
#include <logging/sys_log.h>

static u32_t sector_count, sector_size, disk_size;

int configure_sdio_sel(void);

DISK_ACCESS_API_QUALIFIER int disk_access_status(DISK_ACCESS_INIT_STATUS_API_ARGS)
{
	return DISK_STATUS_OK;
}

DISK_ACCESS_API_QUALIFIER int disk_access_init(DISK_ACCESS_INIT_STATUS_API_ARGS)
{
	int ret;
	u64_t ns;

	/* Setup SDIO select straps/MFIO for citadel svk first */
	configure_sdio_sel();

	ret = sd_mem_card_test_init();
	if (ret)
		return ret;

	sector_size = sd_mem_card_get_sector_size();
	ns = sd_mem_card_get_num_blocks();
	/* Limit disk size to U32_MAX */
	if (ns*sector_size >= 0x100000000ULL)
		ns = 0xFFFFFFFF / sector_size;

	sector_count = ns;
	disk_size = ns*sector_size;

	return 0;
}

DISK_ACCESS_API_QUALIFIER int disk_access_read(DISK_ACCESS_API_ARG1 u8_t *buff, u32_t sector, u32_t count)
{
	int ret;

	ret = sd_mem_card_read_blocks(sector, count, buff);

	ret = (ret == count) ? 0 : -EIO;
	if (ret)
		SYS_LOG_ERR("Failed to read %d\n", sector);
	return ret;
}

DISK_ACCESS_API_QUALIFIER int disk_access_write(DISK_ACCESS_API_ARG1 const u8_t *buff, u32_t sector, u32_t count)
{
	int ret;

	ret = sd_mem_card_erase_blocks(sector, count);
	if (ret != count)
		ret = -EIO;
	else
		ret = sd_mem_card_write_blocks(sector, count, (u8_t *)buff);

	ret = (ret == count) ? 0 : -EIO;
	if (ret)
		SYS_LOG_ERR("Failed to write %d\n", sector);
	return ret;
}

DISK_ACCESS_API_QUALIFIER int disk_access_ioctl(DISK_ACCESS_API_ARG1 u8_t cmd, void *buff)
{
	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buff = sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(u32_t *)buff = sector_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(u32_t *)buff  = 1;
		break;
	case DISK_IOCTL_GET_DISK_SIZE:
		*(u32_t *)buff  = disk_size;
		break;
	default:
		return -EINVAL;
	}

	return DISK_STATUS_OK;
}


#ifdef DISK_ACCESS_REGISTER_API_AVAILABLE
static const struct disk_operations sd_disk_ops = {
    .init = disk_access_init,
    .status = disk_access_status,
    .read = disk_access_read,
    .write = disk_access_write,
    .ioctl = disk_access_ioctl,
};

static struct disk_info sd_disk = {
    .name = VOLUME_NAME,
    .ops = &sd_disk_ops,
};

#include <ff.h>

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

int disk_sd_init(struct device *dev)
{
	int res;

	disk_access_register(&sd_disk);

	mp.mnt_point = MOUNT_POINT;
    res = fs_mount(&mp);

    if (res != FR_OK) {
        SYS_LOG_ERR("Error mounting disk.\n");
    }

    return res;
}

SYS_INIT(disk_sd_init, APPLICATION, 99);
#endif /* DISK_ACCESS_REGISTER_API_AVAILABLE */
