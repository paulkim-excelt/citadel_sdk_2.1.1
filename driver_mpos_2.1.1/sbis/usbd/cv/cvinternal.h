/*
 * $Copyright Broadcom Corporation$
 *
 */
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

#include <platform.h>

/*
 * Enumerated values and types
 */
#define MAX_INPUT_PARAMS 	20 /* TBD */
#define MAX_OUTPUT_PARAMS 	20 /* TBD */
#define MAX_OUTPUT_PARAM_SIZE 	4096 /* TBD */
#define ALIGN_DWORD(x) 			((x + 0x3) & 0xfffffffc)
#define CALLBACK_STATUS_TIMEOUT			5000 /* milliseconds */
#define HOST_STORAGE_REQUEST_TIMEOUT 	500 /* milliseconds */
#define HOST_CONTROL_REQUEST_TIMEOUT 	500 /* milliseconds */
#define FP_IMAGE_TRANSFER_TIMEOUT 		500 /* milliseconds */
#define USER_PROMPT_TIMEOUT 			30000 /* milliseconds */
#define USER_PROMPT_CALLBACK_INTERVAL 	1000 /* milliseconds */
#define SMART_CARD_INSERTION_DELAY 		5000 /* milliseconds */
#define MAX_CV_OBJ_CACHE_NUM_OBJS			4	/* must divide obj cache into 4-byte aligned areas */
#define MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing other directories */
#define MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing objects */
#define INVALID_CV_CACHE_ENTRY				0xff /* invalid value to use for initializing cache entries */
#define MAX_CV_ENTRIES_PER_DIR_PAGE			16	/* number of directory pages per higher level page */
#define MAX_DIR_LEVELS						3   /* note: this reflects implementation and can't be changed w/o changing code */
#define MAX_CV_NUM_VOLATILE_OBJS			40	/* must divide obj cache into 4-byte aligned areas */
#define MAX_DIR_PAGE_0_ENTRIES				200	/* maximum number of directory page 0 entries */
#define HANDLE_DIR_SHIFT 					24
#define GET_DIR_PAGE(x) 					(x>>HANDLE_DIR_SHIFT)
#define GET_HANDLE_FROM_PAGE(x) 			(x<<HANDLE_DIR_SHIFT)
#define HANDLE_VALUE_MASK 					0x00ffffff
#define GET_L1_HANDLE_FROM_OBJ_HANDLE(x) 	((x>>4) & ~HANDLE_VALUE_MASK)
#define GET_L2_HANDLE_FROM_OBJ_HANDLE(x) 	((x) & ~HANDLE_VALUE_MASK)
#define DIRECTORY_PAGES_MASK 				0xffff
#define MAX_DIR_PAGES 						256
#define MIN_L2_DIR_PAGE 					16
#define MAX_L2_DIR_PAGE 					254
#define AES_128_MASK 						(AES_128_BLOCK_LEN - 1)
#define AES_256_MASK 						(AES_256_BLOCK_LEN - 1)
#define GET_AES_128_PAD_LEN(x) 				((~x + 1) & AES_128_MASK)
#define GET_AES_192_PAD_LEN(x) 				((x % AES_192_BLOCK_LEN) ? (AES_192_BLOCK_LEN - (x % AES_192_BLOCK_LEN)) : 0)
#define GET_AES_256_PAD_LEN(x) 				((~x + 1) & AES_256_MASK)
#define MAX_CV_AUTH_PROMPTS 				5 /* TBD */
#define MAX_FP_FEATURE_SET_SIZE 			600
#define MAX_FP_TEMPLATES 					50
#define DEFAULT_CV_DA_AUTH_FAILURE_THRESHOLD 6
#define DEFAULT_CV_DA_INITIAL_LOCKOUT_PERIOD 5000 /* in milliseconds */
#define RSA_PUBKEY_CONSTANT 				65537
#define KDC_PUBKEY_LEN 						2048
#define PRIME_FIELD							0
#define BINARY_FIELD 						1
#define PRIV_KEY_INVALID 					0
#define PRIV_KEY_VALID 						1
#define CV_OBJ_CACHE_SIZE					9216
#define CV_LEVEL1_DIR_PAGE_CACHE_SIZE 		(sizeof(cv_dir_page_level1)*MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES)
#define CV_LEVEL2_DIR_PAGE_CACHE_SIZE 		(sizeof(cv_dir_page_level2)*MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES)
#define OPEN_MODE_BLOCK_PTRS 				(0x00000000)
#define SECURE_MODE_BLOCK_PTRS 				(0x00000000)
#define SYS_AND_TPM_IO_OPEN_WINDOW	0
#define SMART_CARD_IO_OPEN_WINDOW	1
#define RF_IO_OPEN_WINDOW			2
#define CV_IO_OPEN_WINDOW			3
#define CV_OBJ_CACHE_ENTRY_SIZE		CV_OBJ_CACHE_SIZE/MAX_CV_OBJ_CACHE_NUM_OBJS
#define CV_VOLATILE_OBJECT_HEAP_LENGTH 3*1024
/* below is the definition of the memory block that is shared between CV IO, FP match and FP capture */
/*-------------------------------   <---   CV_SECURE_IO_COMMAND_BUF
|  CV I/O command buf (4K)    |
|							  |     <---   CV_SECURE_IO_FP_ASYNC_BUF
|  FP I/O command buf (4K)    |
|-----------------------------|     <---   FP_CAPTURE_LARGE_HEAP_PTR & FP_MATCH_HEAP_PTR
|              |              |
| FP image buf |  FP match    |
|    (90K)     |   heap (40K) |
|              |              |
| FP capture   |              |
|   heap (50K) |              |
|              |              |
|              |              |
|              |              |
-------------------------------*/


