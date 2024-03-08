#include <object/tcb.h>
#include <object/ipc.h>
#include <sys/math_extras.h>
#include <sys/util.h>
#include <model/atomic.h>
#include <api/syscall.h>
#include <kernel/thread.h>
#include <kernel/boot.h>
#include <sys/string.h>
#include <sys/stdbool.h>
#include <model/spinlock.h>
#include <state/statedata.h>
#include <drivers/timer/system_timer.h>
#include <object/cnode.h>
#include <object/anode.h>
#include <arch/thread.h>
#include <kernel/time.h>
#include <model/entryhandler.h>
#include <model/sporadic.h>
#include <sys/util.h>
#include <api/errno.h>
#include <sys/assert.h>
#include <default/default.h>
#include <object/objecttype.h>

static spinlock_t thread_conf_lock;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

static word_t find_thread(word_t tid, word_t from, word_t to)
{
	if(from == to || GLOBALID_TO_TID(record_threads[from]->thread_id) >= tid)
		return(from);

	while(from <= to)
	{
		if((to - from) <= 1)
			return(to);
		
		word_t mid = from + ((to - from) >> 1);

		if(GLOBALID_TO_TID(record_threads[mid]->thread_id) > tid)
			to = mid;
		else if(GLOBALID_TO_TID(record_threads[mid]->thread_id) < tid)
			from = mid;
		else
			return(mid);
	}
	return(0);
}

static void add_to_threads(word_t tid, struct ktcb *thread)
{
	if(!record_thread_count) 
	{
		record_threads[record_thread_count++] = thread;
	}
	else
	{
		word_t i = find_thread(tid, 0, record_thread_count);
		word_t j   = record_thread_count;

		for(; i <= j; j--)
		{
			record_threads[j] = record_threads[j - 1];
		}

		record_threads[i] = thread;
		record_thread_count++;
	}
}

static void delete_from_threads(word_t tid)
{
	/* reserve record_thread_count 0 - idle */
	if(record_thread_count == 1) 
	{
		record_thread_count --;
	}
	else
	{
		word_t i = find_thread(tid, 0, record_thread_count);

		record_thread_count --;

		for(; i < record_thread_count; i++)
		{
			record_threads[i] = record_threads[i + 1];
		}
	}
}

struct ktcb *get_thread(word_t id)
{
	word_t target_id = GLOBALID_TO_TID(id);
	struct ktcb *target_thread = NULL;
	word_t map_index = 0;

	switch (target_id)
	{
		case id_idle_id:
			target_thread = &idle_thread;
			break;
		case id_main_id:
			target_thread = &main_thread;
			break;
		case id_privilege_id:
			target_thread = &privilege_thread;
			break;
		case id_scheduler_id:
			break;
		case id_spacer_id:
			break;
		default:
			map_index = find_thread(target_id, 0, record_thread_count);
			if (record_threads[map_index]->thread_id != id)
			{
				target_thread = NULL;
			}
			else
			{
				target_thread = record_threads[map_index];
			}
			break;
	}

	return target_thread;
}

#if defined(CONFIG_THREAD_MONITOR)
void thread_foreach(thread_user_cb_t user_cb, void *user_data)
{
	struct ktcb *thread;

	assert_info(user_cb != NULL, "user_cb can not be NULL");

	/*
	 * Lock is needed to make sure that the _kernel.record_threads is not being
	 * modified by the user_cb either directly or indirectly.
	 * The indirect ways are through calling ktcb_create and
	 * thread_abort from user_cb.
	 */

	LOCKED(&thread_conf_lock)
	{
		for (thread = _kernel.monitor_thread; thread != NULL; thread = thread->monitor_thread_next) 
		{
			user_cb(thread, user_data);
		}
	}
}

/*
 * Remove a thread from the kernel's list of active record_threads.
 */
void thread_foreach_exit(struct ktcb *thread)
{
	LOCKED(&thread_conf_lock)
	{
		if (thread == _kernel.monitor_thread) 
		{
			_kernel.monitor_thread = _kernel.monitor_thread->monitor_thread_next;
		} 
		else 
		{
			struct ktcb *prev_thread = _kernel.monitor_thread;

			while ((prev_thread != NULL) && (thread != prev_thread->monitor_thread_next)) 
			{
				prev_thread = prev_thread->monitor_thread_next;
			}
			
			if (prev_thread != NULL) 
			{
				prev_thread->monitor_thread_next = thread->monitor_thread_next;
			}
		}
	}
}
#endif

bool_t is_thread_in_isr(void)
{
	return arch_is_in_isr();
}

#if defined(CONFIG_THREAD_NAME)
void thread_set_name(struct ktcb *thread, const string value)
{
	if (thread == NULL) 
	{
		thread = _current_thread;
	}
	
	strncpy(thread->name, value, CONFIG_THREAD_MAX_NAME_LEN);
	thread->name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';
}
const string thread_get_name(struct ktcb *thread)
{
	return (const string)thread->name;
}
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS)
void thread_busy_wait(word_t usec_to_wait)
{
#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
	/* use 64-bit math to prevent overflow when multiplying */
	word_t cycles_to_wait = (word_t)(
		(u64_t)usec_to_wait *
		(u64_t)sys_clock_hw_cycles_per_sec() /
		(u64_t)USEC_PER_SEC
	);

	word_t start_cycles = get_cycle_32();

	for (;;)
	{
		word_t current_cycles = get_cycle_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait)
		{
			break;
		}
	}
#else
	arch_busy_wait(usec_to_wait);
#endif
}
#endif

