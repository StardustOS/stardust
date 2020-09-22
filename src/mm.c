/* Copyright (C) 2018, Ward Jaradat
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
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: mm.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos
 *            : Mick Jordan
 *
 *        Date: Aug 2003, changes Aug 2005, 2008, 2009, 2010
 *
 * Environment: Xen Minimal OS
 * Description: page allocator
 *
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <os/kernel.h>
#include <os/hypervisor.h>
#include <os/mm.h>
#include <os/xmalloc.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <public/memory.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/bitmap.h>
#include <os/crash.h>

static DEFINE_SPINLOCK(bitmap_lock);

static unsigned long *alloc_bitmap;

#define PAGES_PER_MAPWORD ENTRIES_PER_MAPWORD

#define BITS_PER_PAGE (4096 * 8)

#define DEFAULT_SMALL_PERCENTAGE 2

static unsigned long first_alloc_page;  

static unsigned long end_alloc_page;    

static unsigned long max_end_alloc_page;

static unsigned long first_free_page; 

static unsigned long first_bulk_page;

static unsigned long first_free_bulk_page;

static unsigned long num_free_pages;

static unsigned long num_free_bulk_pages;

unsigned long *phys_to_machine_mapping;

static LIST_HEAD(memory_hole_list);

typedef struct memory_hole 
{
	struct list_head memory_hole_next;
	unsigned long start_pfn;
	unsigned long end_pfn;
} memory_hole_t;

#define memory_hole_head (list_entry(memory_hole_list.next, memory_hole_t, memory_hole_next))

static int in_increase_memory = 0;

#define BULK_ALLOCATION 512

#define is_bulk(_n) (_n >= BULK_ALLOCATION)

static long increase_reservation(unsigned long n, memory_hole_t *hole);

static unsigned long can_increase(unsigned long n);

#define MAX_L2_PAGES 32

#define ENTRIES_PER_L2_PAGE (PAGE_SIZE / sizeof(unsigned long))

#define PFNS_PER_L3_PAGE (256 * 1024)

static unsigned long *pfn_to_mfn_table;

static unsigned long *l2_list_pages[MAX_L2_PAGES];

static int num_l2_pages;

static unsigned long max_pfn_table;

static unsigned long maxmem_pfn_table;

static void map_alloc(unsigned long first_page, unsigned long nr_pages)
{
	unsigned long start_off, end_off, curr_idx, end_idx;
	curr_idx  = first_page / PAGES_PER_MAPWORD;
	start_off = first_page & (PAGES_PER_MAPWORD-1);
	end_idx   = (first_page + nr_pages) / PAGES_PER_MAPWORD;
	end_off   = (first_page + nr_pages) & (PAGES_PER_MAPWORD-1);

	if ( curr_idx == end_idx )
	{
		alloc_bitmap[curr_idx] |= ((1UL<<end_off)-1) & -(1UL<<start_off);
	}
	else
	{
		alloc_bitmap[curr_idx] |= -(1UL<<start_off);
		while ( ++curr_idx < end_idx ) alloc_bitmap[curr_idx] = ~0L;
		alloc_bitmap[curr_idx] |= (1UL<<end_off)-1;
	}
}

static void map_free(unsigned long first_page, unsigned long nr_pages)
{
	unsigned long start_off, end_off, curr_idx, end_idx;

	curr_idx = first_page / PAGES_PER_MAPWORD;
	start_off = first_page & (PAGES_PER_MAPWORD-1);
	end_idx   = (first_page + nr_pages) / PAGES_PER_MAPWORD;
	end_off   = (first_page + nr_pages) & (PAGES_PER_MAPWORD-1);

	if ( curr_idx == end_idx )
	{
		alloc_bitmap[curr_idx] &= -(1UL<<end_off) | ((1UL<<start_off)-1);
	}
	else
	{
		alloc_bitmap[curr_idx] &= (1UL<<start_off)-1;
		while ( ++curr_idx != end_idx ) alloc_bitmap[curr_idx] = 0;
		alloc_bitmap[curr_idx] &= -(1UL<<end_off);
	}
}

static int next_free(int page) 
{
	while (page < end_alloc_page) 
	{
		if (!allocated_in_map(alloc_bitmap, page)) 
		{
			break;
		}
		page++;
	}
	return page;
}

static long increase_memory_holding_lock(unsigned long n) 
{
	long result = 0;
	if (in_increase_memory) 
	{
		in_increase_memory = 0;
		return 0;
	} 
	else 
	{
		in_increase_memory = 1;
	}
	if (can_increase(n)) 
	{
		while (n > 0) 
		{
			memory_hole_t *memory_hole = memory_hole_head;
			unsigned long hole_size = memory_hole->end_pfn - memory_hole->start_pfn;
			unsigned long nn = hole_size <= n ? hole_size : n;
			spin_unlock(&bitmap_lock); /* there may be recursive entry for page table frames */
			long rc = increase_reservation(nn, memory_hole);
			spin_lock(&bitmap_lock);
			if (rc > 0) 
			{
				map_free(memory_hole->start_pfn, rc);
				if (memory_hole->start_pfn + rc > end_alloc_page) 
				{
					end_alloc_page = memory_hole->start_pfn + rc;
				}
				num_free_pages += rc;
				num_free_bulk_pages += rc;

				if (hole_size == rc) 
				{
					list_del(&memory_hole->memory_hole_next);
				} 
				else 
				{
					memory_hole->start_pfn += rc;
				}
			} 
			else 
			{
				break; /* failed */
			}
			n -= rc;
			result += rc;
		}
	}
	in_increase_memory = 0;
	return result;
}

