/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

#include <kernel.h>
#include <misc/util.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <logging/sys_log.h>
#include <broadcom/dma.h>
#include <board.h>
#include <string.h>
#include <usbh/usb_host.h>
#include "usb_host_bcm5820x.h"
#include "ohci.h"

volatile ohci_reg_t *ohci_reg = ((volatile ohci_reg_t *)(USBH_OHCI_BASE_ADDR));
/*
 *  Macros for dealing with hardware
 *
 *  This is all yucky stuff that needs to be made more
 *  processor-independent.  It's mostly here now to help us with
 *  our test harness.
 */
static ohci_transfer_t __aligned(ALIGNMENT) ohci_trnsf[OHCI_TDPOOL_SIZE];
static ohci_td_t __aligned(64) ohci_td_array[OHCI_TDPOOL_SIZE]; /*Always 64 */

static ohci_endpoint_t __aligned(ALIGNMENT) ohci_endp[OHCI_EDPOOL_SIZE];
static ohci_ed_t __aligned(64) ohci_ed_array[OHCI_EDPOOL_SIZE]; /*Always 64 */

static ohci_hcca_t __aligned(256) ohci_hcca_array[OHCI_EDPOOL_SIZE]; /*Always 256 */

/* In case of a transfer spanning multiple TDs raise ProcDone only on completion 
 * of last TD in transfer else device may end up losing sync
 */
#define MTD_DI_LAST

#define BSWAP32(x) (x)
#define OHCI_VTOP(ptr) ((u32_t)((ptr)))
#define OHCI_PTOV(ptr) ((void *)((ptr)))

/*
 * Debug message macro
 */

#ifdef _OHCI_DEBUG_
#define OHCIDEBUG(x)	do { if (ohcidebug > 0) x; } while (0)
#else
#define OHCIDEBUG(x)	do { } while (0)
#endif

static inline void ohci_write(volatile u32_t *ohci_regs, u32_t x, u32_t y)
{
	*((volatile u32_t *) (USB_OHCI_BASE + (x))) = (y);
}

static inline u32_t ohci_read(volatile u32_t *ohci_regs, u32_t x)
{
	u32_t val;

	val = *((volatile u32_t *) (USB_OHCI_BASE + (x)));

	return val;
}

#define ALIGN(n, align) (((n)+((align)-1)) & ~((align)-1))
void * ohci_dma_alloc(size_t size, unsigned int align)
{
	void *base;
	size_t len = ALIGN(size, align);

	base = cache_line_aligned_alloc(len);

	return base;
}

/*
 *  Bit-reverse table - this table consists of the numbers
 *  at its index, listed in reverse.  So, the reverse of 0000 0010
 *  is 0100 0000.
 */

static const int ohci_revbits[OHCI_INTTABLE_SIZE] = {
	0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
	0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
	0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
	0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f
};

/*
 *  Macros to convert from "hardware" endpoint and transfer
 *  descriptors (ohci_ed_t, ohci_td_t) to "software"
 *  data structures (ohci_transfer_t, ohci_endpoint_t).
 *
 *  Basically, there are two tables, indexed by the same value
 *  By subtracting the base of one pool from a pointer, we get
 *  the index into the other table.
 *
 *  We *could* have included the ed and td in the software
 *  data structures, but placing all the hardware stuff in one
 *  pool will make it easier for hardware that does not handle
 *  coherent DMA, since we can be less careful about what we flush
 *  and what we invalidate.
 */

#define ohci_td_from_transfer(softc, transfer) \
	((softc)->ohci_hwtdpool + ((transfer) - (softc)->ohci_transfer_pool))

#define ohci_transfer_from_td(softc, td) \
	((softc)->ohci_transfer_pool + ((td) - (softc)->ohci_hwtdpool))

#define ohci_ed_from_endpoint(softc, endpoint) \
	((softc)->ohci_hwedpool + ((endpoint) - (softc)->ohci_endpoint_pool))

#define ohci_endpoint_from_ed(softc, ed) \
	((softc)->ohci_endpoint_pool + ((ed) - (softc)->ohci_hwedpool))


/*
 *  Forward declarations
 */

static int ohci_roothub_xfer(struct usbbus *bus, void *uept, struct usbreq *ur);
static void ohci_roothub_statchg(ohci_softc_t *softc);

bool usb_is_itd(volatile ohci_td_t *td);

/*
 *  Globals
 */

#ifdef _OHCI_DEBUG_
void ohci_dumprhstat(u32_t reg);
void ohci_dumpportstat(int idx, u32_t reg);
void ohci_dumptd(ohci_softc_t *softc, ohci_td_t *td);
void ohci_dumptdchain(ohci_softc_t *softc, ohci_td_t *td);
void ohci_dumped(ohci_softc_t *softc, ohci_ed_t *ed);
void ohci_dumpedchain(ohci_softc_t *softc, ohci_ed_t *ed);
void ohci_dumpdoneq(ohci_softc_t *softc);
#endif

/*
 *  Some debug routines
 */

#ifdef _OHCI_DEBUG_
void ohci_dumprhstat(u32_t reg)
{
	printk("HubStatus: %08X  ", reg);

	if (reg & M_OHCI_RHSTATUS_LPS)
		printk("LocalPowerStatus ");
	if (reg & M_OHCI_RHSTATUS_OCI)
		printk("OverCurrent ");
	if (reg & M_OHCI_RHSTATUS_DRWE)
		printk("DeviceRemoteWakeupEnable ");
	if (reg & M_OHCI_RHSTATUS_LPSC)
		printk("LocalPowerStatusChange ");
	if (reg & M_OHCI_RHSTATUS_OCIC)
		printk("OverCurrentIndicatorChange ");
	printk("\n");
}

void ohci_dumpportstat(int idx, u32_t reg)
{
	printk("Port %d: %08X  ", idx, reg);
	if (reg & M_OHCI_RHPORTSTAT_CCS)
		printk("Connected ");
	if (reg & M_OHCI_RHPORTSTAT_PES)
		printk("PortEnabled ");
	if (reg & M_OHCI_RHPORTSTAT_PSS)
		printk("PortSuspended ");
	if (reg & M_OHCI_RHPORTSTAT_POCI)
		printk("PortOverCurrent ");
	if (reg & M_OHCI_RHPORTSTAT_PRS)
		printk("PortReset ");
	if (reg & M_OHCI_RHPORTSTAT_PPS)
		printk("PortPowered ");
	if (reg & M_OHCI_RHPORTSTAT_LSDA)
		printk("LowSpeed ");
	if (reg & M_OHCI_RHPORTSTAT_CSC)
		printk("ConnectStatusChange ");
	if (reg & M_OHCI_RHPORTSTAT_PESC)
		printk("PortEnableStatusChange ");
	if (reg & M_OHCI_RHPORTSTAT_PSSC)
		printk("PortSuspendStatusChange ");
	if (reg & M_OHCI_RHPORTSTAT_OCIC)
		printk("OverCurrentIndicatorChange ");
	if (reg & M_OHCI_RHPORTSTAT_PRSC)
		printk("PortResetStatusChange ");
	printk("\n");
}

const char *const ohci_td_pids[4] = { "SETUP", "OUT", "IN", "RSVD" };

void ohci_dumptd(ohci_softc_t *softc, ohci_td_t *td)
{
	u32_t ctl;

	ctl = BSWAP32(td->td_control);

	if (!usb_is_itd(td)) {
		printk
		    ("[%p] ctl=%p (DP=%s, DI=%d, T=%d, EC=%d, CC=%d%s) cbp=%p be=%p next=%p\n",
		     (void *)OHCI_VTOP(td), (void *)ctl,
		     ohci_td_pids[G_OHCI_TD_PID(ctl)], G_OHCI_TD_DI(ctl),
		     G_OHCI_TD_DT(ctl), G_OHCI_TD_EC(ctl), G_OHCI_TD_CC(ctl),
		     (ctl & M_OHCI_TD_SHORTOK) ? ", R" : "",
		     (void *)BSWAP32(td->td_cbp),
		     (void *)BSWAP32(td->td_be),
		     (void *)BSWAP32(td->td_next_td));
	} 
}

void ohci_dumptdchain(ohci_softc_t *softc, ohci_td_t *td)
{
	ohci_transfer_t *transfer;
	int idx = 0;

	for (;;) {
		ohci_dumptd(softc, td);
		if (!td->td_next_td)
			break;
		td = (ohci_td_t *) OHCI_PTOV(BSWAP32(td->td_next_td));
		idx++;
	}
}

const char *const ohci_ed_pids[4] = { "FTD", "OUT", "IN", "FTD" };

void ohci_dumped(ohci_softc_t *softc, ohci_ed_t *ed)
{
	u32_t ctl;

	ctl = BSWAP32(ed->ed_control);
	printk("[%p] Ctl=%x (MPS=%d%s%s%s, EN=%d, FA=%d, D=%s) ",
	       OHCI_VTOP(ed),
	       ctl,
	       G_OHCI_ED_MPS(ctl),
	       (ctl & M_OHCI_ED_LOWSPEED) ? ", LS" : "",
	       (ctl & M_OHCI_ED_SKIP) ? ", SKIP" : "",
	       (ctl & M_OHCI_ED_ISOCFMT) ? ", ISOC" : "",
	       G_OHCI_ED_EN(ctl),
	       G_OHCI_ED_FA(ctl), ohci_ed_pids[G_OHCI_ED_DIR(ctl)]);
	printk("Tailp=%x headp=%x next=%x %s\n",
	       BSWAP32(ed->ed_tailp),
	       BSWAP32(ed->ed_headp),
	       BSWAP32(ed->ed_next_ed),
	       BSWAP32(ed->ed_headp) & M_OHCI_ED_HALT ? "HALT" : "");
	if ((ed->ed_headp & M_OHCI_ED_PTRMASK) == 0)
		return;
	ohci_dumptdchain(softc,
			 OHCI_PTOV(BSWAP32(ed->ed_headp) & M_OHCI_ED_PTRMASK));
}

void ohci_dumpedchain(ohci_softc_t *softc, ohci_ed_t *ed)
{
	int idx = 0;

	for (;;) {
		printk("---\nED#%d -> ", idx);
		ohci_dumped(softc, ed);
		if (!ed->ed_next_ed)
			break;
		if (idx > 50)
			break;
		ed = (ohci_ed_t *) OHCI_PTOV(BSWAP32(ed->ed_next_ed));
		idx++;
	}
}

void ohci_dump_control(u32_t reg)
{
	printk("Control: %08X ", reg);
	printk("CBSR=%02x ", G_OHCI_CONTROL_CBSR(reg));
	if (reg & M_OHCI_CONTROL_PLE)
		printk("PLE ");
	if (reg & M_OHCI_CONTROL_IE)
		printk("IE ");
	if (reg & M_OHCI_CONTROL_CLE)
		printk("CLE ");
	if (reg & M_OHCI_CONTROL_BLE)
		printk("BLE ");
	printk("HCFS=%02x ", G_OHCI_CONTROL_HCFS(reg));
	if (reg & M_OHCI_CONTROL_IR)
		printk("IR ");
	if (reg & M_OHCI_CONTROL_RWC)
		printk("RWC ");
	if (reg & M_OHCI_CONTROL_RWE)
		printk("RWE ");
	printk("\n");
}

void ohci_dump_status(u32_t reg)
{
	printk("Status: %08X ", reg);
	if (reg & M_OHCI_CMDSTATUS_HCR)
		printk("HCR ");
	if (reg & M_OHCI_CMDSTATUS_CLF)
		printk("CLF ");
	if (reg & M_OHCI_CMDSTATUS_BLF)
		printk("BLF ");
	if (reg & M_OHCI_CMDSTATUS_OCR)
		printk("OCR ");
	if (reg & M_OHCI_CMDSTATUS_SOC)
		printk("SOC ");
	printk("\n");
}

void ohci_dump_int(char *name, u32_t reg)
{
	printk("%s: %08X ", name, reg);
	if (reg & M_OHCI_INT_SO)
		printk("SO ");
	if (reg & M_OHCI_INT_WDH)
		printk("WDH ");
	if (reg & M_OHCI_INT_SF)
		printk("SF ");
	if (reg & M_OHCI_INT_RD)
		printk("RD ");
	if (reg & M_OHCI_INT_UE)
		printk("UE ");
	if (reg & M_OHCI_INT_FNO)
		printk("FNO ");
	if (reg & M_OHCI_INT_RHSC)
		printk("RHSC ");
	printk("\n");
}

void ohci_dump_fminterval(u32_t reg)
{
	printk("FM Interval: %08X ", reg);
	if (reg & M_OHCI_FMINTERVAL_FIT)
		printk("FIT ");
	printk("FSMPS=%02x ", G_OHCI_FMINTERVAL_FSMPS(reg));
	printk("FI=%02x ", G_OHCI_FMINTERVAL_FI(reg));
	printk("\n");
}

void ohci_dump_fmremaining(u32_t reg)
{
	printk("FM Remaining: %08X ", reg);
	if (reg & M_OHCI_FMREMAINING_FRT)
		printk("FRT ");
	printk("FR=%02x ", G_OHCI_FMREMAINING_FR(reg));
	printk("\n");
}

