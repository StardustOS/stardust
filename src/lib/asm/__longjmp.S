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
.global __longjmp
.type __longjmp,@function
__longjmp:
	mov	$1,%eax
	movq	 (JB_PC*8)(%rdi),%rdx
	movq	(JB_RBX*8)(%rdi),%rbx
	movq	(JB_RBP*8)(%rdi),%rbp
	movq	(JB_R12*8)(%rdi),%r12
	movq	(JB_R13*8)(%rdi),%r13
	movq	(JB_R14*8)(%rdi),%r14
	movq	(JB_R15*8)(%rdi),%r15
	movq	(JB_RSP*8)(%rdi),%rsp
	test	%esi,%esi
	cmovne	%esi,%eax
	jmp	*%rdx
.size __longjmp,.-__longjmp;
