
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
 * @file sc_protocol.h
 * @brief Smart card protocol defines
 */

#define SC_TPDU_MIN_HEADER_SIZE		4	/* CLA INS P1 P2 */
#define SC_TPDU_HEADER_WITH_P3_SIZE	5	/* CLA INS P1 P2 P3 */
#define SC_TPDU_HEADER_LC_LE_SIZE	6	/* CLA INS P1 P2 LC ... LE */
#define SC_APDU_OFFSET_CLA		0
#define SC_APDU_OFFSET_INS		1
#define SC_APDU_OFFSET_P1		2
#define SC_APDU_OFFSET_P2		3
#define SC_APDU_OFFSET_LE		4
#define SC_APDU_OFFSET_LC		4

/* 3 (prologue) + 255 (max INFO len) + 2 (LRC/CRC) = 260 */
#define SC_T1_MAX_BUF_SIZE		(260)
#define SC_T1_OFFSET_NAD		0
#define SC_T1_OFFSET_PCB		1
#define SC_T1_OFFSET_LEN		2
#define SC_T1_OFFSET_INF		3

#define SC_T1_BLK_MASK			0xC0

#define SC_T1_I_BLK			0x0
#define SC_T1_I_BLK_N_SHIFT		6    /* I block seq no. b7 of b8-b1 */
#define SC_T1_I_BLK_MORE		0x20 /* I block More bit. b6 of b8-b1 */
#define SC_T1_I_BLK_N(pcb)		((pcb >> SC_T1_I_BLK_N_SHIFT) & 0x01)

#define SC_T1_R_BLK			0x80
#define SC_T1_R_BLK_N_SHIFT		4    /* R blk seq number. b5 of b8-b1 */
#define SC_T1_R_BLK_OK(pcb)		((pcb & 0x03) == 0)  /* b2-b1 are err */
#define SC_T1_R_IS_ERROR(pcb)		((pcb) & 0x0F)
#define SC_T1_R_BLK_N(pcb)		((pcb >> SC_T1_R_BLK_N_SHIFT) & 0x01)
#define SC_T1_R_BLK_ERR_PARITY		0x01
#define SC_T1_R_BLK_ERR_OTHER		0x02


#define SC_T1_S_BLK			0xC0
#define SC_T1_S_RESPONSE		0x20
#define SC_T1_S_RESYNC_REQ		(0xC0)
#define SC_T1_S_RESYNC_REP		(0xE0)
#define SC_T1_S_IFS_REQ			(0xC1)
#define SC_T1_S_IFS_REP			(0xE1)
#define SC_T1_S_ABORT_REQ		(0xC2)
#define SC_T1_S_ABORT_REP		(0xE2)
#define SC_T1_S_WTX_REQ			(0xC3)
#define SC_T1_S_WTX_REP			(0xE3)

#define SC_T1_GET_BLK_TYPE(pcb) (((pcb & 0x80) == 0 ? SC_T1_I_BLK : \
				   ((pcb & 0xC0) == 0x80 ? SC_T1_R_BLK : pcb)))
#define SC_T1_ERR_COUNT_RETRANSMIT	3     /* Rule 7.4.2 */
#define SC_T1_ERR_COUNT_RESYNCH		3     /* Rule 6.4 */

#define SC_RESPONSE_SUCCESS		(0x9000)

#define SC_MAX_ATR_SIZE			(32)
#define SC_MAX_ATR_HISTORICAL_SIZE	(15)
#define SC_ATR_LSN_MASK			(0x0f)
#define SC_ATR_MSN_MASK			(0xf0)
#define SC_ATR_INVERSE_CONVENSION	(0x3f)
#define SC_ATR_DIRECT_CONVENSION	(0x3b)

#define SC_ATR_TA_PRESENT_MASK		(0x10)
#define SC_ATR_TB_PRESENT_MASK		(0x20)
#define SC_ATR_TC_PRESENT_MASK		(0x40)
#define SC_ATR_TD_PRESENT_MASK		(0x80)

#define SC_ATR_TA1_DEFAULT		(0x11)
#define SC_EMV_ATR_TA1_VALID1		(0x11)
#define SC_EMV_ATR_TA1_VALID2		(0x12)
#define SC_EMV_ATR_TA1_VALID3		(0x13)

#define SC_ATR_TA1_D_FACTOR		(0x0f)
#define SC_ATR_TA1_F_FACTOR		(0xf0)
#define SC_ATR_TA1_F_FACTOR_SHIFT	(4)
#define SC_ATR_DEFAULT_D_FACTOR		(0x01)
#define SC_ATR_DEFAULT_F_FACTOR		(0x01)

#define SC_ATR_TD1_PROTOCOL_MASK	(0x0f)
#define SC_ATR_PROTOCOL_T1		(0x01)
#define SC_ATR_PROTOCOL_T0		(0x00)
#define SC_ATR_PROTOCOL_MAX		(14)


#define SC_ATR_TB3_BWI_MASK		(0xf0)
#define SC_ATR_TB3_BWI_SHIFT		(4)

#define SC_RESP_STATUS_OK(val)		((val[0] == 0x90) && (val[1] == 0x00))

#define SC_RESP_VERIFY_FAIL(val)		((val[0] == 0x63) &&	\
						 ((val[1] & 0xf0) == 0xc0))

#define SC_RESP_NO_SECURITY_CODE(val)		((val[0] == 0x69) &&	\
						 (val[1] == 0x82))

#define SC_RESP_AUTH_METHOD_BLOCKED(val)	((val[0] == 0x69) &&	\
						 (val[1] == 0x83))

#define SC_RESP_COND_NOT_SATISFIED(val)		((val[0] == 0x69) &&	\
						 (val[1] == 0x85))

#define SC_RESP_FILE_NOT_FOUND(val)		((val[0] == 0x6a) &&	\
						 (val[1] == 0x83))

#define SC_RESP_RECORD_NOT_FOUND(val)		((val[0] == 0x6a) &&	\
						 (val[1] == 0x82))

#define SC_RESP_WRONG_LENGTH(val)		((val[0] == 0x67) &&	\
						 (val[1] == 0x00))

#define SC_RESP_NO_FILE_SELECTED(val)		((val[0] == 0x69) &&	\
						 (val[1] == 0x85))

#define SC_RESP_COMMAND_ABORTED(val)		((val[0] == 0x6f) &&	\
						 (val[1] == 0x00))

#define SC_RESP_CMD_SUCCESS_DATA_AVAILABLE	(0x6100)
