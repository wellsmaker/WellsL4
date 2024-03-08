#include <kernel/thread.h>
#include <model/spinlock.h>
#include <api/syscall.h>
#include <drivers/timer/system_timer.h>
#include <sys/stdbool.h>
#include <object/tcb.h>
#include <model/sporadic.h>
#include <object/ipc.h>
#include <object/interrupt.h>
#include <kernel/time.h>
#include <api/errno.h>
#include <state/statedata.h>
#include <device.h>
#include <sys/util.h>
#include <sys/assert.h>
#include <kernel/privilege.h>

/* time constants */
#define MS_IN_S     1000u
#define US_IN_MS    1000u
#define HIN_KHZ   1000u
#define KHIN_MHZ  1000u
#define HIN_MHZ   1000000u

static spinlock_t thread_swap_lock;

extern struct ktcb main_thread;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

/* The sched_prior starts from 0, and the higher the number, the higher the sched_prior */
static inline prio_t get_highest_prio(word_t dom)
{
	word_t l1index;
	word_t l2index;
	word_t l1index_inverted;

	/* it's undefined to call __CLZ on 0 */
	assert(ready_queues_l1_bitmap[dom] != 0);

	l1index = WORD_BITS - 1 - clzl(ready_queues_l1_bitmap[dom]); /* clzl ~ WORD_BITS */
	l1index_inverted = invert_l1index(l1index);

	assert(ready_queues_l2_bitmap[dom][l1index_inverted] != 0);

	l2index = WORD_BITS - 1 - clzl(ready_queues_l2_bitmap[dom][l1index_inverted]);

	return (l1index_to_prio(l1index) | l2index);
}

static inline bool_t is_highest_prio(word_t dom, prio_t prio)
{
	return ready_queues_l1_bitmap[dom] == 0 ||
		   prio >= get_highest_prio(dom);
}

static inline bool_t is_t1_higher_prio_than_t2(struct ktcb *thread_1,
				 struct ktcb *thread_2)
{
	if (thread_1->base.sched_prior > thread_2->base.sched_prior) 
	{
		return true;
	}

#ifdef CONFIG_SCHED_DEADLINE
	/* Note that we don't care about wraparound conditions.  The
	 * expectation is that the application will have arranged to
	 * block the record_threads, change their priorities or reset their
	 * deadlines when the job is complete.  Letting the deadlines
	 * go negative is fine and in fact prevents aliasing bugs.
	 */
	if (thread_1->base.sched_prior == thread_2->base.sched_prior) 
	{
		sword_t dt1 = thread_1->base.mcp;
		sword_t dt2 = thread_2->base.mcp;

		return dt1 > dt2;
	}
#endif

	return false;
}

static inline bool_t should_preempt(struct ktcb *thread, word_t preempt_ok)
 {
	 /* Preemption is OK if it's being explicitly allowed by
	  * software state (e.g. the thread called k_yield())
	  */
	 if (preempt_ok != false) 
	 {
		 return true;
	 }
 
	 /* assert_info(_current_thread != NULL, "current thread is invaild!"); */
 
	 /* Or if we're pended/suspended/dummy (duh) */
	 if (is_thread_not_running(_current_thread)) 
	 {
		 return true;
	 }
 
	 /* Edge case on ARM where a thread can be pended out of an
	  * interrupt handler before the "synchronous" swap starts
	  * context switching.	Platforms with atomic swap can never
	  * hit this.
	  */
	 if (IS_ENABLED(CONFIG_SWAP_NONATOMIC) && is_thread_not_running(thread))
	 {
		 return true;
	 }
 
	 /* The idle record_threads can look "cooperative" if there are no
	  * preemptible priorities (this is sort of an API glitch).
	  * They must always be preemptible.
	  */
	 if (!IS_ENABLED(CONFIG_PREEMPT_ENABLED) && smp_idle_thread_object(_current_thread)) 
	 {
		 return true;
	 }
 
	 return false;
 }


#ifdef CONFIG_SCHED_CPU_MASK

