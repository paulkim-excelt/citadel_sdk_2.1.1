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

/* 
 * Enumerated values and types
 */
#define MAX_INPUT_PARAMS 20 /* TBD */
#define MAX_OUTPUT_PARAMS 20 /* TBD */
#define MAX_OUTPUT_PARAM_SIZE 4096 /* TBD */
#define ALIGN_DWORD(x) ((x + 0x3) & 0xfffffffc)
#define CALLBACK_STATUS_TIMEOUT 5000 /* milliseconds */
#define HOST_STORAGE_REQUEST_TIMEOUT 500 /* milliseconds */
#define USER_PROMPT_TIMEOUT 30000 /* milliseconds */
#define USER_PROMPT_CALLBACK_INTERVAL 1000 /* milliseconds */
#define MAX_CV_OBJ_CACHE_NUM_OBJS			4	/* must divide obj cache into 4-byte aligned areas */
#define MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing other directories */
#define MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES	2	/* number of cached directory pages for pages containing objects */
#define INVALID_CV_CACHE_ENTRY				0xff /* invalid value to use for initializing cache entries */
#define MAX_CV_ENTRIES_PER_DIR_PAGE			16	/* number of directory pages per higher level page */
#define MAX_DIR_LEVELS						3   /* note: this reflects implementation and can't be changed w/o changing code */
#define MAX_CV_NUM_VOLATILE_OBJS			20	/* must divide obj cache into 4-byte aligned areas */
#define MAX_DIR_PAGE_0_ENTRIES				32	/* maximum number of directory page 0 entries */
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
#define GET_AES_128_PAD_LEN(x) (AES_128_BLOCK_LEN - ((x) &(AES_128_BLOCK_LEN - 1)))
#define GET_AES_192_PAD_LEN(x) (AES_192_BLOCK_LEN - (AES_192_BLOCK_LEN - (x % AES_192_BLOCK_LEN)))
#define GET_AES_256_PAD_LEN(x) (AES_256_BLOCK_LEN - ((x) &(AES_256_BLOCK_LEN - 1)))
#define MAX_CV_AUTH_PROMPTS 5 /* TBD */
#define CV_HID_CREDENTIAL_SIZE 0 /* TBD */
#define DEFAULT_CV_DA_AUTH_FAILURE_THRESHOLD 6
#define DEFAULT_CV_DA_INITIAL_LOCKOUT_PERIOD 5000 /* in milliseconds */
#define RSA_PUBKEY_CONSTANT 65537
#define KDC_PUBKEY_LEN 2048
#define CV_OBJ_CACHE_SIZE			9216
#define CV_LEVEL1_DIR_PAGE_CACHE_SIZE (sizeof(cv_dir_page_level1)*MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES)
#define CV_LEVEL2_DIR_PAGE_CACHE_SIZE (sizeof(cv_dir_page_level1)*MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES)
#define CV_IO_COMMAND_BUF_SIZE		9216
#define OPEN_MODE_BLOCK_PTRS (0x00000000)
#define SECURE_MODE_BLOCK_PTRS (0x00000000)
#define SYS_AND_TPM_IO_OPEN_WINDOW	0
#define SMART_CARD_IO_OPEN_WINDOW	1
#define RF_IO_OPEN_WINDOW			2
#define CV_IO_OPEN_WINDOW			3
#define CV_OBJ_CACHE_ENTRY_SIZE		CV_OBJ_CACHE_SIZE/MAX_CV_OBJ_CACHE_NUM_OBJS
#define CV_VOLATILE_OBJECT_HEAP_LENGTH USH_HEAP_SIZE

typedef unsigned int cv_status_internal;
#define CV_FP_NO_MATCH					0x80000001
#define CV_FP_MATCH						0x80000002
#define CV_FP_IMAGE_MATCH_AND_UPDATED	0x80000003
#define CV_OBJECT_UPDATE_REQUIRED		0x80000004

typedef unsigned int cv_transport_type;
enum cv_transport_type_e {
		CV_TRANS_TYPE_ENCAPSULATED = 0x00000001,
		CV_TRANS_TYPE_HOST_STORAGE = 0x00000002
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
		CV_RET_TYPE_INTERMED_STATUS		= 0x00000001,
		CV_RET_TYPE_RESULT				= 0x00000002
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
};

