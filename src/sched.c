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

/* (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: sched.c
 *      Author: Grzegorz Milos
 *     Changes: Robert Kaiser
 *     Changes: Grzegorz Milos
 *              Harald Roeck
 *               - split runqueue into multiple queues: ready, zombie, wait
 *               - make them static to this file
 *               - use of upcalls
 *              Mick Jordan - misc changes
 *
 *        Date: Aug 2005 onwards
 *
 * Environment: Xen Minimal OS
 * Description: simple scheduler
 *
 * The scheduler is preemptive, and schedules with a Round Robin algorithm.
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
#include <os/time.h>
#include <os/mm.h>
#include <os/xmalloc.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/events.h> 
#include <os/completion.h>
#include <os/spinlock.h>
#include <os/list.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/mutexes.h>

#define DEFAULT_TIMESLICE_MS 	20

#define DEFAULT_SLEEP_MS 		1

static long timeslice = MILLISECS(DEFAULT_TIMESLICE_MS);

static uint16_t thread_id = MAX_VIRT_CPUS;

struct thread * sched_current_thread()
{
	return current;
}

static LIST_HEAD(ready_queue);
static DEFINE_SPINLOCK(ready_lock);

static LIST_HEAD(sleep_queue);
static DEFINE_SPINLOCK(sleep_lock);

static LIST_HEAD(dead_queue);
static DEFINE_SPINLOCK(dead_lock);

DEFINE_SPINLOCK(thread_list_lock);
LIST_HEAD(thread_list);

struct thread * sched_get_thread(uint16_t id)
{
	struct thread *thread;
	struct list_head *list_head;
	spin_lock(&thread_list_lock);
	list_for_each(list_head, &thread_list)
    {
		thread = list_entry(list_head, struct thread, thread_list);
		if(thread->id == id)
		{
			spin_unlock(&thread_list_lock);
			return thread;
		}
	}
	spin_unlock(&thread_list_lock);
	return NULL;
}

static void sched_add_thread_list(struct thread *thread)
{
	spin_lock(&thread_list_lock);
	list_add_tail(&thread->thread_list, &thread_list);
	spin_unlock(&thread_list_lock);
}

static void sched_del_thread_list(struct thread *thread)
{
	spin_lock(&thread_list_lock);
	list_del_init(&thread->thread_list);
	spin_unlock(&thread_list_lock);
}

void os_sched_delete(struct thread *thread)
{
	sched_del_thread_list(thread);
}

void sched_print_ready_queue()
{
	struct list_head *it;
	struct thread *th;
	long flags;
	int i;
	printk("Scheduler's ready queue [queue %lx, lock %lx, timestamp %ld]:\n", &ready_queue, &ready_lock, NOW());
	th = current;
	printk("\tCurrent thread \"%s\", id=%d, flags %x, cpu %d\n", th->name, th->id, th->flags, th->cpu);
	i =0 ;
	spin_lock_irqsave(&ready_lock, flags);
	list_for_each(it, &ready_queue)
	{
		BUG_ON(++i > thread_id);
		th = list_entry(it, struct thread, ready_list);
		printk("\tThread \"%s\", id=%d, flags %x, cpu %d\n", th->name, th->id, th->flags, th->cpu);
	}
	printk("\n");
	spin_unlock_irqrestore(&ready_lock, flags);
}

void sched_print_sleep_queue()
{
	struct list_head *it;
	struct thread *th;
	struct sleep_queue *sq;
	int i;
	long flags;
	printk("Scheduler's sleep queue [queue %lx, lock %lx, timestamp %ld]:\n", &sleep_queue, &sleep_lock, NOW());
	i = 0;
	spin_lock_irqsave(&sleep_lock, flags);
	list_for_each(it, &sleep_queue)
	{
		BUG_ON(++i > thread_id);
		sq = list_entry(it, struct sleep_queue, list);
		th = sq->thread;
		printk("\tThread \"%s\", id=%d, flags %x, wakeup %ld, \n", th->name, th->id, th->flags, sq->timeout);
	}
	printk("\n");
	spin_unlock_irqrestore(&sleep_lock, flags);
}

void sched_print_threads()
{
	struct list_head *it;
	struct thread *th;
	printk("Scheduler's list of threads [timestamp %ld]:\n", NOW());
	spin_lock(&thread_list_lock);
	list_for_each(it, &thread_list)
	{
		th = list_entry(it, struct thread, thread_list);
		printk("\tThread \"%s\", id=%d, flags %x, preempt %d, cpu %d\n", th->name, th->id, th->flags, th->preempt_count, th->cpu);
	}
	spin_unlock(&thread_list_lock);
}

void sched_print_hibernating_threads()
{
	struct list_head *it;
	struct thread *th;
	printk("Scheduler's list of hibernating threads [timestamp %ld]:\n", NOW());
	spin_lock(&thread_list_lock);
	list_for_each(it, &thread_list)
	{
		th = list_entry(it, struct thread, thread_list);
		if (is_hibernating(th))
			printk("\tThread \"%s\", id=%d, flags %x, preempt %d, cpu %d\n", th->name, th->id, th->flags, th->preempt_count, th->cpu);
	}
	spin_unlock(&thread_list_lock);
}

s_time_t sched_blocking_time(int cpu) 
{
	s_time_t timeout, then;
	struct sleep_queue *q;
	long flags;
	then = NOW();
	timeout = NOW() + SECONDS(DEFAULT_SLEEP_MS);
	spin_lock_irqsave(&sleep_lock, flags);
	
	if (!list_empty(&sleep_queue)) 
	{
		q = list_entry(sleep_queue.next, struct sleep_queue, list);
		timeout = q->timeout;
	}

	spin_unlock_irqrestore(&sleep_lock, flags);

	if (timeout < then) 
	{
		/* This causes the idle thread to call the scheduler */
		timeout = -1; 
	}

	return timeout;
}

