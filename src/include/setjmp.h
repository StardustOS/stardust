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
 * Some parts were obtained from the dietlibc project but unnecessary code
 * still needs to be removed.
 */

#ifndef _SETJMP_H
#define _SETJMP_H

#include <sys/cdefs.h>

#ifdef __x86_64__
#ifndef __ASSEMBLER__
typedef long __jmp_buf[8];
#endif
# define JB_RBX	0
# define JB_RBP	1
# define JB_R12	2
# define JB_R13	3
# define JB_R14	4
# define JB_R15	5
# define JB_RSP	6
# define JB_PC	7
# define JB_SIZE 64
#endif

#ifndef __ASSEMBLER__
#include <signal.h>

/* typedef int sig_atomic_t; */
#define __sig_atomic_t sig_atomic_t

/* Calling environment, plus possibly a saved signal mask.  */
typedef struct __jmp_buf_tag {	/* C++ doesn't like tagless structs.  */
/* NOTE: The machine-dependent definitions of `__sigsetjmp'
 * assume that a `jmp_buf' begins with a `__jmp_buf'.
 * Do not move this member or add others before it.  */
  __jmp_buf __jmpbuf;		/* Calling environment.  */
  int __mask_was_saved;		/* Saved the signal mask?  */
  sigset_t __saved_mask;	/* Saved signal mask.  */
} jmp_buf[1];

extern int __sigsetjmp(jmp_buf __env,int __savemask);

extern void longjmp(jmp_buf __env,int __val) __THROW __attribute__((__noreturn__));

typedef jmp_buf sigjmp_buf;

#define setjmp(env) __sigsetjmp(env,0)
#define sigsetjmp(a,b) __sigsetjmp(a,b)

#endif

#endif
