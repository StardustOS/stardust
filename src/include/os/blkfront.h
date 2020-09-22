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
 * 
 * Notes: 
 * 
 * Code ported and adapted from Xen/MiniOS
 */

/*
 * Block device front end
 *
 * Author: Harald Roeck
 */

#ifndef _BLK_FRONT_H_
#define _BLK_FRONT_H_

#include <os/config.h>

#ifdef BLKFRONT

#include <public/io/blkif.h>
#include <os/blkfront-extra.h>

void init_block_front();

struct blk_request;

typedef void (*blk_callback)(struct blk_request*);

#define MAX_PAGES_PER_REQUEST  BLKIF_MAX_SEGMENTS_PER_REQUEST

struct blk_request {
    void *pages[MAX_PAGES_PER_REQUEST]; 
    int num_pages;
    int start_sector;
    int end_sector;
    int device;
    unsigned long address;

    enum 
    {
        BLK_EMPTY = 1,
        BLK_SUBMITTED,
        BLK_DONE_SUCCESS,
        BLK_DONE_ERROR,
    } state;

    enum 
    {
	    BLK_REQ_READ = BLKIF_OP_READ,
	    BLK_REQ_WRITE = BLKIF_OP_WRITE,
    } operation;

    blk_callback callback;
    unsigned long callback_data;
};

extern int blk_do_io(struct blk_request *req);

#endif

#endif