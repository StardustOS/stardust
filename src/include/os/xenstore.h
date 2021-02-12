#ifndef _XENSTORE_H_
#define _XENSTORE_H_

#include <os/types.h>
#include <public/xen.h>
#include <public/io/xs_wire.h>
#include <public/features.h>
#include <os/kernel.h>

int xenstore_init(start_info_t * start);
int xenstore_write(char * key, char * value);
int xenstore_read(char * key, char * value, int value_length);
int xenstore_read_domain_id();
#endif