/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/
/*
 * Broadcom Corporation Credential Vault API
 */
/*
 * cvinternal.h:  Credential Vault internal function prototypes and structures
 */
/*
 * Revision History:
 *
 * 01/03/07 DPA Created.
*/

#ifndef _CVINTERNAL_H_
#define _CVINTERNAL_H_ 1

#include "ush_mmap.h"
#include "phx_scapi.h"
#include "phx_osl.h"

#include "usbd.h"

#include "crypto_stubs.h"

/*
 * Enumerated values and types
 */
#define MAX_INPUT_PARAMS 20 /* TBD */
#define MAX_OUTPUT_PARAMS 20 /* TBD */
#define MAX_OUTPUT_PARAM_SIZE 4096 /* TBD */
#define ALIGN_DWORD(x) ((x + 0x3) & 0xfffffffc)
#define CALLBACK_STATUS_TIMEOUT 5000 /* milliseconds */
#define HOST_STORAGE_REQUEST_TIMEOUT (60*1000) /* milliseconds */
#define HOST_CONTROL_REQUEST_TIMEOUT (60*1000) /* milliseconds */
#define FP_IMAGE_TRANSFER_TIMEOUT 500 /* milliseconds */
#define USER_PROMPT_TIMEOUT 30000 /* milliseconds */
#define HOST_RESPONSE_TIMEOUT (60*1000) /* milliseconds */
#define USER_PROMPT_CALLBACK_INTERVAL 1000 /* milliseconds */
#define SMART_CARD_INSERTION_DELAY 5000 /* milliseconds */
#define MAX_CV_OBJ_CACHE_NUM_OBJS			4	/* must divide obj cache into 4-byte aligned areas */
#define MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing other directories */
#define MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing objects */
#define INVALID_CV_CACHE_ENTRY				0xff /* invalid value to use for initializing cache entries */
#define MAX_CV_ENTRIES_PER_DIR_PAGE			16	/* number of directory pages per higher level page */
#define MAX_DIR_LEVELS						3   /* note: this reflects implementation and can't be changed w/o changing code */
#define MAX_CV_NUM_VOLATILE_OBJS			40	/* must divide obj cache into 4-byte aligned areas */
#define MAX_DIR_PAGE_0_ENTRIES				200	/* maximum number of directory page 0 entries */
#define HANDLE_DIR_SHIFT 24
#define GET_DIR_PAGE(x) (x>>HANDLE_DIR_SHIFT)
#define GET_HANDLE_FROM_PAGE(x) (x<<HANDLE_DIR_SHIFT)
#define HANDLE_VALUE_MASK 0x00ffffff
#define GET_L1_HANDLE_FROM_OBJ_HANDLE(x) ((x>>4) & ~HANDLE_VALUE_MASK)
#define GET_L2_HANDLE_FROM_OBJ_HANDLE(x) ((x) & ~HANDLE_VALUE_MASK)
#define DIRECTORY_PAGES_MASK 0xffff
#define MAX_DIR_PAGES 256
#define MIN_L2_DIR_PAGE 16
#define MAX_L2_DIR_PAGE 254
#define AES_128_MASK (AES_128_BLOCK_LEN - 1)
#define GET_AES_128_PAD_LEN(x) ((~x + 1) & AES_128_MASK)
#define MAX_CV_AUTH_PROMPTS 5 /* TBD */
#define CV_HID_CREDENTIAL_SIZE 0 /* TBD */
#define CV_MAX_CHUID_SIZE 3377 /* this is the maximum size of the CHUID field of a PIV card */
#define CV_MAX_CAC_CERT_SIZE 1100 /* this is the maximum size of the cert field of a CAC card */
#define MAX_FP_FEATURE_SET_SIZE 600
#define MAX_FP_TEMPLATE_SIZE	2048
#define MAX_FP_TEMPLATES 50
#define AVERAGE_FP_TEMPLATE_SIZE	1024
#define MAX_NUM_FP_TEMPLATES (CV_PERSISTENT_OBJECT_HEAP_LENGTH/AVERAGE_FP_TEMPLATE_SIZE)
#define DEFAULT_CV_DA_AUTH_FAILURE_THRESHOLD 20
#define DEFAULT_CV_DA_INITIAL_LOCKOUT_PERIOD 5000 /* in milliseconds */
#define RSA_PUBKEY_CONSTANT 65537
#define KDC_PUBKEY_LEN 2048
#define PRIME_FIELD	0
#define BINARY_FIELD 1
#define PRIV_KEY_INVALID 0
#define PRIV_KEY_VALID 1
#define CV_OBJ_CACHE_SIZE			(2048*7 * MAX_CV_OBJ_CACHE_NUM_OBJS) /* MAX_CV_OBJ_SIZE * MAX_CV_OBJ_CACHE_NUM_OBJS */
#define CV_LEVEL1_DIR_PAGE_CACHE_SIZE (sizeof(cv_dir_page_level1)*MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES)
#define CV_LEVEL2_DIR_PAGE_CACHE_SIZE (sizeof(cv_dir_page_level2)*MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES)
#define OPEN_MODE_BLOCK_PTRS (0x00000000)
#define SECURE_MODE_BLOCK_PTRS (0x00000000)
#define SYS_AND_TPM_IO_OPEN_WINDOW	0
#define SMART_CARD_IO_OPEN_WINDOW	1
#define RF_IO_OPEN_WINDOW			2
#define CV_IO_OPEN_WINDOW			3
#define CV_OBJ_CACHE_ENTRY_SIZE		CV_OBJ_CACHE_SIZE/MAX_CV_OBJ_CACHE_NUM_OBJS
#define CV_VOLATILE_OBJECT_HEAP_LENGTH 3*1024
#define CV_BRCM_RFID_PARAMS_CUST_ID		0
#define CV_UPK_FLASH_BLOCK_CUST_ID		1
#define CV_HID_FLASH_BLOCK_CUST_ID		2
#define CV_BRCM_HDOTP_CUST_ID			3
#define CV_HID2_FLASH_BLOCK_CUST_ID		4
#define CV_DPPBA_FLASH_BLOCK_CUST_ID	5
#define CV_VALIDITY_FLASH_BLOCK_CUST_ID	6
#define CV_HID_KEY_FLASH_BLOCK_CUST_ID	7
/* below is the definition of the memory block that is shared between CV IO, FP match and FP capture */
/*-------------------------------   <---   CV_SECURE_IO_COMMAND_BUF
|  CV I/O command buf (4K)    |
|							  |   <---   CV_SECURE_IO_FP_ASYNC_BUF
|  FP I/O command buf (4K)    |
|-----------------------------|   <---   FP_CAPTURE_LARGE_HEAP_PTR
|              |              |
|FP persistent |              |
|    data      |   Large      |
|    for FP    |   heap       |
| vendors (91K)|   for FP     |
|              |   vendors    |
| Data for     |   (140K)     |
| matcher (49K)|              |
|              |              |
-------------------------------*/


#define CV_IO_COMMAND_BUF_SIZE		(MAX_CV_COMMAND_LENGTH + 100)	/* 4K Buffer + space for header */
#define FP_MATCH_HEAP_SIZE				(49*1024)
#define FP_CAPTURE_LARGE_HEAP_SIZE_5882	(150*1024 + 100)
#define FP_CAPTURE_LARGE_HEAP_SIZE_5880	(140*1024 + 100)
#define FP_CAPTURE_LARGE_HEAP_SIZE		(215*1024 + 100)
#define FP_CAPTURE_SMALL_HEAP_SIZE	(3*1024)
#define HID_LARGE_HEAP_SIZE			(12*1024)
#define HID_SMALL_HEAP_SIZE			(1*1024)
#define USBH_HEAP_SIZE				(7*1024)
#define FP_CAPTURE_LARGE_HEAP_SIZE_THAT_IS_KEPT_PERSISTENT_FOR_FP_VENDORS (91*1024)
#define FP_MATCH_HEAP_PTR			(FP_CAPTURE_LARGE_HEAP_PTR + FP_CAPTURE_LARGE_HEAP_SIZE_THAT_IS_KEPT_PERSISTENT_FOR_FP_VENDORS)
#define FP_IMAGE_MAX_SIZE			90*1024  /* 90K bytes */
#define HID_SEOS_HEAP_SIZE                      (16*1024)
/* async FP capture task parameters */
/*
 * Global ChipID
 */
#define CHIP_IS_5882 ((unsigned int)VOLATILE_MEM_PTR->chipIs5882)

#if 0 //FIXME: LYCX_CS_PORT
extern  unsigned int Image$$USH_ARM_LIB_HEAP$$ZI$$Limit; 
#define CV_SKIP_TABLE_BUFFER ((uint8_t *)(&Image$$USH_ARM_LIB_HEAP$$ZI$$Limit))
#else
uint8_t ImageUSH_ARM_LIB_HEAPZILimit[1024]; 
#define CV_SKIP_TABLE_BUFFER ((uint8_t *)(ImageUSH_ARM_LIB_HEAPZILimit))
#endif

#define CV_ASYNC_FP_CAPTURE_TASK_STACK_SIZE 6000
#define CV_ASYNC_FP_CAPTURE_TASK_STACK ((uint8_t *)(CV_SKIP_TABLE_BUFFER + sizeof(cv_hdotp_skip_table)))
#define CV_ENROLLMENT_FEATURE_SET_BUF ((uint8_t *)(CV_ASYNC_FP_CAPTURE_TASK_STACK + CV_ASYNC_FP_CAPTURE_TASK_STACK_SIZE))
#define CV_ENROLLMENT_FEATURE_SET_BUF0 (CV_ENROLLMENT_FEATURE_SET_BUF)
#define CV_ENROLLMENT_FEATURE_SET_BUF1 (CV_ENROLLMENT_FEATURE_SET_BUF0 + MAX_FP_FEATURE_SET_SIZE)
#define CV_ENROLLMENT_FEATURE_SET_BUF2 (CV_ENROLLMENT_FEATURE_SET_BUF1 + MAX_FP_FEATURE_SET_SIZE)


#define FP_DEFAULT_USER_TIMEOUT		15000 /* 15 seconds in milliseconds */
#define CV_DEFAULT_IDENTIFY_USER_TIMEOUT		2000 /* 2 seconds in milliseconds */
#define FP_MIN_USER_TIMEOUT			1000 /* 1 seconds in milliseconds */
#define FP_MAX_USER_TIMEOUT			30000 /* 30 seconds in milliseconds */
#define FP_DEFAULT_CALLBACK_PERIOD	1000 /* 1 second in milliseconds */
#define FP_MIN_CALLBACK_PERIOD		500 /* .5 second in milliseconds */
#define FP_MAX_CALLBACK_PERIOD		10000 /* 10 second in milliseconds */
#define FP_DEFAULT_SW_FDET_POLLING_INTERVAL 1000 /* 1 second in milliseconds */
#define FP_MIN_SW_FDET_POLLING_INTERVAL		250 /* .25 second in milliseconds */
#define FP_MAX_SW_FDET_POLLING_INTERVAL		5000 /* 5 seconds in milliseconds */
#define FP_DEFAULT_PERSISTENCE		10000 /* 10 seconds in milliseconds */
#define FP_MIN_PERSISTENCE			1 /* 0 second in milliseconds */
#define FP_MAX_PERSISTENCE			10000 /* 10 seconds in milliseconds */
#define FP_DEFAULT_FAR				0x53E2 	/* 1/100,000 */
#define FP_MIN_FAR					0 			/* 0.0 */
#define FP_MAX_FAR					0x7FFFFFFF 	/* 1.0 */
#define FP_UPKS_VID					0x147E /* UPEK USB swipe sendor vendor ID */
#define FP_UPKA_VID					0x147E /* UPEK USB area sensor vendor ID */
#define FP_AT_VID					0x08FF /* Authentec USB vendor ID */
#define FP_UPKS_PID					0x1000 /* UPEK swipe sensor PID */
#define FP_UPKA_PID					0x3000 /* UPEK area sensor PID */
#define FP_AT_PID					0x2810 /* AT swipe sensor PID */
#define FP_VALIDITY_VID				0x138A /* Validity swipe sensor Vendor ID */
#define FP_VALIDITY_PID_VFS301		0x0005	   /* Validity swipe sensor Product ID */
#define FP_VALIDITY_PID_VFS5111		0x0010	   /* Validity swipe sensor Product ID */
#define USB_VID_OFFSET				8 /* vendor ID offset in USB device descriptor */
#define USB_PID_OFFSET				10 /* product ID offset in USB device descriptor */
#define CV_MAX_USER_ID_LEN			1090 /* maximum user ID length */
#define CV_MAX_USER_KEY_LEN			20 /* TBD - maximum user key length */
#define CV_MAX_EXPORT_BLOB_EMBEDDED_HANDLES 100
#define CV_MAX_PROPRIETARY_SC_KEYLEN 16
#define CV_MALLOC_HEAP_OVERHEAD 36
#define CV_MALLOC_ALLOC_OVERHEAD 8
#define CV_FLASH_ALLOC_HEAP_OVERHEAD 0
#define CV_FLASH_ALLOC_OVERHEAD 0
#define CV_MAX_DP_HINT 2048
#define PBA_FP_DETECT_TIMEOUT 400	/* 500ms for fingerprint detection for pba identify user */
#define CV_2T_BUMP_SCALE_FACTOR 4
#define CV_MAX_2T_BUMP_RATE ((uint32_t)((128*1024*CV_2T_BUMP_SCALE_FACTOR)/(5*26*40)))  /* max 2T bump rate is 128K 2T bits / 5 years * 26 weeks/year * 40 hours/week */
#define CV_MAX_2T_BUMP_FAIL 64			/* when bumping 2T, loop will exit after this many tries if errors occur */
#define DP_BAD_SWIPE_MASK 0x80048000	/* Ignoring two lower bytes signalling type of error */
#define DP_FR_ERR_IO	  0x8007054F	/* DP generic file I/O error */
#define DP_FR_ERR_INVALID_IMAGE	0x85BA0022	/* Invalid image */
#define FP_POLLING_PERIOD	200		/* Poll sensor every .2 secs.  This is worst case when CLSC enabled too */
                                    /* It will allow low res timer to be called every .2 secs to support CLSC */
#define FP_POLLING_PERIOD_WBF	200	/* always use same default poll interval */	
#define FP_POLLING_PERIOD_FAST	200	/* always use same default poll interval */	
#define FP_POLLING_TIMEOUT      300000  /* timeout for fp detect and capture */

#define CV_AH_MS_PER_CREDIT 374400		/* based on 9.6 2T bumps/hour: 100000/(5 yr * 52 wk/yr * 40 hr/wk) */
#define CV_AH_CREDIT_THRESHOLD_LOW 10
#define CV_AH_CREDIT_THRESHOLD_MED 20
#define CV_AH_CREDIT_THRESHOLD_HIGH 40
#define CV_AH_BUMP_INTERVAL_LOW 60000
#define CV_AH_BUMP_INTERVAL_MED 30000
#define CV_AH_BUMP_INTERVAL_HIGH 10000
#define CV_AH_INITIAL_CREDITS 1000
#define CV_UPDATE_AH_CREDITS_INTERVAL 3600000
#define CV_AH_MAX_CREDITS 1000
#define CV_MAX_HDOTP_SKIP_TABLE_ENTRIES 1024 /* max number of entries for HDOTP skip table */
#define CV_MIN_TIME_BETWEEN_FLASH_UPDATES 6*60*1000		/* 6 minutes minimum between flash updates, for antihammering */
#define CV_FP_CONFIGURE_DURATION 10*1000				/* 10 sec duration of FP configure update */
#define CV_PBA_CODE_UPDATE_DURATION 2*60*1000			/* 2 minute duration of PBA code update */
#define CV_HDOTP_READ_LIMIT 10000	/* max number of reads of HDOTP value before bump necessary */	
#define CV_EXPORT_ENC_INNER_KEY_ADDR 0x10087aa6
#define CV_MAX_CACHED_CREDENTIAL_LENGTH SHA256_LEN