void ohci_dumpdoneq(ohci_softc_t *softc)
{
	u32_t donehead = softc->ohci_hcca->hcca_donehead;
	u32_t doneq;
	ohci_td_t *td;

	doneq = BSWAP32(donehead);

	if (!doneq) {
		printk("NULL\n");
		return;
	}

	td = (ohci_td_t *) OHCI_PTOV(doneq);

	printk("Done Queue:\n");
	ohci_dumptdchain(softc, td);

}

static void eptstats(ohci_softc_t *softc)
{
	int cnt;
	ohci_endpoint_t *e;

	cnt = 0;

	e = softc->ohci_endpoint_freelist;
	while (e) {
		e = e->ep_next;
		cnt++;
	}
	printk("%d left, %d inuse\n", cnt, OHCI_EDPOOL_SIZE - cnt);
}

void ohci_dump_hcca(ohci_softc_t *softc)
{
	printk("HCCA:\n");
	ohci_dump_control(ohci_read(softc->ohci_regs, R_OHCI_CONTROL));
	ohci_dump_status(ohci_read(softc->ohci_regs, R_OHCI_CMDSTATUS));
	ohci_dump_int("Int Status ", ohci_read(softc->ohci_regs, R_OHCI_INTSTATUS));
	ohci_dump_int("Int Enable ", ohci_read(softc->ohci_regs, R_OHCI_INTENABLE));
	ohci_dump_int("Int Disable", ohci_read(softc->ohci_regs, R_OHCI_INTDISABLE));
	printk("hcca:			   %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_HCCA));
	printk("Period CurrentED:  %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_PERIODCURRENTED));
	printk("Control HeadED:	%08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_CONTROLHEADED));
	printk("Control CurrentED: %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_CONTROLCURRENTED));
	printk("Bulk HeadED:	   %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_BULKHEADED));
	printk("Bulk CurrentED:	%08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_BULKCURRENTED));
	printk("Done Head:		   %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_DONEHEAD));
	ohci_dump_fminterval(ohci_read(softc->ohci_regs, R_OHCI_FMINTERVAL));
	ohci_dump_fmremaining(ohci_read(softc->ohci_regs, R_OHCI_FMREMAINING));
	printk("FM Number:		   %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER));
	printk("Periodic Start:	%08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_PERIODICSTART));
	printk("LSThreshold:	   %08x\n",
	       ohci_read(softc->ohci_regs, R_OHCI_LSTHRESHOLD));
	ohci_dumprhstat(ohci_read(softc->ohci_regs, R_OHCI_RHSTATUS));
	ohci_dumpportstat(1, ohci_read(softc->ohci_regs, R_OHCI_RHPORTSTATUS(1)));
	ohci_dumpportstat(2, ohci_read(softc->ohci_regs, R_OHCI_RHPORTSTATUS(2)));

	printk("HCCA stats:\n");
	eptstats(softc);
	printk("done queue:\n");
	ohci_dumpdoneq(softc);
	printk("ctl list:\n");
	ohci_dumpedchain(softc,
			 ohci_ed_from_endpoint(softc, softc->ohci_ctl_list));
	printk("bulk list:\n");
	ohci_dumpedchain(softc,
			 ohci_ed_from_endpoint(softc, softc->ohci_bulk_list));
	printk("iso list:\n");
	ohci_dumpedchain(softc,
			 ohci_ed_from_endpoint(softc, softc->ohci_isoc_list));
	printk("inttable[0] list:\n");
	ohci_dumpedchain(softc,
			 ohci_ed_from_endpoint(softc, softc->ohci_inttable[0]));
}
#endif

bool usb_is_itd(volatile ohci_td_t *td)
{
	ARG_UNUSED(td);
	return 0;
}

/*
 *  Q_ENQUEUE(qb,item)
 *
 *  Add item to a queue
 *
 *  Input Parameters:
 *	qb - queue block
 *	item - item to add
 *
 *  Return Value:
 *	Nothing.
 */

static void q_enqueue(queue_t *qb, queue_t *item)
{
	qb->q_prev->q_next = item;
	item->q_next = qb;
	item->q_prev = qb->q_prev;
	qb->q_prev = item;
}

/*
 *  Q_DEQNEXT(qb)
 *
 *  Dequeue next element from the specified queue
 *
 *  Input Parameters:
 *	qb - queue block
 *
 *  Return Value:
 *	next element, or NULL
 */

static queue_t *q_deqnext(queue_t *qb)
{
	if (qb->q_next == qb)
		return NULL;

	qb = qb->q_next;

	qb->q_prev->q_next = qb->q_next;
	qb->q_next->q_prev = qb->q_prev;

	return qb;
}

/*
 *  ohci_allocept(softc)
 *
 *  Allocate an endpoint data structure from the pool, and
 *  make it ready for use.  The endpoint is NOT attached to
 *  the hardware at this time.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   pointer to endpoint or NULL
 */

static ohci_endpoint_t *ohci_allocept(ohci_softc_t *softc)
{
	ohci_endpoint_t *e;
	ohci_ed_t *ed;

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 2) {
		printk("AllocEpt: ");
		eptstats(softc);
	}
#endif

	e = softc->ohci_endpoint_freelist;

	if (!e) {
		printk("No endpoints left!\n");
		return NULL;
	}

	softc->ohci_endpoint_freelist = e->ep_next;

	ed = ohci_ed_from_endpoint(softc, e);

	ed->ed_control = BSWAP32(M_OHCI_ED_SKIP);
	ed->ed_tailp = BSWAP32(0);
	ed->ed_headp = BSWAP32(0);
	ed->ed_next_ed = BSWAP32(0);

	e->ep_phys = OHCI_VTOP(ed);
	e->ep_next = NULL;

	return e;
}

/*
 *  ohci_allocxfer(softc)
 *
 *  Allocate a transfer descriptor.	It is prepared for use
 *  but not attached to the hardware.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   transfer descriptor, or NULL
 */

static ohci_transfer_t *ohci_allocxfer(ohci_softc_t *softc)
{
	ohci_transfer_t *t;
	ohci_td_t *td;

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 2) {
		int cnt;

		cnt = 0;
		t = softc->ohci_transfer_freelist;
		while (t) {
			t = t->t_next;
			cnt++;
		}
		printk("AllocXfer: %d left, %d inuse\n", cnt,
		       OHCI_TDPOOL_SIZE - cnt);
	}
#endif

	t = softc->ohci_transfer_freelist;

	if (!t) {
		OHCIDEBUG(printk("No more transfer descriptors!\n"));
		return NULL;
	}

	softc->ohci_transfer_freelist = t->t_next;
	if(t->t_next == NULL) {	printk (" ====>A_ALERT \n");}

	/* both td and itd is allocated the same buffer chain. */
	td = (ohci_td_t *) ohci_td_from_transfer(softc, t);

	/* clear the whole TD buffer, 64 bytes. */
	td->td_control = BSWAP32(0);
	td->td_cbp     = BSWAP32(0);
	td->td_next_td = BSWAP32(0);
	 td->td_be      = BSWAP32(0);

	t->t_ref = NULL;
	t->t_next = NULL;

	return t;
}

/*
 *  ohci_freeept(softc, e)
 *
 *  Free an endpoint, returning it to the pool.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *	   e - endpoint descriptor to return
 *
 *  Return value:
 *	   nothing
 */

static void ohci_freeept(ohci_softc_t *softc, ohci_endpoint_t *e)
{
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 2) {
		int cnt;
		ohci_endpoint_t *ee;

		cnt = 0;
		ee = softc->ohci_endpoint_freelist;
		while (ee) {
			ee = ee->ep_next;
			cnt++;
		}
		printk("FreeEpt[%p]: %d left, %d inuse\n", e, cnt,
		       OHCI_EDPOOL_SIZE - cnt);
	}
#endif
	e->ep_next = softc->ohci_endpoint_freelist;
	softc->ohci_endpoint_freelist = e;
}

/*
 *  ohci_freexfer(softc, t)
 *
 *  Free a transfer descriptor, returning it to the pool.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *	   t - transfer descriptor to return
 *
 *  Return value:
 *	   nothing
 */

static void ohci_freexfer(ohci_softc_t *softc, ohci_transfer_t *t)
{


	t->t_next = softc->ohci_transfer_freelist;
	softc->ohci_transfer_freelist = t;
	if(t->t_next == NULL) {printk (" ====>F_ALERT t_next is NULL \n");}
}

/*
 *  ohci_initpools(softc)
 *
 *  Allocate and initialize the various pools of things that
 *  we use in the OHCI driver.  We do this by allocating some
 *  big chunks from the heap and carving them up.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   0 if ok
 *	   else error code
 */

static int ohci_initpools(ohci_softc_t *softc)
{
	int idx;

	/*
	 * In the case of noncoherent DMA, make the hardware-accessed
	 * pools use uncached addresses.  This way all our descriptors
	 * will be uncached.  Makes life easier, as we do not need to
	 * worry about flushing descriptors, etc.
	 */

	/*
	 * Do the transfer descriptor pool
	 */
#ifdef USE_STATIC_MEM_ALLOC
	softc->ohci_transfer_pool = (ohci_transfer_t *)(ohci_trnsf);
	softc->ohci_hwtdpool = (ohci_td_t *)(ohci_td_array);
#else 
	softc->ohci_transfer_pool = (ohci_transfer_t *)ohci_dma_alloc(OHCI_TDPOOL_SIZE * 
							sizeof(ohci_transfer_t), 64);
	softc->ohci_hwtdpool = (ohci_td_t *)ohci_dma_alloc(OHCI_TDPOOL_SIZE *
							sizeof(ohci_td_t), 64);
#endif
	if (!softc->ohci_transfer_pool || !softc->ohci_hwtdpool) {
		printk("Could not allocate transfer descriptors\n");
		return -1;
	}

	memset(softc->ohci_transfer_pool, 0, sizeof(ohci_trnsf));
	memset(softc->ohci_hwtdpool, 0, sizeof(ohci_td_array));

	softc->ohci_transfer_freelist = NULL;

	for (idx = 0; idx < OHCI_TDPOOL_SIZE; idx++) {
		ohci_freexfer(softc, softc->ohci_transfer_pool + idx);
	}

	/*
	 * Do the endpoint descriptor pool
	 */

#ifdef USE_STATIC_MEM_ALLOC
	softc->ohci_endpoint_pool = (ohci_endpoint_t *)(ohci_endp);
	softc->ohci_hwedpool = (ohci_ed_t *)(ohci_ed_array);
#else
	softc->ohci_endpoint_pool = (ohci_endpoint_t *)ohci_dma_alloc(OHCI_EDPOOL_SIZE *
					sizeof(ohci_endpoint_t), 64);
	softc->ohci_hwedpool = (ohci_ed_t *)ohci_dma_alloc(OHCI_EDPOOL_SIZE *
					sizeof(ohci_ed_t), 64);
#endif
	if (!softc->ohci_endpoint_pool || !softc->ohci_hwedpool) {
		printk("Could not allocate transfer descriptors\n");
		return -1;
	}

	memset(softc->ohci_endpoint_pool, 0, sizeof(ohci_endp));
	memset(softc->ohci_hwedpool, 0, sizeof(ohci_ed_array));

	softc->ohci_endpoint_freelist = NULL;

	for (idx = 0; idx < OHCI_EDPOOL_SIZE; idx++) {
		ohci_freeept(softc, softc->ohci_endpoint_pool + idx);
	}

	/*
	 * Finally the host communications area
	 */
#ifdef USE_STATIC_MEM_ALLOC
	softc->ohci_hcca = (ohci_hcca_t *)(ohci_hcca_array);
#else
	softc->ohci_hcca = (ohci_hcca_t *)ohci_dma_alloc(sizeof(ohci_hcca_t), 256);
#endif
	if (!softc->ohci_hcca) {
		printk("Could not allocate host communication area\n");
		return -1;
	}

#ifdef USE_STATIC_MEM_ALLOC
	memset(softc->ohci_hcca, 0, sizeof(ohci_hcca_array));
#else
	memset(softc->ohci_hcca, 0, sizeof(ohci_hcca_t));
#endif

	return 0;
}

/*
 *  ohci_start(bus)
 *
 *  Start the OHCI controller.  After this routine is called,
 *  the hardware will be operational and ready to accept
 *  descriptors and interrupt calls.
 *
 *  Input parameters:
 *	   bus - bus structure, from ohci_create
 *
 *  Return value:
 *	   0 if ok
 *	   else error code
 */

static int ohci_start(struct usbbus *bus)
{
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	u32_t frameint;
	u32_t reg;
	int idx;

	/*
	 * Force a reset to the controller, followed by a short delay
	 */
	ohci_write(softc->ohci_regs, R_OHCI_CONTROL,
		      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_RESET));
	usb_delay_ms(10);

	/* Host controller state is now "RESET" */

	/*
	 * We need the frame interval later, so get a copy of it now.
	 */
	frameint = G_OHCI_FMINTERVAL_FI(ohci_read(softc->ohci_regs, R_OHCI_FMINTERVAL));

	/*
	 * Reset the host controller.  When you set the HCR bit
	 * it self-clears when the reset is complete.
	 */

	ohci_write(softc->ohci_regs, R_OHCI_CMDSTATUS, M_OHCI_CMDSTATUS_HCR);
	for (idx = 0; idx < 10000; idx++) {
		if (!(ohci_read(softc->ohci_regs, R_OHCI_CMDSTATUS) &
		     M_OHCI_CMDSTATUS_HCR))
			break;
		return -1;
	}

	if (ohci_read(softc->ohci_regs, R_OHCI_CMDSTATUS) & M_OHCI_CMDSTATUS_HCR)
		return -1;

	/*
	 * Host controller state is now "SUSPEND".      We must exit
	 * from this state within 2ms.  (5.1.1.4)
	 *
	 * Set up pointers to data structures.
	 */

	ohci_write(softc->ohci_regs, R_OHCI_HCCA, OHCI_VTOP(softc->ohci_hcca));
	ohci_write(softc->ohci_regs, R_OHCI_CONTROLHEADED,
		      softc->ohci_ctl_list->ep_phys);
	ohci_write(softc->ohci_regs, R_OHCI_BULKHEADED, softc->ohci_bulk_list->ep_phys);

	/*
	 * Our driver is polled, turn off interrupts
	 */

	ohci_write(softc->ohci_regs, R_OHCI_INTDISABLE, M_OHCI_INT_ALL);

	/*
	 * Set up the control register.
	 */

	reg = ohci_read(softc->ohci_regs, R_OHCI_CONTROL);

	reg = M_OHCI_CONTROL_PLE | M_OHCI_CONTROL_CLE | M_OHCI_CONTROL_BLE |
	      M_OHCI_CONTROL_IE | M_OHCI_CONTROL_RWC |
	      M_OHCI_CONTROL_RWE |
	      V_OHCI_CONTROL_CBSR(K_OHCI_CBSR_41) |
	      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_OPERATIONAL);

	ohci_write(softc->ohci_regs, R_OHCI_CONTROL, reg);

	/*
	 * controller state is now OPERATIONAL
	 */

	reg = ohci_read(softc->ohci_regs, R_OHCI_FMINTERVAL);
	reg &= M_OHCI_FMINTERVAL_FIT;
	reg ^= M_OHCI_FMINTERVAL_FIT;
	reg |= V_OHCI_FMINTERVAL_FSMPS(OHCI_CALC_FSMPS(frameint)) |
	    V_OHCI_FMINTERVAL_FI(frameint);
	ohci_write(softc->ohci_regs, R_OHCI_FMINTERVAL, reg);

	reg = frameint * 90 / 100;	/* calculate 90% */
	ohci_write(softc->ohci_regs, R_OHCI_PERIODICSTART, reg);

	usb_delay_ms(10);

	/*
	 * Remember how many ports we have
	 */

	reg = ohci_read(softc->ohci_regs, R_OHCI_RHDSCRA);
	softc->ohci_ndp = G_OHCI_RHDSCRA_NDP(reg);

	/*
	 * Enable port power
	 */

	ohci_write(softc->ohci_regs, R_OHCI_RHSTATUS, M_OHCI_RHSTATUS_LPSC |
		      M_OHCI_RHSTATUS_DRWE);
	usb_delay_ms(10);

#ifdef USBHOST_INTERRUPT_MODE
	/* Enable interrupts */
	ohci_write(softc->ohci_regs, R_OHCI_INTENABLE, M_OHCI_INT_ALL);
	ohci_write(softc->ohci_regs, R_OHCI_INTDISABLE, M_OHCI_INT_SF);
	ohci_write(softc->ohci_regs, R_OHCI_INTDISABLE, M_OHCI_INT_FNO);
#endif
	return 0;
}

/*
 *  ohci_setupepts(softc)
 *
 *  Set up the endpoint tree, as described in the OHCI manual.
 *  Basically the hardware knows how to scan lists of lists,
 *  so we build a tree where each level is pointed to by two
 *  parent nodes.  We can choose our scanning rate by attaching
 *  endpoints anywhere within this tree.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   0 if ok
 *	   else error (out of descriptors)
 */

static int ohci_setupepts(ohci_softc_t *softc)
{
	int idx;
	ohci_endpoint_t *e;
	ohci_ed_t *ed;
	ohci_endpoint_t *child;

	/*
	 * Set up the list heads for the isochronous, control,
	 * and bulk transfer lists.  They don't get the same "tree"
	 * treatment that the interrupt devices get.
	 *
	 * For the purposes of CFE, it's probably not necessary
	 * to be this fancy.  The only device we're planning to
	 * talk to is the keyboard and some hubs, which should
	 * have pretty minimal requirements.  It's conceivable
	 * that this firmware may find a new home in other
	 * devices, so we'll meet halfway and do some things
	 * "fancy."
	 */

	softc->ohci_ctl_list = ohci_allocept(softc);
	softc->ohci_bulk_list = ohci_allocept(softc);
	softc->ohci_isoc_list = ohci_allocept(softc);

	/*
	 * Set up a tree of empty endpoint descriptors.  This is
	 * tree is scanned by the hardware from the leaves up to
	 * the root.  Once a millisecond, the hardware picks the
	 * next leaf and starts scanning descriptors looking
	 * for something to do.  It traverses all of the endpoints
	 * along the way until it gets to the root.
	 *
	 * The idea here is if you put a transfer descriptor on the
	 * root node, the hardware will see it every millisecond,
	 * since the root will be examined each time.  If you
	 * put the TD on the leaf, it will be 1/32 millisecond.
	 * The tree therefore is six levels deep.
	 */

	for (idx = 0; idx < OHCI_INTTREE_SIZE; idx++) {
		e = ohci_allocept(softc);	/* allocated with sKip bit set */
		softc->ohci_edtable[idx] = e;
		child =
		    (idx ==
		     0) ? softc->ohci_isoc_list : softc->ohci_edtable[(idx -
								       1) / 2];
		ed = ohci_ed_from_endpoint(softc, e);
		ed->ed_next_ed = BSWAP32(child->ep_phys);
		e->ep_next = child;
	}

	/*
	 * We maintain both physical and virtual copies of the interrupt
	 * table (leaves of the tree).
	 */

	for (idx = 0; idx < OHCI_INTTABLE_SIZE; idx++) {
		child =
		    softc->ohci_edtable[OHCI_INTTREE_SIZE - OHCI_INTTABLE_SIZE +
					idx];
		softc->ohci_inttable[ohci_revbits[idx]] = child;
		softc->ohci_hcca->hcca_inttable[ohci_revbits[idx]] =
		    BSWAP32(child->ep_phys);
	}

	/*
	 * Okay, at this point the tree is built.
	 */
	return 0;
}

/*
 *  ohci_stop(bus)
 *
 *  Stop the OHCI hardware.
 *
 *  Input parameters:
 *	   bus - our bus structure
 *
 *  Return value:
 *	   nothing
 */

static void ohci_stop(struct usbbus *bus)
{
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;

	ohci_write(softc->ohci_regs, R_OHCI_CONTROL,
		      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_RESET));
}

/*
 *  ohci_queueept(softc, queue, e)
 *
 *  Add an endpoint to a list of endpoints.	This routine
 *  does things in a particular way according to the OHCI
 *  spec so we can add endpoints while the hardware is running.
 *
 *  Input parameters:
 *	   queue - endpoint descriptor for head of queue
 *	   e - endpoint to add to queue
 *
 *  Return value:
 *	   nothing
 */

static void ohci_queueept(ohci_softc_t *softc, ohci_endpoint_t *queue,
			   ohci_endpoint_t *newept)
{
	ohci_ed_t *qed;
	ohci_ed_t *newed;

	qed = ohci_ed_from_endpoint(softc, queue);
	newed = ohci_ed_from_endpoint(softc, newept);

	newept->ep_next = queue->ep_next;
	newed->ed_next_ed = qed->ed_next_ed;

	queue->ep_next = newept;
	qed->ed_next_ed = BSWAP32(newept->ep_phys);

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1)
		ohci_dumped(softc, newed);
#endif
}

/*
 *  ohci_deqept(queue, e)
 *
 *  Remove and endpoint from the list of endpoints.	This
 *  routine does things in a particular way according to
 *  the OHCI specification, since we are operating on
 *  a running list.
 *
 *  Input parameters:
 *	   queue - base of queue to look for endpoint on
 *	   e - endpoint to remove
 *
 *  Return value:
 *	   nothing
 */

static void ohci_deqept(ohci_softc_t *softc, ohci_endpoint_t *queue,
			 ohci_endpoint_t *e)
{
	ohci_endpoint_t *cur;
	ohci_ed_t *cured;
	ohci_ed_t *ed;

	cur = queue;

	while (cur && (cur->ep_next != e))
		cur = cur->ep_next;

	if (cur == NULL) {
		OHCIDEBUG(printk
			  ("Could not remove EP %p: not on the list!\n", e));
		return;
	}

	/*
	 * Remove from our regular list
	 */

	cur->ep_next = e->ep_next;

	/*
	 * now remove from the hardware's list
	 */

	cured = ohci_ed_from_endpoint(softc, cur);
	ed = ohci_ed_from_endpoint(softc, e);

	cured->ed_next_ed = ed->ed_next_ed;
}

/*
 *  ohci_intr_procdoneq(softc)
 *
 *  Process the "done" queue for this ohci controller.  As
 *  descriptors are retired, the hardware links them to the
 *  "done" queue so we can examine the results.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   nothing
 */

static void ohci_intr_procdoneq(ohci_softc_t *softc)
{
	u32_t doneq;
	ohci_transfer_t *transfer;
	ohci_td_t *td;
	int ccVal = 0;
	struct usbreq *ur= NULL;

	/*
	 * Get the head of the queue
	 */
#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* this is messy but essential */
	invalidate_dcache_by_addr((u32_t)softc->ohci_hwedpool,
					 (u32_t)sizeof(ohci_ed_array));

	invalidate_dcache_by_addr((u32_t)softc->ohci_endpoint_pool,
					 (u32_t)sizeof(ohci_endp));

	invalidate_dcache_by_addr((u32_t)softc->ohci_hwtdpool,
					 (u32_t)sizeof(ohci_td_array));

	invalidate_dcache_by_addr((u32_t)softc->ohci_transfer_pool,
					 (u32_t)sizeof(ohci_trnsf));

	invalidate_dcache_by_addr((u32_t)softc->ohci_hcca,
					 (u32_t)sizeof(ohci_hcca_array));

#endif

	doneq = softc->ohci_hcca->hcca_donehead;
	doneq = BSWAP32(doneq);

	/* here is td or itd */
	td = (ohci_td_t *) (doneq);
	transfer = ohci_transfer_from_td(softc, td);

	/*
	 * Process all elements from the queue
	 */

	while (doneq) {
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/*each time a td and transfer is freed invalidate */
		invalidate_dcache_by_addr((u32_t)td, sizeof(ohci_td_t));
		invalidate_dcache_by_addr((u32_t)transfer, sizeof(ohci_transfer_t));
#endif

#ifdef _OHCI_DEBUG_
		ohci_ed_t *ed;
		ohci_endpoint_t *ept;
		struct usbreq *xur = transfer->t_ref;

		if (ohcidebug > 1) {
			if (xur) {
				ept =
				    (ohci_endpoint_t *) xur->ur_pipe->
				    up_hwendpoint;
				ed = ohci_ed_from_endpoint(softc, ept);
				printk("ProcDoneQ:ED [%08X] -> ", ept->ep_phys);
				ohci_dumped(softc, ed);
			}
		}

		/*
		 * Get the pointer to next one before freeing this one
		 */

		if (ohcidebug > 1) {
			ur = transfer->t_ref;
			printk("Done(%d): ", ur ? ur->ur_tdcount : -1);
			ohci_dumptd(softc, td);
		}
#endif

		ccVal = G_OHCI_TD_CC(BSWAP32(td->td_control));

		if (ccVal != 0)
			printk("[Transfer error: %d]\n", ccVal);

#ifdef _OHCI_DEBUG_
		if (ccVal != 0) {
			ur = transfer->t_ref;
			printk
			    ("[%p Transfer error: %d %s %04X/%02X len=%d/%d (%s)]\n",
			     ur, ccVal,
			     (ur->ur_flags & UR_FLAG_IN) ? "IN" : "OUT",
			     ur->ur_pipe ? ur->ur_pipe->up_flags : 0,
			     ur->ur_pipe ? ur->ur_pipe->up_num : 0,
			     ur->ur_xferred, ur->ur_length,
			     ur->ur_dev->ud_drv->udrv_name);
		}
#endif

		/*
		 * See if it's time to call the callback.
		 */
		ur = transfer->t_ref;
#ifdef CONFIG_DATA_CACHE_SUPPORT
		/*HW has written CPU will read */
		invalidate_dcache_by_addr((u32_t)ur, sizeof(struct usbreq));
#endif

		if (ur) {
			ur->ur_status = ccVal;
			ur->ur_tdcount--;

			if (usb_is_itd(td)) {
				if (ccVal == 0) {
					ur->ur_xferred += transfer->t_length;
				}
			} else {
				if (BSWAP32(td->td_cbp) == 0) {
					ur->ur_xferred += transfer->t_length;
				} else {
					ur->ur_xferred += transfer->t_length -
					    (BSWAP32(td->td_be) -
					     BSWAP32(td->td_cbp) + 1);
				}
			}

			if (ur->ur_tdcount == 0) {
				/* Noncoherent DMA: need to invalidate,
				 * since data is in phys mem
				 * If device was being removed, change
				 * return code to "cancelled"
				 */
				if (ur->ur_dev
				    && (ur->ur_dev->
					ud_flags & UD_FLAG_REMOVING)) {
					OHCIDEBUG(printk
						  ("Changing status of %p to CANCELLED\n",
						   ur));
					ccVal = K_OHCI_CC_CANCELLED;
				}
				usb_complete_request(ur, ccVal);
			}
		}

		/*
		 * Free up the request
		 */
		doneq = BSWAP32(td->td_next_td);
		ohci_freexfer(softc, transfer);

		/*
		 * Advance to the next request.
		 */

		td = (ohci_td_t *) OHCI_PTOV(doneq);
		transfer = ohci_transfer_from_td(softc, td);
	}

}

/*
 *  ohci_roothub_reset(softc, statport)
 *
 *  Reset the specified port on the root hub.
 *
 *  Input parameters:
 *	   softc - our softc
 *	   statport - port to reset
 *
 *  Return value:
 *	   N/A
 */

void ohci_roothub_reset(ohci_softc_t *softc, int statport)
{
	int i;

	ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PRS);
	for (i = 0; i < 5; i++) {	/* FIXME: timer */
		usb_delay_ms(100);
		if (!(ohci_read(softc->ohci_regs, statport) & M_OHCI_RHPORTSTAT_PRS))
			break;
	}
}

/*
 *  ohci_intr(bus)
 *
 *  Process pending interrupts for the OHCI controller.
 *
 *  Input parameters:
 *	   bus - our bus structure
 *
 *  Return value:
 *	   0 if we did nothing
 *	   nonzero if we did something.
 */

static int ohci_intr(struct usbbus *bus)
{
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	u32_t reg;
#ifdef USBHOST_INTERRUPT_MODE
	u32_t enabled;
#endif
	static u32_t intr_processing;

	if (intr_processing == 1) {
		return 0;
	}
	/*
	 * Read the interrupt status register.
	 */

	reg = ohci_read(softc->ohci_regs, R_OHCI_INTSTATUS);
#ifdef USBHOST_INTERRUPT_MODE
	enabled = ohci_read(softc->ohci_regs, R_OHCI_INTENABLE);
	/* only process enabled interrupts */
	reg &= enabled;
#endif

	/*
	 * Don't bother doing anything if nothing happened.
	 */
	if (reg == 0) {
		return 0;
	}

	intr_processing = 1;

	/* Don't clear interrupt bits until processing is complete
	 * possible race condition with WDH / HccaDoneHead updates here
	 */

	/* Scheduling Overruns */
	if (reg & M_OHCI_INT_SO) {
		reg &= ~M_OHCI_INT_SO;
		printk("SchedOverrun\n");
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_SO);
	}

	/* Done Queue */
	if (reg & M_OHCI_INT_WDH) {
		reg &= ~M_OHCI_INT_WDH;
		/* printk("DoneQueue\n"); */
		ohci_intr_procdoneq(softc);
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_WDH);
	}

	/* Start of Frame */
	if (reg & M_OHCI_INT_SF) {
		/* don't be noisy about this */
		reg &= ~M_OHCI_INT_SF;
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_SF);
	}

	/* Resume Detect */
	if (reg & M_OHCI_INT_RD) {
		reg &= ~M_OHCI_INT_RD;
		printk("ResumeDetect\n");
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_RD);
	}

	/* Unrecoverable errors */
	if (reg & M_OHCI_INT_UE) {
		reg &= ~M_OHCI_INT_UE;
		printk("UnrecoverableError\n");
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_UE);
	}

	/* Frame number overflow */
	if (reg & M_OHCI_INT_FNO) {
		/*printk("FrameNumberOverflow\n"); */
		reg &= ~M_OHCI_INT_FNO;
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_FNO);
	}

	/* Root Hub Status Change */
	if ((reg) & M_OHCI_INT_RHSC) {
		reg &= ~M_OHCI_INT_RHSC;
		printk("roothub_statchg\n");
		ohci_roothub_statchg(softc);
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_RHSC);
	}

	/* Ownership Change */
	if (reg & M_OHCI_INT_OC) {
		reg &= ~M_OHCI_INT_OC;
		printk("OwnershipChange\n");
		ohci_write(softc->ohci_regs, R_OHCI_INTSTATUS, M_OHCI_INT_OC);
	}

	if (reg) {
		printk("ohci reg=%08x\n", reg);
		if (reg & M_OHCI_INT_RHSC) {
			printk("softc->ohci_intdisable=%d\n",
			       softc->ohci_intdisable);
		}
	}

	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PES) {
			if (flag) {
				flag = 0;
				printk("OHCI port enable rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PSS) {
			if (flag) {
				flag = 0;
				printk("OHCI port suspend rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PRS) {
			if (flag) {
				flag = 0;
				printk("OHCI port reset rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_CSC) {
			if (flag) {
				flag = 0;
				printk("OHCI port connect change rhport00=%x\n",
				       ohci_reg->rhport00);

				/* t3(TATTDB) - debounce interval, at least 100ms from usb2.0 spec. */
				usb_delay_ms(100);

				/* usbhohci_port_resume() */
				reg = ohci_reg->rhport00;
				reg =
				    M_OHCI_RHPORTSTAT_PRS |
				    M_OHCI_RHPORTSTAT_PES;
				reg &= ~M_OHCI_RHPORTSTAT_PSS;
				ohci_reg->rhport00 = reg;

				usb_delay_ms(20);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PESC) {
			if (flag) {
				flag = 0;
				printk("OHCI port enable change rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PSSC) {
			if (flag) {
				flag = 0;
				printk("OHCI port suspend change rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}
	{
		static int flag = 1;

		if (ohci_reg->rhport00 & M_OHCI_RHPORTSTAT_PRSC) {
			if (flag) {
				flag = 0;
				printk("OHCI port reset change rhport00=%x\n",
				       ohci_reg->rhport00);
			}
		} else {
			flag = 1;
		}
	}

	intr_processing = 0;

	return 1;
}

/*
 *  ohci_delete(bus)
 *
 *  Remove an OHCI bus structure and all resources allocated to
 *  it (used when shutting down USB)
 *
 *  Input parameters:
 *	   bus - our USB bus structure
 *
 *  Return value:
 *	   nothing
 */

static void ohci_delete(struct usbbus *bus)
{
	ARG_UNUSED(bus);
	/* TODO: Do nothing? */
}

/*
 *  ohci_create(addr)
 *
 *  Create a USB bus structure and associate it with our OHCI
 *  controller device.
 *
 *  Input parameters:
 *	   addr - physical address of controller
 *
 *  Return value:
 *	   usbbus structure pointer
 */

static struct usbbus *ohci_create(u32_t addr)
{
	ohci_softc_t *softc;
	struct usbbus *bus;

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* Entry point to OHCI flush the dcache */
	clean_n_invalidate_dcache();
#endif

	softc = (ohci_softc_t *)cache_line_aligned_alloc(sizeof (ohci_softc_t));
	if (!softc)
		return NULL;

	bus = (struct usbbus *)cache_line_aligned_alloc(sizeof (struct usbbus));
	if (!bus)
		return NULL;

	memset(softc, 0, sizeof(ohci_softc_t));
	memset(bus, 0, sizeof(struct usbbus));

	bus->ub_hwsoftc = (ohci_softc_t *)softc;
	bus->ub_hwdisp = (struct usb_hcd *)&ohci_driver;

	q_init(&(softc->ohci_rh_intrq));
	softc->ohci_regs = (volatile u32_t *)addr;

	softc->ohci_rh_newaddr = -1;
	softc->ohci_bus = bus;

	if ((ohci_initpools(softc)) != 0) {
		SYS_LOG_ERR("Error initializing memory pools\n");
		goto error;
	}

	if ((ohci_setupepts(softc)) != 0) {
		SYS_LOG_ERR("Error setting up endpoints\n");
		goto error;
	}

	ohci_write(softc->ohci_regs, R_OHCI_CONTROL,
		      V_OHCI_CONTROL_HCFS(K_OHCI_HCFS_RESET));

	return bus;

error:
	k_free(softc);
	return NULL;
}

/*
 *  ohci_ept_create(bus, usbaddr, eptnum, mps, flags)
 *
 *  Create a hardware endpoint structure and attach it to
 *  the hardware's endpoint list.  The hardware manages lists
 *  of queues, and this routine adds a new queue to the appropriate
 *  list of queues for the endpoint in question.  It roughly
 *  corresponds to the information in the OHCI specification.
 *
 *  Input parameters:
 *	   bus - the USB bus we're dealing with
 *	   usbaddr - USB address (0 means default address)
 *	   eptnum - the endpoint number
 *	   mps - the packet size for this endpoint
 *	   flags - various flags to control endpoint creation
 *
 *  Return value:
 *	   endpoint structure poihter, or NULL
 */

static void *ohci_ept_create(struct usbbus *bus,
				  int usbaddr, int eptnum, int mps, int flags)
{
	u32_t eptflags;
	ohci_endpoint_t *ept;
	ohci_ed_t *ed;
	ohci_transfer_t *tailtransfer;
	ohci_td_t *tailtd;

	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;

	ept = ohci_allocept(softc);
	ed = ohci_ed_from_endpoint(softc, ept);

	tailtransfer = ohci_allocxfer(softc);
	tailtd = ohci_td_from_transfer(softc, tailtransfer);

	/*
	 * Set up functional address, endpoint number, and packet size
	 */

	eptflags = V_OHCI_ED_FA(usbaddr) |
	    V_OHCI_ED_EN(eptnum) | V_OHCI_ED_MPS(mps) | 0;

	/*
	 * Set up the endpoint type based on the flags
	 * passed to us
	 */

	if (flags & UP_TYPE_IN) {
		eptflags |= V_OHCI_ED_DIR(K_OHCI_ED_DIR_IN);
	} else if (flags & UP_TYPE_OUT) {
		eptflags |= V_OHCI_ED_DIR(K_OHCI_ED_DIR_OUT);
	} else {
		eptflags |= V_OHCI_ED_DIR(K_OHCI_ED_DIR_FROMTD);
	}

	/*
	 * Don't forget about lowspeed devices.
	 */

	if (flags & UP_TYPE_LOWSPEED) {
		eptflags |= M_OHCI_ED_LOWSPEED;
	}

	/* enable this EP as isochronous transfer. */
	if (flags & UP_TYPE_ISOC) {
		eptflags |= M_OHCI_ED_ISOCFMT;
	}
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 0) {
		printk("Create endpoint %d addr %d flags %08X mps %d\n",
		       eptnum, usbaddr, eptflags, mps);
	}
#endif

	/*
	 * Transfer this info into the endpoint descriptor.
	 * No need to flush the cache here, it'll get done when
	 * we add to the hardware list.
	 */

	ed->ed_control = BSWAP32(eptflags);
	ed->ed_tailp = BSWAP32(OHCI_VTOP(tailtd));
	ed->ed_headp = BSWAP32(OHCI_VTOP(tailtd));
	ept->ep_flags = flags;
	ept->ep_mps = mps;
	ept->ep_num = eptnum;

	/*
	 * Put it on the right queue
	 */

	if (flags & UP_TYPE_CONTROL) {
		ohci_queueept(softc, softc->ohci_ctl_list, ept);
	} else if (flags & UP_TYPE_BULK) {
		ohci_queueept(softc, softc->ohci_bulk_list, ept);
	} else if (flags & UP_TYPE_INTR) {
		/* FIXME: Choose place in inttable properly. */
		ohci_queueept(softc, softc->ohci_inttable[0], ept);
	} else if (flags & UP_TYPE_ISOC) {
#if 0
		/* put in isoc list, which is attached in ohci_inttable[0] */
		ohci_queueept(softc, softc->ohci_isoc_list, ept);
#endif
		printk("WARNING: ISOC not supported\n");
		return NULL;
	}
	return (void *) ept;
}

/*
 *  ohci_ept_setaddr(bus, ept, usbaddr)
 *
 *  Change the functional address for a USB endpoint.  We do this
 *  when we switch the device's state from DEFAULT to ADDRESSED
 *  and we've already got the default pipe open.  This
 *  routine mucks with the descriptor and changes its address
 *  bits.
 *
 *  Input parameters:
 *	   bus - usb bus structure
 *	   ept - an open endpoint descriptor
 *	   usbaddr - new address for this endpoint
 *
 *  Return value:
 *	   nothing
 */

static void ohci_ept_setaddr(struct usbbus *bus, void *uept, int usbaddr)
{
	u32_t eptflags;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);

	eptflags = BSWAP32(ed->ed_control);
	eptflags &= ~M_OHCI_ED_FA;
	eptflags |= V_OHCI_ED_FA(usbaddr);
	ed->ed_control = BSWAP32(eptflags);
}

/*
 *  ohci_ept_setmps(bus, ept, mps)
 *
 *  Set the maximum packet size of this endpoint.  This is
 *  normally used during the processing of endpoint 0 (default
 *  pipe) after we find out how big ep0's packets can be.
 *
 *  Input parameters:
 *	   bus - our USB bus structure
 *	   ept - endpoint structure
 *	   mps - new packet size
 *
 *  Return value:
 *	   nothing
 */

static void ohci_ept_setmps(struct usbbus *bus, void *uept, int mps)
{
	u32_t eptflags;
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);

	eptflags = BSWAP32(ed->ed_control);
	eptflags &= ~M_OHCI_ED_MPS;
	eptflags |= V_OHCI_ED_MPS(mps);
	ed->ed_control = BSWAP32(eptflags);
	ept->ep_mps = mps;

}

/*
 *  ohci_ept_cleartoggle(bus, ept, mps)
 *
 *  Clear the data toggle for the specified endpoint.
 *
 *  Input parameters:
 *	   bus - our USB bus structure
 *	   ept - endpoint structure
 *
 *  Return value:
 *	   nothing
 */

static void ohci_ept_cleartoggle(struct usbbus *bus, void *uept)
{
	u32_t eptflags;
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);

	eptflags = BSWAP32(ed->ed_headp);
	eptflags &= ~(M_OHCI_ED_HALT | M_OHCI_ED_TOGGLECARRY);
	ed->ed_headp = BSWAP32(eptflags);

	ohci_write(softc->ohci_regs, R_OHCI_CMDSTATUS, M_OHCI_CMDSTATUS_CLF);
}

/*
 *  ohci_ept_delete(bus, ept)
 *
 *  Deletes an endpoint from the OHCI controller.  This
 *  routine also completes pending transfers for the
 *  endpoint and gets rid of the hardware ept (queue base).
 *
 *  Input parameters:
 *	   bus - ohci bus structure
 *	   ept - endpoint to remove
 *
 *  Return value:
 *	   nothing
 */

static void ohci_ept_delete(struct usbbus *bus, void *uept)
{
	ohci_endpoint_t *queue=NULL;
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);
	u32_t framenum;
	u32_t tdphys;
	struct usbreq *ur;
	ohci_td_t *td;
	ohci_transfer_t *transfer;

	if (ept->ep_flags & UP_TYPE_CONTROL) {
		queue = softc->ohci_ctl_list;
	} else if (ept->ep_flags & UP_TYPE_BULK) {
		queue = softc->ohci_bulk_list;
	} else if (ept->ep_flags & UP_TYPE_INTR) {
		queue = softc->ohci_inttable[0];
	} else if (ept->ep_flags & UP_TYPE_ISOC) {
#if 0
		queue = softc->ohci_isoc_list;
#endif
		printk("WARNING: ISOC not supported\n");

	} else {
		OHCIDEBUG(printk("Invalid endpoint\n"));
		return;
	}

	/*
	 * Set the SKIP bit on the endpoint and
	 * wait for two SOFs to guarantee that we're
	 * not processing this ED anymore.
	 */

	ed->ed_control =
	    (volatile u32_t)ed->ed_control | BSWAP32(M_OHCI_ED_SKIP);

	framenum = ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF;
	while ((ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF) == framenum)
		;/* NULL LOOP */

	framenum = ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF;
	while ((ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF) == framenum)
		;/* NULL LOOP */

	/*
	 * Remove endpoint from queue
	 */

	ohci_deqept(softc, queue, ept);

	/*
	 * Free/complete the TDs on the queue
	 */

	tdphys = BSWAP32(ed->ed_headp) & M_OHCI_ED_PTRMASK;

	while (tdphys != BSWAP32(ed->ed_tailp)) {
		td = (ohci_td_t *) OHCI_PTOV(tdphys);
		tdphys = BSWAP32(td->td_next_td);
		transfer = ohci_transfer_from_td(softc, td);

		ur = transfer->t_ref;
		if (ur) {
			ur->ur_status = K_OHCI_CC_CANCELLED;
			ur->ur_tdcount--;
			if (ur->ur_tdcount == 0) {
#ifdef _OHCI_DEBUG_
				if (ohcidebug > 0)
					printk
					    ("dev %p Completing request due to closed pipe: %p (%s, %04X/%02X, %s)\n",
					     ur->ur_dev, ur,
					     (ur->
					      ur_flags & UR_FLAG_IN) ? "IN" :
					     "OUT", ept->ep_flags, ept->ep_num,
					     ur->ur_dev->ud_drv->udrv_name);
#endif
				usb_complete_request(ur, K_OHCI_CC_CANCELLED);
				/* FIXME: it is expected that the callee will free the usbreq. */
			}
		}

		ohci_freexfer(softc, transfer);
	}

	/*
	 * tdphys now points at the tail TD.  Just free it.
	 */

	td = (ohci_td_t *) OHCI_PTOV(tdphys);
	ohci_freexfer(softc, ohci_transfer_from_td(softc, td));

	/*
	 * Return endpoint to free pool
	 */

	ohci_freeept(softc, ept);
}

/*
 *  ohci_cancel_xfer(bus, ept, ur)
 *
 *  Remove a transfer from the specified endpoint.
 *  Very similar to ohci_ept_delete, but we're just taking down the
 *  td-s and not the ed itself
 *
 *  Input parameters:
 *	   bus - bus structure
 *	   ept - endpoint descriptor
 *	   ur - request (includes pointer to user buffer)
 *
 *  Return value:
 *	   0 if ok
 *	   else error
 */

static int ohci_cancel_xfer(struct usbbus *bus, void *uept, struct usbreq *ur)
{
	ohci_endpoint_t *queue;
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);
	u32_t framenum;
	u32_t tdphys;
	ohci_td_t *td;
	volatile u32_t *tdp;
	ohci_transfer_t *transfer;
	u32_t togglecarry = 0;

	printk("ohci_cancel_xfer(ur=%p)\n", ur);
	if (ept->ep_flags & UP_TYPE_CONTROL) {
		queue = softc->ohci_ctl_list;
	} else if (ept->ep_flags & UP_TYPE_BULK) {
		queue = softc->ohci_bulk_list;
	} else if (ept->ep_flags & UP_TYPE_INTR) {
		queue = softc->ohci_inttable[0];
	} else if (ept->ep_flags & UP_TYPE_ISOC) {
#if 0
		queue = softc->ohci_isoc_list;
#endif
		printk("WARNING: ISOC not supported\n");
	} else {
		OHCIDEBUG(printk("Invalid endpoint\n"));
		return -1;
	}

	if (queue)
		SYS_LOG_DBG("ohci_cancel_xfer(ur=%p))\n", ur);
	/*
	 * Set the SKIP bit on the endpoint and
	 * wait for two SOFs to guarantee that we're
	 * not processing this ED anymore.
	 */

	ed->ed_control |= BSWAP32(M_OHCI_ED_SKIP);

	framenum = ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF;
	while ((ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF) == framenum)
		;/* NULL LOOP */

	framenum = ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF;
	while ((ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) & 0xFFFF) == framenum)
		;/* NULL LOOP */

	/*
	 * Find the TDs corresponding to this request, and complete them
	 * with a cancelled status.
	 */

	tdp = (u32_t *)&ed->ed_headp;
	tdphys = BSWAP32(*tdp) & M_OHCI_ED_PTRMASK;
	togglecarry = BSWAP32(*tdp) & M_OHCI_ED_TOGGLECARRY;

	while (tdphys != BSWAP32(ed->ed_tailp)) {
		td = (ohci_td_t *) OHCI_PTOV(tdphys);
		transfer = ohci_transfer_from_td(softc, td);
		if (ur == transfer->t_ref) {
			printk("\tfreeing transfer %p\n", transfer);
			/* remove the TD from the ED chain */
			*tdp = td->td_next_td;
			/* free the transfer */
			ohci_freexfer(softc, transfer);
		} else
			tdp = &td->td_next_td;
		tdphys = BSWAP32(*tdp);
	}
#ifdef _OHCI_DEBUG_
	printk
	    ("\tdev %p Completing request due to cancel: %p (%s, %04X/%02X, %s)\n",
	     ur->ur_dev, ur, (ur->ur_flags & UR_FLAG_IN) ? "IN" : "OUT",
	     ept->ep_flags, ept->ep_num, ur->ur_dev->ud_drv->udrv_name);
#endif
	ur->ur_status = K_OHCI_CC_CANCELLED;
	ur->ur_tdcount = 0;
	printk("=%1x", ur->ur_tdcount);
	usb_complete_request(ur, K_OHCI_CC_CANCELLED);
	/* it is expected that the callee will free the usbreq. */

	ed->ed_headp |= BSWAP32(togglecarry);
	ed->ed_control &= ~BSWAP32(M_OHCI_ED_SKIP);

	return 0;
}

/*
 *  ohci_iso_xfer(bus, ept, ur)
 *
 *  Queue a isochronous transfer for the specified endpoint.  Depending on
 *  the transfer type, the transfer may go on one of many queues.
 *  When the transfer completes, a callback will be called.
 *
 *  Input parameters:
 *	   bus - bus structure
 *	   ept - endpoint descriptor
 *	   ur - request (includes pointer to user buffer)
 *
 *  Return value:
 *	   0 if ok
 *	   else error
 */

static int ohci_iso_xfer(struct usbbus *bus, void *uept, struct usbreq *ur)
{
	printk("WARNING: ISOC not supported \n");
	ARG_UNUSED(bus);
	ARG_UNUSED(uept);
	ARG_UNUSED(ur);
	return 0;

#if 0
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);
	ohci_transfer_t *newtailtransfer = 0;
	ohci_transfer_t *curtransfer;
	ohci_utd_t *curutd, *newtailutd = NULL;
	u8_t *ptr;
	u32_t tdcontrol = 0;
	u16_t offset[OHCI_ITD_MAX_FC];
	u16_t startFrame, frameCount;
	int i, len, pktlen, amtcopy, bufptr = 0;

	ptr = ur->ur_buffer;
	len = ur->ur_length;
	ur->ur_tdcount = 0;
	pktlen = ur->ur_pipe->up_mps;

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk
		    (">> Queueing xfer addr %d pipe %d ED %p ptr %p length %d\n",
		     ur->ur_dev->ud_address, ur->ur_pipe->up_num, ept->ep_phys,
		     ptr, len);
		ohci_dumped(softc, ed);
	}
#endif

	startFrame = ohci_read(softc->ohci_regs, R_OHCI_FMNUMBER) + 10;

	curutd = (ohci_utd_t *) OHCI_PTOV(BSWAP32(ed->ed_tailp));
	curtransfer = ohci_transfer_from_utd(softc, curutd);

	if (len == 0) {
		newtailtransfer = ohci_allocxfer(softc);
		newtailutd =
		    (ohci_utd_t *) ohci_utd_from_transfer(softc,
							  newtailtransfer);

		curutd->type = OHCI_TYPE_ITD;

		tdcontrol = V_OHCI_ITD_SF(startFrame) |
		    V_OHCI_TD_DI(1) |
		    V_OHCI_ITD_FC(0) | V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED);

		curutd->u.itd.itd_bp0 = 0;
		curutd->u.itd.itd_be = 0;
		curutd->u.itd.itd_next_itd = BSWAP32(OHCI_VTOP(newtailutd));
		curutd->u.itd.itd_control = BSWAP32(tdcontrol);
		curtransfer->t_next = newtailtransfer;
		curtransfer->t_ref = ur;
		curtransfer->t_length = 0;
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 1) {
			printk("QueueTD: ");
			ohci_dumptd(softc, curutd);
		}
#endif
		ur->ur_tdcount++;
	} else {
		while (len > 0) {
			int frag;

			/* the max transfer size for each ITD is 8K, so the
			 * buffer point is only allowed cross one 4K page
			 * boundary.
			 */
			amtcopy = len;
			if (amtcopy > (pktlen * OHCI_ITD_MAX_FC))
				bufptr = amtcopy = pktlen * OHCI_ITD_MAX_FC;

			/* td can only cross one 4k boundary, so max is
			 * remainder of this page
			 * (4k - (ptr & (4k - 1))) + 4k
			 */
			frag = OHCI_VTOP(ptr) & ((4 * 1024) - 1);
			if (frag) {
				int maxcopy = (4 * 1024 - frag) + (4 * 1024);

				if (amtcopy > maxcopy) {
					/* each transfer size should be endpoint maxPacketSize. */
					amtcopy = (maxcopy / pktlen) * pktlen;

					/* adapt buffer point address to 4K align to avoid it cross two 4K boundary. */
					bufptr = maxcopy;
				}
			}

			/* transfer packet count for this ITD. */
			frameCount = amtcopy / pktlen;

			newtailtransfer = ohci_allocxfer(softc);
			newtailutd =
			    ohci_utd_from_transfer(softc, newtailtransfer);

			/* to differentiate the TD type in doneheader queue. */
			curutd->type = OHCI_TYPE_ITD;

			tdcontrol = V_OHCI_ITD_SF(startFrame) |
			    V_OHCI_TD_DI(1) |
			    V_OHCI_ITD_FC(frameCount - 1) |
			    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED);

			curutd->u.itd.itd_bp0 = BSWAP32(OHCI_VTOP(ptr));
			curutd->u.itd.itd_be =
			    BSWAP32(OHCI_VTOP(ptr + amtcopy) - 1);
			curutd->u.itd.itd_next_itd =
			    BSWAP32(OHCI_VTOP(newtailutd));
			curutd->u.itd.itd_control = BSWAP32(tdcontrol);

			memset(offset, 0, sizeof(offset));
			for (i = 0; i < frameCount; i++) {
				u32_t addr = (u32_t) ptr + i * pktlen;

				offset[i] = addr & 0x0fff;
				if ((addr & 0xfffff000) ==
				    (curutd->u.itd.itd_be & 0xfffff000)) {
					/* bit12 is for upper 20 bits address selection, see openHCI spec. */
					offset[i] |= 0x1000;
				}
				offset[i] |= 0xe000;
			}
			curutd->u.itd.itd_offsetPSW10 =
			    (offset[1] << 16) | offset[0];
			curutd->u.itd.itd_offsetPSW32 =
			    (offset[3] << 16) | offset[2];
			curutd->u.itd.itd_offsetPSW54 =
			    (offset[5] << 16) | offset[4];
			curutd->u.itd.itd_offsetPSW76 =
			    (offset[7] << 16) | offset[6];

			curtransfer->t_next = newtailtransfer;
			curtransfer->t_ref = ur;
			curtransfer->t_length = amtcopy;
#ifdef _OHCI_DEBUG_
			if (ohcidebug > 1) {
				printk("QueueTD: ");
				ohci_dumptd(softc, curutd);
			}
#endif
			curutd = newtailutd;
			curtransfer = newtailtransfer;

			/* buffer point address increasing is not same as transfer length. */
			ptr += bufptr;
			len -= amtcopy;
			ur->ur_tdcount++;

			/* next transfer frame interval. */
			startFrame += frameCount;
		}
	}

	curutd = OHCI_PTOV(BSWAP32(ed->ed_headp & M_OHCI_ED_PTRMASK));
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("======= HCCA before queue: =======\n");
		ohci_dump_hcca(softc);
	}
