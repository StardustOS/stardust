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

#ifdef ENABLE_PTE

#include <os/atomic.h>
#include <pte/pte_osal.h>
#include <pte/pte_generic_osal.h>
#include <pte/implement.h>
#include <pte/pthread.h>
#include <os/trace.h>

#define MAX_SKIPS 1

pte_osResult pte_osInit(void)
{
	return pte_osTlsInit();
}

pte_osResult pte_osMutexCreate(pte_osMutexHandle * pHandle)
{
	*pHandle = mutex_create();
	return PTE_OS_OK;
}

pte_osResult pte_osMutexDelete(pte_osMutexHandle handle)
{
	mutex_delete(handle);
	return PTE_OS_OK;
}

pte_osResult pte_osMutexLock(pte_osMutexHandle handle)
{
	mutex_lock(handle);
	return PTE_OS_OK;
}

pte_osResult pte_osMutexTimedLock(pte_osMutexHandle handle, unsigned int timeoutMsecs)
{
	if (timeoutMsecs == 0)
	{
		if (mutex_try_lock(handle) == 0)
		{
			return PTE_OS_TIMEOUT;
		}
	}
	else
	{
		if(mutex_timed_lock(handle, timeoutMsecs) == 0)
		{
			return PTE_OS_TIMEOUT;
		}
	}
	return PTE_OS_OK;
}

pte_osResult pte_osMutexUnlock(pte_osMutexHandle handle)
{
	mutex_unlock(handle);
	return PTE_OS_OK;
}

pte_osResult pte_osSemaphoreCreate(int initialValue, pte_osSemaphoreHandle *pHandle)
{
	*pHandle = semaphore_create(initialValue);
	if (*pHandle == NULL)
	{
		return PTE_OS_NO_RESOURCES;
	}
	return PTE_OS_OK;
}

pte_osResult pte_osSemaphoreDelete(pte_osSemaphoreHandle handle)
{
	semaphore_delete(handle);
	return PTE_OS_OK;
}

pte_osResult pte_osSemaphorePend(pte_osSemaphoreHandle handle, unsigned int *pTimeout)
{

	s_time_t past_time;
	s_time_t current_time;
	s_time_t timeout;

	timeout = 0;
	if ((pTimeout != NULL) || (pTimeout != 0))
		timeout =  (s_time_t) *pTimeout * 1000000;

	past_time = NOW();

	while (1) {

		current_time = NOW();
		s_time_t timespent;
		timespent = current_time - past_time;

		if ( pTimeout == NULL ) {
			semaphore_pend(handle);
			return PTE_OS_OK;
		}
		if (semaphore_try_pend(handle) == 1) return PTE_OS_OK;
		else if ( (pTimeout == 0) || (timespent >=  timeout ) ) return PTE_OS_TIMEOUT;
	}
}

pte_osResult pte_osSemaphorePost(pte_osSemaphoreHandle handle, int count)
{
	int i;
	for (i=0;i<count;i++)
	{
        semaphore_post(handle);
	}
	return PTE_OS_OK;
}

pte_osResult pte_osThreadCancel(pte_thread_t *pte_thread_pntr)
{
    struct thread * t = pte_thread_pntr->os_thread_pntr;
    if (t != NULL ) {
        if (is_joining(t)) { // if thread is currently blocked - waiting to join another thread
            sched_wake_joiner(t); // for deferred cancellation, need to wake target thread
        }
        set_cancelled(t);
    }
    return PTE_OS_OK;
}

pte_osResult pte_osThreadCheckCancel(pte_thread_t *pte_thread_pntr)
{
	struct thread * t = pte_thread_pntr->os_thread_pntr;
	if (t != NULL && pte_thread_pntr->state != PThreadStateLast)
	{
		if (is_pthread(t))
		{
			if (is_cancelled(t))
			{
				return PTE_OS_INTERRUPTED;
			}
			else
			{
				return PTE_OS_OK;
			}
		}
	}
	return PTE_OS_OK;
}

int pte_osAtomicCompareExchange(int *pdest, int exchange, int comp) 
{
	return atomic_compare_exchange(pdest, comp, exchange);
}

unsigned long pte_osAtomicCompareExchange_x86_64(unsigned long *pdest, unsigned long exchange, unsigned long comp)
{
	return atomic_compare_exchange_x86_64(pdest, comp, exchange);
}

int pte_osAtomicDecrement(int *target) 
{
		return atomic_decrement(target);
}

int pte_osAtomicExchange(int * ptarg, int val)
{
		return atomic_exchange(ptarg, val);
}

unsigned long pte_osAtomicExchange_x86_64(unsigned long *ptarg, unsigned long val) 
{
	return atomic_exchange_x86_64(ptarg, val);
}