typedef unsigned int cv_status_internal;
#define CV_FP_NO_MATCH			0x80000001
#define CV_FP_MATCH			0x80000002
#define CV_FP_IMAGE_MATCH_AND_UPDATED	0x80000003
#define CV_OBJECT_UPDATE_REQUIRED	0x80000004

typedef unsigned int cv_transport_type;
enum cv_transport_type_e {
		CV_TRANS_TYPE_ENCAPSULATED = 0x00000001,
		CV_TRANS_TYPE_HOST_STORAGE = 0x00000002,
		CV_TRANS_TYPE_HOST_CONTROL = 0x00000004
};

typedef unsigned int cv_param_type;
enum cv_param_type_e {
	CV_ENCAP_STRUC,
	CV_ENCAP_LENVAL_STRUC,
	CV_ENCAP_LENVAL_PAIR,
	CV_ENCAP_INOUT_LENVAL_PAIR
};

typedef unsigned short cv_secure_session_protocol;
enum cv_secure_session_protocol_e {
		AES128wHMACSHA1 = 0x00000000
};

typedef unsigned short cv_return_type;
enum cv_return_type_e {
		CV_RET_TYPE_SUBMISSION_STATUS	= 0x00000000,
		CV_RET_TYPE_INTERMED_STATUS	= 0x00000001,
		CV_RET_TYPE_RESULT		= 0x00000002,
		CV_RET_TYPE_RESUBMITTED	= 0x00000003
};

typedef unsigned short cv_storage_type;
enum cv_storage_type_e {
		CV_STORAGE_TYPE_HARD_DRIVE		= 0x00000000,
		CV_STORAGE_TYPE_FLASH			= 0x00000001,
		CV_STORAGE_TYPE_SMART_CARD		= 0x00000002,
		CV_STORAGE_TYPE_CONTACTLESS		= 0x00000003,
		CV_STORAGE_TYPE_FLASH_NON_OBJ		= 0x00000004,
		CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ	= 0x00000005,
		CV_STORAGE_TYPE_FLASH_PERSIST_DATA	= 0x00000006,
		CV_STORAGE_TYPE_HARD_DRIVE_UCK		= 0x00000007,
		CV_STORAGE_TYPE_VOLATILE			= 0x00000008,
};

#define	CV_DIR_0_HANDLE			0x00000000
#define	CV_TOP_LEVEL_DIR_HANDLE		0x00ffffff
#define	CV_UCK_OBJ_HANDLE		0x01000000
#define	CV_PERSISTENT_DATA_HANDLE	0x00000001
#define	CV_MIRRORED_DIR_0		0xff000000
#define	CV_MIRRORED_TOP_LEVEL_DIR 	0xffffffff
#define	CV_ANTIHAMMERING_CREDIT_DATA_HANDLE	0x00000002
#define	CV_HDOTP_TABLE_HANDLE	0x00000003

#define CV_MIRRORED_OBJ_MASK	0xff000000
#define CV_OBJ_HANDLE_PAGE_MASK	0xff000000

typedef unsigned short cv_command_id;
//typedef uint32_t cv_command_id;
enum cv_command_id_e {
		CV_ABORT,					// 0x0000
		CV_CALLBACK_STATUS,				// 0x0001
		CV_CMD_OPEN,					// 0x0002
		CV_CMD_OPEN_REMOTE,				// 0x0003
		CV_CMD_CLOSE,					// 0x0004
		CV_CMD_GET_SESSION_COUNT,			// 0x0005
		CV_CMD_INIT,					// 0x0006
		CV_CMD_GET_STATUS,				// 0x0007
		CV_CMD_SET_CONFIG,				// 0x0008
		CV_CMD_CREATE_OBJECT,				// 0x0009
		CV_CMD_DELETE_OBJECT,				// 0x000A
		CV_CMD_SET_OBJECT,				// 0x000B
		CV_CMD_GET_OBJECT,				// 0x000C
		CV_CMD_ENUMERATE_OBJECTS,			// 0x000D
		CV_CMD_DUPLICATE_OBJECT,			// 0x000E
		CV_CMD_IMPORT_OBJECT,				// 0x000F
		CV_CMD_EXPORT_OBJECT,				// 0x0010
		CV_CMD_CHANGE_AUTH_OBJECT,			// 0x0011
		CV_CMD_AUTHORIZE_SESSION_OBJECT,		// 0x0012
		CV_CMD_DEAUTHORIZE_SESSION_OBJECT,		// 0x0013
		CV_CMD_GET_RANDOM,				// 0x0014
		CV_CMD_ENCRYPT,					// 0x0015
		CV_CMD_DECRYPT,					// 0x0016
		CV_CMD_HMAC,					// 0x0017
		CV_CMD_HASH,					// 0x0018
		CV_CMD_HASH_OBJECT,				// 0x0019
		CV_CMD_GENKEY,					// 0x001A
		CV_CMD_SIGN,					// 0x001B
		CV_CMD_VERIFY,					// 0x001C
		CV_CMD_OTP_GET_VALUE,				// 0x001D
		CV_CMD_OTP_GET_MAC,				// 0x001E
		CV_CMD_UNWRAP_DLIES_KEY,			// 0x001F
		CV_CMD_FINGERPRINT_ENROLL,			// 0x0020
		CV_CMD_FINGERPRINT_VERIFY,			// 0x0021
        CV_CMD_LOAD_SBI,                        // 0x0022
		CV_SECURE_SESSION_GET_PUBKEY,			// 0x0023
		CV_SECURE_SESSION_HOST_NONCE,			// 0x0024
		CV_HOST_STORAGE_GET_REQUEST,			// 0x0025
		CV_HOST_STORAGE_STORE_OBJECT,			// 0x0026
		CV_HOST_STORAGE_DELETE_OBJECT,			// 0x0027
		CV_HOST_STORAGE_RETRIEVE_OBJECT,		// 0x0028
		CV_HOST_STORAGE_OPERATION_STATUS,		// 0x0029
		CV_HOST_CONTROL_GET_REQUEST,			// 0x002A
		CV_HOST_CONTROL_FEATURE_EXTRACTION,		// 0x002B
		CV_HOST_CONTROL_CREATE_TEMPLATE,		// 0x002C
		CV_HOST_CONTROL_IDENTIFY_AND_HINT_CREATION,	// 0x002D
		CV_CMD_FINGERPRINT_CAPTURE,				// 0x002E
		CV_CMD_FINGERPRINT_IDENTIFY,			// 0x002F
		CV_CMD_FINGERPRINT_CONFIGURE,			// 0x0030
		CV_CMD_FINGERPRINT_TEST,				// 0x0031
		CV_CMD_FINGERPRINT_CALIBRATE,			// 0x0032
		CV_CMD_BIND,							// 0x0033
		CV_CMD_BIND_CHALLENGE,					// 0x0034
		CV_CMD_IDENTIFY_USER,					// 0x0035
		CV_CMD_FLASH_PROGRAMMING,				// 0x0036
		CV_CMD_ACCESS_PBA_CODE_FLASH,			// 0x0037
		CV_CMD_DIAG_TEST,            			// 0x0038
		CV_CMD_USH_VERSIONS,            		// 0x0039
		CV_HOST_STORAGE_DELETE_ALL_FILES,		// 0x003a
		CV_CMD_OTP_PROG,						// 0x003b
		CV_CMD_CUSTID_PROG,						// 0x003c
		CV_CMD_PARTNER_DISABLE_PROG,			// 0x003d
		CV_CMD_PARTNER_ENABLE_PROG,				// 0x003e
		CV_CMD_ENABLE_RADIO,					// 0x003f
		CV_CMD_ENABLE_SMARTCARD,				// 0x0040
		CV_CMD_ENABLE_FINGERPRINT,				// 0x0041
		CV_CMD_READ_CONTACTLESS_CARD_ID,		// 0x0042
        CV_CMD_FW_UPGRADE_START,                // 0x0043
        CV_CMD_FW_UPGRADE_UPDATE,               // 0x0044
        CV_CMD_FW_UPGRADE_COMPLETE,             // 0x0045
        CV_CMD_ENABLE_AND_LOCK_RADIO,           // 0x0046
        CV_CMD_SET_RADIO_PRESENT,				// 0x0047
        CV_CMD_NOTIFY_POWER_STATE_TRANSITION,	// 0x0048
        CV_CMD_SET_CV_ONLY_RADIO_MODE,			// 0x0049
        CV_CMD_NOTIFY_SYSTEM_POWER_STATE,		// 0x004a
        CV_CMD_REBOOT_TO_SBI,					// 0x004b
        CV_CMD_REBOOT_USH,						// 0x004c
		CV_CMD_START_CONSOLE_LOG,				// 0x004d
		CV_CMD_STOP_CONSOLE_LOG,				// 0x004e
		CV_CMD_GET_CONSOLE_LOG,					// 0x004f
		CV_CMD_PUTCHARS,						// 0x0050
		CV_CMD_CONSOLE_MENU,					// 0x0051
		CV_CMD_SET_SYSTEM_ID,					// 0x0052
		CV_CMD_SET_RFID_PARAMS,					// 0x0053
        CV_CMD_SET_SMARTCARD_PRESENT,			// 0x0054
        CV_CMD_SEND_BLOCKDATA,					// 0x0055
        CV_CMD_RECEIVE_BLOCKDATA,				// 0x0056
        CV_CMD_PWR,								// 0x0057
        CV_CMD_PWR_START_IDLE_TIME,				// 0x0058	
		CV_CMD_IO_DIAG,                         // 0x0059  
		CV_CMD_UPGRADE_OBJECTS,                 // 0x005a  
		CV_CMD_GET_MACHINE_ID,                  // 0x005b  
		CV_CMD_FINGERPRINT_CAPTURE_WBF,         // 0x005c
		CV_CMD_FINGERPRINT_RESET,		// 0x005d  
		CV_CMD_REGISTER_SUPER,                  // 0x005e  
		CV_CMD_CHALLENGE_SUPER,                 // 0x005f  
		CV_CMD_RESPONSE_SUPER,                  // 0x0060  
		CV_CMD_TEST_TPM,						// 0x0061
		CV_CMD_FINGERPRINT_TEST_OUT,			// 0x0062	
		CV_CMD_ENABLE_WBDI,						// 0x0063
		CV_CMD_SET_CV_ONLY_RADIO_MODE_VOL,		// 0x0064
		CV_CMD_FINGERPRINT_SET,					// 0x0065  
		CV_CMD_FINGERPRINT_CAPTURE_START,		// 0x0066  
		CV_CMD_FINGERPRINT_CAPTURE_WAIT,		// 0x0067  
		CV_CMD_FINGERPRINT_CAPTURE_CANCEL,		// 0x0068  
		CV_CMD_FINGERPRINT_CAPTURE_GET_RESULT,	// 0x0069  
		CV_CMD_FINGERPRINT_CREATE_FEATURE_SET,	// 0x006a  
		CV_CMD_FINGERPRINT_COMMIT_FEATURE_SET,	// 0x006b  
		CV_CMD_FINGERPRINT_UPDATE_ENROLLMENT,	// 0x006c  
		CV_CMD_FINGERPRINT_DISCARD_ENROLLMENT,	// 0x006d  
		CV_CMD_FINGERPRINT_COMMIT_ENROLLMENT,	// 0x006e  
		CV_CMD_FINGERPRINT_CREATE_TEMPLATE,		// 0x006f  
		CV_CMD_FINGERPRINT_ENROLL_DUP_CHK, 		// 0x0070
		CV_CMD_ENUMERATE_OBJECTS_DIRECT, 		// 0x0071
		CV_CMD_FINGERPRINT_VERIFY_W_HASH,		// 0x0072
		CV_CMD_FINGERPRINT_IDENTIFY_W_HASH,		// 0x0073
		CV_CMD_POWER_STATE_SLEEP,				// 0x0074
		CV_CMD_POWER_STATE_RESUME,				// 0x0075
		CV_CMD_DEVICE_STATE_REMOVE,				// 0x0076
		CV_CMD_DEVICE_STATE_ARRIVAL,			// 0x0077
		CV_CMD_FUNCTION_ENABLE,					// 0x0078
		CV_CMD_READ_NFC_CARD_UUID, 				// 0x0079
		CV_CMD_SET_NFC_POWER_TO_MAX,			// 0x007A
		CV_CMD_GET_RPB,							// 0x007B
		CV_CMD_ENABLE_NFC,						// 0x007C
		CV_CMD_SET_NFC_PRESENT,					// 0x007D		
		CV_CMD_HOTP_UNBLOCK,					// 0x007E
		CV_CMD_SET_T4_CARDS_ROUTING,			// 0x007F
		CV_CMD_DETECT_FP,						// 0x0080
		CV_CMD_GET_SBL_PID,						// 0x0081
		CV_CMD_BIOMETRIC_UNIT_CREATED,			// 0x0082
		CV_CMD_GENERATE_ECDSA_KEY,				// 0x0083
		CV_CMD_MANAGE_POA,						// 0x0084
		CV_CMD_SET_DEBUG_LOGGING,				// 0x0085
		CV_CMD_GET_DEBUG_LOGGING,				// 0x0086
		CV_CMD_SET_EC_PRESENCE,					// 0x0087
		CV_CMD_WBF_GET_SESSION_ID,				// 0x0088
        CV_CMD_ERASE_FLASH_CHIP,                // 0x0089
		CV_CMD_ENROLLMENT_STARTED,				// 0x008A
        CV_CMD_FINGERPRINT_CAPTURE_START_MS,    // 0x008B  Capture in Modern Standby Mode
        CV_CMD_HOST_ENTER_CS_STATE,             // 0x008C
        CV_CMD_HOST_EXIT_CS_STATE,              // 0x008D
        CV_CMD_CANCEL_POA_SSO, 					// 0x008E
};

typedef unsigned int cv_interrupt_type;
enum cv_interrupt_type_e {
		CV_COMMAND_PROCESSING_INTERRUPT,
		CV_HOST_STORAGE_INTERRUPT,
		CV_HOST_CONTROL_INTERRUPT,
		CV_FINGERPRINT_EVENT_INTERRUPT,
		CV_CONTACTLESS_EVENT_INTERRUPT
};

typedef unsigned int cv_internal_state;
#define CV_PENDING_WRITE_TO_SC		0x00000001 	/* an object is waiting to be written to the smart card,
							but  cant be completed yet due to smart card not
							available or PIN needed */
#define CV_PENDING_WRITE_TO_CONTACTLESS	0x00000002 	/* an object is waiting to be written to the contactless
							smart card, but  cant be completed yet due to smart
							card not available or PIN needed */
#define CV_IN_DA_LOCKOUT				0x00000004 /* dictionary attack mitigation has initiated a lockout */
#define CV_WAITING_FOR_PIN_ENTRY	0x00000008 	/* authorization testing has determined that a PIN is
							needed and control has returned to calling app to allow
							command to be resubmitted with PIN */
#define CV_IN_ANTIHAMMERING_LOCKOUT			0x00000010  /* actions resulting in 2T bit incrementing (typically
							flash writes) have exceeded allowed rate and will be locked out for a period of time */
#define CV_SESSION_UI_PROMPTS_SUPPRESSED	0x00000020  /* this session has UI prompts suppressed. */
#define CV_ANTIHAMMERING_SUSPENDED			0x00000040	/* if TRUE, allow 2T bump to occur regardless of antihammering */

typedef unsigned int cv_device_state;
#define CV_SMART_CARD_PRESENT		0x00000001 	/* indicates smart card present in reader */
#define CV_CONTACTLESS_SC_PRESENT	0x00000002 	/* indicates a contactless smart card is present */
#define CV_USE_REMOTE_USH_DEVICES	0x00000004 	/* indicates laptop USH in docking station and must */
												/* use RF, SC and FP devices on attached keyboard USH */