#endif

	ed->ed_tailp = BSWAP32(OHCI_VTOP(newtailutd));
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("======= HCCA after queue: =======\n");
		ohci_dump_hcca(softc);
	}
#endif

	/*
	 * Clear halted state
	 */

	ed->ed_headp &= BSWAP32(~M_OHCI_ED_HALT);
	ed->ed_control &= BSWAP32(~M_OHCI_ED_SKIP);
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("======= HCCA after halt-clear: =======\n");
		ohci_dump_hcca(softc);
	}
#endif

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("======= HCCA after prod: =======\n");
		ohci_dump_hcca(softc);
	}
#endif
	return 0;
#endif

}

/*
 *  ohci_xfer(bus, ept, ur)
 *
 *  Queue a transfer for the specified endpoint.  Depending on
 *  the transfer type, the transfer may go on one of many queues.
 *  When the transfer completes, a callback will be called.
 *
 *  Input parameters:
 *	   bus - bus structure
 *	   ept - endpoint descriptor
 *	   ur - request (includes pointer to user buffer)
 *
 *  Return value:
 *	   0 if ok
 *	   else error
 */

static int ohci_xfer(struct usbbus *bus, void *uept, struct usbreq *ur)
{
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	ohci_ed_t *ed = ohci_ed_from_endpoint(softc, ept);
	ohci_transfer_t *newtailtransfer = 0;
	ohci_transfer_t *curtransfer;
	ohci_td_t *curtd, *newtailtd = NULL;
	u8_t *ptr;
	u32_t len;
	u32_t amtcopy;
	u32_t pktlen;
	u32_t tdcontrol = 0;
	u8_t multi_td = 0;

	/* different handler for isochronous transfer. */
	if (ept->ep_flags & UP_TYPE_ISOC) {
		return ohci_iso_xfer(bus, uept, ur);
	}

	/*
	 * If the destination USB address matches
	 * the address of the root hub, shunt the request
	 * over to our root hub emulation.
	 */

	if (ur->ur_dev->ud_address == softc->ohci_rh_addr) {
		return ohci_roothub_xfer(bus, uept, ur);
	}

	/*Check if the transfer will span multiple td */
	if (ur->ur_length > OHCI_TD_MAX_DATA) {
		multi_td = 1;
	}

	pktlen = OHCI_TD_MAX_DATA;

	/*
	 * Set up the TD flags based on the
	 * request type.
	 */
	/*
	 * defaults for all types of packet types
	 */
	if (ur->ur_flags & UR_FLAG_SETUP) {
		tdcontrol = V_OHCI_TD_PID(K_OHCI_TD_SETUP) |
		    V_OHCI_TD_DT(K_OHCI_TD_DT_DATA0) |
		    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED) | V_OHCI_TD_DI(1);
	} else if (ur->ur_flags & UR_FLAG_IN) {
		tdcontrol = V_OHCI_TD_PID(K_OHCI_TD_IN) |
		    V_OHCI_TD_DT(K_OHCI_TD_DT_TCARRY) |
		    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED) | V_OHCI_TD_DI(1);
	} else if (ur->ur_flags & UR_FLAG_OUT) {
		tdcontrol = V_OHCI_TD_PID(K_OHCI_TD_OUT) |
		    V_OHCI_TD_DT(K_OHCI_TD_DT_TCARRY) |
		    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED) | V_OHCI_TD_DI(1);
	} else if (ur->ur_flags & UR_FLAG_STATUS_OUT) {
		tdcontrol = V_OHCI_TD_PID(K_OHCI_TD_OUT) |
		    V_OHCI_TD_DT(K_OHCI_TD_DT_DATA1) |
		    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED) | V_OHCI_TD_DI(1);
	} else if (ur->ur_flags & UR_FLAG_STATUS_IN) {
		tdcontrol = V_OHCI_TD_PID(K_OHCI_TD_IN) |
		    V_OHCI_TD_DT(K_OHCI_TD_DT_DATA1) |
		    V_OHCI_TD_CC(K_OHCI_CC_NOTACCESSED) | V_OHCI_TD_DI(1);
	} else {
		OHCIDEBUG(printk("Shouldn't happen!\n"));
	}

	if (ur->ur_flags & UR_FLAG_SHORTOK) {
		tdcontrol |= M_OHCI_TD_SHORTOK;
	}

	ptr = ur->ur_buffer;
	len = ur->ur_length;

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("\n>> %s addr %d ED %p EP_NUM %d ptr %p length %d ",__func__,
				ur->ur_dev->ud_address, ept->ep_phys,ept->ep_num,
				ur->ur_buffer, ur->ur_length);
	}
