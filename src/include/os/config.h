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

#ifndef _OS_CONFIG_H_
#define _OS_CONFIG_H_

/* Supports debug tracing and tprintk */
// #define ENABLE_DEBUG_TRACE

/* Enables the block device driver*/
// #define BLKFRONT

/* Enables the network device driver*/
// #define NETFRONT

/* Enables support for POSIX threads */
#define ENABLE_PTE

/* Enables the experimental fs library */
// #define ENABLE_FS

/* Permits the kernel to compile with POSIX threads tests */
// #define ENABLE_PTE_TESTS

/* Java support requires libraries from the packages repository */ 
// #define ENABLE_JAVA

/* Supports the standard maths library */
// #define ENABLE_FDLIBM

#endif
