/* Copyright (C) 2019, Ward Jaradat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 ****************************************************************************
 * (C) 2006 - Cambridge University
 ****************************************************************************
 *
 *        File: gnttab.c
 *      Author: Steven Smith
 *     Changes: Grzegorz Milos
 *     Changes: Harald Roeck
 *     Changes: Mick Jordan
 *
 *        Date: July 2006
 *
 * Environment: Guest VM microkernel evolved from Xen Minimal OS
 * Description: Simple grant tables implementation. About as stupid as it's
 * possible to be and still work.
 *
 ****************************************************************************
 */
#include <os/kernel.h>
#include <os/mm.h>
#include <os/gnttab.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/spinlock.h>

#define NR_RESERVED_ENTRIES 8
#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_v1_t))

static DEFINE_SPINLOCK(gnttab_lock);

static grant_entry_v1_t *gnttab_table;

static grant_ref_t gnttab_list[NR_GRANT_ENTRIES];

static void put_free_entry(grant_ref_t ref)
{
	unsigned long flags;
	spin_lock_irqsave(&gnttab_lock, flags);
	gnttab_list[ref] = gnttab_list[0];
	gnttab_list[0] = ref;
	spin_unlock_irqrestore(&gnttab_lock, flags);
}

static grant_ref_t get_free_entry(void)
{
	unsigned int ref;
	unsigned long flags;
	spin_lock_irqsave(&gnttab_lock, flags);
	ref = gnttab_list[0];
	BUG_ON(ref < NR_RESERVED_ENTRIES || ref >= NR_GRANT_ENTRIES);
	gnttab_list[0] = gnttab_list[ref];
	spin_unlock_irqrestore(&gnttab_lock, flags);
	return ref;
}

grant_ref_t gnttab_grant_access(domid_t domid, unsigned long frame, int readonly)
{
	grant_ref_t ref;
	ref = get_free_entry();
	gnttab_table[ref].frame = frame;
	gnttab_table[ref].domid = domid;
	wmb();
	readonly *= GTF_readonly;
	gnttab_table[ref].flags = GTF_permit_access | readonly;
	return ref;
}

grant_ref_t gnttab_grant_transfer(domid_t domid, unsigned long pfn)
{
	grant_ref_t ref;
	ref = get_free_entry();
	gnttab_table[ref].frame = pfn;
	gnttab_table[ref].domid = domid;
	wmb();
	gnttab_table[ref].flags = GTF_accept_transfer;
	return ref;
}

int gnttab_end_access(grant_ref_t ref)
{
	uint16_t flags, nflags;
	BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);
	nflags = gnttab_table[ref].flags;
	do 
	{
		if ((flags = nflags) & (GTF_reading | GTF_writing))
		{
			return 0;
		}
	} while ((nflags = synch_cmpxchg(&gnttab_table[ref].flags, flags, 0)) != flags);
	put_free_entry(ref);
	return 1;
}

unsigned long gnttab_end_transfer(grant_ref_t ref)
{
	unsigned long frame;
	uint16_t flags;
	BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);

	while (!((flags = gnttab_table[ref].flags) & GTF_transfer_committed))
	{
		if (synch_cmpxchg(&gnttab_table[ref].flags, flags, 0) == flags)
		{
			put_free_entry(ref);
			return 0;
		}
	}

	while (!(flags & GTF_transfer_completed))
	{
		flags = gnttab_table[ref].flags;
	}

	rmb();
	frame = gnttab_table[ref].frame;
	put_free_entry(ref);
	return frame;
}

grant_ref_t gnttab_alloc_and_grant(void **map)
{
	unsigned long mfn;
	grant_ref_t gref;
	*map = (void *) alloc_page();
	mfn = virt_to_mfn(*map);
	gref = gnttab_grant_access(0, mfn, 0);
	return gref;
}

static const char *gnttabop_error_msgs[] = GNTTABOP_error_msgs;

const char * gnttabop_error(int16_t status)
{
	status = -status;
	if (status < 0 || status >= ARRAY_SIZE(gnttabop_error_msgs)) 
	{
		return "bad status";
	}
	else
	{
		return gnttabop_error_msgs[status];
	}
}

static void init_frames(unsigned long *frames, int num_frames)
{
	struct gnttab_setup_table setup;
	int i;
	
	for (i = NR_RESERVED_ENTRIES; i < NR_GRANT_ENTRIES; i++)
	{
		put_free_entry(i);
	}

	setup.dom = DOMID_SELF;
	setup.nr_frames = num_frames;
	set_xen_guest_handle(setup.frame_list, frames);
	HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
}

long pfn_gntframe_alloc(pfn_alloc_env_t *env, unsigned long addr)
{
	unsigned long *frames = (unsigned long *) env->data;
	return frames[env->pfn++];;
}

void init_gnttab(void)
{
	unsigned long frames[NR_GRANT_FRAMES];
	init_frames(frames, NR_GRANT_FRAMES);
	gnttab_table = pfn_to_virt(maximum_ram_page());
	struct pfn_alloc_env pfn_pageframe_env = { .pfn_alloc = pfn_alloc_alloc };
	struct pfn_alloc_env pfn_gntframe_env = { .pfn_alloc = pfn_gntframe_alloc };
	pfn_gntframe_env.pfn = 0;
	pfn_gntframe_env.data = (void*) frames;
	build_pagetable((unsigned long) gnttab_table, (unsigned long) (gnttab_table) + NR_GRANT_FRAMES * PAGE_SIZE, &pfn_gntframe_env, &pfn_pageframe_env);
}
