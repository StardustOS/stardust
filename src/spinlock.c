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
 */


#include <os/spinlock.h>
#include <os/kernel.h>
#include <os/sched.h>
#include <os/xmalloc.h>

#define BUILD_LOCK_OPS(op, locktype)												\
void 																				\
os_##op##_lock(locktype##_t *lock)			                						\
{																					\
	for (;;) {																		\
		preempt_disable();															\
		if (likely(_raw_##op##_trylock(lock)))										\
			break;																	\
		preempt_enable();															\
																					\
		if (!(lock)->break_lock)													\
			(lock)->break_lock = 1;													\
		while (!op##_can_lock(lock) && (lock)->break_lock)							\
                {                                                  					\
                        (lock)->spin_count++;                       				\
						relax();												\
                }                                                       			\
	}																				\
	(lock)->break_lock = 0;															\
        (lock)->spin_count = 0;                                         			\
        (lock)->owner = current;                                      				\
        current->lock_count++;                                      				\
}																					\
																					\
																					\
unsigned long 																		\
os_##op##_lock_irqsave(locktype##_t *lock)											\
{																					\
	unsigned long flags;															\
																					\
	for (;;) {																		\
		preempt_disable();															\
		local_irq_save(flags);														\
		if (likely(_raw_##op##_trylock(lock)))										\
			break;																	\
		local_irq_restore(flags);													\
		preempt_enable();															\
																					\
		if (!(lock)->break_lock)													\
			(lock)->break_lock = 1;													\
		while (!op##_can_lock(lock) && (lock)->break_lock)							\
                {                                                       			\
                        (lock)->spin_count++;	 	                  				\
						relax();												\
                }                                                       			\
	}																				\
	(lock)->break_lock = 0;															\
        (lock)->spin_count = 0;                                         			\
        (lock)->owner = current;                                      				\
        current->lock_count++;                                          			\
	return flags;																	\
}																					\

BUILD_LOCK_OPS(spin, spinlock);

void
os_spin_unlock(spinlock_t *lock)
{
	BUG_ON(lock->owner != current);
    current->lock_count--;
    lock->owner = NULL;
    _raw_spin_unlock(lock);
#ifdef DEBUG_LOCKS
    if (current->lock_count < 0)
    {
    	printk("spinlock count is negative\n");
    	BUG();
    }
#endif
    preempt_enable();
}

void
os_spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	BUG_ON(lock->owner != current);
	current->lock_count--;
	lock->owner = NULL;
	_raw_spin_unlock(lock);
	local_irq_restore(flags);
	#ifdef DEBUG_LOCKS
	if (current->lock_count < 0)
	{
		printk("spinlock count is negative\n");
		BUG();
    }
	#endif
	preempt_enable();
}

spinlock_t
*create_spin_lock(void)
{
	struct spinlock *lock = (struct spinlock *)xmalloc(struct spinlock);
	spin_lock_init(lock);
	return lock;
}

void
delete_spin_lock(spinlock_t *lock)
{
    xfree(lock);
}
