/******************************************************************************
 *  Copyright (C) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc and/or its subsidiaries.
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
/*
 * Broadcom Corporation Credential Vault API
 */
/*
 * cvapi.h:  Credential Vault function prototypes and structures
 */
/*
 * Revision History:
 *
 * 01/02/07 DPA Created.
*/

#ifndef _CVAPI_H_
#define _CVAPI_H_ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>

#pragma pack(1)
#define PACKED //__packed

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**********************************************************************
 *  Basic types
**********************************************************************/

#define ULONG_PTR uintptr_t
typedef unsigned long DWORD;
typedef unsigned short BOOL;
#define ULSIZE sizeof(uintptr_t)

typedef void* HANDLE;

#ifdef UNICODE
 typedef wchar_t TCHAR;
#else
 typedef char TCHAR;
#endif

/*
 * Enumerated values and types
 */
typedef unsigned char byte;
typedef unsigned char cv_bool;

typedef uint32_t cv_handle;
typedef uint32_t cv_obj_handle;

typedef uint32_t CSSAUTHCONTEXT;
typedef cv_obj_handle CSSID;
typedef unsigned int BIPSTATUS;

typedef unsigned int cv_options;
#define CV_SECURE_SESSION		0x00000001
#define CV_USE_SUITE_B			0x00000002
#define CV_SYNCHRONOUS			0x00000004
#define CV_SUPPRESS_UI_PROMPTS		0x00000008
#define CV_INTERMEDIATE_CALLBACKS	0x00000010
#define CV_FP_INTERMEDIATE_CALLBACKS	0x00000020
#define CV_OPTIONS_MASK (CV_SECURE_SESSION | CV_USE_SUITE_B | CV_SYNCHRONOUS | CV_SUPPRESS_UI_PROMPTS | \
							CV_INTERMEDIATE_CALLBACKS | CV_FP_INTERMEDIATE_CALLBACKS)

typedef unsigned int cv_fp_control;
#define CV_UPDATE_TEMPLATE		0x00000001
#define CV_INVALIDATE_CAPTURE	0x00000002
#define CV_HINT_AVAILABLE		0x00000004
#define CV_USE_FAR				0x00000008
#define CV_USE_FEATURE_SET		0x00000010
#define CV_USE_FEATURE_SET_HANDLE 0x00000020
#define CV_USE_PERSISTED_IMAGE_ONLY 0x00000040
#define CV_CONTAINS_CAPTURE_ID  0x00000080
#define CV_ASYNC_FP_CAPTURE_ONLY  0x00000100
/* this mask defines the only bits valid for input parameter */
#define CV_FP_CONTROL_MASK (CV_INVALIDATE_CAPTURE | CV_HINT_AVAILABLE | CV_USE_FAR | CV_USE_FEATURE_SET | CV_USE_FEATURE_SET_HANDLE | CV_USE_PERSISTED_IMAGE_ONLY | CV_CONTAINS_CAPTURE_ID | CV_ASYNC_FP_CAPTURE_ONLY)

typedef unsigned int cv_fp_result;
#define CV_MATCH_FOUND			0x00000001
#define CV_TEMPLATE_UPDATED		0x00000002

typedef unsigned int cv_policy;
#define CV_ALLOW_SESSION_AUTH		0x00000001
#define CV_REQUIRE_OTP_SIGNED_TIME	0x00000002
#define CV_DISALLOW_ASYNC_FP_SWIPE	0x00000004
#define CV_DISALLOW_FAR_PARAMETER	0x00000008

typedef unsigned short cv_obj_auth_flags;
#define CV_OBJ_AUTH_READ_PUB		0x00000001
#define CV_OBJ_AUTH_READ_PRIV		0x00000002
#define CV_OBJ_AUTH_WRITE_PUB		0x00000004
#define CV_OBJ_AUTH_WRITE_PRIV		0x00000008
#define CV_OBJ_AUTH_DELETE			0x00000010
#define CV_OBJ_AUTH_ACCESS			0x00000020
#define CV_OBJ_AUTH_CHANGE_AUTH		0x00000040
#define CV_OBJ_AUTH_EXPORT			0x00000080
#define CV_ADMIN_PBA_BIOS_UPDATE    0x00000800
#define CV_CHANGE_AUTH_PARTIAL 		0x00001000
#define CV_CLEAR_ADMIN_AUTH			0x00002000
#define CV_PHYSICAL_PRESENCE_AUTH	0x00004000
#define CV_ADMINISTRATOR_AUTH		0x00008000
#define CV_OBJ_AUTH_FLAGS_MASK (CV_OBJ_AUTH_READ_PUB | CV_OBJ_AUTH_READ_PRIV | CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV \
								| CV_OBJ_AUTH_DELETE | CV_OBJ_AUTH_ACCESS | CV_OBJ_AUTH_CHANGE_AUTH | CV_OBJ_AUTH_EXPORT)

typedef unsigned short cv_auth_session_status;
#define CV_AUTH_SESSION_TERMINATED	0x00000001

typedef unsigned int cv_auth_session_flags;
#define CV_AUTH_SESSION_CONTAINS_SHARED_SECRET	0x00000001
#define CV_AUTH_SESSION_PER_USAGE_AUTH		0x00000002
#define CV_AUTH_SESSION_SINGLE_USE		0x00000004
#define CV_AUTH_SESSION_FLAGS_MASK (CV_AUTH_SESSION_CONTAINS_SHARED_SECRET | CV_AUTH_SESSION_PER_USAGE_AUTH | CV_AUTH_SESSION_SINGLE_USE)


#define CV_MAJOR_VERSION 2
#define CV_MINOR_VERSION 0

