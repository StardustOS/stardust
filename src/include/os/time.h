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
 * 
 * Notes:
 * 
 * Code ported and adapted from Xen/Mini-OS
 */

/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: time.h
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *              Robert Kaiser (kaiser@informatik.fh-wiesbaden.de)
 *              
 *        Date: Jul 2003, changes: Jun 2005, Sep 2006
 * 
 * Environment: Guest VM microkernel evolved from Xen Minimal OS
 * Description: Time and timer functions
 *
 ****************************************************************************
 */

#ifndef _TIME_H_
#define _TIME_H_

#include <os/types.h>

struct shadow_time_info {
	u64 tsc_timestamp; 
	u64 system_timestamp;  
	u32 tsc_to_nsec_mul;
	u32 tsc_to_usec_mul;
	int tsc_shift;
	u32 version;
};

typedef s64 s_time_t;
extern s64 time_addend;

#define NOW()                   ((s_time_t)monotonic_clock() + time_addend)
#define SECONDS(_s)             (((s_time_t)(_s))  * 1000000000UL )
#define TENTHS(_ts)             (((s_time_t)(_ts)) * 100000000UL )
#define HUNDREDTHS(_hs)         (((s_time_t)(_hs)) * 10000000UL )
#define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MICROSECS(_us)          (((s_time_t)(_us)) * 1000UL )
#define Time_Max                ((s_time_t) 0x7fffffffffffffffLL)
#define FOREVER                 Time_Max
#define MSEC_TO_USEC(_ms)		((s_time_t)(_ms) * 1000UL )
#define NSEC_TO_USEC(_nsec)     ((_nsec) / 1000UL)
#define NSEC_TO_SEC(_nsec)      ((_nsec) / 1000000000ULL)

#ifndef _STRUCT_TIMEVAL
#define _STRUCT_TIMEVAL
typedef long time_t;
typedef long suseconds_t;
struct timeval {
	time_t		tv_sec;
	suseconds_t	tv_usec;
};
#endif

struct timespec {
	time_t      tv_sec;
    long        tv_nsec;
};

void     init_time(void);
u64      monotonic_clock(void);
int     gettimeofday(struct timeval *tv);
void     block_domain(s_time_t until);
void     check_need_resched(void);
u64      get_cpu_running_time(int cpu);
void     set_timer_interrupt(u64 delta);

#define get_running_time() get_cpu_running_time(smp_processor_id())

#endif /* _TIME_H_ */
