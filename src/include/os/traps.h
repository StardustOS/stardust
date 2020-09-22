/* Copyright (C) 2017, Ward Jaradat and Jonathan Lewis
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
 * Code ported and adapted from Xen/Mini-OS
 */

#ifndef _TRAPS_H_
#define _TRAPS_H_

struct pt_regs {
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long rbp;
	unsigned long rbx;
 	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rax;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long orig_rax;
	unsigned long rip;
	unsigned long cs;
	unsigned long eflags;
	unsigned long rsp;
	unsigned long ss;
};

#define DIVIDE_ERROR 0
#define GENERAL_PROTECTION_FAULT 13
#define PAGE_FAULT 14

typedef void (*fault_handler_t)(int fault, unsigned long address, struct pt_regs *regs);
typedef void (*printk_function_ptr)(const char *fmt, ...);
void dump_sp(unsigned long *sp, printk_function_ptr printk_function);
void dump_regs_and_stack(struct pt_regs *regs, printk_function_ptr printk_function);

#endif
