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

/*
 ****************************************************************************
 * (C) 2006 - Cambridge University
 ****************************************************************************
 *
 *        File: xenbus.c
 *      Author: Steven Smith (sos22@cam.ac.uk)
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *     Changes: John D. Ramsdell
 *
 *        Date: Jun 2006, chages Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: Minimal implementation of xenbus
 *
 ****************************************************************************
 **/

#include <os/kernel.h>
#include <os/mm.h>
#include <os/traps.h>
#include <os/xenbus.h>
#include <os/events.h>
#include <os/sched.h>
#include <os/sched.h>
#include <os/wait.h>
#include <os/xmalloc.h>
#include <os/smp.h>
#include <os/completion.h>
#include <os/spinlock.h>
#include <os/lib.h>
#include <os/crash.h>
#include <errno.h>
#include <public/io/xs_wire.h>

#define min(x,y) ({                       	 	\
		typeof(x) tmpx = (x);                 	\
		typeof(y) tmpy = (y);                 	\
		tmpx < tmpy ? tmpx : tmpy;            	\
})


#define DEBUG(_f, _a...)    ((void)0)

static struct xenstore_domain_interface *xenstore_buf;
static DEFINE_SPINLOCK(xb_lock);
static DECLARE_WAIT_QUEUE_HEAD(xb_waitq);
static int suspend = 0;
static DECLARE_COMPLETION(suspend_comp);
static DECLARE_COMPLETION(resume_comp);

static struct thread *xenbus_thread;

struct xenbus_req_info
{
	int in_use;
	struct wait_queue_head waitq;
	void *reply;
};

#define NR_REQS 32
static struct xenbus_req_info req_info[NR_REQS];
static LIST_HEAD(watch_list);
static DEFINE_SPINLOCK(watch_list_lock);

struct xenbus_watch
{
	char *token;
	char *path;
#define MAX_PATHS   10
	int path_prod_idx;
	int path_cons_idx;
	char *paths[MAX_PATHS];
	struct thread *thread;
	struct list_head list;
};

static struct xenbus_watch* find_watch(char *token)
{
	struct list_head *list_element;
	struct xenbus_watch *watch;

	list_for_each(list_element, &watch_list)
	{
		watch = list_entry(list_element, struct xenbus_watch, list);
		if(strcmp(watch->token, token) == 0) 
		{
			return watch;
		}
	}
	return NULL;
}

static void memcpy_from_ring(const void *Ring, void *Dest, int off, int len)
{
	int c1, c2;
	const char *ring = Ring;
	char *dest = Dest;
	c1 = min(len, XENSTORE_RING_SIZE - off);
	c2 = len - c1;
	memcpy(dest, ring + off, c1);
	memcpy(dest + c1, ring, c2);
}

