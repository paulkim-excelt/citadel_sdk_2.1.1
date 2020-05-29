/*******************************************************************
 *
 *  Copyright 2013
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  Irvine CA 92619-7013
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 *******************************************************************
 *
 *
 *******************************************************************/
#ifndef __STUB_H
#define __STUB_H

#include "usbd.h"
#include "usbdevice_internal.h"
#include "usbdevice.h"


#define is_there_sc_cage()										(0)
#define printf( ...)			do { } while (0)

#undef ENTER_CRITICAL
#define ENTER_CRITICAL()										do { } while (0)
#undef EXIT_CRITICAL
#define EXIT_CRITICAL()											do { } while (0)

#undef ush_panic
#define ush_panic	printf
#undef cvGetDeltaTime
#define cvGetDeltaTime(x) (0)
#define SCHEDULE_FROM_ISR(task_woken)			do { } while (0)

typedef enum {
	FP_SENSOR_NONE,
	FP_SENSOR_SWIPE,
	FP_SENSOR_TOUCH
} fp_sensor_type_t;

#define CRMU_IHOST_POWER_CONFIG_NORMAL      					0x0
#define CRMU_IHOST_POWER_CONFIG_SLEEP       					0x2
#define CRMU_IHOST_POWER_CONFIG_DEEPSLEEP   					0x3
#define lynx_sleep_state()										0
#define lynx_sleep_mode_blocked()								0

#define handle_turnon_ush_command()								do { } while (0)
#define lynx_set_pwr_state(x)									do { } while (0)
#define lynx_put_lowpower_mode(x)								do { } while (0)

#define fp_sensor_type_enumerated() 							FP_SENSOR_NONE
#define lynx_get_fp_sensor_state()								FP_SENSOR_NONE

#define USB_BLOCK_TIMER											(0)
#define PHX_BLOCKED_DELAY_TIMER_CV_USBBOOT						(0)
#define TIMER_MODE_FREERUN										(0)
#define TIMER_MODE_PERIODIC										(1)

//#define ush_bootrom_versions(a, b, c, d)						(0)
uint32_t ush_bootrom_versions(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer);


#define setKeys(a,b,c)											(0)
#define readKeys(a,b,c)											(0)

#define bcm_generate_ecdsa_priv_key(a)							(100)
#define ush_getMachineID(a,b,c)									(0)
#define phx_rng_fips_init(a,b,c)								(0)
#define phx_crypto_init(a)										do { } while(0)
#define phx_rng_raw_generate(a,b,c,d,e)							(0)
#define phx_bulk_cipher_init(a, b, c, d, e, f)					(0)
#define phx_bulk_cipher_start(a,b,c,d,f,e,g,h,i)				(0)
#define phx_bulk_cipher_swap_endianness(a,b,c,d,e)				do { } while(0)
#define phx_bulk_cipher_dma_start(a,b,c,d,e)					(0)
#define phx_get_hmac_sha1_hash(a,b,c,d,e,f,g)					(0)
#define phx_get_hmac_sha256_hash(a,b,c,d,e,f,g)					(0)
#define phx_qspi_flash_enumerate(a, b)							(0)
#define phx_qspi_flash_write_extrachecks(a,b,c,d)				(0)

#define ushFieldUpgradeStart(a,b,c,d)							(0)
#define ushFieldUpgradeUpdate(a,b,c)							(0)
#define ushFieldUpgradeComplete(a,b,c)							(0)

#define USH_FLASH_SIZE											(4*1024*1024)
#define BOOTSTRAP_SIZE											(96*1024)
#define USH_SBI_SIZE											(128*1024)
#define SBI_FLASH_START_OFFSET									(0)
#define USH_FLASH_SIZE											(4*1024*1024)
#define USH_FLASH_PERSISTANT_SIZE								(252*1024)
#define USH_FLASH_CV_PERSISTANT_SIZE							(251*1024)
#define USH_FLASH_SCD_PERSISTANT_SIZE							(1*1024)