#define CV_FP_CAPTURED_FEATURE_SET_VALID	0x00000008 	/* previously captured FP feature set is valid */
#define CV_FP_CAPTURE_CONTEXT_OPEN	0x00000010	/* FP capture context is open and allows FP capture operation */
#define CV_HID_CONTEXT_OPEN			0x00000020	/* HID (RF) context is open and allows HID operation */
#define CV_FP_FINGER_DETECT_ENABLED 0x00000040  /* indicates finger detection is enabled */
#define CV_2T_ACTIVE				0x00000080  /* indicates 2T monotonic counter is active */
#define CV_CV_ONLY_RADIO_OVERRIDE	0x00000100  /* allow RF enable while executing CV commands in CV-only radio mode */
#define CV_ANTIHAMMERING_ACTIVE		0x00000200  /* indicates antihammering is active */
#define CV_DA_ACTIVE				0x00000400  /* indicates dictionary attack protection is active */
#define CV_FP_PERSISTANCE_OVERRIDE	0x00000800 	/* allow FP to persist for cv_fingerprint_capture (for async FP swipe) */
#define CV_CV_ONLY_RADIO_MODE_VOL	0x00001000 	/* volatile state of CV-only radio mode */
#define CV_CV_ONLY_RADIO_MODE_VOL_ACTIVE 0x00002000 	/* indication that volatile version of CV-only radio mode can be used */
#define CV_FP_CAPTURE_IN_PROGRESS	0x00004000 	/* indication that async FP capture is in progress */
#define CV_FP_CAPTURED_IMAGE_VALID	0x00008000 	/* indication that captured FP image is valid */
#define CV_COMMAND_ACTIVE			0x00010000 	/* indication that CV command is active */
#define CV_FP_TEMPLATE_VALID		0x00020000 	/* indication that FP template is available */
#define CV_HAS_CAPTURE_ID           0x00040000  /* indication that async capture has created capture ID */
#define CV_FP_CHECK_CALIBRATION     0x00080000  /* indication that FP device might need to be calibrated */
#define CV_FP_CAPTURE_CANCELLED     0x00100000  /* indication that FP capture has been cancelled */
#define CV_FP_REINIT_CANCELLATION   0x00200000  /* indication that FP capture has been cancelled due to resume or restart */
#define CV_FP_RESTART_HW_FDET       0x00400000  /* indication that FP h/w finger detect needs to be restarted */
#define CV_CANCEL_PENDING_CMD       0x00800000  /* indication that pending CV command should be cancelled */
#define CV_FP_TERMINATE_ASYNC		0x01000000  /* indication that FP async task should be terminated */
#define CV_FIRST_PASS_PBA			0x02000000  /* first pass after resume/restart for PBA */
#define CV_ENUMERATION_ACTIVE		0x04000000  /* USB enumeration is active */
#define CV_CV_ONLY_RADIO_MODE_PEND	0x08000000  /* change to disable persistent CV only radio mode is pending USB enum completion */
#define CV_USH_INITIALIZED			0x10000000  /* USH has populated all devices  */
#define CV_USBD_BLOCK_SUSPEND       0x20000000  /* Block USBD suspend mode */

#define __IS_CV_ONLY_RADIO_MODE_CV() ((CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_MODE_VOL_ACTIVE) \
		? (CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_MODE_VOL) \
		: (CV_PERSISTENT_DATA->deviceEnables & CV_CV_ONLY_RADIO_MODE))
#define IS_CV_ONLY_RADIO_MODE() ((CV_PERSISTENT_DATA->deviceEnables & CV_CV_ONLY_RADIO_MODE) \
	?  __IS_CV_ONLY_RADIO_MODE_CV() \
	: (CV_PERSISTENT_DATA->deviceEnables & CV_CV_ONLY_RADIO_MODE))



typedef unsigned int cv_phx2_device_status;
enum cv_phx2_device_status_e {
	CV_PHX2_OPERATIONAL = 0x00000001,			/* PHX2 ok, no failures detected */
	CV_PHX2_MONOTONIC_CTR_FAIL = 0x00000002,	/* monotonic counter failure */
	CV_PHX2_FLASH_INIT_FAIL = 0x00000004,		/* flash init failure */
	CV_PHX2_FP_DEVICE_FAIL = 0x00000008		/* FP device failure */
};

typedef unsigned int cv_persistent_flags;
#define CV_UCK_EXISTS			0x00000001	/* unique CV key exists */
#define CV_GCK_EXISTS			0x00000002	/* group corporate key exists */
#define CV_HAS_CV_ADMIN			0x00000004	/* authorization data associated with CV administrator exists */
#define CV_HAS_SECURE_SESS_RSA_KEY	0x00000008	/* RSA public key used for secure session setup has been generated */
#define CV_HAS_SECURE_SESS_ECC_KEY	0x00000010	/* ECC public key used for secure session setup has been generated */
#define CV_RTC_TIME_CONFIGURED		0x00000020	/* RTC has been configured to maintain time of day */
#define CV_USE_SUITE_B_ALGS		0x00000040	/* crypto operations must use Suite B algorithms */
#define CV_USE_KDIX_DSA			0x00000080	/* use provisioned DSA key for secure session signing instead of Kdi in NVM */
#define CV_USE_KDIX_ECC			0x00000100	/* use provisioned ECC key for secure session signing instead of Kdi in NVM */
#define CV_HAS_FP_UPEK_SWIPE    0x00000200  /* Attached fingerprint device is UPEK swipe sensor */
#define CV_HAS_FP_UPEK_TOUCH    0x00000400  /* Attached fingerprint device is UPEK touch sensor */
#define CV_HAS_FP_AT_SWIPE      0x00000800  /* Attached fingerprint device is AT swipe sensor */
#define CV_HAS_BIOS_SHARED_SECRET 0x00001000 /* Shared secret with system BIOS has been configured */
#define CV_HAS_SAPP_SHARED_SECRET 0x00002000 /* Shared secret with secure application has been configured */
#define CV_HAS_FAR_VALUE		0x00004000	/* FAR value has been configured */
#define CV_HAS_SUPER_PARAMS		0x00008000	/* SUPER params have been configured */
#define CV_ANTI_REPLAY_OVERRIDE	0x00010000	/* an anti-replay override has been issued */
#define CV_RF_PRESENT_SET		0x00020000	/* indicates a command has set the value of RF present */
#define CV_CV_ONLY_RADIO_MODE_SET 0x00040000	/* indicates a command has set the value of CV only radio mode */
#define CV_AH_CREDITS_RESET		0x00080000	/* indicates antihammering credits should be reset */
#define CV_CV_ONLY_RADIO_MODE_DEFAULT 0x00100000 /* indicates default for CV only radio mode should be reset */
#define CV_AH_CREDITS_RESET_1	0x00200000	/* indicates antihammering credits should be reset (2nd time) */
#define CV_SC_PRESENT_SET		0x00400000	/* indicates a command has set the value of SC present */
#define CV_MANAGED_MODE         0x00800000  /* indicates that CV is managed (ie, CV admin auth and SUPER registration exist) */
#define CV_HAS_VALIDITY_SWIPE	0x01000000	/* Attached fingerprint device is Validity swipe sensor */
#define CV_DA_THRESHOLD_DEFAULT 0x04000000 /* indicates default for DA threshold should be reset */
#define CV_FP_PERSIST_DEFAULT	0x08000000 /* indicates default for FP persistence should be reset */
#define CV_FP_PERSIST_DEFAULT1	0x10000000 /* indicates default for FP persistence should be reset */
#define CV_FP_PERSIST_DEFAULT2	0x40000000 /* indicates default for FP persistence should be reset */
#define CV_FP_SENSOR_CLEAR_MASK	0xFEFFF1FF	/* Mask to clear sensor detect bits */

typedef unsigned int cv_session_flags;
/* bits map to cv_options, where applicable */
#define CV_HAS_PRIVACY_KEY	0x40000000 /* session has specified privacy key to wrap objects*/
#define CV_REMOTE_SESSION	0x80000000	/* remote session (applies to CV Host mode API only) */

typedef unsigned int cv_retry_writes;
#define CV_RETRY_TLD_WRITE		0x00000001
#define CV_RETRY_DIR0_WRITE		0x00000002

typedef unsigned char cv_mscapi_blob_type;
enum cv_mscapi_blob_type_e {
	CV_MSCAPI_SIMPLE_BLOB	= 0x1,
	CV_MSCAPI_PUBKEY_BLOB	= 0x6,
	CV_MSCAPI_PRIVKEY_BLOB	= 0x7
};

typedef unsigned int cv_mscapi_alg_id;
enum cv_mscapi_alg_id_e {
	CV_MSCAPI_CALG_3DES	= 0x00006603,
	CV_MSCAPI_CALG_AES_128	= 0x0000660e,
	CV_MSCAPI_CALG_AES_192	= 0x0000660f,
	CV_MSCAPI_CALG_AES_256	= 0x00006610,
	CV_MSCAPI_CALG_DSS_SIGN = 0x00002200,
	CV_MSCAPI_CALG_DH_EPHEM = 0x0000aa02,
	CV_MSCAPI_CALG_DH_SF	= 0x0000aa01,
	CV_MSCAPI_CALG_RSA_KEYX = 0x0000a400,
	CV_MSCAPI_CALG_RSA_SIGN = 0x00002400,
};

typedef unsigned int cv_mscapi_magic;
enum cv_mscapi_magic_e {
	CV_MSCAPI_MAGIC_RSAPUB		= 0x31415352,
	CV_MSCAPI_MAGIC_RSAPRIV		= 0x32415352,
	CV_MSCAPI_MAGIC_RSADSA		= 0x31535344,
	CV_MSCAPI_MAGIC_RSADH		= 0x31484400,
};

typedef unsigned int cv_admin_auth_permission;
enum cv_admin_auth_permission_e {
	CV_ALLOW_ADMIN_AUTH,
	CV_DENY_ADMIN_AUTH,
	CV_REQUIRE_ADMIN_AUTH,
	CV_REQUIRE_ADMIN_AUTH_ALLOW_PP_AUTH,
	CV_REQUIRE_ADMIN_AUTH_ALLOW_SUPER_AUTH
};

typedef unsigned int cv_hash_stage;
enum cv_hash_stage_e {
	CV_HASH_STAGE_START,
	CV_HASH_STAGE_UPDATE,
	CV_HASH_STAGE_FINAL
};

typedef unsigned int cv_hash_op_internal;
enum cv_hash_op_internal_e {
		/* other values same as cv_hash_op */
		CV_SHA_LEGACY = 100
};

enum cv_swipe_type_e {
	CV_SWIPE_NORMAL,
	CV_SWIPE_FIRST,
	CV_SWIPE_SECOND,
	CV_SWIPE_THIRD,
	CV_SWIPE_FOURTH,
	CV_SWIPE_FIFTH,
	CV_SWIPE_SIXTH,
	CV_SWIPE_SEVENTH,
	CV_SWIPE_EIGHTH,
	CV_SWIPE_NINETH,
	CV_SWIPE_TENTH,
	CV_RESWIPE,
	CV_SWIPE_TIMEOUT
};

enum fpSensorState_e {
	CV_FP_WAS_PRESENT 	= 0x01,			/* WAS present bit is set if fp device is enumerated and never cleared */
	CV_FP_ENUMERATED 	= 0x02
};

typedef unsigned int cv_fp_cap_control_internal;
#define CV_CAP_CONTROL_INTERNAL				0x00000040
#define CV_CAP_CONTROL_RESTART_CAPTURE		0x00000080
#define CV_CAP_CONTROL_NOSESSION_VALIDATION 0x00000100

#define CV_INIT_TOP_LEVEL_DIR				0x00000001
#define CV_INIT_PERSISTENT_DATA				0x00000002
#define CV_INIT_DIR_0						0x00000004
#define CV_INIT_ANTIHAMMERING_CREDITS_DATA	0x00000008

/* default ECC domain parameters P-256 */

#define ECC_DEFAULT_DOMAIN_PARAMS_PRIME \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_A \
	0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_B \
	0x4B, 0x60, 0xD2, 0x27, 0x3E, 0x3C, 0xCE, 0x3B, 0xF6, 0xB0, 0x53, 0xCC, 0xB0, 0x06, 0x1D, 0x65,  \
	0xBC, 0x86, 0x98, 0x76, 0x55, 0xBD, 0xEB, 0xB3, 0xE7, 0x93, 0x3A, 0xAA, 0xD8, 0x35, 0xC6, 0x5A

#define ECC_DEFAULT_DOMAIN_PARAMS_G \
	0x96, 0xC2, 0x98, 0xD8, 0x45, 0x39, 0xA1, 0xF4, 0xA0, 0x33, 0xEB, 0x2D, 0x81, 0x7D, 0x03, 0x77,  \
	0xF2, 0x40, 0xA4, 0x63, 0xE5, 0xE6, 0xBC, 0xF8, 0x47, 0x42, 0x2C, 0xE1, 0xF2, 0xD1, 0x17, 0x6B,  \
	0xF5, 0x51, 0xBF, 0x37, 0x68, 0x40, 0xB6, 0xCB, 0xCE, 0x5E, 0x31, 0x6B, 0x57, 0x33, 0xCE, 0x2B,  \
	0x16, 0x9E, 0x0F, 0x7C, 0x4A, 0xEB, 0xE7, 0x8E, 0x9B, 0x7F, 0x1A, 0xFE, 0xE2, 0x42, 0xE3, 0x4F

#define ECC_DEFAULT_DOMAIN_PARAMS_n \
	0x51, 0x25, 0x63, 0xFC, 0xC2, 0xCA, 0xB9, 0xF3, 0x84, 0x9E, 0x17, 0xA7, 0xAD, 0xFA, 0xE6, 0xBC,  \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_h 0x01

/* default DSA domain parameters */
#define DSA_DEFAULT_DOMAIN_PARAMS_P \
0xd7, 0xa8, 0x5a, 0x0f, 0x67, 0x46, 0x6f, 0x30, 0x66, 0xbe, 0xaa, 0x86, 0x3d, 0xdd, 0x96, 0xc1, \
0xf2, 0x34, 0x76, 0xdf, 0x3a, 0xfc, 0x7e, 0xde, 0x5e, 0x55, 0xb5, 0xd2, 0xe9, 0x16, 0xeb, 0xc5, \
0xd3, 0xb9, 0x25, 0xb4, 0x62, 0xf3, 0xbf, 0x77, 0x34, 0x97, 0xe1, 0x0f, 0x29, 0x42, 0x53, 0x0e, \
0x2f, 0x3a, 0x74, 0x46, 0x70, 0xb9, 0x34, 0xcc, 0x1e, 0x7f, 0xba, 0xd1, 0x82, 0x69, 0x15, 0x8e, \
0x21, 0x30, 0x7f, 0xb7, 0x0e, 0x26, 0x10, 0x62, 0x87, 0xe3, 0x36, 0xe0, 0xf7, 0x24, 0x5a, 0x6e, \
0x0e, 0x31, 0xd9, 0x03, 0x32, 0xad, 0x63, 0x22, 0x6c, 0x1a, 0x66, 0xf3, 0x48, 0x00, 0xac, 0xb2, \
0x76, 0x34, 0x6c, 0xf8, 0xc1, 0x71, 0xed, 0x21, 0xda, 0xc7, 0xc6, 0xc8, 0xcc, 0x6e, 0x73, 0x19, \
0xc2, 0x54, 0x19, 0x28, 0x06, 0xdf, 0x25, 0xff, 0xb4, 0x22, 0x69, 0xda, 0x02, 0xe5, 0x56, 0xb7 
#define DSA_DEFAULT_DOMAIN_PARAMS_Q \
0x8d, 0xe6, 0xc4, 0x7e, 0x06, 0xe3, 0x42, 0x76, 0xc4, 0xc0, 0x5c, 0x5e, 0x82, 0x95, 0xf0, 0x10,  \
0x54, 0xf7, 0x33, 0xdc
#define DSA_DEFAULT_DOMAIN_PARAMS_G \
0xa1, 0xaf, 0xf1, 0x48, 0xa8, 0xc4, 0x24, 0x73, 0x4c, 0x90, 0xd9, 0x6a, 0x65, 0xd8, 0xfb, 0x63, \
0xe9, 0x9f, 0x2e, 0x5e, 0xd5, 0x67, 0x82, 0x29, 0xd1, 0xb1, 0xc7, 0x77, 0x92, 0x18, 0x07, 0x10, \
0x6a, 0x65, 0x86, 0x81, 0x2e, 0xda, 0x2e, 0xee, 0xde, 0x34, 0xf2, 0xd5, 0x24, 0x1c, 0x4f, 0xe8, \
0xc1, 0xab, 0x95, 0x25, 0x73, 0x77, 0x5b, 0xc5, 0x38, 0xd4, 0xc6, 0x1a, 0x52, 0x0b, 0x75, 0x78, \
0xe5, 0x7b, 0x25, 0x02, 0xe9, 0xe4, 0x77, 0x5d, 0x85, 0x3d, 0x35, 0x13, 0x22, 0x32, 0x1c, 0xdc, \
0x67, 0xf3, 0x3b, 0x4d, 0x3d, 0xac, 0xf9, 0x1f, 0xf5, 0x3a, 0x72, 0x13, 0x16, 0xfe, 0x5b, 0x7a, \
0x0b, 0x6c, 0x12, 0x56, 0x6d, 0x61, 0xa3, 0x1a, 0xb6, 0x4d, 0x69, 0x64, 0x89, 0x13, 0xcb, 0x2b, \
0xfe, 0x3f, 0x97, 0x37, 0x33, 0x4e, 0xc0, 0xd0, 0xca, 0x93, 0x15, 0x3f, 0x9c, 0xbb, 0xe2, 0x3f

