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
 * File: semaphore4.c
 *
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
 *
 * --------------------------------------------------------------------------
 *
 * Test Synopsis: Verify sem_getvalue returns the correct number of waiters
 * after threads are cancelled.
 * -
 *
 * Test Method (Validation or Falsification):
 * - Validation
 *
 * Requirements Tested:
 * -
 *
 * Features Tested:
 * -
 *
 * Cases Tested:
 * -
 *
 * Description:
 * -
 *
 * Environment:
 * -
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - No output on success.
 *
 * Assumptions:
 * -
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

/* Define DEBUG_ME to enable debugging */
//#define DEBUG_ME

#include <os/config.h>

#ifdef ENABLE_PTE_TESTS

#include <pte/test.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_COUNT OS_MAX_SIMUL_THREADS

static sem_t s;

static void *
thr (void * arg)
{
	assert(sem_wait(&s) == 0);
	return NULL;
}

int
pthread_test_semaphore4(void)
{
	int value = 0;
	int i;
	pthread_t t[MAX_COUNT+1];

	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == 0);

	for (i = 1; i <= MAX_COUNT; i++)
	{
		/* The following line will likely cause a compiler warning but there is no serious issue to worry about in this case */
		assert(pthread_create(&t[i], NULL, thr, (void *)i) == 0);

		do
		{
			sched_yield();
			assert(sem_getvalue(&s, &value) == 0);
		}
		while (value != -i);

		assert(-value == i);
	}

	assert(sem_getvalue(&s, &value) == 0);
	assert(-value == MAX_COUNT);

	for (i = MAX_COUNT - 2; i >= 0; i--)
	{
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
	}

	for (i = 1; i < MAX_COUNT; i++)
	{
		#ifdef DEBUG_ME
		printf("pthread_test_semaphore4: Just about to attempt joining thread %i\n", i);
		#endif

		assert(pthread_join(t[i], NULL) == 0);
	}

	#ifdef DEBUG_ME
	printf("pthread_test_semaphore4: about to cancel thread 10\n");
	#endif

	assert(pthread_cancel(t[10]) == 0);
	assert(sem_destroy(&s) == -1);
	assert(errno == EBUSY);

	#ifdef DEBUG_ME
	printf("pthread_test_semaphore4: destroyed sem\n");
	#endif

	return 0;
}

#endif