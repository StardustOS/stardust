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

/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: mm.c
 *      Author: Rolf Neugebauer
 *     Changes: Grzegorz Milos
 *     Changes: Harald Roeck
 *     Changes: Mick Jordan
 *
 *        Date: Aug 2003, changes Aug 2005, 2008, 2009, 2010
 *
 * Environment: Xen Minimal OS
 * Description: Arch specific memory management related functions (pagetables)
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
#include <os/crash.h>
#include <public/memory.h>

#include <os/types.h>
#include <os/lib.h>

long pfn_linear_alloc(pfn_alloc_env_t *env, unsigned long addr) 
{
	return pfn_to_mfn(env->pfn++);
}

long pfn_alloc_alloc(pfn_alloc_env_t *env, unsigned long addr) 
{
	unsigned long va = allocate_pages(1, PAGE_FRAME_VM);
	if (va == 0) return -1;
	return pfn_to_mfn(virt_to_pfn(va));
}

static void new_pt_frame(unsigned long pt_mfn_for_pfn, unsigned long prev_l_mfn, unsigned long offset, unsigned long level)
{
	pgentry_t *tab = (pgentry_t *)start_info.pt_base;
	unsigned long pt_pfn = mfn_to_pfn(pt_mfn_for_pfn);
	unsigned long pt_page = (unsigned long)pfn_to_virt(pt_pfn);
	unsigned long prot_e, prot_t, pincmd;
	mmu_update_t mmu_updates[1];
	struct mmuext_op pin_request;
	prot_e = prot_t = pincmd = 0;
	memset((unsigned long*)pfn_to_virt(pt_pfn), 0, PAGE_SIZE);
	switch ( level )
	{
	case L1_FRAME:
		prot_e = L1_PROT;
		prot_t = L2_PROT;
		pincmd = MMUEXT_PIN_L1_TABLE;
		break;
#if defined(__x86_64__) || defined(CONFIG_X86_PAE)
	case L2_FRAME:
		prot_e = L2_PROT;
		prot_t = L3_PROT;
		pincmd = MMUEXT_PIN_L2_TABLE;
		break;
#endif
#if defined(__x86_64__)
	case L3_FRAME:
		prot_e = L3_PROT;
		prot_t = L4_PROT;
		pincmd = MMUEXT_PIN_L3_TABLE;
		break;
#endif
	default:
		crash();
		break;
	}
#if defined(__x86_64__)
	tab = pte_to_virt(tab[l4_table_offset(pt_page)]);
	tab = pte_to_virt(tab[l3_table_offset(pt_page)]);
#endif
	mmu_updates[0].ptr = ((pgentry_t)tab[l2_table_offset(pt_page)] & PAGE_MASK) + sizeof(pgentry_t) * l1_table_offset(pt_page);
	mmu_updates[0].val = (pgentry_t)pfn_to_mfn(pt_pfn) << PAGE_SHIFT | (prot_e & ~_PAGE_RW);
	if(HYPERVISOR_mmu_update(mmu_updates, 1, NULL, DOMID_SELF) < 0)
	{
		crash();
	}
	pin_request.cmd = pincmd;
	pin_request.arg1.mfn = pfn_to_mfn(pt_pfn);
	if(HYPERVISOR_mmuext_op(&pin_request, 1, NULL, DOMID_SELF) < 0)
	{
		crash();
	}
	mmu_updates[0].ptr = ((pgentry_t)prev_l_mfn << PAGE_SHIFT) + sizeof(pgentry_t) * offset;
	mmu_updates[0].val = (pgentry_t)pfn_to_mfn(pt_pfn) << PAGE_SHIFT | prot_t;
	if(HYPERVISOR_mmu_update(mmu_updates, 1, NULL, DOMID_SELF) < 0)
	{
		crash();
	}
}

