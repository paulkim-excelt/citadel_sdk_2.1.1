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

/* @file sotp.h
 *
 *
 * This file contains the secure OTP read/write and utility routines
 *
 * TODO
 * 1. Replace the socregs.h file to the actual registers for upstreaming
 *
 */

#ifndef __SOTP_H__
#define __SOTP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ***************************************************************************
 * Includes
 * ****************************************************************************/

#include <zephyr/types.h>
#include <device.h>
#include <board.h>

/* SOTP register offsets */
#define SOTP_REGS_OTP_PROG_CONTROL             (SOTP_BASE_ADDR + 0x00)
#define SOTP_REGS_OTP_WRDATA_0                 (SOTP_BASE_ADDR + 0x04)
#define SOTP_REGS_OTP_WRDATA_1                 (SOTP_BASE_ADDR + 0x08)
#define SOTP_REGS_OTP_ADDR                     (SOTP_BASE_ADDR + 0x0c)
#define SOTP_REGS_OTP_CTRL_0                   (SOTP_BASE_ADDR + 0x10)
#define SOTP_REGS_OTP_CTRL_1                   (SOTP_BASE_ADDR + 0x14)
#define SOTP_REGS_OTP_STATUS_0                 (SOTP_BASE_ADDR + 0x18)
#define SOTP_REGS_OTP_STATUS_1                 (SOTP_BASE_ADDR + 0x1c)
#define SOTP_REGS_OTP_RDDATA_0                 (SOTP_BASE_ADDR + 0x20)
#define SOTP_REGS_OTP_RDDATA_1                 (SOTP_BASE_ADDR + 0x24)
#define SOTP_REGS_SOTP_CHIP_STATES             (SOTP_BASE_ADDR + 0x28)
#define SOTP_REGS_OTP_ECCCNT                   (SOTP_BASE_ADDR + 0x30)
#define SOTP_REGS_OTP_BAD_ADDR                 (SOTP_BASE_ADDR + 0x34)
#define SOTP_REGS_OTP_WR_LOCK                  (SOTP_BASE_ADDR + 0x38)
#define SOTP_REGS_OTP_RD_LOCK                  (SOTP_BASE_ADDR + 0x3c)
#define SOTP_REGS_ROM_BLOCK_START              (SOTP_BASE_ADDR + 0x40)
#define SOTP_REGS_ROM_BLOCK_END                        (SOTP_BASE_ADDR + 0x44)
#define SOTP_REGS_SMAU_CTRL                    (SOTP_BASE_ADDR + 0x48)
#define SOTP_REGS_CHIP_CTRL                    (SOTP_BASE_ADDR + 0x4c)
#define SOTP_REGS_SR_STATE_0                   (SOTP_BASE_ADDR + 0x50)
#define SOTP_REGS_SR_STATE_1                   (SOTP_BASE_ADDR + 0x54)
#define SOTP_REGS_SR_STATE_2                   (SOTP_BASE_ADDR + 0x58)
#define SOTP_REGS_SR_STATE_3                   (SOTP_BASE_ADDR + 0x5c)
#define SOTP_REGS_SR_STATE_4                   (SOTP_BASE_ADDR + 0x60)
#define SOTP_REGS_SR_STATE_5                   (SOTP_BASE_ADDR + 0x64)
#define SOTP_REGS_SR_STATE_6                   (SOTP_BASE_ADDR + 0x68)
#define SOTP_REGS_SR_STATE_7                   (SOTP_BASE_ADDR + 0x6c)
#define SOTP_REGS_OTP_CLK_CTRL                 (SOTP_BASE_ADDR + 0x70)
#define SOTP_TZMA_ROM_SECURE_REGION_SIZE       (SOTP_BASE_ADDR + 0x74)

#define OTPC_MODE_REG                          (OTP_BASE_ADDR + 0x00)
#define OTPC_CNTRL_0                           (OTP_BASE_ADDR + 0x08)
#define OTPC_CNTRL_1                           (OTP_BASE_ADDR + 0x04)
#define OTPC_CPU_STATUS                                (OTP_BASE_ADDR + 0x0c)
#define OTPC_CPU_DATA                          (OTP_BASE_ADDR + 0x10)
#define OTPC_CPUADDR_REG                       (OTP_BASE_ADDR + 0x28)