#if defined(CONFIG_STACK_SENTINEL)
/* Check that the stack sentinel is still present
 *
 * The stack sentinel feature writes a magic value to the lowest 4 bytes of
 * the thread's stack when the thread is initialized. This value gets checked
 * in a few places:
 *
 * 1) In k_yield() if the _current_thread thread is not swapped out
 * 2) After servicing a non-int_nest_count interrupt
 * 3) In swap_thread(), check the sentinel in the outgoing thread
 *
 * Item 2 requires support in arch/ code.
 *
 * If the check fails, the thread will be terminated appropriately through
 * the system fatal error handler.
 */
void check_stack_sentinel(void)
{
	word_t *stack;
	
	if (get_thread_state(_current_thread, state_dummy_state) != 0) 
	{
		return;
	}

	stack = (word_t *)_current_thread->stack_info.start;
	
	if (*stack != STACSENTINEL) 
	{
		/* Restore it so further checks don't trigger this same error */
		*stack = STACSENTINEL;
		catch_exception(fatal_stack_check_fatal);
	}
}
#endif

#if !defined(CONFIG_STACK_POINTER_RANDOM)
size_t adjust_stack_size(size_t stack_size)
{
	return stack_size;
}
#else
word_t stack_adjust_initialized;
size_t adjust_stack_size(size_t stack_size)
{
	size_t random_val;
	
	if (!stack_adjust_initialized)
	{
		set_random32_canary_word((u8_t *)&random_val, sizeof(random_val));
	} 
	else
	{
		set_random32((u8_t *)&random_val, sizeof(random_val));
	}

	/* Don't need to worry about alignment of the size here,
	 * arch_new_thread() is required to do it.
	 *
	 * FIXME: Not the best way to get a random number in a range.
	 * See #6493
	 */
	const size_t fuzz = random_val % CONFIG_STACK_POINTER_RANDOM;

	if (unlikely(fuzz * 2 > stack_size))
	{
		return stack_size;
	}

	return stack_size - fuzz;
}
#endif

/* These spinlock assertion predicates are defined here because having
 * them in spinlock.h is a giant header ordering headache.
 */
#if defined(CONFIG_SPIN_VALIDATE) 
bool_t is_not_spinlock(spinlock_t *spin_lock)
{
	uintptr_t thread_cpu = spin_lock->thread_cpu;

	if (thread_cpu) 
	{
		if ((thread_cpu & THREAD_CPU_MASK) == _current_cpu->core_id) 
		{
			return false;
		}
	}
	return true;
}

bool_t is_spinlock_unlock(spinlock_t *spin_lock)
{
	if (spin_lock->thread_cpu != (_current_cpu->core_id | (uintptr_t)_current_thread)) 
	{
		return false;
	}
	
	spin_lock->thread_cpu = 0;
	return true;
}

void set_spinlock(spinlock_t *spin_lock)
{
	spin_lock->thread_cpu = _current_cpu->core_id | (uintptr_t)_current_thread;
}
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
void disable_float_mode(struct ktcb *thread)
{
	arch_float_disable(thread);
}
#endif

#if defined(CONFIG_IRQ_OFFLOAD) 
void thread_irq_offload(irq_offload_routine_t routine, void *parameter)
{
	arch_irq_offload(routine, parameter);
}
#endif


static void update_consumed(struct ktcb *thread)
{
	if (thread)
	{
		ticks_t consumed = thread->sched->consumed;
		/* overflow */
		if (consumed >= get_max_ticks())
		{
			thread->sched->consumed -= get_max_ticks();
		}
		else
		{
			thread->sched->consumed = 0;
		}
	}
}

static void cancel_yield(struct ktcb *thread)
{
    if (thread && thread->yield)
	{
		thread->yield = NULL;
    }
}

static void set_base(thread_base_t *base, word_t state, word_t option)
{
	base->option = (byte_t)option;
	base->thread_state.obj_state = (word_t)state;
	base->sched_prior = 0u;
	base->sched_locked = 0u;
	base->mcp = 0u;
	base->domain = 0u;
	base->level = 0u;

#if defined(CONFIG_SMP) 
	base->smp_is_idle = 0u;
	base->smp_cpu_id = 0u;
	base->smp_lock_count = 0u;
#endif

#if defined(CONFIG_SCHED_CPU_MASK)
	base->smp_cpu_mask = -1;
#endif

	base->swap_data = NULL;
}


/* thread tcb init, some set need in thread create process (fixed) */
struct ktcb *thread_init(struct ktcb *thread, s8_t *stack_start, size_t stack_size, word_t options)
{
	assert(thread);

#if !defined(CONFIG_INIT_STACKS) && !defined(CONFIG_THREAD_STACK_INFO)
	ARG_UNUSED(stack_start);
	ARG_UNUSED(stack_size);
#endif

#if defined(CONFIG_INIT_STACKS) 
	memset(stack_start, 0xAA, stack_size);
#endif

#if defined(CONFIG_STACK_SENTINEL) 
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	 
	*((word_t *)stack_start) = STACSENTINEL;
#endif

	/* Initialize various struct ktcb members */
	set_base(&thread->base, state_restart_state, options);

	/* static record_threads overwrite it afterwards with real value */
	thread->abort_handle = NULL;

#ifdef CONFIG_ERRNO
	thread->errno_var = 0;
#endif

#ifdef CONFIG_THREAD_NAME
	thread->name[0] = '\0';
#endif

#if defined(CONFIG_THREAD_STACK_INFO)
	thread->stack_info.start = (uintptr_t)stack_start;
	thread->stack_info.size = (word_t)stack_size;
#endif

