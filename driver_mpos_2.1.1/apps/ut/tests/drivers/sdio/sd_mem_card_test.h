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

#ifndef __SD_MEM_CARD_TEST__
#define __SD_MEM_CARD_TEST__

#ifdef __ZEPHYR__
#include <version.h>
#else
#define KERNEL_VERSION_MAJOR	1
#define KERNEL_VERSION_MINOR	9
#endif

#if (KERNEL_VERSION_MAJOR == 1) && (KERNEL_VERSION_MINOR == 9)
	#define FS_DIR_T	fs_dir_t
	#define FS_FILE_T	fs_file_t
	#define DISK_ACCESS_API_ARG1

	#define DISK_ACCESS_INIT_STATUS_API_ARGS	void
	#define DISK_ACCESS_API_QUALIFIER

	#define	ROOT_PATH	"/"
	#ifdef CONFIG_DISK_ACCESS
		#include <disk_access.h>
	#endif
#else /* Zephyr v1.14 */
	#define FS_DIR_T	struct fs_dir_t
	#define FS_FILE_T	struct fs_file_t

	#define DISK_ACCESS_API_ARG1			struct disk_info *di,
	#define DISK_ACCESS_INIT_STATUS_API_ARGS	struct disk_info *di
	#define DISK_ACCESS_API_QUALIFIER		static

	#define DISK_IOCTL_GET_DISK_SIZE		3
	#define DISK_ACCESS_REGISTER_API_AVAILABLE

	/* Zv1.14 added these functions in disk_access.c, but these are
	 * already defined here for sdio test. So to avoid name conflict
	 * these define redirections are added here.
	 */
	#define disk_access_status	disk_sdio_access_status
	#define disk_access_init	disk_sdio_access_init
	#define disk_access_read	disk_sdio_access_read
	#define disk_access_write	disk_sdio_access_write
	#define disk_access_ioctl	disk_sdio_access_ioctl

	#define VOLUME_NAME	"SD"
	#define MOUNT_POINT	"/" VOLUME_NAME ":"
	#define ROOT_PATH	MOUNT_POINT "/"
#endif

#ifdef CONFIG_FILE_SYSTEM
#include <fs.h>
#endif

int sd_mem_card_test_init(void);
int sd_mem_card_read_blocks(u32_t start_block, u32_t num_blocks, u8_t *buf);
int sd_mem_card_erase_blocks(u32_t start_block, u32_t num_blocks);
int sd_mem_card_write_blocks(u32_t start_block, u32_t num_blocks, u8_t *buf);
u32_t sd_mem_card_get_sector_size(void);
u32_t sd_mem_card_get_num_blocks(void);

#endif /* __SD_MEM_CARD_TEST__ */
