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

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#define DEBUG_LOCKS

typedef struct spinlock {
	volatile unsigned int slock;
#if defined(CONFIG_PREEMPT) && defined(CONFIG_SMP)
	unsigned int break_lock;
        unsigned long spin_count;
        struct thread *owner;
#endif
} spinlock_t;


#define ARCH_SPIN_LOCK_UNLOCKED (spinlock_t) { 1 }

#define arch_spin_is_locked(x)	(*(volatile signed char *)(&(x)->slock) <= 0)
#define arch_spin_can_lock(lock) (!arch_spin_is_locked(lock)) 

#define cpu_relax_string    "rep;nop"
static inline void relax(void)
{
	__asm__ __volatile__(cpu_relax_string: : :"memory");
}

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

#define spin_lock_string \
        "1:\n" \
	LOCK \
	"decb %0\n\t" \
	"jns 3f\n" \
	"2:\t" \
    cpu_relax_string \
	"cmpb $0,%0\n\t" \
	"jle 2b\n\t" \
	"jmp 1b\n" \
	"3:\n\t"

#define spin_unlock_string \
	"xchgb %b0, %1" \
		:"=q" (oldval), "=m" (lock->slock) \
		:"0" (oldval) : "memory"

static inline void _raw_spin_unlock(spinlock_t *lock)
{
	char oldval = 1;
	__asm__ __volatile__(
		spin_unlock_string
	);
}

static inline int _raw_spin_trylock(spinlock_t *lock)
{
	char oldval;
	__asm__ __volatile__(
		"lock xchgb %b0,%1\n"
		:"=q" (oldval), "=m" (lock->slock)
		:"0" (0) : "memory");
	return oldval > 0;
}

static inline void _raw_spin_lock(spinlock_t *lock)
{
	__asm__ __volatile__(
		spin_lock_string
		:"=m" (lock->slock) : : "memory");
}

#define SPIN_LOCK_UNLOCKED ARCH_SPIN_LOCK_UNLOCKED

#define spin_lock_init(x)	do { *(x) = SPIN_LOCK_UNLOCKED; } while(0)

#define spin_is_locked(x)	arch_spin_is_locked(x)
#define spin_can_lock(x)	arch_spin_can_lock(x)

extern spinlock_t *create_spin_lock(void);
extern void delete_spin_lock(spinlock_t *lock);
extern void os_spin_lock(spinlock_t *lock);
extern void os_spin_unlock(spinlock_t *lock);
extern unsigned long os_spin_lock_irqsave(spinlock_t *lock);
extern void os_spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

#define spin_lock(lock)     os_spin_lock(lock)
#define spin_unlock(lock)   os_spin_unlock(lock)

#define spin_lock_irqsave(lock, flags)  flags = os_spin_lock_irqsave(lock)
#define spin_unlock_irqrestore(lock, flags) os_spin_unlock_irqrestore(lock, flags)

#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED

#endif