void unlocked_free(void *p) 
{
	spin_unlock(&bitmap_lock);
	xfree(p);
	spin_lock(&bitmap_lock);
}

void memory_hole_add(memory_hole_t *new_memory_hole) 
{
	struct list_head *list;
	memory_hole_t *memory_hole;
	list_for_each(list, &memory_hole_list) 
	{
		memory_hole = list_entry(list, memory_hole_t, memory_hole_next);
		if (memory_hole->start_pfn > new_memory_hole->start_pfn) break;
	}
	list_add_tail(&new_memory_hole->memory_hole_next, list);
}

static unsigned long _allocate_pages(int n, int type)
{
	unsigned long page;
	unsigned long result = 0;
	int is_bulk_alloc = is_bulk(n);
	int initial_is_bulk_alloc = is_bulk_alloc;

	BUG_ON(in_irq());
	spin_lock(&bitmap_lock);

	while (result == 0) 
	{
		unsigned long end_page = is_bulk_alloc ? end_alloc_page : first_bulk_page;
		page = is_bulk_alloc ? first_free_bulk_page : first_free_page;
		while (page < end_page) 
		{
			if (!allocated_in_map(alloc_bitmap, page)) 
			{
				int nn = n;
				unsigned long npage = page + 1;
				while (nn > 1 && (npage < end_page)) 
				{
					if (!allocated_in_map(alloc_bitmap, npage)) 
					{
						nn--; npage++;
					} 
					else 
					{
						page = npage;
						break;
					}
				}
				if (nn == 1) 
				{
					result = (unsigned long) to_virt(PFN_PHYS(page));
					if (is_bulk_alloc) 
					{
						num_free_bulk_pages -= n;
						if (page == first_free_bulk_page) 
						{
							first_free_bulk_page = next_free(page + n);
						}
					} 
					else 
					{
						if (page == first_free_page) 
						{
							first_free_page = next_free(page + n);
						}
					}
					num_free_pages -= n;
					map_alloc(page, n);
					break;
				}
			}
			page++;
		}

		if (result > 0) break;
		else 
		{
			if (!is_bulk_alloc) 
			{
				is_bulk_alloc = 1;
			} 
			else 
			{
				if (!increase_memory_holding_lock(n)) break;
			}
		}
	}

	spin_unlock(&bitmap_lock);

	if (result == 0 && !initial_is_bulk_alloc) 
	{
		crash();
	}
	return result;
}

unsigned long alloc_pages(int order) 
{
	return _allocate_pages(1 << order, DATA_VM);
}

unsigned long allocate_pages(int n, int type) 
{
	return _allocate_pages(n, type);
}

void deallocate_pages(void *pointer, int n, int type) 
{
	BUG_ON(in_irq());
	spin_lock(&bitmap_lock);
	unsigned long page = virt_to_pfn(pointer);
	if (is_bulk(n)) 
	{
		if (page < first_free_bulk_page) first_free_bulk_page = page;
		num_free_bulk_pages += n;
	} 
	else 
	{
		if (page < first_free_page) first_free_page = page;
	}
	map_free(page, n);
	spin_unlock(&bitmap_lock);
	num_free_pages += n;
}

void free_pages(void *pointer, int order)
{
	deallocate_pages(pointer, 1 << order, DATA_VM);
}

unsigned long xen_maximum_reservation(void) 
{
	domid_t domid = DOMID_SELF;
	return HYPERVISOR_memory_op(XENMEM_maximum_reservation, &domid);
}