#if 0
#define DSA_DEFAULT_DOMAIN_PARAMS_P \
	 0x0f, 0x5a, 0xa8, 0xd7, 0x30, 0x6f, 0x46, 0x67, 0x86, 0xaa, 0xbe, 0x66, \
	 0xc1, 0x96, 0xdd, 0x3d, 0xdf, 0x76, 0x34, 0xf2, 0xde, 0x7e, 0xfc, 0x3a, \
	 0xd2, 0xb5, 0x55, 0x5e, 0xc5, 0xeb, 0x16, 0xe9, 0xb4, 0x25, 0xb9, 0xd3, \
	 0x77, 0xbf, 0xf3, 0x62, 0x0f, 0xe1, 0x97, 0x34, 0x0e, 0x53, 0x42, 0x29, \
	 0x46, 0x74, 0x3a, 0x2f, 0xcc, 0x34, 0xb9, 0x70, 0xd1, 0xba, 0x7f, 0x1e, \
	 0x8e, 0x15, 0x69, 0x82, 0xb7, 0x7f, 0x30, 0x21, 0x62, 0x10, 0x26, 0x0e, \
	 0xe0, 0x36, 0xe3, 0x87, 0x6e, 0x5a, 0x24, 0xf7, 0x03, 0xd9, 0x31, 0x0e, \
	 0x22, 0x63, 0xad, 0x32, 0xf3, 0x66, 0x1a, 0x6c, 0xb2, 0xac, 0x00, 0x48, \
	 0xf8, 0x6c, 0x34, 0x76, 0x21, 0xed, 0x71, 0xc1, 0xc8, 0xc6, 0xc7, 0xda, \
	 0x19, 0x73, 0x6e, 0xcc, 0x28, 0x19, 0x54, 0xc2, 0xff, 0x25, 0xdf, 0x06, \
	 0xda, 0x69, 0x22, 0xb4, 0xb7, 0x56, 0xe5, 0x02
#define DSA_DEFAULT_DOMAIN_PARAMS_Q \
	0x7e, 0xc4, 0xe6, 0x8d, 0x76, 0x42, 0xe3, 0x06, 0x5e, 0x5c, 0xc0, 0xc4, \
	0x10, 0xf0, 0x95, 0x82, 0xdc, 0x33, 0xf7, 0x54
#define DSA_DEFAULT_DOMAIN_PARAMS_G \
	0x48, 0xf1, 0xaf, 0xa1, 0x73, 0x24, 0xc4, 0xa8, 0x6a, 0xd9, 0x90, 0x4c, \
	0x63, 0xfb, 0xd8, 0x65, 0x5e, 0x2e, 0x9f, 0xe9, 0x29, 0x82, 0x67, 0xd5, \
	0x77, 0xc7, 0xb1, 0xd1, 0x10, 0x07, 0x18, 0x92, 0x81, 0x86, 0x65, 0x6a, \
	0xee, 0x2e, 0xda, 0x2e, 0xd5, 0xf2, 0x34, 0xde, 0xe8, 0x4f, 0x1c, 0x24, \
	0x25, 0x95, 0xab, 0xc1, 0xc5, 0x5b, 0x77, 0x73, 0x1a, 0xc6, 0xd4, 0x38, \
	0x78, 0x75, 0x0b, 0x52, 0x02, 0x25, 0x7b, 0xe5, 0x5d, 0x77, 0xe4, 0xe9, \
	0x13, 0x35, 0x3d, 0x85, 0xdc, 0x1c, 0x32, 0x22, 0x4d, 0x3b, 0xf3, 0x67, \
	0x1f, 0xf9, 0xac, 0x3d, 0x13, 0x72, 0x3a, 0xf5, 0x7a, 0x5b, 0xfe, 0x16, \
	0x56, 0x12, 0x6c, 0x0b, 0x1a, 0xa3, 0x61, 0x6d, 0x64, 0x69, 0x4d, 0xb6, \
	0x2b, 0xcb, 0x13, 0x89, 0x37, 0x97, 0x3f, 0xfe, 0xd0, 0xc0, 0x4e, 0x33, \
	0x3f, 0x15, 0x93, 0xca, 0x3f, 0xe2, 0xbb, 0x9c
#endif

/* default DH domain parameters TBD */
#define DH_DEFAULT_DOMAIN_PARAMS_G 0x02
#define DH_DEFAULT_DOMAIN_PARAMS_M \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8A, 0xAC, 0xAA, 0x68,  \
	0x15, 0x72, 0x8E, 0x5A, 0x98, 0xFA, 0x05, 0x10, 0x15, 0xD2, 0x26, 0x18,  \
	0xEA, 0x95, 0x6A, 0xE5, 0x39, 0x95, 0x49, 0x7C, 0x95, 0x58, 0x17, 0x18,  \
	0xDE, 0x2B, 0xCB, 0xF6, 0x6F, 0x4C, 0x52, 0xC9, 0xB5, 0xC5, 0x5D, 0xF0,  \
	0xEC, 0x07, 0xA2, 0x8F, 0x9B, 0x27, 0x83, 0xA2, 0x18, 0x0E, 0x86, 0x03,  \
	0xE3, 0x9E, 0x77, 0x2C, 0x2E, 0x36, 0xCE, 0x3B, 0x32, 0x90, 0x5E, 0x46,  \
	0xCA, 0x18, 0x21, 0x7C, 0xF1, 0x74, 0x6C, 0x08, 0x4A, 0xBC, 0x98, 0x04,  \
	0x67, 0x0C, 0x35, 0x4E, 0x70, 0x96, 0x96, 0x6D, 0x9E, 0xD5, 0x29, 0x07,  \
	0x20, 0x85, 0x52, 0xBB, 0x1C, 0x62, 0xF3, 0x56, 0xDC, 0xA3, 0xAD, 0x96,  \
	0x83, 0x65, 0x5D, 0x23, 0xFD, 0x24, 0xCF, 0x5F, 0x69, 0x16, 0x3F, 0xA8,  \
	0x1C, 0x55, 0xD3, 0x9A, 0x98, 0xDA, 0x48, 0x36, 0xA1, 0x63, 0xBF, 0x05,  \
	0xC2, 0x00, 0x7C, 0xB8, 0xEC, 0xE4, 0x5B, 0x3D, 0x49, 0x28, 0x66, 0x51,  \
	0x7C, 0x4B, 0x1F, 0xE6, 0xAE, 0x9F, 0x24, 0x11, 0x5A, 0x89, 0x9F, 0xA5,  \
	0xEE, 0x38, 0x6B, 0xFB, 0xF4, 0x06, 0xB7, 0xED, 0x0B, 0xFF, 0x5C, 0xB6,  \
	0xA6, 0x37, 0xED, 0x6B, 0xF4, 0x4C, 0x42, 0xE9, 0x62, 0x5E, 0x7E, 0xC6,  \
	0xE4, 0x85, 0xB5, 0x76, 0x6D, 0x51, 0xC2, 0x45, 0x4F, 0xE1, 0x35, 0x6D,  \
	0xF2, 0x5F, 0x14, 0x37, 0x30, 0x2B, 0x0A, 0x6D, 0xCD, 0x3A, 0x43, 0x1B,  \
	0xEF, 0x95, 0x19, 0xB3, 0x8E, 0x34, 0x04, 0xDD, 0x51, 0x4A, 0x08, 0x79,  \
	0x3B, 0x13, 0x9B, 0x22, 0x02, 0x0B, 0xBE, 0xA6, 0x8A, 0x67, 0xCC, 0x74,  \
	0x29, 0x02, 0x4E, 0x08, 0x80, 0xDC, 0x1C, 0xD1, 0xC4, 0xC6, 0x62, 0x8B,  \
	0x21, 0x68, 0xC2, 0x34, 0xC9, 0x0F, 0xDA, 0xA2, 0xFF, 0xFF, 0xFF, 0xFF,  \
	0xFF, 0xFF, 0xFF, 0xFF

/* ECDSA and DSA test values for secure sessions */
#define ECDSA_PRIV_TEST_KEY \
	0x15, 0x23, 0xc3, 0xde, 0xed, 0xd5, 0x39, 0xcc, 0x44, 0x95, 0x6b, 0x06, 0xe8, 0x1e, 0x4a, 0xc9, \
	0x6b, 0x2f, 0xf5, 0x12, 0x8d, 0x8a, 0xcf, 0x6c, 0xbc, 0x83, 0xa2, 0x19, 0x30, 0xb6, 0x2d, 0x7b
#define DSA_PRIV_TEST_KEY \
	0x32, 0xa3, 0x3e, 0x2b, 0x68, 0x51, 0x40, 0x72, 0xe7, 0x9e, 0x8a, 0x21, 0x93, 0xe4, 0x03, 0x77, \
	0x29, 0x3a, 0x11, 0x2f

/* post processing types for cvReadContactlessID when type is PIV */
typedef unsigned int cv_post_process_piv_chuid;
enum cv_post_process_piv_chuid_e {
	CV_POST_PROCESS_FASCN,
	CV_POST_PROCESS_SHA1,
	CV_POST_PROCESS_SHA256
};

/*
 * structures
 */
typedef struct td_cv_obj_attribs_hdr {
	uint16_t	attribsLen;	/* length of object attributes in bytes */
	cv_attrib_hdr attrib;	/* header for first attribute in list */
} PACKED cv_obj_attribs_hdr;

typedef struct td_cv_obj_auth_lists_hdr {
	uint16_t	authListsLen;	/* length of object auth lists in bytes */
	cv_auth_list_hdr authList;	/* header for first auth list in list */
} PACKED cv_obj_auth_lists_hdr;

typedef union td_cv_hmac_key
{
	uint8_t SHA1[SHA1_LEN];		/* HMAC-SHA1 key */
	uint8_t SHA256[SHA256_LEN];	/* HMAC-SHA256 key */
} PACKED cv_hmac_key;
typedef cv_hmac_key cv_hmac_val;

typedef struct td_cv_rsa2048_key {
	uint8_t m[RSA2048_KEY_LEN];		/* public modulus */
	uint32_t e;				/* public exponent */
	uint8_t p[RSA2048_KEY_LEN/2];		/* private prime */
	uint8_t q[RSA2048_KEY_LEN/2];		/* private prime */
	uint8_t pinv[RSA2048_KEY_LEN/2];	/* CRT param */
	uint8_t dp[RSA2048_KEY_LEN/2];	/* CRT param */
	uint8_t dq[RSA2048_KEY_LEN/2];	/* CRT param */
} PACKED cv_rsa2048_key;

typedef struct td_cv_dsa1024_key {
	uint8_t p[DSA1024_KEY_LEN];	/* public p */
	uint8_t q[DSA1024_PRIV_LEN];	/* public q */
	uint8_t g[DSA1024_KEY_LEN];	/* public g */
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
	uint8_t y[DSA1024_KEY_LEN];	/* public key */
} PACKED cv_dsa1024_key;

typedef struct td_cv_dsa1024_key_no_dp {
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
	uint8_t y[DSA1024_KEY_LEN];	/* public key */
} PACKED cv_dsa1024_key_no_dp;

typedef struct td_cv_dsa1024_key_priv_only {
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
} PACKED cv_dsa1024_key_priv_only;

typedef struct td_cv_ecc256 {
	uint8_t prime[ECC256_KEY_LEN];		/* prime */
	uint8_t a[ECC256_KEY_LEN];		/* curve parameter */
	uint8_t b[ECC256_KEY_LEN];		/* curve parameter */
	uint8_t G[ECC256_POINT];		/* base point */
	uint8_t n[ECC256_KEY_LEN];		/* order of base point */
	uint8_t h[ECC256_KEY_LEN];		/* cofactor */
	uint8_t x[ECC256_KEY_LEN];		/* private key */
	uint8_t Q[ECC256_POINT];		/* public key */
} PACKED cv_ecc256;

typedef struct td_cv_ecc256_no_dp {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
	uint8_t Q[ECC256_POINT];		/* public key */
} PACKED cv_ecc256_no_dp;

typedef struct td_cv_ecc256_priv_only {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
} PACKED cv_ecc256_priv_only;

typedef struct td_cv_encap_flags {
	unsigned short	secureSession : 1 ;			/* secure session */
	unsigned short	completionCallback : 1 ;	/* contains completion callback */
	unsigned short	host64Bit : 1 ;				/* 64 bit host */
	cv_secure_session_protocol	secureSessionProtocol : 3 ;	/* secure session protocol*/
	unsigned short	USBTransport : 1 ;			/* 1 for USB; 0 for LPC */
	cv_return_type	returnType : 2 ;			/* return type */
	unsigned short	suppressUIPrompts : 1;		/* session has CV_SUPPRESS_UI_PROMPTS set */
	unsigned short	spare : 6;					/* spare */
} PACKED cv_encap_flags;

typedef struct td_cv_encap_command {
	cv_transport_type	transportType;		/* CV_TRANS_TYPE_ENCAPSULATED */
	uint32_t			transportLen;		/* length of entire transport block in bytes */
	cv_command_id		commandID;			/* ID of encapsulated command */
	cv_encap_flags		flags;				/* flags related to encapsulated commands */
	cv_version			version;			/* version of CV which created encapsulated command */
	cv_handle			hSession;			/* session handle */
	cv_status			returnStatus;		/* return status (0, if not applicable) */
	uint8_t				IV[AES_128_IV_LEN];	/* IV for bulk encryption */
	uint32_t			parameterBlobLen;	/* length of parameter blob in bytes */
	uint32_t			parameterBlob;		/* encapsulated parameters (variable length) */
	/* structure is variable length here:
		parameter blob
		secure session HMAC
		padding with zeros for block length, if encrypted		*/
} PACKED cv_encap_command;

typedef struct td_cv_secure_session_get_pubkey_out {
	uint8_t		nonce[CV_NONCE_LEN];	/* anti-replay nonce */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} PACKED cv_secure_session_get_pubkey_out;

