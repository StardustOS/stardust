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

#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <os/config.h>

#define EPERM		    1	    // Operation not permitted 
#define ENOENT		    2	    // No such file or directory 
#define ESRCH		    3	    // No such process 
#define EINTR		    4	    // Interrupted system call 
#define EIO		 	    5	    // I/O error 
#define E2BIG		    7	    // Arg list too long 
#define ENOEXEC		    8	    // Exec format error 
#define EBADF		    9	    // Bad file number 
#define ECHILD		    10	    // No child processes 
#define EAGAIN		    11	    // Try again 
#define ENOMEM		    12	    // Out of memory 
#define EACCES		    13	    // Permission denied 
#define EFAULT		    14	    // Bad address 
#define EBUSY		    16	    // Device or resource busy 
#define EEXIST		    17	    // File exists
#define EISDIR		    21	    // Is a directory
#define EINVAL		    22	    // Invalid argument 
#define ENFILE		    23	    // File table overflow 
#define EMFILE		    24	    // Too many open files 
#define ENOTTY		    25	    // Not a typewriter 
#define EFBIG		    27	    // File too large 
#define ENOSPC		    28	    // No space left on device 
#define EROFS		    30	    // Read-only file system
#define EPIPE		    32	    // Broken pipe 
#define EDOM		    33	    // Math argument out of domain of func 
#define ERANGE		    34	    // Math result not representable 
#define EDEADLK		    35	    // Resource deadlock would occur 
#define ENAMETOOLONG	36	    // File name too long 
#define ENOSYS		    38	    // Function not implemented 
#define ENOTEMPTY	    39	    // Directory not empty
#define EWOULDBLOCK	    EAGAIN  // Operation would block 
#define EOVERFLOW	    75	    // Value too large for defined data type 
#define EILSEQ		    84	    // Illegal byte sequence 
#define ENOTSOCK	    88	    // Socket operation on non-socket 
#define EDESTADDRREQ	89	    // Destination address required 
#define EMSGSIZE	    90	    // Message too long 
#define EPROTONOSUPPORT	93	    // Protocol not supported 
#define EOPNOTSUPP	    95	    // Operation not supported on transport endpoint 
#define ENOTSUP		    EOPNOTSUPP// Operation not supported on transport endpoint 
#define EAFNOSUPPORT	97	// Address family not supported by protocol 
#define EADDRINUSE	    98	// Address already in use 
#define EADDRNOTAVAIL	99	// Cannot assign requested address 
#define ENETUNREACH	    101	// Network is unreachable 
#define ECONNRESET	    104	// Connection reset by peer 
#define ENOBUFS		    105	// No buffer space available 
#define EISCONN		    106	// Transport endpoint is already connected 
#define ENOTCONN	    107	// Transport endpoint is not connected 
#define ETIMEDOUT	    110	// Connection timed out 
#define ECONNREFUSED	111	// Connection refused 
#define EHOSTUNREACH	113	// No route to host 
#define EALREADY	    114	// Operation already in progress 
#define EINPROGRESS	    115	// Operation now in progress 
#define EDQUOT		    122	// Quota exceeded 

#ifndef __ASSEMBLER__

#include <sys/cdefs.h>

#ifndef _REENTRANT
extern int errno;
#endif

extern int *__errno_location(void);

#define __set_errno(x) errno=(x)

#endif

#endif
