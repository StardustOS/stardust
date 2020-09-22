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

#ifndef _BLK_FRONT_EXTRA_H_
#define _BLK_FRONT_EXTRA_H_

#include <os/config.h>

#ifdef BLKFRONT

extern int blk_has_initialised(void);
extern int blk_get_sectors(int device_id);
extern int blk_get_sector_size(int device_id);
extern int blk_write(int device, unsigned long address, void *buf, int size);
extern int blk_read(int device, unsigned long address, void *buf, int size);

#define SECTOR_BITS 9
#define SECTOR_SIZE (1<<SECTOR_BITS)
#define SECTORS_PER_PAGE (PAGE_SIZE/SECTOR_SIZE)
#define addr_to_sec(addr) (addr/SECTOR_SIZE)
#define sec_to_addr(sec) (sec * SECTOR_SIZE)

#endif

#endif
