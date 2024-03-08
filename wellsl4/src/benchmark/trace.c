#ifdef CONFIG_TRACING

#include <benchmark/trace.h>
#include <object/tcb.h>

void sys_trace_thread_switched_out(void)
{
	struct ktcb *thread = _current_thread;

	trace_thread_switched_out((u32_t)thread->thread_id);
}

void sys_trace_thread_switched_in(void)
{
	struct ktcb *thread = _current_thread;

	trace_thread_switched_in((u32_t)thread->thread_id);
}

void sys_trace_thread_priority_set(struct ktcb *thread)
{
	trace_thread_priority_set((u32_t)thread->thread_id,
				    thread->base.sched_prior);
}

void sys_trace_thread_create(struct ktcb *thread)
{
	string_t name = { "Unnamed thread" };

#if defined(CONFIG_THREAD_NAME)
	const char *tname = thread_get_name(thread);

	if (tname != NULL) 
	{
		strncpy(name.buf, tname, sizeof(name.buf));
		/* strncpy may not always null-terminate */
		name.buf[sizeof(name.buf) - 1] = 0;
	}
#endif

	trace_thread_create(
		(u32_t)thread->thread_id,
		thread->base.sched_prior,
		name
		);

#if defined(CONFIG_THREAD_STACK_INFO)
	trace_thread_info(
		(u32_t)thread->thread_id,
		thread->stack_info.size,
		thread->stack_info.start
		);
#endif
}

void sys_trace_thread_abort(struct ktcb *thread)
{
	trace_thread_abort((u32_t)thread->thread_id);
}

void sys_trace_thread_suspend(struct ktcb *thread)
{
	trace_thread_suspend((u32_t)thread->thread_id);
}

void sys_trace_thread_resume(struct ktcb *thread)
{
	trace_thread_resume((u32_t)thread->thread_id);
}

void sys_trace_thread_ready(struct ktcb *thread)
{
	trace_thread_ready((u32_t)thread->thread_id);
}

void sys_trace_thread_pend(struct ktcb *thread)
{
	trace_thread_pend((u32_t)thread->thread_id);
}

void sys_trace_thread_info(struct ktcb *thread)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	trace_thread_info(
		(u32_t)thread->thread_id,
		thread->stack_info.size,
		thread->stack_info.start
		);
#endif
}

void sys_trace_thread_name_set(struct ktcb *thread)
{
#if defined(CONFIG_THREAD_NAME)
	string_t name = { "Unnamed thread" };
	const char *tname = thread_get_name(thread);

	if (tname != NULL) 
	{
		strncpy(name.buf, tname, sizeof(name.buf));
		/* strncpy may not always null-terminate */
		name.buf[sizeof(name.buf) - 1] = 0;
	}
	
	trace_thread_name_set(
		(u32_t)thread->thread_id,
		name
		);
#endif
}

void sys_trace_isr_enter(void)
{
	trace_isr_enter();
}

void sys_trace_isr_exit(void)
{
	trace_isr_exit();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	trace_isr_exit_to_scheduler();
}

void sys_trace_idle(void)
{
	trace_idle();
}

void sys_trace_void(u32_t id)
{
	trace_void(id);
}

void sys_trace_end_call(u32_t id)
{
	trace_end_call(id);
}

#endif
