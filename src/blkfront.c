/* Copyright (C) 2016, Ward Jaradat
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
 * Block device front end
 *
 * Author: Harald Roeck
 */

#include <os/config.h>

#ifdef BLKFRONT

#include <os/sched.h>
#include <os/blkfront-extra.h>
#include <os/blkfront.h>
#include <os/completion.h>
#include <os/events.h>
#include <os/gnttab.h>
#include <os/mutexes.h>
#include <os/kernel.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <os/time.h>
#include <os/xenbus.h>
#include <os/xmalloc.h>
#include <public/io/blkif.h>
#include <public/io/xenbus.h>
#include <errno.h>
#include <unistd.h>

#define MAX_PATH 64
#define MAX_DEVICES 1

#define DEVICE_STRING "device/vbd"

struct device_info
{
  int sector_size;
  int sectors;
  int info;
  int id;
};

struct blk_dev
{
  int ring_ref;
  int16_t blk_id;
  int16_t state;
  evtchn_port_t evtchn;
  unsigned int local_port;
  char *backend;
  struct blkif_front_ring ring;
  struct device_info device;
  spinlock_t lock;
};

#define ST_UNKNOWN 0

#define ST_READY 1

int blk_initialised;

static struct blk_dev blk_devices[MAX_DEVICES];

#define BLK_RING_SIZE __RING_SIZE((struct blkif_sring *)0, PAGE_SIZE)

struct blk_shadow
{
  grant_ref_t gref[MAX_PAGES_PER_REQUEST];
  short num_refs;
  short free;
  struct blk_request *request;
};

static DEFINE_SPINLOCK(freelist_lock);

static unsigned short freelist[BLK_RING_SIZE];

static struct blk_shadow shadows[BLK_RING_SIZE];

static DECLARE_COMPLETION(ready_completion);

static inline unsigned int get_freelist_id(void)
{
  unsigned int id;
  int flags;
  spin_lock_irqsave(&freelist_lock, flags);
  id = freelist[0];
  freelist[0] = freelist[id];
  shadows[id].free = 0;
  spin_unlock_irqrestore(&freelist_lock, flags);
  return id;
}

static inline void add_id_freelist(unsigned int id)
{
  int flags;
  spin_lock_irqsave(&freelist_lock, flags);
  freelist[id] = freelist[0];
  freelist[0] = id;
  shadows[id].free = 1;
  spin_unlock_irqrestore(&freelist_lock, flags);
}

static void init_buffers(void)
{
  int i;
  for (i = 0; i < BLK_RING_SIZE; ++i)
  {
    add_id_freelist(i);
  }
}

static inline void complete_request(struct blk_shadow *shadow)
{
  int i;
  for (i = 0; i < shadow->num_refs; ++i)
  {
    gnttab_end_access(shadow->gref[i]);
  }
}

static void blk_front_handler(evtchn_port_t port, struct blk_dev *dev)
{
  struct blk_request *io_req;
  struct blk_shadow *shadow;
  struct blkif_response *response;
  RING_IDX cons, prod;

again:
  prod = dev->ring.sring->rsp_prod;
  rmb();
  for (cons = dev->ring.rsp_cons; cons != prod; cons++)
  {
    response = RING_GET_RESPONSE(&dev->ring, cons);
    shadow = &shadows[response->id];
    BUG_ON(shadow->free);
    complete_request(shadow);
    io_req = shadow->request;
    BUG_ON(io_req->state != BLK_SUBMITTED);
    add_id_freelist(response->id);
    io_req->state = response->status == BLKIF_RSP_OKAY ? BLK_DONE_SUCCESS : BLK_DONE_ERROR;
    if (io_req->callback)
    {
      io_req->callback(io_req);
    }
    else
    {
      printk("blk_front WARNING: no callback\n");
    }
  }

  dev->ring.rsp_cons = cons;
  if (cons != dev->ring.req_prod_pvt)
  {
    int more_to_do;
    RING_FINAL_CHECK_FOR_RESPONSES(&dev->ring, more_to_do);
    if (more_to_do)
      goto again;
  }
  else
  {
    dev->ring.sring->rsp_event = cons + 1;
  }
}

static void __blk_front_handler(evtchn_port_t port, void *data)
{
  struct blk_dev *dev = (struct blk_dev *)data;
  spin_lock(&dev->lock);
  if (dev->state == ST_READY)
  {
    blk_front_handler(port, dev);
  }
  spin_unlock(&dev->lock);
}

