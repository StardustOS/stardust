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
 * pthread_self.c
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

/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *      This function returns a reference to the current running
 *      thread.
 *
 * PARAMETERS
 *      N/A
 *
 *
 * DESCRIPTION
 *      This function returns a reference to the current running
 *      thread.
 *
 * RESULTS
 *              pthread_t       reference to the current thread
 *
 * ------------------------------------------------------
 */
pthread_t pthread_self (void) {

	pthread_t self;
	pte_thread_t * sp;

	sp = (pte_thread_t *) pthread_getspecific (pte_selfThreadKey);

	if (sp != NULL)
	{
		self = sp->ptHandle;
	}
	else
	{
		/*
		 * Need to create an implicit 'self' for the currently
		 * executing thread.
		 *
		 * Note that this is a potential memory leak as there is
		 * no way to free the memory and any resources allocated
		 * by pte_new!
		 */
		self = pte_new ();
		sp = (pte_thread_t *) self.p;

		if (sp != NULL)
		{
			/*
			 * This is a non-POSIX thread which has chosen to call
			 * a POSIX threads function for some reason. We assume that
			 * it isn't joinable, but we do assume that it's
			 * (deferred) cancelable.
			 */
			sp->implicit = 1;
			sp->detachState = PTHREAD_CREATE_DETACHED;

			sp->threadId = pte_osThreadGetHandle();
			/*
			 * No need to explicitly serialise access to sched_priority
			 * because the new handle is not yet public.
			 */
			sp->sched_priority = 0;

			pthread_setspecific (pte_selfThreadKey, (void *) sp);
		}
	}

	return (self);


}				/* pthread_self */

#endif
