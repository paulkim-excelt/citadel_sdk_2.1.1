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
/* FIXME: Remove what is not required. */
/* FIXME: Reconcile with MSR algorithms and merge. */
#include <stdio.h>
#include <board.h>
#include <string.h>
#include <errno.h>
#include <kernel.h>
/* For data type definitions */
#include <zephyr/types.h>
#include "logging/sys_log.h"

static const u8_t track23cset[32] = {
0x00,  '1',  '2',  0x00,  '4',  0x00,  0x00,  '7',
'8',  0x00,  0x00,  ';',  0x00,  '=',  '>',  0x00,
'0',  0x00,  0x00,  '3',  0x00,  '5',  '6',  0x00,
0x00,  '9',  ':',  0x00,  '<',  0x00,  0x00,  '?',
};

static const u8_t track1cset[128] = {
0x00,  '!',  '"',  0x00,  '$',  0x00,  0x00,  '\'',
'(',  0x00,  0x00,  '+',  0x00,  '-',  '.',  0x00,
'0',  0x00,  0x00,  '3',  0x00,  '5',  '6',  0x00,
0x00,  '9',  ':',  0x00,  '<',  0x00,  0x00,  '?',
'@',  0x00,  0x00,  'C',  0x00,  'E',  'F',  0x00,
0x00,  'I',  'J',  0x00,  'L',  0x00,  0x00,  'O',
0x00,  'Q',  'R',  0x00,  'T',  0x00,  0x00,  'W',
'X',  0x00,  0x00,  '[',  0x00,  ']',  '^',  0x00,
' ',  0x00,  0x00,  '#',  0x00,  '%',  '&',  0x00,
0x00,  ')',  '*',  0x00,  ',',  0x00,  0x00,  '/',
0x00,  '1',  '2',  0x00,  '4',  0x00,  0x00,  '7',
'8',  0x00,  0x00,  ';',  0x00,  '=',  '>',  0x00,
0x00,  'A',  'B',  0x00,  'D',  0x00,  0x00,  'G',
'H',  0x00,  0x00,  'K',  0x00,  'M',  'N',  0x00,
'P',  0x00,  0x00,  'S',  0x00,  'U',  'V',  0x00,
'X',   'Y',  'Z',  0x00,  '\\',  0x00,  0x00,  '-',
};

/* Convert to Bit Stream
 *- ts size should be len - 1
 *- skip first ts, size len - 2, begins from [2-1]
 */
s32_t ft_convert_bs(u16_t *p, s32_t len)
{
	s32_t bitlen = p[0];
	s32_t skip = 0;
	s32_t i, j = 0;

	for (i = 0; i < len - 1; i++) {
		if (skip) {
			skip = 0;
			continue;
		}
		if ((p[i] < (bitlen * 70 / 100))
			&& (p[i+1] < (bitlen * 70 / 100))
			&& ((p[i] + p[i+1]) < (bitlen * 140 / 100))) {
			bitlen = bitlen * 80 / 100 + (p[i] + p[i+1]) * 20 / 100 ;
			p[j++] = 1;
			skip = 1;
		} else {
			if (p[i] < bitlen * 80 / 100 ) {
				p[i+1] -= (bitlen - p[i]) * 80 / 100;
			}
			bitlen = bitlen * 80 / 100 + p[i] * 20 / 100;
			p[j++] = 0;
		}
	}
	return j;
}

s32_t ft_convert_ts(u16_t *p, s32_t len)
{
	s32_t i;

	for (i = 0; i < len - 2; i++) {
		if (p[i + 2] > p[i + 1])
			p[i] = p[i + 2] - p[i + 1];
		else
			p[i] = 0xffff + p[i + 2] - p[i + 1];
	}

	return 0;
}