	thread->ready_q_next = NULL;
	thread->ready_q_prev = NULL;
	thread->mesg_q_next = NULL;
	thread->mesg_q_prev = NULL;
	thread->yield = NULL;
	
	thread->notifation_node = NULL;

	return(thread);
}

/* thread tcb deinit */
void thread_deinit(struct ktcb *thread)
{
	assert(thread != NULL);

	delete_from_threads(GLOBALID_TO_TID(thread->thread_id));
}

FUNC_NORETURN void thread_user_mode_enter(ktcb_entry_t entry,
			void *p1, void *p2, void *p3)
{
	_current_thread->base.option |= option_user_option;
	_current_thread->base.option &= ~option_essential_option;

#if defined(CONFIG_THREAD_MONITOR) 
	_current_thread->entry.entry_ptr = entry;
	_current_thread->entry.para1 = p1;
	_current_thread->entry.para2 = p2;
	_current_thread->entry.para3 = p3;
#endif

#if defined(CONFIG_USERSPACE) 
	arch_user_mode_enter(entry, p1, p2, p3);
#else
	/* In this case we do not reset the stack */
	thread_entry_point(entry, p1, p2, p3);
#endif
}

/* need some mem */
/* make the thread s utcb 'grant', and add it to the thread s as */
struct ktcb *thread_alloc(word_t id)
{
	if(!is_user_vaild(id)) 
	{
		return (NULL);
	}
	
	if (id == _current_thread->thread_id)
	{
		return (_current_thread);
	}
	
	struct ktcb *new_thread = (struct ktcb *)d_object_alloc(obj_thread_obj, 0);
	struct thread_sched *new_sched = (struct thread_sched *)d_object_alloc(obj_sched_context_obj, 0);
	
	if (!new_thread || !new_sched)
	{
		return (NULL);
	}
	
	new_thread->thread_id = id;
	new_thread->sched = new_sched;
	add_to_threads(GLOBALID_TO_TID(id), new_thread);

	return new_thread;
}

void thread_free(struct ktcb *thread)
{
	d_object_free(thread);
	d_object_free(thread->sched);
}

static void update_context_schedule(struct ktcb *thread)
{
	assert(thread != NULL);

	/* get out other blocked state and not vaild parameter of sched */
    thread_resume(thread);

    if (is_runnable(thread) && thread != _current_thread) 
	{
        possible_switchto(thread);
    }
    else if (IS_ENABLED(CONFIG_SMP) && !is_thread_queued(thread)) 
	{
        sched_enqueue(thread);
    } 
	
    if (thread == _current_thread) 
	{
        reschedule_required();
    }
}

/* Except for manual state changes and SMP state changes during task switching, 
   the state remains unchanged */
void set_schedule_context(struct ktcb *thread, ticks_t budget, ticks_t period, 
									word_t max_refills)
{
	assert(thread != NULL && thread->sched != NULL);
	
    /* don't modify parameters of tcb while it is in a sorted queue */
	if (is_schedulable(thread))
	{
		/* remove from scheduler */
		release_remove(thread);
		sched_dequeue(thread);
		
		/* bill the _current_thread consumed amount before adjusting the params */
		if (_current_thread == thread) 
		{
			/* check_budget(void) at the old thread end, commit_time(void) at the new thread start */
			/* so, if _current_thread == thread, the two functon should use togther */
			if (check_budget()) 
			{
				commit_time();
			}
		}
	}
	
    if (budget == period) 
	{
        /* this is a cool hack: for round robin, we set the
         * period to 0, which means that the budget will always be ready to be refilled
         * and avoids some special casing.
         */
        period = 0;
        max_refills = MIN_REFILLS;
    }

    if (is_schedulable(thread)) 
	{
        /* the scheduling context is active - it can be used, so
         * we need to preserve the bandwidth */
        refill_update(thread->sched, period, budget, max_refills);
    } 
	else if (IS_ENABLED(CONFIG_SMP) && !is_thread_not_running(thread))
	{
        /* the scheduling context is active - it can be used, so
         * we need to preserve the bandwidth */
        refill_update(thread->sched, period, budget, max_refills);
	}
	else
	{
        /* the scheduling context isn't active - it's budget is not being used, so
         * we can just populate the parameters from now */
        refill_new(thread->sched, max_refills, budget, period);
    }

    if (thread->sched->refill_max > 0) 
	{
		update_context_schedule(thread);
    }
}

void set_thread_state(struct ktcb *thread, word_t ts)
{
	assert(thread != NULL);

    thread->base.thread_state.obj_state = ts;
	/* if change _current_thread thread base.thread_state */
	schedule_tcb(thread);
}

void set_thread_state_object(struct ktcb *thread, uintptr_t object)
{
	assert(thread != NULL);
	thread->base.thread_state.obj_ptr = object;
}

uintptr_t get_thread_state_object(struct ktcb *thread)
{
	assert(thread != NULL);
	return thread->base.thread_state.obj_ptr;
}

word_t get_thread_state(struct ktcb *thread, word_t ts)
{
	assert(thread != NULL);

	return thread->base.thread_state.obj_state & ts;
}

word_t get_thread_object_state(struct ktcb *thread)
{
	assert(thread != NULL);

	return thread->base.thread_state.obj_state;
}