static void xenbus_thread_func(void *ign)
{
	struct xsd_sockmsg msg;
	unsigned prod = 0;

	for (;;)
	{
		wait_event(xb_waitq, (prod != xenstore_buf->rsp_prod) || suspend);
		if(suspend) 
		{
			complete_all(&suspend_comp);
			wait_for_completion(&resume_comp);
			BUG_ON(suspend);
		}
		while (1) 
		{
			long flags;
			spin_lock_irqsave(&xb_lock, flags);
			prod = xenstore_buf->rsp_prod;
			if (xenstore_buf->rsp_prod - xenstore_buf->rsp_cons < sizeof(msg)) {
				spin_unlock_irqrestore(&xb_lock, flags);
				break;
			}
			rmb();
			memcpy_from_ring(xenstore_buf->rsp,
					&msg,
					MASK_XENSTORE_IDX(xenstore_buf->rsp_cons),
					sizeof(msg));
			if (xenstore_buf->rsp_prod - xenstore_buf->rsp_cons <
					sizeof(msg) + msg.len) {
				spin_unlock_irqrestore(&xb_lock, flags);
				break;
			}

			if(msg.type == XS_WATCH_EVENT) {
				char* payload = (char*)xmalloc_align(sizeof(msg) + msg.len, 4);
				char *path,*token;
				struct xenbus_watch *watch;

				memcpy_from_ring(xenstore_buf->rsp,
						payload,
						MASK_XENSTORE_IDX(xenstore_buf->rsp_cons),
						msg.len + sizeof(msg));

				path = payload + sizeof(msg);
				token = path + strlen(path) + 1;
				DEBUG("watch event %s %s\n", path, token);
				spin_lock(&watch_list_lock);
				watch = find_watch(token);
				if(watch == NULL)
				{
					printk("Spurious watch event for token: %s\n", token);
					goto free_watch_msg;
				}
				BUG_ON(watch->path_prod_idx - watch->path_cons_idx > MAX_PATHS);
				watch->paths[watch->path_prod_idx++ % MAX_PATHS] = strdup(path);
				if(watch->thread) 
				{
					wake(watch->thread);
				}

				free_watch_msg:
				spin_unlock(&watch_list_lock);
				xenstore_buf->rsp_cons += msg.len + sizeof(msg);
				xfree(payload);
			} 
			else 
			{
				DEBUG("NO watch event, msg id %d in use %d\n", msg.req_id, req_info[msg.req_id].in_use);
				BUG_ON(!req_info[msg.req_id].in_use);
				req_info[msg.req_id].reply = xmalloc_align(sizeof(msg) + msg.len, 4);
				memcpy_from_ring(xenstore_buf->rsp, req_info[msg.req_id].reply, MASK_XENSTORE_IDX(xenstore_buf->rsp_cons), msg.len + sizeof(msg));
				xenstore_buf->rsp_cons += msg.len + sizeof(msg);
				wake_up(&req_info[msg.req_id].waitq);
			}
			spin_unlock_irqrestore(&xb_lock, flags);
		}
	}
}

static void xenbus_evtchn_handler(evtchn_port_t port, void *ign)
{
	wake_up(&xb_waitq);
}

static int nr_live_reqs;
static spinlock_t req_lock = SPIN_LOCK_UNLOCKED;
static DECLARE_WAIT_QUEUE_HEAD(req_wq);

static void release_xenbus_id(int id)
{
	BUG_ON(!req_info[id].in_use);
	spin_lock(&req_lock);
	nr_live_reqs--;
	req_info[id].in_use = 0;
	if (nr_live_reqs == NR_REQS - 1 || nr_live_reqs == 0)
	{
		wake_up(&req_wq);
	}
	spin_unlock(&req_lock);
}

static int allocate_xenbus_id(void)
{
	static int probe;
	int o_probe;

	while (1)
	{
		spin_lock(&req_lock);
		if (nr_live_reqs < NR_REQS)
			break;
		spin_unlock(&req_lock);
		wait_event(req_wq, (nr_live_reqs < NR_REQS) && !suspend);
	}

	o_probe = probe;
	for (;;)
	{
		if (!req_info[o_probe].in_use)
			break;
		o_probe = (o_probe + 1) % NR_REQS;
		BUG_ON(o_probe == probe);
	}
	nr_live_reqs++;
	req_info[o_probe].in_use = 1;
	probe = (o_probe + 1) % NR_REQS;
	spin_unlock(&req_lock);
	init_waitqueue_head(&req_info[o_probe].waitq);

	return o_probe;
}

char* xenbus_printf(xenbus_transaction_t xbt, char* node, char* path, char* fmt, ...)
{
	#define BUFFER_SIZE 256
	char fullpath[BUFFER_SIZE];
	char val[BUFFER_SIZE];
	va_list args;

	BUG_ON(strlen(node) + strlen(path) + 1 >= BUFFER_SIZE);
	sprintf(fullpath,"%s/%s", node, path);
	va_start(args, fmt);
	vsprintf(val, fmt, args);
	va_end(args);
	return xenbus_write(xbt,fullpath,val);
}

extern char xenstore_page[PAGE_SIZE];
static struct xenstore_domain_interface *map_xenstore_page(xen_pfn_t mfn)
{
	if (HYPERVISOR_update_va_mapping((unsigned long)xenstore_page, __pte( PFN_PHYS(mfn) | 7), UVMF_INVLPG))
	{
		printk("Failed to map xenstore_page!!\n");
		crash();
	}
	return (struct xenstore_domain_interface *)xenstore_page;
}

