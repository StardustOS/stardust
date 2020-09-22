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
 * pthread_detach.c
 *
 * Description:
 * This translation unit implements functions related to thread
 * synchronisation.
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
#include <pte/pthread.h>
#include <pte/implement.h>
#include <errno.h>
#include <pte/pte_generic_osal.h>

int
pthread_detach (pthread_t thread)
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *      This function detaches the given thread.
 *
 * PARAMETERS
 *      thread
 *              an instance of a pthread_t
 *
 *
 * DESCRIPTION
 *      This function detaches the given thread. You may use it to
 *      detach the main thread or to detach a joinable thread.
 *      NOTE:   detached threads cannot be joined;
 *              storage is freed immediately on termination.
 *
 * RESULTS
 *              0               successfully detached the thread,
 *              EINVAL          thread is not a joinable thread,
 *              ENOSPC          a required resource has been exhausted,
 *              ESRCH           no thread could be found for 'thread',
 *
 * ------------------------------------------------------
 */
{
	int result;
	unsigned char destroyIt = 0;
	pte_thread_t * tp = (pte_thread_t *) thread.p;


	pte_osMutexLock (pte_thread_reuse_lock);

	if (NULL == tp
			|| thread.x != tp->ptHandle.x)
	{
		result = ESRCH;
	}
	else if (PTHREAD_CREATE_DETACHED == tp->detachState)
	{
		result = EINVAL;
	}
	else
	{
		/*
		 * Joinable pte_thread_t structs are not scavenged until
		 * a join or detach is done. The thread may have exited already,
		 * but all of the state and locks etc are still there.
		 */
		result = 0;

		if (pthread_mutex_lock (&tp->cancelLock) == 0)
		{
			if (tp->state != PThreadStateLast)
			{
				tp->detachState = PTHREAD_CREATE_DETACHED;
			}
			else if (tp->detachState != PTHREAD_CREATE_DETACHED)
			{
				/*
				 * Thread is joinable and has exited or is exiting.
				 */
				destroyIt = 1;
			}
			(void) pthread_mutex_unlock (&tp->cancelLock);
		}
		else
		{
			/* cancelLock shouldn't fail, but if it does ... */
			result = ESRCH;
		}
	}

	pte_osMutexUnlock(pte_thread_reuse_lock);

	if (result == 0)
	{
		/* Thread is joinable */

		if (destroyIt)
		{
			/* The thread has exited or is exiting but has not been joined or
			 * detached. Need to wait in case it's still exiting.
			 */
			pte_osThreadWaitForEnd(tp);
			pte_threadDestroy (thread);
		}
	}

	return (result);

}				/* pthread_detach */

#endif