/* some set need after thread create process or runtime change */
void set_domain(struct ktcb *thread, dom_t dom)
{
	assert(thread != NULL);
	
    if (is_schedulable(thread))
	{
	    sched_dequeue(thread);
    	thread->base.domain = dom;
        sched_enqueue(thread);
    }
	else
	{
		thread->base.domain = dom;
	}

    if (thread == _current_thread)
	{
        reschedule_required();
    }
}

void set_thread_action(struct ktcb *thread, word_t action)
{
	assert(thread != NULL);
	thread->base.level = action;
}

word_t get_thread_action(struct ktcb *thread)
{
	assert(thread != NULL);
	return thread->base.level;
}

/*
 * Note:
 * The caller must guarantee that the stack_size passed here corresponds
 * to the amount of stack memory available for the thread.
 */
void set_new_thread(struct ktcb *new_thread, struct thread_stack *stack, 
						size_t stack_size, ktcb_entry_t entry,
		       			void *p1, void *p2, void *p3, word_t options)
{
#if defined(CONFIG_USERSPACE) 
	new_thread->userspace_stack_point = stack;
	k_object_access_grant(new_thread, new_thread);
#endif
	stack_size = adjust_stack_size(stack_size);
	arch_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3, options);
#if defined(CONFIG_THREAD_MONITOR) 
	LOCKED(thread_conf_lock)
	{
		new_thread->monitor_thread_function.entry_ptr  = entry;
		new_thread->monitor_thread_function.para1  = p1;
		new_thread->monitor_thread_function.para2  = p2;
		new_thread->monitor_thread_function.para3  = p3;
		new_thread->monitor_thread_next	= _kernel.monitor_thread;
		_kernel.monitor_thread  = new_thread;
	}
#endif
#if defined(CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN) 
	/* _current_thread may be null if the dummy thread is not used */
	if (!_current_thread) 
		return;
#endif
#if defined(CONFIG_USERSPACE) 
	/* New threads inherit any memory domain membership by the parent */
	if (_current_thread->userspace_fpage_table.pagetable_item != NULL) 
		add_to_page_table(_current_thread->userspace_fpage_table.pagetable_item, new_thread);
	else
		new_thread->userspace_fpage_table.pagetable_item = NULL;
#endif
}


void prepare_start_thread(struct ktcb *thread)
{
	/* not restart */
	if (!has_thread_started(thread)) 
	{
		/* set start */
		marktcb_as_started(thread);
		
		/* enqueue */
		if (!is_thread_queued(thread) && is_thread_ready(thread)) 
		{
			add_to_ready_q(thread);
		}
	}
}

struct ktcb *thread_create(struct ktcb *new_thread, struct thread_stack *stack, 
						size_t stack_size, ktcb_entry_t entry, 
						void *p1, void *p2, void *p3, word_t options)
{
  	assert_info(!is_thread_in_isr(), "Threads may not be created in ISRs");
  /* Special case, only for unit tests */
#if defined(CONFIG_TEST) && defined(CONFIG_ARCH_HAS_USERSPACE) && !defined(CONFIG_USERSPACE)
	assert_info((options & option_user_option) == 0,
	"Platform is capable of user mode, and test thread created with option_user_option option,"
	" but neither CONFIG_TEST_USERSPACE nor CONFIG_USERSPACE is set\n");
#endif

	set_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3, options);
	prepare_start_thread(new_thread);
	
  	return new_thread;
}

/* user thread destroy */
void thread_destroy(struct ktcb *thread)
{
	assert(thread != NULL);
	if (is_user_vaild(thread->thread_id)) 
	{
		cancel_yield(thread);
		cancel_ipc(thread);
		cancel_signal(thread, thread->notifation_node);
		
		/*remove from schedule queue*/
		if (is_thread_queued(thread))
		{
			marktcb_as_not_queued(thread);
			sched_dequeue(thread);
			release_remove(thread);
		}
	
		thread_deinit(thread);
		if (thread == _current_thread)
		{
			reschedule_required();
		}
	}
}

void schedule_tcb(struct ktcb *thread)
{
	/* when change _current_thread thread */
    if (thread == _current_thread &&
    scheduler_action == SCHEDULER_ACTION_RESUME_CURRENT_THREAD &&
    !is_schedulable(thread)) 
    {
    	reschedule_required();
	}
}

/* after switch next thread to exec - right now */
void thread_activate(void)
{
	/* the old thread is over, and the new thread is start, 
	   so the old thread sched consumed should be reset */
    if (_current_thread->yield) 
	{
		/* complete yield */
        update_consumed(_current_thread->yield);

		cancel_yield(_current_thread);
		
        assert(is_thread_running(_current_thread)     == true);
    }
}

/* this is the 'soft' way to check the thread s refills is ok */
/* another 'hard' way is the 'assert'*/
/* if sched not ready thread(include timeout thread) ,add a release queue */
void thread_resume(struct ktcb *thread)
{
    assert(thread != NULL);
	/* assert(thread->sched != NULL); */
	assert(get_thread_object_state(thread) != state_dummy_state);

	if (is_thread_pending(thread)) 
	{
		return;
	}

    if (is_thread_ready(thread)) 
	{
		/* budget is not enough */
		if (!(refill_ready(thread->sched) && refill_sufficient(thread->sched, 0))) 
		{
			postpone_from_ready_q(thread);
		}
    }

	marktcb_as_not_suspended(thread);

#ifdef CONFIG_SMP
	if (!IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) 
	{
		sched_ipi();
	}
#endif
	/* if arch_sched_ipi */
	
	/* neeed resume interrupt */
	if (!is_thread_in_isr()) 
	{
		reschedule_unlocked();
	}

	reschedule_required();
}

