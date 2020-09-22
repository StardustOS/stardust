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

/* Some unnecessary code needs to be removed, and optimisation work is needed for this scheduler. */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <os/types.h>
#include <os/list.h>
#include <os/spinlock.h>
#include <os/traps.h>
#include <os/time.h>
#include <os/bug.h>
#include <os/function.h>
#include <os/mutexes.h>
#include <os/atomic.h>
#include <os/trace.h>
#include <os/tls.h>

struct fp_regs {
    unsigned long filler1; unsigned long filler2;
    unsigned long filler3; unsigned int mxcsr; unsigned int mxcsr_mask;
    unsigned long mm0; unsigned long reserved0;
    unsigned long mm1; unsigned long reserved1;
    unsigned long mm2; unsigned long reserved2;
    unsigned long mm3; unsigned long reserved3;
    unsigned long mm4; unsigned long reserved4;
    unsigned long mm5; unsigned long reserved5;
    unsigned long mm6; unsigned long reserved6;
    unsigned long mm7; unsigned long reserved7;
    unsigned long xmm0; unsigned long xmmh0;
    unsigned long xmm1; unsigned long xmmh1;
    unsigned long xmm2; unsigned long xmmh2;
    unsigned long xmm3; unsigned long xmmh3;
    unsigned long xmm4; unsigned long xmmh4;
    unsigned long xmm5; unsigned long xmmh5;
    unsigned long xmm6; unsigned long xmmh6;
    unsigned long xmm7; unsigned long xmmh7;
    unsigned long xmm8; unsigned long xmmh8;
    unsigned long xmm9; unsigned long xmmh9;
    unsigned long xmm10; unsigned long xmmh10;
    unsigned long xmm11; unsigned long xmmh11;
    unsigned long xmm12; unsigned long xmmh12;
    unsigned long xmm13; unsigned long xmmh13;
    unsigned long xmm14; unsigned long xmmh14;
    unsigned long xmm15; unsigned long xmmh15;
    unsigned long reserved8; unsigned long reserved9;
    unsigned long reserved10; unsigned long reserved11;
    unsigned long reserved12; unsigned long reserved13;
    unsigned long reserved14; unsigned long reserved15;
    unsigned long reserved16; unsigned long reserved17;
    unsigned long reserved18; unsigned long reserved19;
};


struct thread {
    int preempt_count;
    u32 flags;     
    struct pt_regs *regs;
    struct fp_regs *fpregs;
    uint16_t id;
    int16_t appsched_id;
    int16_t stack_allocated;
    char *name;
    char *stack;
    unsigned long stack_size;
    void *specific;
    u64 timeslice;
    u64 resched_running_time;
    u64 start_running_time;
    u64 cum_running_time;
    unsigned int cpu;
    int lock_count;
    unsigned long sp;
    unsigned long ip;
    struct list_head thread_list;
    struct list_head ready_list;
    struct list_head joiners;
    struct list_head aux_thread_list;
    void *db_data;
    unsigned long r14;
    struct tls * tls;
};

extern struct list_head thread_list;
extern spinlock_t thread_list_lock;

void idle_thread_fn(void *data);

#define RUNNABLE_FLAG           0x00000001     /* Thread can be run on a processor */
#define RUNNING_FLAG            0x00000002     /* Thread is currently running */
#define RESCHED_FLAG            0x00000004     /* Scheduler should be called at the first opportunity. WARN: flags used explicitly in x86_64.S, don't change */
#define DYING_FLAG              0x00000008     /* Thread scheduled to die */

#define CANCELLED_FLAG          0x00000100	   /* Thread cancelled */
#define	SUSPENDED_FLAG			0x00000200	   /* Thread suspended */
#define SLEEP_FLAG              0x00000400     /* Sleeping */
#define HIBERNATION_FLAG        0x00000800     /* Hibernating */

#define UKERNEL_FLAG            0x00001000     	/* Thread is a ukernel thread */
#define PTHREAD_FLAG            0x00002000     	/* Thread is a pthread */

#define JOINING_FLAG            0x00004000      /* Thread is currently blocked trying to join another */

#define DEFINE_THREAD_FLAG(flag_name, flag_set_prefix, funct_name)   \
static unsigned long inline flag_set_prefix##funct_name(             \
                                        struct thread *thread)       \
{                                                                    \
    return thread->flags & flag_name##_FLAG;                         \
}                                                                    \
                                                                     \
static void inline set_##funct_name(struct thread *thread)           \
{                                                                    \
    thread->flags |= flag_name##_FLAG;                               \
}                                                                    \
                                                                     \
static void inline clear_##funct_name(struct thread *thread)         \
{                                                                    \
    thread->flags &= ~flag_name##_FLAG;                              \
}

