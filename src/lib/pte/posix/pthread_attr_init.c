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
 * 
 * Changes: 
 * Using malloc, and general refactoring
 */

/*
 * pthread_attr_init.c
 *
 * Description:
 * This translation unit implements operations on thread attribute objects.
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

#include <stdlib.h>
#include <pte/pthread.h>
#include <pte/implement.h>
#include <errno.h>
#include <pte/pte_generic_osal.h>

/**
 * pthread_attr_init
 * @attr: pointer to an instance of pthread_attr_t
 * 
 * This function initializes a thread attributes object with default
 * attributes. On success it returns 0 and on failure it returns ENOMEM.
 */
int pthread_attr_init (pthread_attr_t * attr)
{
	pthread_attr_t attr_result;
	if (attr == NULL) return EINVAL;
	attr_result = (pthread_attr_t) malloc (sizeof (*attr_result));
	if (attr_result == NULL) return ENOMEM;
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
	attr_result->stacksize = 0;
#endif
#ifdef _POSIX_THREAD_ATTR_STACKADDR
	/* todo Set this to something sensible when we support it. */
	// I don't think this is currently supported in PTE so there's no need to change it
	attr_result->stackaddr = NULL;
#endif
	attr_result->detachstate = PTHREAD_CREATE_JOINABLE;
	attr_result->param.sched_priority = pte_osThreadGetDefaultPriority();
	attr_result->inheritsched = PTHREAD_EXPLICIT_SCHED;
	attr_result->contentionscope = PTHREAD_SCOPE_SYSTEM;
	attr_result->valid = PTE_ATTR_VALID;
	*attr = attr_result;
	return 0;
}

#endif
