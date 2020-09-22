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

#ifndef SRC_INCLUDE_PTE_OSAL_H_
#define SRC_INCLUDE_PTE_OSAL_H_

#include <os/config.h>

#ifdef ENABLE_PTE

#include <os/types.h>
#include <os/tls.h>
#include <os/mutexes.h>
#include <os/semaphores.h>
#include <pte/pthreads-table.h>
#include <os/sched.h>
#include <os/function.h>
#include <stdlib.h>

typedef uint16_t osThreadHandle_t;

typedef osThreadHandle_t pte_osThreadHandle;
typedef semaphore_t * pte_osSemaphoreHandle;
typedef mutex_t * pte_osMutexHandle;

#define OS_MAX_SIMUL_THREADS 10

tls_pointer getPteTls();

#endif

#endif
