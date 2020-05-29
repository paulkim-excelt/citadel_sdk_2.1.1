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

/** Power Manager Modes
 *
 * MODE         DESCRIPTION  WAKEUP    NOTE
 *-------------------------------------------
 * RUN_1000     = 1000MHz			  Console @ 115200
 * RUN_250      = 250MHz			  Console @ 115200
 * RUN_200      = 200MHz			  Console @ 115200
 * RUN_100      = 100MHz			  Console @ 38400
 * RUN_25       = 25MHz				  Console @ 19200
 * SLEEP        = A7 CORE & MPROC clock gate, except wakeup sources	AON_WAKEUP
 * button	JTAG will be lost
 *
 * DRIPS        = A7 Core is powered down, But system RAM (part of  MPROC-Ihost)
 * power is preserved. Rest of MPROC (other than A7 & GIC) is only clock gated.
 * Wake-up source are same as that of Sleep. However, Since SRAM is powered on,
 * the content is not lost, but CPU & GIC are powered off, so their content(state)
 * has to be preserved somewhere and restored upon wake-up from DRIPS. JTAG
 * will be lost. No Code execution is possible through JTAG because the
 * core-sight slave inside the A7 CPU is powered down. However, since the
 * coresight master has access to the buses some register of the components
 * such as WDT, NIC, DMA etc. could be read.
 *
 * DEEPSLEEP    = CORE + MPROC power off	AON_WAKEUP button
 *
 *
 * Interrupts from CRMU to (IHost) A7
 * #define  IRQ34 CHIP_INTR__CRMU_MPROC_M0_LOCKUP_INTERRUPT
 * #define  IRQ33 CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT
 * #define  IRQ32 CHIP_INTR__CRMU_MPROC_AXI_DEC_ERR_INTERRUPT
 * #define  IRQ31 CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT
 * #define  IRQ30 CHIP_INTR__CRMU_MPROC_SPL_WDOG_INTERRUPT
 * #define  IRQ29 CHIP_INTR__CRMU_MPROC_SPL_RST_INTERRUPT
 * #define  IRQ28 CHIP_INTR__CRMU_MPROC_SPL_PVT_INTERRUPT
 * #define  IRQ27 CHIP_INTR__CRMU_MPROC_SPL_FREQ_INTERRUPT
 * #define  IRQ26 CHIP_INTR__CRMU_MPROC_SPRU_ALARM_EVENT_INTERRUPT
 * #define  IRQ25 CHIP_INTR__CRMU_MPROC_SPRU_RTC_EVENT_INTERRUPT
 * #define  IRQ24 CHIP_INTR__CRMU_MPROC_IHOST_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ23 CHIP_INTR__CRMU_MPROC_BBL_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ22 CHIP_INTR__CRMU_MPROC_CRMU_CLK_GLITCH_TAMPER_INTR
 * #define  IRQ21 CHIP_INTR__CRMU_MPROC_CRMU_FID_TAMPER_INTR
 * #define  IRQ20 CHIP_INTR__CRMU_MPROC_CHIP_FID_TAMPER_INTR
 * #define  IRQ19 CHIP_INTR__CRMU_MPROC_RTIC_TAMPER_INTR
 * #define  IRQ18 CHIP_INTR__CRMU_MPROC_CRMU_ECC_TAMPER_INTR
 * #define  IRQ17 CHIP_INTR__CRMU_MPROC_VOLTAGE_GLITCH_TAMPER_INTR
 * #define  IRQ16 CHIP_INTR__CRMU_MPROC_INTERRUPT
 *
 * MailBox is the only IRQ against which User can install handlers.
 */

/**
 * FIXME:
 * (i) All code annotated "TODO".
 * (ii) Confirm / Clean all code that are commented using "#if 0"
 *
 */
#include "pm_regs.h"
#include "mproc_pm.h"
#include "save_restore.h"
#include <logging/sys_log.h>
#include <clock_a7.h>
#include <spru/spru_access.h>

/* FIXME: Once this driver is migrated to Zephyr, socregs.h will not be
 * available and the following define can be removed and smau.h can be included
 * Including smau.h now, results in redefinition errors.
 */
#ifndef SMU_CFG_NUM_WINDOWS
#define SMU_CFG_NUM_WINDOWS 4
#endif

#include <errno.h>
#include <uart.h>

#include "bbl.h"

#define INT_SRC_CRMU_MPROC_MAILBOX CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT

#define INT_SRC_CRMU_MPROC_SPRU_RTC_PERIODIC CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT

#define INT_SRC_SMART_CARD0 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT0
#define INT_SRC_SMART_CARD1 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT1
#define INT_SRC_SMART_CARD2 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT2

#define INT_SRC_UART0 CHIP_INTR__IOSYS_UART0_INTERRUPT
#define INT_SRC_UART1 CHIP_INTR__IOSYS_UART1_INTERRUPT
#define INT_SRC_UART2 CHIP_INTR__IOSYS_UART2_INTERRUPT
#define INT_SRC_UART3 CHIP_INTR__IOSYS_UART3_INTERRUPT

extern u32_t __start;
static u32_t xip_start;

typedef struct mcu_idram_bin_info {
	MCU_IDRAM_BIN_TYPE_e bin_type;
	u32_t bin_id;
	char bin_name[16];
	u8_t *bin_start_addr; /* first byte included */
	u8_t *bin_end_addr; /*first byte not included */
	MCU_IDRAM_CONTENT_ID_e mcu_cid;
} MCU_IDRAM_BIN_INFO_s;

#ifndef PATH_TO_M0_IMAGE_BIN
#define PATH_TO_M0_IMAGE_BIN
#endif

__asm__(
	"\n\t"
	/*	"	.data\n\t"*/
	"	.global MCU_M0_BIN_START\n\t"
	"MCU_M0_BIN_START:\n\t"
	"	.incbin \"" PATH_TO_M0_IMAGE_BIN "M0_image.bin\"\n\t"
	"	.global MCU_M0_BIN_END\n\t"
	"MCU_M0_BIN_END:\n\t"
	"	.word 0x11223344\n\t"
	/*	"	.text\n\t"*/

	"\n\t"
	);

extern u8_t MCU_M0_BIN_START;
extern u8_t MCU_M0_BIN_END;

/** Power Manager Device Private data structure */
struct bcm5820x_pm_dev_data_t {
	/**< Current State of Power Manager */
	MPROC_PM_MODE_e cur_pm_state;
	/**< Target State of Power Manager */
	MPROC_PM_MODE_e tgt_pm_state;

	/**< Wake Up Sources Configuration of Power Manager */
	MPROC_AON_GPIO_WAKEUP_SETTINGS_s *aon_gpio_wakeup_config;

	/**< RTC Wake-Up Time in Seconds
	 * after this time, rtc will wakeup system. if 0, disabled.
	 */
	u32_t rtc_wakeup_time;

	/**< 1. AON GPIO IRQ Callback function pointer */
	pm_aon_gpio_irq_callback_t aon_gpio_irq_cb[MPROC_AON_GPIO_END];

	/**< 2. NFC LPTD IRQ Callback function pointer */
	pm_nfc_lptd_irq_callback_t nfc_lptd_irq_cb;

	/**< 3. USB IRQ Callback function pointer */
	pm_usb_irq_callback_t usb_irq_cb[MPROC_USB_END];

	/**< 4. EC Command IRQ Callback function pointer */
	pm_ec_cmd_irq_callback_t ec_cmd_irq_cb;

	/**< 5. RTC IRQ Callback function pointer */
	pm_rtc_irq_callback_t rtc_irq_cb;

	/**< 6. Tamper IRQ Callback function pointer */
	pm_tamper_irq_callback_t tamper_irq_cb;

	/**< 7. SMBUS IRQ Callback function pointer */
	pm_smbus_irq_callback_t smbus_irq_cb;

	/* AON GPIO IRQ Callback when the system is in RUN mode */
	pm_aon_gpio_run_irq_callback_t aon_gpio_run_irq_cb;
};

/* convenience defines */
#define DEV_DATA(dev) ((struct bcm5820x_pm_dev_data_t *)(dev)->driver_data)

static CITADEL_PM_ISR_ARGS_s citadel_pm_isr_args;
static MCU_PLATFORM_REGS_s platform_regs;
static MPROC_PM_MODE_e citadel_cur_pm_state = MPROC_DEFAULT_PM_MODE;
static MPROC_PM_MODE_e citadel_tgt_pm_state = MPROC_DEFAULT_PM_MODE;

#ifdef MPROC_PM_ACCESS_BBL
/* 1=bbl ok */
static u32_t citadel_bbl_access_ok = 1;
/* unit in second, after this time, rtc will wakeup system. if 0, disabled. */
static u32_t citadel_rtc_wakeup_time = MPROC_PM_DEFAULT_RTC_WAKEUP_TIME;
#endif

/* Public API Declarations */
MPROC_PM_MODE_e mproc_get_current_pm_state(struct device *dev);
char *mproc_get_pm_state_desc(struct device *dev, MPROC_PM_MODE_e state);
u32_t bcm5820x_get_rtc_wakeup_time(struct device *dev);

/**
 * Initial value of WAKEUP_AON_GPIOS mask
 * 1. Only 0 (SW11.0) & 3 are supported by default on SVK board
 * Note: To generate AON GPIO3 which indicates the insertion of
 * smart card into the slot, turn SW11.3 (the 4th switch on SW11)
 * ON.
 */
#define CITADEL_WAKEUP_AON_GPIOS (0)

static MPROC_AON_GPIO_WAKEUP_SETTINGS_s citadel_aon_gpio_wakeup_config = {
	.wakeup_src = CITADEL_WAKEUP_AON_GPIOS, /* bitmap: 1=used, 0=not used*/
	.int_type   = 0x0, /*1=level, 0=edge*/
	.int_de     = CITADEL_WAKEUP_AON_GPIOS, /*1=both edge,  0=single edge*/
	.int_edge   = 0x0, /*1=rising edge or high level, 0=falling edge or low level*/
}; /*AON_GPIO0 both edges*/

/**
 * Initial value of WAKEUP_USB mask
 */
#define CITADEL_WAKEUP_USB (BIT(MPROC_USB0_WAKE) | BIT(MPROC_USB0_FLTR) | BIT(MPROC_USB1_WAKE) | BIT(MPROC_USB1_FLTR))

static MPROC_USB_WAKEUP_SETTINGS_s citadel_usb_wakeup_config = {
	.wakeup_src = CITADEL_WAKEUP_USB, /* bitmap: 1=used, 0=not used*/
};

static MCU_IDRAM_BIN_INFO_s m0_bins_info = {
	MCU_IDRAM__ISR_BIN,
	MCU_NVIC_MAILBOX_EVENT,
	"M0_IMAGE",
	&MCU_M0_BIN_START,
	&MCU_M0_BIN_END,
	MCU_IDRAM_CONTENT__M0_IMAGE_BIN
};

/******************************************************************************
 *********** Local Helper Functions *************
 * TODO: Will be replaced with global alternatives after PM code has matured
 * & tested.
 *****************************************************************************/
#define  local_irq_enable() \
	do { __asm__ volatile("cpsie i"::: "memory", "cc"); } while (0)
#define  local_irq_disable() \
	do { __asm__ volatile("cpsid i"::: "memory", "cc"); } while (0)
#define  dsb() do { __asm__ volatile("dsb"); } while (0)
#define  isb() do { __asm__ volatile("isb"); } while (0)
#define  wfi() do { __asm__ volatile("wfi"); } while (0)

/* set specified bits to 1, other bits unchange */
static void mproc_reg32_set_masked_bits(u32_t addr, u32_t mask)
{
	u32_t regval = sys_read32(addr);

	regval |= mask;

	sys_write32(regval, addr);
}

/* set specified bits to 0, other bits unchange */
static void mproc_reg32_clr_masked_bits(u32_t addr, u32_t mask)
{
	u32_t regval = sys_read32(addr);

	regval &= ~mask;

	sys_write32(regval, addr);
}

/******************************************************************************
 * P R I V A T E  F U N C T I O N s
 *****************************************************************************/
#ifdef MPROC_PM_FASTXIP_SUPPORT

#define TAG_VALID_SCRAM			(0x3)
#define AES_KEY_ADDRESS		    (0x50020200)
#define AUTH_KEY_ADDRESS	    (0x50020280)
#define SMAU_AES_KEY_WORD_0	    (AES_KEY_ADDRESS  + 0x00)
#define SMAU_AES_KEY_WORD_1     (AES_KEY_ADDRESS  + 0x04)
#define SMAU_AES_KEY_WORD_2     (AES_KEY_ADDRESS  + 0x08)
#define SMAU_AES_KEY_WORD_3     (AES_KEY_ADDRESS  + 0x0C)
#define SMAU_AES_KEY_WORD_4     (AES_KEY_ADDRESS  + 0x10)
#define SMAU_AES_KEY_WORD_5     (AES_KEY_ADDRESS  + 0x14)
#define SMAU_AES_KEY_WORD_6     (AES_KEY_ADDRESS  + 0x18)
#define SMAU_AES_KEY_WORD_7     (AES_KEY_ADDRESS  + 0x1C)
#define SMAU_AUTH_KEY_WORD_0    (AUTH_KEY_ADDRESS + 0x00)
#define SMAU_AUTH_KEY_WORD_1    (AUTH_KEY_ADDRESS + 0x04)
#define SMAU_AUTH_KEY_WORD_2    (AUTH_KEY_ADDRESS + 0x08)
#define SMAU_AUTH_KEY_WORD_3    (AUTH_KEY_ADDRESS + 0x0C)
#define SMAU_AUTH_KEY_WORD_4    (AUTH_KEY_ADDRESS + 0x10)
#define SMAU_AUTH_KEY_WORD_5    (AUTH_KEY_ADDRESS + 0x14)
#define SMAU_AUTH_KEY_WORD_6    (AUTH_KEY_ADDRESS + 0x18)
#define SMAU_AUTH_KEY_WORD_7    (AUTH_KEY_ADDRESS + 0x1C)