#endif


#ifdef CONFIG_DATA_CACHE_SUPPORT
	if(ur->ur_flags & UR_FLAG_OUT)
	{
		/*CPU has written HW will read */
		clean_dcache_by_addr((u32_t)ptr, (u32_t)len);
	}
	else
	{
		/* clean and invalidate the buffer from upper layers */
		/* Dont care */
		clean_n_invalidate_dcache_by_addr((u32_t)ptr, (u32_t)len);
	}
#endif
	ur->ur_tdcount = 0;

	/* Noncoherent DMA: need to flush user buffer to real memory first */

#ifdef MTD_DI_LAST
	if(multi_td)
		tdcontrol |= V_OHCI_TD_DI(7);
#endif

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk
		    (">> Queueing xfer addr %d pipe %d ED %p ptr %p length %d\n",
		     ur->ur_dev->ud_address, ur->ur_pipe->up_num, ept->ep_phys,
		     ptr, len);
	}
#endif

	curtd = OHCI_PTOV(BSWAP32(ed->ed_tailp));
	curtransfer = ohci_transfer_from_td(softc, curtd);

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/*each time a td and transfer is allocated flush */
	clean_dcache_by_addr((u32_t)curtd, sizeof(ohci_td_t));
	clean_dcache_by_addr((u32_t)curtransfer, sizeof(ohci_transfer_t));
