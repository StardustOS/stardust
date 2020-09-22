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
 * 
 * Notes:
 * 
 * Code ported and adapted from Xen/Mini-OS
 */

 /****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: types.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: 
 *              
 *        Date: May 2003
 * 
 * Environment: Guest VM microkernel evolved from Xen Minimal OS
 * Description: a random collection of type definitions
 *
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#define _LIBC_LIMITS_H_
#include <limits.h>
#undef _LIBC_LIMITS_H_

typedef unsigned char       u_char;
typedef unsigned int        u_int;
typedef unsigned long       u_long;
typedef long                quad_t;
typedef unsigned long       u_quad_t;
typedef unsigned long       uintptr_t;
typedef long int            intptr_t;

typedef struct { unsigned long pte; } pte_t;

typedef  unsigned char uint8_t;
typedef  char int8_t;
typedef  unsigned short uint16_t;
typedef  short int16_t;
typedef  unsigned int uint32_t;
typedef  int int32_t;
typedef  unsigned long uint64_t;
typedef  long int64_t;

typedef  uint16_t u16;
typedef  int16_t  s16;
typedef  uint32_t u32;
typedef  int32_t  s32;
typedef  uint64_t u64;
typedef  int64_t  s64;

#endif /* _TYPES_H_ */
