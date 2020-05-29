/******************************************************************************
 *
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
 * Swipe Card
 *
 *  Detect signal transitions, and collect MSR data, for 3 tracks.
 *
 * TODO: Annotate all functions with explanation of algorithm
 *
 * FIXME:
 * MSR Swipe test fails with a Buffer Full after exiting DRIPS mode.
 * This problem doesn't happen after exiting Deep Sleep. Possibly
 * some ADC setting is not getting restored or Memory corruption.
 *
 */

#include <stdio.h>
#include <board.h>
#include "msr_swipe.h"
#include "adc/adc.h"
#include <string.h>
#include <errno.h>

/*
 * TODO: Annotate all functions with explanation of algorithm
 */

static u8_t msr_track23cset[32] = {
	0x00,  '0',  '8',  0x00,  '4',  0x00,  0x00,  '<',
	'2',  0x00,  0x00,  ':',  0x00,  '6',  '>',  0x00,
	'1',  0x00,  0x00,  '9',  0x00,  '5',  '=',  0x00,
	0x00,  '3',  ';',  0x00,  '7',  0x00,  0x00,  '?',
};

static u8_t msr_track1cset[128] = {
	0x00,  ' ',  '@',  0x00,  '0',  0x00,  0x00,  'P', /* 0 - 7 */
	'(',  0x00,  0x00,  'H',  0x00,  '8',  'X',  0x00, /* 8 - 15 */
	'$',  0x00,  0x00,  'D',  0x00,  '4',  'T',  0x00, /* 16 - 23 */
	0x00,  ',',  'L',  0x00,  '<',  0x00,  0x00,  '\\', /* 24 - 31 */
	'\"',  0x00,  0x00,  'B',  0x00,  '2',  'R',  0x00, /* 32 - 39 */
	0x00,  '*',  'J',  0x00,  ':',  0x00,  0x00,  'Z', /* 40 - 47 */
	0x00,  '&',  'F',  0x00,  '6',  0x00,  0x00,  'V', /* 48 - 63 */
	'.',  0x00,  0x00,  'N',  0x00,  '>',  '^',  0x00, /* 64 - 71 */
	'!',  0x00,  0x00,  'A',  0x00,  '1',  'Q',  0x00, /* 72 - 79 */
	0x00,  ')',  'I',  0x00,  '9',  0x00,  0x00,  'Y',
	0x00,  '%',  'E',  0x00,  '5',  0x00,  0x00,  'U',
	'-',  0x00,  0x00,  'M',  0x00,  '=',  ']',  0x00,
	0x00,  '#',  'C',  0x00,  '3',  0x00,  0x00,  'S',
	'+',  0x00,  0x00,  'K',  0x00,  ';',  '[',  0x00,
	'\'',  0x00,  0x00,  'G',  0x00,  '7',  'W',  0x00,
	0x00,  '/',  'O',  0x00,  '?',  0x00,  0x00,  '-',
};

#if MSR_DEVELOPMENT_TEST
#include <strings.h>
/* Card info, should be made change according to your test card. */
/* TODO: Verify if unsigned char works
 * u8_t *msr_tc[3] = {
 */
char *msr_tc[3] = {
	"%0123456789012345678901234567890123456789012345678901234567890123456789012345?",
	";0123456789012345678901234567890123456?",
	";01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123?"
};

char msr_oc[3][256];
#endif

/******************************************************************************
 * P R I V A T E  D E F I N I T I O N s
 ******************************************************************************/
/* Output Data Bufer Per Track
 * i: the iterator
 */
typedef struct {
	u8_t buf[MSR_BUF_SZ];
	u32_t i;
} msr_msr_obuf_t;

/* Other private Information
 * Per Track
 */
typedef struct __attribute__((aligned(32))) {
	u8_t  zcnt; /* Zero counter */
	u8_t  bcnt; /* Raw data counter */
	u8_t  icyc; /* count 3 cycles */
	u8_t  zwait;
	u8_t  first1;
	u8_t  bit_record; /* Bits keep track of stages */
	u8_t  edge; /* State of edges: 1/0, Positive/Negative */
	u8_t  bytedata;

	u8_t  after_1stedge;
	u8_t  start_phase;
	u8_t  bpc; /* bit per character */
	u8_t  movingcnt;

	/* Fields to process Raw data through an averaging LPF */
	u16_t raw_data_sum;
	u16_t raw_data_ptr;
	u16_t raw_data[MSR_AVG_DATA_NUM];

	u16_t adc_data;
	u16_t ladc_data;
	u16_t lladc_data;

	u16_t vth_low;
	u16_t vth_hi;

	u16_t vth_ref;

	u16_t vth_low_cap;
	u16_t vth_hi_cap;
	u16_t bytecnt;

	u16_t ccnt; /* Cycle counters */
	u16_t lmax; /* local max      */
	u16_t lmin; /* local min      */
	/* samples above ADC maximum value '1023' at one cycle */
	u16_t above_adc_max_cnt;
	/* samples below ADC minimum value '0' at one cycle */
	u16_t below_adc_min_cnt;

	MSR_TS_t ts;       /* time stamp counter (count ADC samples) */
	MSR_TS_t maxts;    /* Max time stamps                        */
	MSR_TS_t mints;    /* Min time stamps                        */
	MSR_TS_t lmaxts;   /* Max time stamps of prior cycle         */
	MSR_TS_t lmints;   /* Min time stamps of prior cycle         */
	MSR_TS_t cycsum;   /* Sum over 3 cycles                      */
	MSR_TS_t pt75;     /* 75% comparison                         */
	MSR_TS_t pt150;    /* 150% comparison                        */

	/* output buffer */
	msr_msr_obuf_t mob;
} msr_msr_track_t;

/* ADC Information per track:
 * ADC Number & ADC Channel Number.
 */
typedef struct {
	u8_t adcno;
	u8_t adcch;
} track_adc_info_t;

/** MSR SWIPE Device Private data structure
 * An array of Track Data
 */
struct bcm5820x_msr_swipe_dev_data_t {
	u32_t tick_start;
	u32_t tick_end;
	u32_t swip_dir; /* Direction of Swipe: Forward or Backward */
	u32_t swip_start[MSR_NUM_TRACK]; /*= {0, 0, 0}; Has Swipe Started? */
	u32_t swip_end[MSR_NUM_TRACK]; /*= {0, 0, 0}; Has Swipe Ended? */
	u32_t swip_start_index[MSR_NUM_TRACK]; /*= {0, 0, 0}; Swipe Start Index */
	u32_t swip_end_index[MSR_NUM_TRACK]; /*= {0, 0, 0}; Swipe End Index */
	u32_t track_data_len[MSR_NUM_TRACK]; /*= {MSR_TRACK_1_BYTE_NUM, MSR_TRACK_2_BYTE_NUM, MSR_TRACK_3_BYTE_NUM};  Length of each Track */

	u32_t swip_timeout; /* = 0; */
	u32_t swip_catch_aready[MSR_NUM_TRACK]; /* = {0,0,0}; */

	msr_msr_track_t track[MSR_NUM_TRACK];
	track_adc_info_t track_adc_info[MSR_NUM_TRACK];
};

#define MSR_LMAX(tk) { \
		if (tk->lmax <= tk->adc_data) { \
			tk->lmax   = tk->adc_data; \
			tk->maxts  = tk->ts; \
			tk->lmaxts = tk->ts; \
		} \
	}

#define MSR_LMIN(tk) {\
		if (tk->lmin >= tk->adc_data) { \
			tk->lmin   = tk->adc_data; \
			tk->mints  = tk->ts; \
			tk->lmints = tk->ts; \
		} \
	}

/* convenience defines */
#define MSR_DEV_DATA(dev) ((struct bcm5820x_msr_swipe_dev_data_t *)(dev)->driver_data)

/******************************************************************************
 * P R I V A T E  F U N C T I O N s
 ******************************************************************************/
