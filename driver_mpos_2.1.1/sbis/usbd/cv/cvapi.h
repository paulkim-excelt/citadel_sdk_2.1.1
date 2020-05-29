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
 * cvapi.h:  Credential Vault function prototypes and structures
 */
/*
 * Revision History:
 *
 * 01/02/07 DPA Created.
*/

#ifndef _CVAPI_H_
#define _CVAPI_H_

#include "platform.h"

#ifdef _WINDOWS
#define PACKED
#else
#define PACKED __packed
#endif
#undef PACKED
#define PACKED

/*
 * Enumerated values and types
 */
typedef unsigned char byte;
typedef unsigned char cv_bool;

typedef uint32_t cv_handle;

typedef uint32_t cv_obj_handle;

typedef unsigned int cv_options;
#define CV_SECURE_SESSION		0x00000001
#define CV_USE_SUITE_B			0x00000002
#define CV_SYNCHRONOUS			0x00000004
#define CV_SUPPRESS_UI_PROMPTS		0x00000008
#define CV_INTERMEDIATE_CALLBACKS	0x00000010
#define CV_FP_INTERMEDIATE_CALLBACKS	0x00000020
#define CV_OPTIONS_MASK (CV_SECURE_SESSION | CV_USE_SUITE_B | CV_SYNCHRONOUS | CV_SUPPRESS_UI_PROMPTS | CV_INTERMEDIATE_CALLBACKS | CV_FP_INTERMEDIATE_CALLBACKS)

typedef unsigned int cv_fp_control;
#define CV_UPDATE_TEMPLATE		0x00000001
#define CV_INVALIDATE_CAPTURE	0x00000002
#define CV_HINT_AVAILABLE		0x00000004
#define CV_USE_FAR				0x00000008
#define CV_USE_FEATURE_SET		0x00000010
#define CV_USE_FEATURE_SET_HANDLE 0x00000020
/* this mask defines the only bits valid for input parameter */
#define CV_FP_CONTROL_MASK (CV_INVALIDATE_CAPTURE | CV_USE_FAR | CV_USE_FEATURE_SET | CV_USE_FEATURE_SET)

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
#define CV_OBJ_AUTH_DELETE		0x00000010
#define CV_OBJ_AUTH_ACCESS		0x00000020
#define CV_OBJ_AUTH_CHANGE_AUTH		0x00000040
#define CV_ADMINISTRATOR_AUTH		0x00008000
#define CV_OBJ_AUTH_FLAGS_MASK (CV_OBJ_AUTH_READ_PUB | CV_OBJ_AUTH_READ_PRIV | CV_OBJ_AUTH_WRITE_PUB | CV_OBJ_AUTH_WRITE_PRIV \
								| CV_OBJ_AUTH_DELETE | CV_OBJ_AUTH_ACCESS | CV_OBJ_AUTH_CHANGE_AUTH)

typedef unsigned short cv_auth_session_status;
#define CV_AUTH_SESSION_TERMINATED	0x00000001

typedef unsigned int cv_auth_session_flags;
#define CV_AUTH_SESSION_CONTAINS_SHARED_SECRET	0x00000001
#define CV_AUTH_SESSION_PER_USAGE_AUTH		0x00000002
#define CV_AUTH_SESSION_SINGLE_USE		0x00000004
#define CV_AUTH_SESSION_FLAGS_MASK (CV_AUTH_SESSION_CONTAINS_SHARED_SECRET | CV_AUTH_SESSION_PER_USAGE_AUTH | CV_AUTH_SESSION_SINGLE_USE)

typedef unsigned int cv_attrib_flags;
#define CV_ATTRIB_PERSISTENT			0x00000001
#define CV_ATTRIB_NON_HASHABLE			0x00000002
#define CV_ATTRIB_NVRAM_STORAGE			0x00000004
#define CV_ATTRIB_SC_STORAGE			0x00000008
#define CV_ATTRIB_CONTACTLESS_STORAGE		0x00000010
#define CV_ATTRIB_USE_FOR_SIGNING		0x00010000
#define CV_ATTRIB_USE_FOR_ENCRYPTION		0x00020000
#define CV_ATTRIB_USE_FOR_DECRYPTION		0x00040000
#define CV_ATTRIB_USE_FOR_KEY_DERIVATION	0x00080000
#define CV_ATTRIB_USE_FOR_WRAPPING		0x00100000
#define CV_ATTRIB_USE_FOR_UNWRAPPING		0x00200000