/* Reflects the input data about it's centre bit */
static u32_t reflect(u32_t data, u32_t nbits)
{
	u32_t reflection = 0;
	u32_t i;

	for (i = 0; i < nbits; i++) {
		/* If the LSB bit is set, set the reflection of it. */
		if (data & 0x01)
			reflection |= (1 << ((nbits - 1) - i));

		data = (data >> 1);
	}

	return reflection;
}
/* Compute CRC32 of char array */
static u32_t calc_crc32(u32_t initval, u8_t *charArr, u32_t arraySize)
{
	u32_t cval = initval;
	u8_t data, charVal;
	u32_t i;
	s32_t j;
	/* use const here to put this big array into rodata section */
	const u32_t ctable[256] = {\
		0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
		0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
		0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
		0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
		0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
		0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
		0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
		0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
		0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
		0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
		0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
		0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
		0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
		0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
		0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
		0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
		0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
		0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
		0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
		0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
		0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
		0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
		0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
		0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
		0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
		0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
		0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
		0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
		0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
		0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
		0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
		0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
		0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
		0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
		0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
		0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
		0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
		0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
		0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
		0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
		0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
		0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
		0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
		0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
		0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
		0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
		0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
		0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
		0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
		0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
		0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
		0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
		0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
		0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
		0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
		0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
		0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
		0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
		0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
		0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
		0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
		0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
		0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
		0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
	};

	for (i = 0; i < arraySize / 4; i++) {
		for (j = 3; j >= 0; j--) {
			charVal = charArr[4 * i + j];
			charVal = reflect(charVal, 8);
			data = charVal ^ (cval >> 24);
			cval = ctable[data] ^ (cval << 8);
		}
	}

	cval = reflect(cval, 32);
	cval = cval ^ 0xFFFFFFFF;

	return cval;
}

/**
 * @brief Save SMAU Settings into SCRAM (Scratch RAM).
 *
 * This function stores the SMAU settings into SCRAM.
 *
 * TODO:
 * 1. Should we be saving the SMAU settings for all the 4 windows?
 * If yes, then check if we have enough space in SCRAM for 4 structures.
 * It doesn't look like this should be case, since we have only one return
 * address to return to.
 *
 * @param arg XIP Address
 * @param arg SMAU window number
 *
 * @return N/A
 */
static void mproc_save_smau_setting(u32_t XIP_ADDRESS, u32_t win)
{
	MCU_SCRAM_FAST_XIP_s *fastxip = CRMU_GET_FAST_XIP_SCRAM_ENTRY();

	if (win >= SMU_CFG_NUM_WINDOWS) {
		SYS_LOG_DBG("%s %d wrong window:%x \n", __func__, __LINE__, win);
		return;
	}

	/* other windows no encryption */
	if( win > 0 ) {
		fastxip->smau_win_base_a[win-1] = sys_read32(SMU_WIN_BASE_0 + (win * 4));
		fastxip->smau_win_size_a[win-1] = sys_read32(SMU_WIN_SIZE_0 + (win * 4));
		fastxip->smau_hmac_base_a[win-1] = sys_read32(SMU_HMAC_BASE_0 + (win * 4));
		return;
	}

	/* Offset: 0x20 */
	fastxip->fast_xip_tag = TAG_VALID_SCRAM; /* FastXIP present in AON Scratch RAM */
	fastxip->smau_cfg0 = sys_read32(SMU_CFG_0);
	fastxip->smau_cfg1 = sys_read32(SMU_CFG_1);
	/* thumb mode, SBL firstly jumps to the workaround bin to restore
	 * SMU_CFG_1, then jumps to real AAI at XIP_ADDRESS.
	 */
	fastxip->xip_addr = (u32_t)xip_start; ;

	/* SMAU Window Number */
	fastxip->win = win;

	/* Offset: 0x30 */
	fastxip->smau_win_base = sys_read32(SMU_WIN_BASE_0 + (win * 4));
	fastxip->smau_win_size = sys_read32(SMU_WIN_SIZE_0 + (win * 4));
	fastxip->smau_hmac_base = sys_read32(SMU_HMAC_BASE_0 + (win * 4));
	/* 0x0, real aai image saved here
	 * Jump Address in Thumb Mode while exiting DRIPS
	 * & in ARM Mode while exiting DeepSleep.
	 *
	 * FIXME: Since there is no FastXIP WAR in Citadel, is this required
	 * anymore? But No harm, since this is considered reserved by SBL.
	 */
	fastxip->smc_cfg = XIP_ADDRESS;

	/* Offset: 0x40 */
	fastxip->aes_key_31_0 = sys_read32(SMAU_AES_KEY_WORD_0 + (win * 0x20));
	fastxip->aes_key_63_32 = sys_read32(SMAU_AES_KEY_WORD_1 + (win * 0x20));
	fastxip->aes_key_95_64 = sys_read32(SMAU_AES_KEY_WORD_2 + (win * 0x20));
	fastxip->aes_key_127_96 = sys_read32(SMAU_AES_KEY_WORD_3 + (win * 0x20));
	/* Offset: 0x50 */
	fastxip->aes_key_159_128 = sys_read32(SMAU_AES_KEY_WORD_4 + (win * 0x20));
	fastxip->aes_key_191_160 = sys_read32(SMAU_AES_KEY_WORD_5 + (win * 0x20));
	fastxip->aes_key_223_192 = sys_read32(SMAU_AES_KEY_WORD_6 + (win * 0x20));
	fastxip->aes_key_255_224 = sys_read32(SMAU_AES_KEY_WORD_7 + (win * 0x20));
	/* Offset: 0x60 */
	fastxip->hmac_key_31_0 = sys_read32(SMAU_AUTH_KEY_WORD_0 + (win * 0x20));
	fastxip->hmac_key_63_32 = sys_read32(SMAU_AUTH_KEY_WORD_1 + (win * 0x20));
	fastxip->hmac_key_95_64 = sys_read32(SMAU_AUTH_KEY_WORD_2 + (win * 0x20));
	fastxip->hmac_key_127_96 = sys_read32(SMAU_AUTH_KEY_WORD_3 + (win * 0x20));
	/* Offset: 0x70 */
	fastxip->hmac_key_159_128 = sys_read32(SMAU_AUTH_KEY_WORD_4 + (win * 0x20));
	fastxip->hmac_key_191_160 = sys_read32(SMAU_AUTH_KEY_WORD_5 + (win * 0x20));
	fastxip->hmac_key_223_192 = sys_read32(SMAU_AUTH_KEY_WORD_6 + (win * 0x20));
	fastxip->hmac_key_255_224 = sys_read32(SMAU_AUTH_KEY_WORD_7 + (win * 0x20));
	/* Offset: 0x80; Number of Bytes Stored
	 * in SCRAM = (0x80 - 0x20) = 96
	 */
	fastxip->scram_cksum = calc_crc32(0xFFFFFFFF,
			(u8_t *)((u32_t)fastxip + 0x20), 96);

	SYS_LOG_DBG("Saved SMAU Config for FastXIP: wakeup_entry=%08x", fastxip->xip_addr);
}
#endif

/**
 * @brief Status of MCU Job Progress.
 *
 * -- TODO --  -- what is the usage?
 * Can the MPROC(A7) wait on this flag?
 * Noone is using this.
 *
 * @param arg XIP Address
 *
 * @return N/A
 */
#ifdef MPROC_PM_WAIT_FOR_MCU_JOB_DONE
static u32_t mproc_get_mcu_job_progress(void)
{
	CITADEL_PM_ISR_ARGS_s *pm_args = CRMU_GET_PM_ISR_ARGS();

	return pm_args->progress;
}
#endif

/**
 * @brief Send Mailbox Msg to MCU to set a desired PM Mode.
 *
 * -- TODO --  -- The name is bit misleading.
 * This function appears to be setting a mode/state
 * so, ideally this should be called something else?
 *
 * @param arg XIP Address
 *
 * @return N/A
 */
static s32_t mproc_send_mailbox_msg_to_mcu(MPROC_PM_MODE_e state)
{
	u32_t word0, word1;

	switch (state) {
	case MPROC_PM_MODE__RUN_1000:
		word0 = MAILBOX_CODE0__RUN_1000;
		word1 = MAILBOX_CODE1__PM;
		break;
	case MPROC_PM_MODE__RUN_500:
		word0 = MAILBOX_CODE0__RUN_500;
		word1 = MAILBOX_CODE1__PM;
		break;
	case MPROC_PM_MODE__RUN_200:
		word0 = MAILBOX_CODE0__RUN_200;
		word1 = MAILBOX_CODE1__PM;
		break;
#ifdef MPROC_PM__DPA_CLOCK
	case MPROC_PM_MODE__RUN_187:
		word0 = MAILBOX_CODE0__RUN_187;
		word1 = MAILBOX_CODE1__PM;
		break;
#endif
	case MPROC_PM_MODE__SLEEP:
		word0 = MAILBOX_CODE0__SLEEP;
		word1 = MAILBOX_CODE1__PM;
		break;
	case MPROC_PM_MODE__DRIPS:
		word0 = MAILBOX_CODE0__DRIPS;
		word1 = MAILBOX_CODE1__PM;
		break;
	case MPROC_PM_MODE__DEEPSLEEP:
		word0 = MAILBOX_CODE0__DEEPSLEEP;
		word1 = MAILBOX_CODE1__PM;
		break;
	default:
		SYS_LOG_ERR("Not supported PM state: %d!", state);
		return -EINVAL;
	}

	bcm_sync();

	dmu_send_mbox_msg_to_mcu(word0, word1);
	bcm_sync();
	SYS_LOG_DBG("Send msg to MCU: word0=0x%08x, word1=0x%08x", word0, word1);
	return 0;
}

#ifdef MPROC_PM_ACCESS_BBL
/**
 * @brief Get to know if BBL code is compiled and available.
 *
 * @param None
 *
 * @return N/A
 */
u32_t citadel_pm_get_bbl_access(void)
{
	return citadel_bbl_access_ok;
}
#endif

/**
 * @brief Show Available Wake-Up Sources.
 *
 * For a given state, print the available wakeup sources
 * -- TODO --  --
 * 1. Should this be made a public API? Because it seems customer will benefit
 * by knowing the wake-up sources for a given state.
 *
 * 2. What are the wake-up sources for DRIPS mode?
 * 3. what modifies vds_aon?
 *
 * @param PM Mode / State
 *
 * @return N/A
 */
static s32_t mproc_show_available_wakeup_srcs(struct device *dev,
				MPROC_PM_MODE_e state)
{
	u32_t aon_gpio0 = 0, rtc = 0, tamper = 0, bbl = 0, vds_aon = 0;

	ARG_UNUSED(dev);

	if (state >= MPROC_PM_MODE__SLEEP
		&& state < MPROC_PM_MODE__END) {
		aon_gpio0 = 1;
#ifdef MPROC_PM_ACCESS_BBL
		bbl = 1;
#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
		rtc = 1;
#endif
#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
		tamper = 1;
#endif
#endif
	} else {
		SYS_LOG_ERR("No wakeup source for PM state: %d", state);
		return -EINVAL;
	}
	SYS_LOG_INF("Available Wakeup Sources:");
	if (vds_aon)
		SYS_LOG_INF(" VDS_WAKEUP/SW2");

	if (aon_gpio0)
		SYS_LOG_INF(" AON_WAKEUP/SW10  (SW11.1 ON)");

	if (rtc)
		SYS_LOG_INF(" RTC(%d sec) (0 ignored)",
				  bcm5820x_get_rtc_wakeup_time(dev));

	if (tamper)
		SYS_LOG_INF(" TAMPER");

	if (bbl)
		SYS_LOG_INF("If BBL timeout, Power Down->Remove BAT->Wait"
			" 10s->Power On->Insert BAT->SW7.4 ON");

	SYS_LOG_INF("\r");

	return 0;
}

/**
 * @brief Get IDRAM Content Information.
 *
 * -- TODO --  --
 * 1. Why are we not gathering the Content ID information?
 *
 * @param None
 *
 * @return Info
 */
static s32_t mproc_pm_get_mcu_idram_content_info(
	MCU_IDRAM_CONTENT_INFO_s *info)
{
	u32_t offset = 0, maxlen = 0;

	switch (info->cid) {
	case MCU_IDRAM_CONTENT__START:
		offset = 0;
		maxlen = CRMU_M0_IDRAM_LEN;
		break;
	case MCU_IDRAM_CONTENT__USR:
		offset = MPROC_M0_USER_BASE;
		maxlen = MPROC_M0_USER_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__ISR_ARGS:
		offset = MPROC_M0_ISR_ARGS_OFFSET;
		maxlen = MPROC_M0_ISR_ARGS_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__RTC_OP_MUTEX:
		offset = MPROC_PM_BBL_OP_MUTEX_OFFSET;
		maxlen = MPROC_PM_BBL_OP_MUTEX_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__MAILBOX_ISR:
		offset = MPROC_MAILBOX_ISR_OFFSET;
		maxlen = MPROC_MAILBOX_ISR_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__WAKEUP_ISR:
		offset = MPROC_WAKEUP_ISR_OFFSET;
		maxlen = MPROC_WAKEUP_ISR_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__FASTXIP_WORKAROUND_BIN:
		offset = MPROC_FASTXIP_WORKAROUND_OFFSET;
		maxlen = MPROC_FASTXIP_WORKAROUND_MAXLEN;
		break;
	case MCU_IDRAM_CONTENT__M0_IMAGE_BIN:
		offset = MPROC_M0_CODE_OFFSET;
		maxlen = MPROC_M0_CODE_MAXLEN;
		break;
	default:
		info->start_addr = 0;
		info->maxlen     = 0;
		SYS_LOG_ERR("Unsupported MCU IDRAM content!");
		return -EINVAL;
	}

	info->start_addr = CRMU_M0_IDRAM_ADDR(offset);
	info->maxlen     = maxlen;

	return 0;
}