unsigned int sbi_upgrade_flag;
#define SBI_UPGRADE_FLAG										(sbi_upgrade_flag)
#define SET_SBI_UPGRADE_FLAG()									(sbi_upgrade_flag=1)
#define UNSET_SBI_UPGRADE_FLAG()								(sbi_upgrade_flag=0)
#define IS_SBI_UPGRADE()										(sbi_upgrade_flag)
#define MEM_BASE_XMEM											(0)

uint8_t OTP_MEMORY[100];
#define MEM_OTP													(&OTP_MEMORY)
#define get_reboot_count() 										(0)
#define isTPMUpgradeActivated()									(0)
//#define checkIfAntiHammeringTriggered(x)						(0)

#define reinitialize_function_table()
#define push_glb_regs()
#define pop_glb_regs()
#define check_and_wake_system()

// HSIZE defines
#ifndef AHB_BIT8
#define AHB_BIT8       0x0        //3'b000
#endif
#ifndef AHB_BIT16
#define AHB_BIT16      0x1        //3'b001
#endif
#ifndef AHB_BIT32
#define AHB_BIT32      0x2        //3'b010
#endif
#ifndef AHB_BIT64
#define AHB_BIT64      0x3        //3'b011
#endif
#define AHB_BIT128     0x4        //3'b100
#define AHB_BIT256     0x5        //3'b101
#define AHB_BIT512     0x6        //3'b110
#define AHB_BIT1024    0x7        //3'b111

#ifndef USH_BOOTROM /* SBI */

#define ARG_UNUSED(x) (void)(x)

PHX_STATUS phx_set_timer_milli_seconds(uint8_t timer, uint32_t time_ms, uint8_t mode, bool oneshot);
uint32_t phx_timer_get_current_value(uint8_t timer);
void phx_blocked_delay_us(unsigned int us);
void phx_blocked_delay_ms(unsigned int ms);
#define get_ms_ticks(x)  						(*x = 0)

#define SBI_DEBUG_LOG

#ifdef SBI_DEBUG_LOG /* enable for USB DEBUG only */
extern int post_log(char *format, ...);
#else
#define post_log( ...)									do { } while (0)
#endif

#else /* AAI */

#define phx_blocked_delay_us(x)
#define phx_blocked_delay_ms(x)
#define phx_set_timer_milli_seconds(a,b,c,d)
#define phx_timer_get_current_value(a)         0
#define get_ms_ticks(x)  						(*x = 0)

#define post_log( ...)									do { } while (0)

#endif /* AAI */


#define OTP_SIZE_KECDSA_PRIV						32 /* 32 bytes of ECDSA P-256 private key */
typedef enum _sotp_status{
	IPROC_OTP_INVALID,
	IPROC_OTP_VALID,
	IPROC_OTP_ALLONE,
	IPROC_OTP_ERASED,
	IPROC_OTP_FASTAUTH,
	IPROC_OTP_NOFASTAUTH
} OTP_STATUS;
typedef enum _OTPKeyType
{
	OTPKey_AES,
	OTPKey_HMAC_SHA256,
	OTPKey_DAUTH,
	OTPKey_DevIdentity,
	OTPKey_ecDSA,
	OTPKey_Binding,
	OTPKey_CustKey1,
	OTPKey_CustKey2,
	OTPKey_CustKey3
} OTPKeyType;

typedef enum bcm_scapi_auth_optype {
	BCM_SCAPI_AUTH_OPTYPE_FULL      = 0,
	BCM_SCAPI_AUTH_OPTYPE_INIT,
	BCM_SCAPI_AUTH_OPTYPE_UPDATE,
	BCM_SCAPI_AUTH_OPTYPE_FINAL,
	BCM_SCAPI_AUTH_OPTYPE_HMAC_HASH
} BCM_SCAPI_AUTH_OPTYPE;