DEFINE_THREAD_FLAG(RUNNABLE, is_, runnable);
DEFINE_THREAD_FLAG(RUNNING, is_, running);
DEFINE_THREAD_FLAG(RESCHED, , need_resched);
DEFINE_THREAD_FLAG(DYING, is_, dying);
DEFINE_THREAD_FLAG(UKERNEL, is_, ukernel);
DEFINE_THREAD_FLAG(SLEEP, is_, sleeping);
DEFINE_THREAD_FLAG(HIBERNATION, is_, hibernating);
DEFINE_THREAD_FLAG(CANCELLED, is_, cancelled);
DEFINE_THREAD_FLAG(SUSPENDED, is_, suspended);
DEFINE_THREAD_FLAG(PTHREAD, is_, pthread);
DEFINE_THREAD_FLAG(JOINING, is_, joining);

#define switch_threads(prev, next, last) arch_switch_threads(prev, next, last)

struct thread* arch_create_thread(char *name, void (*function)(void *), void *stack, unsigned long stack_size, void *data);
struct thread* arch_create_pthread(char *name, function function, void *stack, unsigned long stack_size, void *data);

extern void init_sched(char *cmd_line);
extern void init_initial_context(void);
extern void run_idle_thread(void);

struct thread* create_thread(char *name, void (*function), int flags, void *data);
struct thread* create_thread_with_stack(char *name, void (*function)(void *), int ukernel, void *stack, unsigned long stack_size, void *data);
struct thread* create_idle_thread(unsigned int cpu);
struct thread* create_vm_thread(char *name, void (*function)(void *), void *stack, unsigned long stack_size, int priority, void *data);

void schedule(void);
int join_thread(struct thread *joinee);
int sched_wake_joiner(struct thread *joiner);
u32 get_flags(struct thread *thread);
struct thread *sched_get_thread(uint16_t);
void start_thread(struct thread *thread);
void wake(struct thread *thread);
void wake_suspended_thread(struct thread * thread);
void block(struct thread *thread);
int sleep(u32 millisecs);
int nanosleep(u64 nanosecs);
void sched_kick_processor(int cpu);
struct thread *current_not_idle(void);
void sched_print_ready_queue();
void sched_print_sleep_queue();
void sched_print_threads();
void sched_print_hibernating_threads();
struct thread *sched_current_thread(void);

#define PREEMPT_ACTIVE     0x10000000
#define IRQ_ACTIVE         0x00100000

void preempt_schedule(void);
static void inline add_preempt_count(struct thread *ti, int val)
{
	/*  
        Legacy code from MiniOS below is not atomic which could cause issues if preemption 
        count alteration were interrupted by eg. timer 
       
        ti->preempt_count += val;
    
        could replace above with inline atomic 'lock xadd' assembly code instead of doing 
        
        int *mem = &ti->preempt_count;
     
        and passing mem to atomic update, we can simply use ti as preempt count as it is 
        at 0-offset from ti
    */

	asm(	"movl %[val], %%eax\n\t"                 			// store double word val in A
			"movq %[ti], %%rbx\n\t"                   			// store quad word mem in B
			"lock xadd %%eax, (%%rbx)\n\t"             			// lock mem BUS & do swap-add value
			: 				                                	// outputs
			: [ti] "m"(ti), [val] "r"(val)                  	// inputs
			  : "eax", "rbx"                                   	// clobbers
	);

	/* 
        or use function call to atomic_exchange_add which contains above assembly code
        
        atomic_exchange_add(&ti->preempt_count, val);
	
        however, every additional function call will slow the system, and altering preemption counts
        happens a lot, so have opted for direct inlining of assembly code here
    */

#ifdef ENABLE_DEBUG_TRACE
    if (ti->preempt_count < 0) {
    	tprintk("thread %u - preempt_count is %d.\n", ti->id, ti->preempt_count);
    }
#endif
    BUG_ON(ti->preempt_count < 0);
}

#define sub_preempt_count(thread, _x)   add_preempt_count(thread, -(_x))
#define inc_preempt_count() add_preempt_count(current, 1)
#define dec_preempt_count() sub_preempt_count(current, 1)


#define preempt_disable()                   \
do {                                        \
    inc_preempt_count();                    \
    barrier();                              \
} while (0)


#define preempt_enable_no_resched()         \
do {                                        \
    barrier();                              \
    dec_preempt_count();                    \
} while (0)

#define preempt_check_resched()             \
do {                                        \
    if (unlikely(need_resched(current)))    \
        preempt_schedule();                 \
} while (0)

#define preempt_enable()                    \
do {                                        \
    preempt_enable_no_resched();            \
    barrier();                              \
    preempt_check_resched();                \
} while (0)

static inline int in_spinlock(struct thread *t)
{
    return t->lock_count;
}