void thread_suspend(struct ktcb *thread)
{
	assert(thread != NULL);

    cancel_ipc(thread);
	cancel_yield(thread);

    if (get_thread_state(thread, state_queued_state)    != 0)
	{
        /* whilst in the running base.thread_state it is possible that thread_restart pc of a thread is
         * incorrect. As we do not know what base.thread_state this thread will transition to
         * after we make it inactive we update its thread_restart pc so that the thread next
         * runs at the correct address whether it is restarted or moved directly to
         * running */
         /* set_restartPC(thread); */
    }

    set_thread_state(thread, state_suspended_state);
	
	/* remove from the ready or release queue */
	if (is_thread_ready(thread))
	{
		marktcb_as_not_queued(thread);
		sched_dequeue(thread);
		release_remove(thread);
	}

	/* neeed resume interrupt */
	if (!is_thread_in_isr()) 
	{
		reschedule_unlocked();
	}

	if (thread == _current_thread) 
	{
		reschedule_required();
	} 
}

/* thread_restart still at ready queue */
void thread_restart(struct ktcb *thread)
{
	assert(thread != NULL);

    if (is_blocked(thread))
	{
        cancel_ipc(thread);
        set_thread_state(thread, state_restart_state);
		prepare_start_thread(thread);
		/* the thread is thread_restart base.thread_state, and maybe the thread base.thread_state to running */
        thread_resume(thread);
    }
}

void thread_single_abort(struct ktcb *thread)
{
	assert(thread != NULL);

	if (thread->abort_handle) 
	{
		thread->abort_handle();
	}

#ifdef CONFIG_SMP
	sched_abort(thread);
#endif

	if (is_thread_ready(thread)) 
	{
		remove_from_ready_q(thread);
	} 

	set_thread_state(thread, state_dead_state);

#if defined(CONFIG_USERSPACE) 
	/* Revoke permissions on thread's ID so that it may be recycled */
	k_object_access_revoke(thread, thread);
#endif
}

/* Yield is used for tasks of cannot switch context.
   A measure for the task to give up the scheduler actively 
   of _current_thread thread, switch to another equal or higher sched_prior thread */
/* 'yield' use before: 1. set_threadbase.thread_state(_current_thread, state_restart_state); 2. set_threadbase.thread_state(thread, state_queued_state);*/
void thread_yield(struct ktcb *thread)
{
	assert(thread != NULL);
	assert(!arch_is_in_isr());

	/* CHECK IS Completed */
	if (thread->yield)
	{
	    /* complete yield */
        update_consumed(thread->yield);
		cancel_yield(thread);
        assert(thread->yield == NULL);
    }

    /* if the tcb is in the scheduler, it's ready and sufficient.
     * Otherwise, check that it is ready and sufficient and if not,
     * place the thread in the release queue. This way, from this point,
     * if the thread is_Schedulable, it is ready and sufficient.*/
    thread_resume(thread);

    bool_t return_now = true;
	
	LOCKED(&thread_conf_lock) 
	{
		if (is_schedulable(thread))
		{
			refill_noblock_check(thread->sched);

			/* canot be schedule */
			if (thread->base.sched_prior < _current_thread->base.sched_prior) 
			{
				sched_dequeue(thread);
				sched_enqueue(thread);
			} 
			/* can be schedule */
			else
			{
				_current_thread->yield = thread;
				sched_dequeue(thread);
				if (!IS_ENABLED(CONFIG_SMP))
				{
					sched_dequeue(_current_thread);
				}
				sched_enqueue(_current_thread);
				sched_enqueue(thread);
				reschedule_required();
				/* we are scheduling the thread associated with sched,
				 * so we don't need to write to the ipc buffer
				 * until the _current_thread is scheduled again */
				return_now = false;
			}
		}
	}

	/* the thread canot be schedule still, so need reset the thread sched consumed */
    if (return_now) 
	{
        update_consumed(thread);
    }
}

/* Only the thread sched is NULL */
void thread_donate(struct ktcb *src, struct ktcb *dest)
{
	assert(src != NULL);
	assert(dest != NULL);

	if (is_schedulable(src) && !is_schedulable(dest))
	{
		sched_dequeue(src);
		marktcb_as_not_queued(src);
		src->sched  = NULL;
	}

	if (src == _current_thread)
	{
		reschedule_required();
	}

	if (dest->sched->refill_max > 0) 
	{
		update_context_schedule(dest);
	}
}

/*  ----------------------------  */

/* L4_ExchangeRegisters : read/exchange kernel registers syscall function
  Exchanges or reads a thread FLAGS, SP, and IP hardware registers well pager and UserDefinedHandle TCRs.
  Furthermore, thread execution can be suspended or resumed. The destination thread must be an active thread (see thread_page 23)
  residing in the invoker address space.

  [Systemcall]
  output:
  	1. result(return)
    2. old control:
    3. old sp:_current_thread user-level stack pointer
    4. old ip:_current_thread user-level instruction pointer
    5. old flag:user-level processor flag of the thread
    6. old userdefinedhandle:thread UserDefinedHandle
    7. old pager:thread pager

  input:
    1. dest:Thread ID of the addressed thread. This may be a local or a global ID. However, the addressed
            thread must reside in the _current_thread address space.
    2. control:
    3. sp:_current_thread user-level stack pointer
    4. ip:_current_thread user-level instruction pointer
    5. flag:user-level processor flag of the thread
    6. userdefinedhandle:thread UserDefinedHandle
    7. pager:thread pager
    
  pagefaults:
    none

  ErrorCode:
  	2
  */



