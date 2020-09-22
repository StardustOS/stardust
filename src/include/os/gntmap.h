/* Copyright (C) 2019, Ward Jaradat
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

#ifndef _GNTMAP_H_
#define _GNTMAP_H_

#include <os/kernel.h>

struct gntmap 
{
    int nentries;
    struct gntmap_entry *entries;
};

int gntmap_set_max_grants(struct gntmap *map, int count);
int gntmap_munmap(struct gntmap *map, unsigned long start_address, int count);
void* gntmap_map_grant_refs(struct gntmap *map, uint32_t count, uint32_t *domids, int domids_stride, uint32_t *refs, int writable);
void gntmap_init(struct gntmap *map);
void gntmap_fini(struct gntmap *map);

#endif