#define CV_MAJOR_VERSION 1
#define CV_MINOR_VERSION 0

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

#define GENERAL_PROMPT_RETURN_CODE_MASK 0x00010000
#define SC_PROMPT_RETURN_CODE_MASK		0x00020000
#define FP_PROMPT_RETURN_CODE_MASK		0x00040000
#define CV_USER_LIB_RETURN_CODE_MASK	0x00080000
#define CV_USER_INTERFACE_RETURN_CODE_MASK	0x00100000
typedef unsigned int cv_status;
enum cv_status_e {
		CV_SUCCESS,								// 0x00000000
		CV_SUCCESS_SUBMISSION,					// 0x00000001
		CV_READ_HID_CREDENTIAL = 4,				// 0x00000004
		CV_INVALID_HANDLE = 6,					// 0x00000006
		CV_OBJECT_NOT_VALID,					// 0x00000007
		CV_AUTH_FAIL,							// 0x00000008
		CV_OBJECT_WRITE_REQUIRED,				// 0x00000009
		CV_IN_LOCKOUT,							// 0x0000000A
		CV_INVALID_VERSION = 0xC,				// 0x0000000C
		CV_PARAM_BLOB_INVALID_LENGTH,			// 0x0000000D
		CV_PARAM_BLOB_INVALID,					// 0x0000000E
		CV_PARAM_BLOB_AUTH_FAIL,				// 0x0000000F
		CV_PARAM_BLOB_DECRYPTION_FAIL,			// 0x00000010
		CV_PARAM_BLOB_ENCRYPTION_FAIL,			// 0x00000011
		CV_PARAM_BLOB_RNG_FAIL,					// 0x00000012
		CV_SECURE_SESSION_SUITE_B_REQUIRED,		// 0x00000013
		CV_SECURE_SESSION_KEY_GENERATION_FAIL,	// 0x00000014
		CV_SECURE_SESSION_KEY_HASH_FAIL,		// 0x00000015
		CV_SECURE_SESSION_KEY_SIGNATURE_FAIL,	// 0x00000016
		CV_VOLATILE_MEMORY_ALLOCATION_FAIL,		// 0x00000017
		CV_SECURE_SESSION_BAD_PARAMETERS,		// 0x00000018
		CV_SECURE_SESSION_SHARED_SECRET_FAIL,	// 0x00000019
		CV_CALLBACK_TIMEOUT,					// 0x0000001A
		CV_INVALID_OBJECT_HANDLE,				// 0x0000001B
		CV_HOST_STORAGE_REQUEST_TIMEOUT,		// 0x0000001C
		CV_HOST_STORAGE_REQUEST_RESULT_INVALID,	// 0x0000001D
		CV_OBJ_AUTH_FAIL,						// 0x0000001E
		CV_OBJ_DECRYPTION_FAIL,					// 0x0000001F
		CV_OBJ_ANTIREPLAY_FAIL,					// 0x00000020
		CV_OBJ_VALIDATION_FAIL,					// 0x00000021
		CV_OBJECT_ATTRIBUTES_INVALID = 0x24,	// 0x00000024
		CV_NO_PERSISTENT_OBJECT_ENTRY_AVAIL,	// 0x00000025
		CV_OBJECT_DIRECTORY_FULL,				// 0x00000026
		CV_NO_VOLATILE_OBJECT_ENTRY_AVAIL,		// 0x00000027
		CV_FLASH_MEMORY_ALLOCATION_FAIL,		// 0x00000028
		CV_ENUMERATION_BUFFER_FULL,				// 0x00000029
		CV_ADMIN_AUTH_NOT_ALLOWED,				// 0x0000002A
		CV_ADMIN_AUTH_REQUIRED,					// 0x0000002B
		CV_CORRESPONDING_AUTH_NOT_FOUND_IN_OBJECT,	// 0x0000002C
		CV_INVALID_AUTH_LIST,					// 0x0000002D
		CV_UNSUPPORTED_CRYPT_FUNCTION,			// 0x0000002E
		CV_CANCELLED_BY_USER,					// 0x0000002F
		CV_POLICY_DISALLOWS_SESSION_AUTH,		// 0x00000030
		CV_CRYPTO_FAILURE,						// 0x00000031
		CV_RNG_FAILURE, 						// 0x00000032
		CV_INVALID_OUTPUT_PARAMETER_LENGTH = 0x34, 		// 0x00000034
		CV_INVALID_OBJECT_SIZE,					// 0x00000035
		CV_INVALID_GCK_KEY_LENGTH = 0x37,		// 0x00000037
		CV_INVALID_DA_PARAMS,					// 0x00000038
		CV_CV_NOT_EMPTY,						// 0x00000039
		CV_NO_GCK,								// 0x0000003A
		CV_RTC_FAILURE,							// 0x0000003B
		CV_INVALID_KDIX_KEY,					// 0x0000003C
		CV_INVALID_KEY_TYPE,					// 0x0000003D
		CV_INVALID_KEY_LENGTH,					// 0x0000003E
		CV_KEY_GENERATION_FAILURE,				// 0x0000003F
		CV_INVALID_KEY_PARAMETERS,				// 0x00000040
		CV_INVALID_OBJECT_TYPE,					// 0x00000041
		CV_INVALID_ENCRYPTION_PARAMETER,		// 0x00000042
		CV_INVALID_HMAC_KEY,					// 0x00000043
		CV_INVALID_INPUT_PARAMETER_LENGTH, 		// 0x00000044
		CV_INVALID_BULK_ENCRYPTION_PARAMETER,	// 0x00000045
		CV_ENCRYPTION_FAILURE,					// 0x00000046
		CV_INVALID_INPUT_PARAMETER,				// 0x00000047
		CV_SIGNATURE_FAILURE,					// 0x00000048
		CV_INVALID_OBJECT_PERSISTENCE,			// 0x00000049
		CV_OBJECT_NONHASHABLE,					// 0x0000004A
		CV_SIGNED_TIME_REQUIRED,				// 0x0000004B
		CV_INVALID_SIGNATURE,					// 0x0000004C
		CV_INTERNAL_DEVICE_FAILURE,				// 0x0000004D
		CV_FLASH_ACCESS_FAILURE,				// 0x0000004E
		CV_RTC_NOT_AVAILABLE,					// 0x0000004F
        	CV_USH_BOOT_FAILURE,                // 0x00000050
        	CV_INVALID_FINGERPRINT_TYPE,        // 0x00000051
		CV_FAR_PARAMETER_DISALLOWED,			// 0x00000052
		CV_FAR_VALUE_NOT_CONFIGURED,			// 0x00000053
		CV_FINGERPRINT_CAPTURE_FAILED,			// 0x00000054
		CV_HOST_CONTROL_REQUEST_TIMEOUT,		// 0x00000055
		CV_HOST_CONTROL_REQUEST_RESULT_INVALID,	// 0x00000056
		CV_INVALID_COMMAND,						// 0x00000057
		CV_INVALID_COMMAND_LENGTH,				// 0x00000058
		CV_FP_MATCH_GENERAL_ERROR,				// 0x00000059
		CV_FP_DEVICE_GENERAL_ERROR,				// 0x0000005A
		CV_NO_BIOS_SHARED_SECRET,				// 0x0000005B
		CV_INVALID_HASH_TYPE,					// 0x0000005C
		CV_IDENTIFY_USER_FAILED,				// 0x0000005D
		CV_CONTACTLESS_FAILURE,					// 0x0000005E
		CV_INVALID_CONTACTLESS_CARD_TYPE,		// 0x0000005F
		CV_IDENTIFY_USER_TIMEOUT,				// 0x00000060
		CV_INVALID_IMPORT_BLOB,					// 0x00000061
		CV_CANT_REMAP_HANDLES,					// 0x00000062
		CV_OBJECT_REQUIRES_PRIVACY_KEY,			// 0x00000063
		CV_SMART_CARD_FAILURE,					// 0x00000064
		CV_INVALID_SMART_CARD_TYPE,				// 0x00000065
		CV_SUPER_AUTH_TYPE_NOT_CONFIGURED,		// 0x00000066
		CV_DIAG_FAIL,                       	// 0x00000067
		CV_MONOTONIC_COUNTER_FAIL,				// 0x00000068
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
		CV_INVALID_SESSION,
		CV_FAILED_OPEN_REMOTE_SESSION,
		CV_FAILED_CLOSE_REMOTE_SESSION,

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
		CV_HOST_CONTROL_REQUEST_SUBMISSION_FAILED
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
		CV_CCMP
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
		CV_STATUS_DEVICES_ENABLED
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
		CV_CONFIG_SUPER
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

typedef unsigned short cv_fingerprint_type;
enum cv_fingerprint_type_e {
		CV_FINGERPRINT_FEATURE_SET,
		CV_FINGERPRINT_TEMPLATE
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
		CV_TYPE_AUTH_SESSION
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
		CV_AUTH_SUPER

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
enum cv_fp_event_e {
		CV_FP_EVENT_FP_AVAIL
};

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
	uint16_t					authLen;		/* length of authorization in bytes */
} cv_auth_hdr;

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

typedef PACKED struct td_cv_auth_entry_fingerprint_obj {
	uint8_t			numTemplates;	/* number of templates */
	cv_obj_handle	templates[1];	/* list of template handles */
} cv_auth_entry_fingerprint_obj;

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

typedef PACKED struct td_cv_auth_entry_auth_session_HMAC {
	cv_obj_handle	hAuthSessionObj;	/* handle for auth session object */
	uint8_t			HMACLen;			/* length of HMAC; 20 bytes for SHA1 and 32 bytes for SHA-256 */
	uint8_t			HMAC[1];			/* HMAC */
} cv_auth_entry_auth_session_HMAC;

typedef PACKED struct td_cv_auth_entry_count {
	uint32_t	count;	/* count of times object usage allowed */
} cv_auth_entry_count;

typedef PACKED struct td_cv_auth_entry_time {
	cv_time	startTime;	/* start time for authorization */
	cv_time	endTime;	/* end time for authorization */
} cv_auth_entry_time;

typedef PACKED struct td_cv_auth_entry_contactless {
	cv_contactless_type	type;	/* contactless credential type */
	uint16_t	length;			/* length of contactless credential */
	uint8_t		credential[1];	/* contactless credential */
} cv_auth_entry_contactless;

typedef PACKED struct td_cv_auth_entry_contactless_handle {
	cv_contactless_type	type;	/* contactless credential type */
	uint16_t	constZero;		/* value: 0 */
	cv_obj_handle	hHIDObj;	/* handle to object containing HID credential */
} cv_auth_entry_contactless_handle;

typedef PACKED struct td_cv_auth_entry_smartcard_handle {
	uint16_t	constZero;		/* value: 0 */
	cv_obj_handle	keyObj;		/* handle to key object */
} cv_auth_entry_smartcard_handle;

typedef PACKED struct td_cv_auth_entry_challenge {
	cv_nonce	nonce;			/* nonce */
	uint8_t		*signature;		/* signature */
} cv_auth_entry_challenge;

typedef PACKED struct td_cv_auth_entry_challenge_handle_obj {
	uint16_t	constZero;		/* value: 0 */
	cv_obj_handle	keyObj;		/* handle to key object */
} cv_auth_entry_challenge_handle_obj;

typedef PACKED struct td_cv_auth_entry_PIN {
	uint8_t PINLen;	/* length of PIN */
	uint8_t PIN[1];	/* PIN byte string */
} cv_auth_entry_PIN;

typedef PACKED struct td_cv_obj_value_certificate {
	uint16_t	certLen;	/* length of certificate */
	uint8_t	cert[1];		/* certificate */
} cv_obj_value_certificate;

typedef PACKED struct td_cv_obj_value_fingerprint {
	cv_fingerprint_type	type;	/* cv_fingerprint_type */
	uint16_t			fpLen;	/* fingerprint length */
	uint8_t				fp[1];	/* fingerprint */
} cv_obj_value_fingerprint;

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

typedef PACKED struct td_cv_auth_entry_smartcard {
	cv_key_type	type;			/* key type */
	cv_obj_value_key key;		/* key */
} cv_auth_entry_smartcard;

typedef PACKED struct td_cv_auth_entry_challenge_obj {
	cv_key_type	type;			/* key type */
	cv_obj_value_key key;		/* key */
} cv_auth_entry_challenge_obj;

typedef PACKED struct td_cv_auth_entry_proprietary_sc {
	cv_obj_value_hash	hash;	/* SHA1(user key || public data field) */
} cv_auth_entry_proprietary_sc;

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

typedef PACKED struct td_cv_bulk_params {
	cv_bulk_mode		bulkMode;		/* bulk mode */
	cv_auth_algorithm	authAlgorithm;	/* algorithm for authorization */
	cv_obj_handle		hAuthKey;		/* handle to authorization key object */
	uint16_t			authOffset;		/* offset to start of authorization */
	uint16_t			encOffset;		/* offset to start of encryption */
	uint16_t			IVLen;			/* length of IV */
	uint8_t				IV[AES_256_IV_LEN];	/* pointer to IV byte string */
	cv_obj_handle		hHashObj;		/* pointer to output hash object */
} cv_bulk_params;

typedef PACKED struct tdotp_rsa_alg_params {
	uint8_t serialNumber[4];
	uint8_t displayDigits;
	uint8_t displayIntervalSeconds;
} tpm_otp_rsa_alg_params;

typedef PACKED struct td_cv_obj_value_rsa_token {
	cv_otp_token_type type;				/* token type (RSA or OATH) */
	tpm_otp_rsa_alg_params rsaParams;	/* RSA parameters */
	uint8_t clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t sharedSecretLen;			/* length of shared secret */
	uint8_t	sharedSecret[AES_128_BLOCK_LEN];			/* sharedSecret */
} cv_obj_value_rsa_token;

typedef PACKED struct td_cv_otp_rsa_keygen_params_direct {
	cv_otp_provisioning_type	otpProvType;	/* provisioning type, direct or CT-KIP */
	tpm_otp_rsa_alg_params		rsaParams;		/* RSA params */
	uint8_t						masterSeed[AES_128_BLOCK_LEN];	/* master seed, for direct provisioning */
} cv_otp_rsa_keygen_params_direct;

typedef PACKED struct td_cv_otp_rsa_keygen_params_ctkip {
	cv_otp_provisioning_type	otpProvType;	/* provisioning type, direct or CT-KIP */
	tpm_otp_rsa_alg_params		rsaParams;		/* RSA params */
	uint8_t						clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t						serverNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint32_t					serverPubkeyLen; /* length of server public key */
	uint8_t						serverPubkey[1]; /* server public key */
} cv_otp_rsa_keygen_params_ctkip;

typedef PACKED struct td_cv_otp_oath_keygen_params {
	uint8_t						clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t						serverNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint32_t					serverPubkeyLen; /* length of server public key */
	uint8_t						serverPubkey[1]; /* server public key */
} cv_otp_oath_keygen_params;

typedef PACKED struct td_cv_space {
	uint32_t	totalVolatileSpace;			/* total bytes of volatile space */
	uint32_t	totalNonvolatileSpace;		/* total bytes of non-volatile space */
	uint32_t	remainingVolatileSpace;		/* remaining bytes of volatile space) */
	uint32_t	remainingNonvolatileSpace;	/* remaining bytes of non-volatile space */
} cv_space;

typedef PACKED struct td_cv_da_params {
	uint32_t	maxFailedAttempts;			/* maximum failed attempts before lockout */
	uint32_t	lockoutDuration;			/* duration in seconds of initial lockout period */
} cv_da_params;

typedef PACKED struct td_cv_time_params {
	cv_time	localTime;
	cv_time	GMTTime;
} cv_time_params;

typedef	struct td_cv_super_config {
	uint8_t Z[CV_SUPER_LENGTH];
	uint8_t P[CV_SUPER_LENGTH];
	uint8_t X[CV_SUPER_LENGTH];
} cv_super_config;

typedef	struct td_cv_super_challenge {
	uint8_t Q[CV_SUPER_LENGTH];
	uint8_t S[CV_SUPER_LENGTH];
} cv_super_challenge;

typedef unsigned int cv_device_enabled_status;
enum cv_device_enabled_status_e {
	CV_RADIO_ENABLED = 0x00000001,			/* radio hardware enabled */
	CV_SMARTCARD_ENABLED = 0x00000002,		/* smartcard hardware enabled */
	CV_FINGERPRINT_ENABLED = 0x00000004		/* fingerprint hardware enabled */
};

typedef struct td_cv_contactless_identifier {
	cv_contactless_type	type;	/* type of contactless card */
	uint8_t				id[1];	/* identifier */
} cv_contactless_identifier;

typedef cv_status cv_callback (uint32_t status, uint32_t extraDataLength,
							void * extraData, void  *arg);
typedef void * cv_callback_ctx;
/*
 * Utility Routines
 */

cv_status
cv_open (cv_options options, cv_app_id *appID, cv_user_id *userID,
			cv_obj_value_passphrase *objAuth, cv_handle *pCvHandle);
cv_status
cv_open_remote (cv_options options, cv_app_id *appID, cv_user_id *userID,
			cv_obj_value_passphrase *objAuth, cv_handle *pCvHandle,
			uint32_t IPAddrLen, char *pIPAddr);

cv_status
cv_close (cv_handle cvHandle);

cv_status
cv_get_session_count (cv_handle cvHandle, uint32_t *sessionCount);

cv_status
cv_init (cv_handle cvHandle, cv_bool clearObjects, cv_bool cryptoSuiteB,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_obj_value_aes_key *pGCK,
			uint32_t newAuthListsLength, cv_auth_list *pNewAuthLists,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_get_status (cv_handle cvHandle, cv_status_type statusType,
			   void *pStatus);

cv_status
cv_set_config (cv_handle cvHandle, uint32_t authListLength,
				cv_auth_list *pAuthList, cv_config_type configType,
				void *pConfig,
				cv_callback *callback, cv_callback_ctx context);

/*
 * Object Management Routines
 */

cv_status
cv_create_object (cv_handle cvHandle, cv_obj_type objType,
				  uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
				  uint32_t authListsLength, cv_auth_list *pAuthLists,
				  uint32_t objValueLength, void *pObjValue,
				  cv_obj_handle *pObjHandle,
				  cv_callback *callback, cv_callback_ctx context);

cv_status
cv_delete_object (cv_handle cvHandle, cv_obj_handle objHandle,
					uint32_t authListLength, cv_auth_list *pAuthList,
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_set_object (cv_handle cvHandle, cv_obj_handle objHandle,
				uint32_t authListLength, cv_auth_list *pAuthList,
				uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
				uint32_t objValueLength, void *pObjValue,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cv_get_object (cv_handle cvHandle, cv_obj_handle objHandle,
				uint32_t authListLength, cv_auth_list *pAuthList,
				cv_obj_hdr *objHeader,
				uint32_t *pAuthListsLength, cv_auth_list *pAuthLists,
				uint32_t *pObjAttributesLength, cv_obj_attributes *pObjAttributes,
				uint32_t *pObjValueLength, void *pObjValue,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cv_enumerate_objects (cv_handle cvHandle, cv_obj_type objType,
					  uint32_t *pObjHandleListLength, cv_obj_handle *pObjHandleList);

cv_status
cv_duplicate_object (cv_handle cvHandle, cv_obj_handle objHandle,
						uint32_t authListLength, cv_auth_list *pAuthList,
						cv_obj_handle *pNewObjHandle, cv_persistence persistenceAction,
						cv_callback *callback, cv_callback_ctx context);

cv_status
cv_import_object (cv_handle cvHandle, cv_blob_type blobType,
					uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
					uint32_t authListsLength, cv_auth_list *pAuthLists,
					uint32_t objValueLength, void *pObjValue,
					cv_dec_op decryptionOperation,
					cv_bulk_params *pBulkParameters,
					uint32_t importBlobLength, void *pImportBlob,
					uint32_t authListLength, cv_auth_list *pAuthList,
					cv_obj_handle unwrappingObjectHandle,
					uint32_t embeddedHandlesLength,
					cv_obj_handle *pEmbeddedHandlesMap,
					cv_obj_handle *pNewObjHandle,
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_export_object (cv_handle cvHandle, cv_blob_type blobType,
					byte blobLabel[16], cv_obj_handle objectHandle,
					uint32_t authListLength, cv_auth_list *pAuthList,
					cv_enc_op encryptionOperation,
					cv_bulk_params *pBulkParameters,
					cv_obj_handle wrappingObjectHandle,
					uint32_t *pExportBlobLength, void *pExportBlob,
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_change_auth_object (cv_handle cvHandle, cv_obj_handle objHandle,
						uint32_t curAuthListLength, cv_auth_list *pCurAuthList,
						uint32_t newAuthListsLength, cv_auth_list *pNewAuthLists,
						cv_callback *callback, cv_callback_ctx context);

cv_status
cv_authorize_session_object (cv_handle cvHandle,
								cv_obj_handle authSessObjHandle,
								uint32_t authListLength, cv_auth_list *pAuthList,
								cv_callback *callback, cv_callback_ctx context);

cv_status
cv_deauthorize_session_object (cv_handle cvHandle, cv_obj_handle authSessObjHandle, cv_obj_auth_flags flags);

/*
 * Cryptographic Routines
 */

cv_status
cv_get_random (cv_handle cvHandle, uint32_t randLen, byte *pRandom, cv_callback *callback, cv_callback_ctx context);

cv_status
cv_encrypt (cv_handle cvHandle, cv_obj_handle encKeyHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_enc_op encryptionOperation,
			uint32_t cleartextLength, byte *pCleartext,
			cv_bulk_params *pBulkParameters,
			uint32_t *pEncLength, byte *pEncData,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_decrypt (cv_handle cvHandle, cv_obj_handle decKeyHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_dec_op decryptionOperation,
			uint32_t encLength, byte * pEncData,
			cv_bulk_params *pBulkParameters,
			uint32_t *pCleartextLength, byte *pCleartext,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_hmac (cv_handle cvHandle, cv_obj_handle hmacKeyHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_hmac_op hmacOperation,
			uint32_t dataLength, byte *pData, cv_obj_handle hmacHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_hash (cv_handle cvHandle, cv_hash_op hashOperation,
		 uint32_t dataLength, byte *pData, cv_obj_handle hashHandle);

cv_status
cv_hash_object (cv_handle cvHandle, cv_hash_op hashOperation,
				cv_obj_handle objectHandle, cv_obj_handle hashHandle);

cv_status
cv_genkey (cv_handle cvHandle, cv_obj_type objType, cv_key_type keyType, uint32_t keyLength,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t extraParamsLength,    void *pExtraParams,
			cv_obj_handle *pKeyObjHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_sign (cv_handle cvHandle, cv_obj_handle signKeyHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_sign_op signOperation,
			cv_obj_handle hashHandle,
			uint32_t *pSignatureLength, byte *pSignature,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_verify (cv_handle cvHandle, cv_obj_handle verifyKeyHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_verify_op verifyOperation,
			cv_obj_handle hashHandle,
			uint32_t signatureLength, byte *pSignature, cv_bool *pVerify,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_otp_get_value (cv_handle cvHandle, cv_obj_handle otpTokenHandle,
					uint32_t authListLength, cv_auth_list *pAuthList,
					cv_time *pTime, uint32_t signatureLength, byte *pSignature,
					cv_obj_handle verifyKeyHandle,
					uint32_t *pOtpValueLength, byte *pOtpValue,
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_otp_get_mac (cv_handle cvHandle, cv_obj_handle otpTokenHandle,
				uint32_t authListLength, cv_auth_list *pAuthList,
				cv_otp_mac_type macType,
				cv_otp_nonce *serverRandom, cv_otp_nonce *pMac,
				cv_callback *callback, cv_callback_ctx context);

cv_status
cv_unwrap_dlies_key (cv_handle cvHandle, uint32_t numKeys,
						cv_obj_handle *keyHandleList,
						uint32_t authListLength, cv_auth_list *pAuthList,
						uint32_t messageLength, byte *pMessage,
						cv_obj_handle newKeyHandle, cv_bool *pbUserKeyFound,
						cv_callback *callback, cv_callback_ctx context);

/*
 * Device Control Routines
 */

cv_status
cv_fingerprint_enroll (cv_handle cvHandle,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes,
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pTemplateLength, uint8_t *pTemplate,
			cv_obj_handle *pTemplateHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_verify (cv_handle cvHandle, cv_fp_control fpControl,
		    sint32_t FARval,
			uint32_t featureSetLength, byte *pFeatureSet,
			cv_obj_handle featureSetHandle,
			uint32_t templateLength, byte *pTemplate,
			cv_obj_handle templateHandle,
			cv_fp_result *pFpResult,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_capture (cv_handle cvHandle, cv_bool captureOnly,
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pFeatureSetLength, byte *pFeatureSet,
			cv_obj_handle *pFeatureSetHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_identify (cv_handle cvHandle, cv_fp_control fpControl,
			sint32_t FARval,
			cv_obj_handle featureSetHandle,
			uint32_t numTemplates, cv_obj_handle *pTemplateHandles,
			cv_fp_result *pFpResult,
			cv_obj_handle *pResultTemplateHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_register_event (cv_handle cvHandle, cv_fp_event fpEvent,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_configure (cv_handle cvHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			cv_fp_config_data_type cvFPConfigType,
			uint32_t offset, uint32_t *pDataLength, byte *pData,
			cv_bool read, cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_test (cv_handle cvHandle,
			uint32_t tests, uint32_t *pResults,
			uint32_t extraDataLength, byte *pExtraData);

cv_status
cv_fingerprint_calibrate(cv_handle cvHandle);

/*
 * Preboot Authentication Routines
 */

cv_status
cv_bind (cv_handle cvHandle, cv_obj_value_hash *sharedSecret);

cv_status
cv_bind_challenge (cv_handle cvHandle, cv_obj_value_hash *bindChallenge, cv_obj_value_hash *bindResponse);

cv_status
cv_identify_user (cv_handle cvHandle,
				uint32_t authListLength, cv_auth_list *pAuthList,
				uint32_t handleListLength, cv_obj_handle *handleList,
				cv_identify_type *pIdentifyType, uint32_t *pIdentifyLength,
				uint8_t *pIdentifyInfo);

cv_status
cv_access_PBA_code_flash (cv_handle cvHandle,
			uint32_t authListLength, cv_auth_list *pAuthList,
			uint32_t offset, uint32_t *pDataLength, uint8_t *pData,
			cv_bool read, cv_callback *callback, cv_callback_ctx context);


/*
 * Other Device Routines
 */

cv_status 
cv_read_contactless_card_id(cv_handle cvHandle, uint32_t authListLength, cv_auth_list *pAuthList, 
					cv_contactless_type *type, uint32_t *idLen, uint8_t *idValue, 
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_enable_radio(cv_handle cvHandle, cv_bool radioEnable);

cv_status
cv_enable_smartcard(cv_handle cvHandle, cv_bool scEnable);

cv_status
cv_enable_fingerprint(cv_handle cvHandle, cv_bool clEnable);

#endif				       /* end _CVAPI_H_ */
