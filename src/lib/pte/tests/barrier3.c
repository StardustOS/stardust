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
 * barrier3.c
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
 * Declare a single barrier object with barrier attribute, wait on it,
 * and then destroy it.
 *
 */

#include <os/config.h>

#ifdef ENABLE_PTE_TESTS

#include <pte/test.h>

static pthread_barrier_t barrier = NULL;
static int result = 1;

static void *
func(void * arg)
{
	/* The following line will likely cause a compiler warning but there is no serious issue to worry about in this case */
	return (void *) pthread_barrier_wait(&barrier);
}


int
pthread_test_barrier3()
{
	pthread_t t;
	pthread_barrierattr_t ba;
	assert(pthread_barrierattr_init(&ba) == 0);
	assert(pthread_barrierattr_setpshared(&ba, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_barrier_init(&barrier, &ba, 1) == 0);
	assert(pthread_create(&t, NULL, func, NULL) == 0);
	assert(pthread_join(t, (void **) &result) == 0);
	assert(result == PTHREAD_BARRIER_SERIAL_THREAD);
	assert(pthread_barrier_destroy(&barrier) == 0);
	assert(pthread_barrierattr_destroy(&ba) == 0);
	return 0;
}

#endif