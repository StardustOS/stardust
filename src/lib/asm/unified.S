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

#define SYS_exit 0x3c

.text

.weak exit
exit:
.global _exit
_exit:
	mov	$SYS_exit,%al

.global __error_unified_syscall
.global __unified_syscall
.global __unified_syscall_16bit
__unified_syscall:
	mov	$0, %ah
__unified_syscall_16bit:
	movzwl	%ax, %eax
#if defined(__ILP32__)
	or	$0x40000000, %eax
#endif
	mov	%rcx, %r10
	syscall
__error_unified_syscall:
	cmpq	$-132, %rax
	jbe	.Lnoerror
	negl	%eax
	pushq	%rax
	call	__errno_location
	popq	%rcx
	movl	%ecx,(%rax)
	orq	$-1, %rax
.Lnoerror:

	ret
.Lhere:
	.size	 __unified_syscall,.Lhere-__unified_syscall