/**
 * @brief Get MPROC Console Baudrate.
 *
 * -- TODO --  --
 * 1. What should be the baudrate for DRIPS? Are these values valid?
 * 2. Should this be a public API?
 *
 * @param PM state
 *
 * @return Info
 */
static MPROC_CONSOLE_BAUDRATE_e mproc_get_console_baudrate(MPROC_PM_MODE_e state)
{
	MPROC_CONSOLE_BAUDRATE_e baudrate = MPROC_CONSOLE_BAUDRATE__UNCHANGE;

	switch (state) {
	case MPROC_PM_MODE__RUN_1000:
	case MPROC_PM_MODE__RUN_500:
	case MPROC_PM_MODE__RUN_200:
#ifdef MPROC_PM__DPA_CLOCK
	case MPROC_PM_MODE__RUN_187:
#endif
		baudrate = MPROC_CONSOLE_BAUDRATE__115200;
		break;
	case MPROC_PM_MODE__SLEEP:
		baudrate = MPROC_CONSOLE_BAUDRATE__UNCHANGE;
		break;
	case MPROC_PM_MODE__DRIPS:
		baudrate = MPROC_CONSOLE_BAUDRATE__UNCHANGE;
		break;
	case MPROC_PM_MODE__DEEPSLEEP: /* reboot to default */
		baudrate = mproc_get_console_baudrate(MPROC_DEFAULT_PM_MODE);
	default:
		break;
	}

	return baudrate;
}

/**
 * @brief Get MPROC Wakeup Source Decsription.
 *
 * @param wakeup source
 *
 * @return Info
 */
static inline char *mproc_get_pm_wakeup_desc(MPROC_WAKEUP_SRC_MCU_e src)
{
	static char *pm_wakeup_desc[] = {
		[ws_none]       = "NONE",
		[ws_aon_gpio]   = "AON_GPIO",
		[ws_usb]        = "USBPHY",
		[ws_tamper]     = "TAMPER (SPRU ALARM)",
		[ws_rtc]        = "RTC",
		[ws_end]        = "UNKNOWN",
	};

	if (src > ws_end)
		return NULL;

	return pm_wakeup_desc[src];
}

/**
 * @brief Set Current PM Mode / state.
 *
 * -- TODO --  --
 * 1. When does the new mode take effect?
 * 2. Should this be a public API?
 *
 * @param new mode
 *
 * @return none
 */
static void mproc_set_current_pm_state(struct device *dev, MPROC_PM_MODE_e new_mode)
{
	ARG_UNUSED(dev);
	/* mproc_dbg("Set current PM mode to: %s",
	 * mproc_get_pm_state_desc(dev, new_mode));
	 */

	citadel_cur_pm_state = new_mode;
}

/**
 * @brief Get PM Mode / state.
 *
 * -- TODO --  --
 * 1. When does the new mode take effect?
 * 2. Should this be a public API?
 *
 * @param none
 *
 * @return mode
 */
static MPROC_PM_MODE_e mproc_get_target_pm_state(void)
{
	return citadel_tgt_pm_state;
}

/**
 * @brief Set Target PM Mode / state.
 *
 * -- TODO --  --
 * 1. When does the new mode take effect?
 * How this is different from set current state?
 * 2. Should this be a public API?
 *
 * @param new mode
 *
 * @return none
 */
static void mproc_set_target_pm_state(struct device *dev, MPROC_PM_MODE_e new_mode)
{
	ARG_UNUSED(dev);
	/*mproc_dbg("Set target PM mode to: %s", mproc_get_pm_state_desc(dev, new_mode));*/

	citadel_tgt_pm_state = new_mode;
}

/**
 * @brief Set Platform registers
 *
 * GPIO interrupt mask register. Setting the respective bits
 * in this register permits the generation of an interrupt
 * for the respective GPIO input. The interrupt generation
 * is masked by setting the respective bit in this register to logic 0.
 *
 * @param Low Power Entry or Exit
 *
 * @return Info
 */
static void mproc_set_platform_regs(MPROC_LOW_POWER_OP_e en)
{
	MCU_PLATFORM_REGS_s *regs = &platform_regs;

	if (en == MPROC_ENTER_LP) {
		SYS_LOG_DBG("Save platform regs");
		regs->aon_gpio_mask = sys_read32(GP_INT_MSK);
		/* regs->clk_gate_ctrl = sys_read32(CRMU_CLOCK_GATE_CONTROL); */
	} else {
		SYS_LOG_DBG("Restore platform regs");
		sys_write32(regs->aon_gpio_mask, GP_INT_MSK);
		/* cpu_reg32_wr(CRMU_CLOCK_GATE_CONTROL, regs->clk_gate_ctrl); */
	}
}

/**
 * @brief Set systick
 *
 * -- TODO --  -*****-
 * 1. Need to use GIC to disable & enable systick upon LP entry & exit?
 * 2. Should this be a public API?
 *
 * @param enable flag
 *
 * @return none
 */
static void mproc_set_systick(MPROC_LOW_POWER_OP_e en)
{
	if (en == MPROC_ENTER_LP) {
		SYS_LOG_DBG("Disable SYSTICK");

		bcm_sync();

		irq_disable(PPI_IRQ(ARM_TIMER_PPI_IRQ));
	} else {
		SYS_LOG_DBG("Restore SYSTICK");

		irq_enable(PPI_IRQ(ARM_TIMER_PPI_IRQ));
	}

}

/**
 * @brief Set GIC Registers
 *
 * -- TODO --  -*****-
 * 1. Need to use GIC to store and restore GIC registers upon LP entry & exit?
 * 2. The entire state of GIC need to stored & restored.
 * 3. What happens to GIC in different power states(Since the GIC is part of IHOST)?
 * 4. Should the ICFG (Configuration) regs be saved & restored as well?
 *
 * @param enable flag
 *
 * @return none
 */
#define PM_MAX_INTERRUPTS_SUPPORTED	160
#define PM_MAX_ISENABLER_REGS (PM_MAX_INTERRUPTS_SUPPORTED / 32)
#define PM_GICD_ISENABLER(n)	(GIC_GICD_BASE_ADDR + 0x0100 + ((n) * 4))
#define PM_GICD_ICENABLER(n)	(GIC_GICD_BASE_ADDR + 0x0180 + ((n) * 4))
static void mproc_set_gic_regs(MPROC_LOW_POWER_OP_e en)
{
	static u32_t gic_setenable_regs[PM_MAX_ISENABLER_REGS];
	u32_t addr, i = 0, j = 0, bitval = 0, irqno = 0;

	if (en == MPROC_ENTER_LP) {
		SYS_LOG_DBG("Save current GIC interrupt status");
		for (i = 0; i < PM_MAX_ISENABLER_REGS; i++) {
			addr = PM_GICD_ISENABLER(i);
			gic_setenable_regs[i] = sys_read32(addr);
		}
	} else {
		SYS_LOG_DBG("Restore GIC interrupt status");
		for (i = 0; i < PM_MAX_ISENABLER_REGS; i++) {
			for (j = 0; j < 32; j++) {
				irqno  = 32 * i + j;
				bitval = (gic_setenable_regs[i] >> j) & 0x1;

				if (bitval)
					irq_enable(irqno);
				else
					irq_disable(irqno);
			}
		}
	}
	bcm_sync();
}

static void disable_all_irqs(void)
{
	u32_t irq;
	u32_t start = CHIP_INTR__MPROC_PKA_DONE_INTERRUPT;
	u32_t end = CHIP_INTR__IOSYS_PWM_INTERRUPT_BIT5;
	for (irq = start; irq <= end; irq++) {
		irq_disable(SPI_IRQ(irq));
	}
}

/**
 * @brief Set Low Power Flag
 *
 * @param desired state
 *
 * @return result (Success / Failure)
 */
static s32_t citadel_pm_set_low_power_flag(MPROC_PM_MODE_e state)
{
	MPROC_CPU_PW_STAT_e pm_stat, pm_stat_for_sbl;
	u32_t lpstate;

	switch (state) {
	case MPROC_PM_MODE__RUN_1000:
	case MPROC_PM_MODE__RUN_500:
	case MPROC_PM_MODE__RUN_200:
#ifdef MPROC_PM__DPA_CLOCK
	case MPROC_PM_MODE__RUN_187:
#endif
		pm_stat = CPU_PW_NORMAL_MODE;
		pm_stat_for_sbl = CPU_PW_NORMAL_MODE;
		break;
	case MPROC_PM_MODE__SLEEP:
		pm_stat = CPU_PW_STAT_SLEEP;
		pm_stat_for_sbl = CPU_PW_STAT_SLEEP;
		break;
	case MPROC_PM_MODE__DRIPS:
		pm_stat = CPU_PW_STAT_DRIPS;
		pm_stat_for_sbl = CPU_PW_STAT_DEEPSLEEP;
		break;
	case MPROC_PM_MODE__DEEPSLEEP:
		pm_stat = CPU_PW_STAT_DEEPSLEEP;
		pm_stat_for_sbl = CPU_PW_STAT_DEEPSLEEP;
#ifdef MPROC_PM_FASTXIP_DISABLED_ON_A0
		if (dmu_get_soc_revision() < CITADEL_REV__BX) {
			pm_stat = CPU_PW_NORMAL_MODE;
		}
#endif
		break;
	default:
		SYS_LOG_ERR("Unsupported power mode!");
		return -EINVAL;
	}

	/* Set POWER Config register to indicate the system's
	 * pm stat for SKU rom.
	 * This is the register that SBL reads to know which Low Power
	 * mode it is exiting from. It only recognises Sleep &
	 * Deep Sleep, but not DRIPS. So for DRIPS, we program
	 * the same code as Deep Sleep. To help the wake-up code
	 * distinguish between DRIPS & Deep Sleep we use a Persistent
	 * Register, which is currently Persistent Register 10.
	 *
	 */
	sys_write32(pm_stat_for_sbl, CRMU_IHOST_POWER_CONFIG);
	lpstate = sys_read32(DRIPS_OR_DEEPSLEEP_REG);
	sys_write32((lpstate & PM_STATE_MASK) | pm_stat, DRIPS_OR_DEEPSLEEP_REG);

	return 0;
}

/**
 * @brief Set interrupts on Ihost(A7) which are allowed to wakeup STANDBY, not MCU
 *
 * -- TODO --  -*****-
 * 1. Need to use GIC to store and restore GIC registers upon LP entry & exit?
 * 2. The entire state of GIC need to stored & restored.
 * 3. What happens to GIC in different power states(Since the GIC is part of IHOST)?
 * 4. Should the ICFG (Configuration) regs be saved & restored as well?
 *
 * Here all the pending interrupts are being cleared before entering LP mode,
 * is that desirable?
 *
 * @param enable flag
 * @param state
 *
 * @return result (Success / Failure)
 */
