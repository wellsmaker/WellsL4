#include <kernel/time.h>
#include <object/interrupt.h>
#include <state/statedata.h>
#include <object/tcb.h>
#include <kernel/thread.h>
#include <model/spinlock.h>
#include <api/syscall.h>
#include <drivers/timer/system_timer.h>
#include <sys/util.h>
#include <sys/limits.h>
#include <default/default.h>
#include <sys/assert.h>
#include <object/objecttype.h>
#include <api/errno.h>
#include <object/ipc.h>
#include <kernel/thread.h>
#include <kernel/privilege.h>

/* SYSTIMER */
/* para1:delay time;para2:timer handler;para3:thread */
/* handler return:period time */
/* System timer can use exception or interrupt; its function is to provide system tick and create 
   critical periodic or one-time timer events */
/* So you don't need an acceptance test, just execute the handler */

/* DECLARE_KOBJECT(struct timer_event, timer_event_table, CONFIG_MAX_SYSTIMER_EVENTS); */

static spinlock_t time_lock;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

#define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) ? FOREVER : INT_MAX)

/* timer event list */
static timer_list_t timer_event_list = SYS_DLIST_STATIC_INIT(&timer_event_list);

/* timer global var */
static uint64_t current_tick = 0;   /* timer _current_thread tick value */
/* such as in the same time period, create multiple timer events,need elapsing "create process time" */
/* or multiple timer_update functions */
/* Cycles left to process in the currently-executing handler */
static word_t dvalue_elapse  = 0;   /* dvalue of next_node event elapse - dvalue_elapse */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
s32_t clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif

static struct timer_event *first_node(void)
{
	sys_dnode_t *to = sys_dlist_peek_head(&timer_event_list);

	return (to == NULL) ? NULL : CONTAINER_OF(to, struct timer_event, index);
}

static struct timer_event *next_node(struct timer_event *to)
{
	if (to != NULL)
	{
		sys_dnode_t *node = sys_dlist_peek_next(&timer_event_list, &to->index);
		return (node == NULL) ? NULL : CONTAINER_OF(node, struct timer_event, index);
	}

	return NULL;
}

static s32_t elapsed_dvalue(void)
{
	return (dvalue_elapse == 0) ? clock_elapsed() : 0;
}

static s32_t next_dvalue(void)
{
	struct timer_event *to = first_node();
	s32_t ticks_elapsed = elapsed_dvalue();
	
	return (to == NULL) ? MAX_WAIT : MAX(0, to->dvalue - ticks_elapsed);
}

void add_to_timelist(struct timer_event *to, timer_handler_t handler, generptr_t data,
	s32_t ticks)
{
	if (to != NULL && is_inactive_timelist(to))
	{
		LOCKED(&time_lock) 
		{
			struct timer_event *index;
	
			to->handler = handler;
			to->data = data;
			to->dvalue = MAX(1, ticks) + elapsed_dvalue();
			
			for (index = first_node(); index; index = next_node(index)) 
			{
				assert(index->dvalue >= 0);
	
				if (index->dvalue > to->dvalue) 
				{
					index->dvalue -= to->dvalue;
					sys_dlist_insert(&index->index, &to->index);
					break;
				}
				
				to->dvalue -= index->dvalue;
			}
	
			if (index == NULL) 
			{
				sys_dlist_append(&timer_event_list, &to->index);
			}
	
			if (to == first_node()) 
			{
				clock_set_timeout(next_dvalue(), false);
			}
		}
	}
}

void remove_from_timelist(struct timer_event *to)
{
	if (to != NULL && is_active_timelist(to))
	{
		LOCKED(&time_lock) 
		{
			/*
			if (next_node(to) != NULL) 
			{
				next_node(to)->dvalue += to->dvalue;
			}
			*/
			sys_dlist_remove(&to->index);
		}
	}
}

void readd_to_timelist(struct timer_event *to, word_t dvalue)
{
	uint64_t dvalue_curve  = 0;
	word_t dvalue_borrow = 0;       
	struct timer_event *creditor;

	assert(dvalue != 0 && to != NULL);

	/*presupposition*/
	dvalue += dvalue_elapse;
		
	if (dvalue >= first_node()->dvalue)
	{
		for (struct timer_event *index = first_node(); index; index = next_node(index))
		{
			dvalue_curve += index->dvalue;
			dvalue_borrow = dvalue - dvalue_curve;

			if (dvalue <= dvalue_curve + next_node(index)->dvalue)
			{
				sys_dlist_insert(&to->index, &index->index);
				creditor = next_node(index);
				break;
			}
		}
	}
	else
	{
		sys_dlist_insert(&first_node()->index, &to->index);
		dvalue_borrow  = dvalue;
		creditor = first_node();
	}

	/* CONFIG_SYSTIMER_MIN_TICKS is the MAX_EXEC_TIME(timer_handler1,2,...) */
	/* Avoid loss any timer event */
	if (dvalue_borrow < SYSTIMER_MIN_TICKS)
	{
		dvalue_borrow = 0;
	}
	
	creditor->dvalue = dvalue_borrow;
	/* borrow next timer event,so need reduce next timer event dvalue */
	if (creditor) 
	{
		creditor->dvalue -= dvalue_borrow;
	}
}

