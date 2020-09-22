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

#ifndef SRC_INCLUDE_THREADS_MUTEXES_H_
#define SRC_INCLUDE_THREADS_MUTEXES_H_

#include <os/spinlock.h>

typedef struct mutex_t
{
	spinlock_t * lock;
	char name[64];
} mutex_t;

mutex_t* mutex_create(void);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
void mutex_delete(mutex_t *m);
int mutex_try_lock(mutex_t *m);
int mutex_timed_lock(mutex_t *m, unsigned int timeout);

#endif
