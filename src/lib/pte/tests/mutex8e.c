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
 * mutex8e.c
 *
 *
 *      Based upon Pthreads-win32 - POSIX Threads Library for Win32
 * Copyright (C) 1998 Ben Elliston and Ross Johnson
 * Copyright (C) 1999,2000,2001 Ross Johnson
 *
 * Contact Email: rpj@ise.canberra.edu.au
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-embedded (PTE) - POSIX Threads Library for embedded systems
 *      Copyright(C) 2008 Jason Schmidlapp
 *
 *      Contact Email: jschmidlapp@users.sourceforge.net
 *
 *
 * Tests PTHREAD_MUTEX_ERRORCHECK mutex type exercising timedlock.
 * Thread locks mutex, another thread timedlocks the mutex.
 * Timed thread should timeout.
 *
 * Depends on API functions:
 *	pthread_create()
 *	pthread_mutexattr_init()
 *	pthread_mutexattr_destroy()
 *	pthread_mutexattr_settype()
 *	pthread_mutexattr_gettype()
 *	pthread_mutex_init()
 *	pthread_mutex_destroy()
 *	pthread_mutex_lock()
 *	pthread_mutex_timedlock()
 *	pthread_mutex_unlock()
 */
#include <os/config.h>

#ifdef ENABLE_PTE_TESTS
#include <stdio.h>
#include <stdlib.h>

#include <pte/test.h>
#include <errno.h>

static int lockCount = 0;

static pthread_mutex_t mutex;
static pthread_mutexattr_t mxAttr;

static void * locker(void * arg)
{
  struct timespec abstime =
    {
      0, 0
    };
  struct _timeb currSysTime;
  const unsigned long NANOSEC_PER_MILLISEC = 1000000;

  _ftime(&currSysTime);

  abstime.tv_sec = currSysTime.time;
  abstime.tv_nsec = NANOSEC_PER_MILLISEC * currSysTime.millitm;

  abstime.tv_sec += 1;

  assert(pthread_mutex_timedlock(&mutex, &abstime) == ETIMEDOUT);

  lockCount++;

  return 0;
}

int
pthread_test_mutex8e()
{
  pthread_t t;
  int mxType = -1;

  lockCount = 0;

  assert(pthread_mutexattr_init(&mxAttr) == 0);
  assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
  assert(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
  assert(mxType == PTHREAD_MUTEX_ERRORCHECK);

  assert(pthread_mutex_init(&mutex, &mxAttr) == 0);

  assert(pthread_mutex_lock(&mutex) == 0);

  assert(pthread_create(&t, NULL, locker, NULL) == 0);

  pte_osThreadSleep(2000);

  assert(lockCount == 1);

  assert(pthread_mutex_unlock(&mutex) == 0);

  assert(pthread_join(t,NULL) == 0);

  assert(pthread_mutex_destroy(&mutex) == 0);

  return 0;
}

#endif