/**************************************************************
 * Private Ring Buffers & Operations on Them
 **************************************************************/
/* FIXME:
 * 1. Such large memory should not be statically allocated.
 * These should either come from heap during card reading
 * and freed after card reading is done or the memory
 * should be provided by user.
 *
 * 2. Provide for the routines to work off a pointer to
 * any u16_t * type buffer.
 *
 * 3. The size parameter also should be a variable. Need
 * some experiments to determine the ideal size for an
 * RB for a given ADC sampling rate or MSR Type.
 *
 */
static u16_t *rb1;
static u16_t *rb2;
static u16_t *rb3;

static u32_t rb1_rd;
static u32_t rb1_wr;
static u32_t rb2_rd;
static u32_t rb2_wr;
static u32_t rb3_rd;
static u32_t rb3_wr;
/* gathers Statistics on how many times
 * attempt was made to write to a full
 * buffer
 */
static u32_t rb1_full = 0;
static u32_t rb2_full = 0;
static u32_t rb3_full = 0;

static void rb1_init(void)
{
	rb1_rd = 0;
	rb1_wr = 0;
	rb1_full = 0;
}

static void rb2_init(void)
{
	rb2_rd = 0;
	rb2_wr = 0;
	rb2_full = 0;
}

static void rb3_init(void)
{
	rb3_rd = 0;
	rb3_wr = 0;
	rb3_full = 0;
}

/* The ringX_len() function Returns the current number
 * of buffers that are occupied. To obtain free slots
 * this need to subtracted from ADC_BSIZE.
 */
static s32_t rb1_len(void)
{
	return (rb1_wr + ADC_MSR_BSIZE - rb1_rd) % ADC_MSR_BSIZE;
}

static s32_t rb2_len(void)
{
	return (rb2_wr + ADC_MSR_BSIZE - rb2_rd) % ADC_MSR_BSIZE;
}

static s32_t rb3_len(void)
{
	return (rb3_wr + ADC_MSR_BSIZE - rb3_rd) % ADC_MSR_BSIZE;
}
/* Full condition is when only one slot is free */
static s32_t rb1_in(u16_t v)
{
	if (rb1_len() == ADC_MSR_BSIZE - 1) {
		msrdebug("rb1 buffer full\n");
		rb1_full++;
		return 1;
	}

	rb1[rb1_wr] = v;
	rb1_wr = (rb1_wr + 1) % ADC_MSR_BSIZE;

	return 0;
}

static s32_t rb2_in(u16_t v)
{
	if (rb2_len() == ADC_MSR_BSIZE - 1) {
		msrdebug("rb2 buffer full\n");
		rb2_full++;
		return 1;
	}

	rb2[rb2_wr] = v;
	rb2_wr = (rb2_wr + 1) % ADC_MSR_BSIZE;
	return 0;
}

static s32_t rb3_in(u16_t v)
{
	if (rb3_len() == ADC_MSR_BSIZE - 1) {
		msrdebug("rb3 buffer full\n");
		rb3_full++;
		return 1;
	}

	rb3[rb3_wr] = v;
	rb3_wr = (rb3_wr + 1) % ADC_MSR_BSIZE;
	return 0;
}

/* Empty condition is when both pointers are same */
static s32_t rb1_out(u16_t *v)
{
	if (rb1_wr == rb1_rd)
		return 1;

	*v = rb1[rb1_rd];
	rb1_rd = (rb1_rd + 1) % ADC_MSR_BSIZE;
	return 0;
}

static s32_t rb2_out(u16_t *v)
{
	if (rb2_wr == rb2_rd)
		return 1;

	*v = rb2[rb2_rd];
	rb2_rd = (rb2_rd + 1) % ADC_MSR_BSIZE;
	return 0;
}

static s32_t rb3_out(u16_t *v)
{
	if (rb3_wr == rb3_rd)
		return 1;

	*v = rb3[rb3_rd];
	rb3_rd = (rb3_rd + 1) % ADC_MSR_BSIZE;
	return 0;
}

/* add data to output buffer (per track) */
static void msr_add_msr_output(u8_t d, msr_msr_obuf_t *mob)
{
	if (mob->i >= MSR_BUF_SZ) {
		msrdebug("buffer overflow");
		return;
	}

	mob->buf[mob->i++] = d;
}

#ifdef TRACE_ADC_DATA
static s32_t msr_dump_buf(u16_t *p, s32_t len)
{
	s32_t i;

	for (i = 0; i < len; i++) {
		msrdebug("%04x ", p[i]);
		if (!((i+1) % 40))
			msrdebug("\n");
	}
	msrdebug("\n");

	return 0;
}
#endif

/* calculate the difference between two TSs. */
static MSR_TS_t msr_ts_diff(MSR_TS_t ts0, MSR_TS_t ts1)
{
	if (ts0 >= ts1)
		return (ts0 - ts1);
	else
		return ((0xFFFF - ts1 + ts0));
}

/* Check Parity */
static s32_t msr_check_parity(u8_t d, u8_t n)
{
	u8_t i;
	u8_t sum = 0;

	for (i = 0; i < n; i++) {
		sum ^= (d & 0x1);
		d >>= 1;
	}

	if (sum == 1) /* odd parity */
		return 0;
	else
		return 1;
}

/*
 * we have reached a full cycle transition
 */
static void msr_full_cycle_log(struct device *msr, u8_t tkno)
{
	struct bcm5820x_msr_swipe_dev_data_t *const data = MSR_DEV_DATA(msr);
	msr_msr_track_t *tk = &data->track[tkno];

	tk->cycsum += tk->ccnt; /* Update cycle sum */
	tk->ccnt = 0; /* Reset cycle cnt for measure distance to next edge */
	tk->icyc++;

	if (tk->first1) {
		tk->bytedata = (tk->bytedata << 1) + tk->bit_record;
		tk->bcnt++;
		tk->bit_record = 0; /* Reset bit value to 0 */
	}

	if (tk->icyc >= 3) {
		tk->pt75 = tk->cycsum >> 2;
		tk->cycsum = 0;
		tk->icyc = 0;
	}

	if (tk->bcnt == tk->bpc) { /* has collected 8 bits, output data */
		msr_add_msr_output(tk->bytedata, &tk->mob);
		tk->bcnt = 0;
		tk->bytedata = 0;
		tk->bytecnt++;
	}
}

/* An upward turn is a low to high voltage transition
 */
static s32_t msr_turn_upward(msr_msr_track_t *tk)
{
	/* had encountered a low, and now come across VHI, trending up */
	if ((tk->edge == MSR_SWIPE_EDGE_DOWN)) {
		if (tk->lmin < tk->vth_low) {
			if ((tk->adc_data > tk->ladc_data)
				 && (tk->ladc_data > tk->lladc_data)
				 && (tk->adc_data > tk->vth_hi))
				return 1;
			if ((tk->adc_data > tk->vth_hi)
				 && (tk->adc_data > tk->lladc_data))
				return 1;
		} else {
			/* addition logic to deal with the case
			 * when _vth_low is set too low, or
			 * _vth_hi is set too high. Under such
			 * conditions, we may not be able to
			 * detect the signal turning...
			 *
			 * we have not seen the lmin change for
			 * consecutive 10 sampled or have crossed
			 * Vth_Hi once have moved some samples,
			 * e.g. 10, as well as go-up through high
			 * threshold. It is considered as turn-around.
			 */
			if (tk->movingcnt > 10 && tk->adc_data > tk->vth_hi)
				return 1; /* we have crossed the Vth_HI */
		}
	}
	return 0;
}

/* An downward turn is a high to lowh voltage transition
 */