static s32_t mproc_set_ihost_wakeup_irqs(MPROC_PM_MODE_e state,
										 MPROC_LOW_POWER_OP_e en)
{
	/* Keep UART IRQ Enabled because we may want to be able to testing
	 * and send some commands.
	 * FIXME: Keeping UART IRQ enabled here doesn't work, in the sense
	 * that issuing a command from the console doesn't wake up the
	 * system from WFI Sleep.
	 */
	u32_t a7_wakeup_src__uart    = 0;
	u32_t a7_wakeup_src__sci     = 0;
	u32_t a7_wakeup_src__rtc     = 0;
	u32_t a7_wakeup_src__aongpio = 1;
	u32_t a7_wakeup_src__usb     = 1;
	u32_t aon_src = citadel_aon_gpio_wakeup_config.wakeup_src;
	u32_t usb_src = citadel_usb_wakeup_config.wakeup_src;

	if (en == MPROC_ENTER_LP) {
		SYS_LOG_DBG("Disable all ihost interrupts except wakeup irqs");

		/* 1: Save current state of GIC IRQs
		 * This includes enable / disable state of all IRQs (PPIs & SPIs)
		 * This also includes the sys_tick PPI IRQ 29
		 */
		if(state == MPROC_PM_MODE__SLEEP)
			mproc_set_gic_regs(MPROC_ENTER_LP);

		/*2.1: disable all external interrupts on GIC,
		 * INT_SRC_MPROC_PKA_DONE ~ INT_SRC_MAX,
		 * not including PPI System Interrupts, i.e. systick
		 */
		disable_all_irqs();

		/*2.2: disable systick interrup*/
		mproc_set_systick(MPROC_ENTER_LP);

		/*2.4: clear tamper interrupt*/

		/*2.5: clear gpio interrupt*/
		/* FIXME: NEED_TBD
		 * Confirm if and why this is required?
		 * Also in Citadel we are supposed to have
		 * 10 GPIOs, so 0x3f should be 0x3ff?
		 */
		sys_write32(0xffffffff, GPIO_INT_MSK_0);
		sys_write32(0xffffffff, GPIO_INT_MSK_1);
		sys_write32(0xffffffff, GPIO_INT_MSK_2);
		sys_write32(0xffffffff, GPIO_INT_CLR_0);
		sys_write32(0xffffffff, GPIO_INT_CLR_1);
		sys_write32(0xffffffff, GPIO_INT_CLR_2);
		sys_write32(0x0, GP_INT_MSK);
		sys_write32(0x3f, GP_INT_CLR);

		/*2.8: mask all mproc interrupts */
		sys_write32(0xffffffff, CRMU_IPROC_INTR_MASK);
		sys_write32(0xffffffff, CRMU_IPROC_INTR_CLEAR);

		/*2.9: clear all pending external interrupts on GIC
		 * --- TODO --  -- **** --
		 * 1. Confirm if this clearing is to be done for GIC.
		 * clear_pend_all_irqs();
		 */

#if 0 /* NEED_TBD */
		if (state > MPROC_PM_MODE__SLEEP)
		return 0;
#endif

		/*3: only unmask user specified A7 wakeup irqs*/
		if (a7_wakeup_src__uart) {
			SYS_LOG_DBG("Unmask ihost uart irqs");
			irq_enable(SPI_IRQ(INT_SRC_UART0));
			irq_enable(SPI_IRQ(INT_SRC_UART1));
			irq_enable(SPI_IRQ(INT_SRC_UART2));
			irq_enable(SPI_IRQ(INT_SRC_UART3));
		}

		if (a7_wakeup_src__sci) {
			SYS_LOG_DBG("Unmask ihost smartcard irqs");
			irq_enable(SPI_IRQ(INT_SRC_SMART_CARD0));
			irq_enable(SPI_IRQ(INT_SRC_SMART_CARD1));
			irq_enable(SPI_IRQ(INT_SRC_SMART_CARD2));
		}

		if (a7_wakeup_src__rtc) {
			SYS_LOG_DBG("Unmask ihost rtc irqs");
			irq_enable(SPI_IRQ(INT_SRC_CRMU_MPROC_SPRU_RTC_PERIODIC));
		}

		if (a7_wakeup_src__aongpio) {
			SYS_LOG_DBG("Unmask ihost mbox irq for AON GPIO");
			/* AON_GPIO interrupt only routed to MCU, then MCU trigger a mailbox
			 * interrupt on mproc
			 */
			irq_enable(SPI_IRQ(INT_SRC_CRMU_MPROC_MAILBOX));
			sys_clear_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK);
			sys_set_bit(CRMU_IPROC_INTR_CLEAR, CRMU_IPROC_INTR_CLEAR__IPROC_MAILBOX_INTR_CLR);

			dmu_set_mcu_interrupt(MCU_AON_GPIO_INTR, 0);
			mproc_reg32_clr_masked_bits(GP_INT_MSK, aon_src); /* 0=mask, 1=unmask */
			mproc_reg32_set_masked_bits(GP_INT_CLR, aon_src); /* 1=clear */
		}

		if (a7_wakeup_src__usb) {
			SYS_LOG_DBG("Unmask ihost mbox irq for USB");

			irq_enable(SPI_IRQ(INT_SRC_CRMU_MPROC_MAILBOX));
			sys_clear_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK);
			sys_set_bit(CRMU_IPROC_INTR_CLEAR, CRMU_IPROC_INTR_CLEAR__IPROC_MAILBOX_INTR_CLR);

			dmu_set_mcu_event(MCU_USBPHY0_WAKE_EVENT, usb_src & BIT(MPROC_USB0_WAKE));
			dmu_set_mcu_event(MCU_USBPHY0_FILTER_EVENT, usb_src & BIT(MPROC_USB0_FLTR));
			dmu_set_mcu_event(MCU_USBPHY1_WAKE_EVENT, usb_src & BIT(MPROC_USB1_WAKE));
			dmu_set_mcu_event(MCU_USBPHY1_FILTER_EVENT, usb_src & BIT(MPROC_USB1_FLTR));
		}
	} else {
		/* Restore Systick
		 * FIXME - This may not be required
		 */
		mproc_set_systick(MPROC_EXIT_LP);

		/* Restore the enable / disable states of all GIC IRQs
		 * Note: This includes Systick - So the above line may not be required
		 */
		if(state == MPROC_PM_MODE__SLEEP)
			mproc_set_gic_regs(MPROC_EXIT_LP);

		/* AON_GPIO interrupt only routed to MCU, then MCU trigger a mailbox
		 * interrupt on mproc.
		 *
		 * FIXME: This is NOT required since the above takes care of this.
		 */
		irq_enable(SPI_IRQ(INT_SRC_CRMU_MPROC_MAILBOX));
		/* Should have been already done.
		 * But being extra cautious here
		 */
		sys_clear_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK);
		sys_set_bit(CRMU_IPROC_INTR_CLEAR, CRMU_IPROC_INTR_CLEAR__IPROC_MAILBOX_INTR_CLR);
	}

	return 0;
}

/**
 * @brief Init MCU ISR args zone in IDRAM
 *
 *
 * @param none
 *
 * @return result (Success / Failure)
 */
static s32_t mproc_init_m0_isr_args_to_idram(void)
{
	MCU_IDRAM_CONTENT_INFO_s sync_info;

	size_t args_size = sizeof(CITADEL_PM_ISR_ARGS_s);

	sync_info.cid = MCU_IDRAM_CONTENT__ISR_ARGS;
	mproc_pm_get_mcu_idram_content_info(&sync_info);

	if (args_size > sync_info.maxlen) {
		SYS_LOG_ERR("M0 ISR args oversize: %u, max %u.",
				args_size, sync_info.maxlen);
		return -EINVAL;
	}

	memset((u8_t *)sync_info.start_addr, 0, args_size);

	return 0;
}

/**
 * @brief copy isr args from A7 to M0
 *
 *
 * @param device
 *
 * @return result (Success / Failure)
 */
static s32_t mproc_sync_m0_isr_args_to_idram(struct device *dev)
{
	CITADEL_PM_ISR_ARGS_s *args_src = &citadel_pm_isr_args;
	MPROC_PM_MODE_e tgt_pm_state = mproc_get_target_pm_state();
	MPROC_PM_MODE_e prv_pm_state = mproc_get_current_pm_state(dev);
	MCU_IDRAM_CONTENT_INFO_s sync_info;
	u32_t soc_id;

	sync_info.cid = MCU_IDRAM_CONTENT__ISR_ARGS;
	mproc_pm_get_mcu_idram_content_info(&sync_info);

	/* ONLY init or fill all A7 controlled parameters, do NOT modify any A7
	 * readonly args
	 */
	args_src->pm_state_reserved = 0xFF; /* SBL expects 0xFF here for LP States */
	args_src->tgt_pm_state        = tgt_pm_state;
#ifdef MPROC_PM__ADV_S_POWER_OFF_PLL
	args_src->prv_pm_state        = prv_pm_state;
#endif

	memcpy(&args_src->aon_gpio_settings, &citadel_aon_gpio_wakeup_config,
		sizeof(MPROC_AON_GPIO_WAKEUP_SETTINGS_s));

	memcpy(&args_src->usb_settings, &citadel_usb_wakeup_config,
		sizeof(MPROC_USB_WAKEUP_SETTINGS_s));

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
	args_src->seconds_to_wakeup   = bcm5820x_get_rtc_wakeup_time(dev)
	* (MPROC_PM_RTC_STEP__1S / MPROC_PM_RTC_STEP);
	args_src->seconds_left        = args_src->seconds_to_wakeup;
#endif
	args_src->do_policy           = CITADEL_PM_DO_POLICY;
#ifdef MPROC_PM_ACCESS_BBL
	args_src->bbl_access_ok       = citadel_pm_get_bbl_access();
#endif
	memset(&args_src->wakeup, 0, sizeof(MPROC_PM_WAKEUP_CODE_s));
	args_src->soc_rev             = dmu_get_soc_revision(&soc_id);

	/* do copy args from ddr to idram */
	memcpy((u8_t *)sync_info.start_addr, (u8_t *)args_src,
		   sizeof(CITADEL_PM_ISR_ARGS_s));

	SYS_LOG_DBG("Sync PM ISR parameters to IDRAM at 0x%08x, size %u",
			  sync_info.start_addr, sizeof(CITADEL_PM_ISR_ARGS_s));
	return 0;
}

/**
 * @brief copy isr args from M0 to A7
 *
 * @param device
 *
 * @return result (Success / Failure)
 */
static s32_t mproc_sync_m0_isr_args_from_idram(void)
{
	CITADEL_PM_ISR_ARGS_s *args_ddr = &citadel_pm_isr_args;
	CITADEL_PM_ISR_ARGS_s *args_idram = NULL;
	MCU_IDRAM_CONTENT_INFO_s sync_info;

	SYS_LOG_DBG("Sync PM ISR parameters from IDRAM");

	sync_info.cid = MCU_IDRAM_CONTENT__ISR_ARGS;
	mproc_pm_get_mcu_idram_content_info(&sync_info);

	args_idram = (CITADEL_PM_ISR_ARGS_s *)sync_info.start_addr;

	if (!args_idram)
		return -EINVAL;

	/* copy wakeup related info */
	memcpy(&args_ddr->wakeup, &args_idram->wakeup,
		   sizeof(MPROC_PM_WAKEUP_CODE_s));

	return 0;
}

/**
 * @brief install the indirect ISR and update secondary ISR table
 *
 * @param none
 *
 * @return result (Success / Failure)
 */
static s32_t citadel_install_new_m0_isr(void)
{
	MCU_IDRAM_BIN_INFO_s *bin_info = NULL;
	u32_t res_len = 0;
	MCU_IDRAM_CONTENT_INFO_s content_info;
	u8_t *start = NULL, *target = NULL;

	/* get the M0 image info */
	bin_info = &m0_bins_info;

	/* Just one image, find the start place in the image, skip 0x100 bytes */
	bin_info->bin_start_addr += MPROC_M0_CODE_OFFSET;
	res_len  = (u32_t)bin_info->bin_end_addr - (u32_t)bin_info->bin_start_addr;

	/* check parameters */
	content_info.cid = bin_info->mcu_cid;

	if (mproc_pm_get_mcu_idram_content_info(&content_info) < 0)
	{
		SYS_LOG_ERR("Unsupported MCU ISR %d!", bin_info->bin_id);
		return -EINVAL;
	}

	if (bin_info->bin_start_addr >= bin_info->bin_end_addr
		|| !bin_info->bin_start_addr || !bin_info->bin_end_addr)
	{
		SYS_LOG_ERR("Invalid info for MCU ISR %d! bin_start_addr=0x%08x,"
		" bin_end_addr=0x%08x.", bin_info->bin_id,
		(u32_t)bin_info->bin_start_addr,
		(u32_t)bin_info->bin_end_addr);

		return -EINVAL;
	}

	/* now copy binary code into IDRAM target addr */
	start  = (u8_t *)bin_info->bin_start_addr;
	target = (u8_t *)content_info.start_addr;
	memcpy(target, start, res_len);

	/* reset the pointer address t the head of image */
	bin_info->bin_start_addr -= MPROC_M0_CODE_OFFSET;

	/* set the secondary ISR table for ISR bin
	 * Note that the ISR table m0_isr_vector is placed right
	 * at the start of the M0 bin.
	 */
	memcpy((u8_t *)CRMU_IRQx_vector_USER(0), bin_info->bin_start_addr, 32*4);

	bcm_sync();

	/* Get the xip_fast address:
	 * In the M0 ISR table, the fast XIP handler is a pseudo ISR
	 * at an invalid IRQ #32; IRQ# 0 through 31 being the valid M0
	 * IRQs. This is done as a matter of convenience in implementing
	 * the Fast XIP Work Around.
	 */
	start = bin_info->bin_start_addr + 32*4;
	xip_start = *(u32_t *)start;

	SYS_LOG_WRN("MCU %s bin %d installed at 0x%08x, size %u.",
		bin_info->bin_name, bin_info->bin_id, (u32_t)target, res_len);

	/* clear the sleep event log */
	sys_write32(0x0, CRMU_SLEEP_EVENT_LOG_0);
	sys_write32(0x0, CRMU_SLEEP_EVENT_LOG_1);

	return 0;
}

/**
 * @brief dmu test will destory the MCU ISR, so need to check all MCU ISRs
 *        installed or modified, then re-install all ISR
 *
 * @param none
 *
 * @return result (Success / Failure)
 */
static s32_t citadel_check_and_reinstall_new_m0_isr(void)
{
	s32_t ret = 0;

#ifdef CITADEL_PM_CHECK_MCU_ISR
	MCU_IDRAM_BIN_INFO_s *bin_info = NULL;
	MCU_IDRAM_BIN_TYPE_e bin_type;
	u32_t bin_id = 0, res_len = 0;
	u32_t installed_addr = 0;
	/* the first bytes must be ignored if using MCU jtag for debug */
	u32_t ignore_len = CITADEL_PM_CHECK_MCU_ISR_IGNORE_LEN;

	/* check M0 bin's integrity */
	bin_info = &m0_bins_info;
	bin_type = bin_info->bin_type;
	bin_id   = bin_info->bin_id;

	res_len  = (u32_t)bin_info->bin_end_addr - (u32_t)bin_info->bin_start_addr;

	if (bin_type == MCU_IDRAM__ISR_BIN) {
		installed_addr = sys_read32(CRMU_IRQx_vector_USER(bin_id));

		if (!installed_addr ||
			memcmp((void *)((u32_t)installed_addr + ignore_len),
				   (void *)((u32_t)bin_info->bin_start_addr + ignore_len),
				   res_len - ignore_len)) {
			SYS_LOG_INF("MCU %s bin need reinstall!", bin_info->bin_name);
			/* install all bins if any one broken */
			SYS_LOG_INF("Reinstalling MCU M0 user bin.");
			ret = citadel_install_new_m0_isr();
		}
	}
#endif

	return ret;
}

#ifdef MPROC_PM_FASTXIP_SUPPORT
/**
 * @brief PM Fast XIP Support
 *
 * @param none
 * --TODO--  --****--
 * 1. What does this function do?
 * 2. Is Flash Tag still located at offset 0x20 from FLASH device start address
 *    on citadel and is the pattern valid for citadel?
 * 3. If so, invoke the macro for start address of Flash.
 *
 * @return result (Success / Failure)
 */