#ifdef CONFIG_SMP
/* Right now we use a single byte for this mask */
BUILD_ASSERT_MSG(CONFIG_MP_NUM_CPUS <= 8, "Too many CPUs for mask word");
#endif

static sword_t smp_cpu_mask_mod(struct ktcb *thread, word_t enable_mask, word_t disable_mask)
{
	sword_t ret = 0;

	LOCKED(&thread_swap_lock) 
	{
		if (is_thread_not_running(thread)) 
		{
			thread->base.smp_cpu_mask |= enable_mask;
			thread->base.smp_cpu_mask  &= ~disable_mask;
		} 
		else 
		{
			ret = -EINVAL;
		}
	}
	return ret;
}

sword_t enable_thread_smp_cpu_mask_all(struct ktcb *thread)
{
	return smp_cpu_mask_mod(thread, 0xffffffff, 0);
}

sword_t enable_thread_smp_cpu_mask(struct ktcb *thread, sword_t cpu)
{
	return smp_cpu_mask_mod(thread, BIT(cpu), 0);
}

sword_t disable_thread_smp_cpu_mask_all(struct ktcb *thread)
{
	return smp_cpu_mask_mod(thread, 0, 0xffffffff);
}

sword_t disable_thread_smp_cpu_mask(struct ktcb *thread, sword_t cpu)
{
	return smp_cpu_mask_mod(thread, 0, BIT(cpu));
}

/* With masks enabled we need to be prepared to walk the list
 * looking for one we can run
 */

static inline struct ktcb *is_cur_cpu_expired(void)
{
	struct ktcb *thread;
	
	if(thread->base.smp_cpu_mask & BIT(_current_cpu->core_id) != 0)
	{
		return thread;
	}
	else
	{
		return NULL;

	}
}

#endif /* CONFIG_SCHED_CPU_MASK */

#ifdef CONFIG_SMP
/* Called out of the scheduler interprocessor interrupt.  All it does
 * is flag the _current_thread thread as dead if it needs to abort, so the ISR
 * return into something else and the other thread which called
 * thread_abort() can finish its work knowing the thing won't be
 * rescheduled.
 * arch_sched_ipi() - sched_ipi()
 */
void sched_ipi(void)
{
	LOCKED(&thread_swap_lock) 
	{
		if (get_thread_state(_current_thread, state_aborting_state)) 
		{
			set_thread_state(_current_thread, state_dead_state);
			_current_cpu->swap_ok = true;
		}
	}
}

void sched_abort(struct ktcb *thread)
{
	spinlock_key_t key;

	if (thread == _current_thread)
	{
		remove_from_ready_q(thread);
		reschedule_required();
		return;
	}

	/* First broadcast an IPI to the other CPUs so they can stop
	 * it locally.  Not all architectures support that, alas.  If
	 * we don't have it, we need to wait for some other interrupt.
	 */
	set_thread_state(thread, state_aborting_state);

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	arch_sched_ipi();
#endif /* CONFIG_SCHED_IPI_SUPPORTED */

	/* Wait for it to be flagged dead either by the CPU it was
	 * running on or because we caught it idle in the queue
	 */
	while (get_thread_state(_current_thread, state_dead_state) == 0U) 
	{
		key = lock_spin_lock(&thread_swap_lock);
		if (!is_thread_not_running(thread) && !is_schedulable(thread)) 
		{
			unlock_spin_unlock(&thread_swap_lock, key);
			thread_busy_wait(100);
		}
		else
		{
			remove_from_ready_q(thread);
			set_thread_state(thread, state_dead_state);
			unlock_spin_unlock(&thread_swap_lock, key);
		}
	}
}

struct ktcb *get_next_ready_thread(void)
{
	struct ktcb *ret;

	LOCKED(&thread_swap_lock) 
	{
		ret = next_thread();
	}

	return ret;
}

#endif /* CONFIG_SMP */



/* domain time is small to consume time, and need to re-schedule, and switch next domain */
static inline bool_t is_cur_domain_expired(void)
{
	return CONFIG_NUM_DOMAINS > 1 &&
		   current_domain_time < (consume_time + MIN_BUDGET);
}


