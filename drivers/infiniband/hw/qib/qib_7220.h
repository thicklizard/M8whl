#ifndef _QIB_7220_H
#define _QIB_7220_H
/*
 * Copyright (c) 2007, 2009, 2010 QLogic Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "qib_7220_regs.h"

#define IBA7220_KRCVEGRCNT      2048U

#define IB_7220_LT_STATE_CFGRCVFCFG      0x09
#define IB_7220_LT_STATE_CFGWAITRMT      0x0a
#define IB_7220_LT_STATE_TXREVLANES      0x0d
#define IB_7220_LT_STATE_CFGENH          0x10

struct qib_chip_specific {
	u64 __iomem *cregbase;
	u64 *cntrs;
	u64 *portcntrs;
	spinlock_t sdepb_lock; 
	spinlock_t rcvmod_lock; 
	spinlock_t gpio_lock; 
	u64 hwerrmask;
	u64 errormask;
	u64 gpio_out; 
	u64 gpio_mask; 
	u64 extctrl; 
	u32 ncntrs;
	u32 nportcntrs;
	u32 cntrnamelen;
	u32 portcntrnamelen;
	u32 numctxts;
	u32 rcvegrcnt;
	u32 autoneg_tries;
	u32 serdes_first_init_done;
	u32 sdmabufcnt;
	u32 lastbuf_for_pio;
	u32 updthresh; 
	u32 updthresh_dflt; 
	int irq;
	u8 presets_needed;
	u8 relock_timer_active;
	char emsgbuf[128];
	char sdmamsgbuf[192];
	char bitsmsgbuf[64];
	struct timer_list relock_timer;
	unsigned int relock_interval; 
};

struct qib_chippport_specific {
	struct qib_pportdata pportdata;
	wait_queue_head_t autoneg_wait;
	struct delayed_work autoneg_work;
	struct timer_list chase_timer;
	u64 ibdeltainprog;
	u64 ibsymdelta;
	u64 ibsymsnap;
	u64 iblnkerrdelta;
	u64 iblnkerrsnap;
	u64 ibcctrl; 
	u64 ibcddrctrl; 
	unsigned long chase_end;
	u32 last_delay_mult;
};

int qib_sd7220_presets(struct qib_devdata *dd);
int qib_sd7220_init(struct qib_devdata *dd);
void qib_sd7220_clr_ibpar(struct qib_devdata *);
#define IB_7220_SERDES 2

static inline u32 qib_read_kreg32(const struct qib_devdata *dd,
				  const u16 regno)
{
	if (!dd->kregbase || !(dd->flags & QIB_PRESENT))
		return -1;
	return readl((u32 __iomem *)&dd->kregbase[regno]);
}

static inline u64 qib_read_kreg64(const struct qib_devdata *dd,
				  const u16 regno)
{
	if (!dd->kregbase || !(dd->flags & QIB_PRESENT))
		return -1;

	return readq(&dd->kregbase[regno]);
}

static inline void qib_write_kreg(const struct qib_devdata *dd,
				  const u16 regno, u64 value)
{
	if (dd->kregbase)
		writeq(value, &dd->kregbase[regno]);
}

void set_7220_relock_poll(struct qib_devdata *, int);
void shutdown_7220_relock_poll(struct qib_devdata *);
void toggle_7220_rclkrls(struct qib_devdata *);


#endif 