typedef enum bcm_scapi_auth_modes {
	BCM_SCAPI_AUTH_MODE_HASH      = 0,
	BCM_SCAPI_AUTH_MODE_CTXT,
	BCM_SCAPI_AUTH_MODE_HMAC,
	BCM_SCAPI_AUTH_MODE_FHMAC,
	BCM_SCAPI_AUTH_MODE_CCM,
	BCM_SCAPI_AUTH_MODE_GCM,
	BCM_SCAPI_AUTH_MODE_XCBCMAC,
	BCM_SCAPI_AUTH_MODE_CMAC
} BCM_SCAPI_AUTH_MODE;
typedef enum phx_cipher_modes {
	PHX_CIPHER_MODE_NULL   = 0,
	PHX_CIPHER_MODE_ENCRYPT,
	PHX_CIPHER_MODE_DECRYPT,
	PHX_CIPHER_MODE_AUTHONLY
} PHX_CIPHER_MODE;

typedef enum phx_cipher_order {
	PHX_CIPHER_ORDER_NULL  = 0,
	PHX_CIPHER_ORDER_AUTH_CRYPT,
	PHX_CIPHER_ORDER_CRYPT_AUTH
} PHX_CIPHER_ORDER;

typedef enum phx_auth_algs {
    PHX_AUTH_ALG_NONE      = 0,
    PHX_AUTH_ALG_HMAC_SHA1,
    PHX_AUTH_ALG_SHA1,
    PHX_AUTH_ALG_DES_MAC,
    PHX_AUTH_ALG_3DES_MAC,
    PHX_AUTH_ALG_SHA256,
    PHX_AUTH_ALG_HMAC_SHA256
} PHX_AUTH_ALG;
typedef enum phx_encr_mode {
	PHX_ENCR_MODE_NONE     = 0,
	PHX_ENCR_MODE_CBC,
	PHX_ENCR_MODE_ECB,
	PHX_ENCR_MODE_CTR,
	PHX_ENCR_MODE_CCM = 5,
	PHX_ENCR_MODE_CMAC
} PHX_ENCR_MODE;

typedef enum phx_encr_algs {
	PHX_ENCR_ALG_NONE      = 0,
    PHX_ENCR_ALG_AES_128,
    PHX_ENCR_ALG_AES_192,
    PHX_ENCR_ALG_AES_256,
    PHX_ENCR_ALG_DES,
    PHX_ENCR_ALG_3DES,
} PHX_ENCR_ALG;
typedef enum phx_auth_mode {
    PHX_AUTH_MODE_ALL      =  BCM_SCAPI_AUTH_MODE_HASH
} PHX_AUTH_MODE;

// HBURST defines
#define AHB_SINGLE     0x0        //3'b000
#define AHB_INCR       0x1        //3'b001
#define AHB_WRAP4      0x2        //3'b010
#define AHB_INCR4      0x3        //3'b011
#define AHB_WRAP8      0x4        //3'b100
#define AHB_INCR8      0x5        //3'b101
#define AHB_WRAP16     0x6        //3'b110
#define AHB_INCR16     0x7        //3'b111

#define AHB_READ_SINGLE(addr,size)                                      \
    ((size == AHB_BIT8) ?  *((volatile uint8_t *)addr) :                  \
     ((size == AHB_BIT16) ? *((volatile uint16_t *)addr) :                \
      *((volatile uint32_t *)addr)))

#define AHB_WRITE(dest_addr, src_addr, byte_length)                     \
    memcpy((void *)dest_addr, (const void *)src_addr, (size_t) byte_length)

#define AHB_WRITE_BZERO(dest_addr, byte_length)                         \
    memset((void *)dest_addr, 0, (size_t)byte_length)

#define AHB_WRITE_SINGLE(addr,data,size)                                \
     (size == AHB_BIT8)? (*((volatile uint8_t *)addr) = data):            \
     ((size == AHB_BIT16)?(*((volatile uint16_t *)addr) = data):(*((volatile uint32_t *)addr) = data))