/* Register Bit fields */
#define SOTP_REGS_OTP_STATUS_0__FDONE                  7
#define SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN    15
#define SOTP_REGS_OTP_PROG_CONTROL__OTP_DISABLE_ECC    9
#define SOTP_REGS_OTP_ADDR__OTP_ROW_ADDR_R             6
#define SOTP_REGS_OTP_CTRL_0__OTP_CMD_R                        1
#define SOTP_REGS_OTP_CTRL_0__START                    0
#define SOTP_REGS_OTP_STATUS_1__CMD_DONE               1
#define SOTP_REGS_OTP_STATUS_0__FDONE                  7
#define SOTP_REGS_OTP_PROG_CONTROL__OTP_CPU_MODE_EN    15
#define SOTP_REGS_OTP_PROG_CONTROL__OTP_ECC_WREN       8
#define SOTP_REGS_OTP_STATUS_0__PROG_OK                        12
#define SOTP_REGS_OTP_CTRL_0__BURST_START_SEL          24
#define SOTP_REGS_OTP_CTRL_0__WRP_CONTINUE_ON_FAIL     19
#define SOTP_REGS_OTP_CTRL_0__OTP_PROG_EN              21
#define SOTP_REGS_SOTP_CHIP_STATES__MANU_DEBUG         8
#define SOTP_REGS_OTP_CTRL_0__OTP_DEBUG_MODE           20
#define SOTP_REGS_OTP_CTRL_0__WRP_QUADFUSE             13
#define SOTP_REGS_OTP_CTRL_1__WRP_FUSELSEL0            27
#define SOTP_REGS_OTP_CTRL_1__WRP_TM_R                 0
#define SOTP_REGS_OTP_CTRL_1__WRP_VSEL_R               9
#define SOTP_REGS_OTP_CTRL_0__WRP_REGC_SEL_R           14

#define OTPC_CNTRL_0__OTP_PROG_EN                      21
#define OTPC_CNTRL_0__COMMAND_R                                1
#define OTPC_CNTRL_0__START                            0

/* ***************************************************************************
 * MACROS/Defines
 * ****************************************************************************/

/**
 * @brief SOTP states
 */
#define SOTP_READ			0
#define SOTP_PROG_ENABLE	1
#define SOTP_READ_DISABLE	2
#define SOTP_PROG_BIT		10
#define SOTP_PROG_WORD		11

/**
 * @brief SOTP commands
 */
#define SOTP_REGS_sotp_bcm58202_status__CMDDONE	1
#define SOTP_REGS_sotp_bcm58202_status__PROGOK	2
#define SOTP_REGS_sotp_bcm58202_status__FDONE		3

/**
 * @brief Device states
 */
#define SOTP_ECC_ERR_DETECT		0x8000000000000000
#define UNPROGRAMMED			0
#define UNASSIGNED				1
#define NON_AB					2
#define AB_PROD					4
#define AB_DEV					5
#define CHIP_INVALID			6

/**
 * @brief Device states
 */
#define DEVICE_TYPE_UNASSIGNED		0x01	/* Five bit*2 signal   */
#define EDTYPE_unassigned		0x101	/* Nine bit*2 signal   */
#define CHIP_STATE_AB_DEV		0x20
#define DEVICE_TYPE_AB_DEV		0x1b	/* Five bit*2 signal   */
#define EDTYPE_ABdev			0x155	/* Nine bit*2 signal   */
#define DEVICE_TYPE_AB_PROD		0x0b	/* Five bit*2 signal   */
#define EDTYPE_ABprod			0x1b5	/* Nine bit*2 signal   */

/**
 * @brief Key Sizes
 */
#define OTP_SIZE_KAES		32	/* 32 bytes of AES key  */
#define OTP_SIZE_KHMAC_SHA256	32	/* 32 bytes of HMAC-SHA256 key */
#define OTP_SIZE_DAUTH		32	/* 32 bytes of DAUTH key   */
#define OTP_SIZE_KDI_PRIV	32	/* 32 bytes of ECDSA P-256 private key*/
#define OTP_SIZE_KBIND		32	/* 32 bytes of AES-256 binding key */
#define OTP_SIZE_KECDSA_PRIV 32 /* 32 bytes of ECDSA P-256 private key */

/* ***************************************************************************
 * Types/Structure Declarations
 * ****************************************************************************/

/**
 * @brief Define SOTP return status.
 */
typedef enum sotp_bcm58202_status {
	IPROC_OTP_INVALID,	/**< Invalid status */
	IPROC_OTP_VALID,	/**< Valid status */
	IPROC_OTP_ALLONE,	/**< All bits are one */
	IPROC_OTP_ERASED,	/**< Erased */
	IPROC_OTP_FASTAUTH,	/**< Authenticate fast */
	IPROC_OTP_NOFASTAUTH	/**< No fast authentication */
} sotp_bcm58202_status_t;

