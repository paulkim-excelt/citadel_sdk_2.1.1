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
#define _CVAPI_H_ 1
#include "phx.h"

#ifdef _WINDOWS
#define PACKED
#else
#define PACKED __packed
#endif

#include "usbd.h"
/* 
 * Enumerated values and types
 */
typedef unsigned char byte;
typedef unsigned char cv_bool;

typedef uint32_t cv_handle;

typedef uint32_t cv_obj_handle;

typedef unsigned int cv_options;
#define CV_SECURE_SESSION			0x00000001
#define CV_USE_SUITE_B				0x00000002
#define CV_SYNCHRONOUS				0x00000004
#define CV_SUPPRESS_UI_PROMPTS		0x00000008
#define CV_INTERMEDIATE_CALLBACKS	0x00000010

typedef unsigned int cv_fp_control;
#define CV_UPDATE_TEMPLATE			0x00000001

typedef unsigned int cv_fp_result;
#define CV_MATCH_FOUND				0x00000001
#define CV_TEMPLATE_UPDATED			0x00000002

typedef unsigned int cv_policy;
#define CV_ALLOW_SESSION_AUTH		0x00000001
#define CV_REQUIRE_OTP_SIGNED_TIME	0x00000002

typedef unsigned short cv_obj_auth_flags;
#define CV_OBJ_AUTH_READ_PUB		0x00000001
#define CV_OBJ_AUTH_READ_PRIV		0x00000002
#define CV_OBJ_AUTH_WRITE_PUB		0x00000004
#define CV_OBJ_AUTH_WRITE_PRIV		0x00000008
#define CV_OBJ_AUTH_DELETE			0x00000010
#define CV_OBJ_AUTH_ACCESS			0x00000020
#define CV_OBJ_AUTH_CHANGE_AUTH		0x00000040
#define CV_ADMINISTRATOR_AUTH		0x00008000

typedef unsigned short cv_auth_session_status;
#define CV_AUTH_SESSION_TERMINATED	0x00000001

typedef unsigned int cv_auth_session_flags;
#define CV_AUTH_SESSION_CONTAINS_SHARED_SECRET	0x00000001
#define CV_AUTH_SESSION_PER_USAGE_AUTH			0x00000002
#define CV_AUTH_SESSION_SINGLE_USE				0x00000004

typedef unsigned int cv_attrib_flags;
#define CV_ATTRIB_PERSISTENT			0x00000001
#define CV_ATTRIB_NON_HASHABLE			0x00000002
#define CV_ATTRIB_NVRAM_STORAGE			0x00000004
#define CV_ATTRIB_SC_STORAGE			0x00000008
#define CV_ATTRIB_CONTACTLESS_STORAGE	0x00000010
#define CV_ATTRIB_USE_FOR_SIGNING		0x00010000
#define CV_ATTRIB_USE_FOR_ENCRYPTION	0x00020000
#define CV_ATTRIB_USE_FOR_DECRYPTION	0x00040000
#define CV_ATTRIB_USE_FOR_KEY_DERIVATION	0x00080000
#define CV_ATTRIB_USE_FOR_WRAPPING		0x00100000
#define CV_ATTRIB_USE_FOR_UNWRAPPING	0x00200000

#define CV_MAJOR_VERSION 1
#define CV_MINOR_VERSION 1

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
#define DSA2048_PRIV_LEN 20
#define DSA_SIG_LEN 2*CV_NONCE_LEN
#define RSA2048_KEY_LEN 256
#define RSA2048_KEY_LEN_BITS 2048
#define ECC256_KEY_LEN 32
#define ECC256_KEY_LEN_BITS ECC256_KEY_LEN*8
#define ECC256_SIG_LEN 2*ECC256_KEY_LEN
#define ECC256_POINT (2*ECC256_KEY_LEN) 
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
#define HOTP_KEY_MAX_LEN 32