static void next_domain(void)
{
	current_domain_schedule_idx++;
	if (current_domain_schedule_idx >= domain_schedule_length)
	{
		current_domain_schedule_idx = 0;
	}

	work_units_completed = 0;
	reprogram			   =  TRUE;
	current_domain		   =  domain_schedule[current_domain_schedule_idx].domain;
	current_domain_time    =  k_us_to_ticks_ceil32((domain_schedule[current_domain_schedule_idx].length) * US_IN_MS);
}

/* _current_thread time is next thread exec time point , this _current_thread thread has updated after switch context */
static void set_next_arrival_interrupt(void)
{
	times_t next_interrupt = 0;

	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	next_interrupt = current_time + REFILL_HEAD(_current_thread->sched).refill_amount;
	
	if (CONFIG_NUM_DOMAINS > 1)
	{
		next_interrupt = MIN(next_interrupt, current_time + current_domain_time);
	}

	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (release_queue) 
	{
		next_interrupt = MIN(REFILL_HEAD(release_queue->sched).refill_time, next_interrupt);
	}
	
	if (next_interrupt != 0)
	{
		set_deadline(next_interrupt - get_timer_precision() - current_time, 
			&_current_thread->thread_id);
	}
}

static void switch_to_sc(void)
{
	assert(_current_thread->sched != NULL);
	
	/* check the _current_thread thread sched */
	if (_current_thread && _current_thread->sched->refill_max > 0) 
	{
		reprogram = TRUE;
		
		refill_noblock_check(_current_thread->sched);

		assert(refill_ready(_current_thread->sched));
		assert(refill_sufficient(_current_thread->sched, 0));
	}


	/* whenever sched is ready, current_sched update */
	current_sched = _current_thread->sched;
}

/** next_thread */
struct ktcb *next_thread(void)
{
	word_t prio;
	word_t dom;
	struct ktcb *thread;

	if (CONFIG_NUM_DOMAINS > 1)
	{
		dom = current_domain;
	} 
	else 
	{
		dom = 0;
	}

	/* get 'sched_prior max' thread to exec */
	if (ready_queues_l1_bitmap[dom]) 
	{
		prio = get_highest_prio(dom);
		thread = ready_queues[ready_queues_index(dom, prio)].head;
		
		if (thread != _current_thread 
#ifdef CONFIG_SCHED_CPU_MASK
		&& is_cur_cpu_expired(thread)
#endif 
		)
		{
			assert(thread);

			/* Add support for direct jump scheduling and preset sched_prior scheduling */
			if (is_time_sensitived(thread))
			{
				assert(is_schedulable(thread));
				assert(refill_sufficient(thread->sched, 0));
				assert(refill_ready(thread->sched));
			}

			/* Under SMP, the "cache" mechanism for selecting the next
			 * thread doesn't work, so we have more work to do to test
			 * _current_thread against the best choice from the queue.  Here, the
			 * thread selected above represents "the best thread that is
			 * not _current_thread".
			 *
			 * Subtle note on "queued": in SMP mode, _current_thread does not
			 * live in the queue, so this isn't exactly the same thing as
			 * "ready", it means "is _current_thread already added back to the
			 * queue such that we don't want to re-add it".
			 */
			
			word_t queued = is_thread_queued(_current_thread);
			word_t active = !is_thread_not_running(_current_thread);

			if (active) 
			{
				if (!queued && !is_t1_higher_prio_than_t2(thread, _current_thread)) 
				{
					thread = _current_thread;
				}

				if (!should_preempt(thread, _current_cpu->swap_ok)) 
				{
					thread = _current_thread;
				}
			}

			/* Put current thread back into the queue */
			if (active && !queued && !smp_idle_thread_object(_current_thread)) 
			{
				marktcb_as_queued(_current_thread);
				sched_enqueue(_current_thread);
			}

			/* Take the new current thread out of the queue */
			if (is_thread_queued(thread)) 
			{
				sched_dequeue(thread);
				marktcb_as_not_queued(thread);	
			}
	
			return thread;
		}
		else
		{
			return _current_thread;
		}
	} 
	else 
	{
		return &idle_thread;
	}
}

