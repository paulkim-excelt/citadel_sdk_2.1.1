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
 * cvcommon.h:  Credential Vault internal structures used for HostStorage & Host Control Services
 */
/*
 * Revision History:
 *
 * 01/03/07 Created.
*/

#ifndef _CVCOMMON_H_
#define _CVCOMMON_H_ 1

#include "cvapi.h"

#define CV_SEM_NAME "cvUshSem"
#define CV_SEM_FILE_NAME "/dev/shm/sem.cvUshSem"

#define MAX_ENCAP_RESULT_BUFFER	(4096+100)
#define MAX_ENCAP_FP_RESULT_BUFFER	96 * 1024

#define USB_CV_VENDOR_ID		0x0a5c
#define USB_CV_PID_5840         0x5840
#define USB_CV_PID_5841         0x5841
#define USB_CV_PID_5842         0x5842
#define USB_CV_PID_5843         0x5843
#define USB_CV_PID_5844         0x5844
#define USB_CV_PID_5845         0x5845
#define USB_CV_EP_BULK_ADDR     1
#define USB_CV_EP_INT_ADDR      5
#define USB_CV_MAX_INT_DATA_IN_LEN  32
#define MAX_WAIT_TX_TIME	    3000   /* 3000 ms */
#define MAX_WAIT_RX_TIME	    3000   /* 3000 ms */
#define DISPLEN				    0x08

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

typedef unsigned short cv_command_id;
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
		CV_CMD_USH_VERSIONS,           			// 0x0039
		CV_HOST_STORAGE_DELETE_ALL_FILES,       // 0x003A
		CV_CMD_OTP_PROG,						// 0x003b
		CV_CMD_CUSTID_PROG,						// 0x003c
		CV_CMD_PARTNER_DISABLE_PROG,			// 0x003d
		CV_CMD_PARTNER_ENABLE_PROG,				// 0x003e
		CV_CMD_ENABLE_RADIO,					// 0x003f
		CV_CMD_ENABLE_SMARTCARD,				// 0x0040
		CV_CMD_ENABLE_FINGERPRINT,				// 0x0041
		//command ids for the new cv commands
		CV_CMD_READ_CONTACTLESS_CARD_ID, 		// 0x0042
        CV_CMD_FW_UPGRADE_START,                // 0x0043
        CV_CMD_FW_UPGRADE_UPDATE,               // 0x0044
        CV_CMD_FW_UPGRADE_COMPLETE,             // 0x0045
		CV_CMD_ENABLE_AND_LOCK_RADIO,			// 0x0046
        CV_CMD_SET_RF_PRESENT,					// 0x0047
        CV_CMD_NOTIFY_POWER_STATE_TRANSITION,	// 0x0048
        CV_CMD_SET_CV_ONLY_RADIO_MODE,			// 0x0049
        CV_CMD_NOTIFY_SYSTEM_POWER_STATE,		// 0x004A
        CV_CMD_REBOOT_TO_SBI,					// 0x004B
        CV_CMD_REBOOT_USH,						// 0x004C
		CV_CMD_START_CONSOLE_LOG,				// 0x004d
		CV_CMD_STOP_CONSOLE_LOG,				// 0x004e
		CV_CMD_GET_CONSOLE_LOG,					// 0x004f
		CV_CMD_PUTCHARS,						// 0x0050
		CV_CMD_CONSOLE_MENU,					// 0x0051
    	CV_CMD_SET_SYSTEM_ID,			        // 0x0052
        CV_CMD_SET_RFID_PARAMS,					// 0x0053
		CV_CMD_SET_SMARTCARD_PRESENT,			// 0x0054
        CV_CMD_SEND_BLOCKDATA,					// 0x0055
        CV_CMD_RECEIVE_BLOCKDATA,				// 0x0056
        CV_CMD_PWR,								// 0x0057
        CV_CMD_PWR_START_IDLE_TIME,				// 0x0058
		CV_CMD_IO_DIAG,	                    	// 0x0059
		CV_CMD_UPGRADE_OBJECTS,					// 0x005a
		CV_CMD_GET_MACHINE_ID,                  // 0x005b
		CV_CMD_FINGERPRINT_CAPTURE_WBF,         // 0x005c
		CV_CMD_FINGERPRINT_RESET,				// 0x005d
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
		CV_CMD_FINGERPRINT_CREATE_TEMPLATE,	// 0x006f
		CV_CMD_FINGERPRINT_ENROLL_DUP_CHK, 	// 0x0070
		CV_CMD_ENUMERATE_OBJECTS_DIRECT, 	// 0x0071
		CV_CMD_FINGERPRINT_VERIFY_W_HASH,	// 0x0072
		CV_CMD_FINGERPRINT_IDENTIFY_W_HASH,	// 0x0073
		CV_CMD_POWER_STATE_SLEEP,		// 0x0074
		CV_CMD_POWER_STATE_RESUME,		// 0x0075
		CV_CMD_DEVICE_STATE_REMOVE,		// 0x0076
		CV_CMD_DEVICE_STATE_ARRIVAL,		// 0x0077
		CV_CMD_FUNCTION_ENABLE,			// 0x0078
		CV_CMD_READ_NFC_CARD_UUID, 		// 0x0079
		CV_CMD_SET_NFC_POWER_TO_MAX,		// 0x007A
		CV_CMD_GET_RPB,				// 0x007B
		CV_CMD_ENABLE_NFC,			// 0x007C
		CV_CMD_SET_NFC_PRESENT,			// 0x007D
		CV_CMD_HOTP_UNBLOCK,			// 0x007E
		CV_CMD_SET_T4_CARDS_ROUTING,	// 0x007F
		CV_CMD_DETECT_FP,               // 0x0080
		CV_CMD_GET_SBL_PID,             // 0x0081
		CV_CMD_BIOMETRIC_UNIT_CREATED,  // 0x0082
		CV_CMD_GENERATE_ECDSA_KEY,      // 0x0083
		CV_CMD_MANAGE_POA,              // 0x0084
		CV_CMD_SET_DEBUG_LOGGING,       // 0x0085
		CV_CMD_GET_DEBUG_LOGGING,       // 0x0086
		CV_CMD_SET_EC_PRESENT,          // 0x0087
		CV_CMD_WBF_GET_SESSION_ID,      // 0x0088
		CV_CMD_ERASE_FLASH_CHIP,        // 0x0089
		CV_CMD_ENROLLMENT_STARTED,      // 0x008A
		CV_CMD_FINGERPRINT_CAPTURE_START_MS,    // 0x008B  Capture in Modern Standby Mode
		CV_CMD_HOST_ENTER_CS_STATE,             // 0x008C
		CV_CMD_HOST_EXIT_CS_STATE,              // 0x008D
		CV_CMD_HOST_USER_LOGGED,                // 0x008E
};

