/* Copyright (C) 2018, Ward Jaradat
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

#ifndef SRC_INCLUDE_OS_PTHREADS_TABLE_H_
#define SRC_INCLUDE_OS_PTHREADS_TABLE_H_

#include <os/types.h>
#include <os/console.h>
#include <os/mutexes.h>
#include <os/semaphores.h>

#include <string.h>
#include <stdlib.h>

typedef struct pthreads_table_node pthreads_table_node_struct, *pthreads_table_node_pointer;

struct pthreads_table_node {
	uint16_t key;
	void * value;
	pthreads_table_node_pointer left, right;
};

typedef struct pthreads_table pthreads_table_struct, *pthreads_table_pointer;

struct pthreads_table {
	pthreads_table_node_pointer root;
	mutex_t * lock;
};

extern pthreads_table_pointer pthreads_table_init();
extern int pthreads_table_put(pthreads_table_pointer m, uint16_t key, void *value);
extern void * pthreads_table_get(pthreads_table_pointer tls, uint16_t key);
extern int pthreads_table_set(pthreads_table_pointer m, uint16_t key, void *value);

#endif
