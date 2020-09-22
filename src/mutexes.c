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

#include <os/mutexes.h>
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
#include <os/xenbus.h>

mutex_t* mutex_create()
{
	struct mutex_t * m = (struct mutex_t*) xmalloc(struct mutex_t);
	m->lock = create_spin_lock();
	return m;
}

void mutex_lock(mutex_t *m)
{
	os_spin_lock(m->lock);
}

void mutex_unlock(mutex_t *m)
{
	spin_unlock(m->lock);
}

void mutex_delete(mutex_t *m)
{
	delete_spin_lock(m->lock);
	xfree(m);
}

int mutex_try_lock(mutex_t *m)
{
	int result = 0;
	if(arch_spin_can_lock(m->lock))
	{
		os_spin_lock(m->lock);
		result = 1;
	}
	return result;
}

int mutex_timed_lock(mutex_t *m, unsigned int timeout)
{
	s_time_t end_time;
	end_time = NOW() + MILLISECS(timeout);
	while(NOW() < end_time)
	{
		if (mutex_try_lock(m) == 1)
		{
			return 1;
		}
	}
	return 0;
}