#endif

	if (len == 0) {
		newtailtransfer = ohci_allocxfer(softc);
		if(newtailtransfer == NULL) {
			printk("WARNING no free xfer: %p\n",
				softc->ohci_transfer_freelist);
		}
		newtailtd = ohci_td_from_transfer(softc, newtailtransfer);
		curtd->td_cbp = 0;
		curtd->td_be = 0;
		curtd->td_next_td = BSWAP32(OHCI_VTOP(newtailtd));
		curtd->td_control = BSWAP32(tdcontrol);
		curtransfer->t_next = newtailtransfer;
		curtransfer->t_ref = ur;
		curtransfer->t_length = 0;
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 1) {
			printk("QueueTD: ");
			ohci_dumptd(softc, curtd);
		}
#endif
#ifdef CONFIG_DATA_CACHE_SUPPORT
			/*each time a td and transfer is allocated flush */
			clean_dcache_by_addr((u32_t)curtd, sizeof(ohci_td_t));
			clean_dcache_by_addr((u32_t)curtransfer, sizeof(ohci_transfer_t));
#endif
		

		ur->ur_tdcount++;
	} else {

		while (len > 0) {

			u32_t first_page_remainder=0;
			u32_t td_maxcopy = 0;

			/* Transfer which will span multiple TD */
			if(multi_td) {
					/*if its the first TD and buffer is a non 4K aligned*/
						amtcopy = len;
						first_page_remainder = (KB(4) - (OHCI_VTOP(ptr) % KB(4)));
						if (amtcopy > pktlen)
							amtcopy = pktlen;

				/* Non 4k page aligned address */
				if (first_page_remainder) {
					td_maxcopy = first_page_remainder + KB(4);

					if (amtcopy > td_maxcopy)
						amtcopy = td_maxcopy;
					}

				/* avoid adjustments on last TD */
				if((len > amtcopy) &&
				   (amtcopy % ept->ep_mps)) {
						u32_t pg_change;
						u32_t b_end;
						/* Adjust the amount to be handled in this TD to next higher
						 * multiple of ep->mps boundary as the last packet size
						 * would be equal to ep->ep_mps
						 */
						amtcopy = amtcopy + (ept->ep_mps - (amtcopy % ept->ep_mps));

						/* The above adjustment may cause the second page
						 * to go beyond the second 4K page boundary.
						 */
						pg_change = OHCI_VTOP(ptr + first_page_remainder);
						b_end = (OHCI_VTOP(ptr + amtcopy) - 1);
						/*check b_end - pg_change > 4K !!! */
						if((b_end - pg_change) > KB(4)) {
							/* reduce overall length by single ep->ep_mps */
							amtcopy = amtcopy - ( ept->ep_mps);
							}
					}
				} 
			else {

				/* For a transfer which can be handled within a 
				 * single TD original implementation is great
				 */
				amtcopy = len;
				if (amtcopy > pktlen)
					amtcopy = pktlen;
				/* max is remainder of this page (4k - (ptr & (4k - 1))) + 4k */
				first_page_remainder = OHCI_VTOP(ptr) & (KB(4) - 1);
				if (first_page_remainder) {
					td_maxcopy = first_page_remainder + KB(4);

					if (amtcopy > td_maxcopy)
						amtcopy = td_maxcopy;
				}
			}
#ifdef MTD_DI_LAST
			/* SET THE DI to 1 FOR LAST TD */
			if((multi_td) && ((len - amtcopy) == 0))
			{
				tdcontrol &= ~ V_OHCI_TD_DI(7);
				tdcontrol |= V_OHCI_TD_DI(1);
			}
#endif
			if(softc->ohci_transfer_freelist == NULL) {
				printk("WARNING no free xfer left: %p len:%d\n",
					softc->ohci_transfer_freelist, len);
			}

			newtailtransfer = ohci_allocxfer(softc);

			newtailtd =
			    ohci_td_from_transfer(softc, newtailtransfer);
			curtd->td_cbp = BSWAP32(OHCI_VTOP(ptr));
			curtd->td_be =
			    BSWAP32(OHCI_VTOP(ptr + amtcopy) - 1);
			curtd->td_next_td =
			    BSWAP32(OHCI_VTOP(newtailtd));
			curtd->td_control = BSWAP32(tdcontrol);

			curtransfer->t_next = newtailtransfer;
			curtransfer->t_ref = ur;
			curtransfer->t_length = amtcopy;

#ifdef _OHCI_DEBUG_
			if (ohcidebug > 1) {
				printk("QueueTD: ");
				ohci_dumptd(softc, curtd);
			}
#endif

			curtd = newtailtd;
			curtransfer = ohci_transfer_from_td(softc, curtd);

#ifdef CONFIG_DATA_CACHE_SUPPORT
			/*CPU has written HW will read and write*/
			/*each time a td and transfer is allocated flush */
			clean_dcache_by_addr((u32_t)curtd, sizeof(ohci_td_t));
			clean_dcache_by_addr((u32_t)curtransfer, sizeof(ohci_transfer_t));
#endif

			ptr += amtcopy;
			len -= amtcopy;
			ur->ur_tdcount++;

		}
	}

	curtd = OHCI_PTOV(BSWAP32(ed->ed_headp & M_OHCI_ED_PTRMASK));

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("HCCA before queue:\n");
		ohci_dump_hcca(softc);
	}
