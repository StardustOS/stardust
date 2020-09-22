/* Copyright (C) 2017, Ward Jaradat and Jonathan Lewis
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
 * pte_new.c
 *
 * Description:
 * This translation unit implements miscellaneous thread functions.
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

#include <os/stdio.h>
#include <stdlib.h>
#include <pte/pthread.h>
#include <pte/implement.h>
#include <pte/pte_generic_osal.h>
#include <stdlib.h>

pthread_t
pte_new (void)
{
	pthread_t t;

    pthread_t nil = {NULL, 0}; // original PTE code
	// todo: we might need to change the above line to the following line for the jvm to work
	// pthread_t nil = {NULL, 0, 0};
	pte_thread_t * tp;

	/*
	 * If there's a reusable pthread_t then use it.
	 */
	t = pte_threadReusePop ();

	if (NULL != t.p)
	{
		tp = (pte_thread_t *) t.p;
	}
	else
	{
		/* No reuse threads available */
		tp = (pte_thread_t *) calloc (1, sizeof(pte_thread_t));

		if (tp == NULL)
		{
			return nil;
		}

		/* ptHandle.p needs to point to it's parent pte_thread_t. */
		t.p = tp->ptHandle.p = tp;
		t.x = tp->ptHandle.x = 0;
		// todo: may need to add the following line for jvm support
		// t.disableSuspend = tp->ptHandle.disableSuspend = 0;
	}

	/* Set default state. */
	tp->sched_priority = pte_osThreadGetMinPriority();

	tp->detachState = PTHREAD_CREATE_JOINABLE;
	tp->cancelState = PTHREAD_CANCEL_ENABLE;
	tp->cancelType = PTHREAD_CANCEL_DEFERRED;
	tp->cancelLock = PTHREAD_MUTEX_INITIALIZER;
	tp->threadLock = PTHREAD_MUTEX_INITIALIZER;

	return t;

}

#endif
