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

#ifndef SRC_INCLUDE_UNISTD_H_
#define SRC_INCLUDE_UNISTD_H_

#include <os/config.h>
#include <os/types.h>
#include <os/sched.h>
#include <os/lib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#ifdef ENABLE_JAVA

#include <sys/stat.h>
#include <stddef.h>

char *getcwd(char *buf, size_t size);
int stat(const char *pathname, struct stat *statbuf);

#endif

int write (int file, const char *ptr, int len);

static inline long syscall(long number, ...) {
   return -1;
}

#endif