#define AES_BLOCK_LENGTH 16   /* AES block length is always 16, regardless of key length */
#define AES_128_BLOCK_LEN 16
#define AES_192_BLOCK_LEN 24
#define AES_256_BLOCK_LEN 32
#define AES_128_IV_LEN AES_128_BLOCK_LEN
#define AES_192_IV_LEN AES_192_BLOCK_LEN
#define AES_256_IV_LEN AES_256_BLOCK_LEN
#define SHA1_LEN 20
#define SHA256_LEN 32
#define CV_NONCE_LEN 20
#define DSA1024_KEY_LEN 128
#define DSA1024_KEY_LEN_BITS 1024
#define DSA1024_PRIV_LEN 20
#define DSA2048_PRIV_LEN 32
#define DSA_SIG_LEN 2*CV_NONCE_LEN
#define RSA2048_KEY_LEN 256
#define RSA2048_KEY_LEN_BITS 2048
#define RSA4096_KEY_LEN 512
#define RSA4096_KEY_LEN_BITS 4096
#define ECC256_KEY_LEN 32
#define ECC256_KEY_LEN_BITS ECC256_KEY_LEN*8
#define ECC256_SIG_LEN 2*ECC256_KEY_LEN
#define ECC256_POINT (2*ECC256_KEY_LEN)
#define ECC256_SIZE_R	ECC256_KEY_LEN
#define ECC256_SIZE_S	ECC256_KEY_LEN
#define MAX_CV_COMMAND_LENGTH (0x20000+300)/* Increased for flash upgrade */
#define MAX_CV_OBJ_SIZE 2048 /* maximum object size in bytes */
#define MAX_CV_ADMIN_AUTH_LISTS_LENGTH 256	/* maximum length of auth lists associated with CV admin.  can't be changed */
						/* unless flash mapping impact is evaluated */
#define CV_GCK_KEY_LEN 128
#define CV_DA_MIN_MAX_FAILED_ATTEMPTS 1
#define CV_DA_MAX_MAX_FAILED_ATTEMPTS 20
#define CV_DA_MIN_INITIAL_LOCKOUT_PERIOD 1
#define CV_DA_MAX_INITIAL_LOCKOUT_PERIOD 30
#define CV_MAX_RANDOM_NUMBER_LEN 256
#define CV_MAX_CTKIP_PUBKEY_LEN 256
#define CV_SUPER_LENGTH 16			/* byte length of parameters associated with SUPER auth type */
#define MAX_ATTRIB_TYPE_LABEL_LENGTH 20
#define MAX_CONTACTLESS_ID_LENGTH 20 /* maximum byte length of ID for all contactless cards supported by ProcessCard */
#define CV_SUPER_RSN_LEN 32
#define HOTP_KEY_MAX_LEN 32