static s32_t msr_turn_downward(msr_msr_track_t *tk)
{
	/* had encountered a high, and now come across VLO, trending down */
	if (tk->edge == MSR_SWIPE_EDGE_UP) {
		if (tk->lmax > tk->vth_hi) {
			if ((tk->adc_data < tk->ladc_data)
				 && (tk->ladc_data < tk->lladc_data)
				 && (tk->adc_data < tk->vth_low))
				return 1;
			if ((tk->adc_data < tk->vth_low)
				 && (tk->adc_data < tk->lladc_data))
				return 1;
		} else {
			/* addition logic to deal with the case
			 * when _vth_low is set too low, or
			 * _vth_hi is set too high.  Under such
			 * conditions, we may not be able to
			 * detect the signal turning...
			 *
			 * we have not seen the lmax change for
			 * consecutive 10 sampled or have crossed
			 * Vth_low once have moved some samples,
			 * e.g. 10, as well as go-down through low
			 * threshold.
			 * It is considered as turn-around.
			 */
			if (tk->movingcnt > 10 && tk->adc_data < tk->vth_low)
				return 1;
		}
	}
	return 0;
}

#if MSR_DEVELOPMENT_TEST
/* ADC Interrupt Statistics
 * 1. Counts how often did we see
 * a given ADC interrupt.
 * 2. How much data was collected.
 */
u32_t int0_count = 0;
u32_t int1_count = 0;
u32_t int2_count = 0;
u32_t int_data_size = 0;
#endif

static void msr_adc_msrr_tr1cb(struct device *adc, adc_no adcno, adc_ch ch, s32_t count)
{
	s32_t size = 0;
	u16_t v;
	const struct adc_driver_api *adcapi;

	if (!adc)
		msrdebug("Invalid ADC controller\n");
	else {
		adcapi = adc->driver_api;

		while (count--) {
			 size += adcapi->get_data(adc, adcno, ch, &v, 1);
			 /* Put it in the RB of track 1 */
			 rb1_in(v);
		}
#if MSR_DEVELOPMENT_TEST
		/* For gathering stats of how many time interrupt happened
		 * & how much data was gathered.
		 */
		 int0_count++;
		 int_data_size += size;
#endif
	}
}

static void msr_adc_msrr_tr2cb(struct device *adc, adc_no adcno, adc_ch ch, s32_t count)
{
	s32_t size = 0;
	u16_t v;
	const struct adc_driver_api *adcapi;

	if (!adc)
		msrdebug("Invalid ADC controller\n");
	else {
		adcapi = adc->driver_api;

		while (count--) {
			 size += adcapi->get_data(adc, adcno, ch, &v, 1);
			 /* Put it in the RB of track 2 */
			 rb2_in(v);
		}
#if MSR_DEVELOPMENT_TEST
		/* For gathering stats of how many time interrupt happened
		 * & how much data was gathered.
		 */
		 int1_count++;
		 int_data_size += size;
#endif
	}
}

static void msr_adc_msrr_tr3cb(struct device *adc, adc_no adcno, adc_ch ch, s32_t count)
{
	s32_t size = 0;
	u16_t v;
	const struct adc_driver_api *adcapi;

	if (!adc)
		msrdebug("Invalid ADC controller\n");
	else {
		adcapi = adc->driver_api;

		while (count--) {
			 size += adcapi->get_data(adc, adcno, ch, &v, 1);
			 /* Put it in the RB of track 3 */
			 rb3_in(v);
		}
#if MSR_DEVELOPMENT_TEST
		/* For gathering stats of how many time interrupt happened
		 * & how much data was gathered.
		 */
		 int2_count++;
		 int_data_size += size;
#endif
	}
}

/*
 * Detect the direction - Use a simple test.
 * Find SS and ES for track 1 and track 2.
 */
static s32_t msr_detect_dir(msr_msr_obuf_t *t1mob, msr_msr_obuf_t *t2mob)
{
	int i, dir = MSR_SWIPE_ABORT;
	u8_t *ds, *de, *ds2;

	ds = &t1mob->buf[0];
	ds2 = &t2mob->buf[0];
	i =  t1mob->i - 1;
	de = &t1mob->buf[i];

	if (*ds == MSR_T1_SS) { /* Start Sentinel as the first char */
		for (; i > 0; i--, de--) { /* Look for End Sentinel */
			if (*de == 0x0)
				continue; /* trailing 0's */

			if (*de == RV_MSR_T1_SS) { /* if backward char is also a SS! */
				if (*(ds + 1) == RV_MSR_T1_ES) { /* found a SS in front */
					if (*(de - 1) == MSR_T1_ES) {
						/* SS            ES LRC    */
						/* LRC RV_ES        RV_SS  */
						if (*ds2 == MSR_T23_SS)
							dir = MSR_FWD_SWIPE;
						else
							dir = MSR_BWD_SWIPE;
					} else
						dir = MSR_BWD_SWIPE;
				}
			} else {
				if (*de == MSR_T1_ES) {
					dir = MSR_FWD_SWIPE;
					break;
				}
			}
		}
	} else
		dir = MSR_BWD_SWIPE;

	return dir;
}

/*
 * print out the track data in a forward direction
 */
static s32_t msr_forward_msr_output(int ch, msr_msr_obuf_t *mob, u8_t bpc)
{
	u32_t i, lrc;
	u8_t *cset;
	u8_t *d, es, parity;
	u8_t LRC = 0;
	u32_t errcnt = 0;
	int es_end = 0;

	if (ch == 0) {
		cset = msr_track1cset;
		es = MSR_T1_ES;
	} else {
		cset = msr_track23cset;
		es = MSR_T23_ES;
	}

	d = mob->buf;

	msrdebug("------------------------Forward Swipe------------------------\n");

	if(mob->i > 0) {
		/* In this loop, we only handle data bytes.
		 *  LRC is outside the loop.
		 */
		for (i = 0; i < (mob->i - 1) && es_end == 0; i++) {
			/* msrdebug("[%x] ", *d); */

			/* This is where we print out the char. */
			msrdebug("%c", cset[*d]); /* print the char. */

			if (es_end == 0)
				errcnt += msr_check_parity(*d, bpc); /* check parity */

			if (*d == es)
				es_end = 1; /* mark it that we have found es */

#if MSR_DEVELOPMENT_TEST
			msr_oc[ch][i] = cset[mob->buf[i]];
#endif

			LRC ^= *d++; /* accumulate the checksum */
		}
	}
	msrdebug("\n");

	/* now check LRC - count the number of 1's
	 * of the final LRC, which is at *d
	 */
	/* TODO: Add #defines for below constants */
	/* mask off the parity of all prior accumulated data */
	if (bpc == MSR_T1_BPC)
		LRC &= 0x7e; /* Track 1 */
	else
		LRC &= 0x1e; /* Track 2/3 */

	for (i = 0, lrc = LRC, parity = 1; i < bpc; i++) {
		parity ^= (lrc & 0x01);
		lrc >>= 1;
	}

	LRC += parity;

	if (errcnt != 0 || LRC != *d) {
		msrdebug("\n\n Track%d Parity or LRC failed. errcnt[0x%x],"
			  " LRC[0x%x != 0x%x]\n", ch, errcnt, LRC, *d);
		return 0;  /* return error */
	}

#if MSR_DEVELOPMENT_TEST
	if (strncmp(msr_oc[ch], msr_tc[ch], strlen(msr_tc[ch])) != 0) {
		msrdebug("%s", msr_tc[ch]);
		msrdebug("\ntrack %d fail\n", ch);
	} else {
		msrdebug("\ntrack %d succeed\n", ch);
	}
#endif

	return 1;
}