typedef struct td_cv_secure_session_get_pubkey_in {
	uint8_t		pubkey[RSA2048_KEY_LEN];/* RSA 2048-bit public key for encrypting host nonce */
	uint8_t		sig[DSA_SIG_LEN];				/* DSA signature of pubkey using Kdi or Kdix (if any) */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} PACKED cv_secure_session_get_pubkey_in;

typedef struct td_cv_secure_session_get_pubkey_in_suite_b {
	uint8_t		pubkey[ECC256_POINT];	/* ECC 256 public key for encrypting host nonce */
	uint8_t		sig[ECC256_SIG_LEN];	/* ECC signature of pubkey using provisioned device ECC signing key */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} PACKED cv_secure_session_get_pubkey_in_suite_b;

typedef struct td_cv_secure_session_host_nonce {
	uint8_t	encNonce[RSA2048_KEY_LEN];	/* host nonce encrypted with CV RSA public key */
} PACKED cv_secure_session_host_nonce;

typedef struct td_cv_secure_session_host_nonce_suite_b {
	uint8_t	nonce[CV_NONCE_LEN];	/* host nonce (unencrypted) */
	uint8_t pubkey[ECC256_POINT];	/* host ECC256 public key */
} PACKED cv_secure_session_host_nonce_suite_b;

typedef struct td_cv_transport_header {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* command ID */
} PACKED cv_transport_header;

typedef struct td_cv_host_storage_get_request {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_GET_REQUEST */
} PACKED cv_host_storage_get_request;

typedef struct td_cv_host_storage_object_entry {
	cv_obj_handle	handle;	/* object/dir ID */
	uint32_t		length;	/* object/dir blob length */
} PACKED cv_host_storage_object_entry;

typedef struct td_cv_host_storage_store_object {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_STORE_OBJECT */
	/* remainder is variable length of the form (see cv_host_storage_object_entry):
		object ID 1
		object length 1
		object blob 1
		...
		object ID n
		object length n
		object blob n			*/
} PACKED cv_host_storage_store_object;


typedef struct td_cv_host_storage_delete_object {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_DELETE_OBJECT */
	cv_obj_handle		objectID;		/* ID of object to be deleted */
} PACKED cv_host_storage_delete_object;

typedef struct td_cv_host_storage_delete_all_files {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_DELETE_ALL_FILES */
} PACKED cv_host_storage_delete_all_files;

typedef struct td_cv_host_storage_retrieve_object_in {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_RETRIEVE_OBJECT */
	/* remainder is variable length of the form:
		object ID 1
		...
		object ID n				*/
} PACKED cv_host_storage_retrieve_object_in;

typedef struct td_cv_host_storage_retrieve_object_out {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_RETRIEVE_OBJECT */
	cv_status			status;			/* return status of retrieve object request */
	/* remainder is variable length of the form:
		object ID 1
		object length 1
		object blob 1
		...
		object ID n
		object length n
		object blob n			*/
} PACKED cv_host_storage_retrieve_object_out;

typedef struct td_cv_host_storage_operation_status {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_OPERATION_STATUS */
	cv_status			status;			/* return status of retrieve object request */
} PACKED cv_host_storage_operation_status;

typedef struct td_cv_host_control_get_request {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_CONTROL */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_CONTROL_GET_REQUEST */
} PACKED cv_host_control_get_request;

typedef struct td_cv_host_control_header_in {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* command ID */
	uint32_t			HMACLen;		/* length of HMAC */
} PACKED cv_host_control_header_in;

typedef struct td_cv_host_control_feature_extraction_in {
	cv_host_control_header_in hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		image length (32 bits)
		image (length given in image length)*/
} PACKED cv_host_control_feature_extraction_in;

typedef struct td_cv_host_control_create_template_in {
	cv_host_control_header_in hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		feature set 1 length (32 bits)
		feature set 1
		feature set 2 length (32 bits)
		feature set 2
		feature set 3 length (32 bits)
		feature set 3
		feature set 4 length (32 bits)
		feature set 4 */
} PACKED cv_host_control_create_template_in;

typedef struct td_cv_host_control_identify_and_hint_creation_in {
	cv_host_control_header_in hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		false acceptance rate (FAR - 32 bits)
		feature set length (32 bits)
		feature set
		number of templates (32 bits)
		template 1 length (32 bits)
		template 1
		template 2 length (32 bits)
		template 2
		template n length (32 bits)
		template n */
} PACKED cv_host_control_identify_and_hint_creation_in;

typedef struct td_cv_host_control_header_out {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* command ID */
	cv_status			status;			/* CV_SUCCESS or error code */
	uint32_t			HMACLen;		/* length of HMAC */
} PACKED cv_host_control_header_out;

typedef struct td_cv_host_control_feature_extraction_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		feature set length (32 bits)
		feature set (length given in feature set length)*/
} PACKED cv_host_control_feature_extraction_out;

typedef struct td_cv_host_control_create_template_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		template length (32 bits)
		template (length given in template length)*/
} PACKED cv_host_control_create_template_out;

typedef struct td_cv_host_control_identify_and_hint_creation_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		hint length (32 bits)
		hint (length given in hint length)
		template index (32 bits) */
} PACKED cv_host_control_identify_and_hint_creation_out;

/* for interruptType of CV_FINGERPRINT_EVENT_INTERRUPT, resultLen is replaced by the capture result type */
typedef uint32_t cv_capture_result_type;
enum cv_capture_result_type_e {
	CV_CAP_RSLT_SUCCESSFUL,
	CV_CAP_RSLT_TOO_HIGH,
	CV_CAP_RSLT_TOO_LOW,
	CV_CAP_RSLT_TOO_LEFT,
	CV_CAP_RSLT_TOO_RIGHT,
	CV_CAP_RSLT_TOO_FAST,
	CV_CAP_RSLT_TOO_SLOW,
	CV_CAP_RSLT_POOR_QUALITY,
	CV_CAP_RSLT_TOO_SKEWED,
	CV_CAP_RSLT_TOO_SHORT,
	CV_CAP_RSLT_MERGE_FAILURE,
	CV_CAP_RSLT_FP_DEV_RESET,
	CV_CAP_RSLT_IOCTL_FAIL,
	CV_CAP_RSLT_CANCELLED
};

typedef struct td_cv_usb_interrupt {
	cv_interrupt_type	interruptType;	/* CV_COMMAND_PROCESSING_INTERRUPT or CV_HOST_STORAGE_INTERRUPT */
	uint32_t			resultLen;		/* length of result to read */
} PACKED cv_usb_interrupt;

typedef struct td_cv_dir_page_entry_dir {
	cv_obj_handle	hDirPage;	/* handle of directory page */
	uint32_t		counter;	/* anti-replay counter value */
} PACKED cv_dir_page_entry_dir;

typedef struct td_cv_dir_page_entry_persist {
	cv_dir_page_entry_dir	entry;				/* directory entry */
	int32_t					multiBufferIndex;	/* multi-buffering index */
	uint32_t				len;				/* length of persistent data */
} PACKED cv_dir_page_entry_persist;

typedef struct td_cv_dir_page_entry_tpm {
	uint32_t		counter;			/* anti-replay counter value */
	uint32_t		multiBufferIndex;	/* multi-buffering index */
} PACKED cv_dir_page_entry_tpm;

typedef struct td_cv_dir_page_hdr {
	uint32_t				numEntries;		/* number of entries in this directory page */
	cv_dir_page_entry_dir	thisDirPage;	/* handle and counter for this dir page */
	/* remainder is variable length of the form:
			directory page entry 1
			...
			directory page entry n		*/
} PACKED cv_dir_page_hdr;

typedef struct td_cv_tld_page_hdr {
	uint32_t				antiHammeringCredits;	/* credits allowing for 2T usage */
	cv_dir_page_entry_dir	thisDirPage;			/* handle and counter for this dir page */
} PACKED cv_tld_page_hdr;

typedef struct td_cv_obj_storage_attributes {
	unsigned short	hasPrivacyKey : 1 ;		/* object has privacy key encryption */
	cv_storage_type	storageType : 4 ;		/* type of device for object storage */
	unsigned short	PINRequired : 1 ;		/* storage device requires PIN */
	unsigned short	reqHandle : 1 ;			/* flag indicating specific handle requested */
	unsigned short	replayOverride : 1 ;	/* flag indicating flag indicating anti-replay check should be overriden */
	unsigned short	spare : 8 ;			/* spare */
	cv_obj_type		objectType: 16;			/* object type */
} PACKED cv_obj_storage_attributes;

typedef struct td_cv_dir_page_entry_obj {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
} PACKED cv_dir_page_entry_obj;

typedef struct td_cv_dir_page_entry_obj_flash {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
	uint32_t		objLen;					/* length of object */
	uint8_t			*pObj;					/* pointer to object in flash */
} PACKED cv_dir_page_entry_obj_flash;

// not necessary?
//typedef PACKED struct td_cv_dir_page_entry_dir0 {
//	cv_obj_handle	hObj;					/* handle of object */
//	uint32_t		counter;				/* anti-replay counter value */
//	uint32_t		objLen;					/* length of object */
//	byte			*pObj;					/* pointer to object in flash */
//} cv_dir_page_entry_dir0;

typedef struct td_cv_dir_page_0 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj_flash dir0Objs[MAX_DIR_PAGE_0_ENTRIES];	/* object entries for directory page 0 */
} PACKED cv_dir_page_0;

typedef struct td_cv_dir_page_level1 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_dir dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 1 directory page */
} PACKED cv_dir_page_level1;

typedef struct td_cv_dir_page_level2 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 2 directory page */
} PACKED cv_dir_page_level2;

