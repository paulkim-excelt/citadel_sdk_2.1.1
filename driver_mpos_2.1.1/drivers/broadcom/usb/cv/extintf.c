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
 * Broadcom Corporation Credential Vault
 */
/*
 * extintf.c:  CVAPI stub file for external interfaces
 */
/*
 * Revision History:
 *
 * 03/14/07 DPA Created.
*/
#include "cvmain.h"
#include <phx_osl.h>
#include "isr.h"
#include "tmr.h"
#include "phx_scapi.h"
#include "qspi_flash.h"
#include "fmalloc.h"
#include "cvdiag.h"
#include "extintf.h"

#include "bstd.h"
#include "bkni.h"
#include "bint.h"
#include "berr_ids.h"
#include "bscd.h"
#include "bscd_cli_states.h"
#include "bscd_cli_infra.h"
#include "bscd_cli_util.h"
#include "bscd_datatypes.h"
#include "bscd_priv.h"

#include "sc_java.h"
#include "console.h"

#include "HID/brfd_api.h"

#include "phx_upgrade.h"

extern void *hid_glb_ptr;
#include "nvm_cpuapi.h"
#include "sched_cmd.h"

#define AES_KEYBYTES        16
#define AES_BLOCKBYTES      16

extern cv_status cvXlatefpRetCode(fp_status result);

#ifdef HID_CNSL_REDIRECT
extern uint32_t HIDRF_getDebugData(char *output, uint32_t limit);
#endif
/*
 * PHX2 scheduler
 *
*/
void
yield(void)
{
	;
}


/* Yields control to the os with delay in ms */
void
yield_and_delay(uint32_t ms)
{
	DELAY_MS_WITH_YIELD(ms);
}

/*
 * OS and system
 *
*/
cv_status
ush_read_flash(uint32_t flashAddr, uint32_t len, uint8_t *buf)
{
	if (phx_qspi_flash_read_from_rom(flashAddr, buf, len, VOLATILE_MEM_PTR->flash_device) != PHX_STATUS_OK)
	{
		return CV_FLASH_ACCESS_FAILURE;
	} else {
		return CV_SUCCESS;
	}
}

cv_status
ush_write_flash(uint32_t flashAddr, uint32_t len, uint8_t *buf)
{
	if (phx_qspi_flash_write_verify(flashAddr, buf, len, VOLATILE_MEM_PTR->flash_device) != PHX_STATUS_OK)
		return CV_FLASH_ACCESS_FAILURE;
	else
		return CV_SUCCESS;
}

cv_status
get_ms_ticks(uint32_t *ticks)
{
	*ticks = GET_CUR_TICK_IN_MS();
	return CV_SUCCESS;
}

cv_status
ms_wait(uint32_t ms)
{
	DELAY_MS_WITH_YIELD(ms);
	return CV_SUCCESS;
}

cv_status
get_time(cv_time *time)
{
	return CV_RTC_NOT_AVAILABLE;
}

cv_status
set_time(cv_time *time)
{
	return CV_RTC_NOT_AVAILABLE;
}

void
handle_low_res_timer_int(void)
{
	/* this routine handles a timer tick and determines which handlers to call */
	uint32_t i;
	uint32_t schedEvent = FALSE;

	/* check table for enabled handlers */
	for (i=0;i<MAX_INTERRUPT_HANDLERS;i++)
	{
		if (LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler != NULL)
		{
			LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].count -= LOW_RES_TIMER_INTERVAL;
			if (LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].count == 0)
			{
				if ((*LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler)() == USH_RET_TIMER_INT_SCHEDULE_EVENT)
					schedEvent = TRUE;
				LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].count = LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].interval;
			}
		}
	}
	if (schedEvent) {
		SCHEDULE_FROM_ISR( VOLATILE_MEM_PTR->tim_int_xtaskwoken );
	}

}

void
init_low_res_timer_ints(void)
{
	/* this routine initializes timer interrupts */
	uint32_t i;

	/* clear interrupt handler table */
	for (i=0;i<MAX_INTERRUPT_HANDLERS;i++)
		LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler = 0;
}

uint32_t
enable_low_res_timer_int(uint32_t ms_interval, uint8_t (*int_handler)(void))
{
	/* this routine sets up a timer interrupt, if one is available.  when tick occurs, if interval appropriate, supplied */
	/* interrupt handler will be called.  */
	uint32_t i, retStatus = USH_RET_TIMER_INT_NOT_AVAIL;

	/* check to see if requested interval is multiple of tick */
	if (ms_interval%LOW_RES_TIMER_INTERVAL)
		return USH_RET_TIMER_INT_INVALID_INTERVAL;
	for (i=0;i<MAX_INTERRUPT_HANDLERS;i++)
	{
		if (LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler == NULL)
		{
			/* use this entry */
			LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler = int_handler;
			LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].interval = ms_interval;
			LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].count = ms_interval;
			retStatus = USH_RET_TIMER_INT_OK;
			break;
		}
	}
	return retStatus;
}

uint32_t
disable_low_res_timer_int(uint8_t (*int_handler)(void))
{
	/* this routine disables a timer interrupt handler */
	uint32_t i;

	/* clear interrupt handler table */
	for (i=0;i<MAX_INTERRUPT_HANDLERS;i++)
	{
		if (LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler == int_handler)
		{
			LOW_RES_TIMER_INTERRUPT_TABLE_PTR->timerEntries[i].int_handler = 0;
			break;
		}
	}
	return USH_RET_TIMER_INT_OK;
}

void *
cv_malloc(uint32_t bytes)
{
	void *retVal;

#ifdef CV_5890_DBG
	retVal = malloc(bytes);
#else
	retVal = pvPortMalloc_in_heap(bytes, CV_HEAP);
#endif
	if (retVal != NULL)
		CV_VOLATILE_DATA->space.remainingVolatileSpace -= (bytes + CV_MALLOC_ALLOC_OVERHEAD);

	//printf("cv_malloc: %p %p %d\n", CV_HEAP, retVal, bytes);
	return retVal;
}

void
cv_free(void *memptr, uint32_t bytes)
{
CV_VOLATILE_DATA->space.remainingVolatileSpace += (bytes + CV_MALLOC_ALLOC_OVERHEAD);
  	//printf("cv_free:%p %p\n", CV_HEAP, memptr);
#ifdef CV_5890_DBG
	return free(bytes);
#else
	return vPortFree_in_heap(memptr, CV_HEAP);;
#endif
}

unsigned int cv_heap_free_size() {
	return prvGetTotalFreeMemory_in_heap(CV_HEAP);
}

void *
cv_malloc_flash(uint32_t bytes)
{
	return (phx_fmalloc(bytes));
}

void
cv_free_flash(void *memptr)
{
	phx_ffree(memptr);
}

void
cv_flash_heap_init(void)
{
	init_fheap();
}

void
set_smem_openwin_base_addr(uint32_t window, uint8_t *addr)
{
	;
}

void
set_smem_openwin_end_addr(uint32_t window, uint8_t *addr)
{
	;
}

void
set_smem_openwin_open(uint32_t window)
{
	;
}

void
set_smem_openwin_secure(uint32_t window)
{
	;
}

cv_status
get_monotonic_counter(uint32_t *count)
{
	/* check to see if enabled */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE)
	{
		/* only read if value is 0 (ie, first time after reset or if hdotp skip table needs to be read) */
		if (CV_VOLATILE_DATA->count == 0)
		{
#ifdef HDOTP_SKIP_TABLES
 			if (!phx_get_skp_monotonic_counter(count, CV_VOLATILE_DATA->hdotpSkipTablePtr->table, CV_VOLATILE_DATA->hdotpSkipTablePtr->tableEntries))
#else
			if (!phx_get_monotonic_counter(count))
#endif
				return CV_MONOTONIC_COUNTER_FAIL;
			CV_VOLATILE_DATA->count = *count;
		} else
			*count = CV_VOLATILE_DATA->count;
	} else
		*count = 0;
	return CV_SUCCESS;
}

