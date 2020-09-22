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

#include <setjmp.h>

.text
.weak setjmp
.type setjmp,@function
setjmp:
.globl __setjmp
.type __setjmp,@function
__setjmp:
	xorq	%rsi,%rsi

.globl __sigsetjmp
.type __sigsetjmp,@function
__sigsetjmp:
	movq	%rbx,(JB_RBX*8)(%rdi)
	movq	%rbp,(JB_RBP*8)(%rdi)
	movq	%r12,(JB_R12*8)(%rdi)
	movq	%r13,(JB_R13*8)(%rdi)
	movq	%r14,(JB_R14*8)(%rdi)
	movq	%r15,(JB_R15*8)(%rdi)
	leaq	8(%rsp),%rdx
	movq	%rdx,(JB_RSP*8)(%rdi)
	movq	(%rsp),%rdx
	movq	%rdx,(JB_PC*8)(%rdi)
	jmp	__sigjmp_save
.size __sigsetjmp,.-__sigsetjmp;


