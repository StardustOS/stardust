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

#ifndef _POWEROFF_H_
#define _POWEROFF_H_

static inline void **get_bp(void)
{
    void **bp;
    asm ("movq %%rbp, %0" : "=r"(bp));
    return bp;
}

static inline void **get_sp(void)
{
    void **sp;
    asm ("movq %%rsp, %0" : "=r"(sp));
    return sp;
}

extern void poweroff(void);
extern void backtrace_and_poweroff(void);

#endif