unsigned long maximum_ram_page(void) 
{
	unsigned long result;
	return HYPERVISOR_memory_op(XENMEM_maximum_ram_page, &result);
	return result;
}

void resize_phys_to_machine_mapping_table(void) 
{
	if (max_end_alloc_page > end_alloc_page) 
	{
		unsigned long new_phys_to_machine_mapping_size = round_pgup(max_end_alloc_page * sizeof(long)) / PAGE_SIZE;
		unsigned long *new_phys_to_machine_mapping = (unsigned long *) allocate_pages(new_phys_to_machine_mapping_size, DATA_VM);
		if (new_phys_to_machine_mapping == 0) 
		{
			crash_with_message("can't resize phys_to_machine_mapping\n");
		}
		memcpy(new_phys_to_machine_mapping, phys_to_machine_mapping, end_alloc_page * sizeof(long));
		phys_to_machine_mapping = new_phys_to_machine_mapping;
	}

	memory_hole_t *memory_hole = xmalloc(memory_hole_t);
	memory_hole->start_pfn = end_alloc_page;
	memory_hole->end_pfn = max_end_alloc_page;
	list_add(&memory_hole->memory_hole_next, &memory_hole_list);
}

static long increase_reservation(unsigned long nr_pages, memory_hole_t *memory_hole) 
{
	struct xen_memory_reservation reservation = {
			.address_bits = 0,
			.extent_order = 0,
			.domid        = DOMID_SELF
	};
	int pfn;

	unsigned long start_pfn = memory_hole->start_pfn;
	BUG_ON(memory_hole->end_pfn - memory_hole->start_pfn < nr_pages);
	
	for (pfn = 0; pfn < nr_pages; pfn++) 
	{
		phys_to_machine_mapping[start_pfn + pfn] = start_pfn + pfn;
	}
	
	set_xen_guest_handle(reservation.extent_start, &phys_to_machine_mapping[start_pfn]);
	reservation.nr_extents = nr_pages;
	long rc = HYPERVISOR_memory_op(XENMEM_populate_physmap, &reservation);

	if (rc > 0) 
	{
		struct pfn_alloc_env pfn_frame_alloc_env = {
				.pfn_alloc = pfn_alloc_alloc
		};

		struct pfn_alloc_env pfn_linear_tomap_env = {
				.pfn_alloc = pfn_linear_alloc
		};

		pfn_linear_tomap_env.pfn = start_pfn;
		if (build_pagetable(pfn_to_virtu(start_pfn), pfn_to_virtu(start_pfn + rc), &pfn_linear_tomap_env, &pfn_frame_alloc_env)) 
		{
			arch_update_p2m(start_pfn, start_pfn + rc, 1);
		} 
		else 
		{
			rc = 0;
		}
	}
	return rc;
}

static unsigned long can_increase(unsigned long n) 
{
	struct list_head *list;
	memory_hole_t *memory_hole;
	list_for_each(list, &memory_hole_list) 
	{
		memory_hole = list_entry(list, memory_hole_t, memory_hole_next);
		unsigned long hole = memory_hole->end_pfn - memory_hole->start_pfn;
		if (hole >= n) return n;
		n -= hole;
	}
	return 0;
}

static void init_page_allocator(char *cmd_line, unsigned long min, unsigned long max)
{
	unsigned long bitmap_pages;
	int small_pct;

	max_end_alloc_page = xen_maximum_reservation();
	bitmap_pages = max_end_alloc_page / BITS_PER_PAGE;
	alloc_bitmap = (unsigned long *)pfn_to_virt(min);
	min += bitmap_pages;
	first_alloc_page = min;
	end_alloc_page = max;
	num_free_pages = max - min;
	first_free_page = first_alloc_page;
	small_pct = DEFAULT_SMALL_PERCENTAGE;
	first_bulk_page = first_free_page + (max_end_alloc_page * small_pct) / 100;
	if (first_bulk_page > end_alloc_page) first_bulk_page = end_alloc_page;
	first_free_bulk_page = first_bulk_page;
	num_free_bulk_pages = max - first_bulk_page;
	memset(alloc_bitmap, ~0, bitmap_pages * 4096);
	map_free(first_alloc_page, num_free_pages);
}

static unsigned long demand_map_area_start;
static unsigned long demand_map_area_end;

void arch_init_demand_mapping_area(void)
{
    demand_map_area_start = VIRT_DEMAND_AREA;
    demand_map_area_end = demand_map_area_start + DEMAND_MAP_PAGES * PAGE_SIZE;
    printk("Demand map pfns at %lx-%lx.\n", demand_map_area_start, demand_map_area_end);
}

