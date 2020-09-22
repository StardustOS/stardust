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

#include <os/config.h>

#include <os/kernel.h>
#include <os/hypervisor.h>
#include <os/mm.h>
#include <os/events.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/blkfront.h>
#include <os/xenbus.h>
#include <os/gnttab.h>
#include <os/types.h>
#include <os/lib.h>
#include <os/startup.h>
#include <os/suspension.h>
#include <os/crash.h>
#include <public/features.h>
#include <public/version.h>
#include <unistd.h>
#include <stdio.h>
#include <pte/pthread.h>

uint8_t xen_features[XENFEAT_NR_SUBMAPS * 32];

__attribute__((weak)) int app_main(struct app_main_args *aargs){
	create_thread("startup_thread", startup_thread, UKERNEL_FLAG, NULL);
	return 0;
}

static void run_main(struct app_main_args *args)
{
	app_main(args);
}

void start_kernel(start_info_t *si)
{
	struct app_main_args aargs;
	xen_feature_info_t fi;
	int i, r, k, inits;

	arch_init(si);
	init_initial_context();
	trap_init();
	
	printk("Initialising...\n");
	printf(" _____ _____ _____ _____ _____ __ __ _____ _____ \n");
	printf("|   __|_   _|  _  |  _  |     |  |  |   __|_   _|\n");
	printf("|__   | | | |     |    _| |   |     |__   | | |  \n");
	printf("|_____| |_| |__|__|__|__|_____|_____|_____| |_|  \n");
	printf("\n");						
	printk("start_info	: %p\n", si);
	printk("nr_pages	: %lu\n", si->nr_pages);
	printk("shared_inf	: %08lx\n", si->shared_info);
	printk("pt_base		: %p\n", (void *)si->pt_base);
	printk("mod_start	: 0x%lx\n", si->mod_start);
	printk("mod_len		: %lu\n", si->mod_len);
	printk("flags		: 0x%x\n", (unsigned int)si->flags);
	printk("cmd_line	: %s\n", si->cmd_line ? (const char *)si->cmd_line : "NULL");

	long num_phyical_pages = si->nr_pages;
	init_events();
	arch_print_info();

	for (r = 0; r < XENFEAT_NR_SUBMAPS; r++)
	{
		fi.submap_idx = r;
		if (HYPERVISOR_xen_version(XENVER_get_features, &fi) < 0)
			break;

		for (k=0; k<32; k++)
			xen_features[r*32+k] = !!(fi.submap & 1<<k);
	}

	init_mm((char *)si->cmd_line);
	init_time();
	init_console();
	init_gnttab();
	init_sched((char *)si->cmd_line);
	init_smp();
	init_xenbus();
	if (unmap_page(0)) 
	{
		crash_with_message("Failed mapping a virtual address.\n");
	}
	
	inits = __init_end - __init_start;

	for (i=0; i < inits; ++i) 
	{
		__init_start[i]();
	}

#ifdef BLKFRONT
	init_block_front();
#endif

#ifdef ENABLE_PTE
	pthread_init();
#endif

	aargs.cmd_line = (char *)si->cmd_line;
	aargs.si_info = si;
	run_main(&aargs);
	run_idle_thread();
}
