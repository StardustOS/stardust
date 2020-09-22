/* Copyright (C) 2016, Ward Jaradat
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
 * Completion interface
 *
 * Author: Harald Roeck
 *
 */

#include <os/sched.h>
#include <os/wait.h>
#include <os/spinlock.h>

struct completion 
{
    int done;
    struct wait_queue_head wait;
};

#define COMPLETION_INITIALIZER(work) { 0, __WAIT_QUEUE_HEAD_INITIALIZER((work).wait)}
#define COMPLETION_INITIALIZER_ONSTACK(work) ({ init_completion(&work); work; })
#define DECLARE_COMPLETION(work) struct completion work = COMPLETION_INITIALIZER(work)

static inline void init_completion(struct completion *x) 
{
	x->done = 0;
	init_waitqueue_head(&x->wait);
}

extern void wait_for_completion(struct completion *);
extern void complete_all(struct completion *);
extern void complete(struct completion *x);
#define INIT_COMPLETION(x)	((x).done = 0)

