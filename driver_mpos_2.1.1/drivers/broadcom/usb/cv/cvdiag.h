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

#ifndef __CV_DIAG_H__
#define __CV_DIAG_H__


/*
 * Includes
 */


/*
 * Enumerated values and types
 */

typedef unsigned int cv_diag_cmd_type;
enum cv_diag_cmd_type_e {
    CV_DIAG_CMD_TEST_GRP         = 0x00000001,
    CV_DIAG_CMD_TEST_IND,
    CV_DIAG_CMD_TEST_RFID_PARAMS,
    CV_DIAG_CMD_GET_ANT_STATUS,
    CV_DIAG_CMD_GET_CONSOLE,
    CV_DIAG_CMD_HDOTP_GET_STATUS,        // get hdotp & antihammering status (enable/disable)
    CV_DIAG_CMD_HDOTP_SET_STATUS,	 // set hdotp & antihammering status (enable/disable)
    CV_DIAG_CMD_HDOTP_PEEK,		 // get value in hdotp array
    CV_DIAG_CMD_HDOTP_POKE,		 // set value in hdotp array
    CV_DIAG_CMD_HDOTP_GET_SKIP_LIST,	 // get hdotp skip list
    CV_DIAG_CMD_HDOTP_SET_SKIP_LIST,	 // set hdotp skip list
    CV_DIAG_CMD_HDOTP_GET_CV_CNTR,	 // get CV counter
    CV_DIAG_CMD_HDOTP_GET_AH_CRDT_LVL,	 // get antihammering credit level
    CV_DIAG_CMD_HDOTP_GET_ELPSD_TIME,	 // get elapsed time since boot
    CV_DIAG_CMD_HDOTP_GET_SKIP_MONO_CNTR,// get skip monotonic counter
    CV_DIAG_CMD_HDOTP_GET_SPEC_SKIP_CNTR,// get speculated skip next counter
    CV_DIAG_CMD_HDOTP_SET_SKIP_CNTR, 	 // set skip counter
    CV_DIAG_CMD_TEST_RFID_PRODUCTION,
	CV_DIAG_CMD_SC_DEACTIVATE,
    CV_DIAG_CMD_GET_RFID_STATUS
};

typedef unsigned int cv_diag_test_grp_type;
enum cv_diag_test_grp_type_e {
    CV_DIAG_TEST_GRP_SMARTCARD   = 0x00000001,
    CV_DIAG_TEST_GRP_FINGERPRINT = 0x00000002,
    CV_DIAG_TEST_GRP_RFID        = 0x00000004,
    CV_DIAG_TEST_GRP_USB_HOST    = 0x00000008,
    CV_DIAG_TEST_GRP_TPM         = 0x00000010,
    CV_DIAG_TEST_GRP_CV          = 0x00000020,
    CV_DIAG_TEST_GRP_ALL         = 0x0000003f
};

typedef unsigned int cv_diag_test_ind_type;
enum cv_diag_test_ind_type_e {
    CV_DIAG_TEST_IND_SMARTCARD   = 0x00000001,
    CV_DIAG_TEST_IND_FINGERPRINT,
    CV_DIAG_TEST_IND_RFID,
    CV_DIAG_TEST_IND_USB_HOST,
    CV_DIAG_TEST_IND_TPM,
    CV_DIAG_TEST_IND_CV,
    CV_DIAG_TEST_IND_SMARTCARD_JC,
    CV_DIAG_TEST_IND_LAST
};

typedef unsigned int cv_diag_test_production;
enum cv_diag_test_production_e {
    CV_DIAG_TEST_PRODUCTION_5SEC   = 0x00000000,
    CV_DIAG_TEST_PRODUCTION_1MIN,
    CV_DIAG_TEST_PRODUCTION_3MIN
};

enum cv_diag_rfid_params_type_e {
    CV_DIAG_RFID_PARAMS_14A   = 0,
    CV_DIAG_RFID_PARAMS_14B,
    CV_DIAG_RFID_PARAMS_15,
    CV_DIAG_RFID_PARAMS_MAX
};

#define CvDiagRfidCurMsk        0x000000ff
#define CvDiagRfidCurShft       0
#define CvDiagRfidDrvStrMsk     0x0000ff00
#define CvDiagRfidDrvStrShft    8
#define CvDiagRfidModDepMsk     0x00ff0000
#define CvDiagRfidModDepShft    16