/* print out the track data in a backward direction */
static s32_t msr_backward_msr_output(int ch, msr_msr_obuf_t *mob, u8_t bpc)
{
	int i, j;
	u8_t *cset, first1;
	u8_t es, dd, tmpd, bc, parity;
	u8_t LRC = 0, lrc;
	int errcnt = 0, es_end = 0;
	u8_t *rp, *wp;

	/* this is onlyto facilitate checking */
#if MSR_DEVELOPMENT_TEST
	int   tp = 0;
	u8_t  tk_dat[256];
	ARG_UNUSED(tk_dat);
#endif

	if (ch == 0) {
		cset = msr_track1cset;
		es = MSR_T1_ES;
	} else {
		cset = msr_track23cset;
		es = MSR_T23_ES;
	}

	/* reverse the data */
	wp = rp = &mob->buf[mob->i - 1];

	bc = 0;
	tmpd = 0;
	first1 = 0;

	msrdebug("------------------------Backward Swipe------------------------\n");

	for (i = mob->i; i > 0 && es_end == 0; i--) {
		dd = *rp--;
		for (j = 0; j < bpc; j++) {
			if (first1) { /* start converting */
				tmpd <<= 1;

				if (dd & 0x01)
					tmpd += 1;

				if (++bc == bpc) { /* accumulate enough bits */
					*wp = tmpd;
#if MSR_DEVELOPMENT_TEST
					msr_oc[ch][tp] = cset[tmpd];
					tk_dat[tp++] = tmpd;
#endif
					/*msrdebug(" [%x]", tmpd);*/
					/*
					 * XXX- This is where we print out the char.
					 */
					msrdebug("%c", cset[tmpd]); /* print the char. */

					/* parity checking */
					if (es_end == 0)
						errcnt += msr_check_parity(tmpd, bpc); /* check parity */

					if (*wp == es)
						es_end = 1; /* mark it that we have found es */

					LRC ^= tmpd;

					bc = 0;
					tmpd = 0;
				}
			} else {
				if (dd & 0x01) {
					first1 = 1;
					bc++;
					tmpd = 1;
				}
			}
			dd >>= 1;
		}
	}

	/* when we reach here, we should have hit end sentinel */
	/* append 0's, and this shall be the LRC character */
	/* XXX- we should not append 0's, we should continue
	 * accumulate one more character
	 */
	/* TODO: Confirm & Cleanup */
	/*  if (bc !=0) {
	 * while (bc++ < bpc)
	 * tmpd <<=1;

	 * *wp = tmpd;
	 *}
	 */

	while (bc < bpc) {
		tmpd <<= 1;
		if (bc == 0)
			dd = *rp--;
		if (dd & 0x01)
			tmpd += 1;
		dd >>= 1;
		bc++;
	}

	/* at this point, tmpd contains the final LRC! */

	/* now check LRC - count the number of 1's of
	 * the final LRC, which is at *rp
	 */

	/* mask off the parity of all prior accumulated data */
	/* TODO: Add #defines for the below constants */
	if (bpc == MSR_T1_BPC)
		LRC &= 0x7e; /* Track 1 */
	else
		LRC &= 0x1e; /* Track 2/3 */

	for (i = 0, lrc = LRC, parity = 1; i < bpc; i++) {
		parity ^= (lrc & 0x01);
		lrc >>= 1;
	}

	LRC += parity;

	/* check error */
	if (errcnt != 0 || LRC != tmpd) {
		msrdebug("\n\n Track%d Parity or LRC failed."
			  " errcnt[0x%x], LRC[0x%x != 0x%x]\n",
			  ch, errcnt, LRC, tmpd);
		return 0; /* return error */
	}

#if MSR_DEVELOPMENT_TEST
	if (strncmp(msr_oc[ch], msr_tc[ch], strlen(msr_tc[ch])) != 0) {
		msrdebug("%s", msr_oc[ch]);
		msrdebug("\ntrack %d fail\n", ch);
		return 0;
	} else {
		msrdebug("\ntrack %d succeed\n", ch);
	}
#endif
	return 1;
}

/* init high/low threshold and margin value based
 * on reference point for each track.
 */
static s32_t msr_track_vth_param_init(struct device *dev, u8_t tk_n, u16_t vth_idle)
{
	struct bcm5820x_msr_swipe_dev_data_t *const data = MSR_DEV_DATA(dev);
	msr_msr_track_t *tk = &data->track[tk_n];

	tk->vth_ref = vth_idle;

	/* set starting high and low threshold value for each track.*/
	tk->vth_hi = vth_idle + MSR_VTH_NOISE_FILTER_THRESHOLD;
	tk->vth_low = vth_idle - MSR_VTH_NOISE_FILTER_THRESHOLD;

	/* set high and low capability value for each track.*/
	tk->vth_hi_cap = vth_idle + MSR_VTH_HIGH_MARGIN;
	tk->vth_low_cap = vth_idle - MSR_VTH_HIGH_MARGIN;

	return 0;
}

/*
 * Call on the beginning of each Card Swipe
 * Depending on how many Tracks the card has
 * pass the ntracks parameter (2 or 3)
 */
static void msr_swipe_init(struct device *msr, int ntracks)
{
	int i, j;
	msr_msr_track_t *tk;
	struct bcm5820x_msr_swipe_dev_data_t *const msrdata = MSR_DEV_DATA(msr);

	for (i = 0; i < ntracks; i++) {
		tk = &msrdata->track[i];  /* track */

		/* 5 raw sample value */
		tk->raw_data_ptr = 0;
		tk->raw_data_sum = 0;
		for(j = 0; j < MSR_AVG_DATA_NUM; j++)
			tk->raw_data[j] = 0;
		tk->zcnt = 0; /* Zero counter */
		tk->bcnt = 0; /* Raw data counter */
		tk->icyc = 0; /* count 3 cycles */
		tk->zwait = 1; /* start with training 0's */
		tk->first1 = 0;
		tk->bit_record = 0; /* Bits keep track of stages */
		tk->edge = 0; /* starting edge */
		tk->adc_data = 0;
		tk->ladc_data = 0;
		tk->lladc_data = 0;
		tk->vth_low = 0;
		tk->vth_hi = 0;
		tk->bytecnt = 0; /* the number of bytes collected */
		tk->after_1stedge = 0;
		tk->start_phase = 1;
		tk->movingcnt = 0;

		if (i == 0)
			tk->bpc = MSR_T1_BPC; /* 6 bit + parity */
		else
			tk->bpc = MSR_T23_BPC; /* 4 bits + parity */

		tk->above_adc_max_cnt = 0; /* samples counter which value > 1023 */
		tk->below_adc_min_cnt = 0; /* samples counter which value = 0 */

		tk->lmax = 0;	/* local max */
		tk->lmin = 0;	/* local min */
		tk->ccnt = 0;	/* Cycle counters */
		tk->ts = 0;	/* Main time stamp for ADC conversions */
		tk->maxts = 0; /* Max time stamps */
		tk->mints = 0; /* Min time stamps */
		tk->lmaxts = 0; /* Max time stamps */
		tk->lmints = 0; /* Min time stamps */
		tk->cycsum = 0; /* Sum over 3 cycles */
		tk->pt75 = 0; /* 75% comparison */
		tk->pt150 = 0; /* 150% comparison */
		tk->mob.i = 0; /* reset output buffer pointer to 0 */
	}
}

/*
 * (Non Zero Wait state) In this routine, we start
 * at a falling edge, and try to see if we climb
 * back out and start rising.  That is, we start
 * a Low to High transition.
 *
 * FIXME:
 * Directly pass the track to save time
 */
