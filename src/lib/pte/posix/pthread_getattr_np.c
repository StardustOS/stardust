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

#include <os/sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <pte/pthread.h>
#include <pte/implement.h>

int
pthread_getattr_np (pthread_t thread, pthread_attr_t * attr)
{
	*attr = malloc (sizeof(struct pthread_attr_t_));
	pte_thread_t * sp;

	sp = (pte_thread_t *) thread.p;

		if (sp != NULL && sp->state != PThreadStateLast)
		{
			struct thread * t = sp->os_thread_pntr;
			if (t != NULL)
			{
				(*attr)->stacksize = t->stack_size;
				(*attr)->stackaddr = t->stack;
				return 0;
			}
		}
	return -1;
}
