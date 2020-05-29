/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
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
 * @file sc_cli_states.c
 *
 * @brief  smart card cli states
 */

#include <arch/cpu.h>
#include <board.h>
#include <device.h>
#include <dmu.h>
#include <errno.h>
#include <init.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>
#include <sc/sc_datatypes.h>
#include <sc/sc.h>
#include "test_sc_infra.h"
#include <ztest.h>

static const s32_t
sc_cli_state_transition_table[sc_cli_state_max_state]
			     [sc_cli_event_max_event] = {
	/*   0,   1,   2,  3,   4,   5,  6,  7,  8,   9,  10, 11,  12,  13, 14,
	 *  CR, CRF, CRP, WR, WRF, WRP,  A, AF, AP, RAF, RAP, TD, TDF, TDP, RD,
	 *  15,  16, 17, 18
	 * RDD,   D,  I,  R
	 */

/*UNK  0*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  10, 11, 10},
/*CRST 1*/{ 10,   2,   3, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  10, 10, 10},
/*WRST 2*/{ 10,  10,  10, 10,   0,   3, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  10, 10, 10},
/*RSTD 3*/{  1,  10,  10, 10,  10,  10,  4, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  11, 10, 10},
/*ACT  4*/{ 10,  10,  10, 10,  10,  10, 10, 11,  5,   2,   6, 10,  10,  10, 10,
	    10,  11, 10, 10},
/*RDA  5*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,   2,   6, 10,  10,  10, 10,
	    10,  11, 10, 10},
/*RDY  6*/{  1,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10,  7,  10,  10,  9,
	    10,  11, 10, 10},
/*TX   7*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,   6,   6, 10,
	    10,  11, 10, 10},
/*TXED 8*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10,  9,
	    10,  11, 10, 10},
/*REC  9*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    6,  11, 10, 10},
/*IGR 10*/{ 10,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  11, 10, 10},
/*ID  11*/{  1,  10,  10, 10,  10,  10, 10, 10, 10,  10,  10, 10,  10,  10, 10,
	    10,  10, 10,  0},
};


char *sc_cli_get_event_string(enum sc_cli_event event)
{
	switch (event) {
	case sc_cli_event_initialize:
		return "sc_cli_event_initialize";

	case sc_cli_event_cold_reset:
		return "sc_cli_event_cold_reset";

	case sc_cli_event_cold_reset_failed:
		return "sc_cli_event_cold_reset_failed";

	case sc_cli_event_cold_reset_passed:
		return "sc_cli_event_cold_reset_passed";

	case sc_cli_event_warm_reset:
		return "sc_cli_event_warm_reset";

	case sc_cli_event_warm_reset_failed:
		return "sc_cli_event_warm_reset_failed";

	case sc_cli_event_warm_reset_passed:
		return "sc_cli_event_warm_reset_passed";

	case sc_cli_event_activate:
		return "sc_cli_event_activate";

	case sc_cli_event_activate_failed:
		return "sc_cli_event_activate_failed";

	case sc_cli_event_activate_passed:
		return "sc_cli_event_activate_passed";

	case sc_cli_event_rx_decode_failed:
		return "sc_cli_event_receive_decode_failed";

	case sc_cli_event_rx_decode_passed:
		return "sc_cli_event_receive_decode_passed";

	case sc_cli_event_tx_data:
		return "sc_cli_event_transmit_data";

	case sc_cli_event_tx_data_failed:
		return "sc_cli_event_transmit_data_failed";

	case sc_cli_event_tx_data_passed:
		return "sc_cli_event_transmit_data_passed";

	case sc_cli_event_rx_data:
		return "sc_cli_event_receive_data";

	case sc_cli_event_rx_data_done:
		return "sc_cli_event_receive_data_done";

	case sc_cli_event_deactivate:
		return "sc_cli_event_deactivate";

	case sc_cli_event_release:
		return "sc_cli_event_release";

	default:
		TC_ERROR("Cannot find the event string = %d", event);
		return " ";
	}
}

char *sc_cli_get_state_string(enum sc_cli_state state)
{
	switch (state) {
	case sc_cli_state_unknown:
		return "sc_cli_state_unknown";

	case sc_cli_state_initialized:
		return "sc_cli_state_initialized";

	case sc_cli_state_cold_resetting:
		return "sc_cli_state_cold_resetting";

	case sc_cli_state_warm_resetting:
		return "sc_cli_state_warm_resetting";

	case sc_cli_state_reset_done:
		return "sc_cli_state_reset_done";

	case sc_cli_state_activating:
		return "sc_cli_state_activating";

	case sc_cli_state_receive_decode_atr:
		return "sc_cli_state_receive_decode_atr";

	case sc_cli_state_ready:
		return "sc_cli_state_ready";

	case sc_cli_state_transmitting:
		return "sc_cli_state_transmitting";

	case sc_cli_state_transmitted:
		return "sc_cli_state_transmitted";

	case sc_cli_state_receiving:
		return "sc_cli_state_receiving";

	default:
		TC_ERROR("Cannot find the state string = %d", state);
		return "";
	}
}