/** update_cache */
static void update_cache(struct ktcb *thread,  word_t preempt_ok)
{
	assert(get_thread_state(thread, state_dummy_state) == 0);

	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (is_time_sensitived(thread))
	{
		assert(refill_sufficient(thread->sched, 0));
		assert(refill_ready(thread->sched));
	}

	if (should_preempt(thread, preempt_ok))
	{
		_kernel.ready_thread = thread;
	}
	else
	{
		_kernel.ready_thread = _current_thread;
	}

	/* The way this works is that the CPU record keeps its
	 * "cooperative swapping is OK" flag until the next reschedule
	 * call or context switch.	It doesn't need to be tracked per
	 * thread because if the thread gets preempted for whatever
	 * reason the scheduler will make the same decision anyway.
	 */
	_current_cpu->swap_ok = preempt_ok;
}

static void choose_new_thread(void)
{
	struct ktcb *new_thread;

	new_thread = next_thread();
	
	if (current_domain_time == 0) 
	{
		next_domain();
	}

	update_cache(new_thread, true);
}

/** Centralized scheduling, so changing the ready cache task can only be performed 
    in the scheduler, and the rest is to update the ready queue */

void schedule(void)
{
	awaken();
	if (scheduler_action != SCHEDULER_ACTION_RESUME_CURRENT_THREAD) 
	{
		if (scheduler_action == SCHEDULER_ACTION_CHOOSE_NEW_THREAD) 
		{
			choose_new_thread();
		}
		else 
		{	
			struct ktcb *candidate = scheduler_action;

			if (candidate == SCHEDULER_ACTION_CHOOSE_PRIV_THREAD)
			{
				update_cache(&privilege_thread, true);
				goto _schedule_level;
			}
			
			assert (is_thread_queued(candidate));
			/* Avoid checking bitmap when _current_thread is higher prio, to
			 * match fast path.
			 * Don't look at _current_thread prio when it's idle, to respect
			 * information flow in non-fastpath cases. */
			bool_t fastfail = (_current_thread == _idle_thread) || (candidate->base.sched_prior < _current_thread->base.sched_prior);
			
			if (fastfail && !is_highest_prio(current_domain, candidate->base.sched_prior)) 
			{
				marktcb_as_queued(candidate);
				sched_enqueue(candidate);
				/* we can't, need to reschedule */
				scheduler_action = SCHEDULER_ACTION_CHOOSE_NEW_THREAD;
				choose_new_thread();
			} 
			else
			{
				/* We append the candidate at the end of the scheduling queue, that way the
				 * _current_thread thread, that was enqueued at the start of the scheduling queue
				 * will get picked during chooseNewThread */
				marktcb_as_queued(candidate);
				sched_append(candidate);
				scheduler_action = SCHEDULER_ACTION_CHOOSE_NEW_THREAD;
				choose_new_thread();
			}
		}
	}

_schedule_level:
	scheduler_action = SCHEDULER_ACTION_RESUME_CURRENT_THREAD;
}

/* The following 5 functions should all cooperate with the schedule
   function and the swap function */
void add_to_ready_q(struct ktcb *thread)
{
	LOCKED (&thread_swap_lock) 
	{
		marktcb_as_queued(thread);
		sched_enqueue(thread);
		/* update_cache(thread, true); */
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
		arch_sched_ipi();
#endif
	}
}

void add_to_end_ready_q(struct ktcb *thread)
{
	LOCKED(&thread_swap_lock) 
	{
		marktcb_as_queued(thread);
		sched_append(thread);
		/* update_cache(thread, false); */
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
		arch_sched_ipi();
#endif
	}
}

void move_to_end_ready_q(struct ktcb *thread)
{
	LOCKED (&thread_swap_lock) 
	{
		if (is_thread_queued(thread)) 
		{
			sched_dequeue(thread);
			marktcb_as_queued(thread);
		}

		sched_append(thread);
		/* update_cache(thread, thread == _current_thread); */
	}
}