static s32_t citadel_pm_fastxip_support(void)
{
	CITADEL_SOC_STATES_e soc_state = dmu_get_soc_state();
	/* tag locates at FLASH start addr with offset 0x20 */
	u32_t flash_tag = sys_read32(CRMU_M0_SCRAM_START + 0x20);
	s32_t ret = 1;

#ifdef MPROC_PM_FASTXIP_DISABLED_ON_A0
	u32_t soc_id;
	CITADEL_REVISION_e soc_rev = dmu_get_soc_revision(&soc_id);
	SYS_LOG_INF("SoC VER: %s, %s, tag 0x%08x",
			  dmu_get_soc_revision_desc(soc_rev),
			  dmu_get_soc_state_desc(soc_state), flash_tag);

	if (soc_rev < CITADEL_REV__BX) {
		SYS_LOG_ERR("FASTXIP: not supported on A0");
		ret = 0;
	}
#endif

	if (soc_state == CITADEL_STATE__unassigned && flash_tag == 0x55aa55aa) {
		SYS_LOG_ERR("FASTXIP: not supported on unassigned with magic tag");
		ret = 0;
	}

	if (soc_state == CITADEL_STATE__unprogrammed) {
		SYS_LOG_ERR("FASTXIP: not supported on unprogrammed");
		ret = 0;
	}

	return ret;
}
#endif

/******************************************************************************
  P R I V A T E  F U N C T I O N S  for Installing & Invoking ISR Callbacks
******************************************************************************/
/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev PM device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void bcm5820x_set_aon_gpio_irq_callback(struct device *dev,
		pm_aon_gpio_irq_callback_t cb, MPROC_AON_GPIO_ID_e aongpio)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->aon_gpio_irq_cb[aongpio] = cb;
}

/**
 * @brief AON GPIO IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_aon_gpio_irq_isr(void *arg, MPROC_AON_GPIO_ID_e aongpio)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->aon_gpio_irq_cb[aongpio])
		dev_data->aon_gpio_irq_cb[aongpio](dev);
}

/**
 * @brief Set the callback function - Register ISR for NFC LPTD
 *
 * @details This is for NFC LPTD events. e.g. to detect presence of
 *          a NFC device in proximity
 *
 * @param[in] dev PM device structure.
 *
 * @retval None
 */
static void bcm5820x_set_nfc_lptd_irq_callback(struct device *dev,
		pm_nfc_lptd_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->nfc_lptd_irq_cb = cb;
}

/**
 * @brief NFC LPTD IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_nfc_lptd_irq_isr(void *arg)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->nfc_lptd_irq_cb)
		dev_data->nfc_lptd_irq_cb(dev);
}

/**
 * @brief Set the callback function - Register ISR for USB
 *
 * @details This is for USB events. e.g. to detect insertion of USB device
 *          while A7 is sleeping
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 * @param[in] usbno: USB number, 0 or 1
 *
 * @retval None
 */
static void bcm5820x_set_usb_irq_callback(struct device *dev,
		pm_usb_irq_callback_t cb, MPROC_USB_ID_e usbno)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->usb_irq_cb[usbno] = cb;
}

/**
 * @brief USB IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_usb_irq_isr(void *arg, MPROC_USB_ID_e usbno)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->usb_irq_cb[usbno])
		dev_data->usb_irq_cb[usbno](dev);
}

/**
 * @brief Set the callback function - Register ISR for EC
 *  	  Command
 *
 * @details This is for EC Command events from Host.
 *
 * @param[in] dev PM device structure.
 *
 * @retval None
 */
static void bcm5820x_set_ec_cmd_irq_callback(struct device *dev,
		pm_ec_cmd_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->ec_cmd_irq_cb = cb;
}

/**
 * @brief EC Command IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_ec_cmd_irq_isr(void *arg)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->ec_cmd_irq_cb)
		dev_data->ec_cmd_irq_cb(dev);
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev PM device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void bcm5820x_set_tamper_irq_callback(struct device *dev,
		pm_tamper_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->tamper_irq_cb = cb;
}

/**
 * @brief Tamper IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_tamper_irq_isr(void *arg)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->tamper_irq_cb)
		dev_data->tamper_irq_cb(dev);
}

/**
 * @brief Set the callback function pointer for GPIO IRQ.
 *
 * @param dev PM device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void bcm5820x_set_aon_gpio_run_irq_callback(struct device *dev,
					pm_aon_gpio_run_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	dev_data->aon_gpio_run_irq_cb = cb;
}

/**
 * @brief aon gpio Interrupt service routine with system in run mode.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_aon_gpio_run_irq_isr(void *arg, u32_t int_stat)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->aon_gpio_run_irq_cb)
		dev_data->aon_gpio_run_irq_cb(dev, int_stat);
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev PM device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void bcm5820x_set_smbus_irq_callback(struct device *dev,
		pm_smbus_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->smbus_irq_cb = cb;
}

/**
 * @brief SMBUS IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_smbus_irq_isr(void *arg)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->smbus_irq_cb)
		dev_data->smbus_irq_cb(dev);
}

/**
 * @brief Set the callback function - Register ISR for RTC
 *
 * @details This is for RTC wakeup event.
 *
 * @param[in] dev PM device structure.
 * @param[in] cb: The desired call back function that user may provide
 *
 * @retval None
 */
static void bcm5820x_set_rtc_irq_callback(struct device *dev,
		pm_rtc_irq_callback_t cb)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);
	dev_data->rtc_irq_cb = cb;
}

/**
 * @brief RTC IRQ Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void bcm5820x_rtc_irq_isr(void *arg)
{
	struct device *dev = arg;
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	if (dev_data->rtc_irq_cb)
		dev_data->rtc_irq_cb(dev);
}


/**
 * @brief The main Mail Box IRQ Interrupt service routine.
 *
 * This retrievs and parses the mailbox code & then invokes corresponding
 * ISRs for different interrupts such as AON GPIO, SMBUS etc. as received
 * by the CRMU while A7 was sleeping.
 *
 * @param none.
 *
 * @return N/A
 */
static void mproc_mbox_isr(void *dev)
{
	u32_t code0 = sys_read32(CRMU_IPROC_MAIL_BOX0);
	u32_t code1 = sys_read32(CRMU_IPROC_MAIL_BOX1);

	sys_set_bit(CRMU_IPROC_INTR_CLEAR,
	CRMU_IPROC_INTR_CLEAR__IPROC_MAILBOX_INTR_CLR);

	SYS_LOG_DBG("Received MBox Interrupt 0x%08x - 0x%08x from M0.", code0, code1);

#ifdef IPROC_PM_CLEAR_AON_GPIO_INTR_IN_M0_ISR
	if (code1 == MAILBOX_CODE1__AON_GPIO_INTR_IN_RUN) {
		SYS_LOG_DBG("Clear mbox irq: aon gpio intr in RUN mode, mstat=0x%08x", code0);
		bcm5820x_aon_gpio_run_irq_isr(dev, code0);
	}
#else
	if (code0 == MAILBOX_CODE0__AON_GPIO_INTR_IN_RUN
		&& code1 == MAILBOX_CODE1__AON_GPIO_INTR_IN_RUN) {
		u32_t mstat = sys_read32(GP_INT_MSTAT);
		cpu_reg32_wr(GP_INT_CLR, mstat); /* should be cleared by gpio driver */
		bcm5820x_aon_gpio_run_irq_isr(dev, mstat);
		SYS_LOG_DBG("Clear mbox irq: aon gpio intr in RUN mode, mstat=0x%08x", mstat);
	}
#endif
	else if (code0 == MAILBOX_CODE0__WAKEUP) {
		/* SYS_LOG_DBG("Clear mbox irq: wakeup"); */
		switch(code1) {
		case MAILBOX_CODE1__WAKEUP_TEST:
			SYS_LOG_DBG("Woke Up in Test\n");
			break;
		case MAILBOX_CODE1__WAKEUP_AON_GPIO0:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO1:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO2:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO3:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO4:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO5:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO6:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO7:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO8:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO9:
		case MAILBOX_CODE1__WAKEUP_AON_GPIO10:
			SYS_LOG_DBG("Woke Up Due to AON GPIO %x\n", code1);
			bcm5820x_aon_gpio_irq_isr(dev, code1);
			break;
		case MAILBOX_CODE1__WAKEUP_NFC:
			SYS_LOG_DBG("Woke Up Due to NFC %x\n", code1);
			bcm5820x_nfc_lptd_irq_isr(dev);
			break;
		case MAILBOX_CODE1__WAKEUP_USB0_WAKE:
			SYS_LOG_DBG("Woke Up Due to USB0 Wake\n");
			bcm5820x_usb_irq_isr(dev, MPROC_USB0_WAKE);
			break;
		case MAILBOX_CODE1__WAKEUP_USB0_FLTR:
			SYS_LOG_DBG("Woke Up Due to USB0 FLTR\n");
			bcm5820x_usb_irq_isr(dev, MPROC_USB0_FLTR);
			break;
		case MAILBOX_CODE1__WAKEUP_USB1_WAKE:
			SYS_LOG_DBG("Woke Up Due to USB1 WAKE\n");
			bcm5820x_usb_irq_isr(dev, MPROC_USB1_WAKE);
			break;
		case MAILBOX_CODE1__WAKEUP_USB1_FLTR:
			SYS_LOG_DBG("Woke Up Due to USB1 FLTR\n");
			bcm5820x_usb_irq_isr(dev, MPROC_USB1_FLTR);
			break;
		case MAILBOX_CODE1__WAKEUP_EC_COMMAND:
			SYS_LOG_DBG("Woke Up Due to EC Command %x\n", code1);
			bcm5820x_ec_cmd_irq_isr(dev);
			break;
		case MAILBOX_CODE1__WAKEUP_TAMPER:
			SYS_LOG_DBG("Woke Up Due to Tamper %x\n", code1);
			bcm5820x_tamper_irq_isr(dev);
			break;
		case MAILBOX_CODE1__WAKEUP_SMBUS:
			SYS_LOG_DBG("Woke Up Due to SMBUS %x\n", code1);
			bcm5820x_smbus_irq_isr(dev);
			break;
		case MAILBOX_CODE1__WAKEUP_RTC:
			SYS_LOG_DBG("Woke Up Due to RTC Event %x\n", code1);
			bcm5820x_rtc_irq_isr(dev);
			break;
		default: break;
		}
	} else if (code0 == MAILBOX_CODE0__NOTHING && code1 == MAILBOX_CODE1__NOTHING) {
		/* SYS_LOG_DBG("Clear mbox irq: dummy"); */
	}
#ifdef MPROC_PM__A7_WDOG_RESET
	else if (code0 == MAILBOX_CODE0__SMBUS_ISR
		 && code1 == MAILBOX_CODE1__SMBUS_ISR) {
		SYS_LOG_DBG("SMBus Interrupt Received by CRMU forwarded to A7.");
		bcm5820x_smbus_irq_isr(dev);
	}
#endif
	else if (code0 == MAILBOX_CODE0__NONE_WAKEUP_AON_GPIO
		 && code1 == MAILBOX_CODE1__NONE_WAKEUP_AON_GPIO) {
		SYS_LOG_DBG("AON GPIO Interrupt Received by CRMU forwarded to A7.");
	} else if (code0 == MAILBOX_CODE0__TAMPER_INTR_IN_RUN
			   && code1 == MAILBOX_CODE1__TAMPER_INTR_IN_RUN) {
		SYS_LOG_DBG("Tamper Interrupt Received by CRMU forwarded to A7.");
		bcm5820x_tamper_irq_isr(dev);
	} else {
		/* SYS_LOG_DBG("Clear mbox irq: code0=0x%08x, code1=0x%08x", code0, code1); */
	}

	return;
}

/**
 * @brief The Installer for Mail Box IRQ Interrupt service routine.
 *
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_mailbox_isr_init(struct device *dev);

/*******************************************
 * APIs to enter Low Power States
 *******************************************/
/* Partially initialize the Console UART
 * & clear all UART interrupts
 */
#ifdef CITADEL_A7_GPIO_RING_POWER_OFF
#define UART_REG_ADDR_INTERVAL 4
#define REG_RDR 0x00  /* Receiver data reg.       */
#define REG_IIR 0x02  /* Interrupt ID reg.        */
#define REG_FCR 0x02  /* FIFO control reg.        */
#define REG_LCR 0x03  /* Line control reg.        */
#define REG_LSR 0x05  /* Line status reg.         */
#define REG_MSR 0x06  /* Modem status reg.        */

extern void uart_console_isr(struct device *unused);
#endif

static void reinit_console_uart(void)
{
#ifdef CITADEL_A7_GPIO_RING_POWER_OFF
	u8_t msr, iir, rdr, lsr;
	static struct device *uart_console_dev;

	/* To Suppress Compiler Error */
	ARG_UNUSED(msr);
	ARG_UNUSED(iir);
	ARG_UNUSED(rdr);
	ARG_UNUSED(lsr);

	/* Somehow when the IO Ring is powered down in DRIPS,
	 * the UART Rx Interrupt doesn't work upon Exit. This makes
	 * the Console (of for that matter any other application
	 * that may be using the UART Rx Interrupt) unusable.
	 *
	 * Partially Initializing the UART, i.e.
	 * re-enabling the Interrupt, makes it work.
	 */
	uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	uart_irq_rx_disable(uart_console_dev);
	uart_irq_tx_disable(uart_console_dev);
	uart_irq_rx_enable(uart_console_dev);

	/* However, there seem to be some UART interrupt pending on the
	 * IIR (not necessarily Rx Interrupts), when we are at this point.
	 * So as soon as we enable IRQ, we keep getting this interrupt.
	 * Unfortunately it doesn't get fully cleared by the ISR so the
	 * interrupt keeps triggering continuously. Here clearing these
	 * interrupts explicitly (By Reading out RDR, MSR etc.) to prevent
	 * this situation.
	 */
	rdr = sys_read32(CONFIG_UART_CONSOLE_PORT_ADDR + REG_RDR * UART_REG_ADDR_INTERVAL);
	iir = sys_read32(CONFIG_UART_CONSOLE_PORT_ADDR + REG_IIR * UART_REG_ADDR_INTERVAL);
	lsr = sys_read32(CONFIG_UART_CONSOLE_PORT_ADDR + REG_LSR * UART_REG_ADDR_INTERVAL);
	msr = sys_read32(CONFIG_UART_CONSOLE_PORT_ADDR + REG_MSR * UART_REG_ADDR_INTERVAL);
	SYS_LOG_DBG("UART Status After DRIPS: MSR: %x, IIR: %x, RDR: %x, LSR: %x\n", msr, iir, rdr, lsr);
#endif
}