#define GENERAL_PROMPT_RETURN_CODE_MASK 0x00010000
#define SC_PROMPT_RETURN_CODE_MASK		0x00020000
#define FP_PROMPT_RETURN_CODE_MASK		0x00040000
#define CV_USER_LIB_RETURN_CODE_MASK	0x00080000
#define CV_USER_INTERFACE_RETURN_CODE_MASK	0x00100000
typedef unsigned int cv_status;
enum cv_status_e {
		CV_SUCCESS,									// 0x00000000
		CV_SUCCESS_SUBMISSION,						// 0x00000001
		CV_READ_HID_CREDENTIAL = 4,					// 0x00000004
		CV_INVALID_HANDLE = 6,						// 0x00000006
		CV_OBJECT_NOT_VALID,						// 0x00000007
		CV_AUTH_FAIL,								// 0x00000008
		CV_OBJECT_WRITE_REQUIRED,					// 0x00000009
		CV_IN_LOCKOUT,								// 0x0000000A
		CV_INVALID_VERSION = 0xC,					// 0x0000000C
		CV_PARAM_BLOB_INVALID_LENGTH,				// 0x0000000D
		CV_PARAM_BLOB_INVALID,						// 0x0000000E
		CV_PARAM_BLOB_AUTH_FAIL,					// 0x0000000F
		CV_PARAM_BLOB_DECRYPTION_FAIL,				// 0x00000010
		CV_PARAM_BLOB_ENCRYPTION_FAIL,				// 0x00000011
		CV_PARAM_BLOB_RNG_FAIL,						// 0x00000012
		CV_SECURE_SESSION_SUITE_B_REQUIRED,			// 0x00000013
		CV_SECURE_SESSION_KEY_GENERATION_FAIL,		// 0x00000014
		CV_SECURE_SESSION_KEY_HASH_FAIL,			// 0x00000015
		CV_SECURE_SESSION_KEY_SIGNATURE_FAIL,		// 0x00000016
		CV_VOLATILE_MEMORY_ALLOCATION_FAIL,			// 0x00000017
		CV_SECURE_SESSION_BAD_PARAMETERS,			// 0x00000018
		CV_SECURE_SESSION_SHARED_SECRET_FAIL,		// 0x00000019
		CV_CALLBACK_TIMEOUT,						// 0x0000001A
		CV_INVALID_OBJECT_HANDLE,					// 0x0000001B
		CV_HOST_STORAGE_REQUEST_TIMEOUT,			// 0x0000001C
		CV_HOST_STORAGE_REQUEST_RESULT_INVALID,		// 0x0000001D
		CV_OBJ_AUTH_FAIL,							// 0x0000001E
		CV_OBJ_DECRYPTION_FAIL,						// 0x0000001F
		CV_OBJ_ANTIREPLAY_FAIL,						// 0x00000020
		CV_OBJ_VALIDATION_FAIL,						// 0x00000021
		CV_OBJECT_ATTRIBUTES_INVALID = 0x24,		// 0x00000024
		CV_NO_PERSISTENT_OBJECT_ENTRY_AVAIL,		// 0x00000025
		CV_OBJECT_DIRECTORY_FULL,					// 0x00000026
		CV_NO_VOLATILE_OBJECT_ENTRY_AVAIL,			// 0x00000027
		CV_FLASH_MEMORY_ALLOCATION_FAIL,			// 0x00000028
		CV_ENUMERATION_BUFFER_FULL,					// 0x00000029
		CV_ADMIN_AUTH_NOT_ALLOWED,					// 0x0000002A
		CV_ADMIN_AUTH_REQUIRED,						// 0x0000002B
		CV_CORRESPONDING_AUTH_NOT_FOUND_IN_OBJECT,	// 0x0000002C
		CV_INVALID_AUTH_LIST,						// 0x0000002D
		CV_UNSUPPORTED_CRYPT_FUNCTION,				// 0x0000002E
		CV_CANCELLED_BY_USER,						// 0x0000002F
		CV_POLICY_DISALLOWS_SESSION_AUTH,			// 0x00000030
		CV_CRYPTO_FAILURE,							// 0x00000031
		CV_RNG_FAILURE, 							// 0x00000032
		CV_INVALID_OUTPUT_PARAMETER_LENGTH = 0x34, 	// 0x00000034
		CV_INVALID_OBJECT_SIZE,						// 0x00000035
		CV_INVALID_GCK_KEY_LENGTH = 0x37,			// 0x00000037
		CV_INVALID_DA_PARAMS,						// 0x00000038
		CV_CV_NOT_EMPTY,							// 0x00000039
		CV_NO_GCK,									// 0x0000003A
		CV_RTC_FAILURE,								// 0x0000003B
		CV_INVALID_KDIX_KEY,						// 0x0000003C
		CV_INVALID_KEY_TYPE,						// 0x0000003D
		CV_INVALID_KEY_LENGTH,						// 0x0000003E
		CV_KEY_GENERATION_FAILURE,					// 0x0000003F
		CV_INVALID_KEY_PARAMETERS,					// 0x00000040
		CV_INVALID_OBJECT_TYPE,						// 0x00000041
		CV_INVALID_ENCRYPTION_PARAMETER,			// 0x00000042
		CV_INVALID_HMAC_KEY,						// 0x00000043
		CV_INVALID_INPUT_PARAMETER_LENGTH, 			// 0x00000044
		CV_INVALID_BULK_ENCRYPTION_PARAMETER,		// 0x00000045
		CV_ENCRYPTION_FAILURE,						// 0x00000046
		CV_INVALID_INPUT_PARAMETER,					// 0x00000047
		CV_SIGNATURE_FAILURE,						// 0x00000048
		CV_INVALID_OBJECT_PERSISTENCE,				// 0x00000049
		CV_OBJECT_NONHASHABLE,						// 0x0000004A
		CV_SIGNED_TIME_REQUIRED,					// 0x0000004B
		CV_INVALID_SIGNATURE,						// 0x0000004C
		CV_INTERNAL_DEVICE_FAILURE,					// 0x0000004D
		CV_FLASH_ACCESS_FAILURE,					// 0x0000004E
		CV_RTC_NOT_AVAILABLE,						// 0x0000004F
        CV_USH_BOOT_FAILURE,                		// 0x00000050
        CV_INVALID_FINGERPRINT_TYPE,        		// 0x00000051
		CV_FAR_PARAMETER_DISALLOWED,				// 0x00000052
		CV_FAR_VALUE_NOT_CONFIGURED,				// 0x00000053
		CV_FINGERPRINT_CAPTURE_FAILED,				// 0x00000054
		CV_HOST_CONTROL_REQUEST_TIMEOUT,			// 0x00000055
		CV_HOST_CONTROL_REQUEST_RESULT_INVALID,		// 0x00000056
		CV_INVALID_COMMAND,							// 0x00000057
		CV_INVALID_COMMAND_LENGTH,					// 0x00000058
		CV_FP_MATCH_GENERAL_ERROR,					// 0x00000059
		CV_FP_DEVICE_GENERAL_ERROR,					// 0x0000005A
		CV_NO_BIOS_SHARED_SECRET,					// 0x0000005B
		CV_INVALID_HASH_TYPE,						// 0x0000005C
		CV_IDENTIFY_USER_FAILED,					// 0x0000005D
		CV_CONTACTLESS_FAILURE,						// 0x0000005E
		CV_INVALID_CONTACTLESS_CARD_TYPE,			// 0x0000005F
		CV_IDENTIFY_USER_TIMEOUT,					// 0x00000060
		CV_INVALID_IMPORT_BLOB,						// 0x00000061
		CV_CANT_REMAP_HANDLES,						// 0x00000062
		CV_OBJECT_REQUIRES_PRIVACY_KEY,				// 0x00000063
		CV_SMART_CARD_FAILURE,						// 0x00000064
		CV_INVALID_SMART_CARD_TYPE,					// 0x00000065
		CV_SUPER_AUTH_TYPE_NOT_CONFIGURED,			// 0x00000066
		CV_DIAG_FAIL,								// 0x00000067
		CV_MONOTONIC_COUNTER_FAIL,					// 0x00000068
		CV_FW_UPGRADE_START_FAILED,					// 0x00000069
		CV_FW_UPGRADE_UPDATE_FAILED,				// 0x0000006A
		CV_FW_UPGRADE_COMPLETE_FAILED,				// 0x0000006B
		CV_FP_USER_TIMEOUT,							// 0x0000006C
		CV_ANTIHAMMERING_PROTECTION,				// 0x0000006D
		CV_UNALIGNED_ENCRYPTION_DATA,				// 0x0000006E
		CV_FP_BAD_SWIPE,							// 0x0000006F
		CV_ANTIHAMMERING_PROTECTION_DELAY_MED,		// 0x00000070
		CV_ANTIHAMMERING_PROTECTION_DELAY_HIGH,		// 0x00000071
		CV_RADIO_DISABLED_AND_LOCKED,				// 0x00000072
		CV_FEATURE_NOT_AVAILABLE,					// 0x00000073
		CV_RADIO_NOT_PRESENT,						// 0x00000074
		CV_FP_NOT_PRESENT,							// 0x00000075
		CV_RADIO_NOT_ENABLED,						// 0x00000076
		CV_FP_RESET,								// 0x00000077
		CV_SC_NOT_PRESENT,							// 0x00000078
		CV_SC_NOT_ENABLED,							// 0x00000079
		CV_FP_NOT_ENABLED,							// 0x0000007A
		CV_PHYSICAL_PRESENCE_AUTH_NOT_ALLOWED,		// 0x0000007B
		CV_ANTIHAMMERING_PROTECTION_REBOOT,			// 0x0000007C
		CV_ANTIHAMMERING_PROTECTION_FLSHPRG,		// 0x0000007D
		CV_ANTIHAMMERING_PROTECTION_FWUP_FAIL,		// 0x0000007E
		CV_INVALID_RFID_PARAMETERS,					// 0x0000007F
		CV_OBJECT_ATTRIBUTES_NOT_IN_GLB_BKUP,		// 0x00000080
		CV_INVALID_OBJECT_ID,						// 0x00000081
		CV_SUPER_NOT_REGISTERED,					// 0x00000082
		CV_SUPER_RESPONSE_FAILURE,					// 0x00000083
		CV_MANAGED_MODE_REQUIRED,					// 0x00000084
		CV_FP_DEVICE_BUSY,							// 0x00000085
		CV_PERSISTED_FP_AVAILABLE,					// 0x00000086
		CV_NO_CAPTURE_INITIATED,					// 0x00000087
		CV_INVALID_CAPTURE_ID,						// 0x00000088
		CV_NO_VALID_FP_IMAGE,						// 0x00000089
		CV_MUTEX_CREATION_FAILURE,					// 0x0000008a
		CV_TASK_CREATION_FAILURE,					// 0x0000008b
		CV_FP_TEMPLATE_EXISTS,						// 0x0000008c
		CV_NO_VALID_FP_TEMPLATE,					// 0x0000008d
		CV_INVALID_ENROLLMENT_ID,					// 0x0000008e
		CV_MORE_DATA,								// 0x0000008f
		CV_FP_RSLT_TOO_HIGH,						// 0x00000090
		CV_FP_RSLT_TOO_LOW,							// 0x00000091
		CV_FP_RSLT_TOO_LEFT,						// 0x00000092
		CV_FP_RSLT_TOO_RIGHT,						// 0x00000093
		CV_FP_RSLT_TOO_FAST,						// 0x00000094
		CV_FP_RSLT_TOO_SLOW,						// 0x00000095
		CV_FP_RSLT_POOR_QUALITY,					// 0x00000096
		CV_FP_RSLT_TOO_SKEWED,						// 0x00000097
		CV_FP_RSLT_TOO_SHORT,						// 0x00000098
		CV_FP_RSLT_MERGE_FAILURE,					// 0x00000099
		CV_FP_DUPLICATE,							// 0x0000009a
/*********  add general return codes above this line *************/

/* general prompt related return codes */
		CV_REMOVE_PROMPT = GENERAL_PROMPT_RETURN_CODE_MASK,
		CV_PROMPT_SUPRESSED,

/* smart card and contactless prompt related return codes */
		CV_PROMPT_FOR_SMART_CARD = SC_PROMPT_RETURN_CODE_MASK,
		CV_PROMPT_FOR_CONTACTLESS,
		CV_PROMPT_PIN,
		CV_PROMPT_PIN_AND_SMART_CARD,
		CV_PROMPT_PIN_AND_CONTACTLESS,
		CV_PROMPT_CONTACTLESS_DETECTED,

/* fingerprint prompt related return codes */
		CV_PROMPT_FOR_FINGERPRINT_SWIPE = FP_PROMPT_RETURN_CODE_MASK,
		CV_PROMPT_FOR_FINGERPRINT_TOUCH,
		CV_PROMPT_FOR_FIRST_FINGERPRINT_SWIPE,
		CV_PROMPT_FOR_FIRST_FINGERPRINT_TOUCH,
		CV_PROMPT_FOR_SECOND_FINGERPRINT_SWIPE,
		CV_PROMPT_FOR_SECOND_FINGERPRINT_TOUCH,
		CV_PROMPT_FOR_THIRD_FINGERPRINT_SWIPE,
		CV_PROMPT_FOR_THIRD_FINGERPRINT_TOUCH,
		CV_PROMPT_FOR_FOURTH_FINGERPRINT_SWIPE,
		CV_PROMPT_FOR_FOURTH_FINGERPRINT_TOUCH,
		CV_PROMPT_FOR_RESAMPLE_SWIPE,
		CV_PROMPT_FOR_RESAMPLE_TOUCH,
		CV_PROMPT_FOR_RESAMPLE_SWIPE_TIMEOUT,
		CV_PROMPT_FOR_RESAMPLE_TOUCH_TIMEOUT,

/* CV User Library return codes */
		CV_INVALID_OBJ_AUTH_FLAG = CV_USER_LIB_RETURN_CODE_MASK,
		CV_MEMORY_ALLOCATION_FAIL,
		CV_INVALID_PERSISTENT_TYPE,
		CV_INVALID_LIBRARY,
		CV_ERROR_LOADING_INTERFACE_LIBRARY,
		CV_FAILURE,
		CV_SECURE_SESSION_FAILURE,
		CV_INVALID_SUITEB_FLAG,
		CV_INVALID_CALLBACK_ADDRESS,
		CV_GENERAL_ERROR,
		CV_INVALID_BLOB_TYPE,
		CV_INVALID_ENCRYPT,
		CV_INVALID_DECRYPT,
		CV_INVALID_HMAC_TYPE,
		CV_INVALID_SIGN_TYPE,
		//CV_INVALID_HASH_TYPE,
		CV_INVALID_VERIFY,
		//CV_INVALID_COMMAND,
		CV_INVALID_AUTH_ALG,
		CV_INVALID_DEVICE,
		CV_INVALID_OTP_PROV,
		CV_INVALID_MAC_TYPE,
		CV_INVALID_CONFIG_TYPE,
		CV_INVALID_ENC_OP_TYPE,
		CV_INVALID_DEC_OP_TYPE,
		CV_INVALID_HASH_OP,
		CV_INVALID_BULK_MODE,
		CV_INVALID_OPTIONS,
		CV_INVALID_APPID,
		CV_INVALID_USERID,
		CV_INVALID_INBOUND_PARAM_TYPE,
		CV_INVALID_IPADDRESS,
		CV_INVALID_AUTHORIZATION_TYPE,
		CV_INVALID_STATUS_TYPE,
		CV_INVALID_CONTACTLESS_TYPE,
		CV_INVALID_SESSION,
		CV_FAILED_OPEN_REMOTE_SESSION,
		CV_FAILED_CLOSE_REMOTE_SESSION,
		CV_HMAC_VERIFICATION_FAILURE,

/* CV User Interface return codes */
		CV_UI_TRANSMIT_INVALID_INPUT = CV_USER_INTERFACE_RETURN_CODE_MASK,
		CV_UI_HSR_INVALID_INPUT,
		CV_UI_FAILED_SEMAPHORE_CREATION,
		CV_UI_FAILED_COMMAND_PROCESS,
		CV_UI_FAILED_COMMAND_SUBMISSION,
		CV_UI_FAILED_ASYNC_THREAD_CREATION,
		CV_UI_FAILED_HOST_STORE_REQUEST,
		CV_UI_FAILED_ABORT_COMMAND,
		CV_UI_TRANSMIT_SERVER_INVALID_INPUT,
		CV_FAILED_REMOTE_SERVER_FUNCTION,
		CV_UI_FAILED_FP_REGISTER_EVENT_REQUEST,
		CV_UI_TRANSMIT_CLIENT_INVALID_INPUT,
		CV_UI_HCR_INVALID_INPUT,
		CV_PROMPT_PIN_FAILURE,
		CV_PROMPT_FAILURE,
		CV_INVALID_HOST_STORE_REQUEST,
		CV_INVALID_HOST_STORE_COMMAND_ID,
		CV_HOST_STORE_REQUEST_FAILED,
		CV_HOST_STORE_REQUEST_SUBMISSION_FAILED,
		CV_HOST_CONTROL_REQUEST_FAILED,
		CV_HOST_CONTROL_REQUEST_SUBMISSION_FAILED,
		CV_UI_FAILED_EVENT
};