#define SHA1_HASH_SIZE               20
#define SBI_CMD_REBOOT_USH_TO_USH	0x58800100
#define SBI_CMD_REBOOT_USH_TO_USH_MASK	0x00000100
#define SBI_CMD_REBOOT_USH_TO_SBI	0x58801000
#define SBI_CMD_REBOOT_USH_TO_USH_LRESET	0x58802000
#define SBI_CMD_REBOOT_USH_SKIP_USB   0x4000
#define USH_MAX_REBOOT_HAMMERING_THRESHOLD 1

typedef enum LYNX_STATUS{
	LYNX_STATUS_OK = 0,
	LYNX_STATUS_OK_NEED_DMA                  =  1,
	LYNX_STATUS_DMA_ERROR,
	LYNX_STATUS_NO_FREE_DMA_CHANNEL,
	LYNX_EXCEPTION_UNDEFINED                 = 10, /* THIS SET OF VALUE REPEATED IN SYSTEM.S */
	LYNX_EXCEPTION_SWI,
	LYNX_EXCEPTION_PRE_ABT,
	LYNX_EXCEPTION_DATA_ABT,
	LYNX_EXCEPTION_RESEVED,
	LYNX_EXCEPTION_IRQ,
	LYNX_EXCEPTION_FIQ,
	LYNX_STATUS_MBIST_FAIL_PKE,
	LYNX_STATUS_MBIST_FAIL_IDTCM,
	LYNX_STATUS_MBIST_FAIL_SMAU_SCRATCH,           /* FOR SBI */
	LYNX_STATUS_MBIST_FAIL,

	LYNX_EXCEPTION_OPEN_UNDEFINED            = 40, /* THIS SET OF VALUE REPEATED IN SYSTEM.S */
	LYNX_EXCEPTION_OPEN_SWI,
	LYNX_EXCEPTION_OPEN_PRE_ABT,
	LYNX_EXCEPTION_OPEN_DATA_ABT,
	LYNX_EXCEPTION_OPEN_RESEVED,
	LYNX_EXCEPTION_OPEN_IRQ,
	LYNX_EXCEPTION_OPEN_FIQ,

	LYNX_STATUS_NO_DEVICE                    = 50,
	LYNX_STATUS_DEVICE_BUSY,
	LYNX_STATUS_DEVICE_FAILED,
	LYNX_STATUS_NO_MEMORY,
	//  LYNX_STATUS_NO_RESOURCES,
	LYNX_STATUS_NO_BUFFERS,
	LYNX_STATUS_PARAMETER_INVALID,
	LYNX_STATUS_TIMED_OUT                    = 60,
	LYNX_STATUS_UNAUTHORIZED,
	LYNX_STATUS_SSMC_FLASH_CFI_FAIL          = 70,
	LYNX_STATUS_SSMC_FLASH_AUTOSELECT_FAIL,
	LYNX_STATUS_SSMC_FLASH_ERASE_FAIL,
	LYNX_STATUS_SSMC_FLASH_PROGRAM_FAIL,

	LYNX_STATUS_BSC_DEVICE_NO_ACK            = 80,
	LYNX_STATUS_BSC_DEVICE_INCOMPLETE,
	LYNX_STATUS_BSC_TIMEOUT,
	LYNX_STATUS_BBL_BUSY                     = 90,

	LYNX_STATUS_SOTP_ERROR	= 95

}LYNX_STATUS;

#define BCM_SCAPI_STATUS_OK (BCM_SCAPI_STATUS)LYNX_STATUS_OK
#define BCM_SCAPI_STATUS_OK_NEED_DMA (BCM_SCAPI_STATUS)LYNX_STATUS_OK_NEED_DMA
#define BCM_SCAPI_STATUS_DEVICE_BUSY (BCM_SCAPI_STATUS)LYNX_STATUS_DEVICE_BUSY
#define SHA256_HASH_SIZE             32

