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

#ifndef SRC_INCLUDE_THREADS_SEMAPHORES_H_
#define SRC_INCLUDE_THREADS_SEMAPHORES_H_

#include <os/kernel.h> 
#include <os/hypervisor.h>
#include <os/mm.h>
#include <os/events.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/gnttab.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/spinlock.h>
#include <os/xmalloc.h>
#include <os/mutexes.h>
#include <os/xenbus.h>

#define DEFAULT_SEM_VALUE (int) 0

typedef struct semaphore_list_t {
	struct thread * thread;
	struct semaphore_list_t * next;
} semaphore_list_t;

typedef struct semaphore_t {
	int value;
	mutex_t * mutex;
	semaphore_list_t * head;
} semaphore_t;

semaphore_t * semaphore_create(int value);
void semaphore_pend(semaphore_t *sem);
void semaphore_post(semaphore_t *sem);
void semaphore_delete(semaphore_t *sem);
int semaphore_try_pend(semaphore_t *sem);
int semaphore_count(semaphore_t *sem);

semaphore_list_t * semaphore_list_create(struct thread * thread, semaphore_list_t * next);
semaphore_list_t * semaphore_list_append(struct thread * thread, semaphore_list_t * head);
semaphore_list_t * semaphore_list_delete_first(semaphore_list_t * head);

#endif