typedef unsigned int cv_persistence;
enum cv_persistence_e {
		CV_KEEP_PERSISTENCE,
		CV_MAKE_PERSISTENT,
		CV_MAKE_VOLATILE
};

typedef unsigned int cv_blob_type;
enum cv_blob_type_e {
		CV_NATIVE_BLOB,
		CV_MSCAPI_BLOB,
		CV_CNG_BLOB,
		CV_PKCS_BLOB,
		CV_NATIVE_BLOB_RETAIN_HANDLE
};

typedef unsigned int cv_enc_op;
enum cv_enc_op_e {
		CV_BULK_ENCRYPT,
		CV_BULK_ENCRYPT_THEN_AUTH,
		CV_BULK_AUTH_THEN_ENCRYPT,
		CV_ASYM_ENCRYPT
};

typedef unsigned int cv_dec_op;
enum cv_dec_op_e {
		CV_BULK_DECRYPT,
		CV_BULK_DECRYPT_THEN_AUTH,
		CV_BULK_AUTH_THEN_DECRYPT,
		CV_ASYM_DECRYPT
};

typedef unsigned int cv_auth_op;
enum cv_auth_op_e {
		CV_AUTH_SHA,
		CV_AUTH_HMAC_SHA
};

typedef unsigned int cv_hmac_op;
enum cv_hmac_op_e {
		CV_HMAC_SHA,
		CV_HMAC_SHA_START,
		CV_HMAC_SHA_UPDATE,
		CV_HMAC_SHA_FINAL
};