void remove_from_ready_q(struct ktcb *thread)
{
	LOCKED (&thread_swap_lock) 
	{
		if (is_thread_queued(thread)) 
		{
			sched_dequeue(thread);
			marktcb_as_not_queued(thread);
			set_thread_state(thread, state_restart_state);
			/* update_cache(thread, true); */
		}
	}
}

/* call this function when you need exec a thread of the timeout or other not sched ready thread and not right now*/
/* pend */
void postpone_from_ready_q(struct ktcb *thread)
{
	LOCKED (&thread_swap_lock)
	{
		if (is_thread_queued(thread))
		{
			sched_dequeue(thread);
			marktcb_as_not_queued(thread);
		}
		
		release_enqueue(thread);
		reprogram = TRUE;
		/* update_cache(thread, true); */
	}
}

static inline bool_t is_rescheduled(word_t key)
{
#ifdef CONFIG_SMP
	_current_cpu->swap_ok = 0;
#endif

	return arch_irq_unlocked(key) && !arch_is_in_isr();
}

/** thread_swap_lock - spinlock; key - irqlock */
void reschedule(spinlock_t *thread_swap_lock, spinlock_key_t key)
{
	if (is_rescheduled(key.key)) 
	{
		swap_thread(thread_swap_lock, key);
	} 
	else 
	{
		unlock_spin_unlock(thread_swap_lock, key);
	}
}

void reschedule_irqlock(word_t key)
{
	if (is_rescheduled(key))
	{
		swap_thread_irqlock(key);
	} 
	else 
	{
		irq_unlock(key);
	}
}

void reschedule_unlocked(void)
{
	swap_thread_unlocked();
}

void sched_lock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	assert_info(!arch_is_in_isr(), "");
	assert_info(_current_thread->base.sched_locked != 1, "");
	
	LOCKED(&thread_swap_lock)
	{
		--_current_thread->base.sched_locked;
		compiler_barrier();
	}
#endif
}

void sched_unlock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	assert_info(_current_thread->base.sched_locked != 0, "");
	assert_info(!arch_is_in_isr(), "");

	LOCKED(&thread_swap_lock) 
	{
		++_current_thread->base.sched_locked;
		compiler_barrier();
		/* update_cache(next_thread(), false); */
	}
	
	/* enable/disable interrupts */
	reschedule_unlocked();
#endif
}

void set_prior(struct ktcb *thread, prio_t prio, prio_t mcp)
{
	assert(thread != NULL);

	/*
	 * Use NULL, since we cannot know what the entry point is (we do not
	 * keep track of it) and idle cannot change its sched_prior.
	 */
	assert_info(!arch_is_in_isr(), "can not do this in isr!\n");

	bool_t need_sched = 0;

	LOCKED(&thread_swap_lock)
	{
		need_sched = is_thread_ready(thread);
		
		if (need_sched)
		{
			/* Don't requeue on SMP if it's the running thread */
			if (!IS_ENABLED(CONFIG_SMP) && is_thread_queued(thread))
			{
				sched_dequeue(thread);
				thread->base.sched_prior = prio;
				thread->base.mcp	= mcp;
				
				assert(check_prio(prio,thread));

				sched_enqueue(thread);

			}
			else
			{
				thread->base.sched_prior = prio;
				thread->base.mcp	= mcp;
				
				assert(check_prio(prio,thread));
			}

			/* in queue or in running - other way to call it except that syscall */
			reschedule_required();
			schedule();
		}
		else
		{
			thread->base.sched_prior = prio;
			thread->base.mcp	= mcp;
			
			assert(check_prio(prio,thread));
		}
	}

	if (is_thread_pending(thread))
	{
		reorder_message_node(thread);
	}

#ifdef CONFIG_SMP
	if (!IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) 
	{
		sched_ipi();
	}
#endif	

	/* maybe re-schedule - other way to call it except that syscall */
	if (need_sched && _current_thread->base.sched_locked == 0) 
	{
		reschedule_unlocked();
	}
}