static int blk_init_ring(struct blkif_sring *ring, struct blk_dev *dev)
{
  if (ring == NULL)
  {
    ring = (struct blkif_sring *)alloc_page();
  }

  if (!ring)
  {
    return 1;
  }

  memset(ring, 0, PAGE_SIZE);
  SHARED_RING_INIT(ring);
  FRONT_RING_INIT(&dev->ring, ring, PAGE_SIZE);
  dev->ring_ref = gnttab_grant_access(0, virt_to_mfn(ring), 0);
  return 0;
}

static int blk_get_evtchn(struct blk_dev *dev)
{
  evtchn_alloc_unbound_t op;
  op.dom = DOMID_SELF;
  op.remote_dom = 0;
  if (HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &op))
  {
    return 1;
  }

  clear_evtchn(op.port);
  dev->local_port = bind_evtchn(op.port, ANY_CPU, __blk_front_handler, dev);
  dev->evtchn = op.port;
  return 0;
}

void blk_rebind_evtchn(int cpu, struct blk_dev *dev)
{
  evtchn_bind_to_cpu(dev->evtchn, cpu);
}

static int blk_explore(char ***dirs)
{
  char *msg;
  int i;
  msg = xenbus_ls(XBT_NIL, DEVICE_STRING, dirs);

  if (msg)
  {
    xfree(msg);
    *dirs = NULL;
    return -1;
  }

  for (i = 0; (*dirs)[i]; i++)
    ;

  return i;
}

static int blk_inform_back(char *path, struct blk_dev *dev)
{
  int retry = 0;
  char *err;
  xenbus_transaction_t xbt;

again:
  err = xenbus_transaction_start(&xbt);
  if (err)
  {
    xfree(err);
    printk("%s ERROR: transaction_start\n", __FUNCTION__);
    return EAGAIN;
  }

  err = xenbus_printf(xbt, path, "ring-ref", "%u", dev->ring_ref);

  if (err)
  {
    printk("%s ERROR: printf ring ref\n", __FUNCTION__);
    goto abort;
  }

  err = xenbus_printf(xbt, path, "event-channel", "%u", dev->evtchn);

  if (err)
  {
    printk("%s ERROR: printf path\n", __FUNCTION__);
    goto abort;
  }

  err = xenbus_printf(xbt, path, "state", "%u", XenbusStateInitialised);

  if (err)
  {
    printk("%s ERROR: printf state\n", __FUNCTION__);
    goto abort;
  }

  err = xenbus_transaction_end(xbt, 0, &retry);

  if (retry)
  {
    goto again;
  }
  else if (err)
  {
    xfree(err);
  }

  return 0;

abort:

  if (err)
  {
    xfree(err);
  }

  err = xenbus_transaction_end(xbt, 1, &retry);

  if (err)
  {
    xfree(err);
  }

  return 1;
}

static int blk_connected(char *path)
{
  int retry = 0;
  char *err;
  xenbus_transaction_t xbt;

again:
  err = xenbus_transaction_start(&xbt);
  if (err)
  {
    printk("%s ERROR: transaction_start\n", __FUNCTION__);
    return EAGAIN;
  }

  err = xenbus_printf(xbt, path, "state", "%u", XenbusStateConnected);

  if (err)
  {
    xfree(err);
    goto abort;
  }

  err = xenbus_transaction_end(xbt, 0, &retry);

  if (retry)
  {
    printk("%s ERROR: transaction end\n", __FUNCTION__);
    goto again;
  }
  else if (err)
  {
    xfree(err);
  }
  return 0;

abort:
  err = xenbus_transaction_end(xbt, 1, &retry);
  if (err)
  {
    xfree(err);
  }
  return 1;
}

static inline void wait_for_init(void)
{
  wait_for_completion(&ready_completion);
}

extern int blk_get_sectors(int device_id)
{
  if (device_id >= MAX_DEVICES || blk_devices[device_id].state != ST_READY)
  {
    return 0;
  }
  return blk_devices[device_id].device.sectors;
}

static inline long wait_for_device_ready(struct blk_dev *dev)
{
  long flags;
  spin_lock_irqsave(&dev->lock, flags);
  while (dev->state != ST_READY)
  {
    spin_unlock_irqrestore(&dev->lock, flags);
    wait_for_completion(&ready_completion);
    spin_lock_irqsave(&dev->lock, flags);
  }
  return flags;
}