typedef unsigned int cv_hash_op;
enum cv_hash_op_e {
		CV_SHA,
		CV_SHA_START,
		CV_SHA_UPDATE,
		CV_SHA_FINAL
};

typedef unsigned int cv_sign_op;
enum cv_sign_op_e {
		CV_SIGN_NORMAL,
		CV_SIGN_USE_KDI,
		CV_SIGN_USE_KDIECC
};

typedef unsigned int cv_verify_op;
enum cv_verify_op_e {
		CV_VERIFY_NORMAL
};

typedef unsigned int cv_bulk_mode;
enum cv_bulk_mode_e {
		CV_CBC,
		CV_ECB,
		CV_CTR,
		CV_CMAC,
		CV_CCMP,
		CV_CBC_NOPAD,
		CV_ECB_NOPAD
};

typedef unsigned int cv_auth_algorithm;
enum cv_auth_algorithm_e {
	CV_AUTH_ALG_NONE,
	CV_AUTH_ALG_HMAC_SHA1,
	CV_AUTH_ALG_SHA1,
	CV_AUTH_ALG_HMAC_SHA2_256,
	CV_AUTH_ALG_SHA2_256
};

typedef unsigned char cv_hash_type;
enum cv_hash_type_e {
		CV_SHA1,
		CV_SHA256
};

typedef unsigned int cv_key_type;
enum cv_key_type_e {
		CV_AES,
		CV_DH_GENERATE,
		CV_DH_SHARED_SECRET,
		CV_RSA,
		CV_DSA,
		CV_ECC,
		CV_ECDH_SHARED_SECRET,
		CV_OTP_RSA,
		CV_OTP_OATH
};

typedef unsigned int cv_status_type;
enum cv_status_type_e {
		CV_STATUS_VERSION,
		CV_STATUS_OP,
		CV_STATUS_SPACE,
		CV_STATUS_TIME,
		CV_STATUS_FP_POLLING_INTERVAL,
		CV_STATUS_FP_PERSISTANCE,
		CV_STATUS_FP_HMAC_TYPE,
		CV_STATUS_FP_FAR,
		CV_STATUS_DA_PARAMS,
		CV_STATUS_POLICY,
		CV_STATUS_HAS_CV_ADMIN,
		CV_STATUS_IDENTIFY_USER_TIMEOUT,
		CV_STATUS_ERROR_CODE_MORE_INFO,
		CV_STATUS_SUPER_CHALLENGE,
		CV_STATUS_IN_DA_LOCKOUT,
		CV_STATUS_DEVICES_ENABLED,
		CV_STATUS_ANTI_REPLAY_OVERRIDE_OCCURRED,
		CV_STATUS_RFID_PARAMS_ID,
		CV_STATUS_ST_CHALLENGE,
		CV_STATUS_IS_BOUND,
		CV_STATUS_HAS_SUPER_REGISTRATION
};

