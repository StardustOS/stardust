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

#include <os/console.h>
#include <os/flush.h>
#include <os/hypervisor.h>
#include <os/sched.h>

#define print_backtrace(thread)                                                                 \
		if(thread)                                                                              \
        {                                                                                       \
			void **bp;                                                                          \
			printk("Current thread: %s, %d, cpu=%d\n", thread->name, thread->id, thread->cpu);  \
			bp = get_bp();                                                                      \
			dump_sp((unsigned long*)get_sp(), printk);                                          \
			backtrace(bp, 0);                                                                   \
		}

void poweroff(void)
{
	printk("Shutting down. Goodbye!\n");
	for( ;; )
	{
		struct sched_shutdown sched_shutdown = { .reason = 2};
		HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	}
}

void backtrace_and_poweroff(void) 
{ 
    sched_print_threads();
    print_backtrace(current);
    flush();
    poweroff();
}