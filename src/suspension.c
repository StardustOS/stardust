/* Copyright (C) 2017, Ward Jaradat and Jon Lewis
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

#include <os/suspension.h>
#include <pte/pte_generic_osal.h>
#include <os/trace.h>

void suspend_pthread(pte_thread_t *pte_thread_pntr, int attempting_to_suspend_self) 
{
    tprintk("Called with pntr: %p, suspend_self: %i \n", pte_thread_pntr, attempting_to_suspend_self);
    struct thread * thread = pte_thread_pntr->os_thread_pntr;
    
    if (thread == NULL) 
    {
        tprintk("Do nothing, non-existent %i thread \n", 0);
        return;
    }
    
    if (pte_thread_pntr->state == PThreadStateLast) 
    {
        tprintk("Do nothing, non-existent pthread %i\n", 0);
        return;
    }
    
    if (is_dying(thread)) 
    {
        tprintk("Do nothing, called on dying thread %i\n", thread->id);
        return;
    }
    
    if (is_suspended(thread)) 
    {
        tprintk("do nothing, called on already suspended thread %i\n", thread->id);
        return;
    }

    tprintk("suspend_pthread: suspending thread %i\n", thread->id);
    suspend_thread(thread);

    if (attempting_to_suspend_self) 
    {
        /* 
        Synchronous behaviour expected when suspending self, must call sched ourselves so as to be suspended 
        prior to returning from the call and must also release mutex prior to to calling schedule, otherwise 
        we would have deadlock.
        */
        pte_osMutexUnlock (pte_thread_reuse_lock);
        schedule();
    }
    
    /* 
    Ayschrounous behaviour expected when suspending a different thread for this case we can let a regular call
    to schedule take care of suspension 
    */
}

void resume_pthread(pte_thread_t *pte_thread_pntr) 
{
    struct thread * thread = pte_thread_pntr->os_thread_pntr;

    if (thread == NULL) 
    {
        tprintk("Do nothing, non-existent %i thread \n", 0);
        return;
    }

    if (pte_thread_pntr->state == PThreadStateLast) 
    {
        tprintk("Do nothing, non-existent pthread %i\n", 0);
        return;
    }

    if (is_dying(thread)) 
    { 
        tprintk("Do nothing, called on dying thread %i\n", thread->id);
        return;
    }

    if (!is_suspended(thread)) 
    { 
        tprintk("Do nothing, called on thread %i that is not suspended\n", thread->id);
        return;
    }

    wake_suspended_thread(thread);

    /* Waking a thread cannot be done by self, so we expect asynchronous behaviour here */
}