void sched_kick_processor(int cpu)
{
	if(cpu >= 0 && smp_init_completed)
	{
		smp_signal_cpu(cpu);
	}
}

static void sched_wake_expired(void) 
{

	struct list_head *iterator;
	struct thread *thread;
	struct sleep_queue *sq;
	long flags;
	s_time_t now = NOW();
	spin_lock_irqsave(&sleep_lock, flags);

	list_for_each(iterator, &sleep_queue)
	{
		sq = list_entry(iterator, struct sleep_queue, list);
		if(sq->timeout <= now) 
		{
			thread = sq->thread;
			wake(thread);
		} 
		else 
		{
			break;
		}
	}

	spin_unlock_irqrestore(&sleep_lock, flags);
}

int join_thread(struct thread *joinee)  
{
	struct thread *this_thread = current;
    spin_lock(&dead_lock);

	/* TODO Jon & Ward
	 * Need to fix the inefficient thread lookup here
	 * The problem is, how do we detect at kernel level that the pointer we have to a
	 * thread struct is no longer valid -- the kernel reaps dead threads, so the pntr could
	 * already be pointing at something else -- possibly by maintaining a pntr back to PTE thread pntr
	 * and checking whether its state is PThreadStateLast or similar.
	 */

	if (!is_dying(this_thread) && sched_get_thread(joinee->id) != NULL)
	{
		this_thread->regs = NULL;
		block(this_thread);
		list_add_tail(&this_thread->ready_list, &joinee->joiners);
		spin_unlock(&dead_lock);
        set_joining(this_thread);
		schedule();
        clear_joining(this_thread);
    }
    else
    {
		spin_unlock(&dead_lock);
		return -1;
	}

	return 0;
}

int sched_wake_joiner(struct thread *joiner) 
{
	list_del_init(&joiner->ready_list);
	wake(joiner);
	return 0;
}

static void sched_wake_joiners(struct thread *joinee) 
{
	struct list_head *iterator, *tmp;
	struct thread *thread;
	list_for_each_safe(iterator, tmp, &joinee->joiners)
	{
		thread = list_entry(iterator, struct thread, ready_list);
		list_del_init(&thread->ready_list);
        wake(thread);
	}
}

