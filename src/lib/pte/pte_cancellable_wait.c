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
 * pte_cancellable_wait.c
 *
 * Description:
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
#include <pte/semaphore.h>
#include <pte/implement.h>
#include <errno.h>
#include <os/trace.h>

#include <pte/pte_generic_osal.h>


int pte_cancellable_wait (pte_osSemaphoreHandle semHandle, unsigned int* timeout) {
	int result = EINVAL;
	pte_osResult osResult;
	int cancelEnabled = 0;
	pthread_t self;
	pte_thread_t * sp;

	self = pthread_self();
	sp = (pte_thread_t *) self.p;

	if (sp != NULL)
	{
		/*
		 * Get cancelEvent handle
		 */
		if (sp->cancelState == PTHREAD_CANCEL_ENABLE)
		{
			cancelEnabled = 1;
		}
	}

#ifdef ENABLE_DEBUG_TRACE
	if (sp==NULL) {
		tprintk("pte_cancellable_wait: thread:%p(%u) -- p is null\n", self.p, self.x);
	}
#endif
	pte_osThreadHandle thread_handle = sp->threadId;
#ifdef ENABLE_DEBUG_TRACE
	if (thread_handle == 0) {
		tprintk("pte_cancellable_wait: thread:%p(%u) -- thread_handle==0\n", self.p, self.x);
	}
	struct thread *t = sp->os_thread_pntr;
	if (t == NULL) {
		tprintk("pte_cancellable_wait: thread:%p(%u) -- OS-level thread pntr is null\n", self.p, self.x);
	}
#endif

	if (cancelEnabled)
	{
		tprintk("pte_cancellable_wait thread: %u(%s) about to do pte_osSemaphoreCancellablePend(%p, %p/%i)\n", t->id, t->name, semHandle, timeout, (timeout == 0 ? 0 : *timeout));
		osResult = pte_osSemaphoreCancellablePend(semHandle, timeout);
		tprintk("pte_cancellable_wait thread:%u returned from pte_osSemaphoreCancellablePend with %i\n", t->id, osResult);
	}
	else
	{
		tprintk("pte_cancellable_wait: thread:%u(%s) about to do pte_osSemaphorePend(%p, %p/%i)\n", t->id, t->name, semHandle, timeout, (timeout == 0 ? 0 : *timeout));
		osResult = pte_osSemaphorePend(semHandle, timeout);
		tprintk("pte_cancellable_wait thread:%u returned from pte_osSemaphorePend with %i\n", t->id, osResult);
	}

	switch (osResult)
	{
	case PTE_OS_OK:
	{
		result = 0;
		break;
	}

	case PTE_OS_TIMEOUT:
	{
		result = ETIMEDOUT;
		break;
	}

	case PTE_OS_INTERRUPTED:
	{
		if (sp != NULL)
		{
			/*
			 * Should handle POSIX and implicit POSIX threads..
			 * Make sure we haven't been async-canceled in the meantime.
			 */
			(void) pthread_mutex_lock (&sp->cancelLock);
			if (sp->state < PThreadStateCanceling)
			{
				sp->state = PThreadStateCanceling;
				sp->cancelState = PTHREAD_CANCEL_DISABLE;
				(void) pthread_mutex_unlock (&sp->cancelLock);
				pte_throw (PTE_EPS_CANCEL);

				/* Never reached */
			}
			(void) pthread_mutex_unlock (&sp->cancelLock);
		}
		break;
	}

	default:
	{
		result = EINVAL;
	}

	}


	return (result);

}                               /* CancelableWait */



#endif
