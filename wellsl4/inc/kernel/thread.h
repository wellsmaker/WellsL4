#ifndef KERNEL_THREAD_H_
#define KERNEL_THREAD_H_

#include <object/kip.h>
#include <state/statedata.h>
#include <model/spinlock.h>
#include <types_def.h>
#include <sys/assert.h>
#include <sys/stdbool.h>
#include <model/smp.h>
#include <arch/cpu.h>
#include <default/default.h>
#include <kernel/idle.h>
#include <kernel_object.h>
#include <model/sporadic.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SCHEDULER_ACTION_RESUME_CURRENT_THREAD     ((struct ktcb *) 0)
#define SCHEDULER_ACTION_CHOOSE_NEW_THREAD         ((struct ktcb *) 1)
#define SCHEDULER_ACTION_CHOOSE_PRIV_THREAD        ((struct ktcb *) 2) 

#define READY_Q_ADD_HEAD    (0)
#define READY_Q_ADD_TAIL    (1)
#define READY_Q_MOVE_TAIL    (2)
#define READY_Q_REMOVE_HEAD    (3)
#define READY_Q_POP_PUSH (4)
#define READY_Q_POSTPONE (5)

#define SCHEDULER_PRIORITY_MASK    		0x00000FFF
#define SCHEDULER_MCP_MASK         		0x00FFF000
#define SCHEDULER_LEVEL_MASK            0xFF000000
#define SCHEDULER_BUDGET_MASK    		0xFFF00000
#define SCHEDULER_PERIOD_MASK 			0x000FFF00
#define SCHEDULER_MAX_REFILLS_MASK 		0x000000FF

#define HARD_PRIOR_ACTION 0xFF
#define SOFT_PRIOR_ACTION 0x00


static FORCE_INLINE bool_t __PURE is_roundrobin(struct thread_sched *sched)
{
    return sched->period == 0;
}

static FORCE_INLINE __CONST word_t ready_queues_index(word_t dom, word_t prio)
{
    if (CONFIG_NUM_DOMAINS > 1) 
	{
        return dom * NUM_PRIORITIES + prio;
    } 
	else 
	{
        assert(dom == 0);
        return prio;
    }
}

static FORCE_INLINE __CONST word_t prio_to_l1index(word_t prio)
{
    return (prio >> WORD_SIZE);
}

static FORCE_INLINE __CONST word_t l1index_to_prio(word_t l1index)
{
    return (l1index << WORD_SIZE);
}

static FORCE_INLINE __CONST word_t invert_l1index(word_t l1index)
{
    word_t inverted = (L2_BITMAP_BITS - 1 - l1index);
    assert(inverted < L2_BITMAP_BITS);
    return inverted;
}

static FORCE_INLINE bool_t __PURE is_runnable(const struct ktcb *thread)
{
    switch (thread->base.thread_state.obj_state & state_queued_state) 
	{
	    case state_queued_state: /*ready*/
	        return TRUE;
	    default:
	        return FALSE;
    }
}

static FORCE_INLINE bool_t __PURE is_time_sensitived(struct ktcb *thread)
{
    return thread->sched != NULL && thread->sched->refill_max > 0;
}


static FORCE_INLINE bool_t __PURE is_schedulable(struct ktcb *thread)
{
    return thread->sched != NULL &&
		   	is_runnable(thread) &&
           	thread->sched->refill_max > 0;
}


static FORCE_INLINE bool_t check_prio(prio_t prio, struct ktcb *thread)
{
    prio_t mcp;

    mcp = thread->base.mcp;

    /* system invariant: existing MCPs are bounded */
    assert(mcp <= NUM_PRIORITIES);

    /* can't assign a sched_prior greater than our own mcp */
    if (prio > mcp) 
	{
        return FALSE;
    }

    return TRUE;
}

static FORCE_INLINE bool_t __PURE is_blocked(const struct ktcb *thread)
{
    switch (thread->base.thread_state.obj_state) 
	{
		case state_suspended_state:
	        return TRUE;
	    default:
	        return FALSE;
    }
}