#define is_preemptible(_ti) (((_ti)->preempt_count == 0) || ((_ti)->preempt_count == PREEMPT_ACTIVE))

static inline void stack_push(struct thread *thread, unsigned long value)
{
    thread->sp -= sizeof(unsigned long);
    *((unsigned long *)thread->sp) = value;
}

extern void backtrace(void **bp, void *ip);

struct sleep_queue
{
    uint32_t flags;
    struct list_head list;
    s_time_t timeout;
    struct thread *thread;
};

#define DEFINE_SLEEP_QUEUE(name)                \
    struct sleep_queue name = {                 \
	.list = LIST_HEAD_INIT((name).list),        \
	.thread = current,                          \
	.flags = 0,                                 \
    }

static inline void init_sleep_queue(struct sleep_queue *sq)
{
    INIT_LIST_HEAD(&sq->list);
    sq->flags = 0;
}

void *create_timer(void);
void delete_timer(struct sleep_queue *sq);
void add_timer(struct sleep_queue *sq, s_time_t timeout);
int remove_timer(struct sleep_queue *sq);

#define ACTIVE_SQ_FLAG    0x00000001
#define EXPIRED_SQ_FLAG   0x00000002

#define DEFINE_SLEEP_FLAG(flag_name, flag_set_prefix, funct_name)    \
static unsigned long inline flag_set_prefix##funct_name(             \
                                        struct sleep_queue *sq)      \
{                                                                    \
    return sq->flags & flag_name##_SQ_FLAG;                          \
}                                                                    \
                                                                     \
static void inline set_##funct_name(struct sleep_queue *sq)          \
{                                                                    \
    sq->flags |= flag_name##_SQ_FLAG;                                \
}                                                                    \
                                                                     \
static void inline clear_##funct_name(struct sleep_queue *sq)        \
{                                                                    \
    sq->flags &= ~flag_name##_SQ_FLAG;                               \
}

DEFINE_SLEEP_FLAG(ACTIVE, is_, active);
DEFINE_SLEEP_FLAG(EXPIRED, is_, expired);

void sleep_queue_add(struct sleep_queue *sq);
void sleep_queue_del(struct sleep_queue *sq);

#define MXCSRINIT 0x1f80

#define save_fp_regs_asm "fxsave (%[fpr])\n\t"

#define restore_fp_regs_asm "fxrstor (%[fpr])\n\t"

void exit_current_thread(void);
void exit_thread(struct thread *thread);

void cancel_thread(struct thread *thread);

void init_local_space(void);
void *get_local_space(void);
void reorder(struct thread *prev, int cpu);

void hibernate(struct thread *thread);
void restore(struct thread *thread);
void os_sched_delete(struct thread *thread);

void suspend_thread (struct thread * thread);

#include <os/bug.h>

#define STACK_SIZE_PAGE_ORDER    2
#define STACK_SIZE               (PAGE_SIZE * (1 << STACK_SIZE_PAGE_ORDER))

#define CLOBBER_LIST  \
    ,"r8","r9","r10","r11","r12","r13","r14","r15"
#define arch_switch_threads(prev, next, last) do {                      \
    unsigned long rsi,rdi;                                              \
    __asm__ __volatile__("pushfq\n\t"                                   \
                         "pushq %%rbp\n\t"                              \
                         "movq %%rsp,%0\n\t"         /* save RSP */     \
                         "movq %5,%%rsp\n\t"        /* restore RSP */   \
                         "movq $1f,%1\n\t"          /* save RIP */      \
                         "pushq %6\n\t"             /* restore RIP */   \
                         "ret\n\t"                                      \
                         "1:\t"                                         \
                         "popq %%rbp\n\t"                               \
                         "popfq\n\t"                                    \
                         :"=m" (prev->sp),"=m" (prev->ip),              \
                          "=a"(last), "=S" (rsi),"=D" (rdi)             \
                         :"m" (next->sp),"m" (next->ip),                \
                          "2" (prev), "d" (next)                        \
                         :"memory", "cc" CLOBBER_LIST);                 \
} while (0)

#define current ({                                                      \
        struct thread *_current;                                        \
        asm volatile ("mov %%gs:24,%0" : "=r"(_current));               \
        _current;                                                       \
        })


static inline void restart_idle_thread(long cpu,
	unsigned long sp,
	unsigned long ip,
	void (*idle_thread_fn)(void *data))
{

    __asm__ __volatile__("mov %1,%%rsp\n\t"
	                 "push %3\n\t"
	                 "push %0\n\t"
	                 "push %2\n\t"
			 "ret"
	    : "=m" (cpu)
	    : "r" (sp), "r" (ip), "r" (idle_thread_fn)
	    );
}

#endif