static void sched_reap_dead(void) 
{
	struct list_head *iterator, *tmp;
	struct thread *thread;
	spin_lock(&dead_lock);

	list_for_each_safe(iterator, tmp, &dead_queue) 
	{
		thread = list_entry(iterator, struct thread, ready_list);
		if(unlikely(is_dying(thread))) 
		{
			if(!is_running(thread) && (thread->cpu == smp_processor_id())) 
			{
				list_del_init(&thread->ready_list);
				if(!list_empty(&thread->joiners)) 
				{
					sched_wake_joiners(thread);
				}

				if (thread->stack_allocated)
				{
					free_pages(thread->stack, STACK_SIZE_PAGE_ORDER);
				}

				sched_del_thread_list(thread);
				xfree(thread);
			}
		}
	}

	spin_unlock(&dead_lock);
}

void sched_switch_thread_in(struct thread *prev) 
{
	clear_running(prev);
	local_irq_enable();
}

static inline struct thread *pick_thread(struct thread *prev, int cpu)
{
	struct thread *next, *thread;
	struct list_head *t;
	long flags;
	int i;

	next = NULL;
	spin_lock_irqsave(&ready_lock, flags);

	if(is_runnable(prev) && prev != this_cpu(idle_thread))
	{
		list_del_init(&prev->ready_list);
		list_add_tail(&prev->ready_list, &ready_queue);
	}

	/* If there is a 'runnable' thread that is not running on some other processor, 
	then it is the next thread to run */

	i = 0;

	list_for_each(t, &ready_queue)
    {
		if(++i > thread_id)
		{ 
			/* if the ready queue is corrupted then break and raise a bug */
			spin_unlock_irqrestore(&ready_lock, flags);
			BUG();
		}

		thread = list_entry(t, struct thread, ready_list);

		/* Jon and Ward: the original logic has been modified, we need a load balancer to deal 
		with threads being assigned on processors */
		if (!is_running(thread)) {
			if(is_runnable(thread) && !(is_running(thread) && thread->cpu != cpu)) {
				/* Comment out conditional statement to permit threads switching processors, each 
				thread will only be scheduled on the processor it was first scheduled on (thread
				affinity) */
				if (thread->cpu == cpu || thread->cpu == -1) 
				{
					next = thread;
					break;
				}
			}
		}

	}

	if (next != NULL) 
	{
		/* Changing the logic here; this used to be:
		if (is_running(next) && next->cpu != cpu) */
		if (is_running(next)) 
		{
			printk("Scheduling error, %d\n", next->id);
			BUG();
		}

		set_running(next);
		spin_unlock_irqrestore(&ready_lock, flags);
		return next;
	}

	spin_unlock_irqrestore(&ready_lock, flags);
	next = this_cpu(idle_thread);
	set_running(next);
	return next;
}

#define save_r14 "mov %%r14, %[sr14]"

#define restore_r14 "mov %[sr14], %%r14"

void schedule(void)
{
	struct thread *prev, *next;
	int cpu;

	BUG_ON(in_irq());
	BUG_ON(irqs_disabled());
	prev = current;
	if (!is_pthread(current))
	{
	    BUG_ON(in_spinlock(prev));
        BUG_ON(!is_preemptible(prev));
    }

	u64 running_time = get_running_time();
	prev->cum_running_time += running_time - prev->start_running_time;

	/* Check if there are any threads that need to be woken up. 
	The overhead of checking it every schedule could be avoided if we check it from the timer interrupt handler */
	sched_wake_expired();
	preempt_disable();

	cpu = prev->cpu;

	/* Schedule the idle thread, while the processor is down */
	if(per_cpu(cpu, cpu_state) != CPU_UP)
	{
		next = per_cpu(cpu, idle_thread);
	}
	else 
	{
		/* Lookup the next thread to run */
		next = pick_thread(prev, cpu);
		if (!is_runnable(next) && next != per_cpu(cpu, idle_thread)) {
			printk("Scheduling error, %d %x %d %x\n", next->id, next->flags, prev->id, prev->flags);
			BUG();
		}
	}

	BUG_ON(is_dying(next));

	if (next->cpu != cpu) next->cpu = cpu;
	clear_need_resched(prev);
	next->resched_running_time = running_time + next->timeslice;
	set_timer_interrupt(next->timeslice);
	next->start_running_time = running_time;

	if(prev != next) 
	{
		BUG_ON(irqs_disabled());
		local_irq_disable();
		this_cpu(current_thread) = next;
		if (1) 
		{
			struct fp_regs *fpregs = prev->fpregs;
			asm (save_fp_regs_asm : : [fpr] "r"(fpregs));
		}
		asm (save_r14 : [sr14] "=m" (prev->r14));
		if (1) 
		{
			struct fp_regs *fpregs = next->fpregs;
			asm (restore_fp_regs_asm : : [fpr] "r"(fpregs));
		}
		asm (restore_r14 : : [sr14] "m" (next->r14));
		switch_threads(prev, next, prev);
		sched_switch_thread_in(prev);
	} 
	else 
	{
		if (prev != this_cpu(idle_thread)) 
		{
			// this needs to go away, used for debugging occasionally
		}
	}
	preempt_enable();
}