typedef unsigned int cv_config_type;
enum cv_config_type_e {
		CV_CONFIG_DA_PARAMS,
		CV_CONFIG_REPLAY_OVERRIDE,
		CV_CONFIG_POLICY,
		CV_CONFIG_TIME,
		CV_CONFIG_KDIX,
		CV_CONFIG_FP_POLLING_INTERVAL,
		CV_CONFIG_FP_PERSISTANCE,
		CV_CONFIG_FP_HMAC_TYPE,
		CV_CONFIG_FP_FAR,
		CV_CONFIG_IDENTIFY_USER_TIMEOUT,
		CV_CONFIG_SUPER,
		CV_CONFIG_RESET_ANTI_REPLAY_OVERRIDE
};

typedef unsigned int cv_operating_status;
enum cv_operating_status_e {
		CV_OK,
		CV_DEVICE_FAILURE,
		CV_MONOTONIC_COUNTER_FAILURE,
		CV_FLASH_INIT_FAILURE,
		CV_FP_DEVICE_FAILURE
};

typedef unsigned int cv_otp_mac_type;
enum cv_otp_mac_type_e {
		CV_MAC1,
		CV_MAC2
};

typedef unsigned short cv_obj_type;
enum cv_obj_type_e {
		CV_TYPE_CERTIFICATE = 1,
		CV_TYPE_RSA_KEY,
		CV_TYPE_DH_KEY,
		CV_TYPE_DSA_KEY,
		CV_TYPE_ECC_KEY,
		CV_TYPE_AES_KEY,
		CV_TYPE_FINGERPRINT,
		CV_TYPE_OPAQUE,
		CV_TYPE_OTP_TOKEN,
		CV_TYPE_PASSPHRASE,
		CV_TYPE_HASH,
		CV_TYPE_AUTH_SESSION,
		CV_TYPE_HOTP
};

typedef unsigned short cv_obj_attribute_type;
enum cv_obj_attribute_type_e {
		CV_ATTRIB_TYPE_FLAGS,
		CV_ATTRIB_TYPE_LABEL
};

typedef unsigned short cv_object_authorizations;
enum cv_object_authorizations_e {
		CV_AUTH_PASSPHRASE,
		CV_AUTH_PASSPHRASE_HMAC,
		CV_AUTH_FINGERPRINT,
		CV_AUTH_SESSION,
		CV_AUTH_SESSION_HMAC,
		CV_AUTH_COUNT,
		CV_AUTH_TIME,
		CV_AUTH_CONTACTLESS,
		CV_AUTH_PIN,
		CV_AUTH_SMARTCARD,
		CV_AUTH_PROPRIETARY_SC,
		CV_AUTH_CHALLENGE,
		CV_AUTH_SUPER,
		CV_AUTH_ST_CHALLENGE,
		CV_AUTH_CAC_PIV,
		CV_AUTH_PIV_CTLS,
		CV_AUTH_HOTP
};

typedef unsigned short cv_otp_provisioning_type;
enum cv_otp_provisioning_type_e {
		CV_OTP_PROV_DIRECT,
		CV_OTP_PROV_CTKIP
};

typedef unsigned char cv_otp_token_type;
enum cv_otp_token_type_e {
		CV_OTP_TYPE_RSA = 1,
		CV_OTP_TYPE_OATH = 2
};

typedef unsigned int cv_fp_config_data_type;
enum cv_fp_config_data_type_e {
		CV_FP_CONFIG_DATA_DEVICE,
		CV_FP_CONFIG_DATA_MATCHING
};

typedef unsigned int cv_identify_type;
enum cv_identify_type_e {
		CV_ID_TYPE_TEMPLATE,
		CV_ID_TYPE_USER_ID
};

typedef unsigned int cv_fp_event;
#define CV_FP_EVENT_FP_AVAIL		1
#define CV_FP_EVENT_CANCEL_OR_FAIL	2

typedef unsigned int cv_contactless_type;
enum cv_contactless_type_e {
		CV_CONTACTLESS_TYPE_HID,
		CV_CONTACTLESS_TYPE_PIV,
		CV_CONTACTLESS_TYPE_MIFARE_CLASSIC,
		CV_CONTACTLESS_TYPE_MIFARE_ULTRALIGHT,
		CV_CONTACTLESS_TYPE_MIFARE_DESFIRE,
		CV_CONTACTLESS_TYPE_FELICA,
		CV_CONTACTLESS_TYPE_ISO15693
};

typedef unsigned int cv_device_enables;
#define CV_RF_ENABLED 0x1
#define CV_SC_ENABLED 0x2
#define CV_FP_ENABLED 0x4
#define CV_RF_PRESENT 0x8
#define CV_SC_PRESENT 0x10
#define CV_FP_PRESENT 0x20
#define CV_RF_LOCKED  0x40
#define CV_CV_ONLY_RADIO_MODE	0x80
#define CV_RF_AUTODET 0x100
#define CV_RF_PRESENT_FORCED   0X200
#define CV_WBDI_ENABLED	0x400
#define CV_CV_ONLY_RADIO_MODE_PERSISTENT   0X800

typedef unsigned int cv_cac_piv_card_type;
enum cv_cac_piv_card_type_e {
	CV_TYPE_CAC,
	CV_TYPE_PIV
};

typedef unsigned int cv_cac_piv_card_signature_alg;
enum cv_cac_piv_card_signature_alg_e {
	CV_SIGN_ALG_RSA_1024,
	CV_SIGN_ALG_RSA_2048,
	CV_SIGN_ALG_ECC_256
};

typedef unsigned int cv_fp_cap_control;
#define CV_CAP_CONTROL_DO_FE				0x00000001
#define CV_CAP_CONTROL_TERMINATE			0x00000002
#define CV_CAP_CONTROL_PURPOSE_ENROLLMENT	0x00000004
#define CV_CAP_CONTROL_PURPOSE_VERIFY		0x00000008
#define CV_CAP_CONTROL_PURPOSE_IDENTIFY		0x00000010
#define CV_CAP_CONTROL_OVERRIDE_PERSISTENCE 0x00000020