/* read : _current_thread thread ; exchange : dest thread */
exception_t syscall_exchange_registers(
	word_t  dest_thread_id,
	word_t  control,
	word_t  *sp,
	word_t  *ip,
	word_t  *flag)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		struct ktcb *dest_thread = get_thread(dest_thread_id);
#if(0)
		if (inv_level != exchange_registers)
		{
			user_error("THREAD Object: Illegal operation attempted.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
#if defined(CONFIG_USERSPACE)
#endif
	
		if (!is_user_vaild(dest_thread_id) || !dest_thread)
		{
			user_error("THREAD Object: Illegal operation parameter.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if (is_thread_queued(dest_thread))
		{
			sched_dequeue(dest_thread);
			/*
			if(CONTROL_BIT_MASK(control, CONTROL_BIT_p))
			if(CONTROL_BIT_MASK(control, CONTROL_BIT_u))
			*/
			if (CONTROL_BIT_MASK(control, CONTROL_BIT_f))
				((word_t *)(dest_thread->callee_saved.psp))[15] = *flag;
			if (CONTROL_BIT_MASK(control, CONTROL_BIT_i))
				((word_t *)(dest_thread->callee_saved.psp))[14] = *ip;
			if (CONTROL_BIT_MASK(control, CONTROL_BIT_s))
				dest_thread->callee_saved.psp = *sp;
			
			sched_enqueue(dest_thread);
	
			if (dest_thread == _current_thread)
			{
				reschedule_required();
			}
		}
		
		if (CONTROL_BIT_MASK(control, CONTROL_BIT_S) || 
			CONTROL_BIT_MASK(control, CONTROL_BIT_R))
		{
			/* cancel ipc */ 
			current_kernel_status_code = IPC_CANCELED;
			cancel_ipc(dest_thread);
		}
		else if (!CONTROL_BIT_MASK(control, CONTROL_BIT_S))
		{
			/* wait send IPC final */
			if (get_thread_object_state(dest_thread) == state_send_blocked_state)
			{
				message_t *node = (message_t *)get_thread_state_object(dest_thread);
				if (node->state == message_state_recv)
				{
					send_ipc(dest_thread, false, false, node);
				}
			}
		}
		else if (!CONTROL_BIT_MASK(control, CONTROL_BIT_R))
		{
			/* wait receive IPC final */
			if (get_thread_object_state(dest_thread) == state_recv_blocked_state)
			{
				message_t *node = (message_t *)get_thread_state_object(dest_thread);
				if (node->state == message_state_send)
				{
					receive_ipc(dest_thread, false, node);
				}
			}
		}	
	
		if (CONTROL_BIT_MASK(control, CONTROL_BIT_H)) 
		{
			/* thread suspend */
			thread_suspend(dest_thread);
		}
		
		if (CONTROL_BIT_MASK(control, CONTROL_BIT_h))
		{
			/* thread restart */
			thread_restart(dest_thread);
		}
		
		if(CONTROL_BIT_MASK(control, CONTROL_BIT_d))
		{
			*sp = _current_thread->callee_saved.psp;
			*ip = ((word_t *)(_current_thread->callee_saved.psp))[14];
			*flag  = ((word_t *)(_current_thread->callee_saved.psp))[15];
		}
		else
		{
			*sp = UNDEFINE_VALUE;
			*ip = UNDEFINE_VALUE;
			*flag = UNDEFINE_VALUE;
		}
	
	
		schedule();
		reschedule_unlocked();
		
		return EXCEPTION_NONE;

	}

	return EXCEPTION_FAULT;
}

/* L4_ThreadControl : control thread exec syscall function
	A privileged thread, e.g., the root server, can delete and create threads through this function. It can also modify the global
    thread ID (version field only) of an existing thread.
    Threads can be created mem_space active or inactive record_threads. Inactive record_threads do not execute but can be activated by active
    record_threads that execute in the same address space.
    
    An actively created thread starts immediately by executing a short receive operation from its pager. (An active thread
	must have a pager.) The activeted thread expects a start message (MsgTag and two untyped words) from its pager.
	Once it receives the start message, it takes the value of MR1 mem_space its new IP, the value of MR2 mem_space its new SP, and then
	starts execution at user level with the received IP and SP. The new thread will execute on the same processor where the
	activating ThreadControl was invoked.
	Interrupt record_threads are treated mem_space normal record_threads. They are active at system startup and can not be deleted or migrated
	into a different address space (i.e., SpaceSpecifier must be equal to the interrupt thread ID). When an interrupt occurs the
	interrupt thread sends an IPC to its pager and waits for an empty end-of-interrupt acknowledgment message (MR 0=0).
	Interrupt record_threads never raise pagefaults. To deactivate interrupt message delivery, the pager is set to the interrupt thread鈥檚
	own ID.
	
	[Privileged Systemcall]
	output:
	1. result(return)

	input:
	1. dest: Addressed thread. Must be a global thread ID. Only the thread number is effectively used
            to address the thread. If a thread with the specified thread number exists, its version bits are
            overwritten by the version bits of dest id and any ongoing IPC operations are aborted. Otherwise,
            the specified version bits are used for thread creations, i.e., a thread creation generates a thread
            with ID dest.
	2. SpaceSpecifier: != nilthread, dest not existing, create; != nilthread, dest existing, modifiaction
						= nilthread, dest existing, deletion
	3. Scheduler: != nilthread, Defines the scheduler thread that is permitted to schedule the addressed thread. Note that the
				  scheduler thread must exist when the addressed thread starts executing;
				  = nilthread, The _current_thread scheduler association is not modified
	4. Pager:!= nilthread, The pager of dest is set to the specified thread. If dest was inactive before, it is activated
			 = nilthread, The _current_thread pager association is not modified.
	5. UtcbLocation: != -1, The start address of the UTCB of the thread is set to UtcbLocation;
	                  = -1, The UTCB location is not modified.

	pagefaults:
	none

	ErrorCode:
	1 2 3 4 6 8
*/
exception_t syscall_thread_control(
	word_t dest_thread_id,
	word_t dest_space_id,
	word_t dest_schedule,
	word_t dest_pager_id)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

    if (is_sufficient)
    {
		struct ktcb *dest_thread = get_thread(dest_thread_id);
#if(0)
		if (inv_level != thread_control)
		{
			user_error("THREAD Object: Illegal operation attempted.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
#if defined(CONFIG_USERSPACE)
#endif
	
		/* the _current_thread thread must be root */
		if (!is_user_vaild(dest_thread_id)) 
		{
			user_error("THREAD Object: Illegal operation parameter.");
			current_syscall_error_code = TCR_INVAL_THREAD;
			return EXCEPTION_SYSCALL_ERROR;
		}
		
		if (is_user_vaild(dest_schedule))
		{
			user_error("THREAD Object: Illegal operation scheduler.");
			current_syscall_error_code = TCR_INVAL_SCHED;
			return EXCEPTION_SYSCALL_ERROR; 
		}
		
		if (is_user_vaild(dest_space_id))
		{
			user_error("THREAD Object: Illegal operation spacer.");
			current_syscall_error_code = TCR_INVAL_SPACE;
			return EXCEPTION_SYSCALL_ERROR; 
		}
		
		if (dest_space_id != GLOBALID_NILTHREAD)
		{
			if (!dest_thread)
			{
				/* create the thread */
				struct ktcb *new_thread = thread_alloc(dest_thread_id);
				struct pager_context *pager = (struct pager_context *)dest_pager_id;
				
				if (!new_thread)
				{
					user_error("THREAD Object: Illegal range memory.");
					current_syscall_error_code = TCR_OUT_OF_MEM;
					return EXCEPTION_SYSCALL_ERROR; 
				}
				if (new_thread == _current_thread)
				{
					user_error("THREAD Object: Illegal privilege.");
					current_syscall_error_code = TCR_NO_PRIVILIGE;
					return EXCEPTION_SYSCALL_ERROR; 
				}
	
				if (dest_pager_id == GLOBALID_NILTHREAD)
				{
					user_error("THREAD Object: Illegal privilege.");
					current_syscall_error_code = TCR_INVAL_UTCB;
					return EXCEPTION_SYSCALL_ERROR; 
				}
				
				new_thread = thread_create(new_thread, pager->stack_ptr, pager->stack_size, 
					pager->entry, pager->p1, pager->p2, pager->p3, pager->options);
				new_thread->user = pager->virual_user;
			}
			else
			{
				/* modification the thread */
				current_kernel_status_code = IPC_CANCELED;
				cancel_ipc(dest_thread);
				thread_restart(dest_thread);
			}
		}
		else
		{
			/* delete the thread */
			if (!dest_thread)
			{
				user_error("THREAD Object: Illegal operation parameter.");
				current_syscall_error_code = TCR_INVAL_THREAD;
				return EXCEPTION_SYSCALL_ERROR;
			}
			thread_destroy(dest_thread);
			thread_free(dest_thread);
			reschedule_required();
		}
	
		schedule();
		/* reschedule_unlocked(); */
		
		return EXCEPTION_NONE;

	}
	return EXCEPTION_FAULT;
}

/* L4_ThreadSwitch : switch other thread(not pree) syscall function
   The invoking thread releases the processor (non-preemptively) so that another ready thread can be processed. 

   [Systemcall]
   output:
	 none
   input:
     1. dest: = nilthread, Processing switches to an undefined ready thread which is selected by the scheduler. (It might
				be the invoking thread.) Since this is scheduling, the thread gets a new timeslice.
			  != nilthread, If dest is ready, processing switches to this thread. In this scheduling, the invoking
              thread donates its remaining timeslice to the destination thread. (This one gets the donation
              in addition to its ordinarily scheduled timeslices, if any.)
              If the destination thread is not ready or resides on a different processor, the system call operates
              described for dest = nilthread.
   pagefaults:
     none
*/
exception_t syscall_switch_thread(word_t dest_thread_id)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if(0)
		if (inv_level != thread_switch)
		{
			user_error("THREAD Object: Illegal operation switch.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
#if defined(CONFIG_USERSPACE)
#endif

		/* yield self - strong level */
		if (dest_thread_id == GLOBALID_NILTHREAD)
		{
			/* something */
			remove_from_ready_q(_current_thread);
			set_thread_state(_current_thread, state_restart_state);
			reschedule_required();
		}
		/* yield others - weak level */
		else
		{
			struct ktcb *dest_thread = get_thread(dest_thread_id);
			if (!is_user_vaild(dest_thread_id) || !dest_thread) 
			{
				user_error("THREAD Object: Illegal operation parameter.");
				current_syscall_error_code = TCR_INVAL_THREAD;
				return EXCEPTION_SYSCALL_ERROR;
			}

			/* not in queue */
			if (!is_schedulable(dest_thread))
			{
				move_to_end_ready_q(dest_thread);
				reschedule_required();
			}
			else /* in queue - current thread release, and dest thread get change */
			{
				thread_yield(dest_thread);
			}
		}

		schedule();
		reschedule_unlocked();
	
		return EXCEPTION_NONE;

	}
	return EXCEPTION_FAULT;
}

/* L4_Schedule : set scheduler syscall function
   The system call can be used by schedulers to define the sched_prior, timeslice length, and other scheduling parameters of
   threads. Furthermore, it delivers thread states.
   The system call is only effective if the calling thread resides in the same address space the destination thread
   scheduler (see thread control, thread_page 23).
   
   [Systemcall]
   output:
     1. result(return)
     	0 - Error. The operation failed completely. The ErrorCode TCR indicates the reason for the failure.
     	1 - Dead. The thread is unable to execute or does not exist.
     	2 - Inactive. The thread is inactive/stopped.
     	3 - Running. The thread is ready to execute at user-level.
     	4 - Pending send. A user-invoked IPC send operation currently waits for the destination (recipient)
			to become ready to receive.
     	5 - Sending. A user-invoked IPC send operation currently transfers an outgoing message.
     	6 - Waiting to receive. A user-invoked IPC receive operation currently waits for an incoming message.
     	7 - Receiving. A user-invoked IPC receive operation currently receives an incoming message
     2. old_TimeControl
   input:
     1. dest
     2. TimeControl
     3. ProcessorControl
     4. PrioControl
     5. PreemptionControl
     
   pagefaults:
     none

   ErrorCode:
     1 2 5
*/

/* dest_time:budget | period | max_refills */
/* dest_process:processer number */
/* dest_prior:mcp | prior */
/* dest_domain:domain number */

exception_t syscall_schedule_control(
	word_t dest_thread_id,
	word_t dest_time, 
	word_t dest_process, 
	word_t dest_prior, 
	word_t dest_domain)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		ARG_UNUSED(dest_process);
		struct ktcb *dest_thread = get_thread(dest_thread_id);
		
		prio_t sched_prior = GET_UNIT(dest_prior, SCHEDULER_PRIORITY_MASK);
		prio_t mcp = GET_UNIT(dest_prior, SCHEDULER_MCP_MASK) >> 12;
		prio_t level = GET_UNIT(dest_prior, SCHEDULER_LEVEL_MASK) >> 24;
		
		word_t budget	= GET_UNIT(dest_time, SCHEDULER_BUDGET_MASK) >> 20;
		word_t period	= GET_UNIT(dest_time, SCHEDULER_PERIOD_MASK) >> 8;
		word_t refills	= GET_UNIT(dest_time, SCHEDULER_MAX_REFILLS_MASK);
	
#if(0)
		if (inv_level != schedule_control)
		{
			user_error("THREAD Object: Illegal operation attempted - inv_level.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
#if defined(CONFIG_USERSPACE)
#endif
	
		if (!is_user_vaild(dest_thread_id) || !dest_thread)
		{
			user_error("THREAD Object: Illegal operation TCR error.");
			current_syscall_error_code = SCHED_TCR_ERROR;
			return EXCEPTION_SYSCALL_ERROR;
		}
		
		if (sched_prior > mcp || sched_prior > NUM_PRIORITIES || mcp > NUM_PRIORITIES)
		{
			user_error("THREAD Object: Illegal operation attempted - priority.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if (level != HARD_PRIOR_ACTION && level != SOFT_PRIOR_ACTION)
		{
			user_error("THREAD Object: Illegal operation attempted - level %d.", level);
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if (dest_domain > CONFIG_NUM_DOMAINS || refills > NUM_SCHED_REFILLS)
		{
			user_error("THREAD Object: Illegal operation attempted - domain or refills.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if (get_thread_state(dest_thread, state_dummy_state | state_dead_state | state_aborting_state))
		{
			user_error("THREAD Object: Illegal operation thread not exist.");
			current_syscall_error_code = SCHED_THREAD_NOT_EXIST;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if (get_thread_state(dest_thread,	 state_suspended_state | state_restart_state))
		{
			user_error("THREAD Object: Illegal operation thread is inactive.");
			current_syscall_error_code = SCHED_THREAD_INAVTIVE;
			return EXCEPTION_SYSCALL_ERROR;
		}
		
		if (dest_thread == _current_thread)
		{
			user_error("THREAD Object: Illegal operation thread is running.");
			current_syscall_error_code = SCHED_THREAD_RUNNING;
			return EXCEPTION_SYSCALL_ERROR;
		}	
		
		if(get_thread_object_state(dest_thread) == state_send_blocked_state)
		{
			user_error("THREAD Object: Illegal operation thread is running.");
			current_syscall_error_code = SCHED_THREAD_SEND_BLOCKED;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		if(get_thread_object_state(dest_thread) == state_recv_blocked_state)
		{
			user_error("THREAD Object: Illegal operation thread is running.");
			current_syscall_error_code = SCHED_THREAD_RECV_BLOCKED;
			return EXCEPTION_SYSCALL_ERROR;
		}
		
		if(get_thread_object_state(dest_thread) == state_notify_blocked_state)
		{
			user_error("THREAD Object: Illegal operation thread is running.");
			current_syscall_error_code = SCHED_THREAD_NOTIFY_BLOCKED;
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		set_prior(dest_thread, sched_prior, mcp);
		set_thread_action(dest_thread, level);
		set_domain(dest_thread, dest_domain);
		set_schedule_context(dest_thread, budget, period, refills);
		
		reschedule_required();
	
		schedule();
		/* reschedule_unlocked(); */
	
	
		return EXCEPTION_NONE;

	}
	return EXCEPTION_FAULT;
}