int blk_do_io(struct blk_request *io_req)
{
  struct blkif_request *xen_req;
  struct blk_shadow *shadow;
  int notify;
  int id;
  int err = 0;
  RING_IDX prod;
  struct blk_dev *dev;
  BUG_ON(io_req->state != BLK_EMPTY);
  BUG_ON(io_req->device >= MAX_DEVICES);
  dev = &blk_devices[io_req->device];
  BUG_ON(dev->state != ST_READY);
  BUG_ON((io_req->address >> SECTOR_BITS) + io_req->end_sector > dev->device.sectors);
  prod = dev->ring.req_prod_pvt;
  xen_req = RING_GET_REQUEST(&dev->ring, prod);
  id = get_freelist_id();
  if (!id)
  {
    err = ENOMEM;
    goto out;
  }
  shadow = &shadows[id];
  shadow->request = io_req;
  xen_req->id = id;
  xen_req->seg[0].gref = gnttab_grant_access(0, virt_to_mfn(io_req->pages[0]), 0);
  shadow->gref[0] = xen_req->seg[0].gref;
  xen_req->seg[0].first_sect = io_req->start_sector;
  if (io_req->num_pages == 1)
    xen_req->seg[0].last_sect = io_req->end_sector;
  else
  {
    int i;
    xen_req->seg[0].last_sect = SECTORS_PER_PAGE - 1;
    for (i = 1; i < io_req->num_pages; ++i)
    {
      xen_req->seg[i].gref =
          gnttab_grant_access(0, virt_to_mfn(io_req->pages[i]), 0);
      ;
      shadow->gref[i] = xen_req->seg[i].gref;
      xen_req->seg[i].first_sect = 0;
      xen_req->seg[i].last_sect = SECTORS_PER_PAGE - 1;
    }
    xen_req->seg[i].last_sect = io_req->end_sector;
  }

  shadow->num_refs = io_req->num_pages;
  xen_req->nr_segments = io_req->num_pages;
  xen_req->sector_number = addr_to_sec(io_req->address);
  xen_req->handle = 0x12;
  xen_req->operation = io_req->operation;
  io_req->state = BLK_SUBMITTED;
  notify = 0;
  dev->ring.req_prod_pvt = prod + 1;
  wmb();
  RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&dev->ring, notify);
  if (notify)
  {
    notify_remote_via_evtchn(dev->evtchn);
  }

out:
  return err;
}

static void complete_callback(struct blk_request *req)
{
  struct completion *comp = (struct completion *)req->callback_data;
  complete(comp);
}

static inline void set_default_callback(struct blk_request *req, struct completion *comp)
{
  req->callback = complete_callback;
  req->callback_data = (unsigned long)comp;
}

int blk_write(int device, unsigned long address, void *buf, int size)
{
  uint8_t *pages;
  int sectors;
  int free_buf, order;
  struct blk_request req;
  struct completion comp;
  int i;
  BUG_ON(address & (SECTOR_SIZE - 1));
  BUG_ON(size & (SECTOR_SIZE - 1));
  sectors = size >> SECTOR_BITS;
  if (((unsigned long)buf & (PAGE_SIZE - 1)) || ((unsigned long)size & (SECTOR_SIZE - 1)))
  {
    order = get_order(size);
    pages = (unsigned char *)alloc_pages(order);
    memcpy(pages, buf, size);
    free_buf = 1;
  }
  else
  {
    pages = buf;
    free_buf = order = 0;
  }
  i = 0;
  while (size > PAGE_SIZE)
  {
    req.pages[i] = pages + i * PAGE_SIZE;
    size -= PAGE_SIZE;
    ++i;
  }
  req.pages[i] = pages + i * PAGE_SIZE;
  req.num_pages = i + 1;
  req.start_sector = 0;
  req.end_sector = (size >> SECTOR_BITS) - 1;
  req.device = device;
  req.address = address;
  req.state = BLK_EMPTY;
  req.operation = BLK_REQ_WRITE;
  long flags = wait_for_device_ready(&blk_devices[device]);
  init_completion(&comp);
  set_default_callback(&req, &comp);
  if (blk_do_io(&req))
  {
    sectors = -1;
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
  }
  else
  {
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
    wait_for_completion(&comp);
  }

  if (free_buf)
  {
    free_pages(pages, order);
  }

  if (req.state == BLK_DONE_SUCCESS)
  {
    return sectors;
  }
  else
  {
    return -1;
  }
}