typedef unsigned int cv_fp_fe_control;
#define CV_FE_CONTROL_PURPOSE_ENROLLMENT	0x00000004
#define CV_FE_CONTROL_PURPOSE_VERIFY		0x00000008
#define CV_FE_CONTROL_PURPOSE_IDENTIFY		0x00000010


typedef unsigned int cv_cap_prompt_type;
enum cv_cap_prompt_type_e {
	CV_CAP_PROMPT_START_CAPTURE,
	CV_CAP_PROMPT_SUCCESS,
	CV_CAP_PROMPT_BAD_SWIPE,
	CV_CAP_PROMPT_TIMEOUT,
	CV_CAP_PROMPT_CANCELLED,
	CV_CAP_PROMPT_IOCTL_FAIL,
	CV_CAP_PROMPT_RESET
};

/*
 * Structures
 */

typedef PACKED struct td_cv_auth_list_hdr {
	uint8_t				numEntries;	/* number of auth list entries */
	uint8_t				listID;		/* ID of auth list */
	cv_obj_auth_flags	flags;		/* booleans indicating authorization granted */
} cv_auth_list_hdr;
typedef PACKED cv_auth_list_hdr cv_auth_list;

typedef PACKED struct td_cv_attrib_hdr {
	cv_obj_attribute_type	attribType;		/* type of attribute */
	uint16_t				attribLen;		/* length of attribute in bytes */
} cv_attrib_hdr;
typedef cv_attrib_hdr cv_obj_attributes;

typedef PACKED struct td_cv_auth_hdr {
	cv_object_authorizations	authType;		/* type of authorization */
	uint16_t			authLen;		/* length of authorization in bytes */
} cv_auth_hdr;


typedef PACKED struct td_cv_obj_attribs_hdr {
	uint16_t	attribsLen;	/* length of object attributes in bytes */
	cv_attrib_hdr attrib;	/* header for first attribute in list */
} cv_obj_attribs_hdr;

typedef PACKED struct td_cv_obj_auth_lists_hdr {
	uint16_t	authListsLen;	/* length of object auth lists in bytes */
	cv_auth_list_hdr authList;	/* header for first auth list in list */
} cv_obj_auth_lists_hdr;

typedef PACKED struct td_cv_obj_hdr {
	cv_obj_type			objType;	/* type of object */
	uint16_t			objLen;		/* length of object in bytes, excluding objType and objLen */
} cv_obj_hdr;

typedef PACKED struct td_cv_auth_entry_passphrase {
	uint8_t passphraseLen;		/* length of passphrase */
	uint8_t passphrase[1];		/* passphrase byte string */
} cv_auth_entry_passphrase;


typedef PACKED struct td_cv_auth_entry_passphrase_handle {
	uint8_t			constZero;		/* value: 0 */
	cv_obj_handle	hPassphrase;	/* handle to passphrase object */
} cv_auth_entry_passphrase_handle;

typedef PACKED struct td_cv_auth_entry_passphrase_HMAC {
	uint8_t HMACLen;	/* length of HMAC; 20 bytes for SHA1 and 32 bytes for SHA-256 */
	uint8_t HMAC[1];	/* HMAC */
} cv_auth_entry_passphrase_HMAC;


typedef PACKED struct td_cv_auth_entry_hotp {
	cv_obj_handle	hHOTPObj;	/* handle for HOTP object */
} cv_auth_entry_hotp;

typedef PACKED struct td_cv_auth_entry_auth_session {
	cv_obj_handle	hAuthSessionObj;	/* handle for auth session object */
} cv_auth_entry_auth_session;

typedef PACKED struct td_cv_auth_data {
	uint8_t	authData[20];	/* authorization data */
} cv_auth_data;

typedef PACKED struct td_cv_nonce {
	uint8_t	nonce[20];	/* nonce */
} cv_nonce;


typedef PACKED struct td_cv_otp_nonce {
	uint8_t	nonce[AES_128_BLOCK_LEN];	/* nonce */
} cv_otp_nonce;

typedef PACKED struct td_cv_time {
	uint8_t	time[7];	/* time in hex: YYYYMMDDHHMMSS (eg. 0x20,0x05,0x12,0x14,0x21,0x10,0x00 for  2005 Dec 14 21:10:00) */
} cv_time;

typedef PACKED struct td_cv_auth_entry_count {
	uint32_t	count;	/* count of times object usage allowed */
} cv_auth_entry_count;

typedef PACKED struct td_cv_auth_entry_PIN {
	uint8_t PINLen;	/* length of PIN */
	uint8_t PIN[1];	/* PIN byte string */
} cv_auth_entry_PIN;

typedef PACKED struct td_cv_obj_value_certificate {
	uint16_t	certLen;	/* length of certificate */
	uint8_t	cert[1];		/* certificate */
} cv_obj_value_certificate;

typedef PACKED struct td_cv_obj_value_opaque {
	uint16_t	valueLen;	/* value length in bytes*/
	uint8_t	value[1];		/* value */
} cv_obj_value_opaque;

typedef PACKED struct td_cv_obj_value_passphrase {
	uint16_t	passphraseLen;	/* value length in bytes*/
	uint8_t	passphrase[1];		/* value */
} cv_obj_value_passphrase;

typedef PACKED struct td_cv_obj_value_hash {
	cv_hash_type	hashType;	/* cv_hash_type */
	uint8_t			hash[1];	/* hash */
} cv_obj_value_hash;

typedef PACKED struct td_cv_obj_value_hotp {
	uint32_t version; /* Set this to 1 */
	uint32_t currentCounterValue; /* Set this to the desired counter start value at object creation time */
	uint8_t counterInterval; /* Set this to 1 or to another small number. The counter will be increased by this number each time */
	uint8_t	randomKey[HOTP_KEY_MAX_LEN];
	uint8_t	randomKeyLength; /* At least 16. 20 is recommended. */
	uint32_t allowedMaxWindow;
	uint32_t maxTries;
	uint32_t allowedLargeMaxWindow; /* Two subsequent OTPs are required in this window. It is also used for unblock */
	uint8_t	numberOfDigits; /* 6, 8 or 10*/
	cv_bool	isBlocked; /* Set this to FALSE at object creation time */
	uint32_t currentNumberOfTries; /* Set this to 0 at creation time */
	uint32_t secondSubsequentOTPLocationInWindow; /* Set this to 0 at creation time. When set to 0 - a second OTP is not required */
} cv_obj_value_hotp;