cv_status
get_predicted_monotonic_counter(uint32_t *count)
{
	/* this routine gets the predicted (ie, next) 2T count value */
	uint32_t startCount;

	/* check to see if enabled */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE)
	{
		/* if 0, just use current count.  non-zero case is if attempt was made to bump 2T and failed */
		/* so next predicted value needs to start after failed one */
		if (CV_VOLATILE_DATA->predictedCount == 0) {
			if ((get_monotonic_counter(&startCount)) != CV_SUCCESS)
				return CV_MONOTONIC_COUNTER_FAIL;
		} else
			startCount = CV_VOLATILE_DATA->predictedCount;
#ifdef HDOTP_SKIP_TABLES
		/* skip table must have been read in before this routine was called */
		if (!specSkpNxtCntr(startCount, count, CV_VOLATILE_DATA->hdotpSkipTablePtr->table, CV_VOLATILE_DATA->hdotpSkipTablePtr->tableEntries))
#else
		if (!specNxtCntr(startCount, count))
#endif
			return CV_MONOTONIC_COUNTER_FAIL;
	} else
		*count = 0;
	return CV_SUCCESS;
}

cv_status
bump_monotonic_counter(uint32_t *count)
{
	uint32_t local_count;
	cv_status retVal = CV_SUCCESS;

	/* check to see if enabled */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE)
	{
		if ((retVal = get_predicted_monotonic_counter(&local_count)) != CV_SUCCESS)
			goto err_exit;
#ifdef HDOTP_SKIP_TABLES
		/* skip table must have been read in before this routine was called */
		if (!progSkpCntr(local_count, CV_VOLATILE_DATA->hdotpSkipTablePtr->table, &CV_VOLATILE_DATA->hdotpSkipTablePtr->tableEntries))
#else
		if (!progCntr(local_count))
#endif
		{
			/* failed, set predicted count to value that just tried to program, so next value will be gotten */
			/* next time get_predicted_monotonic_counter() is called */
#ifndef HDOTP_SKIP_TABLES
			/* this isn't needed for skip tables implementation, because 2T rtn can always use current monotonic counter value */
			CV_VOLATILE_DATA->predictedCount = local_count;
#endif
			retVal = CV_MONOTONIC_COUNTER_FAIL;
			goto err_exit;
		}
		/* bump was successful, reset predicted value to 0 and set current count to value just programmed */
		CV_VOLATILE_DATA->predictedCount = 0;
		*count = local_count;
		CV_VOLATILE_DATA->count = local_count;
		/* invalidate HDOTP skip table here */
		cvInvalidateHDOTPSkipTable();
		/* reset HDOTP read count here, because new count value has been generated */
		resetHDOTPReadLimit();
	} else
		*count = 0;

err_exit:
	return retVal;
}

/*
 * TPM
 */

/* to send command to host */
cv_status
tpmCvQueueCmdToHost(cv_encap_command *cmd)
{
	return CV_SUCCESS;
}

/* to submit buffer to receive command from host */
cv_status
tpmCvQueueBufFromHost(cv_encap_command *cmd)
{
	return CV_SUCCESS;
}

/* to check availability of command from host */
cv_status
tpmCvPollCmdRcvd(void)
{
	return CV_SUCCESS;
}

/* inform TPM that monotonic counter bumped */
void
tpmMonotonicCounterUpdated(void)
{
	;
}

/* inform CV that TPM bumped monotonic counter */
void
cvMonotonicCounterUpdated(void)
{
	;
}

/*
 * fingerprint device
 */
cv_status
fpRegCallback(cv_callback callback, cv_callback_ctx context)
{
	return CV_SUCCESS;
}

cv_status
fpCapture(uint8_t **image, uint32_t *length)
{
	return CV_SUCCESS;
}

#define TRACE_SC(x...)
//#define TRACE_SC(x...) printf(x)

/*
 * smart card device
 */
cvsc_status
scDeactivateCard(void)
{
        cv_status status = CVSC_OK;
#ifdef PHX_REQ_SCI
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;
		if (!scIsCardPresent()) return CVSC_NOT_PRESENT;
		status = BSCD_Channel_Deactivate(in_channelHandle);
		if (status != CVSC_OK)
			status = CVSC_FAILED;
#endif
        return status;
}

cv_status
scGetObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *obj)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;

		TRACE_SC("enter scGetObj\n");

        if (!scIsCardPresent()) return CVSC_NOT_PRESENT;

        /* Map CV object with smart card private object */
        BSCD_Channel_APDU_Get_Objects(in_channelHandle, objID, objIDLen);
		/* Removed the below dead code to fix coverity issue */
        /*if (status != CVSC_OK) return CVSC_FAILED;*/

        /* Verify PIN */
        status = BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_PIN_VERIFY, PIN, PINLen);
        if (status != CVSC_OK) return CVSC_FAILED;

        /* Read object */
        /* status = */BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_READ_DATA, obj, *objLen);
        /* Removed the below dead code to fix coverity issue */
		/*if (status != CVSC_OK) return CVSC_FAILED;*/

		TRACE_SC("read obj ok, len=%d\n", objLen);

#endif
        return CVSC_OK;
}

cv_status
scPutObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t objLen, uint8_t *obj)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;

		TRACE_SC("enter scPutObj\n");

        if (!scIsCardPresent()) return CVSC_NOT_PRESENT;

        /* Verify PIN */
        status = BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_PIN_VERIFY, PIN, PINLen);
        if (status != CVSC_OK) return CVSC_FAILED;

        /* Map CV object with smart card private object */
        status = BSCD_Channel_APDU_Set_Objects(in_channelHandle, objID, objIDLen);
        if (status != CVSC_OK) return CVSC_FAILED;

 		TRACE_SC("set obj ok, IDlen=%d\n", objIDLen);

        /* Write object */
        status = BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_WRITE_DATA, obj, objLen);
        if (status != CVSC_OK) return CVSC_FAILED;

 		TRACE_SC("write obj ok, len=%d\n", objLen);
#endif
        return CVSC_OK;
}

cv_status
scDelObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;

		TRACE_SC("enter scDelObj\n");

        /* Map CV object with smart card private object */
        status = BSCD_Channel_APDU_Get_Objects(in_channelHandle, objID, objIDLen);
        if (status != CVSC_OK) return CVSC_FAILED;

        /* Verify PIN */
        status = BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_PIN_VERIFY, PIN, PINLen);
        if (status != CVSC_OK) return CVSC_FAILED;

        /* Delete object */
        status = BSCD_Channel_APDU_Commands(in_channelHandle, BSCD_APDU_DELETE_FILE, NULL, NULL);
        if (status != CVSC_OK) return CVSC_FAILED;

 		TRACE_SC("del obj ok\n");
#endif
        return CVSC_OK;
}

/* for card detect */
bool
scIsCardPresent(void)
{
#ifdef PHX_REQ_SCI
/*
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;
        BSCD_ChannelSettings    channelSettings;

		TRACE_SC("enter scIsCardPresent\n");

		BSCD_GetChannelDefaultSettings(NULL, 0, &channelSettings);
        BSCD_Channel_SetParameters(in_channelHandle, &channelSettings);
        BSCD_Channel_ResetIFD(in_channelHandle, BSCD_ResetType_eCold);
        BSCD_Channel_ResetCard(in_channelHandle, BSCD_ResetCardAction_eReceiveAndDecode);

       return BSCD_Channel_DetectCardNonBlock(in_channelHandle, BSCD_CardPresent_eRemoved);
*/

        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;
        BSCD_ChannelSettings    channelSettings;
        BERR_Code errCode;

		TRACE_SC("enter scIsCardPresent\n");

        errCode = BSCD_Channel_DetectCardNonBlock(in_channelHandle, BSCD_CardPresent_eInserted);
        if (errCode == BERR_SUCCESS)
        {
			if (BSCD_Channel_IsCardActivated(in_channelHandle) == false)
			{
				errCode = BSCD_GetChannelDefaultSettings(NULL, 0, &channelSettings);
				if (errCode == BERR_SUCCESS)
					errCode = BSCD_Channel_SetParameters(in_channelHandle, &channelSettings);
				if (errCode == BERR_SUCCESS)
					errCode = BSCD_Channel_ResetIFD(in_channelHandle, BSCD_ResetType_eCold);
				if (errCode == BERR_SUCCESS)
					errCode = BSCD_Channel_ResetCard(in_channelHandle, BSCD_ResetCardAction_eReceiveAndDecode);

				if (errCode != BERR_SUCCESS)
					return false;
			}

			/* PPS might not be done while card already been activated in ccid, e.g. cv comes in between ccid PowerOn and SetP */
			if (BSCD_Channel_IsPPSDone(in_channelHandle) == false)
			{
				errCode = BSCD_Channel_PPS(in_channelHandle);
				if (errCode == BERR_SUCCESS)
				{
					errCode = BSCD_Channel_SetParameters(in_channelHandle, &(in_channelHandle->negotiateChannelSettings));
					if (errCode == BERR_SUCCESS)
					{	printf("cv: PPS ok, SetP ok\n"); }
					else
					{	printf("cv: PPS ok, SetP fail\n");}
				}
				else
					printf("cv: PPS fail\n");
			}

			return true;
		}

		return false;

#endif
}