#define CV_IO_COMMAND_BUF_SIZE		(4*1024 + 100)	/* 4K Buffer + space for header */
#define FP_MATCH_HEAP_SIZE			(40*1024)  		/* TBD - this is estimate only */
#define FP_CAPTURE_LARGE_HEAP_SIZE	(140*1024)
#define FP_CAPTURE_SMALL_HEAP_SIZE	(3*1024)
#define USBH_HEAP_SIZE				(7*1024)
#define FP_MATCH_HEAP_PTR			FP_CAPTURE_LARGE_HEAP_PTR
#define FP_IMAGE_MAX_SIZE			90*1024  		/* 90K bytes */
#define FP_DEFAULT_USER_TIMEOUT		15000 			/* 15 seconds in milliseconds */
#define FP_MIN_USER_TIMEOUT			1000 			/* 1 seconds in milliseconds */
#define FP_MAX_USER_TIMEOUT			30000 			/* 30 seconds in milliseconds */
#define FP_DEFAULT_CALLBACK_PERIOD	1000 			/* 1 second in milliseconds */
#define FP_MIN_CALLBACK_PERIOD		500 			/* .5 second in milliseconds */
#define FP_MAX_CALLBACK_PERIOD		10000 			/* 10 second in milliseconds */
#define FP_DEFAULT_SW_FDET_POLLING_INTERVAL 1000 	/* 1 second in milliseconds */
#define FP_MIN_SW_FDET_POLLING_INTERVAL		250 	/* .25 second in milliseconds */
#define FP_MAX_SW_FDET_POLLING_INTERVAL		5000 	/* 5 seconds in milliseconds */
#define FP_DEFAULT_PERSISTENCE		1000 			/* 1 second in milliseconds */
#define FP_MIN_PERSISTENCE			1 				/* 0 second in milliseconds */
#define FP_MAX_PERSISTENCE			10000 			/* 10 seconds in milliseconds */
#define FP_DEFAULT_FAR				0x20C49B 		/* 0.001 as recommended by DP */
#define FP_MIN_FAR					0 				/* 0.0 */
#define FP_MAX_FAR					0x7FFFFFFF 		/* 1.0 */
#define USB_VID_OFFSET				8 				/* vendor ID offset in USB device descriptor */
#define USB_PID_OFFSET				10 				/* product ID offset in USB device descriptor */
#define CV_MAX_USER_ID_LEN			20 				/* TBD - maximum user ID length */
#define CV_MAX_USER_KEY_LEN			20 				/* TBD - maximum user key length */
#define CV_MAX_EXPORT_BLOB_EMBEDDED_HANDLES 100
#define CV_MAX_PROPRIETARY_SC_KEYLEN 16
#define CV_MALLOC_HEAP_OVERHEAD 36
#define CV_MALLOC_ALLOC_OVERHEAD 8
#define CV_FLASH_ALLOC_HEAP_OVERHEAD 0
#define CV_FLASH_ALLOC_OVERHEAD 0
#define CV_MAX_DP_HINT 2048

typedef unsigned int cv_status_internal;
#define CV_FP_NO_MATCH					0x80000001
#define CV_FP_MATCH						0x80000002
#define CV_FP_IMAGE_MATCH_AND_UPDATED	0x80000003
#define CV_OBJECT_UPDATE_REQUIRED		0x80000004

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
	CV_STORAGE_TYPE_HARD_DRIVE			= 0x00000000,
	CV_STORAGE_TYPE_FLASH				= 0x00000001,
	CV_STORAGE_TYPE_SMART_CARD			= 0x00000002,
	CV_STORAGE_TYPE_CONTACTLESS			= 0x00000003,
	CV_STORAGE_TYPE_FLASH_NON_OBJ		= 0x00000004,
	CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ	= 0x00000005,
	CV_STORAGE_TYPE_FLASH_PERSIST_DATA	= 0x00000006,
	CV_STORAGE_TYPE_HARD_DRIVE_UCK		= 0x00000007,
	CV_STORAGE_TYPE_VOLATILE			= 0x00000008,
};

#define	CV_DIR_0_HANDLE				0x00000000
#define	CV_TOP_LEVEL_DIR_HANDLE		0x00ffffff
#define	CV_UCK_OBJ_HANDLE			0x01000000
#define	CV_PERSISTENT_DATA_HANDLE	0x00000001
#define	CV_MIRRORED_DIR_0			0xff000000
#define	CV_MIRRORED_TOP_LEVEL_DIR 	0xffffffff

#define CV_MIRRORED_OBJ_MASK		0xff000000
#define CV_OBJ_HANDLE_PAGE_MASK		0xff000000

typedef unsigned short cv_command_id;
enum cv_command_id_e {
		CV_ABORT,							// 0x0000
		CV_CALLBACK_STATUS,					// 0x0001
		CV_CMD_OPEN,						// 0x0002
		CV_CMD_OPEN_REMOTE,					// 0x0003
		CV_CMD_CLOSE,						// 0x0004
		CV_CMD_GET_SESSION_COUNT,			// 0x0005
		CV_CMD_INIT,						// 0x0006
		CV_CMD_GET_STATUS,					// 0x0007
		CV_CMD_SET_CONFIG,					// 0x0008
		CV_CMD_CREATE_OBJECT,				// 0x0009
		CV_CMD_DELETE_OBJECT,				// 0x000A
		CV_CMD_SET_OBJECT,					// 0x000B
		CV_CMD_GET_OBJECT,					// 0x000C
		CV_CMD_ENUMERATE_OBJECTS,			// 0x000D
		CV_CMD_DUPLICATE_OBJECT,			// 0x000E
		CV_CMD_IMPORT_OBJECT,				// 0x000F
		CV_CMD_EXPORT_OBJECT,				// 0x0010
		CV_CMD_CHANGE_AUTH_OBJECT,			// 0x0011
		CV_CMD_AUTHORIZE_SESSION_OBJECT,		// 0x0012
		CV_CMD_DEAUTHORIZE_SESSION_OBJECT,		// 0x0013
		CV_CMD_GET_RANDOM,					// 0x0014
		CV_CMD_ENCRYPT,						// 0x0015
		CV_CMD_DECRYPT,						// 0x0016
		CV_CMD_HMAC,						// 0x0017
		CV_CMD_HASH,						// 0x0018
		CV_CMD_HASH_OBJECT,					// 0x0019
		CV_CMD_GENKEY,						// 0x001A
		CV_CMD_SIGN,						// 0x001B
		CV_CMD_VERIFY,						// 0x001C
		CV_CMD_OTP_GET_VALUE,				// 0x001D
		CV_CMD_OTP_GET_MAC,					// 0x001E
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
        CV_CMD_GET_PRB = 0x007B					// 0x
};