void preempt_schedule(void) 
{
	struct thread *ti = current;

	if (unlikely(ti->preempt_count || irqs_disabled())) 
	{
		return;
	}

	need_resched:
	BUG_ON(!need_resched(ti));
	ti->regs = NULL;
	add_preempt_count(current, PREEMPT_ACTIVE);
	schedule();
	sub_preempt_count(current, PREEMPT_ACTIVE);
	BUG_ON(ti->preempt_count != 0);
	BUG_ON(ti->regs != NULL);
	barrier();

	if (unlikely(need_resched(ti)))
	{
		goto need_resched;
	}
}

void preempt_schedule_irq(void)
{
	struct thread *ti = current;
	BUG_ON(ti->preempt_count || !irqs_disabled());
	need_resched:
	add_preempt_count(current, PREEMPT_ACTIVE);
	local_irq_enable();
	schedule();
	local_irq_disable();
	sub_preempt_count(current, PREEMPT_ACTIVE);
	BUG_ON(ti->preempt_count != 0);
	barrier();

	if (unlikely(need_resched(ti))) 
	{
		goto need_resched;
	}
}

static struct thread* create_thread_with_id_stack(char *name, void (*function)(void *), int flags, void *stack, unsigned long stack_size, void *data, uint16_t id)
{

	struct thread *thread;

	thread = arch_create_thread(name, function, stack, stack_size, data);

	if (thread == NULL) 
	{
		return NULL;
	}
	
	thread->flags = flags;
	thread->regs = NULL;
	thread->fpregs = (struct fp_regs *)alloc_page();
	
	if (thread->fpregs == (struct fp_regs *) 0) 
	{
		return NULL;
	}
	
	thread->fpregs->mxcsr = MXCSRINIT;

	if (stack == NULL ) 
	{
		thread->cpu = -1;
	}

	thread->preempt_count = 0;
	thread->resched_running_time = 0;
	thread->cum_running_time = 0;
	thread->timeslice = timeslice;
	thread->lock_count = 0;
	thread->appsched_id = -1;
	clear_running(thread);
	INIT_LIST_HEAD(&thread->joiners);
	INIT_LIST_HEAD(&thread->ready_list);
	INIT_LIST_HEAD(&thread->thread_list);
	INIT_LIST_HEAD(&thread->aux_thread_list);
	thread->id = id;
	sched_add_thread_list(thread);
	BUG_ON(thread->id == 0);

	if (thread->flags == UKERNEL_FLAG)
	{
		start_thread(thread);
	}

	return thread;
}

static struct thread* create_thread_with_id_stack_at(char *name, void (*function)(void *), int flags, void *stack, unsigned long stack_size, void *data, uint16_t id, const void *addr_at)
{

	struct thread *thread;

	thread = arch_create_thread_at(name, function, stack, stack_size, data, addr_at);

	if (thread == NULL) 
	{
		return NULL;
	}
	
	thread->flags = flags;
	thread->regs = NULL;
	thread->fpregs = (struct fp_regs *)alloc_page();
	
	if (thread->fpregs == (struct fp_regs *) 0) 
	{
		return NULL;
	}
	
	thread->fpregs->mxcsr = MXCSRINIT;

	if (stack == NULL ) 
	{
		thread->cpu = -1;
	}