/**
 * @brief enter default state.
 * this routine could be called at startup without scheduler started
 * but with the flag init_phase set to non-zero value.
 *
 * @param device
 * @param init_phase: Whether this is being invoked during device Initialisation
 *
 * @return result
 */
static s32_t bcm5820x_enter_default_state(struct device *dev, u32_t init_phase)
{
	MPROC_PM_MODE_e state = MPROC_DEFAULT_PM_MODE;

	mproc_set_target_pm_state(dev, state);

	/* Disable local CPSR IRQs only if not being
	 * invoked during device initialization.
	 */
	if(init_phase == 0)
		local_irq_disable();

	mproc_set_current_pm_state(dev, state);

	dmu_set_mcu_access_permission(2);

	dmu_clear_mcu_event(MCU_MAILBOX_EVENT);

	dmu_set_mcu_event(MCU_MAILBOX_EVENT, MPROC_ENABLED);

	mproc_sync_m0_isr_args_to_idram(dev);

	mproc_send_mailbox_msg_to_mcu(state);

	mproc_sync_m0_isr_args_from_idram();

	/* Re-enable local CPSR IRQs only if not being
	 * invoked during device initialization.
	 */
	if(init_phase == 0)
		local_irq_enable();

	return 0;
}

/**
 * @brief enter target state.
 * this routine could be called at anytime with scheduler started
 *
 * -- TODO --  -- *****--
 * This is the main function that enables low power modes.
 * Note: Warning!! This function ordinarily DOESN'T return!!
 * We return and resume execution of this function only after
 * waking up from Sleep / DRIPS modes.
 *
 * @param device
 * @param new state
 *
 * @return result
 */
static s32_t bcm5820x_enter_target_state(struct device *dev,
			MPROC_PM_MODE_e new_state)
{
	u32_t win;
	CITADEL_PM_ISR_ARGS_s *pm_args = &citadel_pm_isr_args;
	MPROC_PM_MODE_e old_state = mproc_get_current_pm_state(dev);
	MPROC_CONSOLE_BAUDRATE_e cur_baudrate = 0, new_baudrate = 0;
	u32_t lpstate;
	u32_t wakeup_code0 = MAILBOX_CODE0__WAKEUP;
	u32_t wakeup_code1 = MAILBOX_CODE1__WAKEUP_TEST;

	if (new_state >= MPROC_PM_MODE__END) {
		SYS_LOG_ERR("Invalid pm state!");
		return -EINVAL;
	}

	SYS_LOG_DBG("Current mode: %s", mproc_get_pm_state_desc(dev, old_state));
	SYS_LOG_DBG("Target  mode: %s", mproc_get_pm_state_desc(dev, new_state));
	if (new_state == old_state) {
		SYS_LOG_ERR("Already in %s",
			  mproc_get_pm_state_desc(dev, new_state));
		return 0;
	}

	/* FIXME: Consider adding 200MHz & 1000MHz as separate modes
	 * in MPROC_CPU_PW_STAT_e, so that the current run frequency
	 * could be reflected in the persistent register
	 * CRMU_IHOST_POWER_CONFIG.
	 *---------------------- LP_Run200 Mode --------------------------
	 */
	if(new_state == MPROC_PM_MODE__RUN_200) {
		/* Set the CPU Clock to 208MHz. */
		if (clk_a7_set(MHZ(208))) {
			SYS_LOG_ERR("Cannot set clock speed to 200MHz.");
			return -EIO;
		}
		mproc_set_current_pm_state(dev, new_state);
		citadel_pm_set_low_power_flag(MPROC_PM_MODE__RUN_200);
		return 0;
	}

	/*---------------------- LP_Run500 Mode --------------------------*/
	if(new_state == MPROC_PM_MODE__RUN_500) {
		/* Set the CPU Clock to 500MHz. */
		if (clk_a7_set(MHZ(500))) {
			SYS_LOG_ERR("Cannot set clock speed to 500MHz.");
			return -EIO;
		}
		mproc_set_current_pm_state(dev, new_state);
		citadel_pm_set_low_power_flag(MPROC_PM_MODE__RUN_500);
		return 0;
	}

	/*--------------------- LP_Run1000 Mode -------------------------- */
	if(new_state == MPROC_PM_MODE__RUN_1000) {
		/* Set the CPU Clock to 1000MHz. */
		if (clk_a7_set(MHZ(1000))) {
			SYS_LOG_ERR("Cannot set clock speed to 1000MHz.");
			return -EIO;
		}
		mproc_set_current_pm_state(dev, new_state);
		citadel_pm_set_low_power_flag(MPROC_PM_MODE__RUN_1000);
		return 0;
	}

	/* Show warning if console baudrate will be changed in new state */
	if (new_state < MPROC_PM_MODE__END) {
		cur_baudrate = mproc_get_console_baudrate(old_state);
		new_baudrate = mproc_get_console_baudrate(new_state);

		if (new_baudrate != MPROC_CONSOLE_BAUDRATE__UNCHANGE
			&& new_baudrate != cur_baudrate) {
			SYS_LOG_ERR(""
			"*******************************************************"
			"!!! WARNING: PLEASE RE-CONNECT WITH BAUDRATE %d !!!"
			"*******************************************************"
			"", new_baudrate);
		}
	}

	if (citadel_check_and_reinstall_new_m0_isr() < 0) {
		SYS_LOG_ERR("Failed to reinstall MCU ISR!");
		return -EINVAL;
	}
	mproc_set_target_pm_state(dev, new_state);
	if (new_state >= MPROC_PM_MODE__SLEEP)
		mproc_show_available_wakeup_srcs(dev, new_state);

	/* Must Disable IRQ in CPSR before starting preparation for entering
	 * Low Power Mode.
	 */
	local_irq_disable();

	switch(new_state) {
	/*----------------------------------------------------------------
	 *--------------- Simple WFI Sleep Mode --------------------------
	 *----------------------------------------------------------------
	 */
	case MPROC_PM_MODE__SLEEP:
		/* only SLEEP needs to save & restore platform regs
		 * FIXME: Is this really Required??
		 */
		mproc_set_platform_regs(MPROC_ENTER_LP);

		/* This is disabling/enabling GIC IRQs and the Sys Tick IRQ.
		 * We are doing this for two reasons:
		 * 1. While going through steps to gate the CPU clocks
		 * (in Sleep) we would like to avoid any unwanted interrupts
		 * including systicks or other spurious interrupts.
		 *
		 */
		SYS_LOG_DBG("Disabling all GIC IRQs except those for WakeUp.\n");
		mproc_set_ihost_wakeup_irqs(new_state, MPROC_ENTER_LP);

		/* FIXME: What is the purpose of this?
		 * CRMU regs can be accessed just fine without this.
		 */
		SYS_LOG_DBG("Set MCU Access Permission.\n");
		dmu_set_mcu_access_permission(2);

		SYS_LOG_DBG("Sync M0 ISR args.\n");
		mproc_sync_m0_isr_args_to_idram(dev);

		/* FIXME: Is this global state required?? */
		mproc_set_current_pm_state(dev, new_state);
		SYS_LOG_DBG("Set LP Flag with New State.\n");
		if (citadel_pm_set_low_power_flag(new_state) < 0)
			return -EINVAL;

		SYS_LOG_DBG("Ready to enter LP state: %d\n", new_state);
		SYS_LOG_DBG("~WFI for Sleep or Deep Sleep~");

		k_busy_wait(1000); /* wait till all prints are done */
		mproc_send_mailbox_msg_to_mcu(new_state);
		bcm_sync();
		wfi();

		/* 1. Wakeup from RUN or SLEEP will resume from here.
		 * 2. Wakeup from Deep Sleep will be a direct reboot
		 * 3. Wakeup from DRIPS will be a direct reboot but
		 * control will transfer to the jump address which is
		 * cpu_powerup.
		 */
		mproc_sync_m0_isr_args_from_idram();

		citadel_pm_set_low_power_flag(MPROC_DEFAULT_PM_MODE);

		/* FIXME: Is this global state required?? */
		mproc_set_current_pm_state(dev, old_state);

		/* This is restoring the disabled/enabled states of GIC IRQs
		 * that was saved earlier.
		 */
		SYS_LOG_DBG("Restoring GIC state to what was before Sleep.\n");
		mproc_set_ihost_wakeup_irqs(new_state, MPROC_EXIT_LP);

		/* only SLEEP needs to save & restore platform regs
		 * FIXME: Is this really Required??
		 */
		mproc_set_platform_regs(MPROC_EXIT_LP);
		break;

	/*----------------------------------------------------------------
	 *--------------------- Deep Sleep Mode --------------------------
	 *----------------------------------------------------------------
	 */
	case MPROC_PM_MODE__DEEPSLEEP:
#ifdef MPROC_PM_FASTXIP_SUPPORT
		/* only deepsleep needs to save smau and other settings */
		if (citadel_pm_fastxip_support()) {
			/* save smau info */
			/* reset_handler defined in citadel_*.ld,
			 * aai=0x03010100, others=0x03000000
			 * --- TODO --  Confirm this flow
			 *
			 * Saving SMAU Window 0 Configuration since we are
			 * building at 0x63000400
			 */
			for(win = 0; win < SMU_CFG_NUM_WINDOWS; win++)
				mproc_save_smau_setting((u32_t)&__start, win);

			/* TODO: save flash,chipid,usb ???? */
			SYS_LOG_DBG("FastXIP: enabled");
		} else
			SYS_LOG_DBG("FastXIP: disabled");
#endif

		/* FIXME: What is the purpose of this?
		 * CRMU regs can be accessed just fine without this.
		 */
		SYS_LOG_DBG("Set MCU Access Permission.\n");
		dmu_set_mcu_access_permission(2);
		SYS_LOG_DBG("Sync M0 ISR args.\n");
		mproc_sync_m0_isr_args_to_idram(dev);
		/* FIXME: Is this global state required?? */
		mproc_set_current_pm_state(dev, new_state);
		SYS_LOG_DBG("Set LP Flag with New State.\n");
		if (citadel_pm_set_low_power_flag(new_state) < 0)
			return -EINVAL;

		SYS_LOG_WRN("Ready to enter LP state: %d\n", new_state);
		SYS_LOG_WRN("~WFI for Sleep or Deep Sleep~");

		k_busy_wait(1000); /* wait till all prints are done */
		mproc_send_mailbox_msg_to_mcu(new_state);
		bcm_sync();
		/* Enter Infinite Loop
		 * A7 will be powered down when M0 turns PD_SYS OFF.
		 */
		while(1)
			;
		/* Deep Sleep Returns in POR path.
		 * So the following code will never be executed.
		 */
		return 0;

	/*----------------------------------------------------------------
	 *----- Deepest Run-Time Idle Power State Sleep Mode (DRIPS) -----
	 *----------------------------------------------------------------
	 */
	case MPROC_PM_MODE__DRIPS:
#ifdef MPROC_PM_FASTXIP_SUPPORT
		/* only deepsleep needs to save smau and other settings */
		if (citadel_pm_fastxip_support()) {
			/* save smau info */
			/* reset_handler defined in citadel_*.ld,
			 * aai=0x03010100, others=0x03000000
			 * --- TODO --  Confirm this flow
			 *
			 * Saving SMAU Window 0 Configuration since we are
			 * building at 0x63000400
			 */
			for(win = 0; win < SMU_CFG_NUM_WINDOWS; win++)
				mproc_save_smau_setting((u32_t)cpu_wakeup, win);

			/* TODO: save flash,chipid,usb ???? */
			SYS_LOG_DBG("FastXIP: enabled");
		} else
			SYS_LOG_DBG("FastXIP: disabled");
#endif
		/* 2. Set MCU Access Permission
		 * FIXME: What is the purpose of this?
		 * CRMU regs can be accessed just fine without this.
		 */
		SYS_LOG_DBG("Set MCU Access Permission.\n");
		dmu_set_mcu_access_permission(2);
		/* 3. Sync ISR Arguments */
		SYS_LOG_DBG("Sync M0 ISR args.\n");
		mproc_sync_m0_isr_args_to_idram(dev);
		/* 4. Set Current State (Global)
		 * FIXME: Is this global state required??
		 */
		mproc_set_current_pm_state(dev, new_state);
		/* 5. Set the Low Power Mode Flag
		 * This is used to indicate whether DRIPS or Deep Sleep.
		 */
		SYS_LOG_DBG("Set LP Flag with New State.\n");
		if (citadel_pm_set_low_power_flag(new_state) < 0)
			return -EINVAL;

		/* 7. Clear all interrupts from M0 to A7
		 * Before entering LP Mode
		 */
		sys_write32(0xffffffff, CRMU_IPROC_INTR_CLEAR);

		/* 8. Mask all events from A7 to M0 except the mailbox event
		 * that A7 will be sending to M0 to request M0 to turn A7's
		 * PD_CPU OFF, i.e. request to enter DRIPS state.
		 */
		sys_write32(0xffffffff, CRMU_MCU_EVENT_MASK);
		sys_clear_bit(CRMU_MCU_EVENT_MASK, CRMU_MCU_EVENT_MASK__MCU_MAILBOX_EVENT_MASK);

		SYS_LOG_WRN("Ready to enter LP state: %d\n", new_state);
		cpu_power_down();
		lpstate = sys_read32(DRIPS_OR_DEEPSLEEP_REG);
		if((lpstate & ENTRY_EXIT_MASK_SET) == ENTERING_LP) {
			sys_write32(MAILBOX_CODE0__DRIPS, IPROC_CRMU_MAIL_BOX0);
			sys_write32(MAILBOX_CODE1__PM, IPROC_CRMU_MAIL_BOX1);
			__asm__ volatile("dsb");
			__asm__ volatile("isb");
			/* Enter Infinite Loop
			 * A7 will be powered down when M0 turns PD_CPU OFF.
			 */
			while(1)
				;
		}

		/* 11. Sync ISR Args from IDRAM
		 * FIXME: Is this necessary?
		 */
		mproc_sync_m0_isr_args_from_idram();

		/* 12. Restore PM State */
		citadel_pm_set_low_power_flag(MPROC_DEFAULT_PM_MODE);
		/* 13. Set Current PM State Global
		 * FIXME: Is this global state required??
		 */
		mproc_set_current_pm_state(dev, old_state);

#ifdef MPROC_PM__ULTRA_LOW_POWER
		/* Restore the CPU Clock back to what it was before entering DRIPS. */
		if(old_state == MPROC_PM_MODE__RUN_200) {
			if (clk_a7_set(MHZ(200))) {
				SYS_LOG_ERR("Cannot set clock speed to 200MHz.");
				return -EIO;
			}
		}
		else if(old_state == MPROC_PM_MODE__RUN_500) {
			if (clk_a7_set(MHZ(500))) {
				SYS_LOG_ERR("Cannot set clock speed to 500MHz.");
				return -EIO;
			}
		}
		else if (clk_a7_set(MHZ(1000))) {
			SYS_LOG_ERR("Cannot set clock speed to 1GHz.");
			return -EIO;
		}
#endif

#ifndef SAVE_RESTORE_GENERIC_TIMER
		/* 14. Initialize the Generic Timer */
		_sys_clock_driver_init(NULL);
#endif

		/* Partially initialize the Console UART
		 * & clear all UART interrupts
		 */
		reinit_console_uart();

#ifdef MPROC_PM_ACCESS_BBL
		/* 16. Enable BBL Access. Seems like Low Power state of
		 * DRIPS doesn't retain the BBL access and the exit
		 * sequence by M0 does not restore it.
		 *
		 * So restore BBl Access here.
		 * FIXME: Consider moving this step to M0 code if possible.
		 */
		bbl_access_enable();
#endif
		break;

	/* All Other Cases */
	default:
		break;
	}

	/* At this point, we have exited LP Mode DRIPS.
	 * Must Re-anble IRQ.
	 *
	 * Note: Systick will start only after this.
	 */
	local_irq_enable();

	if (new_state >= MPROC_PM_MODE__SLEEP) {
		SYS_LOG_WRN("Wakeup from: %s",
			mproc_get_pm_state_desc(dev, new_state));
		SYS_LOG_WRN("Wakeup reason: %s",
			mproc_get_pm_wakeup_desc(pm_args->wakeup.wakeup_src));

		if (pm_args->wakeup.wakeup_src == ws_aon_gpio) {
			u32_t aon_gpio_intr, i;
			SYS_LOG_WRN("(0x%02x)", pm_args->wakeup.info.aon_gpio.src);
			aon_gpio_intr = pm_args->wakeup.info.aon_gpio.src;
			/* Pick the first AON GPIO that has caused this wake-up
			 * and prepare code1 of the mailbox message.
			 */
			for(i = 0; i < MPROC_AON_GPIO_END; i++) {
				if(aon_gpio_intr & BIT(i)) {
					wakeup_code1 = MAILBOX_CODE1__WAKEUP_AON_GPIO0 + i;
					break;
				}
			}
		}
		else if (pm_args->wakeup.wakeup_src == ws_usb) {
			u32_t usb_intr, i;
			SYS_LOG_WRN("(0x%02x)", pm_args->wakeup.info.usbno.usbno);
			usb_intr = pm_args->wakeup.info.usbno.usbno;
			/* Pick the first USB event that has caused this wake-up
			 * and prepare code1 of the mailbox message.
			 */
			for(i = 0; i < MPROC_USB_END; i++) {
				if(usb_intr & BIT(i)) {
					wakeup_code1 = MAILBOX_CODE1__WAKEUP_USB0_WAKE + i;
					break;
				}
			}
		}

#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
		else if (pm_args->wakeup.wakeup_src == ws_tamper) {
			SYS_LOG_WRN("(Tamper Timestamp 0x%08x, src 0x%08x, src1 0x%08x)",
			pm_args->wakeup.info.tamper.time,
			pm_args->wakeup.info.tamper.src,
			pm_args->wakeup.info.tamper.src1);
		}
#endif
#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
		else if (pm_args->wakeup.wakeup_src == ws_rtc) {
			SYS_LOG_WRN("(%d seconds)", bcm5820x_get_rtc_wakeup_time(dev));
		}
#endif
		else {
			SYS_LOG_WRN("(unknown)");
		}

		/* Send message to MPROC(A7) to trigger the message handler
		 * FIXME: This is a work around, but it still doesn't work
		 * properly. The messaging from M0 to A7 has to be investigated
		 * thoroughly as it is found to be unreliable.
		 */
		sys_write32(wakeup_code0, CRMU_IPROC_MAIL_BOX0);
		sys_write32(wakeup_code1, CRMU_IPROC_MAIL_BOX1);
	}

	SYS_LOG_WRN("Current mode: %s", mproc_get_pm_state_desc(dev,
		mproc_get_current_pm_state(dev)));

	return 0;
}