/*
 * structures
 */

typedef unsigned int cv_session_flags;  
/* bits map to cv_options, where applicable */
#define CV_HAS_PRIVACY_KEY	0x40000000 /* session has specified privacy key to wrap objects*/
#define CV_REMOTE_SESSION	0x80000000	/* remote session (applies to CV Host mode API only) */

typedef union td_cv_hmac_key 
{
	uint8_t SHA1[SHA1_LEN];		/* HMAC-SHA1 key */
	uint8_t SHA256[SHA256_LEN];	/* HMAC-SHA256 key */
} cv_hmac_key;

typedef unsigned short cv_secure_session_protocol;
enum cv_secure_session_protocol_e {
		AES128wHMACSHA1 = 0x00000000
};


 typedef unsigned short cv_return_type;
 enum cv_return_type_e {
 		CV_RET_TYPE_SUBMISSION_STATUS	= 0x00000000,
 		CV_RET_TYPE_INTERMED_STATUS		= 0x00000001,
 		CV_RET_TYPE_RESULT				= 0x00000002,
		CV_RET_TYPE_RESUBMITTED         = 0x00000003
};

typedef PACKED struct td_cv_encap_flags {
	unsigned short	secureSession : 1 ;			/* secure session */
	unsigned short	completionCallback : 1 ;	/* contains completion callback */
	unsigned short	host64Bit : 1 ;				/* 64 bit host */
	cv_secure_session_protocol	secureSessionProtocol : 3 ;	/* secure session protocol*/
	unsigned short	USBTransport : 1 ;			/* 1 for USB; 0 for LPC */
	cv_return_type	returnType : 2 ;			/* return type */
	unsigned short suppressUIPrompts : 1; /* session has CV_SUPPRESS_UI_PROMPTS set */
	unsigned short	spare : 6;					/* spare */
} cv_encap_flags;