cvsc_status
scGetCardType(cvsc_card_type *cardType)
{
        cv_status status = CVSC_OK;
#ifdef PHX_REQ_SCI
        BSCD_ChannelHandle in_channelHandle = (BSCD_ChannelHandle)VOLATILE_MEM_PTR->gchannelHandle;
        uint32_t scType;
	BSCD_Get_CardType(in_channelHandle, &scType);

        switch (scType) {
                case BSCD_CardType_eJAVA:
                     *cardType = CVSC_DELL_JAVA_CARD;
			printf("CV: extintf:enum JAVA card\n");
                     break;

                case BSCD_CardType_ePKI:
                     *cardType = CVSC_PKI_CARD;
			printf("CV: extintf:enum PKI card\n");
                     break;
                case BSCD_CardType_ePIV:
                     *cardType = CVSC_PIV_CARD;
			printf("CV: extintf:enum PIV card\n");
                     break;
                case BSCD_CardType_eCAC:
                     *cardType = CVSC_CAC_CARD;
			printf("CV: extintf:enum CAC card\n");
                     break;
                /*
                case BSCD_CardType_eACOS:
                     *cardType = CVSC_ACOS_CARD;
                     break;
                */
                default:
			printf("CV: extintf:enum failed\n");
                     status = CVSC_FAILED;
        }

	printf("scType %08x status %08x\n", scType, status);

		/* double check in the failure case to see if we are dell java card, since different card can be used */
		while ((status == CVSC_FAILED) && scIsCardPresent())
		{
			cv_status sc_status = CVSC_OK;
			uint8_t strPubData[JAVA_PIN_LENGTH]; /* anylen would do, this is not pin */
			uint16_t dataSize;

			sc_status =  phx_sc_java_select_applet();
			if (sc_status != CVSC_OK) break;

			sc_status = phx_sc_java_get_data_size(JAVA_INS_GET_PUBLIC_DATA_SIZE,  &dataSize);
			if (sc_status != CVSC_OK) break;

			/* if we reach here we are java card */
			BSCD_Set_CardType(in_channelHandle, BSCD_CardType_eJAVA);

			*cardType = CVSC_DELL_JAVA_CARD;
			status = CVSC_OK;
			printf("CV: extintf: JAVA card\n");

			break;
		}

		if (status == CVSC_FAILED) {
			printf("not java/pki card.\n"); 
			
			if (scIsCardPresent()) {
				status = scSelectDefaultApplet_PIV();
				if (status == PHX_STATUS_OK) {
					printf("CV: extintf: PIV card\n");
					BSCD_Set_CardType(in_channelHandle, BSCD_CardType_ePIV);

					*cardType = CVSC_PIV_CARD;
					status = CVSC_OK;
					goto return_exit;
				} else {
					printf("CV: extintf: not PIV card\n");
                 			status = CVSC_FAILED;
					/* fall through */
				} 

				status = scSelectDefaultApplet_CAC();
				if (status == PHX_STATUS_OK) {
					printf("CV: extintf: CAC card\n");
					BSCD_Set_CardType(in_channelHandle, BSCD_CardType_eCAC);

					*cardType = CVSC_CAC_CARD;
					status = CVSC_OK;
					goto return_exit;
				} else {
					printf("CV: extintf: not CAC card\n");
                     			status = CVSC_FAILED;
					/* fall through */
				} 
			}
		}


#endif

return_exit:
        return status;
}

/* for Dell Java applet smart cards */
cvsc_status
scGetUserID(uint32_t *userIDLen, uint8_t *userID)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        cvsc_card_type cardType;
        uint16_t dataSize;

		TRACE_SC("enter scGetUserID\n");

        *userIDLen = 0;
        if (!scIsCardPresent()) return CVSC_NOT_PRESENT;

        status = scGetCardType(&cardType);
        if ((status != CVSC_OK) || (cardType != CVSC_DELL_JAVA_CARD)) return CVSC_FAILED;

        status =  phx_sc_java_select_applet();
        if (status != CVSC_OK) return CVSC_FAILED;

        status = phx_sc_java_get_data_size(JAVA_INS_GET_PUBLIC_DATA_SIZE,  &dataSize);
        if (status != CVSC_OK) return CVSC_FAILED;

        status = phx_sc_java_read_file(JAVA_INS_GET_PUBLIC_DATA, userID, dataSize);
        if (status != CVSC_OK) return CVSC_FAILED;

		TRACE_SC("read file ok size=%d, id=%x %x %x %x %x\n", dataSize, userID[0], userID[1], userID[2], userID[3], userID[4]);

        *userIDLen = dataSize;

        return status;
#endif
}

cvsc_status
scGetUserKey(uint32_t *userKeyLen, uint8_t *userKey, uint32_t PINLen, uint8_t *PIN)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        cvsc_card_type cardType;
		uint8_t localPIN[JAVA_PIN_LENGTH];

		TRACE_SC("enter scGetUserKey\n");

        *userKeyLen = 0;
        if (!scIsCardPresent()) return CVSC_NOT_PRESENT;

        status = scGetCardType(&cardType);
        if ((status != CVSC_OK) || (cardType != CVSC_DELL_JAVA_CARD)) return CVSC_FAILED;

        status =  phx_sc_java_select_applet();
        if (status != CVSC_OK) return CVSC_FAILED;

		/* make sure PIN supplied */
		if (PINLen == 0 || PINLen > JAVA_PIN_LENGTH)
			return CVSC_PIN_REQUIRED;
		/* PIN length must be 8.  if < 8, zero fill */
		if (PINLen < JAVA_PIN_LENGTH)
		{
			memset(localPIN,0,JAVA_PIN_LENGTH);
			memcpy(localPIN, PIN, PINLen);
			PIN = &localPIN[0];
			PINLen = JAVA_PIN_LENGTH;
		}
		status = phx_sc_java_verify_PIN(PIN, PINLen);
        if (status != CVSC_OK) {
            if (status == SC_JAVA_STATUS_ERR_PIN_INCORRECT) {
                return CVSC_PIN_REQUIRED;
            } else {
                return CVSC_FAILED;
            }
        }

        status = phx_sc_java_get_key(userKey,  userKeyLen);
        if (status != CVSC_OK) return CVSC_FAILED;

		TRACE_SC("get key ok size=%d, key=%x %x %x %x %x\n", *userKeyLen, userKey[0], userKey[1], userKey[2], userKey[3], userKey[4]);

        return status;
#endif
}