typedef unsigned int cv_status;
enum cv_status_e {
		CV_SUCCESS,
		CV_SUCCESS_SUBMISSION,
		CV_PROMPT_FOR_SMART_CARD,
		CV_PROMPT_FOR_CONTACTLESS,
		CV_READ_HID_CREDENTIAL,
		CV_PROMPT_PIN,
		CV_INVALID_HANDLE,
		CV_OBJECT_NOT_VALID,
		CV_AUTH_FAIL,
		CV_OBJECT_WRITE_REQUIRED,
		CV_IN_LOCKOUT,
		CV_PROMPT_FOR_FINGERPRINT_SWIPE,
		CV_INVALID_VERSION,
		CV_PARAM_BLOB_INVALID_LENGTH,
		CV_PARAM_BLOB_INVALID,
		CV_PARAM_BLOB_AUTH_FAIL,
		CV_PARAM_BLOB_DECRYPTION_FAIL,
		CV_PARAM_BLOB_ENCRYPTION_FAIL,
		CV_PARAM_BLOB_RNG_FAIL,
		CV_SECURE_SESSION_SUITE_B_REQUIRED,
		CV_SECURE_SESSION_KEY_GENERATION_FAIL,
		CV_SECURE_SESSION_KEY_HASH_FAIL,
		CV_SECURE_SESSION_KEY_SIGNATURE_FAIL,
		CV_VOLATILE_MEMORY_ALLOCATION_FAIL,
		CV_SECURE_SESSION_BAD_PARAMETERS,
		CV_SECURE_SESSION_SHARED_SECRET_FAIL,
		CV_CALLBACK_TIMEOUT,
		CV_INVALID_OBJECT_HANDLE,
		CV_HOST_STORAGE_REQUEST_TIMEOUT,
		CV_HOST_STORAGE_REQUEST_RESULT_INVALID,
		CV_OBJ_AUTH_FAIL,
		CV_OBJ_DECRYPTION_FAIL,
		CV_OBJ_ANTIREPLAY_FAIL,
		CV_OBJ_VALIDATION_FAIL,
		CV_PROMPT_PIN_AND_SMART_CARD,
		CV_PROMPT_PIN_AND_CONTACTLESS,
		CV_OBJECT_ATTRIBUTES_INVALID,
		CV_NO_PERSISTENT_OBJECT_ENTRY_AVAIL,
		CV_OBJECT_DIRECTORY_FULL,
		CV_NO_VOLATILE_OBJECT_ENTRY_AVAIL,
		CV_FLASH_MEMORY_ALLOCATION_FAIL,
		CV_ENUMERATION_BUFFER_FULL,
		CV_ADMIN_AUTH_NOT_ALLOWED,
		CV_ADMIN_AUTH_REQUIRED,
		CV_CORRESPONDING_AUTH_NOT_FOUND_IN_OBJECT,
		CV_INVALID_AUTH_LIST,
		CV_UNSUPPORTED_CRYPT_FUNCTION,
		CV_CANCELLED_BY_USER,
		CV_POLICY_DISALLOWS_SESSION_AUTH,
		CV_CRYPTO_FAILURE,
		CV_RNG_FAILURE, 
		CV_REMOVE_PROMPT,
		CV_INVALID_OUTPUT_PARAMETER_LENGTH, 
		CV_INVALID_OBJECT_SIZE,
		CV_PROMPT_SUPRESSED,
		CV_INVALID_GCK_KEY_LENGTH,
		CV_INVALID_DA_PARAMS,
		CV_CV_NOT_EMPTY,
		CV_NO_GCK,
		CV_RTC_FAILURE,
		CV_INVALID_KDIX_KEY,
		CV_INVALID_KEY_TYPE,
		CV_INVALID_KEY_LENGTH,
		CV_KEY_GENERATION_FAILURE,
		CV_INVALID_KEY_PARAMETERS,
		CV_INVALID_OBJECT_TYPE,
		CV_INVALID_ENCRYPTION_PARAMETER,
		CV_INVALID_HMAC_KEY,
		CV_INVALID_INPUT_PARAMETER_LENGTH, 
		CV_INVALID_BULK_ENCRYPTION_PARAMETER,
		CV_ENCRYPTION_FAILURE,
		CV_INVALID_INPUT_PARAMETER,
		CV_SIGNATURE_FAILURE,
		CV_INVALID_OBJECT_PERSISTENCE,
		CV_OBJECT_NONHASHABLE,
		CV_SIGNED_TIME_REQUIRED,
		CV_INVALID_SIGNATURE,
		CV_INTERNAL_DEVICE_FAILURE,
		CV_FLASH_ACCESS_FAILURE,
		CV_RTC_NOT_AVAILABLE
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
		CV_PKCS_BLOB
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
		CV_STATUS_TIME
};

