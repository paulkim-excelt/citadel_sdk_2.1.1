/******************************************************************************
*
* Copyright (c) 2009, Broadcom Co.
* 16215 Alton Parkway
* PO Box 57013
* Irvine CA 92619-7013
*
* File: smbus.h
* Desc: tpm command pre/post processing
*
* Revision History:
*
*****************************************************************************/

#ifndef _SMBUS_H_
#define _SMBUS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Ported from M0.h of Cortex M3*/
#define  M0_POWER_OFF_PLL
/* These two are defined in ihost_config.h
#define  PLL_PWRON_WAIT       0x50
#define  PLL_CONFIG_WAIT      0x50 
*/ 
#define  PWRDN_WAIT           0x50
#define  PLL_SWITCH_WAIT      0x50

/* Ported from fw/pwr/mproc_pm.h for Temp buffer for CDRU_CLK_DIS_CTRL */
#define  CDRU_SAVE_CLK_DIS_CTRL              0x30018EC4   //Temp buffer for CDRU_CLK_DIS_CTRL

/* Ported from fw\include\broadcom\drivers\dmu.h */
#define DMU_UNUSED_DEVICE            (              \
(1<<CDRU_CLK_DIS_CTRL__wdt_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__usbh_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__spi1_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__spi3_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__spi4_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__uart1_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__uart2_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__uart3_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__fsk_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__pwm_clk_disable ) |       \
(1<<CDRU_CLK_DIS_CTRL__adc_clk_disable ) )

#define UDID3		0x810814E4
#define	UDID2		0x16Fa0044
#define UDID1		0x0
#define UDID0		0x14E416FA

#define DEFAULT_SMB_SLAVE_ADDRESS			82
#define MAX_SMBUS_FW_FIFO_SIZE				128
#define SLAVE_RD_EVENT_INT					0x00200000
#define SLAVE_RX_EVENT_INT					0x01000000
#define SMB_SLAVE_START_BUSY				0x00800000
#define SMB_SLAVE_ARP_EVENT					0x00100000
#define SMB_QUICK_COMMAND_ON				0xC00000A5
#define SMB_QUICK_COMMAND_OFF				0xC00000A4
#define SMB_COMMAND_PROCESS_CALL			0x80000009
#define	SMB_COMMAND_CODE_OFFSET				0x1
#define DEVICE_MASK_OFFSET					0x2
#define USH_TURN_DEVICE_OFF					0x0
#define USH_TURN_DEVICE_ON					0x1
#define USH_SOFT_RESET_DEVICE               0x4
#define USH_GET_FIRMWARE_VERSION			0x6
#define USH_ENABLE_DEVICES					0x2
#define USH_DISABLE_DEVICES					0x3
#define USH_GET_PRESENCE_FOR_DEVICES		0x4
#define	USH_GET_DEVICE_STATUS				0x5
#define USH_PREPARE_TO_ARP					0x1
#define USH_GENERIC_ARP_RESET_DEVICE		0x2
#define USH_GETGENERIC_UDID					0x3
#define USH_ARP_ASSIGN_ADDRESS				0x4
#define USH_GETDIRECTED_UDID				0xA5
#define USH_DIRECTED_ARP_RESET_DEVICE		0xA4
#define PROCESSCALL_PAYLOAD					0x5
#define READWORD_PAYLOAD					0x3
#define DEVICE_ENABNLE_MASK					0X0F
#define DEVICE_PRESENCE_MASK				0xF0
#define	SMB_DEVICE_FP						0x1
#define SMD_DEVICE_SMARTCARD				0x2
#define	SMB_DEVICE_RFID						0x4
#define SMB_DEVICE_CV_ONLY_RFID				0x8
#define MIN_NON_QUICK_COMMAND_PAYLOAD		0x2
#define UDID_SIZE							16

/* Add EC CMD for POA */
#define USH_POA_STARTUP_FP                  0x2
#define USH_POA_GET_TIMEOUT                 0x08
#define USH_POA_GET_CMD_STS                 0x09
#define USH_POA_MODE_ENABLED                0x0A
#define USH_POA_MODE_DISABLED               0x0B

#define SMB_GPIO_WAKEUP_N_PIN				0x4
#define SMB_GPIO_INT_LEVEL_ALL				0x7