/* any thread switch */
void set_current_thread(struct ktcb *thread)
{
	assert(thread != NULL);

	_current_thread   = thread;
	
	 /*
     * Clear PendSV so that if another interrupt comes in and
     * decides, with the new kernel base.thread_state based on the new thread
     * being context-switched in, that it needs to reschedule, it
     * will take, but that previously pended PendSVs do not take,
     * since they were based on the previous kernel base.thread_state and this
     * has been handled.
     */

	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (is_time_sensitived(thread))
	{
		assert(is_thread_running(thread));
		
		switch_to_sc();
		if (reprogram) 
		{
			/* The time generated between task switching is included in the 
			   time budget of the next task */
	        commit_time();
			
			/* means of guaranteeing a deadline that the last */
			/* Mandatory inspection : prevent adverse effects on other tasks due to abnormal operation of one task */
			set_next_arrival_interrupt();
			reprogram = FALSE;
	    }


		thread_activate();
	}
}

static void end_timeslice(bool_t can_timeout_fault)
{
	/* timeout action right now */
	if (can_timeout_fault) 
	{
		/* need consider - current_sched exec time vs deadline time */
		user_error("#### HM PROCESS FOR The thread - %d Start: ####", _current_thread->thread_id);
		/* hard handle way */
		/* Start health monitoring task */
		/* Periodic tasks are set as hard real-time in advance, and aperiodic tasks are soft real-time */
		scheduler_action = SCHEDULER_ACTION_CHOOSE_PRIV_THREAD;
		set_privilege_status(priv_health_monitor_priv, priv_ready_priv);
	} 
	/* Just finished */
	else if (refill_ready(_current_thread->sched) && refill_sufficient(_current_thread->sched, 0)) 
	{
		add_to_end_ready_q(_current_thread);
	} 
	/* postpone until ready */
	else 
	{
		postpone_from_ready_q(_current_thread);
	}
}

/* call it at charge time(also called check_budget(FALSE)), and this time means that thread need checking budge;
   and need set consume time to 0;
   and this thread should be over; if this time ,the thread not final, call 'timeout'*/
void charge_budget(ticks_t capacity, ticks_t consumed, bool_t canTimeoutFault)
{
	if (is_roundrobin(_current_thread->sched))
	{
		assert(refill_size(_current_thread->sched) == MIN_REFILLS);
		REFILL_HEAD(_current_thread->sched).refill_amount += REFILL_TAIL(_current_thread->sched).refill_amount;
		REFILL_TAIL(_current_thread->sched).refill_amount = 0;
	} 
	else 
	{
		/* capacity need set to 0 when not enough (consumed > amount)*/
		/* capacity need set to no-0,when enough (consumed < amount )*/
		refill_budget_check(consumed, capacity);
	}

	assert(REFILL_HEAD(_current_thread->sched).refill_amount >= MIN_BUDGET);
	_current_thread->sched->consumed += consumed;

	/* this time, the _current_thread thread should not be schedulable because of not enough budget */
	/* Preventive examination : Since the task is switching at this time, it is generally not schedulable, 
	   that is, the task state has to become non running state */
	if (is_thread_running(_current_thread)) 
	{
		end_timeslice(canTimeoutFault);
		reschedule_required();
		reprogram = TRUE;
	}
}

/* Update the kernels timestamp and stores in ksCurTime.
 * The difference between the previous kernel timestamp and the one just read
 * is stored in ksConsumed.
 *
 * Should be called on every kernel entry : should be set at 'before thread end exec' or 'next thread start exec'.
 * where record_threads can be billed.
 */
void update_timestamp(bool_t is_reset)
{
	if (is_reset == true)
	{
		consume_time = 0;
	}
	else
	{
		/* _current_thread time always add */
		ticks_t prev = current_time;
		current_time = get_current_tick();
		if(is_time_sensitived(_current_thread))
		{
			consume_time += (current_time - prev);
		}
	}
}

/* Check if the _current_thread thread/domain budget has expired. ( in normal, _current_thread thread should be over )
 * if it has, bill the thread, add it to the scheduler and
 * set up a reschedule.
 *
 * @return TRUE if the thread/domain has enough budget to
 *				get through the _current_thread kernel operation.
 */
 