enum cv_diag_get_ant_status_e {
    CV_DIAG_ANTENNA_DETECTED   = 0x00000000,
    CV_DIAG_ANTENNA_NOT_DETECTED,
    CV_DIAG_ANTENNA_NO_POWER
};

enum cv_diag_get_rfid_status_e {
    CV_DIAG_RFID_CURRENT   = 0x00000000,
    CV_DIAG_RFID_VOLTAGE,
    CV_DIAG_RFID_UNKNOWN
};

// cv_diag_test_rfid_params.general defines
#define CV_DIAG_RFID_PARAM_GEN_PWRON		0x00000100
#define CV_DIAG_RFID_PARAM_GEN_PWROFF		0x00000200
#define CV_DIAG_RFID_PARAM_GEN_CCFON 		0x00000400
#define CV_DIAG_RFID_PARAM_GEN_CCFOFF 		0x00000800
#define CV_DIAG_RFID_PARAM_GEN_PROTO_FIXED	0x00000080
#define CV_DIAG_RFID_PARAM_GEN_PROTO_MSK	0x0000000f
#define CV_DIAG_RFID_PARAM_GEN_PROTO_SHFT	0

/*
 * structures
 */

typedef PACKED struct td_cv_diag_test_rfid_params
{
    uint32_t    unused;
    uint32_t    type[CV_DIAG_RFID_PARAMS_MAX];
    uint32_t    general;
} cv_diag_test_rfid_params;

typedef PACKED struct td_cv_diag_ush_cnsl_params
{
    uint32_t    *bufLen;
    uint32_t    *bufPtr;
} cv_diag_ush_cnsl_params;

typedef PACKED struct td_cv_diag_test_gen_params
{
    uint32_t    testNum;
} cv_diag_test_gen_params;

typedef PACKED union td_cv_diag_cmd_params
{
    cv_diag_test_grp_type   group;          /* group test bitmask */
    cv_diag_test_ind_type   individual;     /* individual test number */
    cv_diag_test_rfid_params rfid_params;   /* rfid parameters */
    cv_diag_ush_cnsl_params cnsl_params;    /* get ush cnsl */
} cv_diag_cmd_params;

typedef PACKED struct td_cv_diag_command {
    cv_diag_cmd_type        diag_cmd;       /* diagnostic command */
    cv_diag_cmd_params      params;         /* diagnostic command parameters */
} cv_diag_command;

typedef PACKED struct td_cv_diag_cmd_grp_status {
    cv_diag_test_grp_type   test_status;    /* bitmap of test status; 0-failed, 1-passed */
} cv_diag_cmd_grp_status;

typedef PACKED struct td_cv_vend_dev_id {
    uint16_t vendor;
    uint16_t device;
} cv_vend_dev_id;

#define MAX_USB_DEVICES     8
typedef PACKED struct td_cv_usb_device_info {
    uint32_t        num_dev;
    cv_vend_dev_id  dev_ids[MAX_USB_DEVICES];
} cv_usb_device_info;

typedef PACKED union td_cv_ind__params
{
    cv_usb_device_info    usb;            /* usb parameters */
} cv_ind_params;

typedef PACKED struct td_cv_diag_cmd_ind_status {
    unsigned int            test_status;    /* individual diagnostic test status */
    cv_ind_params           params;
} cv_diag_cmd_ind_status;


/*
 * rfid diag tests
 */

enum cv_diag_rfid_type_e {
    CV_DIAG_RFID_14A   = 0x00000001,
    CV_DIAG_RFID_14B,
    CV_DIAG_RFID_15,
    CV_DIAG_RFID_ICLASS_14B,
    CV_DIAG_RFID_ICLASS_15,
    CV_DIAG_RFID_FELICA,
    CV_DIAG_RFID_FELICA_RW   /* for Felica read/write test */

};

/*
 * function prototypes
 */

/* core */
cv_status ush_bootrom_core_selftest(void);
cv_status ush_bootrom_core_tests(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer);

/* rfid */
cv_status rfid_simple_diag(void);
cv_status rfid_complex_diag(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer, uint32_t bInitOnly);
cv_status rfid_param_diag(uint32_t numInputParams, uint32_t **inputparam, uint32_t *outLength, uint32_t *outBuffer);

/* sc */
cv_status sc_complex_diag(uint32_t numInputParams, uint32_t **inputparam, uint32_t *outLength, uint32_t *outBuffer);

cv_status tpm_connectivity_test(void);

#endif  /* __CV_DIAG_H__ */
