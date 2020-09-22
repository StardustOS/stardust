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

#ifndef XENBUS_H__
#define XENBUS_H__

#include <public/xen.h>

typedef unsigned long xenbus_transaction_t;
#define XBT_NIL ((xenbus_transaction_t)0)

void init_xenbus(void);
char *xenbus_read(xenbus_transaction_t xbt, const char *path, char **value);
char *xenbus_watch_path(xenbus_transaction_t xbt, char *path, char *token);
char* xenbus_wait_for_value(char* token, char *path, char* value);
char * xenbus_read_watch(char *token);
int xenbus_rm_watch(char *token);
char *xenbus_write(xenbus_transaction_t xbt, const char *path, const char *value);
char *xenbus_rm(xenbus_transaction_t xbt, const char *path);
char *xenbus_ls(xenbus_transaction_t xbt, const char *prefix, char ***contents);
char *xenbus_get_perms(xenbus_transaction_t xbt, const char *path, char **value);
char *xenbus_set_perms(xenbus_transaction_t xbt, const char *path, domid_t dom, char perm);
char *xenbus_transaction_start(xenbus_transaction_t *xbt);
char *xenbus_transaction_end(xenbus_transaction_t, int abort, int *retry);
int xenbus_read_integer(char *path);
char* xenbus_printf(xenbus_transaction_t xbt, char* node, char* path, char* fmt, ...);
domid_t xenbus_get_self_id(void);

#endif