typedef struct td_cv_top_level_dir {
	cv_tld_page_hdr			header;										/* handle and count associated with top level directory */
	uint8_t					pageMapFull[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page full (1) or not (0) */
	uint8_t					pageMapExists[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page exists (1) or not (0) */
	cv_dir_page_entry_dir	dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* lower level directories contained in top level */
	cv_dir_page_entry_persist	persistentData;							/* entry associated with persistent data */
	int32_t				dir0multiBufferIndex;							/* multi buffer index for directory 0 */
	cv_dir_page_entry_tpm	dirEntryTPM;								/* anti-replay count and multi-buffering index for TPM */
} PACKED cv_top_level_dir;

typedef struct td_cv_obj_dir_page_blob_enc {
	cv_obj_handle	hObj;						/* handle for object or directory page */
	uint32_t		counter;					/* anti-replay counter value for this blob */
} PACKED cv_obj_dir_page_blob_enc;


typedef struct td_cv_obj_dir_page_blob {
	cv_version		version;					/* version of CV used to create blob */
	uint32_t		encAreaLen;					/* length in bytes of encrypted area */
	byte			IV[AES_128_IV_LEN];			/* IV for AES-CTR mode */
	byte			IVPrivKey[AES_128_IV_LEN];	/* IV for AES-CTR mode for privacy key */
	uint32_t		objPageDirLen;				/* length of object/directory page */
	/* start of encrypted area */
	cv_obj_dir_page_blob_enc encArea;			/* encrypted header */
	/* remainder is variable length of the form:
			encrypted object/diretory page
			encrypted HMAC
			(padded to AES-128 block size with zeroes)    */
} PACKED cv_obj_dir_page_blob;

typedef struct td_cv_session {
	uint32_t			magicNumber;							/* s/b 'SeSs' to identify this as session */
	cv_session_flags	flags;									/* various flags associated with session */
	uint8_t				appUserID[SHA1_LEN];					/* hash of app ID and user ID */
	uint8_t				privacyKey[AES_128_BLOCK_LEN];			/* Key generated from objAuth value when session is created.
																	This is used as a privacy wrapper on top of UCK */
	uint8_t				secureSessionKey[AES_128_BLOCK_LEN];	/* Symmetric key used to encrypt/decrypt parameters for
																	shared sessions */
	cv_hmac_key			secureSessionHMACKey;					/* HMAC key (can be SHA1 or SHA256) (& temp storage for client nonce
																	during session setup) */
	uint32_t			secureSessionUsageCount;				/* 1-based count incremented whenever shared session is used
																	to encrypt parameters (inbound or outbound) for anti-replay */
	cv_obj_handle		nextEnumeratedHandle;					/* for multi-stage enumeration, this is handle for next enumeration */
} PACKED cv_session;

typedef struct td_cv_volatile_obj_dir_page_entry {
	cv_obj_handle	hObj;		/* handle of volatile object */
	uint8_t			*pObj;		/* pointer to volatile object in scratch RAM */
	cv_session		*hSession;	/* session ID which created this object */
	uint8_t			appUserID[20];	/* hash of app ID and user ID fields for session */
} PACKED cv_volatile_obj_dir_page_entry;

typedef struct td_cv_volatile_obj_dir {
	cv_volatile_obj_dir_page_entry	volObjs[MAX_CV_NUM_VOLATILE_OBJS];	/* list of page entries */
} PACKED cv_volatile_obj_dir;

typedef struct td_cv_obj_cache {
	uint8_t cache[CV_OBJ_CACHE_SIZE];
} cv_obj_cache;

typedef struct td_cv_l1dir_cache {
	uint8_t cache[CV_LEVEL1_DIR_PAGE_CACHE_SIZE];
} cv_l1dir_cache;

typedef struct td_cv_l2dir_cache {
	uint8_t cache[CV_LEVEL2_DIR_PAGE_CACHE_SIZE];
} cv_l2dir_cache;

typedef struct td_cv_cmdbuf {
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE];
} cv_cmdbuf;

typedef struct td_cv_fp_small_heap_buf {
	uint32_t buf[ALIGN_DWORD(FP_CAPTURE_SMALL_HEAP_SIZE)/4];
} cv_fp_small_heap_buf;

typedef struct td_cv_hid_small_heap_buf {
	uint32_t buf[ALIGN_DWORD(HID_SMALL_HEAP_SIZE)/4];
} cv_hid_small_heap_buf;

typedef struct td_cv_hid_large_heap_buf {
	uint32_t buf[ALIGN_DWORD(HID_LARGE_HEAP_SIZE)/4];
} cv_hid_large_heap_buf;

typedef struct td_cv_persisted_fp {
	/* size of feature set + size of CV object info */
	uint8_t buf[MAX_FP_FEATURE_SET_SIZE + sizeof(cv_obj_value_fingerprint) - 1];
} cv_persisted_fp;


typedef struct td_cv_secure_iobuf {
	/* this buffer has the CV IO buffer followed by the FP capture/FP match heap */
	uint32_t buf[ALIGN_DWORD((2*CV_IO_COMMAND_BUF_SIZE)
#ifdef USH_BOOTROM  /*AAI */
				+ FP_CAPTURE_LARGE_HEAP_SIZE
#endif /* USH_BOOTROM */
		)/4];
} cv_secure_iobuf;

#ifdef LYNX_C_MEM_MODE
#define CV_SECURE_IO_FP_ASYNC_BUF   (USH_ADDR_2_UNCACHED(CV_SECURE_IO_COMMAND_BUF + (CV_IO_COMMAND_BUF_SIZE)))
#define FP_CAPTURE_LARGE_HEAP_PTR	(USH_ADDR_2_CACHED(CV_SECURE_IO_COMMAND_BUF + 2*CV_IO_COMMAND_BUF_SIZE))
#else
#define CV_SECURE_IO_FP_ASYNC_BUF   (CV_SECURE_IO_COMMAND_BUF + (CV_IO_COMMAND_BUF_SIZE))
#define FP_CAPTURE_LARGE_HEAP_PTR	(CV_SECURE_IO_COMMAND_BUF + 2*CV_IO_COMMAND_BUF_SIZE)
#endif

typedef struct td_cv_hdotp_skip_table {
	uint16_t	tableEntries;
	uint16_t	spare;
	uint32_t	table[CV_MAX_HDOTP_SKIP_TABLE_ENTRIES];
} cv_hdotp_skip_table;
/* the buffer below is to read/write the HDOTP skip table or to write the TLD.  this is used instead of the */
/* CV_SECURE_IO_COMMAND_BUF, because these writes may happen in response to a TPM command while a CV command */
/* is in the CV_SECURE_IO_COMMAND_BUF */
#define HDOTP_AND_TLD_IO_BUF (FP_CAPTURE_LARGE_HEAP_PTR + sizeof(cv_hdotp_skip_table) + 256 /* buffer from HDOTP table */)


typedef struct td_cv_hid_seos_heap_buf {
	uint32_t buf[ALIGN_DWORD(HID_SEOS_HEAP_SIZE)/4];
} cv_hid_seos_heap_buf;


#undef USE_CRMU_TIMER_FOR_TIME_BETWEEN_MATCH_AND_CONTROLUNIT_API

typedef struct td_cv_use_wbf_for_protecting_objects {
	uint8_t					sessionID[WBF_CONTROL_UNIT_SESSION_ID_SIZE];
	cv_obj_handle			template_id_of_identified_fingerprint;
	uint32_t				timeoutForObjectAccess; /* in seconds */
	uint32_t				didStartCRMUTimer;
	uint32_t				CRMUElapsedTime;
	uint32_t				startTimeCountForSimpleTimer;
} PACKED cv_use_wbf_for_protecting_objects;

#define CV_POA_AUTH_NOT_DONE			0x00
#define CV_POA_AUTH_SUCCESS_COMPLETE    0x01
#define CV_POA_AUTH_USED_1              0x02
#define CV_POA_AUTH_USED_2              0x04
#define CV_POA_AUTH_USED_3              0x08
#define CV_POA_AUTH_USED_MASK    (CV_POA_AUTH_USED_1 | CV_POA_AUTH_USED_2 | CV_POA_AUTH_USED_3)
#define CV_POA_AUTH_TIMEOUT             45 // keeping the credential for maximum 45 seconds 

typedef struct td_cv_volatile {
	uint16_t				DAFailedAuthCount;	/* dictionary attack failed auth count */
	uint16_t				DALockoutCount;		/* dictionary attack lockout count */
	uint32_t				DALockoutStart;		/* start time for lockout period */
	cv_internal_state		CVInternalState;	/* CV internal state */
	cv_device_state			CVDeviceState;		/* CV device state */
	cv_phx2_device_status	PHX2DeviceStatus;	/* PHX2 device status */
	cv_session				*secureSession;		/* temporary pointer for secure session being set up */
	uint8_t					objCacheMRU[MAX_CV_OBJ_CACHE_NUM_OBJS];	/* most recently used list for obj cache */
	uint8_t					level1DirCacheMRU[MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES];	/* MRU list for dir pages containing dirs */
	uint8_t					level2DirCacheMRU[MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES];	/* MRU list for dir pages containing objs */
	cv_obj_handle			objCache[MAX_CV_OBJ_CACHE_NUM_OBJS];					/* handles of objects in object cache */
	cv_obj_handle			level1DirCache[MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES];		/* handles of objects in directory page directory cache */
	cv_obj_handle			level2DirCache[MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES];		/* handles of objects in directory page object cache */
	uint8_t					*HIDCredentialPtr;	 /* for HID credential read in */
	cv_contactless_type		credentialType;		 /* type of contactless credential pointed to by HIDCredentialPtr */
	uint32_t				fpPersistanceStart;	 /* start time for FP feature set persistence */
	uint32_t				fpPersistedFeatureSetLength; /* length in bytes of persisted feature set */
	uint8_t					fpHostControlNonce[SHA256_LEN]; /* nonce for FP requests sent to host */
	cv_bool					asyncFPEventProcessing;/* TRUE if processing async FP event */
	uint32_t				moreInfoErrorCode;		/* device or vendor specific error code */
	uint8_t					superQ[CV_SUPER_LENGTH];		/* challenge for SUPER auth algorithm */
	uint8_t					superQPrime[CV_SUPER_LENGTH];	/* challenge inverse */
	cv_space				space;						/* memory allocation info (only flash portion is persistent) */
	uint32_t				count;						/* 2T count */
	uint32_t				predictedCount;				/* predicted 2T count */
	uint32_t				tldMultibufferIndex;		/* index for TLD multibuffering */
	uint32_t				actualCount;				/* counter value from object in case of mismatch */
	uint32_t				timeLastAHCreditsUpdate;	/* time last antihammering credits updated */
	uint32_t				timeLast2Tbump;				/* time last 2T allowed to bump */
	uint32_t				fpSensorState;				/* Bits to indicate fp sensor presence */
	cv_retry_writes			retryWrites;				/* bit set if this is a retry */
	uint8_t					identifyUserCredential[CV_MAX_CACHED_CREDENTIAL_LENGTH];	/* cache for contactless credential */
	uint32_t				initType;					/* used to determine if virgin init required */
	cv_hdotp_skip_table		*hdotpSkipTablePtr;			/* pointer to HDOTP skip table */
	uint32_t				timeLastPBAImageWrite;		/* time of last PBA image write - for antihammering */
	uint32_t				timeLastFPConfigWrite;		/* time of last FP config write - for antihammering */
	uint32_t				timeLastRFIDParamsWrite;	/* time of last RFID params write - for antihammering */

	cv_version curCmdVersion;	/* The current cmd version is saved here */
	cv_st_challenge			stChallenge;				/* last ST challenge (used in response validation) */
	uint32_t				fp_polling_period;
	uint32_t				fp_actual_polling_period;	/* Variable used by fp_api */
	uint32_t				extraCount;					/* if TRUE, monotonic counter bumped twice at init */
	cv_nonce				captureID;					/* nonce identifying initiator of capture */
	uint32_t				fpPersistedFPImageLength;   /* length in bytes of persisted FP image */
#ifdef USH_BOOTROM  /*AAI */
	TASK_HANDLE				highPrioFPCaptureTaskHandle;/* handle to task kicked off by h/w finger detect to capture FP */
	MUTEX_HANDLE			fingerDetectMutexHandle;	/* handle to mutex triggered by finger detect */
	MUTEX_HANDLE			fingerDetectMutexHandleForWakeup;	/* handle to mutex for deferred wakeup */
#else
	void *					highPrioFPCaptureTaskHandle;/* handle to task kicked off by h/w finger detect to capture FP */
	void *					fingerDetectMutexHandle;	/* handle to mutex triggered by finger detect */
	void *					fingerDetectMutexHandleForWakeup;	/* handle to mutex for deferred wakeup */
#endif
	uint32_t				fingerDetectStatusForWakeup;/* FP status at finger detect time to use before wakeup of async task */
	uint32_t				fpCaptureStatus;			/* status of latest FP capture */
	cv_fp_cap_control		fpCapControl;				/* flags pertaining to FP capture */
	uint32_t                enrollmentFeatureSetLens[3]; /* lengths of persisted feature sets to be used for enrollment */
	cv_nonce				enrollmentID;				/* nonce identifying initiator of enrollment */
	unsigned int			lastPoll;                   /* last time low res int poll was called (for CLSC) */
	cv_fp_cap_control		fpCapControlLastOSPresent;	/* retained values of FP cap control for restart */
	uint8_t					keyCacheVld;				/* key cache is valid */
	byte					keyCache[256];				/* 2048b RSA Key Cache */
	uint8_t					contactlessCardDetectEnable; /* CV api to keep track of contactless smart card insert and remove */
	volatile uint8_t		contactlessCardDetected;    /* CV api to keep track of contactless smart card insert and remove */
	uint32_t				nfcRegPu;    /* keep track of the NFC power toggle */
	cv_use_wbf_for_protecting_objects wbfProtectData;
	uint8_t					doingFingerprintEnrollmentNow;
	uint8_t					poaAuthSuccessFlag;  /* flag to keep track of poa authentication */
    uint8_t                 poaFailed;           /* flag to track the POA Matching result */
	unsigned int				poaAuthSuccessTemplate; /* place holder for tracking authenticated template */ 
	uint32_t				poaAuthTimeout;    
	uint32_t				cvTimer2Start;    /* keep track CV Timer2 values  */
	uint32_t				cvTimer2ElapsedTime;    /* keep track CV Timer2 values  */

} PACKED cv_volatile;

typedef struct td_cv_persistent {
	cv_persistent_flags	persistentFlags;			/* CV persistent flags */
	cv_policy			cvPolicy;					/* CV policy */
	byte				UCKKey[AES_128_BLOCK_LEN];	/* Unique CV encryption key used for encrypting objects within security
											boundary of CV, but stored externally */
	byte				GCKKey[AES_128_BLOCK_LEN];	/* Optional group corporate encryption key used for encrypting UCK for
											storage on hard drive */
	cv_rsa2048_key		rsaSecureSessionSetupKey;	/* RSA key used for secure session setup */
	cv_ecc256_no_dp		eccSecureSessionSetupKey;	/* ECC key used for secure session setup */
	union {
		cv_dsa1024_key_priv_only dsaKey;
		cv_ecc256_no_dp eccKey;
	} PACKED secureSessionSigningKey;	/* key used for signing secureSessionSetupKey */
	int32_t				localTimeZoneOffset;		/* offset +/- of local time from GMT */
	uint32_t			DAInitialLockoutPeriod;		/* configured lockout period for dictionary attack processing */
	uint16_t			DAAuthFailureThreshold;		/* Configured count of auth failures before lockout */
	uint32_t			pwrSaveStartIdleTimeMs;		/* Stores the idle time to start pwr savings in ms */
	uint32_t			USH_FW_version;			/* Stores the current FW vesion */
	uint32_t			spare1[2];
	uint8_t				BIOSSharedSecret[SHA1_LEN];/* shared secret with BIOS */
	uint8_t				secureAppSharedSecret[SHA256_LEN];/* shared secret with secure application */
	uint32_t			fpPollingInterval;			/* interval in ms for polling for finger detect */
	uint32_t			fpPersistence;				/* interval in ms for fingerprint to pesist after swipe occurs */
	uint32_t			identifyUserTimeout;		/* timeout in ms for cv_identify_user command */
	int32_t				FARval;						/* false acceptance rate value for FP processing */
	cv_hash_type		fpAntiReplayHMACType;		/* fingerprint anti-replay HMAC type */
	cv_super_config		superConfigParams;			/* parameters for SUPER auth algorithm */
	cv_device_enables	deviceEnables;				/* enables for USH devices */
	uint32_t			systemID;					/* system ID */
	uint8_t				superRSN[CV_SUPER_RSN_LEN]; /* SUPER hardware unique ID */
	uint32_t			superCtr;					/* SUPER counter */
#ifdef PHX_POA_BASIC
	uint8_t             poa_enable;                 /* 0, disabled; 1, enable POA feature */
	uint8_t             poa_timeout;                /* timeout for POA match routione */
	uint8_t             poa_max_failedmatch;        /* the max times of the failed matches for each poa wakeup */
	cv_logging_enables  loggingEnables;				/* enables for logging features */
	uint8_t             spare[76];
#else
	cv_logging_enables  loggingEnables; 			/* enables for logging features */
	uint8_t             spare[79];
#endif
	/* following item is variable length, but has maximum size indicated by array */
	uint32_t			CVAdminAuthListLen;			/* Length of CV admin auth list */
	cv_auth_list		CVAdminAuthList;			/* CV admin auth list */
	uint8_t				authList[MAX_CV_ADMIN_AUTH_LISTS_LENGTH];
#ifdef PHX_NEXT_OTP
	uint32_t            next_otp_valid;
	uint8_t             next_otp[596];
#endif

} cv_persistent;

#ifndef USH_BOOTROM  /* SBI */
cv_cmdbuf cv_cmdbuf_ptr;
cv_secure_iobuf cv_secure_iobuf_ptr;
cv_volatile cv_volatile_data;

#define CV_COMMAND_BUF   ((uint8_t *)&cv_cmdbuf_ptr)
//#define CV_SECURE_IO_COMMAND_BUF  ((cv_secure_iobuf *)&cv_secure_iobuf_ptr)
#define CV_SECURE_IO_COMMAND_BUF  ((uint8_t *)&cv_secure_iobuf_ptr)

#define CV_VOLATILE_DATA  ((cv_volatile *)&cv_volatile_data)

#endif /* USH_BOOTROM */



typedef struct td_cv_obj_properties {
	cv_obj_storage_attributes attribs;		/* attributes related to object storage */
	cv_obj_handle	objHandle;				/* object handle */
	cv_session		*session;				/* session being used to access object */
	uint32_t		PINLen;					/* length of PIN associated with object */
	uint8_t			*PIN;					/* PIN associated with object */
	uint32_t		objAttributesLength;	/* length of attributes, if supplied */
	cv_obj_attributes *pObjAttributes;		/* object attributes */
	uint32_t		authListsLength;		/* length of auth lists, if supplied */
	cv_auth_list	*pAuthLists;			/* object auth lists */
	uint32_t		objValueLength;			/* length of object value, if supplied */
	void			*pObjValue;				/* object value */
	uint8_t			*pIVPriv;				/* IV for privacy encrypted object */
	uint8_t			*altBuf;				/* alternate buffer for object wrapping */
	cv_version		version;					/* version of CV used to create blob */
} PACKED cv_obj_properties;

typedef struct td_cv_export_blob {
	cv_version		version;					/* version of CV used to create blob */
	uint8_t			label[16];					/* user-provided label */
	cv_obj_handle	handle;						/* handle of object */
	cv_obj_storage_attributes storageAttribs;	/* object storage attributes */
	uint32_t		symKeyEncAreaLen;			/* length of encrypted symmetric key (if any) in bytes */
	uint32_t		encAreaLen;					/* length in bytes of symmetric encrypted area */
	uint32_t		objLen;						/* length of object */
	/* remainder is variable length of the form:
			IV for privacy key (16 bytes, only present if indicated in object storage attributes)
			embedded handles list length (32-bits)
			embedded handles list
			IV (length based on symmetric encryption algorithm)
			asymmetric encrypted symmetric key (if any)
			encrypted object (padded to AES-128 block size with zeroes)
			encrypted HMAC		*/
} PACKED cv_export_blob;

typedef struct td_cv_mscapi_blob {
	cv_mscapi_blob_type	bType;	/* blob type */
	uint8_t				bVersion;			/* blob version */
	uint16_t			reserved;			/* not used */
	cv_mscapi_alg_id	aiKeyAlg;			/* algorithm identifier for wrapped key */
	/* remainder is variable length of the form:
			blob type-specific header
			key		*/
} PACKED cv_mscapi_blob;

typedef struct td_cv_mscapi_simple_blob_hdr {
	cv_mscapi_alg_id	aiKeyAlg;			/* encryption algorithm used to encrypt key */
} PACKED cv_mscapi_simple_blob_hdr;

typedef struct td_cv_mscapi_rsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in modulus */
	uint32_t		pubexp;	/* public exponent */
} PACKED cv_mscapi_rsa_blob_hdr;

typedef struct td_cv_mscapi_dsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime, P */
} PACKED cv_mscapi_dsa_blob_hdr;

typedef struct td_cv_mscapi_dh_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime modulus, P */
} PACKED cv_mscapi_dh_blob_hdr;

typedef struct td_cv_param_list_entry {
	cv_param_type	paramType;	/* type of parameter */
	uint32_t		paramLen;	/* length of parameter */
	uint32_t		param;		/* parameter (variable length) */
} PACKED cv_param_list_entry;

typedef struct td_cv_internal_callback_ctx {
	cv_encap_command	*cmd;	/* command being processed */
} PACKED cv_internal_callback_ctx;

typedef struct td_cv_input_parameters {
	uint32_t numParams;	/* number of input parameters */
	struct {
		uint32_t paramLen;	/* length of parameter */
		void *param;		/* pointer to parameter */
	} PACKED paramLenPtr[MAX_INPUT_PARAMS];
} PACKED cv_input_parameters;

typedef struct td_cv_fp_callback_ctx {
	cv_callback		*callback;	/* callback routine passed in with API */
	cv_callback_ctx context;	/* context passed in with API */
	uint32_t		swipe;		/* type of fingerprint swipe */
} PACKED cv_fp_callback_ctx;