static int build_pagetable_vs(unsigned long start_address, unsigned long end_address, int page_size, pfn_alloc_env_t *env_pfn, pfn_alloc_env_t *env_npf)
{
	mmu_update_t mmu_updates[L1_PAGETABLE_ENTRIES + 1];
	pgentry_t *tab = (pgentry_t *)start_info.pt_base, page;
	unsigned long mfn;
	unsigned long offset;
	int count = 0;
	while(start_address < end_address)
	{
		tab = (pgentry_t *)start_info.pt_base;
		mfn = pfn_to_mfn(virt_to_pfn(start_info.pt_base));
#if defined(__x86_64__)
		offset = l4_table_offset(start_address);
		if(!(tab[offset] & _PAGE_PRESENT)) {
			/* Need new L3 pt frame */
			long npf_pfn = env_npf->pfn_alloc(env_npf, start_address);
			if (npf_pfn < 0) {
				return 0;
			} else {
				new_pt_frame(npf_pfn, mfn, offset, L3_FRAME);
			}
		}

		page = tab[offset];
		mfn = pte_to_mfn(page);
		tab = to_virt(mfn_to_pfn(mfn) << PAGE_SHIFT);
#endif
#if defined(__x86_64__) || defined(CONFIG_X86_PAE)
		offset = l3_table_offset(start_address);
		if(!(tab[offset] & _PAGE_PRESENT)) {
			/* Need new L2 pt frame */
			long npf_pfn = env_npf->pfn_alloc(env_npf, start_address);
			if (npf_pfn < 0) {
				return 0;
			} else {
				new_pt_frame(npf_pfn, mfn, offset, L2_FRAME);
			}
		}

		page = tab[offset];
		mfn = pte_to_mfn(page);
		tab = to_virt(mfn_to_pfn(mfn) << PAGE_SHIFT);
#endif
		offset = l2_table_offset(start_address);

		if (page_size == PAGE_SIZE) {
			if(!(tab[offset] & _PAGE_PRESENT)) {
				/* Need new L1 pt frame */
				long npf_pfn = env_npf->pfn_alloc(env_npf, start_address);
				if (npf_pfn < 0) {
					return 0;
				} else {
					new_pt_frame(npf_pfn, mfn, offset, L1_FRAME);
				}
			}
			page = tab[offset];
			mfn = pte_to_mfn(page);
			offset = l1_table_offset(start_address);
		}

		long mfn_for_pfn = env_pfn->pfn_alloc(env_pfn, start_address);
		if (mfn_for_pfn < 0) {
			return 0;
		}

		if (mfn_for_pfn > 0) {
			mmu_updates[count].ptr = ((pgentry_t)mfn << PAGE_SHIFT) + sizeof(pgentry_t) * offset;
			if (page_size == PAGE_SIZE) {
				mmu_updates[count].val = (pgentry_t)mfn_for_pfn << L1_PAGETABLE_SHIFT | L1_PROT;
			} else {
				mmu_updates[count].val = (pgentry_t)mfn_for_pfn << L2_PAGETABLE_SHIFT | L1_PROT | _PAGE_PSE;
			}
			count++;
		}
		start_address += page_size;
		if (count == L1_PAGETABLE_ENTRIES || start_address == end_address)
		{
			if(HYPERVISOR_mmu_update(mmu_updates, count, NULL, DOMID_SELF) < 0)
			{
				crash();
			}
			count = 0;
		}
	}
	return 1;
}

int build_pagetable(unsigned long start_address, unsigned long end_address, pfn_alloc_env_t *env_pfn, pfn_alloc_env_t *env_npf)
{
	return build_pagetable_vs(start_address, end_address, PAGE_SIZE, env_pfn, env_npf);
}

static void demolish_pagetable_vs(unsigned long start_address, unsigned long end_address, int page_size)
{
	int i = 0;
	multicall_entry_t call[1024];

	while (start_address < end_address)
	{
		int arg = 0;
		call[i].op = __HYPERVISOR_update_va_mapping;
		call[i].args[arg++] = start_address;
		call[i].args[arg++] = 0;
		call[i].args[arg++] = UVMF_ALL | UVMF_INVLPG;
		start_address += page_size;

		if (i == 1023 || start_address == end_address) {
			int ret = HYPERVISOR_multicall(call, i);
			if (ret) {
				crash();
			}
			i = 0;
		}
		i++;
	}
}

void demolish_pagetable(unsigned long start_address, unsigned long end_address)
{
	demolish_pagetable_vs(start_address, end_address, PAGE_SIZE);
}