typedef unsigned int cv_config_type;
enum cv_config_type_e {
		CV_CONFIG_DA_PARAMS,
		CV_CONFIG_REPLAY_OVERRIDE,
		CV_CONFIG_POLICY,
		CV_CONFIG_TIME,
		CV_CONFIG_KDIX
};

typedef unsigned int cv_operating_status;
enum cv_operating_status_e {
		CV_OK,
		CV_DEVICE_FAILURE,
		CV_MONOTONIC_COUNTER_FAILURE,
		CV_FLASH_INIT_FAILURE
};

typedef unsigned int cv_otp_mac_type;
enum cv_otp_mac_type_e {
		CV_MAC1,
		CV_MAC2
};

typedef unsigned short cv_fingerprint_type;
enum cv_fingerprint_type_e {
		CV_FINGERPRINT_IMAGE,
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
		CV_TYPE_AUTH_SESSION,
		CV_TYPE_HOTP,
};

typedef unsigned short cv_obj_attribute_type;
enum cv_obj_attribute_type_e {
		CV_ATTRIB_TYPE_FLAGS,
		CV_ATTRIB_TYPE_APP_DEFINED
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
		CV_AUTH_HID,
		CV_AUTH_PIN,
		CV_AUTH_SMARTCARD,
		CV_AUTH_PROPRIETARY_SC	,
		CV_AUTH_CHALLENGE,
		CV_AUTH_SUPER,
		CV_AUTH_ST_CHALLENGE,
		CV_AUTH_CAC_PIV,
		CV_AUTH_PIV_CTLS,
		CV_AUTH_HOTP,
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

/*
 * Structures
 */

typedef struct PACKED  td_cv_auth_list_hdr {
	uint8_t				numEntries;	/* number of auth list entries */
	uint8_t				listID;		/* ID of auth list */
	cv_obj_auth_flags	flags;		/* booleans indicating authorization granted */
} cv_auth_list_hdr;
typedef PACKED cv_auth_list_hdr cv_auth_list;

typedef struct PACKED  td_cv_attrib_hdr {
	cv_obj_attribute_type	attribType;		/* type of attribute */
	uint16_t				attribLen;		/* length of attribute in bytes */
} cv_attrib_hdr;
typedef cv_attrib_hdr cv_obj_attributes;

typedef struct PACKED  td_cv_auth_hdr {
	cv_object_authorizations	authType;		/* type of authorization */
	uint16_t					authLen;		/* length of authorization in bytes */
} cv_auth_hdr;

typedef struct PACKED  td_cv_obj_attribs_hdr {
	uint16_t	attribsLen;	/* length of object attributes in bytes */
	cv_attrib_hdr attrib;	/* header for first attribute in list */
} cv_obj_attribs_hdr;

typedef struct PACKED  td_cv_obj_auth_lists_hdr {
	uint16_t	authListsLen;	/* length of object auth lists in bytes */
	cv_auth_list_hdr authList;	/* header for first auth list in list */
} cv_obj_auth_lists_hdr;

typedef struct PACKED  td_cv_obj_hdr {
	cv_obj_type			objType;	/* type of object */
	uint16_t			objLen;		/* length of object in bytes, excluding objType and objLen */
} cv_obj_hdr;

typedef struct PACKED  td_cv_auth_entry_passphrase {
	uint8_t passphraseLen;		/* length of passphrase */
	uint8_t passphrase[1];		/* passphrase byte string */
} cv_auth_entry_passphrase;

typedef struct PACKED  td_cv_auth_entry_passphrase_handle {
	uint8_t			constZero;		/* value: 0 */
	cv_obj_handle	hPassphrase;	/* handle to passphrase object */
} cv_auth_entry_passphrase_handle;

typedef struct PACKED  td_cv_auth_entry_passphrase_HMAC {
	uint8_t HMACLen;	/* length of HMAC; 20 bytes for SHA1 and 32 bytes for SHA-256 */
	uint8_t HMAC[1];	/* HMAC */
} cv_auth_entry_passphrase_HMAC;

typedef struct PACKED  td_cv_auth_entry_fingerprint_obj {
	uint8_t			numTemplates;	/* number of templates */
	cv_obj_handle	templates[1];	/* list of template handles */
} cv_auth_entry_fingerprint_obj;

typedef struct PACKED  td_cv_auth_entry_hotp {
	cv_obj_handle	hHOTPObj;	/* handle for HOTP object */
} cv_auth_entry_hotp;

typedef struct PACKED  td_cv_auth_hotp_command {
	uint64_t	OTPValue; /* 6, 8 or 10 digit OTP value for authentication */
} cv_auth_hotp_command;

typedef struct PACKED  td_cv_auth_entry_auth_session {
	cv_obj_handle	hAuthSessionObj;	/* handle for auth session object */
} cv_auth_entry_auth_session;

typedef struct PACKED  td_cv_auth_data {
	uint8_t	authData[20];	/* authorization data */
} cv_auth_data;

typedef struct PACKED  td_cv_nonce {
	uint8_t	nonce[20];	/* nonce */
} cv_nonce;

typedef struct PACKED  td_cv_hotp_unblock_auth {
	uint64_t	OTPValue1;
	uint64_t	OTPValue2;
} cv_hotp_unblock_auth;

typedef struct PACKED  td_cv_otp_nonce {
	uint8_t	nonce[AES_128_BLOCK_LEN];	/* nonce */
} cv_otp_nonce;

typedef struct PACKED  td_cv_time {
	uint8_t	time[14];	/* time in hex: YYYYMMDDHHMMSS (eg. 0x20,0x05,0x12,0x14,0x21,0x10,0x00 for  2005 Dec 14 21:10:00) */
} cv_time;

typedef struct PACKED  td_cv_auth_entry_auth_session_HMAC {
	cv_obj_handle	hAuthSessionObj;	/* handle for auth session object */
	uint8_t			HMACLen;			/* length of HMAC; 20 bytes for SHA1 and 32 bytes for SHA-256 */
	uint8_t			HMAC[1];			/* HMAC */
} cv_auth_entry_auth_session_HMAC;

typedef struct PACKED  td_cv_auth_entry_count {
	uint32_t	count;	/* count of times object usage allowed */
} cv_auth_entry_count;

typedef struct PACKED  td_cv_auth_entry_time {
	cv_time	startTime;	/* start time for authorization */
	cv_time	endTime;	/* end time for authorization */
} cv_auth_entry_time;

typedef struct PACKED  td_cv_auth_entry_HID {
	cv_obj_handle	hHIDObj;	/* handle to object containing HID credential */
} cv_auth_entry_HID;

typedef struct PACKED  td_cv_auth_entry_PIN {
	uint8_t PINLen;	/* length of PIN */
	uint8_t PIN[1];	/* PIN byte string */
} cv_auth_entry_PIN;

typedef struct PACKED  td_cv_obj_value_certificate {
	uint16_t	certLen;/* length of certificate */
	uint8_t	cert;		/* certificate */
} cv_obj_value_certificate;

typedef struct PACKED  td_cv_obj_value_fingerprint {
	cv_fingerprint_type	type;	/* cv_fingerprint_type */
	uint16_t			fpLen;	/* fingerprint length */
	uint8_t				fp[1];	/* fingerprint */
} cv_obj_value_fingerprint;

typedef struct PACKED  td_cv_obj_value_opaque {
	uint16_t	valueLen;	/* value length in bytes*/
	uint8_t	value[1];		/* value */
} cv_obj_value_opaque;

typedef struct PACKED  td_cv_obj_value_passphrase {
	uint16_t	passphraseLen;	/* value length in bytes*/
	uint8_t	passphrase[1];		/* value */
} cv_obj_value_passphrase;

typedef struct PACKED  td_cv_obj_value_hash {
	cv_hash_type	hashType;	/* cv_hash_type */
	uint8_t			hash[1];	/* hash */
} cv_obj_value_hash;

typedef struct PACKED  td_cv_obj_value_hotp {
	uint32_t version; /* Set this to 1 */
	uint32_t currentCounterValue; /* Set this to the desired counter start value at object creation time */
	uint8_t counterInterval; /* Set this to 1 or to another small number. The counter will be increased by this number each time */
	uint8_t	randomKey[HOTP_KEY_MAX_LEN];
	uint8_t	randomKeyLength; /* At least 16. 20 is recommended. */
	uint32_t allowedMaxWindow;
	uint32_t maxTries;
	uint32_t allowedLargeMaxWindowForUnblock;
	uint8_t	numberOfDigits; /* 6, 8 or 10*/
	cv_bool	isBlocked; /* Set this to FALSE at object creation time */
	uint32_t currentNumberOfTries; /* Set this to 0 at creation time */
} cv_obj_value_hotp;

typedef struct PACKED  td_cv_obj_value_aes_key {
	uint16_t	keyLen;	/* key size in bits */
	uint8_t	key[1];		/* key */
} cv_obj_value_aes_key;

typedef struct PACKED  td_cv_obj_value_key {
	uint16_t	keyLen;	/* key size in bits */
	uint8_t	key[1];		/* key */
} cv_obj_value_key;

typedef struct PACKED  td_cv_obj_value_oath_token {
	cv_otp_token_type type;				/* token type (RSA or OATH) */
	uint32_t countValue;				/* count value */
	uint8_t clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t sharedSecretLen;			/* length of shared secret */
	uint8_t	sharedSecret[CV_NONCE_LEN];		/* sharedSecret */
} cv_obj_value_oath_token;

typedef struct PACKED  td_cv_obj_value_auth_session {
	cv_obj_auth_flags		statusLSH;	/* status of auth session */
	cv_auth_session_status	statusMSH;	/* status of auth session */
	cv_auth_session_flags	flags;		/* flags indicating usage of auth session */
	cv_auth_data			authData;	/* auth data to allow usage of this auth session */
	/* shared secret included if indicated by flags */
} cv_obj_value_auth_session;

typedef struct PACKED  td_cv_app_id {
	uint8_t	appIDLen;	/* length of application ID */
	uint8_t	appID[1];	/* application ID byte string */
} cv_app_id;

typedef struct PACKED  td_cv_user_id {
	uint8_t	userIDLen;	/* length of application ID */
	uint8_t	userID[1];	/* application ID byte string */
} cv_user_id;

typedef struct PACKED  td_cv_version {
	uint16_t	versionMajor;	/* major version of CV */
	uint16_t	versionMinor;	/* minor version of CV */
} cv_version;

typedef struct PACKED  td_cv_bulk_params {
	cv_bulk_mode		bulkMode;		/* bulk mode */
	cv_auth_algorithm	authAlgorithm;	/* algorithm for authorization */
	cv_obj_handle		hAuthKey;		/* handle to authorization key object */
	uint16_t			authOffset;		/* offset to start of authorization */
	uint16_t			encOffset;		/* offset to start of encryption */
	uint16_t			IVLen;			/* length of IV */
	uint8_t				*IV;			/* pointer to IV byte string */
	cv_obj_handle		hHashObj;		/* pointer to output hash object */
} cv_bulk_params;

typedef struct PACKED  tdotp_rsa_alg_params {
	uint8_t serialNumber[4];
	uint8_t displayDigits;
	uint8_t displayIntervalSeconds;
} tpm_otp_rsa_alg_params;

typedef struct PACKED  td_cv_obj_value_rsa_token {
	cv_otp_token_type type;				/* token type (RSA or OATH) */
	tpm_otp_rsa_alg_params rsaParams;	/* RSA parameters */
	uint8_t clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t sharedSecretLen;			/* length of shared secret */
	uint8_t	sharedSecret[AES_128_BLOCK_LEN];			/* sharedSecret */
} cv_obj_value_rsa_token;

typedef struct PACKED  td_cv_otp_rsa_keygen_params_direct {
	cv_otp_provisioning_type	otpProvType;	/* provisioning type, direct or CT-KIP */
	tpm_otp_rsa_alg_params		rsaParams;		/* RSA params */
	uint8_t						masterSeed[AES_128_BLOCK_LEN];	/* master seed, for direct provisioning */
} cv_otp_rsa_keygen_params_direct;

typedef struct PACKED  td_cv_otp_rsa_keygen_params_ctkip {
	cv_otp_provisioning_type	otpProvType;	/* provisioning type, direct or CT-KIP */
	tpm_otp_rsa_alg_params		rsaParams;		/* RSA params */
	uint8_t						clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t						serverNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint32_t					serverPubkeyLen; /* length of server public key */
	uint8_t						serverPubkey[1]; /* server public key */
} cv_otp_rsa_keygen_params_ctkip;

typedef struct PACKED  td_cv_otp_oath_keygen_params {
	uint8_t						clientNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint8_t						serverNonce[AES_128_BLOCK_LEN];	/* client nonce */
	uint32_t					serverPubkeyLen; /* length of server public key */
	uint8_t						serverPubkey[1]; /* server public key */
} cv_otp_oath_keygen_params;

typedef struct PACKED  td_cv_space {
	uint32_t	totalVolatileSpace;			/* total bytes of volatile space */
	uint32_t	totalNonvolatileSpace;		/* total bytes of non-volatile space */
	uint32_t	remainingVolatileSpace;		/* remaining bytes of volatile space) */
	uint32_t	remainingNonvolatileSpace;	/* remaining bytes of non-volatile space */
} cv_space;

typedef struct PACKED  td_cv_da_params {
	uint32_t	maxFailedAttempts;			/* maximum failed attempts before lockout */
	uint32_t	lockoutDuration;			/* duration in seconds of initial lockout period */
} cv_da_params;

typedef struct PACKED  td_cv_time_params {
	cv_time	localTime;	
	cv_time	GMTTime;	
} cv_time_params;

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
				cv_obj_type *objType,                                    
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
					cv_obj_handle *pNewObjHandle,
					cv_callback *callback, cv_callback_ctx context);