/* flash size structures */
#define CV_NUM_FLASH_MULTI_BUFS 2		/* number of multiple buffers for flash object where necessary (top level */
										/* directory and persistent data) */
#define CV_FLASH_MULTI_BUF_MASK (CV_NUM_FLASH_MULTI_BUFS - 1)
typedef struct td_cv_flash_dir_page_0_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_dir_page_0			dirPage0;					/* directory page 0 */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
} PACKED cv_flash_dir_page_0_entry;

typedef struct td_cv_flash_dir_page_0 {
	cv_flash_dir_page_0_entry dir_page_0[CV_NUM_FLASH_MULTI_BUFS];
} PACKED cv_flash_dir_page_0;

typedef struct td_cv_flash_top_level_dir_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_top_level_dir		topLevelDir;				/* top level dir */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} PACKED cv_flash_top_level_dir_entry;

typedef struct td_cv_flash_top_level_dir {
	cv_flash_top_level_dir_entry top_level_dir[CV_NUM_FLASH_MULTI_BUFS];
} PACKED cv_flash_top_level_dir;

typedef struct td_cv_persistent_data_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_persistent			persistentData;				/* top level dir */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} PACKED cv_flash_persistent_data_entry;

typedef struct td_cv_flash_persistent_data {
	cv_flash_persistent_data_entry persistentData[CV_NUM_FLASH_MULTI_BUFS];
} cv_flash_persistent_data;

typedef struct td_cv_antihammering_credit {
	uint32_t credits;		/* number of 2T credits */
	uint32_t counter;		/* 2T counter when written */
} cv_antihammering_credit;

typedef struct td_cv_flash_antihammering_credit_data_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_antihammering_credit	creditData;					/* antihammering credit data */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} cv_flash_antihammering_credit_data_entry;

typedef struct td_cv_flash_antihammering_credit_data {
	cv_flash_antihammering_credit_data_entry antihammeringCreditData;
} cv_flash_antihammering_credit_data;

typedef struct td_cv_hdotp_skip_table_blob {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_hdotp_skip_table		skipTable;					/* HDOTP skip table */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} cv_hdotp_skip_table_blob;

typedef struct td_cv_rfid_params_flash_blob {
	uint32_t	magicNumber;	/* magic number ("rFiD") identifying flash blob */
	uint32_t	paramsID;		/* ID associated with stored parameters */
	uint32_t	length;			/* length in bytes of stored parameters */
	/* parameter blob is here */
} cv_rfid_params_flash_blob;
#define CV_MAX_RFID_PARAMS_LENGTH (1024 - sizeof(cv_rfid_params_flash_blob))

typedef struct td_cv_hdotp_skip_table_block {
	cv_hdotp_skip_table_blob skipTableBlob[2];			/* double-buffered */
} cv_hdotp_skip_table_block;

#define CV_PERSISTENT_OBJECT_HEAP_LENGTH (222*1024) /* 222K */
typedef unsigned int cv_enables_2TandAH;
#define CV_ENABLES_2TANDAH_COOKIE_MASK		0xFFFF0000 	/* this field should be '2T' */
#define CV_ENABLES_2T                       0x00000001  /* 2T enabled */
#define CV_ENABLES_AH                       0x00000002  /* AH enabled */
#define CV_ENABLES_USE_DEFAULTS	            0x00000004  /* use 2T and AH and DA defaults */
#define CV_ENABLES_DA                       0x00000008  /* DA enabled */

typedef unsigned int cv_hdotp_read_limit;
#define CV_HDOTP_READ_LIMIT_MASK			0xFFFF0000 	/* this field should be 'HD' */

typedef struct td_flash_cv {
	uint8_t						spare1a[512];		/* spare to allow for growth of persistent object heap */
	cv_hdotp_read_limit			hdotpReadLimit1;	/* hdotp read limit - buffer 1 */
	uint8_t						spare1b[512];		/* spare to allow for growth of persistent object heap */
	cv_hdotp_read_limit			hdotpReadLimit2;	/* hdotp read limit - buffer 2 */
	uint8_t						spare1c[3*1024 - 2*sizeof(cv_hdotp_read_limit)]; /* spare to allow for growth of persistent object heap */
	uint8_t						persistentObjectHeap[CV_PERSISTENT_OBJECT_HEAP_LENGTH];
	cv_flash_dir_page_0			dirPage0;
	uint8_t						spare2[1024];		/* spare to allow for growth of dirPage0 */
	cv_flash_persistent_data	persistentData;
#ifdef PHX_NEXT_OTP
	uint8_t						spare3[848];		/* spare to allow for growth of persistent data */
#else
    uint8_t                     spare3[2048];       /* spare to allow for growth of persistent data */
#endif
	cv_flash_top_level_dir		topLevelDir;
	uint8_t						spare4[844];		/* spare to allow for growth of top level dir */
	cv_enables_2TandAH			enables2TandAH;		/* enables for 2T and AH (engr only) */
	uint32_t					tldMultiBufIndex;	/* multibuffer index only used in 2T disabled mode */
	cv_flash_antihammering_credit_data ahCreditData;
} flash_cv;

#define CV_DIR_PAGE_0_FLASH_OFFSET ((uint32_t)&(((cv_flash_dir_page_0 *)0)->dir_page_0[1])) /* offset between buffers for multi-buffering (max size) */
#define CV_PERSISTENT_DATA_FLASH_OFFSET ((uint32_t)&(((cv_flash_persistent_data *)0)->persistentData[1]))	/* offset between buffers for multi-buffering (max size) */
#define CV_TOP_LEVEL_DIR_FLASH_OFFSET ((uint32_t)&(((cv_flash_top_level_dir *)0)->top_level_dir[1])) /* offset between buffers for multi-buffering (max size) */
#define CV_HDOTP_SKIP_TABLE_FLASH_OFFSET ((uint32_t)&(((cv_hdotp_skip_table_block *)0)->skipTableBlob[1])) /* offset between buffers for multi-buffering (max size) */

typedef struct td_cv_heap_buf  {
	uint32_t	heap[ALIGN_DWORD(CV_VOLATILE_OBJECT_HEAP_LENGTH)/4];
} cv_heap_buf;

typedef struct td_usbh_heap_buf  {
	uint32_t	heap[ALIGN_DWORD(USBH_HEAP_SIZE)/4];
} usbh_heap_buf;

typedef struct td_nci_init_state {
    const uint8_t * p_cur_patch_data;       /* points to offset in patch data */
    uint16_t cur_patch_offset;              /* keeps the patch offset index */
    uint16_t cur_patch_len_remaining;       /* keeps the length of remaining patch data */
    uint8_t spd_cur_patch_idx;              /* Current patch being downloaded */
    uint8_t spd_patch_count;                /* total number of patches */
    cv_bool spd_signature_sent;             /* set to TRUE after Signature is sent */
} cv_nci_init_state;

/*
 * function prototypes
 */
void
cvManager(void);

void
cvFpAsyncCapture(void);

void
cvFpEnum(void);

void
cvFpDenum(void);

cv_status
cvDecapsulateCmd(cv_encap_command *cmd, uint32_t **inputParams, uint32_t *numInputParams);

cv_status
cvEncapsulateCmd(cv_encap_command *cmd, uint32_t numOutputParams, uint32_t **outputParams, uint32_t *outputLengths,
				 uint32_t *outputEncapTypes, cv_status retVal);

cv_status
cvDecryptCmd(cv_encap_command *cmd);

cv_status
cvEncryptCmd(cv_encap_command *cmd);

uint32_t
cvGetDeltaTime(uint32_t startTime);

uint32_t
cvGetDeltaTimeSecs(uint32_t startTimeSecs);

cv_status
cvGetObj(cv_obj_properties *objProperties, cv_obj_hdr **objPtr);

cv_status
cvGetFlashObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr);

cv_status
cvUnwrapObj(cv_obj_properties *objProperties, cv_obj_storage_attributes objAttribs, cv_obj_handle *handles, uint8_t **ptrs, uint8_t **outPtrs);

cv_status
cvGetHostObj(cv_obj_handle *handles, uint8_t **ptrs, uint8_t **outPtrs);

cv_status
cvDetermineObjAvail(cv_obj_properties *objProperties);

cv_status
cvPutObj(cv_obj_properties *objProperties, cv_obj_hdr *objPtr);

cv_status
cvWrapObj(cv_obj_properties *objProperties, cv_obj_storage_attributes objAttribs, cv_obj_handle *handles,
		  uint8_t **ptrs, uint32_t *objLen);

cv_status
cvPutFlashObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr);

cv_status
cvPutSmartCardObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr);

cv_status
cvPutContactlessObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr);

cv_status
cvPutHostObj(cv_host_storage_store_object *objOut);

cv_status
cvDelObj(cv_obj_properties *objProperties, cv_bool suppressExtDeviceObjDel);

cv_status
cvEnumObj(cv_obj_properties *objProperties, cv_obj_type objType, uint32_t *bufLen, cv_obj_handle *handleBuf, cv_bool resume);

cv_status
cvDoAuth(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties,
		 uint32_t paramAuthListLength, cv_auth_list *paramAuthList,
		 cv_obj_auth_flags *authFlags, cv_input_parameters *inputParameters,
		 cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail,
		 uint32_t *statusCount, cv_status *statusList);

cv_status
cvAuthPassphrase(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties,
				 cv_bool determineAvail);

cv_status
cvAuthPassphraseHMAC(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties,
					 cv_input_parameters *inputParameters, cv_bool determineAvail);

cv_status
cvAuthSession(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_input_parameters *inputParameters,
			  cv_obj_auth_flags *authFlags);

cv_status
cvAuthFingerprint(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties,
				  cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail);

cv_status
cvAuthHOTP(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties,
		   cv_callback *callback, cv_callback_ctx context, cv_bool determineAvail);

cv_status
cvAuthTime(cv_auth_hdr *authEntryObj);

cv_status
cvAuthCount(cv_auth_hdr *authEntryObj);

cv_status
cvAuthHID(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail);

cv_status
cvAuthSmartCard(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail);

cv_status
cvAuthProprietarySC(cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail);

cv_status
cvAuthChallenge(cv_auth_hdr *authEntryParam, cv_auth_hdr *authEntryObj, cv_obj_properties *objProperties, cv_bool determineAvail);

cv_status
cvAuthSuper(cv_auth_hdr *authEntryParam);


cv_bool
cvInDALockout(void);

void
cvChkDA(cv_status status);

cv_status
cvDetermineAuthAvail(cv_auth_list *paramAuthList, uint32_t *numResults, cv_status *statusList);

cv_status
cvDecryptBlob(uint8_t *authStart, uint32_t authLen, uint8_t *cryptStart, uint32_t crtypLen,
			  uint8_t *cryptStart2, uint32_t cryptLen2,
			  uint8_t *IV, uint8_t *AESKey, uint8_t *IV2, uint8_t *AESKey2, uint8_t *HMACKey,
			  uint8_t *HMAC, uint32_t HMACLen, cv_bulk_mode mode,
			  uint8_t *otherHashData, uint32_t otherHashDataLen);

cv_status
cvEncryptBlob(uint8_t *authStart, uint32_t authLen, uint8_t *cryptStart, uint32_t crtypLen,
			  uint8_t *cryptStart2, uint32_t cryptLen2,
			  uint8_t *IV, uint8_t *AESKey, uint8_t *IV2, uint8_t *AESKey2,
			  uint8_t *HMACKey, uint8_t *HMAC, uint32_t HMACLen, cv_bulk_mode mode,
			  uint8_t *otherHashData, uint32_t otherHashDataLen);

cv_status
cvAuth(uint8_t *authStart, uint32_t authLen, uint8_t *HMACKey, uint8_t *hashOrHMAC, uint32_t hashOrHMACLen,
			  uint8_t *otherHashData, uint32_t otherHashDataLen, cv_hash_op_internal hashOp);

cv_status
cvCrypt(uint8_t *cryptStart, uint32_t cryptLen, uint8_t *AESKey, uint8_t *IV, cv_bulk_mode mode, cv_bool encrypt);

cv_status
cvRandom(uint32_t randLen, uint8_t *rand);

cv_status
cvWritePersistentData(cv_bool mirror);

cv_status
cvWriteTopLevelDir(cv_bool mirror);

cv_status
cvWriteDirectoryPage0(cv_bool mirror);

void
cvObjCacheGetLRU(uint32_t *LRUObj, uint8_t **objStoragePtr);

uint32_t
cvGetLevel1DirPageHandle(uint32_t level2DirPageHandle);

uint8_t
cvGetLevel2DirPage(uint8_t level1DirPage, uint8_t dirIndex);

uint8_t
cvGetLevel1DirPage(uint8_t level2DirPage);

void
cvLevel2DirCacheGetLRU(uint32_t *LRUL2Dir, uint8_t **level2DirPagePtr);

void
cvLevel1DirCacheGetLRU(uint32_t *LRUL1Dir, uint8_t **level1DirPagePtr);

uint32_t
cvGetLevel2FromL1DirPageHandle(uint32_t level1DirPage, uint32_t dirIndex);

cv_bool
cvIsPageFull(uint32_t handle);

void
cvSetPageFull(uint32_t handle);

void
cvSetPageAvail(uint32_t handle);

void
cvCreateL2Dir(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties,
			  cv_obj_handle *objHandle, cv_dir_page_level1 *dirL1Ptr, uint32_t l1Index,
			  uint32_t *LRU2Dir, cv_dir_page_level2 **dirL2Ptr);

void
cvCreateL1Dir(cv_obj_handle level1DirHandle, cv_obj_handle level2DirHandle,
			  uint32_t *LRUL1Dir, cv_dir_page_level1 **dirL1Ptr);

cv_status
cvGetL1L2Dir(cv_obj_properties *objProperties,cv_obj_handle level1DirHandle, cv_obj_handle level2DirHandle,
		   uint32_t *LRUL1Dir, cv_dir_page_level1 **dirL1Ptr, uint32_t *LRUL2Dir, cv_dir_page_level2 **dirL2Ptr);

cv_bool
cvPageExists(uint32_t handle);

void
cvRemoveL2Entry(cv_obj_handle objHandle, cv_dir_page_level2 *dirL2Ptr);

cv_status
cvCreateDir0Entry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties,
				  cv_obj_hdr *objPtr, uint8_t **objDirPtrs, cv_obj_handle objHandle, uint32_t *index);

cv_status
cvCreateVolatileDirEntry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties,
				  cv_obj_hdr *objPtr, cv_obj_handle objHandle);

uint16_t
cvGetAvailL2Index(cv_dir_page_level2 *l2Ptr);

void
cvComposeObj(cv_obj_properties *objProperties, cv_obj_hdr *objPtr, uint8_t *buf);

cv_bool
cvFindDirPageInL1Cache(cv_obj_handle objHandle, cv_dir_page_level1 **dirL1Ptr, uint32_t *MRUIndex);

cv_bool
cvFindDirPageInL2Cache(cv_obj_handle objHandle, cv_dir_page_level2 **dirL2Ptr, uint32_t *MRUIndex);

cv_bool
cvFindDirPageInPageMap(cv_obj_handle *level2DirHandle);

cv_status
cvUpdateVolatileDirEntry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties,
				  cv_obj_hdr *objPtr, cv_obj_handle objHandle);

cv_bool
cvFindObjInCache(cv_obj_handle objHandle, cv_obj_hdr **objPtr, uint32_t *MRUIndex);

void
cvUpdateMRU(uint8_t *MRUList, uint32_t index);

cv_bool
cvFindDir0Entry(cv_obj_handle handle, uint32_t *dir0Index);

cv_bool
cvFindVolatileDirEntry(cv_obj_handle objHandle, uint32_t *index);

