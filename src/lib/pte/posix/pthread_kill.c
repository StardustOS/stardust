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
 * pthread_kill.c
 *
 * Description:
 * This translation unit implements the pthread_kill routine.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pte/pthread.h>
#include <pte/implement.h>
#include <pte/pte_generic_osal.h>

//int
//pthread_kill (pthread_t thread, int sig)
///*
// * ------------------------------------------------------
// * DOCPUBLIC
// *      This function requests that a signal be delivered to the
// *      specified thread. If sig is zero, error checking is
// *      performed but no signal is actually sent such that this
// *      function can be used to check for a valid thread ID.
// *
// * PARAMETERS
// *      thread  reference to an instances of pthread_t
// *      sig     signal. Currently only a value of 0 is supported.
// *
// *
// * DESCRIPTION
// *      This function requests that a signal be delivered to the
// *      specified thread. If sig is zero, error checking is
// *      performed but no signal is actually sent such that this
// *      function can be used to check for a valid thread ID.
// *
// * RESULTS
// *              ESRCH           the thread is not a valid thread ID,
// *              EINVAL          the value of the signal is invalid
// *                              or unsupported.
// *              0               the signal was successfully sent.
// *
// * ------------------------------------------------------
// */
//{
//  int result = 0;
//  pte_thread_t * tp;
//
//
//  pte_osMutexLock (pte_thread_reuse_lock);
//
//  tp = (pte_thread_t *) thread.p;
//
//  if (NULL == tp
//      || thread.x != tp->ptHandle.x
//      || 0 == tp->threadId)
//    {
//      result = ESRCH;
//    }
//
//  pte_osMutexUnlock(pte_thread_reuse_lock);
//
//  if (0 == result && 0 != sig)
//    {
//      /*
//       * Currently does not support any signals.
//       */
//      result = EINVAL;
//    }
//
//  return result;
//
//}				/* pthread_kill */
//


/*
 * pthread_kill.c
 *
 * Description:
 * This translation unit implements the pthread_kill routine.
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
#include <os/sched.h>
#include <stdlib.h>
#include <os/suspension.h>
#include <string.h>

#include <pte/pthread.h>
#include <pte/implement.h>

#include <errno.h>

#include <os/trace.h>

#define DEBUG_ME


int pthread_kill (pthread_t thread, int sig)
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *      This function requests that a signal be delivered to the
 *      specified thread. If sig is zero, error checking is
 *      performed but no signal is actually sent such that this
 *      function can be used to check for a valid thread ID.
 *
 * PARAMETERS
 *      thread  reference to an instances of pthread_t
 *      sig     signal. Currently only a value of 0 is supported.
 *
 *
 * DESCRIPTION
 *      This function requests that a signal be delivered to the
 *      specified thread. If sig is zero, error checking is
 *      performed but no signal is actually sent such that this
 *      function can be used to check for a valid thread ID.
 *
 * RESULTS
 *              ESRCH           the thread is not a valid thread ID,
 *              EINVAL          the value of the signal is invalid
 *                              or unsupported.
 *              0               the signal was successfully sent.
 *
 * ------------------------------------------------------
 */
{

	int result = 0;
	pte_thread_t * tp;
	tp = (pte_thread_t *) thread.p;

	if (NULL == tp || thread.x != tp->ptHandle.x || 0 == tp->threadId || tp->state == PThreadStateLast) {
        tprintk("pthread_kill: do nothing called on non-existent pthread %i\n", 0);
        return ESRCH;
	}

//    struct thread * osthread = sched_get_thread(tp->threadId);
//    if (osthread == NULL) { // Do nothing - the target thread doesn't exist in scheduler's list
//        tprintk("pthread_kill: do nothing called on non-existent osthread %i\n", tp->threadId);
//        return ESRCH;
//    }

    if (0 == result && 0 != sig && sig != SIGUSR1)
	{
		/*
		 * Currently does not support any signals apart from SIGUSR1
		 */
		return EINVAL;
	}

	if (sig == 0) {
		return 0;
	}

	pte_osMutexLock (pte_thread_reuse_lock);

	pte_osThreadHandle pthread_id = tp->threadId;
	uint16_t t_handle = pthread_id;

	struct thread * current_thread = sched_current_thread();

	int attempting_to_suspend_self = (current_thread->id == t_handle);

    struct thread * osthread = tp->os_thread_pntr;

	if (sig == SIGUSR1) {
		if (is_suspended(osthread)) {

			// resume the thread
			resume_pthread(tp);
		}
#ifdef ENABLE_JAVA
		else {

			// suspend the thread
			// todo
			if (thread.disableSuspend == 0)
				suspend_pthread(tp, attempting_to_suspend_self);
			// special case: suspend_pthread unlocks the mutex pte_thread_reuse_lock (prior to calling sched)
            // when a thread calls pthread_kill on itself
		}
#endif

	}

	if (!attempting_to_suspend_self) {
	    // normal case: unlock pte_thread_reuse_lock mutex here
	    // unless a thread called pthread_kill on itself, in which case mutex has already been unlocked by calling
        // suspend_pthread call abvove
		pte_osMutexUnlock(pte_thread_reuse_lock);
	}

	return result;

} /* pthread_kill */

#endif
