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
 *
 * Notes:
 * 
 * This header file defines string handling functions but cleaning here is 
 * required to remove any unnecessary, or unused code fragments.
 */

#ifndef _STRING_H
#define _STRING_H

#include <os/config.h>
#include <os/lib.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>
#include <ctype.h>

void *memmove(void* dest, const void *src, size_t n);
char *strrchr(const char *s, int c);
void *memccpy(void* __restrict__ dest, const void* __restrict__ src, int c, size_t n);
char *strerror(int errnum);
char *strerror_r(int errnum, char *buf, size_t buflen);
void* memchr(const void *s, int c, size_t n);
size_t strspn(const char *s, const char *_accept);
size_t strcspn(const char *s, const char *reject);
char *strtok(char * __restrict__ s, const char * __restrict__ delim);
char *strtok_r(char * __restrict__ s, const char * __restrict__ delim, char ** __restrict__ ptrptr);
char *strncat(char *s, const char *t, size_t n);
char *strpbrk(const char *s, const char *accept);
size_t strxfrm(char *dest, const char *src, size_t n);

#endif
