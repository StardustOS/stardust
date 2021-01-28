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
 * Some parts were obtained from the standard libraries like dietlibc, 
 * whereas others were implemented from scratch. This header file is 
 * expected to declare numeric conversion, pseudo-random, memory allocation, 
 * and process control functions in a Unix-like environment. However, in this
 * environment only a subset of these functions are used.
 * 
 * Cleaning here is required to remove unnecessary parts which are no
 * longer used in the kernel.
 */

#ifndef SRC_INCLUDE_STDLIB_H_
#define SRC_INCLUDE_STDLIB_H_

#include <os/config.h>
#include <stddef.h>
#include <os/types.h>
#include <os/xmalloc.h>
#include <ctype.h>

#define DEFAULT_ALIGN (sizeof(unsigned long))

typedef struct { int quot,rem; } div_t;

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nelem, size_t elsize);
void free(void *ptr);
void exit(int status);
void abort(void);
int rand_r(unsigned int * seed);
int rand(void);
void srand(unsigned int i);
long int strtol(const char *nptr, char **endptr, int base);
unsigned long int strtoul(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
int atoi(const char* s);
div_t div(int numerator, int denominator);

#ifdef ENABLE_JAVA
int getloadavg(double loadavg[], int nelem);
char *getenv(const char *name);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
#endif

/* This function is used to sort an array */
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

typedef void (*fun)(void);

extern void * malloc_at(const void *p, size_t size);
extern void free_at(void *start, size_t length);

#endif