typedef unsigned int cv_interrupt_type;
enum cv_interrupt_type_e {
		CV_COMMAND_PROCESSING_INTERRUPT,
		CV_HOST_STORAGE_INTERRUPT,
		CV_HOST_CONTROL_INTERRUPT,
		CV_FINGERPRINT_EVENT_INTERRUPT
};

typedef unsigned int cv_internal_state;
#define CV_PENDING_WRITE_TO_SC			0x00000001 	/* an object is waiting to be written to the smart card,
							but  cant be completed yet due to smart card not
							available or PIN needed */
#define CV_PENDING_WRITE_TO_CONTACTLESS	0x00000002 	/* an object is waiting to be written to the contactless
							smart card, but  cant be completed yet due to smart
							card not available or PIN needed */
#define CV_IN_DA_LOCKOUT				0x00000004 /* dictionary attack mitigation has initiated a lockout */
#define CV_WAITING_FOR_PIN_ENTRY		0x00000008 	/* authorization testing has determined that a PIN is
							needed and control has returned to calling app to allow
							command to be resubmitted with PIN */
#define CV_IN_ANTIHAMMERING_LOCKOUT		0x00000010  /* actions resulting in 2T bit incrementing (typically 
							flash writes) have exceeded allowed rate and will be locked out for a period of time */

typedef unsigned int cv_device_state;
#define CV_SMART_CARD_PRESENT			0x00000001 	/* indicates smart card present in reader */
#define CV_CONTACTLESS_SC_PRESENT		0x00000002 	/* indicates a contactless smart card is present */
#define CV_USE_REMOTE_USH_DEVICES		0x00000004 	/* indicates laptop USH in docking station and must */
												/* use RF, SC and FP devices on attached keyboard USH */
#define CV_FP_CAPTURED_FEATURE_SET_VALID	0x00000008 	/* previously captured FP feature set is valid */
#define CV_FP_CAPTURE_CONTEXT_OPEN		0x00000010	/* FP capture context is open and allows FP capture operation */
#define CV_HID_CONTEXT_OPEN				0x00000020	/* HID (RF) context is open and allows HID operation */
#define CV_FP_FINGER_DETECT_ENABLED 	0x00000040  /* indicates finger detection is enabled */
#define CV_2T_ACTIVE					0x00000080  /* indicates 2T monotonic counter is active */

typedef unsigned int cv_phx2_device_status;
enum cv_phx2_device_status_e {
	CV_PHX2_OPERATIONAL = 0x00000001,			/* PHX2 ok, no failures detected */
	CV_PHX2_MONOTONIC_CTR_FAIL = 0x00000002,	/* monotonic counter failure */
	CV_PHX2_FLASH_INIT_FAIL = 0x00000004,		/* flash init failure */
	CV_PHX2_FP_DEVICE_FAIL = 0x00000008		/* FP device failure */
};

typedef unsigned int cv_persistent_flags;
#define CV_UCK_EXISTS				0x00000001	/* unique CV key exists */
#define CV_GCK_EXISTS				0x00000002	/* group corporate key exists */
#define CV_HAS_CV_ADMIN				0x00000004	/* authorization data associated with CV administrator exists */
#define CV_HAS_SECURE_SESS_RSA_KEY	0x00000008	/* RSA public key used for secure session setup has been generated */
#define CV_HAS_SECURE_SESS_ECC_KEY	0x00000010	/* ECC public key used for secure session setup has been generated */
#define CV_RTC_TIME_CONFIGURED		0x00000020	/* RTC has been configured to maintain time of day */
#define CV_USE_SUITE_B_ALGS			0x00000040	/* crypto operations must use Suite B algorithms */
#define CV_USE_KDIX_DSA				0x00000080	/* use provisioned DSA key for secure session signing instead of Kdi in NVM */
#define CV_USE_KDIX_ECC				0x00000100	/* use provisioned ECC key for secure session signing instead of Kdi in NVM */
#define CV_HAS_BIOS_SHARED_SECRET 	0x00001000 	/* Shared secret with system BIOS has been configured */
#define CV_HAS_SAPP_SHARED_SECRET 	0x00002000 	/* Shared secret with secure application has been configured */
#define CV_HAS_FAR_VALUE			0x00004000	/* FAR value has been configured */
#define CV_HAS_SUPER_PARAMS			0x00008000	/* SUPER params have been configured */
#define CV_ANTI_REPLAY_OVERRIDE		0x00010000	/* an anti-replay override has been issued */

typedef unsigned int cv_session_flags;
/* bits map to cv_options, where applicable */
#define CV_HAS_PRIVACY_KEY	0x40000000 	/* session has specified privacy key to wrap objects*/
#define CV_REMOTE_SESSION	0x80000000	/* remote session (applies to CV Host mode API only) */

typedef unsigned char cv_mscapi_blob_type;
enum cv_mscapi_blob_type_e {
	CV_MSCAPI_SIMPLE_BLOB	= 0x1,
	CV_MSCAPI_PUBKEY_BLOB	= 0x6,
	CV_MSCAPI_PRIVKEY_BLOB	= 0x7
};