#define	CV_DIR_0_HANDLE				0x00000000
#define	CV_TOP_LEVEL_DIR_HANDLE		0x00ffffff
#define	CV_UCK_OBJ_HANDLE			0x01000000
#define	CV_PERSISTENT_DATA_HANDLE	0x00000001
#define	CV_MIRRORED_DIR_0			0xff000000
#define	CV_MIRRORED_TOP_LEVEL_DIR	 0xffffffff

#define CV_MIRRORED_OBJ_MASK	0xff000000
#define CV_OBJ_HANDLE_PAGE_MASK	0xff000000

typedef unsigned short cv_command_id;
enum cv_command_id_e {
		CV_ABORT,
		CV_CALLBACK_STATUS,
		CV_CMD_OPEN,
		CV_CMD_OPEN_REMOTE,
		CV_CMD_CLOSE,
		CV_CMD_GET_SESSION_COUNT,
		CV_CMD_INIT,
		CV_CMD_GET_STATUS,
		CV_CMD_SET_CONFIG,
		CV_CMD_CREATE_OBJECT,
		CV_CMD_DELETE_OBJECT,
		CV_CMD_SET_OBJECT,
		CV_CMD_GET_OBJECT,
		CV_CMD_ENUMERATE_OBJECTS,
		CV_CMD_DUPLICATE_OBJECT,
		CV_CMD_IMPORT_OBJECT,
		CV_CMD_EXPORT_OBJECT,
		CV_CMD_CHANGE_AUTH_OBJECT,
		CV_CMD_AUTHORIZE_SESSION_OBJECT,
		CV_CMD_DEAUTHORIZE_SESSION_OBJECT,
		CV_CMD_GET_RANDOM,
		CV_CMD_ENCRYPT,
		CV_CMD_DECRYPT,
		CV_CMD_HMAC,
		CV_CMD_HASH,
		CV_CMD_HASH_OBJECT,
		CV_CMD_GENKEY,
		CV_CMD_SIGN,
		CV_CMD_VERIFY,
		CV_CMD_OTP_GET_VALUE,
		CV_CMD_OTP_GET_MAC,
		CV_CMD_UNWRAP_DLIES_KEY,
		CV_CMD_FINGERPRINT_ENROLL,
		CV_CMD_FINGERPRINT_VERIFY,
		CV_SECURE_SESSION_GET_PUBKEY,
		CV_SECURE_SESSION_HOST_NONCE,
		CV_HOST_STORAGE_GET_REQUEST,
		CV_HOST_STORAGE_STORE_OBJECT,
		CV_HOST_STORAGE_DELETE_OBJECT,
		CV_HOST_STORAGE_RETRIEVE_OBJECT,
		CV_HOST_STORAGE_OPERATION_STATUS
};

typedef unsigned int cv_interrupt_type;
enum cv_interrupt_type_e {
		CV_COMMAND_PROCESSING_INTERRUPT,
		CV_HOST_STORAGE_INTERRUPT
};

typedef unsigned int cv_internal_state;
#define CV_PENDING_WRITE_TO_SC			0x00000001 /* an object is waiting to be written to the smart card, 
														but  can’t be completed yet due to smart card not 
														available or PIN needed */
#define CV_PENDING_WRITE_TO_CONTACTLESS	0x00000002 /* an object is waiting to be written to the contactless 
														smart card, but  can’t be completed yet due to smart 
														card not available or PIN needed */
#define CV_IN_DA_LOCKOUT				0x00000004 /* dictionary attack mitigation has initiated a lockout */
#define CV_WAITING_FOR_PIN_ENTRY		0x00000008 /* authorization testing has determined that a PIN is 
														needed and control has returned to calling app to allow 
														command to be resubmitted with PIN */

typedef unsigned int cv_device_state;
#define CV_SMART_CARD_PRESENT		0x00000001 /* indicates smart card present in reader */
#define CV_CONTACTLESS_SC_PRESENT	0x00000002 /* indicates a contactless smart card is present */

