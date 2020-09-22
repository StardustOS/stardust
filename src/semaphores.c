/* Copyright (C) 2017, Ward Jaradat, Jonathan Lewis
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

#include <os/semaphores.h>

semaphore_list_t * semaphore_list_create(struct thread * thread, semaphore_list_t * next)
{
	struct semaphore_list_t *node = (struct semaphore_list_t *)xmalloc(struct semaphore_list_t);
	node->next = next;
	node->thread = thread;
	return node;
}

semaphore_list_t * semaphore_list_append(struct thread * thread, semaphore_list_t * head)
{
	semaphore_list_t *new_node =  semaphore_list_create(thread, NULL);
	if(head == NULL)
	{
		return new_node;
	}
	semaphore_list_t *cursor = head;
	while(cursor->next != NULL)
	{
		cursor = cursor->next;
	}
	cursor->next = new_node;
	return head;
}

semaphore_list_t * semaphore_list_delete_first(semaphore_list_t * head)
{
	if(head == NULL)
	{
		return NULL;
	}
	semaphore_list_t *old_head = head;
	head = head->next;
	xfree(old_head);
	return head;
}

semaphore_t * semaphore_create(int value)
{
	struct semaphore_t * sem = (struct semaphore_t*) xmalloc(struct semaphore_t);
	sem->mutex = mutex_create();
	sem->value = value;
	sem->head = NULL;
	return sem;
}

void hibernate_thread(semaphore_t *sem)
{
	struct thread * thread = sched_current_thread();
	sem->head = semaphore_list_append(thread, sem->head);
    hibernate(thread);
	mutex_unlock(sem->mutex);
	schedule();
}

void restore_thread(semaphore_t *sem)
{
	struct thread * thread;
	thread = sem->head->thread;
	if (thread)
	{
		sem->head = semaphore_list_delete_first(sem->head);
		restore(thread);
	}
}

void semaphore_pend(semaphore_t *sem)
{
	mutex_lock(sem->mutex);
	sem->value--;
	if (sem->value<0)
	{
        hibernate_thread(sem);
	}
	else
	{
		mutex_unlock(sem->mutex);
	}
}

void semaphore_post(semaphore_t *sem)
{
	mutex_lock(sem->mutex);
	sem->value++;
	if (sem->value <= 0)
	{
        restore_thread(sem);
	}
	mutex_unlock(sem->mutex);
}

int semaphore_count(semaphore_t *sem)
{
	int count;
	mutex_lock(sem->mutex);
	count = sem->value;
	mutex_unlock(sem->mutex);
	return count;
}

int semaphore_try_pend(semaphore_t *sem)
{
	mutex_lock(sem->mutex);
	if (sem->value<=0)
	{
		mutex_unlock(sem->mutex);
		return 0;
	}
	sem->value--;
	mutex_unlock(sem->mutex);
	return 1;
}

void semaphore_delete(semaphore_t *sem)
{
	mutex_delete(sem->mutex);
	xfree(sem);
}