typedef unsigned int cv_mscapi_alg_id;
enum cv_mscapi_alg_id_e {
	CV_MSCAPI_CALG_3DES		= 0x00006603,
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
	CV_REQUIRE_ADMIN_AUTH
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

#define CV_INIT_TOP_LEVEL_DIR	0x00000001
#define CV_INIT_PERSISTENT_DATA 0x00000002
#define CV_INIT_DIR_0			0x00000004

/* default ECC domain parameters P-256 */

#define ECC_DEFAULT_DOMAIN_PARAMS_PRIME \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_A \
	0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_B \
	0x27, 0xD2, 0x60, 0x4B, 0x3B, 0xCE, 0x3C, 0x3E, 0xCC, 0x53, 0xB0, 0xF6, 0x65, 0x1D, 0x06, 0xB0,  \
	0x76, 0x98, 0x86, 0xBC, 0xB3, 0xEB, 0xBD, 0x55, 0xAA, 0x3A, 0x93, 0xE7, 0x5A, 0xC6, 0x35, 0xD8

#define ECC_DEFAULT_DOMAIN_PARAMS_G \
	0xD8, 0x98, 0xC2, 0x96, 0xF4, 0xA1, 0x39, 0x45, 0x2D, 0xEB, 0x33, 0xA0, 0x77, 0x03, 0x7D, 0x81,  \
	0x63, 0xA4, 0x40, 0xF2, 0xF8, 0xBC, 0xE6, 0xE5, 0xE1, 0x2C, 0x42, 0x47, 0x6B, 0x17, 0xD1, 0xF2,  \
	0x37, 0xBF, 0x51, 0xF5, 0xCB, 0xB6, 0x40, 0x68, 0x6B, 0x31, 0x5E, 0xCE, 0x2B, 0xCE, 0x33, 0x57,  \
	0x7C, 0x0F, 0x9E, 0x16, 0x8E, 0xE7, 0xEB, 0x4A, 0xFE, 0x1A, 0x7F, 0x9B, 0x4F, 0xE3, 0x42, 0xE2

#define ECC_DEFAULT_DOMAIN_PARAMS_n \
	0xFC, 0x63, 0x25, 0x51, 0xF3, 0xB9, 0xCA, 0xC2, 0xA7, 0x17, 0x9E, 0x84, 0xBC, 0xE6, 0xFA, 0xAD,  \
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF

#define ECC_DEFAULT_DOMAIN_PARAMS_h 0x01

/* default DSA domain parameters */
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

typedef unsigned int cv_device_enables;
#define CV_RF_ENABLED 0x1
#define CV_SC_ENABLED 0x2
#define CV_FP_ENABLED 0x4

/*
 * structures
 */
typedef PACKED struct td_cv_obj_attribs_hdr {
	uint16_t	attribsLen;	/* length of object attributes in bytes */
	cv_attrib_hdr attrib;	/* header for first attribute in list */
} cv_obj_attribs_hdr;

typedef PACKED struct td_cv_obj_auth_lists_hdr {
	uint16_t	authListsLen;	/* length of object auth lists in bytes */
	cv_auth_list_hdr authList;	/* header for first auth list in list */
} cv_obj_auth_lists_hdr;

typedef PACKED union td_cv_hmac_key
{
	uint8_t SHA1[SHA1_LEN];		/* HMAC-SHA1 key */
	uint8_t SHA256[SHA256_LEN];	/* HMAC-SHA256 key */
} cv_hmac_key;
typedef cv_hmac_key cv_hmac_val;

typedef PACKED struct td_cv_rsa2048_key {
	uint8_t m[RSA2048_KEY_LEN];		/* public modulus */
	uint32_t e;						/* public exponent */
	uint8_t p[RSA2048_KEY_LEN/2];	/* private prime */
	uint8_t q[RSA2048_KEY_LEN/2];	/* private prime */
	uint8_t pinv[RSA2048_KEY_LEN/2];/* CRT param */
	uint8_t dp[RSA2048_KEY_LEN/2];	/* CRT param */
	uint8_t dq[RSA2048_KEY_LEN/2];	/* CRT param */
} cv_rsa2048_key;

typedef PACKED struct td_cv_dsa1024_key {
	uint8_t p[DSA1024_KEY_LEN];		/* public p */
	uint8_t q[DSA1024_PRIV_LEN];	/* public q */
	uint8_t g[DSA1024_KEY_LEN];		/* public g */
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
	uint8_t y[DSA1024_KEY_LEN];		/* public key */
} cv_dsa1024_key;

typedef PACKED struct td_cv_dsa1024_key_no_dp {
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
	uint8_t y[DSA1024_KEY_LEN];		/* public key */
} cv_dsa1024_key_no_dp;

typedef PACKED struct td_cv_dsa1024_key_priv_only {
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
} cv_dsa1024_key_priv_only;

typedef PACKED struct td_cv_ecc256 {
	uint8_t prime[ECC256_KEY_LEN];		/* prime */
	uint8_t a[ECC256_KEY_LEN];			/* curve parameter */
	uint8_t b[ECC256_KEY_LEN];			/* curve parameter */
	uint8_t G[ECC256_POINT];			/* base point */
	uint8_t n[ECC256_KEY_LEN];			/* order of base point */
	uint8_t h[ECC256_KEY_LEN];			/* cofactor */
	uint8_t x[ECC256_KEY_LEN];			/* private key */
	uint8_t Q[ECC256_POINT];			/* public key */
} cv_ecc256;

typedef PACKED struct td_cv_ecc256_no_dp {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
	uint8_t Q[ECC256_POINT];		/* public key */
} cv_ecc256_no_dp;

typedef PACKED struct td_cv_ecc256_priv_only {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
} cv_ecc256_priv_only;

typedef PACKED struct td_cv_encap_flags {
	unsigned short	secureSession : 1 ;			/* secure session */
	unsigned short	completionCallback : 1 ;	/* contains completion callback */
	unsigned short	host64Bit : 1 ;				/* 64 bit host */
	cv_secure_session_protocol	secureSessionProtocol : 3 ;	/* secure session protocol*/
	unsigned short	USBTransport : 1 ;			/* 1 for USB; 0 for LPC */
	cv_return_type	returnType : 2 ;			/* return type */
	unsigned short	spare : 7;					/* spare */
} cv_encap_flags;

typedef PACKED struct td_cv_encap_command {
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
} cv_encap_command;

typedef PACKED struct td_cv_secure_session_get_pubkey_out {
	uint8_t		nonce[CV_NONCE_LEN];	/* anti-replay nonce */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_out;

typedef PACKED struct td_cv_secure_session_get_pubkey_in {
	uint8_t		pubkey[RSA2048_KEY_LEN];/* RSA 2048-bit public key for encrypting host nonce */
	uint8_t		sig[DSA_SIG_LEN];		/* DSA signature of pubkey using Kdi or Kdix (if any) */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_in;

typedef PACKED struct td_cv_secure_session_get_pubkey_in_suite_b {
	uint8_t		pubkey[ECC256_POINT];	/* ECC 256 public key for encrypting host nonce */
	uint8_t		sig[ECC256_SIG_LEN];	/* ECC signature of pubkey using provisioned device ECC signing key */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_in_suite_b;

typedef PACKED struct td_cv_secure_session_host_nonce {
	uint8_t	encNonce[RSA2048_KEY_LEN];	/* host nonce encrypted with CV RSA public key */
} cv_secure_session_host_nonce;

typedef PACKED struct td_cv_secure_session_host_nonce_suite_b {
	uint8_t	nonce[CV_NONCE_LEN];	/* host nonce (unencrypted) */
	uint8_t pubkey[ECC256_POINT];	/* host ECC256 public key */
} cv_secure_session_host_nonce_suite_b;

typedef PACKED struct td_cv_transport_header {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* command ID */
} cv_transport_header;

typedef PACKED struct td_cv_host_storage_get_request {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_GET_REQUEST */
} cv_host_storage_get_request;

typedef PACKED struct td_cv_host_storage_object_entry {
	cv_obj_handle	handle;	/* object/dir ID */
	uint32_t		length;	/* object/dir blob length */
} cv_host_storage_object_entry;

typedef PACKED struct td_cv_host_storage_store_object {
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
} cv_host_storage_store_object;


typedef PACKED struct td_cv_host_storage_delete_object {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_DELETE_OBJECT */
	cv_obj_handle		objectID;		/* ID of object to be deleted */
} cv_host_storage_delete_object;

typedef PACKED struct td_cv_host_storage_delete_all_files {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_DELETE_ALL_FILES */
} cv_host_storage_delete_all_files;

typedef PACKED struct td_cv_host_storage_retrieve_object_in {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* CV_HOST_STORAGE_RETRIEVE_OBJECT */
	/* remainder is variable length of the form:
		object ID 1
		...
		object ID n				*/
} cv_host_storage_retrieve_object_in;

typedef PACKED struct td_cv_host_storage_retrieve_object_out {
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
} cv_host_storage_retrieve_object_out;

typedef PACKED struct td_cv_host_storage_operation_status {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_OPERATION_STATUS */
	cv_status			status;			/* return status of retrieve object request */
} cv_host_storage_operation_status;

typedef PACKED struct td_cv_host_control_get_request {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_CONTROL */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_CONTROL_GET_REQUEST */
} cv_host_control_get_request;

typedef PACKED struct td_cv_host_control_header_in {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* command ID */
	uint32_t			HMACLen;		/* length of HMAC */
} cv_host_control_header_in;

typedef PACKED struct td_cv_host_control_feature_extraction_in {
	cv_host_control_header_in hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		image length (32 bits)
		image (length given in image length)*/
} cv_host_control_feature_extraction_in;

typedef PACKED struct td_cv_host_control_create_template_in {
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
} cv_host_control_create_template_in;

typedef PACKED struct td_cv_host_control_identify_and_hint_creation_in {
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
} cv_host_control_identify_and_hint_creation_in;

typedef PACKED struct td_cv_host_control_header_out {
	cv_transport_type	transportType;	/* transport type */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	uint32_t			commandID;		/* command ID */
	cv_status			status;			/* CV_SUCCESS or error code */
	uint32_t			HMACLen;		/* length of HMAC */
} cv_host_control_header_out;

typedef PACKED struct td_cv_host_control_feature_extraction_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		feature set length (32 bits)
		feature set (length given in feature set length)*/
} cv_host_control_feature_extraction_out;

typedef PACKED struct td_cv_host_control_create_template_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		template length (32 bits)
		template (length given in template length)*/
} cv_host_control_create_template_out;

typedef PACKED struct td_cv_host_control_identify_and_hint_creation_out {
	cv_host_control_header_out hostControlHdr;
	/* remainder is variable length of the form:
		HMAC (length given in HMACLen)
		nonce (same length as HMAC)
		hint length (32 bits)
		hint (length given in hint length)
		template index (32 bits) */
} cv_host_control_identify_and_hint_creation_out;


typedef PACKED struct td_cv_usb_interrupt {
	cv_interrupt_type	interruptType;	/* CV_COMMAND_PROCESSING_INTERRUPT or CV_HOST_STORAGE_INTERRUPT */
	uint32_t			resultLen;		/* length of result to read */
} cv_usb_interrupt;

typedef PACKED struct td_cv_dir_page_entry_dir {
	cv_obj_handle	hDirPage;	/* handle of directory page */
	uint32_t		counter;	/* anti-replay counter value */
} cv_dir_page_entry_dir;

typedef PACKED struct td_cv_dir_page_entry_persist {
	cv_dir_page_entry_dir	entry;				/* directory entry */
	sint32_t					multiBufferIndex;	/* multi-buffering index */
	uint32_t				len;				/* length of persistent data */
} cv_dir_page_entry_persist;

typedef PACKED struct td_cv_dir_page_entry_tpm {
	uint32_t		counter;			/* anti-replay counter value */
	uint32_t		multiBufferIndex;	/* multi-buffering index */
} cv_dir_page_entry_tpm;

typedef PACKED struct td_cv_dir_page_hdr {
	uint32_t				numEntries;		/* number of entries in this directory page */
	cv_dir_page_entry_dir	thisDirPage;	/* handle and counter for this dir page */
	/* remainder is variable length of the form:
			directory page entry 1
			...
			directory page entry n		*/
} cv_dir_page_hdr;

typedef PACKED struct td_cv_obj_storage_attributes {
	unsigned short	hasPrivacyKey : 1 ;		/* object has privacy key encryption */
	cv_storage_type	storageType : 4 ;		/* type of device for object storage */
	unsigned short	PINRequired : 1 ;		/* storage device requires PIN */
	unsigned short	reqHandle : 1 ;			/* flag indicating specific handle requested */
	unsigned short	replayOverride : 1 ;	/* flag indicating flag indicating anti-replay check should be overriden */
	unsigned short	spare : 8 ;			/* spare */
	cv_obj_type		objectType: 16;			/* object type */
} cv_obj_storage_attributes;

typedef PACKED struct td_cv_dir_page_entry_obj {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
} cv_dir_page_entry_obj;

typedef PACKED struct td_cv_dir_page_entry_obj_flash {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
	uint32_t		objLen;					/* length of object */
	uint8_t			*pObj;					/* pointer to object in flash */
} cv_dir_page_entry_obj_flash;

// not necessary?
//typedef PACKED struct td_cv_dir_page_entry_dir0 {
//	cv_obj_handle	hObj;					/* handle of object */
//	uint32_t		counter;				/* anti-replay counter value */
//	uint32_t		objLen;					/* length of object */
//	byte			*pObj;					/* pointer to object in flash */
//} cv_dir_page_entry_dir0;

typedef PACKED struct td_cv_dir_page_0 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj_flash dir0Objs[MAX_DIR_PAGE_0_ENTRIES];	/* object entries for directory page 0 */
} cv_dir_page_0;

typedef PACKED struct td_cv_dir_page_level1 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_dir dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 1 directory page */
} cv_dir_page_level1;

