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

#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

#include <os/config.h>

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

typedef struct mem_region {

    unsigned long address;
    unsigned long pages;
    unsigned long end;

} mem_region;

#define MAX_MEM_REGION (100UL << (10 + 10))

#define MREMAP_MAYMOVE	1UL
#define MREMAP_FIXED	2UL
#define PROT_READ		0x1
#define PROT_WRITE		0x2
#define PROT_EXEC		0x4
#define PROT_NONE		0x0
#define MAP_SHARED		0x01
#define MAP_PRIVATE		0x02
#define MAP_ANON    	0x1000
#define MAP_FILE		0
#define MAP_FAILED  	((void *) -1)

extern void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);

extern int munmap(void *start, size_t length);

#define _POSIX_MAPPED_FILES

__END_DECLS

#endif