	thread->preempt_count = 0;
	thread->resched_running_time = 0;
	thread->cum_running_time = 0;
	thread->timeslice = timeslice;
	thread->lock_count = 0;
	thread->appsched_id = -1;
	clear_running(thread);
	INIT_LIST_HEAD(&thread->joiners);
	INIT_LIST_HEAD(&thread->ready_list);
	INIT_LIST_HEAD(&thread->thread_list);
	INIT_LIST_HEAD(&thread->aux_thread_list);
	thread->id = id;
	sched_add_thread_list(thread);
	BUG_ON(thread->id == 0);

	if (thread->flags == UKERNEL_FLAG)
	{
		start_thread(thread);
	}

	return thread;
}

struct thread* create_thread_with_stack(char *name, void (*function)(void *), int flags, void *stack, unsigned long stack_size, void *data)
{
	return create_thread_with_id_stack(name, function, flags, stack, stack_size, data, thread_id++);
}

struct thread* create_thread_with_stack_at(char *name, void (*function)(void *), int flags, void *stack, unsigned long stack_size, void *data, const void *addr_at)
{
	return create_thread_with_id_stack_at(name, function, flags, stack, stack_size, data, thread_id++, addr_at);
}

struct thread* create_thread(char *name, void (*function), int flags, void *data)
{
	return create_thread_with_stack(name, function, flags, NULL, 0, data);
}

struct thread* create_thread_at(char *name, void (*function), int flags, void *data, const void *addr_at)
{
	return create_thread_with_stack_at(name, function, flags, NULL, 0, data, addr_at);
}

struct thread* create_idle_thread(unsigned int cpu)
{
	char buf[256];
	struct thread *thread;

	sprintf(buf, "Idle%d", cpu);
	thread = arch_create_thread(strdup(buf), idle_thread_fn, NULL, 0, (void *)(unsigned long)cpu);
	thread->flags = UKERNEL_FLAG;
	thread->regs = NULL;
	thread->fpregs = (struct fp_regs *)alloc_page();
	thread->fpregs->mxcsr = MXCSRINIT;
	thread->cpu = cpu;
	thread->preempt_count = 1;
	thread->resched_running_time = 0;
	thread->lock_count = 0;
	thread->appsched_id = -1;
	INIT_LIST_HEAD(&thread->joiners);
	thread->id = cpu;
	set_running(thread);
	return thread;
}

void exit_thread(struct thread *thread)
{
	preempt_disable();
	BUG_ON(!is_running(thread));
	block(thread);
	spin_lock(&dead_lock);
	set_dying(thread);
	list_add_tail(&thread->ready_list, &dead_queue);

	if(!list_empty(&thread->joiners))
	{
		sched_wake_joiners(thread);
	}
	
	spin_unlock(&dead_lock);
	preempt_enable();
	schedule();
}

void exit_current_thread(void)
{
	struct thread *thread = current;
	exit_thread(thread);
}

void hibernate(struct thread *thread)
{
	long flags;
	spin_lock_irqsave(&ready_lock, flags);
    clear_runnable(thread);
	clear_running(thread);
	set_hibernating(thread);

	if ( !(is_suspended(thread) || is_sleeping(thread)) ) 
	{
		list_del_init(&thread->ready_list);
	}

	spin_unlock_irqrestore(&ready_lock, flags);
}

void restore(struct thread *thread) 
{
	long flags;
	spin_lock_irqsave(&ready_lock, flags);
	clear_hibernating(thread);
	
	if (!(is_suspended(thread) || is_sleeping(thread))) 
	{
	    set_runnable(thread);
		list_add_tail(&thread->ready_list, &ready_queue);
	}
    
	spin_unlock_irqrestore(&ready_lock, flags);
    if (!(is_suspended(thread) || is_sleeping(thread)) && thread->cpu != smp_processor_id()) sched_kick_processor(thread->cpu);
}

void suspend_thread (struct thread * thread) 
{
    long flags;
    spin_lock_irqsave(&ready_lock, flags);
    clear_runnable(thread);
    clear_running(thread);
    set_suspended(thread);

	if ( !(is_hibernating(thread) || is_sleeping(thread) || is_joining(thread)) ) 
	{
        list_del_init(&thread->ready_list);
    }

    spin_unlock_irqrestore(&ready_lock, flags);
}

