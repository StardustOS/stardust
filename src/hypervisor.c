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

/* hypervisor.c
 *
 * Communication to/from hypervisor.
 *
 * Copyright (c) 2002-2003, K A Fraser
 * Copyright (c) 2005, Grzegorz Milos, gm281@cam.ac.uk,Intel Research Cambridge
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

#include <os/kernel.h>
#include <os/hypervisor.h>
#include <os/events.h>
#include <os/sched.h>
#include <os/smp.h>

#define active_evtchns(idx) (HYPERVISOR_shared_info->evtchn_pending[idx] & ~HYPERVISOR_shared_info->evtchn_mask[idx])

void do_hypervisor_callback(void)
{
	u32 	       l1, l2;
	unsigned int   l1i, l2i, port;
	int            cpu, count;
	vcpu_info_t   *vcpu_info;
    cpu = smp_processor_id();
    vcpu_info = &HYPERVISOR_shared_info->vcpu_info[cpu];
    do 
    {
        vcpu_info->evtchn_upcall_pending = 0;
        if (unlikely(this_cpu(upcall_count)++)) 
        {
            return;
        }

        /* xchg is used as a barrier on x86 architectures */
        l1 = xchg(&vcpu_info->evtchn_pending_sel, 0);

        while ( l1 != 0 ) 
        {
        	l1i = __ffs(l1);
        	l1 &= ~(1 << l1i);
            
            while ( (l2 = active_evtchns(l1i)) != 0 ) 
            {
            	l2i = __ffs(l2);
            	l2 &= ~(1 << l2i);
                port = (l1i << 5) + l2i;
                do_event(port);
            }
        }

        count = this_cpu(upcall_count);
        this_cpu(upcall_count) = 0;
        if(count != 1) 
        {
            printk("Event has been set during IRQ.\n");
        }

    } while (count != 1);
}


inline void mask_evtchn(u32 port) 
{
    shared_info_t *s = HYPERVISOR_shared_info;
    synch_set_bit(port, &s->evtchn_mask[0]);
}

inline void unmask_evtchn(u32 port) 
{
	shared_info_t *s = HYPERVISOR_shared_info;
	vcpu_info_t *vcpu_info = &s->vcpu_info[smp_processor_id()];
    synch_clear_bit(port, &s->evtchn_mask[0]);

    if (synch_test_bit(port, &s->evtchn_pending[0]) && !synch_test_and_set_bit(port>>5, &vcpu_info->evtchn_pending_sel)) 
    {
    	vcpu_info->evtchn_upcall_pending = 1;
    	if ( !vcpu_info->evtchn_upcall_mask ) 
        {
            force_evtchn_callback();
        }
    }
}

inline void clear_evtchn(u32 port) 
{
	shared_info_t *s = HYPERVISOR_shared_info;
    synch_clear_bit(port, &s->evtchn_pending[0]);
}
