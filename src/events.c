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

/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: events.c
 *      Author: Rolf Neugebauer
 *     Changes: Grzegorz Milos
 *              Mick Jordan
 *
 *        Date: Jul 2003, changes Jun 2005, 2007-9
 *
 * Environment: Xen Minimal OS
 * Description: Deals with events recieved on event channels
 *
 ****************************************************************************
 */

#include <os/kernel.h>
#include <os/mm.h>
#include <os/hypervisor.h>
#include <os/events.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lib.h>

#define NR_EVS 1024

typedef struct _ev_action_t 
{
	int cpu;
	evtchn_handler_t handler;
	void *data;
	u32 count;
} ev_action_t;

static ev_action_t ev_actions[NR_EVS];

static void default_handler(evtchn_port_t port, void *data);

static unsigned long bound_ports[NR_EVS/(8*sizeof(unsigned long))];

struct virq_info 
{
	int used;
	uint32_t cpu;
	evtchn_handler_t handler;
	void *data;
	int evtchn[MAX_VIRT_CPUS];
};

static struct virq_info virq_table[NR_VIRQS];

void do_event(evtchn_port_t port)
{
	ev_action_t  *action;
	int cpu = smp_processor_id();

	add_preempt_count(current, IRQ_ACTIVE);
	mask_evtchn(port);
	clear_evtchn(port);

	if (port >= NR_EVS) 
	{
		printk("Large port number, %d\n", port);
		goto out;
	}

	action = &ev_actions[port];
	action->count++;
	
	if((action->cpu != ANY_CPU) && (action->cpu != cpu)) 
	{
		goto out;
	}
	
	if(action->handler)
	{
		action->handler(port, action->data);
	}

	out:
	unmask_evtchn(port);
	sub_preempt_count(current, IRQ_ACTIVE);
}

evtchn_port_t bind_evtchn(evtchn_port_t port, int cpu, evtchn_handler_t handler, void *data)
{
	if(ev_actions[port].handler != default_handler) 
	{
		xprintk("WARN: Handler for port %d already registered, replacing\n", port);
	}

	ev_actions[port].data = data;
	ev_actions[port].cpu = cpu;
	ev_actions[port].count = 0;
	wmb();
	ev_actions[port].handler = handler;
	unmask_evtchn(port);
	return port;
}

void unbind_evtchn(evtchn_port_t port)
{
	if (ev_actions[port].handler == default_handler) 
	{
		xprintk("WARN: No handler for port %d when unbinding\n", port);
	}

	ev_actions[port].handler = default_handler;
	wmb();
	ev_actions[port].data = NULL;
}

int bind_virq(uint32_t virq, int cpu, evtchn_handler_t handler, void *data)
{
	evtchn_bind_virq_t op;
	op.virq = virq;
	op.vcpu = smp_processor_id();
	
	if (HYPERVISOR_event_channel_op(EVTCHNOP_bind_virq, &op) != 0) 
	{
		xprintk("Failed to bind virtual IRQ %d\n", virq);
		BUG();
	}
	
	set_bit(op.port, bound_ports);
	bind_evtchn(op.port, cpu, handler, data);
	return 0;
}

void init_events(void)
{
	int i;

	for ( i = 0; i < NR_EVS; i++ ) 
	{
		ev_actions[i].handler = default_handler;
		mask_evtchn(i);
	}

	for (i=0; i< NR_VIRQS; ++i) 
	{
		virq_table[i].used = 0;
	}
}

static void default_handler(evtchn_port_t port, void *ignore)
{
	printk("Event received, port %d\n", port);
}

extern void timer_handler(evtchn_port_t ev, void *ign);

evtchn_port_t evtchn_alloc_ipi(evtchn_handler_t handler, int cpu, void *data)
{
	struct evtchn_bind_ipi bind_ipi;
	bind_ipi.vcpu = cpu;
	BUG_ON(HYPERVISOR_event_channel_op(EVTCHNOP_bind_ipi, &bind_ipi));
	set_bit(bind_ipi.port, bound_ports);
	bind_evtchn(bind_ipi.port, cpu, handler, data);
	return bind_ipi.port;
}

void evtchn_bind_to_cpu(int port, int cpu)
{
	int err;
	struct evtchn_bind_vcpu op;
	op.port = port;
	op.vcpu = cpu;
	err = HYPERVISOR_event_channel_op(EVTCHNOP_bind_vcpu, &op);
	if (err) 
	{
		printk("error in bind_vcpu %d\n", err);
	}
}