#endif

	ed->ed_tailp = BSWAP32(OHCI_VTOP(newtailtd));

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("HCCA after queue:\n");
		ohci_dump_hcca(softc);
	}
#endif

	/*
	 * Clear halted state
	 */
	ed->ed_headp &= BSWAP32(~M_OHCI_ED_HALT);

#ifdef CONFIG_DATA_CACHE_SUPPORT
	/* this is messy but essential */
	clean_n_invalidate_dcache_by_addr((u32_t)softc->ohci_hwedpool,
					 (u32_t)sizeof(ohci_ed_array));
	clean_n_invalidate_dcache_by_addr((u32_t)softc->ohci_endpoint_pool,
					 (u32_t)sizeof(ohci_endp));
	clean_n_invalidate_dcache_by_addr((u32_t)softc->ohci_hwtdpool,
					 (u32_t)sizeof(ohci_td_array));
	clean_n_invalidate_dcache_by_addr((u32_t)softc->ohci_transfer_pool,
					 (u32_t)sizeof(ohci_trnsf));
	clean_n_invalidate_dcache_by_addr((u32_t)softc->ohci_hcca,
					 (u32_t)sizeof(ohci_hcca_array));
#endif

#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("HCCA after halt-clear:\n");
		ohci_dump_hcca(softc);
	}