/******************************************
 * Setting / Configuring Wake Up Sources
 *******************************************/

/**
 * @brief Set AON GPIO sources for Wake Up
 *
 * @details Configure AON GPIO sources for Wake Up
 *
 * @param[in] dev PM device structure.
 * @param[in] aongpio the desired GPIO number
 * @param[in] int_type 1=level, 0=edge
 * @param[in] int_de 1=both edge, 0=single edge
 * @param[in] int_edge 1=rising edge or high level, 0=falling edge or low level
 * @param[in] enable 1=enable the aongpio as a wakeup source,
 *  	 0=disable aongpio as a wakeup source.
 *
 * @retval
 */
static u32_t bcm5820x_set_aongpio_wakeup_source(struct device *dev,
		MPROC_AON_GPIO_ID_e aongpio, u32_t int_type, u32_t int_de,
		u32_t int_edge, u32_t enable)
{
	struct bcm5820x_pm_dev_data_t *const dev_data = DEV_DATA(dev);

	/* FIXME:
	 * Debug in Progress
	 * remove direct update this private structure
	 * and update only through the driver data pointer
	 */
	citadel_aon_gpio_wakeup_config.wakeup_src &= ~(0x1 << aongpio);
	dev_data->aon_gpio_wakeup_config->wakeup_src &= ~(0x1 << aongpio);
	if (enable)
	{
		citadel_aon_gpio_wakeup_config.wakeup_src |= (0x1 << aongpio);
		dev_data->aon_gpio_wakeup_config->wakeup_src |= (0x1 << aongpio);
		if(int_type) {
			citadel_aon_gpio_wakeup_config.int_type |= (0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_type |= (0x1 << aongpio);
		} else {
			citadel_aon_gpio_wakeup_config.int_type &= ~(0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_type &= ~(0x1 << aongpio);
		}
		if(int_de) {
			citadel_aon_gpio_wakeup_config.int_de |= (0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_de |= (0x1 << aongpio);
		} else {
			citadel_aon_gpio_wakeup_config.int_de &= ~(0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_de &= ~(0x1 << aongpio);
		}
		if(int_edge) {
			citadel_aon_gpio_wakeup_config.int_edge |= (0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_edge |= (0x1 << aongpio);
		} else {
			citadel_aon_gpio_wakeup_config.int_edge &= ~(0x1 << aongpio);
			dev_data->aon_gpio_wakeup_config->int_edge &= ~(0x1 << aongpio);
		}
	}

	return 0;
}

/**
 * @brief Set NFC LPTD as a Wake Up Source
 *
 * @details Set NFC LPTD as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval
 */
static u32_t bcm5820x_set_nfc_lptd_wakeup_source(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief Set USB as a Wake Up Source
 *
 * @details Configure USB port as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 * @param[in] usbno 0: USB0, 1: USB1
 *
 * @retval
 */
static u32_t bcm5820x_set_usb_wakeup_source(struct device *dev, MPROC_USB_ID_e usbno)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(usbno);
	return 0;
}

/**
 * @brief Set EC Command as a Wake Up Source
 *
 * @details Set EC Command as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval
 */
static u32_t bcm5820x_set_ec_cmd_wakeup_source(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief Set Tamper as a Wake Up Source
 *
 * @details Set Tamper as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval
 */
static u32_t bcm5820x_set_tamper_wakeup_source(struct device *dev)
{
	u32_t ret = 0;
	u32_t numattempts = 10;
	u32_t delay = 100;
	ARG_UNUSED(dev);

#ifdef MPROC_PM_USE_TAMPER_AS_WAKEUP_SRC
	if (citadel_pm_get_bbl_access()) {
		u32_t bbl_int_stat, bblinten;
		u32_t tmprsrc, tmprsrc1, tmprstat, tmprstat1;
		/* 9. Unmask desired events / Interrupt from CRMU to A7 so that
		 * any event/interrupt that happens while we are in LP mode
		 * can be registered and pended on GIC. Respective ISRs can be executed
		 * afterwards.
		 *
		 * Unmasking Tamper Interrupt
		 * FIXME: Clear the tamper sources. It is noticed that even though
		 * the A7 tamper handler is expected to clear the tamper sources
		 * the tamper event keeps triggering the M0 ISR (once a legitimate
		 * tamper has happened). So to make extra sure clear the tamper source
		 * as well as BBL interrupt.
		 */
		/* 1. Clear BBL Interrupt Source Status Register */
		bbl_read_reg(BBL_TAMPER_SRC_STAT, &tmprstat);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0xffffffff);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0);

		bbl_read_reg(BBL_TAMPER_SRC_STAT_1, &tmprstat1);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0xffffffff);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0);
		SYS_LOG_WRN("1.BBL Tamper Source Status: %x, Source Status1: %x", tmprstat, tmprstat1);
		bbl_read_reg(BBL_TAMPER_SRC_STAT, &tmprstat);
		bbl_read_reg(BBL_TAMPER_SRC_STAT_1, &tmprstat1);
		SYS_LOG_WRN("2.BBL Tamper Source Status: %x, Source Status1: %x", tmprstat, tmprstat1);

		/* Unmask the SPRU Alarm (tamper) Interrupt from CRMU(M0) to IPROC(A7) */
		sys_clear_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_SPRU_ALARM_INTR_MASK);
		/* Mask the RTC Interrupt from CRMU(M0) to IPROC(A7) */
		sys_set_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_PERIODIC_INTR_MASK);
		/* Mask the SPRU Alarm event for M0.
		 * Note: This should not be required. But Sometimes we would like to
		 * keep this event for M0 masked until we enter the LP mode, as a part
		 * of which, during preparing wake-up sources, M0 will unmask this
		 * event.
		 */
		sys_set_bit(CRMU_MCU_EVENT_CLEAR, MCU_SPRU_ALARM_EVENT);
		sys_set_bit(CRMU_MCU_EVENT_MASK, MCU_SPRU_ALARM_EVENT);

		bbl_read_reg(BBL_INTERRUPT_EN, &bblinten);
		bbl_read_reg(BBL_INTERRUPT_stat, &bbl_int_stat);
		SYS_LOG_WRN("BBL Interrupt Enable: %x & Sts: %x", bblinten, bbl_int_stat);
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE, &tmprsrc);
		bbl_read_reg(BBL_TAMPER_SRC_ENABLE_1, &tmprsrc1);
		SYS_LOG_WRN("BBL Tamper Source Enable: %x, Source Enable1: %x", tmprsrc, tmprsrc1);
		/* 1. Clear BBL Interrupt Status Register */
		bbl_write_reg(BBL_INTERRUPT_clr, 0xfc);
		/* Loop on the status - looking for '0' */
		do {
			bbl_read_reg(BBL_INTERRUPT_stat, &bbl_int_stat);
			k_busy_wait(delay);
			SYS_LOG_WRN("BBL Interrupt Status: %x", bbl_int_stat);
			numattempts--;
		} while((bbl_int_stat & 0xfc) && numattempts);
		if (numattempts == 0)
		{
			SYS_LOG_ERR("Cannot clear BBL Interrupt Status!");
			ret = -EINVAL;
		}
	}
	else
	{
		SYS_LOG_ERR("Cannot access BBL! Tamper wakeup ignored!");
		ret = -EINVAL;
	}
#endif

	return ret;
}

/**
 * @brief Set smbus as a Wake Up Source
 *
 * @details Set smbus as a Wake Up Source
 *
 * @param[in] dev PM device structure.
 *
 * @retval
 */
