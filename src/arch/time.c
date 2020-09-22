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
 * (C) 2002-2003 - Keir Fraser - University of Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 * (C) 2006 - Robert Kaiser - FH Wiesbaden
 ****************************************************************************
 *
 *        File: time.c
 *      Author: Rolf Neugebauer and Keir Fraser
 *     Changes: Grzegorz Milos
 *              Mick Jordan
 *
 * Description: Simple time and timer functions
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
#include <os/traps.h>
#include <os/hypervisor.h>
#include <os/events.h>
#include <os/time.h>
#include <os/smp.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <os/sched.h>
#include <public/vcpu.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/types.h>

static struct timespec shadow_ts;
static u32 shadow_ts_version;
static u64 suspend_time;
int suspended = 0;
s64 time_addend = 0;
DEFINE_SPINLOCK(wallclock_lock);

#define HANDLE_USEC_OVERFLOW(_tv)				\
		do {                                   	\
			while ( (_tv)->tv_usec >= 1000000 ) \
			{                                  	\
				(_tv)->tv_usec -= 1000000;      \
				(_tv)->tv_sec++;                \
			}                                  	\
		} while ( 0 )

static inline int time_values_up_to_date(void)
{
	int cpu = smp_processor_id();
	struct vcpu_time_info   *src = &HYPERVISOR_shared_info->vcpu_info[cpu].time;
	struct shadow_time_info *shadow = &per_cpu(cpu, shadow_time);

	return (shadow->version == src->version);
}

static inline u64 scale_delta(u64 delta, u32 mul_frac, int shift)
{
	u64 product;
	if ( shift < 0 )
		delta >>= -shift;
	else
		delta <<= shift;
	
	__asm__ (
			"mul %%rdx ; shrd $32,%%rdx,%%rax"
			: "=a" (product) : "0" (delta), "d" ((u64)mul_frac) );

	return product;
}


static unsigned long get_nsec_offset(void)
{
	u64 now, delta;
	struct shadow_time_info *shadow = &this_cpu(shadow_time);
	rdtscll(now);
	delta = now - shadow->tsc_timestamp;
	return scale_delta(delta, shadow->tsc_to_nsec_mul, shadow->tsc_shift);
}


static void get_time_values_from_xen(void)
{
	int cpu = smp_processor_id();
	struct vcpu_time_info    *src = &HYPERVISOR_shared_info->vcpu_info[cpu].time;
	struct shadow_time_info  *shadow = &per_cpu(cpu, shadow_time);
	do {
		shadow->version = src->version;
		rmb();
		shadow->tsc_timestamp     = src->tsc_timestamp;
		shadow->system_timestamp  = src->system_time;
		shadow->tsc_to_nsec_mul   = src->tsc_to_system_mul;
		shadow->tsc_shift         = src->tsc_shift;
		rmb();
	}
	while ((src->version & 1) | (shadow->version ^ src->version));
	shadow->tsc_to_usec_mul = shadow->tsc_to_nsec_mul / 1000;
}

u64 monotonic_clock(void)
{
	u64 time;
	u32 local_time_version;
	struct shadow_time_info  *shadow = &this_cpu(shadow_time);

	preempt_disable();
	do {
		local_time_version = shadow->version;
		rmb();
		time = shadow->system_timestamp + get_nsec_offset();
		if (!time_values_up_to_date())
			get_time_values_from_xen();
		rmb();
	} while (local_time_version != shadow->version);
	preempt_enable();

	return time;
}

static void update_wallclock(void)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	spin_lock(&wallclock_lock);
	BUG_ON(!in_irq() && smp_init_completed && !suspended);
	do {
		shadow_ts_version = s->wc_version;
		rmb();
		shadow_ts.tv_sec  = s->wc_sec;
		shadow_ts.tv_nsec = s->wc_nsec;
		rmb();
	}
	while ((s->wc_version & 1) | (shadow_ts_version ^ s->wc_version));
	spin_unlock(&wallclock_lock);
}

int gettimeofday(struct timeval *tv)
{
	u64 nsec;
	unsigned long flags;
	spin_lock_irqsave(&wallclock_lock, flags);
	nsec = monotonic_clock();
	nsec += shadow_ts.tv_nsec;
	BUG_ON(shadow_ts_version == 0);
	tv->tv_sec = shadow_ts.tv_sec;
	tv->tv_sec += NSEC_TO_SEC(nsec);
	tv->tv_usec = NSEC_TO_USEC(nsec % 1000000000UL);
	spin_unlock_irqrestore(&wallclock_lock, flags);
	return 0;
}

void set_timer_interrupt(u64 delta)
{
	if(delta < MILLISECS(1)) delta = MILLISECS(1);
	BUG_ON(HYPERVISOR_set_timer_op(monotonic_clock() + delta));
}

void block_domain(s_time_t until)
{
	if(NOW() < until) {
		HYPERVISOR_set_timer_op(until - time_addend);
		this_cpu(cpu_state) = CPU_SLEEPING;
		HYPERVISOR_sched_op(SCHEDOP_block, 0);
		this_cpu(cpu_state) = CPU_UP;
	} else
		local_irq_enable();
	BUG_ON(irqs_disabled());
}

u64 get_cpu_running_time(int cpu)
{
	vcpu_runstate_info_t runstate;
	if(!smp_init_completed) return 0ULL;
	BUG_ON(HYPERVISOR_vcpu_op(VCPUOP_get_runstate_info, cpu, &runstate));
	return runstate.time[RUNSTATE_running];
}

void check_need_resched(void)
{
	u64 resched_time, running_time;
	struct thread *current_thread = current_not_idle();

	if(current_thread != NULL)
	{
		resched_time = current_thread->resched_running_time;
		if(resched_time == 0ULL)
		{
			return;
		}
		running_time = get_running_time();
		if (running_time >= resched_time) {
			set_need_resched(current_thread);
		}
		else
		{
			set_timer_interrupt(resched_time - running_time);
		}
	}
}

void timer_handler(evtchn_port_t ev, void *ign)
{
	get_time_values_from_xen();
	update_wallclock();
	check_need_resched();
}

void init_time(void)
{
	memset(&shadow_ts, 0, sizeof(struct timespec));
	time_addend = 0;
	shadow_ts_version = 0;
	update_wallclock();
}
