/* Copyright (C) 2017, Jonathan Lewis and Ward Jaradat
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

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

int atomic_compare_exchange(int *mem, int cmp, int new);
int atomic_decrement(int *mem);
int atomic_exchange(int *mem, int new);
int atomic_exchange_add(int volatile *mem, int value);
int atomic_increment(int *mem);
unsigned long atomic_exchange_x86_64(unsigned long *mem, unsigned long new);
unsigned long atomic_compare_exchange_x86_64(unsigned long *mem, unsigned long cmp, unsigned long new);
unsigned long atomic_exchange_add_x86_64(unsigned long volatile *mem, unsigned long value);

#endif