#endif

	/*
	 * Prod the controller depending on what type of list we put
	 * a TD on.
	 */

	if (ept->ep_flags & UP_TYPE_BULK) {
		ohci_write(softc->ohci_regs, R_OHCI_CMDSTATUS, M_OHCI_CMDSTATUS_BLF);
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 1) {
			printk("HCCA bulk prod\n");
		}
#endif
	} else {
		/* FIXME: should probably make sure we're UP_TYPE_CONTROL here */
		ohci_write(softc->ohci_regs, R_OHCI_CMDSTATUS, M_OHCI_CMDSTATUS_CLF);
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 1) {
			printk("HCCA ctl prod:\n");
		}
#endif
	}
#ifdef _OHCI_DEBUG_
	if (ohcidebug > 1) {
		printk("HCCA after prod:\n");
		ohci_dump_hcca(softc);
	}
#endif

	return 0;
}

/*
 *  Driver structure
 */
const struct usb_hcd ohci_driver = {
	ohci_create,
	ohci_delete,
	ohci_start,
	ohci_stop,
	ohci_intr,
	ohci_ept_create,
	ohci_ept_delete,
	ohci_ept_setmps,
	ohci_ept_setaddr,
	ohci_ept_cleartoggle,
	ohci_xfer,
	ohci_cancel_xfer
};

/*
 *  Root Hub
 *
 *  Data structures and functions
 */

/*
 * Data structures and routines to emulate the root hub.
 */
static const struct usb_device_descr ohci_root_devdsc = {
	sizeof(struct usb_device_descr),	/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,	/* bDescriptorType */
	USBWORD(0x0100),	/* bcdUSB */
	USB_DEVICE_CLASS_HUB,	/* bDeviceClass */
	0,			/* bDeviceSubClass */
	0,			/* bDeviceProtocol */
	64,			/* bMaxPacketSize0 */
	USBWORD(0),		/* idVendor */
	USBWORD(0),		/* idProduct */
	USBWORD(0x0100),	/* bcdDevice */
	1,			/* iManufacturer */
	2,			/* iProduct */
	0,			/* iSerialNumber */
	1			/* bNumConfigurations */
};

static const struct usb_config_descr ohci_root_cfgdsc = {
	sizeof(struct usb_config_descr),	/* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,	/* bDescriptorType */
	USBWORD(sizeof(struct usb_config_descr) +
		sizeof(struct usb_interface_descr) +
		sizeof(struct usb_endpoint_descr)),	/* wTotalLength */
	1,			/* bNumInterfaces */
	1,			/* bConfigurationValue */
	0,			/* iConfiguration */
	USB_CONFIG_SELF_POWERED,	/* bmAttributes */
	0			/* MaxPower */
};

static const struct usb_interface_descr ohci_root_ifdsc = {
	sizeof(struct usb_interface_descr),	/* bLength */
	USB_INTERFACE_DESCRIPTOR_TYPE,	/* bDescriptorType */
	0,			/* bInterfaceNumber */
	0,			/* bAlternateSetting */
	1,			/* bNumEndpoints */
	USB_INTERFACE_CLASS_HUB,	/* bInterfaceClass */
	0,			/* bInterfaceSubClass */
	0,			/* bInterfaceProtocol */
	0			/* iInterface */
};

static const struct usb_endpoint_descr ohci_root_epdsc = {
	sizeof(struct usb_endpoint_descr),	/* bLength */
	USB_ENDPOINT_DESCRIPTOR_TYPE,	/* bDescriptorType */
	(USB_ENDPOINT_DIRECTION_IN | 1),	/* bEndpointAddress */
	USB_ENDPOINT_TYPE_INTERRUPT,	/* bmAttributes */
	USBWORD(8),		/* wMaxPacketSize */
	255			/* bInterval */
};

static const struct usb_hub_descr ohci_root_hubdsc = {
	USB_HUB_DESCR_SIZE,	/* bLength */
	USB_HUB_DESCRIPTOR_TYPE,	/* bDescriptorType */
	0,			/* bNumberOfPorts */
	USBWORD(0),		/* wHubCharacteristics */
	0,			/* bPowreOnToPowerGood */
	0,			/* bHubControl Current */
	{0}			/* bRemoveAndPowerMask */
};

/*
 *  ohci_roothb_strdscr(ptr, str)
 *
 *  Construct a string descriptor for root hub requests
 *
 *  Input parameters:
 *	   ptr - pointer to where to put descriptor
 *	   str - regular string to put into descriptor
 *
 *  Return value:
 *	   number of bytes written to descriptor
 */

static int ohci_roothub_strdscr(u8_t *ptr, char *str)
{
	u8_t *p = ptr;

	*p++ = strlen(str) * 2 + 2;	/* Unicode strings */
	*p++ = USB_STRING_DESCRIPTOR_TYPE;
	while (*str) {
		*p++ = *str++;
		*p++ = 0;
	}
	return (p - ptr);
}

/*
 *  ohci_roothub_req(softc, req)
 *
 *  Handle a descriptor request on the control pipe for the
 *  root hub.  We pretend to be a real root hub here and
 *  return all the standard descriptors.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *	   req - a usb request (completed immediately)
 *
 *  Return value:
 *	   0 if ok
 *	   else error code
 */