static void detect_ascending_edge(struct device *msr, u8_t tkno)
{
	struct bcm5820x_msr_swipe_dev_data_t *const msrdata = MSR_DEV_DATA(msr);
	msr_msr_track_t *tk = &msrdata->track[tkno];

	if (msr_turn_upward(tk)) { /* turning */
		tk->lmax = tk->adc_data; /* local max */
		tk->lmints = tk->mints;  /* mints of prior cycle */
		/* (tk->mints - tk->lmaxts), #samples has elapsed... */
		tk->ccnt += msr_ts_diff(tk->mints, tk->lmaxts);

		tk->below_adc_min_cnt = 0; /* reset the count to zero */

		/* Are we in detecting the training 0s? */
		if (tk->zwait == 0) { /* passed training */
			/* printf("TURN UP: %u\t%u\t%u\n", tk->ccnt, tk->pt75, tk->lmaxts); */
			/* __db_rpt_edge(tk->lmaxts); */
			if (tk->ccnt < tk->pt75) { /* 1/2 */
				tk->bit_record = 1;
				tk->first1 = 1;
			} else { /* Full cycle */
				msr_full_cycle_log(msr, tkno);
			}
			/* dump_tk(tk, "NONZW_TURN_UP"); */
		} else { /* we are with training 0's */
			tk->cycsum += tk->ccnt;

			if (tk->after_1stedge) {
				tk->icyc++;
				if ((tk->ccnt > tk->pt75) && (tk->ccnt < tk->pt150)) {
					/* find periodic traing 0's */
					if (++tk->zcnt == MSR_Z_LIMIT)
						tk->zwait = 0;
				} else /* outside of range, reset */
					tk->zcnt = 0;
			}
			tk->ccnt = 0; /* Reset cycle counter */
			tk->after_1stedge = 1;

			if (tk->icyc >= 3) { /* Calculate 75% and 150% */
				tk->pt150 = tk->cycsum >> 1;
				tk->pt75 = tk->pt150 >> 1;
				tk->cycsum = 0;
				tk->icyc = 0;
			}
		}

		tk->edge = MSR_SWIPE_EDGE_UP; /* confirm in L-H edge */
		tk->movingcnt = 0;
	}
#if 1
else if (tk->adc_data <= tk->lmin) { /* Check against local min */
		tk->lmin = tk->adc_data; /* Update local min */
		tk->mints = tk->ts; /* Time stamp local min */

		tk->movingcnt = 0;

		/* if tk->adc_data reach to minmum value - '0', then record it.*/
		if (tk->adc_data == MSR_ADC_MIN_VALUE_10B) {
			tk->below_adc_min_cnt++;
		}
		/* adjust the peak timestamp in case of their tk->adc_data
		 * is same at minimum level.
		 */
		tk->mints -= tk->below_adc_min_cnt / 2;

		/*
		 * THREE threshold adjustment.
		 */
		if ((tk->adc_data < tk->ladc_data)
			 && (tk->ladc_data < tk->lladc_data)
			 && (tk->adc_data < tk->vth_low)) {
			if (tk->adc_data < (tk->vth_ref - MSR_VTH_HIGH_MARGIN))
				/* use high threshold if
				 * (amplitude < high margin)
				 */
				tk->vth_low = tk->vth_ref - MSR_VTH_HIGH_THRESHOLD;
			else if (tk->adc_data < (tk->vth_ref - MSR_VTH_MID_MARGIN))
				/* use middle threshold
				 * if (high margin < amplitude < middle margin)
				 */
				tk->vth_low = tk->vth_ref - MSR_VTH_MID_THRESHOLD;
			else if (tk->adc_data < (tk->vth_ref - MSR_VTH_LOW_MARGIN))
				/* use low threshold if (middle margin < amplitude < low margin) */
				tk->vth_low = tk->vth_ref - MSR_VTH_LOW_THRESHOLD;
			else {
				/* use default MSR_VTH_NOISE_FILTER_THRESHOLD. */
				;
			}
		}
	}
#else
else if (_adc_data < tk->lmin) { /* Check against local min */
		tk->lmin = tk->adc_data; /* Update local min */
		tk->mints = tk->ts; /* Time stamp local min */

		tk->movingcnt = 0;

		/*
		 * dynamically adjust the threshold
		 */
		u16_t t;

		t = tk->lmin+MSR_VTH_HIGH_MARGIN;
		if ((t >= tk->vth_low_cap) && (t < tk->vth_low)) {
			tk->vth_low = t;
		}
	}
#endif
else
		tk->movingcnt++;
}

/*
 * (Non Zero Wait State) In this routine, we start at a rising edge,
 * and try to see if we fall back down and start falling.
 * That is, we start a High to Low transition.
 *
 * FIXME:
 * Directly pass the track to save time
 */
static void detect_descending_edge(struct device *msr, u8_t tkno)
{
	struct bcm5820x_msr_swipe_dev_data_t *const msrdata = MSR_DEV_DATA(msr);
	msr_msr_track_t *tk = &msrdata->track[tkno];

	if (msr_turn_downward(tk)) { /* Turning */
		tk->lmin = tk->adc_data; /* local min */
		tk->lmaxts = tk->maxts; /* maxts of prior cycle */
		/* (tk->maxts - tk->lmints), elapsed time of prior cycle */
		tk->ccnt += msr_ts_diff(tk->maxts, tk->lmints);
		/* reset the count to zero */
		tk->above_adc_max_cnt = 0;

		if (tk->zwait == 0) {
			/* printf("TURN DWD %u\t%u\t%u\n",
			 * tk->ccnt, tk->pt75, tk->lmints);
			 */
			/* __db_rpt_edge(_lmints); */

			if (tk->ccnt < tk->pt75) { /* 1/2 */
				tk->bit_record = 1;
				tk->first1 = 1;
			} else { /* full cycle */
				msr_full_cycle_log(msr, tkno);
			}
			/* dump_tk(tk, "NONZW_TURN_DOWN"); */
		} else {
			tk->cycsum += tk->ccnt;
			if (tk->after_1stedge) {
				tk->icyc++;
				if ((tk->ccnt > tk->pt75) && (tk->ccnt < tk->pt150)) {
					/* find periodic training 0's */
					if (++tk->zcnt == MSR_Z_LIMIT)
						tk->zwait = 0;
				} else
					/* outside of range, reset */
					tk->zcnt = 0;
			}

			tk->ccnt = 0; /* Reset cycle counter */
			tk->after_1stedge = 1;

			if (tk->icyc >= 3) { /* Calculate 75% and 150% */
				tk->pt150 = tk->cycsum >> 1;
				tk->pt75 = tk->pt150 >> 1;
				tk->cycsum = 0;
				tk->icyc = 0;
			}
		}
		/* Confirmed in Negative H-L edge */
		tk->edge = MSR_SWIPE_EDGE_DOWN;

		tk->movingcnt = 0;
	}
#if 1
/* TODO: Confirm & Cleanup Algorithm */
else if (tk->adc_data >= tk->lmax) { /* Check against local max */
		tk->lmax = tk->adc_data; /* Update local max */
		tk->maxts = tk->ts; /* Time stamp local max */

		tk->movingcnt = 0;

		/* if _adc_data reach to maximum value
		 * - '1023', then record it.
		 */
		if (tk->adc_data == MSR_ADC_MAX_VALUE_10B)
			tk->above_adc_max_cnt++;

		/* adjust the peak timestamp in case of their
		 * _adc_data is same at maximum level.
		 */
		tk->maxts -= tk->above_adc_max_cnt / 2;

		/*
		 * THREE threshold adjustment.
		 */
		if ((tk->adc_data > tk->ladc_data)
			 && (tk->ladc_data > tk->lladc_data)
			 && (tk->adc_data > tk->vth_hi)) {
			if (tk->adc_data > (tk->vth_ref + MSR_VTH_HIGH_MARGIN))
				/* use high threshold if (amplitude > high margin) */
				tk->vth_hi = tk->vth_ref + MSR_VTH_HIGH_THRESHOLD;
			else if (tk->adc_data > (tk->vth_ref + MSR_VTH_MID_MARGIN))
				/* use middle threshold if
				 * (high margin > amplitude > middle margin)
				 */
				tk->vth_hi = tk->vth_ref + MSR_VTH_MID_THRESHOLD;
			else if (tk->adc_data > (tk->vth_ref + MSR_VTH_LOW_MARGIN))
				/* use low threshold if
				 * (middle margin > amplitude > low margin)
				 */
				tk->vth_hi = tk->vth_ref + MSR_VTH_LOW_THRESHOLD;
			else {
				; /* use default MSR_VTH_NOISE_FILTER_THRESHOLD. */
			}
		}
	}
#else  /* original algorithm */
else if (_adc_data > _lmax) { /* Check against local max */
		_lmax = _adc_data; /* Update local max */
		_maxts = _ts; /* Time stamp local max */

		_movingcnt = 0;

		/* dynamically adjust the threshold */
		u16_t t;
		t = _lmax-MSR_VTH_HIGH_MARGIN;
		if ((t <= _vth_hi_cap) && (t > _vth_hi)) {
			_vth_hi = t;
		}
	}
#endif
else
		tk->movingcnt++;
}