typedef PACKED struct td_cv_dir_page_level2 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 2 directory page */
} cv_dir_page_level2;

typedef PACKED struct td_cv_top_level_dir {
	cv_dir_page_hdr			header;										/* handle and count associated with top level directory */
	uint8_t					pageMapFull[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page full (1) or not (0) */
	uint8_t					pageMapExists[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page exists (1) or not (0) */
	cv_dir_page_entry_dir	dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* lower level directories contained in top level */
	cv_dir_page_entry_persist	persistentData;							/* entry associated with persistent data */
	sint32_t				dir0multiBufferIndex;							/* multi buffer index for directory 0 */
	cv_dir_page_entry_tpm	dirEntryTPM;								/* anti-replay count and multi-buffering index for TPM */
} cv_top_level_dir;

typedef PACKED struct td_cv_obj_dir_page_blob_enc {
	cv_obj_handle	hObj;						/* handle for object or directory page */
	uint32_t		counter;					/* anti-replay counter value for this blob */
} cv_obj_dir_page_blob_enc;


typedef PACKED struct td_cv_obj_dir_page_blob {
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
} cv_obj_dir_page_blob;

typedef PACKED struct td_cv_session {
	uint32_t			magicNumber;							/* s/b 'session ' to identify this as session */
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
	cv_obj_handle		lastEnumeratedHandle;					/* for multi-stage enumeration, this is handle from last enumeration */
} cv_session;

typedef PACKED struct td_cv_volatile_obj_dir_page_entry {
	cv_obj_handle	hObj;		/* handle of volatile object */
	uint8_t			*pObj;		/* pointer to volatile object in scratch RAM */
	cv_session		*hSession;	/* session ID which created this object */
	uint8_t			appUserID[20];	/* hash of app ID and user ID fields for session */
} cv_volatile_obj_dir_page_entry;

typedef PACKED struct td_cv_volatile_obj_dir {
	cv_volatile_obj_dir_page_entry	volObjs[MAX_CV_NUM_VOLATILE_OBJS];	/* list of page entries */
} cv_volatile_obj_dir;

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
#ifndef CV_BOOTSTRAP
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE];
#else
	uint8_t buf[100];
#endif
} cv_cmdbuf;