int blk_read(int device, unsigned long address, void *buf, int size)
{
  uint8_t *pages;
  int sectors;
  int free_buf, order;
  struct blk_request req;
  struct completion comp;
  int i;
  BUG_ON(address & (SECTOR_SIZE - 1));
  BUG_ON(size & (SECTOR_SIZE - 1));
  sectors = size >> SECTOR_BITS;
  if (((unsigned long)buf & (PAGE_SIZE - 1)) | ((unsigned long)size & (SECTOR_SIZE - 1)))
  {
    order = get_order(size);
    pages = (unsigned char *)alloc_pages(order);
    free_buf = 1;
  }
  else
  {
    pages = buf;
    free_buf = order = 0;
  }
  i = 0;
  while (size > PAGE_SIZE)
  {
    req.pages[i] = pages + i * PAGE_SIZE;
    size -= PAGE_SIZE;
    ++i;
  }
  req.pages[i] = pages + i * PAGE_SIZE;
  req.num_pages = i + 1;
  req.start_sector = 0;
  req.end_sector = (size >> SECTOR_BITS) - 1;
  req.device = device;
  req.address = address;
  req.state = BLK_EMPTY;
  req.operation = BLK_REQ_READ;
  long flags = wait_for_device_ready(&blk_devices[device]);
  init_completion(&comp);
  set_default_callback(&req, &comp);
  if (blk_do_io(&req))
  {
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
    return -1;
  }
  else
  {
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
    wait_for_completion(&comp);
  }

  if (free_buf)
  {
    memcpy(buf, pages, size);
    free_pages(pages, order);
  }
  if (req.state == BLK_DONE_SUCCESS)
  {
    return sectors;
  }
  else
  {
    return -1;
  }
}

int read_block(int device, long address, int size)
{
  void *buf;
  buf = (unsigned char *)alloc_pages(size);
  uint8_t *pages;
  int sectors;
  int free_buf, order;
  struct blk_request req;
  struct completion comp;
  int i;
  BUG_ON(address & (SECTOR_SIZE - 1));
  BUG_ON(size & (SECTOR_SIZE - 1));
  sectors = size >> SECTOR_BITS;
  pages = buf;
  free_buf = order = 0;
  i = 0;
  while (size > PAGE_SIZE)
  {
    req.pages[i] = pages + i * PAGE_SIZE;
    size -= PAGE_SIZE;
    ++i;
  }

  req.pages[i] = pages + i * PAGE_SIZE;
  req.num_pages = i + 1;
  req.start_sector = 0;
  req.end_sector = (size >> SECTOR_BITS) - 1;
  req.device = device;
  req.address = address;
  req.state = BLK_EMPTY;
  req.operation = BLK_REQ_READ;

  long flags = wait_for_device_ready(&blk_devices[device]);
  init_completion(&comp);
  set_default_callback(&req, &comp);

  if (blk_do_io(&req))
  {
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
    return -1;
  }
  else
  {
    spin_unlock_irqrestore(&blk_devices[device].lock, flags);
    wait_for_completion(&comp);
  }

  if (free_buf)
  {
    memcpy(buf, pages, size);
    free_pages(pages, order);
  }

  if (req.state == BLK_DONE_SUCCESS)
  {
    printk("%s\n", buf);
    return sectors;
  }
  else
  {
    return -1;
  }
}

static int blk_shutdown(void)
{
  return 0;
}

static void post_connect(struct blk_dev *dev)
{
  char xenbus_path[MAX_PATH];
  int error;
  snprintf(xenbus_path, MAX_PATH, "%s/sector-size", dev->backend);
  dev->device.sector_size = xenbus_read_integer(xenbus_path);
  snprintf(xenbus_path, MAX_PATH, "%s/sectors", dev->backend);
  dev->device.sectors = xenbus_read_integer(xenbus_path);
  snprintf(xenbus_path, MAX_PATH, "%s/info", dev->backend);
  dev->device.info = xenbus_read_integer(xenbus_path);
  snprintf(xenbus_path, MAX_PATH, "%s/%d", DEVICE_STRING, dev->device.id);
  error = blk_connected(xenbus_path);
  if (error)
  {
    return;
  }

  long flags;
  spin_lock_irqsave(&dev->lock, flags);
  dev->state = ST_READY;
  spin_unlock_irqrestore(&dev->lock, flags);
}