s32_t sc_cli_activating_state(struct sc_cli_channel_handle *handle)
{
	s32_t rv = 0;

	handle->state = sc_cli_state_activating;

	if (handle->ch_param.read_atr_on_reset == 0) {
		rv = sc_cli_activating(handle);
		if (rv) {
			sc_cli_transit_state(handle,
						 sc_cli_event_activate_failed);
			TC_ERROR("Activating failed %d", rv);
			return rv;
		} else {
			return sc_cli_transit_state(handle,
						 sc_cli_event_activate_passed);
		}
	} else {
		rv = sc_cli_activating(handle);
		if (rv) {
			if ((rv == -ECANCELED) ||
			    (handle->reset_state ==
			     sc_cli_state_warm_resetting)) {
				/*
				 * If this is -ECANCELED, we need to deactivate
				 * ASAP. If we failed in warm reset, we need to
				 * deactivate ASAP too
				 */
				TC_ERROR("smart card activating, deacivate\n");
				return -ECANCELED;
			}
			rv = sc_cli_transit_state(handle,
						sc_cli_event_rx_decode_failed);
			TC_ERROR("cli card activating failed, rv = %d\n", rv);
			return rv;
		} else {
			return sc_cli_transit_state(handle,
						sc_cli_event_rx_decode_passed);
		}
	}
}

s32_t sc_cli_receive_decode_atr_state(struct sc_cli_channel_handle *handle)
{

	s32_t rv = 0;

	handle->state = sc_cli_state_receive_decode_atr;

	if (handle->ch_param.read_atr_on_reset == 0) {
		rv = sc_cli_atr_receive_decode(handle);
		if (rv)
			goto done;
	}

	if (!sc_cli_post_atr_receive(handle))
		return sc_cli_transit_state(handle,
					 sc_cli_event_rx_decode_passed);
done:
	if ((rv == -ECANCELED) ||
			(handle->reset_state == sc_cli_state_warm_resetting)) {
		/*
		 * If this is deactivate, we need to deactivate ASAP or if we
		 * failed in warm reset, we need to deactivate ASAP too.
		 */
		TC_ERROR("Deacivate the card");
		return -ECANCELED;
	}

	rv = sc_cli_transit_state(handle,
				  sc_cli_event_rx_decode_failed);
	TC_ERROR(" Transit state failed");
	return rv;
}

s32_t sc_cli_transmitting_state(struct sc_cli_channel_handle  *handle)
{
	s32_t rv;

	/* Smartcard current state =  Transmitting. Transient state */
	handle->state = sc_cli_state_transmitting;

	if (handle->tx_len == 0) {
		TC_ERROR("Smart Card Transmitting State,tx_len = 0\n");
		sc_cli_transit_state(handle, sc_cli_event_tx_data_failed);
		return -EFAULT;
	}

	rv = sc_cli_send_cmd(handle, handle->tx_buf, handle->tx_len);
	if (rv)
		sc_cli_transit_state(handle, sc_cli_event_tx_data_failed);
	else
		rv = sc_cli_transit_state(handle, sc_cli_event_tx_data_passed);

	return rv;

}

s32_t sc_cli_receiving_state(struct sc_cli_channel_handle  *handle)
{
	s32_t rv;

	handle->state = sc_cli_state_receiving;

	rv = sc_cli_read_cmd(handle,
		handle->rx_buf, &(handle->rx_len),
		(handle->rx_max_len ? handle->rx_max_len : SC_MAX_RX_SIZE));

	/* Regardless of the result, go to Ready state */
	sc_cli_transit_state(handle, sc_cli_event_rx_data_done);

	if (handle->rx_len != 0) {
		TC_DBG("good Leave receiving state rv = %d\n", rv);
		return rv;
	}

	if (rv) {
		TC_DBG("Fail in receiving state rv = %d\n", rv);
		return rv;
	}

	/* len is 0, we should return error */
	TC_DBG("Length 0 in receiving state");
	return -EFAULT;
}

s32_t sc_cli_transit_state(struct sc_cli_channel_handle *handle,
			   enum sc_cli_event event)
{
	u32_t new_state;

	handle->event = event;

	new_state = sc_cli_state_transition_table[handle->state][handle->event];

	if (new_state == sc_cli_state_ignore) {
		TC_DBG("Transit state: Ignore the current event\n");
		return -EINVAL;
	}

	switch (new_state) {

	case sc_cli_state_unknown:
		handle->state = sc_cli_state_unknown;
		return 0;

	case sc_cli_state_initialized:
		handle->state = sc_cli_state_initialized;
		return 0;

	case sc_cli_state_cold_resetting:
		handle->state = sc_cli_state_cold_resetting;
		handle->reset_state = sc_cli_state_cold_resetting;

		if (sc_cli_reset_ifd(handle, SC_COLD_RESET))
			return sc_cli_transit_state(handle,
					sc_cli_event_cold_reset_failed);
		else
			return sc_cli_transit_state(handle,
					sc_cli_event_cold_reset_passed);

	case sc_cli_state_warm_resetting:
		handle->state = sc_cli_state_warm_resetting;
		handle->reset_state = sc_cli_state_warm_resetting;

		if (sc_cli_reset_ifd(handle, SC_WARM_RESET)) {
			return sc_cli_transit_state(handle,
						sc_cli_event_warm_reset_failed);
		} else {
			if ((sc_cli_transit_state(handle,
					sc_cli_event_warm_reset_passed) == 0) &&
				(sc_cli_transit_state(handle,
					sc_cli_event_activate) == 0))
				return 0;
			else
				return -EFAULT;
		}

	case sc_cli_state_reset_done:
		handle->state = sc_cli_state_reset_done;
		return 0;
	case sc_cli_state_activating:
		return sc_cli_activating_state(handle);

	case sc_cli_state_receive_decode_atr:
		return sc_cli_receive_decode_atr_state(handle);

	case sc_cli_state_ready:
		handle->state = sc_cli_state_ready;
		return 0;

	case sc_cli_state_transmitting:
		return sc_cli_transmitting_state(handle);

	case sc_cli_state_transmitted:
		handle->state = sc_cli_state_transmitted;
		return 0;

	case sc_cli_state_receiving:
		return sc_cli_receiving_state(handle);

	default:
		TC_DBG("Invalid smart card transit state");
		return -EINVAL;
	}
}
