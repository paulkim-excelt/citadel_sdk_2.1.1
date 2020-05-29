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
 * scratchram.h:  PHX2 USH external interfaces
 */
/*
 * Revision History:
 *
 * 01/08/07 DPA Created.
*/

#ifndef _EXTINTF_H_
#define _EXTINTF_H_ 1

#define USB_AVAIL 1
#include "sci/berr.h"
#include "ccidmemblock.h"
/*
 * PHX2 scheduler
 *
*/
void
yield(void);

void
yield_to_usb_and_delay(uint32_t ms);

void
yield_and_delay(uint32_t ms);

/*
 * OS and system
 *
*/
cv_status
get_ms_ticks(uint32_t *ticks);

cv_status
ms_wait(uint32_t ms);

cv_status
get_time(cv_time *time);

cv_status
set_time(cv_time *time);

/* timer defines */
#define MAX_INTERRUPT_HANDLERS	5
#define LOW_RES_TIMER_INTERVAL 100 /* 100 ms */
typedef struct td_timer_interrupt_entry {
	uint8_t (*int_handler)(void);	/* interrupt handler to be called at interval */
	uint32_t interval;				/* interval in ms to call isr */
	uint32_t count;					/* timer tics till next call */
} timer_interrupt_entry;
typedef struct td_high_res_timer_interrupt_table {
	timer_interrupt_entry timerEntries[MAX_INTERRUPT_HANDLERS];
} high_res_timer_interrupt_table;
typedef struct td_low_res_timer_interrupt_table {
	timer_interrupt_entry timerEntries[MAX_INTERRUPT_HANDLERS];
} low_res_timer_interrupt_table;
/* timer return codes */
#define USH_RET_TIMER_INT_OK				0
#define USH_RET_TIMER_INT_SCHEDULE_EVENT	1		/* return code for timer interrupt handler to indicate event was scheduled */
#define USH_RET_TIMER_INT_NOT_AVAIL			2		/* return code for enable_timer_int if no timer can be allocated */
#define USH_RET_TIMER_INT_INVALID_INTERVAL	3		/* return code if invalid interval requested */
void
init_low_res_timer_ints(void);

uint32_t
enable_low_res_timer_int(uint32_t ms_interval, uint8_t (*int_handler)(void));

uint32_t
disable_low_res_timer_int(uint8_t (*int_handler)(void));

void
handle_low_res_timer_int(void);

void *
cv_malloc(uint32_t bytes);

void
cv_free(void *, uint32_t bytes);

void *
cv_malloc_flash(uint32_t bytes);

void
cv_free_flash(void *);

void
cv_flash_heap_init(void);

void
set_smem_openwin_base_addr(uint32_t window, uint8_t *addr);

void
set_smem_openwin_end_addr(uint32_t window, uint8_t *addr);

void
set_smem_openwin_open(uint32_t window);

void
set_smem_openwin_secure(uint32_t window);

cv_status
get_monotonic_counter(uint32_t *count);

cv_status
get_predicted_monotonic_counter(uint32_t *count);

cv_status
bump_monotonic_counter(uint32_t *count);

cv_status
ush_read_flash(uint32_t flashAddr, uint32_t len, uint8_t *buf);

cv_status
ush_write_flash(uint32_t flashAddr, uint32_t len, uint8_t *buf);

/* diagnostics */
cv_status
ush_diag(uint32_t subcmd, uint32_t param1, uint32_t numInputParams, uint32_t **inputparam, uint32_t *outputparam, uint32_t *outputLen);

/* version */
cv_status
ush_bootrom_versions(uint32_t numInputParams, uint32_t *inputparam, uint32_t *outLength, uint32_t *outBuffer);

/*
 * USB
 */

#include "cvinterface.h"

/*
 * TPM
 */

/* to send command to host */
cv_status
tpmCvQueueCmdToHost(cv_encap_command *cmd);

/* to submit buffer to receive command from host */
cv_status
tpmCvQueueBufFromHost(cv_encap_command *cmd);

/* to check availability of command from host */
cv_status
tpmCvPollCmdRcvd(void);