static u32_t bcm5820x_set_smbus_wakeup_source(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief set RTC wakeup Time.
 *
 *
 * @param device
 * @param wakeup time in seconds
 *
 * @return result
 */
static u32_t bcm5820x_set_rtc_wakeup_source(struct device *dev, u32_t sec)
{
	u32_t ret = 0;
	u32_t numattempts = 10;
	u32_t delay = 100;
	ARG_UNUSED(dev);

#ifndef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
	ARG_UNUSED(sec);
#else
	citadel_rtc_wakeup_time = sec;
	ret = sec;
	if (citadel_pm_get_bbl_access()) {
		u32_t bbl_int_stat, bblinten;
		bbl_read_reg(BBL_INTERRUPT_EN, &bblinten);
		bbl_read_reg(BBL_INTERRUPT_stat, &bbl_int_stat);
		SYS_LOG_WRN("BBL Interrupt Enable: %x & Sts: %x", bblinten, bbl_int_stat);
		/* 1. Clear BBL Interrupt Status Register.
		 * FIXME: Consider the following steps to remedy instability:
		 * (i) clearing all BBL interrupts, before entering LP mode.
		 * (ii) Clear all tamper sources
		 * (iii) Disable all tamper sources
		 * (iv) Clear the M0 SPRU ALARM event
		 * (v) Disable the M0 SPRU ALARM event
		 *
		 * Note 1: The above two steps allows RTC to function properly.
		 * However, sometimes tamper signal fails to get generated.
		 *
		 * Note 2: The writing of '0' to the Tamper Source Clear Registers
		 * is done for the following behavior of BBL. Writing '1' to a bit
		 * in this register clears corresponding pending BBL Tamper event,
		 * if the tamper event is already pending. However, if there is no
		 * tamper event pending, then this '1' clears the next tamper event
		 * that happens in future silently. In such scenario the tamper event
		 * doesn't get propagated down the chain to the Interrupt controller.
		 */
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0xffffffff);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR, 0);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0xffffffff);
		bbl_write_reg(BBL_TAMPER_SRC_CLEAR_1, 0);

		sys_set_bit(CRMU_MCU_EVENT_CLEAR, MCU_SPRU_ALARM_EVENT);
		sys_set_bit(CRMU_MCU_EVENT_MASK, MCU_SPRU_ALARM_EVENT);

		/* Clear any pending RTC interrupt. */
		bbl_write_reg(BBL_INTERRUPT_clr, (0x1 << BBL_INTERRUPT_clr__bbl_period_intr_clr));
		k_busy_wait(delay);
		/* Loop on the status - looking for '0' */
		do {
			bbl_read_reg(BBL_INTERRUPT_stat, &bbl_int_stat);
			SYS_LOG_WRN("BBL Interrupt Status: %x", bbl_int_stat);
			numattempts--;
		} while((bbl_int_stat & (0x1 << BBL_INTERRUPT_clr__bbl_period_intr_clr)) && numattempts);
		if (numattempts == 0)
		{
			SYS_LOG_ERR("Cannot clear BBL Interrupt Status!");
			ret = -EINVAL;
		}

		/* 9. Unmask desired events / Interrupt from CRMU to A7 so that
		 * any event/interrupt that happens while we are in LP mode
		 * can be registered and pended on GIC. Respective ISRs can be executed
		 * afterwards.
		 *
		 * Unmasking RTC Periodic Interrupt
		 */
		sys_clear_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_SPRU_RTC_PERIODIC_INTR_MASK);
		/* Mask the SPRU Alarm (tamper) Interrupt from CRMU(M0) to IPROC(A7) */
		sys_set_bit(CRMU_IPROC_INTR_MASK, CRMU_IPROC_INTR_MASK__IPROC_SPRU_ALARM_INTR_MASK);
		/* Mask the SPRU Alarm event for M0.
		 * Note: This should not be required. But Sometimes we would like to
		 * keep this event for M0 masked until we enter the LP mode, as a part
		 * of which, during preparing wake-up sources, M0 will unmask this
		 * event.
		 */
		sys_set_bit(CRMU_MCU_EVENT_CLEAR, MCU_SPRU_ALARM_EVENT);
		sys_set_bit(CRMU_MCU_EVENT_MASK, MCU_SPRU_ALARM_EVENT);
	} else {
		SYS_LOG_ERR("Cannot access BBL! RTC wakeup ignored!");
	}
#endif

	return ret;
}

/**
 * @brief Power Manager Miscellaneous Init
 *
 * -- TODO -- *****--
 * What is this function all about??
 *
 * @param device
 *
 * @return result
 */

static void citadel_pm_misc_init(void)
{
#ifdef MPROC_PM__A7_WDOG_RESET
	dmu_clear_mcu_event(MCU_MPROC_CRMU_EVENT3);
	dmu_set_mcu_event(MCU_MPROC_CRMU_EVENT3, MPROC_ENABLED);
#endif
#ifdef MPROC_PM__A7_SMBUS_ISR
	dmu_clear_mcu_interrupt(MCU_SMBUS_INTR);
	dmu_set_mcu_interrupt(MCU_SMBUS_INTR, MPROC_ENABLED);
#endif
#ifdef MPROC_PM_TRACK_MCU_STATE
	sys_write32(MPROC_MCU_LOG_START_ADDR, MPROC_MCU_LOG_PTR);
#endif
}

/*****************************************************************************
************************* P U B L I C   A P I s ******************************
*****************************************************************************/

/*******************************************
 * Utilitarian APIs
 *******************************************/
/**
 * @brief get current PM Mode / state.
 *
 *
 * @param device.
 *
 * @return N/A
 */
MPROC_PM_MODE_e mproc_get_current_pm_state(struct device *dev)
{
	ARG_UNUSED(dev);
	return citadel_cur_pm_state;
}

/**
 * @brief get description of PM Mode / state.
 *
 *
 * @param device
 * @param state
 *
 * @return string containing description
 */
char *mproc_get_pm_state_desc(struct device *dev, MPROC_PM_MODE_e state)
{
	ARG_UNUSED(dev);

	static char *pm_state_desc[] = {
		[MPROC_PM_MODE__RUN_1000]  = "run1000",
		[MPROC_PM_MODE__RUN_500]  = "run500",
		[MPROC_PM_MODE__RUN_200]   = "run200",
#ifdef MPROC_PM__DPA_CLOCK
		[MPROC_PM_MODE__RUN_187]   = "run187",
#endif
		[MPROC_PM_MODE__SLEEP]     = "sleep",
		[MPROC_PM_MODE__DRIPS]     = "DRIPS",
		[MPROC_PM_MODE__DEEPSLEEP] = "deepsleep",
		[MPROC_PM_MODE__END]       = "UNKNOWN",
	};

	if (state > MPROC_PM_MODE__END)
		return NULL;

	return pm_state_desc[state];
}

/**
 * @brief get RTC wakeup Time.
 *
 *
 * @param device
 *
 * @return result
 */
u32_t bcm5820x_get_rtc_wakeup_time(struct device *dev)
{
	ARG_UNUSED(dev);
	u32_t ret = 0;

#ifdef MPROC_PM_USE_RTC_AS_WAKEUP_SRC
	ret = citadel_rtc_wakeup_time;
#endif

	return ret;
}

/**
 * @brief Power Manager Init
 *  Main Initialization routine for the Power manager Device Instant.
 *
 * -- TODO -- *****--
 * Check is the sequence is valid for Citadel.
 * Is BBL access there?
 *
 * @param device
 *
 * @return result
 */
s32_t bcm5820x_pm_init(struct device *dev)
{
	SYS_LOG_DBG("Initializing Power Manager...");
	mproc_init_m0_isr_args_to_idram();

	if (citadel_install_new_m0_isr() < 0) {
		SYS_LOG_ERR("Failed to init PM!");
		return -EINVAL;
	}

	bcm5820x_mailbox_isr_init(dev);

	/* Enable:
	 * 1. A7 WDOG Reset in CRMU_MCU_EVENT
	 * 2. A7 SMBUs Error in CRMU_MCU_INTR
	 * 3. Track MCU State Log
	 */
	citadel_pm_misc_init();

	SYS_LOG_DBG("Citadel PM driver loaded."
		" Entering Default Power State...");

	/* Test Code - Begin
	 * Write to Persistent Register to trigger Waveform Capture
	 * cpu_reg32_wr(CRMU_IHOST_SW_PERSISTENT_REG0, 0xabcdef10);
	 * Test code - End
	 */

	/* Enter the Default State */
	bcm5820x_enter_default_state(dev, 1);

	SYS_LOG_DBG("Completed PM Init & entered Default State.");

	return 0;
}

/** Instance of the Power Manager Device Template
 *  Contains the default values
 */
static const struct pm_driver_api bcm5820x_pm_driver_api = {
	.enter_default_state = bcm5820x_enter_default_state,
	.enter_target_state  = bcm5820x_enter_target_state,
	.get_pm_state = mproc_get_current_pm_state,
	.get_pm_state_desc = mproc_get_pm_state_desc,
	.get_rtc_wakeup_time = bcm5820x_get_rtc_wakeup_time,
	.set_aongpio_wakeup_source = bcm5820x_set_aongpio_wakeup_source,
	.set_nfc_lptd_wakeup_source = bcm5820x_set_nfc_lptd_wakeup_source,
	.set_usb_wakeup_source = bcm5820x_set_usb_wakeup_source,
	.set_ec_cmd_wakeup_source = bcm5820x_set_ec_cmd_wakeup_source,
	.set_smbus_wakeup_source = bcm5820x_set_smbus_wakeup_source,
	.set_rtc_wakeup_source = bcm5820x_set_rtc_wakeup_source,
	.set_tamper_wakeup_source = bcm5820x_set_tamper_wakeup_source,
	.set_aon_gpio_irq_callback = bcm5820x_set_aon_gpio_irq_callback,
	.set_nfc_lptd_irq_callback = bcm5820x_set_nfc_lptd_irq_callback,
	.set_usb_irq_callback = bcm5820x_set_usb_irq_callback,
	.set_ec_cmd_irq_callback = bcm5820x_set_ec_cmd_irq_callback,
	.set_smbus_irq_callback = bcm5820x_set_smbus_irq_callback,
	.set_rtc_irq_callback = bcm5820x_set_rtc_irq_callback,
	.set_tamper_irq_callback = bcm5820x_set_tamper_irq_callback,
	.set_aon_gpio_run_irq_callback = bcm5820x_set_aon_gpio_run_irq_callback,
};

/*
#define INT_SRC_CRMU_MPROC_MAILBOX CHIP_INTR__CRMU_MPROC_MAILBOX_INTERRUPT

#define INT_SRC_CRMU_MPROC_SPRU_RTC_PERIODIC \
		CHIP_INTR__CRMU_MPROC_SPRU_RTC_PERIODIC_INTERRUPT

#define INT_SRC_SMART_CARD0 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT0
#define INT_SRC_SMART_CARD1 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT1
#define INT_SRC_SMART_CARD2 CHIP_INTR__IOSYS_SMART_CARD_INTERRUPT_BIT2

#define INT_SRC_UART0 CHIP_INTR__IOSYS_UART0_INTERRUPT
#define INT_SRC_UART1 CHIP_INTR__IOSYS_UART1_INTERRUPT
#define INT_SRC_UART2 CHIP_INTR__IOSYS_UART2_INTERRUPT
#define INT_SRC_UART3 CHIP_INTR__IOSYS_UART3_INTERRUPT
*/

static const struct pm_device_config bcm5820x_pm_dev_cfg = {
	.base = 0x0,
	.irq_config_func = bcm5820x_mailbox_isr_init,
};

/** Private Data of the Power Manager
 *  Also contain the default values
 */
static struct bcm5820x_pm_dev_data_t bcm5820x_pm_dev_data = {
	.cur_pm_state = MPROC_DEFAULT_PM_MODE,
	.tgt_pm_state = MPROC_DEFAULT_PM_MODE,
	.aon_gpio_wakeup_config = (MPROC_AON_GPIO_WAKEUP_SETTINGS_s *)(&citadel_aon_gpio_wakeup_config.wakeup_src),
	.rtc_wakeup_time = MPROC_PM_DEFAULT_RTC_WAKEUP_TIME,
	.aon_gpio_irq_cb = {NULL},
	.nfc_lptd_irq_cb = NULL,
	.usb_irq_cb = {NULL},
	.ec_cmd_irq_cb = NULL,
	.tamper_irq_cb = NULL,
	.smbus_irq_cb = NULL,
	.rtc_irq_cb = NULL,
	.aon_gpio_run_irq_cb = NULL
};

DEVICE_AND_API_INIT(bcm5820x_pm, CONFIG_PM_NAME, &bcm5820x_pm_init,
		&bcm5820x_pm_dev_data, &bcm5820x_pm_dev_cfg,
		PRE_KERNEL_1, CONFIG_PM_INIT_PRIORITY,
		&bcm5820x_pm_driver_api);

/**
 * @brief The Installer for Mail Box IRQ Interrupt service routine.
 *
 *
 * @param device.
 *
 * @return N/A
 */
static void bcm5820x_mailbox_isr_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* register A7 mbox isr */
	SYS_LOG_DBG("Register A7 mailbox ISR.");

	sys_set_bit(CRMU_IPROC_INTR_CLEAR,
	CRMU_IPROC_INTR_CLEAR__IPROC_MAILBOX_INTR_CLR);
	sys_clear_bit(CRMU_IPROC_INTR_MASK,
	CRMU_IPROC_INTR_MASK__IPROC_MAILBOX_INTR_MASK);

	SYS_LOG_DBG("Set & Cleared MailBox Interrupt.");

	/* Enable CRMU MPROC Mailbox Interrupt */
	IRQ_CONNECT(SPI_IRQ(INT_SRC_CRMU_MPROC_MAILBOX),
	MPROC_INTR__MAILBOX_FROM_MCU_PRIORITY_LEVEL, mproc_mbox_isr, DEVICE_GET(bcm5820x_pm), 0);
	irq_enable(SPI_IRQ(INT_SRC_CRMU_MPROC_MAILBOX));
	SYS_LOG_DBG("Enabled MailBox Interrupt & Installed handler.");
}