cvsc_status
scGetPrivateData(uint32_t *dataLen, uint8_t *data, uint32_t PINLen, uint8_t *PIN)
{
#ifdef PHX_REQ_SCI
        cv_status status = CVSC_OK;
        cvsc_card_type cardType;
        uint16_t dataSize;
	uint8_t localPIN[JAVA_PIN_LENGTH];

	TRACE_SC("enter scGetPrivateData\n");

        *dataLen = 0;
        if (!scIsCardPresent()) return CVSC_NOT_PRESENT;

        status = scGetCardType(&cardType);
        if ((status != CVSC_OK) || (cardType != CVSC_DELL_JAVA_CARD)) return CVSC_FAILED;

        status =  phx_sc_java_select_applet();
        if (status != CVSC_OK) return CVSC_FAILED;

	/* PIN length must be 8.  if < 8, zero fill */
	if (PINLen < JAVA_PIN_LENGTH)
	{
		memset(localPIN,0,JAVA_PIN_LENGTH);
		memcpy(localPIN, PIN, PINLen);
		PIN = &localPIN[0];
		PINLen = JAVA_PIN_LENGTH;
	}

	status = phx_sc_java_verify_PIN(PIN, PINLen);
        if (status != CVSC_OK) {
            if (status == SC_JAVA_STATUS_ERR_PIN_INCORRECT) {
                return CVSC_PIN_REQUIRED;
            } else {
                return CVSC_FAILED;
            }
        }

        status = phx_sc_java_get_data_size(JAVA_INS_GET_PRIVATE_DATA_SIZE,  &dataSize);
        if (status != CVSC_OK) return CVSC_FAILED;

        status = phx_sc_java_read_file(JAVA_INS_GET_PRIVATE_DATA, data, dataSize);
        if (status != CVSC_OK) return CVSC_FAILED;

        *dataLen = dataSize;

	TRACE_SC("get privdata ok size=%d\n", dataSize);

        return status;
#endif
}

/* for PKI smart cards (CAC, PIV, Windows) - don't know which of these requires a PIN */
cvsc_status
scGetPubKey(uint32_t *pubKeyLen, uint8_t *pubKey)
{
	return CVSC_NOT_PRESENT;
}
cvsc_status
scChallenge(uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN)
{
	return CVSC_NOT_PRESENT;
}

#if 0
cvsc_status
scGetPIVCardCHUID(uint32_t *len, uint8_t *chuid)
{
	return CVSC_NOT_PRESENT;
}

cvsc_status
scGetCACCardCert(uint32_t *len, uint8_t *cert)
{
	return CVSC_NOT_PRESENT;
}

cvsc_status
scCACChallenge(cvsc_sign_alg signAlg, uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN)
{
	return CVSC_NOT_PRESENT;
}

cvsc_status
scPIVChallenge(cvsc_sign_alg signAlg, uint32_t challengeLen, uint8_t *challenge, uint32_t *responseLen, uint8_t *response, uint32_t PINLen, uint8_t *PIN)
{
	return CVSC_NOT_PRESENT;
}
#endif

/*---------------------------------------------------------------
 * Name    : sc_complex_diag
 * Purpose : Do sc read/write test on dell java card
 * Input   : numInputParams: number of input parameters
 *           inputparam: pointer to a word array of input parameter
 *                       The first parameter is the rfid protocol that need to be tested
 *           outLength: Caller set to max buffer length when passed in; This function set to used length when passed out
 *           outBuffer: pointer to a buffer for output
 * Return  : cv_status
 * Note    :
 *--------------------------------------------------------------*/
cv_status sc_complex_diag(uint32_t numInputParams, uint32_t **inputparam, uint32_t *outLength, uint32_t *outBuffer)
{
#define MAXPUBDATALEN 255
	cv_status status = CVSC_OK;
	uint32_t  pubDataLen = 0;
	uint8_t   pubData[MAXPUBDATALEN];
	uint8_t   pubDataSample1[] = "0123456789";
	uint8_t   pubDataSample2[] = "9876543210";
	uint8_t   *pubDataUsed;


//	if (numInputParams == 0) return CV_INVALID_INPUT_PARAMETER_LENGTH;

	/* do simple test first, to make sure the power is on */
	status = BSCD_USH_SelfTest();
	if (status != CVSC_OK) return CVSC_FAILED;

	/* read out public data */
	status = scGetUserID(&pubDataLen, pubData);
	if (status != CVSC_OK) return CVSC_FAILED;

	if (pubDataLen > MAXPUBDATALEN)	return CVSC_FAILED;

	/* write public data */
	if (memcmp(pubData, pubDataSample1, sizeof(pubDataSample1)))
		pubDataUsed = pubDataSample1;
	else
		pubDataUsed = pubDataSample2; /* use different data */

	status = phx_sc_java_set_data_size(JAVA_INS_SET_PUBLIC_DATA_SIZE, sizeof(pubDataSample1));
	if (status != CVSC_OK) return CVSC_FAILED;

	status = phx_sc_java_write_file(JAVA_INS_UPDATE_PUBLIC_DATA, pubDataUsed, sizeof(pubDataSample1));
	if (status != CVSC_OK) return CVSC_FAILED;

	/* read out public data again */
	memset(pubData, 0, pubDataLen);
	status = scGetUserID(&pubDataLen, pubData);
	if (status != CVSC_OK) return CVSC_FAILED;

	/* compare */
	if ((pubDataLen != sizeof(pubDataSample1)) || memcmp(pubDataUsed, pubData, pubDataLen))
	{
		status = CVSC_FAILED;
	}

	return status;
}

/*
 * RF device
 */
cv_status
rfGetObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *obj)
{
	return CV_SUCCESS;
}

cv_status
rfPutObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN, uint32_t objLen, uint8_t *obj)
{
	return CV_SUCCESS;
}

cv_status
rfDelObj(uint32_t objIDLen, char *objID, uint32_t PINLen, uint8_t *PIN)
{
	return CV_SUCCESS;
}
BERR_Code
rfGetChannelHandle(BRFD_ChannelHandle *pchannelHandle)
{
#if 0 /* these are for CCID CL not enabled case */
    BERR_Code result;

	/* Initialise the heap */
	ushxInitHeap(HID_LARGE_HEAP_SIZE, HID_LARGE_HEAP_PTR);

	result = BRFD_Open( pchannelHandle, HID_LARGE_HEAP_PTR );

    if ( BERR_SUCCESS == result ) {
        result = BRFD_Channel_Open( *pchannelHandle, HID_LARGE_HEAP_PTR );
	}

	return result;
#else

	*pchannelHandle = hid_glb_ptr;

	return BERR_SUCCESS;
#endif

}

/* This function returns BERR_SUCCESS(e.g. 0) when a PIV or HID card is in the field, otherwise returns 2
   This function expects the regular timer routine is called (HIDEvent handling) which is enabled in CCID CL code.
 */
bool
rfIsCardPresent(void)
{
	uint32_t  i;
	BERR_Code rfVal;
	BRFD_ChannelHandle channelHandle;

	bool result = FALSE, alreadyInCVOnlyRadioModeOverride = FALSE;
	/* first determine if in cv_only radio mode.  if so, need to enable radio here. */
	/* check to see if already in cv-only radio mode override.  this could happen for rfIsCardPresent called within rfGetHIDCredential */
	/* if so, don't want to disable at end */
	if (CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_OVERRIDE)
		alreadyInCVOnlyRadioModeOverride = TRUE;
	if (IS_CV_ONLY_RADIO_MODE())
		CV_VOLATILE_DATA->CVDeviceState |= CV_CV_ONLY_RADIO_OVERRIDE;

	if (rfGetChannelHandle(&channelHandle) == BERR_SUCCESS)
	{
#if 0  /* this is time consuming */
		for (i = 0; i < 10; i++)
		{

			rfVal = BRFD_Channel_DetectCardNonBlock(channelHandle, BRFD_CardPresent_eInserted);
			if (rfVal == BERR_SUCCESS)
	        {
				/* do an extra resetcard in order to get rid of false positive */
				rfVal = BRFD_Channel_ResetCard(channelHandle, BRFD_ResetType_eCV);
				if (rfVal == BERR_SUCCESS)
				{
		        	result = TRUE;
					goto err_exit;
				}
			}
	        /* HID code requires 20ms delay in between its calls */
	        yield_and_delay(30);

	        /* need to call HIDEvent() for HID code to detect if card is present */
	        ushxHIDEvent();
		}
#else
		/* this function will loop through protocol inside so we don't need to do loop here */
		if(HID_IsCardPresent(channelHandle) == TRUE)
		{
			result= TRUE;
		}
		else
		{
			rfVal = BRFD_Channel_ResetCard(channelHandle, BRFD_ResetType_eCV);
			if (rfVal == BERR_SUCCESS)
				result = TRUE;
		}
#endif
	}

err_exit:
	/* determine if in cv_only radio mode.  if so, need to disable radio here (unless already in override mode */
	/* in which case it will get disabled in the calling function) */
	if ((IS_CV_ONLY_RADIO_MODE()) && !alreadyInCVOnlyRadioModeOverride)
	{
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_OVERRIDE;
		if (!isNFCPresent())
		{
#ifdef PHX_REQ_RFIDHF
			rfid_carrier_control(RFID_TXPOWER_OFF);
#endif

			BRFD_Notify_CarrierOff(channelHandle);
		}
	}

    return result;
}