/* to get key from key cache */
cv_status
tpmGetKey(uint8_t *p_key,uint8_t *q_key);

/* inform TPM that monotonic counter bumped */
void
tpmMonotonicCounterUpdated(void);

/* inform CV that TPM bumped monotonic counter */
void
cvMonotonicCounterUpdated(void);

/*
 * SCAPI
 */
void phx_crypto_init(crypto_lib_handle *pHandle);

/*
 * fingerprint device
 */
cv_status
fpRegCallback(cv_callback callback, cv_callback_ctx context);

cv_status
fpCapture(uint8_t **image, uint32_t *length);

cv_status
fpVerify(uint8_t * hint, int32_t FARvalue, uint32_t featureSetLength, uint8_t *featureSet, uint32_t templateLength, uint8_t *fpTemplate, cv_bool *match);

/*
 * smart card device
 */
/* return codes */
typedef unsigned int cvsc_status;
enum cvsc_status_e {
		CVSC_OK,
		CVSC_FAILED,
		CVSC_NOT_PRESENT,
		CVSC_PIN_REQUIRED
};

/* signature algorithms */
typedef unsigned int cvsc_sign_alg;
enum cvsc_sign_alg_e {
		CVSC_RSA_1024,
		CVSC_RSA_2048,
		CVSC_ECC_256
};

cvsc_status
scGetObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *obj);

cvsc_status
scPutObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t objLen, uint8_t *obj);

cvsc_status
scDelObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN);

/* for card detect */
typedef unsigned int cvsc_card_type;
enum cvsc_card_type_e {
		CVSC_DELL_JAVA_CARD,
		CVSC_PKI_CARD,
		CVSC_CAC_CARD,
		CVSC_PIV_CARD
};
cvsc_status
scIsCardPresent(void);

cvsc_status
scGetCardType(cvsc_card_type *cardType);

/* for Dell Java applet smart cards */
cvsc_status
scGetUserID(uint32_t *userIDLen, uint8_t *userID);

cvsc_status
scGetUserKey(uint32_t *userKeyLen, uint8_t *userKey, uint32_t PINLen, uint8_t *PIN);

/* for PKI smart cards (CAC, PIV, Windows) - don't know which of these requires a PIN */
cvsc_status
scGetPubKey(uint32_t *pubKeyLen, uint8_t *pubKey);

cvsc_status
scChallenge(uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN);

/* for CAC/PIV contacted SCs */
cvsc_status
scGetPIVCardCHUID(uint32_t *len, uint8_t *chuid);

cvsc_status
scGetCACCardCert(uint32_t *len, uint8_t *cert);

cvsc_status
scCACChallenge(cvsc_sign_alg signAlg, uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN);

cvsc_status
scPIVChallenge(cvsc_sign_alg signAlg, cv_bool isGSCIS, uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN);

/*
 * RF device
 */
cv_status
rfGetObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *obj);

cv_status
rfPutObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t objLen, uint8_t *obj);

cv_status
rfDelObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN);

cv_status
rfGetHIDCredential(uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *HIDCredential, cv_contactless_type *type);

bool
rfIsCardPresent(void);

/* not used now, use BRFD_PayloadType in HID/brfd_api.h instead */
typedef unsigned int BRFD_credential_type;
enum BRFD_credential_type_e {
		BRFD_HID,
		BRFD_PIV
};

BERR_Code
rfGetChannelHandle(BRFD_ChannelHandle *channelHandle);

BERR_Code
rfGetChannelSettings(BRFD_ChannelSettings **channelSettings);

/*
 * RSA SecurID
 */

uint8_t genTokencode(uint8_t *unknown, uint8_t *isoTime, uint8_t *masterSeed,
					uint8_t *serialNumber,
					uint8_t displayDigits, uint8_t displayIntervalSeconds, uint32_t *result);


/* ush get ver support routines */
uint32_t dp_getversion_upper(void);
uint32_t dp_getversion_upper(void);

void cv_post_nfc_init(void);

#endif				       /* end _EXTINTF_H_ */