static inline void add_to_bitmap(word_t dom, word_t prio)
{
	word_t l1index;
	word_t l1index_inverted;

	l1index = prio_to_l1index(prio);
	l1index_inverted = invert_l1index(l1index);

	ready_queues_l1_bitmap[dom] |= BIT(l1index);
	/* we invert the l1 index when accessed the 2nd level of the bitmap in
	   order to increase the liklihood that high prio record_threads l2 index word will
	   be on the same cache line as the l1 index word - this makes sure the
	   fastpath is fastest for high prio record_threads */
	ready_queues_l2_bitmap[dom][l1index_inverted] |= BIT(prio & MASK(WORD_SIZE));
}

static inline void remove_from_bitmap(word_t dom, word_t prio)
{
	word_t l1index;
	word_t l1index_inverted;

	l1index = prio_to_l1index(prio);
	l1index_inverted = invert_l1index(l1index);
	ready_queues_l2_bitmap[dom][l1index_inverted] &= ~ BIT(prio & MASK(WORD_SIZE));
	if (!(ready_queues_l2_bitmap[dom][l1index_inverted]))
	{
		ready_queues_l1_bitmap[dom] &= ~ BIT(l1index);
	}
}

/* Add sched to the head of a scheduler queue */
static inline void sched_enqueue(struct ktcb *thread)
{
	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (is_time_sensitived(thread))
	{
		assert(is_schedulable(thread));
		assert(refill_sufficient(thread->sched, 0));
		assert(refill_ready(thread->sched));
	}

	struct tcb_queue queue;
	dom_t dom;
	prio_t prio;
	word_t idx;

	dom = thread->base.domain;
	prio = thread->base.sched_prior;
	idx = ready_queues_index(dom, prio);
	queue = ready_queues[idx];

	if (!queue.tail) 
	{ /* Empty list */
		queue.tail = thread;
		add_to_bitmap(dom, prio);
	} 
	else 
	{
		queue.head->ready_q_prev = thread;
	}
	
	thread->ready_q_prev = NULL;
	thread->ready_q_next = queue.head;
	queue.head = thread;

	ready_queues[idx] = queue;
}

/* Add sched to the tail of a scheduler queue */
static inline void sched_append(struct ktcb *thread)
{
	
	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (is_time_sensitived(thread))
	{
		assert (is_schedulable(thread));
		assert(refill_sufficient(thread->sched, 0));
		assert(refill_ready(thread->sched));
	}
	
	struct tcb_queue queue;
	dom_t dom;
	prio_t prio;
	word_t idx;

	dom = thread->base.domain;
	prio = thread->base.sched_prior;
	idx = ready_queues_index(dom, prio);
	queue = ready_queues[idx];

	if (!queue.head) 
	{ /* Empty list */
		queue.head = thread;
		add_to_bitmap(dom, prio);
	} 
	else 
	{
		queue.tail->ready_q_next = thread;
	}
	thread->ready_q_prev = queue.tail;
	thread->ready_q_next = NULL;
	queue.tail = thread;

	ready_queues[idx] = queue;
}

/* Remove sched from a scheduler queue */
static inline void sched_dequeue(struct ktcb *thread)
{
	struct tcb_queue queue;
	dom_t dom;
	prio_t prio;
	word_t idx;

	dom = thread->base.domain;
	prio = thread->base.sched_prior;
	idx = ready_queues_index(dom, prio);
	queue = ready_queues[idx];

	if (thread->ready_q_prev) 
	{
		thread->ready_q_prev->ready_q_next = thread->ready_q_next;
	} 
	else 
	{
		queue.head = thread->ready_q_next;
		if (!thread->ready_q_next) 
		{
			remove_from_bitmap(dom, prio);
		}
	}

	if (thread->ready_q_next)
	{
		thread->ready_q_next->ready_q_prev = thread->ready_q_prev;
	} 
	else 
	{
		queue.tail = thread->ready_q_prev;
	}

	ready_queues[idx] = queue;
}

/* Add sched to release queue anywhere, according to timestramp, and insert sched queue */
static inline void release_enqueue(struct ktcb *thread)
{
	struct ktcb *before = NULL;
	struct ktcb *after = release_queue;

	/* find our place in the ordered queue */
	while (after != NULL &&
		   REFILL_HEAD(thread->sched).refill_time >= REFILL_HEAD(after->sched).refill_time)
	{
		before = after;
		after = after->ready_q_next;
	}

	if (before == NULL) 
	{
		/* insert at head */
		release_queue = thread;
		reprogram = TRUE;
	} 
	else 
	{
		before->ready_q_next = thread;
	}

	if (after != NULL) 
	{
		after->ready_q_prev = thread;
	}

	thread->ready_q_next = after;
	thread->ready_q_prev = before;
}