s32_t ft_convert_char(u16_t *p, s32_t len, s32_t ch, u8_t *out)
{
	s32_t i = 0, j = 0;
	s32_t start = 0;
	u8_t *pout = (u8_t *)out;
	u8_t c;

	while ((i++) < len) {
		if (p[i]) {
			start = i;
			break;
		}
	}

	if (ch == 0) {
		for (i = start; i < len - 7; i += 7) {
			c = (p[i + 6] << 6) + (p[i + 5] << 5) + \
				(p[i + 4] << 4) + (p[i + 3] << 3) + (p[i + 2] << 2) + (p[i + 1] << 1) + (p[i]);
			c &= 0x7f;
			if (track1cset[c]) {
				pout[j] = track1cset[c];
				p[j++] = c;
			}
		}
	} else {
		for (i = start; i < len - 5; i += 5) {
			c = (p[i + 4] << 4) + (p[i + 3] << 3) + (p[i + 2] << 2) + (p[i + 1] << 1) + (p[i]);
			c &= 0x1f;
			if (track23cset[c]) {
				pout[j] = track23cset[c];
				p[j++] = c;
			}
		}
	}
	pout[j-1] = 0;
	return j-1;
}

s32_t ft_check_lrc(u16_t *p, s32_t size, s32_t ch)
{
	u8_t startb;
	u8_t endb;
	u8_t lrc_p = 0;
	u8_t lrc = 0;
	s32_t bits;
	s32_t i,j;

	if (ch == 0) {
		startb = 0x45;
		endb = 0x1f;
		bits = 6;
	} else {
		startb = 0xb;
		endb = 0x1f;
		bits = 4;
	}

	if (p[0] != startb) {
		SYS_LOG_ERR("!! failed to get begin mark 0x45/0xb, %x\n", p[0]);
		return 1;
	}

	for (i = 0; i < size; i++) {
		lrc = lrc ^ p[i];
		if (p[i] == endb)
			break;
	}

	if (p[i] != endb) {
		SYS_LOG_ERR("!! failed to get end mark 0x1f(?) \n");
		return 2;
	}

	lrc_p = 0;
	for (j = 0; j < bits; j++) {
		lrc_p ^= (lrc & (1<<j)) >> j;
	}
	lrc_p ^= 1;

	lrc &= (1<<bits) - 1;
	lrc = lrc | (lrc_p << bits);

	if (p[i + 1] != lrc) {
		SYS_LOG_ERR(">>> lrc not match %x, calc %x, P %x\n", p[i + 1], lrc, lrc_p);
		return 3;
	}
	return 0;
}

s32_t ft_decode(u16_t *p, s32_t *size, s32_t ch, u8_t *out, s32_t outlen)
{
	s32_t i;
	s32_t len, lenq;
	s32_t checkr, checkrq;
	u16_t *q = NULL;
	ARG_UNUSED(outlen);

	ft_convert_ts(p, *size);
	len = ft_convert_bs(p, *size - 2);
	lenq = len;

	/* Acquire Dynamic memory for Buffer */
	q = (u16_t *)k_malloc(1024*2);
	if(q == NULL)
		printk(" Could not allocate memory for backward swipe analysis!!");
	/* For backward swipe gather data in reverse order into q */
	else {
		for(i = 0; i < len; i++)
			q[i] = p[len - 1 - i];
	}

	/* First Check if it was forward swipe */
	len = ft_convert_char(p, len, ch, out);
	checkr =  ft_check_lrc(p, len, ch);
	if(checkr != 0 && q) {
		/* Try to check if it was backward swipe */
		lenq = ft_convert_char((u16_t *)q, lenq, ch, out);
		checkrq =  ft_check_lrc((u16_t *)q, lenq, ch);
		if(checkrq == 0) {
			printk("Backward Swipe !!\n");
			/* Free Dynamically allocated memory */
			k_free(q);
			/*out[lenq] = 0;*/
			*size = lenq;
			return checkrq;
		}
		else
			printk("Swipe Direction Undetected!!\n");
	}
	else
		printk("Forward Swipe !!\n");

	/* Free Dynamically allocated memory */
	k_free(q);
	/*out[len] = 0;*/
	*size = len;
	return checkr;
}