struct write_req 
{
	const void *data;
	unsigned len;
};

static void xb_write(int type, int req_id, xenbus_transaction_t trans_id, const struct write_req *req, int nr_reqs)
{
	XENSTORE_RING_IDX prod;
	int r;
	int len = 0;
	const struct write_req *cur_req;
	int req_off;
	int total_off;
	int this_chunk;
	struct xsd_sockmsg m = {.type = type, .req_id = req_id,
			.tx_id = trans_id };
	struct write_req header_req = { &m, sizeof(m) };

	for (r = 0; r < nr_reqs; r++)
		len += req[r].len;
	m.len = len;
	len += sizeof(m);

	cur_req = &header_req;

	BUG_ON(len > XENSTORE_RING_SIZE);
	long flags;
	spin_lock_irqsave(&xb_lock, flags);
	prod = xenstore_buf->req_prod;
	if (prod + len - xenstore_buf->req_cons > XENSTORE_RING_SIZE)
	{
		spin_unlock_irqrestore(&xb_lock, flags);
		wait_event(xb_waitq, xenstore_buf->req_prod + len - xenstore_buf->req_cons <= XENSTORE_RING_SIZE);
		spin_lock_irqsave(&xb_lock, flags);
		prod = xenstore_buf->req_prod;
	}

	/* We're now guaranteed to be able to send the message without
       overflowing the ring.  Do so. */
	total_off = 0;
	req_off = 0;
	while (total_off < len)
	{
		this_chunk = min(cur_req->len - req_off,
				XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod));
		memcpy((char *)xenstore_buf->req + MASK_XENSTORE_IDX(prod),
				(char *)cur_req->data + req_off, this_chunk);
		prod += this_chunk;
		req_off += this_chunk;
		total_off += this_chunk;
		if (req_off == cur_req->len)
		{
			req_off = 0;
			if (cur_req == &header_req)
				cur_req = req;
			else
				cur_req++;
		}
	}

	BUG_ON(req_off != 0);
	BUG_ON(total_off != len);
	BUG_ON(prod > xenstore_buf->req_cons + XENSTORE_RING_SIZE);
	wmb();
	xenstore_buf->req_prod += len;
	spin_unlock_irqrestore(&xb_lock, flags);
	notify_remote_via_evtchn(start_info.store_evtchn);
}

static struct xsd_sockmsg * xenbus_msg_reply(int type, xenbus_transaction_t trans, struct write_req *io, int nr_reqs)
{
	int id;
	DEFINE_WAIT(w);
	struct xsd_sockmsg *rep;
	id = allocate_xenbus_id();
	preempt_disable();
	add_waiter(w, req_info[id].waitq);
	xb_write(type, id, trans, io, nr_reqs);
	preempt_enable();
	schedule();
	rep = req_info[id].reply;
	BUG_ON(rep->req_id != id);
	release_xenbus_id(id);
	return rep;
}

static char *errmsg(struct xsd_sockmsg *rep)
{
	if (!rep)
	{
		return strdup("no reply");
	}

	if (rep->type != XS_ERROR)
	{
		return NULL;
	}

	char *res = xmalloc_align(rep->len + 1, 4);
	memcpy(res, rep + 1, rep->len);
	res[rep->len] = 0;
	xfree(rep);
	return res;
}