/* get/remove the release queue head item */
static inline struct ktcb *release_dequeue(void)
{
	struct ktcb *detached_head = release_queue;
	release_queue = release_queue->ready_q_next;

	if (release_queue) 
	{
		release_queue->ready_q_prev = NULL;
	}

	if (detached_head->ready_q_next) 
	{
		detached_head->ready_q_next->ready_q_prev = NULL;
		detached_head->ready_q_next = NULL;
	}

	reprogram = TRUE;

	return detached_head;
}

/* remove the release queue item anywhere */
static inline void release_remove(struct ktcb *thread)
{
	if (thread->ready_q_prev) 
	{
		thread->ready_q_prev->ready_q_next = thread->ready_q_next;
	} 
	else 
	{
		release_queue = thread->ready_q_next;
		/* the head has changed, we might need to set a new timeout */
		reprogram = TRUE;
	}

	if (thread->ready_q_next) 
	{
		thread->ready_q_next->ready_q_prev = thread->ready_q_prev;
	}

	thread->ready_q_next = NULL;
	thread->ready_q_prev = NULL;
}



static FORCE_INLINE struct tcb_queue message_append(struct ktcb *thread, struct tcb_queue queue)
{
	if(!queue.head)
	{
		queue.head = thread;
	}
	else
	{
		queue.tail->mesg_q_next = thread;
	}

	thread->mesg_q_prev = queue.tail;
	thread->mesg_q_next = NULL;
	queue.tail = thread;
	return queue;
}

static FORCE_INLINE struct tcb_queue message_dequeue(struct ktcb *thread, struct tcb_queue queue)
{
	if(thread->mesg_q_prev)
	{
		thread->mesg_q_prev->mesg_q_next = thread->mesg_q_next;
	}
	else
	{
		queue.head = thread->mesg_q_next;
	}

	if(thread->mesg_q_next)
	{
		thread->mesg_q_next->mesg_q_prev = thread->mesg_q_prev;
	}
	else
	{
		queue.tail = thread->mesg_q_prev;
	}

	return queue;
}

static FORCE_INLINE bool_t smp_idle_domain(void)
{
	prio_t prio = _idle_thread->base.sched_prior;
	dom_t  dom = _idle_thread->base.domain;
	word_t l1index = prio_to_l1index(prio);

	return (ready_queues_l1_bitmap[dom]    & BIT(l1index)) ? true : false;
}

static FORCE_INLINE bool is_idle_thread_set(void *entry_point)
{
	return entry_point == idle_thread_entry;
}

static FORCE_INLINE bool smp_idle_thread_object(struct ktcb *thread)
{
#ifdef CONFIG_SMP
	return thread->base.smp_is_idle;
#else
	return thread == _idle_thread;
#endif
}

static FORCE_INLINE bool is_thread_dummy(struct ktcb *thread)
{
	return (thread->base.thread_state.obj_state & state_dummy_state) != 0U;
}

static FORCE_INLINE bool is_thread_pending(struct ktcb *thread)
{
	return (thread->base.thread_state.obj_state & 
		(state_send_blocked_state | state_recv_blocked_state | state_notify_blocked_state)) != 0U;
}

static FORCE_INLINE bool is_thread_not_running(struct ktcb *thread)
{
	byte_t state = thread->base.thread_state.obj_state;

	return (state & (state_send_blocked_state | state_recv_blocked_state | 
			 state_notify_blocked_state |
			 state_restart_state | state_dead_state |
			 state_dummy_state | state_suspended_state)) != 0U;

}

static FORCE_INLINE bool is_thread_ready(struct ktcb *thread)
{
	return is_thread_not_running(thread) == 0;
}

static FORCE_INLINE bool has_thread_started(struct ktcb *thread)
{
	return (thread->base.thread_state.obj_state & state_restart_state) == 0U;
}

static FORCE_INLINE bool is_thread_state_set(struct ktcb *thread, u32_t state)
{
	return (thread->base.thread_state.obj_state & state) != 0U;
}

static FORCE_INLINE bool is_thread_queued(struct ktcb *thread)
{
	return is_thread_state_set(thread, state_queued_state);
}

static FORCE_INLINE bool is_thread_running(struct ktcb *thread)
{
	return !is_thread_not_running(thread) && !is_thread_queued(thread);
}