/* means of guaranteeing a deadline that the first */
bool_t check_budget(void)
{
	/* Add support for direct jump scheduling and preset sched_prior scheduling */
	if (is_time_sensitived(_current_thread))
	{
		/* currently running thread must have available capacity */
		assert(refill_ready(_current_thread->sched));
		
		ticks_t capacity = refill_capacity(_current_thread->sched, consume_time);
		/* if the budget isn't enough, the timeslice for this SC is over. For
		 * round robin record_threads this is sufficient, however for periodic record_threads
		 * we also need to check there is space to schedule the replenishment - if the refill
		 * is full then the timeslice is also over as the rest of the budget is forfeit. */
		
		/* budget is enough */
		if (capacity >= MIN_BUDGET && (is_roundrobin(_current_thread->sched) || !refill_full(_current_thread->sched))) 
		{
			/* budget is not enough for domain time but enough for _current_thread thread amount budget(no need to charge) */
			if (is_cur_domain_expired()) 
			{
				reprogram = TRUE;
				reschedule_required();
				return FALSE;
			}
			/* budget is enough */
			return TRUE;
		}
		
		/* budget is not enough for _current_thread thread amount budget so need charge the budget */
		if (get_thread_action(_current_thread) == HARD_PRIOR_ACTION)
		{
			charge_budget(capacity, consume_time, TRUE);
		}
		else
		{
			charge_budget(capacity, consume_time, FALSE);
		}

		/* consume time means a thread from start to end, elapse time */
		/* so at the thread end, this needs to reset */
		consume_time = 0llu;

		return FALSE;
	}
	
	/* Add support for direct jump scheduling and preset sched_prior scheduling */

	return TRUE;
}

/* consume time reset 0 or this function apply to normal confd, also called budget is enough to consume */
/* so this function maybe call after check_budget(TRUE) or if you think the consume time is small to budget amount */
/* at thread start */
/* when switch to a new thread, you need start a new thread time reset */
void commit_time(void)
{
	if (consume_time > 0) 
	{
		/* if this function is called the head refill must be sufficient to
		 * charge ksConsumed */
		assert(refill_sufficient(_current_thread->sched, consume_time));
		/* and it must be ready to use */
		assert(refill_ready(_current_thread->sched));
	
		if (is_roundrobin(_current_thread->sched)) 
		{
			/* for round robin record_threads, there are only two refills: the HEAD, which is what
			 * we are consuming, and the tail, which is what we have consumed */
			assert(refill_size(_current_thread->sched) == MIN_REFILLS);
			REFILL_HEAD(_current_thread->sched).refill_amount -= consume_time;
			REFILL_TAIL(_current_thread->sched).refill_amount += consume_time;
		}
		else 
		{
			/* the consume time must be small to amount */
			refill_split_check(consume_time);
		}
		
		assert(refill_sufficient(_current_thread->sched, 0));
		assert(refill_ready(_current_thread->sched));
	}
	
	_current_thread->sched->consumed += consume_time;
	
	if (CONFIG_NUM_DOMAINS > 1) 
	{
		assert(current_domain_time > consume_time);
		assert(current_domain_time - consume_time >= MIN_BUDGET);
		current_domain_time -= consume_time;
	}

	/*re-set 0*/
	consume_time = 0llu;
}


/* Everything check_budget does, but also set the thread
 * state to ThreadState_Restart. To be called from kernel entries
 * where the operation should be restarted once the _current_thread thread
 * has budget again.
 */
/* at thread exit */
/* when the thread exec a time, and need exit or switch another thread, 
   you need chenck the budget and consume */
/* the thread exit when syscall(include yield) ; so you need at their handle check budget */
bool_t check_budget_restart(void)
{
	bool_t result = TRUE;

	/* means of guaranteeing a deadline that the first */
	result = check_budget();
	/* budget is not enough , but maybe thread_restart */
	if (!result && is_runnable(_current_thread)) 
	{
		set_thread_state(_current_thread, state_restart_state); /*thread_restart*/
	}
	
	return result;
}