s32_t add_up_timelist(struct timer_event *to)
{
	s32_t ticks = 0;

	if (to != NULL && is_active_timelist(to))
	{
		LOCKED(&time_lock) 
		{
			for (struct timer_event *index = first_node(); index; index = next_node(index)) 
			{
				ticks += index->dvalue;
				if (to == index) 
				{
					break;
				}
			}
		}

		ticks -= elapsed_dvalue();
	}

	return ticks;
}

s32_t get_next_timelist(void)
{
	s32_t ret = MAX_WAIT;

	LOCKED(&time_lock) 
	{
		ret = next_dvalue();
	}
	
	return ret;
}

void set_next_timelist(s32_t ticks, bool_t idle)
{
	LOCKED(&time_lock) 
	{
		s32_t next_node =   next_dvalue();
		bool_t  sooner     =  (next_node == MAX_WAIT) || (ticks < next_node);
		bool_t  imminent   =  next_node <= 1;

		/* Only set new timeouts when they are sooner than
		 * what we have.  Also don't try to set a timeout when
		 * one is about to expire: drivers have internal logic
		 * that will bump the timeout to the "next_node" tick if
		 * it's not considered to be settable as directed.
		 * SMP can't use this optimization though: we don't
		 * know when context switches happen until interrupt
		 * exit and so can't get the timeslicing clamp folded
		 * in.
		 */
		if (!imminent && (sooner || IS_ENABLED(CONFIG_SMP)))
		{
			clock_set_timeout(ticks, idle);
		}
	}
}


void update_timelist(s32_t ticks)
{
	LOCKED(&time_lock)
	{
		dvalue_elapse = ticks;
		
		/* deadline ticks part */
		while (first_node() != NULL && first_node()->dvalue <= dvalue_elapse) 
		{
			struct timer_event *index   =  first_node();
			s32_t dvalue  =  index->dvalue;
			word_t period;

			current_tick	+= dvalue;
			dvalue_elapse -= dvalue;

			if (index->handler != NULL)
			{
				period = index->handler(index->data);

				if (period != 0)
				{
					/* Period Thread */
					readd_to_timelist(index, period);
				}
				else
				{
					/* Once Thread */
					index->dvalue = 0;
					remove_from_timelist(index);
					d_object_free(index);
				}
			}
		}
	
		/* deadline ticks remaining */
		if (first_node() != NULL) 
		{
			first_node()->dvalue -= dvalue_elapse;
		}
	
		current_tick += dvalue_elapse;
		dvalue_elapse = 0;
		clock_set_timeout(next_dvalue(), false);
		update_timestamp(false); 
	}
}

u64_t get_current_tick(void)
{
	u64_t index = 0U;

	LOCKED(&time_lock)
	{
		index = current_tick + clock_elapsed();
	}
	
	return index;
}

u32_t get_current_tick_32(void)
{
#if defined(CONFIG_TICKLESS_KERNEL) 
	return (u32_t)get_current_tick();
#else
	return (u32_t)current_tick;
#endif
}


static word_t deadline_timeout_handler(generptr_t data)
{
	word_t *deadline_thread_gid = (word_t *)data;
	struct ktcb *deadline_thread = get_thread(*deadline_thread_gid);
	
	if (deadline_thread && is_thread_running(deadline_thread) && 
		get_thread_action(deadline_thread) == HARD_PRIOR_ACTION)
	{
		/* need consider - current_sched exec time vs deadline time */
		user_error("#### HM PROCESS FOR The thread - %d Start: ####", deadline_thread->thread_id);
		/* Start health monitoring task */
		/* Periodic tasks are set as hard real-time in advance, and aperiodic tasks are soft real-time */
		scheduler_action = SCHEDULER_ACTION_CHOOSE_PRIV_THREAD;
		set_privilege_status(priv_health_monitor_priv, priv_ready_priv);

		/* Check for errors in postconditions and update once context */
		/* TBD */
		if (get_thread_object_state(deadline_thread) == state_recv_blocked_state) 
			current_kernel_status_code = IPC_TIMEOUT | IPC_RECV_PHASE;
		if (get_thread_object_state(deadline_thread) == state_send_blocked_state) 
			current_kernel_status_code = IPC_TIMEOUT | IPC_SEND_PHASE;

		return (0);
	}

	marktcb_as_started(deadline_thread);
	add_to_ready_q(deadline_thread);
	reschedule_required();
	schedule();
	reschedule_unlocked();

	return (0);
}

void set_deadline(ticks_t deadline, word_t *deadline_gid)
{
	struct timer_event *new_timer_event = (struct timer_event *)d_object_alloc(obj_time_obj, 0);

	if (new_timer_event != NULL)
	{
		initialize_timelist(new_timer_event);
		
		if (!deadline)
		{
			d_object_free(new_timer_event);
			new_timer_event = NULL;
		}
		else
		{
			add_to_timelist(new_timer_event, deadline_timeout_handler, deadline_gid, deadline);
		}
	}
}

u64_t get_uptime_64(void)
{
	return k_ticks_to_ms_floor64(get_current_tick());
}

exception_t syscall_system_clock(dword_t *clock)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if(0)
		if (inv_level != get_system_clock)
		{
			user_error("TIME Object: Illegal operation attempted.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
#if defined(CONFIG_USERSPACE) 
#endif
	
		*clock = sys_clock_hw_cycles_per_sec();
	
	
		schedule();
		/* reschedule_unlocked(); */
	
		return EXCEPTION_NONE;

	}
	
	return EXCEPTION_FAULT;

}

void init_time_object(void)
{
	clock_init();
}