typedef PACKED struct td_cv_obj_value_aes_key {
	uint16_t	keyLen;	/* key size in bits */
	uint8_t	key[1];		/* key */
} cv_obj_value_aes_key;

typedef PACKED struct td_cv_obj_value_key {
	uint16_t	keyLen;	/* key size in bits */
	uint8_t	key[1];		/* key */
} cv_obj_value_key;

typedef PACKED struct td_cv_obj_value_oath_token {
	cv_otp_token_type type;				/* token type (RSA or OATH) */
	uint32_t countValue;				/* count value */
	uint8_t clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t sharedSecretLen;			/* length of shared secret */
	uint8_t	sharedSecret[CV_NONCE_LEN];		/* sharedSecret */
} cv_obj_value_oath_token;

typedef PACKED struct td_cv_obj_value_auth_session {
	cv_obj_auth_flags		statusLSH;	/* status of auth session */
	cv_auth_session_status	statusMSH;	/* status of auth session */
	cv_auth_session_flags	flags;		/* flags indicating usage of auth session */
	cv_auth_data			authData;	/* auth data to allow usage of this auth session */
	/* shared secret included if indicated by flags */
} cv_obj_value_auth_session;

typedef PACKED struct td_cv_app_id {
	uint8_t	appIDLen;	/* length of application ID */
	uint8_t	appID[1];	/* application ID byte string */
} cv_app_id;

typedef PACKED struct td_cv_user_id {
	uint8_t	userIDLen;	/* length of application ID */
	uint8_t	userID[1];	/* application ID byte string */
} cv_user_id;

typedef PACKED struct td_cv_version {
	uint16_t	versionMajor;	/* major version of CV */
	uint16_t	versionMinor;	/* minor version of CV */
} cv_version;

typedef PACKED struct td_cv_space {
	uint32_t	totalVolatileSpace;			/* total bytes of volatile space */
	uint32_t	totalNonvolatileSpace;		/* total bytes of non-volatile space */
	uint32_t	remainingVolatileSpace;		/* remaining bytes of volatile space) */
	uint32_t	remainingNonvolatileSpace;	/* remaining bytes of non-volatile space */
} cv_space;

typedef PACKED struct td_cv_time_params {
	cv_time	localTime;
	cv_time	GMTTime;
} cv_time_params;

typedef	PACKED struct td_cv_st_challenge {
	uint32_t rowNumber;
	uint8_t nonce[CV_NONCE_LEN];
} cv_st_challenge;

typedef	PACKED struct td_cv_st_hash_buff {
	uint32_t crc32;
	uint8_t nonce[CV_NONCE_LEN];
} cv_st_hash_buff;

typedef struct td_cv_super_challenge {
	uint8_t		RSN[CV_SUPER_RSN_LEN];	// recovery serial number
	uint32_t	ctr;        			// counter
	uint8_t 	encCtr[4];       			// encrypted counter
} cv_super_challenge;

typedef cv_status cv_callback (uint32_t status, uint32_t extraDataLength,
							void * extraData, void  *arg);
typedef void * cv_callback_ctx;



typedef PACKED struct td_cv_da_params {
        uint32_t        maxFailedAttempts;                      /* maximum failed attempts before lockout */
        uint32_t        lockoutDuration;                        /* duration in seconds of initial lockout period */
} cv_da_params;

#define MAX_USB_DEVICES     8
#define GET_START_BITPOS(a32t)  (((a32t) & 0xFC000000) >> 26) /* count from 0-31, inclusive */
#define GET_END_BITPOS(a32t)    (((a32t) & 0x03F00000) >> 20) /* count from 1-32: start=0 end=1 means only change bit0 */
#define GET_SPEED(a32t)         (((a32t) & 0x000F0000) >> 16)
#define GET_PROTOCOL(a32t)      (((a32t) & 0x0000F000) >> 12)
#define GET_REGISTER(a32t)      (((a32t) & 0x00000FFF) | RFID_REG_PREFIX)

#define RFID_CONFIG_ITEM_VALID   0x08000
#define RFID_CONFIG_ITEM_VALID32 0x80000000 /* for 32 bit memebers */

#ifdef CVUSERLIBIFC_EXPORTS
#define CVUSERLIBIFC_API __declspec(dllexport)
#else
#define CVUSERLIBIFC_API
#endif

/*
 * Utility Routines
 */

CVUSERLIBIFC_API cv_status
cv_open (cv_options options, cv_app_id *appID, cv_user_id *userID,
			cv_obj_value_passphrase *objAuth, cv_handle *pCvHandle);

CVUSERLIBIFC_API cv_status
cv_close (cv_handle cvHandle);

CVUSERLIBIFC_API cv_status
cv_get_session_count (cv_handle cvHandle, uint32_t *sessionCount);

CVUSERLIBIFC_API cv_status
cv_get_status (cv_handle cvHandle, cv_status_type statusType,
			   void *pStatus);
CVUSERLIBIFC_API cv_status
cv_get_ush_ver (	cv_handle cvHandle,
					uint32_t bufLen,
					uint8_t *pBuf);

CVUSERLIBIFC_API cv_status
cv_flash_update (	cv_handle cvHandle,
					char *flashPath,
					uint32_t offset);

CVUSERLIBIFC_API cv_status
cv_reboot_to_sbi( void );

CVUSERLIBIFC_API cv_status
cv_reboot_to_ush( void );

CVUSERLIBIFC_API cv_status
cv_firmware_upgrade ( cv_handle cvHandle,
					  char *fwPath,
					  uint32_t offset,
					  uint8_t progress);

CVUSERLIBIFC_API cv_status
cv_load_sbi (		char *sbiPath);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif				       /* end _CVAPI_H_ */