static FORCE_INLINE void marktcb_as_not_dummy(struct ktcb *thread)
{
	thread->base.thread_state.obj_state &= ~state_dummy_state;
}

static FORCE_INLINE void marktcb_as_suspended(struct ktcb *thread)
{
	thread->base.thread_state.obj_state |= state_suspended_state;
}

static FORCE_INLINE void marktcb_as_not_suspended(struct ktcb *thread)
{
	thread->base.thread_state.obj_state &= ~state_suspended_state;
}

static FORCE_INLINE void marktcb_as_started(struct ktcb *thread)
{
	thread->base.thread_state.obj_state &= ~state_restart_state;
}

static FORCE_INLINE void marktcb_as_s_pending(struct ktcb *thread)
{
	thread->base.thread_state.obj_state |= state_send_blocked_state;
}

static FORCE_INLINE void marktcb_as_s_not_pending(struct ktcb *thread)
{
	thread->base.thread_state.obj_state &= ~state_send_blocked_state;
}

static FORCE_INLINE void marktcb_as_r_pending(struct ktcb *thread)
{
	thread->base.thread_state.obj_state |= state_send_blocked_state;
}

static FORCE_INLINE void marktcb_as_r_not_pending(struct ktcb *thread)
{
	thread->base.thread_state.obj_state &= ~state_send_blocked_state;
}

static FORCE_INLINE void set_thread_states(struct ktcb *thread, u32_t states)
{
	thread->base.thread_state.obj_state |= states;
}

static FORCE_INLINE void reset_thread_states(struct ktcb *thread,
					u32_t states)
{
	thread->base.thread_state.obj_state &= ~states;
}

static FORCE_INLINE void marktcb_as_queued(struct ktcb *thread)
{
	set_thread_states(thread, state_queued_state);
}

static FORCE_INLINE void marktcb_as_not_queued(struct ktcb *thread)
{
	reset_thread_states(thread, state_queued_state);
}

static FORCE_INLINE bool is_prio1_higher_than_or_equal_to_prio2(sword_t prio1, sword_t prio2)
{
	return prio1 <= prio2;
}

static FORCE_INLINE bool is_prio_higher_or_equal(sword_t prio1, sword_t prio2)
{
	return is_prio1_higher_than_or_equal_to_prio2(prio1, prio2);
}

static FORCE_INLINE bool is_prio1_lower_than_or_equal_to_prio2(sword_t prio1, sword_t prio2)
{
	return prio1 >= prio2;
}

static FORCE_INLINE bool is_prio1_higher_than_prio2(sword_t prio1, sword_t prio2)
{
	return prio1 < prio2;
}

static FORCE_INLINE bool is_prio_higher(sword_t prio, sword_t test_prio)
{
	return is_prio1_higher_than_prio2(prio, test_prio);
}

static FORCE_INLINE bool is_prio_lower_or_equal(sword_t prio1, sword_t prio2)
{
	return is_prio1_lower_than_or_equal_to_prio2(prio1, prio2);
}


static FORCE_INLINE void set_ready_thread(struct ktcb *thread)
{
	if (is_thread_ready(thread)) 
	{
		add_to_ready_q(thread);
	}
}

static FORCE_INLINE byte_t get_sched_lock(struct ktcb *thread)
{
	return thread->base.sched_locked;
}

static FORCE_INLINE void sched_unlock_no_reschedule(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	assert_info(!arch_is_in_isr(), "");
	assert_info(_current_thread->base.sched_locked != 0, "");
	
	compiler_barrier();
	++_current_thread->base.sched_locked;
#endif
}

#ifdef CONFIG_STACK_SENTINEL
extern void check_stack_sentinel(void);
#else
#define check_stack_sentinel() 
#endif

sword_t enable_thread_smp_cpu_mask_all(struct ktcb *thread);
sword_t enable_thread_smp_cpu_mask(struct ktcb *thread, sword_t cpu);
sword_t disable_thread_smp_cpu_mask_all(struct ktcb *thread);
sword_t disable_thread_smp_cpu_mask(struct ktcb *thread, sword_t cpu);