static int ohci_roothub_req(ohci_softc_t *softc, struct usb_device_request *req)
{
	u8_t *ptr;
	u16_t wValue;
	u16_t wIndex;
	struct usb_port_status ups;
	struct usb_hub_descr hdsc;
	u32_t status;
	u32_t statport;
	u32_t tmpval;
	int res = 0;

	ptr = softc->ohci_rh_buf;

	wValue = GETUSBFIELD(req, wValue);
	wIndex = GETUSBFIELD(req, wIndex);

	switch (REQSW(req->bRequest, req->bmRequestType)) {

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    (USB_GETSTATUS_SELF_POWERED & 0xFF);
		*ptr++ = (USB_GETSTATUS_SELF_POWERED >> 8);
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		*ptr++ =
		    0;
		*ptr++ = 0;
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		status =
		    ohci_read(softc->ohci_regs, (R_OHCI_RHPORTSTATUS(wIndex)));
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 0) {
			printk("RHGetStatus: ");
			ohci_dumpportstat(wIndex, status);
		}
#endif
		PUTUSBFIELD((&ups), wPortStatus, (status & 0xFFFF));
		PUTUSBFIELD((&ups), wPortChange, (status >> 16));
		memcpy(ptr, &ups, sizeof(ups));
		ptr += sizeof(ups);
		break;

	case REQCODE(USB_REQUEST_GET_STATUS, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		break;

	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		/* do nothing, not supported */
		break;

	case REQCODE(USB_REQUEST_CLEAR_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		statport =
		    R_OHCI_RHPORTSTATUS(wIndex);
#ifdef _OHCI_DEBUG_
		if (ohcidebug > 0) {
			printk("RHClearFeature(%d): ", wValue);
			ohci_dumpportstat(wIndex,
					  ohci_read(softc->ohci_regs, statport));
		}
#endif
		switch (wValue) {
		case USB_PORT_FEATURE_CONNECTION:
			break;
		case USB_PORT_FEATURE_ENABLE:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_CCS);
			break;
		case USB_PORT_FEATURE_SUSPEND:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_POCI);
			break;
		case USB_PORT_FEATURE_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_RESET:
			break;
		case USB_PORT_FEATURE_POWER:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_LSDA);
			break;
		case USB_PORT_FEATURE_LOW_SPEED:
			break;
		case USB_PORT_FEATURE_C_PORT_CONNECTION:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_CSC);
			break;
		case USB_PORT_FEATURE_C_PORT_ENABLE:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PESC);
			break;
		case USB_PORT_FEATURE_C_PORT_SUSPEND:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PSSC);
			break;
		case USB_PORT_FEATURE_C_PORT_OVER_CURRENT:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_OCIC);
			break;
		case USB_PORT_FEATURE_C_PORT_RESET:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PRSC);
			break;

		}

		/*
		 * If we've cleared all of the conditions that
		 * want our attention on the port status,
		 * then we can accept port status interrupts again.
		 */

		if ((wValue >= USB_PORT_FEATURE_C_PORT_CONNECTION) &&
		    (wValue <= USB_PORT_FEATURE_C_PORT_RESET)) {
			status = ohci_read(softc->ohci_regs, statport);
			if ((status & M_OHCI_RHPORTSTAT_ALLC) == 0) {
				softc->ohci_intdisable &= ~M_OHCI_INT_RHSC;
#ifdef USBHOST_INTERRUPT_MODE
				ohci_write(softc->ohci_regs, R_OHCI_INTENABLE,
					      M_OHCI_INT_RHSC);
#endif
			}
		}
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		res =
		    -1;
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		/* nothing */
		break;

	case REQCODE(USB_REQUEST_SET_FEATURE, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_OTHER):
		statport =
		    R_OHCI_RHPORTSTATUS(wIndex);
		switch (wValue) {
		case USB_PORT_FEATURE_CONNECTION:
			break;
		case USB_PORT_FEATURE_ENABLE:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PES);
			break;
		case USB_PORT_FEATURE_SUSPEND:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PSS);
			break;
		case USB_PORT_FEATURE_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_RESET:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PRS);
			for (;;) {	/* FIXME: timer */
				usb_delay_ms(100);
				if (!
				    (ohci_read(softc->ohci_regs, statport) &
				     M_OHCI_RHPORTSTAT_PRS))
					break;
			}
			break;
		case USB_PORT_FEATURE_POWER:
			ohci_write(softc->ohci_regs, statport, M_OHCI_RHPORTSTAT_PPS);
			break;
		case USB_PORT_FEATURE_LOW_SPEED:
			break;
		case USB_PORT_FEATURE_C_PORT_CONNECTION:
			break;
		case USB_PORT_FEATURE_C_PORT_ENABLE:
			break;
		case USB_PORT_FEATURE_C_PORT_SUSPEND:
			break;
		case USB_PORT_FEATURE_C_PORT_OVER_CURRENT:
			break;
		case USB_PORT_FEATURE_C_PORT_RESET:
			break;

		}

		break;

	case REQCODE(USB_REQUEST_SET_ADDRESS, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		softc->ohci_rh_newaddr =
		    wValue;
		break;

	case REQCODE(USB_REQUEST_GET_DESCRIPTOR, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		switch (wValue >> 8) {
		case USB_DEVICE_DESCRIPTOR_TYPE:
			memcpy(ptr, &ohci_root_devdsc,
			       sizeof(ohci_root_devdsc));
			ptr += sizeof(ohci_root_devdsc);
			break;
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			memcpy(ptr, &ohci_root_cfgdsc,
			       sizeof(ohci_root_cfgdsc));
			ptr += sizeof(ohci_root_cfgdsc);
			memcpy(ptr, &ohci_root_ifdsc, sizeof(ohci_root_ifdsc));
			ptr += sizeof(ohci_root_ifdsc);
			memcpy(ptr, &ohci_root_epdsc, sizeof(ohci_root_epdsc));
			ptr += sizeof(ohci_root_epdsc);
			break;
		case USB_STRING_DESCRIPTOR_TYPE:
			switch (wValue & 0xFF) {
			case 1:
				ptr += ohci_roothub_strdscr(ptr, "Generic");
				break;
			case 2:
				ptr += ohci_roothub_strdscr(ptr, "Root Hub");
				break;
			default:
				*ptr++ = 0;
				break;
			}
			break;
		default:
			res = -1;
			break;
		}
		break;

	case REQCODE(USB_REQUEST_GET_DESCRIPTOR, USBREQ_DIR_IN, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		memcpy(&hdsc, &ohci_root_hubdsc,
		       sizeof(hdsc));
		hdsc.bNumberOfPorts = softc->ohci_ndp;
		status = ohci_read(softc->ohci_regs, R_OHCI_RHDSCRA);
		tmpval = 0;
		if (status & M_OHCI_RHDSCRA_NPS)
			tmpval |= USB_HUBCHAR_PWR_NONE;
		if (status & M_OHCI_RHDSCRA_PSM)
			tmpval |= USB_HUBCHAR_PWR_GANGED;
		else
			tmpval |= USB_HUBCHAR_PWR_IND;
		PUTUSBFIELD((&hdsc), wHubCharacteristics, tmpval);
		tmpval = G_OHCI_RHDSCRA_POTPGT(status);
		hdsc.bPowerOnToPowerGood = tmpval;
		hdsc.bDescriptorLength = USB_HUB_DESCR_SIZE + 1;
		status = ohci_read(softc->ohci_regs, R_OHCI_RHDSCRB);
		hdsc.bRemoveAndPowerMask[0] = (u8_t) status;
		memcpy(ptr, &hdsc, sizeof(hdsc));
		ptr += sizeof(hdsc);
		break;

	case REQCODE(USB_REQUEST_SET_DESCRIPTOR, USBREQ_DIR_OUT, USBREQ_TYPE_CLASS,
		     USBREQ_REC_DEVICE):
		/* nothing */
		break;

	case REQCODE(USB_REQUEST_GET_CONFIGURATION, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		*ptr++ =
		    softc->ohci_rh_conf;
		break;

	case REQCODE(USB_REQUEST_SET_CONFIGURATION, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_DEVICE):
		softc->ohci_rh_conf =
		    wValue;
		break;

	case REQCODE(USB_REQUEST_GET_INTERFACE, USBREQ_DIR_IN, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		*ptr++ =
		    0;
		break;

	case REQCODE(USB_REQUEST_SET_INTERFACE, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_INTERFACE):
		/* nothing */
		break;

	case REQCODE(USB_REQUEST_SYNC_FRAME, USBREQ_DIR_OUT, USBREQ_TYPE_STD,
		     USBREQ_REC_ENDPOINT):
		/* nothing */
		break;
	}

	softc->ohci_rh_ptr = softc->ohci_rh_buf;
	softc->ohci_rh_len = ptr - softc->ohci_rh_buf;

	return res;
}

/*
 *  ohci_roothub_statchg(softc)
 *
 *  This routine is called from the interrupt service routine
 *  (well, polling routine) for the ohci controller.  If the
 *  controller notices a root hub status change, it dequeues an
 *  interrupt transfer from the root hub's queue and completes
 *  it here.
 *
 *  Input parameters:
 *	   softc - our OHCI controller
 *
 *  Return value:
 *	   nothing
 */

static void ohci_roothub_statchg(ohci_softc_t *softc)
{
	struct usbreq *ur;
	u32_t status;
	u8_t portstat = 0;
	int idx;

	/* Note: this only works up to 8 ports */
	for (idx = 1; idx <= softc->ohci_ndp; idx++) {
		status = ohci_read(softc->ohci_regs, R_OHCI_RHPORTSTATUS(idx));
		if (status & M_OHCI_RHPORTSTAT_ALLC)
			portstat = (1 << idx);
	}

	/* Complete the root hub's interrupt usbreq if there's a change */
	if (portstat != 0) {
#ifdef USBHOST_INTERRUPT_MODE
		ohci_write(softc->ohci_regs, R_OHCI_INTDISABLE, M_OHCI_INT_RHSC);
#endif
		softc->ohci_intdisable |= M_OHCI_INT_RHSC;

		ur = (struct usbreq *) q_deqnext(&(softc->ohci_rh_intrq));
		if (!ur)
			return;	/* no requests pending, ignore it */

		memset(ur->ur_buffer, 0, ur->ur_length);
		ur->ur_buffer[0] = portstat;
		ur->ur_xferred = ur->ur_length;

		usb_complete_request(ur, 0);
	}
}

/*
 *  ohci_roothub_xfer(softc, req)
 *
 *  Handle a root hub xfer - ohci_xfer transfers control here
 *  if we detect the address of the root hub - no actual transfers
 *  go out on the wire, we just handle the requests directly to
 *  make it look like a hub is attached.
 *
 *  This seems to be common practice in the USB world, so we do
 *  it here too.
 *
 *  Input parameters:
 *	   softc - our OHCI controller structure
 *	   req - usb request destined for host controller
 *
 *  Return value:
 *	   0 if ok
 *	   else error
 */

static int ohci_roothub_xfer(struct usbbus *bus, void *uept, struct usbreq *ur)
{
	ohci_softc_t *softc = (ohci_softc_t *) bus->ub_hwsoftc;
	ohci_endpoint_t *ept = (ohci_endpoint_t *) uept;
	int res;

	switch (ept->ep_num) {
		/*
		 * CONTROL ENDPOINT
		 */
	case 0:
		/*
		 * Three types of transfers:  OUT (SETUP), IN (data), or STATUS.
		 * figure out which is which.
		 */

		if (ur->ur_flags & UR_FLAG_SETUP) {
			/*
			 * SETUP packet - this is an OUT request to the control
			 * pipe.  We emulate the hub request here.
			 */
			struct usb_device_request *req;

			req = (struct usb_device_request *) ur->ur_buffer;

			res = ohci_roothub_req(softc, req);
			if (res)
				printk("Root hub request returned an error\n");

			ur->ur_xferred = ur->ur_length;
			ur->ur_status = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_STATUS_IN) {
			/*
			 * STATUS IN : it's sort of like a dummy IN request
			 * to acknowledge a SETUP packet that otherwise has no
			 * status.      Just complete the usbreq.
			 */

			if (softc->ohci_rh_newaddr != -1) {
				softc->ohci_rh_addr = softc->ohci_rh_newaddr;
				softc->ohci_rh_newaddr = -1;
			}
			ur->ur_status = 0;
			ur->ur_xferred = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_STATUS_OUT) {
			/*
			 * STATUS OUT : it's sort of like a dummy OUT request
			 */
			ur->ur_status = 0;
			ur->ur_xferred = 0;
			usb_complete_request(ur, 0);
		} else if (ur->ur_flags & UR_FLAG_IN) {
			/*
			 * IN : return data from the root hub
			 */
			int amtcopy;

			amtcopy = softc->ohci_rh_len;
			if (amtcopy > ur->ur_length)
				amtcopy = ur->ur_length;

			memcpy(ur->ur_buffer, softc->ohci_rh_ptr, amtcopy);

			softc->ohci_rh_ptr += amtcopy;
			softc->ohci_rh_len -= amtcopy;

			ur->ur_status = 0;
			ur->ur_xferred = amtcopy;
			usb_complete_request(ur, 0);
		} else {
			OHCIDEBUG(printk("Unknown root hub transfer type\n"));
			return -1;
		}
		break;
		/*
		 * INTERRUPT ENDPOINT
		 */
	case 1:		/* interrupt pipe */
		if (ur->ur_flags & UR_FLAG_IN) {
			printk("%s: enque\n", __func__);
			q_enqueue(&(softc->ohci_rh_intrq), (queue_t *) ur);
		}
		break;
	}
	return 0;
}

void ohci_reg_dump(volatile ohci_reg_t *ohci_reg)
{
	printk("==================OHCI REG DUMP====================\n");
	printk("revision		 (%p)=%08x\n", &ohci_reg->revision,
	       ohci_reg->revision);
	printk("control		 (%p)=%08x\n", &ohci_reg->control,
	       ohci_reg->control);
	printk("cmdstatus		 (%p)=%08x\n", &ohci_reg->cmdstatus,
	       ohci_reg->cmdstatus);
	printk("intstatus		 (%p)=%08x\n", &ohci_reg->intstatus,
	       ohci_reg->intstatus);
	printk("intenable		 (%p)=%08x\n", &ohci_reg->intenable,
	       ohci_reg->intenable);
	printk("intdisable		 (%p)=%08x\n", &ohci_reg->intdisable,
	       ohci_reg->intdisable);
	printk("hcca			 (%p)=%08x\n", &ohci_reg->hcca,
	       ohci_reg->hcca);
	printk("periodcurrented (%p)=%08x\n", &ohci_reg->periodcurrented,
	       ohci_reg->periodcurrented);
	printk("controlheaded	 (%p)=%08x\n", &ohci_reg->controlheaded,
	       ohci_reg->controlheaded);
	printk("controlcurrented(%p)=%08x\n", &ohci_reg->controlcurrented,
	       ohci_reg->controlcurrented);
	printk("bulkheaded		 (%p)=%08x\n", &ohci_reg->bulkheaded,
	       ohci_reg->bulkheaded);
	printk("bulkcurrented	 (%p)=%08x\n", &ohci_reg->bulkcurrented,
	       ohci_reg->bulkcurrented);
	printk("donehead		 (%p)=%08x\n", &ohci_reg->donehead,
	       ohci_reg->donehead);
	printk("fminterval		 (%p)=%08x\n", &ohci_reg->fminterval,
	       ohci_reg->fminterval);
	printk("fmremaining	 (%p)=%08x\n", &ohci_reg->fmremaining,
	       ohci_reg->fmremaining);
	printk("fmnumber		 (%p)=%08x\n", &ohci_reg->fmnumber,
	       ohci_reg->fmnumber);
	printk("periodicstart	 (%p)=%08x\n", &ohci_reg->periodicstart,
	       ohci_reg->periodicstart);
	printk("lsthreshold	 (%p)=%08x\n", &ohci_reg->lsthreshold,
	       ohci_reg->lsthreshold);
	printk("rhdscra		 (%p)=%08x\n", &ohci_reg->rhdscra,
	       ohci_reg->rhdscra);
	printk("rhdscrb		 (%p)=%08x\n", &ohci_reg->rhdscrb,
	       ohci_reg->rhdscrb);
	printk("rhstatus		 (%p)=%08x\n", &ohci_reg->rhstatus,
	       ohci_reg->rhstatus);
	printk("rhport00		 (%p)=%08x\n", &ohci_reg->rhport00,
	       ohci_reg->rhport00);
	printk("rhport01		 (%p)=%08x\n", &ohci_reg->rhport01,
	       ohci_reg->rhport01);
	printk("rhport02		 (%p)=%08x\n", &ohci_reg->rhport02,
	       ohci_reg->rhport02);
	printk("phyctrlp0		 (%p)=%08x\n", &ohci_reg->phyctrlp0,
	       ohci_reg->phyctrlp0);
	printk("framelenadj	 (%p)=%08x\n", &ohci_reg->framelenadj,
	       ohci_reg->framelenadj);
	printk("shimctrl		 (%p)=%08x\n", &ohci_reg->shimctrl,
	       ohci_reg->shimctrl);
	printk("shimslavestatus (%p)=%08x\n", &ohci_reg->shimslavestatus,
	       ohci_reg->shimslavestatus);
	printk("shimmasterstatus(%p)=%08x\n", &ohci_reg->shimmasterstatus,
	       ohci_reg->shimmasterstatus);
	printk("ehcistatus		 (%p)=%08x\n", &ohci_reg->ehcistatus,
	       ohci_reg->ehcistatus);
	printk("utmip0ctl		 (%p)=%08x\n", &ohci_reg->utmip0ctl,
	       ohci_reg->utmip0ctl);
	printk("tpctl			 (%p)=%08x\n", &ohci_reg->tpctl,
	       ohci_reg->tpctl);
	printk("tpoutp0		 (%p)=%08x\n", &ohci_reg->tpoutp0,
	       ohci_reg->tpoutp0);
	printk("ohcipwrsts		 (%p)=%08x\n", &ohci_reg->ohcipwrsts,
	       ohci_reg->ohcipwrsts);
	printk("ehcixfer		 (%p)=%08x\n", &ohci_reg->ehcixfer,
	       ohci_reg->ehcixfer);
	printk("ehcilpsmcstate  (%p)=%08x\n", &ohci_reg->ehcilpsmcstate,
	       ohci_reg->ehcilpsmcstate);
	printk("ohciehcistrap	 (%p)=%08x\n", &ohci_reg->ohciehcistrap,
	       ohci_reg->ohciehcistrap);
	printk("ohciccssts		 (%p)=%08x\n", &ohci_reg->ohciccssts,
	       ohci_reg->ohciccssts);
	printk("intrcontrol	 (%p)=%08x\n", &ohci_reg->intrcontrol,
	       ohci_reg->intrcontrol);
	printk("ehciincrxstrap  (%p)=%08x\n", &ohci_reg->ehciincrxstrap,
	       ohci_reg->ehciincrxstrap);
	printk("===================================================\n");
}