/* Decode F2F signal on Track.
 * FIXME:
 * Directly pass the track to save time
 */
static void msr_f2f_decode_track(struct device *msr, u8_t tk_n, u16_t adc_data)
{
	u32_t i;
	msr_msr_track_t *tk;
	struct bcm5820x_msr_swipe_dev_data_t *const data = MSR_DEV_DATA(msr);

	tk = &data->track[tk_n];  /* track */

	/* up one more ADC samples as a time stamp */
	tk->ts++;

	if (tk->ts == 1) {
		/* 5 samples average for one input data
		 * FIXME:
		 * Considering the fact that for Citadel
		 * we have 250Ksps ADCs may be we should consider
		 * increasing the number of samples here.
		 *
		 */
		tk->raw_data[0] = adc_data;
		tk->lladc_data = tk->ladc_data;
		tk->ladc_data = tk->adc_data;
		tk->raw_data_sum = 0;
		for(i = 0; i < MSR_AVG_DATA_NUM; i++)
			tk->raw_data_sum += tk->raw_data[i];
		tk->adc_data = tk->raw_data_sum / MSR_AVG_DATA_NUM;
		tk->raw_data_ptr = 0;

		tk->lmax   = tk->adc_data;
		tk->maxts  = tk->ts;
		tk->lmaxts = tk->ts;
		tk->lmin   = tk->adc_data;
		tk->mints  = tk->ts;
		tk->lmints = tk->ts;
		return;
	}
	else {
		tk->lladc_data = tk->ladc_data;
		tk->ladc_data = tk->adc_data;
		tk->raw_data_ptr = (tk->raw_data_ptr + MSR_AVG_DATA_NUM - 1) % MSR_AVG_DATA_NUM;
		tk->raw_data_sum = tk->raw_data_sum + adc_data - tk->raw_data[tk->raw_data_ptr];
		tk->adc_data = (tk->raw_data_sum) / MSR_AVG_DATA_NUM;
		tk->raw_data[tk->raw_data_ptr] = adc_data;
	}

	if (tk->ts <= 3) {
		MSR_LMAX(tk);
		MSR_LMIN(tk);
		return;
	}

	if (tk->start_phase) {
		if (tk->maxts > tk->mints)
			tk->edge = MSR_SWIPE_EDGE_UP;

		if (tk->maxts < tk->mints)
			tk->edge = MSR_SWIPE_EDGE_DOWN;

		tk->start_phase = 0;
		return;
	}

	if (tk->edge == MSR_SWIPE_EDGE_DOWN) /* H-L edge */
		detect_ascending_edge(msr, tk_n);
	else /* L-H edge */
		detect_descending_edge(msr, tk_n);
}


/* Record start point and end point timestamp
 * as well as determine whether card swipe is completed or not.
 */
static s32_t msr_check_swipe(struct device *msr, s32_t ch)
{
	u8_t ss, es, rv_ss, rv_es;
	msr_msr_obuf_t *mob;
	struct bcm5820x_msr_swipe_dev_data_t *const data = MSR_DEV_DATA(msr);

	mob = &data->track[ch].mob;
	if (ch == 0) {
		ss = MSR_T1_SS;
		es = MSR_T1_ES;
		rv_ss = RV_MSR_T1_SS;
		rv_es = RV_MSR_T1_ES;

	} else {
		ss = MSR_T23_SS;
		es = MSR_T23_ES;
		rv_ss = RV_MSR_T23_SS;
		rv_es = RV_MSR_T23_ES;
	}

	/* check Forward SS byte as start point.
	 * byte stream is SS-...... ES-LRC
	 */
	if ((mob->buf[mob->i - 1] == ss) && (data->swip_start[ch] == 0)) {
		data->swip_start[ch] = 1;
		data->swip_dir = MSR_FWD_SWIPE;
		/* first start time got, ignore other track start. */
		data->tick_start = (data->tick_start == 0) ? k_uptime_get_32() : data->tick_start;
		msrdebug("fwd start! ch:%d, ss:0x%2x, i: %d\n",
			ch, mob->buf[mob->i - 1], mob->i);
	}

	/* check Backward Reverted-ES byte as start point,
	 * sometimes can't identify correctly.
	 */
	if (mob->i > 2) {
		/* first bytedata should be LRC but this reverted
		 * ES ! byte stream is RV_LRC-...... RV_ES-RV_SS
		 */
		if ((mob->buf[mob->i - 2] == rv_es) && (data->swip_start[ch] == 0)) {
			data->swip_start[ch] = 1;
			data->swip_dir = MSR_BWD_SWIPE;
			/* first start time got, ignore other track start. */
			data->tick_start = (data->tick_start == 0) ? k_uptime_get_32() : data->tick_start;
			msrdebug("back start! ch:%d, reverted"
				  " lrc:0x%2x, i: %d\n", ch,
				  mob->buf[mob->i - 3], mob->i);
		}
	}

	/* check Forward ES byte as end point.*/
	if (data->swip_dir == MSR_FWD_SWIPE) {
		/* one additional byte for LRC. */
		if (mob->buf[mob->i - 2] == es) {
			data->swip_end[ch] = 1;
			/* last end time got */
			data->tick_end = k_uptime_get_32();

			msrdebug("fwd end! ch:%d, lrc:0x%2x, i: %d\n",
				   ch, mob->buf[mob->i - 1], mob->i);
			return 0;
		}
	}
	/* check Backward Reverted-SS byte as end point,
	 * sometimes can't identify correctly.
	 */
	else if (mob->buf[mob->i - 1] == rv_ss) {
		data->swip_end[ch] = 1;
		data->tick_end = k_uptime_get_32();
		msrdebug("backward end! ch:%d, es:0x%2x, i: %d\n",
			   ch, mob->buf[mob->i - 1], mob->i);
		return 0;
	}

	/* !!! Always get to end point, if byte number larger than
	 * track data length, this situation happens for Backward case.
	 */
	if (mob->i >= data->track_data_len[ch]) {
		data->swip_end[ch] = 1;
		data->tick_end = k_uptime_get_32();
		msrdebug("swipe forced to end!"
			  " ch:%d, es:0x%2x, i: %d\n",
			  ch, mob->buf[mob->i - 1], mob->i);
		return 0;
	}

	return -EIO;
}

/******************************************************************************
 * P U B L I C  F U N C T I O N s (A P I s)
 ******************************************************************************/
s32_t bcm5820x_msr_swipe_configure_adc_info(struct device *msr,
   u8_t trk1adc, u8_t trk1ch, u8_t trk2adc, u8_t trk2ch, u8_t trk3adc, u8_t trk3ch)
{
	struct bcm5820x_msr_swipe_dev_data_t *msrdata;

	if(!msr) {
		msrdebug("Invalid MSR Swipe device pointer!!\n");
		return -EINVAL;
	}

	if(trk1adc >= adc_max || trk2adc >= adc_max || trk3adc >= adc_max ||
	   trk1ch >= adc_chmax || trk2ch >= adc_chmax || trk3ch >= adc_chmax) {
		msrdebug("Invalid MSR Swipe ADC Configuration!!\n");
		return -EINVAL;
	}

	msrdata = MSR_DEV_DATA(msr);

	msrdata->track_adc_info[0].adcno = trk1adc;
	msrdata->track_adc_info[0].adcch = trk1ch;
	msrdata->track_adc_info[1].adcno = trk2adc;
	msrdata->track_adc_info[1].adcch = trk2ch;
	msrdata->track_adc_info[2].adcno = trk3adc;
	msrdata->track_adc_info[2].adcch = trk3ch;

	return 0;
}