/**
 * @brief Define OTP Key types.
 */
typedef enum sotp_bcm58202_key_type {
	OTPKey_AES,		/**< Key type AES */
	OTPKey_HMAC_SHA256,	/**< Key type HMAC Sha 256 bit */
	OTPKey_DAUTH,		/**< Key type DAUTH */
	OTPKey_DevIdentity,	/**< Key type device identity */
	OTPKey_ecDSA,		/**< Key type ecDSA */
	OTPKey_Binding		/**< Key type for device binding */
} sotp_bcm58202_key_type_t;

/**
 * @brief Define Device security configurations.
 */
struct sotp_bcm58202_sec_config {
	u16_t	deviceConfig;	/**< Device config */
	u16_t	secureROMSize;	/**< Secure ROM size */
	u8_t	configNonAB;	/**< Device config in Non AB */
	u8_t	configAB;	/**< Device config in AB*/
	u8_t	configDev;	/**< Device config in Dev*/
	u8_t	configManu;	/**< Device config in Manual*/
};

/**
 * @brief Define Device configurations.
 */
struct sotp_bcm58202_dev_config {
	u16_t    BRCMRevisionID;	/**< Broadcom device revision ID */
	u16_t    devSecurityConfig;	/**< Device security config */
	u16_t    ProductID;		/**< Product ID */
	u16_t    Reserved;		/**< Reserved */
};

/**
 * @brief Define SOTP full configuration structure.
 */
struct sotp_bcm58202_config {
	/**< Device configuration */
	struct sotp_bcm58202_dev_config	Dcfg;
	/**< Secure Boot Loader configuration */
	u32_t		SBLConfiguration;
	/**< Customer ID */
	u32_t		CustomerID;
	/**< Customer revision ID */
	u16_t		CustomerRevisionID;
	/**< Secure Boot Image revision ID */
	u16_t		SBIRevisionID;
	/**< 256 bits of AES key */
	u8_t		kAES[OTP_SIZE_KAES];
	/**< 256 bits of HMAC-SHA256 key */
	u8_t		kHMAC_SHA256[OTP_SIZE_KHMAC_SHA256];
	/**< 256 bits of DAUTH key */
	u8_t		DAUTH[OTP_SIZE_DAUTH];
};

/* ***************************************************************************
 * Extern Variables
 * ****************************************************************************/

#ifdef CONFIG_BRCM_DRIVER_SOTP
/* ***************************************************************************
 * Function Prototypes
 * ****************************************************************************/
/**
 * @brief SOTP memory read
 * @details This Api is used to read the row data from the SOTP memory
 * @param[in]   Address(Row) from which the memory to be read.
 * @param[in]   sotp ecc address.
 * @retval  The read value in 64 bit is returned
 */
u64_t sotp_mem_read(u32_t addr, u32_t sotp_add_ecc);

/**
 * @brief SOTP memory write
 * @details This Api is used to write the row data to the SOTP memory
 * @param[in]   Address(Row) to which the memory to be written.
 * @param[in]   sotp ecc address.
 * @param[in]   Data to be written.
 * @retval  The failed bits
 */
u64_t sotp_mem_write(u32_t addr, u32_t sotp_add_ecc, u64_t wdata);

/**
 * @brief Read the SOTP fuse
 * @details This Api is used to read the SOTP fuse in verify0
 * FIXME "verify0"
 * @param[in]   Address of the fuse
 * @retval  The value read from the fuse
 */
u64_t sotp_mem_verify0_read(u32_t addr);

/**
 * @brief Read the SOTP fuse
 * @details This Api is used to read the SOTP fuse in normal mode
 * @param[in]   Address of threflecte fuse
 * @param[in]   Function
 * @retval  The value read from the fuse
 */
u64_t sotp_fuse_normal_read(u32_t addr, u32_t fn);

/**
 * @brief Read the SOTP fuse
 * @details This Api is used to read the SOTP fuse in verify mode
 * @param[in]   Address of the fuse
 * @param[in]   Function
 * @retval  The value read from the fuse
 */
u64_t sotp_fuse_verify_read(u32_t addr, u32_t fn);

