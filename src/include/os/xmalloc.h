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

#ifndef _XMALLOC_H_
#define _XMALLOC_H_

#include <os/lib.h>
#include <limits.h>

#define xmalloc(_type) ((_type *)xmalloc_align(sizeof(_type), __alignof__(_type)))
#define xmalloc_array(_type, _num) ((_type *)xmalloc_array_align(sizeof(_type), __alignof__(_type), _num))

extern void xfree(const void *);
extern void * xmalloc_align(size_t size, size_t align);
extern void * xrealloc(const void *, size_t size, size_t align);
void * xcalloc(size_t n, size_t size);

static inline void * xmalloc_array_align(size_t size, size_t align, size_t num)
{
	if (size && num > UINT_MAX / size) return NULL;
 	return xmalloc_align(size * num, align);
}

#endif
