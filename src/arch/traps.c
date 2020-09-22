 /* Copyright (C) 2017, Ward Jaradat
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

#include <os/kernel.h>
#include <os/traps.h>
#include <os/hypervisor.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/lib.h>
#include <os/crash.h>

void divide_error(void);
void debug(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void simd_coprocessor_error(void);
void alignment_check(void);
void spurious_interrupt_bug(void);
void machine_check(void);

void dump_regs(struct pt_regs *regs, printk_function_ptr printk_function) 
{
	(*printk_function)("RIP: %04lx:[<%016lx>] ", regs->cs & 0xffff, regs->rip);
	(*printk_function)("\nRSP: %04lx:%016lx  EFLAGS: %08lx\n",
			regs->ss, regs->rsp, regs->eflags);
	(*printk_function)("RAX: %016lx RBX: %016lx RCX: %016lx\n",
			regs->rax, regs->rbx, regs->rcx);
	(*printk_function)("RDX: %016lx RSI: %016lx RDI: %016lx\n",
			regs->rdx, regs->rsi, regs->rdi);
	(*printk_function)("RBP: %016lx R08: %016lx R09: %016lx\n",
			regs->rbp, regs->r8, regs->r9);
	(*printk_function)("R10: %016lx R11: %016lx R12: %016lx\n",
			regs->r10, regs->r11, regs->r12);
	(*printk_function)("R13: %016lx R14: %016lx R15: %016lx\n",
			regs->r13, regs->r14, regs->r15);
}

void dump_regs_and_stack(struct pt_regs *regs, printk_function_ptr printk_function)
{
	struct thread *thread = current;
	if (thread) (*printk_function)("Thread: %s, %d, CPU=%d\n", thread->name, thread->id, thread->cpu);

	dump_regs(regs, printk_function);

	dump_sp((unsigned long*)regs->rsp, printk_function);

	if (thread)
	{
		(*printk_function)("stack backtrace:\n");
		backtrace((void **)regs->rbp, (void *)regs->rip);
	}
}

void dump_sp(unsigned long *sp, printk_function_ptr printk_function) 
{
	(*printk_function)("[RSP++]\n");
	{
		int i = 0;
		for (i = 1; i <= 64; i++) {
			if (validate((unsigned long)sp)) {
				(*printk_function)("%016lx ", *sp++);
				if ((i % 4) == 0) (*printk_function)("\n");
			} else {
				(*printk_function)("(SP) is not mapped\n");
				break;
			}
		}
	}
}

static void do_trap(int trapnr, char *str, struct pt_regs * regs, unsigned long error_code)
{
	struct thread *thread = current;
	if (thread) {
		// Do nothing
	} else {
		printk("FATAL:  Unhandled Trap %d (%s), error code=0x%lx\n", trapnr, str, error_code);
		printk("Regs address %p\n", regs);
		dump_regs_and_stack(regs, printk);
		crash();
	}
}

#define DO_ERROR(trapnr, str, name) void do_##name(struct pt_regs * regs, unsigned long error_code) { do_trap(trapnr, str, regs, error_code); }

#define DO_ERROR_INFO(trapnr, str, name, sicode, siaddr) void do_##name(struct pt_regs * regs, unsigned long error_code) { do_trap(trapnr, str, regs, error_code); }

static fault_handler_t fault_handler_table[] = {
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};

#define CHECK_ERROR_INFO(trapnr, str, fname) fault_handler_t check_##fname(struct pt_regs * regs, unsigned long error_code) { struct thread *thread = current; fault_handler_t f = fault_handler_table[trapnr]; if (!f || !thread) { do_trap(trapnr, str, regs, error_code); } return f; }

CHECK_ERROR_INFO( 0, "divide error", divide_error)
CHECK_ERROR_INFO( 0, "general protection", general_protection)
CHECK_ERROR_INFO( 0, "page fault", page_fault)
DO_ERROR( 4, "overflow", overflow)
DO_ERROR( 5, "bounds", bounds)
DO_ERROR_INFO( 6, "invalid operand", invalid_op, ILL_ILLOPN, regs->eip)
DO_ERROR( 7, "device not available", device_not_available)
DO_ERROR( 9, "coprocessor segment overrun", coprocessor_segment_overrun)
DO_ERROR(10, "invalid TSS", invalid_TSS)
DO_ERROR(11, "segment not present", segment_not_present)
DO_ERROR(12, "stack segment", stack_segment)
DO_ERROR_INFO(17, "alignment check", alignment_check, BUS_ADRALN, 0)
DO_ERROR(18, "machine check", machine_check)

#define read_cr2() (HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].arch.cr2)

void do_page_fault(struct pt_regs *regs, unsigned long error_code)
{
	unsigned long addr = read_cr2();
	fault_handler_t f = check_page_fault(regs, error_code);
	(*f)(GENERAL_PROTECTION_FAULT, addr, regs);
}

void do_general_protection(struct pt_regs *regs, long error_code)
{
	fault_handler_t f = check_general_protection(regs, error_code);
	(*f)(GENERAL_PROTECTION_FAULT, regs->rip, regs);
}

void do_divide_error(struct pt_regs * regs, unsigned long error_code) {
	fault_handler_t f = check_divide_error(regs, error_code);
	(*f)(DIVIDE_ERROR, regs->rip, regs);
}

void do_coprocessor_error(struct pt_regs * regs)
{
	dump_regs_and_stack(regs, printk);
	crash();
}

void do_simd_coprocessor_error(struct pt_regs * regs)
{
	dump_regs_and_stack(regs, printk);
	crash();
}

void do_spurious_interrupt_bug(struct pt_regs * regs)
{
}

void do_int3(struct pt_regs *regs)
{
	struct thread *thread = current;
	struct fp_regs *fpregs = thread->fpregs;
	asm (save_fp_regs_asm : : [fpr] "r" (fpregs));
	BUG_ON(!is_preemptible(thread));
	set_need_resched(thread);
}


void do_debug(struct pt_regs *regs)
{
	struct thread *thread = current;
	struct fp_regs *fpregs = thread->fpregs;
	asm (save_fp_regs_asm : : [fpr] "r" (fpregs));
	BUG_ON(thread->regs != regs);
	set_need_resched(thread);
	regs->eflags &= ~0x00000100;
}

static trap_info_t trap_table[] = {
		{  0, 0, __KERNEL_CS, (unsigned long)divide_error                },
		{  1, 0, __KERNEL_CS, (unsigned long)debug                       },
		{  3, 3, __KERNEL_CS, (unsigned long)int3                        },
		{  4, 3, __KERNEL_CS, (unsigned long)overflow                    },
		{  5, 3, __KERNEL_CS, (unsigned long)bounds                      },
		{  6, 0, __KERNEL_CS, (unsigned long)invalid_op                  },
		{  7, 0, __KERNEL_CS, (unsigned long)device_not_available        },
		{  9, 0, __KERNEL_CS, (unsigned long)coprocessor_segment_overrun },
		{ 10, 0, __KERNEL_CS, (unsigned long)invalid_TSS                 },
		{ 11, 0, __KERNEL_CS, (unsigned long)segment_not_present         },
		{ 12, 0, __KERNEL_CS, (unsigned long)stack_segment               },
		{ 13, 0, __KERNEL_CS, (unsigned long)general_protection          },
		{ 14, 0, __KERNEL_CS, (unsigned long)page_fault                  },
		{ 15, 0, __KERNEL_CS, (unsigned long)spurious_interrupt_bug      },
		{ 16, 0, __KERNEL_CS, (unsigned long)coprocessor_error           },
		{ 17, 0, __KERNEL_CS, (unsigned long)alignment_check             },
		{ 19, 0, __KERNEL_CS, (unsigned long)simd_coprocessor_error      },
		{  0, 0,           0, 0                           }
};

void trap_init(void)
{
	HYPERVISOR_set_trap_table(trap_table);
}