s32_t bcm5820x_msr_swipe_init_with_adc(struct device *msr, struct device *adc)
{
	s32_t i;
	u16_t v;
	u32_t sample_loop = 1000;
	u32_t track_ref_value[MSR_NUM_TRACK] = {0,0,0};
	u32_t adc_value[MSR_NUM_TRACK] = {0,0,0};
	u32_t times[MSR_NUM_TRACK] = {0,0,0};
	struct bcm5820x_msr_swipe_dev_data_t *msrdata;

	if(!msr) {
		msrdebug("Invalid MSR Swipe device pointer!!\n");
		return -EINVAL;
	}
	if(!adc) {
		msrdebug("Invalid ADC device pointer!!\n");
		return -EINVAL;
	}

	msrdata = MSR_DEV_DATA(msr);

	msrdata->tick_start = 0;
	msrdata->tick_end = 0;
	msrdata->swip_dir = 0;
	msrdata->swip_timeout = 0;

	for(i = 0; i < MSR_NUM_TRACK; i++)
	{
		msrdata->swip_start[i] = 0;
		msrdata->swip_end[i] = 0;
		msrdata->swip_catch_aready[i] = 0;
		msrdata->swip_start_index[i] = 0;
		msrdata->swip_end_index[i] = 0;
	}

	/* Initialize the Ring Buffers
	 * for each track.
	 * FIXME: Consider allocating memory from
	 * heap or have the user pass a memory buffer
	 * pointer & its size
	 * Avoid repeated malloc with multiple init
	 *
	 */
	if (!rb1)
		rb1 = (u16_t *)k_malloc(ADC_MSR_BSIZE * sizeof(u16_t));
	else
		msrdebug("Data buffer1 not released in previous swipe.\n");

	if (!rb2)
		rb2 = (u16_t *)k_malloc(ADC_MSR_BSIZE * sizeof(u16_t));
	else
		msrdebug("Data buffer2 not released in previous swipe.\n");

	if (!rb3)
		rb3 = (u16_t *)k_malloc(ADC_MSR_BSIZE * sizeof(u16_t));
	else
		msrdebug("Data buffer3 not released in previous swipe.\n");

	if(rb1 == NULL || rb2 == NULL || rb3 == NULL) {
		msrdebug("Could not allocate memory for Ring Buffers.\n");
		if (rb1)
			k_free(rb1);

		if (rb2)
			k_free(rb2);

		if (rb3)
			k_free(rb3);

		rb1 = NULL;
		rb2 = NULL;
		rb3 = NULL;

		return -EINVAL;
	}
	rb1_init();
	rb2_init();
	rb3_init();

	/* ADC Clock Divisor:
	 * FIXME: This value seems to determine one waveform
	 * of the ADC Clock. The value existing for Lynx:
	 * 25Mhz/(4+1+4+1) = 2.5Mhz
	 * seems to be too fast for Citadel, with the rbs becoming
	 * full very quickly before content could be read out. This
	 * happens because processing time of a write is much less than
	 * that of a read, which, in addition to reading out the sample
	 * also analyses it to detect start/end etc. of a swipe.
	 *
	 * Using a divider of 20 instead of 4 for each of the low and
	 * high period of the ADC Clock seems to be working fine.
	 * sys_write32(0x80140014, CRMU_ADC_CLK_DIV);
	 *
	 * Other option to make this work is to have larger buffer as
	 * it was in Lynx. Need to strike a good tradeoff between speed
	 * and buffer size.
	 */
	adc_set_clk_div(adc, 0x14, 0x14);
	msrdebug("\nCRMU_ADC_CLK_DIV : 0x%8x\n", adc_get_clk_div(adc));

	adc_init(adc, 0);
	adc_init(adc, 1);
	adc_init(adc, 2);

	/* Enable TDM Continuous Mode with default ISRs */
	adc_enable_tdm(adc, msrdata->track_adc_info[0].adcno, msrdata->track_adc_info[0].adcch, msr_adc_msrr_tr1cb);
	adc_enable_tdm(adc, msrdata->track_adc_info[1].adcno, msrdata->track_adc_info[1].adcch, msr_adc_msrr_tr2cb);
	adc_enable_tdm(adc, msrdata->track_adc_info[2].adcno, msrdata->track_adc_info[2].adcch, msr_adc_msrr_tr3cb);

	/* capture and caculate the ADC idle value for MSR signal reference.
	 *
	 * Notes & Observations (for Improvement):
	 * ---------------------------------------
	 * It was observed that to gather the idle values from the ADC
	 * was rather challenging. This loop will not terminate if the
	 * the ISRs are not snappy enough. This loop seems to be very
	 * sensitive to interrupt latency. For example, if Ring Buffers
	 * are made part of the MSR object and if the rb_in function
	 * is made to resolve the msr object to get to the actual ring buffer
	 * it turned out to be too slow and the loop never terminated.
	 * This was because the times value never exceeded the sample loop.
	 *
	 * So the need here is that the insertion & extraction of data from
	 * the ring buffers has to be fast. The issue resolved when the rb_in
	 * function was directly given access to the buffer to store data.
	 *
	 * FIXME:
	 * From the above observation it seems imperative to search for a
	 * better mechanism to write in & read out data from an RB. May be
	 * the writes should be allowed when RB is full, overwriting the
	 * the old data, but care should be taken such that in such a case
	 * the latest samples (last samples collected) can be gathered off
	 * the RB. This should be easy to implement.
	 */
	while(1)
	{
		if (rb1_len()) {
			rb1_out(&v);
			adc_value[0] += v;
			times[0] ++;
		}

		if (rb2_len()) {
			rb2_out(&v);
			adc_value[1] += v;
			times[1] ++;
		}

		if (rb3_len()) {
			rb3_out(&v);
			adc_value[2] += v;
			times[2] ++;
		}

		if((times[0] > sample_loop) && (times[1] > sample_loop) && (times[2] > sample_loop))
			break;
	}

	track_ref_value[0] = adc_value[0] / times[0];
	track_ref_value[1] = adc_value[1] / times[1];
	track_ref_value[2] = adc_value[2] / times[2];

	msr_swipe_init(msr, MSR_NUM_TRACK);

	/* init adc-msr threshold and margin. */
	msr_track_vth_param_init(msr, 0, track_ref_value[0]);
	msr_track_vth_param_init(msr, 1, track_ref_value[1]);
	msr_track_vth_param_init(msr, 2, track_ref_value[2]);

	msrdebug("\n3 tracks ADC reference value : [%d, %d, %d]\n", track_ref_value[0], track_ref_value[1], track_ref_value[2]);

	return 0;
}

s32_t bcm5820x_msr_swipe_disable_adc(struct device *msr, struct device *adc)
{
	struct bcm5820x_msr_swipe_dev_data_t *msrdata;
	const struct adc_driver_api *adcapi;

	if(!msr) {
		msrdebug("Invalid MSR Swipe device pointer!!\n");
		return -EINVAL;
	}
	if(!adc) {
		msrdebug("Invalid ADC device pointer!!\n");
		return -EINVAL;
	}

	msrdata = MSR_DEV_DATA(msr);
	adcapi = adc->driver_api;

	adcapi->disable_tdm(adc, msrdata->track_adc_info[0].adcno, msrdata->track_adc_info[0].adcch);
	adcapi->disable_tdm(adc, msrdata->track_adc_info[1].adcno, msrdata->track_adc_info[1].adcch);
	adcapi->disable_tdm(adc, msrdata->track_adc_info[2].adcno, msrdata->track_adc_info[2].adcch);

	adcapi->powerdown(adc);

	return 0;
}