typedef struct td_cv_fp_small_heap_buf {
	uint32_t buf[ALIGN_DWORD(FP_CAPTURE_SMALL_HEAP_SIZE)/4];
} cv_fp_small_heap_buf;
#if 0
typedef struct td_cv_hid_small_heap_buf {
	uint32_t buf[ALIGN_DWORD(HID_SMALL_HEAP_SIZE)/4];
} cv_hid_small_heap_buf;

typedef struct td_cv_hid_large_heap_buf {
	uint32_t buf[ALIGN_DWORD(HID_LARGE_HEAP_SIZE)/4];
} cv_hid_large_heap_buf;
#endif
typedef struct td_cv_persisted_fp {
	uint8_t buf[MAX_FP_FEATURE_SET_SIZE];
} cv_persisted_fp;

typedef struct td_cv_secure_iobuf {
	/* this buffer has the CV IO buffer followed by the FP capture/FP match heap */
#ifndef CV_BOOTSTRAP
	uint32_t buf[ALIGN_DWORD(2*CV_IO_COMMAND_BUF_SIZE
#else
    uint32_t buf[ALIGN_DWORD((4 * 1024 - 16)
#endif 
#if 0 //ndef CV_USBBOOT
			+ FP_CAPTURE_LARGE_HEAP_SIZE
#endif /* CV_USBBOOT */
	)/4];
} cv_secure_iobuf;

#define CV_SECURE_IO_FP_ASYNC_BUF   CV_SECURE_IO_COMMAND_BUF + (CV_IO_COMMAND_BUF_SIZE)
#define FP_CAPTURE_LARGE_HEAP_PTR	(CV_SECURE_IO_COMMAND_BUF + 2*CV_IO_COMMAND_BUF_SIZE)

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
	uint32_t				count;					/* 2T count */
	uint32_t				tldMultibufferIndex;	/* index for TLD multibuffering */
	uint32_t				actualCount;			/* counter value from object in case of mismatch */
} cv_volatile;

#if 1 //def CV_USBBOOT
extern cv_secure_iobuf cv_secure_iobuf_ptr;
#define  CV_SECURE_IO_COMMAND_BUF  ((cv_secure_iobuf *)&cv_secure_iobuf_ptr)
extern cv_volatile cv_volatile_data;
#define  CV_VOLATILE_DATA  ((cv_volatile *)&cv_volatile_data)
#endif /* CV_USBBOOT */

typedef struct td_cv_persistent {
	cv_persistent_flags	persistentFlags;			/* CV persistent flags */
	cv_policy			cvPolicy;					/* CV policy */
	byte				UCKKey[AES_128_BLOCK_LEN];	/* Unique CV encryption key used for encrypting objects within security
											boundary of CV, but stored externally */
	byte				GCKKey[AES_128_BLOCK_LEN];	/* Optional group corporate encryption key used for encrypting UCK for
											storage on hard drive */
	cv_rsa2048_key		rsaSecureSessionSetupKey;	/* RSA key used for secure session setup */
	cv_ecc256_no_dp		eccSecureSessionSetupKey;	/* ECC key used for secure session setup */
	PACKED union {
		cv_dsa1024_key_priv_only dsaKey;
		cv_ecc256_no_dp eccKey;
	}					secureSessionSigningKey;	/* key used for signing secureSessionSetupKey */
	sint32_t				localTimeZoneOffset;		/* offset +/- of local time from GMT */
	uint32_t			DAInitialLockoutPeriod;		/* configured lockout period for dictionary attack processing */
	uint16_t			DAAuthFailureThreshold;		/* Configured count of auth failures before lockout */
	uint32_t			spare1[4];
	uint8_t				BIOSSharedSecret[SHA1_LEN];/* shared secret with BIOS */
	uint8_t				secureAppSharedSecret[SHA256_LEN];/* shared secret with secure application */
	uint32_t			fpPollingInterval;			/* interval in ms for polling for finger detect */
	uint32_t			fpPersistence;				/* interval in ms for fingerprint to pesist after swipe occurs */
	uint32_t			identifyUserTimeout;		/* timeout in ms for cv_identify_user command */
	sint32_t				FARval;						/* false acceptance rate value for FP processing */
	cv_hash_type		fpAntiReplayHMACType;		/* fingerprint anti-replay HMAC type */
	cv_super_config		superConfigParams;			/* parameters for SUPER auth algorithm */
	cv_device_enables	deviceEnables;				/* enables for USH devices */
	uint8_t				spare[123];
	/* following item is variable length, but has maximum size indicated by array */
	uint32_t			CVAdminAuthListLen;			/* Length of CV admin auth list */
	cv_auth_list		CVAdminAuthList;			/* CV admin auth list */
	uint8_t				authList[MAX_CV_ADMIN_AUTH_LISTS_LENGTH];
} cv_persistent;

typedef PACKED struct td_cv_obj_properties {
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
} cv_obj_properties;

typedef PACKED struct td_cv_export_blob {
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
} cv_export_blob;

typedef PACKED struct td_cv_mscapi_blob {
	cv_mscapi_blob_type	bType;	/* blob type */
	uint8_t				bVersion;			/* blob version */
	uint16_t			reserved;			/* not used */
	cv_mscapi_alg_id	aiKeyAlg;			/* algorithm identifier for wrapped key */
	/* remainder is variable length of the form:
			blob type-specific header
			key		*/
} cv_mscapi_blob;

typedef PACKED struct td_cv_mscapi_simple_blob_hdr {
	cv_mscapi_alg_id	aiKeyAlg;			/* encryption algorithm used to encrypt key */
} cv_mscapi_simple_blob_hdr;

typedef PACKED struct td_cv_mscapi_rsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in modulus */
	uint32_t		pubexp;	/* public exponent */
} cv_mscapi_rsa_blob_hdr;

