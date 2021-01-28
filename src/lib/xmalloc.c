/* Copyright (C) 2017, Ward Jaradat
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

/****************************************************************************
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: xmalloc.c
 *      Author: Grzegorz Milos
 *     Changes: Mick Jordan
 *
 *        Date: Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: simple memory allocator
 *
 ****************************************************************************
 * Simple allocator for Mini-os.  If larger than a page, simply use the
 * page-order allocator.
 *
 * Copy of the allocator for Xen by Rusty Russell:
 * Copyright (C) 2005 Rusty Russell IBM Corporation
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <os/config.h>
#include <os/kernel.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/list.h>

static LIST_HEAD(freelist);
static spinlock_t freelist_lock = SPIN_LOCK_UNLOCKED;

struct xmalloc_hdr
{
	size_t size;
	struct list_head freelist;
};

extern void *
xmalloc_at(const void *p, size_t size)
{
	struct xmalloc_hdr *hdr;
	int p_size;
	BUG_ON(in_irq());
	if (p == NULL) return xmalloc_align(size, sizeof(unsigned long));
	hdr = (struct xmalloc_hdr *)p - 1;
	p_size = hdr->size - sizeof(struct xmalloc_hdr);
	if (p_size >= size) return (void *)p;
	void *result = xmalloc_align(size, sizeof(unsigned long));
	memcpy(result, p, p_size);
	xfree(p);
	return result;
}

extern void 
xfree_at(void *start, size_t length)
{
    free_pages(start, get_order(length));
}

static void maybe_split(struct xmalloc_hdr *hdr, size_t size, size_t block)
{
	struct xmalloc_hdr *extra;
	size_t leftover = block - size;

	if ( leftover >= (2 * sizeof(struct xmalloc_hdr)) )
	{
		extra = (struct xmalloc_hdr *)((unsigned long)hdr + size);

		extra->size = leftover;

		list_add(&extra->freelist, &freelist);
	}
	else
	{
		size = block;
	}

	hdr->size = size;
	hdr->freelist.next = hdr->freelist.prev = NULL;
}

static void *xmalloc_new_page(size_t size)
{
	struct xmalloc_hdr *hdr;
	unsigned long flags;
	hdr = (struct xmalloc_hdr *)alloc_page();

	if ( hdr == NULL )
	{
		return NULL;
	}

	spin_lock_irqsave(&freelist_lock, flags);
	maybe_split(hdr, size, PAGE_SIZE);
	spin_unlock_irqrestore(&freelist_lock, flags);
	return hdr+1;
}

static inline size_t align_up(size_t size, size_t align) 
{
	return (size + align - 1) & ~(align - 1);
}

static void *xmalloc_whole_pages(size_t size) 
{
	struct xmalloc_hdr *hdr;
	size_t asize = align_up(size, PAGE_SIZE);
	hdr = (struct xmalloc_hdr *)allocate_pages(asize / PAGE_SIZE, DATA_VM);
	if ( hdr == NULL ) return NULL;
	hdr->size = asize;
	hdr->freelist.next = hdr->freelist.prev = NULL;
	return hdr+1;
}

void *xmalloc_align(size_t asize, size_t align) 
{
	struct xmalloc_hdr *i;
	unsigned long flags;
	size_t size = asize;
	void *result;

	BUG_ON(in_irq());
	size += sizeof(struct xmalloc_hdr);
	size = align_up(size, __alignof__(struct xmalloc_hdr));

	if ( size >= PAGE_SIZE ) 
	{
		result = xmalloc_whole_pages(size);
		goto done;
	}

	spin_lock_irqsave(&freelist_lock, flags);
	list_for_each_entry(i, &freelist, freelist)
	{
		if ( i->size < size ) continue;
		list_del(&i->freelist);
		maybe_split(i, size, i->size);
		spin_unlock_irqrestore(&freelist_lock, flags);
		i = i + 1;
		result = (void *) i;
		goto done;
	}
	spin_unlock_irqrestore(&freelist_lock, flags);
	result = xmalloc_new_page(size);
	done:
	return result;
}

void xfree(const void *p) 
{
	unsigned long flags;
	struct xmalloc_hdr *i, *tmp, *hdr;
	BUG_ON(in_irq());

	if (p == NULL) return;
	hdr = (struct xmalloc_hdr *)p - 1;

	if(((long)p & PAGE_MASK) != ((long)hdr & PAGE_MASK))
	{
		printk("Header should be on the same page, p=%lx, hdr=%lx\n", p, hdr);
		*(int*)0=0;
	}

	if(hdr->freelist.next || hdr->freelist.prev)
	{
		printk("Should not be previously freed\n");
		return;
		*(int*)0=0;
	}

	if ( hdr->size >= PAGE_SIZE )
	{
		deallocate_pages(hdr, hdr->size / PAGE_SIZE, DATA_VM);
		goto done;
	}

	spin_lock_irqsave(&freelist_lock, flags);
	list_for_each_entry_safe( i, tmp, &freelist, freelist )
	{
		unsigned long _i   = (unsigned long)i;
		unsigned long _hdr = (unsigned long)hdr;

		if ( ((_i ^ _hdr) & PAGE_MASK) != 0 ) continue;

		if ( (_i + i->size) == _hdr )
		{
			list_del(&i->freelist);
			i->size += hdr->size;
			hdr = i;
		}

		if ( (_hdr + hdr->size) == _i )
		{
			list_del(&i->freelist);
			hdr->size += i->size;
		}
	}

	if ( hdr->size == PAGE_SIZE )
	{
		if((((unsigned long)hdr) & (PAGE_SIZE-1)) != 0)
		{
			printk("Bug\n");
			*(int*)0=0;
		}
		free_pages(hdr, 0);
	}
	else
	{
		list_add(&hdr->freelist, &freelist);
	}

	spin_unlock_irqrestore(&freelist_lock, flags);
	done:
	return;
}

void *xrealloc(const void *p, size_t size, size_t align)
{
	struct xmalloc_hdr *hdr;
	int psize;
	BUG_ON(in_irq());
	if (p == NULL)
	{
		return xmalloc_align(size, align);
	}
	hdr = (struct xmalloc_hdr *)p - 1;
	psize = hdr->size - sizeof(struct xmalloc_hdr);
	if (psize >= size) return (void *)p;
	void *result = xmalloc_align(size, align);
	memcpy(result, p, psize);
	xfree(p);
	return result;
}

void *xcalloc(size_t n, size_t size)
{
	register size_t total= size*n;
	void * x;
	x = xmalloc_align(total, sizeof(unsigned long));
	if (x) 
	{
		memset(x,0,total);
	}
	return x;
}
