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
 * (C) 2006 - Grzegorz Milos - Cambridge University
 ****************************************************************************
 *
 *        File: console.h
 *      Author: Grzegorz Milos
 *     Changes:
 *
 *        Date: Mar 2006
 *
 * Environment: Guest VM microkernel volved from Xen Minimal OS
 * Description: Console interface.
 *
 * Handles console I/O. Defines printk.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <os/kernel.h>
#include <os/wait.h>
#include <os/mm.h>
#include <os/hypervisor.h>
#include <os/events.h>
#include <os/xenbus.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lib.h>
#include <os/types.h>
#include <public/io/console.h>

extern int xencons_ring_init(void);

extern int xencons_ring_send(const char *data, unsigned len);

extern int xencons_ring_send_no_notify(const char *data, unsigned len);

extern int xencons_ring_avail(void);

extern struct wait_queue_head console_queue;

extern int xencons_ring_recv(char *data, unsigned len);

extern void xencons_notify_daemon(void);

static int console_initialised = 0;

static int console_dying = 0;

static int (*xencons_ring_send_fn)(const char *data, unsigned len) = xencons_ring_send_no_notify;

static DEFINE_SPINLOCK(xencons_lock);

static char *init_overflow = "init_overflow\r\n";

DECLARE_WAIT_QUEUE_HEAD(console_queue);

static inline struct xencons_interface *xencons_interface(void)
{
    return mfn_to_virt(start_info.console.domU.mfn);
}

void xencons_notify_daemon(void)
{
    notify_remote_via_evtchn(start_info.console.domU.evtchn);
}

int xencons_ring_avail(void)
{
    struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;
    cons = intf->in_cons;
    prod = intf->in_prod;
    mb();
    BUG_ON((prod - cons) > sizeof(intf->in));
    return prod - cons;
}

int xencons_ring_send_no_notify(const char *data, unsigned len)
{
    int sent = 0;
    struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;
    cons = intf->out_cons;
    prod = intf->out_prod;
    mb();
    BUG_ON((prod - cons) > sizeof(intf->out));
    while ((sent < len) && ((prod - cons) < sizeof(intf->out)))
	intf->out[MASK_XENCONS_IDX(prod++, intf->out)] = data[sent++];
    wmb();
    intf->out_prod = prod;
    return sent;
}

int xencons_ring_send(const char *data, unsigned len)
{
    int sent;
    sent = xencons_ring_send_no_notify(data, len);
    xencons_notify_daemon();
    return sent;
}	

static void handle_input(evtchn_port_t port, void *ign)
{
    wake_up(&console_queue);
}

int xencons_ring_recv(char *data, unsigned len)
{
	struct xencons_interface *intf = xencons_interface();
	XENCONS_RING_IDX cons, prod;
    unsigned filled = 0;
	cons = intf->in_cons;
	prod = intf->in_prod;
	mb();
	BUG_ON((prod - cons) > sizeof(intf->in));

    while (filled < len && cons + filled != prod) 
    {
        data[filled] = *(intf->in + MASK_XENCONS_IDX(cons + filled, intf->in));
        filled++;
	}
	
	mb();
    intf->in_cons = cons + filled;
	xencons_notify_daemon();
    return filled;
}

int xencons_ring_init(void)
{
	int err;
	if (!start_info.console.domU.evtchn) return 0;
	err = bind_evtchn(start_info.console.domU.evtchn, ANY_CPU, handle_input, NULL);
	if (err <= 0) return err;
	xencons_notify_daemon();
	return 0;
}

static void checked_print(char *data, int length) {
	int printed = 0;

	while (length > 0) 
	{
		printed = xencons_ring_send_fn(data, length);
		data += printed;
		length -= printed;
	}
}

static void console_print(char *data, int length)
{
	char *curr_char, saved_char;
	int part_len;

	if (console_dying) 
	{
		return;
	}

	if(!console_initialised && (xencons_ring_avail() - length < strlen(init_overflow)))
	{
		console_dying = 1;
		(void)HYPERVISOR_console_io(CONSOLEIO_write,
				strlen(init_overflow),
				init_overflow);
		xencons_ring_send(init_overflow, strlen(init_overflow));
		BUG();
	}

	for(curr_char = data; curr_char < data+length-1; curr_char++) 
	{
		if (*curr_char == '\n') 
		{
			saved_char = *(curr_char+1);
			*curr_char = '\r';
			*(curr_char+1) = '\n';
			part_len = curr_char - data + 2;
			checked_print(data, part_len);
			*(curr_char+1) = saved_char;
			*curr_char = '\n';
			data = curr_char+1;
			length -= part_len - 1;
		}
	}

	if(data[length-1] == '\n') 
	{
		checked_print(data, length - 1);
		checked_print("\r\n", 2);
	} 
	else 
	{
		checked_print(data, length);
	}

}

void print(int direct, const char *fmt, va_list args)
{
	static char   buf[1024];

	(void)vsnprintf(buf, sizeof(buf), fmt, args);

	if(direct)
	{
		(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
		return;
	} 
	else 
	{
#ifndef USE_XEN_CONSOLE
		if(!console_initialised)
#endif
			(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
		console_print(buf, strlen(buf));
	}
}

void printbytes(char *buf, int length)
{
	unsigned long flags;
	spin_lock_irqsave(&xencons_lock, flags);
	console_print(buf, length);
	spin_unlock_irqrestore(&xencons_lock, flags);
}

void cprintk(int tohyp, const char *fmt, va_list args)
{
	static char   buf[1024];
	unsigned long flags;

	flags = 0;
	
	if (!tohyp) 
	{
		spin_lock_irqsave(&xencons_lock, flags);
	}
	vsnprintf(buf, sizeof(buf), fmt, args);

	if (tohyp) 
	{
		(void)HYPERVISOR_console_io(CONSOLEIO_write, strlen(buf), buf);
	} 
	else 
	{
		console_print(buf, strlen(buf));
	}

	if(!tohyp) 
	{
		spin_unlock_irqrestore(&xencons_lock, flags);
	}
}

void printk(const char *fmt, ...)
{
	va_list       args;
	va_start(args, fmt);
	cprintk(0, fmt, args);
	va_end(args);
}

void xprintk(const char *fmt, ...)
{
	va_list       args;
	va_start(args, fmt);
	cprintk(1, fmt, args);
	va_end(args);
}

int readbytes(char *buf, unsigned len) {
	unsigned long flags;
	int result;
	spin_lock_irqsave(&xencons_lock, flags);

	if (xencons_ring_avail()) 
	{
		result = xencons_ring_recv(buf, len);
		spin_unlock_irqrestore(&xencons_lock, flags);
	} 
	else 
	{
		spin_unlock_irqrestore(&xencons_lock, flags);
		DEFINE_WAIT(w);
		add_waiter(w, console_queue);
		while (1) 
		{
			schedule();
			spin_lock_irqsave(&xencons_lock, flags);
			result = xencons_ring_recv(buf, len);
			spin_unlock_irqrestore(&xencons_lock, flags);
			if (result) break;
			block(current);
		}
		remove_waiter(w);
	}
	return result;
}

void init_console(void)
{
	xencons_ring_init();
	console_initialised = 1;
	xencons_ring_send_fn = xencons_ring_send;
}