static pgentry_t *get_pgt(unsigned long va)
{
    unsigned long mfn;
    pgentry_t *tab;
    unsigned offset;

    tab = start_info.pt_base;
    mfn = virt_to_mfn(start_info.pt_base);

    offset = l4_table_offset(va);
    if (!(tab[offset] & _PAGE_PRESENT)) 
	{
        return NULL;
	}
	mfn = pte_to_mfn(tab[offset]);
    tab = mfn_to_virt(mfn);
    offset = l3_table_offset(va);
    if (!(tab[offset] & _PAGE_PRESENT)) 
	{
        return NULL;
	}
    mfn = pte_to_mfn(tab[offset]);
    tab = mfn_to_virt(mfn);
    offset = l2_table_offset(va);
    if (!(tab[offset] & _PAGE_PRESENT)) 
	{
        return NULL;
	}
	if (tab[offset] & _PAGE_PSE) 
	{
        return &tab[offset];
	}
	mfn = pte_to_mfn(tab[offset]);
    tab = mfn_to_virt(mfn);
    offset = l1_table_offset(va);
    return &tab[offset];
}

unsigned long allocate_ondemand(unsigned long n, unsigned long alignment)
{
    unsigned long x;
    unsigned long y = 0;

    for (x = 0; x <= DEMAND_MAP_PAGES - n; x = (x + y + 1 + alignment - 1) & ~(alignment - 1))
    {
        unsigned long addr = demand_map_area_start + x * PAGE_SIZE;
        pgentry_t *pgt = get_pgt(addr);
        for (y = 0; y < n; y++, addr += PAGE_SIZE) 
        {
            if (!(addr & L1_MASK))
			{
                pgt = get_pgt(addr);
			}

            if (pgt)
            {
                if (*pgt & _PAGE_PRESENT)
				{
                    break;
				}
                pgt++;
            }
        }
        if (y == n)
		{
            break;
		}
    }

    if (y != n)
    {
        printk("Failed to find %ld frames!\n", n);
        return 0;
    }
    return demand_map_area_start + x * PAGE_SIZE;
}

void arch_update_p2m(unsigned long start_pfn, unsigned long end_pfn, int adding)
{
    unsigned long *l2_list;
    unsigned long l1_offset, l2_offset, l3_offset;
    int i = 0;
    if (adding) 
	{
    	if (start_pfn == 0) 
	  	{
			for(l1_offset=start_pfn, l2_offset=start_pfn / ENTRIES_PER_L2_PAGE, l3_offset=0; l1_offset < end_pfn; l1_offset += ENTRIES_PER_L2_PAGE, l2_offset++) 
			{
	  			if(l2_offset % ENTRIES_PER_L2_PAGE == 0) 
				{
					BUG_ON(num_l2_pages + i >= MAX_L2_PAGES);
					l2_list = (unsigned long *)alloc_page();
					l2_list_pages[num_l2_pages + i] = l2_list;
					pfn_to_mfn_table[num_l2_pages + l3_offset] = virt_to_mfn(l2_list);
					l3_offset++;
					i++;
	  			}
				l2_list[l2_offset % ENTRIES_PER_L2_PAGE] = virt_to_mfn(&phys_to_machine_mapping[l1_offset]);
			}
			num_l2_pages += i;
      	}
      
		if (end_pfn > max_pfn_table) 
	  	{
			max_pfn_table = end_pfn;
      	}
    } 
	else 
	{
    	if (end_pfn == max_pfn_table) 
		{
			max_pfn_table = start_pfn;
      	}
    }
    HYPERVISOR_shared_info->arch.max_pfn = max_pfn_table;
}

void arch_init_p2m(unsigned long max_pfn, unsigned long maxmem_pfn) 
{
	num_l2_pages = 0;
  	pfn_to_mfn_table = (unsigned long *)alloc_page();
  	HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list_list = virt_to_mfn(pfn_to_mfn_table);
  	max_pfn_table = 0;
  	maxmem_pfn_table = maxmem_pfn;
  	arch_update_p2m(0, maxmem_pfn, 1);
  	max_pfn_table = max_pfn;  
  	HYPERVISOR_shared_info->arch.max_pfn = max_pfn_table;
}

void init_mm(char *cmd_line)
{
	unsigned long pt_pfn, max_pfn;
	arch_init_mm(&pt_pfn, &max_pfn);
	barrier();
	init_page_allocator(cmd_line, pt_pfn, max_pfn);
	resize_phys_to_machine_mapping_table();
	arch_init_p2m(max_pfn, max_end_alloc_page);
	arch_init_demand_mapping_area();
}
