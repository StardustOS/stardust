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
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: sched.c
 *      Author: Grzegorz Milos
 *     Changes: Robert Kaiser
 *     Changes: Harald Roeck
 *
 *        Date: Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: Arch specific part of scheduler
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
#include <os/time.h>
#include <os/mm.h>
#include <os/xmalloc.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/sched.h>
#include <os/list.h>
#include <os/types.h>
#include <os/lib.h>

#define NUM_LOCAL_SPACE_MEMBERS 64

static struct local_space 
{
    struct local_space *members[NUM_LOCAL_SPACE_MEMBERS];
} _fake_local_space;

void init_local_space(void) 
{
  int i;
  for (i=0; i < NUM_LOCAL_SPACE_MEMBERS; ++i) {
    _fake_local_space.members[i] = &_fake_local_space;
  }
}

void *get_local_space(void)
{
  return &_fake_local_space;
}

void backtrace(void **bp, void *ip)
{
    int i;
    if(bp[1] < (void *)0x1000)
	return;
    if(ip == NULL)
	    ip = bp[1];

    xprintk("backtrace: \n");
    for (i=0; i < 16; ++i) {
	if (!bp || !ip)
	    break;
	xprintk("\t%016lx\n", ip);
	if ((void **)bp[0] < bp)
	    break;
	if(bp[1] < (void *)0x1000)
	    return;
	ip = bp[1];
	bp = (void **)bp[0];
    }
}

extern void thread_starter(void);
extern void idle_thread_starter(void);

struct thread* arch_create_thread(char *name, void (*function)(void *), void *stack, unsigned long stack_size, void *data)
{
    struct thread *thread;
    thread = xmalloc(struct thread);
    if (thread == NULL) {
    	return NULL;
    }
    if(stack != NULL)
    {
        thread->stack = (char *)stack;
        thread->stack_allocated = 0;
        thread->stack_size = stack_size;
    }
    else
    {
        thread->stack = (char *)alloc_pages(STACK_SIZE_PAGE_ORDER);
        if (thread->stack == NULL) {
        	xfree(thread);
        	return NULL;
        }
        thread->stack_allocated = 1;
        thread->stack_size = STACK_SIZE;
    }
    thread->specific = NULL;
    thread->name = name;
    thread->sp = (unsigned long)thread->stack + thread->stack_size;
    stack_push(thread, (unsigned long) get_local_space());
    stack_push(thread, (unsigned long) function);
    stack_push(thread, (unsigned long) data);
    if(function == idle_thread_fn)
        thread->ip = (unsigned long) idle_thread_starter;
    else
        thread->ip = (unsigned long) thread_starter;
    return thread;
}

struct thread* arch_create_thread_at(char *name, void (*function)(void *), void *stack, unsigned long stack_size, void *data, const void *addr_at)
{
    struct thread *thread;
    thread = malloc_at(addr_at, sizeof(struct thread));
    if (thread == NULL) {
    	return NULL;
    }
    if(stack != NULL)
    {
        thread->stack = (char *)stack;
        thread->stack_allocated = 0;
        thread->stack_size = stack_size;
    }
    else
    {
        thread->stack = (char *)alloc_pages(STACK_SIZE_PAGE_ORDER);
        if (thread->stack == NULL) {
        	xfree(thread);
        	return NULL;
        }
        thread->stack_allocated = 1;
        thread->stack_size = STACK_SIZE;
    }
    thread->specific = NULL;
    thread->name = name;
    thread->sp = (unsigned long)thread->stack + thread->stack_size;
    stack_push(thread, (unsigned long) get_local_space());
    stack_push(thread, (unsigned long) function);
    stack_push(thread, (unsigned long) data);
    if(function == idle_thread_fn)
        thread->ip = (unsigned long) idle_thread_starter;
    else
        thread->ip = (unsigned long) thread_starter;
    return thread;
}

struct thread initial_context;

void init_initial_context(void)
{
    initial_context.name = "boot-context";
    initial_context.stack = stack;
    initial_context.flags = 0;
    initial_context.cpu = 0;
    initial_context.preempt_count = 0x123;
    initial_context.resched_running_time = 0;
    INIT_LIST_HEAD(&initial_context.joiners);
    initial_context.id = -1;
    per_cpu(0, current_thread) = &initial_context;
}

void run_idle_thread(void)
{
    __asm__ __volatile__("mov %0,%%rsp\n\t"
                         "push %1\n\t"
                         "ret"
                         :"=m" (per_cpu(0, idle_thread)->sp)
                         :"m" (per_cpu(0, idle_thread)->ip));
}
