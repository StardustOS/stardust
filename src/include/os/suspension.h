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

#ifndef _OS_SUSPENSION_H_
#define _OS_SUSPENSION_H_

/* This has been written initially to support the suspension of pthreads which
is typically achieved through the use of signals in Unix-like systems. In this
environment we provide thread suspend and resume functions in the scheduler of
the kernel and couple them with pthread_kill which we ported and modified from
the PTE library to accommodate the requirements of the ported Java interpreter. 
*/

#include <os/sched.h>
#include <os/xmalloc.h>
#include <pte/pthread.h>
#include <pte/implement.h>
#include <os/xmalloc.h>

void suspend_pthread(pte_thread_t *pte_thread_pntr, int attempting_to_suspend_self);
void resume_pthread(pte_thread_t *pte_thread_pntr);

#endif