#define DEFAULT_WAKUP_TIMER_VALUE			0x40
#define DEFAULT_DELAY_COUNTER_VALUE			0x36
#define DEFUALT_REF_CLOCK_DIV_RATIO			0x10
#define DEFAULT_LPENTRY_SELECT_VECT_VALUE	0xDEF
#define DEFAULT_LPEXIT_SELECT_VECT_VALUE	0x37
#define SMB_SLAVE_TRANSACTION_STATUS_BITS	0x03800000
#define SMB_SLAVE_STAUS_BIT_START			23
#define	FW_VERSION_SIZE_IN_BYTES			7
#define MAJOR_BRANCH_BYTE					24
#define MINOR_BRANCH_BYTE					16
#define ROM_VERSION_BYTE					8

//=========================================================================================
//  Usage of M0 SCRAM
//=========================================================================================
/* Citadel_SMBus, These defines are moved from pwr/mproc_pm.h here */

#define CRMU_FLASH_DEVICE_TYPE              0x300180C0
#define CRMU_CHIP_VERION_ID                 0x300180C4
#define CRMU_HOST_CS_STATE                  0x300180C8
#define CRMU_FP_CAPTURE_STATUS              0x300180CC
// Saved log info in M0_SCRAM

#define CRMU_RESET_EVENT_LOG                0x30018f78   // Reset Interrupt log
#define CRMU_POWER_EVENT_LOG                0x30018f74   // Power Interrupt log
#define CRMU_ERROR_EVENT_LOG                0x30018f70   // Error Interrupt log
//#define CRMU_SLEEP_EVENT_LOG_0              0x30018f60   // Sleeping interrupt log
//#define CRMU_SLEEP_EVENT_LOG_1              0x30018f64   // Sleeping interrupt log

#define CRMU_SMBUS_BUF_SIZE                 128 
#define CRMU_SMBUS_BUFFER                   0x30018ED0   // size 128+16
#define CRMU_SMBUS_DATA_SIZE                (CRMU_SMBUS_BUFFER+CRMU_SMBUS_BUF_SIZE)
#define CRMU_POA_WAKEUP                     (CRMU_SMBUS_BUFFER+CRMU_SMBUS_BUF_SIZE+4)   // 1 => this is POA_WAKEUP
#define CRMU_POA_EC_COUNTER                 (CRMU_SMBUS_BUFFER+CRMU_SMBUS_BUF_SIZE+8)   // how many packages has been recevied from EC, two bytes */
#define CRMU_POA_LATEST_CMD                 (CRMU_SMBUS_BUFFER+CRMU_SMBUS_BUF_SIZE+10)  // one byte

#define CRMU_IHOST_POWER_CONFIG_NORMAL      0x0
#define CRMU_IHOST_POWER_CONFIG_SLEEP       0x2
#define CRMU_IHOST_POWER_CONFIG_DEEPSLEEP   0x3

//
// Wake up source
//
#define CITADEL_WAKEUP_USB_RESUME   1
#define CITADEL_WAKEUP_NFC_PRES     2
#define CITADEL_WAKEUP_SCDET        4
#define CITADEL_WAKEUP_SMBUS        8
#define CITADEL_WAKEUP_FP           0x10
#define CITADEL_WAKEUP_DEFAULT_SOURCE      (CITADEL_WAKEUP_USB_RESUME | CITADEL_WAKEUP_NFC_PRES | CITADEL_WAKEUP_SCDET)
#define CITADEL_WAKEUP_ALL_SUSPEND_SOURCE  (CITADEL_WAKEUP_USB_RESUME | CITADEL_WAKEUP_NFC_PRES | CITADEL_WAKEUP_SCDET)

typedef struct _phx2_smbus_fifo_data {
	uint32_t slaveRxData[MAX_SMBUS_FW_FIFO_SIZE];
	uint32_t slaveTxData[MAX_SMBUS_FW_FIFO_SIZE];
	uint32_t masterRxData[MAX_SMBUS_FW_FIFO_SIZE];
	uint32_t masterTxData[MAX_SMBUS_FW_FIFO_SIZE];
	uint32_t udid[4];
	uint32_t errorCount;
	uint8_t  lowPowerState;
}phx2_smbus_fifo_data;

void smbus_init(void);
u8_t get_latest_ec_cmd(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SMBUS_H_ */