cv_status
cv_export_object (cv_handle cvHandle, cv_blob_type blobType,                               
					byte blobLabel[16], cv_obj_handle objectHandle,                                 
					uint32_t authListLength, cv_auth_list *pAuthList,                             
					cv_enc_op encryptionOperation,                              
					cv_bulk_params *pBulkParameters,                            
					cv_obj_handle wrappingObjectHandle,                                 
					cv_hash_type HMACtype,                                 
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
cv_hotp_unblock (cv_handle cvHandle, cv_obj_handle hotpObjectHandle,
				 cv_hotp_unblock_auth* unblockAuth,
				 byte* bIsBlockedNow);

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
cv_fingerprint_enroll (cv_handle cvHandle, cv_fingerprint_type fingerprintType,             
			uint32_t objAttributesLength, cv_obj_attributes *pObjAttributes, 
			uint32_t authListsLength, cv_auth_list *pAuthLists,
			uint32_t *pTemplateLength, byte *pTemplate,
			cv_obj_handle *pTemplateHandle,
			cv_callback *callback, cv_callback_ctx context);

cv_status
cv_fingerprint_verify (cv_handle cvHandle, cv_fp_control fpControl,
			uint32_t numTemplates, cv_obj_handle *pTemplateHandles,             
			cv_fp_result *pFpResult, uint32_t *pTemplateIndex,
			cv_callback *callback, cv_callback_ctx context);

#endif				       /* end _CVAPI_H_ */

