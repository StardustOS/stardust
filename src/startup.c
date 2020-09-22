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

#include <os/config.h>
#include <os/startup.h>
#include <os/console.h>
#include <os/sched.h>
 
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <pthread.h>

#ifdef ENABLE_PTE_TESTS
#include <pte/test.h>
#endif

extern void startup_thread(void *p)
{

#ifdef ENABLE_PTE_TESTS
	/* Example for running the PTE tests. Please note that
	you need to enable the PTE test files commented out in
	the makefile */
	pthread_t r;
	pthread_create(&r, NULL, pte_test_main, NULL);
#endif

}