void set_prior(struct ktcb *thread, prio_t prio, prio_t mcp);
void schedule(void);
void add_to_ready_q(struct ktcb *thread);
void add_to_end_ready_q(struct ktcb *thread);
void move_to_end_ready_q(struct ktcb *thread);
void remove_from_ready_q(struct ktcb *thread);
void postpone_from_ready_q(struct ktcb *thread);
struct ktcb *next_thread(void);
void reschedule(spinlock_t *sched_lock, spinlock_key_t key);
void reschedule_irqlock(u32_t key);
void reschedule_unlocked(void);
void sched_lock(void);
void sched_unlock(void);
void set_current_thread(struct ktcb *thread);
void charge_budget(ticks_t capacity, ticks_t consumed, bool_t canTimeoutFault);
void update_timestamp(bool_t is_reset);
bool_t check_budget(void);
void commit_time(void);
bool_t check_budget_restart(void);
void reschedule_required(void);
void possible_switchto(struct ktcb *thread);
void awaken(void);
sword_t postpone_cur_irqlock(word_t key);
sword_t postpone_cur(spinlock_t *thread_swap_lock, spinlock_key_t key);

#ifdef CONFIG_SMP
void sched_ipi(void);
void sched_abort(struct ktcb *thread);
#endif

/* find which one is the next thread to run */
/* must be called with interrupts locked */
#ifdef CONFIG_SMP
struct ktcb *get_next_ready_thread(void);
#else
static FORCE_INLINE struct ktcb *get_next_ready_thread(void)
{
	return _kernel.ready_thread;
}
#endif

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* This is a arch function traditionally, but when the switch-based
 * swap_thread() is in use it's a simple FORCE_INLINE provided by the kernel.
 */
static FORCE_INLINE void arch_thread_swap_retval_set(struct ktcb *thread, word_t value)
{
	thread->swap_retval = value;
}

/* New style context switching.  arch_switch() is a lower level
 * primitive that doesn't know about the scheduler or return value.
 * Needed for SMP, where the scheduler requires spinlocking that we
 * don't want to have to do in per-architecture assembly.
 *
 * Note that isspinlock is a compile-time construct which will be
 * optimized out when this function is expanded.
 */
static FORCE_INLINE word_t non_arch_swap(word_t key, spinlock_t *lock, word_t isspinlock)
{
	ARG_UNUSED(lock);
	
	struct ktcb *thread;
#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_swap(void);
	read_timer_start_of_swap();
#endif

	check_stack_sentinel();

	if (isspinlock) 
	{
		release_spin_release(lock);
	}

	thread = get_next_ready_thread();

	if (thread != _current_thread) 
	{
		_current_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;

		thread->base.smp_cpu_id = arch_curr_cpu()->core_id;

		if (!isspinlock) 
		{
			smp_release_global_lock(thread);
		}
#endif

		set_current_thread(thread);
		update_timestamp(true);
		arch_switch(thread->swap_handler, &_current_thread->swap_handler);
	}

	if (isspinlock)
	{
		arch_irq_unlock(key);
	} 
	else
	{
		irq_unlock(key);
	}

	return _current_thread->swap_retval;
}

static FORCE_INLINE sword_t swap_thread_irqlock(word_t key)
{
	return non_arch_swap(key, NULL, false);
}

static FORCE_INLINE sword_t swap_thread(spinlock_t *lock, spinlock_key_t key)
{
	return non_arch_swap(key.key, lock, true);
}

static FORCE_INLINE void swap_thread_unlocked(void)
{
	spinlock_t lock = {};
	spinlock_key_t key = lock_spin_lock(&lock);

	(void) swap_thread(&lock, key);
}

#else

extern sword_t arch_swap(word_t key);
static FORCE_INLINE sword_t swap_thread_irqlock(word_t key)
{
	check_stack_sentinel();
	update_timestamp(true);
	return arch_swap(key);
}

/* If !USE_SWITCH, then spinlocks are guaranteed degenerate as we
 * can't be in SMP.  The release_spin_release() call is just for validation
 * handling.
 */
static FORCE_INLINE sword_t swap_thread(spinlock_t *lock, spinlock_key_t key)
{
	release_spin_release(lock);
	
	return swap_thread_irqlock(key.key);
}

static FORCE_INLINE void swap_thread_unlocked(void)
{
	(void) swap_thread_irqlock(arch_irq_lock());
}

#endif

static FORCE_INLINE void swap_thread_retval_data_set(
				struct ktcb *thread,
				word_t value, void *data)
{
	arch_thread_swap_retval_set(thread, value);
	thread->base.swap_data = data;
}

#ifdef __cplusplus
}
#endif

#endif