typedef unsigned int cv_phx2_device_status;
enum cv_phx2_device_status_e {
	CV_PHX2_OPERATIONAL = 0x00000001,			/* PHX2 ok, no failures detected */
	CV_PHX2_MONOTONIC_CTR_FAIL = 0x00000002,	/* monotonic counter failure */
	CV_PHX2_FLASH_INIT_FAIL = 0x00000004		/* flash init failure */
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

typedef unsigned int cv_session_flags;
/* bits map to cv_options, where applicable */
#define CV_HAS_PRIVACY_KEY	0x40000000 /* session has specified privacy key to wrap objects*/
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

#define CV_INIT_TOP_LEVEL_DIR	0x00000001
#define CV_INIT_PERSISTENT_DATA 0x00000002
#define CV_INIT_DIR_0			0x00000004

/* default ECC domain parameters TBD */
#define ECC_DEFAULT_DOMAIN_PARAMS_PRIME 0x0, 0x0
#define ECC_DEFAULT_DOMAIN_PARAMS_A 0x0, 0x0
#define ECC_DEFAULT_DOMAIN_PARAMS_B 0x0, 0x0
#define ECC_DEFAULT_DOMAIN_PARAMS_G 0x0, 0x0 
#define ECC_DEFAULT_DOMAIN_PARAMS_n 0x0, 0x0 
#define ECC_DEFAULT_DOMAIN_PARAMS_h 0x0, 0x0 

/* default DSA domain parameters TBD */
#define DSA_DEFAULT_DOMAIN_PARAMS_P 0x0, 0x0
#define DSA_DEFAULT_DOMAIN_PARAMS_Q 0x0, 0x0
#define DSA_DEFAULT_DOMAIN_PARAMS_G 0x0, 0x0

/* default DH domain parameters TBD */
#define DH_DEFAULT_DOMAIN_PARAMS_G 0x0, 0x0
#define DH_DEFAULT_DOMAIN_PARAMS_M 0x0, 0x0

/*
 * structures
 */
typedef PACKED union td_cv_hmac_key
{
	uint8_t SHA1[SHA1_LEN];		/* HMAC-SHA1 key */
	uint8_t SHA256[SHA256_LEN];	/* HMAC-SHA256 key */
} cv_hmac_key;
typedef cv_hmac_key cv_hmac_val;

typedef struct PACKED  td_cv_rsa2048_key {
	uint8_t m[RSA2048_KEY_LEN];		/* public modulus */
	uint32_t e;						/* public exponent */
	uint8_t p[RSA2048_KEY_LEN/2];		/* private prime */
	uint8_t q[RSA2048_KEY_LEN/2];		/* private prime */
	uint8_t pinv[RSA2048_KEY_LEN/2];	/* CRT param */
	uint8_t dp[RSA2048_KEY_LEN/2];	/* CRT param */
	uint8_t dq[RSA2048_KEY_LEN/2];	/* CRT param */
} cv_rsa2048_key;

typedef struct PACKED  td_cv_dsa1024_key {
	uint8_t p[DSA1024_KEY_LEN];	/* public p */
	uint8_t q[DSA1024_PRIV_LEN];/* public q */
	uint8_t g[DSA1024_KEY_LEN];	/* public g */
	uint8_t x[DSA1024_PRIV_LEN];/* private key */
	uint8_t y[DSA1024_KEY_LEN];	/* public key */
} cv_dsa1024_key;

typedef struct PACKED  td_cv_dsa1024_key_no_dp {
	uint8_t x[DSA1024_PRIV_LEN];/* private key */
	uint8_t y[DSA1024_KEY_LEN];	/* public key */
} cv_dsa1024_key_no_dp;

typedef struct PACKED  td_cv_dsa1024_key_priv_only {
	uint8_t x[DSA1024_PRIV_LEN];	/* private key */
} cv_dsa1024_key_priv_only;

typedef struct PACKED  td_cv_ecc256 {
	uint8_t prime[ECC256_KEY_LEN];	/* prime */
	uint8_t a[ECC256_KEY_LEN];		/* curve parameter */
	uint8_t b[ECC256_KEY_LEN];		/* curve parameter */
	uint8_t G[ECC256_POINT];		/* base point */
	uint8_t n[ECC256_KEY_LEN];		/* order of base point */
	uint8_t h[ECC256_KEY_LEN];		/* cofactor */
	uint8_t x[ECC256_KEY_LEN];		/* private key */
	uint8_t Q[ECC256_POINT];		/* public key */
} cv_ecc256;

typedef struct PACKED  td_cv_ecc256_no_dp {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
	uint8_t Q[ECC256_POINT];		/* public key */
} cv_ecc256_no_dp;

typedef struct PACKED  td_cv_ecc256_priv_only {
	uint8_t x[ECC256_KEY_LEN];		/* private key */
} cv_ecc256_priv_only;

typedef struct PACKED  td_cv_encap_flags {
	unsigned short	secureSession : 1 ;			/* secure session */
	unsigned short	completionCallback : 1 ;	/* contains completion callback */
	unsigned short	host64Bit : 1 ;				/* 64 bit host */
	cv_secure_session_protocol	secureSessionProtocol : 3 ;	/* secure session protocol*/
	unsigned short	USBTransport : 1 ;			/* 1 for USB; 0 for LPC */
	cv_return_type	returnType : 2 ;			/* return type */
	unsigned short	spare : 7;					/* spare */
} cv_encap_flags;

typedef struct PACKED  td_cv_encap_command {
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

typedef struct PACKED  td_cv_secure_session_get_pubkey_out {
	uint8_t		nonce[CV_NONCE_LEN];	/* anti-replay nonce */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_out;

typedef struct PACKED  td_cv_secure_session_get_pubkey_in {
	uint8_t		pubkey[RSA2048_KEY_LEN];/* RSA 2048-bit public key for encrypting host nonce */
	uint8_t		sig[DSA_SIG_LEN];				/* DSA signature of pubkey using Kdi or Kdix (if any) */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_in;

typedef struct PACKED  td_cv_secure_session_get_pubkey_in_suite_b {
	uint8_t		pubkey[ECC256_POINT];	/* ECC 256 public key for encrypting host nonce */
	uint8_t		sig[ECC256_SIG_LEN];	/* ECC signature of pubkey using provisioned device ECC signing key */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} cv_secure_session_get_pubkey_in_suite_b;

typedef struct PACKED  td_cv_secure_session_host_nonce {
	uint8_t	encNonce[RSA2048_KEY_LEN];	/* host nonce encrypted with CV RSA public key */
} cv_secure_session_host_nonce;

typedef struct PACKED  td_cv_secure_session_host_nonce_suite_b {
	uint8_t	nonce[CV_NONCE_LEN];	/* host nonce (unencrypted) */
	uint8_t pubkey[ECC256_POINT];	/* host ECC256 public key */
} cv_secure_session_host_nonce_suite_b;

typedef struct PACKED  td_cv_host_storage_get_request {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_GET_REQUEST */
} cv_host_storage_get_request;

typedef struct PACKED  td_cv_host_storage_object_entry {
	cv_obj_handle	handle;	/* object/dir ID */
	uint32_t		length;	/* object/dir blob length */
} cv_host_storage_object_entry;

typedef struct PACKED  td_cv_host_storage_store_object {
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

typedef struct PACKED  td_cv_host_storage_delete_object {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_DELETE_OBJECT */
	cv_obj_handle		objectID;		/* ID of object to be deleted */
} cv_host_storage_delete_object;

typedef struct PACKED  td_cv_host_storage_retrieve_object_in {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_RETRIEVE_OBJECT */
	/* remainder is variable length of the form:
		object ID 1
		...
		object ID n				*/
} cv_host_storage_retrieve_object_in;

typedef struct PACKED  td_cv_host_storage_retrieve_object_out {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_RETRIEVE_OBJECT */
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

typedef struct PACKED  td_cv_host_storage_operation_status {
	cv_transport_type	transportType;	/* CV_TRANS_TYPE_HOST_STORAGE */
	uint32_t			transportLen;	/* length of entire transport block in bytes */
	cv_command_id		commandID;		/* CV_HOST_STORAGE_OPERATION_STATUS */
	cv_status			status;			/* return status of retrieve object request */
} cv_host_storage_operation_status;

typedef struct PACKED  td_cv_usb_interrupt {
	cv_interrupt_type	interruptType;	/* CV_COMMAND_PROCESSING_INTERRUPT or CV_HOST_STORAGE_INTERRUPT */
	uint32_t			resultLen;		/* length of result to read */
} cv_usb_interrupt;

typedef struct PACKED  td_cv_dir_page_entry_dir {
	cv_obj_handle	hDirPage;	/* handle of directory page */
	uint32_t		counter;	/* anti-replay counter value */
} cv_dir_page_entry_dir;

typedef struct PACKED  td_cv_dir_page_entry_persist {
	cv_dir_page_entry_dir	entry;				/* directory entry */
	uint32_t				multiBufferIndex;	/* anti-replay counter value */
	uint32_t				len;				/* length of persistent data */
} cv_dir_page_entry_persist;

typedef struct PACKED  td_cv_dir_page_hdr {
	uint32_t				numEntries;		/* number of entries in this directory page */
	cv_dir_page_entry_dir	thisDirPage;	/* handle and counter for this dir page */
	/* remainder is variable length of the form:
			directory page entry 1
			...
			directory page entry n		*/
} cv_dir_page_hdr;

typedef struct PACKED  td_cv_obj_storage_attributes {
	unsigned short	hasPrivacyKey : 1 ;		/* object has privacy key encryption */
	cv_storage_type	storageType : 3 ;		/* type of device for object storage */
	unsigned short	PINRequired : 1 ;		/* storage device requires PIN */
	unsigned short	spare : 11 ;			/* spare */
	cv_obj_type		objectType: 16;			/* object type */  
} cv_obj_storage_attributes;

typedef struct PACKED  td_cv_dir_page_entry_obj {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
} cv_dir_page_entry_obj;

typedef struct PACKED  td_cv_dir_page_entry_obj_flash {
	cv_obj_handle	hObj;					/* handle of object */
	uint32_t		counter;				/* anti-replay counter value */
	cv_obj_storage_attributes	attributes;	/* object storage attributes */
	uint8_t			appUserID[20];			/* hash of app ID and user ID fields for session */
	uint32_t		objLen;					/* length of object */
	uint8_t			*pObj;					/* pointer to object in flash */
} cv_dir_page_entry_obj_flash;

// not necessary?
//typedef struct PACKED  td_cv_dir_page_entry_dir0 {
//	cv_obj_handle	hObj;					/* handle of object */
//	uint32_t		counter;				/* anti-replay counter value */
//	uint32_t		objLen;					/* length of object */
//	byte			*pObj;					/* pointer to object in flash */
//} cv_dir_page_entry_dir0;

typedef struct PACKED  td_cv_dir_page_0 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj_flash dir0Objs[MAX_DIR_PAGE_0_ENTRIES];	/* object entries for directory page 0 */
} cv_dir_page_0;

typedef struct PACKED  td_cv_dir_page_level1 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_dir dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 1 directory page */
} cv_dir_page_level1;

typedef struct PACKED  td_cv_dir_page_level2 {
	cv_dir_page_hdr header;
	cv_dir_page_entry_obj dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* object entries for level 2 directory page */
} cv_dir_page_level2;

typedef struct PACKED  td_cv_top_level_dir {
	cv_dir_page_hdr			header;										/* handle and count associated with top level directory */
	uint8_t					pageMapFull[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page full (1) or not (0) */
	uint8_t					pageMapExists[MAX_DIR_PAGES/8];				/* map indicating if corresponding dir page exists (1) or not (0) */
	cv_dir_page_entry_dir	dirEntries[MAX_CV_ENTRIES_PER_DIR_PAGE];	/* lower level directories contained in top level */
	cv_dir_page_entry_persist	persistentData;							/* entry associated with persistent data */
} cv_top_level_dir;

typedef struct PACKED  td_cv_obj_dir_page_blob_enc {
	cv_obj_handle	hObj;						/* handle for object or directory page */
	uint32_t		counter;					/* anti-replay counter value for this blob */
} cv_obj_dir_page_blob_enc;


typedef struct PACKED  td_cv_obj_dir_page_blob {
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

typedef struct PACKED  td_cv_session {
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
} cv_session;

typedef struct PACKED  td_cv_volatile_obj_dir_page_entry {
	cv_obj_handle	hObj;		/* handle of volatile object */
	uint8_t			*pObj;		/* pointer to volatile object in scratch RAM */
	cv_session		*hSession;	/* session ID which created this object */
} cv_volatile_obj_dir_page_entry;

typedef struct PACKED  td_cv_volatile_obj_dir {
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
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE];
} cv_cmdbuf;

typedef struct td_cv_secure_iobuf {
	uint8_t buf[CV_IO_COMMAND_BUF_SIZE];
} cv_secure_iobuf;

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
	cv_obj_handle			lastEnumeratedHandle;/* for multi-stage enumeration, this is handle from last enumeration */
	uint8_t					*HIDCredentialPtr;	 /* for HID credential read in */
} cv_volatile;

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
	int32_t				localTimeZoneOffset;		/* offset +/- of local time from GMT */
	uint32_t			DAInitialLockoutPeriod;		/* configured lockout period for dictionary attack processing */
	uint16_t			DAAuthFailureThreshold;		/* Configured count of auth failures before lockout */
	cv_space			space;						/* memory allocation info (only flash portion is persistent) */
	uint32_t			CVAdminAuthListLen;			/* Length of CV admin auth list */
	uint8_t				spare[256];
	/* following item is variable length, but has maximum size indicated by array */
	cv_auth_list		CVAdminAuthList;			/* CV admin auth list */
	uint8_t				authList[MAX_CV_ADMIN_AUTH_LISTS_LENGTH];
} cv_persistent;

typedef struct PACKED  td_cv_obj_properties {
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
} cv_obj_properties;

typedef struct PACKED  td_cv_export_blob {
	cv_version		version;					/* version of CV used to create blob */
	uint8_t			label[16];					/* user-provided label */
	uint32_t		encAreaLen;					/* length in bytes of symmetric encrypted area */
	uint8_t			IV[AES_128_IV_LEN];			/* IV for AES-CTR mode */
	uint32_t		symKeyEncAreaLen;			/* length of encrypted symmetric key (if any) in bytes */
	/* start of encrypted area */
	/* remainder is variable length of the form:
			asymmetric encrypted symmetric key (if any)
			encrypted object (padded to AES-128 block size with zeroes)
			encrypted HMAC		*/
} cv_export_blob;

typedef struct PACKED  td_cv_mscapi_blob {
	cv_mscapi_blob_type	bType;	/* blob type */
	uint8_t				bVersion;			/* blob version */
	uint16_t			reserved;			/* not used */
	cv_mscapi_alg_id	aiKeyAlg;			/* algorithm identifier for wrapped key */
	/* remainder is variable length of the form:
			blob type-specific header
			key		*/
} cv_mscapi_blob;

typedef struct PACKED  td_cv_mscapi_simple_blob_hdr {
	cv_mscapi_alg_id	aiKeyAlg;			/* encryption algorithm used to encrypt key */
} cv_mscapi_simple_blob_hdr;

typedef struct PACKED  td_cv_mscapi_rsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in modulus */
	uint32_t		pubexp;	/* public exponent */
} cv_mscapi_rsa_blob_hdr;

typedef struct PACKED  td_cv_mscapi_dsa_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime, P */
} cv_mscapi_dsa_blob_hdr;

typedef struct PACKED  td_cv_mscapi_dh_blob_hdr {
	cv_mscapi_magic	magic;	/* string indicating blob contents */
	uint32_t		bitlen;	/* number of bits in prime modulus, P */
} cv_mscapi_dh_blob_hdr;

typedef struct td_cv_param_list_entry {
	cv_param_type	paramType;	/* type of parameter */
	uint32_t		paramLen;	/* length of parameter */
	uint32_t		param;		/* parameter (variable length) */
} cv_param_list_entry;

typedef struct PACKED  td_cv_internal_callback_ctx {
	cv_encap_command	*cmd;	/* command being processed */
} cv_internal_callback_ctx;

typedef struct PACKED  td_cv_input_parameters {
	uint32_t numParams;	/* number of input parameters */
	struct PACKED  {
		uint32_t paramLen;	/* length of parameter */
		void *param;		/* pointer to parameter */
	} paramLenPtr[MAX_INPUT_PARAMS];
} cv_input_parameters;

typedef struct PACKED  td_cv_fp_callback_ctx {
	cv_callback		*callback;	/* callback routine passed in with API */
	cv_callback_ctx context;	/* context passed in with API */
} cv_fp_callback_ctx;

/* flash size structures */
#define CV_NUM_FLASH_MULTI_BUFS 2		/* number of multiple buffers for flash object where necessary (top level */
										/* directory and persistent data) */
#define CV_FLASH_MULTI_BUF_MASK (CV_NUM_FLASH_MULTI_BUFS - 1)
typedef struct PACKED  td_cv_flash_dir_page_0 {
	cv_obj_dir_page_blob	wrapper;					/* blob wrapper */
	cv_dir_page_0			dirPage0;					/* directory page 0 */
	uint8_t					hmac[SHA256_LEN];			/* blob HMAC */
	uint8_t					align[AES_128_BLOCK_LEN];	/* max padding for encryption */
} cv_flash_dir_page_0;

typedef struct PACKED  td_cv_flash_top_level_dir_entry {
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

#define CV_PERSISTENT_OBJECT_HEAP_LENGTH (46*1024) /* 46K */
typedef struct td_flash_cv {
	cv_flash_dir_page_0			dirPage0;
	cv_flash_top_level_dir		topLevelDir;
	cv_flash_persistent_data	persistentData;
	uint8_t						persistentObjectHeap[CV_PERSISTENT_OBJECT_HEAP_LENGTH];
} flash_cv;

typedef	struct td_cv_local_context {
		cv_callback *callback;
		cv_callback_ctx context;
} cv_local_context;

#endif				       /* end _CVINTERNAL_H_ */


