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
 * pthread_mutex_unlock.c
 *
 * Description:
 * This translation unit implements mutual exclusion (mutex) primitives.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-embedded (PTE) - POSIX Threads Library for embedded systems
 *      Copyright(C) 2008 Jason Schmidlapp
 *
 *      Contact Email: jschmidlapp@users.sourceforge.net
 *
 *
 *      Based upon Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 *
 *      Contact Email: rpj@callisto.canberra.edu.au
 *
 *      The original list of contributors to the Pthreads-win32 project
 *      is contained in the file CONTRIBUTORS.ptw32 included with the
 *      source code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <os/config.h>

#ifdef ENABLE_PTE

#include <pte/pte_osal.h>
#include <os/stdio.h>
#include <stdlib.h>
#include <pte/pthread.h>
#include <pte/implement.h>
#include <errno.h>
#include <pte/pte_generic_osal.h>

#define TEST_IE InterlockedExchange

int pthread_mutex_unlock (pthread_mutex_t * mutex) {

	int result = 0;
	pthread_mutex_t mx;

	/*
	 * Let the system deal with invalid pointers.
	 */

	mx = *mutex;

	/*
	 * If the thread calling us holds the mutex then there is no
	 * race condition. If another thread holds the
	 * lock then we shouldn't be in here.
	 */
	if (mx < PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
	{
		if (mx->kind == PTHREAD_MUTEX_NORMAL)
		{
			int idx;

			idx = PTE_ATOMIC_EXCHANGE ((int *) &mx->lock_idx,0);
			if (idx != 0)
			{
				if (idx < 0)
				{
					/*
					 * Someone may be waiting on that mutex.
					 */
					if (pte_osSemaphorePost(mx->handle, 1) != PTE_OS_OK)
					{
						result = EINVAL;

					}
				}
			}
			else
			{
				/*
				 * Was not locked (so can't be owned by us).
				 */
				result = EPERM;

			}
		}
		else
		{
			if (pthread_equal (mx->ownerThread, pthread_self ()))
			{
				if (mx->kind != PTHREAD_MUTEX_RECURSIVE
						|| 0 == --mx->recursive_count)
				{
					mx->ownerThread.p = NULL;

					if (PTE_ATOMIC_EXCHANGE ((int *)&mx->lock_idx,0) < 0)
					{
						if (pte_osSemaphorePost(mx->handle, 1) != PTE_OS_OK)
						{
							result = EINVAL;

						}

					}
				}
			}
			else
			{
				result = EPERM;

			}
		}
	}
	else
	{
		result = EINVAL;

	}

	return (result);
}

#endif