typedef PACKED struct td_cv_mscapi_dsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime, P */
} cv_mscapi_dsa_blob_hdr;

typedef PACKED struct td_cv_mscapi_dh_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime modulus, P */
} cv_mscapi_dh_blob_hdr;

typedef struct td_cv_param_list_entry {
	cv_param_type	paramType;	/* type of parameter */
	uint32_t		paramLen;	/* length of parameter */
	uint32_t		param;		/* parameter (variable length) */
} cv_param_list_entry;

typedef PACKED struct td_cv_internal_callback_ctx {
	cv_encap_command	*cmd;	/* command being processed */
} cv_internal_callback_ctx;

typedef PACKED struct td_cv_input_parameters {
	uint32_t numParams;	/* number of input parameters */
	PACKED struct {
		uint32_t paramLen;	/* length of parameter */
		void *param;		/* pointer to parameter */
	} paramLenPtr[MAX_INPUT_PARAMS];
} cv_input_parameters;

typedef PACKED struct td_cv_fp_callback_ctx {
	cv_callback		*callback;	/* callback routine passed in with API */
	cv_callback_ctx context;	/* context passed in with API */
} cv_fp_callback_ctx;

/* flash size structures */
#define CV_NUM_FLASH_MULTI_BUFS 2		/* number of multiple buffers for flash object where necessary (top level */
										/* directory and persistent data) */