typedef PACKED struct td_cv_encap_command {
	cv_transport_type	transportType;		/* CV_TRANS_TYPE_ENCAPSULATED */
	uint32_t			transportLen;		/* length of entire transport block in bytes */
	cv_command_id		commandID;			/* ID of encapsulated command */
	cv_encap_flags		flags;				/* flags related to encapsulated commands */
	cv_version			version;			/* version of CV which created encapsulated command */
	//cv_handle			hSession;			/* session handle */
	uint32_t			hSession;			/* session handle */
	cv_status			returnStatus;		/* return status (0, if not applicable) */
	uint8_t				IV[AES_128_IV_LEN];	/* IV for bulk encryption */
	uint32_t			parameterBlobLen;	/* length of parameter blob in bytes */
	uint32_t			parameterBlob;		/* encapsulated parameters (variable length) */
	/* structure is variable length here:
		parameter blob
		secure session HMAC
		padding with zeros for block length, if encrypted		*/
} cv_encap_command;

typedef struct PACKED chip_spec_hdr {
        uint16_t cmd_magic;
        uint16_t cmd;
        uint32_t data[15];
} chip_spec_hdr_t;

#define USBD_COMMAND_MAGIC	(0x0606)
/* command id  */
#define CMD_COMMON		(0x01)
#define CMD_EXTEND		(0x02)

#define CMD_DOWNLOAD		((CMD_COMMON << 8) | 0x01)

typedef struct td_cv_session { 
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
} PACKED cv_session;

typedef PACKED struct td_cv_session_details {
	cv_handle		cvHandle;			/*An unique no for the session. This no will be
										  treated as a cv handle by the application*/
	cv_handle		cvInternalHandle;	/*Actual cv handle*/
	cv_session		stCVSession;		/*Instance of cv_session structure*/
	uint32_t		hasAppCallbacks;	/*TRUE if app has received prompt callbacks for this session*/
} cv_session_details;

typedef struct td_cv_param_list_entry { 
	cv_param_type	paramType;	/* type of parameter */
	uint32_t		paramLen;	/* length of parameter */
	//uint32_t		param;		/* parameter (variable length) */
} PACKED cv_param_list_entry;

typedef struct td_cv_fp_callback_ctx {  
	cv_callback		*callback;	/* callback routine passed in with API */
	cv_callback_ctx context;	/* context passed in with API */
} PACKED cv_fp_callback_ctx;

typedef struct td_cv_secure_session_get_pubkey_in { 
	uint8_t		pubkey[RSA2048_KEY_LEN];/* RSA 2048-bit public key for encrypting host nonce */
	uint8_t		sig[40];				/* DSA signature of pubkey using Kdi or Kdix (if any) */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} PACKED cv_secure_session_get_pubkey_in;

typedef struct td_cv_secure_session_get_pubkey_in_suite_b { 
	uint8_t		pubkey[ECC256_POINT];	/* ECC 256 public key for encrypting host nonce */
	uint8_t		sig[ECC256_SIG_LEN];	/* ECC signature of pubkey using provisioned device ECC signing key */
	uint8_t		nonce[CV_NONCE_LEN];	/* PHX2 nonce for secure session key generation */
	uint32_t	useSuiteB;				/* flag indicating suite b crypto should be used */
} PACKED cv_secure_session_get_pubkey_in_suite_b;

#define CV_IOC_MAGIC    'k'

#define CV_GET_LAST_COMMAND_STATUS _IOR(CV_IOC_MAGIC,1,int)
#define CV_GET_LATEST_COMMAND_STATUS _IOR(CV_IOC_MAGIC,2,int)
#define CV_GET_COMMAND_STATUS _IOR(CV_IOC_MAGIC,3,int)
#define CV_GET_CONFIG_DESCRIPTOR  _IOR(CV_IOC_MAGIC,4,int)
#define CV_SUBMIT_CMD       _IOW(CV_IOC_MAGIC,5,int)

#endif				       /* end _CVCOMMON_H_ */