BERR_Code
rfGetChannelSettings(BRFD_ChannelSettings **channelSettings)
{
	return BERR_SUCCESS;
}

cv_status
rfGetHIDCredential(uint32_t PINLen, uint8_t *PIN, uint32_t *objLen, uint8_t *obj, cv_contactless_type *type)
{
	/* this is a wrapper routine for the HID routine that reads the HID credential */
	cv_status retVal = CV_CONTACTLESS_FAILURE;
	BRFD_ChannelHandle channelHandle;
	BRFD_ChannelSettings *channelSettings;
	unsigned int  credentialType;
	unsigned char PINRequired = 0;
	BERR_Code rfRetVal = !BERR_SUCCESS;
	uint32_t maxPayloadSize = *objLen;
	uint32_t i;

	/* first determine if in cv_only radio mode.  if so, need to enable radio here */
	if (IS_CV_ONLY_RADIO_MODE())
		CV_VOLATILE_DATA->CVDeviceState |= CV_CV_ONLY_RADIO_OVERRIDE;

	/* first get the channel info */
	if (rfGetChannelHandle(&channelHandle) != BERR_SUCCESS)
		goto err_exit;

	if (rfGetChannelSettings(&channelSettings) != BERR_SUCCESS)
		goto err_exit;

	/* check card present again, this is to work around the issue of process card fails if called twice */
	if (rfIsCardPresent())
	{
		/* we need to reserve the interface first */
		rfRetVal = cl_interface_reserve(channelHandle);

		if (rfRetVal == BERR_SUCCESS)
		{
			/* calls it first without PIN */
			rfRetVal = BRFD_Process_Card(channelHandle, channelSettings, obj, objLen, &credentialType,
				maxPayloadSize, NULL, 0, &PINRequired);

			// check if card didn't respond to check command.
			if (rfRetVal == BERR_SUCCESS && credentialType == BRFD_PAYLOAD_NONE) {
				// card did not respond to check command, unable to get credential type
				// HID says try again.
				printf("rfGetHidCredential() invalid credentialType calling BRFD_Process_Card again\n");
				rfRetVal = BRFD_Process_Card(channelHandle, channelSettings, obj, objLen, &credentialType,
						maxPayloadSize, NULL, 0, &PINRequired);
				//printf("rfGetHidCredential() BRFD_Process_Card: 0x%x, credentialType: 0x%x\n", rfRetVal, credentialType);
				if (rfRetVal != BERR_SUCCESS || credentialType == BRFD_PAYLOAD_NONE) {
					printf("rfGetHidCredential() communication error\n");
					retVal = CV_CONTACTLESS_COM_ERR;
					/* we need to release the interface */
					cl_interface_release(channelHandle);
					goto err_exit;
				}
			}

//			if (rfRetVal != BERR_SUCCESS)
//				rfRetVal = BRFD_Process_Card(channelHandle, channelSettings, obj, (unsigned long *)objLen, &credentialType,
//					maxPayloadSize, NULL, 0, &PINRequired);

			/* calls it second time with PIN if the first time failed */
			if ((rfRetVal != BERR_SUCCESS) && PINRequired && PINLen)
				rfRetVal = BRFD_Process_Card(channelHandle, channelSettings, obj, objLen, &credentialType,
					maxPayloadSize, PIN, PINLen, &PINRequired);

			printf("RFID: process card : %x\n", rfRetVal);

			/* we need to release the interface */
			/* Note do not take rfRetVal otherwise it corrupts the value */
			cl_interface_release(channelHandle);
		}
	}
	else {
		printf("rfGetHidCredential() !rfIsCardPresent - communication error\n");
		retVal = CV_CONTACTLESS_COM_ERR;
		goto err_exit;
	}

	if (rfRetVal != BERR_SUCCESS)
	{
		if (PINRequired)
			retVal = CV_PROMPT_PIN_AND_CONTACTLESS;
		goto err_exit;
	}

	switch (credentialType)
	{
	case BRFD_PAYLOAD_HID_ACCESS:
		*type = CV_CONTACTLESS_TYPE_HID;
		break;
	case BRFD_PAYLOAD_PIV_CHUID:
		*type = CV_CONTACTLESS_TYPE_PIV;
		break;
	case BRFD_PAYLOAD_MIFARE_CLASSIC_UID:
		*type = CV_CONTACTLESS_TYPE_MIFARE_CLASSIC;
		break;
	case BRFD_PAYLOAD_MIFARE_ULTRALIGHT_UID:
		*type = CV_CONTACTLESS_TYPE_MIFARE_ULTRALIGHT;
		break;
	case BRFD_PAYLOAD_MIFARE_DESFIRE_UID:
		*type = CV_CONTACTLESS_TYPE_MIFARE_DESFIRE;
		break;
	case BRFD_PAYLOAD_FELICA_IDM:
		*type = CV_CONTACTLESS_TYPE_FELICA;
		break;
	case BRFD_PAYLOAD_ISO15693_UID:
		*type = CV_CONTACTLESS_TYPE_ISO15693;
		break;
	default:
		retVal = CV_INVALID_CONTACTLESS_CARD_TYPE;
		goto err_exit;
	}

	retVal = CV_SUCCESS;

err_exit:
	/* determine if in cv_only radio mode.  if so, need to disable radio here */
	if (IS_CV_ONLY_RADIO_MODE())
	{
		CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_OVERRIDE;
#ifdef PHX_REQ_RFIDHF
		rfid_carrier_control(RFID_TXPOWER_OFF);
#endif

#if 0
		for (i = 0; i<30; i++)
		{
	        /* HID code requires 20ms delay in between its calls */
	        yield_and_delay(30);

	        /* need to call HIDEvent() for HID code to detect if card is present */
	        ushxHIDEvent();
		}
#else
		 BRFD_Notify_CarrierOff(channelHandle);
#endif

	}
	return retVal;
}

cv_status
cv_get_session_count (cv_handle cvHandle, uint32_t *sessionCount)
{
	return CV_SUCCESS;
}


/*
 * Cryptographic Routines
 */

cv_status
cv_unwrap_dlies_key (cv_handle cvHandle, uint32_t numKeys,
						cv_obj_handle *keyHandleList,
						uint32_t authListLength, cv_auth_list *pAuthList,
						uint32_t messageLength, byte *pMessage,
						cv_obj_handle newKeyHandle, cv_bool *pbUserKeyFound,
						cv_callback *callback, cv_callback_ctx context)
{
	return CV_SUCCESS;
}

#ifndef ARM926EJ_S

/**********************************************************************
 *  SCAPI Function Prototypes
 **********************************************************************/

PHX_STATUS phx_dsa_key_generate_1 (crypto_lib_handle *pHandle, uint8_t *privkey, uint8_t *pubkey, uint32_t bPrivValid)
{
	return PHX_STATUS_OK;
}
#endif

#if 0
uint8_t genTokencode(uint8_t *isoTime, uint8_t *masterSeed,
					uint8_t *serialNumber,
					uint8_t displayDigits, uint8_t displayIntervalSeconds, uint32_t *result)
{
	return 0;
}
#endif

/* Used by RSA library */
int RSAmod(uint8_t *xmem, int message, int modulus)
{
	return message % modulus;
}

