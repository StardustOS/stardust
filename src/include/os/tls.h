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

#ifndef SRC_INCLUDE_OS_TLS_H_
#define SRC_INCLUDE_OS_TLS_H_

#include <os/types.h>
#include <os/console.h>
#include <os/mutexes.h>
#include <os/semaphores.h>

#include <string.h>
#include <stdlib.h>

typedef struct tls_node tls_node_struct, *tls_node_pointer;

struct tls_node {
	unsigned key;
	void * value;
	tls_node_pointer left, right;
};

typedef struct tls tls_struct, *tls_pointer;

struct tls {
	tls_node_pointer root;
	mutex_t * lock;
};

extern tls_pointer tls_init();

int tls_alloc(tls_pointer m, unsigned key, void *value);
extern int tls_put(tls_pointer m, unsigned key, void *value);
extern void * tls_get(tls_pointer tls, unsigned key);
extern int tls_has_key(tls_pointer m, unsigned key);
extern int tls_set(tls_pointer m, unsigned key, void *value);
extern unsigned tls_count(tls_node_pointer root);

#endif
