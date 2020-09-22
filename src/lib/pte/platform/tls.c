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

#include <pte/pte_osal.h>
#include <pte/pte_generic_osal.h>

pthreads_table_pointer pthreads_table;

tls_pointer pte_tls;

tls_pointer
getPteTls()
{
	return pte_tls;
}

pte_osResult
pte_osTlsInit()
{
	pte_osResult result;

	pte_tls = tls_init();
	if (pte_tls == NULL)
	{
		result = PTE_OS_NO_RESOURCES;
		return result;
	}

	result = PTE_OS_OK;
	return result;
}

pte_osResult
pte_osTlsAlloc(unsigned * pKey)
{

	pte_osResult result;
	struct thread * thread = sched_current_thread();
	uint16_t thread_id = thread->id;

	if (is_pthread(thread))
    {
        tls_pointer pTls = thread->tls;
        mutex_lock(pTls->lock);
        unsigned count = tls_count(pTls->root);
        unsigned nKey = count + 1;
        *pKey = nKey;
        if (tls_put(pTls, *pKey, 0) == 1)
        {
            // Creating the key in the thread's TLS has been successful. However, this key must also be created in PTE's TLS
            tls_pointer pteTls = getPteTls();
            mutex_lock(pteTls->lock);
            tls_put(pteTls, *pKey, 0);
            mutex_unlock(pteTls->lock);

            result = PTE_OS_OK;
            mutex_unlock(pTls->lock);
            return result;
        }
        else
        {
            result = PTE_OS_NO_RESOURCES;
            mutex_unlock(pTls->lock);
            return result;
        }
    }

    tls_pointer pteTls = getPteTls();
    mutex_lock(pteTls->lock);
    unsigned count = tls_count(pteTls->root);
    unsigned nKey = count + 1;
    *pKey = nKey;
    if (tls_put(pteTls, *pKey, 0) == 1)
    {
        result = PTE_OS_OK;
        mutex_unlock(pteTls->lock);
        return result;
    }
    else
    {
        result = PTE_OS_NO_RESOURCES;
        mutex_unlock(pteTls->lock);
        return result;
    }
}

pte_osResult
pte_osTlsSetValue(unsigned key, void * value)
{

	pte_osResult result;
	struct thread * thread = sched_current_thread();

	if (is_pthread(thread))
	{
		tls_pointer pTls = thread->tls;
		mutex_lock(pTls->lock);
		if (tls_set(pTls, key, value) == 1)
		{
			result = PTE_OS_OK;
			mutex_unlock(pTls->lock);
			return result;
		}
		else
		{
		    // Couldn't locate the key in the thread's TLS. It is therefore necessary to create it and set its value accordingly
            tls_put(pTls, key, value);
			mutex_unlock(pTls->lock);
		}
	}

	tls_pointer pteTls = getPteTls();
	mutex_lock(pteTls->lock);
	if (tls_set(pteTls, key, value) == 1)
	{
		result = PTE_OS_OK;
	}
	else
	{
		result = PTE_OS_NO_RESOURCES;
	}
	mutex_unlock(pteTls->lock);
	return result;
}

void *
pte_osTlsGetValue(unsigned index)
{
	struct thread * thread = sched_current_thread();

	if (is_pthread(thread))
	{
		tls_pointer  pTls = thread->tls;
		if (tls_has_key(pTls, index) == 1)
		{
			void * entry;
			mutex_lock(pTls->lock);
			entry = tls_get(pTls, index);
			mutex_unlock(pTls->lock);
			return entry;
		} else {
		    // Couldn't find the key to the thread's TLS. Hence, it is necessary to create it and set its value to NULL
            tls_put(pTls, index, NULL);
            return NULL;
		}
	}

	tls_pointer pteTls = getPteTls();
	if (tls_has_key(pteTls, index) == 1)
	{
		void * entry;
		mutex_lock(pteTls->lock);
		entry = tls_get(pteTls, index);
		mutex_unlock(pteTls->lock);
		return entry;
	}

	return NULL;
}

pte_osResult
pte_osTlsFree(unsigned index)
{
	// todo
	return PTE_OS_OK;
}

#endif