s32_t bcm5820x_msr_swipe_gather_data(struct device *msr, struct device *adc)
{
	u8_t i;
	u16_t v;
	s32_t flag;
	/* Number of Iterations */
	volatile s32_t t1 = 8000000;
	msr_msr_track_t *tk;
	struct bcm5820x_msr_swipe_dev_data_t *const data = MSR_DEV_DATA(msr);
	s32_t ret = 0;

	do {
		if (rb1_len()) {
			rb1_out(&v);
			msr_f2f_decode_track(msr, 0, v);
		}
		if (rb2_len()) {
			rb2_out(&v);
			msr_f2f_decode_track(msr, 1, v);
		}
		if (rb3_len()) {
			rb3_out(&v);
			msr_f2f_decode_track(msr, 2, v);
		}

#ifdef TRACE_ADC_DATA
		for(i = 0; i < MSR_NUM_TRACK; i++) {
			if(data->swip_start[i] && (data->swip_catch_aready[i] == 0))
			{
				data->swip_start_index[i] = data->track[i].rb.rd;
				data->swip_catch_aready[i] = 1;
			}
		}
#endif

		if((t1--) == 0 )
		{
			msrdebug(" !!! timeout waiting for swipe !!! \n");
			data->swip_timeout = 1;
			break;
		}

		for(i = 0; i < MSR_NUM_TRACK; i++) {
			if(!data->swip_end[i])
				msr_check_swipe(msr, i);
		}
	}
	/* FIXME:
	 * Bring in the option to test both 2-track and 3-track cards.
	 * track2 is not supported as some test cards may not have the third
	 * track. But for a card with 3rd track this should be uncommented.
	 */
	while (!(data->swip_end[0] && data->swip_end[1] /*&& data->swip_end[2]*/));


#ifdef TRACE_ADC_DATA
	for(i = 0; i < MSR_NUM_TRACK; i++)
		data->swip_end_index[i] = data->track[i].rb.rd;
#endif

	bcm5820x_msr_swipe_disable_adc(msr, adc);

	if(rb1_full)
		msrdebug(">> ringbuf1 full: %d\n", rb1_full);
	if(rb2_full)
		msrdebug(">> ringbuf2 full: %d\n", rb2_full);
	if(rb3_full)
		msrdebug(">> ringbuf3 full: %d\n", rb3_full);

	/* residual data...*/
	for (i = 0; i < MSR_NUM_TRACK; i++) {
		tk = &data->track[i]; /* track */
		if (tk->bcnt != 0) {
			while (tk->bcnt++ < tk->bpc) {
				tk->bytedata = tk->bytedata << 1;
			}
			tk->bytecnt++;
		}
	}

	/* Output the decoded data */
	data->swip_dir = msr_detect_dir(&data->track[0].mob, &data->track[1].mob);
	msrdebug("\nSwipe direction: *%s* \n\n", (data->swip_dir == MSR_FWD_SWIPE) ? "Forward" : (data->swip_dir == MSR_BWD_SWIPE) ? "Backward" : "Not identified");
	if (data->swip_dir == MSR_SWIPE_ABORT) {
		msrdebug("corrupted\n");
		ret = -EFAULT;
		goto exit;
	}
	else {
		/* output the track data, swiped in forward direction */
		for (i = 0; i < MSR_NUM_TRACK; i++) {
			if (data->swip_dir == MSR_FWD_SWIPE)
				flag = msr_forward_msr_output(i, &data->track[i].mob, data->track[i].bpc);
			else
				flag = msr_backward_msr_output(i, &data->track[i].mob, data->track[i].bpc);

			if (flag == 0) {
				msrdebug("data corrupted\n");
				/* return -1; */
			}
		}
	}

#ifdef TRACE_ADC_DATA
#ifdef DUMP_ADC_DATA
	if(swip_timeout == 0)
	{
		msrdebug("\n\n Preparing dump ADC samples... \n\n");
		/* 5s delay */
		k_busy_wait(5000000);

		msrdebug("\ntrack1 raw data:\n");
		msr_dump_buf(rb1, ADC_MSR_BSIZE);

		msrdebug("\ntrack2 raw data:\n");
		msr_dump_buf(rb2, ADC_MSR_BSIZE);

		msrdebug("\ntrack3 raw data:\n");
		msr_dump_buf(rb3, ADC_MSR_BSIZE);

		msrdebug("\nBuffer Index : [Start [%d] ~ End [%d] = Size [%d] %s]\n", data->swip_start_index[0], data->swip_end_index[0], \
			(data->swip_end_index[0] + ADC_MSR_BSIZE - data->swip_start_index[0]) % ADC_MSR_BSIZE, (data->swip_start_index[0] > data->swip_end_index[0]) ? "Rollover" : "");
	}
	else
	{
		msrdebug("\n Failed! No valid ADC samples dump! \n");
		ret = -EFAULT;
		goto exit;
	}
#endif
#endif

	/* if (data->swip_dir == MSR_FWD_SWIPE) */
	{
		/* For Backward, sometimes it is not correct as can't identify SS and ES byte dynamically. Maybe can use some other signal...*/
		msrdebug("\nSwipe Time : [Start [%d] ~ End [%d] = Total [%d] ms]\n", data->tick_start, data->tick_end, data->tick_end - data->tick_start);
	}

exit:
	/* Free the dynamic memory used for Ring Buffers */
	if (rb1)
		k_free(rb1);

	if (rb2)
		k_free(rb2);

	if (rb3)
		k_free(rb3);

	rb1 = NULL;
	rb2 = NULL;
	rb3 = NULL;

	return ret;
}

s32_t bcm5820x_msr_swipe_init(struct device *dev)
{
	ARG_UNUSED(dev);
	rb1 = NULL;
	rb2 = NULL;
	rb3 = NULL;
	return 0;
}

/** Instance of the MSR SWIPE Device Template
 *  Contains the default values
 */
static const struct msr_swipe_driver_api bcm5820x_msr_swipe_driver_api = {
	.configure_adc_info     = bcm5820x_msr_swipe_configure_adc_info,
	.init                   = bcm5820x_msr_swipe_init,
	.init_with_adc          = bcm5820x_msr_swipe_init_with_adc,
	.disable_adc            = bcm5820x_msr_swipe_disable_adc,
	.gather_data            = bcm5820x_msr_swipe_gather_data,
};

static const struct msr_swipe_device_config bcm5820x_msr_swipe_dev_cfg = {
	.adc_flextimer = 0,
	.numtracks = MSR_NUM_TRACK,
};

/** Private Data of the ADC
 *  Also contain the default values
 */
static struct bcm5820x_msr_swipe_dev_data_t bcm5820x_msr_swipe_dev_data = {
	.tick_start = 0,
	.tick_end = 0,
	.swip_dir = 0,
	.swip_start = {0},
	.swip_end = {0},
	.swip_start_index = {0},
	.swip_end_index = {0},
	.track_data_len = {MSR_TRACK_1_BYTE_NUM, MSR_TRACK_2_BYTE_NUM, MSR_TRACK_3_BYTE_NUM},
	.swip_timeout = 0,
	.swip_catch_aready = {0},
	.track = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {{0}, 0}}},
	.track_adc_info = {{0, 0}, {1, 0}, {2, 0}},
};

DEVICE_AND_API_INIT(bcm5820x_msr_swipe, CONFIG_MSR_NAME, bcm5820x_msr_swipe_init,
		&bcm5820x_msr_swipe_dev_data, &bcm5820x_msr_swipe_dev_cfg,
		PRE_KERNEL_2, CONFIG_MSR_SWIPE_DRIVER_INIT_PRIORITY,
		&bcm5820x_msr_swipe_driver_api);