cv_bool
cvFindObjEntryInL2Dir(cv_obj_handle objHandle, cv_dir_page_level2 *dirL2Ptr, uint32_t *index);

cv_bool
cvFindL2DirEntryInL1Dir(cv_obj_handle dirHandle, cv_dir_page_level1 *dirL1Ptr, uint32_t *index);

cv_bool
cvFindL1DirEntryInTopLevelDir(cv_obj_handle dirHandle, uint32_t *index);

cv_status
cvDelHostObj(cv_obj_handle handle, cv_host_storage_delete_object *objOut);

void *
cvFindObjValue(cv_obj_hdr *objPtr);

cv_status
ctkipPrfAes(uint8_t *key, uint8_t *message, uint32_t messageLen, uint32_t secretLen, uint8_t *secret);

void
cvGetVersion(cv_version *version);

void
cvGetCurCmdVersion(cv_version *version);

cv_auth_entry_PIN *
cvFindPINAuthEntry(cv_auth_list *pList, cv_obj_properties *objProperties);

uint32_t
cvComputeObjLen(cv_obj_properties *objProperties);

void
cvFindObjPtrs(cv_obj_properties *objProperties, cv_obj_hdr *objPtr);

void
cvCopySwap(uint32_t *in, uint32_t *out, uint32_t numBytes);

void
cvInplaceSwap(uint32_t *in, uint32_t numWords);

cv_status
cvWaitInsertion(cv_callback *callback, cv_callback_ctx context, cv_status prompt, uint32_t userPromptTimeout);

cv_status
cvHandlerPromptOrPin(cv_status status, cv_obj_properties *objProperties,
					 cv_callback *callback, cv_callback_ctx context);

cv_status
cvHandlerDetermineObjAvail(cv_obj_properties *objProperties, cv_callback *callback, cv_callback_ctx context);

cv_status
cvCopyObjValue(cv_obj_type objType, uint32_t lenObjValue, void *pObjValueOut, void *pObjValue, cv_bool allowPriv, cv_bool omitPriv);

cv_status
cvHandlerObjAuth(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties,
				 uint32_t authListLength, cv_auth_list *pAuthList, cv_input_parameters *inputParameters, cv_obj_hdr **objPtr,
				 cv_obj_auth_flags *authFlags, cv_bool *needObjWrite, cv_callback *callback, cv_callback_ctx context);

cv_status
cvHandlerPostObjSave(cv_obj_properties *objProperties, cv_obj_hdr *objPtr, cv_callback *callback, cv_callback_ctx context);

cv_status
cvValidateAuthLists(cv_admin_auth_permission cvAdminAuthPermission, uint32_t authListsLength, cv_auth_list *pAuthLists);

cv_status
cvHandlerDetermineAuthAvail(cv_admin_auth_permission cvAdminAuthPermission, cv_obj_properties *objProperties,
							uint32_t authListLength, cv_auth_list *pAuthList, cv_callback *callback, cv_callback_ctx context);
uint32_t
cvGetWrappedSize(uint32_t objLen);

uint32_t
cvGetNormalizedLength(uint8_t *in, uint32_t totalLength);

void
cvSetupInputParameterPtrs(cv_input_parameters *inputParameters, uint32_t count,
						  uint32_t paramLen1, void *paramPtr1, uint32_t paramLen2, void *paramPtr2,
						  uint32_t paramLen3, void *paramPtr3, uint32_t paramLen4, void *paramPtr4,
						  uint32_t paramLen5, void *paramPtr5, uint32_t paramLen6, void *paramPtr6,
						  uint32_t paramLen7, void *paramPtr7, uint32_t paramLen8, void *paramPtr8,
						  uint32_t paramLen9, void *paramPtr9, uint32_t paramLen10, void *paramPtr10);
void
cvSetupInputParameterPtrsLong(cv_input_parameters *inputParameters, uint32_t count,
						  uint32_t paramLen1, void *paramPtr1, uint32_t paramLen2, void *paramPtr2,
						  uint32_t paramLen3, void *paramPtr3, uint32_t paramLen4, void *paramPtr4,
						  uint32_t paramLen5, void *paramPtr5, uint32_t paramLen6, void *paramPtr6,
						  uint32_t paramLen7, void *paramPtr7, uint32_t paramLen8, void *paramPtr8,
						  uint32_t paramLen9, void *paramPtr9, uint32_t paramLen10, void *paramPtr10,
						  uint32_t paramLen11, void *paramPtr11, uint32_t paramLen12, void *paramPtr12,
						  uint32_t paramLen13, void *paramPtr13, uint32_t paramLen14, void *paramPtr14,
						  uint32_t paramLen15, void *paramPtr15, uint32_t paramLen16, void *paramPtr16,
						  uint32_t paramLen17, void *paramPtr17, uint32_t paramLen18, void *paramPtr18,
						  uint32_t paramLen19, void *paramPtr19, uint32_t paramLen20, void *paramPtr20);
cv_status
cvFindObjAtribFlags(cv_obj_properties *objProperties, cv_obj_hdr *objPtr, cv_attrib_flags **flags, cv_obj_storage_attributes *attributes);

void
cvUpdateObjCacheMRUFromHandle(cv_obj_handle handle);

void
cv_abort(cv_bool hostRequest);

void
cvFlushCacheEntry(cv_obj_handle objHandle);

cv_status
cvReadCommandsFromFile(void);

cv_status
cvDetectFPDevice(void);

cv_status
cvInitFPDevice(void);

cv_status
cvUSHfpCapture(cv_bool doGrabOnly, cv_bool doFingerDetect, cv_fp_control fpControl,
			   uint8_t * image, uint32_t *imageLength, uint32_t *imageWidth, uint32_t swipe,
			   cv_callback *callback, cv_callback_ctx context);

cv_status
cvUSHfpIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cvUSHfpVerify(cv_bool doFingerDetect, int32_t FARvalue,
				uint32_t featureSetLength, uint8_t *featureSet,
				cv_obj_handle templateHandle, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cvUSHfpCreateTemplate(uint32_t featureSet1Length, uint8_t *featureSet1,
					 uint32_t featureSet2Length, uint8_t *featureSet2,
					 uint32_t featureSet3Length, uint8_t *featureSet3,
					 uint32_t featureSet4Length, uint8_t *featureSet4,
					 uint32_t *templateLength, cv_obj_value_fingerprint **createdTemplate);

cv_status
cvUSHfpEnableFingerDetect(cv_bool enable);

uint8_t
fpFingerDetectCallback(uint8_t status, void *context);

uint8_t
cvFpSwFdetTimerISR(void);

cv_status
cvHostFPAsyncInterrupt(void);

void
cvFpCancel(cv_bool fromISR);

cv_status
cvBulkDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength,
			  uint8_t *hashOut, uint32_t *hashOutLen, cv_enc_op decryptionOperation, cv_bulk_params *pBulkParameters,
			  cv_obj_type objType, cv_obj_value_key *key, cv_obj_value_hash *HMACkey);

cv_status
cvAsymDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength,
			  cv_obj_type objType, cv_obj_value_key *key);

void
cvRemapHandles(uint32_t authListsLength, cv_auth_list *pAuthLists, uint32_t count, cv_obj_handle *currentHandles, cv_obj_handle *newHandles);

cv_bool
cvFindObjAuthList(cv_auth_list *pList, cv_auth_list *oList, uint32_t objAuthListsLen, cv_auth_list **oListFound);

cv_status
cvBuildEmbeddedHandlesList(cv_status status, uint8_t * maxPtr, uint32_t authListsLength, cv_auth_list *pAuthLists, uint32_t *length, cv_obj_handle *handles);

cv_status
cvAsymEncrypt(byte *pCleartext, uint32_t cleartextLength, byte *pEncData, uint32_t  *pEncLength,
			  cv_obj_type objType, cv_obj_value_key *key);

cv_status
cvAsymDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength,
			  cv_obj_type objType, cv_obj_value_key *key);

cv_status
cvBulkEncrypt(byte *pCleartext, uint32_t cleartextLength, byte *pEncData, uint32_t  *pEncLength,
			  uint8_t *hashOut, uint32_t *hashOutLen, cv_enc_op encryptionOperation, cv_bulk_params *pBulkParameters,
			  cv_obj_type objType, cv_obj_value_key *key, cv_obj_value_hash *HMACkey);

cv_status
cvBulkDecrypt(byte *pEncData, uint32_t  encLength, byte *pCleartext, uint32_t *pCleartextLength,
			  uint8_t *hashOut, uint32_t *hashOutLen, cv_enc_op decryptionOperation, cv_bulk_params *pBulkParameters,
			  cv_obj_type objType, cv_obj_value_key *key, cv_obj_value_hash *HMACkey);

cv_status
cvVerify(cv_obj_type objType, uint8_t *hash, uint32_t hashLen, cv_obj_value_key *key,
			  uint8_t *signature, uint32_t signatureLen, cv_bool *verify);

cv_status
cvValidateAttributes(uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes);

void
cvCopyAuthListsUnauthorized( cv_auth_list *toAuthLists, cv_auth_list *fromAuthLists, uint32_t authListsLength);

cv_status
cvValidateObjWithExistingAuth(uint32_t authListLength, cv_auth_list *pAuthList,
							  uint32_t authListsLengthRef, cv_auth_list *pAuthListsRef,
							  uint32_t authListsLength, cv_auth_list *pAuthLists,
							  uint32_t altFlags, cv_bool *needObjWrite);

cv_status
cvValidateSessionHandle(cv_handle cvHandle);

cv_status
cvValidateObjectValue(cv_obj_type objType, uint32_t objValueLength, void *pObjValue);

cv_status
cvDelAllHostObjs(void);

void
cvByteSwap(uint8_t *word);

void
cvUpdateObjCacheLRUFromHandle(cv_obj_handle handle);

void
cvUpdateObjCacheLRU(uint32_t index);

void
cvPrintMemSizes(void);

cv_bool
cvIsZero(uint8_t *in, uint32_t totalLength);

void
cvSetConfigDefaults(void);

void
cvCheckFlashHeapConsistency(void);

cv_bool
isRadioEnabled(uint32_t bCheckOverride);

cv_status
cvReadContactlessID(cv_post_process_piv_chuid postProcess, uint32_t PINLen, uint8_t *PIN, cv_contactless_type *type, uint32_t *len, uint8_t *id);

cv_status
cvReadAndHashCACPIVCardID(cv_cac_piv_card_type cardType, uint8_t *hash);

void
cvCloseOpenSessions(cv_handle cvHandle);

void
cvPrintOpenSessions(void);

cv_status
cvCheckAntiHammeringStatus(void);

cv_status
cvCheckAntiHammeringStatusPreCheck(void);

uint8_t
cvUpdateAntiHammeringCredits(void);

uint32_t
cvGetPrevTime(uint32_t deltaTime);

cv_status
cvWriteAntiHammeringCreditData(void);

cv_status
cv_pkcs1_v15_encrypt_encode(uint32_t mLen,  uint8_t *M, uint32_t emLen, uint8_t *em);

cv_status
cv_pkcs1_v15_decrypt_decode(uint32_t emLen,  uint8_t *em, uint32_t *mLen, uint8_t *M);

void
cv_reinit(void);

void
cv_reinit_resume(void);

cv_bool
isRadioPresent(void);

cv_bool
isSCEnabled(void);

cv_bool
isSCPresent(void);

cv_bool
isNFCEnabled(void);

cv_bool
isNFCPresent(void);

cv_bool
isFPEnabled(void);

int
is_fp_synaptics_present(void);

int
is_fp_synaptics_present_with_polling_support(void);

int
getExpectedFingerprintTemplateType(void);

int
getExpectedFingerprintFeatureSetType(void);

void
cvFlushAllCaches(void);

void
cvInvalidateHDOTPSkipTable(void);

cv_bool
cvIsValidHDOTPSkipTable(void);

cv_status
cvReadHDOTPSkipTable(cv_hdotp_skip_table *hdotpSkipTable);

cv_status
cvWriteHDOTPSkipTable(void);

void
cvSet2TandAHEnables(cv_enables_2TandAH enables);

void
cvGet2TandAHEnables(cv_enables_2TandAH *enables);

void
cvGetAHCredits(uint32_t *credits);

void
cvGetElapsedTime(uint32_t *time);

uint32_t
cvGetRFIDParamsID(void);

cv_status
cvGetRFIDParams(uint32_t *paramsLength, uint8_t *params);

void
resetHDOTPReadLimit(void);

cv_bool
updateHDOTPReadLimitCount(void);

void
cvZeroSTTable( cv_auth_list *toAuthLists, uint32_t authListsLength);

cv_bool
cvHasIndicatedAuth( cv_auth_list *toAuthLists, uint32_t authListsLength, uint32_t authFlags);

cv_status
cvDoPartialAuthListsChange(
	/* in */ uint32_t changeAuthListsLen,
	/* in */ cv_auth_list *changeAuthLists,    /* note: this points past the initial empty auth list with CV_CHANGE_AUTH_PARTIAL set in it */
	/* in */ uint32_t currentAuthListsLen,
	/* in */ cv_auth_list *currentAuthLists,
	/* out */ uint32_t *newAuthListsLen,
	/* out */ cv_auth_list *newAuthLists);

void
cvCopyAuthList(cv_auth_list *toAuthList, cv_auth_list *fromAuthList, uint32_t *len);

cv_status
cvAESCMAC(uint8_t *data, uint32_t dataLen, uint8_t *key, uint32_t keySize, uint8_t *digest);

cv_status
cvResponseSuper ( uint8_t response[AES_128_BLOCK_LEN]);	/* response */ 

cv_status
cvDPFROnChipFeatureExtraction(uint32_t imageLength, cv_fp_fe_control cvFpFeControl, 
							  uint32_t *featureSetLength, uint8_t *featureSet);

cv_status
cvSynapticsOnChipFeatureExtraction(uint32_t *featureSetLength, uint8_t *featureSet);

cv_status
cvDPFROnChipCreateTemplate(uint32_t featureSet1Length, uint8_t *featureSet1,
					 uint32_t featureSet2Length, uint8_t *featureSet2,
					 uint32_t featureSet3Length, uint8_t *featureSet3,
					 uint32_t featureSet4Length, uint8_t *featureSet4,
					 uint32_t *templateLength, cv_obj_value_fingerprint **createdTemplate);

cv_status
cvSynapticsOnChipCreateTemplate(uint8_t* featureSet, uint32_t featureSetSize, uint8_t **pTemplate, uint32_t *templateSize, uint32_t *enrollComplete);

cv_status
cvFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl,  cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,				  
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cvSynapticsFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl,  cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,				  
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cvDPFROnChipIdentify(cv_handle cvHandle, int32_t FARvalue, cv_fp_control fpControl,  cv_bool isIdentifyPurpose,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,				  
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool fingerDetected, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

PHX_STATUS
cvPkcs1V15Encode(crypto_lib_handle *pHandle,
				uint32_t mLen,  uint8_t *M, uint32_t emLen, uint8_t *em);

cv_status
cvUshFpEnableFingerDetect(cv_bool enable);

void
cvFpFEandInt(void);

cv_status
cvHostFeatureExtraction(uint32_t imageLength, uint32_t *featureSetLength, uint8_t *featureSet);

cv_status
cvUSHFpEnableFingerDetect(cv_bool enable);

cv_bool
isFPImageBufAvail(void);

void
DumpBuffer(uint32_t len, void *buf);

void
cv_reenum(void);

uint8_t
cvClCardDetectTimerISR(void);

void
cvCLCardDetectPresence(void);

uint8_t
cvNFCTimerISR(void);

void
cvNFCTimerCommand(void);

cv_status
cvHostCLAsyncInterrupt(cv_bool arrival);

void
cvStartTimerForWBFObjectProtection(void);

#ifdef TYPE4_WORK
cv_status cv_nfc_send_data_host_driver (uint8_t *pBuffer, uint32_t len);
#endif

#endif				       /* end _CVINTERNAL_H_ */


