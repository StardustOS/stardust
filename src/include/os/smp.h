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

/* 
 * Author: Grzegorz Milos
 * Changes: Harald Roeck
 *          Mick Jordan
 */

#ifndef _SMP_H_
#define _SMP_H_

#include <os/kernel.h>
#include <os/time.h>

struct cpu_private 
{
    int irqcount;
    unsigned long irqstackptr;
    struct thread *idle_thread;   
    struct thread *current_thread;
    int cpunumber;
    int upcall_count;
    struct shadow_time_info shadow_time;
    int cpu_state;
    evtchn_port_t ipi_port;
    void *db_support;
};

extern struct cpu_private percpu[];

#define per_cpu(_cpu, _variable)   ((percpu[(_cpu)])._variable)
#define this_cpu(_variable)        per_cpu(smp_processor_id(), _variable)

void init_smp(void);

void smp_signal_cpu(int cpu);
void smp_cpu_safe(int cpu);
int smp_num_active(void);

#define ANY_CPU                    (-1)

#define CPU_DOWN        0
#define CPU_UP          1
#define CPU_SUSPENDING  2 
#define CPU_RESUMING    3 
#define CPU_SLEEPING    4 

#endif