int pte_osAtomicExchangeAdd(int volatile* pAddend, int value)
{
	return atomic_exchange_add(pAddend, value);
}

unsigned long pte_osAtomicExchangeAdd_x86_64(unsigned long volatile* pAddend, unsigned long value)
{
	return atomic_exchange_add_x86_64(pAddend, value);
}

int pte_osAtomicIncrement(int *pdest)
{
	return atomic_increment(pdest);
}

pte_osResult pte_osThreadCreate(function entryPoint, int stackSize, int initialPriority, void *argv, pte_thread_t *pte_thread_pntr)
{
	pte_osResult result;
	if (stackSize < STACK_SIZE)
	{
		stackSize = STACK_SIZE;
	}
	struct thread * pthread = create_thread("pthread", entryPoint, PTHREAD_FLAG, argv);
	if (pthread == NULL)
	{
		return PTE_OS_NO_RESOURCES;
	}
	pte_thread_pntr->os_thread_pntr = pthread;
	pthread->tls = tls_init();
	pte_thread_pntr->priority = initialPriority;
	unsigned key = 1;
	void * value = NULL;
	if (tls_alloc(pthread->tls, key, value) == 0)
	{
		return PTE_OS_NO_RESOURCES;
	}
	pte_thread_pntr->threadId = (uint16_t) pthread->id;
	return PTE_OS_OK;
}

pte_osResult pte_osThreadDelete(pte_thread_t *pte_thread_pntr)
{
	return PTE_OS_OK;
}

void pte_osThreadExit()
{
	exit_current_thread();
}

pte_osResult pte_osThreadExitAndDelete(pte_thread_t *pte_thread_pntr)
{
	struct thread *thread = pte_thread_pntr->os_thread_pntr;
	pte_osThreadDelete(pte_thread_pntr);
	if (thread != NULL)
	{
        exit_thread(thread);
	}
	return PTE_OS_OK;
}

int pte_osThreadGetDefaultPriority()
{
	return (PTE_MIN_PRIORITY + PTE_MAX_PRIORITY)/2;
}

pte_osThreadHandle pte_osThreadGetHandle(void)
{
	uint16_t threadHandle = sched_current_thread()->id;
	return threadHandle;
}

int pte_osThreadGetMaxPriority()
{
	return PTE_MAX_PRIORITY;
}


int pte_osThreadGetMinPriority()
{
	return PTE_MIN_PRIORITY;
}

int pte_osThreadGetPriority(pte_thread_t *pte_thread_pntr)
{
    struct thread * thread = pte_thread_pntr->os_thread_pntr;
    if (thread != NULL && pte_thread_pntr->state != PThreadStateLast)
    {
        if (is_pthread(thread))
        {
            return pte_thread_pntr->priority;
        }
        else
        {
            return pteThreadPriority;
        }
    }
    return -1;
}

pte_osResult pte_osThreadSetPriority(pte_thread_t *pte_thread_pntr, int newPriority)
{

	struct thread * thread = pte_thread_pntr->os_thread_pntr;

	if (thread != NULL && pte_thread_pntr->state != PThreadStateLast)
	{
		if (is_pthread(thread))
		{
			pte_thread_pntr->priority = newPriority;
		}
		else
		{
            pteThreadPriority = newPriority;
		}
	}
	return PTE_OS_OK;
}

void pte_osThreadSleep(unsigned int msecs)
{
	sleep(msecs);
}

pte_osResult pte_osThreadStart(pte_thread_t *pte_thread_pntr)
{
    struct thread * thread_to_start = pte_thread_pntr->os_thread_pntr;
	if (thread_to_start != NULL)
	{
		start_thread(thread_to_start);
		schedule();
		return PTE_OS_OK;
	}
	return PTE_OS_GENERAL_FAILURE;
}

/**
 * This function blocks until the specified thread exits.
 * Note that according to documentation, this call should be cancellable – that is, it should return (even if the target thread has not exited) if OsThreadCancel is called.
 * @param threadHandle the 16-bit ID of the thread we are waiting for
 * @return the pte_osResult returned is PTE_OS_OK once the target thread has exited or PTE_OS_INTERRUPTED if the calling thread is cancelled prior to the target exiting
 *
 */