/**
 * @brief Check if fast authentication enabled
 * @details This Api is used to check if fast authentication enabled
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_fastauth_enabled(void);

/**
 * @brief Check if SOTP is erased
 * @details This Api is used to check if SOTP is erased
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_sotp_erased(void);

/**
 * @brief Read the status of FIPSMode
 * @details This Api is used to read the status of FIPSMode
 * @param[in]   Pointer to the FIPSMode
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_fips_mode(u32_t *FIPSMode);

/**
 * @brief Read the device configuration
 * @details This Api is used to read the device configuration from the
 * memory like broadcom revision ID
 * @param[in]   Pointer to device configuration structure where
 * the values will get returned
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_dev_config(
			struct sotp_bcm58202_dev_config *devConfig);

/**
 * @brief Read the secure boot loader configuration
 * @details This Api is used to read the secure boot loader configuration
 * @param[in]   Pointer to SBL configuration value
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_sbl_config(u32_t *SBLConfiguration);

/**
 * @brief Read the customer ID
 * @details This Api is used to read the customer ID
 * @param[in]   Pointer to customer ID value
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_customer_id(u32_t *customerID);

/**
 * @brief Read the customer configurations
 * @details This Api is used to read the customer configurations
 * @param[in]   Pointer to customer revision ID value
 * @param[in]   Pointer to SBI revision ID value
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_customer_config(u16_t *CustomerRevisionID,
					  u16_t *SBIRevisionID);

/**
 * @brief Read the Keys
 * @details This Api is used to read different keys from the SOTP
 * @param[in]   Pointer to the key buffer
 * @param[in]   Pointer to the key size
 * @param[in]   Key type like AES or DAUTH or HMAC etc
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_read_key(u8_t *key, u16_t *keySize,
				sotp_bcm58202_key_type_t type);

/**
 * @brief Check the device state
 * @details This Api is used to check the device state is AB Production
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_ABProd(void);

/**
 * @brief Check the device state
 * @details This Api is used to check the device state is ABDev
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_ABDev(void);

/**
 * @brief Check the device state
 * @details This Api is used to check the device state is unassigned
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_unassigned(void);

/**
 * @brief Check the device state
 * @details This Api is used to check the device state is Non AB
 * @retval  #sotp_bcm58202_status
 */
sotp_bcm58202_status_t sotp_is_nonAB(void);

/**
 * @brief Set the keys
 * @details This API is used to set the different OTP keys.
 * @param[in]   key :  Pointer to the key array
 * @param[in]   keySize :  size of the key
 * @param[in]   type :  Type of the key #OTPKeyType
 * @return  Valid status and key is set
 * @return Invalid status if issues in setting the key
 */
sotp_bcm58202_status_t sotp_set_key(u8_t *key, u16_t keySize,
				sotp_bcm58202_key_type_t type);

/**
 * @brief Read CRMU Max VCO bits from Chip OTP
 * @details This API is used to Read CRMU Max VCO bits from Chip
 *  		OTP
 * @return : Max VCO (0 - 3)
 */
u32_t read_crmu_otp_max_vco(void);

/**
 * @brief Read Buffer Vout Control bit from Chip OTP
 * @details This API is used to Read Buffer Vout Control bit from Chip
 *  		OTP
 * @return 0: Buffer Vout Control (0 or 1)
 */
u32_t read_bvc_from_chip_otp(void);

/**
 * @brief Read chip ID
 * @details This API is used to read the chip ID
 * @return  chip ID
 */
u32_t sotp_read_chip_id(void);
/** @} */

#else
#define sotp_mem_read(addr, sotp_add_ecc)		0
#define sotp_mem_write(addr, sotp_add_ecc, wdata)	do {} while (0)
#define sotp_mem_verify0_read(addr)			do {} while (0)
#define sotp_fuse_normal_read(addr, fn)			do {} while (0)
#define sotp_fuse_verify_read(addr, fn)			do {} while (0)
#define sotp_is_fastauth_enabled()			do {} while (0)
#define sotp_is_sotp_erased()				do {} while (0)
#define sotp_read_fips_mode(FIPSMode)			do {} while (0)
#define sotp_read_dev_config(devConfig)			0
#define sotp_read_sbl_config(SBLConfiguration)		do {} while (0)
#define sotp_read_customer_id(customerID)		0
#define sotp_read_customer_config(Customer, SBI)	0
#define sotp_read_key(key, keySize, type)		({ ARG_UNUSED(key);    \
							   ARG_UNUSED(keySize);\
							   0; })
#define sotp_is_ABProd()				do {} while (0)
#define sotp_is_ABDev()					do {} while (0)
#define sotp_is_unassigned()				do {} while (0)
#define sotp_is_nonAB()					do {} while (0)
#define sotp_set_key(key, keySize, type)		({ ARG_UNUSED(key);    \
							   ARG_UNUSED(keySize);\
							   0; })
#define sotp_read_chip_id()				do {} while (0)
#endif

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* __SOTP_H__ */