static void blk_init(void)
{
  char **devices;
  int num_devices;
  int i;
  char xenbus_path[MAX_PATH];
  int error;
  char *err;
  xenbus_watch_path(XBT_NIL, DEVICE_STRING, "blk_xenbus");

again:
  num_devices = blk_explore(&devices);
  if (num_devices < 0)
  {
    char *msg;
    msg = xenbus_read_watch("blk_xenbus");
    xfree(msg);
    complete_all(&ready_completion);
    goto again;
  }

  xenbus_rm_watch("blk_xenbus");

  init_buffers();

  for (i = 0; i < num_devices && i < MAX_DEVICES; ++i)
  {
    blk_devices[i].device.id = (int)simple_strtol(devices[i], NULL, 10);
    snprintf(xenbus_path, MAX_PATH, "%s/%s/backend", DEVICE_STRING, devices[i]);
    err = xenbus_read(XBT_NIL, xenbus_path, &blk_devices[i].backend);
    if (err)
    {
      printk("%s %d ERROR reading from xenbus: %s\n", __FILE__, __LINE__, err);
      xfree(err);
      continue;
    }

    snprintf(xenbus_path, MAX_PATH, "%s/%s", DEVICE_STRING, devices[i]);

    if (blk_init_ring(NULL, &blk_devices[i]))
    {
      continue;
    }

    if (blk_get_evtchn(&blk_devices[i]))
    {
      continue;
    }

    error = blk_inform_back(xenbus_path, &blk_devices[i]);

    if (error)
    {
      continue;
    }

    snprintf(xenbus_path, MAX_PATH, "%s/state", blk_devices[i].backend);
    xenbus_watch_path(XBT_NIL, xenbus_path, "blk-front");
    xenbus_wait_for_value("blk-front", xenbus_path, "4");
    xenbus_rm_watch("blk-front");
    post_connect(&blk_devices[i]);
  }

  for (i = 0; i < num_devices; ++i)
    xfree(devices[i]);

  xfree(devices);
  complete_all(&ready_completion);
  return;
}

#define MAX_POLL 12

int write_to_console(char *ptr, int len);

int write_to_console(char *ptr, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    printk("%c", ptr[i]);
  }
  return len;
}

void blk_read_sectors(int device_id)
{
  int sectors;
  int sector_size;
  sectors = blk_get_sectors(device_id);
  sector_size = blk_devices[device_id].device.sector_size;
  long i;

  for (i = 0; i < sectors; ++i)
  {
    void *buf;
    buf = (unsigned char *)alloc_pages(sector_size);
    int res;
    res = blk_read(device_id, sec_to_addr(i), buf, sector_size);
    if (res > 0)
    {
      if (buf != NULL)
      {
        int len;
        len = sector_size;
        printk("sector %d, address %x: ", i, sec_to_addr(i));
        write_to_console(buf, len);
        printk("\n");
      }
      else
      {
        printk("sector %d, address %x: buffer is null\n", i, sec_to_addr(i));
      }
    }
    else
    {
      printk("error reading sector %d\n", i);
    }
  }
}

extern int blk_get_sector_size(int device_id)
{
  return blk_devices[device_id].device.sector_size;
}

extern int blk_has_initialised(void)
{
  return blk_initialised;
}

static void blkfront_thread(void *p)
{
  int num_devices;
  blk_init();
  wait_for_init();
  for (num_devices = 0; num_devices < MAX_DEVICES; ++num_devices)
  {
    if (blk_devices[num_devices].state != ST_READY)
      break;
  }

  if (num_devices > 0)
  {
    int i;
    for (i = 0; i < num_devices; ++i)
    {
      int sectors = blk_get_sectors(i);
      printk("block device \t: %d (id), %d (n. sectors), %d (sector size)\n", i, sectors, blk_devices[i].device.sector_size);
    }
  }
  blk_initialised = 1;
}

static int start_blk_front(void *arg)
{
  create_thread("blkfront_thread", blkfront_thread, UKERNEL_FLAG, arg);
  return 0;
}

void init_block_front()
{
  blk_initialised = 0;
  start_blk_front(NULL);
}

USED static int init_func(void)
{
  int i;
  memset(&blk_devices, 0, sizeof(blk_devices));
  for (i = 0; i < MAX_DEVICES; ++i)
  {
    spin_lock_init(&blk_devices[i].lock);
    blk_devices[i].state = ST_UNKNOWN;
  }
  init_completion(&ready_completion);
  return 0;
}

DECLARE_INIT(init_func);

#endif