/* Used by RSA library */
void aesEncryptECB(uint8_t *xmem, uint8_t *abBlock, const uint8_t *abKey)
{
	BCM_SCAPI_STATUS status=BCM_SCAPI_STATUS_OK; 
	uint8_t cryptoHandle[BCM_SCAPI_CRYPTO_LIB_HANDLE_SIZE]={0};

	bcm_crypto_init((crypto_lib_handle *)cryptoHandle);
	
	status = bcm_cust_aes_crypto((crypto_lib_handle *)cryptoHandle,
								 abBlock,AES_BLOCKBYTES, 
								 abKey,AES_KEYBYTES,
								 (uint8_t *)NULL, AES_BLOCKBYTES,
								 abBlock,
								 BCM_SCAPI_CIPHER_MODE_ENCRYPT,
								 BCM_SCAPI_ENCR_MODE_ECB);
	if (status != BCM_SCAPI_STATUS_OK)
	{
		printf("%s:%d bcm_cust_aes_crypto status= %d \n",__FUNCTION__,__LINE__,status);
	}
}


/**********************************************************************
 *  FP Device Routines
 **********************************************************************/

/* TBD - this is needed for linking with DP library.  don't know if it's permanent yet or not */
unsigned char * __aeabi_vec_ctor_nocookie_nodtor(unsigned char * start, void (*ctor)(unsigned char *), size_t size, size_t count) {
	unsigned char *cur;
  if (ctor) {
    for (cur = start; count--; cur += size) {
      ctor(cur);
    }
  }
  return start;
}

/* DP stubs */
#ifdef LIB_COMPILE_DP_STUBS
DPFR_RESULT DPFRCreateContext(
  DPFR_CONTEXT_INIT * contextInit,  ///< [in] pointer to filled context initialization structure
  DPFR_HANDLE_PT phContext          ///< [out] pointer to where to put an open handle to created context
)
{
	return FR_OK;
}

DPFR_RESULT DPFRGetMemoryRequirements(
  size_t sizeOfRequirements,                 ///< [in] Size of the requirements structure to fill
  DPFR_MEMORY_REQUIREMENTS * requirements    ///< [out] Pointer to structure where to put memory requirements
)
{
	return FR_OK;
}

DPFR_RESULT DPFRImport(
  DPFR_HANDLE hContext,        ///< [in] Handle to a fingerprint recognition context
  const unsigned char data[],  ///< [in] (fingerprint) data to import
  size_t size,                 ///< [in] size of the (fingerprint) data
  DPFR_DATA_TYPE dataType,     ///< [in] type and format of the (fingerprint) data
  DPFR_PURPOSE purpose,        ///< [in] purpose of the processed sample, for example enrollment or fingerprint recognition
  DPFR_HANDLE_PT pHandle       ///< [out] pointer to where to put an open handle to the fingerprint data
)
{
	return FR_OK;
}

DPFR_RESULT DPFRGetVerificationResult(
  DPFR_HANDLE      hComparisonOper,     ///< [in] Handle to comparison operation
  DPFR_PROBABILITY targetFalseAccept   ///< [in] Target false accept probability
)
{
	return FR_OK;
}

DPFR_RESULT DPFRAddFeatureSetToEnrollment(
  DPFR_HANDLE hEnrollment,          ///< [in] Handle to an enrollment operation
  DPFR_HANDLE hFeatureSet           ///< [in] Handle to feature set to include in enrollment
)
{
	return FR_OK;
}

DPFR_RESULT DPFRCloseHandle(
  DPFR_HANDLE handle            ///< [in] Handle to close. Object is freed when the reference count is zero
)
{
	return FR_OK;
}
DPFR_RESULT DPFRCreateComparisonWithAlignment(
  DPFR_HANDLE hFtrSet, 
  DPFR_HANDLE hTemplate, 
  DPFR_HANDLE hAlignment, 
  unsigned int reserved,
  DPFR_HANDLE_PT  phComparisonOper
)
{
	return FR_OK;
}

DPFR_RESULT DPFRCreateEnrollment(
  DPFR_HANDLE hContext,          ///< [in] Handle to a fingerprint recognition context
  unsigned int reserved,         ///< [in] Enrollment options : 0 or a combination of DPFR_ENROLLMENT_XXX defines
  DPFR_HANDLE_PT phEnrollment    ///< [out] pointer to where to put an open handle to an enrollment operation
)
{
	return FR_OK;
}
DPFR_RESULT DPFRCreateFeatureSetInPlace(
  DPFR_HANDLE hContext,          ///< [in] Handle to a fingerprint recognition context
  unsigned char fpData[],        ///< [in] fingerprint sample, buffer is overridden during feature extraction to save memory
  size_t size,                   ///< [in] size of the sample buffer
  DPFR_DATA_TYPE dataType,       ///< [in] type of the sample, for instance image format
  DPFR_PURPOSE purpose,          ///< [in] purpose of the processed sample, for example enrollment or fingerprint recognition
  unsigned int reserved,        ///< [in] Reserved, set to 0
  DPFR_HANDLE_PT phFeatureSet    ///< [out] pointer to where to put an open handle to the feature set
)
{
	return FR_OK;
}

DPFR_RESULT DPFRCreateIdentificationFromStorage(
  DPFR_HANDLE hFeatureSet,               ///< [in] Handle to feature set to include in identification
  const DPFR_STORAGE * pStorage,         ///< [in] Storage interface and context 
  DPFR_PROBABILITY targetFPIR,           ///< [in] Target false positive identification rate
  DPFR_IDENTIFICATION_FLAGS idFlags,     ///< [in] Identification options (i.e. disable early out), set to 0 for default behavior
  DPFR_HANDLE_PT phIdentificationOper    ///< [out] Pointer to where to put the handle to identification operation
)
{
	return FR_OK;
}

DPFR_RESULT DPFRCreateTemplate(
  DPFR_HANDLE hEnrollment,          ///< [in] Handle to an enrollment operation (or comparison operation for the template adaptation)
  DPFR_PURPOSE purpose,             ///< [in] Purpose of the fingerprint template. Supported purposes that can be combined with a bitwise OR: DPFR_PURPOSE_VERIFICATION, DPFR_PURPOSE_IDENTIFICATION
  DPFR_HANDLE_PT phTemplate         ///< [out] Pointer where to put handle to a created fingerprint template
)
{
	return FR_OK;
}

DPFR_RESULT DPFRExport(
  DPFR_HANDLE handle,          ///< [in] Handle to data object to export
  DPFR_DATA_TYPE dataType,     ///< [in] Type and format of data to export
  unsigned char pbData[],      ///< [out] Buffer where to export the data, optional
  size_t * pcbData             ///< [in/out] Pointer where to store the length of exported data, optional
)
{
	return FR_OK;
}

DPFR_RESULT DPFRGetEnrollmentStatus(
  DPFR_HANDLE hEnrollment,          ///< [in] Handle to an enrollment operation
  DPFR_ENROLLMENT_STATUS * pStatus  ///< [out] Status of the enrollment operations
)
{
	return FR_OK;
}

DPFR_RESULT DPFRGetNextCandidate(
  DPFR_HANDLE hIdentificationOper,      ///< [in]  Handle to identification operation
  DPFR_TIME timeout,                    ///< [in]  A timeout value in milliseconds or DPFR_INFINITE
  DPFR_HANDLE_PT phCandidate            ///< [out] Pointer to where to put the handle to candidate object
)
{
	return FR_OK;
}

DPFR_RESULT DPFRGetSubjectID(
  DPFR_HANDLE hCandidate,                ///< [in] Handle to candidate object
  DPFR_SUBJECT_ID subjectID              ///< [out] ID of a fingerprint data subject associated with matched template
)
{
	return FR_OK;
}
#endif

#ifndef LIB_COMPILE_DP_STUBS
uint32_t dp_getversion_upper()
{
  DPFR_VERSION version = {0};
  uint32_t retValue = 0;
	
  DPFRGetLibraryVersion(&version);
  /*printf("DP Version: %d.%d\n", version.major, version.minor, version.revision, version.build);*/
  
  retValue = ((version.major & 0xFFFF) << 16) | (version.minor & 0xFFFF);
  return retValue;
}

