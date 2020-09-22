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

/* Original author: Grzegorz Milos
 * Changes: Harald Roeck
 *          Mick Jordan
 */

#include <os/kernel.h>
#include <os/mm.h>
#include <os/smp.h>
#include <os/time.h>
#include <os/events.h>
#include <public/xen.h>
#include <public/vcpu.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <errno.h>

#define X86_EFLAGS_IF   0x00000200

int smp_init_completed = 0;

int smp_active = 1;

struct cpu_private percpu[MAX_VIRT_CPUS];

void hypervisor_callback(void);

void failsafe_callback(void);

extern void idle_thread_starter(void);

static void ipi_handler(evtchn_port_t port, void *unused) 
{
	if(per_cpu(smp_processor_id(), cpu_state) == CPU_SUSPENDING) set_need_resched(current);
}

static void init_cpu_pda(unsigned int cpu) 
{
	unsigned long *irqstack;
	irqstack = (unsigned long *)alloc_pages(STACK_SIZE_PAGE_ORDER);

	if (cpu == 0) 
	{
		asm volatile ("movl %0,%%fs ; movl %0,%%gs"::"r" (0));
		wrmsrl(0xc0000101, &(percpu[cpu]));
	}

	per_cpu(cpu, irqcount) = -1;
	per_cpu(cpu, irqstackptr) = (unsigned long)irqstack + STACK_SIZE;
	per_cpu(cpu, idle_thread) = create_idle_thread(cpu);
	BUG_ON(!per_cpu(cpu, idle_thread));
	irqstack[0] = (unsigned long)per_cpu(cpu, idle_thread);
	per_cpu(cpu, current_thread) = per_cpu(cpu, idle_thread);
	per_cpu(cpu, cpunumber) = cpu;
	per_cpu(cpu, upcall_count) = 0;
	memset(&per_cpu(cpu, shadow_time), 0, sizeof(struct shadow_time_info));
	per_cpu(cpu, ipi_port) = evtchn_alloc_ipi(ipi_handler, cpu, NULL);
	per_cpu(cpu, cpu_state) = cpu == 0 ? CPU_UP : CPU_SLEEPING;
	per_cpu(cpu, db_support) = NULL;
}

void smp_signal_cpu(int cpu) 
{
	if (smp_init_completed) 
	{
		if(cpu != smp_processor_id() && per_cpu(cpu, cpu_state) == CPU_SLEEPING) notify_remote_via_evtchn(per_cpu(cpu, ipi_port));
	}
}

static DEFINE_SPINLOCK(cpu_lock);

static int suspend_count = 0;

void smp_cpu_safe(int cpu) 
{
	struct thread *idle;
	switch (per_cpu(cpu, cpu_state)) 
	{
		case CPU_SUSPENDING:
		spin_lock(&cpu_lock);
		--suspend_count;
		spin_unlock(&cpu_lock);
		per_cpu(cpu, cpu_state) = CPU_DOWN; 
		spin_lock(&cpu_lock);
		smp_active--;
		spin_unlock(&cpu_lock);
		break;

		case CPU_RESUMING:
		local_irq_disable();
		idle = per_cpu(cpu, idle_thread);
		spin_lock(&cpu_lock);
		smp_active++;
		spin_unlock(&cpu_lock);
		restart_idle_thread((long)cpu, (long)idle->stack + idle->stack_size, (long)idle_thread_starter, idle_thread_fn);
		BUG();
		break;

		case CPU_DOWN:
		case CPU_UP:
		case CPU_SLEEPING:
		break;

		default:
			printk("Unknown processor state\n");
	}
}

int smp_num_active(void) 
{
	int result;
	spin_lock(&cpu_lock);
	result = smp_active;
	spin_unlock(&cpu_lock);
	return result;
}

static void cpu_initialize_context(unsigned int cpu) 
{
	vcpu_guest_context_t ctxt;
	struct thread *idle_thread;
	init_cpu_pda(cpu);
	idle_thread = per_cpu(cpu, idle_thread);
	memset(&ctxt, 0, sizeof(ctxt));
	ctxt.flags = VGCF_IN_KERNEL;
	ctxt.user_regs.ds = __KERNEL_DS;
	ctxt.user_regs.es = 0;
	ctxt.user_regs.fs = 0;
	ctxt.user_regs.gs = 0;
	ctxt.user_regs.ss = __KERNEL_SS;
	ctxt.user_regs.eip = idle_thread->ip;
	ctxt.user_regs.eflags = X86_EFLAGS_IF | 0x1000;	/* IOPL_RING1 */
	memset(&ctxt.fpu_ctxt, 0, sizeof(ctxt.fpu_ctxt));
	ctxt.ldt_ents = 0;
	ctxt.gdt_ents = 0;
	ctxt.user_regs.cs = __KERNEL_CS;
	ctxt.user_regs.esp = idle_thread->sp;
	ctxt.kernel_ss = __KERNEL_SS;
	ctxt.kernel_sp = ctxt.user_regs.esp;
	ctxt.event_callback_eip = (unsigned long)hypervisor_callback;
	ctxt.failsafe_callback_eip = (unsigned long)failsafe_callback;
	ctxt.syscall_callback_eip = 0;
	ctxt.ctrlreg[3] = xen_pfn_to_cr3(virt_to_mfn(start_info.pt_base));
	ctxt.gs_base_kernel = (unsigned long)&percpu[cpu];

	int err = HYPERVISOR_vcpu_op(VCPUOP_initialise, cpu, &ctxt);
	if (err) 
	{
		char *str;
		switch (err) 
		{
			case -EINVAL:
			str = "something is wrong :(";
			break;

			case -ENOENT:
			str = "no such cpu";
			break;

			case -ENOMEM:
			str = "no mem to copy ctxt";
			break;

			case -EFAULT:
			str = "bad address";
			break;

			case -EEXIST:
			str = "already initialized";
			break;

			default:
			str = "<unexpected>";
			break;
		}
		printk("vcpu%d: failed to init: error %d: %s", cpu, -err, str);
	}
}

void init_smp(void)
{
	unsigned int cpu;
	int res;
	memset(percpu, 0, sizeof(struct cpu_private) * MAX_VIRT_CPUS);
	init_cpu_pda(0);
	smp_init_completed = 1;
	
	for (cpu = 1; cpu < MAX_VIRT_CPUS; cpu++)
	{
		per_cpu(cpu, cpu_state) = CPU_DOWN;
		res = HYPERVISOR_vcpu_op(VCPUOP_is_up, cpu, NULL);
		
		if (res >= 0)
		{
			cpu_initialize_context(cpu);
			BUG_ON(HYPERVISOR_vcpu_op(VCPUOP_up, cpu, NULL));
			spin_lock(&cpu_lock);
			smp_active++;
			spin_unlock(&cpu_lock);
		}
	}
}