char *xenbus_ls(xenbus_transaction_t xbt, const char *pre, char ***contents)
{
	struct xsd_sockmsg *reply, *repmsg;
	struct write_req req[] = { { pre, strlen(pre)+1 } };
	int nr_elems, x, i;
	char **res;

	repmsg = xenbus_msg_reply(XS_DIRECTORY, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(repmsg);
	if (msg) 
	{
		*contents = NULL;
		return msg;
	}
	reply = repmsg + 1;
	for (x = nr_elems = 0; x < repmsg->len; x++)
		nr_elems += (((char *)reply)[x] == 0);
	res = xmalloc_align(sizeof(res[0]) * (nr_elems + 1), 4);
	for (x = i = 0; i < nr_elems; i++) {
		int l = strlen((char *)reply + x);
		res[i] = xmalloc_align(l + 1, 4);
		memcpy(res[i], (char *)reply + x, l + 1);
		x += l + 1;
	}
	res[i] = NULL;
	xfree(repmsg);
	*contents = res;
	return NULL;
}

char *xenbus_read(xenbus_transaction_t xbt, const char *path, char **value)
{
	struct write_req req[] = { {path, strlen(path) + 1} };
	struct xsd_sockmsg *rep;
	char *res;
	rep = xenbus_msg_reply(XS_READ, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(rep);
	if (msg) 
	{
		*value = NULL;
		return msg;
	}
	res = xmalloc_align(rep->len + 1, 4);
	memcpy(res, rep + 1, rep->len);
	res[rep->len] = 0;
	xfree(rep);
	*value = res;
	return NULL;
}

char *xenbus_write(xenbus_transaction_t xbt, const char *path, const char *value)
{
	struct write_req req[] = {
			{path, strlen(path) + 1},
			{value, strlen(value) + 1},
	};
	struct xsd_sockmsg *rep;
	rep = xenbus_msg_reply(XS_WRITE, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(rep);
	if (msg) return msg;
	xfree(rep);
	return NULL;
}

char* xenbus_watch_path(xenbus_transaction_t xbt, char *path, char *token)
{
	struct xsd_sockmsg *rep;
	struct xenbus_watch *watch;
	char *msg;
	struct write_req req[] = {
			{path,  strlen(path)  + 1},
			{token, strlen(token) + 1},
	};

	spin_lock(&watch_list_lock);
	BUG_ON(find_watch(token) != NULL);
	watch = xmalloc(struct xenbus_watch);
	watch->path = path;
	watch->token = token;
	watch->thread = NULL;
	watch->path_prod_idx = 0;
	watch->path_cons_idx = 0;
	INIT_LIST_HEAD(&watch->list);
	list_add(&watch->list, &watch_list);
	spin_unlock(&watch_list_lock);

	rep = xenbus_msg_reply(XS_WATCH, xbt, req, ARRAY_SIZE(req));
	msg = errmsg(rep);
	if(msg != NULL)
	{
		spin_lock(&watch_list_lock);
		xfree(rep);
		watch = find_watch(token);
		BUG_ON(watch == NULL);
		list_del(&watch->list);
		xfree(watch);
		spin_unlock(&watch_list_lock);
	}

	return msg;
}

int xenbus_rm_watch(char *token)
{
	struct xenbus_watch *watch;
	spin_lock(&watch_list_lock);
	watch = find_watch(token);
	if (watch) 
	{
		struct write_req req[] = {
				{watch->path, strlen(watch->path) + 1},
				{watch->token, strlen(watch->token) + 1},
		};
		spin_unlock(&watch_list_lock);
		xenbus_msg_reply(XS_UNWATCH, XBT_NIL, req, ARRAY_SIZE(req));
		spin_lock(&watch_list_lock);
		list_del(&watch->list);
		xfree(watch);
	}
	spin_unlock(&watch_list_lock);
	return 1;
}

char *xenbus_read_watch(char *token)
{
	struct xenbus_watch *watch;
	char *path;

	spin_lock(&watch_list_lock);
	watch = find_watch(token);
	again:
	if(watch->path_cons_idx < watch->path_prod_idx)
	{
		path = watch->paths[watch->path_cons_idx++ % MAX_PATHS];
		watch->thread = NULL;
		spin_unlock(&watch_list_lock);
		return path;
	}

	watch->thread = current;
	block(current);
	spin_unlock(&watch_list_lock);
	schedule();
	spin_lock(&watch_list_lock);
	goto again;
}

char* xenbus_wait_for_value(char* token, char *path, char* value)
{
	for(;;)
	{
		char *changed_path, *msg, *res;

		changed_path = xenbus_read_watch(token);
		if(strcmp(changed_path, path) == 0)
		{
			xfree(changed_path);
			msg = xenbus_read(XBT_NIL, path, &res);
			if(msg) return msg;
			if(strcmp(value, res) == 0)
			{
				xfree(res);
				return NULL;
			}
			else
				xfree(res);
		}
		else
			xfree(changed_path);
	}
	BUG();
	return NULL;
}


char *xenbus_rm(xenbus_transaction_t xbt, const char *path)
{
	struct write_req req[] = { {path, strlen(path) + 1} };
	struct xsd_sockmsg *rep;
	rep = xenbus_msg_reply(XS_RM, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(rep);
	if (msg)
		return msg;
	xfree(rep);
	return NULL;
}

char *xenbus_get_perms(xenbus_transaction_t xbt, const char *path, char **value)
{
	struct write_req req[] = { {path, strlen(path) + 1} };
	struct xsd_sockmsg *rep;
	char *res;
	rep = xenbus_msg_reply(XS_GET_PERMS, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(rep);
	if (msg) {
		*value = NULL;
		return msg;
	}
	res = xmalloc_align(rep->len + 1, 4);
	memcpy(res, rep + 1, rep->len);
	res[rep->len] = 0;
	xfree(rep);
	*value = res;
	return NULL;
}

#define PERM_MAX_SIZE 32
char *xenbus_set_perms(xenbus_transaction_t xbt, const char *path, domid_t dom, char perm)
{
	char value[PERM_MAX_SIZE];
	snprintf(value, PERM_MAX_SIZE, "%c%hu", perm, dom);
	struct write_req req[] = {
			{path, strlen(path) + 1},
			{value, strlen(value) + 1},
	};
	struct xsd_sockmsg *rep;
	rep = xenbus_msg_reply(XS_SET_PERMS, xbt, req, ARRAY_SIZE(req));
	char *msg = errmsg(rep);
	if (msg)
		return msg;
	xfree(rep);
	return NULL;
}

char *xenbus_transaction_start(xenbus_transaction_t *xbt)
{
	struct write_req req = {"", 1};
	struct xsd_sockmsg *rep;
	char *err;

	rep = xenbus_msg_reply(XS_TRANSACTION_START, 0, &req, 1);
	err = errmsg(rep);
	if (err) {
		printk("Xenbus error, %s\n", err);
		return err;
	}

	sscanf((char *)(rep + 1), "%u", xbt);
	xfree(rep);
	return NULL;
}

char *
xenbus_transaction_end(xenbus_transaction_t t, int abort, int *retry)
{
	struct xsd_sockmsg *rep;
	struct write_req req;
	char *err;

	*retry = 0;

	req.data = abort ? "F" : "T";
	req.len = 2;
	rep = xenbus_msg_reply(XS_TRANSACTION_END, t, &req, 1);
	err = errmsg(rep);
	if (err) 
	{
		if (!strcmp(err, "EAGAIN")) 
		{
			*retry = 1;
			xfree(err);
			return NULL;
		} 
		else 
		{
			return err;
		}
	}
	xfree(rep);
	return NULL;
}

int xenbus_read_integer(char *path)
{
	char *res, *buf;
	int t;
	res = xenbus_read(XBT_NIL, path, &buf);
	if (res) 
	{
		DEBUG("%s %d ERROR reading xenbus: %s\n", __FILE__, __LINE__, res);
		xfree(res);
		return -1;
	}
	sscanf(buf, "%d", &t);
	xfree(buf);
	return t;
}

void init_xenbus(void)
{

	DEBUG("init_xenbus called.\n");
	suspend = 0;
	xenstore_buf = map_xenstore_page(start_info.store_mfn);
	xenbus_thread = create_thread("xenstore", xenbus_thread_func, UKERNEL_FLAG, NULL);
	DEBUG("buf at %p.\n", xenstore_buf);
	bind_evtchn(start_info.store_evtchn, ANY_CPU, xenbus_evtchn_handler, NULL);
}

domid_t xenbus_get_self_id(void)
{
	char *dom_id;
	domid_t ret;
	BUG_ON(xenbus_read(XBT_NIL, "domid", &dom_id));
	sscanf(dom_id, "%d", &ret);
	return ret;
}
