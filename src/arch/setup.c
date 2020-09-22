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

/******************************************************************************
 * common.c
 * 
 * Common stuff special to x86 goes here.
 * 
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * Copyright (c) 2005, Grzegorz Milos, Intel Research Cambridge
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
 *
 */

#include <os/kernel.h>
#include <os/sched.h>
#include <os/crash.h>

shared_info_t *HYPERVISOR_shared_info;
union start_info_union start_info_union;
start_info_t *xen_info;
char stack[STACK_SIZE]  __attribute__ ((__aligned__(STACK_SIZE)));;
void hypervisor_callback(void);
void failsafe_callback(void);

static shared_info_t *map_shared_info(unsigned long pa)
{
	if ( HYPERVISOR_update_va_mapping(
		(unsigned long)shared_info, __pte(pa | 7), UVMF_INVLPG) )
	{
		printk("Failed to map shared_info!!\n");
		crash();
	}
	return (shared_info_t *)shared_info;
}

void arch_init(start_info_t *si)
{
    unsigned long sp;
    xen_info = si;
    memcpy(&start_info, si, sizeof(*si));
    phys_to_machine_mapping = (unsigned long *)start_info.mfn_list;
    HYPERVISOR_shared_info = map_shared_info(start_info.shared_info);
    HYPERVISOR_set_callbacks((unsigned long)hypervisor_callback, (unsigned long)failsafe_callback, 0);
    asm volatile("movq %%rsp,%0" : "=r"(sp));
    BUG_ON((((unsigned long)stack) & (STACK_SIZE - 1)) != 0);
    BUG_ON((sp < (unsigned long)stack) || 
	    (sp > (unsigned long)stack + STACK_SIZE));
}

void arch_print_info(void)
{
	printk("stack		: %p-%p\n", stack, stack + STACK_SIZE);
}