void wake_suspended_thread(struct thread * thread) 
{
    long flags;
    spin_lock_irqsave(&ready_lock, flags);
    clear_suspended(thread);
    if ( !(is_hibernating(thread) || is_sleeping(thread) || is_joining(thread)) ) 
	{
        set_runnable(thread);
        list_add_tail(&thread->ready_list, &ready_queue);
    }
    spin_unlock_irqrestore(&ready_lock, flags);
    if ( !(is_hibernating(thread) || is_sleeping(thread) || is_joining(thread)) && thread->cpu != smp_processor_id()) sched_kick_processor(thread->cpu);
}

void block(struct thread *thread) 
{
    if (is_runnable(thread)) 
	{
        long flags;
        spin_lock_irqsave(&ready_lock, flags);
        clear_runnable(thread);
        if (!(is_hibernating(thread) || is_suspended(thread) || is_joining(thread))) 
		{
            list_del_init(&thread->ready_list);
        }
        spin_unlock_irqrestore(&ready_lock, flags);
    } 
	else 
	{
        printk("Scheduling error, trying to block a non-runnable thread %d, flag %x\n", thread->id, thread->flags);
        BUG();
    }

}

void wake(struct thread *thread) 
{
    thread->regs = NULL;
    BUG_ON(is_dying(thread));
    if (!is_runnable(thread)) 
	{
        long flags;
        spin_lock_irqsave(&ready_lock, flags);
        if(!is_runnable(thread) && !(is_hibernating(thread) || is_suspended(thread)) ) 
		{
            BUG_ON(is_runnable(thread));
            set_runnable(thread);
            list_add_tail(&thread->ready_list, &ready_queue);
        }
        spin_unlock_irqrestore(&ready_lock, flags);
        if (!(is_hibernating(thread) || is_suspended(thread)) && thread->cpu != smp_processor_id()) sched_kick_processor(thread->cpu);
    }
}

void start_thread(struct thread *thread)
{
    thread->regs = NULL;
    long flags;
    spin_lock_irqsave(&ready_lock, flags);
    set_runnable(thread);
    list_add_tail(&thread->ready_list, &ready_queue);
    spin_unlock_irqrestore(&ready_lock, flags);
    if (thread->cpu != smp_processor_id()) sched_kick_processor(thread->cpu);
}

void sleep_queue_add(struct sleep_queue *sq)
{
	long flags;
	struct list_head *iterator;
	struct sleep_queue *entry;

	spin_lock_irqsave(&sleep_lock, flags);
	
	list_for_each(iterator, &sleep_queue) 
	{
		entry = list_entry(iterator, struct sleep_queue, list);

		if (entry->timeout > sq->timeout) 
		{
			break;
		}
	}

	list_add_tail(&sq->list, iterator);
	set_active(sq);
	spin_unlock_irqrestore(&sleep_lock, flags);
}

void sleep_queue_del(struct sleep_queue *sq)
{
	long flags;
	spin_lock_irqsave(&sleep_lock, flags);
	list_del_init(&sq->list);
	clear_active(sq);
	spin_unlock_irqrestore(&sleep_lock, flags);
}

void *create_timer(void)
{
	struct sleep_queue *sq = (struct sleep_queue *)xmalloc(struct sleep_queue);
	init_sleep_queue(sq);
	return sq;
}

void delete_timer(struct sleep_queue *sq)
{
	xfree(sq);
}

void add_timer(struct sleep_queue *sq, s_time_t timeout)
{
	sq->thread = current;
	sq->timeout = NOW() + timeout*1000000;
	sleep_queue_add(sq);
}

int remove_timer(struct sleep_queue *sq) 
{
	sleep_queue_del(sq);
	return 1;
}

int nanosleep(u64 nanosecs) 
{
	struct thread *thread = current;
	DEFINE_SLEEP_QUEUE(sq);
	preempt_disable();
	block(thread);
	sq.timeout = NOW()  + nanosecs;
	set_sleeping(thread);
	sleep_queue_add(&sq);
	preempt_enable();
	schedule();
	if (is_sleeping(thread)) 
	{
		clear_sleeping(thread);
		sleep_queue_del(&sq);
	}

	return 0;
}