void arch_init_mm(unsigned long* free_pfn_ptr, unsigned long* max_pfn_ptr)
{
	unsigned long start_pfn;
	printk("memory layout\t:\n");
	printk("    _text       : %p\n", &_text);
	printk("    _etext      : %p\n", &_etext);
	printk("    _edata      : %p\n", &_edata);
	printk("    stack start : %p %lx (%ld)\n", &stack, virt_to_pfn(&stack), virt_to_pfn(&stack));
	printk("    _end        : %p\n", &_end);
	printk("    pt_base     : %p %lx (%ld)\n", start_info.pt_base, virt_to_pfn(start_info.pt_base), virt_to_pfn(start_info.pt_base));
	printk("    nr_pt_frames: %ld\n", start_info.nr_pt_frames);
	printk("    nr_pages    : %ld\n", start_info.nr_pages);
	printk("    mfn_list    : %p\n", start_info.mfn_list);
	printk("    store_addr  : %p %lx (%ld)\n", mfn_to_virt(start_info.store_mfn), mfn_to_pfn(start_info.store_mfn), mfn_to_pfn(start_info.store_mfn));
	printk("    pfn_to_mfn  : %p %lx (%ld)\n", phys_to_machine_mapping, virt_to_pfn(phys_to_machine_mapping), virt_to_pfn(phys_to_machine_mapping));
	printk("    mfn_to_pfn  : %p\n", machine_to_phys_mapping);
	printk("    hyp_start   : %p\n", HYPERVISOR_VIRT_START);
	printk("    hyp_end     : %p\n", HYPERVISOR_VIRT_END);
	*free_pfn_ptr = PFN_UP(to_phys(start_info.pt_base)) + start_info.nr_pt_frames + 3;
	*max_pfn_ptr = start_info.nr_pages;
	printk("    free_pfn    : %lx (%ld)\n", *free_pfn_ptr, *free_pfn_ptr);
	printk("    max_pfn     : %lx (%ld)\n", *max_pfn_ptr, *max_pfn_ptr);
	start_pfn = (start_info.nr_pt_frames - NOT_L1_FRAMES) * L1_PAGETABLE_ENTRIES;

	struct pfn_alloc_env pfn_frame_alloc_env = {
			.pfn_alloc = pfn_linear_alloc
	};

	struct pfn_alloc_env pfn_linear_tomap_env = {
			.pfn_alloc = pfn_linear_alloc
	};

	pfn_frame_alloc_env.pfn = *free_pfn_ptr;
	pfn_linear_tomap_env.pfn = start_pfn;
	build_pagetable(pfn_to_virtu(start_pfn), pfn_to_virtu(*max_pfn_ptr), &pfn_linear_tomap_env, &pfn_frame_alloc_env);
	*free_pfn_ptr = pfn_frame_alloc_env.pfn;
}

int unmap_page_pfn(unsigned long addr, unsigned long pfn)
{
	pte_t val = __pte(pfn_to_mfn(pfn) << PAGE_SHIFT);
	return HYPERVISOR_update_va_mapping(PAGE_ALIGN(addr), val, (unsigned long)UVMF_ALL | UVMF_INVLPG);
}

int unmap_page(unsigned long addr)
{
	return unmap_page_pfn(addr, virt_to_pfn(addr));
}

int clear_pte(unsigned long addr) 
{
	return HYPERVISOR_update_va_mapping(PAGE_ALIGN(addr), __pte(0), (unsigned long)UVMF_ALL | UVMF_INVLPG);
}

int remap_page_pfn(unsigned long addr, unsigned long pfn) 
{
	pte_t val = __pte( (pfn_to_mfn(pfn) << PAGE_SHIFT) | L1_PROT );
	return HYPERVISOR_update_va_mapping(PAGE_ALIGN(addr), val, (unsigned long)UVMF_ALL | UVMF_INVLPG);
}

int remap_page(unsigned long addr)
{
	return remap_page_pfn(addr, virt_to_pfn(addr));
}

long not11_virt_to_pfn(unsigned long addr, unsigned long *pte_ptr) 
{
	pgentry_t *tab = (pgentry_t *)start_info.pt_base;
	long pfn;
	int level = 4;
	int offset;
	unsigned long pte;
	while (level > 0) {
		switch (level) {
		case 4: offset = l4_table_offset(addr); break;
		case 3: offset = l3_table_offset(addr); break;
		case 2: offset = l2_table_offset(addr); break;
		case 1: offset = l1_table_offset(addr); break;
		}
		pte = tab[offset];
		if (level > 1 && !(pte & _PAGE_PRESENT)) {
			return -1;
		}
		pfn = mfn_to_pfn(pte_to_mfn(pte));
		tab = to_virt(pfn << PAGE_SHIFT);
		level--;
	}
	*pte_ptr = pte;
	return pfn;
}

int validate(unsigned long addr) {
	unsigned long pte;
	long r = not11_virt_to_pfn(addr, &pte);
	return r > 0 && (pte & _PAGE_PRESENT) ? 1 : 0;
}