typedef enum BCM_SCAPI_STATUS{
	BCM_SCAPI_STATUS_CRYPTO_ERROR                 = 100,
	BCM_SCAPI_STATUS_PARAMETER_INVALID,
	BCM_SCAPI_STATUS_CRYPTO_NEED2CALLS,
	BCM_SCAPI_STATUS_CRYPTO_PLAINTEXT_TOOLONG,
	BCM_SCAPI_STATUS_CRYPTO_AUTH_FAILED,
	BCM_SCAPI_STATUS_CRYPTO_ENCR_UNSUPPORTED,
	BCM_SCAPI_STATUS_CRYPTO_AUTH_UNSUPPORTED,
	BCM_SCAPI_STATUS_CRYPTO_MATH_UNSUPPORTED,
	BCM_SCAPI_STATUS_CRYPTO_WRONG_STEP,
	BCM_SCAPI_STATUS_CRYPTO_KEY_SIZE_MISMATCH,
	BCM_SCAPI_STATUS_CRYPTO_AES_LEN_NOTALIGN,	/* 110 */
	BCM_SCAPI_STATUS_CRYPTO_AES_OFFSET_NOTALIGN,
	BCM_SCAPI_STATUS_CRYPTO_AUTH_OFFSET_NOTALIGN,
	BCM_SCAPI_STATUS_CRYPTO_DEVICE_NO_OPERATION,
	BCM_SCAPI_STATUS_CRYPTO_NOT_DONE,
	BCM_SCAPI_STATUS_CRYPTO_RNG_NO_ENOUGH_BITS,
	BCM_SCAPI_STATUS_CRYPTO_RNG_RAW_COMPARE_FAIL,       /* generate rng is same as what saved last time */
	BCM_SCAPI_STATUS_CRYPTO_RNG_SHA_COMPARE_FAIL,       /* rng after hash is same as what saved last time */
	BCM_SCAPI_STATUS_CRYPTO_TIMEOUT,
	BCM_SCAPI_STATUS_CRYPTO_AUTH_LENGTH_NOTALIGN,       /* for sha init/update */
	BCM_SCAPI_STATUS_CRYPTO_RN_PRIME,			/* 120 */
	BCM_SCAPI_STATUS_CRYPTO_RN_NOT_PRIME,
	BCM_SCAPI_STATUS_CRYPTO_QLIP_CTX_OVERFLOW,
	BCM_SCAPI_STATUS_INPUTDATA_EXCEEDEDSIZE,
	BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY,
	BCM_SCAPI_STATUS_INVALID_AES_KEYSIZE,

	BCM_SCAPI_STATUS_RSA_PKCS1_LENGTH_NOT_MATCH   = 150, /* PKCS1 related error codes */
	BCM_SCAPI_STATUS_RSA_PKCS1_SIG_VERIFY_FAIL,
	BCM_SCAPI_STATUS_RSA_PKCS1_ENCODED_MSG_TOO_SHORT,
	BCM_SCAPI_STATUS_RSA_INVALID_HASHALG,
	BCM_SCAPI_STATUS_ECDSA_VERIFY_FAIL,
	BCM_SCAPI_STATUS_ECDSA_SIGN_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_FAIL                = 200,
	BCM_SCAPI_STATUS_SELFTEST_3DES_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DES_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_AES_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_SHA1_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DH_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_RSA_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DSA_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DSA_SIGN_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DSA_VERIFY_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_DSA_PAIRWISE_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_ECDSA_SIGN_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_ECDSA_VERIFY_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_ECDSA_PAIRWISE_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_ECDSA_PAIRWISE_INTRN_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_MATH_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_RNG_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_MODEXP_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_MODINV_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_SHA256_FAIL,
	BCM_SCAPI_STATUS_SELFTEST_AES_SHA256_FAIL,
	BCM_SCAPI_STATUS_INVALID_KEY
}BCM_SCAPI_STATUS;

unsigned int isUsbdEnumDone(void);

#endif /* _STUB_HEADER */