#define CV_FLASH_MULTI_BUF_MASK (CV_NUM_FLASH_MULTI_BUFS - 1)
typedef PACKED struct td_cv_flash_dir_page_0_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_dir_page_0			dirPage0;					/* directory page 0 */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
} cv_flash_dir_page_0_entry;

typedef struct td_cv_flash_dir_page_0 {
	cv_flash_dir_page_0_entry dir_page_0[CV_NUM_FLASH_MULTI_BUFS];
} cv_flash_dir_page_0;

typedef PACKED struct td_cv_flash_top_level_dir_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_top_level_dir		topLevelDir;				/* top level dir */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} cv_flash_top_level_dir_entry;

typedef struct td_cv_flash_top_level_dir {
	cv_flash_top_level_dir_entry top_level_dir[CV_NUM_FLASH_MULTI_BUFS];
} cv_flash_top_level_dir;

typedef struct td_cv_persistent_data_entry {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_persistent			persistentData;				/* top level dir */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
	uint8_t					spare[64];
} cv_flash_persistent_data_entry;

typedef struct td_cv_flash_persistent_data {
	cv_flash_persistent_data_entry persistentData[CV_NUM_FLASH_MULTI_BUFS];
} cv_flash_persistent_data;

#define CV_PERSISTENT_OBJECT_HEAP_LENGTH (111*1024) /* 111K */

typedef struct td_flash_cv {
	uint8_t						spare1[4*1024];		/* spare to allow for growth of persistent object heap */
	uint8_t						persistentObjectHeap[CV_PERSISTENT_OBJECT_HEAP_LENGTH];
	cv_flash_dir_page_0			dirPage0;			
	uint8_t						spare2[1024];		/* spare to allow for growth of dirPage0 */
	cv_flash_persistent_data	persistentData;
	uint8_t						spare3[1024];		/* spare to allow for growth of persistent data */
	cv_flash_top_level_dir		topLevelDir;
	uint8_t						spare4[1024];		/* spare to allow for growth of top level dir */
} flash_cv;

#define CV_DIR_PAGE_0_FLASH_OFFSET ((uint32_t)&(((cv_flash_dir_page_0 *)0)->dir_page_0[1])) /* offset between buffers for multi-buffering (max size) */
#define CV_PERSISTENT_DATA_FLASH_OFFSET ((uint32_t)&(((cv_flash_persistent_data *)0)->persistentData[1]))	/* offset between buffers for multi-buffering (max size) */
#define CV_TOP_LEVEL_DIR_FLASH_OFFSET ((uint32_t)&(((cv_flash_top_level_dir *)0)->top_level_dir[1])) /* offset between buffers for multi-buffering (max size) */

typedef struct td_cv_heap_buf  {
	uint32_t	heap[ALIGN_DWORD(CV_VOLATILE_OBJECT_HEAP_LENGTH)/4];
} cv_heap_buf;

typedef struct td_usbh_heap_buf  {
	uint32_t	heap[ALIGN_DWORD(USBH_HEAP_SIZE)/4];
} usbh_heap_buf;

/*
 * function prototypes
 */
void
cvManager(uint32_t);

void
cvFpAsyncCapture(void);

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
			  uint8_t *HMAC, uint32_t HMACLen,
			  uint8_t *otherHashData, uint32_t otherHashDataLen);

cv_status
cvEncryptBlob(uint8_t *authStart, uint32_t authLen, uint8_t *cryptStart, uint32_t crtypLen,
			  uint8_t *cryptStart2, uint32_t cryptLen2,
			  uint8_t *IV, uint8_t *AESKey, uint8_t *IV2, uint8_t *AESKey2,
			  uint8_t *HMACKey, uint8_t *HMAC, uint32_t HMACLen,
			  uint8_t *otherHashData, uint32_t otherHashDataLen);

cv_status
cvAuth(uint8_t *authStart, uint32_t authLen, uint8_t *HMACKey, uint8_t *hashOrHMAC, uint32_t hashOrHMACLen,
			  uint8_t *otherHashData, uint32_t otherHashDataLen, cv_hash_op_internal hashOp);

cv_status
cvCrypt(uint8_t *cryptStart, uint32_t cryptLen, uint8_t *AESKey, uint8_t *IV, cv_bool encrypt);

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
cvWaitInsertion(cv_callback *callback, cv_callback_ctx context, cv_status prompt);

cv_status
cvHandlerPromptOrPin(cv_status status, cv_obj_properties *objProperties,
					 cv_callback *callback, cv_callback_ctx context);

cv_status
cvHandlerDetermineObjAvail(cv_obj_properties *objProperties, cv_callback *callback, cv_callback_ctx context);

void
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
cvUSHfpCapture(cv_bool doGrabOnly, cv_bool doFingerDetect,
			   uint8_t * image, uint32_t *imageLength, uint32_t *imageWidth,
			   cv_callback *callback, cv_callback_ctx context);

cv_status
cvUSHfpIdentify(cv_handle cvHandle, sint32_t FARvalue,
				uint32_t featureSetLength, uint8_t *featureSet,
				uint32_t templateCount, cv_obj_handle *templateHandles,
				uint32_t templateLength, byte *pTemplate,
				cv_obj_handle *identifiedTemplate, cv_bool *match,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cvUSHfpVerify(cv_bool doFingerDetect, sint32_t FARvalue,
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
cvUSHfpEnableHWFingerDetect(cv_bool enable);

uint8_t
fpFingerDetectCallback(uint8_t status, void *context);

uint8_t
cvFpSwFdetTimerISR(void);

cv_status
cvHostFPAsyncInterrupt(void);

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
							  cv_bool *needObjWrite);

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
isRadioEnabled(void);

cv_status
cvReadContactlessID(uint32_t PINLen, uint8_t *PIN, cv_contactless_type *type, uint32_t *len, uint8_t *id);

#endif				       /* end _CVINTERNAL_H_ */