uint32_t dp_getversion_lower()
{
  DPFR_VERSION version = {0};
  uint32_t retValue = 0;
	
  DPFRGetLibraryVersion(&version);
  /*printf("DP Version Lower: %d.%d\n", version.revision, version.build);*/
  
  retValue = ((version.revision & 0xFFFF) << 16) | (version.build & 0xFFFF);
  return retValue;
}
#endif

/**********************************************************************
 *  Diagnostics Routines
 **********************************************************************/

cv_status
ush_diag(uint32_t subcmd, uint32_t param1, uint32_t numInputParams, uint32_t **inputparam, uint32_t *outputparam, uint32_t *outputLen)
{
	cv_status status, ret_status = CV_SUCCESS;
	uint32_t  bitval = 0;
    uint32_t tests=1;
    uint32_t results=0;
    uint32_t extraDataLength=0;
    uint8_t *pExtraData=NULL;
    fp_status fpStatus;

	*outputLen = 1024; /* give an arbitary number for max buffer length. The actual buffer is 4k- */

	switch (subcmd)
	{
		/* perform group test */
		case CV_DIAG_CMD_TEST_GRP:

#ifdef PHX_REQ_SCI
			if (param1 & CV_DIAG_TEST_GRP_SMARTCARD)
			{
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
					status = CV_SC_NOT_ENABLED;
				}
				else
					status = BSCD_USH_SelfTest();
				bitval |= (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_SMARTCARD : 0;
			}
#endif

#ifdef PHX_REQ_FP
			if (param1 & CV_DIAG_TEST_GRP_FINGERPRINT)
			{
				/* best would be to use function cv_fingerprint_test() but we don't have cvhandler here */

				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
					status = CV_FP_NOT_ENABLED;
				}
				else
				/* check to make sure FP device present */
				if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT))
					status = CV_FP_NOT_PRESENT;
				else
				{
					/* not allowed if FP image buf not available, which implies an async FP capture is active */
					if (!isFPImageBufAvail()) {
						status = CV_FP_DEVICE_BUSY;
					} else {
						fpStatus = fpTest(tests, &results, extraDataLength, pExtraData);
						status = cvXlatefpRetCode(fpStatus);
					}
				}

				bitval |= (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_FINGERPRINT : 0;
			}
#endif

#ifdef PHX_REQ_RFIDHF
			if (param1 & CV_DIAG_TEST_GRP_RFID)
			{
				status = (rfIsCardPresent()==TRUE)? CV_SUCCESS : CV_DIAG_FAIL;
				bitval |=  (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_RFID : 0;
			}
#endif

#ifdef PHX_REQ_USBH
			if (param1 & CV_DIAG_TEST_GRP_USB_HOST)
			{
				status = usbhost_simple_diag();
				bitval |= (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_USB_HOST : 0;
			}
#endif
#ifdef PHX_REQ_TPM
#ifndef PHX_NREQ_TPM
			if (param1 & CV_DIAG_TEST_GRP_TPM)
			{
				status = tpm_connectivity_test();
				bitval |= (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_TPM : 0;
			}
#endif
#endif

			if (param1 & CV_DIAG_TEST_GRP_CV)
			{
				status = ush_bootrom_core_selftest();
				bitval |= (status == CV_SUCCESS) ? CV_DIAG_TEST_GRP_CV : 0;
			}

			*outputparam = bitval;
			*outputLen = sizeof(uint32_t);

			break;

		/* perform individual test */
		case CV_DIAG_CMD_TEST_IND:
			switch (param1)
			{
				case CV_DIAG_TEST_IND_CV:
					/* note *inputparam should be inputparam as used in rfid_param_diag, otherwise only inputparam[0] works */
					ret_status = ush_bootrom_core_tests(numInputParams, *inputparam, outputLen, outputparam);
					break;

#ifdef PHX_REQ_RFIDHF
				case CV_DIAG_TEST_IND_RFID:
				{
					BRFD_ChannelHandle channelHandle;
					bool alreadyInCVOnlyRadioModeOverride = FALSE;

					/* --- this piece of code copied from rfIsCardPresent() ----*/
					/* first determine if in cv_only radio mode.  if so, need to enable radio here. */
					/* check to see if already in cv-only radio mode override.  this could happen for rfIsCardPresent called within rfGetHIDCredential */
					/* if so, don't want to disable at end */
					if (CV_VOLATILE_DATA->CVDeviceState & CV_CV_ONLY_RADIO_OVERRIDE)
						alreadyInCVOnlyRadioModeOverride = TRUE;
					if (IS_CV_ONLY_RADIO_MODE())
						CV_VOLATILE_DATA->CVDeviceState |= CV_CV_ONLY_RADIO_OVERRIDE;

					if (isNFCPresent())
					{
						ret_status = nci_test_individual_cards(numInputParams, *inputparam, outputLen, outputparam, 0);
					}
					else
					{
						/* note *inputparam should be inputparam as used in rfid_param_diag, otherwise only inputparam[0] works */
						ret_status = CV_DIAG_FAIL;
					}

					/* --- this piece of code copied from rfIsCardPresent() ----*/
					/* determine if in cv_only radio mode.  if so, need to disable radio here (unless already in override mode */
					/* in which case it will get disabled in the calling function) */
					if ((IS_CV_ONLY_RADIO_MODE()) && !alreadyInCVOnlyRadioModeOverride)
					{
						CV_VOLATILE_DATA->CVDeviceState &= ~CV_CV_ONLY_RADIO_OVERRIDE;
						if (!isNFCPresent())
						{
							rfid_carrier_control(RFID_TXPOWER_OFF);

							if (rfGetChannelHandle(&channelHandle) == BERR_SUCCESS)
								BRFD_Notify_CarrierOff(channelHandle);
						}
					}

					break;

				}
#endif

#ifdef PHX_REQ_SCI
				case CV_DIAG_TEST_IND_SMARTCARD:
					if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
						status = CV_SC_NOT_ENABLED;
					}
					else
						status = BSCD_USH_SelfTest();
						
					*outputparam = status;
					*outputLen = sizeof(uint32_t);
					break;

				case CV_DIAG_TEST_IND_SMARTCARD_JC:
					if (!(CV_PERSISTENT_DATA->deviceEnables & CV_SC_ENABLED)) {
						status = CV_SC_NOT_ENABLED;
					}
					else
						status = sc_complex_diag(numInputParams, inputparam, outputLen, outputparam); /* do read/write test */
						
					*outputparam = status;
					*outputLen = sizeof(uint32_t);
					break;
#endif

#ifdef PHX_REQ_TPM
#ifndef PHX_NREQ_TPM
				case CV_DIAG_TEST_IND_TPM:
					status = tpm_connectivity_test(); /* use the simple test for the complex one */
					*outputparam = status;
					*outputLen = sizeof(uint32_t);
					break;
#endif
#endif

#ifdef PHX_REQ_FP
				case CV_DIAG_TEST_IND_FINGERPRINT:
					/* best would be to use function cv_fingerprint_test() but we don't have cvhandler here */

					if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_ENABLED)) {
						status = CV_FP_NOT_ENABLED;
					}
					else
					/* check to make sure FP device present */
					if (!(CV_PERSISTENT_DATA->deviceEnables & CV_FP_PRESENT) && !(CV_VOLATILE_DATA->fpSensorState & CV_FP_WAS_PRESENT))
						status = CV_FP_NOT_PRESENT;
					else
					{
						if (!isFPImageBufAvail()) {
							status = CV_FP_DEVICE_BUSY;
						} else {
							fpStatus = fpTest(tests, &results, extraDataLength, pExtraData);
							status = cvXlatefpRetCode(fpStatus);
						}
					}

					*outputparam = status;
					*outputLen = sizeof(uint32_t);
#endif
					break;
#if PHX_REQ_USBH
				case CV_DIAG_TEST_IND_USB_HOST:
					status = usbhost_complex_diag(&((cv_diag_cmd_ind_status *)outputparam)->params.usb);
					((cv_diag_cmd_ind_status *)outputparam)->test_status = status;
					*outputLen = sizeof(cv_diag_cmd_ind_status);
					break;
#endif
				default:
					ret_status = CV_INVALID_INPUT_PARAMETER;
			}
			break;

		/* this is for EMI testing to override HID settings of tx current & drive strength */
		case CV_DIAG_CMD_TEST_RFID_PARAMS:
			ret_status = CV_DIAG_FAIL;
			*outputparam = ret_status;
			*outputLen = sizeof(uint32_t);
			break;

		/* this is for production test */
		case CV_DIAG_CMD_TEST_RFID_PRODUCTION:
			{
				ret_status=  CV_DIAG_FAIL;
				*outputparam = ret_status;
				*outputLen = sizeof(uint32_t);
			}

			break;

		/* this is for EMI testing to override HID settings of tx current & drive strength */
		case CV_DIAG_CMD_GET_ANT_STATUS:
			ret_status = CV_DIAG_FAIL;
			break;

		case CV_DIAG_CMD_GET_RFID_STATUS:
			{
				ret_status=  CV_DIAG_FAIL;
				*outputparam = ret_status;
				*outputLen = sizeof(uint32_t);
				break;
			}