pte_osResult pte_osThreadWaitForEnd(volatile pte_thread_t *pte_thread_pntr)
{
    static int instance;
    instance++;

    struct thread *current_thread = sched_current_thread();

    // Jon caution, we don't have mutex protection here, using atomic add as atomic read instead
    struct thread *target_thread = (struct thread *) pte_osAtomicExchangeAdd_x86_64(((unsigned long *)&(pte_thread_pntr->os_thread_pntr)), 0);
//    int orig_thread_id = pte_osAtomicExchangeAdd(((int *)&(pte_thread_pntr->threadId)), 0);
//    int orig_thread_reuse_count = pte_osAtomicExchangeAdd(((int *)&(pte_thread_pntr->ptHandle.x)), 0);

    int join_return_value = 0;
    if (target_thread == NULL) {
//        printk("1 waitForEnd %i: target null    tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_OK;
    }

    if (is_dying(target_thread)) {
//        printk("1 waitForEnd %i: target dying   tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_OK;
    }

    if (pte_thread_pntr->state == PThreadStateLast) {
//        printk("1 waitForEnd %i: target exited  tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_OK;
    }

    if ((current_thread != NULL) && is_cancelled(current_thread)) { // Jon and Ward, need lock/mutex protection (or atomic read) for is_cancelled
//        printk("1 waitForEnd %i: join interrupt tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_INTERRUPTED;
    }

    // Otherwise, block until target has exited or joiner has been cancelled
    join_return_value = join_thread(target_thread);

    if (join_return_value == -1) { // there was no such process at the OS level
//        printk("2 waitForEnd %i: no OS level thread for tp: os-%p, tid: %i, pte-id: %i, reuse: %i\n", instance, target_thread, target_thread->id, orig_thread_id, orig_thread_reuse_count);
        return join_return_value;
    }

    if (is_dying(target_thread)) {
//        printk("2 waitForEnd %i: target dying   tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_OK;
    }

    if (pte_thread_pntr->state == PThreadStateLast) {
//        printk("2 waitForEnd %i: target exited  tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_OK;
    }

    if ((current_thread != NULL) && is_cancelled(current_thread)) { // Jon and Ward, need lock/mutex protection (or atomic read) for is_cancelled
//        printk("2 waitForEnd %i: join interrupt tp: pte-%p os-%p, tid: %i, reuse: %i\n", instance, pte_thread_pntr, target_thread, orig_thread_id, orig_thread_reuse_count);
        return PTE_OS_INTERRUPTED;
    }

    // Otherwise we woke the target thread by mistake and have a BUG
//    printk("2 waitForEnd %i: woken by mistake -- tp: os-%p, tid: %i, pte-id: %i, reuse: %i\n", instance, target_thread, target_thread->id, orig_thread_id, orig_thread_reuse_count);
    BUG();

    // unreachable code due to BUG statement above, but put in to avoid compiler warning.
    return PTE_OS_OK;
}

pte_osResult
pte_osSemaphoreCancellablePend(pte_osSemaphoreHandle semHandle, unsigned int *pTimeout)
{

	s_time_t past_time;
	s_time_t current_time;
	s_time_t last_current_time = 0;
	s_time_t timeout;
	int time_skipped_backwards_count = 0;
	unsigned int last_cpu, current_cpu;

	if (pTimeout == NULL) {
		semaphore_pend(semHandle);
		return PTE_OS_OK;
	}

	struct thread * t = sched_current_thread();

	timeout = 0;
	if ((pTimeout != NULL))
		timeout =  (s_time_t) *pTimeout * 1000000;

#ifdef ENABLE_DEBUG_TRACE
	pte_osThreadHandle self_id = sched_current_thread()->id;
	if (timeout < 0) {
		tprintk("pte_osSemaphoreCancellablePend: thread %u - timeout is %ld.\n", self_id, timeout);
	}
#endif
#ifdef DEBUG
	assert(timeout >= 0);
#endif
	past_time = NOW();
#ifdef DEBUG
	last_cpu = sched_current_thread()->cpu;
#endif

	while (1) {
#ifdef DEBUG
		current_cpu = sched_current_thread()->cpu;
#endif
		current_time = NOW();
		s_time_t timespent;
		timespent = current_time - past_time;
#ifdef DEBUG
		if (current_time - last_current_time < 0) {
			time_skipped_backwards_count++;
			tprintk("pte_osSemaphoreCancellablePend: skip %i, thread %u - current_time=%ld, last_current_time=%ld, difference=%ld, timeout=%ld, last_cpu=%i, current_cpu=%i.\n", time_skipped_backwards_count, self_id, current_time, last_current_time, current_time - last_current_time, timeout, last_cpu, current_cpu);
		}
		assert(timespent >= 0);
		assert(current_time >= 0);
		assert(time_skipped_backwards_count < MAX_SKIPS);
#endif
		if (semaphore_try_pend(semHandle) == 1 ) return PTE_OS_OK;
		else if ( (t != NULL) && is_cancelled(t)) return PTE_OS_INTERRUPTED;
		else if ( (timeout == 0) || (timespent >= timeout) ) return PTE_OS_TIMEOUT;
#ifdef DEBUG
		last_current_time = current_time;
		last_cpu = current_cpu;
#endif

	}
}

#endif