int sleep(u32 millisecs) 
{
	return nanosleep(MILLISECS(millisecs));
}

static inline void ht_pause(void)
{
	__asm__ __volatile__("pause");
}

unsigned long local_irqsave(void) 
{
	unsigned long flags = 0;
	local_irq_save(flags);
	return flags;
}

void local_irqrestore(unsigned long flags) 
{
	local_irq_restore(flags);
}

struct thread* current_not_idle(void) 
{
	struct thread *current_thread = this_cpu(current_thread);
	if (current_thread != this_cpu(idle_thread)) 
	{
		return current_thread;
	}
	return NULL;
}

static void check_suspend(int cpu)
{
	if (per_cpu(cpu, cpu_state) != CPU_UP) 
	{
		preempt_disable();
		while(1) 
		{
			smp_cpu_safe(cpu); 
			ht_pause();
		}
		preempt_enable();
		BUG();
	}
}

static int runnable_threads(int cpu)
{
	struct thread *thread;
	struct list_head *iterator;
	int retval = 0;
	long flags;
	spin_lock_irqsave(&ready_lock, flags);
	list_for_each(iterator, &ready_queue) 
	{
		thread = list_entry(iterator, struct thread, ready_list);
		if (!is_running(thread)) 
		{
			retval = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&ready_lock, flags);
	return retval;
}

extern void timer_handler(evtchn_port_t ev, void *ign);

void idle_thread_fn(void *data)
{
	unsigned long cpu = (unsigned long)data;
	BUG_ON(cpu != smp_processor_id());
	bind_virq(VIRQ_TIMER, cpu, timer_handler, NULL);
	per_cpu(cpu, cpu_state) = CPU_UP;

	if(cpu > 0) 
	{
		trap_init();
	}

	__sti();
	preempt_enable();

	for(;;)
	{
		schedule();

		check_suspend(cpu);
		sched_reap_dead();
		local_irq_disable();
		s_time_t until = sched_blocking_time(cpu);
		if (until > 0 && !runnable_threads(cpu)) 
		{
			block_domain(until); 
			check_suspend(cpu);
		} 
		else 
		{
			local_irq_enable();
		}
	}
}

void init_sched(char *cmd_line) 
{
	init_local_space();
}

void delete_thread_from_sleep_queue(struct thread * thread)
{
	struct sleep_queue *found_sq;
	struct list_head *it;
	struct thread *th;
	struct sleep_queue *sq;
	int i;
	long flags;

	i = 0;
	spin_lock_irqsave(&sleep_lock, flags);

	list_for_each(it, &sleep_queue)
	{
		BUG_ON(++i > thread->id);
		sq = list_entry(it, struct sleep_queue, list);
		th = sq->thread;
		if (th->id == thread->id)
		{
			found_sq = sq;
		}
	}

	spin_unlock_irqrestore(&sleep_lock, flags);
	sleep_queue_del(found_sq);
}

void wait_for_completion(struct completion *comp)
{
	unsigned long flags;
	spin_lock_irqsave(&comp->wait.lock, flags);
	rmb();

	if(!comp->done) 
	{
		DEFINE_WAIT(wait);
		add_wait_queue(&comp->wait, &wait);
		
		do 
		{
			block(current);
			spin_unlock_irqrestore(&comp->wait.lock, flags);
			schedule();
			spin_lock_irqsave(&comp->wait.lock, flags);
			rmb();
		} while(!comp->done);
		
		remove_wait_queue(&wait);
	}

	comp->done--;
	spin_unlock_irqrestore(&comp->wait.lock, flags);
}

void complete_all(struct completion *comp) 
{
	unsigned long flags;
	spin_lock_irqsave(&comp->wait.lock, flags);
	comp->done = UINT_MAX/2;
	wmb();
	__wake_up(&comp->wait);
	spin_unlock_irqrestore(&comp->wait.lock, flags);
}

void complete(struct completion *comp) 
{
	unsigned long flags;
	spin_lock_irqsave(&comp->wait.lock, flags);
	comp->done = 1;
	wmb();
	__wake_up(&comp->wait);
	spin_unlock_irqrestore(&comp->wait.lock, flags);
}