#ifdef HID_CNSL_REDIRECT
		/* this is to redirect HID's RFID test console output to USH diagnostics window */
		case CV_DIAG_CMD_GET_CONSOLE:
			if(ush_get_custid() < PRINT_DISABLE_CUST_ID) {
				*outputLen  = HIDRF_getDebugData(outputparam, param1);
			} else {
				ret_status = CV_INVALID_COMMAND;
			}
			break;
#endif

#ifdef PHX_REQ_HDOTP_DIAGS
		case CV_DIAG_CMD_HDOTP_GET_STATUS:
			cvGet2TandAHEnables((cv_enables_2TandAH *)outputparam);
			*outputLen = sizeof(uint32_t);
			break;

		case CV_DIAG_CMD_HDOTP_SET_STATUS:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				if (param1 == (CV_ENABLES_2T | CV_ENABLES_DA | CV_ENABLES_AH))
					cvSet2TandAHEnables((cv_enables_2TandAH) param1);
				else
					ret_status = CV_INVALID_COMMAND;
			}
			else {
				cvSet2TandAHEnables((cv_enables_2TandAH) param1);
			}
			break;
		/* Get value of HDOTP bit */
		case CV_DIAG_CMD_HDOTP_PEEK:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				uint32_t data_0, data_1, rowoffset;
			  	ret_status = nvm_init();
				if (ret_status != PHX_STATUS_OK) break;
				nvm_2totp_rdRow(param1, &data_0, &data_1);
				rowoffset = param1 >> 6;
				*outputparam = (rowoffset > 31) ? (data_1 >> (rowoffset-32)) & 1 : (data_0 >> rowoffset) & 1;
				*outputLen = sizeof(uint32_t);
			}
			break;

		/* Set HDOTP bit */
		case CV_DIAG_CMD_HDOTP_POKE:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
		  		ret_status = nvm_init();
				if (ret_status != PHX_STATUS_OK) break;
				nvm_sv_2totp_programBit_WA(param1, 1, &ret_status);
			}
			break;

		case CV_DIAG_CMD_HDOTP_GET_SKIP_LIST:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cv_hdotp_skip_table *skip_table = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
				ret_status = cvReadHDOTPSkipTable(skip_table);
				if(ret_status != CV_SUCCESS) break;
				*outputLen = skip_table->tableEntries * sizeof(uint32_t);
				memcpy((uint8_t *)outputparam, (uint8_t *)skip_table->table, *outputLen);
			}
			break;

		case CV_DIAG_CMD_HDOTP_SET_SKIP_LIST:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cv_hdotp_skip_table *skip_table = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
				ret_status = cvReadHDOTPSkipTable(skip_table);
				if(ret_status != CV_SUCCESS) break;

				skip_table->tableEntries = param1 / sizeof(uint32_t);
				memcpy((uint8_t *)skip_table->table, (uint8_t *)*inputparam, param1);
				ret_status = cvWriteHDOTPSkipTable();
			}
			break;

		case CV_DIAG_CMD_HDOTP_GET_CV_CNTR:
		  	ret_status = nvm_init();
			if (ret_status != PHX_STATUS_OK) break;
			ret_status = get_monotonic_counter(outputparam);
			if(ret_status != CV_SUCCESS) break;
			*outputLen = sizeof(uint32_t);
			break;

		case CV_DIAG_CMD_HDOTP_GET_AH_CRDT_LVL:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cvGetAHCredits(outputparam);
				*outputLen = sizeof(uint32_t);
			}
			break;

		case CV_DIAG_CMD_HDOTP_GET_ELPSD_TIME:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cvGetElapsedTime(outputparam);
				*outputLen = sizeof(uint32_t);
			}
			break;

		case CV_DIAG_CMD_HDOTP_GET_SPEC_SKIP_CNTR:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				uint32_t ocnt;
				cv_hdotp_skip_table *skip_table = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
			  	ret_status = nvm_init();
				if (ret_status != PHX_STATUS_OK) break;
                //ret_status = get_monotonic_counter(&ocnt);
				//if(ret_status != CV_SUCCESS) break;
				ret_status = cvReadHDOTPSkipTable(skip_table);
				if(ret_status != CV_SUCCESS) break;
				//ret_status = specSkpNxtCntr(ocnt, outputparam, skip_table->table, skip_table->tableEntries);
				ret_status = specSkpNxtCntr(param1, outputparam, skip_table->table, skip_table->tableEntries);
				if(ret_status) {
					*outputLen = sizeof(uint32_t);
					ret_status = CV_SUCCESS;
				}
				else
					ret_status = 1; /* Some value other than 0, for failure */
			}
			break;

		case CV_DIAG_CMD_HDOTP_GET_SKIP_MONO_CNTR:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cv_hdotp_skip_table *skip_table = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
			  	ret_status = nvm_init();
				if (ret_status != PHX_STATUS_OK) break;
				ret_status = cvReadHDOTPSkipTable(skip_table);
				if(ret_status != CV_SUCCESS) break;
				ret_status = phx_get_skp_monotonic_counter(outputparam, skip_table->table, skip_table->tableEntries);
				if(ret_status) {
					ret_status = CV_SUCCESS;
					*outputLen = sizeof(uint32_t);
				}
				else
					ret_status = 1;	/* Some value other than 0, for failure */
			}
			break;

		case CV_DIAG_CMD_HDOTP_SET_SKIP_CNTR:
 			if(ush_get_custid() && ush_get_custid() != 0xff) {
 				/* only allow enabling of AH if cust ID non-zero and not 0xff */
				ret_status = CV_INVALID_COMMAND;
			}
			else {
				cv_hdotp_skip_table *skip_table = (cv_hdotp_skip_table *)FP_CAPTURE_LARGE_HEAP_PTR;
			  	ret_status = nvm_init();
				if (ret_status != PHX_STATUS_OK) break;
				ret_status = cvReadHDOTPSkipTable(skip_table);
				if(ret_status != CV_SUCCESS) break;
				ret_status = progSkpCntr(param1, skip_table->table, &skip_table->tableEntries);
				if(ret_status) {
					/* Programming successful */
					ret_status = CV_SUCCESS;
					*outputLen = skip_table->tableEntries * sizeof(uint32_t);
					memcpy((uint8_t *)outputparam, (uint8_t *)skip_table->table, *outputLen);
				}
				else {
					/* Programming not successful, update the skip list in the flash */
					ret_status = cvWriteHDOTPSkipTable();
					/* Pass in the return value if the above call fails, else
						return 1 to indicate HDOTP programming failure */
					if(ret_status ==  CV_SUCCESS)
						ret_status = 1;	/* Some value other than 0, for failure */
				}
			}
			break;
#endif
#ifdef PHX_REQ_SCI
		case CV_DIAG_CMD_SC_DEACTIVATE:
			ret_status = scDeactivateCard();
			break;
#endif
		default:
			ret_status = CV_INVALID_INPUT_PARAMETER;

	}

	return ret_status;
}

void cv_post_nfc_init(void)
{
	if (isNFCEnabled() && isNFCPresent()) 
	{
		/* Post the Initialize NFC chip now */
		printf("NFC initialization post\n");
		queue_nfc_cmd(SCHED_NFC_INIT_CMD, FALSE);
	}
}

