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
 * 
 * Notes:
 * 
 * Code ported and adapted from Xen/Mini-OS 
 */

/*
 *
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * Copyright (c) 2005, Keir A Fraser
 *
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

#ifndef _MM_H_
#define _MM_H_

#include <os/lib.h>
#include <public/arch-x86_64.h>
#include <public/xen.h>

#define __CONST(x) x ## UL
#define CONST(x) __CONST(x)

#define L1_FRAME                1
#define L2_FRAME                2
#define L3_FRAME                3

#define L1_PAGETABLE_SHIFT      12

#define L2_PAGETABLE_SHIFT      21
#define L3_PAGETABLE_SHIFT      30
#define L4_PAGETABLE_SHIFT      39

#define L1_PAGETABLE_ENTRIES    512
#define L2_PAGETABLE_ENTRIES    512
#define L3_PAGETABLE_ENTRIES    512
#define L4_PAGETABLE_ENTRIES    512

#define PADDR_BITS              52
#define VADDR_BITS              48
#define PADDR_MASK              ((1UL << PADDR_BITS)-1)
#define VADDR_MASK              ((1UL << VADDR_BITS)-1)

#define L2_MASK  ((1UL << L3_PAGETABLE_SHIFT) - 1)
#define L3_MASK  ((1UL << L4_PAGETABLE_SHIFT) - 1)

#define NOT_L1_FRAMES           3
#define PRIpte "016lx"
typedef unsigned long pgentry_t;

#define VIRT_DEMAND_AREA        CONST(0x0000100000000000)
#define DEMAND_MAP_PAGES        CONST(0x8000000)

#define L1_MASK  ((1UL << L2_PAGETABLE_SHIFT) - 1)

#define l1_table_offset(_a) (((_a) >> L1_PAGETABLE_SHIFT) & (L1_PAGETABLE_ENTRIES - 1))
#define l2_table_offset(_a) (((_a) >> L2_PAGETABLE_SHIFT) & (L2_PAGETABLE_ENTRIES - 1))
#define l3_table_offset(_a) (((_a) >> L3_PAGETABLE_SHIFT) & (L3_PAGETABLE_ENTRIES - 1))
#define l4_table_offset(_a) (((_a) >> L4_PAGETABLE_SHIFT) & (L4_PAGETABLE_ENTRIES - 1))

#define _PAGE_PRESENT  0x001UL
#define _PAGE_RW       0x002UL
#define _PAGE_USER     0x004UL
#define _PAGE_PWT      0x008UL
#define _PAGE_PCD      0x010UL
#define _PAGE_ACCESSED 0x020UL
#define _PAGE_DIRTY    0x040UL
#define _PAGE_PAT      0x080UL
#define _PAGE_PSE      0x080UL
#define _PAGE_GLOBAL   0x100UL

#define L1_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_USER)
#define L1_PROT_RO (_PAGE_PRESENT|_PAGE_ACCESSED|_PAGE_USER)
#define L2_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)
#define L3_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)
#define L4_PROT (_PAGE_PRESENT|_PAGE_RW|_PAGE_ACCESSED|_PAGE_DIRTY|_PAGE_USER)

#define PAGE_SIZE       (1UL << L1_PAGETABLE_SHIFT)

#define PAGE_SHIFT      L1_PAGETABLE_SHIFT
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> L1_PAGETABLE_SHIFT)
#define PFN_DOWN(x)	((x) >> L1_PAGETABLE_SHIFT)
#define PFN_PHYS(x)	((x) << L1_PAGETABLE_SHIFT)
#define PHYS_PFN(x)	((x) >> L1_PAGETABLE_SHIFT)

#define PAGE_ALIGN(addr)        (((addr)+PAGE_SIZE-1)&PAGE_MASK)

typedef unsigned long paddr_t;
typedef unsigned long maddr_t;

extern unsigned long *phys_to_machine_mapping;
extern char _text, _etext, _edata, _end;
#define pfn_to_mfn(_pfn) (phys_to_machine_mapping[(_pfn)])

static __inline__ maddr_t phys_to_machine(paddr_t phys)
{
	maddr_t machine = pfn_to_mfn(phys >> PAGE_SHIFT);
	machine = (machine << PAGE_SHIFT) | (phys & ~PAGE_MASK);
	return machine;
}

#define mfn_to_pfn(_mfn) (machine_to_phys_mapping[(_mfn)])
static __inline__ paddr_t machine_to_phys(maddr_t machine)
{
	paddr_t phys = mfn_to_pfn(machine >> PAGE_SHIFT);
	phys = (phys << PAGE_SHIFT) | (machine & ~PAGE_MASK);
	return phys;
}

#define VIRT_START                 ((unsigned long)&_text)

#define to_phys(x)                 ((unsigned long)(x)-VIRT_START)
#define to_virt(x)                 ((void *)((unsigned long)(x)+VIRT_START))
#define to_virtu(x)                ((unsigned long)(x)+VIRT_START)
#define virt_to_pfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)         (pfn_to_mfn(virt_to_pfn(_virt)))
#define mach_to_virt(_mach)        (to_virt(machine_to_phys(_mach)))
#define virt_to_mach(_virt)        (phys_to_machine(to_phys(_virt)))
#define mfn_to_virt(_mfn)          (to_virt(mfn_to_pfn(_mfn) << PAGE_SHIFT))
#define pfn_to_virt(_pfn)          (to_virt((_pfn) << PAGE_SHIFT))
#define pfn_to_virtu(_pfn)         (to_virtu((_pfn) << PAGE_SHIFT))
#define pte_to_mfn(_pte)           (((_pte) & (PADDR_MASK&PAGE_MASK)) >> L1_PAGETABLE_SHIFT)
#define pte_to_virt(_pte)          to_virt(mfn_to_pfn(pte_to_mfn(_pte)) << PAGE_SHIFT)
#define __pte(x) ((pte_t) { (x) } )

#define round_pgdown(_p)  ((_p)&PAGE_MASK)
#define round_pgup(_p)    (((_p)+(PAGE_SIZE-1))&PAGE_MASK)

extern unsigned long mfn_zero;

#define do_map_zero(start, n) do_map_frames(start, &mfn_zero, n, 0, 0, DOMID_SELF, NULL, L1_PROT_RO)

#define HEAP_VM 0
#define STACK_VM 1
#define CODE_VM 2
#define DATA_VM 3
#define PAGE_FRAME_VM 4

void init_mm(char *cmd_line);
void arch_init_mm(unsigned long* start_pfn_p, unsigned long* max_pfn_p);
unsigned long allocate_ondemand(unsigned long n, unsigned long alignment);
unsigned long allocate_pages(int n, int type);
void deallocate_pages(void *pointer, int n, int type);
unsigned long alloc_pages(int order);
#define alloc_page() alloc_pages(0)
void free_pages(void *pointer, int order);
#define free_page(_pointer) free_pages(_pointer, 0)

static __inline__ int get_order(unsigned long size) 
{
	int order;
    size = (size-1) >> PAGE_SHIFT;
    for ( order = 0; size; order++ )
        size >>= 1;
    return order;
}

struct pfn_alloc_env;
typedef struct pfn_alloc_env pfn_alloc_env_t;

struct pfn_alloc_env {
    void *data;
    long pfn;
    long (*pfn_alloc)(pfn_alloc_env_t *env, unsigned long addr);
};

long pfn_linear_alloc(pfn_alloc_env_t *env, unsigned long addr);
long pfn_alloc_alloc(pfn_alloc_env_t *env, unsigned long addr);
int build_pagetable(unsigned long start_address, unsigned long end_address, pfn_alloc_env_t *env_pfn, pfn_alloc_env_t *env_npf);
void demolish_pagetable(unsigned long start_address, unsigned long end_address);
unsigned long maximum_ram_page(void);
extern int unmap_page(unsigned long addr);
extern int remap_page(unsigned long addr);
extern int unmap_page_pfn(unsigned long addr, unsigned long pfn);
extern int remap_page_pfn(unsigned long addr, unsigned long pfn);
extern long not11_virt_to_pfn(unsigned long addr, unsigned long *pte);
extern int clear_pte(unsigned long addr);
extern int validate(unsigned long addr);
void do_map_frames(unsigned long va, const unsigned long *mfns, unsigned long n, unsigned long stride, unsigned long incr, domid_t id, int *err, unsigned long prot);
void arch_update_p2m(unsigned long start_pfn, unsigned long end_pfn, int adding);
#endif 