/* enqueue scheduler_action to ready_queue and set choose a new thread */
void reschedule_required(void)
{
	/** Since the scheduler behavior needs to be updated, the existing 
	    scheduler behavior variables need to be queued */
	if (scheduler_action != SCHEDULER_ACTION_RESUME_CURRENT_THREAD
		&& scheduler_action != SCHEDULER_ACTION_CHOOSE_NEW_THREAD)
	{
		if (is_time_sensitived(scheduler_action))
		{
			assert(is_schedulable(scheduler_action));
			assert(refill_sufficient(scheduler_action->sched, 0));
			assert(refill_ready(scheduler_action->sched));
		}

		marktcb_as_queued(scheduler_action);
		sched_enqueue(scheduler_action);
	}
	   
	scheduler_action = SCHEDULER_ACTION_CHOOSE_NEW_THREAD;
}

/* Note that this thread will possibly continue at the end of this kernel
 * entry. Do not queue it yet, since a queue+unqueue operation is wasteful
 * if it will be picked. Instead, it waits in the 'scheduler_action' site
 * on which the scheduler will take action. */
/* also called 'set candidate thread' */
/* so the candidate thread state only not inactive */
void possible_switchto(struct ktcb *thread)
{	
	if (thread != NULL && is_thread_ready(thread))
	{
		if (current_domain != thread->base.domain) 
		{
			move_to_end_ready_q(thread);
		} 
		else if (scheduler_action != SCHEDULER_ACTION_RESUME_CURRENT_THREAD) /* this _current_thread thread means thread para */
		{
			/* Too many record_threads want special treatment, use regular queues. */
			move_to_end_ready_q(thread);
			reschedule_required();
		} 
		else 
		{
			/* sched domain is the same to current_domain and scheduler_action is resume _current_thread */
			/* but this time is not apply to resume _current_thread because of sched != curent_sc */
			scheduler_action = thread;
		}
	}
}

/* awake process specials the release queue of thread , and need to add the released thread to sched queue 
   and set to candidate thread(also called timeout thread or other not sched ready thread) */
/* each exec , all release thread */

/* unpend all */
void awaken(void)
{
	while (release_queue != NULL && refill_ready(release_queue->sched)) 
	{
		struct ktcb *awakened;

		/* dequeue a thread of the first item and its time budget is minimum */
		awakened = release_dequeue();
		
		/* the currently running thread cannot have just woken up */
		assert(awakened != _current_thread);
		/* round robin record_threads should not be in the release queue */
		assert(!is_roundrobin(awakened->sched));
		/* record_threads HEAD refill should always be > MIN_BUDGET */
		assert(refill_sufficient(awakened->sched, 0));

		possible_switchto(awakened);
		/* changed head of release queue -> need to reprogram */
		reprogram = TRUE;
	}
}

sword_t postpone_cur_irqlock(word_t key)
{
	postpone_from_ready_q(_current_thread);
	reschedule_required();
	schedule();
	return swap_thread_irqlock(key);
}

sword_t postpone_cur(spinlock_t *thread_swap_lock, spinlock_key_t key)
{
	postpone_from_ready_q(_current_thread);
	reschedule_required();
	schedule();
	return swap_thread(thread_swap_lock, key);
}

static s32_t init_schedule_object_module(struct device *dev)
{
	dom_t  dom_index;
	prio_t prior_index;

	ARG_UNUSED(dev);
	for(dom_index = 0; dom_index < CONFIG_NUM_DOMAINS; dom_index ++)
	{
		domain_schedule[dom_index].domain = dom_index;
		domain_schedule[dom_index].length = domain_length[dom_index];
	}

	for(prior_index = 0; prior_index < NUM_READY_QUEUES; prior_index ++)
	{
		ready_queues[prior_index].head = NULL;
		ready_queues[prior_index].tail = NULL;
	}

	scheduler_action   = SCHEDULER_ACTION_RESUME_CURRENT_THREAD;
	_current_thread    = NULL;
	release_queue	   = NULL;
	reprogram		   = FALSE;
	consume_time	   = 0llu;
	current_time	   = get_current_tick();

	return 0;
}

SYS_INIT(init_schedule_object_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
